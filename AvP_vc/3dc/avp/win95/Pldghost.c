/* Patrick 14/7/97----------------------------
  Source for Multi-Player ghost object support header
d  ----------------------------------------------------------------------*/
#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"
#include "dynblock.h"
#include "bh_debri.h"
#include "bh_alien.h"
#include "bh_pred.h"
#include "bh_marin.h"
#include "bh_fhug.h"
#include "pvisible.h"
#include "pldnet.h"
#include "pldghost.h"
#include "lighting.h"
#include "psnd.h"
#include "load_shp.h"
#include "projload.hpp"
#include "particle.h"
#include "sfx.h"
#include "psndplat.h"
#include "bh_corpse.h"
#include "bh_weap.h"
#include "ShowCmds.h"
#include "DetailLevels.h"

#define UseLocalAssert Yes
#include "ourasert.h"

/*----------------------------------------------------------------------
  Global Variables
  ----------------------------------------------------------------------*/


/*----------------------------------------------------------------------
  External globals
  ----------------------------------------------------------------------*/
extern int NormalFrameTime;
extern int GlobalFrameCounter;
extern int cosine[], sine[];
extern ACTIVESOUNDSAMPLE ActiveSounds[];
extern DEATH_DATA Alien_Deaths[];
extern HITLOCATIONTABLE *GetThisHitLocationTable(char *id);

extern MATRIXCH Identity_RotMat; /* From HModel.c */
extern void MakePlasmaTrailParticles(DYNAMICSBLOCK *dynPtr, int number);
extern int ValidTargetForProxMine(STRATEGYBLOCK *obstaclePtr);
extern void CauseDamageToObject(STRATEGYBLOCK *sbPtr, DAMAGE_PROFILE *damage, int multiple, VECTORCH *incoming);
extern void DoAlienLimbLossSound(VECTORCH *position);
extern int TotalKineticDamage(DAMAGE_PROFILE *damage);
extern void ModExSys(HMODELCONTROLLER *hmodel, char *model1, char *model2);

/*-----------------------------------------------------------------------
  Prototypes
  ----------------------------------------------------------------------*/

static void SetPlayerGhostAnimationSequence(STRATEGYBLOCK *sbPtr, int sequence, int special);
static void InitPlayerGhostAnimSequence(STRATEGYBLOCK *sbPtr);
static void UpdatePlayerGhostAnimSequence(STRATEGYBLOCK *sbPtr, int sequence, int special);
int DetermineMarineVoice(unsigned int Class);

SOUND3DDATA Ghost_Explosion_SoundData={
	{0,0,0,},
	{0,0,0,},
	15000,
	150000,
};

void UpdateObjectTrails(STRATEGYBLOCK *sbPtr);
void CreateMarineHModel(NETGHOSTDATABLOCK *ghostDataPtr, int weapon);
void CreateAlienHModel(NETGHOSTDATABLOCK *ghostDataPtr,int alienType);
void CreatePredatorHModel(NETGHOSTDATABLOCK *ghostDataPtr, int weapon);
static void CalculatePosnForGhostAutoGunMuzzleFlash(STRATEGYBLOCK *sbPtr,VECTORCH *position, EULER *orientation);
void UpdateAlienAIGhostAnimSequence(STRATEGYBLOCK *sbPtr,HMODEL_SEQUENCE_TYPES type, int subtype, int length, int tweeningtime);

/*-----------------------------------------------------------------------
  Functions...
  ----------------------------------------------------------------------*/

/* updates an existing ghost's position and orientation: Before calling this
need to use FindGhost to locate the ghost (which confirms it's existance) */
void UpdateGhost(STRATEGYBLOCK *sbPtr,VECTORCH *position,EULER *orientation,int sequence, int special)
{
	NETGHOSTDATABLOCK *ghostData;

	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	
	/* for visibility support: as ghosts can be moved when invisible, we need to work out
	which module they're in	whenever we update them. We must be carefull, however, not
	to set the containingModule to NULL if the object has moved outside the env, as 
	the	visibility system expects that we at least know what module any object WAS in,
	even if we do not now... thus, if we cannot find a containing module, we abort the update */
	
	/* KJL 21:01:09 23/05/98 - I've put this test here because the player's image in a mirror goes
	throught this code, and it's obviously going to be outside the environment */
	if (sbPtr->I_SBtype==I_BehaviourNetGhost)
	{
		MODULE *myContainingModule = ModuleFromPosition(position, (sbPtr->containingModule));
		if(myContainingModule==NULL)
		{
			//Not in any module , so don't try updating ghost,
			//except for various projectiles which can be shot 
			//out of the environment semi-legitamately
			if(ghostData->type!=I_BehaviourGrenade &&  
			   ghostData->type!=I_BehaviourRocket &&
			   ghostData->type!=I_BehaviourPPPlasmaBolt &&
			   ghostData->type!=I_BehaviourSpeargunBolt &&
			   ghostData->type!=I_BehaviourThrownSpear &&
			   ghostData->type!=I_BehaviourPredatorDisc_SeekTrack &&
			   ghostData->type!=I_BehaviourPredatorEnergyBolt &&
			   ghostData->type!=I_BehaviourPulseGrenade &&
			   ghostData->type!=I_BehaviourFlareGrenade &&
			   ghostData->type!=I_BehaviourFragmentationGrenade &&
			   ghostData->type!=I_BehaviourClusterGrenade &&
			   ghostData->type!=I_BehaviourFrisbeeEnergyBolt &&
			   ghostData->type!=I_BehaviourProximityGrenade)
					return;	
		}
		else
		{
			sbPtr->containingModule = myContainingModule;
		}
	}

	/* CDF 29/7/98 I'll assume that stationary discs are stuck. */
	if (ghostData->type==I_BehaviourPredatorDisc_SeekTrack) {
		DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;
		if ((dynPtr->Position.vx==dynPtr->PrevPosition.vx)&&
			(dynPtr->Position.vy==dynPtr->PrevPosition.vy)&&
			(dynPtr->Position.vz==dynPtr->PrevPosition.vz)) {
			ghostData->HModelController.Playing=0;

			
		} else {
			extern void NewTrailPoint(DYNAMICSBLOCK *dynPtr);
			ghostData->HModelController.Playing=1;
			//draw the disc's trail
			NewTrailPoint(dynPtr);

			//update sound for disc
			if(ghostData->SoundHandle!=SOUND_NOACTIVEINDEX) {
				Sound_Update3d(ghostData->SoundHandle,&(sbPtr->DynPtr->Position));
			} else {
			 	Sound_Play(SID_PREDATOR_DISK_FLYING,"del",&(sbPtr->DynPtr->Position),&ghostData->SoundHandle);
				
			}
		}
	}

	if (ghostData->type==I_BehaviourFrisbee) {
		//update sound for disc
		if(ghostData->SoundHandle!=SOUND_NOACTIVEINDEX) {
			Sound_Update3d(ghostData->SoundHandle,&(sbPtr->DynPtr->Position));
		} else {
		 	Sound_Play(SID_ED_SKEETERDISC_SPIN,"del",&(sbPtr->DynPtr->Position),&ghostData->SoundHandle);
		}
	}

	/* update the dynamics block */
	{
		DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;
//		dynPtr->Position = dynPtr->PrevPosition = *position;
		dynPtr->PrevPosition = dynPtr->Position;
		dynPtr->PrevOrientMat = dynPtr->OrientMat;
		dynPtr->Position = *position;
		dynPtr->OrientEuler = *orientation;
		CreateEulerMatrix(&dynPtr->OrientEuler,&dynPtr->OrientMat);
		TransposeMatrixCH(&dynPtr->OrientMat);
	}
	UpdateObjectTrails(sbPtr);
	#if 0	
	if (ghostData->type == I_BehaviourPredatorEnergyBolt)
	{
	 	MakePlasmaTrailParticles(sbPtr->DynPtr,32);
	}
	#endif


	/* if we're a player type, update the animation sequence */
	if((ghostData->type==I_BehaviourMarinePlayer)||
	   (ghostData->type==I_BehaviourAlienPlayer)||
	   (ghostData->type==I_BehaviourPredatorPlayer))
	{
		if(sequence!=-1)
		{
			UpdatePlayerGhostAnimSequence(sbPtr,sequence, special);
		}
   	}					   

	/* KJL 15:59:59 26/11/98 - no pheromone trails */
	#if 0
	if (AvP.PlayerType == I_Alien)
	{
		if ((ghostData->type==I_BehaviourMarinePlayer)
		  ||(ghostData->type==I_BehaviourPredatorPlayer))
		NewTrailPoint(sbPtr->DynPtr);
	}
	#endif

	/* KJL 16:58:04 17/06/98 - we want to update anims differently for NPCS */
	if((ghostData->type==I_BehaviourMarine)||
	   (ghostData->type==I_BehaviourAlien)||
	   (ghostData->type==I_BehaviourPredator))
	{
		UpdatePlayerGhostAnimSequence(sbPtr,sequence, special);
	}
	
	/* refresh integrity */
	ghostData->integrity = GHOST_INTEGRITY;		

}
void UpdateObjectTrails(STRATEGYBLOCK *sbPtr)
{
	NETGHOSTDATABLOCK *ghostDataPtr;

    LOCALASSERT(sbPtr);
	
	ghostDataPtr = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostDataPtr);

	LOCALASSERT(sbPtr->DynPtr);

	switch(ghostDataPtr->type)
	{
		case I_BehaviourPulseGrenade:
		{
			MakeRocketTrailParticles(&(sbPtr->DynPtr->PrevPosition), &(sbPtr->DynPtr->Position));
			break;
		}
		/*case I_BehaviourRocket:
		{
			MakeRocketTrailParticles(&(sbPtr->DynPtr->PrevPosition), &(sbPtr->DynPtr->Position));
			break;
		}*/
		case I_BehaviourPredatorEnergyBolt:
		{
		  	MakePlasmaTrailParticles(sbPtr->DynPtr,32);
			break;
		}
		case I_BehaviourFrisbeeEnergyBolt:
		{
		  	MakeDewlineTrailParticles(sbPtr->DynPtr,32);
			break;
		}
		case I_BehaviourFrisbee:
		{
			{
				int l;
				for (l=0;l<4;l++)
					MakeFlareParticle(sbPtr->DynPtr);
			}	
			break;
		}
		case I_BehaviourGrenade:
		case I_BehaviourProximityGrenade:
		{
			//MakeGrenadeTrailParticles(&(sbPtr->DynPtr->PrevPosition), &(sbPtr->DynPtr->Position));
			break;
		}
		default:
			break;
	}
}


/* removes a ghost */
void RemoveGhost(STRATEGYBLOCK *sbPtr)
{
	NETGHOSTDATABLOCK *ghostData;

	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);
	
	/* this is where we add fragmentation and explosion	effects to destroyed ghosts */
	switch(ghostData->type)
	{
		case(I_BehaviourAlienPlayer):
		case(I_BehaviourMarinePlayer):
		case(I_BehaviourPredatorPlayer):	
		{
			Extreme_Gibbing(sbPtr,ghostData->HModelController.section_data,ONE_FIXED);
			break;
		}
		case(I_BehaviourGrenade):
		{
			if (sbPtr->containingModule) {
				Ghost_Explosion_SoundData.position=sbPtr->DynPtr->Position;
				Sound_Play(SID_ED_GRENADE_PROXEXPLOSION,"n",&Ghost_Explosion_SoundData);
			}
			break;
    	}
		/*case(I_BehaviourRocket):
		{
			if (sbPtr->containingModule) {
				Ghost_Explosion_SoundData.position=sbPtr->DynPtr->Position;
				Sound_Play(SID_NICE_EXPLOSION,"n",&Ghost_Explosion_SoundData);
			}
			break;
    	}*/
		case(I_BehaviourFragmentationGrenade):
		{
			unsigned int i;

			for (i=0; i<2; i++)
			{
				VECTORCH position = sbPtr->DynPtr->Position;
				VECTORCH velocity;

				velocity.vy = (-(FastRandom()%1023)-512)*2;
				velocity.vx = ((FastRandom()&1023)-512)*2;
				velocity.vz = ((FastRandom()&1023)-512)*2;

				position.vx += (FastRandom()&2047)-1024;
				position.vy -= (FastRandom()&1024);
				position.vz += (FastRandom()&2047)-1024;

				MakeParticle(&position, &velocity, PARTICLE_STREAM);
			}
			break;
		}
		case(I_BehaviourProximityGrenade):
		{
			if (sbPtr->containingModule) {
				Ghost_Explosion_SoundData.position=sbPtr->DynPtr->Position;
				Sound_Play(SID_ED_GRENADE_EXPLOSION,"n",&Ghost_Explosion_SoundData);
			}
			break;
    	}
		case(I_BehaviourClusterGrenade):
		{
			if (sbPtr->containingModule) {
				Ghost_Explosion_SoundData.position=sbPtr->DynPtr->Position;
				Sound_Play(SID_ED_GRENADE_EXPLOSION,"n",&Ghost_Explosion_SoundData);
			}
			break;
    	}
		case(I_BehaviourPulseGrenade):
		{
			if (sbPtr->containingModule) {
				Ghost_Explosion_SoundData.position=sbPtr->DynPtr->Position;
				Sound_Play(SID_NADEEXPLODE,"n",&Ghost_Explosion_SoundData);
			}
			break;
    	}
		case(I_BehaviourNPCPredatorDisc):
		case(I_BehaviourPredatorDisc_SeekTrack):
		{
			/* MakeAnExplosion... ?	*/
		    //Sound_Play(SID_NADEEXPLODE,"d",&(sbPtr->DynPtr->Position));
			break;
		}
		case(I_BehaviourFrisbee):
		{
			//Ghost_Explosion_SoundData.position=sbPtr->DynPtr->Position;
			//Sound_Play(SID_NICE_EXPLOSION,"n",&Ghost_Explosion_SoundData);
			break;
    	}
		case (I_BehaviourPPPlasmaBolt) :
		{
			break;
		}


		case(I_BehaviourAlienSpit):
		{		
			break;
		}		
		case(I_BehaviourNetCorpse):
		{
			break;
		}
		default:
		{
			/* do absolutely bugger all, probably:
			remaining cases are: predator energy bolt;flame;predator disc;flare;
			and AUTOGUN */
			break;
		}
	}

	/* see if we've got a sound... */
	if(ghostData->SoundHandle  != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
	if(ghostData->SoundHandle2 != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle2);
	if(ghostData->SoundHandle3 != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle3);
	if(ghostData->SoundHandle4 != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle4);

	/* see if we've got a muzzle flash... */
	if(ghostData->myGunFlash)
	{
		RemoveNPCGunFlashEffect(ghostData->myGunFlash);
		ghostData->myGunFlash = NULL;
	}

	DestroyAnyStrategyBlock(sbPtr);
}

/* removes a given player's ghost, and all associated ghosts, eg when a player
leaves the game... */
void RemovePlayersGhosts(DPID id)
{
	extern int NumActiveStBlocks;
	extern STRATEGYBLOCK *ActiveStBlockList[];	

	int sbIndex = 0;
	STRATEGYBLOCK *sbPtr;

	while(sbIndex < NumActiveStBlocks)
	{	
		sbPtr = ActiveStBlockList[sbIndex++];
		if(sbPtr->I_SBtype==I_BehaviourNetGhost)
		{
			NETGHOSTDATABLOCK *ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
			LOCALASSERT(ghostData);			
			if(ghostData->playerId==id) {
				RemoveGhost(sbPtr);
			}
		}
	}	
}


/* locates a ghost from Id and ObId */
STRATEGYBLOCK *FindGhost(DPID Id, int obId)
{
	extern int NumActiveStBlocks;
	extern STRATEGYBLOCK *ActiveStBlockList[];	

	int sbIndex = 0;
	STRATEGYBLOCK *sbPtr;

	while(sbIndex < NumActiveStBlocks)
	{	
		sbPtr = ActiveStBlockList[sbIndex++];
		if(sbPtr->I_SBtype==I_BehaviourNetGhost)
		{
			NETGHOSTDATABLOCK *ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
			LOCALASSERT(ghostData);			

			if((ghostData->playerId==Id)&&(ghostData->playerObjectId==obId)) return sbPtr;	
		}
	}	
	return NULL;
}

/* create a new ghost: doesn't do shape animation stuff: this is performed using a
seperate set of functions */
STRATEGYBLOCK *CreateNetGhost(DPID playerId, int objectId, VECTORCH *position, EULER* orientation, AVP_BEHAVIOUR_TYPE type, unsigned char IOType, unsigned char subtype)
{
	int i;
	STRATEGYBLOCK *sbPtr;
	MODULE *myContainingModule;

	/* first check that the position we've been passed is in a module */
	myContainingModule = ModuleFromPosition(position, (MODULE *)0);
	if(myContainingModule==NULL) return NULL;

	/* create a strategy block */	
	sbPtr = CreateActiveStrategyBlock();
	if(!sbPtr) 
	{
		/* allocation failed */
		return NULL;
	}
	InitialiseSBValues(sbPtr);
	sbPtr->I_SBtype = I_BehaviourNetGhost;
	
	for(i = 0; i < SB_NAME_LENGTH; i++) sbPtr->SBname[i] = '\0';	
	AssignNewSBName(sbPtr);

	/* dynamics block */
	{
		DYNAMICSBLOCK *dynPtr;

		/* need different templates for objects and sprites */
		#if EXTRAPOLATION_TEST
		if(type==I_BehaviourMarinePlayer||type==I_BehaviourAlienPlayer||type==I_BehaviourPredatorPlayer || type==I_BehaviourAlien)
		{
			dynPtr = AllocateDynamicsBlock(DYNAMICS_TEMPLATE_MARINE_PLAYER);
			if(type==I_BehaviourAlienPlayer || type==I_BehaviourAlien)
			{
				dynPtr->ToppleForce=TOPPLE_FORCE_ALIEN;	
			}
		}
		else
		#endif
		{
			dynPtr = AllocateDynamicsBlock(DYNAMICS_TEMPLATE_NET_GHOST);
		}
		if(!dynPtr) 
		{
			/* allocation failed */
			RemoveBehaviourStrategy(sbPtr);
			return NULL;
		}

		sbPtr->DynPtr = dynPtr;
		/* zero linear velocity in dynamics block */
		dynPtr->LinVelocity.vx = dynPtr->LinVelocity.vy = dynPtr->LinVelocity.vz = 0;
		dynPtr->LinImpulse.vx = dynPtr->LinImpulse.vy = dynPtr->LinImpulse.vz = 0;
		dynPtr->GravityOn = 0;
		
		dynPtr->Position = dynPtr->PrevPosition = *position;
		dynPtr->OrientEuler = *orientation;
		CreateEulerMatrix(&dynPtr->OrientEuler, &dynPtr->OrientMat);
		TransposeMatrixCH(&dynPtr->OrientMat);
		
	}

	/* data block */
	{
		NETGHOSTDATABLOCK *ghostData = AllocateMem(sizeof(NETGHOSTDATABLOCK)); 
		if(!ghostData) 
		{
			/* allocation failed */
			RemoveBehaviourStrategy(sbPtr);
			return NULL;
		}		
		sbPtr->SBdataptr = (void *)ghostData;
		ghostData->playerId = playerId;
		ghostData->playerObjectId = objectId;
		ghostData->type = type;
		ghostData->IOType=(INANIMATEOBJECT_TYPE)IOType;
		ghostData->subtype=(int)subtype;
		ghostData->myGunFlash = NULL;
		ghostData->GunFlashFrameStamp=GlobalFrameCounter;
		ghostData->SoundHandle  = SOUND_NOACTIVEINDEX;
		ghostData->SoundHandle2 = SOUND_NOACTIVEINDEX;
		ghostData->SoundHandle3 = SOUND_NOACTIVEINDEX;
		ghostData->SoundHandle4 = SOUND_NOACTIVEINDEX;
		ghostData->currentAnimSequence = 0;
		ghostData->timer = 0;
		ghostData->CloakingEffectiveness = 0;
		// AMP Additions
		ghostData->ArmorType = 0;
		ghostData->Class = 0;
		ghostData->SpecialSequence = 0;
		ghostData->Grab = 0;

		ghostData->IgnitionHandshaking = 0;
		ghostData->soundStartFlag = 0;
		ghostData->FlameHitCount = 0;
		ghostData->FlechetteHitCount = 0;
		ghostData->invulnerable=0;
		ghostData->onlyValidFar=0;

		// SHOULDER LAMP IN MULTI
		ghostData->IAmUsingShoulderLamp = 0;
		ghostData->ShoulderLampOffset.vx = 0;
		ghostData->ShoulderLampOffset.vy = 0;
		ghostData->ShoulderLampOffset.vz = 0;
		// END OF SHOULDER LAMP IN MULTI

		#if EXTRAPOLATION_TEST
		ghostData->velocity.vx=0;
		ghostData->velocity.vy=0;
		ghostData->velocity.vz=0;
		ghostData->extrapTimerLast=0;
		ghostData->extrapTimer=0;
		#endif
		
		/* Clear HModelController. */
		ghostData->HModelController.Seconds_For_Sequence=0;
		ghostData->HModelController.timer_increment=0;
		ghostData->HModelController.Sequence_Type=0;
		ghostData->HModelController.Sub_Sequence=0;
		ghostData->HModelController.sequence_timer=0;
		ghostData->HModelController.FrameStamp=0;
		ghostData->HModelController.keyframe_flags=0;
		ghostData->HModelController.Deltas=NULL;
		ghostData->HModelController.Root_Section=NULL;
		ghostData->HModelController.section_data=NULL;
		ghostData->HModelController.After_Tweening_Sequence_Type=0;
		ghostData->HModelController.After_Tweening_Sub_Sequence=0;
		ghostData->HModelController.AT_seconds_for_sequence=0;
		ghostData->HModelController.Playing=0;
		ghostData->HModelController.Reversed=0;
		ghostData->HModelController.Looped=0;
		ghostData->HModelController.Tweening=0;
		ghostData->HModelController.LoopAfterTweening=0;
		ghostData->HModelController.ElevationTweening=0;
		/* Whew. */
		ghostData->hltable=0;
		/* init the integrity */
		ghostData->integrity = GHOST_INTEGRITY;		
 		
 		/* set the shape */
		switch(type)
		{
			case(I_BehaviourMarinePlayer):
			{
				CreateMarineHModel(ghostData,WEAPON_PULSERIFLE);
				ProveHModel_Far(&ghostData->HModelController,sbPtr);
				break;
			}
			case(I_BehaviourAlienPlayer):
			{
				CreateAlienHModel(ghostData,subtype);
				ProveHModel_Far(&ghostData->HModelController,sbPtr);
				break;
			}
 			case(I_BehaviourPredatorPlayer):
			{
				CreatePredatorHModel(ghostData,WEAPON_PRED_WRISTBLADE);
				ProveHModel_Far(&ghostData->HModelController,sbPtr);
				break;
			}
			
			/* KJL 16:57:14 17/06/98 - NPC characters */
			case(I_BehaviourAlien):
			{
				CreateAlienHModel(ghostData,subtype);
				
				ProveHModel_Far(&ghostData->HModelController,sbPtr);
				break;
			}
			
			
			case(I_BehaviourGrenade):
				sbPtr->shapeIndex = GetLoadedShapeMSL("incen");
				break;
			case(I_BehaviourPulseGrenade):
				sbPtr->shapeIndex = GetLoadedShapeMSL("pulsegrenade");
				break;
			case(I_BehaviourFragmentationGrenade):
				sbPtr->shapeIndex = GetLoadedShapeMSL("smoke");
				break;
			case(I_BehaviourClusterGrenade):
				sbPtr->shapeIndex = GetLoadedShapeMSL("handgrenade");
				break;
			case(I_BehaviourRocket):
				sbPtr->shapeIndex = GetLoadedShapeMSL("net");
				break;
			case(I_BehaviourPredatorEnergyBolt):
				sbPtr->shapeIndex = 0; // uses a special effect
				//need to play the energy bolt being fired sound
				//Sound_Play(SID_PRED_LAUNCHER,"hpd",(FastRandom()&255)-128,&sbPtr->DynPtr->Position);
				break;
			case(I_BehaviourFrisbeeEnergyBolt):
				sbPtr->shapeIndex = 0; // uses a special effect
				//need to play the energy bolt being fired sound
				//Sound_Play(SID_PRED_LAUNCHER,"hpd",(FastRandom()&255)-128,&sbPtr->DynPtr->Position);
				break;
			case(I_BehaviourPPPlasmaBolt):
				sbPtr->shapeIndex = 0; // uses a special effect
				break;
			case(I_BehaviourNPCPredatorDisc):
			case(I_BehaviourPredatorDisc_SeekTrack):
				{

				SECTION *root_section = GetNamedHierarchyFromLibrary("disk","Disk");
	
				GLOBALASSERT(root_section);

				Create_HModel(&ghostData->HModelController,root_section);
				InitHModelSequence(&ghostData->HModelController,HMSQT_MarineStand,MSSS_Minigun_Delta,ONE_FIXED);
				ProveHModel_Far(&ghostData->HModelController,sbPtr);

				/* Just to make sure. */
				sbPtr->shapeIndex = GetLoadedShapeMSL("Shell");
				break;
				}
			case(I_BehaviourFrisbee):
				{

				SECTION *root_section = GetNamedHierarchyFromLibrary("mdisk","Mdisk");
	
				GLOBALASSERT(root_section);

				Create_HModel(&ghostData->HModelController,root_section);
				InitHModelSequence(&ghostData->HModelController,HMSQT_MarineStand,MSSS_Minigun_Delta,(ONE_FIXED>>1));
				ProveHModel_Far(&ghostData->HModelController,sbPtr);

				/* Just to make sure. */
				sbPtr->shapeIndex = GetLoadedShapeMSL("Shell");

				Sound_Play(SID_ED_SKEETERLAUNCH,"hd",&(sbPtr->DynPtr->Position));

				break;
				}
			case(I_BehaviourInanimateObject):
			/* That *should* only happen for pred discs at this stage */
			//Now there are also weapons that appear when players die
				if(IOType==IOT_Weapon)
				{
					switch(subtype)
					{
	 					case WEAPON_MARINE_PISTOL:
	 					case WEAPON_PULSERIFLE:
							sbPtr->shapeIndex=GetLoadedShapeMSL("pulse");
							break;
						case WEAPON_SMARTGUN:
							sbPtr->shapeIndex=GetLoadedShapeMSL("smart");
							break;
						case WEAPON_FLAMETHROWER:
							sbPtr->shapeIndex=GetLoadedShapeMSL("flame");
							break;
						case WEAPON_SADAR:
							sbPtr->shapeIndex=GetLoadedShapeMSL("sadar");
							break;
						case WEAPON_GRENADELAUNCHER:
							sbPtr->shapeIndex=GetLoadedShapeMSL("grenade");
							break;
						case WEAPON_MINIGUN:
							sbPtr->shapeIndex=GetLoadedShapeMSL("minigun");
							break;
						case WEAPON_FRISBEE_LAUNCHER:
							sbPtr->shapeIndex=GetLoadedShapeMSL("sadar");
							break;
						default :
							GLOBALASSERT(0=="Unexpected weapon type");
							
						
					}
					sbPtr->DynPtr->IsPickupObject=1;
					GLOBALASSERT(sbPtr->shapeIndex!=-1);

				}
				else
				{
					SECTION *disc_section;
					SECTION *root_section = GetNamedHierarchyFromLibrary("disk","Disk");
	
					GLOBALASSERT(IOType==IOT_Ammo);
					GLOBALASSERT(subtype==AMMO_PRED_DISC);
					GLOBALASSERT(root_section);

					#if 0
					Create_HModel(&ghostData->HModelController,root_section);
					InitHModelSequence(&ghostData->HModelController,HMSQT_MarineStand,MSSS_Minigun_Delta,ONE_FIXED);
					ProveHModel_Far(&ghostData->HModelController,sbPtr);

					/* Just to make sure. */
					sbPtr->shapeIndex = GetLoadedShapeMSL("Shell");
					#else
					/* Now it's not hierarchical! */
					disc_section=GetThisSection(root_section,"disk");
					GLOBALASSERT(disc_section);

					sbPtr->shapeIndex = disc_section->ShapeNum;
					#endif
				}
				break;
				
			case(I_BehaviourFlareGrenade):
				sbPtr->shapeIndex = GetLoadedShapeMSL("newflare");
				ghostData->timer = FLARE_LIFETIME*ONE_FIXED;
				break;		
			case(I_BehaviourProximityGrenade):
				sbPtr->shapeIndex = GetLoadedShapeMSL("setcharge");
				ghostData->timer = PROX_GRENADE_LIFETIME*ONE_FIXED*2;
				break;
			case(I_BehaviourAlienSpit):
				sbPtr->shapeIndex = GetLoadedShapeMSL("Spit");
				break;					
			case(I_BehaviourAutoGun):
				sbPtr->shapeIndex = GetLoadedShapeMSL("Sentry01");
				break;					
			case(I_BehaviourSpeargunBolt):
				sbPtr->shapeIndex = GetLoadedShapeMSL("prong");
				//speargun bolt won't get any location updates , so set integrity 
				//to be the standard speargun timeout time.
				ghostData->integrity = 20*ONE_FIXED;		
		
			    //play the sound for the bolt as well
			    Sound_Play(SID_SPEARGUN_HITTING_WALL,"d",&sbPtr->DynPtr->Position);  
				//and make the sparks
				{
					VECTORCH pos = sbPtr->DynPtr->Position;
					pos.vx += sbPtr->DynPtr->OrientMat.mat31;
					pos.vy += sbPtr->DynPtr->OrientMat.mat32;
					pos.vz += sbPtr->DynPtr->OrientMat.mat33;
					MakeFocusedExplosion(&(sbPtr->DynPtr->Position), &pos, 20, PARTICLE_SPARK);
				}
				
				break;
			case(I_BehaviourThrownSpear):
				sbPtr->shapeIndex = GetLoadedShapeMSL("combistick_thrown");
				break;
			default:
				break;
		}
		LOCALASSERT(sbPtr->shapeIndex!=-1);	
		
	}

	/* strategy block initialisation, after dynamics block creation */
	sbPtr->maintainVisibility = 1;		  
 	sbPtr->containingModule = myContainingModule;

 	return sbPtr;
}

int UseExtrapolation=1;
#if EXTRAPOLATION_TEST
void PlayerGhostExtrapolation()
{
	extern int NumActiveStBlocks;
	extern STRATEGYBLOCK *ActiveStBlockList[];	

	int sbIndex = 0;
	STRATEGYBLOCK *sbPtr;

	
	//search for all ghosts of players
	while(sbIndex < NumActiveStBlocks)
	{	
		sbPtr = ActiveStBlockList[sbIndex++];
		if(sbPtr->I_SBtype==I_BehaviourNetGhost)
		{
			NETGHOSTDATABLOCK *ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
			if(ghostData->type==I_BehaviourMarinePlayer ||
			   ghostData->type==I_BehaviourAlienPlayer ||
			   ghostData->type==I_BehaviourAlien ||
			   ghostData->type==I_BehaviourPredatorPlayer)
			{
				int time;
				DYNAMICSBLOCK* dynPtr= sbPtr->DynPtr;
				if(ghostData->onlyValidFar) continue;

				if(UseExtrapolation)
				{

					dynPtr->LinVelocity.vx=0;
					dynPtr->LinVelocity.vy=0;
					dynPtr->LinVelocity.vz=0;

					dynPtr->IsNetGhost=0;

					ghostData->extrapTimer+=NormalFrameTime;
					if(ghostData->extrapTimer<0) ghostData->extrapTimer=0;
					if(ghostData->extrapTimer>ONE_FIXED/2) ghostData->extrapTimer=ONE_FIXED/2;

					time=ghostData->extrapTimer-ghostData->extrapTimerLast;
					ghostData->extrapTimerLast=ghostData->extrapTimer;

					if(ghostData->velocity.vx==0 && ghostData->velocity.vy==0 && ghostData->velocity.vz==0)
					{
						/*
						Not moving , so alter the dynamics block settings to match those of a non-extrapolated
						net ghost.
						*/
						dynPtr->LinImpulse.vx=0;
						dynPtr->LinImpulse.vy=0;
						dynPtr->LinImpulse.vz=0;

						ghostData->extrapTimerLast=ghostData->extrapTimer=0;
						dynPtr->IsNetGhost=1;
						dynPtr->UseStandardGravity=1;
						sbPtr->DynPtr->ToppleForce=TOPPLE_FORCE_NONE;
					}
					else if(time>0)
					{
						dynPtr->LinVelocity=ghostData->velocity;
						if(time!=NormalFrameTime)
						{
							//only doing interpolation for a fraction of a frame
							//so we need to scale the velocity accordingly
							dynPtr->LinVelocity.vx=WideMulNarrowDiv(dynPtr->LinVelocity.vx,time,NormalFrameTime);
							dynPtr->LinVelocity.vy=WideMulNarrowDiv(dynPtr->LinVelocity.vy,time,NormalFrameTime);
							dynPtr->LinVelocity.vz=WideMulNarrowDiv(dynPtr->LinVelocity.vz,time,NormalFrameTime);
						}
					}


					/*
					if(sbPtr->SBdptr)
					{
						sbPtr->SBdptr->ObRadius=1200;
					}
					*/

				}
				else
				{
					//not using extrapolation , so make sure dynamics block
					//contains normal ghost settings
					dynPtr->LinImpulse.vx=0;
					dynPtr->LinImpulse.vy=0;
					dynPtr->LinImpulse.vz=0;

					dynPtr->LinVelocity.vx=0;
					dynPtr->LinVelocity.vy=0;
					dynPtr->LinVelocity.vz=0;

					dynPtr->UseStandardGravity=1;
					
					dynPtr->IsNetGhost=1;
					
				   	sbPtr->DynPtr->ToppleForce=TOPPLE_FORCE_NONE;
				}
			}
		}
	}	
}

void PostDynamicsExtrapolationUpdate()
{
	extern DPID MultiplayerObservedPlayer;
	extern int NumActiveStBlocks;
	extern STRATEGYBLOCK *ActiveStBlockList[];	

	int sbIndex = 0;
	STRATEGYBLOCK *sbPtr;

	if(!UseExtrapolation) return;
	
	//search for all ghosts of players
	while(sbIndex < NumActiveStBlocks)
	{	
		sbPtr = ActiveStBlockList[sbIndex++];
		if(sbPtr->I_SBtype==I_BehaviourNetGhost)
		{
			NETGHOSTDATABLOCK *ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
			if(ghostData->type==I_BehaviourMarinePlayer ||
			   ghostData->type==I_BehaviourAlienPlayer ||
			   ghostData->type==I_BehaviourPredatorPlayer)
			{
				if(ghostData->myGunFlash)
				{
					HandleGhostGunFlashEffect(sbPtr, 3);
				}
				//are we currently following this player's movements
				if(MultiplayerObservedPlayer)
				{
					if(MultiplayerObservedPlayer==ghostData->playerId)
					{
						PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
						Player->ObStrategyBlock->DynPtr->Position=sbPtr->DynPtr->Position;
						Player->ObStrategyBlock->DynPtr->PrevPosition=sbPtr->DynPtr->Position;
						
						Player->ObStrategyBlock->DynPtr->OrientEuler = sbPtr->DynPtr->OrientEuler;
						CreateEulerMatrix(&Player->ObStrategyBlock->DynPtr->OrientEuler,&Player->ObStrategyBlock->DynPtr->OrientMat);
						TransposeMatrixCH(&Player->ObStrategyBlock->DynPtr->OrientMat);
												
					}
				}
			}
		}
	}	
}
#endif //EXTRAPOLATION_TEST

int DetermineMarineVoice(unsigned int Class)
{
	switch(Class)
	{
	case CLASS_RIFLEMAN:
	case CLASS_AA_SPEC:
		return 0;
		break;
	case CLASS_SMARTGUNNER:
		return 0;
		break;
	case CLASS_INC_SPEC:
	case CLASS_EXF_SNIPER:
		return 0;
		break;
	case CLASS_COM_TECH:
	case CLASS_ENGINEER:
		return 0;
		break;
	case CLASS_MEDIC_FT:
		return 0;
		break;
	}
	return 0;
}

int DeterminePredatorVoice(unsigned int Class)
{
	switch(Class)
	{
	case CLASS_PRED_HUNTER:
		return 0;
		break;
	case CLASS_PRED_WARRIOR:
		return -100;
		break;
	case CLASS_RENEGADE:
		return 0;
		break;
	case CLASS_ELDER:
		return -200;
		break;
	case CLASS_TANK_ALIEN:
		return 200;
		break;
	}
	return 0;
}

void SetAlternateAlienSkin(HMODELCONTROLLER *HModelController,unsigned int Class)
{
	SECTION_DATA *head;
	TXACTRLBLK *tacb;
	unsigned int i=0;

	head=GetThisSectionData(HModelController->section_data,"head");

	if (Class == CLASS_ALIEN_DRONE) i=1;	// Drone skin

	if (head) {
		if ((head->flags&section_data_notreal)==0) {
			tacb = head->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, head->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
}

void SetAlternateCharacterSkin(HMODELCONTROLLER *HModelController,unsigned int Class)
{
	SECTION_DATA *head,*pelvis,*chest;
	SECTION_DATA *l_shoulder,*l_bicep,*l_elbow,*l_forearm,*l_thi,*l_shin;
	SECTION_DATA *r_shoulder,*r_bicep,*r_elbow,*r_forearm,*r_thi,*r_shin;
	TXACTRLBLK *tacb;
	unsigned int i=0;

	head=GetThisSectionData(HModelController->section_data,"head");
	pelvis=GetThisSectionData(HModelController->section_data,"Pelvis");
	chest=GetThisSectionData(HModelController->section_data,"chest");
	l_shoulder=GetThisSectionData(HModelController->section_data,"L shoulder");
	r_shoulder=GetThisSectionData(HModelController->section_data,"R shoulder");
	l_bicep=GetThisSectionData(HModelController->section_data,"L bicep");
	r_bicep=GetThisSectionData(HModelController->section_data,"R bicep");
	l_elbow=GetThisSectionData(HModelController->section_data,"L elbow");
	r_elbow=GetThisSectionData(HModelController->section_data,"R elbow");
	l_forearm=GetThisSectionData(HModelController->section_data,"L forearm");
	r_forearm=GetThisSectionData(HModelController->section_data,"R forearm");
	l_thi=GetThisSectionData(HModelController->section_data,"L thi");
	r_thi=GetThisSectionData(HModelController->section_data,"R thi");
	l_shin=GetThisSectionData(HModelController->section_data,"L shin");
	r_shin=GetThisSectionData(HModelController->section_data,"R shin");

	if (Class == CLASS_TANK_ALIEN) i=1;			// Female Hunter Skin
	else if (Class == CLASS_RENEGADE) i=2;		// Bad Blood Skin
	else if (Class == CLASS_PRED_HUNTER) i=3;	// Hunter Skin
	else if (Class == CLASS_PRED_WARRIOR) i=4;	// Warrior Skin

	if (head) {
		if ((head->flags&section_data_notreal)==0) {
			tacb = head->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, head->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	/* Only change pelvis and chest textures if this class is another than Hunter */
	if (pelvis && (i != 3)) {
		if ((pelvis->flags&section_data_notreal)==0) {
			tacb = pelvis->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, pelvis->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (chest && (i != 3)) {
		if ((chest->flags&section_data_notreal)==0) {
			tacb = chest->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, chest->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (l_shoulder) {
		if ((l_shoulder->flags&section_data_notreal)==0) {
			tacb = l_shoulder->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, l_shoulder->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (r_shoulder) {
		if ((r_shoulder->flags&section_data_notreal)==0) {
			tacb = r_shoulder->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, r_shoulder->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (l_bicep) {
		if ((l_bicep->flags&section_data_notreal)==0) {
			tacb = l_bicep->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, l_bicep->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (r_bicep) {
		if ((r_bicep->flags&section_data_notreal)==0) {
			tacb = r_bicep->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, r_bicep->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (l_elbow) {
		if ((l_elbow->flags&section_data_notreal)==0) {
			tacb = l_elbow->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, l_elbow->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (r_elbow) {
		if ((r_elbow->flags&section_data_notreal)==0) {
			tacb = r_elbow->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, r_elbow->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (l_forearm) {
		if ((l_forearm->flags&section_data_notreal)==0) {
			tacb = l_forearm->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, l_forearm->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (r_forearm) {
		if ((r_forearm->flags&section_data_notreal)==0) {
			tacb = r_forearm->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, r_forearm->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (l_thi) {
		if ((l_thi->flags&section_data_notreal)==0) {
			tacb = l_thi->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, l_thi->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (r_thi) {
		if ((r_thi->flags&section_data_notreal)==0) {
			tacb = r_thi->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, r_thi->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (l_shin) {
		if ((l_shin->flags&section_data_notreal)==0) {
			tacb = l_shin->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, l_shin->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
	if (r_shin) {
		if ((r_shin->flags&section_data_notreal)==0) {
			tacb = r_shin->tac_ptr;

			while (tacb) {
				tacb->tac_sequence = i;
				tacb->tac_txah_s = GetTxAnimHeaderFromShape(tacb, r_shin->ShapeNum);
				tacb = tacb->tac_next;
			}
		}
	}
}

extern HIERARCHY_VARIANT_DATA* GetHierarchyAlternateShapeSetCollectionFromLibrary(const char* rif_name,int index);
void ChangeGhostMarineAccoutrementSet(HMODELCONTROLLER *HModelController,DPID playerId,unsigned int Class)
{

	HIERARCHY_VARIANT_DATA* variant_data;
	HIERARCHY_SHAPE_REPLACEMENT* replacement_array;
	int index=0;
	int a;

	LOCALASSERT(HModelController);

	//find the index for this player
	if(playerId)
	{
		for(index=0;index<(NET_MAXPLAYERS);index++)
		{
			if(netGameData.playerData[index].playerId==playerId)
			{
				break;
			}
		}
	}

	// Extended Multiplayer -- Eldritch
	{
		switch(Class) {
		case CLASS_RIFLEMAN:
			// Soldier+Helmet, Face1
			variant_data=GetHierarchyAlternateShapeSetCollectionFromLibrary("hnpcmarine",1);
			break;
		case CLASS_SMARTGUNNER:
			// Smartgunner, Face1
			variant_data=GetHierarchyAlternateShapeSetCollectionFromLibrary("hnpcmarine",9);
			break;
		case CLASS_MEDIC_FT:
		case CLASS_COM_TECH:
		case CLASS_AA_SPEC:
		default:
			// Soldier+Helmet, Face2
			variant_data=GetHierarchyAlternateShapeSetCollectionFromLibrary("hnpcmarine",3);
			break;
		case CLASS_ENGINEER:
		case CLASS_INC_SPEC:
			// Soldier+Helmet, Face3
			variant_data=GetHierarchyAlternateShapeSetCollectionFromLibrary("hnpcmarine",4);
			break;
		}
	}
	// -- End

	//variant_data=GetHierarchyAlternateShapeSetCollectionFromLibrary("hnpcmarine",index+1);

	if (variant_data==NULL) {
		return;
	}
	

	replacement_array=(HIERARCHY_SHAPE_REPLACEMENT*)variant_data->replacements;

	if (replacement_array==NULL) {
		return;
	}
	
	
	a=0;

	while (replacement_array[a].replaced_section_name!=NULL) 
	{
		SECTION_DATA *target_section;

		target_section=GetThisSectionData(HModelController->section_data,
			replacement_array[a].replaced_section_name);
		if (target_section) {
			target_section->Shape=replacement_array[a].replacement_shape;
			#if 1
			target_section->ShapeNum=replacement_array[a].replacement_shape_index;
			#endif
			
			Setup_Texture_Animation_For_Section(target_section);
			
		}
		a++;
	}

}

void CreateMarineHModel(NETGHOSTDATABLOCK *ghostDataPtr, int weapon)
{
	SECTION *root_section;
	
	/* KJL 11:09:14 27/01/98 - pick which model to create based on the character's weapon */
	switch (weapon)
	{
		default:
		case WEAPON_PULSERIFLE:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcmarine","marine with pulse rifle");
			break;
		}
		case WEAPON_TWO_PISTOLS:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcmarine","Two Pistol");
			break;
		}
		case WEAPON_MARINE_PISTOL:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcmarine","PISTOL");
			break;
		}
		case WEAPON_FLAMETHROWER:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcmarine","marine with flame thrower");
			break;
		}
		case WEAPON_SMARTGUN:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcmarine","marine with smart gun");
			break;
		}
		case WEAPON_MINIGUN:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcmarine","Marine with Mini Gun");
			break;
		}
		case WEAPON_SADAR:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcmarine","marine with SADAR");
			break;
		}
		case WEAPON_GRENADELAUNCHER:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcmarine","marine + grenade launcher");
			break;
		}
		case WEAPON_CUDGEL:
		case WEAPON_PLASMAGUN:
		case WEAPON_AUTOSHOTGUN:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcmarine","Cudgel");
			break;
		}
		case WEAPON_FRISBEE_LAUNCHER:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcmarine","skeeter");
			break;
		}
	}
	Create_HModel(&ghostDataPtr->HModelController,root_section);
	ghostDataPtr->CurrentWeapon = weapon;

	if (weapon == WEAPON_AUTOSHOTGUN || weapon == WEAPON_PLASMAGUN || weapon == WEAPON_CUDGEL)
	{
		SECTION_DATA *pistol;

		pistol=GetThisSectionData(ghostDataPtr->HModelController.section_data,"pulse rifle");
		pistol->flags|=(section_data_notreal);

		pistol=GetThisSectionData(ghostDataPtr->HModelController.section_data,"pulse mag");
		pistol->flags|=(section_data_notreal);

		pistol=GetThisSectionData(ghostDataPtr->HModelController.section_data,"pump");
		pistol->flags|=(section_data_notreal);
	}

	ChangeGhostMarineAccoutrementSet(&ghostDataPtr->HModelController,ghostDataPtr->playerId,ghostDataPtr->Class);

	/* Static Model Exchange System */
	if (ghostDataPtr->Class == CLASS_RIFLEMAN)
		ModExSys(&ghostDataPtr->HModelController,"chest","riflemanchest");
	else if (ghostDataPtr->Class == CLASS_ENGINEER)
		ModExSys(&ghostDataPtr->HModelController,"chest","engineerchest");
	else if (ghostDataPtr->Class == CLASS_COM_TECH)
		ModExSys(&ghostDataPtr->HModelController,"chest","comtechchest");
	else if (ghostDataPtr->Class == CLASS_MEDIC_FT)
		ModExSys(&ghostDataPtr->HModelController,"chest","medicchest");

	if ((ghostDataPtr->Class) && (ghostDataPtr->IAmUsingShoulderLamp))
		ModExSys(&ghostDataPtr->HModelController,"light ","sl");

	/* KJL 11:09:38 27/01/98 - set a default anim sequence to use */
	ghostDataPtr->currentAnimSequence = MSQ_Stand;
	InitHModelSequence(&ghostDataPtr->HModelController,(int)HMSQT_MarineStand,(int)MSSS_Standard,ONE_FIXED);
	
	/* KJL 11:09:52 27/01/98 - find the section which hold the muzzle flash data */
	ghostDataPtr->GunflashSectionPtr=GetThisSectionData(ghostDataPtr->HModelController.section_data,"dum flash");
	GLOBALASSERT(ghostDataPtr->GunflashSectionPtr);

	/* CDF 8/4/98 Elevation Delta Sequence. */
	{
		DELTA_CONTROLLER *delta;
		delta=Add_Delta_Sequence(&ghostDataPtr->HModelController,"Elevation",(int)HMSQT_MarineStand,(int)MSSS_Elevation,0);
		GLOBALASSERT(delta);
		delta->timer=32767;
	}
	//add hit delta
	if (HModelSequence_Exists(&ghostDataPtr->HModelController,(int)HMSQT_MarineStand,(int)MSSS_HitChestFront)) 
	{
		DELTA_CONTROLLER *delta;
		delta=Add_Delta_Sequence(&ghostDataPtr->HModelController,"HitDelta",(int)HMSQT_MarineStand,(int)MSSS_HitChestFront,(ONE_FIXED>>2));
		GLOBALASSERT(delta);
		delta->Playing=0;
	}

	ghostDataPtr->hltable=(HITLOCATIONTABLE*)GetThisHitLocationTable("marine with pulse rifle");

}
void CreateAlienHModel(NETGHOSTDATABLOCK *ghostDataPtr,int alienType)
{
	SECTION *root_section;

	// Extended Multiplayer -- Eldritch
	#if 1
	switch(alienType)
	{
		case AT_Predalien :
			root_section=GetNamedHierarchyFromLibrary("hnpcpred_alien","TEMPLATE");
			ghostDataPtr->hltable=GetThisHitLocationTable("predalien");
			break;
		case AT_Praetorian :
			root_section=GetHierarchyFromLibrary("hnpchugger");
			ghostDataPtr->hltable=GetThisHitLocationTable("alien");
			break;
		case 3:
			root_section=GetNamedHierarchyFromLibrary("queen","Template");
			ghostDataPtr->hltable=GetThisHitLocationTable("alien");
			break;
		default :
			root_section = GetNamedHierarchyFromLibrary("hnpcalien","alien");
			ghostDataPtr->hltable=GetThisHitLocationTable("alien");
			break;
	}
	#else
	//root_section=GetNamedHierarchyFromLibrary("hnpcpred_alien","TEMPLATE");
	//ghostDataPtr->hltable=GetThisHitLocationTable("predalien");
	//root_section=GetNamedHierarchyFromLibrary("queen","Template");
	//ghostDataPtr->hltable=GetThisHitLocationTable("alien");
	//root_section=GetHierarchyFromLibrary("hnpchugger");
	//ghostDataPtr->hltable=GetThisHitLocationTable("alien");
	#endif
	// -- End

	Create_HModel(&ghostDataPtr->HModelController,root_section);

	if (ghostDataPtr->Class == CLASS_CHESTBURSTER)
	{
		SECTION_DATA *part;

		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l00 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l01 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l02 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l03 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l04 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l05 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l06 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l07 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l08 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l09 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l10 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"l11 tail");
		part->flags|=(section_data_notreal);

		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left a1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left a2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left a3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left b1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left b2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left b3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left c1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left c2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left c3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left d1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left d2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left d3 finger");
		part->flags|=(section_data_notreal);

		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right a1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right a2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right a3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right b1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right b2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right b3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right c1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right c2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right c3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right d1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right d2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right d3 finger");
		part->flags|=(section_data_notreal);

		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"left air sack");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(ghostDataPtr->HModelController.section_data,"right air sack");
		part->flags|=(section_data_notreal);

		ModExSys(&ghostDataPtr->HModelController,"body","burster");
	}

	if (ghostDataPtr->Class != CLASS_EXF_SNIPER &&
		ghostDataPtr->Class != CLASS_EXF_W_SPEC &&
		ghostDataPtr->Class != CLASS_MEDIC_PR &&
		ghostDataPtr->Class != CLASS_CHESTBURSTER)
		SetAlternateAlienSkin(&ghostDataPtr->HModelController,ghostDataPtr->Class);

	if ((ghostDataPtr->Class == CLASS_EXF_W_SPEC) ||
		(ghostDataPtr->Class == CLASS_CHESTBURSTER))
	{
		ghostDataPtr->currentAnimSequence = FhSQ_Stand;
		InitHModelSequence(&ghostDataPtr->HModelController,(int)HMSQT_Hugger,(int)HSS_Stand,ONE_FIXED);
	}
	else if (ghostDataPtr->Class == CLASS_MEDIC_PR)
	{
		ghostDataPtr->currentAnimSequence = ASQ_Stand;
		InitHModelSequence(&ghostDataPtr->HModelController,(int)HMSQT_QueenRightStanceTemplate,(int)QRSTSS_Standard,ONE_FIXED);
	}
	else 
	{
		ghostDataPtr->currentAnimSequence = ASQ_Stand;
		InitHModelSequence(&ghostDataPtr->HModelController,(int)HMSQT_AlienStand,(int)ASSS_Standard,ONE_FIXED);
	}
	ghostDataPtr->CurrentWeapon = 0;
	
	/* CDF 12/4/98 Elevation Delta Sequence. */
	{
		DELTA_CONTROLLER *delta;
		delta=Add_Delta_Sequence(&ghostDataPtr->HModelController,"Elevation",(int)HMSQT_AlienStand,(int)ASSS_Standard_Elevation,0);
		GLOBALASSERT(delta);
		delta->timer=32767;
	
		delta->Active=0; /* Default for predator. */
	}
}
void CreatePredatorHModel(NETGHOSTDATABLOCK *ghostDataPtr, int weapon)
{
	SECTION *root_section;
	/* KJL 11:09:14 27/01/98 - pick which model to create based on the character's weapon */
	switch (weapon)
	{
		default:
		case WEAPON_PRED_WRISTBLADE:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred with wristblade");
			break;
		}
		case WEAPON_PRED_STAFF:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred with staff");
			break;
		}
		case WEAPON_SONICCANNON:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcpredator","Speargun");
			break;
		}
		case WEAPON_PRED_RIFLE:
		{
			// Extended Multiplayer -- Eldritch
			root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred with staff");
			// -- End
			break;
		}
		case WEAPON_PRED_PISTOL:
		case WEAPON_BEAMCANNON:
		case WEAPON_MYSTERYGUN:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred + pistol");
			break;
		}
		case WEAPON_PRED_SHOULDERCANNON:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred with Plasma Caster");
			break;
		}
		case WEAPON_PRED_DISC:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred with disk");
			break;
		}
		case WEAPON_PRED_MEDICOMP:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcpredator","medicomp");
			break;
		}
		case 666:
		{
			root_section = GetNamedHierarchyFromLibrary("hnpcpredator","Template");
			break;
		}

		/*
	WEAPON_PRED_RIFLE,
	,
		  */
	}

	Create_HModel(&ghostDataPtr->HModelController,root_section);
	ghostDataPtr->CurrentWeapon = weapon;

	/* Remove pistol model from HModel when using Wrist Launcher/Dart Launcher */
	if (weapon == WEAPON_BEAMCANNON || weapon == WEAPON_MYSTERYGUN)
	{
		SECTION_DATA *pistol;

		pistol=GetThisSectionData(ghostDataPtr->HModelController.section_data,"pistol");
		pistol->flags|=(section_data_notreal);

		pistol=GetThisSectionData(ghostDataPtr->HModelController.section_data,"Pistol A");
		pistol->flags|=(section_data_notreal);

		pistol=GetThisSectionData(ghostDataPtr->HModelController.section_data,"Pistol B");
		pistol->flags|=(section_data_notreal);

		pistol=GetThisSectionData(ghostDataPtr->HModelController.section_data,"Pistol C");
		pistol->flags|=(section_data_notreal);

		pistol=GetThisSectionData(ghostDataPtr->HModelController.section_data,"Pistol MECH");
		pistol->flags|=(section_data_notreal);
	}

	// Change Skin
	SetAlternateCharacterSkin(&ghostDataPtr->HModelController,ghostDataPtr->Class);

	/* Static Model Exchange System */
	if (ghostDataPtr->Class == CLASS_PRED_HUNTER)
	{
		ModExSys(&ghostDataPtr->HModelController,"chest","hunterchest");
		ModExSys(&ghostDataPtr->HModelController,"Pelvis","hunterpelvis");
	}

	GLOBALASSERT(ghostDataPtr->HModelController.Root_Section==root_section);
	
	InitHModelSequence(&ghostDataPtr->HModelController,(int)HMSQT_PredatorStand,(int)PSSS_Standard,ONE_FIXED);
	ghostDataPtr->currentAnimSequence = PredSQ_Stand;
	
//	ghostDataPtr->GunflashSectionPtr=GetThisSectionData(ghostDataPtr->HModelController.section_data,"dum flash");
//	GLOBALASSERT(ghostDataPtr->GunflashSectionPtr);

	/* CDF 11/4/98 Elevation Delta Sequence. */
	{
		DELTA_CONTROLLER *delta;
		delta=Add_Delta_Sequence(&ghostDataPtr->HModelController,"Elevation",(int)HMSQT_PredatorStand,(int)PSSS_Elevation,0);
		GLOBALASSERT(delta);
		delta->timer=32767;
	
		delta->Active=0; /* Default for predator. */
	}

    if (HModelSequence_Exists(&ghostDataPtr->HModelController,(int)HMSQT_PredatorStand,(int)PSSS_HitChestFront)) 
    {
    	DELTA_CONTROLLER *delta;
    	delta=Add_Delta_Sequence(&ghostDataPtr->HModelController,"HitDelta",(int)HMSQT_PredatorStand,(int)PSSS_HitChestFront,-1);
    	GLOBALASSERT(delta);
    	delta->Playing=0;
    }
	ghostDataPtr->hltable=GetThisHitLocationTable("predator");
}


/* these functions are called directly by the visibility management system */
void MakeGhostNear(STRATEGYBLOCK *sbPtr)
{
	extern MODULEMAPBLOCK AlienDefaultMap;
	NETGHOSTDATABLOCK *ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	MODULE tempModule;
	DISPLAYBLOCK *dPtr;
	DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;
	SFXBLOCK *sfxPtr=0;

	LOCALASSERT(ghostData);
	
	/* KJL 11:56:09 08/05/98 - if shape doesn't exist, use a sfx */
	if ( (ghostData->type==I_BehaviourPredatorEnergyBolt)
	   ||(ghostData->type==I_BehaviourFrisbeeEnergyBolt)
	   ||(ghostData->type==I_BehaviourPPPlasmaBolt) )
	{
		sfxPtr = AllocateSfxBlock();
		// if we haven't got a free sfx block, return */
		if (!sfxPtr) return;
	}
	
	if(ghostData->onlyValidFar)
	{
		//Far alien ai , don't have enough information to make it near
		return;
	}

    LOCALASSERT(dynPtr);
	LOCALASSERT(sbPtr->SBdptr == NULL);

	AlienDefaultMap.MapShape = sbPtr->shapeIndex;
	tempModule.m_mapptr = &AlienDefaultMap;
	tempModule.m_sbptr = (STRATEGYBLOCK*)NULL;
	tempModule.m_numlights = 0;
	tempModule.m_lightarray = (struct lightblock *)0;
	tempModule.m_extraitemdata = (struct extraitemdata *)0;
	tempModule.m_dptr = NULL; /* this is important */	
	AllocateModuleObject(&tempModule); 
	dPtr = tempModule.m_dptr;
	if(dPtr==NULL) return; /* cannot create displayblock, so leave object "far" */

			
	sbPtr->SBdptr = dPtr;
	dPtr->ObStrategyBlock = sbPtr;
	dPtr->ObMyModule = NULL;					
	dPtr->HModelControlBlock=NULL;

	 /* set the animation sequence, if we're a player ghost */
	if(ghostData->playerObjectId==GHOST_PLAYEROBJECTID)
	{
		LOCALASSERT((ghostData->type == I_BehaviourMarinePlayer)||
					(ghostData->type == I_BehaviourPredatorPlayer)||
					(ghostData->type == I_BehaviourAlienPlayer));

		dPtr->HModelControlBlock=&ghostData->HModelController;
		ProveHModel(dPtr->HModelControlBlock,dPtr);
    }
	else if (
		(ghostData->type == I_BehaviourAlien)||
		(ghostData->type == I_BehaviourPredatorDisc_SeekTrack)||
		(ghostData->type == I_BehaviourFrisbee)||
		(ghostData->type == I_BehaviourNetCorpse)
		) {
		dPtr->HModelControlBlock=&ghostData->HModelController;
		ProveHModel(dPtr->HModelControlBlock,dPtr);
	}
	
	else dPtr->ShapeAnimControlBlock = NULL; 
                            
	/* need to initialise positional information in the new display block */ 
	dPtr->ObWorld = dynPtr->Position;
	dPtr->ObEuler = dynPtr->OrientEuler;
	dPtr->ObMat = dynPtr->OrientMat;	
	dPtr->SfxPtr = sfxPtr;

	/* finally, need to add lighting effects to the displayblock */
	switch(ghostData->type)
	{
		case(I_BehaviourPulseGrenade):
		{
			AddLightingEffectToObject(dPtr,LFX_ROCKETJET);
			break;		
		}
		case(I_BehaviourPredatorEnergyBolt):
		{
			sfxPtr->SfxID = SFX_PREDATOR_PLASMA_BOLT;
			AddLightingEffectToObject(dPtr,LFX_PLASMA_BOLT);
			break;
		}
		case(I_BehaviourFrisbeeEnergyBolt):
		{
			sfxPtr->SfxID = SFX_FRISBEE_PLASMA_BOLT;
			AddLightingEffectToObject(dPtr,LFX_PLASMA_BOLT);
			break;
		}
		case(I_BehaviourPPPlasmaBolt):
		{
			sfxPtr->SfxID = SFX_SMALL_PREDATOR_PLASMA_BOLT;
			AddLightingEffectToObject(dPtr,LFX_PLASMA_BOLT);
			break;
		}

		case(I_BehaviourFlareGrenade):
		{
			AddLightingEffectToObject(dPtr,LFX_FLARE);

			break;		
		}
		default:
			break;
	}
}

void MakeGhostFar(STRATEGYBLOCK *sbPtr)
{
	int i;
	LOCALASSERT(sbPtr->SBdptr != NULL);

	/* get rid of the displayblock */
	i = DestroyActiveObject(sbPtr->SBdptr);
	LOCALASSERT(i==0);
	sbPtr->SBdptr = NULL;	
}
/* this function handles damage to a netghost object: 
basically, it just sends a network message, which should be picked up by the owning object */
void DamageNetworkGhost(STRATEGYBLOCK *sbPtr, DAMAGE_PROFILE *damage, int multiple, SECTION_DATA *section,VECTORCH* incoming)
{
	int sectionID;
	int delta_seq=-1;
	int delta_sub_seq=-1;

	LOCALASSERT(AvP.Network!=I_No_Network);
	
	if(section)
	{
		//get the appropriate delta sequence
		NETGHOSTDATABLOCK *ghostData;
		int frontback;
		ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;

		/* Moved the friendly fire stuff here, to fix a bug */
		if (netGameData.disableFriendlyFire)
		{
			if ((AvP.PlayerType == I_Marine) && (ghostData->type == I_BehaviourMarinePlayer))
				return;

			if ((AvP.PlayerType == I_Alien) && (ghostData->type == I_BehaviourAlienPlayer))
				return;

			if ((AvP.PlayerType == I_Predator) && (ghostData->type == I_BehaviourPredatorPlayer))
				return;
		}

		/* Check so grab hits the back of the target. */
		if ((ghostData->type == I_BehaviourMarinePlayer) ||
			(ghostData->type == I_BehaviourPredatorPlayer))
		{
			if (damage->Id == AMMO_FRISBEE_FIRE)
			{
				if (incoming)
				{
					if (incoming->vz < 0)
						return;
				}
				else
				{
					return;
				}
			}
		}

		/* Aliens ignore damage caused by grabbing (bug fix). */
		if (ghostData->type == I_BehaviourAlienPlayer)
		{
			if (damage->Id == AMMO_FRISBEE_FIRE)
			{
				return;
			}
		}

		if(ghostData->type==I_BehaviourMarinePlayer)
		{
			int CrouchSubSequence;
			int StandSubSequence;

			if (incoming) {
				if (incoming->vz>=0) {
					frontback=0;
				} else {
					frontback=1;
				}
			} else {
				/* Default to front. */
				frontback=1;
			}
		

			if (section==NULL) 
			{
				if (frontback==0) 
				{
					CrouchSubSequence=MCrSS_HitChestBack;
					StandSubSequence=MSSS_HitChestBack;
				} 
				else 
				{
					CrouchSubSequence=MCrSS_HitChestFront;
					StandSubSequence=MSSS_HitChestFront;
				}
			} else if (section->sempai->flags&section_flag_head) 
			{
				if (frontback==0) 
				{
					CrouchSubSequence=MCrSS_HitHeadBack;
					StandSubSequence=MSSS_HitHeadBack;
				}
				else 
				{
					CrouchSubSequence=MCrSS_HitHeadFront;
					StandSubSequence=MSSS_HitHeadFront;
				}
			} else if ((section->sempai->flags&section_flag_left_arm)
				||(section->sempai->flags&section_flag_left_hand)) 
			{
				if (frontback==0) 
				{
					CrouchSubSequence=MCrSS_HitRightArm;
					StandSubSequence=MSSS_HitRightArm;
				}
				else
				{
					CrouchSubSequence=MCrSS_HitLeftArm;
					StandSubSequence=MSSS_HitLeftArm;
				}
			} else if ((section->sempai->flags&section_flag_right_arm)
				||(section->sempai->flags&section_flag_right_hand)) 
			{
				if (frontback==0) 
				{
					CrouchSubSequence=MCrSS_HitLeftArm;
					StandSubSequence=MSSS_HitLeftArm;
				}
				else
				{
				 	CrouchSubSequence=MCrSS_HitRightArm;
				 	StandSubSequence=MSSS_HitRightArm;
				}
				
			} else if ((section->sempai->flags&section_flag_left_leg)
				||(section->sempai->flags&section_flag_left_foot)) 
			{
				CrouchSubSequence=MCrSS_HitLeftLeg;
				StandSubSequence=MSSS_HitLeftLeg;
				
			} else if ((section->sempai->flags&section_flag_right_leg)
				||(section->sempai->flags&section_flag_right_foot)) 
			{
				CrouchSubSequence=MCrSS_HitRightLeg;
				StandSubSequence=MSSS_HitRightLeg;
				
			} else {
				/* Chest or misc. hit. */
				if (frontback==0) {
					CrouchSubSequence=MCrSS_HitChestBack;
					StandSubSequence=MSSS_HitChestBack;
				} else {
					CrouchSubSequence=MCrSS_HitChestFront;
					StandSubSequence=MSSS_HitChestFront;
				}
			}

			if(ghostData->currentAnimSequence==MSQ_Crawl ||
			   ghostData->currentAnimSequence==MSQ_Crawl_Backwards ||
			   ghostData->currentAnimSequence==MSQ_Crouch)
			{
				delta_seq=(int)HMSQT_MarineCrouch;
				delta_sub_seq=(int)CrouchSubSequence;
			}
			else
			{
				delta_seq=(int)HMSQT_MarineStand;
				delta_sub_seq=(int)StandSubSequence;
			}

			PlayHitDeltaOnGhost(sbPtr,delta_seq,delta_sub_seq);
		}
		else if(ghostData->type==I_BehaviourPredatorPlayer)
		{
			int CrouchSubSequence;
			int StandSubSequence;

			if (incoming) {
				if (incoming->vz>=0) {
					frontback=0;
				} else {
					frontback=1;
				}
			} else {
				/* Default to front. */
				frontback=1;
			}
		
            if (section==NULL) 
            {
            	if (frontback==0) 
            	{
                	CrouchSubSequence=PCrSS_HitChestBack;
                	StandSubSequence=PSSS_HitChestBack;
                } 
                else 
                {
                	CrouchSubSequence=PCrSS_HitChestFront;
                	StandSubSequence=PSSS_HitChestFront;
                }
            } else if (section->sempai->flags&section_flag_head) 
            {
            	if (frontback==0) 
            	{
                	CrouchSubSequence=PCrSS_HitHeadBack;
                	StandSubSequence=PSSS_HitHeadBack;
                } 
                else 
                {
                	CrouchSubSequence=PCrSS_HitHeadFront;
                	StandSubSequence=PSSS_HitHeadFront;
                }
            } else if ((section->sempai->flags&section_flag_left_arm)
            	||(section->sempai->flags&section_flag_left_hand)) 
            {
            	if (frontback==0) 
            	{
            		CrouchSubSequence=PCrSS_HitRightArm;
            		StandSubSequence=PSSS_HitRightArm;
				}
				else
				{
            		CrouchSubSequence=PCrSS_HitLeftArm;
            		StandSubSequence=PSSS_HitLeftArm;
				}
            	
            } else if ((section->sempai->flags&section_flag_right_arm)
            	||(section->sempai->flags&section_flag_right_hand)) 
            {
            	if (frontback==0) 
				{
            	   	CrouchSubSequence=PCrSS_HitLeftArm;
            		StandSubSequence=PSSS_HitLeftArm;
				}
				else
				{
            	   	CrouchSubSequence=PCrSS_HitRightArm;
            		StandSubSequence=PSSS_HitRightArm;
				}
            	
            } else if ((section->sempai->flags&section_flag_left_leg)
            	||(section->sempai->flags&section_flag_left_foot)) {
           		CrouchSubSequence=PCrSS_HitLeftLeg;
           		StandSubSequence=PSSS_HitLeftLeg;
            	
            } else if ((section->sempai->flags&section_flag_right_leg)
            	||(section->sempai->flags&section_flag_right_foot)) {
           		CrouchSubSequence=PCrSS_HitRightLeg;
           		StandSubSequence=PSSS_HitRightLeg;
            } else {
                    /* Chest or misc. hit. */
           		if (frontback==0) {
           			CrouchSubSequence=PCrSS_HitChestBack;
           			StandSubSequence=PSSS_HitChestBack;
           		} else {
           			CrouchSubSequence=PCrSS_HitChestFront;
           			StandSubSequence=PSSS_HitChestFront;
           		}
            }
            

			if(ghostData->currentAnimSequence==PredSQ_Crawl ||
			   ghostData->currentAnimSequence==PredSQ_Crawl_Backwards ||
			   ghostData->currentAnimSequence==PredSQ_Crouch ||
			   ghostData->currentAnimSequence==PredSQ_CrouchedSwipe ||
			   ghostData->currentAnimSequence==PredSQ_CrawlingSwipe ||
			   ghostData->currentAnimSequence==PredSQ_CrawlingSwipe_Backwards)
			   
			{
				delta_seq=(int)HMSQT_PredatorCrouch;
				delta_sub_seq=(int)CrouchSubSequence;
			}
			else
			{
				delta_seq=(int)HMSQT_PredatorStand;
				delta_sub_seq=(int)StandSubSequence;
			}
            
			PlayHitDeltaOnGhost(sbPtr,delta_seq,delta_sub_seq);
            
		}
		
	}
	
	if (section) {
		sectionID=section->sempai->IDnumber;
	} else {
		sectionID=-1;
	}
	AddNetMsg_LocalObjectDamaged(sbPtr, damage, multiple,sectionID,delta_seq,delta_sub_seq,incoming);
}

/* This function maintains a player ghost's gunflash effect, and is called from 
processmsg_playerstate(). It maintains the ghost given by index, and takes as a 
parameter the state of the net-player's muzzle flash, as indicated in the last
network message.  It */ 
void HandleGhostGunFlashEffect(STRATEGYBLOCK *sbPtr, int gunFlashOn)
{
	NETGHOSTDATABLOCK *ghostData;

	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);

	/* aliens shouldn't have gun flashes */
	if((ghostData->type == I_BehaviourAlienPlayer) 
	 ||(ghostData->type == I_BehaviourPredatorPlayer)) 
	{
		ghostData->myGunFlash = NULL;
		return;
	}

	LOCALASSERT((ghostData->type == I_BehaviourMarinePlayer)||
				(ghostData->type == I_BehaviourPredatorPlayer));

	//ReleasePrintDebuggingText("Muzzle Flash %d\n",gunFlashOn);

	/* Handle two pistols? */
	if (ghostData->type == I_BehaviourMarinePlayer) {
		if (gunFlashOn==2) {
			ghostData->GunflashSectionPtr=GetThisSectionData(ghostData->HModelController.section_data,"dum L flash");
			if (ghostData->GunflashSectionPtr==NULL) {
				ghostData->GunflashSectionPtr=GetThisSectionData(ghostData->HModelController.section_data,"dum flash");
			}
		} else if (gunFlashOn==1) {
			ghostData->GunflashSectionPtr=GetThisSectionData(ghostData->HModelController.section_data,"dum flash");
		} else if (gunFlashOn==3) {
			/* Extrapolation. */
		}
	}

	if(ghostData->myGunFlash)
	{
		/* I've already got a gun flash... */
		if(gunFlashOn)
		{
			/* Maintain existing gun flash */
			LOCALASSERT(ghostData->GunflashSectionPtr);
			
			if (sbPtr->SBdptr)
			{
				ProveHModel(&ghostData->HModelController,sbPtr->SBdptr);
				MaintainNPCGunFlashEffect
				(
					ghostData->myGunFlash,
					&ghostData->GunflashSectionPtr->World_Offset,
					&ghostData->GunflashSectionPtr->SecMat
				);
			}
		}
		else
		{
			if (ghostData->GunFlashFrameStamp!=GlobalFrameCounter) {
				/* Maintain for at least one frame. */
				RemoveNPCGunFlashEffect(ghostData->myGunFlash);
				ghostData->myGunFlash = NULL;
			}
		} 
	}
	else
	{
		if(gunFlashOn)
		{
			/* new flash */
			LOCALASSERT(ghostData->GunflashSectionPtr);
			ghostData->GunFlashFrameStamp=GlobalFrameCounter;
	 		if (sbPtr->SBdptr)
	 		{
			 	ProveHModel(&ghostData->HModelController,sbPtr->SBdptr);

				if (ghostData->CurrentWeapon==WEAPON_SMARTGUN)
				{
					ghostData->myGunFlash = AddNPCGunFlashEffect
					(
						&ghostData->GunflashSectionPtr->World_Offset,
						&ghostData->GunflashSectionPtr->SecMat,
						SFX_MUZZLE_FLASH_SMARTGUN
					);
				}
				else if (ghostData->CurrentWeapon==WEAPON_FRISBEE_LAUNCHER)
				{
					/*ghostData->myGunFlash = AddNPCGunFlashEffect
					(
						&ghostData->GunflashSectionPtr->World_Offset,
						&ghostData->GunflashSectionPtr->SecMat,
						SFX_MUZZLE_FLASH_SKEETER
					);*/
				}
				else
				{
					ghostData->myGunFlash = AddNPCGunFlashEffect
					(
						&ghostData->GunflashSectionPtr->World_Offset,
						&ghostData->GunflashSectionPtr->SecMat,
						SFX_MUZZLE_FLASH_AMORPHOUS
					);
				}
			}
		}
	}
	// Eject shells from Pistol and Shotgun
	/*if ((ghostData->CurrentWeapon == WEAPON_MARINE_PISTOL) && (gunFlashOn) &&
		(LocalDetailLevels.Shells))
	{
		extern DISPLAYBLOCK *MakeAIPistolCasing(VECTORCH *position, MATRIXCH *orient);
		MakeAIPistolCasing(&ghostData->GunflashSectionPtr->World_Offset,&ghostData->GunflashSectionPtr->SecMat);
	}
	if ((ghostData->CurrentWeapon == WEAPON_GRENADELAUNCHER) && (gunFlashOn) &&
		(LocalDetailLevels.Shells))
	{
		extern DISPLAYBLOCK *MakeShotgunShell(VECTORCH *position, MATRIXCH *orient);
		MakeShotgunShell(&ghostData->GunflashSectionPtr->World_Offset,&ghostData->GunflashSectionPtr->SecMat);
	}*/


	#if 0
	/* KJL 15:32:57 13/05/98 - Tracer code - isn't working too well */
	if(ghostData->GunflashSectionPtr && !(FastRandom()&15)) 
	{
		VECTORCH endPosition = ghostData->GunflashSectionPtr->World_Offset;
		endPosition.vx += ghostData->GunflashSectionPtr->SecMat.mat31>>3; 
		endPosition.vy += ghostData->GunflashSectionPtr->SecMat.mat32>>3; 
		endPosition.vz += ghostData->GunflashSectionPtr->SecMat.mat33>>3; 
		MakeParticle(&(ghostData->GunflashSectionPtr->World_Offset),&endPosition,PARTICLE_TRACER);
	}
	#endif

}


/* Patrick 14/7/97 -----------------------------------------------------------
Shape animation control functions for player ghosts:

Init initilaises the character sequence, and should only be called when a new
player ghost is created, after the sb and datablock have been set up.

Update changes the sequence if appropriate (and calls set if the sbPtr has a dptr)

Set selects the correct sequence/type, infers the speed and follow-on sequences,
etc, and sets it.
------------------------------------------------------------------------------*/
static void InitPlayerGhostAnimSequence(STRATEGYBLOCK *sbPtr)
{
	NETGHOSTDATABLOCK *ghostData;
	AVP_BEHAVIOUR_TYPE type;
	
	LOCALASSERT(sbPtr);
	LOCALASSERT(!(sbPtr->SBdptr));
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);
	type = ghostData->type;

	InitShapeAnimationController(&ghostData->ShpAnimCtrl, GetShapeData(sbPtr->shapeIndex));
	switch(type)
	{
		case(I_BehaviourMarinePlayer):
		{
			ghostData->currentAnimSequence = MSQ_Stand;
			break;
		}
		case(I_BehaviourAlienPlayer):
		{
			ghostData->currentAnimSequence = ASQ_Stand;
			break;
		}
		case(I_BehaviourPredatorPlayer):
		{
			ghostData->currentAnimSequence = PredSQ_Stand;
			break;
		}
		default:
		{
			LOCALASSERT(1==0);
			break;
		}
	}
}

static void UpdatePlayerGhostAnimSequence(STRATEGYBLOCK *sbPtr, int sequence, int special)
{
	NETGHOSTDATABLOCK *ghostData;
	
	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);

	switch(ghostData->type)
	{
		case(I_BehaviourMarinePlayer):
		{
			//ReleasePrintDebuggingText("Update Marine with Special %d\n",special);
			/* if the current sequence is the same as the new sequence, do nothing */
			if (((MARINE_SEQUENCE)ghostData->currentAnimSequence == (MARINE_SEQUENCE)sequence)
				&&(ghostData->CurrentWeapon!=WEAPON_TWO_PISTOLS)
				&&(!((ghostData->CurrentWeapon==WEAPON_MARINE_PISTOL)&&(special)))
				) {
				return;
			}
			/* if we're dead, and passed dying, do nothing */
			if(((MARINE_SEQUENCE)ghostData->currentAnimSequence == MSQ_StandDeadFront)&&
			   ((MARINE_SEQUENCE)sequence == MSQ_StandDieFront)) return;
			if(((MARINE_SEQUENCE)ghostData->currentAnimSequence == MSQ_CrouchDead)&&
			   ((MARINE_SEQUENCE)sequence == MSQ_CrouchDie)) return;
			/* need to update the anim sequence, then*/
			SetPlayerGhostAnimationSequence(sbPtr, sequence, special);
			break;
		}
		case(I_BehaviourAlienPlayer):
		{
			/* if the current sequence is the same as the new sequence, do nothing */
			if(!HModelAnimation_IsFinished(&ghostData->HModelController))
			{
				if((ALIEN_SEQUENCE)ghostData->currentAnimSequence == (ALIEN_SEQUENCE)sequence)
				{
					if(ghostData->currentAnimSequence==ASQ_RunningAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_RunningTailStrike ||
					   ghostData->currentAnimSequence==ASQ_RunningAttack_Claw_Backwards ||
					   ghostData->currentAnimSequence==ASQ_RunningTailStrike_Backwards ||
					   ghostData->currentAnimSequence==ASQ_CrawlingAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_CrawlingAttack_Claw_Backwards ||
					   ghostData->currentAnimSequence==ASQ_CrawlingTailStrike ||
					   ghostData->currentAnimSequence==ASQ_CrawlingTailStrike_Backwards)
					{
						ghostData->HModelController.Looped=1;
						ghostData->HModelController.LoopAfterTweening=1;
					}

					#if 0
					if(ghostData->currentAnimSequence==ASQ_RunningAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_RunningTailStrike ||
					   ghostData->currentAnimSequence==ASQ_RunningAttack_Claw_Backwards ||
					   ghostData->currentAnimSequence==ASQ_RunningTailStrike_Backwards ||
					   ghostData->currentAnimSequence==ASQ_CrawlingAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_CrawlingAttack_Claw_Backwards ||
					   ghostData->currentAnimSequence==ASQ_CrawlingTailStrike ||
					   ghostData->currentAnimSequence==ASQ_CrawlingTailStrike_Backwards ||
					   ghostData->currentAnimSequence==ASQ_Run ||
					   ghostData->currentAnimSequence==ASQ_RunningTailPoise ||
					   ghostData->currentAnimSequence==ASQ_Run_Backwards ||
					   ghostData->currentAnimSequence==ASQ_RunningTailPoise_Backwards ||
					   ghostData->currentAnimSequence==ASQ_Crawl ||
					   ghostData->currentAnimSequence==ASQ_Scamper ||
					   ghostData->currentAnimSequence==ASQ_CrawlingTailPoise ||
					   ghostData->currentAnimSequence==ASQ_Crawl_Backwards ||
					   ghostData->currentAnimSequence==ASQ_Scamper_Backwards ||
					   ghostData->currentAnimSequence==ASQ_CrawlingTailPoise_Backwards)

					{
						if(ghostData->HModelController.Tweening==Controller_NoTweening)
						{
							HModel_SetToolsRelativeSpeed(&ghostData->HModelController,(512*ONE_FIXED)/18000/*ALIEN_MOVESCALE*/);
						}
					}
					#endif
					

					return;
				}
			}
			/* need to update the anim sequence, then*/
			SetPlayerGhostAnimationSequence(sbPtr, sequence, special);
			break;
		}
		case(I_BehaviourPredatorPlayer):
		{
			/* if the current sequence is the same as the new sequence, do nothing */
			if(!HModelAnimation_IsFinished(&ghostData->HModelController))
			{
				if(ghostData->currentAnimSequence == sequence)
				{
					if(ghostData->currentAnimSequence==PredSQ_RunningSwipe ||
					   ghostData->currentAnimSequence==PredSQ_RunningSwipe_Backwards ||
					   ghostData->currentAnimSequence==PredSQ_CrawlingSwipe ||
					   ghostData->currentAnimSequence==PredSQ_CrawlingSwipe_Backwards)
					{
						ghostData->HModelController.Looped=1;
						ghostData->HModelController.LoopAfterTweening=1;
					}
					return;
				}
			}
			/* if we're dead, and passed dying, do nothing */
			if(((PREDATOR_SEQUENCE)ghostData->currentAnimSequence == PredSQ_StandDead)&&
			   ((PREDATOR_SEQUENCE)sequence == PredSQ_StandDie)) return;
			if(((PREDATOR_SEQUENCE)ghostData->currentAnimSequence == PredSQ_CrouchDead)&&
			   ((PREDATOR_SEQUENCE)sequence == PredSQ_CrouchDie)) return;
			/* need to update the anim sequence, then*/
			SetPlayerGhostAnimationSequence(sbPtr, sequence, special);
			break;
		}

		case(I_BehaviourAlien):
		{
			/* if the current sequence is the same as the new sequence, do nothing */
			
			if(ghostData->currentAnimSequence == sequence) return;
			
			/* need to update the anim sequence, then*/
			SetPlayerGhostAnimationSequence(sbPtr, sequence, special);
			break;
		}


		default:
		{
			LOCALASSERT(1==0);
			break;
		}
	}
}

static void SetPlayerGhostAnimationSequence(STRATEGYBLOCK *sbPtr, int sequence, int special)
{
	NETGHOSTDATABLOCK *ghostData;
	AVP_BEHAVIOUR_TYPE type;
	
	DELTA_CONTROLLER *FireLeft,*FireRight;

	LOCALASSERT(sbPtr);	
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);
	type = ghostData->type;

	if(!(sbPtr->SBdptr))
	{
		/* update the ghost's current sequence: do this even if we don't have a displayblock */
		ghostData->currentAnimSequence = sequence;
		return; /* no displayblock: can't actually set it */
	}

	switch(type)
	{
		case(I_BehaviourMarinePlayer):
		{	
			switch((MARINE_SEQUENCE)sequence)
			{
				/* New animations for Cancer Black. */
				case(M_RELOAD):
				{
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineStand,45,ONE_FIXED,1);
					break;
				}
				case(MSQ_Walk):
				{
					if (ghostData->CurrentWeapon==WEAPON_SMARTGUN) {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1),1);
					} else if (ghostData->CurrentWeapon==WEAPON_MARINE_PISTOL) {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1),1);
					} else if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {
						/* Oh, no. */
						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Fire_From_Hips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Fire_From_Hips,(ONE_FIXED>>1),1);
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Standard,(ONE_FIXED>>1),1);
							}
						}
					} else {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Standard,(ONE_FIXED>>1),1);
					}
					break;
				}
				case(MSQ_Walk_Backwards):
				{
					if (ghostData->CurrentWeapon==WEAPON_SMARTGUN) {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1));
					} else if (ghostData->CurrentWeapon==WEAPON_MARINE_PISTOL) {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1));
					} else if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {
						/* Oh, no. */
						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Fire_From_Hips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Fire_From_Hips,(ONE_FIXED>>1),1);
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Standard,(ONE_FIXED>>1),1);
							}
						}
					} else {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Standard,(ONE_FIXED>>1));
					}
					ghostData->HModelController.Reversed=1;
					break;
				}
				case(MSQ_RunningFire):
				{
					if (ghostData->CurrentWeapon==WEAPON_MARINE_PISTOL) {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1));
					} else if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {

						FireRight=Get_Delta_Sequence(&ghostData->HModelController,"FireRight");
						if (FireRight==NULL) {
							FireRight=Add_Delta_Sequence(&ghostData->HModelController,"FireRight",HMSQT_MarineStand,MSSS_Attack_Primary,ONE_FIXED);
							FireRight->Playing=0;
							FireRight->Active=0;
						}

						FireLeft=Get_Delta_Sequence(&ghostData->HModelController,"FireLeft");
						if (FireLeft==NULL) {
							FireLeft=Add_Delta_Sequence(&ghostData->HModelController,"FireLeft",HMSQT_MarineStand,MSSS_Attack_Secondary,ONE_FIXED);
							FireLeft->Playing=0;
							FireLeft->Active=0;
						}

						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Fire_From_Hips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Fire_From_Hips,(ONE_FIXED>>1),1);
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Standard,(ONE_FIXED>>1),1);
							}
						}

						Start_Delta_Sequence(FireRight,HMSQT_MarineStand,MSSS_Attack_Primary,-1);
						FireRight->Playing=1;
						FireRight->Active=1;
						
					} else {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1));
					}
					break;
				}
				case(MSQ_RunningFireSecondary):
				{
					if (ghostData->CurrentWeapon==WEAPON_MARINE_PISTOL) {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1));
					} else if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {

						FireRight=Get_Delta_Sequence(&ghostData->HModelController,"FireRight");
						if (FireRight==NULL) {
							FireRight=Add_Delta_Sequence(&ghostData->HModelController,"FireRight",HMSQT_MarineStand,MSSS_Attack_Primary,ONE_FIXED);
							FireRight->Playing=0;
							FireRight->Active=0;
						}

						FireLeft=Get_Delta_Sequence(&ghostData->HModelController,"FireLeft");
						if (FireLeft==NULL) {
							FireLeft=Add_Delta_Sequence(&ghostData->HModelController,"FireLeft",HMSQT_MarineStand,MSSS_Attack_Secondary,ONE_FIXED);
							FireLeft->Playing=0;
							FireLeft->Active=0;
						}

						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Fire_From_Hips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Fire_From_Hips,(ONE_FIXED>>1),1);
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Standard,(ONE_FIXED>>1),1);
							}
						}

						Start_Delta_Sequence(FireLeft,HMSQT_MarineStand,MSSS_Attack_Secondary,-1);
						FireLeft->Playing=1;
						FireLeft->Active=1;
						
					} else {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1));
					}
					break;
				}
				case(MSQ_RunningFire_Backwards):
				{
					if (ghostData->CurrentWeapon==WEAPON_MARINE_PISTOL) {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1));
						ghostData->HModelController.Reversed=1;
					} else if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {

						FireRight=Get_Delta_Sequence(&ghostData->HModelController,"FireRight");
						if (FireRight==NULL) {
							FireRight=Add_Delta_Sequence(&ghostData->HModelController,"FireRight",HMSQT_MarineStand,MSSS_Attack_Primary,ONE_FIXED);
							FireRight->Playing=0;
							FireRight->Active=0;
						}

						FireLeft=Get_Delta_Sequence(&ghostData->HModelController,"FireLeft");
						if (FireLeft==NULL) {
							FireLeft=Add_Delta_Sequence(&ghostData->HModelController,"FireLeft",HMSQT_MarineStand,MSSS_Attack_Secondary,ONE_FIXED);
							FireLeft->Playing=0;
							FireLeft->Active=0;
						}

						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Fire_From_Hips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Fire_From_Hips,(ONE_FIXED>>1),1);
								ghostData->HModelController.Reversed=1;
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Standard,(ONE_FIXED>>1),1);
								ghostData->HModelController.Reversed=1;
							}
						}

						Start_Delta_Sequence(FireRight,HMSQT_MarineStand,MSSS_Attack_Primary,-1);
						FireRight->Playing=1;
						FireRight->Active=1;

					} else {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1));
						ghostData->HModelController.Reversed=1;
					}
					break;
				}
				case(MSQ_RunningFireSecondary_Backwards):
				{
					if (ghostData->CurrentWeapon==WEAPON_MARINE_PISTOL) {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1));
						ghostData->HModelController.Reversed=1;
					} else if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {

						FireRight=Get_Delta_Sequence(&ghostData->HModelController,"FireRight");
						if (FireRight==NULL) {
							FireRight=Add_Delta_Sequence(&ghostData->HModelController,"FireRight",HMSQT_MarineStand,MSSS_Attack_Primary,ONE_FIXED);
							FireRight->Playing=0;
							FireRight->Active=0;
						}

						FireLeft=Get_Delta_Sequence(&ghostData->HModelController,"FireLeft");
						if (FireLeft==NULL) {
							FireLeft=Add_Delta_Sequence(&ghostData->HModelController,"FireLeft",HMSQT_MarineStand,MSSS_Attack_Secondary,ONE_FIXED);
							FireLeft->Playing=0;
							FireLeft->Active=0;
						}

						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Fire_From_Hips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Fire_From_Hips,(ONE_FIXED>>1),1);
								ghostData->HModelController.Reversed=1;
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineRun)||(ghostData->HModelController.Sub_Sequence!=MRSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineRun,(int)MRSS_Standard,(ONE_FIXED>>1),1);
								ghostData->HModelController.Reversed=1;
							}
						}

						Start_Delta_Sequence(FireLeft,HMSQT_MarineStand,MSSS_Attack_Secondary,-1);
						FireLeft->Playing=1;
						FireLeft->Active=1;

					} else {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineRun,(int)MRSS_Attack_Primary,(ONE_FIXED>>1));
						ghostData->HModelController.Reversed=1;
					}
					break;
				}
				case(MSQ_StandingFire):
				{
					if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {

						FireRight=Get_Delta_Sequence(&ghostData->HModelController,"FireRight");
						if (FireRight==NULL) {
							FireRight=Add_Delta_Sequence(&ghostData->HModelController,"FireRight",HMSQT_MarineStand,MSSS_Attack_Primary,ONE_FIXED);
							FireRight->Playing=0;
							FireRight->Active=0;
						}

						FireLeft=Get_Delta_Sequence(&ghostData->HModelController,"FireLeft");
						if (FireLeft==NULL) {
							FireLeft=Add_Delta_Sequence(&ghostData->HModelController,"FireLeft",HMSQT_MarineStand,MSSS_Attack_Secondary,ONE_FIXED);
							FireLeft->Playing=0;
							FireLeft->Active=0;
						}

						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineStand)||(ghostData->HModelController.Sub_Sequence!=MSSS_FireFromHips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineStand,(int)MSSS_FireFromHips,(ONE_FIXED>>1),1);
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineStand)||(ghostData->HModelController.Sub_Sequence!=MSSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineStand,(int)MSSS_Standard,(ONE_FIXED>>1),1);
							}
						}

						Start_Delta_Sequence(FireRight,HMSQT_MarineStand,MSSS_Attack_Primary,-1);
						FireRight->Playing=1;
						FireRight->Active=1;

					} else {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineStand,(int)MSSS_Attack_Primary,ONE_FIXED);
					}
					break;
				}
				case(MSQ_StandingFireSecondary):
				{
					if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {

						FireRight=Get_Delta_Sequence(&ghostData->HModelController,"FireRight");
						if (FireRight==NULL) {
							FireRight=Add_Delta_Sequence(&ghostData->HModelController,"FireRight",HMSQT_MarineStand,MSSS_Attack_Primary,ONE_FIXED);
							FireRight->Playing=0;
							FireRight->Active=0;
						}

						FireLeft=Get_Delta_Sequence(&ghostData->HModelController,"FireLeft");
						if (FireLeft==NULL) {
							FireLeft=Add_Delta_Sequence(&ghostData->HModelController,"FireLeft",HMSQT_MarineStand,MSSS_Attack_Secondary,ONE_FIXED);
							FireLeft->Playing=0;
							FireLeft->Active=0;
						}

						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineStand)||(ghostData->HModelController.Sub_Sequence!=MSSS_FireFromHips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineStand,(int)MSSS_FireFromHips,(ONE_FIXED>>1),1);
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineStand)||(ghostData->HModelController.Sub_Sequence!=MSSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineStand,(int)MSSS_Standard,(ONE_FIXED>>1),1);
							}
						}

						Start_Delta_Sequence(FireLeft,HMSQT_MarineStand,MSSS_Attack_Secondary,-1);
						FireLeft->Playing=1;
						FireLeft->Active=1;

					} else {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineStand,(int)MSSS_Attack_Primary,ONE_FIXED);
					}
					break;
				}
				case(MSQ_Stand):
				{
					BOOL standing_attack=FALSE;
					/* Removed the test for 'standing swipe'... */
					if(ghostData->currentAnimSequence>=MSQ_BaseOfCudgelAttacks)
					{
						if((ghostData->currentAnimSequence-MSQ_BaseOfCudgelAttacks)%2==1)
						{
							standing_attack=TRUE;
						}
					}
					
					if(standing_attack)
					{
						//if currently playing a standing attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}

					if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {
						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineStand)||(ghostData->HModelController.Sub_Sequence!=MSSS_FireFromHips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineStand,(int)MSSS_FireFromHips,(ONE_FIXED>>1),1);
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineStand)||(ghostData->HModelController.Sub_Sequence!=MSSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineStand,(int)MSSS_Standard,(ONE_FIXED>>1),1);
							}
						}
					} else {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineStand,(int)MSSS_Standard,ONE_FIXED,1);
					}
					break;
				}
				case(MSQ_Crawl):
				{
					if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {
						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineCrawl)||(ghostData->HModelController.Sub_Sequence!=MCSS_FireFromHips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineCrawl,(int)MCSS_FireFromHips,(ONE_FIXED>>1),1);
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineCrawl)||(ghostData->HModelController.Sub_Sequence!=MCSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineCrawl,(int)MCSS_Standard,(ONE_FIXED>>1),1);
							}
						}
					} else {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_MarineCrawl,(int)MCSS_Standard,(ONE_FIXED>>1),1);
					}
					break;
				}
				case(MSQ_Crawl_Backwards):
				{
					if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {
						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineCrawl)||(ghostData->HModelController.Sub_Sequence!=MCSS_FireFromHips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineCrawl,(int)MCSS_FireFromHips,(ONE_FIXED>>1),1);
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineCrawl)||(ghostData->HModelController.Sub_Sequence!=MCSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineCrawl,(int)MCSS_Standard,(ONE_FIXED>>1),1);
							}
						}
					} else {
						InitHModelSequence(&ghostData->HModelController,(int)HMSQT_MarineCrawl,(int)MCSS_Standard,(ONE_FIXED>>1));
					}
					ghostData->HModelController.Reversed=1;
					break;
				}
				case(MSQ_Crouch):
				{
					BOOL crouched_attack=FALSE;
					if(ghostData->currentAnimSequence>=MSQ_BaseOfCudgelAttacks)
					{
						if((ghostData->currentAnimSequence-MSQ_BaseOfCudgelAttacks)%2==0)
						{
							crouched_attack=TRUE;
						}
					}
					
					if(crouched_attack)
					{
						//if currently playing a standing attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					
					if (ghostData->CurrentWeapon==WEAPON_TWO_PISTOLS) {
						if (special) {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineCrouch)||(ghostData->HModelController.Sub_Sequence!=MCrSS_FireFromHips)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineCrouch,(int)MCrSS_FireFromHips,(ONE_FIXED>>1),1);
							}
						} else {
							if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineCrouch)||(ghostData->HModelController.Sub_Sequence!=MCrSS_Standard)) {
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineCrouch,(int)MCrSS_Standard,(ONE_FIXED>>1),1);
							}
						}
					} else {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_MarineCrouch,(int)MCrSS_Standard,ONE_FIXED,1);
					}
					break;
				}
				case(MSQ_Jump):
				{
					if (HModelSequence_Exists(&ghostData->HModelController,(int)HMSQT_MarineStand,(int)MSSS_Jump)) {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_MarineStand,(int)MSSS_Jump,(ONE_FIXED),0);
						break;
					} else {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineStand,(int)MSSS_Standard,ONE_FIXED,1);
						break;
					}
					
				}
				case(MSQ_Taunt):
				{
					if (HModelSequence_Exists(&ghostData->HModelController,(int)HMSQT_MarineStand,(int)MSSS_Taunt_One)) {
						if ((ghostData->HModelController.Sequence_Type!=(int)HMSQT_MarineStand)||(ghostData->HModelController.Sub_Sequence!=MSSS_Taunt_One)) {
							InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_MarineStand,(int)MSSS_Taunt_One,(TAUNT_LENGTH-(ONE_FIXED>>3)),0);
						}
						break;
					} else {
						/* Default. */
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_MarineStand,(int)MSSS_Standard,ONE_FIXED,1);
						break;
					}
					
				}
				case(MSQ_StandDieFront):
				case(MSQ_CrouchDie):
				{
					break;
				}
				default:
				{
					int attack;
					/* Now includes cudgel attacks... */
					if ((int)sequence>=(int)MSQ_BaseOfCudgelAttacks) {
						attack=(int)sequence-(int)MSQ_BaseOfCudgelAttacks;
						
						if(ghostData->currentAnimSequence>=MSQ_BaseOfCudgelAttacks)
						{
							//already playing some form of attack
							if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
						}
						switch (attack) {
							case 0:
								/* Crouching Primary Fire. */
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_MarineCrouch,(int)MCrSS_Attack_Primary,-1,0);
								break;
							case 1:
								/* Standing Primary Fire. */
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_MarineStand,(int)MSSS_Attack_Primary,-1,0);
								break;
							case 2:
								/* Crouching Secondary Fire. */
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_MarineCrouch,(int)MCrSS_Attack_Secondary,-1,0);
								break;
							case 3:
								/* Standing Secondary Fire. */
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_MarineStand,(int)MSSS_Attack_Secondary,-1,0);
								break;
							default:
							/* KJL 16:56:15 26/11/98 - hmm. Any 'unknown' attacks will be replaced with a fast jab */
							{
								if (attack&1)
								{
									/* Standing Fast Jab. */
									InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_MarineStand,(int)MSSS_Attack_Primary,-1,0);
								}
								else
								{
									/* Crouching Fast Jab. */
									InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_MarineCrouch,(int)MCrSS_Attack_Primary,-1,0);
								}
								break;
							}
						}
					} else {
						attack=(int)sequence-(int)MSQ_BaseOfCudgelAttacks;
						
						if (attack) {
							InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorCrouch,(int)PCrSS_Attack_Primary,-1,0);
						} else {
							InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorStand,(int)PSSS_Attack_Primary,-1,0);
						}
					}
					break;
				}
			}
			break;
		}
		case(I_BehaviourAlienPlayer):
		case(I_BehaviourAlien):
		{
			if (ghostData->Class == CLASS_MEDIC_PR)
			{
				switch(sequence)
				{
					case ASQ_Stand:	// stand
					{
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_QueenLeftStanceTemplate,(int)QLSTSS_Standard,ONE_FIXED,0);
						break;
					}
					case ASQ_Taunt: //taunt
					{
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_QueenRightStanceFull,(int)QRSFSS_Taunt,(TAUNT_LENGTH-(ONE_FIXED>>3)),0);
						break;
					}
					case ASQ_StandingAttack_Claw: //claw
					case ASQ_RunningAttack_Claw_Backwards:
					case ASQ_RunningAttack_Claw:
					//case ASQ_StandingTailPoise:
					//case ASQ_StandingTailStrike:
					//case ASQ_RunningTailPoise_Backwards:
					//case ASQ_RunningTailStrike_Backwards:
					//case ASQ_RunningTailPoise:
					//case ASQ_RunningTailStrike:
					{
						// Select an anim that looks like the queen is clawing...
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_QueenRightStanceTemplate,2,ONE_FIXED,0);

						if (sequence == ASQ_RunningAttack_Claw_Backwards)
							ghostData->HModelController.Reversed = 1;

						break;
					}
					case ASQ_Run_Backwards:	// run
					case ASQ_Run:
					{
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_QueenGeneral,(int)QGSS_Walk,ONE_FIXED,1);

						if (sequence == ASQ_Run_Backwards)
							ghostData->HModelController.Reversed = 1;

						break;
					}
				}
			}
			else if ((ghostData->Class == CLASS_EXF_W_SPEC) ||
					 (ghostData->Class == CLASS_CHESTBURSTER))
			{
				switch((FHUG_SEQUENCE)sequence)
				{
					case(FhSQ_Run):
					{
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_Hugger,(int)HSS_Run,ONE_FIXED,1);
						break;
					}
					case(FhSQ_Attack):
					{
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_Hugger,(int)HSS_Attack,ONE_FIXED,0);
						break;
					}
					case(FhSQ_Stand):
					{
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_Hugger,(int)HSS_Stand,ONE_FIXED,0);
						break;
					}
					case(FhSQ_Jump):
					{
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_Hugger,(int)HSS_Jump,ONE_FIXED,0);
						break;
					}
				}
			}
			else if (ghostData->Class == CLASS_EXF_SNIPER)
			{
				int anim=2, seq=0, rev=0, time=ONE_FIXED, loop=1;

				switch(sequence)
				{
					case(ASQ_Stand):
					case(ASQ_StandingTailPoise):
						anim = 2;
						seq = 0;
						break;
					case(ASQ_StandingAttack_Claw):
					case(ASQ_RunningAttack_Claw):
					case(ASQ_RunningAttack_Claw_Backwards):
						anim = 2;
						loop = 0;
						switch(FastRandom()%3)
						{
							case 0:
								seq = 2;
								break;
							case 1:
								seq = 20;
								break;
							case 2:
								seq = 25;
								break;
						}
						if (sequence == ASQ_RunningAttack_Claw_Backwards)
							rev = 1;
						break;
					case(ASQ_StandingTailStrike):
					case(ASQ_RunningTailStrike):
					case(ASQ_RunningTailStrike_Backwards):
						anim = 2;
						seq = 3;
						loop = 0;
						if (sequence == ASQ_RunningTailStrike_Backwards)
							rev = 1;
						break;
					case(ASQ_Run):
					case(ASQ_RunningTailPoise):
						anim = 0;
						seq = 0;
						break;
					case(ASQ_Run_Backwards):
					case(ASQ_RunningTailPoise_Backwards):
						anim = 0;
						seq = 0;
						rev = 1;
						break;
					case(ASQ_Crawl):
					case(ASQ_CrawlingTailPoise):
					case(ASQ_Scamper):
						anim = 1;
						seq = 0;
						break;
					case(ASQ_Crawl_Backwards):
					case(ASQ_CrawlingTailPoise_Backwards):
					case(ASQ_Scamper_Backwards):
						anim = 1;
						seq = 0;
						rev = 1;
						break;
					case(ASQ_CrawlingAttack_Claw):
					case(ASQ_CrawlingAttack_Claw_Backwards):
					case(ASQ_CrouchedAttack_Claw):
						anim = 3;
						seq = 4;
						time = -1;
						loop = 0;

						if (sequence == ASQ_CrawlingAttack_Claw_Backwards)
							rev = 1;

						break;
					case(ASQ_CrawlingTailStrike):
					case(ASQ_CrawlingTailStrike_Backwards):
					case(ASQ_CrouchedTailStrike):
						anim = 3;
						seq = 3;
						time = -1;
						loop = 0;

						if (sequence == ASQ_CrawlingTailStrike_Backwards)
							rev = 1;

						break;
					case(ASQ_Crouch):
					case(ASQ_CrouchedTailPoise):
						anim = 3;
						seq = 0;
						break;
					case(ASQ_Jump):
					case(ASQ_Pounce):
					case(ASQ_JumpingTailPoise):
					case(ASQ_JumpingTailStrike):
						anim = 0;
						seq = 3;
						break;
					case(ASQ_Taunt):
						anim = 2;
						seq = 19;
						break;
				}
				if (sequence != ASQ_Taunt)
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),anim,seq,time,loop);
				else
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),anim,seq,(TAUNT_LENGTH-(ONE_FIXED>>3)),0);
			} 
			else 
			{
			switch((ALIEN_SEQUENCE)sequence)
			{
				case(ASQ_Stand):
				case(ASQ_StandingTailPoise):
				{
					if(ghostData->currentAnimSequence==ASQ_StandingAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_StandingTailStrike ||
					   ghostData->currentAnimSequence==ASQ_Eat)
					{
						//if currently playing a standing attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}

					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienStand,(int)ASSS_Standard,ONE_FIXED,1);
					break;
				}
				case(ASQ_StandingAttack_Claw):
				{
					int chosenSequence;
					
					if(ghostData->currentAnimSequence==ASQ_StandingAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_StandingTailStrike ||
					   ghostData->currentAnimSequence==ASQ_Eat)
					{
						//if already playing a standing attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					
					/*Pick a random claw attack*/
					switch(FastRandom()%6)
					{
						case 0:
							chosenSequence=ASSS_Attack_Right_Swipe_In;
							break;
						case 1:
							chosenSequence=ASSS_Attack_Left_Swipe_In;
							break;
						case 2:
							chosenSequence=ASSS_Attack_Both_In;
							break;
						case 3:
							chosenSequence=ASSS_Attack_Both_Down;
							break;
						case 4:
							chosenSequence=ASSS_Attack_Low_Left_Swipe;
							break;
						case 5:
							chosenSequence=ASSS_Attack_Low_Right_Swipe;
							break;

					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienStand,chosenSequence,-1/*(ONE_FIXED/6)*/,0);
					
					break;
				}
				
				case(ASQ_StandingTailStrike):
				{
					if(ghostData->currentAnimSequence==ASQ_StandingAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_StandingTailStrike ||
					   ghostData->currentAnimSequence==ASQ_Eat)
					{
						//if already playing a standing attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienStand,(int)ASSS_Attack_Tail,-1/*(ONE_FIXED/6)*/,0);
					break;
				}
				case(ASQ_Eat) :
				{
					if(ghostData->currentAnimSequence==ASQ_StandingAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_StandingTailStrike ||
					   ghostData->currentAnimSequence==ASQ_Eat)
					{
						//if already playing a standing attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienStand,(int)ASSS_Attack_Bite,-1,0);
					break;
				}

				case(ASQ_Run):
				case(ASQ_RunningTailPoise):
				{
					if(ghostData->currentAnimSequence==ASQ_RunningAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_RunningTailStrike)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							ghostData->HModelController.Looped=0;
							ghostData->HModelController.LoopAfterTweening=0;
							return;
						}
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienRun,(int)ARSS_Standard,ONE_FIXED,1);
					break;
				}
				case(ASQ_Run_Backwards):
				case(ASQ_RunningTailPoise_Backwards):
				{
					if(ghostData->currentAnimSequence==ASQ_RunningAttack_Claw_Backwards ||
					   ghostData->currentAnimSequence==ASQ_RunningTailStrike_Backwards)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							ghostData->HModelController.Looped=0;
							ghostData->HModelController.LoopAfterTweening=0;
							return;
						}
					}
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_AlienRun,(int)ARSS_Standard,ONE_FIXED);
					ghostData->HModelController.Reversed=1;
					break;
				}
				case(ASQ_RunningAttack_Claw):
				case(ASQ_RunningTailStrike):
				{
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienRun,(int)ARSS_Attack_Swipe,ONE_FIXED/*(ONE_FIXED/6)*/,1);
					break;
				}
				case(ASQ_RunningAttack_Claw_Backwards):
				case(ASQ_RunningTailStrike_Backwards):
				{
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_AlienRun,(int)ARSS_Attack_Swipe,ONE_FIXED/*(ONE_FIXED/6)*/);
					ghostData->HModelController.Reversed=1;
					ghostData->HModelController.Looped=1;
					break;
				}
				case(ASQ_Crawl):
				case(ASQ_CrawlingTailPoise):
				{
					if(ghostData->currentAnimSequence==ASQ_CrawlingAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_CrawlingTailStrike)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							ghostData->HModelController.Looped=0;
							ghostData->HModelController.LoopAfterTweening=0;
							return;
						}
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienCrawl,(int)ACSS_Standard,ONE_FIXED,1);
					break;
				}
				case (ASQ_Scamper):
				{
					if(ghostData->currentAnimSequence==ASQ_CrawlingAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_CrawlingTailStrike)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							ghostData->HModelController.Looped=0;
							ghostData->HModelController.LoopAfterTweening=0;
							return;
						}
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienCrawl,(int)ACSS_Scamper,ONE_FIXED,1);
					break;
				}

				case(ASQ_Crawl_Backwards):
				case(ASQ_CrawlingTailPoise_Backwards):
				{
					if(ghostData->currentAnimSequence==ASQ_CrawlingAttack_Claw_Backwards ||
					   ghostData->currentAnimSequence==ASQ_CrawlingTailStrike_Backwards)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							ghostData->HModelController.Looped=0;
							ghostData->HModelController.LoopAfterTweening=0;
							return;
						}
					}
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_AlienCrawl,(int)ACSS_Standard,ONE_FIXED);
					ghostData->HModelController.Reversed=1;
					break;
				}
				case(ASQ_Scamper_Backwards):
				{
					if(ghostData->currentAnimSequence==ASQ_CrawlingAttack_Claw_Backwards ||
					   ghostData->currentAnimSequence==ASQ_CrawlingTailStrike_Backwards)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							ghostData->HModelController.Looped=0;
							ghostData->HModelController.LoopAfterTweening=0;
							return;
						}
					}
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_AlienCrawl,(int)ACSS_Scamper,ONE_FIXED);
					ghostData->HModelController.Reversed=1;
					break;
				}

				case(ASQ_CrawlingAttack_Claw):
				case(ASQ_CrawlingTailStrike):
				{
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienCrouch,(int)ACrSS_Attack_Swipe,ONE_FIXED/*(ONE_FIXED/6)*/,1);
					break;
				}
				case(ASQ_CrawlingAttack_Claw_Backwards):
				case(ASQ_CrawlingTailStrike_Backwards):
				{
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_AlienCrouch,(int)ACrSS_Attack_Swipe,ONE_FIXED/*(ONE_FIXED/6)*/);
					ghostData->HModelController.Reversed=1;
					ghostData->HModelController.Looped=1;
					break;
				}
				#if 0
				case(ASQ_CrawlingTailStrike):
				{
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienCrawl,(int)ACSS_Attack_Tail,ONE_FIXED/*(ONE_FIXED/6)*/,1);
					break;
				}
				case(ASQ_CrawlingTailStrike_Backwards):
				{
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_AlienCrawl,(int)ACSS_Attack_Tail,ONE_FIXED/*(ONE_FIXED/6)*/);
					ghostData->HModelController.Reversed=1;
					ghostData->HModelController.Looped=1;
					break;
				}
				#endif
				case(ASQ_Crouch):
				case(ASQ_CrouchedTailPoise):
				{
					if(ghostData->currentAnimSequence==ASQ_CrouchedAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_CrouchEat||	
					   ghostData->currentAnimSequence==ASQ_CrouchedTailStrike)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienCrouch,(int)ACSS_Standard,ONE_FIXED,1);
					break;
				}
				case(ASQ_CrouchedAttack_Claw):
				{
					if(ghostData->currentAnimSequence==ASQ_CrouchedAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_CrouchedTailStrike ||
					   ghostData->currentAnimSequence==ASQ_CrouchEat)
					{
						//if already playing a crouched attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienCrouch,(int)ACrSS_Attack_Swipe,-1/*(ONE_FIXED/6)*/,0);
					break;
				}
				case(ASQ_CrouchedTailStrike):
				{
					if(ghostData->currentAnimSequence==ASQ_CrouchedAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_CrouchedTailStrike ||
					   ghostData->currentAnimSequence==ASQ_CrouchEat)
					{
						//if already playing a crouched attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienCrouch,(int)ACrSS_Attack_Tail,-1/*(ONE_FIXED/6)*/,0);
					break;
				}
				case(ASQ_CrouchEat) :
				{
					if(ghostData->currentAnimSequence==ASQ_CrouchedAttack_Claw ||
					   ghostData->currentAnimSequence==ASQ_CrouchedTailStrike ||
					   ghostData->currentAnimSequence==ASQ_CrouchEat)
					{
						//if already playing a crouched attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienCrouch,(int)ACrSS_Attack_Bite,-1,0);
					break;
				}
				case(ASQ_Jump):
				case(ASQ_Pounce):
				case(ASQ_JumpingTailPoise):
				case(ASQ_JumpingTailStrike):
				{
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienRun,(int)ARSS_Jump,ONE_FIXED,1);
					break;
				}
				case(ASQ_Taunt):
				{
					if (HModelSequence_Exists(&ghostData->HModelController,(int)HMSQT_AlienStand,(int)ASSS_Taunt)) {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_AlienStand,(int)ASSS_Taunt,(TAUNT_LENGTH-(ONE_FIXED>>3)),0);
						break;
					} else {
						/* Default. */
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_AlienStand,(int)ASSS_Standard,ONE_FIXED,1);
						break;
					}
					
				}
				default:
				{
					/* no other sequences should be set */
					LOCALASSERT("Unknown Alien anim sequence"==0);
					break;
				}	
			} // Facehugger Sequences...
			}
		  	break;
		}
		case(I_BehaviourPredatorPlayer):
		{
			switch((PREDATOR_SEQUENCE)sequence)
			{
				/* Self-Destruct. */
				case(P_ACTIVATE_BOMB):
				{
					if (ghostData->currentAnimSequence == 17)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorCrouch,17,(ONE_FIXED*6),0);
					break;
				}
				case(P_LAUGH):
				{
					if (ghostData->currentAnimSequence == 18)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorCrouch,18,(ONE_FIXED*3),0);
					break;
				}
				/* Predator combi-stick attacks. */
				case(P_SPEAR_STAB):
				{
					int anim=0;

					switch(FastRandom()%4)
					{
						case 0:
							anim=23;
							break;
						case 1:
							anim=24;
							break;
						case 2:
							anim=25;
							break;
						case 3:
							anim=3;
							break;
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorStand,anim,(ONE_FIXED<<1),0);
					break;
				}
				case(P_CROUCH_SPEAR_STAB):
				{
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorCrouch,3,ONE_FIXED,0);
					break;
				}
				case(PredSQ_Stand):
				{
					BOOL standing_attack=FALSE;
					if(ghostData->currentAnimSequence==PredSQ_StandingSwipe) 
					{
						standing_attack=TRUE;
					}
					else if(ghostData->currentAnimSequence>=PredSQ_BaseOfWristbladeAttacks)
					{
						if((ghostData->currentAnimSequence-PredSQ_BaseOfWristbladeAttacks)%2==1)
						{
							standing_attack=TRUE;
						}
					}
					
					if(standing_attack)
					{
						//if currently playing a standing attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorStand,(int)PSSS_Standard,ONE_FIXED,1);
					break;
				}
				case(PredSQ_StandingSwipe):
				{
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorStand,(int)PSSS_Attack_Primary,-1,0);
					break;
				}
				
				case(PredSQ_RunningSwipe):
				{
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_PredatorRun,(int)PRSS_Attack_Primary,(ONE_FIXED>>1));
					ghostData->HModelController.Looped=0;
					break;
				}
				
				case(PredSQ_RunningSwipe_Backwards):
				{
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_PredatorRun,(int)PRSS_Attack_Primary,(ONE_FIXED>>1));
					ghostData->HModelController.Looped=0;
					ghostData->HModelController.Reversed=1;
					break;
				}
				
				case(PredSQ_Run):
				{
					if(ghostData->currentAnimSequence==PredSQ_RunningSwipe)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							ghostData->HModelController.Looped=0;
							ghostData->HModelController.LoopAfterTweening=0;
							return;
						}
					}
					
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorRun,(int)PSSS_Standard,(ONE_FIXED>>1),1);

					break;
				}

				case(PredSQ_Run_Backwards):
				{
					if(ghostData->currentAnimSequence==PredSQ_RunningSwipe_Backwards)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							ghostData->HModelController.Looped=0;
							ghostData->HModelController.LoopAfterTweening=0;
							return;
						}
					}
					
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_PredatorRun,(int)PSSS_Standard,(ONE_FIXED>>1));
					ghostData->HModelController.Reversed=1;
					break;
				}
				case(PredSQ_Crawl):
				{
					if(ghostData->currentAnimSequence==PredSQ_CrawlingSwipe)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							ghostData->HModelController.Looped=0;
							ghostData->HModelController.LoopAfterTweening=0;
							return;
						}
					}

					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorCrawl,(int)PCSS_Standard,(ONE_FIXED>>1),1);
					break;
				}
				case(PredSQ_Crawl_Backwards):
				{
					if(ghostData->currentAnimSequence==PredSQ_CrawlingSwipe_Backwards)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							ghostData->HModelController.Looped=0;
							ghostData->HModelController.LoopAfterTweening=0;
							return;
						}
					}

					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_PredatorCrawl,(int)PCSS_Standard,(ONE_FIXED>>1));
					ghostData->HModelController.Reversed=1;
					break;
				}
				case(PredSQ_Crouch):
				{
					BOOL crouched_attack=FALSE;
					if(ghostData->currentAnimSequence==PredSQ_CrouchedSwipe) 
					{
						crouched_attack=TRUE;
					}
					else if(ghostData->currentAnimSequence>=PredSQ_BaseOfWristbladeAttacks)
					{
						if((ghostData->currentAnimSequence-PredSQ_BaseOfWristbladeAttacks)%2==0)
						{
							crouched_attack=TRUE;
						}
					}
					
					if(crouched_attack)
					{
						//if currently playing a standing attack allow it to finish first
						if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
					}
					
					if(ghostData->currentAnimSequence==PredSQ_CrouchedSwipe)
					{
						if(!HModelAnimation_IsFinished(&ghostData->HModelController))
						{
							return;
						}
					}

					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorCrouch,(int)PCrSS_Standard,ONE_FIXED,1);
					break;
				}
				case(PredSQ_CrouchedSwipe):
				{
					InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorCrouch,(int)PCrSS_Attack_Primary,-1,0);
					break;
				}
				case(PredSQ_CrawlingSwipe):
				{
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_PredatorCrawl,(int)PCSS_Attack_Primary,(ONE_FIXED>>1));
					ghostData->HModelController.Looped=0;
					break;
				}
				case(PredSQ_CrawlingSwipe_Backwards):
				{
					InitHModelSequence(&ghostData->HModelController,(int)HMSQT_PredatorCrawl,(int)PCSS_Attack_Primary,(ONE_FIXED>>1));
					ghostData->HModelController.Looped=0;
					ghostData->HModelController.Reversed=1;
					break;
				}
				case(PredSQ_Jump):
				{
					if (HModelSequence_Exists(&ghostData->HModelController,(int)HMSQT_PredatorStand,(int)PSSS_Jump)) {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorStand,(int)PSSS_Jump,(ONE_FIXED),0);
						break;
					} else {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_PredatorStand,(int)PSSS_Standard,ONE_FIXED,1);
						break;
					}
					
				}
				case(PredSQ_Taunt):
				{
					if (HModelSequence_Exists(&ghostData->HModelController,(int)HMSQT_PredatorStand,(int)PSSS_Taunt_One)) {
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorStand,(int)PSSS_Taunt_One,(TAUNT_LENGTH-(ONE_FIXED>>3)),0);
						break;
					} else {
						/* Default. */
						InitHModelTweening(&ghostData->HModelController,(ONE_FIXED)>>3,(int)HMSQT_PredatorStand,(int)PSSS_Standard,ONE_FIXED,1);
						break;
					}
					
				}
				case(PredSQ_StandDie):
				{
					break;
				}
				case(PredSQ_CrouchDie):
				{
					break;
				}
				default:
				{
					int attack;
					/* Must cover STAFF ATTACKS, and WRISTBLADE ATTACKS! */
					if ((int)sequence>=(int)PredSQ_BaseOfWristbladeAttacks) {
						attack=(int)sequence-(int)PredSQ_BaseOfWristbladeAttacks;
						
						if(ghostData->currentAnimSequence>=PredSQ_BaseOfWristbladeAttacks)
						{
							//already playing some form of attack
							if(!HModelAnimation_IsFinished(&ghostData->HModelController)) return;
						}
						switch (attack) {
							case 0:
								/* Crouching Fast Jab. */
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_PredatorCrouch,(int)PCrSS_Attack_Primary,-1,0);
								break;
							case 1:
								/* Standing Fast Jab. */
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_PredatorStand,(int)PSSS_Attack_Quick_Jab,-1,0);
								break;
							case 2:
								/* Crouching Slow Jab. */
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_PredatorCrouch,(int)PCrSS_Attack_Primary,-1,0);
								break;
							case 3:
								/* Standing Slow Jab. */
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_PredatorStand,(int)PSSS_Attack_Primary,-1,0);
								break;
							case 4:
								/* Crouching Uppercut. */
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_PredatorCrouch,(int)PCrSS_Attack_Primary,-1,0);
								break;
							case 5:
								/* Standing Uppercut. */
								InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_PredatorStand,(int)PSSS_Attack_Uppercut,-1,0);
								break;
							default:
							/* KJL 16:56:15 26/11/98 - hmm. Any 'unknown' attacks will be replaced with a fast jab */
							{
								if (attack&1)
								{
									/* Standing Fast Jab. */
									InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_PredatorStand,(int)PSSS_Attack_Quick_Jab,-1,0);
								}
								else
								{
									/* Crouching Fast Jab. */
									InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>4),(int)HMSQT_PredatorCrouch,(int)PCrSS_Attack_Primary,-1,0);
								}
								break;
							}
						}
					} else {
						attack=(int)sequence-(int)PredSQ_BaseOfStaffAttacks;
						
						if (attack) {
							InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorCrouch,(int)PCrSS_Attack_Primary,-1,0);
						} else {
							InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_PredatorStand,(int)PSSS_Attack_Primary,-1,0);
						}
					}
					break;
				}
			}
			break;
		}
		default:
		{
			LOCALASSERT(1==0);
			break;
		}
	}
	/* update the ghost's current sequence*/
	ghostData->currentAnimSequence = sequence;
}



/* Patrick 5/8/97 --------------------------------------------------
Handles the player's weapon sound effects... this should be called
after update ghost/create ghost has been called for player ghost
in processnetmsg_playerstate().
---------------------------------------------------------------------*/
void HandlePlayerGhostWeaponSound(STRATEGYBLOCK *sbPtr, int weapon, int firingPrimary, int firingSecondary)
{
	NETGHOSTDATABLOCK *ghostData;
	AVP_BEHAVIOUR_TYPE type;

	LOCALASSERT(sbPtr);
	LOCALASSERT(sbPtr->DynPtr);		
	LOCALASSERT(weapon>=0);

	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);
	type = ghostData->type;

	switch((enum WEAPON_ID)weapon)
	{
		case(NULL_WEAPON):
		{
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
			break;
		}
		case(WEAPON_CUDGEL):
		{
			//no sound for the moment.
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
			break;
		}
		case(WEAPON_PULSERIFLE):
		{
			LOCALASSERT(type==I_BehaviourMarinePlayer);

			if(firingPrimary)
			{
				if (ghostData->SoundHandle == SOUND_NOACTIVEINDEX)
				{
					unsigned int rand = 1;//FastRandom()%3;
					switch(rand)
					{
						case (0):
         				{
          					Sound_Play(SID_PULSE_START,"ehpd",&ghostData->SoundHandle,(FastRandom()&255)-128, &(sbPtr->DynPtr->Position));
          					break;
         				}
		    			case (1):
			 			{
          					Sound_Play(SID_PULSE_LOOP,"ehpd",&ghostData->SoundHandle,(FastRandom()&255)-128, &(sbPtr->DynPtr->Position));					
          					break;
         				}
		       			case (2):
			 			{
          					Sound_Play(SID_PULSE_END,"ehpd",&ghostData->SoundHandle,(FastRandom()&255)-128, &(sbPtr->DynPtr->Position));					
          					break;
         				}
			 			default:
				 		{
				 			break;
				 		}
					}
				} 
				else Sound_Update3d(ghostData->SoundHandle, &(sbPtr->DynPtr->Position));
			} 
			else if (firingSecondary) {
				Sound_Play(SID_NADEFIRE,"ehd",&ghostData->SoundHandle,&(sbPtr->DynPtr->Position));			
			}
			else
			{
				if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);			
			}				

			break;
		}
		case(WEAPON_TWO_PISTOLS):
		case(WEAPON_MARINE_PISTOL):
		{
			LOCALASSERT(type==I_BehaviourMarinePlayer);
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) {
				Sound_Stop(ghostData->SoundHandle);
			}
			if ((firingPrimary)||(firingSecondary)) {
				//ReleasePrintDebuggingText("Ghost Gunfire Sound!\n");
				Sound_Play(SID_SHOTGUN,"hd",&(sbPtr->DynPtr->Position));
			}
			break;
		}
		case(WEAPON_AUTOSHOTGUN):
		{
			LOCALASSERT(type==I_BehaviourMarinePlayer);
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
			if(firingPrimary) Sound_Play(SID_MINIGUN_LOOP,"ehd",&ghostData->SoundHandle,&(sbPtr->DynPtr->Position));						
			break;
		}
		case(WEAPON_SMARTGUN):
		{
			LOCALASSERT(type==I_BehaviourMarinePlayer);
			if(firingPrimary)
			{
				if(ghostData->SoundHandle == SOUND_NOACTIVEINDEX)
				{
    				unsigned int rand= 2;//FastRandom() % 3;
    				switch (rand)
    				{
      					case(0):
      					{
        					Sound_Play(SID_SMART1,"ehpd",&(ghostData->SoundHandle),((FastRandom()&255)-128),&(sbPtr->DynPtr->Position));					
        					break;
      					}
      					case(1):
      					{
        					Sound_Play(SID_SMART2,"ehpd",&(ghostData->SoundHandle),((FastRandom()&255)-128),&(sbPtr->DynPtr->Position));					
        					break;
      					}
      					case(2):
      					{
        					Sound_Play(SID_SMART3,"ehpd",&(ghostData->SoundHandle),((FastRandom()&255)-128),&(sbPtr->DynPtr->Position));					
        					break;
      					}
						default:
						{
							break;
						}
					}
			   	}
				else Sound_Update3d(ghostData->SoundHandle, &(sbPtr->DynPtr->Position));
			}
			else
			{
				if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);			
			}				
			break;
		}
		case(WEAPON_FLAMETHROWER):
		{
			LOCALASSERT(type==I_BehaviourMarinePlayer);
			if(firingPrimary)
			{
				if(ghostData->SoundHandle == SOUND_NOACTIVEINDEX)
				   	Sound_Play(SID_INCIN_LOOP,"elhd",&(ghostData->SoundHandle),&(sbPtr->DynPtr->Position));					
				else
					Sound_Update3d(ghostData->SoundHandle, &(sbPtr->DynPtr->Position));
			}
			else
			{
				if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			}
			if (firingPrimary)
			{
				if (sbPtr->SBdptr)
				{
					ProveHModel(&ghostData->HModelController,sbPtr->SBdptr);
					FireNetGhostFlameThrower
					(
						&ghostData->GunflashSectionPtr->World_Offset,
						&ghostData->GunflashSectionPtr->SecMat
					);
					/* Lighting? */
					if (sbPtr->SBdptr) {
						AddLightingEffectToObject(sbPtr->SBdptr,LFX_OBJECTONFIRE);
					}
				}
			}
			break;
		}
		case(WEAPON_PLASMAGUN):
		{
			LOCALASSERT(type==I_BehaviourMarinePlayer);
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
			if(firingPrimary) Sound_Play(SID_MINIGUN_EMPTY,"ehd",&ghostData->SoundHandle,&(sbPtr->DynPtr->Position));
			break;
		}
		case(WEAPON_SADAR):
		{
			LOCALASSERT(type==I_BehaviourMarinePlayer);
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			if(firingPrimary) Sound_Play(SID_SADAR_FIRE,"hd",&(sbPtr->DynPtr->Position));
			break;
		}
		case(WEAPON_GRENADELAUNCHER):
		{
			LOCALASSERT(type==I_BehaviourMarinePlayer);
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			if(firingPrimary) Sound_Play(SID_ROCKFIRE,"ehd",&ghostData->SoundHandle,&(sbPtr->DynPtr->Position));						
			break;
		}
		case(WEAPON_MINIGUN):
		{
			break;
		}
		case(WEAPON_FRISBEE_LAUNCHER):
		{
			LOCALASSERT(type==I_BehaviourMarinePlayer);
			/* stop sound if we've got it */
			if (firingPrimary) {
				if (ghostData->SoundHandle!=SOUND_NOACTIVEINDEX) {
					if (ActiveSounds[ghostData->SoundHandle].soundIndex!=SID_ED_SKEETERCHARGE) {
						Sound_Stop(ghostData->SoundHandle);
					}
				}
				if (ghostData->SoundHandle==SOUND_NOACTIVEINDEX) {
					if (ghostData->soundStartFlag==0) {
						Sound_Play(SID_ED_SKEETERCHARGE,"ehd",&ghostData->SoundHandle,&(sbPtr->DynPtr->Position));
						ghostData->soundStartFlag = 1;
					}
				}
			} else {
				ghostData->soundStartFlag = 0;
				if (ghostData->SoundHandle != SOUND_NOACTIVEINDEX) {
					Sound_Stop(ghostData->SoundHandle);
				}
			}
			break;
		}
		case(WEAPON_PRED_WRISTBLADE):
		case(WEAPON_PRED_STAFF):
		{
			LOCALASSERT(type==I_BehaviourPredatorPlayer);
			/* stop sound if we've got it */
			//use the sounds connected to the hierarchy
			#if 0
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			if((firingPrimary)&&(ghostData->SoundHandle == SOUND_NOACTIVEINDEX)) 
				Sound_Play(SID_SWIPE,"ehd",&(ghostData->SoundHandle),&(sbPtr->DynPtr->Position));
			#endif
			break;
		}
		case(WEAPON_SONICCANNON):
		{
			LOCALASSERT(type==I_BehaviourPredatorPlayer);
			#if 0
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			//sound is played by spear creation
			#else
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
			if(firingPrimary) Sound_Play(SID_STOMP,"hd",&(sbPtr->DynPtr->Position));
			#endif
			break;
		}
		case(WEAPON_BEAMCANNON):
		{
			LOCALASSERT(type==I_BehaviourPredatorPlayer);
			#if 0
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			//sound is played by spear creation
			#else
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
			if(firingPrimary) Sound_Play(SID_FRAG_RICOCHETS,"hd",&(sbPtr->DynPtr->Position));
			#endif
			break;
		}
		case(WEAPON_MYSTERYGUN):
		{
			LOCALASSERT(type==I_BehaviourPredatorPlayer);
			#if 0
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			//sound is played by spear creation
			#else
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
			if(firingPrimary) Sound_Play(SID_GRAPPLE_HIT_WALL,"hd",&(sbPtr->DynPtr->Position));
			#endif
			break;
		}
		case(WEAPON_PRED_PISTOL):
		{
			LOCALASSERT(type==I_BehaviourPredatorPlayer);
			if (firingPrimary)
			{
				if(ghostData->SoundHandle == SOUND_NOACTIVEINDEX) 
				{					
					Sound_Play(SID_PRED_PISTOL,"hd",&(sbPtr->DynPtr->Position));									
				}
			}
			break;
		}
		case(WEAPON_PRED_RIFLE):
		{
			LOCALASSERT(type==I_BehaviourPredatorPlayer);
			#if 0
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			//sound is played by spear creation
			#else
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
			if(firingPrimary) Sound_Play(SID_PRED_LASER,"hd",&(sbPtr->DynPtr->Position));
			#endif
			break;
		}
		case(WEAPON_PRED_SHOULDERCANNON):
		{
			LOCALASSERT(type==I_BehaviourPredatorPlayer);
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			//sound is caused by energy bolt creation		   	
			break;
		}
		case(WEAPON_PRED_DISC):
		{
			LOCALASSERT(type==I_BehaviourPredatorPlayer);
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			/* currently no sound for this */
			break;
		}
		case(WEAPON_ALIEN_CLAW):
		{
			//appropriate sounds should be triggered by the animation
			#if 0
			LOCALASSERT(type==I_BehaviourAlienPlayer);
			/* stop sound if we've got it */
			//if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				

			if((firingPrimary)&&(ghostData->SoundHandle == SOUND_NOACTIVEINDEX)) 
				Sound_Play(SID_SWIPE,"ehd",&(ghostData->SoundHandle),&(sbPtr->DynPtr->Position));
			#endif

			break;
		}
		case(WEAPON_ALIEN_GRAB):
		{
			//appropriate sounds should be triggered by the animation
			#if 0
			LOCALASSERT(type==I_BehaviourAlienPlayer);
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			if((firingPrimary)&&(ghostData->SoundHandle == SOUND_NOACTIVEINDEX)) 
				Sound_Play(SID_SWISH,"ehd",&(ghostData->SoundHandle),&(sbPtr->DynPtr->Position));
			#endif
			break;
		}
		case(WEAPON_ALIEN_SPIT):
		{
			LOCALASSERT(type==I_BehaviourAlienPlayer);
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			if(firingPrimary) Sound_Play(SID_PRED_NEWROAR,"hd",&(sbPtr->DynPtr->Position));						
			break;
		}
		case(WEAPON_PRED_MEDICOMP):
		{
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);				
			break;
		}
		default:
		{
			LOCALASSERT(1==0);
			/* stop sound if we've got it */
			if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
			break;
		}
	}
}

int ExtractTimerFromElevation(int elevation) {
	
	int fake_timer,angle1;
	
	if (elevation > 1024) elevation-=4096;
	angle1=-elevation;
	
	GLOBALASSERT(angle1>=-1024);
	GLOBALASSERT(angle1<=1024);
	
	fake_timer=1024-angle1;
	fake_timer<<=5;
	if (fake_timer==65536) fake_timer=65535;
	
	GLOBALASSERT(fake_timer>=0);
	GLOBALASSERT(fake_timer<65536);

	return(fake_timer);
}

/* KJL 17:11:43 26/01/98 - weapon elevation */
void HandleWeaponElevation(STRATEGYBLOCK *sbPtr, int elevation, int weapon)
{
	NETGHOSTDATABLOCK *ghostDataPtr;
	DELTA_CONTROLLER *elevation_controller;
	
	LOCALASSERT(sbPtr);
	ghostDataPtr = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostDataPtr);


	LOCALASSERT(elevation>=0);
	LOCALASSERT(elevation<4096);

	if (ghostDataPtr->type == I_BehaviourMarinePlayer)
	{
		if (!ghostDataPtr->modelUpdated)
		{
			Dispel_HModel(&ghostDataPtr->HModelController);
			CreateMarineHModel(ghostDataPtr,weapon);
			ProveHModel_Far(&ghostDataPtr->HModelController,sbPtr);
			ghostDataPtr->modelUpdated = 1;
		}
		if (weapon != ghostDataPtr->CurrentWeapon)
		{
			Dispel_HModel(&ghostDataPtr->HModelController);
			CreateMarineHModel(ghostDataPtr, weapon);
			ProveHModel_Far(&ghostDataPtr->HModelController,sbPtr);
		}
	
		elevation_controller=Get_Delta_Sequence(&ghostDataPtr->HModelController,"Elevation");
		/* Deal with elevation sequence. */
		GLOBALASSERT(elevation_controller);
		elevation_controller->Active=1;
		switch(ghostDataPtr->currentAnimSequence) {
			case MSQ_StandDieFront:
			case MSQ_StandDieBack:
			case MSQ_StandDeadFront:
			case MSQ_StandDeadBack:
			case MSQ_CrouchDie:
			case MSQ_CrouchDead:
			case MSQ_Taunt:
				{
					/* Force no elevation. */
					elevation_controller->Active=0;
					break;
				}
			case MSQ_RunningFire:
			case MSQ_RunningFire_Backwards:
				if (ghostDataPtr->CurrentWeapon==WEAPON_GRENADELAUNCHER) {
					elevation_controller->Active=0;
					ghostDataPtr->HModelController.Looped=0;
					break;
				} else if (ghostDataPtr->CurrentWeapon==WEAPON_TWO_PISTOLS) {
					if (ghostDataPtr->HModelController.Sub_Sequence==MRSS_Fire_From_Hips) {
						elevation_controller->sequence_type=HMSQT_MarineStand;
						elevation_controller->sub_sequence=(int)MSSS_Hip_Fire_Elevation;
					} else {
						if (HModelSequence_Exists(&ghostDataPtr->HModelController,
							HMSQT_MarineRun,(int)MRSS_Elevation)) {
							elevation_controller->sequence_type=HMSQT_MarineRun;
							elevation_controller->sub_sequence=(int)MRSS_Elevation;
						} else {
							elevation_controller->sequence_type=HMSQT_MarineStand;
							elevation_controller->sub_sequence=(int)MSSS_Elevation;
						}
					}
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
				} else {
					if (HModelSequence_Exists(&ghostDataPtr->HModelController,
						HMSQT_MarineRun,(int)MRSS_Elevation)) {
						elevation_controller->sequence_type=HMSQT_MarineRun;
						elevation_controller->sub_sequence=(int)MRSS_Elevation;
					} else {
						elevation_controller->sequence_type=HMSQT_MarineStand;
						elevation_controller->sub_sequence=(int)MSSS_Elevation;
					}
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
					break;
				}
			case MSQ_StandingFire:
				if (ghostDataPtr->CurrentWeapon==WEAPON_GRENADELAUNCHER) {
					elevation_controller->Active=0;
					ghostDataPtr->HModelController.Looped=0;
					break;
				} else if (ghostDataPtr->CurrentWeapon==WEAPON_TWO_PISTOLS) {
					if (ghostDataPtr->HModelController.Sub_Sequence==MSSS_FireFromHips) {
						elevation_controller->sequence_type=HMSQT_MarineStand;
						elevation_controller->sub_sequence=(int)MSSS_Hip_Fire_Elevation;
					} else {
						elevation_controller->sequence_type=HMSQT_MarineStand;
						elevation_controller->sub_sequence=(int)MSSS_Elevation;
					}
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
				} else {
					elevation_controller->sequence_type=HMSQT_MarineStand;
					elevation_controller->sub_sequence=(int)MSSS_Elevation;
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
					break;
				}
			case MSQ_Walk:
			case MSQ_Walk_Backwards:
				{
					if (ghostDataPtr->CurrentWeapon==WEAPON_TWO_PISTOLS) {
						if (ghostDataPtr->HModelController.Sub_Sequence==MRSS_Fire_From_Hips) {
							elevation_controller->sequence_type=HMSQT_MarineStand;
							elevation_controller->sub_sequence=(int)MSSS_Hip_Fire_Elevation;
						} else {
							if (HModelSequence_Exists(&ghostDataPtr->HModelController,
								HMSQT_MarineRun,(int)MRSS_Elevation)) {
								elevation_controller->sequence_type=HMSQT_MarineRun;
								elevation_controller->sub_sequence=(int)MRSS_Elevation;
							} else {
								elevation_controller->sequence_type=HMSQT_MarineStand;
								elevation_controller->sub_sequence=(int)MSSS_Elevation;
							}
						}
						elevation_controller->timer=ExtractTimerFromElevation(elevation);
					} else {
						if (HModelSequence_Exists(&ghostDataPtr->HModelController,
							HMSQT_MarineRun,(int)MRSS_Elevation)) {
							elevation_controller->sequence_type=HMSQT_MarineRun;
							elevation_controller->sub_sequence=(int)MRSS_Elevation;
						} else {
							elevation_controller->sequence_type=HMSQT_MarineStand;
							elevation_controller->sub_sequence=(int)MSSS_Elevation;
						}
						elevation_controller->timer=ExtractTimerFromElevation(elevation);
					}
					break;
				}
			case MSQ_Crawl:
			case MSQ_Crawl_Backwards:
				{
					if (ghostDataPtr->CurrentWeapon==WEAPON_TWO_PISTOLS) {
						if (ghostDataPtr->HModelController.Sub_Sequence==MCSS_FireFromHips) {
							elevation_controller->sequence_type=HMSQT_MarineCrouch;
							elevation_controller->sub_sequence=(int)MCrSS_Hip_Fire_Elevation;
						} else {
							if (HModelSequence_Exists(&ghostDataPtr->HModelController,
								HMSQT_MarineCrawl,(int)MCSS_Elevation)) {
								elevation_controller->sequence_type=HMSQT_MarineCrawl;
								elevation_controller->sub_sequence=(int)MCSS_Elevation;
							} else {
								elevation_controller->sequence_type=HMSQT_MarineCrouch;
								elevation_controller->sub_sequence=(int)MCrSS_Elevation;
							}
						}
					} else {
						if (HModelSequence_Exists(&ghostDataPtr->HModelController,
							HMSQT_MarineCrawl,(int)MCSS_Elevation)) {
							elevation_controller->sequence_type=HMSQT_MarineCrawl;
							elevation_controller->sub_sequence=(int)MCSS_Elevation;
						} else {
							elevation_controller->sequence_type=HMSQT_MarineCrouch;
							elevation_controller->sub_sequence=(int)MCrSS_Elevation;
						}
					}
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
					break;
				}
			case MSQ_Crouch:
				{
					if (ghostDataPtr->CurrentWeapon==WEAPON_TWO_PISTOLS) {
						if (ghostDataPtr->HModelController.Sub_Sequence==MCrSS_FireFromHips) {
							elevation_controller->sequence_type=HMSQT_MarineCrouch;
							elevation_controller->sub_sequence=(int)MCrSS_Hip_Fire_Elevation;
						} else {
							elevation_controller->sequence_type=HMSQT_MarineCrouch;
							elevation_controller->sub_sequence=(int)MCrSS_Elevation;
						}
					} else {
						elevation_controller->sequence_type=HMSQT_MarineCrouch;
						elevation_controller->sub_sequence=(int)MCrSS_Elevation;
					}
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
					break;
				}
			case MSQ_Stand:
			case MSQ_Jump:
			default:
				{
					if (ghostDataPtr->CurrentWeapon==WEAPON_TWO_PISTOLS) {
						if (ghostDataPtr->HModelController.Sub_Sequence==MSSS_FireFromHips) {
							elevation_controller->sequence_type=HMSQT_MarineStand;
							elevation_controller->sub_sequence=(int)MSSS_Hip_Fire_Elevation;
						} else {
							elevation_controller->sequence_type=HMSQT_MarineStand;
							elevation_controller->sub_sequence=(int)MSSS_Elevation;
						}
					} else {
						elevation_controller->sequence_type=HMSQT_MarineStand;
						elevation_controller->sub_sequence=(int)MSSS_Elevation;
					}
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
				}
				break;
		}
		if ((HModelSequence_Exists(&ghostDataPtr->HModelController,elevation_controller->sequence_type,elevation_controller->sub_sequence))
			&& (elevation_controller->Active)) {
			elevation_controller->Active=1;
			textprint("Using elevation delta.\n");
		} else {
			elevation_controller->Active=0;
		}
	}
	else if (ghostDataPtr->type == I_BehaviourPredatorPlayer)
	{
		/* Self-Destruct test */
		if (ghostDataPtr->ArmorType)
		{
			Dispel_HModel(&ghostDataPtr->HModelController);
			CreatePredatorHModel(ghostDataPtr, 666);
			ProveHModel_Far(&ghostDataPtr->HModelController,sbPtr);
		}
		else
		if (weapon != ghostDataPtr->CurrentWeapon)
		{
			Dispel_HModel(&ghostDataPtr->HModelController);
			CreatePredatorHModel(ghostDataPtr, weapon);
			ProveHModel_Far(&ghostDataPtr->HModelController,sbPtr);
		}

		elevation_controller=Get_Delta_Sequence(&ghostDataPtr->HModelController,"Elevation");
		/* Deal with elevation sequence. */
		GLOBALASSERT(elevation_controller);
		elevation_controller->Active=1; /* enabled? */
		switch(ghostDataPtr->currentAnimSequence) {
			case PredSQ_Taunt:
			case PredSQ_StandDie:
			case PredSQ_StandDead:
			case PredSQ_CrouchDie:	
			case PredSQ_CrouchDead:
				{
					elevation_controller->Active=0;
					/* Force no elevation. */
					break;
				}
			case PredSQ_Run:
			case PredSQ_Run_Backwards:
			case PredSQ_RunningSwipe:
			case PredSQ_RunningSwipe_Backwards:
				{
					elevation_controller->sequence_type=HMSQT_PredatorStand;
					elevation_controller->sub_sequence=(int)PSSS_Elevation;
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
				}
				break;
			case PredSQ_Crouch:
			case PredSQ_CrouchedSwipe:
				{
					elevation_controller->sequence_type=HMSQT_PredatorCrouch;
					elevation_controller->sub_sequence=(int)PCrSS_Elevation;
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
				}
				break;
			case PredSQ_Crawl:
			case PredSQ_CrawlingSwipe:
			case PredSQ_Crawl_Backwards:
			case PredSQ_CrawlingSwipe_Backwards:
				{
					elevation_controller->sequence_type=HMSQT_PredatorCrouch;
					elevation_controller->sub_sequence=(int)PCrSS_Elevation;
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
				}
				break;
			case PredSQ_Jump:
			case PredSQ_Stand:
			case PredSQ_StandingSwipe:
			default:
				{
					elevation_controller->sequence_type=HMSQT_PredatorStand;
					elevation_controller->sub_sequence=(int)PSSS_Elevation;
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
				}
				break;
		}
		if ((HModelSequence_Exists(&ghostDataPtr->HModelController,elevation_controller->sequence_type,elevation_controller->sub_sequence))
			&& (elevation_controller->Active)) {
			elevation_controller->Active=1;
		} else {
			elevation_controller->Active=0;
		}
	}
	else if (ghostDataPtr->type == I_BehaviourAlienPlayer)
	{
		if ((ghostDataPtr->Class == CLASS_EXF_SNIPER) && (!ghostDataPtr->modelUpdated))
		{
			Dispel_HModel(&ghostDataPtr->HModelController);
			CreateAlienHModel(ghostDataPtr,AT_Predalien);
			ProveHModel_Far(&ghostDataPtr->HModelController,sbPtr);
			ghostDataPtr->modelUpdated = 1;
		}

		if ((ghostDataPtr->Class == CLASS_EXF_W_SPEC) && (!ghostDataPtr->modelUpdated))
		{
			Dispel_HModel(&ghostDataPtr->HModelController);
			CreateAlienHModel(ghostDataPtr,AT_Praetorian);
			ProveHModel_Far(&ghostDataPtr->HModelController,sbPtr);
			ghostDataPtr->modelUpdated = 1;
		}

		if ((ghostDataPtr->Class == CLASS_CHESTBURSTER) && (!ghostDataPtr->modelUpdated))
		{
			Dispel_HModel(&ghostDataPtr->HModelController);
			CreateAlienHModel(ghostDataPtr,AT_Praetorian);
			ProveHModel_Far(&ghostDataPtr->HModelController,sbPtr);
			ghostDataPtr->modelUpdated = 1;
		}

		if ((ghostDataPtr->Class == CLASS_MEDIC_PR) && (!ghostDataPtr->modelUpdated))
		{
			Dispel_HModel(&ghostDataPtr->HModelController);
			CreateAlienHModel(ghostDataPtr,3);
			ProveHModel_Far(&ghostDataPtr->HModelController,sbPtr);
			ghostDataPtr->modelUpdated = 1;
		}

		if (ghostDataPtr->CurrentWeapon==-1)
		{
			//this only happens for aliens , if a player changes character
			Dispel_HModel(&ghostDataPtr->HModelController);
			CreateAlienHModel(ghostDataPtr,0);
			ProveHModel_Far(&ghostDataPtr->HModelController,sbPtr);
		}
		
		elevation_controller=Get_Delta_Sequence(&ghostDataPtr->HModelController,"Elevation");
		/* Deal with elevation sequence. */
		GLOBALASSERT(elevation_controller);
		elevation_controller->Active=1; /* enabled? */
		switch(ghostDataPtr->currentAnimSequence) {
			case ASQ_Pain:
			case ASQ_Jump:
			case ASQ_Eat:
			case ASQ_Pounce:
			case ASQ_JumpingTailPoise:
			case ASQ_JumpingTailStrike:
			case ASQ_Taunt:
				{
					elevation_controller->Active=0;
					/* Force no elevation. */
					break;
				}
			case ASQ_Crouch:
			case ASQ_CrouchedAttack_Claw:
			case ASQ_CrouchedTailPoise:
			case ASQ_CrouchedTailStrike:
			case ASQ_Crawl:
			case ASQ_CrawlingAttack_Claw:
			case ASQ_CrawlingTailPoise:
			case ASQ_CrawlingTailStrike:
			case ASQ_Crawl_Backwards:
			case ASQ_CrawlingTailPoise_Backwards:
			case ASQ_CrawlingTailStrike_Backwards:
			case ASQ_CrawlingAttack_Claw_Backwards:
			case ASQ_Scamper:
			case ASQ_Scamper_Backwards:
			case ASQ_CrouchEat :
				{
					elevation_controller->sequence_type=HMSQT_AlienCrouch;
					elevation_controller->sub_sequence=(int)ACrSS_Standard_Elevation;
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
					break;
				}
			case ASQ_Run:
			case ASQ_RunningAttack_Claw:
			case ASQ_RunningTailPoise:
			case ASQ_RunningTailStrike:
			case ASQ_Run_Backwards:
			case ASQ_RunningAttack_Claw_Backwards:
			case ASQ_RunningTailPoise_Backwards:
			case ASQ_RunningTailStrike_Backwards:
			case ASQ_Stand:
			case ASQ_StandingAttack_Claw:
			case ASQ_StandingTailPoise:
			case ASQ_StandingTailStrike:
				{
					elevation_controller->sequence_type=HMSQT_AlienStand;
					elevation_controller->sub_sequence=(int)ASSS_Standard_Elevation;
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
					break;
				}
			default:
				{
					elevation_controller->sequence_type=HMSQT_AlienStand;
					elevation_controller->sub_sequence=(int)ASSS_Standard_Elevation;
					elevation_controller->timer=ExtractTimerFromElevation(elevation);
				}
				break;
		}
		if ((HModelSequence_Exists(&ghostDataPtr->HModelController,elevation_controller->sequence_type,elevation_controller->sub_sequence))
			&& (elevation_controller->Active)) {
			elevation_controller->Active=1;
		} else {
			elevation_controller->Active=0;
		}
	}


}


/* Patrick 15/7/97 --------------------------------------------------
Manages the ghost's integrities
---------------------------------------------------------------------*/
void MaintainGhosts(void)
{
	extern int NumActiveStBlocks;
	extern STRATEGYBLOCK *ActiveStBlockList[];	

	int sbIndex = 0;
	STRATEGYBLOCK *sbPtr;

	/* only do this when we're playing */
	if(netGameData.myGameState!=NGS_Playing) return;

	while(sbIndex < NumActiveStBlocks)
	{	
		sbPtr = ActiveStBlockList[sbIndex++];
		if(sbPtr->I_SBtype==I_BehaviourNetGhost)
		{
			NETGHOSTDATABLOCK *ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
			LOCALASSERT(ghostData);			
			ghostData->integrity-=NormalFrameTime;
			if(ghostData->integrity<0) {
				RemoveGhost(sbPtr);
			}
			else switch (ghostData->type)
			{
				case I_BehaviourFlareGrenade:
				{
					if (ghostData->timer>ONE_FIXED*4)
					{
						ghostData->EventCounter += NormalFrameTime;
					}
					else
					{
						ghostData->EventCounter += MUL_FIXED(NormalFrameTime,ghostData->timer)/4;
					}
			   		
					while (ghostData->EventCounter >= FLARE_PARTICLE_GENERATION_TIME)
					{
						ghostData->EventCounter -= FLARE_PARTICLE_GENERATION_TIME;
						//MakeFlareParticle(sbPtr->DynPtr);
					}

					/* add lighting effect if near */
					if (sbPtr->SBdptr)
					{
						LIGHTBLOCK *lightPtr = sbPtr->SBdptr->ObLights[0];
						LOCALASSERT(sbPtr->SBdptr->ObNumLights==1);
						LOCALASSERT(lightPtr);
						lightPtr->LightBright = 1+MUL_FIXED
												(
													(ONE_FIXED*4-(FastRandom()&32767)),
													ghostData->timer/FLARE_LIFETIME
												);
					}
			   		ghostData->timer -= NormalFrameTime;
					
					//Stop flare from disappearing from lack of update messages.
					//This is so the number of meessages sent can be reduced when the flare has stopped moving.
					ghostData->integrity=ghostData->timer;
					
					break;		
				}
				case I_BehaviourProximityGrenade:
				{
					DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;
					LOCALASSERT(dynPtr);
					if (ghostData->timer<=20*ONE_FIXED)
					{
						{
							int scale = ONE_FIXED-ghostData->timer/PROX_GRENADE_LIFETIME;
							scale = MUL_FIXED(scale,scale);
							scale = MUL_FIXED(scale,scale)*8;
							ghostData->EventCounter += NormalFrameTime + MUL_FIXED(NormalFrameTime,scale);
				   		}
						//sbPtr->SBDamageBlock.IsOnFire=1;
						while (ghostData->EventCounter >= PROX_GRENADE_SOUND_GENERATION_TIME)
						{
							ghostData->EventCounter -= PROX_GRENADE_SOUND_GENERATION_TIME;
							//Sound_Play(SID_PROX_GRENADE_ACTIVE,"d",&(dynPtr->Position));
						}

				   		ghostData->timer -= NormalFrameTime;
					   	if (ghostData->timer<=PROX_GRENADE_TRIGGER_TIME && ghostData->SoundHandle==SOUND_NOACTIVEINDEX)
						{
							//Sound_Play(SID_PROX_GRENADE_READYTOBLOW,"de",&(dynPtr->Position),&ghostData->SoundHandle);
						}
						
						//Stop flare from disappearing from lack of update messages.
						//This is so the number of meessages sent can be reduced when the flare has stopped moving.
					}
					else
					{
						if(!(dynPtr->Position.vx!=dynPtr->PrevPosition.vx ||
						     dynPtr->Position.vy!=dynPtr->PrevPosition.vy ||
						     dynPtr->Position.vz!=dynPtr->PrevPosition.vz))
						{
							ghostData->timer = 20*ONE_FIXED;		
						}
					}
					{
						// scan for objects in proximity
						extern int NumActiveStBlocks;
						extern STRATEGYBLOCK *ActiveStBlockList[];	
						int i = NumActiveStBlocks;
						while(i--)
						{						  						
							STRATEGYBLOCK *obstaclePtr = ActiveStBlockList[i];
							DYNAMICSBLOCK *obstacleDynPtr = obstaclePtr->DynPtr;
							
							if (obstacleDynPtr)
							{
								if(ValidTargetForProxMine(obstaclePtr))
								{
									VECTORCH disp = obstacleDynPtr->Position;
									disp.vx -= dynPtr->Position.vx;
									disp.vy -= dynPtr->Position.vy;
									disp.vz -= dynPtr->Position.vz;
									
									if (Approximate3dMagnitude(&disp)<=PROX_GRENADE_RANGE)
									{	
										if(ghostData->SoundHandle  != SOUND_NOACTIVEINDEX)
										{
											if (ActiveSounds[ghostData->SoundHandle].soundIndex!=SID_PROX_GRENADE_READYTOBLOW)
											{
												Sound_Stop(ghostData->SoundHandle);
											}
											else
											{
												break;
											}
										}
										//Sound_Play(SID_PROX_GRENADE_READYTOBLOW,"de",&(dynPtr->Position),&ghostData->SoundHandle);
										break;
									}
								}
							}
						}
					}

					ghostData->integrity=ghostData->timer;
					break;
				}

				// SHOULDER LAMP IN MULTI				
				case (I_BehaviourMarinePlayer):				
				{
					// Create lighting effect if player is using shoulder lamp
					if ( ghostData->IAmUsingShoulderLamp )
					{
						DISPLAYBLOCK *dispPtr = sbPtr->SBdptr;

						if ( dispPtr )
						{
							// Add light to ghost as an 'object light'
							LIGHTBLOCK	*ShoulderLamp = AddLightEffectToObjReturnReference (dispPtr, 
																							LFX_SHOULDER_LAMP);
							if (   ( ShoulderLamp )
								&& ( sbPtr->DynPtr ))
							{								
								// Position net ghost lamp - ShoulderLampOffset is relative to DynPtr->Position
								ShoulderLamp->LightWorld.vx = sbPtr->DynPtr->Position.vx + ghostData->ShoulderLampOffset.vx;
								ShoulderLamp->LightWorld.vy = sbPtr->DynPtr->Position.vy + ghostData->ShoulderLampOffset.vy;
								ShoulderLamp->LightWorld.vz = sbPtr->DynPtr->Position.vz + ghostData->ShoulderLampOffset.vz;
							}
						}
					}
					break;
				}
				// END OF SHOULDER LAMP IN MULTI

				default:
					break;				
			}
		}
	}	
}

/* Patrick 13/10/97 --------------------------------------------------
Added for autogun ghost support
---------------------------------------------------------------------*/
#define AGUNGHOST_FIRINGPOINT_INFRONT	500
#define AGUNGHOST_FIRINGPOINT_ACROSS	0
#define AGUNGHOST_FIRINGPOINT_UP		200

void HandleGhostAutoGunMuzzleFlash(STRATEGYBLOCK *sbPtr, int firing)
{
	NETGHOSTDATABLOCK *ghostData;

	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);
	LOCALASSERT(ghostData->type == I_BehaviourAutoGun);

	if(ghostData->myGunFlash)
	{
		/* I've already got a gun flash... */
		if(firing)
		{
			/* Maintain existing gun flash */
			VECTORCH position;
			EULER orientation;
			MATRIXCH mat;
			CalculatePosnForGhostAutoGunMuzzleFlash(sbPtr, &position, &orientation);
			
			CreateEulerMatrix(&orientation, &mat);
			TransposeMatrixCH(&mat);	

			MaintainNPCGunFlashEffect(ghostData->myGunFlash, &position, &mat);
		}
		else
		{
			RemoveNPCGunFlashEffect(ghostData->myGunFlash);
			ghostData->myGunFlash = NULL;
		} 
	}
	else
	{
		if(firing)
		{
  			/* Need a new gun flash */
			VECTORCH position;
			EULER orientation;
			MATRIXCH mat;
			CalculatePosnForGhostAutoGunMuzzleFlash(sbPtr, &position, &orientation);
			CreateEulerMatrix(&orientation, &mat);
			TransposeMatrixCH(&mat);	
			ghostData->myGunFlash = AddNPCGunFlashEffect(&position, &mat, SFX_MUZZLE_FLASH_AMORPHOUS);
		}
	}
}

void HandleGhostAutoGunSound(STRATEGYBLOCK *sbPtr, int firing)
{
	NETGHOSTDATABLOCK *ghostData;
	AVP_BEHAVIOUR_TYPE type;

	LOCALASSERT(sbPtr);
	LOCALASSERT(sbPtr->DynPtr);		

	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);
	type = ghostData->type;
	LOCALASSERT(type==I_BehaviourAutoGun);

	if(firing)
	{
		if(ghostData->SoundHandle == SOUND_NOACTIVEINDEX)
			Sound_Play(SID_SENTRY_GUN,"eld",&(ghostData->SoundHandle),&(sbPtr->DynPtr->Position));					
		/* don't need sound update, as autoguns don't move */
	}
	else if(ghostData->SoundHandle != SOUND_NOACTIVEINDEX) 
	{	
		Sound_Stop(ghostData->SoundHandle);				
		Sound_Play(SID_SENTRY_END,"hd",&(sbPtr->DynPtr->Position));					
	}
}

static void CalculatePosnForGhostAutoGunMuzzleFlash(STRATEGYBLOCK *sbPtr,VECTORCH *position, EULER *orientation)
{
	VECTORCH upNormal = {0,-65536,0};
	VECTORCH inFrontVec = {0,0,0};
	VECTORCH toSideVec = {0,0,0};
	EULER firingOrient = {0,0,0};

	/* orientation is easy */
	firingOrient.EulerY = sbPtr->DynPtr->OrientEuler.EulerY;
	*orientation = firingOrient;

	/* vector in front */
	inFrontVec.vx = GetSin(firingOrient.EulerY);
	inFrontVec.vz = GetCos(firingOrient.EulerY);
	Normalise(&inFrontVec);

	/* vectors in front, to side, and up */
	CrossProduct(&inFrontVec,&upNormal,&toSideVec);
	LOCALASSERT(toSideVec.vy==0);
	inFrontVec.vx = MUL_FIXED(inFrontVec.vx,AGUNGHOST_FIRINGPOINT_INFRONT);
	inFrontVec.vz = MUL_FIXED(inFrontVec.vz,AGUNGHOST_FIRINGPOINT_INFRONT);
	toSideVec.vx = MUL_FIXED(toSideVec.vx,AGUNGHOST_FIRINGPOINT_ACROSS);
	toSideVec.vz = MUL_FIXED(toSideVec.vz,AGUNGHOST_FIRINGPOINT_ACROSS);

	position->vx = sbPtr->DynPtr->Position.vx + inFrontVec.vx +	toSideVec.vx;
	position->vy = sbPtr->DynPtr->Position.vy - AGUNGHOST_FIRINGPOINT_UP;
	position->vz = sbPtr->DynPtr->Position.vz + inFrontVec.vz +	toSideVec.vz;
}

/* patrick 14/10/97 : added to support cloaked predators in net game */
void MaintainGhostCloakingStatus(STRATEGYBLOCK *sbPtr, int IsCloaked)
{
	NETGHOSTDATABLOCK *ghostData;
	AVP_BEHAVIOUR_TYPE type;

	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);
	type = ghostData->type;

	if(type!=I_BehaviourPredatorPlayer && type!=I_BehaviourAlienPlayer) 
	{
		ghostData->CloakingEffectiveness = 0;		
		return;
	}

	/* Handle sound playing. */
	if (ghostData->CloakingEffectiveness) {
		if (IsCloaked==0) {
			/* Cloak turns off. */
			if (type == I_BehaviourPredatorPlayer)
				Sound_Play(SID_PRED_CLOAKOFF,"hd",&(sbPtr->DynPtr->Position));
		}
	} else {
		if (IsCloaked) {
			/* Cloak turns on. */
			if (type == I_BehaviourPredatorPlayer)
				Sound_Play(SID_PRED_CLOAKON,"hd",&(sbPtr->DynPtr->Position));
		}
	}

	ghostData->CloakingEffectiveness = IsCloaked;
}


/* KJL 14:18:32 27/01/98 - object behaviour for network ghosts */
extern void NetGhostBehaviour(STRATEGYBLOCK *sbPtr)
{
	NETGHOSTDATABLOCK *ghostDataPtr;

    LOCALASSERT(sbPtr);
	
	ghostDataPtr = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostDataPtr);

	switch(ghostDataPtr->type)
	{
		case I_BehaviourNetCorpse:
		{
			/* A copy of the fn. for real ones, minus existance. */
			DISPLAYBLOCK *dispPtr = sbPtr->SBdptr;
	
			/* do we have a displayblock? */
			if (dispPtr)
			{
				dispPtr->SpecialFXFlags |= SFXFLAG_MELTINGINTOGROUND;
				dispPtr->ObFlags2 = ghostDataPtr->timer/2;
	
			}

			/* Does the corpse that falls when not visible make no sound? */
			ProveHModel_Far(&ghostDataPtr->HModelController,sbPtr);
			ghostDataPtr->timer-=NormalFrameTime;
			break;
		}
		default:
			break;
	}

	if (sbPtr->SBdptr) {
		if (sbPtr->SBdptr->HModelControlBlock) {

			/* Fire sound code. */
			if(ghostDataPtr->SoundHandle3!=SOUND_NOACTIVEINDEX) Sound_Update3d(ghostDataPtr->SoundHandle3,&(sbPtr->DynPtr->Position));
			if(ghostDataPtr->SoundHandle4!=SOUND_NOACTIVEINDEX) Sound_Update3d(ghostDataPtr->SoundHandle4,&(sbPtr->DynPtr->Position));
		
			if (sbPtr->SBDamageBlock.IsOnFire) {
				if (ghostDataPtr->IgnitionHandshaking==0) {
					AddNetMsg_LocalObjectOnFire(sbPtr);
				}
				if (ghostDataPtr->SoundHandle3!=SOUND_NOACTIVEINDEX) {
					if (ActiveSounds[ghostDataPtr->SoundHandle3].soundIndex!=SID_FIRE) {
						Sound_Stop(ghostDataPtr->SoundHandle3);
					 	Sound_Play(SID_FIRE,"dlev",&(sbPtr->DynPtr->Position),&ghostDataPtr->SoundHandle3,127);
					}
				} else {
				 	Sound_Play(SID_FIRE,"dlev",&(sbPtr->DynPtr->Position),&ghostDataPtr->SoundHandle3,127);
				}
			} else {
				if (ghostDataPtr->SoundHandle3!=SOUND_NOACTIVEINDEX) {
					Sound_Stop(ghostDataPtr->SoundHandle3);
				}
			}
		}
	}

	if(ghostDataPtr->FlameHitCount)
	{
		//send a damage message for all the flame particles that have hit
		CauseDamageToObject(sbPtr,&TemplateAmmo[AMMO_FLAMETHROWER].MaxDamage[AvP.Difficulty], (ONE_FIXED/400)*ghostDataPtr->FlameHitCount,NULL);
		ghostDataPtr->FlameHitCount=0;

	}
	if(ghostDataPtr->FlechetteHitCount)
	{
		//send a damage message for all the flechette particles that have hit
		extern DAMAGE_PROFILE FlechetteDamage;
		CauseDamageToObject(sbPtr,&FlechetteDamage,ONE_FIXED*ghostDataPtr->FlechetteHitCount,NULL);
		ghostDataPtr->FlechetteHitCount=0;

	}
}

void MaintainGhostFireStatus(STRATEGYBLOCK *sbPtr, int IsOnFire)
{
	NETGHOSTDATABLOCK *ghostData;

	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);
	
	if (IsOnFire) {
		if (ghostData->IgnitionHandshaking==0) {
			/* Recieved confirmation. */
			ghostData->IgnitionHandshaking=1;
			if (sbPtr->SBDamageBlock.IsOnFire==0) {
				/* Set alight by something else? */
				sbPtr->SBDamageBlock.IsOnFire=1;
			}
		} else {
			/* Corroberated - do nothing. */
		}
	} else {
		if (ghostData->IgnitionHandshaking==0) {
			/* Corroberated - do nothing. */
		} else {
			/* Fire gone out? */
			ghostData->IgnitionHandshaking=0;
			sbPtr->SBDamageBlock.IsOnFire=0;
		}
	}
}

void Engage_Template_Death(STRATEGYBLOCK *sbPtr,SECTION *root,HMODELCONTROLLER *controller,int type, int subtype,
	int length, int tweening) {

	RemoveAllDeltas(controller);
	Transmogrify_HModels(sbPtr,controller, root, 1, 0,0);
	InitHModelTweening(controller,tweening,type,subtype,length,0);

}

void Create_Marine_Standing_Template_Death(STRATEGYBLOCK *sbPtr,HMODELCONTROLLER *controller,int type, int subtype,
	int length, int tweening) {

	/* Standing template. */
	SECTION *root;
	
	/* This has gotta be considered weird. */
	InitHModelSequence(controller,(int)HMSQT_MarineStand,
		(int)MSSS_Dies_Standard,(ONE_FIXED<<1));
	ProveHModel_Far(controller,sbPtr);
	/* And now we change it.  See? */
	
	root=GetNamedHierarchyFromLibrary("hnpcmarine","Template");
	
	Engage_Template_Death(sbPtr,root,controller,type,subtype,length,tweening);

}

void Create_Predator_Standing_Template_Death(STRATEGYBLOCK *sbPtr,HMODELCONTROLLER *controller,int type, int subtype,
	int length, int tweening) {

	/* Standing template. */
	SECTION *root;
	
	/* This has gotta be considered weird. */
	InitHModelSequence(controller,(int)HMSQT_PredatorStand,
		(int)PSSS_Dies_Standard,(ONE_FIXED<<1));
	ProveHModel_Far(controller,sbPtr);
	/* And now we change it.  See? */
	
	root=GetNamedHierarchyFromLibrary("hnpcpredator","Template");

	Engage_Template_Death(sbPtr,root,controller,type,subtype,length,tweening);
}

/* Kills a ghost, to leave a corpse */
void KillGhost(STRATEGYBLOCK *sbPtr,int objectId)
{
	NETGHOSTDATABLOCK *ghostData;
	
	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);
	
	/* this is where we add fragmentation and explosion	effects to destroyed ghosts */
	switch(ghostData->type)
	{
		case(I_BehaviourAlienPlayer):
		case(I_BehaviourMarinePlayer):
		case(I_BehaviourPredatorPlayer):	
		{
			/* Drop through, for the moment. */
			#if EXTRAPOLATION_TEST
			sbPtr->DynPtr->LinImpulse.vx=0;
			sbPtr->DynPtr->LinImpulse.vy=0;
			sbPtr->DynPtr->LinImpulse.vz=0;

		   	sbPtr->DynPtr->LinVelocity.vx=0;
			sbPtr->DynPtr->LinVelocity.vy=0;
			sbPtr->DynPtr->LinVelocity.vz=0;

			sbPtr->DynPtr->UseStandardGravity=1;
			
			sbPtr->DynPtr->IsNetGhost=1;
			
			sbPtr->DynPtr->ToppleForce=TOPPLE_FORCE_NONE;
			
			#endif
			break;
		}
		case(I_BehaviourGrenade):
		case(I_BehaviourRocket):
		case(I_BehaviourThrownSpear):
		case(I_BehaviourProximityGrenade):
		case(I_BehaviourFragmentationGrenade):
		case(I_BehaviourClusterGrenade):
		case(I_BehaviourPulseGrenade):
		case(I_BehaviourNPCPredatorDisc):
		case(I_BehaviourPredatorDisc_SeekTrack):
		case(I_BehaviourAlienSpit):
		case(I_BehaviourInanimateObject):
		default:
		{
			/* Default to something else. */
			RemoveGhost(sbPtr);
			return;
			break;
		}
	}

	/* see if we've got a sound... */
	if(ghostData->SoundHandle  != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
	if(ghostData->SoundHandle2 != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle2);
	if(ghostData->SoundHandle3 != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle3);
	if(ghostData->SoundHandle4 != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle4);

	/* see if we've got a muzzle flash... */
	if(ghostData->myGunFlash)
	{
		RemoveNPCGunFlashEffect(ghostData->myGunFlash);
		ghostData->myGunFlash = NULL;
	}

	/* Now.. you're a ghost.  Of a corpse. */
	
/*--------------------------------------------------------------------**
** 	The animation sequence we should use will come in a later message **
**--------------------------------------------------------------------*/
		
	ghostData->subtype=ghostData->type;
	ghostData->type=I_BehaviourNetCorpse;
	ghostData->IOType=IOT_Non;
	ghostData->playerObjectId=objectId;
	ghostData->timer=CORPSE_EXPIRY_TIME; /* Arbitrarily */
	
	if (ghostData->HModelController.Deltas) {
		RemoveAllDeltas(&ghostData->HModelController);
	}

	sbPtr->DynPtr->OnlyCollideWithEnvironment = 1;
	
	//allow the corpse to fall to the floor
	sbPtr->DynPtr->GravityOn=1;

	/* And the final touch. */
//	KillRandomSections(ghostData->HModelController.section_data,(ONE_FIXED>>2));
}

extern void ApplyGhostCorpseDeathAnim(STRATEGYBLOCK *sbPtr,int deathId)
{
	extern DEATH_DATA Marine_Deaths[];
	extern DEATH_DATA Alien_Deaths[];
	extern DEATH_DATA Predator_Deaths[];
	DEATH_DATA* this_death;
	
	NETGHOSTDATABLOCK *ghostData;
	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);

	if(ghostData->type!=I_BehaviourNetCorpse) return;
	
	switch(ghostData->subtype)
	{
		case I_BehaviourMarinePlayer :
		{
			this_death=GetThisDeath_FromCode(&ghostData->HModelController,&Marine_Deaths[0],deathId);
			if (this_death->Template) 
			{
				SECTION* template_root=GetNamedHierarchyFromLibrary("hnpcmarine","Template");
				LOCALASSERT(template_root);
				/* Convert to template. */
				Transmogrify_HModels(sbPtr,&ghostData->HModelController,
					template_root, 0, 0,0);
			}
			else
			{
				SECTION* template_root=GetNamedHierarchyFromLibrary("hnpcmarine","Template");
				LOCALASSERT(template_root);
				/* Convert to template. */
				TrimToTemplate(sbPtr,&ghostData->HModelController,
					template_root, 0);
			}
		}
		break;
		
		case I_BehaviourPredatorPlayer :
		{
			this_death=GetThisDeath_FromCode(&ghostData->HModelController,&Predator_Deaths[0],deathId);
			if (this_death->Template) 
			{
				SECTION* template_root=GetNamedHierarchyFromLibrary("hnpcpredator","Template");
				LOCALASSERT(template_root);
				/* Convert to template. */
				Transmogrify_HModels(sbPtr,&ghostData->HModelController,
					template_root, 1, 0,0);
			}
		}
		break;
		
		case I_BehaviourAlienPlayer :
		{
			// Apply another death anim for player facehuggers.
			if ((ghostData->Class == CLASS_EXF_W_SPEC) ||
				(ghostData->Class == CLASS_CHESTBURSTER))
			{
				InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_Hugger,(int)HSS_Dies,ONE_FIXED,0);
				return;
			} else if (ghostData->Class == CLASS_MEDIC_PR) {
				InitHModelTweening(&ghostData->HModelController,(ONE_FIXED>>3),(int)HMSQT_QueenGeneral,(int)QGSS_FaceDeath,ONE_FIXED,0);
				return;
			} else
				this_death=GetThisDeath_FromCode(&ghostData->HModelController,&Alien_Deaths[0],deathId);
		}
		break;
		
		default :
			return;

	}


	if(this_death->TweeningTime<=0)
	{
		InitHModelSequence(&ghostData->HModelController,this_death->Sequence_Type,this_death->Sub_Sequence,this_death->Sequence_Length);
	}
	else
	{
		InitHModelTweening(&ghostData->HModelController, this_death->TweeningTime,this_death->Sequence_Type,this_death->Sub_Sequence,this_death->Sequence_Length,0);
	}

	/* Electric death sound? */
	if (this_death->Electrical) {
		Sound_Play(SID_ED_ELEC_DEATH,"de",&sbPtr->DynPtr->Position,&ghostData->SoundHandle4);
	}
	
}

extern void ApplyCorpseDeathAnim(STRATEGYBLOCK *sbPtr,int deathId)
{
	extern DEATH_DATA Marine_Deaths[];
	extern DEATH_DATA Alien_Deaths[];
	extern DEATH_DATA Predator_Deaths[];
	DEATH_DATA* this_death;
	
	NETCORPSEDATABLOCK *corpseDataPtr=(NETCORPSEDATABLOCK *)sbPtr->SBdataptr;
	
	switch(corpseDataPtr->Type)
	{
		case I_BehaviourMarinePlayer :
		{
			this_death=GetThisDeath_FromCode(&corpseDataPtr->HModelController,&Marine_Deaths[0],deathId);
			if (this_death->Template) 
			{
				SECTION* template_root=GetNamedHierarchyFromLibrary("hnpcmarine","Template");
				LOCALASSERT(template_root);
				/* Convert to template. */
				Transmogrify_HModels(sbPtr,&corpseDataPtr->HModelController,
					template_root, 0, 0,0);
			}
			else
			{
				SECTION* template_root=GetNamedHierarchyFromLibrary("hnpcmarine","Template");
				LOCALASSERT(template_root);
				/* Convert to template. */
				TrimToTemplate(sbPtr,&corpseDataPtr->HModelController,
					template_root, 0);
			}
		}
		break;
		
		case I_BehaviourPredatorPlayer :
		{
			this_death=GetThisDeath_FromCode(&corpseDataPtr->HModelController,&Predator_Deaths[0],deathId);
			if (this_death->Template) 
			{
				SECTION* template_root=GetNamedHierarchyFromLibrary("hnpcpredator","Template");
				LOCALASSERT(template_root);
				/* Convert to template. */
				Transmogrify_HModels(sbPtr,&corpseDataPtr->HModelController,
					template_root, 1, 0,0);
			}
		}
		break;
		
		case I_BehaviourAlienPlayer :
		{
			this_death=GetThisDeath_FromCode(&corpseDataPtr->HModelController,&Alien_Deaths[0],deathId);
		}
		break;
		
		default :
			return;

	}


	if(this_death->TweeningTime<=0)
	{
		InitHModelSequence(&corpseDataPtr->HModelController,this_death->Sequence_Type,this_death->Sub_Sequence,this_death->Sequence_Length);
	}
	else
	{
		InitHModelTweening(&corpseDataPtr->HModelController, this_death->TweeningTime,this_death->Sequence_Type,this_death->Sub_Sequence,this_death->Sequence_Length,0);
	}
	corpseDataPtr->This_Death=this_death;

	/* Electric death sound? */
	if (this_death->Electrical) {
		Sound_Play(SID_ED_ELEC_DEATH,"de",&sbPtr->DynPtr->Position,&corpseDataPtr->SoundHandle4);
	}
	
}

extern void KillAlienAIGhost(STRATEGYBLOCK *sbPtr,int death_code,int death_time,int GibbFactor) {

	NETGHOSTDATABLOCK *ghostData;
	DEATH_DATA *this_death;
	
	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	LOCALASSERT(ghostData);
	
	
	/* To ensure we know what we're doing... */
	if (ghostData->type!=I_BehaviourAlien) {
		GLOBALASSERT(0);
	}

	if(ghostData->onlyValidFar)
	{
		RemoveGhost(sbPtr);
		return;
	}

	this_death=GetThisDeath_FromCode(&ghostData->HModelController,Alien_Deaths,death_code);
	GLOBALASSERT(this_death);

	/* see if we've got a sound... */
	if(ghostData->SoundHandle  != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle);
	if(ghostData->SoundHandle2 != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle2);
	if(ghostData->SoundHandle3 != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle3);
	if(ghostData->SoundHandle4 != SOUND_NOACTIVEINDEX) Sound_Stop(ghostData->SoundHandle4);

	/* see if we've got a muzzle flash... */
	if(ghostData->myGunFlash)
	{
		RemoveNPCGunFlashEffect(ghostData->myGunFlash);
		ghostData->myGunFlash = NULL;
	}
	
	/* Convert that sucker. */
	ghostData->type=I_BehaviourNetCorpse;
	/*
	Alien subtype (alien/predalien/praetorian) gets shuffled down into IOType.
	Not entirely appropriate , but it will do...
	*/
	ghostData->IOType=ghostData->subtype;
	ghostData->subtype=I_BehaviourAlien;
	ghostData->timer=death_time;
	
	if (ghostData->HModelController.Deltas) {
		RemoveAllDeltas(&ghostData->HModelController);
	}
	/* Now let's do the sequence. */
 	UpdateAlienAIGhostAnimSequence(sbPtr,this_death->Sequence_Type,this_death->Sub_Sequence,
 		this_death->Sequence_Length,this_death->TweeningTime);
	ghostData->HModelController.LoopAfterTweening=0;

	//Allow players to walk through the corpse
	sbPtr->DynPtr->OnlyCollideWithEnvironment = 1;
	
	#if EXTRAPOLATION_TEST
	sbPtr->DynPtr->LinImpulse.vx=0;
	sbPtr->DynPtr->LinImpulse.vy=0;
	sbPtr->DynPtr->LinImpulse.vz=0;

	sbPtr->DynPtr->LinVelocity.vx=0;
	sbPtr->DynPtr->LinVelocity.vy=0;
	sbPtr->DynPtr->LinVelocity.vz=0;

	sbPtr->DynPtr->UseStandardGravity=1;
	
	sbPtr->DynPtr->IsNetGhost=1;
	
	sbPtr->DynPtr->ToppleForce=TOPPLE_FORCE_NONE;
	#endif
	//allow the corpse to fall to the floor
	sbPtr->DynPtr->GravityOn=1;

	/* Gibb the corpse? */
	if (GibbFactor) {
		DoAlienLimbLossSound(&sbPtr->DynPtr->Position);
		Extreme_Gibbing(sbPtr,ghostData->HModelController.section_data,GibbFactor);
	}

	/* Electric death sound? */
	if (this_death->Electrical) {
		Sound_Play(SID_ED_ELEC_DEATH,"de",&sbPtr->DynPtr->Position,&ghostData->SoundHandle4);
	}

}

int Deduce_PlayerDeathSequence(void) {

	int a;
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);

	a=((FastRandom()&65535)>>12)&14; /* 0,2,4,6,8,10,12,14 */
	if (playerStatusPtr->ShapeState == PMph_Crouching) {
		return(a);
	} else {
		return(a+1);
	}

}

int Deduce_PlayerMarineDeathSequence(STRATEGYBLOCK* sbPtr,DAMAGE_PROFILE* damage,int multiple,VECTORCH* incoming)
{
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	NETCORPSEDATABLOCK *corpseDataPtr=(NETCORPSEDATABLOCK *)sbPtr->SBdataptr;
	
	int deathtype,gibbFactor;
//	int a;

//	SECTION_DATA *head;

	/* Set GibbFactor  and death type*/
	gibbFactor=0;
	{
		int tkd;
		
		tkd=TotalKineticDamage(damage);
		deathtype=0;

		if (damage->ExplosivePower==1) {
			if (MUL_FIXED(tkd,(multiple&((ONE_FIXED<<1)-1)))>20) {
				/* Okay, you can gibb now. */
				gibbFactor=ONE_FIXED>>1;
				deathtype=2;
			}
		} else if ((tkd>60)&&((multiple>>16)>1)) {
			int newmult;

			newmult=DIV_FIXED(multiple,NormalFrameTime);
			if (MUL_FIXED(tkd,newmult)>(500)) {
				/* Loadsabullets! */
				gibbFactor=-(ONE_FIXED>>2);
				deathtype=2;
			}
		}

		if ((damage->ExplosivePower==2)||(damage->ExplosivePower==6)) {
			/* Basically SADARS only. */
			gibbFactor=ONE_FIXED;
			deathtype=3;
		}
	}

	if (damage->ForceBoom) {
		deathtype+=damage->ForceBoom;
	}

	{
		SECTION_DATA *chest;
		
		chest=GetThisSectionData(corpseDataPtr->HModelController.section_data,"chest");
		
		if (chest==NULL) {
			/* I'm impressed. */
			deathtype+=2;
		} else if ((chest->flags&section_data_notreal)
			&&(chest->flags&section_data_terminate_here)) {
			/* That's gotta hurt. */
			deathtype++;
		}
	}


	/* Now final stage. */
	{
		DEATH_DATA *this_death;
		HIT_FACING facing;
		SECTION *root;
		int burning,electrical;
		int crouched;

		root=GetNamedHierarchyFromLibrary("hnpcmarine","Template");
		
		facing.Front=0;
		facing.Back=0;
		facing.Left=0;
		facing.Right=0;
		
		if (incoming) {
			if (incoming->vz>0) {
				facing.Back=1;
			} else {
				facing.Front=1;
			}
			if (incoming->vx>0) {
				facing.Right=1;
			} else {
				facing.Left=1;
			}
		}
		
		if ((playerStatusPtr->fireTimer>0)
			&&(damage->Impact==0) 		
			&&(damage->Cutting==0)  	
			&&(damage->Penetrative==0)
			&&(damage->Fire>0)
			&&(damage->Electrical==0)
			&&(damage->Acid==0)
			) {
			burning=1;
		} else {
			burning=0;
		}

		if ((damage->Impact==0) 		
			&&(damage->Cutting==0)  	
			&&(damage->Penetrative==0)
			&&(damage->Fire==0)
			&&(damage->Electrical>0)
			&&(damage->Acid==0)
			) {
			electrical=1;
		} else {
			electrical=0;
		}

		if (playerStatusPtr->ShapeState == PMph_Crouching) 
		{
			crouched=1;
		} 
		else 
		{
			crouched=0;
		}
		
		this_death=GetMarineDeathSequence(&corpseDataPtr->HModelController,root,corpseDataPtr->Wounds,corpseDataPtr->Wounds,
			deathtype,&facing,burning,crouched,electrical);
		
		GLOBALASSERT(this_death);

		return this_death->Multiplayer_Code;
	}
}

int Deduce_PlayerAlienDeathSequence(STRATEGYBLOCK* sbPtr,DAMAGE_PROFILE* damage,int multiple,VECTORCH* incoming)
{
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	NETCORPSEDATABLOCK *corpseDataPtr=(NETCORPSEDATABLOCK *)sbPtr->SBdataptr;
	
	int tkd,deathtype;
	
	/* Get death type. */
	tkd=TotalKineticDamage(damage);
	deathtype=0;

	if (damage->ExplosivePower==1) {
	 	/* Explosion case. */
	 	if (MUL_FIXED(tkd,(multiple&((ONE_FIXED<<1)-1)))>20) {
	 		/* Okay, you can gibb now. */
			deathtype=2;
	 	}
	} else if ((tkd<40)&&((multiple>>16)>1)) {
	 	int newmult;

	 	newmult=DIV_FIXED(multiple,NormalFrameTime);
	 	if (MUL_FIXED(tkd,newmult)>700) {
	 		/* Excessive bullets case 1. */
			deathtype=2;
	 	} else if (MUL_FIXED(tkd,newmult)>250) {
	 		/* Excessive bullets case 2. */
			deathtype=1;
	 	}
	}
	
	if ((damage->ExplosivePower==2)||(damage->ExplosivePower==6)) {
		/* Basically SADARS only. */
		deathtype=3;
	}

	if (damage->ForceBoom) {
		deathtype+=damage->ForceBoom;
	}
	
	{
		SECTION_DATA *chest=GetThisSectionData(corpseDataPtr->HModelController.section_data,"chest");
		
		if (chest==NULL) {
			/* I'm impressed. */
			deathtype+=2;
		} else if ((chest->flags&section_data_notreal)
			&&(chest->flags&section_data_terminate_here)) {
			/* That's gotta hurt. */
			deathtype++;
		}
	}

	{
	

		DEATH_DATA *this_death;
		HIT_FACING facing;
		int crouched;

		facing.Front=0;
		facing.Back=0;
		facing.Left=0;
		facing.Right=0;

		if (incoming) {
			if (incoming->vz>0) {
				facing.Back=1;
			} else {
				facing.Front=1;
			}
			if (incoming->vx>0) {
				facing.Right=1;
			} else {
				facing.Left=1;
			}
		}
		if (playerStatusPtr->ShapeState == PMph_Crouching) 
		{
			crouched=1;
		} 
		else 
		{
			crouched=0;
		}

		this_death=GetAlienDeathSequence(&corpseDataPtr->HModelController,NULL,corpseDataPtr->Wounds,corpseDataPtr->Wounds,
			deathtype,&facing,0,crouched,0);

		return this_death->Multiplayer_Code;
	}
	
}

int Deduce_PlayerPredatorDeathSequence(STRATEGYBLOCK* sbPtr,DAMAGE_PROFILE* damage,int multiple,VECTORCH* incoming)
{
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	NETCORPSEDATABLOCK *corpseDataPtr=(NETCORPSEDATABLOCK *)sbPtr->SBdataptr;

    int deathtype=0;
    int tkd = TotalKineticDamage(damage);
	
    if (damage->ExplosivePower==1) {
    	if (MUL_FIXED(tkd,(multiple&((ONE_FIXED<<1)-1)))>20) {
        	/* Okay, you can... splat now. */
        	deathtype=2;
        }
    } else if ((tkd<40)&&((multiple>>16)>1)) {
    	int newmult;

    	newmult=DIV_FIXED(multiple,NormalFrameTime);
    	if (MUL_FIXED(tkd,newmult)>(500)) {
    	    deathtype=2;
        }
    }

    if ((damage->ExplosivePower==2)||(damage->ExplosivePower==6)) {
    	/* Basically SADARS only. */
    	deathtype=3;
    }

    if (damage->ForceBoom) {
    	deathtype+=damage->ForceBoom;
    }

    {
    	SECTION_DATA *chest;
    
    	chest=GetThisSectionData(corpseDataPtr->HModelController.section_data,"chest");
    
    	if (chest==NULL) {
    		/* I'm impressed. */
    		deathtype+=2;
    	} else if ((chest->flags&section_data_notreal)
    		&&(chest->flags&section_data_terminate_here)) {
    		/* That's gotta hurt. */
    		deathtype++;
    	}
    }
	
	{
		DEATH_DATA *this_death;
		HIT_FACING facing;
		SECTION *root;
		int burning;
//		int wounds;
		int crouched;

		root=GetNamedHierarchyFromLibrary("hnpcpredator","Template");
	
		facing.Front=0;
		facing.Back=0;
		facing.Left=0;
		facing.Right=0;
	
		if (incoming) {
			if (incoming->vz>0) {
				facing.Back=1;
			} else {
				facing.Front=1;
			}
			if (incoming->vx>0) {
				facing.Right=1;
			} else {
				facing.Left=1;
			}
		}
	
		if ((damage->Impact==0)                 
			&&(damage->Cutting==0)          
			&&(damage->Penetrative==0)
			&&(damage->Fire>0)
			&&(damage->Electrical==0)
			&&(damage->Acid==0)
			) {
			burning=1;
		} else {
			burning=0;
		}

		if (playerStatusPtr->ShapeState == PMph_Crouching) 
		{
			crouched=1;
		} 
		else 
		{
			crouched=0;
		}
	
		this_death=GetPredatorDeathSequence(&corpseDataPtr->HModelController,root,corpseDataPtr->Wounds,
		      corpseDataPtr->Wounds,deathtype,&facing,burning,crouched,0);
	
		GLOBALASSERT(this_death);
	

		return this_death->Multiplayer_Code;
	}
	
}

STRATEGYBLOCK *MakeNewCorpse()
{
	int i;
	STRATEGYBLOCK *sbPtr;
	SECTION *root_section;
	signed char weapon;
	NETCORPSEDATABLOCK *corpseData; 
	PLAYER_WEAPON_DATA *weaponPtr;
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	unsigned int Class;

	LOCALASSERT(playerStatusPtr);    	        
   	weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
	weapon = (signed char)(weaponPtr->WeaponIDNumber);
	Class = playerStatusPtr->Class;

	/* create a strategy block - same as for a ghost */	
	sbPtr = CreateActiveStrategyBlock();
	if(!sbPtr) 
	{
		/* allocation failed */
		return NULL;
	}
	InitialiseSBValues(sbPtr);
	sbPtr->I_SBtype = I_BehaviourNetCorpse;
	
	for(i = 0; i < SB_NAME_LENGTH; i++) sbPtr->SBname[i] = '\0';	
	AssignNewSBName(sbPtr);

	/* dynamics block */
	{
		DYNAMICSBLOCK *dynPtr;

		/* need different templates for objects and sprites */
		dynPtr = AllocateDynamicsBlock(DYNAMICS_TEMPLATE_MARINE_PLAYER);
		if(!dynPtr) 
		{
			/* allocation failed */
			RemoveBehaviourStrategy(sbPtr);
			return NULL;
		}
		dynPtr->CanClimbStairs = 0;
		dynPtr->IgnoreThePlayer = 1;

		sbPtr->DynPtr = dynPtr;
		/* zero linear velocity in dynamics block */
		dynPtr->LinVelocity.vx = dynPtr->LinVelocity.vy = dynPtr->LinVelocity.vz = 0;
		dynPtr->LinImpulse.vx = dynPtr->LinImpulse.vy = dynPtr->LinImpulse.vz = 0;
		dynPtr->GravityOn = 1;
		dynPtr->UseStandardGravity = 1;
		
		dynPtr->Position = dynPtr->PrevPosition = Player->ObStrategyBlock->DynPtr->Position;
		dynPtr->OrientEuler = Player->ObStrategyBlock->DynPtr->OrientEuler;
		CreateEulerMatrix(&dynPtr->OrientEuler, &dynPtr->OrientMat);
		TransposeMatrixCH(&dynPtr->OrientMat);

	}

	sbPtr->maintainVisibility = 1;
	sbPtr->containingModule = ModuleFromPosition(&(sbPtr->DynPtr->Position), 0);

	/* data block */
	{
		corpseData = AllocateMem(sizeof(NETCORPSEDATABLOCK)); 
		if(!corpseData) 
		{
			/* allocation failed */
			RemoveBehaviourStrategy(sbPtr);
			return NULL;
		}		
		sbPtr->SBdataptr = (void *)corpseData;
		corpseData->SoundHandle  = SOUND_NOACTIVEINDEX;
		corpseData->SoundHandle2 = SOUND_NOACTIVEINDEX;
		corpseData->SoundHandle3 = SOUND_NOACTIVEINDEX;
		corpseData->SoundHandle4 = SOUND_NOACTIVEINDEX;

		switch(AvP.PlayerType)
		{
			case I_Marine :
				corpseData->Type=I_BehaviourMarinePlayer;
				break;
			case I_Alien :
				corpseData->Type=I_BehaviourAlienPlayer;
				break;
			case I_Predator :
				corpseData->Type=I_BehaviourPredatorPlayer;
				break;
		}
		corpseData->GibbFactor=0;
		corpseData->This_Death=NULL;
		
		/* Clear cloak data... */
		corpseData->CloakStatus = PCLOAK_Off;
		corpseData->CloakTimer = 0;
		corpseData->destructTimer = -1;
		/* Clear deathfiring stuff... */
		corpseData->WeaponMisfireFunction=NULL;
		corpseData->My_Gunflash_Section=NULL;
		corpseData->weapon_variable=0;
		corpseData->Android=0;
		corpseData->TemplateRoot=NULL;
		corpseData->DeathFiring=0;
		corpseData->hltable=NULL;

		corpseData->Wounds=0;

 		/* set the shape */
		switch(AvP.PlayerType)
		{
			case(I_Marine):
			{
				
				corpseData->hltable=GetThisHitLocationTable("marine with pulse rifle");
				/* Select hierarchy from character's selected weapon. */
				//while we're at it , also deal with creating the pickupable weapon
				//
				// Do NOT create a weapon pickup... -- ELDRITCH
			   	if (0)
				{
			   		VECTORCH location=Player->ObStrategyBlock->DynPtr->Position;
			   		STRATEGYBLOCK* weaponSbPtr = CreateMultiplayerWeaponPickup(&location,weapon,0);
					if(weaponSbPtr)
					{
						//hide the newly created weapon from this player , until the player respawns
						weaponSbPtr->maintainVisibility=0;
					}
				}
				switch (weapon)
				{
					default:
					case WEAPON_PULSERIFLE:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcmarine","marine with pulse rifle");
						break;
					}
					case WEAPON_TWO_PISTOLS:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcmarine","Two Pistol");
						break;
					}
					case WEAPON_MARINE_PISTOL:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcmarine","PISTOL");
						break;
					}
					case WEAPON_FLAMETHROWER:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcmarine","marine with flame thrower");
						break;
					}
					case WEAPON_SMARTGUN:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcmarine","marine with smart gun");
						break;
					}
					case WEAPON_MINIGUN:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcmarine","Marine with Mini Gun");
						break;
					}
					case WEAPON_SADAR:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcmarine","marine with SADAR");
						break;
					}
					case WEAPON_GRENADELAUNCHER:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcmarine","marine + grenade launcher");
						break;
					}
					case WEAPON_CUDGEL:
					case WEAPON_AUTOSHOTGUN:
					case WEAPON_PLASMAGUN:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcmarine","Cudgel");
						break;
					}
					case WEAPON_FRISBEE_LAUNCHER:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcmarine","skeeter");
						break;
					}
				}
				Create_HModel(&corpseData->HModelController,root_section);
				{
					extern DPID AVPDPNetID;
					ChangeGhostMarineAccoutrementSet(&corpseData->HModelController,AVPDPNetID,Class);
				}
				/* Static Model Exchange System */
				if (Class == CLASS_RIFLEMAN)
					ModExSys(&corpseData->HModelController,"chest","riflemanchest");
				else if (Class == CLASS_ENGINEER)
					ModExSys(&corpseData->HModelController,"chest","engineerchest");
				else if (Class == CLASS_COM_TECH)
					ModExSys(&corpseData->HModelController,"chest","comtechchest");
				else if (Class == CLASS_MEDIC_FT)
					ModExSys(&corpseData->HModelController,"chest","medicchest");

				//choose a default sequence , the proper death anim will be set later	
				InitHModelSequence(&corpseData->HModelController,(int)HMSQT_MarineStand,(int)MSSS_Standard,ONE_FIXED);
				
				break;
			}
			case(I_Alien):
			{
				// Extended Multiplayer -- Eldritch
				
				corpseData->hltable=GetThisHitLocationTable("alien");
				root_section = GetNamedHierarchyFromLibrary("hnpcalien","alien");
				
				if (Class == CLASS_EXF_SNIPER)
				{
					root_section=GetNamedHierarchyFromLibrary("hnpcpred_alien","TEMPLATE");
					corpseData->hltable=GetThisHitLocationTable("predalien");
				}
				if (Class == CLASS_MEDIC_PR)
				{
					root_section=GetNamedHierarchyFromLibrary("queen","Template");
					corpseData->hltable=GetThisHitLocationTable("alien");
				}
				if ((Class == CLASS_EXF_W_SPEC) || (Class == CLASS_CHESTBURSTER))
				{
					root_section=GetHierarchyFromLibrary("hnpchugger");
					corpseData->hltable=GetThisHitLocationTable("alien");
				}
				// -- End

				Create_HModel(&corpseData->HModelController,root_section);

	if (Class == CLASS_CHESTBURSTER)
	{
		SECTION_DATA *part;

		part=GetThisSectionData(corpseData->HModelController.section_data,"l00 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l01 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l02 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l03 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l04 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l05 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l06 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l07 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l08 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l09 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l10 tail");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"l11 tail");
		part->flags|=(section_data_notreal);

		part=GetThisSectionData(corpseData->HModelController.section_data,"left a1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left a2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left a3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left b1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left b2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left b3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left c1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left c2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left c3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left d1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left d2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"left d3 finger");
		part->flags|=(section_data_notreal);

		part=GetThisSectionData(corpseData->HModelController.section_data,"right a1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right a2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right a3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right b1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right b2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right b3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right c1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right c2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right c3 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right d1 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right d2 finger");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right d3 finger");
		part->flags|=(section_data_notreal);

		part=GetThisSectionData(corpseData->HModelController.section_data,"left air sack");
		part->flags|=(section_data_notreal);
		part=GetThisSectionData(corpseData->HModelController.section_data,"right air sack");
		part->flags|=(section_data_notreal);

		ModExSys(&corpseData->HModelController,"body","burster");
	}
				
				if ((Class != CLASS_EXF_SNIPER) && 
					(Class != CLASS_EXF_W_SPEC) &&
					(Class != CLASS_MEDIC_PR) &&
					(Class != CLASS_CHESTBURSTER))
					SetAlternateAlienSkin(&corpseData->HModelController,Class);

				//choose a default sequence , the proper death anim will be set later	
				InitHModelSequence(&corpseData->HModelController,(int)HMSQT_AlienStand,(int)ASSS_Standard,ONE_FIXED);
				break;
			}
			case(I_Predator):
			{
				corpseData->hltable=GetThisHitLocationTable("predator");
				switch (weapon)
				{
					default:
					case WEAPON_PRED_WRISTBLADE:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred with wristblade");
						break;
					}
					case WEAPON_PRED_RIFLE:
					{
						// Extended Multiplayer -- Eldritch
						root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred with staff");
						// -- End
						break;
					}
					case WEAPON_SONICCANNON:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcpredator","Speargun");
						break;
					}
					case WEAPON_BEAMCANNON:
					case WEAPON_MYSTERYGUN:
					case WEAPON_PRED_PISTOL:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred + pistol");
						break;
					}
					case WEAPON_PRED_SHOULDERCANNON:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred with Plasma Caster");
						break;
					}
					case WEAPON_PRED_DISC:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcpredator","pred with disk");
						break;
					}
					case WEAPON_PRED_MEDICOMP:
					{
						root_section = GetNamedHierarchyFromLibrary("hnpcpredator","medicomp");
						break;
					}
				}
				Create_HModel(&corpseData->HModelController,root_section);

				// Change Skin
				SetAlternateCharacterSkin(&corpseData->HModelController,Class);

				/* Model Exchange System */
#if 0
				if (Class == CLASS_PRED_HUNTER)
				{
					ModExSys(&corpseData->HModelController,"chest","hunterchest");
					ModExSys(&corpseData->HModelController,"Pelvis","hunterpelvis");
				}
#endif
				//choose a default sequence , the proper death anim will be set later	
				InitHModelSequence(&corpseData->HModelController,(int)HMSQT_PredatorStand,(int)PSSS_Elevation,ONE_FIXED);
				break;
			}
		}
	}
	/* Now fix timer. */
	corpseData->timer=CORPSE_EXPIRY_TIME; /* Arbitrarily */
	corpseData->validityTimer=CORPSE_VALIDITY_TIME;
	corpseData->HModelController.Looped=0;
	ProveHModel_Far(&corpseData->HModelController,sbPtr);
	MakeCorpseNear(sbPtr);
	GLOBALASSERT(sbPtr->SBdptr);
	sbPtr->SBdptr->ObFlags|=ObFlag_NotVis;
	/* And the fire. */
	
	if (Player->ObStrategyBlock->SBDamageBlock.IsOnFire) {
		sbPtr->SBDamageBlock.IsOnFire=1;
	}
	/* Defaults to zero. */

	if (playerStatusPtr->soundHandle3!=SOUND_NOACTIVEINDEX) {
		if (ActiveSounds[playerStatusPtr->soundHandle3].soundIndex==SID_FIRE) {
			/* Pass fire sound across. */
			corpseData->SoundHandle3=playerStatusPtr->soundHandle3;
			playerStatusPtr->soundHandle3=SOUND_NOACTIVEINDEX;
		}
	}

	/* And the final touch. */
//	KillRandomSections(corpseData->HModelController.section_data,(ONE_FIXED>>2));

	return(sbPtr);
}

void UpdateAlienAIGhostAnimSequence(STRATEGYBLOCK *sbPtr,HMODEL_SEQUENCE_TYPES type, int subtype, int length, int tweeningtime)
{

	NETGHOSTDATABLOCK *ghostData;

	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;

	GLOBALASSERT(length!=0);

	

	/* Are we already playing this one? */
	if ((ghostData->HModelController.Sequence_Type==type)&&(ghostData->HModelController.Sub_Sequence==subtype)) {
		/* Yes, but... */
		/*I think we only want to change speed if we're not tweening*/
		if (tweeningtime<=0)
		{
			if (length!=ghostData->HModelController.Seconds_For_Sequence) {
				HModel_ChangeSpeed(&ghostData->HModelController,length);
			}
		}

	} else if (tweeningtime<=0) {
		InitHModelSequence(&ghostData->HModelController,(int)type,subtype,length);
	} else {
		InitHModelTweening(&ghostData->HModelController, tweeningtime, (int)type,subtype,length, 1);
	}

	ghostData->HModelController.Playing=1;
	/* Might be unset... */
}

/* This a copy of UpdateGhost, with extra parameters. */
void UpdateAlienAIGhost(STRATEGYBLOCK *sbPtr,VECTORCH *position,EULER *orientation,int sequence_type,int sub_sequence, int sequence_length)
{
	NETGHOSTDATABLOCK *ghostData;

	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;
	
	/* for visibility support: as ghosts can be moved when invisible, we need to work out
	which module they're in	whenever we update them. We must be carefull, however, not
	to set the containingModule to NULL if the object has moved outside the env, as 
	the	visibility system expects that we at least know what module any object WAS in,
	even if we do not now... thus, if we cannot find a containing module, we abort the update */
	
	/* KJL 21:01:09 23/05/98 - I've put this test here because the player's image in a mirror goes
	throught this code, and it's obviously going to be outside the environment */
	if (sbPtr->I_SBtype==I_BehaviourNetGhost)
	{
		MODULE *myContainingModule = ModuleFromPosition(position, (sbPtr->containingModule));
		if(myContainingModule==NULL) return;	
		sbPtr->containingModule = myContainingModule;
	}

	
	if (ghostData->type!=I_BehaviourAlien) {
		GLOBALASSERT(0);
	}

	//We have enough information to be able to make this alien near (should we want to)
	ghostData->onlyValidFar=0;

	/* update the dynamics block */
	{
		DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;

		dynPtr->PrevPosition = dynPtr->Position;
		dynPtr->PrevOrientMat = dynPtr->OrientMat;
		dynPtr->Position = *position;
		dynPtr->OrientEuler = *orientation;
		CreateEulerMatrix(&dynPtr->OrientEuler,&dynPtr->OrientMat);
		TransposeMatrixCH(&dynPtr->OrientMat);
	}	

	/* KJL 16:58:04 17/06/98 - we want to update anims differently for NPCS */
	if ((sequence_type!=-1)&&(sub_sequence!=-1)) {
		/* Not tweening! */
		UpdateAlienAIGhostAnimSequence(sbPtr,sequence_type,sub_sequence,sequence_length,(ONE_FIXED>>2));
	}
		
	/* refresh integrity */
	ghostData->integrity = GHOST_INTEGRITY;		
}

void Convert_DiscGhost_To_PickupGhost(STRATEGYBLOCK *sbPtr) {

	DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;
	NETGHOSTDATABLOCK *ghostData= (NETGHOSTDATABLOCK * ) sbPtr->SBdataptr;
	/* Transmogrify a disc ghost behaviour to a disc ammo pickup! */

	/* Sort out dynamics block */
	dynPtr->LinVelocity.vx=0;
	dynPtr->LinVelocity.vy=0;
	dynPtr->LinVelocity.vz=0;

	dynPtr->LinImpulse.vx=0;
	dynPtr->LinImpulse.vy=0;
	dynPtr->LinImpulse.vz=0;

	ghostData->type=I_BehaviourInanimateObject;
	ghostData->IOType=IOT_Ammo;
	ghostData->subtype=AMMO_PRED_DISC;

	ghostData->HModelController.Playing=0;
	/* Deal with flip up case? */

	/* The final shape is NOT hierarchical. */
	{
		SECTION_DATA *disc_section;
		disc_section=GetThisSectionData(ghostData->HModelController.section_data,"disk");
		GLOBALASSERT(disc_section);

		sbPtr->shapeIndex=disc_section->sempai->ShapeNum;
		Dispel_HModel(&ghostData->HModelController);
		if (sbPtr->SBdptr) {
			sbPtr->SBdptr->ObShape=disc_section->sempai->ShapeNum;
			sbPtr->SBdptr->ObShapeData=disc_section->sempai->Shape;
			sbPtr->SBdptr->HModelControlBlock=NULL;
		}
	}

	if(ghostData->SoundHandle!=SOUND_NOACTIVEINDEX) 
	{
		Sound_Stop(ghostData->SoundHandle);
	}
	Sound_Play(SID_DISC_STICKSINWALL,"dp",&(sbPtr->DynPtr->Position),((FastRandom()&511)-255));
	
	
}



void PlayHitDeltaOnGhost(STRATEGYBLOCK *sbPtr,char delta_seq,char delta_sub_seq)
{
	DELTA_CONTROLLER *hitdelta;
	NETGHOSTDATABLOCK *ghostData;

	LOCALASSERT(sbPtr);
	ghostData = (NETGHOSTDATABLOCK *)sbPtr->SBdataptr;

	//only play hit deltas on marine and predator players
	if(ghostData->type!=I_BehaviourMarinePlayer && ghostData->type!=I_BehaviourPredatorPlayer) return;
	
	hitdelta=Get_Delta_Sequence(&ghostData->HModelController,"HitDelta");

	if(hitdelta)
	{
   		if (HModelSequence_Exists(&ghostData->HModelController,(int)delta_seq,(int)delta_sub_seq)) 
		{
			Start_Delta_Sequence(hitdelta,(int)delta_seq,(int) delta_sub_seq,ONE_FIXED/2); 
			hitdelta->Playing=1;
		}
	}
	
}

void PlayOtherSound(enum soundindex SoundIndex, VECTORCH *position, int explosion) {

	if (explosion) {
		Ghost_Explosion_SoundData.position=*position;
		Sound_Play(SoundIndex,"n",&Ghost_Explosion_SoundData);
	} else {
	    Sound_Play(SoundIndex,"d",position);
	}

}












