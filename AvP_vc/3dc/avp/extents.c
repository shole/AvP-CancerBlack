/* KJL 12:15:10 8/23/97 - extents.c
 *
 * This file contains the collision extents
 * data for the game characters.
 *
 */
#include "extents.h"

/*
 * Format:
 *
 * Collision Radius (mm)
 *
 * Bottom (Y-axis)
 *
 * StandingTop
 *
 * CrouchingTop
 *
 */

COLLISION_EXTENTS CollisionExtents[MAX_NO_OF_COLLISION_EXTENTS] =
{
	/* CE_MARINE */
	{450,0, -1950, -1200},

	/* CE_PREDATOR */
	{450,0, -2439, -1570},

	/* CE_ALIEN */
	{550,0, -2517, -1080},

	/* CE_XENOBORG */
	{450,0, -2439, -900},

	/* CE_PREDATORALIEN */
	{550,0, -2626, -1276},
						 
	/* CE_FACEHUGGER */
	{300,0, -1080, -1080},

	/* CE_QUEEN */
	{1300,0, -4000, -867},

	/* CE_CORPSE */
	{700,0, -200, -200},

	/* CE_APC */
	{2200,0, -2950, -1200},
};

