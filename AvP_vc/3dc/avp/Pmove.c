/*-------------- Patrick 15/10/96 ------------------
	Source file for Player Movement ...
----------------------------------------------------*/
#include "3dc.h"
#include "inline.h"
#include "module.h"

#include "gamedef.h"
#include "stratdef.h"
#include "dynblock.h"
#include "dynamics.h"
#include "gameplat.h"

#include "bh_types.h"

#define UseLocalAssert 1
#include "ourasert.h"
#include "comp_shp.h"

#include "pmove.h"
#include "usr_io.h"
#include "bh_far.h"
#include "triggers.h"
#include "pvisible.h"
#include "inventry.h"
#include "pfarlocs.h"
#include "weapons.h"
#include "pheromon.h"
#include "bh_pred.h"
#include "psnd.h"
#include "bh_weap.h"
#include "equipmnt.h"
#include "bh_agun.h"
#include "los.h"
#include "krender.h"
#include "pldnet.h"
#include "BonusAbilities.h"
#include "avp_menus.h"
#include "lighting.h"
#include "scream.h"
#include "avp_userprofile.h"
#include "Pldghost.h"


#define ALIEN_CONTACT_WEAPON 0
#if ALIEN_CONTACT_WEAPON
static void AlienContactWeapon(void);
#endif

#ifdef AVP_DEBUG_VERSION
	#define FLY_MODE_CHEAT_ON 1
#else
	#ifdef AVP_DEBUG_FOR_FOX
		#define FLY_MODE_CHEAT_ON 1
	#else
		#define FLY_MODE_CHEAT_ON 1
	#endif
#endif
//!(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO||DEATHMATCH_DEMO)
extern int DebouncedGotAnyKey;
extern int GrabAttackInProgress;
extern int RamAttackInProgress;
extern int Kit[3];
extern int evolution;

/*KJL*****************************************************
* If the define below is set to non-zero then the player *
* movement values will be loaded in from movement.txt	 *
*****************************************************KJL*/
#define LOAD_IN_MOVEMENT_VALUES 0

#if SupportWindows95 && LOAD_IN_MOVEMENT_VALUES	

static int AlienForwardSpeed;
static int AlienStrafeSpeed;
static int AlienTurnSpeed;	
static int AlienJumpSpeed;
static int PredatorForwardSpeed;
static int PredatorStrafeSpeed;
static int PredatorTurnSpeed;	
static int PredatorJumpSpeed;
static int MarineForwardSpeed;
static int MarineStrafeSpeed;
static int MarineTurnSpeed;	
static int MarineJumpSpeed;

static void LoadInMovementValues(void);
#endif

/* Globals */
extern int CrouchIsToggleKey;
char CrouchKeyDebounced;
int executeDemo;
int ZeroG;
int Underwater;
int Surface;
int Fog;
int CurrentSpeed;
int StrugglePool;
extern int OnLadder;
extern int Operable;
extern int StrugglePoolDelay;

/* Global Externs */
extern DISPLAYBLOCK* Player;
extern int NormalFrameTime;
extern int cosine[], sine[];
extern int predHUDSoundHandle;
extern int predOVision_SoundHandle;
extern int TauntSoundPlayed;

extern DPID GrabPlayer;
extern DPID HuggedBy;

#if SupportWindows95
extern unsigned char GotAnyKey;
#else
unsigned char GotAnyKey;
#endif

static char FlyModeOn = 0;			
static char FlyModeDebounced = 0;

static char BonusAbilityDebounced = 0;

extern int deathFadeLevel;
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;

// DISPLAYBLOCK *playerdb;

extern void DeInitialisePlayer(void);
extern int MeleeWeapon_90Degree_Front_Core(DAMAGE_PROFILE *damage, int multiple, int range);
extern void ChangeToMarine(void);
extern void ChangeToAlien(void);
extern void ChangeToPredator(void);
extern STRATEGYBLOCK *CreateGrenadeKernel(AVP_BEHAVIOUR_TYPE behaviourID, VECTORCH *position, MATRIXCH *orient, int fromplayer);
extern int DetermineMarineVoice(unsigned int Class);
extern void BinocularsZoom(void);
extern void IronSightsZoom(void);
extern DPID GrabbedPlayer;

/* some prototypes for this source file */
static void MakePlayerCrouch(STRATEGYBLOCK* sbPtr);
static void MakePlayerLieDown(STRATEGYBLOCK* sbPtr);
static void MaintainPlayerShape(STRATEGYBLOCK* sbPtr);
static void NetPlayerDeadProcessing(STRATEGYBLOCK* sbPtr);
static void CorpseMovement(STRATEGYBLOCK *sbPtr);

extern SECTION * GetNamedHierarchyFromLibrary(const char * rif_name, const char * hier_name);
extern void NewOnScreenMessage(unsigned char *messagePtr);
extern void RemoveAllThisPlayersDiscs(void);
extern int CarryingTooMuch(void);
void NetPlayerRespawn(STRATEGYBLOCK *sbPtr);

int timeInContactWithFloor;
float Stamina;

extern int weaponHandle;

extern int PlayerDamagedOverlayIntensity;


#define JETPACK_MAX_SPEED 10000
#define JETPACK_THRUST 40000

/*----------------------------------------------------------- 
Initialise player movement data
-------------------------------------------------------------*/
void InitPlayerMovementData(STRATEGYBLOCK* sbPtr)
{
	InitPlayerGameInput(sbPtr);
	
	/* set the player's morph control block and state*/
	{
		PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(sbPtr->SBdataptr);    
    	LOCALASSERT(playerStatusPtr);

		playerStatusPtr->ShapeState = PMph_Standing;
		playerStatusPtr->ViewPanX = 0;
	
		playerStatusPtr->DemoMode = 0;
	}
	
	/* KJL 13:35:13 16/03/98 - make sure fly mode is off */
	FlyModeOn = 0;
	
	timeInContactWithFloor=(ONE_FIXED/10);

	#if SupportWindows95 && LOAD_IN_MOVEMENT_VALUES	
	LoadInMovementValues();
	#endif

}

void StartPlayerTaunt(void) {

	PLAYER_STATUS *playerStatusPtr;
    
	/* get the player status block ... */
	playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
    GLOBALASSERT(playerStatusPtr);
	
	if (playerStatusPtr->tauntTimer) {
		return;
	}
	if (Underwater) {
		return;
	}

	playerStatusPtr->tauntTimer=-1; /* Cue to start. */
	TauntSoundPlayed=0;
}

/*-------------- Patrick 15/10/96 ----------------
--------------------------------------------------*/
void PlayerBehaviour(STRATEGYBLOCK* sbPtr)
{
	PLAYER_STATUS *playerStatusPtr;
    
	/* get the player status block ... */
	playerStatusPtr = (PLAYER_STATUS *) (sbPtr->SBdataptr);
    GLOBALASSERT(playerStatusPtr);
  
    /* KJL 18:05:55 03/10/97 - is anybody there? */
    if (playerStatusPtr->IsAlive)
	{
		if (playerStatusPtr->MyFaceHugger!=NULL) return;

		if (playerStatusPtr->tauntTimer>0) {
			playerStatusPtr->tauntTimer-=NormalFrameTime;
			if (playerStatusPtr->tauntTimer<0) {
				playerStatusPtr->tauntTimer=0;
			}
		} else if (AvP.Network==I_No_Network) {
			/* *Might* need to monitor this... */
			if (playerStatusPtr->tauntTimer==-1) {
				/* Begin taunt. */
				playerStatusPtr->tauntTimer=TAUNT_LENGTH;
			} else if (playerStatusPtr->tauntTimer>0) {
				playerStatusPtr->tauntTimer-=NormalFrameTime;
				if (playerStatusPtr->tauntTimer<0) {
					playerStatusPtr->tauntTimer=0;
				}
			}
		}
		ExecuteFreeMovement(sbPtr);
	}
	else CorpseMovement(sbPtr);

	if(playerStatusPtr->IsAlive)
	{
		if ((sbPtr->containingModule)&&(!Observer)) {
			/* Update pheromone system. If there's no containing module,           *
			 * well... I sigh with despair at the system.  But I cannot change it. */
			
			switch(AvP.PlayerType)
			{
				case I_Marine:
					AddMarinePheromones(sbPtr->containingModule->m_aimodule);
					break;
				case I_Predator:
					/* Ah well, for the moment... */
					AddMarinePheromones(sbPtr->containingModule->m_aimodule);
					break;
				case I_Alien:
					break;
				default:
					GLOBALASSERT(0);
					break;
			}
		}
	}

}

/* Struggle() :: Try to escape from a grab */
void Struggle()
{
	PLAYER_STATUS *psPtr = Player->ObStrategyBlock->SBdataptr;
	int Class = psPtr->Class;
	int chance = FastRandom()%100;

	if (StrugglePoolDelay > 0) return;

	if (chance <= 20)
	{
		PlayMarineScream(0, SC_Oooph, 0, &psPtr->soundHandle, &(Player->ObStrategyBlock->DynPtr->Position));
		netGameData.myLastScream = SC_Oooph;
	}

	StrugglePool = (StrugglePool+(ONE_FIXED*5));

	StrugglePoolDelay = ONE_FIXED;

	if (StrugglePool >= (ONE_FIXED*50))
	{
		StrugglePool = 0;
		GrabbedPlayer = 0;

		//reset dynamics
		{
			EULER zeroEuler = {0,0,0};
			VECTORCH zeroVec = {0,0,0};
			DYNAMICSBLOCK *dynPtr = Player->ObStrategyBlock->DynPtr;

			dynPtr->Position = Player->ObStrategyBlock->DynPtr->Position;
			dynPtr->OrientEuler = zeroEuler;
			dynPtr->LinVelocity = zeroVec;
			dynPtr->LinImpulse = zeroVec;

			CreateEulerMatrix(&dynPtr->OrientEuler, &dynPtr->OrientMat);
			TransposeMatrixCH(&dynPtr->OrientMat);
		}
	}
}

/* ThrowHugger() :: Try to throw away a facehugger */
void ThrowHugger()
{
  if (HuggedBy)
  {
	  extern int NumActiveStBlocks;
	  extern STRATEGYBLOCK *ActiveStBlockList[];
	  STRATEGYBLOCK *sbPtr;
	  int sbIndex = 0;
	  PLAYER_STATUS *psPtr = (PLAYER_STATUS *) Player->ObStrategyBlock->SBdataptr;

	  /* You have only 2 seconds to throw away the hugger */
	  if (psPtr->ChestbursterTimer < (ONE_FIXED*18))
		  return;

	  /* Search all active Strategyblocks for the player hugging us */
	  while(sbIndex < NumActiveStBlocks)
	  {
		  sbPtr = ActiveStBlockList[sbIndex++];

		  if (sbPtr && sbPtr->I_SBtype && sbPtr->DynPtr)
		  {
			  if (sbPtr->I_SBtype == I_BehaviourNetGhost)
			  {
				  NETGHOSTDATABLOCK *ghostData = sbPtr->SBdataptr;

				  /* Found him! */
				  if (ghostData->playerId == HuggedBy)
				  {
					  /* Added 10% chance that it works */
					  if ((FastRandom()%100) <= 10)
					  {
						  CauseDamageToObject(sbPtr, &TemplateAmmo[AMMO_CUDGEL].MaxDamage[AvP.Difficulty], ONE_FIXED, NULL);
  						  HuggedBy = 0;
						  PlayerStatusPtr->ChestbursterTimer = 0;
					  }
				  }
			  }
		  }
	  }
  }
}

/*------------------------Patrick 21/10/96------------------------
  Newer cleaned up version, supporting new input functions
  ----------------------------------------------------------------*/
/* Tweaked movementrates, Eldritch */
#define ALIEN_MOVESCALE    5000 //6000
#define PREDATOR_MOVESCALE 4600
#define MARINE_MOVESCALE   3600

#define TURNSCALE 2000
#define JUMPVELOCITY 7200
#define ALIEN_JUMPVELOCITY 9000
#define PREDATOR_JUMPVELOCITY 8100

#define FASTMOVESCALE 6000
#define SLOWMOVESCALE 4000
#define FASTTURNSCALE 1700
#define SLOWTURNSCALE 1700
#define FASTSTRAFESCALE 5000
#define SLOWSTRAFESCALE 3000

/* KJL 14:39:45 01/14/97 - Camera stuff */
#define	PANRATESHIFT 6	
#define TIMEBEFOREAUTOCENTREVIEW 16384

/* patrick 9/7/97: these are for testing AI pre-calculated values... */
#define PATTEST_EPS	0
#define PATTEST_AUXLOCS 0
#if (PATTEST_EPS&&PATTEST_AUXLOCS)
	#error Cannot have both
#endif 
#if PATTEST_EPS
	void EpLocationTest(void);
#endif
#if PATTEST_AUXLOCS
	void AuxLocationTest(void);
#endif

void ExecuteFreeMovement(STRATEGYBLOCK* sbPtr)
{
	DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;

	/* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (sbPtr->SBdataptr);

	if (dynPtr->IsInContactWithFloor) {
		timeInContactWithFloor+=NormalFrameTime;
	} else {
		timeInContactWithFloor=0;
	}
	
	/*------------------------------------------------------ 
	GAME INPUTS 
	Call the (platform dependant) game input reading fn.
	------------------------------------------------------*/ 
	ReadPlayerGameInput(sbPtr);
 
	/* KJL 11:07:42 10/09/98 - Bonus Abilities */
	switch (AvP.PlayerType)
	{
		case I_Alien:
			break;
		#if 0
		case I_Predator: /* KJL 11:08:19 10/09/98 - Grappling Hook */
		{
			if (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_BonusAbility)
			{
				if(BonusAbilityDebounced)
				{
					ActivateGrapplingHook();
					BonusAbilityDebounced = 0;
				}
			}
			else BonusAbilityDebounced = 1;
			
			break;
		}
		#endif
		case I_Predator: /* KJL 11:08:19 10/09/98 - Cycle Vision Mode */
		{
			if (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_CycleVisionMode)
			{
				ChangePredatorVisionMode();
			}
			if (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_ReverseCycleVisionMode)
			{
				ReverseChangePredatorVisionMode();
			}
			if (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_GrapplingHook && 
				playerStatusPtr->GrapplingHookEnabled)
			{
				ActivateGrapplingHook();
			}

			break;
		}
		case I_Marine:
			break;
	}

	if (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Operate)
	{
		// Medikit, Repair-Kit and Tech-Kit class abilities
		if (AvP.Network != I_No_Network)
		{
			if (GrabbedPlayer)
			{
				Struggle();
				return;
			}

			if (HuggedBy)
			{
				ThrowHugger();
				return;
			}

			if (playerStatusPtr->Class == CLASS_COM_TECH ||
				playerStatusPtr->Class == CLASS_SMARTGUNNER ||
				playerStatusPtr->Class == CLASS_INC_SPEC ||
				playerStatusPtr->Class == CLASS_RIFLEMAN ||
				playerStatusPtr->Class == CLASS_AA_SPEC ||
				playerStatusPtr->Class == CLASS_ENGINEER)
			{
				if (Operable == 2)
					OperateObjectInLineOfSight();
			}
			if (playerStatusPtr->Class == CLASS_MEDIC_FT)
			{
				if (Operable == 2)
					OperateObjectInLineOfSight();
				else 
				{
					if (playerStatusPtr->Medikit) {
						Sound_Play(SID_PICKUP,"h");
						MeleeWeapon_90Degree_Front_Core(&TemplateAmmo[AMMO_SONIC_PULSE].MaxDamage[AvP.Difficulty],0,TemplateAmmo[AMMO_SONIC_PULSE].MaxRange);
						playerStatusPtr->Medikit--;
					}
				}
			}
#if 0
			if ((Kit[0] == KIT_TECH) || (Kit[1] == KIT_TECH))
			{
				if (Operable == 2)
					OperateObjectInLineOfSight();
				else 
				{
					if (playerStatusPtr->Medikit) {
						Sound_Play(SID_MARINE_PICKUP_ARMOUR,"h");
						MeleeWeapon_90Degree_Front_Core(&TemplateAmmo[AMMO_FLARE_GRENADE].MaxDamage[AvP.Difficulty],0,TemplateAmmo[AMMO_FLARE_GRENADE].MaxRange);
						playerStatusPtr->Medikit--;
					}
				}
			}
#endif
		} else {
			OperateObjectInLineOfSight();
		}
	}
			
	/* patrick 9/7/97: these are for testing AI pre-calculated values... */
	#if PATTEST_EPS
		EpLocationTest();
	#endif
	#if PATTEST_AUXLOCS
		AuxLocationTest();
	#endif
	

	/* Alien damages things by being in contact with them */
	#if ALIEN_CONTACT_WEAPON
	if (AvP.PlayerType == I_Alien) AlienContactWeapon();
	#endif

	/*------------------------------------------------------ 
	MOVEMENT

	NB player must be standing for faster movement
	------------------------------------------------------*/ 
	
	/* KJL 16:59:53 01/07/97 - New 3d strategy code	*/
	{
		int MaxSpeed;
		int forwardSpeed;
		int strafeSpeed; 
		int turnSpeed; 	
		int jumpSpeed;
		int Running=0;

		#if SupportWindows95 && LOAD_IN_MOVEMENT_VALUES	
		switch (AvP.PlayerType)
		{
			case I_Alien:
				forwardSpeed = AlienForwardSpeed;
				strafeSpeed  = AlienStrafeSpeed;
				turnSpeed    = AlienTurnSpeed;	
				jumpSpeed    = AlienJumpSpeed;
				break;
			
			case I_Predator:
				forwardSpeed = PredatorForwardSpeed;
				strafeSpeed  = PredatorStrafeSpeed;
				turnSpeed    = PredatorTurnSpeed;	
				jumpSpeed    = PredatorJumpSpeed;
				break;
			
			case I_Marine:
				forwardSpeed = MarineForwardSpeed;
				strafeSpeed  = MarineStrafeSpeed;
				turnSpeed    = MarineTurnSpeed;	
				jumpSpeed    = MarineJumpSpeed;
				break;
		}
		#else
		switch (AvP.PlayerType)
		{
			case I_Alien:
				forwardSpeed = ALIEN_MOVESCALE;
				strafeSpeed = ALIEN_MOVESCALE;
				turnSpeed =	TURNSCALE;
				jumpSpeed = ALIEN_JUMPVELOCITY;
				break;
			case I_Predator:
				forwardSpeed = PREDATOR_MOVESCALE;
				strafeSpeed = PREDATOR_MOVESCALE;
				turnSpeed =	TURNSCALE;
				jumpSpeed = PREDATOR_JUMPVELOCITY;
				break;
			case I_Marine:
				forwardSpeed = MARINE_MOVESCALE;
				strafeSpeed = MARINE_MOVESCALE;
				turnSpeed =	TURNSCALE;
				jumpSpeed = JUMPVELOCITY;
				break;
		}
		#endif

		MaxSpeed=forwardSpeed;

		if((playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe)&&(playerStatusPtr->Mvt_SideStepIncrement==0))
		{
			strafeSpeed	= MUL_FIXED(strafeSpeed,playerStatusPtr->Mvt_TurnIncrement);
		}
		else
		{
			strafeSpeed	= MUL_FIXED(strafeSpeed,playerStatusPtr->Mvt_SideStepIncrement);
		}
		forwardSpeed = MUL_FIXED(forwardSpeed,playerStatusPtr->Mvt_MotionIncrement);
		turnSpeed    = MUL_FIXED(turnSpeed,playerStatusPtr->Mvt_TurnIncrement);
		
		if (MIRROR_CHEATMODE)
		{
			turnSpeed = -turnSpeed;
			strafeSpeed = -strafeSpeed;
		}
		
		{
			extern int CameraZoomLevel;
			if(CameraZoomLevel)
			{
				turnSpeed >>= CameraZoomLevel;
				playerStatusPtr->Mvt_PitchIncrement >>= CameraZoomLevel;
			}
		}

		{
			extern float CameraZoomScale;
			if ((AvP.PlayerType == I_Marine) && (CameraZoomScale != 1.0f))
			{
				if (CameraZoomScale != 0.2f)
				{
					turnSpeed >>= 1;
					playerStatusPtr->Mvt_PitchIncrement >>= 1;
				} else {
					turnSpeed >>= 2;
					playerStatusPtr->Mvt_PitchIncrement >>= 2;
				}
			}
		}
		
		if (playerStatusPtr->Honor)
		{
			/* Brake! */
			if (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jump)
			{
				CurrentSpeed = CurrentSpeed-1000;
				if (CurrentSpeed < 0) CurrentSpeed=0;
				forwardSpeed = CurrentSpeed;
				if (forwardSpeed < 0) forwardSpeed=0;
				return;
			}
			/* Forward */
			if (playerStatusPtr->Mvt_MotionIncrement > 0)
			{
				if (CurrentSpeed < 0)
				{
					CurrentSpeed = CurrentSpeed+200;

					if (!stricmp("apclevel2",&LevelName))
					{
						CurrentSpeed = CurrentSpeed+300;
						if (CurrentSpeed > 22500) CurrentSpeed=22500;
							forwardSpeed = CurrentSpeed;
						if (forwardSpeed > 22500) forwardSpeed=22500;
							forwardSpeed = CurrentSpeed;
					}
					else 
					{
						if (CurrentSpeed > 15000) CurrentSpeed=15000;
							forwardSpeed = CurrentSpeed;
						if (forwardSpeed > 15000) forwardSpeed=15000;
							forwardSpeed = CurrentSpeed;
						return;
					}
				}
				if (playerStatusPtr->Mvt_TurnIncrement !=0) {
					if (playerStatusPtr->Mvt_TurnIncrement > 0)
						turnSpeed = CurrentSpeed/10;
					else
						turnSpeed = -CurrentSpeed/10;
					if (turnSpeed > 700) turnSpeed = 700;
					if (turnSpeed < -700) turnSpeed = -700;
				}
				if (CurrentSpeed < 15000)
				{
					if (!stricmp("apclevel2", &LevelName))
					{
						CurrentSpeed = CurrentSpeed+75;
						if (CurrentSpeed > 22500)
							CurrentSpeed = 22500;
					}
					else
					{
						CurrentSpeed = CurrentSpeed+50;
						if (CurrentSpeed > 15000)
							CurrentSpeed = 15000;
					}
				}
				forwardSpeed = CurrentSpeed;
			}
			/* Backward */
			else if (playerStatusPtr->Mvt_MotionIncrement < 0)
			{
				if (CurrentSpeed > 0)
				{
					if (!stricmp("apclevel2", &LevelName))
						CurrentSpeed = CurrentSpeed-300;
					else
						CurrentSpeed = CurrentSpeed-200;

					if (CurrentSpeed < 0) CurrentSpeed=0;
						forwardSpeed = CurrentSpeed;
					if (forwardSpeed < 0) forwardSpeed=0;
						forwardSpeed = CurrentSpeed;
					return;
				}
				if (playerStatusPtr->Mvt_TurnIncrement !=0) {
					if (playerStatusPtr->Mvt_TurnIncrement > 0)
						turnSpeed = -CurrentSpeed/10;
					else
						turnSpeed = CurrentSpeed/10;
					if (turnSpeed > 700) turnSpeed = 700;
					if (turnSpeed < -700) turnSpeed = -700;
				}
				if (CurrentSpeed > -7000)
				{
					if (!stricmp("apclevel2", &LevelName))
						CurrentSpeed = CurrentSpeed-60;
					else
						CurrentSpeed = CurrentSpeed-40;
				}
				if (CurrentSpeed < -7000)
					CurrentSpeed = -7000;
				forwardSpeed = CurrentSpeed;
			} else {
				/* Just letting it roll baby... */
				if (playerStatusPtr->Mvt_TurnIncrement !=0) {
					if (playerStatusPtr->Mvt_TurnIncrement > 0)
						turnSpeed = CurrentSpeed/10;
					else
						turnSpeed = -CurrentSpeed/10;
					if (turnSpeed > 700) turnSpeed = 700;
					if (turnSpeed < -700) turnSpeed = -700;
				}
				CurrentSpeed = CurrentSpeed-200;
				if (CurrentSpeed < 0) CurrentSpeed=0;
				forwardSpeed = CurrentSpeed;
				if (forwardSpeed < 0) forwardSpeed=0;
				forwardSpeed = CurrentSpeed;
			}
		}

		// Slow! and Fast! class abilities
		if ((AvP.Network != I_No_Network) && (playerStatusPtr->Mvt_MotionIncrement != 0) &&
			(!playerStatusPtr->Honor))
		{
			if (playerStatusPtr->Class == CLASS_PRED_HUNTER ||
				playerStatusPtr->Class == CLASS_TANK_ALIEN)
			{
				forwardSpeed = ((forwardSpeed)+500);
			}
			if (playerStatusPtr->Class == CLASS_ALIEN_DRONE)
			{
				forwardSpeed = ((forwardSpeed)+1000);
			}
			if (playerStatusPtr->Class == CLASS_EXF_SNIPER)
			{
				forwardSpeed = ((forwardSpeed)-1000);
			}
			if (playerStatusPtr->Class == CLASS_MEDIC_PR)
			{
				forwardSpeed = ((forwardSpeed)-1000);
			}
			if (playerStatusPtr->Immobilized)
			{
				forwardSpeed = 0;
				strafeSpeed = 0;
				turnSpeed = 0;
				jumpSpeed = 0;
			}
			if (playerStatusPtr->Destr)
			{
				forwardSpeed = ((forwardSpeed)-1000);
			}
			if ((AvP.PlayerType == I_Alien) && (GrabPlayer))
			{
				forwardSpeed = (forwardSpeed)/2;
			}
		}
		// No network...
		if ((AvP.Network == I_No_Network) && (playerStatusPtr->Mvt_MotionIncrement != 0) &&
			(!playerStatusPtr->Honor))
		{
			if ((Underwater==1) || (ZeroG==1) || (playerStatusPtr->IsMovingInWater))
			{
				forwardSpeed = ((forwardSpeed)-500);
				strafeSpeed = ((strafeSpeed)-500);
			}
			// HEP-Suits are encumbering
			if (playerStatusPtr->ArmorType==1)
			{
				forwardSpeed = ((forwardSpeed)-600);
			}
			if ((AvP.PlayerType == I_Alien) && (GrabAttackInProgress))
			{
				forwardSpeed = (forwardSpeed)/2;
			}
		}

#if 1
		// Standing...
		if (playerStatusPtr->ShapeState == PMph_Standing)
		{
			// ... and Running...
			if (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Faster)
			{
				// ... Forwards...
				if (playerStatusPtr->Mvt_MotionIncrement >= 0)
				{
					// ... in other env's than Underwater, Zero-G and Ladders...
					if ((!Underwater) && (!ZeroG) && (!OnLadder))
					{
						// Will cause humans to run 75% faster, aliens 120% and preds 90%.
						if (AvP.PlayerType == I_Alien)
							forwardSpeed = (forwardSpeed)*1.75;
						else if (AvP.PlayerType == I_Marine)
							forwardSpeed = (forwardSpeed)*1.75;
						else
							forwardSpeed = (forwardSpeed)*1.9;
					}
					// ... in water, zero-G and/or ladders does nothing.
				} else
				// ... backwards decreases speed by 500mm/s.
				forwardSpeed = (forwardSpeed)/2;//-500;
			}
			else	// ... and Walking...
			{
				// ... forwards...
				if (playerStatusPtr->Mvt_MotionIncrement >= 0)
				{
					// ...does nothing.
				}
				else // ... backwards
				{
					// ... decreases speed by 500mm/s.
					forwardSpeed = (forwardSpeed)/2;//-500;
				}
			}
			// Crouching...
		} 
		else if (playerStatusPtr->ShapeState == PMph_Crouching)
		{
			// ... forwards...
			if (playerStatusPtr->Mvt_MotionIncrement >= 0)
			{
				// ... will cause aliens to increase speed by 75%....
				if (AvP.PlayerType == I_Alien)
					forwardSpeed = (forwardSpeed)*2.2;
			}
			else	// ... backwards... 
			{
				// ... will cause aliens to get a decreament by 50%.
				if (AvP.PlayerType == I_Alien)
					forwardSpeed = (forwardSpeed)/2;
			}
			// ... while the rest (preds and humans) will gain a decrease by 50%.
			if (AvP.PlayerType != I_Alien)
			{
				strafeSpeed = (strafeSpeed)/2;
				forwardSpeed = (forwardSpeed)/2;
			}
		}
#else
		if (((AvP.PlayerType == I_Alien) || (playerStatusPtr->ShapeState == PMph_Standing)) && 
			(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Faster == 0))
		{

		} else 
		{
			if ((playerStatusPtr->ShapeState == PMph_Standing)) {
				/* run = 50% more speed */
				if ((playerStatusPtr->Mvt_MotionIncrement >= 0) &&
					(!Underwater) && (!ZeroG) && (!OnLadder))
					forwardSpeed = (forwardSpeed)*1.75;
			} else {
				if (AvP.PlayerType != I_Alien) {
					/* crouching = half speed */
					strafeSpeed = (strafeSpeed)/2;
					forwardSpeed = (forwardSpeed)/2;
				} else {
					forwardSpeed = (forwardSpeed)*2;
				}
			}
		}
#endif
		/* Marker */

		if (!playerStatusPtr->Honor) {
			strafeSpeed=MUL_FIXED(strafeSpeed,playerStatusPtr->Encumberance.MovementMultiple);
			forwardSpeed=MUL_FIXED(forwardSpeed,playerStatusPtr->Encumberance.MovementMultiple);
			turnSpeed=MUL_FIXED(turnSpeed,playerStatusPtr->Encumberance.TurningMultiple);
			jumpSpeed=MUL_FIXED(jumpSpeed,playerStatusPtr->Encumberance.JumpingMultiple);
		}
		/* KJL 17:45:03 9/9/97 - inertia means it's difficult to stop */			
	  	if (forwardSpeed*playerStatusPtr->ForwardInertia<0) playerStatusPtr->ForwardInertia = 0;
	  	if (strafeSpeed*playerStatusPtr->StrafeInertia<0) playerStatusPtr->StrafeInertia = 0;
	  	
	  	if (!forwardSpeed)
		{
			int deltaForward = (FASTMOVESCALE*NormalFrameTime)>>14;
			if (playerStatusPtr->ForwardInertia>0)
			{
				forwardSpeed = playerStatusPtr->ForwardInertia - deltaForward;
				if (forwardSpeed<0) forwardSpeed=0;
			}
			else if (playerStatusPtr->ForwardInertia<0)
			{
				forwardSpeed = playerStatusPtr->ForwardInertia + deltaForward;
				if (forwardSpeed>0) forwardSpeed=0;
			}
		}
		else
		{
			int deltaForward = MUL_FIXED(forwardSpeed*4,NormalFrameTime);
			{
				int a = playerStatusPtr->ForwardInertia + deltaForward;
				if (forwardSpeed>0)
				{
					if (a<forwardSpeed) forwardSpeed = a;
				}
				else
				{
					if (a>forwardSpeed) forwardSpeed = a;
				}
			}
		}

		if (!strafeSpeed)
		{
			int deltaStrafe = (FASTSTRAFESCALE*NormalFrameTime)>>14;
			if (playerStatusPtr->StrafeInertia>0)
			{
				strafeSpeed = playerStatusPtr->StrafeInertia - deltaStrafe;
				if (strafeSpeed<0) strafeSpeed=0;
			}
			else if (playerStatusPtr->StrafeInertia<0)
			{
				strafeSpeed = playerStatusPtr->StrafeInertia + deltaStrafe;
				if (strafeSpeed>0) strafeSpeed=0;
			}
		}
		else
		{
			int deltaForward = MUL_FIXED(strafeSpeed*4,NormalFrameTime);
			{
				int a = playerStatusPtr->StrafeInertia + deltaForward;
				if (strafeSpeed>0)
				{
					if (a<strafeSpeed) strafeSpeed = a;
				}
				else
				{
					if (a>strafeSpeed) strafeSpeed = a;
				}
			}
		}

		/* inertia on turning - currently off */
		#if 0
		if(!turnSpeed)
		{
			int deltaTurn = (FASTTURNSCALE*NormalFrameTime)>>15;
			if (playerStatusPtr->TurnInertia>0)
			{
				turnSpeed = playerStatusPtr->TurnInertia - deltaTurn;
				if (turnSpeed<0) turnSpeed=0;
			}
			else if (playerStatusPtr->TurnInertia<0)
			{
				turnSpeed = playerStatusPtr->TurnInertia + deltaTurn;
				if (turnSpeed>0) turnSpeed=0;
			}
		}
		#endif

		/* Hold it! Correct forwardSpeed vs. strafeSpeed? */

		#if 0
		{
			int mag,angle;

			mag=(forwardSpeed*forwardSpeed)+(strafeSpeed*strafeSpeed);
			if (mag>(MaxSpeed*MaxSpeed)) {

				angle=ArcTan(forwardSpeed,strafeSpeed);

				forwardSpeed=MUL_FIXED(GetSin(angle),MaxSpeed);
				strafeSpeed=MUL_FIXED(GetCos(angle),MaxSpeed);
			
			}
		}
		#endif
		
		#if 0	//No Jetpack in AMP build
		if (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jetpack &&
			playerStatusPtr->JetpackEnabled)
		{
			if (dynPtr->LinImpulse.vy>-JETPACK_MAX_SPEED)
			{
				dynPtr->LinImpulse.vy-=MUL_FIXED(JETPACK_THRUST,NormalFrameTime);
			}
			AddLightingEffectToObject(Player,LFX_OBJECTONFIRE);
			/* Sound handling. */
			if (playerStatusPtr->soundHandle5==SOUND_NOACTIVEINDEX) {
				Sound_Play(SID_ED_JETPACK_START,"h");
				Sound_Play(SID_ED_JETPACK_MID,"el",&playerStatusPtr->soundHandle5);
			}

		} else {
			/* Sound handling. */
			if (playerStatusPtr->soundHandle5!=SOUND_NOACTIVEINDEX) {
				Sound_Play(SID_ED_JETPACK_END,"h");
				Sound_Stop(playerStatusPtr->soundHandle5);
			}
		}
		#endif

		#if FLY_MODE_CHEAT_ON
		dynPtr->GravityOn=1;
		if ((ZeroG==1) || (Underwater==1) || (OnLadder==1))
		{
			FlyModeOn = 1;
		} else {
			FlyModeOn = 0;
		}

		if(FlyModeOn)
		{
			dynPtr->LinVelocity.vx = 0;
			dynPtr->LinVelocity.vy = 0;
			dynPtr->LinVelocity.vz = forwardSpeed;
			playerStatusPtr->Encumberance.CanCrouch = 0;
			playerStatusPtr->Encumberance.CanRun = 0;
			if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe)
			{
				dynPtr->LinVelocity.vx = strafeSpeed;
			}
			else if((playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft)
				|| (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight))
			{
				dynPtr->LinVelocity.vx = strafeSpeed;
			}
			else if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jump)
			{
				dynPtr->LinVelocity.vy = -1500;
			}
			else if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Crouch)
			{
				dynPtr->LinVelocity.vy = 1500;
			}

		   	/* rotate LinVelocity along camera view */
			{
				MATRIXCH mat = Global_VDB_Ptr->VDB_Mat;
				TransposeMatrixCH(&mat);
				RotateVector(&dynPtr->LinVelocity,&mat);
			}
			dynPtr->GravityOn=0;
			dynPtr->LinImpulse.vx=0;
			dynPtr->LinImpulse.vy=0;
			dynPtr->LinImpulse.vz=0;
		}
		else
		#endif
		/* KJL 12:28:48 14/04/98 - if we're not in contact with the floor, but we've hit
		something, set our velocity to zero (otherwise leave it alone) */
		if(!dynPtr->IsInContactWithFloor)
		{
			if (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jetpack &&
				playerStatusPtr->JetpackEnabled)
			{
				dynPtr->LinVelocity.vx = 0;
				dynPtr->LinVelocity.vy = 0;
				if (forwardSpeed>0)
				{
					dynPtr->LinVelocity.vz = forwardSpeed/2;
				}
				else
				{
					dynPtr->LinVelocity.vz = forwardSpeed/4;
				}
	//			dynPtr->IsNetGhost=1;
				if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe)
				{
					dynPtr->LinVelocity.vx = strafeSpeed/4;
				}
				else if((playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft)
					|| (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight))
				{
					dynPtr->LinVelocity.vx = strafeSpeed/4;
				}

				/* rotate LinVelocity into world space */
				RotateVector(&dynPtr->LinVelocity,&dynPtr->OrientMat);
			}
			else if (dynPtr->CollisionReportPtr)
			{
	  			dynPtr->LinVelocity.vx = 0;
	  			dynPtr->LinVelocity.vy = 0;
	  			dynPtr->LinVelocity.vz = forwardSpeed/8;
				/* rotate LinVelocity into world space */
				RotateVector(&dynPtr->LinVelocity,&dynPtr->OrientMat);
				
			}	
		}
		/* this bit sets the velocity: don't do it in demo mode, though
		as we set our own velocity... */
		else if((dynPtr->IsInContactWithFloor)&&(!(playerStatusPtr->DemoMode)))
		{
			dynPtr->LinVelocity.vx = 0;
			dynPtr->LinVelocity.vy = 0;
			dynPtr->LinVelocity.vz = forwardSpeed;
		
			if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe)
			{
				dynPtr->LinVelocity.vx = strafeSpeed;
			}
			else if((playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft)
				|| (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight))
			{
				dynPtr->LinVelocity.vx = strafeSpeed;
			}

			if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jump)
			{
				COLLISIONREPORT *reportPtr = Player->ObStrategyBlock->DynPtr->CollisionReportPtr;
				int notTooSteep = 0;
				
				while (reportPtr) /* while there is a valid report */
				{
					int dot = DotProduct(&(reportPtr->ObstacleNormal),&(dynPtr->GravityDirection));

					if (dot<-60000) 
					{
						notTooSteep = 1;
						break;
					}
					/* skip to next report */
					reportPtr = reportPtr->NextCollisionReportPtr;
				}
						
				if (notTooSteep)
				{
					float StaminaLoss = 1.0;
					float StaminaLossHigh = 2.0;	// for pounces/high leaps
					float StaminaMult = Stamina;

					if (AvP.Network == I_No_Network)
					{
						StaminaLoss = 0.0;
						StaminaLossHigh = 0.0;
					}

					if (StaminaMult > 1.0) StaminaMult = 1.0;

					/* alien can jump in the direction it's looking */									
					if (AvP.PlayerType == I_Alien)
					{
						VECTORCH viewDir;

						viewDir.vx = Global_VDB_Ptr->VDB_Mat.mat13;
						viewDir.vy = Global_VDB_Ptr->VDB_Mat.mat23;
						viewDir.vz = Global_VDB_Ptr->VDB_Mat.mat33;
						if ((playerStatusPtr->ShapeState == PMph_Crouching) && (DotProduct(&viewDir,&dynPtr->GravityDirection)<-32768))
						{
							dynPtr->LinImpulse.vx += MUL_FIXED(viewDir.vx,(jumpSpeed*3)*StaminaMult);
							dynPtr->LinImpulse.vy += MUL_FIXED(viewDir.vy,(jumpSpeed*3)*StaminaMult);
							dynPtr->LinImpulse.vz += MUL_FIXED(viewDir.vz,(jumpSpeed*3)*StaminaMult);

							Stamina -= StaminaLossHigh;
						}
						else
						{
							dynPtr->LinImpulse.vx -= MUL_FIXED(dynPtr->GravityDirection.vx,jumpSpeed);
							dynPtr->LinImpulse.vy -= MUL_FIXED(dynPtr->GravityDirection.vy,jumpSpeed);
							dynPtr->LinImpulse.vz -= MUL_FIXED(dynPtr->GravityDirection.vz,jumpSpeed);

							if ((playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward) &&
								(playerStatusPtr->ShapeState == PMph_Crouching))
							{
								/* Ramming should be shorter. */
								if (!RamAttackInProgress)
								{
								  	dynPtr->LinVelocity.vz += (jumpSpeed*1.5)*StaminaMult;
									Stamina -= StaminaLoss;
								}
								else
								{
									dynPtr->LinVelocity.vz += jumpSpeed*StaminaMult;
									Stamina -= StaminaLoss;
								}
							}
						}
						dynPtr->TimeNotInContactWithFloor = -1;
					}
					else if (AvP.PlayerType == I_Predator)
					{
						VECTORCH viewDir;

						viewDir.vx = Global_VDB_Ptr->VDB_Mat.mat13;
						viewDir.vy = Global_VDB_Ptr->VDB_Mat.mat23;
						viewDir.vz = Global_VDB_Ptr->VDB_Mat.mat33;

						if ((playerStatusPtr->ShapeState == PMph_Crouching) && (DotProduct(&viewDir,&dynPtr->GravityDirection)<-32768)) {
							dynPtr->LinImpulse.vx += MUL_FIXED(viewDir.vx,(jumpSpeed*2.3)*StaminaMult);
							dynPtr->LinImpulse.vy += MUL_FIXED(viewDir.vy,(jumpSpeed*2.3)*StaminaMult);
							dynPtr->LinImpulse.vz += MUL_FIXED(viewDir.vz,(jumpSpeed*2.3)*StaminaMult);
							dynPtr->TimeNotInContactWithFloor = 0;
							Stamina -= StaminaLossHigh;
						} else {
							dynPtr->LinImpulse.vx -= MUL_FIXED(dynPtr->GravityDirection.vx,jumpSpeed);
							dynPtr->LinImpulse.vy -= MUL_FIXED(dynPtr->GravityDirection.vy,jumpSpeed);
							dynPtr->LinImpulse.vz -= MUL_FIXED(dynPtr->GravityDirection.vz,jumpSpeed);
							dynPtr->TimeNotInContactWithFloor = 0;

							if (playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward &&
								playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Crouch)
							{
								dynPtr->LinVelocity.vz += (jumpSpeed*2)*StaminaMult;
								Stamina -= StaminaLoss;
							}
						}
					}
					else
					{
						dynPtr->LinImpulse.vx -= MUL_FIXED(dynPtr->GravityDirection.vx,jumpSpeed);
						dynPtr->LinImpulse.vy -= MUL_FIXED(dynPtr->GravityDirection.vy,jumpSpeed);
						dynPtr->LinImpulse.vz -= MUL_FIXED(dynPtr->GravityDirection.vz,jumpSpeed);
						dynPtr->TimeNotInContactWithFloor = 0;
					}

					switch(AvP.PlayerType)
					{
						case I_Marine:
						{
							#if 0
							if (playerStatusPtr->soundHandle==SOUND_NOACTIVEINDEX) {
								int rand=(FastRandom()%4);

								switch (rand) {
									case 0:
										Sound_Play(SID_MARINE_JUMP_START,"he",&playerStatusPtr->soundHandle);
										break;
									case 1:
										Sound_Play(SID_MARINE_JUMP_START_2,"he",&playerStatusPtr->soundHandle);
										break;
									case 2:
										Sound_Play(SID_MARINE_JUMP_START_3,"he",&playerStatusPtr->soundHandle);
										break;
									default:
										Sound_Play(SID_MARINE_JUMP_START_4,"he",&playerStatusPtr->soundHandle);
										break;
								}
							}
							#else
							if (playerStatusPtr->soundHandle==SOUND_NOACTIVEINDEX) {
								PlayMarineScream((DetermineMarineVoice(playerStatusPtr->Class)),SC_Jump,0,&playerStatusPtr->soundHandle,NULL);
								if(AvP.Network!=I_No_Network) netGameData.myLastScream=SC_Jump;
							}
							#endif
							break;
						}
						case I_Alien:
							break;
						case I_Predator:
						{
							#if 0
							if (playerStatusPtr->soundHandle==SOUND_NOACTIVEINDEX) {
								int rand=(FastRandom()%3);

								switch (rand) {
									case 0:
										Sound_Play(SID_PRED_JUMP_START_1,"he",&playerStatusPtr->soundHandle);
										break;
									case 1:
										Sound_Play(SID_PRED_JUMP_START_2,"he",&playerStatusPtr->soundHandle);
										break;
									default:
										Sound_Play(SID_PRED_JUMP_START_3,"he",&playerStatusPtr->soundHandle);
										break;
								}
							}
							#else
							if (playerStatusPtr->soundHandle==SOUND_NOACTIVEINDEX) {
								PlayPredatorSound(0,PSC_Jump,0,&playerStatusPtr->soundHandle,NULL);
								if(AvP.Network!=I_No_Network) netGameData.myLastScream=PSC_Jump;
							}
							#endif
							break;
						}
						default:
							break;

					}
				}
			}
			/* rotate LinVelocity into world space */
			RotateVector(&dynPtr->LinVelocity,&dynPtr->OrientMat);
		}

		/* zero angular velocity */
		dynPtr->AngVelocity.EulerX = 0;
		dynPtr->AngVelocity.EulerZ = 0;
		
		if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe)
		{
			dynPtr->AngVelocity.EulerY = 0;
		}
		else
		{
		 	dynPtr->AngVelocity.EulerY = turnSpeed;                       
		}
		
		playerStatusPtr->ForwardInertia = forwardSpeed;
		playerStatusPtr->StrafeInertia = strafeSpeed; 
		playerStatusPtr->TurnInertia = turnSpeed; 	
	}
	/*KJL****************************************************************************************
	* The player's AngVelocity as set by the above code is only valid in the player's object    *
	* space, and so has to be rotated into world space. So aliens can walk on the ceiling, etc. *
	****************************************************************************************KJL*/
	if (dynPtr->AngVelocity.EulerY)
	{
		MATRIXCH mat;
   	
   		int angle = MUL_FIXED(NormalFrameTime,dynPtr->AngVelocity.EulerY)&4095;
 	  	int cos = GetCos(angle);
 	  	int sin = GetSin(angle);
 	  	mat.mat11 = cos;		 
 	  	mat.mat12 = 0;
 	  	mat.mat13 = -sin;
 	  	mat.mat21 = 0;	  	
 	  	mat.mat22 = 65536;	  	
 	  	mat.mat23 = 0;	  	
 	  	mat.mat31 = sin;	  	
 	  	mat.mat32 = 0;	  	
 	  	mat.mat33 = cos;	  	

		MatrixMultiply(&dynPtr->OrientMat,&mat,&dynPtr->OrientMat);
	 	MatrixToEuler(&dynPtr->OrientMat, &dynPtr->OrientEuler);

	}
	/*------------------------------------------------------ 
	CROUCHING, LYING DOWN, ETC.
	------------------------------------------------------*/ 
	MaintainPlayerShape(sbPtr);
	
	/* Alien's wall-crawling abilities */
	if (AvP.PlayerType == I_Alien)
	{
		/* let alien walk on walls & ceiling */
		if ( (playerStatusPtr->ShapeState == PMph_Crouching)
		   &&(!dynPtr->RequestsToStandUp) )
		{
			dynPtr->UseStandardGravity=0;
		}
		else
		{
			dynPtr->UseStandardGravity=1;
		}
	}

    /*------------------------------------------------------ 
	WEAPON FIRING
	Kevin: The player input functions now interface directly
	with the weapons state machine.	I hope.
	------------------------------------------------------*/

	/*------------------------------------------------------ 
	CAMERA Controls
	------------------------------------------------------*/ 
	
	/* If AbsolutePitch is set, view angle comes direct from Mvt_PitchIncrement,
	   which takes values -65536 to +65536. */
	
	
	if (playerStatusPtr->Absolute_Pitching)
	{
		playerStatusPtr->ViewPanX = MUL_FIXED(playerStatusPtr->Mvt_PitchIncrement,1024-128);
		playerStatusPtr->ViewPanX &= wrap360;
	}
	else
	{
		static int timeBeenContinuouslyMoving=0;
		int AllowedLookDownAngle;
		int AllowedLookUpAngle;

		if (AvP.PlayerType==I_Alien)
		{
			AllowedLookUpAngle = 0;
			AllowedLookDownAngle = 2048;
		}
		else
		{
			AllowedLookUpAngle = 128;
			AllowedLookDownAngle = 2048-128;
		}

		#if SupportWindows95
		if (!ControlMethods.AutoCentreOnMovement)
		{
			timeBeenContinuouslyMoving = 0;
		}
		#endif

		if (playerStatusPtr->Mvt_MotionIncrement == 0)
		{
			timeBeenContinuouslyMoving=0;
		}
		else
		{
			if (timeBeenContinuouslyMoving>TIMEBEFOREAUTOCENTREVIEW
			&& !playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp
			&& !playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_CentreView =1;
			}
			else
			{
				timeBeenContinuouslyMoving+=NormalFrameTime;	
			}
		}
		
		if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp)
		{
        	playerStatusPtr->ViewPanX += 1024;
			playerStatusPtr->ViewPanX &= wrap360;
                               
			playerStatusPtr->ViewPanX += MUL_FIXED
									(
										playerStatusPtr->Mvt_PitchIncrement,
										NormalFrameTime>>PANRATESHIFT
									);

			if (playerStatusPtr->ViewPanX < AllowedLookUpAngle) playerStatusPtr->ViewPanX=AllowedLookUpAngle; 

        	playerStatusPtr->ViewPanX -= 1024;
			playerStatusPtr->ViewPanX &= wrap360;
		}
		else if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown)
		{
        	playerStatusPtr->ViewPanX += 1024;
			playerStatusPtr->ViewPanX &= wrap360;
                               
			playerStatusPtr->ViewPanX += MUL_FIXED
									(
										playerStatusPtr->Mvt_PitchIncrement,
										NormalFrameTime>>PANRATESHIFT
									);

			if (playerStatusPtr->ViewPanX > AllowedLookDownAngle) playerStatusPtr->ViewPanX=AllowedLookDownAngle; 

        	playerStatusPtr->ViewPanX -= 1024;
			playerStatusPtr->ViewPanX &= wrap360;
		} 
		if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_CentreView)
		{
        	playerStatusPtr->ViewPanX += 1024;
			playerStatusPtr->ViewPanX &= wrap360;
            
            if (playerStatusPtr->ViewPanX > 1024)
            {                  
				playerStatusPtr->ViewPanX -= (NormalFrameTime>>PANRATESHIFT)*2;
				if (playerStatusPtr->ViewPanX < 1024) playerStatusPtr->ViewPanX=1024; 
			}
            else if (playerStatusPtr->ViewPanX < 1024)
            {                  
				playerStatusPtr->ViewPanX += (NormalFrameTime>>PANRATESHIFT)*2;
				if (playerStatusPtr->ViewPanX > 1024) playerStatusPtr->ViewPanX=1024; 
			}

        	playerStatusPtr->ViewPanX -= 1024;
			playerStatusPtr->ViewPanX &= wrap360;
		}
	}

	HandleGrapplingHookForces();
}


/*------------------------------------------------------ 
Crouch and Lie down support fns.
------------------------------------------------------*/ 

static void MaintainPlayerShape(STRATEGYBLOCK* sbPtr)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (sbPtr->SBdataptr);

	/* maintain play morphing state */
	switch (playerStatusPtr->ShapeState)
	{
		case(PMph_Standing):
		{
			/* if we're standing, check inputs for a request to 
			   crouch or lie down */
			if (playerStatusPtr->Encumberance.CanCrouch)
			{
				if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Crouch) 
				{
					if (CrouchKeyDebounced)
					{
						MakePlayerCrouch(sbPtr);
						CrouchKeyDebounced = 0;
					}
				}
				else
				{
					CrouchKeyDebounced = 1;
				}
			
			}


			sbPtr->DynPtr->RequestsToStandUp=0;
					   
			break;
		}
		case(PMph_Crouching):
		{
			/* if we're crouching, then check inputs for crouch request.
			   if there isn't one, stand up again */
			if(sbPtr->DynPtr->RequestsToStandUp)
			{
				//currently crouching , but have had a request to stand up.
				//cancel request if the crouch key is pressed again
				if (playerStatusPtr->Encumberance.CanCrouch)
				{
					if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Crouch)
					{
						if (CrouchKeyDebounced)
						{
							sbPtr->DynPtr->RequestsToStandUp = 0;
							CrouchKeyDebounced = 0;
						}
					}
					else
					{
						CrouchKeyDebounced = 1;
					}
			
				}
			}
			else
			{
				if (!(playerStatusPtr->Encumberance.CanCrouch)) 
				{
					sbPtr->DynPtr->RequestsToStandUp=1;
				}
			
				if ((!CrouchIsToggleKey) && (AvP.PlayerType == I_Alien))
				{
					if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Crouch)
					{
						if (CrouchKeyDebounced)
						{
							sbPtr->DynPtr->RequestsToStandUp=1;
							CrouchKeyDebounced = 0;
						}
					}
					else
					{
						CrouchKeyDebounced = 1;
					}
				}
				else if(!(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Crouch))
				{
					sbPtr->DynPtr->RequestsToStandUp=1;
				}
			}
			break;
		}
		case(PMph_Lying):
		{
			/* if we're lying, then check inputs for lie request.
			if there isn't one, stand up again */
			break;
		}
		default:
		{
			/* should never get here */
			GLOBALASSERT(1==0);
		}
	
	}

}

static void MakePlayerCrouch(STRATEGYBLOCK* sbPtr)
{	
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (sbPtr->SBdataptr);

	/* set player state */
	playerStatusPtr->ShapeState = PMph_Crouching;

	return;
}

#if 0
static void MakePlayerLieDown(STRATEGYBLOCK* sbPtr)
{	
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (sbPtr->SBdataptr);
	

	/* set player state */
	playerStatusPtr->ShapeState = PMph_Lying;

	return;
}
#endif


int deathFadeLevel;

static void CorpseMovement(STRATEGYBLOCK *sbPtr)
{
	extern int RealFrameTime;

	/* only fade non-net game */
	if(AvP.Network == I_No_Network)
	{
		if(deathFadeLevel>0)
		{
			/* fade screen to black */
			//SetPaletteFadeLevel(deathFadeLevel);
			deathFadeLevel-= RealFrameTime/4;
			if (deathFadeLevel<0) deathFadeLevel = 0;

		}
		else
		{
			deathFadeLevel = 0;
			/* KJL 15:44:10 03/11/97 - game over, quit main loop */
			/* restart level instead -Richard*/
		  	if (DebouncedGotAnyKey)
			{
			  	AvP.RestartLevel = 1;
			}
		}
	}
	else
	{
		if(deathFadeLevel>0)
		{
			deathFadeLevel-= RealFrameTime/2;	
		}
		else
		{
			deathFadeLevel = 0;
			NetPlayerDeadProcessing(sbPtr);
		}
	}
}

/*-------------------Patrick 14/4/97--------------------
  This function does necessary processing for a dead
  network player...
  ------------------------------------------------------*/
static void NetPlayerDeadProcessing(STRATEGYBLOCK *sbPtr)
{
	//SECTION *root_section;
	BOOL Bursted=FALSE;
	extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
	extern int ChangedMyClass;

	#if SupportWindows95
	PLAYER_STATUS *psPtr= (PLAYER_STATUS *) (sbPtr->SBdataptr);

	/* call the read input function so that we can still respawn/quit, etc */
	ReadPlayerGameInput(sbPtr);

	/* If we are playing as a facehugger and have impregnated someone, then
	   we will have to wait until the chestburster erupts until we can respawn */
	if ((psPtr->Class == CLASS_EXF_W_SPEC) && (psPtr->AirSupply > (ONE_FIXED)))
	{
		psPtr->AirSupply -= NormalFrameTime;
		return;
	} else {
		psPtr->AirSupply = 0;
	}

	/* check for re-spawn */
	if(psPtr->Mvt_InputRequests.Flags.Rqst_FirePrimaryWeapon || (RamAttackInProgress))
	{
		if(AreThereAnyLivesLeft())
		{
			/* We opted to change our class after respawn, so do that now */
			if (psPtr->Class != psPtr->Cocoons)
			{
				psPtr->Class = psPtr->Cocoons;

				if (psPtr->Class == CLASS_CHESTBURSTER)
				{
					evolution = (ONE_FIXED*31);
					ChangedMyClass = 1;
				}
			}

			/* If alien lifecycle is enabled, respawn as a hugger, unless we have
			   just hugged someone */
			if (netGameData.LifeCycle && AvP.PlayerType == I_Alien)
			{
				if (!psPtr->ChestbursterTimer)
				{
					psPtr->Class = CLASS_EXF_W_SPEC;
					psPtr->Cocoons = CLASS_EXF_W_SPEC;
				}
				else
				{
					psPtr->ChestbursterTimer = 0;
					psPtr->Cocoons = CLASS_EXF_W_SPEC;
					//Bursted = TRUE;
				}
			}

			//check for change of character
			if(netGameData.myCharacterType!=netGameData.myNextCharacterType)
			{
				switch(netGameData.myNextCharacterType)
				{
					case (NGCT_Marine) :
						ChangeToMarine();
						break;

					case (NGCT_Alien) :
						ChangeToAlien();
						break;

					case (NGCT_Predator) :
						ChangeToPredator();
						break;

					default :
						GLOBALASSERT("dodgy character type"==0);
						break;
						
				}

				netGameData.myCharacterType=netGameData.myNextCharacterType;
			}
			else
			{
				/* CDF 15/3/99, delete all discs... */
				RemoveAllThisPlayersDiscs();

				NetPlayerRespawn(sbPtr);
			}

			/* dynamics block stuff... */
			{
				EULER zeroEuler = {0,0,0};
				VECTORCH zeroVec = {0,0,0};
				DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;

				dynPtr->Position = zeroVec;
				dynPtr->OrientEuler = zeroEuler;
				dynPtr->LinVelocity = zeroVec;
				dynPtr->LinImpulse = zeroVec;

				CreateEulerMatrix(&dynPtr->OrientEuler, &dynPtr->OrientMat);
				TransposeMatrixCH(&dynPtr->OrientMat);

				//Need to get rid of collisions for this frame , so player doesn't pick up
				//his dropped weapon when he respawns.
				dynPtr->CollisionReportPtr=0;
			}
			if (Bursted) // newly bursted alien players spawn at their present location.
				TeleportNetPlayerToAStartingPosition(sbPtr,5);
			else
				TeleportNetPlayerToAStartingPosition(sbPtr,0);
		}
		else
		{
			//no lives left , so have to act as an observer
			GetNextMultiplayerObservedPlayer();

			//The player's dropped weapon (if there was one) can now be drawn
			//MakePlayersWeaponPickupVisible();
			
		}
	}
	#endif
}

extern void InitPlayerCloakingSystem(void);
//make the player into new healthy character
void NetPlayerRespawn(STRATEGYBLOCK *sbPtr)
{
	extern int LeanScale;
	//SECTION *root_section;

	#if SupportWindows95
	PLAYER_STATUS *psPtr= (PLAYER_STATUS *) (sbPtr->SBdataptr);


	/* Turn on corpse. */
	if (psPtr->MyCorpse) {
		if (psPtr->MyCorpse->SBdptr) {
			psPtr->MyCorpse->SBdptr->ObFlags&=~ObFlag_NotVis;
		}
	}
	psPtr->MyCorpse=NULL;
	DeInitialisePlayer();
	psPtr->IsAlive = 1;
	/* When you're going to respawn... you might change */
	/* character class, after all. */
	InitialisePlayersInventory(psPtr);
    /* psPtr->Health=STARTOFGAME_MARINE_HEALTH; */
    /* psPtr->Armour=STARTOFGAME_MARINE_ARMOUR; */
	psPtr->MyFaceHugger=NULL;
    psPtr->Energy=STARTOFGAME_MARINE_ENERGY;
	   {
		NPC_DATA *NpcData;
		NPC_TYPES PlayerType;

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
				LeanScale=ONE_FIXED;

				#if 0  //this hmodel isn't being set up for the moment - Richard
				root_section=GetNamedHierarchyFromLibrary("hnpcmarine","Template");
				if (!root_section) {
					GLOBALASSERT(0);
					/* Sorry, there's just no bouncing back from this one.  Fix it. */
					return;
				}
				Create_HModel(&psPtr->HModelController,root_section);
				InitHModelSequence(&psPtr->HModelController,0,0,ONE_FIXED);
				/* Doesn't matter what the sequence is... */
				#endif
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
				LeanScale=ONE_FIXED;

				#if 0  //this hmodel isn't being set up for the moment - Richard
				root_section=GetNamedHierarchyFromLibrary("hnpcpredator","Template");
				if (!root_section) {
					GLOBALASSERT(0);
					/* Sorry, there's just no bouncing back from this one.  Fix it. */
					return;
				}
				Create_HModel(&psPtr->HModelController,root_section);
				InitHModelSequence(&psPtr->HModelController,0,0,ONE_FIXED);
				/* Doesn't matter what the sequence is... */
				#endif
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
				LeanScale=ONE_FIXED*3;

				#if 0  //this hmodel isn't being set up for the moment - Richard
				root_section=GetNamedHierarchyFromLibrary("hnpcalien","alien");
				if (!root_section) {
					GLOBALASSERT(0);
					/* Sorry, there's just no bouncing back from this one.  Fix it. */
					return;
				}
				Create_HModel(&psPtr->HModelController,root_section);
				InitHModelSequence(&psPtr->HModelController,0,0,ONE_FIXED);
				/* Doesn't matter what the sequence is... */
				#endif
				break;
			}
			default:
			{
				LOCALASSERT(1==0);
				break;
			}
		}

		NpcData = GetThisNpcData(PlayerType);
		LOCALASSERT(NpcData);
		sbPtr->SBDamageBlock.Health=NpcData->StartingStats.Health<<ONE_FIXED_SHIFT;
		sbPtr->SBDamageBlock.Armour=NpcData->StartingStats.Armour<<ONE_FIXED_SHIFT;			
		sbPtr->SBDamageBlock.SB_H_flags=NpcData->StartingStats.SB_H_flags;
		sbPtr->SBDamageBlock.IsOnFire=0;
	}
	
	psPtr->Encumberance.MovementMultiple=ONE_FIXED;
	psPtr->Encumberance.TurningMultiple=ONE_FIXED;
	psPtr->Encumberance.JumpingMultiple=ONE_FIXED;
	psPtr->Encumberance.CanCrouch=1;
	psPtr->Encumberance.CanRun=1;
	psPtr->Health=sbPtr->SBDamageBlock.Health;
	psPtr->Armour=sbPtr->SBDamageBlock.Armour;

	psPtr->ForwardInertia=0;
	psPtr->StrafeInertia=0; 
	psPtr->TurnInertia=0; 	
	psPtr->IsMovingInWater = 0;

	psPtr->incidentFlag=0;
	psPtr->incidentTimer=0;

	if (psPtr->soundHandle!=SOUND_NOACTIVEINDEX) {
 		Sound_Stop(psPtr->soundHandle);
	}
	if (psPtr->soundHandle3!=SOUND_NOACTIVEINDEX) {
 		Sound_Stop(psPtr->soundHandle3);
	}
	
	if (weaponHandle!=SOUND_NOACTIVEINDEX) {
 		Sound_Stop(weaponHandle);
	}

	if (predHUDSoundHandle!=SOUND_NOACTIVEINDEX) {
		Sound_Stop(predHUDSoundHandle);
	}

	if (predOVision_SoundHandle!=SOUND_NOACTIVEINDEX) {
		Sound_Stop(predOVision_SoundHandle);
	}

	//reset the player's elasticity (which gets altered upon death)
	sbPtr->DynPtr->Elasticity = 0;
	

	InitPlayerCloakingSystem();
		
	SetupVision();

    PlayerDamagedOverlayIntensity = 0;

	//no longer acting as an observer
	TurnOffMultiplayerObserveMode();
	
	//The player's dropped weapon (if there was one) can now be drawn
	//MakePlayersWeaponPickupVisible();
#endif
}


/* Patrick 9/7/97 ---------------------------------------------------
These two functions are used for testing the pre-processed AI 
locations... (either entry points or auxilary locs)
They teleport the player to the next location in the sequence, 
in response to the player pressing 'unused3' (currently the U key).
--------------------------------------------------------------------*/
#if PATTEST_EPS
static int pF_ModuleIndex = 0;
static int pF_EpIndex = 0;
static int pF_HaveStarted = 0;
static int pF_CanMove = 0;

void EpLocationTest(void)
{
	extern SCENE Global_Scene;
	extern SCENEMODULE **Global_ModulePtr;
	extern int ModuleArraySize;

	SCENEMODULE *ScenePtr;
	MODULE **moduleListPointer;
	DYNAMICSBLOCK *dynPtr = Player->ObStrategyBlock->DynPtr;
	MODULE *thisModulePtr;

	LOCALASSERT(Global_ModulePtr);
	ScenePtr = Global_ModulePtr[Global_Scene];
	moduleListPointer = ScenePtr->sm_marray;		

	if(PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_Unused3)
	{			
		if(pF_CanMove == 1)
		{
			/* move to the next one */
			pF_EpIndex++;
			if(pF_EpIndex >= FALLP_EntryPoints[pF_ModuleIndex].numEntryPoints)
			{
				pF_EpIndex=0;
				do
				{
					pF_ModuleIndex++;
					if(pF_ModuleIndex>=ModuleArraySize) pF_ModuleIndex = 0;
				}
				while(FALLP_EntryPoints[pF_ModuleIndex].numEntryPoints==0);
			}

			/* now move to the new location */
			thisModulePtr = moduleListPointer[pF_ModuleIndex];
			dynPtr->Position = FALLP_EntryPoints[pF_ModuleIndex].entryPointsList[(pF_EpIndex)].position;
			dynPtr->Position.vx += thisModulePtr->m_world.vx;
			dynPtr->Position.vy += thisModulePtr->m_world.vy;
			dynPtr->Position.vz += thisModulePtr->m_world.vz;

			dynPtr->PrevPosition = dynPtr->Position;	
			
			pF_HaveStarted = 1;
			pF_CanMove = 0;
		}			
	}
	else pF_CanMove = 1;
					
	if (pF_HaveStarted)
	{
		textprint("CURRENT FAR MODULE %d \n", pF_ModuleIndex);
		textprint("EP number %d from module %d \n", pF_EpIndex, FALLP_EntryPoints[pF_ModuleIndex].entryPointsList[(pF_EpIndex)].donorIndex);
	}	
}

#endif
#if PATTEST_AUXLOCS
static int pF_ModuleIndex = 0;
static int pF_AuxIndex = 0;
static int pF_HaveStarted = 0;
static int pF_CanMove = 0;

void AuxLocationTest(void)
{
	extern SCENE Global_Scene;
	extern SCENEMODULE **Global_ModulePtr;
	extern int ModuleArraySize;

	SCENEMODULE *ScenePtr;
	MODULE **moduleListPointer;
	DYNAMICSBLOCK *dynPtr=Player->ObStrategyBlock->DynPtr;
	MODULE *thisModulePtr;

	LOCALASSERT(Global_ModulePtr);
	ScenePtr = Global_ModulePtr[Global_Scene];
	moduleListPointer = ScenePtr->sm_marray;		

	/* dynPtr->GravityOn = 0; */

	if(PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_Unused3)
	{			
		if(pF_CanMove == 1)
		{
			/* move to the next one */
			pF_AuxIndex++;
			if(pF_AuxIndex >= FALLP_AuxLocs[pF_ModuleIndex].numLocations)
			{
				pF_AuxIndex=0;
				do
				{
					pF_ModuleIndex++;
					if(pF_ModuleIndex>=ModuleArraySize) pF_ModuleIndex = 0;
				}
				while(FALLP_AuxLocs[pF_ModuleIndex].numLocations==0);
			}

			/* now move to the new location */
			thisModulePtr = moduleListPointer[pF_ModuleIndex];
			dynPtr->Position = FALLP_AuxLocs[pF_ModuleIndex].locationsList[pF_AuxIndex];
			dynPtr->Position.vx += thisModulePtr->m_world.vx;
			dynPtr->Position.vy += thisModulePtr->m_world.vy;
			dynPtr->Position.vz += thisModulePtr->m_world.vz;
			dynPtr->Position.vy -= 1000;

			dynPtr->PrevPosition = dynPtr->Position;				
			pF_HaveStarted = 1;
			pF_CanMove = 0;
		}			
	}
	else pF_CanMove = 1;
					
	if (pF_HaveStarted)
	{
		textprint("CURRENT FAR MODULE %d \n", pF_ModuleIndex);
		textprint("AUX number %d \n", pF_AuxIndex);
	}	
}

#endif





/* KJL 10:34:54 8/5/97 - The alien can damage things by merely touching them 

   This will need work to get the values right - the damage done could be
   scaled by the alien's experience points, the relative velocities of the
   objects, and so on.
*/
#define ALIEN_CONTACT_WEAPON_DELAY 65536

#if ALIEN_CONTACT_WEAPON
static void AlienContactWeapon(void)
{
	COLLISIONREPORT *reportPtr = Player->ObStrategyBlock->DynPtr->CollisionReportPtr;
	static int contactWeaponTimer = 0;
	static int Speed;

	Speed = Approximate3dMagnitude(&Player->ObStrategyBlock->DynPtr->LinVelocity);

	if (contactWeaponTimer<=0)
	{
		contactWeaponTimer = ALIEN_CONTACT_WEAPON_DELAY;

		while (reportPtr) /* while there is a valid report */
		{
			if (reportPtr->ObstacleSBPtr)
			{
				switch(reportPtr->ObstacleSBPtr->I_SBtype)
				{
					case I_BehaviourMarinePlayer:
					case I_BehaviourPredatorPlayer:
					case I_BehaviourPredator:
					case I_BehaviourMarine:
					case I_BehaviourSeal:
					case I_BehaviourNetGhost:
					{
						// Must at least be running to cause damage
						if (Speed > 10000)
						{
							/* make alienesque noise */
							Sound_Play(SID_BODY_BEING_HACKED_UP_0,"h");

							/* damage unfortunate object */
							CauseDamageToObject(reportPtr->ObstacleSBPtr,&TemplateAmmo[AMMO_ALIEN_TAIL].MaxDamage[AvP.Difficulty],ONE_FIXED,NULL);
						}
						break;
					}
					default:
						break;
				}
			}								 
			/* skip to next report */
			reportPtr = reportPtr->NextCollisionReportPtr;
		}
	}
	else 
	{
		contactWeaponTimer -= NormalFrameTime;
	}

}
#endif

/* Demo code removed, CDF 28/9/98, by order of Kevin */

#if SupportWindows95 && LOAD_IN_MOVEMENT_VALUES	
static void LoadInMovementValues(void)
{

	FILE *fpInput;

	fpInput = fopen("movement.txt","rb");

	while(fgetc(fpInput) != '#');
	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&AlienForwardSpeed);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&AlienStrafeSpeed);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&AlienTurnSpeed);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&AlienJumpSpeed);

	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&PredatorForwardSpeed);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&PredatorStrafeSpeed);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&PredatorTurnSpeed);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&PredatorJumpSpeed);

	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&MarineForwardSpeed);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&MarineStrafeSpeed);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&MarineTurnSpeed);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput, "%d",&MarineJumpSpeed);

	fclose(fpInput);
}
#endif

extern void ThrowAFlare(void)
{
  PLAYER_STATUS *playerStatusPtr;
  extern int NumberOfFlaresActive;
  playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
  GLOBALASSERT(playerStatusPtr);

  if (NumberOfFlaresActive<4) {
	extern VECTORCH CentreOfMuzzleOffset;
	extern VIEWDESCRIPTORBLOCK *ActiveVDBList[];
	VIEWDESCRIPTORBLOCK *VDBPtr = ActiveVDBList[0];
	MATRIXCH mat = VDBPtr->VDB_Mat;
	VECTORCH position = VDBPtr->VDB_World;

	TransposeMatrixCH(&mat);

	if (!playerStatusPtr->FlaresLeft) return;
	if (GrabbedPlayer) return;
	//if (playerStatusPtr->OnSurface) return;

	CreateGrenadeKernel(I_BehaviourFlareGrenade,&position,&mat,0);
	Sound_Play(SID_THROW_FLARE,"d",&(Player->ObStrategyBlock->DynPtr->Position));
	playerStatusPtr->FlaresLeft--;
  }
}