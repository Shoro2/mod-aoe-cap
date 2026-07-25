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

// From SC
// NOTE: this symbol must stay unique across the whole module fleet — a colliding
// AddSC_* name silently drops an entire object file (see the FL handover doc,
// trap 7: the gossip-bug saga).
void AddSC_AoECap();

// Add all
void Addmod_aoe_capScripts()
{
    AddSC_AoECap();
}
