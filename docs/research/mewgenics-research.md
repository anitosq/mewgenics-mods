# Mewgenics: gameplay and mod-design research

Researched 24 September 2026. This is a working foundation for this project's future mods, with progression spoilers. Sources are linked beside the relevant material. The wiki is community-maintained; this brief distinguishes its documented behavior from design analysis and untested implementation assumptions.

## 1. What the game is

Mewgenics is a single-player tactical roguelite and cat-breeding life simulation by Edmund McMillen and Tyler Glaiel, released on PC on 10 February 2026. Players develop a household across generations and send selected cats on turn-based expeditions. Its identity combines difficult tactical decisions, extensive interactions between systems, procedural cats, grotesque humor, and long-term consequences. The developers advertise more than 1,000 abilities and 900 items; those figures describe breadth rather than a stable modding schema. [Official Steam page](https://store.steampowered.com/app/686060/Mewgenics/)

The useful design interpretation is that there are three connected timescales: an action within a battle, a build within an adventure, and a bloodline across adventures. A change at one timescale can affect the others.

## 2. The core loop

1. **Manage the household.** Accept strays, arrange cats and furniture, maintain food, and choose prospective adventurers and breeding pairs.
2. **Prepare a party.** Ordinarily select 1–4 eligible, non-retired cats. Assign class collars, then inspect their resulting starting abilities and equip stored items.
3. **Adventure.** Travel through chapter nodes containing battles, events, treasure, shops, minibosses, and bosses. Acquire resources and draft level-up rewards.
4. **Decide how far to push.** After a chapter boss, return home or continue where progression permits. Hard routes offer extra danger and rewards.
5. **Convert the outcome into future strength.** Returning survivors retire from ordinary adventures; their abilities and mutations can inform future breeding. Loot funds the household and future runs. A party defeat loses its cats and carried resources, with limited recovery through an NPC.
6. **Advance the day.** Feed and age the household; resolve breeding, fights, mutations, recovery, and deaths. Prepare a new generation.

Sources: [Adventure](https://mewgenics.wiki.gg/wiki/Adventure), [House](https://mewgenics.wiki.gg/wiki/House), [Breeding](https://mewgenics.wiki.gg/wiki/Breeding).

**Design implication:** survivors have value beyond their next combat. Retirement, aging, equipment attrition, and donations create demand for replacement cats and continued expeditions.

## 3. What makes an individual cat

Cats have generated body parts, seven core stats, abilities, and persistent traits. Parentage can influence appearance. Player party cats are distinct from other allied cat units, which matters for targeting and defeat checks. [Cats](https://mewgenics.wiki.gg/wiki/Cats)

| Stat | Principal role |
| --- | --- |
| Strength | Most melee damage; basic attacks commonly gain one damage per point above 5, while melee spells commonly scale at half that rate. |
| Dexterity | Most ranged physical damage, commonly one damage per two points above 5. |
| Constitution | Maximum HP, normally four times Constitution, and post-battle recovery. |
| Intelligence | End-of-turn mana regeneration, normally clamped at zero minimum. |
| Speed | Initiative and movement range. |
| Charisma | Starting mana, maximum mana, and household compatibility. |
| Luck | Many random outcomes and physical critical-hit chances. |

Ordinary starting base stats range from 3–7. The baseline is generally 5. Default starting mana equals Charisma; maximum mana is three times Charisma. Cat movement normally starts at four tiles, adjusted by one per two Speed away from 5, with truncation and a minimum. Initiative normally adds twice Speed to a base value.

Luck commonly grants probabilistic extra rolls, keeping the favorable or unfavorable result. It is not a universal flat success bonus. Critical chance instead normally gains two percentage points per Luck. Base stats and later modifiers must remain distinct for inheritance. [Stats](https://mewgenics.wiki.gg/wiki/Stats)

**Design implication:** a stat buff can affect several systems. Intelligence changes spell throughput, Charisma changes opening resources and breeding, and Speed can let a cat act before a threat as well as reach it.

## 4. Classes and build construction

Collars determine stat adjustments, basic attacks, and reward pools. The initial options are Collarless, Fighter, Hunter, Mage, and Tank. Further collars unlock through chapter completion and Butch. [Classes](https://mewgenics.wiki.gg/wiki/Classes)

| Class | Main identity | Unlock |
| --- | --- | --- |
| Collarless | General pool, no collar stat bonuses | Initial |
| Fighter | Melee damage | Initial |
| Hunter | Ranged damage | Initial |
| Mage | Spells and elements | Initial |
| Tank | Durability and displacement | Initial |
| Cleric | Healing and support | Alley |
| Thief | Mobility, evasion, back attacks, coins | Sewers |
| Necromancer | Corpses and life manipulation | Boneyard |
| Tinkerer | Gadgets and temporary equipment | Bunker |
| Butcher | Meat generation and pulling targets | Core |
| Druid | Animal allies and a crow companion | Crater |
| Psychic | Remote manipulation | Moon |
| Monk | Stances and repeated attacks | Lab |
| Jester | Broad, unpredictable ability access | Rift |

The usual ability budget is one basic attack, four learned active abilities, and two passives. A separate contextual bonus spell can occupy a fifth slot. Disorders have two additional dedicated slots. Basic attacks ordinarily cost no mana and have one use per turn. Active spells can generally be repeated while affordable unless their own rules restrict use. Most magic damage does not automatically scale with Intelligence. Upgraded abilities use a “+” version and may change behavior as well as numbers. [Abilities](https://mewgenics.wiki.gg/wiki/Abilities)

After a battle, normally one eligible lowest-level cat levels up, with random selection among ties. Downed cats usually do not qualify. Early rewards develop actives and passives; level 6 offers an active upgrade, with passive upgrades appearing at levels 7 and 9. Smaller parties therefore concentrate levels. Exceptions arise from items and other effects. [Adventure: level ups](https://mewgenics.wiki.gg/wiki/Adventure#Level_Ups)

**Design implication:** evaluate new abilities at their acquisition stage, against slot opportunity cost, and when inherited into another class. A seemingly balanced class-exclusive skill may behave very differently on a hybrid cat.

## 5. Combat: turns, positioning, and survival

Battles use a 10×10 board. A tile can contain separate layers for a unit/object, gas, pickups, traps, and terrain. Initiative determines unit turns, with ties randomized and order capable of changing during a round. Movement, basic attacks, weapons, and usable trinkets have separate action refreshes. Health and mana regeneration occur at turn end.

HP reaching zero normally downs a cat and leaves a corpse. Destroying that corpse usually kills the cat permanently. Winning can revive surviving bodies; all party cats being downed causes defeat. An allied familiar does not necessarily keep a lost party alive.

Exhaustion normally begins at round 10, earlier for old cats or Insomnia. It causes damage and reduces recovery, preventing indefinite stalling. Certain boss fights provide Adrenaline that delays it. [Battle](https://mewgenics.wiki.gg/wiki/Battle)

Physical back attacks gain 25% damage rounded down, with a minimum bonus of one. Physical critical hits normally double damage. Contact is its own trigger: adjacent melee strikes, throws, collision, trample, and jumps do not all trigger exactly the same participants' effects. Reach can avoid ordinary adjacent melee contact. [Damage Mechanics](https://mewgenics.wiki.gg/wiki/Damage_Mechanics)

Shield absorbs damage points; Holy Shield blocks individual hits. Neither is universal protection: several damage-over-time effects bypass them. Refreshing an already available action does not bank another use. Trample combines movement, damage, displacement, and contact. [Keywords](https://mewgenics.wiki.gg/wiki/Keywords)

**Design implication:** extra actions, free casts, movement resets, and recursive triggers deserve more scrutiny than an isolated damage increase. Corpse hits, contact, and damage instances are separate quantities.

## 6. Status effects and environmental combinations

Status timing is part of balance:

- **Burn:** damage at the affected unit's turn start; stacks decrease.
- **Bleed:** damage at round end; stacks increase.
- **Poison:** damage at turn start; healing interacts with and reduces poison.
- **Stun:** denies a turn; repeated applications do not simply add durations.
- **Marked / Magic Weakness:** modify incoming physical / magic attacks respectively.
- **Brace / Thorns:** reduce incoming damage / punish contact.

Stat increases and decreases can cancel into one net status, while temporary and conditional variants may remain separate. Downing removes most statuses; some passive-granted statuses do not reapply upon mid-battle revival. [Status Effects](https://mewgenics.wiki.gg/wiki/Status_Effects)

Elements are interaction tags, separate from physical/magical damage classification. Electricity conducts across water and Wet/Metal units, potentially spreading an action's whole effect. Robots can benefit from electricity. Water removes Burn, grows plants, and interacts with freezing. Fire ignites suitable terrain but does **not** intrinsically mean “apply Burn.” Ice does not intrinsically mean “apply Slow.” Holy healing harms undead. Multiple elements can coexist on an action. [Elements](https://mewgenics.wiki.gg/wiki/Elements)

Terrain changes movement cost and combat opportunities: water costs extra movement and conducts electricity; tall grass provides dodge; flowers provide regeneration; brambles and glass punish travel; ice enables sliding; lava and tar introduce further interactions. Flying avoids many, but not all, terrain effects. [Tiles](https://mewgenics.wiki.gg/wiki/Tiles)

Weather can last across a chapter and affect both sides: persistent elements, spawns, damage, movement, or altered regeneration. Seasons influence the opening chapter's weather; some areas have fixed conditions. [Weather](https://mewgenics.wiki.gg/wiki/Weather)

**Design implication:** test an elemental addition on terrain, allies, corpses, and relevant unit types. A new tag can create a larger change than its tooltip damage suggests.

## 7. Equipment and the resource economy

Cats ordinarily have weapon, head, face, neck, and trinket slots; consumables share the trinket slot. Innate class items can occupy slots permanently. Equipment can grant actions, passive effects, stats, and set bonuses. Three matching set pieces generally activate a bonus. Cursed equipment normally cannot be removed without an exception.

Storage-origin items become Worn after use on an adventure or house boss. The dedicated Items and Keywords pages say already-Worn items can break with a 50% chance on early returns, or guaranteed after chapter 4 / a house boss. This is more specific than the Adventure overview; verify the exact transition order before modding durability. Other item properties such as Brittle, Fragile, and Flammable have distinct break conditions. [Items](https://mewgenics.wiki.gg/wiki/Items), [Keywords](https://mewgenics.wiki.gg/wiki/Keywords#Worn)

Food supports the household; coins buy supplies and upgrades; equipment and furniture broaden future options. Home capacity can constrain what is retained. Items queued in household trash are discarded at day end. [House](https://mewgenics.wiki.gg/wiki/House)

**Design implication:** removing attrition changes the demand for loot. Increasing household population also changes food requirements, crowding, and the pace of NPC donations.

## 8. Household, inheritance, and persistent traits

| House stat | Main effect |
| --- | --- |
| Appeal | House-wide quality and ability variety of arriving strays |
| Comfort | Breeding versus fighting conditions; penalized by crowding above four cats per room |
| Stimulation | Inheritance quality and probability |
| Health | Aging, longevity, and recovery from injuries/disorders |
| Mutation | Overnight mutation chance and access to mutation varieties |

Furniture changes these values. Cats remain in their assigned rooms, so placement affects available partners and outcomes. Advancing a day normally costs one food per cat, with exceptions. [House](https://mewgenics.wiki.gg/wiki/House)

Breeding depends on shared room, compatibility, total Charisma, libido, sexuality, relationships, Comfort, and special restrictions. Children inherit parental **base** stats, not accumulated adventure stat bonuses. Stimulation biases better stats and ability inheritance; it does not guarantee a particular skill. Two active inheritance attempts can select the same ability without rerolling. Passive inheritance has special exceptions. Disorders use separate inheritance rolls rather than the Stimulation rule.

Shared ancestry determines inbreeding and associated defects. Neutral-gender cats can fill either parental role. Male–male and female–female pairings do not produce kittens but can influence incoming strays. Personality values are independently generated rather than straightforwardly inherited. [Breeding](https://mewgenics.wiki.gg/wiki/Breeding)

A stray normally arrives each day and supplies unrelated ancestry. Appeal affects stats and access to class abilities. The special “Gay Stray” mechanic can grant abilities drawn from the previous night's same-sex breeders' classes; it is distinct from a regular stray simply having a gay sexuality. [Stray Cats](https://mewgenics.wiki.gg/wiki/Stray_Cats)

Mutations are body-part changes with mechanical effects, and many are inheritable. Birth defects are a related category, usually detrimental but sometimes exploitable. Inheritance ordinarily restores left/right symmetry, so a cat's adventure-acquired asymmetry may not pass on intact. Mutation magnitude can change the available pool, not just frequency. [Mutations](https://mewgenics.wiki.gg/wiki/Mutations)

Disorders occupy two separate slots and often combine a penalty with an advantage. Some spread through contact; others affect targeting, control, equipment, or resource use. They can persist into breeding, but may be cured. Injuries are another system: persistent stat penalties commonly caused by being downed, with their own recovery rules. Retirement bars normal expeditions; surviving a house boss can cause “Super Retired” status, barring further house-boss participation. [Disorders](https://mewgenics.wiki.gg/wiki/Disorders), [Stats](https://mewgenics.wiki.gg/wiki/Stats#Injuries)

**Design implication:** “cleaning up” every negative trait can erase intentional synergies. Household randomness also creates supply constraints for the campaign, so breeder improvements can accelerate progression substantially.

## 9. World structure and permanent progression

The main route families are:

| Act | Opening | Two route branches | Fourth chapter |
| --- | --- | --- | --- |
| 1 | Alley | Sewers → Caves; Junkyard → Boneyard | Throbbing Domain |
| 2 | Desert | Bunker → Core; Crater → Moon | Rift |
| 3 | Lab | Ice Age → Jurassic; Future → End | Infinite |

Fourth chapters have additional quest requirements. Act 2 follows completion of Caves and Boneyard. Act 3 requires Core and Moon completion and Dr. Beanies' progression; its routes have additional gates. Maps have largely prescribed node sequences with varied encounters, rather than unrestricted random graphs. Chapters maintain their own encounter, item, and boss pools. [Chapters](https://mewgenics.wiki.gg/wiki/Chapters)

NPC donation requirements turn cats into permanent upgrades:

| NPC | Requested cats / role | Main return |
| --- | --- | --- |
| Butch | Cats that reached specified areas | Storage and class progression |
| Frank | Retired cats | House expansion |
| Tink | One-day-old kittens | More visible cat information |
| Tracy | Cats age five or older | Food storage, blank collars, idols |
| Baby Jack | Injured cats | Furniture progression |
| Dr. Beanies | Cats with specified unusual traits | Side quests |
| Organ Grinder | Adventure deaths | Recovery-related progression |
| Steven | Special progression role | Difficulty and house-boss systems |

Source: [Characters](https://mewgenics.wiki.gg/wiki/Characters). Tink's unlocks mean some apparently missing information is deliberately progression-gated. [Tink](https://mewgenics.wiki.gg/wiki/Tink)

Events can alter resources, traits, or the next encounter. Resolution commonly separates good/bad from common/rare outcomes. Tested stats, whether a stat is the cat's highest/lowest, difficulty, and Luck can all matter. Some outcomes use fixed rules. Direct event damage generally stops at one HP, but explicit sacrifices and other consequences can still remove cats. [Events](https://mewgenics.wiki.gg/wiki/Events#Outcomes)

**Design implication:** rewards, information visibility, and convenience changes can bypass campaign gates even if they never alter combat damage.

## 10. Modding foundations: documented, not yet tested here

Follow-up: [the dedicated modding research](mewgenics-modding-research.md) adds local installation and source-code evidence gathered after this first gameplay pass. The paragraphs below describe what was established at the earlier stage.

The wiki documents ordered `-modpaths` loading, a `resources.gpak` asset archive, and mods that mirror its internal paths. It describes replacement files and `.append`, `.merge`, and `.patch` variants, plus Mewtator and GPAK extraction tools. Its CSV merge example is positional, so row insertions can invalidate an old patch. The page is explicitly unfinished, including missing examples and an empty SWF section. These are research leads, not a verified toolchain for this installation. [Modding](https://mewgenics.wiki.gg/wiki/Modding)

Tyler Glaiel's own **GON** repository confirms the structured-data format and its support for combining objects for stackable mods. Scalar values are represented as strings with conversion helpers; arrays and objects are supported. The README acknowledges incomplete documentation of PatchMerge. This validates the format, but does not establish every Mewgenics-specific field or engine hook. [Developer's GON repository](https://github.com/TylerGlaiel/GON)

The developer resource linked by the wiki is for **The End Is Nigh**. It is useful background, but its uploader, assets, and game-specific rules are not automatically Mewgenics tooling. [Developer's TEIN resources](https://www.glaielgames.com/teinworkshop/)

The wiki documents `-dev_mode true`, with combat, event, breeding, and chapter test entry points. It describes separate development campaigns, but also a route into ordinary saves. Treat isolation as something to confirm when setting up tests. [Dev Mode](https://mewgenics.wiki.gg/wiki/Dev_Mode)

No game files were inspected or changed, and no tools or mods were installed during this research. No Lua API, general scripting interface, arbitrary new-class support, or current Workshop workflow has been established by this work.

## 11. Evidence quality and unresolved details

- **Version baseline:** the wiki's latest listed desktop build was v1.1.21239. This is not a verification of the user's installed build. Some mechanical explanations cite older datamined versions. [Version History](https://mewgenics.wiki.gg/wiki/Version_History)
- **Equipment wear:** Adventure simplifies Worn breakage to 50%; Items and Keywords add guaranteed breakage in specified circumstances. Use those detailed pages as the working reference and test before implementation.
- **Relationships:** Stats says a lover never changes; Breeding describes affinity decay that can eventually change the lover. The more detailed algorithm is the stronger working account, but should be reproduced before modifying it.
- **Age thresholds:** overview prose sometimes says “above age 2” while the kitten description says adulthood begins at 2; Tracy's dedicated descriptions use age 5 or older. Test boundary values rather than relying on loose wording.
- **Inbreeding disorders:** Stats and Breeding present differing formulas/ranges. Breeding's detailed birth sequence is the provisional reference; exact odds remain a validation item.
- **Incomplete articles:** House, Adventure, Modding, Dev Mode, and Keywords carry maintenance notices. A rendered tooltip, wiki explanation, data definition, and actual engine behavior need not agree perfectly.

## 12. How this research should guide our first mod

The following is project analysis, not a claim about already-supported engine features:

1. Choose one concrete behavior and the intended player experience.
2. Identify the installed build and the exact vanilla definition that owns that behavior.
3. Trace related pools, tags, localization, unlocks, and lifecycle events before deciding which files to change.
4. Prefer the smallest compatible data change once its loading and merge behavior are confirmed.
5. Test acquisition and actual use, not just a successful launch.
6. Test relevant combinations: other classes, inherited abilities, upgrades, terrain, unit types, death/revival, and returning home.
7. Check whether the effect survives loading, persists across generations, or interacts with another mod touching the same definition.

Useful first areas to investigate include a narrowly scoped balance adjustment, an item variant using existing effects, or a furniture tweak. Feasibility depends on inspecting the actual game data. New engine behaviors, UI rewrites, classes, and complete chapters require a separate technical investigation.

The design priorities are clear: preserve meaningful party-building choices, account for cross-class inheritance, respect action and resource limits, and evaluate campaign effects alongside combat effects.

---

Attribution: this brief paraphrases the linked Mewgenics Wiki articles by their contributors and adds project analysis. Wiki-derived adaptations are shared under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/), matching the wiki's stated content license. Developer and store sources are separately linked. Research date and open questions above should travel with this document.
