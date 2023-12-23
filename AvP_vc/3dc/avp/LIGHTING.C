/*KJL***************************************************
* lighting.c - a lighting interface for simple effects *
***************************************************KJL*/

#include "3dc.h"

#include "inline.h"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"
#include "bh_weap.h"

#include "lighting.h"
#include "particle.h"
#include "dynamics.h"
#define UseLocalAssert Yes
#include "ourasert.h"
#include "weapons.h"

static VECTORCH RotatingLightPosition;
extern int CloakingPhase;
extern int NormalFrameTime;
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern int LightScale;
extern int ChangedMyClass;

extern DPID GrabbedPlayer;

// For Predator Self-Destruct FX.
int SpatialShockwave;

// SHOULDER LAMP IN MULTI - this function is no longer needed
// static LIGHTELEMENT	*FindLightElement(enum LIGHTELEMENT_BEHAVIOUR_ID TargetBehaviourID);
extern void NewOnScreenMessage(unsigned char *messagePtr);
// END OF SHOULDER LAMP IN MULTI

void PositionShoulderLamp(VECTORCH *Position);

void AddLightingEffectToObject(DISPLAYBLOCK *objectPtr, enum LIGHTING_EFFECTS_ID lfxID)
{
	LIGHTBLOCK *lightPtr;
	
	lightPtr = AddLightBlock(objectPtr, NULL);

	if (!lightPtr) return;

	switch(lfxID)
	{
		case LFX_EXPLOSION:
		{
			/* brightness */
			lightPtr->LightBright = ONE_FIXED*4;
			/* flags */
			lightPtr->LightFlags = LFlag_Omni;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = EXPLOSION_LIGHT_RANGE; 

			lightPtr->RedScale=255*256;
			lightPtr->GreenScale=120*256;
			lightPtr->BlueScale=0;
			#if PSX
			lightPtr->LightColour.r=255;
			lightPtr->LightColour.g=120;
			lightPtr->LightColour.b=0;
			#endif

			break;
		}
		case LFX_BIGEXPLOSION:
		{
			/* brightness */
			lightPtr->LightBright = ONE_FIXED << 2;
			/* flags */
			lightPtr->LightFlags = LFlag_Omni;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = EXPLOSION_LIGHT_RANGE; 

			lightPtr->RedScale=255*256;
			lightPtr->GreenScale=120*256;
			lightPtr->BlueScale=0;
			#if PSX
			lightPtr->LightColour.r=255;
			lightPtr->LightColour.g=120;
			lightPtr->LightColour.b=0;
			#endif

			break;
		}

		case LFX_MUZZLEFLASH:
		{
			/* brightness */
			lightPtr->LightBright = 65536 - (FastRandom()&32767);
			/* flags */
			lightPtr->LightFlags = LFlag_Omni|LFlag_Deallocate;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			#if 0
			lightPtr->LightRange = 5000; /* ? */

			lightPtr->RedScale=255*256;
			lightPtr->GreenScale=192*256;
			lightPtr->BlueScale=128*256;
			#else
			lightPtr->LightRange = 10000; /* ? */
			lightPtr->RedScale=255*256;
			lightPtr->GreenScale=230*256;
			lightPtr->BlueScale=200*256;
			#endif
			#if PSX
			lightPtr->LightColour.r=255;
			lightPtr->LightColour.g=192;
			lightPtr->LightColour.b=128;
			#endif

			break;
		}
		case LFX_PARTICLECANNON:
		{
			/* brightness */
			lightPtr->LightBright = 65536 - (FastRandom()&32767);
			/* flags */
			lightPtr->LightFlags = LFlag_Omni|LFlag_Deallocate;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = 10000; /* ? */

			lightPtr->RedScale=0;
			lightPtr->GreenScale=0;
			lightPtr->BlueScale=255*256; 
			#if PSX
			lightPtr->LightColour.r=255;
			lightPtr->LightColour.g=32;
			lightPtr->LightColour.b=0;
			#endif
			break;
		}
		case LFX_ROCKETJET:
		{
			/* brightness */
			lightPtr->LightBright = ONE_FIXED*3/4;
			/* flags */
			lightPtr->LightFlags = LFlag_Omni|LFlag_CosAtten;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = 5000; /* ? */

			lightPtr->RedScale=255*256;
			lightPtr->GreenScale=255*256;
			lightPtr->BlueScale=128*256;
			#if PSX
			lightPtr->LightColour.r=255;
			lightPtr->LightColour.g=255;
			lightPtr->LightColour.b=128;
			#endif

			break;
		}
		case LFX_FLARE:
		{
			/* brightness */
			lightPtr->LightBright = 0;
			/* flags */
			lightPtr->LightFlags = LFlag_Omni;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = 10000; /* ? */

			lightPtr->RedScale=255*256;
			lightPtr->GreenScale=0;
			lightPtr->BlueScale=0;
			#if PSX
			lightPtr->LightColour.r=255;
			lightPtr->LightColour.g=64;
			lightPtr->LightColour.b=255;
			#endif

			break;
		}
		case LFX_XENO_FIRING:
		{
			/* brightness */
			lightPtr->LightBright = 226100;
			/* flags */
			lightPtr->LightFlags = LFlag_Omni|LFlag_Deallocate;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = 5000; /* ? */

			lightPtr->RedScale=255*256;
			lightPtr->GreenScale=64*256;
			lightPtr->BlueScale=255*256;
			#if PSX
			lightPtr->LightColour.r=64;
			lightPtr->LightColour.g=64;
			lightPtr->LightColour.b=255;
			#endif

			break;
		}
		case LFX_PLASMA_BOLT:
		{
			/* brightness */
			lightPtr->LightBright = ONE_FIXED/4;
			/* flags */
			lightPtr->LightFlags = LFlag_Omni|LFlag_CosAtten;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = 10000;
			
			lightPtr->RedScale=200*256;
			lightPtr->GreenScale=255*256;
			lightPtr->BlueScale=255*256;

			break;
		}
		case LFX_OBJECTONFIRE:
		{
			/* brightness */
			lightPtr->LightBright = ONE_FIXED - (FastRandom()&4095);
			/* flags */
			lightPtr->LightFlags = LFlag_Omni|LFlag_Deallocate|LFlag_Thermal;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = 10000;
			lightPtr->RedScale=255*256;
			lightPtr->GreenScale=60*256;
			lightPtr->BlueScale=0;//50*256;

			break;
		}
		case LFX_SPEARGUNBOLT:
		{
			/* Just an experiment. */
			/* brightness */
			lightPtr->LightBright = ONE_FIXED/4;
			/* flags */
			lightPtr->LightFlags = LFlag_Omni|LFlag_CosAtten;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = 10000;
			
			lightPtr->RedScale=255*256;
			lightPtr->GreenScale=255*256;
			lightPtr->BlueScale=255*256;

			break;
		}
	}
}

// SHOULDER LAMP IN MULTI

LIGHTBLOCK *AddLightEffectToObjReturnReference(DISPLAYBLOCK *objectPtr, enum LIGHTING_EFFECTS_ID lfxID)
{
	// Copied from AddLightingEffectToObject, but returns reference to the created light.

	LIGHTBLOCK *lightPtr;
	
	lightPtr = AddLightBlock (objectPtr, NULL);

	if ( !lightPtr ) 
	{
		return (NULL);
	}

	switch (lfxID)
	{		
		case LFX_SHOULDER_LAMP:		
		default:
			lightPtr->LightBright	= ONE_FIXED << 1;	
			lightPtr->LightRange	= 6000;				
			// LFlag_Deallocate - kill at end of frame, recreate next time
			// LFlag_AbsPos - Don't move this light in UpdateObjectLights
			lightPtr->LightFlags	= LFlag_Omni | LFlag_Deallocate | LFlag_AbsPos;
			lightPtr->LightType		= LightType_PerVertex;
			lightPtr->RedScale		= 240*256;		
			lightPtr->GreenScale	= 240*256;		
			lightPtr->BlueScale		= 255*256;			
			break;	
	}

	return (lightPtr);
}

// END OF SHOULDER LAMP IN MULTI

void LightBlockDeallocation(void)
{
	extern int NumActiveBlocks;
	extern DISPLAYBLOCK *ActiveBlockList[];
	int numblocks = NumActiveBlocks;

	while(numblocks)
	{
		DISPLAYBLOCK *dptr = ActiveBlockList[--numblocks];

		int numlights = dptr->ObNumLights;
		while(numlights)
		{
			LIGHTBLOCK *lightPtr = dptr->ObLights[--numlights];
			if(lightPtr->LightFlags & LFlag_Deallocate)
				DeleteLightBlock(lightPtr, dptr);
		}
	}

}

/*KJL******************************
*                                 *
*      LIGHT ELEMENT CODE         *
* 								  *
******************************KJL*/
#define MAX_NO_OF_LIGHTELEMENTS 500
LIGHTELEMENT LightElementStorage[MAX_NO_OF_LIGHTELEMENTS];
int NumActiveLightElements;

void InitialiseLightElementSystem(void)
{
	NumActiveLightElements = 0;
}

static LIGHTELEMENT* AllocateLightElement(void)
{
	LIGHTELEMENT *lightElementPtr = 0; /* Default to null ptr */

	if (NumActiveLightElements != MAX_NO_OF_LIGHTELEMENTS) 
	{
		lightElementPtr = &LightElementStorage[NumActiveLightElements];
		NumActiveLightElements++;
	}
	else
	{
		/* unable to allocate a lightElement */
	}

	return lightElementPtr;
}
static void DeallocateLightElement(LIGHTELEMENT *lightElementPtr)
{
	/* is pointer within array? */
	LOCALASSERT(lightElementPtr>=LightElementStorage);
	LOCALASSERT(lightElementPtr<=&LightElementStorage[MAX_NO_OF_LIGHTELEMENTS-1]);
	
	NumActiveLightElements--;
	*lightElementPtr = LightElementStorage[NumActiveLightElements];
}


void MakeLightElement(VECTORCH *positionPtr, enum LIGHTELEMENT_BEHAVIOUR_ID behaviourID)
{
	LIGHTELEMENT *lightElementPtr = AllocateLightElement();
	LIGHTBLOCK *lightPtr = &(lightElementPtr->LightBlock);

	/* if we failed to make an element, get the hell out of here */
	if (!lightElementPtr) return;

	lightElementPtr->BehaviourID = behaviourID;
	lightElementPtr->LightBlock.LightWorld = *positionPtr;	
	lightElementPtr->LifeTime=ONE_FIXED;

	if (lightElementPtr->BehaviourID == LIGHTELEMENT_ELECTRICAL_EXPLOSION)
		lightElementPtr->LifeTime = ONE_FIXED*3;

	switch (behaviourID)
	{
		case LIGHTELEMENT_ROTATING:
		{
			RotatingLightPosition = *positionPtr;
			break;
		}
		case LIGHTELEMENT_ALIEN_TEETH:
		{
			lightPtr->LightBright = ONE_FIXED/2;
			/* flags */
			lightPtr->LightFlags = LFlag_Omni;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = 200; 

			lightPtr->RedScale=	  ONE_FIXED;
			lightPtr->GreenScale= ONE_FIXED;
			lightPtr->BlueScale=  ONE_FIXED;

			{
				VECTORCH position;
			   	position.vx = MUL_FIXED(200,GetSin((CloakingPhase)&4095));
			   	position.vy = MUL_FIXED(200,GetCos((CloakingPhase)&4095));
			   	position.vz = 80+MUL_FIXED(50,GetCos((CloakingPhase/2)&4095));
			   	{
			   		MATRIXCH myMat = Global_VDB_Ptr->VDB_Mat;
			   		TransposeMatrixCH(&myMat);
			   		RotateVector(&(position), &(myMat));	
			   		position.vx += Global_VDB_Ptr->VDB_World.vx;
			   		position.vy += Global_VDB_Ptr->VDB_World.vy;
			   		position.vz += Global_VDB_Ptr->VDB_World.vz;
			   	}
				lightElementPtr->LightBlock.LightWorld = position;
			}
			lightElementPtr->LifeTime = 0;
			break;						  
		}
		case LIGHTELEMENT_ALIEN_TEETH2:
		{
			lightPtr->LightBright = ONE_FIXED/2;
			/* flags */
			lightPtr->LightFlags = LFlag_Omni;
			/* lightblock light type */
			lightPtr->LightType = LightType_PerVertex;
			/* range */
			lightPtr->LightRange = 200; 

			lightPtr->RedScale=	  ONE_FIXED;
			lightPtr->GreenScale= ONE_FIXED;
			lightPtr->BlueScale=  ONE_FIXED;

			{
				VECTORCH position;
			   	position.vx = MUL_FIXED(200,GetSin((CloakingPhase/3+2048)&4095));
			   	position.vy = MUL_FIXED(200,GetCos((CloakingPhase/3+2048)&4095));
			   	position.vz = 80+MUL_FIXED(50,GetCos((CloakingPhase/2+2048)&4095));
			   	{
			   		MATRIXCH myMat = Global_VDB_Ptr->VDB_Mat;
			   		TransposeMatrixCH(&myMat);
			   		RotateVector(&(position), &(myMat));	
			   		position.vx += Global_VDB_Ptr->VDB_World.vx;
			   		position.vy += Global_VDB_Ptr->VDB_World.vy;
			   		position.vz += Global_VDB_Ptr->VDB_World.vz;
			   	}
				lightElementPtr->LightBlock.LightWorld = position;
			}
			lightElementPtr->LifeTime = 0;
			break;
		}

		// SHOULDER LAMP IN MULTI - this no longer needed as the lamp is now attached to the
		// Players DISPLAY_BLOCK as an 'Object Light'

		/*
		case LIGHTELEMENT_SHOULDER_LAMP:
			lightPtr->LightBright = ONE_FIXED << 1;
			lightPtr->LightRange = 6000;
			lightPtr->LightFlags = LFlag_CosAtten;
			lightPtr->LightType = LightType_Infinite;

			lightPtr->RedScale = ONE_FIXED;
			lightPtr->GreenScale = ONE_FIXED;
			lightPtr->BlueScale = ONE_FIXED >> 1;

			lightElementPtr->LifeTime = 1;
			PositionShoulderLamp(&(lightPtr->LightWorld));
			break;
		*/

		// END OF SHOULDER LAMP IN MULTI
	}
}


void HandleLightElementSystem(void)
{
	int i = NumActiveLightElements;
	LIGHTELEMENT *lightElementPtr = LightElementStorage;
	
	while(i--)
	{
		LIGHTBLOCK *lightPtr = &(lightElementPtr->LightBlock);
		
		switch(lightElementPtr->BehaviourID)
		{
			case LIGHTELEMENT_MOLTENMETAL:
			{

				lightPtr->LightBright = ONE_FIXED/4;
				/* flags */
				lightPtr->LightFlags = LFlag_Omni;
				/* lightblock light type */
				lightPtr->LightType = LightType_PerVertex;
				/* range */
				lightPtr->LightRange = EXPLOSION_LIGHT_RANGE; 

				lightPtr->RedScale=	  255*256;
				lightPtr->GreenScale= 120*256;
				lightPtr->BlueScale=  0;
				
				break;
			}
			case LIGHTELEMENT_PLASMACASTERHIT:
			{
				/* flags */
				lightPtr->LightFlags = LFlag_Omni;
				/* lightblock light type */
				lightPtr->LightType = LightType_PerVertex;
				/* range */
				lightPtr->LightRange = EXPLOSION_LIGHT_RANGE; 

				if (lightElementPtr->LifeTime==ONE_FIXED)
				{
					lightPtr->LightBright = ONE_FIXED*4;

					lightPtr->RedScale=	  0*256;
					lightPtr->GreenScale= 255*256;
					lightPtr->BlueScale=  255*256;
				}
				else
				{
					lightPtr->LightBright = lightElementPtr->LifeTime/2;
					lightPtr->LightRange = 1+MUL_FIXED(EXPLOSION_LIGHT_RANGE,lightElementPtr->LifeTime);

					lightPtr->RedScale=	  255*256;
					lightPtr->GreenScale= 120*256;
					lightPtr->BlueScale=  0*256;
				}

				lightElementPtr->LifeTime-=NormalFrameTime*4;
				
				break;
			}
			case LIGHTELEMENT_FROMFMV:
			{						 
				extern int FmvColourRed;
				extern int FmvColourGreen;
				extern int FmvColourBlue;

				lightPtr->LightBright = ONE_FIXED*4;
				/* flags */
				lightPtr->LightFlags = LFlag_Omni;
				/* lightblock light type */
				lightPtr->LightType = LightType_PerVertex;
				/* range */
				lightPtr->LightRange = 15000; 

				lightPtr->RedScale=	  FmvColourRed;
				lightPtr->GreenScale= FmvColourGreen;
				lightPtr->BlueScale=  FmvColourBlue;
				  					
				break;
			}
			case LIGHTELEMENT_EXPLOSION:
			{

				/* flags */
				lightPtr->LightFlags = LFlag_Omni;
				/* lightblock light type */
				lightPtr->LightType = LightType_PerVertex;

				lightPtr->RedScale=255*256;
				lightPtr->GreenScale=120*256;
				lightPtr->BlueScale=0;
				{
					int scale = lightElementPtr->LifeTime*4;
					
					if (scale < ONE_FIXED)
					{
						lightPtr->LightRange = 1+MUL_FIXED(EXPLOSION_LIGHT_RANGE,scale);
						lightPtr->LightBright = scale*8;
					}
					else 
					{
						lightPtr->LightRange = EXPLOSION_LIGHT_RANGE;
						lightPtr->LightBright = ONE_FIXED*8;
					}
				}

				
				{
					PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);

					if (playerStatusPtr->IsAlive)
					{
						VECTORCH d = lightElementPtr->LightBlock.LightWorld;
						int m;
						d.vx -= Global_VDB_Ptr->VDB_World.vx;
						d.vy -= Global_VDB_Ptr->VDB_World.vy;
						d.vz -= Global_VDB_Ptr->VDB_World.vz;
						m = Approximate3dMagnitude(&d);

						if (m<ONE_FIXED)
						{
							int maxTilt = MUL_FIXED((ONE_FIXED-m)>>9,lightElementPtr->LifeTime); 
							int halfTilt = maxTilt/2;
							if (maxTilt)
							{
								HeadOrientation.EulerX = (FastRandom()%maxTilt)-halfTilt;
								HeadOrientation.EulerY = (FastRandom()%maxTilt)-halfTilt;
								HeadOrientation.EulerZ = (FastRandom()%maxTilt)-halfTilt;

								if (HeadOrientation.EulerX < 0) HeadOrientation.EulerX += 4096;
								if (HeadOrientation.EulerY < 0) HeadOrientation.EulerY += 4096;
								if (HeadOrientation.EulerZ < 0) HeadOrientation.EulerZ += 4096;
							}
						}
					}
				}

				lightElementPtr->LifeTime-=NormalFrameTime;
				break;
			}
			case LIGHTELEMENT_ELECTRICAL_EXPLOSION:
			{
				int scale = ONE_FIXED*5/4-lightElementPtr->LifeTime;

				/* flags */
				lightPtr->LightFlags = LFlag_Omni|LFlag_Electrical;
				/* lightblock light type */
				lightPtr->LightType = LightType_PerVertex;

				if (scale>65536) scale = 65536;
				scale = ONE_FIXED - scale;

				lightPtr->RedScale= scale;
				lightPtr->GreenScale=ONE_FIXED;
				lightPtr->BlueScale=ONE_FIXED;
				lightPtr->LightRange = EXPLOSION_LIGHT_RANGE;// 1+MUL_FIXED(EXPLOSION_LIGHT_RANGE,scale);
				lightPtr->LightBright = scale*16;
				
				{
					PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);

					if (playerStatusPtr->IsAlive)
					{
						VECTORCH d = lightElementPtr->LightBlock.LightWorld;
						int m;
						d.vx -= Global_VDB_Ptr->VDB_World.vx;
						d.vy -= Global_VDB_Ptr->VDB_World.vy;
						d.vz -= Global_VDB_Ptr->VDB_World.vz;
						m = Approximate3dMagnitude(&d);

						if (m<ONE_FIXED) {
							// Create Spatial Shockwave effect.. niiiice &-P
							SpatialShockwave=1;
						} else {
							SpatialShockwave=0;
						}
					}
				}
				lightElementPtr->LifeTime-=NormalFrameTime;

				if (lightElementPtr->LifeTime <= NormalFrameTime) {
					SpatialShockwave=0;
				}
				break;
			}
			case LIGHTELEMENT_ELECTRICAL_SPARKS:
			{
				int scale = lightElementPtr->LifeTime;

				/* flags */
				lightPtr->LightFlags = LFlag_Omni|LFlag_Electrical;
				/* lightblock light type */
				lightPtr->LightType = LightType_PerVertex;

				if (scale == ONE_FIXED)
				{
					lightPtr->RedScale=ONE_FIXED;
					lightPtr->GreenScale=ONE_FIXED;
					lightPtr->BlueScale=ONE_FIXED;
				}
				else
				{
					lightPtr->RedScale= 0;
					lightPtr->GreenScale= scale/2;
					lightPtr->BlueScale= scale;
				}
				lightPtr->LightRange = 4000;// 1+MUL_FIXED(EXPLOSION_LIGHT_RANGE,scale);
				lightPtr->LightBright = scale;
				
				lightElementPtr->LifeTime-=NormalFrameTime*2;
				break;
			}

			case LIGHTELEMENT_ROTATING:
			{						 

				lightPtr->LightBright = ONE_FIXED/2;
				/* flags */
				lightPtr->LightFlags = LFlag_Omni;
				/* lightblock light type */
				lightPtr->LightType = LightType_PerVertex;
				/* range */
				lightPtr->LightRange = 10000; 

				lightPtr->RedScale=	  ONE_FIXED;
				lightPtr->GreenScale= ONE_FIXED;
				lightPtr->BlueScale=  ONE_FIXED;

				lightElementPtr->LightBlock.LightWorld = Player->ObWorld;//RotatingLightPosition;
//				lightElementPtr->LightBlock.LightWorld.vx += MUL_FIXED(2000,GetCos((CloakingPhase/2)&4095));  					
				lightElementPtr->LightBlock.LightWorld.vy -= 2200;  					
//				lightElementPtr->LightBlock.LightWorld.vz += MUL_FIXED(2000,GetSin((CloakingPhase/2)&4095));  					
				#if 0
				{	

					VECTORCH zero = {0,0,0};
					MakeParticle(&(lightElementPtr->LightBlock.LightWorld),&zero,PARTICLE_SPARK);	
				}
				#endif
				break;
			}
			case LIGHTELEMENT_ALIEN_TEETH:
			case LIGHTELEMENT_ALIEN_TEETH2:
			{
				break;
			}
			
			case LIGHTELEMENT_PARGEN_FLAME :
			{
				if(lightElementPtr->LifeTime == ONE_FIXED)
				{
					lightElementPtr->LifeTime = 1;

					/* flags */
					lightPtr->LightFlags = LFlag_Omni;
					/* lightblock light type */
					lightPtr->LightType = LightType_PerVertex;

					//lightPtr->RedScale=	  255*256;
					//lightPtr->GreenScale= 120*256;
					lightPtr->RedScale=	  255*(200+(CloakingPhase%56));
					lightPtr->GreenScale= 120*(200+((CloakingPhase/8)%56));
					lightPtr->BlueScale=  0;
					
					lightPtr->LightRange = 6000;
					lightPtr->LightBright = ONE_FIXED;
				
				}
				else
				{
					lightElementPtr->LifeTime = 0;
				}
			}
			break;

			// SHOULDER LAMP IN MULTI - this no longer needed as the lamp is now attached to the
			// Players DISPLAY_BLOCK as an 'Object Light'
			/*
			case LIGHTELEMENT_SHOULDER_LAMP:
				PositionShoulderLamp(&(lightPtr->LightWorld));
				break;
			*/
			// END OF SHOULDER LAMP IN MULTI

			default:
				break;
		}

		lightPtr->BrightnessOverRange = DIV_FIXED(MUL_FIXED(lightPtr->LightBright,LightScale),lightPtr->LightRange);
		
		if (lightElementPtr->LifeTime<=0)
		{
			DeallocateLightElement(lightElementPtr);
		}
		else
		{
			lightElementPtr++;
		}
	}
}

// SHOULDER LAMP IN MULTI - this no longer needed as the lamp is now attached to the
// Players DISPLAY_BLOCK as an 'Object Light'

/*
static LIGHTELEMENT *FindLightElement(enum LIGHTELEMENT_BEHAVIOUR_ID TargetBehaviourID)
{
	LIGHTELEMENT *lightElementPtr = LightElementStorage;
	int ListIndex = 0;

	while (ListIndex < NumActiveLightElements)
	{
		if (lightElementPtr->BehaviourID == TargetBehaviourID)
		{
			return (lightElementPtr);
		}
		ListIndex++;
		lightElementPtr++;
	}
	return (NULL);
}

*/
// END OF SHOULDER LAMP IN MULTI

/*--------------------------**
** Load/Save Light Elements **
**--------------------------*/
#include "savegame.h"

typedef struct light_element_save_block_header
{
	SAVE_BLOCK_HEADER header;

	int NumActiveLightElements;

	//followed by array of light elements
}LIGHT_ELEMENT_SAVE_BLOCK_HEADER;

void Load_LightElements(SAVE_BLOCK_HEADER* header)
{
	int i;
	LIGHT_ELEMENT_SAVE_BLOCK_HEADER* block = (LIGHT_ELEMENT_SAVE_BLOCK_HEADER*) header;
	LIGHTELEMENT* saved_light_element = (LIGHTELEMENT*) (block+1);
	int expected_size;

	//make sure the block is the correct size
	expected_size = sizeof(*block);
	expected_size += sizeof(LIGHTELEMENT) * block->NumActiveLightElements;
	if(header->size != expected_size) return;


	for(i=0;i<block->NumActiveLightElements;i++)
	{
		LIGHTELEMENT* light_element = AllocateLightElement();
		if(light_element) 
		{
			*light_element = *saved_light_element++;	
		}
	}

}

void Save_LightElements()
{
	LIGHT_ELEMENT_SAVE_BLOCK_HEADER* block;
	int i;

	if(!NumActiveLightElements) return;

	//get memory for header
	GET_SAVE_BLOCK_POINTER(block);

	//fill in header
	block->header.type = SaveBlock_LightElements;
	block->header.size = sizeof(*block) + NumActiveLightElements * sizeof(LIGHTELEMENT);

	block->NumActiveLightElements = NumActiveLightElements;


	//now save the light elements
	for(i=0;i<NumActiveLightElements;i++)
	{
		LIGHTELEMENT* light = GET_SAVE_BLOCK_POINTER(light);
		*light = LightElementStorage[i];	
	}
	
}

#define DEFAULT_LAMP_DISTANCE 7500

// SHOULDER LAMP IN MULTI - remove the 'static' from this function
// static void PositionShoulderLamp(VECTORCH *Position)
// END OF SHOULDER LAMP IN MULTI
void PositionShoulderLamp(VECTORCH *Position)
{
	int DistanceFromPlayer;

	if (Global_VDB_Ptr == NULL) return;

	if ((PlayersTarget.DispPtr) && (PlayersTarget.Distance < DEFAULT_LAMP_DISTANCE))
	{
		DistanceFromPlayer = PlayersTarget.Distance;
	}
	else
	{
		DistanceFromPlayer = DEFAULT_LAMP_DISTANCE;
	}

	{
		VECTORCH LampOffset = {-50,50,DistanceFromPlayer};
		MATRIXCH mat = Global_VDB_Ptr->VDB_Mat;
		TransposeMatrixCH(&mat);
		RotateVector(&LampOffset, &mat);

		Position->vx = Global_VDB_Ptr->VDB_World.vx + LampOffset.vx;
		Position->vy = Global_VDB_Ptr->VDB_World.vy + LampOffset.vy;
		Position->vz = Global_VDB_Ptr->VDB_World.vz + LampOffset.vz;
	}
}

// SHOULDER LAMP IN MULTI - Replace the existing function with this one
void DisableShoulderLamp()
{
	PLAYER_STATUS	*playerStatusPtr;

	if (   ( Player == NULL )
		|| ( Player->ObStrategyBlock == NULL )
		|| ( Player->ObStrategyBlock->SBdataptr == NULL ))
	{
		return;
	}

	// Get reference to player's state...
	playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	// ...and ensure the lamp is switched off
	playerStatusPtr->IAmUsingShoulderLamp = 0;
}
// END OF SHOULDER LAMP IN MULTI



// SHOULDER LAMP IN MULTI - Replace the existing function with this one
void	ToggleShoulderLamp ()
{
	PLAYER_STATUS	*playerStatusPtr;

	if (AvP.PlayerType != I_Marine) return;
	if (GrabbedPlayer) return;

	if (   ( Player == NULL )
		|| ( Player->ObStrategyBlock == NULL )
		|| ( Player->ObStrategyBlock->SBdataptr == NULL ))
	{
		return;
	}

	// Get reference to player's state...
	playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);

	// Don't have a Shoulder Lamp? Tough luck! :-P
	if (!playerStatusPtr->IRGoggles) return;

	// ...then reverse the IAmUsingShoulderLamp setting
	playerStatusPtr->IAmUsingShoulderLamp = !playerStatusPtr->IAmUsingShoulderLamp;

	if ( playerStatusPtr->IAmUsingShoulderLamp )
	{
		NewOnScreenMessage("Shoulder Lamp activated");
		Sound_Play(SID_POWERDN,"h");
	}
	else
	{
		NewOnScreenMessage("Shoulder Lamp deactivated");
		Sound_Play(SID_POWERUP,"h");
	}

	if (AvP.Network != I_No_Network)
		ChangedMyClass = 1;
}
// END OF SHOULDER LAMP IN MULTI
