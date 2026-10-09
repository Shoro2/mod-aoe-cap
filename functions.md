# Functions

## Scripts

| Script | Hook | What |
|---|---|---|
| `AoECapWorldScript` (`WorldScript`) | `void OnAfterConfigLoad(bool reload)` | reads the six `AoECap.*` keys into file-local statics; logs `>> mod-aoe-cap: ACTIVE - ...` (with the gates and a reminder that the subset is random) or `>> mod-aoe-cap: inactive ...` |
| `AoECapSpellScript` (`SpellSC`, `ALLSPELLHOOK_ON_SPELL_CHECK_CAST`) | `void OnSpellCheckCast(Spell* spell, bool strict, SpellCastResult& res)` | fills in the target cap, see below |

## The cap decision (`OnSpellCheckCast`, in this order)

1. Return when `AoECap.Enable` is 0, `AoECap.MaxTargets` is 0 or there is no spell.
2. Return when the cast already fails (`res != SPELL_CAST_OK`).
3. Return when the spell declares its own limit (`SpellInfo::MaxAffectedTargets != 0`): that value is never
   touched.
4. Return for passive spells and for spells that do not target an area (`IsTargetingArea()`).
5. `HarmfulOnly`: return for positive spells (`IsPositive()`).
6. Caster: `Spell::GetCaster()` is a `WorldObject` since AzerothCore #27627; a unit is used as is, a
   GameObject counts as its owner, anything else returns.
7. `PlayerCastersOnly`: return unless `GetCharmerOrOwnerPlayerOrPlayerItself()` finds a player.
8. `DungeonsOnly`: return unless the caster's map `IsDungeon()` (5-player dungeons and raids).
9. `spell->SetSpellValue(SPELLVALUE_MAX_TARGETS, MaxTargets)`.
10. `LogEveryNth`: a relaxed atomic counter; every Nth capped cast logs
    `mod-aoe-cap: <n> area casts capped at <cap> targets so far.` (logger `module`).

## Where the core applies the value

`Spell::CheckCast` runs inside `Spell::prepare` before the target selection, so the value is in place when
`Spell::SelectImplicitConeTargets` and `Spell::SelectImplicitAreaTargets` (`src/server/game/Spells/Spell.cpp`)
read `m_spellValue->MaxAffectedTargets`. Both trim an over-long list with
`Acore::Containers::RandomResize`: the subset is **random**, not the nearest N, and a module cannot change
that. Casts that skip `CheckCast` (fully triggered casts) are never capped.

With `AoECap.Enable = 0` the hook returns at step 1, no spell value is written, and target selection is the
stock behaviour.

## Config

See [data_structure.md](data_structure.md) for the six keys and their defaults.
