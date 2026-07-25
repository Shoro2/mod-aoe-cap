# mod-aoe-cap

An optional, config-gated target cap for area spells that the spell data leaves uncapped.
**Ships disabled.**

## Why it exists, and why it is off

Baseline M0 (2026-07-25, see the vault doc `forgotten-land/10-mass-pull-performance-plan.md`
§2.1) measured a 960-creature pull with a full loot drain on the workbench:

| Metric | Value |
|---|---|
| World tick | mean 7 ms · p95 15 ms · p99 16 ms · **max 23 ms** |
| Updates above 50 ms | **none**, across 17 minutes and two pulls |
| worldserver CPU at the worst moment | **35 % of ONE core** (1.1 % of 32) |

The server did not need a cap. This module is an **emergency valve** for a situation
measurement has not yet demonstrated, so `AoECap.Enable` defaults to `0` and nothing changes
until an operator turns it on.

## How it works

The core applies a per-cast limit from `m_spellValue->MaxAffectedTargets` at exactly two
places — the cone search and the area search in `Spell::SelectImplicit*Targets`
(`src/server/game/Spells/Spell.cpp`, the two
`if (uint32 maxTargets = m_spellValue->MaxAffectedTargets)` blocks). `0` there means
"no limit".

`Spell::CheckCast` runs **before** target selection (inside `Spell::prepare`: `CheckCast` at
~3516, `SelectSpellTargets` at ~3594), so the `ALLSPELLHOOK_ON_SPELL_CHECK_CAST` hook is a
valid module-side seam. The module fills that value in via
`Spell::SetSpellValue(SPELLVALUE_MAX_TARGETS, n)` — and only where the spell declared no limit
of its own. No core change is required.

## Two limitations you must know before enabling

1. **The subset is RANDOM, not nearest.** The core trims with
   `Acore::Containers::RandomResize`. A module cannot change that — it is hardcoded at both cap
   sites. With a cap of 99 on a 960-mob pull, ~90 % of the pull is skipped per cast *at random*,
   including mobs standing directly in front of the caster. That reads as broken, not merely
   slower. Use a **high** cap as a safety limit; do not use a low cap as a balance lever. Making
   it distance-ordered would require a core edit.
2. **Fully-triggered casts skip `CheckCast`** and are therefore never capped.

## Configuration

All keys are re-read by `.reload config`, so the module can be switched on and off without a
restart. See `conf/mod_aoe_cap.conf.dist` for the documented defaults.

| Key | Default | Meaning |
|---|---|---|
| `AoECap.Enable` | `0` | Master switch |
| `AoECap.MaxTargets` | `250` | Cap for spells that declare none; `0` also disables |
| `AoECap.HarmfulOnly` | `1` | Never truncate group heals or raid buffs |
| `AoECap.PlayerCastersOnly` | `1` | Leave creature and boss abilities alone (pets resolve to their owner) |
| `AoECap.DungeonsOnly` | `0` | Restrict to dungeon and raid maps |
| `AoECap.LogEveryNth` | `0` | One summary line every Nth capped cast; never one per cast |

## Installing

The module is picked up by CMake's module scan, which **generates** the loader
(`modules/gen_scriptloader/static/ModulesLoader.cpp`) with the symbol
`Addmod_aoe_capScripts()` derived from this directory name. Adding the module therefore
requires a **CMake reconfigure**, not just a rebuild:

1. Clone into `azerothcore-wotlk/modules/`.
2. Re-run CMake, then build.
3. Copy `conf/mod_aoe_cap.conf.dist` to the server's `configs/modules/` as
   `mod_aoe_cap.conf` (or leave the `.dist` in place — AzerothCore reads both).
4. Restart. The boot log states whether the module is active or inactive.

## Verifying it does nothing when disabled

With `AoECap.Enable = 0`, the hook returns on its first condition and no spell value is ever
written, so target selection is bit-for-bit the stock behaviour. The boot log line
`mod-aoe-cap: inactive` confirms it.
