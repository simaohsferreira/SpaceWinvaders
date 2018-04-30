//DLL.cpp
#include <windows.h>
#include "dll.h"
//Para verificar ao carregar a dll que a aplicação irá ocupar mais memória
char ponteiro[40960];
//Definição da variável global
int nDLL = 1234;

//Exportar a função para ser utilizada fora da DLL
int UmaString(void) {
	TCHAR str[TAM];
	_tprintf(TEXT("Dentro da Dll\nIntroduza uma frase:"));
	_fgetts(str, TAM, stdin);
	if (_tcslen(str) > 1) //Introduzir mais caracteres do que apenas <enter>
		return 1;
	else
		return 0;
}

DLL_IMP_API int sharedMemory(HANDLE *hSMem, TCHAR SMName[], LARGE_INTEGER SMemSize)
{
	*hSMem = CreateFileMapping(		//Maps a file in memory 
		INVALID_HANDLE_VALUE,		//Handle to file being mapped (INVALID_HANDLE_VALUE to swap)
		NULL,						//Security attributes
		PAGE_READWRITE,				//Maped file permissions
		SMemSize.HighPart,			//MaximumSizeHigh
		SMemSize.LowPart,			//MaximumSizeLow
		SMName);						//File mapping name

	if (*hSMem == INVALID_HANDLE_VALUE) {
		return 1;
	}
	return 0;
}

