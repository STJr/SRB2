// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2016 by John "JTE" Muniz.
// Copyright (C) 2014-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hud.h
/// \brief HUD enable/disable flags for Lua scripting

enum hud {
	hud_stagetitle = 0,
	hud_textspectator,
	hud_crosshair,
	// Singleplayer / Co-op
	hud_score,
	hud_time,
	hud_rings,
	hud_lives,
	// Match / CTF / Tag / Ringslinger
	hud_weaponrings,
	hud_powerstones,
	hud_teamscores,
	// NiGHTS mode
	hud_nightslink,
	hud_nightsdrill,
	hud_nightsspheres,
	hud_nightsscore,
	hud_nightstime,
	hud_nightsrecords,
	// TAB scores overlays
	hud_rankings,
	hud_coopemeralds,
	hud_tokens,
	hud_tabemblems,
	// Intermission
	hud_intermissiontally,
	hud_intermissionmessages,
	hud_MAX
};

extern boolean hud_running;

boolean LUA_HudEnabled(enum hud option);

void LUAh_GameHUD(player_t *stplyr);
void LUAh_ScoresHUD(void);
void LUAh_TitleHUD(void);
void LUAh_TitleCardHUD(player_t *stplayr);
void LUAh_IntermissionHUD(void);
