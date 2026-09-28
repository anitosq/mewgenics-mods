# Vortex Load Order association

Tested 28 September 2026 as a packaging probe for 0.3.2. The results below
describe that probe; the fix subsequently shipped in 0.3.3.

## Cause and fix

Mewgenics extension 0.4.0 links a deployed folder to an installed mod using
the `modName` attribute. Its ordinary installer sets this attribute; the
FOMOD route used by our universal ZIP bypasses that installer. Consequently,
the mod worked but its Load Order row said "Not managed by Vortex".

The packager now adds `vortex_override_instructions.json` at the archive root.
It sets `modName` to `ImprovedInventory`. Vortex consumes this file during
installation without deploying it. The instruction has a distinct `source`
key so Vortex's override merge preserves FOMOD's `installerChoices` attribute.
Mewtator imports the `description.json` subtree and ignores the sibling
installer files. DLL placement and runtime files are unchanged.

This uses Vortex's existing installer support. No extension edits, new
dependencies or user configuration are required.

## Validation

Probe: `outputs/candidates/vortex-association-1/ImprovedInventory-0.3.2-association-test.zip`.
SHA-256: `47b59155cbfdc332591fd600d1ffce2ccb7b0b4c45cfc9fbda6c00170520e52c`.
All ten original 0.3.2 archive entries are byte-identical; the metadata file is
the only addition. This probe is not a release archive.

- Vortex 2.7.1, Mewgenics extension 0.4.0, Default profile with the existing
  mod collection. Game closed; no save accessed.
- Installed the probe with Install From File and Replace the existing mod.
  Load Order then recognized the mod and no longer showed the warning.
- All nine staged and eight deployed payload hashes matched public 0.3.2.
  The override file was absent from staging and the game directory. No
  deployment backups appeared; chainloader.ini was unchanged.
- Unchecking Load Order removed the asset launch argument and the compiled
  startup guard rejected it. Rechecking restored the argument and passed.
- Actual Mewtator services imported and replaced the probe in an external
  directory containing spaces. Payload hashes matched; replacement removed
  stale files; discovery found one DLL when enabled and none when disabled.
  Its generated launch arguments passed the compiled asset guard.
- Packaging regression tests check the metadata, unchanged runtime layout,
  one DLL and no interactive installer choices.

The local test was imported as a separate Vortex record, leaving the original
download available as Uninstalled. The fixed copy is enabled, with local
version metadata `0.3.2+vortex-fix` and display name Improved Inventory. The original
cached Nexus download still matches the published archive. Install From File
did not retain the download's Nexus metadata; this is not a hosted update test.
Gameplay was not rerun because all runtime files are byte-identical.
Collection installation was not tested; Vortex's dependency-install path can
clear override instructions and needs separate investigation before claiming
collection support.

## Sources and reproduction

- Installed Mewgenics extension 0.4.0: `installMod`, `deserializeLoadOrder`
  and the `!loEntry.modId` warning branch in `index.js`.
- [Vortex source at 5b1fdef](https://github.com/Nexus-Mods/Vortex/tree/5b1fdefadd42405c1f00cf725173e62d46a27739/src/renderer/src/extensions/mod_management):
  `OVERRIDE_INSTRUCTIONS_FILENAME`, `InstallManager.ts` override loading,
  merging and `processAttribute`. The installed 2.7.1 renderer has the same
  relevant behavior.
- Mewtator source revision `dd48fd8c04c9257af346363bd8fc7384308fd587`.
- Private checks: `work/check_association_mewtator.py` and
  `work/check_vortex_032_public.py`. The latter accepts `enabled unchecked`
  to verify the disabled asset argument without removing deployed files.

Before publishing, make a new versioned candidate and run its package checks.
Do not overwrite the public 0.3.2 archive with this probe. Verify the hosted
update retains its Nexus association as well as the Load Order folder link.

## Published outcome

Version 0.3.3 passed the candidate and hosted-download checks. The public Nexus
copy retains its name, version, thumbnail and managed Load Order association.
The local probe was removed through Vortex and replaced by that public copy.
See the [publication record](../releases/improved-inventory/0.3.3-publication.md).
