# <Mod> test session <date/time>

Status: <prepared / running / exited / cleanup complete / cleanup needs attention>

## Setup

- Purpose and change under test:
- Source commit, mod version and archive SHA-256:
- Game build/hash; loader runtime/API:
- Vortex profile, enabled DLLs, asset paths and order:
- Save slot and whether it is a test copy:
- Local backup location and files preserved:
- Temporary configuration/markers created:
- Who performs the test (user or automation):

## Handoff

<Exact build ready, slot to open, a few actions to try, and expected results.
State whether the game is open or needs launching. Identify any action such
as End Day that would delete test items.>

## Results

| Check | Expected | Observed | Evidence | Result |
| --- | --- | --- | --- | --- |
| <One behavior> | <Outcome> | <Actual outcome> | <User report, log, screenshot or fixture> | <Pass / fail / not run> |

## Exit and cleanup

- Game exited normally:
- Log archived before next launch:
- Before/after counts or meaningful save differences:
- Intentional changes to preserve:
- Temporary config restored, or withheld because it changed externally:
- Test campaign archived; original slots preserved:
- Diagnostic markers removed:
- Active Vortex profile left for the user:
- Unresolved issue / next test:

Keep saves, private paths and raw logs locally under `work/`. Commit a short
summary that identifies the build and evidence without including save data.
