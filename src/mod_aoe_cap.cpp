/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * mod-aoe-cap — an optional, config-gated target cap for area spells that the
 * spell data leaves uncapped.
 *
 * WHY THIS IS DISABLED BY DEFAULT
 * ------------------------------------------------------------------------
 * Baseline M0 (2026-07-25, see share-public forgotten-land/10) measured a
 * 960-creature pull with a full loot drain: world tick mean 7 ms, p99 16 ms,
 * max 23 ms, worldserver at 35 % of ONE core, and not a single world update
 * above 50 ms. The server did not need a cap. This module exists as an
 * emergency valve for a future situation that measurement has not yet shown,
 * so `AoECap.Enable` ships as 0 and a cap only ever takes effect when an
 * operator turns it on deliberately.
 *
 * HOW IT WORKS
 * ------------------------------------------------------------------------
 * The core applies a per-cast target limit from `m_spellValue->MaxAffectedTargets`
 * at exactly two places — the cone search and the area search in
 * `Spell::SelectImplicit*Targets` (src/server/game/Spells/Spell.cpp, the two
 * `if (uint32 maxTargets = m_spellValue->MaxAffectedTargets)` blocks). A value
 * of 0 there means "no limit".
 *
 * `Spell::CheckCast` runs *before* target selection (Spell.cpp: CheckCast at
 * ~3516, SelectSpellTargets at ~3594 inside `Spell::prepare`), so the
 * `OnSpellCheckCast` hook is a valid seam to fill that value in from a module
 * without touching core. We only ever fill in a cap where the spell itself
 * declared none.
 *
 * TWO LIMITATIONS AN OPERATOR MUST KNOW
 * ------------------------------------------------------------------------
 * 1. The core trims the list with `Acore::Containers::RandomResize`, i.e. a
 *    *random* subset — not the nearest N. A module cannot change that; it is
 *    hardcoded at both cap sites. So with a cap of 99 on a 960-mob pull, 90 %
 *    of the pull is ignored per cast *at random*, including mobs standing right
 *    next to the caster. Prefer a high cap used as a safety limit over a low
 *    cap used as a normal-operation balance lever.
 * 2. Casts that skip `CheckCast` entirely (fully-triggered casts) are never
 *    seen by this hook and stay uncapped.
 */

#include "Config.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"

#include <atomic>

namespace
{
    bool   conf_Enable            = false;
    uint32 conf_MaxTargets        = 0;
    bool   conf_HarmfulOnly       = true;
    bool   conf_PlayerCastersOnly = true;
    bool   conf_DungeonsOnly      = false;
    uint32 conf_LogEveryNth       = 0;

    // Diagnostics only. Relaxed so it stays correct if MapUpdate.Threads is ever
    // raised above 1, and so it never becomes a synchronisation point on the
    // cast path.
    std::atomic<uint32> s_cappedCasts{ 0 };
}

class AoECapWorldScript : public WorldScript
{
public:
    AoECapWorldScript() : WorldScript("AoECapWorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        conf_Enable            = sConfigMgr->GetOption<bool>("AoECap.Enable", false);
        conf_MaxTargets        = sConfigMgr->GetOption<uint32>("AoECap.MaxTargets", 250);
        conf_HarmfulOnly       = sConfigMgr->GetOption<bool>("AoECap.HarmfulOnly", true);
        conf_PlayerCastersOnly = sConfigMgr->GetOption<bool>("AoECap.PlayerCastersOnly", true);
        conf_DungeonsOnly      = sConfigMgr->GetOption<bool>("AoECap.DungeonsOnly", false);
        conf_LogEveryNth       = sConfigMgr->GetOption<uint32>("AoECap.LogEveryNth", 0);

        if (conf_Enable && conf_MaxTargets)
        {
            LOG_INFO("module", ">> mod-aoe-cap: ACTIVE - uncapped area spells limited to {} targets "
                "(harmfulOnly={}, playerCastersOnly={}, dungeonsOnly={}). Note: the core selects "
                "the subset at RANDOM, not by distance.",
                conf_MaxTargets, conf_HarmfulOnly, conf_PlayerCastersOnly, conf_DungeonsOnly);
        }
        else
        {
            LOG_INFO("module", ">> mod-aoe-cap: inactive (AoECap.Enable=0 or MaxTargets=0) - "
                "spell target limits behave exactly as the spell data defines.");
        }
    }
};

class AoECapSpellScript : public SpellSC
{
public:
    AoECapSpellScript() : SpellSC("AoECapSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& res) override
    {
        if (!conf_Enable || !conf_MaxTargets || !spell)
            return;

        // Never interfere with a cast that is already failing for another reason.
        if (res != SPELL_CAST_OK)
            return;

        SpellInfo const* info = spell->GetSpellInfo();
        if (!info)
            return;

        // A spell that declares its own limit keeps it, untouched. This module
        // only ever fills in a value where the spell data says "unlimited".
        if (info->MaxAffectedTargets != 0)
            return;

        // Only area/cone selection is capped by the core, so nothing else can be
        // affected by setting this value.
        if (info->IsPassive() || !info->IsTargetingArea())
            return;

        // Beneficial area effects (group heals, raid buffs) must not be silently
        // truncated unless the operator explicitly asks for it.
        if (conf_HarmfulOnly && info->IsPositive())
            return;

        // AzerothCore #27627 made Spell::GetCaster() a WorldObject*. A GameObject caster counts
        // as its owner, as the trigger creature it used to cast through did (a player's trap).
        WorldObject* casterObject = spell->GetCaster();
        Unit* caster = casterObject ? casterObject->ToUnit() : nullptr;
        if (!caster && casterObject)
            if (GameObject* go = casterObject->ToGameObject())
                caster = go->GetOwner();
        if (!caster)
            return;

        // Resolve through pets/totems/charms so a player's minion counts as the
        // player, matching how the rest of the fleet attributes ownership.
        if (conf_PlayerCastersOnly && !caster->GetCharmerOrOwnerPlayerOrPlayerItself())
            return;

        if (conf_DungeonsOnly)
        {
            Map* map = caster->GetMap();
            if (!map || !map->IsDungeon())
                return;
        }

        spell->SetSpellValue(SPELLVALUE_MAX_TARGETS, static_cast<int32>(conf_MaxTargets));

        // Bounded diagnostics: every Nth cap, never one line per cast.
        if (conf_LogEveryNth)
        {
            uint32 const total = s_cappedCasts.fetch_add(1, std::memory_order_relaxed) + 1;
            if (total % conf_LogEveryNth == 0)
            {
                LOG_INFO("module", "mod-aoe-cap: {} area casts capped at {} targets so far.",
                    total, conf_MaxTargets);
            }
        }
    }
};

void AddSC_AoECap()
{
    new AoECapWorldScript();
    new AoECapSpellScript();
}
