/* KJL 16:41:33 29/03/98 - not the most complicated code I've ever written... */
#include "version.h"
extern void NewOnScreenMessage(unsigned char *messagePtr);


void GiveVersionDetails(void)
{
	/* KJL 15:54:25 29/03/98 - give version details; this is not language localised since I thought that would be a little odd */
	NewOnScreenMessage("Cancer Black \nPublic Beta 1.1 \nBuild 5.3"); //11:25

}
