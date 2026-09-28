#define _POSIX_C_SOURCE 200809L
#include "terminal_ui.h"
#include "app_log.h"
#include "log_internal.h"

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios saved_term;
static struct sigaction saved_int, saved_term_signal;
static volatile sig_atomic_t interrupted;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t renderer;
static int active, stopping;
static char input_text[1024], prompt_text[128], busy_text[128];

static void on_signal(int signal_number)
{
    interrupted = signal_number;
}

/* Limit lines to the terminal width and suppress terminal control bytes. */
static void print_text(const char *text, unsigned width)
{
    for (unsigned i = 0; text[i] && i < width; ++i) {
        unsigned char ch = (unsigned char)text[i];
        putchar(ch >= 32 && ch != 127 ? ch : ' ');
    }
    fputs("\033[K", stdout);
}

static void draw(void)
{
    log_snapshot logs;
    char input[sizeof input_text], prompt[sizeof prompt_text], busy[sizeof busy_text];
    pthread_mutex_lock(&lock);
    memcpy(input, input_text, sizeof input);
    memcpy(prompt, prompt_text, sizeof prompt);
    memcpy(busy, busy_text, sizeof busy);
    pthread_mutex_unlock(&lock);
    log_get_snapshot(&logs);

    struct winsize size = {0};
    (void)ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    unsigned rows = size.ws_row ? size.ws_row : 24;
    unsigned columns = size.ws_col ? size.ws_col - 1 : 79;
    unsigned visible = rows > 3 ? rows - 3 : 0;
    if (visible > LOG_LINES) visible = LOG_LINES;
    size_t first = logs.count > visible ? logs.count - visible : 0;

    fputs("\033[?25l\033[H", stdout);
    if (rows > 1) {
        char title[180];
        snprintf(title, sizeof title, "Logs | %s", *busy ? busy : "IDLE");
        print_text(title, columns);
        fputs("\r\n", stdout);
    }
    for (unsigned i = 0; i < visible; ++i) {
        print_text(first + i < logs.count ? logs.lines[first + i] : "", columns);
        fputs("\r\n", stdout);
    }
    if (rows > 2) {
        print_text("--- help / back / exit ---", columns);
        fputs("\r\n", stdout);
    }
    char line[1200];
    snprintf(line, sizeof line, "%s> %s", prompt, input);
    size_t length = strlen(line);
    print_text(line + (length > columns ? length - columns : 0), columns);
    fputs("\033[J\033[?25h", stdout);
    fflush(stdout);
}

static void *render_loop(void *unused)
{
    (void)unused;
    const struct timespec interval = {0, 100000000};
    for (;;) {
        pthread_mutex_lock(&lock);
        int stop = stopping;
        pthread_mutex_unlock(&lock);
        if (stop) break;
        draw();
        nanosleep(&interval, NULL);
    }
    return NULL;
}

int terminal_start(void)
{
    const char *plain = getenv("CLI_PLAIN");
    const char *term = getenv("TERM");
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO) ||
        (plain && strcmp(plain, "1") == 0) || (term && strcmp(term, "dumb") == 0)) {
        return 0;
    }
    if (tcgetattr(STDIN_FILENO, &saved_term)) return -1;

    interrupted = 0;
    struct sigaction action = {0};
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, &saved_int)) return -1;
    if (sigaction(SIGTERM, &action, &saved_term_signal)) {
        sigaction(SIGINT, &saved_int, NULL);
        return -1;
    }
    struct termios raw = saved_term;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw)) {
        sigaction(SIGINT, &saved_int, NULL);
        sigaction(SIGTERM, &saved_term_signal, NULL);
        return -1;
    }
    stopping = 0;
    input_text[0] = busy_text[0] = '\0';
    snprintf(prompt_text, sizeof prompt_text, "cli");
    log_set_live(1);
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
    int error = pthread_create(&renderer, NULL, render_loop, NULL);
    if (error) {
        log_set_live(0);
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_term);
        sigaction(SIGINT, &saved_int, NULL);
        sigaction(SIGTERM, &saved_term_signal, NULL);
        errno = error;
        return -1;
    }
    active = 1;
    return 1;
}

void terminal_stop(void)
{
    if (!active) return;
    pthread_mutex_lock(&lock);
    stopping = 1;
    pthread_mutex_unlock(&lock);
    pthread_join(renderer, NULL);
    draw();
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_term);
    fputs("\033[?25h\033[0m\r\n", stdout);
    fflush(stdout);
    log_set_live(0);
    sigaction(SIGINT, &saved_int, NULL);
    sigaction(SIGTERM, &saved_term_signal, NULL);
    active = 0;
}

int terminal_interrupted(void)
{
    return interrupted;
}

void terminal_set_busy(const char *command)
{
    pthread_mutex_lock(&lock);
    snprintf(busy_text, sizeof busy_text, "%s", command ? command : "");
    pthread_mutex_unlock(&lock);
}

static void set_input(const char *prompt, const char *line)
{
    pthread_mutex_lock(&lock);
    snprintf(prompt_text, sizeof prompt_text, "%s", prompt);
    snprintf(input_text, sizeof input_text, "%s", line);
    pthread_mutex_unlock(&lock);
}

int terminal_read_line(const char *prompt, char *line, size_t capacity)
{
    size_t length = 0;
    int overflow = 0;
    int escape = 0;
    line[0] = '\0';
    set_input(prompt, line);

    while (!interrupted) {
        struct pollfd fd = {.fd = STDIN_FILENO, .events = POLLIN};
        int result = poll(&fd, 1, 100);
        if (result < 0 && errno == EINTR) continue;
        if (result < 0) { app_log_errno("poll"); return 0; }
        if (!result) continue;
        unsigned char ch;
        if (read(STDIN_FILENO, &ch, 1) != 1) return 0;
        if (escape) {
            if (escape == 1 && (ch == '[' || ch == 'O')) escape = 2;
            else if (escape == 1 || (ch >= 0x40 && ch <= 0x7e)) escape = 0;
            continue;
        }
        if (ch == 27) { escape = 1; continue; }
        if (ch == 4 && length == 0) return 0;
        if (ch == '\r' || ch == '\n') {
            set_input(prompt, "");
            if (overflow) { app_log_errorf("Input line too long"); return -1; }
            if (length) app_logf("%s> %s", prompt, line);
            return 1;
        }
        if (ch == 127 || ch == 8) {
            if (length) line[--length] = '\0';
        } else if (ch >= 32 || ch == '\t') {
            if (length + 1 < capacity) {
                line[length++] = (char)ch;
                line[length] = '\0';
            } else {
                overflow = 1;
            }
        }
        set_input(prompt, line);
    }
    return 0;
}
