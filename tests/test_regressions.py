"""Regression checks in disposable build directories; no hardware access."""
import os
import pathlib
import shlex
import shutil
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    work = pathlib.Path(directory)
    source = work / 'menu.c'
    source.write_text('''#include "cli.h"
static const cli_menu leaf = {"leaf", NULL, 0};
static const cli_command leaf_command = {
    .name="leaf", .usage="leaf", .description="Leaf", .submenu=&leaf
};
static const cli_command *const middle_commands[] = {&leaf_command};
static const cli_menu middle = {"middle", middle_commands, 1};
static const cli_command middle_command = {
    .name="middle", .usage="middle", .description="Middle", .submenu=&middle
};
static const cli_command *const root_commands[] = {&middle_command};
int main(void) { return cli_run(root_commands, 1); }
''')
    compiler = shlex.split(os.environ.get('CC', 'cc'))
    subprocess.run(compiler + ['-std=c11', '-I' + str(root / 'include'),
                   str(source), str(root / 'src/core/cli.c'), '-o', str(work / 'menu')], check=True)
    def menu(commands):
        result = subprocess.run([str(work / 'menu')], input=commands, text=True,
                                capture_output=True, check=True, timeout=5)
        return result.stdout, result.stderr

    out, err = menu('middle leaf\nback\nback\nexit\n')
    assert out == 'cli> leaf> middle> cli> ' and not err, (out, err)
    out, err = menu('middle leaf help\nexit\n')
    assert out.endswith('cli> ') and 'Parent menu' in out and not err, (out, err)
    out, err = menu('middle nonexistent\nexit\n')
    assert out == 'cli> cli> ' and 'Unknown command' in err, (out, err)

    for name in ['src', 'include']:
        shutil.copytree(root / name, work / name)
    shutil.copy(root / 'Makefile', work / 'Makefile')
    extra = work / 'src/commands/extra.c'
    extra.write_text('const char regression_marker[] = "REMOVED_SOURCE_MARKER";\n')
    def build():
        subprocess.run(['make', '-s', '-j2'], cwd=work, check=True)
    build()
    executable = work / 'build/mock/debug_cli'
    assert b'REMOVED_SOURCE_MARKER' in executable.read_bytes()
    previous_time = executable.stat().st_mtime_ns
    build()
    assert executable.stat().st_mtime_ns == previous_time, 'Unchanged build relinked'
    extra.unlink()
    build()
    assert b'REMOVED_SOURCE_MARKER' not in executable.read_bytes(), 'Deleted source remains linked'
print('PASS: nested menus, unchanged builds, source deletion')
