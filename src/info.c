// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  info.c
/// \brief Thing frame/state LUT

// Data.
#include "doomdef.h"
#include "doomstat.h"
#include "sounds.h"
#include "p_mobj.h"
#include "p_local.h" // DMG_ constants
#include "m_misc.h"
#include "z_zone.h"
#include "d_player.h"
#include "v_video.h" // V_*MAP constants
#include "lzf.h"
#ifdef HWRENDER
#include "hardware/hw_light.h"
#endif

// Hey, moron! If you change this table, don't forget about the sprite enum in info.h and the sprite lights in hw_light.c!
// For the sake of constant merge conflicts, let's spread this out
char sprnames[NUMSPRITES + 1][5] =
{
	"NULL", // invisible object
	"UNKN",

	"THOK", // Thok! mobj
	"PLAY",

	// Enemies
	"POSS", // Crawla (Blue)
	"SPOS", // Crawla (Red)
	"FISH", // SDURF
	"BUZZ", // Buzz (Gold)
	"RBUZ", // Buzz (Red)
	"JETB", // Jetty-Syn Bomber
	"JETG", // Jetty-Syn Gunner
	"CCOM", // Crawla Commander
	"DETN", // Deton
	"SKIM", // Skim mine dropper
	"TRET", // Industrial Turret
	"TURR", // Pop-Up Turret
	"SHRP", // Sharp
	"CRAB", // Crushstacean
	"CR2B", // Banpyura
	"CSPR", // Banpyura spring
	"JJAW", // Jet Jaw
	"SNLR", // Snailer
	"VLTR", // BASH
	"PNTY", // Pointy
	"ARCH", // Robo-Hood
	"CBFS", // Castlebot Facestabber
	"STAB", // Castlebot Facestabber spear aura
	"SPSH", // Egg Guard
	"ESHI", // Egg Guard's shield
	"GSNP", // Green Snapper
	"GSNL", // Green Snapper leg
	"GSNH", // Green Snapper head
	"MNUS", // Minus
	"MNUD", // Minus dirt
	"SSHL", // Spring Shell
	"UNID", // Unidus
	"CANA", // Canarivore
	"CANG", // Canarivore gas
	"PYRE", // Pyre Fly
	"PTER", // Pterabyte
	"DRAB", // Dragonbomber

	// Generic Boss Items
	"JETF", // Boss jet fumes

	// Boss 1 (Greenflower)
	"EGGM", // Boss 1
	"EGLZ", // Boss 1 Junk

	// Boss 2 (Techno Hill)
	"EGGN", // Boss 2
	"TANK", // Boss 2 Junk
	"GOOP", // Boss 2 Goop

	// Boss 3 (Deep Sea)
	"EGGO", // Boss 3
	"SEBH", // Boss 3 Junk
	"FAKE", // Boss 3 Fakemobile
	"SHCK", // Boss 3 Shockwave

	// Boss 4 (Castle Eggman)
	"EGGP",
	"EFIR", // Boss 4 jet flame
	"EGR1", // Boss 4 Spectator Eggrobo

	// Boss 5 (Arid Canyon)
	"FANG", // replaces EGGQ
	"BRKN", // broken robot chunk
	"WHAT", // alart
	"VWRE",
	"PROJ", // projector light
	"FBOM",
	"FSGN",
	"BARX", // bomb explosion (also used by barrel)
	"BARD", // bomb dust (also used by barrel)

	// Boss 6 (Red Volcano)
	"EGGR",

	// Boss 7 (Dark City)
	"BRAK",
	"BGOO", // Goop
	"BMSL",

	// Boss 8 (Egg Rock)
	"EGGT",

	// Cy-Brak-Demon; uses "BRAK" as well, but has some extras
	"RCKT", // Rockets!
	"ELEC", // Electricity!
	"TARG", // Targeting reticules!
	"NPLM", // Big napalm bombs!
	"MNPL", // Mini napalm bombs!

	// Metal Sonic
	"METL",
	"MSCF",
	"MSCB",

	// Collectible Items
	"RING",
	"TRNG", // Team Rings
	"TOKE", // Special Stage Token
	"RFLG", // Red CTF Flag
	"BFLG", // Blue CTF Flag
	"SPHR", // Sphere
	"NCHP", // NiGHTS chip
	"NSTR", // NiGHTS star
	"EMBM", // Emblem
	"CEMG", // Chaos Emeralds
	"SHRD", // Emerald hunt shards

	// Interactive Objects
	"BBLS", // water bubble source
	"SIGN", // Level end sign
	"SPIK", // Spike Ball
	"SFLM", // Spin fire
	"USPK", // Floor spike
	"WSPK", // Wall spike
	"WSPB", // Wall spike base
	"STPT", // Starpost
	"BMNE", // Big floating mine
	"PUMI", // Rollout Rock

	// Monitor Boxes
	"MSTV", // MiSc TV sprites
	"XLTV", // eXtra Large TV sprites

	"TRRI", // Red team:  10 RIngs
	"TBRI", // Blue team: 10 RIngs

	"TVRI", // 10 RIng
	"TVPI", // PIty shield
	"TVAT", // ATtraction shield
	"TVFO", // FOrce shield
	"TVAR", // ARmageddon shield
	"TVWW", // WhirlWind shield
	"TVEL", // ELemental shield
	"TVSS", // Super Sneakers
	"TVIV", // InVincibility
	"TV1U", // 1Up
	"TV1P", // 1uP (textless)
	"TVEG", // EGgman
	"TVMX", // MiXup
	"TVMY", // MYstery
	"TVGV", // GraVity boots
	"TVRC", // ReCycler
	"TV1K", // 1,000 points  (1 K)
	"TVTK", // 10,000 points (Ten K)
	"TVFL", // FLame shield
	"TVBB", // BuBble shield
	"TVZP", // Thunder shield (ZaP)

	// Projectiles
	"MISL",
	"LASR", // GFZ3 laser
	"LASF", // GFZ3 laser flames
	"TORP", // Torpedo
	"ENRG", // Energy ball
	"MINE", // Skim mine
	"JBUL", // Jetty-Syn Bullet
	"TRLS",
	"CBLL", // Cannonball
	"AROW", // Arrow
	"CFIR", // Colored fire of various sorts

	// The letter
	"LETR",

	// Greenflower Scenery
	"FWR1",
	"FWR2", // GFZ Sunflower
	"FWR3", // GFZ budding flower
	"FWR4",
	"BUS1", // GFZ Bush w/ berries
	"BUS2", // GFZ Bush w/o berries
	"BUS3", // GFZ Bush w/ BLUE berries
	// Trees (both GFZ and misc)
	"TRE1", // GFZ
	"TRE2", // Checker
	"TRE3", // Frozen Hillside
	"TRE4", // Polygon
	"TRE5", // Bush tree
	"TRE6", // Spring tree

	// Techno Hill Scenery
	"THZP", // THZ1 Steam Flower
	"FWR5", // THZ1 Spin flower (red)
	"FWR6", // THZ1 Spin flower (yellow)
	"THZT", // Steam Whistle tree/bush
	"ALRM", // THZ2 Alarm

	// Deep Sea Scenery
	"GARG", // Deep Sea Gargoyle
	"SEWE", // Deep Sea Seaweed
	"DRIP", // Dripping water
	"CORL", // Coral
	"BCRY", // Blue Crystal
	"KELP", // Kelp
	"ALGA", // Animated algae top
	"ALGB", // Animated algae segment
	"DSTG", // DSZ Stalagmites
	"LIBE", // DSZ Light beam

	// Castle Eggman Scenery
	"CHAN", // CEZ Chain
	"FLAM", // Flame
	"ESTA", // Eggman esta una estatua!
	"SMCH", // Small Mace Chain
	"BMCH", // Big Mace Chain
	"SMCE", // Small Mace
	"BMCE", // Big Mace
	"YSPB", // Yellow spring on a ball
	"RSPB", // Red spring on a ball
	"SFBR", // Small Firebar
	"BFBR", // Big Firebar
	"BANR", // Banner
	"PINE", // Pine Tree
	"CEZB", // Bush
	"CNDL", // Candle/pricket
	"FLMH", // Flame holder
	"CTRC", // Fire torch
	"CFLG", // Waving flag/segment
	"CSTA", // Crawla statue
	"CBBS", // Facestabber statue
	"CABR", // Brambles

	// Arid Canyon Scenery
	"BTBL", // Big tumbleweed
	"STBL", // Small tumbleweed
	"CACT", // Cacti
	"WWSG", // Caution Sign
	"WWS2", // Cacti Sign
	"WWS3", // Sharp Turn Sign
	"OILL", // Oil lamp
	"OILF", // Oil lamp flare
	"BARR", // TNT barrel
	"REMT", // TNT proximity shell
	"TAZD", // Dust devil
	"ADST", // Arid dust
	"MCRT", // Minecart
	"MCSP", // Minecart spark
	"SALD", // Saloon door
	"TRAE", // Train cameo locomotive
	"TRAI", // Train cameo wagon
	"STEA", // Train steam

	// Red Volcano Scenery
	"FLME", // Flame jet
	"DFLM", // Blade's flame
	"LFAL", // Lavafall
	"JPLA", // Jungle palm
	"TFLO", // Torch flower
	"WVIN", // Wall vines

	// Dark City Scenery

	// Egg Rock Scenery

	// Christmas Scenery
	"XMS1", // Christmas Pole
	"XMS2", // Candy Cane
	"XMS3", // Snowman
	"XMS4", // Lamppost
	"XMS5", // Hanging Star
	"XMS6", // Mistletoe
	"FHZI", // FHZ ice
	"ROSY",

	// Halloween Scenery
	"PUMK", // Pumpkins
	"HHPL", // Dr Seuss Trees
	"SHRM", // Mushroom
	"HHZM", // Misc

	// Azure Temple Scenery
	"BGAR", // ATZ Gargoyles
	"RCRY", // ATZ Red Crystal (Target)
	"CFLM", // Green torch flame

	// Botanic Serenity Scenery
	"BSZ1", // Tall flowers
	"BSZ2", // Medium flowers
	"BSZ3", // Small flowers
	//"BSZ4", -- Tulips
	"BST1", // Red tulip
	"BST2", // Purple tulip
	"BST3", // Blue tulip
	"BST4", // Cyan tulip
	"BST5", // Yellow tulip
	"BST6", // Orange tulip
	"BSZ5", // Cluster of Tulips
	"BSZ6", // Bush
	"BSZ7", // Vine
	"BSZ8", // Misc things

	// Misc Scenery
	"STLG", // Stalagmites
	"DBAL", // Disco

	// Powerup Indicators
	"ARMA", // Armageddon Shield Orb
	"ARMF", // Armageddon Shield Ring, Front
	"ARMB", // Armageddon Shield Ring, Back
	"WIND", // Whirlwind Shield Orb
	"MAGN", // Attract Shield Orb
	"ELEM", // Elemental Shield Orb and Fire
	"FORC", // Force Shield Orb
	"PITY", // Pity Shield Orb
	"FIRS", // Flame Shield Orb
	"BUBS", // Bubble Shield Orb
	"ZAPS", // Thunder Shield Orb
	"IVSP", // invincibility sparkles
	"SSPK", // Super Sonic Spark

	"GOAL", // Special Stage goal (here because lol NiGHTS)

	// Flickies
	"FBUB", // Flicky-sized bubble
	"FL01", // Bluebird
	"FL02", // Rabbit
	"FL03", // Chicken
	"FL04", // Seal
	"FL05", // Pig
	"FL06", // Chipmunk
	"FL07", // Penguin
	"FL08", // Fish
	"FL09", // Ram
	"FL10", // Puffin
	"FL11", // Cow
	"FL12", // Rat
	"FL13", // Bear
	"FL14", // Dove
	"FL15", // Cat
	"FL16", // Canary
	"FS01", // Spider
	"FS02", // Bat

	// Springs
	"FANS", // Fan
	"STEM", // Steam riser
	"BUMP", // Bumpers
	"BLON", // Balloons
	"SPRY", // Yellow spring
	"SPRR", // Red spring
	"SPRB", // Blue spring
	"YSPR", // Yellow Diagonal Spring
	"RSPR", // Red Diagonal Spring
	"BSPR", // Blue Diagonal Spring
	"SSWY", // Yellow Side Spring
	"SSWR", // Red Side Spring
	"SSWB", // Blue Side Spring
	"BSTY", // Yellow Booster
	"BSTR", // Red Booster

	// Environmental Effects
	"RAIN", // Rain
	"SNO1", // Snowflake
	"SPLH", // Water Splish
	"LSPL", // Lava Splish
	"SPLA", // Water Splash
	"SMOK",
	"BUBL", // Bubble
	"WZAP",
	"DUST", // Spindash dust
	"FPRT", // Spindash dust (flame)
	"TFOG", // Teleport Fog
	"SEED", // Sonic CD flower seed
	"PRTL", // Particle (for fans, etc.)

	// Game Indicators
	"SCOR", // Score logo
	"DRWN", // Drowning Timer
	"FLII", // Flight indicator
	"LCKN", // Target
	"TTAG", // Tag Sign
	"GFLG", // Got Flag sign
	"FNSF", // Finish flag

	"CORK",
	"LHRT",

	// Ring Weapons
	"RRNG", // Red Ring
	"RNGB", // Bounce Ring
	"RNGR", // Rail Ring
	"RNGI", // Infinity Ring
	"RNGA", // Automatic Ring
	"RNGE", // Explosion Ring
	"RNGS", // Scatter Ring
	"RNGG", // Grenade Ring

	"PIKB", // Bounce Ring Pickup
	"PIKR", // Rail Ring Pickup
	"PIKA", // Automatic Ring Pickup
	"PIKE", // Explosion Ring Pickup
	"PIKS", // Scatter Ring Pickup
	"PIKG", // Grenade Ring Pickup

	"TAUT", // Thrown Automatic Ring
	"TGRE", // Thrown Grenade Ring
	"TSCR", // Thrown Scatter Ring

	// Mario-specific stuff
	"COIN",
	"CPRK",
	"GOOM",
	"BGOM",
	"FFWR",
	"FBLL",
	"SHLL",
	"PUMA",
	"HAMM",
	"KOOP",
	"BFLM",
	"MAXE",
	"MUS1",
	"MUS2",
	"TOAD",

	// NiGHTS Stuff
	"NDRN", // NiGHTS drone
	"NSPK", // NiGHTS sparkle
	"NBMP", // NiGHTS Bumper
	"HOOP", // NiGHTS hoop sprite
	"NSCR", // NiGHTS score sprite
	"NPRU", // Nights Powerups
	"CAPS", // Capsule thingy for NiGHTS
	"IDYA", // Ideya
	"NTPN", // Nightopian
	"SHLP", // Shleep

	// Secret badniks and hazards, shhhh
	"PENG",
	"POPH",
	"HIVE",
	"BUMB",
	"BBUZ",
	"FMCE",
	"HMCE",
	"CACO",
	"BAL2",
	"SBOB",
	"SBFL",
	"SBSK",
	"HBAT",

	// Debris
	"SPRK", // Sparkle
	"BOM1", // Robot Explosion
	"BOM2", // Boss Explosion 1
	"BOM3", // Boss Explosion 2
	"BOM4", // Underwater Explosion
	"BMNB", // Mine Explosion

	// Crumbly rocks
	"ROIA",
	"ROIB",
	"ROIC",
	"ROID",
	"ROIE",
	"ROIF",
	"ROIG",
	"ROIH",
	"ROII",
	"ROIJ",
	"ROIK",
	"ROIL",
	"ROIM",
	"ROIN",
	"ROIO",
	"ROIP",

	// Level debris
	"GFZD", // GFZ debris
	"BRIC", // Bricks
	"WDDB", // Wood Debris
	"BRIR", // CEZ3 colored bricks
	"BRIB", // CEZ3 colored bricks
	"BRIY", // CEZ3 colored bricks

	// Gravity Well Objects
	"GWLG",
	"GWLR",
};

char spr2names[NUMPLAYERSPRITES][5] =
{
	"STND",
	"WAIT",
	"WALK",
	"SKID",
	"RUN_",
	"DASH",
	"PAIN",
	"STUN",
	"DEAD",
	"DRWN",
	"ROLL",
	"GASP",
	"JUMP",
	"SPNG",
	"FALL",
	"EDGE",
	"RIDE",

	"SPIN",

	"FLY_",
	"SWIM",
	"TIRE",

	"GLID",
	"LAND",
	"CLNG",
	"CLMB",

	"FLT_",
	"FRUN",

	"BNCE",

	"FIRE",

	"TWIN",

	"MLEE",
	"MLEL",

	"TRNS",

	"NSTD",
	"NFLT",
	"NFLY",
	"NDRL",
	"NSTN",
	"NPUL",
	"NATK",

	"TAL0",
	"TAL1",
	"TAL2",
	"TAL3",
	"TAL4",
	"TAL5",
	"TAL6",
	"TAL7",
	"TAL8",
	"TAL9",
	"TALA",
	"TALB",

	"CNT1",
	"CNT2",
	"CNT3",
	"CNT4",

	"SIGN",
	"LIFE",

	"XTRA",
};
playersprite_t free_spr2 = SPR2_FIRSTFREESLOT;

playersprite_t spr2defaults[NUMPLAYERSPRITES] = {
	0, // SPR2_STND,
	0, // SPR2_WAIT,
	0, // SPR2_WALK,
	SPR2_WALK, // SPR2_SKID,
	SPR2_WALK, // SPR2_RUN ,
	SPR2_FRUN, // SPR2_DASH,
	0, // SPR2_PAIN,
	SPR2_PAIN, // SPR2_STUN,
	SPR2_PAIN, // SPR2_DEAD,
	SPR2_DEAD, // SPR2_DRWN,
	0, // SPR2_ROLL,
	SPR2_SPNG, // SPR2_GASP,
	0, // SPR2_JUMP, (conditional, will never be referenced)
	SPR2_FALL, // SPR2_SPNG,
	SPR2_WALK, // SPR2_FALL,
	0, // SPR2_EDGE,
	SPR2_FALL, // SPR2_RIDE,

	SPR2_ROLL, // SPR2_SPIN,

	SPR2_SPNG, // SPR2_FLY ,
	SPR2_FLY , // SPR2_SWIM,
	0, // SPR2_TIRE, (conditional, will never be referenced)

	SPR2_FLY , // SPR2_GLID,
	SPR2_ROLL, // SPR2_LAND,
	SPR2_CLMB, // SPR2_CLNG,
	SPR2_ROLL, // SPR2_CLMB,

	SPR2_WALK, // SPR2_FLT ,
	SPR2_RUN , // SPR2_FRUN,

	SPR2_FALL, // SPR2_BNCE,

	0, // SPR2_FIRE,

	SPR2_ROLL, // SPR2_TWIN,

	SPR2_TWIN, // SPR2_MLEE,
	0, // SPR2_MLEL,

	0, // SPR2_TRNS,

	FF_SPR2SUPER|SPR2_STND, // SPR2_NSTD,
	FF_SPR2SUPER|SPR2_FALL, // SPR2_NFLT,
	0, // SPR2_NFLY, (will never be referenced unless skin 0 lacks this)
	SPR2_NFLY, // SPR2_NDRL,
	FF_SPR2SUPER|SPR2_STUN, // SPR2_NSTN,
	SPR2_NSTN, // SPR2_NPUL,
	FF_SPR2SUPER|SPR2_ROLL, // SPR2_NATK,

	0, // SPR2_TAL0, (this will look mighty stupid but oh well)
	SPR2_TAL0, // SPR2_TAL1,
	SPR2_TAL1, // SPR2_TAL2,
	SPR2_TAL2, // SPR2_TAL3,
	SPR2_TAL1, // SPR2_TAL4,
	SPR2_TAL4, // SPR2_TAL5,
	SPR2_TAL0, // SPR2_TAL6,
	SPR2_TAL3, // SPR2_TAL7,
	SPR2_TAL7, // SPR2_TAL8,
	SPR2_TAL0, // SPR2_TAL9,
	SPR2_TAL9, // SPR2_TALA,
	SPR2_TAL0, // SPR2_TALB,

	SPR2_WAIT, // SPR2_CNT1,
	SPR2_FALL, // SPR2_CNT2,
	SPR2_SPNG, // SPR2_CNT3,
	SPR2_CNT1, // SPR2_CNT4,

	0, // SPR2_SIGN,
	0, // SPR2_LIFE,

	0, // SPR2_XTRA (should never be referenced)
};

// Doesn't work with g++, needs actionf_p1 (don't modify this comment)
state_t states[NUMSTATES] =
{
	// frame is masked through FF_FRAMEMASK
	// FF_ANIMATE makes simple state animations (var1 #frames, var2 tic delay)
	// FF_FULLBRIGHT activates the fullbright colormap
	// use FF_TRANS10 - FF_TRANS90 for easy translucency
	// (or tr_trans10<<FF_TRANSSHIFT if you want to make it hard on yourself)

	// Keep this comment directly above S_NULL.
	{SPR_NULL, 0,  1, {NULL}, 0, 0, S_NULL}, // S_NULL
	{SPR_UNKN, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_UNKNOWN
	{SPR_NULL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_INVISIBLE

	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 0, 0, S_NULL}, // S_SPAWNSTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 1, 0, S_NULL}, // S_SEESTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 2, 0, S_NULL}, // S_MELEESTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 3, 0, S_NULL}, // S_MISSILESTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 4, 0, S_NULL}, // S_DEATHSTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 5, 0, S_NULL}, // S_XDEATHSTATE
	{SPR_UNKN, FF_FULLBRIGHT, -1, {A_InfoState}, 6, 0, S_NULL}, // S_RAISESTATE

	// Thok
	{SPR_THOK, FF_TRANS50, 8, {NULL}, 0, 0, S_NULL}, // S_THOK

	// Player
	{SPR_PLAY, SPR2_STND|FF_ANIMATE,    105, {NULL}, 0,  7, S_PLAY_WAIT}, // S_PLAY_STND
	{SPR_PLAY, SPR2_WAIT|FF_ANIMATE,     -1, {NULL}, 0, 16, S_NULL},      // S_PLAY_WAIT
	{SPR_PLAY, SPR2_WALK,                 4, {NULL}, 0,  0, S_PLAY_WALK}, // S_PLAY_WALK
	{SPR_PLAY, SPR2_SKID,                 1, {NULL}, 0,  0, S_PLAY_WALK}, // S_PLAY_SKID
	{SPR_PLAY, SPR2_RUN ,                 2, {NULL}, 0,  0, S_PLAY_RUN},  // S_PLAY_RUN
	{SPR_PLAY, SPR2_DASH,                 2, {NULL}, 0,  0, S_PLAY_DASH}, // S_PLAY_DASH
	{SPR_PLAY, SPR2_PAIN|FF_ANIMATE,    350, {NULL}, 0,  4, S_PLAY_FALL}, // S_PLAY_PAIN
	{SPR_PLAY, SPR2_STUN|FF_ANIMATE,    350, {NULL}, 0,  4, S_PLAY_FALL}, // S_PLAY_STUN
	{SPR_PLAY, SPR2_DEAD|FF_ANIMATE,     -1, {NULL}, 0,  4, S_NULL},      // S_PLAY_DEAD
	{SPR_PLAY, SPR2_DRWN|FF_ANIMATE,     -1, {NULL}, 0,  4, S_NULL},      // S_PLAY_DRWN
	{SPR_PLAY, SPR2_ROLL,                 1, {NULL}, 0,  0, S_PLAY_ROLL}, // S_PLAY_ROLL
	{SPR_PLAY, SPR2_GASP|FF_ANIMATE,     14, {NULL}, 0,  4, S_PLAY_WALK}, // S_PLAY_GASP
	{SPR_PLAY, SPR2_JUMP,                 1, {NULL}, 0,  0, S_PLAY_JUMP}, // S_PLAY_JUMP
	{SPR_PLAY, SPR2_SPNG,                 2, {NULL}, 0,  0, S_PLAY_SPRING}, // S_PLAY_SPRING
	{SPR_PLAY, SPR2_FALL,                 2, {NULL}, 0,  0, S_PLAY_FALL}, // S_PLAY_FALL
	{SPR_PLAY, SPR2_EDGE,                12, {NULL}, 0,  0, S_PLAY_EDGE}, // S_PLAY_EDGE
	{SPR_PLAY, SPR2_RIDE,                 4, {NULL}, 0,  0, S_PLAY_RIDE}, // S_PLAY_RIDE

	// CA2_SPINDASH
	{SPR_PLAY, SPR2_SPIN,                 2, {NULL}, 0,  0, S_PLAY_SPINDASH}, // S_PLAY_SPINDASH

	// CA_FLY/CA_SWIM
	{SPR_PLAY, SPR2_FLY ,                 2, {NULL}, 0,  0, S_PLAY_FLY},  // S_PLAY_FLY
	{SPR_PLAY, SPR2_SWIM,                 4, {NULL}, 0,  0, S_PLAY_SWIM}, // S_PLAY_SWIM
	{SPR_PLAY, SPR2_TIRE,                12, {NULL}, 0,  0, S_PLAY_FLY_TIRED}, // S_PLAY_FLY_TIRED

	// CA_GLIDEANDCLIMB
	{SPR_PLAY, SPR2_GLID,                 2, {NULL}, 0,  0, S_PLAY_GLIDE}, // S_PLAY_GLIDE
	{SPR_PLAY, SPR2_LAND,                 7, {NULL}, 0,  0, S_PLAY_STND},  // S_PLAY_GLIDE_LANDING
	{SPR_PLAY, SPR2_CLNG|FF_ANIMATE,     -1, {NULL}, 0,  4, S_NULL},       // S_PLAY_CLING
	{SPR_PLAY, SPR2_CLMB,                 5, {NULL}, 0,  0, S_PLAY_CLIMB}, // S_PLAY_CLIMB

	// CA_FLOAT/CA_SLOWFALL
	{SPR_PLAY, SPR2_FLT ,                 7, {NULL}, 0,  0, S_PLAY_FLOAT}, // S_PLAY_FLOAT
	{SPR_PLAY, SPR2_FRUN,                 7, {NULL}, 0,  0, S_PLAY_FLOAT_RUN},  // S_PLAY_FLOAT_RUN

	// CA_BOUNCE
	{SPR_PLAY, SPR2_BNCE|FF_ANIMATE,     -1, {NULL},             0,  0, S_NULL},                // S_PLAY_BOUNCE
	{SPR_PLAY, SPR2_LAND|FF_SPR2ENDSTATE, 2, {NULL}, S_PLAY_BOUNCE,  0, S_PLAY_BOUNCE_LANDING}, // S_PLAY_BOUNCE_LANDING

	// CA2_GUNSLINGER
	{SPR_PLAY, SPR2_FIRE|FF_SPR2ENDSTATE,  2, {NULL}, S_PLAY_FIRE_FINISH, 0, S_PLAY_FIRE},   // S_PLAY_FIRE
	{SPR_PLAY, SPR2_FIRE,                 15, {NULL},        S_PLAY_STND, 0, S_PLAY_STND},   // S_PLAY_FIRE_FINISH

	// CA_TWINSPIN
	{SPR_PLAY, SPR2_TWIN|FF_SPR2ENDSTATE, 2, {NULL}, S_PLAY_JUMP, 0, S_PLAY_TWINSPIN}, // S_PLAY_TWINSPIN

	// CA2_MELEE
	{SPR_PLAY, SPR2_MLEE|FF_SPR2ENDSTATE, 2, {NULL}, S_PLAY_MELEE_FINISH, 0, S_PLAY_MELEE}, // S_PLAY_MELEE
	{SPR_PLAY, SPR2_MLEE,                70, {NULL},                   0, 0, S_PLAY_FALL},  // S_PLAY_MELEE_FINISH
	{SPR_PLAY, SPR2_MLEL,                35, {NULL},                   0, 0, S_PLAY_WALK},  // S_PLAY_MELEE_LANDING

	// SF_SUPER
	{SPR_PLAY, SPR2_TRNS|FF_SPR2SUPER|FF_ANIMATE,     7, {NULL},          0, 4, S_PLAY_SUPER_TRANS2}, // S_PLAY_SUPER_TRANS1
	{SPR_PLAY, SPR2_TRNS|FF_SPR2SUPER,                3, {NULL},          0, 0, S_PLAY_SUPER_TRANS3}, // S_PLAY_SUPER_TRANS2
	{SPR_PLAY, SPR2_TRNS|FF_SPR2SUPER|FF_FULLBRIGHT,  2, {NULL},          0, 0, S_PLAY_SUPER_TRANS4}, // S_PLAY_SUPER_TRANS3
	{SPR_PLAY, SPR2_TRNS|FF_SPR2SUPER|FF_FULLBRIGHT,  2, {NULL},          0, 0, S_PLAY_SUPER_TRANS5}, // S_PLAY_SUPER_TRANS4
	{SPR_PLAY, SPR2_TRNS|FF_SPR2SUPER|FF_FULLBRIGHT,  2, {NULL},          0, 0, S_PLAY_SUPER_TRANS6}, // S_PLAY_SUPER_TRANS5
	{SPR_PLAY, SPR2_TRNS|FF_SPR2SUPER|FF_FULLBRIGHT, 19, {A_FadeOverlay}, 0, 0, S_PLAY_FALL},         // S_PLAY_SUPER_TRANS6

	{SPR_NULL, 0, -1, {NULL}, 0, 0, S_OBJPLACE_DUMMY}, //S_OBJPLACE_DUMMY

	// 1-Up box sprites (uses player sprite)
	{SPR_PLAY, SPR2_LIFE,  2, {NULL}, 0, 18, S_PLAY_BOX2},  // S_PLAY_BOX1
	{SPR_NULL,         0,  1, {NULL}, 0,  0, S_PLAY_BOX1},  // S_PLAY_BOX2
	{SPR_PLAY, SPR2_LIFE,  4, {NULL}, 0,  4, S_PLAY_ICON2}, // S_PLAY_ICON1
	{SPR_NULL,         0, 12, {NULL}, 0,  0, S_PLAY_ICON3}, // S_PLAY_ICON2
	{SPR_PLAY, SPR2_LIFE, 20, {NULL}, 0,  4, S_NULL},       // S_PLAY_ICON3

	// Level end sign (uses player sprite)
	{SPR_PLAY, SPR2_SIGN|FF_PAPERSPRITE, 2, {NULL}, 0, 29, S_PLAY_SIGN},         // S_PLAY_SIGN

	// NiGHTS Player, transforming
	{SPR_PLAY, SPR2_TRNS|FF_ANIMATE,     7, {NULL},          0, 4, S_PLAY_NIGHTS_TRANS2}, // S_PLAY_NIGHTS_TRANS1
	{SPR_PLAY, SPR2_TRNS,                3, {NULL},          0, 0, S_PLAY_NIGHTS_TRANS3}, // S_PLAY_NIGHTS_TRANS2
	{SPR_PLAY, SPR2_TRNS|FF_FULLBRIGHT,  2, {NULL},          0, 0, S_PLAY_NIGHTS_TRANS4}, // S_PLAY_NIGHTS_TRANS3
	{SPR_PLAY, SPR2_TRNS|FF_FULLBRIGHT,  2, {NULL},          0, 0, S_PLAY_NIGHTS_TRANS5}, // S_PLAY_NIGHTS_TRANS4
	{SPR_PLAY, SPR2_TRNS|FF_FULLBRIGHT,  2, {NULL},          0, 0, S_PLAY_NIGHTS_TRANS6}, // S_PLAY_NIGHTS_TRANS5
	{SPR_PLAY, SPR2_TRNS|FF_FULLBRIGHT, 21, {A_FadeOverlay}, 2, 0, S_PLAY_NIGHTS_FLOAT},  // S_PLAY_NIGHTS_TRANS5

	// NiGHTS Player
	{SPR_PLAY, SPR2_NSTD, 7, {NULL}, 0, 0, S_PLAY_NIGHTS_STAND},  // S_PLAY_NIGHTS_STAND
	{SPR_PLAY, SPR2_NFLT, 7, {NULL}, 0, 0, S_PLAY_NIGHTS_FLOAT},  // S_PLAY_NIGHTS_FLOAT
	{SPR_PLAY, SPR2_NFLY, 2, {NULL}, 0, 0, S_PLAY_NIGHTS_FLY},    // S_PLAY_NIGHTS_FLY
	{SPR_PLAY, SPR2_NDRL, 2, {NULL}, 0, 0, S_PLAY_NIGHTS_DRILL},  // S_PLAY_NIGHTS_DRILL
	{SPR_PLAY, SPR2_NSTN, 2, {NULL}, 0, 0, S_PLAY_NIGHTS_STUN},   // S_PLAY_NIGHTS_STUN
	{SPR_PLAY, SPR2_NPUL, 1, {NULL}, 0, 0, S_PLAY_NIGHTS_PULL},   // S_PLAY_NIGHTS_PULL
	{SPR_PLAY, SPR2_NATK, 1, {NULL}, 0, 0, S_PLAY_NIGHTS_ATTACK}, // S_PLAY_NIGHTS_ATTACK

	// c:
	{SPR_PLAY, SPR2_TAL0|FF_SPR2MIDSTART,  5, {NULL}, 0, 0, S_TAILSOVERLAY_STAND}, // S_TAILSOVERLAY_STAND
	{SPR_PLAY, SPR2_TAL1|FF_SPR2MIDSTART, 35, {NULL}, 0, 0, S_TAILSOVERLAY_0DEGREES}, // S_TAILSOVERLAY_0DEGREES
	{SPR_PLAY, SPR2_TAL2|FF_SPR2MIDSTART, 35, {NULL}, 0, 0, S_TAILSOVERLAY_PLUS30DEGREES}, // S_TAILSOVERLAY_PLUS30DEGREES
	{SPR_PLAY, SPR2_TAL3|FF_SPR2MIDSTART, 35, {NULL}, 0, 0, S_TAILSOVERLAY_PLUS60DEGREES}, // S_TAILSOVERLAY_PLUS60DEGREES
	{SPR_PLAY, SPR2_TAL4|FF_SPR2MIDSTART, 35, {NULL}, 0, 0, S_TAILSOVERLAY_MINUS30DEGREES}, // S_TAILSOVERLAY_MINUS30DEGREES
	{SPR_PLAY, SPR2_TAL5|FF_SPR2MIDSTART, 35, {NULL}, 0, 0, S_TAILSOVERLAY_MINUS60DEGREES}, // S_TAILSOVERLAY_MINUS60DEGREES
	{SPR_PLAY, SPR2_TAL6|FF_SPR2MIDSTART, 35, {NULL}, 0, 0, S_TAILSOVERLAY_RUN}, // S_TAILSOVERLAY_RUN
	{SPR_PLAY, SPR2_TAL7|FF_SPR2MIDSTART,  4, {NULL}, 0, 0, S_TAILSOVERLAY_FLY}, // S_TAILSOVERLAY_FLY
	{SPR_PLAY, SPR2_TAL8|FF_SPR2MIDSTART,  4, {NULL}, 0, 0, S_TAILSOVERLAY_TIRE}, // S_TAILSOVERLAY_TIRE
	{SPR_PLAY, SPR2_TAL9|FF_SPR2MIDSTART, 35, {NULL}, 0, 0, S_TAILSOVERLAY_PAIN}, // S_TAILSOVERLAY_PAIN
	{SPR_PLAY, SPR2_TALA|FF_SPR2MIDSTART, 35, {NULL}, 0, 0, S_TAILSOVERLAY_GASP}, // S_TAILSOVERLAY_GASP
	{SPR_PLAY, SPR2_TALB                , 35, {NULL}, 0, 0, S_TAILSOVERLAY_EDGE}, // S_TAILSOVERLAY_EDGE

	// [:
	{SPR_JETF, 3|FF_ANIMATE|FF_FULLBRIGHT, 2, {NULL}, 1, 1, S_JETFUME1}, // S_JETFUMEFLASH

	// Blue Crawla
	{SPR_POSS, 0, 5, {A_Look}, 0, 0, S_POSS_STND},   // S_POSS_STND
	{SPR_POSS, 0, 3, {A_Chase}, 0, 0, S_POSS_RUN2},   // S_POSS_RUN1
	{SPR_POSS, 1, 3, {A_Chase}, 0, 0, S_POSS_RUN3},   // S_POSS_RUN2
	{SPR_POSS, 2, 3, {A_Chase}, 0, 0, S_POSS_RUN4},   // S_POSS_RUN3
	{SPR_POSS, 3, 3, {A_Chase}, 0, 0, S_POSS_RUN5},   // S_POSS_RUN4
	{SPR_POSS, 4, 3, {A_Chase}, 0, 0, S_POSS_RUN6},   // S_POSS_RUN5
	{SPR_POSS, 5, 3, {A_Chase}, 0, 0, S_POSS_RUN1},   // S_POSS_RUN6

	// Red Crawla
	{SPR_SPOS, 0, 5, {A_Look}, 0, 0, S_SPOS_STND},   // S_SPOS_STND
	{SPR_SPOS, 0, 1, {A_Chase}, 0, 0, S_SPOS_RUN2},   // S_SPOS_RUN1
	{SPR_SPOS, 1, 1, {A_Chase}, 0, 0, S_SPOS_RUN3},   // S_SPOS_RUN2
	{SPR_SPOS, 2, 1, {A_Chase}, 0, 0, S_SPOS_RUN4},   // S_SPOS_RUN3
	{SPR_SPOS, 3, 1, {A_Chase}, 0, 0, S_SPOS_RUN5},   // S_SPOS_RUN4
	{SPR_SPOS, 4, 1, {A_Chase}, 0, 0, S_SPOS_RUN6},   // S_SPOS_RUN5
	{SPR_SPOS, 5, 1, {A_Chase}, 0, 0, S_SPOS_RUN1},   // S_SPOS_RUN6

	// Greenflower Fish
	{SPR_FISH, 1, 1, {NULL}, 0, 0, S_FISH2},         // S_FISH1
	{SPR_FISH, 1, 1, {A_FishJump}, 0, 0, S_FISH1},   // S_FISH2
	{SPR_FISH, 0, 1, {NULL}, 0, 0, S_FISH4},         // S_FISH3
	{SPR_FISH, 0, 1, {A_FishJump}, 0, 0, S_FISH3},   // S_FISH4

	// Gold Buzz
	{SPR_BUZZ, 0, 2, {A_Look}, 0, 0, S_BUZZLOOK2},   // S_BUZZLOOK1
	{SPR_BUZZ, 1, 2, {A_Look}, 0, 0, S_BUZZLOOK1},   // S_BUZZLOOK2
	{SPR_BUZZ, 0, 2, {A_BuzzFly}, sfx_buzz4, 0, S_BUZZFLY2}, // S_BUZZFLY1
	{SPR_BUZZ, 1, 2, {A_BuzzFly}, 0, 0, S_BUZZFLY1}, // S_BUZZFLY2

	// Red Buzz
	{SPR_RBUZ, 0, 2, {A_Look}, 0, 0, S_RBUZZLOOK2},   // S_RBUZZLOOK1
	{SPR_RBUZ, 1, 2, {A_Look}, 0, 0, S_RBUZZLOOK1},   // S_RBUZZLOOK2
	{SPR_RBUZ, 0, 2, {A_BuzzFly}, sfx_buzz4, 0, S_RBUZZFLY2}, // S_RBUZZFLY1
	{SPR_RBUZ, 1, 2, {A_BuzzFly}, 0, 0, S_RBUZZFLY1}, // S_RBUZZFLY2

	// Jetty-Syn Bomber
	{SPR_JETB, 0, 4, {A_Look}, 0, 0, S_JETBLOOK2},      // S_JETBLOOK1
	{SPR_JETB, 1, 4, {A_Look}, 0, 0, S_JETBLOOK1},      // S_JETBLOOK2
	{SPR_JETB, 0, 1, {A_JetbThink}, 0, 0, S_JETBZOOM2}, // S_JETBZOOM1
	{SPR_JETB, 1, 1, {A_JetbThink}, 0, 0, S_JETBZOOM1}, // S_JETBZOOM2

	// Jetty-Syn Gunner
	{SPR_JETG, 0, 4, {A_Look}, 0, 0, S_JETGLOOK2},       // S_JETGLOOK1
	{SPR_JETG, 1, 4, {A_Look}, 0, 0, S_JETGLOOK1},       // S_JETGLOOK2
	{SPR_JETG, 0, 1, {A_JetgThink}, 0, 0, S_JETGZOOM2},  // S_JETGZOOM1
	{SPR_JETG, 1, 1, {A_JetgThink}, 0, 0, S_JETGZOOM1},  // S_JETGZOOM2
	{SPR_JETG, 2, 1, {A_JetgShoot}, 0, 0, S_JETGSHOOT2}, // S_JETGSHOOT1
	{SPR_JETG, 3, 1, {NULL}, 0, 0, S_JETGZOOM1},         // S_JETGSHOOT2

	// Crawla Commander
	{SPR_CCOM, 0, 1, {A_CrawlaCommanderThink}, 0, 15*FRACUNIT, S_CCOMMAND2}, // S_CCOMMAND1
	{SPR_CCOM, 1, 1, {A_CrawlaCommanderThink}, 0, 15*FRACUNIT, S_CCOMMAND1}, // S_CCOMMAND2
	{SPR_CCOM, 2, 1, {A_CrawlaCommanderThink}, 0, 15*FRACUNIT, S_CCOMMAND4}, // S_CCOMMAND3
	{SPR_CCOM, 3, 1, {A_CrawlaCommanderThink}, 0, 15*FRACUNIT, S_CCOMMAND3}, // S_CCOMMAND4

	// Deton
	{SPR_DETN, 0, 35, {A_Look}, 0, 0, S_DETON1},       // S_DETON1
	{SPR_DETN, 0,  1, {A_DetonChase}, 0, 0, S_DETON3},  // S_DETON2
	{SPR_DETN, 1,  1, {A_DetonChase}, 0, 0, S_DETON4},  // S_DETON3
	{SPR_DETN, 2,  1, {A_DetonChase}, 0, 0, S_DETON5},  // S_DETON4
	{SPR_DETN, 3,  1, {A_DetonChase}, 0, 0, S_DETON6},  // S_DETON5
	{SPR_DETN, 4,  1, {A_DetonChase}, 0, 0, S_DETON7},  // S_DETON6
	{SPR_DETN, 5,  1, {A_DetonChase}, 0, 0, S_DETON8},  // S_DETON7
	{SPR_DETN, 6,  1, {A_DetonChase}, 0, 0, S_DETON9},  // S_DETON8
	{SPR_DETN, 7,  1, {A_DetonChase}, 0, 0, S_DETON10}, // S_DETON9
	{SPR_DETN, 6,  1, {A_DetonChase}, 0, 0, S_DETON11}, // S_DETON10
	{SPR_DETN, 5,  1, {A_DetonChase}, 0, 0, S_DETON12}, // S_DETON11
	{SPR_DETN, 4,  1, {A_DetonChase}, 0, 0, S_DETON13}, // S_DETON12
	{SPR_DETN, 3,  1, {A_DetonChase}, 0, 0, S_DETON14}, // S_DETON13
	{SPR_DETN, 2,  1, {A_DetonChase}, 0, 0, S_DETON15}, // S_DETON14
	{SPR_DETN, 1,  1, {A_DetonChase}, 0, 0, S_DETON2},  // S_DETON15

	// Skim Mine Dropper
	{SPR_SKIM, 0,  1, {A_SkimChase}, 0, 0, S_SKIM2},    // S_SKIM1
	{SPR_SKIM, 0,  1, {A_SkimChase}, 0, 0, S_SKIM1},    // S_SKIM2
	{SPR_SKIM, 0, 14,        {NULL}, 0, 0, S_SKIM4},    // S_SKIM3
	{SPR_SKIM, 0, 14,  {A_DropMine}, 0, 0, S_SKIM1},    // S_SKIM4

	// THZ Turret
	{SPR_TRET, FF_FULLBRIGHT, 105, {A_TurretStop}, 0, 0, S_TURRETFIRE},   // S_TURRET
	{SPR_TRET, FF_FULLBRIGHT, 105, {A_TurretFire}, 0, 0, S_TURRET},       // S_TURRETFIRE
	{SPR_TRET, FF_FULLBRIGHT|1, 7, {A_Pain}, 0, 0, S_TURRETSHOCK2},       // S_TURRETSHOCK1
	{SPR_TRET, FF_FULLBRIGHT|2, 7, {NULL}, 0, 0, S_TURRETSHOCK3},         // S_TURRETSHOCK2
	{SPR_TRET, FF_FULLBRIGHT|3, 7, {NULL}, 0, 0, S_TURRETSHOCK4},         // S_TURRETSHOCK3
	{SPR_TRET, FF_FULLBRIGHT|4, 7, {NULL}, 0, 0, S_TURRETSHOCK5},         // S_TURRETSHOCK4
	{SPR_TRET, FF_FULLBRIGHT|1, 7, {NULL}, 0, 0, S_TURRETSHOCK6},         // S_TURRETSHOCK5
	{SPR_TRET, FF_FULLBRIGHT|2, 7, {A_Pain}, 0, 0, S_TURRETSHOCK7},       // S_TURRETSHOCK6
	{SPR_TRET, FF_FULLBRIGHT|3, 7, {NULL}, 0, 0, S_TURRETSHOCK8},         // S_TURRETSHOCK7
	{SPR_TRET, FF_FULLBRIGHT|4, 7, {NULL}, 0, 0, S_TURRETSHOCK9},         // S_TURRETSHOCK8
	{SPR_TRET, FF_FULLBRIGHT|4, 7, {A_LinedefExecute}, LE_TURRET, 0, S_XPLD1}, // S_TURRETSHOCK9

	{SPR_TURR, 0, 1, {A_Look}, 1, 0, S_TURRETPOPDOWN8},          // S_TURRETLOOK
	{SPR_TURR, 0, 0, {A_FaceTarget}, 0, 0, S_TURRETPOPUP1},  // S_TURRETSEE
	{SPR_TURR, 1, 2, {A_Pain}, 0, 0, S_TURRETPOPUP2},  // S_TURRETPOPUP1
	{SPR_TURR, 2, 2, {NULL}, 0, 0, S_TURRETPOPUP3},  // S_TURRETPOPUP2
	{SPR_TURR, 3, 2, {NULL}, 0, 0, S_TURRETPOPUP4},  // S_TURRETPOPUP3
	{SPR_TURR, 4, 2, {NULL}, 0, 0, S_TURRETPOPUP5},  // S_TURRETPOPUP4
	{SPR_TURR, 5, 2, {NULL}, 0, 0, S_TURRETPOPUP6},  // S_TURRETPOPUP5
	{SPR_TURR, 6, 2, {NULL}, 0, 0, S_TURRETPOPUP7},  // S_TURRETPOPUP6
	{SPR_TURR, 7, 2, {NULL}, 0, 0, S_TURRETPOPUP8},  // S_TURRETPOPUP7
	{SPR_TURR, 8, 14,{NULL}, 0, 0, S_TURRETSHOOT},   // S_TURRETPOPUP8
	{SPR_TURR, 8, 14,{A_JetgShoot}, 0, 0, S_TURRETPOPDOWN1}, // S_TURRETSHOOT
	{SPR_TURR, 7, 2, {A_Pain}, 0, 0, S_TURRETPOPDOWN2},        // S_TURRETPOPDOWN1
	{SPR_TURR, 6, 2, {NULL}, 0, 0, S_TURRETPOPDOWN3},        // S_TURRETPOPDOWN2
	{SPR_TURR, 5, 2, {NULL}, 0, 0, S_TURRETPOPDOWN4},        // S_TURRETPOPDOWN3
	{SPR_TURR, 4, 2, {NULL}, 0, 0, S_TURRETPOPDOWN5},        // S_TURRETPOPDOWN4
	{SPR_TURR, 3, 2, {NULL}, 0, 0, S_TURRETPOPDOWN6},        // S_TURRETPOPDOWN5
	{SPR_TURR, 2, 2, {NULL}, 0, 0, S_TURRETPOPDOWN7},        // S_TURRETPOPDOWN6
	{SPR_TURR, 1, 2, {NULL}, 0, 0, S_TURRETPOPDOWN8},        // S_TURRETPOPDOWN7
	{SPR_TURR, 0, 69,{A_SetTics}, 0, 1, S_TURRETLOOK},       // S_TURRETPOPDOWN8

	// Spincushion
	{SPR_SHRP, 0,  2, {A_Look},                 0, 0, S_SPINCUSHION_LOOK},   // S_SPINCUSHION_LOOK
	{SPR_SHRP, 1,  2, {A_SharpChase},           0, 0, S_SPINCUSHION_CHASE2}, // S_SPINCUSHION_CHASE1
	{SPR_SHRP, 2,  2, {A_SharpChase},           0, 0, S_SPINCUSHION_CHASE3}, // S_SPINCUSHION_CHASE2
	{SPR_SHRP, 3,  2, {A_SharpChase},           0, 0, S_SPINCUSHION_CHASE4}, // S_SPINCUSHION_CHASE3
	{SPR_SHRP, 0,  2, {A_SharpChase},           0, 0, S_SPINCUSHION_CHASE1}, // S_SPINCUSHION_CHASE4
	{SPR_SHRP, 0,  2, {NULL},                   0, 0, S_SPINCUSHION_AIM2},   // S_SPINCUSHION_AIM1
	{SPR_SHRP, 4,  2, {NULL},                   0, 0, S_SPINCUSHION_AIM3},   // S_SPINCUSHION_AIM2
	{SPR_SHRP, 5,  2, {A_SetObjectFlags}, MF_PAIN, 2, S_SPINCUSHION_AIM4},   // S_SPINCUSHION_AIM3
	{SPR_SHRP, 6, 16, {A_MultiShotDist}, (MT_DUST<<16)|6, -32, S_SPINCUSHION_AIM5}, // S_SPINCUSHION_AIM4
	{SPR_SHRP, 6,  0, {A_PlaySound},   sfx_shrpgo, 1, S_SPINCUSHION_SPIN1},  // S_SPINCUSHION_AIM5
	{SPR_SHRP, 6,  1, {A_SharpSpin},            0, 0, S_SPINCUSHION_SPIN2},  // S_SPINCUSHION_SPIN1
	{SPR_SHRP, 8,  1, {A_SharpSpin},            0, 0, S_SPINCUSHION_SPIN3},  // S_SPINCUSHION_SPIN2
	{SPR_SHRP, 7,  1, {A_SharpSpin},            0, 0, S_SPINCUSHION_SPIN4},  // S_SPINCUSHION_SPIN3
	{SPR_SHRP, 8,  1, {A_SharpSpin},  MT_SPINDUST, 0, S_SPINCUSHION_SPIN1},  // S_SPINCUSHION_SPIN4
	{SPR_SHRP, 6,  1, {A_PlaySound},    sfx_s3k69, 1, S_SPINCUSHION_STOP2},  // S_SPINCUSHION_STOP1
	{SPR_SHRP, 6,  4, {A_SharpDecel},           0, 0, S_SPINCUSHION_STOP2},  // S_SPINCUSHION_STOP2
	{SPR_SHRP, 5,  4, {A_FaceTarget},           0, 0, S_SPINCUSHION_STOP4},  // S_SPINCUSHION_STOP3
	{SPR_SHRP, 4,  4, {A_SetObjectFlags}, MF_PAIN, 1, S_SPINCUSHION_LOOK},   // S_SPINCUSHION_STOP4

	// Crushstacean
	{SPR_CRAB, 0,  3, {A_CrushstaceanWalk},  0, S_CRUSHSTACEAN_ROAMPAUSE, S_CRUSHSTACEAN_ROAM2},     // S_CRUSHSTACEAN_ROAM1
	{SPR_CRAB, 1,  3, {A_CrushstaceanWalk},  0, S_CRUSHSTACEAN_ROAMPAUSE, S_CRUSHSTACEAN_ROAM3},     // S_CRUSHSTACEAN_ROAM2
	{SPR_CRAB, 0,  3, {A_CrushstaceanWalk},  0, S_CRUSHSTACEAN_ROAMPAUSE, S_CRUSHSTACEAN_ROAM4},     // S_CRUSHSTACEAN_ROAM3
	{SPR_CRAB, 2,  3, {A_CrushstaceanWalk},  0, S_CRUSHSTACEAN_ROAMPAUSE, S_CRUSHSTACEAN_ROAM1},     // S_CRUSHSTACEAN_ROAM4
	{SPR_CRAB, 0, 40, {NULL},                0,                        0, S_CRUSHSTACEAN_ROAM1},     // S_CRUSHSTACEAN_ROAMPAUSE
	{SPR_CRAB, 0, 10, {NULL},                0,                        0, S_CRUSHSTACEAN_PUNCH2},    // S_CRUSHSTACEAN_PUNCH1
	{SPR_CRAB, 0, -1, {A_CrushstaceanPunch}, 0,                        0, S_CRUSHSTACEAN_ROAMPAUSE}, // S_CRUSHSTACEAN_PUNCH2
	{SPR_CRAB, 3,  1, {A_CrushclawAim},   40,               20, S_CRUSHCLAW_AIM}, // S_CRUSHCLAW_AIM
	{SPR_CRAB, 3,  1, {A_CrushclawLaunch}, 0, S_CRUSHCLAW_STAY, S_CRUSHCLAW_OUT}, // S_CRUSHCLAW_OUT
	{SPR_CRAB, 3, 10, {NULL},              0,                0, S_CRUSHCLAW_IN},  // S_CRUSHCLAW_STAY
	{SPR_CRAB, 3,  1, {A_CrushclawLaunch}, 1, S_CRUSHCLAW_WAIT, S_CRUSHCLAW_IN},  // S_CRUSHCLAW_IN
	{SPR_CRAB, 3, 37, {NULL},              0,                0, S_CRUSHCLAW_AIM}, // S_CRUSHCLAW_WAIT
	{SPR_CRAB, 4, -1, {NULL}, 0, 0, S_NULL}, // S_CRUSHCHAIN

	// Banpyura
	{SPR_CR2B, 0,  3, {A_CrushstaceanWalk},  0, S_BANPYURA_ROAMPAUSE, S_BANPYURA_ROAM2}, // S_BANPYURA_ROAM1
	{SPR_CR2B, 1,  3, {A_CrushstaceanWalk},  0, S_BANPYURA_ROAMPAUSE, S_BANPYURA_ROAM3}, // S_BANPYURA_ROAM2
	{SPR_CR2B, 0,  3, {A_CrushstaceanWalk},  0, S_BANPYURA_ROAMPAUSE, S_BANPYURA_ROAM4}, // S_BANPYURA_ROAM3
	{SPR_CR2B, 2,  3, {A_CrushstaceanWalk},  0, S_BANPYURA_ROAMPAUSE, S_BANPYURA_ROAM1}, // S_BANPYURA_ROAM4
	{SPR_CR2B, 0, 40, {NULL},                0,                    0, S_BANPYURA_ROAM1}, // S_BANPYURA_ROAMPAUSE

	{SPR_CSPR, 0, 1, {A_CrushclawAim}, 50, 20, S_CDIAG1}, // S_CDIAG1
	{SPR_CSPR, 1, 1, {A_Pain},          0,  0, S_CDIAG3}, // S_CDIAG2
	{SPR_CSPR, 2, 1, {A_CrushclawAim}, 50, 20, S_CDIAG4}, // S_CDIAG3
	{SPR_CSPR, 3, 1, {A_CrushclawAim}, 50, 20, S_CDIAG5}, // S_CDIAG4
	{SPR_CSPR, 4, 1, {A_CrushclawAim}, 50, 20, S_CDIAG6}, // S_CDIAG5
	{SPR_CSPR, 3, 1, {A_CrushclawAim}, 50, 20, S_CDIAG7}, // S_CDIAG6
	{SPR_CSPR, 2, 1, {A_CrushclawAim}, 50, 20, S_CDIAG8}, // S_CDIAG7
	{SPR_CSPR, 1, 1, {A_CrushclawAim}, 50, 20, S_CDIAG1}, // S_CDIAG8

	// Jet Jaw
	{SPR_JJAW, 0, 1, {A_JetJawRoam},               0,               0,   S_JETJAW_ROAM2}, // S_JETJAW_ROAM1
	{SPR_JJAW, 0, 1, {A_JetJawRoam},               0,               0,   S_JETJAW_ROAM3}, // S_JETJAW_ROAM2
	{SPR_JJAW, 0, 1, {A_JetJawRoam},               0,               0,   S_JETJAW_ROAM4}, // S_JETJAW_ROAM3
	{SPR_JJAW, 0, 1, {A_JetJawRoam},               0,               0,   S_JETJAW_ROAM5}, // S_JETJAW_ROAM4
	{SPR_JJAW, 1, 1, {A_JetJawRoam},               0,               0,   S_JETJAW_ROAM6}, // S_JETJAW_ROAM5
	{SPR_JJAW, 1, 1, {A_JetJawRoam},               0,               0,   S_JETJAW_ROAM7}, // S_JETJAW_ROAM6
	{SPR_JJAW, 1, 1, {A_JetJawRoam},               0,               0,   S_JETJAW_ROAM8}, // S_JETJAW_ROAM7
	{SPR_JJAW, 1, 1, {A_JetJawRoam},               0,               0,   S_JETJAW_ROAM1}, // S_JETJAW_ROAM8
	{SPR_JJAW, 0, 1, {A_DualAction}, S_JETJAW_CHOMP16, S_JETJAW_SOUND,  S_JETJAW_CHOMP2}, // S_JETJAW_CHOMP1
	{SPR_JJAW, 0, 1, {A_JetJawChomp},              0,               0,  S_JETJAW_CHOMP3}, // S_JETJAW_CHOMP2
	{SPR_JJAW, 0, 1, {A_JetJawChomp},              0,               0,  S_JETJAW_CHOMP4}, // S_JETJAW_CHOMP3
	{SPR_JJAW, 0, 1, {A_JetJawChomp},              0,               0,  S_JETJAW_CHOMP5}, // S_JETJAW_CHOMP4
	{SPR_JJAW, 1, 1, {A_JetJawChomp},              0,               0,  S_JETJAW_CHOMP6}, // S_JETJAW_CHOMP5
	{SPR_JJAW, 1, 1, {A_JetJawChomp},              0,               0,  S_JETJAW_CHOMP7}, // S_JETJAW_CHOMP6
	{SPR_JJAW, 1, 1, {A_JetJawChomp},              0,               0,  S_JETJAW_CHOMP8}, // S_JETJAW_CHOMP7
	{SPR_JJAW, 1, 1, {A_JetJawChomp},              0,               0,  S_JETJAW_CHOMP9}, // S_JETJAW_CHOMP8
	{SPR_JJAW, 2, 1, {A_JetJawChomp},              0,               0, S_JETJAW_CHOMP10}, // S_JETJAW_CHOMP9
	{SPR_JJAW, 2, 1, {A_JetJawChomp},              0,               0, S_JETJAW_CHOMP11}, // S_JETJAW_CHOMP10
	{SPR_JJAW, 2, 1, {A_JetJawChomp},              0,               0, S_JETJAW_CHOMP12}, // S_JETJAW_CHOMP11
	{SPR_JJAW, 2, 1, {A_JetJawChomp},              0,               0, S_JETJAW_CHOMP13}, // S_JETJAW_CHOMP12
	{SPR_JJAW, 3, 1, {A_JetJawChomp},              0,               0, S_JETJAW_CHOMP14}, // S_JETJAW_CHOMP13
	{SPR_JJAW, 3, 1, {A_JetJawChomp},              0,               0, S_JETJAW_CHOMP15}, // S_JETJAW_CHOMP14
	{SPR_JJAW, 3, 1, {A_JetJawChomp},              0,               0, S_JETJAW_CHOMP16}, // S_JETJAW_CHOMP15
	{SPR_JJAW, 3, 1, {A_JetJawChomp},              0,               0,  S_JETJAW_CHOMP1}, // S_JETJAW_CHOMP16
	{SPR_JJAW, 0, 1, {A_PlayAttackSound},          0,               0,   S_JETJAW_SOUND}, // S_JETJAW_SOUND

	// Snailer
	{SPR_SNLR, 0, 1, {A_SnailerThink}, 0, 0, S_SNAILER1}, // S_SNAILER1
	{SPR_BOM1, 0, 0, {A_FlickySpawn}, 1<<17, 0, S_XPLD1}, // S_SNAILER_FLICKY

	// Vulture
	{SPR_VLTR, 4, 35,        {A_Look},         1, 0, S_VULTURE_STND},  // S_VULTURE_STND
	{SPR_VLTR, 4, 3,         {A_VultureHover}, 0, 0, S_VULTURE_DRIFT}, // S_VULTURE_DRIFT
	{SPR_VLTR, 0, 6,         {A_VultureBlast}, 0, 0, S_VULTURE_ZOOM2}, // S_VULTURE_ZOOM1
	{SPR_VLTR, 0, 3,         {A_VultureFly},   0, 0, S_VULTURE_ZOOM2}, // S_VULTURE_ZOOM2
	{SPR_VLTR, 0, 3*TICRATE, {NULL},           0, 0, S_VULTURE_DRIFT}, // S_VULTURE_STUNNED

	// Pointy
	{SPR_PNTY, 0,  1, {A_PointyThink}, 0, 0, S_POINTY1}, // S_POINTY1

	// Pointy Ball
	{SPR_PNTY, 1,  1, {A_CheckBuddy}, 0, 0, S_POINTYBALL1}, // S_POINTYBALL1

	// Robo-Hood
	{SPR_ARCH, 0,       4,            {A_Look}, 2048<<FRACBITS,   0, S_ROBOHOOD_LOOK},  // S_ROBOHOOD_LOOK
	{SPR_ARCH, 0,       1,       {A_HoodThink},              0,   0, S_ROBOHOOD_STAND}, // S_ROBOHOOD_STAND
	{SPR_ARCH, 2, TICRATE, {A_PlayActiveSound},              0,   0, S_ROBOHOOD_FIRE2}, // S_ROBOHOOD_FIRE1
	{SPR_ARCH, 2,      20,        {A_HoodFire},       MT_ARROW,   0, S_ROBOHOOD_STAND}, // S_ROBOHOOD_FIRE2
	{SPR_ARCH, 1,       1,      {A_FaceTarget},              0,   0, S_ROBOHOOD_JUMP2}, // S_ROBOHOOD_JUMP1
	{SPR_ARCH, 1,       1,        {A_BunnyHop},              4, -10, S_ROBOHOOD_JUMP3}, // S_ROBOHOOD_JUMP2
	{SPR_ARCH, 1,       1,        {A_HoodFall},              0,   0, S_ROBOHOOD_JUMP3}, // S_ROBOHOOD_JUMP3

	// Castlebot Facestabber
	{SPR_CBFS, 0,  1,        {A_Chase},  0, 0, S_FACESTABBER_STND2},   // S_FACESTABBER_STND1
	{SPR_CBFS, 1,  1,        {A_Chase},  0, 0, S_FACESTABBER_STND3},   // S_FACESTABBER_STND2
	{SPR_CBFS, 2,  1,        {A_Chase},  0, 0, S_FACESTABBER_STND4},   // S_FACESTABBER_STND3
	{SPR_CBFS, 3,  1,        {A_Chase},  0, 0, S_FACESTABBER_STND5},   // S_FACESTABBER_STND4
	{SPR_CBFS, 4,  1,        {A_Chase},  0, 0, S_FACESTABBER_STND6},   // S_FACESTABBER_STND5
	{SPR_CBFS, 5,  1,        {A_Chase},  0, 0, S_FACESTABBER_STND1},   // S_FACESTABBER_STND6
	{SPR_CBFS, 0,  1,  {A_FaceStabRev},                  20, S_FACESTABBER_CHARGE2, S_FACESTABBER_CHARGE1}, // S_FACESTABBER_CHARGE1
	{SPR_CBFS, 0,  0,   {A_FaceTarget},                   0,                     0, S_FACESTABBER_CHARGE3}, // S_FACESTABBER_CHARGE2
	{SPR_CBFS, 7,  1, {A_FaceStabHurl},                   6, S_FACESTABBER_CHARGE4, S_FACESTABBER_CHARGE3}, // S_FACESTABBER_CHARGE3
	{SPR_CBFS, 7,  1, {A_FaceStabMiss}, 0,   S_FACESTABBER_STND1, S_FACESTABBER_CHARGE4}, // S_FACESTABBER_CHARGE4
	{SPR_CBFS, 0, 35,         {A_Pain}, 0,                     0, S_FACESTABBER_STND1}, // S_FACESTABBER_PAIN
	{SPR_CBFS, 0,  2,   {A_BossScream}, 1, 0, S_FACESTABBER_DIE2},  // S_FACESTABBER_DIE1
	{SPR_NULL, 0,  2,   {A_BossScream}, 1, 0, S_FACESTABBER_DIE3},  // S_FACESTABBER_DIE2
	{SPR_NULL, 0,  0,       {A_Repeat}, 7, S_FACESTABBER_DIE1,    S_XPLD_FLICKY},       // S_FACESTABBER_DIE3

	{SPR_STAB, FF_PAPERSPRITE|FF_TRANS50|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_FACESTABBERSPEAR

	// Egg Guard
	{SPR_SPSH,  0,  1,       {A_Look}, 0, 0, S_EGGGUARD_STND},  // S_EGGGUARD_STND
	{SPR_SPSH,  1,  3, {A_GuardChase}, 0, 0, S_EGGGUARD_WALK2}, // S_EGGGUARD_WALK1
	{SPR_SPSH,  2,  3, {A_GuardChase}, 0, 0, S_EGGGUARD_WALK3}, // S_EGGGUARD_WALK2
	{SPR_SPSH,  3,  3, {A_GuardChase}, 0, 0, S_EGGGUARD_WALK4}, // S_EGGGUARD_WALK3
	{SPR_SPSH,  4,  3, {A_GuardChase}, 0, 0, S_EGGGUARD_WALK1}, // S_EGGGUARD_WALK4
	{SPR_SPSH,  5,  5,         {NULL}, 0, 0, S_EGGGUARD_MAD2},  // S_EGGGUARD_MAD1
	{SPR_SPSH,  6,  5,         {NULL}, 0, 0, S_EGGGUARD_MAD3},  // S_EGGGUARD_MAD2
	{SPR_SPSH,  7, 15,         {NULL}, 0, 0, S_EGGGUARD_RUN1},  // S_EGGGUARD_MAD3
	{SPR_SPSH,  8,  1, {A_GuardChase}, 0, 0, S_EGGGUARD_RUN2},  // S_EGGGUARD_RUN1
	{SPR_SPSH,  9,  1, {A_GuardChase}, 0, 0, S_EGGGUARD_RUN3},  // S_EGGGUARD_RUN2
	{SPR_SPSH, 10,  1, {A_GuardChase}, 0, 0, S_EGGGUARD_RUN4},  // S_EGGGUARD_RUN3
	{SPR_SPSH, 11,  1, {A_GuardChase}, 0, 0, S_EGGGUARD_RUN1},  // S_EGGGUARD_RUN4

	{SPR_ESHI, 0, 8, {A_EggShield}, 0, 0, S_EGGSHIELD},  // S_EGGSHIELD
	{SPR_ESHI, 0, TICRATE/2, {NULL}, 0, 0, S_NULL}, // S_EGGSHIELDBREAK

	// Green Snapper
	{SPR_GSNP, 0, TICRATE, {NULL},             0,              0,               S_SNAPPER_SPAWN2}, // S_SNAPPER_SPAWN
	{SPR_GSNP, 0, 2,       {A_SnapperSpawn},   MT_SNAPPER_LEG, MT_SNAPPER_HEAD, S_GSNAPPER_STND},  // S_SNAPPER_SPAWN2
	{SPR_GSNP, 0, 1,       {A_SnapperThinker}, 0,              0,               S_GSNAPPER_STND},  // S_GSNAPPER_STND
	{SPR_GSNP, 0, 2,       {A_Chase},          0,              0,               S_GSNAPPER2},      // S_GSNAPPER1
	{SPR_GSNP, 1, 2,       {A_Chase},          0,              0,               S_GSNAPPER3},      // S_GSNAPPER2
	{SPR_GSNP, 2, 2,       {A_Chase},          0,              0,               S_GSNAPPER4},      // S_GSNAPPER3
	{SPR_GSNP, 3, 2,       {A_Chase},          0,              0,               S_GSNAPPER1},      // S_GSNAPPER4
	{SPR_GSNP, 0, 0,       {A_KillSegments},   0,              0,               S_XPLD_FLICKY},    // S_SNAPPER_XPLD
	{SPR_GSNL, 0, -1,      {NULL},             0,              0,               S_NULL},           // S_SNAPPER_LEG
	{SPR_GSNL, 1, -1,      {NULL},             0,              0,               S_NULL},           // S_SNAPPER_LEGRAISE
	{SPR_GSNH, 0, -1,      {NULL},             0,              0,               S_NULL},           // S_SNAPPER_HEAD

	// Minus
	{SPR_MNUD, 0,            1,  {NULL},           0, 0, S_MINUS_STND},     // S_MINUS_INIT (required for objectplace to work)
	{SPR_NULL, 0,            10, {A_Look},         0, 0, S_MINUS_STND},     // S_MINUS_STND
	{SPR_NULL, 0,            1,  {A_MinusDigging}, 1, 0, S_MINUS_DIGGING2}, // S_MINUS_DIGGING1
	{SPR_NULL, 0,            1,  {A_MinusDigging}, 0, 0, S_MINUS_DIGGING3}, // S_MINUS_DIGGING2
	{SPR_NULL, 0,            1,  {A_MinusDigging}, 0, 0, S_MINUS_DIGGING4}, // S_MINUS_DIGGING3
	{SPR_NULL, 0,            1,  {A_MinusDigging}, 0, 0, S_MINUS_DIGGING1}, // S_MINUS_DIGGING4
	{SPR_NULL, 0,            25, {NULL},           0, 0, S_MINUS_POPUP},    // S_MINUS_BURST0
	{SPR_MNUD, FF_ANIMATE,   5,  {NULL},           1, 2, S_MINUS_BURST2},   // S_MINUS_BURST1
	{SPR_MNUD, 1|FF_ANIMATE, 5,  {NULL},           1, 2, S_MINUS_BURST3},   // S_MINUS_BURST2
	{SPR_MNUD, 2|FF_ANIMATE, 5,  {NULL},           1, 2, S_MINUS_BURST4},   // S_MINUS_BURST3
	{SPR_MNUD, 3|FF_ANIMATE, 5,  {NULL},           1, 2, S_MINUS_BURST5},   // S_MINUS_BURST4
	{SPR_MNUD, 4|FF_ANIMATE, 5,  {NULL},           1, 2, S_MINUSDIRT2},     // S_MINUS_BURST5
	{SPR_MNUS, 3, 1, {A_MinusPopup}, 0, 0, S_MINUS_AERIAL1}, // S_MINUS_POPUP
	{SPR_MNUS, 0, 1, {A_MinusCheck}, 0, 1, S_MINUS_AERIAL2},   // S_MINUS_AERIAL1
	{SPR_MNUS, 1, 1, {A_MinusCheck}, 0, 1, S_MINUS_AERIAL3},   // S_MINUS_AERIAL2
	{SPR_MNUS, 2, 1, {A_MinusCheck}, 0, 1, S_MINUS_AERIAL4},   // S_MINUS_AERIAL3
	{SPR_MNUS, 3, 1, {A_MinusCheck}, 0, 1, S_MINUS_AERIAL1},   // S_MINUS_AERIAL4

	{SPR_MNUD, FF_ANIMATE, 6, {NULL}, 1, 5, S_MINUSDIRT2}, // S_MINUSDIRT1
	{SPR_MNUD, 5,          8, {NULL}, 3, 5, S_MINUSDIRT3}, // S_MINUSDIRT2
	{SPR_MNUD, 4,          8, {NULL}, 3, 5, S_MINUSDIRT4}, // S_MINUSDIRT3
	{SPR_MNUD, 3,          8, {NULL}, 3, 5, S_MINUSDIRT5}, // S_MINUSDIRT4
	{SPR_MNUD, 2,          8, {NULL}, 3, 5, S_MINUSDIRT6}, // S_MINUSDIRT5
	{SPR_MNUD, 1,          8, {NULL}, 3, 5, S_MINUSDIRT7}, // S_MINUSDIRT6
	{SPR_MNUD, 0,          8, {NULL}, 3, 5, S_NULL},       // S_MINUSDIRT7

	// Spring Shell
	{SPR_SSHL,  0,  4, {A_Look},  0, 0, S_SSHELL_STND},    // S_SSHELL_STND
	{SPR_SSHL,  0,  4, {A_Chase}, 0, 0, S_SSHELL_RUN2},    // S_SSHELL_RUN1
	{SPR_SSHL,  1,  4, {A_Chase}, 0, 0, S_SSHELL_RUN3},    // S_SSHELL_RUN2
	{SPR_SSHL,  2,  4, {A_Chase}, 0, 0, S_SSHELL_RUN4},    // S_SSHELL_RUN3
	{SPR_SSHL,  3,  4, {A_Chase}, 0, 0, S_SSHELL_RUN1},    // S_SSHELL_RUN4
	{SPR_SSHL,  7,  4, {A_Pain},  0, 0, S_SSHELL_SPRING2}, // S_SSHELL_SPRING1
	{SPR_SSHL,  6,  1, {NULL},    0, 0, S_SSHELL_SPRING3}, // S_SSHELL_SPRING2
	{SPR_SSHL,  5,  1, {NULL},    0, 0, S_SSHELL_SPRING4}, // S_SSHELL_SPRING3
	{SPR_SSHL,  4,  1, {NULL},    0, 0, S_SSHELL_RUN1},    // S_SSHELL_SPRING4

	// Spring Shell (yellow)
	{SPR_SSHL,  8,  4, {A_Look},  0, 0, S_YSHELL_STND},    // S_YSHELL_STND
	{SPR_SSHL,  8,  4, {A_Chase}, 0, 0, S_YSHELL_RUN2},    // S_YSHELL_RUN1
	{SPR_SSHL,  9,  4, {A_Chase}, 0, 0, S_YSHELL_RUN3},    // S_YSHELL_RUN2
	{SPR_SSHL, 10,  4, {A_Chase}, 0, 0, S_YSHELL_RUN4},    // S_YSHELL_RUN3
	{SPR_SSHL, 11,  4, {A_Chase}, 0, 0, S_YSHELL_RUN1},    // S_YSHELL_RUN4
	{SPR_SSHL, 15,  4, {A_Pain},  0, 0, S_YSHELL_SPRING2}, // S_YSHELL_SPRING1
	{SPR_SSHL, 14,  1, {NULL},    0, 0, S_YSHELL_SPRING3}, // S_YSHELL_SPRING2
	{SPR_SSHL, 13,  1, {NULL},    0, 0, S_YSHELL_SPRING4}, // S_YSHELL_SPRING3
	{SPR_SSHL, 12,  1, {NULL},    0, 0, S_YSHELL_RUN1},    // S_YSHELL_SPRING4

	// Unidus
	{SPR_UNID, 0, 4, {A_Look},       0, 0, S_UNIDUS_STND}, // S_UNIDUS_STND
	{SPR_UNID, 0, 1, {A_Chase},      0, 0, S_UNIDUS_RUN }, // S_UNIDUS_RUN
	{SPR_UNID, 1, 1, {A_UnidusBall}, 1, 0, S_UNIDUS_BALL}, // S_UNIDUS_BALL

	// Canarivore
	{SPR_CANA, 0, 5,  {A_Look},          1200*FRACUNIT+1,   1,                 S_CANARIVORE_LOOK},      // S_CANARIVORE_LOOK
	{SPR_CANA, 0, 3,  {A_PlaySound},     sfx_s3k76,         1,                 S_CANARIVORE_AWAKEN2},   // S_CANARIVORE_AWAKEN1
	{SPR_CANA, 1, 5,  {NULL},            0,                 0,                 S_CANARIVORE_AWAKEN3},   // S_CANARIVORE_AWAKEN2
	{SPR_CANA, 2, 8,  {NULL},            0,                 0,                 S_CANARIVORE_GAS1},      // S_CANARIVORE_AWAKEN3
	{SPR_CANA, 2, 15, {A_PlaySound},     sfx_s3k93,         1,                 S_CANARIVORE_GAS2},      // S_CANARIVORE_GAS1
	{SPR_CANA, 1, 4,  {NULL},            0,                 0,                 S_CANARIVORE_GAS3},      // S_CANARIVORE_GAS2
	{SPR_CANA, 2, 0,  {A_PlaySound},     sfx_s3k97,         1,                 S_CANARIVORE_GAS4},      // S_CANARIVORE_GAS3
	{SPR_CANA, 2, 5,  {A_CanarivoreGas}, MT_CANARIVORE_GAS, 0,                 S_CANARIVORE_GAS5},      // S_CANARIVORE_GAS4
	{SPR_CANA, 1, 5,  {NULL},            0,                 0,                 S_CANARIVORE_GASREPEAT}, // S_CANARIVORE_GAS5
	{SPR_CANA, 2, 0,  {A_Repeat},        6,                 S_CANARIVORE_GAS4, S_CANARIVORE_CLOSE1},    // S_CANARIVORE_GASREPEAT
	{SPR_CANA, 1, 8,  {NULL},            0,                 0,                 S_CANARIVORE_CLOSE2},     // S_CANARIVORE_CLOSE1
	{SPR_CANA, 0, 90, {NULL},            sfx_s3k5d,         1,                 S_CANARIVORE_LOOK},      // S_CANARIVORE_CLOSE2

	{SPR_CANG, 0|FF_TRANS90, 2,         {NULL},            0, 0,       S_CANARIVOREGAS_2}, // S_CANARIVOREGAS_1
	{SPR_CANG, 0|FF_TRANS30, 2*TICRATE, {A_SetRandomTics}, 2, TICRATE, S_CANARIVOREGAS_3}, // S_CANARIVOREGAS_2
	{SPR_CANG, 0|FF_TRANS40, 10,        {NULL},            0, 0,       S_CANARIVOREGAS_4}, // S_CANARIVOREGAS_3
	{SPR_CANG, 0|FF_TRANS50, 10,        {NULL},            0, 0,       S_CANARIVOREGAS_5}, // S_CANARIVOREGAS_4
	{SPR_CANG, 0|FF_TRANS60, 10,        {NULL},            0, 0,       S_CANARIVOREGAS_6}, // S_CANARIVOREGAS_5
	{SPR_CANG, 0|FF_TRANS70, 10,        {NULL},            0, 0,       S_CANARIVOREGAS_7}, // S_CANARIVOREGAS_6
	{SPR_CANG, 0|FF_TRANS80, 10,        {NULL},            0, 0,       S_CANARIVOREGAS_8}, // S_CANARIVOREGAS_7
	{SPR_CANG, 0|FF_TRANS90, 10,        {NULL},            0, 0,       S_NULL},            // S_CANARIVOREGAS_8

	// Pyre Fly
	{SPR_PYRE, FF_ANIMATE, -1, {NULL}, 3, 2, S_NULL}, // S_PYREFLY_FLY
	{SPR_PYRE, 4|FF_ANIMATE|FF_FULLBRIGHT, -1, {NULL}, 3, 2, S_NULL}, // S_PYREFLY_BURN
	{SPR_FLAM, FF_FULLBRIGHT, 10, {NULL}, 0, 0, S_PYREFIRE2}, // S_PYREFIRE1
	{SPR_FLAM, 1|FF_FULLBRIGHT, 10, {A_FireShrink}, 0, 16, S_NULL}, // S_PYREFIRE2

	// Pterabyte
	{SPR_NULL, 0, -1, {A_SpawnPterabytes}, 0, 0, S_PTERABYTESPAWNER},    // S_PTERABYTESPAWNER
	{SPR_NULL, 0,  1, {A_PterabyteHover},  0, 0, S_PTERABYTEWAYPOINT},   // S_PTERABYTEWAYPOINT
	{SPR_PTER, 0,  6, {NULL},              0, 0, S_PTERABYTE_FLY2},      // S_PTERABYTE_FLY1
	{SPR_PTER, 1,  2, {NULL},              0, 0, S_PTERABYTE_FLY3},      // S_PTERABYTE_FLY2
	{SPR_PTER, 2,  6, {NULL},              0, 0, S_PTERABYTE_FLY4},      // S_PTERABYTE_FLY3
	{SPR_PTER, 3,  2, {NULL},              0, 0, S_PTERABYTE_FLY1},      // S_PTERABYTE_FLY4
	{SPR_PTER, 4,  1, {NULL},              0, 0, S_PTERABYTE_SWOOPDOWN}, // S_PTERABYTE_SWOOPDOWN
	{SPR_PTER, 0,  1, {NULL},              0, 0, S_PTERABYTE_SWOOPUP},   // S_PTERABYTE_SWOOPUP

	// Dragonbomber
	{SPR_DRAB,                0, -1, {A_DragonbomberSpawn}, 6, 0, S_NULL},                          // S_DRAGONBOMBER
	{SPR_DRAB, FF_PAPERSPRITE|7,  1,        {A_DragonWing}, 0, 0, S_DRAGONWING2},                   // S_DRAGONWING1
	{SPR_DRAB, FF_PAPERSPRITE|8,  1,        {A_DragonWing}, 0, 0, S_DRAGONWING3},                   // S_DRAGONWING2
	{SPR_DRAB, FF_PAPERSPRITE|9,  1,        {A_DragonWing}, 0, 0, S_DRAGONWING4},                   // S_DRAGONWING3
	{SPR_DRAB, FF_PAPERSPRITE|10, 1,        {A_DragonWing}, 0, 0, S_DRAGONWING1},                   // S_DRAGONWING4
	{SPR_DRAB,                1,  1,     {A_DragonSegment}, 0, 0, S_DRAGONTAIL_LOADED},             // S_DRAGONTAIL_LOADED
	{SPR_DRAB,                2,  1,     {A_DragonSegment}, 0, 0, S_DRAGONTAIL_EMPTYLOOP},          // S_DRAGONTAIL_EMPTY
	{SPR_DRAB,                2,  0,            {A_Repeat}, 3*TICRATE, S_DRAGONTAIL_EMPTY, S_DRAGONTAIL_RELOAD}, // S_DRAGONTAIL_EMPTYLOOP
	{SPR_DRAB,                1,  0,   {A_PlayActiveSound}, 0, 0, S_DRAGONTAIL_LOADED},             // S_DRAGONTAIL_RELOAD
	{SPR_DRAB,                3,  1,        {A_MinusCheck}, S_DRAGONMINE_LAND1, 0, S_DRAGONMINE},   // S_DRAGONMINE
	{SPR_DRAB,                4,  0,   {A_PlayActiveSound}, 0, 0, S_DRAGONMINE_LAND2},              // S_DRAGONMINE_LAND1
	{SPR_DRAB,                4,  2,            {A_Thrust}, 0, 1, S_DRAGONMINE_SLOWFLASH1},         // S_DRAGONMINE_LAND2
	{SPR_DRAB,                5, 11,                {NULL}, 0, 0, S_DRAGONMINE_SLOWFLASH2},         // S_DRAGONMINE_SLOWFLASH1
	{SPR_DRAB,  FF_FULLBRIGHT|6,  1,   {A_PlayAttackSound}, 0, 0, S_DRAGONMINE_SLOWLOOP},           // S_DRAGONMINE_SLOWFLASH2
	{SPR_DRAB,                5,  0,            {A_Repeat}, 4, S_DRAGONMINE_SLOWFLASH1, S_DRAGONMINE_FASTFLASH1}, // S_DRAGONMINE_SLOWLOOP
	{SPR_DRAB,                5,  3,                {NULL}, 0, 0, S_DRAGONMINE_FASTFLASH2},         // S_DRAGONMINE_FASTFLASH1
	{SPR_DRAB,  FF_FULLBRIGHT|6,  1,   {A_PlayAttackSound}, 0, 0, S_DRAGONMINE_FASTLOOP},           // S_DRAGONMINE_FASTFLASH2
	{SPR_DRAB,                5,  0,            {A_Repeat}, 5, S_DRAGONMINE_FASTFLASH1, S_DEATHSTATE}, // S_DRAGONMINE_FASTLOOP

	// Boss Explosion
	{SPR_BOM2, FF_FULLBRIGHT|FF_ANIMATE, (5*7), {NULL}, 6, 5, S_NULL}, // S_BOSSEXPLODE

	// S3&K Boss Explosion
	{SPR_BOM3, FF_FULLBRIGHT,   1, {NULL}, 0, 0, S_SONIC3KBOSSEXPLOSION2}, // S_SONIC3KBOSSEXPLOSION1
	{SPR_BOM3, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_SONIC3KBOSSEXPLOSION3}, // S_SONIC3KBOSSEXPLOSION2
	{SPR_BOM3, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_SONIC3KBOSSEXPLOSION4}, // S_SONIC3KBOSSEXPLOSION3
	{SPR_BOM3, FF_FULLBRIGHT|3, 2, {NULL}, 0, 0, S_SONIC3KBOSSEXPLOSION5}, // S_SONIC3KBOSSEXPLOSION4
	{SPR_BOM3, FF_FULLBRIGHT|4, 3, {NULL}, 0, 0, S_SONIC3KBOSSEXPLOSION6}, // S_SONIC3KBOSSEXPLOSION5
	{SPR_BOM3, FF_FULLBRIGHT|5, 4, {NULL}, 0, 0, S_NULL}, // S_SONIC3KBOSSEXPLOSION6

	{SPR_JETF, FF_ANIMATE|FF_FULLBRIGHT, -1, {NULL}, 2, 1, S_NULL}, // S_JETFUME1

	// Boss 1
	{SPR_EGGM,  0,   1, {A_Boss1Chase},            0, 0, S_EGGMOBILE_STND},   // S_EGGMOBILE_STND
	{SPR_EGGM, FF_ANIMATE|17, 35, {A_FaceTarget},  1, 2, S_EGGMOBILE_STND},   // S_EGGMOBILE_ROFL
	{SPR_EGGM,  1,   3, {A_FaceTarget},            0, 0, S_EGGMOBILE_LATK2},  // S_EGGMOBILE_LATK1
	{SPR_EGGM,  2,  15, {NULL},                    0, 0, S_EGGMOBILE_LATK3},  // S_EGGMOBILE_LATK2
	{SPR_EGGM,  3,   2, {NULL},                    0, 0, S_EGGMOBILE_LATK4},  // S_EGGMOBILE_LATK3
	{SPR_EGGM,  4,   2, {A_FaceTarget},            0, 0, S_EGGMOBILE_LATK5},  // S_EGGMOBILE_LATK4
	{SPR_EGGM,  6,   0, {A_PrepareRepeat},        45, 0, S_EGGMOBILE_LATK6},  // S_EGGMOBILE_LATK5
	{SPR_EGGM,  5,   1, {A_Boss1Laser},     MT_LASER, 0, S_EGGMOBILE_LATK7},  // S_EGGMOBILE_LATK6
	{SPR_EGGM,  6,   1, {A_Boss1Laser},     MT_LASER, (1<<16), S_EGGMOBILE_LATK8},  // S_EGGMOBILE_LATK7
	{SPR_EGGM,  5,   0, {A_Repeat}, 45, S_EGGMOBILE_LATK6, S_EGGMOBILE_LATK9},  // S_EGGMOBILE_LATK8
	{SPR_EGGM,  8,  2, {NULL},                     0, 0, S_EGGMOBILE_ROFL}, // S_EGGMOBILE_LATK9
	{SPR_EGGM,  9,   3, {A_FaceTarget},            0, 0, S_EGGMOBILE_RATK2},  // S_EGGMOBILE_RATK1
	{SPR_EGGM, 10,  15, {NULL},                    0, 0, S_EGGMOBILE_RATK3},  // S_EGGMOBILE_RATK2
	{SPR_EGGM, 11,   2, {NULL},                    0, 0, S_EGGMOBILE_RATK4},  // S_EGGMOBILE_RATK3
	{SPR_EGGM, 12,   2, {A_FaceTarget},            0, 0, S_EGGMOBILE_RATK5},  // S_EGGMOBILE_RATK4
	{SPR_EGGM, 14,   0, {A_PrepareRepeat},        45, 0, S_EGGMOBILE_RATK6},  // S_EGGMOBILE_RATK5
	{SPR_EGGM, 13,   1, {A_Boss1Laser},     MT_LASER, 1, S_EGGMOBILE_RATK7},  // S_EGGMOBILE_RATK6
	{SPR_EGGM, 14,   1, {A_Boss1Laser},     MT_LASER, 1|(1<<16), S_EGGMOBILE_RATK8},  // S_EGGMOBILE_RATK7
	{SPR_EGGM, 13,   0, {A_Repeat}, 45, S_EGGMOBILE_RATK6, S_EGGMOBILE_RATK9},  // S_EGGMOBILE_RATK8
	{SPR_EGGM, 16,  2, {NULL},                     0, 0, S_EGGMOBILE_ROFL}, // S_EGGMOBILE_RATK9
	{SPR_EGGM,  0,  0, {A_PrepareRepeat},            45, 0, S_EGGMOBILE_PANIC2},  // S_EGGMOBILE_PANIC1
	{SPR_EGGM, FF_ANIMATE|1, 16, {A_FaceTarget},      3, 4, S_EGGMOBILE_PANIC3},  // S_EGGMOBILE_PANIC2
	{SPR_EGGM,  7,  1, {A_Boss1Laser},         MT_LASER, 2, S_EGGMOBILE_PANIC4},  // S_EGGMOBILE_PANIC3
	{SPR_EGGM,  6,  1, {A_Boss1Laser},         MT_LASER, 2|(1<<16), S_EGGMOBILE_PANIC5},  // S_EGGMOBILE_PANIC4
	{SPR_EGGM,  6,  0, {A_Repeat}, 45,  S_EGGMOBILE_PANIC3, S_EGGMOBILE_PANIC6},  // S_EGGMOBILE_PANIC5
	{SPR_EGGM,  0,  0, {A_PrepareRepeat},            45, 0, S_EGGMOBILE_PANIC7},  // S_EGGMOBILE_PANIC6
	{SPR_EGGM, FF_ANIMATE|9, 16, {A_FaceTarget},      3, 4, S_EGGMOBILE_PANIC8},  // S_EGGMOBILE_PANIC7
	{SPR_EGGM, 15,  1, {A_Boss1Laser},         MT_LASER, 2, S_EGGMOBILE_PANIC9},  // S_EGGMOBILE_PANIC8
	{SPR_EGGM, 14,  1, {A_Boss1Laser},         MT_LASER, 2|(1<<16), S_EGGMOBILE_PANIC10},  // S_EGGMOBILE_PANIC9
	{SPR_EGGM, 14,  0, {A_Repeat}, 45,  S_EGGMOBILE_PANIC8, S_EGGMOBILE_PANIC11}, // S_EGGMOBILE_PANIC10
	{SPR_EGGM,  0,  0, {A_PrepareRepeat},            45, 0, S_EGGMOBILE_PANIC12}, // S_EGGMOBILE_PANIC11
	{SPR_EGGM, FF_ANIMATE|1, 16, {A_FaceTarget},      3, 4, S_EGGMOBILE_PANIC13}, // S_EGGMOBILE_PANIC12
	{SPR_EGGM,  7,  1, {A_Boss1Laser},         MT_LASER, 2, S_EGGMOBILE_PANIC14}, // S_EGGMOBILE_PANIC13
	{SPR_EGGM,  6,  1, {A_Boss1Laser},         MT_LASER, 2|(1<<16), S_EGGMOBILE_PANIC15}, // S_EGGMOBILE_PANIC14
	{SPR_EGGM,  6,  0, {A_Repeat}, 45,    S_EGGMOBILE_PANIC13, S_EGGMOBILE_ROFL}, // S_EGGMOBILE_PANIC15
	{SPR_EGGM, 19,  24, {A_Pain},                  0, 0, S_EGGMOBILE_PAIN2},  // S_EGGMOBILE_PAIN
	{SPR_EGGM, 19,  16, {A_SkullAttack},           3, 1, S_EGGMOBILE_STND},   // S_EGGMOBILE_PAIN2
	{SPR_EGGM, 20,  2, {A_Fall},                  17, 0, S_EGGMOBILE_DIE2},   // S_EGGMOBILE_DIE1
	{SPR_EGGM, 20,  2, {A_BossScream},             0, 0, S_EGGMOBILE_DIE3},   // S_EGGMOBILE_DIE2
	{SPR_EGGM, 20,  0, {A_Repeat}, 17, S_EGGMOBILE_DIE2, S_EGGMOBILE_DIE4},   // S_EGGMOBILE_DIE3
	{SPR_EGGM, 20,  -1, {A_BossDeath},             0, 0, S_NULL},             // S_EGGMOBILE_DIE4
	{SPR_EGGM, 21,  2, {A_BossScream},             0, 0, S_EGGMOBILE_FLEE2},  // S_EGGMOBILE_FLEE1
	{SPR_EGGM, 22,  2, {A_BossScream},             0, 0, S_EGGMOBILE_FLEE1},  // S_EGGMOBILE_FLEE2
	{SPR_UNID,  1,  1, {A_UnidusBall},             2, 0, S_EGGMOBILE_BALL},   // S_EGGMOBILE_BALL
	{SPR_NULL,  0,  1, {A_FocusTarget},            0, 0, S_EGGMOBILE_TARGET}, // S_EGGMOBILE_TARGET

	{SPR_EGLZ, 0, 35, {NULL}, 0, 0, S_NULL}, // S_BOSSEGLZ1
	{SPR_EGLZ, 1, 35, {NULL}, 0, 0, S_NULL}, // S_BOSSEGLZ2

	// Boss 2
	{SPR_EGGN, 0, -1,              {NULL},           0,          0, S_NULL},             // S_EGGMOBILE2_STND
	{SPR_EGGN, 1, 4,               {NULL},           0,          0, S_EGGMOBILE2_POGO2}, // S_EGGMOBILE2_POGO1
	{SPR_EGGN, 0, 2,  {A_Boss2PogoTarget},  9*FRACUNIT, 8*FRACUNIT, S_EGGMOBILE2_POGO3}, // S_EGGMOBILE2_POGO2
	{SPR_EGGN, 1, 2,               {NULL},           0,          0, S_EGGMOBILE2_POGO4}, // S_EGGMOBILE2_POGO3
	{SPR_EGGN, 2, -1,              {NULL},           0,          0, S_NULL},             // S_EGGMOBILE2_POGO4
	{SPR_EGGN, 1, 4,               {NULL},           0,          0, S_EGGMOBILE2_POGO6}, // S_EGGMOBILE2_POGO5
	{SPR_EGGN, 0, 2,  {A_Boss2PogoTarget},  7*FRACUNIT, 8*FRACUNIT, S_EGGMOBILE2_POGO7}, // S_EGGMOBILE2_POGO6
	{SPR_EGGN, 1, 2,               {NULL},           0,          0, S_EGGMOBILE2_POGO4}, // S_EGGMOBILE2_POGO7
	{SPR_EGGN, 3, 24, {A_Boss2TakeDamage},  24+TICRATE,          0, S_EGGMOBILE2_STND},  // S_EGGMOBILE2_PAIN
	{SPR_EGGN, 4, 24, {A_Boss2TakeDamage},  24+TICRATE,          0, S_EGGMOBILE2_POGO4}, // S_EGGMOBILE2_PAIN2
	{SPR_EGGN, 5, 2,             {A_Fall},           0,          0, S_EGGMOBILE2_DIE2},  // S_EGGMOBILE2_DIE1
	{SPR_EGGN, 5, 2,       {A_BossScream},           0,          0, S_EGGMOBILE2_DIE3},  // S_EGGMOBILE2_DIE2
	{SPR_EGGN, 5, 0,       {A_Repeat}, 17, S_EGGMOBILE2_DIE2, S_EGGMOBILE2_DIE4},  // S_EGGMOBILE2_DIE3
	{SPR_EGGN, 5, -1,       {A_BossDeath},           0,          0, S_NULL},             // S_EGGMOBILE2_DIE4
	{SPR_EGGN, 6, 2,       {A_BossScream},           0,          0, S_EGGMOBILE2_FLEE2}, // S_EGGMOBILE2_FLEE1
	{SPR_EGGN, 7, 2,       {A_BossScream},           0,          0, S_EGGMOBILE2_FLEE1}, // S_EGGMOBILE2_FLEE2

	{SPR_TANK, 0, 35, {NULL}, 0, 0, S_NULL}, // S_BOSSTANK1
	{SPR_TANK, 1, 35, {NULL}, 0, 0, S_NULL}, // S_BOSSTANK2
	{SPR_TANK, 2, 35, {NULL}, 0, 0, S_NULL}, // S_BOSSSPIGOT

	// Boss 2 Goop
	{SPR_GOOP,            0,  2, {A_SpawnObjectRelative}, 0, MT_GOOPTRAIL, S_GOOP2}, // S_GOOP1
	{SPR_GOOP,            1,  2, {A_SpawnObjectRelative}, 0, MT_GOOPTRAIL, S_GOOP1}, // S_GOOP2
	{SPR_GOOP,            2, -1,                  {NULL}, 0,            0, S_NULL},  // S_GOOP3
	{SPR_GOOP, FF_ANIMATE|3, 11,                  {NULL}, 2,            6, S_NULL},  // S_GOOPTRAIL

	// Boss 3
	{SPR_EGGO,  0,   1, {NULL},                    0, 0, S_EGGMOBILE3_STND},    // S_EGGMOBILE3_STND
	{SPR_EGGO,   FF_ANIMATE, 24, {NULL}, 1, 2, S_EGGMOBILE3_ATK2},  // S_EGGMOBILE3_SHOCK
	{SPR_EGGO, 6|FF_ANIMATE, 24, {NULL}, 1, 2, S_EGGMOBILE3_ATK2}, // S_EGGMOBILE3_ATK1
	{SPR_EGGO,  2,   2, {NULL},                    0, 0, S_EGGMOBILE3_ATK3A},   // S_EGGMOBILE3_ATK2
	{SPR_EGGO,  3,   2, {A_BossFireShot}, MT_TORPEDO, 2, S_EGGMOBILE3_ATK3B},   // S_EGGMOBILE3_ATK3A
	{SPR_EGGO,  3,   2, {A_BossFireShot}, MT_TORPEDO, 4, S_EGGMOBILE3_ATK3C},   // S_EGGMOBILE3_ATK3B
	{SPR_EGGO,  3,   2, {A_BossFireShot}, MT_TORPEDO, 3, S_EGGMOBILE3_ATK3D},   // S_EGGMOBILE3_ATK3C
	{SPR_EGGO,  3,   2, {A_BossFireShot}, MT_TORPEDO, 5, S_EGGMOBILE3_ATK4},    // S_EGGMOBILE3_ATK3D
	{SPR_EGGO,  4,   2, {NULL},                    0, 0, S_EGGMOBILE3_ATK5},    // S_EGGMOBILE3_ATK4
	{SPR_EGGO,  5,   2, {NULL},                    0, 0, S_EGGMOBILE3_ROFL},  // S_EGGMOBILE3_ATK5
	{SPR_EGGO,  6|FF_ANIMATE, 60, {NULL}, 1, 2, S_EGGMOBILE3_STND},  // S_EGGMOBILE3_ROFL
	{SPR_EGGO,  8,   1, {A_Boss3TakeDamage},       0, 0, S_EGGMOBILE3_PAIN2},   // S_EGGMOBILE3_PAIN
	{SPR_EGGO,  8,  23, {A_Pain},                  0, 0, S_EGGMOBILE3_STND},    // S_EGGMOBILE3_PAIN2
	{SPR_EGGO,  9,   2, {A_Fall},                  0, 0, S_EGGMOBILE3_DIE2},    // S_EGGMOBILE3_DIE1
	{SPR_EGGO,  9,   2, {A_BossScream},            0, 0, S_EGGMOBILE3_DIE3},    // S_EGGMOBILE3_DIE2
	{SPR_EGGO,  9,   0, {A_Repeat}, 17, S_EGGMOBILE3_DIE2, S_EGGMOBILE3_DIE4},  // S_EGGMOBILE3_DIE3
	{SPR_EGGO,  9,  -1, {A_BossDeath},             0, 0, S_NULL},               // S_EGGMOBILE3_DIE4
	{SPR_EGGO, 10,   2, {A_BossScream},            0, 0, S_EGGMOBILE3_FLEE2},   // S_EGGMOBILE3_FLEE1
	{SPR_EGGO, 11,   2, {A_BossScream},            0, 0, S_EGGMOBILE3_FLEE1},   // S_EGGMOBILE3_FLEE2

	// Boss 3 Pinch
	{SPR_FAKE, 0,  1, {NULL},                     0, 0, S_FAKEMOBILE},       // S_FAKEMOBILE_INIT
	{SPR_FAKE, 0,  1, {A_Boss3Path},              0, 0, S_FAKEMOBILE},       // S_FAKEMOBILE
	{SPR_FAKE, 0, 22, {NULL},                     0, 0, S_FAKEMOBILE_ATK2},  // S_FAKEMOBILE_ATK1
	{SPR_FAKE, 0,  2, {NULL},                     0, 0, S_FAKEMOBILE_ATK3A}, // S_FAKEMOBILE_ATK2
	{SPR_FAKE, 0,  2, {A_BossFireShot}, MT_TORPEDO2, 2, S_FAKEMOBILE_ATK3B}, // S_FAKEMOBILE_ATK3A
	{SPR_FAKE, 0,  2, {A_BossFireShot}, MT_TORPEDO2, 4, S_FAKEMOBILE_ATK3C}, // S_FAKEMOBILE_ATK3B
	{SPR_FAKE, 0,  2, {A_BossFireShot}, MT_TORPEDO2, 3, S_FAKEMOBILE_ATK3D}, // S_FAKEMOBILE_ATK3C
	{SPR_FAKE, 0,  2, {A_BossFireShot}, MT_TORPEDO2, 5, S_FAKEMOBILE},       // S_FAKEMOBILE_ATK3D
	{SPR_FAKE, 1,  1, {NULL},                     0, 0, S_FAKEMOBILE_DIE2},  // S_FAKEMOBILE_DIE1
	{SPR_NULL, 0,  1, {NULL},                     0, 0, S_FAKEMOBILE_DIE1},  // S_FAKEMOBILE_DIE2

	{SPR_SEBH, 0, 35, {NULL}, 0, 0, S_NULL}, // S_BOSSSEBH1
	{SPR_SEBH, 1, 35, {NULL}, 0, 0, S_NULL}, // S_BOSSSEBH2

	// Boss 3 Shockwave

	{SPR_SHCK,   FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 8, {A_Boss3ShockThink}, 4, 2, S_SHOCKWAVE2}, // S_SHOCKWAVE1
	{SPR_SHCK, 3|FF_FULLBRIGHT|FF_PAPERSPRITE|FF_ANIMATE, 8, {A_Boss3ShockThink}, 4, 2, S_SHOCKWAVE1}, // S_SHOCKWAVE2

	// Boss 4
	{SPR_EGGP, 0, -1, {NULL},           0,          0, S_NULL},              // S_EGGMOBILE4_STND
	{SPR_EGGP, 1,  3, {NULL},           0,          0, S_EGGMOBILE4_LATK2},  // S_EGGMOBILE4_LATK1
	{SPR_EGGP, 2, 15, {NULL},           0,          0, S_EGGMOBILE4_LATK3},  // S_EGGMOBILE4_LATK2
	{SPR_EGGP, 3,  2, {NULL},           0,          0, S_EGGMOBILE4_LATK4},  // S_EGGMOBILE4_LATK3
	{SPR_EGGP, 4,  2, {NULL},           0,          0, S_EGGMOBILE4_LATK5},  // S_EGGMOBILE4_LATK4
	{SPR_EGGP, 5, 50, {A_Boss4Reverse}, sfx_mswing, 0, S_EGGMOBILE4_LATK6},  // S_EGGMOBILE4_LATK5
	{SPR_EGGP, 6,  2, {NULL},           0,          0, S_EGGMOBILE4_STND},   // S_EGGMOBILE4_LATK6
	{SPR_EGGP, 7,  3, {NULL},           0,          0, S_EGGMOBILE4_RATK2},  // S_EGGMOBILE4_RATK1
	{SPR_EGGP, 8, 15, {NULL},           0,          0, S_EGGMOBILE4_RATK3},  // S_EGGMOBILE4_RATK2
	{SPR_EGGP, 9,  2, {NULL},           0,          0, S_EGGMOBILE4_RATK4},  // S_EGGMOBILE4_RATK3
	{SPR_EGGP,10,  2, {NULL},           0,          0, S_EGGMOBILE4_RATK5},  // S_EGGMOBILE4_RATK4
	{SPR_EGGP,11,150, {A_Boss4SpeedUp}, sfx_mswing, 0, S_EGGMOBILE4_RATK6},  // S_EGGMOBILE4_RATK5
	{SPR_EGGP,12,  2, {NULL},           0,          0, S_EGGMOBILE4_STND},   // S_EGGMOBILE4_RATK6
	{SPR_EGGP,13, 20, {A_Boss4Raise},   sfx_doord1, 0, S_EGGMOBILE4_RAISE2}, // S_EGGMOBILE4_RAISE1
	{SPR_EGGP,15|FF_ANIMATE, -1, {NULL},        1,        10, S_NULL},             // S_EGGMOBILE4_RAISE2
	{SPR_EGGP,13,  0, {A_Boss4Reverse}, sfx_alarm, sfx_s3k60, S_EGGMOBILE4_PAIN2}, // S_EGGMOBILE4_PAIN1
	{SPR_EGGP,13, 24, {A_Pain},                 0,         0, S_EGGMOBILE4_STND},  // S_EGGMOBILE4_PAIN2
	{SPR_EGGP,14,  2, {A_Fall},         0,          0, S_EGGMOBILE4_DIE2},   // S_EGGMOBILE4_DIE1
	{SPR_EGGP,14,  2, {A_BossScream},   0,          0, S_EGGMOBILE4_DIE3},   // S_EGGMOBILE4_DIE2
	{SPR_EGGP,14,  0, {A_Repeat},   17, S_EGGMOBILE4_DIE2, S_EGGMOBILE4_DIE4},   // S_EGGMOBILE4_DIE3
	{SPR_EGGP,14, -1, {A_BossDeath},    0,          0, S_NULL},              // S_EGGMOBILE4_DIE4
	{SPR_EGGP,15,  2, {A_BossScream},   0,          0, S_EGGMOBILE4_FLEE2},  // S_EGGMOBILE4_FLEE1
	{SPR_EGGP,16,  2, {A_BossScream},   0,          0, S_EGGMOBILE4_FLEE1},  // S_EGGMOBILE4_FLEE2
	{SPR_BMCE, 0, -1, {NULL},           0,          0, S_NULL},              // S_EGGMOBILE4_MACE
	{SPR_BMCE, 0,  2, {A_BossScream},   1, 0, S_EGGMOBILE4_MACE_DIE2},  // S_EGGMOBILE4_MACE_DIE1
	{SPR_NULL, 0,  2, {A_BossScream},   1, 0, S_EGGMOBILE4_MACE_DIE3},  // S_EGGMOBILE4_MACE_DIE2
	{SPR_NULL, 0,  0, {A_Repeat},       7, S_EGGMOBILE4_MACE_DIE1, S_SONIC3KBOSSEXPLOSION1}, // S_EGGMOBILE4_MACE_DIE3

	// Boss 4 jet flame
	{SPR_EFIR, FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 1, 1, S_NULL}, // S_JETFLAME

	// Boss 4 Spectator Eggrobo
	{SPR_EGR1,            0, -1, {NULL}, 0, 0, S_NULL},            // S_EGGROBO1_STND
	{SPR_EGR1,            5,  2, {NULL}, 0, 0, S_EGGROBO1_BSLAP2}, // S_EGGROBO1_BSLAP1
	{SPR_EGR1, FF_ANIMATE|6, 35, {NULL}, 1, 2, S_EGGROBO1_STND},   // S_EGGROBO1_BSLAP2
	{SPR_EGR1, FF_ANIMATE|3, -1, {NULL}, 1, 2, S_NULL},            // S_EGGROBO1_PISSED

	// Boss 4 Spectator Eggrobo jet flame
	{SPR_EFIR, FF_FULLBRIGHT|2,          -1, {NULL}, 0, 0, S_NULL}, // S_EGGROBOJET

	// Boss 5
	{SPR_NULL, 0, 2, {A_CheckFlags2}, MF2_AMBUSH, S_FANG_IDLE0, S_FANG_INTRO0}, // S_FANG_SETUP

	{SPR_NULL, 0, 2, {NULL}, 0, 0, S_FANG_INTRO1}, // S_FANG_INTRO0
	{SPR_NULL, 0, 2, {A_Boss5MakeJunk}, -S_FANG_CLONE1, 0, S_FANG_INTRO2}, // S_FANG_INTRO1
	{SPR_NULL, 0, 0, {A_Repeat}, 25, S_FANG_INTRO1, S_FANG_INTRO3}, // S_FANG_INTRO2
	{SPR_NULL, 0, 0, {A_Boss5MakeJunk}, 0, 1, S_FANG_INTRO4}, // S_FANG_INTRO3
	{SPR_FANG, 30, 1, {A_ZThrust}, 9, (1<<16)|1, S_FANG_INTRO5}, // S_FANG_INTRO4
	{SPR_FANG, 27, 1, {A_Boss5CheckOnGround}, S_FANG_INTRO9, 0, S_FANG_INTRO6}, // S_FANG_INTRO5
	{SPR_FANG, 28, 1, {A_Boss5CheckOnGround}, S_FANG_INTRO9, 0, S_FANG_INTRO7}, // S_FANG_INTRO6
	{SPR_FANG, 29, 1, {A_Boss5CheckOnGround}, S_FANG_INTRO9, 0, S_FANG_INTRO8}, // S_FANG_INTRO7
	{SPR_FANG, 30, 1, {A_Boss5CheckOnGround}, S_FANG_INTRO9, 0, S_FANG_INTRO5}, // S_FANG_INTRO8
	{SPR_FANG, 23|FF_ANIMATE, 50, {NULL}, 1, 4, S_FANG_INTRO10}, // S_FANG_INTRO9
	{SPR_FANG, 25, 5, {NULL}, 0, 0, S_FANG_INTRO11}, // S_FANG_INTRO10
	{SPR_FANG, 26, 2, {A_Boss5MakeJunk}, S_BROKENROBOTD, 2, S_FANG_INTRO12}, // S_FANG_INTRO11
	{SPR_FANG, 31|FF_ANIMATE, 50, {NULL}, 3, 4, S_FANG_IDLE1}, // S_FANG_INTRO12

	{SPR_FANG, 11, 2, {A_Boss5MakeJunk}, 0, -1, S_FANG_CLONE2}, // S_FANG_CLONE1
	{SPR_FANG, 11, 0, {A_Repeat}, 49, S_FANG_CLONE1, S_FANG_CLONE3}, // S_FANG_INTRO2
	{SPR_FANG, 12, 0, {A_SetObjectFlags}, MF_NOGRAVITY, 1, S_FANG_CLONE4}, // S_FANG_CLONE3
	{SPR_FANG, 12, 1, {A_Boss5CheckOnGround}, S_FANG_IDLE0, 0, S_FANG_CLONE4}, // S_FANG_CLONE4

	{SPR_FANG, 0,  0, {A_SetObjectFlags}, MF_NOCLIPTHING, 1, S_FANG_IDLE1}, // S_FANG_IDLE0
	{SPR_FANG, 2, 16, {A_Look}, 1, 0, S_FANG_IDLE2}, // S_FANG_IDLE1
	{SPR_FANG, 3, 16, {A_Look}, 1, 0, S_FANG_IDLE3}, // S_FANG_IDLE2
	{SPR_FANG, 3, 16, {A_Look}, 1, 0, S_FANG_IDLE4}, // S_FANG_IDLE3
	{SPR_FANG, 3, 16, {A_Look}, 1, 0, S_FANG_IDLE5}, // S_FANG_IDLE4
	{SPR_FANG, 2, 16, {A_Look}, 1, 0, S_FANG_IDLE6}, // S_FANG_IDLE5
	{SPR_FANG, 1, 16, {A_Look}, 1, 0, S_FANG_IDLE7}, // S_FANG_IDLE6
	{SPR_FANG, 1, 16, {A_Look}, 1, 0, S_FANG_IDLE8}, // S_FANG_IDLE7
	{SPR_FANG, 1, 16, {A_Look}, 1, 0, S_FANG_IDLE1}, // S_FANG_IDLE8

	{SPR_FANG, 14, 0, {A_DoNPCPain}, FRACUNIT, 0, S_FANG_PAIN2}, // S_FANG_PAIN1
	{SPR_FANG, 14, 1, {A_Boss5CheckOnGround}, S_FANG_PATHINGSTART1, S_FANG_PINCHPATHINGSTART1, S_FANG_PAIN2}, // S_FANG_PAIN2

	{SPR_FANG,  8, 0, {A_Boss5ExtraRepeat}, 5, 4, S_FANG_PATHINGSTART2}, // S_FANG_PATHINGSTART1
	{SPR_FANG,  8, 0, {A_PlayActiveSound}, 0, 0, S_FANG_PATHING}, // S_FANG_PATHINGSTART2
	{SPR_FANG,  8, 0, {A_Boss5FindWaypoint}, 0, 0, S_FANG_BOUNCE1}, // S_FANG_PATHING

	{SPR_FANG,  8, 2, {A_Thrust}, 0, 1, S_FANG_BOUNCE2}, // S_FANG_BOUNCE1
	{SPR_FANG,  9, 2, {NULL}, 0, 0, S_FANG_BOUNCE3}, // S_FANG_BOUNCE2
	{SPR_FANG, 10, 1, {A_Boss5Jump}, 0, 0, S_FANG_BOUNCE4}, // S_FANG_BOUNCE3
	{SPR_FANG, 10, 1, {A_Boss5CheckFalling}, S_FANG_CHECKPATH1, S_FANG_FALL1, S_FANG_BOUNCE4}, // S_FANG_BOUNCE4

	{SPR_FANG, 12, 1, {A_Boss5CheckOnGround}, S_FANG_CHECKPATH1, 0, S_FANG_FALL2}, // S_FANG_FALL1
	{SPR_FANG, 13, 1, {A_Boss5CheckOnGround}, S_FANG_CHECKPATH1, 0, S_FANG_FALL1}, // S_FANG_FALL2

	{SPR_FANG,  8, 0, {A_Boss5Calm}, 0, 0, S_FANG_CHECKPATH2}, // S_FANG_CHECKPATH1
	{SPR_FANG,  8, 0, {A_Repeat}, 0, S_FANG_PATHINGCONT1, S_FANG_SKID1}, // S_FANG_CHECKPATH2

	{SPR_FANG,  9, 0, {A_Boss5PinchShot}, MT_FBOMB, -16, S_FANG_PATHINGCONT2}, // S_FANG_PATHINGCONT1
	{SPR_FANG,  9, 0, {A_PlayActiveSound}, 0, 0, S_FANG_PATHINGCONT3}, // S_FANG_PATHINGCONT2
	{SPR_FANG,  9, 2, {A_Thrust}, 0, 1, S_FANG_PATHING}, // S_FANG_PATHINGCONT3

	{SPR_FANG,  4,  0, {A_PlayAttackSound}, 0, 0, S_FANG_SKID2}, // S_FANG_SKID1
	{SPR_FANG,  4,  1, {A_DoNPCSkid}, S_FANG_SKID3, 0, S_FANG_SKID2}, // S_FANG_SKID2
	{SPR_FANG,  4, 10, {NULL}, 0, 0, S_FANG_CHOOSEATTACK}, // S_FANG_SKID3

	{SPR_FANG,  0, 0, {A_RandomState}, S_FANG_LOBSHOT0, S_FANG_FIRESTART1, S_NULL}, // S_FANG_CHOOSEATTACK

	{SPR_FANG,  5,  0, {A_PrepareRepeat}, 3, 0, S_FANG_FIRESTART2}, // S_FANG_FIRESTART1 // Reset loop
	{SPR_FANG,  5, 18, {A_LookForBetter}, 1, 0, S_FANG_FIRE1}, // S_FANG_FIRESTART2
	{SPR_FANG,  5,  5, {A_FireShot}, MT_CORK, -16, S_FANG_FIRE2}, // S_FANG_FIRE1 // Start of loop
	{SPR_FANG,  6,  5, {NULL}, 0, 0, S_FANG_FIRE3}, // S_FANG_FIRE2
	{SPR_FANG,  7,  5, {NULL}, 0, 0, S_FANG_FIRE4}, // S_FANG_FIRE3
	{SPR_FANG,  5,  5, {NULL}, 2, 0, S_FANG_FIREREPEAT}, // S_FANG_FIRE4
	{SPR_FANG,  5,  0, {A_Repeat}, 3, S_FANG_FIRE1, S_FANG_WAIT1}, // S_FANG_FIREREPEAT // End of loop

	{SPR_FANG, 18, 16, {A_LookForBetter}, 1, 0, S_FANG_LOBSHOT1}, // S_FANG_LOBSHOT0
	{SPR_FANG, 19,  2, {A_LookForBetter}, 1, 0, S_FANG_LOBSHOT2}, // S_FANG_LOBSHOT1
	{SPR_FANG, 20, 18, {A_BrakLobShot}, MT_FBOMB, 32+(1<<16), S_FANG_WAIT1}, // S_FANG_LOBSHOT2

	{SPR_FANG, FF_ANIMATE|15, 70, {NULL}, 1, 5, S_FANG_WAIT2}, // S_FANG_WAIT1
	{SPR_FANG,             0, 35, {A_Look}, 1, 0, S_FANG_IDLE1}, // S_FANG_WAIT2

	{SPR_FANG, 12, 1, {A_Boss5CheckOnGround}, S_FANG_PATHINGSTART2, S_FANG_PINCHPATHINGSTART1, S_FANG_WALLHIT}, // S_FANG_WALLHIT

	{SPR_FANG,  8,  0, {A_PrepareRepeat}, 1, 0, S_FANG_PINCHPATHINGSTART2}, // S_FANG_PINCHPATHINGSTART1
	{SPR_FANG,  8,  0, {A_PlayActiveSound}, 0, 0, S_FANG_PINCHPATHING}, // S_FANG_PINCHPATHINGSTART2
	{SPR_FANG,  8,  0, {A_Boss5FindWaypoint}, 1, 0, S_FANG_PINCHBOUNCE0}, // S_FANG_PINCHPATHING
	{SPR_FANG,  8,  0, {A_SetObjectFlags}, MF_NOCLIP|MF_NOCLIPHEIGHT, 2, S_FANG_PINCHBOUNCE1}, // S_FANG_PINCHBOUNCE0
	{SPR_FANG,  8,  2, {A_Thrust}, 0, 1, S_FANG_PINCHBOUNCE2}, // S_FANG_PINCHBOUNCE1
	{SPR_FANG,  9,  2, {NULL}, 0, 0, S_FANG_PINCHBOUNCE3}, // S_FANG_PINCHBOUNCE2
	{SPR_FANG, 10,  2, {A_Boss5Jump}, 0, 0, S_FANG_PINCHBOUNCE4}, // S_FANG_PINCHBOUNCE3
	{SPR_FANG, 10,  1, {A_Boss5CheckFalling}, S_FANG_PINCHSKID1, S_FANG_PINCHFALL0, S_FANG_PINCHBOUNCE4}, // S_FANG_PINCHBOUNCE4
	{SPR_FANG, 12,  0, {A_SetObjectFlags}, MF_NOCLIP|MF_NOCLIPHEIGHT, 1, S_FANG_PINCHFALL1}, // S_FANG_PINCHFALL0
	{SPR_FANG, 12,  1, {A_Boss5CheckOnGround}, S_FANG_PINCHSKID1, 0, S_FANG_PINCHFALL2}, // S_FANG_PINCHFALL1
	{SPR_FANG, 13,  1, {A_Boss5CheckOnGround}, S_FANG_PINCHSKID1, 0, S_FANG_PINCHFALL1}, // S_FANG_PINCHFALL2
	{SPR_FANG,  4,  0, {A_PlayAttackSound}, 0, 0, S_FANG_PINCHSKID2}, // S_FANG_PINCHSKID1
	{SPR_FANG,  4,  1, {A_DoNPCSkid}, S_FANG_PINCHLOBSHOT0, 0, S_FANG_PINCHSKID2}, // S_FANG_PINCHSKID2
	{SPR_FANG, 18, 16, {A_FaceTarget}, 3, 0, S_FANG_PINCHLOBSHOT1}, // S_FANG_PINCHLOBSHOT0
	{SPR_FANG, 19,  2, {A_FaceTarget}, 3, 0, S_FANG_PINCHLOBSHOT2}, // S_FANG_PINCHLOBSHOT1
	{SPR_FANG, 20, 30, {A_Boss5MakeItRain}, MT_FBOMB, -16, S_FANG_PINCHLOBSHOT3}, // S_FANG_PINCHLOBSHOT2
	{SPR_FANG, 20, 18, {A_LinedefExecute}, LE_BOSS4DROP, 0, S_FANG_PINCHLOBSHOT4}, // S_FANG_PINCHLOBSHOT3
	{SPR_FANG,  0,  0, {A_Boss5Calm}, 0, 0, S_FANG_PATHINGSTART1}, // S_FANG_PINCHLOBSHOT4

	{SPR_FANG, 21, 0, {A_DoNPCPain},                    0, 0, S_FANG_DIE2}, // S_FANG_DIE1
	{SPR_FANG, 21, 1, {A_Boss5CheckOnGround}, S_FANG_DIE3, 0, S_FANG_DIE2}, // S_FANG_DIE2

	{SPR_FANG, 22,  0, {A_Scream}, 0, 0, S_FANG_DIE4}, // S_FANG_DIE3
	{SPR_FANG, 22, -1, {A_SetFuse}, 70, 0, S_FANG_DIE5}, // S_FANG_DIE4

	{SPR_FANG, 11, 0, {A_PlaySound}, sfx_jump, 0, S_FANG_DIE6}, // S_FANG_DIE5
	{SPR_FANG, 11, 1, {A_ZThrust}, 6, (1<<16)|1, S_FANG_DIE7}, // S_FANG_DIE6
	{SPR_FANG, 11, 1, {A_Boss5CheckFalling}, S_FANG_FLEEPATHING1, S_FANG_DIE8, S_FANG_DIE7}, // S_FANG_DIE7
	{SPR_FANG, 12, 1, {A_Boss5CheckOnGround}, S_FANG_FLEEPATHING1, 0, S_FANG_DIE8}, // S_FANG_DIE8

	{SPR_FANG,  9,  0, {A_PlayActiveSound}, 0, 0, S_FANG_FLEEPATHING2}, // S_FANG_FLEEPATHING1
	{SPR_FANG,  8,  2, {A_Boss5FindWaypoint}, 2, 0, S_FANG_FLEEBOUNCE1}, // S_FANG_FLEEPATHING2
	{SPR_FANG,  9,  2, {NULL}, 0, 0, S_FANG_FLEEBOUNCE2}, // S_FANG_FLEEBOUNCE1
	{SPR_FANG, 10, -1, {A_BossDeath}, 0, 0, S_NULL}, // S_FANG_FLEEBOUNCE2

	{SPR_FANG, 17, 7*TICRATE, {NULL}, 0, 0, S_NULL}, // S_FANG_KO

	{SPR_NULL, 0, -1, {A_RandomStateRange}, S_BROKENROBOTA, S_BROKENROBOTF, S_NULL}, // S_BROKENROBOTRANDOM
	{SPR_BRKN,    FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 3, 4, S_NULL}, // S_BROKENROBOTA
	{SPR_BRKN,  4|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 3, 4, S_NULL}, // S_BROKENROBOTB
	{SPR_BRKN,  8|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 3, 4, S_NULL}, // S_BROKENROBOTC
	{SPR_BRKN, 12|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 3, 4, S_NULL}, // S_BROKENROBOTD
	{SPR_BRKN, 16|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 3, 4, S_NULL}, // S_BROKENROBOTE
	{SPR_BRKN, 20|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 3, 4, S_NULL}, // S_BROKENROBOTF

	{SPR_WHAT,   FF_ANIMATE|FF_FULLBRIGHT,  4, {NULL}, 1, 2, S_ALART2}, // S_ALART1
	{SPR_WHAT, 2|FF_ANIMATE|FF_FULLBRIGHT, -1, {NULL}, 1, 2, S_NULL},   // S_ALART2

	{SPR_VWRE,   FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_VWREF
	{SPR_VWRE, 1|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_VWREB

	{SPR_PROJ,   FF_TRANS20|FF_FULLBRIGHT,  4, {NULL}, 0, 0, S_PROJECTORLIGHT2}, // S_PROJECTORLIGHT1
	{SPR_PROJ, 1|FF_TRANS40|FF_FULLBRIGHT,  1, {NULL}, 0, 0, S_PROJECTORLIGHT3}, // S_PROJECTORLIGHT2
	{SPR_PROJ, 2|FF_TRANS20|FF_FULLBRIGHT,  1, {NULL}, 0, 0, S_PROJECTORLIGHT4}, // S_PROJECTORLIGHT3
	{SPR_PROJ, 3|FF_TRANS40|FF_FULLBRIGHT,  2, {A_Repeat}, 39, S_PROJECTORLIGHT2, S_PROJECTORLIGHT5}, // S_PROJECTORLIGHT4
	{SPR_PROJ, 4|FF_TRANS60|FF_FULLBRIGHT,  2, {NULL}, 0, 0, S_NULL}, // S_PROJECTORLIGHT5

	{SPR_FBOM, 0, 1, {A_GhostMe}, 0, 0, S_FBOMB2}, // S_FBOMB1
	{SPR_FBOM, 1, 1, {A_GhostMe}, 0, 0, S_FBOMB1}, // S_FBOMB2
	{SPR_BARX, 0|FF_FULLBRIGHT, 3, {A_SetObjectFlags}, MF_NOCLIP|MF_NOGRAVITY|MF_NOBLOCKMAP, 0, S_FBOMB_EXPL2}, // S_FBOMB_EXPL1
	{SPR_BARX, 1|FF_FULLBRIGHT, 2, {A_Boss5BombExplode}, MT_TNTDUST, 0, S_FBOMB_EXPL3}, // S_FBOMB_EXPL2
	{SPR_BARX, 1|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_FBOMB_EXPL4}, // S_FBOMB_EXPL3
	{SPR_BARX, 2|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_FBOMB_EXPL5}, // S_FBOMB_EXPL4
	{SPR_BARX, 3|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_FBOMB_EXPL6}, // S_FBOMB_EXPL5
	{SPR_NULL, 0, 2*TICRATE, {NULL}, 0, 0, S_NULL}, // S_FBOMB_EXPL6
	{SPR_BARD, 0|FF_TRANS90, 2, {NULL}, 0, 0, S_TNTDUST_2}, // S_TNTDUST_1
	{SPR_BARD, 0|FF_TRANS30, 2*TICRATE, {A_SetRandomTics}, 2, TICRATE, S_TNTDUST_3}, // S_TNTDUST_2
	{SPR_BARD, 0|FF_TRANS40, 10, {NULL}, 0, 0, S_TNTDUST_4}, // S_TNTDUST_3
	{SPR_BARD, 0|FF_TRANS50, 10, {NULL}, 0, 0, S_TNTDUST_5}, // S_TNTDUST_4
	{SPR_BARD, 0|FF_TRANS60, 10, {NULL}, 0, 0, S_TNTDUST_6}, // S_TNTDUST_5
	{SPR_BARD, 0|FF_TRANS70, 10, {NULL}, 0, 0, S_TNTDUST_7}, // S_TNTDUST_6
	{SPR_BARD, 0|FF_TRANS80, 10, {NULL}, 0, 0, S_TNTDUST_8}, // S_TNTDUST_7
	{SPR_BARD, 0|FF_TRANS90, 10, {NULL}, 0, 0, S_NULL}, // S_TNTDUST_8
	{SPR_FSGN, 0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_FSGNA
	{SPR_FSGN, 1|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_FSGNB
	{SPR_FSGN, 2|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_FSGNC
	{SPR_FSGN, 3|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_FSGND

	// Black Eggman (Boss 7)
	{SPR_BRAK, 0, 1, {A_SetReactionTime}, 0, 0, S_BLACKEGG_STND2}, // S_BLACKEGG_STND
	{SPR_BRAK, 0, 7, {A_Look}, 1, 0, S_BLACKEGG_STND2}, // S_BLACKEGG_STND2
	{SPR_BRAK, 1, 7, {NULL}, 0, 0, S_BLACKEGG_WALK2}, // S_BLACKEGG_WALK1
	{SPR_BRAK, 2, 7, {NULL}, 0, 0, S_BLACKEGG_WALK3}, // S_BLACKEGG_WALK2
	{SPR_BRAK, 3, 7, {A_PlaySound}, sfx_bestep, 0, S_BLACKEGG_WALK4}, // S_BLACKEGG_WALK3
	{SPR_BRAK, 4, 7, {NULL}, 0, 0, S_BLACKEGG_WALK5}, // S_BLACKEGG_WALK4
	{SPR_BRAK, 5, 7, {NULL}, 0, 0, S_BLACKEGG_WALK6}, // S_BLACKEGG_WALK5
	{SPR_BRAK, 6, 7, {A_PlaySound}, sfx_bestp2, 0, S_BLACKEGG_WALK1}, // S_BLACKEGG_WALK6
	{SPR_BRAK, 7, 3, {NULL}, 0, 0, S_BLACKEGG_SHOOT2}, // S_BLACKEGG_SHOOT1
	{SPR_BRAK, 24, 1, {A_PlaySound}, sfx_befire, 0, S_BLACKEGG_SHOOT1}, // S_BLACKEGG_SHOOT2

	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN2}, // S_BLACKEGG_PAIN1
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN3}, // S_BLACKEGG_PAIN2
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN4}, // S_BLACKEGG_PAIN3
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN5}, // S_BLACKEGG_PAIN4
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN6}, // S_BLACKEGG_PAIN5
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN7}, // S_BLACKEGG_PAIN6
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN8}, // S_BLACKEGG_PAIN7
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN9}, // S_BLACKEGG_PAIN8
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN10}, // S_BLACKEGG_PAIN9
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN11}, // S_BLACKEGG_PAIN10
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN12}, // S_BLACKEGG_PAIN11
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN13}, // S_BLACKEGG_PAIN12
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN14}, // S_BLACKEGG_PAIN13
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN15}, // S_BLACKEGG_PAIN14
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN16}, // S_BLACKEGG_PAIN15
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN17}, // S_BLACKEGG_PAIN16
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN18}, // S_BLACKEGG_PAIN17
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN19}, // S_BLACKEGG_PAIN18
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN20}, // S_BLACKEGG_PAIN19
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN21}, // S_BLACKEGG_PAIN20
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN22}, // S_BLACKEGG_PAIN21
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN23}, // S_BLACKEGG_PAIN22
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN24}, // S_BLACKEGG_PAIN23
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN25}, // S_BLACKEGG_PAIN24
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN26}, // S_BLACKEGG_PAIN25
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN27}, // S_BLACKEGG_PAIN26
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN28}, // S_BLACKEGG_PAIN27
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN29}, // S_BLACKEGG_PAIN28
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN30}, // S_BLACKEGG_PAIN29
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN31}, // S_BLACKEGG_PAIN30
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN32}, // S_BLACKEGG_PAIN31
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN33}, // S_BLACKEGG_PAIN32
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN34}, // S_BLACKEGG_PAIN33
	{SPR_BRAK, 25, 1, {NULL}, 0, 0, S_BLACKEGG_PAIN35}, // S_BLACKEGG_PAIN34
	{SPR_BRAK, 8, 1, {NULL}, 0, 0, S_BLACKEGG_WALK1}, // S_BLACKEGG_PAIN35

	{SPR_BRAK, 9, 20, {NULL}, 0, 0, S_BLACKEGG_HITFACE2}, // S_BLACKEGG_HITFACE1
	{SPR_BRAK, 10, 2, {NULL}, 0, 0, S_BLACKEGG_HITFACE3}, // S_BLACKEGG_HITFACE2
	{SPR_BRAK, 11, 2, {NULL}, 0, 0, S_BLACKEGG_HITFACE4}, // S_BLACKEGG_HITFACE3
	{SPR_BRAK, 12,14, {NULL}, 0, 0, S_BLACKEGG_PAIN1}, // S_BLACKEGG_HITFACE4

	{SPR_BRAK, 13, 14, {NULL}, 0, 0, S_BLACKEGG_DIE2}, // S_BLACKEGG_DIE1
	{SPR_BRAK, 14, 7, {NULL}, 0, 0, S_BLACKEGG_DIE3}, // S_BLACKEGG_DIE2
	{SPR_BRAK, 15, 5, {NULL}, 0, 0, S_BLACKEGG_DIE4}, // S_BLACKEGG_DIE3
	{SPR_BRAK, 16, 3, {A_PlaySound}, sfx_bgxpld, 0, S_BLACKEGG_DIE5}, // S_BLACKEGG_DIE4
	{SPR_BRAK, 17, -1, {NULL}, 0, 0, S_BLACKEGG_DIE5}, // S_BLACKEGG_DIE5

	{SPR_BRAK, 18, 14, {NULL}, 0, 0, S_BLACKEGG_MISSILE2}, // S_BLACKEGG_MISSILE1
	{SPR_BRAK, 19, 5, {NULL}, 0, 0, S_BLACKEGG_MISSILE3}, // S_BLACKEGG_MISSILE2
	{SPR_BRAK, 20, 35, {A_Boss7FireMissiles}, MT_BLACKEGGMAN_MISSILE, sfx_beshot, S_BLACKEGG_JUMP1}, // S_BLACKEGG_MISSILE3

	{SPR_BRAK, 21, -1, {NULL}, 0, 0, S_BLACKEGG_STND}, // S_BLACKEGG_GOOP

	{SPR_BRAK, 22, 14, {A_PlaySound}, sfx_belnch, 0, S_BLACKEGG_JUMP2}, // S_BLACKEGG_JUMP1
	{SPR_BRAK, 23, -1, {NULL}, 0, 0, S_BLACKEGG_WALK1}, // S_BLACKEGG_JUMP2

	{SPR_BRAK, 21, 3*TICRATE, {NULL}, 0, 0, S_BLACKEGG_DESTROYPLAT2}, // S_BLACKEGG_DESTROYPLAT1
	{SPR_BRAK, 21, 1, {A_PlaySound}, sfx_s3k54, 0, S_BLACKEGG_DESTROYPLAT3}, // S_BLACKEGG_DESTROYPLAT2
	{SPR_BRAK, 21, 14, {A_LinedefExecute}, LE_BRAKPLATFORM, 0, S_BLACKEGG_STND}, // S_BLACKEGG_DESTROYPLAT3

	{SPR_NULL, 0, 1, {A_CapeChase}, (160 - 20) << 16, 0, S_BLACKEGG_HELPER}, // S_BLACKEGG_HELPER

	{SPR_BGOO, FF_TRANS50  , 2, {NULL}, 0, 0, S_BLACKEGG_GOOP2}, // S_BLACKEGG_GOOP1
	{SPR_BGOO, FF_TRANS50|1, 2, {NULL}, 0, 0, S_BLACKEGG_GOOP1}, // S_BLACKEGG_GOOP2
	{SPR_BGOO, FF_TRANS50|2, 6*TICRATE, {A_GoopSplat}, 0, 0, S_BLACKEGG_GOOP4}, // S_BLACKEGG_GOOP3
	{SPR_BGOO, FF_TRANS60|2, 4, {NULL}, 0, 0, S_BLACKEGG_GOOP5}, // S_BLACKEGG_GOOP4
	{SPR_BGOO, FF_TRANS70|2, 4, {NULL}, 0, 0, S_BLACKEGG_GOOP6}, // S_BLACKEGG_GOOP5
	{SPR_BGOO, FF_TRANS80|2, 4, {NULL}, 0, 0, S_BLACKEGG_GOOP7}, // S_BLACKEGG_GOOP6
	{SPR_BGOO, FF_TRANS90|2, 4, {NULL}, 0, 0, S_NULL}, // S_BLACKEGG_GOOP7

	{SPR_BMSL, 0, 1, {NULL}, 0, 0, S_BLACKEGG_MISSILE}, // S_BLACKEGG_MISSILE

	// New Very-Last-Minute 2.1 Brak Eggman (Cy-Brak-demon)
	{SPR_BRAK, 0, 10, {A_Look}, 0, 0, S_CYBRAKDEMON_IDLE}, // S_CYBRAKDEMON_IDLE
	{SPR_BRAK, 1, 8, {A_BrakChase}, 3, 0, S_CYBRAKDEMON_WALK2}, // S_CYBRAKDEMON_WALK1
	{SPR_BRAK, 2, 8, {A_BrakChase}, 3, 0, S_CYBRAKDEMON_WALK3}, // S_CYBRAKDEMON_WALK2
	{SPR_BRAK, 3, 8, {A_BrakChase}, 3, sfx_bestep, S_CYBRAKDEMON_WALK4}, // S_CYBRAKDEMON_WALK3
	{SPR_BRAK, 4, 8, {A_BrakChase}, 3, 0, S_CYBRAKDEMON_WALK5}, // S_CYBRAKDEMON_WALK4
	{SPR_BRAK, 5, 8, {A_BrakChase}, 3, 0, S_CYBRAKDEMON_WALK6}, // S_CYBRAKDEMON_WALK5
	{SPR_BRAK, 6, 8, {A_BrakChase}, 3, sfx_bestp2, S_CYBRAKDEMON_WALK1}, // S_CYBRAKDEMON_WALK6
	{SPR_BRAK, 7, 6, {A_RandomState}, S_CYBRAKDEMON_MISSILE_ATTACK1, S_CYBRAKDEMON_FLAME_ATTACK1, S_CYBRAKDEMON_MISSILE_ATTACK1}, // S_CYBRAKDEMON_CHOOSE_ATTACK1
	{SPR_BRAK, 7, 6, {A_FaceTarget}, 0, 0, S_CYBRAKDEMON_MISSILE_ATTACK2}, // S_CYBRAKDEMON_MISSILE_ATTACK1 // Aim
	{SPR_BRAK, 26 + FF_FULLBRIGHT, 12, {A_BrakFireShot}, MT_CYBRAKDEMON_MISSILE, 128, S_CYBRAKDEMON_MISSILE_ATTACK3}, // S_CYBRAKDEMON_MISSILE_ATTACK2 // Fire
	{SPR_BRAK, 7, 12, {A_FaceTarget}, 0, 0, S_CYBRAKDEMON_MISSILE_ATTACK4}, // S_CYBRAKDEMON_MISSILE_ATTACK3 // Aim
	{SPR_BRAK, 26 + FF_FULLBRIGHT, 12, {A_BrakFireShot}, MT_CYBRAKDEMON_MISSILE, 128, S_CYBRAKDEMON_MISSILE_ATTACK5}, // S_CYBRAKDEMON_MISSILE_ATTACK4 // Fire
	{SPR_BRAK, 7, 12, {A_FaceTarget}, 0, 0, S_CYBRAKDEMON_MISSILE_ATTACK6}, // S_CYBRAKDEMON_MISSILE_ATTACK5 // Aim
	{SPR_BRAK, 26 + FF_FULLBRIGHT, 12, {A_BrakFireShot}, MT_CYBRAKDEMON_MISSILE, 128, S_CYBRAKDEMON_FINISH_ATTACK1}, // S_CYBRAKDEMON_MISSILE_ATTACK6 // Fire
	{SPR_BRAK, 7, 1, {A_Repeat}, 1, S_CYBRAKDEMON_FLAME_ATTACK1, S_CYBRAKDEMON_FLAME_ATTACK2}, // S_CYBRAKDEMON_FLAME_ATTACK1 // Reset
	{SPR_BRAK, 7, 6, {A_FaceTarget}, 0, 0, S_CYBRAKDEMON_FLAME_ATTACK3}, // S_CYBRAKDEMON_FLAME_ATTACK2 // Aim
	{SPR_BRAK, 26 + FF_FULLBRIGHT, 2, {A_BrakFireShot}, MT_CYBRAKDEMON_FLAMESHOT, 128, S_CYBRAKDEMON_FLAME_ATTACK4}, // S_CYBRAKDEMON_FLAME_ATTACK3 // Fire
	{SPR_BRAK, 7, 1, {A_Repeat}, 30, S_CYBRAKDEMON_FLAME_ATTACK3, S_CYBRAKDEMON_FINISH_ATTACK1}, // S_CYBRAKDEMON_FLAME_ATTACK4 // Loop
	{SPR_BRAK, 0, 6, {A_RandomState}, S_CYBRAKDEMON_VILE_ATTACK1, S_CYBRAKDEMON_NAPALM_ATTACK1, S_CYBRAKDEMON_MISSILE_ATTACK1}, // S_CYBRAKDEMON_CHOOSE_ATTACK2
	{SPR_BRAK, 20, 0, {A_LinedefExecute}, LE_BRAKVILEATACK, 0, S_CYBRAKDEMON_VILE_ATTACK2}, // S_CYBRAKDEMON_VILE_ATTACK1
	{SPR_BRAK, 20, 24, {A_VileTarget}, MT_CYBRAKDEMON_TARGET_RETICULE, 1, S_CYBRAKDEMON_VILE_ATTACK3}, // S_CYBRAKDEMON_VILE_ATTACK2
	{SPR_BRAK, 19, 8, {A_FaceTarget}, 0, 0, S_CYBRAKDEMON_VILE_ATTACK4}, // S_CYBRAKDEMON_VILE_ATTACK3
	{SPR_BRAK, 18, 8, {A_FaceTarget}, 0, 0, S_CYBRAKDEMON_VILE_ATTACK5}, // S_CYBRAKDEMON_VILE_ATTACK4
	{SPR_BRAK, 8, 32, {A_FaceTarget}, 0, 0, S_CYBRAKDEMON_VILE_ATTACK6}, // S_CYBRAKDEMON_VILE_ATTACK5
	{SPR_BRAK, 20 + FF_FULLBRIGHT, 28, {A_VileAttack}, sfx_brakrx, MT_CYBRAKDEMON_VILE_EXPLOSION + (1<<16), S_CYBRAKDEMON_FINISH_ATTACK1}, // S_CYBRAKDEMON_VILE_ATTACK6
	{SPR_BRAK, 0, 6, {A_FaceTarget}, 0, 0, S_CYBRAKDEMON_NAPALM_ATTACK2}, // S_CYBRAKDEMON_NAPALM_ATTACK1
	{SPR_BRAK, 21 + FF_FULLBRIGHT, 8, {A_BrakLobShot}, MT_CYBRAKDEMON_NAPALM_BOMB_LARGE, 96, S_CYBRAKDEMON_NAPALM_ATTACK3}, // S_CYBRAKDEMON_NAPALM_ATTACK2
	{SPR_BRAK, 0, 8, {A_FaceTarget}, 0, 0, S_CYBRAKDEMON_FINISH_ATTACK1}, // S_CYBRAKDEMON_NAPALM_ATTACK3
	{SPR_BRAK, 0, 0, {A_SetObjectFlags2}, MF2_FRET, 1, S_CYBRAKDEMON_FINISH_ATTACK2}, // S_CYBRAKDEMON_FINISH_ATTACK1 // If just attacked, remove MF2_FRET w/out going back to spawnstate
	{SPR_BRAK, 0, 0, {A_SetReactionTime}, 0, 0, S_CYBRAKDEMON_WALK1}, // S_CYBRAKDEMON_FINISH_ATTACK2 // If just attacked, remove MF2_FRET w/out going back to spawnstate
	{SPR_BRAK, 18, 24, {A_Pain}, 0, 0, S_CYBRAKDEMON_PAIN2}, // S_CYBRAKDEMON_PAIN1
	{SPR_BRAK, 18, 0, {A_CheckHealth}, 3, S_CYBRAKDEMON_PAIN3, S_CYBRAKDEMON_CHOOSE_ATTACK1}, // S_CYBRAKDEMON_PAIN2
	{SPR_BRAK, 18, 0, {A_LinedefExecute}, LE_PINCHPHASE, 0, S_CYBRAKDEMON_CHOOSE_ATTACK1}, // S_CYBRAKDEMON_PAIN3
	{SPR_BRAK, 18, 1, {A_Repeat}, 1, S_CYBRAKDEMON_DIE1, S_CYBRAKDEMON_DIE2}, // S_CYBRAKDEMON_DIE1
	{SPR_BRAK, 18, 2, {A_BossScream}, 2, 0, S_CYBRAKDEMON_DIE3}, // S_CYBRAKDEMON_DIE2
	{SPR_BRAK, 18, 0, {A_Repeat}, 52, S_CYBRAKDEMON_DIE2, S_CYBRAKDEMON_DIE4}, // S_CYBRAKDEMON_DIE3
	{SPR_BRAK, 13, 34, {A_BossDeath}, 0, 0, S_CYBRAKDEMON_DIE5}, // S_CYBRAKDEMON_DIE4
	{SPR_BRAK, 14, 34, {NULL}, 0, 0, S_CYBRAKDEMON_DIE6}, // S_CYBRAKDEMON_DIE5
	{SPR_BRAK, 15, 34, {NULL}, 0, 0, S_CYBRAKDEMON_DIE7}, // S_CYBRAKDEMON_DIE6
	{SPR_BRAK, 16, 34, {NULL}, 0, 0, S_CYBRAKDEMON_DIE8}, // S_CYBRAKDEMON_DIE7
	{SPR_BRAK, 17, 34, {NULL}, sfx_befall, 0, S_CYBRAKDEMON_DIE8}, // S_CYBRAKDEMON_DIE8
	{SPR_BRAK, 0, 0, {A_SetObjectFlags}, MF_SPECIAL|MF_SHOOTABLE, 2, S_CYBRAKDEMON_IDLE}, // S_CYBRAKDEMON_DEINVINCIBLERIZE
	{SPR_BRAK, 0, 0, {A_SetObjectFlags}, MF_SPECIAL|MF_SHOOTABLE, 1, S_CYBRAKDEMON_IDLE}, // S_CYBRAKDEMON_INVINCIBLERIZE

	{SPR_RCKT, 0 + FF_FULLBRIGHT, 1, {A_SetObjectFlags2}, MF2_RAILRING, 2, S_CYBRAKDEMONMISSILE}, // S_CYBRAKDEMONMISSILE
	{SPR_RCKT, 1 + FF_FULLBRIGHT, 8, {A_Explode}, 0, 0, S_CYBRAKDEMONMISSILE_EXPLODE2}, // S_CYBRAKDEMONMISSILE_EXPLODE1 //TODO: set missile mobj's "damage" to an appropriate radius
	{SPR_RCKT, 2 + FF_FULLBRIGHT, 6, {A_NapalmScatter}, MT_CYBRAKDEMON_NAPALM_FLAMES + (6<<16), 32 + (16<<16), S_CYBRAKDEMONMISSILE_EXPLODE3}, // S_CYBRAKDEMONMISSILE_EXPLODE2
	{SPR_RCKT, 3 + FF_FULLBRIGHT, 4, {NULL}, 0, 0, S_NULL}, // S_CYBRAKDEMONMISSILE_EXPLODE3

	{SPR_FLME, FF_FULLBRIGHT  , 15, {NULL}, 0, 0, S_CYBRAKDEMONFLAMESHOT_FLY2}, // S_CYBRAKDEMONFLAMESHOT_FLY1
	{SPR_FLME, FF_FULLBRIGHT|1, 15, {NULL}, 0, 0, S_CYBRAKDEMONFLAMESHOT_FLY3}, // S_CYBRAKDEMONFLAMESHOT_FLY2
	{SPR_FLME, FF_FULLBRIGHT|2, -1, {NULL}, 0, 0, S_CYBRAKDEMONFLAMESHOT_FLY3}, // S_CYBRAKDEMONFLAMESHOT_FLY3
	{SPR_FLME, FF_FULLBRIGHT|2, 0, {A_SpawnObjectRelative}, 0, MT_CYBRAKDEMON_FLAMEREST, S_NULL}, // S_CYBRAKDEMONFLAMESHOT_DIE

	{SPR_FLAM, FF_FULLBRIGHT, 1, {A_SetFuse}, 10*TICRATE, 0, S_FLAMEREST}, // S_CYBRAKDEMONFLAMEREST

	{SPR_ELEC, 0 + FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER_INIT2}, // S_CYBRAKDEMONELECTRICBARRIER_INIT1
	{SPR_ELEC, 0 + FF_FULLBRIGHT, 0, {A_RemoteAction}, -1, S_CYBRAKDEMON_INVINCIBLERIZE, S_CYBRAKDEMONELECTRICBARRIER_PLAYSOUND}, // S_CYBRAKDEMONELECTRICBARRIER_INIT2
	{SPR_ELEC, 0 + FF_FULLBRIGHT, 0, {A_PlayActiveSound}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER1}, // S_CYBRAKDEMONELECTRICBARRIER_PLAYSOUND
	{SPR_ELEC, 0 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER2}, // S_CYBRAKDEMONELECTRICBARRIER1
	{SPR_ELEC, 0 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER3}, // S_CYBRAKDEMONELECTRICBARRIER2
	{SPR_ELEC, 1 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER4}, // S_CYBRAKDEMONELECTRICBARRIER3
	{SPR_ELEC, 1 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER5}, // S_CYBRAKDEMONELECTRICBARRIER4
	{SPR_ELEC, 2 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER6}, // S_CYBRAKDEMONELECTRICBARRIER5
	{SPR_ELEC, 2 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER7}, // S_CYBRAKDEMONELECTRICBARRIER6
	{SPR_ELEC, 3 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER8}, // S_CYBRAKDEMONELECTRICBARRIER7
	{SPR_ELEC, 3 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER9}, // S_CYBRAKDEMONELECTRICBARRIER8
	{SPR_ELEC, 4 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER10}, // S_CYBRAKDEMONELECTRICBARRIER9
	{SPR_ELEC, 4 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER11}, // S_CYBRAKDEMONELECTRICBARRIER10
	{SPR_ELEC, 5 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER12}, // S_CYBRAKDEMONELECTRICBARRIER11
	{SPR_ELEC, 5 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER13}, // S_CYBRAKDEMONELECTRICBARRIER12
	{SPR_ELEC, 6 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER14}, // S_CYBRAKDEMONELECTRICBARRIER13
	{SPR_ELEC, 6 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER15}, // S_CYBRAKDEMONELECTRICBARRIER14
	{SPR_ELEC, 7 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER16}, // S_CYBRAKDEMONELECTRICBARRIER15
	{SPR_ELEC, 7 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER17}, // S_CYBRAKDEMONELECTRICBARRIER16
	{SPR_ELEC, 8 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER18}, // S_CYBRAKDEMONELECTRICBARRIER17
	{SPR_ELEC, 8 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER19}, // S_CYBRAKDEMONELECTRICBARRIER18
	{SPR_ELEC, 9 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER20}, // S_CYBRAKDEMONELECTRICBARRIER19
	{SPR_ELEC, 9 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER21}, // S_CYBRAKDEMONELECTRICBARRIER20
	{SPR_ELEC, 10 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER22}, // S_CYBRAKDEMONELECTRICBARRIER21
	{SPR_ELEC, 10 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER23}, // S_CYBRAKDEMONELECTRICBARRIER22
	{SPR_ELEC, 11 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER24}, // S_CYBRAKDEMONELECTRICBARRIER23
	{SPR_ELEC, 11 + FF_FULLBRIGHT, 1, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER_PLAYSOUND}, // S_CYBRAKDEMONELECTRICBARRIER24
	{SPR_NULL, 0, 0, {A_RemoteAction}, -1, S_CYBRAKDEMON_DEINVINCIBLERIZE, S_CYBRAKDEMONELECTRICBARRIER_DIE2}, // S_CYBRAKDEMONELECTRICBARRIER_DIE1
	{SPR_NULL, 0, 0, {A_SetObjectFlags}, MF_PUSHABLE|MF_FIRE|MF_PAIN, 1, S_CYBRAKDEMONELECTRICBARRIER_DIE3}, // S_CYBRAKDEMONELECTRICBARRIER_DIE2
	{SPR_NULL, 0, 20*TICRATE, {A_Scream}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMSUCCESS}, // S_CYBRAKDEMONELECTRICBARRIER_DIE3
	{SPR_NULL, 0, 0, {A_CheckRandom}, 10, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMSUCCESS, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMFAIL}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMCHECK,
	{SPR_NULL, 0, 0, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMCHOOSE}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMSUCCESS,
	{SPR_NULL, 0, 0, {A_RandomStateRange}, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM12, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM1}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMCHOOSE,
	{SPR_ELEC, 0 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM1,
	{SPR_ELEC, 1 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM2,
	{SPR_ELEC, 2 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM3,
	{SPR_ELEC, 3 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM4,
	{SPR_ELEC, 4 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM5,
	{SPR_ELEC, 5 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM6,
	{SPR_ELEC, 6 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM7,
	{SPR_ELEC, 7 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM8,
	{SPR_ELEC, 8 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM9,
	{SPR_ELEC, 9 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM10,
	{SPR_ELEC, 10 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM11,
	{SPR_ELEC, 11 + FF_FULLBRIGHT, 1, {A_PlaySound}, sfx_s3k5c, 1, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM12,
	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMFAIL,
	{SPR_NULL, 0, 0, {A_Repeat}, 5*TICRATE, S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMCHECK, S_CYBRAKDEMONELECTRICBARRIER_REVIVE1}, // S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP,
	{SPR_NULL, 0, 0, {A_CapeChase}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER_REVIVE2}, // S_CYBRAKDEMONELECTRICBARRIER_REVIVE1
	{SPR_NULL, 0, 0, {A_SpawnFreshCopy}, 0, 0, S_CYBRAKDEMONELECTRICBARRIER_REVIVE3}, // S_CYBRAKDEMONELECTRICBARRIER_REVIVE2
	{SPR_NULL, 0, TICRATE, {A_PlaySound}, sfx_s3k79, 0, S_NULL}, // S_CYBRAKDEMONELECTRICBARRIER_REVIVE3

	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT  , 1, {A_VileFire}, sfx_s3k9d, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE2}, // S_CYBRAKDEMONTARGETRETICULE1
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|6, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE3}, // S_CYBRAKDEMONTARGETRETICULE2
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|1, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE4}, // S_CYBRAKDEMONTARGETRETICULE3
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|6, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE5}, // S_CYBRAKDEMONTARGETRETICULE4
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|2, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE6}, // S_CYBRAKDEMONTARGETRETICULE5
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|6, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE7}, // S_CYBRAKDEMONTARGETRETICULE6
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|3, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE8}, // S_CYBRAKDEMONTARGETRETICULE7
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|6, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE9}, // S_CYBRAKDEMONTARGETRETICULE8
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|4, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE10}, // S_CYBRAKDEMONTARGETRETICULE9
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|6, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE11}, // S_CYBRAKDEMONTARGETRETICULE10
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|5, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE12}, // S_CYBRAKDEMONTARGETRETICULE11
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|6, 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE13}, // S_CYBRAKDEMONTARGETRETICULE12
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT  , 1, {A_VileFire}, 0, MT_CYBRAKDEMON_TARGET_DOT, S_CYBRAKDEMONTARGETRETICULE14}, // S_CYBRAKDEMONTARGETRETICULE13
	{SPR_TARG, FF_TRANS50|FF_FULLBRIGHT|6, 1, {A_Repeat}, 6, S_CYBRAKDEMONTARGETRETICULE2, S_NULL}, // S_CYBRAKDEMONTARGETRETICULE14

	{SPR_HOOP, FF_TRANS50|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL}, // S_CYBRAKDEMONTARGETDOT

	{SPR_NPLM, 0, 2, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMBOMBLARGE_FLY2}, //S_CYBRAKDEMONNAPALMBOMBLARGE_FLY1,
	{SPR_NPLM, 1, 2, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMBOMBLARGE_FLY3}, //S_CYBRAKDEMONNAPALMBOMBLARGE_FLY2,
	{SPR_NPLM, 2, 2, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMBOMBLARGE_FLY4}, //S_CYBRAKDEMONNAPALMBOMBLARGE_FLY3,
	{SPR_NPLM, 3, 2, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMBOMBLARGE_FLY1}, //S_CYBRAKDEMONNAPALMBOMBLARGE_FLY4,
	{SPR_NPLM, 0, 1, {A_Explode}, 0, 0, S_CYBRAKDEMONNAPALMBOMBLARGE_DIE2}, //S_CYBRAKDEMONNAPALMBOMBLARGE_DIE1, // Explode
	{SPR_NPLM, 0, 1, {A_NapalmScatter}, MT_CYBRAKDEMON_NAPALM_BOMB_SMALL + (6<<16), 256 + (48<<16), S_CYBRAKDEMONNAPALMBOMBLARGE_DIE3}, //S_CYBRAKDEMONNAPALMBOMBLARGE_DIE2, // Outer ring
	{SPR_NPLM, 0, 1, {A_NapalmScatter}, MT_CYBRAKDEMON_NAPALM_BOMB_SMALL + (1<<16), 32<<16, S_CYBRAKDEMONNAPALMBOMBLARGE_DIE4}, //S_CYBRAKDEMONNAPALMBOMBLARGE_DIE3, // Center
	{SPR_NULL, 0, 81, {A_Scream}, 0, 0, S_NULL}, //S_CYBRAKDEMONNAPALMBOMBLARGE_DIE4, // Sound

	{SPR_MNPL, 0, 1, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMBOMBSMALL}, //S_CYBRAKDEMONNAPALMBOMBSMALL,
	{SPR_MNPL, 0, 1, {A_Explode}, 0, 0, S_CYBRAKDEMONNAPALMBOMBSMALL_DIE2}, //S_CYBRAKDEMONNAPALMBOMBSMALL_DIE1, // Explode
	{SPR_MNPL, 0, 1, {A_NapalmScatter}, MT_CYBRAKDEMON_NAPALM_FLAMES + (12<<16), 128 + (40<<16), S_CYBRAKDEMONNAPALMBOMBSMALL_DIE3}, //S_CYBRAKDEMONNAPALMBOMBSMALL_DIE2, // Outer ring
	{SPR_MNPL, 0, 1, {A_NapalmScatter}, MT_CYBRAKDEMON_NAPALM_FLAMES + (8<<16), 64 + (32<<16), S_CYBRAKDEMONNAPALMBOMBSMALL_DIE4}, //S_CYBRAKDEMONNAPALMBOMBSMALL_DIE3, // Inner ring
	{SPR_MNPL, 0, 1, {A_NapalmScatter}, MT_CYBRAKDEMON_NAPALM_FLAMES + (1<<16), 24<<16, S_CYBRAKDEMONNAPALMBOMBSMALL_DIE5}, //S_CYBRAKDEMONNAPALMBOMBSMALL_DIE4, // Center
	{SPR_NULL, 0, 24, {A_Scream}, 0, 0, S_NULL}, //S_CYBRAKDEMONNAPALMBOMBSMALL_DIE5, // Sound

	{SPR_SFLM, FF_FULLBRIGHT,   2, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMFLAME_FLY2}, //S_CYBRAKDEMONNAPALMFLAME_FLY1,
	{SPR_SFLM, FF_FULLBRIGHT|1, 2, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMFLAME_FLY3}, //S_CYBRAKDEMONNAPALMFLAME_FLY2,
	{SPR_SFLM, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMFLAME_FLY4}, //S_CYBRAKDEMONNAPALMFLAME_FLY3,
	{SPR_SFLM, FF_FULLBRIGHT|3, 2, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMFLAME_FLY5}, //S_CYBRAKDEMONNAPALMFLAME_FLY4,
	{SPR_SFLM, FF_FULLBRIGHT|4, 2, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMFLAME_FLY6}, //S_CYBRAKDEMONNAPALMFLAME_FLY5,
	{SPR_SFLM, FF_FULLBRIGHT|5, 2, {NULL}, 0, 0, S_CYBRAKDEMONNAPALMFLAME_FLY1}, //S_CYBRAKDEMONNAPALMFLAME_FLY6,
	{SPR_SFLM, FF_FULLBRIGHT,   0, {A_SpawnObjectRelative}, 0, MT_CYBRAKDEMON_FLAMEREST, S_NULL}, //S_CYBRAKDEMONNAPALMFLAME_DIE,

	{SPR_NULL, 0, 1, {A_SetFuse}, TICRATE, 0, S_CYBRAKDEMONVILEEXPLOSION2}, //S_CYBRAKDEMONVILEEXPLOSION1,
	{SPR_NULL, 0, 0, {A_ScoreRise}, 0, 0, S_CYBRAKDEMONVILEEXPLOSION3}, //S_CYBRAKDEMONVILEEXPLOSION2,
	{SPR_NULL, 0, 1, {A_BossScream}, 0, 0, S_CYBRAKDEMONVILEEXPLOSION1}, //S_CYBRAKDEMONVILEEXPLOSION3,

	// Metal Sonic
	{SPR_PLAY, SPR2_STND, -1, {NULL}, 0, 0, S_METALSONIC_RACE}, // S_METALSONIC_RACE

	{SPR_METL,  4, -1, {NULL},         0, 0, S_NULL},             // S_METALSONIC_FLOAT
	{SPR_METL, 16|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_METALSONIC_STUN}, // S_METALSONIC_VECTOR
	{SPR_METL, 15, -1, {NULL},         0, 0, S_METALSONIC_FLOAT}, // S_METALSONIC_STUN
	{SPR_METL, 17, 20, {NULL},         0, 0, S_METALSONIC_GATHER},// S_METALSONIC_RAISE
	{SPR_METL, 18, -1, {NULL},         0, 0, S_NULL},             // S_METALSONIC_GATHER
	{SPR_METL,  6|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 1, 2, S_METALSONIC_BOUNCE},// S_METALSONIC_DASH
	{SPR_METL, 18|FF_FULLBRIGHT|FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 1, 2, S_NULL},             // S_METALSONIC_BOUNCE
	{SPR_METL, 14, -1, {NULL},         0, 0, S_NULL},             // S_METALSONIC_BADBOUNCE
	{SPR_METL, 17, -1, {NULL},         0, 0, S_METALSONIC_GATHER},// S_METALSONIC_SHOOT
	{SPR_METL, 15, 40, {A_Pain},       0, 0, S_METALSONIC_FLOAT}, // S_METALSONIC_PAIN
	{SPR_METL, 17,  2, {A_Fall},       0, 0, S_METALSONIC_DEATH2},// S_METALSONIC_DEATH1
	{SPR_METL, 17,  4, {A_BossScream}, 0, 0, S_METALSONIC_DEATH3},// S_METALSONIC_DEATH2
	{SPR_METL, 17,  0, {A_Repeat}, 17, S_METALSONIC_DEATH2, S_METALSONIC_DEATH4}, // S_METALSONIC_DEATH3
	{SPR_METL, 17, -1, {A_BossDeath},  0, 0, S_NULL},             // S_METALSONIC_DEATH4
	{SPR_METL, 15,  1, {A_BossScream},         0, 0, S_METALSONIC_FLEE2}, // S_METALSONIC_FLEE1
	{SPR_METL, 15,  7, {NULL},                 0, 0, S_METALSONIC_FLEE1}, // S_METALSONIC_FLEE2

	{SPR_MSCF, FF_FULLBRIGHT|FF_TRANS30|FF_ANIMATE, -1, {NULL}, 11, 1, S_NULL},  // S_MSSHIELD_F1
	{SPR_MSCF, FF_FULLBRIGHT|FF_ANIMATE|12, -1, {NULL}, 8, 2, S_NULL},  // S_MSSHIELD_F2

	// Ring
	{SPR_RING, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 23, 1, S_RING}, // S_RING

	// Blue Sphere for special stages
	{SPR_SPHR, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_BLUESPHERE
	{SPR_SPHR, FF_FULLBRIGHT
#ifdef MANIASPHERES
							|FF_ANIMATE|FF_RANDOMANIM
#endif
													, -1, {NULL}, 1, 4, S_NULL}, // S_BLUESPHEREBONUS
	{SPR_SPHR, 0, 20, {NULL}, 0, 0, S_NULL}, // S_BLUESPHERESPARK

	// Bomb Sphere
	{SPR_SPHR, FF_FULLBRIGHT|3, 2, {NULL}, 0, 0, S_BOMBSPHERE2}, // S_BOMBSPHERE1
	{SPR_SPHR, FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_BOMBSPHERE3}, // S_BOMBSPHERE2
	{SPR_SPHR, FF_FULLBRIGHT|5, 2, {NULL}, 0, 0, S_BOMBSPHERE4}, // S_BOMBSPHERE3
	{SPR_SPHR, FF_FULLBRIGHT|4, 1, {NULL}, 0, 0, S_BOMBSPHERE1}, // S_BOMBSPHERE4

	// NiGHTS Chip
	{SPR_NCHP, FF_FULLBRIGHT|FF_ANIMATE,    -1, {NULL}, 15, 2, S_NULL}, // S_NIGHTSCHIP
	{SPR_NCHP, FF_FULLBRIGHT|FF_ANIMATE|16, -1, {NULL}, 15, 2, S_NULL}, // S_NIGHTSCHIPBONUS

	// NiGHTS Star
	{SPR_NSTR, FF_ANIMATE, -1, {NULL}, 14, 2, S_NULL}, // S_NIGHTSSTAR
	{SPR_NSTR, 15, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSSTARXMAS

	// Gravity Well sprites for Egg Rock's Special Stage
	{SPR_GWLG, FF_ANIMATE, -1, {NULL}, 2, 1, S_NULL}, // S_GRAVWELLGREEN
	{SPR_GWLR, FF_ANIMATE, -1, {NULL}, 2, 1, S_NULL}, // S_GRAVWELLRED

	// Individual Team Rings (now with shield attracting action! =P)
	{SPR_TRNG, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 23, 1, S_TEAMRING},  // S_TEAMRING

	// Special Stage Token
	{SPR_TOKE, FF_ANIMATE|FF_FULLBRIGHT, -1, {NULL}, 19, 1, S_TOKEN}, // S_TOKEN

	// CTF Flags
	{SPR_RFLG, 0, -1, {NULL}, 0, 0, S_NULL}, // S_REDFLAG
	{SPR_BFLG, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BLUEFLAG

	// Emblem
	{SPR_EMBM,  0, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM1
	{SPR_EMBM,  1, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM2
	{SPR_EMBM,  2, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM3
	{SPR_EMBM,  3, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM4
	{SPR_EMBM,  4, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM5
	{SPR_EMBM,  5, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM6
	{SPR_EMBM,  6, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM7
	{SPR_EMBM,  7, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM8
	{SPR_EMBM,  8, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM9
	{SPR_EMBM,  9, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM10
	{SPR_EMBM, 10, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM11
	{SPR_EMBM, 11, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM12
	{SPR_EMBM, 12, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM13
	{SPR_EMBM, 13, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM14
	{SPR_EMBM, 14, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM15
	{SPR_EMBM, 15, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM16
	{SPR_EMBM, 16, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM17
	{SPR_EMBM, 17, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM18
	{SPR_EMBM, 18, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM19
	{SPR_EMBM, 19, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM20
	{SPR_EMBM, 20, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM21
	{SPR_EMBM, 21, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM22
	{SPR_EMBM, 22, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM23
	{SPR_EMBM, 23, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM24
	{SPR_EMBM, 24, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM25
	{SPR_EMBM, 25, -1, {NULL}, 0, 0, S_NULL}, // S_EMBLEM26

	// Chaos Emeralds
	{SPR_CEMG, FF_FULLBRIGHT,   -1, {NULL}, 0, 0, S_NULL}, // S_CEMG1
	{SPR_CEMG, FF_FULLBRIGHT|1, -1, {NULL}, 0, 0, S_NULL}, // S_CEMG2
	{SPR_CEMG, FF_FULLBRIGHT|2, -1, {NULL}, 0, 0, S_NULL}, // S_CEMG3
	{SPR_CEMG, FF_FULLBRIGHT|3, -1, {NULL}, 0, 0, S_NULL}, // S_CEMG4
	{SPR_CEMG, FF_FULLBRIGHT|4, -1, {NULL}, 0, 0, S_NULL}, // S_CEMG5
	{SPR_CEMG, FF_FULLBRIGHT|5, -1, {NULL}, 0, 0, S_NULL}, // S_CEMG6
	{SPR_CEMG, FF_FULLBRIGHT|6, -1, {NULL}, 0, 0, S_NULL}, // S_CEMG7

	// Emerald hunt shards
	{SPR_SHRD, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SHRD1
	{SPR_SHRD, 1, -1, {NULL}, 0, 0, S_NULL}, // S_SHRD2
	{SPR_SHRD, 2, -1, {NULL}, 0, 0, S_NULL}, // S_SHRD3

	// Bubble Source
	{SPR_BBLS, 0, 8, {A_BubbleSpawn}, 2048, 0, S_BUBBLES2}, // S_BUBBLES1
	{SPR_BBLS, 1, 8, {A_BubbleCheck}, 0, 0, S_BUBBLES3}, // S_BUBBLES2
	{SPR_BBLS, 2, 8, {A_BubbleSpawn}, 2048, 0, S_BUBBLES4}, // S_BUBBLES3
	{SPR_BBLS, 3, 8, {A_BubbleCheck}, 0, 0, S_BUBBLES1}, // S_BUBBLES4

	// Level End Sign
	{SPR_SIGN,                 0, -1, {A_SignPlayer}, -3, 0, S_NULL},                // S_SIGN
	{SPR_SIGN,                 0,  1,   {A_SignSpin}, 30, 0, S_SIGNSPIN2},           // S_SIGNSPIN1
	{SPR_SIGN,                 0,  0,     {A_Repeat},  4, S_SIGNSPIN1, S_SIGNSPIN3}, // S_SIGNSPIN2
	{SPR_SIGN,                 0,  0, {A_SignPlayer}, -2, 0, S_SIGNSPIN4},           // S_SIGNSPIN3
	{SPR_SIGN,                 0,  1,   {A_SignSpin}, 30, 0, S_SIGNSPIN5},           // S_SIGNSPIN4
	{SPR_SIGN,                 0,  0,     {A_Repeat},  4, S_SIGNSPIN4, S_SIGNSPIN6}, // S_SIGNSPIN5
	{SPR_SIGN,                 0,  0, {A_SignPlayer}, -3, 0, S_SIGNSPIN1},           // S_SIGNSPIN6
	{SPR_SIGN,                 0,  1, {A_SignPlayer}, -1, 0, S_SIGNSLOW},            // S_SIGNPLAYER
	{SPR_SIGN,                 0,  1,   {A_SignSpin}, 30, 0, S_SIGNSLOW},            // S_SIGNSLOW
	{SPR_SIGN,                 0, -1,         {NULL},  0, 0, S_NULL},                // S_SIGNSTOP
	{SPR_SIGN, FF_PAPERSPRITE| 2, -1,         {NULL},  0, 0, S_NULL},                // S_SIGNBOARD
	{SPR_SIGN, FF_PAPERSPRITE| 1, -1,         {NULL},  0, 29, S_NULL},               // S_EGGMANSIGN
	{SPR_SIGN, FF_PAPERSPRITE|18, -1,         {NULL},  0, 29, S_NULL},               // S_CLEARSIGN

	// Spike Ball
	{SPR_SPIK, 0, 1, {NULL}, 0, 0, S_SPIKEBALL2}, // S_SPIKEBALL1
	{SPR_SPIK, 1, 1, {NULL}, 0, 0, S_SPIKEBALL3}, // S_SPIKEBALL2
	{SPR_SPIK, 2, 1, {NULL}, 0, 0, S_SPIKEBALL4}, // S_SPIKEBALL3
	{SPR_SPIK, 3, 1, {NULL}, 0, 0, S_SPIKEBALL5}, // S_SPIKEBALL4
	{SPR_SPIK, 4, 1, {NULL}, 0, 0, S_SPIKEBALL6}, // S_SPIKEBALL5
	{SPR_SPIK, 5, 1, {NULL}, 0, 0, S_SPIKEBALL7}, // S_SPIKEBALL6
	{SPR_SPIK, 6, 1, {NULL}, 0, 0, S_SPIKEBALL8}, // S_SPIKEBALL7
	{SPR_SPIK, 7, 1, {NULL}, 0, 0, S_SPIKEBALL1}, // S_SPIKEBALL8

	// Elemental Shield's Spawn
	{SPR_SFLM, FF_FULLBRIGHT,   2, {NULL}, 0, 0, S_SPINFIRE2}, // S_SPINFIRE1
	{SPR_SFLM, FF_FULLBRIGHT|1, 2, {NULL}, 0, 0, S_SPINFIRE3}, // S_SPINFIRE2
	{SPR_SFLM, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_SPINFIRE4}, // S_SPINFIRE3
	{SPR_SFLM, FF_FULLBRIGHT|3, 2, {NULL}, 0, 0, S_SPINFIRE5}, // S_SPINFIRE4
	{SPR_SFLM, FF_FULLBRIGHT|4, 2, {NULL}, 0, 0, S_SPINFIRE6}, // S_SPINFIRE5
	{SPR_SFLM, FF_FULLBRIGHT|5, 2, {NULL}, 0, 0, S_SPINFIRE1}, // S_SPINFIRE6

	// Floor Spike
	{SPR_USPK, 0,-1, {A_SpikeRetract}, 1, 0, S_SPIKE2}, // S_SPIKE1 -- Fully extended
	{SPR_USPK, 1, 2, {A_Pain},         0, 0, S_SPIKE3}, // S_SPIKE2
	{SPR_USPK, 2, 2, {NULL},           0, 0, S_SPIKE4}, // S_SPIKE3
	{SPR_USPK, 3,-1, {A_SpikeRetract}, 0, 0, S_SPIKE5}, // S_SPIKE4 -- Fully retracted
	{SPR_USPK, 2, 2, {A_Pain},         0, 0, S_SPIKE6}, // S_SPIKE5
	{SPR_USPK, 1, 2, {NULL},           0, 0, S_SPIKE1}, // S_SPIKE6
	{SPR_USPK, 4,-1, {NULL}, 0, 0, S_NULL}, // S_SPIKED1 -- Busted spike particles
	{SPR_USPK, 5,-1, {NULL}, 0, 0, S_NULL}, // S_SPIKED2

	// Wall Spike
	{SPR_WSPK, 0|FF_PAPERSPRITE,-1, {A_SpikeRetract}, 1, 0, S_WALLSPIKE2}, // S_WALLSPIKE1 -- Fully extended
	{SPR_WSPK, 1|FF_PAPERSPRITE, 2, {A_Pain},         0, 0, S_WALLSPIKE3}, // S_WALLSPIKE2
	{SPR_WSPK, 2|FF_PAPERSPRITE, 2, {NULL},           0, 0, S_WALLSPIKE4}, // S_WALLSPIKE3
	{SPR_WSPK, 3|FF_PAPERSPRITE,-1, {A_SpikeRetract}, 0, 0, S_WALLSPIKE5}, // S_WALLSPIKE4 -- Fully retracted
	{SPR_WSPK, 2|FF_PAPERSPRITE, 2, {A_Pain},         0, 0, S_WALLSPIKE6}, // S_WALLSPIKE5
	{SPR_WSPK, 1|FF_PAPERSPRITE, 2, {NULL},           0, 0, S_WALLSPIKE1}, // S_WALLSPIKE6
	{SPR_WSPB, 0|FF_PAPERSPRITE,-1, {NULL}, 0, 0, S_NULL}, // S_WALLSPIKEBASE -- Base
	{SPR_WSPK, 4,-1, {NULL}, 0, 0, S_NULL}, // S_WALLSPIKED1 -- Busted spike particles
	{SPR_WSPK, 5,-1, {NULL}, 0, 0, S_NULL}, // S_WALLSPIKED2

	// Starpost
	{SPR_STPT, 0            , -1, {NULL},  0, 0, S_NULL},           // S_STARPOST_IDLE
	{SPR_STPT, FF_ANIMATE|17, -1, {NULL},  5, 1, S_NULL},           // S_STARPOST_FLASH
	{SPR_STPT, FF_ANIMATE|13,  2, {NULL},  1, 1, S_STARPOST_SPIN},  // S_STARPOST_STARTSPIN
	{SPR_STPT, FF_ANIMATE|1 , 23, {NULL}, 11, 1, S_STARPOST_ENDSPIN}, // S_STARPOST_SPIN
	{SPR_STPT, FF_ANIMATE|15,  2, {NULL},  1, 1, S_STARPOST_FLASH}, // S_STARPOST_ENDSPIN

	// Big floating mine
	{SPR_BMNE, 0,  2, {A_Look},      ((224<<FRACBITS)|1), 0, S_BIGMINE_IDLE},   // S_BIGMINE_IDLE
	{SPR_BMNE, 1,  2, {A_MineRange}, 112,                 0, S_BIGMINE_ALERT2}, // S_BIGMINE_ALERT1
	{SPR_BMNE, 2,  2, {A_MineRange}, 112,                 0, S_BIGMINE_ALERT3}, // S_BIGMINE_ALERT2
	{SPR_BMNE, 0,  1, {A_Look},      ((224<<FRACBITS)|1), 1, S_BIGMINE_IDLE},   // S_BIGMINE_ALERT3
	{SPR_BMNE, 3, 25, {A_Pain},           0,            0, S_BIGMINE_SET2},   // S_BIGMINE_SET1
	{SPR_BMNE, 3, 10, {A_SetObjectFlags}, MF_SHOOTABLE, 1, S_BIGMINE_SET3},   // S_BIGMINE_SET1
	{SPR_BMNE, 3,  1, {A_MineExplode},    0,            0, S_BIGMINE_BLAST1}, // S_BIGMINE_SET3
	{SPR_BMNB,   FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_BIGMINE_BLAST2}, // S_BIGMINE_BLAST1
	{SPR_BMNB, 1|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_BIGMINE_BLAST3}, // S_BIGMINE_BLAST2
	{SPR_BMNB, 2|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_BIGMINE_BLAST4}, // S_BIGMINE_BLAST3
	{SPR_BMNB, 3|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_BIGMINE_BLAST5}, // S_BIGMINE_BLAST4
	{SPR_NULL, 0, 35, {NULL}, 0, 0, S_NULL}, // S_BIGMINE_BLAST5

	// Cannon launcher
	{SPR_NULL, 0, 1,    {A_FindTarget},     MT_PLAYER,         0, S_CANNONLAUNCHER2}, // S_CANNONLAUNCHER1
	{SPR_NULL, 0, 1,       {A_LobShot}, MT_CANNONBALL, 4*TICRATE, S_CANNONLAUNCHER3}, // S_CANNONLAUNCHER2
	{SPR_NULL, 0, 2, {A_SetRandomTics},     TICRATE/2, 3*TICRATE, S_CANNONLAUNCHER1}, // S_CANNONLAUNCHER3

	// Monitor Miscellany
	{SPR_NSPK, 0, 16, {NULL}, 0, 0, S_BOXSPARKLE2}, // S_BOXSPARKLE1
	{SPR_NSPK, 1, 12, {NULL}, 0, 0, S_BOXSPARKLE3}, // S_BOXSPARKLE2
	{SPR_NSPK, 2,  8, {NULL}, 0, 0, S_BOXSPARKLE4}, // S_BOXSPARKLE3
	{SPR_NSPK, 3,  4, {NULL}, 0, 0, S_NULL},        // S_BOXSPARKLE4

	{SPR_MSTV, 0,  1, {NULL}, 0, 0, S_SPAWNSTATE},  // S_BOX_FLICKER
	{SPR_MSTV, 0,  4, {A_MonitorPop}, 0, 0, S_BOX_POP2}, // S_BOX_POP1
	{SPR_MSTV, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BOX_POP2

	{SPR_XLTV, 0,  1, {NULL}, 0, 0, S_SPAWNSTATE},  // S_GOLDBOX_FLICKER
	{SPR_XLTV, 1, 89, {A_GoldMonitorPop}, 0, 0, S_GOLDBOX_OFF2}, // S_GOLDBOX_OFF1
	{SPR_XLTV, 2,  4, {A_PlayAttackSound}, 0, 0, S_GOLDBOX_OFF3}, // S_GOLDBOX_OFF2
	{SPR_XLTV, 3,  4, {NULL}, 0, 0, S_GOLDBOX_OFF4}, // S_GOLDBOX_OFF3
	{SPR_XLTV, 4,  4, {NULL}, 0, 0, S_GOLDBOX_OFF5}, // S_GOLDBOX_OFF4
	{SPR_XLTV, 5,  2, {NULL}, 0, 0, S_GOLDBOX_OFF6}, // S_GOLDBOX_OFF5
	{SPR_XLTV, 6,  2, {NULL}, 0, 0, S_GOLDBOX_OFF7}, // S_GOLDBOX_OFF6
	{SPR_XLTV, 6,  0, {A_GoldMonitorRestore}, 0, 0, S_SPAWNSTATE}, // S_GOLDBOX_OFF7

	// Monitor States (one per box)
	{SPR_TVMY, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_MYSTERY_BOX
	{SPR_TVRI, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_RING_BOX
	{SPR_TVPI, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_PITY_BOX
	{SPR_TVAT, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_ATTRACT_BOX
	{SPR_TVFO, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_FORCE_BOX
	{SPR_TVAR, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_ARMAGEDDON_BOX
	{SPR_TVWW, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_WHIRLWIND_BOX
	{SPR_TVEL, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_ELEMENTAL_BOX
	{SPR_TVSS, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_SNEAKERS_BOX
	{SPR_TVIV, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_INVULN_BOX
	{SPR_TV1U, 0, 2, {A_1upThinker}, 0, 0, S_BOX_FLICKER}, // S_1UP_BOX
	{SPR_TVEG, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_EGGMAN_BOX
	{SPR_TVMX, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_MIXUP_BOX
	{SPR_TVGV, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_GRAVITY_BOX
	{SPR_TVRC, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_RECYCLER_BOX
	{SPR_TV1K, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_SCORE1K_BOX
	{SPR_TVTK, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_SCORE10K_BOX
	{SPR_TVFL, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_FLAMEAURA_BOX
	{SPR_TVBB, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_BUBBLEWRAP_BOX
	{SPR_TVZP, 0, 2, {NULL}, 0, 0, S_BOX_FLICKER}, // S_THUNDERCOIN_BOX

	// Gold Repeat Monitor States (one per box)
	{SPR_TVPI, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_PITY_GOLDBOX
	{SPR_TVAT, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_ATTRACT_GOLDBOX
	{SPR_TVFO, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_FORCE_GOLDBOX
	{SPR_TVAR, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_ARMAGEDDON_GOLDBOX
	{SPR_TVWW, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_WHIRLWIND_GOLDBOX
	{SPR_TVEL, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_ELEMENTAL_GOLDBOX
	{SPR_TVSS, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_SNEAKERS_GOLDBOX
	{SPR_TVIV, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_INVULN_GOLDBOX
	{SPR_TVEG, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_EGGMAN_GOLDBOX
	{SPR_TVGV, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_GRAVITY_GOLDBOX
	{SPR_TVFL, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_FLAMEAURA_GOLDBOX
	{SPR_TVBB, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_BUBBLEWRAP_GOLDBOX
	{SPR_TVZP, 1, 2, {A_GoldMonitorSparkle}, 0, 0, S_GOLDBOX_FLICKER}, // S_THUNDERCOIN_GOLDBOX

	// Team Ring Boxes (these are special)
	{SPR_TRRI, 0, 2, {NULL}, 0, 0, S_RING_REDBOX2}, // S_RING_REDBOX1
	{SPR_TRRI, 1, 1, {NULL}, 0, 0, S_RING_REDBOX1}, // S_RING_REDBOX2
	{SPR_TRRI, 1, 4, {A_MonitorPop}, 0, 0, S_REDBOX_POP2},  // S_REDBOX_POP1
	{SPR_TRRI, 2, -1, {NULL}, 0, 0, S_NULL},        // S_REDBOX_POP2

	{SPR_TBRI, 0, 2, {NULL}, 0, 0, S_RING_BLUEBOX2}, // S_RING_BLUEBOX1
	{SPR_TBRI, 1, 1, {NULL}, 0, 0, S_RING_BLUEBOX1}, // S_RING_BLUEBOX2
	{SPR_TBRI, 1, 4, {A_MonitorPop}, 0, 0, S_BLUEBOX_POP2},  // S_BLUEBOX_POP1
	{SPR_TBRI, 2, -1, {NULL}, 0, 0, S_NULL},         // S_BLUEBOX_POP2

	// Box Icons -- 2 states each, animation and action
	{SPR_TVRI, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_RING_ICON2}, // S_RING_ICON1
	{SPR_TVRI, 2, 18, {A_RingBox}, 0, 0, S_NULL}, // S_RING_ICON2

	{SPR_TVPI, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_PITY_ICON2}, // S_PITY_ICON1
	{SPR_TVPI, 2, 18, {A_GiveShield}, SH_PITY, 0, S_NULL},  // S_PITY_ICON2

	{SPR_TVAT, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_ATTRACT_ICON2}, // S_ATTRACT_ICON1
	{SPR_TVAT, 2, 18, {A_GiveShield}, SH_ATTRACT, 0, S_NULL}, // S_ATTRACT_ICON2

	{SPR_TVFO, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_FORCE_ICON2}, // S_FORCE_ICON1
	{SPR_TVFO, 2, 18, {A_GiveShield}, SH_FORCE|1, 0, S_NULL}, // S_FORCE_ICON2

	{SPR_TVAR, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_ARMAGEDDON_ICON2}, // S_ARMAGEDDON_ICON1
	{SPR_TVAR, 2, 18, {A_GiveShield}, SH_ARMAGEDDON, 0, S_NULL}, // S_ARMAGEDDON_ICON2

	{SPR_TVWW, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_WHIRLWIND_ICON2}, // S_WHIRLWIND_ICON1
	{SPR_TVWW, 2, 18, {A_GiveShield}, SH_WHIRLWIND, 0, S_NULL}, // S_WHIRLWIND_ICON2

	{SPR_TVEL, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_ELEMENTAL_ICON2}, // S_ELEMENTAL_ICON1
	{SPR_TVEL, 2, 18, {A_GiveShield}, SH_ELEMENTAL, 0, S_NULL}, // S_ELEMENTAL_ICON2

	{SPR_TVSS, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_SNEAKERS_ICON2}, // S_SNEAKERS_ICON1
	{SPR_TVSS, 2, 18, {A_SuperSneakers}, 0, 0, S_NULL}, // S_SNEAKERS_ICON2

	{SPR_TVIV, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_INVULN_ICON2}, // S_INVULN_ICON1
	{SPR_TVIV, 2, 18, {A_Invincibility}, 0, 0, S_NULL}, // S_INVULN_ICON2

	{SPR_TV1U, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_1UP_ICON2}, // S_1UP_ICON1
	{SPR_TV1U, 2, 18, {A_ExtraLife},  0, 0, S_NULL},  // S_1UP_ICON2

	{SPR_TVEG, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_EGGMAN_ICON2}, // S_EGGMAN_ICON1
	{SPR_TVEG, 2, 18, {A_EggmanBox}, 0, 0, S_NULL}, // S_EGGMAN_ICON2

	{SPR_TVMX, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_MIXUP_ICON2}, // S_MIXUP_ICON1
	{SPR_TVMX, 2, 18, {A_MixUp}, 0, 0, S_NULL}, // S_MIXUP_ICON2

	{SPR_TVGV, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_GRAVITY_ICON2}, // S_GRAVITY_ICON1
	{SPR_TVGV, 2, 18, {A_GravityBox}, 0, 0, S_NULL}, // S_GRAVITY_ICON2

	{SPR_TVRC, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_RECYCLER_ICON2}, // S_RECYCLER_ICON1
	{SPR_TVRC, 2, 18, {A_RecyclePowers}, 0, 0, S_NULL}, // S_RECYCLER_ICON2

	{SPR_TV1K, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_SCORE1K_ICON2}, // S_SCORE1K_ICON1
	{SPR_TV1K, 2, 18, {A_AwardScore}, 0, 0, S_NULL}, // S_SCORE1K_ICON2

	{SPR_TVTK, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_SCORE10K_ICON2}, // S_SCORE10K_ICON1
	{SPR_TVTK, 2, 18, {A_AwardScore}, 0, 0, S_NULL}, // S_SCORE10K_ICON2

	{SPR_TVFL, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_FLAMEAURA_ICON2}, // S_FLAMEAURA_ICON1
	{SPR_TVFL, 2, 18, {A_GiveShield}, SH_FLAMEAURA, 0, S_NULL}, // S_FLAMEAURA_ICON2

	{SPR_TVBB, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_BUBBLEWRAP_ICON2}, // S_BUBBLEWRAP_ICON1
	{SPR_TVBB, 2, 18, {A_GiveShield}, SH_BUBBLEWRAP, 0, S_NULL}, // S_BUBBLERWAP_ICON2

	{SPR_TVZP, FF_ANIMATE|2, 18, {NULL}, 3, 4, S_THUNDERCOIN_ICON2}, // S_THUNDERCOIN_ICON1
	{SPR_TVZP, 2, 18, {A_GiveShield}, SH_THUNDERCOIN, 0, S_NULL}, // S_THUNDERCOIN_ICON2

	// ---

	{SPR_MISL, FF_FULLBRIGHT, 1, {A_SmokeTrailer}, MT_SMOKE, 0, S_ROCKET}, // S_ROCKET

	{SPR_LASR, FF_FULLBRIGHT|0, 2, {NULL}, 0, 0, S_NULL}, // S_LASER
	{SPR_LASR, FF_FULLBRIGHT|1, 2, {NULL}, 0, 0, S_NULL}, // S_LASER2
	{SPR_LASR, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_NULL}, // S_LASERFLASH

	{SPR_LASF,                           FF_FULLBRIGHT|0,       2,           {NULL}, 0, 0, S_LASERFLAME2}, // S_LASERFLAME1
	{SPR_LASF,                           FF_FULLBRIGHT|1,       1, {A_ChangeHeight}, 156*FRACUNIT, 3, S_LASERFLAME3}, // S_LASERFLAME2
	{SPR_LASF,                           FF_FULLBRIGHT|2,       0, {A_ChangeHeight}, 32*FRACUNIT, 3, S_LASERFLAME4}, // S_LASERFLAME3
	{SPR_LASF, FF_ANIMATE|FF_PAPERSPRITE|FF_FULLBRIGHT|2,       4,           {NULL}, 1, 2, S_LASERFLAME5}, // S_LASERFLAME4
	{SPR_LASF, FF_ANIMATE|FF_PAPERSPRITE|FF_FULLBRIGHT|4,      28,           {NULL}, 2, 2, S_NULL}, // S_LASERFLAME5

	{SPR_TORP, 0, 1, {A_SmokeTrailer}, MT_SMOKE, 0, S_TORPEDO}, // S_TORPEDO

	{SPR_ENRG, FF_FULLBRIGHT|FF_TRANS30, 1, {NULL}, 0, 0, S_ENERGYBALL2}, // S_ENERGYBALL1
	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_ENERGYBALL1}, // S_ENERGYBALL2

	// Skim Mine (also dropped by Jetty-Syn bomber)
	{SPR_MINE, 0, -1, {NULL}, 0, 0, S_NULL},           // S_MINE1
	{SPR_MINE, 1, 1, {A_Fall}, 0, 0, S_MINE_BOOM2},    // S_MINE_BOOM1
	{SPR_MINE, 2, 3, {A_Scream}, 0, 0, S_MINE_BOOM3},  // S_MINE_BOOM2
	{SPR_MINE, 3, 3, {A_Explode}, 0, 0, S_MINE_BOOM4}, // S_MINE_BOOM3
	{SPR_MINE, 4, 3, {NULL}, 0, 0, S_NULL},            // S_MINE_BOOM4

	// Jetty-Syn Bullet
	{SPR_JBUL, FF_FULLBRIGHT,   1, {NULL}, 0, 0, S_JETBULLET2}, // S_JETBULLET1
	{SPR_JBUL, FF_FULLBRIGHT|1, 1, {NULL}, 0, 0, S_JETBULLET1}, // S_JETBULLET2

	{SPR_TRLS, FF_FULLBRIGHT,   1, {NULL}, 0, 0, S_TURRETLASER},          // S_TURRETLASER
	{SPR_TRLS, FF_FULLBRIGHT|1, 2, {NULL}, 0, 0, S_TURRETLASEREXPLODE2},  // S_TURRETLASEREXPLODE1
	{SPR_TRLS, FF_FULLBRIGHT|2, 2, {NULL}, 0, 0, S_NULL},                 // S_TURRETLASEREXPLODE2

	{SPR_CBLL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CANNONBALL1

	{SPR_AROW, 0, -1, {NULL}, 0, 0, S_NULL}, // S_ARROW
	{SPR_AROW, FF_ANIMATE, TICRATE, {A_ArrowBonks}, 7, 2, S_NULL}, // S_ARROWBONK

	{SPR_CFIR, FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 5, 2, S_NULL}, // S_DEMONFIRE

	{SPR_LETR, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_LETTER

	// GFZ flowers
	{SPR_FWR1, FF_ANIMATE, -1, {NULL},  7, 3, S_NULL}, // S_GFZFLOWERA
	{SPR_FWR2, FF_ANIMATE, -1, {NULL}, 19, 3, S_NULL}, // S_GFZFLOWERB
	{SPR_FWR3, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_GFZFLOWERC

	{SPR_BUS3, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BLUEBERRYBUSH
	{SPR_BUS1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BERRYBUSH
	{SPR_BUS2, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BUSH

	// Trees
	{SPR_TRE1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_GFZTREE
	{SPR_TRE1, 1, -1, {NULL}, 0, 0, S_NULL}, // S_GFZBERRYTREE
	{SPR_TRE1, 2, -1, {NULL}, 0, 0, S_NULL}, // S_GFZCHERRYTREE
	{SPR_TRE2, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CHECKERTREE
	{SPR_TRE2, 1, -1, {NULL}, 0, 0, S_NULL}, // S_CHECKERSUNSETTREE
	{SPR_TRE3, 0, -1, {NULL}, 0, 0, S_NULL}, // S_FHZTREE
	{SPR_TRE3, 1, -1, {NULL}, 0, 0, S_NULL}, // S_FHZPINKTREE
	{SPR_TRE4, 0, -1, {NULL}, 0, 0, S_NULL}, // S_POLYGONTREE
	{SPR_TRE5, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BUSHTREE
	{SPR_TRE5, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BUSHREDTREE
	{SPR_TRE6, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SPRINGTREE

	// THZ flowers
	{SPR_THZP, FF_ANIMATE, -1, {NULL},  7, 4, S_NULL}, // S_THZFLOWERA
	{SPR_FWR5, FF_ANIMATE, -1, {NULL}, 19, 2, S_NULL}, // S_THZFLOWERB
	{SPR_FWR6, FF_ANIMATE, -1, {NULL}, 19, 2, S_NULL}, // S_THZFLOWERC

	// THZ Steam Whistle tree/bush
	{SPR_THZT, 0, -1, {NULL}, 0, 0, S_NULL}, // S_THZTREE
	{SPR_THZT,  1|FF_PAPERSPRITE, 40, {NULL}, 0, 0, S_THZTREEBRANCH2}, // S_THZTREEBRANCH1
	{SPR_THZT,  2|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH3}, // S_THZTREEBRANCH2
	{SPR_THZT,  3|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH4}, // S_THZTREEBRANCH3
	{SPR_THZT,  4|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH5}, // S_THZTREEBRANCH4
	{SPR_THZT,  5|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH6}, // S_THZTREEBRANCH5
	{SPR_THZT,  6|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH7}, // S_THZTREEBRANCH6
	{SPR_THZT,  7|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH8}, // S_THZTREEBRANCH7
	{SPR_THZT,  8|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH9}, // S_THZTREEBRANCH8
	{SPR_THZT,  9|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH10}, // S_THZTREEBRANCH9
	{SPR_THZT, 10|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH11}, // S_THZTREEBRANCH10
	{SPR_THZT, 11|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH12}, // S_THZTREEBRANCH11
	{SPR_THZT, 12|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH13}, // S_THZTREEBRANCH12
	{SPR_THZT, 13|FF_PAPERSPRITE,  4, {NULL}, 0, 0, S_THZTREEBRANCH1}, // S_THZTREEBRANCH13

	// THZ Alarm
	{SPR_ALRM, FF_FULLBRIGHT, 35, {A_Scream}, 0, 0, S_ALARM1}, // S_ALARM1

	// Deep Sea Gargoyle
	{SPR_GARG, 0, -1, {NULL}, 0, 0, S_NULL},  // S_GARGOYLE
	{SPR_GARG, 1, -1, {NULL}, 0, 0, S_NULL},  // S_BIGGARGOYLE

	// DSZ Seaweed
	{SPR_SEWE, 0, -1, {NULL}, 0, 0, S_SEAWEED2}, // S_SEAWEED1
	{SPR_SEWE, 1, 5, {NULL}, 0, 0, S_SEAWEED3}, // S_SEAWEED2
	{SPR_SEWE, 2, 5, {NULL}, 0, 0, S_SEAWEED4}, // S_SEAWEED3
	{SPR_SEWE, 3, 5, {NULL}, 0, 0, S_SEAWEED5}, // S_SEAWEED4
	{SPR_SEWE, 4, 5, {NULL}, 0, 0, S_SEAWEED6}, // S_SEAWEED5
	{SPR_SEWE, 5, 5, {NULL}, 0, 0, S_SEAWEED1}, // S_SEAWEED6

	// Dripping water
	{SPR_NULL, FF_TRANS30  , 3*TICRATE, {NULL},                  0, 0, S_DRIPA2}, // S_DRIPA1
	{SPR_DRIP, FF_TRANS30  ,         2, {NULL},                  0, 0, S_DRIPA3}, // S_DRIPA2
	{SPR_DRIP, FF_TRANS30|1,         2, {NULL},                  0, 0, S_DRIPA4}, // S_DRIPA3
	{SPR_DRIP, FF_TRANS30|2,         2, {A_SpawnObjectRelative}, 0, MT_WATERDROP, S_DRIPA1}, // S_DRIPA4
	{SPR_DRIP, FF_TRANS30|3,        -1, {NULL},                  0, 0, S_DRIPB1}, // S_DRIPB1
	{SPR_DRIP, FF_TRANS30|4,         1, {NULL},                  0, 0, S_DRIPC2}, // S_DRIPC1
	{SPR_DRIP, FF_TRANS30|5,         1, {NULL},                  0, 0,   S_NULL}, // S_DRIPC2

	// Coral
	{SPR_CORL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CORAL1
	{SPR_CORL, 1, -1, {NULL}, 0, 0, S_NULL}, // S_CORAL2
	{SPR_CORL, 2, -1, {NULL}, 0, 0, S_NULL}, // S_CORAL3
	{SPR_CORL, 3, -1, {NULL}, 0, 0, S_NULL}, // S_CORAL4
	{SPR_CORL, 4, -1, {NULL}, 0, 0, S_NULL}, // S_CORAL5

	// Blue Crystal
	{SPR_BCRY, FF_TRANS30, -1, {NULL}, 0, 0, S_NULL}, // S_BLUECRYSTAL1

	// Kelp
	{SPR_KELP, 0, -1, {NULL}, 0, 0, S_NULL}, // S_KELP

	// Animated algae
	{SPR_ALGA, 0, 1, {A_ConnectToGround}, MT_ANIMALGAESEG, 0, S_ANIMALGAETOP2}, // S_ANIMALGAETOP1
	{SPR_ALGA, 0|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 11, 4, S_NULL},          // S_ANIMALGAETOP2
	{SPR_ALGB, 0|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 11, 4, S_NULL},          // S_ANIMALGAESEG

	// DSZ Stalagmites
	{SPR_DSTG, 0, -1, {NULL}, 0, 0, S_NULL}, // S_DSZSTALAGMITE
	{SPR_DSTG, 1, -1, {NULL}, 0, 0, S_NULL}, // S_DSZ2STALAGMITE

	// DSZ Light beam
	{SPR_LIBE, 0|FF_TRANS80|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {A_LightBeamReset}, 0, 0, S_LIGHTBEAM2}, // S_LIGHTBEAM1
	{SPR_LIBE, 0|FF_TRANS70|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {NULL}, 0, 0, S_LIGHTBEAM3},  // S_LIGHTBEAM2
	{SPR_LIBE, 0|FF_TRANS60|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {NULL}, 0, 0, S_LIGHTBEAM4},  // S_LIGHTBEAM3
	{SPR_LIBE, 0|FF_TRANS50|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {NULL}, 0, 0, S_LIGHTBEAM5},  // S_LIGHTBEAM4
	{SPR_LIBE, 0|FF_TRANS40|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {NULL}, 0, 0, S_LIGHTBEAM6},  // S_LIGHTBEAM5
	{SPR_LIBE, 0|FF_TRANS30|FF_FULLBRIGHT|FF_PAPERSPRITE, 9, {NULL}, 0, 0, S_LIGHTBEAM7},  // S_LIGHTBEAM6
	{SPR_LIBE, 0|FF_TRANS40|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {NULL}, 0, 0, S_LIGHTBEAM8},  // S_LIGHTBEAM7
	{SPR_LIBE, 0|FF_TRANS50|FF_FULLBRIGHT|FF_PAPERSPRITE, 2, {NULL}, 0, 0, S_LIGHTBEAM9},  // S_LIGHTBEAM8
	{SPR_LIBE, 0|FF_TRANS60|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {NULL}, 0, 0, S_LIGHTBEAM10}, // S_LIGHTBEAM9
	{SPR_LIBE, 0|FF_TRANS70|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {NULL}, 0, 0, S_LIGHTBEAM11}, // S_LIGHTBEAM10
	{SPR_LIBE, 0|FF_TRANS80|FF_FULLBRIGHT|FF_PAPERSPRITE, 4, {NULL}, 0, 0, S_LIGHTBEAM12}, // S_LIGHTBEAM11
	{SPR_NULL, 0, 2, {A_SetRandomTics}, 4, 35, S_LIGHTBEAM1}, // S_LIGHTBEAM12

	// CEZ Chain
	{SPR_CHAN, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CEZCHAIN

	// Flame
	{SPR_FLAM, FF_FULLBRIGHT|FF_ANIMATE,       3*8, {A_FlameParticle}, 7, 3, S_FLAME}, // S_FLAME
	{SPR_FLAM, FF_FULLBRIGHT|FF_ANIMATE|8, TICRATE,            {NULL}, 3, 3, S_NULL},  // S_FLAMEPARTICLE
	{SPR_FLAM, FF_FULLBRIGHT|FF_ANIMATE,        -1,            {NULL}, 7, 3, S_NULL},  // S_FLAMEREST

	// Eggman statue
	{SPR_ESTA, 0, -1, {NULL}, 0, 0, S_NULL}, // S_EGGSTATUE1

	// Hidden sling appears
	{SPR_NULL, 0, -1, {NULL},          0, 0, S_SLING2}, // S_SLING1
	{SPR_NULL, 0, -1, {A_SlingAppear}, 0, 0, S_NULL},   // S_SLING2

	// CEZ maces and chains
	{SPR_SMCH, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SMALLMACECHAIN
	{SPR_BMCH, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BIGMACECHAIN
	{SPR_SMCE, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SMALLMACE
	{SPR_BMCE, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BIGMACE
	{SPR_SMCH, 1, -1, {NULL}, 0, 0, S_NULL}, // S_SMALLGRABCHAIN
	{SPR_BMCH, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BIGGRABCHAIN

	// Yellow spring on a ball
	{SPR_YSPB, 0, -1, {NULL},   0, 0, S_NULL},              // S_YELLOWSPRINGBALL
	{SPR_YSPB, 4,  4, {A_Pain}, 0, 0, S_YELLOWSPRINGBALL3}, // S_YELLOWSPRINGBALL2
	{SPR_YSPB, 3,  1, {NULL},   0, 0, S_YELLOWSPRINGBALL4}, // S_YELLOWSPRINGBALL3
	{SPR_YSPB, 2,  1, {NULL},   0, 0, S_YELLOWSPRINGBALL5}, // S_YELLOWSPRINGBALL4
	{SPR_YSPB, 1,  1, {NULL},   0, 0, S_YELLOWSPRINGBALL},  // S_YELLOWSPRINGBALL5

	// Red spring on a ball
	{SPR_RSPB, 0, -1, {NULL},   0, 0, S_NULL},           // S_REDSPRINGBALL
	{SPR_RSPB, 4,  4, {A_Pain}, 0, 0, S_REDSPRINGBALL3}, // S_REDSPRINGBALL2
	{SPR_RSPB, 3,  1, {NULL},   0, 0, S_REDSPRINGBALL4}, // S_REDSPRINGBALL3
	{SPR_RSPB, 2,  1, {NULL},   0, 0, S_REDSPRINGBALL5}, // S_REDSPRINGBALL4
	{SPR_RSPB, 1,  1, {NULL},   0, 0, S_REDSPRINGBALL},  // S_REDSPRINGBALL5

	// Small Firebar
	{SPR_SFBR, FF_FULLBRIGHT,     1, {NULL},            0, 0, S_SMALLFIREBAR2},  // S_SMALLFIREBAR1
	{SPR_SFBR, FF_FULLBRIGHT| 1,  1, {NULL},            0, 0, S_SMALLFIREBAR3},  // S_SMALLFIREBAR2
	{SPR_SFBR, FF_FULLBRIGHT| 2,  1, {A_FlameParticle}, 0, 0, S_SMALLFIREBAR4},  // S_SMALLFIREBAR3
	{SPR_SFBR, FF_FULLBRIGHT| 3,  1, {NULL},            0, 0, S_SMALLFIREBAR5},  // S_SMALLFIREBAR4
	{SPR_SFBR, FF_FULLBRIGHT| 4,  1, {NULL},            0, 0, S_SMALLFIREBAR6},  // S_SMALLFIREBAR5
	{SPR_SFBR, FF_FULLBRIGHT| 5,  1, {NULL},            0, 0, S_SMALLFIREBAR7},  // S_SMALLFIREBAR6
	{SPR_SFBR, FF_FULLBRIGHT| 6,  1, {A_FlameParticle}, 0, 0, S_SMALLFIREBAR8},  // S_SMALLFIREBAR7
	{SPR_SFBR, FF_FULLBRIGHT| 7,  1, {NULL},            0, 0, S_SMALLFIREBAR9},  // S_SMALLFIREBAR8
	{SPR_SFBR, FF_FULLBRIGHT| 8,  1, {NULL},            0, 0, S_SMALLFIREBAR10}, // S_SMALLFIREBAR9
	{SPR_SFBR, FF_FULLBRIGHT| 9,  1, {NULL},            0, 0, S_SMALLFIREBAR11}, // S_SMALLFIREBAR10
	{SPR_SFBR, FF_FULLBRIGHT|10,  1, {A_FlameParticle}, 0, 0, S_SMALLFIREBAR12}, // S_SMALLFIREBAR11
	{SPR_SFBR, FF_FULLBRIGHT|11,  1, {NULL},            0, 0, S_SMALLFIREBAR13}, // S_SMALLFIREBAR12
	{SPR_SFBR, FF_FULLBRIGHT|12,  1, {NULL},            0, 0, S_SMALLFIREBAR14}, // S_SMALLFIREBAR13
	{SPR_SFBR, FF_FULLBRIGHT|13,  1, {NULL},            0, 0, S_SMALLFIREBAR15}, // S_SMALLFIREBAR14
	{SPR_SFBR, FF_FULLBRIGHT|14,  1, {A_FlameParticle}, 0, 0, S_SMALLFIREBAR16}, // S_SMALLFIREBAR15
	{SPR_SFBR, FF_FULLBRIGHT|15,  1, {NULL},            0, 0, S_SMALLFIREBAR1},  // S_SMALLFIREBAR16

	// Big Firebar
	{SPR_BFBR, FF_FULLBRIGHT,     1, {NULL},            0, 0, S_BIGFIREBAR2},  // S_BIGFIREBAR1
	{SPR_BFBR, FF_FULLBRIGHT| 1,  1, {NULL},            0, 0, S_BIGFIREBAR3},  // S_BIGFIREBAR2
	{SPR_BFBR, FF_FULLBRIGHT| 2,  1, {A_FlameParticle}, 0, 0, S_BIGFIREBAR4},  // S_BIGFIREBAR3
	{SPR_BFBR, FF_FULLBRIGHT| 3,  1, {NULL},            0, 0, S_BIGFIREBAR5},  // S_BIGFIREBAR4
	{SPR_BFBR, FF_FULLBRIGHT| 4,  1, {NULL},            0, 0, S_BIGFIREBAR6},  // S_BIGFIREBAR5
	{SPR_BFBR, FF_FULLBRIGHT| 5,  1, {NULL},            0, 0, S_BIGFIREBAR7},  // S_BIGFIREBAR6
	{SPR_BFBR, FF_FULLBRIGHT| 6,  1, {A_FlameParticle}, 0, 0, S_BIGFIREBAR8},  // S_BIGFIREBAR7
	{SPR_BFBR, FF_FULLBRIGHT| 7,  1, {NULL},            0, 0, S_BIGFIREBAR9},  // S_BIGFIREBAR8
	{SPR_BFBR, FF_FULLBRIGHT| 8,  1, {NULL},            0, 0, S_BIGFIREBAR10}, // S_BIGFIREBAR9
	{SPR_BFBR, FF_FULLBRIGHT| 9,  1, {NULL},            0, 0, S_BIGFIREBAR11}, // S_BIGFIREBAR10
	{SPR_BFBR, FF_FULLBRIGHT|10,  1, {A_FlameParticle}, 0, 0, S_BIGFIREBAR12}, // S_BIGFIREBAR11
	{SPR_BFBR, FF_FULLBRIGHT|11,  1, {NULL},            0, 0, S_BIGFIREBAR13}, // S_BIGFIREBAR12
	{SPR_BFBR, FF_FULLBRIGHT|12,  1, {NULL},            0, 0, S_BIGFIREBAR14}, // S_BIGFIREBAR13
	{SPR_BFBR, FF_FULLBRIGHT|13,  1, {NULL},            0, 0, S_BIGFIREBAR15}, // S_BIGFIREBAR14
	{SPR_BFBR, FF_FULLBRIGHT|14,  1, {A_FlameParticle}, 0, 0, S_BIGFIREBAR16}, // S_BIGFIREBAR15
	{SPR_BFBR, FF_FULLBRIGHT|15,  1, {NULL},            0, 0, S_BIGFIREBAR1},  // S_BIGFIREBAR16

	{SPR_FWR4, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CEZFLOWER
	{SPR_BANR, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CEZPOLE

	{SPR_BANR, FF_PAPERSPRITE|1, -1, {NULL}, 0, 0, S_NULL}, // S_CEZBANNER1
	{SPR_BANR, FF_PAPERSPRITE|2, -1, {NULL}, 0, 0, S_NULL}, // S_CEZBANNER2

	{SPR_PINE, 0, -1, {NULL}, 0, 0, S_NULL}, // S_PINETREE
	{SPR_CEZB, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CEZBUSH1
	{SPR_CEZB, 1, -1, {NULL}, 0, 0, S_NULL}, // S_CEZBUSH2

	{SPR_CNDL, FF_FULLBRIGHT,   -1, {NULL}, 0, 0, S_NULL}, // S_CANDLE
	{SPR_CNDL, FF_FULLBRIGHT|1, -1, {NULL}, 0, 0, S_NULL}, // S_CANDLEPRICKET

	{SPR_FLMH, 0, -1, {NULL}, 0, 0, S_NULL}, // S_FLAMEHOLDER

	{SPR_CTRC, FF_FULLBRIGHT|FF_ANIMATE, 8*3, {A_FlameParticle}, 3, 3, S_FIRETORCH}, // S_FIRETORCH

	{SPR_CFLG,                0, -1, {NULL}, 0, 0, S_NULL}, // S_WAVINGFLAG
	{SPR_CFLG, FF_PAPERSPRITE|1, -1, {NULL}, 0, 0, S_NULL}, // S_WAVINGFLAGSEG1
	{SPR_CFLG, FF_PAPERSPRITE|2, -1, {NULL}, 0, 0, S_NULL}, // S_WAVINGFLAGSEG2

	{SPR_CSTA, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CRAWLASTATUE

	{SPR_CBBS, 0, -1, {NULL}, 0, 0, S_NULL}, // S_FACESTABBERSTATUE

	{SPR_CBBS, 0, 5, {A_Look}, 768*FRACUNIT, 0, S_SUSPICIOUSFACESTABBERSTATUE_WAIT},   // S_SUSPICIOUSFACESTABBERSTATUE_WAIT
	{SPR_CBBS, FF_ANIMATE, 23, {NULL},    6, 1, S_SUSPICIOUSFACESTABBERSTATUE_BURST2}, // S_SUSPICIOUSFACESTABBERSTATUE_BURST1
	{SPR_NULL, 0, 40, {A_StatueBurst}, MT_FACESTABBER, S_FACESTABBER_CHARGE2, S_NULL}, // S_SUSPICIOUSFACESTABBERSTATUE_BURST2

	{SPR_CABR, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BRAMBLES

	// Big Tumbleweed
	{SPR_BTBL, 0, -1, {NULL}, 0, 0, S_NULL},                // S_BIGTUMBLEWEED
	{SPR_BTBL, 0,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL2}, // S_BIGTUMBLEWEED_ROLL1
	{SPR_BTBL, 1,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL3}, // S_BIGTUMBLEWEED_ROLL2
	{SPR_BTBL, 2,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL4}, // S_BIGTUMBLEWEED_ROLL3
	{SPR_BTBL, 3,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL5}, // S_BIGTUMBLEWEED_ROLL4
	{SPR_BTBL, 4,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL6}, // S_BIGTUMBLEWEED_ROLL5
	{SPR_BTBL, 5,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL7}, // S_BIGTUMBLEWEED_ROLL6
	{SPR_BTBL, 6,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL8}, // S_BIGTUMBLEWEED_ROLL7
	{SPR_BTBL, 7,  5, {NULL}, 0, 0, S_BIGTUMBLEWEED_ROLL1}, // S_BIGTUMBLEWEED_ROLL8

	// Little Tumbleweed
	{SPR_STBL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_LITTLETUMBLEWEED
	{SPR_STBL, 0, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL2}, // S_LITTLETUMBLEWEED_ROLL1
	{SPR_STBL, 1, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL3}, // S_LITTLETUMBLEWEED_ROLL2
	{SPR_STBL, 2, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL4}, // S_LITTLETUMBLEWEED_ROLL3
	{SPR_STBL, 3, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL5}, // S_LITTLETUMBLEWEED_ROLL4
	{SPR_STBL, 4, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL6}, // S_LITTLETUMBLEWEED_ROLL5
	{SPR_STBL, 5, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL7}, // S_LITTLETUMBLEWEED_ROLL6
	{SPR_STBL, 6, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL8}, // S_LITTLETUMBLEWEED_ROLL7
	{SPR_STBL, 7, 5, {NULL}, 0, 0, S_LITTLETUMBLEWEED_ROLL1}, // S_LITTLETUMBLEWEED_ROLL8

	// Cacti
	{SPR_CACT, 0, -1, {A_ConnectToGround}, MT_CACTITINYSEG, 0, S_NULL}, // S_CACTI1
	{SPR_CACT, 1, -1, {A_ConnectToGround}, MT_CACTISMALLSEG, 0, S_NULL}, // S_CACTI2
	{SPR_CACT, 2, -1, {A_ConnectToGround}, MT_CACTITINYSEG, 0, S_NULL}, // S_CACTI3
	{SPR_CACT, 3, -1, {A_ConnectToGround}, MT_CACTISMALLSEG, 0, S_NULL}, // S_CACTI4
	{SPR_CACT, 4, -1, {NULL}, 0, 0, S_NULL}, // S_CACTI5
	{SPR_CACT, 5, -1, {NULL}, 0, 0, S_NULL}, // S_CACTI6
	{SPR_CACT, 6, -1, {NULL}, 0, 0, S_NULL}, // S_CACTI7
	{SPR_CACT, 7, -1, {NULL}, 0, 0, S_NULL}, // S_CACTI8
	{SPR_CACT, 8, -1, {NULL}, 0, 0, S_NULL}, // S_CACTI9
	{SPR_CACT, 9, -1, {A_ConnectToGround}, MT_CACTITINYSEG, 0, S_NULL}, // S_CACTI10
	{SPR_CACT, 10, -1, {A_ConnectToGround}, MT_CACTISMALLSEG, 0, S_NULL}, // S_CACTI11
	{SPR_CACT, 11, -1, {NULL}, 0, 0, S_NULL}, // S_CACTITINYSEG
	{SPR_CACT, 12, -1, {NULL}, 0, 0, S_NULL}, // S_CACTISMALLSEG

	// Warning Signs
	{SPR_WWSG, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_ARIDSIGN_CAUTION
	{SPR_WWS2, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_ARIDSIGN_CACTI
	{SPR_WWS3, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_ARIDSIGN_SHARPTURN

	// Oil lamp
	{SPR_OILL, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_OILLAMP
	{SPR_OILF, FF_TRANS90|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_OILLAMPFLARE

	// TNT barrel
	{SPR_BARR, 0, -1, {NULL}, 0, 0, S_NULL}, // S_TNTBARREL_STND1
	{SPR_BARX, 0, 0, {A_RollAngle}, 0, 1, S_TNTBARREL_EXPL2}, // S_TNTBARREL_EXPL1
	{SPR_BARX, 0|FF_FULLBRIGHT, 3, {A_SetObjectFlags}, MF_NOCLIP|MF_NOGRAVITY|MF_NOBLOCKMAP, 0, S_TNTBARREL_EXPL3}, // S_TNTBARREL_EXPL2
	{SPR_BARX, 1|FF_FULLBRIGHT, 2, {A_TNTExplode}, MT_TNTDUST, 0, S_TNTBARREL_EXPL4}, // S_TNTBARREL_EXPL3
	{SPR_BARX, 1|FF_FULLBRIGHT, 1, {NULL}, 0, 0, S_TNTBARREL_EXPL5}, // S_TNTBARREL_EXPL4
	{SPR_BARX, 2|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_TNTBARREL_EXPL6}, // S_TNTBARREL_EXPL5
	{SPR_BARX, 3|FF_FULLBRIGHT, 3, {NULL}, 0, 0, S_TNTBARREL_EXPL7}, // S_TNTBARREL_EXPL6
	{SPR_NULL, 0, 35, {NULL}, 0, 0, S_NULL}, // S_TNTBARREL_EXPL7
#ifndef ROTSPRITE
	{SPR_BARR, 1|FF_ANIMATE, -1, {NULL}, 7, 2, S_NULL}, // S_TNTBARREL_FLYING
#else
	{SPR_BARR, 1, 1, {A_RollAngle}, 14, 0, S_TNTBARREL_FLYING}, // S_TNTBARREL_FLYING
#endif

	// TNT proximity shell
	{SPR_REMT, 0, 10, {A_Look}, 33554433, 0, S_PROXIMITY_TNT}, // S_PROXIMITY_TNT
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER2}, // S_PROXIMITY_TNT_TRIGGER1
	{SPR_REMT, 0, 16, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER3}, // S_PROXIMITY_TNT_TRIGGER2
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER4}, // S_PROXIMITY_TNT_TRIGGER3
	{SPR_REMT, 0, 16, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER5}, // S_PROXIMITY_TNT_TRIGGER4
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER6}, // S_PROXIMITY_TNT_TRIGGER5
	{SPR_REMT, 0, 4, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER7}, // S_PROXIMITY_TNT_TRIGGER6
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER8}, // S_PROXIMITY_TNT_TRIGGER7
	{SPR_REMT, 0, 4, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER9}, // S_PROXIMITY_TNT_TRIGGER8
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER10}, // S_PROXIMITY_TNT_TRIGGER9
	{SPR_REMT, 0, 4, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER11}, // S_PROXIMITY_TNT_TRIGGER10
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER12}, // S_PROXIMITY_TNT_TRIGGER11
	{SPR_REMT, 0, 4, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER13}, // S_PROXIMITY_TNT_TRIGGER12
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER14}, // S_PROXIMITY_TNT_TRIGGER13
	{SPR_REMT, 0, 2, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER15}, // S_PROXIMITY_TNT_TRIGGER14
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER16}, // S_PROXIMITY_TNT_TRIGGER15
	{SPR_REMT, 0, 2, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER17}, // S_PROXIMITY_TNT_TRIGGER16
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER18}, // S_PROXIMITY_TNT_TRIGGER17
	{SPR_REMT, 0, 2, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER19}, // S_PROXIMITY_TNT_TRIGGER18
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER20}, // S_PROXIMITY_TNT_TRIGGER19
	{SPR_REMT, 0, 2, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER21}, // S_PROXIMITY_TNT_TRIGGER20
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_PROXIMITY_TNT_TRIGGER22}, // S_PROXIMITY_TNT_TRIGGER21
	{SPR_REMT, 0, 2, {NULL}, 0, 0, S_PROXIMITY_TNT_TRIGGER23}, // S_PROXIMITY_TNT_TRIGGER22
	{SPR_REMT, 1|FF_FULLBRIGHT, 1, {A_PlayActiveSound}, 0, 0, S_TNTBARREL_EXPL1}, // S_PROXIMITY_TNT_TRIGGER23

	// Dust devil
	{SPR_NULL, 0, 1, {A_DustDevilThink}, 0, 0, S_DUSTDEVIL}, //S_DUSTDEVIL
	{SPR_TAZD, 1|FF_PAPERSPRITE|FF_TRANS70, 2 * TICRATE, {NULL}, 0, 0, S_DUSTLAYER2}, // S_DUSTLAYER1
	{SPR_TAZD, 1|FF_PAPERSPRITE|FF_TRANS70, 5, {NULL}, 0, 0, S_DUSTLAYER3}, // S_DUSTLAYER2
	{SPR_TAZD, 1|FF_PAPERSPRITE|FF_TRANS80, 5, {NULL}, 0, 0, S_DUSTLAYER4}, // S_DUSTLAYER3
	{SPR_TAZD, 1|FF_PAPERSPRITE|FF_TRANS80, 5, {NULL}, 0, 0, S_DUSTLAYER5}, // S_DUSTLAYER4
	{SPR_TAZD, 1|FF_PAPERSPRITE|FF_TRANS90, 5, {NULL}, 0, 0, S_NULL}, // S_DUSTLAYER5
	{SPR_ADST, 0|FF_ANIMATE, 24, {NULL}, 3, 8, S_NULL}, // S_ARIDDUST1
	{SPR_ADST, 3|FF_ANIMATE, 24, {NULL}, 3, 8, S_NULL}, // S_ARIDDUST2
	{SPR_ADST, 6|FF_ANIMATE, 24, {NULL}, 3, 8, S_NULL}, // S_ARIDDUST3

	// Minecart
	{SPR_NULL, 0,                            1, {NULL},                 0, 0, S_MINECART_IDLE},   // S_MINECART_IDLE
	{SPR_NULL, 0,                            0, {A_KillSegments},       0, 0, S_TNTBARREL_EXPL4}, // S_MINECART_DTH1
	{SPR_MCRT, 8|FF_PAPERSPRITE,            -1, {NULL},                 0, 0, S_NULL},            // S_MINECARTEND
	{SPR_MCRT, 0|FF_PAPERSPRITE,            -1, {NULL},                 0, 0, S_NULL},            // S_MINECARTSEG_FRONT
	{SPR_MCRT, 1|FF_PAPERSPRITE,            -1, {NULL},                 0, 0, S_NULL},            // S_MINECARTSEG_BACK
	{SPR_MCRT, 2|FF_PAPERSPRITE|FF_ANIMATE, -1, {NULL},                 2, 3, S_NULL},            // S_MINECARTSEG_LEFT
	{SPR_MCRT, 5|FF_PAPERSPRITE|FF_ANIMATE, -1, {NULL},                 2, 3, S_NULL},            // S_MINECARTSEG_RIGHT
	{SPR_LCKN, 2|FF_FULLBRIGHT,              2, {NULL},                 0, 0, S_NULL},            // S_MINECARTSIDEMARK1
	{SPR_LCKN, 0|FF_FULLBRIGHT,              2, {NULL},                 0, 0, S_NULL},            // S_MINECARTSIDEMARK2
	{SPR_MCSP, FF_FULLBRIGHT,                1, {A_MinecartSparkThink}, 0, 0, S_MINECARTSPARK},   // S_MINECARTSPARK

	// Saloon door
	{SPR_SALD, 0|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_SALOONDOOR
	{SPR_NULL, 0, -1, {A_SaloonDoorSpawn}, MT_SALOONDOOR, 48, S_NULL}, // S_SALOONDOORCENTER

	// Train cameo
	{SPR_NULL, 0, -1, {NULL}, 0, 0, S_TRAINCAMEOSPAWNER_2}, // S_TRAINCAMEOSPAWNER_1
	{SPR_NULL, 0, 14, {A_TrainCameo}, 20, 18, S_TRAINCAMEOSPAWNER_3}, // S_TRAINCAMEOSPAWNER_2
	{SPR_NULL, 0, 1, {A_Repeat}, 1, 0, S_TRAINCAMEOSPAWNER_4}, // S_TRAINCAMEOSPAWNER_3
	{SPR_NULL, 0, 18, {A_TrainCameo2}, 20, 44, S_TRAINCAMEOSPAWNER_5}, // S_TRAINCAMEOSPAWNER_4
	{SPR_NULL, 0, 1, {A_Repeat}, 5, S_TRAINCAMEOSPAWNER_4, S_NULL}, // S_TRAINCAMEOSPAWNER_5
	{SPR_NULL, 0, 2, {A_SmokeTrailer}, MT_SMOKE, 0, S_TRAINPUFFMAKER}, // S_TRAINPUFFMAKER

	{SPR_ADST, FF_TRANS50, -1, {NULL}, 0, 0, S_NULL}, // S_TRAINDUST
	{SPR_STEA, FF_TRANS50, -1, {NULL}, 0, 0, S_NULL}, // S_TRAINSTEAM

	// Flame jet
	{SPR_NULL, 0, 2*TICRATE, {NULL},             0, 0, S_FLAMEJETSTART}, // S_FLAMEJETSTND
	{SPR_NULL, 0, 3*TICRATE, {A_ToggleFlameJet}, 0, 0,  S_FLAMEJETSTOP}, // S_FLAMEJETSTART
	{SPR_NULL, 0,         1, {A_ToggleFlameJet}, 0, 0,  S_FLAMEJETSTND}, // S_FLAMEJETSTOP
	{SPR_FLME, FF_FULLBRIGHT  ,  4, {NULL}, 0, 0, S_FLAMEJETFLAME2}, // S_FLAMEJETFLAME1
	{SPR_FLME, FF_FULLBRIGHT|1,  5, {NULL}, 0, 0, S_FLAMEJETFLAME3}, // S_FLAMEJETFLAME2
	{SPR_FLME, FF_FULLBRIGHT|2, 11, {NULL}, 0, 0,           S_NULL}, // S_FLAMEJETFLAME3
	{SPR_FLME, FF_FULLBRIGHT|3,  4, {NULL}, 0, 0, S_FLAMEJETFLAME5}, // S_FLAMEJETFLAME4
	{SPR_FLME, FF_FULLBRIGHT|4,  5, {NULL}, 0, 0, S_FLAMEJETFLAME6}, // S_FLAMEJETFLAME5
	{SPR_FLME, FF_FULLBRIGHT|5, 11, {NULL}, 0, 0,           S_NULL}, // S_FLAMEJETFLAME6
	{SPR_FLME, FF_FULLBRIGHT|6,  4, {NULL}, 0, 0, S_FLAMEJETFLAME8}, // S_FLAMEJETFLAME7
	{SPR_FLME, FF_FULLBRIGHT|7,  5, {NULL}, 0, 0, S_FLAMEJETFLAME9}, // S_FLAMEJETFLAME8
	{SPR_FLME, FF_FULLBRIGHT|8, 11, {NULL}, 0, 0,           S_NULL}, // S_FLAMEJETFLAME9

	// Spinning flame jets
	// A: Counter-clockwise
	{SPR_NULL, 0, 1,            {A_TrapShot}, MT_FLAMEJETFLAMEB, -(16<<16)|(1<<15)|64, S_FJSPINAXISA2}, // S_FJSPINAXISA1
	{SPR_NULL, 0, 2, {A_ChangeAngleRelative},                 6,         6, S_FJSPINAXISA1}, // S_FJSPINAXISA2

	// B: Clockwise
	{SPR_NULL, 0, 1,            {A_TrapShot}, MT_FLAMEJETFLAMEB, -(16<<16)|(1<<15)|64, S_FJSPINAXISB2}, // S_FJSPINAXISB1
	{SPR_NULL, 0, 2, {A_ChangeAngleRelative},                -6,        -6, S_FJSPINAXISB1}, // S_FJSPINAXISB2

	// Blade's flame
	{SPR_DFLM, FF_FULLBRIGHT|FF_TRANS40, 1, {A_MoveRelative}, 0, 5, S_FLAMEJETFLAMEB2}, // S_FLAMEJETFLAMEB1
	{SPR_DFLM, FF_FULLBRIGHT|FF_TRANS40, 1, {A_MoveRelative}, 0, 7, S_FLAMEJETFLAMEB3}, // S_FLAMEJETFLAMEB2
	{SPR_DFLM, FF_FULLBRIGHT|FF_TRANS40|FF_ANIMATE, (12*7), {NULL}, 7, 12, S_NULL},  // S_FLAMEJETFLAMEB3

	// Lavafall
	{SPR_LFAL, 5, 1, {NULL}, 0, 0, S_LAVAFALL_DORMANT}, // S_LAVAFALL_DORMANT
	{SPR_LFAL, 6|FF_ANIMATE, 4, {A_LavafallRocks}, 1, 2, S_LAVAFALL_TELL}, // S_LAVAFALL_TELL
	{SPR_LFAL, 9|FF_FULLBRIGHT|FF_ANIMATE, 2, {A_LavafallLava}, 1, 1, S_LAVAFALL_SHOOT}, // S_LAVAFALL_SHOOT
	{SPR_LFAL, FF_FULLBRIGHT, 1, {A_FallingLavaCheck}, 0, 0, S_LAVAFALL_LAVA2}, // S_LAVAFALL_LAVA1
	{SPR_LFAL, FF_FULLBRIGHT, 1, {A_FallingLavaCheck}, 0, 0, S_LAVAFALL_LAVA1}, // S_LAVAFALL_LAVA2
	{SPR_LFAL, 2|FF_FULLBRIGHT|FF_ANIMATE, 9, {NULL}, 2, 3, S_NULL}, // S_LAVAFALL_LAVA3
	{SPR_LFAL, 11|FF_ANIMATE|FF_RANDOMANIM, 12, {NULL}, 3, 3, S_LAVAFALLROCK}, // S_LAVAFALLROCK

	// Rollout Rock
	{SPR_NULL, 0, 1, {A_RolloutSpawn}, 256*FRACUNIT, MT_ROLLOUTROCK, S_ROLLOUTSPAWN}, // S_ROLLOUTSPAWN
	{SPR_PUMI, 0, 1, {A_RolloutRock},    63*FRACUNIT/64,  7*FRACUNIT/10,  S_ROLLOUTROCK}, // S_ROLLOUTROCK

	// RVZ scenery
	{SPR_JPLA, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_BIGFERNLEAF
	{SPR_JPLA, 1, 1, {NULL}, 0, 0, S_BIGFERN2}, // S_BIGFERN1
	{SPR_JPLA, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BIGFERN2
	{SPR_JPLA, 2, -1, {NULL}, 0, 0, S_NULL}, // S_JUNGLEPALM
	{SPR_TFLO, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_TORCHFLOWER}, // S_TORCHFLOWER
	{SPR_WVIN, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_WALLVINE_LONG
	{SPR_WVIN, 1|FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_WALLVINE_SHORT

	// Glaregoyles
	{SPR_BGAR,            0, 22, {NULL},       0, 0, S_GLAREGOYLE_CHARGE},  // S_GLAREGOYLE
	{SPR_BGAR,            2,  6, {NULL},       0, 0, S_GLAREGOYLE_BLINK},  // S_GLAREGOYLE_CHARGE
	{SPR_BGAR, FF_ANIMATE|1, 18, {NULL},       1, 3, S_GLAREGOYLE_HOLD}, // S_GLAREGOYLE_BLINK
	{SPR_BGAR,            1,  9, {NULL},       0, 0, S_GLAREGOYLE_FIRE},  // S_GLAREGOYLE_HOLD
	{SPR_BGAR,            1,  1, {A_TrapShot}, (16<<16)+MT_DEMONFIRE, (30<<16), S_GLAREGOYLE_LOOP},  // S_GLAREGOYLE_FIRE
	{SPR_BGAR,            1,  0, {A_Repeat},   3, S_GLAREGOYLE_FIRE, S_GLAREGOYLE_COOLDOWN}, // S_GLAREGOYLE_LOOP
	{SPR_BGAR, FF_ANIMATE|1, 15, {NULL},       1, 9, S_GLAREGOYLE}, // S_GLAREGOYLE_COOLDOWN

	{SPR_BGAR,            0, 22, {NULL},       0, 0, S_GLAREGOYLEUP_CHARGE},  // S_GLAREGOYLEUP
	{SPR_BGAR,            2,  6, {NULL},       0, 0, S_GLAREGOYLEUP_BLINK},  // S_GLAREGOYLEUP_CHARGE
	{SPR_BGAR, FF_ANIMATE|1, 18, {NULL},       1, 3, S_GLAREGOYLEUP_HOLD}, // S_GLAREGOYLEUP_BLINK
	{SPR_BGAR,            1,  9, {NULL},       0, 0, S_GLAREGOYLEUP_FIRE},  // S_GLAREGOYLEUP_HOLD
	{SPR_BGAR,            1,  1, {A_TrapShot}, (16<<16)+MT_DEMONFIRE, (30<<16)+45, S_GLAREGOYLEUP_LOOP},  // S_GLAREGOYLEUP_FIRE
	{SPR_BGAR,            1,  0, {A_Repeat},   3, S_GLAREGOYLEUP_FIRE, S_GLAREGOYLEUP_COOLDOWN}, // S_GLAREGOYLEUP_LOOP
	{SPR_BGAR, FF_ANIMATE|1, 15, {NULL},       1, 9, S_GLAREGOYLEUP}, // S_GLAREGOYLEUP_COOLDOWN

	{SPR_BGAR,            0, 22, {NULL},       0, 0, S_GLAREGOYLEDOWN_CHARGE},  // S_GLAREGOYLEDOWN
	{SPR_BGAR,            2,  6, {NULL},       0, 0, S_GLAREGOYLEDOWN_BLINK},  // S_GLAREGOYLEDOWN_CHARGE
	{SPR_BGAR, FF_ANIMATE|1, 18, {NULL},       1, 3, S_GLAREGOYLEDOWN_HOLD}, // S_GLAREGOYLEDOWN_BLINK
	{SPR_BGAR,            1,  9, {NULL},       0, 0, S_GLAREGOYLEDOWN_FIRE},  // S_GLAREGOYLEDOWN_HOLD
	{SPR_BGAR,            1,  1, {A_TrapShot}, (16<<16)+MT_DEMONFIRE, (30<<16)+315, S_GLAREGOYLEDOWN_LOOP},  // S_GLAREGOYLEDOWN_FIRE
	{SPR_BGAR,            1,  0, {A_Repeat},   3, S_GLAREGOYLEDOWN_FIRE, S_GLAREGOYLEDOWN_COOLDOWN}, // S_GLAREGOYLEDOWN_LOOP
	{SPR_BGAR, FF_ANIMATE|1, 15, {NULL},       1, 9, S_GLAREGOYLEDOWN}, // S_GLAREGOYLEDOWN_COOLDOWN

	{SPR_BGAR,            0, 90, {NULL},       0, 0, S_GLAREGOYLELONG_CHARGE},  // S_GLAREGOYLELONG
	{SPR_BGAR,            2,  6, {NULL},       0, 0, S_GLAREGOYLELONG_BLINK},  // S_GLAREGOYLELONG_CHARGE
	{SPR_BGAR, FF_ANIMATE|1, 18, {NULL},       1, 3, S_GLAREGOYLELONG_HOLD}, // S_GLAREGOYLELONG_BLINK
	{SPR_BGAR,            1,  9, {NULL},       0, 0, S_GLAREGOYLELONG_FIRE},  // S_GLAREGOYLELONG_HOLD
	{SPR_BGAR,            1,  1, {A_TrapShot}, (16<<16)+MT_DEMONFIRE, (30<<16), S_GLAREGOYLELONG_LOOP},  // S_GLAREGOYLELONG_FIRE
	{SPR_BGAR,            1,  0, {A_Repeat},   5, S_GLAREGOYLELONG_FIRE, S_GLAREGOYLELONG_COOLDOWN}, // S_GLAREGOYLELONG_LOOP
	{SPR_BGAR, FF_ANIMATE|1, 15, {NULL},       1, 9, S_GLAREGOYLELONG}, // S_GLAREGOYLELONG_COOLDOWN

	// Target/Red Crystal
	{SPR_RCRY,               0, -1, {NULL},                  0, 0, S_TARGET_IDLE},  // S_TARGET_IDLE
	{SPR_RCRY, FF_FULLBRIGHT|1,  0, {A_PlaySound},           sfx_ding, 1, S_TARGET_HIT2},  // S_TARGET_HIT1
	{SPR_RCRY, FF_FULLBRIGHT|1, 45, {A_SetObjectFlags},      MF_PUSHABLE, 2, S_TARGET_RESPAWN},  // S_TARGET_HIT2
	{SPR_RCRY,               1,  0, {A_SpawnObjectRelative}, 0, MT_TARGET, S_NULL},  // S_TARGET_RESPAWN
	{SPR_RCRY, FF_FULLBRIGHT|1, -1, {A_SetObjectFlags},      MF_PUSHABLE, 1, S_TARGET_ALLDONE},  // S_TARGET_ALLDONE

	// Green flame
	{SPR_CFLM, FF_FULLBRIGHT|FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 3, S_GREENFLAME}, // S_GREENFLAME

	// ATZ Blue Gargoyle
	{SPR_BGAR, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BLUEGARGOYLE

	// Stalagmites
	{SPR_STLG, 0, -1, {NULL}, 0, 0, S_NULL}, // S_STG0
	{SPR_STLG, 1, -1, {NULL}, 0, 0, S_NULL}, // S_STG1
	{SPR_STLG, 2, -1, {NULL}, 0, 0, S_NULL}, // S_STG2
	{SPR_STLG, 3, -1, {NULL}, 0, 0, S_NULL}, // S_STG3
	{SPR_STLG, 4, -1, {NULL}, 0, 0, S_NULL}, // S_STG4
	{SPR_STLG, 5, -1, {NULL}, 0, 0, S_NULL}, // S_STG5
	{SPR_STLG, 6, -1, {NULL}, 0, 0, S_NULL}, // S_STG6
	{SPR_STLG, 7, -1, {NULL}, 0, 0, S_NULL}, // S_STG7
	{SPR_STLG, 8, -1, {NULL}, 0, 0, S_NULL}, // S_STG8
	{SPR_STLG, 9, -1, {NULL}, 0, 0, S_NULL}, // S_STG9

	// Xmas-specific stuff
	{SPR_XMS1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_XMASPOLE
	{SPR_XMS2, 0, -1, {NULL}, 0, 0, S_NULL}, // S_CANDYCANE
	{SPR_XMS3, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SNOWMAN
	{SPR_XMS3, 1, -1, {NULL}, 0, 0, S_NULL}, // S_SNOWMANHAT
	{SPR_XMS4, 0, -1, {NULL}, 0, 0, S_NULL}, // S_LAMPPOST1
	{SPR_XMS4, 1, -1, {NULL}, 0, 0, S_NULL}, // S_LAMPPOST2
	{SPR_XMS5, 0, -1, {NULL}, 0, 0, S_NULL}, // S_HANGSTAR
	{SPR_XMS6, 0, -1, {NULL}, 0, 0, S_NULL}, // S_MISTLETOE
	// Xmas GFZ bushes
	{SPR_BUS3, 1, -1, {NULL}, 0, 0, S_NULL}, // S_XMASBLUEBERRYBUSH
	{SPR_BUS1, 1, -1, {NULL}, 0, 0, S_NULL}, // S_XMASBERRYBUSH
	{SPR_BUS2, 1, -1, {NULL}, 0, 0, S_NULL}, // S_XMASBUSH
	// FHZ
	{SPR_FHZI, 0, -1, {NULL}, 0, 0, S_NULL}, // S_FHZICE1
	{SPR_FHZI, 1, -1, {NULL}, 0, 0, S_NULL}, // S_FHZICE2
	{SPR_ROSY, 16, 8, {NULL}, 0, 0, S_ROSY_IDLE2}, // S_ROSY_IDLE1
	{SPR_ROSY, 17, 4, {NULL}, 0, 0, S_ROSY_IDLE3}, // S_ROSY_IDLE2
	{SPR_ROSY, 18, 8, {NULL}, 0, 0, S_ROSY_IDLE4}, // S_ROSY_IDLE3
	{SPR_ROSY, 17, 4, {NULL}, 0, 0, S_ROSY_IDLE1}, // S_ROSY_IDLE4
	{SPR_ROSY, 14, -1, {NULL}, 1, 0, S_NULL}, // S_ROSY_JUMP
	{SPR_ROSY,  5, -1, {NULL}, 7, 0, S_NULL}, // S_ROSY_WALK
	{SPR_ROSY, 19, -1, {NULL}, 0, 0, S_NULL}, // S_ROSY_HUG
	{SPR_ROSY, 13, -1, {NULL}, 0, 0, S_NULL}, // S_ROSY_PAIN
	{SPR_ROSY,  1|FF_ANIMATE, -1, {NULL}, 3, 16, S_NULL}, // S_ROSY_STND
	{SPR_ROSY, 20|FF_ANIMATE, TICRATE, {NULL}, 3, 4, S_ROSY_WALK}, // S_ROSY_UNHAPPY

	// Halloween Scenery
	// Pumpkins
	{SPR_PUMK,  0, -1, {NULL}, 0, 0, S_NULL}, // S_JACKO1
	{SPR_PUMK,  3|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO1OVERLAY_2}, // S_JACKO1OVERLAY_1
	{SPR_PUMK,  4|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO1OVERLAY_3}, // S_JACKO1OVERLAY_2
	{SPR_PUMK,  5|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO1OVERLAY_4}, // S_JACKO1OVERLAY_3
	{SPR_PUMK,  4|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO1OVERLAY_1}, // S_JACKO1OVERLAY_4
	{SPR_PUMK,  1, -1, {NULL}, 0, 0, S_NULL}, // S_JACKO2
	{SPR_PUMK,  6|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO2OVERLAY_2}, // S_JACKO2OVERLAY_1
	{SPR_PUMK,  7|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO2OVERLAY_3}, // S_JACKO2OVERLAY_2
	{SPR_PUMK,  8|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO2OVERLAY_4}, // S_JACKO2OVERLAY_3
	{SPR_PUMK,  7|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO2OVERLAY_1}, // S_JACKO2OVERLAY_4
	{SPR_PUMK,  2, -1, {NULL}, 0, 0, S_NULL}, // S_JACKO3
	{SPR_PUMK,  9|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO3OVERLAY_2}, // S_JACKO3OVERLAY_1
	{SPR_PUMK, 10|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO3OVERLAY_3}, // S_JACKO3OVERLAY_2
	{SPR_PUMK, 11|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO3OVERLAY_4}, // S_JACKO3OVERLAY_3
	{SPR_PUMK, 10|FF_FULLBRIGHT, 5, {NULL}, 0, 0, S_JACKO3OVERLAY_1}, // S_JACKO3OVERLAY_4
	// Dr Seuss Trees
	{SPR_HHPL, 2, -1, {A_ConnectToGround}, MT_HHZTREE_PART, 0, S_NULL}, // S_HHZTREE_TOP,
	{SPR_HHPL, 1, -1, {NULL}, 0, 0, S_NULL}, // S_HHZTREE_TRUNK,
	{SPR_HHPL, FF_PAPERSPRITE, -1, {NULL}, 0, 0, S_NULL}, // S_HHZTREE_LEAF,
	// Mushroom
	{SPR_SHRM, 4,  3, {NULL}, 0, 0, S_HHZSHROOM_2},  // S_HHZSHROOM_1,
	{SPR_SHRM, 3,  3, {NULL}, 0, 0, S_HHZSHROOM_3},  // S_HHZSHROOM_2,
	{SPR_SHRM, 2,  2, {NULL}, 0, 0, S_HHZSHROOM_4},  // S_HHZSHROOM_3,
	{SPR_SHRM, 1,  1, {NULL}, 0, 0, S_HHZSHROOM_5},  // S_HHZSHROOM_4,
	{SPR_SHRM, 0,  1, {NULL}, 0, 0, S_HHZSHROOM_6},  // S_HHZSHROOM_5,
	{SPR_SHRM, 1,  4, {NULL}, 0, 0, S_HHZSHROOM_7},  // S_HHZSHROOM_6,
	{SPR_SHRM, 2,  2, {NULL}, 0, 0, S_HHZSHROOM_8},  // S_HHZSHROOM_7,
	{SPR_SHRM, 3,  3, {NULL}, 0, 0, S_HHZSHROOM_9},  // S_HHZSHROOM_8,
	{SPR_SHRM, 4,  3, {NULL}, 0, 0, S_HHZSHROOM_10}, // S_HHZSHROOM_9,
	{SPR_SHRM, 3,  3, {NULL}, 0, 0, S_HHZSHROOM_11}, // S_HHZSHROOM_10,
	{SPR_SHRM, 5,  2, {NULL}, 0, 0, S_HHZSHROOM_12}, // S_HHZSHROOM_11,
	{SPR_SHRM, 6,  1, {NULL}, 0, 0, S_HHZSHROOM_13}, // S_HHZSHROOM_12,
	{SPR_SHRM, 7,  1, {NULL}, 0, 0, S_HHZSHROOM_14}, // S_HHZSHROOM_13,
	{SPR_SHRM, 6,  4, {NULL}, 0, 0, S_HHZSHROOM_15}, // S_HHZSHROOM_14,
	{SPR_SHRM, 5,  2, {NULL}, 0, 0, S_HHZSHROOM_16}, // S_HHZSHROOM_15,
	{SPR_SHRM, 3,  3, {NULL}, 0, 0, S_HHZSHROOM_1},  // S_HHZSHROOM_16,
	// Misc
	{SPR_HHZM, 0, -1, {NULL}, 0, 0, S_NULL}, // S_HHZGRASS,
	{SPR_HHZM, 1, -1, {NULL}, 0, 0, S_NULL}, // S_HHZTENT1,
	{SPR_HHZM, 2, -1, {NULL}, 0, 0, S_NULL}, // S_HHZTENT2,
	{SPR_HHZM, 4, -1, {NULL}, 0, 0, S_NULL}, // S_HHZSTALAGMITE_TALL,
	{SPR_HHZM, 5, -1, {NULL}, 0, 0, S_NULL}, // S_HHZSTALAGMITE_SHORT,

	// Loads of Botanic Serenity bullshit
	{SPR_BSZ1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_RED
	{SPR_BSZ1, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_PURPLE
	{SPR_BSZ1, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_BLUE
	{SPR_BSZ1, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_CYAN
	{SPR_BSZ1, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_YELLOW
	{SPR_BSZ1, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZTALLFLOWER_ORANGE
	{SPR_BSZ2, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_RED
	{SPR_BSZ2, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_PURPLE
	{SPR_BSZ2, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_BLUE
	{SPR_BSZ2, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_CYAN
	{SPR_BSZ2, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_YELLOW
	{SPR_BSZ2, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZFLOWER_ORANGE
	{SPR_BSZ3, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_RED
	{SPR_BSZ3, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_PURPLE
	{SPR_BSZ3, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_BLUE
	{SPR_BSZ3, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_CYAN
	{SPR_BSZ3, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_YELLOW
	{SPR_BSZ3, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHORTFLOWER_ORANGE
	{SPR_BST1, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_RED
	{SPR_BST2, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_PURPLE
	{SPR_BST3, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_BLUE
	{SPR_BST4, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_CYAN
	{SPR_BST5, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_YELLOW
	{SPR_BST6, FF_ANIMATE, -1, {NULL}, 11, 4, S_NULL}, // S_BSZTULIP_ORANGE
	{SPR_BSZ5, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_RED
	{SPR_BSZ5, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_PURPLE
	{SPR_BSZ5, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_BLUE
	{SPR_BSZ5, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_CYAN
	{SPR_BSZ5, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_YELLOW
	{SPR_BSZ5, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLUSTER_ORANGE
	{SPR_BSZ6, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_RED
	{SPR_BSZ6, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_PURPLE
	{SPR_BSZ6, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_BLUE
	{SPR_BSZ6, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_CYAN
	{SPR_BSZ6, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_YELLOW
	{SPR_BSZ6, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZBUSH_ORANGE
	{SPR_BSZ7, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_RED
	{SPR_BSZ7, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_PURPLE
	{SPR_BSZ7, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_BLUE
	{SPR_BSZ7, 3, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_CYAN
	{SPR_BSZ7, 4, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_YELLOW
	{SPR_BSZ7, 5, -1, {NULL}, 0, 0, S_NULL}, // S_BSZVINE_ORANGE
	{SPR_BSZ8, 0, -1, {NULL}, 0, 0, S_NULL}, // S_BSZSHRUB
	{SPR_BSZ8, 1, -1, {NULL}, 0, 0, S_NULL}, // S_BSZCLOVER
	{SPR_BSZ8, 2, -1, {NULL}, 0, 0, S_NULL}, // S_BIG_PALMTREE_TRUNK
	{SPR_BSZ8, 3, -1, {A_ConnectToGround}, MT_BIG_PALMTREE_TRUNK, 0, S_NULL}, // S_BIG_PALMTREE_TOP
	{SPR_BSZ8, 4, -1, {NULL}, 0, 0, S_NULL}, // S_PALMTREE_TRUNK
	{SPR_BSZ8, 5, -1, {A_ConnectToGround},     MT_PALMTREE_TRUNK, 0, S_NULL}, // S_PALMTREE_TOP

	// Disco ball
	{SPR_DBAL, FF_FULLBRIGHT,   5, {NULL}, 0, 0, S_DBALL2}, // S_DBALL1
	{SPR_DBAL, FF_FULLBRIGHT|1, 5, {NULL}, 0, 0, S_DBALL3}, // S_DBALL2
	{SPR_DBAL, FF_FULLBRIGHT|2, 5, {NULL}, 0, 0, S_DBALL4}, // S_DBALL3
	{SPR_DBAL, FF_FULLBRIGHT|3, 5, {NULL}, 0, 0, S_DBALL5}, // S_DBALL4
	{SPR_DBAL, FF_FULLBRIGHT|4, 5, {NULL}, 0, 0, S_DBALL6}, // S_DBALL5
	{SPR_DBAL, FF_FULLBRIGHT|5, 5, {NULL}, 0, 0, S_DBALL1}, // S_DBALL6

	{SPR_ESTA, 1, -1, {NULL}, 0, 0, S_NULL}, // S_EGGSTATUE2

	// Shield Orb
	{SPR_ARMA, FF_TRANS40   , 2, {NULL}, 0, 0, S_ARMA2 }, // S_ARMA1
	{SPR_ARMA, FF_TRANS40| 1, 2, {NULL}, 0, 0, S_ARMA3 }, // S_ARMA2
	{SPR_ARMA, FF_TRANS40| 2, 2, {NULL}, 0, 0, S_ARMA4 }, // S_ARMA3
	{SPR_ARMA, FF_TRANS40| 3, 2, {NULL}, 0, 0, S_ARMA5 }, // S_ARMA4
	{SPR_ARMA, FF_TRANS40| 4, 2, {NULL}, 0, 0, S_ARMA6 }, // S_ARMA5
	{SPR_ARMA, FF_TRANS40| 5, 2, {NULL}, 0, 0, S_ARMA7 }, // S_ARMA6
	{SPR_ARMA, FF_TRANS40| 6, 2, {NULL}, 0, 0, S_ARMA8 }, // S_ARMA7
	{SPR_ARMA, FF_TRANS40| 7, 2, {NULL}, 0, 0, S_ARMA9 }, // S_ARMA8
	{SPR_ARMA, FF_TRANS40| 8, 2, {NULL}, 0, 0, S_ARMA10}, // S_ARMA9
	{SPR_ARMA, FF_TRANS40| 9, 2, {NULL}, 0, 0, S_ARMA11}, // S_ARMA10
	{SPR_ARMA, FF_TRANS40|10, 2, {NULL}, 0, 0, S_ARMA12}, // S_ARMA11
	{SPR_ARMA, FF_TRANS40|11, 2, {NULL}, 0, 0, S_ARMA13}, // S_ARMA12
	{SPR_ARMA, FF_TRANS40|12, 2, {NULL}, 0, 0, S_ARMA14}, // S_ARMA13
	{SPR_ARMA, FF_TRANS40|13, 2, {NULL}, 0, 0, S_ARMA15}, // S_ARMA14
	{SPR_ARMA, FF_TRANS40|14, 2, {NULL}, 0, 0, S_ARMA16}, // S_ARMA15
	{SPR_ARMA, FF_TRANS40|15, 2, {NULL}, 0, 0, S_ARMA1 }, // S_ARMA16

	{SPR_ARMF, FF_FULLBRIGHT   , 2, {NULL}, 0, 0, S_ARMF2 }, // S_ARMF1
	{SPR_ARMF, FF_FULLBRIGHT| 1, 2, {NULL}, 0, 0, S_ARMF3 }, // S_ARMF2
	{SPR_ARMF, FF_FULLBRIGHT| 2, 2, {NULL}, 0, 0, S_ARMF4 }, // S_ARMF3
	{SPR_ARMF, FF_FULLBRIGHT| 3, 2, {NULL}, 0, 0, S_ARMF5 }, // S_ARMF4
	{SPR_ARMF, FF_FULLBRIGHT| 4, 2, {NULL}, 0, 0, S_ARMF6 }, // S_ARMF5
	{SPR_ARMF, FF_FULLBRIGHT| 5, 2, {NULL}, 0, 0, S_ARMF7 }, // S_ARMF6
	{SPR_ARMF, FF_FULLBRIGHT| 6, 2, {NULL}, 0, 0, S_ARMF8 }, // S_ARMF7
	{SPR_ARMF, FF_FULLBRIGHT| 7, 2, {NULL}, 0, 0, S_ARMF9 }, // S_ARMF8
	{SPR_ARMF, FF_FULLBRIGHT| 8, 2, {NULL}, 0, 0, S_ARMF10}, // S_ARMF9
	{SPR_ARMF, FF_FULLBRIGHT| 9, 2, {NULL}, 0, 0, S_ARMF11}, // S_ARMF10
	{SPR_ARMF, FF_FULLBRIGHT|10, 2, {NULL}, 0, 0, S_ARMF12}, // S_ARMF11
	{SPR_ARMF, FF_FULLBRIGHT|11, 2, {NULL}, 0, 0, S_ARMF13}, // S_ARMF12
	{SPR_ARMF, FF_FULLBRIGHT|12, 2, {NULL}, 0, 0, S_ARMF14}, // S_ARMF13
	{SPR_ARMF, FF_FULLBRIGHT|13, 2, {NULL}, 0, 0, S_ARMF15}, // S_ARMF14
	{SPR_ARMF, FF_FULLBRIGHT|14, 2, {NULL}, 0, 0, S_ARMF16}, // S_ARMF15
	{SPR_ARMF, FF_FULLBRIGHT|15, 2, {NULL}, 0, 0, S_ARMF17}, // S_ARMF16
	{SPR_ARMB, FF_FULLBRIGHT   , 2, {NULL}, 0, 0, S_ARMF18}, // S_ARMF17
	{SPR_ARMB, FF_FULLBRIGHT| 1, 2, {NULL}, 0, 0, S_ARMF19}, // S_ARMF18
	{SPR_ARMB, FF_FULLBRIGHT| 2, 2, {NULL}, 0, 0, S_ARMF20}, // S_ARMF19
	{SPR_ARMB, FF_FULLBRIGHT| 3, 2, {NULL}, 0, 0, S_ARMF21}, // S_ARMF20
	{SPR_ARMB, FF_FULLBRIGHT| 4, 2, {NULL}, 0, 0, S_ARMF22}, // S_ARMF21
	{SPR_ARMB, FF_FULLBRIGHT| 5, 2, {NULL}, 0, 0, S_ARMF23}, // S_ARMF22
	{SPR_ARMB, FF_FULLBRIGHT| 6, 2, {NULL}, 0, 0, S_ARMF24}, // S_ARMF23
	{SPR_ARMB, FF_FULLBRIGHT| 7, 2, {NULL}, 0, 0, S_ARMF25}, // S_ARMF24
	{SPR_ARMB, FF_FULLBRIGHT| 8, 2, {NULL}, 0, 0, S_ARMF26}, // S_ARMF25
	{SPR_ARMB, FF_FULLBRIGHT| 9, 2, {NULL}, 0, 0, S_ARMF27}, // S_ARMF26
	{SPR_ARMB, FF_FULLBRIGHT|10, 2, {NULL}, 0, 0, S_ARMF28}, // S_ARMF27
	{SPR_ARMB, FF_FULLBRIGHT|11, 2, {NULL}, 0, 0, S_ARMF29}, // S_ARMF28
	{SPR_ARMB, FF_FULLBRIGHT|12, 2, {NULL}, 0, 0, S_ARMF30}, // S_ARMF29
	{SPR_ARMB, FF_FULLBRIGHT|13, 2, {NULL}, 0, 0, S_ARMF31}, // S_ARMF30
	{SPR_ARMB, FF_FULLBRIGHT|14, 2, {NULL}, 0, 0, S_ARMF32}, // S_ARMF31
	{SPR_ARMB, FF_FULLBRIGHT|15, 2, {NULL}, 0, 0, S_ARMF1 }, // S_ARMF32

	{SPR_ARMB, FF_FULLBRIGHT   , 2, {NULL}, 1, 0, S_ARMB2 }, // S_ARMB1
	{SPR_ARMB, FF_FULLBRIGHT| 1, 2, {NULL}, 1, 0, S_ARMB3 }, // S_ARMB2
	{SPR_ARMB, FF_FULLBRIGHT| 2, 2, {NULL}, 1, 0, S_ARMB4 }, // S_ARMB3
	{SPR_ARMB, FF_FULLBRIGHT| 3, 2, {NULL}, 1, 0, S_ARMB5 }, // S_ARMB4
	{SPR_ARMB, FF_FULLBRIGHT| 4, 2, {NULL}, 1, 0, S_ARMB6 }, // S_ARMB5
	{SPR_ARMB, FF_FULLBRIGHT| 5, 2, {NULL}, 1, 0, S_ARMB7 }, // S_ARMB6
	{SPR_ARMB, FF_FULLBRIGHT| 6, 2, {NULL}, 1, 0, S_ARMB8 }, // S_ARMB7
	{SPR_ARMB, FF_FULLBRIGHT| 7, 2, {NULL}, 1, 0, S_ARMB9 }, // S_ARMB8
	{SPR_ARMB, FF_FULLBRIGHT| 8, 2, {NULL}, 1, 0, S_ARMB10}, // S_ARMB9
	{SPR_ARMB, FF_FULLBRIGHT| 9, 2, {NULL}, 1, 0, S_ARMB11}, // S_ARMB10
	{SPR_ARMB, FF_FULLBRIGHT|10, 2, {NULL}, 1, 0, S_ARMB12}, // S_ARMB11
	{SPR_ARMB, FF_FULLBRIGHT|11, 2, {NULL}, 1, 0, S_ARMB13}, // S_ARMB12
	{SPR_ARMB, FF_FULLBRIGHT|12, 2, {NULL}, 1, 0, S_ARMB14}, // S_ARMB13
	{SPR_ARMB, FF_FULLBRIGHT|13, 2, {NULL}, 1, 0, S_ARMB15}, // S_ARMB14
	{SPR_ARMB, FF_FULLBRIGHT|14, 2, {NULL}, 1, 0, S_ARMB16}, // S_ARMB15
	{SPR_ARMB, FF_FULLBRIGHT|15, 2, {NULL}, 1, 0, S_ARMB17}, // S_ARMB16
	{SPR_ARMF, FF_FULLBRIGHT   , 2, {NULL}, 1, 0, S_ARMB18}, // S_ARMB17
	{SPR_ARMF, FF_FULLBRIGHT| 1, 2, {NULL}, 1, 0, S_ARMB19}, // S_ARMB18
	{SPR_ARMF, FF_FULLBRIGHT| 2, 2, {NULL}, 1, 0, S_ARMB20}, // S_ARMB19
	{SPR_ARMF, FF_FULLBRIGHT| 3, 2, {NULL}, 1, 0, S_ARMB21}, // S_ARMB20
	{SPR_ARMF, FF_FULLBRIGHT| 4, 2, {NULL}, 1, 0, S_ARMB22}, // S_ARMB21
	{SPR_ARMF, FF_FULLBRIGHT| 5, 2, {NULL}, 1, 0, S_ARMB23}, // S_ARMB22
	{SPR_ARMF, FF_FULLBRIGHT| 6, 2, {NULL}, 1, 0, S_ARMB24}, // S_ARMB23
	{SPR_ARMF, FF_FULLBRIGHT| 7, 2, {NULL}, 1, 0, S_ARMB25}, // S_ARMB24
	{SPR_ARMF, FF_FULLBRIGHT| 8, 2, {NULL}, 1, 0, S_ARMB26}, // S_ARMB25
	{SPR_ARMF, FF_FULLBRIGHT| 9, 2, {NULL}, 1, 0, S_ARMB27}, // S_ARMB26
	{SPR_ARMF, FF_FULLBRIGHT|10, 2, {NULL}, 1, 0, S_ARMB28}, // S_ARMB27
	{SPR_ARMF, FF_FULLBRIGHT|11, 2, {NULL}, 1, 0, S_ARMB29}, // S_ARMB28
	{SPR_ARMF, FF_FULLBRIGHT|12, 2, {NULL}, 1, 0, S_ARMB30}, // S_ARMB29
	{SPR_ARMF, FF_FULLBRIGHT|13, 2, {NULL}, 1, 0, S_ARMB31}, // S_ARMB30
	{SPR_ARMF, FF_FULLBRIGHT|14, 2, {NULL}, 1, 0, S_ARMB32}, // S_ARMB31
	{SPR_ARMF, FF_FULLBRIGHT|15, 2, {NULL}, 1, 0, S_ARMB1 }, // S_ARMB32

	{SPR_WIND, FF_TRANS70  , 2, {NULL}, 0, 0, S_WIND2}, // S_WIND1
	{SPR_WIND, FF_TRANS70|1, 2, {NULL}, 0, 0, S_WIND3}, // S_WIND2
	{SPR_WIND, FF_TRANS70|2, 2, {NULL}, 0, 0, S_WIND4}, // S_WIND3
	{SPR_WIND, FF_TRANS70|3, 2, {NULL}, 0, 0, S_WIND5}, // S_WIND4
	{SPR_WIND, FF_TRANS70|4, 2, {NULL}, 0, 0, S_WIND6}, // S_WIND5
	{SPR_WIND, FF_TRANS70|5, 2, {NULL}, 0, 0, S_WIND7}, // S_WIND6
	{SPR_WIND, FF_TRANS70|6, 2, {NULL}, 0, 0, S_WIND8}, // S_WIND7
	{SPR_WIND, FF_TRANS70|7, 2, {NULL}, 0, 0, S_WIND1}, // S_WIND8

	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40   , 2, {NULL}, 0, 0, S_MAGN2 }, // S_MAGN1
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40| 1, 2, {NULL}, 0, 0, S_MAGN3 }, // S_MAGN2
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40| 2, 2, {NULL}, 0, 0, S_MAGN4 }, // S_MAGN3
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40| 3, 2, {NULL}, 0, 0, S_MAGN5 }, // S_MAGN4
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40| 4, 2, {NULL}, 0, 0, S_MAGN6 }, // S_MAGN5
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40| 5, 2, {NULL}, 0, 0, S_MAGN7 }, // S_MAGN6
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40| 6, 2, {NULL}, 0, 0, S_MAGN8 }, // S_MAGN7
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40| 7, 2, {NULL}, 0, 0, S_MAGN9 }, // S_MAGN8
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40| 8, 2, {NULL}, 0, 0, S_MAGN10}, // S_MAGN9
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40| 9, 2, {NULL}, 0, 0, S_MAGN11}, // S_MAGN10
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40|10, 2, {NULL}, 0, 0, S_MAGN12}, // S_MAGN11
	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS40|11, 2, {NULL}, 0, 0, S_MAGN1 }, // S_MAGN12

	{SPR_MAGN, FF_FULLBRIGHT|FF_TRANS10|12, 2, {NULL}, 0, 0, S_MAGN1 }, // S_MAGN13

	{SPR_FORC, FF_TRANS50  , 3, {NULL}, 0, 0, S_FORC2 }, // S_FORC1
	{SPR_FORC, FF_TRANS50|1, 3, {NULL}, 0, 0, S_FORC3 }, // S_FORC2
	{SPR_FORC, FF_TRANS50|2, 3, {NULL}, 0, 0, S_FORC4 }, // S_FORC3
	{SPR_FORC, FF_TRANS50|3, 3, {NULL}, 0, 0, S_FORC5 }, // S_FORC4
	{SPR_FORC, FF_TRANS50|4, 3, {NULL}, 0, 0, S_FORC6 }, // S_FORC5
	{SPR_FORC, FF_TRANS50|5, 3, {NULL}, 0, 0, S_FORC7 }, // S_FORC6
	{SPR_FORC, FF_TRANS50|6, 3, {NULL}, 0, 0, S_FORC8 }, // S_FORC7
	{SPR_FORC, FF_TRANS50|7, 3, {NULL}, 0, 0, S_FORC9 }, // S_FORC8
	{SPR_FORC, FF_TRANS50|8, 3, {NULL}, 0, 0, S_FORC10}, // S_FORC9
	{SPR_FORC, FF_TRANS50|9, 3, {NULL}, 0, 0, S_FORC1 }, // S_FORC10

	{SPR_FORC, FF_TRANS50|10, 3, {NULL}, 0, 0, S_FORC12}, // S_FORC11
	{SPR_FORC, FF_TRANS50|11, 3, {NULL}, 0, 0, S_FORC13}, // S_FORC12
	{SPR_FORC, FF_TRANS50|12, 3, {NULL}, 0, 0, S_FORC14}, // S_FORC13
	{SPR_FORC, FF_TRANS50|13, 3, {NULL}, 0, 0, S_FORC15}, // S_FORC14
	{SPR_FORC, FF_TRANS50|14, 3, {NULL}, 0, 0, S_FORC16}, // S_FORC15
	{SPR_FORC, FF_TRANS50|15, 3, {NULL}, 0, 0, S_FORC17}, // S_FORC16
	{SPR_FORC, FF_TRANS50|16, 3, {NULL}, 0, 0, S_FORC18}, // S_FORC17
	{SPR_FORC, FF_TRANS50|17, 3, {NULL}, 0, 0, S_FORC19}, // S_FORC18
	{SPR_FORC, FF_TRANS50|18, 3, {NULL}, 0, 0, S_FORC20}, // S_FORC19
	{SPR_FORC, FF_TRANS50|19, 3, {NULL}, 0, 0, S_FORC11}, // S_FORC20

	{SPR_FORC, FF_TRANS50|20, -1, {NULL}, 0, 0, S_NULL}, // S_FORC21

	{SPR_ELEM, FF_TRANS50   , 4, {NULL}, 0, 0, S_ELEM2 }, // S_ELEM1
	{SPR_ELEM, FF_TRANS50| 1, 4, {NULL}, 0, 0, S_ELEM3 }, // S_ELEM2
	{SPR_ELEM, FF_TRANS50| 2, 4, {NULL}, 0, 0, S_ELEM4 }, // S_ELEM3
	{SPR_ELEM, FF_TRANS50| 3, 4, {NULL}, 0, 0, S_ELEM5 }, // S_ELEM4
	{SPR_ELEM, FF_TRANS50| 4, 4, {NULL}, 0, 0, S_ELEM6 }, // S_ELEM5
	{SPR_ELEM, FF_TRANS50| 5, 4, {NULL}, 0, 0, S_ELEM7 }, // S_ELEM6
	{SPR_ELEM, FF_TRANS50| 6, 4, {NULL}, 0, 0, S_ELEM8 }, // S_ELEM7
	{SPR_ELEM, FF_TRANS50| 7, 4, {NULL}, 0, 0, S_ELEM9 }, // S_ELEM8
	{SPR_ELEM, FF_TRANS50| 8, 4, {NULL}, 0, 0, S_ELEM10}, // S_ELEM9
	{SPR_ELEM, FF_TRANS50| 9, 4, {NULL}, 0, 0, S_ELEM11}, // S_ELEM10
	{SPR_ELEM, FF_TRANS50|10, 4, {NULL}, 0, 0, S_ELEM12}, // S_ELEM11
	{SPR_ELEM, FF_TRANS50|11, 4, {NULL}, 0, 0, S_ELEM1 }, // S_ELEM12

	{SPR_NULL,             0, 1, {NULL}, 0, 0, S_ELEM14}, // S_ELEM13
	{SPR_ELEM, FF_TRANS50|11, 1, {NULL}, 0, 0, S_ELEM1 }, // S_ELEM14

	{SPR_ELEM, FF_FULLBRIGHT|12, 3, {NULL}, 0, 0, S_ELEMF2 }, // S_ELEMF1
	{SPR_ELEM, FF_FULLBRIGHT|13, 3, {NULL}, 0, 0, S_ELEMF3 }, // S_ELEMF2
	{SPR_ELEM, FF_FULLBRIGHT|14, 3, {NULL}, 0, 0, S_ELEMF4 }, // S_ELEMF3
	{SPR_ELEM, FF_FULLBRIGHT|15, 3, {NULL}, 0, 0, S_ELEMF5 }, // S_ELEMF4
	{SPR_ELEM, FF_FULLBRIGHT|16, 3, {NULL}, 0, 0, S_ELEMF6 }, // S_ELEMF5
	{SPR_ELEM, FF_FULLBRIGHT|17, 3, {NULL}, 0, 0, S_ELEMF7 }, // S_ELEMF6
	{SPR_ELEM, FF_FULLBRIGHT|18, 3, {NULL}, 0, 0, S_ELEMF8 }, // S_ELEMF7
	{SPR_ELEM, FF_FULLBRIGHT|19, 3, {NULL}, 0, 0, S_ELEMF1 }, // S_ELEMF8

	{SPR_ELEM, FF_FULLBRIGHT|20, 1, {NULL}, 0, 0, S_ELEMF10}, // S_ELEMF9
	{SPR_NULL, 0,                1, {NULL}, 0, 0, S_ELEMF1 }, // S_ELEMF10

	{SPR_PITY, FF_TRANS30   , 2, {NULL}, 0, 0, S_PITY2},  // S_PITY1
	{SPR_PITY, FF_TRANS30| 1, 2, {NULL}, 0, 0, S_PITY3},  // S_PITY2
	{SPR_PITY, FF_TRANS30| 2, 2, {NULL}, 0, 0, S_PITY4},  // S_PITY3
	{SPR_PITY, FF_TRANS30| 3, 2, {NULL}, 0, 0, S_PITY5},  // S_PITY4
	{SPR_PITY, FF_TRANS30| 4, 2, {NULL}, 0, 0, S_PITY6},  // S_PITY5
	{SPR_PITY, FF_TRANS30| 5, 2, {NULL}, 0, 0, S_PITY7},  // S_PITY6
	{SPR_PITY, FF_TRANS30| 6, 2, {NULL}, 0, 0, S_PITY8},  // S_PITY7
	{SPR_PITY, FF_TRANS30| 7, 2, {NULL}, 0, 0, S_PITY9},  // S_PITY8
	{SPR_PITY, FF_TRANS30| 8, 2, {NULL}, 0, 0, S_PITY10}, // S_PITY9
	{SPR_PITY, FF_TRANS30| 9, 2, {NULL}, 0, 0, S_PITY11}, // S_PITY10
	{SPR_PITY, FF_TRANS30|10, 2, {NULL}, 0, 0, S_PITY12}, // S_PITY11
	{SPR_PITY, FF_TRANS30|11, 2, {NULL}, 0, 0, S_PITY1},  // S_PITY12

	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40  , 2, {NULL}, 0, 0, S_FIRS2}, // S_FIRS1
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|1, 2, {NULL}, 0, 0, S_FIRS3}, // S_FIRS2
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|2, 2, {NULL}, 0, 0, S_FIRS4}, // S_FIRS3
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|3, 2, {NULL}, 0, 0, S_FIRS5}, // S_FIRS4
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|4, 2, {NULL}, 0, 0, S_FIRS6}, // S_FIRS5
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|5, 2, {NULL}, 0, 0, S_FIRS7}, // S_FIRS6
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|6, 2, {NULL}, 0, 0, S_FIRS8}, // S_FIRS7
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|7, 2, {NULL}, 0, 0, S_FIRS9}, // S_FIRS8
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|8, 2, {NULL}, 0, 0, S_FIRS1}, // S_FIRS9

	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|18, 1, {NULL}, 0, 0, S_FIRS11}, // S_FIRS10
	{SPR_NULL, 0,                           1, {NULL}, 0, 0, S_FIRS1 }, // S_FIRS11

	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40| 9, 2, {NULL}, 0, 0, S_FIRSB2}, // S_FIRSB1
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|10, 2, {NULL}, 0, 0, S_FIRSB3}, // S_FIRSB2
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|11, 2, {NULL}, 0, 0, S_FIRSB4}, // S_FIRSB3
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|12, 2, {NULL}, 0, 0, S_FIRSB5}, // S_FIRSB4
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|13, 2, {NULL}, 0, 0, S_FIRSB6}, // S_FIRSB5
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|14, 2, {NULL}, 0, 0, S_FIRSB7}, // S_FIRSB6
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|15, 2, {NULL}, 0, 0, S_FIRSB8}, // S_FIRSB7
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|16, 2, {NULL}, 0, 0, S_FIRSB9}, // S_FIRSB8
	{SPR_FIRS, FF_FULLBRIGHT|FF_TRANS40|17, 2, {NULL}, 0, 0, S_FIRSB1}, // S_FIRSB9

	{SPR_NULL, 0,                           2, {NULL}, 0, 0, S_FIRSB1 }, // S_FIRSB10

	{SPR_BUBS, FF_TRANS30  , 3, {NULL}, 0, 0, S_BUBS2}, // S_BUBS1
	{SPR_BUBS, FF_TRANS30|1, 3, {NULL}, 0, 0, S_BUBS3}, // S_BUBS2
	{SPR_BUBS, FF_TRANS30|2, 3, {NULL}, 0, 0, S_BUBS4}, // S_BUBS3
	{SPR_BUBS, FF_TRANS30|3, 3, {NULL}, 0, 0, S_BUBS5}, // S_BUBS4
	{SPR_BUBS, FF_TRANS30|4, 3, {NULL}, 0, 0, S_BUBS6}, // S_BUBS5
	{SPR_BUBS, FF_TRANS30|5, 3, {NULL}, 0, 0, S_BUBS7}, // S_BUBS6
	{SPR_BUBS, FF_TRANS30|6, 3, {NULL}, 0, 0, S_BUBS8}, // S_BUBS7
	{SPR_BUBS, FF_TRANS30|7, 3, {NULL}, 0, 0, S_BUBS9}, // S_BUBS8
	{SPR_BUBS, FF_TRANS30|8, 3, {NULL}, 0, 0, S_BUBS1}, // S_BUBS9

	{SPR_NULL, 0,   3, {NULL}, 0, 0, S_BUBS1}, // S_BUBS10
	{SPR_NULL, 0, 4*3, {NULL}, 0, 0, S_BUBS1}, // S_BUBS11

	{SPR_BUBS, FF_TRANS30| 9, 3, {NULL}, 0, 0, S_BUBSB2}, // S_BUBSB1
	{SPR_BUBS, FF_TRANS30|10, 3, {NULL}, 0, 0, S_BUBSB3}, // S_BUBSB2
	{SPR_BUBS, FF_TRANS30|11, 3, {NULL}, 0, 0, S_BUBSB4}, // S_BUBSB3
	{SPR_BUBS, FF_TRANS30|10, 3, {NULL}, 0, 0, S_BUBSB1}, // S_BUBSB4

	{SPR_BUBS, FF_TRANS30|12, 3, {NULL}, 0, 0, S_BUBSB3}, // S_BUBSB5
	{SPR_BUBS, FF_TRANS30|13, 3, {NULL}, 0, 0, S_BUBSB5}, // S_BUBSB6

	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20   ,   2, {NULL}, 0, 0, S_ZAPS2 }, // S_ZAPS1
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 1,   2, {NULL}, 0, 0, S_ZAPS3 }, // S_ZAPS2
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 2,   2, {NULL}, 0, 0, S_ZAPS4 }, // S_ZAPS3
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 3,   2, {NULL}, 0, 0, S_ZAPS5 }, // S_ZAPS4
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 4,   2, {NULL}, 0, 0, S_ZAPS6 }, // S_ZAPS5
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 5,   2, {NULL}, 0, 0, S_ZAPS7 }, // S_ZAPS6
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 6,   2, {NULL}, 0, 0, S_ZAPS8 }, // S_ZAPS7
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 7,   2, {NULL}, 0, 0, S_ZAPS9 }, // S_ZAPS8
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 8,   2, {NULL}, 0, 0, S_ZAPS10}, // S_ZAPS9
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 9,   2, {NULL}, 0, 0, S_ZAPS11}, // S_ZAPS10
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20|10,   2, {NULL}, 0, 0, S_ZAPS12}, // S_ZAPS11
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20|11,   2, {NULL}, 0, 0, S_ZAPS13}, // S_ZAPS12
	{SPR_NULL,                           0, 9*2, {NULL}, 0, 0, S_ZAPS14}, // S_ZAPS13
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 9,   2, {NULL}, 0, 0, S_ZAPS15}, // S_ZAPS14
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20|10,   2, {NULL}, 0, 0, S_ZAPS16}, // S_ZAPS15
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20|11,   2, {NULL}, 0, 0, S_ZAPS1 }, // S_ZAPS16

	{SPR_NULL,                           0, 12*2, {NULL}, 0, 0, S_ZAPSB2 }, // S_ZAPSB1
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 8,    2, {NULL}, 0, 0, S_ZAPSB3 }, // S_ZAPSB2
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 7,    2, {NULL}, 0, 0, S_ZAPSB4 }, // S_ZAPSB3
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 6,    2, {NULL}, 0, 0, S_ZAPSB5 }, // S_ZAPSB4
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 5,    2, {NULL}, 0, 0, S_ZAPSB6 }, // S_ZAPSB5
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 4,    2, {NULL}, 0, 0, S_ZAPSB7 }, // S_ZAPSB6
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 3,    2, {NULL}, 0, 0, S_ZAPSB8 }, // S_ZAPSB7
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 2,    2, {NULL}, 0, 0, S_ZAPSB9 }, // S_ZAPSB8
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20| 1,    2, {NULL}, 0, 0, S_ZAPSB10}, // S_ZAPSB9
	{SPR_ZAPS, FF_FULLBRIGHT|FF_TRANS20   ,    2, {NULL}, 0, 0, S_ZAPSB11}, // S_ZAPSB10
	{SPR_NULL,                           0, 15*2, {NULL}, 0, 0, S_ZAPSB2 }, // S_ZAPSB11

	// Thunder spark
	{SPR_SSPK, FF_ANIMATE, -1, {NULL}, 1, 2, S_NULL},   // S_THUNDERCOIN_SPARK

	// Invincibility Sparkles
	{SPR_IVSP, FF_ANIMATE, 32, {NULL}, 31, 1, S_NULL},   // S_IVSP

	// Super Sonic Spark
	{SPR_SSPK,   FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_SSPK2}, // S_SSPK1
	{SPR_SSPK, 1|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_SSPK3}, // S_SSPK2
	{SPR_SSPK, 2|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_SSPK4}, // S_SSPK3
	{SPR_SSPK, 1|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_SSPK5}, // S_SSPK4
	{SPR_SSPK,   FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL},  // S_SSPK5

	// Flicky-sized bubble
	{SPR_FBUB, 0, -1, {NULL}, 0, 0, S_NULL}, // S_FLICKY_BUBBLE

	// Bluebird
	{SPR_FL01, 0, 2, {A_FlickyCheck}, S_FLICKY_01_FLAP1, S_FLICKY_01_FLAP1, S_FLICKY_01_OUT},   // S_FLICKY_01_OUT
	{SPR_FL01, 1, 3, {A_FlickyFly},          4*FRACUNIT,       16*FRACUNIT, S_FLICKY_01_FLAP2}, // S_FLICKY_01_FLAP1
	{SPR_FL01, 2, 3, {A_FlickyFly},          4*FRACUNIT,       16*FRACUNIT, S_FLICKY_01_FLAP3}, // S_FLICKY_01_FLAP2
	{SPR_FL01, 3, 3, {A_FlickyFly},          4*FRACUNIT,       16*FRACUNIT, S_FLICKY_01_FLAP1}, // S_FLICKY_01_FLAP3
	{SPR_FL01, FF_ANIMATE|1, -1, {NULL}, 2, 3, S_NULL},                                         // S_FLICKY_01_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_01, 384*FRACUNIT, S_FLICKY_01_CENTER},        // S_FLICKY_01_CENTER

	// Rabbit
	{SPR_FL02, 0, 2, {A_FlickyCheck}, S_FLICKY_02_AIM,                0, S_FLICKY_02_OUT},  // S_FLICKY_02_OUT
	{SPR_FL02, 1, 1, {A_FlickyAim},             ANG30,      32*FRACUNIT, S_FLICKY_02_HOP},  // S_FLICKY_02_AIM
	{SPR_FL02, 1, 1, {A_FlickyHop},        6*FRACUNIT,       4*FRACUNIT, S_FLICKY_02_UP},   // S_FLICKY_02_HOP
	{SPR_FL02, 2, 2, {A_FlickyCheck}, S_FLICKY_02_AIM, S_FLICKY_02_DOWN, S_FLICKY_02_UP},   // S_FLICKY_02_UP
	{SPR_FL02, 3, 2, {A_FlickyCheck}, S_FLICKY_02_AIM,                0, S_FLICKY_02_DOWN}, // S_FLICKY_02_DOWN
	{SPR_FL02, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_02_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_02, 384*FRACUNIT, S_FLICKY_02_CENTER},        // S_FLICKY_02_CENTER

	// Chicken
	{SPR_FL03, 0, 2, {A_FlickyCheck},   S_FLICKY_03_AIM, S_FLICKY_03_FLAP1, S_FLICKY_03_OUT},   // S_FLICKY_03_OUT
	{SPR_FL03, 1, 1, {A_FlickyAim},            ANGLE_45,       32*FRACUNIT, S_FLICKY_03_HOP},   // S_FLICKY_03_AIM
	{SPR_FL03, 1, 1, {A_FlickyHop},          7*FRACUNIT,        2*FRACUNIT, S_FLICKY_03_UP},    // S_FLICKY_03_HOP
	{SPR_FL03, 2, 2, {A_FlickyFlutter}, S_FLICKY_03_HOP, S_FLICKY_03_FLAP1, S_FLICKY_03_UP},    // S_FLICKY_03_UP
	{SPR_FL03, 3, 2, {A_FlickyFlutter}, S_FLICKY_03_HOP,                 0, S_FLICKY_03_FLAP2}, // S_FLICKY_03_FLAP1
	{SPR_FL03, 4, 2, {A_FlickyFlutter}, S_FLICKY_03_HOP,                 0, S_FLICKY_03_FLAP1}, // S_FLICKY_03_FLAP2
	{SPR_FL03, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_03_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_03, 384*FRACUNIT, S_FLICKY_03_CENTER},        // S_FLICKY_03_CENTER

	// Seal
	{SPR_FL04, 0, 2, {A_FlickyCheck}, S_FLICKY_04_AIM,                 0, S_FLICKY_04_OUT},   // S_FLICKY_04_OUT
	{SPR_FL04, 1, 1, {A_FlickyAim},             ANG30,       32*FRACUNIT, S_FLICKY_04_HOP},   // S_FLICKY_04_AIM
	{SPR_FL04, 1, 1, {A_FlickyHop},        3*FRACUNIT,        2*FRACUNIT, S_FLICKY_04_UP},    // S_FLICKY_04_HOP
	{SPR_FL04, 2, 4, {A_FlickyCheck}, S_FLICKY_04_AIM,  S_FLICKY_04_DOWN, S_FLICKY_04_UP},    // S_FLICKY_04_UP
	{SPR_FL04, 3, 4, {A_FlickyCheck}, S_FLICKY_04_AIM,                 0, S_FLICKY_04_DOWN},  // S_FLICKY_04_DOWN
	{SPR_FL04, 3, 4, {A_FlickyFly},        2*FRACUNIT,       48*FRACUNIT, S_FLICKY_04_SWIM2}, // S_FLICKY_04_SWIM1
	{SPR_FL04, 4, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_04_SWIM1, S_FLICKY_04_SWIM3}, // S_FLICKY_04_SWIM2
	{SPR_FL04, 3, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_04_SWIM1, S_FLICKY_04_SWIM4}, // S_FLICKY_04_SWIM3
	{SPR_FL04, 5, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_04_SWIM1, S_FLICKY_04_SWIM1}, // S_FLICKY_04_SWIM4
	{SPR_FL04, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_04_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_04, 384*FRACUNIT, S_FLICKY_04_CENTER},        // S_FLICKY_04_CENTER

	// Pig
	{SPR_FL05, 0, 2, {A_FlickyCheck}, S_FLICKY_05_AIM,                0, S_FLICKY_05_OUT},  // S_FLICKY_05_OUT
	{SPR_FL05, 1, 1, {A_FlickyAim},             ANG20,      32*FRACUNIT, S_FLICKY_05_HOP},  // S_FLICKY_05_AIM
	{SPR_FL05, 1, 1, {A_FlickyHop},        4*FRACUNIT,       3*FRACUNIT, S_FLICKY_05_UP},   // S_FLICKY_05_HOP
	{SPR_FL05, 2, 2, {A_FlickyCheck}, S_FLICKY_05_AIM, S_FLICKY_05_DOWN, S_FLICKY_05_UP},   // S_FLICKY_05_UP
	{SPR_FL05, 3, 2, {A_FlickyCheck}, S_FLICKY_05_AIM,                0, S_FLICKY_05_DOWN}, // S_FLICKY_05_DOWN
	{SPR_FL05, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_05_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_05, 384*FRACUNIT, S_FLICKY_05_CENTER},        // S_FLICKY_05_CENTER

	// Chipmunk
	{SPR_FL06, 0, 2, {A_FlickyCheck}, S_FLICKY_06_AIM,                0, S_FLICKY_06_OUT},  // S_FLICKY_06_OUT
	{SPR_FL06, 1, 1, {A_FlickyAim},          ANGLE_90,      32*FRACUNIT, S_FLICKY_06_HOP},  // S_FLICKY_06_AIM
	{SPR_FL06, 1, 1, {A_FlickyHop},        5*FRACUNIT,       6*FRACUNIT, S_FLICKY_06_UP},   // S_FLICKY_06_HOP
	{SPR_FL06, 2, 2, {A_FlickyCheck}, S_FLICKY_06_AIM, S_FLICKY_06_DOWN, S_FLICKY_06_UP},   // S_FLICKY_06_UP
	{SPR_FL06, 3, 2, {A_FlickyCheck}, S_FLICKY_06_AIM,                0, S_FLICKY_06_DOWN}, // S_FLICKY_06_DOWN
	{SPR_FL06, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_06_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_06, 384*FRACUNIT, S_FLICKY_06_CENTER},        // S_FLICKY_06_CENTER

	// Penguin
	{SPR_FL07, 0, 2, {A_FlickyCheck}, S_FLICKY_07_AIML,                 0, S_FLICKY_07_OUT},   // S_FLICKY_07_OUT
	{SPR_FL07, 1, 1, {A_FlickyAim},              ANG30,       32*FRACUNIT, S_FLICKY_07_HOPL},  // S_FLICKY_07_AIML
	{SPR_FL07, 1, 1, {A_FlickyHop},         4*FRACUNIT,        2*FRACUNIT, S_FLICKY_07_UPL},   // S_FLICKY_07_HOPL
	{SPR_FL07, 2, 4, {A_FlickyCheck}, S_FLICKY_07_AIMR, S_FLICKY_07_DOWNL, S_FLICKY_07_UPL},   // S_FLICKY_07_UPL
	{SPR_FL07, 1, 4, {A_FlickyCheck}, S_FLICKY_07_AIMR,                 0, S_FLICKY_07_DOWNL}, // S_FLICKY_07_DOWNL
	{SPR_FL07, 1, 1, {A_FlickyAim},              ANG30,       32*FRACUNIT, S_FLICKY_07_HOPR},  // S_FLICKY_07_AIMR
	{SPR_FL07, 1, 1, {A_FlickyHop},         4*FRACUNIT,        2*FRACUNIT, S_FLICKY_07_UPR},   // S_FLICKY_07_HOPR
	{SPR_FL07, 3, 4, {A_FlickyCheck}, S_FLICKY_07_AIML, S_FLICKY_07_DOWNR, S_FLICKY_07_UPR},   // S_FLICKY_07_UPR
	{SPR_FL07, 1, 4, {A_FlickyCheck}, S_FLICKY_07_AIML,                 0, S_FLICKY_07_DOWNR}, // S_FLICKY_07_DOWNR
	{SPR_FL07, 4, 4, {A_FlickyFly},         3*FRACUNIT,       72*FRACUNIT, S_FLICKY_07_SWIM2}, // S_FLICKY_07_SWIM1
	{SPR_FL07, 5, 4, {A_FlickyCoast},         FRACUNIT, S_FLICKY_07_SWIM1, S_FLICKY_07_SWIM3}, // S_FLICKY_07_SWIM2
	{SPR_FL07, 6, 4, {A_FlickyCoast},       2*FRACUNIT, S_FLICKY_07_SWIM1, S_FLICKY_07_SWIM3}, // S_FLICKY_07_SWIM3
	{SPR_FL07, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_07_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_07, 384*FRACUNIT, S_FLICKY_07_CENTER},        // S_FLICKY_07_CENTER

	// Fish
	{SPR_FL08, 0, 2, {A_FlickyCheck}, S_FLICKY_08_AIM,                 0, S_FLICKY_08_OUT},   // S_FLICKY_08_OUT
	{SPR_FL08, 2, 1, {A_FlickyAim},             ANG30,       32*FRACUNIT, S_FLICKY_08_HOP},   // S_FLICKY_08_AIM
	{SPR_FL08, 2, 1, {A_FlickyFlounder},   2*FRACUNIT,        1*FRACUNIT, S_FLICKY_08_FLAP1}, // S_FLICKY_08_HOP
	{SPR_FL08, 0, 4, {A_FlickyCheck}, S_FLICKY_08_AIM,                 0, S_FLICKY_08_FLAP2}, // S_FLICKY_08_FLAP1
	{SPR_FL08, 1, 4, {A_FlickyCheck}, S_FLICKY_08_AIM,                 0, S_FLICKY_08_FLAP3}, // S_FLICKY_08_FLAP2
	{SPR_FL08, 0, 4, {A_FlickyCheck}, S_FLICKY_08_AIM,                 0, S_FLICKY_08_FLAP4}, // S_FLICKY_08_FLAP3
	{SPR_FL08, 2, 4, {A_FlickyCheck}, S_FLICKY_08_AIM,                 0, S_FLICKY_08_FLAP1}, // S_FLICKY_08_FLAP4
	{SPR_FL08, 0, 4, {A_FlickyFly},        3*FRACUNIT,       64*FRACUNIT, S_FLICKY_08_SWIM2}, // S_FLICKY_08_SWIM1
	{SPR_FL08, 1, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_08_SWIM1, S_FLICKY_08_SWIM3}, // S_FLICKY_08_SWIM2
	{SPR_FL08, 0, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_08_SWIM1, S_FLICKY_08_SWIM4}, // S_FLICKY_08_SWIM3
	{SPR_FL08, 2, 4, {A_FlickyCoast},        FRACUNIT, S_FLICKY_08_SWIM1, S_FLICKY_08_SWIM4}, // S_FLICKY_08_SWIM4
	{SPR_FL08, FF_ANIMATE, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_08_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_08, 384*FRACUNIT, S_FLICKY_08_CENTER},        // S_FLICKY_08_CENTER

	// Ram
	{SPR_FL09, 0, 2, {A_FlickyCheck}, S_FLICKY_09_AIM,                0, S_FLICKY_09_OUT},  // S_FLICKY_09_OUT
	{SPR_FL09, 1, 1, {A_FlickyAim},             ANG30,      32*FRACUNIT, S_FLICKY_09_HOP},  // S_FLICKY_09_AIM
	{SPR_FL09, 1, 1, {A_FlickyHop},        7*FRACUNIT,       2*FRACUNIT, S_FLICKY_09_UP},   // S_FLICKY_09_HOP
	{SPR_FL09, 2, 2, {A_FlickyCheck}, S_FLICKY_09_AIM, S_FLICKY_09_DOWN, S_FLICKY_09_UP},   // S_FLICKY_09_UP
	{SPR_FL09, 3, 2, {A_FlickyCheck}, S_FLICKY_09_AIM,                0, S_FLICKY_09_DOWN}, // S_FLICKY_09_DOWN
	{SPR_FL09, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_09_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_09, 384*FRACUNIT, S_FLICKY_09_CENTER},        // S_FLICKY_09_CENTER

	// Puffin
	{SPR_FL10, 0, 2, {A_FlickyCheck}, S_FLICKY_10_FLAP1, S_FLICKY_10_FLAP1, S_FLICKY_10_OUT},   // S_FLICKY_10_OUT
	{SPR_FL10, 1, 3, {A_FlickySoar},         4*FRACUNIT,       16*FRACUNIT, S_FLICKY_10_FLAP2}, // S_FLICKY_10_FLAP1
	{SPR_FL10, 2, 3, {A_FlickySoar},         4*FRACUNIT,       16*FRACUNIT, S_FLICKY_10_FLAP1}, // S_FLICKY_10_FLAP2
	{SPR_FL10, FF_ANIMATE|1, -1, {NULL}, 1, 3, S_NULL}, // S_FLICKY_10_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_10, 384*FRACUNIT, S_FLICKY_10_CENTER},        // S_FLICKY_10_CENTER

	// Cow
	{SPR_FL11, 0, 2, {A_FlickyCheck}, S_FLICKY_11_AIM,           0, S_FLICKY_11_OUT},  // S_FLICKY_11_OUT
	{SPR_FL11, 1, 1, {A_FlickyAim},          ANGLE_90, 64*FRACUNIT, S_FLICKY_11_RUN1}, // S_FLICKY_11_AIM
	{SPR_FL11, 1, 3, {A_FlickyHop},        FRACUNIT/2,  2*FRACUNIT, S_FLICKY_11_RUN2}, // S_FLICKY_11_RUN1
	{SPR_FL11, 2, 4, {A_FlickyHop},        FRACUNIT/2,  2*FRACUNIT, S_FLICKY_11_RUN3}, // S_FLICKY_11_RUN2
	{SPR_FL11, 3, 4, {A_FlickyHop},        FRACUNIT/2,  2*FRACUNIT, S_FLICKY_11_AIM},  // S_FLICKY_11_RUN3
	{SPR_FL11, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_11_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_11, 384*FRACUNIT, S_FLICKY_11_CENTER},        // S_FLICKY_11_CENTER

	// Rat
	{SPR_FL12, 0, 2, {A_FlickyCheck}, S_FLICKY_12_AIM,           0, S_FLICKY_12_OUT},  // S_FLICKY_12_OUT
	{SPR_FL12, 1, 1, {A_FlickyAim},          ANGLE_90, 32*FRACUNIT, S_FLICKY_12_RUN1}, // S_FLICKY_12_AIM
	{SPR_FL12, 1, 2, {A_FlickyHop},                 1, 12*FRACUNIT, S_FLICKY_12_RUN2}, // S_FLICKY_12_RUN1
	{SPR_FL12, 2, 3, {A_FlickyHop},                 1, 12*FRACUNIT, S_FLICKY_12_RUN3}, // S_FLICKY_12_RUN2
	{SPR_FL12, 3, 3, {A_FlickyHop},                 1, 12*FRACUNIT, S_FLICKY_12_AIM},  // S_FLICKY_12_RUN3
	{SPR_FL12, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_12_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_12, 384*FRACUNIT, S_FLICKY_12_CENTER},        // S_FLICKY_12_CENTER

	// Bear
	{SPR_FL13, 0, 2, {A_FlickyCheck}, S_FLICKY_13_AIM,                0, S_FLICKY_13_OUT},  // S_FLICKY_13_OUT
	{SPR_FL13, 1, 1, {A_FlickyAim},             ANG30,      32*FRACUNIT, S_FLICKY_13_HOP},  // S_FLICKY_13_AIM
	{SPR_FL13, 1, 1, {A_FlickyHop},        5*FRACUNIT,       3*FRACUNIT, S_FLICKY_13_UP},   // S_FLICKY_13_HOP
	{SPR_FL13, 2, 2, {A_FlickyCheck}, S_FLICKY_13_AIM, S_FLICKY_13_DOWN, S_FLICKY_13_UP},   // S_FLICKY_13_UP
	{SPR_FL13, 3, 2, {A_FlickyCheck}, S_FLICKY_13_AIM,                0, S_FLICKY_13_DOWN}, // S_FLICKY_13_DOWN
	{SPR_FL13, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_13_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_13, 384*FRACUNIT, S_FLICKY_13_CENTER},        // S_FLICKY_13_CENTER

	// Dove
	{SPR_FL14, 0, 2, {A_FlickyCheck}, S_FLICKY_14_FLAP1, S_FLICKY_14_FLAP1, S_FLICKY_14_OUT},   // S_FLICKY_14_OUT
	{SPR_FL14, 1, 3, {A_FlickySoar},         4*FRACUNIT,       32*FRACUNIT, S_FLICKY_14_FLAP2}, // S_FLICKY_14_FLAP1
	{SPR_FL14, 2, 3, {A_FlickySoar},         4*FRACUNIT,       32*FRACUNIT, S_FLICKY_14_FLAP3}, // S_FLICKY_14_FLAP2
	{SPR_FL14, 3, 3, {A_FlickySoar},         4*FRACUNIT,       32*FRACUNIT, S_FLICKY_14_FLAP1}, // S_FLICKY_14_FLAP3
	{SPR_FL14, FF_ANIMATE|1, -1, {NULL}, 2, 3, S_NULL}, // S_FLICKY_14_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_14, 384*FRACUNIT, S_FLICKY_14_CENTER},        // S_FLICKY_14_CENTER

	// Cat
	{SPR_FL15, 0, 2, {A_FlickyCheck}, S_FLICKY_15_AIM,                0, S_FLICKY_15_OUT},  // S_FLICKY_15_OUT
	{SPR_FL15, 1, 1, {A_FlickyAim},             ANG30,      32*FRACUNIT, S_FLICKY_15_HOP},  // S_FLICKY_15_AIM
	{SPR_FL15, 1, 1, {A_FlickyFlounder},   2*FRACUNIT,       6*FRACUNIT, S_FLICKY_15_UP},   // S_FLICKY_15_HOP
	{SPR_FL15, 2, 2, {A_FlickyCheck}, S_FLICKY_15_AIM, S_FLICKY_15_DOWN, S_FLICKY_15_UP},   // S_FLICKY_15_UP
	{SPR_FL15, 3, 2, {A_FlickyCheck}, S_FLICKY_15_AIM,                0, S_FLICKY_15_DOWN}, // S_FLICKY_15_DOWN
	{SPR_FL15, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_FLICKY_15_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_15, 384*FRACUNIT, S_FLICKY_15_CENTER},        // S_FLICKY_15_CENTER

	// Canary
	{SPR_FL16, 0, 2, {A_FlickyHeightCheck}, S_FLICKY_16_FLAP1,          0, S_FLICKY_16_OUT},   // S_FLICKY_16_OUT
	{SPR_FL16, 1, 3, {A_FlickyFly},                4*FRACUNIT, 8*FRACUNIT, S_FLICKY_16_FLAP2}, // S_FLICKY_16_FLAP1
	{SPR_FL16, 2, 3, {A_SetObjectFlags},         MF_NOGRAVITY,          1, S_FLICKY_16_FLAP3}, // S_FLICKY_16_FLAP2
	{SPR_FL16, 3, 3, {A_FlickyHeightCheck}, S_FLICKY_16_FLAP1,          0, S_FLICKY_16_FLAP3}, // S_FLICKY_16_FLAP3
	{SPR_FL16, FF_ANIMATE|1, -1, {NULL}, 2, 3, S_NULL}, // S_FLICKY_16_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_FLICKY_16, 384*FRACUNIT, S_FLICKY_16_CENTER},        // S_FLICKY_16_CENTER

	// Spider
	{SPR_FS01, 0, 2, {A_FlickyCheck}, S_SECRETFLICKY_01_AIM,                      0, S_SECRETFLICKY_01_OUT},  // S_SECRETFLICKY_01_OUT
	{SPR_FS01, 1, 1, {A_FlickyAim},                   ANG30,            32*FRACUNIT, S_SECRETFLICKY_01_HOP},  // S_SECRETFLICKY_01_AIM
	{SPR_FS01, 1, 1, {A_FlickyFlounder},         2*FRACUNIT,             6*FRACUNIT, S_SECRETFLICKY_01_UP},   // S_SECRETFLICKY_01_HOP
	{SPR_FS01, 2, 2, {A_FlickyCheck}, S_SECRETFLICKY_01_AIM, S_SECRETFLICKY_01_DOWN, S_SECRETFLICKY_01_UP},   // S_SECRETFLICKY_01_UP
	{SPR_FS01, 3, 2, {A_FlickyCheck}, S_SECRETFLICKY_01_AIM,                      0, S_SECRETFLICKY_01_DOWN}, // S_SECRETFLICKY_01_DOWN
	{SPR_FS01, FF_ANIMATE|1, -1, {NULL}, 2, 4, S_NULL}, // S_SECRETFLICKY_01_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_SECRETFLICKY_01, 384*FRACUNIT, S_SECRETFLICKY_01_CENTER},        // S_SECRETFLICKY_01_CENTER

	// Bat
	{SPR_FS02, 0, 2, {A_FlickyHeightCheck}, S_SECRETFLICKY_02_FLAP1, S_SECRETFLICKY_02_FLAP1, S_SECRETFLICKY_02_OUT},   // S_SECRETFLICKY_02_OUT
	{SPR_FS02, 1, 3, {A_FlickyFly},                      4*FRACUNIT,             16*FRACUNIT, S_SECRETFLICKY_02_FLAP2}, // S_SECRETFLICKY_02_FLAP1
	{SPR_FS02, 2, 3, {A_FlickyFly},                      4*FRACUNIT,             16*FRACUNIT, S_SECRETFLICKY_02_FLAP3}, // S_SECRETFLICKY_02_FLAP2
	{SPR_FS02, 3, 3, {A_FlickyFly},                      4*FRACUNIT,             16*FRACUNIT, S_SECRETFLICKY_02_FLAP1}, // S_SECRETFLICKY_02_FLAP3
	{SPR_FS02, FF_ANIMATE|1, -1, {NULL}, 2, 2, S_NULL}, // S_SECRETFLICKY_02_STAND
	{SPR_NULL, 0, 15, {A_FlickyCenter}, MT_SECRETFLICKY_02, 384*FRACUNIT, S_SECRETFLICKY_02_CENTER},        // S_SECRETFLICKY_02_CENTER

	// Fan
	{SPR_FANS, 0, 1, {A_FanBubbleSpawn}, 2048, 0, S_FAN2}, // S_FAN
	{SPR_FANS, 1, 1, {A_FanBubbleSpawn}, 1024, 0, S_FAN3}, // S_FAN2
	{SPR_FANS, 2, 1, {A_FanBubbleSpawn},  512, 0, S_FAN4}, // S_FAN3
	{SPR_FANS, 3, 1, {A_FanBubbleSpawn}, 1024, 0, S_FAN5}, // S_FAN4
	{SPR_FANS, 4, 1, {A_FanBubbleSpawn},  512, 0, S_FAN},  // S_FAN5

	// Steam Riser
	{SPR_STEM, 0, 2, {A_SetSolidSteam}, 0, 0, S_STEAM2},   // S_STEAM1
	{SPR_STEM, 1, 2, {A_UnsetSolidSteam}, 0, 0, S_STEAM3}, // S_STEAM2
	{SPR_STEM, 2, 2, {NULL}, 0, 0, S_STEAM4},              // S_STEAM3
	{SPR_STEM, 3, 2, {NULL}, 0, 0, S_STEAM5},              // S_STEAM4
	{SPR_STEM, 4, 2, {NULL}, 0, 0, S_STEAM6},              // S_STEAM5
	{SPR_STEM, 5, 2, {NULL}, 0, 0, S_STEAM7},              // S_STEAM6
	{SPR_STEM, 6, 2, {NULL}, 0, 0, S_STEAM8},              // S_STEAM7
	{SPR_NULL, 0, 18, {NULL}, 0, 0, S_STEAM1},             // S_STEAM8

	// Bumpers
	{SPR_BUMP, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL},   3, 4, S_NULL},   // S_BUMPER
	{SPR_BUMP, FF_ANIMATE|4,             12, {A_Pain}, 1, 3, S_BUMPER}, //S_BUMPERHIT

	// Balloons
	{SPR_BLON, FF_ANIMATE, -1, {NULL}, 2, 5, S_NULL}, // S_BALLOON
	{SPR_BLON, 3, 0, {A_RemoteDamage},   0, 1, S_BALLOONPOP2}, // S_BALLOONPOP1
	{SPR_BLON, 3, 1, {A_Pain},           0, 0, S_BALLOONPOP3}, // S_BALLOONPOP2
	{SPR_BLON, 4, 1, {NULL},             0, 0, S_BALLOONPOP4}, // S_BALLOONPOP3
	{SPR_NULL, 0, TICRATE, {A_CheckFlags2}, MF2_AMBUSH, S_BALLOONPOP5, S_NULL}, // S_BALLOONPOP4
	{SPR_NULL, 0, 15*TICRATE, {NULL},    0, 0, S_BALLOONPOP6}, // S_BALLOONPOP5
	{SPR_NULL, 0, 0, {A_SpawnFreshCopy}, 0, 0, S_NULL},        // S_BALLOONPOP6

	// Yellow Spring
	{SPR_SPRY, 0, -1, {NULL}, 0, 0, S_NULL},           // S_YELLOWSPRING
	{SPR_SPRY, 4, 4, {A_Pain}, 0, 0, S_YELLOWSPRING3}, // S_YELLOWSPRING2
	{SPR_SPRY, 3, 1, {NULL}, 0, 0, S_YELLOWSPRING4},   // S_YELLOWSPRING3
	{SPR_SPRY, 2, 1, {NULL}, 0, 0, S_YELLOWSPRING5},   // S_YELLOWSPRING4
	{SPR_SPRY, 1, 1, {NULL}, 0, 0, S_YELLOWSPRING},    // S_YELLOWSPRING5

	// Red Spring
	{SPR_SPRR, 0, -1, {NULL}, 0, 0, S_NULL},        // S_REDSPRING
	{SPR_SPRR, 4, 4, {A_Pain}, 0, 0, S_REDSPRING3}, // S_REDSPRING2
	{SPR_SPRR, 3, 1, {NULL}, 0, 0, S_REDSPRING4},   // S_REDSPRING3
	{SPR_SPRR, 2, 1, {NULL}, 0, 0, S_REDSPRING5},   // S_REDSPRING4
	{SPR_SPRR, 1, 1, {NULL}, 0, 0, S_REDSPRING},    // S_REDSPRING5

	// Blue Spring
	{SPR_SPRB, 0, -1, {NULL}, 0, 0, S_NULL},         // S_BLUESPRING
	{SPR_SPRB, 4, 4, {A_Pain}, 0, 0, S_BLUESPRING3}, // S_BLUESPRING2
	{SPR_SPRB, 3, 1, {NULL}, 0, 0, S_BLUESPRING4},   // S_BLUESPRING3
	{SPR_SPRB, 2, 1, {NULL}, 0, 0, S_BLUESPRING5},   // S_BLUESPRING4
	{SPR_SPRB, 1, 1, {NULL}, 0, 0, S_BLUESPRING},    // S_BLUESPRING5

	// Yellow Diagonal Spring
	{SPR_YSPR, 0, -1, {NULL}, 0, 0, S_NULL},    // S_YDIAG1
	{SPR_YSPR, 1, 1, {A_Pain}, 0, 0, S_YDIAG3}, // S_YDIAG2
	{SPR_YSPR, 2, 1, {NULL}, 0, 0, S_YDIAG4},   // S_YDIAG3
	{SPR_YSPR, 3, 1, {NULL}, 0, 0, S_YDIAG5},   // S_YDIAG4
	{SPR_YSPR, 4, 1, {NULL}, 0, 0, S_YDIAG6},   // S_YDIAG5
	{SPR_YSPR, 3, 1, {NULL}, 0, 0, S_YDIAG7},   // S_YDIAG6
	{SPR_YSPR, 2, 1, {NULL}, 0, 0, S_YDIAG8},   // S_YDIAG7
	{SPR_YSPR, 1, 1, {NULL}, 0, 0, S_YDIAG1},   // S_YDIAG8

	// Red Diagonal Spring
	{SPR_RSPR, 0, -1, {NULL}, 0, 0, S_NULL},    // S_RDIAG1
	{SPR_RSPR, 1, 1, {A_Pain}, 0, 0, S_RDIAG3}, // S_RDIAG2
	{SPR_RSPR, 2, 1, {NULL}, 0, 0, S_RDIAG4},   // S_RDIAG3
	{SPR_RSPR, 3, 1, {NULL}, 0, 0, S_RDIAG5},   // S_RDIAG4
	{SPR_RSPR, 4, 1, {NULL}, 0, 0, S_RDIAG6},   // S_RDIAG5
	{SPR_RSPR, 3, 1, {NULL}, 0, 0, S_RDIAG7},   // S_RDIAG6
	{SPR_RSPR, 2, 1, {NULL}, 0, 0, S_RDIAG8},   // S_RDIAG7
	{SPR_RSPR, 1, 1, {NULL}, 0, 0, S_RDIAG1},   // S_RDIAG8

	// Blue Diagonal Spring
	{SPR_BSPR, 0, -1, {NULL}, 0, 0, S_NULL},    // S_BDIAG1
	{SPR_BSPR, 1, 1, {A_Pain}, 0, 0, S_BDIAG3}, // S_BDIAG2
	{SPR_BSPR, 2, 1, {NULL}, 0, 0, S_BDIAG4},   // S_BDIAG3
	{SPR_BSPR, 3, 1, {NULL}, 0, 0, S_BDIAG5},   // S_BDIAG4
	{SPR_BSPR, 4, 1, {NULL}, 0, 0, S_BDIAG6},   // S_BDIAG5
	{SPR_BSPR, 3, 1, {NULL}, 0, 0, S_BDIAG7},   // S_BDIAG6
	{SPR_BSPR, 2, 1, {NULL}, 0, 0, S_BDIAG8},   // S_BDIAG7
	{SPR_BSPR, 1, 1, {NULL}, 0, 0, S_BDIAG1},   // S_BDIAG8

	// Yellow Side Spring
	{SPR_SSWY, 0, -1, {NULL}, 0, 0, S_NULL},    // S_YHORIZ1
	{SPR_SSWY, 1, 1, {A_Pain}, 0, 0, S_YHORIZ3}, // S_YHORIZ2
	{SPR_SSWY, 2, 1, {NULL}, 0, 0, S_YHORIZ4},   // S_YHORIZ3
	{SPR_SSWY, 3, 1, {NULL}, 0, 0, S_YHORIZ5},   // S_YHORIZ4
	{SPR_SSWY, 4, 1, {NULL}, 0, 0, S_YHORIZ6},   // S_YHORIZ5
	{SPR_SSWY, 3, 1, {NULL}, 0, 0, S_YHORIZ7},   // S_YHORIZ6
	{SPR_SSWY, 2, 1, {NULL}, 0, 0, S_YHORIZ8},   // S_YHORIZ7
	{SPR_SSWY, 1, 1, {NULL}, 0, 0, S_YHORIZ1},   // S_YHORIZ8

	// Red Side Spring
	{SPR_SSWR, 0, -1, {NULL}, 0, 0, S_NULL},    // S_RHORIZ1
	{SPR_SSWR, 1, 1, {A_Pain}, 0, 0, S_RHORIZ3}, // S_RHORIZ2
	{SPR_SSWR, 2, 1, {NULL}, 0, 0, S_RHORIZ4},   // S_RHORIZ3
	{SPR_SSWR, 3, 1, {NULL}, 0, 0, S_RHORIZ5},   // S_RHORIZ4
	{SPR_SSWR, 4, 1, {NULL}, 0, 0, S_RHORIZ6},   // S_RHORIZ5
	{SPR_SSWR, 3, 1, {NULL}, 0, 0, S_RHORIZ7},   // S_RHORIZ6
	{SPR_SSWR, 2, 1, {NULL}, 0, 0, S_RHORIZ8},   // S_RHORIZ7
	{SPR_SSWR, 1, 1, {NULL}, 0, 0, S_RHORIZ1},   // S_RHORIZ8

	// Blue Side Spring
	{SPR_SSWB, 0, -1, {NULL}, 0, 0, S_NULL},    // S_BHORIZ1
	{SPR_SSWB, 1, 1, {A_Pain}, 0, 0, S_BHORIZ3}, // S_BHORIZ2
	{SPR_SSWB, 2, 1, {NULL}, 0, 0, S_BHORIZ4},   // S_BHORIZ3
	{SPR_SSWB, 3, 1, {NULL}, 0, 0, S_BHORIZ5},   // S_BHORIZ4
	{SPR_SSWB, 4, 1, {NULL}, 0, 0, S_BHORIZ6},   // S_BHORIZ5
	{SPR_SSWB, 3, 1, {NULL}, 0, 0, S_BHORIZ7},   // S_BHORIZ6
	{SPR_SSWB, 2, 1, {NULL}, 0, 0, S_BHORIZ8},   // S_BHORIZ7
	{SPR_SSWB, 1, 1, {NULL}, 0, 0, S_BHORIZ1},   // S_BHORIZ8

	// Boosters
	{SPR_NULL, 0, 1, {A_Pain}, 0, 0, S_INVISIBLE}, // S_BOOSTERSOUND
	{SPR_BSTY,                  FF_ANIMATE, -1, {NULL}, 2, 1, S_NULL}, // S_YELLOWBOOSTERROLLER
	{SPR_BSTY, 3|FF_PAPERSPRITE|FF_ANIMATE, -1, {NULL}, 2, 3, S_NULL}, // S_YELLOWBOOSTERSEG_LEFT
	{SPR_BSTY, 6|FF_PAPERSPRITE|FF_ANIMATE, -1, {NULL}, 2, 3, S_NULL}, // S_YELLOWBOOSTERSEG_RIGHT
	{SPR_BSTY, 9|FF_PAPERSPRITE,            -1, {NULL}, 0, 0, S_NULL}, // S_YELLOWBOOSTERSEG_FACE
	{SPR_BSTR,                  FF_ANIMATE, -1, {NULL}, 2, 1, S_NULL}, // S_REDBOOSTERROLLER
	{SPR_BSTR, 3|FF_PAPERSPRITE|FF_ANIMATE, -1, {NULL}, 2, 3, S_NULL}, // S_REDBOOSTERSEG_LEFT
	{SPR_BSTR, 6|FF_PAPERSPRITE|FF_ANIMATE, -1, {NULL}, 2, 3, S_NULL}, // S_REDBOOSTERSEG_RIGHT
	{SPR_BSTR, 9|FF_PAPERSPRITE,            -1, {NULL}, 0, 0, S_NULL}, // S_REDBOOSTERSEG_FACE

	// Rain
	{SPR_RAIN, FF_FULLBRIGHT|FF_TRANS50, -1, {NULL}, 0, 0, S_NULL}, // S_RAIN1
	{SPR_RAIN, FF_FULLBRIGHT|FF_TRANS50, 1, {NULL}, 0, 0, S_RAIN1}, // S_RAINRETURN

	// Snowflake
	{SPR_SNO1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SNOW1
	{SPR_SNO1, 1, -1, {NULL}, 0, 0, S_NULL}, // S_SNOW2
	{SPR_SNO1, 2, -1, {NULL}, 0, 0, S_NULL}, // S_SNOW3

	// Water Splish
	{SPR_SPLH, FF_TRANS50  , 2, {NULL}, 0, 0, S_SPLISH2}, // S_SPLISH1
	{SPR_SPLH, FF_TRANS50|1, 2, {NULL}, 0, 0, S_SPLISH3}, // S_SPLISH2
	{SPR_SPLH, FF_TRANS50|2, 2, {NULL}, 0, 0, S_SPLISH4}, // S_SPLISH3
	{SPR_SPLH, FF_TRANS50|3, 2, {NULL}, 0, 0, S_SPLISH5}, // S_SPLISH4
	{SPR_SPLH, FF_TRANS50|4, 2, {NULL}, 0, 0, S_SPLISH6}, // S_SPLISH5
	{SPR_SPLH, FF_TRANS50|5, 2, {NULL}, 0, 0, S_SPLISH7}, // S_SPLISH6
	{SPR_SPLH, FF_TRANS50|6, 2, {NULL}, 0, 0, S_SPLISH8}, // S_SPLISH7
	{SPR_SPLH, FF_TRANS50|7, 2, {NULL}, 0, 0, S_SPLISH9}, // S_SPLISH8
	{SPR_SPLH, FF_TRANS50|8, 2, {NULL}, 0, 0, S_NULL},    // S_SPLISH9

	// Lava splish
	{SPR_LSPL, FF_ANIMATE, 16, {NULL}, 7, 2, S_NULL}, // S_LAVASPLISH

	// Water Splash
	{SPR_SPLA, FF_TRANS50  , 3, {NULL}, 0, 0, S_SPLASH2},    // S_SPLASH1
	{SPR_SPLA, FF_TRANS70|1, 3, {NULL}, 0, 0, S_SPLASH3},    // S_SPLASH2
	{SPR_SPLA, FF_TRANS90|2, 3, {NULL}, 0, 0, S_RAINRETURN}, // S_SPLASH3

	// Smoke
	{SPR_SMOK, FF_TRANS50  , 4, {NULL}, 0, 0, S_SMOKE2}, // S_SMOKE1
	{SPR_SMOK, FF_TRANS50|1, 5, {NULL}, 0, 0, S_SMOKE3}, // S_SMOKE2
	{SPR_SMOK, FF_TRANS50|2, 6, {NULL}, 0, 0, S_SMOKE4}, // S_SMOKE3
	{SPR_SMOK, FF_TRANS50|3, 7, {NULL}, 0, 0, S_SMOKE5}, // S_SMOKE4
	{SPR_SMOK, FF_TRANS50|4, 8, {NULL}, 0, 0, S_NULL},   // S_SMOKE5

	// Bubbles
	{SPR_BUBL, FF_TRANS50,   1, {A_BubbleRise}, 0, 1024, S_SMALLBUBBLE},  // S_SMALLBUBBLE
	{SPR_BUBL, FF_TRANS50|1, 1, {A_BubbleRise}, 0, 1024, S_MEDIUMBUBBLE}, // S_MEDIUMBUBBLE

	// Extra Large Bubble (breathable)
	{SPR_BUBL, FF_TRANS50|FF_FULLBRIGHT|2,   8, {A_BubbleRise}, 0, 1024, S_LARGEBUBBLE2}, // S_LARGEBUBBLE1
	{SPR_BUBL, FF_TRANS50|FF_FULLBRIGHT|3,   8, {A_BubbleRise}, 0, 1024, S_EXTRALARGEBUBBLE}, // S_LARGEBUBBLE2
	{SPR_BUBL, FF_TRANS50|FF_FULLBRIGHT|4,  16, {A_BubbleRise}, 0, 1024, S_EXTRALARGEBUBBLE}, // S_EXTRALARGEBUBBLE

	// Extra Large Bubble goes POP!
	{SPR_BUBL, 5, 16, {NULL}, 0, 0, S_NULL}, // S_POP1

	{SPR_WZAP, FF_TRANS10|FF_ANIMATE|FF_RANDOMANIM, 4, {NULL}, 3, 2, S_NULL},  // S_WATERZAP

	// Spindash dust
	{SPR_DUST,            0, 7, {NULL}, 0, 0, S_SPINDUST2}, // S_SPINDUST1
	{SPR_DUST,            1, 6, {NULL}, 0, 0, S_SPINDUST3}, // S_SPINDUST2
	{SPR_DUST, FF_TRANS30|2, 4, {NULL}, 0, 0, S_SPINDUST4}, // S_SPINDUST3
	{SPR_DUST, FF_TRANS60|3, 3, {NULL}, 0, 0, S_NULL}, // S_SPINDUST4
	{SPR_BUBL,            0, 7, {NULL}, 0, 0, S_SPINDUST_BUBBLE2}, // S_SPINDUST_BUBBLE1
	{SPR_BUBL,            0, 6, {NULL}, 0, 0, S_SPINDUST_BUBBLE3}, // S_SPINDUST_BUBBLE2
	{SPR_BUBL, FF_TRANS30|0, 4, {NULL}, 0, 0, S_SPINDUST_BUBBLE4}, // S_SPINDUST_BUBBLE3
	{SPR_BUBL, FF_TRANS60|0, 3, {NULL}, 0, 0, S_NULL}, // S_SPINDUST_BUBBLE4
	{SPR_FPRT,            0, 7, {NULL}, 0, 0, S_SPINDUST_FIRE2}, // S_SPINDUST_FIRE1
	{SPR_FPRT,            0, 6, {NULL}, 0, 0, S_SPINDUST_FIRE3}, // S_SPINDUST_FIRE2
	{SPR_FPRT, FF_TRANS30|0, 4, {NULL}, 0, 0, S_SPINDUST_FIRE4}, // S_SPINDUST_FIRE3
	{SPR_FPRT, FF_TRANS60|0, 3, {NULL}, 0, 0, S_NULL}, // S_SPINDUST_FIRE4


	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50,    2, {NULL}, 0, 0, S_FOG2},  // S_FOG1
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|1,  2, {NULL}, 0, 0, S_FOG3},  // S_FOG2
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|2,  2, {NULL}, 0, 0, S_FOG4},  // S_FOG3
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|3,  2, {NULL}, 0, 0, S_FOG5},  // S_FOG4
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|4,  2, {NULL}, 0, 0, S_FOG6},  // S_FOG5
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|5,  2, {NULL}, 0, 0, S_FOG7},  // S_FOG6
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|6,  2, {NULL}, 0, 0, S_FOG8},  // S_FOG7
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|7,  2, {NULL}, 0, 0, S_FOG9},  // S_FOG8
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|8,  2, {NULL}, 0, 0, S_FOG10}, // S_FOG9
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|9,  2, {NULL}, 0, 0, S_FOG11}, // S_FOG10
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|10, 2, {NULL}, 0, 0, S_FOG12}, // S_FOG11
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|11, 2, {NULL}, 0, 0, S_FOG13}, // S_FOG12
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|12, 2, {NULL}, 0, 0, S_FOG14}, // S_FOG13
	{SPR_TFOG, FF_FULLBRIGHT|FF_TRANS50|13, 2, {NULL}, 0, 0, S_NULL},  // S_FOG14

	// Flower Seed
	{SPR_SEED, FF_FULLBRIGHT|FF_ANIMATE, -1, {NULL}, 2, 2, S_NULL}, // S_SEED

	// Particle sprite
	{SPR_PRTL, 0, 2*TICRATE, {NULL}, 0, 0, S_NULL}, // S_PARTICLE

	{SPR_SCOR, 0, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRA  - 100
	{SPR_SCOR, 1, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRB  - 200
	{SPR_SCOR, 2, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRC  - 500
	{SPR_SCOR, 3, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRD  - 1000
	{SPR_SCOR, 4, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRE  - 10000
	{SPR_SCOR, 5, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRF  - 400 (mario mode)
	{SPR_SCOR, 6, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRG  - 800 (mario mode)
	{SPR_SCOR, 7, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRH  - 2000 (mario mode)
	{SPR_SCOR, 8, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRI  - 4000 (mario mode)
	{SPR_SCOR, 9, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRJ  - 8000 (mario mode)
	{SPR_SCOR, 10, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRK - 1UP (mario mode)
	{SPR_SCOR, 11, 32, {A_ScoreRise}, 0, 0, S_NULL}, // S_SCRL - 10

	// Drowning Timer Numbers
	{SPR_DRWN, 0, 40, {NULL}, 0, 0, S_NULL}, // S_ZERO1
	{SPR_DRWN, 1, 40, {NULL}, 0, 0, S_NULL}, // S_ONE1
	{SPR_DRWN, 2, 40, {NULL}, 0, 0, S_NULL}, // S_TWO1
	{SPR_DRWN, 3, 40, {NULL}, 0, 0, S_NULL}, // S_THREE1
	{SPR_DRWN, 4, 40, {NULL}, 0, 0, S_NULL}, // S_FOUR1
	{SPR_DRWN, 5, 40, {NULL}, 0, 0, S_NULL}, // S_FIVE1

	{SPR_DRWN,  6, 40, {NULL}, 0, 0, S_NULL}, // S_ZERO2
	{SPR_DRWN,  7, 40, {NULL}, 0, 0, S_NULL}, // S_ONE2
	{SPR_DRWN,  8, 40, {NULL}, 0, 0, S_NULL}, // S_TWO2
	{SPR_DRWN,  9, 40, {NULL}, 0, 0, S_NULL}, // S_THREE2
	{SPR_DRWN, 10, 40, {NULL}, 0, 0, S_NULL}, // S_FOUR2
	{SPR_DRWN, 11, 40, {NULL}, 0, 0, S_NULL}, // S_FIVE2

	// Flight indicator
	{SPR_FLII, FF_FULLBRIGHT|FF_ANIMATE|0, -1, {NULL}, 4, 4, S_NULL}, // S_FLIGHTINDICATOR

	{SPR_LCKN,   FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL}, // S_LOCKON1
	{SPR_LCKN, 1|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL}, // S_LOCKON2
	{SPR_LCKN, 2|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL}, // S_LOCKON3
	{SPR_LCKN, 3|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL}, // S_LOCKON4

	{SPR_LCKN,   FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_LOCKONINF1
	{SPR_LCKN, 1|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_LOCKONINF2
	{SPR_LCKN, 2|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_LOCKONINF3
	{SPR_LCKN, 3|FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_LOCKONINF4

	{SPR_TTAG, FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL}, // S_TTAG

	// CTF Sign
	{SPR_GFLG, FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL}, // S_GOTFLAG

	// Finish flag
	{SPR_FNSF,    FF_TRANS30, -1, {NULL}, 0, 0, S_NULL}, // S_FINISHFLAG

	{SPR_CORK,             0, -1, {NULL}, 0, 0, S_NULL}, // S_CORK
	{SPR_LHRT, FF_FULLBRIGHT, -1, {NULL}, 0, 0, S_NULL}, // S_LHRT

	// Red Rings (thrown)
	{SPR_RRNG, FF_FULLBRIGHT,   1, {A_ThrownRing}, 0, 0, S_RRNG2}, // S_RRNG1
	{SPR_RRNG, FF_FULLBRIGHT|1, 1, {A_ThrownRing}, 0, 0, S_RRNG3}, // S_RRNG2
	{SPR_RRNG, FF_FULLBRIGHT|2, 1, {A_ThrownRing}, 0, 0, S_RRNG4}, // S_RRNG3
	{SPR_RRNG, FF_FULLBRIGHT|3, 1, {A_ThrownRing}, 0, 0, S_RRNG5}, // S_RRNG4
	{SPR_RRNG, FF_FULLBRIGHT|4, 1, {A_ThrownRing}, 0, 0, S_RRNG6}, // S_RRNG5
	{SPR_RRNG, FF_FULLBRIGHT|5, 1, {A_ThrownRing}, 0, 0, S_RRNG7}, // S_RRNG6
	{SPR_RRNG, FF_FULLBRIGHT|6, 1, {A_ThrownRing}, 0, 0, S_RRNG1}, // S_RRNG7

	// Weapon Ring Ammo
	{SPR_RNGB, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 34, 1, S_BOUNCERINGAMMO},    // S_BOUNCERINGAMMO
	{SPR_RNGR, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 34, 1, S_RAILRINGAMMO},      // S_RAILRINGAMMO
	{SPR_RNGI, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 34, 1, S_INFINITYRINGAMMO},  // S_INFINITYRINGAMMO
	{SPR_RNGA, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 34, 1, S_AUTOMATICRINGAMMO}, // S_AUTOMATICRINGAMMO
	{SPR_RNGE, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 34, 1, S_EXPLOSIONRINGAMMO}, // S_EXPLOSIONRINGAMMO
	{SPR_RNGS, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 34, 1, S_SCATTERRINGAMMO},   // S_SCATTERRINGAMMO
	{SPR_RNGG, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 34, 1, S_GRENADERINGAMMO},   // S_GRENADERINGAMMO

	// Bounce Ring Pickup
	{SPR_PIKB, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 15, 1, S_BOUNCEPICKUP},  // S_BOUNCEPICKUP

	{SPR_PIKB,  0, 1, {NULL}, 0, 0, S_BOUNCEPICKUPFADE2}, // S_BOUNCEPICKUPFADE1
	{SPR_PIKB,  2, 1, {NULL}, 0, 0, S_BOUNCEPICKUPFADE3}, // S_BOUNCEPICKUPFADE2
	{SPR_PIKB,  4, 1, {NULL}, 0, 0, S_BOUNCEPICKUPFADE4}, // S_BOUNCEPICKUPFADE3
	{SPR_PIKB,  6, 1, {NULL}, 0, 0, S_BOUNCEPICKUPFADE5}, // S_BOUNCEPICKUPFADE4
	{SPR_PIKB,  8, 1, {NULL}, 0, 0, S_BOUNCEPICKUPFADE6}, // S_BOUNCEPICKUPFADE5
	{SPR_PIKB, 10, 1, {NULL}, 0, 0, S_BOUNCEPICKUPFADE7}, // S_BOUNCEPICKUPFADE6
	{SPR_PIKB, 12, 1, {NULL}, 0, 0, S_BOUNCEPICKUPFADE8}, // S_BOUNCEPICKUPFADE7
	{SPR_PIKB, 14, 1, {NULL}, 0, 0, S_BOUNCEPICKUPFADE1}, // S_BOUNCEPICKUPFADE8

	// Rail Ring Pickup
	{SPR_PIKR, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 15, 1, S_RAILPICKUP},  // S_RAILPICKUP

	{SPR_PIKR,  0, 1, {NULL}, 0, 0, S_RAILPICKUPFADE2}, // S_RAILPICKUPFADE1
	{SPR_PIKR,  2, 1, {NULL}, 0, 0, S_RAILPICKUPFADE3}, // S_RAILPICKUPFADE2
	{SPR_PIKR,  4, 1, {NULL}, 0, 0, S_RAILPICKUPFADE4}, // S_RAILPICKUPFADE3
	{SPR_PIKR,  6, 1, {NULL}, 0, 0, S_RAILPICKUPFADE5}, // S_RAILPICKUPFADE4
	{SPR_PIKR,  8, 1, {NULL}, 0, 0, S_RAILPICKUPFADE6}, // S_RAILPICKUPFADE5
	{SPR_PIKR, 10, 1, {NULL}, 0, 0, S_RAILPICKUPFADE7}, // S_RAILPICKUPFADE6
	{SPR_PIKR, 12, 1, {NULL}, 0, 0, S_RAILPICKUPFADE8}, // S_RAILPICKUPFADE7
	{SPR_PIKR, 14, 1, {NULL}, 0, 0, S_RAILPICKUPFADE1}, // S_RAILPICKUPFADE8

	// Auto Ring Pickup
	{SPR_PIKA, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 15, 1, S_AUTOPICKUP},  // S_AUTOPICKUP

	{SPR_PIKA,  0, 1, {NULL}, 0, 0, S_AUTOPICKUPFADE2}, // S_AUTOPICKUPFADE1
	{SPR_PIKA,  2, 1, {NULL}, 0, 0, S_AUTOPICKUPFADE3}, // S_AUTOPICKUPFADE2
	{SPR_PIKA,  4, 1, {NULL}, 0, 0, S_AUTOPICKUPFADE4}, // S_AUTOPICKUPFADE3
	{SPR_PIKA,  6, 1, {NULL}, 0, 0, S_AUTOPICKUPFADE5}, // S_AUTOPICKUPFADE4
	{SPR_PIKA,  8, 1, {NULL}, 0, 0, S_AUTOPICKUPFADE6}, // S_AUTOPICKUPFADE5
	{SPR_PIKA, 10, 1, {NULL}, 0, 0, S_AUTOPICKUPFADE7}, // S_AUTOPICKUPFADE6
	{SPR_PIKA, 12, 1, {NULL}, 0, 0, S_AUTOPICKUPFADE8}, // S_AUTOPICKUPFADE7
	{SPR_PIKA, 14, 1, {NULL}, 0, 0, S_AUTOPICKUPFADE1}, // S_AUTOPICKUPFADE8

	// Explode Ring Pickup
	{SPR_PIKE, FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 15, 1, S_EXPLODEPICKUP},  // S_EXPLODEPICKUP

	{SPR_PIKE,  0, 1, {NULL}, 0, 0, S_EXPLODEPICKUPFADE2}, // S_EXPLODEPICKUPFADE1
	{SPR_PIKE,  2, 1, {NULL}, 0, 0, S_EXPLODEPICKUPFADE3}, // S_EXPLODEPICKUPFADE2
	{SPR_PIKE,  4, 1, {NULL}, 0, 0, S_EXPLODEPICKUPFADE4}, // S_EXPLODEPICKUPFADE3
	{SPR_PIKE,  6, 1, {NULL}, 0, 0, S_EXPLODEPICKUPFADE5}, // S_EXPLODEPICKUPFADE4
	{SPR_PIKE,  8, 1, {NULL}, 0, 0, S_EXPLODEPICKUPFADE6}, // S_EXPLODEPICKUPFADE5
	{SPR_PIKE, 10, 1, {NULL}, 0, 0, S_EXPLODEPICKUPFADE7}, // S_EXPLODEPICKUPFADE6
	{SPR_PIKE, 12, 1, {NULL}, 0, 0, S_EXPLODEPICKUPFADE8}, // S_EXPLODEPICKUPFADE7
	{SPR_PIKE, 14, 1, {NULL}, 0, 0, S_EXPLODEPICKUPFADE1}, // S_EXPLODEPICKUPFADE8

	// Scatter Ring Pickup
	{SPR_PIKS,  FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 15, 1, S_SCATTERPICKUP},  // S_SCATTERPICKUP

	{SPR_PIKS,  0, 1, {NULL}, 0, 0, S_SCATTERPICKUPFADE2}, // S_SCATTERPICKUPFADE1
	{SPR_PIKS,  2, 1, {NULL}, 0, 0, S_SCATTERPICKUPFADE3}, // S_SCATTERPICKUPFADE2
	{SPR_PIKS,  4, 1, {NULL}, 0, 0, S_SCATTERPICKUPFADE4}, // S_SCATTERPICKUPFADE3
	{SPR_PIKS,  6, 1, {NULL}, 0, 0, S_SCATTERPICKUPFADE5}, // S_SCATTERPICKUPFADE4
	{SPR_PIKS,  8, 1, {NULL}, 0, 0, S_SCATTERPICKUPFADE6}, // S_SCATTERPICKUPFADE5
	{SPR_PIKS, 10, 1, {NULL}, 0, 0, S_SCATTERPICKUPFADE7}, // S_SCATTERPICKUPFADE6
	{SPR_PIKS, 12, 1, {NULL}, 0, 0, S_SCATTERPICKUPFADE8}, // S_SCATTERPICKUPFADE7
	{SPR_PIKS, 14, 1, {NULL}, 0, 0, S_SCATTERPICKUPFADE1}, // S_SCATTERPICKUPFADE8

	// Grenade Ring Pickup
	{SPR_PIKG,  FF_ANIMATE|FF_GLOBALANIM, -1, {NULL}, 15, 1, S_GRENADEPICKUP},  // S_GRENADEPICKUP

	{SPR_PIKG,  0, 1, {NULL}, 0, 0, S_GRENADEPICKUPFADE2}, // S_GRENADEPICKUPFADE1
	{SPR_PIKG,  2, 1, {NULL}, 0, 0, S_GRENADEPICKUPFADE3}, // S_GRENADEPICKUPFADE2
	{SPR_PIKG,  4, 1, {NULL}, 0, 0, S_GRENADEPICKUPFADE4}, // S_GRENADEPICKUPFADE3
	{SPR_PIKG,  6, 1, {NULL}, 0, 0, S_GRENADEPICKUPFADE5}, // S_GRENADEPICKUPFADE4
	{SPR_PIKG,  8, 1, {NULL}, 0, 0, S_GRENADEPICKUPFADE6}, // S_GRENADEPICKUPFADE5
	{SPR_PIKG, 10, 1, {NULL}, 0, 0, S_GRENADEPICKUPFADE7}, // S_GRENADEPICKUPFADE6
	{SPR_PIKG, 12, 1, {NULL}, 0, 0, S_GRENADEPICKUPFADE8}, // S_GRENADEPICKUPFADE7
	{SPR_PIKG, 14, 1, {NULL}, 0, 0, S_GRENADEPICKUPFADE1}, // S_GRENADEPICKUPFADE8

	// Thrown Weapon Rings
	{SPR_RNGB, FF_FULLBRIGHT   , 1, {A_ThrownRing}, 0, 0, S_THROWNBOUNCE2}, // S_THROWNBOUNCE1
	{SPR_RNGB, FF_FULLBRIGHT| 5, 1, {A_ThrownRing}, 0, 0, S_THROWNBOUNCE3}, // S_THROWNBOUNCE2
	{SPR_RNGB, FF_FULLBRIGHT|10, 1, {A_ThrownRing}, 0, 0, S_THROWNBOUNCE4}, // S_THROWNBOUNCE3
	{SPR_RNGB, FF_FULLBRIGHT|15, 1, {A_ThrownRing}, 0, 0, S_THROWNBOUNCE5}, // S_THROWNBOUNCE4
	{SPR_RNGB, FF_FULLBRIGHT|20, 1, {A_ThrownRing}, 0, 0, S_THROWNBOUNCE6}, // S_THROWNBOUNCE5
	{SPR_RNGB, FF_FULLBRIGHT|25, 1, {A_ThrownRing}, 0, 0, S_THROWNBOUNCE7}, // S_THROWNBOUNCE6
	{SPR_RNGB, FF_FULLBRIGHT|30, 1, {A_ThrownRing}, 0, 0, S_THROWNBOUNCE1}, // S_THROWNBOUNCE7

	{SPR_RNGI, FF_FULLBRIGHT   , 1, {A_ThrownRing}, 0, 0, S_THROWNINFINITY2}, // S_THROWNINFINITY1
	{SPR_RNGI, FF_FULLBRIGHT| 5, 1, {A_ThrownRing}, 0, 0, S_THROWNINFINITY3}, // S_THROWNINFINITY2
	{SPR_RNGI, FF_FULLBRIGHT|10, 1, {A_ThrownRing}, 0, 0, S_THROWNINFINITY4}, // S_THROWNINFINITY3
	{SPR_RNGI, FF_FULLBRIGHT|15, 1, {A_ThrownRing}, 0, 0, S_THROWNINFINITY5}, // S_THROWNINFINITY4
	{SPR_RNGI, FF_FULLBRIGHT|20, 1, {A_ThrownRing}, 0, 0, S_THROWNINFINITY6}, // S_THROWNINFINITY5
	{SPR_RNGI, FF_FULLBRIGHT|25, 1, {A_ThrownRing}, 0, 0, S_THROWNINFINITY7}, // S_THROWNINFINITY6
	{SPR_RNGI, FF_FULLBRIGHT|30, 1, {A_ThrownRing}, 0, 0, S_THROWNINFINITY1}, // S_THROWNINFINITY7

	{SPR_TAUT, FF_FULLBRIGHT  , 1, {A_ThrownRing}, 0, 0, S_THROWNAUTOMATIC2}, // S_THROWNAUTOMATIC1
	{SPR_TAUT, FF_FULLBRIGHT|1, 1, {A_ThrownRing}, 0, 0, S_THROWNAUTOMATIC3}, // S_THROWNAUTOMATIC2
	{SPR_TAUT, FF_FULLBRIGHT|2, 1, {A_ThrownRing}, 0, 0, S_THROWNAUTOMATIC4}, // S_THROWNAUTOMATIC3
	{SPR_TAUT, FF_FULLBRIGHT|3, 1, {A_ThrownRing}, 0, 0, S_THROWNAUTOMATIC5}, // S_THROWNAUTOMATIC4
	{SPR_TAUT, FF_FULLBRIGHT|4, 1, {A_ThrownRing}, 0, 0, S_THROWNAUTOMATIC6}, // S_THROWNAUTOMATIC5
	{SPR_TAUT, FF_FULLBRIGHT|5, 1, {A_ThrownRing}, 0, 0, S_THROWNAUTOMATIC7}, // S_THROWNAUTOMATIC6
	{SPR_TAUT, FF_FULLBRIGHT|6, 1, {A_ThrownRing}, 0, 0, S_THROWNAUTOMATIC1}, // S_THROWNAUTOMATIC7

	{SPR_RNGE, FF_FULLBRIGHT   , 1, {A_ThrownRing}, 0, 0, S_THROWNEXPLOSION2}, // S_THROWNEXPLOSION1
	{SPR_RNGE, FF_FULLBRIGHT| 5, 1, {A_ThrownRing}, 0, 0, S_THROWNEXPLOSION3}, // S_THROWNEXPLOSION2
	{SPR_RNGE, FF_FULLBRIGHT|10, 1, {A_ThrownRing}, 0, 0, S_THROWNEXPLOSION4}, // S_THROWNEXPLOSION3
	{SPR_RNGE, FF_FULLBRIGHT|15, 1, {A_ThrownRing}, 0, 0, S_THROWNEXPLOSION5}, // S_THROWNEXPLOSION4
	{SPR_RNGE, FF_FULLBRIGHT|20, 1, {A_ThrownRing}, 0, 0, S_THROWNEXPLOSION6}, // S_THROWNEXPLOSION5
	{SPR_RNGE, FF_FULLBRIGHT|25, 1, {A_ThrownRing}, 0, 0, S_THROWNEXPLOSION7}, // S_THROWNEXPLOSION6
	{SPR_RNGE, FF_FULLBRIGHT|30, 1, {A_ThrownRing}, 0, 0, S_THROWNEXPLOSION1}, // S_THROWNEXPLOSION7

	{SPR_TGRE, FF_FULLBRIGHT   , 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE2},  // S_THROWNGRENADE1
	{SPR_TGRE, FF_FULLBRIGHT| 1, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE3},  // S_THROWNGRENADE2
	{SPR_TGRE, FF_FULLBRIGHT| 2, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE4},  // S_THROWNGRENADE3
	{SPR_TGRE, FF_FULLBRIGHT| 3, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE5},  // S_THROWNGRENADE4
	{SPR_TGRE, FF_FULLBRIGHT| 4, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE6},  // S_THROWNGRENADE5
	{SPR_TGRE, FF_FULLBRIGHT| 5, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE7},  // S_THROWNGRENADE6
	{SPR_TGRE, FF_FULLBRIGHT| 6, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE8},  // S_THROWNGRENADE7
	{SPR_TGRE, FF_FULLBRIGHT| 7, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE9},  // S_THROWNGRENADE8
	{SPR_TGRE, FF_FULLBRIGHT| 8, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE10}, // S_THROWNGRENADE9
	{SPR_TGRE, FF_FULLBRIGHT| 9, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE11}, // S_THROWNGRENADE10
	{SPR_TGRE, FF_FULLBRIGHT|10, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE12}, // S_THROWNGRENADE11
	{SPR_TGRE, FF_FULLBRIGHT|11, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE13}, // S_THROWNGRENADE12
	{SPR_TGRE, FF_FULLBRIGHT|12, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE14}, // S_THROWNGRENADE13
	{SPR_TGRE, FF_FULLBRIGHT|13, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE15}, // S_THROWNGRENADE14
	{SPR_TGRE, FF_FULLBRIGHT|14, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE16}, // S_THROWNGRENADE15
	{SPR_TGRE, FF_FULLBRIGHT|15, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE17}, // S_THROWNGRENADE16
	{SPR_TGRE, FF_FULLBRIGHT|16, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE18}, // S_THROWNGRENADE17
	{SPR_TGRE, FF_FULLBRIGHT|17, 1, {A_ThrownRing}, 0, 0, S_THROWNGRENADE1},  // S_THROWNGRENADE18

	{SPR_TSCR, FF_FULLBRIGHT, 1, {A_ThrownRing}, 0, 0, S_THROWNSCATTER}, // S_THROWNSCATTER

	{SPR_NULL, 0, 1, {A_RingExplode}, 0, 0, S_XPLD1}, // S_RINGEXPLODE

	// Coin
	{SPR_COIN, FF_FULLBRIGHT,   5, {NULL}, 0, 0, S_COIN2}, // S_COIN1
	{SPR_COIN, FF_FULLBRIGHT|1, 5, {NULL}, 0, 0, S_COIN3}, // S_COIN2
	{SPR_COIN, FF_FULLBRIGHT|2, 5, {NULL}, 0, 0, S_COIN1}, // S_COIN3

	// Coin Sparkle
	{SPR_CPRK, FF_FULLBRIGHT,   5, {NULL}, 0, 0, S_COINSPARKLE2}, // S_COINSPARKLE1
	{SPR_CPRK, FF_FULLBRIGHT|1, 5, {NULL}, 0, 0, S_COINSPARKLE3}, // S_COINSPARKLE2
	{SPR_CPRK, FF_FULLBRIGHT|2, 5, {NULL}, 0, 0, S_COINSPARKLE4}, // S_COINSPARKLE3
	{SPR_CPRK, FF_FULLBRIGHT|3, 5, {NULL}, 0, 0, S_NULL},         // S_COINSPARKLE4

	// Goomba
	{SPR_GOOM, 0, 6, {A_Look}, 0, 0, S_GOOMBA1B}, // S_GOOMBA1
	{SPR_GOOM, 1, 6, {A_Look}, 0, 0, S_GOOMBA1},  // S_GOOMBA1B
	{SPR_GOOM, 0, 3, {A_Chase}, 0, 0, S_GOOMBA3}, // S_GOOMBA2
	{SPR_GOOM, 0, 3, {A_Chase}, 0, 0, S_GOOMBA4}, // S_GOOMBA3
	{SPR_GOOM, 1, 3, {A_Chase}, 0, 0, S_GOOMBA5}, // S_GOOMBA4
	{SPR_GOOM, 1, 3, {A_Chase}, 0, 0, S_GOOMBA6}, // S_GOOMBA5
	{SPR_GOOM, 0, 3, {A_Chase}, 0, 0, S_GOOMBA7}, // S_GOOMBA6
	{SPR_GOOM, 0, 3, {A_Chase}, 0, 0, S_GOOMBA8}, // S_GOOMBA7
	{SPR_GOOM, 1, 3, {A_Chase}, 0, 0, S_GOOMBA9}, // S_GOOMBA8
	{SPR_GOOM, 1, 3, {A_Chase}, 0, 0, S_GOOMBA2}, // S_GOOMBA9
	{SPR_GOOM, 2, 16, {A_Scream}, 0, 0, S_NULL},  // S_GOOMBA_DEAD

	// Blue Goomba
	{SPR_BGOM, 0, 6, {A_Look}, 0, 0, S_BLUEGOOMBA1B}, // BLUEGOOMBA1
	{SPR_BGOM, 1, 6, {A_Look}, 0, 0, S_BLUEGOOMBA1},  // BLUEGOOMBA1B
	{SPR_BGOM, 0, 3, {A_Chase}, 0, 0, S_BLUEGOOMBA3}, // S_BLUEGOOMBA2
	{SPR_BGOM, 0, 3, {A_Chase}, 0, 0, S_BLUEGOOMBA4}, // S_BLUEGOOMBA3
	{SPR_BGOM, 1, 3, {A_Chase}, 0, 0, S_BLUEGOOMBA5}, // S_BLUEGOOMBA4
	{SPR_BGOM, 1, 3, {A_Chase}, 0, 0, S_BLUEGOOMBA6}, // S_BLUEGOOMBA5
	{SPR_BGOM, 0, 3, {A_Chase}, 0, 0, S_BLUEGOOMBA7}, // S_BLUEGOOMBA6
	{SPR_BGOM, 0, 3, {A_Chase}, 0, 0, S_BLUEGOOMBA8}, // S_BLUEGOOMBA7
	{SPR_BGOM, 1, 3, {A_Chase}, 0, 0, S_BLUEGOOMBA9}, // S_BLUEGOOMBA8
	{SPR_BGOM, 1, 3, {A_Chase}, 0, 0, S_BLUEGOOMBA2}, // S_BLUEGOOMBA9
	{SPR_BGOM, 2, 16, {A_Scream}, 0, 0, S_NULL},      // S_BLUEGOOMBA_DEAD

	// Fire Flower
	{SPR_FFWR, 0, 3, {NULL}, 0, 0, S_FIREFLOWER2}, // S_FIREFLOWER1
	{SPR_FFWR, 1, 3, {NULL}, 0, 0, S_FIREFLOWER3}, // S_FIREFLOWER2
	{SPR_FFWR, 2, 3, {NULL}, 0, 0, S_FIREFLOWER4}, // S_FIREFLOWER3
	{SPR_FFWR, 3, 3, {NULL}, 0, 0, S_FIREFLOWER1}, // S_FIREFLOWER4

	// Thrown Mario Fireball
	{SPR_FBLL, FF_FULLBRIGHT,   1, {A_SpawnObjectRelative}, 0, MT_FIREBALLTRAIL, S_FIREBALL},   // S_FIREBALL
	{SPR_FBLL, 1|FF_FULLBRIGHT|FF_TRANS50, 1, {A_SetScale}, FRACUNIT*3/4, 0, S_FIREBALLTRAIL2}, // S_FIREBALLTRAIL1
	{SPR_FBLL, 1|FF_FULLBRIGHT|FF_TRANS50, 8, {A_SetScale}, FRACUNIT/6, 1, S_NULL},             // S_FIREBALLTRAIL2

	// Turtle Shell
	{SPR_SHLL, 0, -1, {NULL}, 0, 0, S_NULL}, // S_SHELL

	// Puma (Mario fireball)
	{SPR_PUMA, FF_FULLBRIGHT|2, 1, {A_FishJump}, 0, MT_PUMATRAIL, S_PUMA_START2},   // S_PUMA_START1
	{SPR_PUMA, FF_FULLBRIGHT|2, 1, {A_PlaySound}, sfx_s3k70, 1 + (1<<16), S_PUMA_UP1},   // S_PUMA_START2
	{SPR_PUMA, FF_FULLBRIGHT  , 2, {A_FishJump}, 0, MT_PUMATRAIL, S_PUMA_UP2},   // S_PUMA_UP1
	{SPR_PUMA, FF_FULLBRIGHT|1, 2, {A_FishJump}, 0, MT_PUMATRAIL, S_PUMA_UP3},   // S_PUMA_UP2
	{SPR_PUMA, FF_FULLBRIGHT|2, 2, {A_FishJump}, 0, MT_PUMATRAIL, S_PUMA_UP1},   // S_PUMA_UP3
	{SPR_PUMA, FF_FULLBRIGHT|3, 2, {A_FishJump}, 0, MT_PUMATRAIL, S_PUMA_DOWN2}, // S_PUMA_DOWN1
	{SPR_PUMA, FF_FULLBRIGHT|4, 2, {A_FishJump}, 0, MT_PUMATRAIL, S_PUMA_DOWN3}, // S_PUMA_DOWN2
	{SPR_PUMA, FF_FULLBRIGHT|5, 2, {A_FishJump}, 0, MT_PUMATRAIL, S_PUMA_DOWN1}, // S_PUMA_DOWN3

	{SPR_PUMA, FF_FULLBRIGHT|FF_TRANS20|6, 4,       {NULL},        0, 0, S_PUMATRAIL2},   // S_PUMATRAIL1
	{SPR_PUMA, FF_FULLBRIGHT|FF_TRANS40|6, 5, {A_SetScale}, FRACUNIT, 1, S_PUMATRAIL3},   // S_PUMATRAIL2
	{SPR_PUMA, FF_FULLBRIGHT|FF_TRANS50|7, 4,       {NULL},        0, 0, S_PUMATRAIL4},   // S_PUMATRAIL3
	{SPR_PUMA, FF_FULLBRIGHT|FF_TRANS60|8, 3,       {NULL},        0, 0, S_NULL},         // S_PUMATRAIL4

	// Hammer
	{SPR_HAMM, FF_ANIMATE, -1, {NULL}, 3, 3, S_NULL}, // S_HAMMER

	// Koopa
	{SPR_KOOP, 0, -1, {NULL}, 0, 0, S_NULL},   // S_KOOPA1
	{SPR_KOOP, 1, 24, {NULL}, 0, 0, S_KOOPA1}, // S_KOOPA2

	{SPR_BFLM, 0, 3,{NULL}, 0, 0, S_KOOPAFLAME2}, // S_KOOPAFLAME1
	{SPR_BFLM, 1, 3,{NULL}, 0, 0, S_KOOPAFLAME3}, // S_KOOPAFLAME2
	{SPR_BFLM, 2, 3,{NULL}, 0, 0, S_KOOPAFLAME1}, // S_KOOPAFLAME3

	// Axe
	{SPR_MAXE, 0, 3, {NULL}, 0, 0, S_AXE2}, // S_AXE1
	{SPR_MAXE, 1, 3, {NULL}, 0, 0, S_AXE3}, // S_AXE2
	{SPR_MAXE, 2, 3, {NULL}, 0, 0, S_AXE1}, // S_AXE3

	{SPR_MUS1, 0, -1, {NULL}, 0, 0, S_NULL}, // S_MARIOBUSH1
	{SPR_MUS2, 0, -1, {NULL}, 0, 0, S_NULL}, // S_MARIOBUSH2
	{SPR_TOAD, 0, -1, {NULL}, 0, 0, S_NULL}, // S_TOAD

	// Nights Drone
	{SPR_NDRN, 0, -1, {NULL}, 0, 0, S_NIGHTSDRONE_MAN2}, // S_NIGHTSDRONE_MAN1
	{SPR_NDRN, 0, -1, {NULL}, 0, 0, S_NIGHTSDRONE_MAN1}, // S_NIGHTSDRONE_MAN2

	// Sparkling point (RETURN TO THE GOAL, etc)
	{SPR_IVSP, 0, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING2},   // S_NIGHTSDRONE_SPARKLING1
	{SPR_IVSP, 2, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING3},   // S_NIGHTSDRONE_SPARKLING2
	{SPR_IVSP, 4, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING4},   // S_NIGHTSDRONE_SPARKLING3
	{SPR_IVSP, 6, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING5},   // S_NIGHTSDRONE_SPARKLING4
	{SPR_IVSP, 8, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING6},  // S_NIGHTSDRONE_SPARKLING5
	{SPR_IVSP, 10, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING7}, // S_NIGHTSDRONE_SPARKLING6
	{SPR_IVSP, 12, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING8}, // S_NIGHTSDRONE_SPARKLING7
	{SPR_IVSP, 14, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING9}, // S_NIGHTSDRONE_SPARKLING8
	{SPR_IVSP, 16, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING10}, // S_NIGHTSDRONE_SPARKLING9
	{SPR_IVSP, 18, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING11}, // S_NIGHTSDRONE_SPARKLING10
	{SPR_IVSP, 20, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING12}, // S_NIGHTSDRONE_SPARKLING11
	{SPR_IVSP, 22, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING13}, // S_NIGHTSDRONE_SPARKLING12
	{SPR_IVSP, 24, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING14}, // S_NIGHTSDRONE_SPARKLING13
	{SPR_IVSP, 26, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING15}, // S_NIGHTSDRONE_SPARKLING14
	{SPR_IVSP, 28, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING16}, // S_NIGHTSDRONE_SPARKLING15
	{SPR_IVSP, 30, 1, {A_GhostMe}, 0, 0, S_NIGHTSDRONE_SPARKLING1}, // S_NIGHTSDRONE_SPARKLING16

	// NiGHTS GOAL banner (inside the sparkles!)
	{SPR_GOAL, 0, 4, {NULL}, 0, 0, S_NIGHTSDRONE_GOAL2}, // S_NIGHTSDRONE_GOAL1
	{SPR_GOAL, 1, 4, {NULL}, 0, 0, S_NIGHTSDRONE_GOAL3}, // S_NIGHTSDRONE_GOAL2
	{SPR_GOAL, 2, 4, {NULL}, 0, 0, S_NIGHTSDRONE_GOAL4}, // S_NIGHTSDRONE_GOAL3
	{SPR_GOAL, 3, 4, {NULL}, 0, 0, S_NIGHTSDRONE_GOAL1}, // S_NIGHTSDRONE_GOAL4

	// Nights Sparkle
	{SPR_NSPK, FF_FULLBRIGHT, 140, {NULL}, 0, 0, S_NIGHTSPARKLE2},   // S_NIGHTSPARKLE1
	{SPR_NSPK, FF_FULLBRIGHT|1, 7, {NULL}, 0, 0, S_NIGHTSPARKLE3},   // S_NIGHTSPARKLE2
	{SPR_NSPK, FF_FULLBRIGHT|2, 7, {NULL}, 0, 0, S_NIGHTSPARKLE4},   // S_NIGHTSPARKLE3
	{SPR_NSPK, FF_FULLBRIGHT|3, 7, {NULL}, 0, 0, S_NULL},            // S_NIGHTSPARKLE4

	// Red Sparkle
	{SPR_NSPK, FF_FULLBRIGHT|4, 140, {NULL}, 0, 0, S_NIGHTSPARKLESUPER2}, // S_NIGHTSPARKLESUPER1
	{SPR_NSPK, FF_FULLBRIGHT|5, 7, {NULL}, 0, 0, S_NIGHTSPARKLESUPER3},   // S_NIGHTSPARKLESUPER2
	{SPR_NSPK, FF_FULLBRIGHT|6, 7, {NULL}, 0, 0, S_NIGHTSPARKLESUPER4},   // S_NIGHTSPARKLESUPER3
	{SPR_NSPK, FF_FULLBRIGHT|7, 7, {NULL}, 0, 0, S_NULL},                 // S_NIGHTSPARKLESUPER4

	// Paraloop helper -- THIS IS WHAT DETERMINES THE TIMER NOW
	{SPR_NULL, 0, 160, {NULL}, 0, 0, S_NULL}, // S_NIGHTSLOOPHELPER

	// NiGHTS bumper
	{SPR_NBMP, 0, -1, {NULL}, 0, 0, S_NULL},  // S_NIGHTSBUMPER1
	{SPR_NBMP, 1, -1, {NULL}, 0, 0, S_NULL},  // S_NIGHTSBUMPER2
	{SPR_NBMP, 2, -1, {NULL}, 0, 0, S_NULL},  // S_NIGHTSBUMPER3
	{SPR_NBMP, 3, -1, {NULL}, 0, 0, S_NULL},  // S_NIGHTSBUMPER4
	{SPR_NBMP, 4, -1, {NULL}, 0, 0, S_NULL},  // S_NIGHTSBUMPER5
	{SPR_NBMP, 5, -1, {NULL}, 0, 0, S_NULL},  // S_NIGHTSBUMPER6
	{SPR_NBMP, 6, -1, {NULL}, 0, 0, S_NULL},  // S_NIGHTSBUMPER7
	{SPR_NBMP, 7, -1, {NULL}, 0, 0, S_NULL},  // S_NIGHTSBUMPER8
	{SPR_NBMP, 8, -1, {NULL}, 0, 0, S_NULL},  // S_NIGHTSBUMPER9
	{SPR_NBMP, 9, -1, {NULL}, 0, 0, S_NULL},  // S_NIGHTSBUMPER10
	{SPR_NBMP, 10, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSBUMPER11
	{SPR_NBMP, 11, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSBUMPER12

	{SPR_HOOP, 0, -1, {NULL}, 0, 0, S_NULL}, // S_HOOP
	{SPR_HOOP, 1, -1, {NULL}, 0, 0, S_NULL}, // S_HOOP_XMASA
	{SPR_HOOP, 2, -1, {NULL}, 0, 0, S_NULL}, // S_HOOP_XMASB

	{SPR_NSCR, FF_FULLBRIGHT,    -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE10
	{SPR_NSCR, FF_FULLBRIGHT|1,  -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE20
	{SPR_NSCR, FF_FULLBRIGHT|2,  -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE30
	{SPR_NSCR, FF_FULLBRIGHT|3,  -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE40
	{SPR_NSCR, FF_FULLBRIGHT|4,  -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE50
	{SPR_NSCR, FF_FULLBRIGHT|5,  -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE60
	{SPR_NSCR, FF_FULLBRIGHT|6,  -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE70
	{SPR_NSCR, FF_FULLBRIGHT|7,  -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE80
	{SPR_NSCR, FF_FULLBRIGHT|8,  -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE90
	{SPR_NSCR, FF_FULLBRIGHT|9,  -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE100
	{SPR_NSCR, FF_FULLBRIGHT|10, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE10_2
	{SPR_NSCR, FF_FULLBRIGHT|11, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE20_2
	{SPR_NSCR, FF_FULLBRIGHT|12, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE30_2
	{SPR_NSCR, FF_FULLBRIGHT|13, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE40_2
	{SPR_NSCR, FF_FULLBRIGHT|14, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE50_2
	{SPR_NSCR, FF_FULLBRIGHT|15, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE60_2
	{SPR_NSCR, FF_FULLBRIGHT|16, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE70_2
	{SPR_NSCR, FF_FULLBRIGHT|17, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE80_2
	{SPR_NSCR, FF_FULLBRIGHT|18, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE90_2
	{SPR_NSCR, FF_FULLBRIGHT|19, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSCORE100_2

	// NiGHTS Paraloop Powerups
	{SPR_NPRU, 0, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSSUPERLOOP
	{SPR_NPRU, 1, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSDRILLREFILL
	{SPR_NPRU, 2, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSHELPER
	{SPR_NPRU, 3, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSEXTRATIME
	{SPR_NPRU, 4, -1, {NULL}, 0, 0, S_NULL}, // S_NIGHTSLINKFREEZE

	{SPR_CAPS, 0, -1, {NULL}, 0, 0, S_NULL}, // S_EGGCAPSULE

	// Orbiting Chaos Emeralds/Ideya for NiGHTS
	{SPR_CEMG, FF_FULLBRIGHT,   1, {A_OrbitNights}, ANG2*2, 0, S_ORBITEM1}, // S_ORBITEM1
	{SPR_CEMG, FF_FULLBRIGHT|1, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBITEM2}, // S_ORBITEM2
	{SPR_CEMG, FF_FULLBRIGHT|2, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBITEM3}, // S_ORBITEM3
	{SPR_CEMG, FF_FULLBRIGHT|3, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBITEM4}, // S_ORBITEM4
	{SPR_CEMG, FF_FULLBRIGHT|4, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBITEM5}, // S_ORBITEM5
	{SPR_CEMG, FF_FULLBRIGHT|5, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBITEM6}, // S_ORBITEM6
	{SPR_CEMG, FF_FULLBRIGHT|6, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBITEM7}, // S_ORBITEM7
	{SPR_CEMG, FF_FULLBRIGHT|7, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBITEM8}, // S_ORBITEM8
	{SPR_IDYA, FF_TRANS20|FF_FULLBRIGHT,   1, {A_OrbitNights}, ANG2*2, 0, S_ORBIDYA1}, // S_ORBIDYA1
	{SPR_IDYA, FF_TRANS20|FF_FULLBRIGHT|1, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBIDYA2}, // S_ORBIDYA2
	{SPR_IDYA, FF_TRANS20|FF_FULLBRIGHT|2, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBIDYA3}, // S_ORBIDYA3
	{SPR_IDYA, FF_TRANS20|FF_FULLBRIGHT|3, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBIDYA4}, // S_ORBIDYA4
	{SPR_IDYA, FF_TRANS20|FF_FULLBRIGHT|4, 1, {A_OrbitNights}, ANG2*2, 0, S_ORBIDYA5}, // S_ORBIDYA5

	// Flicky helper for NiGHTS
	{SPR_FL01, 1, 1, {A_OrbitNights}, ANG2*2, 180 | 0x10000, S_NIGHTOPIANHELPER2}, // S_NIGHTOPIANHELPER1
	{SPR_FL01, 1, 1, {A_OrbitNights}, ANG2*2, 180 | 0x10000, S_NIGHTOPIANHELPER3}, // S_NIGHTOPIANHELPER2
	{SPR_FL01, 1, 1, {A_OrbitNights}, ANG2*2, 180 | 0x10000, S_NIGHTOPIANHELPER4}, // S_NIGHTOPIANHELPER3
	{SPR_FL01, 2, 1, {A_OrbitNights}, ANG2*2, 180 | 0x10000, S_NIGHTOPIANHELPER5}, // S_NIGHTOPIANHELPER4
	{SPR_FL01, 2, 1, {A_OrbitNights}, ANG2*2, 180 | 0x10000, S_NIGHTOPIANHELPER6}, // S_NIGHTOPIANHELPER5
	{SPR_FL01, 2, 1, {A_OrbitNights}, ANG2*2, 180 | 0x10000, S_NIGHTOPIANHELPER7}, // S_NIGHTOPIANHELPER6
	{SPR_FL01, 3, 1, {A_OrbitNights}, ANG2*2, 180 | 0x10000, S_NIGHTOPIANHELPER8}, // S_NIGHTOPIANHELPER7
	{SPR_FL01, 3, 1, {A_OrbitNights}, ANG2*2, 180 | 0x10000, S_NIGHTOPIANHELPER9}, // S_NIGHTOPIANHELPER8
	{SPR_FL01, 3, 1, {A_OrbitNights}, ANG2*2, 180 | 0x10000, S_NIGHTOPIANHELPER1}, // S_NIGHTOPIANHELPER9

	// Nightopian
	{SPR_NTPN, 0, 4, {A_Look}, 0, 0, S_PIAN0}, // S_PIAN0
	{SPR_NTPN, 0, 4, {A_JetgThink}, 0, 0, S_PIAN2}, // S_PIAN1
	{SPR_NTPN, 1, 4, {NULL}, 0, 0, S_PIAN3}, // S_PIAN2
	{SPR_NTPN, 2, 4, {NULL}, 0, 0, S_PIAN4}, // S_PIAN3
	{SPR_NTPN, 3, 4, {NULL}, 0, 0, S_PIAN5}, // S_PIAN4
	{SPR_NTPN, 2, 4, {NULL}, 0, 0, S_PIAN6}, // S_PIAN5
	{SPR_NTPN, 1, 4, {NULL}, 0, 0, S_PIAN1}, // S_PIAN6
	{SPR_NTPN, 5|FF_ANIMATE, 4, {NULL}, 1, 4, S_PIAN1}, // S_PIANSING

	// Shleep
	{SPR_SHLP, 0, 15, {NULL}, 0, 0, S_SHLEEP2}, // S_SHLEEP1
	{SPR_SHLP, 1, 15, {NULL}, 0, 0, S_SHLEEP3}, // S_SHLEEP2
	{SPR_SHLP, 2, 15, {NULL}, 0, 0, S_SHLEEP4}, // S_SHLEEP3
	{SPR_SHLP, 1, 15, {NULL}, 0, 0, S_SHLEEP1}, // S_SHLEEP4
	{SPR_SHLP, 3, 1, {A_Scream},  0, 0, S_SHLEEPBOUNCE2}, // S_SHLEEPBOUNCE1
	{SPR_SHLP, 3, 1, {A_ZThrust}, 9, 0, S_SHLEEPBOUNCE3}, // S_SHLEEPBOUNCE2
	{SPR_SHLP, 3, 400, {A_SetObjectFlags}, MF_SLIDEME|MF_ENEMY|MF_BOUNCE|MF_NOCLIP|MF_NOCLIPHEIGHT, 0, S_NULL}, // S_SHLEEPBOUNCE3

	// Secret badniks and hazards, shhhh
	{SPR_PENG, 0, 2, {A_Look},  0, 0, S_PENGUINATOR_LOOK},    // S_PENGUINATOR_LOOK
	{SPR_PENG, 0, 2, {A_Chase}, 0, 0, S_PENGUINATOR_WADDLE2}, // S_PENGUINATOR_WADDLE1
	{SPR_PENG, 1, 2, {A_Chase}, 0, 0, S_PENGUINATOR_WADDLE3}, // S_PENGUINATOR_WADDLE2
	{SPR_PENG, 0, 2, {A_Chase}, 0, 0, S_PENGUINATOR_WADDLE4}, // S_PENGUINATOR_WADDLE3
	{SPR_PENG, 2, 2, {A_Chase}, 0, 0, S_PENGUINATOR_WADDLE1}, // S_PENGUINATOR_WADDLE4
	{SPR_PENG, 0,  0, {A_FaceTarget},      0,  0, S_PENGUINATOR_SLIDE2}, // S_PENGUINATOR_SLIDE1
	{SPR_PENG, 3,  5, {A_BunnyHop},        4, 10, S_PENGUINATOR_SLIDE3}, // S_PENGUINATOR_SLIDE2
	{SPR_PENG, 4, 90, {A_PlayAttackSound}, 0,  0, S_PENGUINATOR_SLIDE4}, // S_PENGUINATOR_SLIDE3
	{SPR_PENG, 3,  5, {A_Thrust},          0,  1, S_PENGUINATOR_SLIDE5}, // S_PENGUINATOR_SLIDE4
	{SPR_PENG, 0,  5, {A_FaceTarget},      0,  0, S_PENGUINATOR_LOOK},   // S_PENGUINATOR_SLIDE5

	{SPR_POPH, 0,  2, {A_Look},  (2048<<16)|1,           0, S_POPHAT_LOOK},   // S_POPHAT_LOOK
	{SPR_POPH, 1,  0, {A_MultiShotDist}, (MT_SPINDUST<<16)|4, 24, S_POPHAT_SHOOT2}, // S_POPHAT_SHOOT1
	{SPR_POPH, 1,  2, {A_LobShot}, MT_POPSHOT, (70<<16)|60, S_POPHAT_SHOOT3}, // S_POPHAT_SHOOT2
	{SPR_POPH, 2,  1, {NULL},               0,           0, S_POPHAT_SHOOT4}, // S_POPHAT_SHOOT3
	{SPR_POPH, 0, 57, {NULL},               0,           0, S_POPHAT_LOOK},   // S_POPHAT_SHOOT4
	{SPR_POPH, 3,  3, {A_SpawnObjectRelative}, 0, MT_POPSHOT_TRAIL, S_POPSHOT},   // S_POPSHOT
	{SPR_NULL, 0,  2, {NULL},                  0, 0,                S_SPINDUST1}, // S_POPSHOT_TRAIL

	{SPR_HIVE, 0,  5, {A_Look}, 1, 1, S_HIVEELEMENTAL_LOOK}, // S_HIVEELEMENTAL_LOOK
	{SPR_HIVE, 0, 14, {A_PlaySound}, sfx_s3k76, 1, S_HIVEELEMENTAL_PREPARE2}, // S_HIVEELEMENTAL_PREPARE1
	{SPR_HIVE, 0,  6, {A_PlaySound}, sfx_s3k8c, 1, S_HIVEELEMENTAL_SHOOT1}, // S_HIVEELEMENTAL_PREPARE2
	{SPR_HIVE, 1,  4, {A_WhoCaresIfYourSonIsABee}, (MT_BUMBLEBORE<<16)|4, (1<<16)|32, S_HIVEELEMENTAL_SHOOT2}, // S_HIVEELEMENTAL_SHOOT1
	{SPR_HIVE, 2,  2, {NULL}, 0, 0, S_HIVEELEMENTAL_DORMANT}, // S_HIVEELEMENTAL_SHOOT2
	{SPR_HIVE, 0,  5, {A_ParentTriesToSleep}, S_HIVEELEMENTAL_PREPARE1, 0, S_HIVEELEMENTAL_DORMANT}, // S_HIVEELEMENTAL_DORMANT
	{SPR_HIVE, 3, 35, {A_Pain}, 0, 0, S_HIVEELEMENTAL_LOOK}, // S_HIVEELEMENTAL_PAIN
	{SPR_HIVE, 3,  2, {A_BossScream}, 1, 0, S_HIVEELEMENTAL_DIE2}, // S_HIVEELEMENTAL_DIE1
	{SPR_NULL, 0,  2, {A_BossScream}, 1, 0, S_HIVEELEMENTAL_DIE3}, // S_HIVEELEMENTAL_DIE2
	{SPR_NULL, 0,  0, {A_Repeat}, 7, S_HIVEELEMENTAL_DIE1, S_XPLD_FLICKY}, // S_HIVEELEMENTAL_DIE3

	{SPR_BUMB, 1, 10, {NULL}, 0, 0, S_BUMBLEBORE_LOOK1}, // S_BUMBLEBORE_SPAWN
	{SPR_BUMB, 0,  4, {A_Look}, 1, 1, S_BUMBLEBORE_LOOK2}, // S_BUMBLEBORE_LOOK1
	{SPR_BUMB, 1,  4, {A_Look}, 1, 1, S_BUMBLEBORE_LOOK1}, // S_BUMBLEBORE_LOOK2
	{SPR_BUMB, 0,  4, {A_JetbThink}, 0, 0, S_BUMBLEBORE_FLY2}, // S_BUMBLEBORE_FLY1
	{SPR_BUMB, 1,  4, {A_JetbThink}, 0, 0, S_BUMBLEBORE_FLY1}, // S_BUMBLEBORE_FLY2
	{SPR_BUMB, 2|FF_FULLBRIGHT,  12, {A_ZThrust},  4, (1<<16)|1, S_BUMBLEBORE_FALL1},  // S_BUMBLEBORE_RAISE
	{SPR_BUMB, 2|FF_FULLBRIGHT,   0, {A_ZThrust}, -8, (1<<16)|1, S_BUMBLEBORE_FALL2},  // S_BUMBLEBORE_FALL1
	{SPR_BUMB, 2|FF_FULLBRIGHT, 300, {NULL},       0,         0, S_BUMBLEBORE_DIE},    // S_BUMBLEBORE_FALL2
	{SPR_BUMB, 4, 3, {A_MultiShotDist}, (MT_DUST<<16)|6, -40, S_BUMBLEBORE_STUCK2},    // S_BUMBLEBORE_STUCK1
	{SPR_BUMB, 5, 120, {NULL}, 0, 0, S_BUMBLEBORE_DIE}, // S_BUMBLEBORE_STUCK2
	{SPR_BUMB, 5, 0, {A_CryingToMomma}, 0, 0, S_XPLD1}, // S_BUMBLEBORE_DIE

	{SPR_BBUZ,          0, -1, {NULL}, 0, 0, S_NULL}, // S_BUGGLEIDLE
	{SPR_BBUZ, FF_ANIMATE, -1, {NULL}, 1, 2, S_NULL}, // S_BUGGLEFLY

	{SPR_FMCE, 0, 20, {NULL}, 0, 0, S_SMASHSPIKE_EASE1}, // S_SMASHSPIKE_FLOAT
	{SPR_FMCE, 0,  4, {A_ZThrust},  4, (1<<16)|1, S_SMASHSPIKE_EASE2}, // S_SMASHSPIKE_EASE1
	{SPR_FMCE, 0,  4, {A_ZThrust},  0, (1<<16)|1, S_SMASHSPIKE_FALL},  // S_SMASHSPIKE_EASE2
	{SPR_FMCE, 0,  2, {A_ZThrust}, -6,         1, S_SMASHSPIKE_FALL},  // S_SMASHSPIKE_FALL
	{SPR_FMCE, 1,  2, {A_MultiShotDist}, (MT_DUST<<16)|10, -48, S_SMASHSPIKE_STOMP2}, // S_SMASHSPIKE_STOMP1
	{SPR_FMCE, 2, 14, {NULL}, 0, 0, S_SMASHSPIKE_RISE1}, // S_SMASHSPIKE_STOMP2
	{SPR_FMCE, 1,  2, {NULL}, 0, 0, S_SMASHSPIKE_RISE2}, // S_SMASHSPIKE_RISE1
	{SPR_FMCE, 0,  2, {A_ZThrust}, 6, (1<<16)|1, S_SMASHSPIKE_RISE2}, // S_SMASHSPIKE_RISE2

	{SPR_CACO, 0,  5, {A_Look}, (1100<<16)|1, 0, S_CACO_LOOK}, // S_CACO_LOOK
	{SPR_CACO, 1,  0, {A_MultiShotDist}, (MT_DUST<<16)|7, -48, S_CACO_WAKE2}, // S_CACO_WAKE1
	{SPR_CACO, 1, 10, {A_ZThrust}, 4, (1<<16)|1, S_CACO_WAKE3}, // S_CACO_WAKE2
	{SPR_CACO, 2,  8, {A_ZThrust}, 2, (1<<16)|1, S_CACO_WAKE4}, // S_CACO_WAKE3
	{SPR_CACO, 2,  4, {A_ZThrust}, 0, (1<<16)|1, S_CACO_ROAR},  // S_CACO_WAKE4
	{SPR_CACO, 2, 10, {A_PlayActiveSound}, 0, 0, S_CACO_CHASE}, // S_CACO_ROAR
	{SPR_CACO, 2,  5, {A_JetChase}, 0, 0, S_CACO_CHASE_REPEAT}, // S_CACO_CHASE
	{SPR_CACO, 2,  0, {A_Repeat}, 5, S_CACO_CHASE, S_CACO_RANDOM}, // S_CACO_CHASE_REPEAT
	{SPR_CACO, 2,  0, {A_RandomState}, S_CACO_PREPARE_SOUND, S_CACO_CHASE, S_CACO_RANDOM}, // S_CACO_RANDOM
	{SPR_CACO, 2,  8, {A_PlaySound}, sfx_s3k95, 1, S_CACO_PREPARE1},  // S_CACO_PREPARE_SOUND
	{SPR_CACO, 3,               8, {NULL}, 0, 0, S_CACO_PREPARE2},    // S_CACO_PREPARE1
	{SPR_CACO, 4|FF_FULLBRIGHT, 8, {NULL}, 0, 0, S_CACO_PREPARE3},    // S_CACO_PREPARE2
	{SPR_CACO, 5|FF_FULLBRIGHT, 8, {NULL}, 0, 0, S_CACO_SHOOT_SOUND}, // S_CACO_PREPARE3
	{SPR_CACO, 4|FF_FULLBRIGHT, 0, {A_PlaySound}, sfx_s3k4e, 1, S_CACO_SHOOT1}, // S_CACO_SHOOT_SOUND
	{SPR_CACO, 4|FF_FULLBRIGHT, 0, {A_SpawnParticleRelative}, 0, S_CACOFIRE_EXPLODE1, S_CACO_SHOOT2}, // S_CACO_SHOOT1
	{SPR_CACO, 4|FF_FULLBRIGHT, 6, {A_FireShot}, MT_CACOFIRE, -24, S_CACO_CLOSE}, // S_CACO_SHOOT2
	{SPR_CACO, 3,              15, {NULL}, 0, 0, S_CACO_CHASE}, // S_CACO_CLOSE
	{SPR_CACO, 10, 0, {A_SetObjectFlags}, MF_NOBLOCKMAP, 0, S_CACO_DIE_GIB1}, // S_CACO_DIE_FLAGS
	{SPR_CACO, 10, 0, {A_NapalmScatter}, (7<<16)|MT_CACOSHARD, (30<<16)|20, S_CACO_DIE_GIB2}, // S_CACO_DIE_GIB1
	{SPR_CACO, 10, 0, {A_NapalmScatter}, (10<<16)|MT_CACOSHARD, (24<<16)|32, S_CACO_DIE_SCREAM}, // S_CACO_DIE_GIB2
	{SPR_CACO, 10, 0, {A_Scream}, 0, 0, S_CACO_DIE_SHATTER}, // S_CACO_DIE_SCREAM
	{SPR_CACO, 10, 0, {A_PlaySound}, sfx_pumpkn, 1, S_CACO_DIE_FALL}, // S_CACO_DIE_SHATTER
	{SPR_CACO, 10, 250, {A_FlickySpawn}, (1<<16), 0, S_NULL}, // S_CACO_DIE_FALL

	{SPR_CACO, 6, 0, {A_RandomState}, S_CACOSHARD1_1, S_CACOSHARD2_1, S_NULL}, // S_CACOSHARD_RANDOMIZE
	{SPR_CACO, 6, 3, {NULL}, 0, 0, S_CACOSHARD1_2}, // S_CACOSHARD1_1
	{SPR_CACO, 7, 3, {NULL}, 0, 0, S_CACOSHARD1_1}, // S_CACOSHARD1_2
	{SPR_CACO, 8, 3, {NULL}, 0, 0, S_CACOSHARD2_2}, // S_CACOSHARD2_1
	{SPR_CACO, 9, 3, {NULL}, 0, 0, S_CACOSHARD2_1}, // S_CACOSHARD2_2
	{SPR_BAL2,   FF_FULLBRIGHT, 2, {A_GhostMe}, 0, 0, S_CACOFIRE2}, // S_CACOFIRE1
	{SPR_BAL2, 1|FF_FULLBRIGHT, 2, {A_GhostMe}, 0, 0, S_CACOFIRE3}, // S_CACOFIRE2
	{SPR_BAL2,   FF_FULLBRIGHT, 0, {A_PlayActiveSound}, 0, 0, S_CACOFIRE1}, // S_CACOFIRE3
	{SPR_BAL2, 2|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_CACOFIRE_EXPLODE2}, // S_CACOFIRE_EXPLODE1
	{SPR_BAL2, 3|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_CACOFIRE_EXPLODE3}, // S_CACOFIRE_EXPLODE2
	{SPR_BAL2, 4|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_CACOFIRE_EXPLODE4}, // S_CACOFIRE_EXPLODE3
	{SPR_BAL2, 5|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL},      // S_CACOFIRE_EXPLODE4

	{SPR_SBOB, 1, 10, {A_ZThrust}, -2, (1<<16)|1, S_SPINBOBERT_MOVE_UP},       // S_SPINBOBERT_MOVE_FLIPUP
	{SPR_SBOB, 0, 45, {A_ZThrust},  4, (1<<16)|1, S_SPINBOBERT_MOVE_FLIPDOWN}, // S_SPINBOBERT_MOVE_UP
	{SPR_SBOB, 1, 10, {A_ZThrust},  2, (1<<16)|1, S_SPINBOBERT_MOVE_DOWN},     // S_SPINBOBERT_MOVE_FLIPDOWN
	{SPR_SBOB, 2, 45, {A_ZThrust}, -4, (1<<16)|1, S_SPINBOBERT_MOVE_FLIPUP},   // S_SPINBOBERT_MOVE_DOWN
	{SPR_SBSK, FF_FULLBRIGHT, 1, {A_RotateSpikeBall},       0,                        0, S_SPINBOBERT_FIRE_GHOST}, // S_SPINBOBERT_FIRE_MOVE
	{SPR_SBSK, FF_FULLBRIGHT, 0, {A_SpawnParticleRelative}, 0, S_SPINBOBERT_FIRE_TRAIL1, S_SPINBOBERT_FIRE_MOVE},  // S_SPINBOBERT_FIRE_GHOST
	{SPR_SBFL, 2|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_SPINBOBERT_FIRE_TRAIL2}, // S_SPINBOBERT_FIRE_TRAIL1
	{SPR_SBFL, 1|FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_SPINBOBERT_FIRE_TRAIL3}, // S_SPINBOBERT_FIRE_TRAIL2
	{SPR_SBFL,   FF_FULLBRIGHT, 2, {NULL}, 0, 0, S_NULL},                   // S_SPINBOBERT_FIRE_TRAIL3

	{SPR_HBAT, 0,  5, {A_Look}, (900<<16)|1, 0, S_HANGSTER_LOOK}, // S_HANGSTER_LOOK
	{SPR_HBAT, 1,  0, {A_MultiShotDist}, (MT_DUST<<16)|10, -34, S_HANGSTER_SWOOP2}, // S_HANGSTER_SWOOP1
	{SPR_HBAT, 1,  2, {A_ZThrust}, -8, (1<<16)|1, S_HANGSTER_SWOOP2}, // S_HANGSTER_SWOOP2
	{SPR_HBAT, 1,  6, {A_ZThrust}, -5, (1<<16), S_HANGSTER_ARC2}, // S_HANGSTER_ARC1
	{SPR_HBAT, 1,  5, {A_ZThrust}, -2, (1<<16), S_HANGSTER_ARC3}, // S_HANGSTER_ARC2
	{SPR_HBAT, 1,  1, {A_ZThrust},  0, (1<<16), S_HANGSTER_FLY1}, // S_HANGSTER_ARC3
	{SPR_HBAT, 1,  4, {A_Thrust}, 6, 1, S_HANGSTER_FLY2}, // S_HANGSTER_FLY1
	{SPR_HBAT, 2,  1, {A_PlaySound}, sfx_s3k52, 1, S_HANGSTER_FLY3}, // S_HANGSTER_FLY2
	{SPR_HBAT, 3,  4, {A_Thrust}, 6, 1, S_HANGSTER_FLY4}, // S_HANGSTER_FLY3
	{SPR_HBAT, 2,  1, {A_Thrust}, 6, 1, S_HANGSTER_FLYREPEAT}, // S_HANGSTER_FLY4
	{SPR_HBAT, 2,  0, {A_Repeat}, 12, S_HANGSTER_FLY1, S_HANGSTER_ARCUP1}, // S_HANGSTER_FLYREPEAT
	{SPR_HBAT, 1,  5, {A_ZThrust},  2, (1<<16), S_HANGSTER_ARCUP2}, // S_HANGSTER_ARCUP1
	{SPR_HBAT, 1,  6, {A_ZThrust},  5, (1<<16), S_HANGSTER_ARCUP3}, // S_HANGSTER_ARCUP2
	{SPR_HBAT, 1,  1, {A_ZThrust},  0, (1<<16), S_HANGSTER_RETURN1}, // S_HANGSTER_ARCUP3
	{SPR_HBAT, 1,  1, {A_ZThrust},  8, (1<<16), S_HANGSTER_RETURN2}, // S_HANGSTER_RETURN1
	{SPR_HBAT, 3,  1, {NULL}, 0, 0, S_HANGSTER_RETURN1}, // S_HANGSTER_RETURN2
	{SPR_HBAT, 0, 15, {NULL}, 0, 0, S_HANGSTER_LOOK}, // S_HANGSTER_RETURN3

	{SPR_NULL, 0,  35, {NULL}, 0, 0, S_CRUMBLE2}, // S_CRUMBLE1
	{SPR_NULL, 0, 105, {A_Scream}, 0, 0, S_NULL}, // S_CRUMBLE2

	// Spark
	{SPR_NULL, 0, 1, {A_ModuloToState}, 2, S_SPRK2, S_SPRK3},  // S_SPRK1
	{SPR_SPRK, FF_TRANS20|FF_ANIMATE|0, 18, {NULL}, 8, 2, S_NULL},  // S_SPRK2
	{SPR_SPRK, FF_TRANS20|FF_ANIMATE|9, 18, {NULL}, 8, 2, S_NULL},  // S_SPRK3

	// Robot Explosion
	{SPR_BOM1, 0, 0, {A_FlickySpawn}, 0, 0, S_XPLD1}, // S_XPLD_FLICKY
	{SPR_BOM1, 0, 2, {A_Scream},      0, 0, S_XPLD2}, // S_XPLD1
	{SPR_BOM1, 1, 2, {NULL},          0, 0, S_XPLD3}, // S_XPLD2
	{SPR_BOM1, 2, 3, {NULL},          0, 0, S_XPLD4}, // S_XPLD3
	{SPR_BOM1, 3, 3, {NULL},          0, 0, S_XPLD5}, // S_XPLD4
	{SPR_BOM1, 4, 4, {NULL},          0, 0, S_XPLD6}, // S_XPLD5
	{SPR_BOM1, 5, 4, {NULL},          0, 0, S_NULL},  // S_XPLD6

	{SPR_BOM1, FF_ANIMATE,   21, {NULL},          5, 4, S_INVISIBLE}, // S_XPLD_EGGTRAP

	// Underwater Explosion
	{SPR_BOM4, 0, 3, {A_Scream}, 0, 0, S_WPLD2}, // S_WPLD1
	{SPR_BOM4, 1, 3, {NULL},     0, 0, S_WPLD3}, // S_WPLD2
	{SPR_BOM4, 2, 3, {NULL},     0, 0, S_WPLD4}, // S_WPLD3
	{SPR_BOM4, 3, 3, {NULL},     0, 0, S_WPLD5}, // S_WPLD4
	{SPR_BOM4, 4, 3, {NULL},     0, 0, S_WPLD6}, // S_WPLD5
	{SPR_BOM4, 5, 3, {NULL},     0, 0, S_NULL},  // S_WPLD6

	{SPR_DUST,   FF_TRANS40, 4, {NULL}, 0, 0, S_DUST2}, // S_DUST1
	{SPR_DUST, 1|FF_TRANS50, 5, {NULL}, 0, 0, S_DUST3}, // S_DUST2
	{SPR_DUST, 2|FF_TRANS60, 3, {NULL}, 0, 0, S_DUST4}, // S_DUST3
	{SPR_DUST, 3|FF_TRANS70, 2, {NULL}, 0, 0, S_NULL},  // S_DUST4

	{SPR_NULL, 0, 1, {A_RockSpawn}, 0, 0, S_ROCKSPAWN}, // S_ROCKSPAWN

	{SPR_ROIA, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 4, 2, S_NULL}, // S_ROCKCRUMBLEA
	{SPR_ROIB, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEB
	{SPR_ROIC, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEC
	{SPR_ROID, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLED
	{SPR_ROIE, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEE
	{SPR_ROIF, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEF
	{SPR_ROIG, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 4, 2, S_NULL}, // S_ROCKCRUMBLEG
	{SPR_ROIH, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 4, 2, S_NULL}, // S_ROCKCRUMBLEH
	{SPR_ROII, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEI
	{SPR_ROIJ, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 4, 2, S_NULL}, // S_ROCKCRUMBLEJ
	{SPR_ROIK, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 4, 2, S_NULL}, // S_ROCKCRUMBLEK
	{SPR_ROIL, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEL
	{SPR_ROIM, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEM
	{SPR_ROIN, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEN
	{SPR_ROIO, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEO
	{SPR_ROIP, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 7, 2, S_NULL}, // S_ROCKCRUMBLEP

	{SPR_GFZD, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 31, 1, S_NULL}, // S_GFZDEBRIS
	{SPR_BRIC, FF_ANIMATE, -1, {A_DebrisRandom}, 7, 2, S_NULL}, // S_BRICKDEBRIS
	{SPR_WDDB, FF_ANIMATE, -1, {A_DebrisRandom}, 7, 2, S_NULL}, // S_WOODDEBRIS
	{SPR_BRIR, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 31, 1, S_NULL}, // S_REDBRICKDEBRIS
	{SPR_BRIB, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 31, 1, S_NULL}, // S_BLUEBRICKDEBRIS
	{SPR_BRIY, FF_ANIMATE|FF_RANDOMANIM, -1, {NULL}, 31, 1, S_NULL}, // S_YELLOWBRICKDEBRIS

#ifdef SEENAMES
	{SPR_NULL, 0, 1, {NULL}, 0, 0, S_NULL}, // S_NAMECHECK
#endif
};

mobjinfo_t mobjinfo[NUMMOBJTYPES] =
{
	{           // MT_NULL
		-1,             // doomednum
		S_NULL,         // spawnstate
		0,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		0,              // radius
		0,              // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_UNKNOWN
		-1,             // doomednum
		S_UNKNOWN,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THOK
		-1,             // doomednum
		S_THOK,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PLAYER
		-1,             // doomednum
		S_PLAY_STND,    // spawnstate
		1,              // spawnhealth
		S_PLAY_WALK,    // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_thok,       // attacksound
		S_PLAY_PAIN,    // painstate
		MT_THOK,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_PLAY_ROLL,    // missilestate
		S_PLAY_DEAD,    // deathstate
		S_PLAY_DRWN,    // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		16*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		MT_THOK,        // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE, // flags
		(statenum_t)MT_NULL// raisestate
	},

	{           // MT_TAILSOVERLAY
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		2,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_METALJETFUME
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_JETFUMEFLASH, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		2,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_JETFUME1      // raisestate
	},

	{           // MT_BLUECRAWLA
		100,            // doomednum
		S_POSS_STND,    // spawnstate
		1,              // spawnhealth
		S_POSS_RUN1,    // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		3,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDCRAWLA
		101,            // doomednum
		S_SPOS_STND,    // spawnstate
		1,              // spawnhealth
		S_SPOS_RUN1,    // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		170,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		3,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFISH
		102,            // doomednum
		S_FISH2,        // spawnstate
		1,              // spawnhealth
		S_FISH1,        // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_FISH3,        // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_FISH4,        // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		28*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_GOLDBUZZ
		103,            // doomednum
		S_BUZZLOOK1,    // spawnstate
		1,              // spawnhealth
		S_BUZZFLY1,     // seestate
		sfx_None,       // seesound
		2,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3072,           // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		4*FRACUNIT,     // speed
		28*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDBUZZ
		104,            // doomednum
		S_RBUZZLOOK1,   // spawnstate
		1,              // spawnhealth
		S_RBUZZFLY1,    // seestate
		sfx_None,       // seesound
		2,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3072,           // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		8*FRACUNIT,     // speed
		28*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_JETTBOMBER
		105,            // doomednum
		S_JETBLOOK1,    // spawnstate
		1,              // spawnhealth
		S_JETBZOOM1,    // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_s3k51,      // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1*FRACUNIT,     // speed
		20*FRACUNIT,    // radius
		50*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		(statenum_t)MT_MINE// raisestate
	},

	{           // MT_JETTGUNNER
		106,            // doomednum
		S_JETGLOOK1,    // spawnstate
		1,              // spawnhealth
		S_JETGZOOM1,    // seestate
		sfx_None,       // seesound
		5,              // reactiontime
		sfx_s3k4d,      // attacksound
		S_NULL,         // painstate
		3072,           // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_JETGSHOOT1,   // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1*FRACUNIT,     // speed
		20*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		(statenum_t)MT_JETTBULLET// raisestate
	},

	{           // MT_CRAWLACOMMANDER
		107,            // doomednum
		S_CCOMMAND1,    // spawnstate
		2,              // spawnhealth
		S_CCOMMAND3,    // seestate
		sfx_None,       // seesound
		2*TICRATE,      // reactiontime
		sfx_s3k60,      // attacksound
		S_CCOMMAND3,    // painstate
		200,            // painchance
		sfx_dmpain,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		3,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k5d,      // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_DETON
		108,            // doomednum
		S_DETON1,       // spawnstate
		1,              // spawnhealth
		S_DETON2,       // seestate
		sfx_s3k86,      // seesound -- sfx_kc57 for a self-propelled deton...
		1,              // reactiontime
		sfx_deton,      // attacksound
		S_NULL,         // painstate
		3072,           // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1*FRACUNIT,     // speed
		20*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SHOOTABLE|MF_NOGRAVITY|MF_MISSILE, // flags
		(statenum_t)ANG15// raisestate: largest angle to turn in one tic (here, 15 degrees)
	},

	{           // MT_SKIM
		109,            // doomednum
		S_SKIM1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_s3k51,      // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_SKIM3,        // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_NOGRAVITY|MF_SHOOTABLE, // flags
		(statenum_t)MT_MINE// raisestate
	},

	{           // MT_TURRET
		110,            // doomednum
		S_TURRET,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_trfire,     // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_fizzle,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_TURRETSHOCK1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		MT_TURRETLASER, // mass
		1,              // damage
		sfx_trpowr,     // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_POPUPTURRET
		111,            // doomednum
		S_TURRETLOOK,   // spawnstate
		1,              // spawnhealth
		S_TURRETSEE,    // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_trfire,     // attacksound
		S_NULL,         // painstate
		1024,           // painchance
		sfx_s3k64,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		(statenum_t)MT_JETTBULLET// raisestate
	},

	{           // MT_SPINCUSHION
		112,            // doomednum
		S_SPINCUSHION_LOOK, // spawnstate
		1,              // spawnhealth
		S_SPINCUSHION_CHASE1, // seestate
		sfx_None,       // seesound
		3*TICRATE,      // reactiontime
		sfx_s3kd8s,     // attacksound
		S_NULL,         // painstate
		5*TICRATE,      // painchance
		sfx_shrpsp,     // painsound
		S_SPINCUSHION_STOP1, // meleestate
		S_SPINCUSHION_AIM1, // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_SPINCUSHION_STOP3, // xdeathstate
		sfx_pop,        // deathsound
		2,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_s3kaa,      // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_BOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_CRUSHSTACEAN
		126,            // doomednum
		S_CRUSHSTACEAN_ROAM1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_s3k6b,      // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_CRUSHSTACEAN_PUNCH1, // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		8,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_CRUSHCLAW
		-1,             // doomednum
		S_CRUSHCLAW_AIM, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		16,             // reactiontime
		sfx_s3k6b,      // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_CRUSHCLAW_OUT,// missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		600,            // speed
		22*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		(sfx_s3k49<<8), // mass
		0,              // damage
		sfx_s3kd2l,     // activesound
		MF_PAIN|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		(statenum_t)MT_CRUSHCHAIN// raisestate
	},

	{           // MT_CRUSHCHAIN
		-1,             // doomednum
		S_CRUSHCHAIN,   // spawnstate
		0,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		0,              // radius
		0,              // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BANPYURA
		138,            // doomednum
		S_BANPYURA_ROAM1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		8,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_BANPSPRING
		-1,             // doomednum
		S_CDIAG1,       // spawnstate
		1,              // spawnhealth
		S_CDIAG2,       // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_cdfm08,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		300,            // speed
		22*FRACUNIT,    // radius
		22*FRACUNIT,    // height
		0,              // display offset
		11*FRACUNIT,    // mass
		11*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SPRING|MF_NOGRAVITY, // flags
		S_CDIAG2        // raisestate
	},

	{           // MT_JETJAW
		113,            // doomednum
		S_JETJAW_ROAM1, // spawnstate
		1,              // spawnhealth
		S_JETJAW_CHOMP1,// seestate
		sfx_None,       // seesound
		4*TICRATE,      // reactiontime
		sfx_s1ab,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		8,              // speed
		12*FRACUNIT,    // radius
		20*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_FLOAT|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNAILER
		114,            // doomednum
		S_SNAILER1,     // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		2,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SNAILER_FLICKY, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		FRACUNIT,       // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_VULTURE
		115,            // doomednum
		S_VULTURE_STND, // spawnstate
		1,              // spawnhealth
		S_VULTURE_DRIFT,// seestate
		sfx_None,       // seesound
		TICRATE/2,      // reactiontime
		sfx_s3k60,      // attacksound
		S_VULTURE_STUNNED, // painstate
		S_NULL,         // painchance
		sfx_s3k96,      // painsound
		S_NULL,         // meleestate
		S_VULTURE_ZOOM1,// missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		5,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		TICRATE,        // mass
		0,              // damage
		sfx_jet,        // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY|MF_SLIDEME, // flags
		S_NULL          // raisestate
	},

	{           // MT_POINTY
		116,            // doomednum
		S_POINTY1,      // spawnstate
		1,              // spawnhealth
		S_POINTY1,      // seestate
		sfx_None,       // seesound
		6,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		4,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		5*FRACUNIT,     // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		MT_POINTYBALL,  // mass
		128,            // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_POINTYBALL
		-1,             // doomednum
		S_POINTYBALL1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		DMG_SPIKE,      // mass
		1,              // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROBOHOOD
		117,              // doomednum
		S_ROBOHOOD_LOOK,  // spawnstate
		1,                // spawnhealth
		S_ROBOHOOD_STAND, // seestate
		sfx_None,         // seesound
		TICRATE,          // reactiontime
		sfx_ngjump,       // attacksound
		S_NULL,           // painstate
		0,                // painchance
		sfx_None,         // painsound
		S_ROBOHOOD_JUMP3, // meleestate
		S_ROBOHOOD_FIRE1, // missilestate
		S_XPLD_FLICKY,    // deathstate
		S_NULL,           // xdeathstate
		sfx_pop,          // deathsound
		3,                // speed
		24*FRACUNIT,      // radius
		32*FRACUNIT,      // height
		0,                // display offset
		100,              // mass
		0,                // damage
		sfx_s3k4a,        // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_ROBOHOOD_JUMP1  // raisestate
	},

	{           // MT_FACESTABBER
		118,            // doomednum
		S_FACESTABBER_STND1, // spawnstate
		2,              // spawnhealth
		S_FACESTABBER_STND1, // seestate
		sfx_None,       // seesound
		70,             // reactiontime
		sfx_zoom,       // attacksound
		S_FACESTABBER_PAIN, // painstate
		0,              // painchance
		sfx_dmpain,     // painsound
		S_FACESTABBER_CHARGE1, // meleestate
		S_FACESTABBER_CHARGE1, // missilestate
		S_FACESTABBER_DIE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3kb4,      // deathsound
		3,              // speed
		32*FRACUNIT,    // radius
		72*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3kc5s,      // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_SLIDEME, // flags
		S_NULL          // raisestate
	},

	{           // MT_FACESTABBERSPEAR
		-1,              // doomednum
		S_FACESTABBERSPEAR, // spawnstate
		1,               // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		35,              // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		32*FRACUNIT,     // radius
		72*FRACUNIT,     // height
		0,               // display offset
		DMG_SPIKE,       // mass
		0,               // damage
		sfx_None,        // activesound
		MF_PAIN|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL           // raisestate
	},

	{           // MT_EGGGUARD
		119,             // doomednum
		S_EGGGUARD_STND, // spawnstate
		1,               // spawnhealth
		S_EGGGUARD_WALK1,// seestate
		sfx_None,        // seesound
		35,              // reactiontime
		sfx_None,        // attacksound
		S_EGGGUARD_MAD1, // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_EGGGUARD_RUN1, // meleestate
		S_NULL,          // missilestate
		S_XPLD_FLICKY,   // deathstate
		S_NULL,          // xdeathstate
		sfx_pop,         // deathsound
		6,               // speed
		16*FRACUNIT,     // radius
		48*FRACUNIT,     // height
		0,               // display offset
		100,             // mass
		0,               // damage
		sfx_None,        // activesound
		MF_ENEMY,        // flags
		S_NULL           // raisestate
	},

	{           // MT_EGGSHIELD
		-1,              // doomednum
		S_EGGSHIELD,     // spawnstate
		1,               // spawnhealth
		S_EGGSHIELD,     // seestate
		sfx_None,        // seesound
		35,              // reactiontime
		sfx_s3k7b,       // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_s3k7b,       // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_EGGSHIELDBREAK,// deathstate
		S_NULL,          // xdeathstate
		sfx_wbreak,      // deathsound
		3,               // speed
		24*FRACUNIT,     // radius
		128*FRACUNIT,    // height
		0,               // display offset
		100,             // mass
		0,               // damage
		sfx_None,        // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL           // raisestate
	},

	{           // MT_GSNAPPER
		120,            // doomednum
		S_SNAPPER_SPAWN,// spawnstate
		1,              // spawnhealth
		S_GSNAPPER1,    // seestate
		sfx_None,       // seesound
		10,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SNAPPER_XPLD, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		4,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k7e,      // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNAPPER_LEG
		-1,             // doomednum
		S_SNAPPER_LEG,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		6*FRACUNIT,     // radius
		12*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_SNAPPER_LEGRAISE // raisestate
	},

	{           // MT_SNAPPER_HEAD
		-1,             // doomednum
		S_SNAPPER_HEAD, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		6*FRACUNIT,     // radius
		12*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MINUS
		121,            // doomednum
		S_MINUS_INIT,   // spawnstate
		1,              // spawnhealth
		S_MINUS_DIGGING1,// seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_s3kccs,     // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_MINUS_BURST0, // meleestate
		S_MINUS_POPUP,  // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		17,             // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3kd3s,     // activesound
		MF_ENEMY|MF_NOCLIPTHING, // flags
		S_MINUS_BURST1  // raisestate
	},

	{           // MT_MINUSDIRT
		-1,             // doomednum
		S_MINUSDIRT1,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPRINGSHELL
		122,            // doomednum
		S_SSHELL_STND,  // spawnstate
		1,              // spawnhealth
		S_SSHELL_RUN1,  // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		6,              // speed
		24*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		15*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_SSHELL_SPRING1// raisestate
	},

	{           // MT_YELLOWSHELL
		125,            // doomednum
		S_YSHELL_STND,  // spawnstate
		1,              // spawnhealth
		S_YSHELL_RUN1,  // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		6,              // speed
		24*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		20*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_YSHELL_SPRING1// raisestate
	},

	{           // MT_UNIDUS
		123,            // doomednum
		S_UNIDUS_STND,  // spawnstate
		1,              // spawnhealth
		S_UNIDUS_RUN,   // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_UNIBALL,     // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		2,              // speed
		18*FRACUNIT,    // radius
		36*FRACUNIT,    // height
		0,              // display offset
		4*FRACUNIT,     // mass
		5,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_UNIBALL
		-1,             // doomednum
		S_UNIDUS_BALL,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		1,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		20*FRACUNIT,    // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		13*FRACUNIT,    // radius
		26*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		8*FRACUNIT,     // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANARIVORE
		134,            // doomednum
		S_CANARIVORE_LOOK,  // spawnstate
		1,              // spawnhealth
		S_CANARIVORE_AWAKEN1,   // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY|MF_SPAWNCEILING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANARIVORE_GAS
		-1,             // doomednum
		S_CANARIVOREGAS_1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k5d,      // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_PYREFLY
		136,            // doomednum
		S_PYREFLY_FLY,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_PYREFLY_BURN, // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		24*FRACUNIT,    // radius
		34*FRACUNIT,    // height
		0,              // display offset
		DMG_FIRE,       // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY|MF_SLIDEME, // flags
		S_NULL          // raisestate
	},

	{           // MT_PYREFLY_FIRE
		-1,             // doomednum
		S_PYREFIRE1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		34*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_FIRE|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_PTERABYTESPAWNER
		135,            // doomednum
		S_PTERABYTESPAWNER, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPTHING|MF_NOBLOCKMAP|MF_RUNSPAWNFUNC|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PTERABYTEWAYPOINT
		-1,             // doomednum
		S_PTERABYTEWAYPOINT, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIPTHING|MF_NOBLOCKMAP|MF_RUNSPAWNFUNC|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PTERABYTE
		-1,             // doomednum
		S_PTERABYTE_FLY1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_pscree,     // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		4*FRACUNIT,     // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY|MF_NOGRAVITY|MF_SLIDEME, // flags
		S_NULL          // raisestate
	},

	{           // MT_DRAGONBOMBER
		137,            // doomednum
		S_DRAGONBOMBER, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		6,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		10*FRACUNIT,    // speed
		28*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY|MF_NOGRAVITY|MF_SLIDEME|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_DRAGONWING
		-1,             // doomednum
		S_DRAGONWING1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		12*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_DRAGONTAIL
		-1,             // doomednum
		S_DRAGONTAIL_LOADED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_DRAGONMINE,  // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_tink,       // activesound
		MF_NOGRAVITY|MF_SLIDEME|MF_PAIN, // flags
		S_DRAGONTAIL_EMPTY // raisestate
	},

	{           // MT_DRAGONMINE
		-1,             // doomednum
		S_DRAGONMINE,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_s3k76,      // seesound
		0,              // reactiontime
		sfx_s3k89,      // attacksound
		S_NULL,         // painstate
		6,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_TNTBARREL_EXPL1, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k6e,      // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k5d,      // activesound
		MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSSEXPLODE
		-1,             // doomednum
		S_BOSSEXPLODE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SONIC3KBOSSEXPLODE
		-1,                      // doomednum
		S_SONIC3KBOSSEXPLOSION1, // spawnstate
		1000,                    // spawnhealth
		S_NULL,                  // seestate
		sfx_None,                // seesound
		8,                       // reactiontime
		sfx_None,                // attacksound
		S_NULL,                  // painstate
		0,                       // painchance
		sfx_None,                // painsound
		S_NULL,                  // meleestate
		S_NULL,                  // missilestate
		S_NULL,                  // deathstate
		S_NULL,                  // xdeathstate
		sfx_None,                // deathsound
		1,                       // speed
		8*FRACUNIT,              // radius
		16*FRACUNIT,             // height
		0,                       // display offset
		4,                       // mass
		0,                       // damage
		sfx_None,                // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL                   // raisestate
	},

	{           // MT_BOSSFLYPOINT
		290,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGTRAP
		291,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_pop,        // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_INVISIBLE,    // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSS3WAYPOINT
		292,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSS9GATHERPOINT
		293,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSSJUNK
		-1,             // doomednum
		S_BOSSEGLZ1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		64*FRACUNIT,    // height
		2,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMOBILE
		200,               // doomednum
		S_EGGMOBILE_STND,  // spawnstate
		8,                 // spawnhealth
		S_EGGMOBILE_STND,  // seestate
		sfx_None,          // seesound
		45,                // reactiontime
		sfx_None,          // attacksound
		S_EGGMOBILE_PAIN,  // painstate
		MT_THOK,           // painchance
		sfx_dmpain,        // painsound
		S_EGGMOBILE_LATK1, // meleestate
		S_EGGMOBILE_RATK1, // missilestate
		S_EGGMOBILE_DIE1,  // deathstate
		S_EGGMOBILE_FLEE1, // xdeathstate
		sfx_s3kb4,         // deathsound
		4,                 // speed
		24*FRACUNIT,       // radius
		76*FRACUNIT,       // height
		0,                 // display offset
		sfx_None,          // mass
		3,                 // damage
		sfx_telept,        // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_FLOAT|MF_NOGRAVITY|MF_BOSS, // flags
		S_EGGMOBILE_PANIC1 // raisestate
	},

	{           // MT_JETFUME1
		-1,             // doomednum
		S_JETFUME1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMOBILE_BALL
		-1,             // doomednum
		S_EGGMOBILE_BALL,// spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_cannon,     // seesound
		1,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		40*FRACUNIT,    // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		13*FRACUNIT,    // radius
		26*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		8*FRACUNIT,     // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMOBILE_TARGET
		-1,             // doomednum
		S_EGGMOBILE_TARGET, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		0,              // radius
		0,              // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMOBILE_FIRE
		-1,             // doomednum
		S_LASERFLAME1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_s3kc2s,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_s3k8d,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		84*FRACUNIT,    // height
		0,              // display offset
		DMG_FIRE,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_FIRE|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMOBILE2
		201,               // doomednum
		S_EGGMOBILE2_STND, // spawnstate
		8,                 // spawnhealth
		S_NULL,            // seestate
		0,                 // seesound
		-666,              // reactiontime
		sfx_gspray,        // attacksound
		S_EGGMOBILE2_PAIN, // painstate
		MT_GOOP,           // painchance
		sfx_dmpain,        // painsound
		S_EGGMOBILE2_PAIN2, // meleestate
		(statenum_t)MT_EGGMOBILE2_POGO, // missilestate
		S_EGGMOBILE2_DIE1, // deathstate
		S_EGGMOBILE2_FLEE1,// xdeathstate
		sfx_s3kb4,         // deathsound
		2*FRACUNIT,        // speed
		24*FRACUNIT,       // radius
		76*FRACUNIT,       // height
		0,                 // display offset
		0,                 // mass
		3,                 // damage
		sfx_pogo,          // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY|MF_BOSS, // flags
		S_EGGMOBILE2_POGO1 // raisestate
	},

	{           // MT_EGGMOBILE2_POGO
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		15*FRACUNIT,    // radius
		28*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_spring,     // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT, // flags
		S_EGGMOBILE2_POGO5 // raisestate
	},

	{           // MT_GOOP
		-1,             // doomednum
		S_GOOP1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_ghit,       // painsound
		S_GOOP3,        // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		DMG_WATER,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_PAIN,        // flags
		S_NULL          // raisestate
	},

	{           // MT_GOOPTRAIL
		-1,             // doomednum
		S_GOOPTRAIL,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		3,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMOBILE3
		202,                // doomednum
		S_EGGMOBILE3_STND,  // spawnstate
		8,                  // spawnhealth
		S_NULL,             // seestate
		sfx_None,           // seesound
		0,                  // reactiontime
		sfx_None,           // attacksound
		S_EGGMOBILE3_PAIN,  // painstate
		MT_NULL,            // painchance
		sfx_dmpain,         // painsound
		S_NULL,             // meleestate
		S_EGGMOBILE3_SHOCK, // missilestate
		S_EGGMOBILE3_DIE1,  // deathstate
		S_EGGMOBILE3_FLEE1, // xdeathstate
		sfx_s3kb4,          // deathsound
		8*FRACUNIT,         // speed
		32*FRACUNIT,        // radius
		116*FRACUNIT,       // height
		0,                  // display offset
		MT_FAKEMOBILE,      // mass
		3,                  // damage
		sfx_telept,         // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY|MF_BOSS|MF_NOCLIPHEIGHT, // flags
		S_EGGMOBILE3_ROFL   // raisestate
	},

	{           // MT_FAKEMOBILE
		-1,                 // doomednum
		S_FAKEMOBILE_INIT,  // spawnstate
		1000,               // spawnhealth
		S_NULL,             // seestate
		sfx_None,           // seesound
		0,                  // reactiontime
		sfx_None,           // attacksound
		S_NULL,             // painstate
		MT_NULL,            // painchance
		sfx_s3k7b,          // painsound
		S_NULL,             // meleestate
		S_FAKEMOBILE_ATK1,  // missilestate
		S_FAKEMOBILE_DIE1,  // deathstate
		S_NULL,             // xdeathstate
		sfx_mswarp,         // deathsound
		8*FRACUNIT,         // speed
		32*FRACUNIT,        // radius
		116*FRACUNIT,       // height
		0,                  // display offset
		0,                  // mass
		3,                  // damage
		sfx_None,           // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_RUNSPAWNFUNC|MF_NOCLIPHEIGHT, // flags
		S_NULL              // raisestate
	},

	{           // MT_SHOCKWAVE
		-1,             // doomednum
		S_SHOCKWAVE1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3k5e,      // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		8*TICRATE,      // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		12*FRACUNIT,    // speed
		48*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		0,              // display offset
		DMG_ELECTRIC|(sfx_buzz2<<8), // mass
		3,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_PAIN|MF_NOGRAVITY|MF_PAPERCOLLISION, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMOBILE4
		203,               // doomednum
		S_EGGMOBILE4_STND, // spawnstate
		8,                 // spawnhealth
		S_NULL,            // seestate
		sfx_None,          // seesound
		0,                 // reactiontime
		sfx_None,          // attacksound
		S_EGGMOBILE4_PAIN1,// painstate
		0,                 // painchance
		sfx_dmpain,        // painsound
		S_EGGMOBILE4_LATK1,// meleestate
		S_EGGMOBILE4_RATK1,// missilestate
		S_EGGMOBILE4_DIE1, // deathstate
		S_EGGMOBILE4_FLEE1,// xdeathstate
		sfx_s3kb4,         // deathsound
		0,                 // speed
		24*FRACUNIT,       // radius
		76*FRACUNIT,       // height
		0,                 // display offset
		0,                 // mass
		3,                 // damage
		sfx_None,          // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY|MF_BOSS|MF_NOCLIPHEIGHT, // flags
		S_EGGMOBILE4_RAISE1// raisestate
	},

	{           // MT_EGGMOBILE4_MACE
		-1,             // doomednum
		S_EGGMOBILE4_MACE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_EGGMOBILE4_MACE_DIE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		48*FRACUNIT,    // speed
		34*FRACUNIT,    // radius
		68*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		1,              // damage
		sfx_mswing,     // activesound
		MF_PAIN|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_JETFLAME
		-1,             // doomednum
		S_JETFLAME,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		20*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		DMG_FIRE,       // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGROBO1
		1127,           // doomednum
		S_EGGROBO1_STND,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3ka0,      // seesound
		8,              // reactiontime
		sfx_bsnipe,     // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_EGGROBO1_BSLAP1, // meleestate
		S_NULL,         // missilestate
		S_EGGROBO1_PISSED, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3ka0,      // deathsound
		12*FRACUNIT,    // speed
		20*FRACUNIT,    // radius
		72*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGROBOJET
		-1,             // doomednum
		S_EGGROBOJET,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		10*FRACUNIT,    // radius
		28*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FANG
		204,               // doomednum
		S_FANG_SETUP,      // spawnstate
		8,                 // spawnhealth
		S_FANG_PATHINGSTART1, // seestate
		sfx_None,          // seesound
		0,                 // reactiontime
		sfx_skid,          // attacksound
		S_FANG_PAIN1,      // painstate
		0,                 // painchance
		sfx_s3k5d,         // painsound
		S_NULL,            // meleestate
		S_NULL,            // missilestate
		S_FANG_DIE1,       // deathstate
		S_FANG_KO,         // xdeathstate
		sfx_s3k90,         // deathsound
		0,                 // speed
		24*FRACUNIT,       // radius
		60*FRACUNIT,       // height
		0,                 // display offset
		0,                 // mass
		3,                 // damage
		sfx_boingf,        // activesound
		MF_RUNSPAWNFUNC|MF_SPECIAL|MF_BOSS|MF_SHOOTABLE|MF_GRENADEBOUNCE|MF_NOCLIPTHING, // flags -- MF_NOCLIPTHING will be removed after intro event ends
		S_NULL             // raisestate
	},

	{           // MT_BROKENROBOT
		-1,             // doomednum
		S_BROKENROBOTRANDOM, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_RUNSPAWNFUNC|MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_VWREF
		-1,             // doomednum
		S_VWREF,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		42*FRACUNIT,    // radius
		12*FRACUNIT,    // height
		1,              // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_VWREB
		-1,             // doomednum
		S_VWREB,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		42*FRACUNIT,    // radius
		12*FRACUNIT,    // height
		-1,             // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PROJECTORLIGHT
		-1,             // doomednum
		S_PROJECTORLIGHT1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		42*FRACUNIT,    // radius
		52*FRACUNIT,    // height
		-1,             // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FBOMB
		-1,                // doomednum
		S_FBOMB1,          // spawnstate
		1,                 // spawnhealth
		S_NULL,            // seestate
		sfx_s3k51,         // seesound
		0,                 // reactiontime
		sfx_None,          // attacksound
		S_NULL,            // painstate
		0,                 // painchance
		sfx_None,          // painsound
		S_NULL,            // meleestate
		S_NULL,            // missilestate
		S_FBOMB_EXPL1,     // deathstate
		S_NULL,            // xdeathstate
		sfx_s3k4e,         // deathsound
		20*FRACUNIT,       // speed
		24*FRACUNIT,       // radius
		48*FRACUNIT,       // height
		0,                 // display offset
		0,                 // mass
		0,                 // damage
		sfx_None,          // activesound
		MF_NOBLOCKMAP|MF_MISSILE, // flags
		S_NULL             // raisestate
	},

	{           // MT_TNTDUST
		-1,                // doomednum
		S_TNTDUST_1,       // spawnstate
		1,                 // spawnhealth
		S_NULL,            // seestate
		sfx_None,          // seesound
		0,                 // reactiontime
		sfx_None,          // attacksound
		S_NULL,            // painstate
		0,                 // painchance
		sfx_None,          // painsound
		S_NULL,            // meleestate
		S_NULL,            // missilestate
		S_NULL,            // deathstate
		S_NULL,            // xdeathstate
		sfx_None,          // deathsound
		20*FRACUNIT,       // speed
		16*FRACUNIT,       // radius
		32*FRACUNIT,       // height
		0,                 // display offset
		100,               // mass
		0,                 // damage
		sfx_None,          // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL             // raisestate
	},
	{           // MT_FSGNA
		-1,                // doomednum
		S_FSGNA,           // spawnstate
		1,                 // spawnhealth
		S_NULL,            // seestate
		sfx_None,          // seesound
		0,                 // reactiontime
		sfx_None,          // attacksound
		S_NULL,            // painstate
		0,                 // painchance
		sfx_None,          // painsound
		S_NULL,            // meleestate
		S_NULL,            // missilestate
		S_NULL,            // deathstate
		S_NULL,            // xdeathstate
		sfx_mspogo,        // deathsound
		0,                 // speed
		124*FRACUNIT,      // radius
		124*FRACUNIT,      // height
		0,                 // display offset
		0,                 // mass
		0,                 // damage
		sfx_None,          // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL             // raisestate
	},

	{           // MT_FSGNB
		-1,                // doomednum
		S_FSGNB,           // spawnstate
		1,                 // spawnhealth
		S_NULL,            // seestate
		sfx_None,          // seesound
		0,                 // reactiontime
		sfx_None,          // attacksound
		S_NULL,            // painstate
		0,                 // painchance
		sfx_None,          // painsound
		S_NULL,            // meleestate
		S_NULL,            // missilestate
		S_FSGNC,           // deathstate
		S_FSGND,           // xdeathstate
		sfx_None,          // deathsound
		0,                 // speed
		74*FRACUNIT,       // radius
		320*FRACUNIT,      // height
		0,                 // display offset
		0,                 // mass
		0,                 // damage
		sfx_None,          // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY|MF_SOLID, // flags
		S_NULL             // raisestate
	},

	{           // MT_FANGWAYPOINT
		294,               // doomednum
		S_INVISIBLE,       // spawnstate
		1000,              // spawnhealth
		S_NULL,            // seestate
		sfx_None,          // seesound
		0,                 // reactiontime
		sfx_None,          // attacksound
		S_NULL,            // painstate
		0,                 // painchance
		sfx_None,          // painsound
		S_NULL,            // meleestate
		S_NULL,            // missilestate
		S_NULL,            // deathstate
		S_NULL,            // xdeathstate
		sfx_None,          // deathsound
		0,                 // speed
		FRACUNIT,          // radius
		FRACUNIT,          // height
		0,                 // display offset
		0,                 // mass
		0,                 // damage
		sfx_None,          // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_NULL             // raisestate
	},

	{           // MT_BLACKEGGMAN
		206,               // doomednum
		S_BLACKEGG_STND,   // spawnstate
		8,                 // spawnhealth
		S_BLACKEGG_WALK1,  // seestate
		sfx_None,          // seesound
		8*TICRATE,         // reactiontime
		sfx_None,          // attacksound
		S_BLACKEGG_PAIN1,  // painstate
		0,                 // painchance
		sfx_None,          // painsound
		S_BLACKEGG_HITFACE1, // meleestate
		S_BLACKEGG_MISSILE1, // missilestate
		S_BLACKEGG_DIE1,   // deathstate
		S_BLACKEGG_GOOP,   // xdeathstate
		sfx_None,          // deathsound
		1,                 // speed
		48*FRACUNIT,       // radius
		160*FRACUNIT,      // height
		0,                 // display offset
		0,                 // mass
		3,                 // damage
		sfx_None,          // activesound
		MF_SPECIAL|MF_BOSS,// flags
		S_BLACKEGG_JUMP1   // raisestate
	},

	{           // MT_BLACKEGGMAN_HELPER
		-1,                // doomednum
		S_BLACKEGG_HELPER, // spawnstate
		8,                 // spawnhealth
		S_NULL,            // seestate
		sfx_None,          // seesound
		0,                 // reactiontime
		sfx_None,          // attacksound
		S_NULL,            // painstate
		0,                 // painchance
		sfx_None,          // painsound
		S_NULL,            // meleestate
		S_NULL,            // missilestate
		S_NULL,            // deathstate
		S_NULL,            // xdeathstate
		sfx_None,          // deathsound
		1,                 // speed
		48*FRACUNIT,       // radius
		32*FRACUNIT,       // height
		0,                 // display offset
		0,                 // mass
		1,                 // damage
		sfx_None,          // activesound
		MF_SOLID|MF_NOGRAVITY,          // flags
		S_NULL             // raisestate
	},

	{           // MT_BLACKEGGMAN_GOOPFIRE
		-1,             // doomednum
		S_BLACKEGG_GOOP1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BLACKEGG_GOOP3, // deathstate
		S_NULL,         // xdeathstate
		sfx_ghit,       // deathsound
		30*FRACUNIT,    // speed
		11*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		100,            // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLACKEGGMAN_MISSILE
		-1,             // doomednum
		S_BLACKEGG_MISSILE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOSSEXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_bexpld,     // deathsound
		10*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CYBRAKDEMON
		209,                // doomednum
		S_CYBRAKDEMON_IDLE, // spawnstate
		12,                 // spawnhealth
		S_CYBRAKDEMON_WALK1,// seestate
		sfx_None,           // seesound
		15,                 // reactiontime
		sfx_None,           // attacksound
		S_CYBRAKDEMON_PAIN1,// painstate
		0,                  // painchance
		sfx_behurt,         // painsound
		S_CYBRAKDEMON_CHOOSE_ATTACK2, // meleestate
		S_CYBRAKDEMON_CHOOSE_ATTACK1, // missilestate
		S_CYBRAKDEMON_DIE1, // deathstate
		S_NULL,             // xdeathstate
		sfx_s3kb4,          // deathsound
		40,                 // speed
		48*FRACUNIT,        // radius
		160*FRACUNIT,       // height
		0,                  // display offset
		100,                // mass
		1,                  // damage
		sfx_bewar1,         // activesound
		MF_SPECIAL|MF_BOSS|MF_SHOOTABLE, // flags
		S_NULL              // raisestate
	},

	{           // MT_CYBRAKDEMON_ELECTRIC_BARRIER
		-1,             // doomednum
		S_CYBRAKDEMONELECTRICBARRIER_INIT1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3k79,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_CYBRAKDEMONELECTRICBARRIER_DIE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_fizzle,     // deathsound
		10*FRACUNIT,    // speed
		48*FRACUNIT,    // radius
		160*FRACUNIT,   // height
		0,              // display offset
		DMG_ELECTRIC,   // mass
		1,              // damage
		sfx_beelec,     // activesound
		MF_PAIN|MF_FIRE|MF_NOGRAVITY|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_CYBRAKDEMON_MISSILE
		-1,             // doomednum
		S_CYBRAKDEMONMISSILE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_brakrl,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_CYBRAKDEMONMISSILE_EXPLODE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_brakrx,     // deathsound
		40*FRACUNIT,    // speed
		11*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		32*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CYBRAKDEMON_FLAMESHOT
		-1,             // doomednum
		S_CYBRAKDEMONFLAMESHOT_FLY1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3kc2s,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_CYBRAKDEMONFLAMESHOT_DIE, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		DMG_FIRE,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CYBRAKDEMON_FLAMEREST
		-1,             // doomednum
		S_CYBRAKDEMONFLAMEREST, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3kc2s,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_NULL,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		DMG_FIRE,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_PAIN|MF_FIRE|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CYBRAKDEMON_TARGET_RETICULE
		-1,             // doomednum
		S_CYBRAKDEMONTARGETRETICULE1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOSSEXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CYBRAKDEMON_TARGET_DOT
		-1,             // doomednum
		S_CYBRAKDEMONTARGETDOT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOSSEXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_CYBRAKDEMON_NAPALM_BOMB_LARGE
		-1,             // doomednum
		S_CYBRAKDEMONNAPALMBOMBLARGE_FLY1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3k81,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		20*TICRATE,     // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_CYBRAKDEMONNAPALMBOMBLARGE_DIE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k4e,      // deathsound
		10*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		48*FRACUNIT,    // damage
		sfx_s3k5d,      // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_BOUNCE|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_CYBRAKDEMON_NAPALM_BOMB_SMALL
		-1,             // doomednum
		S_CYBRAKDEMONNAPALMBOMBSMALL, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_CYBRAKDEMONNAPALMBOMBSMALL_DIE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k70,      // deathsound
		10*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		32*FRACUNIT,    // damage
		sfx_s3k99,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_BOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_CYBRAKDEMON_NAPALM_FLAMES
		-1,             // doomednum
		S_CYBRAKDEMONNAPALMFLAME_FLY1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_CYBRAKDEMONNAPALMFLAME_DIE, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE, // flags
		S_NULL          // raisestate
	},

	{           // MT_CYBRAKDEMON_VILE_EXPLOSION
		-1,             // doomednum
		S_CYBRAKDEMONVILEEXPLOSION1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3kb4,      // deathsound
		1*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},


	{           // MT_METALSONIC_RACE
		207,                // doomednum
		S_METALSONIC_RACE,  // spawnstate
		8,                  // spawnhealth
		S_NULL,             // seestate
		sfx_None,           // seesound
		0,                  // reactiontime
		sfx_None,           // attacksound
		S_NULL,             // painstate
		0,                  // painchance
		sfx_None,           // painsound
		S_NULL,             // meleestate
		S_NULL,             // missilestate
		S_PLAY_DEAD,        // deathstate
		S_NULL,             // xdeathstate
		sfx_None,           // deathsound
		0,                  // speed
		16*FRACUNIT,        // radius
		48*FRACUNIT,        // height
		0,                  // display offset
		0,                  // mass
		0,                  // damage
		sfx_None,           // activesound
		MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL              // raisestate
	},

	{           // MT_METALSONIC_BATTLE
		208,                // doomednum
		S_METALSONIC_FLOAT, // spawnstate
		8,                  // spawnhealth
		S_METALSONIC_DASH,  // seestate
		sfx_s3k54,          // seesound
		0,                  // reactiontime
		sfx_bechrg,         // attacksound
		S_METALSONIC_PAIN,  // painstate
		S_METALSONIC_VECTOR,// painchance
		sfx_dmpain,         // painsound
		S_METALSONIC_BADBOUNCE, // meleestate
		S_METALSONIC_SHOOT, // missilestate
		S_METALSONIC_DEATH1,// deathstate
		S_METALSONIC_FLEE1, // xdeathstate
		sfx_s3kb4,          // deathsound
		MT_ENERGYBALL,      // speed
		16*FRACUNIT,        // radius
		48*FRACUNIT,        // height
		0,                  // display offset
		0,                  // mass
		3,                  // damage
		sfx_mswarp,         // activesound
		MF_NOGRAVITY|MF_BOSS|MF_SLIDEME, // flags
		S_METALSONIC_RAISE  // raisestate
	},

	{           // MT_MSSHIELD_FRONT
		-1,             // doomednum
		S_MSSHIELD_F1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		52*FRACUNIT,    // height
		1,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MSGATHER
		-1,             // doomednum
		S_JETFUME1,     // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		24*FRACUNIT,    // speed
		6*FRACUNIT,     // radius
		12*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_RING
		300,            // doomednum
		S_RING,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGRING,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		38*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLINGRING
		-1,             // doomednum
		S_RING,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGRING,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_RING,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		38*FRACUNIT,    // speed
		15*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUESPHERE
		1706,           // doomednum
		S_BLUESPHERE,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGBLUESPHERE,        // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BLUESPHERESPARK, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k65,      // deathsound
		38*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_BLUESPHEREBONUS // raisestate
	},

	{           // MT_FLINGBLUESPHERE
		-1,             // doomednum
		S_BLUESPHERE,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGBLUESPHERE,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_BLUESPHERE,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BLUESPHERESPARK, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k65,     // deathsound
		38*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL, // flags
		S_BLUESPHEREBONUS // raisestate
	},

	{           // MT_BOMBSPHERE
		520,            // doomednum
		S_BOMBSPHERE1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_NULL,        // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SONIC3KBOSSEXPLOSION1, // deathstate
		S_NULL,         // xdeathstate
		sfx_cybdth,     // deathsound
		38*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDTEAMRING
		308,            // doomednum
		S_TEAMRING,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGRING,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		38*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUETEAMRING
		309,            // doomednum
		S_TEAMRING,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGRING,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		38*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_TOKEN
		312,            // doomednum
		S_TOKEN,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDFLAG
		310,            // doomednum
		S_REDFLAG,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_lvpass,     // deathsound
		8,              // speed
		24*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEFLAG
		311,            // doomednum
		S_BLUEFLAG,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_lvpass,     // deathsound
		8,              // speed
		24*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_EMBLEM
		322,            // doomednum
		S_EMBLEM1,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_ncitem,     // deathsound
		1,              // speed
		16*FRACUNIT,     // radius
		30*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_EMERALD1
		313,            // doomednum
		S_CEMG1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cgot,       // deathsound
		EMERALD1,       // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD2
		314,            // doomednum
		S_CEMG2,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cgot,       // deathsound
		EMERALD2,       // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD3
		315,            // doomednum
		S_CEMG3,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cgot,       // deathsound
		EMERALD3,       // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD4
		316,            // doomednum
		S_CEMG4,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cgot,       // deathsound
		EMERALD4,       // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD5
		317,            // doomednum
		S_CEMG5,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cgot,       // deathsound
		EMERALD5,       // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD6
		318,            // doomednum
		S_CEMG6,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cgot,       // deathsound
		EMERALD6,       // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD7
		319,            // doomednum
		S_CEMG7,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cgot,       // deathsound
		EMERALD7,       // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_EMERHUNT
		320,            // doomednum
		S_SHRD1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cgot,       // deathsound
		8,              // speed
		12*FRACUNIT,    // radius
		42*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EMERALDSPAWN
		321,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOSECTOR,  // flags
		S_NULL          // raisestate
	},

	{           // MT_FLINGEMERALD
		-1,             // doomednum
		S_CEMG1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cgot,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_FAN
		540,            // doomednum
		S_FAN,          // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		0,              // display offset
		5*FRACUNIT,     // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_STEAM
		541,            // doomednum
		S_STEAM1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_steam2,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_steam1,     // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		1*FRACUNIT,     // height
		0,              // display offset
		20*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_BUMPER
		542,            // doomednum
		S_BUMPER,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		5,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		-1,             // painchance
		sfx_s3kaa,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPRING|MF_NOGRAVITY, // flags
		S_BUMPERHIT     // raisestate
	},

	{           // MT_BALLOON
		543,            // doomednum
		S_BALLOON,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		2,              // painchance
		sfx_s3k77,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BALLOONPOP2,  // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		20*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPRING|MF_NOGRAVITY, // flags
		S_BALLOONPOP1   // raisestate
	},

	{           // MT_YELLOWSPRING
		550,            // doomednum
		S_YELLOWSPRING, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		20*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPRING, // flags
		S_YELLOWSPRING2 // raisestate
	},

	{           // MT_REDSPRING
		551,            // doomednum
		S_REDSPRING,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		32*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPRING, // flags
		S_REDSPRING2    // raisestate
	},

	{           // MT_BLUESPRING
		552,            // doomednum
		S_BLUESPRING,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		11*FRACUNIT,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPRING, // flags
		S_BLUESPRING2   // raisestate
	},

	{           // MT_YELLOWDIAG
		555,            // doomednum
		S_YDIAG1,       // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		20*FRACUNIT,    // mass
		20*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SPRING, // flags
		S_YDIAG2        // raisestate
	},

	{           // MT_REDDIAG
		556,            // doomednum
		S_RDIAG1,       // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		32*FRACUNIT,    // mass
		32*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SPRING, // flags
		S_RDIAG2        // raisestate
	},

	{           // MT_BLUEDIAG
		557,            // doomednum
		S_BDIAG1,       // spawnstate
		1,              // spawnhealth
		S_BDIAG2,       // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		11*FRACUNIT,    // mass
		11*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SPRING, // flags
		S_BDIAG2        // raisestate
	},

	{           // MT_YELLOWHORIZ
		558,            // doomednum
		S_YHORIZ1,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		36*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SPRING|MF_NOGRAVITY, // flags
		S_YHORIZ2       // raisestate
	},

	{           // MT_REDHORIZ
		559,            // doomednum
		S_RHORIZ1,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		72*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SPRING|MF_NOGRAVITY, // flags
		S_RHORIZ2       // raisestate
	},

	{           // MT_BLUEHORIZ
		560,            // doomednum
		S_BHORIZ1,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		11*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SPRING|MF_NOGRAVITY, // flags
		S_BHORIZ2       // raisestate
	},

	{          // MT_BOOSTERSEG
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		28*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP,  // flags
		S_NULL          // raisestate
	},

	{          // MT_BOOSTERROLLER
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		14*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP,    // flags
		S_NULL          // raisestate
	},

	{           // MT_YELLOWBOOSTER
		544,            // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_cdfm62,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		28*FRACUNIT,    // radius
		FRACUNIT,       // height
		0,              // display offset
		0,              // mass
		36*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SPRING|MF_NOGRAVITY, // flags
		S_BOOSTERSOUND  // raisestate
	},

	{           // MT_REDBOOSTER
		545,            // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_cdfm62,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		28*FRACUNIT,    // radius
		FRACUNIT,       // height
		0,              // display offset
		0,              // mass
		72*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SPRING|MF_NOGRAVITY, // flags
		S_BOOSTERSOUND  // raisestate
	},

	{           // MT_BUBBLES
		500,            // doomednum
		S_BUBBLES1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_SIGN
		501,            // doomednum
		S_SIGN,         // spawnstate
		1000,           // spawnhealth
		S_PLAY_SIGN,    // seestate
		sfx_lvpass,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_SIGNPLAYER,   // painstate
		MT_SPARK,       // painchance
		sfx_s3kb8,      // painsound
		S_EGGMANSIGN,   // meleestate
		S_CLEARSIGN,    // missilestate
		S_SIGNSTOP,     // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k64,      // deathsound
		8,              // speed
		36*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_SCENERY|MF_BOUNCE|MF_RUNSPAWNFUNC, // flags
		S_SIGNBOARD     // raisestate
	},

	{           // MT_SPIKEBALL
		521,            // doomednum
		S_SPIKEBALL1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		12*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		0,              // display offset
		DMG_SPIKE,      // mass
		1,              // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPINFIRE
		-1,             // doomednum
		S_SPINFIRE1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		DMG_FIRE,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY|MF_FIRE, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPIKE
		523,            // doomednum
		S_SPIKE1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_s3k64,      // painsound
		S_SPIKE4,       // meleestate
		S_NULL,         // missilestate
		S_SPIKED1,      // deathstate
		S_SPIKED2,      // xdeathstate
		sfx_mspogo,     // deathsound
		2*TICRATE,      // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_WALLSPIKE
		522,            // doomednum
		S_WALLSPIKE1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_s3k64,      // painsound
		S_WALLSPIKE4,   // meleestate
		S_NULL,         // missilestate
		S_WALLSPIKED1,  // deathstate
		S_WALLSPIKED2,  // xdeathstate
		sfx_mspogo,     // deathsound
		2*TICRATE,      // speed
		16*FRACUNIT,    // radius
		14*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SCENERY|MF_NOCLIPHEIGHT|MF_PAPERCOLLISION,  // flags
		S_NULL          // raisestate
	},

	{           // MT_WALLSPIKEBASE
		-1,            // doomednum
		S_WALLSPIKEBASE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		7*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPTHING,  // flags
		S_NULL          // raisestate
	},

	{           // MT_STARPOST
		502,            // doomednum
		S_STARPOST_IDLE, // spawnstate
		1,              // spawnhealth
		S_STARPOST_FLASH, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_STARPOST_STARTSPIN, // painstate
		0,              // painchance
		sfx_strpst,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		64*FRACUNIT,    // radius
		128*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_BIGMINE
		1012,           // doomednum
		S_BIGMINE_IDLE, // spawnstate
		1,              // spawnhealth
		S_BIGMINE_ALERT1, // seestate
		sfx_s3k5c,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_s3k86,      // painsound
		S_BIGMINE_SET1, // meleestate
		S_NULL,         // missilestate
		S_BIGMINE_SET2, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		28*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		MT_UWEXPLODE,   // mass
		0,              // damage
		sfx_s3k9e,      // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_SHOOTABLE|MF_ENEMY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLASTEXECUTOR
		756,            // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SHOOTABLE|MF_NOGRAVITY|MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANNONLAUNCHER
		1123,           // doomednum
		S_CANNONLAUNCHER1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		2*TICRATE,      // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		TICRATE,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_pop,        // activesound
		MF_NOGRAVITY|MF_NOSECTOR|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOXSPARKLE
		-1,             // doomednum
		S_BOXSPARKLE1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		3*FRACUNIT,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SCENERY|MF_NOBLOCKMAP|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_RING_BOX
		400,            // doomednum
		S_RING_BOX,     // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_RING_BOX,     // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_RING_ICON,   // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_PITY_BOX
		401,            // doomednum
		S_PITY_BOX,     // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_PITY_BOX,     // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_PITY_ICON,   // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_ATTRACT_BOX
		402,            // doomednum
		S_ATTRACT_BOX,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_ATTRACT_BOX,  // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_ATTRACT_ICON,// damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_FORCE_BOX
		403,            // doomednum
		S_FORCE_BOX,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_FORCE_BOX,    // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_FORCE_ICON,  // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARMAGEDDON_BOX
		404,            // doomednum
		S_ARMAGEDDON_BOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_ARMAGEDDON_BOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_ARMAGEDDON_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_WHIRLWIND_BOX
		405,            // doomednum
		S_WHIRLWIND_BOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_WHIRLWIND_BOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_WHIRLWIND_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_ELEMENTAL_BOX
		406,            // doomednum
		S_ELEMENTAL_BOX,     // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_ELEMENTAL_BOX,     // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_ELEMENTAL_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNEAKERS_BOX
		407,            // doomednum
		S_SNEAKERS_BOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_SNEAKERS_BOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_SNEAKERS_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_INVULN_BOX
		408,            // doomednum
		S_INVULN_BOX,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_INVULN_BOX,   // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_INVULN_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_1UP_BOX
		409,            // doomednum
		S_1UP_BOX,      // spawnstate
		1,              // spawnhealth
		S_PLAY_BOX1,    // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_1UP_BOX,      // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_1UP_ICON,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMAN_BOX
		410,            // doomednum
		S_EGGMAN_BOX,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_EGGMAN_BOX,   // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_EGGMAN_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_MIXUP_BOX
		411,            // doomednum
		S_MIXUP_BOX,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_MIXUP_BOX,    // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_MIXUP_ICON,  // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_MYSTERY_BOX
		-1, //412,      // doomednum
		S_MYSTERY_BOX,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_MYSTERY_BOX,  // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_UNKNOWN,     // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_GRAVITY_BOX
		413,            // doomednum
		S_GRAVITY_BOX,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_GRAVITY_BOX,  // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_GRAVITY_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_RECYCLER_BOX
		416,            // doomednum
		S_RECYCLER_BOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_RECYCLER_BOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_RECYCLER_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_SCORE1K_BOX
		418,            // doomednum
		S_SCORE1K_BOX,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_SCORE1K_BOX,  // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_SCORE1K_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_SCORE10K_BOX
		419,            // doomednum
		S_SCORE10K_BOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_SCORE10K_BOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_SCORE10K_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEAURA_BOX
		420,            // doomednum
		S_FLAMEAURA_BOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_FLAMEAURA_BOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_FLAMEAURA_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUBBLEWRAP_BOX
		421,            // doomednum
		S_BUBBLEWRAP_BOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_BUBBLEWRAP_BOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_BUBBLEWRAP_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_THUNDERCOIN_BOX
		422,            // doomednum
		S_THUNDERCOIN_BOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_THUNDERCOIN_BOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOX_POP1,     // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_THUNDERCOIN_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_PITY_GOLDBOX
		431,            // doomednum
		S_PITY_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_PITY_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_PITY_ICON,   // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_ATTRACT_GOLDBOX
		432,            // doomednum
		S_ATTRACT_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_ATTRACT_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_ATTRACT_ICON,// damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_FORCE_GOLDBOX
		433,            // doomednum
		S_FORCE_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_FORCE_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_FORCE_ICON,  // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARMAGEDDON_GOLDBOX
		434,            // doomednum
		S_ARMAGEDDON_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_ARMAGEDDON_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_ARMAGEDDON_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_WHIRLWIND_GOLDBOX
		435,            // doomednum
		S_WHIRLWIND_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_WHIRLWIND_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_WHIRLWIND_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_ELEMENTAL_GOLDBOX
		436,            // doomednum
		S_ELEMENTAL_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_ELEMENTAL_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_ELEMENTAL_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNEAKERS_GOLDBOX
		437,            // doomednum
		S_SNEAKERS_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_SNEAKERS_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_SNEAKERS_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_INVULN_GOLDBOX
		438,            // doomednum
		S_INVULN_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_INVULN_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_INVULN_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMAN_GOLDBOX
		440,            // doomednum
		S_EGGMAN_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_EGGMAN_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_EGGMAN_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_GRAVITY_GOLDBOX
		443,            // doomednum
		S_GRAVITY_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_GRAVITY_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_GRAVITY_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEAURA_GOLDBOX
		450,            // doomednum
		S_FLAMEAURA_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_FLAMEAURA_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_FLAMEAURA_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUBBLEWRAP_GOLDBOX
		451,            // doomednum
		S_BUBBLEWRAP_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_BUBBLEWRAP_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_BUBBLEWRAP_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_THUNDERCOIN_GOLDBOX
		452,            // doomednum
		S_THUNDERCOIN_GOLDBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_monton,     // attacksound
		S_THUNDERCOIN_GOLDBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOLDBOX_OFF1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		44*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_THUNDERCOIN_ICON, // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{  		   // MT_RING_REDBOX
		414,            // doomednum
		S_RING_REDBOX1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_RING_REDBOX1, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_REDBOX_POP1,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_RING_ICON,   // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_RING_BLUEBOX
		415,            // doomednum
		S_RING_BLUEBOX1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_RING_BLUEBOX1, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BLUEBOX_POP1, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		18*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		MT_RING_ICON,   // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_RING_ICON
		-1,              // doomednum
		S_RING_ICON1,    // spawnstate
		1,               // spawnhealth
		S_NULL,          // seestate
		sfx_itemup,      // seesound
		10,              // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		2*FRACUNIT,      // speed
		8*FRACUNIT,      // radius
		14*FRACUNIT,     // height
		0,               // display offset
		100,             // mass
		62*FRACUNIT,     // damage
		sfx_None,        // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL           // raisestate
	},

	{           // MT_PITY_ICON
		-1,             // doomednum
		S_PITY_ICON1,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_shield,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_ATTRACT_ICON
		-1,             // doomednum
		S_ATTRACT_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_attrsg,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_FORCE_ICON
		-1,             // doomednum
		S_FORCE_ICON1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_forcsg,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARMAGEDDON_ICON
		-1,             // doomednum
		S_ARMAGEDDON_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_armasg,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_WHIRLWIND_ICON
		-1,             // doomednum
		S_WHIRLWIND_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_wirlsg,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_ELEMENTAL_ICON
		-1,             // doomednum
		S_ELEMENTAL_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_elemsg,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNEAKERS_ICON
		-1,             // doomednum
		S_SNEAKERS_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_INVULN_ICON
		-1,             // doomednum
		S_INVULN_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_1UP_ICON
		-1,             // doomednum
		S_1UP_ICON1,    // spawnstate
		1,              // spawnhealth
		S_PLAY_ICON1,   // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMAN_ICON
		-1,             // doomednum
		S_EGGMAN_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_MIXUP_ICON
		-1,             // doomednum
		S_MIXUP_ICON1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_GRAVITY_ICON
		-1,             // doomednum
		S_GRAVITY_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		20*TICRATE,     // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_RECYCLER_ICON
		-1,             // doomednum
		S_RECYCLER_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_SCORE1K_ICON
		-1,             // doomednum
		S_SCORE1K_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_chchng,     // seesound
		1000,           // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_SCORE10K_ICON
		-1,             // doomednum
		S_SCORE10K_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_chchng,     // seesound
		10000,          // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEAURA_ICON
		-1,             // doomednum
		S_FLAMEAURA_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_s3k3e,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUBBLEWRAP_ICON
		-1,             // doomednum
		S_BUBBLEWRAP_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_s3k3f,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_THUNDERCOIN_ICON
		-1,             // doomednum
		S_THUNDERCOIN_ICON1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_s3k41,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_BOXICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKET
		-1,             // doomednum
		S_ROCKET,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_rlaunc,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20*FRACUNIT,    // speed
		11*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		20,             // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_LASER
		-1,             // doomednum
		S_LASER,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_rlaunc,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_LASERFLASH,   // meleestate
		S_LASER2,       // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20*FRACUNIT,    // speed
		11*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		20,             // damage
		sfx_None,       // activesound
		MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_TORPEDO
		-1,             // doomednum
		S_TORPEDO,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_rlaunc,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cybdth,     // deathsound
		20*FRACUNIT,    // speed
		11*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		20,             // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_TORPEDO2
		-1,             // doomednum
		S_TORPEDO,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cybdth,     // deathsound
		20*FRACUNIT,    // speed
		11*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		20,             // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ENERGYBALL
		-1,             // doomednum
		S_ENERGYBALL1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_bexpld,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		40*FRACUNIT,    // speed
		60*FRACUNIT,    // radius
		120*FRACUNIT,   // height
		0,              // display offset
		0,              // mass
		20,             // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_MINE
		-1,             // doomednum
		S_MINE1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_MINE_BOOM1,   // deathstate
		S_MINE_BOOM1,   // xdeathstate
		sfx_cybdth,     // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		10*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		64*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE, // flags
		S_NULL          // raisestate
	},

	{           // MT_JETTBULLET
		-1,             // doomednum
		S_JETBULLET1,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_TURRETLASER
		-1,             // doomednum
		S_TURRETLASER,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_TURRETLASEREXPLODE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_turhit,     // deathsound
		50*FRACUNIT,    // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANNONBALL
		-1,             // doomednum
		S_CANNONBALL1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_cannon,     // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_cybdth,     // deathsound
		16*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANNONBALLDECOR
		1124,           // doomednum
		S_CANNONBALL1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		16*FRACUNIT,    // speed
		20*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_PUSHABLE|MF_SLIDEME, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARROW
		-1,             // doomednum
		S_ARROW,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_s3ka0,      // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_ARROWBONK,    // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k52,      // deathsound
		16*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		DMG_SPIKE,      // mass
		1,              // damage
		sfx_s3k51,      // activesound
		MF_NOBLOCKMAP|MF_MISSILE, // flags
		S_NULL          // raisestate
	},

	{           // MT_DEMONFIRE
		-1,             // doomednum
		S_DEMONFIRE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		16*FRACUNIT,    // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_LETTER
		-1,             // doomednum
		S_LETTER,       // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		7*FRACUNIT,     // radius -- heaven
		13*FRACUNIT,    // height -- hell
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFLOWER1
		800,            // doomednum
		S_GFZFLOWERA,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFLOWER2
		801,            // doomednum
		S_GFZFLOWERB,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFLOWER3
		802,            // doomednum
		S_GFZFLOWERC,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEBERRYBUSH
		803,            // doomednum
		S_BLUEBERRYBUSH, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BERRYBUSH
		804,            // doomednum
		S_BERRYBUSH,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUSH
		805,            // doomednum
		S_BUSH,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZTREE
		806,            // doomednum
		S_GFZTREE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		128*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZBERRYTREE
		807,            // doomednum
		S_GFZBERRYTREE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		128*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZCHERRYTREE
		808,            // doomednum
		S_GFZCHERRYTREE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		128*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHECKERTREE
		809,            // doomednum
		S_CHECKERTREE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHECKERSUNSETTREE
		810,            // doomednum
		S_CHECKERSUNSETTREE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FHZTREE
		2102,           // doomednum
		S_FHZTREE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FHZPINKTREE
		2103,           // doomednum
		S_FHZPINKTREE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_POLYGONTREE
		811,            // doomednum
		S_POLYGONTREE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUSHTREE
		812,            // doomednum
		S_BUSHTREE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUSHREDTREE
		813,            // doomednum
		S_BUSHREDTREE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		200*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPRINGTREE
		1600,           // doomednum
		S_SPRINGTREE,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZFLOWER1
		900,            // doomednum
		S_THZFLOWERA,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZFLOWER2
		902,            // doomednum
		S_THZFLOWERB,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZFLOWER3
		903,            // doomednum
		S_THZFLOWERC,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZTREE
		904,            // doomednum
		S_THZTREE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZTREEBRANCH
		-1,             // doomednum
		S_THZTREEBRANCH1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ALARM
		901,            // doomednum
		S_ALARM1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_alarm,      // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPAWNCEILING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GARGOYLE
		1000,           // doomednum
		S_GARGOYLE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		21*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_statu2,     // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIGGARGOYLE
		1009,           // doomednum
		S_BIGGARGOYLE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		12*FRACUNIT,    // speed
		32*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_statu2,     // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_SEAWEED
		1001,           // doomednum
		S_SEAWEED1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WATERDRIP
		1002,           // doomednum
		S_DRIPA1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		15*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WATERDROP
		-1,             // doomednum
		S_DRIPB1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DRIPC1,       // deathstate
		S_NULL,         // xdeathstate
		sfx_wdrip1,     // deathsound
		0,              // speed
		2*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		8,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORAL1
		1003,           // doomednum
		S_CORAL1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		29*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORAL2
		1004,           // doomednum
		S_CORAL2,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		30*FRACUNIT,    // radius
		53*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORAL3
		1005,           // doomednum
		S_CORAL3,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		28*FRACUNIT,    // radius
		41*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORAL4
		1014,           // doomednum
		S_CORAL4,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		56*FRACUNIT,    // radius
		112*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORAL5
		1015,           // doomednum
		S_CORAL5,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		56*FRACUNIT,    // radius
		112*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUECRYSTAL
		1006,           // doomednum
		S_BLUECRYSTAL1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_KELP
		1007,           // doomednum
		S_KELP,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		292*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ANIMALGAETOP
		1013,            // doomednum
		S_ANIMALGAETOP1, // spawnstate
		1000,            // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		48*FRACUNIT,     // radius
		120*FRACUNIT,    // height
		0,               // display offset
		4,               // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOCLIP|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_ANIMALGAESEG
		-1,             // doomednum
		S_ANIMALGAESEG, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		120*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DSZSTALAGMITE
		1008,           // doomednum
		S_DSZSTALAGMITE,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		116*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SOLID, // flags
		S_NULL          // raisestate
	},

	{           // MT_DSZ2STALAGMITE
		1011,           // doomednum
		S_DSZ2STALAGMITE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		116*FRACUNIT,   // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SOLID, // flags
		S_NULL          // raisestate
	},

	{           // MT_LIGHTBEAM
		1010,           // doomednum
		S_LIGHTBEAM1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHAIN
		1100,           // doomednum
		S_CEZCHAIN,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		128*FRACUNIT,   // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAME
		1101,           // doomednum
		S_FLAME,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_FLAMEPARTICLE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		DMG_FIRE,       // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_PAIN|MF_FIRE, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEPARTICLE
		-1,             // doomednum
		S_FLAMEPARTICLE,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGSTATUE
		1102,           // doomednum
		S_EGGSTATUE1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		240*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_PUSHABLE|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MACEPOINT
		1104,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		10000,          // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHAINMACEPOINT
		1105,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		10000,          // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPRINGBALLPOINT
		1106,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		10000,          // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHAINPOINT
		1107,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		10000,          // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_HIDDEN_SLING
		1108,           // doomednum
		S_SLING1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FIREBARPOINT
		1109,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		200,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CUSTOMMACEPOINT
		1110,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		128*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		200,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{            // MT_SMALLMACECHAIN
		-1,               // doomednum
		S_SMALLMACECHAIN, // spawnstate
		1000,             // spawnhealth
		S_NULL,           // seestate
		sfx_None,         // seesound
		8,                // reactiontime
		sfx_None,         // attacksound
		S_NULL,           // painstate
		0,                // painchance
		sfx_None,         // painsound
		S_NULL,           // meleestate
		S_NULL,           // missilestate
		S_NULL,           // deathstate
		S_NULL,           // xdeathstate
		sfx_None,         // deathsound
		24*FRACUNIT,      // speed
		17*FRACUNIT,      // radius
		34*FRACUNIT,      // height
		0,                // display offset
		100,              // mass
		1,                // damage
		sfx_None,         // activesound
		MF_SCENERY|MF_NOGRAVITY, // flags
		S_NULL            // raisestate
	},

	{            // MT_BIGMACECHAIN
		-1,             // doomednum
		S_BIGMACECHAIN,	// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		48*FRACUNIT,    // speed
		34*FRACUNIT,    // radius
		68*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{            // MT_SMALLMACE
		1130,           // doomednum
		S_SMALLMACE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		24*FRACUNIT,    // speed
		17*FRACUNIT,    // radius
		34*FRACUNIT,    // height
		1,              // display offset
		0,              // mass
		1,              // damage
		sfx_s3kc9s, //sfx_mswing, -- activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{            // MT_BIGMACE
		1131,           // doomednum
		S_BIGMACE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		48*FRACUNIT,    // speed
		34*FRACUNIT,    // radius
		68*FRACUNIT,    // height
		1,              // display offset
		0,              // mass
		1,              // damage
		sfx_s3kc9s, //sfx_mswing, -- activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{            // MT_SMALLGRABCHAIN
		-1,               // doomednum
		S_SMALLGRABCHAIN, // spawnstate
		1000,             // spawnhealth
		S_NULL,           // seestate
		sfx_None,         // seesound
		8,                // reactiontime
		sfx_None,         // attacksound
		S_NULL,           // painstate
		0,                // painchance
		sfx_None,         // painsound
		S_NULL,           // meleestate
		S_NULL,           // missilestate
		S_NULL,           // deathstate
		S_NULL,           // xdeathstate
		sfx_None,         // deathsound
		24*FRACUNIT,      // speed
		17*FRACUNIT,      // radius
		34*FRACUNIT,      // height
		0,                // display offset
		100,              // mass
		1,                // damage
		sfx_None,         // activesound
		MF_SCENERY|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL            // raisestate
	},

	{            // MT_BIGGRABCHAIN
		-1,             // doomednum
		S_BIGGRABCHAIN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		48*FRACUNIT,    // speed
		34*FRACUNIT,    // radius
		68*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{            // MT_YELLOWSPRINGBALL
		1134,           // doomednum
		S_YELLOWSPRINGBALL, // spawnstate
		1000,           // spawnhealth
		S_YELLOWSPRINGBALL2, // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		24*FRACUNIT,    // speed
		17*FRACUNIT,    // radius
		34*FRACUNIT,    // height
		1,              // display offset
		20*FRACUNIT,    // mass
		0,              // damage
		sfx_mswing,     // activesound
		MF_SCENERY|MF_SPRING|MF_NOGRAVITY, // flags
		S_YELLOWSPRINGBALL2 // raisestate
	},

	{            // MT_REDSPRINGBALL
		1135,           // doomednum
		S_REDSPRINGBALL, // spawnstate
		1000,           // spawnhealth
		S_REDSPRINGBALL2, // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		24*FRACUNIT,    // speed
		17*FRACUNIT,    // radius
		34*FRACUNIT,    // height
		1,              // display offset
		32*FRACUNIT,    // mass
		0,              // damage
		sfx_mswing,     // activesound
		MF_SCENERY|MF_SPRING|MF_NOGRAVITY, // flags
		S_REDSPRINGBALL2 // raisestate
	},

	{            // MT_SMALLFIREBAR
		1136,           // doomednum
		S_SMALLFIREBAR1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_FLAMEPARTICLE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		24*FRACUNIT,    // speed
		17*FRACUNIT,    // radius
		34*FRACUNIT,    // height
		0,              // display offset
		DMG_FIRE,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_FIRE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{            // MT_BIGFIREBAR
		1137,           // doomednum
		S_BIGFIREBAR1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_FLAMEPARTICLE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		48*FRACUNIT,    // speed
		34*FRACUNIT,    // radius
		68*FRACUNIT,    // height
		1,              // display offset
		DMG_FIRE,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_FIRE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZFLOWER
		1103,           // doomednum
		S_CEZFLOWER,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZPOLE1
		1117,           // doomednum
		S_CEZPOLE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		224*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZPOLE2
		1118,           // doomednum
		S_CEZPOLE,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		224*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZBANNER1
		-1,             // doomednum
		S_CEZBANNER1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		224*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZBANNER2
		-1,             // doomednum
		S_CEZBANNER2,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		40*FRACUNIT,    // radius
		224*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PINETREE
		1114,           // doomednum
		S_PINETREE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		628*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SOLID|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZBUSH1
		1115,           // doomednum
		S_CEZBUSH1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CEZBUSH2
		1116,           // doomednum
		S_CEZBUSH2,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		3*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANDLE
		1119,           // doomednum
		S_CANDLE,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANDLEPRICKET
		1120,           // doomednum
		S_CANDLEPRICKET, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		176*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEHOLDER
		1121,           // doomednum
		S_FLAMEHOLDER,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_FIRETORCH
		1122,           // doomednum
		S_FIRETORCH,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_FLAMEPARTICLE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WAVINGFLAG1
		1128,           // doomednum
		S_WAVINGFLAG,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		208*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_WAVINGFLAG2
		1129,           // doomednum
		S_WAVINGFLAG,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		208*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_WAVINGFLAGSEG1
		-1,             // doomednum
		S_WAVINGFLAGSEG1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		1,              // height -- this is not a typo
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WAVINGFLAGSEG2
		-1,             // doomednum
		S_WAVINGFLAGSEG2, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		1,              // height -- this is not a typo
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CRAWLASTATUE
		1111,           // doomednum
		S_CRAWLASTATUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_FACESTABBERSTATUE
		1112,           // doomednum
		S_FACESTABBERSTATUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		72*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_SUSPICIOUSFACESTABBERSTATUE
		1113,           // doomednum
		S_SUSPICIOUSFACESTABBERSTATUE_WAIT, // spawnstate
		1000,           // spawnhealth
		S_SUSPICIOUSFACESTABBERSTATUE_BURST1, // seestate
		sfx_s3k6f,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		72*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		(statenum_t)MT_ROCKCRUMBLE3// raisestate
	},

	{           // MT_BRAMBLES
		1125,           // doomednum
		S_BRAMBLES,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIGTUMBLEWEED
		1200,           // doomednum
		S_BIGTUMBLEWEED,// spawnstate
		1000,           // spawnhealth
		S_BIGTUMBLEWEED_ROLL1, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_s3k64,      // activesound
		MF_SPECIAL|MF_BOUNCE,      // flags
		S_NULL          // raisestate
	},

	{           // MT_LITTLETUMBLEWEED
		1201,           // doomednum
		S_LITTLETUMBLEWEED,// spawnstate
		1000,           // spawnhealth
		S_LITTLETUMBLEWEED_ROLL1, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_s3k64,      // activesound
		MF_SPECIAL|MF_BOUNCE,      // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI1
		1203,           // doomednum
		S_CACTI1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		13*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI2
		1204,           // doomednum
		S_CACTI2,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		15*FRACUNIT,    // radius
		52*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI3
		1205,           // doomednum
		S_CACTI3,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		13*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI4
		1206,           // doomednum
		S_CACTI4,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		15*FRACUNIT,    // radius
		52*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI5
		1207,           // doomednum
		S_CACTI5,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI6
		1208,           // doomednum
		S_CACTI6,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		128*FRACUNIT,   // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI7
		1209,           // doomednum
		S_CACTI7,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		224*FRACUNIT,   // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI8
		1210,           // doomednum
		S_CACTI8,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		256*FRACUNIT,   // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI9
		1211,           // doomednum
		S_CACTI9,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SCENERY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI10
		1230,           // doomednum
		S_CACTI10,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		13*FRACUNIT,    // radius
		28*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTI11
		1231,           // doomednum
		S_CACTI11,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		15*FRACUNIT,    // radius
		60*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_PAIN|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTITINYSEG
		-1,             // doomednum
		S_CACTITINYSEG, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		13*FRACUNIT,    // radius
		28*FRACUNIT,    // height
		0,              // display offset
		DMG_SPIKE,      // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACTISMALLSEG
		-1,              // doomednum
		S_CACTISMALLSEG, // spawnstate
		1000,            // spawnhealth
		S_NULL,          // seestate
		sfx_None,        // seesound
		8,               // reactiontime
		sfx_None,        // attacksound
		S_NULL,          // painstate
		0,               // painchance
		sfx_None,        // painsound
		S_NULL,          // meleestate
		S_NULL,          // missilestate
		S_NULL,          // deathstate
		S_NULL,          // xdeathstate
		sfx_None,        // deathsound
		0,               // speed
		15*FRACUNIT,     // radius
		60*FRACUNIT,     // height
		0,               // display offset
		DMG_SPIKE,       // mass
		0,               // damage
		sfx_None,        // activesound
		MF_NOTHINK|MF_SCENERY|MF_PAIN|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARIDSIGN_CAUTION
		1212,           // doomednum
		S_ARIDSIGN_CAUTION,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		22*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SOLID|MF_PAPERCOLLISION, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARIDSIGN_CACTI
		1213,           // doomednum
		S_ARIDSIGN_CACTI,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		22*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SOLID|MF_PAPERCOLLISION, // flags
		S_NULL          // raisestate
	},

	{           // MT_ARIDSIGN_SHARPTURN
		1214,           // doomednum
		S_ARIDSIGN_SHARPTURN,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		192*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SOLID|MF_PAPERCOLLISION, // flags
		S_NULL          // raisestate
	},

	{           // MT_OILLAMP
		1215,           // doomednum
		S_OILLAMP,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		22*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k4b,      // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SPAWNCEILING, // flags
		S_NULL          // raisestate
	},

	{           // MT_TNTBARREL
		1216,           // doomednum
		S_TNTBARREL_STND1,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_bowl,       // attacksound
		S_TNTBARREL_EXPL1,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_TNTBARREL_FLYING,         // missilestate
		S_TNTBARREL_EXPL1,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k4e,      // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		63*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_PROXIMITYTNT
		1217,           // doomednum
		S_PROXIMITY_TNT,    // spawnstate
		1,              // spawnhealth
		S_PROXIMITY_TNT_TRIGGER1,         // seestate
		sfx_s3k5c,      // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k4e,      // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k89,      // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_DUSTDEVIL
		1218,           // doomednum
		S_DUSTDEVIL,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2,              // speed
		80*FRACUNIT,    // radius
		416*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k4b,      // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DUSTLAYER
		-1,             // doomednum
		S_DUSTLAYER1,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		64*FRACUNIT,    // radius
		256*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{          // MT_ARIDDUST
		-1,             // doomednum
		S_ARIDDUST1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{          // MT_MINECART
		-1,             // doomednum
		S_MINECART_IDLE,// spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		24*FRACUNIT,    // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_statu2,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_MINECART_DTH1,// deathstate
		S_NULL,         // xdeathstate
		sfx_s3k59,      // deathsound
		20*FRACUNIT,    // speed
		22*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k76,      // activesound
		MF_PUSHABLE,    // flags
		(statenum_t)MT_MINECARTSIDEMARK// raisestate
	},

	{          // MT_MINECARTSEG
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP,    // flags
		S_NULL          // raisestate
	},

	{          // MT_MINECARTSPAWNER
		1219,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		22*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{          // MT_MINECARTEND
		1220,           // doomednum
		S_MINECARTEND,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		160*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{          // MT_MINECARTENDSOLID
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_PAPERCOLLISION|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{          // MT_MINECARTSIDEMARK
		-1,             // doomednum
		S_MINECARTSIDEMARK2, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		22*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{          // MT_MINECARTSPARK
		-1,             // doomednum
		S_MINECARTSPARK,// spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		2*FRACUNIT,     // radius
		2*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_BOUNCE|MF_NOCLIPTHING|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{          // MT_SALOONDOOR
		-1,             // doomednum
		S_SALOONDOOR,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		48*FRACUNIT,    // radius
		160*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k90,      // activesound
		MF_SOLID|MF_NOGRAVITY|MF_RUNSPAWNFUNC|MF_PAPERCOLLISION|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{          // MT_SALOONDOORCENTER
		1221,           // doomednum
		S_SALOONDOORCENTER, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		96*FRACUNIT,    // radius
		160*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_NOGRAVITY|MF_RUNSPAWNFUNC|MF_PAPERCOLLISION|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{          // MT_TRAINCAMEOSPAWNER
		1222,           // doomednum
		S_TRAINCAMEOSPAWNER_1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		28*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{          // MT_TRAINSEG
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{          // MT_TRAINDUSTSPAWNER
		1223,           // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{          // MT_TRAINSTEAMSPAWNER
		1224,           // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{          // MT_MINECARTSWITCHPOINT
		1229,           // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		160*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEJET
		1300,           // doomednum
		S_FLAMEJETSTND, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_VERTICALFLAMEJET
		1301,           // doomednum
		S_FLAMEJETSTND, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEJETFLAME
		-1,             // doomednum
		S_FLAMEJETFLAME1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		5*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		DMG_FIRE,       // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_MISSILE|MF_FIRE, // flags
		S_NULL          // raisestate
	},

	{           // MT_FJSPINAXISA
		1302,           // doomednum
		S_FJSPINAXISA1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOCLIPTHING|MF_NOGRAVITY|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_FJSPINAXISB
		1303,           // doomednum
		S_FJSPINAXISB1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOCLIPTHING|MF_NOGRAVITY|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEJETFLAMEB
		-1,             // doomednum
		S_FLAMEJETFLAMEB1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_fire,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		18,             // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		DMG_FIRE,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_MISSILE|MF_FIRE|MF_NOBLOCKMAP|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_LAVAFALL
		1304,           // doomednum
		S_LAVAFALL_DORMANT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_lvfal1,     // seesound
		8,              // reactiontime
		sfx_s3kd5l,     // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		3200*FRACUNIT,  // speed
		30*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPAWNCEILING, // flags
		S_NULL          // raisestate
	},

	{           // MT_LAVAFALL_LAVA
		-1,             // doomednum
		S_LAVAFALL_LAVA1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_LAVAFALL_LAVA3, // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		30*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_PAIN|MF_NOGRAVITY|MF_FIRE, // flags
		S_NULL          // raisestate
	},

	{           // MT_LAVAFALLROCK
		-1,             // doomednum
		S_LAVAFALLROCK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROLLOUTSPAWN
		1305,           // doomednum
		S_ROLLOUTSPAWN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SPAWNCEILING|MF_SCENERY,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROLLOUTROCK
		-1,             // doomednum
		S_ROLLOUTROCK,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime (sets number of frames the rock cycles through)
		sfx_None,       // attacksound
		S_NULL,         // painstate
		12*TICRATE,     // painchance (sets how long an unridden rock should last before disappearing - set to 0 to disable)
		sfx_s3k49,      // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		32*FRACUNIT,    // speed
		30*FRACUNIT,    // radius
		60*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_PUSHABLE|MF_SOLID|MF_SLIDEME,  // flags
		S_NULL          // raisestate
	},

	{           // MT_BIGFERNLEAF
		-1,             // doomednum
		S_BIGFERNLEAF,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIGFERN
		1306,           // doomednum
		S_BIGFERN1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_JUNGLEPALM
		1307,           // doomednum
		S_JUNGLEPALM,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_TORCHFLOWER
		1308,           // doomednum
		S_TORCHFLOWER,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		14*FRACUNIT,    // radius
		110*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_WALLVINE_LONG
		1309,           // doomednum
		S_WALLVINE_LONG, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,    // radius
		288*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_WALLVINE_SHORT
		1310,           // doomednum
		S_WALLVINE_SHORT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,    // radius
		288*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_GLAREGOYLE
		1500,           // doomednum
		S_GLAREGOYLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		21*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_statu2,     // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_GLAREGOYLEUP
		1501,           // doomednum
		S_GLAREGOYLEUP,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		21*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_statu2,     // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_GLAREGOYLEDOWN
		1502,           // doomednum
		S_GLAREGOYLEDOWN,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		21*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_statu2,     // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_GLAREGOYLELONG
		1503,           // doomednum
		S_GLAREGOYLELONG,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		21*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_statu2,     // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_TARGET
		1504,           // doomednum
		S_TARGET_IDLE,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_TARGET_HIT1,  // deathstate
		S_TARGET_ALLDONE, // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_GREENFLAME
		1505,           // doomednum
		S_GREENFLAME,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_NULL,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,       // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEGARGOYLE
		1506,           // doomednum
		S_BLUEGARGOYLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		21*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_statu2,     // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE0
		1900,           // doomednum
		S_STG0,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE1
		1901,           // doomednum
		S_STG1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE2
		1902,           // doomednum
		S_STG2,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE3
		1903,           // doomednum
		S_STG3,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE4
		1904,           // doomednum
		S_STG4,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE5
		1905,           // doomednum
		S_STG5,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE6
		1906,           // doomednum
		S_STG6,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE7
		1907,           // doomednum
		S_STG7,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE8
		1908,           // doomednum
		S_STG8,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STALAGMITE9
		1909,           // doomednum
		S_STG9,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_XMASPOLE
		1850,           // doomednum
		S_XMASPOLE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANDYCANE
		1851,           // doomednum
		S_CANDYCANE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNOWMAN
		1852,           // doomednum
		S_SNOWMAN,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		25*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNOWMANHAT
		1853,           // doomednum
		S_SNOWMANHAT,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		25*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_LAMPPOST1
		1854,           // doomednum
		S_LAMPPOST1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		120*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_LAMPPOST2
		1855,           // doomednum
		S_LAMPPOST2,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		120*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_HANGSTAR
		1856,           // doomednum
		S_HANGSTAR,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4*FRACUNIT,     // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MISTLETOE
		2105,           // doomednum
		S_MISTLETOE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		52*FRACUNIT,    // radius
		106*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_XMASBLUEBERRYBUSH
		1859,           // doomednum
		S_XMASBLUEBERRYBUSH, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_XMASBERRYBUSH
		1857,           // doomednum
		S_XMASBERRYBUSH, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_XMASBUSH
		1858,           // doomednum
		S_XMASBUSH,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FHZICE1
		2100,           // doomednum
		S_FHZICE1,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FHZICE2
		2101,           // doomednum
		S_FHZICE2,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROSY
		2104,           // doomednum
		S_ROSY_IDLE1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_ENEMY|MF_SLIDEME, // flags -- "enemy" may seem weird but it doesn't have any unintended consequences in context because no MF_SHOOTABLE|MF_SPECIAL
		S_NULL          // raisestate
	},

	{           // MT_CDLHRT
		-1,             // doomednum
		S_LHRT,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		1,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_JACKO1
		2006,           // doomednum
		S_JACKO1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_JACKO1OVERLAY_1 // raisestate
	},

	{           // MT_JACKO2
		2007,           // doomednum
		S_JACKO2,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_JACKO2OVERLAY_1 // raisestate
	},

	{           // MT_JACKO3
		2008,           // doomednum
		S_JACKO3,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_JACKO3OVERLAY_1 // raisestate
	},

	{           // MT_HHZTREE_TOP
		2010,           // doomednum
		S_HHZTREE_TOP,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZTREE_PART
		-1,             // doomednum
		S_HHZTREE_TRUNK,// spawnstate
		1000,           // spawnhealth
		S_HHZTREE_LEAF, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		40*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZSHROOM
		2009,           // doomednum
		S_HHZSHROOM_1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZGRASS
		2001,           // doomednum
		S_HHZGRASS,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZTENTACLE1
		2002,           // doomednum
		S_HHZTENT1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZTENTACLE2
		2003,           // doomednum
		S_HHZTENT2,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZSTALAGMITE_TALL
		2004,           // doomednum
		S_HHZSTALAGMITE_TALL, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HHZSTALAGMITE_SHORT
		2005,           // doomednum
		S_HHZSTALAGMITE_SHORT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_SCENERY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	// No, I did not do all of this by hand.
	// I made a script to make all of these for me.
	// Ha HA. ~Inuyasha
	{           // MT_BSZTALLFLOWER_RED
		1400,           // doomednum
		S_BSZTALLFLOWER_RED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTALLFLOWER_PURPLE
		1401,           // doomednum
		S_BSZTALLFLOWER_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTALLFLOWER_BLUE
		1402,           // doomednum
		S_BSZTALLFLOWER_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTALLFLOWER_CYAN
		1403,           // doomednum
		S_BSZTALLFLOWER_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTALLFLOWER_YELLOW
		1404,           // doomednum
		S_BSZTALLFLOWER_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTALLFLOWER_ORANGE
		1405,           // doomednum
		S_BSZTALLFLOWER_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_RED
		1410,           // doomednum
		S_BSZFLOWER_RED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_PURPLE
		1411,           // doomednum
		S_BSZFLOWER_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_BLUE
		1412,           // doomednum
		S_BSZFLOWER_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_CYAN
		1413,           // doomednum
		S_BSZFLOWER_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_YELLOW
		1414,           // doomednum
		S_BSZFLOWER_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZFLOWER_ORANGE
		1415,           // doomednum
		S_BSZFLOWER_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_RED
		1420,           // doomednum
		S_BSZSHORTFLOWER_RED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_PURPLE
		1421,           // doomednum
		S_BSZSHORTFLOWER_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_BLUE
		1422,           // doomednum
		S_BSZSHORTFLOWER_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_CYAN
		1423,           // doomednum
		S_BSZSHORTFLOWER_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_YELLOW
		1424,           // doomednum
		S_BSZSHORTFLOWER_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHORTFLOWER_ORANGE
		1425,           // doomednum
		S_BSZSHORTFLOWER_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_RED
		1430,           // doomednum
		S_BSZTULIP_RED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_PURPLE
		1431,           // doomednum
		S_BSZTULIP_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_BLUE
		1432,           // doomednum
		S_BSZTULIP_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_CYAN
		1433,           // doomednum
		S_BSZTULIP_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_YELLOW
		1434,           // doomednum
		S_BSZTULIP_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZTULIP_ORANGE
		1435,           // doomednum
		S_BSZTULIP_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_RED
		1440,           // doomednum
		S_BSZCLUSTER_RED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_PURPLE
		1441,           // doomednum
		S_BSZCLUSTER_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_BLUE
		1442,           // doomednum
		S_BSZCLUSTER_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_CYAN
		1443,           // doomednum
		S_BSZCLUSTER_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_YELLOW
		1444,           // doomednum
		S_BSZCLUSTER_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLUSTER_ORANGE
		1445,           // doomednum
		S_BSZCLUSTER_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_RED
		1450,           // doomednum
		S_BSZBUSH_RED,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_PURPLE
		1451,           // doomednum
		S_BSZBUSH_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_BLUE
		1452,           // doomednum
		S_BSZBUSH_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_CYAN
		1453,           // doomednum
		S_BSZBUSH_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_YELLOW
		1454,           // doomednum
		S_BSZBUSH_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZBUSH_ORANGE
		1455,           // doomednum
		S_BSZBUSH_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_RED
		1460,           // doomednum
		S_BSZVINE_RED,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_PURPLE
		1461,           // doomednum
		S_BSZVINE_PURPLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_BLUE
		1462,           // doomednum
		S_BSZVINE_BLUE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_CYAN
		1463,           // doomednum
		S_BSZVINE_CYAN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_YELLOW
		1464,           // doomednum
		S_BSZVINE_YELLOW, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZVINE_ORANGE
		1465,           // doomednum
		S_BSZVINE_ORANGE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZSHRUB
		1470,           // doomednum
		S_BSZSHRUB,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSZCLOVER
		1471,           // doomednum
		S_BSZCLOVER,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIG_PALMTREE_TRUNK
		-1,             // doomednum
		S_BIG_PALMTREE_TRUNK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		160*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BIG_PALMTREE_TOP
		1473,           // doomednum
		S_BIG_PALMTREE_TOP, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		160*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_RUNSPAWNFUNC|MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PALMTREE_TRUNK
		-1,             // doomednum
		S_PALMTREE_TRUNK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PALMTREE_TOP
		1475,           // doomednum
		S_PALMTREE_TOP, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_RUNSPAWNFUNC|MF_NOTHINK|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DBALL
		1875,           // doomednum
		S_DBALL1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		54*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGSTATUE2
		1876,           // doomednum
		S_EGGSTATUE2,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_PUSHABLE|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ELEMENTAL_ORB
		-1,             // doomednum
		S_ELEM1,        // spawnstate
		1000,           // spawnhealth
		S_ELEMF1,       // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_ELEM13,       // painstate
		SKINCOLOR_NONE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		SH_ELEMENTAL,   // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		4,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_ELEMF9        // raisestate
	},

	{           // MT_ATTRACT_ORB
		-1,             // doomednum
		S_MAGN1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_MAGN13,       // painstate
		SKINCOLOR_NONE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		SH_ATTRACT,     // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		4,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FORCE_ORB
		-1,             // doomednum
		S_FORC1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_FORC11,       // painstate
		SKINCOLOR_NONE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		SH_FORCE,       // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		4,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_FORC21        // raisestate
	},

	{           // MT_ARMAGEDDON_ORB
		-1,             // doomednum
		S_ARMA1,        // spawnstate
		1000,           // spawnhealth
		S_ARMF1,        // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_NONE, // painchance
		sfx_None,       // painsound
		S_ARMB1,        // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		SH_ARMAGEDDON,  // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		4,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_WHIRLWIND_ORB
		-1,             // doomednum
		S_WIND1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_NONE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		SH_WHIRLWIND,        // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		4,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PITY_ORB
		-1,             // doomednum
		S_PITY1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		SKINCOLOR_NONE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		SH_PITY,        // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		4,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAMEAURA_ORB
		-1,             // doomednum
		S_FIRSB1,       // spawnstate
		1000,           // spawnhealth
		S_FIRS1,        // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_FIRSB10,      // painstate
		SKINCOLOR_NONE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		SH_FLAMEAURA,   // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		-4,             // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_FIRS10        // raisestate
	},

	{           // MT_BUBBLEWRAP_ORB
		-1,             // doomednum
		S_BUBSB1,       // spawnstate
		1000,           // spawnhealth
		S_BUBS1,        // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_BUBSB5,       // painstate
		SKINCOLOR_NONE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		SH_BUBBLEWRAP,  // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		4,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_BUBS10        // raisestate
	},

	{           // MT_THUNDERCOIN_ORB
		-1,             // doomednum
		S_ZAPSB1,       // spawnstate
		1000,           // spawnhealth
		S_ZAPS1,        // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_ZAPSB11,      // painstate
		SKINCOLOR_NONE, // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		SH_THUNDERCOIN, // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		-4,             // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_ZAPS14        // raisestate
	},

	{           // MT_THUNDERCOIN_SPARK
		-1,             // doomednum
		S_THUNDERCOIN_SPARK, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_IVSP
		-1,             // doomednum
		S_IVSP,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		3,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SUPERSPARK
		-1,             // doomednum
		S_SSPK1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// Bluebird
	{           // MT_FLICKY_01
		-1,             // doomednum
		S_FLICKY_01_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_01_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_01_CENTER
		2200,             // doomednum
		S_FLICKY_01_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_02
		-1,             // doomednum
		S_FLICKY_02_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_02_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_02_CENTER
		2201,             // doomednum
		S_FLICKY_02_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_03
		-1,             // doomednum
		S_FLICKY_03_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_03_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_03_CENTER
		2202,             // doomednum
		S_FLICKY_03_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_04
		-1,             // doomednum
		S_FLICKY_04_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_04_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_FLICKY_04_SWIM1, // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLICKY_04_CENTER
		2203,             // doomednum
		S_FLICKY_04_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_05
		-1,             // doomednum
		S_FLICKY_05_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_05_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_05_CENTER
		2204,             // doomednum
		S_FLICKY_05_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_06
		-1,             // doomednum
		S_FLICKY_06_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_06_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_06_CENTER
		2205,             // doomednum
		S_FLICKY_06_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_07
		-1,             // doomednum
		S_FLICKY_07_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_07_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_FLICKY_07_SWIM1, // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLICKY_07_CENTER
		2206,             // doomednum
		S_FLICKY_07_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_08
		-1,             // doomednum
		S_FLICKY_08_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_08_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_FLICKY_08_SWIM1, // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLICKY_08_CENTER
		2207,             // doomednum
		S_FLICKY_08_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_09
		-1,             // doomednum
		S_FLICKY_09_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_09_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_09_CENTER
		2208,             // doomednum
		S_FLICKY_09_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_10
		-1,             // doomednum
		S_FLICKY_10_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_10_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_10_CENTER
		2209,             // doomednum
		S_FLICKY_10_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_11
		-1,             // doomednum
		S_FLICKY_11_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_11_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_11_CENTER
		2210,             // doomednum
		S_FLICKY_11_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_12
		-1,             // doomednum
		S_FLICKY_12_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_12_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_12_CENTER
		2211,             // doomednum
		S_FLICKY_12_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_13
		-1,             // doomednum
		S_FLICKY_13_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_13_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_13_CENTER
		2212,             // doomednum
		S_FLICKY_13_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_14
		-1,             // doomednum
		S_FLICKY_14_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_14_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_14_CENTER
		2213,             // doomednum
		S_FLICKY_14_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_15
		-1,             // doomednum
		S_FLICKY_15_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_15_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_15_CENTER
		2214,             // doomednum
		S_FLICKY_15_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_FLICKY_16
		-1,             // doomednum
		S_FLICKY_16_OUT, // spawnstate
		1000,           // spawnhealth
		S_FLICKY_16_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_FLICKY_16_CENTER
		2215,             // doomednum
		S_FLICKY_16_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_SECRETFLICKY_01
		-1,             // doomednum
		S_SECRETFLICKY_01_OUT, // spawnstate
		1000,           // spawnhealth
		S_SECRETFLICKY_01_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_SECRETFLICKY_01_CENTER
		2216,             // doomednum
		S_SECRETFLICKY_01_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_SECRETFLICKY_02
		-1,             // doomednum
		S_SECRETFLICKY_02_OUT, // spawnstate
		1000,           // spawnhealth
		S_SECRETFLICKY_02_STAND, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING, // flags
		S_FLICKY_BUBBLE // raisestate
	},

	{           // MT_SECRETFLICKY_02_CENTER
		2217,             // doomednum
		S_SECRETFLICKY_02_CENTER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		20*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY|MF_RUNSPAWNFUNC, // flags
		S_NULL // raisestate
	},

	{           // MT_SEED
		-1,             // doomednum
		S_SEED,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-2*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_RAIN
		-1,             // doomednum
		S_RAIN1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-72*FRACUNIT,   // speed
		1*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_SNOWFLAKE
		-1,             // doomednum
		S_SNOW1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-2*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_SPLISH
		-1,             // doomednum
		S_SPLISH1,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		6*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_LAVASPLISH
		-1,             // doomednum
		S_LAVASPLISH,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		6*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SMOKE
		-1,             // doomednum
		S_SMOKE1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SMALLBUBBLE
		-1,             // doomednum
		S_SMALLBUBBLE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MEDIUMBUBBLE
		-1,             // doomednum
		S_MEDIUMBUBBLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EXTRALARGEBUBBLE
		-1,             // doomednum
		S_LARGEBUBBLE1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_POP1,         // deathstate
		S_NULL,         // xdeathstate
		sfx_gasp,       // deathsound
		8,              // speed
		23*FRACUNIT,    // radius
		43*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_SCENERY, // flags
		S_EXTRALARGEBUBBLE // raisestate
	},

	{           // MT_WATERZAP
		-1,             // doomednum
		S_WATERZAP,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPINDUST
		-1,             // doomednum
		S_SPINDUST1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_TFOG
		-1,             // doomednum
		S_FOG1,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_PARTICLE
		-1,             // doomednum
		S_PARTICLE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		1,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_PARTICLEGEN
		757,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SCORE
		-1,             // doomednum
		S_SCRA,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		3*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		1,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DROWNNUMBERS
		-1,             // doomednum
		S_ZERO1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		113,            // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GOTEMERALD
		-1,             // doomednum
		S_CEMG1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_ORBITEM1,     // meleestate
		S_ORBIDYA1,     // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k8a,      // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		112,            // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_LOCKON
		-1,             // doomednum
		S_LOCKON1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		111,            // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_LOCKONINF
		1126,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		111,            // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_TAG
		-1,             // doomednum
		S_TTAG,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		111,            // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GOTFLAG
		-1,             // doomednum
		S_GOTFLAG,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		64*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		111,            // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FINISHFLAG
		-1,             // doomednum
		S_FINISHFLAG,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		1,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// ambient water 1a (large)
	{           // MT_AWATERA
		700,            // doomednum
		S_INVISIBLE,    // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr1,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 1b (large)
	{           // MT_AWATERB
		701,            // doomednum
		S_INVISIBLE,    // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr2,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 2a (medium)
	{           // MT_AWATERC
		702,            // doomednum
		S_INVISIBLE,    // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr3,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 2b (medium)
	{           // MT_AWATERD
		703,            // doomednum
		S_INVISIBLE,    // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr4,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 3a (small)
	{           // MT_AWATERE
		704,            // doomednum
		S_INVISIBLE,    // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr5,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 3b (small)
	{           // MT_AWATERF
		705,            // doomednum
		S_INVISIBLE,    // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr6,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 4a (extra large)
	{           // MT_AWATERG
		706,            // doomednum
		S_INVISIBLE,    // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr7,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 4b (extra large)
	{           // MT_AWATERH
		707,            // doomednum
		S_INVISIBLE,    // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr8,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	{           // MT_RANDOMAMBIENT
		708,            // doomednum
		S_INVISIBLE,    // spawnstate
		512,            // spawnhealth: repeat speed
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	{           // MT_RANDOMAMBIENT2
		709,            // doomednum
		S_INVISIBLE,    // spawnstate
		220,            // spawnhealth: repeat speed
		S_NULL,         // seestate
		sfx_ambin2,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	{           // MT_MACHINEAMBIENCE
		710,            // doomednum
		S_INVISIBLE,    // spawnstate
		24,             // spawnhealth: repeat speed
		S_NULL,         // seestate
		sfx_ambmac,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1*FRACUNIT,     // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		20,             // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	{           // MT_CORK
		-1,             // doomednum
		S_CORK,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_corkp,      // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SMOKE1,       // deathstate
		S_NULL,         // xdeathstate
		sfx_corkh,      // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_LHRT
		-1,             // doomednum
		S_LHRT,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_SPRK1,        // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDRING
		-1,             // doomednum
		S_RRNG1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_wepfir,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

// Ring ammo: Health = amount given
	{           // MT_BOUNCERING
		301,            // doomednum
		S_BOUNCERINGAMMO, // spawnstate
		10,             // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_bouncering,  // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_RAILRING
		302,            // doomednum
		S_RAILRINGAMMO, // spawnstate
		5,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_railring,    // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_INFINITYRING
		303,            // doomednum
		S_INFINITYRINGAMMO,// spawnstate
		80,             // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_infinityring,// mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_AUTOMATICRING
		304,            // doomednum
		S_AUTOMATICRINGAMMO, // spawnstate
		40,             // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_automaticring, // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_EXPLOSIONRING
		305,            // doomednum
		S_EXPLOSIONRINGAMMO, // spawnstate
		5,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_explosionring, // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SCATTERRING
		306,            // doomednum
		S_SCATTERRINGAMMO, // spawnstate
		5,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_scatterring, // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_GRENADERING
		307,            // doomednum
		S_GRENADERINGAMMO, // spawnstate
		10,             // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_grenadering, // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

// Ring panels: Reactiontime = amount given
	{           // MT_BOUNCEPICKUP
		330,            // doomednum
		S_BOUNCEPICKUP, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		10,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		1,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BOUNCEPICKUPFADE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_ncitem,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_bouncering,  // mass
		2*TICRATE,      // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_RAILPICKUP
		331,            // doomednum
		S_RAILPICKUP,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		5,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		2,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_RAILPICKUPFADE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_ncitem,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_railring,    // mass
		2*TICRATE,      // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_AUTOPICKUP
		332,            // doomednum
		S_AUTOPICKUP,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		40,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		4,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_AUTOPICKUPFADE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_ncitem,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_automaticring, // mass
		2*TICRATE,      // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_EXPLODEPICKUP
		333,            // doomednum
		S_EXPLODEPICKUP,// spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		5,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		8,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_EXPLODEPICKUPFADE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_ncitem,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_explosionring, // mass
		2*TICRATE,      // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SCATTERPICKUP
		334,            // doomednum
		S_SCATTERPICKUP,// spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		5,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		8,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SCATTERPICKUPFADE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_ncitem,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_scatterring, // mass
		2*TICRATE,      // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_GRENADEPICKUP
		335,            // doomednum
		S_GRENADEPICKUP,// spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		10,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		8,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GRENADEPICKUPFADE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_ncitem,     // deathsound
		60*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		pw_grenadering, // mass
		2*TICRATE,      // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNBOUNCE
		-1,             // doomednum
		S_THROWNBOUNCE1,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_bnce1,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_bnce1,      // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY|MF_BOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNINFINITY
		-1,             // doomednum
		S_THROWNINFINITY1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_wepfir,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNAUTOMATIC
		-1,             // doomednum
		S_THROWNAUTOMATIC1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_wepfir,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNSCATTER
		-1,             // doomednum
		S_THROWNSCATTER,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_bnce2,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNEXPLOSION
		-1,             // doomednum
		S_THROWNEXPLOSION1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_cannon,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		192*FRACUNIT,   // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_RINGEXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNGRENADE
		-1,             // doomednum
		S_THROWNGRENADE1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_wepfir,     // seesound
		6*TICRATE,      // reactiontime (<-- Looking for the Grenade Ring's fuse? It's right here! Again!)
		sfx_gbeep,      // attacksound
		S_NULL,         // painstate
		192*FRACUNIT,   // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_RINGEXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		30*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_s3k5d,      // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_BOUNCE|MF_GRENADEBOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_COIN
		1800,           // doomednum
		S_COIN1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGCOIN,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_COINSPARKLE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_mario4,     // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLINGCOIN
		-1,             // doomednum
		S_COIN1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGCOIN,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_COIN,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_COINSPARKLE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_mario4,     // deathsound
		60*FRACUNIT,    // speed
		15*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_GOOMBA
		1801,           // doomednum
		S_GOOMBA1,      // spawnstate
		1,              // spawnhealth
		S_GOOMBA2,      // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GOOMBA_DEAD,  // deathstate
		S_NULL,         // xdeathstate
		sfx_mario5,     // deathsound
		6,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEGOOMBA
		1802,              // doomednum
		S_BLUEGOOMBA1,     // spawnstate
		1,                 // spawnhealth
		S_BLUEGOOMBA2,     // seestate
		sfx_None,          // seesound
		32,                // reactiontime
		sfx_None,          // attacksound
		S_NULL,            // painstate
		170,               // painchance
		sfx_None,          // painsound
		S_NULL,            // meleestate
		S_NULL,            // missilestate
		S_BLUEGOOMBA_DEAD, // deathstate
		S_NULL,            // xdeathstate
		sfx_mario5,        // deathsound
		6,                 // speed
		24*FRACUNIT,       // radius
		32*FRACUNIT,       // height
		0,                 // display offset
		100,               // mass
		0,                 // damage
		sfx_None,          // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL             // raisestate
	},

	{           // MT_FIREFLOWER
		1803,           // doomednum
		S_FIREFLOWER1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_FIREBALL
		-1,             // doomednum
		S_FIREBALL,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		40*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		DMG_FIRE,       // mass
		1,              // damage
		sfx_None,       // activesound
		MF_FIRE|MF_BOUNCE|MF_MISSILE, // flags
		S_NULL          // raisestate
	},

	{           // MT_FIREBALLTRAIL
		-1,                  // doomednum
		S_FIREBALLTRAIL1,    // spawnstate
		1000,                // spawnhealth
		S_NULL,              // seestate
		sfx_None,            // seesound
		8,                   // reactiontime
		sfx_None,            // attacksound
		S_NULL,              // painstate
		0,                   // painchance
		sfx_None,            // painsound
		S_NULL,              // meleestate
		S_NULL,              // missilestate
		S_NULL,              // deathstate
		S_NULL,              // xdeathstate
		sfx_None,            // deathsound
		0,                   // speed
		16*FRACUNIT,         // radius
		16*FRACUNIT,         // height
		0,                   // display offset
		0,                   // mass
		1,                   // damage
		sfx_None,            // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_RUNSPAWNFUNC, // flags
		S_NULL               // raisestate
	},

	{           // MT_SHELL
		1804,           // doomednum
		S_SHELL,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		16,             // speed
		16*FRACUNIT,    // radius
		20*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		1,              // damage
		sfx_mario1,     // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_BOUNCE, // flags
		S_NULL          // raisestate
	},

	{           // MT_PUMA
		1805,           // doomednum
		S_PUMA_START1,  // spawnstate
		1000,           // spawnhealth
		S_PUMA_START1,  // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_PUMA_DOWN1,   // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_PUMA_DOWN3,   // xdeathstate
		sfx_None,       // deathsound
		2000*FRACUNIT,  // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		DMG_FIRE,       // mass
		0,              // damage
		sfx_None,       // activesound
		MF_PAIN|MF_FIRE, // flags
		S_NULL          // raisestate
	},

	{           // MT_PUMATRAIL
		-1,             // doomednum
		S_PUMATRAIL1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_HAMMER
		-1,             // doomednum
		S_HAMMER,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_PAIN,        // flags
		S_NULL          // raisestate
	},
	{           // MT_KOOPA
		1806,           // doomednum
		S_KOOPA1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_PAIN,        // flags
		S_NULL          // raisestate
	},

	{           // MT_KOOPAFLAME
		-1,             // doomednum
		S_KOOPAFLAME1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		5*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		DMG_FIRE,       // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_MISSILE|MF_FIRE, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXE
		1807,           // doomednum
		S_AXE1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_MARIOBUSH1
		1808,           // doomednum
		S_MARIOBUSH1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MARIOBUSH2
		1809,           // doomednum
		S_MARIOBUSH2,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_TOAD
		1810,           // doomednum
		S_TOAD,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXIS
		1700,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		256*FRACUNIT,   // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXISTRANSFER
		1701,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		16*FRACUNIT,    // radius
		1,              // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP,    // flags
		S_NULL          // raisestate
	},

	{           // MT_AXISTRANSFERLINE
		1702,           // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		32*FRACUNIT,    // radius
		1,              // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP,    // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSDRONE
		1703,           // doomednum
		S_INVISIBLE,  // spawnstate
		120,            // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		1,              // display offset
		1000,           // mass
		0,              // damage
		sfx_ideya,      // activesound
		MF_NOGRAVITY|MF_NOCLIP|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSDRONE_MAN
		-1,           // doomednum
		S_INVISIBLE,  // spawnstate
		120,            // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NIGHTSDRONE_MAN1, // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		1,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,      // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSDRONE_SPARKLING
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NIGHTSDRONE_SPARKLING1, // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		1,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,      // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSDRONE_GOAL
		-1,             // doomednum
		S_INVISIBLE,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NIGHTSDRONE_GOAL1,  // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		-1,             // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSPARKLE
		-1,             // doomednum
		S_NIGHTSPARKLE1,// spawnstate
		1000,           // spawnhealth
		S_NIGHTSPARKLESUPER1, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSLOOPHELPER
		-1,             // doomednum
		S_NIGHTSLOOPHELPER,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSBUMPER
		1704,           // doomednum
		S_NIGHTSBUMPER1,// spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_nbmper,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		21000,          // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_HOOP
		-1,             // doomednum
		S_HOOP,         // spawnstate
		1000,           // spawnhealth
		S_HOOP_XMASA,   // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_HOOPCOLLIDE
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_SPECIAL|MF_NOTHINK, // flags
		S_NULL          // raisestate
	},

	{           // MT_HOOPCENTER
		-1,             // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSCORE
		-1,             // doomednum
		S_NIGHTSCORE10, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NIGHTSCORE10_2, // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSCHIP
		-1,             // doomednum
		S_NIGHTSCHIP,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGNIGHTSCHIP, // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_s3k33,      // painsound
		S_RING,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_ncchip,     // deathsound
		1,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NIGHTSCHIPBONUS // raisestate
	},

	{           // MT_FLINGNIGHTSCHIP
		-1,             // doomednum
		S_NIGHTSCHIP,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGNIGHTSCHIP, // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_NIGHTSCHIP,  // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_ncchip,     // deathsound
		38*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL, // flags
		S_NIGHTSCHIPBONUS // raisestate
	},

	{           // MT_NIGHTSSTAR
		-1,             // doomednum
		S_NIGHTSSTAR,   // spawnstate
		1000,           // spawnhealth
		S_NIGHTSSTARXMAS, // seestate
		sfx_None,       // seesound
		MT_FLINGNIGHTSSTAR, // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_s3k33,      // painsound
		S_RING,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_ncitem,     // deathsound
		1,              // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLINGNIGHTSSTAR
		-1,             // doomednum
		S_NIGHTSSTAR,   // spawnstate
		1000,           // spawnhealth
		S_NIGHTSSTARXMAS, // seestate
		sfx_None,       // seesound
		MT_FLINGNIGHTSSTAR, // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_NIGHTSSTAR,  // painchance
		sfx_s3k33,      // painsound
		S_RING,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_ncitem,     // deathsound
		38*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSSUPERLOOP
		1707,           // doomednum
		S_NIGHTSSUPERLOOP, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_ncspec,     // deathsound
		20*TICRATE,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_NIGHTSITEM,   // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSDRILLREFILL
		1708,           // doomednum
		S_NIGHTSDRILLREFILL, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_ncspec,     // deathsound
		96*20,          // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_NIGHTSITEM,   // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSHELPER
		1709,           // doomednum
		S_NIGHTSHELPER, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_ncspec,     // deathsound
		20*TICRATE,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_NIGHTSITEM,   // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSEXTRATIME
		1711,           // doomednum
		S_NIGHTSEXTRATIME, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_ncspec,     // deathsound
		30*TICRATE,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_NIGHTSITEM,   // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSLINKFREEZE
		1712,           // doomednum
		S_NIGHTSLINKFREEZE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_ncspec,     // deathsound
		15*TICRATE,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_NIGHTSITEM,   // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGCAPSULE
		1710,           // doomednum
		S_EGGCAPSULE,   // spawnstate
		20,             // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		72*FRACUNIT,    // radius
		144*FRACUNIT,   // height
		0,              // display offset
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_IDEYAANCHOR
		1714,           // doomednum
		S_INVISIBLE,    // spawnstate
		0,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		2*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTOPIANHELPER
		-1,             // doomednum
		S_NIGHTOPIANHELPER1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY,   // flags
		S_NULL          // raisestate
	},

	{           // MT_PIAN
		1602,           // doomednum
		S_PIAN0,        // spawnstate
		1000,           // spawnhealth
		S_PIAN1,        // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_PIANSING,     // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		FRACUNIT,       // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SHLEEP
		1601,           // doomednum
		S_SHLEEP1,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SHLEEPBOUNCE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_peww,       // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PENGUINATOR
		129,            // doomednum
		S_PENGUINATOR_LOOK, // spawnstate
		1,              // spawnhealth
		S_PENGUINATOR_WADDLE1, // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_ngjump,     // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_PENGUINATOR_SLIDE1, // meleestate
		S_PENGUINATOR_SLIDE1, // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		5,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY|MF_SLIDEME, // flags
		S_NULL          // raisestate
	},

	{           // MT_POPHAT
		130,            // doomednum -- happy anniversary!
		S_POPHAT_LOOK,  // spawnstate
		1,              // spawnhealth
		S_POPHAT_SHOOT1, // seestate
		sfx_None,       // seesound
		1,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		24*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY, // flags
		S_NULL          // raisestate
	},

	{           // MT_POPSHOT
		-1,             // doomednum
		S_POPSHOT,      // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_kc4c,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SONIC3KBOSSEXPLOSION1, // deathstate
		S_NULL,         // xdeathstate
		sfx_cybdth,     // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE, // flags
		S_NULL          // raisestate
	},

	{           // MT_POPSHOT_TRAIL
		-1,             // doomednum
		S_POPSHOT_TRAIL,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4*FRACUNIT,     // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HIVEELEMENTAL
		127,            // doomednum
		S_HIVEELEMENTAL_LOOK, // spawnstate
		2,              // spawnhealth
		S_HIVEELEMENTAL_PREPARE1, // seestate
		sfx_s3k74,      // seesound
		0,              // reactiontime
		sfx_s3k91,      // attacksound
		S_HIVEELEMENTAL_PAIN, // painstate
		0,              // painchance
		sfx_dmpain,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_HIVEELEMENTAL_DIE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3kb4,      // deathsound
		6*FRACUNIT,     // speed
		30*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_s3k72,      // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUMBLEBORE
		128,			// doomednum
		S_BUMBLEBORE_SPAWN, // spawnstate
		0,              // spawnhealth -- this is how you do drones...
		S_BUMBLEBORE_FLY1, // seestate
		sfx_s3k8e,      // seesound
		2,              // reactiontime
		sfx_s3k9e,      // attacksound
		S_BUMBLEBORE_STUCK1, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_BUMBLEBORE_RAISE, // meleestate
		S_NULL,         // missilestate
		S_BUMBLEBORE_DIE, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		4*FRACUNIT,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY|MF_NOGRAVITY|MF_SLIDEME, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUGGLE
		124,            // doomednum
		S_BUGGLEIDLE,   // spawnstate
		1,              // spawnhealth
		S_BUGGLEFLY,    // seestate
		sfx_None,       // seesound
		2,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		TICRATE,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		6*FRACUNIT,     // speed
		20*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_gbeep,      // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SMASHINGSPIKEBALL
		2000,           // doomednum
		S_SMASHSPIKE_FLOAT, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		18*FRACUNIT,    // radius
		28*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_PAIN, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACOLANTERN
		132,            // doomednum
		S_CACO_LOOK,    // spawnstate
		1,              // spawnhealth
		S_CACO_WAKE1,   // seestate
		sfx_s3k8a,      // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_CACO_DIE_FLAGS, // deathstate
		S_NULL,         // xdeathstate
		sfx_lntdie,     // deathsound
		FRACUNIT,       // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_lntsit,       // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACOSHARD
		-1,             // doomednum
		S_CACOSHARD_RANDOMIZE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_lntdie,     // deathsound
		FRACUNIT,       // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_MISSILE|MF_NOBLOCKMAP|MF_RUNSPAWNFUNC, // flags
		S_NULL          // raisestate
	},

	{           // MT_CACOFIRE
		-1,             // doomednum
		S_CACOFIRE1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_s3k70,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_CACOFIRE_EXPLODE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k81,      // deathsound
		20*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		20,             // damage
		sfx_s3k48,      // activesound
		MF_MISSILE|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPINBOBERT
		131,            // doomednum
		S_SPINBOBERT_MOVE_FLIPUP, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_s3ka0,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_s3k92,      // deathsound
		20*FRACUNIT,    // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		20,             // damage
		sfx_s3k48,      // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPINBOBERT_FIRE1
		-1,             // doomednum
		S_SPINBOBERT_FIRE_MOVE, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		(sfx_ghosty<<8),// mass
		20,             // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPINBOBERT_FIRE2
		-1,             // doomednum
		S_SPINBOBERT_FIRE_MOVE, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-10*FRACUNIT,   // speed - only difference from above
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		(sfx_ghosty<<8),// mass
		20,             // damage
		sfx_None,       // activesound
		MF_PAIN|MF_NOGRAVITY|MF_NOCLIPHEIGHT, // flags
		S_NULL          // raisestate
	},

	{           // MT_HANGSTER
		133,            // doomednum
		S_HANGSTER_LOOK, // spawnstate
		1,              // spawnhealth
		S_HANGSTER_SWOOP1, // seestate
		sfx_s3ka0,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD_FLICKY,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		20*FRACUNIT,    // speed
		24*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		20,             // damage
		sfx_s3k48,      // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_ENEMY|MF_NOGRAVITY|MF_SPAWNCEILING, // flags
		S_NULL          // raisestate
	},

	{           // MT_TELEPORTMAN
		751,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ALTVIEWMAN
		752,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CRUMBLEOBJ
		-1,             // doomednum
		S_CRUMBLE1,     // spawnstate
		1000,           // spawnhealth
		S_CRUMBLE1,     // seestate
		0,              // seesound
		1,              // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		200,            // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_crumbl,     // deathsound
		3,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// Waypoint for zoom tubes
	{           // MT_TUBEWAYPOINT
		753,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		2*FRACUNIT,     // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT,    // flags
		S_NULL          // raisestate
	},

	// for use with wind and current effects
	{           // MT_PUSH
		754,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY,  // flags
		S_NULL          // raisestate
	},

	// for use with wind and current effects
	{           // MT_PULL
		755,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GHOST
		-1,             // doomednum
		S_THOK,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		1,              // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_OVERLAY
		-1,             // doomednum
		S_NULL,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ANGLEMAN
		758,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOTHINK|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_POLYANCHOR
		760,            // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_POLYSPAWN
		761,            // doomednum
		S_INVISIBLE,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		0,              // display offset
		1000,           // mass
		8,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SKYBOX
		780,            // doomednum
		S_INVISIBLE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		0,              // display offset
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SCENERY|MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPARK
		-1,             // doomednum
		S_SPRK1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		2,              // display offset
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EXPLODE
		-1,             // doomednum
		S_XPLD1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_UWEXPLODE
		-1,             // doomednum
		S_WPLD1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		32,             // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		200,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1*FRACUNIT,     // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_DUST
		-1,             // doomednum
		S_DUST1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		3*FRACUNIT,     // speed
		FRACUNIT,       // radius
		FRACUNIT,       // height
		0,              // display offset
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKSPAWNER
		1202,           // doomednum
		S_ROCKSPAWN,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY,  // flags
		S_NULL          // raisestate
	},

	{           // MT_FALLINGROCK
		-1,             // doomednum
		S_ROCKCRUMBLEA, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		4,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_rocks1,     // activesound
		MF_PAIN|MF_BOUNCE,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE1
		-1,             // doomednum
		S_ROCKCRUMBLEA, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE2
		-1,             // doomednum
		S_ROCKCRUMBLEB, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE3
		-1,             // doomednum
		S_ROCKCRUMBLEC, //spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE4
		-1,             // doomednum
		S_ROCKCRUMBLED, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE5
		-1,             // doomednum
		S_ROCKCRUMBLEE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE6
		-1,             // doomednum
		S_ROCKCRUMBLEF, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE7
		-1,             // doomednum
		S_ROCKCRUMBLEG, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE8
		-1,             // doomednum
		S_ROCKCRUMBLEH, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE9
		-1,             // doomednum
		S_ROCKCRUMBLEI, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE10
		-1,             // doomednum
		S_ROCKCRUMBLEJ, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE11
		-1,             // doomednum
		S_ROCKCRUMBLEK, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE12
		-1,             // doomednum
		S_ROCKCRUMBLEL, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE13
		-1,             // doomednum
		S_ROCKCRUMBLEM, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE14
		-1,             // doomednum
		S_ROCKCRUMBLEN, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE15
		-1,             // doomednum
		S_ROCKCRUMBLEO, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE16
		-1,             // doomednum
		S_ROCKCRUMBLEP, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		0,              // display offset
		1000,           // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_SCENERY|MF_NOCLIPHEIGHT,  // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZDEBRIS
		-1,             // doomednum
		S_GFZDEBRIS,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_RUNSPAWNFUNC|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BRICKDEBRIS
		-1,             // doomednum
		S_BRICKDEBRIS,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_RUNSPAWNFUNC|MF_NOCLIPHEIGHT|MF_SCENERY,  // flags
		S_NULL          // raisestate
	},

	{           // MT_WOODDEBRIS
		-1,             // doomednum
		S_WOODDEBRIS,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_wbreak,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_RUNSPAWNFUNC|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDBRICKDEBRIS
		-1,             // doomednum
		S_REDBRICKDEBRIS,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_RUNSPAWNFUNC|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEBRICKDEBRIS
		-1,             // doomednum
		S_BLUEBRICKDEBRIS,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_RUNSPAWNFUNC|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_YELLOWBRICKDEBRIS
		-1,             // doomednum
		S_YELLOWBRICKDEBRIS,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		0,              // display offset
		100,            // mass
		0,              // damage
		sfx_crumbl,     // activesound
		MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_RUNSPAWNFUNC|MF_NOCLIPHEIGHT|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

#ifdef SEENAMES
	{           // MT_NAMECHECK
		-1,             // doomednum
		S_NAMECHECK,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		30*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		0,              // display offset
		0,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},
#endif
};

skincolor_t skincolors[MAXSKINCOLORS] = {
	{"None", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, SKINCOLOR_NONE, 0, 0, false}, // SKINCOLOR_NONE

	// Greyscale ranges
	{"White",  {0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x02, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x11}, SKINCOLOR_BLACK,  5,  0,         true}, // SKINCOLOR_WHITE
	{"Bone",   {0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x05, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x11, 0x12}, SKINCOLOR_JET,    7,  0,         true}, // SKINCOLOR_BONE
	{"Cloudy", {0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14}, SKINCOLOR_CARBON, 7,  0,         true}, // SKINCOLOR_CLOUDY
	{"Grey",   {0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18}, SKINCOLOR_AETHER, 12, 0,         true}, // SKINCOLOR_GREY
	{"Silver", {0x02, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1b, 0x1d, 0x1f}, SKINCOLOR_SLATE,  12, 0,         true}, // SKINCOLOR_SILVER
	{"Carbon", {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x16, 0x17, 0x17, 0x19, 0x19, 0x1a, 0x1a, 0x1b, 0x1c, 0x1d}, SKINCOLOR_CLOUDY, 7,  V_GRAYMAP, true}, // SKINCOLOR_CARBON
	{"Jet",    {0x00, 0x05, 0x0a, 0x0f, 0x14, 0x19, 0x1a, 0x1b, 0x1c, 0x1e, 0x1e, 0x1e, 0x1f, 0x1f, 0x1f, 0x1f}, SKINCOLOR_BONE,   7,  V_GRAYMAP, true}, // SKINCOLOR_JET
	{"Black",  {0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1b, 0x1b, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f, 0x1f}, SKINCOLOR_WHITE,  7,  V_GRAYMAP, true}, // SKINCOLOR_BLACK

	// Desaturated
	{"Aether",   {0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x91, 0x91, 0x91, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xaf}, SKINCOLOR_GREY,    15, 0,           true}, // SKINCOLOR_AETHER
	{"Slate",    {0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0xaa, 0xaa, 0xaa, 0xab, 0xac, 0xac, 0xad, 0xad, 0xae, 0xaf}, SKINCOLOR_SILVER,  12, 0,           true}, // SKINCOLOR_SLATE
	{"Bluebell", {0x90, 0x91, 0x92, 0x93, 0x94, 0x94, 0x95, 0xac, 0xac, 0xad, 0xad, 0xa8, 0xa8, 0xa9, 0xfd, 0xfe}, SKINCOLOR_COPPER,  4,  V_BLUEMAP,   true}, // SKINCOLOR_BLUEBELL
	{"Pink",     {0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0xd3, 0xd3, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0x2b, 0x2c, 0x2e}, SKINCOLOR_AZURE,   9,  V_REDMAP,    true}, // SKINCOLOR_PINK
	{"Yogurt",   {0xd0, 0x30, 0xd8, 0xd9, 0xda, 0xdb, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe3, 0xe6, 0xe8, 0xe9}, SKINCOLOR_RUST,    7,  V_BROWNMAP,  true}, // SKINCOLOR_YOGURT
	{"Brown",    {0xdf, 0xe0, 0xe1, 0xe2, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef}, SKINCOLOR_TAN,     2,  V_BROWNMAP,  true}, // SKINCOLOR_BROWN
	{"Bronze",   {0xde, 0xe0, 0xe1, 0xe4, 0xe7, 0xe9, 0xeb, 0xec, 0xed, 0xed, 0xed, 0x19, 0x19, 0x1b, 0x1d, 0x1e}, SKINCOLOR_KETCHUP, 0,  V_BROWNMAP,  true}, // SKINCOLOR_BRONZE
	{"Tan",      {0x51, 0x51, 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x56, 0x57, 0xf5, 0xf5, 0xf9, 0xf9, 0xed, 0xed}, SKINCOLOR_BROWN,   12, V_BROWNMAP,  true}, // SKINCOLOR_TAN
	{"Beige",    {0x54, 0x55, 0x56, 0x56, 0xf2, 0xf3, 0xf3, 0xf4, 0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfb, 0xed, 0xed}, SKINCOLOR_MOSS,    5,  V_BROWNMAP,  true}, // SKINCOLOR_BEIGE
	{"Moss",     {0x58, 0x58, 0x59, 0x59, 0x5a, 0x5a, 0x5b, 0x5b, 0x5b, 0x5c, 0x5d, 0x5d, 0x5e, 0x5e, 0x5f, 0x5f}, SKINCOLOR_BEIGE,   13, V_GREENMAP,  true}, // SKINCOLOR_MOSS
	{"Azure",    {0x90, 0x90, 0x91, 0x91, 0xaa, 0xaa, 0xab, 0xab, 0xab, 0xac, 0xad, 0xad, 0xae, 0xae, 0xaf, 0xaf}, SKINCOLOR_PINK,    5,  V_AZUREMAP,  true}, // SKINCOLOR_AZURE
	{"Lavender", {0xc0, 0xc0, 0xc1, 0xc1, 0xc2, 0xc2, 0xc3, 0xc3, 0xc3, 0xc4, 0xc5, 0xc5, 0xc6, 0xc6, 0xc7, 0xc7}, SKINCOLOR_GOLD,    4,  V_PURPLEMAP, true}, // SKINCOLOR_LAVENDER

	// Viv's vivid colours (toast 21/07/17)
	{"Ruby",       {0xb0, 0xb0, 0xc9, 0xca, 0xcc, 0x26, 0x27, 0x28, 0x29, 0x2a, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xfd}, SKINCOLOR_EMERALD,    10, V_REDMAP,     true}, // SKINCOLOR_RUBY
	{"Salmon",     {0xd0, 0xd0, 0xd1, 0xd2, 0x20, 0x21, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e}, SKINCOLOR_FOREST,     6,  V_REDMAP,     true}, // SKINCOLOR_SALMON
	{"Red",        {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x47, 0x2e, 0x2f}, SKINCOLOR_GREEN,      10, V_REDMAP,     true}, // SKINCOLOR_RED
	{"Crimson",    {0x27, 0x27, 0x28, 0x28, 0x29, 0x2a, 0x2b, 0x2b, 0x2c, 0x2d, 0x2e, 0x2e, 0x2e, 0x2f, 0x2f, 0x1f}, SKINCOLOR_ICY,        10, V_REDMAP,     true}, // SKINCOLOR_CRIMSON
	{"Flame",      {0x31, 0x32, 0x33, 0x36, 0x22, 0x22, 0x25, 0x25, 0x25, 0xcd, 0xcf, 0xcf, 0xc5, 0xc5, 0xc7, 0xc7}, SKINCOLOR_PURPLE,     8,  V_REDMAP,     true}, // SKINCOLOR_FLAME
	{"Ketchup",    {0x48, 0x49, 0x40, 0x33, 0x34, 0x36, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2b, 0x2c, 0x47, 0x2e, 0x2f}, SKINCOLOR_BRONZE,     8,  V_REDMAP,     true}, // SKINCOLOR_KETCHUP
	{"Peachy",     {0xd0, 0x30, 0x31, 0x31, 0x32, 0x32, 0xdc, 0xdc, 0xdc, 0xd3, 0xd4, 0xd4, 0xcc, 0xcd, 0xce, 0xcf}, SKINCOLOR_TEAL,       7,  V_ROSYMAP,    true}, // SKINCOLOR_PEACHY
	{"Quail",      {0xd8, 0xd9, 0xdb, 0xdc, 0xde, 0xdf, 0xd5, 0xd5, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0x1d, 0x1f}, SKINCOLOR_WAVE,       5,  V_BROWNMAP,   true}, // SKINCOLOR_QUAIL
	{"Sunset",     {0x51, 0x52, 0x40, 0x40, 0x34, 0x36, 0xd5, 0xd5, 0xd6, 0xd7, 0xcf, 0xcf, 0xc6, 0xc6, 0xc7, 0xfe}, SKINCOLOR_SAPPHIRE,   5,  V_ORANGEMAP,  true}, // SKINCOLOR_SUNSET
	{"Copper",     {0x58, 0x54, 0x40, 0x34, 0x35, 0x38, 0x3a, 0x3c, 0x3d, 0x2a, 0x2b, 0x2c, 0x2c, 0xba, 0xba, 0xbb}, SKINCOLOR_BLUEBELL,   5,  V_ORANGEMAP,  true}, // SKINCOLOR_COPPER
	{"Apricot",    {0x00, 0xd8, 0xd9, 0xda, 0xdb, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e}, SKINCOLOR_CYAN,       4,  V_ORANGEMAP,  true}, // SKINCOLOR_APRICOT
	{"Orange",     {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x2c}, SKINCOLOR_BLUE,       4,  V_ORANGEMAP,  true}, // SKINCOLOR_ORANGE
	{"Rust",       {0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3c, 0x3d, 0x3d, 0x3d, 0x3f, 0x2c, 0x2d, 0x47, 0x2e, 0x2f, 0x2f}, SKINCOLOR_YOGURT,     8,  V_ORANGEMAP,  true}, // SKINCOLOR_RUST
	{"Gold",       {0x51, 0x51, 0x54, 0x54, 0x41, 0x42, 0x43, 0x43, 0x44, 0x45, 0x46, 0x3f, 0x2d, 0x2e, 0x2f, 0x2f}, SKINCOLOR_LAVENDER,   10, V_YELLOWMAP,  true}, // SKINCOLOR_GOLD
	{"Sandy",      {0x53, 0x40, 0x41, 0x42, 0x43, 0xe6, 0xe9, 0xe9, 0xea, 0xec, 0xec, 0xc6, 0xc6, 0xc7, 0xc7, 0xfe}, SKINCOLOR_SKY,        8,  V_YELLOWMAP,  true}, // SKINCOLOR_SANDY
	{"Yellow",     {0x52, 0x53, 0x49, 0x49, 0x4a, 0x4a, 0x4b, 0x4b, 0x4b, 0x4c, 0x4d, 0x4d, 0x4e, 0x4e, 0x4f, 0xed}, SKINCOLOR_CORNFLOWER, 8,  V_YELLOWMAP,  true}, // SKINCOLOR_YELLOW
	{"Olive",      {0x4b, 0x4b, 0x4c, 0x4c, 0x4d, 0x4e, 0xe7, 0xe7, 0xe9, 0xc5, 0xc5, 0xc6, 0xc6, 0xc7, 0xc7, 0xfd}, SKINCOLOR_DUSK,       3,  V_YELLOWMAP,  true}, // SKINCOLOR_OLIVE
	{"Lime",       {0x50, 0x51, 0x52, 0x53, 0x48, 0xbc, 0xbd, 0xbe, 0xbe, 0xbf, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f}, SKINCOLOR_MAGENTA,    9,  V_PERIDOTMAP, true}, // SKINCOLOR_LIME
	{"Peridot",    {0x58, 0x58, 0xbc, 0xbc, 0xbd, 0xbd, 0xbe, 0xbe, 0xbe, 0xbf, 0x5e, 0x5e, 0x5f, 0x5f, 0x77, 0x77}, SKINCOLOR_COBALT,     2,  V_PERIDOTMAP, true}, // SKINCOLOR_PERIDOT
	{"Apple",      {0x49, 0x49, 0xbc, 0xbd, 0xbe, 0xbe, 0xbe, 0x67, 0x69, 0x6a, 0x6b, 0x6b, 0x6c, 0x6d, 0x6d, 0x6d}, SKINCOLOR_RASPBERRY,  13, V_PERIDOTMAP, true}, // SKINCOLOR_APPLE
	{"Green",      {0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f}, SKINCOLOR_RED,        6,  V_GREENMAP,   true}, // SKINCOLOR_GREEN
	{"Forest",     {0x65, 0x66, 0x67, 0x68, 0x69, 0x69, 0x6a, 0x6b, 0x6b, 0x6c, 0x6d, 0x6d, 0x6e, 0x6e, 0x6e, 0x6f}, SKINCOLOR_SALMON,     9,  V_GREENMAP,   true}, // SKINCOLOR_FOREST
	{"Emerald",    {0x70, 0x70, 0x71, 0x71, 0x72, 0x72, 0x73, 0x73, 0x73, 0x74, 0x75, 0x75, 0x76, 0x76, 0x77, 0x77}, SKINCOLOR_RUBY,       4,  V_GREENMAP,   true}, // SKINCOLOR_EMERALD
	{"Mint",       {0x00, 0x00, 0x58, 0x58, 0x59, 0x62, 0x62, 0x62, 0x64, 0x67, 0x7e, 0x7e, 0x8f, 0x8f, 0x8a, 0x8a}, SKINCOLOR_VIOLET,     5,  V_GREENMAP,   true}, // SKINCOLOR_MINT
	{"Seafoam",    {0x01, 0x58, 0x59, 0x5a, 0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x8f, 0x8f, 0x8a, 0x8a, 0x8a, 0xfd, 0xfd}, SKINCOLOR_PLUM,       6,  V_AQUAMAP,    true}, // SKINCOLOR_SEAFOAM
	{"Aqua",       {0x78, 0x79, 0x7a, 0x7a, 0x7b, 0x7b, 0x7c, 0x7c, 0x7c, 0x7d, 0x7e, 0x7e, 0x7f, 0x7f, 0x76, 0x77}, SKINCOLOR_ROSY,       7,  V_AQUAMAP,    true}, // SKINCOLOR_AQUA
	{"Teal",       {0x78, 0x78, 0x8c, 0x8c, 0x8d, 0x8d, 0x8d, 0x8e, 0x8e, 0x8f, 0x8f, 0x8f, 0x8a, 0x8a, 0x8a, 0x8a}, SKINCOLOR_PEACHY,     7,  V_SKYMAP,     true}, // SKINCOLOR_TEAL
	{"Wave",       {0x00, 0x78, 0x78, 0x79, 0x8d, 0x87, 0x88, 0x89, 0x89, 0xae, 0xa8, 0xa8, 0xa9, 0xa9, 0xfd, 0xfd}, SKINCOLOR_QUAIL,      5,  V_SKYMAP,     true}, // SKINCOLOR_WAVE
	{"Cyan",       {0x80, 0x81, 0xff, 0xff, 0x83, 0x83, 0x8d, 0x8d, 0x8d, 0x8e, 0x7e, 0x7f, 0x76, 0x76, 0x77, 0x6e}, SKINCOLOR_APRICOT,    6,  V_SKYMAP,     true}, // SKINCOLOR_CYAN
	{"Sky",        {0x80, 0x80, 0x81, 0x82, 0x83, 0x83, 0x84, 0x85, 0x85, 0x86, 0x87, 0x88, 0x89, 0x89, 0x8a, 0x8b}, SKINCOLOR_SANDY,      1,  V_SKYMAP,     true}, // SKINCOLOR_SKY
	{"Cerulean",   {0x85, 0x86, 0x87, 0x88, 0x88, 0x89, 0x89, 0x89, 0x8a, 0x8a, 0xfd, 0xfd, 0xfd, 0x1f, 0x1f, 0x1f}, SKINCOLOR_NEON,       4,  V_SKYMAP,     true}, // SKINCOLOR_CERULEAN
	{"Icy",        {0x00, 0x00, 0x00, 0x00, 0x80, 0x81, 0x83, 0x83, 0x86, 0x87, 0x95, 0x95, 0xad, 0xad, 0xae, 0xaf}, SKINCOLOR_CRIMSON,    0,  V_SKYMAP,     true}, // SKINCOLOR_ICY
	{"Sapphire",   {0x80, 0x83, 0x86, 0x87, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xfd, 0xfe}, SKINCOLOR_SUNSET,     5,  V_SKYMAP,     true}, // SKINCOLOR_SAPPHIRE
	{"Cornflower", {0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x9a, 0x9c, 0x9d, 0x9d, 0x9e, 0x9e, 0x9e}, SKINCOLOR_YELLOW,     4,  V_BLUEMAP,    true}, // SKINCOLOR_CORNFLOWER
	{"Blue",       {0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xfd, 0xfe}, SKINCOLOR_ORANGE,     5,  V_BLUEMAP,    true}, // SKINCOLOR_BLUE
	{"Cobalt",     {0x93, 0x94, 0x95, 0x96, 0x98, 0x9a, 0x9b, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xfd, 0xfd, 0xfe, 0xfe}, SKINCOLOR_PERIDOT,    5,  V_BLUEMAP,    true}, // SKINCOLOR_COBALT
	{"Vapor",      {0x80, 0x81, 0x83, 0x86, 0x94, 0x94, 0xa3, 0xa3, 0xa4, 0xa6, 0xa6, 0xa6, 0xa8, 0xa8, 0xa9, 0xa9}, SKINCOLOR_LILAC,      4,  V_SKYMAP,     true}, // SKINCOLOR_VAPOR
	{"Dusk",       {0x92, 0x93, 0x94, 0x94, 0xac, 0xad, 0xad, 0xad, 0xae, 0xae, 0xaf, 0xaf, 0xa9, 0xa9, 0xfd, 0xfd}, SKINCOLOR_OLIVE,      0,  V_BLUEMAP,    true}, // SKINCOLOR_DUSK
	{"Pastel",     {0x90, 0x90, 0xa0, 0xa0, 0xa1, 0xa1, 0xa2, 0xa2, 0xa2, 0xa3, 0xa4, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8}, SKINCOLOR_BUBBLEGUM,  9,  V_PURPLEMAP,  true}, // SKINCOLOR_PASTEL
	{"Purple",     {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa5, 0xa6, 0xa7, 0xa7, 0xa8, 0xa8, 0xa9, 0xa9}, SKINCOLOR_FLAME,      7,  V_PURPLEMAP,  true}, // SKINCOLOR_PURPLE
	{"Bubblegum",  {0x00, 0xd0, 0xd0, 0xc8, 0xc8, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8}, SKINCOLOR_PASTEL,     8,  V_MAGENTAMAP, true}, // SKINCOLOR_BUBBLEGUM
	{"Magenta",    {0xb3, 0xb3, 0xb4, 0xb5, 0xb6, 0xb6, 0xb7, 0xb7, 0xb7, 0xb8, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xbb}, SKINCOLOR_LIME,       6,  V_MAGENTAMAP, true}, // SKINCOLOR_MAGENTA
	{"Neon",       {0xb3, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xbb, 0xc7, 0xc7, 0x1d, 0x1d, 0x1e}, SKINCOLOR_CERULEAN,   2,  V_MAGENTAMAP, true}, // SKINCOLOR_NEON
	{"Violet",     {0xd0, 0xd1, 0xd2, 0xca, 0xcc, 0xb8, 0xb9, 0xb9, 0xba, 0xa8, 0xa8, 0xa9, 0xa9, 0xfd, 0xfe, 0xfe}, SKINCOLOR_MINT,       6,  V_MAGENTAMAP, true}, // SKINCOLOR_VIOLET
	{"Lilac",      {0x00, 0xd0, 0xd1, 0xd2, 0xd3, 0xc1, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc5, 0xc6, 0xc6, 0xfe, 0x1f}, SKINCOLOR_VAPOR,      4,  V_ROSYMAP,    true}, // SKINCOLOR_LILAC
	{"Plum",       {0xc8, 0xd3, 0xd5, 0xd6, 0xd7, 0xce, 0xcf, 0xb9, 0xb9, 0xba, 0xba, 0xa9, 0xa9, 0xa9, 0xfd, 0xfe}, SKINCOLOR_MINT,       7,  V_ROSYMAP,    true}, // SKINCOLOR_PLUM
	{"Raspberry",  {0xc8, 0xc9, 0xca, 0xcb, 0xcb, 0xcc, 0xcd, 0xcd, 0xce, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xfe, 0xfe}, SKINCOLOR_APPLE,      13, V_MAGENTAMAP, true}, // SKINCOLOR_RASPBERRY
	{"Rosy",       {0xfc, 0xc8, 0xc8, 0xc9, 0xc9, 0xca, 0xca, 0xcb, 0xcb, 0xcc, 0xcc, 0xcd, 0xcd, 0xce, 0xce, 0xcf}, SKINCOLOR_AQUA,       1,  V_ROSYMAP,    true}, // SKINCOLOR_ROSY

	// super
	{"Super Silver 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03}, SKINCOLOR_BLACK, 15, 0,         false}, // SKINCOLOR_SUPERSILVER1
	{"Super Silver 2", {0x00, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07}, SKINCOLOR_BLACK, 6,  0,         false}, // SKINCOLOR_SUPERSILVER2
	{"Super Silver 3", {0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07, 0x09, 0x0b}, SKINCOLOR_BLACK, 5,  0,         false}, // SKINCOLOR_SUPERSILVER3
	{"Super Silver 4", {0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f, 0x11}, SKINCOLOR_BLACK, 5,  V_GRAYMAP, false}, // SKINCOLOR_SUPERSILVER4
	{"Super Silver 5", {0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f, 0x11, 0x13}, SKINCOLOR_BLACK, 5,  V_GRAYMAP, false}, // SKINCOLOR_SUPERSILVER5

	{"Super Red 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2}, SKINCOLOR_CYAN, 15, 0,         false}, // SKINCOLOR_SUPERRED1
	{"Super Red 2", {0x00, 0x00, 0x00, 0xd0, 0xd0, 0xd0, 0xd1, 0xd1, 0xd1, 0xd2, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21}, SKINCOLOR_CYAN, 14, V_ROSYMAP, false}, // SKINCOLOR_SUPERRED2
	{"Super Red 3", {0x00, 0x00, 0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23}, SKINCOLOR_CYAN, 13, V_REDMAP,  false}, // SKINCOLOR_SUPERRED3
	{"Super Red 4", {0x00, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24}, SKINCOLOR_CYAN, 11, V_REDMAP,  false}, // SKINCOLOR_SUPERRED4
	{"Super Red 5", {0xd0, 0xd1, 0xd2, 0xd2, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25}, SKINCOLOR_CYAN, 10, V_REDMAP,  false}, // SKINCOLOR_SUPERRED5

	{"Super Orange 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x30, 0x31, 0x32, 0x33, 0x34}, SKINCOLOR_SAPPHIRE, 15, 0,           false}, // SKINCOLOR_SUPERORANGE1
	{"Super Orange 2", {0x00, 0x00, 0x00, 0x00, 0xd0, 0xd0, 0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34}, SKINCOLOR_SAPPHIRE, 12, V_ORANGEMAP, false}, // SKINCOLOR_SUPERORANGE2
	{"Super Orange 3", {0x00, 0x00, 0xd0, 0xd0, 0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x35}, SKINCOLOR_SAPPHIRE, 9,  V_ORANGEMAP, false}, // SKINCOLOR_SUPERORANGE3
	{"Super Orange 4", {0x00, 0xd0, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x44, 0x45, 0x46}, SKINCOLOR_SAPPHIRE, 4,  V_ORANGEMAP, false}, // SKINCOLOR_SUPERORANGE4
	{"Super Orange 5", {0xd0, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x44, 0x45, 0x46, 0x47}, SKINCOLOR_SAPPHIRE, 3,  V_ORANGEMAP, false}, // SKINCOLOR_SUPERORANGE5

	{"Super Gold 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x50, 0x51, 0x52, 0x53, 0x48, 0x48, 0x48}, SKINCOLOR_CORNFLOWER, 15, 0,           false}, // SKINCOLOR_SUPERGOLD1
	{"Super Gold 2", {0x00, 0x50, 0x51, 0x52, 0x53, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41, 0x41, 0x41}, SKINCOLOR_CORNFLOWER, 9,  V_YELLOWMAP, false}, // SKINCOLOR_SUPERGOLD2
	{"Super Gold 3", {0x51, 0x52, 0x53, 0x53, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41, 0x42, 0x43, 0x43, 0x43}, SKINCOLOR_CORNFLOWER, 8,  V_YELLOWMAP, false}, // SKINCOLOR_SUPERGOLD3
	{"Super Gold 4", {0x53, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x46, 0x46}, SKINCOLOR_CORNFLOWER, 8,  V_YELLOWMAP, false}, // SKINCOLOR_SUPERGOLD4
	{"Super Gold 5", {0x48, 0x48, 0x49, 0x49, 0x49, 0x40, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x47, 0x47}, SKINCOLOR_CORNFLOWER, 8,  V_YELLOWMAP, false}, // SKINCOLOR_SUPERGOLD5

	{"Super Peridot 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x58, 0x58, 0xbc, 0xbc, 0xbc}, SKINCOLOR_COBALT, 15, 0,            false}, // SKINCOLOR_SUPERPERIDOT1
	{"Super Peridot 2", {0x00, 0x58, 0x58, 0x58, 0xbc, 0xbc, 0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe}, SKINCOLOR_COBALT, 4,  V_PERIDOTMAP, false}, // SKINCOLOR_SUPERPERIDOT2
	{"Super Peridot 3", {0x58, 0x58, 0xbc, 0xbc, 0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe, 0xbf, 0xbf}, SKINCOLOR_COBALT, 3,  V_PERIDOTMAP, false}, // SKINCOLOR_SUPERPERIDOT3
	{"Super Peridot 4", {0x58, 0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe, 0xbf, 0xbf, 0x5e, 0x5e, 0x5f}, SKINCOLOR_COBALT, 3,  V_PERIDOTMAP, false}, // SKINCOLOR_SUPERPERIDOT4
	{"Super Peridot 5", {0xbc, 0xbc, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbe, 0xbe, 0xbf, 0xbf, 0x5e, 0x5e, 0x5f, 0x77}, SKINCOLOR_COBALT, 3,  V_PERIDOTMAP, false}, // SKINCOLOR_SUPERPERIDOT5

	{"Super Sky 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x81, 0x82, 0x83, 0x84}, SKINCOLOR_RUST, 15, 0,        false}, // SKINCOLOR_SUPERSKY1
	{"Super Sky 2", {0x00, 0x80, 0x81, 0x82, 0x83, 0x83, 0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86}, SKINCOLOR_RUST, 4,  V_SKYMAP, false}, // SKINCOLOR_SUPERSKY2
	{"Super Sky 3", {0x81, 0x82, 0x83, 0x83, 0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87}, SKINCOLOR_RUST, 3,  V_SKYMAP, false}, // SKINCOLOR_SUPERSKY3
	{"Super Sky 4", {0x83, 0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87, 0x88, 0x89, 0x8a}, SKINCOLOR_RUST, 3,  V_SKYMAP, false}, // SKINCOLOR_SUPERSKY4
	{"Super Sky 5", {0x84, 0x84, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87, 0x88, 0x89, 0x8a, 0x8b}, SKINCOLOR_RUST, 3,  V_SKYMAP, false}, // SKINCOLOR_SUPERSKY5

	{"Super Purple 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0xa0, 0xa0, 0xa1, 0xa2}, SKINCOLOR_EMERALD, 15, 0,           false}, // SKINCOLOR_SUPERPURPLE1
	{"Super Purple 2", {0x00, 0x90, 0xa0, 0xa0, 0xa1, 0xa1, 0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5}, SKINCOLOR_EMERALD, 4,  V_PURPLEMAP, false}, // SKINCOLOR_SUPERPURPLE2
	{"Super Purple 3", {0xa0, 0xa0, 0xa1, 0xa1, 0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa6, 0xa6}, SKINCOLOR_EMERALD, 0,  V_PURPLEMAP, false}, // SKINCOLOR_SUPERPURPLE3
	{"Super Purple 4", {0xa1, 0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa6, 0xa6, 0xa7, 0xa8, 0xa9}, SKINCOLOR_EMERALD, 0,  V_PURPLEMAP, false}, // SKINCOLOR_SUPERPURPLE4
	{"Super Purple 5", {0xa2, 0xa2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa6, 0xa6, 0xa7, 0xa8, 0xa9, 0xfd}, SKINCOLOR_EMERALD, 0,  V_PURPLEMAP, false}, // SKINCOLOR_SUPERPURPLE5

	{"Super Rust 1", {0x00, 0xd0, 0xd0, 0xd0, 0x30, 0x30, 0x31, 0x32, 0x33, 0x37, 0x3a, 0x44, 0x45, 0x46, 0x47, 0x2e}, SKINCOLOR_CYAN, 14, V_ORANGEMAP, false}, // SKINCOLOR_SUPERRUST1
	{"Super Rust 2", {0x30, 0x31, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x38, 0x3a, 0x44, 0x45, 0x46, 0x47, 0x47, 0x2e}, SKINCOLOR_CYAN, 10, V_ORANGEMAP, false}, // SKINCOLOR_SUPERRUST2
	{"Super Rust 3", {0x31, 0x32, 0x33, 0x34, 0x36, 0x37, 0x38, 0x3a, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x2e, 0x2e}, SKINCOLOR_CYAN, 9,  V_ORANGEMAP, false}, // SKINCOLOR_SUPERRUST3
	{"Super Rust 4", {0x48, 0x40, 0x41, 0x42, 0x43, 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x47, 0x2e, 0x2e, 0x2e}, SKINCOLOR_CYAN, 8,  V_ORANGEMAP, false}, // SKINCOLOR_SUPERRUST4
	{"Super Rust 5", {0x41, 0x42, 0x43, 0x43, 0x44, 0x44, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xed, 0xee, 0xee, 0xef, 0xef}, SKINCOLOR_CYAN, 8,  V_ORANGEMAP, false}, // SKINCOLOR_SUPERRUST5

	{"Super Tan 1", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x50, 0x51, 0x51, 0x52, 0x52}, SKINCOLOR_BROWN, 14, 0,          false}, // SKINCOLOR_SUPERTAN1
	{"Super Tan 2", {0x00, 0x50, 0x50, 0x51, 0x51, 0x52, 0x52, 0x52, 0x54, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5}, SKINCOLOR_BROWN, 13, V_BROWNMAP, false}, // SKINCOLOR_SUPERTAN2
	{"Super Tan 3", {0x50, 0x51, 0x51, 0x52, 0x52, 0x52, 0x54, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5, 0xf7, 0xf9}, SKINCOLOR_BROWN, 12, V_BROWNMAP, false}, // SKINCOLOR_SUPERTAN3
	{"Super Tan 4", {0x51, 0x52, 0x52, 0x52, 0x52, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5, 0xf7, 0xf9, 0xfb, 0xed}, SKINCOLOR_BROWN, 11, V_BROWNMAP, false}, // SKINCOLOR_SUPERTAN4
	{"Super Tan 5", {0x52, 0x52, 0x54, 0x54, 0x54, 0x55, 0x56, 0x57, 0xf5, 0xf7, 0xf9, 0xfb, 0xed, 0xee, 0xef, 0xef}, SKINCOLOR_BROWN, 10, V_BROWNMAP, false}  // SKINCOLOR_SUPERTAN5
};

/** Patches the mobjinfo, state, and skincolor tables.
  * Free slots are emptied out and set to initial values.
  */
void P_PatchInfoTables(void)
{
	INT32 i;
	char *tempname;

#if NUMSPRITEFREESLOTS > 1000
"Update P_PatchInfoTables, you big dumb head"
#endif

	// empty out free slots
	for (i = SPR_FIRSTFREESLOT; i <= SPR_LASTFREESLOT; i++)
	{
		tempname = sprnames[i];
		tempname[0] = 'F';
		tempname[1] = (char)('0' + (char)((i-SPR_FIRSTFREESLOT+1)/100));
		tempname[2] = (char)('0' + (char)(((i-SPR_FIRSTFREESLOT+1)/10)%10));
		tempname[3] = (char)('0' + (char)((i-SPR_FIRSTFREESLOT+1)%10));
		tempname[4] = '\0';
#ifdef HWRENDER
		t_lspr[i] = &lspr[NOLIGHT];
#endif
	}
	sprnames[i][0] = '\0'; // i == NUMSPRITES
	memset(&states[S_FIRSTFREESLOT], 0, sizeof (state_t) * NUMSTATEFREESLOTS);
	memset(&mobjinfo[MT_FIRSTFREESLOT], 0, sizeof (mobjinfo_t) * NUMMOBJFREESLOTS);
	memset(&skincolors[SKINCOLOR_FIRSTFREESLOT], 0, sizeof (skincolor_t) * NUMCOLORFREESLOTS);
	for (i = SKINCOLOR_FIRSTFREESLOT; i <= SKINCOLOR_LASTFREESLOT; i++) {
		skincolors[i].accessible = false;
		skincolors[i].name[0] = '\0';
	}
	for (i = MT_FIRSTFREESLOT; i <= MT_LASTFREESLOT; i++)
		mobjinfo[i].doomednum = -1;
}

#ifdef ALLOW_RESETDATA
static char *sprnamesbackup;
static state_t *statesbackup;
static mobjinfo_t *mobjinfobackup;
static skincolor_t *skincolorsbackup;
static size_t sprnamesbackupsize, statesbackupsize, mobjinfobackupsize, skincolorsbackupsize;
#endif

void P_BackupTables(void)
{
#ifdef ALLOW_RESETDATA
	// Allocate buffers in size equal to that of the uncompressed data to begin with
	sprnamesbackup = Z_Malloc(sizeof(sprnames), PU_STATIC, NULL);
	statesbackup = Z_Malloc(sizeof(states), PU_STATIC, NULL);
	mobjinfobackup = Z_Malloc(sizeof(mobjinfo), PU_STATIC, NULL);
	skincolorsbackup = Z_Malloc(sizeof(skincolors), PU_STATIC, NULL);

	// Sprite names
	sprnamesbackupsize = lzf_compress(sprnames, sizeof(sprnames), sprnamesbackup, sizeof(sprnames));
	if (sprnamesbackupsize > 0)
		sprnamesbackup = Z_Realloc(sprnamesbackup, sprnamesbackupsize, PU_STATIC, NULL);
	else
		M_Memcpy(sprnamesbackup, sprnames, sizeof(sprnames));

	// States
	statesbackupsize = lzf_compress(states, sizeof(states), statesbackup, sizeof(states));
	if (statesbackupsize > 0)
		statesbackup = Z_Realloc(statesbackup, statesbackupsize, PU_STATIC, NULL);
	else
		M_Memcpy(statesbackup, states, sizeof(states));

	// Mobj info
	mobjinfobackupsize = lzf_compress(mobjinfo, sizeof(mobjinfo), mobjinfobackup, sizeof(mobjinfo));
	if (mobjinfobackupsize > 0)
		mobjinfobackup = Z_Realloc(mobjinfobackup, mobjinfobackupsize, PU_STATIC, NULL);
	else
		M_Memcpy(mobjinfobackup, mobjinfo, sizeof(mobjinfo));

	//Skincolor info
	skincolorsbackupsize = lzf_compress(skincolors, sizeof(skincolors), skincolorsbackup, sizeof(skincolors));
	if (skincolorsbackupsize > 0)
		skincolorsbackup = Z_Realloc(skincolorsbackup, skincolorsbackupsize, PU_STATIC, NULL);
	else
		M_Memcpy(skincolorsbackup, skincolors, sizeof(skincolors));
#endif
}

void P_ResetData(INT32 flags)
{
#ifndef ALLOW_RESETDATA
	(void)flags;
	CONS_Alert(CONS_NOTICE, M_GetText("P_ResetData(): not supported in this build.\n"));
#else
	if (flags & 1)
	{
		if (sprnamesbackupsize > 0)
			lzf_decompress(sprnamesbackup, sprnamesbackupsize, sprnames, sizeof(sprnames));
		else
			M_Memcpy(sprnames, sprnamesbackup, sizeof(sprnamesbackup));
	}

	if (flags & 2)
	{
		if (statesbackupsize > 0)
			lzf_decompress(statesbackup, statesbackupsize, states, sizeof(states));
		else
			M_Memcpy(states, statesbackup, sizeof(statesbackup));
	}

	if (flags & 4)
	{
		if (mobjinfobackupsize > 0)
			lzf_decompress(mobjinfobackup, mobjinfobackupsize, mobjinfo, sizeof(mobjinfo));
		else
			M_Memcpy(mobjinfo, mobjinfobackup, sizeof(mobjinfobackup));
	}

	if (flags & 8)
	{
		if (skincolorsbackupsize > 0)
			lzf_decompress(skincolorsbackup, skincolorsbackupsize, skincolors, sizeof(skincolors));
		else
			M_Memcpy(skincolors, skincolorsbackup, sizeof(skincolorsbackup));
	}
#endif
}
