/*------------------------------- Patrick 21/10/96 ------------------------------
  Source for reading player inputs.
  Note that whilst ReadUserInput() reads raw input data, the functions in this
  file map those inputs onto the player movement structures (defined in Player
  Status).  This is, of course, entirely platform dependant.  Consoles will
  need their own equivalent functions.... 

  -------------------------------------------------------------------------------*/ 
#include "3dc.h"
#include "module.h"
#include "inline.h"

#include "stratdef.h"
#include "gamedef.h"
#include "gameplat.h"

#include "bh_types.h"
#include "ourasert.h"
#include "comp_shp.h"

#include "pmove.h"
#include "usr_io.h"
#include "hud_map.h"
#include "krender.h"

#include "iofocus.h"

#include "paintball.h"
#include "avp_menus.h"

#include "pldnet.h"

extern int InGameMenusAreRunning(void);
extern void AvP_TriggerInGameMenus(void);
extern void Recall_Disc(void);
extern void Reload_Weapon(void);
extern void ShowMultiplayerScores(void);
extern void ThrowAFlare();
extern void StartPlayerTaunt();
extern void MessageHistory_DisplayPrevious();
extern void BringDownConsoleWithSayTypedIn();
extern void BringDownConsoleWithSaySpeciesTypedIn();
extern void MaintainZoomingLevel();
extern void ToggleShoulderLamp();
extern void FireAPCGun();
extern void OverLoadDrill(PLAYER_WEAPON_DATA *weaponPtr);
extern void UseMedikit();
extern void DeploySentry();
extern void ChangeClass();
extern void ChangeSpecies();
extern void Grab();
extern void Ram();
extern void ToggleRadioMenu();

extern void PaintBallMode_Rotate();
extern void PaintBallMode_ChangeSubclass(int delta);
extern void PaintBallMode_Randomise();
extern void PaintBallMode_RemoveDecal();

extern int NormalFrameTime;
extern int CurrentSpeed;
extern int GrabAttackInProgress;
extern int DisplayRadioMenu;
extern int DisplayClasses;
int Run;
extern int Underwater;
extern int ZeroG;
extern int OnLadder;
extern int HackPool;
extern int WeldPool;

FIXED_INPUT_CONFIGURATION FixedInputConfig =
{
	KEY_1,				// Weapon1;
	KEY_2,				// Weapon2;
	KEY_3,				// Weapon3;
	KEY_4,				// Weapon4;
	KEY_5,				// Weapon5;
	KEY_6,				// Weapon6;
	KEY_7,				// Weapon7;
	KEY_8,				// Weapon8;
	KEY_9,				// Weapon9;
	KEY_0,				// Weapon10;
	KEY_ESCAPE,			// PauseGame;

};

PLAYER_INPUT_CONFIGURATION MarineInputPrimaryConfig;
PLAYER_INPUT_CONFIGURATION MarineInputSecondaryConfig;
PLAYER_INPUT_CONFIGURATION PredatorInputPrimaryConfig;
PLAYER_INPUT_CONFIGURATION PredatorInputSecondaryConfig;
PLAYER_INPUT_CONFIGURATION AlienInputPrimaryConfig;
PLAYER_INPUT_CONFIGURATION AlienInputSecondaryConfig;

PLAYER_INPUT_CONFIGURATION DefaultMarineInputPrimaryConfig =
{
	KEY_W,				// Forward;
	KEY_S,			    // Backward;
	KEY_VOID, 			// Left;
	KEY_VOID,	 		// Right;

	KEY_VOID,			// Strafe;
	KEY_A,		 		// StrafeLeft;
	KEY_D,		 		// StrafeRight;

	KEY_VOID,			// LookUp;
	KEY_VOID, 			// LookDown;
	KEY_VOID,			// CentreView;

	KEY_LEFTCTRL,		// Walk;
	KEY_LEFTSHIFT, 		// Crouch;
	KEY_SPACE,			// Jump;

	KEY_E,				// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;

	KEY_MOUSEWHEELUP,  	// NextWeapon;
	KEY_MOUSEWHEELDOWN, // PreviousWeapon;
	KEY_BACKSPACE,		// FlashbackWeapon;

	KEY_T,		  		// ImageIntensifier;
	KEY_F,		  		// ThrowFlare;
	KEY_C,			  	// Jetpack;
	KEY_R,				// Taunt
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
	KEY_G,				// Throw Grenade
	KEY_M,				// Use Medikit
	KEY_Y,				// Deploy Sentry Gun
	KEY_F9,				// Change Species
	KEY_F10,			// Change Class
};
PLAYER_INPUT_CONFIGURATION DefaultPredatorInputPrimaryConfig =
{
	KEY_W,				// Forward;
	KEY_S,				// Backward;
	KEY_VOID, 			// Left;
	KEY_VOID, 			// Right;

	KEY_VOID,			// Strafe;
	KEY_A,	 			// StrafeLeft;
	KEY_D,	 			// StrafeRight;

	KEY_VOID, 			// LookUp;
	KEY_VOID, 			// LookDown;
	KEY_VOID,			// CentreView;

	KEY_LEFTCTRL,		// Walk;
	KEY_LEFTSHIFT, 		// Crouch;
	KEY_SPACE,			// Jump;

	KEY_E,				// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;
	
	KEY_VOID,			// NextWeapon;
	KEY_VOID, 			// PreviousWeapon;
	KEY_BACKSPACE,		// FlashbackWeapon;
	
	KEY_C,	 			// Cloak;
	KEY_Q,	 			// CycleVisionMode;
	KEY_MOUSEWHEELUP,	// ZoomIn;
	KEY_MOUSEWHEELDOWN,	// ZoomOut;
	KEY_END,	  		// GrapplingHook
	KEY_COMMA,			// RecallDisk
	KEY_R,				// Taunt
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
	KEY_F9,				// Change Species
	KEY_F10,			// Change Class
};

PLAYER_INPUT_CONFIGURATION DefaultAlienInputPrimaryConfig =
{
	KEY_W,				// Forward;
	KEY_S,				// Backward;
	KEY_VOID,			// Left;
	KEY_VOID,	 		// Right;

	KEY_VOID,			// Strafe;
	KEY_A,				// StrafeLeft;
	KEY_D,		 		// StrafeRight;

	KEY_VOID, 			// LookUp;
	KEY_VOID, 			// LookDown;
	KEY_VOID,			// CentreView;

	KEY_LEFTCTRL,		// Walk;
	KEY_LEFTSHIFT, 		// Crouch;
	KEY_SPACE,			// Jump;

	KEY_E,				// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;

	KEY_Q,				// AlternateVision;
	KEY_R,				// Taunt;
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
	KEY_F9,				// Change Species
	KEY_F10,			// Change Class
};

PLAYER_INPUT_CONFIGURATION DefaultMarineInputSecondaryConfig =
{
	KEY_VOID,			// Forward;
	KEY_VOID,			// Backward;
	KEY_VOID, 			// Left;
	KEY_VOID, 			// Right;

	KEY_VOID,			// Strafe;
	KEY_VOID,	 		// StrafeLeft;
	KEY_VOID,	 		// StrafeRight;

	KEY_VOID,			// LookUp;
	KEY_VOID,			// LookDown;
	KEY_VOID,			// CentreView;

	KEY_VOID,			// Walk;
	KEY_VOID,			// Crouch;
	KEY_VOID,			// Jump;

	KEY_VOID,			// Operate;

	KEY_VOID,			// FirePrimaryWeapon;
	KEY_VOID,			// FireSecondaryWeapon;

	KEY_VOID,	 		// NextWeapon;
	KEY_VOID,			// PreviousWeapon;
	KEY_VOID,			// FlashbackWeapon;

	KEY_VOID,			// ImageIntensifier;
	KEY_VOID, 			// ThrowFlare;
	KEY_VOID, 			// Jetpack;
	KEY_VOID,			// Taunt

	KEY_VOID,
	KEY_VOID,
	KEY_VOID,
	KEY_VOID,
	KEY_VOID,			// Throw Grenade
	KEY_VOID,			// Use Medikit
	KEY_VOID,			// Deploy Sentry Gun
	KEY_VOID,			// Change Species
	KEY_VOID,			// Change Class
};

PLAYER_INPUT_CONFIGURATION DefaultPredatorInputSecondaryConfig =
{
	KEY_VOID,			// Forward;
	KEY_VOID,			// Backward;
	KEY_VOID, 			// Left;
	KEY_VOID, 			// Right;

	KEY_VOID,			// Strafe;
	KEY_VOID,	 		// StrafeLeft;
	KEY_VOID,	 		// StrafeRight;

	KEY_VOID,			// LookUp;
	KEY_VOID,			// LookDown;
	KEY_VOID,			// CentreView;

	KEY_VOID,			// Walk;
	KEY_VOID,			// Crouch;
	KEY_VOID,			// Jump;

	KEY_VOID,			// Operate;

	KEY_VOID,	 		// FirePrimaryWeapon;
	KEY_VOID,	 		// FireSecondaryWeapon;

	KEY_VOID,			// NextWeapon;
	KEY_VOID, 			// PreviousWeapon;
	KEY_VOID,			// FlashbackWeapon;
	
	KEY_VOID,	 		// Cloak;
	KEY_VOID,	 		// CycleVisionMode;
	KEY_VOID,			// ZoomIn;
	KEY_VOID,			// ZoomOut;
	KEY_VOID,	 		// GrapplingHook;
	KEY_VOID,			// RecallDisk
	KEY_VOID,			// Taunt
	
	KEY_VOID,
	KEY_VOID,
	KEY_VOID,
	KEY_VOID,
	KEY_VOID,			// Change Species
	KEY_VOID,			// Change Class

};

PLAYER_INPUT_CONFIGURATION DefaultAlienInputSecondaryConfig =
{
	KEY_VOID,			// Forward;
	KEY_VOID,			// Backward;
	KEY_VOID,			// Left;
	KEY_VOID,			// Right;

	KEY_VOID,			// Strafe;
	KEY_VOID,	 		// StrafeLeft;
	KEY_VOID,	 		// StrafeRight;

	KEY_VOID,			// LookUp;
	KEY_VOID,			// LookDown;
	KEY_VOID,			// CentreView;

	KEY_VOID,			// Walk;
	KEY_VOID, 			// Crouch;
	KEY_VOID,			// Jump;

	KEY_VOID,			// Operate;

	KEY_VOID,			// FirePrimaryWeapon;
	KEY_VOID,			// FireSecondaryWeapon;
	
	KEY_VOID, 			// AlternateVision;
	KEY_VOID,	 		// Taunt;
	KEY_VOID,
	KEY_VOID,
	KEY_VOID,
	KEY_VOID,
	KEY_VOID,			// Change Species
	KEY_VOID,			// Change Class
};


CONTROL_METHODS ControlMethods =
{
	/* analogue stuff */
	DEFAULT_MOUSEX_SENSITIVITY, //unsigned int MouseXSensitivity;
	DEFAULT_MOUSEY_SENSITIVITY, //unsigned int MouseYSensitivity;

	0,//unsigned int VAxisIsMovement :1; // else it's looking
	1,//unsigned int HAxisIsTurning :1; // else it's sidestepping

	0,//unsigned int FlipVerticalAxis :1;
	
	/* general stuff */
	0,//unsigned int AutoCentreOnMovement :1;
};

CONTROL_METHODS DefaultControlMethods =
{
	/* analogue stuff */
	DEFAULT_MOUSEX_SENSITIVITY, //unsigned int MouseXSensitivity;
	DEFAULT_MOUSEY_SENSITIVITY, //unsigned int MouseYSensitivity;

	0,//unsigned int VAxisIsMovement :1; // else it's looking
	1,//unsigned int HAxisIsTurning :1; // else it's sidestepping

	0,//unsigned int FlipVerticalAxis :1;
	
	/* general stuff */
	0,//unsigned int AutoCentreOnMovement :1;
};

JOYSTICK_CONTROL_METHODS JoystickControlMethods =
{
	0,//unsigned int JoystickEnabled;
	
	1,//unsigned int JoystickVAxisIsMovement; // else it's looking
	1,//unsigned int JoystickHAxisIsTurning;  // else it's sidestepping
	0,//unsigned int JoystickFlipVerticalAxis;

	0,//unsigned int JoystickPOVVAxisIsMovement; // else it's looking
	0,//unsigned int JoystickPOVHAxisIsTurning;	 // else it's sidestepping
	0,//unsigned int JoystickPOVFlipVerticalAxis;

	0,//unsigned int JoystickRudderEnabled;		  
	0,//unsigned int JoystickRudderAxisIsTurning; // else it's sidestepping
	
	0,//unsigned int JoystickTrackerBallEnabled;
	0,//unsigned int JoystickTrackerBallFlipVerticalAxis;
	DEFAULT_TRACKERBALL_HORIZONTAL_SENSITIVITY,//unsigned int JoystickTrackerBallHorizontalSensitivity;
	DEFAULT_TRACKERBALL_VERTICAL_SENSITIVITY,//unsigned int JoystickTrackerBallVerticalSensitivity;

};
JOYSTICK_CONTROL_METHODS DefaultJoystickControlMethods =
{
	0,//unsigned int JoystickEnabled;
	
	1,//unsigned int JoystickVAxisIsMovement; // else it's looking
	1,//unsigned int JoystickHAxisIsTurning;  // else it's sidestepping
	0,//unsigned int JoystickFlipVerticalAxis;

	0,//unsigned int JoystickPOVVAxisIsMovement; // else it's looking
	0,//unsigned int JoystickPOVHAxisIsTurning;	 // else it's sidestepping
	0,//unsigned int JoystickPOVFlipVerticalAxis;

	0,//unsigned int JoystickRudderEnabled;		  
	0,//unsigned int JoystickRudderAxisIsTurning; // else it's sidestepping
	
	0,//unsigned int JoystickTrackerBallEnabled;
	0,//unsigned int JoystickTrackerBallFlipVerticalAxis;
	DEFAULT_TRACKERBALL_HORIZONTAL_SENSITIVITY,//unsigned int JoystickTrackerBallHorizontalSensitivity;
	DEFAULT_TRACKERBALL_VERTICAL_SENSITIVITY,//unsigned int JoystickTrackerBallVerticalSensitivity;
};

/* Extern for global keyboard buffer */
extern unsigned char KeyboardInput[];
extern unsigned char DebouncedKeyboardInput[];
extern int GotJoystick;
extern int GotMouse;

/* initialise the player input structure(s) in the player_status block */
void InitPlayerGameInput(STRATEGYBLOCK* sbPtr)
{
	PLAYER_STATUS *playerStatusPtr;

    /* get the player status block ... */
    playerStatusPtr = (PLAYER_STATUS *) (sbPtr->SBdataptr);
    LOCALASSERT(playerStatusPtr);
	

	/* analogue type inputs */
	playerStatusPtr->Mvt_MotionIncrement = 0;
	playerStatusPtr->Mvt_TurnIncrement = 0;
	playerStatusPtr->Mvt_PitchIncrement = 0;
	playerStatusPtr->Mvt_AnalogueTurning = 0;
	playerStatusPtr->Mvt_AnaloguePitching = 0;
	playerStatusPtr->Mvt_SideStepIncrement = 0;

	/* request flags */
	playerStatusPtr->Mvt_InputRequests.Mask = 0;
	playerStatusPtr->Mvt_InputRequests.Mask2 = 0;

	/* KJL 14:23:54 8/7/97 - default to walk */
	playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Faster = Run;

}


/* This function maps raw inputs onto the players movement attributes in
   the player_status block.  It is called from the ExecuteFreeMovement
   function.
   NB Currently, only keyboard input is supported. */
void ReadPlayerGameInput(STRATEGYBLOCK* sbPtr)
{
	PLAYER_INPUT_CONFIGURATION *primaryInput;
	PLAYER_INPUT_CONFIGURATION *secondaryInput;
	PLAYER_STATUS *playerStatusPtr;

    /* get the player status block ... */
    playerStatusPtr = (PLAYER_STATUS *) (sbPtr->SBdataptr);
    LOCALASSERT(playerStatusPtr);
	
	/* start off by initialising the inputs */
	InitPlayerGameInput(sbPtr);

	switch (AvP.PlayerType)
	{
		case I_Marine:
		{
			primaryInput = &MarineInputPrimaryConfig;
			secondaryInput = &MarineInputSecondaryConfig;
			break;
		}
		case I_Predator:
		{
			primaryInput = &PredatorInputPrimaryConfig;
			secondaryInput = &PredatorInputSecondaryConfig;
			break;
		}
		case I_Alien:
		{
			primaryInput = &AlienInputPrimaryConfig;
			secondaryInput = &AlienInputSecondaryConfig;
			break;
		}
	}

	if ( IOFOCUS_AcceptControls() && !InGameMenusAreRunning())
	{
		// Multiplayer class menus

		if (DisplayRadioMenu)
		{
			int rand = FastRandom()%2;

			if (DebouncedKeyboardInput[KEY_F1]) {
				if (rand) {
					AddNetMsg_ChatBroadcast("I need help!",TRUE);
					Sound_Play(SID_HELP_HELP,"hv",127);
					playerStatusPtr->AirStatus = SID_HELP_HELP;
				} else {
					AddNetMsg_ChatBroadcast("Requesting assistance!",TRUE);
					Sound_Play(SID_HELP_ASSISTANCE,"hv",127);
					playerStatusPtr->AirStatus = SID_HELP_ASSISTANCE;
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F2]) {
				if (rand) {
					AddNetMsg_ChatBroadcast("Cover me!",TRUE);
					Sound_Play(SID_ENGAGE_COVER,"hv",127);
					playerStatusPtr->AirStatus = SID_ENGAGE_COVER;
				} else {
					AddNetMsg_ChatBroadcast("Enemy spotted!",TRUE);
					Sound_Play(SID_ENGAGE_SPOTTED,"hv",127);
					playerStatusPtr->AirStatus = SID_ENGAGE_SPOTTED;
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F3]) {
				if (rand) {
					AddNetMsg_ChatBroadcast("Enemy down!",TRUE);
					Sound_Play(SID_KILL_DOWN,"hv",127);
					playerStatusPtr->AirStatus = SID_KILL_DOWN;
				} else {
					AddNetMsg_ChatBroadcast("Area secured!",TRUE);
					Sound_Play(SID_KILL_SECURED,"hv",127);
					playerStatusPtr->AirStatus = SID_KILL_SECURED;
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F4]) {
				if (playerStatusPtr->Class != CLASS_AA_SPEC) {
					if (rand) {
						AddNetMsg_ChatBroadcast("Affirmative!",TRUE);
						Sound_Play(SID_YES_AFFIRMATIVE,"hv",127);
						playerStatusPtr->AirStatus = SID_YES_AFFIRMATIVE;
					} else {
						AddNetMsg_ChatBroadcast("Roger!",TRUE);
						Sound_Play(SID_YES_ROGER,"hv",127);
						playerStatusPtr->AirStatus = SID_YES_ROGER;
					}
				} else {
					if (rand) {
						AddNetMsg_ChatBroadcast("I'm the ultimate badass!",FALSE);
						Sound_Play(SID_TAUNT_BADASS,"hv",127);
						playerStatusPtr->AirStatus = SID_TAUNT_BADASS;
					} else {
						AddNetMsg_ChatBroadcast("Let's rock and roll!",FALSE);
						Sound_Play(SID_TAUNT_ROCK,"hv",127);
						playerStatusPtr->AirStatus = SID_TAUNT_ROCK;
					}
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F5]) {
				if (playerStatusPtr->Class != CLASS_AA_SPEC) {
					if (rand) {
						AddNetMsg_ChatBroadcast("No way sir!",TRUE);
						Sound_Play(SID_NO_NOWAY,"hv",127);
						playerStatusPtr->AirStatus = SID_NO_NOWAY;
					} else {
						AddNetMsg_ChatBroadcast("Negative!",TRUE);
						Sound_Play(SID_NO_NEGATIVE,"hv",127);
						playerStatusPtr->AirStatus = SID_NO_NEGATIVE;
					}
				} else {
					if (rand) {
						AddNetMsg_ChatBroadcast("Engage!",TRUE);
						Sound_Play(SID_ATTACK_ENGAGE,"hv",127);
						playerStatusPtr->AirStatus = SID_ATTACK_ENGAGE;
					} else {
						AddNetMsg_ChatBroadcast("Prepare for assault!",TRUE);
						Sound_Play(SID_ATTACK_ASSAULT,"hv",127);
						playerStatusPtr->AirStatus = SID_ATTACK_ASSAULT;
					}
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F6]) {
				if (playerStatusPtr->Class != CLASS_AA_SPEC) {
					if (rand) {
						AddNetMsg_ChatBroadcast("I'm the ultimate badass!",FALSE);
						Sound_Play(SID_TAUNT_BADASS,"hv",127);
						playerStatusPtr->AirStatus = SID_TAUNT_BADASS;
					} else {
						AddNetMsg_ChatBroadcast("Let's rock and roll!",FALSE);
						Sound_Play(SID_TAUNT_ROCK,"hv",127);
						playerStatusPtr->AirStatus = SID_TAUNT_ROCK;
					}
				} else {
					if (rand) {
						AddNetMsg_ChatBroadcast("Defensive formation!",TRUE);
						Sound_Play(SID_DEFEND_FORMATION,"hv",127);
						playerStatusPtr->AirStatus = SID_DEFEND_FORMATION;
					} else {
						AddNetMsg_ChatBroadcast("Hold your positions!",TRUE);
						Sound_Play(SID_DEFEND_POSITIONS,"hv",127);
						playerStatusPtr->AirStatus = SID_DEFEND_POSITIONS;
					}
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F7]) {
				if (playerStatusPtr->Class != CLASS_AA_SPEC) {
					AddNetMsg_ChatBroadcast("I need medical attention!",TRUE);
					Sound_Play(SID_SPEC_MEDIC,"hv",127);
					playerStatusPtr->AirStatus = SID_SPEC_MEDIC;
				} else {
					if (rand) {
						AddNetMsg_ChatBroadcast("Stick together!",TRUE);
						Sound_Play(SID_CONVERGE_STICK,"hv",127);
						playerStatusPtr->AirStatus = SID_CONVERGE_STICK;
					} else {
						AddNetMsg_ChatBroadcast("Keep it tight people!",TRUE);
						Sound_Play(SID_CONVERGE_TIGHT,"hv",127);
						playerStatusPtr->AirStatus = SID_CONVERGE_TIGHT;
					}
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F8]) {
				if (playerStatusPtr->Class != CLASS_AA_SPEC) {
					AddNetMsg_ChatBroadcast("Fire in the hole!",TRUE);
					Sound_Play(SID_SPEC_FIREHOLE,"hv",127);
					playerStatusPtr->AirStatus = SID_SPEC_FIREHOLE;
				} else {
					if (rand) {
						AddNetMsg_ChatBroadcast("Disperse!",TRUE);
						Sound_Play(SID_DISPERSE_DISPERSE,"hv",127);
						playerStatusPtr->AirStatus = SID_DISPERSE_DISPERSE;
					} else {
						AddNetMsg_ChatBroadcast("Spread out!",TRUE);
						Sound_Play(SID_DISPERSE_SPREAD,"hv",127);
						playerStatusPtr->AirStatus = SID_DISPERSE_SPREAD;
					}
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F9]) {
				if (playerStatusPtr->Class == CLASS_RIFLEMAN ||
					playerStatusPtr->Class == CLASS_COM_TECH) {
					if (rand) {
						AddNetMsg_ChatBroadcast("There's something moving in here!",TRUE);
						Sound_Play(SID_TRACKER_MOVING,"hv",127);
						playerStatusPtr->AirStatus = SID_TRACKER_MOVING;
					} else {
						AddNetMsg_ChatBroadcast("I've got movement!",TRUE);
						Sound_Play(SID_TRACKER_MOVEMENT,"hv",127);
						playerStatusPtr->AirStatus = SID_TRACKER_MOVEMENT;
					}
				} else if (playerStatusPtr->Class == CLASS_AA_SPEC) {
					if (rand) {
						AddNetMsg_ChatBroadcast("Stay frosty!",TRUE);
						Sound_Play(SID_ATTENTION_FROSTY,"hv",127);
						playerStatusPtr->AirStatus = SID_ATTENTION_FROSTY;
					} else {
						AddNetMsg_ChatBroadcast("Stay sharp!",TRUE);
						Sound_Play(SID_ATTENTION_SHARP,"hv",127);
						playerStatusPtr->AirStatus = SID_ATTENTION_SHARP;
					}
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F10]) {
				if (playerStatusPtr->Class == CLASS_AA_SPEC) {
					AddNetMsg_ChatBroadcast("Use grenades.",TRUE);
					Sound_Play(SID_REQUEST_GRENADES,"hv",127);
					playerStatusPtr->AirStatus = SID_REQUEST_GRENADES;
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F11]) {
				if (playerStatusPtr->Class == CLASS_AA_SPEC) {
					AddNetMsg_ChatBroadcast("Run a bypass.",TRUE);
					Sound_Play(SID_REQUEST_BYPASS,"hv",127);
					playerStatusPtr->AirStatus = SID_REQUEST_BYPASS;
				}
				DisplayRadioMenu = 0;
			}
			if (DebouncedKeyboardInput[KEY_F12]) {
				if (playerStatusPtr->Class == CLASS_AA_SPEC) {
					AddNetMsg_ChatBroadcast("Use trackers.",TRUE);
					Sound_Play(SID_REQUEST_TRACKERS,"hv",127);
					playerStatusPtr->AirStatus = SID_REQUEST_TRACKERS;
				}
				DisplayRadioMenu = 0;
			}
			if(DebouncedKeyboardInput[KEY_ESCAPE])
				 ToggleRadioMenu();

			return;
		}
		if ((AvP.Network != I_No_Network) && (playerStatusPtr->Class == 20))
		{
			extern void ChangeToMarine();
			extern void ChangeToAlien();
			extern void ChangeToPredator();

			if (DebouncedKeyboardInput[KEY_F1])
			{
				netGameData.myCharacterSubType=NGSCT_General;
				ChangeToAlien();
				playerStatusPtr->Class = CLASS_NONE;
				playerStatusPtr->Cocoons = CLASS_NONE;
				AddNetMsg_ChatBroadcast("(AUTO) Joined the Alien team.", FALSE);

				if (netGameData.LifeCycle)
					playerStatusPtr->Class = CLASS_EXF_W_SPEC;
			}
			if (DebouncedKeyboardInput[KEY_F2])
			{
				netGameData.myCharacterSubType=NGSCT_General;
				ChangeToMarine();
				playerStatusPtr->Class = CLASS_NONE;
				playerStatusPtr->Cocoons = CLASS_NONE;
				AddNetMsg_ChatBroadcast("(AUTO) Joined the Marine team.", FALSE);
			}
			if (DebouncedKeyboardInput[KEY_F3])
			{
				netGameData.myCharacterSubType=NGSCT_General;
				ChangeToPredator();
				playerStatusPtr->Class = CLASS_NONE;
				playerStatusPtr->Cocoons = CLASS_NONE;
				AddNetMsg_ChatBroadcast("(AUTO) Joined the Predator team.", FALSE);
			}
			return;
		}
		if ((AvP.Network != I_No_Network) && ((playerStatusPtr->Class == CLASS_NONE) ||
			(DisplayClasses)))
		{
			extern void ChangeToMarine();
			extern void ChangeToAlien();
			extern void ChangeToPredator();
			extern void NewOnScreenMessage(unsigned char *messagePtr);

			if (DebouncedKeyboardInput[KEY_F1]) 
			{
				if (AvP.PlayerType == I_Marine)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_RIFLEMAN;
						ChangeToMarine();
					}
					playerStatusPtr->Cocoons = CLASS_RIFLEMAN;
					netGameData.myCharacterSubType=NGSCT_General;
					NewOnScreenMessage("You will respawn as a Rifleman.");
				}
				if (AvP.PlayerType == I_Predator)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_PRED_WARRIOR;
						ChangeToPredator();
					}
					netGameData.myCharacterSubType=NGSCT_General;
					playerStatusPtr->Cocoons = CLASS_PRED_WARRIOR;
					NewOnScreenMessage("You will respawn as a Predator Warrior.");				
				}
				if (AvP.PlayerType == I_Alien)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_ALIEN_DRONE;
						ChangeToAlien();
					}
					netGameData.myCharacterSubType=NGSCT_General;
					playerStatusPtr->Cocoons = CLASS_ALIEN_DRONE;
					NewOnScreenMessage("You will respawn as an Alien Drone.");
				}
				DisplayClasses = 0;
			}
			if (DebouncedKeyboardInput[KEY_F2]) 
			{
				if (AvP.PlayerType == I_Marine)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_SMARTGUNNER;
						ChangeToMarine();
					}
					playerStatusPtr->Cocoons = CLASS_SMARTGUNNER;
					netGameData.myCharacterSubType=NGSCT_General;
					NewOnScreenMessage("You will respawn as a Smartgunner.");
				}
				if (AvP.PlayerType == I_Predator)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_PRED_HUNTER;
						ChangeToPredator();
					}
					netGameData.myCharacterSubType=NGSCT_General;
					playerStatusPtr->Cocoons = CLASS_PRED_HUNTER;
					NewOnScreenMessage("You will respawn as a Predator Hunter.");
				}
				if (AvP.PlayerType == I_Alien)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_ALIEN_WARRIOR;
						ChangeToAlien();
					}
					netGameData.myCharacterSubType=NGSCT_General;
					playerStatusPtr->Cocoons = CLASS_ALIEN_WARRIOR;
					NewOnScreenMessage("You will respawn as an Alien Warrior.");
				}
				DisplayClasses = 0;
			}
			if (DebouncedKeyboardInput[KEY_F3]) 
			{
				if (AvP.PlayerType == I_Marine)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_INC_SPEC;
						ChangeToMarine();
					}
					playerStatusPtr->Cocoons = CLASS_INC_SPEC;
					netGameData.myCharacterSubType=NGSCT_General;
					NewOnScreenMessage("You will respawn as an Incinerator Specialist.");
				}
				if (AvP.PlayerType == I_Predator)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_RENEGADE;
						ChangeToPredator();
					}
					netGameData.myCharacterSubType=NGSCT_General;
					playerStatusPtr->Cocoons = CLASS_RENEGADE;
					NewOnScreenMessage("You will respawn as a Predator Bad Blood.");
				}
				if (AvP.PlayerType == I_Alien)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_EXF_SNIPER;
						ChangeToAlien();
					}
					netGameData.myCharacterSubType=NGSCT_Frisbee;
					playerStatusPtr->Cocoons = CLASS_EXF_SNIPER;
					NewOnScreenMessage("You will respawn as a Predalien.");
				}
				DisplayClasses = 0;
			}
			if (DebouncedKeyboardInput[KEY_F4]) 
			{
				if (AvP.PlayerType == I_Marine)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_ENGINEER;
						ChangeToMarine();
					}
					playerStatusPtr->Cocoons = CLASS_ENGINEER;
					netGameData.myCharacterSubType=NGSCT_General;
					NewOnScreenMessage("You will respawn as an Engineer.");
				}
				if (AvP.PlayerType == I_Predator)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_ELDER;
						ChangeToPredator();
					}
					netGameData.myCharacterSubType=NGSCT_General;
					playerStatusPtr->Cocoons = CLASS_ELDER;
					NewOnScreenMessage("You will respawn as a Predator Elder.");
				}
				/*if (AvP.PlayerType == I_Alien)
				{
					netGameData.myCharacterSubType=NGSCT_Minigun;
					playerStatusPtr->Class = CLASS_EXF_W_SPEC;
					ChangeToAlien();
					AddNetMsg_ChatBroadcast("(AUTO) Changed class to Facehugger.", TRUE);
				}*/
				DisplayClasses = 0;
			}
			if (DebouncedKeyboardInput[KEY_F5]) 
			{
				if (AvP.PlayerType == I_Marine)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_COM_TECH;
						ChangeToMarine();
					}
					playerStatusPtr->Cocoons = CLASS_COM_TECH;
					netGameData.myCharacterSubType=NGSCT_General;
					NewOnScreenMessage("You will respawn as a Com-Tech.");
				}
				if (AvP.PlayerType == I_Predator)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_TANK_ALIEN;
						ChangeToPredator();
					}
					playerStatusPtr->Cocoons = CLASS_TANK_ALIEN;
					NewOnScreenMessage("You will respawn as a Female Hunter.");
				}
				DisplayClasses = 0;
			}
			if (DebouncedKeyboardInput[KEY_F6]) 
			{
				if (AvP.PlayerType == I_Marine)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_MEDIC_FT;
						ChangeToMarine();
					}
					playerStatusPtr->Cocoons = CLASS_MEDIC_FT;
					netGameData.myCharacterSubType=NGSCT_General;
					NewOnScreenMessage("You will respawn as a Medic.");
				}
				DisplayClasses = 0;
			}
			if (DebouncedKeyboardInput[KEY_F7]) 
			{
				if (AvP.PlayerType == I_Marine)
				{
					if (playerStatusPtr->Class == CLASS_NONE)
					{
						playerStatusPtr->Class = CLASS_AA_SPEC;
						ChangeToMarine();
					}
					playerStatusPtr->Cocoons = CLASS_AA_SPEC;
					netGameData.myCharacterSubType=NGSCT_General;
					NewOnScreenMessage("You will respawn as an Officer.");
				}
				DisplayClasses = 0;
			}
		}
		/* now do forward,backward,left,right,up and down 
		   IMPORTANT:  The request flag and the movement 
		   increment must BOTH be set!
		*/
		if ((AvP.Network==I_No_Network) || ((AvP.Network != I_No_Network) &&
			(playerStatusPtr->Class != CLASS_NONE) && (playerStatusPtr->Class != 20)))
		{
		if(KeyboardInput[primaryInput->Forward]
		 ||KeyboardInput[secondaryInput->Forward])
		if (!playerStatusPtr->Immobilized) {
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward = 1;
			playerStatusPtr->Mvt_MotionIncrement = ONE_FIXED;
			HackPool = 0;
			WeldPool = 0;
		}	
		if(KeyboardInput[primaryInput->Backward]
		 ||KeyboardInput[secondaryInput->Backward])
		if (!playerStatusPtr->Immobilized) {
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward = 1;
			playerStatusPtr->Mvt_MotionIncrement = -ONE_FIXED;
			HackPool = 0;
			WeldPool = 0;
		}

		if (!playerStatusPtr->Honor)
		{

		if(KeyboardInput[primaryInput->Left]
		 ||KeyboardInput[secondaryInput->Left])
		if (!playerStatusPtr->Immobilized) {
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
			playerStatusPtr->Mvt_TurnIncrement = -ONE_FIXED;
			HackPool = 0;
			WeldPool = 0;
		}
		if(KeyboardInput[primaryInput->Right]
		 ||KeyboardInput[secondaryInput->Right])
		if (!playerStatusPtr->Immobilized) {
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
			playerStatusPtr->Mvt_TurnIncrement = ONE_FIXED;
			HackPool = 0;
			WeldPool = 0;
		}

		}//APC

		if(KeyboardInput[primaryInput->StrafeLeft]
		 ||KeyboardInput[secondaryInput->StrafeLeft])
		{
			if (playerStatusPtr->Honor) {
				if (CurrentSpeed) {
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
					playerStatusPtr->Mvt_TurnIncrement = -ONE_FIXED;
				}
			} else {
				if (!playerStatusPtr->Immobilized) {
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft = 1;
					playerStatusPtr->Mvt_SideStepIncrement = -ONE_FIXED;
					HackPool = 0;
					WeldPool = 0;
				}
			}
		}
		if(KeyboardInput[primaryInput->StrafeRight]
		 ||KeyboardInput[secondaryInput->StrafeRight])
		{
			if (playerStatusPtr->Honor) {
				if (CurrentSpeed) {
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
					playerStatusPtr->Mvt_TurnIncrement = ONE_FIXED;
				}
			} else {
				if (!playerStatusPtr->Immobilized) {
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight = 1;
					playerStatusPtr->Mvt_SideStepIncrement = ONE_FIXED;
					HackPool = 0;
					WeldPool = 0;
				}
			}
		}
		if (!playerStatusPtr->Honor)
		{
			extern int RunMode;
			if (!RunMode) {
				if(DebouncedKeyboardInput[primaryInput->Walk]
				||DebouncedKeyboardInput[secondaryInput->Walk])
				{
					if (!RunMode) // Toggle
					{
						if (Run) {
							Run = 0;
						} else {
							Run = 1;
						}
					} else {	// P&H
						playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Faster=1;
					}
				}
			} else {
				if (KeyboardInput[primaryInput->Walk]
				||KeyboardInput[secondaryInput->Walk])
				{
					if (!RunMode) // Toggle
					{
						if (Run) {
							Run = 0;
						} else {
							Run = 1;
						}
					} else {	// P&H
						playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Faster=1;
					}
				}
			}

		if(KeyboardInput[primaryInput->Strafe]
		 ||KeyboardInput[secondaryInput->Strafe])
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe = 1;

		if(KeyboardInput[primaryInput->Crouch]
		 ||KeyboardInput[secondaryInput->Crouch])
		 if (!playerStatusPtr->Destr && !playerStatusPtr->Immobilized)
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Crouch = 1;

		}//APC

		if (OnLadder || Underwater || ZeroG)
		{
			if(KeyboardInput[primaryInput->Jump]
			 ||KeyboardInput[secondaryInput->Jump])
				if (!playerStatusPtr->Immobilized) {
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jump = 1;
				}
		} else {
			if (DebouncedKeyboardInput[primaryInput->Jump]
				||DebouncedKeyboardInput[secondaryInput->Jump])
				if (!playerStatusPtr->Immobilized) {
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jump = 1;
				}
		}

		 if (AvP.PlayerType != I_Alien)
		 {
			if(DebouncedKeyboardInput[primaryInput->Operate]
			 ||DebouncedKeyboardInput[secondaryInput->Operate])
			{
				if (!playerStatusPtr->Honor)
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Operate = 1;
			} 
		 } else {
			if(KeyboardInput[primaryInput->Operate]
			||KeyboardInput[secondaryInput->Operate])
			{
				if (playerStatusPtr->Class != CLASS_EXF_W_SPEC)
				{
					Grab();
				}
			} else {
				GrabAttackInProgress=0;
			}
		 }
		} // Multiplayer class check

		/* check for character specific abilities */
		if (playerStatusPtr->IsAlive)
		switch (AvP.PlayerType)
		{
			case I_Marine:
			{
				if ((AvP.Network==I_No_Network) || ((AvP.Network != I_No_Network) &&
					(playerStatusPtr->Class != CLASS_NONE) && (playerStatusPtr->Class != 20)))
				{
					if (!playerStatusPtr->Honor)
					{

				if(DebouncedKeyboardInput[primaryInput->ImageIntensifier]
					||DebouncedKeyboardInput[secondaryInput->ImageIntensifier])
				{
					extern void UseMotionTracker(void);
					//playerStatusPtr->Mvt_InputRequests.Flags.Rqst_ChangeVision = 1;
					UseMotionTracker();
				}

				if(DebouncedKeyboardInput[primaryInput->ThrowFlare]
				 ||DebouncedKeyboardInput[secondaryInput->ThrowFlare])
					ThrowAFlare();

				if (DebouncedKeyboardInput[primaryInput->Marine_Grenade]
				||DebouncedKeyboardInput[secondaryInput->Marine_Grenade])
					OverLoadDrill(0);

				if (DebouncedKeyboardInput[primaryInput->Marine_Medikit]
				||DebouncedKeyboardInput[secondaryInput->Marine_Medikit])
					UseMedikit();

				if (DebouncedKeyboardInput[primaryInput->Marine_SentryGun]
				||DebouncedKeyboardInput[secondaryInput->Marine_SentryGun])
					DeploySentry();

					}

				if(DebouncedKeyboardInput[primaryInput->Jetpack]
				 ||DebouncedKeyboardInput[secondaryInput->Jetpack])
				{
					if (!playerStatusPtr->Honor)
						ToggleShoulderLamp();
				}
				if(KeyboardInput[primaryInput->MarineTaunt]
				 ||KeyboardInput[secondaryInput->MarineTaunt])
				 if (AvP.Network == I_No_Network)
					StartPlayerTaunt();
				 else
					 ToggleRadioMenu();
				
				if(DebouncedKeyboardInput[primaryInput->Marine_MessageHistory]
				 ||DebouncedKeyboardInput[secondaryInput->Marine_MessageHistory])
					MessageHistory_DisplayPrevious();

				} // Multiplayer class check

				if(DebouncedKeyboardInput[primaryInput->Marine_ChangeSpecies]
				 ||DebouncedKeyboardInput[secondaryInput->Marine_ChangeSpecies])
					ChangeSpecies();

				if(DebouncedKeyboardInput[primaryInput->Marine_ChangeClass]
				 ||DebouncedKeyboardInput[secondaryInput->Marine_ChangeClass])
					ChangeClass();
					

				break;
			}
			case I_Predator:
			{
				extern int CameraZoomLevel;
				
				if ((AvP.Network==I_No_Network) || ((AvP.Network != I_No_Network) &&
					(playerStatusPtr->Class != CLASS_NONE) && (playerStatusPtr->Class != 20)))
				{
				if(KeyboardInput[primaryInput->Cloak]
				 ||KeyboardInput[secondaryInput->Cloak])
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_ChangeVision = 1;
				
				if(DebouncedKeyboardInput[primaryInput->CycleVisionMode]
				 ||DebouncedKeyboardInput[secondaryInput->CycleVisionMode])
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_CycleVisionMode = 1;

				#if !(PREDATOR_DEMO||DEATHMATCH_DEMO)
				if(DebouncedKeyboardInput[primaryInput->GrapplingHook]
				 ||DebouncedKeyboardInput[secondaryInput->GrapplingHook])
				{
					extern void PCSelfDestruct(PLAYER_WEAPON_DATA *weaponPtr);
					//playerStatusPtr->Mvt_InputRequests.Flags.Rqst_GrapplingHook = 0;
					PCSelfDestruct(0);
				}
				#endif

				if(DebouncedKeyboardInput[primaryInput->ZoomIn]
				 ||DebouncedKeyboardInput[secondaryInput->ZoomIn])
				{
					if (CameraZoomLevel<3) CameraZoomLevel++;
				}
				if(DebouncedKeyboardInput[primaryInput->ZoomOut]
				 ||DebouncedKeyboardInput[secondaryInput->ZoomOut])
				{
					if (CameraZoomLevel>0) CameraZoomLevel--;
				}
				
				MaintainZoomingLevel();
				
				if(KeyboardInput[primaryInput->PredatorTaunt]
				 ||KeyboardInput[secondaryInput->PredatorTaunt])
					StartPlayerTaunt();

				if(KeyboardInput[primaryInput->RecallDisc]
				 ||KeyboardInput[secondaryInput->RecallDisc])
					Recall_Disc();
					
				if(DebouncedKeyboardInput[primaryInput->Predator_MessageHistory]
				 ||DebouncedKeyboardInput[secondaryInput->Predator_MessageHistory])
					MessageHistory_DisplayPrevious();
					
				} // Multiplayer class check

				if(DebouncedKeyboardInput[primaryInput->Predator_ChangeSpecies]
				 ||DebouncedKeyboardInput[secondaryInput->Predator_ChangeSpecies])
					ChangeSpecies();

				if(DebouncedKeyboardInput[primaryInput->Predator_ChangeClass]
				 ||DebouncedKeyboardInput[secondaryInput->Predator_ChangeClass])
					ChangeClass();

				break;
			}

			case I_Alien:
			{
				if ((AvP.Network==I_No_Network) || ((AvP.Network != I_No_Network) &&
					(playerStatusPtr->Class != CLASS_NONE) && (playerStatusPtr->Class != 20)))
				{
				if(KeyboardInput[primaryInput->AlternateVision]
				 ||KeyboardInput[secondaryInput->AlternateVision])
					//playerStatusPtr->Mvt_InputRequests.Flags.Rqst_ChangeVision = 1;
					Ram();

				if(KeyboardInput[primaryInput->Taunt]
				||KeyboardInput[secondaryInput->Taunt]) {
				    if (playerStatusPtr->Class != CLASS_EXF_W_SPEC)
						StartPlayerTaunt();
				}
	
				if(DebouncedKeyboardInput[primaryInput->Alien_MessageHistory]
				 ||DebouncedKeyboardInput[secondaryInput->Alien_MessageHistory])
					MessageHistory_DisplayPrevious();
					
				} // Multiplayer class check

				if(DebouncedKeyboardInput[primaryInput->Alien_ChangeSpecies]
				 ||DebouncedKeyboardInput[secondaryInput->Alien_ChangeSpecies])
					ChangeSpecies();

				if(DebouncedKeyboardInput[primaryInput->Alien_ChangeClass]
				 ||DebouncedKeyboardInput[secondaryInput->Alien_ChangeClass])
					ChangeClass();

				break;
			}
		}
		/* Moved these here, wanted them to be available even for the dead :) */
		if (AvP.PlayerType == I_Marine)
		{
			if(DebouncedKeyboardInput[primaryInput->Marine_Say]
			||DebouncedKeyboardInput[secondaryInput->Marine_Say])
				BringDownConsoleWithSayTypedIn();

			if(DebouncedKeyboardInput[primaryInput->Marine_SpeciesSay]
			||DebouncedKeyboardInput[secondaryInput->Marine_SpeciesSay])
				BringDownConsoleWithSaySpeciesTypedIn();

			if(KeyboardInput[primaryInput->Marine_ShowScores]
			||KeyboardInput[secondaryInput->Marine_ShowScores])
				ShowMultiplayerScores();
		}
		if (AvP.PlayerType == I_Predator)
		{
			if(DebouncedKeyboardInput[primaryInput->Predator_Say]
			||DebouncedKeyboardInput[secondaryInput->Predator_Say])
				BringDownConsoleWithSayTypedIn();

			if(DebouncedKeyboardInput[primaryInput->Predator_SpeciesSay]
			||DebouncedKeyboardInput[secondaryInput->Predator_SpeciesSay])
				BringDownConsoleWithSaySpeciesTypedIn();

			if(KeyboardInput[primaryInput->Predator_ShowScores]
			||KeyboardInput[secondaryInput->Predator_ShowScores])
				ShowMultiplayerScores();
		}
		if (AvP.PlayerType == I_Alien)
		{
			if(DebouncedKeyboardInput[primaryInput->Alien_Say]
			||DebouncedKeyboardInput[secondaryInput->Alien_Say])
				BringDownConsoleWithSayTypedIn();

			if(DebouncedKeyboardInput[primaryInput->Alien_SpeciesSay]
			||DebouncedKeyboardInput[secondaryInput->Alien_SpeciesSay])
				BringDownConsoleWithSaySpeciesTypedIn();

			if(KeyboardInput[primaryInput->Alien_ShowScores]
			||KeyboardInput[secondaryInput->Alien_ShowScores])
				ShowMultiplayerScores();
		}
		if(DebouncedKeyboardInput[FixedInputConfig.PauseGame])
			AvP_TriggerInGameMenus();
	//		playerStatusPtr->Mvt_InputRequests.Flags.Rqst_QuitGame = 1;
//			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_PauseGame = 1;

		if(!PaintBallMode.IsOn)
		{
			if ((AvP.Network==I_No_Network) || ((AvP.Network != I_No_Network) &&
				(playerStatusPtr->Class != CLASS_NONE) && (playerStatusPtr->Class != 20)))
			{
				if (playerStatusPtr->Honor) {
					if(DebouncedKeyboardInput[primaryInput->FirePrimaryWeapon]
					||DebouncedKeyboardInput[secondaryInput->FirePrimaryWeapon])
					FireAPCGun();
				} else {
					if(KeyboardInput[primaryInput->FirePrimaryWeapon]
					||KeyboardInput[secondaryInput->FirePrimaryWeapon])
					if (!GrabAttackInProgress) {
						if (playerStatusPtr->Class == CLASS_EXF_W_SPEC) {
							Ram();
						} else {
							playerStatusPtr->Mvt_InputRequests.Flags.Rqst_FirePrimaryWeapon = 1;	
						}
					}
				}

			if (!playerStatusPtr->Honor)
			{

			if(KeyboardInput[primaryInput->LookUp]
			 ||KeyboardInput[secondaryInput->LookUp])
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp = 1;
				playerStatusPtr->Mvt_PitchIncrement = -ONE_FIXED;
			}
			else if(KeyboardInput[primaryInput->LookDown]
			 ||KeyboardInput[secondaryInput->LookDown])
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown = 1;
				playerStatusPtr->Mvt_PitchIncrement = ONE_FIXED;
			}
  			
			if(KeyboardInput[primaryInput->CentreView]
			 ||KeyboardInput[secondaryInput->CentreView])
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_CentreView = 1;

			if(KeyboardInput[primaryInput->NextWeapon]
			 ||KeyboardInput[secondaryInput->NextWeapon])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_NextWeapon = 1;
			}
			if(KeyboardInput[primaryInput->PreviousWeapon]
			 ||KeyboardInput[secondaryInput->PreviousWeapon])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_PreviousWeapon = 1;
			}
			if(DebouncedKeyboardInput[primaryInput->FlashbackWeapon]
			 ||DebouncedKeyboardInput[secondaryInput->FlashbackWeapon])
				Reload_Weapon();
			
			if(KeyboardInput[primaryInput->FireSecondaryWeapon]
			 ||KeyboardInput[secondaryInput->FireSecondaryWeapon])
			 if (!GrabAttackInProgress) {
				if (playerStatusPtr->Class == CLASS_EXF_W_SPEC) {
					Ram();
				} else {
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_FireSecondaryWeapon = 1;
				}
			 }
			
			/* fixed controls */
			if(KeyboardInput[FixedInputConfig.Weapon1])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = 1;
			}
		  	if(KeyboardInput[FixedInputConfig.Weapon2])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = 2;
			}
			if(KeyboardInput[FixedInputConfig.Weapon3])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = 3;
			}
			if(KeyboardInput[FixedInputConfig.Weapon4])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = 4;
			}
			if(KeyboardInput[FixedInputConfig.Weapon5])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = 5;
			}
			if(KeyboardInput[FixedInputConfig.Weapon6])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = 6;
			}
			if(KeyboardInput[FixedInputConfig.Weapon7])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = 7;
			}
			if(KeyboardInput[FixedInputConfig.Weapon8])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = 8;
			}
			if(KeyboardInput[FixedInputConfig.Weapon9])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = 9;
			}
			if(KeyboardInput[FixedInputConfig.Weapon10])
			{
				HackPool = 0;
				WeldPool = 0;
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = 10;
			}

			}// APC check

			}// Multiplayer class check
		}
		#if !(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO||DEATHMATCH_DEMO)
		else // Cool - paintball mode				`
		{
			if(DebouncedKeyboardInput[primaryInput->NextWeapon]
			 ||DebouncedKeyboardInput[secondaryInput->NextWeapon])
			{
				PaintBallMode_ChangeSelectedDecalID(+1);
			}
			
			if(DebouncedKeyboardInput[primaryInput->PreviousWeapon]
			 ||DebouncedKeyboardInput[secondaryInput->PreviousWeapon])
			{
				PaintBallMode_ChangeSelectedDecalID(-1);
			}
				
			if(KeyboardInput[primaryInput->LookUp]
			 ||KeyboardInput[secondaryInput->LookUp])
			{
				PaintBallMode_ChangeSize(+1);
			}
			
			if(KeyboardInput[primaryInput->LookDown]
			 ||KeyboardInput[secondaryInput->LookDown])
			{
				PaintBallMode_ChangeSize(-1);
			}
			
			if(KeyboardInput[primaryInput->CentreView]
			 ||KeyboardInput[secondaryInput->CentreView])
			{
				PaintBallMode_Rotate();				
			}
						  
			if(DebouncedKeyboardInput[FixedInputConfig.Weapon1])
			{
				PaintBallMode_ChangeSubclass(+1);
			}
			
		  	if(DebouncedKeyboardInput[FixedInputConfig.Weapon2])
			{
				PaintBallMode_ChangeSubclass(-1);
			}

		  	if(DebouncedKeyboardInput[FixedInputConfig.Weapon3])
			{
				PaintBallMode.DecalIsInverted = ~PaintBallMode.DecalIsInverted;
			}

		  	if(DebouncedKeyboardInput[FixedInputConfig.Weapon4])
			{
				PaintBallMode_Randomise();
			}

		  	if(DebouncedKeyboardInput[FixedInputConfig.Weapon10])
			{
				extern void save_preplaced_decals();
				save_preplaced_decals();
			}
	

			if(DebouncedKeyboardInput[primaryInput->FirePrimaryWeapon]
			 ||DebouncedKeyboardInput[secondaryInput->FirePrimaryWeapon])
			{
				PaintBallMode_AddDecal();
			}
			if(DebouncedKeyboardInput[primaryInput->FireSecondaryWeapon]
			 ||DebouncedKeyboardInput[secondaryInput->FireSecondaryWeapon])
			{
				PaintBallMode_RemoveDecal();
			}
			
		}
		#endif
	}
	/* end of block conditional on input focus */


	/* KJL 10:16:49 04/29/97 - mouse control */



	if ((AvP.Network==I_No_Network) || ((AvP.Network != I_No_Network) &&
		(playerStatusPtr->Class != CLASS_NONE) && (playerStatusPtr->Class != 20)))
	{
	if (GotMouse && !playerStatusPtr->Honor)
	{
		extern int MouseVelX;
		extern int MouseVelY;


		if(ControlMethods.HAxisIsTurning)
		{
			if(MouseVelX<0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				playerStatusPtr->Mvt_TurnIncrement = ((int)MouseVelX)*ControlMethods.MouseXSensitivity;
			   
			}
			else if(MouseVelX>0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				playerStatusPtr->Mvt_TurnIncrement = ((int)MouseVelX)*ControlMethods.MouseXSensitivity;
			}

			/* KJL 17:36:37 9/9/97 - cap values if strafing */
		   	if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe)
			{
		   		if(playerStatusPtr->Mvt_TurnIncrement < -ONE_FIXED)
		   			playerStatusPtr->Mvt_TurnIncrement = -ONE_FIXED;
		   		if(playerStatusPtr->Mvt_TurnIncrement > ONE_FIXED)
		   			playerStatusPtr->Mvt_TurnIncrement = ONE_FIXED;
			}
			
		}
		else // it's sidestep
		{
			if(MouseVelX<0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft = 1;
				playerStatusPtr->Mvt_SideStepIncrement = ((int)MouseVelX)*ControlMethods.MouseXSensitivity;
			}
			else if(MouseVelX>0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight = 1;
				playerStatusPtr->Mvt_SideStepIncrement = ((int)MouseVelX)*ControlMethods.MouseXSensitivity;
			}
	   		
	   		if(playerStatusPtr->Mvt_SideStepIncrement < -ONE_FIXED)
	   			playerStatusPtr->Mvt_SideStepIncrement = -ONE_FIXED;
	   		if(playerStatusPtr->Mvt_SideStepIncrement > ONE_FIXED)
	   			playerStatusPtr->Mvt_SideStepIncrement = ONE_FIXED;
			

		}

		if(ControlMethods.VAxisIsMovement)
		{
			
			if(MouseVelY<0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward = 1;
			 	playerStatusPtr->Mvt_MotionIncrement = -((int)MouseVelY)*ControlMethods.MouseYSensitivity;
			}
			else if(MouseVelY>0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward = 1;
			 	playerStatusPtr->Mvt_MotionIncrement = -((int)MouseVelY)*ControlMethods.MouseYSensitivity;
			}
	   	
	   		if(playerStatusPtr->Mvt_MotionIncrement < -ONE_FIXED)
	   			playerStatusPtr->Mvt_MotionIncrement = -ONE_FIXED;
	   		if(playerStatusPtr->Mvt_MotionIncrement > ONE_FIXED)
	   			playerStatusPtr->Mvt_MotionIncrement = ONE_FIXED;
		}
		else // it's looking
		{
			int newMouseVelY;

			if (ControlMethods.FlipVerticalAxis) newMouseVelY = -MouseVelY;
			else newMouseVelY = MouseVelY;

			if(newMouseVelY<0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				playerStatusPtr->Mvt_PitchIncrement = ((int)newMouseVelY)*ControlMethods.MouseYSensitivity;
			}
			else if(newMouseVelY>0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				playerStatusPtr->Mvt_PitchIncrement = ((int)newMouseVelY)*ControlMethods.MouseYSensitivity;
			}
		}	
								 
	}
	
	/* KJL 18:27:34 04/29/97 - joystick control */
	if (GotJoystick)
	{
		#define JOYSTICK_DEAD_ZONE 12000
		extern int GotJoystick;
		extern JOYINFOEX JoystickData;
		extern JOYCAPS JoystickCaps;
		
		
		int yAxis = (32768-JoystickData.dwYpos)*2;
		int xAxis = (JoystickData.dwXpos-32768)*2;
		
		if(JoystickControlMethods.JoystickVAxisIsMovement)
		{
			if(JoystickControlMethods.JoystickFlipVerticalAxis) yAxis=-yAxis;

			if(yAxis>JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward = 1;
				playerStatusPtr->Mvt_MotionIncrement = yAxis;
			}	
			else if(yAxis<-JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward = 1;
				playerStatusPtr->Mvt_MotionIncrement = yAxis;
			}
		}
		else // looking up/down
		{
			if(!JoystickControlMethods.JoystickFlipVerticalAxis) yAxis=-yAxis;

			if(yAxis>JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				playerStatusPtr->Mvt_PitchIncrement = yAxis;
			}
			else if(yAxis<-JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				playerStatusPtr->Mvt_PitchIncrement = yAxis;
			}
		}

		if (JoystickControlMethods.JoystickHAxisIsTurning)
		{
			if(xAxis<-JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				playerStatusPtr->Mvt_TurnIncrement = xAxis;
			}
			else if(xAxis>JOYSTICK_DEAD_ZONE)
			{			  
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				playerStatusPtr->Mvt_TurnIncrement = xAxis;
			}
		}
		else // strafing
		{
			if(xAxis<-JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft = 1;
				playerStatusPtr->Mvt_SideStepIncrement = xAxis;
			}
			else if(xAxis>JOYSTICK_DEAD_ZONE)
			{			  
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight = 1;
				playerStatusPtr->Mvt_SideStepIncrement = xAxis;
			}
		}
		
		/* check for rudder */
		if ((JoystickCaps.wCaps & JOYCAPS_HASR) && JoystickControlMethods.JoystickRudderEnabled)
		{
			int rAxis = (JoystickData.dwRpos-32768)*2;
			if (JoystickControlMethods.JoystickRudderAxisIsTurning)
			{
				if(rAxis>JOYSTICK_DEAD_ZONE)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = rAxis;
				}
				else if(rAxis<-JOYSTICK_DEAD_ZONE)
				{			  
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = rAxis;
				}
			}
			else
			{
				if(rAxis>JOYSTICK_DEAD_ZONE)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe = 1;
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = rAxis;
				}	
				else if(rAxis<-JOYSTICK_DEAD_ZONE)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe = 1;
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = rAxis;
				}	
			}
		}

		/* check joystick buttons */
		#if 0 
		if(JoystickData.dwButtons & JOY_BUTTON1)
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_FirePrimaryWeapon = 1;
		else if(JoystickData.dwButtons & JOY_BUTTON2)
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_FireSecondaryWeapon = 1;
		else if(JoystickData.dwButtons & JOY_BUTTON3)
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_NextWeapon = 1;
		else if(JoystickData.dwButtons & JOY_BUTTON4)
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_PreviousWeapon = 1;
		#endif

		/* Point Of View Hat */
		if (JoystickData.dwPOV<36000)
		{
			int theta = ((JoystickData.dwPOV * 4096) /36000);
			int verticalAxis = GetCos(theta);
			int horizontalAxis = GetSin(theta);

			if (JoystickControlMethods.JoystickPOVFlipVerticalAxis)
			{
				verticalAxis = -verticalAxis;
			}

			if (JoystickControlMethods.JoystickPOVVAxisIsMovement)
			{
				if(verticalAxis>0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward = 1;
					playerStatusPtr->Mvt_MotionIncrement = verticalAxis;
				}								  
				else if(verticalAxis<0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward = 1;
					playerStatusPtr->Mvt_MotionIncrement = verticalAxis;
				}
			}
			else
			{
				if(verticalAxis>0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp = 1;
					playerStatusPtr->Mvt_PitchIncrement -= verticalAxis;
				}								  
				else if(verticalAxis<0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown = 1;
					playerStatusPtr->Mvt_PitchIncrement -= verticalAxis;
				}
			}
			if (JoystickControlMethods.JoystickPOVHAxisIsTurning)
			{
				if(horizontalAxis>0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = horizontalAxis;
				}
				else if(horizontalAxis<0)
				{			  
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = horizontalAxis;
				}
			}
			else // strafing
			{
				if(horizontalAxis>0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight = 1;
					playerStatusPtr->Mvt_SideStepIncrement = horizontalAxis;
				}
				else if(horizontalAxis<0)
				{			  
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft = 1;
					playerStatusPtr->Mvt_SideStepIncrement = horizontalAxis;
				}
			}
		}
		if (JoystickControlMethods.JoystickTrackerBallEnabled)
		{
			int trackerballH = JoystickData.dwUpos - 32768;
			int trackerballV = JoystickData.dwVpos - 32768;

			if (JoystickControlMethods.JoystickTrackerBallFlipVerticalAxis)
			{
				trackerballV = -trackerballV;
			}

			if(trackerballH<0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				playerStatusPtr->Mvt_TurnIncrement = trackerballH*JoystickControlMethods.JoystickTrackerBallHorizontalSensitivity;
			   
			}
			else if(trackerballH>0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				playerStatusPtr->Mvt_TurnIncrement = trackerballH*JoystickControlMethods.JoystickTrackerBallHorizontalSensitivity;

			}
			if(trackerballV<0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				playerStatusPtr->Mvt_PitchIncrement = trackerballV*JoystickControlMethods.JoystickTrackerBallVerticalSensitivity;
			}
			else if(trackerballV>0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				playerStatusPtr->Mvt_PitchIncrement = trackerballV*JoystickControlMethods.JoystickTrackerBallVerticalSensitivity;;
			}

		}
					   
		#if 0
		textprint("%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
			JoystickData.dwXpos,
			JoystickData.dwYpos,
			JoystickData.dwZpos,
			JoystickData.dwRpos,
			JoystickData.dwUpos,
			JoystickData.dwVpos,
			JoystickData.dwButtons,
			JoystickData.dwPOV);
		#endif
	}
	} // Multiplayer class check

	/* KJL 16:03:06 05/11/97 - Handle map options */
	#if 0
	if(KeyboardInput[KEY_NUMPADADD])
		HUDMapOn();
	else if(KeyboardInput[KEY_NUMPADSUB])
		HUDMapOff();
	if(KeyboardInput[KEY_NUMPAD7])
		HUDMapZoomIn();
	else if(KeyboardInput[KEY_NUMPAD9])
		HUDMapZoomOut();
	if(KeyboardInput[KEY_NUMPAD1])
		HUDMapSmaller();
	else if(KeyboardInput[KEY_NUMPAD3])
		HUDMapLarger();
	if(KeyboardInput[KEY_NUMPAD4])
		HUDMapLeft();
	else if(KeyboardInput[KEY_NUMPAD6])
		HUDMapRight();
	if(KeyboardInput[KEY_NUMPAD8])
		HUDMapUp();
	else if(KeyboardInput[KEY_NUMPAD2])
		HUDMapDown();
	#endif
		
	/* KJL 10:55:22 10/9/97 - HUD transparency */
	#if 0
	{
		extern signed int HUDTranslucencyLevel;

		if (KeyboardInput[KEY_F1])
		{
			HUDTranslucencyLevel-=NormalFrameTime>>9;
			if (HUDTranslucencyLevel<0) HUDTranslucencyLevel=0;
		}
		else if (KeyboardInput[KEY_F2])
		{
			HUDTranslucencyLevel+=NormalFrameTime>>9;
			if (HUDTranslucencyLevel>255) HUDTranslucencyLevel=255;
		}
	}
	#endif
	/* KJL 10:55:32 10/9/97 - screen size */
	#if 0
	if(KeyboardInput[KEY_F3])
		MakeViewingWindowLarger();
	else if(KeyboardInput[KEY_F4])
		MakeViewingWindowSmaller();
	#endif
	#if 0
	if (DebouncedKeyboardInput[KEY_F3])
	{
		MessageHistory_DisplayPrevious();
	}
	#endif
	if (DebouncedKeyboardInput[KEY_GRAVE]) IOFOCUS_Toggle();
}

void LoadKeyConfiguration(void)
{
	#if ALIEN_DEMO
	LoadAKeyConfiguration("alienavpkey.cfg");
	#else
	LoadAKeyConfiguration("avpkey.cfg");
	#endif

}

void SaveKeyConfiguration(void)
{
	#if ALIEN_DEMO
	SaveAKeyConfiguration("alienavpkey.cfg");
	#else
	SaveAKeyConfiguration("avpkey.cfg");
	#endif
}

void LoadAKeyConfiguration(char* Filename)
{
	#if 0
	FILE* file=fopen(Filename,"rb");
	if(!file)
	{
		MarineInputPrimaryConfig = DefaultMarineInputPrimaryConfig;
		MarineInputSecondaryConfig = DefaultMarineInputSecondaryConfig;
		PredatorInputPrimaryConfig = DefaultPredatorInputPrimaryConfig;
		PredatorInputSecondaryConfig = DefaultPredatorInputSecondaryConfig;
		AlienInputPrimaryConfig = DefaultAlienInputPrimaryConfig;
		AlienInputSecondaryConfig = DefaultAlienInputSecondaryConfig;
		return;
	}
	fread(&MarineInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&MarineInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&PredatorInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&PredatorInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&AlienInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&AlienInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);

	fread(&ControlMethods,sizeof(CONTROL_METHODS),1,file);
	fread(&JoystickControlMethods,sizeof(JOYSTICK_CONTROL_METHODS),1,file);
	
	fclose(file);
	#endif
}

void SaveAKeyConfiguration(char* Filename)
{
	#if 0
	FILE* file=fopen(Filename,"wb");
	if(!file) return;

	fwrite(&MarineInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&MarineInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&PredatorInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&PredatorInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&AlienInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&AlienInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);

	fwrite(&ControlMethods,sizeof(CONTROL_METHODS),1,file);
	fwrite(&JoystickControlMethods,sizeof(JOYSTICK_CONTROL_METHODS),1,file);

	fclose(file);
	#endif
}

void SaveDefaultPrimaryConfigs(void)
{
	FILE* file=fopen("cb_default.cfg","wb");
	if(!file) return;

	fwrite(&DefaultMarineInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&DefaultPredatorInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&DefaultAlienInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);

	fclose(file);
}
void LoadDefaultPrimaryConfigs(void)
{
	FILE* file=fopen("cb_default.cfg","rb");
	if(!file) return;

	fread(&DefaultMarineInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&DefaultPredatorInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&DefaultAlienInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);

	fclose(file);
}











