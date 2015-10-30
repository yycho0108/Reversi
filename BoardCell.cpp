#include <Windows.h>
#include <CommCtrl.h>
#include "BoardCell.h"
#include "resource.h"
#pragma comment(lib, "comctl32.lib")

LRESULT CALLBACK CellProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
VOID CALLBACK AutoPlayProc(HWND hWnd, UINT iMsg, UINT_PTR dwEvent, DWORD dwTime);
int Placeable(HWND hWnd);
void GameOver(HWND hWnd);
void AutoPlay(HWND hWnd);

enum { RCM_CALL = WM_USER + 1, RCM_ECHO, RCM_SETSTATE, RCM_GETSTATE, RCM_PEEK,RCM_PEEKECHO};
HIMAGELIST CellImage;
static HINSTANCE hInstance = GetModuleHandle(NULL);
extern BoardInfo::tag_Turn StartTurn;

class RegisterCellClassHelper
{
public:
	RegisterCellClassHelper()
	{
		CellImage = ImageList_Create(32, 32,ILC_COLOR, 0, 5);
		for (int i = 0; i < 5; ++i)
		{
			HBITMAP hBit = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP1 + i));
			ImageList_Add(CellImage, hBit, NULL);
			//ImageList_Replace(CellImage, i, hBit, NULL);
			DeleteObject(hBit);
		}
		ImageList_SetOverlayImage(CellImage, 4, 1);

		WNDCLASS wc;
		wc.cbClsExtra = sizeof(LONG_PTR);
		wc.cbWndExtra = sizeof(LONG_PTR);
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.hCursor = NULL;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hInstance = hInstance;
		wc.lpfnWndProc = CellProc;
		wc.lpszClassName = TEXT("ReversiCell");
		wc.lpszMenuName = NULL;
		wc.style = 0;
		//MOREOVER...
		RegisterClass(&wc);
	}
	~RegisterCellClassHelper()
	{
		ImageList_Destroy(CellImage);
	}
} RCCHelper;

LRESULT CALLBACK CellProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_CREATE:
	{
		BoardState* MyState = (BoardState*)(((CREATESTRUCT*)lParam)->lpCreateParams);
		SetWindowLongPtr(hWnd, 0, (LONG)MyState);
		
	}
		break;
	case WM_MOUSEMOVE:
	{
		HDC hdc = GetDC(hWnd);
		BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
		if (!MyState->hot && MyState->State == BoardState::tag_State::EMPTY && Placeable(hWnd)) //not hot, not wall
		{
			//AND IF PLACEABLE
			TRACKMOUSEEVENT MT;
			MT.cbSize = sizeof(TRACKMOUSEEVENT);
			MT.dwFlags = TME_LEAVE;// | TME_HOVER;
			MT.hwndTrack = hWnd;
			TrackMouseEvent(&MT);
			ImageList_Draw(CellImage, (int)MyState->State + 1, hdc, 0, 0, ILD_BLEND50);
			MyState->hot = true;
		}

		SetCursor(LoadCursor(NULL, MyState->hot ? IDC_HAND : IDC_ARROW));

		ReleaseDC(hWnd, hdc);
		break;
	}
	case WM_LBUTTONDOWN: //new stone
	{
		BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
		BoardInfo* hBoard = (BoardInfo*)GetClassLongPtr(hWnd, 0);

		if (MyState->hot || hBoard->Auto)
		{
			//if (MyState->hot) //user
			{
				for (short int i = MyState->i - 1; i <= MyState->i + 1; ++i)
				{
					for (short int j = MyState->j - 1; j <= MyState->j + 1; ++j)
					{
						//	if (i!=MyState->i || j!=MyState->j)
						SendMessage(hBoard->hBoardCell[i][j], RCM_CALL,
							(WPARAM)((hBoard->Turn == BoardInfo::tag_Turn::BLACK) ? BoardState::tag_State::BLACK : BoardState::tag_State::WHITE),
							MAKELPARAM(j - MyState->j, i - MyState->i));//lParam = DIR

					}
				}
			}

			switch (hBoard->Turn)
			{
			case BoardInfo::tag_Turn::BLACK:
				hBoard->Turn = BoardInfo::tag_Turn::WHITE;
				SendMessage(hBoard->hBoardCell[MyState->i][MyState->j], RCM_SETSTATE, (WPARAM)BoardState::tag_State::BLACK, NULL);
				break;
			case BoardInfo::tag_Turn::WHITE:
				hBoard->Turn = BoardInfo::tag_Turn::BLACK;
				SendMessage(hBoard->hBoardCell[MyState->i][MyState->j], RCM_SETSTATE, (WPARAM)BoardState::tag_State::WHITE, NULL);
				break;
			}

			if (--(hBoard->EmptySpace) == 0)
				GameOver(hWnd);
			else if (hBoard->Mode == BoardInfo::tag_Mode::PVC && wParam && lParam) //by player
			{
				//AutoPlay(hWnd);
				SetCursor(LoadCursor(NULL, IDC_WAIT));
				SetTimer(hWnd, 0, 1000, AutoPlayProc);
			}
		}
		break;
	}
	case WM_MOUSELEAVE: //occurs only when tracking. no need to check
	{
		HDC hdc = GetDC(hWnd);
		BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
		MyState->hot = false;
		ImageList_Draw(CellImage, (int)MyState->State + 1, hdc, 0, 0, ILD_NORMAL);
		ReleaseDC(hWnd, hdc);
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
			ImageList_Draw(CellImage, (int)MyState->State + 1, hdc, 0, 0, ILD_NORMAL);
		EndPaint(hWnd, &ps);
		break;
	}
	case RCM_SETSTATE:
	{
		BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
		MyState->State = (BoardState::tag_State)wParam;
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	}
	case RCM_GETSTATE:
	{
		BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
		BoardState::tag_State* AskState = (BoardState::tag_State*)lParam;
		*AskState = MyState->State;
		break;
	}
	case RCM_CALL:
	{
		if (lParam)// nonzero
		{
			BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
			if (MyState->State == BoardState::tag_State::EMPTY || MyState->State == BoardState::tag_State::OUTSIDE)
				; //stop propagating
			else
			{
				short int i = HIWORD(lParam);
				short int j = LOWORD(lParam);
				BoardInfo* hBoard = (BoardInfo*)GetClassLongPtr(hWnd, 0);
				HWND** hBoardCell = hBoard->hBoardCell;
				if ((BoardState::tag_State)wParam == MyState->State)//same state
					SendMessage(hBoardCell[MyState->i - i][MyState->j - j], RCM_ECHO, wParam, MAKELPARAM(-j, -i));
				else
					SendMessage(hBoardCell[MyState->i + i][MyState->j + j], RCM_CALL, wParam, MAKELPARAM(j, i));
			}
		}
		else //SELF
		{
			switch ((BoardState::tag_State)wParam)
			{
			case BoardState::tag_State::BLACK:
				SendMessage(hWnd, RCM_SETSTATE, (WPARAM)BoardState::tag_State::BLACK, 0);
				break;
			case BoardState::tag_State::WHITE:
				SendMessage(hWnd, RCM_SETSTATE, (WPARAM)BoardState::tag_State::WHITE, 0);
				break;
			}
		}
		break;
	}
	case RCM_ECHO:
	{
		BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
		short int i = HIWORD(lParam);
		short int j = LOWORD(lParam);
		BoardInfo* hBoard = (BoardInfo*)GetClassLongPtr(hWnd, 0);
		HWND** hBoardCell = hBoard->hBoardCell;
		BoardState::tag_State ThisState = (BoardState::tag_State)wParam;
		if (MyState->State == BoardState::tag_State::EMPTY)
		{
			SendMessage(hBoardCell[MyState->i][MyState->j], RCM_SETSTATE, wParam, NULL);
			//orig.loc.
		}
		else if (ThisState != MyState->State)//different state = flip
		{	
			SendMessage(hBoardCell[MyState->i + i][MyState->j + j], RCM_ECHO, wParam, lParam);
			SendMessage(hBoardCell[MyState->i][MyState->j], RCM_SETSTATE, wParam, NULL);
		}
		else; //same state = do nothing
		break;
	}
	case RCM_PEEK:
	{
		BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
		PeekInfo* pInfo = (PeekInfo*)wParam;
		if (MyState->State == BoardState::tag_State::EMPTY || MyState->State == BoardState::tag_State::OUTSIDE)
			; //stop propagating
		else
		{
			short int i = HIWORD(lParam);
			short int j = LOWORD(lParam);
			BoardInfo* hBoard = (BoardInfo*)GetClassLongPtr(hWnd, 0);
			HWND** hBoardCell = hBoard->hBoardCell;
			if (pInfo->State == MyState->State)//same state
				SendMessage(hBoardCell[MyState->i - i][MyState->j - j], RCM_PEEKECHO, wParam, MAKELPARAM(-j, -i));
			else //different
				SendMessage(hBoardCell[MyState->i + i][MyState->j + j], RCM_PEEK, wParam, MAKELPARAM(j, i));
		}

		/*
		BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
		BoardState* SrcState = (BoardState*)wParam;
		if (MyState->State == BoardState::tag_State::EMPTY || MyState->State == BoardState::tag_State::OUTSIDE)
			; //stop propagating
		else
		{
			BoardInfo* hBoard = (BoardInfo*)GetClassLongPtr(hWnd, 0);
			short int i = HIWORD(lParam);
			short int j = LOWORD(lParam);
			if (MyState->State == SrcState->State)
			{
				if (abs(MyState->i - SrcState->i) > 1 || abs(MyState->j - SrcState->j) > 1)
					SrcState->hot = true;
			}
			else
				SendMessage(hBoard->hBoardCell[MyState->i + i][MyState->j + j], RCM_PEEK, wParam, lParam);
		}
		*/
		break;
	}
	case RCM_PEEKECHO:
	{
		BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
		BoardInfo* hBoard = (BoardInfo*)GetClassLongPtr(hWnd, 0);
		PeekInfo* pInfo = (PeekInfo*)wParam;
		short int i = HIWORD(lParam);
		short int j = LOWORD(lParam);
		HWND** hBoardCell = hBoard->hBoardCell;

		if (MyState->State == BoardState::tag_State::EMPTY || MyState->State == BoardState::tag_State::OUTSIDE)
		{
			;
		}
		else if (pInfo->State != MyState->State)//different state = flip
		{
			++pInfo->NumStone;
			SendMessage(hBoardCell[MyState->i + i][MyState->j + j], RCM_PEEKECHO, wParam, lParam);
		}
		else; //same state = do nothing
		break;
	}
	case WM_DESTROY:
		delete (BoardState*)GetWindowLongPtr(hWnd, 0);
		break;

	default:
		return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}

void GameOver(HWND hWnd)
{
	BoardInfo* hBoard = (BoardInfo*)GetClassLongPtr(hWnd, 0);
	HWND** hBoardCell = hBoard->hBoardCell;
	BoardState::tag_State CheckState;
	int NumWhite = 0;
	int NumBlack = 0;
	for (int i = 1; i <= hBoard->Height; ++i)
	{
		for (int j = 1; j <= hBoard->Width; ++j)
		{
			SendMessage(hBoardCell[i][j], RCM_GETSTATE, NULL, (LPARAM)&CheckState);
			if (CheckState == BoardState::tag_State::BLACK) ++NumBlack;
			else if (CheckState == BoardState::tag_State::WHITE) ++NumWhite;
		}
	}
	TCHAR ScoreReport[256] = {};
	wsprintf(ScoreReport, TEXT("BLACK %d : WHITE %d\n%s"), NumBlack, NumWhite
		,(NumBlack > NumWhite) ? TEXT("BLACK WINS") : ( (NumWhite > NumBlack) ? TEXT("WHITE WINS") : TEXT("DRAW"))
		);
	MessageBox(hWnd, ScoreReport, L"GAME OVER", MB_OK);//GAME OVER.
}

int Placeable(HWND hWnd)
{
	BoardInfo* hBoard = (BoardInfo*)GetClassLongPtr(hWnd, 0);
	BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
	
	PeekInfo pInfo;
	pInfo.NumStone = 0;
	pInfo.State = (hBoard->Turn == BoardInfo::tag_Turn::BLACK) ? BoardState::tag_State::BLACK : BoardState::tag_State::WHITE;
	if (MyState->State != BoardState::tag_State::EMPTY)
		return 0;
	for (short int i = MyState->i - 1; i <= MyState->i + 1; ++i)
	{
		for (short int j = MyState->j - 1; j <= MyState->j + 1; ++j)
		{
			if (i != MyState->i || j != MyState->j)
				SendMessage(hBoard->hBoardCell[i][j], RCM_PEEK, (WPARAM)&pInfo, MAKELPARAM(j - MyState->j, i - MyState->i));
				//SendMessage(hBoard->hBoardCell[i][j], RCM_PEEK, (WPARAM)&MyState, MAKELPARAM(j - MyState.j, i - MyState.i));//lParam = DIR
		}
	}
	return pInfo.NumStone;
}

void AutoPlay(HWND hWnd)
{
	BoardInfo* hBoard = (BoardInfo*)GetClassLongPtr(hWnd, 0);
	BoardState* MyState = (BoardState*)GetWindowLongPtr(hWnd, 0);
	HWND** hBoardCell = hBoard->hBoardCell;

	int MaxStone = 0;
	int iDest = 0;
	int jDest = 0;

	hBoard->Auto = true;
	for (short int i = 1; i <= hBoard->Height; ++i)
	{
		for (short int j = 1; j <= hBoard->Width; ++j)
		{
			int NumStone = Placeable(hBoardCell[i][j]);

			if (NumStone)
			{
				NumStone += hBoard->Width*hBoard->Height*(i == 1 && j == 1);
				NumStone += hBoard->Width*hBoard->Height*(i == 1 && j == hBoard->Width);
				NumStone += hBoard->Width*hBoard->Height*(i == hBoard->Height && j == 1);
				NumStone += hBoard->Width*hBoard->Height*(i == hBoard->Height && j == hBoard->Width);

				//NumStone -= hBoard->Width*hBoard->Height*(i == 2 && (j==1 || j==2 || j == hBoard->Width || j == hBoard-));
				//NumStone -= hBoard->Width*hBoard->Height*(j == 2 && (i==1 || i == hBoard->Height));
				//NumStone -= hBoard->Width*hBoard->Height*(i == hBoard->Height-1 && (j==1 || j==hBoard->Width));
				//NumStone -= hBoard->Width*hBoard->Height*(j == hBoard->Width-1 && (i==1 || i==hBoard->Height));
				NumStone -= 4 * hBoard->Width*hBoard->Height*((i == 2 || j == 2 || i == hBoard->Height - 1 || j == hBoard->Width - 1) && ((i<3 || i>hBoard->Height - 2) && (j<3 || j>hBoard->Width - 2)));
				//if (i == 2 || j == 2 || i == hBoard->Height - 1 || j == hBoard->Width - 1)
				//	NumStone -= 4 * (hBoard->Height*hBoard->Width);
				if (NumStone > MaxStone || !MaxStone) //MaxStone = 0 = No Other Moves
				{
					MaxStone = NumStone;
					iDest = i;
					jDest = j;
				}
			}
		}
	}

	if (MaxStone != 0)
	{
		SendMessage(hBoardCell[iDest][jDest], WM_LBUTTONDOWN, 0, 0);
	}
	else
	{
		hBoard->Turn = hBoard->Turn == BoardInfo::tag_Turn::BLACK ? BoardInfo::tag_Turn::WHITE : BoardInfo::tag_Turn::BLACK;
		
		if (hBoard->EmptySpace)
			MessageBox(hWnd, L"NO MOVES", L"SKIP TURN", MB_OK);
	}
	hBoard->Auto = false;
}

VOID CALLBACK AutoPlayProc(HWND hWnd, UINT iMsg, UINT_PTR EventID, DWORD dwTime)
{
	AutoPlay(hWnd);
	KillTimer(hWnd, EventID);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
}