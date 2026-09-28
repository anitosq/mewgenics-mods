# Improved Inventory: single-archive installer experiment

Tested 28 September 2026. This is an unpublished packaging experiment using
the frozen 0.3.1 payloads, not a new mod version. The two published downloads
remain the supported packages.

## Result

A single ZIP can produce the required layouts in both managers. A follow-up
using Vortex's normal **Replace the existing mod** operation passed replacement,
disable, removal and reinstall checks. The earlier side-by-side migration
failed; that failure does not establish that normal replacement is broken.
The intended release direction is one download, with manager-specific placement
handled inside the archive. Do not replace the published downloads yet: clean
metadata, Nexus update association and game smoke checks remain outstanding.

The candidate contains the Mewtator layout (DLL inside ImprovedInventory/)
and fomod/ModuleConfig.xml. The XML maps each payload individually into
Vortex's default game-root deployment: mods/ImprovedInventory.dll and
mods/ImprovedInventory/ for assets. It never deploys the nested DLL or the
fomod directory. No loader configuration is included or edited.

## Evidence

Environment: Vortex 2.7.1, Mewgenics extension 0.4.0, Default profile, usual
enabled mod collection. Public 0.3.1 was initially enabled; the older local
test package was disabled. Mewgenics remained closed throughout. No campaign
was opened or changed.

1. The first XML included an optional XML declaration. Vortex rejected it
   with Xml_InvalidCharacter at line 1, position 39. The install was cancelled,
   not forced. Removing the declaration allowed normal validation and
   installation. This resembles the upstream [XML-header issue](https://github.com/Nexus-Mods/Vortex/issues/17957).
2. The corrected ZIP installed through Vortex's actual FOMOD installer.
   Its staging folder contained all nine unchanged release payloads in the
   intended destinations. Eight files deployed with matching hashes;
   Vortex excludes CHANGELOG.md from deployment.
3. Installing the candidate while the public package was enabled exposed a
   cross-mod-type conflict. FOMOD uses the default game-root type; the public
   package uses the extension's mewgenics-mod type rooted at mods/. Disabling
   the public package then produced external-change prompts and removed
   eight paths also owned by the candidate. The candidate remained enabled.
   This was a local package switch, not a Nexus same-mod version update.
4. Disabling and redeploying the candidate after the public package was
   disabled restored the expected file layout. The compiled DLL's exported
   asset gate accepted Vortex's generated launch arguments. Disabling its
   asset entry through Load Order made the gate reject them; re-enabling
   restored acceptance. These are startup-gate checks, not a game launch.
5. Disabling the candidate later restored Vortex's backup copies of the old
   public files, leaving a DLL/assets on disk with both packages disabled.
   Thus this sequence did not meet the clean-removal requirement. A fresh
   Vortex profile with no prior installation was not tested.
6. The actual upstream Mewtator import service imported the same ZIP into a
   temporary external mods directory with spaces, selected ImprovedInventory/
   using description.json, and omitted the sibling fomod directory. All nine
   imported payload hashes matched the release. Its replace operation removed
   a deliberately added stale fixture file. Its DLL service discovered exactly
   one DLL, discovered none when disabled, and generated a manifest in a
   temporary fake game directory. The compiled asset gate accepted the enabled
   external path and rejected the disabled case. Mewtator GUI, uninstall and
   gameplay were not tested in this experiment.

Mewtator source revision: dd48fd8c04c9257af346363bd8fc7384308fd587.
See its [import service](https://github.com/dancomstock/mewtator/blob/dd48fd8c04c9257af346363bd8fc7384308fd587/app/core/services/mod_import_service.py)
and [DLL service](https://github.com/dancomstock/mewtator/blob/dd48fd8c04c9257af346363bd8fc7384308fd587/app/core/services/dll_injection_service.py).

## Integration issue

The extension's ordinary asset installer sets both the mod type and a modName
attribute used to associate the asset folder with its Vortex entry. FOMOD
bypasses that installer and does neither. The existing Load Order label still
resolved through metadata from the disabled public package, so it does not
prove correct association on a fresh installation.

The next investigation should verify asset association without old mod metadata
and a real Nexus same-mod version update.
An extension-supported mixed DLL/assets layout is one possible route; changes
to the installed extension were not made. Do not compensate by silently
shipping a replacement chainloader.ini, duplicating DLLs, or changing users'
loader search settings. A game smoke test should follow once lifecycle checks
pass. The XML-only prototype is not sufficient evidence for a universal release.

## Follow-up: normal replacement

The exact corrected ZIP was copied to a private path with the same basename as
the installed 0.3.1 archive. Vortex offered **Replace the existing mod**, which
was selected. This local replacement purged the old type's files before deploying
the FOMOD game-root layout. No external-change prompts or backup copies appeared.
All nine staged and eight deployed payloads matched the frozen 0.3.1 hashes;
the startup asset gate accepted the generated launch arguments. Disabling it
removed all eight deployed files with no restored backups. Removing its Vortex
entry and reimporting the same ZIP also produced the correct layout and gate
result after automatic deployment finished.

This is a local same-ID replacement and reinstall, not a downloaded Nexus update
or a virgin Vortex database. Disabled 0.3.0 and local-test metadata still existed,
so the fresh-user load-order association question remains open. The game stayed
closed. Chainloader configuration was unchanged. Evidence and checker are in
`work/backups/fomod-replacement/` and `work/check_fomod_vortex.py`.

The packaging tool now has an opt-in `--layout universal` candidate mode. It
generates a single archive containing one DLL and declarative, choice-free file
mappings. The existing default remains for reproducing the published packaging
until release validation is complete. Packaging tests check both manager layouts,
unchanged payload bytes and the absence of duplicate DLLs or installer choices.

## Cleanup and local reproduction

Removed the candidate through Vortex, re-enabled the public Nexus 0.3.1
installation and verified all nine staging hashes and eight deployed hashes.
The original chainloader.ini and modlist.txt bytes matched the pre-test backup.
The candidate's game-root deployment manifest and temporary .vortex_backup
files were gone. Other mods retained their enabled states. Public archives,
release versions and listings were not changed.

Private scripts: work/build_fomod_probe.py and work/check_fomod_probe.py.
The builder verifies the frozen Mewtator ZIP and each payload, then adds one
requiredInstallFiles/file XML entry per payload, with no install choices.
The checker invokes the pinned Mewtator services in temporary directories.
Private log/config evidence: work/backups/fomod-probe-1/.

After the replacement follow-up, restored public 0.3.1 through Vortex's
**Downloads** entry and **Update current profile**. Installing the preserved
archive through Install From File restored the files but lost its Nexus metadata;
using the original download record restored the name, version and grouping too.
The final Mods view showed Improved Inventory 0.3.1 enabled, with the older local
test entry disabled. Rechecked all nine staging and eight deployed hashes and
byte-identical chainloader configuration/modlist; no FOMOD root deployment
manifest remained. No save files or public downloads changed.

Corrected candidate:
outputs/candidates/fomod-probe-2/ImprovedInventory-0.3.1-FOMOD-probe.zip

SHA-256: 96a9f05b5e91c4ca8a91262cac5d5b2dfbbaeb9e133dc1bb9da6c84385fd2a65

Its bundled installation text still describes the public two-package release.
This archive is only an experiment and must not be uploaded as a release.

## Release outcome, 28 September 2026

Version 0.3.2 adopts the universal ZIP as the default. The final archive passed
Vortex installation, asset enable/disable checks and a Storage/Trash smoke test.
The same ZIP passed Mewtator's actual import/replace and launcher services in
an external directory, plus an equipment-selection smoke test. See the
[candidate record](../releases/improved-inventory/0.3.2-candidate.md) for scope
and the [publication record](../releases/improved-inventory/0.3.2-publication.md)
for hosted download verification. The public update instructions retain the
remove-and-deploy step for older Vortex packages. Vortex's Load Order view can
show an unmanaged thumbnail because the FOMOD installer does not set the
extension's modName attribute; its checkbox and launch arguments still work.

The subsequent [association fix](improved-inventory-vortex-association.md)
adds installer metadata to restore that link. It is tested locally and has
not yet been published.
