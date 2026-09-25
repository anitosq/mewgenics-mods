"""Run Improved Inventory fixture checks without launching the game or reading saves."""
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
TESTS = ROOT / 'mods/improved-inventory/tests'


def main():
    compiler = ROOT / 'work/toolchains/zig-x86_64-windows-0.15.2/zig.exe'
    if not compiler.is_file():
        raise SystemExit('Missing native compiler. Run: python tools/bootstrap_zig.py')
    node = shutil.which('node')
    if node is None:
        raise SystemExit('Node.js must be available on PATH.')
    subprocess.run([sys.executable, '-m', 'unittest', 'discover', '-s', str(TESTS),
                    '-p', 'test_*.py'], cwd=ROOT, check=True)
    subprocess.run([node, '--test', str(TESTS / 'model.test.mjs')], cwd=ROOT, check=True)
    build = ROOT / 'work/native-tests'
    build.mkdir(parents=True, exist_ok=True)
    for name in ('native_filters', 'native_search', 'native_set_rows', 'native_hooks', 'native_lifetime', 'native_hit'):
        output = build / f'{name}.exe'
        # Each fixture uses only part of the shared static helper headers.
        subprocess.run([str(compiler), 'cc', '-O2', '-UNDEBUG', '-Wall', '-Wextra',
                        '-Werror', '-Wno-unused-function',
                        str(TESTS / f'{name}.c'), '-o', str(output)],
                       cwd=ROOT, check=True)
        subprocess.run([str(output)], cwd=ROOT, check=True)
    print('Improved Inventory fixture checks passed.')


if __name__ == '__main__':
    main()
