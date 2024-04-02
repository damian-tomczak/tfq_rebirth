/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef GUI_H_
#define GUI_H_

#include "Framework.hpp"

namespace gui
{

enum LAYER
{
	LAYER_NORMAL = 0,
	LAYER_STAYONTOP = 1,
	LAYER_VOLATILE = 2,
	LAYER_COUNT = 3
};

class Hint
{
public:
	virtual ~Hint();
};

class PopupHint : public Hint
{
public:
	virtual ~PopupHint();

	virtual void OnDraw(const VEC2 &Pos) = 0;
};

class Cursor
{
public:
	virtual ~Cursor();
};

class DragData : public Cursor
{
public:
	virtual ~DragData();
};

typedef shared_ptr<DragData> DRAG_DATA_SHARED_PTR;

typedef fastdelegate::FastDelegate1<uint4> EVENT_KEY_DOWN_UP;
typedef fastdelegate::FastDelegate1<char> EVENT_KEY_CHAR;
typedef fastdelegate::FastDelegate1<Cursor*> EVENT_CURSOR_CHANGE;
typedef fastdelegate::FastDelegate1<Hint*> EVENT_HINT_CHANGE;

class CompositeControl;

// Abstrakcyjna klasa bazowa dla wszelkich kontrolek GUI
class Control
{
	friend class CompositeControl;
	friend class GuiManager;
	friend class GuiManager_pimpl;

private:
	// Ma zostaæ przypisane z u¿yciem SetParent tylko raz, zaraz po utworzeniu,
	// podczas dodawania tego obiektu do kontrolki nadrzêdnej.
	// 0 oznacza, ¿e to kontrolka najwy¿szego poziomu.
	CompositeControl *m_Parent;
	// Widzialnoœæ tej konkretnie kontrolki
	bool m_Visible;
	// Aktywnoœæ tej konkretnie kontrolki
	bool m_Enabled;
	// Czy ta kontrolka mo¿e mieæ ognisko
	bool m_Focusable;
	// Widzialnoœæ odziedziczona (true tylko jeœli rodzic ma true i ta te¿ ma true)
	bool m_RealVisible;
	// Aktywnoœæ odziedziczona (true tylko jeœli rodzic ma true i ta te¿ ma true)
	bool m_RealEnabled;
	// Prostok¹t tej kontrolki w uk³adzie obszaru klienta kontrolki nadrzêdnej
	RECTF m_Rect;
	// Kursor tej kontrolki
	Cursor *m_Cursor;
	// Hint tej kontrolki
	Hint *m_Hint;
	// Prostok¹t tej kontrolki w jej w³asnym uk³adzie (czyli 0,0,Width,Height)
	RECTF m_LocalRect;

	// == DLA MODU£U GUI ==

	// Odrysowanie siebie i ewentualnie podkontrolek w CompositeControl (dlatego wirtualna)
	// Wywo³uje OnDraw.
	virtual void Draw(const VEC2 &Translation);
	// Uaktualnia swój stan RealEnabled i RealVisible na podstawie swojego Enabled, Visible
	// i stanu kontrolki parenta
	virtual void UpdateRealEnabledVisible();
	// Znajduje i zwraca pierwsz¹ kontrolkê mog¹c¹ mieæ ogniska poœród kontrolek tej i
	// ewentualnie jej podrzêdnych w CompositeControl (dlatego wirtualna)
	// Jeœli nie znaleziono, zwraca 0.
	virtual Control * FindFirstFocusableControl();
	// Znajduje i zwraca ostatni¹ kontrolkê mog¹c¹ mieæ ogniska poœród kontrolek tej i
	// ewentualnie jej podrzêdnych w CompositeControl (dlatego wirtualna)
	// Jeœli nie znaleziono, zwraca 0.
	virtual Control * FindLastFocusableControl();

protected:
	virtual ~Control();

	// == DLA KLAS POTOMNYCH ==

	// Kontrolka ma siê odrysowaæ
	// Nie kombinowaæ w tej funkcji z tworzeniem, usuwaniem czy innym grzebaniem w strukturze kontrolek,
	// bo trwa ich przechodzenie i mo¿e siê wszystko posypaæ!
	virtual void OnDraw(const VEC2 &Translation) { }
	// Testowanie, czy pozycja kursora myszy le¿y wewn¹trz tego okna
	// - Zawsze zwraca true (domyœlnie) - okno obejmuje ca³y swój prostok¹t.
	// - Zawsze zwraca false - okno jest przezroczyste dla myszy.
	// - Czasem zwraca true - pozwala testowaæ nieregularne kszta³ty.
	//   (Wtedy nale¿y tak¿e rysowaæ taki sam kszta³t.)
	// Pos jest we wspó³rzêdnych wzglêdnych tego okna.
	virtual bool OnHitTest(const VEC2 &Pos) { return true; }
	// Zwraca preferowany rozmiar kontrolki lub 0.0f jeœli nie ma preferencji co do danego wymiaru
	virtual void GetDefaultSize(VEC2 *Out) { Out->x = 0.0f; Out->y = 0.0f; }

	virtual void OnEnable() { }
	virtual void OnDisable() { }
	virtual void OnShow() { }
	virtual void OnHide() { }
	virtual void OnFocusEnter() { }
	virtual void OnFocusLeave() { }
	virtual void OnMouseEnter() { }
	virtual void OnMouseLeave() { }
	virtual void OnRectChange() { }
	virtual void OnMouseMove(const VEC2 &Pos) { }
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action) { }
	virtual void OnMouseWheel(const VEC2 &Pos, float Delta) { }
	virtual void OnDragEnter(Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData) { }
	virtual void OnDragLeave(Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData) { }
	virtual void OnDragOver(const VEC2 &Pos, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData) { }
	virtual void OnDragDrop(const VEC2 &Pos, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData) { }
	virtual void OnDragCancel(const VEC2 &Pos, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData) { }
	virtual void OnDragFinished(bool Success, Control *DropControl, DRAG_DATA_SHARED_PTR a_DragData) { }
	virtual bool OnKeyDown(uint4 Key); // Wywo³ywaæ t¹ wersjê z klas odziedziczonych!
	virtual bool OnKeyUp(uint4 Key); // Wywo³ywaæ t¹ wersjê z klas odziedziczonych!
	virtual bool OnChar(char Ch); // Wywo³ywaæ t¹ wersjê z klas odziedziczonych!

public:
	// Do dowolnego u¿ytku dla u¿ytkownika
	void *Tag;

	Control(CompositeControl *Parent, bool Focusable, const RECTF &Rect);

	// == DLA U¯YTKOWNIKA ==

	// Usuwa kontrolkê, zwalnia pamiêæ
	// W³aœnie tej funkcji nale¿y u¿ywaæ, nie destruktora.
	void Release();

	// Zwraca kontrolkê nadrzêdn¹
	CompositeControl * GetParent() { return m_Parent; }
	// Zwraca kontrolkê g³ównego poziomu, do którego nale¿y ta kontrolka (lub j¹ samê jeœli ona jest g³ównego poziomu)
	Control * GetTopLevelParent();
	bool GetVisible() { return m_Visible; }
	bool GetEnabled() { return m_Enabled; }
	bool GetFocusable() { return m_Focusable; }
	bool GetRealVisible() { return m_RealVisible; }
	bool GetRealEnabled() { return m_RealEnabled; }
	Cursor * GetCursor() { return m_Cursor; }
	Hint * GetHint() { return m_Hint; }

	void SetVisible(bool Visible);
	void SetEnabled(bool Enabled);
	void SetFocusable(bool Focusable);
	void SetCursor(Cursor *c);
	// Wywo³ywaæ, kiedy coœ zmieni³o siê wewn¹trz obiektu kursora tej kontrolki
	void CursorChanged();
	void SetHint(Hint *h);
	// Wywo³ywaæ, kiedy coœ zmieni³o siê wewn¹trz obiektu hinta tej kontrolki
	void HintChanged();
	// Przenosi okno na wierzch w kolejnoœci
	// Dzia³a tylko dla okien g³ównego poziomu.
	void BringToFront();
	// Przenosi okno na spód w kolejnoœci
	// Dzia³a tylko dla okien g³ównego poziomu.
	void SendToBack();

	const RECTF & GetRect() { return m_Rect; }
	const RECTF & GetLocalRect() { return m_LocalRect; }
	void SetRect(const RECTF &Rect);
	// Zwraca pozycjê lewego górnego rogu tej kontrolki we wspó³rzêdnych ekranu
	void GetAbsolutePos(VEC2 *Out);
	void ClientToScreen(VEC2 *Out, const VEC2 &v);
	void ClientToScreen(RECTF *Out, const RECTF &r);
	void ScreenToClient(VEC2 *Out, const VEC2 &v);
	void ScreenToClient(RECTF *Out, const RECTF &r);
	// Testuje trafienie dla tej kontrolki
	// Pos we wsp. lokalnych tej kontrolki.
	// Zwraca t¹ kontrolkê lub 0 jeœli nie ma trafienia.
	// Do rozbudowania w klasie CompositeControl - nie u¿ywaæ w klasach pochodnych, tylko nadpisywaæ OnHitTest.
	virtual Control * HitTest(const VEC2 &Pos, VEC2 *LocalPos);
	// Ustawia warstwê, na której ma siê znajdowaæ ta kontrolka
	// Dzia³a tylko dla kontrolek najwy¿szego poziomu.
	void SetLayer(LAYER Layer);
	// Czy mysz jest nad t¹ kontrolk¹
	bool IsMouseOver();
	// Czy ta kontrolka ma ognisko
	bool IsFocused();
	// Daje ognisko tej kontrolce
	void Focus();
	// Daje ognisko kontrolce nastêpnej po tej
	void FocusNext();
	// Daje ognisko kontrolce poprzedniej przed t¹
	void FocusPrev();
	// Rozpoczyna przeci¹ganie
	// Zadzia³a tylko wewn¹trz obs³ugi OnMouseButton tej kontrolki z Action = frame::MOUSE_DOWN
	void StartDrag(DRAG_DATA_SHARED_PTR a_DragData = DRAG_DATA_SHARED_PTR());
	// Anluje przeci¹ganie
	// Zadzia³a tylko jeœli trwa przeci¹ganie od tej kontrolki.
	void CancelDrag();
};

typedef std::vector<Control*> CONTROL_VECTOR;

typedef fastdelegate::FastDelegate1<Control*> STANDARD_EVENT;

// Klasa bazowa dla wszelkich kontrolek przechowuj¹cych w swoim wnêtrzu inne kontrolki
class CompositeControl : public Control
{
	friend class Control;
	friend class GuiManager;
	friend class GuiManager_pimpl;

private:
	CONTROL_VECTOR m_SubControls;

	// To jest taki haczyk. Zabezpiecza sytuacjê, kiedy destruktor tej klasy usuwa swoje podkontrolki,
	// a te próbuj¹ w swoich destruktorach nakazaæ mu usun¹æ siê ze swojej listy,
	// choæ jesteœmy przecie¿ w trakcie jej przechodzenia.
	bool m_InDestructor;

	// == DLA MODU£U GUI ==
	
	// Uaktualnia ten stan dla siebie i swoich pod-kontrolek rekurencyjnie
	virtual void UpdateRealEnabledVisible();
	virtual void Draw(const VEC2 &Translation);
	void AddSubControl(Control *SubControl);
	void DeleteSubControl(Control *SubControl);
	virtual Control * FindFirstFocusableControl();
	virtual Control * FindLastFocusableControl();
	// C jest jego kontrolk¹ podrzêdn¹
	Control * FindNextFocusableControl(Control *c);
	// C jest jego kontrolk¹ podrzêdn¹
	Control * FindPrevFocusableControl(Control *c);

protected:
	virtual ~CompositeControl();

public:
	CompositeControl(CompositeControl *Parent, bool Focusable, const RECTF &Rect);

	// == DLA KLAS POTOMNYCH ==

	// Ma wersjê domyœln¹, zwracaj¹c¹ ca³y obszar
	virtual void GetClientRect(RECTF *Out);

	// == DLA U¯YTKOWNIKA ==

	uint4 GetSubControlCount() { return m_SubControls.size(); }
	Control * GetSubControl(uint4 Index) { if (Index < m_SubControls.size()) return m_SubControls[Index]; else return 0; }
	uint4 FindSubControl(Control *SubControl);
	// Przenosi wybran¹ kontrolkê na pocz¹tek listy (po prostu zmiana kolejnoœci)
	void MoveSubControlToBegin(uint4 Index);
	void MoveSubControlToBegin(Control *SubControl);
	// Przenosi wybran¹ kontrolkê na koniec listy (po prostu zmiana kolejnoœci)
	void MoveSubControlToEnd(uint4 Index);
	void MoveSubControlToEnd(Control *SubControl);
	// Zamienia miejscami wybrane kontrolki
	void ExchangeSubControls(uint4 Index1, uint4 Index2);
	void ExchangeSubControls(Control *SubControl1, Control *SubControl2);

	// Zwraca true, jeœli ta kontrolka jest parentem podanej, tak¿e poœrednio
	bool IsParent(Control *c);
	virtual Control * HitTest(const VEC2 &Pos, VEC2 *LocalPos);
};

class GuiManager_pimpl;

// Klasa g³ówna ca³ego modu³u GUI - singleton
class GuiManager : public frame::IInputObject
{
	friend class Control;
	friend class CompositeControl;

private:
	scoped_ptr<GuiManager_pimpl> pimpl;

	// Skasuj kontrolki zaznaczone do skasowania
	void DeleteControls();

public:
	GuiManager();
	~GuiManager();

	// == DLA FRAMEWORKA ==
	// Implementacja interfejsu frame::IInputObject

	virtual bool OnKeyDown(uint4 Key);
	virtual bool OnKeyUp(uint4 Key);
	virtual bool OnChar(char Ch);
	virtual bool OnMouseMove(const VEC2 &Pos);
	virtual bool OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	virtual bool OnMouseWheel(const VEC2 &Pos, float Delta);

	// == DLA APLIKACJI ==

	// Wywo³ywaæ co klatkê w celu policzenia
	// Generuje wiêkszoœæ zdarzeñ dla kontrolek.
	void OnFrame();
	// Wywo³ywaæ co klatkê w celu odrysowania.
	// Wywo³uje OnDraw kontrolek.
	void OnDraw();

	// == DLA U¯YTKOWNIKA ==

	uint4 GetControlCount(LAYER Layer);
	Control * GetControl(LAYER Layer, uint4 Index);
	uint4 FindControl(LAYER Layer, Control *Control);
	// Szuka podanej kontrolki na wszystkich warstwach
	// Zwraca warstwê i indeks na tej warstwie.
	// Jako Layer i Index mo¿na podawaæ 0 jeœli nas nie interesuje.
	void FindControl(Control *c, LAYER *Layer, uint4 *Index);
	// Zwraca true, jeœli podana kontrolka istnieje
	bool ControlExists(Control *c);
	// Jeœli zwróci != 0 i LocalPos jest podane != 0, to przez LocalPos zwraca wspó³rzêdn¹ Pos wyra¿on¹ w lokalnym uk³adzie wsp. zwróconej kontrolki.
	Control * HitTest(const VEC2 &Pos, VEC2 *LocalPos);
	const VEC2 & GetMousePos();
	// Zwraca kontrolkê, nad któr¹ naprawdê jest aktualnie kursor myszy niezale¿nie od przeci¹gania
	Control * GetMouseOverControl();
	// Zwraca true, jeœli trwa przeci¹ganie
	bool IsDrag();
	// Zwraca kontrolkê Ÿród³ow¹ przeci¹gania
	Control * GetDragControl();
	// Zwraca przycisk myszy, który spowodowa³ rozpoczêcie przeci¹gania
	frame::MOUSE_BUTTON GetDragButton();
	// Zwraca dane przeci¹gania
	DRAG_DATA_SHARED_PTR GetDragData();
	// Zwraca kontrolkê, która aktualnie ma ognisko lub 0 jeœli ¿adna
	Control * GetFocusedControl();
	// Ustawia ognisko na ¿adn¹ kontrolkê
	void ResetFocus();
	Cursor * GetCurrentCursor();
	Hint * GetCurrentHint();
	void HideVolatileControls();

	// == ZDARZENIA ==
	
	// Nieobs³u¿one przez ¿adn¹ kontrolkê zdarzenia z klawiatury
	EVENT_KEY_DOWN_UP OnUnhandledKeyDown;
	EVENT_KEY_DOWN_UP OnUnhandledKeyUp;
	EVENT_KEY_CHAR OnUnhandledChar;
	EVENT_CURSOR_CHANGE OnCursorChange;
	EVENT_HINT_CHANGE OnHintChange;
};

extern GuiManager *g_GuiManager;

} // namespace gui

#endif
