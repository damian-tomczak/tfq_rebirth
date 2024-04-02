/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include <stack>
#include <list>
#include <fstream>
#include <commctrl.h> // do funkcji InitCommonControls()
#include <windowsx.h>
#include <shellapi.h> // do ShellExecute
#include "D3dUtils.hpp"
#include "Framework.hpp"

namespace frame
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Sta³e

const string CONFIG_FILENAME = "Config.dat";

// Parametry macierzy rzutowania 2D do uk³adu wspó³rzêdnych myszki
float Z_NEAR = 0.5f;
float Z_FAR = 100.0f;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Zmienne

// Ustawienia, których zmiana wymaga³aby ponownego tworzenia urz¹dzenia

// Adapter
// Zalecane: D3DADAPTER_DEFAULT.
uint4 g_Settings_Adapter = D3DADAPTER_DEFAULT;
// Rodzaj urz¹dzenia
// Zalecane: D3DDEVTYPE_HAL.
D3DDEVTYPE g_Settings_DeviceType = D3DDEVTYPE_HAL;
// Sposób przetwarzania wierzcho³ków
// Wartoœæ specjalna 0 oznacza, ¿eby automatycznie wybraæ najlepszy.
uint4 g_Settings_VertexProcessing = 0;

// Czy urz¹dzenie jest w stanie utracenia
bool g_DeviceLost = false;
// Czy okno jest zmaksymalizowane
bool g_Maximized = false;
// Czy okno jest zminimalizowane
bool g_Minimized = false;
// Czy jesteœmy wewn¹trz wywo³ania funkcji Go
bool g_InsideGo = false;
// Licznik zapauzowañ
// (0 - nie zapauzowany, ka¿dy powód do zapauzowania dodaje 1)
int g_PauseCounter = 0;
// Licznik aktywnoœci
// (0 - nieaktywny, ka¿dy powód do aktywnoœci dodaje 1)
int g_ActiveCounter = 0;
// Czy ostatnim wywo³aniem ActiveOn/ActiveOff by³o wywo³anie ActiveOn w reakcji na WM_ACTIVATE
// To jest zabezpieczenie przed czasami otrzymywanym dwukrotnie na raz komunikatem o aktywacji,
// który powodowa³ zwiêkszenie licznika dwa razy i w konsekwencji buraki.
bool g_LastWmActivate = false;

// ¯¹danie zmiany ustawieñ
SETTINGS g_NewSettings;
SETTINGS_CHANGE_STATE g_SettingsChangeState = SCS_NONE;

// Zapamiêtany domyœlny stan urz¹dzenia
LPDIRECT3DSTATEBLOCK9 g_DefaultState = NULL;
// Event do flushowania kolejki
// - NULL jeœli aktualnie nie ma lub jeœli nie u¿ywamy
IDirect3DQuery9 *g_FlushEventQuery = NULL;
// Powierzchnie u¿ywane do blokowania kiedy Settings.FlushMode == SETTINGS::FLUSH_LOCK
// Jeœli nieu¿ywane, NULL
IDirect3DSurface9 *g_FlushSurfaces[2] = { NULL, NULL };

// Tryb pêtlenia siê
bool g_LoopMode = true;
// Tytu³ okna
// Muszê go pamiêtaæ na wypadek jakby u¿ytkownik chcia³ to zmieniaæ w czasie
// kiedy okno jeszcze/ju¿ nie istnieje.
string g_WindowTitle = "Bez tytu³u";
// Czy pokazywaæ kursor systemowy
bool g_ShowSystemCursor = true;


// ==== Zmienne inicjalizacyjne ====

// Przekazywane do Go i podczas jej wywo³ania przypisywane, potem ju¿
// niezmienne.
string g_ClassName;
HICON g_Icon;
HCURSOR g_Cursor;
bool g_AllowResize;
int g_MinWidth, g_MinHeight, g_MaxWidth, g_MaxHeight;
bool g_CloseButton;
MOUSE_COORDS g_MouseCoords;
float g_MouseCoordsWidth, g_MouseCoordsHeight;
FRAME_FUNC g_OnFrame;


// ==== Czas i liczniki ====

bool g_Time_WasInited = false;
float g_OldTime = 0.0f;
// Aktualna wartoœæ licznika FPS
float g_FPS = 0.0f;
// Czas ostatniej aktualizacji licznika
float g_FPS_LastTime = 0.0f;
// Liczka klatek od ostatniej aktualizacji
int g_FPS_Frames = 0;
// Liczba klatek w ogóle
uint4 g_FrameNumber = 0;
// Liczba rysowañ w poprzedniej i w tej klatce
uint4 g_LastDrawCount = 0;
uint4 g_NewDrawCount = 0;
// Liczba prymitywów narysowanych w poprzedniej i w tej klatce
uint4 g_LastPrimitiveCount = 0;
uint4 g_NewPrimitiveCount = 0;


// ==== Wejœcie ====

// Automatycznie zerowana z definicji
BYTE g_Keys[256];
// Czy mysz jest w trybie kamery
bool g_MouseCameraMode = false;
// Zapamiêtana pozycja kursora dla trybu kamery
POINT g_MouseSavedPos;
// Zapamiêtana pozycja, gdzie prawdziwy kursor jest ustawiany dla trybu kamery
POINT g_MouseRealPos;
// Zapamiêtane, wczytane ustawienie systemu, czy klawisze myszy s¹ zamienione
bool g_MouseSwapButtons = false;
// Zapamiêtane, wczytane ustawienie systemu, o ile linii scrolluje scroll
int g_MouseWheelLines = 3;
// Czy nastêpny komunikat WM_MOUSEMOVE ma zostaæ zignorowany?
bool g_IgnoreNextMouseMove = false;

typedef std::list<IInputObject*> INPUT_OBJECT_LIST;
INPUT_OBJECT_LIST g_InputObjectList;

D3DXMATRIX g_Projection2D;
float g_AspectRatio = 0.0f;
float g_PixelsPerUnitX = 0.0f;
float g_PixelsPerUnitY = 0.0f;
float g_UnitsPerPixelX = 0.0f;
float g_UnitsPerPixelY = 0.0f;

Timer Timer1;
Timer Timer2;

// ==== Timery ====

uint4 g_NextTimerID = 1;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Obs³uga b³êdów

// ======== Implementacja ========

// Plik z logiem z b³êdami
const char *ERROR_LOG_FILENAME = "Errors.log";

string GetModuleFileName()
{
	char sz[MAX_PATH];
	::GetModuleFileName(NULL, sz, MAX_PATH);
	return string(sz);
}

string GetModulePath()
{
	string R;
	ExtractFilePath(&R, GetModuleFileName());
	return R;
}

void ErrorLog_Time(std::ostream &stream)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	char s[32];
	sprintf(s, "%.4u-%.2u-%.2u %.2u:%.2u:%.2u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	stream << "  Czas       : " << s << std::endl;
}

void ErrorLog_ComputerName(std::ostream &stream)
{
	const DWORD SIZE = MAX_COMPUTERNAME_LENGTH+1;
	DWORD dupa = SIZE;
	char lpsz[SIZE];
	if (GetComputerName(lpsz, &dupa))
		stream << "  Komputer   : " << lpsz << std::endl;
}

void ErrorLog_UserName(std::ostream &stream)
{
	const DWORD SIZE = 256/*UNLEN*/+1;
	DWORD dupa = SIZE;
	char lpsz[SIZE];
	if (GetUserName(lpsz, &dupa))
		stream << "  U¿ytkownik : " << lpsz << std::endl;
}

void ErrorLog_FileName(std::ostream &stream)
{
	stream << "  Plik       : " << GetModuleFileName() << std::endl;
}


// ======== INTERFEJS ========

// Przetwarza b³¹d, a konkretnie to pokazuje go i loguje do pliku
void ProcessError(const Error &e)
{
	std::ofstream log(ERROR_LOG_FILENAME, std::ios_base::out | std::ios_base::app);
	if (log)
	{
		// Œrodowisko
		log << "Œrodowisko" << std::endl;
		ErrorLog_Time(log);
		ErrorLog_ComputerName(log);
		ErrorLog_UserName(log);
		ErrorLog_FileName(log);

		string msg;
		// Dlaczego "\n"?
		// Bo jak siê poda "\r\n", to strumieñ zamieni na "\r\n\n" i bêdzie burak.
		// Jak siê z kolei otworzy plik w trybie std::ios_base::binary, to std::endl stanie siê samym "\n" i te¿ lipa.
		e.GetMessage_(&msg, "  ", "\n");
		log << "B³¹d" << std::endl;
		log << msg << std::endl;

		log << std::endl;
		log.close();

		// Nie pamiêtam, czemu kiedyœ nie chcia³em, ¿eby b³¹d pokazywa³ siê te¿ w okienku.
		// Musia³em mieæ jakiœ powód.
		// Teraz jak spróbowa³em to odkomentowaæ to nie zawsze siê pokazuje, np. nie pokazuje siê,
		// kiedy rzucê wyj¹tek w OnTimer.

		//string FullMsg;
		//e.GetMessage_(&FullMsg);
		//MessageBox(Wnd, FullMsg.c_str(), "B³¹d", MB_OK | MB_ICONERROR);

		ShellExecute(Wnd, NULL, ERROR_LOG_FILENAME, NULL, NULL, SW_SHOWNORMAL);
	}
}

// Pocz¹tek sekcji przechwytuj¹cej b³êdy
#define ERR_HANDLE_BEGIN try {
#define ERR_HANDLE_END   } \
	catch(Error &) \
	{ \
		throw; \
	} \
	catch(std::exception &e) \
	{ \
		throw Error(string("(std-c++) ") + e.what(), __FILE__, __LINE__); \
	} \
	catch(...) \
	{ \
		throw Error("Nieznany b³¹d (najprawdopodobniej b³¹d ochrony pamiêci)", __FILE__, __LINE__); \
	}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Kolekcja obiektów IFrameObject

class FrameObjectList
{
private:
	typedef void (IFrameObject::*FRAME_OBJECT_FUNC)();

	struct FrameObjectInfo
	{
		IFrameObject *fo;
		bool DoneCreate;
		FrameObjectInfo(IFrameObject *a_fo) : fo(a_fo), DoneCreate(false) { }
	};
	typedef bool FrameObjectInfo::*FRAME_OBJECT_INFO_FIELD;

	typedef std::list<FrameObjectInfo> FRAME_OBJECT_INFO_LIST;
	typedef FRAME_OBJECT_INFO_LIST::iterator FRAME_OBJECT_INFO_ITERATOR;
	typedef FRAME_OBJECT_INFO_LIST::reverse_iterator FRAME_OBJECT_INFO_REV_ITERATOR;

	FRAME_OBJECT_INFO_LIST m_List;

public:

	void Add(IFrameObject *a_fo)
	{
		m_List.push_back(FrameObjectInfo(a_fo));
	}

	void CallCreateFunc(FRAME_OBJECT_FUNC Func, FRAME_OBJECT_INFO_FIELD Field, const string &ErrMsg);
	void CallDestroyFunc(FRAME_OBJECT_FUNC Func, FRAME_OBJECT_INFO_FIELD Field);

	void CallCreate()      { CallCreateFunc (&IFrameObject::OnCreate,      &FrameObjectInfo::DoneCreate, "B³¹d w OnCreate"); }
	void CallDestroy()     { CallDestroyFunc(&IFrameObject::OnDestroy,     &FrameObjectInfo::DoneCreate); }
	void CallResetDevice() { CallCreateFunc (&IFrameObject::OnResetDevice, NULL, "B³¹d w OnReRestore"); }
	void CallLostDevice()  { CallDestroyFunc(&IFrameObject::OnLostDevice,  NULL); }
	void CallRestore()     { CallCreateFunc (&IFrameObject::OnRestore,     NULL, "B³¹d w OnRestore"); }
	void CallMinimize()    { CallDestroyFunc(&IFrameObject::OnMinimize,    NULL); }
	void CallActivate()    { CallCreateFunc (&IFrameObject::OnActivate,    NULL, "B³¹d w OnActivate"); }
	void CallDeactivate()  { CallDestroyFunc(&IFrameObject::OnDeactivate,  NULL); }
	void CallResume()      { CallCreateFunc (&IFrameObject::OnResume,      NULL, "B³¹d w OnResume"); }
	void CallPause()       { CallDestroyFunc(&IFrameObject::OnPause,       NULL); }
	void CallTimerFunc(uint4 TimerID);
};

FrameObjectList g_FrameObjectList;

void FrameObjectList::CallCreateFunc(FRAME_OBJECT_FUNC Func, FRAME_OBJECT_INFO_FIELD Field, const string &ErrMsg)
{
	try
	{
		for (FRAME_OBJECT_INFO_ITERATOR it = m_List.begin(); it != m_List.end(); ++it)
		{
			if (Field == 0 || !((*it).*Field))
			{
				if (Field)
					(*it).*Field = true;
				((*it).fo->*Func)();
			}
		}
	}
	catch(Error &e)
	{
		e.Push(ErrMsg, __FILE__, __LINE__);
		throw;
	}
}

void FrameObjectList::CallDestroyFunc(FRAME_OBJECT_FUNC Func, FRAME_OBJECT_INFO_FIELD Field)
{
	for (FRAME_OBJECT_INFO_REV_ITERATOR it = m_List.rbegin(); it != m_List.rend(); ++it)
	{
		if (Field == 0 || (*it).*Field)
		{
			if (Field)
				(*it).*Field = false;
			try
			{
				((*it).fo->*Func)();
			}
			catch (...)
			{
				// Przemilcz wyj¹tek.
			}
		}
	}
}

void FrameObjectList::CallTimerFunc(uint4 TimerID)
{
	try
	{
		for (FRAME_OBJECT_INFO_ITERATOR it = m_List.begin(); it != m_List.end(); ++it)
			it->fo->OnTimer(TimerID);
	}
	catch(Error &e)
	{
		e.Push("B³¹d w OnTimer", __FILE__, __LINE__);
		throw;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Kod

void FrameObjectsDeviceLost()
{
	SAFE_RELEASE(g_FlushEventQuery);
	SAFE_RELEASE(g_FlushSurfaces[1]);
	SAFE_RELEASE(g_FlushSurfaces[0]);
	SAFE_RELEASE(g_DefaultState);
}

void Timer::UpdateTime(float DeltaTime)
{
	if (m_WasPaused)
	{
		m_DeltaTime = 0.f;
		m_WasPaused = false;
	}
	else if (IsPaused())
		m_DeltaTime = 0.f;
	else
	{
		m_DeltaTime = DeltaTime;
		m_Time += DeltaTime;
	}
}

SETTINGS::SETTINGS(uint4 BackBufferWidth, uint4 BackBufferHeight, uint4 RefreshRate, bool FullScreen, D3DFORMAT DepthStencilFormat, bool VSync) :
	BackBufferWidth(BackBufferWidth),
	BackBufferHeight(BackBufferHeight),
	RefreshRate(RefreshRate),
	BackBufferFormat(D3DFMT_A8R8G8B8),
	BackBufferCount(2),
	MultiSampleType(D3DMULTISAMPLE_NONE),
	MultiSampleQuality(0),
	SwapEffect(D3DSWAPEFFECT_DISCARD),
	FullScreen(FullScreen),
	EnableAutoDepthStencil(DepthStencilFormat != D3DFMT_UNKNOWN),
	AutoDepthStencilFormat(DepthStencilFormat),
	DiscardDepthStencil(DepthStencilFormat != D3DFMT_UNKNOWN),
	LockableBackBuffer(false),
	PresentationInverval(VSync ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE),
	FlushMode(FLUSH_NONE)
{
}

// Wynik porównania dwóch ustawieñ
enum SETTINGS_COMP
{
	// Ustawienia identyczne
	SC_EQUAL,
	// Ustawienia ró¿ne ale zmianê da siê za³atwiæ bez manipulowania urz¹dzeniem
	SC_NEED_NOTHING_SPECIAL,
	// Ustawienia ró¿ne, wymagaj¹ tylko zresetowania urz¹dzenia
	SC_NEED_RESET,
};

// Porównuje dwa ustawienia
SETTINGS_COMP CompareSettings(const SETTINGS &Settings1, const SETTINGS &Settings2)
{
	if (
		Settings1.BackBufferWidth != Settings2.BackBufferWidth ||
		Settings1.BackBufferHeight != Settings2.BackBufferHeight ||
		(Settings1.RefreshRate != Settings2.RefreshRate && Settings1.FullScreen) ||
		Settings1.BackBufferFormat != Settings2.BackBufferFormat ||
		Settings1.BackBufferCount != Settings2.BackBufferCount ||
		Settings1.MultiSampleType != Settings2.MultiSampleType ||
		Settings1.MultiSampleQuality != Settings2.MultiSampleQuality ||
		Settings1.SwapEffect != Settings2.SwapEffect ||
		Settings1.FullScreen != Settings2.FullScreen ||
		Settings1.EnableAutoDepthStencil != Settings2.EnableAutoDepthStencil ||
		Settings1.AutoDepthStencilFormat != Settings2.AutoDepthStencilFormat ||
		Settings1.DiscardDepthStencil != Settings2.DiscardDepthStencil ||
		Settings1.LockableBackBuffer != Settings2.LockableBackBuffer ||
		Settings1.PresentationInverval != Settings2.PresentationInverval)
		return SC_NEED_RESET;
	else if (Settings1.FlushMode != Settings2.FlushMode)
		return SC_NEED_NOTHING_SPECIAL;
	else
		return SC_EQUAL;
}

void MouseSetRealPos()
{
	if (Wnd == 0) return;

	// Powinno byæ wzglêdem œrodka obszaru klienta, ale co tam...
	// A mo¿e jednak?!?! mo¿e wystarczy GetClientRect czy coœ???
	RECT rect;
	GetWindowRect(Wnd, &rect);
	g_MouseRealPos.x = (rect.left+rect.right)/2;
	g_MouseRealPos.y = (rect.top+rect.bottom)/2;
	SetCursorPos(g_MouseRealPos.x, g_MouseRealPos.y);
	ScreenToClient(Wnd, &g_MouseRealPos);
}

// Zwraca bie¿¹c¹ pozycjê kursora myszy we wspó³rzêdnych obszaru klienta okna aplikacji
void GetMousePos(POINT *Pos)
{
	if (g_MouseCameraMode && GetActive())
		*Pos = g_MouseSavedPos;
	else
	{
		::GetCursorPos(Pos);
		if (Wnd)
			ScreenToClient(Wnd, Pos);
	}
}

// Ustawia pozycjê kursora myszy we wspó³rzêdnych obszaru klienta okna aplikacji
void SetMousePos(const POINT &Pos)
{
	POINT my_pos = Pos;
	if (Wnd)
		ClientToScreen(Wnd, &my_pos);

	if (GetActive())
	{
		if (g_MouseCameraMode)
			g_MouseSavedPos = my_pos;
		else
			::SetCursorPos(my_pos.x, my_pos.y);
	}
}

void InputActivate()
{
	if (g_MouseCameraMode)
	{
		// Zapamiêtaj pozycjê kursora
		GetMousePos(&g_MouseSavedPos);
		// Ustaw pozycjê kursora na œrodek okna
		MouseSetRealPos();
	}
}

void InputDeactivate()
{
	if (g_MouseCameraMode)
	{
		// Przywróæ pozyzjê kursora
		SetMousePos(g_MouseSavedPos);
	}
}

void CreateDefaultState()
{
	assert(Dev != NULL);

	HRESULT hr = Dev->CreateStateBlock(D3DSBT_ALL, &g_DefaultState);
	if (FAILED(hr)) throw DirectXError(hr, "Nie mo¿na zapamiêtaæ domyœlnego stanu urz¹dzenia", __FILE__, __LINE__);
}

// Jeœli bie¿¹ce ustawienia mówi¹ TAK, utwórz Flush Event Query
void CreateFlushObjects()
{
	HRESULT hr;

	if (Settings.FlushMode == SETTINGS::FLUSH_EVENT)
	{
		hr = Dev->CreateQuery(D3DQUERYTYPE_EVENT, &g_FlushEventQuery);
		if (FAILED(hr)) throw DirectXError(hr, "CreateFlushObjects - CreateQuery(EVENT)", __FILE__, __LINE__);
	}
	else if (Settings.FlushMode == SETTINGS::FLUSH_LOCK)
	{
		hr = Dev->CreateOffscreenPlainSurface(16, 16, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_FlushSurfaces[0], NULL);
		if (FAILED(hr)) throw DirectXError(hr, "CreateFlushObjects - CreateOffscreenPlainSurface(g_FlushSurfaces[0])", __FILE__, __LINE__);
		hr = Dev->CreateOffscreenPlainSurface(16, 16, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_FlushSurfaces[1], NULL);
		if (FAILED(hr)) throw DirectXError(hr, "CreateFlushObjects - CreateOffscreenPlainSurface(g_FlushSurfaces[1])", __FILE__, __LINE__);
	}
}

// Jeœli Flush Event Query istnieje, u¿yj go do poczekania na opró¿nienie kolejki renderowania.
// Wywo³ywaæ tylko (co tu mia³o pisaæ???)
// - Zwraca true po pomyœlnym doczekaniu siê.
// - Zwraca false po wykryciu utraty urz¹dzenia.
bool UseFlushObjects()
{
	if (Settings.FlushMode == SETTINGS::FLUSH_EVENT)
	{
		HRESULT hr = g_FlushEventQuery->Issue(D3DISSUE_END);
		if (FAILED(hr)) throw DirectXError(hr, "IDirect3DQuery9(Event)::Issue(END)", __FILE__, __LINE__);
		BOOL Result;
		for (;;)
		{
			hr = g_FlushEventQuery->GetData(&Result, sizeof(BOOL), D3DGETDATA_FLUSH);
			if (hr == S_OK)
				return true;
			else if (hr == D3DERR_DEVICELOST)
				return false;
			Sleep(0);
		}
	}
	else if (Settings.FlushMode == SETTINGS::FLUSH_LOCK)
	{
		static int dblock;
		D3DLOCKED_RECT lr;
		volatile int dummy;

		Dev->ColorFill(g_FlushSurfaces[dblock], NULL, 0xDEADC0DE);
		dblock = 1 - dblock;
		if(!FAILED(g_FlushSurfaces[dblock]->LockRect(&lr, NULL, D3DLOCK_READONLY)))
		{
			dummy = *(int*)lr.pBits;
			g_FlushSurfaces[dblock]->UnlockRect();
		}
		return true;
	}
	else return true;
}

// Ponownie wype³nia zmienne zwi¹zane z uk³adem wspó³rzêdnych 3D wykorzysstuj¹c zmienn¹ Settings
// Wywo³ywaæ po zmianie Settings.
void ResetCoordinateSystem()
{
	switch (g_MouseCoords)
	{
	case MC_PIXELS:
		g_AspectRatio = static_cast<float>(Settings.BackBufferWidth) / static_cast<float>(Settings.BackBufferHeight);
		g_PixelsPerUnitX = g_PixelsPerUnitY = 1.0f;
		g_UnitsPerPixelX = g_UnitsPerPixelY = 1.0f;
		D3DXMatrixOrthoOffCenterLH(&g_Projection2D,
			0.0f,
			static_cast<float>(Settings.BackBufferWidth),
			static_cast<float>(Settings.BackBufferHeight),
			0.0f,
			Z_NEAR, Z_FAR);
		break;
	case MC_FIXED_HEIGHT:
		g_AspectRatio = static_cast<float>(Settings.BackBufferWidth) / static_cast<float>(Settings.BackBufferHeight);
		g_PixelsPerUnitY = static_cast<float>(Settings.BackBufferHeight) / g_MouseCoordsHeight;
		g_PixelsPerUnitX = g_PixelsPerUnitY;
		g_UnitsPerPixelY = (g_PixelsPerUnitY == 0.0f ? 0.0f : 1.0f / g_PixelsPerUnitY);
		g_UnitsPerPixelX = (g_PixelsPerUnitX == 0.0f ? 0.0f : 1.0f / g_PixelsPerUnitX);
		D3DXMatrixOrthoOffCenterLH(&g_Projection2D,
			0.0f,
			g_MouseCoordsHeight * g_AspectRatio,
			g_MouseCoordsHeight,
			0.0f,
			Z_NEAR, Z_FAR);
		break;
	case MC_FIXED:
		g_AspectRatio = g_MouseCoordsWidth / g_MouseCoordsHeight;
		g_PixelsPerUnitX = Settings.BackBufferWidth / g_MouseCoordsWidth;
		g_PixelsPerUnitY = Settings.BackBufferHeight / g_MouseCoordsHeight;
		g_UnitsPerPixelX = (g_PixelsPerUnitX == 0.0f ? 0.0f : 1.0f / g_PixelsPerUnitX);
		g_UnitsPerPixelY = (g_PixelsPerUnitY == 0.0f ? 0.0f : 1.0f / g_PixelsPerUnitY);
		D3DXMatrixOrthoOffCenterLH(&g_Projection2D,
			0.0f,
			g_MouseCoordsWidth,
			g_MouseCoordsHeight,
			0.0f,
			Z_NEAR, Z_FAR);
		break;
	default:
		assert(0 && "ResetCoordinateSystem: B³êdna wartoœæ g_MouseCoords");
	}
}

void OnDeviceRestore()
{
	// domyœlny stan
	CreateDefaultState();
	// Flush event
	CreateFlushObjects();
	// uk³ad wspó³rzêdnych 2D
	ResetCoordinateSystem();
}

void MinimizedEnable()
{
	g_Minimized = true;
	g_FrameObjectList.CallMinimize();
}

void MinimizedDisable()
{
	if (g_Minimized)
	{
		g_FrameObjectList.CallRestore();
		g_Minimized = false;
	}
}

void Pause()
{
	if (g_PauseCounter == 0)
		g_FrameObjectList.CallPause();
	g_PauseCounter++;
}

void Resume()
{
	g_PauseCounter--;
	if (g_PauseCounter == 0)
		g_FrameObjectList.CallResume();
}

void ActiveOn(bool wmActivate)
{
	if (!wmActivate || !g_LastWmActivate)
	{
		if (g_PauseCounter == 0)
		{
			InputActivate();
			g_FrameObjectList.CallActivate();
		}
		g_ActiveCounter++;
	}
	g_LastWmActivate = wmActivate;
}

void ActiveOff()
{
	g_LastWmActivate = false;
	if (g_PauseCounter == 0)
	{
		g_FrameObjectList.CallDeactivate();
		InputDeactivate();
	}
	g_ActiveCounter--;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje jakieœtam...

// Zwraca styl, jaki powinno mieæ okno na podstawie Settings i g_Params
DWORD WindowGetStyle()
{
	if (Settings.FullScreen)
		return WS_POPUP | WS_SYSMENU | WS_VISIBLE;
	else
	{
		if (g_AllowResize)
		{
			if (g_MaxWidth == 0 && g_MaxHeight == 0)
				return WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | 
					WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
			else
				return WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | 
					WS_MINIMIZEBOX | WS_VISIBLE;
		}
		else
			return WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
				WS_VISIBLE;
	}
}

void Create_D3D()
{
	D3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (D3D == 0)
		throw Error("Nie mo¿na zainicjalizowaæ Direct3D 9 (wymaga co najmniej DirectX 9.0c)", __FILE__, __LINE__);
}

// forward
LRESULT CALLBACK WndProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam);

void Create_WndClass()
{
	UINT Style = CS_DBLCLKS;
	if (!g_CloseButton)
		Style |= CS_NOCLOSE;

	WNDCLASS WndClass = {
		Style, WndProc, 0, 0, Instance, g_Icon,
		g_Cursor, 0, 0, g_ClassName.c_str()
	};
	if (RegisterClass(&WndClass) == 0)
		throw Win32Error("Nie mo¿na zarejestrowaæ klasy okna", __FILE__, __LINE__);
}

void Destroy_WndClass()
{
	UnregisterClass(g_ClassName.c_str(), Instance);
}

void Create_Window()
{
	// Styl
	DWORD WndStyle = WindowGetStyle();
	// Wymiary
	RECT rc = { 0, 0, Settings.BackBufferWidth, Settings.BackBufferHeight };
	AdjustWindowRect(&rc, WndStyle, false);
	// Utworzenie
	Wnd = CreateWindow(
		g_ClassName.c_str(), g_WindowTitle.c_str(), WndStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top,
		0, 0, Instance, 0);
	if (Wnd == 0)
		throw Win32Error("Nie mo¿na utworzyæ okna", __FILE__, __LINE__);
}

void Destroy_Window()
{
	DestroyWindow(Wnd);
	Wnd = 0;
}

// Wype³nia podan¹ strukturê na podstawie globalnych zmiennych inicjalizacyjnych
// i Settings
void GeneratePresentParams(D3DPRESENT_PARAMETERS *pp)
{
	assert(Wnd != 0 && "GeneratePresentParams: Wnd == 0");

	pp->BackBufferWidth = Settings.BackBufferWidth;
	pp->BackBufferHeight = Settings.BackBufferHeight;
	pp->BackBufferFormat = Settings.BackBufferFormat;
	pp->BackBufferCount = Settings.BackBufferCount;
	pp->MultiSampleType = Settings.MultiSampleType;
	pp->MultiSampleQuality = Settings.MultiSampleQuality;
	pp->SwapEffect = Settings.SwapEffect;
	pp->hDeviceWindow = Wnd;
	pp->Windowed = Settings.FullScreen ? FALSE : TRUE;
	pp->EnableAutoDepthStencil = Settings.EnableAutoDepthStencil ? TRUE : FALSE;
	pp->AutoDepthStencilFormat = Settings.AutoDepthStencilFormat;
	pp->FullScreen_RefreshRateInHz = Settings.FullScreen ? Settings.RefreshRate : 0;
	pp->PresentationInterval = Settings.PresentationInverval;

	pp->Flags = 0;
	if (Settings.DiscardDepthStencil) pp->Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
	if (Settings.LockableBackBuffer) pp->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
}

void Create_Device()
{
	assert(Wnd != 0 && "Create_Device: Wnd == 0");
	assert(D3D != 0 && "Create_Device: D3D == 0");

	D3DPRESENT_PARAMETERS PresentParams;
	GeneratePresentParams(&PresentParams);

	HRESULT hr;

	// AUTO
	if (g_Settings_VertexProcessing == 0)
	{
		hr = D3D->CreateDevice(g_Settings_Adapter, g_Settings_DeviceType, Wnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &PresentParams, &Dev);
		if (FAILED(hr))
		{
			hr = D3D->CreateDevice(g_Settings_Adapter, g_Settings_DeviceType, Wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &PresentParams, &Dev);
			if (FAILED(hr))
			{
				Dev = 0;
				throw DirectXError(hr, "Nie mo¿na utworzyæ urz¹dzenia.", __FILE__, __LINE__);
			}
		}
	}
	// Okreœlone
	else
	{
		hr = D3D->CreateDevice(g_Settings_Adapter, g_Settings_DeviceType, Wnd, g_Settings_VertexProcessing, &PresentParams, &Dev);
		if (FAILED(hr))
			throw DirectXError(hr, "Nie mo¿na utworzyæ urz¹dzenia.", __FILE__, __LINE__);
	}
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Inne funkcje prywatne

// Resetuje urz¹dzenie (po utracie albo po zmianie rozmiaru)
// B³¹d: wywo³uje Exit() i rzuca dalej
void EnvironmentReset()
{
	if (Dev == 0)
		throw new Error("CreateDefaultState: Urz¹dzenie Direct3D nie istnieje.", __FILE__, __LINE__);

	HRESULT hr;

	Pause();

	// reset
	D3DPRESENT_PARAMETERS PresentParams;
	GeneratePresentParams(&PresentParams);
	hr = Dev->Reset(&PresentParams);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na zresetowaæ urz¹dzenia. Czy¿by niezwolnione zasoby D3D po utracie urz¹dzenia?", __FILE__, __LINE__);

	OnDeviceRestore();
	g_FrameObjectList.CallResetDevice();

	Resume();
}

// Jest ¿¹danie zmiany trybu, jest odpowiedni moment - zmieñ tryb graficzny.
// Jeœli siê nie uda zmieniæ na nowy, próbuje przywróciæ stary.
// Jeœli i to siê nie uda, wyrzuca wyj¹tek i Settings pozostawia stare.
// Ma zmieniæ g_SettingsChangeState.
// Tak, wiem, ta funkcja jest koszmarna. I tak jest lepiej ni¿ by³o
// w poprzednich wersjach frameworka :P
void ApplySettingsChange()
{
	// Nie trzeba zresetowaæ urz¹dzenia
	if (CompareSettings(g_NewSettings, Settings) == SC_NEED_NOTHING_SPECIAL)
	{
		SAFE_RELEASE(g_FlushEventQuery);
		SAFE_RELEASE(g_FlushSurfaces[1]);
		SAFE_RELEASE(g_FlushSurfaces[0]);
		Settings = g_NewSettings;
		CreateFlushObjects();
		g_SettingsChangeState = SCS_SUCCEEDED;
		return;
	}

	Pause();

	// Zapamiêtaj stare ustawienia
	SETTINGS OldSettings = Settings;
	HRESULT hr;
	DWORD Style;
	D3DPRESENT_PARAMETERS PresentParams;

	// Uniewa¿nij urz¹dzenie
	g_FrameObjectList.CallLostDevice();
	FrameObjectsDeviceLost();

	// Trzeba zresetowaæ urz¹dzenie
	if (CompareSettings(g_NewSettings, Settings) == SC_NEED_RESET)
	{
		// Próba zresetowania z nowymi ustawieniami
		try
		{
			// Nowe ustawienia
			Settings = g_NewSettings;
			// Styl okna
			Style = WindowGetStyle();
			SetWindowLong(Wnd, GWL_STYLE, Style);
			// HACK - nowe ustawienia raz jeszcze, bo jak siê okazuje, SetWindowLong powoduje ich nieporz¹dan¹ zmianê.
			Settings = g_NewSettings;
			// Zresetuj urz¹dzenie
			GeneratePresentParams(&PresentParams);
			hr = Dev->Reset(&PresentParams);
			if (FAILED(hr))
				throw DirectXError(hr, "Nie mo¿na zmieniæ ustawieñ wyœwietlania - nie mo¿na zresetowaæ urz¹dzenia.", __FILE__, __LINE__);
			
			g_SettingsChangeState = SCS_SUCCEEDED;

		}
		// Nie uda³o siê z nowymi - spróbuj ze starymi
		// Jak i tu siê nie uda, to poleci wyj¹tek
		catch (...)
		{
			// Nowe ustawienia
			Settings = OldSettings;
			// Styl okna
			Style = WindowGetStyle();
			SetWindowLong(Wnd, GWL_STYLE, Style);
			// HACK - nowe ustawienia raz jeszcze, bo jak siê okazuje, SetWindowLong powoduje ich nieporz¹dan¹ zmianê.
			Settings = OldSettings;
			// Zresetuj urz¹dzenie
			GeneratePresentParams(&PresentParams);
			hr = Dev->Reset(&PresentParams);
			if (FAILED(hr))
				throw DirectXError(hr, "Nie mo¿na zmieniæ ustawieñ wyœwietlania ani przywróciæ poprzednich - nie mo¿na zresetowaæ urz¹dzenia.", __FILE__, __LINE__);
			
			g_SettingsChangeState = SCS_FAILED;
		}
		// Rozmiar okna
		RECT rc = { 0, 0, Settings.BackBufferWidth, Settings.BackBufferHeight };
		AdjustWindowRect(&rc, Style, false);
		SetWindowPos(Wnd, HWND_NOTOPMOST, 0, 0, rc.right-rc.left, rc.bottom-rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
	}

	// Przywróæ urz¹dzenie
	OnDeviceRestore();
	g_FrameObjectList.CallResetDevice();

	Resume();
}

SETTINGS_CHANGE_STATE GetSettingsChangeState()
{
	return g_SettingsChangeState;
}

void ResetSettingsChangeState()
{
	g_SettingsChangeState = SCS_NONE;
}

// Wykonanie ramki
void Frame()
{
	assert(Wnd != 0 && "Frame: Wnd == 0");
	if (Dev == 0) return;

	HRESULT hr;
	bool JustLost = false;

	try
	{
		ERR_HANDLE_BEGIN;
		{
			// zmiana ustawieñ
			if (g_SettingsChangeState == SCS_PENDING)
				ApplySettingsChange();

			// odzyskanie urz¹dzenia
			if (g_DeviceLost)
			{
				hr = Dev->TestCooperativeLevel();
				if (hr == D3DERR_DEVICELOST)
				{
					// nic - nadal jest utracone
				}
				else if (hr == D3DERR_DEVICENOTRESET)
				{
					// zosta³o odzyskane
					EnvironmentReset();
					g_DeviceLost = false;
				}
				else if (FAILED(hr))
					throw DirectXError(hr, "B³¹d podczas testowania poziomu kooperatywnoœci urz¹dzenia", __FILE__, __LINE__);
			}
			// utrata urz¹dzenia
			// podobno warto sprawdzaæ te¿ na pocz¹tku przed ramk¹
			else
			{
				hr = Dev->TestCooperativeLevel();
				// Urz¹dzenie utracono
				if (hr == D3DERR_DEVICELOST)
					JustLost = true;
				// B³¹d
				else if (hr == D3DERR_DRIVERINTERNALERROR)
					throw DirectXError(hr, "TestCooperativeLevel", __FILE__, __LINE__);
			}

			// czas - przed
			float CurrentTime = g_Timer.GetTimeF();
			if (!g_Time_WasInited)
			{
				g_OldTime = CurrentTime;
				g_Time_WasInited = true;
			}
			float DeltaTime = minmax(0.f, CurrentTime - g_OldTime, 0.25f);

			Timer1.UpdateTime(DeltaTime);
			Timer2.UpdateTime(DeltaTime);

			// FPS
			g_FPS_Frames++;
			if (g_FPS_LastTime+1.0f < CurrentTime)
			{
				g_FPS = static_cast<float>(g_FPS_Frames) / (CurrentTime-g_FPS_LastTime);
				g_FPS_LastTime = CurrentTime;
				g_FPS_Frames = 0;
			}

			// Liczniki
			g_FrameNumber++;
			g_NewDrawCount = 0;
			g_NewPrimitiveCount = 0;

			// wykonanie ramki
			if (g_OnFrame)
				(*g_OnFrame)();

			// Obliczenia po
			g_OldTime = CurrentTime;
			g_LastDrawCount = g_NewDrawCount;
			g_LastPrimitiveCount = g_NewPrimitiveCount;

			// zaprezentowanie
			if (!g_DeviceLost)
			{
				if (UseFlushObjects() == false)
					JustLost = true;

				hr = Dev->Present(0, 0, 0, 0);
				if (hr == D3DERR_DEVICELOST)
					JustLost = true;
				else if (FAILED(hr))
					throw DirectXError(hr, "Nie mo¿na wyœwietliæ ekranu", __FILE__, __LINE__);

				if (JustLost)
				{
					g_DeviceLost = true;
					MinimizedEnable();
					// invalidate
					g_FrameObjectList.CallLostDevice();
					// zasoby frameworka
					FrameObjectsDeviceLost();
				}
			}
		}
		ERR_HANDLE_END;
	}
	catch(Error &e)
	{
		Exit();
		e.Push("B³ad poczas przetwarzania klatki", __FILE__, __LINE__);
		ProcessError(e);
	}
}

// Pêtla komunikatów
int Run()
{
	assert(Wnd != 0 && "Run: Wnd == 0");

	MSG msg;
	msg.message = WM_NULL;

	while (msg.message != WM_QUIT)
	{
		if (g_LoopMode)
		{
			if (PeekMessage(&msg, Wnd, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
				Frame();
		}
		else
		{
			if (GetMessage(&msg, Wnd, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	return static_cast<int>(msg.wParam);
}

void wmGetMinMaxInfo(WPARAM wParam, LPARAM lParam)
{
	DWORD Style = WindowGetStyle();
	RECT min_rc = { 0, 0, g_MinWidth, g_MinHeight };
	RECT max_rc = { 0, 0, g_MaxWidth, g_MaxHeight };
	AdjustWindowRect(&min_rc, Style, false);
	AdjustWindowRect(&max_rc, Style, false);

	MINMAXINFO *mmi = reinterpret_cast<MINMAXINFO*>(lParam);
	if (g_MinWidth > 0) mmi->ptMinTrackSize.x = min_rc.right-min_rc.left;
	if (g_MinHeight > 0) mmi->ptMinTrackSize.y = min_rc.bottom-min_rc.top;
	if (g_MaxWidth > 0) mmi->ptMaxTrackSize.x = max_rc.right-max_rc.left;
	if (g_MaxHeight > 0) mmi->ptMaxTrackSize.y = max_rc.bottom-max_rc.top;
}

// Zreserowanie urz¹dzenia i wprowadzenie nowej rozdzielczoœci jeœli rozmiar
// okna siê zmieni³
// B³¹d: sam obs³uguje
void HandlePossibleSizeChange()
{
	assert(Wnd != 0 && "HandlePossibleSizeChange: Wnd == 0");
	if (Settings.FullScreen) return;

	try
	{
		ERR_HANDLE_BEGIN;
		{
			// Jeœli rozdzielczoœæ siê zmieni³a
			RECT rc;
			GetClientRect(Wnd, &rc);
			int width = rc.right-rc.left;
			int height = rc.bottom-rc.top;

			// Na pocz¹tku aplikacji wywo³uje siê WM_SIZE, które zwraca tutaj zera
			// dziwne zjawisko, trzeba to zignorowaæ
			if (width != 0 && height != 0)
			{
				if (width != Settings.BackBufferWidth || height != Settings.BackBufferHeight)
				{
					Settings.BackBufferWidth = width;
					Settings.BackBufferHeight = height;

					if (Dev)
					{
						// invalidate
						g_FrameObjectList.CallLostDevice();
						// zasoby frameworka
						FrameObjectsDeviceLost();
						
						EnvironmentReset();

						Redraw();
					}
				}
			}
		}
		ERR_HANDLE_END;
	}
	catch(Error &e)
	{
		Exit();
		ProcessError(e);
	}
}

POINT lParamToPos(LPARAM lParam)
{
	POINT pt;
	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	return pt;
}

void OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	if (g_IgnoreNextMouseMove)
	{
		g_IgnoreNextMouseMove = false;
		return;
	}

	POINT Pos;
	g_IgnoreNextMouseMove = false;

	if (g_MouseCameraMode)
	{
		if (frame::GetActive())
		{
			Pos = lParamToPos(lParam);
			Pos.x -= g_MouseRealPos.x;
			Pos.y -= g_MouseRealPos.y;
			g_IgnoreNextMouseMove = true;
			MouseSetRealPos();
		}
		else
		{
			Pos.x = 0;
			Pos.y = 0;
		}
	}
	else
		Pos = lParamToPos(lParam);

	VEC2 Pos2 = PixelsToUnits(Pos);

	for (INPUT_OBJECT_LIST::iterator it = g_InputObjectList.begin(); it != g_InputObjectList.end(); ++it)
		if ((*it)->OnMouseMove(Pos2))
			break;
}

void OnMouseButton(WPARAM wParam, LPARAM lParam, MOUSE_BUTTON Button, MOUSE_ACTION Action)
{
	POINT Pos;

	if (g_MouseCameraMode && GetActive())
		Pos = g_MouseSavedPos;
	else
		Pos = lParamToPos(lParam);

	VEC2 Pos2 = PixelsToUnits(Pos);

	for (INPUT_OBJECT_LIST::iterator it = g_InputObjectList.begin(); it != g_InputObjectList.end(); ++it)
		if ((*it)->OnMouseButton(Pos2, Button, Action))
			break;
}

void OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
	assert(Wnd != 0 && "OnMouseWheel: Wnd == 0");

	POINT Pos;

	if (g_MouseCameraMode && GetActive())
		Pos = g_MouseSavedPos;
	else
	{
		Pos = lParamToPos(lParam);
		ScreenToClient(Wnd, &Pos);
	}

	VEC2 Pos2 = PixelsToUnits(Pos);

	short d = static_cast<short>(HIWORD(wParam));
	float Delta = d/120.0f*g_MouseWheelLines;

	for (INPUT_OBJECT_LIST::iterator it = g_InputObjectList.begin(); it != g_InputObjectList.end(); ++it)
		if ((*it)->OnMouseWheel(Pos2, Delta))
			break;
}

void InputDestroy()
{
}

LRESULT CALLBACK WndProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	try
	{
		ERR_HANDLE_BEGIN;
		{
			switch (Msg)
			{
			case WM_CREATE:
				Wnd = Wnd;
				return 0;

			case WM_ENTERSIZEMOVE:
				Pause();
				return 0;

			case WM_EXITSIZEMOVE:
				HandlePossibleSizeChange();
				Resume();
				return 0;

			case WM_SIZE:
				switch (wParam)
				{
				case SIZE_MINIMIZED:
					MinimizedEnable();
					g_Maximized = false;
					break;
				case SIZE_MAXIMIZED:
					MinimizedDisable();
					HandlePossibleSizeChange();
					g_Maximized = true;
					break;
				case SIZE_RESTORED:
					if (g_Maximized) {
						g_Maximized = false;
						HandlePossibleSizeChange();
					}
					else if (GetMinimized()) {
						HandlePossibleSizeChange();
						MinimizedDisable();
					}
					else {
						// czekamy na WM_EXITSIZEMOVE
					}
					break;
				}
				return 0;

			case WM_ENTERMENULOOP:
				Pause();
				return 0;

			case WM_EXITMENULOOP:
				Resume();
				return 0;

			case WM_GETMINMAXINFO:
				wmGetMinMaxInfo(wParam, lParam);
				return 0;

			case WM_NCHITTEST:
				// Prevent the user from selecting the menu in fullscreen mode
				if(Settings.FullScreen)
					return HTCLIENT;
				break;

			case WM_POWERBROADCAST:
				if (wParam == PBT_APMQUERYSUSPEND)
					return BROADCAST_QUERY_DENY;
				break;

			case WM_SYSCOMMAND:
				// Prevent moving/sizing and power loss in fullscreen mode
				if (Settings.FullScreen) {
					switch(wParam)
					{
					case SC_MOVE:
					case SC_SIZE:
					case SC_MAXIMIZE:
					case SC_KEYMENU:
					case SC_MONITORPOWER:
						return 1;
					}
				}
				break;

			case WM_CLOSE:
				FrameObjectsDeviceLost();
				SAFE_RELEASE(Dev);
				g_FrameObjectList.CallDestroy();
				SAFE_RELEASE(D3D);
				InputDestroy();
				Destroy_Window();
				PostQuitMessage(0);
				break;

			case WM_MOUSEMOVE:
				OnMouseMove(wParam, lParam);
				return 0;

			case WM_LBUTTONDOWN:
				SetCapture(Wnd);
				OnMouseButton(wParam, lParam, MOUSE_LEFT, MOUSE_DOWN);
				break;
			case WM_MBUTTONDOWN:
				SetCapture(Wnd);
				OnMouseButton(wParam, lParam, MOUSE_MIDDLE, MOUSE_DOWN);
				break;
			case WM_RBUTTONDOWN:
				SetCapture(Wnd);
				OnMouseButton(wParam, lParam, MOUSE_RIGHT, MOUSE_DOWN);
				break;

			case WM_LBUTTONUP:
				OnMouseButton(wParam, lParam, MOUSE_LEFT, MOUSE_UP);
				ReleaseCapture();
				break;
			case WM_MBUTTONUP:
				OnMouseButton(wParam, lParam, MOUSE_MIDDLE, MOUSE_UP);
				ReleaseCapture();
				break;
			case WM_RBUTTONUP:
				OnMouseButton(wParam, lParam, MOUSE_RIGHT, MOUSE_UP);
				ReleaseCapture();
				break;

			case WM_LBUTTONDBLCLK:
				OnMouseButton(wParam, lParam, MOUSE_LEFT, MOUSE_DBLCLK);
				break;
			case WM_MBUTTONDBLCLK:
				OnMouseButton(wParam, lParam, MOUSE_MIDDLE, MOUSE_DBLCLK);
				break;
			case WM_RBUTTONDBLCLK:
				OnMouseButton(wParam, lParam, MOUSE_RIGHT, MOUSE_DBLCLK);
				break;

			case 0x020A: // WM_MOUSEWHEEL
				OnMouseWheel(wParam, lParam);
				break;

			case WM_KEYDOWN:
				for (INPUT_OBJECT_LIST::iterator it = g_InputObjectList.begin(); it != g_InputObjectList.end(); ++it)
					if ((*it)->OnKeyDown(wParam))
						break;
				break;
			case WM_KEYUP:
				for (INPUT_OBJECT_LIST::iterator it = g_InputObjectList.begin(); it != g_InputObjectList.end(); ++it)
					if ((*it)->OnKeyUp(wParam))
						break;
				break;
			case WM_CHAR:
				for (INPUT_OBJECT_LIST::iterator it = g_InputObjectList.begin(); it != g_InputObjectList.end(); ++it)
					if ((*it)->OnChar((char)wParam))
						break;
				break;

			case WM_ACTIVATE:
				// Reaguj¹ te¿ na Alt+Tab w trybie pe³noekranowym

				// Aplikacja deaktywowana
				if (LOBYTE(wParam) == WA_INACTIVE)
					ActiveOff();
				// Aplikacja aktywowana (czasem wywo³uje siê po 2 razy)
				else
					ActiveOn(true);
				break;

			case WM_PAINT:
				// Tu jest taki ma³y hak opieraj¹cy siê na tym, ¿e jak zachodzi WM_PAINT w czasie
				// kiedy jest Paused to pewnie znaczy ¿e rozszerzamy okno i wtedy nie nale¿y rysowaæ
				if (!g_LoopMode)
					Frame();
				break;

			case WM_TIMER:
				g_FrameObjectList.CallTimerFunc(wParam);
				break;
			}
		}
		ERR_HANDLE_END;
	}
	catch(Error &e)
	{
		Exit();
		e.Push(Format("B³¹d poczas przetwarzania komunikatu Win32API (uMsg = #, wParam = #, lParam = #)") % UintToStrR(Msg, 16) % UintToStrR(wParam, 16) % UintToStrR(lParam, 16), __FILE__, __LINE__);
		ProcessError(e);
	}

	return DefWindowProc(Wnd, Msg, wParam, lParam);
}

void InputCreate()
{
	// Pobierz z systemu ustawienia
	g_MouseSwapButtons = (GetSystemMetrics(SM_SWAPBUTTON) == TRUE);

	// Nie wiem czemu ta sta³a, zadeklarowana przecie¿ w windows.h, nie dzia³a³a!
	if (!SystemParametersInfo(104 /*SPI_GETWHEELSCROLLLINES*/, 0, &g_MouseWheelLines, 0))
		g_MouseWheelLines = 3;
}

void LoadConfig()
{
	ERR_TRY;

	// Wczytaj konfiguracjê z pliku
	LOG(1, Format("Framework: Wczytywanie konfiguracji z pliku: #") % CONFIG_FILENAME);
	g_Config.reset(new Config());
	g_Config->LoadFromFile(CONFIG_FILENAME);

	// Pobierz u¿ywan¹ przeze mnie konfiguracjê wyœwietlania
	string s;
	g_Config->MustGetDataEx("Display/BackBufferWidth", &Settings.BackBufferWidth);
	g_Config->MustGetDataEx("Display/BackBufferHeight", &Settings.BackBufferHeight);
	g_Config->MustGetDataEx("Display/RefreshRate", &Settings.RefreshRate);
	g_Config->MustGetData("Display/BackBufferFormat", &s); if (!StrToD3dfmt(&Settings.BackBufferFormat, s)) throw Error("B³¹d w BackBufferFormat");
	g_Config->MustGetDataEx("Display/BackBufferCount", &Settings.BackBufferCount);
	g_Config->MustGetData("Display/MultiSampleType", &s); if (!StrToD3dmultisampletype(&Settings.MultiSampleType, s)) throw Error("B³¹d w MultiSampleType");
	g_Config->MustGetDataEx("Display/MultiSampleQuality", &Settings.MultiSampleQuality);
	g_Config->MustGetData("Display/SwapEffect", &s); if (!StrToD3dswapeffect(&Settings.SwapEffect, s)) throw Error("B³¹d w SwapEffect");
	g_Config->MustGetDataEx("Display/FullScreen", &Settings.FullScreen);
	g_Config->MustGetDataEx("Display/EnableAutoDepthStencil", &Settings.EnableAutoDepthStencil);
	g_Config->MustGetData("Display/AutoDepthStencilFormat", &s); if (!StrToD3dfmt(&Settings.AutoDepthStencilFormat, s)) throw Error("B³¹d w AutoDepthStencilFormat");
	g_Config->MustGetDataEx("Display/DiscardDepthStencil", &Settings.DiscardDepthStencil);
	g_Config->MustGetDataEx("Display/LockableBackBuffer", &Settings.LockableBackBuffer);
	g_Config->MustGetData("Display/PresentationInverval", &s); if (!StrToD3dpresentationinterval(&Settings.PresentationInverval, s)) throw Error("B³¹d w PresentationInverval");

	g_Config->MustGetData("Display/FlushMode", &s);
	if (StrEqualI(s, "none")) Settings.FlushMode = SETTINGS::FLUSH_NONE;
	else if (StrEqualI(s, "event")) Settings.FlushMode = SETTINGS::FLUSH_EVENT;
	else if (StrEqualI(s, "lock")) Settings.FlushMode = SETTINGS::FLUSH_LOCK;
	else throw Error("B³êdna wartoœæ: Display/FlushMode.", __FILE__, __LINE__);

	g_Config->MustGetData("Display/Adapter", &s); if (!StrToD3dadapter(&g_Settings_Adapter, s)) throw Error("B³¹d w Adapter");
	g_Config->MustGetData("Display/DeviceType", &s); if (!StrToD3ddevtype(&g_Settings_DeviceType, s)) throw Error("B³¹d w DeviceType");
	g_Config->MustGetData("Display/VertexProcessing", &s); if (!StrToD3dvertexprocessing(&g_Settings_VertexProcessing, s)) throw Error("B³¹d w VertexProcessing");

	ERR_CATCH("Nie mo¿na wczytaæ konfiguracji g³ównej programu z pliku: " + CONFIG_FILENAME);
}

void WriteSettingsToConfig()
{
	ERR_TRY;

	string s;
	g_Config->MustSetDataEx("Display/BackBufferWidth", Settings.BackBufferWidth, 0);
	g_Config->MustSetDataEx("Display/BackBufferHeight", Settings.BackBufferHeight, 0);
	g_Config->MustSetDataEx("Display/RefreshRate", Settings.RefreshRate, 0);
	D3dfmtToStr(&s, Settings.BackBufferFormat); g_Config->MustSetDataEx("Display/BackBufferFormat", s, 0);
	g_Config->MustSetDataEx("Display/BackBufferCount", Settings.BackBufferCount, 0);
	D3dmultisampletypeToStr(&s, Settings.MultiSampleType); g_Config->MustSetDataEx("Display/MultiSampleType", s, 0);
	g_Config->MustSetDataEx("Display/MultiSampleQuality", Settings.MultiSampleQuality, 0);
	D3dswapeffectToStr(&s, Settings.SwapEffect); g_Config->MustSetDataEx("Display/SwapEffect", s, 0);
	g_Config->MustSetDataEx("Display/FullScreen", Settings.FullScreen, 0);
	g_Config->MustSetDataEx("Display/EnableAutoDepthStencil", Settings.EnableAutoDepthStencil, 0);
	D3dfmtToStr(&s, Settings.AutoDepthStencilFormat); g_Config->MustSetDataEx("Display/AutoDepthStencilFormat", s, 0);
	g_Config->MustSetDataEx("Display/DiscardDepthStencil", Settings.DiscardDepthStencil, 0);
	g_Config->MustSetDataEx("Display/LockableBackBuffer", Settings.LockableBackBuffer, 0);
	D3dpresentationinvervalToStr(&s, Settings.PresentationInverval); g_Config->MustSetDataEx("Display/PresentationInverval", s, 0);

	switch (Settings.FlushMode)
	{
	case SETTINGS::FLUSH_NONE:
		g_Config->MustSetData("Display/FlushMode", "None", 0);
		break;
	case SETTINGS::FLUSH_EVENT:
		g_Config->MustSetData("Display/FlushMode", "Event", 0);
		break;
	case SETTINGS::FLUSH_LOCK:
		g_Config->MustSetData("Display/FlushMode", "Lock", 0);
		break;
	default:
		assert(0 && "WTF");
	}

	D3dadapterToStr(&s, g_Settings_Adapter); g_Config->MustSetDataEx("Display/Adapter", s, 0);
	D3ddevtypeToStr(&s, g_Settings_DeviceType); g_Config->MustSetDataEx("Display/DeviceType", s, 0);
	D3dvertexprocessingToStr(&s, g_Settings_VertexProcessing); g_Config->MustSetDataEx("Display/VertexProcessing", s, 0);

	ERR_CATCH("Nie mo¿na zachowaæ bie¿¹cych ustawieñ wyœwietlania w konfiguracji.");
}

void SaveConfig()
{
	ERR_TRY;

	g_Config->SaveToFile(CONFIG_FILENAME);

	ERR_CATCH("Nie mo¿na zapisaæ konfiguracji g³ównej programu do pliku: " + CONFIG_FILENAME);
}

void UnloadConfig()
{
	g_Config.reset(0);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy publiczne

HINSTANCE Instance = 0;
HWND Wnd = 0;
LPDIRECT3D9 D3D = 0;
LPDIRECT3DDEVICE9 Dev = 0;
SETTINGS Settings;
float Time = 0.0f;
float DeltaTime = 0.0f;

void RegisterFrameObject(IFrameObject *fo)
{
	if (g_InsideGo)
		assert(0 && "RegisterFrameObject: Nie mo¿na zarejestrowaæ obiektu IFrameObject we frameworku wewn¹trz funkcji Go.");

	g_FrameObjectList.Add(fo);
}

int Go(
	HINSTANCE Instance,
	const string &ClassName,
	HICON Icon,
	HCURSOR Cursor,
	bool AllowResize,
	int MinWidth, int MinHeight, int MaxWidth, int MaxHeight,
	bool CloseButton,
	MOUSE_COORDS MouseCoords,
	float MouseCoordsWidth, float MouseCoordsHeight,
	FRAME_FUNC OnFrame)
{
	// Co by siê mierzenie czasu nie perniczy³o na maszynach wielordzeniowych
	SetThreadAffinityMask(GetCurrentThread(), 1);

	// Ustaw g_InsideGo na true, a na koniec na false
	struct InsideGoClass
	{
		InsideGoClass() { g_InsideGo = true; }
		~InsideGoClass() { g_InsideGo = false; }
	};
	InsideGoClass InsideGoObj;
	
	// Zapamiêtaj zmienne inicjalizacyjne
	Instance = Instance;
	g_ClassName = ClassName;
	g_Icon = Icon;
	g_Cursor = Cursor;
	g_AllowResize = AllowResize;
	g_MinWidth = MinWidth; g_MinHeight = MinHeight; g_MaxWidth = MaxWidth; g_MaxHeight = MaxHeight;
	g_CloseButton = CloseButton;
	g_MouseCoords = MouseCoords;
	g_MouseCoordsWidth = MouseCoordsWidth;
	g_MouseCoordsHeight = MouseCoordsHeight;
	g_OnFrame = OnFrame;

	int R = 0;

	try
	{
		ERR_HANDLE_BEGIN;
		{
			// Wczytaj konfiguracjê
			LoadConfig();

			ResetCoordinateSystem();

			// Jakaœtam inicjalizacja
			InitCommonControls();
			// Utworzenie urz¹dzenia Direct3D
			Create_D3D();
			// Zarejestrowanie klasy okna
			Create_WndClass();
			// Utworzenie okna
			Create_Window();
			// Zainicjuj wejœcie
			InputCreate();
			// Utworzenie urz¹dzenia
			Create_Device();
			// Callback inicjalizacji gry
			g_FrameObjectList.CallCreate();
			OnDeviceRestore();

			// URUCHOMIENIE GRY
			R = Run();

			UnloadConfig();
		}
		ERR_HANDLE_END;
	}
	catch(Error &e)
	{
		FrameObjectsDeviceLost();
		SAFE_RELEASE(Dev);
		g_FrameObjectList.CallDestroy();
		SAFE_RELEASE(D3D);
		InputDestroy();
		Destroy_Window();
		UnloadConfig();

		ProcessError(e);
		R = -1;
	}

	return R;
}

void ChangeSettings(const SETTINGS &NewSettings)
{
	// Jeœli nie ma ¿adnych zmian - koniec
	if (CompareSettings(NewSettings, Settings) == SC_EQUAL)
		return;

	if (g_LoopMode)
	{
		// Czekamy ze zmian¹
		if (Dev && !g_DeviceLost)
		{
			g_NewSettings = NewSettings;
			g_SettingsChangeState = SCS_PENDING;
		}
		// Od razu zmiana
		else
		{
			Settings = NewSettings;
			ResetCoordinateSystem();
		}
	}
	else
	{
		g_NewSettings = NewSettings;
		g_SettingsChangeState = SCS_PENDING;
		ApplySettingsChange();
		ResetCoordinateSystem();
		Redraw();
	}
}

void SetLoopMode(bool LoopMode)
{
	if (g_LoopMode == false && LoopMode == true)
		g_OldTime = g_Timer.GetTimeF();

	g_LoopMode = LoopMode;
}

void SetWindowTitle(const string &Title)
{
	g_WindowTitle = Title;
	if (Wnd)
		SetWindowText(Wnd, Title.c_str());
}

void ShowSystemCursor(bool Show)
{
	if (Show != g_ShowSystemCursor)
		::ShowCursor(Show ? TRUE : FALSE);

	g_ShowSystemCursor = Show;
}

float GetFPS()
{
	return g_FPS;
}

void Exit()
{
	if (Wnd)
		PostMessage(Wnd, WM_CLOSE, 0, 0);
	else
		throw Error("Nie mo¿na wy³¹czyæ aplikacji - okno nie istnieje.", __FILE__, __LINE__);
}

void Redraw()
{
	if (Wnd && g_LoopMode == false)
		InvalidateRect(Wnd, 0, FALSE);
}

void RestoreDefaultState()
{
	if (g_DefaultState)
	{
		HRESULT hr;
		hr = g_DefaultState->Apply();
		if (FAILED(hr))
			throw DirectXError(hr, "Nie mo¿na przywróciæ domyœlnego stanu urz¹dzenia", __FILE__, __LINE__);
	}
	else
		assert(0 && "Nie mo¿na przywróciæ domyœlnego stanu urz¹dzenia - brak zapamiêtanego stanu");
}

bool GetActive()
{
	return (g_ActiveCounter > 0);
}

bool GetMinimized()
{
	return g_Minimized;
}

bool GetPaused()
{
	return (g_PauseCounter > 0);
}

bool GetDeviceLost()
{
	return g_DeviceLost;
}

uint4 GetFrameNumber()
{
	return g_FrameNumber;
}

void GetDrawStats(uint4 *DrawCount, uint4 *PrimitiveCount)
{
	if (DrawCount)
		*DrawCount = g_LastDrawCount;
	if (PrimitiveCount)
		*PrimitiveCount = g_LastPrimitiveCount;
}

void RegisterDrawCall(uint4 PrimitiveCount)
{
	g_NewDrawCount++;
	g_NewPrimitiveCount += PrimitiveCount;
}

bool GetMaximized() { return g_Maximized; }

void GetDisplaySettings(int *Width, int *Height, int *RefreshRate)
{
	if (!D3D)
		throw Error("GetDisplaySettings: Nie mo¿na pobraæ ustawieñ pulpitu - obiekt Direct3D nie istnieje", __FILE__, __LINE__);

	D3DDISPLAYMODE dm;
	D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dm);

	*Width = dm.Width;
	*Height = dm.Height;
	*RefreshRate = dm.RefreshRate;
}

D3DFORMAT GetAdapterFormat() { return D3DFMT_X8R8G8B8; }
uint GetAdapter() { return g_Settings_Adapter; }
D3DDEVTYPE GetDeviceType() { return g_Settings_DeviceType; }
void GetMouseCoordsProjection(D3DXMATRIX *Matrix) { *Matrix = g_Projection2D; }

POINT UnitsToPixels(const VEC2 &Pos)
{
	POINT R;
	R.x = round(Pos.x * g_PixelsPerUnitX);
	R.y = round(Pos.y * g_PixelsPerUnitY);
	return R;
}

VEC2 PixelsToUnits(const POINT &Pos)
{
	VEC2 R;
	R.x = Pos.x * g_UnitsPerPixelX;
	R.y = Pos.y * g_UnitsPerPixelY;
	return R;
}

float GetAspectRatio() { return g_AspectRatio; }

float GetScreenWidth()
{
	switch (g_MouseCoords)
	{
	case MC_PIXELS:
		return static_cast<float>(Settings.BackBufferWidth);
	case MC_FIXED_HEIGHT:
		return g_MouseCoordsHeight * g_AspectRatio;
	case MC_FIXED:
		return g_MouseCoordsWidth;
	default:
		assert(0 && "GetScreenWidth: B³êdna wartoœæ g_MouseCoords");
		return 0.0f;
	}
}

float GetScreenHeight()
{
	switch (g_MouseCoords)
	{
	case MC_PIXELS:
		return static_cast<float>(Settings.BackBufferHeight);
	case MC_FIXED_HEIGHT:
	case MC_FIXED:
		return g_MouseCoordsHeight;
	default:
		assert(0 && "GetScreenHeight: B³êdna wartoœæ g_MouseCoords");
		return 0.0f;
	}
}

int UnitsToPixelsX(float UnitsX)
{
	return round(UnitsX * g_PixelsPerUnitX);
}

int UnitsToPixelsY(float UnitsY)
{
	return round(UnitsY * g_PixelsPerUnitY);
}

float PixelsToUnitsX(int PixelsX)
{
	return PixelsX * g_UnitsPerPixelX;
}

float PixelsToUnitsY(int PixelsY)
{
	return PixelsY * g_UnitsPerPixelY;
}

uint4 CreateTimer(uint4 Miliseconds, uint4 *RealMiliseconds)
{
	Miliseconds = minmax((uint4)USER_TIMER_MINIMUM, Miliseconds, (uint4)USER_TIMER_MAXIMUM);
	if (RealMiliseconds)
		*RealMiliseconds = Miliseconds;

	if (Wnd)
	{
		SetTimer(Wnd, g_NextTimerID, Miliseconds, 0);
		return g_NextTimerID;
	}
	else
		return 0;
}

void DestroyTimer(uint4 TimerID)
{
	if (Wnd)
		KillTimer(Wnd, TimerID);
}

int MouseButtonToKey(MOUSE_BUTTON button)
{
	if (g_MouseSwapButtons) {
		if (button == MOUSE_LEFT)
			button = MOUSE_RIGHT;
		else if (button == MOUSE_RIGHT)
			button = MOUSE_LEFT;
	}
	if (button == MOUSE_LEFT)
		return VK_LBUTTON;
	else if (button == MOUSE_RIGHT)
		return VK_RBUTTON;
	else // button == MOUSE_MIDDLE
		return VK_MBUTTON;
}

void RegisterInputObject(IInputObject *io)
{
	g_InputObjectList.push_back(io);
}

bool GetKeyboardKey(uint4 Key)
{
	if (GetActive())
		return (GetAsyncKeyState(Key) & 0x8000) != 0;
	else
		return false;
}

void GetMousePos(VEC2 *Pos)
{
	POINT pt;
	GetMousePos(&pt);
	*Pos = PixelsToUnits(pt);
}

void SetMousePos(const VEC2 &Pos)
{
	SetMousePos(UnitsToPixels(Pos));
}

void SetMouseMode(bool CameraMode)
{
	if (CameraMode == g_MouseCameraMode) return;

	// Za³¹cz tryb kamery
	if (CameraMode)
	{
		// Zapamiêtaj pozycjê kursora
		GetMousePos(&g_MouseSavedPos);
		// Ustaw pozycjê kursora na œrodek okna
		MouseSetRealPos();

		g_MouseCameraMode = true;
	}
	// Wy³¹cz tryb kamery
	else
	{
		g_MouseCameraMode = false;

		// Przywróæ pozyzjê kursora
		SetMousePos(g_MouseSavedPos);
	}
}

bool GetMouseMode()
{
	return g_MouseCameraMode;
}

bool GetMouseButton(MOUSE_BUTTON Button)
{
	if (GetActive())
	{
		int key = MouseButtonToKey(Button);
		return (GetAsyncKeyState(key) & 0x8000) != 0;
	}
	else return false;
}


} // namespace frame
