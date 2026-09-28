#define _POSIX_C_SOURCE 200809L
#include "app_log.h"
#include "log_internal.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static char lines[LOG_LINES][LOG_WIDTH];
static size_t next_line;
static size_t line_count;
static int live;

void log_set_live(int enabled)
{
    pthread_mutex_lock(&lock);
    live = enabled;
    pthread_mutex_unlock(&lock);
}

static void append_line(const char *text, size_t length)
{
    if (length >= LOG_WIDTH) {
        length = LOG_WIDTH - 1;
    }
    memcpy(lines[next_line], text, length);
    lines[next_line][length] = '\0';
    next_line = (next_line + 1) % LOG_LINES;
    if (line_count < LOG_LINES) {
        ++line_count;
    }
}

static void write_log(FILE *stream, const char *format, va_list args)
{
    char text[2048];
    vsnprintf(text, sizeof text, format, args);

    pthread_mutex_lock(&lock);
    if (!live) {
        fputs(text, stream);
        size_t length = strlen(text);
        if (!length || text[length - 1] != '\n') {
            fputc('\n', stream);
        }
        fflush(stream);
    }

    const char *start = text;
    const char *end;
    while ((end = strchr(start, '\n')) != NULL) {
        append_line(start, (size_t)(end - start));
        start = end + 1;
    }
    if (*start || !*text) {
        append_line(start, strlen(start));
    }
    pthread_mutex_unlock(&lock);
}

void app_logf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    write_log(stdout, format, args);
    va_end(args);
}

void app_log_errorf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    write_log(stderr, format, args);
    va_end(args);
}

void app_log_errno(const char *prefix)
{
    int saved = errno;
    char message[256];
    if (strerror_r(saved, message, sizeof message) != 0) {
        snprintf(message, sizeof message, "errno %d", saved);
    }
    app_log_errorf("%s: %s", prefix, message);
}

void log_get_snapshot(log_snapshot *snapshot)
{
    pthread_mutex_lock(&lock);
    snapshot->count = line_count;
    size_t first = (next_line + LOG_LINES - line_count) % LOG_LINES;
    for (size_t i = 0; i < line_count; ++i) {
        memcpy(snapshot->lines[i], lines[(first + i) % LOG_LINES], LOG_WIDTH);
    }
    pthread_mutex_unlock(&lock);
}
