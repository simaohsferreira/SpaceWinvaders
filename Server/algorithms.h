#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "localStructs.h"

DWORD WINAPI InvadersBomb(LPVOID tParam);
DWORD WINAPI RegPathInvaders(LPVOID tParam);
DWORD WINAPI RandPathInvaders(LPVOID tParam);
DWORD WINAPI ShipInstruction(LPVOID tParam);
DWORD WINAPI BombMovement(LPVOID tParam);
int RandomValue(int value);
//DWORD WINAPI ShipShots(LPVOID tParam);
DWORD WINAPI ShotMovement(LPVOID tParam);

#endif /* ALGORITHMS_H */