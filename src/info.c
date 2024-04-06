// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
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
#include "deh_tables.h"
#ifdef HWRENDER
#include "hardware/hw_light.h"
#endif


// Hey, moron! If you change this table, don't forget about the sprite enum in info.h and the sprite lights in hw_light.c!
// For the sake of constant merge conflicts, let's spread this out
char sprnames[NUMSPRITES + 1][MAXSPRITENAME + 1] =
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
	"TFLM", // Spin fire (team)
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

	// Tutorial Scenery
	"TUPL",
	"TUPF",

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
	"BSPB", // Blue spring on a ball
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

char spr2names[NUMPLAYERSPRITES][MAXSPRITENAME + 1] =
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
	"TALC",

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

	SPR2_STND, // SPR2_NSTD,
	SPR2_FALL, // SPR2_NFLT,
	0, // SPR2_NFLY, (will never be referenced unless skin 0 lacks this)
	SPR2_NFLY, // SPR2_NDRL,
	SPR2_STUN, // SPR2_NSTN,
	SPR2_NSTN, // SPR2_NPUL,
	SPR2_ROLL, // SPR2_NATK,

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
	SPR2_TAL6, // SPR2_TALC,

	SPR2_WAIT, // SPR2_CNT1,
	SPR2_FALL, // SPR2_CNT2,
	SPR2_SPNG, // SPR2_CNT3,
	SPR2_CNT1, // SPR2_CNT4,

	0, // SPR2_SIGN,
	0, // SPR2_LIFE,

	0, // SPR2_XTRA (should never be referenced)
};

state_t states[NUMSTATES];

statenum_t S_ALART1;
statenum_t S_BLACKEGG_DESTROYPLAT1;
statenum_t S_BLACKEGG_DIE4;
statenum_t S_BLACKEGG_GOOP;
statenum_t S_BLACKEGG_HITFACE4;
statenum_t S_BLACKEGG_JUMP1;
statenum_t S_BLACKEGG_JUMP2;
statenum_t S_BLACKEGG_PAIN1;
statenum_t S_BLACKEGG_PAIN35;
statenum_t S_BLACKEGG_SHOOT1;
statenum_t S_BLACKEGG_SHOOT2;
statenum_t S_BLACKEGG_STND;
statenum_t S_BLACKEGG_WALK1;
statenum_t S_BLACKEGG_WALK6;
statenum_t S_BLUESPHEREBONUS;
statenum_t S_BOSSEGLZ1;
statenum_t S_BOSSEGLZ2;
statenum_t S_BOSSSEBH1;
statenum_t S_BOSSSEBH2;
statenum_t S_BOSSSPIGOT;
statenum_t S_BOSSTANK1;
statenum_t S_BOSSTANK2;
statenum_t S_BUMBLEBORE_FALL2;
statenum_t S_BUMBLEBORE_FLY1;
statenum_t S_BUMBLEBORE_FLY2;
statenum_t S_BUMBLEBORE_RAISE;
statenum_t S_BUMBLEBORE_STUCK2;
statenum_t S_CEMG1;
statenum_t S_CEMG2;
statenum_t S_CEMG3;
statenum_t S_CEMG4;
statenum_t S_CEMG5;
statenum_t S_CEMG6;
statenum_t S_CEMG7;
statenum_t S_CLEARSIGN;
statenum_t S_CRUSHCLAW_STAY;
statenum_t S_CYBRAKDEMON_DIE8;
statenum_t S_EGGMANSIGN;
statenum_t S_EXTRALARGEBUBBLE;
statenum_t S_FANG_BOUNCE3;
statenum_t S_FANG_BOUNCE4;
statenum_t S_FANG_INTRO12;
statenum_t S_FANG_PAIN1;
statenum_t S_FANG_PAIN2;
statenum_t S_FANG_PATHINGCONT1;
statenum_t S_FANG_PATHINGCONT2;
statenum_t S_FANG_PINCHBOUNCE3;
statenum_t S_FANG_PINCHBOUNCE4;
statenum_t S_FANG_PINCHPATHINGSTART2;
statenum_t S_FANG_WALLHIT;
statenum_t S_FLAMEJETFLAME4;
statenum_t S_FLAMEJETFLAME7;
statenum_t S_FLIGHTINDICATOR;
statenum_t S_HANGSTER_ARC1;
statenum_t S_HANGSTER_ARC2;
statenum_t S_HANGSTER_ARC3;
statenum_t S_HANGSTER_ARCUP1;
statenum_t S_HANGSTER_FLY1;
statenum_t S_HANGSTER_RETURN1;
statenum_t S_HANGSTER_RETURN2;
statenum_t S_HANGSTER_RETURN3;
statenum_t S_HANGSTER_SWOOP1;
statenum_t S_HANGSTER_SWOOP2;
statenum_t S_INVISIBLE;
statenum_t S_JETFUME1;
statenum_t S_JETFUMEFLASH;
statenum_t S_LAVAFALL_DORMANT;
statenum_t S_LAVAFALL_SHOOT;
statenum_t S_LAVAFALL_TELL;
statenum_t S_METALSONIC_BOUNCE;
statenum_t S_MINECARTSEG_FRONT;
statenum_t S_MSSHIELD_F2;
statenum_t S_NIGHTSDRONE_SPARKLING1;
statenum_t S_NIGHTSDRONE_SPARKLING16;
statenum_t S_NULL;
statenum_t S_OBJPLACE_DUMMY;
statenum_t S_OILLAMPFLARE;
statenum_t S_PLAY_BOUNCE;
statenum_t S_PLAY_BOUNCE_LANDING;
statenum_t S_PLAY_CLIMB;
statenum_t S_PLAY_CLING;
statenum_t S_PLAY_DASH;
statenum_t S_PLAY_DEAD;
statenum_t S_PLAY_DRWN;
statenum_t S_PLAY_EDGE;
statenum_t S_PLAY_FALL;
statenum_t S_PLAY_FIRE;
statenum_t S_PLAY_FIRE_FINISH;
statenum_t S_PLAY_FLOAT;
statenum_t S_PLAY_FLOAT_RUN;
statenum_t S_PLAY_FLY;
statenum_t S_PLAY_FLY_TIRED;
statenum_t S_PLAY_GASP;
statenum_t S_PLAY_GLIDE;
statenum_t S_PLAY_GLIDE_LANDING;
statenum_t S_PLAY_JUMP;
statenum_t S_PLAY_MELEE;
statenum_t S_PLAY_MELEE_FINISH;
statenum_t S_PLAY_MELEE_LANDING;
statenum_t S_PLAY_NIGHTS_ATTACK;
statenum_t S_PLAY_NIGHTS_DRILL;
statenum_t S_PLAY_NIGHTS_FLOAT;
statenum_t S_PLAY_NIGHTS_FLY;
statenum_t S_PLAY_NIGHTS_PULL;
statenum_t S_PLAY_NIGHTS_STAND;
statenum_t S_PLAY_NIGHTS_STUN;
statenum_t S_PLAY_NIGHTS_TRANS1;
statenum_t S_PLAY_NIGHTS_TRANS6;
statenum_t S_PLAY_PAIN;
statenum_t S_PLAY_RIDE;
statenum_t S_PLAY_ROLL;
statenum_t S_PLAY_RUN;
statenum_t S_PLAY_SIGN;
statenum_t S_PLAY_SKID;
statenum_t S_PLAY_SPINDASH;
statenum_t S_PLAY_SPRING;
statenum_t S_PLAY_STND;
statenum_t S_PLAY_STUN;
statenum_t S_PLAY_SUPER_TRANS1;
statenum_t S_PLAY_SUPER_TRANS6;
statenum_t S_PLAY_SWIM;
statenum_t S_PLAY_TWINSPIN;
statenum_t S_PLAY_WAIT;
statenum_t S_PLAY_WALK;
statenum_t S_PTERABYTE_FLY1;
statenum_t S_PTERABYTE_SWOOPDOWN;
statenum_t S_PTERABYTE_SWOOPUP;
statenum_t S_RAIN1;
statenum_t S_RAINRETURN;
statenum_t S_REDBOOSTERROLLER;
statenum_t S_REDBOOSTERSEG_FACE;
statenum_t S_REDBOOSTERSEG_LEFT;
statenum_t S_REDBOOSTERSEG_RIGHT;
statenum_t S_ROSY_HUG;
statenum_t S_ROSY_IDLE1;
statenum_t S_ROSY_IDLE2;
statenum_t S_ROSY_IDLE3;
statenum_t S_ROSY_IDLE4;
statenum_t S_ROSY_JUMP;
statenum_t S_ROSY_PAIN;
statenum_t S_ROSY_STND;
statenum_t S_ROSY_UNHAPPY;
statenum_t S_ROSY_WALK;
statenum_t S_SIGNBOARD;
statenum_t S_SIGNSPIN1;
statenum_t S_SMASHSPIKE_FALL;
statenum_t S_SMASHSPIKE_FLOAT;
statenum_t S_SMASHSPIKE_RISE2;
statenum_t S_SMASHSPIKE_STOMP1;
statenum_t S_SNOW2;
statenum_t S_SNOW3;
statenum_t S_SPINCUSHION_AIM1;
statenum_t S_SPINCUSHION_AIM5;
statenum_t S_SPINDUST_BUBBLE1;
statenum_t S_SPINDUST_BUBBLE4;
statenum_t S_SPINDUST_FIRE1;
statenum_t S_SPLASH1;
statenum_t S_STEAM1;
statenum_t S_TAILSOVERLAY_0DEGREES;
statenum_t S_TAILSOVERLAY_DASH;
statenum_t S_TAILSOVERLAY_EDGE;
statenum_t S_TAILSOVERLAY_FLY;
statenum_t S_TAILSOVERLAY_GASP;
statenum_t S_TAILSOVERLAY_MINUS30DEGREES;
statenum_t S_TAILSOVERLAY_MINUS60DEGREES;
statenum_t S_TAILSOVERLAY_PAIN;
statenum_t S_TAILSOVERLAY_PLUS30DEGREES;
statenum_t S_TAILSOVERLAY_PLUS60DEGREES;
statenum_t S_TAILSOVERLAY_RUN;
statenum_t S_TAILSOVERLAY_STAND;
statenum_t S_TAILSOVERLAY_TIRE;
statenum_t S_TEAM_SPINFIRE1;
statenum_t S_TRAINDUST;
statenum_t S_TRAINPUFFMAKER;
statenum_t S_TRAINSTEAM;
statenum_t S_TUTORIALFLOWER1;
statenum_t S_TUTORIALFLOWERF1;
statenum_t S_TUTORIALLEAF1;
statenum_t S_UNKNOWN;
statenum_t S_XPLD_EGGTRAP;
statenum_t S_XPLD1;
statenum_t S_YELLOWBOOSTERROLLER;
statenum_t S_YELLOWBOOSTERSEG_FACE;
statenum_t S_YELLOWBOOSTERSEG_LEFT;
statenum_t S_YELLOWBOOSTERSEG_RIGHT;

mobjinfo_t mobjinfo[NUMMOBJTYPES];

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
	{"Aether",    {0x00, 0x00, 0x01, 0x01, 0x90, 0x90, 0x91, 0x91, 0x92, 0xaa, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xae}, SKINCOLOR_GREY,      15, 0,           true}, // SKINCOLOR_AETHER
	{"Slate",     {0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0xaa, 0xaa, 0xaa, 0xab, 0xac, 0xac, 0xad, 0xad, 0xae, 0xaf}, SKINCOLOR_SILVER,    12, 0,           true}, // SKINCOLOR_SLATE
	{"Moonstone", {   0,    4,    8,    9,   11,   12,   14,   15,  171,  172,  173,  174,  175,   27,   29,   31}, SKINCOLOR_TOPAZ,     15, V_GRAYMAP,   true}, // SKINCOLOR_MOONSTONE
	{"Bluebell",  {0x90, 0x91, 0x92, 0x93, 0x94, 0x94, 0x95, 0xac, 0xac, 0xad, 0xad, 0xa8, 0xa8, 0xa9, 0xfd, 0xfe}, SKINCOLOR_COPPER,    4,  V_BLUEMAP,   true}, // SKINCOLOR_BLUEBELL
	{"Pink",      {0xd0, 0xd0, 0xd1, 0xd1, 0xd2, 0xd2, 0xd3, 0xd3, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0x2b, 0x2c, 0x2e}, SKINCOLOR_AZURE,     9,  V_REDMAP,    true}, // SKINCOLOR_PINK
	{"Rosewood",  { 209,  210,  211,  212,  213,  214,  228,  230,  232,  234,  235,  237,   26,   27,   28,   29}, SKINCOLOR_SEPIA,     5,  V_BROWNMAP,  true}, // SKINCOLOR_ROSEWOOD
	{"Yogurt",    {0xd0, 0x30, 0xd8, 0xd9, 0xda, 0xdb, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe3, 0xe6, 0xe8, 0xe9}, SKINCOLOR_RUST,      7,  V_BROWNMAP,  true}, // SKINCOLOR_YOGURT
	{"Latte",     {  48,  217,  219,  221,  223,  224,  226,  228,   68,   69,   70,   70,   44,   45,   46,   47}, SKINCOLOR_BOTTLE,    12, V_BROWNMAP,  true}, // SKINCOLOR_LATTE
	{"Brown",     {0xdf, 0xe0, 0xe1, 0xe2, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef}, SKINCOLOR_TAN,       2,  V_BROWNMAP,  true}, // SKINCOLOR_BROWN
	{"Boulder",   {0xde, 0xe0, 0xe1, 0xe4, 0xe7, 0xe9, 0xeb, 0xec, 0xed, 0xed, 0xed, 0x19, 0x19, 0x1b, 0x1d, 0x1e}, SKINCOLOR_KETCHUP,   0,  V_BROWNMAP,  true}, // SKINCOLOR_BOULDER
	{"Bronze",    {  82,   84,   50,   51,  223,  228,  230,  232,  234,  236,  237,  238,  239,  239,   30,   31}, SKINCOLOR_VOLCANIC,  9,  V_BROWNMAP,  true}, // SKINCOLOR_BRONZE
	{"Sepia",     {  88,   84,   85,   86,  224,  226,  228,  230,  232,  235,  236,  237,  238,  239,   28,   28}, SKINCOLOR_ROSEWOOD,  5,  V_BROWNMAP,  true}, // SKINCOLOR_SEPIA
	{"Ecru",      {  80,   83,   84,   85,   86,  242,  243,  245,  230,  232,  234,  236,  238,  239,   47,   47}, SKINCOLOR_ARCTIC,    12, V_BROWNMAP,  true}, // SKINCOLOR_ECRU
	{"Tan",       {0x51, 0x51, 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x56, 0x57, 0xf5, 0xf5, 0xf9, 0xf9, 0xed, 0xed}, SKINCOLOR_BROWN,     12, V_BROWNMAP,  true}, // SKINCOLOR_TAN
	{"Beige",     {0x54, 0x55, 0x56, 0x56, 0xf2, 0xf3, 0xf3, 0xf4, 0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfb, 0xed, 0xed}, SKINCOLOR_MOSS,      5,  V_BROWNMAP,  true}, // SKINCOLOR_BEIGE
	{"Rosebush",  { 208,  216,  209,   85,   90,   91,   91,   92,  191,   93,   94,  107,  109,  110,  111,  111}, SKINCOLOR_EGGPLANT,  5,  V_GREENMAP,  true}, // SKINCOLOR_ROSEBUSH
	{"Moss",      {0x58, 0x58, 0x59, 0x59, 0x5a, 0x5a, 0x5b, 0x5b, 0x5b, 0x5c, 0x5d, 0x5d, 0x5e, 0x5e, 0x5f, 0x5f}, SKINCOLOR_BEIGE,     13, V_GREENMAP,  true}, // SKINCOLOR_MOSS
	{"Azure",     {0x90, 0x90, 0x91, 0x91, 0xaa, 0xaa, 0xab, 0xab, 0xab, 0xac, 0xad, 0xad, 0xae, 0xae, 0xaf, 0xaf}, SKINCOLOR_PINK,      5,  V_AZUREMAP,  true}, // SKINCOLOR_AZURE
	{"Eggplant",  {   4,   8,    11,   11,   16,  195,  195,  195,  196,  186,  187,  187,  254,  254,   30,   31}, SKINCOLOR_ROSEBUSH,  5,  V_PURPLEMAP, true}, // SKINCOLOR_EGGPLANT
	{"Lavender",  {0xc0, 0xc0, 0xc1, 0xc1, 0xc2, 0xc2, 0xc3, 0xc3, 0xc3, 0xc4, 0xc5, 0xc5, 0xc6, 0xc6, 0xc7, 0xc7}, SKINCOLOR_GOLD,      4,  V_PURPLEMAP, true}, // SKINCOLOR_LAVENDER

	// Viv's vivid colours (toast 21/07/17)
	// Tweaks & additions (Lach, Chrispy, sphere, Alice, MotorRoach & Saneko 26/10/22)
	{"Ruby",       {0xb0, 0xb0, 0xc9, 0xca, 0xcc, 0x26, 0x27, 0x28, 0x29, 0x2a, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xfd}, SKINCOLOR_EMERALD,    10, V_REDMAP,     true}, // SKINCOLOR_RUBY
	{"Cherry",     { 202,  203,  204,  205,  206,   40,   41,   42,   43,   44,  186,  187,   28,   29,   30,   31}, SKINCOLOR_MIDNIGHT,   10, V_REDMAP,     true}, // SKINCOLOR_CHERRY
	{"Salmon",     {0xd0, 0xd0, 0xd1, 0xd2, 0x20, 0x21, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e}, SKINCOLOR_FOREST,     6,  V_REDMAP,     true}, // SKINCOLOR_SALMON
	{"Pepper",     { 210,   32,   33,   34,   35,   35,   36,   37,   38,   39,   41,   43,   45,   45,   46,   47}, SKINCOLOR_MASTER,     8,  V_REDMAP,     true}, // SKINCOLOR_PEPPER
	{"Red",        {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x47, 0x2e, 0x2f}, SKINCOLOR_GREEN,      10, V_REDMAP,     true}, // SKINCOLOR_RED
	{"Crimson",    {0x27, 0x27, 0x28, 0x28, 0x29, 0x2a, 0x2b, 0x2b, 0x2c, 0x2d, 0x2e, 0x2e, 0x2e, 0x2f, 0x2f, 0x1f}, SKINCOLOR_ICY,        10, V_REDMAP,     true}, // SKINCOLOR_CRIMSON
	{"Flame",      {0x31, 0x32, 0x33, 0x36, 0x22, 0x22, 0x25, 0x25, 0x25, 0xcd, 0xcf, 0xcf, 0xc5, 0xc5, 0xc7, 0xc7}, SKINCOLOR_PURPLE,     8,  V_REDMAP,     true}, // SKINCOLOR_FLAME
	{"Garnet",     {   0,   83,   50,   53,   34,   35,   37,   38,   39,   40,   42,   44,   45,   46,   47,   47}, SKINCOLOR_AQUAMARINE, 6,  V_REDMAP,     true}, // SKINCOLOR_GARNET
	{"Ketchup",    {0x48, 0x49, 0x40, 0x33, 0x34, 0x36, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2b, 0x2c, 0x47, 0x2e, 0x2f}, SKINCOLOR_BOULDER,    8,  V_REDMAP,     true}, // SKINCOLOR_KETCHUP
	{"Peachy",     {0xd0, 0x30, 0x31, 0x31, 0x32, 0x32, 0xdc, 0xdc, 0xdc, 0xd3, 0xd4, 0xd4, 0xcc, 0xcd, 0xce, 0xcf}, SKINCOLOR_TEAL,       7,  V_ROSYMAP,    true}, // SKINCOLOR_PEACHY
	{"Quail",      {0xd8, 0xd9, 0xdb, 0xdc, 0xde, 0xdf, 0xd5, 0xd5, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0x1d, 0x1f}, SKINCOLOR_WAVE,       5,  V_BROWNMAP,   true}, // SKINCOLOR_QUAIL
	{"Foundation", {  80,   81,   82,   84,  219,  221,  221,  212,  213,  214,  215,  197,  186,  187,  187,   30}, SKINCOLOR_DREAM,      6,  V_ORANGEMAP,  true}, // SKINCOLOR_FOUNDATION
	{"Sunset",     {0x51, 0x52, 0x40, 0x40, 0x34, 0x36, 0xd5, 0xd5, 0xd6, 0xd7, 0xcf, 0xcf, 0xc6, 0xc6, 0xc7, 0xfe}, SKINCOLOR_SAPPHIRE,   5,  V_ORANGEMAP,  true}, // SKINCOLOR_SUNSET
	{"Copper",     {0x58, 0x54, 0x40, 0x34, 0x35, 0x38, 0x3a, 0x3c, 0x3d, 0x2a, 0x2b, 0x2c, 0x2c, 0xba, 0xba, 0xbb}, SKINCOLOR_BLUEBELL,   5,  V_ORANGEMAP,  true}, // SKINCOLOR_COPPER
	{"Apricot",    {0x00, 0xd8, 0xd9, 0xda, 0xdb, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e}, SKINCOLOR_CYAN,       4,  V_ORANGEMAP,  true}, // SKINCOLOR_APRICOT
	{"Orange",     {  49,   50,   51,   52,   53,   54,   55,   57,   58,   59,   60,   42,   44,   45,   46,   46}, SKINCOLOR_BLUE,       4,  V_ORANGEMAP,  true}, // SKINCOLOR_ORANGE
	{"Rust",       {0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3c, 0x3d, 0x3d, 0x3d, 0x3f, 0x2c, 0x2d, 0x47, 0x2e, 0x2f, 0x2f}, SKINCOLOR_YOGURT,     8,  V_ORANGEMAP,  true}, // SKINCOLOR_RUST
	{"Tangerine",  {  81,   83,   64,   64,   51,   52,   53,   54,   56,   58,   60,   61,   63,   45,   46,   47}, SKINCOLOR_OCEAN,      12, V_ORANGEMAP,  true}, // SKINCOLOR_TANGERINE
	{"Topaz",      {   0,   81,   83,   73,   74,   74,   65,   52,   53,   54,   56,   58,   60,   42,   43,   45}, SKINCOLOR_MOONSTONE,  10, V_YELLOWMAP,  true}, // SKINCOLOR_TOPAZ
	{"Gold",       {0x51, 0x51, 0x54, 0x54, 0x41, 0x42, 0x43, 0x43, 0x44, 0x45, 0x46, 0x3f, 0x2d, 0x2e, 0x2f, 0x2f}, SKINCOLOR_LAVENDER,   10, V_YELLOWMAP,  true}, // SKINCOLOR_GOLD
	{"Sandy",      {0x53, 0x40, 0x41, 0x42, 0x43, 0xe6, 0xe9, 0xe9, 0xea, 0xec, 0xec, 0xc6, 0xc6, 0xc7, 0xc7, 0xfe}, SKINCOLOR_SKY,        8,  V_YELLOWMAP,  true}, // SKINCOLOR_SANDY
	{"Goldenrod",  {   0,   80,   81,   81,   83,   73,   73,   64,   65,   66,   67,   68,   69,   62,   44,   45}, SKINCOLOR_MAJESTY,    8,  V_YELLOWMAP,  true}, // SKINCOLOR_GOLDENROD
	{"Yellow",     {0x52, 0x53, 0x49, 0x49, 0x4a, 0x4a, 0x4b, 0x4b, 0x4b, 0x4c, 0x4d, 0x4d, 0x4e, 0x4e, 0x4f, 0xed}, SKINCOLOR_CORNFLOWER, 8,  V_YELLOWMAP,  true}, // SKINCOLOR_YELLOW
	{"Olive",      {0x4b, 0x4b, 0x4c, 0x4c, 0x4d, 0x4e, 0xe7, 0xe7, 0xe9, 0xc5, 0xc5, 0xc6, 0xc6, 0xc7, 0xc7, 0xfd}, SKINCOLOR_DUSK,       3,  V_YELLOWMAP,  true}, // SKINCOLOR_OLIVE
	{"Pear",       {  88,   89,  188,  189,  189,   76,   76,   67,   67,   68,   69,   70,   45,   46,   47,   47}, SKINCOLOR_MARINE,     9,  V_PERIDOTMAP, true}, // SKINCOLOR_PEAR
	{"Lemon",      {   0,   80,   81,   83,   73,   73,   74,   74,   76,   76,  191,  191,   79,   79,  110,  111}, SKINCOLOR_FUCHSIA,    8,  V_YELLOWMAP,  true}, // SKINCOLOR_LEMON
	{"Lime",       {0x50, 0x51, 0x52, 0x53, 0x48, 0xbc, 0xbd, 0xbe, 0xbe, 0xbf, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f}, SKINCOLOR_MAGENTA,    9,  V_PERIDOTMAP, true}, // SKINCOLOR_LIME
	{"Peridot",    {0x58, 0x58, 0xbc, 0xbc, 0xbd, 0xbd, 0xbe, 0xbe, 0xbe, 0xbf, 0x5e, 0x5e, 0x5f, 0x5f, 0x77, 0x77}, SKINCOLOR_COBALT,     2,  V_PERIDOTMAP, true}, // SKINCOLOR_PERIDOT
	{"Apple",      {0x49, 0x49, 0xbc, 0xbd, 0xbe, 0xbe, 0xbe, 0x67, 0x69, 0x6a, 0x6b, 0x6b, 0x6c, 0x6d, 0x6d, 0x6d}, SKINCOLOR_RASPBERRY,  13, V_PERIDOTMAP, true}, // SKINCOLOR_APPLE
	{"Headlight",  {   0,   80,   81,   82,   73,   84,   64,   65,   91,   91,  124,  125,  126,  137,  138,  139}, SKINCOLOR_MAUVE,      8,  V_YELLOWMAP,  true}, // SKINCOLOR_HEADLIGHT
	{"Chartreuse", {  80,   82,   72,   73,  188,  188,  113,  114,  114,  125,  126,  137,  138,  139,  253,  254}, SKINCOLOR_NOBLE,      9,  V_PERIDOTMAP, true}, // SKINCOLOR_CHARTREUSE
	{"Green",      {0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f}, SKINCOLOR_RED,        6,  V_GREENMAP,   true}, // SKINCOLOR_GREEN
	{"Forest",     {0x65, 0x66, 0x67, 0x68, 0x69, 0x69, 0x6a, 0x6b, 0x6b, 0x6c, 0x6d, 0x6d, 0x6e, 0x6e, 0x6e, 0x6f}, SKINCOLOR_SALMON,     9,  V_GREENMAP,   true}, // SKINCOLOR_FOREST
	{"Shamrock",   {0x70, 0x70, 0x71, 0x71, 0x72, 0x72, 0x73, 0x73, 0x73, 0x74, 0x75, 0x75, 0x76, 0x76, 0x77, 0x77}, SKINCOLOR_SIBERITE,   10, V_GREENMAP,   true}, // SKINCOLOR_SHAMROCK
	{"Jade",       { 128,  120,  121,  122,  122,  113,  114,  114,  115,  116,  117,  118,  119,  110,  111,   30}, SKINCOLOR_TAFFY,      10, V_GREENMAP,   true}, // SKINCOLOR_JADE
	{"Mint",       {0x00, 0x00, 0x58, 0x58, 0x59, 0x62, 0x62, 0x62, 0x64, 0x67, 0x7e, 0x7e, 0x8f, 0x8f, 0x8a, 0x8a}, SKINCOLOR_VIOLET,     5,  V_GREENMAP,   true}, // SKINCOLOR_MINT
	{"Master",     {   0,   80,   88,   96,  112,  113,   99,  100,  124,  125,  126,  117,  107,  118,  119,  111}, SKINCOLOR_PEPPER,     8,  V_GREENMAP,   true}, // SKINCOLOR_MASTER
	{"Emerald",    {  80,   96,  112,  113,  114,  114,  125,  125,  126,  126,  137,  137,  138,  138,  139,  139}, SKINCOLOR_RUBY,       9,  V_GREENMAP,   true}, // SKINCOLOR_EMERALD
	{"Seafoam",    {0x01, 0x58, 0x59, 0x5a, 0x7c, 0x7d, 0x7d, 0x7e, 0x7e, 0x8f, 0x8f, 0x8a, 0x8a, 0x8b, 0xfd, 0xfd}, SKINCOLOR_PLUM,       6,  V_AQUAMAP,    true}, // SKINCOLOR_SEAFOAM
	{"Island",     {  96,   97,  113,  113,  114,  124,  142,  136,  136,  150,  151,  153,  168,  168,  169,  169}, SKINCOLOR_GALAXY,     7,  V_AQUAMAP,    true}, // SKINCOLOR_ISLAND
	{"Bottle",     {   0,    1,    3,    4,    5,  140,  141,  141,  124,  125,  126,  127,  118,  119,  111,  111}, SKINCOLOR_LATTE,      14, V_AQUAMAP,    true}, // SKINCOLOR_BOTTLE
	{"Aqua",       {0x78, 0x79, 0x7a, 0x7a, 0x7b, 0x7b, 0x7c, 0x7c, 0x7c, 0x7d, 0x7e, 0x7e, 0x7f, 0x7f, 0x76, 0x77}, SKINCOLOR_ROSY,       7,  V_AQUAMAP,    true}, // SKINCOLOR_AQUA
	{"Teal",       {0x78, 0x78, 0x8c, 0x8c, 0x8d, 0x8d, 0x8d, 0x8e, 0x8e, 0x8f, 0x8f, 0x8f, 0x8a, 0x8a, 0x8a, 0x8a}, SKINCOLOR_PEACHY,     7,  V_SKYMAP,     true}, // SKINCOLOR_TEAL
	{"Ocean",      { 120,  121,  122,  122,  123,  141,  142,  142,  136,  137,  138,  138,  139,  139,  253,  253}, SKINCOLOR_TANGERINE,  4,  V_AQUAMAP,    true}, // SKINCOLOR_OCEAN
	{"Wave",       {0x00, 0x78, 0x78, 0x79, 0x8d, 0x87, 0x88, 0x89, 0x89, 0xae, 0xa8, 0xa8, 0xa9, 0xa9, 0xfd, 0xfd}, SKINCOLOR_QUAIL,      5,  V_SKYMAP,     true}, // SKINCOLOR_WAVE
	{"Cyan",       {0x80, 0x81, 0xff, 0xff, 0x83, 0x83, 0x8d, 0x8d, 0x8d, 0x8e, 0x7e, 0x7f, 0x76, 0x76, 0x77, 0x6e}, SKINCOLOR_APRICOT,    6,  V_SKYMAP,     true}, // SKINCOLOR_CYAN
	{"Turquoise",  {  0,   120,  121,  122,  123,  141,  141,  135,  136,  136,  150,  153,  155,  157,  159,  253}, SKINCOLOR_SANGRIA,    12, V_SKYMAP,     true}, // SKINCOLOR_TURQUOISE
	{"Aquamarine", {   0,  120,  121,  131,  132,  133,  134,  134,  135,  135,  149,  149,  172,  173,  174,  175}, SKINCOLOR_GARNET,     8,  V_SKYMAP,     true}, // SKINCOLOR_AQUAMARINE
	{"Sky",        {0x80, 0x80, 0x81, 0x82, 0x83, 0x83, 0x84, 0x85, 0x85, 0x86, 0x87, 0x88, 0x89, 0x89, 0x8a, 0x8b}, SKINCOLOR_SANDY,      1,  V_SKYMAP,     true}, // SKINCOLOR_SKY
	{"Marine",     { 144,  146,  147,  147,  148,  135,  136,  136,  137,  137,  127,  118,  119,  111,  111,  111}, SKINCOLOR_PEAR,       13, V_SKYMAP,     true}, // SKINCOLOR_MARINE
	{"Cerulean",   {0x85, 0x86, 0x87, 0x88, 0x88, 0x89, 0x89, 0x89, 0x8a, 0x8a, 0xfd, 0xfd, 0xfd, 0x1f, 0x1f, 0x1f}, SKINCOLOR_NEON,       4,  V_SKYMAP,     true}, // SKINCOLOR_CERULEAN
	{"Dream",      {  80,  208,  200,  200,  146,  146,  133,  134,  135,  136,  137,  138,  139,  139,  254,  254}, SKINCOLOR_FOUNDATION, 9,  V_SKYMAP,     true}, // SKINCOLOR_DREAM
	{"Icy",        {0x00, 0x00, 0x00, 0x00, 0x80, 0x81, 0x83, 0x83, 0x86, 0x87, 0x95, 0x95, 0xad, 0xad, 0xae, 0xaf}, SKINCOLOR_CRIMSON,    0,  V_SKYMAP,     true}, // SKINCOLOR_ICY
	{"Daybreak",   {  80,   81,   82,   72,   64,    9,   11,  171,  149,  150,  151,  153,  156,  157,  159,  253}, SKINCOLOR_EVENTIDE,   12, V_BLUEMAP,    true}, // SKINCOLOR_DAYBREAK
	{"Sapphire",   {0x80, 0x82, 0x86, 0x87, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xfd, 0xfe}, SKINCOLOR_SUNSET,     5,  V_BLUEMAP,    true}, // SKINCOLOR_SAPPHIRE
	{"Arctic",     {   0,    1,    3,    4,  145,  146,  147,  148,  148,  149,  150,  153,  156,  159,  253,  254}, SKINCOLOR_ECRU,       15, V_BLUEMAP,    true}, // SKINCOLOR_ARCTIC
	{"Cornflower", {0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x9a, 0x9c, 0x9d, 0x9d, 0x9e, 0x9e, 0x9e}, SKINCOLOR_YELLOW,     4,  V_BLUEMAP,    true}, // SKINCOLOR_CORNFLOWER
	{"Blue",       {0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xfd, 0xfe}, SKINCOLOR_ORANGE,     5,  V_BLUEMAP,    true}, // SKINCOLOR_BLUE
	{"Cobalt",     { 145,  147,  149,  150,  151,  153,  154,  155,  156,  157,  158,  159,  253,  253,  254,  254}, SKINCOLOR_PERIDOT,    5,  V_BLUEMAP,    true}, // SKINCOLOR_COBALT
	{"Midnight",   { 171,  171,  172,  173,  173,  174,  175,  157,  158,  159,  253,  253,  254,  254,   31,   31}, SKINCOLOR_CHERRY,     10, V_GRAYMAP,    true}, // SKINCOLOR_MIDNIGHT
	{"Galaxy",     { 160,  161,  162,  163,  164,  165,  166,  166,  154,  155,  156,  157,  159,  253,  254,   31}, SKINCOLOR_ISLAND,     7,  V_PURPLEMAP,  true}, // SKINCOLOR_GALAXY
	{"Vapor",      {0x80, 0x81, 0x83, 0x86, 0x94, 0x94, 0xa3, 0xa3, 0xa4, 0xa6, 0xa6, 0xa6, 0xa8, 0xa8, 0xa9, 0xa9}, SKINCOLOR_LILAC,      4,  V_SKYMAP,     true}, // SKINCOLOR_VAPOR
	{"Dusk",       {0x92, 0x93, 0x94, 0x94, 0xac, 0xad, 0xad, 0xad, 0xae, 0xae, 0xaf, 0xaf, 0xa9, 0xa9, 0xfd, 0xfd}, SKINCOLOR_OLIVE,      0,  V_BLUEMAP,    true}, // SKINCOLOR_DUSK
	{"Majesty",    {   0,    1,  176,  160,  160,  161,  162,  162,  163,  172,  173,  174,  174,  175,  139,  139}, SKINCOLOR_GOLDENROD,  9,  V_PURPLEMAP,  true}, // SKINCOLOR_MAJESTY
	{"Pastel",     {0x90, 0x90, 0xa0, 0xa0, 0xa1, 0xa1, 0xa2, 0xa2, 0xa2, 0xa3, 0xa4, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8}, SKINCOLOR_BUBBLEGUM,  9,  V_PURPLEMAP,  true}, // SKINCOLOR_PASTEL
	{"Purple",     {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa4, 0xa5, 0xa5, 0xa5, 0xa6, 0xa7, 0xa7, 0xa8, 0xa8, 0xa9, 0xa9}, SKINCOLOR_FLAME,      7,  V_PURPLEMAP,  true}, // SKINCOLOR_PURPLE
	{"Noble",      { 144,  146,  147,  148,  149,  164,  164,  165,  166,  185,  186,  186,  187,  187,   28,   29}, SKINCOLOR_CHARTREUSE, 12, V_PURPLEMAP,  true}, // SKINCOLOR_NOBLE
	{"Fuchsia",    { 200,  201,  203,  204,  204,  183,  184,  184,  165,  166,  167,  168,  169,  159,  253,  254}, SKINCOLOR_LEMON,      10, V_PURPLEMAP,  true}, // SKINCOLOR_FUCHSIA
	{"Bubblegum",  {   0,  208,  208,  176,  177,  178,  179,  180,  181,  182,  164,  166,  167,  168,  169,  253}, SKINCOLOR_PASTEL,     8,  V_MAGENTAMAP, true}, // SKINCOLOR_BUBBLEGUM
	{"Siberite",   { 252,  177,  179,  180,  181,  181,  182,  182,  183,  164,  166,  167,  167,  168,  169,  159}, SKINCOLOR_EMERALD,    8,  V_MAGENTAMAP, true}, // SKINCOLOR_SIBERITE
	{"Magenta",    {0xb3, 0xb3, 0xb4, 0xb5, 0xb6, 0xb6, 0xb7, 0xb7, 0xb7, 0xb8, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xbb}, SKINCOLOR_LIME,       6,  V_MAGENTAMAP, true}, // SKINCOLOR_MAGENTA
	{"Neon",       {0xb3, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xbb, 0xc7, 0xc7, 0x1d, 0x1d, 0x1e}, SKINCOLOR_CERULEAN,   2,  V_MAGENTAMAP, true}, // SKINCOLOR_NEON
	{"Violet",     {0xd0, 0xd1, 0xd2, 0xca, 0xcc, 0xb8, 0xb9, 0xb9, 0xba, 0xa8, 0xa8, 0xa9, 0xa9, 0xfd, 0xfe, 0xfe}, SKINCOLOR_MINT,       6,  V_MAGENTAMAP, true}, // SKINCOLOR_VIOLET
	{"Royal",      { 208,  209,  192,  192,  192,  193,  193,  194,  194,  172,  173,  174,  175,  175,  139,  139}, SKINCOLOR_FANCY,      9,  V_PURPLEMAP,  true}, // SKINCOLOR_ROYAL
	{"Lilac",      {0x00, 0xd0, 0xd1, 0xd2, 0xd3, 0xc1, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc5, 0xc6, 0xc6, 0xfe, 0x1f}, SKINCOLOR_VAPOR,      4,  V_ROSYMAP,    true}, // SKINCOLOR_LILAC
	{"Mauve",      { 176,  177,  178,  192,  193,  194,  195,  195,  196,  185,  185,  186,  186,  187,  187,  253}, SKINCOLOR_HEADLIGHT,  8,  V_PURPLEMAP,  true}, // SKINCOLOR_MAUVE
	{"Eventide",   {  51,   52,   53,   33,   34,  204,  183,  183,  184,  184,  166,  167,  168,  169,  253,  254}, SKINCOLOR_DAYBREAK,   13, V_MAGENTAMAP, true}, // SKINCOLOR_EVENTIDE
	{"Plum",       {0xc8, 0xd3, 0xd5, 0xd6, 0xd7, 0xce, 0xcf, 0xb9, 0xb9, 0xba, 0xba, 0xa9, 0xa9, 0xa9, 0xfd, 0xfe}, SKINCOLOR_MINT,       7,  V_ROSYMAP,    true}, // SKINCOLOR_PLUM
	{"Raspberry",  {0xc8, 0xc9, 0xca, 0xcb, 0xcb, 0xcc, 0xcd, 0xcd, 0xce, 0xb9, 0xb9, 0xba, 0xba, 0xbb, 0xfe, 0xfe}, SKINCOLOR_APPLE,      13, V_ROSYMAP,    true}, // SKINCOLOR_RASPBERRY
	{"Taffy",      {   1,  176,  176,  177,  178,  179,  202,  203,  204,  204,  205,  206,  207,   44,   45,   46}, SKINCOLOR_JADE,       8,  V_ROSYMAP,    true}, // SKINCOLOR_TAFFY
	{"Rosy",       {0xfc, 0xc8, 0xc8, 0xc9, 0xc9, 0xca, 0xca, 0xcb, 0xcb, 0xcc, 0xcc, 0xcd, 0xcd, 0xce, 0xce, 0xcf}, SKINCOLOR_AQUA,       1,  V_ROSYMAP,    true}, // SKINCOLOR_ROSY
	{"Fancy",      {   0,  208,   49,  210,  210,  202,  202,  203,  204,  204,  205,  206,  207,  207,  186,  186}, SKINCOLOR_ROYAL,      9,  V_ROSYMAP,    true}, // SKINCOLOR_FANCY
	{"Sangria",    { 210,   32,   33,   34,   34,  215,  215,  207,  207,  185,  186,  186,  186,  169,  169,  253}, SKINCOLOR_TURQUOISE,  12, V_ROSYMAP,    true}, // SKINCOLOR_SANGRIA
	{"Volcanic",   {  54,   36,   42,   44,   45,   46,   46,   47,   28,  253,  253,  254,  254,   30,   31,   31}, SKINCOLOR_BRONZE,     9,  V_REDMAP,     true}, // SKINCOLOR_VOLCANIC

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

mobjtype_t GetMobjTypeByName(const char *name)
{
	for (mobjtype_t i = 0; i < NUMMOBJTYPES; i++)
		if (FREE_MOBJS[i] && !strcmp(FREE_MOBJS[i], name))
			return i;

	I_Error("Mobj type %s not found\n", name);
	return MT_NULL;
}

statenum_t GetStateByName(const char *name)
{
	for (statenum_t i = 0; i < NUMSTATES; i++)
		if (FREE_STATES[i] && !strcmp(FREE_STATES[i], name))
			return i;

	I_Error("State %s not found\n", name);
	return S_NULL;
}

void CacheInfoConstants(void)
{
	S_ALART1 = GetStateByName("ALART1");
	S_BLACKEGG_DESTROYPLAT1 = GetStateByName("BLACKEGG_DESTROYPLAT1");
	S_BLACKEGG_DIE4 = GetStateByName("BLACKEGG_DIE4");
	S_BLACKEGG_GOOP = GetStateByName("BLACKEGG_GOOP");
	S_BLACKEGG_HITFACE4 = GetStateByName("BLACKEGG_HITFACE4");
	S_BLACKEGG_JUMP1 = GetStateByName("BLACKEGG_JUMP1");
	S_BLACKEGG_JUMP2 = GetStateByName("BLACKEGG_JUMP2");
	S_BLACKEGG_PAIN1 = GetStateByName("BLACKEGG_PAIN1");
	S_BLACKEGG_PAIN35 = GetStateByName("BLACKEGG_PAIN35");
	S_BLACKEGG_SHOOT1 = GetStateByName("BLACKEGG_SHOOT1");
	S_BLACKEGG_SHOOT2 = GetStateByName("BLACKEGG_SHOOT2");
	S_BLACKEGG_STND = GetStateByName("BLACKEGG_STND");
	S_BLACKEGG_WALK1 = GetStateByName("BLACKEGG_WALK1");
	S_BLACKEGG_WALK6 = GetStateByName("BLACKEGG_WALK6");
	S_BLUESPHEREBONUS = GetStateByName("BLUESPHEREBONUS");
	S_BOSSEGLZ1 = GetStateByName("BOSSEGLZ1");
	S_BOSSEGLZ2 = GetStateByName("BOSSEGLZ2");
	S_BOSSSEBH1 = GetStateByName("BOSSSEBH1");
	S_BOSSSEBH2 = GetStateByName("BOSSSEBH2");
	S_BOSSSPIGOT = GetStateByName("BOSSSPIGOT");
	S_BOSSTANK1 = GetStateByName("BOSSTANK1");
	S_BOSSTANK2 = GetStateByName("BOSSTANK2");
	S_BUMBLEBORE_FALL2 = GetStateByName("BUMBLEBORE_FALL2");
	S_BUMBLEBORE_FLY1 = GetStateByName("BUMBLEBORE_FLY1");
	S_BUMBLEBORE_FLY2 = GetStateByName("BUMBLEBORE_FLY2");
	S_BUMBLEBORE_RAISE = GetStateByName("BUMBLEBORE_RAISE");
	S_BUMBLEBORE_STUCK2 = GetStateByName("BUMBLEBORE_STUCK2");
	S_CEMG1 = GetStateByName("CEMG1");
	S_CEMG2 = GetStateByName("CEMG2");
	S_CEMG3 = GetStateByName("CEMG3");
	S_CEMG4 = GetStateByName("CEMG4");
	S_CEMG5 = GetStateByName("CEMG5");
	S_CEMG6 = GetStateByName("CEMG6");
	S_CEMG7 = GetStateByName("CEMG7");
	S_CLEARSIGN = GetStateByName("CLEARSIGN");
	S_CRUSHCLAW_STAY = GetStateByName("CRUSHCLAW_STAY");
	S_CYBRAKDEMON_DIE8 = GetStateByName("CYBRAKDEMON_DIE8");
	S_EGGMANSIGN = GetStateByName("EGGMANSIGN");
	S_EXTRALARGEBUBBLE = GetStateByName("EXTRALARGEBUBBLE");
	S_FANG_BOUNCE3 = GetStateByName("FANG_BOUNCE3");
	S_FANG_BOUNCE4 = GetStateByName("FANG_BOUNCE4");
	S_FANG_INTRO12 = GetStateByName("FANG_INTRO12");
	S_FANG_PAIN1 = GetStateByName("FANG_PAIN1");
	S_FANG_PAIN2 = GetStateByName("FANG_PAIN2");
	S_FANG_PATHINGCONT1 = GetStateByName("FANG_PATHINGCONT1");
	S_FANG_PATHINGCONT2 = GetStateByName("FANG_PATHINGCONT2");
	S_FANG_PINCHBOUNCE3 = GetStateByName("FANG_PINCHBOUNCE3");
	S_FANG_PINCHBOUNCE4 = GetStateByName("FANG_PINCHBOUNCE4");
	S_FANG_PINCHPATHINGSTART2 = GetStateByName("FANG_PINCHPATHINGSTART2");
	S_FANG_WALLHIT = GetStateByName("FANG_WALLHIT");
	S_FLAMEJETFLAME4 = GetStateByName("FLAMEJETFLAME4");
	S_FLAMEJETFLAME7 = GetStateByName("FLAMEJETFLAME7");
	S_FLIGHTINDICATOR = GetStateByName("FLIGHTINDICATOR");
	S_HANGSTER_ARC1 = GetStateByName("HANGSTER_ARC1");
	S_HANGSTER_ARC2 = GetStateByName("HANGSTER_ARC2");
	S_HANGSTER_ARC3 = GetStateByName("HANGSTER_ARC3");
	S_HANGSTER_ARCUP1 = GetStateByName("HANGSTER_ARCUP1");
	S_HANGSTER_FLY1 = GetStateByName("HANGSTER_FLY1");
	S_HANGSTER_RETURN1 = GetStateByName("HANGSTER_RETURN1");
	S_HANGSTER_RETURN2 = GetStateByName("HANGSTER_RETURN2");
	S_HANGSTER_RETURN3 = GetStateByName("HANGSTER_RETURN3");
	S_HANGSTER_SWOOP1 = GetStateByName("HANGSTER_SWOOP1");
	S_HANGSTER_SWOOP2 = GetStateByName("HANGSTER_SWOOP2");
	S_INVISIBLE = GetStateByName("INVISIBLE");
	S_JETFUME1 = GetStateByName("JETFUME1");
	S_JETFUMEFLASH = GetStateByName("JETFUMEFLASH");
	S_LAVAFALL_DORMANT = GetStateByName("LAVAFALL_DORMANT");
	S_LAVAFALL_SHOOT = GetStateByName("LAVAFALL_SHOOT");
	S_LAVAFALL_TELL = GetStateByName("LAVAFALL_TELL");
	S_METALSONIC_BOUNCE = GetStateByName("METALSONIC_BOUNCE");
	S_MINECARTSEG_FRONT = GetStateByName("MINECARTSEG_FRONT");
	S_MSSHIELD_F2 = GetStateByName("MSSHIELD_F2");
	S_NIGHTSDRONE_SPARKLING1 = GetStateByName("NIGHTSDRONE_SPARKLING1");
	S_NIGHTSDRONE_SPARKLING16 = GetStateByName("NIGHTSDRONE_SPARKLING16");
	S_NULL = GetStateByName("NULL");
	S_OBJPLACE_DUMMY = GetStateByName("OBJPLACE_DUMMY");
	S_OILLAMPFLARE = GetStateByName("OILLAMPFLARE");
	S_PLAY_BOUNCE = GetStateByName("PLAY_BOUNCE");
	S_PLAY_BOUNCE_LANDING = GetStateByName("PLAY_BOUNCE_LANDING");
	S_PLAY_CLIMB = GetStateByName("PLAY_CLIMB");
	S_PLAY_CLING = GetStateByName("PLAY_CLING");
	S_PLAY_DASH = GetStateByName("PLAY_DASH");
	S_PLAY_DEAD = GetStateByName("PLAY_DEAD");
	S_PLAY_DRWN = GetStateByName("PLAY_DRWN");
	S_PLAY_EDGE = GetStateByName("PLAY_EDGE");
	S_PLAY_FALL = GetStateByName("PLAY_FALL");
	S_PLAY_FIRE = GetStateByName("PLAY_FIRE");
	S_PLAY_FIRE_FINISH = GetStateByName("PLAY_FIRE_FINISH");
	S_PLAY_FLOAT = GetStateByName("PLAY_FLOAT");
	S_PLAY_FLOAT_RUN = GetStateByName("PLAY_FLOAT_RUN");
	S_PLAY_FLY = GetStateByName("PLAY_FLY");
	S_PLAY_FLY_TIRED = GetStateByName("PLAY_FLY_TIRED");
	S_PLAY_GASP = GetStateByName("PLAY_GASP");
	S_PLAY_GLIDE = GetStateByName("PLAY_GLIDE");
	S_PLAY_GLIDE_LANDING = GetStateByName("PLAY_GLIDE_LANDING");
	S_PLAY_JUMP = GetStateByName("PLAY_JUMP");
	S_PLAY_MELEE = GetStateByName("PLAY_MELEE");
	S_PLAY_MELEE_FINISH = GetStateByName("PLAY_MELEE_FINISH");
	S_PLAY_MELEE_LANDING = GetStateByName("PLAY_MELEE_LANDING");
	S_PLAY_NIGHTS_ATTACK = GetStateByName("PLAY_NIGHTS_ATTACK");
	S_PLAY_NIGHTS_DRILL = GetStateByName("PLAY_NIGHTS_DRILL");
	S_PLAY_NIGHTS_FLOAT = GetStateByName("PLAY_NIGHTS_FLOAT");
	S_PLAY_NIGHTS_FLY = GetStateByName("PLAY_NIGHTS_FLY");
	S_PLAY_NIGHTS_PULL = GetStateByName("PLAY_NIGHTS_PULL");
	S_PLAY_NIGHTS_STAND = GetStateByName("PLAY_NIGHTS_STAND");
	S_PLAY_NIGHTS_STUN = GetStateByName("PLAY_NIGHTS_STUN");
	S_PLAY_NIGHTS_TRANS1 = GetStateByName("PLAY_NIGHTS_TRANS1");
	S_PLAY_NIGHTS_TRANS6 = GetStateByName("PLAY_NIGHTS_TRANS6");
	S_PLAY_PAIN = GetStateByName("PLAY_PAIN");
	S_PLAY_RIDE = GetStateByName("PLAY_RIDE");
	S_PLAY_ROLL = GetStateByName("PLAY_ROLL");
	S_PLAY_RUN = GetStateByName("PLAY_RUN");
	S_PLAY_SIGN = GetStateByName("PLAY_SIGN");
	S_PLAY_SKID = GetStateByName("PLAY_SKID");
	S_PLAY_SPINDASH = GetStateByName("PLAY_SPINDASH");
	S_PLAY_SPRING = GetStateByName("PLAY_SPRING");
	S_PLAY_STND = GetStateByName("PLAY_STND");
	S_PLAY_STUN = GetStateByName("PLAY_STUN");
	S_PLAY_SUPER_TRANS1 = GetStateByName("PLAY_SUPER_TRANS1");
	S_PLAY_SUPER_TRANS6 = GetStateByName("PLAY_SUPER_TRANS6");
	S_PLAY_SWIM = GetStateByName("PLAY_SWIM");
	S_PLAY_TWINSPIN = GetStateByName("PLAY_TWINSPIN");
	S_PLAY_WAIT = GetStateByName("PLAY_WAIT");
	S_PLAY_WALK = GetStateByName("PLAY_WALK");
	S_PTERABYTE_FLY1 = GetStateByName("PTERABYTE_FLY1");
	S_PTERABYTE_SWOOPDOWN = GetStateByName("PTERABYTE_SWOOPDOWN");
	S_PTERABYTE_SWOOPUP = GetStateByName("PTERABYTE_SWOOPUP");
	S_RAIN1 = GetStateByName("RAIN1");
	S_RAINRETURN = GetStateByName("RAINRETURN");
	S_REDBOOSTERROLLER = GetStateByName("REDBOOSTERROLLER");
	S_REDBOOSTERSEG_FACE = GetStateByName("REDBOOSTERSEG_FACE");
	S_REDBOOSTERSEG_LEFT = GetStateByName("REDBOOSTERSEG_LEFT");
	S_REDBOOSTERSEG_RIGHT = GetStateByName("REDBOOSTERSEG_RIGHT");
	S_ROSY_HUG = GetStateByName("ROSY_HUG");
	S_ROSY_IDLE1 = GetStateByName("ROSY_IDLE1");
	S_ROSY_IDLE2 = GetStateByName("ROSY_IDLE2");
	S_ROSY_IDLE3 = GetStateByName("ROSY_IDLE3");
	S_ROSY_IDLE4 = GetStateByName("ROSY_IDLE4");
	S_ROSY_JUMP = GetStateByName("ROSY_JUMP");
	S_ROSY_PAIN = GetStateByName("ROSY_PAIN");
	S_ROSY_STND = GetStateByName("ROSY_STND");
	S_ROSY_UNHAPPY = GetStateByName("ROSY_UNHAPPY");
	S_ROSY_WALK = GetStateByName("ROSY_WALK");
	S_SIGNBOARD = GetStateByName("SIGNBOARD");
	S_SIGNSPIN1 = GetStateByName("SIGNSPIN1");
	S_SMASHSPIKE_FALL = GetStateByName("SMASHSPIKE_FALL");
	S_SMASHSPIKE_FLOAT = GetStateByName("SMASHSPIKE_FLOAT");
	S_SMASHSPIKE_RISE2 = GetStateByName("SMASHSPIKE_RISE2");
	S_SMASHSPIKE_STOMP1 = GetStateByName("SMASHSPIKE_STOMP1");
	S_SNOW2 = GetStateByName("SNOW2");
	S_SNOW3 = GetStateByName("SNOW3");
	S_SPINCUSHION_AIM1 = GetStateByName("SPINCUSHION_AIM1");
	S_SPINCUSHION_AIM5 = GetStateByName("SPINCUSHION_AIM5");
	S_SPINDUST_BUBBLE1 = GetStateByName("SPINDUST_BUBBLE1");
	S_SPINDUST_BUBBLE4 = GetStateByName("SPINDUST_BUBBLE4");
	S_SPINDUST_FIRE1 = GetStateByName("SPINDUST_FIRE1");
	S_SPLASH1 = GetStateByName("SPLASH1");
	S_STEAM1 = GetStateByName("STEAM1");
	S_TAILSOVERLAY_0DEGREES = GetStateByName("TAILSOVERLAY_0DEGREES");
	S_TAILSOVERLAY_DASH = GetStateByName("TAILSOVERLAY_DASH");
	S_TAILSOVERLAY_EDGE = GetStateByName("TAILSOVERLAY_EDGE");
	S_TAILSOVERLAY_FLY = GetStateByName("TAILSOVERLAY_FLY");
	S_TAILSOVERLAY_GASP = GetStateByName("TAILSOVERLAY_GASP");
	S_TAILSOVERLAY_MINUS30DEGREES = GetStateByName("TAILSOVERLAY_MINUS30DEGREES");
	S_TAILSOVERLAY_MINUS60DEGREES = GetStateByName("TAILSOVERLAY_MINUS60DEGREES");
	S_TAILSOVERLAY_PAIN = GetStateByName("TAILSOVERLAY_PAIN");
	S_TAILSOVERLAY_PLUS30DEGREES = GetStateByName("TAILSOVERLAY_PLUS30DEGREES");
	S_TAILSOVERLAY_PLUS60DEGREES = GetStateByName("TAILSOVERLAY_PLUS60DEGREES");
	S_TAILSOVERLAY_RUN = GetStateByName("TAILSOVERLAY_RUN");
	S_TAILSOVERLAY_STAND = GetStateByName("TAILSOVERLAY_STAND");
	S_TAILSOVERLAY_TIRE = GetStateByName("TAILSOVERLAY_TIRE");
	S_TEAM_SPINFIRE1 = GetStateByName("TEAM_SPINFIRE1");
	S_TRAINDUST = GetStateByName("TRAINDUST");
	S_TRAINPUFFMAKER = GetStateByName("TRAINPUFFMAKER");
	S_TRAINSTEAM = GetStateByName("TRAINSTEAM");
	S_TUTORIALFLOWER1 = GetStateByName("TUTORIALFLOWER1");
	S_TUTORIALFLOWERF1 = GetStateByName("TUTORIALFLOWERF1");
	S_TUTORIALLEAF1 = GetStateByName("TUTORIALLEAF1");
	S_UNKNOWN = GetStateByName("UNKNOWN");
	S_XPLD_EGGTRAP = GetStateByName("XPLD_EGGTRAP");
	S_XPLD1 = GetStateByName("XPLD1");
	S_YELLOWBOOSTERROLLER = GetStateByName("YELLOWBOOSTERROLLER");
	S_YELLOWBOOSTERSEG_FACE = GetStateByName("YELLOWBOOSTERSEG_FACE");
	S_YELLOWBOOSTERSEG_LEFT = GetStateByName("YELLOWBOOSTERSEG_LEFT");
	S_YELLOWBOOSTERSEG_RIGHT = GetStateByName("YELLOWBOOSTERSEG_RIGHT");
}

/** Patches the mobjinfo, state, and skincolor tables.
  * Free slots are emptied out and set to initial values.
  */
void P_PatchInfoTables(void)
{
	INT32 i;
	char *tempname;

#if NUMSPRITEFREESLOTS > 9999 //tempname numbering actually starts at SPR_FIRSTFREESLOT, so the limit is actually 9999 + SPR_FIRSTFREESLOT-1, but the preprocessor doesn't understand enums, so its left at 9999 for safety
"Update P_PatchInfoTables, you big dumb head"
#endif

	// empty out free slots
	for (i = SPR_FIRSTFREESLOT; i <= SPR_LASTFREESLOT; i++)
	{
		tempname = sprnames[i];
		tempname[0] = (char)('0' + (char)((i-SPR_FIRSTFREESLOT+1)/1000));
		tempname[1] = (char)('0' + (char)(((i-SPR_FIRSTFREESLOT+1)/100)%10));
		tempname[2] = (char)('0' + (char)(((i-SPR_FIRSTFREESLOT+1)/10)%10));
		tempname[3] = (char)('0' + (char)((i-SPR_FIRSTFREESLOT+1)%10));
		tempname[4] = '\0';
#ifdef HWRENDER
		t_lspr[i] = &lspr[NOLIGHT];
#endif
	}
	sprnames[i][0] = '\0'; // i == NUMSPRITES
	memset(states, 0, sizeof (state_t) * NUMSTATES);
	memset(mobjinfo, 0, sizeof (mobjinfo_t) * NUMMOBJTYPES);
	memset(&skincolors[SKINCOLOR_FIRSTFREESLOT], 0, sizeof (skincolor_t) * NUMCOLORFREESLOTS);
	for (i = SKINCOLOR_FIRSTFREESLOT; i <= SKINCOLOR_LASTFREESLOT; i++) {
		skincolors[i].accessible = false;
		skincolors[i].name[0] = '\0';
	}
	for (i = 0; i < NUMMOBJTYPES; i++)
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
