"""Mock-only behavior checks; never executes the real-MMIO binary."""
import os
import pathlib
import subprocess
import sys
import tempfile

exe = pathlib.Path(sys.argv[1]).resolve()
if exe.parent.name != 'mock':
    raise SystemExit('Tests require the mock build')
with tempfile.TemporaryDirectory() as d:
    memory = pathlib.Path(d) / 'memory.bin'
    env = dict(os.environ, DEBUG_MEMORY_FILE=str(memory))
    def run(commands):
        p = subprocess.run([str(exe)], input=commands, text=True,
                           capture_output=True, env=env, cwd=d, timeout=5)
        assert p.returncode == 0, p
        return p.stdout, p.stderr

    out, err = run('write 0x400000fc 0x12345678\nwrite 0x40000000 0xaabbccdd\nreg read 0x40000000\nexit\n')
    assert not err, err
    assert '0xaabbccdd' in out, out
    out, err = run('reg\nread 0x400000fc\nread 0x40000000\nback\nread 0x40000004\nexit\n')
    assert not err, err
    assert all(v in out for v in ['reg>', 'cli>', '0x12345678', '0xaabbccdd', '0x00000000']), out
    original = memory.read_bytes()
    out, err = run('write 0x40000001 1\nwrite 0x40001000 1\nwrite 0x40000000 0x100000000\nwrite 0x40000000 -1\nwrite 0x40000000 2 ' + 'x ' * 40 + '\n' + 'write 0x40000000 2 ' + 'x' * 1100 + '\nexit\n')
    assert memory.read_bytes() == original
    assert 'Usage:' in err and 'Too many arguments' in err and 'Input line too long' in err, err
    out, err = run('reg help\nhelp\nexit\nwrite 0x40000000 1\n')
    assert not err and 'Read a 32-bit register' in out, (out, err)
    assert memory.read_bytes() == original
    out, err = run('read 0x40000000')  # EOF without newline
    assert not err and '0xaabbccdd' in out, (out, err)
    # Decimal with leading zero is decimal, not C's octal notation.
    out, err = run('write 0x40000008 010\nread 0x40000008\nexit\n')
    assert not err and '0x0000000a' in out, (out, err)
print('PASS: menus, persistence, bounds, parsing, overflow, EOF')
