/* Maintains the equipmnt the player has, 
rounds fired etc etc etc*/

#include "3dc.h"
#include "inline.h"
#include "module.h"

#include "stratdef.h"
#include "gamedef.h" 
#include "gameplat.h"	
#include "bh_types.h"
#include "bh_weap.h"
#include "equipmnt.h"
#include "dynblock.h"
#include "pvisible.h"
#include "language.h"
#include "huddefs.h"
#include "psnd.h"
#include "weapons.h"
#include "inventry.h"
#include "dp_func.h"

#if SupportWindows95
/* for win95 net game support */
#include "pldnet.h"
#include "pldghost.h"
#endif

#include "AvP_UserProfile.h"

#define UseLocalAssert Yes
#include "ourasert.h"

extern int NormalFrameTime;

void InitialisePlayersInventory(PLAYER_STATUS *playerStatusPtr);
void MaintainPlayersInventory(void);
void SetPlayerSecurityClearance(STRATEGYBLOCK *sbPtr, unsigned int securityLevel);
void UseMedikit(void);
extern void CastMarineBot(int weapon);
extern int ERE_Broken(void);
extern int NoClass;
int SlotForThisWeapon(enum WEAPON_ID weaponID);

static int AbleToPickupAmmo(enum AMMO_ID ammoID);
static int AbleToPickupWeapon(enum WEAPON_ID weaponID);
static int AbleToPickupHealth(int healthID);
static int AbleToPickupArmour(int armourID);
static int AbleToPickupMTrackerUpgrade(int mtrackerID);
static int AbleToPickupFlares(int flaresID);
static int AbleToPickupPGC(int pgcID);
static int AbleToPickupIRGoggles(int goggleID);
static int AbleToPickupStealth(int stealthID);
static int AbleToPickupSentry(int sentryID);
static int AbleToPickupLiquid(int waterID, int integrity);
void RemovePickedUpObject(STRATEGYBLOCK *objectPtr);
static int AbleToPickupFieldCharge(int chargeID);
int AutoWeaponChangeOn = TRUE;
int RunMode = 1; // P&H Mode
int CrouchIsToggleKey = 1; // P&H Mode

PLAYER_STARTING_EQUIPMENT StartingEquipment;

int RecallDisc_Charge=400000;
int KOTH;

/* Character saving */
#if SupportWindows95
static int PulseRifleAmmo=0;
static int PulseGrenades=0;
static int Pistol=0;
static int PistolAmmo=0;
static int Drill=0;
static int DrillAmmo=0;
static int Shotgun=0;
static int ShotgunAmmo=0;
static int Flamer=0;
static int FlamerAmmo=0;
static int Smartgun=0;
static int SmartgunAmmo=0;
static int Plasma=0;
static int PlasmaAmmo=0;
static int Flares=0;
static int ArmorType=0;
static int PGC=0;
static int PortableMedikit=0;
static int MotionTracker=0;
static int Intensifier=0;
static int SentryGun=0;
static int PGCStatus=0;
static int Grenades=0;
static int TrackerTimer=0;
static int AirSupply=0;
static int AirStatus=0;
static int EMW=0;
static int EMWAmmo=0;
static int Bypass=0;
static int Welder=0;
static void ReadCharacter(void);
void WriteCharacter(void);
#endif

#define MEDICOMP_MAX_AMMO	(ONE_FIXED*4)

#define SPECIALIST_PISTOLS	(netGameData.specialistPistols)

void MaintainPlayersInventory(void)
{
	DYNAMICSBLOCK *dynPtr = Player->ObStrategyBlock->DynPtr;
	struct collisionreport *nextReport;

	LOCALASSERT(dynPtr);
	nextReport = dynPtr->CollisionReportPtr;
	
	/* walk the collision report list, looking for collisions against inanimate objects */
	while(nextReport)
	{
		STRATEGYBLOCK* collidedWith = nextReport->ObstacleSBPtr;

		if ((collidedWith) && (collidedWith->I_SBtype == I_BehaviourThrownSpear))
		{
			if (AvP.PlayerType == I_Predator)
			{
				int spear_slot = SlotForThisWeapon(WEAPON_PRED_RIFLE);
				if (PlayerStatusPtr->WeaponSlot[spear_slot].Possessed == 0)
				{
					NewOnScreenMessage("You picked up a combi-stick");
					PlayerStatusPtr->WeaponSlot[spear_slot].Possessed = 1;
					PlayerStatusPtr->WeaponSlot[spear_slot].PrimaryRoundsRemaining = 65536;
					PlayerStatusPtr->WeaponSlot[spear_slot].SecondaryRoundsRemaining = 65536;
					if(AvP.Network != I_No_Network)	AddNetMsg_LocalObjectDestroyed(collidedWith);
	    			DestroyAnyStrategyBlock(collidedWith);
					PlayerStatusPtr->SwapToWeaponSlot = spear_slot;
					Sound_Play(SID_PREDATOR_DISK_RECOVERED,"h");
				}
			}
		}

		/* check collison report for valid object */
		if((collidedWith) && (collidedWith->I_SBtype == I_BehaviourInanimateObject))
		{
			INANIMATEOBJECT_STATUSBLOCK* objStatPtr = collidedWith->SBdataptr;
			
			/*Make sure the object hasn't already been picked up this frame*/
			if(collidedWith->SBflags.please_destroy_me==0 && objStatPtr->respawnTimer==0)
			{
				/* now test for inanimate objects we can pick up... */
				switch(objStatPtr->typeId)
				{
					case(IOT_Weapon):
					{
						if (AbleToPickupWeapon(objStatPtr->subType)) {
							RemovePickedUpObject(collidedWith);
							switch(objStatPtr->subType) {
								case WEAPON_PULSERIFLE:
									NewOnScreenMessage("You picked up a pulse rifle");
									break;
								case WEAPON_SMARTGUN:
									NewOnScreenMessage("You picked up a smartgun");
									break;
								case WEAPON_FLAMETHROWER:
									NewOnScreenMessage("You picked up a flamethrower");
									break;
								case WEAPON_SADAR:
									NewOnScreenMessage("You picked up a sniper scope rifle");
									break;
								case WEAPON_GRENADELAUNCHER:
									NewOnScreenMessage("You picked up a shotgun");
									break;
								case WEAPON_MINIGUN:
									NewOnScreenMessage("You picked up a survey charge");
									break;
								case WEAPON_MARINE_PISTOL:
									NewOnScreenMessage("You picked up a pistol");
									break;
								case WEAPON_FRISBEE_LAUNCHER:
									NewOnScreenMessage("You picked up a phased-plasma gun");
									break;
								case WEAPON_AUTOSHOTGUN:
									NewOnScreenMessage("You picked up an electronic bypass kit");
									break;
								case WEAPON_PLASMAGUN:
									NewOnScreenMessage("You picked up a hand welder");
									break;
							}
						}
						break;
					}
					case(IOT_Ammo):
					{
						if (AbleToPickupAmmo(objStatPtr->subType)) {
							RemovePickedUpObject(collidedWith);
							switch(objStatPtr->subType) {
								case AMMO_10MM_CULW:
									NewOnScreenMessage("You picked up a mag of M309 10mm ammunition");
									break;
								case AMMO_SMARTGUN:
									NewOnScreenMessage("You picked up a mag of M250 10mm ammunition");
									break;
								case AMMO_FLAMETHROWER:
									NewOnScreenMessage("You picked up a canister of napalm");
									break;
								case AMMO_GRENADE:
									NewOnScreenMessage("You picked up 8 rounds of 12 gauge 1235B buckshot");
									break;
								case AMMO_MINIGUN:
									NewOnScreenMessage("You picked up a survey charge");
									break;
								case AMMO_PULSE_GRENADE:
									NewOnScreenMessage("You picked up 2 M60 RPHEF grenades");
									break;
							}
						}
						break;
					}
					case(IOT_Health):
					{
						if (AbleToPickupHealth(objStatPtr->subType)) {
							switch(objStatPtr->subType) {
							case 0:
								NewOnScreenMessage("You picked up a medikit");
								break;
							case 1:
								NewOnScreenMessage("You picked up a portable medikit");
								break;
							case 2:
								if (AvP.PlayerType == I_Alien)
								{
									NewOnScreenMessage("You ate a host...");
								}
								break;
							}
							RemovePickedUpObject(collidedWith);
						}
						break;
					}
					case(IOT_Armour):
					{
						if (AbleToPickupArmour(objStatPtr->subType)) {
							switch(objStatPtr->subType) {
							case 1:
								NewOnScreenMessage("You picked up a HEAP Suit");
								break;
							default:
								NewOnScreenMessage("You picked up an M3 armor");
								break;
							}
							RemovePickedUpObject(collidedWith);
						}
						break;
					}
					case(IOT_Furniture):
					{
						break;
					}
					case(IOT_Key):
					{
						if (AbleToPickupLiquid(objStatPtr->subType,collidedWith->integrity)) {
							RemovePickedUpObject(collidedWith);
						}
						break;
					}
					case(IOT_BoxedSentryGun):
					{
						if (AbleToPickupSentry(objStatPtr->subType)) {
						    RemovePickedUpObject(collidedWith);
							NewOnScreenMessage("You picked a sentry gun box");
						}
						break;
					}
					case(IOT_IRGoggles):
					{
						if (AbleToPickupIRGoggles(objStatPtr->subType)) {
							RemovePickedUpObject(collidedWith);
							NewOnScreenMessage("You picked up a shoulder lamp");
						}
						break;
					}
					case(IOT_DataTape):
					{
						if (AbleToPickupFlares(objStatPtr->subType)) {
							RemovePickedUpObject(collidedWith);
							NewOnScreenMessage("You picked up 5 bundled flares");
						}
						break;
					}
      				case(IOT_MTrackerUpgrade):
					{
						if (AbleToPickupMTrackerUpgrade(objStatPtr->subType)) {			
							RemovePickedUpObject(collidedWith);
							NewOnScreenMessage("You picked up a motion tracker");
						}
						break;
					}
      				case(IOT_PheromonePod):
					{
						if (AbleToPickupPGC(objStatPtr->subType)) {
							/* Do nothing, this is the Alien Egg object */
						}
						break;
					}
					case IOT_SpecialPickupObject:
					{
						if (AbleToPickupStealth(objStatPtr->subType)) {
							RemovePickedUpObject(collidedWith);
						}
						break;
					}
					case IOT_FieldCharge:
					{
						if (AbleToPickupFieldCharge(objStatPtr->subType)) {
							RemovePickedUpObject(collidedWith);
							NewOnScreenMessage("You picked up a field charge unit");
						}
						break;
					}


				}
			}
		} else if((collidedWith) && (collidedWith->I_SBtype == I_BehaviourNetGhost)) {
			NETGHOSTDATABLOCK* ghostData = collidedWith->SBdataptr;

			if (collidedWith->SBflags.please_destroy_me==0) {
				if (ghostData->type==I_BehaviourInanimateObject) {
					if (ghostData->IOType==IOT_Ammo) {
						if (AbleToPickupAmmo(ghostData->subtype)) {
							AddNetMsg_LocalObjectDestroyed_Request(collidedWith);
							ghostData->IOType=IOT_Non;
							/* So it's not picked up again... */
							NewOnScreenMessage(GetTextString(TemplateAmmo[ghostData->subtype].ShortName));
						}
					}
					if (ghostData->IOType==IOT_Weapon) {
						if (AbleToPickupWeapon(ghostData->subtype)) {
							AddNetMsg_LocalObjectDestroyed_Request(collidedWith);
							ghostData->IOType=IOT_Non;
							/* So it's not picked up again... */
						 	/*Message now done in able to pickup function*/
						   //	NewOnScreenMessage(GetTextString(TemplateWeapon[ghostData->subtype].Name));
						}
					}
				}
			}
		}

		nextReport = nextReport->NextCollisionReportPtr;
	}

}

void LoadAllWeapons(PLAYER_STATUS *playerStatusPtr) {

   	int slot = MAX_NO_OF_WEAPON_SLOTS;

	/* Sorry, doesn't do pulse grenades yet. */

	do {
		PLAYER_WEAPON_DATA *wdPtr;
		TEMPLATE_WEAPON_DATA *twPtr;
		TEMPLATE_AMMO_DATA *templateAmmoPtr;

		wdPtr = &playerStatusPtr->WeaponSlot[--slot];
	    twPtr = &TemplateWeapon[wdPtr->WeaponIDNumber];
		templateAmmoPtr = &TemplateAmmo[twPtr->PrimaryAmmoID];

		if ((wdPtr->PrimaryRoundsRemaining==0)
			&&(wdPtr->PrimaryMagazinesRemaining>0)
			&&(twPtr->PrimaryAmmoID!=AMMO_NONE)) {
			wdPtr->PrimaryRoundsRemaining = templateAmmoPtr->AmmoPerMagazine;
			wdPtr->PrimaryMagazinesRemaining--;
		}

		if ((wdPtr->SecondaryRoundsRemaining==0)
			&&(wdPtr->SecondaryMagazinesRemaining>0)
			&&(twPtr->SecondaryAmmoID!=AMMO_NONE)) {
			TEMPLATE_AMMO_DATA *secondaryTemplateAmmoPtr;

			secondaryTemplateAmmoPtr = &TemplateAmmo[twPtr->SecondaryAmmoID];
			
			wdPtr->SecondaryRoundsRemaining = secondaryTemplateAmmoPtr->AmmoPerMagazine;
			wdPtr->SecondaryMagazinesRemaining--;
		}

    } while(slot);

}

#if SupportWindows95
void WriteCharacter(void)
{
	FILE *pFile;
	
	pFile = fopen("User_Profiles/character.txt","w");

	if (!pFile) return;

	{
		PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
		char Ammo[100]; 
		int a;
		GLOBALASSERT(playerStatusPtr);

		/* Pulse Rifle #1 - #2 */
		a=SlotForThisWeapon(WEAPON_PULSERIFLE);
		if (playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining) {
			sprintf(Ammo, "#%d\n", (playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining+1));
			fprintf(pFile, Ammo);
		} else {
			sprintf(Ammo, "#%d\n", 0);
			fprintf(pFile, Ammo);
		}
		if (playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining) {
			sprintf(Ammo, "#%d\n", (playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining/ONE_FIXED));
			fprintf(pFile, Ammo);
		} else {
			sprintf(Ammo, "#%d\n", 0);
			fprintf(pFile, Ammo);
		}
		/* Pistol #3 - #4 */
		a=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
		if (playerStatusPtr->WeaponSlot[a].Possessed) {
			fprintf(pFile, "#1\n");
		} else {
			fprintf(pFile, "#0\n");
		}
		if (playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining) {
			sprintf(Ammo, "#%d\n", (playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining+1));
			fprintf(pFile, Ammo);
		} else {
			sprintf(Ammo, "#%d\n", 0);
			fprintf(pFile, Ammo);
		}
		/* Drill #4 - #5 */
		a=SlotForThisWeapon(WEAPON_MINIGUN);
		if (playerStatusPtr->WeaponSlot[a].Possessed) {
			fprintf(pFile, "#1\n");
		} else {
			fprintf(pFile, "#0\n");
		}
		if (playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining) {
			sprintf(Ammo, "#%d\n", 1);
			fprintf(pFile, Ammo);
		} else {
			fprintf(pFile, "#0\n");
		}
		/* Shotgun #6 - #7 */
		a=SlotForThisWeapon(WEAPON_GRENADELAUNCHER);
		if (playerStatusPtr->WeaponSlot[a].Possessed) {
			fprintf(pFile, "#1\n");
		} else {
			fprintf(pFile, "#0\n");
		}
		if (playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining) {
			sprintf(Ammo, "#%d\n", (playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining+1));
			fprintf(pFile, Ammo);
		} else {
			fprintf(pFile, "#0\n");
		}
		/* Flamer #8 - #9 */
		a=SlotForThisWeapon(WEAPON_FLAMETHROWER);
		if (playerStatusPtr->WeaponSlot[a].Possessed) {
			fprintf(pFile, "#1\n");
		} else {
			fprintf(pFile, "#0\n");
		}
		if (playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining) {
			sprintf(Ammo, "#%d\n", 1);
			fprintf(pFile, Ammo);
		} else {
			fprintf(pFile, "#0\n");
		}
		/* Smartgun #10 - #11 */
		a=SlotForThisWeapon(WEAPON_SMARTGUN);
		if (playerStatusPtr->WeaponSlot[a].Possessed) {
			fprintf(pFile, "#1\n");
		} else {
			fprintf(pFile, "#0\n");
		}
		if (playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining) {
			sprintf(Ammo, "#%d\n", 1);
			fprintf(pFile, Ammo);
		} else {
			fprintf(pFile, "#0\n");
		}
		/* Plasma #12 - #13 */
		a=SlotForThisWeapon(WEAPON_FRISBEE_LAUNCHER);
		if (playerStatusPtr->WeaponSlot[a].Possessed) {
			fprintf(pFile, "#1\n");
		} else {
			fprintf(pFile, "#0\n");
		}
		if (playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining) {
			sprintf(Ammo, "#%d\n", 1);
			fprintf(pFile, Ammo);
		} else {
			fprintf(pFile, "#0\n");
		}
		/* Health And Armor */

		/* Equipment */
		sprintf(Ammo, "#%d\n", playerStatusPtr->FlaresLeft);	/* 14 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->ArmorType);		/* 15 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->PGC);			/* 16 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->Medikit);		/* 17 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->MTrackerType);	/* 18 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->IRGoggles);		/* 19 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->IHaveAPlacedAutogun);	/* 20 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->PGCStatus);		/* 21 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->Grenades);		/* 22 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->TrackerTimer);	/* 23 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->AirSupply);		/* 24 */
		fprintf(pFile, Ammo);
		sprintf(Ammo, "#%d\n", playerStatusPtr->AirStatus);		/* 25 */
		fprintf(pFile, Ammo);

		/* #26 and #27 - Scope Rifle and Ammo */
		a=SlotForThisWeapon(WEAPON_SADAR);
		if (playerStatusPtr->WeaponSlot[a].Possessed) {
			fprintf(pFile, "#1\n");
		} else {
			fprintf(pFile, "#0\n");
		}
		if (playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining) {
			sprintf(Ammo, "#%d\n", (playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining+1));
			fprintf(pFile, Ammo);
		} else {
			fprintf(pFile, "#0\n");
		}
		/* #28 - Bypass Kit */
		a=SlotForThisWeapon(WEAPON_AUTOSHOTGUN);
		if (playerStatusPtr->WeaponSlot[a].Possessed) {
			fprintf(pFile, "#1\n");
		} else {
			fprintf(pFile, "#0\n");
		}
		/* #29 - Hand Welder */
		a=SlotForThisWeapon(WEAPON_PLASMAGUN);
		if (playerStatusPtr->WeaponSlot[a].Possessed) {
			fprintf(pFile, "#1\n");
		} else {
			fprintf(pFile, "#0\n");
		}
	}
	fclose(pFile);
}
static void ReadCharacter(void)
{
	FILE *fpInput;

	fpInput = fopen("User_Profiles/character.txt","rb");

	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &PulseRifleAmmo);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &PulseGrenades);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Pistol);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &PistolAmmo);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Drill);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &DrillAmmo);	/* 6 */
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Shotgun);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &ShotgunAmmo);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Flamer);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &FlamerAmmo);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Smartgun);	/* 11 */
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &SmartgunAmmo);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Plasma);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &PlasmaAmmo);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Flares);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &ArmorType);	/* 16 */
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &PGC);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &PortableMedikit);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &MotionTracker);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Intensifier);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &SentryGun);	/* 21 */
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &PGCStatus);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Grenades);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &TrackerTimer);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &AirSupply);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &AirStatus);	/* 26 */
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &EMW);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &EMWAmmo);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Bypass);
	while(fgetc(fpInput) != '#');
	fscanf(fpInput,"%d", &Welder);
	fclose(fpInput);

	// Level Specifics
	{
		extern char LevelName[];

		// Valore Ganymedes: Start with 5 Flares
		if (!stricmp("valore", &LevelName))
		{
			Pistol=0;
			PistolAmmo=0;
			PulseRifleAmmo=0;
			PulseGrenades=0;
			Drill=0;
			DrillAmmo=0;
			Shotgun=0;
			ShotgunAmmo=0;
			Flamer=0;
			FlamerAmmo=0;
			Plasma=0;
			PlasmaAmmo=0;
			Smartgun=0;
			SmartgunAmmo=0;
			EMW=0;
			EMWAmmo=0;
			Flares=5;
			MotionTracker=0;
			Intensifier=0;
			ArmorType=0;
			PortableMedikit=0;
			SentryGun=0;
			Grenades=0;
			TrackerTimer=0;
			PGCStatus=0;
			AirSupply=0;
			AirStatus=0;
			Bypass=0;
			Welder=0;
		}

		// Arrival: Keep Shotgun, Pistol and Flares
		if (!stricmp("arrival", &LevelName))
		{
			Pistol=1;
			PistolAmmo=2;

			PulseRifleAmmo=0;
			PulseGrenades=0;
			Drill=0;
			DrillAmmo=0;

			//Shotgun=0;
			if (ShotgunAmmo > 1)
				ShotgunAmmo=1;

			Flamer=0;
			FlamerAmmo=0;
			Plasma=0;
			PlasmaAmmo=0;
			Smartgun=0;
			SmartgunAmmo=0;
			EMW=0;
			EMWAmmo=0;

			if (Flares > 5)
				Flares=5;

			MotionTracker=0;
			Intensifier=0;
			ArmorType=0;
			PortableMedikit=0;
			SentryGun=0;
			Grenades=0;
			TrackerTimer=0;
			PGCStatus=0;
			AirSupply=0;
			AirStatus=0;
			Bypass=0;
			Welder=0;
		}
	}
}
#endif
	
void InitialisePlayersInventory(PLAYER_STATUS *playerStatusPtr)
{
	PLAYER_WEAPON_DATA *weaponDataPtr = &playerStatusPtr->WeaponSlot[WEAPON_SLOT_1];

	enum WEAPON_ID *PlayerWeaponKey;
    
	switch(AvP.PlayerType) {
		case I_Marine:
			PlayerWeaponKey=&MarineWeaponKey[0];
			break;
		case I_Alien:
			PlayerWeaponKey=&AlienWeaponKey[0];
			break;
		case I_Predator:
			PlayerWeaponKey=&PredatorWeaponKey[0];
			break;
	}
	/* Character File */
	if (AvP.Network == I_No_Network)
		ReadCharacter();

	/* If the player has recently joined a network game, kill him off */
	if (AvP.Network != I_No_Network)
	{
		extern int RecentlyJoined;
		if (RecentlyJoined)
			playerStatusPtr->IsAlive = 0;
	}

	/* Reset certain globals */
	{
		extern int GrenadeDelay;
		extern int ZeroG;
		extern int Underwater;
		extern int Surface;
		extern int Fog;
		extern int Run;
		extern int OnLadder;
		extern int HackPool;
		extern int WeldPool;
		extern int WeldMode;

		GrenadeDelay=ZeroG=Underwater=Surface=Fog=Run=OnLadder=HackPool=WeldPool=WeldMode=0;
	}

    {
    	int slot = MAX_NO_OF_WEAPON_SLOTS;
        do
        {
    		PLAYER_WEAPON_DATA *wdPtr = &playerStatusPtr->WeaponSlot[--slot];

   			//wdPtr->WeaponIDNumber = NULL_WEAPON;
   			wdPtr->WeaponIDNumber = PlayerWeaponKey[slot];

 		   	wdPtr->PrimaryRoundsRemaining=0;
            wdPtr->PrimaryMagazinesRemaining=0;
 		   	wdPtr->SecondaryRoundsRemaining=0;
            wdPtr->SecondaryMagazinesRemaining=0;
 			wdPtr->CurrentState = WEAPONSTATE_IDLE;
            wdPtr->StateTimeOutCounter=0;
			wdPtr->PositionOffset.vx = 0;
			wdPtr->PositionOffset.vy = 0;
			wdPtr->PositionOffset.vz = 0;
			wdPtr->DirectionOffset.EulerX = 0;
			wdPtr->DirectionOffset.EulerY = 0;
			wdPtr->DirectionOffset.EulerZ = 0;
			wdPtr->Possessed=0;
			InitThisWeapon(wdPtr); // To set the null pointers.
        }
		while(slot);
	}
	
	GrenadeLauncherData.StandardRoundsRemaining=0;	
	GrenadeLauncherData.StandardMagazinesRemaining=0;
	GrenadeLauncherData.FlareRoundsRemaining=0;
	GrenadeLauncherData.FlareMagazinesRemaining=0;
	GrenadeLauncherData.ProximityRoundsRemaining=0;
	GrenadeLauncherData.ProximityMagazinesRemaining=0;
	GrenadeLauncherData.FragmentationRoundsRemaining=0;
	GrenadeLauncherData.FragmentationMagazinesRemaining=0;
	GrenadeLauncherData.SelectedAmmo = AMMO_GRENADE;
														  	
	/* 'swap' to weapon at beginning of game */
    playerStatusPtr->SelectedWeaponSlot = WEAPON_SLOT_1;
	playerStatusPtr->PreviouslySelectedWeaponSlot = WEAPON_SLOT_1;
    playerStatusPtr->SwapToWeaponSlot = WEAPON_SLOT_1;
	weaponDataPtr->CurrentState = WEAPONSTATE_SWAPPING_IN;
	weaponDataPtr->StateTimeOutCounter = 0;

	/* Mission Pack specific */
	if (AvP.Network) {
		playerStatusPtr->FlaresLeft = 0;
		playerStatusPtr->MTrackerType = 0;
		playerStatusPtr->IRGoggles = 0;
		playerStatusPtr->PGC = (ONE_FIXED*245);
		playerStatusPtr->ArmorType = 0;
		playerStatusPtr->Medikit = 0;
		playerStatusPtr->IHaveAPlacedAutogun = 0;
		playerStatusPtr->ChestbursterTimer = 0;
		playerStatusPtr->Grenades = 0;
		playerStatusPtr->TrackerTimer = 0;
		playerStatusPtr->PGCStatus = 0;
		playerStatusPtr->AirSupply = 0;
		playerStatusPtr->AirStatus = 0;
		
		// Fix for class bug
		if (NoClass == 1)
		{
			playerStatusPtr->Class = CLASS_NONE;
			playerStatusPtr->Cocoons = CLASS_NONE;
			NoClass = 0;

			if ((AvP.PlayerType == I_Alien) && (netGameData.LifeCycle))
			{
				playerStatusPtr->Class = CLASS_EXF_W_SPEC;
				playerStatusPtr->Cocoons = CLASS_EXF_W_SPEC;
			}
		}
		playerStatusPtr->Immobilized = 0;
		playerStatusPtr->Honor = 0;
		playerStatusPtr->Destr = 0;
		playerStatusPtr->Toxin = 0;
	} else {
		playerStatusPtr->FlaresLeft = Flares;
		playerStatusPtr->MTrackerType = MotionTracker;
		playerStatusPtr->IRGoggles = Intensifier;
		playerStatusPtr->PGC = PGC;
		playerStatusPtr->ArmorType = ArmorType;
		playerStatusPtr->Medikit = PortableMedikit;
		playerStatusPtr->IHaveAPlacedAutogun = SentryGun;
		playerStatusPtr->ChestbursterTimer = 0;
		playerStatusPtr->Grenades = Grenades;
		playerStatusPtr->TrackerTimer = TrackerTimer;
		playerStatusPtr->PGCStatus = PGCStatus;
		playerStatusPtr->AirSupply = AirSupply;
		playerStatusPtr->AirStatus = AirStatus;
		playerStatusPtr->Class = 0;
		playerStatusPtr->Cocoons = 0;
		playerStatusPtr->Immobilized = 0;
		playerStatusPtr->Honor = 0;
		playerStatusPtr->Destr = 0;
		playerStatusPtr->Toxin = 0;
	}
	/* Initially not facehugged */
	playerStatusPtr->MyFaceHugger=NULL;

	/* Nor dead. */
	playerStatusPtr->MyCorpse=NULL;

	ThisDiscMode=4;//I_Seek_Track;

	SmartgunMode=I_Free;

	/* switch on player type */
	switch(AvP.PlayerType)
	{
		int a;

		case I_Marine:
		{
			/*if in a multiplayer game , check to see if player is a specialist marine*/
			if(AvP.Network != I_No_Network)
			{
				switch(playerStatusPtr->Class)
				{
					case CLASS_NONE:
					case 20:
						a = SlotForThisWeapon(WEAPON_CUDGEL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;

						playerStatusPtr->invulnerabilityTimer = (ONE_FIXED*120);
						break;
					case CLASS_RIFLEMAN:
						a = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 3;

						a = SlotForThisWeapon(WEAPON_PULSERIFLE);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 4;
						playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining = (ONE_FIXED*4);

						playerStatusPtr->MTrackerType = 1;
						playerStatusPtr->IRGoggles = 1;
						playerStatusPtr->Grenades = 2;

						break;
					case CLASS_SMARTGUNNER:
						a = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 2;

						a = SlotForThisWeapon(WEAPON_SMARTGUN);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 1;

						playerStatusPtr->IRGoggles = 1;
						playerStatusPtr->Grenades = 1;
						break;
					case CLASS_INC_SPEC:
						a = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 3;

						a = SlotForThisWeapon(WEAPON_FLAMETHROWER);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 1;

						playerStatusPtr->IRGoggles = 1;
						playerStatusPtr->Grenades = 3;
						break;
					case CLASS_MEDIC_PR:
						a = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 4;

						a = SlotForThisWeapon(WEAPON_PULSERIFLE);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 3;
						playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining = (ONE_FIXED*4);

						playerStatusPtr->Medikit = 10;
						break;
					case CLASS_MEDIC_FT:
						a = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 4;

						a = SlotForThisWeapon(WEAPON_FLAMETHROWER);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 1;

						playerStatusPtr->Medikit = 10;
						playerStatusPtr->IRGoggles = 1;
						break;
					case CLASS_AA_SPEC:
						a = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 3;

						a = SlotForThisWeapon(WEAPON_PULSERIFLE);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 4;
						playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining = (ONE_FIXED*4);

						playerStatusPtr->FlaresLeft = 8;
						playerStatusPtr->Grenades = 5;
						break;
					case CLASS_COM_TECH:
						a = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 3;

						/* Electronic Bypass Kit */
						a = SlotForThisWeapon(WEAPON_AUTOSHOTGUN);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining = ONE_FIXED;

						a = SlotForThisWeapon(WEAPON_PULSERIFLE);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 4;
						playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining = (ONE_FIXED*4);

						playerStatusPtr->MTrackerType = 1;
						playerStatusPtr->IRGoggles = 1;
						break;
					case CLASS_EXF_INFANTRY:
						a = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 3;

						a = SlotForThisWeapon(WEAPON_PULSERIFLE);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 4;
						playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining = (ONE_FIXED*0);

						playerStatusPtr->ArmorType = 1;
						playerStatusPtr->MTrackerType = 1;
						playerStatusPtr->Medikit = 1;
						playerStatusPtr->PGC = (ONE_FIXED*600);
						playerStatusPtr->PGCStatus = 1;
						playerStatusPtr->AirSupply = (ONE_FIXED*600);
						break;
					case CLASS_ENGINEER:
						a = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 3;

						a = SlotForThisWeapon(WEAPON_MINIGUN);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 1;

						a = SlotForThisWeapon(WEAPON_PLASMAGUN);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 1;

						a = SlotForThisWeapon(WEAPON_GRENADELAUNCHER);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 3;

						playerStatusPtr->IRGoggles = 1;
						playerStatusPtr->Grenades = 1;
						playerStatusPtr->Medikit = 10;
						break;
					case CLASS_ARMORER:
						a = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						playerStatusPtr->WeaponSlot[a].Possessed = 1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining = 3;

						playerStatusPtr->MTrackerType = 1;
						playerStatusPtr->IRGoggles = 1;
						break;
					case CLASS_PRED_WARRIOR:
					case CLASS_PRED_HUNTER:
					case CLASS_RENEGADE:
					case CLASS_ELDER:
						playerStatusPtr->Class = CLASS_NONE;
						a = SlotForThisWeapon(WEAPON_CUDGEL);
						playerStatusPtr->WeaponSlot[a].Possessed=1;

						break;
					case CLASS_ALIEN_WARRIOR:
					case CLASS_ALIEN_DRONE:
					case CLASS_TANK_ALIEN:
					case CLASS_EXF_SNIPER:
					case CLASS_EXF_W_SPEC:
						playerStatusPtr->Class = CLASS_NONE;
						a = SlotForThisWeapon(WEAPON_CUDGEL);
						playerStatusPtr->WeaponSlot[a].Possessed=1;

						break;
				}
				playerStatusPtr->PreviouslySelectedWeaponSlot = a;
    			playerStatusPtr->SwapToWeaponSlot = a;
				
			}
			else
			{
				/* Backup Pistol */
				if (Pistol) {
					a=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
					if (a!=-1) {
						playerStatusPtr->WeaponSlot[a].Possessed=1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=PistolAmmo;
					}
				}
				/* Standard Pulse Rifle */
				a=SlotForThisWeapon(WEAPON_PULSERIFLE);
				if (GRENADE_MODE) {
	            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=0;
	            	playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining=(ONE_FIXED*99);
    	        	playerStatusPtr->WeaponSlot[a].SecondaryMagazinesRemaining=0;
				} else {
	            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=PulseRifleAmmo;
	            	playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining=(ONE_FIXED*PulseGrenades);
    	        	playerStatusPtr->WeaponSlot[a].SecondaryMagazinesRemaining=0;
				}
            	playerStatusPtr->WeaponSlot[a].Possessed=1;
				/* Conditional cudgel! */
				a=SlotForThisWeapon(WEAPON_CUDGEL);
				if (a!=-1) {
           			playerStatusPtr->WeaponSlot[a].Possessed=1;
				}
				/* Character Saving retrieval */
				if (Drill) {
					a=SlotForThisWeapon(WEAPON_MINIGUN);
					if (a!=-1) {
						playerStatusPtr->WeaponSlot[a].Possessed=1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=DrillAmmo;
					}
				}
				if (Shotgun) {
					a=SlotForThisWeapon(WEAPON_GRENADELAUNCHER);
					if (a!=-1) {
						playerStatusPtr->WeaponSlot[a].Possessed=1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=ShotgunAmmo;
					}
				}
				if (Flamer) {
					a=SlotForThisWeapon(WEAPON_FLAMETHROWER);
					if (a!=-1) {
						playerStatusPtr->WeaponSlot[a].Possessed=1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=FlamerAmmo;
					}
				}
				if (Plasma) {
					a=SlotForThisWeapon(WEAPON_FRISBEE_LAUNCHER);
					if (a!=-1) {
						playerStatusPtr->WeaponSlot[a].Possessed=1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=PlasmaAmmo;
					}
				}
				if (Smartgun) {
					a=SlotForThisWeapon(WEAPON_SMARTGUN);
					if (a!=-1) {
						playerStatusPtr->WeaponSlot[a].Possessed=1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=SmartgunAmmo;
					}
				}
				if (EMW) {
					a = SlotForThisWeapon(WEAPON_SADAR);
					if (a!=-1) {
						playerStatusPtr->WeaponSlot[a].Possessed=1;
						playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=EMWAmmo;
					}
				}
				if (Bypass) {
					a = SlotForThisWeapon(WEAPON_AUTOSHOTGUN);
					if (a!=-1) {
						playerStatusPtr->WeaponSlot[a].Possessed=1;
						playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=ONE_FIXED;
					}
				}
				if (Welder) {
					a = SlotForThisWeapon(WEAPON_PLASMAGUN);
					if (a!=-1) {
						playerStatusPtr->WeaponSlot[a].Possessed=1;
						playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=ONE_FIXED;
					}
				}
			}
    		break;	
    	}
       	case I_Predator:
		{
			a=SlotForThisWeapon(WEAPON_PRED_WRISTBLADE);
			playerStatusPtr->WeaponSlot[a].Possessed=1;

			if (AvP.Network == I_No_Network)
			{
				a=SlotForThisWeapon(WEAPON_PRED_PISTOL);
            	playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=(ONE_FIXED*9);
				playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=0;
				playerStatusPtr->WeaponSlot[a].Possessed=1;

				a=SlotForThisWeapon(WEAPON_PRED_RIFLE);
            	playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=ONE_FIXED;
				playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining=ONE_FIXED;
				playerStatusPtr->WeaponSlot[a].Possessed=1;

				a=SlotForThisWeapon(WEAPON_PRED_DISC);
            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=1;
				playerStatusPtr->WeaponSlot[a].Possessed=1;

				a=SlotForThisWeapon(WEAPON_PRED_MEDICOMP);
            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=0;
            	playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=MEDICOMP_MAX_AMMO;
				playerStatusPtr->WeaponSlot[a].Possessed=1;

				a=SlotForThisWeapon(WEAPON_SONICCANNON);
				playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=(ONE_FIXED*15);
				playerStatusPtr->WeaponSlot[a].Possessed=1;

				a=SlotForThisWeapon(WEAPON_BEAMCANNON);
				playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=ONE_FIXED;
				playerStatusPtr->WeaponSlot[a].Possessed=1;

				a=SlotForThisWeapon(WEAPON_MYSTERYGUN);
				playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=(ONE_FIXED*5);
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			
			if((playerStatusPtr->Class == CLASS_PRED_WARRIOR) ||
			   (playerStatusPtr->Class == CLASS_TANK_ALIEN))
			{
				a=SlotForThisWeapon(WEAPON_PRED_RIFLE);
           	    playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=ONE_FIXED;
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			if(playerStatusPtr->Class == CLASS_RENEGADE)
			{
				a=SlotForThisWeapon(WEAPON_PRED_PISTOL);
            	playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=(ONE_FIXED*9);
            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=0;
				playerStatusPtr->WeaponSlot[a].Possessed=1;

				a=SlotForThisWeapon(WEAPON_MYSTERYGUN);
				playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=(ONE_FIXED*5);
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			if((playerStatusPtr->Class != CLASS_ELDER) &&
			   (playerStatusPtr->Class != CLASS_TANK_ALIEN))
			{
				a=SlotForThisWeapon(WEAPON_PRED_SHOULDERCANNON);
		        playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=5;
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			if(playerStatusPtr->Class == CLASS_PRED_HUNTER)
			{
				a=SlotForThisWeapon(WEAPON_SONICCANNON);
				playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=(ONE_FIXED*15);
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			if(playerStatusPtr->Class == CLASS_ELDER)
			{
				a=SlotForThisWeapon(WEAPON_PRED_DISC);
            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=1;
				playerStatusPtr->WeaponSlot[a].Possessed=1;

				a=SlotForThisWeapon(WEAPON_BEAMCANNON);
				playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=ONE_FIXED;
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			if((playerStatusPtr->Class != CLASS_ELDER) &&
			   (playerStatusPtr->Class != CLASS_RENEGADE) &&
			   (playerStatusPtr->Class != CLASS_TANK_ALIEN))
			{
				a=SlotForThisWeapon(WEAPON_PRED_MEDICOMP);
            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=0;
            	playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=MEDICOMP_MAX_AMMO;
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}

			break;
		}
		case I_Alien:
		{
			a=SlotForThisWeapon(WEAPON_ALIEN_CLAW);
			playerStatusPtr->WeaponSlot[a].Possessed=1;
			a=SlotForThisWeapon(WEAPON_ALIEN_GRAB);
			playerStatusPtr->WeaponSlot[a].Possessed=0;
			//a=SlotForThisWeapon(WEAPON_ALIEN_SPIT);
            //playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=100*65536;
			//playerStatusPtr->WeaponSlot[a].Possessed=1;
			break;
		}
		default:
			LOCALASSERT(1==0);
			break;							
        
    }
	/*check jetpack and grappling hook*/
    playerStatusPtr->JetpackEnabled = StartingEquipment.marine_jetpack;
    playerStatusPtr->GrapplingHookEnabled = StartingEquipment.predator_grappling_hook;	

	LoadAllWeapons(PlayerStatusPtr);
}


/* fn returns zero if unable to add ammo to inventory,
 eg, of the wrong type */

static int AbleToPickupAmmo(enum AMMO_ID ammoID)
{
	int weaponSlot = -1;

	LOCALASSERT(ammoID>=0);
	LOCALASSERT(ammoID<MAX_NO_OF_AMMO_TEMPLATES);

	switch(AvP.PlayerType)
	{
		case I_Marine:
       	{
			PLAYER_STATUS *psPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
			LOCALASSERT(psPtr);

			if(AvP.Network != I_No_Network)
			{
				switch(ammoID)
				{
					case AMMO_10MM_CULW:
						if (psPtr->Class != CLASS_RIFLEMAN &&
							psPtr->Class != CLASS_MEDIC_PR &&
							psPtr->Class != CLASS_COM_TECH &&
							psPtr->Class != CLASS_EXF_INFANTRY &&
							psPtr->Class != CLASS_AA_SPEC) {
							return 0;
						}
						break;
					case AMMO_PULSE_GRENADE:
						if (psPtr->Class != CLASS_RIFLEMAN &&
							psPtr->Class != CLASS_COM_TECH &&
							psPtr->Class != CLASS_MEDIC_PR &&
							psPtr->Class != CLASS_AA_SPEC) {
							return 0;
						}
						break;
					case AMMO_SMARTGUN:
						if (psPtr->Class != CLASS_SMARTGUNNER) {
							return 0;
						}
						break;
					case AMMO_MARINE_PISTOL:
						/* Everyone can pick up pistol ammo */
						break;
					case AMMO_FLAMETHROWER:
						if (psPtr->Class != CLASS_INC_SPEC &&
							psPtr->Class != CLASS_MEDIC_FT) {
							return 0;
						}
						break;
					case AMMO_SADAR_TOW:
						if (psPtr->Class != CLASS_EXF_SNIPER) {
							return 0;
						}
						break;
					case AMMO_GRENADE:
						if (psPtr->Class != CLASS_ENGINEER) {
							return 0;
						}
						break;
					case AMMO_MINIGUN:
						if (psPtr->Class != CLASS_ENGINEER) {
							return 0;
						}
						break;
					default :
						break;
				}
			}
			switch(ammoID)
			{
				case AMMO_10MM_CULW:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_PULSERIFLE);
					break;
				}
				case AMMO_PLASMA:
				{
					PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
				    GLOBALASSERT(playerStatusPtr);
					NewOnScreenMessage("You picked up a Grenade");
					playerStatusPtr->Grenades++;
					return(1);
					break;
				}
				case AMMO_SHOTGUN:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_AUTOSHOTGUN);
					break;
				}
				case AMMO_SMARTGUN:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_SMARTGUN);
					{
						PLAYER_WEAPON_DATA *weaponPtr;
						    
					    /* access the extra data hanging off the strategy block */
						PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
					    GLOBALASSERT(playerStatusPtr);
	    	
				    	/* init a pointer to the weapon's data */
				    	weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);

						if (weaponPtr->PrimaryRoundsRemaining != ONE_FIXED*950) {
							weaponPtr->PrimaryRoundsRemaining += ONE_FIXED*100;
							if (weaponPtr->PrimaryRoundsRemaining > ONE_FIXED*950)
								weaponPtr->PrimaryRoundsRemaining = ONE_FIXED*950;
						}
						return(1);
					}
					break;
				}
				case AMMO_FLAMETHROWER:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_FLAMETHROWER);
					{
						PLAYER_WEAPON_DATA *weaponPtr;
						    
					    /* access the extra data hanging off the strategy block */
						PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
					    GLOBALASSERT(playerStatusPtr);
	    	
				    	/* init a pointer to the weapon's data */
				    	weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);

						if (weaponPtr->PrimaryRoundsRemaining != ONE_FIXED*500) {
							weaponPtr->PrimaryRoundsRemaining += ONE_FIXED*100;
							if (weaponPtr->PrimaryRoundsRemaining > ONE_FIXED*500)
								weaponPtr->PrimaryRoundsRemaining = ONE_FIXED*500;
						}
						return(1);
					}
					break;
				}
				case AMMO_SADAR_TOW:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_SADAR);
					break;
				}
				case AMMO_FRISBEE:
				{
					//weaponSlot = SlotForThisWeapon(WEAPON_FRISBEE_LAUNCHER);
					return(1);
					break;
				}
				case AMMO_MARINE_PISTOL_PC:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
					break;
				}
				case AMMO_GRENADE: 
				{
					weaponSlot = SlotForThisWeapon(WEAPON_GRENADELAUNCHER);
					GrenadeLauncherData.StandardMagazinesRemaining+=1;
					GrenadeLauncherData.FlareMagazinesRemaining+=1;
					GrenadeLauncherData.ProximityMagazinesRemaining+=1;
					GrenadeLauncherData.FragmentationMagazinesRemaining+=1;
					break;
				}
				case AMMO_MINIGUN:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_MINIGUN);
					{
						PLAYER_WEAPON_DATA *weaponPtr;
						    
					    /* access the extra data hanging off the strategy block */
						PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
					    GLOBALASSERT(playerStatusPtr);
	    	
				    	/* init a pointer to the weapon's data */
				    	weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);

						if (weaponPtr->PrimaryRoundsRemaining != ONE_FIXED*6) {
							weaponPtr->PrimaryRoundsRemaining += ONE_FIXED;
							if (weaponPtr->PrimaryRoundsRemaining > ONE_FIXED*6)
								weaponPtr->PrimaryRoundsRemaining = ONE_FIXED*6;
						}
						return(1);
					}
					break;
				}
				case AMMO_PULSE_GRENADE:
				{
					/* Quick fix... */
					weaponSlot = SlotForThisWeapon(WEAPON_PULSERIFLE);
					{
						PLAYER_WEAPON_DATA *weaponPtr;
						    
					    /* access the extra data hanging off the strategy block */
						PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
					    GLOBALASSERT(playerStatusPtr);
	    	
				    	/* init a pointer to the weapon's data */
				    	weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);
						
						#if 0
						if(weaponPtr->SecondaryMagazinesRemaining == 99)
						{
							/* no room! */
							return 0;
						}
						else
						{
							weaponPtr->SecondaryMagazinesRemaining+=1; /* KJL 12:54:37 03/10/97 - need extra data field in ammo templates */
						
							if(weaponPtr->SecondaryMagazinesRemaining > 4)
								weaponPtr->SecondaryMagazinesRemaining = 4;
						}
						#else
						if(weaponPtr->SecondaryRoundsRemaining == (ONE_FIXED*4))
						{
							/* no room! */
							return 0;
						}

						weaponPtr->SecondaryRoundsRemaining+=(4*ONE_FIXED);
						if (weaponPtr->SecondaryRoundsRemaining>(4*ONE_FIXED)) {
							weaponPtr->SecondaryRoundsRemaining=(4*ONE_FIXED);
						}
						#endif
					}		
					/* successful */
					return 1;
					break;
				}
				default:
					break;
			}
			
			break;
		}
       	case I_Predator:
		{
			switch(ammoID)
			{
				case AMMO_PRED_RIFLE:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_PRED_RIFLE);
					break;
				}
				case AMMO_PRED_DISC:
				{
					PLAYER_WEAPON_DATA *weaponPtr;
					PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);

				    GLOBALASSERT(playerStatusPtr);
					weaponSlot = SlotForThisWeapon(WEAPON_PRED_DISC);

				    weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);

					if ((weaponPtr->PrimaryMagazinesRemaining!=0)
						||(weaponPtr->PrimaryRoundsRemaining!=0)) {
						/* You have a disc: you can't have another! */
						return(0);
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		case I_Alien:
		{
			break;
		}

		default:
			LOCALASSERT(1==0);
			break;
	}

	/* if unable to find the correct weapon slot for the ammo */
	if (weaponSlot == -1) return 0;
	
	{
		PLAYER_WEAPON_DATA *weaponPtr;
	    
	    /* access the extra data hanging off the strategy block */
		PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	    GLOBALASSERT(playerStatusPtr);
	    	
	    /* init a pointer to the weapon's data */
	    weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);
		
		if(weaponPtr->PrimaryMagazinesRemaining == 10)
		{
			/* no room! */
			return 0;
		}
		else
		{
			if (ammoID==AMMO_SADAR_TOW) {
				weaponPtr->PrimaryMagazinesRemaining++;
			} else if (ammoID==AMMO_MARINE_PISTOL_PC) {
				if ((AvP.Network != I_No_Network)&&(PISTOL_INFINITE_AMMO)) {
					weaponPtr->PrimaryMagazinesRemaining=10;
					/* If you have Two Pistols, load that, too. */
					{
						PLAYER_WEAPON_DATA *twoPistolPtr;
						int twoPistolSlot;

						twoPistolSlot=SlotForThisWeapon(WEAPON_TWO_PISTOLS);
						if (twoPistolSlot!=-1) {
							twoPistolPtr=&(playerStatusPtr->WeaponSlot[twoPistolSlot]);
							if (twoPistolPtr) {
								twoPistolPtr->PrimaryMagazinesRemaining=5;
								twoPistolPtr->SecondaryMagazinesRemaining=5;
							}
						}
					}
				} else {
					weaponPtr->PrimaryMagazinesRemaining+=1;
					/* If you have Two Pistols, load that, too. */
					{
						PLAYER_WEAPON_DATA *twoPistolPtr;
						int twoPistolSlot;

						twoPistolSlot=SlotForThisWeapon(WEAPON_TWO_PISTOLS);
						if (twoPistolSlot!=-1) {
							twoPistolPtr=&(playerStatusPtr->WeaponSlot[twoPistolSlot]);
							if (twoPistolPtr) {
								twoPistolPtr->PrimaryMagazinesRemaining+=5;
								twoPistolPtr->SecondaryMagazinesRemaining+=5;
							}
						}
					}
				}
			} else if (ammoID==AMMO_10MM_CULW) {
				weaponPtr->PrimaryMagazinesRemaining+=1;
			} else if (ammoID==AMMO_PRED_RIFLE) {
				/* Add to Rounds. */
				weaponPtr->PrimaryRoundsRemaining+=(ONE_FIXED*SPEARS_PER_PICKUP);
				if (weaponPtr->PrimaryRoundsRemaining>(ONE_FIXED*MAX_SPEARS)) {
					weaponPtr->PrimaryRoundsRemaining=(ONE_FIXED*MAX_SPEARS);
				}
			} else if (ammoID==AMMO_PRED_DISC) {
				/* Disc case... */
				if ((weaponPtr->PrimaryMagazinesRemaining==0)
					&& (weaponPtr->PrimaryRoundsRemaining==0)) {
					weaponPtr->PrimaryRoundsRemaining+=ONE_FIXED;
					/* Autoswap to disc here? */
					AutoSwapToDisc();
				} else {
					weaponPtr->PrimaryMagazinesRemaining+=1;
				}
			} else {
				weaponPtr->PrimaryMagazinesRemaining+=1; /* KJL 12:54:37 03/10/97 - need extra data field in ammo templates */
			}
			if (weaponPtr->PrimaryMagazinesRemaining > 10)
				weaponPtr->PrimaryMagazinesRemaining = 10;
		}
	}

	/* successful */
	return 1;

}


/* fn returns zero if unable to add weapon to inventory,
 eg, of the wrong type */

static int AbleToPickupWeapon(enum WEAPON_ID weaponID)
{
	BOOL WillSwitchToThisWeapon = FALSE;
	int weaponSlot = -1;
    /* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
    GLOBALASSERT(playerStatusPtr);

	LOCALASSERT(weaponID>=0);
	LOCALASSERT(weaponID<MAX_NO_OF_WEAPON_TEMPLATES);

	switch(AvP.PlayerType)
	{
		case I_Marine:
       	{
			PLAYER_STATUS *psPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
			LOCALASSERT(psPtr);
			
			if(AvP.Network != I_No_Network)
			{
				switch(weaponID)
				{
					case WEAPON_PULSERIFLE:
						if (psPtr->Class != CLASS_RIFLEMAN &&
							psPtr->Class != CLASS_MEDIC_PR &&
							psPtr->Class != CLASS_COM_TECH &&
							psPtr->Class != CLASS_EXF_INFANTRY &&
							psPtr->Class != CLASS_AA_SPEC) {
							return 0;
						}
						break;
					case WEAPON_MARINE_PISTOL:
						// Everyone can pick up pistols
						break;
					case WEAPON_SMARTGUN:
						if (psPtr->Class != CLASS_SMARTGUNNER &&
							psPtr->Class != CLASS_EXF_W_SPEC) {
							return 0;
						}
						break;
					case WEAPON_FLAMETHROWER:
						if (psPtr->Class != CLASS_INC_SPEC &&
							psPtr->Class != CLASS_MEDIC_FT) {
							return 0;
						}
						break;
					case WEAPON_SADAR:
						if (psPtr->Class != CLASS_EXF_SNIPER) {
							return 0;
						}
						break;
					case WEAPON_GRENADELAUNCHER:
						if (psPtr->Class != CLASS_ENGINEER) {
							return 0;
						}
						break;
					case WEAPON_MINIGUN:
						if (psPtr->Class != CLASS_ENGINEER) {
							return 0;
						}
						break;
					case WEAPON_FRISBEE_LAUNCHER:
						if (psPtr->Class != CLASS_AA_SPEC) {
							return 0;
						}
						break;
					default :
						break;
				}
			}
			switch(weaponID)
			{
				case WEAPON_PULSERIFLE:
				case WEAPON_AUTOSHOTGUN:
				case WEAPON_SMARTGUN:
				case WEAPON_FLAMETHROWER:
				case WEAPON_PLASMAGUN:
				case WEAPON_SADAR:
				case WEAPON_GRENADELAUNCHER:
				case WEAPON_MINIGUN:
				{
					/* The marine weapons all map to the 
					  weapon slot of the same number */
					weaponSlot = SlotForThisWeapon(weaponID);
					break;
				}
				case WEAPON_MARINE_PISTOL:
				{
					int pistolSlot;

					pistolSlot=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
	    			
	    			if (playerStatusPtr->WeaponSlot[pistolSlot].Possessed==1) {
						weaponSlot = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
					} else {
						weaponSlot = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
					}
					break;
				}
				default:
					weaponSlot = SlotForThisWeapon(weaponID);
					break;
			}
			
			break;
		}
       	case I_Predator:
		{
			weaponSlot = SlotForThisWeapon(weaponID);
			break;
		}
		case I_Alien:
		{
			weaponSlot = SlotForThisWeapon(weaponID);
			break;
		}

		default:
			LOCALASSERT(1==0);
			break;
	}

	
	
	/* if unable to find the correct weapon slot */
	if (weaponSlot == -1) return 0;
	
	{
		PLAYER_WEAPON_DATA *weaponPtr;
		TEMPLATE_WEAPON_DATA *twPtr;

	    	
	    /* init a pointer to the weapon's data */
	    weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);
		
		#if 0
		/* if weapon slot isn't empty, unable to pickup weapon */
		if (weaponPtr->Possessed!=0) return 0;

		/* add weapon to inventory */
		weaponPtr->Possessed = 1;
		#else

		/* Select new weapons. */
		if (weaponPtr->Possessed==0) {
			if(AutoWeaponChangeOn)
			{
        		playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo=(weaponSlot+1);
				WillSwitchToThisWeapon = TRUE;
			}
		}

		if (weaponID == WEAPON_PLASMAGUN || weaponID == WEAPON_AUTOSHOTGUN)
		{
			weaponPtr->Possessed = 1;
			weaponPtr->PrimaryRoundsRemaining = 1;
			return(1);
		}

		twPtr=&TemplateWeapon[weaponID];

		if(weaponID==WEAPON_PULSERIFLE) //get some secondary ammo (grenades) as well
		{
			if (AbleToPickupAmmo(twPtr->SecondaryAmmoID)) {
				/* AbleToPickupAmmo should load some grenades... */
				#if 0
				TEMPLATE_AMMO_DATA *templateAmmoPtr;

				/* Load weapon. */
				templateAmmoPtr = &TemplateAmmo[twPtr->SecondaryAmmoID];

				if ((weaponPtr->SecondaryRoundsRemaining==0)&&(weaponPtr->SecondaryMagazinesRemaining>0)) {
					weaponPtr->SecondaryRoundsRemaining = templateAmmoPtr->AmmoPerMagazine;
					weaponPtr->SecondaryMagazinesRemaining--;
				}
				#endif
			} 
		}
		if (twPtr->PrimaryAmmoID!=AMMO_NONE) {

			if (AbleToPickupAmmo(twPtr->PrimaryAmmoID)) {
				TEMPLATE_AMMO_DATA *templateAmmoPtr;

				weaponPtr->Possessed = 1;
				/* Load weapon. */
				templateAmmoPtr = &TemplateAmmo[twPtr->PrimaryAmmoID];

				if ((weaponPtr->PrimaryRoundsRemaining==0)&&(weaponPtr->PrimaryMagazinesRemaining>0)) {
					weaponPtr->PrimaryRoundsRemaining = templateAmmoPtr->AmmoPerMagazine;
					weaponPtr->PrimaryMagazinesRemaining--;
				}
				if (weaponPtr->WeaponIDNumber==WEAPON_TWO_PISTOLS) {
					if ((weaponPtr->SecondaryRoundsRemaining==0)&&(weaponPtr->SecondaryMagazinesRemaining>0)) {
						weaponPtr->SecondaryRoundsRemaining = templateAmmoPtr->AmmoPerMagazine;
						weaponPtr->SecondaryMagazinesRemaining--;
					}
				}
				return(1);
			} else {
				if (weaponPtr->Possessed==1) {
					/* Get a weapon, but no ammo */
					weaponPtr->Possessed = 1;
					return(1);
				} else {
					return(0);
				}
			}
		
		} else {
			/* if weapon slot isn't empty, unable to pickup weapon */
			if (weaponPtr->Possessed!=0) return 0;
			/* add weapon to inventory */
			weaponPtr->Possessed = 1;
			/* No ammo to load. */
		}
		
		#endif
	}
	/* successful */
	return 1;

}

/* KJL 15:59:48 03/10/97 - enum for health pickups required */
static int AbleToPickupHealth(int healthID)
{
	/* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

	{
		NPC_DATA *NpcData;
		NPC_TYPES PlayerType;

		if ((AvP.PlayerType == I_Alien) && (healthID != 2)) return 0;

		/* Marines can rescue hosts in singleplayer... */
		if ((AvP.PlayerType == I_Marine) && (healthID == 2) &&
			(AvP.Network == I_No_Network)) {
			NewOnScreenMessage("Civilian Rescued.");
			CastMarineBot(10);
			return 1;
		}
		/* ... as well as in multiplayer */
		if ((AvP.PlayerType == I_Marine) && (healthID == 2) &&
			(AvP.Network != I_No_Network)) {

			int myIndex=PlayerIdInPlayerList(AVPDPNetID);
			if(myIndex==NET_IDNOTINPLAYERLIST) return 0;

			netGameData.playerData[myIndex].playerScore += 3;
			AddNetMsg_ChatBroadcast("(AUTO) Rescued a cocooned civilian!",FALSE);
			AddNetMsg_ScoreChange(myIndex,myIndex);
			netGameData.teamScores[AvP.PlayerType]+=3;
			AddNetMsg_SpeciesScores();
			return 1;
		}
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
				/* KJL 17:25:18 28/11/98 - no medipacks for Predator! */
				return 0;

//				PlayerType=I_PC_Predator;
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

		/* Portable Medikit */
		if (healthID == 1) {
			if (playerStatusPtr->Medikit != 1) {
				playerStatusPtr->Medikit = 1;
				return 1;
			} else
				return 0;
		}
		if (Player->ObStrategyBlock->SBDamageBlock.IsOnFire) {
			Player->ObStrategyBlock->SBDamageBlock.IsOnFire=0;
		} else if (Player->ObStrategyBlock->SBDamageBlock.Health==NpcData->StartingStats.Health<<ONE_FIXED_SHIFT) {
			/* Already at max. */
			return(0);
		}

		Player->ObStrategyBlock->SBDamageBlock.Health=NpcData->StartingStats.Health<<ONE_FIXED_SHIFT;
		playerStatusPtr->Health=Player->ObStrategyBlock->SBDamageBlock.Health;
	}
	return 1;
}

/* KJL 15:59:48 03/10/97 - enum for armour pickups required */
static int AbleToPickupArmour(int armourID)
{
	/* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);
	
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
				break;
			}
			case(I_Predator):
			{
				return 0;
				break;
			}
			case(I_Alien):
			{
				return 0;
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

		if (Player->ObStrategyBlock->SBDamageBlock.Armour==NpcData->StartingStats.Armour<<ONE_FIXED_SHIFT) {
			/* Already at max. */
			if (armourID == 1) {
				playerStatusPtr->ArmorType = 1;
				playerStatusPtr->AirSupply = ONE_FIXED*600;
				return 1;
			} else {
				playerStatusPtr->ArmorType = 0;
				playerStatusPtr->AirSupply = 0;
				return 1;
			}
			return(0);
		}
		if (armourID == 1) {
			playerStatusPtr->ArmorType = 1;
			playerStatusPtr->AirSupply = ONE_FIXED*600;
		} else {
			playerStatusPtr->ArmorType = 0;
			playerStatusPtr->AirSupply = 0;
		}
		Player->ObStrategyBlock->SBDamageBlock.Armour=NpcData->StartingStats.Armour<<ONE_FIXED_SHIFT;
		playerStatusPtr->Armour=Player->ObStrategyBlock->SBDamageBlock.Armour;
	}
	return 1;
}

static int AbleToPickupStealth(int stealthID)
{	
	return(1);	
}

/* Andy 21/04/97 - enum for motion tracker upgrades required */
static int AbleToPickupMTrackerUpgrade(int mtrackerID)
{
	/* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

	if (AvP.PlayerType != I_Marine) return 0;

	if (playerStatusPtr->MTrackerType > 0) return 0;

	if (playerStatusPtr->MTrackerType == 0) {
		playerStatusPtr->MTrackerType = 1;
		playerStatusPtr->TrackerTimer = ONE_FIXED*600;
	}
	return 1;
}

static int AbleToPickupLiquid(int waterID, int integrity)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

	if (waterID == 2) {	//Mercury
		if ((playerStatusPtr->ArmorType < 1) || (ERE_Broken())){  //Does not have protection
			NPC_DATA *NpcData;
			NPC_TYPES PlayerType;

			switch(AvP.PlayerType) {
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
				return 0;
				break;
			}
			case(I_Alien):
			{
				return 0;
				break;
			}
			default:
			{
				LOCALASSERT(1==0);
				break;
			}
		} //Switch
		NpcData=GetThisNpcData(PlayerType);
		LOCALASSERT(NpcData);
		
		if (Player->ObStrategyBlock->SBDamageBlock.Armour < NpcData->StartingStats.Armour<<ONE_FIXED_SHIFT) {
			CauseDamageToObject(Player->ObStrategyBlock,&TemplateAmmo[AMMO_ALIEN_FRAG].MaxDamage[AvP.Difficulty], ONE_FIXED,NULL);
		}
		}
	} else if (waterID == 3) {	//Lava
		CauseDamageToObject(Player->ObStrategyBlock,&TemplateAmmo[AMMO_PLASMA].MaxDamage[AvP.Difficulty], ONE_FIXED,NULL);
		Player->ObStrategyBlock->SBDamageBlock.IsOnFire = 1;
		playerStatusPtr->fireTimer=PLAYER_ON_FIRE_TIME;
	} else if (waterID == 4) {	//Acid
		CauseDamageToObject(Player->ObStrategyBlock,&TemplateAmmo[AMMO_ALIEN_FRAG].MaxDamage[AvP.Difficulty], ONE_FIXED,NULL);
	} else if (waterID == 5) {	//Electrically charged
		CauseDamageToObject(Player->ObStrategyBlock,&TemplateAmmo[AMMO_PARTICLE_BEAM].MaxDamage[AvP.Difficulty], ONE_FIXED,NULL);

	/* Missionpoints */
	} else if (waterID == 7) {	// Marine Missionpoint
		if (AvP.PlayerType == I_Marine)
		{
			int myIndex=PlayerIdInPlayerList(AVPDPNetID);
			if(myIndex==NET_IDNOTINPLAYERLIST) return 0;

			netGameData.playerData[myIndex].playerScore += 10;
			AddNetMsg_ChatBroadcast("(AUTO) Marines have reached the Missionpoint!",FALSE);
			TeleportNetPlayerToAStartingPosition(Player->ObStrategyBlock, FALSE);
			AddNetMsg_ScoreChange(myIndex,myIndex);
			netGameData.teamScores[AvP.PlayerType]+=10;
			AddNetMsg_SpeciesScores();
			return 1;
		} else
			return 0;
	} else if (waterID == 8) {	// Alien Missionpoint
		if (AvP.PlayerType == I_Alien)
		{
			int myIndex=PlayerIdInPlayerList(AVPDPNetID);
			if(myIndex==NET_IDNOTINPLAYERLIST) return 0;

			netGameData.playerData[myIndex].playerScore += 10;
			AddNetMsg_ChatBroadcast("(AUTO) Aliens have reached the Missionpoint!",FALSE);
			TeleportNetPlayerToAStartingPosition(Player->ObStrategyBlock, FALSE);
			AddNetMsg_ScoreChange(myIndex,myIndex);
			netGameData.teamScores[AvP.PlayerType]+=10;
			AddNetMsg_SpeciesScores();
			return 1;
		} else
			return 0;
	} else if (waterID == 9) {	// Predator Missionpoint
		if (AvP.PlayerType == I_Predator)
		{
			int myIndex=PlayerIdInPlayerList(AVPDPNetID);
			if(myIndex==NET_IDNOTINPLAYERLIST) return 0;

			netGameData.playerData[myIndex].playerScore += 10;
			AddNetMsg_ChatBroadcast("(AUTO) Predators have reached the Missionpoint!",FALSE);
			TeleportNetPlayerToAStartingPosition(Player->ObStrategyBlock, FALSE);
			AddNetMsg_ScoreChange(myIndex,myIndex);
			netGameData.teamScores[AvP.PlayerType]+=10;
			AddNetMsg_SpeciesScores();
			return 1;
		} else
			return 0;

	/* Escape Points */
	} else if (waterID == 10) {	// Marine Escape Point
		if ((AvP.PlayerType == I_Marine) && (integrity == 200))
		{
			int myIndex=PlayerIdInPlayerList(AVPDPNetID);
			if(myIndex==NET_IDNOTINPLAYERLIST) return 0;

			netGameData.playerData[myIndex].playerScore += 10;
			AddNetMsg_ChatBroadcast("(AUTO) Managed to Escape!",FALSE);
			playerStatusPtr->IsAlive = 0;
			GetNextMultiplayerObservedPlayer();
			AddNetMsg_ScoreChange(myIndex,myIndex);
			netGameData.teamScores[AvP.PlayerType]+=10;
			AddNetMsg_SpeciesScores();
		} else
			return 0;
	} else if (waterID == 11) {	// Alien Escape Point
		if (AvP.PlayerType == I_Alien)
		{
			int myIndex=PlayerIdInPlayerList(AVPDPNetID);
			if(myIndex==NET_IDNOTINPLAYERLIST) return 0;

			netGameData.playerData[myIndex].playerScore += 10;
			AddNetMsg_ChatBroadcast("(AUTO) Managed to Escape!",FALSE);
			playerStatusPtr->IsAlive = 0;
			GetNextMultiplayerObservedPlayer();
			AddNetMsg_ScoreChange(myIndex,myIndex);
			netGameData.teamScores[AvP.PlayerType]+=10;
			AddNetMsg_SpeciesScores();
		} else
			return 0;
	} else if (waterID == 12) {	// Predator Escape Point
		if (AvP.PlayerType == I_Predator)
		{
			int myIndex=PlayerIdInPlayerList(AVPDPNetID);
			if(myIndex==NET_IDNOTINPLAYERLIST) return 0;

			netGameData.playerData[myIndex].playerScore += 10;
			AddNetMsg_ChatBroadcast("(AUTO) Managed to Escape!",FALSE);
			playerStatusPtr->IsAlive = 0;
			GetNextMultiplayerObservedPlayer();
			AddNetMsg_ScoreChange(myIndex,myIndex);
			netGameData.teamScores[AvP.PlayerType]+=10;
			AddNetMsg_SpeciesScores();
		} else
			return 0;
	} else if (waterID == 13) {	// TKOTH Point
		int myIndex=PlayerIdInPlayerList(AVPDPNetID);
		if(myIndex==NET_IDNOTINPLAYERLIST) return 0;

		// Update King Of The Hill timer
		if (KOTH > (ONE_FIXED*3))
		{
			KOTH -= 8192;
			return 0;
		} else {
			KOTH = 0;
		}
		netGameData.playerData[myIndex].playerScore += 1;
		KOTH = (ONE_FIXED*13);	// +1 point/10 seconds
		AddNetMsg_ScoreChange(myIndex,myIndex);
		netGameData.teamScores[AvP.PlayerType]+=1;
		AddNetMsg_SpeciesScores();
		return 0;
	} else if (waterID == 17) {	// Plants

		if (playerStatusPtr->soundHandle4)
			playerStatusPtr->OnSurface = 1;
		
	} else if (waterID == 18) {	// Glass
		return 0;
	} else {	//Regular water
		if (Player->ObStrategyBlock->SBDamageBlock.IsOnFire) {	//On fire? Put it out
			Player->ObStrategyBlock->SBDamageBlock.IsOnFire = 0;
		}
	}
	return 0;
}

static int AbleToPickupFlares(int flaresID)
{
	/* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

	if (AvP.PlayerType != I_Marine) return 0;
	
	playerStatusPtr->FlaresLeft = playerStatusPtr->FlaresLeft + 5;
	return 1;
}

static int AbleToPickupPGC(int pgcID)
{
	/* Cannot pick this up. It is the Alien Egg. */
	return 0;
}

/* SHOULDER LAMP PICKUP */
static int AbleToPickupIRGoggles(int goggleID)
{
	/* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

	if (AvP.PlayerType != I_Marine) return 0;
	
	if (playerStatusPtr->IRGoggles == 1) return 0;

	if (playerStatusPtr->IRGoggles == 0)
	{
		playerStatusPtr->IRGoggles = 1;
		playerStatusPtr->PGC = (ONE_FIXED*245);
	}
	return 1;
}

static int AbleToPickupSentry(int sentryID)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

	if (AvP.PlayerType != I_Marine) return 0;

	if (playerStatusPtr->IHaveAPlacedAutogun == 1) return 0;

	if (playerStatusPtr->IHaveAPlacedAutogun == 0)
		playerStatusPtr->IHaveAPlacedAutogun = 1;

	return 1;
}
/*--------------------------Patrick 11/3/97---------------------------
  A couple of functions to set and check player's security clearances:
  * The 'set' function takes the security level as an UNSIGNED INTEGER 
    parameter
  * The 'Return' function also takes the security level as an UNSIGNED 
    INTEGER parameter, and returns 1 if the player has clearance to the
    specified level, or 0 if not...

  NB valid security levels are 0 to 31.
  --------------------------------------------------------------------*/

void SetPlayerSecurityClearance(STRATEGYBLOCK *sbPtr, unsigned int securityLevel)
{
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);	
	GLOBALASSERT(securityLevel<32);

	/* completely horrible hack: if you're the predator, and level 1 is passed,
	you get all levels except for one. Of course. Obvious isn't it. */
	
	if((AvP.PlayerType==I_Predator)&&(securityLevel==1))
	{
		playerStatusPtr->securityClearances|=0xfffffffe;
	}
	else playerStatusPtr->securityClearances|=(1<<securityLevel);
}

int ReturnPlayerSecurityClearance(STRATEGYBLOCK *sbPtr, unsigned int securityLevel)
{
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);	
	GLOBALASSERT(securityLevel<32);
	
	return(playerStatusPtr->securityClearances&(1<<securityLevel));
}


/* Removes objects picked up by the player: either destroys them for a single
player game, or for a multiplayer game, sets them to respawn */
extern void RemovePickedUpObject(STRATEGYBLOCK *objectPtr)
{
	INANIMATEOBJECT_STATUSBLOCK* objStatPtr = objectPtr->SBdataptr;
	GLOBALASSERT(objectPtr->I_SBtype == I_BehaviourInanimateObject);

	/* patrick, for e3- add a sound effect to explosions */
	switch(objStatPtr->typeId)
	{
		case(IOT_Weapon):
			switch (AvP.PlayerType) {
				case I_Alien:
				default:
					Sound_Play(SID_PICKUP,NULL);
					break;
				case I_Marine:
					Sound_Play(SID_MARINE_PICKUP_WEAPON,NULL);
					break;
				case I_Predator:
					Sound_Play(SID_PREDATOR_PICKUP_WEAPON,NULL);
					break;
			}
			break;
		case(IOT_Ammo):
			Sound_Play(SID_MARINE_PICKUP_AMMO,NULL);
			break;
		case(IOT_Armour):
		case(IOT_DataTape):
		case(IOT_IRGoggles):
		case(IOT_MTrackerUpgrade):
		case(IOT_PheromonePod):
			Sound_Play(SID_MARINE_PICKUP_ARMOUR,NULL);
			break;
		case(IOT_FieldCharge):
			Sound_Play(SID_PREDATOR_PICKUP_FIELDCHARGE,NULL);
			break;
		default:
			Sound_Play(SID_PICKUP,NULL);
			break;
	}

	if (objStatPtr->ghosted_object) {
		/* Must be a runtime pickup... */
		#if SupportWindows95
		AddNetMsg_LocalObjectDestroyed(objectPtr);
		#endif
		DestroyAnyStrategyBlock(objectPtr);
		return;
	}

	//see if object has a target that should be notified upon being picked up
	if(objStatPtr->event_target)
	{
		if(objStatPtr->event_target->triggering_event & ObjectEventFlag_PickedUp)
		{
			RequestState(objStatPtr->event_target->event_target_sbptr,objStatPtr->event_target->request,0);
		}
	}
	
	if(AvP.Network==I_No_Network) DestroyAnyStrategyBlock(objectPtr);
	else
	{
		#if SupportWindows95
		AddNetMsg_ObjectPickedUp(&objectPtr->SBname[0]);
		#endif
		KillInanimateObjectForRespawn(objectPtr);
	}

}

int SlotForThisWeapon(enum WEAPON_ID weaponID) {

	PLAYER_STATUS *psptr;
	int a;

	psptr=(PLAYER_STATUS *)Player->ObStrategyBlock->SBdataptr;
	for (a=0; a<MAX_NO_OF_WEAPON_SLOTS; a++) {
		if (psptr->WeaponSlot[a].WeaponIDNumber==weaponID) {
			break;
		}
	}
	if (a!=MAX_NO_OF_WEAPON_SLOTS) {
		return(a);
	} else {
		return(-1);
	}
}

static int AbleToPickupFieldCharge(int chargeID)
{
	/* access the extra data hanging off the strategy block */
	NPC_TYPES PlayerType;
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

	switch(AvP.PlayerType) 
	{
		case(I_Marine):
		{
			return(0);
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
			return(0);
			break;
		}
		default:
		{
			LOCALASSERT(1==0);
			break;
		}
	}

	{
		int a;
		/* Add a medicomp 'round'. */

		a=SlotForThisWeapon(WEAPON_PRED_MEDICOMP);
		if (playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining<(MEDICOMP_MAX_AMMO)) {
	       	playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining+=(ONE_FIXED);
		}
	}

	if (playerStatusPtr->FieldCharge==PLAYERCLOAK_MAXENERGY) {
		/* Got max already. */
		return(0);
	} else {
		playerStatusPtr->FieldCharge=PLAYERCLOAK_MAXENERGY;
		return(1);
	}
}

void Convert_Disc_To_Pickup(STRATEGYBLOCK *sbPtr) {

	DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;
    PC_PRED_DISC_BEHAV_BLOCK *bbPtr = (PC_PRED_DISC_BEHAV_BLOCK * ) sbPtr->SBdataptr;
	SECTION_DATA *disc_section;
	INANIMATEOBJECT_STATUSBLOCK* objectStatusPtr;
	/* Transmogrify a disc behaviour to a disc ammo pickup! */

	disc_section=GetThisSectionData(bbPtr->HModelController.section_data,"disk");
	GLOBALASSERT(disc_section);

	/* Sort out dynamics block */
	dynPtr->LinVelocity.vx=0;
	dynPtr->LinVelocity.vy=0;
	dynPtr->LinVelocity.vz=0;

	dynPtr->LinImpulse.vx=0;
	dynPtr->LinImpulse.vy=0;
	dynPtr->LinImpulse.vz=0;

	dynPtr->OrientMat=disc_section->SecMat;
	dynPtr->Position=disc_section->World_Offset;
	MatrixToEuler(&dynPtr->OrientMat,&dynPtr->OrientEuler);

	sbPtr->shapeIndex=disc_section->sempai->ShapeNum;

	if (sbPtr->SBdptr) {
		sbPtr->SBdptr->ObShape=disc_section->sempai->ShapeNum;
		sbPtr->SBdptr->ObShapeData=disc_section->sempai->Shape;
		sbPtr->SBdptr->HModelControlBlock=NULL;
	}
	/* Create data block! */

	objectStatusPtr = (void *)AllocateMem(sizeof(INANIMATEOBJECT_STATUSBLOCK));

	objectStatusPtr->respawnTimer = 0; 
			
	/* these should be loaded */
	objectStatusPtr->typeId = IOT_Ammo;
	objectStatusPtr->subType = (int)AMMO_PRED_DISC;
	
	/* set default indestructibility */
	objectStatusPtr->Indestructable = No;
	objectStatusPtr->event_target=NULL;
	objectStatusPtr->fragments=NULL;
	objectStatusPtr->num_frags=0;
	objectStatusPtr->inan_tac=NULL;
	objectStatusPtr->ghosted_object=1;
	objectStatusPtr->explosionTimer=0;
	objectStatusPtr->lifespanTimer=0;

	/* Now swap it round... */
	Sound_Stop(bbPtr->soundHandle);
	Dispel_HModel(&bbPtr->HModelController);
	DeallocateMem(bbPtr);
	sbPtr->SBdataptr=(void *)objectStatusPtr;
	sbPtr->I_SBtype=I_BehaviourInanimateObject;

}

int ObjectIsDiscPickup(STRATEGYBLOCK *sbPtr) {

	if (sbPtr->I_SBtype == I_BehaviourInanimateObject) {

		INANIMATEOBJECT_STATUSBLOCK* objStatPtr = sbPtr->SBdataptr;
		
		/* Make sure the object hasn't already been picked up this frame */
		if(sbPtr->SBflags.please_destroy_me==0 && objStatPtr->respawnTimer==0)
		{
			if (objStatPtr->typeId==IOT_Ammo) {
				if (objStatPtr->subType==AMMO_PRED_DISC) {
					return(1);
				}
			}			
		}
	} else if (sbPtr->I_SBtype == I_BehaviourNetGhost) {
		NETGHOSTDATABLOCK* ghostData = sbPtr->SBdataptr;

		if (sbPtr->SBflags.please_destroy_me==0) {
			if (ghostData->type==I_BehaviourInanimateObject) {
				if (ghostData->IOType==IOT_Ammo) {
					if (ghostData->subtype==AMMO_PRED_DISC) {
						return(1);
					}
				}
			}
		}
	}
	
	return(0);

}

void Recall_Disc(void) {

	/* If you are a predator with NO discs, pick up the nearest one? */
	PLAYER_WEAPON_DATA *weaponPtr;
	int weaponSlot = -1;
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
    GLOBALASSERT(playerStatusPtr);

	if (AvP.PlayerType!=I_Predator) return;

	if (playerStatusPtr->FieldCharge<RecallDisc_Charge) return;
	
	weaponSlot = SlotForThisWeapon(WEAPON_PRED_DISC);

	if (weaponSlot == -1) return;
	
    /* init a pointer to the weapon's data */
    weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);

	if ((weaponPtr->PrimaryMagazinesRemaining==0)
		&& (weaponPtr->PrimaryRoundsRemaining==0)) {

		/* We have no discs: try to find a pickup. */
		STRATEGYBLOCK *nearest, *candidate;
		int neardist,a;

		nearest=NULL;
		neardist=10000000;

		for (a=0; a<NumActiveStBlocks; a++) {
			candidate=ActiveStBlockList[a];
	
			if (candidate->DynPtr) {

				/* Are we the right type? */
				if (ObjectIsDiscPickup(candidate)) {
					VECTORCH offset;
					int dist;

					offset.vx=Player->ObStrategyBlock->DynPtr->Position.vx-candidate->DynPtr->Position.vx;
					offset.vy=Player->ObStrategyBlock->DynPtr->Position.vy-candidate->DynPtr->Position.vy;
					offset.vz=Player->ObStrategyBlock->DynPtr->Position.vz-candidate->DynPtr->Position.vz;

					dist=Approximate3dMagnitude(&offset);
					
					if (dist<neardist) {
						nearest=candidate;
						neardist=dist;
					}
				} else if (candidate->I_SBtype==I_BehaviourPredatorDisc_SeekTrack) {
					VECTORCH offset;
					int dist;

					offset.vx=Player->ObStrategyBlock->DynPtr->Position.vx-candidate->DynPtr->Position.vx;
					offset.vy=Player->ObStrategyBlock->DynPtr->Position.vy-candidate->DynPtr->Position.vy;
					offset.vz=Player->ObStrategyBlock->DynPtr->Position.vz-candidate->DynPtr->Position.vz;

					dist=Approximate3dMagnitude(&offset);
					
					if (dist<neardist) {
						nearest=candidate;
						neardist=dist;
					}
				}
			}
		}
		/* Do we have a disc? */
		if (nearest) {
			/* Attempt to pick it up. */
			if (nearest->I_SBtype == I_BehaviourInanimateObject) {

				INANIMATEOBJECT_STATUSBLOCK* objStatPtr = nearest->SBdataptr;
		
				/* Make sure the object hasn't already been picked up this frame */
				GLOBALASSERT(nearest->SBflags.please_destroy_me==0 && objStatPtr->respawnTimer==0);
				GLOBALASSERT(objStatPtr->typeId==IOT_Ammo);
				GLOBALASSERT(objStatPtr->subType==AMMO_PRED_DISC);

				if (AbleToPickupAmmo(objStatPtr->subType)) {
					RemovePickedUpObject(nearest);
					playerStatusPtr->FieldCharge-=RecallDisc_Charge;
					CurrentGameStats_ChargeUsed(RecallDisc_Charge);
					#if 0
					NewOnScreenMessage("RECALLED DISC");
					#endif
					Sound_Play(SID_PREDATOR_DISK_RECOVERED,"h");
					AutoSwapToDisc_OutOfSequence();
				}

			} else if (nearest->I_SBtype == I_BehaviourNetGhost) {
				NETGHOSTDATABLOCK* ghostData = nearest->SBdataptr;

				GLOBALASSERT(nearest->SBflags.please_destroy_me==0);
				GLOBALASSERT(ghostData->type==I_BehaviourInanimateObject);
				GLOBALASSERT(ghostData->IOType==IOT_Ammo);
				GLOBALASSERT(ghostData->subtype==AMMO_PRED_DISC);

				if (AbleToPickupAmmo(ghostData->subtype)) {
					AddNetMsg_LocalObjectDestroyed_Request(nearest);
					ghostData->IOType=IOT_Non;
					/* So it's not picked up again... */
					playerStatusPtr->FieldCharge-=RecallDisc_Charge;
					CurrentGameStats_ChargeUsed(RecallDisc_Charge);
					#if 0
					NewOnScreenMessage("RECALLED DISC");
					#endif
					Sound_Play(SID_PREDATOR_DISK_RECOVERED,"h");
					AutoSwapToDisc_OutOfSequence();
				}

			} else if (nearest->I_SBtype == I_BehaviourPredatorDisc_SeekTrack) {

			    PC_PRED_DISC_BEHAV_BLOCK *bbPtr = (PC_PRED_DISC_BEHAV_BLOCK * ) nearest->SBdataptr;

				if (AbleToPickupAmmo(AMMO_PRED_DISC)) {
					/* Pick it up. */
						
					AutoSwapToDisc_OutOfSequence();
				
					Sound_Stop(bbPtr->soundHandle);
					Sound_Play(SID_PREDATOR_DISK_RECOVERED,"h");

					#if SupportWindows95
					if(AvP.Network != I_No_Network)	AddNetMsg_LocalObjectDestroyed(nearest);
					#endif
				    DestroyAnyStrategyBlock(nearest);	
				
				}

			} else {
				GLOBALASSERT(0);
			}
		} else {
			#if 0
			NewOnScreenMessage("FOUND NO DISCS");
			#endif
		}
	}
}

int ObjectIsPlayersDisc(STRATEGYBLOCK *sbPtr) {

	if (sbPtr->I_SBtype == I_BehaviourInanimateObject) {

		INANIMATEOBJECT_STATUSBLOCK* objStatPtr = sbPtr->SBdataptr;
		
		/* Make sure the object hasn't already been picked up this frame */
		if(sbPtr->SBflags.please_destroy_me==0 && objStatPtr->respawnTimer==0)
		{
			if (objStatPtr->typeId==IOT_Ammo) {
				if (objStatPtr->subType==AMMO_PRED_DISC) {
					return(1);
				}
			}			
		}
	} else if (sbPtr->I_SBtype==I_BehaviourPredatorDisc_SeekTrack) {
		/* Erm... just return? */
		return(1);
	}

	return(0);
}

void RemoveAllThisPlayersDiscs(void) {

	STRATEGYBLOCK *candidate;
	int a;

	/* All discs that are NOT ghosted must 'belong' to this player. */

	for (a=0; a<NumActiveStBlocks; a++) {
		candidate=ActiveStBlockList[a];

		if (candidate->DynPtr) {

			/* Are we the right type? */
			if (ObjectIsPlayersDisc(candidate)) {
				#if SupportWindows95
				AddNetMsg_LocalObjectDestroyed(candidate);
				#endif
				DestroyAnyStrategyBlock(candidate);
			}
		}
	}
}

void Reload_Weapon(void)
{
  PLAYER_WEAPON_DATA *weaponPtr;
  PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
  GLOBALASSERT(playerStatusPtr);
  GLOBALASSERT(playerStatusPtr->SelectedWeaponSlot<MAX_NO_OF_WEAPON_SLOTS);

  /* Only Marines can reload their weapons */
  if (AvP.PlayerType == I_Alien) return;

  /* Predators get Reversed Cycle Vision Modes */
  if (AvP.PlayerType == I_Predator) {
    playerStatusPtr->Mvt_InputRequests.Flags.Rqst_ReverseCycleVisionMode = 1;
    return;
  }
  /* Player is dead... no reload */
  if (!playerStatusPtr->IsAlive) return;

  /* Initialize pointer to player's weapon */ 
  weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);

  /* No weapon, no reload */
  if (weaponPtr->WeaponIDNumber == NULL_WEAPON) return;

  if (weaponPtr->PrimaryMagazinesRemaining) {
    weaponPtr->PrimaryRoundsRemaining = 0;
    playerStatusPtr->Mvt_InputRequests.Flags.Rqst_FirePrimaryWeapon = 1;
  }
}
