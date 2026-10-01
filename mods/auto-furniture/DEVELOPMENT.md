# Building Auto Furniture

Requires Python 3, Zig 0.15.2, Windows x64 and the supported Mewgenics executable.
Build from the repository root:

```powershell
python -B mods/auto-furniture/build.py --exe "<game>/Mewgenics.exe" --zig "<toolchain>/zig.exe"
python -B mods/auto-furniture/test_probe.py
& "<toolchain>/zig.exe" cc -O2 -UNDEBUG -Wall -Wextra -Werror mods/auto-furniture/test_solver.c -o mods/auto-furniture/build/test_solver.exe
./mods/auto-furniture/build/test_solver.exe
python -B mods/auto-furniture/package.py
```

The build also runs `test_ui.c` with assertions enabled, checking idle read
counts, modal input blocking and retained control lifetime guards.

The build rejects executables other than SHA-256
`4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea`.
Runtime startup validates executable identity, hook entry bytes and the enabled
UI assets. The shared startup guard supports Vortex's sibling DLL and Mewtator's
nested DLL, including external asset paths.

The packager requires a clean source commit and emits a deterministic allowlisted
ZIP, checksum and payload manifest. Build and package outputs are ignored.
Do not publish the diagnostic DLL or any files under work.

`test_probe.py` checks native snapshot safety, UI layout fixtures, input bounds,
asset pairing and disposable-save helpers. `test_solver.c` checks scoring,
placement, bounds, support and utility-fill behavior. These are offline checks,
not proof of in-game rendering or a full manager lifecycle.

`session.py` supports backed-up, disposable campaign testing. Read its commands
and safeguards before using it; never replace intentional player progress.
Gameplay and publication evidence belongs in docs/releases/auto-furniture.
