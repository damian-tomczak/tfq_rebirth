/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\GUI_PropertyGridWindow.hpp"
#include "DebugGui.hpp"


const char * const ABOUT_TEXT =
	"The Final Quest 7\n"
	"Final Demo\n"
	"Wersja 1.1, 20 listopada 2007\n"
	"\n"
	"Autor:\n"
	"Adam Sawicki\n"
	"sawickiap@poczta.onet.pl\n"
	"http://regedit.gamedev.pl\n"
	"\n"
	"Wi�cej informacji w pliku ReadMe.html";

const char * const SHORTCUTS_TEXT =
	"ESC\n"
	"  Wyj�cie\n"
	"W, S, A, D, strza�ki\n"
	"  Sterowanie gr�\n"
  	"Ruch myszk�, lewy przycisk myszy\n"
	"  Sterowanie gr�\n"
	"Ctrl + Enter\n"
	"  Pe�ny ekran / okno\n"
	"\n"
	"Plus te, kt�re znajdziesz w menu po klikni�ciu na okr�g�y przycisk na g�rze.";

const bool INITIAL_SHOW_STATS = false;
const bool INITIAL_SHOW_TEXTURES = false;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa MenuButton

class MenuButton : public gui::Control
{
private:
	gfx2d::Sprite *m_Sprite;

protected:
	// ======== Implementacja Control ========
	virtual void OnDraw(const VEC2 &Translation);
	virtual bool OnHitTest(const VEC2 &Pos);
	virtual void OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);

public:
	gui::STANDARD_EVENT OnClick;

	MenuButton(gui::CompositeControl *Parent, const RECTF &Rect);
};

void MenuButton::OnDraw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->SetSprite(m_Sprite);
	gfx2d::g_Canvas->SetColor(COLOR::WHITE);
	gfx2d::g_Canvas->DrawRect(GetLocalRect(), IsMouseOver() ? 1 : 0);
}

bool MenuButton::OnHitTest(const VEC2 &Pos)
{
	VEC2 Center; GetLocalRect().GetCenter(&Center);
	float Radius = GetLocalRect().bottom * 0.5f;
	return (Distance(Pos, Center) <= Radius);
}

void MenuButton::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Button == frame::MOUSE_LEFT && Action == frame::MOUSE_DOWN)
	{
		if (OnClick) OnClick(this);
	}
}

MenuButton::MenuButton(gui::CompositeControl *Parent, const RECTF &Rect) :
	gui::Control(Parent, false, Rect)
{
	SetCursor(gui::g_StandardCursors[gui::CT_LINK]);
	m_Sprite = gfx2d::g_SpriteRepository->MustGetSprite("MenuButton");
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TextWindow

/* Okienko pokazuj�ce po prostu jaki�. */

class TextWindow : public gui::Window
{
private:
	scoped_ptr<gui::Label> m_Label;

	void UpdateRects();

protected:
	// ======== Implementacja Control ========
	virtual void OnRectChange() { UpdateRects(); }

public:
	TextWindow(const RECTF &Rect, const string &Title, const string &Text = "");
	void SetText(const string &Text) { m_Label->SetText(Text); }
};

TextWindow::TextWindow(const RECTF &Rect, const string &Title, const string &Text) :
	gui::Window(Rect, Title)
{
	m_Label.reset(new gui::Label(this, RECTF::ZERO, Text,
		res::Font::FLAG_HLEFT | res::Font::FLAG_VTOP | res::Font::FLAG_WRAP_WORD));

	UpdateRects();
}

void TextWindow::UpdateRects()
{
	RECTF ClientRect; GetClientRect(&ClientRect);
	ClientRect -= VEC2(ClientRect.left, ClientRect.top);
	ClientRect.Extend(-gui::MARGIN);

	m_Label->SetRect(ClientRect);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa SettingsWindow

/*
To okienko ma kolejno tak:
- Pe�ny ekran - CheckBox [0]
- Rozdzielczo�� - ComboBox [0]
- Od�wie�anie - ComboBox [1]
- VSync - CheckBox [1]
- FlushMode - ComboBox [2]
*/

const float SETTINGS_BUTTON_WIDTH = 70.0f;
const char * const SETTINGS_LABEL_TEXTS[] = {
	"Pe�ny ekran",
	"Rozdzielczo��",
	"Cz�stotliwo�� od�wie�ania",
	"Synchronizacja pionowa",
	"Tryb oczekiwania",
};
const char * const SETTINGS_HINTS[] = {
	"Czy aplikacja ma dzia�a� w trybie pe�noekranowym, czy w zwyk�ym oknie Windows?",
	"Rozdzielczo�� ekranu w pikselach.",
	"Cz�stotliwo�� od�wie�ania ekranu w Hz.",
	"Pionowa synchronizacja, czyli oczekiwanie na powr�t plamki.\n"
		"- Wy��czona - FPS-�w b�dzie najwi�cej jak tylko mo�liwe.\n"
		"- W��czona - FPS-�w b�dzie nie wi�cej, ni� cz�stotliwo�� od�wie�ania ekranu.",
	"Spos�b, w jaki program ma czeka� przed wy�wietleniem ka�dej klatki.\n"
		"- Brak (zalecane) - tryb zapewniaj�cy najszybsze dzia�anie.\n"
		"- Event - w��cz, je�li gra \"szarpie\".\n"
		"- Lock - w��cz, je�li gra \"szarpie\", a tryb Event nie pomaga.",
};
// Nazwy tryb�w FlushMode
const char * const FLUSH_MODE_NAMES[] = {
	"Brak",
	"Event",
	"Lock",
};

const float SETTINGS_WINDOW_CX = 400.0f;
const float SETTINGS_WINDOW_CY = 226.0f;

class SettingsWindow : public gui::Window
{
private:
	class DisplayModeCompare
	{
	public:
		bool operator () (const D3DDISPLAYMODE &m1, const D3DDISPLAYMODE &m2)
		{
			if (m1.Width != m2.Width)
				return (m1.Width < m2.Width);
			if (m1.Height != m2.Height)
				return (m1.Height < m2.Height);
			if (m1.RefreshRate != m2.RefreshRate)
				return (m1.RefreshRate < m2.RefreshRate);
			return m1.Format < m2.Format;
		}
	};

	// Tryby graficzne dla aktywnego adaptera
	// Tylko o formacie X8R8G8B8
	// Posortowane wg Width > Height > RefreshRate
	std::vector<D3DDISPLAYMODE> m_DisplayModes;
	
	// Nie wsz�dzie s� inteligentne wska�niki, bo kontrolka sama usuwa swoje pod-kontrolki.

	scoped_ptr<gui::StringHint> m_Hints[5];
	gui::Label *m_Labels[5];

	scoped_ptr<gui::StringList, ReleasePolicy> m_Lists[3];
	gui::ListComboBox *m_ComboBoxes[3];
	gui::CheckBox *m_CheckBoxes[2];

	gui::Label *m_Label;
	gui::Button *m_OkButton;
	gui::Button *m_CancelButton;
	gui::Button *m_ApplyButton;

	// Aktualizuje rozmieszczenie kontrolek
	void UpdateRects();

	bool StrToResolution(const string &s, uint4 *Width, uint4 *Height);
	bool StrToRefreshRate(const string &s, uint4 *RefreshRate);
	bool StrToFlushMode(const string &s, frame::SETTINGS::FLUSH_MODE *OutFlushMode);
	void ResolutionToStr(string *Out, uint4 Width, uint4 Height);
	void RefreshRateToStr(string *Out, uint4 RefreshRate);
	void FlushModeToStr(string *Out, frame::SETTINGS::FLUSH_MODE FlushMode);

	// Wype�nia tablic� m_DisplayModes
	void LoadDisplayModes();
	// Wype�nia list� rozdzielczo�ci dost�pnymi rozdzielczo�ciami z m_DisplayModes
	void FillResolutions();
	// Wype�nia list� Flush Modes
	void FillFlushModes();
	// Zaznacza na li�cie podan� rozdzielczo��
	void SelectResolution(uint4 Width, uint4 Height);
	// Wype�nia list� cz�stotliwo�ci od�wie�ania dost�pnymi dla podanej rozdzielczo�ci
	void FillRefreshRates(uint4 Width, uint4 Height);
	// Zaznacza na li�cie podan� cz�stotliwo�� od�wie�ania
	void SelectRefreshRate(uint4 RefreshRate);
	// Zaznacza na li�cie podany tryb FlushMode
	void SelectFlushMode(frame::SETTINGS::FLUSH_MODE FlushMode);
	// Zczytuje dane z wype�nionego formularza do struktury z ustawieniami.
	// Wype�nia tylko te pola kt�re obs�uguje - pozosta�e pozostawia.
	bool GuiToSettings(frame::SETTINGS *Out);

	void FullScreenCheckBoxClick(gui::Control *c);
	void ResolutionListSelChange(gui::Control *c);
	void OkButtonClick(gui::Control *c);
	void CancelButtonClick(gui::Control *c);
	void ApplyButtonClick(gui::Control *c);

protected:
	// ======== Implementacja Control ========
	virtual void OnRectChange() { UpdateRects(); }

public:
	SettingsWindow(const RECTF &Rect);
	~SettingsWindow();

	void DisplaySettingsChanged(bool Succeeded);
};

void SettingsWindow::DisplaySettingsChanged(bool Succeeded)
{
	if (Succeeded)
		m_Label->SetText("Tryb zmieniony.");
	else
		m_Label->SetText("Nie uda�o si�!");
}

void SettingsWindow::ResolutionToStr(string *Out, uint4 Width, uint4 Height)
{
	*Out = Format("# x #") % Width % Height;
}

void SettingsWindow::RefreshRateToStr(string *Out, uint4 RefreshRate)
{
	*Out = Format("# Hz") % RefreshRate;
}

void SettingsWindow::FlushModeToStr(string *Out, frame::SETTINGS::FLUSH_MODE FlushMode)
{
	switch (FlushMode)
	{
	case frame::SETTINGS::FLUSH_NONE:
		*Out = FLUSH_MODE_NAMES[0];
		break;
	case frame::SETTINGS::FLUSH_EVENT:
		*Out = FLUSH_MODE_NAMES[1];
		break;
	case frame::SETTINGS::FLUSH_LOCK:
		*Out = FLUSH_MODE_NAMES[2];
		break;
	}
}

bool SettingsWindow::StrToResolution(const string &s, uint4 *Width, uint4 *Height)
{
	string s1, s2;

	size_t Pos1 = s.find(" x ");
	if (Pos1 == string::npos) return false;

	s1 = s.substr(0, Pos1);
	s2 = s.substr(Pos1+3);

	if (StrToUint(Width, s1) != 0) return false;
	if (StrToUint(Height, s2) != 0) return false;
	return true;
}

bool SettingsWindow::StrToRefreshRate(const string &s, uint4 *RefreshRate)
{
	size_t Pos1 = s.find(" Hz");
	if (Pos1 == string::npos) return false;

	if (StrToUint(RefreshRate, s.substr(0, Pos1)) != 0) return false;
	return true;
}

bool SettingsWindow::StrToFlushMode(const string &s, frame::SETTINGS::FLUSH_MODE *OutFlushMode)
{
	if (s == FLUSH_MODE_NAMES[0])
		*OutFlushMode = frame::SETTINGS::FLUSH_NONE;
	else if (s == FLUSH_MODE_NAMES[1])
		*OutFlushMode = frame::SETTINGS::FLUSH_EVENT;
	else if (s == FLUSH_MODE_NAMES[2])
		*OutFlushMode = frame::SETTINGS::FLUSH_LOCK;
	else
		return false;

	return true;
}

void SettingsWindow::LoadDisplayModes()
{
	if (!frame::D3D) return;
	uint4 ModeCount = frame::D3D->GetAdapterModeCount(frame::GetAdapter(), frame::GetAdapterFormat());

	D3DDISPLAYMODE DM;
	for (uint4 mi = 0; mi < ModeCount; mi++)
	{
		frame::D3D->EnumAdapterModes(frame::GetAdapter(), frame::GetAdapterFormat(), mi, &DM);
		if (DM.Format == D3DFMT_X8R8G8B8)
			m_DisplayModes.push_back(DM);
	}

	std::sort(m_DisplayModes.begin(), m_DisplayModes.end(), DisplayModeCompare());
}

void SettingsWindow::FillResolutions()
{
	m_Lists[0]->GetModel().Clear();

	uint4 LastWidth = MAXUINT4, LastHeight = MAXUINT4;
	string s;
	for (size_t i = 0; i < m_DisplayModes.size(); i++)
	{
		if (m_DisplayModes[i].Width != LastWidth || m_DisplayModes[i].Height != LastHeight)
		{
			ResolutionToStr(&s, m_DisplayModes[i].Width, m_DisplayModes[i].Height);
			m_Lists[0]->GetModel().Add(s);
		}
		LastWidth = m_DisplayModes[i].Width;
		LastHeight = m_DisplayModes[i].Height;
	}

	m_Lists[0]->Update();
}

void SettingsWindow::FillFlushModes()
{
	m_Lists[2]->GetModel().Clear();

	for (uint i = 0; i < 3; i++)
		m_Lists[2]->GetModel().Add(FLUSH_MODE_NAMES[i]);

	m_Lists[2]->Update();
}

void SettingsWindow::SelectResolution(uint4 Width, uint4 Height)
{
	uint4 w, h;

	m_Lists[0]->GetModel().GuiList_SetSelIndex(MAXUINT4);

	for (size_t i = 0; i < m_Lists[0]->GetModel().GetCount(); i++)
	{
		if (StrToResolution(m_Lists[0]->GetModel().Get(i), &w, &h) && w == Width && h == Height)
		{
			m_Lists[0]->GetModel().GuiList_SetSelIndex(i);
			break;
		}
	}

	m_Lists[0]->Update();
}

void SettingsWindow::FillRefreshRates(uint4 Width, uint4 Height)
{
	gui::StringListModel & Model = m_Lists[1]->GetModel();

	// Pobierz warto�� zaznaczenia
	string SelValue;
	size_t OldIndex = Model.GuiList_GetSelIndex();
	if (OldIndex < Model.GetCount())
		SelValue = Model.Get(OldIndex);

	Model.Clear();

	string s;
	for (size_t i = 0; i < m_DisplayModes.size(); i++)
	{
		if (m_DisplayModes[i].Width == Width && m_DisplayModes[i].Height == Height)
		{
			RefreshRateToStr(&s, m_DisplayModes[i].RefreshRate);
			Model.Add(s);
		}
	}

	// Zaznacz nowy element taki jak poprzedni
	if (!SelValue.empty())
	{
		for (size_t j = 0; j < Model.GetCount(); j++)
		{
			if (Model.Get(j) == SelValue)
			{
				Model.GuiList_SetSelIndex(j);
				break;
			}
		}
	}

	m_Lists[1]->Update();

	// Poka� zaznaczony element
	if (Model.GuiList_GetSelIndex() < Model.GetCount())
		m_Lists[1]->ScrollTo(Model.GuiList_GetSelIndex());
}

void SettingsWindow::SelectRefreshRate(uint4 RefreshRate)
{
	uint4 r;

	m_Lists[1]->GetModel().GuiList_SetSelIndex(MAXUINT4);

	for (size_t i = 0; i < m_Lists[1]->GetModel().GetCount(); i++)
	{
		if (StrToRefreshRate(m_Lists[1]->GetModel().Get(i), &r) && r == RefreshRate)
		{
			m_Lists[1]->GetModel().GuiList_SetSelIndex(i);
			break;
		}
	}

	m_Lists[1]->Update();
}

void SettingsWindow::SelectFlushMode(frame::SETTINGS::FLUSH_MODE FlushMode)
{
	m_Lists[2]->GetModel().GuiList_SetSelIndex( (uint)FlushMode );
	m_Lists[2]->Update();
}

bool SettingsWindow::GuiToSettings(frame::SETTINGS *Out)
{
	*Out = frame::Settings;

	gui::StringListModel *Model;
	size_t SelIndex;

	// FullScreen
	Out->FullScreen = (m_CheckBoxes[0]->GetState() == gui::CheckBox::CHECKED);

	// Rozdzielczo�� i od�wie�anie
	if (Out->FullScreen)
	{
		Model = &m_Lists[0]->GetModel();
		SelIndex = Model->GuiList_GetSelIndex();
		if (SelIndex >= Model->GetCount()) return false;
		if (!StrToResolution(Model->Get(SelIndex), &Out->BackBufferWidth, &Out->BackBufferHeight)) return false;

		Model = &m_Lists[1]->GetModel();
		SelIndex = Model->GuiList_GetSelIndex();
		if (SelIndex >= Model->GetCount()) return false;
		if (!StrToRefreshRate(Model->Get(SelIndex), &Out->RefreshRate)) return false;
	}
	else
	{
		Out->BackBufferWidth = 800;
		Out->BackBufferHeight = 600;
		Out->RefreshRate = 0; // nie ma znaczenia
	}

	// PresentationInterval
	Out->PresentationInverval = (m_CheckBoxes[1]->GetState() == gui::CheckBox::CHECKED ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE);

	// FlushMode
	Model = &m_Lists[2]->GetModel();
	SelIndex = Model->GuiList_GetSelIndex();
	if (SelIndex >= Model->GetCount()) return false;
	if (!StrToFlushMode(Model->Get(SelIndex), &Out->FlushMode)) return false;

	return true;
}

void SettingsWindow::FullScreenCheckBoxClick(gui::Control *c)
{
	// Je�li to pe�ny ekran
	if (m_CheckBoxes[0]->GetState() == gui::CheckBox::CHECKED)
	{
		m_ComboBoxes[0]->SetEnabled(true);
		m_ComboBoxes[1]->SetEnabled(true);
	}
	// Je�li to nie pe�ny ekran
	else
	{
		m_ComboBoxes[1]->SetEnabled(false);
		m_ComboBoxes[0]->SetEnabled(false);
	}
}

void SettingsWindow::ResolutionListSelChange(gui::Control *c)
{
	if (m_CheckBoxes[0]->GetState() != gui::CheckBox::CHECKED) return;

	// Od�wie� cz�stotliwo�ci do�wie�ania stosownie do wybranej rozdzia�ki

	// Je�li to pe�ny ekran
	uint4 Width, Height;
	size_t ResIndex = m_Lists[0]->GetModel().GuiList_GetSelIndex();
	if (ResIndex >= m_Lists[0]->GetModel().GetCount())
		Width = Height = 0;
	else
	{
		if (!StrToResolution(m_Lists[0]->GetModel().Get(ResIndex), &Width, &Height))
			Width = Height = 0;
	}
	// Wype�nij list� format�w backbuffera mo�liwymi warto�ciami dla bie��cych parametr�w
	FillRefreshRates(Width, Height);
}

void SettingsWindow::OkButtonClick(gui::Control *c)
{
	ApplyButtonClick(c);
	CancelButtonClick(c);
}

void SettingsWindow::CancelButtonClick(gui::Control *c)
{
	if (OnClose) OnClose(c);
}

void SettingsWindow::ApplyButtonClick(gui::Control *c)
{
	frame::SETTINGS Settings;
	if (GuiToSettings(&Settings))
		frame::ChangeSettings(Settings);
	else
		m_Label->SetText("B��dne dane!");
}

SettingsWindow::SettingsWindow(const RECTF &Rect) :
	gui::Window(Rect, "Ustawienia graficzne")
{
	LoadDisplayModes();

	for (size_t i = 0; i < 5; i++)
	{
		m_Hints[i].reset(new gui::StringHint(SETTINGS_HINTS[i]));
		m_Labels[i] = new gui::Label(this, RECTF::ZERO, SETTINGS_LABEL_TEXTS[i], res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
		m_Labels[i]->SetHint(m_Hints[i].get());
	}

	// FullScreen
	m_CheckBoxes[0] = new gui::CheckBox(
		this,
		RECTF::ZERO,
		frame::Settings.FullScreen ? gui::CheckBox::CHECKED : gui::CheckBox::UNCHECKED,
		"W��czony");
	m_CheckBoxes[0]->SetHint(m_Hints[0].get());
	m_CheckBoxes[0]->OnClick.bind(this, &SettingsWindow::FullScreenCheckBoxClick);

	// Resolution
	m_Lists[0].reset(new gui::StringList(NULL, RECTF::ZERO));
	m_ComboBoxes[0] = new gui::ListComboBox(this, RECTF::ZERO, true, m_Lists[0].get());
	m_Lists[0]->SetHint(m_Hints[1].get());
	m_ComboBoxes[0]->SetHint(m_Hints[1].get());
	FillResolutions();
	if (frame::Settings.FullScreen)
		SelectResolution(frame::Settings.BackBufferWidth, frame::Settings.BackBufferHeight);
	else
		m_ComboBoxes[0]->SetEnabled(false);
	m_Lists[0]->OnSelChange.bind(this, &SettingsWindow::ResolutionListSelChange);

	// RefreshRate
	m_Lists[1].reset(new gui::StringList(NULL, RECTF::ZERO));
	m_ComboBoxes[1] = new gui::ListComboBox(this, RECTF::ZERO, true, m_Lists[1].get());
	m_Lists[1]->SetHint(m_Hints[2].get());
	m_ComboBoxes[1]->SetHint(m_Hints[2].get());
	if (frame::Settings.FullScreen)
	{
		FillRefreshRates(frame::Settings.BackBufferWidth, frame::Settings.BackBufferHeight);
		SelectRefreshRate(frame::Settings.RefreshRate);
	}
	else
		m_ComboBoxes[1]->SetEnabled(false);

	// VSync
	m_CheckBoxes[1] = new gui::CheckBox(
		this,
		RECTF::ZERO,
		frame::Settings.PresentationInverval == D3DPRESENT_INTERVAL_IMMEDIATE ? gui::CheckBox::UNCHECKED : gui::CheckBox::CHECKED,
		"W��czona");
	m_CheckBoxes[1]->SetHint(m_Hints[3].get());

	// FlushMode
	m_Lists[2].reset(new gui::StringList(NULL, RECTF::ZERO));
	m_ComboBoxes[2] = new gui::ListComboBox(this, RECTF::ZERO, true, m_Lists[2].get());
	m_Lists[2]->SetHint(m_Hints[4].get());
	m_ComboBoxes[2]->SetHint(m_Hints[4].get());
	FillFlushModes();
	SelectFlushMode(frame::Settings.FlushMode);

	m_Label = new gui::Label(this, RECTF::ZERO, string(), res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);

	m_OkButton = new gui::Button(this, RECTF::ZERO, "OK", true);
	m_CancelButton = new gui::Button(this, RECTF::ZERO, "Anuluj", true);
	m_ApplyButton = new gui::Button(this, RECTF::ZERO, "Zastosuj", true);

	m_OkButton->OnClick.bind(this, &SettingsWindow::OkButtonClick);
	m_CancelButton->OnClick.bind(this, &SettingsWindow::CancelButtonClick);
	m_ApplyButton->OnClick.bind(this, &SettingsWindow::ApplyButtonClick);

	UpdateRects();
}

SettingsWindow::~SettingsWindow()
{
}

void SettingsWindow::UpdateRects()
{
	RECTF ClientRect; GetClientRect(&ClientRect);
	ClientRect -= VEC2(ClientRect.left, ClientRect.top);
	
	float LabelX1 = gui::MARGIN;
	float LabelX2 = ClientRect.right / 2.0f - gui::MARGIN;
	float ControlX1 = LabelX2 + gui::MARGIN;
	float ControlX2 = ClientRect.right - gui::MARGIN;
	float Y1 = gui::MARGIN, Y2;
	RECTF ControlRect;

	for (int i = 0; i < 5; i++)
	{
		Y2 = Y1 + gui::CONTROL_HEIGHT;

		m_Labels[i]->SetRect(RECTF(LabelX1, Y1, LabelX2, Y2));

		ControlRect = RECTF(ControlX1, Y1, ControlX2, Y2);
		switch (i)
		{
		case 0:
			m_CheckBoxes[0]->SetRect(ControlRect);
			break;
		case 1:
			m_ComboBoxes[0]->SetRect(ControlRect);
			m_Lists[0]->SetRect(RECTF(0.0f, 0.0f, ControlX2-ControlX1, gui::CONTROL_HEIGHT*5));
			break;
		case 2:
			m_ComboBoxes[1]->SetRect(ControlRect);
			m_Lists[1]->SetRect(RECTF(0.0f, 0.0f, ControlX2-ControlX1, gui::CONTROL_HEIGHT*5));
			break;
		case 3:
			m_CheckBoxes[1]->SetRect(ControlRect);
			break;
		case 4:
			m_ComboBoxes[2]->SetRect(ControlRect);
			m_Lists[2]->SetRect(RECTF(0.0f, 0.0f, ControlX2-ControlX1, gui::CONTROL_HEIGHT*5));
			break;
		}

		Y1 = Y2 + gui::MARGIN;
	}

	Y1 += gui::CONTROL_HEIGHT;
	Y2 = Y1 + gui::CONTROL_HEIGHT;

	m_Label->SetRect(RECTF(LabelX1, Y1, LabelX2, Y2));
	float X1, X2;
	X2 = ClientRect.right - gui::MARGIN; X1 = X2 - SETTINGS_BUTTON_WIDTH;
	m_ApplyButton->SetRect(RECTF(X1, Y1, X2, Y2));
	X2 = X1 - gui::MARGIN; X1 = X2 - SETTINGS_BUTTON_WIDTH;
	m_CancelButton->SetRect(RECTF(X1, Y1, X2, Y2));
	X2 = X1 - gui::MARGIN; X1 = X2 - SETTINGS_BUTTON_WIDTH;
	m_OkButton->SetRect(RECTF(X1, Y1, X2, Y2));
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa DebugGui_pimpl

const uint4 PROPERTY_GRID_WINDOW_COUNT = 1;

class DebugGui_pimpl
{
public:
	scoped_ptr<MenuButton, ReleasePolicy> m_MenuButton;
	scoped_ptr<gui::Menu, ReleasePolicy> m_Menu;
	scoped_ptr<TextWindow, ReleasePolicy> m_AboutWindow;
	scoped_ptr<TextWindow, ReleasePolicy> m_ShortcutsWindow;
	scoped_ptr<SettingsWindow, ReleasePolicy> m_SettingsWindow;
	bool m_ShowStats;
	bool m_ShowTextures;
	scoped_ptr<gui::PropertyGridWindow, ReleasePolicy> m_PropertyGridWindows[PROPERTY_GRID_WINDOW_COUNT];
	fastdelegate::FastDelegate1<uint> m_GameSelectHandler;

	DebugGui_pimpl(fastdelegate::FastDelegate1<uint> GameSelectHandler);
	~DebugGui_pimpl();

	void MenuButtonClick(gui::Control *c);
	void MenuClick(gui::Control *c, uint4 Id);
	void AboutWindowClose(gui::Control *c) { m_AboutWindow.reset(); }
	void ShortcutsWindowClose(gui::Control *c) { m_ShortcutsWindow.reset(); }
	void SettingsWindowClose(gui::Control *c) { m_SettingsWindow.reset(); }
	void PropertyGridWindowClose(gui::Control *c) { c->SetVisible(false); }

	void ResolutionChanged();
	// Generuje prostok�t dla okna na �rodku ekranu
	RECTF MakeCenterWindowRect(float CX, float CY);
	void TogglePropertyGrid(uint4 Index, uint4 Height);
};

DebugGui_pimpl::DebugGui_pimpl(fastdelegate::FastDelegate1<uint> GameSelectHandler) :
	m_ShowStats(INITIAL_SHOW_STATS),
	m_ShowTextures(INITIAL_SHOW_TEXTURES),
	m_GameSelectHandler(GameSelectHandler)
{
	m_MenuButton.reset(new MenuButton(NULL, RECTF::ZERO));
	m_MenuButton->OnClick.bind(this, &DebugGui_pimpl::MenuButtonClick);
	m_MenuButton->SetLayer(gui::LAYER_STAYONTOP);

	m_PropertyGridWindows[0].reset(new gui::PropertyGridWindow(RECTF::ZERO, "Ustawienia g��wne"));
/*	m_PropertyGridWindows[1].reset(new gui::PropertyGridWindow(RECTF::ZERO, "Postprocessing"));
	m_PropertyGridWindows[1].reset(new gui::PropertyGridWindow(RECTF::ZERO, "�wiat�o i cie�"));
	m_PropertyGridWindows[2].reset(new gui::PropertyGridWindow(RECTF::ZERO, "Materia�"));
	m_PropertyGridWindows[3].reset(new gui::PropertyGridWindow(RECTF::ZERO, "Mg�a"));*/

	for (size_t i = 0; i < PROPERTY_GRID_WINDOW_COUNT; i++)
	{
		m_PropertyGridWindows[i]->SetVisible(false);
		m_PropertyGridWindows[i]->OnClose.bind(this, &DebugGui_pimpl::PropertyGridWindowClose);
	}

	m_Menu.reset(new gui::Menu());
	m_Menu->AddItem(gui::Menu::ITEM(1,    "Statystyki", "F1"));
	m_Menu->AddItem(gui::Menu::ITEM(101,  "Tekstury pomocnicze", "Ctrl+F1"));
	m_Menu->AddItem(gui::Menu::ITEM(2,    "Ustawienia graficzne", "F2"));
	m_Menu->AddItem(gui::Menu::ITEM(3,    "Ustawienia g��wne", "F3"));
/*	m_Menu->AddItem(gui::Menu::ITEM(4,    "Postprocessing", "F4"));
	m_Menu->AddItem(gui::Menu::ITEM(4,    "�wiat�o i cie�", "F4"));
	m_Menu->AddItem(gui::Menu::ITEM(5,    "Materia�", "F5"));
	m_Menu->AddItem(gui::Menu::ITEM(6,    "Mg�a", "F6"));*/
	m_Menu->AddItem(gui::Menu::SEPARATOR);
	m_Menu->AddItem(gui::Menu::ITEM(2001, "Gra 1 (Pacman)", "1"));
	m_Menu->AddItem(gui::Menu::ITEM(2002, "Gra 2 (RTS)", "2"));
	m_Menu->AddItem(gui::Menu::ITEM(2003, "Gra 3 (Space)", "3"));
	m_Menu->AddItem(gui::Menu::ITEM(2004, "Gra 4 (FPP)", "4"));
	m_Menu->AddItem(gui::Menu::ITEM(2005, "Gra 5 (RPG)", "5"));
	m_Menu->AddItem(gui::Menu::SEPARATOR);
	m_Menu->AddItem(gui::Menu::ITEM(1001, "Skr�ty klawiszowe"));
	m_Menu->AddItem(gui::Menu::ITEM(1002, "O programie"));
	m_Menu->AddItem(gui::Menu::ITEM(1003, "Wyj�cie", "ESC"));
	m_Menu->OnClick.bind(this, &DebugGui_pimpl::MenuClick);

	ResolutionChanged();
}

DebugGui_pimpl::~DebugGui_pimpl()
{
}

void DebugGui_pimpl::MenuButtonClick(gui::Control *c)
{
	VEC2 Pos; m_MenuButton->GetAbsolutePos(&Pos);
	Pos.y += m_MenuButton->GetRect().Height();
	m_Menu->PopupMenu(Pos);
}

void DebugGui_pimpl::MenuClick(gui::Control *c, uint4 Id)
{
	switch (Id)
	{
	case 1:
		m_ShowStats = !m_ShowStats;
		break;
	case 101:
		m_ShowTextures = !m_ShowTextures;
		break;
	case 2:
		if (m_SettingsWindow != NULL)
			m_SettingsWindow.reset();
		else
		{
			m_SettingsWindow.reset(new SettingsWindow(MakeCenterWindowRect(SETTINGS_WINDOW_CX, SETTINGS_WINDOW_CY)));
			m_SettingsWindow->OnClose.bind(this, &DebugGui_pimpl::SettingsWindowClose);
		}
		break;
	case 3:
		TogglePropertyGrid(0, 4);
		break;
/*	case 4:
		TogglePropertyGrid(1, 11);
		break;
	case 5:
		TogglePropertyGrid(2, 12);
		break;
	case 6:
		TogglePropertyGrid(3, 3);
		break;
	case 7:
		TogglePropertyGrid(4, 7);
		break;*/
	case 1001:
		if (m_ShortcutsWindow != NULL)
			m_ShortcutsWindow.reset();
		else
		{
			m_ShortcutsWindow.reset(new TextWindow(MakeCenterWindowRect(300.f, 300.f), "Skr�ty klawiszowe", SHORTCUTS_TEXT));
			m_ShortcutsWindow->OnClose.bind(this, &DebugGui_pimpl::ShortcutsWindowClose);
		}
		break;
	case 1002:
		if (m_AboutWindow != NULL)
			m_AboutWindow.reset();
		else
		{
			m_AboutWindow.reset(new TextWindow(MakeCenterWindowRect(300.f, 300.f), "O programie", ABOUT_TEXT));
			m_AboutWindow->OnClose.bind(this, &DebugGui_pimpl::AboutWindowClose);
		}
		break;
	case 1003:
		frame::Exit();
		break;
	}

	if (Id >= 2000 && Id < 3000 && m_GameSelectHandler != NULL)
		m_GameSelectHandler(Id - 2000);
}

void DebugGui_pimpl::ResolutionChanged()
{
	const float MENU_BUTTON_HALFSIZE = 20.f;
	float CenterX = frame::GetScreenWidth() / 2.f;
	m_MenuButton->SetRect(RECTF(CenterX-MENU_BUTTON_HALFSIZE, gui::MARGIN, CenterX+MENU_BUTTON_HALFSIZE, MENU_BUTTON_HALFSIZE*2.f));
}

RECTF DebugGui_pimpl::MakeCenterWindowRect(float CX, float CY)
{
	RECTF R;
	R.left = R.right = frame::GetScreenWidth() * 0.5f;
	R.top = R.bottom = frame::GetScreenHeight() * 0.5f;

	CX *= 0.5f;
	CY *= 0.5f;

	R.left -= CX; R.right += CX;
	R.top -= CY; R.bottom += CY;

	return R;
}

void DebugGui_pimpl::TogglePropertyGrid(uint4 Index, uint4 Height)
{
	const float CX = 450.f;
	const float CY = Height * (gui::CONTROL_HEIGHT+gui::MARGIN) + 32.f;

	if (m_PropertyGridWindows[Index]->GetVisible())
		m_PropertyGridWindows[Index]->SetVisible(false);
	else
	{
		m_PropertyGridWindows[Index]->SetRect(MakeCenterWindowRect(CX, CY));
		m_PropertyGridWindows[Index]->SetVisible(true);
		m_PropertyGridWindows[Index]->BringToFront();
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa DebugGui

DebugGui::DebugGui(fastdelegate::FastDelegate1<uint> GameSelectHandler) :
	pimpl(new DebugGui_pimpl(GameSelectHandler))
{
}

DebugGui::~DebugGui()
{
}

void DebugGui::ResolutionChanged()
{
	pimpl->ResolutionChanged();
}

bool DebugGui::Shortcut(uint4 Key)
{
	switch (Key)
	{
	case VK_F1:
		if (frame::GetKeyboardKey(VK_CONTROL))
			pimpl->MenuClick(NULL, 101);
		else
			pimpl->MenuClick(NULL, 1);
		return true;
	case VK_F2:
		pimpl->MenuClick(NULL, 2);
		return true;
	case VK_F3:
		pimpl->MenuClick(NULL, 3);
		return true;
	}

	return false;
}

void DebugGui::HideMenu()
{
	pimpl->m_Menu->CloseMenu();
}

bool DebugGui::GetShowStats()
{
	return pimpl->m_ShowStats;
}

bool DebugGui::GetShowTextures()
{
	return pimpl->m_ShowTextures;
}

void DebugGui::DisplaySettingsChanged(bool Succeeded)
{
	if (pimpl->m_SettingsWindow != NULL)
		pimpl->m_SettingsWindow->DisplaySettingsChanged(Succeeded);
}

gui::PropertyGridWindow * DebugGui::GetPropertyGridWindow(uint4 Id)
{
	return pimpl->m_PropertyGridWindows[Id].get();
}

DebugGui *g_DebugGui = NULL;

/*
Identyfikatory Property Grid�w:
- 0 - Ustawienia g��wne
- 1 - �wiat�o i cie�
- 2 - Materia�
- 3 - Mg�a
*/