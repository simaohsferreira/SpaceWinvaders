#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "../DLL/dll.h"
#include "../Client/debug.h"

//DWORD WINAPI simulClient(LPVOID tParam) {
//
//	int			*ThreadMustGoOn = &((SMCtrl*)tParam)->ThreadMustGoOn;
//	HANDLE		*hSMGatewayUpdate = ((SMCtrl*)tParam)->hSMGatewayUpdate;
//	SMMessage	*message = ((SMCtrl*)tParam)->pSMemMessage;
//
//	srand((unsigned)time(NULL));
//
//	message->buffer[0].owner = 0;
//
//	while (*ThreadMustGoOn) {
//		message->buffer[0].instruction = rand() % 4;
//		SetEvent(hSMGatewayUpdate);
//		Sleep(1000 * (*ThreadMustGoOn));
//	}
//
//	return 0;
//}

void simulClient(packet * localpacket) {

	srand((unsigned)time(NULL));

	localpacket->owner = 0;

	localpacket->instruction = rand() % 4;

	Sleep(500);
	return;
}

DWORD WINAPI sendMessage(LPVOID tParam) {

	int			*ThreadMustGoOn = &((SMCtrl*)tParam)->ThreadMustGoOn;		//Exit condition
	HANDLE		*hSMGatewayUpdate = ((SMCtrl*)tParam)->hSMGatewayUpdate;	//Event for updates #### maybe not needed ###
	HANDLE		*mhProdConsMut = ((SMCtrl*)tParam)->mhProdConsMut;			//Mutex to grant buffer exclusivity
	HANDLE		*shVacant = ((SMCtrl*)tParam)->shVacant;					//Semaphore to count vacant places
	HANDLE		*shOccupied = ((SMCtrl*)tParam)->shOccupied;				//Semaphore to count occupied places
	SMMessage	*message = ((SMCtrl*)tParam)->pSMemMessage;
	packet		*buffer = ((SMCtrl*)tParam)->pSMemMessage->buffer;

	packet		localpacket;

	int nextIn = 0;

	

	while (*ThreadMustGoOn) {

		//Produces item
		simulClient(&localpacket);

		//Puts it in buffer
		WaitForSingleObject(shVacant,INFINITE);

		WaitForSingleObject(mhProdConsMut, INFINITE);

		buffer[nextIn] = localpacket;

		nextIn = (nextIn + 1) % SMEM_BUFF;

		ReleaseMutex(mhProdConsMut);

		ReleaseSemaphore(shOccupied, 1, NULL);

	}

	return 0;
}

DWORD WINAPI ReadServerMsg(LPVOID tParam) {	

	SMCtrl		*cThread = (SMCtrl*)tParam;
	HANDLE		hStdout = GetStdHandle(STD_OUTPUT_HANDLE); //Handle to stdout to clear screen ##DELETE-ME after May 12th##
	SMGameData	*gameMsg;

	gameMsg = malloc(sizeof(SMGameData));
	int i;

	cls(hStdout);
	hidecursor();

	while (cThread->ThreadMustGoOn) {

		WaitForSingleObject(cThread->hSMServerUpdate, INFINITE);
		WaitForSingleObject(cThread->mhStructSync, INFINITE);
		CopyMemory(gameMsg, cThread->pSMemGameData, sizeof(SMGameData));
		ReleaseMutex(cThread->mhStructSync);

		cls(hStdout);
		for (i = 0; i < MAX_INVADER; i++) {
			if (gameMsg->invad[i].hp) {
				gotoxy(gameMsg->invad[i].x, gameMsg->invad[i].y);
				if (gameMsg->invad[i].rand_path)
					_tprintf(TEXT("X"));
				else
					_tprintf(TEXT("W"));
			}
		}

		if (cThread->pSMemGameData->bomb[0].y < 25) { //this needs another aproach (fired state?)
			gotoxy(gameMsg->bomb[0].x, gameMsg->bomb[0].y);
			_tprintf(TEXT("o"));
		}

		for (i = 0; i < MAX_PLAYERS; i++) {
			gotoxy(gameMsg->ship[i].x, gameMsg->ship[i].y);
			_tprintf(TEXT("�"));
		}
	}

	return 0;
}

int _tmain(int argc, LPTSTR argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	SMCtrl		cThread;
	HANDLE		hCanBootNow;

	HANDLE		htSReadMsg;
	DWORD		tRSMsgID;

	HANDLE		htSimulClient;
	DWORD		tSimulClientID;

	HANDLE		hStdout = GetStdHandle(STD_OUTPUT_HANDLE); //Handle to stdout to clear screen ##DELETE-ME after May 12th##

	SYSTEM_INFO	SysInfo;
	DWORD		dwSysGran;

	GetSystemInfo(&SysInfo);									//Used to get system granularity
	dwSysGran = SysInfo.dwAllocationGranularity;				//Used to get system granularity

	cThread.SMemViewServer.QuadPart		= ((sizeof(SMGameData) / dwSysGran)*dwSysGran) + dwSysGran;
	cThread.SMemViewGateway.QuadPart	= ((sizeof(SMMessage) / dwSysGran)*dwSysGran) + dwSysGran;
	cThread.SMemSize.QuadPart			= cThread.SMemViewServer.QuadPart + cThread.SMemViewGateway.QuadPart;

	cThread.ThreadMustGoOn = 1;

	hCanBootNow = OpenEvent(EVENT_ALL_ACCESS, FALSE, EVE_BOOT);
	if (!hCanBootNow) {
		hCanBootNow = CreateEvent(NULL, FALSE, FALSE, EVE_BOOT);
		_tprintf(TEXT("Waiting for server to boot.\n"));
		WaitForSingleObject(hCanBootNow, INFINITE);
	}

	cThread.mhStructSync = OpenMutex(				
		MUTEX_ALL_ACCESS,							//Desired access
		FALSE,										//Inherit handle by child processes
		STRUCT_SYNC);								//Event name

	cThread.mhProdConsMut = OpenMutex(
		MUTEX_ALL_ACCESS,							//Desired access
		FALSE,										//Inherit handle by child processes
		MUT_PRODCONS);								//Event name

	cThread.shVacant = OpenSemaphore(		//It starts with full vacancies
		NULL,										//Desired access
		FALSE,										//Inherit handle by child processes
		SEM_VACANT);								//Semaphore name

	cThread.shOccupied = OpenSemaphore(		//It starts without occupation
		NULL,										//Desired access
		FALSE,										//Inherit handle by child processes
		SEM_OCCUPIED);								//Semaphore name

	cThread.hSMServerUpdate = OpenEvent(	//Opens the event to warn gateway that the shared memoy is mapped
		EVENT_ALL_ACCESS, 							//Desired access
		FALSE, 										//Inherit handle by child processes
		EVE_SERV_UP);								//Event name

	cThread.hSMGatewayUpdate = OpenEvent(	//Opens the event to warn server that the shared memoy is mapped
		EVENT_ALL_ACCESS, 							//Desired access
		FALSE, 										//Inherit handle by child processes
		EVE_GATE_UP);								//Event name

	//Opens a mapped file by the server
	if (sharedMemory(&cThread.hSMem, NULL) == -1) {
		_tprintf(TEXT("[Error] Opening file mapping (%d)\n"), GetLastError());
		return -1;
	}

	//Creates a view of the desired part <GameDataView>
	if (mapGameDataView(&cThread, FILE_MAP_READ) == -1) {		//Checks for errors
		_tprintf(TEXT("[Error] Mapping GameDataView (%d) at Gateway. Is the server running?\n"), GetLastError());
		return -1;
	}

	//Creates a view of the desired part <MsgView>
	if (mapMsgView(&cThread) == -1) {		//Checks for errors
		_tprintf(TEXT("[Error] Mapping MsgView (%d)\n at Gateway"), GetLastError());
		return -1;
	}

	htSReadMsg = CreateThread(
		NULL,					//Thread security attributes
		0,						//Stack size
		ReadServerMsg,			//Thread function name
		(LPVOID)&cThread,		//Thread parameter struct
		0,						//Creation flags
		&tRSMsgID);				//gets thread ID to close it afterwards

	htSimulClient = CreateThread(
		NULL,					//Thread security attributes
		0,						//Stack size
		sendMessage,			//Thread function name
		(LPVOID)&cThread,		//Thread parameter struct
		0,						//Creation flags
		&tSimulClientID);		//gets thread ID to close it afterwards

	WaitForSingleObject(htSReadMsg, INFINITE);

	UnmapViewOfFile(cThread.pSMemGameData);		//Unmaps view of shared memory
	UnmapViewOfFile(cThread.pSMemMessage);		//Unmaps view of shared memory
	CloseHandle(cThread.hSMem);

	return 0;
}