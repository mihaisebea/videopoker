
#include "common.h"
#include <bgfx.h>
#include "time.h"
#include <algorithm>
#include "bgfx_utils.h"
#include "nanovg/nanovg.h"

///////////////////////////////////////////////////////////////////////////////

#pragma warning(disable: 4100)
#pragma warning(disable: 4189)

///////////////////////////////////////////////////////////////////////////////

static const char* wins[9] = {
	"ROYAL FLUSH",
	"STRAIGHT FLUSH",
	"4 OF A KIND",
	"FULL HOUSE",
	"FLUSH",
	"STRAIGHT",
	"3 OF A KIND",
	"TWO PAIR",
	"JACKS OR BETTER"
};

///////////////////////////////////////////////////////////////////////////////

static uint32_t wins_payout[9] = {
	250,
	50,
	25,
	9,
	6,
	4,
	3,
	2,
	1,
};

///////////////////////////////////////////////////////////////////////////////

typedef uint64_t u64;
typedef int64_t s64;
typedef uint32_t u32;
typedef int32_t s32;

///////////////////////////////////////////////////////////////////////////////

enum EGameState
{
	STATE_WAIT_BET,
	STATE_DEAL,
	STATE_WAIT_SELECT,
	STATE_DEAL_EXTRA,
	STATE_CHECK_WIN,
	STATE_SHOW_WIN,
};

///////////////////////////////////////////////////////////////////////////////

void UpdateScene(u64 delta);

static const s32 START_SPACING_X = 10;
static const s32 START_SPACING_Y = 9;
static const s32 INNER_SPACING_X = 10;
static const s32 INNER_SPACING_Y = 10;
static const s32 IMG_CARD_W = 145;
static const s32 IMG_CARD_H = 205;

void GetCardRect(s32 startX, s32 startY, u32 index, u32 row, u32 colum, u32 screenWidth, u32 screenHeight, float& screenPosX, float& screenPosY, float& cardWidthOnScreen, float& cardHeightOnScreen)
{
	static const s32 screenSpacing = 8;

	float ar = IMG_CARD_H / (float)(IMG_CARD_W);

	cardWidthOnScreen = ((screenWidth - 2 * startX) / float(5)) - screenSpacing; // 5 is the number of cards
	cardHeightOnScreen = cardWidthOnScreen * ar;
	//pos on screen 
	screenPosX = startX + index *	(cardWidthOnScreen + screenSpacing);
	screenPosY = startY;
}

///////////////////////////////////////////////////////////////////////////////

void DrawCard(struct NVGcontext* vg, u32 img, s32 startX, s32 startY, u32 index, u32 row, u32 column, u32 imgW, u32 imgH, u32 screenWidth, u32 screenHeight, bool held)
{
	float screenPosX;
	float screenPosY;
	float cardWidthOnScreen;
	float cardHeightOnScreen;

	GetCardRect(startX, startY, index, row, column, screenWidth, screenHeight, screenPosX, screenPosY, cardWidthOnScreen, cardHeightOnScreen);

	float imgScale = ((cardWidthOnScreen) / (float)(IMG_CARD_W));
	
	// pos in image
	float imgPosX = (START_SPACING_X + column * (IMG_CARD_W + INNER_SPACING_X));
	float imgPosY = (START_SPACING_Y + row * (IMG_CARD_H + INNER_SPACING_Y));

	float ix = -imgPosX * imgScale;
	float iy = -imgPosY * imgScale;
	float iw = imgW * imgScale;
	float ih = imgH * imgScale;// *0.5f;

	NVGpaint imgPaint = nvgImagePattern(vg, screenPosX + ix, screenPosY + iy, iw, ih, 0.0f / 180.0f*NVG_PI, img, 1);
	NVGpaint fadePaint = nvgLinearGradient(vg, 10, 10, imgW, imgH, nvgRGBA(200, 200, 200, 255), nvgRGBA(200, 200, 200, 0));

	nvgBeginPath(vg);
	nvgRect(vg, screenPosX, screenPosY, cardWidthOnScreen, cardHeightOnScreen);
	nvgFillPaint(vg, imgPaint);
	nvgFill(vg);

	//nvgBeginPath(vg);
	//nvgRect(vg, screenPosX, screenPosY, cardWidthOnScreen, cardHeightOnScreen);

	//nvgFillPaint(vg, fadePaint);
	//nvgFill(vg);
	NVGcolor textColor = nvgRGB(250, 255, 138);
	if (held)
	{
		nvgSave(vg);
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, textColor);
		nvgText(vg, screenPosX + cardWidthOnScreen / 2, screenPosY + cardHeightOnScreen + 10, "HELD", NULL);
		nvgRestore(vg);
	}

}

///////////////////////////////////////////////////////////////////////////////

void FillRect(struct NVGcontext* vg, u32 x, u32 y, u32 w, u32 h, const NVGcolor& color)
{
	//nvgSave(vg);

	nvgBeginPath(vg);	
	nvgRoundedRect(vg, x, y, w, h, 2.f);
	nvgFillColor(vg, color);
	nvgFill(vg);
}

///////////////////////////////////////////////////////////////////////////////

void DrawRect(struct NVGcontext* vg, u32 x, u32 y, u32 w, u32 h, const NVGcolor& color)
{
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, w, h, 2.f);
	nvgStrokeColor(vg, color);
	nvgStrokeWidth(vg, 1);

	nvgStroke(vg);
}

///////////////////////////////////////////////////////////////////////////////

void DrawScene(struct NVGcontext* vg, u32 img, u32 bnd_font, u32 scrW, u32 scrH, s32 winIndex, u32 tableBet, uint32_t tableCards[5], uint32_t heldCards[5], u32 tableCredit, EGameState gameState)
{
	u32 startX = 12;
	u32 startY = 12;

	u32 textH = 30;
	u32 rectH = textH * 9 + 10;
	
	FillRect(vg, 0, 0, scrW, scrH, nvgRGB(0, 0, 165));
	
	//draw top part 
	u32 endX = scrW - startX * 2 ;

	FillRect(vg, startX, startY, endX, startY + rectH, nvgRGB(23, 18, 48));
	
	NVGcolor textColor = nvgRGB(250, 255, 138);
	NVGcolor textSelected = nvgRGB(0, 255, 138);
	NVGcolor selectedColor = nvgRGB(170, 34, 22);
		
	//draw filled rectangle for current bet 
	for (u32 j = 0; j < 5; j++)
	{
		//tableBet
		bool sel = (5 - tableBet == j);
		u32 offset = sel ? 1 : 0;

		DrawRect(vg, endX - j * 100 - 100 + 1, startY, 100, startY + rectH, textColor);
		
		if (sel)
		{
			FillRect(vg, endX - j * 100 - 100 + 1 + offset, startY + offset, 100 - 2, startY + rectH - 2, selectedColor);
		}
	}

	float lineh;
	nvgFontSize(vg, 20.0f);
	nvgFontFace(vg, "sans");
	nvgFontFaceId(vg, bnd_font);

	nvgTextMetrics(vg, NULL, NULL, &lineh);

	//u32 align = video::TEXT_CENTER_VERT | video::TEXT_RIGHT_ALIGN;
	char buffer[256];
	for (u32 j = 0; j < 9; j++)
	{
		NVGcolor c = textColor;

		if (winIndex)
		{
			if (winIndex - 1 == j)
			{
				FillRect(vg, startX, startY + 5 + (textH)* j, endX - startX, (textH), nvgRGBA(250, 255, 138, 220));
				c = selectedColor;
			}
		}

		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, c);		
		nvgText(vg, startX + 10, startY + 20 + (textH)* j, wins[j], wins[j] + strlen(wins[j]));
		
		nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
		sprintf(buffer, "%d", wins_payout[j]);
		nvgText(vg, endX - 410, startY + 20 + (textH)* j, buffer, NULL);
		sprintf(buffer, "%d", wins_payout[j] * 2);
		nvgText(vg, endX - 310, startY + 20 + (textH)* j, buffer, NULL);
		sprintf(buffer, "%d", wins_payout[j] * 3);
		nvgText(vg, endX - 210, startY + 20 + (textH)* j, buffer, NULL);
		sprintf(buffer, "%d", wins_payout[j] * 4);
		nvgText(vg, endX - 110, startY + 20 + (textH)* j, buffer, NULL);
		sprintf(buffer, "%d", wins_payout[j] * 5);
		nvgText(vg, endX - 10, startY + 20 + (textH)* j, buffer, NULL);
	}


	u32 cardStartX = startX;
	u32 cardStartY = scrH / 2 - 10;

	for (u32 k = 0; k < 5; k++)
	{
		s32 card = tableCards[k];

		s32 cardRow = card / 13;
		s32 cardColumn = card % 13;

		DrawCard(vg, img, cardStartX, cardStartY, k, cardRow, cardColumn, 2048, 1280, scrW, scrH, heldCards[k]);
	}

	float screenPosX;
	float screenPosY;
	float cardWidthOnScreen;
	float cardHeightOnScreen;

	s32 card = tableCards[0];

	s32 cardRow = card / 13;
	s32 cardColumn = card % 13;

	GetCardRect(cardStartX, cardStartY, card, cardRow, cardColumn, scrW, scrH, screenPosX, screenPosY, cardWidthOnScreen, cardHeightOnScreen);


	nvgFillColor(vg, nvgRGB(255,255,255));
	u32 currentY = cardStartY - 30;
	nvgSave(vg);
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

	////draw WIN 
	sprintf(buffer, "WIN : %d", winIndex ? wins_payout[winIndex - 1] * tableBet : 0);
	nvgText(vg, scrW / 4, currentY, buffer, NULL);
	////draw BET
	sprintf(buffer, "BET : %d", tableBet);
	nvgText(vg, scrW / 2, currentY, buffer, NULL);
	////DRAW credit 
	sprintf(buffer, "CREDIT : %d", tableCredit);
	nvgFillColor(vg, nvgRGB(255, 255, 255));
	nvgText(vg, 3 * scrW / 4, currentY,  buffer, NULL);

	u32 stateY = cardStartY - 10;
	//draw game state
	nvgRestore(vg);

	nvgSave(vg);
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

	switch (gameState)
	{
		case STATE_WAIT_BET:
		{		
			sprintf(buffer, "Choose bet / Click to Deal");
			nvgFillColor(vg, nvgRGB(255, 255, 255));
			nvgText(vg, scrW / 2, stateY, buffer, NULL);

			break;
		}

		case STATE_DEAL:
		{
			
			break;
		}

		case STATE_WAIT_SELECT:
		{
		
			sprintf(buffer, "Hold cards / Click to Deal");
			nvgFillColor(vg, nvgRGB(255, 255, 255));
			nvgText(vg, scrW / 2, stateY, buffer, NULL);

			break;
		}

		case STATE_DEAL_EXTRA:
		{
		
			break;
		}

		case STATE_CHECK_WIN:
		{
		

			break;
		}

		case STATE_SHOW_WIN:
		{
		
			sprintf(buffer, "Click to turn cards");
			nvgFillColor(vg, nvgRGB(255, 255, 255));
			nvgText(vg, scrW / 2, stateY, buffer, NULL);

			break;
		}
	}
	nvgRestore(vg);
}

///////////////////////////////////////////////////////////////////////////////

int cmp_int(void const* a, void const* b)
{
	return (*(int*)a - *(int*)b);
}

///////////////////////////////////////////////////////////////////////////////

int CheckWin(u32 cards[5])
{
	u32 win = 0;

	u32 suits[4];         //  To check for a flush
	u32 matched[13];      //  To check for pairs, threes, fours

	u32 cardvals[5];

	memcpy(cardvals, cards, sizeof(u32) * 5);
	qsort(cardvals, 5, sizeof(u32), cmp_int);

	u32 pairs = 0, threes = 0, fours = 0;
	bool flush = false, straight = false;

	//cardvals.sort(compare);           //  Sort the cards

	memset(suits, 0, sizeof(u32) * 4);
	memset(matched, 0, sizeof(u32) * 13);

	for (u32 i = 0; i < 5; i++)
	{
		matched[cards[i] % 13]++;  //  Update matched for cards
		suits[(cards[i] / 13)]++;    //  Update suits for cards
	}


	//  Check for pairs, threes and fours

	for (u32 i = 0; i < 13; i++)
	{
		if (matched[i] == 2)
		{
			pairs++;
		}
		else if (matched[i] == 3)
		{
			threes++;
		}
		else if (matched[i] == 4)
		{
			fours++;
		}
	}

	//  Check for a flush

	for (u32 i = 0; i < 4; i++)
	{
		if (suits[i] == 5)
		{
			flush = true;
		}
	}

	if (cardvals[4] - cardvals[1] == 3 &&              //  Consistent with 
		cardvals[4] - cardvals[0] == 12 &&              //  A, T, J, Q, K...
		flush)
	{
		//  If we also have a flush, then its a royal flush
		win = 1;
	}
	else if (cardvals[4] - cardvals[0] == 4 && flush)
	{
		//info.value = "Straight flush!";
		//won = bet * 250;
		win = 2;
	}

	//  Sort cards by face value (necessary to check for a straight)

	for (u32 i = 0; i < 5; i++)
	{
		cardvals[i] = cardvals[i] % 13;
	}

	qsort(cardvals, 5, sizeof(u32), cmp_int);

	if (win == 0)
	{           // Don't check further if we've already won
		if (fours > 0)
		{
			//info.value = "Four of a kind!";
			//won = bet * 100;
			win = 3;
		}
		else if (threes == 1 && pairs == 1)
		{
			//info.value = "Full house!";
			//won = bet * 50;
			win = 4;
		}
		else if (flush)
		{
			//info.value = "A flush!";
			//won = bet * 20;
			win = 5;
		}
		else if ((cardvals[1] - cardvals[0] == 1) &&
			(cardvals[2] - cardvals[1] == 1) &&
			(cardvals[3] - cardvals[2] == 1) &&
			(cardvals[4] - cardvals[3] == 1)
			)
		{
			//info.value = "A straight!";
			//won = bet * 15;
			win = 6;
		}
		else if (threes > 0)
		{
			//info.value = "Three of a kind!";
			//won = bet * 4;
			win = 7;
		}
		else if (pairs == 2)
		{
			//info.value = "Two pair!";
			//won = bet * 3;
			win = 8;
		}
		else if (matched[0] == 2 ||
			matched[10] == 2 ||
			matched[11] == 2 ||
			matched[12] == 2)
		{
			//info.value = "Jacks or better!";
			//won = bet * 2;
			win = 9;
		}
	}

	return win;
}


///////////////////////////////////////////////////////////////////////////////
bool IsCardSelected(s32 index, uint32_t tableCards[5], u32 startX, u32 startY, u32 screenWidth, u32 screenHeight, u32 mx, u32 my)
{
	float screenPosX;
	float screenPosY;
	float cardWidthOnScreen;
	float cardHeightOnScreen;

	s32 card = tableCards[index];

	s32 cardRow = card / 13;
	s32 cardColumn = card % 13;

	GetCardRect(startX, startY, index, cardRow, cardColumn, screenWidth, screenHeight, screenPosX, screenPosY, cardWidthOnScreen, cardHeightOnScreen);

	if (mx < screenPosX || my < screenPosY)
		return false;
	if (mx > screenPosX + cardWidthOnScreen || my > screenPosY + cardHeightOnScreen)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool IsBetSelected(s32 index,  u32 scrW, u32 scrH, u32 mx, u32 my)
{
	u32 startX = 12;
	u32 startY = 12; 

	u32 endX = scrW - startX * 2;

	u32 screenPosX = endX - index * 100 - 100 + 1;
	u32 screenPosY = startY;

	u32 width = 100;
	u32 height = 280;
	
	if (mx < screenPosX || my < screenPosY)
		return false;
	if (mx > screenPosX + width || my > screenPosY + height)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

void UpdateScene(u64 delta, const entry::KeyState& keyState, const entry::MouseState& mouseState, const entry::MouseState& oldMouseState, EGameState& gameState, uint32_t tableCards[5], uint32_t heldCards[5],
	uint32_t& tableBet, uint32_t& tableCredit, uint32_t deck[52], uint32_t& winIndex, u32 scrW, u32 scrH)
{
	bool wasMouseReleased = !mouseState.m_buttons[entry::MouseButton::Left] && oldMouseState.m_buttons[entry::MouseButton::Left];
	u32 mx = mouseState.m_mx;
	u32 my = mouseState.m_my;
	//BX_UNUSED(delta);
	switch (gameState)
	{
	case STATE_WAIT_BET:
	{
		//check if we clicked in any row to set bet 

		if (keyState.m_keysDown[entry::Key::Plus])
		{
			tableBet++;

			if (tableBet > 5)
			{
				tableBet = 1;
			}
		}

		bool betSelected = false;
		for (u32 i = 1; i < 6; i++)
		{
			if (wasMouseReleased && IsBetSelected( 5 - i , scrW, scrH, mx, my))
			{
				tableBet = i;
				betSelected = true;
			}
		}

		if (keyState.m_keysDown[entry::Key::Return] || (wasMouseReleased && !betSelected))
		{
			if (tableCredit >= tableBet)
			{
				gameState = STATE_DEAL;
				tableCredit -= tableBet;
			}
			else
			{
				//sound or flash credit ??
			}
		}

		break;
	}

	case STATE_DEAL:
	{
		std::fill_n(deck, 52, 52);

		for (u32 k = 0; k < 5; k++)
		{
			bool found = false;
			while (!found)
			{
				u32 card = rand() % 52;
				if (deck[card] == 52)
				{
					tableCards[k] = card;
					deck[card] = card;
					found = true;
				}
			}
		}

		gameState = STATE_WAIT_SELECT;

		std::fill_n(heldCards, 5, 0);

		winIndex = CheckWin(tableCards);

		break;
	}

	case STATE_WAIT_SELECT:
	{

		
	
		bool card0Clicked = wasMouseReleased && IsCardSelected(0, tableCards, 12, scrH / 2 - 10, scrW, scrH, mx, my);

		if (keyState.m_keysDown[entry::Key::Key1] || card0Clicked)
		{
			heldCards[0] = !heldCards[0];
		}

		bool card1Clicked = wasMouseReleased && IsCardSelected(1, tableCards, 12, scrH / 2 - 10, scrW, scrH, mx, my);
		if (keyState.m_keysDown[entry::Key::Key2] || card1Clicked)
		{
			heldCards[1] = !heldCards[1];
		}
		bool card2Clicked = wasMouseReleased && IsCardSelected(2, tableCards, 12, scrH / 2 - 10, scrW, scrH, mx, my);
		if (keyState.m_keysDown[entry::Key::Key3] || card2Clicked)
		{
			heldCards[2] = !heldCards[2];
		}
		bool card3Clicked = wasMouseReleased && IsCardSelected(3, tableCards, 12, scrH / 2 - 10, scrW, scrH, mx, my);
		if (keyState.m_keysDown[entry::Key::Key4] || card3Clicked)
		{
			heldCards[3] = !heldCards[3];
		}
		bool card4Clicked = wasMouseReleased && IsCardSelected(4, tableCards, 12, scrH / 2 - 10, scrW, scrH, mx, my);
		if (keyState.m_keysDown[entry::Key::Key5] || card4Clicked)
		{
			heldCards[4] = !heldCards[4];
		}

		bool noCardSelected = wasMouseReleased && !card0Clicked && !card1Clicked && !card2Clicked && !card3Clicked && !card4Clicked;
		if (keyState.m_keysDown[entry::Key::Return] || noCardSelected)
		{
			gameState = STATE_DEAL_EXTRA;
		}

			break;
		}

		case STATE_DEAL_EXTRA:
		{
			for (u32 k = 0; k < 5; k++)
			{
				if (!heldCards[k])
				{
					bool found = false;
					while (!found)
					{
						u32 card = rand() % 52;
						if (deck[card] == 52)
						{
							tableCards[k] = card;
							deck[card] = card;
							found = true;
						}
					}
				}
			}

			std::fill_n(heldCards, 5, 0);
			gameState = STATE_CHECK_WIN;

			break;
		}

		case STATE_CHECK_WIN:
		{
			winIndex = CheckWin(tableCards);
			gameState = STATE_SHOW_WIN;

			if (winIndex)
			{
				tableCredit += wins_payout[winIndex - 1] * tableBet;
			}

			//winTime = 3000;

			break;
		}

		case STATE_SHOW_WIN:
		{
			if (keyState.m_keysDown[entry::Key::Return] || wasMouseReleased)
			{
				gameState = STATE_WAIT_BET;
				winIndex = 0;

				std::fill_n(tableCards, 5, 52);
				std::fill_n(deck, 52, 52);
			}

			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

int _main_(int /*_argc*/, char** /*_argv*/)
{
	uint32_t deck[52];
	uint32_t deckIndex;
	uint32_t tableCards[5] = {0};
	uint32_t heldCards[5] = {0};
	uint32_t tableIndex;
	uint32_t tableBet;
	uint32_t tableCredit;
	uint32_t winIndex;
	int64_t winTime;
	
	EGameState gameState;

	uint32_t width = 1024;
	uint32_t height = 768;
	uint32_t debug = BGFX_DEBUG_TEXT;
	uint32_t reset = BGFX_RESET_VSYNC;

	bgfx::init();
	bgfx::reset(width, height, reset);

	// Enable debug text.
	bgfx::setDebug(debug);

	// Set view 0 clear state.
	bgfx::setViewClear(0
		, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
		, 0x303030ff
		, 1.0f
		, 0
		);

	srand(time(NULL));

	NVGcontext* nvg = nvgCreate(1024, 1024, 1, 0);
	bgfx::setViewSeq(0, true);
	//bgfx::TextureHandle image = loadTexture("oxy_white.png", BGFX_TEXTURE_U_CLAMP | BGFX_TEXTURE_V_CLAMP);

	auto bnd_font = nvgCreateFont(nvg, "droidsans", "font/droidsans.ttf");
	
	u32 image = nvgCreateImage(nvg, "textures/oxy_white.png");


	gameState = STATE_WAIT_BET;
	tableBet = 1;
	tableCredit = 100;
	winIndex = 0;

	std::fill_n(tableCards, 5, 52);
	std::fill_n(deck, 52, 52);
	
	u64 lastTime = bx::getHPCounter();
	//const int64_t freq = bx::getHPFrequency();

	/*int64_t now =;
	static int64_t last = now;
	const int64_t frameTime = now - last;
	last = now;
	
	
*/
	entry::MouseState mouseState;
	entry::MouseState oldMouseState;
	entry::KeyState keyState;

	while (!entry::processEvents(width, height, debug, reset, &mouseState, &keyState) )
	{
		// Set view 0 default viewport.
		bgfx::setViewRect(0, 0, 0, width, height);

		// This dummy draw call is here to make sure that view 0 is cleared
		// if no other draw calls are submitted to view 0.
		bgfx::submit(0);

		// Use debug font to print information about this example.
		bgfx::dbgTextClear();
		//bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/00-helloworld");
		//bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: Initialization and debug text.");

		u64 now = bx::getHPCounter();
		//const double toMs = 1000.0 / double(freq);

		float time = (float)((now - lastTime) / double(bx::getHPFrequency()));
		u64 delta = time > lastTime ? time - lastTime : 0;

		lastTime = time;

		UpdateScene(delta, keyState, mouseState, oldMouseState, gameState, tableCards, heldCards, tableBet, tableCredit, deck, winIndex, width, height);

		nvgBeginFrame(nvg, width, height, 1.0f, NVG_STRAIGHT_ALPHA);

		DrawScene(nvg, image, bnd_font, width, height, winIndex, tableBet, tableCards, heldCards, tableCredit, gameState);
		
		nvgEndFrame(nvg);

		// Advance to next frame. Rendering thread will be kicked to 
		// process submitted rendering primitives.
		bgfx::frame();
		oldMouseState = mouseState;
	}

	nvgDelete(nvg);
	// Shutdown bgfx.
	bgfx::shutdown();

	return 0;
}
