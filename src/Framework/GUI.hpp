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
	// Ma zosta� przypisane z u�yciem SetParent tylko raz, zaraz po utworzeniu,
	// podczas dodawania tego obiektu do kontrolki nadrz�dnej.
	// 0 oznacza, �e to kontrolka najwy�szego poziomu.
	CompositeControl *m_Parent;
	// Widzialno�� tej konkretnie kontrolki
	bool m_Visible;
	// Aktywno�� tej konkretnie kontrolki
	bool m_Enabled;
	// Czy ta kontrolka mo�e mie� ognisko
	bool m_Focusable;
	// Widzialno�� odziedziczona (true tylko je�li rodzic ma true i ta te� ma true)
	bool m_RealVisible;
	// Aktywno�� odziedziczona (true tylko je�li rodzic ma true i ta te� ma true)
	bool m_RealEnabled;
	// Prostok�t tej kontrolki w uk�adzie obszaru klienta kontrolki nadrz�dnej
	RECTF m_Rect;
	// Kursor tej kontrolki
	Cursor *m_Cursor;
	// Hint tej kontrolki
	Hint *m_Hint;
	// Prostok�t tej kontrolki w jej w�asnym uk�adzie (czyli 0,0,Width,Height)
	RECTF m_LocalRect;

	// == DLA MODU�U GUI ==

	// Odrysowanie siebie i ewentualnie podkontrolek w CompositeControl (dlatego wirtualna)
	// Wywo�uje OnDraw.
	virtual void Draw(const VEC2 &Translation);
	// Uaktualnia sw�j stan RealEnabled i RealVisible na podstawie swojego Enabled, Visible
	// i stanu kontrolki parenta
	virtual void UpdateRealEnabledVisible();
	// Znajduje i zwraca pierwsz� kontrolk� mog�c� mie� ogniska po�r�d kontrolek tej i
	// ewentualnie jej podrz�dnych w CompositeControl (dlatego wirtualna)
	// Je�li nie znaleziono, zwraca 0.
	virtual Control * FindFirstFocusableControl();
	// Znajduje i zwraca ostatni� kontrolk� mog�c� mie� ogniska po�r�d kontrolek tej i
	// ewentualnie jej podrz�dnych w CompositeControl (dlatego wirtualna)
	// Je�li nie znaleziono, zwraca 0.
	virtual Control * FindLastFocusableControl();

protected:
	virtual ~Control();

	// == DLA KLAS POTOMNYCH ==

	// Kontrolka ma si� odrysowa�
	// Nie kombinowa� w tej funkcji z tworzeniem, usuwaniem czy innym grzebaniem w strukturze kontrolek,
	// bo trwa ich przechodzenie i mo�e si� wszystko posypa�!
	virtual void OnDraw(const VEC2 &Translation) { }
	// Testowanie, czy pozycja kursora myszy le�y wewn�trz tego okna
	// - Zawsze zwraca true (domy�lnie) - okno obejmuje ca�y sw�j prostok�t.
	// - Zawsze zwraca false - okno jest przezroczyste dla myszy.
	// - Czasem zwraca true - pozwala testowa� nieregularne kszta�ty.
	//   (Wtedy nale�y tak�e rysowa� taki sam kszta�t.)
	// Pos jest we wsp�rz�dnych wzgl�dnych tego okna.
	virtual bool OnHitTest(const VEC2 &Pos) { return true; }
	// Zwraca preferowany rozmiar kontrolki lub 0.0f je�li nie ma preferencji co do danego wymiaru
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
	virtual bool OnKeyDown(uint4 Key); // Wywo�ywa� t� wersj� z klas odziedziczonych!
	virtual bool OnKeyUp(uint4 Key); // Wywo�ywa� t� wersj� z klas odziedziczonych!
	virtual bool OnChar(char Ch); // Wywo�ywa� t� wersj� z klas odziedziczonych!

public:
	// Do dowolnego u�ytku dla u�ytkownika
	void *Tag;

	Control(CompositeControl *Parent, bool Focusable, const RECTF &Rect);

	// == DLA U�YTKOWNIKA ==

	// Usuwa kontrolk�, zwalnia pami��
	// W�a�nie tej funkcji nale�y u�ywa�, nie destruktora.
	void Release();

	// Zwraca kontrolk� nadrz�dn�
	CompositeControl * GetParent() { return m_Parent; }
	// Zwraca kontrolk� g��wnego poziomu, do kt�rego nale�y ta kontrolka (lub j� sam� je�li ona jest g��wnego poziomu)
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
	// Wywo�ywa�, kiedy co� zmieni�o si� wewn�trz obiektu kursora tej kontrolki
	void CursorChanged();
	void SetHint(Hint *h);
	// Wywo�ywa�, kiedy co� zmieni�o si� wewn�trz obiektu hinta tej kontrolki
	void HintChanged();
	// Przenosi okno na wierzch w kolejno�ci
	// Dzia�a tylko dla okien g��wnego poziomu.
	void BringToFront();
	// Przenosi okno na sp�d w kolejno�ci
	// Dzia�a tylko dla okien g��wnego poziomu.
	void SendToBack();

	const RECTF & GetRect() { return m_Rect; }
	const RECTF & GetLocalRect() { return m_LocalRect; }
	void SetRect(const RECTF &Rect);
	// Zwraca pozycj� lewego g�rnego rogu tej kontrolki we wsp�rz�dnych ekranu
	void GetAbsolutePos(VEC2 *Out);
	void ClientToScreen(VEC2 *Out, const VEC2 &v);
	void ClientToScreen(RECTF *Out, const RECTF &r);
	void ScreenToClient(VEC2 *Out, const VEC2 &v);
	void ScreenToClient(RECTF *Out, const RECTF &r);
	// Testuje trafienie dla tej kontrolki
	// Pos we wsp. lokalnych tej kontrolki.
	// Zwraca t� kontrolk� lub 0 je�li nie ma trafienia.
	// Do rozbudowania w klasie CompositeControl - nie u�ywa� w klasach pochodnych, tylko nadpisywa� OnHitTest.
	virtual Control * HitTest(const VEC2 &Pos, VEC2 *LocalPos);
	// Ustawia warstw�, na kt�rej ma si� znajdowa� ta kontrolka
	// Dzia�a tylko dla kontrolek najwy�szego poziomu.
	void SetLayer(LAYER Layer);
	// Czy mysz jest nad t� kontrolk�
	bool IsMouseOver();
	// Czy ta kontrolka ma ognisko
	bool IsFocused();
	// Daje ognisko tej kontrolce
	void Focus();
	// Daje ognisko kontrolce nast�pnej po tej
	void FocusNext();
	// Daje ognisko kontrolce poprzedniej przed t�
	void FocusPrev();
	// Rozpoczyna przeci�ganie
	// Zadzia�a tylko wewn�trz obs�ugi OnMouseButton tej kontrolki z Action = frame::MOUSE_DOWN
	void StartDrag(DRAG_DATA_SHARED_PTR a_DragData = DRAG_DATA_SHARED_PTR());
	// Anluje przeci�ganie
	// Zadzia�a tylko je�li trwa przeci�ganie od tej kontrolki.
	void CancelDrag();
};

typedef std::vector<Control*> CONTROL_VECTOR;

typedef fastdelegate::FastDelegate1<Control*> STANDARD_EVENT;

// Klasa bazowa dla wszelkich kontrolek przechowuj�cych w swoim wn�trzu inne kontrolki
class CompositeControl : public Control
{
	friend class Control;
	friend class GuiManager;
	friend class GuiManager_pimpl;

private:
	CONTROL_VECTOR m_SubControls;

	// To jest taki haczyk. Zabezpiecza sytuacj�, kiedy destruktor tej klasy usuwa swoje podkontrolki,
	// a te pr�buj� w swoich destruktorach nakaza� mu usun�� si� ze swojej listy,
	// cho� jeste�my przecie� w trakcie jej przechodzenia.
	bool m_InDestructor;

	// == DLA MODU�U GUI ==
	
	// Uaktualnia ten stan dla siebie i swoich pod-kontrolek rekurencyjnie
	virtual void UpdateRealEnabledVisible();
	virtual void Draw(const VEC2 &Translation);
	void AddSubControl(Control *SubControl);
	void DeleteSubControl(Control *SubControl);
	virtual Control * FindFirstFocusableControl();
	virtual Control * FindLastFocusableControl();
	// C jest jego kontrolk� podrz�dn�
	Control * FindNextFocusableControl(Control *c);
	// C jest jego kontrolk� podrz�dn�
	Control * FindPrevFocusableControl(Control *c);

protected:
	virtual ~CompositeControl();

public:
	CompositeControl(CompositeControl *Parent, bool Focusable, const RECTF &Rect);

	// == DLA KLAS POTOMNYCH ==

	// Ma wersj� domy�ln�, zwracaj�c� ca�y obszar
	virtual void GetClientRect(RECTF *Out);

	// == DLA U�YTKOWNIKA ==

	uint4 GetSubControlCount() { return m_SubControls.size(); }
	Control * GetSubControl(uint4 Index) { if (Index < m_SubControls.size()) return m_SubControls[Index]; else return 0; }
	uint4 FindSubControl(Control *SubControl);
	// Przenosi wybran� kontrolk� na pocz�tek listy (po prostu zmiana kolejno�ci)
	void MoveSubControlToBegin(uint4 Index);
	void MoveSubControlToBegin(Control *SubControl);
	// Przenosi wybran� kontrolk� na koniec listy (po prostu zmiana kolejno�ci)
	void MoveSubControlToEnd(uint4 Index);
	void MoveSubControlToEnd(Control *SubControl);
	// Zamienia miejscami wybrane kontrolki
	void ExchangeSubControls(uint4 Index1, uint4 Index2);
	void ExchangeSubControls(Control *SubControl1, Control *SubControl2);

	// Zwraca true, je�li ta kontrolka jest parentem podanej, tak�e po�rednio
	bool IsParent(Control *c);
	virtual Control * HitTest(const VEC2 &Pos, VEC2 *LocalPos);
};

class GuiManager_pimpl;

// Klasa g��wna ca�ego modu�u GUI - singleton
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

	// Wywo�ywa� co klatk� w celu policzenia
	// Generuje wi�kszo�� zdarze� dla kontrolek.
	void OnFrame();
	// Wywo�ywa� co klatk� w celu odrysowania.
	// Wywo�uje OnDraw kontrolek.
	void OnDraw();

	// == DLA U�YTKOWNIKA ==

	uint4 GetControlCount(LAYER Layer);
	Control * GetControl(LAYER Layer, uint4 Index);
	uint4 FindControl(LAYER Layer, Control *Control);
	// Szuka podanej kontrolki na wszystkich warstwach
	// Zwraca warstw� i indeks na tej warstwie.
	// Jako Layer i Index mo�na podawa� 0 je�li nas nie interesuje.
	void FindControl(Control *c, LAYER *Layer, uint4 *Index);
	// Zwraca true, je�li podana kontrolka istnieje
	bool ControlExists(Control *c);
	// Je�li zwr�ci != 0 i LocalPos jest podane != 0, to przez LocalPos zwraca wsp�rz�dn� Pos wyra�on� w lokalnym uk�adzie wsp. zwr�conej kontrolki.
	Control * HitTest(const VEC2 &Pos, VEC2 *LocalPos);
	const VEC2 & GetMousePos();
	// Zwraca kontrolk�, nad kt�r� naprawd� jest aktualnie kursor myszy niezale�nie od przeci�gania
	Control * GetMouseOverControl();
	// Zwraca true, je�li trwa przeci�ganie
	bool IsDrag();
	// Zwraca kontrolk� �r�d�ow� przeci�gania
	Control * GetDragControl();
	// Zwraca przycisk myszy, kt�ry spowodowa� rozpocz�cie przeci�gania
	frame::MOUSE_BUTTON GetDragButton();
	// Zwraca dane przeci�gania
	DRAG_DATA_SHARED_PTR GetDragData();
	// Zwraca kontrolk�, kt�ra aktualnie ma ognisko lub 0 je�li �adna
	Control * GetFocusedControl();
	// Ustawia ognisko na �adn� kontrolk�
	void ResetFocus();
	Cursor * GetCurrentCursor();
	Hint * GetCurrentHint();
	void HideVolatileControls();

	// == ZDARZENIA ==
	
	// Nieobs�u�one przez �adn� kontrolk� zdarzenia z klawiatury
	EVENT_KEY_DOWN_UP OnUnhandledKeyDown;
	EVENT_KEY_DOWN_UP OnUnhandledKeyUp;
	EVENT_KEY_CHAR OnUnhandledChar;
	EVENT_CURSOR_CHANGE OnCursorChange;
	EVENT_HINT_CHANGE OnHintChange;
};

extern GuiManager *g_GuiManager;

} // namespace gui

#endif
