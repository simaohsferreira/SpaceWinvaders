﻿#include "view_logic.h"
#include "resource.h"

HINSTANCE hInst;

ATOM regClass(HINSTANCE hInstance, TCHAR * szAppName) {
	WNDCLASSEX  wndClass;

	wndClass.cbSize = sizeof(wndClass);
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = szAppName;
	wndClass.lpfnWndProc = winManager; //Thread?
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
	wndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);;

	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;

	return(RegisterClassEx(&wndClass));
}

HWND winCreation(HINSTANCE hInstance, TCHAR * szAppName) {

	return CreateWindow(
		szAppName,
		TEXT("Space Winvaders Server"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL);
}



LRESULT CALLBACK winManager(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC         hdc;

	switch (iMsg) {

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_SETTINGS_CREATEGAME:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, winGameCreateDlg);
			break;
		case ID_HELP_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, winAboutDlg);
			break;
		case ID_SETTINGS_CLOSESERVER:
			SendMessage(hWnd, WM_CLOSE,wParam,lParam);
			break;
		default:
			return DefWindowProc(hWnd, iMsg, wParam, lParam);
		}

	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
		TextOut(hdc, 100, 100, TEXT("Luís & Simão!"), 13);
		//Here get logged clients
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_CLOSE:
		if (MessageBox(hWnd, TEXT("Do you realy want to shutdown the server?"), TEXT("Server Shutdown"), MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
			DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK winAboutDlg(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam) {

	switch (iMsg) {
	case WM_INITDIALOG:
		centerDialogWnd(hDlg);
		
		//set focus
		if (GetDlgCtrlID((HWND)wParam) != IDD_DIALOG1)
		{
			SetFocus(GetDlgItem(hDlg, IDD_DIALOG1));
			return FALSE;
		}
		return TRUE;

	case WM_COMMAND:
		switch (wParam) {
		case IDCLOSE:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
			break;
		}
	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		return TRUE;
	}
	return FALSE;

}

LRESULT CALLBACK winGameCreateDlg(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam) {

	switch (iMsg) {
	case WM_INITDIALOG:
	{
		setCreateGameDlgValues(hDlg);

		centerDialogWnd(hDlg);
		//set focus
		if (GetDlgCtrlID((HWND)wParam) != IDD_DIALOG2)
		{
			SetFocus(GetDlgItem(hDlg, IDD_DIALOG2));
			return FALSE;
		}


		return TRUE;
	}
	break;

	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
		{
			int result = validateCreateGameDlgValues(hDlg);
			switch (result) {
			case 0:
				//do something like send values
				break;
			case 1:
				MessageBox(hDlg, TEXT("Invalid number of players!"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
				break;
			case 2:
				MessageBox(hDlg, TEXT("Invalid number of Invaders!"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
				break;
			case 3:
				MessageBox(hDlg, TEXT("Invalid number of Hard Invaders!"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
				break;
			case 10:
				MessageBox(hDlg, TEXT("Invalid number of Hard Invaders - More than Max Invaders!"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
				break;
			case 4:
				MessageBox(hDlg, TEXT("Invalid Invader Speed!"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
				break;
			case 11:
				MessageBox(hDlg, TEXT("Invalid Projectile Speed!"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
				break;
			case 6:
				MessageBox(hDlg, TEXT("Invalid Bomb Rate!"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
				break;
			case 7:
				MessageBox(hDlg, TEXT("Invalid Shot Rate!"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
				break;
			case 8:
				MessageBox(hDlg, TEXT("Invalid Move Rate!"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
				break;
			case 9:
				MessageBox(hDlg, TEXT("Invalid PowerUp Duration!"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
				break;
			}
			break;
		}
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		return TRUE;
	}
	return FALSE;
}



