// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  deh_tables.c
/// \brief Define DeHackEd tables.

#include "doomdef.h" // Constants
#include "s_sound.h" // Sound constants
#include "info.h" // Mobj, state, sprite, etc constants
#include "m_menu.h" // Menu constants
#include "y_inter.h" // Intermission constants
#include "p_local.h" // some more constants
#include "r_draw.h" // Colormap constants
#include "lua_script.h" // Lua stuff
#include "m_cond.h" // Emblem constants
#include "v_video.h" // video flags (for lua)
#include "i_sound.h" // musictype_t (for lua)
#include "g_state.h" // gamestate_t (for lua)
#include "g_game.h" // Joystick axes (for lua)
#include "i_joy.h"
#include "g_input.h" // Game controls (for lua)

#include "deh_tables.h"

char *FREE_STATES[NUMSTATES];
char *FREE_MOBJS[NUMMOBJTYPES];
char *FREE_SKINCOLORS[NUMCOLORFREESLOTS];
bitarray_t used_spr[BIT_ARRAY_SIZE(NUMSPRITEFREESLOTS)]; // Sprite freeslots in use

const char NIGHTSGRADE_LIST[] = {
	'F', // GRADE_F
	'E', // GRADE_E
	'D', // GRADE_D
	'C', // GRADE_C
	'B', // GRADE_B
	'A', // GRADE_A
	'S', // GRADE_S
	'\0'
};

struct flickytypes_s FLICKYTYPES[] = {
	{"BLUEBIRD", "FLICKY_01"}, // Flicky (Flicky)
	{"RABBIT",   "FLICKY_02"}, // Pocky (1)
	{"CHICKEN",  "FLICKY_03"}, // Cucky (1)
	{"SEAL",     "FLICKY_04"}, // Rocky (1)
	{"PIG",      "FLICKY_05"}, // Picky (1)
	{"CHIPMUNK", "FLICKY_06"}, // Ricky (1)
	{"PENGUIN",  "FLICKY_07"}, // Pecky (1)
	{"FISH",     "FLICKY_08"}, // Nicky (CD)
	{"RAM",      "FLICKY_09"}, // Flocky (CD)
	{"PUFFIN",   "FLICKY_10"}, // Wicky (CD)
	{"COW",      "FLICKY_11"}, // Macky (SRB2)
	{"RAT",      "FLICKY_12"}, // Micky (2)
	{"BEAR",     "FLICKY_13"}, // Becky (2)
	{"DOVE",     "FLICKY_14"}, // Docky (CD)
	{"CAT",      "FLICKY_15"}, // Nyannyan (Flicky)
	{"CANARY",   "FLICKY_16"}, // Lucky (CD)
	{"a", NULL}, // End of normal flickies - a lower case character so will never fastcmp valid with uppercase tmp
	//{"FLICKER",          "FLICKER"}, // Flacky (SRB2)
	{"SPIDER",   "SECRETFLICKY_01"}, // Sticky (SRB2)
	{"BAT",      "SECRETFLICKY_02"}, // Backy (SRB2)
	{"SEED",                "SEED"}, // Seed (CD)
	{NULL, NULL}
};

// IMPORTANT!
// DO NOT FORGET TO SYNC THIS LIST WITH THE ACTIONNUM ENUM IN INFO.H
actionpointer_t actionpointers[] =
{
	{{A_Explode},                "A_EXPLODE"},
	{{A_Pain},                   "A_PAIN"},
	{{A_Fall},                   "A_FALL"},
	{{A_MonitorPop},             "A_MONITORPOP"},
	{{A_GoldMonitorPop},         "A_GOLDMONITORPOP"},
	{{A_GoldMonitorRestore},     "A_GOLDMONITORRESTORE"},
	{{A_GoldMonitorSparkle},     "A_GOLDMONITORSPARKLE"},
	{{A_Look},                   "A_LOOK"},
	{{A_Chase},                  "A_CHASE"},
	{{A_FaceStabChase},          "A_FACESTABCHASE"},
	{{A_FaceStabRev},            "A_FACESTABREV"},
	{{A_FaceStabHurl},           "A_FACESTABHURL"},
	{{A_FaceStabMiss},           "A_FACESTABMISS"},
	{{A_StatueBurst},            "A_STATUEBURST"},
	{{A_FaceTarget},             "A_FACETARGET"},
	{{A_FaceTracer},             "A_FACETRACER"},
	{{A_Scream},                 "A_SCREAM"},
	{{A_BossDeath},              "A_BOSSDEATH"},
	{{A_SetShadowScale},         "A_SETSHADOWSCALE"},
	{{A_ShadowScream},           "A_SHADOWSCREAM"},
	{{A_CustomPower},            "A_CUSTOMPOWER"},
	{{A_GiveWeapon},             "A_GIVEWEAPON"},
	{{A_RingBox},                "A_RINGBOX"},
	{{A_Invincibility},          "A_INVINCIBILITY"},
	{{A_SuperSneakers},          "A_SUPERSNEAKERS"},
	{{A_BunnyHop},               "A_BUNNYHOP"},
	{{A_BubbleSpawn},            "A_BUBBLESPAWN"},
	{{A_FanBubbleSpawn},         "A_FANBUBBLESPAWN"},
	{{A_BubbleRise},             "A_BUBBLERISE"},
	{{A_BubbleCheck},            "A_BUBBLECHECK"},
	{{A_AwardScore},             "A_AWARDSCORE"},
	{{A_ExtraLife},              "A_EXTRALIFE"},
	{{A_GiveShield},             "A_GIVESHIELD"},
	{{A_GravityBox},             "A_GRAVITYBOX"},
	{{A_ScoreRise},              "A_SCORERISE"},
	{{A_AttractChase},           "A_ATTRACTCHASE"},
	{{A_DropMine},               "A_DROPMINE"},
	{{A_FishJump},               "A_FISHJUMP"},
	{{A_ThrownRing},             "A_THROWNRING"},
	{{A_SetSolidSteam},          "A_SETSOLIDSTEAM"},
	{{A_UnsetSolidSteam},        "A_UNSETSOLIDSTEAM"},
	{{A_SignSpin},               "A_SIGNSPIN"},
	{{A_SignPlayer},             "A_SIGNPLAYER"},
	{{A_OverlayThink},           "A_OVERLAYTHINK"},
	{{A_JetChase},               "A_JETCHASE"},
	{{A_JetbThink},              "A_JETBTHINK"},
	{{A_JetgThink},              "A_JETGTHINK"},
	{{A_JetgShoot},              "A_JETGSHOOT"},
	{{A_ShootBullet},            "A_SHOOTBULLET"},
	{{A_MinusDigging},           "A_MINUSDIGGING"},
	{{A_MinusPopup},             "A_MINUSPOPUP"},
	{{A_MinusCheck},             "A_MINUSCHECK"},
	{{A_ChickenCheck},           "A_CHICKENCHECK"},
	{{A_MouseThink},             "A_MOUSETHINK"},
	{{A_DetonChase},             "A_DETONCHASE"},
	{{A_CapeChase},              "A_CAPECHASE"},
	{{A_RotateSpikeBall},        "A_ROTATESPIKEBALL"},
	{{A_SlingAppear},            "A_SLINGAPPEAR"},
	{{A_UnidusBall},             "A_UNIDUSBALL"},
	{{A_RockSpawn},              "A_ROCKSPAWN"},
	{{A_SetFuse},                "A_SETFUSE"},
	{{A_CrawlaCommanderThink},   "A_CRAWLACOMMANDERTHINK"},
	{{A_SmokeTrailer},           "A_SMOKETRAILER"},
	{{A_RingExplode},            "A_RINGEXPLODE"},
	{{A_OldRingExplode},         "A_OLDRINGEXPLODE"},
	{{A_MixUp},                  "A_MIXUP"},
	{{A_RecyclePowers},          "A_RECYCLEPOWERS"},
	{{A_Boss1Chase},             "A_BOSS1CHASE"},
	{{A_FocusTarget},            "A_FOCUSTARGET"},
	{{A_Boss2Chase},             "A_BOSS2CHASE"},
	{{A_Boss2Pogo},              "A_BOSS2POGO"},
	{{A_BossZoom},               "A_BOSSZOOM"},
	{{A_BossScream},             "A_BOSSSCREAM"},
	{{A_Boss2TakeDamage},        "A_BOSS2TAKEDAMAGE"},
	{{A_Boss7Chase},             "A_BOSS7CHASE"},
	{{A_GoopSplat},              "A_GOOPSPLAT"},
	{{A_Boss2PogoSFX},           "A_BOSS2POGOSFX"},
	{{A_Boss2PogoTarget},        "A_BOSS2POGOTARGET"},
	{{A_BossJetFume},            "A_BOSSJETFUME"},
	{{A_EggmanBox},              "A_EGGMANBOX"},
	{{A_TurretFire},             "A_TURRETFIRE"},
	{{A_SuperTurretFire},        "A_SUPERTURRETFIRE"},
	{{A_TurretStop},             "A_TURRETSTOP"},
	{{A_JetJawRoam},             "A_JETJAWROAM"},
	{{A_JetJawChomp},            "A_JETJAWCHOMP"},
	{{A_PointyThink},            "A_POINTYTHINK"},
	{{A_CheckBuddy},             "A_CHECKBUDDY"},
	{{A_HoodFire},               "A_HOODFIRE"},
	{{A_HoodThink},              "A_HOODTHINK"},
	{{A_HoodFall},               "A_HOODFALL"},
	{{A_ArrowBonks},             "A_ARROWBONKS"},
	{{A_SnailerThink},           "A_SNAILERTHINK"},
	{{A_SharpChase},             "A_SHARPCHASE"},
	{{A_SharpSpin},              "A_SHARPSPIN"},
	{{A_SharpDecel},             "A_SHARPDECEL"},
	{{A_CrushstaceanWalk},       "A_CRUSHSTACEANWALK"},
	{{A_CrushstaceanPunch},      "A_CRUSHSTACEANPUNCH"},
	{{A_CrushclawAim},           "A_CRUSHCLAWAIM"},
	{{A_CrushclawLaunch},        "A_CRUSHCLAWLAUNCH"},
	{{A_VultureVtol},            "A_VULTUREVTOL"},
	{{A_VultureCheck},           "A_VULTURECHECK"},
	{{A_VultureHover},           "A_VULTUREHOVER"},
	{{A_VultureBlast},           "A_VULTUREBLAST"},
	{{A_VultureFly},             "A_VULTUREFLY"},
	{{A_SkimChase},              "A_SKIMCHASE"},
	{{A_1upThinker},             "A_1UPTHINKER"},
	{{A_SkullAttack},            "A_SKULLATTACK"},
	{{A_LobShot},                "A_LOBSHOT"},
	{{A_FireShot},               "A_FIRESHOT"},
	{{A_SuperFireShot},          "A_SUPERFIRESHOT"},
	{{A_BossFireShot},           "A_BOSSFIRESHOT"},
	{{A_Boss7FireMissiles},      "A_BOSS7FIREMISSILES"},
	{{A_Boss1Laser},             "A_BOSS1LASER"},
	{{A_Boss4Reverse},           "A_BOSS4REVERSE"},
	{{A_Boss4SpeedUp},           "A_BOSS4SPEEDUP"},
	{{A_Boss4Raise},             "A_BOSS4RAISE"},
	{{A_SparkFollow},            "A_SPARKFOLLOW"},
	{{A_BuzzFly},                "A_BUZZFLY"},
	{{A_GuardChase},             "A_GUARDCHASE"},
	{{A_EggShield},              "A_EGGSHIELD"},
	{{A_SetReactionTime},        "A_SETREACTIONTIME"},
	{{A_Boss1Spikeballs},        "A_BOSS1SPIKEBALLS"},
	{{A_Boss3TakeDamage},        "A_BOSS3TAKEDAMAGE"},
	{{A_Boss3Path},              "A_BOSS3PATH"},
	{{A_Boss3ShockThink},        "A_BOSS3SHOCKTHINK"},
	{{A_Shockwave},              "A_SHOCKWAVE"},
	{{A_LinedefExecute},         "A_LINEDEFEXECUTE"},
	{{A_LinedefExecuteFromArg},  "A_LINEDEFEXECUTEFROMARG"},
	{{A_PlaySeeSound},           "A_PLAYSEESOUND"},
	{{A_PlayAttackSound},        "A_PLAYATTACKSOUND"},
	{{A_PlayActiveSound},        "A_PLAYACTIVESOUND"},
	{{A_SpawnObjectAbsolute},    "A_SPAWNOBJECTABSOLUTE"},
	{{A_SpawnObjectRelative},    "A_SPAWNOBJECTRELATIVE"},
	{{A_ChangeAngleRelative},    "A_CHANGEANGLERELATIVE"},
	{{A_ChangeAngleAbsolute},    "A_CHANGEANGLEABSOLUTE"},
	{{A_RollAngle},              "A_ROLLANGLE"},
	{{A_ChangeRollAngleRelative},"A_CHANGEROLLANGLERELATIVE"},
	{{A_ChangeRollAngleAbsolute},"A_CHANGEROLLANGLEABSOLUTE"},
	{{A_PlaySound},              "A_PLAYSOUND"},
	{{A_FindTarget},             "A_FINDTARGET"},
	{{A_FindTracer},             "A_FINDTRACER"},
	{{A_SetTics},                "A_SETTICS"},
	{{A_SetRandomTics},          "A_SETRANDOMTICS"},
	{{A_ChangeColorRelative},    "A_CHANGECOLORRELATIVE"},
	{{A_ChangeColorAbsolute},    "A_CHANGECOLORABSOLUTE"},
	{{A_Dye},                    "A_DYE"},
	{{A_SetTranslation},         "A_SETTRANSLATION"},
	{{A_MoveRelative},           "A_MOVERELATIVE"},
	{{A_MoveAbsolute},           "A_MOVEABSOLUTE"},
	{{A_Thrust},                 "A_THRUST"},
	{{A_ZThrust},                "A_ZTHRUST"},
	{{A_SetTargetsTarget},       "A_SETTARGETSTARGET"},
	{{A_SetObjectFlags},         "A_SETOBJECTFLAGS"},
	{{A_SetObjectFlags2},        "A_SETOBJECTFLAGS2"},
	{{A_RandomState},            "A_RANDOMSTATE"},
	{{A_RandomStateRange},       "A_RANDOMSTATERANGE"},
	{{A_StateRangeByAngle},      "A_STATERANGEBYANGLE"},
	{{A_StateRangeByParameter},  "A_STATERANGEBYPARAMETER"},
	{{A_DualAction},             "A_DUALACTION"},
	{{A_RemoteAction},           "A_REMOTEACTION"},
	{{A_ToggleFlameJet},         "A_TOGGLEFLAMEJET"},
	{{A_OrbitNights},            "A_ORBITNIGHTS"},
	{{A_GhostMe},                "A_GHOSTME"},
	{{A_SetObjectState},         "A_SETOBJECTSTATE"},
	{{A_SetObjectTypeState},     "A_SETOBJECTTYPESTATE"},
	{{A_KnockBack},              "A_KNOCKBACK"},
	{{A_PushAway},               "A_PUSHAWAY"},
	{{A_RingDrain},              "A_RINGDRAIN"},
	{{A_SplitShot},              "A_SPLITSHOT"},
	{{A_MissileSplit},           "A_MISSILESPLIT"},
	{{A_MultiShot},              "A_MULTISHOT"},
	{{A_InstaLoop},              "A_INSTALOOP"},
	{{A_Custom3DRotate},         "A_CUSTOM3DROTATE"},
	{{A_SearchForPlayers},       "A_SEARCHFORPLAYERS"},
	{{A_CheckRandom},            "A_CHECKRANDOM"},
	{{A_CheckTargetRings},       "A_CHECKTARGETRINGS"},
	{{A_CheckRings},             "A_CHECKRINGS"},
	{{A_CheckTotalRings},        "A_CHECKTOTALRINGS"},
	{{A_CheckHealth},            "A_CHECKHEALTH"},
	{{A_CheckRange},             "A_CHECKRANGE"},
	{{A_CheckHeight},            "A_CHECKHEIGHT"},
	{{A_CheckTrueRange},         "A_CHECKTRUERANGE"},
	{{A_CheckThingCount},        "A_CHECKTHINGCOUNT"},
	{{A_CheckAmbush},            "A_CHECKAMBUSH"},
	{{A_CheckCustomValue},       "A_CHECKCUSTOMVALUE"},
	{{A_CheckCusValMemo},        "A_CHECKCUSVALMEMO"},
	{{A_SetCustomValue},         "A_SETCUSTOMVALUE"},
	{{A_UseCusValMemo},          "A_USECUSVALMEMO"},
	{{A_RelayCustomValue},       "A_RELAYCUSTOMVALUE"},
	{{A_CusValAction},           "A_CUSVALACTION"},
	{{A_ForceStop},              "A_FORCESTOP"},
	{{A_ForceWin},               "A_FORCEWIN"},
	{{A_SpikeRetract},           "A_SPIKERETRACT"},
	{{A_InfoState},              "A_INFOSTATE"},
	{{A_Repeat},                 "A_REPEAT"},
	{{A_SetScale},               "A_SETSCALE"},
	{{A_RemoteDamage},           "A_REMOTEDAMAGE"},
	{{A_HomingChase},            "A_HOMINGCHASE"},
	{{A_TrapShot},               "A_TRAPSHOT"},
	{{A_VileTarget},             "A_VILETARGET"},
	{{A_VileAttack},             "A_VILEATTACK"},
	{{A_VileFire},               "A_VILEFIRE"},
	{{A_BrakChase},              "A_BRAKCHASE"},
	{{A_BrakFireShot},           "A_BRAKFIRESHOT"},
	{{A_BrakLobShot},            "A_BRAKLOBSHOT"},
	{{A_NapalmScatter},          "A_NAPALMSCATTER"},
	{{A_SpawnFreshCopy},         "A_SPAWNFRESHCOPY"},
	{{A_FlickySpawn},            "A_FLICKYSPAWN"},
	{{A_FlickyCenter},           "A_FLICKYCENTER"},
	{{A_FlickyAim},              "A_FLICKYAIM"},
	{{A_FlickyFly},              "A_FLICKYFLY"},
	{{A_FlickySoar},             "A_FLICKYSOAR"},
	{{A_FlickyCoast},            "A_FLICKYCOAST"},
	{{A_FlickyHop},              "A_FLICKYHOP"},
	{{A_FlickyFlounder},         "A_FLICKYFLOUNDER"},
	{{A_FlickyCheck},            "A_FLICKYCHECK"},
	{{A_FlickyHeightCheck},      "A_FLICKYHEIGHTCHECK"},
	{{A_FlickyFlutter},          "A_FLICKYFLUTTER"},
	{{A_FlameParticle},          "A_FLAMEPARTICLE"},
	{{A_FadeOverlay},            "A_FADEOVERLAY"},
	{{A_Boss5Jump},              "A_BOSS5JUMP"},
	{{A_LightBeamReset},         "A_LIGHTBEAMRESET"},
	{{A_MineExplode},            "A_MINEEXPLODE"},
	{{A_MineRange},              "A_MINERANGE"},
	{{A_ConnectToGround},        "A_CONNECTTOGROUND"},
	{{A_SpawnParticleRelative},  "A_SPAWNPARTICLERELATIVE"},
	{{A_MultiShotDist},          "A_MULTISHOTDIST"},
	{{A_WhoCaresIfYourSonIsABee},"A_WHOCARESIFYOURSONISABEE"},
	{{A_ParentTriesToSleep},     "A_PARENTTRIESTOSLEEP"},
	{{A_CryingToMomma},          "A_CRYINGTOMOMMA"},
	{{A_CheckFlags2},            "A_CHECKFLAGS2"},
	{{A_Boss5FindWaypoint},      "A_BOSS5FINDWAYPOINT"},
	{{A_DoNPCSkid},              "A_DONPCSKID"},
	{{A_DoNPCPain},              "A_DONPCPAIN"},
	{{A_PrepareRepeat},          "A_PREPAREREPEAT"},
	{{A_Boss5ExtraRepeat},       "A_BOSS5EXTRAREPEAT"},
	{{A_Boss5Calm},              "A_BOSS5CALM"},
	{{A_Boss5CheckOnGround},     "A_BOSS5CHECKONGROUND"},
	{{A_Boss5CheckFalling},      "A_BOSS5CHECKFALLING"},
	{{A_Boss5PinchShot},         "A_BOSS5PINCHSHOT"},
	{{A_Boss5MakeItRain},        "A_BOSS5MAKEITRAIN"},
	{{A_Boss5MakeJunk},          "A_BOSS5MAKEJUNK"},
	{{A_LookForBetter},          "A_LOOKFORBETTER"},
	{{A_Boss5BombExplode},       "A_BOSS5BOMBEXPLODE"},
	{{A_DustDevilThink},         "A_DUSTDEVILTHINK"},
	{{A_TNTExplode},             "A_TNTEXPLODE"},
	{{A_DebrisRandom},           "A_DEBRISRANDOM"},
	{{A_TrainCameo},             "A_TRAINCAMEO"},
	{{A_TrainCameo2},            "A_TRAINCAMEO2"},
	{{A_CanarivoreGas},          "A_CANARIVOREGAS"},
	{{A_KillSegments},           "A_KILLSEGMENTS"},
	{{A_SnapperSpawn},           "A_SNAPPERSPAWN"},
	{{A_SnapperThinker},         "A_SNAPPERTHINKER"},
	{{A_SaloonDoorSpawn},        "A_SALOONDOORSPAWN"},
	{{A_MinecartSparkThink},     "A_MINECARTSPARKTHINK"},
	{{A_ModuloToState},          "A_MODULOTOSTATE"},
	{{A_LavafallRocks},          "A_LAVAFALLROCKS"},
	{{A_LavafallLava},           "A_LAVAFALLLAVA"},
	{{A_FallingLavaCheck},       "A_FALLINGLAVACHECK"},
	{{A_FireShrink},             "A_FIRESHRINK"},
	{{A_SpawnPterabytes},        "A_SPAWNPTERABYTES"},
	{{A_PterabyteHover},         "A_PTERABYTEHOVER"},
	{{A_RolloutSpawn},           "A_ROLLOUTSPAWN"},
	{{A_RolloutRock},            "A_ROLLOUTROCK"},
	{{A_DragonbomberSpawn},      "A_DRAGONBOMBERSPAWN"},
	{{A_DragonWing},             "A_DRAGONWING"},
	{{A_DragonSegment},          "A_DRAGONSEGMENT"},
	{{A_ChangeHeight},           "A_CHANGEHEIGHT"},
	{{NULL},                     "NONE"},

	// This NULL entry must be the last in the list
	{{NULL},                   NULL},
};

////////////////////////////////////////////////////////////////////////////////
// CRAZY LIST OF STATE NAMES AND ALL FROM HERE DOWN
// TODO: Make this all a seperate file or something, like part of info.c??
// TODO: Read the list from a text lump in a WAD as necessary instead
// or something, don't just keep it all in memory like this.
// TODO: Make the lists public so we can start using actual mobj
// and state names in warning and error messages! :D

const char *const MOBJFLAG_LIST[] = {
	"SPECIAL",
	"SOLID",
	"SHOOTABLE",
	"NOSECTOR",
	"NOBLOCKMAP",
	"PAPERCOLLISION",
	"PUSHABLE",
	"BOSS",
	"SPAWNCEILING",
	"NOGRAVITY",
	"AMBIENT",
	"SLIDEME",
	"NOCLIP",
	"FLOAT",
	"BOXICON",
	"MISSILE",
	"SPRING",
	"BOUNCE",
	"MONITOR",
	"NOTHINK",
	"FIRE",
	"NOCLIPHEIGHT",
	"ENEMY",
	"SCENERY",
	"PAIN",
	"STICKY",
	"NIGHTSITEM",
	"NOCLIPTHING",
	"GRENADEBOUNCE",
	"RUNSPAWNFUNC",
	NULL
};

// \tMF2_(\S+).*// (.+) --> \t"\1", // \2
const char *const MOBJFLAG2_LIST[] = {
	"AXIS",			  // It's a NiGHTS axis! (For faster checking)
	"TWOD",			  // Moves like it's in a 2D level
	"DONTRESPAWN",	  // Don't respawn this object!
	"DONTDRAW",		  // Don't generate a vissprite
	"AUTOMATIC",	  // Thrown ring has automatic properties
	"RAILRING",		  // Thrown ring has rail properties
	"BOUNCERING",	  // Thrown ring has bounce properties
	"EXPLOSION",	  // Thrown ring has explosive properties
	"SCATTER",		  // Thrown ring has scatter properties
	"BEYONDTHEGRAVE", // Source of this missile has died and has since respawned.
	"SLIDEPUSH",	  // MF_PUSHABLE that pushes continuously.
	"CLASSICPUSH",    // Drops straight down when object has negative momz.
	"INVERTAIMABLE",  // Flips whether it's targetable by A_LookForEnemies (enemies no, decoys yes)
	"INFLOAT",		  // Floating to a height for a move, don't auto float to target's height.
	"DEBRIS",		  // Splash ring from explosion ring
	"NIGHTSPULL",	  // Attracted from a paraloop
	"JUSTATTACKED",	  // can be pushed by other moving mobjs
	"FIRING",		  // turret fire
	"SUPERFIRE",	  // Firing something with Super Sonic-stopping properties. Or, if mobj has MF_MISSILE, this is the actual fire from it.
	"SHADOW",		  // Fuzzy draw, makes targeting harder.
	"STRONGBOX",	  // Flag used for "strong" random monitors.
	"OBJECTFLIP",	  // Flag for objects that always have flipped gravity.
	"SKULLFLY",		  // Special handling: skull in flight.
	"FRET",			  // Flashing from a previous hit
	"BOSSNOTRAP",	  // No Egg Trap after boss
	"BOSSFLEE",		  // Boss is fleeing!
	"BOSSDEAD",		  // Boss is dead! (Not necessarily fleeing, if a fleeing point doesn't exist.)
	"AMBUSH",         // Alternate behaviour typically set by MTF_AMBUSH
	"LINKDRAW",       // Draw vissprite of mobj immediately before/after tracer's vissprite (dependent on dispoffset and position)
	"SHIELD",         // Thinker calls P_AddShield/P_ShieldLook (must be partnered with MF_SCENERY to use)
	"SPLAT",          // Object is a splat
	NULL
};

const char *const MOBJEFLAG_LIST[] = {
	"ONGROUND", // The mobj stands on solid floor (not on another mobj or in air)
	"JUSTHITFLOOR", // The mobj just hit the floor while falling, this is cleared on next frame
	"TOUCHWATER", // The mobj stands in a sector with water, and touches the surface
	"UNDERWATER", // The mobj stands in a sector with water, and his waist is BELOW the water surface
	"JUSTSTEPPEDDOWN", // used for ramp sectors
	"VERTICALFLIP", // Vertically flip sprite/allow upside-down physics
	"GOOWATER", // Goo water
	"TOUCHLAVA", // The mobj is touching a lava block
	"PUSHED", // Mobj was already pushed this tic
	"SPRUNG", // Mobj was already sprung this tic
	"APPLYPMOMZ", // Platform movement
	"TRACERANGLE", // Compute and trigger on mobj angle relative to tracer
	"FORCESUPER", // Forces an object to use super sprites with SPR_PLAY.
	"FORCENOSUPER", // Forces an object to NOT use super sprites with SPR_PLAY.
	NULL
};

const char *const MAPTHINGFLAG_LIST[] = {
	"EXTRA", // Extra flag for objects.
	"OBJECTFLIP", // Reverse gravity flag for objects.
	"OBJECTSPECIAL", // Special flag used with certain objects.
	"AMBUSH", // Deaf monsters/do not react to sound.
	"ABSOLUTEZ" // Absolute spawn height flag for objects.
};

const char *const PLAYERFLAG_LIST[] = {

	// Cvars
	"FLIPCAM", // Flip camera angle with gravity flip prefrence.
	"ANALOGMODE", // Analog mode?
	"DIRECTIONCHAR", // Directional character sprites?
	"AUTOBRAKE", // Autobrake?

	// Cheats
	"GODMODE",
	"NOCLIP",
	"INVIS",

	// True if button down last tic.
	"ATTACKDOWN",
	"SPINDOWN",
	"JUMPDOWN",
	"WPNDOWN",

	// Unmoving states
	"STASIS", // Player is not allowed to move
	"JUMPSTASIS", // and that includes jumping.
	// (we don't include FULLSTASIS here I guess because it's just those two together...?)

	// Applying autobrake?
	"APPLYAUTOBRAKE",

	// Character action status
	"STARTJUMP",
	"JUMPED",
	"NOJUMPDAMAGE",

	"SPINNING",
	"STARTDASH",

	"THOKKED",
	"SHIELDABILITY",
	"GLIDING",
	"BOUNCING",

	// Sliding (usually in water) like Labyrinth/Oil Ocean
	"SLIDING",

	// NiGHTS stuff
	"TRANSFERTOCLOSEST",
	"DRILLING",

	// Gametype-specific stuff
	"GAMETYPEOVER", // Race time over, or H&S out-of-game
	"TAGIT", // The player is it! For Tag Mode

	/*** misc ***/
	"FORCESTRAFE", // Translate turn inputs into strafe inputs
	"CANCARRY", // Can carry?
	"FINISHED",

	NULL // stop loop here.
};

const char *const GAMETYPERULE_LIST[] = {
	"CAMPAIGN",
	"RINGSLINGER",
	"SPECTATORS",
	"LIVES",
	"TEAMS",
	"FIRSTPERSON",
	"POWERSTONES",
	"TEAMFLAGS",
	"FRIENDLY",
	"SPECIALSTAGES",
	"EMERALDTOKENS",
	"EMERALDHUNT",
	"RACE",
	"TAG",
	"POINTLIMIT",
	"TIMELIMIT",
	"OVERTIME",
	"HURTMESSAGES",
	"FRIENDLYFIRE",
	"STARTCOUNTDOWN",
	"HIDEFROZEN",
	"BLINDFOLDED",
	"RESPAWNDELAY",
	"PITYSHIELD",
	"DEATHPENALTY",
	"NOSPECTATORSPAWN",
	"DEATHMATCHSTARTS",
	"SPAWNINVUL",
	"SPAWNENEMIES",
	"ALLOWEXIT",
	"NOTITLECARD",
	"CUTSCENES",
	NULL
};

// Linedef flags
const char *const ML_LIST[] = {
	"IMPASSIBLE",
	"BLOCKMONSTERS",
	"TWOSIDED",
	"DONTPEGTOP",
	"DONTPEGBOTTOM",
	"SKEWTD",
	"NOCLIMB",
	"NOSKEW",
	"MIDPEG",
	"MIDSOLID",
	"WRAPMIDTEX",
	"NETONLY",
	"NONET",
	"EFFECT6",
	"BOUNCY",
	"TFERLINE",
	NULL
};

// Sector flags
const char *const MSF_LIST[] = {
	"FLIPSPECIAL_FLOOR",
	"FLIPSPECIAL_CEILING",
	"TRIGGERSPECIAL_TOUCH",
	"TRIGGERSPECIAL_HEADBUMP",
	"TRIGGERLINE_PLANE",
	"TRIGGERLINE_MOBJ",
	"GRAVITYFLIP",
	"HEATWAVE",
	"NOCLIPCAMERA",
	NULL
};

// Sector special flags
const char *const SSF_LIST[] = {
	"OUTERSPACE",
	"DOUBLESTEPUP",
	"NOSTEPDOWN",
	"WINDCURRENT",
	"CONVEYOR",
	"SPEEDPAD",
	"STARPOSTACTIVATOR",
	"EXIT",
	"SPECIALSTAGEPIT",
	"RETURNFLAG",
	"REDTEAMBASE",
	"BLUETEAMBASE",
	"FAN",
	"SUPERTRANSFORM",
	"FORCESPIN",
	"ZOOMTUBESTART",
	"ZOOMTUBEEND",
	"FINISHLINE",
	"ROPEHANG",
	"JUMPFLIP",
	"GRAVITYOVERRIDE",
	NULL
};

// Sector damagetypes
const char *const SD_LIST[] = {
	"NONE",
	"GENERIC",
	"WATER",
	"FIRE",
	"LAVA",
	"ELECTRIC",
	"SPIKE",
	"DEATHPITTILT",
	"DEATHPITNOTILT",
	"INSTAKILL",
	"SPECIALSTAGE",
	NULL
};

// Sector triggerer
const char *const TO_LIST[] = {
	"PLAYER",
	"ALLPLAYERS",
	"MOBJ",
	NULL
};

const char *COLOR_ENUMS[] = {
	"NONE",			// SKINCOLOR_NONE,

	// Greyscale ranges
	"WHITE",     	// SKINCOLOR_WHITE,
	"BONE",     	// SKINCOLOR_BONE,
	"CLOUDY",     	// SKINCOLOR_CLOUDY,
	"GREY",     	// SKINCOLOR_GREY,
	"SILVER",     	// SKINCOLOR_SILVER,
	"CARBON",     	// SKINCOLOR_CARBON,
	"JET",     		// SKINCOLOR_JET,
	"BLACK",     	// SKINCOLOR_BLACK,

	// Desaturated
	"AETHER",     	// SKINCOLOR_AETHER,
	"SLATE",     	// SKINCOLOR_SLATE,
	"MOONSTONE",   	// SKINCOLOR_MOONSTONE,
	"BLUEBELL",   	// SKINCOLOR_BLUEBELL,
	"PINK",     	// SKINCOLOR_PINK,
	"ROSEWOOD",   	// SKINCOLOR_ROSEWOOD,
	"YOGURT",     	// SKINCOLOR_YOGURT,
	"LATTE",     	// SKINCOLOR_LATTE,
	"BROWN",     	// SKINCOLOR_BROWN,
	"BOULDER",   	// SKINCOLOR_BOULDER
	"BRONZE",     	// SKINCOLOR_BRONZE,
	"SEPIA",     	// SKINCOLOR_SEPIA,
	"ECRU",     	// SKINCOLOR_ECRU,
	"TAN",      	// SKINCOLOR_TAN,
	"BEIGE",     	// SKINCOLOR_BEIGE,
	"ROSEBUSH",  	// SKINCOLOR_ROSEBUSH,
	"MOSS",     	// SKINCOLOR_MOSS,
	"AZURE",     	// SKINCOLOR_AZURE,
	"EGGPLANT",  	// SKINCOLOR_EGGPLANT,
	"LAVENDER",   	// SKINCOLOR_LAVENDER,

	// Viv's vivid colours (toast 21/07/17)
	// Tweaks & additions (Lach, sphere, Alice, MotorRoach 26/10/22)
	"RUBY",     	// SKINCOLOR_RUBY,
	"CHERRY",     	// SKINCOLOR_CHERRY,
	"SALMON",     	// SKINCOLOR_SALMON,
	"PEPPER",     	// SKINCOLOR_PEPPER,
	"RED",     		// SKINCOLOR_RED,
	"CRIMSON",     	// SKINCOLOR_CRIMSON,
	"FLAME",     	// SKINCOLOR_FLAME,
	"GARNET",      	// SKINCOLOR_GARNET,
	"KETCHUP",     	// SKINCOLOR_KETCHUP,
	"PEACHY",     	// SKINCOLOR_PEACHY,
	"QUAIL",     	// SKINCOLOR_QUAIL,
	"FOUNDATION",   // SKINCOLOR_FOUNDATION,
	"SUNSET",     	// SKINCOLOR_SUNSET,
	"COPPER",     	// SKINCOLOR_COPPER,
	"APRICOT",     	// SKINCOLOR_APRICOT,
	"ORANGE",     	// SKINCOLOR_ORANGE,
	"RUST",     	// SKINCOLOR_RUST,
	"TANGERINE",   	// SKINCOLOR_TANGERINE,
	"TOPAZ",     	// SKINCOLOR_TOPAZ,
	"GOLD",     	// SKINCOLOR_GOLD,
	"SANDY",     	// SKINCOLOR_SANDY,
	"GOLDENROD",   	// SKINCOLOR_GOLDENROD,
	"YELLOW",     	// SKINCOLOR_YELLOW,
	"OLIVE",     	// SKINCOLOR_OLIVE,
	"PEAR",     	// SKINCOLOR_PEAR,
	"LEMON",     	// SKINCOLOR_LEMON,
	"LIME",     	// SKINCOLOR_LIME,
	"PERIDOT",     	// SKINCOLOR_PERIDOT,
	"APPLE",     	// SKINCOLOR_APPLE,
	"HEADLIGHT",	// SKINCOLOR_HEADLIGHT,
	"CHARTREUSE",   // SKINCOLOR_CHARTREUSE,
	"GREEN",     	// SKINCOLOR_GREEN,
	"FOREST",     	// SKINCOLOR_FOREST,
	"SHAMROCK",    	// SKINCOLOR_SHAMROCK,
	"JADE",     	// SKINCOLOR_JADE,
	"MINT",     	// SKINCOLOR_MINT,
	"MASTER",     	// SKINCOLOR_MASTER,
	"EMERALD",     	// SKINCOLOR_EMERALD,
	"SEAFOAM",     	// SKINCOLOR_SEAFOAM,
	"ISLAND",     	// SKINCOLOR_ISLAND,
	"BOTTLE",     	// SKINCOLOR_BOTTLE,
	"AQUA",     	// SKINCOLOR_AQUA,
	"TEAL",     	// SKINCOLOR_TEAL,
	"OCEAN",     	// SKINCOLOR_OCEAN,
	"WAVE",     	// SKINCOLOR_WAVE,
	"CYAN",     	// SKINCOLOR_CYAN,
	"TURQUOISE",    // SKINCOLOR_TURQUOISE,
	"AQUAMARINE",  	// SKINCOLOR_AQUAMARINE,
	"SKY",     		// SKINCOLOR_SKY,
	"MARINE",     	// SKINCOLOR_MARINE,
	"CERULEAN",     // SKINCOLOR_CERULEAN,
	"DREAM",     	// SKINCOLOR_DREAM,
	"ICY",     		// SKINCOLOR_ICY,
	"DAYBREAK",     // SKINCOLOR_DAYBREAK,
	"SAPPHIRE",     // SKINCOLOR_SAPPHIRE,
	"ARCTIC",     	// SKINCOLOR_ARCTIC,
	"CORNFLOWER",   // SKINCOLOR_CORNFLOWER,
	"BLUE",     	// SKINCOLOR_BLUE,
	"COBALT",     	// SKINCOLOR_COBALT,
	"MIDNIGHT",     // SKINCOLOR_MIDNIGHT,
	"GALAXY",     	// SKINCOLOR_GALAXY,
	"VAPOR",     	// SKINCOLOR_VAPOR,
	"DUSK",     	// SKINCOLOR_DUSK,
	"MAJESTY",     	// SKINCOLOR_MAJESTY,
	"PASTEL",     	// SKINCOLOR_PASTEL,
	"PURPLE",     	// SKINCOLOR_PURPLE,
	"NOBLE",     	// SKINCOLOR_NOBLE,
	"FUCHSIA",     	// SKINCOLOR_FUCHSIA,
	"BUBBLEGUM",   	// SKINCOLOR_BUBBLEGUM,
	"SIBERITE",   	// SKINCOLOR_SIBERITE,
	"MAGENTA",     	// SKINCOLOR_MAGENTA,
	"NEON",     	// SKINCOLOR_NEON,
	"VIOLET",     	// SKINCOLOR_VIOLET,
	"ROYAL",     	// SKINCOLOR_ROYAL,
	"LILAC",     	// SKINCOLOR_LILAC,
	"MAUVE",     	// SKINCOLOR_MAUVE,
	"EVENTIDE",   	// SKINCOLOR_EVENTIDE,
	"PLUM",     	// SKINCOLOR_PLUM,
	"RASPBERRY",  	// SKINCOLOR_RASPBERRY,
	"TAFFY",     	// SKINCOLOR_TAFFY,
	"ROSY",     	// SKINCOLOR_ROSY,
	"FANCY",  		// SKINCOLOR_FANCY,
	"SANGRIA",     	// SKINCOLOR_SANGRIA,
	"VOLCANIC",    	// SKINCOLOR_VOLCANIC,

	// Super special awesome Super flashing colors!
	"SUPERSILVER1",	// SKINCOLOR_SUPERSILVER1
	"SUPERSILVER2",	// SKINCOLOR_SUPERSILVER2,
	"SUPERSILVER3",	// SKINCOLOR_SUPERSILVER3,
	"SUPERSILVER4",	// SKINCOLOR_SUPERSILVER4,
	"SUPERSILVER5",	// SKINCOLOR_SUPERSILVER5,

	"SUPERRED1",	// SKINCOLOR_SUPERRED1
	"SUPERRED2",	// SKINCOLOR_SUPERRED2,
	"SUPERRED3",	// SKINCOLOR_SUPERRED3,
	"SUPERRED4",	// SKINCOLOR_SUPERRED4,
	"SUPERRED5",	// SKINCOLOR_SUPERRED5,

	"SUPERORANGE1",	// SKINCOLOR_SUPERORANGE1
	"SUPERORANGE2",	// SKINCOLOR_SUPERORANGE2,
	"SUPERORANGE3",	// SKINCOLOR_SUPERORANGE3,
	"SUPERORANGE4",	// SKINCOLOR_SUPERORANGE4,
	"SUPERORANGE5",	// SKINCOLOR_SUPERORANGE5,

	"SUPERGOLD1",	// SKINCOLOR_SUPERGOLD1
	"SUPERGOLD2",	// SKINCOLOR_SUPERGOLD2,
	"SUPERGOLD3",	// SKINCOLOR_SUPERGOLD3,
	"SUPERGOLD4",	// SKINCOLOR_SUPERGOLD4,
	"SUPERGOLD5",	// SKINCOLOR_SUPERGOLD5,

	"SUPERPERIDOT1",	// SKINCOLOR_SUPERPERIDOT1
	"SUPERPERIDOT2",	// SKINCOLOR_SUPERPERIDOT2,
	"SUPERPERIDOT3",	// SKINCOLOR_SUPERPERIDOT3,
	"SUPERPERIDOT4",	// SKINCOLOR_SUPERPERIDOT4,
	"SUPERPERIDOT5",	// SKINCOLOR_SUPERPERIDOT5,

	"SUPERSKY1",	// SKINCOLOR_SUPERSKY1
	"SUPERSKY2",	// SKINCOLOR_SUPERSKY2,
	"SUPERSKY3",	// SKINCOLOR_SUPERSKY3,
	"SUPERSKY4",	// SKINCOLOR_SUPERSKY4,
	"SUPERSKY5",	// SKINCOLOR_SUPERSKY5,

	"SUPERPURPLE1",	// SKINCOLOR_SUPERPURPLE1,
	"SUPERPURPLE2",	// SKINCOLOR_SUPERPURPLE2,
	"SUPERPURPLE3",	// SKINCOLOR_SUPERPURPLE3,
	"SUPERPURPLE4",	// SKINCOLOR_SUPERPURPLE4,
	"SUPERPURPLE5",	// SKINCOLOR_SUPERPURPLE5,

	"SUPERRUST1",	// SKINCOLOR_SUPERRUST1
	"SUPERRUST2",	// SKINCOLOR_SUPERRUST2,
	"SUPERRUST3",	// SKINCOLOR_SUPERRUST3,
	"SUPERRUST4",	// SKINCOLOR_SUPERRUST4,
	"SUPERRUST5",	// SKINCOLOR_SUPERRUST5,

	"SUPERTAN1",	// SKINCOLOR_SUPERTAN1
	"SUPERTAN2",	// SKINCOLOR_SUPERTAN2,
	"SUPERTAN3",	// SKINCOLOR_SUPERTAN3,
	"SUPERTAN4",	// SKINCOLOR_SUPERTAN4,
	"SUPERTAN5"		// SKINCOLOR_SUPERTAN5,
};

const char *const POWERS_LIST[] = {
	"INVULNERABILITY",
	"SNEAKERS",
	"FLASHING",
	"SHIELD",
	"CARRY",
	"TAILSFLY", // tails flying
	"UNDERWATER", // underwater timer
	"SPACETIME", // In space, no one can hear you spin!
	"EXTRALIFE", // Extra Life timer
	"PUSHING",
	"JUSTSPRUNG",
	"NOAUTOBRAKE",

	"SUPER", // Are you super?
	"GRAVITYBOOTS", // gravity boots

	// Weapon ammunition
	"INFINITYRING",
	"AUTOMATICRING",
	"BOUNCERING",
	"SCATTERRING",
	"GRENADERING",
	"EXPLOSIONRING",
	"RAILRING",

	// Power Stones
	"EMERALDS", // stored like global 'emeralds' variable

	// NiGHTS powerups
	"NIGHTS_SUPERLOOP",
	"NIGHTS_HELPER",
	"NIGHTS_LINKFREEZE",

	//for linedef exec 427
	"NOCONTROL",

	//for dyes
	"DYE",

	"JUSTLAUNCHED",

	"IGNORELATCH",

	"STRONG"
};

const char *const HUDITEMS_LIST[] = {
	"LIVES",
	"INPUT",

	"RINGS",
	"RINGSNUM",
	"RINGSNUMTICS",

	"SCORE",
	"SCORENUM",

	"TIME",
	"MINUTES",
	"TIMECOLON",
	"SECONDS",
	"TIMETICCOLON",
	"TICS",

	"SS_TOTALRINGS",

	"GETRINGS",
	"GETRINGSNUM",
	"TIMELEFT",
	"TIMELEFTNUM",
	"TIMEUP",
	"HUNTPICS",
	"POWERUPS"
};

const char *const MENUTYPES_LIST[] = {
	"NONE",

	"MAIN",

	// Single Player
	"SP_MAIN",

	"SP_LOAD",
	"SP_PLAYER",

	"SP_LEVELSELECT",
	"SP_LEVELSTATS",

	"SP_TIMEATTACK",
	"SP_TIMEATTACK_LEVELSELECT",
	"SP_GUESTREPLAY",
	"SP_REPLAY",
	"SP_GHOST",

	"SP_NIGHTSATTACK",
	"SP_NIGHTS_LEVELSELECT",
	"SP_NIGHTS_GUESTREPLAY",
	"SP_NIGHTS_REPLAY",
	"SP_NIGHTS_GHOST",

	// Multiplayer
	"MP_MAIN",
	"MP_SPLITSCREEN", // SplitServer
	"MP_SERVER",
	"MP_CONNECT",
	"MP_ROOM",
	"MP_PLAYERSETUP",
	"MP_SERVER_OPTIONS",

	// Options
	"OP_MAIN",

	"OP_P1CONTROLS",
	"OP_CHANGECONTROLS", // OP_ChangeControlsDef shared with P2
	"OP_P1MOUSE",
	"OP_P1JOYSTICK",
	"OP_JOYSTICKSET", // OP_JoystickSetDef shared with P2
	"OP_P1CAMERA",

	"OP_P2CONTROLS",
	"OP_P2MOUSE",
	"OP_P2JOYSTICK",
	"OP_P2CAMERA",

	"OP_PLAYSTYLE",

	"OP_VIDEO",
	"OP_VIDEOMODE",
	"OP_COLOR",
	"OP_OPENGL",
	"OP_OPENGL_LIGHTING",

	"OP_SOUND",

	"OP_SERVER",
	"OP_MONITORTOGGLE",

	"OP_DATA",
	"OP_ADDONS",
	"OP_SCREENSHOTS",
	"OP_ERASEDATA",

	// Extras
	"SR_MAIN",
	"SR_PANDORA",
	"SR_LEVELSELECT",
	"SR_UNLOCKCHECKLIST",
	"SR_EMBLEMHINT",
	"SR_PLAYER",
	"SR_SOUNDTEST",

	// Addons (Part of MISC, but let's make it our own)
	"AD_MAIN",

	// MISC
	// "MESSAGE",
	// "SPAUSE",

	// "MPAUSE",
	// "SCRAMBLETEAM",
	// "CHANGETEAM",
	// "CHANGELEVEL",

	// "MAPAUSE",
	// "HELP",

	"SPECIAL"
};

struct int_const_s const INT_CONST[] = {
	// If a mod removes some variables here,
	// please leave the names in-tact and just set
	// the value to 0 or something.

	// integer type limits, from doomtype.h
	// INT64 and UINT64 limits not included, they're too big for most purposes anyway
	// signed
	{"INT8_MIN",INT8_MIN},
	{"INT16_MIN",INT16_MIN},
	{"INT32_MIN",INT32_MIN},
	{"INT8_MAX",INT8_MAX},
	{"INT16_MAX",INT16_MAX},
	{"INT32_MAX",INT32_MAX},
	// unsigned
	{"UINT8_MAX",UINT8_MAX},
	{"UINT16_MAX",UINT16_MAX},
	{"UINT32_MAX",UINT32_MAX},

	// fixed_t constants, from m_fixed.h
	{"FRACUNIT",FRACUNIT},
	{"FU"      ,FRACUNIT},
	{"FRACBITS",FRACBITS},

	// doomdef.h constants
	{"TICRATE",TICRATE},
	{"MUSICRATE",MUSICRATE},
	{"RING_DIST",RING_DIST},
	{"PUSHACCEL",PUSHACCEL},
	{"MODID",MODID}, // I don't know, I just thought it would be cool for a wad to potentially know what mod it was loaded into.
	{"MODVERSION",MODVERSION}, // or what version of the mod this is.
	{"CODEBASE",CODEBASE}, // or what release of SRB2 this is.
	{"NEWTICRATE",NEWTICRATE}, // TICRATE*NEWTICRATERATIO
	{"NEWTICRATERATIO",NEWTICRATERATIO},

	// Special linedef executor tag numbers!
	{"LE_PINCHPHASE",LE_PINCHPHASE}, // A boss entered pinch phase (and, in most cases, is preparing their pinch phase attack!)
	{"LE_ALLBOSSESDEAD",LE_ALLBOSSESDEAD}, // All bosses in the map are dead (Egg capsule raise)
	{"LE_BOSSDEAD",LE_BOSSDEAD}, // A boss in the map died (Chaos mode boss tally)
	{"LE_BOSS4DROP",LE_BOSS4DROP}, // CEZ boss dropped its cage
	{"LE_BRAKVILEATACK",LE_BRAKVILEATACK}, // Brak's doing his LOS attack, oh noes
	{"LE_TURRET",LE_TURRET}, // THZ turret
	{"LE_BRAKPLATFORM",LE_BRAKPLATFORM}, // v2.0 Black Eggman destroys platform
	{"LE_CAPSULE2",LE_CAPSULE2}, // Egg Capsule
	{"LE_CAPSULE1",LE_CAPSULE1}, // Egg Capsule
	{"LE_CAPSULE0",LE_CAPSULE0}, // Egg Capsule
	{"LE_KOOPA",LE_KOOPA}, // Distant cousin to Gay Bowser
	{"LE_AXE",LE_AXE}, // MKB Axe object
	{"LE_PARAMWIDTH",LE_PARAMWIDTH},  // If an object that calls LinedefExecute has a nonzero parameter value, this times the parameter will be subtracted. (Mostly for the purpose of coexisting bosses...)

	/// \todo Get all this stuff into its own sections, maybe. Maybe.

	// Frame settings
	{"FF_FRAMEMASK",FF_FRAMEMASK},
	{"FF_SPR2SUPER",FF_SPR2SUPER},
	{"FF_SPR2ENDSTATE",FF_SPR2ENDSTATE},
	{"FF_SPR2MIDSTART",FF_SPR2MIDSTART},
	{"FF_ANIMATE",FF_ANIMATE},
	{"FF_RANDOMANIM",FF_RANDOMANIM},
	{"FF_GLOBALANIM",FF_GLOBALANIM},
	{"FF_FULLBRIGHT",FF_FULLBRIGHT},
	{"FF_SEMIBRIGHT",FF_SEMIBRIGHT},
	{"FF_FULLDARK",FF_FULLDARK},
	{"FF_VERTICALFLIP",FF_VERTICALFLIP},
	{"FF_HORIZONTALFLIP",FF_HORIZONTALFLIP},
	{"FF_PAPERSPRITE",FF_PAPERSPRITE},
	{"FF_FLOORSPRITE",FF_FLOORSPRITE},
	{"FF_BLENDMASK",FF_BLENDMASK},
	{"FF_BLENDSHIFT",FF_BLENDSHIFT},
	{"FF_ADD",FF_ADD},
	{"FF_SUBTRACT",FF_SUBTRACT},
	{"FF_REVERSESUBTRACT",FF_REVERSESUBTRACT},
	{"FF_MODULATE",FF_MODULATE},
	{"FF_TRANSMASK",FF_TRANSMASK},
	{"FF_TRANSSHIFT",FF_TRANSSHIFT},
	// new preshifted translucency (used in source)
	{"FF_TRANS10",FF_TRANS10},
	{"FF_TRANS20",FF_TRANS20},
	{"FF_TRANS30",FF_TRANS30},
	{"FF_TRANS40",FF_TRANS40},
	{"FF_TRANS50",FF_TRANS50},
	{"FF_TRANS60",FF_TRANS60},
	{"FF_TRANS70",FF_TRANS70},
	{"FF_TRANS80",FF_TRANS80},
	{"FF_TRANS90",FF_TRANS90},
	// compatibility
	// Transparency for SOCs is pre-shifted
	{"TR_TRANS10",tr_trans10<<FF_TRANSSHIFT},
	{"TR_TRANS20",tr_trans20<<FF_TRANSSHIFT},
	{"TR_TRANS30",tr_trans30<<FF_TRANSSHIFT},
	{"TR_TRANS40",tr_trans40<<FF_TRANSSHIFT},
	{"TR_TRANS50",tr_trans50<<FF_TRANSSHIFT},
	{"TR_TRANS60",tr_trans60<<FF_TRANSSHIFT},
	{"TR_TRANS70",tr_trans70<<FF_TRANSSHIFT},
	{"TR_TRANS80",tr_trans80<<FF_TRANSSHIFT},
	{"TR_TRANS90",tr_trans90<<FF_TRANSSHIFT},
	// Transparency for Lua is not, unless capitalized as above.
	{"tr_trans10",tr_trans10},
	{"tr_trans20",tr_trans20},
	{"tr_trans30",tr_trans30},
	{"tr_trans40",tr_trans40},
	{"tr_trans50",tr_trans50},
	{"tr_trans60",tr_trans60},
	{"tr_trans70",tr_trans70},
	{"tr_trans80",tr_trans80},
	{"tr_trans90",tr_trans90},
	{"NUMTRANSMAPS",NUMTRANSMAPS},

	// Alpha styles (blend modes)
	{"AST_COPY",AST_COPY},
	{"AST_TRANSLUCENT",AST_TRANSLUCENT},
	{"AST_ADD",AST_ADD},
	{"AST_SUBTRACT",AST_SUBTRACT},
	{"AST_REVERSESUBTRACT",AST_REVERSESUBTRACT},
	{"AST_MODULATE",AST_MODULATE},
	{"AST_OVERLAY",AST_OVERLAY},
	{"AST_FOG",AST_FOG},

	// Render flags
	{"RF_HORIZONTALFLIP",RF_HORIZONTALFLIP},
	{"RF_VERTICALFLIP",RF_VERTICALFLIP},
	{"RF_ABSOLUTEOFFSETS",RF_ABSOLUTEOFFSETS},
	{"RF_FLIPOFFSETS",RF_FLIPOFFSETS},
	{"RF_SPLATMASK",RF_SPLATMASK},
	{"RF_SLOPESPLAT",RF_SLOPESPLAT},
	{"RF_OBJECTSLOPESPLAT",RF_OBJECTSLOPESPLAT},
	{"RF_NOSPLATBILLBOARD",RF_NOSPLATBILLBOARD},
	{"RF_NOSPLATROLLANGLE",RF_NOSPLATROLLANGLE},
	{"RF_BRIGHTMASK",RF_BRIGHTMASK},
	{"RF_FULLBRIGHT",RF_FULLBRIGHT},
	{"RF_FULLDARK",RF_FULLDARK},
	{"RF_SEMIBRIGHT",RF_SEMIBRIGHT},
	{"RF_NOCOLORMAPS",RF_NOCOLORMAPS},
	{"RF_SPRITETYPEMASK",RF_SPRITETYPEMASK},
	{"RF_PAPERSPRITE",RF_PAPERSPRITE},
	{"RF_FLOORSPRITE",RF_FLOORSPRITE},
	{"RF_SHADOWDRAW",RF_SHADOWDRAW},
	{"RF_SHADOWEFFECTS",RF_SHADOWEFFECTS},
	{"RF_DROPSHADOW",RF_DROPSHADOW},

	// Animation flags
	{"SPR2F_MASK",SPR2F_MASK},
	{"SPR2F_SUPER",SPR2F_SUPER},

	// Level flags
	{"LF_SCRIPTISFILE",LF_SCRIPTISFILE},
	{"LF_SPEEDMUSIC",LF_SPEEDMUSIC},
	{"LF_NOSSMUSIC",LF_NOSSMUSIC},
	{"LF_NORELOAD",LF_NORELOAD},
	{"LF_NOZONE",LF_NOZONE},
	{"LF_SAVEGAME",LF_SAVEGAME},
	{"LF_MIXNIGHTSCOUNTDOWN",LF_MIXNIGHTSCOUNTDOWN},
	{"LF_NOTITLECARDFIRST",LF_NOTITLECARDFIRST},
	{"LF_NOTITLECARDRESPAWN",LF_NOTITLECARDRESPAWN},
	{"LF_NOTITLECARDRECORDATTACK",LF_NOTITLECARDRECORDATTACK},
	{"LF_NOTITLECARD",LF_NOTITLECARD},
	{"LF_WARNINGTITLE",LF_WARNINGTITLE},
	// And map flags
	{"LF2_HIDEINMENU",LF2_HIDEINMENU},
	{"LF2_HIDEINSTATS",LF2_HIDEINSTATS},
	{"LF2_RECORDATTACK",LF2_RECORDATTACK},
	{"LF2_NIGHTSATTACK",LF2_NIGHTSATTACK},
	{"LF2_NOVISITNEEDED",LF2_NOVISITNEEDED},
	{"LF2_WIDEICON",LF2_WIDEICON},

	// Emeralds
	{"EMERALD1",EMERALD1},
	{"EMERALD2",EMERALD2},
	{"EMERALD3",EMERALD3},
	{"EMERALD4",EMERALD4},
	{"EMERALD5",EMERALD5},
	{"EMERALD6",EMERALD6},
	{"EMERALD7",EMERALD7},

	// SKINCOLOR_ doesn't include these..!
	{"MAXSKINCOLORS",MAXSKINCOLORS},
	{"FIRSTSUPERCOLOR",FIRSTSUPERCOLOR},
	{"NUMSUPERCOLORS",NUMSUPERCOLORS},

	// Precipitation
	{"PRECIP_NONE",PRECIP_NONE},
	{"PRECIP_STORM",PRECIP_STORM},
	{"PRECIP_SNOW",PRECIP_SNOW},
	{"PRECIP_RAIN",PRECIP_RAIN},
	{"PRECIP_BLANK",PRECIP_BLANK},
	{"PRECIP_STORM_NORAIN",PRECIP_STORM_NORAIN},
	{"PRECIP_STORM_NOSTRIKES",PRECIP_STORM_NOSTRIKES},

	// Shields
	{"SH_NONE",SH_NONE},
	// Shield flags
	{"SH_PROTECTFIRE",SH_PROTECTFIRE},
	{"SH_PROTECTWATER",SH_PROTECTWATER},
	{"SH_PROTECTELECTRIC",SH_PROTECTELECTRIC},
	{"SH_PROTECTSPIKE",SH_PROTECTSPIKE},
	// Indivisible shields
	{"SH_PITY",SH_PITY},
	{"SH_WHIRLWIND",SH_WHIRLWIND},
	{"SH_ARMAGEDDON",SH_ARMAGEDDON},
	{"SH_PINK",SH_PINK},
	// normal shields that use flags
	{"SH_ATTRACT",SH_ATTRACT},
	{"SH_ELEMENTAL",SH_ELEMENTAL},
	// Sonic 3 shields
	{"SH_FLAMEAURA",SH_FLAMEAURA},
	{"SH_BUBBLEWRAP",SH_BUBBLEWRAP},
	{"SH_THUNDERCOIN",SH_THUNDERCOIN},
	// The force shield uses the lower 8 bits to count how many extra hits are left.
	{"SH_FORCE",SH_FORCE},
	{"SH_FORCEHP",SH_FORCEHP}, // to be used as a bitmask only
	// Mostly for use with Mario mode.
	{"SH_FIREFLOWER",SH_FIREFLOWER},
	{"SH_STACK",SH_STACK},
	{"SH_NOSTACK",SH_NOSTACK},

	// Carrying
	{"CR_NONE",CR_NONE},
	{"CR_GENERIC",CR_GENERIC},
	{"CR_PLAYER",CR_PLAYER},
	{"CR_NIGHTSMODE",CR_NIGHTSMODE},
	{"CR_NIGHTSFALL",CR_NIGHTSFALL},
	{"CR_BRAKGOOP",CR_BRAKGOOP},
	{"CR_ZOOMTUBE",CR_ZOOMTUBE},
	{"CR_ROPEHANG",CR_ROPEHANG},
	{"CR_MACESPIN",CR_MACESPIN},
	{"CR_MINECART",CR_MINECART},
	{"CR_ROLLOUT",CR_ROLLOUT},
	{"CR_PTERABYTE",CR_PTERABYTE},
	{"CR_DUSTDEVIL",CR_DUSTDEVIL},
	{"CR_FAN",CR_FAN},

	// Strong powers
	{"STR_NONE",STR_NONE},
	{"STR_ANIM",STR_ANIM},
	{"STR_PUNCH",STR_PUNCH},
	{"STR_TAIL",STR_TAIL},
	{"STR_STOMP",STR_STOMP},
	{"STR_UPPER",STR_UPPER},
	{"STR_GUARD",STR_GUARD},
	{"STR_HEAVY",STR_HEAVY},
	{"STR_DASH",STR_DASH},
	{"STR_WALL",STR_WALL},
	{"STR_FLOOR",STR_FLOOR},
	{"STR_CEILING",STR_CEILING},
	{"STR_SPRING",STR_SPRING},
	{"STR_SPIKE",STR_SPIKE},
	{"STR_ATTACK",STR_ATTACK},
	{"STR_BUST",STR_BUST},
	{"STR_FLY",STR_FLY},
	{"STR_GLIDE",STR_GLIDE},
	{"STR_TWINSPIN",STR_TWINSPIN},
	{"STR_MELEE",STR_MELEE},
	{"STR_BOUNCE",STR_BOUNCE},
	{"STR_METAL",STR_METAL},

	// Ring weapons (ringweapons_t)
	// Useful for A_GiveWeapon
	{"RW_AUTO",RW_AUTO},
	{"RW_BOUNCE",RW_BOUNCE},
	{"RW_SCATTER",RW_SCATTER},
	{"RW_GRENADE",RW_GRENADE},
	{"RW_EXPLODE",RW_EXPLODE},
	{"RW_RAIL",RW_RAIL},

	// Character flags (skinflags_t)
	{"SF_SUPER",SF_SUPER},
	{"SF_NOSUPERSPIN",SF_NOSUPERSPIN},
	{"SF_NOSPINDASHDUST",SF_NOSPINDASHDUST},
	{"SF_HIRES",SF_HIRES},
	{"SF_NOSKID",SF_NOSKID},
	{"SF_NOSPEEDADJUST",SF_NOSPEEDADJUST},
	{"SF_RUNONWATER",SF_RUNONWATER},
	{"SF_NOJUMPSPIN",SF_NOJUMPSPIN},
	{"SF_NOJUMPDAMAGE",SF_NOJUMPDAMAGE},
	{"SF_STOMPDAMAGE",SF_STOMPDAMAGE},
	{"SF_MARIODAMAGE",SF_MARIODAMAGE},
	{"SF_MACHINE",SF_MACHINE},
	{"SF_DASHMODE",SF_DASHMODE},
	{"SF_FASTEDGE",SF_FASTEDGE},
	{"SF_MULTIABILITY",SF_MULTIABILITY},
	{"SF_NONIGHTSROTATION",SF_NONIGHTSROTATION},
	{"SF_NONIGHTSSUPER",SF_NONIGHTSSUPER},
	{"SF_NOSUPERSPRITES",SF_NOSUPERSPRITES},
	{"SF_NOSUPERJUMPBOOST",SF_NOSUPERJUMPBOOST},
	{"SF_CANBUSTWALLS",SF_CANBUSTWALLS},
	{"SF_NOSHIELDABILITY",SF_NOSHIELDABILITY},

	// Dashmode constants
	{"DASHMODE_THRESHOLD",DASHMODE_THRESHOLD},
	{"DASHMODE_MAX",DASHMODE_MAX},

	// Character abilities!
	// Primary
	{"CA_NONE",CA_NONE}, // now slot 0!
	{"CA_THOK",CA_THOK},
	{"CA_FLY",CA_FLY},
	{"CA_GLIDEANDCLIMB",CA_GLIDEANDCLIMB},
	{"CA_HOMINGTHOK",CA_HOMINGTHOK},
	{"CA_DOUBLEJUMP",CA_DOUBLEJUMP},
	{"CA_FLOAT",CA_FLOAT},
	{"CA_SLOWFALL",CA_SLOWFALL},
	{"CA_SWIM",CA_SWIM},
	{"CA_TELEKINESIS",CA_TELEKINESIS},
	{"CA_FALLSWITCH",CA_FALLSWITCH},
	{"CA_JUMPBOOST",CA_JUMPBOOST},
	{"CA_AIRDRILL",CA_AIRDRILL},
	{"CA_JUMPTHOK",CA_JUMPTHOK},
	{"CA_BOUNCE",CA_BOUNCE},
	{"CA_TWINSPIN",CA_TWINSPIN},
	// Secondary
	{"CA2_NONE",CA2_NONE}, // now slot 0!
	{"CA2_SPINDASH",CA2_SPINDASH},
	{"CA2_GUNSLINGER",CA2_GUNSLINGER},
	{"CA2_MELEE",CA2_MELEE},

	// Sound flags
	{"SF_TOTALLYSINGLE",SF_TOTALLYSINGLE},
	{"SF_NOMULTIPLESOUND",SF_NOMULTIPLESOUND},
	{"SF_OUTSIDESOUND",SF_OUTSIDESOUND},
	{"SF_X4AWAYSOUND",SF_X4AWAYSOUND},
	{"SF_X8AWAYSOUND",SF_X8AWAYSOUND},
	{"SF_NOINTERRUPT",SF_NOINTERRUPT},
	{"SF_X2AWAYSOUND",SF_X2AWAYSOUND},

	// Global emblem var flags
	{"GE_NIGHTSPULL",GE_NIGHTSPULL},
	{"GE_NIGHTSITEM",GE_NIGHTSITEM},

	// Map emblem var flags
	{"ME_ALLEMERALDS",ME_ALLEMERALDS},
	{"ME_ULTIMATE",ME_ULTIMATE},
	{"ME_PERFECT",ME_PERFECT},

	// p_local.h constants
	{"FLOATSPEED",FLOATSPEED},
	{"MAXSTEPMOVE",MAXSTEPMOVE},
	{"USERANGE",USERANGE},
	{"MELEERANGE",MELEERANGE},
	{"MISSILERANGE",MISSILERANGE},
	{"ONFLOORZ",ONFLOORZ}, // INT32_MIN
	{"ONCEILINGZ",ONCEILINGZ}, //INT32_MAX
	// for P_FlashPal
	{"PAL_WHITE",PAL_WHITE},
	{"PAL_MIXUP",PAL_MIXUP},
	{"PAL_RECYCLE",PAL_RECYCLE},
	{"PAL_NUKE",PAL_NUKE},
	{"PAL_INVERT",PAL_INVERT},
	// for P_DamageMobj
	//// Damage types
	{"DMG_WATER",DMG_WATER},
	{"DMG_FIRE",DMG_FIRE},
	{"DMG_ELECTRIC",DMG_ELECTRIC},
	{"DMG_SPIKE",DMG_SPIKE},
	{"DMG_NUKE",DMG_NUKE},
	//// Death types
	{"DMG_INSTAKILL",DMG_INSTAKILL},
	{"DMG_DROWNED",DMG_DROWNED},
	{"DMG_SPACEDROWN",DMG_SPACEDROWN},
	{"DMG_DEATHPIT",DMG_DEATHPIT},
	{"DMG_CRUSHED",DMG_CRUSHED},
	{"DMG_SPECTATOR",DMG_SPECTATOR},
	//// Masks
	{"DMG_CANHURTSELF",DMG_CANHURTSELF},
	{"DMG_DEATHMASK",DMG_DEATHMASK},

	// Intermission types
	{"int_none",int_none},
	{"int_coop",int_coop},
	{"int_match",int_match},
	{"int_teammatch",int_teammatch},
	//{"int_tag",int_tag},
	{"int_ctf",int_ctf},
	{"int_spec",int_spec},
	{"int_race",int_race},
	{"int_comp",int_comp},

	// Jingles (jingletype_t)
	{"JT_NONE",JT_NONE},
	{"JT_OTHER",JT_OTHER},
	{"JT_MASTER",JT_MASTER},
	{"JT_1UP",JT_1UP},
	{"JT_SHOES",JT_SHOES},
	{"JT_INV",JT_INV},
	{"JT_MINV",JT_MINV},
	{"JT_DROWN",JT_DROWN},
	{"JT_SUPER",JT_SUPER},
	{"JT_GOVER",JT_GOVER},
	{"JT_NIGHTSTIMEOUT",JT_NIGHTSTIMEOUT},
	{"JT_SSTIMEOUT",JT_SSTIMEOUT},
	// {"JT_LCLEAR",JT_LCLEAR},
	// {"JT_RACENT",JT_RACENT},
	// {"JT_CONTSC",JT_CONTSC},

	// Player state (playerstate_t)
	{"PST_LIVE",PST_LIVE}, // Playing or camping.
	{"PST_DEAD",PST_DEAD}, // Dead on the ground, view follows killer.
	{"PST_REBORN",PST_REBORN}, // Ready to restart/respawn???

	// Player animation (panim_t)
	{"PA_ETC",PA_ETC},
	{"PA_IDLE",PA_IDLE},
	{"PA_EDGE",PA_EDGE},
	{"PA_WALK",PA_WALK},
	{"PA_RUN",PA_RUN},
	{"PA_DASH",PA_DASH},
	{"PA_PAIN",PA_PAIN},
	{"PA_ROLL",PA_ROLL},
	{"PA_JUMP",PA_JUMP},
	{"PA_SPRING",PA_SPRING},
	{"PA_FALL",PA_FALL},
	{"PA_ABILITY",PA_ABILITY},
	{"PA_ABILITY2",PA_ABILITY2},
	{"PA_RIDE",PA_RIDE},

	// Current weapon
	{"WEP_AUTO",WEP_AUTO},
	{"WEP_BOUNCE",WEP_BOUNCE},
	{"WEP_SCATTER",WEP_SCATTER},
	{"WEP_GRENADE",WEP_GRENADE},
	{"WEP_EXPLODE",WEP_EXPLODE},
	{"WEP_RAIL",WEP_RAIL},
	{"NUM_WEAPONS",NUM_WEAPONS},

	// Value for infinite lives
	{"INFLIVES",INFLIVES},

	// Got Flags, for player->gotflag!
	// Used to be MF_ for some stupid reason, now they're GF_ to stop them looking like mobjflags
	{"GF_REDFLAG",GF_REDFLAG},
	{"GF_BLUEFLAG",GF_BLUEFLAG},

	// Bot types
	{"BOT_NONE",BOT_NONE},
	{"BOT_2PAI",BOT_2PAI},
	{"BOT_2PHUMAN",BOT_2PHUMAN},
	{"BOT_MPAI",BOT_MPAI},

	// Customisable sounds for Skins, from sounds.h
	{"SKSSPIN",SKSSPIN},
	{"SKSPUTPUT",SKSPUTPUT},
	{"SKSPUDPUD",SKSPUDPUD},
	{"SKSPLPAN1",SKSPLPAN1}, // Ouchies
	{"SKSPLPAN2",SKSPLPAN2},
	{"SKSPLPAN3",SKSPLPAN3},
	{"SKSPLPAN4",SKSPLPAN4},
	{"SKSPLDET1",SKSPLDET1}, // Deaths
	{"SKSPLDET2",SKSPLDET2},
	{"SKSPLDET3",SKSPLDET3},
	{"SKSPLDET4",SKSPLDET4},
	{"SKSPLVCT1",SKSPLVCT1}, // Victories
	{"SKSPLVCT2",SKSPLVCT2},
	{"SKSPLVCT3",SKSPLVCT3},
	{"SKSPLVCT4",SKSPLVCT4},
	{"SKSTHOK",SKSTHOK},
	{"SKSSPNDSH",SKSSPNDSH},
	{"SKSZOOM",SKSZOOM},
	{"SKSSKID",SKSSKID},
	{"SKSGASP",SKSGASP},
	{"SKSJUMP",SKSJUMP},

	// 3D Floor/Fake Floor/FOF/whatever flags
	{"FOF_EXISTS",FOF_EXISTS},                   ///< Always set, to check for validity.
	{"FOF_BLOCKPLAYER",FOF_BLOCKPLAYER},         ///< Solid to player, but nothing else
	{"FOF_BLOCKOTHERS",FOF_BLOCKOTHERS},         ///< Solid to everything but player
	{"FOF_SOLID",FOF_SOLID},                     ///< Clips things.
	{"FOF_RENDERSIDES",FOF_RENDERSIDES},         ///< Renders the sides.
	{"FOF_RENDERPLANES",FOF_RENDERPLANES},       ///< Renders the floor/ceiling.
	{"FOF_RENDERALL",FOF_RENDERALL},             ///< Renders everything.
	{"FOF_SWIMMABLE",FOF_SWIMMABLE},             ///< Is a water block.
	{"FOF_NOSHADE",FOF_NOSHADE},                 ///< Messes with the lighting?
	{"FOF_CUTSOLIDS",FOF_CUTSOLIDS},             ///< Cuts out hidden solid pixels.
	{"FOF_CUTEXTRA",FOF_CUTEXTRA},               ///< Cuts out hidden translucent pixels.
	{"FOF_CUTLEVEL",FOF_CUTLEVEL},               ///< Cuts out all hidden pixels.
	{"FOF_CUTSPRITES",FOF_CUTSPRITES},           ///< Final step in making 3D water.
	{"FOF_BOTHPLANES",FOF_BOTHPLANES},           ///< Render inside and outside planes.
	{"FOF_EXTRA",FOF_EXTRA},                     ///< Gets cut by ::FOF_CUTEXTRA.
	{"FOF_TRANSLUCENT",FOF_TRANSLUCENT},         ///< See through!
	{"FOF_FOG",FOF_FOG},                         ///< Fog "brush."
	{"FOF_INVERTPLANES",FOF_INVERTPLANES},       ///< Only render inside planes.
	{"FOF_ALLSIDES",FOF_ALLSIDES},               ///< Render inside and outside sides.
	{"FOF_INVERTSIDES",FOF_INVERTSIDES},         ///< Only render inside sides.
	{"FOF_DOUBLESHADOW",FOF_DOUBLESHADOW},       ///< Make two lightlist entries to reset light?
	{"FOF_FLOATBOB",FOF_FLOATBOB},               ///< Floats on water and bobs if you step on it.
	{"FOF_NORETURN",FOF_NORETURN},               ///< Used with ::FOF_CRUMBLE. Will not return to its original position after falling.
	{"FOF_CRUMBLE",FOF_CRUMBLE},                 ///< Falls 2 seconds after being stepped on, and randomly brings all touching crumbling 3dfloors down with it, providing their master sectors share the same tag (allows crumble platforms above or below, to also exist).
	{"FOF_GOOWATER",FOF_GOOWATER},               ///< Used with ::FOF_SWIMMABLE. Makes thick bouncey goop.
	{"FOF_MARIO",FOF_MARIO},                     ///< Acts like a question block when hit from underneath. Goodie spawned at top is determined by master sector.
	{"FOF_BUSTUP",FOF_BUSTUP},                   ///< You can spin through/punch this block and it will crumble!
	{"FOF_QUICKSAND",FOF_QUICKSAND},             ///< Quicksand!
	{"FOF_PLATFORM",FOF_PLATFORM},               ///< You can jump up through this to the top.
	{"FOF_REVERSEPLATFORM",FOF_REVERSEPLATFORM}, ///< A fall-through floor in normal gravity, a platform in reverse gravity.
	{"FOF_INTANGIBLEFLATS",FOF_INTANGIBLEFLATS}, ///< Both flats are intangible, but the sides are still solid.
	{"FOF_RIPPLE",FOF_RIPPLE},                   ///< Ripple the flats
	{"FOF_COLORMAPONLY",FOF_COLORMAPONLY},       ///< Only copy the colormap, not the lightlevel
	{"FOF_BOUNCY",FOF_BOUNCY},                   ///< Bounces players
	{"FOF_SPLAT",FOF_SPLAT},                     ///< Use splat flat renderer (treat cyan pixels as invisible)

	// Old FOF flags for backwards compatibility
	{"FF_EXISTS",FF_OLD_EXISTS},
	{"FF_BLOCKPLAYER",FF_OLD_BLOCKPLAYER},
	{"FF_BLOCKOTHERS",FF_OLD_BLOCKOTHERS},
	{"FF_SOLID",FF_OLD_SOLID},
	{"FF_RENDERSIDES",FF_OLD_RENDERSIDES},
	{"FF_RENDERPLANES",FF_OLD_RENDERPLANES},
	{"FF_RENDERALL",FF_OLD_RENDERALL},
	{"FF_SWIMMABLE",FF_OLD_SWIMMABLE},
	{"FF_NOSHADE",FF_OLD_NOSHADE},
	{"FF_CUTSOLIDS",FF_OLD_CUTSOLIDS},
	{"FF_CUTEXTRA",FF_OLD_CUTEXTRA},
	{"FF_CUTLEVEL",FF_OLD_CUTLEVEL},
	{"FF_CUTSPRITES",FF_OLD_CUTSPRITES},
	{"FF_BOTHPLANES",FF_OLD_BOTHPLANES},
	{"FF_EXTRA",FF_OLD_EXTRA},
	{"FF_TRANSLUCENT",FF_OLD_TRANSLUCENT},
	{"FF_FOG",FF_OLD_FOG},
	{"FF_INVERTPLANES",FF_OLD_INVERTPLANES},
	{"FF_ALLSIDES",FF_OLD_ALLSIDES},
	{"FF_INVERTSIDES",FF_OLD_INVERTSIDES},
	{"FF_DOUBLESHADOW",FF_OLD_DOUBLESHADOW},
	{"FF_FLOATBOB",FF_OLD_FLOATBOB},
	{"FF_NORETURN",FF_OLD_NORETURN},
	{"FF_CRUMBLE",FF_OLD_CRUMBLE},
	{"FF_SHATTERBOTTOM",FF_OLD_SHATTERBOTTOM},
	{"FF_GOOWATER",FF_OLD_GOOWATER},
	{"FF_MARIO",FF_OLD_MARIO},
	{"FF_BUSTUP",FF_OLD_BUSTUP},
	{"FF_QUICKSAND",FF_OLD_QUICKSAND},
	{"FF_PLATFORM",FF_OLD_PLATFORM},
	{"FF_REVERSEPLATFORM",FF_OLD_REVERSEPLATFORM},
	{"FF_INTANGIBLEFLATS",FF_OLD_INTANGIBLEFLATS},
	{"FF_INTANGABLEFLATS",FF_OLD_INTANGIBLEFLATS},
	{"FF_SHATTER",FF_OLD_SHATTER},
	{"FF_SPINBUST",FF_OLD_SPINBUST},
	{"FF_STRONGBUST",FF_OLD_STRONGBUST},
	{"FF_RIPPLE",FF_OLD_RIPPLE},
	{"FF_COLORMAPONLY",FF_OLD_COLORMAPONLY},

	// FOF bustable flags
	{"FB_PUSHABLES",FB_PUSHABLES},
	{"FB_EXECUTOR",FB_EXECUTOR},
	{"FB_ONLYBOTTOM",FB_ONLYBOTTOM},

	// Bustable FOF type
	{"BT_TOUCH",BT_TOUCH},
	{"BT_SPINBUST",BT_SPINBUST},
	{"BT_REGULAR",BT_REGULAR},
	{"BT_STRONG",BT_STRONG},

	// PolyObject flags
	{"POF_CLIPLINES",POF_CLIPLINES},               ///< Test against lines for collision
	{"POF_CLIPPLANES",POF_CLIPPLANES},             ///< Test against tops and bottoms for collision
	{"POF_SOLID",POF_SOLID},                       ///< Clips things.
	{"POF_TESTHEIGHT",POF_TESTHEIGHT},             ///< Test line collision with heights
	{"POF_RENDERSIDES",POF_RENDERSIDES},           ///< Renders the sides.
	{"POF_RENDERTOP",POF_RENDERTOP},               ///< Renders the top.
	{"POF_RENDERBOTTOM",POF_RENDERBOTTOM},         ///< Renders the bottom.
	{"POF_RENDERPLANES",POF_RENDERPLANES},         ///< Renders top and bottom.
	{"POF_RENDERALL",POF_RENDERALL},               ///< Renders everything.
	{"POF_INVERT",POF_INVERT},                     ///< Inverts collision (like a cage).
	{"POF_INVERTPLANES",POF_INVERTPLANES},         ///< Render inside planes.
	{"POF_INVERTPLANESONLY",POF_INVERTPLANESONLY}, ///< Only render inside planes.
	{"POF_PUSHABLESTOP",POF_PUSHABLESTOP},         ///< Pushables will stop movement.
	{"POF_LDEXEC",POF_LDEXEC},                     ///< This PO triggers a linedef executor.
	{"POF_ONESIDE",POF_ONESIDE},                   ///< Only use the first side of the linedef.
	{"POF_NOSPECIALS",POF_NOSPECIALS},             ///< Don't apply sector specials.
	{"POF_SPLAT",POF_SPLAT},                       ///< Use splat flat renderer (treat cyan pixels as invisible).

#ifdef HAVE_LUA_SEGS
	// Node flags
	{"NF_SUBSECTOR",NF_SUBSECTOR}, // Indicate a leaf.
#endif

	// Slope flags
	{"SL_NOPHYSICS",SL_NOPHYSICS},
	{"SL_DYNAMIC",SL_DYNAMIC},

	// Angles
	{"ANG1",ANG1},
	{"ANG2",ANG2},
	{"ANG10",ANG10},
	{"ANG15",ANG15},
	{"ANG20",ANG20},
	{"ANG30",ANG30},
	{"ANG60",ANG60},
	{"ANG64h",ANG64h},
	{"ANG105",ANG105},
	{"ANG210",ANG210},
	{"ANG255",ANG255},
	{"ANG340",ANG340},
	{"ANG350",ANG350},
	{"ANGLE_11hh",ANGLE_11hh},
	{"ANGLE_22h",ANGLE_22h},
	{"ANGLE_45",ANGLE_45},
	{"ANGLE_67h",ANGLE_67h},
	{"ANGLE_90",ANGLE_90},
	{"ANGLE_112h",ANGLE_112h},
	{"ANGLE_135",ANGLE_135},
	{"ANGLE_157h",ANGLE_157h},
	{"ANGLE_180",ANGLE_180},
	{"ANGLE_202h",ANGLE_202h},
	{"ANGLE_225",ANGLE_225},
	{"ANGLE_247h",ANGLE_247h},
	{"ANGLE_270",ANGLE_270},
	{"ANGLE_292h",ANGLE_292h},
	{"ANGLE_315",ANGLE_315},
	{"ANGLE_337h",ANGLE_337h},
	{"ANGLE_MAX",ANGLE_MAX},

	// P_Chase directions (dirtype_t)
	{"DI_NODIR",DI_NODIR},
	{"DI_EAST",DI_EAST},
	{"DI_NORTHEAST",DI_NORTHEAST},
	{"DI_NORTH",DI_NORTH},
	{"DI_NORTHWEST",DI_NORTHWEST},
	{"DI_WEST",DI_WEST},
	{"DI_SOUTHWEST",DI_SOUTHWEST},
	{"DI_SOUTH",DI_SOUTH},
	{"DI_SOUTHEAST",DI_SOUTHEAST},
	{"NUMDIRS",NUMDIRS},

	// Sprite rotation axis (rotaxis_t)
	{"ROTAXIS_X",ROTAXIS_X},
	{"ROTAXIS_Y",ROTAXIS_Y},
	{"ROTAXIS_Z",ROTAXIS_Z},

	// Buttons (ticcmd_t)
	{"BT_WEAPONMASK",BT_WEAPONMASK}, //our first three bits.
	{"BT_SHIELD",BT_SHIELD},
	{"BT_WEAPONNEXT",BT_WEAPONNEXT},
	{"BT_WEAPONPREV",BT_WEAPONPREV},
	{"BT_ATTACK",BT_ATTACK}, // shoot rings
	{"BT_SPIN",BT_SPIN},
	{"BT_CAMLEFT",BT_CAMLEFT}, // turn camera left
	{"BT_CAMRIGHT",BT_CAMRIGHT}, // turn camera right
	{"BT_TOSSFLAG",BT_TOSSFLAG},
	{"BT_JUMP",BT_JUMP},
	{"BT_FIRENORMAL",BT_FIRENORMAL}, // Fire a normal ring no matter what
	{"BT_CUSTOM1",BT_CUSTOM1}, // Lua customizable
	{"BT_CUSTOM2",BT_CUSTOM2}, // Lua customizable
	{"BT_CUSTOM3",BT_CUSTOM3}, // Lua customizable

	// Lua command registration flags
	{"COM_ADMIN",COM_ADMIN},
	{"COM_SPLITSCREEN",COM_SPLITSCREEN},
	{"COM_LOCAL",COM_LOCAL},

	// cvflags_t
	{"CV_SAVE",CV_SAVE},
	{"CV_CALL",CV_CALL},
	{"CV_NETVAR",CV_NETVAR},
	{"CV_NOINIT",CV_NOINIT},
	{"CV_FLOAT",CV_FLOAT},
	{"CV_NOTINNET",CV_NOTINNET},
	{"CV_MODIFIED",CV_MODIFIED},
	{"CV_SHOWMODIF",CV_SHOWMODIF},
	{"CV_SHOWMODIFONETIME",CV_SHOWMODIFONETIME},
	{"CV_NOSHOWHELP",CV_NOSHOWHELP},
	{"CV_HIDEN",CV_HIDEN},
	{"CV_HIDDEN",CV_HIDEN},
	{"CV_CHEAT",CV_CHEAT},
	{"CV_ALLOWLUA",CV_ALLOWLUA},

	// v_video flags
	{"V_NOSCALEPATCH",V_NOSCALEPATCH},
	{"V_SMALLSCALEPATCH",V_SMALLSCALEPATCH},
	{"V_MEDSCALEPATCH",V_MEDSCALEPATCH},
	{"V_6WIDTHSPACE",V_6WIDTHSPACE},
	{"V_OLDSPACING",V_OLDSPACING},
	{"V_MONOSPACE",V_MONOSPACE},

	{"V_MAGENTAMAP",V_MAGENTAMAP},
	{"V_YELLOWMAP",V_YELLOWMAP},
	{"V_GREENMAP",V_GREENMAP},
	{"V_BLUEMAP",V_BLUEMAP},
	{"V_REDMAP",V_REDMAP},
	{"V_GRAYMAP",V_GRAYMAP},
	{"V_ORANGEMAP",V_ORANGEMAP},
	{"V_SKYMAP",V_SKYMAP},
	{"V_PURPLEMAP",V_PURPLEMAP},
	{"V_AQUAMAP",V_AQUAMAP},
	{"V_PERIDOTMAP",V_PERIDOTMAP},
	{"V_AZUREMAP",V_AZUREMAP},
	{"V_BROWNMAP",V_BROWNMAP},
	{"V_ROSYMAP",V_ROSYMAP},
	{"V_INVERTMAP",V_INVERTMAP},

	{"V_TRANSLUCENT",V_TRANSLUCENT},
	{"V_10TRANS",V_10TRANS},
	{"V_20TRANS",V_20TRANS},
	{"V_30TRANS",V_30TRANS},
	{"V_40TRANS",V_40TRANS},
	{"V_50TRANS",V_TRANSLUCENT}, // alias
	{"V_60TRANS",V_60TRANS},
	{"V_70TRANS",V_70TRANS},
	{"V_80TRANS",V_80TRANS},
	{"V_90TRANS",V_90TRANS},
	{"V_HUDTRANSHALF",V_HUDTRANSHALF},
	{"V_HUDTRANS",V_HUDTRANS},
	{"V_HUDTRANSDOUBLE",V_HUDTRANSDOUBLE},
	{"V_BLENDSHIFT",V_BLENDSHIFT},
	{"V_BLENDMASK",V_BLENDMASK},
	{"V_ADD",V_ADD},
	{"V_SUBTRACT",V_SUBTRACT},
	{"V_REVERSESUBTRACT",V_REVERSESUBTRACT},
	{"V_MODULATE",V_MODULATE},
	{"V_ALLOWLOWERCASE",V_ALLOWLOWERCASE},
	{"V_FLIP",V_FLIP},
	{"V_CENTERNAMETAG",V_CENTERNAMETAG},
	{"V_SNAPTOTOP",V_SNAPTOTOP},
	{"V_SNAPTOBOTTOM",V_SNAPTOBOTTOM},
	{"V_SNAPTOLEFT",V_SNAPTOLEFT},
	{"V_SNAPTORIGHT",V_SNAPTORIGHT},
	{"V_AUTOFADEOUT",V_AUTOFADEOUT},
	{"V_RETURN8",V_RETURN8},
	{"V_NOSCALESTART",V_NOSCALESTART},
	{"V_PERPLAYER",V_PERPLAYER},

	{"V_PARAMMASK",V_PARAMMASK},
	{"V_SCALEPATCHMASK",V_SCALEPATCHMASK},
	{"V_SPACINGMASK",V_SPACINGMASK},
	{"V_CHARCOLORMASK",V_CHARCOLORMASK},
	{"V_ALPHAMASK",V_ALPHAMASK},

	{"V_CHARCOLORSHIFT",V_CHARCOLORSHIFT},
	{"V_ALPHASHIFT",V_ALPHASHIFT},

	//Kick Reasons
	{"KR_KICK",KR_KICK},
	{"KR_PINGLIMIT",KR_PINGLIMIT},
	{"KR_SYNCH",KR_SYNCH},
	{"KR_TIMEOUT",KR_TIMEOUT},
	{"KR_BAN",KR_BAN},
	{"KR_LEAVE",KR_LEAVE},

	// translation colormaps
	{"TC_DEFAULT",TC_DEFAULT},
	{"TC_BOSS",TC_BOSS},
	{"TC_METALSONIC",TC_METALSONIC},
	{"TC_ALLWHITE",TC_ALLWHITE},
	{"TC_RAINBOW",TC_RAINBOW},
	{"TC_BLINK",TC_BLINK},
	{"TC_DASHMODE",TC_DASHMODE},

	// marathonmode flags
	{"MA_INIT",MA_INIT},
	{"MA_RUNNING",MA_RUNNING},
	{"MA_NOCUTSCENES",MA_NOCUTSCENES},
	{"MA_INGAME",MA_INGAME},

	// music types
	{"MU_NONE", MU_NONE},
	{"MU_WAV", MU_WAV},
	{"MU_MOD", MU_MOD},
	{"MU_MID", MU_MID},
	{"MU_OGG", MU_OGG},
	{"MU_MP3", MU_MP3},
	{"MU_FLAC", MU_FLAC},
	{"MU_GME", MU_GME},
	{"MU_MOD_EX", MU_MOD_EX},
	{"MU_MID_EX", MU_MID_EX},

	// gamestates
	{"GS_NULL",GS_NULL},
	{"GS_LEVEL",GS_LEVEL},
	{"GS_INTERMISSION",GS_INTERMISSION},
	{"GS_CONTINUING",GS_CONTINUING},
	{"GS_TITLESCREEN",GS_TITLESCREEN},
	{"GS_TIMEATTACK",GS_TIMEATTACK},
	{"GS_CREDITS",GS_CREDITS},
	{"GS_EVALUATION",GS_EVALUATION},
	{"GS_GAMEEND",GS_GAMEEND},
	{"GS_INTRO",GS_INTRO},
	{"GS_ENDING",GS_ENDING},
	{"GS_CUTSCENE",GS_CUTSCENE},
	{"GS_DEDICATEDSERVER",GS_DEDICATEDSERVER},
	{"GS_WAITINGPLAYERS",GS_WAITINGPLAYERS},

	// Joystick axes
	{"JA_NONE",JA_NONE},
	{"JA_TURN",JA_TURN},
	{"JA_MOVE",JA_MOVE},
	{"JA_LOOK",JA_LOOK},
	{"JA_STRAFE",JA_STRAFE},
	{"JA_DIGITAL",JA_DIGITAL},
	{"JA_JUMP",JA_JUMP},
	{"JA_SPIN",JA_SPIN},
	{"JA_SHIELD",JA_SHIELD},
	{"JA_FIRE",JA_FIRE},
	{"JA_FIRENORMAL",JA_FIRENORMAL},
	{"JOYAXISRANGE",JOYAXISRANGE},

	// Game controls
	{"GC_NULL",GC_NULL},
	{"GC_FORWARD",GC_FORWARD},
	{"GC_BACKWARD",GC_BACKWARD},
	{"GC_STRAFELEFT",GC_STRAFELEFT},
	{"GC_STRAFERIGHT",GC_STRAFERIGHT},
	{"GC_TURNLEFT",GC_TURNLEFT},
	{"GC_TURNRIGHT",GC_TURNRIGHT},
	{"GC_WEAPONNEXT",GC_WEAPONNEXT},
	{"GC_WEAPONPREV",GC_WEAPONPREV},
	{"GC_WEPSLOT1",GC_WEPSLOT1},
	{"GC_WEPSLOT2",GC_WEPSLOT2},
	{"GC_WEPSLOT3",GC_WEPSLOT3},
	{"GC_WEPSLOT4",GC_WEPSLOT4},
	{"GC_WEPSLOT5",GC_WEPSLOT5},
	{"GC_WEPSLOT6",GC_WEPSLOT6},
	{"GC_WEPSLOT7",GC_WEPSLOT7},
	{"GC_SHIELD",GC_SHIELD},
	{"GC_FIRE",GC_FIRE},
	{"GC_FIRENORMAL",GC_FIRENORMAL},
	{"GC_TOSSFLAG",GC_TOSSFLAG},
	{"GC_SPIN",GC_SPIN},
	{"GC_CAMTOGGLE",GC_CAMTOGGLE},
	{"GC_CAMRESET",GC_CAMRESET},
	{"GC_LOOKUP",GC_LOOKUP},
	{"GC_LOOKDOWN",GC_LOOKDOWN},
	{"GC_CENTERVIEW",GC_CENTERVIEW},
	{"GC_MOUSEAIMING",GC_MOUSEAIMING},
	{"GC_TALKKEY",GC_TALKKEY},
	{"GC_TEAMKEY",GC_TEAMKEY},
	{"GC_SCORES",GC_SCORES},
	{"GC_JUMP",GC_JUMP},
	{"GC_CONSOLE",GC_CONSOLE},
	{"GC_PAUSE",GC_PAUSE},
	{"GC_SYSTEMMENU",GC_SYSTEMMENU},
	{"GC_SCREENSHOT",GC_SCREENSHOT},
	{"GC_RECORDGIF",GC_RECORDGIF},
	{"GC_VIEWPOINTNEXT",GC_VIEWPOINTNEXT},
	{"GC_VIEWPOINT",GC_VIEWPOINTNEXT}, // Alias for retrocompatibility. Remove for the next major version
	{"GC_VIEWPOINTPREV",GC_VIEWPOINTPREV},
	{"GC_CUSTOM1",GC_CUSTOM1},
	{"GC_CUSTOM2",GC_CUSTOM2},
	{"GC_CUSTOM3",GC_CUSTOM3},
	{"NUM_GAMECONTROLS",NUM_GAMECONTROLS},

	// Mouse buttons
	{"MB_BUTTON1",MB_BUTTON1},
	{"MB_BUTTON2",MB_BUTTON2},
	{"MB_BUTTON3",MB_BUTTON3},
	{"MB_BUTTON4",MB_BUTTON4},
	{"MB_BUTTON5",MB_BUTTON5},
	{"MB_BUTTON6",MB_BUTTON6},
	{"MB_BUTTON7",MB_BUTTON7},
	{"MB_BUTTON8",MB_BUTTON8},
	{"MB_SCROLLUP",MB_SCROLLUP},
	{"MB_SCROLLDOWN",MB_SCROLLDOWN},

	{NULL,0}
};

// For this to work compile-time without being in this file,
// this function would need to check sizes at runtime, without sizeof
void DEH_TableCheck(void)
{
#if defined(_DEBUG) || defined(PARANOIA)
	const size_t dehpowers = sizeof(POWERS_LIST)/sizeof(const char*);
	const size_t dehcolors = sizeof(COLOR_ENUMS)/sizeof(const char*);

	if (dehpowers != NUMPOWERS)
		I_Error("You forgot to update the Dehacked powers list, you dolt!\n(%d powers defined, versus %s in the Dehacked list)\n", NUMPOWERS, sizeu1(dehpowers));

	if (dehcolors != SKINCOLOR_FIRSTFREESLOT)
		I_Error("You forgot to update the Dehacked colors list, you dolt!\n(%d colors defined, versus %s in the Dehacked list)\n", SKINCOLOR_FIRSTFREESLOT, sizeu1(dehcolors));
#endif
}
