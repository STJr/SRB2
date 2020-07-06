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
/// \file  info.h
/// \brief Thing frame/state LUT

#ifndef __INFO__
#define __INFO__

// Needed for action function pointer handling.
#include "d_think.h"
#include "sounds.h"
#include "m_fixed.h"

// dehacked.c now has lists for the more named enums! PLEASE keep them up to date!
// For great modding!!

// IMPORTANT NOTE: If you add/remove from this list of action
// functions, don't forget to update them in dehacked.c!
void A_Explode();
void A_Pain();
void A_Fall();
void A_MonitorPop();
void A_GoldMonitorPop();
void A_GoldMonitorRestore();
void A_GoldMonitorSparkle();
void A_Look();
void A_Chase();
void A_FaceStabChase();
void A_FaceStabRev();
void A_FaceStabHurl();
void A_FaceStabMiss();
void A_StatueBurst();
void A_FaceTarget();
void A_FaceTracer();
void A_Scream();
void A_BossDeath();
void A_CustomPower(); // Use this for a custom power
void A_GiveWeapon(); // Gives the player weapon(s)
void A_RingBox(); // Obtained Ring Box Tails
void A_Invincibility(); // Obtained Invincibility Box
void A_SuperSneakers(); // Obtained Super Sneakers Box
void A_BunnyHop(); // have bunny hop tails
void A_BubbleSpawn(); // Randomly spawn bubbles
void A_FanBubbleSpawn();
void A_BubbleRise(); // Bubbles float to surface
void A_BubbleCheck(); // Don't draw if not underwater
void A_AwardScore();
void A_ExtraLife(); // Extra Life
void A_GiveShield(); // Obtained Shield
void A_GravityBox();
void A_ScoreRise(); // Rise the score logo
void A_AttractChase(); // Ring Chase
void A_DropMine(); // Drop Mine from Skim or Jetty-Syn Bomber
void A_FishJump(); // Fish Jump
void A_ThrownRing(); // Sparkle trail for red ring
void A_SetSolidSteam();
void A_UnsetSolidSteam();
void A_SignSpin();
void A_SignPlayer();
void A_OverlayThink();
void A_JetChase();
void A_JetbThink(); // Jetty-Syn Bomber Thinker
void A_JetgThink(); // Jetty-Syn Gunner Thinker
void A_JetgShoot(); // Jetty-Syn Shoot Function
void A_ShootBullet(); // JetgShoot without reactiontime setting
void A_MinusDigging();
void A_MinusPopup();
void A_MinusCheck();
void A_ChickenCheck();
void A_MouseThink(); // Mouse Thinker
void A_DetonChase(); // Deton Chaser
void A_CapeChase(); // Fake little Super Sonic cape
void A_RotateSpikeBall(); // Spike ball rotation
void A_SlingAppear();
void A_UnidusBall();
void A_RockSpawn();
void A_SetFuse();
void A_CrawlaCommanderThink(); // Crawla Commander
void A_SmokeTrailer();
void A_RingExplode();
void A_OldRingExplode();
void A_MixUp();
void A_RecyclePowers();
void A_BossScream();
void A_Boss2TakeDamage();
void A_GoopSplat();
void A_Boss2PogoSFX();
void A_Boss2PogoTarget();
void A_EggmanBox();
void A_TurretFire();
void A_SuperTurretFire();
void A_TurretStop();
void A_JetJawRoam();
void A_JetJawChomp();
void A_PointyThink();
void A_CheckBuddy();
void A_HoodFire();
void A_HoodThink();
void A_HoodFall();
void A_ArrowBonks();
void A_SnailerThink();
void A_SharpChase();
void A_SharpSpin();
void A_SharpDecel();
void A_CrushstaceanWalk();
void A_CrushstaceanPunch();
void A_CrushclawAim();
void A_CrushclawLaunch();
void A_VultureVtol();
void A_VultureCheck();
void A_VultureHover();
void A_VultureBlast();
void A_VultureFly();
void A_SkimChase();
void A_SkullAttack();
void A_LobShot();
void A_FireShot();
void A_SuperFireShot();
void A_BossFireShot();
void A_Boss7FireMissiles();
void A_Boss1Laser();
void A_FocusTarget();
void A_Boss4Reverse();
void A_Boss4SpeedUp();
void A_Boss4Raise();
void A_SparkFollow();
void A_BuzzFly();
void A_GuardChase();
void A_EggShield();
void A_SetReactionTime();
void A_Boss1Spikeballs();
void A_Boss3TakeDamage();
void A_Boss3Path();
void A_Boss3ShockThink();
void A_LinedefExecute();
void A_PlaySeeSound();
void A_PlayAttackSound();
void A_PlayActiveSound();
void A_1upThinker();
void A_BossZoom(); //Unused
void A_Boss1Chase();
void A_Boss2Chase();
void A_Boss2Pogo();
void A_Boss7Chase();
void A_BossJetFume();
void A_SpawnObjectAbsolute();
void A_SpawnObjectRelative();
void A_ChangeAngleRelative();
void A_ChangeAngleAbsolute();
void A_RollAngle();
void A_ChangeRollAngleRelative();
void A_ChangeRollAngleAbsolute();
void A_PlaySound();
void A_FindTarget();
void A_FindTracer();
void A_SetTics();
void A_SetRandomTics();
void A_ChangeColorRelative();
void A_ChangeColorAbsolute();
void A_Dye();
void A_MoveRelative();
void A_MoveAbsolute();
void A_Thrust();
void A_ZThrust();
void A_SetTargetsTarget();
void A_SetObjectFlags();
void A_SetObjectFlags2();
void A_RandomState();
void A_RandomStateRange();
void A_DualAction();
void A_RemoteAction();
void A_ToggleFlameJet();
void A_OrbitNights();
void A_GhostMe();
void A_SetObjectState();
void A_SetObjectTypeState();
void A_KnockBack();
void A_PushAway();
void A_RingDrain();
void A_SplitShot();
void A_MissileSplit();
void A_MultiShot();
void A_InstaLoop();
void A_Custom3DRotate();
void A_SearchForPlayers();
void A_CheckRandom();
void A_CheckTargetRings();
void A_CheckRings();
void A_CheckTotalRings();
void A_CheckHealth();
void A_CheckRange();
void A_CheckHeight();
void A_CheckTrueRange();
void A_CheckThingCount();
void A_CheckAmbush();
void A_CheckCustomValue();
void A_CheckCusValMemo();
void A_SetCustomValue();
void A_UseCusValMemo();
void A_RelayCustomValue();
void A_CusValAction();
void A_ForceStop();
void A_ForceWin();
void A_SpikeRetract();
void A_InfoState();
void A_Repeat();
void A_SetScale();
void A_RemoteDamage();
void A_HomingChase();
void A_TrapShot();
void A_VileTarget();
void A_VileAttack();
void A_VileFire();
void A_BrakChase();
void A_BrakFireShot();
void A_BrakLobShot();
void A_NapalmScatter();
void A_SpawnFreshCopy();
void A_FlickySpawn();
void A_FlickyCenter();
void A_FlickyAim();
void A_FlickyFly();
void A_FlickySoar();
void A_FlickyCoast();
void A_FlickyHop();
void A_FlickyFlounder();
void A_FlickyCheck();
void A_FlickyHeightCheck();
void A_FlickyFlutter();
void A_FlameParticle();
void A_FadeOverlay();
void A_Boss5Jump();
void A_LightBeamReset();
void A_MineExplode();
void A_MineRange();
void A_ConnectToGround();
void A_SpawnParticleRelative();
void A_MultiShotDist();
void A_WhoCaresIfYourSonIsABee();
void A_ParentTriesToSleep();
void A_CryingToMomma();
void A_CheckFlags2();
void A_Boss5FindWaypoint();
void A_DoNPCSkid();
void A_DoNPCPain();
void A_PrepareRepeat();
void A_Boss5ExtraRepeat();
void A_Boss5Calm();
void A_Boss5CheckOnGround();
void A_Boss5CheckFalling();
void A_Boss5PinchShot();
void A_Boss5MakeItRain();
void A_Boss5MakeJunk();
void A_LookForBetter();
void A_Boss5BombExplode();
void A_DustDevilThink();
void A_TNTExplode();
void A_DebrisRandom();
void A_TrainCameo();
void A_TrainCameo2();
void A_CanarivoreGas();
void A_KillSegments();
void A_SnapperSpawn();
void A_SnapperThinker();
void A_SaloonDoorSpawn();
void A_MinecartSparkThink();
void A_ModuloToState();
void A_LavafallRocks();
void A_LavafallLava();
void A_FallingLavaCheck();
void A_FireShrink();
void A_SpawnPterabytes();
void A_PterabyteHover();
void A_RolloutSpawn();
void A_RolloutRock();
void A_DragonbomberSpawn();
void A_DragonWing();
void A_DragonSegment();
void A_ChangeHeight();

// ratio of states to sprites to mobj types is roughly 6 : 1 : 1
#define NUMMOBJFREESLOTS 512
#define NUMSPRITEFREESLOTS NUMMOBJFREESLOTS
#define NUMSTATEFREESLOTS (NUMMOBJFREESLOTS*8)

// Hey, moron! If you change this table, don't forget about sprnames in info.c and the sprite lights in hw_light.c!
typedef enum sprite
{
	SPR_NULL, // invisible object
	SPR_UNKN,

	SPR_THOK, // Thok! mobj
	SPR_PLAY,

	// Enemies
	SPR_POSS, // Crawla (Blue)
	SPR_SPOS, // Crawla (Red)
	SPR_FISH, // SDURF
	SPR_BUZZ, // Buzz (Gold)
	SPR_RBUZ, // Buzz (Red)
	SPR_JETB, // Jetty-Syn Bomber
	SPR_JETG, // Jetty-Syn Gunner
	SPR_CCOM, // Crawla Commander
	SPR_DETN, // Deton
	SPR_SKIM, // Skim mine dropper
	SPR_TRET, // Industrial Turret
	SPR_TURR, // Pop-Up Turret
	SPR_SHRP, // Sharp
	SPR_CRAB, // Crushstacean
	SPR_CR2B, // Banpyura
	SPR_CSPR, // Banpyura spring
	SPR_JJAW, // Jet Jaw
	SPR_SNLR, // Snailer
	SPR_VLTR, // BASH
	SPR_PNTY, // Pointy
	SPR_ARCH, // Robo-Hood
	SPR_CBFS, // Castlebot Facestabber
	SPR_STAB, // Castlebot Facestabber spear aura
	SPR_SPSH, // Egg Guard
	SPR_ESHI, // Egg Guard's shield
	SPR_GSNP, // Green Snapper
	SPR_GSNL, // Green Snapper leg
	SPR_GSNH, // Green Snapper head
	SPR_MNUS, // Minus
	SPR_MNUD, // Minus dirt
	SPR_SSHL, // Spring Shell
	SPR_UNID, // Unidus
	SPR_CANA, // Canarivore
	SPR_CANG, // Canarivore gas
	SPR_PYRE, // Pyre Fly
	SPR_PTER, // Pterabyte
	SPR_DRAB, // Dragonbomber

	// Generic Boss Items
	SPR_JETF, // Boss jet fumes

	// Boss 1 (Greenflower)
	SPR_EGGM, // Boss 1
	SPR_EGLZ, // Boss 1 Junk

	// Boss 2 (Techno Hill)
	SPR_EGGN, // Boss 2
	SPR_TANK, // Boss 2 Junk
	SPR_GOOP, // Boss 2 Goop

	// Boss 3 (Deep Sea)
	SPR_EGGO, // Boss 3
	SPR_SEBH, // Boss 3 Junk
	SPR_FAKE, // Boss 3 Fakemobile
	SPR_SHCK, // Boss 3 Shockwave

	// Boss 4 (Castle Eggman)
	SPR_EGGP,
	SPR_EFIR, // Boss 4 jet flame
	SPR_EGR1, // Boss 4 Spectator Eggrobo

	// Boss 5 (Arid Canyon)
	SPR_FANG, // replaces EGGQ
	SPR_BRKN,
	SPR_WHAT,
	SPR_VWRE,
	SPR_PROJ, // projector light
	SPR_FBOM,
	SPR_FSGN,
	SPR_BARX, // bomb explosion (also used by barrel)
	SPR_BARD, // bomb dust (also used by barrel)

	// Boss 6 (Red Volcano)
	SPR_EGGR,

	// Boss 7 (Dark City)
	SPR_BRAK,
	SPR_BGOO, // Goop
	SPR_BMSL,

	// Boss 8 (Egg Rock)
	SPR_EGGT,

	// Cy-Brak-Demon; uses SPR_BRAK as well, but has some extras
	SPR_RCKT, // Rockets!
	SPR_ELEC, // Electricity!
	SPR_TARG, // Targeting reticules!
	SPR_NPLM, // Big napalm bombs!
	SPR_MNPL, // Mini napalm bombs!

	// Metal Sonic
	SPR_METL,
	SPR_MSCF,
	SPR_MSCB,

	// Collectible Items
	SPR_RING,
	SPR_TRNG, // Team Rings
	SPR_TOKE, // Special Stage Token
	SPR_RFLG, // Red CTF Flag
	SPR_BFLG, // Blue CTF Flag
	SPR_SPHR, // Sphere
	SPR_NCHP, // NiGHTS chip
	SPR_NSTR, // NiGHTS star
	SPR_EMBM, // Emblem
	SPR_CEMG, // Chaos Emeralds
	SPR_SHRD, // Emerald Hunt

	// Interactive Objects
	SPR_BBLS, // water bubble source
	SPR_SIGN, // Level end sign
	SPR_SPIK, // Spike Ball
	SPR_SFLM, // Spin fire
	SPR_USPK, // Floor spike
	SPR_WSPK, // Wall spike
	SPR_WSPB, // Wall spike base
	SPR_STPT, // Starpost
	SPR_BMNE, // Big floating mine
	SPR_PUMI, // Rollout Rock

	// Monitor Boxes
	SPR_MSTV, // MiSc TV sprites
	SPR_XLTV, // eXtra Large TV sprites

	SPR_TRRI, // Red team:  10 RIngs
	SPR_TBRI, // Blue team: 10 RIngs

	SPR_TVRI, // 10 RIng
	SPR_TVPI, // PIty shield
	SPR_TVAT, // ATtraction shield
	SPR_TVFO, // FOrce shield
	SPR_TVAR, // ARmageddon shield
	SPR_TVWW, // WhirlWind shield
	SPR_TVEL, // ELemental shield
	SPR_TVSS, // Super Sneakers
	SPR_TVIV, // InVincibility
	SPR_TV1U, // 1Up
	SPR_TV1P, // 1uP (textless)
	SPR_TVEG, // EGgman
	SPR_TVMX, // MiXup
	SPR_TVMY, // MYstery
	SPR_TVGV, // GraVity boots
	SPR_TVRC, // ReCycler
	SPR_TV1K, // 1,000 points  (1 K)
	SPR_TVTK, // 10,000 points (Ten K)
	SPR_TVFL, // FLame shield
	SPR_TVBB, // BuBble shield
	SPR_TVZP, // Thunder shield (ZaP)

	// Projectiles
	SPR_MISL,
	SPR_LASR, // GFZ3 laser
	SPR_LASF, // GFZ3 laser flames
	SPR_TORP, // Torpedo
	SPR_ENRG, // Energy ball
	SPR_MINE, // Skim mine
	SPR_JBUL, // Jetty-Syn Bullet
	SPR_TRLS,
	SPR_CBLL, // Cannonball
	SPR_AROW, // Arrow
	SPR_CFIR, // Colored fire of various sorts

	// The letter
	SPR_LETR,

	// Greenflower Scenery
	SPR_FWR1,
	SPR_FWR2, // GFZ Sunflower
	SPR_FWR3, // GFZ budding flower
	SPR_FWR4,
	SPR_BUS1, // GFZ Bush w/ berries
	SPR_BUS2, // GFZ Bush w/o berries
	SPR_BUS3, // GFZ Bush w/ BLUE berries
	// Trees (both GFZ and misc)
	SPR_TRE1, // GFZ
	SPR_TRE2, // Checker
	SPR_TRE3, // Frozen Hillside
	SPR_TRE4, // Polygon
	SPR_TRE5, // Bush tree
	SPR_TRE6, // Spring tree

	// Techno Hill Scenery
	SPR_THZP, // THZ1 Steam Flower
	SPR_FWR5, // THZ1 Spin flower (red)
	SPR_FWR6, // THZ1 Spin flower (yellow)
	SPR_THZT, // Steam Whistle tree/bush
	SPR_ALRM, // THZ2 Alarm

	// Deep Sea Scenery
	SPR_GARG, // Deep Sea Gargoyle
	SPR_SEWE, // Deep Sea Seaweed
	SPR_DRIP, // Dripping water
	SPR_CORL, // Coral
	SPR_BCRY, // Blue Crystal
	SPR_KELP, // Kelp
	SPR_ALGA, // Animated algae top
	SPR_ALGB, // Animated algae segment
	SPR_DSTG, // DSZ Stalagmites
	SPR_LIBE, // DSZ Light beam

	// Castle Eggman Scenery
	SPR_CHAN, // CEZ Chain
	SPR_FLAM, // Flame
	SPR_ESTA, // Eggman esta una estatua!
	SPR_SMCH, // Small Mace Chain
	SPR_BMCH, // Big Mace Chain
	SPR_SMCE, // Small Mace
	SPR_BMCE, // Big Mace
	SPR_YSPB, // Yellow spring on a ball
	SPR_RSPB, // Red spring on a ball
	SPR_SFBR, // Small Firebar
	SPR_BFBR, // Big Firebar
	SPR_BANR, // Banner/pole
	SPR_PINE, // Pine Tree
	SPR_CEZB, // Bush
	SPR_CNDL, // Candle/pricket
	SPR_FLMH, // Flame holder
	SPR_CTRC, // Fire torch
	SPR_CFLG, // Waving flag/segment
	SPR_CSTA, // Crawla statue
	SPR_CBBS, // Facestabber statue
	SPR_CABR, // Brambles

	// Arid Canyon Scenery
	SPR_BTBL, // Big tumbleweed
	SPR_STBL, // Small tumbleweed
	SPR_CACT, // Cacti
	SPR_WWSG, // Caution Sign
	SPR_WWS2, // Cacti Sign
	SPR_WWS3, // Sharp Turn Sign
	SPR_OILL, // Oil lamp
	SPR_OILF, // Oil lamp flare
	SPR_BARR, // TNT barrel
	SPR_REMT, // TNT proximity shell
	SPR_TAZD, // Dust devil
	SPR_ADST, // Arid dust
	SPR_MCRT, // Minecart
	SPR_MCSP, // Minecart spark
	SPR_SALD, // Saloon door
	SPR_TRAE, // Train cameo locomotive
	SPR_TRAI, // Train cameo wagon
	SPR_STEA, // Train steam

	// Red Volcano Scenery
	SPR_FLME, // Flame jet
	SPR_DFLM, // Blade's flame
	SPR_LFAL, // Lavafall
	SPR_JPLA, // Jungle palm
	SPR_TFLO, // Torch flower
	SPR_WVIN, // Wall vines

	// Dark City Scenery

	// Egg Rock Scenery

	// Christmas Scenery
	SPR_XMS1, // Christmas Pole
	SPR_XMS2, // Candy Cane
	SPR_XMS3, // Snowman
	SPR_XMS4, // Lamppost
	SPR_XMS5, // Hanging Star
	SPR_XMS6, // Mistletoe
	SPR_FHZI, // FHZ Ice
	SPR_ROSY,

	// Halloween Scenery
	SPR_PUMK, // Pumpkins
	SPR_HHPL, // Dr Seuss Trees
	SPR_SHRM, // Mushroom
	SPR_HHZM, // Misc

	// Azure Temple Scenery
	SPR_BGAR, // ATZ Gargoyles
	SPR_RCRY, // ATZ Red Crystal (Target)
	SPR_CFLM, // Green torch flame

	// Botanic Serenity Scenery
	SPR_BSZ1, // Tall flowers
	SPR_BSZ2, // Medium flowers
	SPR_BSZ3, // Small flowers
	//SPR_BSZ4, -- Tulips
	SPR_BST1, // Red tulip
	SPR_BST2, // Purple tulip
	SPR_BST3, // Blue tulip
	SPR_BST4, // Cyan tulip
	SPR_BST5, // Yellow tulip
	SPR_BST6, // Orange tulip
	SPR_BSZ5, // Cluster of Tulips
	SPR_BSZ6, // Bush
	SPR_BSZ7, // Vine
	SPR_BSZ8, // Misc things

	// Misc Scenery
	SPR_STLG, // Stalagmites
	SPR_DBAL, // Disco

	// Powerup Indicators
	SPR_ARMA, // Armageddon Shield Orb
	SPR_ARMF, // Armageddon Shield Ring, Front
	SPR_ARMB, // Armageddon Shield Ring, Back
	SPR_WIND, // Whirlwind Shield Orb
	SPR_MAGN, // Attract Shield Orb
	SPR_ELEM, // Elemental Shield Orb
	SPR_FORC, // Force Shield Orb
	SPR_PITY, // Pity Shield Orb
	SPR_FIRS, // Flame Shield Orb
	SPR_BUBS, // Bubble Shield Orb
	SPR_ZAPS, // Thunder Shield Orb
	SPR_IVSP, // invincibility sparkles
	SPR_SSPK, // Super Sonic Spark

	SPR_GOAL, // Special Stage goal (here because lol NiGHTS)

	// Flickies
	SPR_FBUB, // Flicky-sized bubble
	SPR_FL01, // Bluebird
	SPR_FL02, // Rabbit
	SPR_FL03, // Chicken
	SPR_FL04, // Seal
	SPR_FL05, // Pig
	SPR_FL06, // Chipmunk
	SPR_FL07, // Penguin
	SPR_FL08, // Fish
	SPR_FL09, // Ram
	SPR_FL10, // Puffin
	SPR_FL11, // Cow
	SPR_FL12, // Rat
	SPR_FL13, // Bear
	SPR_FL14, // Dove
	SPR_FL15, // Cat
	SPR_FL16, // Canary
	SPR_FS01, // Spider
	SPR_FS02, // Bat

	// Springs
	SPR_FANS, // Fan
	SPR_STEM, // Steam riser
	SPR_BUMP, // Bumpers
	SPR_BLON, // Balloons
	SPR_SPRY, // Yellow spring
	SPR_SPRR, // Red spring
	SPR_SPRB, // Blue spring
	SPR_YSPR, // Yellow Diagonal Spring
	SPR_RSPR, // Red Diagonal Spring
	SPR_BSPR, // Blue Diagonal Spring
	SPR_SSWY, // Yellow Side Spring
	SPR_SSWR, // Red Side Spring
	SPR_SSWB, // Blue Side Spring
	SPR_BSTY, // Yellow Booster
	SPR_BSTR, // Red Booster

	// Environmental Effects
	SPR_RAIN, // Rain
	SPR_SNO1, // Snowflake
	SPR_SPLH, // Water Splish
	SPR_LSPL, // Lava Splish
	SPR_SPLA, // Water Splash
	SPR_SMOK,
	SPR_BUBL, // Bubble
	SPR_WZAP,
	SPR_DUST, // Spindash dust
	SPR_FPRT, // Spindash dust (flame)
	SPR_TFOG, // Teleport Fog
	SPR_SEED, // Sonic CD flower seed
	SPR_PRTL, // Particle (for fans, etc.)

	// Game Indicators
	SPR_SCOR, // Score logo
	SPR_DRWN, // Drowning Timer
	SPR_FLII, // AI flight indicator
	SPR_LCKN, // Target
	SPR_TTAG, // Tag Sign
	SPR_GFLG, // Got Flag sign
	SPR_FNSF, // Finish flag

	SPR_CORK,
	SPR_LHRT,

	// Ring Weapons
	SPR_RRNG, // Red Ring
	SPR_RNGB, // Bounce Ring
	SPR_RNGR, // Rail Ring
	SPR_RNGI, // Infinity Ring
	SPR_RNGA, // Automatic Ring
	SPR_RNGE, // Explosion Ring
	SPR_RNGS, // Scatter Ring
	SPR_RNGG, // Grenade Ring

	SPR_PIKB, // Bounce Ring Pickup
	SPR_PIKR, // Rail Ring Pickup
	SPR_PIKA, // Automatic Ring Pickup
	SPR_PIKE, // Explosion Ring Pickup
	SPR_PIKS, // Scatter Ring Pickup
	SPR_PIKG, // Grenade Ring Pickup

	SPR_TAUT, // Thrown Automatic Ring
	SPR_TGRE, // Thrown Grenade Ring
	SPR_TSCR, // Thrown Scatter Ring

	// Mario-specific stuff
	SPR_COIN,
	SPR_CPRK,
	SPR_GOOM,
	SPR_BGOM,
	SPR_FFWR,
	SPR_FBLL,
	SPR_SHLL,
	SPR_PUMA,
	SPR_HAMM,
	SPR_KOOP,
	SPR_BFLM,
	SPR_MAXE,
	SPR_MUS1,
	SPR_MUS2,
	SPR_TOAD,

	// NiGHTS Stuff
	SPR_NDRN, // NiGHTS drone
	SPR_NSPK, // NiGHTS sparkle
	SPR_NBMP, // NiGHTS Bumper
	SPR_HOOP, // NiGHTS hoop sprite
	SPR_NSCR, // NiGHTS score sprite
	SPR_NPRU, // Nights Powerups
	SPR_CAPS, // Capsule thingy for NiGHTS
	SPR_IDYA, // Ideya
	SPR_NTPN, // Nightopian
	SPR_SHLP, // Shleep

	// Secret badniks and hazards, shhhh
	SPR_PENG,
	SPR_POPH,
	SPR_HIVE,
	SPR_BUMB,
	SPR_BBUZ,
	SPR_FMCE,
	SPR_HMCE,
	SPR_CACO,
	SPR_BAL2,
	SPR_SBOB,
	SPR_SBFL,
	SPR_SBSK,
	SPR_HBAT,

	// Debris
	SPR_SPRK, // Sparkle
	SPR_BOM1, // Robot Explosion
	SPR_BOM2, // Boss Explosion 1
	SPR_BOM3, // Boss Explosion 2
	SPR_BOM4, // Underwater Explosion
	SPR_BMNB, // Mine Explosion

	// Crumbly rocks
	SPR_ROIA,
	SPR_ROIB,
	SPR_ROIC,
	SPR_ROID,
	SPR_ROIE,
	SPR_ROIF,
	SPR_ROIG,
	SPR_ROIH,
	SPR_ROII,
	SPR_ROIJ,
	SPR_ROIK,
	SPR_ROIL,
	SPR_ROIM,
	SPR_ROIN,
	SPR_ROIO,
	SPR_ROIP,

	// Level debris
	SPR_GFZD, // GFZ debris
	SPR_BRIC, // Bricks
	SPR_WDDB, // Wood Debris
	SPR_BRIR, // CEZ3 colored bricks
	SPR_BRIB,
	SPR_BRIY,

	// Gravity Well Objects
	SPR_GWLG,
	SPR_GWLR,

	SPR_FIRSTFREESLOT,
	SPR_LASTFREESLOT = SPR_FIRSTFREESLOT + NUMSPRITEFREESLOTS - 1,
	NUMSPRITES
} spritenum_t;

// Make sure to be conscious of FF_FRAMEMASK and the fact sprite2 is stored as a UINT8 whenever you change this table.
// Currently, FF_FRAMEMASK is 0xff, or 255 - but the second half is used by FF_SPR2SUPER, so the limitation is 0x7f.
// Since this is zero-based, there can be at most 128 different SPR2_'s without changing that.
typedef enum playersprite
{
	SPR2_STND = 0,
	SPR2_WAIT,
	SPR2_WALK,
	SPR2_SKID,
	SPR2_RUN ,
	SPR2_DASH,
	SPR2_PAIN,
	SPR2_STUN,
	SPR2_DEAD,
	SPR2_DRWN, // drown
	SPR2_ROLL,
	SPR2_GASP,
	SPR2_JUMP,
	SPR2_SPNG, // spring
	SPR2_FALL,
	SPR2_EDGE,
	SPR2_RIDE,

	SPR2_SPIN, // spindash

	SPR2_FLY ,
	SPR2_SWIM,
	SPR2_TIRE, // tired

	SPR2_GLID, // glide
	SPR2_LAND, // landing after glide/bounce
	SPR2_CLNG, // cling
	SPR2_CLMB, // climb

	SPR2_FLT , // float
	SPR2_FRUN, // float run

	SPR2_BNCE, // bounce

	SPR2_FIRE, // fire

	SPR2_TWIN, // twinspin

	SPR2_MLEE, // melee
	SPR2_MLEL, // melee land

	SPR2_TRNS, // transformation

	SPR2_NSTD, // NiGHTS stand
	SPR2_NFLT, // NiGHTS float
	SPR2_NFLY, // NiGHTS fly
	SPR2_NDRL, // NiGHTS drill
	SPR2_NSTN, // NiGHTS stun
	SPR2_NPUL, // NiGHTS pull
	SPR2_NATK, // NiGHTS attack

	// c:
	SPR2_TAL0,
	SPR2_TAL1,
	SPR2_TAL2,
	SPR2_TAL3,
	SPR2_TAL4,
	SPR2_TAL5,
	SPR2_TAL6,
	SPR2_TAL7,
	SPR2_TAL8,
	SPR2_TAL9,
	SPR2_TALA,
	SPR2_TALB,

	SPR2_CNT1, // continue disappointment
	SPR2_CNT2, // continue lift
	SPR2_CNT3, // continue spin
	SPR2_CNT4, // continue "soooooooniiic!" tugging

	SPR2_SIGN, // end sign head
	SPR2_LIFE, // life monitor icon

	SPR2_XTRA, // stuff that isn't in-map - "would this ever need an md2 or variable length animation?"

	SPR2_FIRSTFREESLOT,
	SPR2_LASTFREESLOT = 0x7f,
	NUMPLAYERSPRITES
} playersprite_t;

// SPR2_XTRA
#define XTRA_LIFEPIC    0                 // Life icon patch
#define XTRA_CHARSEL    1                 // Character select picture
#define XTRA_CONTINUE   2                 // Continue icon
#define XTRA_ENDING     3                 // Ending finale patches

typedef enum state
{
	S_NULL,
	S_UNKNOWN,
	S_INVISIBLE, // state for invisible sprite

	S_SPAWNSTATE,
	S_SEESTATE,
	S_MELEESTATE,
	S_MISSILESTATE,
	S_DEATHSTATE,
	S_XDEATHSTATE,
	S_RAISESTATE,

	// Thok
	S_THOK,

	// Player
	S_PLAY_STND,
	S_PLAY_WAIT,
	S_PLAY_WALK,
	S_PLAY_SKID,
	S_PLAY_RUN,
	S_PLAY_DASH,
	S_PLAY_PAIN,
	S_PLAY_STUN,
	S_PLAY_DEAD,
	S_PLAY_DRWN,
	S_PLAY_ROLL,
	S_PLAY_GASP,
	S_PLAY_JUMP,
	S_PLAY_SPRING,
	S_PLAY_FALL,
	S_PLAY_EDGE,
	S_PLAY_RIDE,

	// CA2_SPINDASH
	S_PLAY_SPINDASH,

	// CA_FLY/SWIM
	S_PLAY_FLY,
	S_PLAY_SWIM,
	S_PLAY_FLY_TIRED,

	// CA_GLIDEANDCLIMB
	S_PLAY_GLIDE,
	S_PLAY_GLIDE_LANDING,
	S_PLAY_CLING,
	S_PLAY_CLIMB,

	// CA_FLOAT/CA_SLOWFALL
	S_PLAY_FLOAT,
	S_PLAY_FLOAT_RUN,

	// CA_BOUNCE
	S_PLAY_BOUNCE,
	S_PLAY_BOUNCE_LANDING,

	// CA2_GUNSLINGER
	S_PLAY_FIRE,
	S_PLAY_FIRE_FINISH,

	// CA_TWINSPIN
	S_PLAY_TWINSPIN,

	// CA2_MELEE
	S_PLAY_MELEE,
	S_PLAY_MELEE_FINISH,
	S_PLAY_MELEE_LANDING,

	// SF_SUPER
	S_PLAY_SUPER_TRANS1,
	S_PLAY_SUPER_TRANS2,
	S_PLAY_SUPER_TRANS3,
	S_PLAY_SUPER_TRANS4,
	S_PLAY_SUPER_TRANS5,
	S_PLAY_SUPER_TRANS6,

	// technically the player goes here but it's an infinite tic state
	S_OBJPLACE_DUMMY,

	// 1-Up Box Sprites overlay (uses player sprite)
	S_PLAY_BOX1,
	S_PLAY_BOX2,
	S_PLAY_ICON1,
	S_PLAY_ICON2,
	S_PLAY_ICON3,

	// Level end sign overlay (uses player sprite)
	S_PLAY_SIGN,

	// NiGHTS character (uses player sprite)
	S_PLAY_NIGHTS_TRANS1,
	S_PLAY_NIGHTS_TRANS2,
	S_PLAY_NIGHTS_TRANS3,
	S_PLAY_NIGHTS_TRANS4,
	S_PLAY_NIGHTS_TRANS5,
	S_PLAY_NIGHTS_TRANS6,
	S_PLAY_NIGHTS_STAND,
	S_PLAY_NIGHTS_FLOAT,
	S_PLAY_NIGHTS_FLY,
	S_PLAY_NIGHTS_DRILL,
	S_PLAY_NIGHTS_STUN,
	S_PLAY_NIGHTS_PULL,
	S_PLAY_NIGHTS_ATTACK,

	// c:
	S_TAILSOVERLAY_STAND,
	S_TAILSOVERLAY_0DEGREES,
	S_TAILSOVERLAY_PLUS30DEGREES,
	S_TAILSOVERLAY_PLUS60DEGREES,
	S_TAILSOVERLAY_MINUS30DEGREES,
	S_TAILSOVERLAY_MINUS60DEGREES,
	S_TAILSOVERLAY_RUN,
	S_TAILSOVERLAY_FLY,
	S_TAILSOVERLAY_TIRE,
	S_TAILSOVERLAY_PAIN,
	S_TAILSOVERLAY_GASP,
	S_TAILSOVERLAY_EDGE,

	// [:
	S_JETFUMEFLASH,

	// Blue Crawla
	S_POSS_STND,
	S_POSS_RUN1,
	S_POSS_RUN2,
	S_POSS_RUN3,
	S_POSS_RUN4,
	S_POSS_RUN5,
	S_POSS_RUN6,

	// Red Crawla
	S_SPOS_STND,
	S_SPOS_RUN1,
	S_SPOS_RUN2,
	S_SPOS_RUN3,
	S_SPOS_RUN4,
	S_SPOS_RUN5,
	S_SPOS_RUN6,

	// Greenflower Fish
	S_FISH1,
	S_FISH2,
	S_FISH3,
	S_FISH4,

	// Buzz (Gold)
	S_BUZZLOOK1,
	S_BUZZLOOK2,
	S_BUZZFLY1,
	S_BUZZFLY2,

	// Buzz (Red)
	S_RBUZZLOOK1,
	S_RBUZZLOOK2,
	S_RBUZZFLY1,
	S_RBUZZFLY2,

	// Jetty-Syn Bomber
	S_JETBLOOK1,
	S_JETBLOOK2,
	S_JETBZOOM1,
	S_JETBZOOM2,

	// Jetty-Syn Gunner
	S_JETGLOOK1,
	S_JETGLOOK2,
	S_JETGZOOM1,
	S_JETGZOOM2,
	S_JETGSHOOT1,
	S_JETGSHOOT2,

	// Crawla Commander
	S_CCOMMAND1,
	S_CCOMMAND2,
	S_CCOMMAND3,
	S_CCOMMAND4,

	// Deton
	S_DETON1,
	S_DETON2,
	S_DETON3,
	S_DETON4,
	S_DETON5,
	S_DETON6,
	S_DETON7,
	S_DETON8,
	S_DETON9,
	S_DETON10,
	S_DETON11,
	S_DETON12,
	S_DETON13,
	S_DETON14,
	S_DETON15,

	// Skim Mine Dropper
	S_SKIM1,
	S_SKIM2,
	S_SKIM3,
	S_SKIM4,

	// THZ Turret
	S_TURRET,
	S_TURRETFIRE,
	S_TURRETSHOCK1,
	S_TURRETSHOCK2,
	S_TURRETSHOCK3,
	S_TURRETSHOCK4,
	S_TURRETSHOCK5,
	S_TURRETSHOCK6,
	S_TURRETSHOCK7,
	S_TURRETSHOCK8,
	S_TURRETSHOCK9,

	// Popup Turret
	S_TURRETLOOK,
	S_TURRETSEE,
	S_TURRETPOPUP1,
	S_TURRETPOPUP2,
	S_TURRETPOPUP3,
	S_TURRETPOPUP4,
	S_TURRETPOPUP5,
	S_TURRETPOPUP6,
	S_TURRETPOPUP7,
	S_TURRETPOPUP8,
	S_TURRETSHOOT,
	S_TURRETPOPDOWN1,
	S_TURRETPOPDOWN2,
	S_TURRETPOPDOWN3,
	S_TURRETPOPDOWN4,
	S_TURRETPOPDOWN5,
	S_TURRETPOPDOWN6,
	S_TURRETPOPDOWN7,
	S_TURRETPOPDOWN8,

	// Spincushion
	S_SPINCUSHION_LOOK,
	S_SPINCUSHION_CHASE1,
	S_SPINCUSHION_CHASE2,
	S_SPINCUSHION_CHASE3,
	S_SPINCUSHION_CHASE4,
	S_SPINCUSHION_AIM1,
	S_SPINCUSHION_AIM2,
	S_SPINCUSHION_AIM3,
	S_SPINCUSHION_AIM4,
	S_SPINCUSHION_AIM5,
	S_SPINCUSHION_SPIN1,
	S_SPINCUSHION_SPIN2,
	S_SPINCUSHION_SPIN3,
	S_SPINCUSHION_SPIN4,
	S_SPINCUSHION_STOP1,
	S_SPINCUSHION_STOP2,
	S_SPINCUSHION_STOP3,
	S_SPINCUSHION_STOP4,

	// Crushstacean
	S_CRUSHSTACEAN_ROAM1,
	S_CRUSHSTACEAN_ROAM2,
	S_CRUSHSTACEAN_ROAM3,
	S_CRUSHSTACEAN_ROAM4,
	S_CRUSHSTACEAN_ROAMPAUSE,
	S_CRUSHSTACEAN_PUNCH1,
	S_CRUSHSTACEAN_PUNCH2,
	S_CRUSHCLAW_AIM,
	S_CRUSHCLAW_OUT,
	S_CRUSHCLAW_STAY,
	S_CRUSHCLAW_IN,
	S_CRUSHCLAW_WAIT,
	S_CRUSHCHAIN,

	// Banpyura
	S_BANPYURA_ROAM1,
	S_BANPYURA_ROAM2,
	S_BANPYURA_ROAM3,
	S_BANPYURA_ROAM4,
	S_BANPYURA_ROAMPAUSE,
	S_CDIAG1,
	S_CDIAG2,
	S_CDIAG3,
	S_CDIAG4,
	S_CDIAG5,
	S_CDIAG6,
	S_CDIAG7,
	S_CDIAG8,

	// Jet Jaw
	S_JETJAW_ROAM1,
	S_JETJAW_ROAM2,
	S_JETJAW_ROAM3,
	S_JETJAW_ROAM4,
	S_JETJAW_ROAM5,
	S_JETJAW_ROAM6,
	S_JETJAW_ROAM7,
	S_JETJAW_ROAM8,
	S_JETJAW_CHOMP1,
	S_JETJAW_CHOMP2,
	S_JETJAW_CHOMP3,
	S_JETJAW_CHOMP4,
	S_JETJAW_CHOMP5,
	S_JETJAW_CHOMP6,
	S_JETJAW_CHOMP7,
	S_JETJAW_CHOMP8,
	S_JETJAW_CHOMP9,
	S_JETJAW_CHOMP10,
	S_JETJAW_CHOMP11,
	S_JETJAW_CHOMP12,
	S_JETJAW_CHOMP13,
	S_JETJAW_CHOMP14,
	S_JETJAW_CHOMP15,
	S_JETJAW_CHOMP16,
	S_JETJAW_SOUND,

	// Snailer
	S_SNAILER1,
	S_SNAILER_FLICKY,

	// Vulture
	S_VULTURE_STND,
	S_VULTURE_DRIFT,
	S_VULTURE_ZOOM1,
	S_VULTURE_ZOOM2,
	S_VULTURE_STUNNED,

	// Pointy
	S_POINTY1,
	S_POINTYBALL1,

	// Robo-Hood
	S_ROBOHOOD_LOOK,
	S_ROBOHOOD_STAND,
	S_ROBOHOOD_FIRE1,
	S_ROBOHOOD_FIRE2,
	S_ROBOHOOD_JUMP1,
	S_ROBOHOOD_JUMP2,
	S_ROBOHOOD_JUMP3,

	// Castlebot Facestabber
	S_FACESTABBER_STND1,
	S_FACESTABBER_STND2,
	S_FACESTABBER_STND3,
	S_FACESTABBER_STND4,
	S_FACESTABBER_STND5,
	S_FACESTABBER_STND6,
	S_FACESTABBER_CHARGE1,
	S_FACESTABBER_CHARGE2,
	S_FACESTABBER_CHARGE3,
	S_FACESTABBER_CHARGE4,
	S_FACESTABBER_PAIN,
	S_FACESTABBER_DIE1,
	S_FACESTABBER_DIE2,
	S_FACESTABBER_DIE3,
	S_FACESTABBERSPEAR,

	// Egg Guard
	S_EGGGUARD_STND,
	S_EGGGUARD_WALK1,
	S_EGGGUARD_WALK2,
	S_EGGGUARD_WALK3,
	S_EGGGUARD_WALK4,
	S_EGGGUARD_MAD1,
	S_EGGGUARD_MAD2,
	S_EGGGUARD_MAD3,
	S_EGGGUARD_RUN1,
	S_EGGGUARD_RUN2,
	S_EGGGUARD_RUN3,
	S_EGGGUARD_RUN4,

	// Egg Shield for Egg Guard
	S_EGGSHIELD,
	S_EGGSHIELDBREAK,

	// Green Snapper
	S_SNAPPER_SPAWN,
	S_SNAPPER_SPAWN2,
	S_GSNAPPER_STND,
	S_GSNAPPER1,
	S_GSNAPPER2,
	S_GSNAPPER3,
	S_GSNAPPER4,
	S_SNAPPER_XPLD,
	S_SNAPPER_LEG,
	S_SNAPPER_LEGRAISE,
	S_SNAPPER_HEAD,

	// Minus
	S_MINUS_INIT,
	S_MINUS_STND,
	S_MINUS_DIGGING1,
	S_MINUS_DIGGING2,
	S_MINUS_DIGGING3,
	S_MINUS_DIGGING4,
	S_MINUS_BURST0,
	S_MINUS_BURST1,
	S_MINUS_BURST2,
	S_MINUS_BURST3,
	S_MINUS_BURST4,
	S_MINUS_BURST5,
	S_MINUS_POPUP,
	S_MINUS_AERIAL1,
	S_MINUS_AERIAL2,
	S_MINUS_AERIAL3,
	S_MINUS_AERIAL4,

	// Minus dirt
	S_MINUSDIRT1,
	S_MINUSDIRT2,
	S_MINUSDIRT3,
	S_MINUSDIRT4,
	S_MINUSDIRT5,
	S_MINUSDIRT6,
	S_MINUSDIRT7,

	// Spring Shell
	S_SSHELL_STND,
	S_SSHELL_RUN1,
	S_SSHELL_RUN2,
	S_SSHELL_RUN3,
	S_SSHELL_RUN4,
	S_SSHELL_SPRING1,
	S_SSHELL_SPRING2,
	S_SSHELL_SPRING3,
	S_SSHELL_SPRING4,

	// Spring Shell (yellow)
	S_YSHELL_STND,
	S_YSHELL_RUN1,
	S_YSHELL_RUN2,
	S_YSHELL_RUN3,
	S_YSHELL_RUN4,
	S_YSHELL_SPRING1,
	S_YSHELL_SPRING2,
	S_YSHELL_SPRING3,
	S_YSHELL_SPRING4,

	// Unidus
	S_UNIDUS_STND,
	S_UNIDUS_RUN,
	S_UNIDUS_BALL,

	// Canarivore
	S_CANARIVORE_LOOK,
	S_CANARIVORE_AWAKEN1,
	S_CANARIVORE_AWAKEN2,
	S_CANARIVORE_AWAKEN3,
	S_CANARIVORE_GAS1,
	S_CANARIVORE_GAS2,
	S_CANARIVORE_GAS3,
	S_CANARIVORE_GAS4,
	S_CANARIVORE_GAS5,
	S_CANARIVORE_GASREPEAT,
	S_CANARIVORE_CLOSE1,
	S_CANARIVORE_CLOSE2,
	S_CANARIVOREGAS_1,
	S_CANARIVOREGAS_2,
	S_CANARIVOREGAS_3,
	S_CANARIVOREGAS_4,
	S_CANARIVOREGAS_5,
	S_CANARIVOREGAS_6,
	S_CANARIVOREGAS_7,
	S_CANARIVOREGAS_8,

	// Pyre Fly
	S_PYREFLY_FLY,
	S_PYREFLY_BURN,
	S_PYREFIRE1,
	S_PYREFIRE2,

	// Pterabyte
	S_PTERABYTESPAWNER,
	S_PTERABYTEWAYPOINT,
	S_PTERABYTE_FLY1,
	S_PTERABYTE_FLY2,
	S_PTERABYTE_FLY3,
	S_PTERABYTE_FLY4,
	S_PTERABYTE_SWOOPDOWN,
	S_PTERABYTE_SWOOPUP,

	// Dragonbomber
	S_DRAGONBOMBER,
	S_DRAGONWING1,
	S_DRAGONWING2,
	S_DRAGONWING3,
	S_DRAGONWING4,
	S_DRAGONTAIL_LOADED,
	S_DRAGONTAIL_EMPTY,
	S_DRAGONTAIL_EMPTYLOOP,
	S_DRAGONTAIL_RELOAD,
	S_DRAGONMINE,
	S_DRAGONMINE_LAND1,
	S_DRAGONMINE_LAND2,
	S_DRAGONMINE_SLOWFLASH1,
	S_DRAGONMINE_SLOWFLASH2,
	S_DRAGONMINE_SLOWLOOP,
	S_DRAGONMINE_FASTFLASH1,
	S_DRAGONMINE_FASTFLASH2,
	S_DRAGONMINE_FASTLOOP,

	// Boss Explosion
	S_BOSSEXPLODE,

	// S3&K Boss Explosion
	S_SONIC3KBOSSEXPLOSION1,
	S_SONIC3KBOSSEXPLOSION2,
	S_SONIC3KBOSSEXPLOSION3,
	S_SONIC3KBOSSEXPLOSION4,
	S_SONIC3KBOSSEXPLOSION5,
	S_SONIC3KBOSSEXPLOSION6,

	S_JETFUME1,

	// Boss 1
	S_EGGMOBILE_STND,
	S_EGGMOBILE_ROFL,
	S_EGGMOBILE_LATK1,
	S_EGGMOBILE_LATK2,
	S_EGGMOBILE_LATK3,
	S_EGGMOBILE_LATK4,
	S_EGGMOBILE_LATK5,
	S_EGGMOBILE_LATK6,
	S_EGGMOBILE_LATK7,
	S_EGGMOBILE_LATK8,
	S_EGGMOBILE_LATK9,
	S_EGGMOBILE_RATK1,
	S_EGGMOBILE_RATK2,
	S_EGGMOBILE_RATK3,
	S_EGGMOBILE_RATK4,
	S_EGGMOBILE_RATK5,
	S_EGGMOBILE_RATK6,
	S_EGGMOBILE_RATK7,
	S_EGGMOBILE_RATK8,
	S_EGGMOBILE_RATK9,
	S_EGGMOBILE_PANIC1,
	S_EGGMOBILE_PANIC2,
	S_EGGMOBILE_PANIC3,
	S_EGGMOBILE_PANIC4,
	S_EGGMOBILE_PANIC5,
	S_EGGMOBILE_PANIC6,
	S_EGGMOBILE_PANIC7,
	S_EGGMOBILE_PANIC8,
	S_EGGMOBILE_PANIC9,
	S_EGGMOBILE_PANIC10,
	S_EGGMOBILE_PANIC11,
	S_EGGMOBILE_PANIC12,
	S_EGGMOBILE_PANIC13,
	S_EGGMOBILE_PANIC14,
	S_EGGMOBILE_PANIC15,
	S_EGGMOBILE_PAIN,
	S_EGGMOBILE_PAIN2,
	S_EGGMOBILE_DIE1,
	S_EGGMOBILE_DIE2,
	S_EGGMOBILE_DIE3,
	S_EGGMOBILE_DIE4,
	S_EGGMOBILE_FLEE1,
	S_EGGMOBILE_FLEE2,
	S_EGGMOBILE_BALL,
	S_EGGMOBILE_TARGET,

	S_BOSSEGLZ1,
	S_BOSSEGLZ2,

	// Boss 2
	S_EGGMOBILE2_STND,
	S_EGGMOBILE2_POGO1,
	S_EGGMOBILE2_POGO2,
	S_EGGMOBILE2_POGO3,
	S_EGGMOBILE2_POGO4,
	S_EGGMOBILE2_POGO5,
	S_EGGMOBILE2_POGO6,
	S_EGGMOBILE2_POGO7,
	S_EGGMOBILE2_PAIN,
	S_EGGMOBILE2_PAIN2,
	S_EGGMOBILE2_DIE1,
	S_EGGMOBILE2_DIE2,
	S_EGGMOBILE2_DIE3,
	S_EGGMOBILE2_DIE4,
	S_EGGMOBILE2_FLEE1,
	S_EGGMOBILE2_FLEE2,

	S_BOSSTANK1,
	S_BOSSTANK2,
	S_BOSSSPIGOT,

	// Boss 2 Goop
	S_GOOP1,
	S_GOOP2,
	S_GOOP3,
	S_GOOPTRAIL,

	// Boss 3
	S_EGGMOBILE3_STND,
	S_EGGMOBILE3_SHOCK,
	S_EGGMOBILE3_ATK1,
	S_EGGMOBILE3_ATK2,
	S_EGGMOBILE3_ATK3A,
	S_EGGMOBILE3_ATK3B,
	S_EGGMOBILE3_ATK3C,
	S_EGGMOBILE3_ATK3D,
	S_EGGMOBILE3_ATK4,
	S_EGGMOBILE3_ATK5,
	S_EGGMOBILE3_ROFL,
	S_EGGMOBILE3_PAIN,
	S_EGGMOBILE3_PAIN2,
	S_EGGMOBILE3_DIE1,
	S_EGGMOBILE3_DIE2,
	S_EGGMOBILE3_DIE3,
	S_EGGMOBILE3_DIE4,
	S_EGGMOBILE3_FLEE1,
	S_EGGMOBILE3_FLEE2,

	// Boss 3 Pinch
	S_FAKEMOBILE_INIT,
	S_FAKEMOBILE,
	S_FAKEMOBILE_ATK1,
	S_FAKEMOBILE_ATK2,
	S_FAKEMOBILE_ATK3A,
	S_FAKEMOBILE_ATK3B,
	S_FAKEMOBILE_ATK3C,
	S_FAKEMOBILE_ATK3D,
	S_FAKEMOBILE_DIE1,
	S_FAKEMOBILE_DIE2,

	S_BOSSSEBH1,
	S_BOSSSEBH2,

	// Boss 3 Shockwave
	S_SHOCKWAVE1,
	S_SHOCKWAVE2,

	// Boss 4
	S_EGGMOBILE4_STND,
	S_EGGMOBILE4_LATK1,
	S_EGGMOBILE4_LATK2,
	S_EGGMOBILE4_LATK3,
	S_EGGMOBILE4_LATK4,
	S_EGGMOBILE4_LATK5,
	S_EGGMOBILE4_LATK6,
	S_EGGMOBILE4_RATK1,
	S_EGGMOBILE4_RATK2,
	S_EGGMOBILE4_RATK3,
	S_EGGMOBILE4_RATK4,
	S_EGGMOBILE4_RATK5,
	S_EGGMOBILE4_RATK6,
	S_EGGMOBILE4_RAISE1,
	S_EGGMOBILE4_RAISE2,
	S_EGGMOBILE4_PAIN1,
	S_EGGMOBILE4_PAIN2,
	S_EGGMOBILE4_DIE1,
	S_EGGMOBILE4_DIE2,
	S_EGGMOBILE4_DIE3,
	S_EGGMOBILE4_DIE4,
	S_EGGMOBILE4_FLEE1,
	S_EGGMOBILE4_FLEE2,
	S_EGGMOBILE4_MACE,
	S_EGGMOBILE4_MACE_DIE1,
	S_EGGMOBILE4_MACE_DIE2,
	S_EGGMOBILE4_MACE_DIE3,

	// Boss 4 jet flame
	S_JETFLAME,

	// Boss 4 Spectator Eggrobo
	S_EGGROBO1_STND,
	S_EGGROBO1_BSLAP1,
	S_EGGROBO1_BSLAP2,
	S_EGGROBO1_PISSED,

	// Boss 4 Spectator Eggrobo jet flame
	S_EGGROBOJET,

	// Boss 5
	S_FANG_SETUP,
	S_FANG_INTRO0,
	S_FANG_INTRO1,
	S_FANG_INTRO2,
	S_FANG_INTRO3,
	S_FANG_INTRO4,
	S_FANG_INTRO5,
	S_FANG_INTRO6,
	S_FANG_INTRO7,
	S_FANG_INTRO8,
	S_FANG_INTRO9,
	S_FANG_INTRO10,
	S_FANG_INTRO11,
	S_FANG_INTRO12,
	S_FANG_CLONE1,
	S_FANG_CLONE2,
	S_FANG_CLONE3,
	S_FANG_CLONE4,
	S_FANG_IDLE0,
	S_FANG_IDLE1,
	S_FANG_IDLE2,
	S_FANG_IDLE3,
	S_FANG_IDLE4,
	S_FANG_IDLE5,
	S_FANG_IDLE6,
	S_FANG_IDLE7,
	S_FANG_IDLE8,
	S_FANG_PAIN1,
	S_FANG_PAIN2,
	S_FANG_PATHINGSTART1,
	S_FANG_PATHINGSTART2,
	S_FANG_PATHING,
	S_FANG_BOUNCE1,
	S_FANG_BOUNCE2,
	S_FANG_BOUNCE3,
	S_FANG_BOUNCE4,
	S_FANG_FALL1,
	S_FANG_FALL2,
	S_FANG_CHECKPATH1,
	S_FANG_CHECKPATH2,
	S_FANG_PATHINGCONT1,
	S_FANG_PATHINGCONT2,
	S_FANG_PATHINGCONT3,
	S_FANG_SKID1,
	S_FANG_SKID2,
	S_FANG_SKID3,
	S_FANG_CHOOSEATTACK,
	S_FANG_FIRESTART1,
	S_FANG_FIRESTART2,
	S_FANG_FIRE1,
	S_FANG_FIRE2,
	S_FANG_FIRE3,
	S_FANG_FIRE4,
	S_FANG_FIREREPEAT,
	S_FANG_LOBSHOT0,
	S_FANG_LOBSHOT1,
	S_FANG_LOBSHOT2,
	S_FANG_WAIT1,
	S_FANG_WAIT2,
	S_FANG_WALLHIT,
	S_FANG_PINCHPATHINGSTART1,
	S_FANG_PINCHPATHINGSTART2,
	S_FANG_PINCHPATHING,
	S_FANG_PINCHBOUNCE0,
	S_FANG_PINCHBOUNCE1,
	S_FANG_PINCHBOUNCE2,
	S_FANG_PINCHBOUNCE3,
	S_FANG_PINCHBOUNCE4,
	S_FANG_PINCHFALL0,
	S_FANG_PINCHFALL1,
	S_FANG_PINCHFALL2,
	S_FANG_PINCHSKID1,
	S_FANG_PINCHSKID2,
	S_FANG_PINCHLOBSHOT0,
	S_FANG_PINCHLOBSHOT1,
	S_FANG_PINCHLOBSHOT2,
	S_FANG_PINCHLOBSHOT3,
	S_FANG_PINCHLOBSHOT4,
	S_FANG_DIE1,
	S_FANG_DIE2,
	S_FANG_DIE3,
	S_FANG_DIE4,
	S_FANG_DIE5,
	S_FANG_DIE6,
	S_FANG_DIE7,
	S_FANG_DIE8,
	S_FANG_FLEEPATHING1,
	S_FANG_FLEEPATHING2,
	S_FANG_FLEEBOUNCE1,
	S_FANG_FLEEBOUNCE2,
	S_FANG_KO,

	S_BROKENROBOTRANDOM,
	S_BROKENROBOTA,
	S_BROKENROBOTB,
	S_BROKENROBOTC,
	S_BROKENROBOTD,
	S_BROKENROBOTE,
	S_BROKENROBOTF,

	S_ALART1,
	S_ALART2,

	S_VWREF,
	S_VWREB,

	S_PROJECTORLIGHT1,
	S_PROJECTORLIGHT2,
	S_PROJECTORLIGHT3,
	S_PROJECTORLIGHT4,
	S_PROJECTORLIGHT5,

	S_FBOMB1,
	S_FBOMB2,
	S_FBOMB_EXPL1,
	S_FBOMB_EXPL2,
	S_FBOMB_EXPL3,
	S_FBOMB_EXPL4,
	S_FBOMB_EXPL5,
	S_FBOMB_EXPL6,
	S_TNTDUST_1,
	S_TNTDUST_2,
	S_TNTDUST_3,
	S_TNTDUST_4,
	S_TNTDUST_5,
	S_TNTDUST_6,
	S_TNTDUST_7,
	S_TNTDUST_8,
	S_FSGNA,
	S_FSGNB,
	S_FSGNC,
	S_FSGND,

	// Black Eggman (Boss 7)
	S_BLACKEGG_STND,
	S_BLACKEGG_STND2,
	S_BLACKEGG_WALK1,
	S_BLACKEGG_WALK2,
	S_BLACKEGG_WALK3,
	S_BLACKEGG_WALK4,
	S_BLACKEGG_WALK5,
	S_BLACKEGG_WALK6,
	S_BLACKEGG_SHOOT1,
	S_BLACKEGG_SHOOT2,
	S_BLACKEGG_PAIN1,
	S_BLACKEGG_PAIN2,
	S_BLACKEGG_PAIN3,
	S_BLACKEGG_PAIN4,
	S_BLACKEGG_PAIN5,
	S_BLACKEGG_PAIN6,
	S_BLACKEGG_PAIN7,
	S_BLACKEGG_PAIN8,
	S_BLACKEGG_PAIN9,
	S_BLACKEGG_PAIN10,
	S_BLACKEGG_PAIN11,
	S_BLACKEGG_PAIN12,
	S_BLACKEGG_PAIN13,
	S_BLACKEGG_PAIN14,
	S_BLACKEGG_PAIN15,
	S_BLACKEGG_PAIN16,
	S_BLACKEGG_PAIN17,
	S_BLACKEGG_PAIN18,
	S_BLACKEGG_PAIN19,
	S_BLACKEGG_PAIN20,
	S_BLACKEGG_PAIN21,
	S_BLACKEGG_PAIN22,
	S_BLACKEGG_PAIN23,
	S_BLACKEGG_PAIN24,
	S_BLACKEGG_PAIN25,
	S_BLACKEGG_PAIN26,
	S_BLACKEGG_PAIN27,
	S_BLACKEGG_PAIN28,
	S_BLACKEGG_PAIN29,
	S_BLACKEGG_PAIN30,
	S_BLACKEGG_PAIN31,
	S_BLACKEGG_PAIN32,
	S_BLACKEGG_PAIN33,
	S_BLACKEGG_PAIN34,
	S_BLACKEGG_PAIN35,
	S_BLACKEGG_HITFACE1,
	S_BLACKEGG_HITFACE2,
	S_BLACKEGG_HITFACE3,
	S_BLACKEGG_HITFACE4,
	S_BLACKEGG_DIE1,
	S_BLACKEGG_DIE2,
	S_BLACKEGG_DIE3,
	S_BLACKEGG_DIE4,
	S_BLACKEGG_DIE5,
	S_BLACKEGG_MISSILE1,
	S_BLACKEGG_MISSILE2,
	S_BLACKEGG_MISSILE3,
	S_BLACKEGG_GOOP,
	S_BLACKEGG_JUMP1,
	S_BLACKEGG_JUMP2,
	S_BLACKEGG_DESTROYPLAT1,
	S_BLACKEGG_DESTROYPLAT2,
	S_BLACKEGG_DESTROYPLAT3,

	S_BLACKEGG_HELPER, // Collision helper

	S_BLACKEGG_GOOP1,
	S_BLACKEGG_GOOP2,
	S_BLACKEGG_GOOP3,
	S_BLACKEGG_GOOP4,
	S_BLACKEGG_GOOP5,
	S_BLACKEGG_GOOP6,
	S_BLACKEGG_GOOP7,

	S_BLACKEGG_MISSILE,

	// New Very-Last-Minute 2.1 Brak Eggman (Cy-Brak-demon)
	S_CYBRAKDEMON_IDLE,
	S_CYBRAKDEMON_WALK1,
	S_CYBRAKDEMON_WALK2,
	S_CYBRAKDEMON_WALK3,
	S_CYBRAKDEMON_WALK4,
	S_CYBRAKDEMON_WALK5,
	S_CYBRAKDEMON_WALK6,
	S_CYBRAKDEMON_CHOOSE_ATTACK1,
	S_CYBRAKDEMON_MISSILE_ATTACK1, // Aim
	S_CYBRAKDEMON_MISSILE_ATTACK2, // Fire
	S_CYBRAKDEMON_MISSILE_ATTACK3, // Aim
	S_CYBRAKDEMON_MISSILE_ATTACK4, // Fire
	S_CYBRAKDEMON_MISSILE_ATTACK5, // Aim
	S_CYBRAKDEMON_MISSILE_ATTACK6, // Fire
	S_CYBRAKDEMON_FLAME_ATTACK1, // Reset
	S_CYBRAKDEMON_FLAME_ATTACK2, // Aim
	S_CYBRAKDEMON_FLAME_ATTACK3, // Fire
	S_CYBRAKDEMON_FLAME_ATTACK4, // Loop
	S_CYBRAKDEMON_CHOOSE_ATTACK2,
	S_CYBRAKDEMON_VILE_ATTACK1,
	S_CYBRAKDEMON_VILE_ATTACK2,
	S_CYBRAKDEMON_VILE_ATTACK3,
	S_CYBRAKDEMON_VILE_ATTACK4,
	S_CYBRAKDEMON_VILE_ATTACK5,
	S_CYBRAKDEMON_VILE_ATTACK6,
	S_CYBRAKDEMON_NAPALM_ATTACK1,
	S_CYBRAKDEMON_NAPALM_ATTACK2,
	S_CYBRAKDEMON_NAPALM_ATTACK3,
	S_CYBRAKDEMON_FINISH_ATTACK1, // If just attacked, remove MF2_FRET w/out going back to spawnstate
	S_CYBRAKDEMON_FINISH_ATTACK2, // Force a delay between attacks so you don't get bombarded with them back-to-back
	S_CYBRAKDEMON_PAIN1,
	S_CYBRAKDEMON_PAIN2,
	S_CYBRAKDEMON_PAIN3,
	S_CYBRAKDEMON_DIE1,
	S_CYBRAKDEMON_DIE2,
	S_CYBRAKDEMON_DIE3,
	S_CYBRAKDEMON_DIE4,
	S_CYBRAKDEMON_DIE5,
	S_CYBRAKDEMON_DIE6,
	S_CYBRAKDEMON_DIE7,
	S_CYBRAKDEMON_DIE8,
	S_CYBRAKDEMON_DEINVINCIBLERIZE,
	S_CYBRAKDEMON_INVINCIBLERIZE,

	S_CYBRAKDEMONMISSILE,
	S_CYBRAKDEMONMISSILE_EXPLODE1,
	S_CYBRAKDEMONMISSILE_EXPLODE2,
	S_CYBRAKDEMONMISSILE_EXPLODE3,

	S_CYBRAKDEMONFLAMESHOT_FLY1,
	S_CYBRAKDEMONFLAMESHOT_FLY2,
	S_CYBRAKDEMONFLAMESHOT_FLY3,
	S_CYBRAKDEMONFLAMESHOT_DIE,

	S_CYBRAKDEMONFLAMEREST,

	S_CYBRAKDEMONELECTRICBARRIER_INIT1,
	S_CYBRAKDEMONELECTRICBARRIER_INIT2,
	S_CYBRAKDEMONELECTRICBARRIER_PLAYSOUND,
	S_CYBRAKDEMONELECTRICBARRIER1,
	S_CYBRAKDEMONELECTRICBARRIER2,
	S_CYBRAKDEMONELECTRICBARRIER3,
	S_CYBRAKDEMONELECTRICBARRIER4,
	S_CYBRAKDEMONELECTRICBARRIER5,
	S_CYBRAKDEMONELECTRICBARRIER6,
	S_CYBRAKDEMONELECTRICBARRIER7,
	S_CYBRAKDEMONELECTRICBARRIER8,
	S_CYBRAKDEMONELECTRICBARRIER9,
	S_CYBRAKDEMONELECTRICBARRIER10,
	S_CYBRAKDEMONELECTRICBARRIER11,
	S_CYBRAKDEMONELECTRICBARRIER12,
	S_CYBRAKDEMONELECTRICBARRIER13,
	S_CYBRAKDEMONELECTRICBARRIER14,
	S_CYBRAKDEMONELECTRICBARRIER15,
	S_CYBRAKDEMONELECTRICBARRIER16,
	S_CYBRAKDEMONELECTRICBARRIER17,
	S_CYBRAKDEMONELECTRICBARRIER18,
	S_CYBRAKDEMONELECTRICBARRIER19,
	S_CYBRAKDEMONELECTRICBARRIER20,
	S_CYBRAKDEMONELECTRICBARRIER21,
	S_CYBRAKDEMONELECTRICBARRIER22,
	S_CYBRAKDEMONELECTRICBARRIER23,
	S_CYBRAKDEMONELECTRICBARRIER24,
	S_CYBRAKDEMONELECTRICBARRIER_DIE1,
	S_CYBRAKDEMONELECTRICBARRIER_DIE2,
	S_CYBRAKDEMONELECTRICBARRIER_DIE3,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMCHECK,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMSUCCESS,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMCHOOSE,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM1,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM2,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM3,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM4,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM5,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM6,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM7,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM8,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM9,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM10,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM11,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOM12,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMFAIL,
	S_CYBRAKDEMONELECTRICBARRIER_SPARK_RANDOMLOOP,
	S_CYBRAKDEMONELECTRICBARRIER_REVIVE1,
	S_CYBRAKDEMONELECTRICBARRIER_REVIVE2,
	S_CYBRAKDEMONELECTRICBARRIER_REVIVE3,

	S_CYBRAKDEMONTARGETRETICULE1,
	S_CYBRAKDEMONTARGETRETICULE2,
	S_CYBRAKDEMONTARGETRETICULE3,
	S_CYBRAKDEMONTARGETRETICULE4,
	S_CYBRAKDEMONTARGETRETICULE5,
	S_CYBRAKDEMONTARGETRETICULE6,
	S_CYBRAKDEMONTARGETRETICULE7,
	S_CYBRAKDEMONTARGETRETICULE8,
	S_CYBRAKDEMONTARGETRETICULE9,
	S_CYBRAKDEMONTARGETRETICULE10,
	S_CYBRAKDEMONTARGETRETICULE11,
	S_CYBRAKDEMONTARGETRETICULE12,
	S_CYBRAKDEMONTARGETRETICULE13,
	S_CYBRAKDEMONTARGETRETICULE14,

	S_CYBRAKDEMONTARGETDOT,

	S_CYBRAKDEMONNAPALMBOMBLARGE_FLY1,
	S_CYBRAKDEMONNAPALMBOMBLARGE_FLY2,
	S_CYBRAKDEMONNAPALMBOMBLARGE_FLY3,
	S_CYBRAKDEMONNAPALMBOMBLARGE_FLY4,
	S_CYBRAKDEMONNAPALMBOMBLARGE_DIE1, // Explode
	S_CYBRAKDEMONNAPALMBOMBLARGE_DIE2, // Outer ring
	S_CYBRAKDEMONNAPALMBOMBLARGE_DIE3, // Center
	S_CYBRAKDEMONNAPALMBOMBLARGE_DIE4, // Sound

	S_CYBRAKDEMONNAPALMBOMBSMALL,
	S_CYBRAKDEMONNAPALMBOMBSMALL_DIE1, // Explode
	S_CYBRAKDEMONNAPALMBOMBSMALL_DIE2, // Outer ring
	S_CYBRAKDEMONNAPALMBOMBSMALL_DIE3, // Inner ring
	S_CYBRAKDEMONNAPALMBOMBSMALL_DIE4, // Center
	S_CYBRAKDEMONNAPALMBOMBSMALL_DIE5, // Sound

	S_CYBRAKDEMONNAPALMFLAME_FLY1,
	S_CYBRAKDEMONNAPALMFLAME_FLY2,
	S_CYBRAKDEMONNAPALMFLAME_FLY3,
	S_CYBRAKDEMONNAPALMFLAME_FLY4,
	S_CYBRAKDEMONNAPALMFLAME_FLY5,
	S_CYBRAKDEMONNAPALMFLAME_FLY6,
	S_CYBRAKDEMONNAPALMFLAME_DIE,

	S_CYBRAKDEMONVILEEXPLOSION1,
	S_CYBRAKDEMONVILEEXPLOSION2,
	S_CYBRAKDEMONVILEEXPLOSION3,

	// Metal Sonic (Race)
	S_METALSONIC_RACE,
	// Metal Sonic (Battle)
	S_METALSONIC_FLOAT,
	S_METALSONIC_VECTOR,
	S_METALSONIC_STUN,
	S_METALSONIC_RAISE,
	S_METALSONIC_GATHER,
	S_METALSONIC_DASH,
	S_METALSONIC_BOUNCE,
	S_METALSONIC_BADBOUNCE,
	S_METALSONIC_SHOOT,
	S_METALSONIC_PAIN,
	S_METALSONIC_DEATH1,
	S_METALSONIC_DEATH2,
	S_METALSONIC_DEATH3,
	S_METALSONIC_DEATH4,
	S_METALSONIC_FLEE1,
	S_METALSONIC_FLEE2,

	S_MSSHIELD_F1,
	S_MSSHIELD_F2,

	// Ring
	S_RING,

	// Blue Sphere for special stages
	S_BLUESPHERE,
	S_BLUESPHEREBONUS,
	S_BLUESPHERESPARK,

	// Bomb Sphere
	S_BOMBSPHERE1,
	S_BOMBSPHERE2,
	S_BOMBSPHERE3,
	S_BOMBSPHERE4,

	// NiGHTS Chip
	S_NIGHTSCHIP,
	S_NIGHTSCHIPBONUS,

	// NiGHTS Star
	S_NIGHTSSTAR,
	S_NIGHTSSTARXMAS,

	// Gravity Wells for special stages
	S_GRAVWELLGREEN,
	S_GRAVWELLRED,

	// Individual Team Rings
	S_TEAMRING,

	// Special Stage Token
	S_TOKEN,

	// CTF Flags
	S_REDFLAG,
	S_BLUEFLAG,

	// Emblem
	S_EMBLEM1,
	S_EMBLEM2,
	S_EMBLEM3,
	S_EMBLEM4,
	S_EMBLEM5,
	S_EMBLEM6,
	S_EMBLEM7,
	S_EMBLEM8,
	S_EMBLEM9,
	S_EMBLEM10,
	S_EMBLEM11,
	S_EMBLEM12,
	S_EMBLEM13,
	S_EMBLEM14,
	S_EMBLEM15,
	S_EMBLEM16,
	S_EMBLEM17,
	S_EMBLEM18,
	S_EMBLEM19,
	S_EMBLEM20,
	S_EMBLEM21,
	S_EMBLEM22,
	S_EMBLEM23,
	S_EMBLEM24,
	S_EMBLEM25,
	S_EMBLEM26,

	// Chaos Emeralds
	S_CEMG1,
	S_CEMG2,
	S_CEMG3,
	S_CEMG4,
	S_CEMG5,
	S_CEMG6,
	S_CEMG7,

	// Emerald hunt shards
	S_SHRD1,
	S_SHRD2,
	S_SHRD3,

	// Bubble Source
	S_BUBBLES1,
	S_BUBBLES2,
	S_BUBBLES3,
	S_BUBBLES4,

	// Level End Sign
	S_SIGN,
	S_SIGNSPIN1,
	S_SIGNSPIN2,
	S_SIGNSPIN3,
	S_SIGNSPIN4,
	S_SIGNSPIN5,
	S_SIGNSPIN6,
	S_SIGNPLAYER,
	S_SIGNSLOW,
	S_SIGNSTOP,
	S_SIGNBOARD,
	S_EGGMANSIGN,
	S_CLEARSIGN,

	// Spike Ball
	S_SPIKEBALL1,
	S_SPIKEBALL2,
	S_SPIKEBALL3,
	S_SPIKEBALL4,
	S_SPIKEBALL5,
	S_SPIKEBALL6,
	S_SPIKEBALL7,
	S_SPIKEBALL8,

	// Elemental Shield's Spawn
	S_SPINFIRE1,
	S_SPINFIRE2,
	S_SPINFIRE3,
	S_SPINFIRE4,
	S_SPINFIRE5,
	S_SPINFIRE6,

	// Spikes
	S_SPIKE1,
	S_SPIKE2,
	S_SPIKE3,
	S_SPIKE4,
	S_SPIKE5,
	S_SPIKE6,
	S_SPIKED1,
	S_SPIKED2,

	// Wall spikes
	S_WALLSPIKE1,
	S_WALLSPIKE2,
	S_WALLSPIKE3,
	S_WALLSPIKE4,
	S_WALLSPIKE5,
	S_WALLSPIKE6,
	S_WALLSPIKEBASE,
	S_WALLSPIKED1,
	S_WALLSPIKED2,

	// Starpost
	S_STARPOST_IDLE,
	S_STARPOST_FLASH,
	S_STARPOST_STARTSPIN,
	S_STARPOST_SPIN,
	S_STARPOST_ENDSPIN,

	// Big floating mine
	S_BIGMINE_IDLE,
	S_BIGMINE_ALERT1,
	S_BIGMINE_ALERT2,
	S_BIGMINE_ALERT3,
	S_BIGMINE_SET1,
	S_BIGMINE_SET2,
	S_BIGMINE_SET3,
	S_BIGMINE_BLAST1,
	S_BIGMINE_BLAST2,
	S_BIGMINE_BLAST3,
	S_BIGMINE_BLAST4,
	S_BIGMINE_BLAST5,

	// Cannon Launcher
	S_CANNONLAUNCHER1,
	S_CANNONLAUNCHER2,
	S_CANNONLAUNCHER3,

	// Monitor Miscellany
	S_BOXSPARKLE1,
	S_BOXSPARKLE2,
	S_BOXSPARKLE3,
	S_BOXSPARKLE4,

	S_BOX_FLICKER,
	S_BOX_POP1,
	S_BOX_POP2,

	S_GOLDBOX_FLICKER,
	S_GOLDBOX_OFF1,
	S_GOLDBOX_OFF2,
	S_GOLDBOX_OFF3,
	S_GOLDBOX_OFF4,
	S_GOLDBOX_OFF5,
	S_GOLDBOX_OFF6,
	S_GOLDBOX_OFF7,

	// Monitor States (one per box)
	S_MYSTERY_BOX,
	S_RING_BOX,
	S_PITY_BOX,
	S_ATTRACT_BOX,
	S_FORCE_BOX,
	S_ARMAGEDDON_BOX,
	S_WHIRLWIND_BOX,
	S_ELEMENTAL_BOX,
	S_SNEAKERS_BOX,
	S_INVULN_BOX,
	S_1UP_BOX,
	S_EGGMAN_BOX,
	S_MIXUP_BOX,
	S_GRAVITY_BOX,
	S_RECYCLER_BOX,
	S_SCORE1K_BOX,
	S_SCORE10K_BOX,
	S_FLAMEAURA_BOX,
	S_BUBBLEWRAP_BOX,
	S_THUNDERCOIN_BOX,

	// Gold Repeat Monitor States (one per box)
	S_PITY_GOLDBOX,
	S_ATTRACT_GOLDBOX,
	S_FORCE_GOLDBOX,
	S_ARMAGEDDON_GOLDBOX,
	S_WHIRLWIND_GOLDBOX,
	S_ELEMENTAL_GOLDBOX,
	S_SNEAKERS_GOLDBOX,
	S_INVULN_GOLDBOX,
	S_EGGMAN_GOLDBOX,
	S_GRAVITY_GOLDBOX,
	S_FLAMEAURA_GOLDBOX,
	S_BUBBLEWRAP_GOLDBOX,
	S_THUNDERCOIN_GOLDBOX,

	// Team Ring Boxes (these are special)
	S_RING_REDBOX1,
	S_RING_REDBOX2,
	S_REDBOX_POP1,
	S_REDBOX_POP2,

	S_RING_BLUEBOX1,
	S_RING_BLUEBOX2,
	S_BLUEBOX_POP1,
	S_BLUEBOX_POP2,

	// Box Icons -- 2 states each, animation and action
	S_RING_ICON1,
	S_RING_ICON2,

	S_PITY_ICON1,
	S_PITY_ICON2,

	S_ATTRACT_ICON1,
	S_ATTRACT_ICON2,

	S_FORCE_ICON1,
	S_FORCE_ICON2,

	S_ARMAGEDDON_ICON1,
	S_ARMAGEDDON_ICON2,

	S_WHIRLWIND_ICON1,
	S_WHIRLWIND_ICON2,

	S_ELEMENTAL_ICON1,
	S_ELEMENTAL_ICON2,

	S_SNEAKERS_ICON1,
	S_SNEAKERS_ICON2,

	S_INVULN_ICON1,
	S_INVULN_ICON2,

	S_1UP_ICON1,
	S_1UP_ICON2,

	S_EGGMAN_ICON1,
	S_EGGMAN_ICON2,

	S_MIXUP_ICON1,
	S_MIXUP_ICON2,

	S_GRAVITY_ICON1,
	S_GRAVITY_ICON2,

	S_RECYCLER_ICON1,
	S_RECYCLER_ICON2,

	S_SCORE1K_ICON1,
	S_SCORE1K_ICON2,

	S_SCORE10K_ICON1,
	S_SCORE10K_ICON2,

	S_FLAMEAURA_ICON1,
	S_FLAMEAURA_ICON2,

	S_BUBBLEWRAP_ICON1,
	S_BUBBLEWRAP_ICON2,

	S_THUNDERCOIN_ICON1,
	S_THUNDERCOIN_ICON2,

	// ---

	S_ROCKET,

	S_LASER,
	S_LASER2,
	S_LASERFLASH,

	S_LASERFLAME1,
	S_LASERFLAME2,
	S_LASERFLAME3,
	S_LASERFLAME4,
	S_LASERFLAME5,

	S_TORPEDO,

	S_ENERGYBALL1,
	S_ENERGYBALL2,

	// Skim Mine, also used by Jetty-Syn bomber
	S_MINE1,
	S_MINE_BOOM1,
	S_MINE_BOOM2,
	S_MINE_BOOM3,
	S_MINE_BOOM4,

	// Jetty-Syn Bullet
	S_JETBULLET1,
	S_JETBULLET2,

	S_TURRETLASER,
	S_TURRETLASEREXPLODE1,
	S_TURRETLASEREXPLODE2,

	// Cannonball
	S_CANNONBALL1,

	// Arrow
	S_ARROW,
	S_ARROWBONK,

	// Glaregoyle Demon fire
	S_DEMONFIRE,

	// The letter
	S_LETTER,

	// GFZ flowers
	S_GFZFLOWERA,
	S_GFZFLOWERB,
	S_GFZFLOWERC,

	S_BLUEBERRYBUSH,
	S_BERRYBUSH,
	S_BUSH,

	// Trees (both GFZ and misc)
	S_GFZTREE,
	S_GFZBERRYTREE,
	S_GFZCHERRYTREE,
	S_CHECKERTREE,
	S_CHECKERSUNSETTREE,
	S_FHZTREE, // Frozen Hillside
	S_FHZPINKTREE,
	S_POLYGONTREE,
	S_BUSHTREE,
	S_BUSHREDTREE,
	S_SPRINGTREE,

	// THZ flowers
	S_THZFLOWERA, // THZ1 Steam flower
	S_THZFLOWERB, // THZ1 Spin flower (red)
	S_THZFLOWERC, // THZ1 Spin flower (yellow)

	// THZ Steam Whistle tree/bush
	S_THZTREE,
	S_THZTREEBRANCH1,
	S_THZTREEBRANCH2,
	S_THZTREEBRANCH3,
	S_THZTREEBRANCH4,
	S_THZTREEBRANCH5,
	S_THZTREEBRANCH6,
	S_THZTREEBRANCH7,
	S_THZTREEBRANCH8,
	S_THZTREEBRANCH9,
	S_THZTREEBRANCH10,
	S_THZTREEBRANCH11,
	S_THZTREEBRANCH12,
	S_THZTREEBRANCH13,

	// THZ Alarm
	S_ALARM1,

	// Deep Sea Gargoyle
	S_GARGOYLE,
	S_BIGGARGOYLE,

	// DSZ Seaweed
	S_SEAWEED1,
	S_SEAWEED2,
	S_SEAWEED3,
	S_SEAWEED4,
	S_SEAWEED5,
	S_SEAWEED6,

	// Dripping Water
	S_DRIPA1,
	S_DRIPA2,
	S_DRIPA3,
	S_DRIPA4,
	S_DRIPB1,
	S_DRIPC1,
	S_DRIPC2,

	// Coral
	S_CORAL1,
	S_CORAL2,
	S_CORAL3,
	S_CORAL4,
	S_CORAL5,

	// Blue Crystal
	S_BLUECRYSTAL1,

	// Kelp,
	S_KELP,

	// Animated algae
	S_ANIMALGAETOP1,
	S_ANIMALGAETOP2,
	S_ANIMALGAESEG,

	// DSZ Stalagmites
	S_DSZSTALAGMITE,
	S_DSZ2STALAGMITE,

	// DSZ Light beam
	S_LIGHTBEAM1,
	S_LIGHTBEAM2,
	S_LIGHTBEAM3,
	S_LIGHTBEAM4,
	S_LIGHTBEAM5,
	S_LIGHTBEAM6,
	S_LIGHTBEAM7,
	S_LIGHTBEAM8,
	S_LIGHTBEAM9,
	S_LIGHTBEAM10,
	S_LIGHTBEAM11,
	S_LIGHTBEAM12,

	// CEZ Chain
	S_CEZCHAIN,

	// Flame
	S_FLAME,
	S_FLAMEPARTICLE,
	S_FLAMEREST,

	// Eggman Statue
	S_EGGSTATUE1,

	// CEZ hidden sling
	S_SLING1,
	S_SLING2,

	// CEZ maces and chains
	S_SMALLMACECHAIN,
	S_BIGMACECHAIN,
	S_SMALLMACE,
	S_BIGMACE,
	S_SMALLGRABCHAIN,
	S_BIGGRABCHAIN,

	// Yellow spring on a ball
	S_YELLOWSPRINGBALL,
	S_YELLOWSPRINGBALL2,
	S_YELLOWSPRINGBALL3,
	S_YELLOWSPRINGBALL4,
	S_YELLOWSPRINGBALL5,

	// Red spring on a ball
	S_REDSPRINGBALL,
	S_REDSPRINGBALL2,
	S_REDSPRINGBALL3,
	S_REDSPRINGBALL4,
	S_REDSPRINGBALL5,

	// Small Firebar
	S_SMALLFIREBAR1,
	S_SMALLFIREBAR2,
	S_SMALLFIREBAR3,
	S_SMALLFIREBAR4,
	S_SMALLFIREBAR5,
	S_SMALLFIREBAR6,
	S_SMALLFIREBAR7,
	S_SMALLFIREBAR8,
	S_SMALLFIREBAR9,
	S_SMALLFIREBAR10,
	S_SMALLFIREBAR11,
	S_SMALLFIREBAR12,
	S_SMALLFIREBAR13,
	S_SMALLFIREBAR14,
	S_SMALLFIREBAR15,
	S_SMALLFIREBAR16,

	// Big Firebar
	S_BIGFIREBAR1,
	S_BIGFIREBAR2,
	S_BIGFIREBAR3,
	S_BIGFIREBAR4,
	S_BIGFIREBAR5,
	S_BIGFIREBAR6,
	S_BIGFIREBAR7,
	S_BIGFIREBAR8,
	S_BIGFIREBAR9,
	S_BIGFIREBAR10,
	S_BIGFIREBAR11,
	S_BIGFIREBAR12,
	S_BIGFIREBAR13,
	S_BIGFIREBAR14,
	S_BIGFIREBAR15,
	S_BIGFIREBAR16,

	S_CEZFLOWER,
	S_CEZPOLE,
	S_CEZBANNER1,
	S_CEZBANNER2,
	S_PINETREE,
	S_CEZBUSH1,
	S_CEZBUSH2,
	S_CANDLE,
	S_CANDLEPRICKET,
	S_FLAMEHOLDER,
	S_FIRETORCH,
	S_WAVINGFLAG,
	S_WAVINGFLAGSEG1,
	S_WAVINGFLAGSEG2,
	S_CRAWLASTATUE,
	S_FACESTABBERSTATUE,
	S_SUSPICIOUSFACESTABBERSTATUE_WAIT,
	S_SUSPICIOUSFACESTABBERSTATUE_BURST1,
	S_SUSPICIOUSFACESTABBERSTATUE_BURST2,
	S_BRAMBLES,

	// Big Tumbleweed
	S_BIGTUMBLEWEED,
	S_BIGTUMBLEWEED_ROLL1,
	S_BIGTUMBLEWEED_ROLL2,
	S_BIGTUMBLEWEED_ROLL3,
	S_BIGTUMBLEWEED_ROLL4,
	S_BIGTUMBLEWEED_ROLL5,
	S_BIGTUMBLEWEED_ROLL6,
	S_BIGTUMBLEWEED_ROLL7,
	S_BIGTUMBLEWEED_ROLL8,

	// Little Tumbleweed
	S_LITTLETUMBLEWEED,
	S_LITTLETUMBLEWEED_ROLL1,
	S_LITTLETUMBLEWEED_ROLL2,
	S_LITTLETUMBLEWEED_ROLL3,
	S_LITTLETUMBLEWEED_ROLL4,
	S_LITTLETUMBLEWEED_ROLL5,
	S_LITTLETUMBLEWEED_ROLL6,
	S_LITTLETUMBLEWEED_ROLL7,
	S_LITTLETUMBLEWEED_ROLL8,

	// Cacti
	S_CACTI1,
	S_CACTI2,
	S_CACTI3,
	S_CACTI4,
	S_CACTI5,
	S_CACTI6,
	S_CACTI7,
	S_CACTI8,
	S_CACTI9,
	S_CACTI10,
	S_CACTI11,
	S_CACTITINYSEG,
	S_CACTISMALLSEG,

	// Warning signs
	S_ARIDSIGN_CAUTION,
	S_ARIDSIGN_CACTI,
	S_ARIDSIGN_SHARPTURN,

	// Oil lamp
	S_OILLAMP,
	S_OILLAMPFLARE,

	// TNT barrel
	S_TNTBARREL_STND1,
	S_TNTBARREL_EXPL1,
	S_TNTBARREL_EXPL2,
	S_TNTBARREL_EXPL3,
	S_TNTBARREL_EXPL4,
	S_TNTBARREL_EXPL5,
	S_TNTBARREL_EXPL6,
	S_TNTBARREL_EXPL7,
	S_TNTBARREL_FLYING,

	// TNT proximity shell
	S_PROXIMITY_TNT,
	S_PROXIMITY_TNT_TRIGGER1,
	S_PROXIMITY_TNT_TRIGGER2,
	S_PROXIMITY_TNT_TRIGGER3,
	S_PROXIMITY_TNT_TRIGGER4,
	S_PROXIMITY_TNT_TRIGGER5,
	S_PROXIMITY_TNT_TRIGGER6,
	S_PROXIMITY_TNT_TRIGGER7,
	S_PROXIMITY_TNT_TRIGGER8,
	S_PROXIMITY_TNT_TRIGGER9,
	S_PROXIMITY_TNT_TRIGGER10,
	S_PROXIMITY_TNT_TRIGGER11,
	S_PROXIMITY_TNT_TRIGGER12,
	S_PROXIMITY_TNT_TRIGGER13,
	S_PROXIMITY_TNT_TRIGGER14,
	S_PROXIMITY_TNT_TRIGGER15,
	S_PROXIMITY_TNT_TRIGGER16,
	S_PROXIMITY_TNT_TRIGGER17,
	S_PROXIMITY_TNT_TRIGGER18,
	S_PROXIMITY_TNT_TRIGGER19,
	S_PROXIMITY_TNT_TRIGGER20,
	S_PROXIMITY_TNT_TRIGGER21,
	S_PROXIMITY_TNT_TRIGGER22,
	S_PROXIMITY_TNT_TRIGGER23,

	// Dust devil
	S_DUSTDEVIL,
	S_DUSTLAYER1,
	S_DUSTLAYER2,
	S_DUSTLAYER3,
	S_DUSTLAYER4,
	S_DUSTLAYER5,
	S_ARIDDUST1,
	S_ARIDDUST2,
	S_ARIDDUST3,

	// Minecart
	S_MINECART_IDLE,
	S_MINECART_DTH1,
	S_MINECARTEND,
	S_MINECARTSEG_FRONT,
	S_MINECARTSEG_BACK,
	S_MINECARTSEG_LEFT,
	S_MINECARTSEG_RIGHT,
	S_MINECARTSIDEMARK1,
	S_MINECARTSIDEMARK2,
	S_MINECARTSPARK,

	// Saloon door
	S_SALOONDOOR,
	S_SALOONDOORCENTER,

	// Train cameo
	S_TRAINCAMEOSPAWNER_1,
	S_TRAINCAMEOSPAWNER_2,
	S_TRAINCAMEOSPAWNER_3,
	S_TRAINCAMEOSPAWNER_4,
	S_TRAINCAMEOSPAWNER_5,
	S_TRAINPUFFMAKER,

	// Train
	S_TRAINDUST,
	S_TRAINSTEAM,

	// Flame jet
	S_FLAMEJETSTND,
	S_FLAMEJETSTART,
	S_FLAMEJETSTOP,
	S_FLAMEJETFLAME1,
	S_FLAMEJETFLAME2,
	S_FLAMEJETFLAME3,
	S_FLAMEJETFLAME4,
	S_FLAMEJETFLAME5,
	S_FLAMEJETFLAME6,
	S_FLAMEJETFLAME7,
	S_FLAMEJETFLAME8,
	S_FLAMEJETFLAME9,

	// Spinning flame jets
	S_FJSPINAXISA1, // Counter-clockwise
	S_FJSPINAXISA2,
	S_FJSPINAXISB1, // Clockwise
	S_FJSPINAXISB2,

	// Blade's flame
	S_FLAMEJETFLAMEB1,
	S_FLAMEJETFLAMEB2,
	S_FLAMEJETFLAMEB3,

	// Lavafall
	S_LAVAFALL_DORMANT,
	S_LAVAFALL_TELL,
	S_LAVAFALL_SHOOT,
	S_LAVAFALL_LAVA1,
	S_LAVAFALL_LAVA2,
	S_LAVAFALL_LAVA3,
	S_LAVAFALLROCK,

	// Rollout Rock
	S_ROLLOUTSPAWN,
	S_ROLLOUTROCK,

	// RVZ scenery
	S_BIGFERNLEAF,
	S_BIGFERN1,
	S_BIGFERN2,
	S_JUNGLEPALM,
	S_TORCHFLOWER,
	S_WALLVINE_LONG,
	S_WALLVINE_SHORT,

	// Glaregoyles
	S_GLAREGOYLE,
	S_GLAREGOYLE_CHARGE,
	S_GLAREGOYLE_BLINK,
	S_GLAREGOYLE_HOLD,
	S_GLAREGOYLE_FIRE,
	S_GLAREGOYLE_LOOP,
	S_GLAREGOYLE_COOLDOWN,
	S_GLAREGOYLEUP,
	S_GLAREGOYLEUP_CHARGE,
	S_GLAREGOYLEUP_BLINK,
	S_GLAREGOYLEUP_HOLD,
	S_GLAREGOYLEUP_FIRE,
	S_GLAREGOYLEUP_LOOP,
	S_GLAREGOYLEUP_COOLDOWN,
	S_GLAREGOYLEDOWN,
	S_GLAREGOYLEDOWN_CHARGE,
	S_GLAREGOYLEDOWN_BLINK,
	S_GLAREGOYLEDOWN_HOLD,
	S_GLAREGOYLEDOWN_FIRE,
	S_GLAREGOYLEDOWN_LOOP,
	S_GLAREGOYLEDOWN_COOLDOWN,
	S_GLAREGOYLELONG,
	S_GLAREGOYLELONG_CHARGE,
	S_GLAREGOYLELONG_BLINK,
	S_GLAREGOYLELONG_HOLD,
	S_GLAREGOYLELONG_FIRE,
	S_GLAREGOYLELONG_LOOP,
	S_GLAREGOYLELONG_COOLDOWN,

	// ATZ's Red Crystal/Target
	S_TARGET_IDLE,
	S_TARGET_HIT1,
	S_TARGET_HIT2,
	S_TARGET_RESPAWN,
	S_TARGET_ALLDONE,

	// ATZ's green flame
	S_GREENFLAME,

	// ATZ Blue Gargoyle
	S_BLUEGARGOYLE,

	// Stalagmites
	S_STG0,
	S_STG1,
	S_STG2,
	S_STG3,
	S_STG4,
	S_STG5,
	S_STG6,
	S_STG7,
	S_STG8,
	S_STG9,

	// Xmas-specific stuff
	S_XMASPOLE,
	S_CANDYCANE,
	S_SNOWMAN,    // normal
	S_SNOWMANHAT, // with hat + scarf
	S_LAMPPOST1,  // normal
	S_LAMPPOST2,  // with snow
	S_HANGSTAR,
	S_MISTLETOE,
	// Xmas GFZ bushes
	S_XMASBLUEBERRYBUSH,
	S_XMASBERRYBUSH,
	S_XMASBUSH,
	// FHZ
	S_FHZICE1,
	S_FHZICE2,
	S_ROSY_IDLE1,
	S_ROSY_IDLE2,
	S_ROSY_IDLE3,
	S_ROSY_IDLE4,
	S_ROSY_JUMP,
	S_ROSY_WALK,
	S_ROSY_HUG,
	S_ROSY_PAIN,
	S_ROSY_STND,
	S_ROSY_UNHAPPY,

	// Halloween Scenery
	// Pumpkins
	S_JACKO1,
	S_JACKO1OVERLAY_1,
	S_JACKO1OVERLAY_2,
	S_JACKO1OVERLAY_3,
	S_JACKO1OVERLAY_4,
	S_JACKO2,
	S_JACKO2OVERLAY_1,
	S_JACKO2OVERLAY_2,
	S_JACKO2OVERLAY_3,
	S_JACKO2OVERLAY_4,
	S_JACKO3,
	S_JACKO3OVERLAY_1,
	S_JACKO3OVERLAY_2,
	S_JACKO3OVERLAY_3,
	S_JACKO3OVERLAY_4,
	// Dr Seuss Trees
	S_HHZTREE_TOP,
	S_HHZTREE_TRUNK,
	S_HHZTREE_LEAF,
	// Mushroom
	S_HHZSHROOM_1,
	S_HHZSHROOM_2,
	S_HHZSHROOM_3,
	S_HHZSHROOM_4,
	S_HHZSHROOM_5,
	S_HHZSHROOM_6,
	S_HHZSHROOM_7,
	S_HHZSHROOM_8,
	S_HHZSHROOM_9,
	S_HHZSHROOM_10,
	S_HHZSHROOM_11,
	S_HHZSHROOM_12,
	S_HHZSHROOM_13,
	S_HHZSHROOM_14,
	S_HHZSHROOM_15,
	S_HHZSHROOM_16,
	// Misc
	S_HHZGRASS,
	S_HHZTENT1,
	S_HHZTENT2,
	S_HHZSTALAGMITE_TALL,
	S_HHZSTALAGMITE_SHORT,

	// Botanic Serenity's loads of scenery states
	S_BSZTALLFLOWER_RED,
	S_BSZTALLFLOWER_PURPLE,
	S_BSZTALLFLOWER_BLUE,
	S_BSZTALLFLOWER_CYAN,
	S_BSZTALLFLOWER_YELLOW,
	S_BSZTALLFLOWER_ORANGE,
	S_BSZFLOWER_RED,
	S_BSZFLOWER_PURPLE,
	S_BSZFLOWER_BLUE,
	S_BSZFLOWER_CYAN,
	S_BSZFLOWER_YELLOW,
	S_BSZFLOWER_ORANGE,
	S_BSZSHORTFLOWER_RED,
	S_BSZSHORTFLOWER_PURPLE,
	S_BSZSHORTFLOWER_BLUE,
	S_BSZSHORTFLOWER_CYAN,
	S_BSZSHORTFLOWER_YELLOW,
	S_BSZSHORTFLOWER_ORANGE,
	S_BSZTULIP_RED,
	S_BSZTULIP_PURPLE,
	S_BSZTULIP_BLUE,
	S_BSZTULIP_CYAN,
	S_BSZTULIP_YELLOW,
	S_BSZTULIP_ORANGE,
	S_BSZCLUSTER_RED,
	S_BSZCLUSTER_PURPLE,
	S_BSZCLUSTER_BLUE,
	S_BSZCLUSTER_CYAN,
	S_BSZCLUSTER_YELLOW,
	S_BSZCLUSTER_ORANGE,
	S_BSZBUSH_RED,
	S_BSZBUSH_PURPLE,
	S_BSZBUSH_BLUE,
	S_BSZBUSH_CYAN,
	S_BSZBUSH_YELLOW,
	S_BSZBUSH_ORANGE,
	S_BSZVINE_RED,
	S_BSZVINE_PURPLE,
	S_BSZVINE_BLUE,
	S_BSZVINE_CYAN,
	S_BSZVINE_YELLOW,
	S_BSZVINE_ORANGE,
	S_BSZSHRUB,
	S_BSZCLOVER,
	S_BIG_PALMTREE_TRUNK,
	S_BIG_PALMTREE_TOP,
	S_PALMTREE_TRUNK,
	S_PALMTREE_TOP,

	S_DBALL1,
	S_DBALL2,
	S_DBALL3,
	S_DBALL4,
	S_DBALL5,
	S_DBALL6,
	S_EGGSTATUE2,

	// Shield Orb
	S_ARMA1,
	S_ARMA2,
	S_ARMA3,
	S_ARMA4,
	S_ARMA5,
	S_ARMA6,
	S_ARMA7,
	S_ARMA8,
	S_ARMA9,
	S_ARMA10,
	S_ARMA11,
	S_ARMA12,
	S_ARMA13,
	S_ARMA14,
	S_ARMA15,
	S_ARMA16,

	S_ARMF1,
	S_ARMF2,
	S_ARMF3,
	S_ARMF4,
	S_ARMF5,
	S_ARMF6,
	S_ARMF7,
	S_ARMF8,
	S_ARMF9,
	S_ARMF10,
	S_ARMF11,
	S_ARMF12,
	S_ARMF13,
	S_ARMF14,
	S_ARMF15,
	S_ARMF16,
	S_ARMF17,
	S_ARMF18,
	S_ARMF19,
	S_ARMF20,
	S_ARMF21,
	S_ARMF22,
	S_ARMF23,
	S_ARMF24,
	S_ARMF25,
	S_ARMF26,
	S_ARMF27,
	S_ARMF28,
	S_ARMF29,
	S_ARMF30,
	S_ARMF31,
	S_ARMF32,

	S_ARMB1,
	S_ARMB2,
	S_ARMB3,
	S_ARMB4,
	S_ARMB5,
	S_ARMB6,
	S_ARMB7,
	S_ARMB8,
	S_ARMB9,
	S_ARMB10,
	S_ARMB11,
	S_ARMB12,
	S_ARMB13,
	S_ARMB14,
	S_ARMB15,
	S_ARMB16,
	S_ARMB17,
	S_ARMB18,
	S_ARMB19,
	S_ARMB20,
	S_ARMB21,
	S_ARMB22,
	S_ARMB23,
	S_ARMB24,
	S_ARMB25,
	S_ARMB26,
	S_ARMB27,
	S_ARMB28,
	S_ARMB29,
	S_ARMB30,
	S_ARMB31,
	S_ARMB32,

	S_WIND1,
	S_WIND2,
	S_WIND3,
	S_WIND4,
	S_WIND5,
	S_WIND6,
	S_WIND7,
	S_WIND8,

	S_MAGN1,
	S_MAGN2,
	S_MAGN3,
	S_MAGN4,
	S_MAGN5,
	S_MAGN6,
	S_MAGN7,
	S_MAGN8,
	S_MAGN9,
	S_MAGN10,
	S_MAGN11,
	S_MAGN12,
	S_MAGN13,

	S_FORC1,
	S_FORC2,
	S_FORC3,
	S_FORC4,
	S_FORC5,
	S_FORC6,
	S_FORC7,
	S_FORC8,
	S_FORC9,
	S_FORC10,

	S_FORC11,
	S_FORC12,
	S_FORC13,
	S_FORC14,
	S_FORC15,
	S_FORC16,
	S_FORC17,
	S_FORC18,
	S_FORC19,
	S_FORC20,

	S_FORC21,

	S_ELEM1,
	S_ELEM2,
	S_ELEM3,
	S_ELEM4,
	S_ELEM5,
	S_ELEM6,
	S_ELEM7,
	S_ELEM8,
	S_ELEM9,
	S_ELEM10,
	S_ELEM11,
	S_ELEM12,

	S_ELEM13,
	S_ELEM14,

	S_ELEMF1,
	S_ELEMF2,
	S_ELEMF3,
	S_ELEMF4,
	S_ELEMF5,
	S_ELEMF6,
	S_ELEMF7,
	S_ELEMF8,
	S_ELEMF9,
	S_ELEMF10,

	S_PITY1,
	S_PITY2,
	S_PITY3,
	S_PITY4,
	S_PITY5,
	S_PITY6,
	S_PITY7,
	S_PITY8,
	S_PITY9,
	S_PITY10,
	S_PITY11,
	S_PITY12,

	S_FIRS1,
	S_FIRS2,
	S_FIRS3,
	S_FIRS4,
	S_FIRS5,
	S_FIRS6,
	S_FIRS7,
	S_FIRS8,
	S_FIRS9,

	S_FIRS10,
	S_FIRS11,

	S_FIRSB1,
	S_FIRSB2,
	S_FIRSB3,
	S_FIRSB4,
	S_FIRSB5,
	S_FIRSB6,
	S_FIRSB7,
	S_FIRSB8,
	S_FIRSB9,

	S_FIRSB10,

	S_BUBS1,
	S_BUBS2,
	S_BUBS3,
	S_BUBS4,
	S_BUBS5,
	S_BUBS6,
	S_BUBS7,
	S_BUBS8,
	S_BUBS9,

	S_BUBS10,
	S_BUBS11,

	S_BUBSB1,
	S_BUBSB2,
	S_BUBSB3,
	S_BUBSB4,

	S_BUBSB5,
	S_BUBSB6,

	S_ZAPS1,
	S_ZAPS2,
	S_ZAPS3,
	S_ZAPS4,
	S_ZAPS5,
	S_ZAPS6,
	S_ZAPS7,
	S_ZAPS8,
	S_ZAPS9,
	S_ZAPS10,
	S_ZAPS11,
	S_ZAPS12,
	S_ZAPS13, // blank frame
	S_ZAPS14,
	S_ZAPS15,
	S_ZAPS16,

	S_ZAPSB1, // blank frame
	S_ZAPSB2,
	S_ZAPSB3,
	S_ZAPSB4,
	S_ZAPSB5,
	S_ZAPSB6,
	S_ZAPSB7,
	S_ZAPSB8,
	S_ZAPSB9,
	S_ZAPSB10,
	S_ZAPSB11, // blank frame

	//Thunder spark
	S_THUNDERCOIN_SPARK,

	// Invincibility Sparkles
	S_IVSP,

	// Super Sonic Spark
	S_SSPK1,
	S_SSPK2,
	S_SSPK3,
	S_SSPK4,
	S_SSPK5,

	// Flicky-sized bubble
	S_FLICKY_BUBBLE,

	// Bluebird
	S_FLICKY_01_OUT,
	S_FLICKY_01_FLAP1,
	S_FLICKY_01_FLAP2,
	S_FLICKY_01_FLAP3,
	S_FLICKY_01_STAND,
	S_FLICKY_01_CENTER,

	// Rabbit
	S_FLICKY_02_OUT,
	S_FLICKY_02_AIM,
	S_FLICKY_02_HOP,
	S_FLICKY_02_UP,
	S_FLICKY_02_DOWN,
	S_FLICKY_02_STAND,
	S_FLICKY_02_CENTER,

	// Chicken
	S_FLICKY_03_OUT,
	S_FLICKY_03_AIM,
	S_FLICKY_03_HOP,
	S_FLICKY_03_UP,
	S_FLICKY_03_FLAP1,
	S_FLICKY_03_FLAP2,
	S_FLICKY_03_STAND,
	S_FLICKY_03_CENTER,

	// Seal
	S_FLICKY_04_OUT,
	S_FLICKY_04_AIM,
	S_FLICKY_04_HOP,
	S_FLICKY_04_UP,
	S_FLICKY_04_DOWN,
	S_FLICKY_04_SWIM1,
	S_FLICKY_04_SWIM2,
	S_FLICKY_04_SWIM3,
	S_FLICKY_04_SWIM4,
	S_FLICKY_04_STAND,
	S_FLICKY_04_CENTER,

	// Pig
	S_FLICKY_05_OUT,
	S_FLICKY_05_AIM,
	S_FLICKY_05_HOP,
	S_FLICKY_05_UP,
	S_FLICKY_05_DOWN,
	S_FLICKY_05_STAND,
	S_FLICKY_05_CENTER,

	// Chipmunk
	S_FLICKY_06_OUT,
	S_FLICKY_06_AIM,
	S_FLICKY_06_HOP,
	S_FLICKY_06_UP,
	S_FLICKY_06_DOWN,
	S_FLICKY_06_STAND,
	S_FLICKY_06_CENTER,

	// Penguin
	S_FLICKY_07_OUT,
	S_FLICKY_07_AIML,
	S_FLICKY_07_HOPL,
	S_FLICKY_07_UPL,
	S_FLICKY_07_DOWNL,
	S_FLICKY_07_AIMR,
	S_FLICKY_07_HOPR,
	S_FLICKY_07_UPR,
	S_FLICKY_07_DOWNR,
	S_FLICKY_07_SWIM1,
	S_FLICKY_07_SWIM2,
	S_FLICKY_07_SWIM3,
	S_FLICKY_07_STAND,
	S_FLICKY_07_CENTER,

	// Fish
	S_FLICKY_08_OUT,
	S_FLICKY_08_AIM,
	S_FLICKY_08_HOP,
	S_FLICKY_08_FLAP1,
	S_FLICKY_08_FLAP2,
	S_FLICKY_08_FLAP3,
	S_FLICKY_08_FLAP4,
	S_FLICKY_08_SWIM1,
	S_FLICKY_08_SWIM2,
	S_FLICKY_08_SWIM3,
	S_FLICKY_08_SWIM4,
	S_FLICKY_08_STAND,
	S_FLICKY_08_CENTER,

	// Ram
	S_FLICKY_09_OUT,
	S_FLICKY_09_AIM,
	S_FLICKY_09_HOP,
	S_FLICKY_09_UP,
	S_FLICKY_09_DOWN,
	S_FLICKY_09_STAND,
	S_FLICKY_09_CENTER,

	// Puffin
	S_FLICKY_10_OUT,
	S_FLICKY_10_FLAP1,
	S_FLICKY_10_FLAP2,
	S_FLICKY_10_STAND,
	S_FLICKY_10_CENTER,

	// Cow
	S_FLICKY_11_OUT,
	S_FLICKY_11_AIM,
	S_FLICKY_11_RUN1,
	S_FLICKY_11_RUN2,
	S_FLICKY_11_RUN3,
	S_FLICKY_11_STAND,
	S_FLICKY_11_CENTER,

	// Rat
	S_FLICKY_12_OUT,
	S_FLICKY_12_AIM,
	S_FLICKY_12_RUN1,
	S_FLICKY_12_RUN2,
	S_FLICKY_12_RUN3,
	S_FLICKY_12_STAND,
	S_FLICKY_12_CENTER,

	// Bear
	S_FLICKY_13_OUT,
	S_FLICKY_13_AIM,
	S_FLICKY_13_HOP,
	S_FLICKY_13_UP,
	S_FLICKY_13_DOWN,
	S_FLICKY_13_STAND,
	S_FLICKY_13_CENTER,

	// Dove
	S_FLICKY_14_OUT,
	S_FLICKY_14_FLAP1,
	S_FLICKY_14_FLAP2,
	S_FLICKY_14_FLAP3,
	S_FLICKY_14_STAND,
	S_FLICKY_14_CENTER,

	// Cat
	S_FLICKY_15_OUT,
	S_FLICKY_15_AIM,
	S_FLICKY_15_HOP,
	S_FLICKY_15_UP,
	S_FLICKY_15_DOWN,
	S_FLICKY_15_STAND,
	S_FLICKY_15_CENTER,

	// Canary
	S_FLICKY_16_OUT,
	S_FLICKY_16_FLAP1,
	S_FLICKY_16_FLAP2,
	S_FLICKY_16_FLAP3,
	S_FLICKY_16_STAND,
	S_FLICKY_16_CENTER,

	// Spider
	S_SECRETFLICKY_01_OUT,
	S_SECRETFLICKY_01_AIM,
	S_SECRETFLICKY_01_HOP,
	S_SECRETFLICKY_01_UP,
	S_SECRETFLICKY_01_DOWN,
	S_SECRETFLICKY_01_STAND,
	S_SECRETFLICKY_01_CENTER,

	// Bat
	S_SECRETFLICKY_02_OUT,
	S_SECRETFLICKY_02_FLAP1,
	S_SECRETFLICKY_02_FLAP2,
	S_SECRETFLICKY_02_FLAP3,
	S_SECRETFLICKY_02_STAND,
	S_SECRETFLICKY_02_CENTER,

	// Fan
	S_FAN,
	S_FAN2,
	S_FAN3,
	S_FAN4,
	S_FAN5,

	// Steam Riser
	S_STEAM1,
	S_STEAM2,
	S_STEAM3,
	S_STEAM4,
	S_STEAM5,
	S_STEAM6,
	S_STEAM7,
	S_STEAM8,

	// Bumpers
	S_BUMPER,
	S_BUMPERHIT,

	// Balloons
	S_BALLOON,
	S_BALLOONPOP1,
	S_BALLOONPOP2,
	S_BALLOONPOP3,
	S_BALLOONPOP4,
	S_BALLOONPOP5,
	S_BALLOONPOP6,

	// Yellow Spring
	S_YELLOWSPRING,
	S_YELLOWSPRING2,
	S_YELLOWSPRING3,
	S_YELLOWSPRING4,
	S_YELLOWSPRING5,

	// Red Spring
	S_REDSPRING,
	S_REDSPRING2,
	S_REDSPRING3,
	S_REDSPRING4,
	S_REDSPRING5,

	// Blue Spring
	S_BLUESPRING,
	S_BLUESPRING2,
	S_BLUESPRING3,
	S_BLUESPRING4,
	S_BLUESPRING5,

	// Yellow Diagonal Spring
	S_YDIAG1,
	S_YDIAG2,
	S_YDIAG3,
	S_YDIAG4,
	S_YDIAG5,
	S_YDIAG6,
	S_YDIAG7,
	S_YDIAG8,

	// Red Diagonal Spring
	S_RDIAG1,
	S_RDIAG2,
	S_RDIAG3,
	S_RDIAG4,
	S_RDIAG5,
	S_RDIAG6,
	S_RDIAG7,
	S_RDIAG8,

	// Blue Diagonal Spring
	S_BDIAG1,
	S_BDIAG2,
	S_BDIAG3,
	S_BDIAG4,
	S_BDIAG5,
	S_BDIAG6,
	S_BDIAG7,
	S_BDIAG8,

	// Yellow Side Spring
	S_YHORIZ1,
	S_YHORIZ2,
	S_YHORIZ3,
	S_YHORIZ4,
	S_YHORIZ5,
	S_YHORIZ6,
	S_YHORIZ7,
	S_YHORIZ8,

	// Red Side Spring
	S_RHORIZ1,
	S_RHORIZ2,
	S_RHORIZ3,
	S_RHORIZ4,
	S_RHORIZ5,
	S_RHORIZ6,
	S_RHORIZ7,
	S_RHORIZ8,

	// Blue Side Spring
	S_BHORIZ1,
	S_BHORIZ2,
	S_BHORIZ3,
	S_BHORIZ4,
	S_BHORIZ5,
	S_BHORIZ6,
	S_BHORIZ7,
	S_BHORIZ8,

	// Booster
	S_BOOSTERSOUND,
	S_YELLOWBOOSTERROLLER,
	S_YELLOWBOOSTERSEG_LEFT,
	S_YELLOWBOOSTERSEG_RIGHT,
	S_YELLOWBOOSTERSEG_FACE,
	S_REDBOOSTERROLLER,
	S_REDBOOSTERSEG_LEFT,
	S_REDBOOSTERSEG_RIGHT,
	S_REDBOOSTERSEG_FACE,

	// Rain
	S_RAIN1,
	S_RAINRETURN,

	// Snowflake
	S_SNOW1,
	S_SNOW2,
	S_SNOW3,

	// Water Splish
	S_SPLISH1,
	S_SPLISH2,
	S_SPLISH3,
	S_SPLISH4,
	S_SPLISH5,
	S_SPLISH6,
	S_SPLISH7,
	S_SPLISH8,
	S_SPLISH9,

	// Lava Splish
	S_LAVASPLISH,

	// added water splash
	S_SPLASH1,
	S_SPLASH2,
	S_SPLASH3,

	// lava/slime damage burn smoke
	S_SMOKE1,
	S_SMOKE2,
	S_SMOKE3,
	S_SMOKE4,
	S_SMOKE5,

	// Bubbles
	S_SMALLBUBBLE,
	S_MEDIUMBUBBLE,
	S_LARGEBUBBLE1,
	S_LARGEBUBBLE2,
	S_EXTRALARGEBUBBLE, // breathable

	S_POP1, // Extra Large bubble goes POP!

	S_WATERZAP,

	// Spindash dust
	S_SPINDUST1,
	S_SPINDUST2,
	S_SPINDUST3,
	S_SPINDUST4,
	S_SPINDUST_BUBBLE1,
	S_SPINDUST_BUBBLE2,
	S_SPINDUST_BUBBLE3,
	S_SPINDUST_BUBBLE4,
	S_SPINDUST_FIRE1,
	S_SPINDUST_FIRE2,
	S_SPINDUST_FIRE3,
	S_SPINDUST_FIRE4,

	S_FOG1,
	S_FOG2,
	S_FOG3,
	S_FOG4,
	S_FOG5,
	S_FOG6,
	S_FOG7,
	S_FOG8,
	S_FOG9,
	S_FOG10,
	S_FOG11,
	S_FOG12,
	S_FOG13,
	S_FOG14,

	S_SEED,

	S_PARTICLE,

	// Score Logos
	S_SCRA, // 100
	S_SCRB, // 200
	S_SCRC, // 500
	S_SCRD, // 1000
	S_SCRE, // 10000
	S_SCRF, // 400 (mario)
	S_SCRG, // 800 (mario)
	S_SCRH, // 2000 (mario)
	S_SCRI, // 4000 (mario)
	S_SCRJ, // 8000 (mario)
	S_SCRK, // 1UP (mario)
	S_SCRL, // 10

	// Drowning Timer Numbers
	S_ZERO1,
	S_ONE1,
	S_TWO1,
	S_THREE1,
	S_FOUR1,
	S_FIVE1,

	S_ZERO2,
	S_ONE2,
	S_TWO2,
	S_THREE2,
	S_FOUR2,
	S_FIVE2,

	S_FLIGHTINDICATOR,

	S_LOCKON1,
	S_LOCKON2,
	S_LOCKON3,
	S_LOCKON4,
	S_LOCKONINF1,
	S_LOCKONINF2,
	S_LOCKONINF3,
	S_LOCKONINF4,

	// Tag Sign
	S_TTAG,

	// Got Flag Sign
	S_GOTFLAG,

	// Finish flag
	S_FINISHFLAG,

	S_CORK,
	S_LHRT,

	// Red Ring
	S_RRNG1,
	S_RRNG2,
	S_RRNG3,
	S_RRNG4,
	S_RRNG5,
	S_RRNG6,
	S_RRNG7,

	// Weapon Ring Ammo
	S_BOUNCERINGAMMO,
	S_RAILRINGAMMO,
	S_INFINITYRINGAMMO,
	S_AUTOMATICRINGAMMO,
	S_EXPLOSIONRINGAMMO,
	S_SCATTERRINGAMMO,
	S_GRENADERINGAMMO,

	// Weapon pickup
	S_BOUNCEPICKUP,
	S_BOUNCEPICKUPFADE1,
	S_BOUNCEPICKUPFADE2,
	S_BOUNCEPICKUPFADE3,
	S_BOUNCEPICKUPFADE4,
	S_BOUNCEPICKUPFADE5,
	S_BOUNCEPICKUPFADE6,
	S_BOUNCEPICKUPFADE7,
	S_BOUNCEPICKUPFADE8,

	S_RAILPICKUP,
	S_RAILPICKUPFADE1,
	S_RAILPICKUPFADE2,
	S_RAILPICKUPFADE3,
	S_RAILPICKUPFADE4,
	S_RAILPICKUPFADE5,
	S_RAILPICKUPFADE6,
	S_RAILPICKUPFADE7,
	S_RAILPICKUPFADE8,

	S_AUTOPICKUP,
	S_AUTOPICKUPFADE1,
	S_AUTOPICKUPFADE2,
	S_AUTOPICKUPFADE3,
	S_AUTOPICKUPFADE4,
	S_AUTOPICKUPFADE5,
	S_AUTOPICKUPFADE6,
	S_AUTOPICKUPFADE7,
	S_AUTOPICKUPFADE8,

	S_EXPLODEPICKUP,
	S_EXPLODEPICKUPFADE1,
	S_EXPLODEPICKUPFADE2,
	S_EXPLODEPICKUPFADE3,
	S_EXPLODEPICKUPFADE4,
	S_EXPLODEPICKUPFADE5,
	S_EXPLODEPICKUPFADE6,
	S_EXPLODEPICKUPFADE7,
	S_EXPLODEPICKUPFADE8,

	S_SCATTERPICKUP,
	S_SCATTERPICKUPFADE1,
	S_SCATTERPICKUPFADE2,
	S_SCATTERPICKUPFADE3,
	S_SCATTERPICKUPFADE4,
	S_SCATTERPICKUPFADE5,
	S_SCATTERPICKUPFADE6,
	S_SCATTERPICKUPFADE7,
	S_SCATTERPICKUPFADE8,

	S_GRENADEPICKUP,
	S_GRENADEPICKUPFADE1,
	S_GRENADEPICKUPFADE2,
	S_GRENADEPICKUPFADE3,
	S_GRENADEPICKUPFADE4,
	S_GRENADEPICKUPFADE5,
	S_GRENADEPICKUPFADE6,
	S_GRENADEPICKUPFADE7,
	S_GRENADEPICKUPFADE8,

	// Thrown Weapon Rings
	S_THROWNBOUNCE1,
	S_THROWNBOUNCE2,
	S_THROWNBOUNCE3,
	S_THROWNBOUNCE4,
	S_THROWNBOUNCE5,
	S_THROWNBOUNCE6,
	S_THROWNBOUNCE7,
	S_THROWNINFINITY1,
	S_THROWNINFINITY2,
	S_THROWNINFINITY3,
	S_THROWNINFINITY4,
	S_THROWNINFINITY5,
	S_THROWNINFINITY6,
	S_THROWNINFINITY7,
	S_THROWNAUTOMATIC1,
	S_THROWNAUTOMATIC2,
	S_THROWNAUTOMATIC3,
	S_THROWNAUTOMATIC4,
	S_THROWNAUTOMATIC5,
	S_THROWNAUTOMATIC6,
	S_THROWNAUTOMATIC7,
	S_THROWNEXPLOSION1,
	S_THROWNEXPLOSION2,
	S_THROWNEXPLOSION3,
	S_THROWNEXPLOSION4,
	S_THROWNEXPLOSION5,
	S_THROWNEXPLOSION6,
	S_THROWNEXPLOSION7,
	S_THROWNGRENADE1,
	S_THROWNGRENADE2,
	S_THROWNGRENADE3,
	S_THROWNGRENADE4,
	S_THROWNGRENADE5,
	S_THROWNGRENADE6,
	S_THROWNGRENADE7,
	S_THROWNGRENADE8,
	S_THROWNGRENADE9,
	S_THROWNGRENADE10,
	S_THROWNGRENADE11,
	S_THROWNGRENADE12,
	S_THROWNGRENADE13,
	S_THROWNGRENADE14,
	S_THROWNGRENADE15,
	S_THROWNGRENADE16,
	S_THROWNGRENADE17,
	S_THROWNGRENADE18,
	S_THROWNSCATTER,

	S_RINGEXPLODE,

	S_COIN1,
	S_COIN2,
	S_COIN3,
	S_COINSPARKLE1,
	S_COINSPARKLE2,
	S_COINSPARKLE3,
	S_COINSPARKLE4,
	S_GOOMBA1,
	S_GOOMBA1B,
	S_GOOMBA2,
	S_GOOMBA3,
	S_GOOMBA4,
	S_GOOMBA5,
	S_GOOMBA6,
	S_GOOMBA7,
	S_GOOMBA8,
	S_GOOMBA9,
	S_GOOMBA_DEAD,
	S_BLUEGOOMBA1,
	S_BLUEGOOMBA1B,
	S_BLUEGOOMBA2,
	S_BLUEGOOMBA3,
	S_BLUEGOOMBA4,
	S_BLUEGOOMBA5,
	S_BLUEGOOMBA6,
	S_BLUEGOOMBA7,
	S_BLUEGOOMBA8,
	S_BLUEGOOMBA9,
	S_BLUEGOOMBA_DEAD,

	// Mario-specific stuff
	S_FIREFLOWER1,
	S_FIREFLOWER2,
	S_FIREFLOWER3,
	S_FIREFLOWER4,
	S_FIREBALL,
	S_FIREBALLTRAIL1,
	S_FIREBALLTRAIL2,
	S_SHELL,
	S_PUMA_START1,
	S_PUMA_START2,
	S_PUMA_UP1,
	S_PUMA_UP2,
	S_PUMA_UP3,
	S_PUMA_DOWN1,
	S_PUMA_DOWN2,
	S_PUMA_DOWN3,
	S_PUMATRAIL1,
	S_PUMATRAIL2,
	S_PUMATRAIL3,
	S_PUMATRAIL4,
	S_HAMMER,
	S_KOOPA1,
	S_KOOPA2,
	S_KOOPAFLAME1,
	S_KOOPAFLAME2,
	S_KOOPAFLAME3,
	S_AXE1,
	S_AXE2,
	S_AXE3,
	S_MARIOBUSH1,
	S_MARIOBUSH2,
	S_TOAD,

	// Nights-specific stuff
	S_NIGHTSDRONE_MAN1,
	S_NIGHTSDRONE_MAN2,
	S_NIGHTSDRONE_SPARKLING1,
	S_NIGHTSDRONE_SPARKLING2,
	S_NIGHTSDRONE_SPARKLING3,
	S_NIGHTSDRONE_SPARKLING4,
	S_NIGHTSDRONE_SPARKLING5,
	S_NIGHTSDRONE_SPARKLING6,
	S_NIGHTSDRONE_SPARKLING7,
	S_NIGHTSDRONE_SPARKLING8,
	S_NIGHTSDRONE_SPARKLING9,
	S_NIGHTSDRONE_SPARKLING10,
	S_NIGHTSDRONE_SPARKLING11,
	S_NIGHTSDRONE_SPARKLING12,
	S_NIGHTSDRONE_SPARKLING13,
	S_NIGHTSDRONE_SPARKLING14,
	S_NIGHTSDRONE_SPARKLING15,
	S_NIGHTSDRONE_SPARKLING16,
	S_NIGHTSDRONE_GOAL1,
	S_NIGHTSDRONE_GOAL2,
	S_NIGHTSDRONE_GOAL3,
	S_NIGHTSDRONE_GOAL4,

	S_NIGHTSPARKLE1,
	S_NIGHTSPARKLE2,
	S_NIGHTSPARKLE3,
	S_NIGHTSPARKLE4,
	S_NIGHTSPARKLESUPER1,
	S_NIGHTSPARKLESUPER2,
	S_NIGHTSPARKLESUPER3,
	S_NIGHTSPARKLESUPER4,
	S_NIGHTSLOOPHELPER,

	// NiGHTS bumper
	S_NIGHTSBUMPER1,
	S_NIGHTSBUMPER2,
	S_NIGHTSBUMPER3,
	S_NIGHTSBUMPER4,
	S_NIGHTSBUMPER5,
	S_NIGHTSBUMPER6,
	S_NIGHTSBUMPER7,
	S_NIGHTSBUMPER8,
	S_NIGHTSBUMPER9,
	S_NIGHTSBUMPER10,
	S_NIGHTSBUMPER11,
	S_NIGHTSBUMPER12,

	S_HOOP,
	S_HOOP_XMASA,
	S_HOOP_XMASB,

	S_NIGHTSCORE10,
	S_NIGHTSCORE20,
	S_NIGHTSCORE30,
	S_NIGHTSCORE40,
	S_NIGHTSCORE50,
	S_NIGHTSCORE60,
	S_NIGHTSCORE70,
	S_NIGHTSCORE80,
	S_NIGHTSCORE90,
	S_NIGHTSCORE100,
	S_NIGHTSCORE10_2,
	S_NIGHTSCORE20_2,
	S_NIGHTSCORE30_2,
	S_NIGHTSCORE40_2,
	S_NIGHTSCORE50_2,
	S_NIGHTSCORE60_2,
	S_NIGHTSCORE70_2,
	S_NIGHTSCORE80_2,
	S_NIGHTSCORE90_2,
	S_NIGHTSCORE100_2,

	// NiGHTS Paraloop Powerups
	S_NIGHTSSUPERLOOP,
	S_NIGHTSDRILLREFILL,
	S_NIGHTSHELPER,
	S_NIGHTSEXTRATIME,
	S_NIGHTSLINKFREEZE,
	S_EGGCAPSULE,

	// Orbiting Chaos Emeralds
	S_ORBITEM1,
	S_ORBITEM2,
	S_ORBITEM3,
	S_ORBITEM4,
	S_ORBITEM5,
	S_ORBITEM6,
	S_ORBITEM7,
	S_ORBITEM8,
	S_ORBIDYA1,
	S_ORBIDYA2,
	S_ORBIDYA3,
	S_ORBIDYA4,
	S_ORBIDYA5,

	// "Flicky" helper
	S_NIGHTOPIANHELPER1,
	S_NIGHTOPIANHELPER2,
	S_NIGHTOPIANHELPER3,
	S_NIGHTOPIANHELPER4,
	S_NIGHTOPIANHELPER5,
	S_NIGHTOPIANHELPER6,
	S_NIGHTOPIANHELPER7,
	S_NIGHTOPIANHELPER8,
	S_NIGHTOPIANHELPER9,

	// Nightopian
	S_PIAN0,
	S_PIAN1,
	S_PIAN2,
	S_PIAN3,
	S_PIAN4,
	S_PIAN5,
	S_PIAN6,
	S_PIANSING,

	// Shleep
	S_SHLEEP1,
	S_SHLEEP2,
	S_SHLEEP3,
	S_SHLEEP4,
	S_SHLEEPBOUNCE1,
	S_SHLEEPBOUNCE2,
	S_SHLEEPBOUNCE3,

	// Secret badniks and hazards, shhhh
	S_PENGUINATOR_LOOK,
	S_PENGUINATOR_WADDLE1,
	S_PENGUINATOR_WADDLE2,
	S_PENGUINATOR_WADDLE3,
	S_PENGUINATOR_WADDLE4,
	S_PENGUINATOR_SLIDE1,
	S_PENGUINATOR_SLIDE2,
	S_PENGUINATOR_SLIDE3,
	S_PENGUINATOR_SLIDE4,
	S_PENGUINATOR_SLIDE5,

	S_POPHAT_LOOK,
	S_POPHAT_SHOOT1,
	S_POPHAT_SHOOT2,
	S_POPHAT_SHOOT3,
	S_POPHAT_SHOOT4,
	S_POPSHOT,
	S_POPSHOT_TRAIL,

	S_HIVEELEMENTAL_LOOK,
	S_HIVEELEMENTAL_PREPARE1,
	S_HIVEELEMENTAL_PREPARE2,
	S_HIVEELEMENTAL_SHOOT1,
	S_HIVEELEMENTAL_SHOOT2,
	S_HIVEELEMENTAL_DORMANT,
	S_HIVEELEMENTAL_PAIN,
	S_HIVEELEMENTAL_DIE1,
	S_HIVEELEMENTAL_DIE2,
	S_HIVEELEMENTAL_DIE3,

	S_BUMBLEBORE_SPAWN,
	S_BUMBLEBORE_LOOK1,
	S_BUMBLEBORE_LOOK2,
	S_BUMBLEBORE_FLY1,
	S_BUMBLEBORE_FLY2,
	S_BUMBLEBORE_RAISE,
	S_BUMBLEBORE_FALL1,
	S_BUMBLEBORE_FALL2,
	S_BUMBLEBORE_STUCK1,
	S_BUMBLEBORE_STUCK2,
	S_BUMBLEBORE_DIE,

	S_BUGGLEIDLE,
	S_BUGGLEFLY,

	S_SMASHSPIKE_FLOAT,
	S_SMASHSPIKE_EASE1,
	S_SMASHSPIKE_EASE2,
	S_SMASHSPIKE_FALL,
	S_SMASHSPIKE_STOMP1,
	S_SMASHSPIKE_STOMP2,
	S_SMASHSPIKE_RISE1,
	S_SMASHSPIKE_RISE2,

	S_CACO_LOOK,
	S_CACO_WAKE1,
	S_CACO_WAKE2,
	S_CACO_WAKE3,
	S_CACO_WAKE4,
	S_CACO_ROAR,
	S_CACO_CHASE,
	S_CACO_CHASE_REPEAT,
	S_CACO_RANDOM,
	S_CACO_PREPARE_SOUND,
	S_CACO_PREPARE1,
	S_CACO_PREPARE2,
	S_CACO_PREPARE3,
	S_CACO_SHOOT_SOUND,
	S_CACO_SHOOT1,
	S_CACO_SHOOT2,
	S_CACO_CLOSE,
	S_CACO_DIE_FLAGS,
	S_CACO_DIE_GIB1,
	S_CACO_DIE_GIB2,
	S_CACO_DIE_SCREAM,
	S_CACO_DIE_SHATTER,
	S_CACO_DIE_FALL,
	S_CACOSHARD_RANDOMIZE,
	S_CACOSHARD1_1,
	S_CACOSHARD1_2,
	S_CACOSHARD2_1,
	S_CACOSHARD2_2,
	S_CACOFIRE1,
	S_CACOFIRE2,
	S_CACOFIRE3,
	S_CACOFIRE_EXPLODE1,
	S_CACOFIRE_EXPLODE2,
	S_CACOFIRE_EXPLODE3,
	S_CACOFIRE_EXPLODE4,

	S_SPINBOBERT_MOVE_FLIPUP,
	S_SPINBOBERT_MOVE_UP,
	S_SPINBOBERT_MOVE_FLIPDOWN,
	S_SPINBOBERT_MOVE_DOWN,
	S_SPINBOBERT_FIRE_MOVE,
	S_SPINBOBERT_FIRE_GHOST,
	S_SPINBOBERT_FIRE_TRAIL1,
	S_SPINBOBERT_FIRE_TRAIL2,
	S_SPINBOBERT_FIRE_TRAIL3,

	S_HANGSTER_LOOK,
	S_HANGSTER_SWOOP1,
	S_HANGSTER_SWOOP2,
	S_HANGSTER_ARC1,
	S_HANGSTER_ARC2,
	S_HANGSTER_ARC3,
	S_HANGSTER_FLY1,
	S_HANGSTER_FLY2,
	S_HANGSTER_FLY3,
	S_HANGSTER_FLY4,
	S_HANGSTER_FLYREPEAT,
	S_HANGSTER_ARCUP1,
	S_HANGSTER_ARCUP2,
	S_HANGSTER_ARCUP3,
	S_HANGSTER_RETURN1,
	S_HANGSTER_RETURN2,
	S_HANGSTER_RETURN3,

	S_CRUMBLE1,
	S_CRUMBLE2,

	// Spark
	S_SPRK1,
	S_SPRK2,
	S_SPRK3,

	// Robot Explosion
	S_XPLD_FLICKY,
	S_XPLD1,
	S_XPLD2,
	S_XPLD3,
	S_XPLD4,
	S_XPLD5,
	S_XPLD6,
	S_XPLD_EGGTRAP,

	// Underwater Explosion
	S_WPLD1,
	S_WPLD2,
	S_WPLD3,
	S_WPLD4,
	S_WPLD5,
	S_WPLD6,

	S_DUST1,
	S_DUST2,
	S_DUST3,
	S_DUST4,

	S_ROCKSPAWN,

	S_ROCKCRUMBLEA,
	S_ROCKCRUMBLEB,
	S_ROCKCRUMBLEC,
	S_ROCKCRUMBLED,
	S_ROCKCRUMBLEE,
	S_ROCKCRUMBLEF,
	S_ROCKCRUMBLEG,
	S_ROCKCRUMBLEH,
	S_ROCKCRUMBLEI,
	S_ROCKCRUMBLEJ,
	S_ROCKCRUMBLEK,
	S_ROCKCRUMBLEL,
	S_ROCKCRUMBLEM,
	S_ROCKCRUMBLEN,
	S_ROCKCRUMBLEO,
	S_ROCKCRUMBLEP,

	// Level debris
	S_GFZDEBRIS,
	S_BRICKDEBRIS,
	S_WOODDEBRIS,
	S_REDBRICKDEBRIS, // for CEZ3
	S_BLUEBRICKDEBRIS, // for CEZ3
	S_YELLOWBRICKDEBRIS, // for CEZ3

#ifdef SEENAMES
	S_NAMECHECK,
#endif

	S_FIRSTFREESLOT,
	S_LASTFREESLOT = S_FIRSTFREESLOT + NUMSTATEFREESLOTS - 1,
	NUMSTATES
} statenum_t;

typedef struct
{
	spritenum_t sprite;
	UINT32 frame; // we use the upper 16 bits for translucency and other shade effects
	INT32 tics;
	actionf_t action;
	INT32 var1;
	INT32 var2;
	statenum_t nextstate;
} state_t;

extern state_t states[NUMSTATES];
extern char sprnames[NUMSPRITES + 1][5];
extern char spr2names[NUMPLAYERSPRITES][5];
extern playersprite_t spr2defaults[NUMPLAYERSPRITES];
extern state_t *astate;
extern playersprite_t free_spr2;

typedef enum mobj_type
{
	MT_NULL,
	MT_UNKNOWN,

	MT_THOK, // Thok! mobj
	MT_PLAYER,
	MT_TAILSOVERLAY, // c:
	MT_METALJETFUME,

	// Enemies
	MT_BLUECRAWLA, // Crawla (Blue)
	MT_REDCRAWLA, // Crawla (Red)
	MT_GFZFISH, // SDURF
	MT_GOLDBUZZ, // Buzz (Gold)
	MT_REDBUZZ, // Buzz (Red)
	MT_JETTBOMBER, // Jetty-Syn Bomber
	MT_JETTGUNNER, // Jetty-Syn Gunner
	MT_CRAWLACOMMANDER, // Crawla Commander
	MT_DETON, // Deton
	MT_SKIM, // Skim mine dropper
	MT_TURRET, // Industrial Turret
	MT_POPUPTURRET, // Pop-Up Turret
	MT_SPINCUSHION, // Spincushion
	MT_CRUSHSTACEAN, // Crushstacean
	MT_CRUSHCLAW, // Big meaty claw
	MT_CRUSHCHAIN, // Chain
	MT_BANPYURA, // Banpyura
	MT_BANPSPRING, // Banpyura spring
	MT_JETJAW, // Jet Jaw
	MT_SNAILER, // Snailer
	MT_VULTURE, // BASH
	MT_POINTY, // Pointy
	MT_POINTYBALL, // Pointy Ball
	MT_ROBOHOOD, // Robo-Hood
	MT_FACESTABBER, // Castlebot Facestabber
	MT_FACESTABBERSPEAR, // Castlebot Facestabber spear aura
	MT_EGGGUARD, // Egg Guard
	MT_EGGSHIELD, // Egg Guard's shield
	MT_GSNAPPER, // Green Snapper
	MT_SNAPPER_LEG, // Green Snapper leg
	MT_SNAPPER_HEAD, // Green Snapper head
	MT_MINUS, // Minus
	MT_MINUSDIRT, // Minus dirt
	MT_SPRINGSHELL, // Spring Shell
	MT_YELLOWSHELL, // Spring Shell (yellow)
	MT_UNIDUS, // Unidus
	MT_UNIBALL, // Unidus Ball
	MT_CANARIVORE, // Canarivore
	MT_CANARIVORE_GAS, // Canarivore gas
	MT_PYREFLY, // Pyre Fly
	MT_PYREFLY_FIRE, // Pyre Fly fire
	MT_PTERABYTESPAWNER, // Pterabyte spawner
	MT_PTERABYTEWAYPOINT, // Pterabyte waypoint
	MT_PTERABYTE, // Pterabyte
	MT_DRAGONBOMBER, // Dragonbomber
	MT_DRAGONWING, // Dragonbomber wing
	MT_DRAGONTAIL, // Dragonbomber tail segment
	MT_DRAGONMINE, // Dragonbomber mine

	// Generic Boss Items
	MT_BOSSEXPLODE,
	MT_SONIC3KBOSSEXPLODE,
	MT_BOSSFLYPOINT,
	MT_EGGTRAP,
	MT_BOSS3WAYPOINT,
	MT_BOSS9GATHERPOINT,
	MT_BOSSJUNK,

	// Boss 1
	MT_EGGMOBILE,
	MT_JETFUME1,
	MT_EGGMOBILE_BALL,
	MT_EGGMOBILE_TARGET,
	MT_EGGMOBILE_FIRE,

	// Boss 2
	MT_EGGMOBILE2,
	MT_EGGMOBILE2_POGO,
	MT_GOOP,
	MT_GOOPTRAIL,

	// Boss 3
	MT_EGGMOBILE3,
	MT_FAKEMOBILE,
	MT_SHOCKWAVE,

	// Boss 4
	MT_EGGMOBILE4,
	MT_EGGMOBILE4_MACE,
	MT_JETFLAME,
	MT_EGGROBO1,
	MT_EGGROBO1JET,

	// Boss 5
	MT_FANG,
	MT_BROKENROBOT,
	MT_VWREF,
	MT_VWREB,
	MT_PROJECTORLIGHT,
	MT_FBOMB,
	MT_TNTDUST, // also used by barrel
	MT_FSGNA,
	MT_FSGNB,
	MT_FANGWAYPOINT,

	// Black Eggman (Boss 7)
	MT_BLACKEGGMAN,
	MT_BLACKEGGMAN_HELPER,
	MT_BLACKEGGMAN_GOOPFIRE,
	MT_BLACKEGGMAN_MISSILE,

	// New Very-Last-Minute 2.1 Brak Eggman (Cy-Brak-demon)
	MT_CYBRAKDEMON,
	MT_CYBRAKDEMON_ELECTRIC_BARRIER,
	MT_CYBRAKDEMON_MISSILE,
	MT_CYBRAKDEMON_FLAMESHOT,
	MT_CYBRAKDEMON_FLAMEREST,
	MT_CYBRAKDEMON_TARGET_RETICULE,
	MT_CYBRAKDEMON_TARGET_DOT,
	MT_CYBRAKDEMON_NAPALM_BOMB_LARGE,
	MT_CYBRAKDEMON_NAPALM_BOMB_SMALL,
	MT_CYBRAKDEMON_NAPALM_FLAMES,
	MT_CYBRAKDEMON_VILE_EXPLOSION,

	// Metal Sonic (Boss 9)
	MT_METALSONIC_RACE,
	MT_METALSONIC_BATTLE,
	MT_MSSHIELD_FRONT,
	MT_MSGATHER,

	// Collectible Items
	MT_RING,
	MT_FLINGRING, // Lost ring
	MT_BLUESPHERE,  // Blue sphere for special stages
	MT_FLINGBLUESPHERE, // Lost blue sphere
	MT_BOMBSPHERE,
	MT_REDTEAMRING,  //Rings collectable by red team.
	MT_BLUETEAMRING, //Rings collectable by blue team.
	MT_TOKEN, // Special Stage token for special stage
	MT_REDFLAG, // Red CTF Flag
	MT_BLUEFLAG, // Blue CTF Flag
	MT_EMBLEM,
	MT_EMERALD1,
	MT_EMERALD2,
	MT_EMERALD3,
	MT_EMERALD4,
	MT_EMERALD5,
	MT_EMERALD6,
	MT_EMERALD7,
	MT_EMERHUNT, // Emerald Hunt
	MT_EMERALDSPAWN, // Emerald spawner w/ delay
	MT_FLINGEMERALD, // Lost emerald

	// Springs and others
	MT_FAN,
	MT_STEAM,
	MT_BUMPER,
	MT_BALLOON,

	MT_YELLOWSPRING,
	MT_REDSPRING,
	MT_BLUESPRING,
	MT_YELLOWDIAG,
	MT_REDDIAG,
	MT_BLUEDIAG,
	MT_YELLOWHORIZ,
	MT_REDHORIZ,
	MT_BLUEHORIZ,

	MT_BOOSTERSEG,
	MT_BOOSTERROLLER,
	MT_YELLOWBOOSTER,
	MT_REDBOOSTER,

	// Interactive Objects
	MT_BUBBLES, // Bubble source
	MT_SIGN, // Level end sign
	MT_SPIKEBALL, // Spike Ball
	MT_SPINFIRE,
	MT_SPIKE,
	MT_WALLSPIKE,
	MT_WALLSPIKEBASE,
	MT_STARPOST,
	MT_BIGMINE,
	MT_BLASTEXECUTOR,
	MT_CANNONLAUNCHER,

	// Monitor miscellany
	MT_BOXSPARKLE,

	// Monitor boxes -- regular
	MT_RING_BOX,
	MT_PITY_BOX,
	MT_ATTRACT_BOX,
	MT_FORCE_BOX,
	MT_ARMAGEDDON_BOX,
	MT_WHIRLWIND_BOX,
	MT_ELEMENTAL_BOX,
	MT_SNEAKERS_BOX,
	MT_INVULN_BOX,
	MT_1UP_BOX,
	MT_EGGMAN_BOX,
	MT_MIXUP_BOX,
	MT_MYSTERY_BOX,
	MT_GRAVITY_BOX,
	MT_RECYCLER_BOX,
	MT_SCORE1K_BOX,
	MT_SCORE10K_BOX,
	MT_FLAMEAURA_BOX,
	MT_BUBBLEWRAP_BOX,
	MT_THUNDERCOIN_BOX,

	// Monitor boxes -- repeating (big) boxes
	MT_PITY_GOLDBOX,
	MT_ATTRACT_GOLDBOX,
	MT_FORCE_GOLDBOX,
	MT_ARMAGEDDON_GOLDBOX,
	MT_WHIRLWIND_GOLDBOX,
	MT_ELEMENTAL_GOLDBOX,
	MT_SNEAKERS_GOLDBOX,
	MT_INVULN_GOLDBOX,
	MT_EGGMAN_GOLDBOX,
	MT_GRAVITY_GOLDBOX,
	MT_FLAMEAURA_GOLDBOX,
	MT_BUBBLEWRAP_GOLDBOX,
	MT_THUNDERCOIN_GOLDBOX,

	// Monitor boxes -- special
	MT_RING_REDBOX,
	MT_RING_BLUEBOX,

	// Monitor icons
	MT_RING_ICON,
	MT_PITY_ICON,
	MT_ATTRACT_ICON,
	MT_FORCE_ICON,
	MT_ARMAGEDDON_ICON,
	MT_WHIRLWIND_ICON,
	MT_ELEMENTAL_ICON,
	MT_SNEAKERS_ICON,
	MT_INVULN_ICON,
	MT_1UP_ICON,
	MT_EGGMAN_ICON,
	MT_MIXUP_ICON,
	MT_GRAVITY_ICON,
	MT_RECYCLER_ICON,
	MT_SCORE1K_ICON,
	MT_SCORE10K_ICON,
	MT_FLAMEAURA_ICON,
	MT_BUBBLEWRAP_ICON,
	MT_THUNDERCOIN_ICON,

	// Projectiles
	MT_ROCKET,
	MT_LASER,
	MT_TORPEDO,
	MT_TORPEDO2, // silent
	MT_ENERGYBALL,
	MT_MINE, // Skim/Jetty-Syn mine
	MT_JETTBULLET, // Jetty-Syn Bullet
	MT_TURRETLASER,
	MT_CANNONBALL, // Cannonball
	MT_CANNONBALLDECOR, // Decorative/still cannonball
	MT_ARROW, // Arrow
	MT_DEMONFIRE, // Glaregoyle fire

	// The letter
	MT_LETTER,

	// Greenflower Scenery
	MT_GFZFLOWER1,
	MT_GFZFLOWER2,
	MT_GFZFLOWER3,

	MT_BLUEBERRYBUSH,
	MT_BERRYBUSH,
	MT_BUSH,

	// Trees (both GFZ and misc)
	MT_GFZTREE,
	MT_GFZBERRYTREE,
	MT_GFZCHERRYTREE,
	MT_CHECKERTREE,
	MT_CHECKERSUNSETTREE,
	MT_FHZTREE, // Frozen Hillside
	MT_FHZPINKTREE,
	MT_POLYGONTREE,
	MT_BUSHTREE,
	MT_BUSHREDTREE,
	MT_SPRINGTREE,

	// Techno Hill Scenery
	MT_THZFLOWER1,
	MT_THZFLOWER2,
	MT_THZFLOWER3,
	MT_THZTREE, // Steam whistle tree/bush
	MT_THZTREEBRANCH, // branch of said tree
	MT_ALARM,

	// Deep Sea Scenery
	MT_GARGOYLE, // Deep Sea Gargoyle
	MT_BIGGARGOYLE, // Deep Sea Gargoyle (Big)
	MT_SEAWEED, // DSZ Seaweed
	MT_WATERDRIP, // Dripping Water source
	MT_WATERDROP, // Water drop from dripping water
	MT_CORAL1, // Coral
	MT_CORAL2,
	MT_CORAL3,
	MT_CORAL4,
	MT_CORAL5,
	MT_BLUECRYSTAL, // Blue Crystal
	MT_KELP, // Kelp
	MT_ANIMALGAETOP, // Animated algae top
	MT_ANIMALGAESEG, // Animated algae segment
	MT_DSZSTALAGMITE, // Deep Sea 1 Stalagmite
	MT_DSZ2STALAGMITE, // Deep Sea 2 Stalagmite
	MT_LIGHTBEAM, // DSZ Light beam

	// Castle Eggman Scenery
	MT_CHAIN, // CEZ Chain
	MT_FLAME, // Flame (has corona)
	MT_FLAMEPARTICLE,
	MT_EGGSTATUE, // Eggman Statue
	MT_MACEPOINT, // Mace rotation point
	MT_CHAINMACEPOINT, // Combination of chains and maces point
	MT_SPRINGBALLPOINT, // Spring ball point
	MT_CHAINPOINT, // Mace chain
	MT_HIDDEN_SLING, // Spin mace chain (activatable)
	MT_FIREBARPOINT, // Firebar
	MT_CUSTOMMACEPOINT, // Custom mace
	MT_SMALLMACECHAIN, // Small Mace Chain
	MT_BIGMACECHAIN, // Big Mace Chain
	MT_SMALLMACE, // Small Mace
	MT_BIGMACE, // Big Mace
	MT_SMALLGRABCHAIN, // Small Grab Chain
	MT_BIGGRABCHAIN, // Big Grab Chain
	MT_YELLOWSPRINGBALL, // Yellow spring on a ball
	MT_REDSPRINGBALL, // Red spring on a ball
	MT_SMALLFIREBAR, // Small Firebar
	MT_BIGFIREBAR, // Big Firebar
	MT_CEZFLOWER, // Flower
	MT_CEZPOLE1, // Pole (with red banner)
	MT_CEZPOLE2, // Pole (with blue banner)
	MT_CEZBANNER1, // Banner (red)
	MT_CEZBANNER2, // Banner (blue)
	MT_PINETREE, // Pine Tree
	MT_CEZBUSH1, // Bush 1
	MT_CEZBUSH2, // Bush 2
	MT_CANDLE, // Candle
	MT_CANDLEPRICKET, // Candle pricket
	MT_FLAMEHOLDER, // Flame holder
	MT_FIRETORCH, // Fire torch
	MT_WAVINGFLAG1, // Waving flag (red)
	MT_WAVINGFLAG2, // Waving flag (blue)
	MT_WAVINGFLAGSEG1, // Waving flag segment (red)
	MT_WAVINGFLAGSEG2, // Waving flag segment (blue)
	MT_CRAWLASTATUE, // Crawla statue
	MT_FACESTABBERSTATUE, // Facestabber statue
	MT_SUSPICIOUSFACESTABBERSTATUE, // :eggthinking:
	MT_BRAMBLES, // Brambles

	// Arid Canyon Scenery
	MT_BIGTUMBLEWEED,
	MT_LITTLETUMBLEWEED,
	MT_CACTI1, // Tiny Red Flower Cactus
	MT_CACTI2, // Small Red Flower Cactus
	MT_CACTI3, // Tiny Blue Flower Cactus
	MT_CACTI4, // Small Blue Flower Cactus
	MT_CACTI5, // Prickly Pear
	MT_CACTI6, // Barrel Cactus
	MT_CACTI7, // Tall Barrel Cactus
	MT_CACTI8, // Armed Cactus
	MT_CACTI9, // Ball Cactus
	MT_CACTI10, // Tiny Cactus
	MT_CACTI11, // Small Cactus
	MT_CACTITINYSEG, // Tiny Cactus Segment
	MT_CACTISMALLSEG, // Small Cactus Segment
	MT_ARIDSIGN_CAUTION, // Caution Sign
	MT_ARIDSIGN_CACTI, // Cacti Sign
	MT_ARIDSIGN_SHARPTURN, // Sharp Turn Sign
	MT_OILLAMP,
	MT_TNTBARREL,
	MT_PROXIMITYTNT,
	MT_DUSTDEVIL,
	MT_DUSTLAYER,
	MT_ARIDDUST,
	MT_MINECART,
	MT_MINECARTSEG,
	MT_MINECARTSPAWNER,
	MT_MINECARTEND,
	MT_MINECARTENDSOLID,
	MT_MINECARTSIDEMARK,
	MT_MINECARTSPARK,
	MT_SALOONDOOR,
	MT_SALOONDOORCENTER,
	MT_TRAINCAMEOSPAWNER,
	MT_TRAINSEG,
	MT_TRAINDUSTSPAWNER,
	MT_TRAINSTEAMSPAWNER,
	MT_MINECARTSWITCHPOINT,

	// Red Volcano Scenery
	MT_FLAMEJET,
	MT_VERTICALFLAMEJET,
	MT_FLAMEJETFLAME,

	MT_FJSPINAXISA, // Counter-clockwise
	MT_FJSPINAXISB, // Clockwise

	MT_FLAMEJETFLAMEB, // Blade's flame

	MT_LAVAFALL,
	MT_LAVAFALL_LAVA,
	MT_LAVAFALLROCK,

	MT_ROLLOUTSPAWN,
	MT_ROLLOUTROCK,

	MT_BIGFERNLEAF,
	MT_BIGFERN,
	MT_JUNGLEPALM,
	MT_TORCHFLOWER,
	MT_WALLVINE_LONG,
	MT_WALLVINE_SHORT,

	// Dark City Scenery

	// Egg Rock Scenery

	// Azure Temple Scenery
	MT_GLAREGOYLE,
	MT_GLAREGOYLEUP,
	MT_GLAREGOYLEDOWN,
	MT_GLAREGOYLELONG,
	MT_TARGET, // AKA Red Crystal
	MT_GREENFLAME,
	MT_BLUEGARGOYLE,

	// Stalagmites
	MT_STALAGMITE0,
	MT_STALAGMITE1,
	MT_STALAGMITE2,
	MT_STALAGMITE3,
	MT_STALAGMITE4,
	MT_STALAGMITE5,
	MT_STALAGMITE6,
	MT_STALAGMITE7,
	MT_STALAGMITE8,
	MT_STALAGMITE9,

	// Christmas Scenery
	MT_XMASPOLE,
	MT_CANDYCANE,
	MT_SNOWMAN,    // normal
	MT_SNOWMANHAT, // with hat + scarf
	MT_LAMPPOST1,  // normal
	MT_LAMPPOST2,  // with snow
	MT_HANGSTAR,
	MT_MISTLETOE,
	// Xmas GFZ bushes
	MT_XMASBLUEBERRYBUSH,
	MT_XMASBERRYBUSH,
	MT_XMASBUSH,
	// FHZ
	MT_FHZICE1,
	MT_FHZICE2,
	MT_ROSY,
	MT_CDLHRT,

	// Halloween Scenery
	// Pumpkins
	MT_JACKO1,
	MT_JACKO2,
	MT_JACKO3,
	// Dr Seuss Trees
	MT_HHZTREE_TOP,
	MT_HHZTREE_PART,
	// Misc
	MT_HHZSHROOM,
	MT_HHZGRASS,
	MT_HHZTENTACLE1,
	MT_HHZTENTACLE2,
	MT_HHZSTALAGMITE_TALL,
	MT_HHZSTALAGMITE_SHORT,

	// Botanic Serenity scenery
	MT_BSZTALLFLOWER_RED,
	MT_BSZTALLFLOWER_PURPLE,
	MT_BSZTALLFLOWER_BLUE,
	MT_BSZTALLFLOWER_CYAN,
	MT_BSZTALLFLOWER_YELLOW,
	MT_BSZTALLFLOWER_ORANGE,
	MT_BSZFLOWER_RED,
	MT_BSZFLOWER_PURPLE,
	MT_BSZFLOWER_BLUE,
	MT_BSZFLOWER_CYAN,
	MT_BSZFLOWER_YELLOW,
	MT_BSZFLOWER_ORANGE,
	MT_BSZSHORTFLOWER_RED,
	MT_BSZSHORTFLOWER_PURPLE,
	MT_BSZSHORTFLOWER_BLUE,
	MT_BSZSHORTFLOWER_CYAN,
	MT_BSZSHORTFLOWER_YELLOW,
	MT_BSZSHORTFLOWER_ORANGE,
	MT_BSZTULIP_RED,
	MT_BSZTULIP_PURPLE,
	MT_BSZTULIP_BLUE,
	MT_BSZTULIP_CYAN,
	MT_BSZTULIP_YELLOW,
	MT_BSZTULIP_ORANGE,
	MT_BSZCLUSTER_RED,
	MT_BSZCLUSTER_PURPLE,
	MT_BSZCLUSTER_BLUE,
	MT_BSZCLUSTER_CYAN,
	MT_BSZCLUSTER_YELLOW,
	MT_BSZCLUSTER_ORANGE,
	MT_BSZBUSH_RED,
	MT_BSZBUSH_PURPLE,
	MT_BSZBUSH_BLUE,
	MT_BSZBUSH_CYAN,
	MT_BSZBUSH_YELLOW,
	MT_BSZBUSH_ORANGE,
	MT_BSZVINE_RED,
	MT_BSZVINE_PURPLE,
	MT_BSZVINE_BLUE,
	MT_BSZVINE_CYAN,
	MT_BSZVINE_YELLOW,
	MT_BSZVINE_ORANGE,
	MT_BSZSHRUB,
	MT_BSZCLOVER,
	MT_BIG_PALMTREE_TRUNK,
	MT_BIG_PALMTREE_TOP,
	MT_PALMTREE_TRUNK,
	MT_PALMTREE_TOP,

	// Misc scenery
	MT_DBALL,
	MT_EGGSTATUE2,

	// Powerup Indicators
	MT_ELEMENTAL_ORB, // Elemental shield mobj
	MT_ATTRACT_ORB, // Attract shield mobj
	MT_FORCE_ORB, // Force shield mobj
	MT_ARMAGEDDON_ORB, // Armageddon shield mobj
	MT_WHIRLWIND_ORB, // Whirlwind shield mobj
	MT_PITY_ORB, // Pity shield mobj
	MT_FLAMEAURA_ORB, // Flame shield mobj
	MT_BUBBLEWRAP_ORB, // Bubble shield mobj
	MT_THUNDERCOIN_ORB, // Thunder shield mobj
	MT_THUNDERCOIN_SPARK, // Thunder spark
	MT_IVSP, // Invincibility sparkles
	MT_SUPERSPARK, // Super Sonic Spark

	// Flickies
	MT_FLICKY_01, // Bluebird
	MT_FLICKY_01_CENTER,
	MT_FLICKY_02, // Rabbit
	MT_FLICKY_02_CENTER,
	MT_FLICKY_03, // Chicken
	MT_FLICKY_03_CENTER,
	MT_FLICKY_04, // Seal
	MT_FLICKY_04_CENTER,
	MT_FLICKY_05, // Pig
	MT_FLICKY_05_CENTER,
	MT_FLICKY_06, // Chipmunk
	MT_FLICKY_06_CENTER,
	MT_FLICKY_07, // Penguin
	MT_FLICKY_07_CENTER,
	MT_FLICKY_08, // Fish
	MT_FLICKY_08_CENTER,
	MT_FLICKY_09, // Ram
	MT_FLICKY_09_CENTER,
	MT_FLICKY_10, // Puffin
	MT_FLICKY_10_CENTER,
	MT_FLICKY_11, // Cow
	MT_FLICKY_11_CENTER,
	MT_FLICKY_12, // Rat
	MT_FLICKY_12_CENTER,
	MT_FLICKY_13, // Bear
	MT_FLICKY_13_CENTER,
	MT_FLICKY_14, // Dove
	MT_FLICKY_14_CENTER,
	MT_FLICKY_15, // Cat
	MT_FLICKY_15_CENTER,
	MT_FLICKY_16, // Canary
	MT_FLICKY_16_CENTER,
	MT_SECRETFLICKY_01, // Spider
	MT_SECRETFLICKY_01_CENTER,
	MT_SECRETFLICKY_02, // Bat
	MT_SECRETFLICKY_02_CENTER,
	MT_SEED,

	// Environmental Effects
	MT_RAIN, // Rain
	MT_SNOWFLAKE, // Snowflake
	MT_SPLISH, // Water splish!
	MT_LAVASPLISH, // Lava splish!
	MT_SMOKE,
	MT_SMALLBUBBLE, // small bubble
	MT_MEDIUMBUBBLE, // medium bubble
	MT_EXTRALARGEBUBBLE, // extra large bubble
	MT_WATERZAP,
	MT_SPINDUST, // Spindash dust
	MT_TFOG,
	MT_PARTICLE,
	MT_PARTICLEGEN, // For fans, etc.

	// Game Indicators
	MT_SCORE, // score logo
	MT_DROWNNUMBERS, // Drowning Timer
	MT_GOTEMERALD, // Chaos Emerald (intangible)
	MT_LOCKON, // Target
	MT_LOCKONINF, // In-level Target
	MT_TAG, // Tag Sign
	MT_GOTFLAG, // Got Flag sign
	MT_FINISHFLAG, // Finish flag

	// Ambient Sounds
	MT_AWATERA, // Ambient Water Sound 1
	MT_AWATERB, // Ambient Water Sound 2
	MT_AWATERC, // Ambient Water Sound 3
	MT_AWATERD, // Ambient Water Sound 4
	MT_AWATERE, // Ambient Water Sound 5
	MT_AWATERF, // Ambient Water Sound 6
	MT_AWATERG, // Ambient Water Sound 7
	MT_AWATERH, // Ambient Water Sound 8
	MT_RANDOMAMBIENT,
	MT_RANDOMAMBIENT2,
	MT_MACHINEAMBIENCE,

	MT_CORK,
	MT_LHRT,

	// Ring Weapons
	MT_REDRING,
	MT_BOUNCERING,
	MT_RAILRING,
	MT_INFINITYRING,
	MT_AUTOMATICRING,
	MT_EXPLOSIONRING,
	MT_SCATTERRING,
	MT_GRENADERING,

	MT_BOUNCEPICKUP,
	MT_RAILPICKUP,
	MT_AUTOPICKUP,
	MT_EXPLODEPICKUP,
	MT_SCATTERPICKUP,
	MT_GRENADEPICKUP,

	MT_THROWNBOUNCE,
	MT_THROWNINFINITY,
	MT_THROWNAUTOMATIC,
	MT_THROWNSCATTER,
	MT_THROWNEXPLOSION,
	MT_THROWNGRENADE,

	// Mario-specific stuff
	MT_COIN,
	MT_FLINGCOIN,
	MT_GOOMBA,
	MT_BLUEGOOMBA,
	MT_FIREFLOWER,
	MT_FIREBALL,
	MT_FIREBALLTRAIL,
	MT_SHELL,
	MT_PUMA,
	MT_PUMATRAIL,
	MT_HAMMER,
	MT_KOOPA,
	MT_KOOPAFLAME,
	MT_AXE,
	MT_MARIOBUSH1,
	MT_MARIOBUSH2,
	MT_TOAD,

	// NiGHTS Stuff
	MT_AXIS,
	MT_AXISTRANSFER,
	MT_AXISTRANSFERLINE,
	MT_NIGHTSDRONE,
	MT_NIGHTSDRONE_MAN,
	MT_NIGHTSDRONE_SPARKLING,
	MT_NIGHTSDRONE_GOAL,
	MT_NIGHTSPARKLE,
	MT_NIGHTSLOOPHELPER,
	MT_NIGHTSBUMPER, // NiGHTS Bumper
	MT_HOOP,
	MT_HOOPCOLLIDE, // Collision detection for NiGHTS hoops
	MT_HOOPCENTER, // Center of a hoop
	MT_NIGHTSCORE,
	MT_NIGHTSCHIP, // NiGHTS Chip
	MT_FLINGNIGHTSCHIP, // Lost NiGHTS Chip
	MT_NIGHTSSTAR, // NiGHTS Star
	MT_FLINGNIGHTSSTAR, // Lost NiGHTS Star
	MT_NIGHTSSUPERLOOP,
	MT_NIGHTSDRILLREFILL,
	MT_NIGHTSHELPER,
	MT_NIGHTSEXTRATIME,
	MT_NIGHTSLINKFREEZE,
	MT_EGGCAPSULE,
	MT_IDEYAANCHOR,
	MT_NIGHTOPIANHELPER, // the actual helper object that orbits you
	MT_PIAN, // decorative singing friend
	MT_SHLEEP, // almost-decorative sleeping enemy

	// Secret badniks and hazards, shhhh
	MT_PENGUINATOR,
	MT_POPHAT,
	MT_POPSHOT,
	MT_POPSHOT_TRAIL,

	MT_HIVEELEMENTAL,
	MT_BUMBLEBORE,

	MT_BUGGLE,

	MT_SMASHINGSPIKEBALL,
	MT_CACOLANTERN,
	MT_CACOSHARD,
	MT_CACOFIRE,
	MT_SPINBOBERT,
	MT_SPINBOBERT_FIRE1,
	MT_SPINBOBERT_FIRE2,
	MT_HANGSTER,

	// Utility Objects
	MT_TELEPORTMAN,
	MT_ALTVIEWMAN,
	MT_CRUMBLEOBJ, // Sound generator for crumbling platform
	MT_TUBEWAYPOINT,
	MT_PUSH,
	MT_PULL,
	MT_GHOST,
	MT_OVERLAY,
	MT_ANGLEMAN,
	MT_POLYANCHOR,
	MT_POLYSPAWN,

	// Skybox objects
	MT_SKYBOX,

	// Debris
	MT_SPARK, //spark
	MT_EXPLODE, // Robot Explosion
	MT_UWEXPLODE, // Underwater Explosion
	MT_DUST,
	MT_ROCKSPAWNER,
	MT_FALLINGROCK,
	MT_ROCKCRUMBLE1,
	MT_ROCKCRUMBLE2,
	MT_ROCKCRUMBLE3,
	MT_ROCKCRUMBLE4,
	MT_ROCKCRUMBLE5,
	MT_ROCKCRUMBLE6,
	MT_ROCKCRUMBLE7,
	MT_ROCKCRUMBLE8,
	MT_ROCKCRUMBLE9,
	MT_ROCKCRUMBLE10,
	MT_ROCKCRUMBLE11,
	MT_ROCKCRUMBLE12,
	MT_ROCKCRUMBLE13,
	MT_ROCKCRUMBLE14,
	MT_ROCKCRUMBLE15,
	MT_ROCKCRUMBLE16,

	// Level debris
	MT_GFZDEBRIS,
	MT_BRICKDEBRIS,
	MT_WOODDEBRIS,
	MT_REDBRICKDEBRIS, // for CEZ3
	MT_BLUEBRICKDEBRIS, // for CEZ3
	MT_YELLOWBRICKDEBRIS, // for CEZ3

#ifdef SEENAMES
	MT_NAMECHECK,
#endif

	MT_FIRSTFREESLOT,
	MT_LASTFREESLOT = MT_FIRSTFREESLOT + NUMMOBJFREESLOTS - 1,
	NUMMOBJTYPES
} mobjtype_t;

typedef struct
{
	INT32 doomednum;
	statenum_t spawnstate;
	INT32 spawnhealth;
	statenum_t seestate;
	sfxenum_t seesound;
	INT32 reactiontime;
	sfxenum_t attacksound;
	statenum_t painstate;
	INT32 painchance;
	sfxenum_t painsound;
	statenum_t meleestate;
	statenum_t missilestate;
	statenum_t deathstate;
	statenum_t xdeathstate;
	sfxenum_t deathsound;
	fixed_t speed;
	fixed_t radius;
	fixed_t height;
	INT32 dispoffset;
	INT32 mass;
	INT32 damage;
	sfxenum_t activesound;
	UINT32 flags;
	statenum_t raisestate;
} mobjinfo_t;

extern mobjinfo_t mobjinfo[NUMMOBJTYPES];

void P_PatchInfoTables(void);

void P_BackupTables(void);

void P_ResetData(INT32 flags);

#endif
