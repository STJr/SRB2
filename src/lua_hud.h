// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014      by John "JTE" Muniz.
// Copyright (C) 2014      by Sonic Team Junior.
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
	// Singleplayer / Co-op
	hud_score,
	hud_time,
	hud_rings,
	hud_lives,
	// NiGHTS mode
	hud_nightslink,
	hud_nightsdrill,
	hud_nightsrings,
	hud_nightsscore,
	hud_nightstime,
	hud_nightsrecords,
	// TAB scores overlays
	hud_rankings,
	hud_coopemeralds,
	hud_tokens,
	hud_tabemblems,
	hud_ndringenergy,
	hud_ndemblems,
	hud_ndtime,
	hud_ndscore,
	hud_ndlives,
	hud_bosshealth,
	hud_MAX
};

extern boolean hud_running;

boolean LUA_HudEnabled(enum hud option);

void LUAh_GameHUD(player_t *stplyr);
void LUAh_ScoresHUD(void);
