# Auto Furniture

Arrange rooms automatically using furniture you own. Choose which stats to
improve, set optional Min and Max values, keep pieces pinned, or return
furniture from one room or the whole house. Includes one-step Undo.

Version 0.2.2 requires Windows x64, Mewgenics 1.1.21239 (Steam build 25143593),
and [Mewjector](https://www.nexusmods.com/mewgenics/mods/218) API 3 or newer.

[Nexus Mods](https://www.nexusmods.com/mewgenics/mods/538) |
[GitHub release](https://github.com/anitosq/mewgenics-mods/releases/tag/auto-furniture/v0.2.2)

## Install

Close the game and back up your saves before installing.

- Vortex: install the ZIP, enable and deploy it. Enable Auto Furniture in
  Load Order, then launch through Vortex.
- Mewtator: import the ZIP, enable Auto Furniture and DLL Mod Support, then
  launch through Mewtator. Mewjector must be installed in the game directory.
- Manual: extract the AutoFurniture folder into the game's mods folder, then
  move AutoFurniture.dll out of it into mods. Include the full path to
  mods/AutoFurniture in the game's -modpaths launch arguments alongside your
  other asset mods. Keep Mewjector enabled and scanning mods.

Keep only one installation. The DLL and asset folder must come from the same
archive. If switching managers, remove the previous installation first.

To remove, close the game and uninstall through your manager (deploy afterward
in Vortex). For manual installs, remove the DLL, AutoFurniture folder and its
launch argument. Furniture you placed remains in the save.

## Use

Enter furniture-placement mode and click a room's wand. Select Comfort,
Stimulation, Health, Mutation, Appeal, or any combination. Click Calculate,
compare Before and Best found, then Apply. Calculate alone moves nothing.

The interface follows your game language: English, Spanish, French, German,
Italian, Brazilian Portuguese, Russian, Korean, Japanese or Simplified Chinese.
After changing language, reopen the room's wand panel.

- Min is a best-effort target; Max is a limit. Leave either blank for no bound.
  Zero is an explicit bound. When both are set, Max must exceed Min.
- Multiple selected stats are balanced, improving the lowest first after
  addressing bounds. Unselected stats may decrease; review all five results.
  Appeal is the selected room's contribution, not the whole-house total.
- Pins keep placed pieces and their supports in position.
- Include utility furniture allows pieces such as food storage boxes to be
  rearranged and added to remaining space. Off leaves placed utility pieces
  fixed and unused ones in inventory. Storage capacity itself is not optimized.
- Return room and Return all rooms send normal furniture, including utility
  pieces, back to inventory. These actions ignore pins. Poop and the game's
  auto-feeder stay in place.
- Undo restores the last successful mod action. Use it before manually moving
  furniture, leaving the scene, or reloading; undo history is not saved.
- Clear resets selections, bounds, utility inclusion and pins without moving
  furniture or clearing Undo.

Only loose inventory and the selected room are used. Furniture is never taken
from other rooms. The search uses horizontal mirroring, not rotation or
vertical flipping. Shapes, supports and inventory limits can leave gaps.
Best found is not a guaranteed mathematical optimum.

Mouse and keyboard required. The mod stays inactive on unsupported game builds
or missing, disabled or mismatched UI assets. If room contents change after
calculation, calculate again. Other mods' loaded furniture stats are respected;
compatibility with every furniture mod is not guaranteed.

## Development

See the [development guide](https://github.com/anitosq/mewgenics-mods/blob/main/mods/auto-furniture/DEVELOPMENT.md) for build and check commands.
Code: MIT; see [LICENSE](LICENSE) and [notices](THIRD_PARTY_NOTICES.md).
