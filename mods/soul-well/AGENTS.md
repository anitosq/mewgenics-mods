# Soul Well development checks

- Follow the repository's owner-testing workflow. If live testing is explicitly authorized, use a journaled disposable campaign; restore temporary loader/settings/slot state and verify original save hashes. Release code must work without fixture markers.
- Before adding or changing a native hook, inspect every relative branch in its copied instructions. Short branches must remain within the copied span or land exactly on the trampoline jump-back boundary. Exercise each branch in an offline regression test before launching the game.
- Source: the 2026-10-14 combat-entry crash landed at SourceItem trampoline +0x1f. The default 16-byte copy left a short conditional branch outside the copied code. A 31-byte span preserves both branches; `source_trampoline_check` executes the direct and nested-parent paths.
