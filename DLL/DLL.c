#include <windows.h>
#include "dll.h"

DLL_IMP_API int sharedMemory(HANDLE * hSMem, LARGE_INTEGER * SMemSize){
	//Receives: HANDLE to shared memory; if LARGE_INTEGER has a value will assume it's server

	if (SMemSize) {										//Server side execution, Creates file mapping
		*hSMem = CreateFileMapping(						//Maps a file in memory 
			INVALID_HANDLE_VALUE,						//Handle to file being mapped (INVALID_HANDLE_VALUE to swap)
			NULL,										//Security attributes
			PAGE_READWRITE,								//Maped file permissions
			SMemSize->HighPart,							//MaximumSizeHigh
			SMemSize->LowPart,							//MaximumSizeLow
			SMEM_NAME);									//File mapping name
	}
	else {												//Gateway side execution, Open file mapping
		*hSMem = OpenFileMapping(
			FILE_MAP_ALL_ACCESS,						//Security attributes
			FALSE,										//Handle inheritance by child processes
			SMEM_NAME);									//File mapping name
	}

	if (*hSMem == NULL)									//Return error code if there's an error
		return -1;

	return 0;
}

DLL_IMP_API int mapMsgView(SMCtrl *smCtrl)
{
	smCtrl->pSMemMessage = (SMMessage *)MapViewOfFile(	//Casts view of shared memory to a known struct type
		smCtrl->hSMem,										//Handle to the whole mapped object
		FILE_MAP_ALL_ACCESS,								//Security attributes
		smCtrl->SMemViewServer.HighPart,					//OffsetHIgh (0 to map the whole thing)
		smCtrl->SMemViewServer.LowPart, 					//OffsetLow (0 to map the whole thing)
		smCtrl->SMemViewGateway.QuadPart);					//Number of bytes to map

	if (smCtrl->pSMemMessage == INVALID_HANDLE_VALUE) {
		return -1;
	}
	return 0;
}

DLL_IMP_API int mapGameDataView(SMCtrl *smCtrl, DWORD permission)
{
	smCtrl->pSMemGameData = (SMGameData *)MapViewOfFile(	//Casts view of shared memory to a known struct type
		smCtrl->hSMem,										//Handle to the whole mapped object
		permission,											//Security attributes
		0,													//OffsetHIgh (0 to map the whole thing)
		0,													//OffsetLow (0 to map the whole thing)
		smCtrl->SMemViewServer.QuadPart);					//Number of bytes to map

	if (smCtrl->pSMemGameData == INVALID_HANDLE_VALUE) {
		return -1;
	}
	return 0;
}



