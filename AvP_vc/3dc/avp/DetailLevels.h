#ifndef _detaillevels_h_
#define _detaillevels_h_ 1
typedef struct
{
	unsigned int MaximumAllowedNumberOfDecals;
	unsigned int AlienEnergyViewThreshold;
	unsigned int NumberOfSmokeParticlesFromLargeExplosion;
	unsigned int NumberOfSmokeParticlesFromSmallExplosion;

	unsigned int BloodCollidesWithEnvironment :1;
	unsigned int DrawLightCoronas :1;
	unsigned int DrawHierarchicalDecals :1;
	unsigned int ExplosionsDeformToEnvironment :1;
	unsigned int GhostFlameThrowerCollisions :1;

	// Additions by Eldritch
	unsigned int ExplosionFX;
	unsigned int WeatherFX;
	unsigned int MuzzleSmoke;
	unsigned int Shells;
	unsigned int CenteredWeapons;

} DETAIL_LEVELS;

extern DETAIL_LEVELS LocalDetailLevels;
typedef struct
{
	int DecalNumber;
	int LightCoronas;
	int DecalsOnCharacters;
	int DeformableExplosions;
	int CharacterComplexity;
	int ParticleComplexity;

	// Additions by Eldritch
	int WeatherFX;
	int MuzzleSmoke;
	int Shells;
	int CenteredWeapons;

} MENU_DETAIL_LEVEL_OPTIONS;

extern MENU_DETAIL_LEVEL_OPTIONS MenuDetailLevelOptions;


extern void SetToDefaultDetailLevels(void);
extern void SetToMinimalDetailLevels(void);

extern void SetDetailLevelsFromMenu(void);

#endif