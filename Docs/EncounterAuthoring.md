# Encounter Authoring Guide

This document explains how to set up a new combat encounter using the Encounter System (replaces the legacy Round/Spawn system).

## Architecture

The encounter runtime is implemented in C++ under `Source/InTheDark/EncounterSystem/`:

| Class | Role |
|-------|------|
| `UEncounterConfig` (UPrimaryDataAsset) | Defines waves, rewards, kind, timing |
| `ACombatArena` | Root actor: holds TriggerVolume, RewardAnchor, Anchors, Gates, Director |
| `UEncounterDirectorComponent` | FSM: Idle → Starting → WaveActive → PostClear → Cleared / Failed |
| `ASpawnAnchor` | Directed spawn point with tags, FOV/distance/cooldown gating, telegraph VFX/SFX |
| `AEncounterGate` | Optional barriers that auto-lock during combat and unlock on clear |
| `UEncounterInteractionComponent` | Starts an encounter from an interactable (rune/altar) |
| `UInTheDarkGameInstance` / `UInTheDarkSaveGame` | Persists cleared encounter ids |

## Data Asset: `UEncounterConfig`

Create: *Content Browser → Miscellaneous → Data Asset → EncounterConfig*.

Example: `/Game/Blueprints/Encounters/DA_Encounter_TestArena`.

Properties:
- `Kind` — Story / Arena / Optional.
- `Waves` — array of `FEncounterWave`:
    - `WaveName` — debug name.
    - `Spawns` — array of `FEnemySpawn { EnemyClass, Count, AnchorTag, RoleTag }`.
    - `Continuation` — rules (`OnAllDead`, `OnBelowPercent` with `AliveFraction`, `OnTimer` with `MaxTimeSeconds`, `OnMixed`).
    - `bLockGatesDuringWave`.
- `PostClearBeatSeconds` — pause between waves / at end (typical 1.5–3.0).
- `Reward` — loot/key/flag.
- `bPersistCleared` — whether GameInstance records the encounter id on clear.
- `bReloadCheckpointOnFailure` — reload level on fail.

## Level setup

1. Place a **`BP_CombatArena`** (subclass of `ACombatArena`, e.g. `BP_CombatArena_Test`).
2. Set its **`EncounterId`** to a unique string — convention: `Encounter.<Kind>.<MapName>`.
3. Assign the **`Config`** DataAsset.
4. Size the **`TriggerVolume`** so the player overlap is the trigger (unless starting via interaction).
5. Place **`BP_SpawnAnchor`** instances around the arena. Give each instance:
    - `AnchorTags` — e.g. `Anchor.Perimeter`, `Anchor.Center`.
    - `SpawnRotationOffset` if the spawned AI should face inward.
    - Optional `TelegraphVFX` / `TelegraphSound`.
6. On the arena, fill the **`Anchors`** array with those anchor instances.
7. Optionally add **`BP_EncounterGate`** actors and fill the arena's **`Gates`** array.
8. Save the level.

## Gating by tags

`FEnemySpawn.AnchorTag` selects spawn sites:
- Empty tag ⇒ any anchor in the arena array.
- Tag present ⇒ only anchors whose `AnchorTags` contain it.

Register new tags in `Config/DefaultGameplayTags.ini` (e.g. `Anchor.Perimeter`, `Role.Grunt`).

## Starting an encounter

- **Auto (overlap):** set `bAutoStartOnOverlap = true` on the arena (default).
- **Interaction:** add a `UEncounterInteractionComponent` to a rune/altar actor and set its `Arena` property; call `Interact()` from Blueprint.
- **Programmatic:** call `ACombatArena::RequestStart(InstigatorPawn)`.

## Persistence

`UInTheDarkGameInstance::IsEncounterCleared(Id)` / `MarkEncounterCleared(Id)`
persist via `UInTheDarkSaveGame` (SaveSlot `InTheDarkPlayer` / user index 0).

When `bSkipIfAlreadyCleared` is true, revisiting a cleared story arena leaves the gates open and spawns nothing.

## Validation

Editor utility script: `Content/Python/validate_encounters.py`

```
py "<Project>/Content/Python/validate_encounters.py"
```

Reports arenas missing Config, Anchors, waves, or EncounterId.

## Migration notes

Legacy `BP_RoundManager`, `BP_StartRoundTrigger`, `RC_ArenaDefault`, and the old
C++ `RoundSystem` / `SpawnSystem` directories have been removed. A CoreRedirect
(`DefaultEngine.ini`) maps `S_SavedInventoryEntry` → `/Script/InTheDark.SavedInventoryEntry`.
