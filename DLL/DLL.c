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
	HANDLE mutex = OpenMutex(
		MUTEX_ALL_ACCESS,							//Desired access
		FALSE,										//Inherit handle by child processes
		TEXT("GameDataSync"));						//Event name
	if (mutex == NULL) {
		mutex = CreateMutex(						//Used for game structure integrity
			NULL,									//Security attributes
			FALSE,									//Initial owner
			TEXT("GameDataSync"));					//Mutex name
	}

	return mutex;
}

DLL_IMP_API HANDLE createProdConsMutex()
{
	HANDLE mutex = OpenMutex(
		MUTEX_ALL_ACCESS,							//Desired access
		FALSE,										//Inherit handle by child processes
		TEXT("ProdConsMutex"));						//Event name
	if (mutex == NULL) {
		mutex = CreateMutex(						//Producer consumer buffer integrity
			NULL,									//Security attributes
			FALSE,									//Initial owner
			TEXT("ProdConsMutex"));					//Mutex name
	}

	return mutex;
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
			SMEM_BUFF,								//Initial count
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

DLL_IMP_API Packet consumePacket(SMCtrl *smCtrl, int *nextOut)
{
	Packet localpacket;

		WaitForSingleObject(smCtrl->shOccupied, INFINITE);			//wait occupied 
		WaitForSingleObject(smCtrl->mhProdConsMut, INFINITE);		//wait 
		
		localpacket = smCtrl->pSMemMessage->buffer[*nextOut];		//copy buffer[nextout] to 		
		*nextOut = (*nextOut + 1) % SMEM_BUFF;						//nextout++

		ReleaseMutex(smCtrl->mhProdConsMut);						//release 
		ReleaseSemaphore(smCtrl->shVacant, 1, NULL);				//release semaphore vacant

	return localpacket;
}

DLL_IMP_API int writeGameData(GameData *sharedMemory, GameData *localGame, HANDLE *mutex)
{

	WaitForSingleObject(mutex, INFINITE);

	CopyMemory(sharedMemory, localGame, sizeof(GameData));			//writes(copy) localGameData in to SharedMemory

	//SetEvent(sGTick->hTick);  ---> this moved from here to outside
	ReleaseMutex(mutex);


	return 0;
}

DLL_IMP_API int writePacket(SMCtrl *smCtrl, int *nextIn, Packet localPacket)		//## the used fields will be different. Keep an eye out! ##
{

	WaitForSingleObject(smCtrl->shVacant, INFINITE);			//wait for vacant 
	WaitForSingleObject(smCtrl->mhProdConsMut, INFINITE);		//wait for mutex
	
	smCtrl->pSMemMessage->buffer[*nextIn] = localPacket;		//copy packet
	*nextIn = (*nextIn + 1) % SMEM_BUFF;						//increment counter
	
	ReleaseMutex(smCtrl->mhProdConsMut);						//release mutex
	ReleaseSemaphore(smCtrl->shOccupied, 1, NULL);				//release occupied semaphore

	return 0;
}

DLL_IMP_API GameData consumeGameData(GameData *sharedMemory, HANDLE*mutex)
{
	GameData game;
	WaitForSingleObject(mutex, INFINITE);

	//cThread->localGameData = *cThread->pSMemGameData;
	CopyMemory(&game, sharedMemory, sizeof(GameData));				//Copies shared memory to a local data structure

	ReleaseMutex(mutex);

	return game;
}

DLL_IMP_API int RandomValue(int value) {						//Random function created because threads instantiate the same value over and over.

	int             num;
	unsigned int    number;
	errno_t         err;

		err = rand_s(&number);
		if (err != 0)
		{
			_tprintf(TEXT("The rand_s function failed!\n"));			//do something here... maybe.
			return -1;
		}
		else {
			num = (unsigned int)((double)number / ((double)UINT_MAX + 1) * value);
		}
	return num;
}


