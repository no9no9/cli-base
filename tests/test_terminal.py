"""Exercise live rendering on a PTY, with a background log producer."""
import fcntl
import os
import pathlib
import pty
import select
import shlex
import signal
import struct
import subprocess
import tempfile
import termios
import time

root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    work = pathlib.Path(directory)
    source = work / 'fixture.c'
    source.write_text(r'''
#define _POSIX_C_SOURCE 200809L
#include "cli.h"
#include "app_log.h"
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
static atomic_int stop;
static void pause_tick(void) {
    const struct timespec delay = {0, 100000000};
    nanosleep(&delay, NULL);
}
static void *producer(void *unused) {
    (void)unused;
    unsigned count = 0;
    while (!atomic_load(&stop)) {
        pause_tick();
        app_logf("ASYNC-%u", ++count);
    }
    return NULL;
}
static int slow(int argc, char **argv) {
    (void)argc; (void)argv;
    for (int i = 0; i < 8; ++i) {
        app_logf("PROGRESS-%d", i);
        pause_tick();
    }
    app_logf("FINISHED");
    return CLI_OK;
}
static const cli_command command = {
    .name="slow", .handler=slow, .usage="slow", .description="Slow operation"
};
static const cli_command *const commands[] = {&command};
int main(void) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, producer, NULL)) return 1;
    int result = cli_run(commands, 1);
    atomic_store(&stop, 1);
    pthread_join(thread, NULL);
    return result;
}
''')
    executable = work / 'fixture'
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=c11', '-pthread', '-I' + str(root / 'include'), str(source),
        str(root / 'src/core/cli.c'), str(root / 'src/core/app_log.c'),
        str(root / 'src/core/terminal_ui.c'),
                   str(root / 'src/core/register_watch.c'), str(root / 'src/core/register_io.c'), '-o', str(executable)], check=True)

    def session(ending):
        master, slave = pty.openpty()
        fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack('HHHH', 24, 80, 0, 0))
        original = termios.tcgetattr(slave)
        env = dict(os.environ, TERM='xterm', CLI_PLAIN='0')
        env.pop('DEBUG_MEMORY_FILE', None)
        process = subprocess.Popen([str(executable)], stdin=slave, stdout=slave,
                                   stderr=slave, env=env, cwd=work)
        def expect(marker):
            captured = b''
            deadline = time.monotonic() + 5
            while marker not in captured and time.monotonic() < deadline:
                if select.select([master], [], [], .2)[0]:
                    captured += os.read(master, 65536)
            assert marker in captured, (marker, captured[-1000:])
            return captured
        try:
            expect(b'Logs | IDLE')
            memory = work / 'dev/memory.bin'
            expect(b'REG [0x40000000]')
            with memory.open('r+b') as file:
                file.seek(0x40000000)
                file.write(struct.pack('=I', 0xabcdef12))
                file.flush()
            expect(b'REG [0x40000000] = 0xabcdef12')
            os.write(master, b'slo')
            expect(b'cli> slo')
            expect(b'ASYNC-')
            os.write(master, b'w\n')
            expect(b'Logs | slow')
            expect(b'PROGRESS-')
            expect(b'FINISHED')
            if ending == 'signal':
                process.send_signal(signal.SIGINT)
            elif ending == 'eof':
                os.write(master, b'\x04')
            else:
                os.write(master, b'exit\n')
            process.wait(timeout=5)
            assert process.returncode == 0
            assert termios.tcgetattr(slave) == original, 'Terminal settings not restored'
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            os.close(master)
            os.close(slave)
    for ending in ['exit', 'signal', 'eof']:
        session(ending)
print('PASS: live register refresh, async logs, partial input, running command updates, terminal restoration')
