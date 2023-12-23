#include "3dc.h"
#include "conscmnd.hpp"
#include "strutil.h"

// Includes for the actual commands:
//#include "consvar.hpp"
//#include "modcmds.hpp"
//#include "textexp.hpp"
//#include "trepgadg.hpp"

#define UseLocalAssert Yes
#include "ourasert.h"

#define NOT_PUBLIC_RELEASE 1

extern "C"
{
#include "3dc.h"
#include "module.h"
#include "inline.h"

#include "stratdef.h"
#include "gamedef.h"
#include "gameplat.h"

#include "bh_types.h"

#include "showcmds.h"
#include "version.h"
#include "equipmnt.h"
#include "cheat.h"
#include "cd_player.h"
#include "dynblock.h"
#include "bh_RubberDuck.h"
#include "pvisible.h"
#include "pldnet.h"

#include "lighting.h"
#include "paintball.h"		  
#include "decal.h"
#include "ConsoleLog.hpp"
#include "psndplat.h"
#include "avp_menus.h"
#include "smacker.h"
#include "detaillevels.h"
#include "savegame.h"
#include "scream.h"

#include "load_shp.h"
#include "bh_alien.h"

int DebuggingCommandsActive=0;
int RamAttackInProgress;
int GrabAttackInProgress;
int DisplayRadioMenu;
int DisplayClasses;
int InfLamp = 0;
STRATEGYBLOCK *DropWeapon(VECTORCH *location, int type, char *name);

extern void GimmeCharge(void);
extern void Display_Inventory(void);
extern void OverLoadDrill(void);
extern void PCSelfDestruct(void);
extern void NewOnScreenMessage(unsigned char *messagePtr);
extern void ToggleShoulderLamp();
extern void GADGET_NewOnScreenMessage( ProjChar* messagePtr );
extern DPID GrabPlayer;
extern HMODELCONTROLLER PlayersWeaponHModelController;
extern int ThirdPersonActive;

// just change these to prototypes etc.
extern void QuickLoad()
{
	//set the load request
	LoadGameRequest = 0; //(that's slot 0 - not false)
}
extern void QuickSave()
{
	//set the save request
	SaveGameRequest = 0; //(that's slot 0 - not false)
}

void ConsoleCommandLoad(int slot)
{
	if(slot>=1 && slot<=NUMBER_OF_SAVE_SLOTS)
	{
		LoadGameRequest = slot-1;
	}
}

void ConsoleCommandSave(int slot)
{
	if(slot>=1 && slot<=NUMBER_OF_SAVE_SLOTS)
	{
		SaveGameRequest = slot-1;
	}
}

void ToggleAPC(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	DYNAMICSBLOCK *dynPtr = Player->ObStrategyBlock->DynPtr;
	LOCALASSERT(playerStatusPtr);

	if(AvP.PlayerType!=I_Marine) 
		return;

	if (AvP.Network != I_No_Network)
		return;

	if (playerStatusPtr->Honor) {
		playerStatusPtr->Honor = 0;
		playerStatusPtr->IAmUsingShoulderLamp=0;
		ThirdPersonActive = 0;
	} else {
		playerStatusPtr->Honor = 1;
		playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Faster = 0;
		playerStatusPtr->ViewPanX = 0;
		playerStatusPtr->IAmUsingShoulderLamp=1;
		ThirdPersonActive = 1;
	}
}

void ToggleRadioMenu(void)
{
	if (!DisplayRadioMenu)
		DisplayRadioMenu=1;
	else
		DisplayRadioMenu=0;
}

void Hug(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);

	if (AvP.PlayerType != I_Alien) return;
	if (!Player->ObStrategyBlock->DynPtr->IsInContactWithFloor) return;
	if (playerStatusPtr->Class != CLASS_EXF_W_SPEC)	return;

	playerStatusPtr->ViewPanX = 0;
	playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jump = 1;
	RamAttackInProgress=1;
}

void Ram(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	int pitch = (FastRandom()%256)-128;

	if (AvP.PlayerType != I_Alien) return;
	if (!Player->ObStrategyBlock->DynPtr->IsInContactWithFloor) return;
	if (GrabPlayer) return;

	/* Facehuggers hug instead of ram */
	if (playerStatusPtr->Class == CLASS_EXF_W_SPEC)
	{
		Hug();
		return;
	}
	playerStatusPtr->ViewPanX = 0;
	playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jump = 1;
	if (playerStatusPtr->Class == CLASS_EXF_SNIPER)
		PlayAlienSound(1, ASC_Scream_General, pitch, &playerStatusPtr->soundHandle, &(Player->ObStrategyBlock->DynPtr->Position));
	else
		PlayAlienSound(0, ASC_Scream_General, pitch, &playerStatusPtr->soundHandle, &(Player->ObStrategyBlock->DynPtr->Position));
	RamAttackInProgress=1;

	if (AvP.Network != I_No_Network)
		netGameData.myLastScream = ASC_Scream_General;
}

void Grab(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	extern void MeleeWeapon_90Degree_Front_Core(DAMAGE_PROFILE *damage,int multiple,int range);

	if (AvP.PlayerType != I_Alien) return;
	if (playerStatusPtr->Immobilized) return;
	if (GrabPlayer) return;

	MeleeWeapon_90Degree_Front_Core(&TemplateAmmo[AMMO_FRISBEE_FIRE].MaxDamage[AvP.PlayerType],ONE_FIXED,TemplateAmmo[AMMO_FRISBEE_FIRE].MaxRange);
	GrabAttackInProgress=1;
	InitHModelTweening(&PlayersWeaponHModelController,(ONE_FIXED>>4),HMSQT_AlienHUD,(int)AHSS_Both_Down,(ONE_FIXED/6),0);
}

void UseMotionTracker(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
	LOCALASSERT(playerStatusPtr);

	if (AvP.PlayerType!=I_Marine) return;
	if (playerStatusPtr->Honor) return;

	if (weaponPtr->WeaponIDNumber == WEAPON_SMARTGUN ||
		weaponPtr->WeaponIDNumber == WEAPON_FRISBEE_LAUNCHER ||
		weaponPtr->WeaponIDNumber == WEAPON_SADAR)
		return;

	if (playerStatusPtr->MTrackerType == 0) return;
	if (playerStatusPtr->MTrackerType == 3) return;

	if (playerStatusPtr->MTrackerType == 1) {
		playerStatusPtr->MTrackerType = 2;
	} else {
		playerStatusPtr->MTrackerType = 1;
	}
}

void InfLampToggle(void)
{
	if (AvP.PlayerType != I_Marine) return;

	if (AvP.Network != I_No_Network)
		return;

	if (InfLamp)
	{
		InfLamp = 0;
		//NewOnScreenMessage("INFINITE BATTERY DISABLED");
	} else {
		InfLamp = 1;
		//NewOnScreenMessage("INFINITE BATTERY ENABLED");
	}
}

void Toggle3rdPerson(void)
{
	if (AvP.Network != I_No_Network)
		return;

	if (ThirdPersonActive)
		ThirdPersonActive = 0;
	else
		ThirdPersonActive = 1;
}

void UseMedikit(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	LOCALASSERT(playerStatusPtr);

	NPC_DATA *NpcData;
	NPC_TYPES PlayerType;

	if (playerStatusPtr->Honor) return;

	switch(AvP.PlayerType) 
	{
		case(I_Marine):
		{
			switch (AvP.Difficulty) {
				case I_Easy:
					PlayerType=I_PC_Marine_Easy;
					break;
				default:
				case I_Medium:
					PlayerType=I_PC_Marine_Medium;
					break;
				case I_Hard:
					PlayerType=I_PC_Marine_Hard;
					break;
				case I_Impossible:
					PlayerType=I_PC_Marine_Impossible;
					break;
			}
			break;
		}
		case(I_Predator):
		{
			return;
			break;
		}
		case(I_Alien):
		{
			return;
			break;
		}
		default:
		{
			LOCALASSERT(1==0);
			break;
		}
	}
	NpcData=GetThisNpcData(PlayerType);
	LOCALASSERT(NpcData);

	if(AvP.PlayerType!=I_Marine) return;

	if (playerStatusPtr->Class == CLASS_MEDIC_PR ||
		playerStatusPtr->Class == CLASS_MEDIC_FT ||
		playerStatusPtr->Class == CLASS_ENGINEER)
		return;

	if (playerStatusPtr->Medikit == 0) return;

	if (Player->ObStrategyBlock->SBDamageBlock.IsOnFire) {
		Player->ObStrategyBlock->SBDamageBlock.IsOnFire=0;
	} else if (Player->ObStrategyBlock->SBDamageBlock.Health==NpcData->StartingStats.Health<<ONE_FIXED_SHIFT) {
		return;
	}
	Player->ObStrategyBlock->SBDamageBlock.Health=NpcData->StartingStats.Health<<ONE_FIXED_SHIFT;
	playerStatusPtr->Health=Player->ObStrategyBlock->SBDamageBlock.Health;
	playerStatusPtr->Medikit = 0;
	Sound_Play(SID_PICKUP,"h");
}

#if 1
void Restore(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	LOCALASSERT(playerStatusPtr);

	NPC_DATA *NpcData;
	NPC_TYPES PlayerType;

	if (AvP.Network != I_No_Network)
		return;

	switch(AvP.PlayerType) 
	{
		case(I_Marine):
		{
			switch (AvP.Difficulty) {
				case I_Easy:
					PlayerType=I_PC_Marine_Easy;
					break;
				default:
				case I_Medium:
					PlayerType=I_PC_Marine_Medium;
					break;
				case I_Hard:
					PlayerType=I_PC_Marine_Hard;
					break;
				case I_Impossible:
					PlayerType=I_PC_Marine_Impossible;
					break;
			}
			break;
		}
		case(I_Predator):
		{
			switch (AvP.Difficulty) {
				case I_Easy:
					PlayerType=I_PC_Predator_Easy;
					break;
				default:
				case I_Medium:
					PlayerType=I_PC_Predator_Medium;
					break;
				case I_Hard:
					PlayerType=I_PC_Predator_Hard;
					break;
				case I_Impossible:
					PlayerType=I_PC_Predator_Impossible;
					break;
			}
			break;
		}
		case(I_Alien):
		{
			switch (AvP.Difficulty) {
				case I_Easy:
					PlayerType=I_PC_Alien_Easy;
					break;
				default:
				case I_Medium:
					PlayerType=I_PC_Alien_Medium;
					break;
				case I_Hard:
					PlayerType=I_PC_Alien_Hard;
					break;
				case I_Impossible:
					PlayerType=I_PC_Alien_Impossible;
					break;
			}
			break;
		}
		default:
		{
			LOCALASSERT(1==0);
			break;
		}
	}
	NpcData=GetThisNpcData(PlayerType);
	LOCALASSERT(NpcData);

	if (Player->ObStrategyBlock->SBDamageBlock.IsOnFire) {
		Player->ObStrategyBlock->SBDamageBlock.IsOnFire=0;
	}
	Player->ObStrategyBlock->SBDamageBlock.Health=NpcData->StartingStats.Health<<ONE_FIXED_SHIFT;
	playerStatusPtr->Health=Player->ObStrategyBlock->SBDamageBlock.Health;

	Player->ObStrategyBlock->SBDamageBlock.Armour=NpcData->StartingStats.Armour<<ONE_FIXED_SHIFT;
	playerStatusPtr->Armour=Player->ObStrategyBlock->SBDamageBlock.Armour;

}

void GodMode(void)
{
	PLAYER_STATUS *psPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	LOCALASSERT(psPtr);

	if (AvP.Network != I_No_Network)
		return;

	if (psPtr->IsImmortal)
	{
		GADGET_NewOnScreenMessage("IMMORTALITY DISABLED");
		psPtr->IsImmortal = 0;
	} else {
		GADGET_NewOnScreenMessage("IMMORTALITY ENABLED");
		psPtr->IsImmortal = 1;
	}
}
#endif

extern void DisplaySavesLeft();

extern void KickPlayer(int index);

struct DEBUGGINGTEXTOPTIONS ShowDebuggingText;

extern void ChangeNetGameType_Individual();
extern void ChangeNetGameType_Coop();
extern void ChangeNetGameType_LastManStanding();
extern void ChangeNetGameType_PredatorTag();
extern void ShowNearestPlayersName();
extern void ScreenShot(void);
extern void CastAlienBot(void);
extern void CastMarineBot(int weapon);
extern void CastPredoBot(int weapon);
extern void CastPredAlienBot(void);
extern void CastPraetorianBot(void);
extern void CastXenoborg(void);
extern void CastSentrygun(void);
extern void ChangeToAlien();
extern void ChangeToPredator();

extern int ShowMultiplayerScoreTimer;

void DeploySentry(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	LOCALASSERT(playerStatusPtr);

	if(AvP.PlayerType!=I_Marine) return;

	if (playerStatusPtr->IHaveAPlacedAutogun == 0) return;

	if (playerStatusPtr->IHaveAPlacedAutogun == 1) {
		CastSentrygun();
		playerStatusPtr->IHaveAPlacedAutogun = 0;
	}
}
static void ShowFPS(void)
{
	ShowDebuggingText.FPS = ~ShowDebuggingText.FPS;
}
static void ShowEnvironment(void)
{
	ShowDebuggingText.Environment = ~ShowDebuggingText.Environment;
}
static void ShowCoords(void)
{
	ShowDebuggingText.Coords = ~ShowDebuggingText.Coords;
}
static void ShowModule(void)
{
	ShowDebuggingText.Module = ~ShowDebuggingText.Module;
}
static void ShowTarget(void)
{
	ShowDebuggingText.Target = ~ShowDebuggingText.Target;
}
static void ShowNetworking(void)
{
	ShowDebuggingText.Networking = ~ShowDebuggingText.Networking;
}
static void ShowDynamics(void)
{
	ShowDebuggingText.Dynamics = ~ShowDebuggingText.Dynamics;
}
static void ShowGunPos(void)
{
	ShowDebuggingText.GunPos = ~ShowDebuggingText.GunPos;
}
static void ShowTears(void)
{
	ShowDebuggingText.Tears = ~ShowDebuggingText.Tears;
}
static void ShowPolyCount(void)
{
	ShowDebuggingText.PolyCount = ~ShowDebuggingText.PolyCount;
}
static void ShowSounds(void)
{
	ShowDebuggingText.Sounds = ~ShowDebuggingText.Sounds;
}


extern void ChangeToMarine();
extern void ChangeToAlien();
extern void ChangeToPredator();
extern int InGameMenusAreRunning(void);

void ChangeSpecies()
{
	PLAYER_STATUS *psPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	LOCALASSERT(psPtr);

	if (InGameMenusAreRunning()) return;

	if (AvP.Network == I_No_Network) return;

	if (AvP.PlayerType == I_Alien) {
		ChangeToAlien();
	} else if (AvP.PlayerType == I_Marine) {
		ChangeToMarine();
	} else {
		ChangeToPredator();
	}
	psPtr->Class = 20; //CLASS_SPECIES_CHANGE...
}
void ChangeClass()
{
	PLAYER_STATUS *psPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	LOCALASSERT(psPtr);

	if (InGameMenusAreRunning()) return;

	if (AvP.Network == I_No_Network) return;

	if ((netGameData.LifeCycle) && (AvP.PlayerType == I_Alien)) return;

	//psPtr->Class = CLASS_NONE;
	//psPtr->invulnerabilityTimer = ONE_FIXED*120;
	DisplayClasses = 1;
}

extern void ShowMultiplayerScores()
{
	ShowMultiplayerScoreTimer=5*ONE_FIXED;
}

extern void AddNetMsg_ChatBroadcast(char *string,BOOL same_species_only);

static void DoMultiplayerSay(char* string)
{
	//if (PlayerStatusPtr->IsAlive) // anti-cheat
		AddNetMsg_ChatBroadcast(string,FALSE);
}

static void DoMultiplayerSaySpecies(char* string)
{
	//if (PlayerStatusPtr->IsAlive) // anti-cheat
		AddNetMsg_ChatBroadcast(string,TRUE);
}

static void ForceAssertionFailure(void)
{
	LOCALASSERT("This assertion has been forced to stop the game"==0);
}


		
static void CDCommand_Play(int track)
{
	if(!CDDA_IsOn()) CDDA_SwitchOn();

	CDDA_Stop();
	CDDA_Play(track);
}
void CDCommand_PlayLoop(int track)
{
	if(!CDDA_IsOn()) CDDA_SwitchOn();

	CDDA_Stop();
	CDDA_PlayLoop(track);
}

static void CDCommand_Stop(void)
{
	CDDA_Stop();
}

static void CDCommand_Volume(int volume)
{
	if (volume>=0 && volume<=127)
	{
		CDDA_ChangeVolume(volume);
	}
	else
	{
		// say the volume setting is incorrect
	}
}


static void GunX(int x)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
	TEMPLATE_WEAPON_DATA *twPtr = &TemplateWeapon[weaponPtr->WeaponIDNumber];

	twPtr->RestPosition.vx = x;
}
static void GunY(int y)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
	TEMPLATE_WEAPON_DATA *twPtr = &TemplateWeapon[weaponPtr->WeaponIDNumber];

	twPtr->RestPosition.vy = y;
}
static void GunZ(int z)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
	TEMPLATE_WEAPON_DATA *twPtr = &TemplateWeapon[weaponPtr->WeaponIDNumber];

	twPtr->RestPosition.vz = z;
}

static void MakeRotatingLight(void)
{
	MakeLightElement(&Player->ObWorld,LIGHTELEMENT_ROTATING);
}
VECTORCH boing = {12345,12345,12345};
VECTORCH boing2 = {23451,34512,45123};

static void Trash_Frame_Rate(void)
{
	int i=0;

	for (i=0; i<10000000; i++)
	{
	 //	Normalise(&boing);
		boing.vx += boing2.vx+FastRandom();
		boing.vy += boing2.vy;
		boing.vz += boing2.vz;
	}
}


void RestartMultiplayer(void)
{
	extern EnoughPlayersAreHere(void);

	/* obviously have to be in a network game... */
	if (AvP.Network==I_No_Network) return;

	/* AND be the host! */
	if (AvP.Network!=I_Host) return;

	/* Must also be enough players */
	if (!EnoughPlayersAreHere()) return;

	int seed=FastRandom();
	AddNetMsg_RestartNetworkGame(seed);
	RestartNetworkGame(seed);
}

static void CompleteLevel(void)
{
	if (AvP.Network != I_No_Network)
		return;

	AvP.LevelCompleted = 1;
}


void CreateGameSpecificConsoleCommands(void)
{
	ShowDebuggingText.FPS = 0;
	ShowDebuggingText.Environment = 0;
	ShowDebuggingText.Coords = 0;
	ShowDebuggingText.Module = 0;
	ShowDebuggingText.Target = 0;
	ShowDebuggingText.Networking = 0;
	ShowDebuggingText.Dynamics = 0;
	ShowDebuggingText.GunPos = 0;
	ShowDebuggingText.Tears = 0;
	ShowDebuggingText.PolyCount = 0;

	#ifndef AVP_DEBUG_VERSION 
	BOOL IsACheat = TRUE;
	#else
	BOOL IsACheat = FALSE;
	#endif
	
	#ifndef AVP_DEBUG_VERSION // allow debug commands without -debug
	#ifndef AVP_DEBUG_FOR_FOX // allow debug commands without -debug
	if (DebuggingCommandsActive)
	#endif
	#endif
	{
		ConsoleCommand::Make
		(
			"GIVEALLWEAPONS",
			"BE CAREFUL WHAT YOU WISH FOR",
			GiveAllWeaponsCheat
		);


		/* KJL 14:51:09 29/03/98 - show commands */
		ConsoleCommand::Make
		(
			"SHOWFPS",
			"DISPLAY THE FRAMERATE",
			ShowFPS,
			IsACheat
		);

		ConsoleCommand::Make
		(
			"SHOWPOLYCOUNT",
			"DISPLAY NUMBER OF LANDSCAPE POLYS, AND NUMBER OF POLYS ACTUALLY RENDERED",
			ShowPolyCount,
			IsACheat
		);
		ConsoleCommand::Make
		(
			"LIGHT",
			"CREATE A LIGHT",
			MakeRotatingLight,
			IsACheat
		);	 
		ConsoleCommand :: Make
		(
			"GIMME_CHARGE",
			"GRANTS FULL FIELD CHARGE",
			GimmeCharge,
			IsACheat
		);
		ConsoleCommand :: Make
		(
			"ALIENBOT",
			"CREATES ALIEN BOT. SAME AS ANY OTHER ALIEN.",
			CastAlienBot,
			IsACheat
		);

		ConsoleCommand :: Make
		(
			"MARINEBOT",
			"CREATES MARINE BOT. SAME AS A GENERATED MARINE.",
			CastMarineBot,
			IsACheat
		);

		ConsoleCommand :: Make
		(
			"PREDOBOT",
			"CREATES PREDATOR BOT.",
			CastPredoBot,
			IsACheat
		);

		ConsoleCommand :: Make
		(
			"PREDALIENBOT",
			"CREATES PREDATOR ALIEN BOT.",
			CastPredAlienBot,
			IsACheat
		);

		ConsoleCommand :: Make
		(
			"PRAETORIANBOT",
			"CREATES PRAETORIAN GUARD BOT.",
			CastPraetorianBot,
			IsACheat
		);

		ConsoleCommand :: Make
		(
			"XENOBORG",
			"THEY'RE ALL BOTS ANYWAY...",
			CastXenoborg,
			IsACheat
		);


	}

	#if CONSOLE_DEBUGGING_COMMANDS_ACTIVATED
	ConsoleCommand::Make
	(
		"SHOWENV",
		"DISPLAY THE ENVIRONMENT NAME",
		ShowEnvironment
	);

	ConsoleCommand::Make
	(
		"SHOWCOORDS",
		"DISPLAY THE PLAYERS CURRENT POSITION",
		ShowCoords
	);

	ConsoleCommand::Make
	(
		"SHOWMODULE",
		"DISPLAY THE PLAYERS CURRENT MODULE",
		ShowModule
	);

	ConsoleCommand::Make
	(
		"SHOWTARGET",
		"DISPLAY THE CURRENT TARGET POSITION",
		ShowTarget
	);

	ConsoleCommand::Make
	(
		"SHOWNETWORKING",
		"DISPLAY NETWORKING DEBUG TEXT",
		ShowNetworking
	);
	
	ConsoleCommand::Make
	(
		"SHOWDYNAMICS",
		"DISPLAY DYNAMICS DEBUG TEXT",
		ShowDynamics
	);

	ConsoleCommand::Make
	(
		"SHOWGUNPOS",
		"DISPLAY GUN OFFSET COORDS",
		ShowGunPos
	);

	ConsoleCommand::Make
	(
		"SHOWTEARS",
		"MAKE TEARS AND LINKING ERRORS APPEAR BRIGHT GREEN",
		ShowTears
	);

	ConsoleCommand::Make
	(
		"SHOWSOUNDS",
		"DISPLAY NUMBER OF ACTIVE SOUNDS",
		ShowSounds
	);


	#if 1
	ConsoleCommand::Make
	(
		"GUNX",
		"CHANGE POSITION",
		GunX
	);
	ConsoleCommand::Make
	(
		"GUNY",
		"CHANGE POSITION",
		GunY
	);
	ConsoleCommand::Make
	(
		"GUNZ",
		"CHANGE POSITION",
		GunZ
	);
	ConsoleCommand::Make
	(
		"DUCKBOT",
		"MAKE A RUBBER DUCK",
		CreateRubberDuckBot
	);

	ConsoleCommand::Make
	(
		"FORCEASSERTIONFAILURE",
		"MAKE AN ASSERTION FIRE, EXITING THE GAME",
		ForceAssertionFailure
	);
	#endif

	ConsoleCommand::Make
	(
		"RESTARTMULTIPLAYER",
		"RESTARTS A NETWORK GAME FROM SCRATCH",
		RestartMultiplayer
	);
	#if 0
	ConsoleCommand::Make
	(
		"NEWPLANET",
		"",
		NewPlanet
	);
	#endif
	ConsoleCommand::Make
	(
		"PAINTBALL",
		"TOGGLES PAINTBALLMODE ON/OFF",
		TogglePaintBallMode
	);

	ConsoleCommand::Make
	(
		"BUG",
		"ADD A BUG REPORT TO CONSOLELOG.TXT",
		OutputBugReportToConsoleLogfile
	);
	ConsoleCommand::Make
	(
		"REMOVEDECALS",
		"DELETES ALL PRE-DECALS",
		RemoveAllFixedDecals
	);
	
	ConsoleCommand::Make
	(
		"TURN3DSOUNDHARDWAREOFF",
		"DEACTIVATES 3D SOUND IN HARDWARE",
		PlatDontUse3DSoundHW
	);
	ConsoleCommand::Make
	(
		"TURN3DSOUNDHARDWAREON",
		"ACTIVATES 3D SOUND IN HARDWARE",
		PlatUse3DSoundHW
	);

	ConsoleCommand::Make
	(
		"NETGAME_INDIVIDUAL",
		"CHANGE NETWORK GAME TYPE",
		ChangeNetGameType_Individual
	);
	ConsoleCommand::Make
	(
		"NETGAME_COOP",
		"CHANGE NETWORK GAME TYPE",
		ChangeNetGameType_Coop
	);
	ConsoleCommand::Make
	(
		"NETGAME_LASTMANSTANDING",
		"CHANGE NETWORK GAME TYPE",
		ChangeNetGameType_LastManStanding
	);
	ConsoleCommand::Make
	(
		"NETGAME_PREDATORTAG",
		"CHANGE NETWORK GAME TYPE",
		ChangeNetGameType_PredatorTag
	);


	ConsoleCommand::Make
	(
		"TRIGGER_PLOT_FMV",
		"",
		StartTriggerPlotFMV
	);

	
	ConsoleCommand::Make
	(
		"SPECIALISTMARINE_PULSERIFLE",
		"Become a pulserifle marine",
		ChangeToSpecialist_PulseRifle
	);
	ConsoleCommand::Make
	(
		"SPECIALISTMARINE_SMARTGUN",
		"Become a smartgun marine",
		ChangeToSpecialist_Smartgun
	);
	ConsoleCommand::Make
	(
		"SPECIALISTMARINE_FLAMER",
		"Become a flamethrower marine",
		ChangeToSpecialist_Flamer
	);
	ConsoleCommand::Make
	(
		"SPECIALISTMARINE_SADAR",
		"Become a sadar marine",
		ChangeToSpecialist_Sadar
	);
	ConsoleCommand::Make
	(
		"SPECIALISTMARINE_GRENADELAUNCHER",
		"Become a grenade launcher marine",
		ChangeToSpecialist_GrenadeLauncher
	);
	ConsoleCommand::Make
	(
		"SPECIALISTMARINE_MINIGUN",
		"Become a minigun marine",
		ChangeToSpecialist_Minigun
	);
	ConsoleCommand::Make
	(
		"SPECIALISTMARINE_SD",
		"Become an SD marine",
		ChangeToSpecialist_Frisbee
	);
	
	#if 1
	ConsoleCommand::Make
	(
		"TRASH_FRAME_RATE",
		"",
		Trash_Frame_Rate
	);
	
	ConsoleCommand::Make
	(
		"COMPLETE_LEVEL",
		"",
		CompleteLevel
	);
	#endif
	#endif
	/* KJL 15:52:41 29/03/98 - version info */
	ConsoleCommand::Make
	(
		"VERSION",
		"",
		GiveVersionDetails
	);

	ConsoleCommand::Make
	(
		"SAY",
		"BROADCAST MESSAGE",
		DoMultiplayerSay
	);

	ConsoleCommand::Make
	(
		"SAY_SPECIES",
		"BROADCAST MESSAGE",
		DoMultiplayerSaySpecies
	);

 	ConsoleCommand::Make
	(
		"CDSTOP",
		"STOP THE CD PLAYING",
		CDCommand_Stop
	);

	ConsoleCommand::Make
	(
		"CDPLAY",
		"SELECT A TRACK TO PLAY",
		CDCommand_Play
	);
	ConsoleCommand::Make
	(
		"CDPLAYLOOP",
		"SELECT A TRACK TO PLAY LOOPED",
		CDCommand_PlayLoop
	);
	
	ConsoleCommand::Make
	(
		"CDVOLUME",
		"SELECT SOUND LEVEL 0 TO 127",
		CDCommand_Volume
	);
	ConsoleCommand::Make
	(
		"ID_PLAYER",
		"Get name of player nearest centre of screen",
		ShowNearestPlayersName
	);
	ConsoleCommand::Make
	(
		"SHOW_SCORE",
		"Show frag table",
		ShowMultiplayerScores
	);

	ConsoleCommand::Make
	(
		"DETAIL_LEVEL_MAX",
		"",
		SetToDefaultDetailLevels
	);
	
	ConsoleCommand::Make
	(
		"DETAIL_LEVEL_MIN",
		"",
		SetToMinimalDetailLevels
	);

	ConsoleCommand::Make
	(
		"SCREENSHOT",
		"",
		ScreenShot
	);
	
	ConsoleCommand::Make
	(
		"QUICKSAVE",
		"",
		QuickSave
	);
	ConsoleCommand::Make
	(
		"QUICKLOAD",
		"",
		QuickLoad
	);
	ConsoleCommand::Make
	(
		"SAVE",
		"Save game to slot 1-8",
		ConsoleCommandSave
	);
	ConsoleCommand::Make
	(
		"LOAD",
		"Load game from slot 1-8",
		ConsoleCommandLoad
	);
	ConsoleCommand::Make
	(
		"SAVESLEFT",
		"",
		DisplaySavesLeft
	);
	ConsoleCommand::Make
	(
		"KICK",
		"Kick a player.",
		KickPlayer
	);

	#if NOT_PUBLIC_RELEASE
	ConsoleCommand::Make
	(
		"GIVESTUFF",
		"GIVES YOU LOTSA NEW STUFF.",
		GiveAllWeaponsCheat
	);
	ConsoleCommand::Make
	(
		"GOD",
		"Become the almight God of AVP.. muahahah!",
		GodMode
	);
	ConsoleCommand::Make
	(
		"RESTORE",
		"Restore full Armor and Health.",
		Restore
	);
	ConsoleCommand::Make
	(
		"APC",
		"Drive an APC!",
		ToggleAPC
	);
	ConsoleCommand::Make
	(
		"INFLAMP",
		"Infinite Lamp Battery.",
		InfLampToggle
	);
	ConsoleCommand::Make
	(
		"COMPLETELEVEL",
		"Complete the level.",
		CompleteLevel
	);
	ConsoleCommand::Make
	(
		"THIRDPERSON",
		"Change to 3rd-person viewmode.",
		Toggle3rdPerson
	);
	#endif
	ConsoleCommand::Make
	(
		"MPRESTART",
		"RESTARTS A NETWORK GAME FROM SCRATCH",
		RestartMultiplayer
	);
}	



						

} // extern "C"