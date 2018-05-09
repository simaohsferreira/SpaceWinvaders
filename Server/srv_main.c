#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "../DLL/dll.h"

typedef struct {
	HANDLE			hTick;							//Handle to event. Warns gateway about updates in shared memory
	int				ThreadMustGoOn;		
	HANDLE			*mhInvader;
}GTickStruct;

DWORD WINAPI RegPathInvaders(LPVOID tParam) {

	int * ThreadMustGoOn = &((SMCtrl *)tParam)->ThreadMustGoOn;
	SMServer_MSG *lvl = ((SMCtrl *)tParam)->pSMemServer;
	HANDLE		*mhInvader = ((SMCtrl *)tParam)->mhInvader;

	int i, j;
	int sidestep = 4;

	while (*ThreadMustGoOn) {						//Thread main loop

		for (i = 0; (i < ((YSIZE-(MAX_INVADER/INVADER_BY_ROW)) * sidestep)) && *ThreadMustGoOn; i++) {

			WaitForSingleObject(mhInvader, INFINITE);

			for (j = 0; (j < (MAX_INVADER - RAND_INVADER)) && *ThreadMustGoOn; j++) {

				lvl->invad[j].y = (i / sidestep) + lvl->invad[j].y_init;				//Invader goes down after n sidesteps

				if ((i % (sidestep * 2)) < sidestep)
					lvl->invad[j].x = (i % (sidestep * 2)) + lvl->invad[j].x_init;		//Invader goes right
				else if ((i % (sidestep * 2)) > sidestep)
					lvl->invad[j].x--;													//Invader goes left
			}

			ReleaseMutex(mhInvader);

			Sleep(INVADER_SPEED*(*ThreadMustGoOn));
		}
	}
}

DWORD WINAPI RandPathInvaders(LPVOID tParam) {

	int * ThreadMustGoOn = &((SMCtrl *)tParam)->ThreadMustGoOn;
	SMServer_MSG *lvl = ((SMCtrl *)tParam)->pSMemServer;
	HANDLE		*mhInvader = ((SMCtrl *)tParam)->mhInvader;
	int i;

	while (*ThreadMustGoOn) {						//Thread main loop

		WaitForSingleObject(mhInvader, INFINITE);

		for (i = (MAX_INVADER - RAND_INVADER); (i < MAX_INVADER) && *ThreadMustGoOn; i++) {

			switch (rand() % 4) {
			case 0:
				if (lvl->invad[i].x > 0)
					lvl->invad[i].x--;
				else
					lvl->invad[i].x=1;
				break;
			case 1:
				if (lvl->invad[i].x < XSIZE-1)
					lvl->invad[i].x++;
				else
					lvl->invad[i].x= XSIZE-2;
				break;
			case 2:
				if (lvl->invad[i].y > 0)
					lvl->invad[i].y--;
				else
					lvl->invad[i].y=1;
				break;
			case 3:
				if (lvl->invad[i].y < YSIZE-1)
					lvl->invad[i].y++;
				else
					lvl->invad[i].y= YSIZE-2;
				break;
			}
		}

		ReleaseMutex(mhInvader);
		Sleep((INVADER_SPEED/4)*(*ThreadMustGoOn));
	}
}

DWORD WINAPI StartGame(LPVOID tParam) {

	int * ThreadMustGoOn = &((SMCtrl *)tParam)->ThreadMustGoOn;
	SMServer_MSG *lvl = ((SMCtrl *)tParam)->pSMemServer;

	DWORD			tRegPathInvaderID;
	HANDLE			htRegPathInvader;
	DWORD			tRandPathInvaderID;
	HANDLE			htRandPathInvader;

	int i;

	srand((unsigned)time(NULL));					//Seeds the RNG
	_tprintf(TEXT("\n %d\n"), rand());

	for (i = 0; (i < MAX_INVADER) && *ThreadMustGoOn; i++) {		//Defines invader path
		if (i < (MAX_INVADER-RAND_INVADER))
			lvl->invad[i].rand_path = 0;
		else
			lvl->invad[i].rand_path = 1;
	}

	for (i = 0; ((i < MAX_INVADER) && *ThreadMustGoOn); i++) {		//Populates invaders with coords

		if (!(lvl->invad[i].rand_path)) {							//If regular path
			
			//deploys INVADER_BY_ROW invaders per line with a spacing of 2
			lvl->invad[i].x = lvl->invad[i].x_init = (i % INVADER_BY_ROW) * 2;

			//Deploys 5 lines of invaders (MAX_INVADER/11=5)
			lvl->invad[i].y = lvl->invad[i].y_init = i / INVADER_BY_ROW;
		}
		else {
			lvl->invad[i].x = lvl->invad[i].x_init = rand() % XSIZE;
			lvl->invad[i].y = lvl->invad[i].y_init = rand() % YSIZE;
			_tprintf(TEXT("\nInvader no: %d\nX= %d\nY= %d\n%d\n"),i, lvl->invad[i].x, lvl->invad[i].y,rand());
		}
	}

	htRegPathInvader = CreateThread(
		NULL,										//Thread security attributes
		0,											//Stack size
		RegPathInvaders,							//Thread function name
		tParam,										//Thread parameter struct
		0,											//Creation flags
		&tRegPathInvaderID);						//gets thread ID to close it afterwards

	htRandPathInvader = CreateThread(
		NULL,										//Thread security attributes
		0,											//Stack size
		RandPathInvaders,							//Thread function name
		tParam,										//Thread parameter struct
		0,											//Creation flags
		&tRandPathInvaderID);						//gets thread ID to close it afterwards

	WaitForSingleObject(htRegPathInvader,INFINITE);
	WaitForSingleObject(htRandPathInvader, INFINITE);
}

DWORD WINAPI GameTick(LPVOID tParam) {				//Warns gateway of structure updates
	
	GTickStruct		*sGTick;
	sGTick = (GTickStruct*)tParam;

	while (sGTick->ThreadMustGoOn) {

		Sleep(100);
		_tprintf(TEXT("."));
		WaitForSingleObject(sGTick->mhInvader, INFINITE);
		SetEvent(sGTick->hTick);
		ReleaseMutex(sGTick->mhInvader);
	}
}

DWORD WINAPI ReadGatewayMsg(LPVOID tParam) {		//Warns gateway of structure updates

	int * ThreadMustGoOn = &((SMCtrl*)tParam)->ThreadMustGoOn;
	HANDLE * hSMGatewayUpdate = ((SMCtrl*)tParam)->hSMGatewayUpdate;

	while (*ThreadMustGoOn) {
		WaitForSingleObject(hSMGatewayUpdate, INFINITE);
		_tprintf(TEXT(" g "));
	}
}

int _tmain(int argc, LPTSTR argv[]) {

	#ifdef UNICODE									//Sets console to unicode
		_setmode(_fileno(stdin), _O_WTEXT);
		_setmode(_fileno(stdout), _O_WTEXT);
	#endif
	
	SMCtrl			cThread;						//Thread parameter structure
	HANDLE			hCanBootNow;					//Handle to event. Warns the gateway the shared memory is mapped
	DWORD			tGameID;						//stores the ID of the game thread
	HANDLE			htGame;							//Handle to the game thread

	GTickStruct		sGTick;
	HANDLE			htGTick;
	HANDLE			htGReadMsg;
	DWORD			tGTickID;
	DWORD			tRGMsgID;

	SYSTEM_INFO		SysInfo;
	DWORD			dwSysGran;

	GetSystemInfo(&SysInfo);						//Used to get system granularity
	dwSysGran = SysInfo.dwAllocationGranularity;	//Used to get system granularity

	cThread.SMemViewServer.QuadPart = ((sizeof(SMServer_MSG) / dwSysGran)*dwSysGran) + dwSysGran;
	cThread.SMemViewGateway.QuadPart = ((sizeof(SMGateway_MSG) / dwSysGran)*dwSysGran) + dwSysGran;
	cThread.SMemSize.QuadPart = cThread.SMemViewServer.QuadPart + cThread.SMemViewGateway.QuadPart;

	//#######################################################################################################################
	//#####################################GRANULARITY TESTS//DELETE THIS####################################################
	//#######################################################################################################################
	_tprintf(TEXT("Sysgran: %d bytes\nSize of servstruct: %d\nSize of gateway: %d\n"), dwSysGran, sizeof(SMServer_MSG), sizeof(SMGateway_MSG));
	_tprintf(TEXT("ServerView:\t((%d/%d)*%d)+%d=%d\n"), sizeof(SMServer_MSG), dwSysGran, dwSysGran, dwSysGran, ((sizeof(SMServer_MSG) / dwSysGran)*dwSysGran) + dwSysGran);
	_tprintf(TEXT("GatewayView:\t((%d/%d)*%d)+%d=%d\n"), sizeof(SMGateway_MSG),dwSysGran,dwSysGran, dwSysGran, ((sizeof(SMGateway_MSG) / dwSysGran)*dwSysGran) + dwSysGran);
	_tprintf(TEXT("TestBigView:\t((%d/%d)*%d)+%d=%d\n"), 66000, dwSysGran, dwSysGran, dwSysGran, ((66000 / dwSysGran)*dwSysGran) + dwSysGran);
	//#######################################################################################################################
	//#######################################################################################################################
	//#######################################################################################################################

	cThread.ThreadMustGoOn = 1;						//Preps thread to run position
	sGTick.ThreadMustGoOn = 1;

	cThread.mhInvader = CreateMutex(				//This a test
		NULL,										//Security attributes
		FALSE,										//Initial owner
		NULL);										//Mutex name

	sGTick.mhInvader = cThread.mhInvader;

	hCanBootNow = CreateEvent(						//Creates the event to warn gateway that the shared memoy is mapped
		NULL,										//Event attributes
		FALSE,										//Manual reset (TRUE for auto-reset)
		FALSE,										//Initial state
		TEXT("LetsBoot"));							//Event name

	cThread.hSMServerUpdate = CreateEvent(			//Creates the event to warn gateway that the shared memoy is mapped
		NULL, 										//Event attributes
		FALSE, 										//Manual reset (TRUE for auto-reset)
		FALSE, 										//Initial state
		TEXT("SMServerUpdate"));					//Event name

	cThread.hSMGatewayUpdate = CreateEvent(			//Creates the event to warn gateway that the shared memoy is mapped
		NULL, 										//Event attributes
		FALSE, 										//Manual reset (TRUE for auto-reset)
		FALSE, 										//Initial state
		TEXT("SMGatewayUpdate"));					//Event name

	sGTick.hTick = cThread.hSMServerUpdate;

	//Creates a mapped file
	if(sharedMemory(&cThread.hSMem,&cThread.SMemSize)==-1){
		_tprintf(TEXT("[Error] Opening file mapping (%d)\n"), GetLastError());
		return -1;
	}

	//Creates a view of the desired part <Server>
	mapServerView(&cThread);
	if (cThread.pSMemServer == NULL) {				//Checks for errors
		_tprintf(TEXT("[Error] Mapping server view (%d)\n"), GetLastError());
		return -1;
	}

	//Creates a view of the desired part <Gateway>
	mapGatewayView(&cThread);
	if (cThread.pSMemGateway== NULL) {				//Checks for errors
		_tprintf(TEXT("[Error] Mapping gateway view (%d)\n"), GetLastError());
		return -1;
	}

	SetEvent(hCanBootNow);							//Warns gateway that Shared memory is mapped

	//Launches game tick thread
	_tprintf(TEXT("Launching game tick thread...\n"));

	htGTick = CreateThread(
		NULL,										//Thread security attributes
		0,											//Stack size (0 for default)
		GameTick,									//Thread function name
		(LPVOID)&sGTick,							//Thread parameter struct
		0,											//Creation flags
		&tGTickID);									//gets thread ID to close it afterwards

	//Launches gateway message receiver thread
	_tprintf(TEXT("Launching gateway message receiver thread...\n"));
	htGReadMsg = CreateThread(
		NULL,										//Thread security attributes
		0,											//Stack size (0 for default)
		ReadGatewayMsg,								//Thread function name
		(LPVOID)&cThread,							//Thread parameter struct
		0,											//Creation flags
		&tRGMsgID);									//gets thread ID to close it afterwards

	//Launches Game thread
	_tprintf(TEXT("Launching Game thread... ENTER to quit\n"));

	htGame = CreateThread(
		NULL,										//Thread security attributes
		0,											//Stack size (0 for default)
		StartGame,									//Thread function name
		(LPVOID)&cThread,							//Thread parameter struct
		0,											//Creation flags
		&tGameID);									//gets thread ID to close it afterwards

	//Enter to end thread and exit
	_gettchar();

	cThread.ThreadMustGoOn = 0;						//Signals thread to gracefully exit
	sGTick.ThreadMustGoOn = 0;						//Signals thread to gracefully exit


	//If this gets bigger we should maybe move all handles into an array and waitformultipleobjects instead
	WaitForSingleObject(htGame, INFINITE);			//Waits for thread to exit

	WaitForSingleObject(htGTick, INFINITE);			//Waits for thread to exit

	SetEvent(cThread.hSMGatewayUpdate);				//Sets event to own process, this will iterate
													//the thread main loop to check ThreadMustGoOn == 0
	WaitForSingleObject(htGReadMsg, INFINITE);		//Waits for thread to exit
	

	UnmapViewOfFile(cThread.pSMemServer);	//Unmaps view of shared memory
	UnmapViewOfFile(cThread.pSMemGateway);	//Unmaps view of shared memory
	CloseHandle(cThread.hSMem);				//Closes shared memory

	return 0;
}
