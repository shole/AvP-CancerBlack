/*KJL****************************************************************************************
*                                         	hud.c                                           *
****************************************************************************************KJL*/


/*
	functions for drawing the HUD, processing sounds and
	Marine Hud contains the information for drawing stuff to the
	HUD. MarineWeaponHud contains he various information needed
	to copy Weapon data into the HUD
*/
#include "3dc.h"
#include "module.h"
#include "inline.h"


#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"
#include "comp_shp.h"
#include "huddefs.h"
#include "dynblock.h"
#include "weapons.h"
#include "hud_map.h"

#include "psnd.h"
#include "psndplat.h"
#include "dynamics.h"

#include "particle.h"
#include "gadget.h"
#include "lighting.h"
#include "d3d_hud.h"
#include "frustrum.h"
#include "pldghost.h"

#include "d3d_render.h"
#include "bh_ais.h"
#include "bh_alien.h"

#define UseLocalAssert Yes
#include "ourasert.h"

#include "vision.h"
#include "BonusAbilities.h"
#include "avp_menus.h"
#include "showcmds.h"
#include "game_statistics.h"
#include "psndplat.h"
#include "pldnet.h"
#include "avp_userprofile.h"
#include "equipmnt.h"

#include "HUD_layout.h"

#include "bh_marin.h"
#include "bh_corpse.h"

extern int ScanDrawMode;

#define DO_PREDATOR_OVERLAY No
#define DO_ALIEN_OVERLAY No

#define DRAW_HUD Yes

#define HUD_WEAPON_SELECT_LIST_DURATION 1
/*KJL****************************************************************************************
*  										G L O B A L S 	            					    *
****************************************************************************************KJL*/


extern DISPLAYBLOCK* Player;

extern int sine[], cosine[]; /* these externs should be with the GetCos GetSin macros!! */

extern int NumActiveBlocks;
extern DISPLAYBLOCK *ActiveBlockList[];
extern int NumOnScreenBlocks;
extern DISPLAYBLOCK *OnScreenBlockList[];
extern int NumActiveStBlocks;
extern STRATEGYBLOCK *ActiveStBlockList[maxstblocks];

extern DPID HuggedBy;

extern void RenderString(char *stringPtr, int x, int y, int colour);
extern void SecondFlushD3DZBuffer(void);
extern void RenderInsideAlienTongue(int offset);
extern void D3D_PlayerDamagedOverlay(int intensity);
extern void RenderBriefingText(int centreY, int brightness);
extern void DoFailedLevelStatisticsScreen(void);
extern void DoStatisticsScreen(int completed_level);
extern void RenderStringCentred(char *stringPtr, int centreX, int y, int colour);
extern void D3D_DrawHUDPredatorHealth(HUDCharDesc *charDescPtr, int scale);
extern void D3D_DrawHUDPredatorEnergy(HUDCharDesc *charDescPtr, int scale);
extern void D3D_DrawHUDPredatorDigit(HUDCharDesc *charDescPtr, int scale);
extern void D3D_DrawHUDPredatorNetDart(HUDCharDesc *charDescPtr, int scale);
extern void RenderPredatorTargetingSegment(int theta, int scale, int drawInRed);
extern void Draw_HUDImage(HUDImageDesc *imageDescPtr);
extern void RenderPredatorPlasmaCasterCharge(int value, VECTORCH *worldOffsetPtr, MATRIXCH *orientationPtr);
extern int GetLoadedShapeMSL(const char *shapename);
extern void DrawAPCOverlay(int t);
extern void BLTSonarToHUD(int scanLineSize);
extern void BLTSonarBlipToHUD(int x, int y, int brightness, int color);

extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern int NormalFrameTime;
extern int ERE_Broken(void);
extern int Underwater;
extern int Operable;
extern int NoOperateImageNumber;
extern int OperateImageNumber;
extern int ComTechOperateImageNumber;
extern int GrabAttackInProgress;
extern int ThirdPersonActive;

extern void ShowTeamNames_Marine();
extern void ShowTeamNames_Alien();
extern void ShowTeamNames_Predator();
extern void BLTWaveformToHUD(int type);
extern void BLTIconsToHUD(void);

extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern signed int RequestFadeToBlackLevel;
extern ACTIVESOUNDSAMPLE ActiveSounds[];

extern int SmartTargetSightX, SmartTargetSightY;
extern char CurrentlySmartTargetingObject;
extern DISPLAYBLOCK *SmartTarget_Object;
extern DISPLAYBLOCK *Old_SmartTarget_Object;

/* In 16.16 for smoothness. On-screen coords for the smart targeting system's sight */

int GunMuzzleSightX, GunMuzzleSightY;
/* In 16.16 for smoothness. On-screen coords indicating to where the gun's muzzle is pointing */

/* motion tracker info */
static int MTScanLineSize=MOTIONTRACKER_SMALLESTSCANLINESIZE;
static int PreviousMTScanLineSize=MOTIONTRACKER_SMALLESTSCANLINESIZE;
static int MTDelayBetweenScans=0;
static BLIP_TYPE MotionTrackerBlips[MOTIONTRACKER_MAXBLIPS];
static SONAR_BLIP_TYPE SonarBlips[MOTIONTRACKER_MAXBLIPS];
static int NoOfMTBlips=0;
static int MTSoundHandle=SOUND_NOACTIVEINDEX;
int predHUDSoundHandle=SOUND_NOACTIVEINDEX;

static int HUD_PrimaryRounds;
static int HUD_SecondaryRounds;
/* numerics buffer - the marine has more digits on his HUD than the other species */
char ValueOfHUDDigit[MAX_NO_OF_COMMON_HUD_DIGITS];


#define PREDATOR_LOCK_ON_TIME (ONE_FIXED*5/3)
#define PREDATOR_LOCK_ON_SPEED (3)
static int PredSight_LockOnTime;
static int PredSight_Angle;

int AlienTeethOffset;
int AlienTongueOffset;

int DrawCompanyLogos;
int LogosAlphaLevel;
int PlayerDamagedOverlayIntensity;

int FadingGameInAfterLoading;
int Wavenum;

int MotionTrackerSpeed = ONE_FIXED*4;
int MotionTrackerVolume = ONE_FIXED;
#define MOTIONTRACKERVOLUME (MUL_FIXED(VOLUME_MAX,MotionTrackerVolume))

int CameraZoomLevel;
extern float CameraZoomScale;
static int DrawScanlineOverlay;
static float ScanlineLevel;

/*KJL****************************************************************************************
*                                    P R O T O T Y P E S	                                *
****************************************************************************************KJL*/
//extern void SmartTarget(int speed);
extern void SmartTarget(int speed,int projectile_speed);

extern void PlatformSpecificKillMarineHUD(void);
extern void PlatformSpecificKillAlienHUD(void);
extern void PlatformSpecificKillPredatorHUD(void);
extern void DrawScanlinesOverlay(float level);
extern void RenderThisDisplayblock(DISPLAYBLOCK *dbPtr);
extern BYTE GetWeaponIconFromAmmoId(enum WEAPON_ID AmmoId);
extern void BLTTargetingSightToScreen(int screenX, int screenY);

void DisplayPredatorHealthAndEnergy(void);

void InitHUD();
static void InitMarineHUD();
static void InitAlienHUD();
//static void CalcCoordsAndBLTWeapon(WEAPON_DATA* wptr);
static int HUD_WeaponSelectListTimer = 0;
static int DoWaveformScan();

void MaintainHUD(void);
void DrawAMPstuff();

static void DisplayHealthAndArmour(void);
static void DisplayMarinesAmmo(void);


static void DoMotionTracker(void);
static void DoSonar(void);
static void DoWaveform(int type);
static void DoPredatorThreatDisplay(void);
static int DoMotionTrackerBlips(void);
static int DoSonarBlips(void);
static void UpdateMarineStatusValues(void);

static void HandleMarineWeapon(void);
static void AimGunSight(int aimingSpeed, TEMPLATE_WEAPON_DATA *twPtr);
static void DrawMarineSights(void);
static void DrawPredatorSights(void);
void DrawWristDisplay(void);
static void DrawAlienTeeth(void);
static void DrawAMPSpecificWeapons(void);
static void RenderFacehuggerBelly(void);
static void RenderNetting(void);
static void Render_WeaponSelectList ();
static BOOL	ConstructWeaponSelectList ();

void CentreGunSight(void);

// Temporary!
int AMPGunX;
int AMPGunY;
int AMPGunZ;
int AMPGunEX;
int AMPGunEY;
int AMPGunEZ;

static void InitPredatorHUD();
static int FindPredatorThreats(void);
#if DO_PREDATOR_OVERLAY
static void UpdatePredatorStatusValues(void);
#endif
static void HandlePredatorWeapon(void);

static void HandleAlienWeapon(void);
#if DO_ALIEN_OVERLAY
static void UpdateAlienStatusValues(void);
#endif

int Fast2dMagnitude(int dx, int dy);


void RotateVertex(VECTOR2D *vertexPtr, int theta);

/*KJL****************************************************************************************
*                                     F U N C T I O N S	                                    *
****************************************************************************************KJL*/
void InitHUD(void)						 
{
	switch(AvP.PlayerType)
	{
		case I_Marine:
  	 		InitMarineHUD();
			break;
       	case I_Predator:
			InitPredatorHUD();
			break;
		case I_Alien:
			InitAlienHUD();
			break;

		default:
			LOCALASSERT(1==0);
			break;
	}
	
	// This should be set elsewhere as well, but just to be sure!!
	RequestFadeToBlackLevel = 0;

	/* KJL 11:18:36 04/25/97 - initialise HUD map code */
	//InitHUDMap();
}
void KillHUD(void)
{
	switch(AvP.PlayerType)
	{
		case I_Marine:
			PlatformSpecificKillMarineHUD();
			break;
       	case I_Predator:
			PlatformSpecificKillPredatorHUD();
			break;
		case I_Alien:
			PlatformSpecificKillAlienHUD();
			break;

		default:
			LOCALASSERT(1==0);
			break;
	}
}
void InitMarineHUD(void)
{
	/*KJL****************************************************************************************
	* Okay. From now on everyone will call the fn below which loads and initialises ALL the gfx *
	* required for a marine, eg. weapons, motion tracker stuff, gun sights, et al.              *
	****************************************************************************************KJL*/
	PlatformSpecificInitMarineHUD();
	
	SmartTarget_Object=NULL;
	Old_SmartTarget_Object=NULL;

	{
		int i;
		for (i=0; i<MAX_NO_OF_MARINE_HUD_DIGITS; i++)
			ValueOfHUDDigit[i]=0;
	}
	
	/* Start the gun sight at the centre of the screen */
	/* SmartTargetSight in 16.16 coords, hence the shift up by 15 */
	SmartTargetSightX = (ScreenDescriptorBlock.SDB_Width<<15);
	SmartTargetSightY = (ScreenDescriptorBlock.SDB_Height<<15);

	GunMuzzleSightX = (ScreenDescriptorBlock.SDB_Width<<15);
	GunMuzzleSightY = (ScreenDescriptorBlock.SDB_Height<<15);

	HUD_PrimaryRounds = 0;
	HUD_SecondaryRounds = 0;
	
}


static void InitAlienHUD(void)
{
	PlatformSpecificInitAlienHUD();
	AlienTeethOffset = 0;
	AlienTongueOffset = 0;

	SmartTarget_Object=NULL;
	Old_SmartTarget_Object=NULL;

}

extern void ReInitHUD(void)
{
	/* KJL 14:21:33 17/11/98 - Alien */
	AlienTeethOffset = 0;
	AlienTongueOffset = 0;


	/* KJL 14:21:48 17/11/98 - Marine */
	/* Start the gun sight at the centre of the screen */
	/* SmartTargetSight in 16.16 coords, hence the shift up by 15 */
	SmartTargetSightX = (ScreenDescriptorBlock.SDB_Width<<15);
	SmartTargetSightY = (ScreenDescriptorBlock.SDB_Height<<15);

	GunMuzzleSightX = (ScreenDescriptorBlock.SDB_Width<<15);
	GunMuzzleSightY = (ScreenDescriptorBlock.SDB_Height<<15);

	HUD_PrimaryRounds = 0;
	HUD_SecondaryRounds = 0;
	
	/* KJL 14:21:54 17/11/98 - Predator */
	InitialiseGrapplingHook();
}

char GetEqIcon(int eq)
{
	switch(eq)
	{
		case 1:
			return ICON_EQ_HANDGRENADE;
			break;
		case 2:
			return ICON_EQ_PGC;
			break;
		case 3:
			return ICON_EQ_MEDIKIT;
			break;
		case 4:
			return ICON_EQ_MT;
			break;
		case 5:
			return ICON_EQ_II;
			break;
		case 6:
			return ICON_EQ_SENTRY;
			break;
		case 7:
			return ICON_EQ_HANDGRENADE + 16;
			break;
		case 8:
			return ICON_EQ_HANDGRENADE + 17;
			break;

	}
	return 0;
}

void DrawAMPHud(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	extern float CameraZoomScale;
	char text[200];
	char eqicon;

	GLOBALASSERT(playerStatusPtr);

	if (AvP.PlayerType!=I_Marine) return;

	/* Equipment HUD */
	if (playerStatusPtr->MTrackerType)
	{
		eqicon = GetEqIcon(4);
		sprintf(text,"%c", eqicon);
		RenderString(text,5,60,0xffffffff);
	}
	if (playerStatusPtr->IHaveAPlacedAutogun)
	{
		eqicon = GetEqIcon(6);
		sprintf(text,"%c", eqicon);
		RenderString(text,5,80,0xffffffff);
	}
	if (playerStatusPtr->Medikit)
	{
		eqicon = GetEqIcon(3);
		sprintf(text,"%c", eqicon);
		RenderString(text,25,80,0xffffffff);
	}
	if (playerStatusPtr->Grenades)
	{
		if (playerStatusPtr->Class == CLASS_INC_SPEC)
		{
			eqicon = GetEqIcon(7);
			sprintf(text,"%c", eqicon);
			RenderString(text,5,100,0xffffffff);
		} else if (playerStatusPtr->Class == CLASS_AA_SPEC) {
			eqicon = GetEqIcon(8);
			sprintf(text,"%c", eqicon);
			RenderString(text,5,100,0xffffffff);
		} else {
			eqicon = GetEqIcon(1);
			sprintf(text,"%c", eqicon);
			RenderString(text,5,100,0xffffffff);
		}
	}
	if (playerStatusPtr->Grenades)
	{
		sprintf(text,"%d", playerStatusPtr->Grenades);
		RenderString(text,30,100,0xffffffff);
	}
}

void DrawAPCHud(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	
	DrawAPCOverlay(255);
}

/* KJL 16:27:39 09/20/96 - routine which handles all HUD activity */
void MaintainHUD(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

//	RenderSmokeTest();
	PlatformSpecificEnteringHUD();

	if (HUD_WeaponSelectListTimer > 0) {
		HUD_WeaponSelectListTimer -= NormalFrameTime;
	}
	if (ScanDrawMode != ScanDrawDirectDraw)
	{
		HandleParticleSystem();
	}
	RenderGrapplingHook();
	
	#if SOFTWARE_RENDERER
	FlushSoftwareZBuffer();
	#else
	SecondFlushD3DZBuffer();
	#endif
	//DrawFontTest();
	if (Observer)
	{
		switch(AvP.PlayerType)
		{
			case I_Marine:
			{
		  		HandleMarineOVision();
				break;
			}
	       	case I_Predator:
			{
  				HandlePredOVision();
				break;
			}
			case I_Alien:
			{
				HandleAlienOVision();
				break;
			}
			default:
				break;
		}
		CheckWireFrameMode(0);
		#if 1||!PREDATOR_DEMO
		GADGET_Render();
		#endif
		return;
	}
	
//	GlobalAmbience=16384;

	/* KJL 18:46:04 03/10/97 - for now I've completely turned off the HUD if you die; this
	can easily be changed */    	
	#if DRAW_HUD
	if (playerStatusPtr->MyFaceHugger!=NULL)
	{
		/* YUCK! */
		extern void PlotFaceHugger(STRATEGYBLOCK *sbPtr);
		PlotFaceHugger(playerStatusPtr->MyFaceHugger);
		
	}
	else if (playerStatusPtr->IsAlive)
	{	   
		/* switch on player type */
		switch(AvP.PlayerType)						
		{
			case I_Marine:
			{
				extern float CameraZoomScale;

				if (playerStatusPtr->Honor)
				{
					DrawMarineSights();
					DrawAMPSpecificWeapons();
					//DrawAPCHud();
				}
				if (!playerStatusPtr->Honor) 
				{
					if (!ThirdPersonActive)
						DrawAMPSpecificWeapons();

					if (CameraZoomScale == 1.0f)
					{
						if (!ThirdPersonActive)
						{
							HandleMarineWeapon();
						}
					}

					if (HUD_WeaponSelectListTimer > 0) {
						Render_WeaponSelectList();
					}
					if (DrawScanlineOverlay) DrawScanlinesOverlay(ScanlineLevel);
					if ((playerStatusPtr->MTrackerType >= 2) && (playerStatusPtr->MTrackerType != 5) &&
						(CameraZoomScale == 1.0f))
						DoMotionTracker();
					CheckWireFrameMode(0);
					if ((AvP.Network==I_No_Network) || ((AvP.Network != I_No_Network) &&
						(playerStatusPtr->Class != CLASS_NONE) && 
						(playerStatusPtr->Class != 20))) {
						//flash health if invulnerable
						if (CameraZoomScale == 1.0f) {
							if((playerStatusPtr->invulnerabilityTimer/12000 %2)==0)
							{
								DisplayHealthAndArmour();
							}
							DisplayMarinesAmmo();
						}
						DrawAMPHud();
						DrawMarineSights();
						ShowTeamNames_Marine();
						BLTIconsToHUD();
					}
					/* Paranoia check. */
		  			if(predHUDSoundHandle != SOUND_NOACTIVEINDEX) {
		       			Sound_Stop(predHUDSoundHandle);
					}
				}
				break;
			}
	       	case I_Predator:
			{
				HandlePredatorWeapon();

				if (!ThirdPersonActive)
				{
					DrawAMPSpecificWeapons();
				}
				if (HUD_WeaponSelectListTimer > 0) {
					Render_WeaponSelectList();
				}
				CheckWireFrameMode(0);
				//DrawPredatorEnergyBar();						
	  			//DisplayHealthAndArmour();

				if (!ThirdPersonActive)
				   	DrawWristDisplay();
  				
  				HandlePredOVision();
				if (DrawScanlineOverlay) DrawScanlinesOverlay(ScanlineLevel);

				if ((AvP.Network==I_No_Network) || ((AvP.Network != I_No_Network) &&
					(playerStatusPtr->Class != CLASS_NONE) &&
					(playerStatusPtr->Class != 20))) {
			   		DrawPredatorSights();
					ShowTeamNames_Predator();
					//flash health if invulnerable
					if((playerStatusPtr->invulnerabilityTimer/12000 %2)==0)
					{
	  					DisplayPredatorHealthAndEnergy();
					}

					//Waveform Analyzer (close proximity sensor)
					if (LocalDetailLevels.ExplosionsDeformToEnvironment)
					{
						if (DoWaveformScan())
						{
							if (Wavenum<=5) {
								DoWaveform(0);
								Wavenum++;
							} else if (Wavenum<=10) {
								DoWaveform(1);
								Wavenum++;//=1;
							} else if (Wavenum<=12) {
								DoWaveform(2);
								Wavenum++;//=2;
							} else if (Wavenum<=14) {
								DoWaveform(3);
								Wavenum++;//=0;
							} else if (Wavenum<=16) {
								Wavenum=0;
							}
						} else {
							DoWaveform(0);
							if (Wavenum!=0) Wavenum=0;
						}
					} else {
						if (DoWaveformScan())
						{
							DoWaveform(1);
						} else {
							DoWaveform(0);
						}
					}
				}
				break;
			}
			case I_Alien:
			{
				if (!ThirdPersonActive)
					DrawAlienTeeth();

				//DoSonar();
				if (AlienTongueOffset)
				{
					RenderInsideAlienTongue(AlienTongueOffset);
					AlienTongueOffset-=NormalFrameTime;
					if (AlienTongueOffset<0)
						AlienTongueOffset = 0;
				}				
				SetFrustrumType(FRUSTRUM_TYPE_NORMAL);
				Global_VDB_Ptr->VDB_ProjX = (Global_VDB_Ptr->VDB_ClipRight - Global_VDB_Ptr->VDB_ClipLeft)/2;
				Global_VDB_Ptr->VDB_ProjY = (Global_VDB_Ptr->VDB_ClipDown - Global_VDB_Ptr->VDB_ClipUp)/2;
	
				if (!ThirdPersonActive)
					HandleAlienWeapon();

				HandleAlienOVision();

				SetFrustrumType(FRUSTRUM_TYPE_WIDE);
				Global_VDB_Ptr->VDB_ProjX = (Global_VDB_Ptr->VDB_ClipRight - Global_VDB_Ptr->VDB_ClipLeft)/4;
				Global_VDB_Ptr->VDB_ProjY = (Global_VDB_Ptr->VDB_ClipDown - Global_VDB_Ptr->VDB_ClipUp)/4;
							
				CheckWireFrameMode(0);

				if ((AvP.Network==I_No_Network) || ((AvP.Network != I_No_Network) &&
					(playerStatusPtr->Class != CLASS_NONE) &&
					(playerStatusPtr->Class != 20))) {
					//flash health if invulnerable
					if((playerStatusPtr->invulnerabilityTimer/12000 %2)==0)
					{
						DisplayHealthAndArmour();
					}
					ShowTeamNames_Alien();
				}
				/* Paranoia check. */
		  		if(predHUDSoundHandle != SOUND_NOACTIVEINDEX) {
		       		Sound_Stop(predHUDSoundHandle);
				}
				break;
			}
			default:
				LOCALASSERT(1==0);
				break;
		}
	}
	else // player is dead!
	{
		switch(AvP.PlayerType)
		{
			case I_Marine:
			{
				break;
			}
	       	case I_Predator:
			{
  				HandlePredOVision();
				break;
			}
			case I_Alien:
			{
				HandleAlienOVision();
				break;
			}
			default:
				break;
		}
	}

	#endif

	CheckWireFrameMode(0);

	{
		extern int HeadUpDisplayZOffset;
		HeadUpDisplayZOffset = 0;

		#if 1||!PREDATOR_DEMO
		GADGET_Render();
		#endif
		
		switch(AvP.PlayerType)
		{
			case I_Marine:
			{
		  		HandleMarineOVision();
				break;
			}
	       	case I_Predator:
			{
  	//			HandlePredOVision();
				break;
			}
			default:
				break;
		}

	}
	{
		extern int AlienBiteAttackInProgress;
		if (AlienBiteAttackInProgress)
		{
			extern void D3D_FadeDownScreen(int brightness, int colour);
			int b;
			if (CameraZoomScale!=0.25f)
			{
				f2i(b,CameraZoomScale*65536.0f);
				if (b<32768) b=32768;
				D3D_FadeDownScreen(b,0xff0000);
			}
		}
	}

	// Hidden aliens have a darker HUD.
	{
		if (playerStatusPtr->cloakOn && (AvP.PlayerType == I_Alien))
			D3D_FadeDownScreen(32768,0x000000);
	}
	// burn baby burn?
	if (Player->ObStrategyBlock->SBDamageBlock.IsOnFire)
	{
		D3D_PlayerOnFireOverlay();
	}
	else
	{
		
		extern int PlayerDamagedOverlayIntensity;
		int intensity = PlayerDamagedOverlayIntensity>>12;
//		if (intensity>255) intensity = 255;
		if (intensity>128) intensity = 128;
		
		if (intensity)
		{
			D3D_PlayerDamagedOverlay(intensity);
		}

		PlayerDamagedOverlayIntensity -= NormalFrameTime<<5;
		if (PlayerDamagedOverlayIntensity<0) PlayerDamagedOverlayIntensity=0;
	}
	{
		#if 0
		if(DrawCompanyLogos && LogosAlphaLevel)
		{
			extern void D3D_DrawRebellionLogo(unsigned int alpha);
			
			if (LogosAlphaLevel<ONE_FIXED)
			{
				D3D_DrawRebellionLogo(LogosAlphaLevel);
			}
			else
			{
				D3D_DrawRebellionLogo(0xff00);
			}
			
			LogosAlphaLevel-=NormalFrameTime;
			if (LogosAlphaLevel<0) LogosAlphaLevel=0;
		}
		#endif
		if(FadingGameInAfterLoading)
		{
			extern void D3D_FadeDownScreen(int brightness, int colour);
			extern int RealFrameTime;
			D3D_FadeDownScreen(ONE_FIXED-FadingGameInAfterLoading, 0);
			
			RenderBriefingText(ScreenDescriptorBlock.SDB_Height/2,FadingGameInAfterLoading);

			FadingGameInAfterLoading-=RealFrameTime/2;
			if (FadingGameInAfterLoading<0) FadingGameInAfterLoading = 0;
		}
	}
	if(!playerStatusPtr->IsAlive)
	{
		if (AvP.Network==I_No_Network)
		{
			DoFailedLevelStatisticsScreen();
		}
		else
		{
			// place multiplayer stuff here - e.g. full scores/frags et al
		}
	}
    if(AvP.Network != I_No_Network)
	{
		DoMultiplayerSpecificHud();
	}
}	

extern void DoCompletedLevelStatisticsScreen(void)
{
	extern int DebouncedGotAnyKey;
	extern unsigned char DebouncedKeyboardInput[];
	extern void ShowSplashScreens(unsigned int screen);
	if (DebouncedKeyboardInput[KEY_ESCAPE])
	{	
		AvP.RestartLevel = 1;
	}
	else if (DebouncedGotAnyKey)
	{
		AvP.MainLoopRunning = 0;
	}
	//ShowSplashScreens(0);
	D3D_FadeDownScreen(0,0);
	DoStatisticsScreen(1);
	RenderStringCentred(GetTextString(TEXTSTRING_COMPLETEDLEVEL_PRESSAKEY),ScreenDescriptorBlock.SDB_Width/2,ScreenDescriptorBlock.SDB_Height-20,0xffffffff);
}

static void DoWaveform(int type)
{
	BLTWaveformToHUD(type);
}

/*   This function scans through the active block list looking for dynamic objects
   which are detectable	by the Motion Tracker.
*/
static void DoMotionTracker(void)
{
 	static char distanceNotLocked=1;
	static int distance=0;
	
	/* draw static motion tracker background, and the moving scanline */
	BLTMotionTrackerToHUD(MTScanLineSize);
  	
	if(distanceNotLocked) /* if MT hasn't found any contacts this scan */
	{
		int nearestDistance=DoMotionTrackerBlips();
		
   		if (nearestDistance<MOTIONTRACKER_RANGE) /* if picked up some blips */
		{
			distance=nearestDistance;
			distanceNotLocked=0;

			if (MTSoundHandle==SOUND_NOACTIVEINDEX)
			{
				int panicFactor = MUL_FIXED(nearestDistance,MOTIONTRACKER_SCALE);
				if (panicFactor < 21845)
				{
					Sound_Play(SID_TRACKER_WHEEP_HIGH,"dev",&(Player->ObStrategyBlock->DynPtr->Position),&MTSoundHandle,MOTIONTRACKERVOLUME);
					if (AvP.Network != I_No_Network)
					{
						if (!netGameData.trackerNoise)
							netGameData.trackerNoise=3;
					}
				}
				else if (panicFactor < 21845*2)
				{
					Sound_Play(SID_TRACKER_WHEEP,"dev",&(Player->ObStrategyBlock->DynPtr->Position),&MTSoundHandle,MOTIONTRACKERVOLUME);
					if (AvP.Network != I_No_Network)
					{
						if (!netGameData.trackerNoise)
							netGameData.trackerNoise=2;
					}
				}
				else
				{
					Sound_Play(SID_TRACKER_WHEEP_LOW,"dev",&(Player->ObStrategyBlock->DynPtr->Position),&MTSoundHandle,MOTIONTRACKERVOLUME);
					if (AvP.Network != I_No_Network)
					{
						if (!netGameData.trackerNoise)
							netGameData.trackerNoise=4;
					}
				}
			}

			// Do motion tracker pitch change?

			#if 0 // no - it sounds fucking awful
			if (MTSoundHandle!=SOUND_NOACTIVEINDEX)			
			{
				int panicFactor = 65536 - MUL_FIXED(nearestDistance,MOTIONTRACKER_SCALE);
				LOCALASSERT(panicFactor>=0);
				panicFactor>>=8;              // Scale to 0-256
				 
				PlatChangeSoundPitch(MTSoundHandle,panicFactor);

			}
			#endif
				
		}
		else if (NoOfMTBlips==0) /* if the MT is blank, cycle the distance digits */
		{
			distance= MUL_FIXED(MTScanLineSize,MOTIONTRACKER_RANGE);
 		}
	}
	else DoMotionTrackerBlips();
	
	/* evaluate the distance digits */
	{
    	int value=distance/10;
        ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_UNITS]=value%10;
		
		value/=10;						  
        ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_TENS]=value%10;
        
        value/=10;
		ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_HUNDREDS]=value%10;
		
		value/=10;
      	ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_THOUSANDS]=value%10;
	}
   
	if (MTDelayBetweenScans)
	{
		MTDelayBetweenScans-=NormalFrameTime;
		if (MTDelayBetweenScans<0) 
		{
			MTDelayBetweenScans=0;
		
			Sound_Play(SID_TRACKER_CLICK,"v",MOTIONTRACKERVOLUME);
			if (AvP.Network != I_No_Network)
			{
				if (!netGameData.trackerNoise)
					netGameData.trackerNoise=1;
			}

			PreviousMTScanLineSize =MTScanLineSize=MOTIONTRACKER_SMALLESTSCANLINESIZE;
			distanceNotLocked=1; /* allow MT to look for a new nearest contact distance */
		}
	}
	else
	{
		/* expand scanline or wrap it around */
		PreviousMTScanLineSize=MTScanLineSize;
		
		if (MTScanLineSize>=65536)
		{	 		
			MTDelayBetweenScans=65536;
			MTScanLineSize=0;
		}
		else if (MTScanLineSize>32768) 
		{
			MTScanLineSize+= MUL_FIXED(MOTIONTRACKER_SPEED*2,NormalFrameTime);
		}
		else
		{
			MTScanLineSize+= MUL_FIXED(MOTIONTRACKER_SPEED,NormalFrameTime);
		}	
		
		if (MTScanLineSize>65536)
		{
			MTScanLineSize=65536;
		}
	}
	
	/* draw blips to HUD */
	{
		DYNAMICSBLOCK *playerDynPtr = Player->ObStrategyBlock->DynPtr;
		int phi = playerDynPtr->OrientEuler.EulerY;
		int cosPhi = MUL_FIXED(GetCos(phi),MOTIONTRACKER_SCALE);
		int sinPhi = MUL_FIXED(GetSin(phi),MOTIONTRACKER_SCALE);
    	int i;
    	/* made more awkward because we want to draw the brightest last */
		i=0;

		while(i<NoOfMTBlips)
		{
			int y = MotionTrackerBlips[i].Y-playerDynPtr->Position.vz;
		  	int x = MotionTrackerBlips[i].X-playerDynPtr->Position.vx;
			int y2 = MUL_FIXED(x,sinPhi) + MUL_FIXED(y,cosPhi);
			
			if (y2>=0)
			{
				x = MUL_FIXED(x,cosPhi) - MUL_FIXED(y,sinPhi);

				if(Fast2dMagnitude(x,y2)<ONE_FIXED)
				{				
					BLTMotionTrackerBlipToHUD
					(
						x,
				   		y2,
						MotionTrackerBlips[i].Brightness
					);
				}
			}
            i++;
		}
        
		/* now fade or kill blips */
		i = NoOfMTBlips;
		while(i--) /* scan through all blips, starting with the last in the list */
		{
			/* decrease blip's brightness */
			MotionTrackerBlips[i].Brightness-=MUL_FIXED(MOTIONTRACKER_SPEED/3,NormalFrameTime);
			if (MotionTrackerBlips[i].Brightness<0)	/* then kill blip */
			{
				/* Kill ith blip by copying the last blip in the list over the ith
				   and decreasing the number of blips */
				NoOfMTBlips--;
				MotionTrackerBlips[i] = MotionTrackerBlips[NoOfMTBlips];
			} 
		}
	}

	return;
}

static void DoSonar(void)
{
 	static char distanceNotLocked=1;
	static int distance=0;
	
	/* draw static motion tracker background, and the moving scanline */
	BLTSonarToHUD(MTScanLineSize);
  	
	if(distanceNotLocked) /* if MT hasn't found any contacts this scan */
	{
		int nearestDistance=DoSonarBlips();
		
   		if (nearestDistance<MOTIONTRACKER_RANGE) /* if picked up some blips */
		{
			distance=nearestDistance;
			distanceNotLocked=0;
		}
		else if (NoOfMTBlips==0) /* if the MT is blank, cycle the distance digits */
		{
			distance= MUL_FIXED(MTScanLineSize,MOTIONTRACKER_RANGE);
 		}
	}
	else DoSonarBlips();
   
	if (MTDelayBetweenScans)
	{
		MTDelayBetweenScans-=NormalFrameTime;
		if (MTDelayBetweenScans<0) 
		{
			MTDelayBetweenScans=0;

			PreviousMTScanLineSize =MTScanLineSize=MOTIONTRACKER_SMALLESTSCANLINESIZE;
			distanceNotLocked=1; /* allow MT to look for a new nearest contact distance */
		}
	}
	else
	{
		/* expand scanline or wrap it around */
		PreviousMTScanLineSize=MTScanLineSize;
		
		if (MTScanLineSize>=65536)
		{	 		
			MTDelayBetweenScans=65536;
			MTScanLineSize=0;
		}
		else if (MTScanLineSize>32768) 
		{
			MTScanLineSize+= MUL_FIXED(MOTIONTRACKER_SPEED*2,NormalFrameTime);
		}
		else
		{
			MTScanLineSize+= MUL_FIXED(MOTIONTRACKER_SPEED*2,NormalFrameTime);
		}	
		
		if (MTScanLineSize>65536)
		{
			MTScanLineSize=65536;
		}
	}

	/* draw blips to HUD */
	{
		DYNAMICSBLOCK *playerDynPtr = Player->ObStrategyBlock->DynPtr;
		int phi = playerDynPtr->OrientEuler.EulerY;
		int cosPhi = MUL_FIXED(GetCos(phi),MOTIONTRACKER_SCALE);
		int sinPhi = MUL_FIXED(GetSin(phi),MOTIONTRACKER_SCALE);
    	int i;
		int blipcolor;
    	/* made more awkward because we want to draw the brightest last */
		i=0;
		blipcolor = 1;

		while(i<NoOfMTBlips)
		{
			int y = SonarBlips[i].Y-playerDynPtr->Position.vz;
		  	int x = SonarBlips[i].X-playerDynPtr->Position.vx;
			int y2 = MUL_FIXED(x,sinPhi) + MUL_FIXED(y,cosPhi);
			
			if (y2>=0)
			{
				x = MUL_FIXED(x,cosPhi) - MUL_FIXED(y,sinPhi);

				if(Fast2dMagnitude(x,y2)<ONE_FIXED)
				{				
					BLTSonarBlipToHUD
					(
						x,
				   		y2,
						SonarBlips[i].Brightness,
						SonarBlips[i].Color
					);
				}
			}
            i++;
		}
		/* now fade or kill blips */
		i = NoOfMTBlips;
		while(i--) /* scan through all blips, starting with the last in the list */
		{
			/* decrease blip's brightness */
			MotionTrackerBlips[i].Brightness-=MUL_FIXED(MOTIONTRACKER_SPEED/6,NormalFrameTime);
			if (MotionTrackerBlips[i].Brightness<0)	/* then kill blip */
			{
				/* Kill ith blip by copying the last blip in the list over the ith
				   and decreasing the number of blips */
				NoOfMTBlips--;
				MotionTrackerBlips[i] = MotionTrackerBlips[NoOfMTBlips];
			} 
		}
	}
	return;
}

/*ELD*********************************************************
* Functions for Waveform Analyzer                            *
*************************************************************/
extern int ObjectShouldAppearOnWaveform(STRATEGYBLOCK *sbPtr)
{
	DYNAMICSBLOCK *objectDynPtr = sbPtr->DynPtr;

	/* KJL 12:46:28 21/08/98 - legitimate objects */
	if (sbPtr->I_SBtype == I_BehaviourNetGhost)
	{
		NETGHOSTDATABLOCK *ghostData;

		ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
		LOCALASSERT(ghostData);
		
		if(ghostData->type == I_BehaviourAlienPlayer ||
		   ghostData->type == I_BehaviourMarinePlayer ||
		   ghostData->type == I_BehaviourPredatorPlayer)
		   return 1;
	}
	else if (sbPtr->I_SBtype == I_BehaviourAlien)
	{
		ALIEN_STATUS_BLOCK * statusPtr = (ALIEN_STATUS_BLOCK *)sbPtr->SBdataptr;

   		if(statusPtr->BehaviourState != ABS_Dormant)
			return 1;
	}
	else if (sbPtr->I_SBtype == I_BehaviourPredator)
	{
		return 1;
	}
	else if (sbPtr->I_SBtype == I_BehaviourMarine)
	{
		return 1;
	}
	return 0;
}
static int DoWaveformScan(void)
{
	DYNAMICSBLOCK *playerDynPtr = Player->ObStrategyBlock->DynPtr;
	int numberOfObjects = NumActiveStBlocks;
	int cosPhi, sinPhi;
	int range=MOTIONTRACKER_RANGE/3;
	{
		int phi = playerDynPtr->OrientEuler.EulerY;
		cosPhi = MUL_FIXED(GetCos(phi),MOTIONTRACKER_SCALE);
		sinPhi = MUL_FIXED(GetSin(phi),MOTIONTRACKER_SCALE);
	}
	
	while (numberOfObjects--)
	{
		STRATEGYBLOCK *objectPtr = ActiveStBlockList[numberOfObjects];
		DYNAMICSBLOCK *objectDynPtr = objectPtr->DynPtr;
  		
  		if ((objectDynPtr)&&(!objectDynPtr->IsStatic || objectDynPtr->IsNetGhost)
			&&(ObjectShouldAppearOnWaveform(objectPtr)))
		{
		    /* 2d vector from player to object */
			int dx = objectDynPtr->Position.vx-playerDynPtr->Position.vx;
			int dz = objectDynPtr->Position.vz-playerDynPtr->Position.vz;
			
			{
				int absdx=dx;
				int absdz=dz;
				if (absdx<0) absdx=-absdx;
				if (absdz<0) absdz=-absdz;
				
				/* ignore objects past MT's detection distance */
				/* do quick box check */
				if (absdx>MOTIONTRACKER_RANGE || absdz>MOTIONTRACKER_RANGE)	continue;
			}
			return TRUE;
		}
	}
	return FALSE;
}

/*KJL*********************************************************
* DoMotionTrackerBlips() looks a bit messy but works well.   *
*********************************************************KJL*/
extern int ObjectShouldAppearOnMotionTracker(STRATEGYBLOCK *sbPtr)
{
	DYNAMICSBLOCK *objectDynPtr = sbPtr->DynPtr;

	/* KJL 12:45:54 21/08/98 - objects which will never appear on the MT */
	if((sbPtr->I_SBtype == I_BehaviourInanimateObject)
		||(sbPtr->I_SBtype == I_BehaviourRubberDuck)) {
		return 0;
	}

	if (sbPtr->SBflags.not_on_motiontracker) {
		return(0);
	}

	/* KJL 12:46:28 21/08/98 - objects which need more checks */
	if (sbPtr->I_SBtype == I_BehaviourNetGhost)
	{
		NETGHOSTDATABLOCK *ghostData;

		ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
		LOCALASSERT(ghostData);
		
		if((ghostData->type == I_BehaviourAlienPlayer || 
			ghostData->type == I_BehaviourMarinePlayer ||
			ghostData->type == I_BehaviourPredatorPlayer)
		 &&(objectDynPtr->Position.vx == objectDynPtr->PrevPosition.vx)
		 &&(objectDynPtr->Position.vy == objectDynPtr->PrevPosition.vy)
		 &&(objectDynPtr->Position.vz == objectDynPtr->PrevPosition.vz))
			return 0;
	}
	else if (sbPtr->I_SBtype == I_BehaviourFragment)
	{
		return 0;
	}
	else if (sbPtr->I_SBtype == I_BehaviourHierarchicalFragment)
	{
		return 0;
	}
	else if (sbPtr->I_SBtype == I_BehaviourNetCorpse)
	{
		return 0;
	}
	else if (sbPtr->I_SBtype == I_BehaviourMarine)
	{
		MARINE_STATUS_BLOCK *statusPtr = (MARINE_STATUS_BLOCK *)sbPtr->SBdataptr;

		if (statusPtr->behaviourState == MBS_Waiting)
			return 0;
	}
	else if (sbPtr->I_SBtype == I_BehaviourAlien)
	{
		ALIEN_STATUS_BLOCK * statusPtr = (ALIEN_STATUS_BLOCK *)sbPtr->SBdataptr;

   		if(statusPtr->BehaviourState == ABS_Dormant)
			return 0;
	}
	else if (sbPtr->I_SBtype == I_BehaviourPlatform)
	{
		if((objectDynPtr->Position.vx == objectDynPtr->PrevPosition.vx)
		 &&(objectDynPtr->Position.vy == objectDynPtr->PrevPosition.vy)
		 &&(objectDynPtr->Position.vz == objectDynPtr->PrevPosition.vz))
			return 0;
	}
	else if (sbPtr->I_SBtype == I_BehaviourProximityDoor)
	{
		PROXDOOR_BEHAV_BLOCK *doorbhv;					  
		doorbhv = (PROXDOOR_BEHAV_BLOCK*)sbPtr->SBdataptr;
		if (doorbhv->door_state == I_door_open || doorbhv->door_state == I_door_closed)
		{
			return 0;
		}
	}

	return 1;
}

extern int ObjectShouldAppearOnSonar(STRATEGYBLOCK *sbPtr)
{
	DYNAMICSBLOCK *objectDynPtr = sbPtr->DynPtr;

	/* KJL 12:45:54 21/08/98 - objects which will never appear on the MT */
	if((sbPtr->I_SBtype == I_BehaviourInanimateObject)
		||(sbPtr->I_SBtype == I_BehaviourRubberDuck)) {
		return 0;
	}

	if (sbPtr->SBflags.not_on_motiontracker) {
		return(0);
	}

	/* KJL 12:46:28 21/08/98 - objects which need more checks */
	if (sbPtr->I_SBtype == I_BehaviourNetGhost)
	{
		NETGHOSTDATABLOCK *ghostData;

		ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
		LOCALASSERT(ghostData);
		
		if((ghostData->type == I_BehaviourAlienPlayer || 
			ghostData->type == I_BehaviourMarinePlayer ||
			ghostData->type == I_BehaviourPredatorPlayer))
			return 1;
	}
	else if (sbPtr->I_SBtype == I_BehaviourFragment)
	{
		return 0;
	}
	else if (sbPtr->I_SBtype == I_BehaviourHierarchicalFragment)
	{
		return 0;
	}
	else if (sbPtr->I_SBtype == I_BehaviourNetCorpse)
	{
		return 1;
	}
	else if (sbPtr->I_SBtype == I_BehaviourMarine)
	{
		return 1;
	}
	else if (sbPtr->I_SBtype == I_BehaviourAlien)
	{
		return 1;
	}
	else if (sbPtr->I_SBtype == I_BehaviourPredator)
	{
		return 1;
	}
	else if (sbPtr->I_SBtype == I_BehaviourPlatform)
	{
		return 0;
	}
	else if (sbPtr->I_SBtype == I_BehaviourProximityDoor)
	{
		return 0;
	}
	return 0;
}

static int DoMotionTrackerBlips(void)
{
	DYNAMICSBLOCK *playerDynPtr = Player->ObStrategyBlock->DynPtr;
	int numberOfObjects = NumActiveStBlocks;
	int cosPhi, sinPhi;
	int nearestDistance=MOTIONTRACKER_RANGE;
	{
		int phi = playerDynPtr->OrientEuler.EulerY;
		cosPhi = MUL_FIXED(GetCos(phi),MOTIONTRACKER_SCALE);
		sinPhi = MUL_FIXED(GetSin(phi),MOTIONTRACKER_SCALE);
	}
	
	while (numberOfObjects--)
	{
		STRATEGYBLOCK *objectPtr = ActiveStBlockList[numberOfObjects];
		DYNAMICSBLOCK *objectDynPtr = objectPtr->DynPtr;
		
		if (NoOfMTBlips==MOTIONTRACKER_MAXBLIPS) break;
  		
  		if ((objectDynPtr)&&(!objectDynPtr->IsStatic || objectDynPtr->IsNetGhost)
			&&(ObjectShouldAppearOnMotionTracker(objectPtr)))
		{
		    /* 2d vector from player to object */
			int dx = objectDynPtr->Position.vx-playerDynPtr->Position.vx;
			int dz = objectDynPtr->Position.vz-playerDynPtr->Position.vz;
			
			{
				int absdx=dx;
				int absdz=dz;
				if (absdx<0) absdx=-absdx;
				if (absdz<0) absdz=-absdz;
				
				/* ignore objects past MT's detection distance */
				/* do quick box check */
				if (absdx>MOTIONTRACKER_RANGE || absdz>MOTIONTRACKER_RANGE)	continue;
			}
					
			{
				int y = MUL_FIXED(dx,sinPhi) + MUL_FIXED(dz,cosPhi);
				
				/* ignore objects 'behind' MT */
				if (y>=0)
				{
//				  	int x = MUL_FIXED(dx,cosPhi) - MUL_FIXED(dz,sinPhi);
					int dist = Fast2dMagnitude(dx,dz);
					int radius = MUL_FIXED(dist,MOTIONTRACKER_SCALE);
					
					if (radius<=MTScanLineSize)
					{
						int prevRadius;
					 	{
							int dx = objectDynPtr->PrevPosition.vx-playerDynPtr->PrevPosition.vx;
							int dz = objectDynPtr->PrevPosition.vz-playerDynPtr->PrevPosition.vz;
							prevRadius = MUL_FIXED(Fast2dMagnitude(dx,dz),MOTIONTRACKER_SCALE);
						}
						
						if ((radius>PreviousMTScanLineSize)
					 	  ||(radius<PreviousMTScanLineSize && prevRadius>PreviousMTScanLineSize))
						{						
							/* remember distance for possible display on HUD */
							if (nearestDistance>dist) nearestDistance=dist;

							/* create new blip */
				//			MotionTrackerBlips[NoOfMTBlips].X = x;
				//			MotionTrackerBlips[NoOfMTBlips].Y = y;
							MotionTrackerBlips[NoOfMTBlips].X = objectDynPtr->Position.vx;
							MotionTrackerBlips[NoOfMTBlips].Y = objectDynPtr->Position.vz;
							MotionTrackerBlips[NoOfMTBlips].Brightness = 65536;
						 	NoOfMTBlips++;
						 }
					}  		   		
				}
			}
		}
	}
	return nearestDistance;
}

int DetermineSpeciesColor(STRATEGYBLOCK *sbPtr)
{
	switch (sbPtr->I_SBtype)
	{
	case I_BehaviourMarine:
		return 1;
		break;
	case I_BehaviourAlien:
		return 2;
		break;
	case I_BehaviourPredator:
		return 3;
		break;
	case I_BehaviourNetGhost:
		{
			NETGHOSTDATABLOCK *ghostData = (NETGHOSTDATABLOCK *) sbPtr->SBdataptr;
			switch(ghostData->type)
			{
			case I_BehaviourMarinePlayer:
				return 1;
				break;
			case I_BehaviourAlienPlayer:
				return 2;
				break;
			case I_BehaviourPredatorPlayer:
				return 3;
				break;
			}
			break;
		}
	case I_BehaviourNetCorpse:
		{
			NETCORPSEDATABLOCK *corpseData = (NETCORPSEDATABLOCK *) sbPtr->SBdataptr;
			switch(corpseData->Type)
			{
			case I_BehaviourMarinePlayer:
				return 1;
				break;
			case I_BehaviourAlienPlayer:
				return 2;
				break;
			case I_BehaviourPredatorPlayer:
				return 3;
				break;
			}
		}
	}
	return 1;
}

static int DoSonarBlips(void)
{
	DYNAMICSBLOCK *playerDynPtr = Player->ObStrategyBlock->DynPtr;
	int numberOfObjects = NumActiveStBlocks;
	int cosPhi, sinPhi;
	int nearestDistance=MOTIONTRACKER_RANGE;
	{
		int phi = playerDynPtr->OrientEuler.EulerY;
		cosPhi = MUL_FIXED(GetCos(phi),MOTIONTRACKER_SCALE);
		sinPhi = MUL_FIXED(GetSin(phi),MOTIONTRACKER_SCALE);
	}
	
	while (numberOfObjects--)
	{
		STRATEGYBLOCK *objectPtr = ActiveStBlockList[numberOfObjects];
		DYNAMICSBLOCK *objectDynPtr = objectPtr->DynPtr;
		
		if (NoOfMTBlips==MOTIONTRACKER_MAXBLIPS) break;
  		
  		if (ObjectShouldAppearOnSonar(objectPtr))
		{
		    /* 2d vector from player to object */
			int dx = objectDynPtr->Position.vx-playerDynPtr->Position.vx;
			int dz = objectDynPtr->Position.vz-playerDynPtr->Position.vz;
			
			{
				int absdx=dx;
				int absdz=dz;
				if (absdx<0) absdx=-absdx;
				if (absdz<0) absdz=-absdz;
				
				/* ignore objects past MT's detection distance */
				/* do quick box check */
				if (absdx>MOTIONTRACKER_RANGE || absdz>MOTIONTRACKER_RANGE)	continue;
			}
					
			{
				int y = MUL_FIXED(dx,sinPhi) + MUL_FIXED(dz,cosPhi);
				
				/* ignore objects 'behind' MT */
				/*if (y>=0)    Actually, dont, since this is the Alien Sonar */
				{
//				  	int x = MUL_FIXED(dx,cosPhi) - MUL_FIXED(dz,sinPhi);
					int dist = Fast2dMagnitude(dx,dz);
					int radius = MUL_FIXED(dist,MOTIONTRACKER_SCALE);
					int color;

					color = DetermineSpeciesColor(objectPtr);
					
					if (radius<=MTScanLineSize)
					{
						int prevRadius;
					 	{
							int dx = objectDynPtr->PrevPosition.vx-playerDynPtr->PrevPosition.vx;
							int dz = objectDynPtr->PrevPosition.vz-playerDynPtr->PrevPosition.vz;
							prevRadius = MUL_FIXED(Fast2dMagnitude(dx,dz),MOTIONTRACKER_SCALE);
						}
						
						if ((radius>PreviousMTScanLineSize)
					 	  ||(radius<PreviousMTScanLineSize && prevRadius>PreviousMTScanLineSize))
						{						
							/* remember distance for possible display on HUD */
							if (nearestDistance>dist) nearestDistance=dist;

							/* create new blip */
				//			MotionTrackerBlips[NoOfMTBlips].X = x;
				//			MotionTrackerBlips[NoOfMTBlips].Y = y;
							SonarBlips[NoOfMTBlips].X = objectDynPtr->Position.vx;
							SonarBlips[NoOfMTBlips].Y = objectDynPtr->Position.vz;
							SonarBlips[NoOfMTBlips].Brightness = 65536;
							SonarBlips[NoOfMTBlips].Color = color;
						 	NoOfMTBlips++;
						 }
					}  		   		
				}
			}
		}
	}
	return nearestDistance;
}

static void DisplayHealthAndArmour(void)
{
//	extern void D3D_RenderHUDString(char *stringPtr,int x,int y,int colour);
	int health,armour,flares,armortype;
    /* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	NPC_DATA *NpcData;
	
	switch (AvP.PlayerType)
	{
		case I_Marine:
			switch (AvP.Difficulty) {
				case I_Easy:
					NpcData=GetThisNpcData(I_PC_Marine_Easy);
					break;
				default:
				case I_Medium:
					NpcData=GetThisNpcData(I_PC_Marine_Medium);
					break;
				case I_Hard:
					NpcData=GetThisNpcData(I_PC_Marine_Hard);
					break;
				case I_Impossible:
					NpcData=GetThisNpcData(I_PC_Marine_Impossible);
					break;
			}
			break;
		case I_Alien:
			switch (AvP.Difficulty) {
				case I_Easy:
					NpcData=GetThisNpcData(I_PC_Alien_Easy);
					break;
				default:
				case I_Medium:
					NpcData=GetThisNpcData(I_PC_Alien_Medium);
					break;
				case I_Hard:
					NpcData=GetThisNpcData(I_PC_Alien_Hard);
					break;
				case I_Impossible:
					NpcData=GetThisNpcData(I_PC_Alien_Impossible);
					break;
			}
			break;
		default:
			LOCALASSERT(0);
	}
    GLOBALASSERT(playerStatusPtr);
    	
	health=(PlayerStatusPtr->Health*100)/NpcData->StartingStats.Health;
	armour=(PlayerStatusPtr->Armour*100)/NpcData->StartingStats.Armour;

	health = (health+65535)>>16;
	armour = (armour+65535)>>16;

	flares=PlayerStatusPtr->FlaresLeft;

	if(PlayerStatusPtr->Health<(NpcData->StartingStats.Health<<16))
	{
		//make sure health isn't displayed as 100 , if it is even slightly below.
		//(ie round down 99.5 , even though health is rounded up normally)
		health = min(health,99);
	}
	if(PlayerStatusPtr->Armour<(NpcData->StartingStats.Armour<<16))
	{
		//similarly for armour
		armour = min(armour,99);
	}
	flares=max(0, flares);
	armortype = PlayerStatusPtr->ArmorType;
	
	Render_HealthAndArmour(health,armour,flares,armortype);

}

static void DisplayMarinesAmmo(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr;
	TEMPLATE_WEAPON_DATA *twPtr;
	int primaryRounds, secondaryRounds;
		        
    /* init a pointer to the weapon's data */
    weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
    twPtr = &TemplateWeapon[weaponPtr->WeaponIDNumber];

	primaryRounds=weaponPtr->PrimaryRoundsRemaining>>16;
    if ( (weaponPtr->PrimaryRoundsRemaining&0xffff) ) primaryRounds+=1;
	secondaryRounds=weaponPtr->SecondaryRoundsRemaining>>16;
    if ( (weaponPtr->SecondaryRoundsRemaining&0xffff) ) secondaryRounds+=1;

	if (primaryRounds>HUD_PrimaryRounds)
	{
		HUD_PrimaryRounds += NormalFrameTime/512;
	}
	if (primaryRounds<HUD_PrimaryRounds)
	{
		HUD_PrimaryRounds = primaryRounds;
	}
	if (secondaryRounds>HUD_SecondaryRounds)
	{
		HUD_SecondaryRounds += NormalFrameTime/512;
	}
	if (secondaryRounds<HUD_SecondaryRounds)
	{
		HUD_SecondaryRounds = secondaryRounds;
	}

    
	{
		enum AMMO_ID ammo;

		if (weaponPtr->WeaponIDNumber == WEAPON_GRENADELAUNCHER)
		{
			ammo = GrenadeLauncherData.SelectedAmmo;
		}
		else
		{
			ammo = twPtr->PrimaryAmmoID;
		}
	    Render_MarineAmmo
	    (
	    	TemplateAmmo[ammo].ShortName,
	    	TEXTSTRING_MAGAZINES,
	    	weaponPtr->PrimaryMagazinesRemaining,
	    	TEXTSTRING_ROUNDS,
	    	HUD_PrimaryRounds,
			1
	    );
	}
	if (weaponPtr->WeaponIDNumber == WEAPON_PULSERIFLE)
	{
        Render_MarineAmmo
        (
        	TemplateAmmo[twPtr->SecondaryAmmoID].ShortName,
        	TEXTSTRING_MAGAZINES,
        	weaponPtr->SecondaryMagazinesRemaining,
        	TEXTSTRING_ROUNDS,
        	HUD_SecondaryRounds,
			0
        );
	}

	if (weaponPtr->WeaponIDNumber == WEAPON_TWO_PISTOLS)
	{
        Render_MarineAmmo
        (
        	TemplateAmmo[twPtr->SecondaryAmmoID].ShortName,
        	TEXTSTRING_MAGAZINES,
        	weaponPtr->SecondaryMagazinesRemaining,
        	TEXTSTRING_ROUNDS,
        	HUD_SecondaryRounds,
			0
        );
	}

}				   
#if 0
static void UpdateMarineStatusValues(void)
{
	PLAYER_WEAPON_DATA *weaponPtr;

    /* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
    GLOBALASSERT(playerStatusPtr);
    	
	{
		/* player's current weapon */
    	GLOBALASSERT(playerStatusPtr->SelectedWeaponSlot<MAX_NO_OF_WEAPON_SLOTS);
        
        /* init a pointer to the weapon's data */
        weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
    }
    

	{
    	int value=playerStatusPtr->Health>>16;	/* stored in 16.16 so shift down */
        ValueOfHUDDigit[MARINE_HUD_HEALTH_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[MARINE_HUD_HEALTH_TENS]=value%10;
        value/=10;
		ValueOfHUDDigit[MARINE_HUD_HEALTH_HUNDREDS]=value%10;
	}
	{
    	#if PC_E3DEMO
    	int value=playerStatusPtr->Energy>>16;	/* stored in 16.16 so shift down */
        #else
		extern int FrameRate;
		int value=FrameRate;
		#endif
        ValueOfHUDDigit[MARINE_HUD_ENERGY_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[MARINE_HUD_ENERGY_TENS]=value%10;
        value/=10;
		ValueOfHUDDigit[MARINE_HUD_ENERGY_HUNDREDS]=value%10;
	}
	{
    	int value=playerStatusPtr->Armour>>16;	/* stored in 16.16 so shift down */
        ValueOfHUDDigit[MARINE_HUD_ARMOUR_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[MARINE_HUD_ARMOUR_TENS]=value%10;
        value/=10;
		ValueOfHUDDigit[MARINE_HUD_ARMOUR_HUNDREDS]=value%10;
	}
	
	{
    	int value=weaponPtr->PrimaryRoundsRemaining>>16;
        /* ammo is in 16.16. we want the integer part, rounded up */
        if ( (weaponPtr->PrimaryRoundsRemaining&0xffff) ) value+=1;
        
        ValueOfHUDDigit[MARINE_HUD_PRIMARY_AMMO_ROUNDS_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[MARINE_HUD_PRIMARY_AMMO_ROUNDS_TENS]=value%10;
        value/=10;
		ValueOfHUDDigit[MARINE_HUD_PRIMARY_AMMO_ROUNDS_HUNDREDS]=value%10;
	}
	{
    	int value=weaponPtr->PrimaryMagazinesRemaining;
        ValueOfHUDDigit[MARINE_HUD_PRIMARY_AMMO_MAGAZINES_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[MARINE_HUD_PRIMARY_AMMO_MAGAZINES_TENS]=value%10;
    }	
	
	/* KJL 14:54:39 03/26/97 - secondary ammo */
	if ( (weaponPtr->WeaponIDNumber == WEAPON_PULSERIFLE)
	   ||(weaponPtr->WeaponIDNumber == WEAPON_MYSTERYGUN) )
	{
    	int value=weaponPtr->SecondaryRoundsRemaining>>16;
        /* ammo is in 16.16. we want the integer part, rounded up */
        if ( (weaponPtr->SecondaryRoundsRemaining&0xffff) ) value+=1;
        
        ValueOfHUDDigit[MARINE_HUD_SECONDARY_AMMO_ROUNDS_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[MARINE_HUD_SECONDARY_AMMO_ROUNDS_TENS]=value%10;
        value/=10;
		ValueOfHUDDigit[MARINE_HUD_SECONDARY_AMMO_ROUNDS_HUNDREDS]=value%10;

    	value=weaponPtr->SecondaryMagazinesRemaining;
        ValueOfHUDDigit[MARINE_HUD_SECONDARY_AMMO_MAGAZINES_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[MARINE_HUD_SECONDARY_AMMO_MAGAZINES_TENS]=value%10;
		
		BLTMarineNumericsToHUD(MARINE_HUD_SECONDARY_AMMO_MAGAZINES_TENS);
    }	
	else if (weaponPtr->WeaponIDNumber == WEAPON_GRENADELAUNCHER)
 	{
		/* KJL 11:46:57 04/09/97 - use to display your ammo type */
		int value;
		switch(GrenadeLauncherData.SelectedAmmo)
		{
			case AMMO_GRENADE:
			{
				value=1;
				break;
			}
			case AMMO_FLARE_GRENADE:
			{
				value=2;
				break;
			}
			case AMMO_FRAGMENTATION_GRENADE:
			{
				value=3;
				break;
			}
			case AMMO_PROXIMITY_GRENADE:
			{
				value=4;
				break;
			}
			default:
			{
				LOCALASSERT(0);
				break;
			}
		}
        ValueOfHUDDigit[MARINE_HUD_SECONDARY_AMMO_ROUNDS_UNITS]=value;
		BLTMarineNumericsToHUD(MARINE_HUD_SECONDARY_AMMO_ROUNDS_UNITS);
	}
	else
	{
		BLTMarineNumericsToHUD(MARINE_HUD_PRIMARY_AMMO_MAGAZINES_TENS);
	}

}
#endif
static void HandleMarineWeapon(void)
{
	PLAYER_WEAPON_DATA *weaponPtr;
	TEMPLATE_WEAPON_DATA *twPtr;
    
	    /* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
    GLOBALASSERT(playerStatusPtr);
    	
	{
		/* player's current weapon */
    	GLOBALASSERT(playerStatusPtr->SelectedWeaponSlot<MAX_NO_OF_WEAPON_SLOTS);
        
        /* init a pointer to the weapon's data */
        weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
        twPtr = &TemplateWeapon[weaponPtr->WeaponIDNumber];
    }
	
	/* draw 3d weapon */
	PositionPlayersWeapon();
	

	/* if there is no shape name then return */
	if (twPtr->WeaponShapeName == NULL) return;
	RenderThisDisplayblock(&PlayersWeapon);

	if ((twPtr->MuzzleFlashShapeName != NULL)
	  &&(!twPtr->PrimaryIsMeleeWeapon)
	  &&( (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY) 
	   	||( (weaponPtr->WeaponIDNumber==WEAPON_MARINE_PISTOL)&&(weaponPtr->CurrentState == WEAPONSTATE_FIRING_SECONDARY) )
	   	||( (weaponPtr->WeaponIDNumber==WEAPON_TWO_PISTOLS)&&(weaponPtr->CurrentState == WEAPONSTATE_FIRING_SECONDARY) ) ) )
		PositionPlayersWeaponMuzzleFlash();

	{
		

		if ((twPtr->MuzzleFlashShapeName != NULL)
		  	   &&(!twPtr->PrimaryIsMeleeWeapon)
		  	   &&((weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY) 
		  	   	||((weaponPtr->WeaponIDNumber==WEAPON_MARINE_PISTOL)&&(weaponPtr->CurrentState == WEAPONSTATE_FIRING_SECONDARY))
		  	   	||((weaponPtr->WeaponIDNumber==WEAPON_TWO_PISTOLS)&&(weaponPtr->CurrentState == WEAPONSTATE_FIRING_SECONDARY) )))
		{
			static int onThisFrame=1;
			if(onThisFrame || !twPtr->PrimaryIsRapidFire)
			{
				VECTORCH direction;

				direction.vx = PlayersWeaponMuzzleFlash.ObMat.mat31;
				direction.vy = PlayersWeaponMuzzleFlash.ObMat.mat32;
				direction.vz = PlayersWeaponMuzzleFlash.ObMat.mat33;
				
				if (weaponPtr->WeaponIDNumber==WEAPON_SMARTGUN)
				{
					DrawMuzzleFlash(&PlayersWeaponMuzzleFlash.ObWorld,&direction,MUZZLE_FLASH_SMARTGUN);
					if ((!Underwater) && (LocalDetailLevels.MuzzleSmoke))
					{
						int i = 1;
						VECTORCH velocity = direction;
						velocity.vx >>= 9;
						velocity.vy >>= 9;
						velocity.vz >>= 9;
						do
						{
							VECTORCH position = PlayersWeaponMuzzleFlash.ObWorld;
							position.vx += (FastRandom()&15)-8;
							position.vy += (FastRandom()&15)-8;
							position.vz += (FastRandom()&15)-8;
							MakeParticle(&position,&velocity,PARTICLE_GUNMUZZLE_SMOKE);
						}
						while(--i);
					}
				}
				else if (weaponPtr->WeaponIDNumber==WEAPON_FRISBEE_LAUNCHER)
				{
				}
				else
				{
					DrawMuzzleFlash(&PlayersWeaponMuzzleFlash.ObWorld,&direction,MUZZLE_FLASH_AMORPHOUS);
					if ((!Underwater) && (LocalDetailLevels.MuzzleSmoke))
					{
						int i = 1;
						VECTORCH velocity = direction;
						velocity.vx >>= 9;
						velocity.vy >>= 9;
						velocity.vz >>= 9;
						do
						{
							VECTORCH position = PlayersWeaponMuzzleFlash.ObWorld;
							position.vx += (FastRandom()&15)-8;
							position.vy += (FastRandom()&15)-8;
							position.vz += (FastRandom()&15)-8;
							MakeParticle(&position,&velocity,PARTICLE_GUNMUZZLE_SMOKE);
						}
						while(--i);
					}
				}
			}
			onThisFrame=!onThisFrame;
		//	RenderThisDisplayblock(&PlayersWeaponMuzzleFlash);
		}
		
	}

	/* handle smart targeting */
    SmartTarget(twPtr->SmartTargetSpeed,0);

	/* aim gun sight */ 
    {
    	int aimingSpeed=twPtr->GunCrosshairSpeed * NormalFrameTime;
        AimGunSight(aimingSpeed,twPtr);
	}
}

static void DrawMarineSights(void)
{
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);

	if (playerStatusPtr->Honor)
	{
		BLTGunSightToScreen(GunMuzzleSightX>>16, GunMuzzleSightY>>16,GUNSIGHT_GREENBOX);
		return;
	}
	/* draw standard crosshairs */
	if (MIRROR_CHEATMODE)
	{
 		BLTGunSightToScreen(ScreenDescriptorBlock.SDB_Width - (GunMuzzleSightX>>16), GunMuzzleSightY>>16, GUNSIGHT_CROSSHAIR);
	}
	else
	{
		if ((weaponPtr->WeaponIDNumber != WEAPON_SMARTGUN) &&
			(weaponPtr->WeaponIDNumber != WEAPON_FRISBEE_LAUNCHER) &&
			(weaponPtr->WeaponIDNumber != WEAPON_GRENADELAUNCHER) &&
			(weaponPtr->WeaponIDNumber != WEAPON_CUDGEL) &&
			(weaponPtr->WeaponIDNumber != WEAPON_MINIGUN))
	 		BLTGunSightToScreen(GunMuzzleSightX>>16, GunMuzzleSightY>>16, GUNSIGHT_CROSSHAIR);
	}

	/* draw smart target sights if required */
	{
	    /* access the extra data hanging off the strategy block */
		extern float CameraZoomScale;

	    if (TemplateWeapon[weaponPtr->WeaponIDNumber].IsSmartTarget)
	    {
			if (MIRROR_CHEATMODE)
			{
		     	if (CurrentlySmartTargetingObject)	/* tracking target so use the red box */
		     	{
		     		BLTGunSightToScreen(ScreenDescriptorBlock.SDB_Width - (SmartTargetSightX>>16),SmartTargetSightY>>16,GUNSIGHT_REDBOX);
					BLTTargetingSightToScreen(ScreenDescriptorBlock.SDB_Width - (SmartTargetSightX>>16),SmartTargetSightY>>16);
				}
				else /* not tracking anything, use green box */
				{
				   	BLTGunSightToScreen(ScreenDescriptorBlock.SDB_Width - (SmartTargetSightX>>16),SmartTargetSightY>>16,GUNSIGHT_GREENBOX);
				}
			}
			else
			{
		     	if (CurrentlySmartTargetingObject)	/* tracking target so use the red box */
		     	{
		     		BLTGunSightToScreen(SmartTargetSightX>>16,SmartTargetSightY>>16,GUNSIGHT_REDBOX);
					BLTTargetingSightToScreen(SmartTargetSightX>>16,SmartTargetSightY>>16);
				}
				else /* not tracking anything, use green box */
				{
				   	BLTGunSightToScreen(SmartTargetSightX>>16,SmartTargetSightY>>16,GUNSIGHT_GREENBOX);
				}
			}
	    }
		if ((weaponPtr->WeaponIDNumber == WEAPON_GRENADELAUNCHER) ||
			(weaponPtr->WeaponIDNumber == WEAPON_FRISBEE_LAUNCHER))
		{
		     BLTGunSightToScreen(GunMuzzleSightX>>16, GunMuzzleSightY>>16,GUNSIGHT_GREENBOX);
		}
		if ((weaponPtr->WeaponIDNumber == WEAPON_SADAR) && (CameraZoomScale == 1.0f))
		{
			BLTGunSightToScreen(GunMuzzleSightX>>16, GunMuzzleSightY>>16,GUNSIGHT_REDBOX);
		}
		if ((weaponPtr->WeaponIDNumber == WEAPON_CUDGEL) || (weaponPtr->WeaponIDNumber == WEAPON_MINIGUN))
		{
			BLTGunSightToScreen(GunMuzzleSightX>>16, GunMuzzleSightY>>16, GUNSIGHT_REDDIAMOND);
		}
	}
}



/*KJL****************************************
* ************** PREDATOR HUD ************* *
****************************************KJL*/

static void InitPredatorHUD(void)
{
	PlatformSpecificInitPredatorHUD();

	SmartTarget_Object=NULL;
	Old_SmartTarget_Object=NULL;
	InitialiseGrapplingHook();
}

#if DO_PREDATOR_OVERLAY

static void UpdatePredatorStatusValues(void)
{
	PLAYER_WEAPON_DATA *weaponPtr;

    /* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
    GLOBALASSERT(playerStatusPtr);
    	
	{
		/* player's current weapon */
    	GLOBALASSERT(playerStatusPtr->SelectedWeaponSlot<MAX_NO_OF_WEAPON_SLOTS);
        
        /* init a pointer to the weapon's data */
        weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
    }
    

	{
    	int value=WideMulNarrowDiv(playerStatusPtr->Health,45,6553600);

        if (value>=37)
        {
        	ValueOfHUDDigit[PREDATOR_HUD_HEALTH_5]= value-36;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_4]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_3]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_2]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_1]= 9;
		}
		else if (value>=28)
        {
        	ValueOfHUDDigit[PREDATOR_HUD_HEALTH_5]=	0;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_4]= value-27;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_3]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_2]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_1]= 9;
		}
		else if (value>=19)
        {
        	ValueOfHUDDigit[PREDATOR_HUD_HEALTH_5]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_4]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_3]= value-18;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_2]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_1]= 9;
		}
		else if (value>=10)
        {
        	ValueOfHUDDigit[PREDATOR_HUD_HEALTH_5]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_4]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_3]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_2]= value-9;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_1]= 9;
		}
		else
		{
        	ValueOfHUDDigit[PREDATOR_HUD_HEALTH_5]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_4]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_3]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_2]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_HEALTH_1]= value;
		}
	}
	{
    	int value=WideMulNarrowDiv(playerStatusPtr->Armour,45,6553600);

        if (value>=37)
        {
        	ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_5]= value-36;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_4]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_3]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_2]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_1]= 9;
		}
		else if (value>=28)
        {
        	ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_5]=	0;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_4]= value-27;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_3]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_2]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_1]= 9;
		}
		else if (value>=19)
        {
        	ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_5]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_4]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_3]= value-18;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_2]= 9;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_1]= 9;
		}
		else if (value>=10)
        {
        	ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_5]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_4]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_3]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_2]= value-9;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_1]= 9;
		}
		else
		{
        	ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_5]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_4]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_3]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_2]= 0;
			ValueOfHUDDigit[PREDATOR_HUD_ARMOUR_1]= value;
		}
	}
	
#if 0
	{
    	int value=playerStatusPtr->Energy>>16;	/* stored in 16.16 so shift down */
        ValueOfHUDDigit[MARINE_HUD_ENERGY_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[MARINE_HUD_ENERGY_TENS]=value%10;
        value/=10;
		ValueOfHUDDigit[MARINE_HUD_ENERGY_HUNDREDS]=value%10;
	}
	{
    	int value=weaponPtr->RoundsRemaining>>16;
        /* ammo is in 16.16. we want the integer part, rounded up */
        if ( (weaponPtr->RoundsRemaining&0xffff) ) value+=1;
        
        ValueOfHUDDigit[MARINE_HUD_AMMO_ROUNDS_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[MARINE_HUD_AMMO_ROUNDS_TENS]=value%10;
        value/=10;
		ValueOfHUDDigit[MARINE_HUD_AMMO_ROUNDS_HUNDREDS]=value%10;
	}
	{
    	int value=weaponPtr->MagazinesRemaining;
        ValueOfHUDDigit[MARINE_HUD_AMMO_MAGAZINES_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[MARINE_HUD_AMMO_MAGAZINES_TENS]=value%10;
    }
#endif	
	
}

#endif

void DisplayPredatorHealthAndEnergy(void)
{
	PLAYER_WEAPON_DATA *weaponPtr = &(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot]);
	int value;
	int i;
	int scale = DIV_FIXED(ScreenDescriptorBlock.SDB_Width,640);
	int size = MUL_FIXED(51,scale);
	{
		NPC_DATA *NpcData;
		switch (AvP.Difficulty) {
			case I_Easy:
				NpcData=GetThisNpcData(I_PC_Predator_Easy);
				break;
			default:
			case I_Medium:
				NpcData=GetThisNpcData(I_PC_Predator_Medium);
				break;
			case I_Hard:
				NpcData=GetThisNpcData(I_PC_Predator_Hard);
				break;
			case I_Impossible:
				NpcData=GetThisNpcData(I_PC_Predator_Impossible);
				break;
		}
		LOCALASSERT(NpcData);
		value=MUL_FIXED(PlayerStatusPtr->Health/NpcData->StartingStats.Health,59);
	}
	//Draw Health Icon
#if 0
	{
		HUDCharDesc charDesc;
		charDesc.X = ScreenDescriptorBlock.SDB_Width-size/2;
		charDesc.Y = 0;

		charDesc.Red = 255;
		charDesc.Green = 255;
		charDesc.Blue = 255;
		charDesc.Alpha = 64;

		charDesc.Character=9;

		D3D_DrawHUDPredatorHealth(&charDesc, scale/2);
	}
#endif
	for (i=0; i<6; i++)
	{
		HUDCharDesc charDesc;
		charDesc.X = ScreenDescriptorBlock.SDB_Width-(40+(i*size/2));
		charDesc.Y = 5;
		//charDesc.X = ScreenDescriptorBlock.SDB_Width-size/2;			
		//charDesc.Y = 40+(i*size/2);

		charDesc.Red = 32;
		charDesc.Green = 32;
		charDesc.Blue = 32;
		charDesc.Alpha= 255;

		charDesc.Character=9;

		D3D_DrawHUDPredatorDigit(&charDesc,scale/3);
	}
	for (i=0; i<6; i++)
	{
		HUDCharDesc charDesc;
		charDesc.X = ScreenDescriptorBlock.SDB_Width-(40+(i*size/2));
		charDesc.Y = 5;
		//charDesc.X = ScreenDescriptorBlock.SDB_Width-size/2;			
		//charDesc.Y = 40+(i*size/2);

		charDesc.Red = 0;
		charDesc.Green = 255;
		charDesc.Blue = 0;
		charDesc.Alpha= 255;

		{
			int v=value-i*9;
			if (v>9) v=9;
			else if (v<0) v=0;
			charDesc.Character=v;
		}

		D3D_DrawHUDPredatorDigit(&charDesc,scale/3);
	}
	value= MUL_FIXED(DIV_FIXED(PlayerStatusPtr->FieldCharge,PLAYERCLOAK_MAXENERGY),59);
	for (i=0; i<6; i++)
	{
		HUDCharDesc charDesc;
		charDesc.X = ScreenDescriptorBlock.SDB_Width-(40+(i*size/2));			
		charDesc.Y = 35;

		charDesc.Red = 50;
		charDesc.Green = 50;
		charDesc.Blue = 50;
		charDesc.Alpha= 255;

		charDesc.Character=9;

		D3D_DrawHUDPredatorDigit(&charDesc,scale/3);
	}
	for (i=0; i<6; i++)
	{
		HUDCharDesc charDesc;
		charDesc.X = ScreenDescriptorBlock.SDB_Width-(40+(i*size/2));			
		charDesc.Y = 35;

		charDesc.Red = 0;
		charDesc.Green = 255;
		charDesc.Blue = 255;
		charDesc.Alpha= 255;

		{
			int v=value-i*9;
			if (v>9) v=9;
			else if (v<0) v=0;
			charDesc.Character=v;
		}

		D3D_DrawHUDPredatorDigit(&charDesc,scale/3);
	}
#if 0
	//Draw Energy Icon
	{
		HUDCharDesc charDesc;
		charDesc.X = ScreenDescriptorBlock.SDB_Width-size/2;
		charDesc.Y = (ScreenDescriptorBlock.SDB_Height/1.5)+40;

		charDesc.Red = 255;
		charDesc.Green = 255;
		charDesc.Blue = 255;
		charDesc.Alpha = 64;

		charDesc.Character=9;

		D3D_DrawHUDPredatorEnergy(&charDesc, scale/2);
	}
#endif
	if (weaponPtr->WeaponIDNumber == WEAPON_SONICCANNON ||
		weaponPtr->WeaponIDNumber == WEAPON_MYSTERYGUN)
	{
		value = weaponPtr->PrimaryRoundsRemaining>>16;
		for (i=0; i<2; i++)
		{
			HUDCharDesc charDesc;
			charDesc.X = (ScreenDescriptorBlock.SDB_Width-(i*size/2))-(size/2);			
			charDesc.Y = ScreenDescriptorBlock.SDB_Height-size/2;

			charDesc.Red = 0;
			charDesc.Green = 0;
			charDesc.Blue = 255;
			charDesc.Alpha= 255;

			{
				int v=value-i*9;
				if (v>9) v=9;
				else if (v<0) v=0;
				charDesc.Character=v;
			}

			D3D_DrawHUDPredatorDigit(&charDesc,scale/2);
		}
	}
	if (weaponPtr->WeaponIDNumber == WEAPON_PRED_PISTOL)
	{
		value = weaponPtr->PrimaryRoundsRemaining>>16;
		for (i=0; i<2; i++)
		{
			HUDCharDesc charDesc;
			charDesc.X = (ScreenDescriptorBlock.SDB_Width-(size/2))-(size/2);
			charDesc.Y = ScreenDescriptorBlock.SDB_Height-size/2;

			charDesc.Red = 255;
			charDesc.Green = 255;
			charDesc.Blue = 0;
			charDesc.Alpha = 255;

			{
				int v=value;
				if (v>9) v=9;
				else if (v<0) v=0;
				charDesc.Character=v;
			}
			D3D_DrawHUDPredatorDigit(&charDesc,scale/2);
		}
	}
}
#if 1
static void DoPredatorThreatDisplay(void)
{
	/* evaluate the distance digits */
	{
    	int value=WideMulNarrowDiv(FindPredatorThreats(),72,MOTIONTRACKER_RANGE);

        if (value>=64)
        {
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_UNITS]= 0;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_TENS]= 6;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_HUNDREDS]= 4;
		}
        else if (value>=55)
        {
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_UNITS]= 0;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_TENS]= 5;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_HUNDREDS]= 5;
		}
        else if (value>=46)
        {
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_UNITS]= 0;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_TENS]= 4;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_HUNDREDS]= 6;
		}
        else if (value>=37)
        {
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_UNITS]= 0;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_TENS]= 3;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_HUNDREDS]= 7;
		}
		else if (value>=28)
        {
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_UNITS]= 0;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_TENS]= 2;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_HUNDREDS]= 8;
		}
		else if (value>=19)
        {
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_UNITS]= 0;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_TENS]= 1;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_HUNDREDS]= 9;
		}
		else if (value>=10)
        {
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_UNITS]= 0;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_TENS]= 1;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_HUNDREDS]= 0;
		}
		else
		{
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_UNITS]= 0;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_TENS]= 0;
        	ValueOfHUDDigit[ALIEN_HUD_HEALTH_HUNDREDS]= 9;
		}
	}
   
	
	return;
}


static int FindPredatorThreats(void)
{
	DYNAMICSBLOCK *playerDynPtr = Player->ObStrategyBlock->DynPtr;
	int numberOfObjects = NumActiveStBlocks;
	int nearestDistance=MOTIONTRACKER_RANGE;
	
	while (numberOfObjects--)
	{
		STRATEGYBLOCK *objectPtr = ActiveStBlockList[numberOfObjects];
		DYNAMICSBLOCK *objectDynPtr = objectPtr->DynPtr;
		
		if (objectPtr == Player->ObStrategyBlock) continue;

  		if (objectDynPtr)
		{
		    /* 2d vector from player to object */
			int dx = objectDynPtr->Position.vx-playerDynPtr->Position.vx;
			int dz = objectDynPtr->Position.vz-playerDynPtr->Position.vz;
			int dy = objectDynPtr->Position.vy-playerDynPtr->Position.vy;
			
			if (dy > -2000 || dy <2000)
			{
				int dist = Fast2dMagnitude(dx,dz);
				/* remember distance for possible display on HUD */
				if (nearestDistance>dist) nearestDistance=dist;
  		   		
			}
		}
	}
	return nearestDistance;
}
#endif
static void HandlePredatorWeapon(void)
{
	PLAYER_WEAPON_DATA *weaponPtr;
	TEMPLATE_WEAPON_DATA *twPtr;
    
    /* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
    GLOBALASSERT(playerStatusPtr);
    	
	{
		/* player's current weapon */
    	GLOBALASSERT(playerStatusPtr->SelectedWeaponSlot<MAX_NO_OF_WEAPON_SLOTS);
        
        /* init a pointer to the weapon's data */
        weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
        twPtr = &TemplateWeapon[weaponPtr->WeaponIDNumber];
    }
	
	//PositionPlayersWeapon();

	{
		extern void RenderThisDisplayblock(DISPLAYBLOCK *dbPtr);
		
		/* draw 3d muzzle flash */
		if ((twPtr->MuzzleFlashShapeName != NULL)
		  &&(!twPtr->PrimaryIsMeleeWeapon)
		  &&(weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY) )
			RenderThisDisplayblock(&PlayersWeaponMuzzleFlash);
		
		/* draw 3d weapon */
		if ((twPtr->WeaponShapeName != NULL) && (!ThirdPersonActive))
			RenderThisDisplayblock(&PlayersWeapon);
	}
	if (twPtr->PrimaryIsMeleeWeapon)
	{
		GunMuzzleSightX = (ScreenDescriptorBlock.SDB_Width<<15);
		GunMuzzleSightY = (ScreenDescriptorBlock.SDB_Height<<15);
	}
	else
	{
		SmartTarget(twPtr->SmartTargetSpeed,ONE_FIXED);
		
		/* aim gun sight */ 
		if (weaponPtr->WeaponIDNumber == WEAPON_PRED_SHOULDERCANNON)
		{
			if (PredSight_LockOnTime)
			{	
				GunMuzzleSightX = (ScreenDescriptorBlock.SDB_Width<<15);
				GunMuzzleSightY = (ScreenDescriptorBlock.SDB_Height<<15);
			}
			else
			{
				GunMuzzleSightX = SmartTargetSightX;
				GunMuzzleSightY = SmartTargetSightY;
			}
		}
		else
	    {
	    	int aimingSpeed=twPtr->GunCrosshairSpeed * NormalFrameTime;
	        AimGunSight(aimingSpeed,twPtr);
		}
	}

}

static void DrawPredatorSights(void)
{
	/* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
 
	/* Enforce quiet unless we've got the plasmacaster. */
	if (weaponPtr->WeaponIDNumber!=WEAPON_PRED_SHOULDERCANNON) {
  		if(predHUDSoundHandle != SOUND_NOACTIVEINDEX) {
       		Sound_Stop(predHUDSoundHandle);
		}
	} else {
		/* Think about plasmacaster HUD sounds! */
//		textprint("PredSight_LockOnTime %d\n",PredSight_LockOnTime);
		if ((PredSight_LockOnTime==0)
			&&(weaponPtr->CurrentState!=WEAPONSTATE_SWAPPING_IN)
			&&(weaponPtr->CurrentState!=WEAPONSTATE_READYING)
			&&(SmartTarget_Object)) {
			/* We must be locked on and steady. */
  			if(predHUDSoundHandle != SOUND_NOACTIVEINDEX) {
				if (ActiveSounds[predHUDSoundHandle].soundIndex!=SID_PREDATOR_PLASMACASTER_TARGET_LOCKED) {
		       		Sound_Stop(predHUDSoundHandle);
				}
			}
  			if(predHUDSoundHandle == SOUND_NOACTIVEINDEX) {
	  			Sound_Play(SID_PREDATOR_PLASMACASTER_TARGET_LOCKED,"elh",&predHUDSoundHandle);
			}
		} else {
			/* Not locked on - don't be looping! */
	  		if(predHUDSoundHandle != SOUND_NOACTIVEINDEX) {
	       		Sound_Stop(predHUDSoundHandle);
				/* We were locked on... */
				Sound_Play(SID_PREDATOR_PLASMACASTER_TARGET_LOST,"h");
			}
		}
	}

    if (TemplateWeapon[weaponPtr->WeaponIDNumber].IsSmartTarget)
    {
		/* ChrisF 19/2/99 Changed this to prevent 'instant' re-targetting. */
		if ((SmartTarget_Object==NULL)||(SmartTarget_Object!=Old_SmartTarget_Object))
		{
			PredSight_LockOnTime = PREDATOR_LOCK_ON_TIME;
			PredSight_Angle = 0;
			/* Think about sound... */
			if (SmartTarget_Object) {
				Sound_Play(SID_PREDATOR_PLASMACASTER_REDTRIANGLES,"h");
			}
		}
		else
		{				 
			int segmentScale=PredSight_LockOnTime;

			 
			if (segmentScale<=ONE_FIXED)
			{
				RenderPredatorTargetingSegment((PredSight_Angle+1365*2)&4095, segmentScale, PredSight_LockOnTime);
			}
			
			segmentScale -= PREDATOR_LOCK_ON_TIME/5;
			if (segmentScale<0) segmentScale = 0;
			if (segmentScale<=ONE_FIXED)
			{
				RenderPredatorTargetingSegment((PredSight_Angle+1365)&4095, segmentScale, PredSight_LockOnTime);
			}

			segmentScale -= PREDATOR_LOCK_ON_TIME/5;
			if (segmentScale<0) segmentScale = 0;
			if (segmentScale>ONE_FIXED) segmentScale = ONE_FIXED;

	  		RenderPredatorTargetingSegment(PredSight_Angle, segmentScale, PredSight_LockOnTime);
			
			if (PredSight_LockOnTime>0)
			{
				PredSight_LockOnTime -= NormalFrameTime*PREDATOR_LOCK_ON_SPEED;
			
				if (PredSight_LockOnTime<0)
				{
					PredSight_LockOnTime = 0;
					/* Locked on - play a sound. */
					if (weaponPtr->WeaponIDNumber==WEAPON_PRED_DISC) {
						Sound_Play(SID_PREDATOR_DISK_TARGET_LOCKED,"h");
					} else if (weaponPtr->WeaponIDNumber==WEAPON_PRED_SHOULDERCANNON) {
						Sound_Play(SID_PREDATOR_PLASMACASTER_TARGET_FOUND,"h");
					}
				}
			}
			else
			{
				PredSight_Angle += (NormalFrameTime>>6);
				PredSight_Angle &= 4095;
			}
		}
	}
	
	if ((weaponPtr->WeaponIDNumber != WEAPON_PRED_WRISTBLADE) &&
		(weaponPtr->WeaponIDNumber != WEAPON_PRED_MEDICOMP) &&
		(weaponPtr->WeaponIDNumber != WEAPON_PRED_RIFLE))
	{
		extern int HUDFontsImageNumber;
	  	HUDImageDesc imageDesc;
	  	
		imageDesc.ImageNumber = HUDFontsImageNumber;
		imageDesc.TopLeftX = (ScreenDescriptorBlock.SDB_Width-16)/2;
		imageDesc.TopLeftY = (ScreenDescriptorBlock.SDB_Height-14)/2;
		imageDesc.TopLeftU = 1;
		imageDesc.TopLeftV = 51;
		imageDesc.Height = 15;
		imageDesc.Width = 17;
		imageDesc.Scale = ONE_FIXED;
		imageDesc.Translucency = 255;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;

		Draw_HUDImage(&imageDesc);
	}
}
void DrawWristDisplay(void)
{
	extern HMODELCONTROLLER PlayersWeaponHModelController;
	SECTION_DATA *sectionPtr;
	//int i;

 	char *sectionName[]= {"Dum bar display","Dum 1 display","Dum 2 display","Dum 3 display","Dum 4 display"};

 	sectionPtr=GetThisSectionData(PlayersWeaponHModelController.section_data,sectionName[0]);
	if (!sectionPtr) return;
	
	RenderPredatorPlasmaCasterCharge(PlayerStatusPtr->PlasmaCasterCharge, &sectionPtr->World_Offset, &sectionPtr->SecMat);
	#if 0
   	for (i=0; i<5; i++)
   	{
   		DECAL CurrentDecal;	
		extern MODULE *playerPherModule;
		int z= 0,halfWidth=50,halfHeight=50;


 		sectionPtr=GetThisSectionData(PlayersWeaponHModelController.section_data,sectionName[i]);
		if (!sectionPtr) return;
	
		CurrentDecal.DecalID = DECAL_PREDATOR_BLOOD;

 		CurrentDecal.Vertices[0].vx = -halfWidth;
		CurrentDecal.Vertices[0].vz = -halfHeight;
		CurrentDecal.Vertices[0].vy = z;
		RotateVector(&(CurrentDecal.Vertices[0]),&sectionPtr->SecMat);
		CurrentDecal.Vertices[0].vx += sectionPtr->World_Offset.vx;
		CurrentDecal.Vertices[0].vy += sectionPtr->World_Offset.vy;
		CurrentDecal.Vertices[0].vz += sectionPtr->World_Offset.vz;


		CurrentDecal.Vertices[1].vx = halfWidth;
		CurrentDecal.Vertices[1].vz = -halfHeight;
		CurrentDecal.Vertices[1].vy = z;
		RotateVector(&(CurrentDecal.Vertices[1]),&sectionPtr->SecMat);
		CurrentDecal.Vertices[1].vx += sectionPtr->World_Offset.vx;
		CurrentDecal.Vertices[1].vy += sectionPtr->World_Offset.vy;
		CurrentDecal.Vertices[1].vz += sectionPtr->World_Offset.vz;

		CurrentDecal.Vertices[2].vx = halfWidth;
		CurrentDecal.Vertices[2].vz = halfHeight;
		CurrentDecal.Vertices[2].vy = z;
		RotateVector(&(CurrentDecal.Vertices[2]),&sectionPtr->SecMat);
		CurrentDecal.Vertices[2].vx += sectionPtr->World_Offset.vx;
		CurrentDecal.Vertices[2].vy += sectionPtr->World_Offset.vy;
		CurrentDecal.Vertices[2].vz += sectionPtr->World_Offset.vz;

		CurrentDecal.Vertices[3].vx = -halfWidth;
		CurrentDecal.Vertices[3].vz = halfHeight;
		CurrentDecal.Vertices[3].vy = z;
		RotateVector(&(CurrentDecal.Vertices[3]),&sectionPtr->SecMat);
		CurrentDecal.Vertices[3].vx += sectionPtr->World_Offset.vx;
		CurrentDecal.Vertices[3].vy += sectionPtr->World_Offset.vy;
		CurrentDecal.Vertices[3].vz += sectionPtr->World_Offset.vz;

		CurrentDecal.ModuleIndex = playerPherModule->m_index;

		CurrentDecal.UOffset = 0;
		
		RenderDecal(&CurrentDecal);
	}
	#endif
}
void RotateVertex(VECTOR2D *vertexPtr, int theta)
{
	extern int sine[],cosine[];
	int vx,vy;
	int sin = GetSin(theta);
	int cos = GetCos(theta);

	vx = MUL_FIXED(vertexPtr->vx,cos) - MUL_FIXED(vertexPtr->vy,sin);
	vy = MUL_FIXED(vertexPtr->vx,sin) + MUL_FIXED(vertexPtr->vy,cos);

	vertexPtr->vx = vx;
	vertexPtr->vy = vy;
}	



/*KJL********
* ALIEN HUD *
********KJL*/

static void HandleAlienWeapon(void)
{
	PLAYER_WEAPON_DATA *weaponPtr;
	TEMPLATE_WEAPON_DATA *twPtr;
	extern int Alien_Visible_Weapon;
    
    /* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
    GLOBALASSERT(playerStatusPtr);
    	
	{
		/* player's current weapon */
    	GLOBALASSERT(playerStatusPtr->SelectedWeaponSlot<MAX_NO_OF_WEAPON_SLOTS);
        
        /* init a pointer to the weapon's data */
        weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
        twPtr = &TemplateWeapon[weaponPtr->WeaponIDNumber];
    }
	
	//PositionPlayersWeapon();

	{
		extern void RenderThisDisplayblock(DISPLAYBLOCK *dbPtr);
		
		/* draw 3d muzzle flash */
		if ((twPtr->MuzzleFlashShapeName != NULL)
		  &&(!twPtr->PrimaryIsMeleeWeapon)
		  &&(weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY) )
			RenderThisDisplayblock(&PlayersWeaponMuzzleFlash);
		
		/* draw 3d weapon */
		/* if there is no shape name then return */
		if (twPtr->WeaponShapeName == NULL) return;
			RenderThisDisplayblock(&PlayersWeapon);
		
	}
	//if ((twPtr->PrimaryIsMeleeWeapon)&&
	if ((weaponPtr->WeaponIDNumber == WEAPON_ALIEN_CLAW)&&(Alien_Visible_Weapon==0))
	{
		GunMuzzleSightX = (ScreenDescriptorBlock.SDB_Width<<15);
		GunMuzzleSightY = (ScreenDescriptorBlock.SDB_Height<<15);
	}
	else
	{
	    SmartTarget(twPtr->SmartTargetSpeed,0);
		/* aim gun sight */ 
	    {
	    	int aimingSpeed=twPtr->GunCrosshairSpeed * NormalFrameTime;
	        AimGunSight(aimingSpeed,twPtr);
		}
	}

}

#if DO_ALIEN_OVERLAY

static void UpdateAlienStatusValues(void)
{
    /* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
    GLOBALASSERT(playerStatusPtr);
	{
    	int value=playerStatusPtr->Health>>16;	/* stored in 16.16 so shift down */
        ValueOfHUDDigit[ALIEN_HUD_HEALTH_UNITS]=value%10;
		value/=10;
        ValueOfHUDDigit[ALIEN_HUD_HEALTH_TENS]=value%10;
        value/=10;
		ValueOfHUDDigit[ALIEN_HUD_HEALTH_HUNDREDS]=value%10;
	}
}

#endif

static void RenderFacehuggerBelly(void)
{
	DISPLAYBLOCK displayblock;

	displayblock.ObMat=Global_VDB_Ptr->VDB_Mat;

	TransposeMatrixCH(&displayblock.ObMat);
	displayblock.name=NULL;
	displayblock.ObEuler.EulerX=0;
	displayblock.ObEuler.EulerY=0;
	displayblock.ObEuler.EulerZ=0;
	displayblock.ObFlags=ObFlag_ArbRot;
	displayblock.ObFlags2=0;
	displayblock.ObFlags3=0;
	displayblock.ObNumLights=0;
	displayblock.ObRadius=0;
	displayblock.ObMaxX=0;
	displayblock.ObMinX=0;
	displayblock.ObMaxY=0;
	displayblock.ObMinY=0;
	displayblock.ObMaxZ=0;
	displayblock.ObMinZ=0;
	displayblock.ObTxAnimCtrlBlks=NULL;
	displayblock.ObEIDPtr=NULL;
	displayblock.ObMorphCtrl=NULL;
	displayblock.ObStrategyBlock=NULL;
	displayblock.ShapeAnimControlBlock=NULL;
	displayblock.HModelControlBlock=NULL;
	displayblock.ObMyModule=NULL;		
	displayblock.SpecialFXFlags = 0;
	displayblock.SfxPtr=0;

	displayblock.ObShape=GetLoadedShapeMSL("huggerbelly");
	displayblock.ObWorld.vx = 0;
	displayblock.ObWorld.vy = 0;
	displayblock.ObWorld.vz = 110;

	displayblock.ObView = displayblock.ObWorld;
	{
	   MATRIXCH myMat = Global_VDB_Ptr->VDB_Mat;
	   TransposeMatrixCH(&myMat);
	   RotateVector(&(displayblock.ObWorld), &(myMat));	
	   displayblock.ObWorld.vx += Global_VDB_Ptr->VDB_World.vx;
	   displayblock.ObWorld.vy += Global_VDB_Ptr->VDB_World.vy;
	   displayblock.ObWorld.vz += Global_VDB_Ptr->VDB_World.vz;
	}
	RenderThisDisplayblock(&displayblock);
}

static void RenderNetting(void)
{
	DISPLAYBLOCK displayblock;

	displayblock.ObMat=Global_VDB_Ptr->VDB_Mat;

	TransposeMatrixCH(&displayblock.ObMat);
	displayblock.name=NULL;
	displayblock.ObEuler.EulerX=0;
	displayblock.ObEuler.EulerY=0;
	displayblock.ObEuler.EulerZ=0;
	displayblock.ObFlags=ObFlag_ArbRot;
	displayblock.ObFlags2=0;
	displayblock.ObFlags3=0;
	displayblock.ObNumLights=0;
	displayblock.ObRadius=0;
	displayblock.ObMaxX=0;
	displayblock.ObMinX=0;
	displayblock.ObMaxY=0;
	displayblock.ObMinY=0;
	displayblock.ObMaxZ=0;
	displayblock.ObMinZ=0;
	displayblock.ObTxAnimCtrlBlks=NULL;
	displayblock.ObEIDPtr=NULL;
	displayblock.ObMorphCtrl=NULL;
	displayblock.ObStrategyBlock=NULL;
	displayblock.ShapeAnimControlBlock=NULL;
	displayblock.HModelControlBlock=NULL;
	displayblock.ObMyModule=NULL;		
	displayblock.SpecialFXFlags = 0;
	displayblock.SfxPtr=0;

	displayblock.ObShape=GetLoadedShapeMSL("hudnetting");

	displayblock.ObWorld.vx = AMPGunX;
	displayblock.ObWorld.vy = AMPGunY;
	displayblock.ObWorld.vz = AMPGunZ;

	displayblock.ObView = displayblock.ObWorld;
	{
	   MATRIXCH myMat = Global_VDB_Ptr->VDB_Mat;
	   TransposeMatrixCH(&myMat);
	   RotateVector(&(displayblock.ObWorld), &(myMat));	
	   displayblock.ObWorld.vx += Global_VDB_Ptr->VDB_World.vx;
	   displayblock.ObWorld.vy += Global_VDB_Ptr->VDB_World.vy;
	   displayblock.ObWorld.vz += Global_VDB_Ptr->VDB_World.vz;
	}
	RenderThisDisplayblock(&displayblock);
}

static void DrawAMPSpecificWeapons(void)
{
	PLAYER_STATUS *psPtr = (PLAYER_STATUS *) Player->ObStrategyBlock->SBdataptr;
	PLAYER_WEAPON_DATA *pwPtr = &(psPtr->WeaponSlot[psPtr->SelectedWeaponSlot]);
	DISPLAYBLOCK displayblock;
	extern float CameraZoomScale;

	// Special-case... yuck!
	if (AvP.Network != I_No_Network && HuggedBy)
	{
		RenderFacehuggerBelly();
		return;
	}
	if (psPtr->Immobilized)
	{
		RenderNetting();
	}

	/* Only count the below list if you're not in an APC */
	if (!psPtr->Honor)
	{
		// Add weapons that do not count here:
		if (pwPtr->WeaponIDNumber == WEAPON_SONICCANNON ||
			pwPtr->WeaponIDNumber == WEAPON_PRED_RIFLE ||
			pwPtr->WeaponIDNumber == WEAPON_BEAMCANNON ||
			pwPtr->WeaponIDNumber == WEAPON_MYSTERYGUN) {	// Pred weapons, only if !zoom
			if (CameraZoomScale != 1.0f)
				return;
		}
		if (pwPtr->WeaponIDNumber == WEAPON_PULSERIFLE) {	// Pulse Rifle, only if zoom
			if (psPtr->Class == CLASS_RIFLEMAN) {
				if (CameraZoomScale == 1.0f) 
					return;
			} else
				return;
		}
		if (pwPtr->WeaponIDNumber == WEAPON_SMARTGUN) {
			if (SmartgunMode == I_Free) 
				return;
		}
		if (pwPtr->WeaponIDNumber == WEAPON_MARINE_PISTOL)
		{
			if (psPtr->Class == CLASS_AA_SPEC) {
				if (CameraZoomScale == 1.0f)
					return;
			} else
				return;
		}
		if (pwPtr->WeaponIDNumber == WEAPON_MINIGUN ||
			pwPtr->WeaponIDNumber == WEAPON_GRENADELAUNCHER ||
			pwPtr->WeaponIDNumber == WEAPON_FRISBEE_LAUNCHER ||
			pwPtr->WeaponIDNumber == WEAPON_FLAMETHROWER ||
			pwPtr->WeaponIDNumber == WEAPON_CUDGEL ||
			pwPtr->WeaponIDNumber == WEAPON_PRED_WRISTBLADE ||
			pwPtr->WeaponIDNumber == WEAPON_PRED_SHOULDERCANNON ||
			pwPtr->WeaponIDNumber == WEAPON_PRED_PISTOL ||
			pwPtr->WeaponIDNumber == WEAPON_PRED_MEDICOMP ||
			pwPtr->WeaponIDNumber == WEAPON_PRED_DISC)
			return;
	}
	displayblock.ObMat=Global_VDB_Ptr->VDB_Mat;

	TransposeMatrixCH(&displayblock.ObMat);
	displayblock.name=NULL;
	displayblock.ObEuler.EulerX=0;
	displayblock.ObEuler.EulerY=0;
	displayblock.ObEuler.EulerZ=0;
	displayblock.ObFlags=ObFlag_ArbRot;
	displayblock.ObFlags2=0;
	displayblock.ObFlags3=0;
	displayblock.ObNumLights=0;
	displayblock.ObRadius=0;
	displayblock.ObMaxX=0;
	displayblock.ObMinX=0;
	displayblock.ObMaxY=0;
	displayblock.ObMinY=0;
	displayblock.ObMaxZ=0;
	displayblock.ObMinZ=0;
	displayblock.ObTxAnimCtrlBlks=NULL;
	displayblock.ObEIDPtr=NULL;
	displayblock.ObMorphCtrl=NULL;
	displayblock.ObStrategyBlock=NULL;
	displayblock.ShapeAnimControlBlock=NULL;
	displayblock.HModelControlBlock=NULL;
	displayblock.ObMyModule=NULL;		
	displayblock.SpecialFXFlags = 0;
	displayblock.SfxPtr=0;

	/* Only draw the below mentioned weapons if you're not in an APC */
	if (!psPtr->Honor)
	{
		// Marine Stuff
		if (pwPtr->WeaponIDNumber == WEAPON_SADAR)
		{
			if (CameraZoomScale == 1.0f)
			{
				displayblock.ObShape=GetLoadedShapeMSL("ScopeRifle");
				displayblock.ObWorld.vx = 300;
				displayblock.ObWorld.vy = 500;
				displayblock.ObWorld.vz = 1000;
			}
			else
			{
				displayblock.ObShape=GetLoadedShapeMSL("riflescope");
				displayblock.ObWorld.vx = 0;
				displayblock.ObWorld.vy = 0;
				displayblock.ObWorld.vz = 0;
			}
		}
		if (pwPtr->WeaponIDNumber == WEAPON_PULSERIFLE)
		{
			displayblock.ObShape=GetLoadedShapeMSL("pulse_crossbk");
			displayblock.ObWorld.vx = 0;
			displayblock.ObWorld.vy = 150;
			displayblock.ObWorld.vz = 400;
		}
		if (pwPtr->WeaponIDNumber == WEAPON_MARINE_PISTOL)
		{
			displayblock.ObShape=GetLoadedShapeMSL("binoculars");
			displayblock.ObWorld.vx = 0;
			displayblock.ObWorld.vy = 0;
			displayblock.ObWorld.vz = 300;
		}
		if (pwPtr->WeaponIDNumber == WEAPON_SMARTGUN)
		{
			displayblock.ObShape=GetLoadedShapeMSL("eyepiece");
			displayblock.ObWorld.vx = 0;
			displayblock.ObWorld.vy = -40;
			displayblock.ObWorld.vz = 210;
		}
		if (pwPtr->WeaponIDNumber == WEAPON_AUTOSHOTGUN)
		{
			displayblock.ObShape=GetLoadedShapeMSL("bypass");
			displayblock.ObWorld.vx = 0;
			displayblock.ObWorld.vy = 150;
			displayblock.ObWorld.vz = 400;
		}
		if (pwPtr->WeaponIDNumber == WEAPON_PLASMAGUN)
		{
			displayblock.ObShape=GetLoadedShapeMSL("Welder");
			displayblock.ObWorld.vx = 0;
			displayblock.ObWorld.vy = 150;
			displayblock.ObWorld.vz = 400;
		}

		// Predator Stuff
		if (pwPtr->WeaponIDNumber == WEAPON_PRED_RIFLE)
		{
			displayblock.ObShape=GetLoadedShapeMSL("combistick");
			displayblock.ObWorld.vx = 150;
			displayblock.ObWorld.vy = 100;
			displayblock.ObWorld.vz = 0;
			
			if (pwPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			{
				displayblock.ObWorld.vx = 150;
				displayblock.ObWorld.vy = 100;
				displayblock.ObWorld.vz = 1300;
			}
		}
		if (pwPtr->WeaponIDNumber == WEAPON_SONICCANNON)
		{
			displayblock.ObShape=GetLoadedShapeMSL("spear_gun");
			displayblock.ObWorld.vx = 150;
			displayblock.ObWorld.vy = 300;
			displayblock.ObWorld.vz = 0;
		}
		if (pwPtr->WeaponIDNumber == WEAPON_BEAMCANNON)
		{
			displayblock.ObShape=GetLoadedShapeMSL("WristLauncher");
			displayblock.ObWorld.vx = 150;
			displayblock.ObWorld.vy = 300;
			displayblock.ObWorld.vz = 0;
		}
		if (pwPtr->WeaponIDNumber == WEAPON_MYSTERYGUN)
		{
			displayblock.ObShape=GetLoadedShapeMSL("DartLauncher");
			displayblock.ObWorld.vx = 150;
			displayblock.ObWorld.vy = 300;
			displayblock.ObWorld.vz = 0;
		}
	}
	if (psPtr->Honor)
	{
		displayblock.ObShape=GetLoadedShapeMSL("apc");
		displayblock.ObWorld.vx = 0;
		displayblock.ObWorld.vy = 0;
		displayblock.ObWorld.vz = 300;
	}

	displayblock.ObView = displayblock.ObWorld;
	{
	   MATRIXCH myMat = Global_VDB_Ptr->VDB_Mat;
	   TransposeMatrixCH(&myMat);
	   RotateVector(&(displayblock.ObWorld), &(myMat));	
	   displayblock.ObWorld.vx += Global_VDB_Ptr->VDB_World.vx;
	   displayblock.ObWorld.vy += Global_VDB_Ptr->VDB_World.vy;
	   displayblock.ObWorld.vz += Global_VDB_Ptr->VDB_World.vz;
	}
	RenderThisDisplayblock(&displayblock);
}

static void DrawAlienTeeth(void)
{
	unsigned int Drool=FALSE;
	/* KJL 14:51:00 08/10/98 - test if bite attack is possible */
	{
		extern STRATEGYBLOCK *GetBitingTarget(void);

		if (GrabAttackInProgress)
			return;

		if(GetBitingTarget()) 
		{
			// Do a hissing sound... veeery spooky... :-O
			if (AlienTeethOffset<ONE_FIXED)
			{
				PLAYER_STATUS *psPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
				LOCALASSERT(psPtr);

				if (psPtr->soundHandle == SOUND_NOACTIVEINDEX) {
					Sound_Play(SID_ED_SKEETERDISC_HITWALL,"he",&psPtr->soundHandle);
					if (AvP.Network != I_No_Network) netGameData.trackerNoise = 1;

					if (LocalDetailLevels.WeatherFX)
					{
						Drool=TRUE;
					}
				}
			}
			if (AlienTeethOffset<ONE_FIXED)
			{
				AlienTeethOffset += NormalFrameTime*2;

				if (LocalDetailLevels.WeatherFX)
				{
					Drool=TRUE;
				}
			}
			if (AlienTeethOffset>ONE_FIXED)
			{
				AlienTeethOffset = ONE_FIXED;
			} 
		}
		else
		{
			if (AlienTeethOffset>0)
			{
				AlienTeethOffset -= NormalFrameTime;
			}
			if (AlienTeethOffset<0)
			{
				AlienTeethOffset = 0;
			} 
		}
	}

	if (AlienTeethOffset)
	{
		extern int CloakingPhase;
		int offsetY;

	   	DISPLAYBLOCK displayblock;

	   	displayblock.ObMat=Global_VDB_Ptr->VDB_Mat;
		TransposeMatrixCH(&displayblock.ObMat);
	   	displayblock.name=NULL;
	   	displayblock.ObEuler.EulerX=0;
	   	displayblock.ObEuler.EulerY=0;
	   	displayblock.ObEuler.EulerZ=0;
	   	displayblock.ObFlags=ObFlag_ArbRot;
	   	displayblock.ObFlags2=0;
	   	displayblock.ObFlags3=0;
	   	displayblock.ObNumLights=0;
	   	displayblock.ObRadius=0;
	   	displayblock.ObMaxX=0;
	   	displayblock.ObMinX=0;
	   	displayblock.ObMaxY=0;
	   	displayblock.ObMinY=0;
	   	displayblock.ObMaxZ=0;
	   	displayblock.ObMinZ=0;
	   	displayblock.ObTxAnimCtrlBlks=NULL;
	   	displayblock.ObEIDPtr=NULL;
	   	displayblock.ObMorphCtrl=NULL;
	   	displayblock.ObStrategyBlock=NULL;
	   	displayblock.ShapeAnimControlBlock=NULL;
	   	displayblock.HModelControlBlock=NULL;
	   	displayblock.ObMyModule=NULL;		
	   	displayblock.SpecialFXFlags = 0;
	   	displayblock.SfxPtr=0;
	   
	   	offsetY = MUL_FIXED(GetSin(AlienTeethOffset/64),80);



	   	displayblock.ObShape=GetLoadedShapeMSL("uppertuth@tongue");
	   	displayblock.ObWorld.vx = 0;
	   	displayblock.ObWorld.vy = -200+offsetY;
	   	displayblock.ObWorld.vz = 80;
	   	displayblock.ObView = displayblock.ObWorld;
	   	{
	   		MATRIXCH myMat = Global_VDB_Ptr->VDB_Mat;
	   		TransposeMatrixCH(&myMat);
	   		RotateVector(&(displayblock.ObWorld), &(myMat));	
	   		displayblock.ObWorld.vx += Global_VDB_Ptr->VDB_World.vx;
	   		displayblock.ObWorld.vy += Global_VDB_Ptr->VDB_World.vy;
	   		displayblock.ObWorld.vz += Global_VDB_Ptr->VDB_World.vz;
	   	}

		RenderThisDisplayblock(&displayblock);
		
		if (Drool && ((FastRandom()%10) <= 6))
		{
			VECTORCH velocity;

			velocity.vx = (FastRandom()&2047)-1024;
			velocity.vy = (FastRandom()&2047)-1024;
			velocity.vz = (FastRandom()&2047)-1024;

			MakeParticle(&displayblock.ObWorld, &velocity, PARTICLE_SHADOW);
		}

	   	displayblock.ObShape=GetLoadedShapeMSL("lowertuth@tongue");
	   	displayblock.ObWorld.vx = 0;
	   	displayblock.ObWorld.vy = 200-offsetY;
	   	displayblock.ObWorld.vz = 80;
	   	displayblock.ObView = displayblock.ObWorld;
	   	{
	   		MATRIXCH myMat = Global_VDB_Ptr->VDB_Mat;
	   		TransposeMatrixCH(&myMat);
	   		RotateVector(&(displayblock.ObWorld), &(myMat));	
	   		displayblock.ObWorld.vx += Global_VDB_Ptr->VDB_World.vx;
	   		displayblock.ObWorld.vy += Global_VDB_Ptr->VDB_World.vy;
	   		displayblock.ObWorld.vz += Global_VDB_Ptr->VDB_World.vz;
	   	}

		RenderThisDisplayblock(&displayblock);  
	}


}



/*KJL**************
* Some useful fns *
**************KJL*/

/* returns approx. magnitude */
int Fast2dMagnitude(int dx, int dy)
{
	if (dx<0) dx = -dx;
	if (dy<0) dy = -dy;
	
	if (dx>dy)
		return dx+dy/3;
	else
		return dy+dx/3;
}
 


/*KJL*************************
* On screen messaging system *
*************************KJL*/

extern void NewOnScreenMessage(unsigned char *messagePtr)
{
	#if SupportWindows95
	GADGET_NewOnScreenMessage( messagePtr );	
	#endif
}

static void AimGunSight(int aimingSpeed, TEMPLATE_WEAPON_DATA *twPtr)
{
	int dx,dy,mag;
	int targetX,targetY;
    int boundary = twPtr->SmartTargetRadius*(ScreenDescriptorBlock.SDB_Height/2);

	/* setup target */
    targetX = SmartTargetSightX;
    targetY = SmartTargetSightY;
    
    /* restrict target to a bounding box */
    {
    	int leftBoundary = (ScreenDescriptorBlock.SDB_Width<<15) - boundary;
        int rightBoundary = (ScreenDescriptorBlock.SDB_Width<<15) + boundary;
	 
	    if (targetX<leftBoundary) targetX=leftBoundary;
    	else if (targetX>rightBoundary)	targetX=rightBoundary;
    }
    {
    	int topBoundary = (ScreenDescriptorBlock.SDB_Height<<15) - boundary;
        int bottomBoundary = (ScreenDescriptorBlock.SDB_Height<<15) + boundary;
	    
	    if (targetY<topBoundary) targetY=topBoundary;
    	else if (targetY>bottomBoundary) targetY=bottomBoundary;
    }   
	    
    dx = targetX-GunMuzzleSightX;
  	dy = targetY-GunMuzzleSightY;
    mag = Fast2dMagnitude(dx,dy);
	
    /* return if no need to move sight */
	if (mag==0) return;
    
    /* move the sight */
	if (aimingSpeed)
	{
		dx = WideMulNarrowDiv(aimingSpeed,dx,mag);
		dy = WideMulNarrowDiv(aimingSpeed,dy,mag);
	    GunMuzzleSightX += dx;
		GunMuzzleSightY += dy;

	    /* if overshoot target, move sight back */
		if ( (dx>0 && GunMuzzleSightX>targetX) || (dx<0 && GunMuzzleSightX<targetX)	)
		{
	    	GunMuzzleSightX = targetX;
	    }
		if ( (dy>0 && GunMuzzleSightY>targetY) || (dy<0 && GunMuzzleSightY<targetY)	)
		{
	    	GunMuzzleSightY = targetY;
	    }
	}
	else
	{
		GunMuzzleSightX = targetX;
		GunMuzzleSightY = targetY;
	}
	return;
}

/* returns approx. 9 times the magnitude of the vector */
int Fast3dMagnitude(VECTORCH *v)
{
	int dx,dy,dz;

	dx = v->vx;
	if (dx<0) dx = -dx;
	
	dy = v->vy;
	if (dy<0) dy = -dy;
	
	dz = v->vz*3;	 	 
	if (dz<0) dz = -dz;
						 
	{
		int temp;
		
		if (dx>dy)
			temp = 3*dx+dy;
		else
			temp = 3*dy+dx;
		
		if (temp>dz)
			return 3*temp+dz;
		else
			return 3*dz+temp;
	}
}

#define ZOOM_SCALE_0 1.0f
#define ZOOM_SCALE_1 0.4f
#define ZOOM_SCALE_2 0.1f
#define ZOOM_SCALE_3 0.02f

static int CurrentCameraZoomLevel=0;

static float ZoomLevels[] = {1.0f,0.4f,0.1f,0.02f};

void MaintainZoomingLevel(void)
{			  
	int i;
	float deltaZoom;
	float requestedZoomScale;

	i = 0;
	while (CameraZoomScale<=ZoomLevels[++i]);


	deltaZoom = (ZoomLevels[i-1] - ZoomLevels[i])*(float)NormalFrameTime/32768.0f;
//	textprint("deltaZoom %f, zone %d\n",deltaZoom,i);

   	requestedZoomScale = ZoomLevels[CameraZoomLevel];

	if (requestedZoomScale<CameraZoomScale)
	{
		if (CameraZoomScale==ZoomLevels[i-1]) Sound_Play(SID_PRED_ZOOM_IN,"d",&(Player->ObStrategyBlock->DynPtr->Position));
		
		CameraZoomScale -= deltaZoom;
		if (CameraZoomScale<ZoomLevels[i]) CameraZoomScale = ZoomLevels[i];
		DrawScanlineOverlay = 1;
		ScanlineLevel = (CameraZoomScale - ZoomLevels[i-1])/(ZoomLevels[i]-ZoomLevels[i-1]);

	}
	else if (requestedZoomScale>CameraZoomScale)
	{
		#if 0
		CameraZoomScale += deltaZoom;
		if (requestedZoomScale<CameraZoomScale) 
		#endif
		CameraZoomScale = requestedZoomScale;
		DrawScanlineOverlay = 1;
		ScanlineLevel=1.0f;
		Sound_Play(SID_PRED_ZOOM_OUT,"d",&(Player->ObStrategyBlock->DynPtr->Position));
	}
	else
	{
		DrawScanlineOverlay = 0;
	}

//	textprint("ScanlineLevel %f\n",ScanlineLevel);


}

/*************************************************************************
 Weapon selection list (Weapon Browser) - used to put up a list of weapon
 icons to aid in weapon selection. Programmed by Astro Muppet.
 *************************************************************************/

static	PLAYER_STATUS		*rws_playerStatusPtr;
static	enum WEAPON_SLOT	rws_WeaponSlotToHilight,
							rws_SlotIndex;
static	unsigned	char	rws_Icon;
static	enum WEAPON_ID		rws_WeaponId;
static	enum AMMO_ID		rws_AmmoId;
static	int					rws_listX,
							rws_centreX,
							rws_centreY,
							rws_listStartX,
							rws_listWidth;
static	PLAYER_WEAPON_DATA	rws_PlayerWeapon;
static	BOOL				rws_PulseIconPresent;

static	unsigned	char	rws_IconList [MAX_NO_OF_WEAPON_SLOTS][2];
static	int					rws_IconColourList [MAX_NO_OF_WEAPON_SLOTS];

// Weapons with no ammo are greyed out, expect for special cases such as Predator wristblades.				
#define	GREY_OUT_ICON	   (   ( rws_PlayerWeapon.PrimaryMagazinesRemaining == 0 )\
							&& ( rws_PlayerWeapon.PrimaryRoundsRemaining == 0 )\
							&& ( rws_PlayerWeapon.SecondaryMagazinesRemaining == 0 )\
							&& ( rws_PlayerWeapon.SecondaryRoundsRemaining == 0 )\
							&& ( TemplateWeapon [rws_WeaponId].PrimaryIsMeleeWeapon == 0 )\
							&& ( TemplateWeapon [rws_WeaponId].SecondaryIsMeleeWeapon == 0 ))

void	StartWeaponSelectList ()
{
	if ( ConstructWeaponSelectList ())
	{
		HUD_WeaponSelectListTimer = HUD_WEAPON_SELECT_LIST_DURATION * ONE_FIXED;	
	}
}

void	RefreshWeaponListIfOnScreen ()
{
	if ( HUD_WeaponSelectListTimer <= 0 )
		return;

	StartWeaponSelectList ();
}

static	BOOL ConstructWeaponSelectList ()
{
	// Setup Weapon Select List. Returns FALSE if error occurs. Note that we don't bother
	// checking for an empty list - this is EXTREMELY unlikely and not disasterous.

	if (   ( Player == NULL )
		|| ( Player->ObStrategyBlock == NULL )
		|| ( Player->ObStrategyBlock->SBdataptr == NULL ))
	{
		return (FALSE);
	}

	rws_playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	rws_listWidth = 0;
	// Cudgel and Pulse Rifle are the same thing really - would look odd to have
	// an icon for both ( especially since cudgel isn't usually selectable ). Therefore
	// display a single icon if either is present.
	rws_PulseIconPresent = FALSE;

	memset (rws_IconColourList, 0, sizeof (rws_IconColourList));

	for (rws_SlotIndex = WEAPON_SLOT_1; rws_SlotIndex < MAX_NO_OF_WEAPON_SLOTS; rws_SlotIndex++)		 
	{
		rws_PlayerWeapon = rws_playerStatusPtr->WeaponSlot [rws_SlotIndex];
		rws_WeaponId = rws_PlayerWeapon.WeaponIDNumber;		

		if ((rws_WeaponId != NULL_WEAPON)
		&& (rws_PlayerWeapon.Possessed == 1 ))
		{
			rws_AmmoId = TemplateWeapon [rws_WeaponId].Name;			
			rws_Icon = GetWeaponIconFromAmmoId (rws_AmmoId);

			if (( rws_Icon != ICON_WB_PULSERIFLE)
				|| (!rws_PulseIconPresent))
			{
				// Add icon to list
				rws_IconList [rws_SlotIndex][0] = rws_Icon;
				rws_IconList [rws_SlotIndex][1] = '\0';

				// Determine its colour
				if ( GREY_OUT_ICON )				
					rws_IconColourList [rws_SlotIndex] = 0xffff0000;	// If no ammo then color it red
				else			
					rws_IconColourList [rws_SlotIndex] = 0xffffffff;	// Other weapons with ammo - white
			
				// rws_listWidth += MUL_FIXED (HUDScaleFactor, AAFontWidths [rws_Icon]);	
				rws_listWidth += AAFontWidths [rws_Icon];

				if ( rws_Icon == ICON_WB_PULSERIFLE )
				{
					rws_PulseIconPresent = TRUE;
				}
			}
		}		
	}

	rws_listStartX = ( ScreenDescriptorBlock.SDB_Width - rws_listWidth ) / 2;	
	rws_centreX = ScreenDescriptorBlock.SDB_Width  / 2;	
	rws_centreY = ScreenDescriptorBlock.SDB_Height;	

	return (TRUE);
}

static void Render_WeaponSelectList ()
{
	// Called as part of Marine/Predator HUD maintenance. Puts up a list of available
	// weapons to allow selection to be made quicker. Weapon to change to will be
	// hilighted - has to be calculated here...can get it wrong sometimes if it done in
	// ConstructWeaponSelectList ().

	if (   ( Player == NULL )
		|| ( Player->ObStrategyBlock == NULL )
		|| ( Player->ObStrategyBlock->SBdataptr == NULL ))
	{
		return;
	}

	rws_playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	rws_WeaponSlotToHilight = rws_playerStatusPtr->SwapToWeaponSlot;

	if ( rws_WeaponSlotToHilight == WEAPON_FINISHED_SWAPPING )
	{
		// Oh right...player is not actually waiting for weapon to swap, so hilight
		// their CURRENT weapon.
		rws_WeaponSlotToHilight = rws_playerStatusPtr->SelectedWeaponSlot;
	}

	rws_PlayerWeapon = rws_playerStatusPtr->WeaponSlot [rws_WeaponSlotToHilight];
	rws_WeaponId = rws_PlayerWeapon.WeaponIDNumber;		

	// Display the weapon name above the list - can't setup name in ConstructWeaponSelectList
	// since may change whilst list onscreen. Note that GetTextString doesn't do string copying
	// etc. so we should be ok performance wise.
	RenderStringCentred (GetTextString(TemplateWeapon [rws_WeaponId].Name),
						 rws_centreX,
						 rws_centreY - 50,
						 0xffffffff);	// White
	
	// Position at which to draw list will have been calculated in ConstructWeaponSelectList.
	rws_listX = rws_listStartX;

	for ( rws_SlotIndex = 0; rws_SlotIndex < MAX_NO_OF_WEAPON_SLOTS; rws_SlotIndex++ )		 
	{
		if ( rws_IconColourList [rws_SlotIndex] != 0 )
		{
			if ( rws_SlotIndex == rws_WeaponSlotToHilight )	
			{
				RenderString (rws_IconList [rws_SlotIndex],
							  rws_listX,
							  rws_centreY - 30,
							  0xff00ff00);	// Selected weapon in green
			}
			else
			{
				RenderString (rws_IconList [rws_SlotIndex],
							  rws_listX,
							  rws_centreY - 30,
							  rws_IconColourList [rws_SlotIndex]);
			}
			rws_Icon = rws_IconList [rws_SlotIndex][0];
			rws_listX += AAFontWidths [rws_Icon];
		}
	}							 
}