// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by Matthew "Inuyasha" Walsh.
// Copyright (C) 2012-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_cond.c
/// \brief Unlockable condition system for SRB2 version 2.1

#include "m_cond.h"
#include "doomstat.h"
#include "z_zone.h"

#include "hu_stuff.h" // CEcho
#include "v_video.h" // video flags

#include "g_game.h" // record info
#include "r_things.h" // numskins
#include "r_draw.h" // R_GetColorByName

// Map triggers for linedef executors
// 32 triggers, one bit each
UINT32 unlocktriggers;

// The meat of this system lies in condition sets
conditionset_t conditionSets[MAXCONDITIONSETS];

// Default Emblem locations
emblem_t emblemlocations[MAXEMBLEMS] =
{
	// GREEN FLOWER 1
	// ---
	{0,  8156,  6936,   129, 1, 'A', SKINCOLOR_BLUE, 0,
		"Go get your feet wet\n"
		"to find this, the first emblem.\n"
		"Yes, it's very deep.", 0},
	{0,  3184,  1812,   928, 1, 'B', SKINCOLOR_LAVENDER, 0,
		"There are many rings,\n"
		"but this one's not what you think.\n"
		"There lies the emblem.", 0},
	{0,  9024,  6716,   769, 1, 'C', SKINCOLOR_RED, 0,
		"Right next to a lake,\n"
		"a ledge has been constructed.\n"
		"Near there is the goal.", 0},
	{0,  2080,  -384,   512, 1, 'D', SKINCOLOR_ORANGE, 0,
		"Streams come to an end\n"
		"where they can no longer fall.\n"
		"But if you went up...", 0},
	{0,  -336,  2064,   195, 1, 'E', SKINCOLOR_EMERALD, 0,
		"This one's in plain sight.\n"
		"Why haven't you claimed it?\n"
		"Surely you saw it.", 0},

	{ET_SCORE, 0,0,0,  1, 'S', SKINCOLOR_BROWN,      125000, "", 0},
	{ET_TIME,  0,0,0,  1, 'T', SKINCOLOR_GREY,   20*TICRATE, "", 0},
	{ET_RINGS, 0,0,0,  1, 'R', SKINCOLOR_GOLD,          200, "", 0},


	// GREEN FLOWER 2
	// ---
	{0,  2624, -6816,  1332, 2, 'A', SKINCOLOR_BLUE, 0,
		"Near the giant lake\n"
		"lies a cave with a 1-Up.\n"
		"An emblem's there, too!", 0},
	{0, -5728, -2848,  2848, 2, 'B', SKINCOLOR_LAVENDER, 0,
		"Near the final lake,\n"
		"a higher lake falls on in.\n"
		"Three platforms await.", 0},
	{0,  3648,  6464,  2576, 2, 'C', SKINCOLOR_RED, 0,
		"Near the level's start,\n"
		"a bridge crosses a river.\n"
		"What's that river's source?", 0},
	{0, -2032,-10048,   986, 2, 'D', SKINCOLOR_ORANGE, 0,
		"Near the level's end,\n"
		"another bridge spans a lake.\n"
		"What could be under...?", 0},
	{0,  -170,   491,  3821, 2, 'E', SKINCOLOR_EMERALD, 0,
		"An ivied tunnel\n"
		"has a corner that's sunlit.\n"
		"Go reach for the sky!", 0},

	{ET_SCORE, 0,0,0,  2, 'S', SKINCOLOR_BROWN,      150000, "", 0},
	{ET_TIME,  0,0,0,  2, 'T', SKINCOLOR_GREY,   40*TICRATE, "", 0},
	{ET_RINGS, 0,0,0,  2, 'R', SKINCOLOR_GOLD,          200, "", 0},


	// GREEN FLOWER 3
	// ---
	{ET_TIME,  0,0,0,  3, 'T', SKINCOLOR_GREY,   30*TICRATE, "", 0},


	// TECHNO HILL 1
	// ---
	{0, -5664, -5072,  2396, 4, 'A', SKINCOLOR_BLUE, 0,
		"Three pipes reside near\n"
		"where our heroes' paths split off.\n"
		"You'll have to look up!", 0},
	{0,  -784,-13968,  2888, 4, 'B', SKINCOLOR_LAVENDER, 0,
		"Climbing yields great range.\n"
		"Yet, on a path for climbers,\n"
		"flying is the key.", 0},
	{0,  4160, -5824,  3776, 4, 'C', SKINCOLOR_RED, 0,
		"That's sure lots of slime.\n"
		"Say, do you ever wonder\n"
		"what's dumping it all?", 0},
	{0,  6400, -8352,  1764, 4, 'D', SKINCOLOR_ORANGE, 0,
		"Spinning through small gaps\n"
		"can slip you into a cave.\n"
		"In that cave's first stretch...", 0},
	{0,  2848, -9088,   488, 4, 'E', SKINCOLOR_EMERALD, 0,
		"The slime lake is deep,\n"
		"but reaching the floor takes height.\n"
		"Scream \"Geronimo!\"...", 0},

	{ET_SCORE, 0,0,0,  4, 'S', SKINCOLOR_BROWN,       75000, "", 0},
	{ET_TIME,  0,0,0,  4, 'T', SKINCOLOR_GREY,   75*TICRATE, "", 0},
	{ET_RINGS, 0,0,0,  4, 'R', SKINCOLOR_GOLD,          300, "", 0},


	// TECHNO HILL 2
	// ---
	{0,-19138, -2692,   688, 5, 'A', SKINCOLOR_BLUE, 0,
		"Near the first checkpoint,\n"
		"a bridge crosses a slime pool.\n"
		"(Sensing a pattern?)", 0},
	{0,-13120,  8062,  1248, 5, 'B', SKINCOLOR_LAVENDER, 0,
		"Behind the windows,\n"
		"near crushers, ever smashing\n"
		"a conveyor belt.", 0},
	{0,   580,  4552,  1344, 5, 'C', SKINCOLOR_RED, 0,
		"A pipe drops onto\n"
		"a half-outdoors conveyor.\n"
		"But is it empty?", 0},
	{0,   192, -8768,    24, 5, 'D', SKINCOLOR_ORANGE, 0,
		"There is a hallway\n"
		"that a button floods with slime.\n"
		"Go through it again!", 0},
	{0, -2468,-12128,  1312, 5, 'E', SKINCOLOR_EMERALD, 0,
		"Jumping on turtles\n"
		"will send you springing skyward.\n"
		"Now, do that six times...", 0},

	{ET_SCORE, 0,0,0,  5, 'S', SKINCOLOR_BROWN,      100000, "", 0},
	{ET_TIME,  0,0,0,  5, 'T', SKINCOLOR_GREY,  120*TICRATE, "", 0},
	{ET_RINGS, 0,0,0,  5, 'R', SKINCOLOR_GOLD,          600, "", 0},


	// TECHNO HILL 3
	// ---
	{ET_TIME,  0,0,0,  6, 'T', SKINCOLOR_GREY,   40*TICRATE, "", 0},


	// DEEP SEA 1
	// ---
	{0,-16224, -2880,  3530, 7, 'A', SKINCOLOR_BLUE, 0,
		"Climb up two maze walls.\n"
		"Break the roof, then a corner.\n"
		"There, glide, but stay dry.", 0},
	{0, -8224,   896,  1056, 7, 'B', SKINCOLOR_LAVENDER, 0,
		"Follow the left path.\n"
		"A square green button lurks deep.\n"
		"Weight it down, somehow.", 0},
	{0,  4992, -5072,  4136, 7, 'C', SKINCOLOR_RED, 0,
		"A certain path holds\n"
		"many gargoyle puzzles.\n"
		"Victors reach a \"V\".", 0},
	{0,  4576,  5168,  2660, 7, 'D', SKINCOLOR_ORANGE, 0,
		"A caved-in hallway?\n"
		"The floor falls; the path goes down.\n"
		"But those rocks looked weak...", 0},
	{0, 12576, 16096,  -992, 7, 'E', SKINCOLOR_EMERALD, 0,
		"The end is quite dry.\n"
		"Some rocks dam the water in.\n"
		"Knuckles can fix that...", 0},

	{ET_SCORE, 0,0,0,  7, 'S', SKINCOLOR_BROWN,       75000, "", 0},
	{ET_TIME,  0,0,0,  7, 'T', SKINCOLOR_GREY,  120*TICRATE, "", 0},
	{ET_RINGS, 0,0,0,  7, 'R', SKINCOLOR_GOLD,          400, "", 0},


	// DEEP SEA 2
	// ---
	{0,-15040,  6976,  2016, 8, 'A', SKINCOLOR_BLUE, 0,
		"A waterfall lands\n"
		"near a starpost in a cave.\n"
		"It's dark up there, but...", 0},
	{0,  4288,  2912,   544, 8, 'B', SKINCOLOR_LAVENDER, 0,
		"So many blocks here!\n"
		"Take five; bathe in the fountain.\n"
		"Hmmm? A hidden path...?", 0},
	{0, -5696, 16992,   791, 8, 'C', SKINCOLOR_RED, 0,
		"An ornate dragon\n"
		"faces a secret passage.\n"
		"Knuckles! Don't get crushed!", 0},
	{0,-13344, 18688,  1034, 8, 'D', SKINCOLOR_ORANGE, 0,
		"In the current maze\n"
		"hides a dark room of columns.\n"
		"Find it, then look up.", 0},
	{0,  3104, 16192,  2408, 8, 'E', SKINCOLOR_EMERALD,  0,
		"That same dragon's eye\n"
		"hides another secret room.\n"
		"There, solve its riddle.", 0},

	{ET_SCORE, 0,0,0,  8, 'S', SKINCOLOR_BROWN,       50000, "", 0},
	{ET_TIME,  0,0,0,  8, 'T', SKINCOLOR_GREY,  150*TICRATE, "", 0},
	{ET_RINGS, 0,0,0,  8, 'R', SKINCOLOR_GOLD,          250, "", 0},


	// DEEP SEA 3
	// ---
	{ET_TIME,  0,0,0,  9, 'T', SKINCOLOR_GREY,   90*TICRATE, "", 0},


	// CASTLE EGGMAN 1
	// ---
	{0, -6176, -5184,  -128, 10, 'A', SKINCOLOR_BLUE, 0,
		"A drain feeds the lake.\n"
		"Water rushes quickly through.\n"
		"Go against the flow.", 0},
	{0,  3648,-15296,  -992, 10, 'B', SKINCOLOR_LAVENDER, 0,
		"The left starting path\n"
		"goes atop a large wood deck.\n"
		"Checked underneath yet?", 0},
	{0, 11712, 21312,  5472, 10, 'C', SKINCOLOR_RED, 0,
		"At last, the castle!\n"
		"Hold up! Don't just barge right in!\n"
		"What's the facade hold...?", 0},
	{0, 20224, 13344,  3104, 10, 'D', SKINCOLOR_ORANGE, 0,
		"The final approach!\n"
		"A tower holds the emblem\n"
		"near a ring arrow.", 0},
	{0,  9472, -5890,   710, 10, 'E', SKINCOLOR_EMERALD, 0,
		"The right starting path\n"
		"hides this near a canopy,\n"
		"high, where two trees meet.", 0},

	{ET_SCORE, 0,0,0, 10, 'S', SKINCOLOR_BROWN,       50000, "", 0},
	{ET_TIME,  0,0,0, 10, 'T', SKINCOLOR_GREY,  120*TICRATE, "", 0},
	{ET_RINGS, 0,0,0, 10, 'R', SKINCOLOR_GOLD,          200, "", 0},


	// CASTLE EGGMAN 2
	// ---
	{0,   832,-15168,  7808, 11, 'A', SKINCOLOR_BLUE, 0,
		"Find a trick bookcase\n"
		"that hides a darkened hallway.\n"
		"There, climb a tower.", 0},
	{0,-18460,-22180,  2416, 11, 'B', SKINCOLOR_LAVENDER, 0,
		"Down in the dungeon,\n"
		"a cracked wall hides secret paths.\n"
		"Echidnas only!", 0},
	{0, -6144,-11792,  3232, 11, 'C', SKINCOLOR_RED, 0,
		"A room you can flood!\n"
		"A brown grate's near its exit.\n"
		"Knuckles can break it...", 0},
	{0,  4608, -7024,  4256, 11, 'D', SKINCOLOR_ORANGE, 0,
		"Some of these bookshelves\n"
		"are not flush against the walls.\n"
		"Wonder why that is?", 0},
	{0, 12708,-13536,  4768, 11, 'E', SKINCOLOR_EMERALD, 0,
		"The ending's towers\n"
		"are hiding a small alcove.\n"
		"Check around outside.", 0},

	{ET_SCORE, 0,0,0, 11, 'S', SKINCOLOR_BROWN,      400000, "", 0},
	{ET_TIME,  0,0,0, 11, 'T', SKINCOLOR_GREY,  210*TICRATE, "", 0},
	{ET_RINGS, 0,0,0, 11, 'R', SKINCOLOR_GOLD,          600, "", 0},


	// CASTLE EGGMAN 3
	// ---
	{ET_TIME,  0,0,0, 12, 'T', SKINCOLOR_GREY,  120*TICRATE, "", 0},


	// ARID CANYON 1
	// ---
	{0,  3488,  2208,  3072, 13, 'A', SKINCOLOR_BLUE, 0,
		"A rather large gap\n"
		"must be crossed by way of tram.\n"
		"At its end, jump left.", 0},
	{0, -7552, 10464,  4094, 13, 'B', SKINCOLOR_LAVENDER, 0,
		"Crushers that go up!\n"
		"Mind your step; if they're triggered,\n"
		"they'll block this emblem.", 0},
	{0,-12093, 14575,  5752, 13, 'C', SKINCOLOR_RED, 0,
		"There's an oil lake\n"
		"that you can sink deep into.\n"
		"Drain it, and explore.", 0},
	{0,   512, -7136,  4640, 13, 'D', SKINCOLOR_ORANGE, 0,
		"Not far from the start,\n"
		"if you climb toward the sky,\n"
		"the cliffs hide something.", 0},
	{0, 12504,  6848,  3424, 13, 'E', SKINCOLOR_EMERALD, 0,
		"Right by the exit,\n"
		"an emblem lies on a cliff.\n"
		"Ride ropes to reach it.", 0},

	{ET_SCORE, 0,0,0, 13, 'S', SKINCOLOR_BROWN,       50000, "", 0},
	{ET_TIME,  0,0,0, 13, 'T', SKINCOLOR_GREY,  120*TICRATE, "", 0},
	{ET_RINGS, 0,0,0, 13, 'R', SKINCOLOR_GOLD,          300, "", 0},


	// RED VOLCANO 1
	// ---
	{0,-13184, 11424,  3080, 16, 'A', SKINCOLOR_BLUE, 0,
		"Look around the room,\n"
		"just before you clear the stage;\n"
		"something's hidden there!", 0},
	{0, -2816,  3120,  3044, 16, 'B', SKINCOLOR_LAVENDER, 0,
		"Ever look upwards\n"
		"when you're traversing across\n"
		"collapsing platforms?", 0},
	{0,  6720,  6784,  1452, 16, 'C', SKINCOLOR_RED, 0,
		"Check out a corner\n"
		"of a lake of magma near\n"
		"spinning jets of flame.", 0},
	{0, -5504,  9824,   800, 16, 'D', SKINCOLOR_ORANGE, 0,
		"Where once a bridge stood,\n"
		"now magma falls from above.\n"
		"The bridge dropped something...", 0},
	{0,  8287,-11043,  1328, 16, 'E', SKINCOLOR_EMERALD, 0,
		"A lake of magma\n"
		"ebbs and flows unendingly.\n"
		"Wait for its nadir.", 0},

	{ET_SCORE, 0,0,0, 16, 'S', SKINCOLOR_BROWN,       30000, "", 0},
	{ET_TIME,  0,0,0, 16, 'T', SKINCOLOR_GREY,  120*TICRATE, "", 0},
	{ET_RINGS, 0,0,0, 16, 'R', SKINCOLOR_GOLD,          100, "", 0},


	// EGG ROCK 1
	// ---
	{0,-10976, -7328,  1584, 22, 'A', SKINCOLOR_BLUE, 0,
		"Vanishing platforms,\n"
		"then collapsing ones herald\n"
		"a last-second jump.", 0},
	{0, -6592,-11200,  2208, 22, 'B', SKINCOLOR_LAVENDER, 0,
		"What is this red stuff?\n"
		"You can't breathe it in, but look!\n"
		"It can't reach up there...", 0},
	{0,  6816,   832,   936, 22, 'C', SKINCOLOR_RED, 0,
		"The team's paths diverge.\n"
		"Should Tails run the crusher path?\n"
		"No! Fly outside it!", 0},
	{0,  6942, -8902,  2080, 22, 'D', SKINCOLOR_ORANGE, 0,
		"Don't jump too high here!\n"
		"No conveyor will catch you;\n"
		"you'd fall to your death.", 0},
	{0, -6432, -6192,   584, 22, 'E', SKINCOLOR_EMERALD, 0,
		"Conveyors! Magma!\n"
		"What an intense room this is!\n"
		"But, what brought you here?", 0},

	{ET_SCORE, 0,0,0, 22, 'S', SKINCOLOR_BROWN,       25000, "", 0},
	{ET_TIME,  0,0,0, 22, 'T', SKINCOLOR_GREY,  120*TICRATE, "", 0},
	{ET_RINGS, 0,0,0, 22, 'R', SKINCOLOR_GOLD,          150, "", 0},


	// EGG ROCK 2
	// ---
	{0, -6672,  7792,   352, 23, 'A', SKINCOLOR_BLUE, 0,
		"Walk on the ceiling;\n"
		"resist the urge to flip back!\n"
		"Find the cyan path...", 0},
	{0,-12256, 15136,  -288, 23, 'B', SKINCOLOR_LAVENDER, 0,
		"X marks the spot? Nope!\n"
		"Try standing further away\n"
		"when the timer flips.", 0},
	{0,  1536, 16224,  1144, 23, 'C', SKINCOLOR_RED, 0,
		"There is more than one\n"
		"elevator inside the\n"
		"elevator shaft...", 0},
	{0,-15968, 14192,  3152, 23, 'D', SKINCOLOR_ORANGE, 0,
		"Gears with missing teeth\n"
		"can hide a clever secret!\n"
		"Think Green Hill Zone boss.", 0},
	{0,  1920, 20608,  1064, 23, 'E', SKINCOLOR_EMERALD, 0,
		"Just before you reach\n"
		"the defective cargo bay,\n"
		"fly under a bridge.", 0},

	{ET_SCORE, 0,0,0, 23, 'S', SKINCOLOR_BROWN,       60000, "", 0},
	{ET_TIME,  0,0,0, 23, 'T', SKINCOLOR_GREY,  300*TICRATE, "", 0},
	{ET_RINGS, 0,0,0, 23, 'R', SKINCOLOR_GOLD,          250, "", 0},


	// EGG ROCK 3
	// ---
/* Just in case, I'll leave these here in the source.
	{0,   848, -3584,   592, 24, 'A', SKINCOLOR_BLUE, 0,
		"[PH] Hiding at the end of the first hallway.", 0},
	{0,-10368, -2816,   144, 24, 'B', SKINCOLOR_LAVENDER, 0,
		"Directions are meaningless.", 0},
	{0, -8160, -5952,   560, 24, 'C', SKINCOLOR_RED, 0,
		"[PH] In the ceiling of the conveyor belt + laser hallway.", 0},
	{0,-13728,-13728,  1552, 24, 'D', SKINCOLOR_ORANGE, 0,
		"[PH] On top of the platform with rows of spikes in reverse gravity.", 0},
	{0,-14944,   768,  1232, 24, 'E', SKINCOLOR_EMERALD, 0,
		"Follow the leader.", 0},
*/

	{ET_SCORE, 0,0,0, 24, 'S', SKINCOLOR_BROWN,       14000, "", 0},
	{ET_TIME,  0,0,0, 24, 'T', SKINCOLOR_GREY,  210*TICRATE, "", 0},
	{ET_RINGS, 0,0,0, 24, 'R', SKINCOLOR_GOLD,          100, "", 0},


	// EGG ROCK CORE
	// ---
	{ET_TIME,  0,0,0, 25, 'T', SKINCOLOR_GREY,  100*TICRATE, "", 0},


	// PIPE TOWERS
	// ---
	{0,  3182,  5040,  3008, 30, 'A', SKINCOLOR_BLUE, 0,
		"A pipe in the roof\n"
		"eternally drops water.\n"
		"Something's stuck up there.", 0},
	{0, -2400,  5984,  2752, 30, 'B', SKINCOLOR_LAVENDER, 0,
		"Pushing a red switch\n"
		"raises the water level;\n"
		"from there, can't miss it.", 0},
	{0,  6112,  7008,  4032, 30, 'C', SKINCOLOR_RED, 0,
		"A high-up passage\n"
		"hides near the second checkpoint.\n"
		"Climb in; then, climb more.", 0},
	{0, 11424, -4832,  1376, 30, 'D', SKINCOLOR_ORANGE, 0,
		"The underground room\n"
		"with platforms that fall and rise\n"
		"only LOOKS empty...", 0},
	{0 , 4960, -6112,  1312, 30, 'E', SKINCOLOR_EMERALD, 0,
		"This one's straightforward.\n"
		"What comes to mind when I say:\n"
		"\"WELCOME TO WARP ZONE!\"?", 0},

	{ET_SCORE, 0,0,0, 30, 'S', SKINCOLOR_BROWN,       75000, "", 0},
	{ET_TIME,  0,0,0, 30, 'T', SKINCOLOR_GREY,  100*TICRATE, "", 0},
	{ET_RINGS, 0,0,0, 30, 'R', SKINCOLOR_GOLD,          300, "", 0},


	// AERIAL GARDEN
	// ---
	{0, 10176,-14304,  1796, 40, 'A', SKINCOLOR_BLUE, 0,
		"A central tower,\n"
		"one with many waterfalls,\n"
		"hides a secret room.", 0},
	{0,   480, 17696,  6496, 40, 'B', SKINCOLOR_LAVENDER, 0,
		"Hidden off the path\n"
		"lies a skyscraping tower.\n"
		"A lake's at the top.", 0},
	{0, -8896, 13248,  3362, 40, 'C', SKINCOLOR_RED, 0,
		"Find all four buttons\n"
		"that sink when you stand on them.\n"
		"They'll open a door...", 0},
	{0, -8896, -9952,  2480, 40, 'D', SKINCOLOR_ORANGE, 0,
		"Much like the last one,\n"
		"you need to find some switches.\n"
		"Only two, this time.", 0},
	{0, 13184, 18880,  6672, 40, 'E', SKINCOLOR_EMERALD, 0,
		"The inner sanctum!\n"
		"Teleport to its switches;\n"
		"then, check near the goal.", 0},

	{ET_SCORE, 0,0,0, 40, 'S', SKINCOLOR_BROWN,      300000, "", 0},
	{ET_TIME,  0,0,0, 40, 'T', SKINCOLOR_GREY,  240*TICRATE, "", 0},
	{ET_RINGS, 0,0,0, 40, 'R', SKINCOLOR_GOLD,         1200, "", 0},


	// AZURE TEMPLE
	// ---
	{0, -2400,  7552,  1120, 41, 'A', SKINCOLOR_BLUE, 0,
		"For those who can swim,\n"
		"a long tunnel hides rewards.\n"
		"Do mind the Buzzes!", 0},
	{0,   -64, 14016,  2072, 41, 'B', SKINCOLOR_LAVENDER, 0,
		"So many skylights!\n"
		"A markedly large one hides\n"
		"behind a starpost...", 0},
	{0,  2976, 13920,   -32, 41, 'C', SKINCOLOR_RED, 0,
		"When you reach gauntlets\n"
		"of diagonal fire,\n"
		"check out the corners.", 0},
	{0,  2176, 22592,  1376, 41, 'D', SKINCOLOR_ORANGE, 0,
		"A room of currents;\n"
		"most of them are marked by spikes.\n"
		"This one? A corner.", 0},
	{0, -4128, 21344,  1120, 41, 'E', SKINCOLOR_EMERALD, 0,
		"The only way to hit\n"
		"all those gems at once is with\n"
		"a radial blast.", 0},

	{ET_SCORE, 0,0,0, 41, 'S', SKINCOLOR_BROWN,      425000, "", 0},
	{ET_TIME,  0,0,0, 41, 'T', SKINCOLOR_GREY,  240*TICRATE, "", 0},
	{ET_RINGS, 0,0,0, 41, 'R', SKINCOLOR_GOLD,          300, "", 0},


	// FLORAL FIELD
	// ---
	{0, 5394, -996, 160, 50, 'N', SKINCOLOR_RUST, 0, "", 0},
	{ET_NGRADE, 0,0,0,   50, 'Q', SKINCOLOR_CYAN,     GRADE_A, "", 0},
	{ET_NTIME,  0,0,0,   50, 'T', SKINCOLOR_GREY,  40*TICRATE, "", 0},


	// TOXIC PLATEAU
	// ---
	{0, 780, -1664, 32, 51, 'N', SKINCOLOR_RUST, 0, "", 0},
	{ET_NGRADE, 0,0,0,  51, 'Q', SKINCOLOR_CYAN,     GRADE_A, "", 0},
	{ET_NTIME,  0,0,0,  51, 'T', SKINCOLOR_GREY,  50*TICRATE, "", 0},


	// FLOODED COVE
	// ---
	{0, 1824, -1888, 2448, 52, 'N', SKINCOLOR_RUST, 0, "", 0},
	{ET_NGRADE, 0,0,0,     52, 'Q', SKINCOLOR_CYAN,     GRADE_A, "", 0},
	{ET_NTIME,  0,0,0,     52, 'T', SKINCOLOR_GREY,  90*TICRATE, "", 0},


	// CAVERN FORTRESS
	// ---
	{0, -3089, -431, 1328, 53, 'N', SKINCOLOR_RUST, 0, "", 0},
	{ET_NGRADE, 0,0,0,     53, 'Q', SKINCOLOR_CYAN,     GRADE_A, "", 0},
	{ET_NTIME,  0,0,0,     53, 'T', SKINCOLOR_GREY,  75*TICRATE, "", 0},


	// DUSTY WASTELAND
	// ---
	{0, 957, 924, 2956, 54, 'N', SKINCOLOR_RUST, 0, "", 0},
	{ET_NGRADE, 0,0,0,  54, 'Q', SKINCOLOR_CYAN,     GRADE_A, "", 0},
	{ET_NTIME,  0,0,0,  54, 'T', SKINCOLOR_GREY,  65*TICRATE, "", 0},


	// MAGMA CAVES
	// ---
	{0, -2752, 3104, 1800, 55, 'N', SKINCOLOR_RUST, 0, "", 0},
	{ET_NGRADE, 0,0,0,     55, 'Q', SKINCOLOR_CYAN,     GRADE_A, "", 0},
	{ET_NTIME,  0,0,0,     55, 'T', SKINCOLOR_GREY,  80*TICRATE, "", 0},


	// EGG SATELLITE
	// ---
	{0, 5334, -609, 3426, 56, 'N', SKINCOLOR_RUST, 0, "", 0},
	{ET_NGRADE, 0,0,0,    56, 'Q', SKINCOLOR_CYAN,     GRADE_A, "", 0},
	{ET_NTIME,  0,0,0,    56, 'T', SKINCOLOR_GREY, 120*TICRATE, "", 0},


	// BLACK HOLE
	// ---
	{0, 2108, 3776, 32, 57, 'N', SKINCOLOR_RUST, 0, "", 0},
	{ET_NGRADE, 0,0,0,  57, 'Q', SKINCOLOR_CYAN,     GRADE_A, "", 0},
	{ET_NTIME,  0,0,0,  57, 'T', SKINCOLOR_GREY, 150*TICRATE, "", 0},


	// SPRING HILL
	// ---
	{0, -1840, -1024, 1644, 58, 'N', SKINCOLOR_RUST, 0, "", 0},
	{ET_NGRADE, 0,0,0,      58, 'Q', SKINCOLOR_CYAN,     GRADE_A, "", 0},
	{ET_NTIME,  0,0,0,      58, 'T', SKINCOLOR_GREY,  60*TICRATE, "", 0},
};

// Default Extra Emblems
extraemblem_t extraemblems[MAXEXTRAEMBLEMS] =
{
	{"Game Complete",  "Complete 1P Mode",                    10, 'X', SKINCOLOR_BLUE, 0},
	{"All Emeralds",   "Complete 1P Mode with all Emeralds",  11, 'V', SKINCOLOR_GREY, 0},
	{"Perfect Bonus",  "Perfect Bonus on a non-secret stage", 30, 'P', SKINCOLOR_GOLD, 0},
	{"PLACEHOLDER", "PLACEHOLDER", 0, 'O', SKINCOLOR_RUST, 0},
	{"NiGHTS Mastery", "Show your mastery of NiGHTS!",        22, 'W', SKINCOLOR_CYAN, 0},
};

// Default Unlockables
unlockable_t unlockables[MAXUNLOCKABLES] =
{
	// Name, Objective, Menu Height, ConditionSet, Unlock Type, Variable, NoCecho, NoChecklist
	/* 01 */ {"Record Attack",     "/", 0, 1, SECRET_RECORDATTACK,  0,  true,  true, 0},
	/* 02 */ {"NiGHTS Mode",       "/",            0, 2, SECRET_NIGHTSMODE,    0,  true,  true, 0},

	/* 03 */ {"Play Credits",      "/", 30, 10, SECRET_CREDITS,   0,  true,  true, 0},
	/* 04 */ {"Sound Test",        "/", 40, 10, SECRET_SOUNDTEST, 0, false, false, 0},

	/* 05 */ {"EXTRA LEVELS", "/", 58, 0, SECRET_HEADER, 0, true, true, 0},

	/* 06 */ {"Aerial Garden Zone", "/", 70, 11, SECRET_WARP, 40, false, false, 0},
	/* 07 */ {"Azure Temple Zone",  "/",      80, 20, SECRET_WARP, 41, false, false, 0},

	/* 08 */ {"BONUS LEVELS", "/", 98, 0, SECRET_HEADER, 0, true, true, 0},

	/* 09 */ {"PLACEHOLDER", "/", 0, 0, SECRET_NONE, 0, true, true, 0},
	/* 10 */ {"Mario Koopa Blast", "/",   110, 42, SECRET_WARP,         30, false, false, 0},
	/* 11 */ {"PLACEHOLDER", "/", 0, 0, SECRET_NONE, 0, true, true, 0},

	/* 12 */ {"Spring Hill Zone", "/",          0, 44, SECRET_NONE, 0, false, false, 0},
	/* 13 */ {"Black Hole",       "Get grade A in all Special Stages", 0, 50, SECRET_NONE, 0, false, true, 0},

	/* 14 */ {"Emblem Hints", "/", 0, 41, SECRET_EMBLEMHINTS, 0, false,  true, 0},
	/* 15 */ {"Emblem Radar", "/", 0, 43, SECRET_ITEMFINDER,  0, false,  true, 0},

	/* 16 */ {"Pandora's Box", "/",  0, 45, SECRET_PANDORA,     0, false, false, 0},
	/* 17 */ {"Level Select",  "/", 20, 45, SECRET_LEVELSELECT, 1, false,  true, 0},
};

// Default number of emblems and extra emblems
INT32 numemblems = 155;
INT32 numextraemblems = 5;

// DEFAULT CONDITION SETS FOR SRB2 2.1:
void M_SetupDefaultConditionSets(void)
{
	memset(conditionSets, 0, sizeof(conditionSets));

	// --   1: Complete GFZ1
	M_AddRawCondition(1, 1, UC_MAPBEATEN, 1, 0, 0);

	// --   2: Complete SS1
	M_AddRawCondition(2, 1, UC_MAPBEATEN, 50, 0, 0);

	// --  10: Complete the game
	M_AddRawCondition(10, 1, UC_GAMECLEAR, 1, 0, 0);

	// --  11: Complete the game with all emeralds
	M_AddRawCondition(11, 1, UC_ALLEMERALDS, 1, 0, 0);

	// --  20: Beat AGZ
	M_AddRawCondition(20, 1, UC_MAPBEATEN, 40, 0, 0);

	// --  22: Beat Black Hole
	M_AddRawCondition(22, 1, UC_MAPBEATEN, 57, 0, 0);

	// --  30: Perfect Bonus
	M_AddRawCondition(30, 1,  UC_MAPPERFECT,  1, 0, 0);
	M_AddRawCondition(30, 2,  UC_MAPPERFECT,  2, 0, 0);
	M_AddRawCondition(30, 3,  UC_MAPPERFECT,  4, 0, 0);
	M_AddRawCondition(30, 4,  UC_MAPPERFECT,  5, 0, 0);
	M_AddRawCondition(30, 5,  UC_MAPPERFECT,  7, 0, 0);
	M_AddRawCondition(30, 6,  UC_MAPPERFECT,  8, 0, 0);
	M_AddRawCondition(30, 7,  UC_MAPPERFECT, 10, 0, 0);
	M_AddRawCondition(30, 8,  UC_MAPPERFECT, 11, 0, 0);
	M_AddRawCondition(30, 9,  UC_MAPPERFECT, 13, 0, 0);
	M_AddRawCondition(30, 10, UC_MAPPERFECT, 16, 0, 0);
	M_AddRawCondition(30, 11, UC_MAPPERFECT, 22, 0, 0);
	M_AddRawCondition(30, 12, UC_MAPPERFECT, 23, 0, 0);
	M_AddRawCondition(30, 13, UC_MAPPERFECT, 24, 0, 0);
	M_AddRawCondition(30, 14, UC_MAPPERFECT, 40, 0, 0);
	M_AddRawCondition(30, 15, UC_MAPPERFECT, 41, 0, 0);

	// --  40: Find 20 emblems
	M_AddRawCondition(40, 1, UC_TOTALEMBLEMS, 20, 0, 0);

	// --  41: Find 40 emblems
	M_AddRawCondition(41, 1, UC_TOTALEMBLEMS, 40, 0, 0);

	// --  42: Find 60 emblems
	M_AddRawCondition(42, 1, UC_TOTALEMBLEMS, 60, 0, 0);

	// --  43: Find 80 emblems
	M_AddRawCondition(43, 1, UC_TOTALEMBLEMS, 80, 0, 0);

	// --  44: Find 100 emblems
	M_AddRawCondition(44, 1, UC_TOTALEMBLEMS, 100, 0, 0);

	// --  45: Find 160 (all) emblems
	M_AddRawCondition(45, 1, UC_TOTALEMBLEMS, 160, 0, 0);

	// --  50: A rank all NiGHTS special stages
	M_AddRawCondition(50, 1, UC_NIGHTSGRADE, GRADE_A, 50, 0);
	M_AddRawCondition(50, 1, UC_NIGHTSGRADE, GRADE_A, 51, 0);
	M_AddRawCondition(50, 1, UC_NIGHTSGRADE, GRADE_A, 52, 0);
	M_AddRawCondition(50, 1, UC_NIGHTSGRADE, GRADE_A, 53, 0);
	M_AddRawCondition(50, 1, UC_NIGHTSGRADE, GRADE_A, 54, 0);
	M_AddRawCondition(50, 1, UC_NIGHTSGRADE, GRADE_A, 55, 0);
	M_AddRawCondition(50, 1, UC_NIGHTSGRADE, GRADE_A, 56, 0);
}

void M_AddRawCondition(UINT8 set, UINT8 id, conditiontype_t c, INT32 r, INT16 x1, INT16 x2)
{
	condition_t *cond;
	UINT32 num, wnum;

	I_Assert(set && set <= MAXCONDITIONSETS);

	wnum = conditionSets[set - 1].numconditions;
	num = ++conditionSets[set - 1].numconditions;

	conditionSets[set - 1].condition = Z_Realloc(conditionSets[set - 1].condition, sizeof(condition_t)*num, PU_STATIC, 0);

	cond = conditionSets[set - 1].condition;

	cond[wnum].id = id;
	cond[wnum].type = c;
	cond[wnum].requirement = r;
	cond[wnum].extrainfo1 = x1;
	cond[wnum].extrainfo2 = x2;
}

void M_ClearConditionSet(UINT8 set)
{
	if (conditionSets[set - 1].numconditions)
	{
		Z_Free(conditionSets[set - 1].condition);
		conditionSets[set - 1].condition = NULL;
		conditionSets[set - 1].numconditions = 0;
	}
	conditionSets[set - 1].achieved = false;
}

// Clear ALL secrets.
void M_ClearSecrets(void)
{
	INT32 i;

	memset(mapvisited, 0, sizeof(mapvisited));

	for (i = 0; i < MAXEMBLEMS; ++i)
		emblemlocations[i].collected = false;
	for (i = 0; i < MAXEXTRAEMBLEMS; ++i)
		extraemblems[i].collected = false;
	for (i = 0; i < MAXUNLOCKABLES; ++i)
		unlockables[i].unlocked = false;
	for (i = 0; i < MAXCONDITIONSETS; ++i)
		conditionSets[i].achieved = false;

	timesBeaten = timesBeatenWithEmeralds = timesBeatenUltimate = 0;

	// Re-unlock any always unlocked things
	M_SilentUpdateUnlockablesAndEmblems();
}

// ----------------------
// Condition set checking
// ----------------------
static UINT8 M_CheckCondition(condition_t *cn)
{
	switch (cn->type)
	{
		case UC_PLAYTIME: // Requires total playing time >= x
			return (totalplaytime >= (unsigned)cn->requirement);
		case UC_GAMECLEAR: // Requires game beaten >= x times
			return (timesBeaten >= (unsigned)cn->requirement);
		case UC_ALLEMERALDS: // Requires game beaten with all 7 emeralds >= x times
			return (timesBeatenWithEmeralds >= (unsigned)cn->requirement);
		case UC_ULTIMATECLEAR: // Requires game beaten on ultimate >= x times (in other words, never)
			return (timesBeatenUltimate >= (unsigned)cn->requirement);
		case UC_OVERALLSCORE: // Requires overall score >= x
			return (M_GotHighEnoughScore(cn->requirement));
		case UC_OVERALLTIME: // Requires overall time <= x
			return (M_GotLowEnoughTime(cn->requirement));
		case UC_OVERALLRINGS: // Requires overall rings >= x
			return (M_GotHighEnoughRings(cn->requirement));
		case UC_MAPVISITED: // Requires map x to be visited
			return ((mapvisited[cn->requirement - 1] & MV_VISITED) == MV_VISITED);
		case UC_MAPBEATEN: // Requires map x to be beaten
			return ((mapvisited[cn->requirement - 1] & MV_BEATEN) == MV_BEATEN);
		case UC_MAPALLEMERALDS: // Requires map x to be beaten with all emeralds in possession
			return ((mapvisited[cn->requirement - 1] & MV_ALLEMERALDS) == MV_ALLEMERALDS);
		case UC_MAPULTIMATE: // Requires map x to be beaten on ultimate
			return ((mapvisited[cn->requirement - 1] & MV_ULTIMATE) == MV_ULTIMATE);
		case UC_MAPPERFECT: // Requires map x to be beaten with a perfect bonus
			return ((mapvisited[cn->requirement - 1] & MV_PERFECT) == MV_PERFECT);
		case UC_MAPSCORE: // Requires score on map >= x
			return (G_GetBestScore(cn->extrainfo1) >= (unsigned)cn->requirement);
		case UC_MAPTIME: // Requires time on map <= x
			return (G_GetBestTime(cn->extrainfo1) <= (unsigned)cn->requirement);
		case UC_MAPRINGS: // Requires rings on map >= x
			return (G_GetBestRings(cn->extrainfo1) >= cn->requirement);
		case UC_NIGHTSSCORE:
			return (G_GetBestNightsScore(cn->extrainfo1, (UINT8)cn->extrainfo2) >= (unsigned)cn->requirement);
		case UC_NIGHTSTIME:
			return (G_GetBestNightsTime(cn->extrainfo1, (UINT8)cn->extrainfo2) <= (unsigned)cn->requirement);
		case UC_NIGHTSGRADE:
			return (G_GetBestNightsGrade(cn->extrainfo1, (UINT8)cn->extrainfo2) >= cn->requirement);
		case UC_TRIGGER: // requires map trigger set
			return !!(unlocktriggers & (1 << cn->requirement));
		case UC_TOTALEMBLEMS: // Requires number of emblems >= x
			return (M_GotEnoughEmblems(cn->requirement));
		case UC_EMBLEM: // Requires emblem x to be obtained
			return emblemlocations[cn->requirement-1].collected;
		case UC_EXTRAEMBLEM: // Requires extra emblem x to be obtained
			return extraemblems[cn->requirement-1].collected;
		case UC_CONDITIONSET: // requires condition set x to already be achieved
			return M_Achieved(cn->requirement-1);
	}
	return false;
}

static UINT8 M_CheckConditionSet(conditionset_t *c)
{
	UINT32 i;
	UINT32 lastID = 0;
	condition_t *cn;
	UINT8 achievedSoFar = true;

	for (i = 0; i < c->numconditions; ++i)
	{
		cn = &c->condition[i];

		// If the ID is changed and all previous statements of the same ID were true
		// then this condition has been successfully achieved
		if (lastID && lastID != cn->id && achievedSoFar)
			return true;

		// Skip future conditions with the same ID if one fails, for obvious reasons
		else if (lastID && lastID == cn->id && !achievedSoFar)
			continue;

		lastID = cn->id;
		achievedSoFar = M_CheckCondition(cn);
	}

	return achievedSoFar;
}

void M_CheckUnlockConditions(void)
{
	INT32 i;
	conditionset_t *c;

	for (i = 0; i < MAXCONDITIONSETS; ++i)
	{
		c = &conditionSets[i];
		if (!c->numconditions || c->achieved)
			continue;

		c->achieved = (M_CheckConditionSet(c));
	}
}

UINT8 M_UpdateUnlockablesAndExtraEmblems(void)
{
	INT32 i;
	char cechoText[992] = "";
	UINT8 cechoLines = 0;

	if (modifiedgame && !savemoddata)
		return false;

	M_CheckUnlockConditions();

	// Go through extra emblems
	for (i = 0; i < numextraemblems; ++i)
	{
		if (extraemblems[i].collected || !extraemblems[i].conditionset)
			continue;
		if ((extraemblems[i].collected = M_Achieved(extraemblems[i].conditionset - 1)) != false)
		{
			strcat(cechoText, va(M_GetText("Got \"%s\" emblem!\\"), extraemblems[i].name));
			++cechoLines;
		}
	}

	// Fun part: if any of those unlocked we need to go through the
	// unlock conditions AGAIN just in case an emblem reward was reached
	if (cechoLines)
		M_CheckUnlockConditions();

	// Go through unlockables
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].unlocked || !unlockables[i].conditionset)
			continue;
		if ((unlockables[i].unlocked = M_Achieved(unlockables[i].conditionset - 1)) != false)
		{
			if (unlockables[i].nocecho)
				continue;
			strcat(cechoText, va(M_GetText("\"%s\" unlocked!\\"), unlockables[i].name));
			++cechoLines;
		}
	}

	// Announce
	if (cechoLines)
	{
		char slashed[1024] = "";
		for (i = 0; (i < 21) && (i < 24 - cechoLines); ++i)
			slashed[i] = '\\';
		slashed[i] = 0;

		strcat(slashed, cechoText);

		HU_SetCEchoFlags(V_YELLOWMAP|V_RETURN8);
		HU_SetCEchoDuration(6);
		HU_DoCEcho(slashed);
		return true;
	}
	return false;
}

// Used when loading gamedata to make sure all unlocks are synched with conditions
void M_SilentUpdateUnlockablesAndEmblems(void)
{
	INT32 i;
	boolean checkAgain = false;

	// Just in case they aren't to sync
	M_CheckUnlockConditions();
	M_CheckLevelEmblems();

	// Go through extra emblems
	for (i = 0; i < numextraemblems; ++i)
	{
		if (extraemblems[i].collected || !extraemblems[i].conditionset)
			continue;
		if ((extraemblems[i].collected = M_Achieved(extraemblems[i].conditionset - 1)) != false)
			checkAgain = true;
	}

	// check again if extra emblems unlocked, blah blah, etc
	if (checkAgain)
		M_CheckUnlockConditions();

	// Go through unlockables
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].unlocked || !unlockables[i].conditionset)
			continue;
		unlockables[i].unlocked = M_Achieved(unlockables[i].conditionset - 1);
	}
}

// Emblem unlocking shit
UINT8 M_CheckLevelEmblems(void)
{
	INT32 i;
	INT32 valToReach;
	INT16 levelnum;
	UINT8 res;
	UINT8 somethingUnlocked = 0;

	// Update Score, Time, Rings emblems
	for (i = 0; i < numemblems; ++i)
	{
		if (emblemlocations[i].type <= ET_SKIN || emblemlocations[i].type == ET_MAP || emblemlocations[i].collected)
			continue;

		levelnum = emblemlocations[i].level;
		valToReach = emblemlocations[i].var;

		switch (emblemlocations[i].type)
		{
			case ET_SCORE: // Requires score on map >= x
				res = (G_GetBestScore(levelnum) >= (unsigned)valToReach);
				break;
			case ET_TIME: // Requires time on map <= x
				res = (G_GetBestTime(levelnum) <= (unsigned)valToReach);
				break;
			case ET_RINGS: // Requires rings on map >= x
				res = (G_GetBestRings(levelnum) >= valToReach);
				break;
			case ET_NGRADE: // Requires NiGHTS grade on map >= x
				res = (G_GetBestNightsGrade(levelnum, 0) >= valToReach);
				break;
			case ET_NTIME: // Requires NiGHTS time on map <= x
				res = (G_GetBestNightsTime(levelnum, 0) <= (unsigned)valToReach);
				break;
			default: // unreachable but shuts the compiler up.
				continue;
		}

		emblemlocations[i].collected = res;
		if (res)
			++somethingUnlocked;
	}
	return somethingUnlocked;
}

UINT8 M_CompletionEmblems(void) // Bah! Duplication sucks, but it's for a separate print when awarding emblems and it's sorta different enough.
{
	INT32 i;
	INT32 embtype;
	INT16 levelnum;
	UINT8 res;
	UINT8 somethingUnlocked = 0;
	UINT8 flags;

	for (i = 0; i < numemblems; ++i)
	{
		if (emblemlocations[i].type != ET_MAP || emblemlocations[i].collected)
			continue;

		levelnum = emblemlocations[i].level;
		embtype = emblemlocations[i].var;
		flags = MV_BEATEN;
		
		if (embtype & ME_ALLEMERALDS)
			flags |= MV_ALLEMERALDS;
		
		if (embtype & ME_ULTIMATE)
			flags |= MV_ULTIMATE;
		
		if (embtype & ME_PERFECT)
			flags |= MV_PERFECT;
		
		res = ((mapvisited[levelnum - 1] & flags) == flags);
		
		emblemlocations[i].collected = res;
		if (res)
			++somethingUnlocked;
	}
	return somethingUnlocked;
}

// -------------------
// Quick unlock checks
// -------------------
UINT8 M_AnySecretUnlocked(void)
{
	INT32 i;
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (!unlockables[i].nocecho && unlockables[i].unlocked)
			return true;
	}
	return false;
}

UINT8 M_SecretUnlocked(INT32 type)
{
	INT32 i;
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].type == type && unlockables[i].unlocked)
			return true;
	}
	return false;
}

UINT8 M_MapLocked(INT32 mapnum)
{
	if (!mapheaderinfo[mapnum-1] || mapheaderinfo[mapnum-1]->unlockrequired < 0)
		return false;
	if (!unlockables[mapheaderinfo[mapnum-1]->unlockrequired].unlocked)
		return true;
	return false;
}

INT32 M_CountEmblems(void)
{
	INT32 found = 0, i;
	for (i = 0; i < numemblems; ++i)
	{
		if (emblemlocations[i].collected)
			found++;
	}
	for (i = 0; i < numextraemblems; ++i)
	{
		if (extraemblems[i].collected)
			found++;
	}
	return found;
}

// --------------------------------------
// Quick functions for calculating things
// --------------------------------------

// Theoretically faster than using M_CountEmblems()
// Stops when it reaches the target number of emblems.
UINT8 M_GotEnoughEmblems(INT32 number)
{
	INT32 i, gottenemblems = 0;
	for (i = 0; i < numemblems; ++i)
	{
		if (emblemlocations[i].collected)
			if (++gottenemblems >= number) return true;
	}
	for (i = 0; i < numextraemblems; ++i)
	{
		if (extraemblems[i].collected)
			if (++gottenemblems >= number) return true;
	}
	return false;
}

UINT8 M_GotHighEnoughScore(INT32 tscore)
{
	INT32 mscore = 0;
	INT32 i;

	for (i = 0; i < NUMMAPS; ++i)
	{
		if (!mapheaderinfo[i] || !(mapheaderinfo[i]->menuflags & LF2_RECORDATTACK))
			continue;
		if (!mainrecords[i])
			continue;

		if ((mscore += mainrecords[i]->score) > tscore)
			return true;
	}
	return false;
}

UINT8 M_GotLowEnoughTime(INT32 tictime)
{
	INT32 curtics = 0;
	INT32 i;

	for (i = 0; i < NUMMAPS; ++i)
	{
		if (!mapheaderinfo[i] || !(mapheaderinfo[i]->menuflags & LF2_RECORDATTACK))
			continue;

		if (!mainrecords[i] || !mainrecords[i]->time)
			return false;
		else if ((curtics += mainrecords[i]->time) > tictime)
			return false;
	}
	return true;
}

UINT8 M_GotHighEnoughRings(INT32 trings)
{
	INT32 mrings = 0;
	INT32 i;

	for (i = 0; i < NUMMAPS; ++i)
	{
		if (!mapheaderinfo[i] || !(mapheaderinfo[i]->menuflags & LF2_RECORDATTACK))
			continue;
		if (!mainrecords[i])
			continue;

		if ((mrings += mainrecords[i]->rings) > trings)
			return true;
	}
	return false;
}

// ----------------
// Misc Emblem shit
// ----------------

// Returns pointer to an emblem if an emblem exists for that level.
// Pass -1 mapnum to continue from last emblem.
// NULL if not found.
// note that this goes in reverse!!
emblem_t *M_GetLevelEmblems(INT32 mapnum)
{
	static INT32 map = -1;
	static INT32 i = -1;

	if (mapnum > 0)
	{
		map = mapnum;
		i = numemblems;
	}

	while (--i >= 0)
	{
		if (emblemlocations[i].level == map)
			return &emblemlocations[i];
	}
	return NULL;
}

skincolors_t M_GetEmblemColor(emblem_t *em)
{
	if (!em || em->color >= MAXSKINCOLORS)
		return SKINCOLOR_NONE;
	return em->color;
}

const char *M_GetEmblemPatch(emblem_t *em)
{
	static char pnamebuf[7] = "GOTITn";

	I_Assert(em->sprite >= 'A' && em->sprite <= 'Z');
	pnamebuf[5] = em->sprite;
	return pnamebuf;
}

skincolors_t M_GetExtraEmblemColor(extraemblem_t *em)
{
	if (!em || em->color >= MAXSKINCOLORS)
		return SKINCOLOR_NONE;
	return em->color;
}

const char *M_GetExtraEmblemPatch(extraemblem_t *em)
{
	static char pnamebuf[7] = "GOTITn";

	I_Assert(em->sprite >= 'A' && em->sprite <= 'Z');
	pnamebuf[5] = em->sprite;
	return pnamebuf;
}
