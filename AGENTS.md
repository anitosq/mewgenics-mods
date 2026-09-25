# Project instructions

Read [the mod development guide](docs/mod-development.md) when starting or
resuming mod work. Use the mod's README, compatibility file and release record
for current status; dated research notes describe experiments at that time.

- Use Conventional Commits 1.0.0 for every commit, including merges, reverts
  and amendments: `<type>[optional scope][!]: <description>`. Mark breaking
  changes with `!` or a `BREAKING CHANGE:` footer. Separate bodies and footers
  from the subject with a blank line.
- Start with the player's problem and a small in-game milestone. Prefer a
  narrow data patch when it can do the job; investigate native hooks only
  where needed. Keep independent mods separately versioned under `mods/`.
- Check gameplay assumptions against game data and effective runtime behavior.
  File names and raw enum values alone do not establish game mechanics.
- Use the game's fonts, symbols and sounds early. Keep rendering, hover and
  hit testing in the same coordinate system. Test popup input and layering
  after transfers and UI rebuilds, not only when first opened.
- Close the game normally before replacing loaded files. Record the active
  profile, mods, build and save slot before testing. Use a backed-up campaign
  copy for transfers and destructive game actions; don't advance the main
  campaign or roll back intentional play as test cleanup.
- Keep Vortex deployment idle during managed diagnostic sessions. Restore
  temporary configuration only if it still matches what the session wrote.
  Preserve logs before another launch overwrites them. Tell the user which
  build and slot are ready when handing over testing.
- Keep DLL discovery and data/asset load order separate. Prefer targeted
  patches over whole-file replacements. Verify dependencies with only the
  loader and our mod before adding the normal mod collection.
- Get a normally starting, Vortex-installable package working early. Release
  builds must not depend on local paths, a diagnostic marker or a test runner.
  Native hooks must validate the supported executable and their entry bytes;
  paired DLL/assets must fail safely when missing, disabled or mismatched.
- For releases, follow [the publishing workflow](docs/releases/README.md).
  Keep Nexus display names version-free; use version fields and versioned
  archive names. Build once and use the tested archive on both hosts.
- Write player copy about features, installation and relevant known issues.
  Keep test evidence in release records and license terms in dedicated files.
  Use the humanizer skill when available for public copy. Keep required notices.
- Review audience and relevance before polishing sentences. Assume a first-time
  player has never seen this chat or a private build. Lead with a concrete
  player benefit; omit private migrations, routine UI correctness and internal
  test history from listings. Include upgrade advice only for versions the
  public could actually obtain. Use the publishing workflow's audience check.
- Distinguish user-confirmed behavior, logs, automated fixtures and untested
  cases. Don't turn fixture results into claims of visual correctness or FPS.
  Run checks appropriate to the change; avoid redundant test runs.

Preserve the user's existing authorizations. These instructions do not add an
approval step for routine research, development, fixes or documentation work.
