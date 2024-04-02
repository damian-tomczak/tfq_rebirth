/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\AsyncConsole.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\ResMngr.hpp"
#include "..\Framework\QMesh.hpp"
#include "..\Framework\Multishader.hpp"
#include "..\Framework\Gfx2D.hpp"
#include "..\Framework\GUI_PropertyGridWindow.hpp"
#include "..\Engine\Engine.hpp"
#include "..\Engine\QMap.hpp" // Potrzebne by zarejestrowaæ klasê zasobu
#include "..\Engine\Terrain.hpp" // Potrzebne by zarejestrowaæ klasê zasobu
#include "DebugGui.hpp"
#include "GameBase.hpp"
#include "GamePacman.hpp"
#include "GameRts.hpp"
#include "GameSpace.hpp"
#include "GameFpp.hpp"
#include "GameRpg.hpp"

#define USE_FILE_LOG

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Sta³e

const string APPLICATION_TITLE = "The Final Quest - Final Demo";
// Flaga bitowa do typu komunikatów loggera
const uint4 LOG_APPLICATION = 0x02;

// Numery gier bêd¹ 0..GAME_COUNT. 0 oznacza brak gry.
const uint GAME_COUNT = 5;
const uint FIRST_GAME = 1;

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Application

class Application : public frame::IFrameObject, public frame::IInputObject
{
public:
	Application();
	~Application();

	////// Implementacja frame::IFrameObject
	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnLostDevice();
	virtual void OnResetDevice();
	virtual void OnRestore();
	virtual void OnMinimize();
	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual void OnResume();
	virtual void OnPause();
	virtual void OnTimer(uint4 TimerId);

	////// Implementaja frame::IInputObject
	virtual bool OnKeyDown(uint4 Key) { return false; } // nieu¿ywana
	virtual bool OnKeyUp(uint4 Key) { return false; } // nieu¿ywana
	virtual bool OnChar(char Ch) { return false; } // nieu¿ywana
	virtual bool OnMouseMove(const common::VEC2 &Pos);
	virtual bool OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	virtual bool OnMouseWheel(const common::VEC2 &Pos, float Delta);

	void OnFrame();
	void OnUnhandledKeyDown(uint4 Key);
	void OnUnhandledKeyUp(uint4 Key);
	void OnUnhandledChar(char Ch);

private:
	scoped_ptr<GameBase> m_Game;
	// 0 = (brak)
	// 1 = PacMan
	uint m_GameIndex;

	void HandleSettingsChangeState();
	void HandleConsoleCommand();
	void DrawStats();
	void DrawTextHint(const string &Text);
	void GuiCursorChange(gui::Cursor *Cursor);
	const char * CursorTypeToWinCursor(gui::CURSOR_TYPE CursorType);
	void Draw2D();
	void CalcFrame();
	void CreatePropertyGrid_General(gui::PropertyGridWindow *pg);
	void OnSetLighting(const bool &v);
	void OnSetSmEnabled(const bool &v);
	void OnSetPpEnabled(const bool &v);
	void OnSelectGame(uint GameId);

	void CreateGame(uint GameIndex);
	void DestroyGame();
};

scoped_ptr<Application> g_App;

Application::Application() :
	m_Game(NULL),
	m_GameIndex(0)
{
	common::GetLogger().Log(LOG_APPLICATION, "Ctor");
}

Application::~Application()
{
	common::GetLogger().Log(LOG_APPLICATION, "Dtor");
}

void Application::OnCreate()
{
	ERR_TRY;

	common::GetLogger().Log(LOG_APPLICATION, "OnCreate");

	// Manager zasobów
	res::g_Manager = new res::ResManager();
	res::RegisterTypesD3d();
	res::QMesh::RegisterResourceType();
	res::Multishader::RegisterResourceType();
	engine::QMap::RegisterResourceType();
	engine::Terrain::RegisterResourceType();
	//res::Mesh::RegisterResourceType();
	res::g_Manager->CreateFromFile("Framework\\Resources.dat");
	res::g_Manager->CreateFromFile("Game\\Resources.dat");
	res::g_Manager->CreateFromFile("Engine\\Resources.dat");

	// Gfx2D
	gfx2d::g_SpriteRepository = new gfx2d::SpriteRepository();
	gfx2d::g_SpriteRepository->CreateSpritesFromFile("Framework\\Sprites.dat");
	gfx2d::g_SpriteRepository->CreateSpritesFromFile("Game\\Sprites.dat");

	// GUI (dodatkowa inicjalizacja)
	gui::g_GuiManager->OnCursorChange.bind(this, &Application::GuiCursorChange);
	gui::g_StandardCursors[gui::CT_NORMAL] = new gui::DragData();
	gui::g_StandardCursors[gui::CT_EDIT] = new gui::DragData();
	gui::g_StandardCursors[gui::CT_LINK] = new gui::DragData();
	gui::g_StandardCursors[gui::CT_SIZE_H] = new gui::DragData();
	gui::g_StandardCursors[gui::CT_SIZE_V] = new gui::DragData();
	gui::g_StandardCursors[gui::CT_SIZE_LT] = new gui::DragData();
	gui::g_StandardCursors[gui::CT_SIZE_RT] = new gui::DragData();
	gui::g_StandardCursors[gui::CT_MOVE] = new gui::DragData();

	// GUI Controls
	gui::InitGuiControls(
		res::g_Manager->MustGetResourceEx<res::Font>("Font02"),
		gfx2d::g_SpriteRepository->MustGetSprite("GUI"));

	// DebugGui
	g_DebugGui = new DebugGui(fastdelegate::FastDelegate1<uint>(this, &Application::OnSelectGame));

	// Engine
	engine::g_Engine = new engine::Engine();

	// Property grid z ustawieniami silnika do Debug GUI
	CreatePropertyGrid_General(g_DebugGui->GetPropertyGridWindow(0));

	// Gra
	CreateGame(FIRST_GAME);

	g_DebugGui->ResolutionChanged();

	ERR_CATCH_FUNC;
}

void Application::OnDestroy()
{
	DestroyGame();

	SAFE_DELETE(engine::g_Engine);

	SAFE_DELETE(g_DebugGui);

	// GUI
	delete gui::g_StandardCursors[gui::CT_NORMAL];
	delete gui::g_StandardCursors[gui::CT_EDIT];
	delete gui::g_StandardCursors[gui::CT_LINK];
	delete gui::g_StandardCursors[gui::CT_SIZE_H];
	delete gui::g_StandardCursors[gui::CT_SIZE_V];
	delete gui::g_StandardCursors[gui::CT_SIZE_LT];
	delete gui::g_StandardCursors[gui::CT_SIZE_RT];
	delete gui::g_StandardCursors[gui::CT_MOVE];

	SAFE_DELETE(gfx2d::g_SpriteRepository);
	SAFE_DELETE(res::g_Manager);

	common::GetLogger().Log(LOG_APPLICATION, "OnDestroy");
}

void Application::OnLostDevice()
{
	engine::g_Engine->OnLostDevice();
	res::g_Manager->Event(res::EVENT_D3D_LOST_DEVICE, 0);

	common::GetLogger().Log(LOG_APPLICATION, "OnLostDevice");
}

void Application::OnResetDevice()
{
	common::GetLogger().Log(LOG_APPLICATION, Format("OnResetDevice. Resolution=#x#") % frame::Settings.BackBufferWidth % frame::Settings.BackBufferHeight);

	//m_Camera->SetAspect(static_cast<float>(frame::Settings.BackBufferWidth) / frame::Settings.BackBufferHeight);

	res::g_Manager->Event(res::EVENT_D3D_RESET_DEVICE, 0);
	g_DebugGui->ResolutionChanged();
	engine::g_Engine->OnResetDevice();
	if (m_Game != NULL)
		m_Game->OnResolutionChanged();
}

void Application::OnRestore()
{
	common::GetLogger().Log(LOG_APPLICATION, "OnRestore");

	frame::Timer1.Resume();
	frame::Timer2.Resume();
	frame::SetLoopMode(true);
}

void Application::OnMinimize()
{
	frame::SetLoopMode(false);
	frame::Timer2.Pause();
	frame::Timer1.Pause();

	common::GetLogger().Log(LOG_APPLICATION, "OnMinimize");
}

void Application::OnActivate()
{
	common::GetLogger().Log(LOG_APPLICATION, "OnActivate");
}

void Application::OnDeactivate()
{
	common::GetLogger().Log(LOG_APPLICATION, "OnDeactivate");
}

void Application::OnResume()
{
	common::GetLogger().Log(LOG_APPLICATION, "OnResume");
	frame::Timer1.Resume();
	frame::Timer2.Resume();
}

void Application::OnPause()
{
	frame::Timer2.Pause();
	frame::Timer1.Pause();
	common::GetLogger().Log(LOG_APPLICATION, "OnPause");
}

void Application::OnTimer(uint4 TimerId)
{
}

bool Application::OnMouseMove(const common::VEC2 &Pos)
{
	if (m_Game != NULL)
		m_Game->OnMouseMove(Pos);

	return true;
}

bool Application::OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	g_DebugGui->HideMenu();

	if (m_Game != NULL)
		m_Game->OnMouseButton(Pos, Button, Action);

	return true;
}

bool Application::OnMouseWheel(const common::VEC2 &Pos, float Delta)
{
	if (m_Game != NULL)
		m_Game->OnMouseWheel(Pos, Delta);

	return true;
}

void Application::OnFrame()
{
	// Ustaw custom prefix loggera na numer nowej klatki
	string FrameNumber_s;
	UintToStr(&FrameNumber_s, frame::GetFrameNumber());
	common::GetLogger().SetCustomPrefixInfo(0, FrameNumber_s);

	// Narysowanie
	if (frame::Dev && !frame::GetDeviceLost())
	{
		ERR_GUARD_DIRECTX( frame::Dev->BeginScene() );

		////// 3D

		engine::g_Engine->Draw();

		if (g_DebugGui->GetShowTextures())
			engine::g_Engine->DrawSpecialTextures();

		////// 2D

		Draw2D();

		ERR_GUARD_DIRECTX( frame::Dev->EndScene() );
	}

	// Stan ¿¹dania zmiany trybu graficznego
	HandleSettingsChangeState();

	// Polecenie konsoli
	HandleConsoleCommand();

	CalcFrame();

	res::g_Manager->OnFrame();
	gui::g_GuiManager->OnFrame();
	engine::g_Engine->Update();
}

void Application::OnUnhandledKeyDown(uint4 Key)
{
	if (g_DebugGui->Shortcut(Key))
		return;

	switch (Key)
	{
	case VK_ESCAPE:
		frame::Exit();
		break;
	case VK_PAUSE:
		frame::Timer2.TogglePause();
		break;
	case VK_F5:
		{
			uint GameIndex = m_GameIndex;
			DestroyGame();
			CreateGame(GameIndex);
		}
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
		OnSelectGame(Key - '0');
		break;
	case VK_RETURN:
		if (frame::GetKeyboardKey(VK_MENU) || frame::GetKeyboardKey(VK_CONTROL))
		{
			if (frame::Settings.FullScreen)
			{
				frame::SETTINGS s = frame::Settings;
				s.BackBufferWidth = 800;
				s.BackBufferHeight = 600;
				s.FullScreen = false;
				frame::ChangeSettings(s);
			}
			else
			{
				frame::SETTINGS s = frame::Settings;
				s.BackBufferWidth = 1024;
				s.BackBufferHeight = 768;
				s.FullScreen = true;
				frame::ChangeSettings(s);
			}
		}
		break;
	}
}

void Application::OnUnhandledKeyUp(uint4 Key)
{
}

void Application::OnUnhandledChar(char Ch)
{
}

void Application::HandleSettingsChangeState()
{
	frame::SETTINGS_CHANGE_STATE scs = frame::GetSettingsChangeState();
	if (scs != frame::SCS_NONE)
	{
		switch (scs)
		{
		case frame::SCS_PENDING:
			common::GetLogger().Log(LOG_APPLICATION, "SettingsChangeState=PENDING");
			break;
		case frame::SCS_SUCCEEDED:
			common::GetLogger().Log(LOG_APPLICATION, "SettingsChangeState=SUCCEEDED");
			g_DebugGui->DisplaySettingsChanged(true);
			frame::ResetSettingsChangeState();
			break;
		case frame::SCS_FAILED:
			common::GetLogger().Log(LOG_APPLICATION, "SettingsChangeState=FAILED");
			g_DebugGui->DisplaySettingsChanged(false);
			frame::ResetSettingsChangeState();
			break;
		}
	}
}

void Application::HandleConsoleCommand()
{
	string s;
	if (g_AsyncConsole->GetInput(&s))
	{
		try
		{
			Tokenizer Tok(&s, 0);
			Tok.Next();
			if (!Tok.QueryEOF())
			{
				string Cmd;
				Tok.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
				Cmd = Tok.GetString();
				Tok.Next();
			}
		}
		catch (const Error &e)
		{
			string ErrMsg;
			e.GetMessage_(&ErrMsg, "  ", "\r\n");
			g_AsyncConsole->Writeln("B³¹d polecenia konsoli:");
			g_AsyncConsole->Write(ErrMsg);
		}
	}
}

void Application::DrawStats()
{
	uint4 DrawCount, PrimitiveCount;
	res::STATS ResStats;
	frame::GetDrawStats(&DrawCount, &PrimitiveCount);
	res::g_Manager->GetStats(&ResStats);
	string EngineInfo; engine::g_Engine->GetInfo(&EngineInfo);
	float FrameTime = (frame::GetFPS() == 0.0f ? 0.0f : 1.0f / frame::GetFPS()) *1000.0f;

	string Stats = Format(
		"Framework: FPS=# (Frame=# ms), Draws=#, Primitives=#\n"
		"ResMngr: Resources=#, Loaded=#, Locked=#\n"
		"Engine: #") %
		frame::GetFPS() % FrameTime % DrawCount % PrimitiveCount %
		ResStats.ResourceCount % ResStats.LoadedCount % ResStats.LockedCount %
		EngineInfo;

	gfx2d::g_Canvas->SetFont(res::g_Manager->MustGetResourceExf<res::Font>("Font02"));
	gfx2d::g_Canvas->SetColor(COLOR::WHITE);
	gfx2d::g_Canvas->DrawText_(gui::MARGIN, frame::GetScreenHeight()-gui::MARGIN, Stats, 12.0f,
		res::Font::FLAG_HLEFT | res::Font::FLAG_VBOTTOM | res::Font::FLAG_WRAP_NORMAL, 0.0f);
}

void Application::DrawTextHint(const string &Text)
{
	if (Text.empty()) return;

	static const float MAX_WIDTH = frame::GetScreenWidth() * 0.5f;
	static const float MARGIN = 8.0f;
	static const float PADDING = 8.0f;
	float TextWidth, TextHeight;
	gui::g_Font->GetTextExtent(&TextWidth, &TextHeight, Text, gui::g_FontSize,
		res::Font::FLAG_HLEFT | res::Font::FLAG_VBOTTOM | res::Font::FLAG_WRAP_WORD, MAX_WIDTH);
	// T³o
	RECTF R;
	R.left = MARGIN;
	R.bottom = frame::GetScreenHeight()-MARGIN;
	R.right = R.left + PADDING*2 + TextWidth;
	R.top = R.bottom - PADDING*2 - TextHeight;
	gfx2d::g_Canvas->SetSprite(NULL);
	gfx2d::g_Canvas->SetColor(gui::g_HintBkgColor);
	gfx2d::g_Canvas->DrawRect(R);
	// Napis
	R.Extend(-PADDING);
	gfx2d::g_Canvas->SetFont(gui::g_Font);
	gfx2d::g_Canvas->SetColor(gui::g_HintFontColor);
	gfx2d::g_Canvas->DrawText_(R.left, R.bottom, Text, gui::g_FontSize,
		res::Font::FLAG_HLEFT | res::Font::FLAG_VBOTTOM | res::Font::FLAG_WRAP_WORD, MAX_WIDTH);
}

void Application::GuiCursorChange(gui::Cursor *Cursor)
{
	const char *WinCursor = NULL;

	if (Cursor == NULL)
	{
		WinCursor = IDC_ARROW;
	}
	else if (typeid(*Cursor) == typeid(gui::DragCursor))
	{
		WinCursor = CursorTypeToWinCursor(static_cast<gui::DragCursor*>(Cursor)->GetCursorType());
	}
	else if (typeid(*Cursor) == typeid(gui::DragData))
	{
		for (size_t i = 0; i < gui::CT_COUNT; i++)
		{
			if (Cursor == gui::g_StandardCursors[i])
			{
				WinCursor = CursorTypeToWinCursor((gui::CURSOR_TYPE)i);
				break;
			}
		}
	}

	if (WinCursor != NULL)
	{
		HCURSOR ResCursor = LoadCursor(0, WinCursor);
		// Uwaga! Tu jest ciekawe zjawisko.
		// Otó¿ samo SetClassLong sprawi, ¿e zmiana kursora wejdzie w ¿ycie dopiero po poruszeniu myszk¹.
		// Z kolei SetCursor zmienia kursor natychmiast, ale zmiana dzia³a tylko do ruszenia myszk¹.
		// Dlatego trzeba zastosowaæ oba.
		SetCursor(ResCursor);
		SetClassLong(frame::Wnd, GCL_HCURSOR, (LONG)ResCursor);
	}
}

const char * Application::CursorTypeToWinCursor(gui::CURSOR_TYPE CursorType)
{
	if (CursorType == gui::CT_NORMAL)
		return IDC_ARROW;
	else if (CursorType == gui::CT_EDIT)
		return IDC_IBEAM;
	else if (CursorType == gui::CT_LINK)
		return IDC_HAND;
	else if (CursorType == gui::CT_SIZE_H)
		return IDC_SIZEWE;
	else if (CursorType == gui::CT_SIZE_V)
		return IDC_SIZENS;
	else if (CursorType == gui::CT_SIZE_LT)
		return IDC_SIZENWSE;
	else if (CursorType == gui::CT_SIZE_RT)
		return IDC_SIZENESW;
	else if (CursorType == gui::CT_MOVE)
		return IDC_SIZEALL;
	else
		return IDC_ARROW;
}

void Application::Draw2D()
{
	frame::RestoreDefaultState();
	frame::Dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	frame::Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	D3DXMATRIX ProjMat;
	frame::GetMouseCoordsProjection(&ProjMat);
	frame::Dev->SetTransform(D3DTS_PROJECTION, &ProjMat);

	if (m_Game != NULL)
		m_Game->Draw2D();

	// Statystyki
	if (g_DebugGui->GetShowStats())
		DrawStats();

	// GUI
	gui::g_GuiManager->OnDraw();

	// Hint tekstowy
	gui::Hint *Hint = gui::g_GuiManager->GetCurrentHint();
	if (Hint && typeid(*Hint) == typeid(gui::StringHint))
		DrawTextHint(static_cast<gui::StringHint*>(Hint)->GetText());

	// FLUSH
	gfx2d::g_Canvas->Flush();
}

void Application::CalcFrame()
{
	if (m_Game != NULL)
	{
		int R = m_Game->CalcFrame();
		if (R == 1)
		{
			if (m_GameIndex == GAME_COUNT)
			{
				DestroyGame();
				frame::Exit();
			}
			else
			{
				uint GameIndex = m_GameIndex;
				DestroyGame();
				CreateGame(GameIndex + 1);
			}
		}
		else if (R == -1)
		{
			uint GameIndex = m_GameIndex;
			DestroyGame();
			CreateGame(GameIndex);
		}
	}
}

void Application::CreatePropertyGrid_General(gui::PropertyGridWindow *pg)
{
	pg->AddProperty(1, "Oœwietlenie",
		new gui::BoolProperty(pg, engine::g_Engine->GetConfigBool(engine::Engine::O_LIGHTING),
		NULL, fastdelegate::MakeDelegate(this, &Application::OnSetLighting)),
		"Czy wszelkie mechanizmy oœwietlenia maj¹ byæ w³¹czone?\n"
		"Wy³¹czaæ tylko w ostatecznoœci (coœ nie dzia³a albo dzia³a zbyt wolno)!");
	pg->AddProperty(2, "Shadow Mapping",
		new gui::BoolProperty(pg, engine::g_Engine->GetConfigBool(engine::Engine::O_SM_ENABLED),
		NULL, fastdelegate::MakeDelegate(this, &Application::OnSetSmEnabled)),
		"Czy wszelkie mechanizmy cienia (technika Shadow Mapping) maj¹ byæ w³¹czone?");
	pg->AddProperty(4, "Postprocessing",
		new gui::BoolProperty(pg, engine::g_Engine->GetConfigBool(engine::Engine::O_PP_ENABLED),
		NULL, fastdelegate::MakeDelegate(this, &Application::OnSetPpEnabled)),
		"Czy Postprocessing jest w³¹czony?\n"
		"Wy³¹cz, jeœli nie dzia³a, dzia³a Ÿle lub przez niego silnik siê wysypuje (mo¿e szwankowaæ na ró¿nych kartach bo czêsto wykorzystuje renderowanie do tekstury).");
}

void Application::OnSetLighting(const bool &v)
{
	engine::g_Engine->SetConfigBool(engine::Engine::O_LIGHTING, v);
}

void Application::OnSetSmEnabled(const bool &v)
{
	engine::g_Engine->SetConfigBool(engine::Engine::O_SM_ENABLED, v);
}

void Application::OnSetPpEnabled(const bool &v)
{
	engine::g_Engine->SetConfigBool(engine::Engine::O_PP_ENABLED, v);
}

void Application::OnSelectGame(uint GameId)
{
	if (m_GameIndex != GameId)
	{
		DestroyGame();
		CreateGame(GameId);
	}
}

void Application::CreateGame(uint GameIndex)
{
	LOG(LOG_GAME, Format("CreateGame: #") % GameIndex);

	switch (GameIndex)
	{
	case 1:
		m_Game.reset(new GamePacman());
		break;
	case 2:
		m_Game.reset(new GameRts());
		break;
	case 3:
		m_Game.reset(new GameSpace());
		break;
	case 4:
		m_Game.reset(new GameFpp());
		break;
	case 5:
		m_Game.reset(new GameRpg());
		break;
	default:
		m_Game.reset();
	}

	m_GameIndex = GameIndex;
}

void Application::DestroyGame()
{
	m_GameIndex = 0;
	m_Game.reset();

	LOG(LOG_GAME, "DestroyGame");
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje globalne

void OnFrame()
{
	if (g_App != NULL)
		g_App->OnFrame();
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE, char *CmdLine, int CmdShow)
{
	int R = -1;

	//_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

	try
	{
		try
		{
			// Logger i asynchroniczna konsola
			common::CreateLogger(false);
			common::Logger & Logger = common::GetLogger();

			g_AsyncConsole.reset(new AsyncConsole);
			g_AsyncConsole->SetTitle(APPLICATION_TITLE);
			g_AsyncConsole->SetOutputColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			g_AsyncConsole->SetInputColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);

			scoped_ptr<AsyncConsoleLog> AsyncConsoleLog(new AsyncConsoleLog);
			Logger.AddLogMapping(0xFFFFFFFF, AsyncConsoleLog.get());

#ifdef USE_FILE_LOG
			scoped_ptr<common::TextFileLog> TextFileLog(new common::TextFileLog("Log.txt", common::FILE_MODE_NORMAL, EOL_CRLF));
			Logger.AddLogMapping(0xFFFFFFFF, TextFileLog.get());
#endif

			Logger.SetPrefixFormat("[F:%1] ");
			Logger.SetCustomPrefixInfo(0, "-");

			Logger.AddTypePrefixMapping(0x02, "App: ");
			Logger.AddTypePrefixMapping(0x04, "Engine: ");
			Logger.AddTypePrefixMapping(0x08, "ResMngr: ");
			Logger.AddTypePrefixMapping(LOG_GAME, "Game: ");

			AsyncConsoleLog->AddColorMapping(0x02, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE);
			AsyncConsoleLog->AddColorMapping(0x04, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
			AsyncConsoleLog->AddColorMapping(0x08, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE);

			// Obiekt aplikacji
			g_App.reset(new Application);

			// Obiekt gfx2d.Canvas
			gfx2d::g_Canvas = new gfx2d::Canvas();
			// Obiekt GUI
			gui::g_GuiManager = new gui::GuiManager();

			// Inicjalizacja frameworka
			frame::RegisterFrameObject(gfx2d::g_Canvas);
			frame::RegisterFrameObject(g_App.get());
			frame::RegisterInputObject(gui::g_GuiManager);
			frame::RegisterInputObject(g_App.get());
			gui::g_GuiManager->OnUnhandledKeyDown.bind(g_App.get(), &Application::OnUnhandledKeyDown);
			gui::g_GuiManager->OnUnhandledKeyUp.bind(g_App.get(), &Application::OnUnhandledKeyUp);
			gui::g_GuiManager->OnUnhandledChar.bind(g_App.get(), &Application::OnUnhandledChar);

			frame::SetWindowTitle(APPLICATION_TITLE);
			frame::SetLoopMode(true);

			// Odpalenie
			R = frame::Go(
				Instance,
				"THE_FINAL_QUEST",
				LoadIcon(0, IDI_APPLICATION),
				LoadCursor(0, IDC_ARROW),
				true,
				0, 0, 0, 0,
				true,
				frame::MC_PIXELS, 800.0f, 600.0f,
				&OnFrame);

			// Koniec GUI
			delete gui::g_GuiManager;
			// Koniec gfx2d.Canvas
			delete gfx2d::g_Canvas;

			// Koniec aplikacji
			g_App.reset(0);

			// Zamkniêcie loggera
			common::DestroyLogger();
#ifdef USE_FILE_LOG
			TextFileLog.reset(0);
#endif
			AsyncConsoleLog.reset(0);
		}
		catch (const common::Error &)
		{
			throw;
		}
		catch (...)
		{
			throw common::Error("Nienznay b³¹d w funkcji WinMain");
		}
	}
	catch (const common::Error &e)
	{
		frame::ProcessError(e);
	}

	// Koniec
	return R;
}

/*
Czas ¿ycia obiektów - Wielkie Drzewo ¯ycia :)
- main()
  - Logger
  - AsyncConsole
  - Application (jako obiekt)
  - gfx2d.Canvas
  - gui.GuiManager
  - frame.Go
    - Config
	- Application (metody On*)
	  - res.ResManager
	  - gfx2d.SpriteRepository
	  - gui.GuiManager
	  - GUI Controls (tylko wywo³anie funkcji inicjalizuj¹cej)
	  - DebugGui
	  - MathHelpDrawing
	  - Engine
*/
