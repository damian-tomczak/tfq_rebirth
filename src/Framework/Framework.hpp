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

// Stan ¿¹dania zmiany trybu graficznego
enum SETTINGS_CHANGE_STATE
{
	SCS_NONE,      // Brak ¿¹dania
	SCS_PENDING,   // ¯¹danie oczekuje
	SCS_SUCCEEDED, // Uda³o siê
	SCS_FAILED     // Nie uda³o siê
};

// Rodzaj uk³adu wspó³rzêdnych
enum MOUSE_COORDS
{
	// Szerokoœæ i wysokoœæ pobierana z okna. Width i Height nie maj¹ znaczenia.
	// Jest to uk³ad dok³adnie w pikselach. Proporcje zachowane.
	MC_PIXELS,
	// Wysokoœæ podana w Height, szerokoœæ proporcjonalnie do niej. Width nie ma
	// znaczenia. Proporcje zachowane.
	MC_FIXED_HEIGHT,
	// Wysokoœæ podana w Height, szerokoœæ w Width. Jest to uk³ad z góry
	// okreœlony, proporcja nie musi byæ zachowana.
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

// Ustawienia wyœwietlania
struct SETTINGS
{
	// Szerokoœæ rozdzielczoœci
	uint4 BackBufferWidth;
	// Wysokoœæ rozdzielczoœci
	uint4 BackBufferHeight;
	// Czêstotliwoœæ odœwie¿ania
	// W trybie okienkowym nie ma znaczenia.
	uint4 RefreshRate;
	// Format bufora ramki
	// W trybie okienkowym mo¿na podaæ D3DFMT_UNKNOWN, aby by³ taki sam jak ekranu.
	// Zalecane: D3DFMT_A8R8G8B8.
	D3DFORMAT BackBufferFormat;
	// Liczba buforów w swap chain
	// Zalecane: 2.
	uint4 BackBufferCount;
	// Multisampling
	// Zalecane: D3DMULTISAMPLE_NONE.
	D3DMULTISAMPLE_TYPE MultiSampleType;
	// Jakoœæ multisamplingu
	// Zalecane: 0
	uint4 MultiSampleQuality;
	// Sposób flipowania
	// Zalecane: D3DSWAPEFFECT_DISCARD
	D3DSWAPEFFECT SwapEffect;
	// Pe³ny ekran
	bool FullScreen;
	// Czy u¿ywaæ bufora depth-stencil
	bool EnableAutoDepthStencil;
	// Format bufora depth-stencil
	// Jeœli EnableAutoDepthStencil == false, nie ma znaczenia.
	D3DFORMAT AutoDepthStencilFormat;
	// Dla wydajnoœci, ustawiæ na true o ile depth-stencil istnieje i nie jest
	// w formacie blokowalnym.
	bool DiscardDepthStencil;
	// Czy bufor ramki ma byæ blokowalny (dla wydajnoœci lepiej nie).
	bool LockableBackBuffer;
	// Sposób flipowania #2
	// Zazwyczaj:
	// - VSync w³¹czone: D3DPRESENT_INTERVAL_DEFAULT
	// - VSync wy³¹czone: D3DPRESENT_INTERVAL_IMMEDIATE
	uint4 PresentationInverval;
	// Czy u¿ywaæ specjalnego eventa czekaj¹cego przed zaprezentowaniem sceny na opróŸnienie kolejki karty graficznej?
	// To normalnie nie powinno byæ potrzebne, ale:
	// - pomaga kiedy coœ miga (bo sterownik ma b³¹d ¿e nie opró¿nia kolejki)
	// - pomaga kiedy skacze i siê przycina
	enum FLUSH_MODE
	{
		FLUSH_NONE, // Brak czekania na koniec klatki
		FLUSH_EVENT, // Czekanie za pomoc¹ Query typu EVENT
		FLUSH_LOCK, // Czekanie brutalne - za pomoc¹ blokowania powierzchni - ostatecznoœæ
	};
	FLUSH_MODE FlushMode;

	// Tworzy strukturê niezainicjalizowan¹
	SETTINGS() { }
	// Tworzy strukturê zainicjalizowan¹ wartoœciami domyœlnymi
	// DepthStencilFormat = D3DFMT_UNKNOWN oznacza brak bufora depth-stencil.
	SETTINGS(uint4 BackBufferWidth, uint4 BackBufferHeight, uint4 RefreshRate,
		bool FullScreen, D3DFORMAT DepthStencilFormat, bool VSync);
};

// Abstrakcyjna klasa bazowa (interfejs) dla obiektów które maj¹ byæ
// powiadamiane o zdarzeniach urz¹dzenia. Obiekty takie nale¿y pododawaæ funkcj¹
// RegisterFrameObject() przed wywo³aniem Go().
class IFrameObject
{
public:
	virtual ~IFrameObject() { }

	// Wywo³ywana raz, podczas inicjalizacji aplikacji, po utworzeniu obiektu D3D i urz¹dzenia D3D.
	virtual void OnCreate() { }
	// Wywo³ywana raz, podczas zwalniania, przed usuniêciem urz¹dzenia D3D i obiektu D3D.
	// Jeœli inicjalizacja OnCreate() siê nie powiedzie (rzuci wyj¹tek), jest wywo³ywana.
	virtual void OnDestroy() { }
	// Urz¹dzenie zosta³o utracone
	virtual void OnLostDevice() { }
	// Urz¹dzenie zosta³o odzyskane i zresetowane
	// To zdarzenie odpowiada te¿ mo¿liwej zmianie rozdzielczoœci i innych ustawieñ - tu je wykrywaæ.
	virtual void OnResetDevice() { }

	// Wywo³ywana podczas minimalizacji i przywracania okna programu
	virtual void OnRestore() { }
	virtual void OnMinimize() { }
	// Wywo³ywana podczas utraty i odzyskania aktywnoœci przez okno programu
	virtual void OnActivate() { }
	virtual void OnDeactivate() { }
	// Wywo³ywana podczas wstrzymywania pracy programu na nieokreœlony czas
	virtual void OnResume() { }
	virtual void OnPause() { }

	virtual void OnTimer(uint4 TimerId) { }
};

// Jak wy¿ej, ale dla obiektów maj¹cych byæ powiadamiane o komunikatach z wejœcia.
class IInputObject
{
public:
	// Ka¿da z tych funkcji jeœli zwróci true to znaczy ¿e komunikat przetworzy³a
	// i dalej nie bêdzie przekazywany do nastêpnego obiektu w ci¹gu.

	// Klawisz to sta³a VK_ z Windows API
	virtual bool OnKeyDown(uint4 Key) { return false; }
	virtual bool OnKeyUp(uint4 Key) { return false; }
	virtual bool OnChar(char Ch) { return false; }

	// Pozycja we wspó³rzêdnych lokalnych okna wed³ug ustawionego uk³adu wspó³rzêdnych.
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

// S¹ tylko do odczytu! S¹ zmiennymi a nie funkcjami ¿eby by³ wygodniejszy,
// krótszy i przede wszystkim szybszy dostêp do nich.


// Uchwyt instancji procesu (0 jeœli aktualnie nie istnieje)
extern HINSTANCE Instance;
// Okno g³ówne aplikacji (0 jeœli aktualnie nie istnieje)
extern HWND Wnd;
// Obiekt Direct3D (0 jeœli aktualnie nie istnieje)
// Czas ¿ycia: od Create() do Destroy() w³¹cznie.
extern LPDIRECT3D9 D3D;
// Obiekt urz¹dzenia Direct3D (0 jeœli aktualnie nie istnieje)
// Czas ¿ycia: od DeviceCreate() do DeviceDestroy() w³¹cznie.
extern LPDIRECT3DDEVICE9 Dev;
// Bie¿¹ce ustawienia
// Czas ¿ycia: od wywo³ania funkcji Go() do koñca
// Mog¹ siê zmieniaæ same po przeskalowaniu okna.
// Now¹ rozdzielczoœæ po przeskalowaniu mo¿na odczytaæ w OnDeviceRestore().
// Mo¿na je zapamiêtaæ i zapisaæ do pliku, np. w funkcji Destroy().
extern SETTINGS Settings;

extern Timer Timer1; // Do GUI i ogólnie timer bazowy aplikacji - raczej nie pausowaæ
extern Timer Timer2; // Do symulacji


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje

// Rejestruje obiekt dziedzicz¹cy z IFrameObject (dodaje, mo¿e byæ ich wiele)
// - Wywo³ywaæ przed wywo³aniem Go().
void RegisterFrameObject(IFrameObject *fo);

// Funkcja g³ówna - wykonuje ca³y framework
// Jeœli b³¹d, to framework sam go loguje i pokazuje. Zwraca to, co ma zwróciæ
// aplikacja.
// MinWidth, MinHeight, MaxWidth, MaxHeight - 0 oznacza brak danego limitu.
// Uwaga! Jeœli s¹ ograniczenia na rozmiar okna, w przypadku rêcznej zmiany
// rozdzielczoœci rozmiar okna mo¿e zostaæ ograniczony i wtedy obraz jest
// rozci¹gniêty!
int Go(
	// Uchwyt instancji procesu
	HINSTANCE Instance,
	// Nazwa klasy okna Win32API
	const string &ClassName,
	// Ikona dla okna
	HICON Icon,
	// Kursor dla okna
	HCURSOR Cursor,
	// Czy okno mo¿na rozszerzaæ
	bool AllowResize,
	// Ograniczenia wielkoœci obszaru roboczego okna
	int MinWidth, int MinHeight, int MaxWidth, int MaxHeight,
	// Czy dostêpny ma byæ przycisk zamkniêcia okna
	bool CloseButton,
	// Rodzaj uk³adu wspó³rzêdnych myszy
	MOUSE_COORDS MouseCoords,
	// Parametry tego uk³adu wspó³rzêdnych
	float MouseCoordsWidth, float MouseCoordsHeight,
	// Callback wykonywany co klatkê (mo¿e byæ 0)
	FRAME_FUNC OnFrame);

// Zmienia tryb graficzny
// ¯¹da zmiany. Zmiany zostan¹ wprowadzone dopiero przed nastêpn¹ klatk¹.
// ¯¹danie zostanie zapisane w wejdzie w ¿ycie dopiero w przysz³oœci.
// Stan ¿¹dania sprawdzaj funkcj¹ GetSettingsChangeState.
void ChangeSettings(const SETTINGS &NewSettings);
// Zwraca bie¿¹cy stan ¿¹dania zmiany trybu graficznego
SETTINGS_CHANGE_STATE GetSettingsChangeState();
// Resetuje bie¿¹cy stan ¿¹dania zmiany trybu graficznego na SCS_NONE
void ResetSettingsChangeState();
// Zapisuje bie¿¹ce ustawienia wyœwietlania z Settings do konfiguracji g_Config
void WriteSettingsToConfig();
// Zapisuje bie¿¹c¹ konfiguracjê z g_Config do pliku konfiguracji
void SaveConfig();
// Przywraca domyœlny stan urz¹dzenia
void RestoreDefaultState();
// Zwraca informacje ekranu
// U¿ywaæ tylko jeœli obiekt D3D istnieje!
// Mo¿na wykorzystaæ do ograniczenia maksymalnej rozdzielczoœci okna.
void GetDisplaySettings(int *Width, int *Height, int *RefreshRate);
// Zwraca u¿ywany globalnie w ca³ym programie Adapter Format
D3DFORMAT GetAdapterFormat();
// Zwraca ustawienia urz¹dzenia
uint GetAdapter();
D3DDEVTYPE GetDeviceType();


// ==== Funkcje, które mo¿na wywo³aæ zawsze ====

// Zmienia tryb pêtlenia siê (patrz dokumentacja)
void SetLoopMode(bool LoopMode);
// Zmienia tytu³ okna
void SetWindowTitle(const string &Title);
// Zmienia tryb pokazywania kursora systemowego
void ShowSystemCursor(bool Show);
// Zwraca aktualn¹ liczbê klatek na sekundê
// Aktualizowana co pewien czas, nie co klatkê.
float GetFPS();
// Zakoñczenie dzia³ania programu
void Exit();
// Przetwarza b³¹d, a konkretnie to loguje go do pliku
void ProcessError(const Error &e);
// ¯¹da odrysowania okna
// Dzia³a tylko kiedy LoopMode == false
void Redraw();
// Zwraca true, jeœli aplikacja jest aktywna
bool GetActive();
// Zwraca true, jeœli aplikacja jest zminimalizowana
bool GetMinimized();
// Zwraca true jeœli okno jest zmaksymalizowane
bool GetMaximized();
// Zwraca true, jeœli aplikacja jest wstrzymana
bool GetPaused();
// Zwraca true, jeœli urz¹dzenie D3D jest utracone
bool GetDeviceLost();
// Zwraca number bie¿¹cej lub ostatnio wykonanej klatki
uint4 GetFrameNumber();
// Zwraca liczbe rysowañ i narysowanych prymitywów w poprzedniej klatce
// Jako parametry mo¿na podawaæ 0 jeœli nas coœ akurat nie interesuje.
void GetDrawStats(uint4 *DrawCount, uint4 *PrimitiveCount);
// Trzeba wywo³ywaæ przy ka¿dym rysowaniu, ¿eby rejestrowaæ liczbê rysowañ i rysowanych prymitywów
void RegisterDrawCall(uint4 PrimitiveCount);


// ==== Timery ====

// Tworzy timer i zwraca jego ID
// Timery trzeba usuwaæ!
// RealMiliseconds [out] - zwraca timeout jaki naprawde uda³o siê ustawiæ.
// Mo¿na podaæ 0 jeœli nas nie interesuje.
// Na moim Widows XP dozwolony zakres to 10..2147483647.
uint4 CreateTimer(uint4 Miliseconds, uint4 *RealMiliseconds = 0);
// Usuwa timer o podanym ID
// Jeœli taki timer nie istnieje, nic siê nie dzieje.
void DestroyTimer(uint4 TimerID);


// ==== Wejœcie ====

// Zwraca macierz rzutowania odpowiadaj¹c¹ uk³adowi wspó³rzêdnych myszki
void GetMouseCoordsProjection(D3DXMATRIX *Matrix);
// Przelicza wspó³rzêdne z/na ten uk³ad
POINT UnitsToPixels(const VEC2 &Pos);
VEC2 PixelsToUnits(const POINT &Pos);
// Zwraca stosunek szerokoœci do wysokoœci w bie¿¹cym uk³adzie
float GetAspectRatio();
// Zwraca szerokoœæ ekranu w jednostkach
float GetScreenWidth();
// Zwraca wysokoœæ ekranu w jednostkach
float GetScreenHeight();
// Przelicza pozycjê lub szerokoœæ/wysokoœæ miêdzy pikselami i jednostkami
int UnitsToPixelsX(float UnitsX);
int UnitsToPixelsY(float UnitsY);
float PixelsToUnitsX(int PixelsX);
float PixelsToUnitsY(int PixelsY);

// Rejestruje obiekt dziedzicz¹cy z IInputObject
// Wywo³ywaæ przed wywo³aniem Go().
void RegisterInputObject(IInputObject *io);
// Zwraca true, jeœli aktualnie podany klawisz jest wciœniêty (sta³e VK_)
// Jeœli klawiatura nie jest pozyskana, wszystkie klawisze s¹ niewciœniête.
bool GetKeyboardKey(uint4 Key);
// Zmienia tryb pracy myszy
// Jeœli true, mysz jest w trybie kamery.
// Wówczas pozycja kursora nie ulega zmianom, a komunikaty o ruchu myszy zawieraj¹
// wzglêdne przesuniêcie. Mo¿na dziêki temu przesuwaæ mysz w nieskoñczonoœæ,
// co przydaje siê do ruchów kamery. Wyœwietlanie kursora na danej pozycji nie ma
// wówczas sensu.
void SetMouseMode(bool CameraMode);
// Zwraca true, jeœli mysz jest aktualnie w trybie kamery
bool GetMouseMode();
// Zwraca bie¿¹c¹ pozycjê kursora myszy
void GetMousePos(VEC2 *Pos);
// Ustawia pozycjê kursora myszy
void SetMousePos(const VEC2 &Pos);
// Zwraca aktualny stan wciœniêcia klawisza myszy
// Jeœli aplikacja jest nieaktywna, zawsze false.
bool GetMouseButton(MOUSE_BUTTON Button);

} // namespace frame

#endif
