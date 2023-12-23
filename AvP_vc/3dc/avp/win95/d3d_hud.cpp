/* KJL 14:43:27 10/6/97 - d3d_hud.cpp

	Things just got too messy with ddplat.cpp & d3_func.cpp,
	so this file will hold all Direct 3D hud code.

 */
extern "C" {

// Mysterious definition required by objbase.h 
// (included via one of the include files below)
// to start definition of obscure unique in the
// universe IDs required  by Direct3D before it
// will deign to cough up with anything useful...

#include "3dc.h"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"
#include "d3dmacs.h"
//#include "string.h"

#include "hudgfx.h"
#include "huddefs.h"
//#include "hud_data.h"
#include "kshape.h"
#include "chnktexi.h"


#include "HUD_layout.h"
#include "language.h"



extern "C++" 									  
{
#include "r2base.h"
#include "pcmenus.h"
//#include "projload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
extern void D3D_RenderHUDString(char *stringPtr, int x, int y, int colour);
extern void D3D_RenderHUDString_Centred(char *stringPtr, int centreX, int y, int colour);
extern void D3D_RenderHUDNumber_Centred(unsigned int number,int x,int y,int colour);

};

#include "d3d_hud.h"


#define UseLocalAssert No
#include "ourasert.h"
											

#include "vision.h"
#define RGBLIGHT_MAKE(rr,gg,bb) \
( \
	LCCM_NORMAL == d3d_light_ctrl.ctrl ? \
		RGB_MAKE(rr,gg,bb) \
	: LCCM_CONSTCOLOUR == d3d_light_ctrl.ctrl ? \
		RGB_MAKE(MUL_FIXED(rr,d3d_light_ctrl.r),MUL_FIXED(gg,d3d_light_ctrl.g),MUL_FIXED(bb,d3d_light_ctrl.b)) \
	: \
		RGB_MAKE(d3d_light_ctrl.GetR(rr),d3d_light_ctrl.GetG(gg),d3d_light_ctrl.GetB(bb)) \
)
#define RGBALIGHT_MAKE(rr,gg,bb,aa) \
( \
		RGBA_MAKE(rr,gg,bb,aa) \
)
#include "kshape.h"



void D3D_DrawHUDFontCharacter(HUDCharDesc *charDescPtr);
void D3D_DrawHUDDigit(HUDCharDesc *charDescPtr);

extern void YClipMotionTrackerVertices(struct VertexTag *v1, struct VertexTag *v2);
extern void XClipMotionTrackerVertices(struct VertexTag *v1, struct VertexTag *v2);
/* HUD globals */
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern int sine[],cosine[];
extern int Operable;
extern int HackPool;
extern int WeldPool;
extern int WeldMode;
extern int StrugglePool;
extern DPID GrabbedPlayer;
extern DPID GrabPlayer;

extern enum HUD_RES_ID HUDResolution;

signed int HUDTranslucencyLevel=64;

static int MotionTrackerHalfWidth;
static int MotionTrackerTextureSize;
static int MotionTrackerCentreY;
static int MotionTrackerCentreX;
static int MT_BlipHeight;
static int MT_BlipWidth;
static HUDImageDesc BlueBar;


int HUDImageNumber;
int SpecialFXImageNumber;
int SmokyImageNumber;
int ChromeImageNumber;
int CloudyImageNumber;
int BurningImageNumber;
int HUDFontsImageNumber;
int RebellionLogoImageNumber;
int FoxLogoImageNumber;
int MotionTrackerScale;
int PredatorVisionChangeImageNumber;
int PredatorNumbersImageNumber;
int StaticImageNumber;
int AlienTongueImageNumber;
int AAFontImageNumber;
int WaterShaftImageNumber;

int SkyImageNumber;

// Waveforms
int WaveIdleImageNumber;
int WaveScan1ImageNumber;
int WaveScan2ImageNumber;
int WaveScan3ImageNumber;

// Marine HUD items
int AirImageNumber;
int TargetingImageNumber;
int MarineHealthImageNumber;
int MarineArmorImageNumber;
int MarineFlareImageNumber;
int LampOnImageNumber;
int LampOffImageNumber;
int LampBatteryImageNumber;
int HepImageNumber;
int OperateImageNumber;
int NoOperateImageNumber;
int ComTechOperateImageNumber;

// Class menu stuff
int CellImageNumber;

int HUDScaleFactor;

static struct HUDFontDescTag HUDFontDesc[] =
{
	//MARINE_HUD_FONT_BLUE,
	{
		225,//XOffset
		24,//Height
		16,//Width
	},
	//MARINE_HUD_FONT_RED,
	{
		242,//XOffset
		24,//Height
		14,//Width
	},
	//MARINE_HUD_FONT_MT_SMALL,
	{
		232,//XOffset
		12,//Height
		8,//Width
	},
	//MARINE_HUD_FONT_MT_BIG,
	{
		241,//XOffset
		24,//Height
		14,//Width
	},
};
#define BLUE_BAR_WIDTH ((203-0)+1)
#define BLUE_BAR_HEIGHT ((226-195)+1)

void D3D_BLTDigitToHUD(char digit, int x, int y, int font);


void Draw_HUDImage(HUDImageDesc *imageDescPtr)
{
	struct VertexTag quadVertices[4];
	int scaledWidth;
	int scaledHeight;

	if (imageDescPtr->Scale == ONE_FIXED)
	{
		scaledWidth = imageDescPtr->Width;
		scaledHeight = imageDescPtr->Height;
	}
	else
	{
		scaledWidth = MUL_FIXED(imageDescPtr->Scale,imageDescPtr->Width);
		scaledHeight = MUL_FIXED(imageDescPtr->Scale,imageDescPtr->Height);
	}

	quadVertices[0].U = imageDescPtr->TopLeftU;
	quadVertices[0].V = imageDescPtr->TopLeftV;
	quadVertices[1].U = imageDescPtr->TopLeftU + imageDescPtr->Width;
	quadVertices[1].V = imageDescPtr->TopLeftV;
	quadVertices[2].U = imageDescPtr->TopLeftU + imageDescPtr->Width;
	quadVertices[2].V = imageDescPtr->TopLeftV + imageDescPtr->Height;
	quadVertices[3].U = imageDescPtr->TopLeftU;
	quadVertices[3].V = imageDescPtr->TopLeftV + imageDescPtr->Height;
	
	quadVertices[0].X = imageDescPtr->TopLeftX;
	quadVertices[0].Y = imageDescPtr->TopLeftY;
	quadVertices[1].X = imageDescPtr->TopLeftX + scaledWidth;
	quadVertices[1].Y = imageDescPtr->TopLeftY;
	quadVertices[2].X = imageDescPtr->TopLeftX + scaledWidth;
	quadVertices[2].Y = imageDescPtr->TopLeftY + scaledHeight;
	quadVertices[3].X = imageDescPtr->TopLeftX;
	quadVertices[3].Y = imageDescPtr->TopLeftY + scaledHeight;
		
	D3D_HUDQuad_Output
	(
		imageDescPtr->ImageNumber,
		quadVertices,
		RGBALIGHT_MAKE
		(
			imageDescPtr->Red,
			imageDescPtr->Green,
			imageDescPtr->Blue,
			imageDescPtr->Translucency
		)
	);
}


void D3D_InitialiseMarineHUD(void)
{
	//SelectGenTexDirectory(ITI_TEXTURE);

	extern unsigned char *ScreenBuffer;

	/* set game mode: different though for multiplayer game */
	if(AvP.Network==I_No_Network)
		cl_pszGameMode = "marine";
	else
		cl_pszGameMode = "multip";

	/* load HUD gfx of correct resolution */
	{
		HUDResolution = HUD_RES_MED;
		HUDImageNumber = CL_LoadImageOnce("Huds\\Marine\\MarineHUD.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		MotionTrackerHalfWidth = 127/2;
		MotionTrackerTextureSize = 128;

		BlueBar.ImageNumber = HUDImageNumber;
		BlueBar.TopLeftX = 0;
		BlueBar.TopLeftY = ScreenDescriptorBlock.SDB_Height-40;
		BlueBar.TopLeftU = 1;
		BlueBar.TopLeftV = 223;
		BlueBar.Red = 255;
		BlueBar.Green = 255;
		BlueBar.Blue = 255;

		BlueBar.Height = BLUE_BAR_HEIGHT;
		BlueBar.Width = BLUE_BAR_WIDTH;

		/* motion tracker blips */
		MT_BlipHeight = 12;
		MT_BlipWidth = 12;

		/* load in sfx */
		SpecialFXImageNumber = CL_LoadImageOnce("Common\\partclfx.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);

//		SpecialFXImageNumber = CL_LoadImageOnceEx("flame1",IRF_D3D,DDSCAPS_SYSTEMMEMORY,0);;
//		SpecialFXImageNumber = CL_LoadImageOnceEx("star",IRF_D3D,DDSCAPS_SYSTEMMEMORY,0);;
//		SmokyImageNumber = CL_LoadImageOnceEx("smoky",IRF_D3D,DDSCAPS_SYSTEMMEMORY,0);

	}

	/* centre of motion tracker */
	MotionTrackerCentreY = BlueBar.TopLeftY;
	MotionTrackerCentreX = BlueBar.TopLeftX+(BlueBar.Width/2);
	MotionTrackerScale = 65536;

	HUDScaleFactor = DIV_FIXED(ScreenDescriptorBlock.SDB_Width,640);	

	#if UseGadgets
//	MotionTrackerGadget::SetCentre(r2pos(100,100));
	#endif
}

void LoadCommonTextures(void)
{
//	PredatorVisionChangeImageNumber = CL_LoadImageOnce("HUDs\\Predator\\predvisfx.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);

	if(AvP.Network==I_No_Network)
	{
		switch(AvP.PlayerType)
		{
			case I_Predator:
			{
				PredatorNumbersImageNumber = CL_LoadImageOnce("HUDs\\Predator\\predNumbers.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				StaticImageNumber = CL_LoadImageOnce("Common\\static.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
				WaveIdleImageNumber = CL_LoadImageOnce("HUDs\\Predator\\wave_idle.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				WaveScan1ImageNumber = CL_LoadImageOnce("HUDs\\Predator\\wave_scan1.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				WaveScan2ImageNumber = CL_LoadImageOnce("HUDs\\Predator\\wave_scan2.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				WaveScan3ImageNumber = CL_LoadImageOnce("HUDs\\Predator\\wave_scan3.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				break;
			}
			case I_Alien:
			{
				AlienTongueImageNumber = CL_LoadImageOnce("HUDs\\Alien\\AlienTongue.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				OperateImageNumber = CL_LoadImageOnce("HUDs\\Marine\\operate.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				NoOperateImageNumber = CL_LoadImageOnce("HUDs\\Marine\\no_operate.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				ComTechOperateImageNumber = CL_LoadImageOnce("HUDs\\Marine\\lockpick_operate.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				break;
			}
			case I_Marine:
			{
				StaticImageNumber = CL_LoadImageOnce("Common\\static.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
				TargetingImageNumber = CL_LoadImageOnce("HUDs\\Marine\\SmartgunReticle.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				AirImageNumber = CL_LoadImageOnce("HUDs\\Marine\\tracker_battery.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				MarineHealthImageNumber = CL_LoadImageOnce("HUDs\\Marine\\health.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				MarineArmorImageNumber = CL_LoadImageOnce("HUDs\\Marine\\armor.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				MarineFlareImageNumber = CL_LoadImageOnce("HUDs\\Marine\\flares.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				LampOnImageNumber = CL_LoadImageOnce("HUDs\\Marine\\lamp_on.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				LampOffImageNumber = CL_LoadImageOnce("HUDs\\Marine\\lamp_off.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				LampBatteryImageNumber = CL_LoadImageOnce("HUDs\\Marine\\lamp_battery.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				HepImageNumber = CL_LoadImageOnce("HUDs\\Marine\\hep.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				OperateImageNumber = CL_LoadImageOnce("HUDs\\Marine\\operate.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				NoOperateImageNumber = CL_LoadImageOnce("HUDs\\Marine\\no_operate.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				ComTechOperateImageNumber = CL_LoadImageOnce("HUDs\\Marine\\lockpick_operate.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				break;
			}
			default:
				break;
		}
	}
	else
	{
		// load menu gfx
		CellImageNumber = CL_LoadImageOnce("Menus\\cell.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);

		OperateImageNumber = CL_LoadImageOnce("HUDs\\Marine\\operate.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		NoOperateImageNumber = CL_LoadImageOnce("HUDs\\Marine\\no_operate.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		ComTechOperateImageNumber = CL_LoadImageOnce("HUDs\\Marine\\lockpick_operate.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
   		PredatorNumbersImageNumber = CL_LoadImageOnce("HUDs\\Predator\\predNumbers.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
   		StaticImageNumber = CL_LoadImageOnce("Common\\static.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
		AlienTongueImageNumber = CL_LoadImageOnce("HUDs\\Alien\\AlienTongue.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		WaveIdleImageNumber = CL_LoadImageOnce("HUDs\\Predator\\wave_idle.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		WaveScan1ImageNumber = CL_LoadImageOnce("HUDs\\Predator\\wave_scan1.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		WaveScan2ImageNumber = CL_LoadImageOnce("HUDs\\Predator\\wave_scan2.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		WaveScan3ImageNumber = CL_LoadImageOnce("HUDs\\Predator\\wave_scan3.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		TargetingImageNumber = CL_LoadImageOnce("HUDs\\Marine\\SmartgunReticle.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		AirImageNumber = CL_LoadImageOnce("HUDs\\Marine\\tracker_battery.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		MarineHealthImageNumber = CL_LoadImageOnce("HUDs\\Marine\\health.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		MarineArmorImageNumber = CL_LoadImageOnce("HUDs\\Marine\\armor.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		MarineFlareImageNumber = CL_LoadImageOnce("HUDs\\Marine\\flares.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		LampOnImageNumber = CL_LoadImageOnce("HUDs\\Marine\\lamp_on.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		LampOffImageNumber = CL_LoadImageOnce("HUDs\\Marine\\lamp_off.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		LampBatteryImageNumber = CL_LoadImageOnce("HUDs\\Marine\\lamp_battery.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		HepImageNumber = CL_LoadImageOnce("HUDs\\Marine\\hep.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	}
	
	HUDFontsImageNumber = CL_LoadImageOnce("Common\\HUDfonts.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	SpecialFXImageNumber = CL_LoadImageOnce("Common\\partclfx.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE/*|LIO_TRANSPARENT*/);
	CloudyImageNumber = CL_LoadImageOnce("Common\\cloudy.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	BurningImageNumber = CL_LoadImageOnce("Common\\burn.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	
	SkyImageNumber = CL_LoadImageOnce("Common\\cloudy.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	{
		extern char LevelName[];

		ChromeImageNumber = CL_LoadImageOnce("Common\\Glass.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);

		// Custom Skies -- AMP Addition
		//sprintf(tex, "Envrnmts\\%s\\Asky.RIM", LevelName);
  		//SkyImageNumber = CL_LoadImageOnce(tex,LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		// End of Custom Skies -- ELD

		if (!strcmp(LevelName,"invasion_a"))
		{
		   	ChromeImageNumber = CL_LoadImageOnce("Envrnmts\\Invasion\\water2.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
			WaterShaftImageNumber = CL_LoadImageOnce("Envrnmts\\Invasion\\water-shaft.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
		}
		else if (!strcmp(LevelName,"genshd1"))
		{
			WaterShaftImageNumber = CL_LoadImageOnce("Envrnmts\\GenShd1\\colonywater.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
			SkyImageNumber = CL_LoadImageOnce("Envrnmts\\GenShd1\\Asky.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);

		}
		else if (!strcmp(LevelName,"fall")||!strcmp(LevelName,"fall_m"))
		{
			ChromeImageNumber = CL_LoadImageOnce("Envrnmts\\fall\\stream_water.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
		}
		else if (!strcmp(LevelName,"derelict"))
		{
			ChromeImageNumber = CL_LoadImageOnce("Envrnmts\\derelict\\water.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
		}
		
	}

	#if 1
	{
		extern void InitDrawTest(void);
		InitDrawTest();
	}
	#endif

}

void D3D_BLTNewIconsToHUD(unsigned int type, unsigned int x, unsigned int y)
{
	HUDImageDesc imageDesc;
	PLAYER_STATUS *psPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	LOCALASSERT(psPtr);

	if (type==0) {	// Health
		imageDesc.ImageNumber = MarineHealthImageNumber;
		imageDesc.TopLeftX = x;
		imageDesc.TopLeftY = y;
		imageDesc.TopLeftU = 0;
		imageDesc.TopLeftV = 0;
		imageDesc.Height = 30;
		imageDesc.Width = 30;
		imageDesc.Scale = ONE_FIXED;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = 255;

		Draw_HUDImage(&imageDesc);
	} else
	if (type==1) {	// Armor
		if (psPtr->ArmorType==0)
			imageDesc.ImageNumber = MarineArmorImageNumber;
		else
			imageDesc.ImageNumber = HepImageNumber;
		imageDesc.TopLeftX = x;
		imageDesc.TopLeftY = y;
		imageDesc.TopLeftU = 0;
		imageDesc.TopLeftV = 0;
		imageDesc.Height = 30;
		imageDesc.Width = 30;
		imageDesc.Scale = ONE_FIXED;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = 255;

		Draw_HUDImage(&imageDesc);
	} else
	if (type==2) {	// Flares
		imageDesc.ImageNumber = MarineFlareImageNumber;
		imageDesc.TopLeftX = x;
		imageDesc.TopLeftY = y;
		imageDesc.TopLeftU = 0;
		imageDesc.TopLeftV = 0;
		imageDesc.Height = 30;
		imageDesc.Width = 30;
		imageDesc.Scale = ONE_FIXED;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = 255;

		Draw_HUDImage(&imageDesc);
	}
}

void D3D_BLTIconsToHUD()
{
	PLAYER_STATUS *psPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	HUDImageDesc imageDesc;
	int Airsupply = (psPtr->TrackerTimer)/ONE_FIXED;
	int Lamp = (psPtr->PGC)/ONE_FIXED;
	int Air;

	LOCALASSERT(psPtr);

	Air = Airsupply/6;
	Lamp = Lamp/2;

	if (Airsupply < 0) Airsupply = 0;
	if (Air > 128) Air = 128;
	if (Lamp < 0) Lamp = 0;

	// Tracker Battery Icon
	if (psPtr->MTrackerType > 1) {
		imageDesc.ImageNumber = AirImageNumber;
		imageDesc.TopLeftX = 270;
		imageDesc.TopLeftY = ScreenDescriptorBlock.SDB_Height-30;
		imageDesc.TopLeftU = 0;
		imageDesc.TopLeftV = 128-16;
		imageDesc.Height = 16;
		imageDesc.Width = Air;
		imageDesc.Scale = ONE_FIXED;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = 255;

		Draw_HUDImage(&imageDesc);
	}

	// Shoulder Lamp Icon
	if (psPtr->IRGoggles) {
		if (psPtr->IAmUsingShoulderLamp)
			imageDesc.ImageNumber = LampOnImageNumber;
		else
			imageDesc.ImageNumber = LampOffImageNumber;
		imageDesc.TopLeftX = 0;
		imageDesc.TopLeftY = 20;
		imageDesc.TopLeftU = 0;
		imageDesc.TopLeftV = 0;
		imageDesc.Height = 30;
		imageDesc.Width = 30;
		imageDesc.Scale = ONE_FIXED;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = 255;

		Draw_HUDImage(&imageDesc);
	}

	// Shoulder Lamp Battery
	if (psPtr->IRGoggles) {
		imageDesc.ImageNumber = LampBatteryImageNumber;
		imageDesc.TopLeftX = 35;
		imageDesc.TopLeftY = 20;
		imageDesc.TopLeftU = 0;
		imageDesc.TopLeftV = 128-30;
		imageDesc.Height = Lamp;
		imageDesc.Width = 16;
		imageDesc.Scale = ONE_FIXED;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = 255;

		Draw_HUDImage(&imageDesc);
	}
}

void D3D_BLTWaveformToHUD(int type)
{
	int WaveformSize = MUL_FIXED(65536*3,31);
	int scanLineSize = 2200;
	int WaveformWidth = 64;
	int WaveformY = 0;
	int WaveformX = 0;

	HUDImageDesc imageDesc;

	if (type == 0)
	{
		imageDesc.ImageNumber = WaveIdleImageNumber;
		
		// Screen location where texture will be drawn
		imageDesc.TopLeftX = WaveformX;		
		imageDesc.TopLeftY = WaveformY;

		// The point is that we don't have to draw the whole texture...this is how
		// the hud-digits work - all the digits are in the same texture, but we know
		// where each of them is positioned within the texture. This allows us to
		// draw individaul hud digits, by specifying that only that part of the texture
		// to be drawn. The next 4 fields define a rectangle inside the texture - only
		// this rectangle will be drawn. In this case we want to draw the whole texture
		// so.....
		imageDesc.TopLeftU = 0;		// First two are the top-left corner of the rectangle.
		imageDesc.TopLeftV = 0;
		imageDesc.Height = ScreenDescriptorBlock.SDB_Height;// 2nd two are the width/height of the rectangle
		imageDesc.Width = 64;
		
		// Allows texture to be scaled e.g. putting 2 * ONE_FIXED would draw twice the size.
		imageDesc.Scale = ONE_FIXED;

		// Not sure what the effect of these is...but if set them all to 255 then the texture
		// appears with the correct colours.
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;

		// Whether u can see through the texture ( from 0 - 255 )
		// ranges from 0 ( invisible ) up to 255 ( cant see throught it )
		imageDesc.Translucency = 255;
	} 
	else if (type == 1)
	{
		imageDesc.ImageNumber = WaveScan1ImageNumber;
		imageDesc.Scale = ONE_FIXED;
		imageDesc.TopLeftX = WaveformX;
		imageDesc.TopLeftY = WaveformY;
		imageDesc.TopLeftU = 0;
		imageDesc.TopLeftV = 0;
		imageDesc.Height = ScreenDescriptorBlock.SDB_Height;
		imageDesc.Width = 64;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = 255;
	}
	else if (type == 2)
	{
		imageDesc.ImageNumber = WaveScan2ImageNumber;
		imageDesc.Scale = ONE_FIXED;
		imageDesc.TopLeftX = WaveformX;
		imageDesc.TopLeftY = WaveformY;
		imageDesc.TopLeftU = 0;
		imageDesc.TopLeftV = 0;
		imageDesc.Height = ScreenDescriptorBlock.SDB_Height;
		imageDesc.Width = 64;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = 255;
	}
	else if (type == 3)
	{
		imageDesc.ImageNumber = WaveScan3ImageNumber;
		imageDesc.Scale = ONE_FIXED;
		imageDesc.TopLeftX = WaveformX;
		imageDesc.TopLeftY = WaveformY;
		imageDesc.TopLeftU = 0;
		imageDesc.TopLeftV = 0;
		imageDesc.Height = ScreenDescriptorBlock.SDB_Height;
		imageDesc.Width = 64;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = 255;
	}
 	Draw_HUDImage(&imageDesc);
}

void D3D_BLTMotionTrackerToHUD(int scanLineSize)
{

	struct VertexTag quadVertices[4];
	int widthCos,widthSin;
	extern int CloakingPhase;

	BlueBar.TopLeftY = ScreenDescriptorBlock.SDB_Height-MUL_FIXED(MotionTrackerScale,40);
	MotionTrackerCentreY = BlueBar.TopLeftY;
	MotionTrackerCentreX = BlueBar.TopLeftX+MUL_FIXED(MotionTrackerScale,(BlueBar.Width/2));
	BlueBar.Scale = MotionTrackerScale;

	int motionTrackerScaledHalfWidth = MUL_FIXED(MotionTrackerScale*3,MotionTrackerHalfWidth/2);

	{
		int angle = 4095 - Player->ObEuler.EulerY;
	
		widthCos = MUL_FIXED
				   (
				   		motionTrackerScaledHalfWidth,
				   		GetCos(angle)
				   );
		widthSin = MUL_FIXED
				   (
				   		motionTrackerScaledHalfWidth,
						GetSin(angle)
				   );
	}			
	
	/* I've put these -1s in here to help clipping 45 degree cases,
	where two vertices can end up around the clipping line of Y=0 */
	quadVertices[0].X = (-widthCos - (-widthSin));
	quadVertices[0].Y = (-widthSin + (-widthCos)) -1;
	quadVertices[0].U = 1;
	quadVertices[0].V = 1;
	quadVertices[1].X = (widthCos - (-widthSin));
	quadVertices[1].Y = (widthSin + (-widthCos)) -1;
	quadVertices[1].U = 1+MotionTrackerTextureSize;
	quadVertices[1].V = 1;
	quadVertices[2].X = (widthCos - widthSin);
	quadVertices[2].Y = (widthSin + widthCos) -1;
	quadVertices[2].U = 1+MotionTrackerTextureSize;
	quadVertices[2].V = 1+MotionTrackerTextureSize;
	quadVertices[3].X = ((-widthCos) - widthSin);
	quadVertices[3].Y = ((-widthSin) + widthCos) -1;
	quadVertices[3].U = 1;							   
	quadVertices[3].V = 1+MotionTrackerTextureSize;

	/* clip to Y<=0 */
	YClipMotionTrackerVertices(&quadVertices[0],&quadVertices[1]);
	YClipMotionTrackerVertices(&quadVertices[1],&quadVertices[2]);
	YClipMotionTrackerVertices(&quadVertices[2],&quadVertices[3]);
	YClipMotionTrackerVertices(&quadVertices[3],&quadVertices[0]);

	/* translate into screen coords */
	quadVertices[0].X += MotionTrackerCentreX;
	quadVertices[1].X += MotionTrackerCentreX;
	quadVertices[2].X += MotionTrackerCentreX;
	quadVertices[3].X += MotionTrackerCentreX;
	quadVertices[0].Y += MotionTrackerCentreY;
	quadVertices[1].Y += MotionTrackerCentreY;
	quadVertices[2].Y += MotionTrackerCentreY;
	quadVertices[3].Y += MotionTrackerCentreY;
	
	/* dodgy offset 'cos I'm not x clipping */
	if (quadVertices[0].X==-1) quadVertices[0].X = 0;
	if (quadVertices[1].X==-1) quadVertices[1].X = 0;
	if (quadVertices[2].X==-1) quadVertices[2].X = 0;
	if (quadVertices[3].X==-1) quadVertices[3].X = 0;
		
	/* check u & v are >0 */
	if (quadVertices[0].V<0) quadVertices[0].V = 0;
	if (quadVertices[1].V<0) quadVertices[1].V = 0;
	if (quadVertices[2].V<0) quadVertices[2].V = 0;
	if (quadVertices[3].V<0) quadVertices[3].V = 0;

	if (quadVertices[0].U<0) quadVertices[0].U = 0;
	if (quadVertices[1].U<0) quadVertices[1].U = 0;
	if (quadVertices[2].U<0) quadVertices[2].U = 0;
	if (quadVertices[3].U<0) quadVertices[3].U = 0;

	D3D_HUD_Setup();
	D3D_HUDQuad_Output(HUDImageNumber,quadVertices,RGBALIGHT_MAKE(255,255,255,255));
	
	#if 1
	{
		HUDImageDesc imageDesc;

		imageDesc.ImageNumber = HUDImageNumber;
		imageDesc.Scale = MUL_FIXED(MotionTrackerScale*3,scanLineSize/2);
		imageDesc.TopLeftX = MotionTrackerCentreX - MUL_FIXED(motionTrackerScaledHalfWidth,scanLineSize);
		imageDesc.TopLeftY = MotionTrackerCentreY - MUL_FIXED(motionTrackerScaledHalfWidth,scanLineSize);
		imageDesc.TopLeftU = 1;
		imageDesc.TopLeftV = 132;
		imageDesc.Height = 64;
		imageDesc.Width = 128;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = 128;//255; //HUDTranslucencyLevel;

 		Draw_HUDImage(&imageDesc);
	}
	#endif

	/* KJL 16:14:29 30/01/98 - draw bottom bar of MT */
	{
		BlueBar.Translucency = 128;//255; //HUDTranslucencyLevel;
		Draw_HUDImage(&BlueBar);
	}
	
	D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_UNITS],17, -4, MARINE_HUD_FONT_MT_SMALL);	  
	D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_TENS],9, -4, MARINE_HUD_FONT_MT_SMALL);	  
	D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_HUNDREDS],-9, -4, MARINE_HUD_FONT_MT_BIG);
    D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_THOUSANDS],-25, -4,MARINE_HUD_FONT_MT_BIG);	
}

void D3D_BLTDrawCell(int X, int Y)
{
	HUDImageDesc imageDesc;

	imageDesc.ImageNumber = CellImageNumber;
	imageDesc.Scale = 65536;
	imageDesc.TopLeftX = X;
	imageDesc.TopLeftY = Y;
	imageDesc.TopLeftU = 0;
	imageDesc.TopLeftV = 0;
	imageDesc.Height = 19;
	imageDesc.Width = 256;
	imageDesc.Red = 128;
	imageDesc.Green = 128;
	imageDesc.Blue = 128;
	imageDesc.Translucency = 255;

 	Draw_HUDImage(&imageDesc);
}

void D3D_BLTMotionTrackerBlipToHUD(int x, int y, int brightness)
{
	HUDImageDesc imageDesc;
	//int screenX,screenY; /* in 16.16 */
	int frame;
	int motionTrackerScaledHalfWidth = MUL_FIXED(MotionTrackerScale*3,MotionTrackerHalfWidth/2);
    
	GLOBALASSERT(brightness<=65536);
	
	frame = (brightness*5)/65537;
	GLOBALASSERT(frame>=0 && frame<5);
	
    frame = 4 - frame; // frames bloody wrong way round
	imageDesc.ImageNumber = HUDImageNumber;
	imageDesc.Scale = MUL_FIXED(MotionTrackerScale*3,(brightness+ONE_FIXED)/4);
	imageDesc.TopLeftX = MotionTrackerCentreX - MUL_FIXED(MT_BlipWidth/2,imageDesc.Scale) + MUL_FIXED(x,motionTrackerScaledHalfWidth);
	imageDesc.TopLeftY = MotionTrackerCentreY - MUL_FIXED(MT_BlipHeight/2,imageDesc.Scale) - MUL_FIXED(y,motionTrackerScaledHalfWidth);
	imageDesc.TopLeftU = 227;
	imageDesc.TopLeftV = 187;
	imageDesc.Height = MT_BlipHeight;
	imageDesc.Width = MT_BlipWidth;
	{
		int trans = MUL_FIXED(brightness*2,HUDTranslucencyLevel);
		if (trans>255) trans = 255;
		imageDesc.Translucency = 255; //trans;
	}
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;
	if (imageDesc.TopLeftX<0) /* then we need to clip */
	{
		imageDesc.Width += imageDesc.TopLeftX;
		imageDesc.TopLeftU -= imageDesc.TopLeftX;
		imageDesc.TopLeftX = 0;
	}
	Draw_HUDImage(&imageDesc);
}

extern void D3D_BlitWhiteChar(int x, int y, unsigned char c)
{
	HUDImageDesc imageDesc;
	
//	if (c>='a' && c<='z') c-='a'-'A';

//	if (c<' ' || c>'_') return;
	if (c==' ') return;

	#if 0
	imageDesc.ImageNumber = HUDFontsImageNumber;

	imageDesc.TopLeftX = x;
	imageDesc.TopLeftY = y;
	imageDesc.TopLeftU = 1+((c-32)&15)*7;
	imageDesc.TopLeftV = 2+((c-32)>>4)*11;
	imageDesc.Height = 8;
	imageDesc.Width = 5;
	#else
	imageDesc.ImageNumber = AAFontImageNumber;

	imageDesc.TopLeftX = x;
	imageDesc.TopLeftY = y;
	imageDesc.TopLeftU = 1+((c-32)&15)*16;
	imageDesc.TopLeftV = 1+((c-32)>>4)*16;
	imageDesc.Height = 15;
	imageDesc.Width = 15;
	#endif
	imageDesc.Scale = ONE_FIXED;
	imageDesc.Translucency = 255;
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;

	Draw_HUDImage(&imageDesc);
} 

void D3D_DrawHUDFontCharacter(HUDCharDesc *charDescPtr)
{
	HUDImageDesc imageDesc;

  //	if (charDescPtr->Character<' ' || charDescPtr->Character>'_') return;
	if (charDescPtr->Character == ' ') return;

	imageDesc.ImageNumber = AAFontImageNumber;

	imageDesc.TopLeftX = charDescPtr->X-1;
	imageDesc.TopLeftY = charDescPtr->Y-1;
	imageDesc.TopLeftU = 0+((charDescPtr->Character-32)&15)*16;
	imageDesc.TopLeftV = 0+((charDescPtr->Character-32)>>4)*16;
	imageDesc.Height = HUD_FONT_HEIGHT+2;
	imageDesc.Width = HUD_FONT_WIDTH+2;

	imageDesc.Scale = ONE_FIXED;
	imageDesc.Translucency = charDescPtr->Alpha;
	imageDesc.Red = charDescPtr->Red;
	imageDesc.Green = charDescPtr->Green;
	imageDesc.Blue = charDescPtr->Blue;

	Draw_HUDImage(&imageDesc);
	
}
void D3D_DrawHUDDigit(HUDCharDesc *charDescPtr)
{
	HUDImageDesc imageDesc;

	imageDesc.ImageNumber = HUDFontsImageNumber;

	imageDesc.TopLeftX = charDescPtr->X;
	imageDesc.TopLeftY = charDescPtr->Y;

	if (charDescPtr->Character<8)
	{
		imageDesc.TopLeftU = 1+(charDescPtr->Character)*16;
		imageDesc.TopLeftV = 81;
	}
	else
	{
		imageDesc.TopLeftU = 1+(charDescPtr->Character-8)*16;
		imageDesc.TopLeftV = 81+24;
	}


	imageDesc.Height = HUD_DIGITAL_NUMBERS_HEIGHT;
	imageDesc.Width = HUD_DIGITAL_NUMBERS_WIDTH;
	imageDesc.Scale = ONE_FIXED;
	imageDesc.Translucency = charDescPtr->Alpha;
	imageDesc.Red = charDescPtr->Red;
	imageDesc.Green = charDescPtr->Green;
	imageDesc.Blue = charDescPtr->Blue;

	Draw_HUDImage(&imageDesc);
	
}

void D3D_DrawHUDPredatorDigit(HUDCharDesc *charDescPtr, int scale)
{
	HUDImageDesc imageDesc;

	imageDesc.ImageNumber = PredatorNumbersImageNumber;

	imageDesc.TopLeftX = charDescPtr->X;
	imageDesc.TopLeftY = charDescPtr->Y;

	if (charDescPtr->Character<5)
	{
		imageDesc.TopLeftU = (charDescPtr->Character)*51;
		imageDesc.TopLeftV = 1;
	}
	else
	{
		imageDesc.TopLeftU = (charDescPtr->Character-5)*51;
		imageDesc.TopLeftV = 52;
	}


	imageDesc.Height = 50;
	imageDesc.Width = 50;
	imageDesc.Scale = scale;
	imageDesc.Translucency = charDescPtr->Alpha;
	imageDesc.Red = charDescPtr->Red;
	imageDesc.Green = charDescPtr->Green;
	imageDesc.Blue = charDescPtr->Blue;

	Draw_HUDImage(&imageDesc);
	
}

void D3D_BLTDigitToHUD(char digit, int x, int y, int font)
{
	HUDImageDesc imageDesc;
	struct HUDFontDescTag *FontDescPtr;
	int gfxID;

	switch (font)
	{
		case MARINE_HUD_FONT_MT_SMALL:
	  	case MARINE_HUD_FONT_MT_BIG:
		{
		   	gfxID = MARINE_HUD_GFX_TRACKERFONT;
			imageDesc.Scale = MotionTrackerScale;
			x = MUL_FIXED(x,MotionTrackerScale) + MotionTrackerCentreX;
			y = MUL_FIXED(y,MotionTrackerScale) + MotionTrackerCentreY;
			break;
		}
		case MARINE_HUD_FONT_RED:
		case MARINE_HUD_FONT_BLUE:
		{
			if (x<0) x+=ScreenDescriptorBlock.SDB_Width;
		   	gfxID = MARINE_HUD_GFX_NUMERALS;
			imageDesc.Scale=ONE_FIXED;
			break;
		}
		case ALIEN_HUD_FONT:
		{
			gfxID = ALIEN_HUD_GFX_NUMBERS;
			imageDesc.Scale=ONE_FIXED;
			break;
		}
		default:
			LOCALASSERT(0);
			break;
	}

	
	if (HUDResolution == HUD_RES_LO)
	{
		FontDescPtr = &HUDFontDesc[font];
	}
	else if (HUDResolution == HUD_RES_MED)
	{
		FontDescPtr = &HUDFontDesc[font];
	}
	else
	{
		FontDescPtr = &HUDFontDesc[font];
	}



	imageDesc.ImageNumber = HUDImageNumber;
	imageDesc.TopLeftX = x;
	imageDesc.TopLeftY = y;
	imageDesc.TopLeftU = FontDescPtr->XOffset;
	imageDesc.TopLeftV = digit*(FontDescPtr->Height+1)+1;
	
	imageDesc.Height = FontDescPtr->Height;
	imageDesc.Width = FontDescPtr->Width;
	imageDesc.Translucency = 255;
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;

	Draw_HUDImage(&imageDesc);

}

void D3D_BLTTargetingSightToHUD(int screenX, int screenY)
{
	HUDImageDesc imageDesc;
	int gunsightSize=128;

	screenX = (screenX-(gunsightSize/2));
	screenY = (screenY-(gunsightSize/2));

	imageDesc.ImageNumber = TargetingImageNumber;
	imageDesc.TopLeftX = screenX;
	imageDesc.TopLeftY = screenY;
	imageDesc.TopLeftU = 0;
	imageDesc.TopLeftV = 0;
	imageDesc.Height = gunsightSize;
	imageDesc.Width = gunsightSize;
	imageDesc.Scale = ONE_FIXED;
	imageDesc.Translucency = 128;
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;

	Draw_HUDImage(&imageDesc);
}

void D3D_BLTGunSightToHUD(int screenX, int screenY, enum GUNSIGHT_SHAPE gunsightShape)
{
  	HUDImageDesc imageDesc;
	int gunsightSize=13;

	screenX = (screenX-(gunsightSize/2));
  	screenY = (screenY-(gunsightSize/2));
  	
	imageDesc.ImageNumber = HUDImageNumber;
	imageDesc.TopLeftX = screenX;
	imageDesc.TopLeftY = screenY;
	imageDesc.TopLeftU = 227;
	imageDesc.TopLeftV = 131+(gunsightShape*(gunsightSize+1));
	imageDesc.Height = gunsightSize;
	imageDesc.Width = gunsightSize;
	imageDesc.Scale = ONE_FIXED;

	imageDesc.Translucency = 128;
	imageDesc.Green = 255;
	imageDesc.Red = 255;
	imageDesc.Blue = 255;

	Draw_HUDImage(&imageDesc);

	if (Operable)
	{
		if (Operable==1) // No Operate
			imageDesc.ImageNumber = NoOperateImageNumber;
		else if (Operable == 2) // Operate
			imageDesc.ImageNumber = OperateImageNumber;
		else	// Com-Tech Operate
			imageDesc.ImageNumber = ComTechOperateImageNumber;

		imageDesc.TopLeftX = (ScreenDescriptorBlock.SDB_Width/2)-32;
		imageDesc.TopLeftY = (ScreenDescriptorBlock.SDB_Height/2)-32;
		imageDesc.TopLeftU = 0;
		imageDesc.TopLeftV = 0;
		imageDesc.Height = 128;
		imageDesc.Width = 128;
		imageDesc.Translucency = 128;
		imageDesc.Green = 255;
		imageDesc.Red = 255;
		imageDesc.Blue = 255;
		imageDesc.Scale = ONE_FIXED/2;

		Draw_HUDImage(&imageDesc);
	}
}

void LoadBackdropImage(void)
{
#if 1
	extern int BackdropImage;
	extern char LevelName[];
	if (!strcmp(LevelName,"pred03"))
	  	BackdropImage = CL_LoadImageOnce("Envrnmts\\Pred03\\backdrop.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
#endif
}

void Render_HealthAndArmour(unsigned int health, unsigned int armour, unsigned int flares, unsigned int type)
{
	//HUDCharDesc charDesc;
	int i=MAX_NO_OF_COMMON_HUD_DIGITS;
	unsigned int healthColour;
	unsigned int armourColour;
	unsigned int flareColour;

	if (AvP.PlayerType == I_Marine)
	{										  
		int xCentre = MUL_FIXED(HUDLayout_RightmostTextCentre,HUDScaleFactor)+ScreenDescriptorBlock.SDB_Width;
		healthColour = HUDLayout_Colour_BrightWhite;
		armourColour = HUDLayout_Colour_BrightWhite;
		flareColour = HUDLayout_Colour_BrightWhite;

		D3D_BLTNewIconsToHUD
		(
			0,	//Health
			xCentre,
			MUL_FIXED(HUDLayout_Health_TopY,HUDScaleFactor)
		);
		D3D_RenderHUDNumber_Centred
		(
			health,
			(xCentre)-30,
			MUL_FIXED(HUDLayout_Health_TopY+3,HUDScaleFactor),
			healthColour
		);
		if (armour)
		{
			D3D_BLTNewIconsToHUD
			(
				1, //Armor
				xCentre,
				MUL_FIXED(HUDLayout_Armour_TopY,HUDScaleFactor)
			);
		
			D3D_RenderHUDNumber_Centred
			(
				armour,
				(xCentre)-30,
				MUL_FIXED(HUDLayout_Armour_TopY+3,HUDScaleFactor),
				armourColour
			);
		}
		if (flares) {
			D3D_BLTNewIconsToHUD
			(
				2,	//Flares
				xCentre,
				MUL_FIXED(HUDLayout_Flares_TopY,HUDScaleFactor)
			);
			D3D_RenderHUDNumber_Centred
			(
				flares,
				(xCentre)-30,
				MUL_FIXED(HUDLayout_Flares_TopY+3,HUDScaleFactor),
				flareColour
			);
		}
		// Survey Charge digits.
		{
			PLAYER_STATUS *psPtr = (PLAYER_STATUS *) Player->ObStrategyBlock->SBdataptr;
			PLAYER_WEAPON_DATA *pwPtr = &psPtr->WeaponSlot[psPtr->SelectedWeaponSlot];

			if (pwPtr->WeaponIDNumber == WEAPON_MINIGUN)
			{
				switch(ThisDiscMode)
				{
					case 4:	// disabled
						D3D_RenderHUDNumber_Centred(0,(ScreenDescriptorBlock.SDB_Width/2)+40,(ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,24))-150,0xffff0000);
						break;
					case I_Seek_Track:	// 10 sec
						D3D_RenderHUDNumber_Centred(10,(ScreenDescriptorBlock.SDB_Width/2)+40,(ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,24))-150,0xffff0000);
						break;
					case I_Search_Destroy:	// 20 sec
						D3D_RenderHUDNumber_Centred(20,(ScreenDescriptorBlock.SDB_Width/2)+40,(ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,24))-150,0xffff0000);
						break;
					case I_Proximity_Mine:	// 30 sec
						D3D_RenderHUDNumber_Centred(30,(ScreenDescriptorBlock.SDB_Width/2)+40,(ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,24))-150,0xffff0000);
						break;
				}
			}
		}

		if (GrabbedPlayer)
		{
			D3D_RenderHUDString_Centred
			(
				"Press Operate to struggle free.",
				(ScreenDescriptorBlock.SDB_Width/2),
				(ScreenDescriptorBlock.SDB_Height/2),
				HUDLayout_Colour_BrightWhite
			);
		}
		// Struggle success indicator
		if (StrugglePool)
		{
			unsigned int h;
			{
				h = (StrugglePool/ONE_FIXED);

				r2rect rectangle
				(
					(ScreenDescriptorBlock.SDB_Width/2)-250,
					(ScreenDescriptorBlock.SDB_Height/2)+40,
					((ScreenDescriptorBlock.SDB_Width/2)-250)+(h*10),
					(ScreenDescriptorBlock.SDB_Height/2)+52
				);
				rectangle . AlphaFill
				(
					0xff, // unsigned char R,
					0xff,// unsigned char G,
					0x00,// unsigned char B,
		   			128 // unsigned char translucency
				);
			}
		}
		// Hacking success indicator
		if (HackPool)
		{
			unsigned int h;
			{
				h = HackPool;

				r2rect rectangle
				(
					(ScreenDescriptorBlock.SDB_Width/2)-50,
					(ScreenDescriptorBlock.SDB_Height/2)+40,
					((ScreenDescriptorBlock.SDB_Width/2)-50)+(h*10),
					(ScreenDescriptorBlock.SDB_Height/2)+52
				);
				rectangle . AlphaFill
				(
					0x00, // unsigned char R,
					0xff,// unsigned char G,
					0x00,// unsigned char B,
		   			128 // unsigned char translucency
				);
			}
		}
		// Welding success indicator
		if (WeldPool)
		{
			unsigned int h;
			{
				h = WeldPool;

				r2rect rectangle
				(
					(ScreenDescriptorBlock.SDB_Width/2)-50,
					(ScreenDescriptorBlock.SDB_Height/2)+40,
					((ScreenDescriptorBlock.SDB_Width/2)-50)+(h*10),
					(ScreenDescriptorBlock.SDB_Height/2)+52
				);
				if (!WeldMode)
				{
					rectangle . AlphaFill
					(
						0x00, // unsigned char R,
						0x00,// unsigned char G,
						0xff,// unsigned char B,
		   				128 // unsigned char translucency
					);
				} else {
					rectangle . AlphaFill
					(
						0xff,
						0x00,
						0x00,
						128
					);
				}
			}
		}
	}
	else
	{
		if (GrabPlayer)
		{
			D3D_RenderHUDString_Centred
			(
				"Primary Fire to kill grabbed victim. Secondary Fire to release.",
				(ScreenDescriptorBlock.SDB_Width/2),
				(ScreenDescriptorBlock.SDB_Height)-100,
				HUDLayout_Colour_BrightWhite
			);
		}

		if (health>100)
		{
			healthColour = HUDLayout_Colour_BrightWhite;
		}
		else
		{
			int r = ((health)*128)/100;
			healthColour = 0xff000000 + ((128-r)<<16) + (r<<8);
		}
		if (armour>100)
		{
			armourColour = HUDLayout_Colour_BrightWhite;
		}
		else
		{
			int r = ((armour)*128)/100;
			armourColour = 0xff000000 + ((128-r)<<16) + (r<<8);
		}

		{
		
   			struct VertexTag quadVertices[4];
			int scaledWidth;
			int scaledHeight;
			int x,y;

			if (health<100)
			{
				scaledWidth = WideMulNarrowDiv(ScreenDescriptorBlock.SDB_Width,health,100);
				scaledHeight = scaledWidth/32;
			}
			else
			{
				scaledWidth = ScreenDescriptorBlock.SDB_Width;
				scaledHeight = scaledWidth/32;
			}
			x = (ScreenDescriptorBlock.SDB_Width - scaledWidth)/2;
			y = ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_Width/32 + x/32;

			quadVertices[0].U = 8;
			quadVertices[0].V = 5;
			quadVertices[1].U = 57;//255;
			quadVertices[1].V = 5;
			quadVertices[2].U = 57;//255;
			quadVertices[2].V = 55;//255;
			quadVertices[3].U = 8;
			quadVertices[3].V = 55;//255;
			
			quadVertices[0].X = x;
			quadVertices[0].Y = y;
			quadVertices[1].X = x + scaledWidth;
			quadVertices[1].Y = y;
			quadVertices[2].X = x + scaledWidth;
			quadVertices[2].Y = y + scaledHeight;
			quadVertices[3].X = x;
			quadVertices[3].Y = y + scaledHeight;
				
			D3D_HUDQuad_Output
			(
				SpecialFXImageNumber,// AlienEnergyBarImageNumber,
				quadVertices,
				0x20fd4817
			);
		
			health = (health/2);
			if (health<0) health=0;

			if (health<100)
			{
				scaledWidth = WideMulNarrowDiv(ScreenDescriptorBlock.SDB_Width,health,100);
				scaledHeight = scaledWidth/32;
			}
			else
			{
				scaledWidth = ScreenDescriptorBlock.SDB_Width;
				scaledHeight = scaledWidth/32;
			}
	
			x = (ScreenDescriptorBlock.SDB_Width - scaledWidth)/2;
			y = ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_Width/32 + x/32;
	
			quadVertices[0].X = x;
			quadVertices[0].Y = y;
			quadVertices[1].X = x + scaledWidth;
			quadVertices[1].Y = y;
			quadVertices[2].X = x + scaledWidth;
			quadVertices[2].Y = y + scaledHeight;
			quadVertices[3].X = x;
			quadVertices[3].Y = y + scaledHeight;
	
			D3D_HUDQuad_Output
			(
				SpecialFXImageNumber,// AlienEnergyBarImageNumber,
				quadVertices,
				0x20ff0000
			);
			
		}

	}	
}
void Render_MarineAmmo(enum TEXTSTRING_ID ammoText, enum TEXTSTRING_ID magazinesText, unsigned int magazines, enum TEXTSTRING_ID roundsText, unsigned int rounds, int primaryAmmo)
{
	//HUDCharDesc charDesc;
	PLAYER_STATUS *psPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr;
	int i=MAX_NO_OF_COMMON_HUD_DIGITS;
	int xCentre = MUL_FIXED(HUDLayout_RightmostTextCentre,HUDScaleFactor)+ScreenDescriptorBlock.SDB_Width;

	LOCALASSERT(psPtr);

	weaponPtr = &(psPtr->WeaponSlot[psPtr->SelectedWeaponSlot]);
	if(!primaryAmmo) xCentre+=MUL_FIXED(HUDScaleFactor,HUDLayout_RightmostTextCentre*2);

#if 0
	/* Display weapon */
	if (primaryAmmo) {
		D3D_RenderHUDString_Centred
		(
			"",
			xCentre,
			ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,HUDLayout_AmmoDesc_TopY),
			HUDLayout_Colour_MarineGreen
		);
	}
#endif
	/* Electronic Bypass Kit and Hand Welder will not display any mags */
	if (weaponPtr->WeaponIDNumber == WEAPON_AUTOSHOTGUN ||
		weaponPtr->WeaponIDNumber == WEAPON_PLASMAGUN)
	{
		if (weaponPtr->WeaponIDNumber == WEAPON_PLASMAGUN)
		{
			if (!WeldMode)
			{
				D3D_RenderHUDString_Centred
				(
					"Weld Mode",
					xCentre,
					ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY),
					HUDLayout_Colour_MarineGreen
				);
			} else {
				D3D_RenderHUDString_Centred
				(
					"Cut Mode",
					xCentre,
					ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY),
					HUDLayout_Colour_MarineGreen
				);
			}
		}
		return;
	}
#if 0
	/* Display magazines */
	if (weaponPtr->WeaponIDNumber == WEAPON_PULSERIFLE ||
		weaponPtr->WeaponIDNumber == WEAPON_MARINE_PISTOL ||
		weaponPtr->WeaponIDNumber == WEAPON_GRENADELAUNCHER ||
		weaponPtr->WeaponIDNumber == WEAPON_SADAR) {
		if (primaryAmmo) {
			D3D_RenderHUDString_Centred
			(
				GetTextString(magazinesText),
				xCentre,
				ScreenDescriptorBlock.SDB_Height -MUL_FIXED(HUDScaleFactor, HUDLayout_Magazines_TopY),
				HUDLayout_Colour_MarineGreen
			);
		}
	}
#endif
	/* Replace magazine to AFTER rounds. */
	if (weaponPtr->WeaponIDNumber == WEAPON_PULSERIFLE ||
		weaponPtr->WeaponIDNumber == WEAPON_MARINE_PISTOL ||
		weaponPtr->WeaponIDNumber == WEAPON_GRENADELAUNCHER ||
		weaponPtr->WeaponIDNumber == WEAPON_SADAR) {
		if (primaryAmmo) {
			D3D_RenderHUDNumber_Centred
			(
				magazines,
				xCentre+25,
				ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY - HUDLayout_Linespacing),
				HUDLayout_Colour_MarineGreen
			);
		}
	}
	/* Display rounds */
	D3D_RenderHUDString_Centred
	(
		GetTextString(ammoText),
		xCentre,
		ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY),
		HUDLayout_Colour_MarineGreen
	);
	D3D_RenderHUDNumber_Centred
	(
		rounds,
		xCentre-25,
		ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY - HUDLayout_Linespacing),
		HUDLayout_Colour_BrightWhite
	);

	if ((AvP.Network != I_No_Network) && (primaryAmmo))
	{
		xCentre = xCentre-50;
	/* Display radio stuff. */
	D3D_RenderHUDString
	("[1] Backup     ", xCentre, ScreenDescriptorBlock.SDB_Height-MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY+100),HUDLayout_Colour_BrightWhite);
	D3D_RenderHUDString
	("[2] Enemy      ", xCentre, ScreenDescriptorBlock.SDB_Height-MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY+90),HUDLayout_Colour_BrightWhite);
	D3D_RenderHUDString
	("[3] Secure     ", xCentre, ScreenDescriptorBlock.SDB_Height-MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY+80),HUDLayout_Colour_BrightWhite);
	D3D_RenderHUDString
	("[4] Acknowledge", xCentre, ScreenDescriptorBlock.SDB_Height-MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY+70),HUDLayout_Colour_BrightWhite);
	D3D_RenderHUDString
	("[5] Disregard  ", xCentre, ScreenDescriptorBlock.SDB_Height-MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY+60),HUDLayout_Colour_BrightWhite);
	D3D_RenderHUDString
	("[6] Medic      ", xCentre, ScreenDescriptorBlock.SDB_Height-MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY+50),HUDLayout_Colour_BrightWhite);
	D3D_RenderHUDString
	("[7] Movement   ", xCentre, ScreenDescriptorBlock.SDB_Height-MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY+40),HUDLayout_Colour_BrightWhite);
	D3D_RenderHUDString
	("[8] Bypass     ", xCentre, ScreenDescriptorBlock.SDB_Height-MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY+30),HUDLayout_Colour_BrightWhite);
	D3D_RenderHUDString
	("[9] Trackers   ", xCentre, ScreenDescriptorBlock.SDB_Height-MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY+20),HUDLayout_Colour_BrightWhite);
	}
} 
void DrawPredatorEnergyBar(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
	int maxHeight = ScreenDescriptorBlock.SDB_Height*3/4;
	int h;
	{
		h = MUL_FIXED(DIV_FIXED(playerStatusPtr->FieldCharge,PLAYERCLOAK_MAXENERGY),maxHeight);
		
		r2rect rectangle
		(
			ScreenDescriptorBlock.SDB_Width+HUDLayout_RightmostTextCentre*3/2,
			ScreenDescriptorBlock.SDB_Height-h,
			ScreenDescriptorBlock.SDB_Width+HUDLayout_RightmostTextCentre/2,
			ScreenDescriptorBlock.SDB_Height
			
		);
		rectangle . AlphaFill
		(
			0xff, // unsigned char R,
			0x00,// unsigned char G,
			0x00,// unsigned char B,
		   	128 // unsigned char translucency
		);
	}
	if (weaponPtr->WeaponIDNumber == WEAPON_PRED_SHOULDERCANNON)
	{
		h = MUL_FIXED(playerStatusPtr->PlasmaCasterCharge,maxHeight);
			
		r2rect rectangle
		(
			ScreenDescriptorBlock.SDB_Width+HUDLayout_RightmostTextCentre*3,
			ScreenDescriptorBlock.SDB_Height-h,
			ScreenDescriptorBlock.SDB_Width+HUDLayout_RightmostTextCentre*2,
			ScreenDescriptorBlock.SDB_Height
			
		);
		rectangle . AlphaFill
		(
			0x00, // unsigned char R,
			0xff,// unsigned char G,
			0xff,// unsigned char B,
		   	128 // unsigned char translucency
		);
	}

}

};
