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

## Localization

`translations.csv` contains the main panel, Pins, room names, tooltip and player
status messages for all ten languages in the current game's `combined.csv`:
English (`en`), Spanish (`sp`), French (`fr`), German (`de`), Italian (`it`),
Brazilian Portuguese (`pt-br`), Russian (`ru`), Korean (`ko`), Japanese (`ja`)
and Simplified Chinese (`zh-cn`). Stat terminology follows the game; the Russian
Appeal label is abbreviated to fit. These are our draft translations, not
community-reviewed translations. Diagnostic logs remain English.

Edit the UTF-8 CSV with a CSV-aware editor. Keep both header rows, the stable
`AUTO_FURNITURE_` keys and each message's `%1`/`%2` placeholders. Placeholders may
be reordered but not added or removed. Quote fields containing commas. Do not
add game markup. `localization.py` validates the table and generates the English
fallback header and `data/text/combined.csv.append`. Empty language cells become
English in the generated file. The package includes the generated CSV; it does
not include the source CSV or diagnostic DLL.

The UI uses the game's selected language and native fonts. It resolves strings
when entering furniture mode and opening a room panel, not on every frame.
Opening language settings closes the panel; reopen the room's wand afterward.
Missing keys, empty or oversized text, invalid Unicode, mismatched placeholders
and unexpected markup retain the built-in English label. CSV edits need a game
restart. Text files remain editable and are not part of the DLL/SWF pairing hash.
Normal rooms get localized names; unknown modded room names stay readable.

The build's native tests cover Unicode conversion and truncation, fallback,
placeholder formatting, room names, status translation and English diagnostics.
Eight Python checks cover complete language columns, invalid catalogs, status
coverage, generated assets and the existing safety guards. The native UI test
also accepts a generated fixture covering all 730 catalog/language pairs. It
checks UTF-8/UTF-16 round trips, native status lookup, placeholder substitution,
returned-item counts and Undo diagnostic-to-player-message mapping. The installed
Edmundm font metrics were also used to check 144 fully covered labels; the
Spanish Clear label was shortened after this check. Fallback fonts still need
in-game checks. Neither offline check establishes visual correctness.

Localization ships in 0.2.0. Do not replace frozen release archives. See the
0.2.0 publication checkpoint under `docs/releases/auto-furniture` for host status.

### Full-language verification

On 6 October 2026, the owner reported testing the happy path in every supported
language, with no untranslated, clipped or missing text, then closed the game.
The managed session used the disposable rightmost Home (slot 3) and the existing
mod collection. This is owner-reported happy-path evidence, not an isolated-mod
test or proof that every error branch was triggered.

Private evidence is under `work/20261006-210838`; pre-launch logs, the active mod
list and tested file hashes are in `work/localization-launch-20261006-210837`.
Cleanup restored the installed DLL and loader configuration byte-for-byte, all
save-file hashes matched, and the active-session journal was removed.

The follow-up source audit found all 43 player status/error messages represented
in the 73-key catalog across ten languages. This includes invalid bounds, no
selection, infeasible Max, unmet Min, stale results, unsupported furniture,
busy or unreadable rooms, failed placement, rollback warnings, empty returns and
Undo failures. Empty Pins, pagination, rarity suffixes and room-name templates
are also covered. Eight offline checks passed, including all 730 native
translation round trips. No product changes were needed for this follow-up.

Font-metric checks found all 258 status strings fully covered by Edmundm's
metrics fit within two 564-pixel lines at size 17. This does not prove native
wrapping or fallback-font rendering: rare error messages have not been visually
verified in-game, and destructive failure conditions were not injected into the
save. Startup/diagnostic logs intentionally remain English; unknown modded room
names retain their readable identifier, and item names use the game's own
localization with the existing identifier fallback.

### One-button prototype evidence

On 6 October 2026, the owner tested the diagnostic build on the disposable
slot-3 copy with the existing mod collection. Their screenshot shows the
Simplified Chinese Calculate label with readable glyphs, centered in its button.
They confirmed switching back to English restores Calculate in the same session.
This verifies the CSV lookup and native font rendering for that label. Calculation
completion was not explicitly confirmed; this was not an isolated-mod test.
The build, native UTF/fallback checks and six Python checks passed. After normal
exit, cleanup restored the installed DLL and loader configuration byte-for-byte;
all save-file hashes matched the pre-test backup. No release was packaged or
published. The remaining UI text was English in that prototype. It used the
older example's `zh` header; the full catalog now matches the current game's
`zh-cn` header in the same column position.

## 0.2.2 performance candidate

Panel/Pins content refreshes only when display inputs change or controls need
initialization. Native lifetime, position and hover checks continue each frame.
The UI fixture stubs only the native rendering calls and executes the production
content functions, including failed-child retries and language changes.

Solver sorting is stable and area divisors are computed once per search. Stop
callbacks run between merge passes, every 64 rebuild/item checks and every 256
placement evaluations. The worker reserves up to 100 ms of its 1.2-second budget
for utility filling when utility candidates exist. This is a cooperative budget,
not a hard wall-clock guarantee; capture/preparation and Apply/Undo are separate.
Tests retain cancellation-free helpers for deterministic fixed-iteration checks.
See the [candidate record](../../docs/releases/auto-furniture/0.2.2-beta.1-candidate.md).
