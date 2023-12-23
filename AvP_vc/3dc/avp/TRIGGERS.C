/*KJL***********************************************************************
* I know "triggers.c" is a strange name, but I did write this on a Monday. *
***********************************************************************KJL*/
#include "3dc.h"
#include "inline.h"
#include "module.h"					  

#include "stratdef.h"
#include "gamedef.h"
#include "dynblock.h"
#include "bh_types.h"
#include "huddefs.h"
#include "triggers.h"
#include "pldnet.h"
#include "pldghost.h"
#include "bh_agun.h"
#include "bh_marin.h"
#include "bh_plift.h"

#define UseLocalAssert Yes
#include "ourasert.h"

/* in mm */
#define ACTIVATION_Z_RANGE 3000
#define ACTIVATION_X_RANGE 1000
#define ACTIVATION_Y_RANGE 1000

#define OP_Z (ACTIVATION_Z_RANGE)
#define OP_X (ACTIVATION_X_RANGE)
#define OP_Y (ACTIVATION_Y_RANGE)

int Operable;
extern int HackPool;
extern int Kit[3];

extern void HealPlayer();
extern void RepairPlayer();
extern void CauseDamageToObject(STRATEGYBLOCK *sbPtr,DAMAGE_PROFILE *damage,int multiple,VECTORCH *incoming);
extern int IsThisObjectVisibleFromThisPosition_WithIgnore(DISPLAYBLOCK *objectPtr, DISPLAYBLOCK *ignoredObjectPtr, VECTORCH *positionPtr, int maxRange);
extern void ConvertToArmedCivvie(STRATEGYBLOCK *sbPtr, int weapon);

extern int NumOnScreenBlocks;
extern DISPLAYBLOCK *OnScreenBlockList[];
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;

int ValidDoorExists(void)
{
	int numberOfObjects = NumOnScreenBlocks;

	DISPLAYBLOCK *nearestObjectPtr=0;
	int nearestMagnitude=1000*1000 + 1000*1000;

	while (numberOfObjects)
	{
		DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numberOfObjects];
		GLOBALASSERT(objectPtr);

		/* Strategyblock? */
		if (objectPtr->ObStrategyBlock)
		{
			AVP_BEHAVIOUR_TYPE behaviour = objectPtr->ObStrategyBlock->I_SBtype;

			/* Platform Lift? */
			if (I_BehaviourPlatform == behaviour)
			{
				if (objectPtr->ObView.vz > 0 && objectPtr->ObView.vz < 1000)
				{
					int absX = objectPtr->ObView.vx;
					int absY = objectPtr->ObView.vy;

					if (absX < 0) absX=-absX;
					if (absY < 0) absY=-absY;

					if (absX < 1000 && absY < 1000)
					{
						int magnitude = (absX*absX + absY*absY);

						if (nearestMagnitude > magnitude)
						{
							nearestMagnitude = magnitude;
							nearestObjectPtr = objectPtr;
						}
					}
				}
			}
		}
	}
	if (nearestObjectPtr)
	{
		PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
		LOCALASSERT(playerStatusPtr);

		if (IsThisObjectVisibleFromThisPosition_WithIgnore(Player,nearestObjectPtr,&nearestObjectPtr->ObWorld,10000))
		{
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

void WeldDoor(unsigned int Cutting)
{
	int numberOfObjects = NumOnScreenBlocks;

	DISPLAYBLOCK *nearestObjectPtr=0;
	int nearestMagnitude=1000*1000 + 1000*1000;

	while (numberOfObjects)
	{
		DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numberOfObjects];
		GLOBALASSERT(objectPtr);

		/* Strategyblock? */
		if (objectPtr->ObStrategyBlock)
		{
			AVP_BEHAVIOUR_TYPE behaviour = objectPtr->ObStrategyBlock->I_SBtype;

			/* Platform Lift? */
			if (I_BehaviourPlatform == behaviour)
			{
				if (objectPtr->ObView.vz > 0 && objectPtr->ObView.vz < 1000)
				{
					int absX = objectPtr->ObView.vx;
					int absY = objectPtr->ObView.vy;

					if (absX < 0) absX=-absX;
					if (absY < 0) absY=-absY;

					if (absX < 1000 && absY < 1000)
					{
						int magnitude = (absX*absX + absY*absY);

						if (nearestMagnitude > magnitude)
						{
							nearestMagnitude = magnitude;
							nearestObjectPtr = objectPtr;
						}
					}
				}
			}
		}
	}
	if (nearestObjectPtr)
	{
		PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
		LOCALASSERT(playerStatusPtr);

		if (IsThisObjectVisibleFromThisPosition_WithIgnore(Player,nearestObjectPtr,&nearestObjectPtr->ObWorld,10000))
		{
			PLATFORMLIFT_BEHAVIOUR_BLOCK *platformliftdata;
			platformliftdata = (PLATFORMLIFT_BEHAVIOUR_BLOCK *)nearestObjectPtr->ObStrategyBlock->SBdataptr;

			/* Welding */
			if (!Cutting)
			{
				platformliftdata->Enabled = 0;
			/* Cutting */
			} else {
				platformliftdata->OneUse = 1;
				platformliftdata->Enabled = 1;
			}
		}
	}
}

void CheckOperableObject(void)
{
	int numberOfObjects = NumOnScreenBlocks;

	DISPLAYBLOCK *nearestObjectPtr=0;
	int nearestMagnitude=OP_X*OP_X + OP_Y*OP_Y;

	while (numberOfObjects)
	{
		DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numberOfObjects];
		GLOBALASSERT(objectPtr);

		/* Strategyblock? */
		if (objectPtr->ObStrategyBlock)
		{
			AVP_BEHAVIOUR_TYPE behaviour = objectPtr->ObStrategyBlock->I_SBtype;

			/* Operable? */
			if ((I_BehaviourBinarySwitch == behaviour) ||
				(I_BehaviourLinkSwitch == behaviour) ||
				(I_BehaviourAutoGun == behaviour))
			{
				if (objectPtr->ObView.vz > 0 && objectPtr->ObView.vz < OP_Z)
				{
					int absX = objectPtr->ObView.vx;
					int absY = objectPtr->ObView.vy;

					if (absX < 0) absX=-absX;
					if (absY < 0) absY=-absY;

					if (absX < OP_X && absY < OP_Y)
					{
						int magnitude = (absX*absX + absY*absY);

						if (nearestMagnitude > magnitude)
						{
							nearestMagnitude = magnitude;
							nearestObjectPtr = objectPtr;
						}
					}
				}
			}
		}
	}
	if (nearestObjectPtr)
	{
		PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
		LOCALASSERT(playerStatusPtr);

		if (IsThisObjectVisibleFromThisPosition_WithIgnore(Player,nearestObjectPtr,&nearestObjectPtr->ObWorld,10000))
		{
			// Read the strategyblock's name. If it contains the phrase
			// "hack", then consider the switch to be a hackable switch.
			if (AvP.Network != I_No_Network)
			{
				if (strstr(nearestObjectPtr->ObStrategyBlock->name,"hack"))
				{
					if ((Kit[0] == KIT_COMTECH) || (Kit[1] == KIT_COMTECH))
						Operable=3;
					else
						Operable=1;
				} else {
					Operable=2;
				}
			} else {
				// Hack that fixes Sentry Guns.
				if (nearestObjectPtr->ObStrategyBlock->I_SBtype == I_BehaviourAutoGun)
				{
					Operable=2;
					return;
				}
				if (strstr(nearestObjectPtr->ObStrategyBlock->name,"hack"))
					Operable=3;
				else
					Operable=2;
			}
		}
	} else {
		Operable=0;
		if (HackPool != 0)
			HackPool=0;
	}
}

void OperateObjectInLineOfSight(void)
{
	int numberOfObjects = NumOnScreenBlocks;
	
	DISPLAYBLOCK *nearestObjectPtr=0;
	int nearestMagnitude=ACTIVATION_X_RANGE*ACTIVATION_X_RANGE + ACTIVATION_Y_RANGE*ACTIVATION_Y_RANGE;
		
	if (AvP.PlayerType == I_Alien || AvP.PlayerType == I_Predator)
		return;

	while (numberOfObjects)
	{
		DISPLAYBLOCK* objectPtr = OnScreenBlockList[--numberOfObjects];
		GLOBALASSERT(objectPtr);
		
		/* does object have a strategy block? */
		if (objectPtr->ObStrategyBlock)
		{		
			AVP_BEHAVIOUR_TYPE behaviour = objectPtr->ObStrategyBlock->I_SBtype;
			
			/* is it operable? */
			if ((I_BehaviourBinarySwitch == behaviour)
			  ||(I_BehaviourLinkSwitch == behaviour) 
			  ||(I_BehaviourAutoGun == behaviour) 
			  ||(I_BehaviourDatabase == behaviour)
			  ||(I_BehaviourMarine == behaviour))
			{
				/* is it in range? */
				if (objectPtr->ObView.vz > 0 && objectPtr->ObView.vz < ACTIVATION_Z_RANGE)
				{
					int absX = objectPtr->ObView.vx;
				   	int absY = objectPtr->ObView.vy;

					if (absX<0) absX=-absX;
					if (absY<0) absY=-absY;
					
					if (absX < ACTIVATION_X_RANGE && absY < ACTIVATION_Y_RANGE)
					{
						int magnitude = (absX*absX + absY*absY);

						if (nearestMagnitude > magnitude)
						{
							nearestMagnitude = magnitude;
							nearestObjectPtr = objectPtr;
						}
					}
				}

				/* Alternate range-scan for AI */
				if (behaviour == I_BehaviourMarine)
				{
					SECTION_DATA *head;
					MARINE_STATUS_BLOCK *mPtr = (MARINE_STATUS_BLOCK *)objectPtr->ObStrategyBlock->SBdataptr;

					head = GetThisSectionData(mPtr->HModelController.section_data,"head");
					if (head)
					{
						VECTORCH temp;
						int dist;

						temp = head->View_Offset;
						temp.vz = 0;
						dist=Approximate3dMagnitude(&temp);
						if (dist < OP_Z) {
							nearestObjectPtr = objectPtr;
						}
					}
				}
			}
		}
	}

	/* if we found a suitable object, operate it */
	if (nearestObjectPtr)
	{
		PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
		LOCALASSERT(playerStatusPtr);

		//only allow activation if you have a line of sight to the switch
		//allow the switch to be activated anyway for the moment
		if(IsThisObjectVisibleFromThisPosition_WithIgnore(Player,nearestObjectPtr,&nearestObjectPtr->ObWorld,10000))
		{
			switch(nearestObjectPtr->ObStrategyBlock->I_SBtype)
			{
				case I_BehaviourBinarySwitch:
				{
					if (strstr(nearestObjectPtr->ObStrategyBlock->name,"hack"))
					{
						// If the switch's name contains "hack", then
						// it is hackable.
						if (HackPool < 10)
							return;
					}
					#if SupportWindows95
					if(AvP.Network!=I_No_Network)
					{
						AddNetMsg_LOSRequestBinarySwitch(nearestObjectPtr->ObStrategyBlock);
					}
					#endif
					RequestState(nearestObjectPtr->ObStrategyBlock,1, 0);
					break;
				}
				case I_BehaviourLinkSwitch:
				{
					if (strstr(nearestObjectPtr->ObStrategyBlock->name,"hack"))
					{
						// If the switch's name contains "hack", then
						// it is hackable.
						if (HackPool < 10)
							return;
					}
					if(AvP.Network!=I_No_Network)
					{
						AddNetMsg_LOSRequestBinarySwitch(nearestObjectPtr->ObStrategyBlock);
					}
					RequestState(nearestObjectPtr->ObStrategyBlock,1, 0);
					break;
				}
				case I_BehaviourAutoGun:
				{
					AUTOGUN_STATUS_BLOCK *agunStatusPointer;
					agunStatusPointer = (AUTOGUN_STATUS_BLOCK*)nearestObjectPtr->ObStrategyBlock->SBdataptr;
				
					if (agunStatusPointer->behaviourState==I_inactive)
					{
						RequestState(nearestObjectPtr->ObStrategyBlock,1, 0);
					} 
					else if (agunStatusPointer->behaviourState==I_tracking)
					{
						RequestState(nearestObjectPtr->ObStrategyBlock,0, 0);
					}
					break;
				}
				case I_BehaviourDatabase:
				{
					#if PC_E3DEMO||PSX_DEMO
					/* KJL 10:56:39 05/28/97 - E3DEMO change */
					/* KJL 10:55:44 05/28/97 - display 'Access Denied' message */
					NewOnScreenMessage(GetTextString(TEXTSTRING_DB_ACCESSDENIED));
					#else
					AvP.GameMode = I_GM_Menus;

					#if PSX
					{
						extern int Global_Database_Num;
						Global_Database_Num=((DATABASE_BLOCK *)nearestObjectPtr->ObStrategyBlock->SBdataptr)->num;
					}
					#else
						AvP.DatabaseAccessNum=((DATABASE_BLOCK *)nearestObjectPtr->ObStrategyBlock->SBdataptr)->num;
					#endif
					

					/* KJL 16:43:01 03/19/97 - CHANGE ME! Need to pass database number */
					// RJHG - would be ((*DATABASEBLOCK)nearestObjectPtr->ObStrategyBlock->SBDataPtr)->num
					// CDF - I think it would be ((DATABASE_BLOCK *)nearestObjectPtr->ObStrategyBlock->SBdataptr)->num
					#endif
					
					break;
				}
				/* Making AI follow you around, -- Eld */
				//
				// Will make a full interaction system out of this, just have
				// to check with the rest of the team. I'd like a menu to appear
				// when you 'operate' with an AI, where you can select from Calm
				// Down, Follow and Give Weapon commands.
				//
				case I_BehaviourMarine:
				{
					extern void Marine_Enter_Return_State(STRATEGYBLOCK *sbPtr);
					extern void Marine_TauntShout(STRATEGYBLOCK *sbPtr);
					MARINE_STATUS_BLOCK *mPtr = (MARINE_STATUS_BLOCK *)nearestObjectPtr->ObStrategyBlock->SBdataptr;
					PLAYER_STATUS *psPtr = (PLAYER_STATUS *) Player->ObStrategyBlock->SBdataptr;
					PLAYER_WEAPON_DATA *weaponPtr = &(psPtr->WeaponSlot[psPtr->SelectedWeaponSlot]);

					// A civvie :)
					if (mPtr && !mPtr->My_Weapon->ARealMarine)
					{
						// Retreating.
						if (mPtr->behaviourState == MBS_Retreating)
						{
							NewOnScreenMessage("Come to your senses boy!");
							Marine_Enter_Return_State(nearestObjectPtr->ObStrategyBlock);
							return;
						}
						// Panicking.
						if (mPtr->behaviourState == MBS_PanicFire)
						{
							NewOnScreenMessage("Take it easy...");
							Marine_Enter_Return_State(nearestObjectPtr->ObStrategyBlock);
							return;
						}
						// Give weapon, based on currently used weapon.
						// Not Working! Crashing, pointing to Elevation...
#if 0
						if (mPtr->Mission == MM_NonCom)
						{
							NewOnScreenMessage("Giving weapon...");
							ConvertToArmedCivvie(nearestObjectPtr->ObStrategyBlock, weaponPtr->WeaponIDNumber);
							return;
						}
#endif
						// Follow.
						if (mPtr->behaviourState != MBS_PanicFire &&
							mPtr->behaviourState != MBS_Retreating)
						{
							NewOnScreenMessage("Come with me!");
							mPtr->Target = Player->ObStrategyBlock;
							Marine_TauntShout(nearestObjectPtr->ObStrategyBlock);
							return;
						}
					}
					break;
				}
				default:
					break;
			}
		}
	}
    
	return;
}


BOOL AnythingInMyModule(MODULE* my_mod)
{
	
		// simple overlap test


	//	this used within level - find objects in module
	// all will have sbs

	int i;
	int max_x, min_x, max_y, min_y, max_z, min_z;	

	max_x = my_mod->m_maxx + my_mod->m_world.vx;
	min_x = my_mod->m_minx + my_mod->m_world.vx;
	max_y = my_mod->m_maxy + my_mod->m_world.vy;
	min_y = my_mod->m_miny + my_mod->m_world.vy;
	max_z = my_mod->m_maxz + my_mod->m_world.vz;
	min_z = my_mod->m_minz + my_mod->m_world.vz;


	for(i = 0; i < NumActiveStBlocks; i++)
		{
			VECTORCH obj_world;
			STRATEGYBLOCK	*sbptr;
			DYNAMICSBLOCK	*dynptr;			

			sbptr = ActiveStBlockList[i];

			if(!(dynptr = sbptr->DynPtr))
				continue;
			
			obj_world = dynptr->Position;

			if(obj_world.vx < max_x)
				if(obj_world.vx > min_x)
					if(obj_world.vz < max_z)
						if(obj_world.vz > min_z)
							if(obj_world.vy < max_y)
								if(obj_world.vy > min_y)
									{
										return(1);
									}
		}

	return(0);
}	