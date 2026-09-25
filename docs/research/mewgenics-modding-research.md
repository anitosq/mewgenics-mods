# Mewgenics modding: tools, loading, and this installation

Research and setup follow-up date: 24 September 2026. Initial research combined tool-author documentation, source-code inspection, and read-only examination of the installed game and Vortex files. This report now also records the subsequent authorized configuration repair and the user's runtime verification. No save was edited. DLL loading and hook installation are verified; gameplay effects and overall mod compatibility are not fully tested.

## The main finding

Your current setup uses **Vortex's Mewgenics extension to launch the game directly with `-modpaths`**. Neither Mewtator nor the separate Mewgenics Mod Manager is invoked by the generated launcher.

There really are two similarly named managers. The archive initially downloaded was **Mewgenics Mod Manager by ciaoShiny, Nexus mod 36**, not **Mewtator by animandan, Nexus mod 1**. Its bundled example mod was deployed without its executable. The user has since removed that package; the example is absent from the game folder and launch list.

**Current result:** Mewjector v3.0 is installed. The 13:40:37 launch loaded Bigger Wallet through an explicit `[LoadOrder]` entry, and its hook installed at 13:40:41. The running game was responding when checked. This resolves the missing loader and unwanted example findings below.

## 1. The tools and their roles

| Tool | Role | Relationship to your setup |
| --- | --- | --- |
| Vortex + ChemBoy1's Mewgenics extension | Downloads/deploys mods, maintains the asset-mod list, writes a launcher | Already doing the ordinary mod-launch work |
| Mewtator | Dedicated manager with asset extraction, launch settings, dependency checking, and optional DLL-loader integration | An alternative/additional authoring tool; not required by Vortex's asset loading |
| Mewgenics Mod Manager, ciaoShiny | Separate manager with drag-and-drop installation, folder detection, English/Chinese UI, and `info.json` metadata | Package removed; bundled example no longer deployed or enabled |
| Mewjector | Loads native DLL mods inside the game process | v3.0 installed; Bigger Wallet loading and hook installation verified |

Sources: [Vortex extension](https://www.nexusmods.com/site/mods/1691), [Mewtator](https://www.nexusmods.com/mewgenics/mods/1), [ciaoShiny's manager](https://www.nexusmods.com/mewgenics/mods/36), [Mewjector](https://github.com/githubuser508/mewjector).

A manager chooses and deploys files. The game's own asset loader interprets ordinary data mods. A DLL loader adds a separate route for native code. These responsibilities are easy to confuse because Nexus requirement lists sometimes recommend a manager as the installation method.

## 2. What I verified locally

| Evidence | Observation |
| --- | --- |
| Game directory | `C:\Program Files (x86)\Steam\steamapps\common\Mewgenics` |
| Steam application | 686060 |
| Installed Steam build ID | 25143593, from `appmanifest_686060.acf` |
| Vortex extension | 0.4.0, with a local changelog dated 20 September 2026 |
| Deployment method | `hardlink_activator`, recorded in the game's deployment manifest |
| Generated launcher | `launch.bat` runs `Mewgenics.exe -modpaths ...` |
| Listed enabled mod folders | 13, matching `mods\modlist.txt`; previously 14 before example removal |
| Mewtator executable in game root | Not present |
| MewgenicsModManager executable in game root | Not present |
| Mewjector's `version.dll` and `chainloader.ini` in game root | Both present after installation through Vortex |
| DLL gameplay payload | `mods\BiggerWallet\BiggerWallet.dll` present and loaded in the verified session |

The Windows executable's version fields were empty. I have **not** equated the Steam build ID with a particular human-readable game version. Bigger Wallet's downloaded filename targets **1.1.21239**, which is a compatibility claim about that package, not proof of the installed game's version.

The search-indexed Vortex page showed 0.3.4. The actual local extension is newer, so its installed code is the better source for this machine's behavior.

### The actual launch path

```text
Vortex deployment
  → game/mods/<individual mod folders>
Vortex load-order serialization
  → mods/modlist.txt
  → launch.bat containing -modpaths
Launch through Vortex's Custom Launch tool
  → Mewgenics.exe
  → native game asset loading
```

The installed extension's `setParameters` function constructs the command directly. Its default primary tool is “Custom Launch,” pointing to `launch.bat`. The separate Mewtator tool entry would launch `Mewtator.exe` if present; it is not part of this chain.

Local source: installed extension `index.js`, notably `setParameters` around line 1098, `serializeLoadOrder` around 1120, and tool registration around 1359. The same file recognizes `description.json` and content folders such as `data`, `swfs`, `audio`, `textures`, `levels`, and `shaders` when installing asset mods.

## 3. Setup repairs and remaining compatibility checks

### Resolved: the manager archive became an example gameplay mod

The downloaded `Mewgenics Mod Manager-36-1-0-1771215159.zip` contains:

```text
MewgenicsModManager.exe
config.json
Mods/example_Infinite_Rerolls/
    info.json
    preview.png
    data/classes/classes.gon
    data/classes/advanced_classes.gon
```

Initially, Vortex's staging directory for this package contained only `example_Infinite_Rerolls`. The same folder was deployed and appeared first in the generated launch list. The manager executable and its configuration were not deployed to the game root. After the user removed the package, I verified that the example folder and its launch entry were gone; the separate `Reroll` mod remains enabled.

This is consistent with the extension treating the archive as a content mod after detecting its nested `data` folder. It does not show that the manager was installed as an operational dependency, nor establish why the package was originally downloaded.

The removed sample's metadata describes 99 rerolls, but its class files contain `AddLevelUpRerolls 14`. The separate `Reroll` mod appends `AddLevelUpRerolls 99` to the same class definitions. Removing the sample eliminates this known overlap; their earlier combined behavior was not tested.

### Resolved: Bigger Wallet's missing loader and DLL path

[Bigger Wallet's author](https://www.nexusmods.com/mewgenics/mods/306) explicitly requires Mewjector and describes Mewtator as optional. Initially the DLL was deployed without Mewjector. After the user installed Mewjector, its first log still reported zero DLLs loaded: `ScanPath=mods` scans that directory, but does not descend into Bigger Wallet's subfolder. Adding a folder to `-modpaths` does not load its DLLs.

The authorized repair kept `ScanPath=mods` and added one explicit entry under the existing `[LoadOrder]` section. See section 6 for the exact configuration. The 13:40:37 log reports one DLL loaded successfully, followed by a Bigger Wallet hook installation at 13:40:41. No crash report belonged to that running process (PID 34924) when checked. Earlier exception reports exist, including one at 13:40:33 from a different process; their causes remain undiagnosed.

Loading and hook installation do not prove every gameplay effect: collecting more than 99 coins during an adventure still needs verification. The package metadata's empty `requirements` array also shows why metadata alone cannot establish that dependencies are satisfied.

### Better Loot remains installed through Vortex

Better Loot contains ordinary `.gon.merge` files, and its folder remains in Vortex's generated launch command. Its author lists a manager requirement but also documents manual installation and recommends Mewtator in the description. There is no manager executable dependency in the inspected payload. Removing ciaoShiny's manager package therefore does not remove Better Loot's data or loading path; its effective reward changes remain a gameplay check. [Better Loot instructions](https://www.nexusmods.com/mewgenics/mods/161)

### File overlaps matter beyond ordinary Vortex conflicts

Several mods target the same logical game files through different suffixes:

- `NoCursesOnItems`, `NoBreakingItems`, `SetBonusTooltips`, and `VisibleSetItemMarkers`: overlapping item definitions, using full replacements and merges.
- `FewerBadEvents` and `Event Descriptions`: overlapping event definitions through patches and merges.
- `Event Descriptions` and `Detailed Ability Descriptions`: both append localization content.

Separate mod directories can contain no identical deployment destination while still targeting the same engine resource. These are **compatibility review candidates**, not proof that all listed pairs conflict. Effects on different fields can coexist.

The local deployment uses hardlinks. Future development should keep editable source in this project and install packaged builds, because editing a deployed hardlinked file can also affect its staging copy.

## 4. How we can make mods

### A. Native data and asset mods

This is the first route to investigate for balance, ability composition, item properties, event rewards, furniture effects, localization, and asset replacements.

The installed mods provide concrete examples:

| Mechanism | Local example | Main tradeoff |
| --- | --- | --- |
| Full replacement | `Better Furniture/data/furniture_effects.gon` | Simple, but carries a whole definition file forward |
| GON merge | `NoBreakingItems/data/items/weapons.gon.merge` | Changes selected fields; same-field edits can still compete |
| GON patch | `Bigger_Storage_Early/data/npc_favor_unlocks.gon.patch` | Explicit per-field operations |
| Append | `SetBonusTooltips/data/keyword_tooltips.gon.append` | Adds content; duplicate names and order still matter |
| Localization append | `Detailed Ability Descriptions/data/text/combined.csv.append` | Text and gameplay can be changed independently |
| SWF replacement | `VisibleSetItemMarkers/swfs/catparts.swf` | Competes with other replacements of the same asset |

Tyler Glaiel's [GON source](https://github.com/TylerGlaiel/GON/blob/7f9600b278231a1d458e0ca7f44784e10cffa953/gon.h#L125) documents object/array merging and patch suffixes: `.overwrite`, `.append`, `.merge`, `.add`, and `.multiply`. Arrays can merge by index rather than semantic identity. Append can retain duplicate fields. A patch is consequently not automatically conflict-free.

The filename operation, such as `classes.gon.patch`, and an internal field operation, such as `innate_passives.append`, are different layers. Existing installed mods demonstrate that distinction. Actual engine integration and precedence still need a controlled test on our chosen build.

### B. Native DLL mods

DLLs can change behavior unavailable through data definitions. Mewjector uses a `version.dll` proxy and configuration-driven loading. It also offers an opt-in API for hook chaining, type identifiers, and name collision tracking. This is community infrastructure, not proof of a stable official gameplay SDK. [Mewjector source](https://github.com/githubuser508/mewjector)

Mewtator's source scans enabled mods for DLLs, writes `mewtator_dll_manifest.txt`, and points `chainloader.ini` to it. Mewtator prepares the manifest; Mewjector performs loading. [DLL integration implementation](https://github.com/dancomstock/mewtator/blob/dd48fd8c04c9257af346363bd8fc7384308fd587/app/core/services/dll_injection_service.py)

DLL order and asset precedence are separate concerns. The inspected Mewjector source supports priority entries, directory scanning, optional game-root scanning, and a manifest phase. A scan can find files independently of Vortex's asset checkbox list. We must verify actual loaded DLLs from the loader log when using that route.

Current repository source can be ahead of published binaries: the inspected loader source defers loading outside the initial Windows loader lock, whereas its README still describes older timing. Pin versions and rely on the actual release's behavior.

### C. Content frameworks

Specific frameworks extend particular content types. For example, the [Furniture Framework author](https://github.com/Pseudonym-Tim/mewgenics-furniture-framework) describes a DLL that intercepts the furniture database and appends validated named rows in memory. That demonstrates one route to genuinely new content. It does not imply that any arbitrary file or new class can be added without additional engine support.

## 5. Extraction and authoring tools

The installed `resources.gpak` contains **19,979 entries**: 227 data files, 2,361 level files, 116 SWFs, 54 textures, 50 shaders, and 17,171 audio files. I read its index and verified that the indexed payload sizes match the archive's total length of 5,134,616,316 bytes.

I copied three small reference files into project scratch space: class definitions, NPC unlock definitions, and `furniture_effects_guide.gon`. The last is an embedded guide listing effects for breeding, kittens, strays, room interactions, and household economy. Presence in a guide is a lead for testing, not a guarantee every listed effect is currently active.

Mewtator's [pack service](https://github.com/dancomstock/mewtator/blob/dd48fd8c04c9257af346363bd8fc7384308fd587/app/core/services/pack_service.py) documents the archive layout in executable source. Its Nexus page also links [GPAK-Extractor](https://github.com/ShootMe/GPAK-Extractor). An ordinary folder mod can use `-modpaths`; repacking the original archive is not a prerequisite.

For deeper visual changes we would investigate the SWF format and compatible editors separately. No SWF authoring pipeline was installed or validated in this pass.

## 6. Packaging and load order

A suitable starting package for our own asset mod is:

```text
OurMod/
    description.json
    data/
        <the exact resource path and chosen operation>
```

Mewtator accepts `description.json`, `info.json`, or `modinfo.json`, with metadata such as title/name, author, version, description, URL, and requirements. Dependencies use folder identity and can include version constraints. `dll_order` specifies DLL ordering inside one mod. ciaoShiny's manager instead documents `info.json` with name, author, description, and a preview image. [Mewtator author guide](https://github.com/dancomstock/mewtator/blob/dd48fd8c04c9257af346363bd8fc7384308fd587/MOD_AUTHOR_GUIDE.md), [dependency guide](https://github.com/dancomstock/mewtator/blob/dd48fd8c04c9257af346363bd8fc7384308fd587/MOD_REQUIREMENTS.md)

The installed Vortex extension says **top entries have priority** and serializes the displayed order into `-modpaths`. Use that as its declared behavior, but test replacements versus merges/patches before promising compatibility. Mewtator's 0.5.1 changelog says it removed a former order workaround after game changes; older tutorials can therefore mislead. [Mewtator changelog](https://www.nexusmods.com/mewgenics/mods/1)

Vortex and Mewtator can both manage a `modlist.txt`. Running two managers against the same collection introduces another writer of that state. For this project, keep one manager responsible for the play setup and use explicit, isolated test launches for development.

### Current DLL configuration and future additions

The relevant entries in the game-root `chainloader.ini` are:

```ini
[Chainloader]
ScanPath=mods

[LoadOrder]
Mod1=BiggerWallet\BiggerWallet.dll
```

This is an excerpt, not a replacement for the complete configuration. Keep the existing logging, enablement, and other settings. The pre-edit backup is `work/backups/chainloader-before-biggerwallet-20260924-133818.ini` in this project.

The installed Vortex extension deploys Mewjector but does not edit its configuration or generate a DLL manifest. Vortex's asset Load Order screen is separate from Mewjector's `[LoadOrder]` section. For future DLLs in subfolders, add consecutive `Mod2=...`, `Mod3=...` entries relative to `ScanPath`; the loader stops reading numbered entries at the first missing number. Remove or renumber entries when removing DLL mods. Ordinary data mods need no such entries. Preserve the configuration when updating or reinstalling Mewjector.

Launch through Vortex's Custom Launch tool and inspect `mod_logs/chainloader.log`. A successful explicit load can coexist with `No .dll files found` from the later top-level scan; that scan message alone is not a failure. Mewtator can generate a DLL manifest if we later choose its management workflow, but is not part of the current setup.

## 7. Recommended project approach

**Keep Vortex as your player-facing manager.** Your installed extension already supports ordinary asset mods, so installing Mewtator is not necessary just to begin authoring those mods. Mewtator remains useful if we want its extraction interface, dependency workflow, or managed DLL manifest.

For our first mod:

1. Choose the behavior and inspect its vanilla definition in this installed build.
2. Prefer a small data patch when existing engine effects can express the change.
3. Keep source files in the project; build an archive with one clear mod root.
4. Test the mod alone, then alongside the relevant existing mods.
5. Verify load order using a deliberate two-mod example before relying on undocumented precedence.
6. If native code is required, establish a specific Mewjector release and supported game build, then confirm loading through its log.
7. Exercise acquisition, gameplay, save/reload, and removal where persistent content is involved.

Mewtator source recognizes development/debug launch options, but its save-suffix/inherited-save options are commented out as nonfunctional. Do not treat a manager setting as proven save isolation. [Launch implementation](https://github.com/dancomstock/mewtator/blob/dd48fd8c04c9257af346363bd8fc7384308fd587/app/core/services/game_launcher_service.py)

The missing loader and enabled manager example have been resolved. The current collection can be used for targeted compatibility tests, but it is not a fully validated baseline: the other logical file overlaps and Bigger Wallet's in-game coin behavior remain untested here.

## Evidence record

Local evidence read:

- Game root, `launch.bat`, `mods/modlist.txt`, per-mod data/metadata, and `mods/vortex.deployment.mewgenics-mod.json`.
- Steam's `steamapps/appmanifest_686060.acf`.
- Vortex extension folder `Mewgenics Vortex Extension 1691 0.4.0 2026-09-20T16-52Z 8HMONKGac`, especially `info.json`, `CHANGELOG.md`, and `index.js`.
- Vortex staging and the locally downloaded mod-36 ZIP, examined without executing its contents.
- Original archive index and three small reference definitions, read without modifying the archive.
- Follow-up `chainloader.ini`, the pre-edit backup, and the 13:40 session's `mod_logs/chainloader.log`, confirming Bigger Wallet's load and hook installation.

During the repair, Windows Defender flagged the first PowerShell editing command as `Trojan:Win32/FileFix.BBA!MTB`. The detection record identified that command, not a mod DLL; this does not establish a false positive. Defender settings were not changed and no exclusions were added. The final configuration was checked against the backup and differs by the single intended `Mod1` entry.

Upstream source snapshots inspected:

| Repository | Commit |
| --- | --- |
| dancomstock/mewtator | `dd48fd8c04c9257af346363bd8fc7384308fd587` |
| githubuser508/mewjector | `ccdd6813cef0f51342eb74c0cecb47654f7dbeef` |
| TylerGlaiel/GON | `7f9600b278231a1d458e0ca7f44784e10cffa953` |

These commits identify research snapshots, not installed binary revisions. Source copies and the archive index remain in `work/modding-sources/` for follow-up investigation. Mewjector v3.0 and Bigger Wallet's runtime load/hook installation are verified. Exact game version mapping, effective gameplay and conflict outcomes, and a complete mod-authoring toolchain remain unverified.
