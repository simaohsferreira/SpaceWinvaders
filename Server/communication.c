#include "communication.h"

void consumePacket(SMCtrl *tParam, int *nextOut, Packet *localpacket) {

	SMCtrl		*cThread = (SMCtrl*)tParam;

	//wait occupied semaphore
	WaitForSingleObject(cThread->shOccupied, INFINITE);

	//wait mutex
	WaitForSingleObject(cThread->mhProdConsMut, INFINITE);

	//copy buffer[nextout] to local
	//CopyMemory(localpacket, &cThread->pSMemMessage->buffer[*nextOut], sizeof(localpacket));
	*localpacket = cThread->pSMemMessage->buffer[*nextOut];

	//nextout++
	*nextOut = (*nextOut + 1) % SMEM_BUFF;

	//release mutex
	ReleaseMutex(cThread->mhProdConsMut);

	//release semaphore vacant	
	ReleaseSemaphore(cThread->shVacant, 1, NULL);
}

DWORD WINAPI ReadGatewayMsg(LPVOID tParam) {
	SMCtrl		*cThread = (SMCtrl*)tParam;

	Packet		localpacket;
	
	Ship		localship;

	int	nextOut = 0;
	//int maxXpos = XSIZE - 1;
	//int maxYpos = YSIZE - 1;
	//int minYpos = YSIZE - (YSIZE*0.2);

	while (cThread->ThreadMustGoOn) {

		//Consume item from buffer
		consumePacket(tParam, &nextOut, &localpacket);

		WaitForSingleObject(cThread->mhStructSync, INFINITE);
		localship = cThread->pSMemGameData->ship[localpacket.owner];
		ReleaseMutex(cThread->mhStructSync);

		UpdateLocalShip(cThread->pSMemGameData, &localpacket, &localship);

		//validate action
		//switch (localpacket.instruction) {
		//case 0:
		//	if (localship.x<maxXpos)
		//		localship.x++;
		//	break;
		//case 1:
		//	if (localship.y<maxYpos)
		//		localship.y++;
		//	break;
		//case 2:
		//	if (localship.x>0)
		//		localship.x--;
		//	break;
		//case 3:
		//	if (localship.y<minYpos)
		//		localship.y--;
		//	break;
		//default:
		//	break;
		//}

		//this is updating the structure either way ## rethink ##
		//put it in respective place

		WaitForSingleObject(cThread->mhStructSync, INFINITE);
		cThread->pSMemGameData->ship[localpacket.owner].x = localship.x;
		cThread->pSMemGameData->ship[localpacket.owner].y = localship.y;
		ReleaseMutex(cThread->mhStructSync);
	}

	return 0;
}


int UpdateLocalShip(GameData *game, Packet *localpacket, Ship *localship) {

		//validate action
		switch (localpacket->instruction) {
		case 0:
			if (localship->x<(game->xsize - 1))
				localship->x++;
			break;
		case 1:
			if (localship->y<(game->ysize - 1))
				localship->y++;
			break;
		case 2:
			if (localship->x>0)
				localship->x--;
			break;
		case 3:
			if (localship->y<(game->ysize - (game->ysize*0.2)))
				localship->y--;
			break;
		default:
			break;
		}
	return 0;
}

DWORD WINAPI WriteGatewayMsg(LPVOID tParam) {
	/*
	Here we will recieve either a copy
	either a pointer to a Game or GameData 
	object to send it!
	*/
}