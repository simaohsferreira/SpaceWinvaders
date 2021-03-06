#include "dll.h"

DLL_IMP_API int sharedMemory(HANDLE * hSMem, LARGE_INTEGER * SMemSize){
	//Receives: HANDLE to shared memory; if LARGE_INTEGER has a value will assume it's server

	if (SMemSize) {											//Server side execution, Creates file mapping
		*hSMem = CreateFileMapping(							//Maps a file in memory 
			INVALID_HANDLE_VALUE,							//Handle to file being mapped (INVALID_HANDLE_VALUE to swap)
			NULL,											//Security attributes
			PAGE_READWRITE,									//Maped file permissions
			SMemSize->HighPart,								//MaximumSizeHigh
			SMemSize->LowPart,								//MaximumSizeLow
			SMEM_NAME);										//File mapping name
	}
	else {													//Gateway side execution, Open file mapping
		*hSMem = OpenFileMapping(
			FILE_MAP_ALL_ACCESS,							//Security attributes
			FALSE,											//Handle inheritance by child processes
			SMEM_NAME);										//File mapping name
	}

	if (*hSMem == NULL)										//Return error code if there's an error
		return -1;

	return 0;
}

DLL_IMP_API int mapMsgView(SMCtrl *smCtrl)
{
	smCtrl->pSMemMessage = (SMMessage *)MapViewOfFile(		//Casts view of shared memory to a known struct type
		smCtrl->hSMem,										//Handle to the whole mapped object
		FILE_MAP_ALL_ACCESS,								//Security attributes
		smCtrl->SMemViewServer.HighPart,					//OffsetHIgh (0 to map the whole thing)
		smCtrl->SMemViewServer.LowPart, 					//OffsetLow (0 to map the whole thing)
		(SIZE_T)smCtrl->SMemViewGateway.QuadPart);			//Number of bytes to map

	if (smCtrl->pSMemMessage == INVALID_HANDLE_VALUE) {
		return -1;
	}
	return 0;
}

DLL_IMP_API int mapGameDataView(SMCtrl *smCtrl, DWORD permission)
{
	smCtrl->pSMemGameData = (GameData *)MapViewOfFile(	//Casts view of shared memory to a known struct type
		smCtrl->hSMem,										//Handle to the whole mapped object
		permission,											//Security attributes
		0,													//OffsetHIgh (0 to map the whole thing)
		0,													//OffsetLow (0 to map the whole thing)
		(SIZE_T)smCtrl->SMemViewServer.QuadPart);			//Number of bytes to map

	if (smCtrl->pSMemGameData == INVALID_HANDLE_VALUE) {
		return -1;
	}
	return 0;
}

DLL_IMP_API HANDLE createGameDataMutex()
{
	HANDLE hMutex = OpenMutex(
		MUTEX_ALL_ACCESS,							//Desired access
		FALSE,										//Inherit handle by child processes
		TEXT("GameDataSync"));						//Event name
	if (hMutex == NULL) {
		hMutex = CreateMutex(						//Used for game structure integrity
			NULL,									//Security attributes
			FALSE,									//Initial owner
			TEXT("GameDataSync"));					//Mutex name
	}

	return hMutex;
}

DLL_IMP_API HANDLE createProdConsMutex()
{
	HANDLE hMutex = OpenMutex(
		MUTEX_ALL_ACCESS,							//Desired access
		FALSE,										//Inherit handle by child processes
		TEXT("ProdConsMutex"));						//Event name
	if (hMutex == NULL) {
		hMutex = CreateMutex(						//Producer consumer buffer integrity
			NULL,									//Security attributes
			FALSE,									//Initial owner
			TEXT("ProdConsMutex"));					//Mutex name
	}

	return hMutex;
}

DLL_IMP_API HANDLE createOccupiedSemaphore()
{
	HANDLE semaphore = OpenSemaphore(				//It starts with full vacancies
		SEMAPHORE_ALL_ACCESS,						//Desired access
		FALSE,										//Inherit handle by child processes
		TEXT("OccupiedFields"));					//Semaphore name
	if (semaphore == NULL) {
		semaphore = CreateSemaphore(				//It starts with full vacancies
			NULL,									//Security attributes
			0,										//Initial count
			SMEM_BUFF,								//Maximum count
			TEXT("OccupiedFields"));
	}
	return semaphore;
}

DLL_IMP_API HANDLE createVacantSemaphore()
{
	HANDLE semaphore = OpenSemaphore(				//It starts with full vacancies
		SEMAPHORE_ALL_ACCESS,						//Desired access
		FALSE,										//Inherit handle by child processes
		TEXT("VacantFields"));						//Semaphore name
	if (semaphore == NULL) {
		semaphore = CreateSemaphore(				//It starts with full vacancies
			NULL,									//Security attributes
			SMEM_BUFF,								//Initial count
			SMEM_BUFF,								//Maximum count
			TEXT("VacantFields"));
	}
	return semaphore;
}

DLL_IMP_API Packet consumePacket(SMCtrl *smCtrl) {

	static int nextOut = 0;
	Packet localpacket;

	WaitForSingleObject(smCtrl->shOccupied, INFINITE);			//decreases one occupied value/ waits for an item in buffer
	WaitForSingleObject(smCtrl->mhProdConsMut, INFINITE);		//waits for exclusive access
		
	localpacket = smCtrl->pSMemMessage->buffer[nextOut];		//copy buffer[nextout] to local memory	
	nextOut = (nextOut + 1) % SMEM_BUFF;						//nextout++ (loop counter [0, ... , SMEM_BUFF-1])

	ReleaseMutex(smCtrl->mhProdConsMut);						//release exclusive access
	ReleaseSemaphore(smCtrl->shVacant, 1, NULL);				//release semaphore vacant / signals a vacant spot

	return localpacket;
}

DLL_IMP_API int writeGameData(GameData *sharedMemory, GameData *localGame, HANDLE mutex){

	WaitForSingleObject(mutex, INFINITE);

	CopyMemory(sharedMemory, localGame, sizeof(GameData));			//writes(copy) localGameData in to SharedMemory

	ReleaseMutex(mutex);

	return 0;
}

DLL_IMP_API int writePacket(SMCtrl *smCtrl, Packet localPacket)		//## the used fields will be different. Keep an eye out! ##
{
	static int nextIn = 0;

	WaitForSingleObject(smCtrl->shVacant, INFINITE);			//wait for vacant 
	WaitForSingleObject(smCtrl->mhProdConsMut, INFINITE);		//wait for mutex
	
	smCtrl->pSMemMessage->buffer[nextIn] = localPacket;		//copy packet
	nextIn = (nextIn + 1) % SMEM_BUFF;						//increment counter
	
	ReleaseMutex(smCtrl->mhProdConsMut);						//release mutex
	ReleaseSemaphore(smCtrl->shOccupied, 1, NULL);				//release occupied semaphore

	return 0;
}

DLL_IMP_API GameData consumeGameData(GameData *sharedMemory, HANDLE*mutex)
{
	GameData game;
	WaitForSingleObject(mutex, INFINITE);

	CopyMemory(&game, sharedMemory, sizeof(GameData));				//Copies shared memory to a local data structure

	ReleaseMutex(mutex);

	return game;
}

DLL_IMP_API int RandomValue(int value) {						//Random function created because threads instantiate the same value over and over.
																//[0, value-1]
	int             num;
	unsigned int    number;
	errno_t         err;

		err = rand_s(&number);
		if (err != 0)
		{														//Fallback to a less secure RNG
			srand((unsigned int)time(NULL));					//This will only run in case of error
			num = rand() % value;
		}
		else {
			num = (unsigned int)((double)number / ((double)UINT_MAX + 1) * value);
		}
	return num;
}


