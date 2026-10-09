# mod-aoe-cap

An optional, config-gated target cap for area spells whose spell data sets no target limit. In Forgotten
Land it is an emergency valve for very large pulls and it **ships disabled**: FL keeps its mass-pull
gameplay, and an AoE target cap is not an accepted optimisation (vault
`forgotten-land/10-mass-pull-performance-plan.md` §3, decision 6). While it is off, players and GMs notice
nothing. Full description, and the two limits to read before enabling it: [README.md](README.md).

## Ids and tables

| What | Value |
|---|---|
| Ids, DB tables, SQL | none |
| C++ scripts | `AoECapWorldScript` (reads the conf, logs the boot line), `AoECapSpellScript` (`SpellSC` on `ALLSPELLHOOK_ON_SPELL_CHECK_CAST`) |
| Loader | `Addmod_aoe_capScripts()` -> `AddSC_AoECap()` |
| Config | `mod_aoe_cap.conf`, six `AoECap.*` keys, all re-read by `.reload config` |
| Boot line (logger `module`) | `>> mod-aoe-cap: inactive (...)` or `>> mod-aoe-cap: ACTIVE - ...` |

## Status and progress

- Where it runs: workbench, built from `azerothcore-wotlk/modules/mod-aoe-cap` (`main` `604a2bc`); the live
  `mod_aoe_cap.conf` holds `AoECap.Enable = 0` and the boot of 2026-10-09 logged `mod-aoe-cap: inactive`.
  Host: since MIG-008 (2026-07-25, `70570b0`), at `604a2bc` since the joint window MIG-051 (2026-10-03,
  carrying MIG-033); the host's `mod_aoe_cap.conf` exists with `AoECap.Enable = 0` (nightly backup copy of
  the host's `etc`, 2026-10-09). Players see nothing.
- Evidence: **T1** for the disabled path (build, link and boot line 2026-07-25; `604a2bc` built against the
  AzerothCore-sync core in AC-1, 2026-09-24). The enabled path has never run anywhere (T0).
- Done:
  - Fills `SPELLVALUE_MAX_TARGETS` only where the spell declares no limit of its own; gates for harmful
    spells, player casters (pets, totems and charms count as their player) and dungeon/raid maps.
  - Bounded diagnostics (one summary line every Nth capped cast).
  - `604a2bc`: AzerothCore #27627 made the spell caster a `WorldObject`; a GameObject caster now counts as
    its owner (`70570b0` does not compile against that core, MIG-033).

## Next steps

No open work known; last change 2026-09-24. The module stays off unless the operator decides otherwise.

1. (suggestion) Before anyone enables it: a mod-fl-testbots scenario that pulls more creatures than
   `AoECap.MaxTargets` and counts how many take damage, because the enabled path is untested.
2. (suggestion) A nearest-N cap would need a core edit at the two `RandomResize` sites in `Spell.cpp`
   (README, limitation 1); only worth it if the operator ever wants a low cap.

Open points in full: [todo.md](todo.md).

## Working here

- Branch `claude/<topic>-<sessionId>`, merge into `main` and push (project rule: no pull requests).
  Repository: public `Shoro2/mod-aoe-cap`.
- Adding or removing a source file needs a CMake reconfigure (the module loader is generated). Build tree
  `C:\wowstuff\dcore_bin`; restart the workbench only with the workspace's `scripts\worldserver_restart.ps1`
  under `tools\shared.lock` (vault `13-bug-report-playbook.md` §3).
- Runtime values come from the deployed `C:\wowstuff\dcore\configs\modules\mod_aoe_cap.conf`, not from the
  `.dist`; `.reload config` applies a change without a restart. Enabling the cap is the operator's decision;
  on the host it is a conf change with its own MIG entry.
- Host-relevant changes need a MIG entry in share-public
  `docs/World of Warcraft/forgotten-land/15-host-migration-log.md` (so far MIG-008, MIG-033).
- Vault (`share-public/docs/World of Warcraft/`): `forgotten-land/10-mass-pull-performance-plan.md` (§2.1
  baseline M0, §3 decisions), `forgotten-land/15-host-migration-log.md` (MIG-008, MIG-033),
  `forgotten-land/16-host-connection-and-backup.md` §9, `12-server-todo.md` (row "mod-aoe-cap config deploy").
- Doc set: INDEX.md, CLAUDE.md, data_structure.md, functions.md, log.md (newest first), todo.md.
