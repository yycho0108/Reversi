struct BoardState
{
	enum struct tag_State{ OUTSIDE = -1, EMPTY, BLACK,WHITE};
	short int i, j;
	bool hot;
	tag_State State;
	BoardState(tag_State State, int i, int j) :i{ i }, j{ j }, State{ State }, hot{ false }{};
};

struct BoardInfo
{
	enum tag_Turn{ BLACK, WHITE };
	enum tag_Mode{PVC, PVP};
	HWND** hBoardCell;
	int Width, Height;
	int EmptySpace;
	bool Auto;
	tag_Mode Mode;
	tag_Turn Turn;
	tag_Turn PlayerTurn;
};

struct PeekInfo
{
	BoardState::tag_State State;
	int NumStone;
};