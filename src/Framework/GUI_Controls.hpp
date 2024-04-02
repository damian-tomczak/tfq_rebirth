/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef GUI_CONTROLS_1_H_
#define GUI_CONTROLS_1_H_

#include <deque>
#include "GUI.hpp"
#include "Res_d3d.hpp"
#include "Gfx2D.hpp"

namespace gui
{

const float CONTROL_HEIGHT = 24.f;
const float MARGIN = 4.f;

enum CURSOR_TYPE
{
	CT_NORMAL,
	CT_EDIT,
	CT_LINK,
	CT_SIZE_H,
	CT_SIZE_V,
	CT_SIZE_LT,
	CT_SIZE_RT,
	CT_MOVE,
	CT_COUNT
};

// Musi by� zainicjowana przed tworzeniem kontrolek z tego modu�u
extern DragData *g_StandardCursors[CT_COUNT];

class StringHint : public gui::Hint
{
private:
	string m_Text;

public:
	StringHint(const string &Text) : m_Text(Text) { }

	const string & GetText() { return m_Text; }
	void SetText(const string &Text) { m_Text = Text; }
};

class DragCursor : public DragData
{
private:
	CURSOR_TYPE m_CursorType;

public:
	DragCursor(CURSOR_TYPE CursorType) : m_CursorType(CursorType) { }

	CURSOR_TYPE GetCursorType() { return m_CursorType; }
};

extern res::Font *g_Font;
extern gfx2d::Sprite *g_Sprite;
extern const float g_FontSize;
extern const COLOR g_FontColor;
extern const COLOR g_HighlightFontColor;
extern const COLOR g_InfoFontColor;
extern const COLOR g_SelBkgColor;
extern const COLOR g_SelFontColor;
extern const COLOR g_SelInfoFontColor;
extern const COLOR g_HeaderFontColor;
extern const COLOR g_FocusColor;
extern const COLOR g_BkgColor;
extern const COLOR g_BkgColor2;
extern const COLOR g_HintBkgColor;
extern const COLOR g_HintFontColor;

const float SCROLLBAR_SIZE = 24.0f;

typedef fastdelegate::FastDelegate2<Control*, uint4> ID_EVENT;

// Reprezentuje statyczny napis
// TextFlags to sta�e z res::Font::FLAG_*.
class Label : public Control
{
private:
	string m_Text;
	uint4 m_TextFlags;

protected:
	// == Implementacja Control ==
	virtual void OnDraw(const VEC2 &Translation);

public:
	Label(CompositeControl *Parent, const RECTF &Rect, const string &Text, uint4 TextFlags);

	const string & GetText() { return m_Text; }
	uint4 GetTextFlags() { return m_TextFlags; }

	void SetText(const string &Text) { m_Text = Text; }
	void SetTextFlags(uint4 Flags) { m_TextFlags = Flags; }
};

// Reprezentuje klas� bazow� dla wszelkich przycisk�w
// Reaguje na wci�ni�cie i puszczenie spowodowane:
// - Klawiszami: Spacja, Enter
// - Myszk�: lewy klawisz
// Zadaniem klasy pochodnej konkretyzuj�cej j� jest:
// - Zdecydowa�, czy kontrolka ma by� Focusable
// - Napisa� OnDraw korzystaj�c z:
//   > GetRealEnabled()
//   > IsMouseOver()
//   > IsFocused()
//   > IsDown()
class AbstractButton : public Control
{
private:
	bool m_Down;

protected:
	// == Implementacja Control ==
	virtual void OnDisable() { m_Down = false; }
	virtual void OnHide() { m_Down = false; }
	virtual void OnFocusLeave() { m_Down = false; }
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	virtual bool OnKeyDown(uint4 Key);
	virtual bool OnKeyUp(uint4 Key);

	// == Dla klas potomnych ==
	// Mo�na nadpisa� je�li si� chec zrobi� co� dodatkowego, ale t� te� nale�y wywo�a�.
	virtual void DoClick();

public:
	STANDARD_EVENT OnClick;

	AbstractButton(CompositeControl *Parent, const RECTF &Rect, bool Focusable);

	// Zwraca true je�li przycisk jest wci�ni�ty
	bool IsDown() { return m_Down; }

	// Wymusza klikni�cie (wywo�uje OnClick je�li ustawione)
	void Click();
};

// Reprezentuje przycisk z tekstem
class Button : public AbstractButton
{
private:
	string m_Text;

protected:
	virtual void OnDraw(const VEC2 &Translation);

public:
	Button(CompositeControl *Parent, const RECTF &Rect, const string &Text, bool Focusable);

	const string & GetText() { return m_Text; }
	void SetText(const string &Text) { m_Text = Text; }
};

// Reprezentuje przycisk z obrazkiem
class SpriteButton : public AbstractButton
{
private:
	uint4 m_SpriteIndex;
	float m_SpriteWidth, m_SpriteHeight;

protected:
	virtual void OnDraw(const VEC2 &Translation);

public:
	SpriteButton(CompositeControl *Parent, const RECTF &Rect, bool Focusable);

	void SetSprite(uint4 SpriteIndex, float SpriteWidth, float SpriteHeight) { m_SpriteIndex = SpriteIndex; m_SpriteWidth = SpriteWidth; m_SpriteHeight = SpriteHeight; }
};

// Reprezentuje checkbox, kt�ry:
// 1. Jest klikalny jak przycisk
// 2. Jest w jednym z trzech stan�w
// Stan GRAYED mo�na tylko przepisa� r�cznie. Klikanie prze��cza go na CHECKED i
// dalej ju� na UNCHECKED, CHECKED, UNCHECKED itd...
// Je�li chce si� reagowa� na zmian� stanu, trzeba obs�u�y� AbstractButton::OnClick.
// To nie wywo�uje si� podczas zmiany r�cznej przez SetState.
// Zadaniem klasy potomnej jest rysowanie - OnDraw.
class AbstractCheckBox : public AbstractButton
{
public:
	enum STATE
	{
		UNCHECKED,
		CHECKED,
		GRAYED
	};

	static bool StateToBool(STATE State) { return (State == CHECKED); }
	static STATE BoolToState(bool B) { return (B ? CHECKED : UNCHECKED); }

private:
	STATE m_State;

protected:
	virtual void DoClick();

public:
	AbstractCheckBox(CompositeControl *Parent, const RECTF &Rect, STATE InitialState, bool Focusable = true);

	STATE GetState() { return m_State; }
	void SetState(STATE NewState) { m_State = NewState; }
};

// Reprezentuje checkbox z tekstem
class CheckBox : public AbstractCheckBox
{
private:
	string m_Text;

protected:
	virtual void OnDraw(const VEC2 &Translation);

public:
	CheckBox(CompositeControl *Parent, const RECTF &Rect, STATE InitialState, const string &Text, bool Focusable = true);

	const string & GetText() { return m_Text; }
	void SetText(const string &Text) { m_Text = Text; }
};

// Reprezentuje radio button, kt�ry:
// 1. Jest klikalny jak przycisk
// 2. Jest zaznaczony lub nie
// 3. Nale�y do pewnej grupy.
// Grup� podaje si� przy tworzeniu i nie mo�na ju� potem jej zmieni�.
// Tylko jeden radio button w danej grupie mo�e by� zaznaczony (albo �aden).
// Jako numer grupy mo�na podawa� w�asny (0, 1, 2, 1000, 1001, dowolny...) lub generowa� za pomoc� GenerateId.
// Je�li chce si� reagowa� na zmian� stanu, trzeba obs�u�y� AbstractButton::OnClick lub
// OnCheckChange. To drugie wykonuje si� przy ka�dej zmianie stanu, czyli powodowanej:
// - klikni�ciem
// - zaznaczeniem innego radiobuttona z grupy
// - wywo�aniem SetChecked
// Zadaniem klasy potomnej jest rysowanie - OnDraw.
class AbstractRadioButton : public AbstractButton
{
private:
	class GroupDatabase;

	uint4 m_Group;
	bool m_Checked;
	static GroupDatabase *sm_Groups;
	static int sm_GroupsRefCount;

protected:
	virtual void DoClick();

public:
	STANDARD_EVENT OnCheckChange;

	AbstractRadioButton(CompositeControl *Parent, const RECTF &Rect, uint4 Group, bool InitialChecked, bool Focusable = true);
	~AbstractRadioButton();

	bool IsChecked() { return m_Checked; }
	uint4 GetGroup() { return m_Group; }
	void SetChecked(bool NewChecked);
};

// Reprezentuje RadioButton z tekstem
class RadioButton : public AbstractRadioButton
{
private:
	string m_Text;

protected:
	virtual void OnDraw(const VEC2 &Translation);

public:
	RadioButton(CompositeControl *Parent, const RECTF &Rect, uint4 Group, bool InitialChecked, const string &Text, bool Focusable = true);

	const string & GetText() { return m_Text; }
	void SetText(const string &Text) { m_Text = Text; }
};

// Reprezentuje poziomy lub pionowy pasek, kt�ry mo�na przeci�ga� zmieniaj�c warto�� liczbow�.
// Pami�tana warto�� mo�e by� typu int (dyskretna) lub float (ci�g�a).
// Zadaniem klasy potomnej jest rysowanie.
class AbstractTrackBar : public Control
{
private:
	ORIENTATION m_Orientation;
	bool m_FloatValue;
	union
	{
		struct
		{
			int m_MinI;
			int m_MaxI;
			int m_StepI;
			int m_ValueI;
		};
		struct 
		{
			float m_MinF;
			float m_MaxF;
			float m_StepF;
			float m_ValueF;
		};
	};
	bool m_Down;
	float m_GrabOffset; // wzgl�dem �rodka

	void HandleDrag(const VEC2 &Pos);

protected:
	// == Implementacja Control ==
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	virtual void OnMouseMove(const VEC2 &Pos);
	virtual bool OnKeyDown(uint4 Key);

	// == Dla klas potomnych ==
	virtual float GetBarSize() = 0;

	// Zwraca pozycj� X lub Y pocz�tku trajektorii po jakiej porusza si� �rodek bara
	float GetBarTrackStart();
	// Zwraca d�ugo�� na X lub Y trajektorii, po jakiej porusza si� �rodek bara
	float GetBarTrackLength();
	// Zwraca pozycj� �rodka paska X lub Y (zale�nie od orientacji)
	// we wsp�rz�dnych lokalnych tej kontrolki - przydatne do rysowania
	float GetBarCenter();
	// Zwraca prostok�t, w jakim rysowany ma by� bar
	void GetBarRect(RECTF *Rect);

public:
	STANDARD_EVENT OnValueChange;

	AbstractTrackBar(CompositeControl *Parent, const RECTF &Rect, ORIENTATION Orientation, bool FloatValue, bool Focusable);

	bool IsFloatValue() { return m_FloatValue; }
	ORIENTATION GetOrientation() { return m_Orientation; }
	int GetMinI() { return m_MinI; }
	int GetMaxI() { return m_MaxI; }
	int GetStepI() { return m_StepI; }
	int GetValueI() { return m_ValueI; }
	float GetMinF() { return m_MinF; }
	float GetMaxF() { return m_MaxF; }
	float GetStepF() { return m_StepF; }
	float GetValueF() { return m_ValueF; }
	// Zwraca warto�� w procentach (0..1), niezale�nie od tego czy jest typu float czy int
	float GetPercentValue();

	void SetRangeI(int Min, int Max, int Step);
	void SetValueI(int Value);
	void SetRangeF(float Min, float Max, float Step);
	void SetValueF(float Value);

	int PercentToValueI(float Percent);
	float PercentToValueF(float Percent);
	float ValueToPercentI(int Value);
	float ValueToPercentF(float Value);
};

class TrackBar : public AbstractTrackBar
{
protected:
	// == Implementacja Control ==
	virtual void OnDraw(const VEC2 &Translation);

	// == Implementacja AbstractTrackBar ==
	virtual float GetBarSize();

public:
	TrackBar(CompositeControl *Parent, const RECTF &Rect, ORIENTATION Orientation, bool FloatValue, bool Focusable);
};

class AbstractScrollBar : public Control
{
protected:
	// Co aktualnie jest wci�ni�te
	enum STATE
	{
		STATE_NONE,
		STATE_BUTTON1,
		STATE_BUTTON2,
		STATE_SPACE,
		STATE_TRACKBUTTON,
	};

private:
	float m_Min;
	float m_Max;
	// D�ugo�� dokumentu wzgl�dem d�ugo�ci paska, np. 2.0f oznacza, �e
	// dokument jest 2 x d�u�szy, ni� pasek.
	float m_Doc;
	float m_Pos; // Min...Max
	float m_Step;
	ORIENTATION m_Orientation;

	STATE m_State;
	// Pozycja, za kt�r� u�ytkownik chwyci� trackbutton w jego wsp�rz�dnych
	VEC2 m_TrackPos;

	// == USTAWIA POZYCJ� SCROLLBARA NA PODSTAWIE POZYCJI KURSORA MYSZY ==
	// Pos we wsp�rz�dnych tego okna.

	// Podczas przeci�gania wolnego obszaru
	void Space_SetPos(const VEC2 &pos);
	// Podczas przeci�gania przycisku przewijania
	void TrackButton_SetPos(const VEC2 &pos);

protected:
	// == Implementacja Control ==
	virtual void OnMouseMove(const VEC2 &Pos);
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	virtual void OnMouseWheel(const VEC2 &Pos, float Delta);
	virtual bool OnKeyDown(uint4 Key);

	// == Dla klas pochodnych ==
	// Wysoko�� (je�li pionowy) lub szeroko�� (je�li poziomy) przycisk�w na pocz�tku i na ko�cu paska.
	// 0 oznacza brak przycisk�w.
	virtual float GetButtonSize() = 0;
	// Minimalna i maksymalna wysoko�� (je�li pionowy) lub szeroko�� (je�li poziomy) przycisku do przewijania.
	virtual float GetMinTrackButtonSize() = 0;
	virtual float GetMaxTrackButtonSize() = 0;

	STATE GetState() { return m_State; }
	// TE FUNKCJE ZWRACAJ� PROSTOK�TY WE WSPӣRZ�DNYCH LOKALNYCH TEGO OKNA
	// Zwraca prostok�t przycisku pierwszego (g�rny/lewy)
	void GetRect_Button1(RECTF *Out);
	// Zwraca prostok�t przycisku drugiego (dolny/prawy)
	void GetRect_Button2(RECTF *Out);
	// Zwraca prostok�t przycisku przewijania
	void GetRect_TrackButton(RECTF *Out);

public:
	STANDARD_EVENT OnScroll;

	AbstractScrollBar(CompositeControl *Parent, const RECTF &Rect, ORIENTATION Orientation, bool Focusable = false);

	float GetMin() { return m_Min; }
	float GetMax() { return m_Max; }
	float GetDoc() { return m_Doc; }
	float GetPos() { return m_Pos; }
	float GetPercentPos() { if (m_Min == m_Max) return 0.0f; return (m_Pos-m_Min)/(m_Max-m_Min); }
	float GetStep() { return m_Step; }
	ORIENTATION GetOrientation() { return m_Orientation; }

	void SetRange(float Min, float Max) { m_Min = Min; m_Max = Max; m_Pos = minmax(m_Min, m_Pos, m_Max); }
	void SetDoc(float Doc) { m_Doc = Doc; }
	// Pozycja w zakresie Min..Max (mo�na poda� poza zakresem, zostanie ograniczona)
	void SetPos(float Pos) { m_Pos = m_Pos = minmax(m_Min, Pos, m_Max); }
	// Pozycja w zakresie 0..1 (mo�na poda� poza zakresem, zostanie ograniczona)
	void SetPercentPos(float PercentPos) { SetPos(PercentPos*(m_Max-m_Min)+m_Min); }
	void SetStep(float Step) { m_Step = Step; }
};

class ScrollBar : public AbstractScrollBar
{
protected:
	// == Implementacja Control ==
	virtual void OnDraw(const VEC2 &Translation);

	// == Implementacja AbstractScrollBar ==
	virtual float GetButtonSize() { return (GetOrientation() == O_HORIZ ? GetLocalRect().bottom : GetLocalRect().right); }
	virtual float GetMinTrackButtonSize() { return 10.0f; }
	virtual float GetMaxTrackButtonSize() { return 1e7f; }

public:
	ScrollBar(CompositeControl *Parent, const RECTF &Rect, ORIENTATION Orientation, bool Focusable = false);
};

class GroupBox : public CompositeControl
{
private:
	float GetBorder() { return 2.0f; }

protected:
	// == Implementacja Control ==
	virtual void OnDraw(const VEC2 &Translation);

public:
	GroupBox(CompositeControl *Parent, const RECTF &Rect);

	// == Implementacja CompositeControl ==
	virtual void GetClientRect(RECTF *Out) { float Border = GetBorder(); Out->left = Out->top = Border; Out->right = GetRect().right-Border; Out->bottom = GetRect().bottom-Border; }	
};

// Podkontrolkami tej kontrolki mog� by� wy��cznie obiekty klasy TabControl::Tab.
// Aktywna zak�adka jest Visible, pozosta�e nie.
// Zak�adkami zarz�dza� przez metody tej klasy, a nie bezpo�rednio metodami CompositeControl.
class TabControl : public CompositeControl
{
public:
	class Tab : public CompositeControl
	{
		friend class TabControl;

	private:
		string m_Title;
		float m_TabWidth;

		void RecalculateTabWidth();

	public:
		Tab(TabControl *Parent, const RECTF &Rect, const string &Title);

		const string & GetTitle() { return m_Title; }
		float GetTabWidth() { return m_TabWidth; }
		void SetTitle(const string &NewTitle);
	};

private:
	// Indeks podkontrolki taba lub MAXUINT4 je�li �adna nie wybrana.
	uint4 m_TabIndex;
	// Nad kt�r� zak�adk� jest kursor myszy (je�li jaka� warto�� spoza zakresu to nad �adna)
	uint4 m_MouseOverTabIndex;

	// Zwraca wysoko�� paska z zak�adkami
	float GetTabHeight();
	// Zwraca prostok�t ograniczaj�cy kontrolki Tab (czyli tre�� zak�adek)
	void GetTabContentRect(RECTF *Out);

protected:
	// == Implementacja Control ==
	virtual void OnDraw(const VEC2 &Translation);
	virtual void OnRectChange();
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	virtual bool OnKeyDown(uint4 Key);
	virtual void OnMouseMove(const VEC2 &Pos);
	virtual void OnMouseLeave() { m_MouseOverTabIndex = MAXUINT4; }

public:
	STANDARD_EVENT OnTabChange;

	TabControl(CompositeControl *Parent, const RECTF &Rect, bool Focusable);

	// == Implementacja CompositeControl ==
	virtual void GetClientRect(RECTF *Out) { *Out = GetLocalRect(); }

	Tab * AddTab(const string &Title);
	bool RemoveTab(uint4 Index);
	bool RemoveTab(Tab *tab);
	bool RenameTab(uint4 Index, const string &NewTitle);
	uint4 GetTabCount() { return GetSubControlCount(); }
	Tab *GetTab(uint4 Index);
	uint4 FindTab(Tab *tab) { return FindSubControl(tab); }

	uint4 GetTabIndex() { return m_TabIndex; }
	void SetTabIndex(uint4 TabIndex);

	// Mo�na wywo�ywa� po zmianie prostok�ta za pomoc� SetRect, �eby uaktualni� pozycje swoich zak�adek
	// natychmiast, nie czekaj�c na komunikat OnRectChange.
	void UpdateTabRects();
};

// Reprezentuje wyskakuj�ce menu
// Obs�uga:
// - Utworzy� obiekt tej klasy. On sobie istnieje niewidzialny.
// - Wywo�a� metod� PopupMenu().
// - Czeka� na zdarzenie OnClick. (Je�li anulowane to go nie b�dzie.)
class Menu : public Control
{
public:
	// Pozycja menu
	struct ITEM
	{
		// G��wny tekst pozycji menu
		string Text;
		// Tekst dodatkowy
		string InfoText;
		// Identyfikator
		// MAXUINT4 oznacza separator.
		uint4 Id;

		// Tworzy niezainicjalizowany
		ITEM() { }
		// Tworzy i inicjuje
		ITEM(uint4 Id, const string &Text) : Id(Id), Text(Text) { }
		ITEM(uint4 Id, const string &Text, const string &InfoText) : Id(Id), Text(Text), InfoText(InfoText) { }
	};

	// Sta�a, kt�rej nale�y u�ywa� do kopiowania z niej separatora
	static const ITEM SEPARATOR;

private:
	std::vector<ITEM> m_Items;
	// Indeks zaznaczonej pozycji
	// Je�li poza zakresem to znaczy �e �adna.
	uint4 m_ItemIndex;

	// Warto�ci obliczone
	float m_MenuWidth;
	float m_MenuHeight;
	float m_ItemContentLeft;
	float m_ItemContentRight;
	std::vector<float> m_OffsetsY; // Pozycje na Y dla poszczeg�lnych pozycji menu z listy m_MenuItems + jedna za-ostatnia

	// Ponownie oblicza to co trzeba
	void HandleItemsChange();
	float GetMenuItemHeight();
	// Pr�buje zaznaczy� pozycj� menu o podanym indeksie lub nast�pn�, jeszcze nast�pn� itd. w d�
	void SelectDown(size_t Index);
	// Pr�buje zaznaczy� pozycj� menu o podanym indeksie lub poprzedni�, jeszcze poprzedni� itd. w g�r�
	void SelectUp(size_t Index);
	// Zwraca true, je�li pozycja menu o podanym indeksie jest zaznaczalna
	bool IsSelectable(size_t Index) { return Index < m_Items.size() && m_Items[Index].Id != MAXUINT4; }
	// Zwraca indeks pozycji menu pod podan� pozycj� kursora lub MAXUINT4 je�li nie znaleziono
	uint4 PosToItem(const VEC2 &Pos);

protected:
	// == Implementacja Control ==
	virtual void OnDraw(const VEC2 &Translation);
	virtual void OnMouseMove(const VEC2 &Pos);
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	virtual void OnMouseLeave() { m_ItemIndex = MAXUINT4; }
	virtual bool OnKeyDown(uint4 Key);

public:
	ID_EVENT OnClick;

	Menu();

	// Operacje na pozycjach menu
	size_t GetItemCount() { return m_Items.size(); }
	const ITEM & GetItem(uint4 Index) { return m_Items[Index]; }
	void AddItem(const ITEM &Item);
	void InsertItem(size_t Index, const ITEM &Item);
	void RemoveItem(size_t Index);
	size_t FindItem(uint4 Id); // Zwraca indeks pozycji o podanym Id lub MAXUINT4 je�li nie znajdzie.
	void SetItem(size_t Index, const ITEM &Item); // Zmienia dane pozycji menu
	void SetItemText(size_t Index, const string &Text); // Zmienia tekst pozycji menu
	void SetItemInfoText(size_t Index, const string &InfoText); // Zmienia info-tekst pozycji menu

	// Pokazuje menu
	void PopupMenu(const VEC2 &Pos);
	// Chowa menu
	void CloseMenu();
};

class Window : public CompositeControl
{
private:
	enum STATE
	{
		STATE_NONE,
		STATE_MOVE,
		STATE_SIZE_L,
		STATE_SIZE_R,
		STATE_SIZE_T,
		STATE_SIZE_B,
		STATE_SIZE_LT,
		STATE_SIZE_RT,
		STATE_SIZE_LB,
		STATE_SIZE_RB,
		STATE_CLOSE
	};
	// Za co aktualnie u�ytkownik trzyma myszk�
	STATE m_State;
	VEC2 m_StatePos;
	string m_Title;

	void GetTitleBarRect(RECTF *Out);
	void GetCloseButtonRect(const RECTF &TitleBarRect, RECTF *Out);

protected:
	// == Implementacja Control ==
	virtual void OnDraw(const VEC2 &Translation);
	virtual void OnMouseMove(const VEC2 &Pos);
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);

public:
	STANDARD_EVENT OnClose;
	STANDARD_EVENT OnMoveBegin;
	STANDARD_EVENT OnMoveEnd;
	STANDARD_EVENT OnResizeBegin;
	STANDARD_EVENT OnResizeEnd;

	Window(const RECTF &Rect, const string &Title);

	// == Implementacja CompositeControl ==
	virtual void GetClientRect(RECTF *Out);

	const string & GetTitle() { return m_Title; }
	void SetTitle(const string &NewTitle) { m_Title = NewTitle; }
};

// Mega wypasione pole edycyjne.
// Features:
// - Pozycjonowanie kursora (klawiatura, mysz)
// - Zaznaczanie
// - Obs�uga schowka
// - Wielopoziomowe cofanie
// Text - tekst przechowywany w kontrolce
// InfoText - tekst pokazywany kiedy kontrolka jest pusta i nieaktywna
class Edit : public Control
{
private:
	enum ACTION
	{
		ACTION_UNKNOWN,
		ACTION_TYPE,
		ACTION_DELETE,
		ACTION_UNDO,
		ACTION_REDO
	};

	struct STATE
	{
		// Dane
		string Text;
		size_t CursorPos;
		size_t SelBegin;
		size_t SelEnd;
		// Metadane
		ACTION Action;

		STATE(const string &Text, size_t CursorPos, size_t SelBegin, size_t SelEnd, ACTION Action) : Text(Text), CursorPos(CursorPos), SelBegin(SelBegin), SelEnd(SelEnd), Action(Action) { }
	};

	// == Te pola s� modyfikowane bezop�rednio podczas edycji ==
	// Trzeba ich pilnowa� (szczeg�lnie zakres�w indeks�w wzgl�dem tekstu).
	string m_Text;
	// Pozycja kursora mi�dzy znakami - 0...Text.length
	size_t m_CursorPos;
	// Pocz�tek i koniec zaznaczenia - 0...Text.length
	// SelBegin = SelEnd oznacza brak zaznaczenia.
	size_t m_SelBegin;
	size_t m_SelEnd;
	bool m_MouseDown;
	// 0..Text.length, wa�ny tylko kiedy MouseDown = true
	size_t m_MouseDownPos;

	// == Te pola s� aktualizowane automatycznie ==
	// Miejsce w tek�cie od kt�rego zaczyna si� on pokazywa� w obszarze klienta kontrolki EDIT
	float m_ScrollX;
	// Czas ostatniej edycji - �eby kursor nie znikn�� przez jaki� czas po tym
	float m_LastEditTime;

	// == Pozosta�e pola ==
	// Po utworzeniu ju� si� nie zmienia.
	bool m_Password;
	size_t m_MaxLength;
	string m_InfoText;
	// Czy po daniu focusa resetuje si� zaznaczenie na ca�o�� tekstu?
	bool m_SelectOnFocus;
	float m_TextWidth;
	float m_SelBeginPos;
	float m_SelEndPos;
	float m_CursorBeginPos;
	float m_CursorEndPos;
	std::deque<STATE> m_Undo;
	std::deque<STATE> m_Redo;

	// Wywo�ywa� kiedy co� si� zmieni�o
	// TextChanged = true - zmieni� si� te� tekst
	// TextChanged = false - zmieni�y sie tylko inne parametry np. zaznaczenia
	void Update(bool TextChanged, ACTION Action = ACTION_UNKNOWN);
	// Oblicza szeroko�� tekstu
	float GetTextWidth(size_t Begin, size_t End);
	float GetTextWidth() { return GetTextWidth(0, m_Text.length()); }
	// Zwraca szeroko�� obszaru wewn�trznego kontrolki (tego na tekst)
	float GetClientWidth();
	// Zwraca pozycj� na lewo od podanej (<=) o tyle �eby przeskoczy� do poprzedniego s�owa
	size_t GetCtrlLeftPos(size_t Pos);
	// Zwraca pozycj� na prawo od podanej (>=) o tyle �eby przeskoczy� do nast�pnego s�owa
	size_t GetCtrlRightPos(size_t Pos);
	bool MouseHitTest(size_t *OutPos, float X);
	void HandleMouseDrag(float X);

	// Rozpisana obs�uga poszczeg�lnych klawiszy, co by zorganizowa� kod
	void KeyLeft();
	void KeyRight();
	void KeyHome();
	void KeyEnd();
	void KeyDelete();
	void KeyEnter();
	void KeyBackspace();
	void KeyChar(char Ch);
	void KeyCtrlX();
	void KeyCtrlC();
	void KeyCtrlV();
	void KeyCtrlA();
	void KeyCtrlZ();
	void KeyCtrlY();

protected:
	// == Implementacja Control ==
	virtual void OnDraw(const VEC2 &Translation);
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	virtual void OnMouseMove(const VEC2 &Pos);
	virtual bool OnKeyDown(uint4 Key);
	virtual bool OnChar(char Ch);
	virtual void OnFocusEnter();
	virtual void OnRectChange() { Update(false); }

	// == Dla klas pochodnych ==
	// Domy�lnie wszystkie akceptuje.
	virtual bool OnValidateChar(char Ch);

public:
	STANDARD_EVENT OnChange;
	STANDARD_EVENT OnEnter; // Klawisz ENTER

	Edit(CompositeControl *Parent, const RECTF &Rect, bool Password = false);

	const string & GetText() { return m_Text; }
	const string & GetInfoText() { return m_InfoText; }
	size_t GetMaxLength() { return m_MaxLength; }
	void SetText(const string &Text);
	void SetInfoText(const string &InfoText) { m_InfoText = InfoText; }
	void SetMaxLength(size_t MaxLength);
	void SetSelectOnFocus(bool SelectOnFocus) { m_SelectOnFocus = SelectOnFocus; }
};

class IListModel
{
public:
	virtual size_t GuiList_GetItemCount() = 0;
	virtual void * GuiList_GetItem(size_t Index) = 0;

	// Indeks poza zakresem (np. MAXUINT4) oznacza brak zaznaczenia dla jakiegokolwiek elementu.
	virtual size_t GuiList_GetSelIndex() = 0;
	virtual void GuiList_SetSelIndex(size_t Index) = 0;
};

class List : public CompositeControl
{
public:
	typedef fastdelegate::FastDelegate3<List*, const VEC2&, size_t> LIST_ITEM_EVENT;
	typedef fastdelegate::FastDelegate2<List*, const VEC2&> LIST_HEADER_EVENT;

private:
	// Mo�e by� 0.
	IListModel *m_Model;
	// Jego Pos przechowuje faktyczn� pozycj� przewini�cia (pocz�tku obszaru klienta) na Y
	ScrollBar *m_ScrollBar;
	bool m_MouseDown;

	// == Dane cache'owane dla rozmiaru kontrolki ==
	RECTF m_ClientRect;
	RECTF m_HeaderRect;
	// ...oraz ScrollBar.Rect

	// Reakcja na ScrollBar.OnScroll
	void ScrollBarScroll(Control *c);
	// Sprawdza i w razie potrzeby limituje zakres ScrollY
	void CheckScrollY();
	// Zwraca indeks elementu w kt�ry trafili�my myszk�
	// bior�c pod uwag� pozycj� na Y wzgl�dem ClientRect oraz bie��ce ScrollY
	// Je�li nietrafione, indeks b�dzie poza zakresem.
	size_t HitTestItem(float Y);
	void HandleMouseSelect(const VEC2 &Pos);
	// Obs�uga klawiszy klawiatury
	void KeyDown();
	void KeyUp();
	void KeyPgup();
	void KeyPgdn();
	void KeyHome();
	void KeyEnd();
	void KeyEnter();
	void KeyDelete();
	void KeyEscape();

protected:
	// == Implementacja Control ==
	virtual void OnEnable() { }
	virtual void OnDisable() { m_MouseDown = false; }
	virtual void OnShow() { }
	virtual void OnHide() { m_MouseDown = false; }
	virtual void OnRectChange();
	virtual void OnDraw(const VEC2 &Translation);
	virtual void OnMouseMove(const VEC2 &Pos);
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	virtual void OnMouseWheel(const VEC2 &Pos, float Delta);
	virtual bool OnKeyDown(uint4 Key);
	virtual bool OnKeyUp(uint4 Key);
	virtual bool OnChar(char Ch);

public:
	// Co� zosta�o zaznaczone, og�lnie zaznaczenie si� zmieni�o
	// (z inicjatywy kontrolki lub jej u�ytkownika, poza tym mo�e te� si� zmienia� w modelu)
	// Indeks mo�e by� poza zakresem, wtedy znaczy �e nie trafiono w �adn� pozycj�.
	STANDARD_EVENT OnSelChange;
	LIST_HEADER_EVENT OnHeaderClick;
	LIST_HEADER_EVENT OnHeaderRightClick;
	LIST_HEADER_EVENT OnHeaderDoubleClick;
	LIST_ITEM_EVENT OnItemClick;
	LIST_ITEM_EVENT OnItemRightClick;
	LIST_ITEM_EVENT OnItemDoubleClick;
	STANDARD_EVENT OnKeyDelete;
	STANDARD_EVENT OnKeyEnter;
	STANDARD_EVENT OnKeyEscape;

	List(CompositeControl *Parent, const RECTF &Rect, IListModel *Model);
	// Wywo�ywa� po utworzeniu, bo w konstruktorze to nie mo�e by� - nie dzia�aj� tam funkcje wirtualne
	void Ctor();
	~List();

	// == Implementacja CompositeControl ==
	virtual void GetClientRect(RECTF *Out);

	IListModel * GetModel() { return m_Model; }
	void SetModel(IListModel *NewModel);

	// Aktualizuje wszystko co wewn�trznie cache'uje
	// Wywo�ywa� po jaki� zmianach typu �e teraz b�dzie inna wysoko�� elementu.
	// Wywo�ywa� te� po zmianie danych (zmianie ilo�ci, dodawaniu i usuwaniu element�w).
	// Wywo�uje si� te� automatycznie z wn�trza.
	// Po zmienie zaznaczenia te� wywo�ywa�.
	void Update();
	// Przewija list� tak, �eby na pewno by� widoczny element o podanym indeksie
	void ScrollTo(size_t Index);
	// Zwraca indeks elementu w kt�ry trafili�my myszk�
	// Je�li nietrafione, indeks b�dzie poza zakresem.
	size_t HitTestItem(const VEC2 &Pos);

	// == Dla klas pochodnych ==
	// Zwraca margines od kraw�dzi kontrolki do jej tre�ci
	virtual float GetMargin() = 0;
	// Zwraca wysoko�� nag��wka (mo�e zwr�ci� 0)
	virtual float GetHeaderHeight() = 0;
	// Zwraca szeroko�� paska przewijania
	virtual float GetScrollBarSize() = 0;
	// Zwraca wysoko�� pojedynczego elementu listy
	virtual float GetItemHeight() = 0;
	// Pocz�tek rysowania
	virtual void DrawBegin() = 0;
	// Koniec rysowania
	virtual void DrawEnd() = 0;
	// Zwraca liczb� przebieg�w rysowania element�w
	virtual size_t GetDrawPassCount() = 0;
	// Rysuje t�o kontrolki
	virtual void DrawBackground() = 0;
	// Rysuje nag��wek kontrolki
	virtual void DrawHeader(const RECTF &Rect) = 0;
	// Pocz�tek przebiegu rysowania element�w
	virtual void DrawPassBegin(size_t Pass) = 0;
	// Koniec przebiegu rysowania element�w
	virtual void DrawPassEnd(size_t Pass) = 0;
	// Rysuje element modelu
	virtual void DrawItem(size_t Pass, const RECTF &Rect, size_t Index) = 0;
	// Do rysowania pojedynczego elementu tej listy na zewn�trz
	// Jego nie obowi�zuj� Passy.
	virtual void DrawSingleItem(const RECTF &Rect, size_t Index) = 0;
};

// Prosta implementacja interfejsu IListModel dla wektora string�w
class StringListModel : public IListModel
{
private:
	STRING_VECTOR m_Items;
	size_t m_SelIndex;

public:
	StringListModel() : m_SelIndex(MAXUINT4) { }

	// == Implementacja IListModel ==

	virtual size_t GuiList_GetItemCount() { return m_Items.size(); }
	virtual void * GuiList_GetItem(size_t Index) { return &m_Items[Index]; }
	virtual size_t GuiList_GetSelIndex() { return m_SelIndex; }
	virtual void GuiList_SetSelIndex(size_t Index) { m_SelIndex = Index; }

	size_t GetCount() { return m_Items.size(); }
	const string & Get(size_t Index) { return m_Items[Index]; }
	void Set(size_t Index, const string &s) { m_Items[Index] = s; }
	void Add(const string &s) { m_Items.push_back(s); }
	void Insert(size_t Index, const string &s) { m_Items.insert(m_Items.begin()+Index, s); if (m_SelIndex <= Index) m_SelIndex++; }
	void Clear() { m_Items.clear(); m_SelIndex = MAXUINT4; }
	void Remove(size_t Index) { m_Items.erase(m_Items.begin()+Index); if (m_SelIndex != MAXUINT4 && m_SelIndex > Index) m_SelIndex--; }
};

class StringList : public List
{
private:
	StringListModel m_Model;

public:
	StringList(CompositeControl *Parent, const RECTF &Rect);

	StringListModel & GetModel() { return m_Model; }

	// == Implementacja SampleList ==
	virtual float GetMargin();
	virtual float GetHeaderHeight();
	virtual float GetScrollBarSize();
	virtual float GetItemHeight();
	virtual void DrawBegin() { }
	virtual void DrawEnd() { }
	virtual size_t GetDrawPassCount() { return 2; }
	virtual void DrawBackground();
	virtual void DrawHeader(const RECTF &Rect);
	virtual void DrawPassBegin(size_t Pass) { }
	virtual void DrawPassEnd(size_t Pass) { }
	virtual void DrawItem(size_t Pass, const RECTF &Rect, size_t Index);
	virtual void DrawSingleItem(const RECTF &Rect, size_t Index);
};

// ComboBox z editem do wpisywania w�asnego �a�cucha lub wybrania z listy
class TextComboBox : public CompositeControl
{
private:
	Edit *m_Edit;
	SpriteButton *m_Button;
	StringList *m_List;

	// Pokazuje list�, daj�c jej focus, inicjalizuj�c pozycj� i indeks zaznaczenia
	void ShowList();
	// Chowa list�
	void HideList();
	// Aktualizuje pozycje podkontrolek i wszelkie wewn�trzne dane
	void Update();

	// Reakcje na zdarzenia podkontrolek
	void ButtonClick(Control *c);
	void ListKeyEnter(Control *c);
	void ListKeyEscape(Control *c);
	void ListItemClick(List *c, const VEC2 &Pos, size_t Index);

protected:
	// == Implementacja Control ==
	virtual void OnDisable() { HideList(); }
	virtual void OnHide() { HideList(); }
	virtual void OnRectChange() { Update(); }
	virtual bool OnKeyDown(uint4 Key);

public:
	// List� trzeba tworzy� we w�asnym zakresie, bo technicznie niemo�liwe jest �eby kontrolka mia�a
	// podkontrolk� kt�ra logicznie b�dzie jej, a fizycznie b�dzie poziomu najwy�szego - nie ma jej jak usun��.
	TextComboBox(CompositeControl *Parent, const RECTF &Rect, StringList *List);
	~TextComboBox();

	Edit & GetEdit() { return *m_Edit; }
	// U�ytkownik ma dost�p do listy celem:
	// - Wpisania do niej element�w
	// - Ustawienia szeroko�ci i wysoko�ci jej prostok�ta (np. jako 0,0,W,H)
	StringList * GetList() { return m_List; }
	void SetList(StringList *NewList);
};

// Uniwersalny ComboBox z wyborem spo�r�d element�w listy
// Aktualny wyb�r przechowuje List.SelIndex
class ListComboBox : public CompositeControl
{
private:
	SpriteButton *m_Button;
	List *m_List;

	// Pokazuje list�, daj�c jej focus, inicjalizuj�c pozycj� i indeks zaznaczenia
	void ShowList();
	// Chowa list�
	void HideList();
	// Aktualizuje pozycje podkontrolek i wszelkie wewn�trzne dane
	void Update();

	// Reakcje na zdarzenia podkontrolek
	void ButtonClick(Control *c);
	void ListKeyEnter(Control *c);
	void ListKeyEscape(Control *c);
	void ListItemClick(List *c, const VEC2 &Pos, size_t Index);

protected:
	// == Implementacja Control ==
	virtual void OnDraw(const VEC2 &Translation);
	virtual void OnDisable() { HideList(); }
	virtual void OnHide() { HideList(); }
	virtual void OnRectChange() { Update(); }
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	virtual bool OnKeyDown(uint4 Key);

public:
	// List� trzeba tworzy� we w�asnym zakresie, bo technicznie niemo�liwe jest �eby kontrolka mia�a
	// podkontrolk� kt�ra logicznie b�dzie jej, a fizycznie b�dzie poziomu najwy�szego - nie ma jej jak usun��.
	ListComboBox(CompositeControl *Parent, const RECTF &Rect, bool Focusable, List *a_List);
	~ListComboBox();

	// U�ytkownik ma dost�p do listy celem:
	// - Wpisania do niej element�w
	// - Ustawienia szeroko�ci i wysoko�ci jej prostok�ta (np. jako 0,0,W,H)
	List * GetList() { return m_List; }
	void SetList(List *NewList);

	// Zwraca indeks zaznaczonego elementu listy lub MAXUINT4 je�li �aden nie zaznaczony.
	size_t GetSelIndex() { if (m_List == 0) return MAXUINT4; if (m_List->GetModel()->GuiList_GetSelIndex() >= m_List->GetModel()->GuiList_GetItemCount()) return MAXUINT4; return m_List->GetModel()->GuiList_GetSelIndex(); }
};


void InitGuiControls(res::Font *StandardFont, gfx2d::Sprite *GuiSprite);
// Generuje unikalne identyfikatory do r�nych rzeczy
// Generowane identyfikatory zaczynaj� si� od 1000000000.
uint4 GenerateId();


} // namespace gui

#endif
