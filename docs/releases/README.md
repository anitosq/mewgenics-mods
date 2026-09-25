# Release and publishing workflow

Policy established 25 September 2026. No public release exists yet; a local candidate packager is available.

## Destinations and source of truth

Publish mods on **Nexus Mods**; players install them through **Vortex**.
Keep development and tagged releases in this GitHub repository. Each standalone
mod gets its own versions, archive and Nexus page; closely related variants
and compatibility patches can share that mod's page.

**Build once, test that archive, upload that same archive to both sites.**
Never rebuild separately for Nexus and GitHub. GitHub is the canonical record
of the source commit, release notes and submitted archive checksum. Nexus is
the Vortex discovery/download channel. A release is synchronized only after
both downloads and their version information have been checked.

Use the manual first-release procedure below. Later uploads can use Nexus's
official action, which requires an existing mod page and at least one uploaded
file. This avoids inventing file IDs or credentials before the first upload.
[Official upload action](https://github.com/Nexus-Mods/upload-action/tree/v1.0.0-beta.10).

## Versions in this multi-mod repository

| Field | Improved Inventory example |
| --- | --- |
| Per-mod version file | `mods/improved-inventory/VERSION` |
| First public test version (proposed) | `0.1.0-beta.2` |
| Git tag | `improved-inventory/v0.1.0-beta.2` |
| GitHub release title | `Improved Inventory 0.1.0-beta.2` |
| Installable asset | `ImprovedInventory-0.1.0-beta.2.zip` |
| Archive checksum asset | `ImprovedInventory-0.1.0-beta.2.zip.sha256` |
| Nexus mod/file version | `0.1.0-beta.2` |

The version file feeds the DLL's version/log output, package metadata,
archive name and release checks. Never use GitHub's
automatic source-code ZIP as the player download.

Use semantic versions: patch for compatible fixes, minor for compatible new
features, major for breaking installation/configuration changes. During `0.x`
development, describe breaking changes explicitly. Use `-beta.N` for public
test builds and mark them as GitHub prereleases and Nexus test/optional files,
not the default stable download. Test Nexus/Vortex's handling of the chosen
prerelease version string on the first upload. Nexus recommends semantic
versioning and clear requirements. [Author guidance](https://help.nexusmods.com/article/136-best-practices-for-mod-authors).

Unrelated mods do not share a version or require simultaneous releases.
Select a mod by its tag prefix, never by this repository's global `latest`
release. Use `--latest=false` for GitHub releases in this multi-mod repository.

## What runs automatically today

The [GitHub checks](../../.github/workflows/ci.yml) run on main-branch pushes,
pull requests and manual dispatch. They use pinned action commits and a pinned
compiler, run fixture tests and generate original UI assets. They need no
game files, saves or publishing credentials.

Full DLL builds and game-font checks still run locally against the verified
game executable. CI does not currently build an installable release, publish
to either site, or establish in-game compatibility. Keep build reports and
private runtime logs locally; only publish a sanitized test summary.

## Prepare a release candidate

1. Complete the mod's [readiness checklist](../../mods/improved-inventory/RELEASE_CHECKLIST.md).
   Resolve installation and startup blockers before making a player archive.
2. Set its version and finalize its [changelog](../../mods/improved-inventory/CHANGELOG.md).
   Write player-facing notes using the [template](release-notes-template.md).
3. Commit the exact source and metadata. Require a clean working tree and
   passing checks for that commit. Build from that commit using the supported
   game and pinned toolchain; capture commit ID, compiler version, executable
   hash and Steam build ID in a local build report.
4. Package only the allowlisted DLL, original generated SWF/append file,
   metadata, installation instructions, MIT license and applicable notices.
   Do not include the loader, replacement `chainloader.ini`, test markers,
   logs, saves, game assets, compilers or other mods. The packager uses an explicit payload allowlist.
5. Install the ZIP through Vortex and finish clean-install, upgrade,
   disable/remove and gameplay checks. Test the frozen archive, not loose
   development files. Any payload change requires repackaging and retesting.
6. Save the archive and a SHA-256 sidecar in ignored
   `outputs/releases/improved-inventory/<version>/`. The sidecar contains one line:
   `<lowercase SHA-256>  ImprovedInventory-<version>.zip`.
   Also record the deployed payload's relative filenames and SHA-256 hashes
   for comparison if a hosting service repacks an archive.

Do not publish until the supported game build, Mewjector requirement,
installation steps and known limitations are accurate. A first beta is a
reasonable target after these checks; version `1.0.0` is not needed to start
collecting public feedback.

## First publication: coordinated manual uploads

1. Create and push an annotated per-mod tag pointing at the tested commit.
   Create a **draft** GitHub release with the tested ZIP, checksum and finished
   notes. Upload every asset before publication. Do not retag a published
   version or replace its payload silently.
2. Create the Mewgenics Nexus page with screenshots, feature summary,
   installation/update/removal instructions, compatibility and known issues.
   Add Mewjector as a requirement; list Vortex plus its Mewgenics extension as
   the tested installation method. Link the repository and MIT license, and
   make Nexus permissions consistent with MIT for our original work.
3. Upload the **same ZIP** with the exact same version. Enable mod-manager
   downloads and keep the page/file unpublished or hidden during preparation
   where the site permits it. Complete Nexus processing/scanning. Use the
   reviewed changelog on both sites, adapting formatting only.
4. Publish the prepared releases in the same session. For the first beta,
   preserve its prerelease/test labeling. The two services cannot publish
   atomically; mark the mirror as pending until verified rather than implying
   instant synchronization.
5. Download from both sites, check the version and archive checksum, and test
   the Nexus mod-manager download in Vortex. If Nexus repacks the container,
   compare the extracted file list and file hashes rather than claiming the
   outer ZIP hashes must match. Any changed DLL/SWF/metadata is a failure.
6. Record source commit, tag, submitted checksum, payload verification,
   GitHub URL, Nexus URL, file/version IDs and test result in a per-version
   record under `docs/releases/improved-inventory/`. Link both destinations from
   the mod README. Only then mark synchronization complete.

Example commands, **only after a tested package exists**; run from the repo
root. The archive is produced by `tools/package_improved_inventory.py`, not these commands:

```powershell
$repo = 'anitosq/mewgenics-mods'
$version = (Get-Content mods/improved-inventory/VERSION -Raw).Trim()
$tag = "improved-inventory/v$version"
$bundle = "outputs/releases/improved-inventory/$version/ImprovedInventory-$version.zip"
$notes = "mods/improved-inventory/releases/$version.md"
git tag -a $tag -m "Improved Inventory $version"
git push origin $tag
gh release create $tag $bundle "$bundle.sha256" --repo $repo --verify-tag --draft --prerelease --latest=false --title "Improved Inventory $version" --notes-file $notes
```

Check each command succeeds before continuing. Remove `--prerelease` for a
stable release. GitHub recommends drafting, attaching assets, then publishing
when using immutable releases. Enable repository release immutability before
the first publication if available; it locks published assets and tags while
allowing notes to be corrected. It has not been enabled by this setup.
[GitHub guidance](https://docs.github.com/en/code-security/concepts/supply-chain-security/immutable-releases),
[CLI reference](https://cli.github.com/manual/gh_release_create).

## Subsequent releases: automate the Nexus mirror

After the first manual release, configure the official
`Nexus-Mods/upload-action`. As reviewed, its latest release is
`v1.0.0-beta.10`, commit `c96019556046053aa26044b44396cd38929daf23`.
Pin the reviewed commit when adding the publishing workflow; review changes
before upgrading this beta integration.

The intended workflow is:

```text
tested commit -> frozen ZIP -> draft GitHub release -> publish
    -> download that release's ZIP -> verify checksum -> Nexus upload
    -> verify Nexus download/version -> mark mirror complete
```

The workflow is not wired yet. Its implementation must:

- Run only for published `improved-inventory/v...` releases, excluding other mods.
  Mirror stable releases automatically; require explicit selection of a
  separate test file/category for prereleases. Do not replace stable with beta.
- Download the explicitly named ZIP and checksum from that tag. Validate tag,
  package version, manifest and checksum; never rebuild on the publishing job.
- Use a dedicated `nexus-improved-inventory` environment. Store the author API key
  as `NEXUSMODS_API_KEY` there, never in source or chat. Configure the file ID
  from Nexus's Files > Advanced/manage-files UI; do not confuse it with the
  legacy ID in a download URL. Configure the mod ID required by the action
  when sending a changelog. Bind each mod and stable/test channel to its own
  target file record. These settings do not exist yet.
- Pass the exact version, reviewed changelog, mod-manager download settings
  and mod-version update setting. Keep old working files available until the
  new download is verified. The action returns a `version_id`; record it.
- Serialize uploads per mod/channel and report the destination URL/version ID
  and verification result in the run summary. Use read-only GitHub permissions
  unless a specific step needs to update a release note.
- Have a retry path that checks Nexus first. The action's interface does not
  expose an idempotency key: a timed-out upload may already have succeeded.
  Do not blindly rerun the upload or create duplicate versions.

The official action updates versions of an existing Nexus file. Its current
inputs, including `api_key`, `file_id`, `filename`, `version`, `mod_id` and
`changelog`, are documented in the
[pinned action source](https://github.com/Nexus-Mods/upload-action/blob/c96019556046053aa26044b44396cd38929daf23/action.yml).
Do not copy the upstream example's source-zipping step: our native mod needs
the already built and tested release asset.

## Failure and rollback

If GitHub succeeds but Nexus fails, keep the GitHub artifact/tag and mark Nexus
as pending. Retry mirroring that same artifact after checking for an existing
upload. Do not issue a new code version for a network failure. If verification
finds different payloads, stop promoting the Nexus file and investigate.

For a shipped code or packaging defect, publish a new patch/beta version.
Retain the prior known-good download, document the regression and revert the
recommended file while preparing the fix. Follow Nexus archive/update controls
to preserve its update chain; do not delete/reupload files just to reset them.
Never roll back a user's save as part of a mod update or uninstall.

## Current next step

The normal-startup candidate was installed through Vortex and passed basic
in-game checks with the existing mod collection. See the
[candidate record](improved-inventory/0.1.0-beta.1-candidate.md). Finish the clean-setup,
installation lifecycle and remaining gameplay/shutdown checks before creating
a public release. The diagnostic DLL remains a separate build.
