# Data structure

| Path | What |
|---|---|
| `README.md` | why the module exists and ships off, how it works, the two limits, config, install |
| `conf/mod_aoe_cap.conf.dist` | the six `AoECap.*` keys with their documented defaults |
| `src/mod_aoe_cap.cpp` | `AoECapWorldScript`, `AoECapSpellScript`, `AddSC_AoECap()` |
| `src/mod_aoe_cap_loader.cpp` | `Addmod_aoe_capScripts()` -> `AddSC_AoECap()` (the `AddSC_*` name must stay unique across the module fleet) |

No database tables and no SQL. No tests in the repository.

## Config keys

All six are re-read on `.reload config` (`OnAfterConfigLoad`).

| Key | Default | Meaning |
|---|---|---|
| `AoECap.Enable` | `0` | master switch |
| `AoECap.MaxTargets` | `250` | cap for area spells that declare none; `0` also disables the module |
| `AoECap.HarmfulOnly` | `1` | only harmful spells (never truncate group heals or raid buffs) |
| `AoECap.PlayerCastersOnly` | `1` | only player casters; pets, totems, charmed units and a player's GameObjects count as the player |
| `AoECap.DungeonsOnly` | `0` | only on dungeon and raid maps |
| `AoECap.LogEveryNth` | `0` | one summary line every Nth capped cast; `0` = no logging |

Deployed values (2026-10-09): the workbench's `C:\wowstuff\dcore\configs\modules\mod_aoe_cap.conf` and the
host's (nightly backup copy) both equal the `.dist` defaults, `AoECap.Enable = 0`.
