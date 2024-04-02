/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef FRAMEWORK_H_
#define FRAMEWORK_H_

#include <d3dx9.h>

namespace frame
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Typy

// Stan ��dania zmiany trybu graficznego
enum SETTINGS_CHANGE_STATE
{
	SCS_NONE,      // Brak ��dania
	SCS_PENDING,   // ��danie oczekuje
	SCS_SUCCEEDED, // Uda�o si�
	SCS_FAILED     // Nie uda�o si�
};

// Rodzaj uk�adu wsp�rz�dnych
enum MOUSE_COORDS
{
	// Szeroko�� i wysoko�� pobierana z okna. Width i Height nie maj� znaczenia.
	// Jest to uk�ad dok�adnie w pikselach. Proporcje zachowane.
	MC_PIXELS,
	// Wysoko�� podana w Height, szeroko�� proporcjonalnie do niej. Width nie ma
	// znaczenia. Proporcje zachowane.
	MC_FIXED_HEIGHT,
	// Wysoko�� podana w Height, szeroko�� w Width. Jest to uk�ad z g�ry
	// okre�lony, proporcja nie musi by� zachowana.
	MC_FIXED
};

// Klawisz myszy
enum MOUSE_BUTTON
{
	MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT
};

// Akcja klawisza myszy
enum MOUSE_ACTION
{
	MOUSE_DOWN, MOUSE_UP, MOUSE_DBLCLK
};

// Ustawienia wy�wietlania
struct SETTINGS
{
	// Szeroko�� rozdzielczo�ci
	uint4 BackBufferWidth;
	// Wysoko�� rozdzielczo�ci
	uint4 BackBufferHeight;
	// Cz�stotliwo�� od�wie�ania
	// W trybie okienkowym nie ma znaczenia.
	uint4 RefreshRate;
	// Format bufora ramki
	// W trybie okienkowym mo�na poda� D3DFMT_UNKNOWN, aby by� taki sam jak ekranu.
	// Zalecane: D3DFMT_A8R8G8B8.
	D3DFORMAT BackBufferFormat;
	// Liczba bufor�w w swap chain
	// Zalecane: 2.
	uint4 BackBufferCount;
	// Multisampling
	// Zalecane: D3DMULTISAMPLE_NONE.
	D3DMULTISAMPLE_TYPE MultiSampleType;
	// Jako�� multisamplingu
	// Zalecane: 0
	uint4 MultiSampleQuality;
	// Spos�b flipowania
	// Zalecane: D3DSWAPEFFECT_DISCARD
	D3DSWAPEFFECT SwapEffect;
	// Pe�ny ekran
	bool FullScreen;
	// Czy u�ywa� bufora depth-stencil
	bool EnableAutoDepthStencil;
	// Format bufora depth-stencil
	// Je�li EnableAutoDepthStencil == false, nie ma znaczenia.
	D3DFORMAT AutoDepthStencilFormat;
	// Dla wydajno�ci, ustawi� na true o ile depth-stencil istnieje i nie jest
	// w formacie blokowalnym.
	bool DiscardDepthStencil;
	// Czy bufor ramki ma by� blokowalny (dla wydajno�ci lepiej nie).
	bool LockableBackBuffer;
	// Spos�b flipowania #2
	// Zazwyczaj:
	// - VSync w��czone: D3DPRESENT_INTERVAL_DEFAULT
	// - VSync wy��czone: D3DPRESENT_INTERVAL_IMMEDIATE
	uint4 PresentationInverval;
	// Czy u�ywa� specjalnego eventa czekaj�cego przed zaprezentowaniem sceny na opr�nienie kolejki karty graficznej?
	// To normalnie nie powinno by� potrzebne, ale:
	// - pomaga kiedy co� miga (bo sterownik ma b��d �e nie opr�nia kolejki)
	// - pomaga kiedy skacze i si� przycina
	enum FLUSH_MODE
	{
		FLUSH_NONE, // Brak czekania na koniec klatki
		FLUSH_EVENT, // Czekanie za pomoc� Query typu EVENT
		FLUSH_LOCK, // Czekanie brutalne - za pomoc� blokowania powierzchni - ostateczno��
	};
	FLUSH_MODE FlushMode;

	// Tworzy struktur� niezainicjalizowan�
	SETTINGS() { }
	// Tworzy struktur� zainicjalizowan� warto�ciami domy�lnymi
	// DepthStencilFormat = D3DFMT_UNKNOWN oznacza brak bufora depth-stencil.
	SETTINGS(uint4 BackBufferWidth, uint4 BackBufferHeight, uint4 RefreshRate,
		bool FullScreen, D3DFORMAT DepthStencilFormat, bool VSync);
};

// Abstrakcyjna klasa bazowa (interfejs) dla obiekt�w kt�re maj� by�
// powiadamiane o zdarzeniach urz�dzenia. Obiekty takie nale�y pododawa� funkcj�
// RegisterFrameObject() przed wywo�aniem Go().
class IFrameObject
{
public:
	virtual ~IFrameObject() { }

	// Wywo�ywana raz, podczas inicjalizacji aplikacji, po utworzeniu obiektu D3D i urz�dzenia D3D.
	virtual void OnCreate() { }
	// Wywo�ywana raz, podczas zwalniania, przed usuni�ciem urz�dzenia D3D i obiektu D3D.
	// Je�li inicjalizacja OnCreate() si� nie powiedzie (rzuci wyj�tek), jest wywo�ywana.
	virtual void OnDestroy() { }
	// Urz�dzenie zosta�o utracone
	virtual void OnLostDevice() { }
	// Urz�dzenie zosta�o odzyskane i zresetowane
	// To zdarzenie odpowiada te� mo�liwej zmianie rozdzielczo�ci i innych ustawie� - tu je wykrywa�.
	virtual void OnResetDevice() { }

	// Wywo�ywana podczas minimalizacji i przywracania okna programu
	virtual void OnRestore() { }
	virtual void OnMinimize() { }
	// Wywo�ywana podczas utraty i odzyskania aktywno�ci przez okno programu
	virtual void OnActivate() { }
	virtual void OnDeactivate() { }
	// Wywo�ywana podczas wstrzymywania pracy programu na nieokre�lony czas
	virtual void OnResume() { }
	virtual void OnPause() { }

	virtual void OnTimer(uint4 TimerId) { }
};

// Jak wy�ej, ale dla obiekt�w maj�cych by� powiadamiane o komunikatach z wej�cia.
class IInputObject
{
public:
	// Ka�da z tych funkcji je�li zwr�ci true to znaczy �e komunikat przetworzy�a
	// i dalej nie b�dzie przekazywany do nast�pnego obiektu w ci�gu.

	// Klawisz to sta�a VK_ z Windows API
	virtual bool OnKeyDown(uint4 Key) { return false; }
	virtual bool OnKeyUp(uint4 Key) { return false; }
	virtual bool OnChar(char Ch) { return false; }

	// Pozycja we wsp�rz�dnych lokalnych okna wed�ug ustawionego uk�adu wsp�rz�dnych.
	virtual bool OnMouseMove(const VEC2 &Pos) { return false; }
	virtual bool OnMouseButton(const VEC2 &Pos, MOUSE_BUTTON Button, MOUSE_ACTION Action) { return false; }
	virtual bool OnMouseWheel(const VEC2 &Pos, float Delta) { return false; }
};

typedef void (*FRAME_FUNC)();

class Timer
{
	friend void Frame();

private:
	float m_Time;
	float m_DeltaTime;
	int m_PauseCounter;
	bool m_WasPaused;

	// Dla funkcji Frame.
	void UpdateTime(float DeltaTime);

public:
	Timer() : m_Time(0.f), m_DeltaTime(0.f), m_PauseCounter(0), m_WasPaused(false) { }

	float GetTime() { return m_Time; }
	float GetDeltaTime() { return m_DeltaTime; }
	bool IsPaused() { return (m_PauseCounter > 0); }

	void Pause() { m_PauseCounter++; }
	void Resume() { m_PauseCounter--; if (m_PauseCounter <= 0) m_WasPaused = true; }
	void TogglePause() { if (IsPaused()) Resume(); else Pause(); }
	void SetTime(float NewTime) { m_Time = NewTime; }
	void ChangeTime(float TimeDiff) { m_Time += TimeDiff; }
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Zmienne globalne

// S� tylko do odczytu! S� zmiennymi a nie funkcjami �eby by� wygodniejszy,
// kr�tszy i przede wszystkim szybszy dost�p do nich.


// Uchwyt instancji procesu (0 je�li aktualnie nie istnieje)
extern HINSTANCE Instance;
// Okno g��wne aplikacji (0 je�li aktualnie nie istnieje)
extern HWND Wnd;
// Obiekt Direct3D (0 je�li aktualnie nie istnieje)
// Czas �ycia: od Create() do Destroy() w��cznie.
extern LPDIRECT3D9 D3D;
// Obiekt urz�dzenia Direct3D (0 je�li aktualnie nie istnieje)
// Czas �ycia: od DeviceCreate() do DeviceDestroy() w��cznie.
extern LPDIRECT3DDEVICE9 Dev;
// Bie��ce ustawienia
// Czas �ycia: od wywo�ania funkcji Go() do ko�ca
// Mog� si� zmienia� same po przeskalowaniu okna.
// Now� rozdzielczo�� po przeskalowaniu mo�na odczyta� w OnDeviceRestore().
// Mo�na je zapami�ta� i zapisa� do pliku, np. w funkcji Destroy().
extern SETTINGS Settings;

extern Timer Timer1; // Do GUI i og�lnie timer bazowy aplikacji - raczej nie pausowa�
extern Timer Timer2; // Do symulacji


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje

// Rejestruje obiekt dziedzicz�cy z IFrameObject (dodaje, mo�e by� ich wiele)
// - Wywo�ywa� przed wywo�aniem Go().
void RegisterFrameObject(IFrameObject *fo);

// Funkcja g��wna - wykonuje ca�y framework
// Je�li b��d, to framework sam go loguje i pokazuje. Zwraca to, co ma zwr�ci�
// aplikacja.
// MinWidth, MinHeight, MaxWidth, MaxHeight - 0 oznacza brak danego limitu.
// Uwaga! Je�li s� ograniczenia na rozmiar okna, w przypadku r�cznej zmiany
// rozdzielczo�ci rozmiar okna mo�e zosta� ograniczony i wtedy obraz jest
// rozci�gni�ty!
int Go(
	// Uchwyt instancji procesu
	HINSTANCE Instance,
	// Nazwa klasy okna Win32API
	const string &ClassName,
	// Ikona dla okna
	HICON Icon,
	// Kursor dla okna
	HCURSOR Cursor,
	// Czy okno mo�na rozszerza�
	bool AllowResize,
	// Ograniczenia wielko�ci obszaru roboczego okna
	int MinWidth, int MinHeight, int MaxWidth, int MaxHeight,
	// Czy dost�pny ma by� przycisk zamkni�cia okna
	bool CloseButton,
	// Rodzaj uk�adu wsp�rz�dnych myszy
	MOUSE_COORDS MouseCoords,
	// Parametry tego uk�adu wsp�rz�dnych
	float MouseCoordsWidth, float MouseCoordsHeight,
	// Callback wykonywany co klatk� (mo�e by� 0)
	FRAME_FUNC OnFrame);

// Zmienia tryb graficzny
// ��da zmiany. Zmiany zostan� wprowadzone dopiero przed nast�pn� klatk�.
// ��danie zostanie zapisane w wejdzie w �ycie dopiero w przysz�o�ci.
// Stan ��dania sprawdzaj funkcj� GetSettingsChangeState.
void ChangeSettings(const SETTINGS &NewSettings);
// Zwraca bie��cy stan ��dania zmiany trybu graficznego
SETTINGS_CHANGE_STATE GetSettingsChangeState();
// Resetuje bie��cy stan ��dania zmiany trybu graficznego na SCS_NONE
void ResetSettingsChangeState();
// Zapisuje bie��ce ustawienia wy�wietlania z Settings do konfiguracji g_Config
void WriteSettingsToConfig();
// Zapisuje bie��c� konfiguracj� z g_Config do pliku konfiguracji
void SaveConfig();
// Przywraca domy�lny stan urz�dzenia
void RestoreDefaultState();
// Zwraca informacje ekranu
// U�ywa� tylko je�li obiekt D3D istnieje!
// Mo�na wykorzysta� do ograniczenia maksymalnej rozdzielczo�ci okna.
void GetDisplaySettings(int *Width, int *Height, int *RefreshRate);
// Zwraca u�ywany globalnie w ca�ym programie Adapter Format
D3DFORMAT GetAdapterFormat();
// Zwraca ustawienia urz�dzenia
uint GetAdapter();
D3DDEVTYPE GetDeviceType();


// ==== Funkcje, kt�re mo�na wywo�a� zawsze ====

// Zmienia tryb p�tlenia si� (patrz dokumentacja)
void SetLoopMode(bool LoopMode);
// Zmienia tytu� okna
void SetWindowTitle(const string &Title);
// Zmienia tryb pokazywania kursora systemowego
void ShowSystemCursor(bool Show);
// Zwraca aktualn� liczb� klatek na sekund�
// Aktualizowana co pewien czas, nie co klatk�.
float GetFPS();
// Zako�czenie dzia�ania programu
void Exit();
// Przetwarza b��d, a konkretnie to loguje go do pliku
void ProcessError(const Error &e);
// ��da odrysowania okna
// Dzia�a tylko kiedy LoopMode == false
void Redraw();
// Zwraca true, je�li aplikacja jest aktywna
bool GetActive();
// Zwraca true, je�li aplikacja jest zminimalizowana
bool GetMinimized();
// Zwraca true je�li okno jest zmaksymalizowane
bool GetMaximized();
// Zwraca true, je�li aplikacja jest wstrzymana
bool GetPaused();
// Zwraca true, je�li urz�dzenie D3D jest utracone
bool GetDeviceLost();
// Zwraca number bie��cej lub ostatnio wykonanej klatki
uint4 GetFrameNumber();
// Zwraca liczbe rysowa� i narysowanych prymityw�w w poprzedniej klatce
// Jako parametry mo�na podawa� 0 je�li nas co� akurat nie interesuje.
void GetDrawStats(uint4 *DrawCount, uint4 *PrimitiveCount);
// Trzeba wywo�ywa� przy ka�dym rysowaniu, �eby rejestrowa� liczb� rysowa� i rysowanych prymityw�w
void RegisterDrawCall(uint4 PrimitiveCount);


// ==== Timery ====

// Tworzy timer i zwraca jego ID
// Timery trzeba usuwa�!
// RealMiliseconds [out] - zwraca timeout jaki naprawde uda�o si� ustawi�.
// Mo�na poda� 0 je�li nas nie interesuje.
// Na moim Widows XP dozwolony zakres to 10..2147483647.
uint4 CreateTimer(uint4 Miliseconds, uint4 *RealMiliseconds = 0);
// Usuwa timer o podanym ID
// Je�li taki timer nie istnieje, nic si� nie dzieje.
void DestroyTimer(uint4 TimerID);


// ==== Wej�cie ====

// Zwraca macierz rzutowania odpowiadaj�c� uk�adowi wsp�rz�dnych myszki
void GetMouseCoordsProjection(D3DXMATRIX *Matrix);
// Przelicza wsp�rz�dne z/na ten uk�ad
POINT UnitsToPixels(const VEC2 &Pos);
VEC2 PixelsToUnits(const POINT &Pos);
// Zwraca stosunek szeroko�ci do wysoko�ci w bie��cym uk�adzie
float GetAspectRatio();
// Zwraca szeroko�� ekranu w jednostkach
float GetScreenWidth();
// Zwraca wysoko�� ekranu w jednostkach
float GetScreenHeight();
// Przelicza pozycj� lub szeroko��/wysoko�� mi�dzy pikselami i jednostkami
int UnitsToPixelsX(float UnitsX);
int UnitsToPixelsY(float UnitsY);
float PixelsToUnitsX(int PixelsX);
float PixelsToUnitsY(int PixelsY);

// Rejestruje obiekt dziedzicz�cy z IInputObject
// Wywo�ywa� przed wywo�aniem Go().
void RegisterInputObject(IInputObject *io);
// Zwraca true, je�li aktualnie podany klawisz jest wci�ni�ty (sta�e VK_)
// Je�li klawiatura nie jest pozyskana, wszystkie klawisze s� niewci�ni�te.
bool GetKeyboardKey(uint4 Key);
// Zmienia tryb pracy myszy
// Je�li true, mysz jest w trybie kamery.
// W�wczas pozycja kursora nie ulega zmianom, a komunikaty o ruchu myszy zawieraj�
// wzgl�dne przesuni�cie. Mo�na dzi�ki temu przesuwa� mysz w niesko�czono��,
// co przydaje si� do ruch�w kamery. Wy�wietlanie kursora na danej pozycji nie ma
// w�wczas sensu.
void SetMouseMode(bool CameraMode);
// Zwraca true, je�li mysz jest aktualnie w trybie kamery
bool GetMouseMode();
// Zwraca bie��c� pozycj� kursora myszy
void GetMousePos(VEC2 *Pos);
// Ustawia pozycj� kursora myszy
void SetMousePos(const VEC2 &Pos);
// Zwraca aktualny stan wci�ni�cia klawisza myszy
// Je�li aplikacja jest nieaktywna, zawsze false.
bool GetMouseButton(MOUSE_BUTTON Button);

} // namespace frame

#endif
