#include <Windows.h>
#include "BoardCell.h"
#include "resource.h"

#define TILESIZE 32

ATOM RegisterCustomClass(HINSTANCE& hInstance);
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DimensionsDlgProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

void NewGame(HWND hWnd);
void EndGame(HWND hWnd);
extern void AutoPlay(HWND hWnd);
extern VOID CALLBACK AutoPlayProc(HWND hWnd, UINT iMsg, UINT_PTR dwEvent, DWORD dwTime);

enum { ID_CHILDWINDOW = 100 };

BoardInfo* hBoard = nullptr;
HWND** hBoardCell = nullptr;

LPCTSTR Title = L"REVERSI";
HINSTANCE g_hInst;
HWND hMainWnd;
int Width, Height;
BoardInfo::tag_Mode Mode;
ATOM RegisterCustomClass(HINSTANCE& hInstance)
{
	WNDCLASS wc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = NULL;
	wc.hIcon = //LoadIcon(NULL, IDI_APPLICATION);
		LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = Title;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wc.style = CS_VREDRAW | CS_HREDRAW;
	return RegisterClass(&wc);
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	g_hInst = hInstance;
	RegisterCustomClass(hInstance);
	hMainWnd = CreateWindow(Title, Title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,NULL, hInstance, NULL);
	ShowWindow(hMainWnd, nCmdShow);

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_CREATE:
		NewGame(hWnd);
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILE_START:
			NewGame(hWnd);
			break;
		case ID_FILE_END:
			EndGame(hWnd);
			break;
		case ID_FILE_SKIPTURN:
			switch (hBoard->Turn)
			{
			case BoardInfo::tag_Turn::WHITE:
				hBoard->Turn = BoardInfo::tag_Turn::BLACK;
				break;
			case BoardInfo::tag_Turn::BLACK:
				hBoard->Turn = BoardInfo::tag_Turn::WHITE;
				break;
			}
			if (hBoard->Mode == BoardInfo::tag_Mode::PVC && hBoard->Turn == BoardInfo::tag_Turn::WHITE) // computer turn
			{
				//AutoPlay(**hBoardCell);

				SetCursor(LoadCursor(NULL, IDC_WAIT));
				SetTimer(**hBoardCell, 0, 1000, AutoPlayProc);
			}
			break;
		case ID_FILE_PLAYFORME:
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			AutoPlay(**hBoardCell);
			SetTimer(**hBoardCell, 0, 1000, AutoPlayProc);

			break;
		case ID_FILE_EXIT:
			DestroyWindow(hWnd);
			break;
		}
		break;
	case WM_DESTROY:
		EndGame(hWnd);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}

void NewGame(HWND hWnd)
{
	if (hBoardCell != nullptr || hBoard != nullptr)
	{
		MessageBox(hWnd, L"Game is Currently Ongoing", L"ERROR", MB_OK);
		return;
	}
	//DialogBox - Get Width/Height
	if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DimensionsDlgProc) == IDCANCEL)
	{
		MessageBox(hWnd, L"Wrong Parameter", L"ERROR", MB_OK);
		return;
	}
	//Width = 5;
	//Height = 5;
	//TEMPORARILY.
	hBoardCell = (HWND**)malloc(sizeof(HWND*)*(Height + 2));
	hBoard = (BoardInfo*)malloc(sizeof(BoardInfo));
	hBoard->hBoardCell = hBoardCell;
	hBoard->Height = Height;
	hBoard->Width = Width;
	hBoard->EmptySpace = Width*Height;
	hBoard->Turn = BoardInfo::tag_Turn::BLACK;
	hBoard->Auto = true;
	hBoard->Mode = Mode;
	for (int i = 0; i <= Height + 1; ++i)
	{
		hBoardCell[i] = (HWND*)malloc(sizeof(HWND) * (Width + 2));
		for (int j = 0; j <= Width + 1; ++j)
		{
			BoardState* tmpBS = new BoardState((!i || !j || i == Height + 1 || j == Width + 1) ? BoardState::tag_State::OUTSIDE : BoardState::tag_State::EMPTY, i, j);
			hBoardCell[i][j] = CreateWindow(TEXT("ReversiCell"), NULL, WS_CHILD | WS_VISIBLE, j*TILESIZE, i*TILESIZE, TILESIZE, TILESIZE, hWnd, (HMENU)(i*Height + j), g_hInst, tmpBS);
	
		}
	}
	SetClassLongPtr(**hBoardCell, 0, (LONG)hBoard);
	RECT R;
	SetRect(&R, 0, 0, (Width + 2)*TILESIZE, (Height + 2)*TILESIZE);
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, TRUE);
	SetWindowPos(hWnd, 0, 0, 0, R.right - R.left, R.bottom - R.top, SWP_NOMOVE);

	SendMessage(hBoardCell[Height / 2][Width / 2], WM_LBUTTONDOWN, 0, 0);
	SendMessage(hBoardCell[Height / 2][Width / 2 + 1], WM_LBUTTONDOWN, 0, 0);
	SendMessage(hBoardCell[Height / 2+1][Width / 2 + 1], WM_LBUTTONDOWN, 0, 0);
	SendMessage(hBoardCell[Height / 2+1][Width / 2], WM_LBUTTONDOWN, 0, 0);
	hBoard->Auto = false;
}

void EndGame(HWND hWnd)
{
	if (hBoardCell == nullptr && hBoard == nullptr)
	{
		//MessageBox(hWnd, L"No Game to End", L"ERROR", MB_OK);
		return;
	}
	for (int i = 0; i <= Height + 1; ++i)
	{
		for (int j = 0; j <= Width + 1; ++j)
			DestroyWindow(hBoardCell[i][j]);
	}
	free((BoardInfo*)GetClassLongPtr(**hBoardCell, 0));
	for (int i = 0; i <= Height + 1; ++i)
		free(hBoardCell[i]);
	free(hBoardCell);
	free(hBoard);
	hBoardCell = nullptr;
	hBoard = nullptr;
}

BOOL CALLBACK DimensionsDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessageW(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"PvP");
		SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"PvC");
		SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, 0, 0);
		return TRUE;
		//NONE
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			BOOL checkw, checkh;
			Width = GetDlgItemInt(hDlg, IDC_EDIT1, &checkw, FALSE);
			Height = GetDlgItemInt(hDlg, IDC_EDIT2, &checkh, FALSE);
			Mode = (BoardInfo::tag_Mode)SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);// PvP
			if (checkw && checkh && Width < 100 && Height < 100 && Width>3 && Height>3 && !(Width&1) && !(Height&1))//EVEN
				EndDialog(hDlg, IDOK);
			else
				EndDialog(hDlg, IDCANCEL);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
	default:
		return FALSE;
		//NONE
	}
	return FALSE;
}