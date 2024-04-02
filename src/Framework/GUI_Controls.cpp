/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "Gfx2D.hpp"
#include "GUI_Controls.hpp"

namespace gui
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Sta³e i zmienne

DragData *g_StandardCursors[CT_COUNT];

res::Font *g_Font = 0;
gfx2d::Sprite *g_Sprite = 0;

const float g_FontSize = 14.0f;
const COLOR g_FontColor = COLOR::WHITE;
const COLOR g_HighlightFontColor = COLOR(0xFFbceaff);
const COLOR g_InfoFontColor = COLOR(0xFF7d9ab7); // A to bêdzie do hintów: 0xFFf4e886
const COLOR g_SelBkgColor = COLOR(0xFF85a0e9);
const COLOR g_SelFontColor = g_FontColor;
const COLOR g_SelInfoFontColor = g_InfoFontColor;
const COLOR g_HeaderFontColor = g_FontColor;
const COLOR g_FocusColor = COLOR::WHITE;
const COLOR g_BkgColor = COLOR(0xFF3d70a4);
const COLOR g_BkgColor2 = COLOR(0xFF4d80b4);
const COLOR g_HintBkgColor = COLOR(0xFFffffe1);
const COLOR g_HintFontColor = COLOR::BLACK;

// Okres migania kursora
const float CURSOR_FLASH_PERIOD = 1.0f;
const COLOR CURSOR_COLOR = g_FontColor;

// Margines wewnêtrzny zak³adki w TabControl - tak pionowy jak i poziomy
const float TAB_MARGIN = 4.0f;
// Margines wewnêtrzny treœci zak³adki w TabControl
const float TAB_CONTENT_MARGIN = 2.0f;
// Margines wewnêtrzny kontrolki TabControl dla tabów
const float TAB_CONTROL_MARGIN = 2.0f;

// Margines wewnêtrzny menu
const float MENU_MARGIN = 4.0f;
// Margines wewnêtrzny pozycji menu
const float MENU_ITEM_MARGIN = 4.0f;
// Wysokoœæ separatora menu
const float MENU_SEPARATOR_HEIGHT = 2.0f;
// Odstêp miêdzy tekstem a info-tekstem w menu na szerokoœæ
const float MENU_INFO_SPACE = 4.0f;
// Minimalna szerokoœæ menu
const float MENU_MIN_WIDTH = 32.0f;
// Przezroczystoœæ menu
const float MENU_ALPHA = 0.9f;

const float WINDOW_MARGIN_1 = 4.0f;
const float WINDOW_MARGIN_2 = 1.0f;
const float WINDOW_MARGIN_3 = 2.0f;
const float WINDOW_TITLE_HEIGHT = g_FontSize + 2.0f*1.0f;
const float WINDOW_CORNER_SIZE = 16.0f;
const float WINDOW_MIN_WIDTH = 128.0f;
const float WINDOW_MIN_HEIGHT = 64.0f;

const float EDIT_MARGIN = 2.0f;
const float EDIT_PASSWORD_DOT_SIZE = 8.0f;
const float EDIT_PASSWORD_DOT_SPACE = 0.0f;
const float EDIT_CURSOR_WIDTH = 2.0f;
const size_t EDIT_UNDO_LIMIT = 64;

const float COMBOBOX_BUTTON_WIDTH = 24.0f;

const float CHECKBOX_BOX_SIZE = 16.0f;
const float RADIOBUTTON_BOX_SIZE = 16.0f;


// Oblicza miejsce lewego górnego rogu dla wyskakuj¹cego czegoœ, co powinno pokazaæ siê w DesiredPos,
// ale szczególnie powinno zmieœciæ siê ca³e na ekranie
void CalculatePopupPos(VEC2 *Out, const VEC2 &DesiredPos, float PopupWidth, float PopupHeight)
{
	if (DesiredPos.x + PopupWidth <= frame::GetScreenWidth())
		Out->x = DesiredPos.x;
	else if (DesiredPos.x - PopupWidth >= 0.0f)
		Out->x = DesiredPos.x - PopupWidth;
	else
		Out->x = frame::GetScreenWidth() - PopupWidth;

	if (DesiredPos.y + PopupHeight <= frame::GetScreenHeight())
		Out->y = DesiredPos.y;
	else if (DesiredPos.y - PopupHeight >= 0.0f)
		Out->y = DesiredPos.y - PopupHeight;
	else
		Out->y = frame::GetScreenHeight() - PopupHeight;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Obs³uga schowka - kod wewnêtrzny

// Nie rzuca wyj¹tków - zwraca false jeœli siê coœ nie uda.
namespace SystemClipboard
{

bool Clear()
{
	bool R;

	R = OpenClipboard(0) != FALSE;
	if (R)
	{
		R = EmptyClipboard() != FALSE;
		CloseClipboard();
	}
	return R;
}

bool SetText(const string &Text)
{
	bool R;

	R = (OpenClipboard(0) != FALSE);
	if (R)
	{
		R = (EmptyClipboard() != FALSE);
		if (R)
		{
			HGLOBAL Mem = GlobalAlloc(GMEM_MOVEABLE, Text.length()+1);
			R = (Mem != 0);
			if (R)
			{
				void *MemPtr = GlobalLock(Mem);
				R = (MemPtr != 0);
				if (R)
				{
					memcpy(MemPtr, Text.c_str(), Text.length()+1);
					GlobalUnlock(Mem);
					R = (SetClipboardData(CF_TEXT, Mem) != 0);
				}
				if (!R)
					GlobalFree(Mem);
			}
		}
		CloseClipboard();
	}
	return R;
}

bool GetText(string *Out)
{
	bool R;

	R = (OpenClipboard(0) != FALSE);
	if (R)
	{
		if (IsClipboardFormatAvailable(CF_TEXT) || IsClipboardFormatAvailable(CF_OEMTEXT))
		{
			HANDLE Mem = GetClipboardData(CF_TEXT);
			R = (Mem != 0);
			if (R)
			{
				char *MemPtr = (char*)GlobalLock(Mem);
				R = (MemPtr != 0);
				if (R)
				{
					*Out = MemPtr;
					GlobalUnlock(Mem);
				}
			}
		}
		else
			R = false;
		CloseClipboard();
	}
	return R;
}

bool GetFirstLineText(string *Out)
{
	string Text;
	if (GetText(&Text))
	{
		size_t Len;
		for (Len = 0; Len < Text.length(); Len++)
		{
			if (Text[Len] == '\n' || Text[Len] == '\r')
				break;
		}
		*Out = Text.substr(0, Len);
		return true;
	}
	else return false;
}

} // namespace SystemClipboard


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Label

Label::Label(CompositeControl *Parent, const RECTF &Rect, const string &Text, uint4 TextFlags) :
	Control(Parent, false, Rect),
	m_Text(Text),
	m_TextFlags(TextFlags)
{
}

void Label::OnDraw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->SetColor(g_FontColor);
	gfx2d::g_Canvas->SetFont(g_Font);

	float X, Y;

	if (m_TextFlags & res::Font::FLAG_HLEFT)
		X = 0.0f;
	else if (m_TextFlags & res::Font::FLAG_HRIGHT)
		X = GetLocalRect().right;
	else
		X = GetLocalRect().GetCenterX();

	if (m_TextFlags & res::Font::FLAG_VTOP)
		Y = 0.0f;
	else if (m_TextFlags & res::Font::FLAG_VBOTTOM)
		Y = GetLocalRect().bottom;
	else
		Y = GetLocalRect().GetCenterY();

	gfx2d::g_Canvas->DrawText_(X, Y, m_Text, g_FontSize, m_TextFlags, GetRect().Width());
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa AbstractButton

AbstractButton::AbstractButton(CompositeControl *Parent, const RECTF &Rect, bool Focusable) :
	Control(Parent, Focusable, Rect),
	m_Down(false)
{
}

void AbstractButton::DoClick()
{
	if (OnClick)
		OnClick(this);
}

void AbstractButton::Click()
{
	if (GetRealEnabled())
		DoClick();
}

void AbstractButton::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Button == frame::MOUSE_LEFT)
	{
		if (Action == frame::MOUSE_DOWN || Action == frame::MOUSE_DBLCLK)
		{
			m_Down = true;
			StartDrag();
		}
		else // Action == frame::MOUSE_UP
		{
			if (m_Down)
			{
				if (IsMouseOver())
					Click();
				m_Down = false;
			}
		}
	}
}

bool AbstractButton::OnKeyDown(uint4 Key)
{
	if (Key == VK_RETURN)
		Click();
	else if (Key == VK_SPACE)
		m_Down = true;
	else
		return Control::OnKeyDown(Key);

	return true;
}

bool AbstractButton::OnKeyUp(uint4 Key)
{
	if (Key == VK_RETURN)
	{
	}
	else if (Key == VK_SPACE)
	{
		if (m_Down)
		{
			Click();
			m_Down = false;
		}
	}
	else
		return Control::OnKeyUp(Key);

	return true;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Button

void Button::OnDraw(const VEC2 &Translation)
{

	// T³o
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetColor(GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY);
	gfx2d::g_Canvas->DrawRect(GetLocalRect(), gfx2d::ComplexElement( IsDown() ? 3 : 1 ));

	// Focus
	if (IsFocused())
	{
		RECTF FocusRect = GetLocalRect();
		FocusRect.Extend(-3.0f);
		gfx2d::g_Canvas->SetColor(g_FocusColor);
		gfx2d::g_Canvas->DrawRect(FocusRect, gfx2d::ComplexElement(2));
	}

	// Napis
	if (IsDown())
		gfx2d::g_Canvas->PushTranslation(VEC2(2.0f, 2.0f));
	gfx2d::g_Canvas->SetColor(GetRealEnabled() && IsMouseOver() ? g_HighlightFontColor : g_FontColor);
	gfx2d::g_Canvas->SetFont(g_Font);
	gfx2d::g_Canvas->DrawText_(
		GetRect().Width()*0.5f, GetRect().Height()*0.5f,
		m_Text, g_FontSize,
		res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE,
		GetRect().Width());
	if (IsDown())
		gfx2d::g_Canvas->PopTranslation();
}

Button::Button(CompositeControl *Parent, const RECTF &Rect, const string &Text, bool Focusable) :
	AbstractButton(Parent, Rect, Focusable),
	m_Text(Text)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa SpriteButton

void SpriteButton::OnDraw(const VEC2 &Translation)
{

	// T³o
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetColor(GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY);
	gfx2d::g_Canvas->DrawRect(GetLocalRect(), gfx2d::ComplexElement( IsDown() ? 3 : 1 ));

	// Focus
	if (IsFocused())
	{
		RECTF FocusRect = GetLocalRect();
		FocusRect.Extend(-6.0f);
		gfx2d::g_Canvas->SetColor(g_FocusColor);
		gfx2d::g_Canvas->DrawRect(FocusRect, gfx2d::ComplexElement(2));
	}

	// Sprite
	if (IsDown())
		gfx2d::g_Canvas->PushTranslation(VEC2(2.0f, 2.0f));
	gfx2d::g_Canvas->SetColor(IsMouseOver() && GetRealEnabled() ? g_HighlightFontColor : g_FontColor);
	RECTF R = GetLocalRect();
	R.left = R.right = R.GetCenterX(); R.top = R.bottom = R.GetCenterY();
	R.left -= m_SpriteWidth*0.5f; R.right += m_SpriteWidth*0.5f;
	R.top -= m_SpriteHeight*0.5f; R.bottom += m_SpriteHeight*0.5f;
	gfx2d::g_Canvas->DrawRect(R, m_SpriteIndex);
	if (IsDown())
		gfx2d::g_Canvas->PopTranslation();
}

SpriteButton::SpriteButton(CompositeControl *Parent, const RECTF &Rect, bool Focusable) :
	AbstractButton(Parent, Rect, Focusable),
	m_SpriteIndex(0),
	m_SpriteWidth(0.0f), m_SpriteHeight(0.0f)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa AbstractCheckBox

void AbstractCheckBox::DoClick()
{
	if (m_State == CHECKED)
		m_State = UNCHECKED;
	else
		m_State = CHECKED;

	AbstractButton::DoClick();
}

AbstractCheckBox::AbstractCheckBox(CompositeControl *Parent, const RECTF &Rect, STATE InitialState, bool Focusable) :
	AbstractButton(Parent, Rect, Focusable),
	m_State(InitialState)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa CheckBox

void CheckBox::OnDraw(const VEC2 &Translation)
{

	// Kwadracik
	float CenterY = GetLocalRect().GetCenterY();
	RECTF SquareRect = RECTF(0.0f, CenterY-CHECKBOX_BOX_SIZE*0.5f, CHECKBOX_BOX_SIZE, CenterY+CHECKBOX_BOX_SIZE*0.5f);
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetColor(GetRealEnabled() && !IsDown() ? (GetState() == GRAYED ? COLOR::SILVER : COLOR::WHITE) : COLOR::GRAY);
	gfx2d::g_Canvas->DrawRect(SquareRect, gfx2d::ComplexElement(3));
	if (GetState() == CHECKED)
	{
		RECTF CheckRect = SquareRect;
		CheckRect.Extend(-4.0f);
		gfx2d::g_Canvas->SetColor(GetRealEnabled() && IsMouseOver() ? g_HighlightFontColor : g_FontColor);
		gfx2d::g_Canvas->DrawRect(CheckRect, 43);
	}

	// Focus
	RECTF TextRect = RECTF(CHECKBOX_BOX_SIZE+4.0f, 0.0f, GetLocalRect().right, SquareRect.bottom);
	if (IsFocused())
	{
		gfx2d::g_Canvas->SetColor(g_FocusColor);
		gfx2d::g_Canvas->DrawRect(TextRect, gfx2d::ComplexElement(2));
	}

	// Napis
	TextRect.left += 4.0f;
	gfx2d::g_Canvas->SetColor(GetRealEnabled() && IsMouseOver() ? g_HighlightFontColor : g_FontColor);
	gfx2d::g_Canvas->SetFont(g_Font);
	gfx2d::g_Canvas->DrawText_(
		TextRect.left, TextRect.GetCenterY(),
		m_Text, g_FontSize,
		res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE,
		TextRect.Width());
}

CheckBox::CheckBox(CompositeControl *Parent, const RECTF &Rect, STATE InitialState, const string &Text, bool Focusable) :
	AbstractCheckBox(Parent, Rect, InitialState, Focusable),
	m_Text(Text)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa AbstractRadioButton

AbstractRadioButton::GroupDatabase *AbstractRadioButton::sm_Groups = 0;
int AbstractRadioButton::sm_GroupsRefCount = 0;

class AbstractRadioButton::GroupDatabase
{
private:
	typedef std::set<AbstractRadioButton*> RADIOBUTTON_SET;
	typedef std::map<uint4, RADIOBUTTON_SET> GROUP_MAP;

	GROUP_MAP m_Groups;

public:
	void Add(uint4 Group, AbstractRadioButton *RadioButton);
	void Remove(uint4 Group, AbstractRadioButton *RadioButton);
	void UncheckGroup(uint4 Group, AbstractRadioButton *Exception);
};

void AbstractRadioButton::GroupDatabase::Add(uint4 Group, AbstractRadioButton *RadioButton)
{
	GROUP_MAP::iterator mit = m_Groups.find(Group);

	if (mit == m_Groups.end())
		mit = m_Groups.insert(std::make_pair(Group, RADIOBUTTON_SET())).first;

	mit->second.insert(RadioButton);
}

void AbstractRadioButton::GroupDatabase::Remove(uint4 Group, AbstractRadioButton *RadioButton)
{
	GROUP_MAP::iterator mit = m_Groups.find(Group);
	assert(mit != m_Groups.end());

	mit->second.erase(RadioButton);

	if (mit->second.empty())
		m_Groups.erase(mit);
}

void AbstractRadioButton::GroupDatabase::UncheckGroup(uint4 Group, AbstractRadioButton *Exception)
{
	GROUP_MAP::iterator mit = m_Groups.find(Group);
	if (mit == m_Groups.end())
		return;
	for (RADIOBUTTON_SET::iterator sit = mit->second.begin(); sit != mit->second.end(); ++sit)
	{
		if (*sit != Exception)
			(*sit)->SetChecked(false);
	}
}

void AbstractRadioButton::DoClick()
{
	if (!m_Checked)
	{
		sm_Groups->UncheckGroup(m_Group, this);
		m_Checked = true;
		if (OnCheckChange)
			OnCheckChange(this);
	}

	AbstractButton::DoClick();
}

AbstractRadioButton::AbstractRadioButton(CompositeControl *Parent, const RECTF &Rect, uint4 Group, bool InitialChecked, bool Focusable) :
	AbstractButton(Parent, Rect, Focusable),
	m_Group(Group),
	m_Checked(InitialChecked)
{
	if (sm_GroupsRefCount == 0)
		sm_Groups = new GroupDatabase;
	sm_GroupsRefCount++;

	sm_Groups->Add(Group, this);

	if (InitialChecked)
		sm_Groups->UncheckGroup(m_Group, this);
}

AbstractRadioButton::~AbstractRadioButton()
{
	sm_Groups->Remove(m_Group, this);

	sm_GroupsRefCount--;
	if (sm_GroupsRefCount == 0)
		SAFE_DELETE(sm_Groups);
}

void AbstractRadioButton::SetChecked(bool NewChecked)
{
	if (NewChecked != m_Checked)
	{
		if (NewChecked)
		{
			sm_Groups->UncheckGroup(m_Group, this);
		}
		m_Checked = NewChecked;
		if (OnCheckChange)
			OnCheckChange(this);
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa RadioButton

void RadioButton::OnDraw(const VEC2 &Translation)
{

	// Kó³eczko
	float Y = GetLocalRect().GetCenterY();
	RECTF SquareRect = RECTF(0.0f, Y-RADIOBUTTON_BOX_SIZE*0.5f, RADIOBUTTON_BOX_SIZE, Y+RADIOBUTTON_BOX_SIZE*0.5f);
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetColor(GetRealEnabled() && !IsDown() ? COLOR::WHITE : COLOR::GRAY);
	gfx2d::g_Canvas->DrawRect(SquareRect, 42);
	if (IsChecked())
	{
		gfx2d::g_Canvas->SetColor(GetRealEnabled() && IsMouseOver() ? g_HighlightFontColor : g_FontColor);
		gfx2d::g_Canvas->DrawRect(SquareRect, 44);
	}

	// Focus
	RECTF TextRect = RECTF(RADIOBUTTON_BOX_SIZE+4.0f, 0.0f, GetLocalRect().right, SquareRect.bottom);
	if (IsFocused())
	{
		gfx2d::g_Canvas->SetColor(g_FocusColor);
		gfx2d::g_Canvas->DrawRect(TextRect, gfx2d::ComplexElement(2));
	}

	// Napis
	TextRect.left += 4.0f;
	gfx2d::g_Canvas->SetColor(GetRealEnabled() && IsMouseOver() ? g_HighlightFontColor : g_FontColor);
	gfx2d::g_Canvas->SetFont(g_Font);
	gfx2d::g_Canvas->DrawText_(
		TextRect.left, TextRect.GetCenterY(),
		m_Text, g_FontSize,
		res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE,
		TextRect.Width());
}

RadioButton::RadioButton(CompositeControl *Parent, const RECTF &Rect, uint4 Group, bool InitialChecked, const string &Text, bool Focusable) :
	AbstractRadioButton(Parent, Rect, Group, InitialChecked, Focusable),
	m_Text(Text)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa AbstractTrackBar

float AbstractTrackBar::GetPercentValue()
{
	if (m_FloatValue)
		return ValueToPercentF(m_ValueF);
	else
		return ValueToPercentI(m_ValueI);
}

int AbstractTrackBar::PercentToValueI(float Percent)
{
	return round(Percent*(m_MaxI-m_MinI)) + m_MinI;
}

float AbstractTrackBar::PercentToValueF(float Percent)
{
	return Percent*(m_MaxF-m_MinF) + m_MinF;
}

float AbstractTrackBar::ValueToPercentI(int Value)
{
	if (m_MinI == m_MaxI)
		return 0.0f;
	return static_cast<float>(Value-m_MinI) / static_cast<float>(m_MaxI-m_MinI);
}

float AbstractTrackBar::ValueToPercentF(float Value)
{
	if (m_MinF == m_MaxF)
		return 0.0f;
	return (Value-m_MinF) / (m_MaxF-m_MinF);
}

float AbstractTrackBar::GetBarTrackStart()
{
	return GetBarSize() * 0.5f;
}

float AbstractTrackBar::GetBarTrackLength()
{
	if (GetOrientation() == O_HORIZ)
		return GetLocalRect().right - GetBarSize();
	else
		return GetLocalRect().bottom - GetBarSize();
}

float AbstractTrackBar::GetBarCenter()
{
	return GetBarTrackLength() * GetPercentValue() + GetBarTrackStart();
}

void AbstractTrackBar::GetBarRect(RECTF *Rect)
{
	float BarSize = GetBarSize();
	float BarCenter = GetBarCenter();
	if (GetOrientation() == O_HORIZ)
	{
		Rect->top = 0.0f;
		Rect->bottom = GetLocalRect().bottom;
		Rect->left = BarCenter - BarSize*0.5f;
		Rect->right = BarCenter + BarSize*0.5f;
	}
	else
	{
		Rect->left = 0.0f;
		Rect->right = GetLocalRect().right;
		Rect->top = BarCenter - BarSize*0.5f;
		Rect->bottom = BarCenter + BarSize*0.5f;
	}
}

void AbstractTrackBar::HandleDrag(const VEC2 &Pos)
{
	float BarTrackLength = GetBarTrackLength();
	if (FLOAT_ALMOST_ZERO(BarTrackLength))
		return;

	float NewPercent;
	if (GetOrientation() == O_HORIZ)
		NewPercent = minmax(0.0f, (Pos.x - m_GrabOffset - GetBarTrackStart()) / BarTrackLength, 1.0f);
	else
		NewPercent = minmax(0.0f, (Pos.y - m_GrabOffset - GetBarTrackStart()) / BarTrackLength, 1.0f);

	if (IsFloatValue())
	{
		float NewValue = PercentToValueF(NewPercent);
		if (NewValue != GetValueF())
		{
			SetValueF(NewValue);
			if (OnValueChange)
				OnValueChange(this);
		}
	}
	else
	{
		int NewValue = PercentToValueI(NewPercent);
		if (NewValue != GetValueI())
		{
			SetValueI(NewValue);
			if (OnValueChange)
				OnValueChange(this);
		}
	}
}

void AbstractTrackBar::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Button == frame::MOUSE_LEFT)
	{
		RECTF BarRect; GetBarRect(&BarRect);

		if (Action == frame::MOUSE_DOWN || Action == frame::MOUSE_DBLCLK)
		{
			if (GetOrientation() == O_HORIZ)
			{
				if (Pos.x >= BarRect.left && Pos.x <= BarRect.right)
				{
					m_Down = true;
					m_GrabOffset = Pos.x - BarRect.GetCenterX();
				}
			}
			else
			{
				if (Pos.y >= BarRect.top && Pos.y <= BarRect.bottom)
				{
					m_Down = true;
					m_GrabOffset = Pos.y - BarRect.GetCenterY();
				}
			}
			StartDrag();
		}
		else if (Action == frame::MOUSE_UP)
		{
			if (m_Down)
			{
				HandleDrag(Pos);
				m_Down = false;
			}
		}
	}
}

void AbstractTrackBar::OnMouseMove(const VEC2 &Pos)
{
	if (m_Down)
		HandleDrag(Pos);
}

bool AbstractTrackBar::OnKeyDown(uint4 Key)
{
	switch (Key)
	{
	case VK_LEFT:
		if (m_Orientation == O_HORIZ)
		{
			if (m_FloatValue)
			{
				if (m_ValueF > m_MinF)
				{
					m_ValueF = std::max(m_MinF, m_ValueF-m_StepF);
					if (OnValueChange)
						OnValueChange(this);
				}
			}
			else
			{
				if (m_ValueI > m_MinI)
				{
					m_ValueI = std::max(m_MinI, m_ValueI-m_StepI);
					if (OnValueChange)
						OnValueChange(this);
				}
			}
			return true;
		}
		break;
	case VK_RIGHT:
		if (m_Orientation == O_HORIZ)
		{
			if (m_FloatValue)
			{
				if (m_ValueF < m_MaxF)
				{
					m_ValueF = std::min(m_MaxF, m_ValueF+m_StepF);
					if (OnValueChange)
						OnValueChange(this);
				}
			}
			else
			{
				if (m_ValueI < m_MaxI)
				{
					m_ValueI = std::min(m_MaxI, m_ValueI+m_StepI);
					if (OnValueChange)
						OnValueChange(this);
				}
			}
			return true;
		}
		break;
	case VK_UP:
		if (m_Orientation == O_VERT)
		{
			if (m_FloatValue)
			{
				if (m_ValueF > m_MinF)
				{
					m_ValueF = std::max(m_MinF, m_ValueF-m_StepF);
					if (OnValueChange)
						OnValueChange(this);
				}
			}
			else
			{
				if (m_ValueI > m_MinI)
				{
					m_ValueI = std::max(m_MinI, m_ValueI-m_StepI);
					if (OnValueChange)
						OnValueChange(this);
				}
			}
			return true;
		}
		break;
	case VK_DOWN:
		if (m_Orientation == O_VERT)
		{
			if (m_FloatValue)
			{
				if (m_ValueF < m_MaxF)
				{
					m_ValueF = std::min(m_MaxF, m_ValueF+m_StepF);
					if (OnValueChange)
						OnValueChange(this);
				}
			}
			else
			{
				if (m_ValueI < m_MaxI)
				{
					m_ValueI = std::min(m_MaxI, m_ValueI+m_StepI);
					if (OnValueChange)
						OnValueChange(this);
				}
			}
			return true;
		}
		break;
	case VK_HOME:
		if (m_FloatValue)
		{
			if (m_ValueF > m_MinF)
			{
				m_ValueF = m_MinF;
				if (OnValueChange)
					OnValueChange(this);
			}
		}
		else
		{
			if (m_ValueI > m_MinI)
			{
				m_ValueI = m_MinI;
				if (OnValueChange)
					OnValueChange(this);
			}
		}
		return true;
	case VK_END:
		if (m_FloatValue)
		{
			if (m_ValueF < m_MaxF)
			{
				m_ValueF = m_MaxF;
				if (OnValueChange)
					OnValueChange(this);
			}
		}
		else
		{
			if (m_ValueI < m_MaxI)
			{
				m_ValueI = m_MaxI;
				if (OnValueChange)
					OnValueChange(this);
			}
		}
		return true;
	}

	return Control::OnKeyDown(Key);
}

AbstractTrackBar::AbstractTrackBar(CompositeControl *Parent, const RECTF &Rect, ORIENTATION Orientation, bool FloatValue, bool Focusable) :
	Control(Parent, Focusable, Rect),
	m_Orientation(Orientation),
	m_FloatValue(FloatValue),
	m_MinI(0), m_MaxI(0), m_StepI(0), m_ValueI(0),
	m_MinF(0.0f), m_MaxF(0.0f), m_StepF(0.0f), m_ValueF(0.0f),
	m_Down(false)
{
}

void AbstractTrackBar::SetRangeI(int Min, int Max, int Step)
{
	m_MinI = Min;
	m_MaxI = Max;
	m_StepI = Step;
	if (m_ValueI < m_MinI)
		m_ValueI = m_MinI;
	if (m_ValueI > m_MaxI)
		m_ValueI = m_MaxI;
}

void AbstractTrackBar::SetValueI(int Value)
{
	m_ValueI = minmax(m_MinI, Value, m_MaxI);
}

void AbstractTrackBar::SetRangeF(float Min, float Max, float Step)
{
	m_MinF = Min;
	m_MaxF = Max;
	m_StepF = Step;
	if (m_ValueF < m_MinF)
		m_ValueF = m_MinF;
	if (m_ValueF > m_MaxF)
		m_ValueF = m_MaxF;
}

void AbstractTrackBar::SetValueF(float Value)
{
	m_ValueF = minmax(m_MinF, Value, m_MaxF);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TrackBar

void TrackBar::OnDraw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetColor(GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY);

	// Pasek w œrodku
	RECTF BkgStrip;
	if (GetOrientation() == O_HORIZ)
	{
		BkgStrip.left = 0.0f;
		BkgStrip.right = GetLocalRect().right;
		BkgStrip.top = BkgStrip.bottom = GetLocalRect().GetCenterY();
		BkgStrip.top -= 2.0f;
		BkgStrip.bottom += 2.0f;
	}
	else
	{
		BkgStrip.top = 0.0f;
		BkgStrip.bottom = GetLocalRect().bottom;
		BkgStrip.left = BkgStrip.right = GetLocalRect().GetCenterX();
		BkgStrip.left -= 2.0f;
		BkgStrip.right += 2.0f;
	}
	gfx2d::g_Canvas->DrawRect(BkgStrip, GetOrientation() == O_HORIZ ? 31 : 33);

	// Bar
	RECTF BarRect; GetBarRect(&BarRect);
	gfx2d::g_Canvas->DrawRect(BarRect, GetOrientation() == O_HORIZ ? 40 : 41);

	// Focus
	if (IsFocused())
	{
		gfx2d::g_Canvas->SetColor(g_FocusColor);
		gfx2d::g_Canvas->DrawRect(GetLocalRect(), gfx2d::ComplexElement(2));
	}
}

TrackBar::TrackBar(CompositeControl *Parent, const RECTF &Rect, ORIENTATION Orientation, bool FloatValue, bool Focusable) :
	AbstractTrackBar(Parent, Rect, Orientation, FloatValue, Focusable)
{
}

float TrackBar::GetBarSize()
{
	if (GetOrientation() == O_HORIZ)
		return GetLocalRect().bottom * 5.0f / 14.0f;
	else
		return GetLocalRect().right * 5.0f / 14.0f;
}

	
//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa AbstractScrollBar

void AbstractScrollBar::GetRect_Button1(RECTF *Out)
{
	if (m_Orientation == O_HORIZ)
		*Out = RECTF(0.0f, 0.0f, GetButtonSize(), GetLocalRect().bottom);
	else
		*Out = RECTF(0.0f, 0.0f, GetLocalRect().right, GetButtonSize());
}

void AbstractScrollBar::GetRect_Button2(RECTF *Out)
{
	const RECTF & LocalRect = GetLocalRect();

	if (m_Orientation == O_HORIZ)
		*Out = RECTF(LocalRect.right-GetButtonSize(), 0.0f, LocalRect.right, GetLocalRect().bottom);
	else
		*Out = RECTF(0.0f, LocalRect.bottom-GetButtonSize(), GetLocalRect().right, LocalRect.bottom);
}

void AbstractScrollBar::GetRect_TrackButton(RECTF *Out)
{
	// Mój prostok¹t
	const RECTF & LocalRect = GetLocalRect();

	// Minimalna d³ugoœæ przycisku przewijania
	// ...jest w GetMinTrackButtonSize().

	// Maksymalna d³ugoœæ przycisku przewijania
	// W sensie d³ugoœci, która zmieœci siê aktualnie.
	// Bo to jest co innego, ni¿ GetMaxTrackButtonSize() - tam jest maksymalna d³ugoœæ ogólnie.
	float MaxTrackButtonSize;
	if (m_Orientation == O_VERT)
		MaxTrackButtonSize = LocalRect.bottom - 2.0f*GetButtonSize();
	else
		MaxTrackButtonSize = LocalRect.right - 2.0f*GetButtonSize();

	// D³ugoœc przycisku przewijania
	float TrackButtonSize;
	if (m_Doc <= 1.0f)
		TrackButtonSize = MaxTrackButtonSize;
	else
	{
		TrackButtonSize = (1.0f/m_Doc) * MaxTrackButtonSize;
		TrackButtonSize = std::min(MaxTrackButtonSize, TrackButtonSize);
		TrackButtonSize = minmax(GetMinTrackButtonSize(), TrackButtonSize, GetMaxTrackButtonSize());
	}

	// Iloœæ miejsca
	float Space;
	if (m_Orientation == O_VERT)
		Space = LocalRect.bottom - 2.0f*GetButtonSize() - TrackButtonSize;
	else
		Space = LocalRect.right - 2.0f*GetButtonSize() - TrackButtonSize;

	// Koñcowy prostok¹t
	if (m_Orientation == O_VERT)
	{
		Out->left = 0.0f;
		Out->right = LocalRect.right;
		Out->top = GetButtonSize()+Space*GetPercentPos();
		Out->bottom = Out->top + TrackButtonSize;
	}
	else
	{
		Out->top = 0.0f;
		Out->bottom = LocalRect.bottom;
		Out->left = GetButtonSize()+Space*GetPercentPos();
		Out->right = Out->left + TrackButtonSize;
	}
}

void AbstractScrollBar::Space_SetPos(const VEC2 &pos)
{
	float ButtonSize = GetButtonSize();

	if (m_Orientation == O_VERT)
		SetPercentPos( (pos.y-ButtonSize) / (GetLocalRect().bottom-2.0f*ButtonSize) );
	else
		SetPercentPos( (pos.x-ButtonSize) / (GetLocalRect().right-2.0f*ButtonSize) );
}

void AbstractScrollBar::TrackButton_SetPos(const VEC2 &pos)
{
	RECTF tbr; GetRect_TrackButton(&tbr);
	VEC2 pos2 = pos - m_TrackPos;
	float ButtonSize = GetButtonSize();

	if (m_Orientation == O_VERT)
	{
		pos2.y -= ButtonSize;
		float SpaceSize = GetLocalRect().bottom - 2.0f*ButtonSize - tbr.Height();
		SetPercentPos(pos2.y/SpaceSize);
	}
	else
	{
		pos2.x -= ButtonSize;
		float SpaceSize = GetLocalRect().right - 2.0f*ButtonSize - tbr.Width();
		SetPercentPos(pos2.x/SpaceSize);
	}
}

void AbstractScrollBar::OnMouseMove(const VEC2 &Pos)
{
	if (m_State == STATE_SPACE)
	{
		Space_SetPos(Pos);
		if (OnScroll)
			OnScroll(this);
	}
	else if (m_State == STATE_TRACKBUTTON)
	{
		TrackButton_SetPos(Pos);
		if (OnScroll)
			OnScroll(this);
	}
}

void AbstractScrollBar::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Button == frame::MOUSE_LEFT)
	{
		if (Action == frame::MOUSE_DOWN || Action == frame::MOUSE_DBLCLK)
		{
			RECTF Button1Rect; GetRect_Button1(&Button1Rect);
			RECTF Button2Rect; GetRect_Button2(&Button2Rect);
			RECTF TrackButtonRect; GetRect_TrackButton(&TrackButtonRect);
			// Przycisk 1
			if (PointInRect(Pos, Button1Rect))
			{
				m_State = STATE_BUTTON1;
				SetPos(GetPos()-m_Step);
				if (OnScroll) OnScroll(this);
			}
			// Przycisk 2
			else if (PointInRect(Pos, Button2Rect))
			{
				m_State = STATE_BUTTON2;
				SetPos(GetPos()+m_Step);
				if (OnScroll) OnScroll(this);
			}
			// Przycisk przewijania
			else if (PointInRect(Pos, TrackButtonRect))
			{
				m_State = STATE_TRACKBUTTON;
				m_TrackPos.x = Pos.x - TrackButtonRect.left;
				m_TrackPos.y = Pos.y - TrackButtonRect.top;
				// To napisa³em tylko dla picu. Tego nie ma, bo jeœli chwyci³em
				// za to miejsce to przy przesuniêciu do tego w³aœnie miejsca
				// ¿adnego przewijania nie ma.
				//TrackButton_SetPos(pos);
				//if (OnScroll) OnScroll(this);
			}
			// Wolna przestrzeñ
			else
			{
				m_State = STATE_SPACE;
				Space_SetPos(Pos);
				if (OnScroll) OnScroll(this);
			}
			StartDrag();
		}
		else if (Action == frame::MOUSE_UP)
			m_State = STATE_NONE;
	}
}

void AbstractScrollBar::OnMouseWheel(const VEC2 &Pos, float Delta)
{
	SetPos(GetPos()-m_Step*Delta);
	if (OnScroll) OnScroll(this);
}

bool AbstractScrollBar::OnKeyDown(uint4 Key)
{
	switch (Key)
	{
	case VK_HOME:
        SetPos(GetMin());
		if (OnScroll) OnScroll(this);
		return true;
	case VK_END:
		SetPos(GetMax());
		if (OnScroll) OnScroll(this);
		return true;
	case VK_UP:
		if (m_Orientation == O_VERT)
		{
			SetPos(GetPos()-m_Step);
			if (OnScroll) OnScroll(this);
			return true;
		}
		break;
	case VK_DOWN:
		if (m_Orientation == O_VERT)
		{
			SetPos(GetPos()+m_Step);
			if (OnScroll) OnScroll(this);
			return true;
		}
		break;
	case VK_LEFT:
		if (m_Orientation == O_HORIZ)
		{
			SetPos(GetPos()-m_Step);
			if (OnScroll) OnScroll(this);
			return true;
		}
		break;
	case VK_RIGHT:
		if (m_Orientation == O_HORIZ)
		{
			SetPos(GetPos()+m_Step);
			if (OnScroll) OnScroll(this);
			return true;
		}
		break;
	case VK_SPACE:
		SetPos(GetPos()+m_Step);
		if (OnScroll) OnScroll(this);
		return true;
	}
	
	return Control::OnKeyDown(Key);
}

AbstractScrollBar::AbstractScrollBar(CompositeControl *Parent, const RECTF &Rect, ORIENTATION Orientation, bool Focusable) :
	Control(Parent, Focusable, Rect),
	m_Min(0.0f),
	m_Max(1.0f),
	m_Doc(1.0f),
	m_Pos(0.0f),
	m_Step(1.0f),
	m_Orientation(Orientation),
	m_State(STATE_NONE),
	m_TrackPos(VEC2::ZERO)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ScrollBar

void ScrollBar::OnDraw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->SetSprite(g_Sprite);

	COLOR WhiteColor = GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY;

	RECTF Button1Rect; GetRect_Button1(&Button1Rect);
	RECTF Button2Rect; GetRect_Button2(&Button2Rect);
	RECTF TrackButtonRect; GetRect_TrackButton(&TrackButtonRect);

	// T³o
	gfx2d::g_Canvas->SetColor(WhiteColor);
	gfx2d::g_Canvas->DrawRect(GetLocalRect(), gfx2d::ComplexElement(0));

	// Button 1
	gfx2d::g_Canvas->SetColor(WhiteColor);
	gfx2d::g_Canvas->DrawRect(Button1Rect, gfx2d::ComplexElement(GetState() == STATE_BUTTON1 ? 3 : 1));
	Button1Rect.Extend(-8.0f);
	gfx2d::g_Canvas->SetColor(GetRealEnabled() && IsMouseOver() ? g_HighlightFontColor : g_FontColor);
	gfx2d::g_Canvas->DrawRect(Button1Rect, gfx2d::ComplexElement(GetOrientation() == O_HORIZ ? 12 : 10));
	// Button 2
	gfx2d::g_Canvas->SetColor(WhiteColor);
	gfx2d::g_Canvas->DrawRect(Button2Rect, gfx2d::ComplexElement(GetState() == STATE_BUTTON2 ? 3 : 1));
	Button2Rect.Extend(-8.0f);
	gfx2d::g_Canvas->SetColor(GetRealEnabled() && IsMouseOver() ? g_HighlightFontColor : g_FontColor);
	gfx2d::g_Canvas->DrawRect(Button2Rect, gfx2d::ComplexElement(GetOrientation() == O_HORIZ ? 13 : 11));
	// Track button
	gfx2d::g_Canvas->SetColor(WhiteColor);
	gfx2d::g_Canvas->DrawRect(TrackButtonRect, gfx2d::ComplexElement(1));
	if (IsFocused())
	{
		TrackButtonRect.Extend(-8.0f);
		gfx2d::g_Canvas->SetColor(g_FocusColor);
		gfx2d::g_Canvas->DrawRect(TrackButtonRect, gfx2d::ComplexElement(2));
	}
}

ScrollBar::ScrollBar(CompositeControl *Parent, const RECTF &Rect, ORIENTATION Orientation, bool Focusable) :
	AbstractScrollBar(Parent, Rect, Orientation, Focusable)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GroupBox

void GroupBox::OnDraw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetColor(GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY);
	gfx2d::g_Canvas->DrawRect(GetLocalRect(), gfx2d::ComplexElement(3));
}

GroupBox::GroupBox(CompositeControl *Parent, const RECTF &Rect) :
	CompositeControl(Parent, false, Rect)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TabControl::Tab

void TabControl::Tab::RecalculateTabWidth()
{
	m_TabWidth = g_Font->GetTextWidth(m_Title, g_FontSize) + TAB_MARGIN*2.0f;
}

TabControl::Tab::Tab(TabControl *Parent, const RECTF &Rect, const string &Title) :
	CompositeControl(Parent, false, Rect),
	m_Title(Title)
{
	SetVisible(false);
	RecalculateTabWidth();
}

void TabControl::Tab::SetTitle(const string &NewTitle)
{
	m_Title = NewTitle;
	RecalculateTabWidth();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TabControl

float TabControl::GetTabHeight()
{
	return g_FontSize + TAB_MARGIN*2.0f;
}

void TabControl::GetTabContentRect(RECTF *Out)
{
	const RECTF & LocalRect = GetLocalRect();

	Out->top = GetTabHeight() + TAB_CONTROL_MARGIN;
	Out->bottom = LocalRect.bottom - TAB_CONTROL_MARGIN;
	Out->left = TAB_CONTROL_MARGIN;
	Out->right = LocalRect.right - TAB_CONTROL_MARGIN;
}

void TabControl::OnDraw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetFont(g_Font);

	COLOR WhiteColor = GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY;

	float TabHeight = GetTabHeight();

	// T³o
	gfx2d::g_Canvas->SetColor(WhiteColor);
	gfx2d::g_Canvas->DrawRect(GetLocalRect(), gfx2d::ComplexElement(0));

	// Zak³adki
	Tab *t;
	RECTF TabRect = RECTF(0.0f, 0.0f, 0.0f, TabHeight);
	// - T³o
	// Tu przy okazji znajdê sobie pozycjê pocz¹tku i koñca wybranego taba
	float SelectedTabBeginX, SelectedTabEndX;
	gfx2d::g_Canvas->SetColor(WhiteColor);
	for (uint4 i = 0; i < GetTabCount(); i++)
	{
		t = GetTab(i);
		if (t)
		{
			TabRect.right = TabRect.left + t->GetTabWidth();
			gfx2d::g_Canvas->DrawRect(TabRect, gfx2d::ComplexElement(5));
			if (m_TabIndex == i)
			{
				SelectedTabBeginX = TabRect.left;
				SelectedTabEndX = TabRect.right;
				// Focus
				if (IsFocused())
				{
					RECTF FocusRect = TabRect;
					FocusRect.Extend(-2.0f);
					gfx2d::g_Canvas->SetColor(g_FocusColor);
					gfx2d::g_Canvas->DrawRect(FocusRect, gfx2d::ComplexElement(2));
					gfx2d::g_Canvas->SetColor(WhiteColor);
				}
			}
			TabRect.left = TabRect.right;
		}
	}
	TabRect.top = TabHeight*0.5f;
	TabRect.left = TAB_MARGIN;
	// - Tytu³
	// Tu wykorzystywane s¹ tylko TabRect.left i TabRect.top jako lewa-œrodkowa pozycja tekstu.
	for (uint4 i = 0; i < GetTabCount(); i++)
	{
		t = GetTab(i);
		if (t)
		{
			gfx2d::g_Canvas->SetColor(m_MouseOverTabIndex == i ? g_HighlightFontColor : g_FontColor);
			gfx2d::g_Canvas->DrawText_(TabRect.left, TabRect.top, t->GetTitle(), g_FontSize,
				res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
			TabRect.left += t->GetTabWidth();
		}
	}

	// Ramka taba
	RECTF ContentRect = RECTF(0.0f, TabHeight, GetLocalRect().right, GetLocalRect().bottom);
	gfx2d::g_Canvas->SetColor(WhiteColor);
	gfx2d::g_Canvas->DrawRect(ContentRect, gfx2d::ComplexElement(1));

	// Po³¹czenie z wybranym tabem - zamalowanie
	if (m_TabIndex != MAXUINT4)
	{
		gfx2d::g_Canvas->DrawRect(
			RECTF(SelectedTabBeginX, TabHeight, SelectedTabEndX, TabHeight+TAB_CONTENT_MARGIN),
			gfx2d::ComplexElement(0));
	}
}

void TabControl::OnRectChange()
{
	UpdateTabRects();
}

void TabControl::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Button == frame::MOUSE_LEFT)
	{
		if (Action == frame::MOUSE_DOWN || Action == frame::MOUSE_DBLCLK)
		{
			if (Pos.x >= 0.0f && Pos.y >= 0.0f && Pos.y < GetTabHeight())
			{
				float StartX = 0.0f, EndX;
				Tab *t;
				for (uint4 i = 0; i < GetTabCount(); i++)
				{
					t = GetTab(i);
					if (t)
					{
						EndX = StartX + t->GetTabWidth();
						if (Pos.x >= StartX && Pos.x < EndX)
						{
							SetTabIndex(i);
							if (OnTabChange) OnTabChange(this);
							break;
						}
						StartX = EndX;
					}
				}
			}
		}
	}
}

bool TabControl::OnKeyDown(uint4 Key)
{
	switch (Key)
	{
	case VK_LEFT:
		if (GetTabCount() > 0)
		{
			if (GetTabIndex() == MAXUINT4)
			{
				SetTabIndex(GetTabCount()-1);
				if (OnTabChange) OnTabChange(this);
			}
			else if (GetTabIndex() > 0)
			{
				SetTabIndex(GetTabIndex()-1);
				if (OnTabChange) OnTabChange(this);
			}
		}
		return true;
	case VK_RIGHT:
		if (GetTabCount() > 0)
		{
			if (GetTabIndex() == MAXUINT4)
			{
				SetTabIndex(0);
				if (OnTabChange) OnTabChange(this);
			}
			else if (GetTabIndex() < GetTabCount()-1)
			{
				SetTabIndex(GetTabIndex()+1);
				if (OnTabChange) OnTabChange(this);
			}
		}
		return true;
	case VK_HOME:
		if (GetTabCount() > 0)
		{
			SetTabIndex(0);
			if (OnTabChange) OnTabChange(this);
		}
		return true;
	case VK_END:
		if (GetTabCount() > 0)
		{
			SetTabIndex(GetTabCount()-1);
			if (OnTabChange) OnTabChange(this);
		}
		return true;
	}

	return CompositeControl::OnKeyDown(Key);
}

void TabControl::OnMouseMove(const VEC2 &Pos)
{
	float x = 0.0f;
	Tab *t;

	if (Pos.x >= 0.0f && Pos.y >= 0.0f && Pos.y < GetTabHeight())
	{
		for (uint4 i = 0; i < GetTabCount(); i++)
		{
			t = GetTab(i);
			if (t)
			{
				x += t->GetTabWidth();
				if (Pos.x < x)
				{
					m_MouseOverTabIndex = i;
					break;
				}
			}
		}
	}
	else
		m_MouseOverTabIndex = MAXUINT4;
}

TabControl::TabControl(CompositeControl *Parent, const RECTF &Rect, bool Focusable) :
	CompositeControl(Parent, Focusable, Rect),
	m_TabIndex(MAXUINT4),
	m_MouseOverTabIndex(MAXUINT4)
{
}

TabControl::Tab * TabControl::AddTab(const string &Title)
{
	RECTF TabContentRect; GetTabContentRect(&TabContentRect);
	return new Tab(this, TabContentRect, Title);
}

bool TabControl::RemoveTab(uint4 Index)
{
	if (Index >= GetTabCount())
		return false;

	// Usuwamy aktywn¹
	if (m_TabIndex == Index)
	{
		// To jest jedyna - aktywna nie bêdzie ¿adna
		if (GetTabCount() == 1)
			m_TabIndex = MAXUINT4;
		// To nie jest pierwsza - aktywna bêdzie poprzednia
		else if (Index > 0)
		{
			m_TabIndex--;
			GetTab(m_TabIndex)->SetVisible(true);
		}
		// To jest pierwsza - aktywna bêdzie nastêpna
		else
		{
			// Indeks pozostanie ten sam
			// A nastêpna (zaraz po usuniêciu wskoczy na ten indeks) ma byæ widoczna
			GetTab(m_TabIndex+1)->SetVisible(true);
		}
	}
	// Aktywna jest któraœ dalej za usuwan¹
	else if (m_TabIndex != MAXUINT4 && m_TabIndex > Index)
	{
		// Indeks aktywnej przekakuje na poprzedni, aktywna zak³adka zostaje ta sama
		m_TabIndex--;
	}

	delete GetTab(Index);
	return true;
}

bool TabControl::RemoveTab(Tab *tab)
{
	uint4 TabIndex = FindTab(tab);
	if (TabIndex != MAXUINT4)
		return RemoveTab(TabIndex);
	else
		return false;
}

bool TabControl::RenameTab(uint4 Index, const string &NewTitle)
{
	Tab *tab = GetTab(Index);
	if (tab)
	{
		tab->SetTitle(NewTitle);
		return true;
	}
	else
		return false;
}

TabControl::Tab * TabControl::GetTab(uint4 Index)
{
	if (Index >= GetTabCount())
		return 0;
	return dynamic_cast<Tab*>(GetSubControl(Index));
}

void TabControl::SetTabIndex(uint4 TabIndex)
{
	if (TabIndex == m_TabIndex)
		return;

	Tab *t;

	// Ukryj poprzednio aktywn¹
	if (m_TabIndex != MAXUINT4)
	{
		t = GetTab(m_TabIndex);
		if (t) t->SetVisible(false);
	}

	m_TabIndex = TabIndex;
	
	// Poka¿ now¹ aktywn¹
	if (m_TabIndex != MAXUINT4)
	{
		t = GetTab(m_TabIndex);
		if (t) t->SetVisible(true);
	}
}

void TabControl::UpdateTabRects()
{
	Tab *t;
	RECTF TabContentRect; GetTabContentRect(&TabContentRect);

	for (uint4 i = 0; i < GetTabCount(); i++)
	{
		t = GetTab(i);
		if (t)
			t->SetRect(TabContentRect);
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Menu

const Menu::ITEM Menu::SEPARATOR = Menu::ITEM(MAXUINT4, string());

float Menu::GetMenuItemHeight()
{
	return g_FontSize + MENU_ITEM_MARGIN*2.0f;
}

Menu::Menu() :
	Control(0, true, RECTF::ZERO),
	m_ItemIndex(MAXUINT4)
{
	SetVisible(false);
	SetLayer(gui::LAYER_VOLATILE);
}

void Menu::AddItem(const ITEM &Item)
{
	m_Items.push_back(Item);
	HandleItemsChange();
}

void Menu::InsertItem(size_t Index, const ITEM &Item)
{
	if (Index > m_Items.size())
		return;
	m_Items.insert(m_Items.begin()+Index, Item);
	HandleItemsChange();
}

void Menu::RemoveItem(size_t Index)
{
	if (Index >= m_Items.size())
		return;
	m_Items.erase(m_Items.begin()+Index);
	HandleItemsChange();
}

size_t Menu::FindItem(uint4 Id)
{
	for (size_t i = 0; i < m_Items.size(); i++)
		if (m_Items[i].Id == Id)
			return i;
	return MAXUINT4;
}

void Menu::SetItem(size_t Index, const ITEM &Item)
{
	if (Index >= m_Items.size())
		return;
	m_Items[Index] = Item;
	HandleItemsChange();
}

void Menu::SetItemText(size_t Index, const string &Text)
{
	if (Index >= m_Items.size())
		return;
	m_Items[Index].Text = Text;
	HandleItemsChange();
}

void Menu::SetItemInfoText(size_t Index, const string &InfoText)
{
	if (Index >= m_Items.size())
		return;
	m_Items[Index].InfoText = InfoText;
	HandleItemsChange();
}

void Menu::HandleItemsChange()
{
	// Wyznaczanie MenuWidth polega na znalezieniu najszerszej pozycji.
	// Wyznaczenie MenuHeight to ostatni Y (offset elementu nastêpnego za ostanim) +
	// margines dolny (górny ju¿ jest dodawany na pocz¹tku).

	m_OffsetsY.clear();
	m_MenuWidth = MENU_MIN_WIDTH;
	float Width, y = MENU_MARGIN;

	for (size_t i = 0; i < m_Items.size(); i++)
	{
		m_OffsetsY.push_back(y);

		// Separator
		if (m_Items[i].Id == MAXUINT4)
		{
			// Wysokoœæ
			y += MENU_SEPARATOR_HEIGHT + 2.0f*MENU_ITEM_MARGIN;
		}
		// Pozycja menu
		else
		{
			// == Szerokoœæ ==
			// Nie ma InfoText - szerokoœæ tekstu
			if (m_Items[i].InfoText.empty())
				Width = g_Font->GetTextWidth(m_Items[i].Text, g_FontSize);
			// Jest InfoTekst - obydwie szerokoœci
			else
			{
				Width =
					g_Font->GetTextWidth(m_Items[i].Text, g_FontSize) +
					MENU_INFO_SPACE +
					g_Font->GetTextWidth(m_Items[i].InfoText, g_FontSize);
			}
			// Nowa szerokoœæ
			m_MenuWidth = std::max(m_MenuWidth, Width);

			// == Wysokoœæ ==
			y += g_FontSize + 2.0f*MENU_ITEM_MARGIN;
		}
	}
	m_OffsetsY.push_back(y);

	m_MenuWidth += MENU_ITEM_MARGIN*2.0f + MENU_MARGIN*2.0f;
	m_MenuHeight = y + MENU_MARGIN;

	m_ItemContentLeft = MENU_MARGIN + MENU_ITEM_MARGIN;
	m_ItemContentRight = m_MenuWidth - MENU_MARGIN - MENU_ITEM_MARGIN;
}

void Menu::PopupMenu(const VEC2 &Pos)
{
	// Oblicz pozycjê lewego górnego rogu
	VEC2 PopupPos;
	CalculatePopupPos(&PopupPos, Pos, m_MenuWidth, m_MenuHeight);

	// Wyœwietl!
	m_ItemIndex = MAXUINT4;

	SetRect(RECTF(PopupPos.x, PopupPos.y, PopupPos.x+m_MenuWidth, PopupPos.y+m_MenuHeight));
	SetVisible(true);
	Focus();
}

void Menu::CloseMenu()
{
	SetVisible(false);
}

void Menu::OnDraw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetFont(g_Font);
	gfx2d::g_Canvas->SetColor(COLOR::WHITE);

	// T³o
	gfx2d::g_Canvas->PushAlpha(MENU_ALPHA);
	gfx2d::g_Canvas->DrawRect(GetLocalRect(), gfx2d::ComplexElement(1));
	gfx2d::g_Canvas->PopAlpha();

	// Elementy
	assert(m_OffsetsY.size() == m_Items.size()+1 && "Nieuaktualnione wyliczane dane na temat pozycji menu");
	float CurrentY;
	COLOR TextColor;

	for (size_t i = 0; i < m_Items.size(); i++)
	{
		CurrentY = m_OffsetsY[i] + MENU_ITEM_MARGIN;
		// Separator
		if (m_Items[i].Id == MAXUINT4)
		{
			gfx2d::g_Canvas->SetColor(COLOR::WHITE);
			gfx2d::g_Canvas->DrawRect(
				RECTF(m_ItemContentLeft, CurrentY, m_ItemContentRight, CurrentY+MENU_SEPARATOR_HEIGHT),
				31);
		}
		// Pozycja menu
		else
		{
			// Zaznaczona
			if (m_ItemIndex == i)
			{
				TextColor = g_SelFontColor;
				gfx2d::g_Canvas->SetColor(g_SelBkgColor);
				gfx2d::g_Canvas->SetSprite(0);
				gfx2d::g_Canvas->DrawRect(RECTF(MENU_MARGIN, m_OffsetsY[i], m_MenuWidth-MENU_MARGIN, m_OffsetsY[i+1]));
				gfx2d::g_Canvas->SetSprite(g_Sprite);
			}
			else
				TextColor = g_FontColor;

			gfx2d::g_Canvas->SetColor(TextColor);
			gfx2d::g_Canvas->DrawText_(m_ItemContentLeft, CurrentY, m_Items[i].Text, g_FontSize,
				res::Font::FLAG_HLEFT | res::Font::FLAG_VTOP | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
			if (!m_Items[i].InfoText.empty())
			{
				gfx2d::g_Canvas->SetColor(m_ItemIndex == i ? g_SelInfoFontColor : g_InfoFontColor);
				gfx2d::g_Canvas->DrawText_(m_ItemContentRight, CurrentY, m_Items[i].InfoText, g_FontSize,
					res::Font::FLAG_HRIGHT | res::Font::FLAG_VTOP | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
			}
		}
	}
}

uint4 Menu::PosToItem(const VEC2 &Pos)
{
	if (Pos.y >= MENU_MARGIN && Pos.y < m_MenuHeight-MENU_MARGIN &&
		Pos.x >= MENU_MARGIN && Pos.x < m_MenuWidth-MENU_MARGIN)
	{
		for (size_t i = 0; i < m_Items.size(); i++)
		{
			if (Pos.y < m_OffsetsY[i+1])
				return i;
		}
	}
	return MAXUINT4;
}

void Menu::OnMouseMove(const VEC2 &Pos)
{
	uint4 Index = PosToItem(Pos);
	if (Index != MAXUINT4 && IsSelectable(Index))
		m_ItemIndex = Index;
}

void Menu::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	// Dzia³a tak DOWN jak i UP
	if (Button == frame::MOUSE_LEFT)
	{
		uint4 Index = PosToItem(Pos);
		if (Index != MAXUINT4 && IsSelectable(Index))
		{
			if (OnClick) OnClick(this, m_Items[Index].Id);
			CloseMenu();
		}
	}
}

bool Menu::OnKeyDown(uint4 Key)
{
	switch (Key)
	{
	case VK_ESCAPE:
		CloseMenu();
		return true;
	case VK_DOWN:
		SelectDown( m_ItemIndex >= m_Items.size() ? 0 : (m_ItemIndex+1) % m_Items.size() );
		return true;
	case VK_UP:
		SelectUp( (m_ItemIndex >= m_Items.size() || m_ItemIndex == 0) ? m_Items.size()-1 : m_ItemIndex-1 );
		return true;
	case VK_HOME:
		SelectDown(0);
		return true;
	case VK_END:
		SelectUp(m_Items.size()-1);
		return true;
	case VK_RETURN:
	case VK_SPACE:
		if (m_ItemIndex < m_Items.size() && IsSelectable(m_ItemIndex))
		{
			if (OnClick) OnClick(this, m_Items[m_ItemIndex].Id);
			CloseMenu();
		}
		return true;
	}

	return Control::OnKeyDown(Key);
}

void Menu::SelectDown(size_t Index)
{
	if (m_Items.empty()) return;

	size_t StartIndex = Index;

	for (;;)
	{
		if (IsSelectable(Index))
		{
			m_ItemIndex = Index;
			break;
		}
		Index = (Index+1) % m_Items.size();
		// Zawinê³o siê od pocz¹tku i nie znaleziono zaznaczalnej pozycji
		if (Index == StartIndex)
			break;
	}
}

void Menu::SelectUp(size_t Index)
{
	if (m_Items.empty()) return;

	size_t StartIndex = Index;

	for (;;)
	{
		if (IsSelectable(Index))
		{
			m_ItemIndex = Index;
			break;
		}
		Index = (Index == 0 ? m_Items.size()-1 : Index-1);
		// Zawinê³o siê od pocz¹tku i nie znaleziono zaznaczalnej pozycji
		if (Index == StartIndex)
			break;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Window

void Window::OnDraw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetFont(g_Font);
	COLOR WhiteColor = GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY;
	gfx2d::g_Canvas->SetColor(WhiteColor);

	const RECTF & LocalRect = GetLocalRect();
	RECTF TitleBarRect, Rect;
	// Ramka zewnêtrzna
	gfx2d::g_Canvas->PushAlpha(0.75f);
	gfx2d::g_Canvas->DrawRect(GetLocalRect(), gfx2d::ComplexElement(1));
	gfx2d::g_Canvas->PopAlpha();
	// Ramka wewnêtrzna
	Rect = LocalRect;
	Rect.Extend(-WINDOW_MARGIN_1);
	gfx2d::g_Canvas->DrawRect(
		RECTF(Rect.left, Rect.top+WINDOW_TITLE_HEIGHT+WINDOW_MARGIN_3, Rect.right, Rect.bottom),
		gfx2d::ComplexElement(3));
	// Przycisk zamkniêcia
	GetTitleBarRect(&TitleBarRect);
	GetCloseButtonRect(TitleBarRect, &Rect);
	gfx2d::g_Canvas->DrawRect(Rect, gfx2d::ComplexElement(m_State == STATE_CLOSE ? 3 : 1));
	Rect.Extend(-4.0f);
	gfx2d::g_Canvas->SetColor(g_FontColor);
	gfx2d::g_Canvas->DrawRect(Rect, 47);
	// Napis
	gfx2d::g_Canvas->SetColor(g_HeaderFontColor);
	gfx2d::g_Canvas->DrawText_(TitleBarRect.left, TitleBarRect.GetCenterY(), m_Title, g_FontSize,
		res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
}

void Window::GetTitleBarRect(RECTF *Out)
{
	Out->top = WINDOW_MARGIN_1;
	Out->bottom = Out->top + WINDOW_TITLE_HEIGHT;
	Out->left = WINDOW_MARGIN_1;
	Out->right = GetLocalRect().right - WINDOW_MARGIN_1;
}

void Window::GetCloseButtonRect(const RECTF &TitleBarRect, RECTF *Out)
{
	*Out = TitleBarRect;
	Out->left = Out->right - Out->Height();
}

void Window::OnMouseMove(const VEC2 &Pos)
{
	Cursor *NewCursor = g_StandardCursors[CT_NORMAL];

	if (m_State == STATE_NONE)
	{
		// Uwaga! Hack! Skopiowa³em to z poni¿ej, bo nie chcia³o mi siê robiæ z tego osobnej funkcji.
		float W = GetLocalRect().right;
		float H = GetLocalRect().bottom;
		RECTF TitleBarRect; GetTitleBarRect(&TitleBarRect);
		RECTF CloseButtonRect; GetCloseButtonRect(TitleBarRect, &CloseButtonRect);
		// 1 - poziome, 2 - pionowe
		RECTF CornerLT1 = RECTF(0.0f, 0.0f, WINDOW_CORNER_SIZE, WINDOW_MARGIN_1);
		RECTF CornerLT2 = RECTF(0.0f, 0.0f, WINDOW_MARGIN_1, WINDOW_CORNER_SIZE);
		RECTF CornerRT1 = RECTF(W-WINDOW_CORNER_SIZE, 0.0f, W, WINDOW_MARGIN_1);
		RECTF CornerRT2 = RECTF(W-WINDOW_MARGIN_1, 0.0f, W, WINDOW_CORNER_SIZE);
		RECTF CornerLB1 = RECTF(0.0f, H-WINDOW_MARGIN_1, WINDOW_CORNER_SIZE, H);
		RECTF CornerLB2 = RECTF(0.0f, H-WINDOW_CORNER_SIZE, WINDOW_MARGIN_1, H);
		RECTF CornerRB1 = RECTF(W-WINDOW_CORNER_SIZE, H-WINDOW_MARGIN_1, W, H);
		RECTF CornerRB2 = RECTF(W-WINDOW_MARGIN_1, H-WINDOW_CORNER_SIZE, W, H);
		RECTF EdgeL = RECTF(CornerLT2.left, CornerLT2.bottom, CornerLT2.right, CornerLB2.top);
		RECTF EdgeR = RECTF(CornerRT2.left, CornerRT2.bottom, CornerRT2.right, CornerRB2.top);
		RECTF EdgeT = RECTF(CornerLT1.right, CornerLT1.top, CornerRT1.left, CornerLT1.bottom);
		RECTF EdgeB = RECTF(CornerLB1.right, CornerLB1.top, CornerRB1.left, CornerLB1.bottom);

		if (PointInRect(Pos, CloseButtonRect))
			NewCursor = g_StandardCursors[CT_NORMAL];
		else if (PointInRect(Pos, TitleBarRect))
			NewCursor = g_StandardCursors[CT_MOVE];
		else if (PointInRect(Pos, EdgeL))
			NewCursor = g_StandardCursors[CT_SIZE_H];
		else if (PointInRect(Pos, EdgeR))
			NewCursor = g_StandardCursors[CT_SIZE_H];
		else if (PointInRect(Pos, EdgeT))
			NewCursor = g_StandardCursors[CT_SIZE_V];
		else if (PointInRect(Pos, EdgeB))
			NewCursor = g_StandardCursors[CT_SIZE_V];
		else if (PointInRect(Pos, CornerLT1) || PointInRect(Pos, CornerLT2))
			NewCursor = g_StandardCursors[CT_SIZE_LT];
		else if (PointInRect(Pos, CornerRT1) || PointInRect(Pos, CornerRT2))
			NewCursor = g_StandardCursors[CT_SIZE_RT];
		else if (PointInRect(Pos, CornerLB1) || PointInRect(Pos, CornerLB2))
			NewCursor = g_StandardCursors[CT_SIZE_RT];
		else if (PointInRect(Pos, CornerRB1) || PointInRect(Pos, CornerRB2))
			NewCursor = g_StandardCursors[CT_SIZE_LT];
	}
	else
	{
		if (m_State == STATE_MOVE)
		{
			RECTF NewRect = GetRect();
			NewRect += VEC2(Pos - m_StatePos);
			SetRect(NewRect);
		}
		else
		{
			if (m_State == STATE_SIZE_L || m_State == STATE_SIZE_LT || m_State == STATE_SIZE_LB)
			{
				RECTF NewRect = GetRect();
				NewRect.left += Pos.x - m_StatePos.x;
				if (NewRect.Width() >= WINDOW_MIN_WIDTH)
					SetRect(NewRect);
			}
			if (m_State == STATE_SIZE_R || m_State == STATE_SIZE_RT || m_State == STATE_SIZE_RB)
			{
				RECTF NewRect = GetRect();
				NewRect.right += Pos.x - m_StatePos.x;
				if (NewRect.Width() >= WINDOW_MIN_WIDTH)
				{
					m_StatePos.x = Pos.x;
					SetRect(NewRect);
				}
			}

			if (m_State == STATE_SIZE_T || m_State == STATE_SIZE_LT || m_State == STATE_SIZE_RT)
			{
				RECTF NewRect = GetRect();
				NewRect.top += Pos.y - m_StatePos.y;
				if (NewRect.Height() >= WINDOW_MIN_HEIGHT)
					SetRect(NewRect);
			}
			if (m_State == STATE_SIZE_B || m_State == STATE_SIZE_LB || m_State == STATE_SIZE_RB)
			{
				RECTF NewRect = GetRect();
				NewRect.bottom += Pos.y - m_StatePos.y;
				if (NewRect.Height() >= WINDOW_MIN_HEIGHT)
				{
					m_StatePos.y = Pos.y;
					SetRect(NewRect);
				}
			}
		}
	}

	if (NewCursor != GetCursor())
		SetCursor(NewCursor);
}

void Window::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Button == frame::MOUSE_LEFT)
	{
		if (Action == frame::MOUSE_DOWN || Action == frame::MOUSE_DBLCLK)
		{
			// Uwaga! Hack! Skopiowa³em to powy¿ej, bo nie chcia³o mi siê robiæ z tego osobnej funkcji.
			float W = GetLocalRect().right;
			float H = GetLocalRect().bottom;
			RECTF TitleBarRect; GetTitleBarRect(&TitleBarRect);
			RECTF CloseButtonRect; GetCloseButtonRect(TitleBarRect, &CloseButtonRect);
			// 1 - poziome, 2 - pionowe
			RECTF CornerLT1 = RECTF(0.0f, 0.0f, WINDOW_CORNER_SIZE, WINDOW_MARGIN_1);
			RECTF CornerLT2 = RECTF(0.0f, 0.0f, WINDOW_MARGIN_1, WINDOW_CORNER_SIZE);
			RECTF CornerRT1 = RECTF(W-WINDOW_CORNER_SIZE, 0.0f, W, WINDOW_MARGIN_1);
			RECTF CornerRT2 = RECTF(W-WINDOW_MARGIN_1, 0.0f, W, WINDOW_CORNER_SIZE);
			RECTF CornerLB1 = RECTF(0.0f, H-WINDOW_MARGIN_1, WINDOW_CORNER_SIZE, H);
			RECTF CornerLB2 = RECTF(0.0f, H-WINDOW_CORNER_SIZE, WINDOW_MARGIN_1, H);
			RECTF CornerRB1 = RECTF(W-WINDOW_CORNER_SIZE, H-WINDOW_MARGIN_1, W, H);
			RECTF CornerRB2 = RECTF(W-WINDOW_MARGIN_1, H-WINDOW_CORNER_SIZE, W, H);
			RECTF EdgeL = RECTF(CornerLT2.left, CornerLT2.bottom, CornerLT2.right, CornerLB2.top);
			RECTF EdgeR = RECTF(CornerRT2.left, CornerRT2.bottom, CornerRT2.right, CornerRB2.top);
			RECTF EdgeT = RECTF(CornerLT1.right, CornerLT1.top, CornerRT1.left, CornerLT1.bottom);
			RECTF EdgeB = RECTF(CornerLB1.right, CornerLB1.top, CornerRB1.left, CornerLB1.bottom);

			CURSOR_TYPE CursorType = CT_NORMAL;

			if (PointInRect(Pos, CloseButtonRect))
			{
				m_State = STATE_CLOSE;
				CursorType = CT_NORMAL;
			}
			else if (PointInRect(Pos, TitleBarRect))
			{
				m_State = STATE_MOVE;
				CursorType = CT_MOVE;
			}
			else if (PointInRect(Pos, EdgeL))
			{
				m_State = STATE_SIZE_L;
				CursorType = CT_SIZE_H;
			}
			else if (PointInRect(Pos, EdgeR))
			{
				m_State = STATE_SIZE_R;
				CursorType = CT_SIZE_H;
			}
			else if (PointInRect(Pos, EdgeT))
			{
				m_State = STATE_SIZE_T;
				CursorType = CT_SIZE_V;
			}
			else if (PointInRect(Pos, EdgeB))
			{
				m_State = STATE_SIZE_B;
				CursorType = CT_SIZE_V;
			}
			else if (PointInRect(Pos, CornerLT1) || PointInRect(Pos, CornerLT2))
			{
				m_State = STATE_SIZE_LT;
				CursorType = CT_SIZE_LT;
			}
			else if (PointInRect(Pos, CornerRT1) || PointInRect(Pos, CornerRT2))
			{
				m_State = STATE_SIZE_RT;
				CursorType = CT_SIZE_RT;
			}
			else if (PointInRect(Pos, CornerLB1) || PointInRect(Pos, CornerLB2))
			{
				m_State = STATE_SIZE_LB;
				CursorType = CT_SIZE_RT;
			}
			else if (PointInRect(Pos, CornerRB1) || PointInRect(Pos, CornerRB2))
			{
				m_State = STATE_SIZE_RB;
				CursorType = CT_SIZE_LT;
			}

			if (m_State != STATE_NONE)
			{
				m_StatePos = Pos;
				StartDrag(DRAG_DATA_SHARED_PTR(new DragCursor(CursorType)));

				if (m_State == STATE_MOVE)
				{
					if (OnMoveBegin) OnMoveBegin(this);
				}
				else if (m_State == STATE_SIZE_L || m_State == STATE_SIZE_R ||
					m_State == STATE_SIZE_T || m_State == STATE_SIZE_B ||
					m_State == STATE_SIZE_LT || m_State == STATE_SIZE_RT ||
					m_State == STATE_SIZE_LB || m_State == STATE_SIZE_RB)
				{
					if (OnResizeBegin) OnResizeBegin(this);
				}
			}
		}
		else if (Action == frame::MOUSE_UP)
		{
			if (m_State == STATE_CLOSE)
			{
				RECTF TitleBarRect; GetTitleBarRect(&TitleBarRect);
				RECTF CloseButtonRect; GetCloseButtonRect(TitleBarRect, &CloseButtonRect);
				if (PointInRect(Pos, CloseButtonRect))
				{
					if (OnClose) OnClose(this);
				}
			}
			else if (m_State == STATE_MOVE)
			{
				if (OnMoveEnd) OnMoveEnd(this);
			}
			else if (m_State == STATE_SIZE_L || m_State == STATE_SIZE_R ||
				m_State == STATE_SIZE_T || m_State == STATE_SIZE_B ||
				m_State == STATE_SIZE_LT || m_State == STATE_SIZE_RT ||
				m_State == STATE_SIZE_LB || m_State == STATE_SIZE_RB)
			{
				if (OnResizeEnd) OnResizeEnd(this);
			}
			m_State = STATE_NONE;
		}
	}
}

void Window::GetClientRect(RECTF *Out)
{
	float MarginSum = WINDOW_MARGIN_1 + WINDOW_MARGIN_2;
	Out->left = MarginSum;
	Out->top = MarginSum + WINDOW_TITLE_HEIGHT + WINDOW_MARGIN_3;
	Out->right = GetLocalRect().right - MarginSum;
	Out->bottom = GetLocalRect().bottom - MarginSum;
}

Window::Window(const RECTF &Rect, const string &Title) :
	CompositeControl(0, false, Rect),
	m_State(STATE_NONE),
	m_Title(Title)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Edit

void Edit::OnDraw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetFont(g_Font);

	COLOR WhiteColor = GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY;

	// Ramka
	RECTF LocalRect = GetLocalRect();
	gfx2d::g_Canvas->SetColor(WhiteColor);
	gfx2d::g_Canvas->DrawRect(LocalRect, gfx2d::ComplexElement(3));

	// Tekst
	LocalRect.Extend(-EDIT_MARGIN);
	gfx2d::g_Canvas->PushTranslation(VEC2(LocalRect.left, LocalRect.top));
	LocalRect.right -= LocalRect.left;
	LocalRect.bottom -= LocalRect.top;
	LocalRect.left = LocalRect.top = 0.0f;
	gfx2d::g_Canvas->PushClipRect(LocalRect);
	// Nie ma tekstu
	if (m_Text.empty())
	{
		// InfoText
		if (!IsFocused() && !m_InfoText.empty())
		{
			gfx2d::g_Canvas->SetColor(g_InfoFontColor);
			gfx2d::g_Canvas->DrawText_(-m_ScrollX, LocalRect.GetCenterY(), m_InfoText, g_FontSize,
				res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
		}
	}
	// Jest tekst
	else
	{
		gfx2d::g_Canvas->SetColor(g_FontColor);
		// Has³o
		if (m_Password)
		{
			float Y = LocalRect.GetCenterY();
			// Nie ma zaznaczenia lub go nie widaæ
			if (!IsFocused() || m_SelBegin == m_SelEnd)
			{
				float X = -m_ScrollX;
				float Y1 = Y - EDIT_PASSWORD_DOT_SIZE*0.5f;
				float Y2 = Y + EDIT_PASSWORD_DOT_SIZE*0.5f;
				for (size_t i = 0; i < m_Text.size(); i++, X += EDIT_PASSWORD_DOT_SIZE + EDIT_PASSWORD_DOT_SPACE)
				{
					// Jeszcze po lewej
					if (X + EDIT_PASSWORD_DOT_SIZE < 0.0f)
						continue;
					// Ju¿ po prawej
					if (X > LocalRect.right)
						break;
					// Trzeba narysowaæ
					gfx2d::g_Canvas->DrawRect(RECTF(X, Y1, X+EDIT_PASSWORD_DOT_SIZE, Y2), 44);
				}
			}
			// Jest zaznaczenie
			else
			{
				// T³o
				float Y1 = Y - g_FontSize*0.5f;
				float Y2 = Y + g_FontSize*0.5f;
				gfx2d::g_Canvas->SetColor(g_SelBkgColor);
				gfx2d::g_Canvas->DrawRect(RECTF(m_SelBeginPos-m_ScrollX, Y1, m_SelEndPos-m_ScrollX, Y2), 48);

				// Kropeczki
				float X = -m_ScrollX;
				Y1 = Y - EDIT_PASSWORD_DOT_SIZE*0.5f;
				Y2 = Y + EDIT_PASSWORD_DOT_SIZE*0.5f;
				gfx2d::g_Canvas->SetColor(g_FontColor);
				for (size_t i = 0; i < m_Text.size(); i++, X += EDIT_PASSWORD_DOT_SIZE + EDIT_PASSWORD_DOT_SPACE)
				{
					// Pocz¹tek zaznaczenia
					if (i == m_SelBegin)
						gfx2d::g_Canvas->SetColor(g_SelFontColor);
					// Koniec zaznaczenia
					else if (i == m_SelEnd)
						gfx2d::g_Canvas->SetColor(g_FontColor);

					// Jeszcze po lewej
					if (X + EDIT_PASSWORD_DOT_SIZE < 0.0f)
						continue;
					// Ju¿ po prawej
					if (X > LocalRect.right)
						break;
					// Trzeba narysowaæ
					gfx2d::g_Canvas->DrawRect(RECTF(X, Y1, X+EDIT_PASSWORD_DOT_SIZE, Y2), 44);
				}
			}
		}
		// Nie has³o
		else
		{
			// Nie ma zaznaczenia lub go nie widaæ
			if (!IsFocused() || m_SelBegin == m_SelEnd)
			{
				gfx2d::g_Canvas->DrawText_(-m_ScrollX, LocalRect.GetCenterY(), m_Text, g_FontSize,
					res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
			}
			// Jest zaznaczenie
			else
			{
				float Y = LocalRect.GetCenterY();
				float Y1 = Y - g_FontSize*0.5f;
				float Y2 = Y + g_FontSize*0.5f;
				// Przed zaznaczeniem
				if (m_SelBegin > 0)
				{
					gfx2d::g_Canvas->SetColor(g_FontColor);
					gfx2d::g_Canvas->DrawText_(-m_ScrollX, Y, m_Text, 0, m_SelBegin, g_FontSize,
						res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
				}
				// Zaznaczenie
				// - T³o
				gfx2d::g_Canvas->SetColor(g_SelBkgColor);
				gfx2d::g_Canvas->DrawRect(RECTF(m_SelBeginPos-m_ScrollX, Y1, m_SelEndPos-m_ScrollX, Y2), 48);
				// - Tekst
				gfx2d::g_Canvas->SetColor(g_SelFontColor);
				gfx2d::g_Canvas->DrawText_(m_SelBeginPos-m_ScrollX, Y, m_Text, m_SelBegin, m_SelEnd, g_FontSize,
					res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
				// Po zaznaczeniu
				if (m_SelEnd < m_Text.length())
				{
					gfx2d::g_Canvas->SetColor(g_FontColor);
					gfx2d::g_Canvas->DrawText_(m_SelEndPos-m_ScrollX, Y, m_Text, m_SelEnd, m_Text.length(), g_FontSize,
						res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
				}
			}
		}
	}

	// Kursor
	if (IsFocused() && frac((frame::Timer1.GetTime()-m_LastEditTime)*CURSOR_FLASH_PERIOD) < 0.5f)
	{
		float CursorTop, CursorBottom;
		CursorTop = CursorBottom = LocalRect.GetCenterY();
		CursorTop -= g_FontSize*0.5f;
		CursorBottom += g_FontSize*0.5f;

		gfx2d::g_Canvas->SetColor(CURSOR_COLOR);
		gfx2d::g_Canvas->DrawRect(RECTF(m_CursorBeginPos-m_ScrollX, CursorTop, m_CursorEndPos-m_ScrollX, CursorBottom), 48);
	}

	gfx2d::g_Canvas->PopClipRect();
	gfx2d::g_Canvas->PopTranslation();
}

bool Edit::MouseHitTest(size_t *OutPos, float X)
{
	size_t Pos;
	float Percent;
	bool B;

	float X2 = X-EDIT_MARGIN+m_ScrollX;

	if (m_Password)
	{
		if (X2 < 0.0f)
			B = false;
		else if (X2 > m_TextWidth)
			B = false;
		else
		{
			float v = X2 / (EDIT_PASSWORD_DOT_SIZE+EDIT_PASSWORD_DOT_SPACE);
			Pos = (size_t)v;
			Percent = frac(v);
			B = true;
		}
	}
	else
	{
		B = g_Font->HitTestSL(&Pos, &Percent, EDIT_MARGIN-m_ScrollX, X, m_Text, g_FontSize,
			res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
	}

	if (B)
	{
		if (Percent > 0.5f)
			Pos = std::min(m_Text.length(), Pos+1);
		*OutPos = Pos;
		return true;
	}
	else if (X2 < 0.0f)
	{
		*OutPos = 0;
		return true;
	}
	else if (X2 > m_TextWidth)
	{
		*OutPos = m_Text.length();
		return true;
	}
	else
		return false;
}

void Edit::HandleMouseDrag(float X)
{
	size_t EndPos;
	if (MouseHitTest(&EndPos, X))
	{
		if (EndPos > m_MouseDownPos)
		{
			m_SelBegin = m_MouseDownPos;
			m_CursorPos = m_SelEnd = EndPos;
		}
		else
		{
			m_CursorPos = m_SelBegin = EndPos;
			m_SelEnd = m_MouseDownPos;
		}
	}
}

void Edit::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Button == frame::MOUSE_LEFT)
	{
		if (Action == frame::MOUSE_DOWN)
		{
			if (Pos.y >= EDIT_MARGIN && Pos.y <= GetLocalRect().bottom-EDIT_MARGIN)
			{
				size_t BeginPos;
				if (MouseHitTest(&BeginPos, Pos.x))
				{
					// Shift+Click
					if (frame::GetKeyboardKey(VK_SHIFT))
					{
						// Jest zaznaczenie
						if (m_SelEnd > m_SelBegin)
						{
							// Zostaje ta krawêdŸ zaznaczenia, na której nie ma kursora.
							// Druga zostaje przesuniêta w miejsce docelowe.
							if (m_CursorPos <= m_SelBegin)
								m_CursorPos = m_SelBegin = BeginPos;
							else
								m_CursorPos = m_SelEnd = BeginPos;

							if (m_SelEnd < m_SelBegin)
								std::swap(m_SelBegin, m_SelEnd);
						}
						// Nie ma zaznaczenia
						else
						{
							// Zaznaczenie od pozycji kursora do miejsca klikniêcia
							m_SelBegin = m_MouseDownPos = m_CursorPos;
							m_SelEnd = BeginPos;
							if (m_SelEnd < m_SelBegin)
								std::swap(m_SelBegin, m_SelEnd);
						}
					}
					// Zwyk³y Click
					else
					{
						m_MouseDownPos = m_CursorPos = m_SelBegin = m_SelEnd = BeginPos;
					}
					m_MouseDown = true;
					StartDrag(DRAG_DATA_SHARED_PTR(new DragCursor(CT_EDIT)));
					Update(false);
				}
			}
		}
		else if (Action == frame::MOUSE_UP)
		{
			if (m_MouseDown)
			{
				HandleMouseDrag(Pos.x);
				m_MouseDown = false;
				Update(false);
			}
		}
		else if (Action == frame::MOUSE_DBLCLK)
		{
			size_t p;
			if (MouseHitTest(&p, Pos.x))
			{
				size_t p1, p2;
				p1 = GetCtrlLeftPos(p);
				p2 = GetCtrlRightPos(p > 0 ? p-1 : 0);
				m_SelBegin = p1;
				m_SelEnd = m_CursorPos = p2;
				m_MouseDown = false;
				Update(false);
			}
		}
	}
}

void Edit::OnMouseMove(const VEC2 &Pos)
{
	if (m_MouseDown)
	{
		HandleMouseDrag(Pos.x);
		Update(false);
	}
}

bool Edit::OnKeyDown(uint4 Key)
{
	// UWAGA! Tu by³ b³¹d, którego siê naszuka³em.
	// To "&& !frame::GetKeyboardKey(VK_MENU)" jest potrzebne, bo o dziwo po wciœniêciu Alt+A
	// (czyli wpisaniu znaku '¹') VK_CONTROL te¿ jest, oprócz VK_MENU, uznawane za wciœniête.

	switch (Key)
	{
	case VK_LEFT:
		KeyLeft();
		return true;
	case VK_RIGHT:
		KeyRight();
		return true;
	case VK_HOME:
		KeyHome();
		return true;
	case VK_END:
		KeyEnd();
		return true;
	case VK_DELETE:
		KeyDelete();
		return true;
	case VK_RETURN:
		KeyEnter();
		return true;
	case 'X':
		if (frame::GetKeyboardKey(VK_CONTROL) && !frame::GetKeyboardKey(VK_MENU))
		{
			KeyCtrlX();
			return true;
		}
		break;
	case 'C':
		if (frame::GetKeyboardKey(VK_CONTROL) && !frame::GetKeyboardKey(VK_MENU))
		{
			KeyCtrlC();
			return true;
		}
		break;
	case 'V':
		if (frame::GetKeyboardKey(VK_CONTROL) && !frame::GetKeyboardKey(VK_MENU))
		{
			KeyCtrlV();
			return true;
		}
		break;
	case 'A':
		if (frame::GetKeyboardKey(VK_CONTROL) && !frame::GetKeyboardKey(VK_MENU))
		{
			KeyCtrlA();
			return true;
		}
		break;
	case 'Z':
		if (frame::GetKeyboardKey(VK_CONTROL) && !frame::GetKeyboardKey(VK_MENU))
		{
			// Ctrl+Shift+Z
			if (frame::GetKeyboardKey(VK_SHIFT))
				KeyCtrlY();
			// Ctrl+Z
			else
				KeyCtrlZ();
			return true;
		}
		break;
	case 'Y':
		if (frame::GetKeyboardKey(VK_CONTROL) && !frame::GetKeyboardKey(VK_MENU))
		{
			KeyCtrlY();
			return true;
		}
		break;
	}

	return Control::OnKeyDown(Key);
}

bool Edit::OnChar(char Ch)
{
	// Backspace
	if (Ch == '\b')
	{
		KeyBackspace();
		return true;
	}
	// Wprowadzenie znaku
	if ((uint1)Ch >= 32)
	{
		KeyChar(Ch);
		return true;
	}

	return Control::OnChar(Ch);
}

void Edit::OnFocusEnter()
{
	if (m_SelectOnFocus)
	{
		m_SelBegin = 0;
		m_CursorPos = m_SelEnd = m_Text.length();
		m_MouseDown = false;
		Update(false);
	}
}

bool Edit::OnValidateChar(char Ch)
{
	return true;
}

Edit::Edit(CompositeControl *Parent, const RECTF &Rect, bool Password) :
	Control(Parent, true, Rect),
	m_CursorPos(0),
	m_SelBegin(0),
	m_SelEnd(0),
	m_ScrollX(0.0f),
	m_Password(Password),
	m_MaxLength(MAXUINT4),
	m_SelectOnFocus(true),
	m_LastEditTime(0.0f),
	m_MouseDown(false)
{
	SetCursor(g_StandardCursors[CT_EDIT]);
	m_Undo.push_back(STATE(m_Text, m_CursorPos, m_SelBegin, m_SelEnd, ACTION_UNKNOWN));
}

void Edit::SetText(const string &Text)
{
	m_Text = Text;
	m_CursorPos = minmax(0u, m_CursorPos, Text.length());
	m_SelBegin = minmax(0u, m_SelBegin, Text.length());
	m_SelEnd = minmax(0u, m_SelEnd, Text.length());
	m_MouseDown = false;
	m_Undo.clear();
	m_Redo.clear();
	Update(true, ACTION_UNKNOWN);
}

void Edit::SetMaxLength(size_t MaxLength)
{
	m_MaxLength = MaxLength;
	if (m_Text.length() > MaxLength)
	{
		m_Text.erase(MaxLength);
		if (m_CursorPos > m_Text.length())
			m_CursorPos = m_Text.length();
		if (m_SelBegin > m_Text.length())
			m_SelBegin = m_Text.length();
		if (m_SelEnd > m_Text.length())
			m_SelEnd = m_Text.length();
		m_MouseDown = false;
		Update(true, ACTION_UNKNOWN);
	}
}

float Edit::GetTextWidth(size_t Begin, size_t End)
{
	if (Begin == End)
		return 0.0f;

	// Has³o - kropeczki
	if (m_Password)
		return (EDIT_PASSWORD_DOT_SIZE + EDIT_PASSWORD_DOT_SPACE) * (End-Begin);
	else
		return g_Font->GetTextWidth(m_Text, Begin, End, g_FontSize);
}

float Edit::GetClientWidth()
{
	return GetLocalRect().right - EDIT_MARGIN*2.0f;
}

size_t Edit::GetCtrlLeftPos(size_t Pos)
{
	// Pomiñ w lewo dowoln¹ iloœæ odstêpów
	while (Pos > 0)
	{
		if (CharIsWhitespace(m_Text[Pos-1]))
			Pos--;
		else
			break;
	}
	// Pomiñ w lewo dowoln¹ iloœæ znaków alfanumerycznych lub jeden inny znak
	if (Pos > 0)
	{
		if (!CharIsAlphaNumeric(m_Text[Pos-1]))
			Pos--;
		else
		{
			while (Pos > 0)
			{
				if (CharIsAlphaNumeric(m_Text[Pos-1]))
					Pos--;
				else
					break;
			}
		}
	}
	return Pos;
}

size_t Edit::GetCtrlRightPos(size_t Pos)
{
	// Pomiñ w prawo dowoln¹ iloœæ znaków alfanumerycznych lub jeden inny znak
	if (Pos < m_Text.length())
	{
		if (!CharIsAlphaNumeric(m_Text[Pos]))
			Pos++;
		else
		{
			while (Pos < m_Text.length())
			{
				if (CharIsAlphaNumeric(m_Text[Pos]))
					Pos++;
				else
					break;
			}
		}
	}
	// Pomiñ w prawo dowoln¹ iloœæ odstêpów
	while (Pos < m_Text.length())
	{
		if (CharIsWhitespace(m_Text[Pos]))
			Pos++;
		else
			break;
	}
	return Pos;
}

void Edit::KeyLeft()
{
	// Shift
	if (frame::GetKeyboardKey(VK_SHIFT))
	{
		// Ctrl+Shift+Lewo
		if (frame::GetKeyboardKey(VK_CONTROL))
		{
			// Jest zaznaczenie
			if (m_SelEnd > m_SelBegin)
			{
				// W praktyce powinno byæ zawsze m_CursorPos == m_Begin lub m_CursorPos == m_SelEnd
				// ale kto wie... wiêc na wszelki wypadek nie sprawdzam tylko obs³uguje te¿ inne wypadki.

				// Kursor jest na na lewej krawêdzi zaznaczenia - rozszerz zaznaczenie w lewo
				if (m_CursorPos <= m_SelBegin)
				{
					if (m_CursorPos > 0)
						m_CursorPos = m_SelBegin = GetCtrlLeftPos(m_CursorPos);
					// Jeœli siê zamieni³y kolejnoœci¹ pocz¹tek i koniec zaznaczenia
					if (m_SelEnd < m_SelBegin)
						std::swap(m_SelBegin, m_SelEnd);
				}
				// Kursor jest na prawej krawêdzi zaznaczenia - rozszerz zmniejsz zaznaczenie w lewo
				else
				{
					if (m_CursorPos > 0) // TO powinno byæ spe³nione zawsze
						m_CursorPos = m_SelEnd = GetCtrlLeftPos(m_CursorPos);
					// Jeœli siê zamieni³y kolejnoœci¹ pocz¹tek i koniec zaznaczenia
					if (m_SelEnd < m_SelBegin)
						std::swap(m_SelBegin, m_SelEnd);
				}
			}
			// Nie ma zaznaczenia
			else
			{
				// Zaznacz znak na lewo od kursora i kursor w lewo
				if (m_CursorPos > 0)
				{
					m_SelEnd = m_CursorPos;
					m_CursorPos = GetCtrlLeftPos(m_CursorPos);
					m_SelBegin = m_CursorPos;
				}
			}
		}
		// Shift+Lewo
		else
		{
			// Jest zaznaczenie
			if (m_SelEnd > m_SelBegin)
			{
				// W praktyce powinno byæ zawsze m_CursorPos == m_Begin lub m_CursorPos == m_SelEnd
				// ale kto wie... wiêc na wszelki wypadek nie sprawdzam tylko obs³uguje te¿ inne wypadki.

				// Kursor jest na na lewej krawêdzi zaznaczenia - rozszerz zaznaczenie w lewo
				if (m_CursorPos <= m_SelBegin)
				{
					if (m_CursorPos > 0)
						m_CursorPos = m_SelBegin = m_CursorPos-1;
				}
				// Kursor jest na prawej krawêdzi zaznaczenia - zmniejsz zaznaczenie w lewo
				else
				{
					if (m_CursorPos > 0) // TO powinno byæ spe³nione zawsze
						m_CursorPos = m_SelEnd = m_CursorPos-1;
				}
			}
			// Nie ma zaznaczenia
			else
			{
				// Zaznacz znak na lewo od kursora i kursor w lewo
				if (m_CursorPos > 0)
				{
					m_SelEnd = m_CursorPos;
					m_CursorPos--;
					m_SelBegin = m_CursorPos;
				}
			}
		}
	}
	// Nie Shift
	else
	{
		// Ctrl+Lewo
		if (frame::GetKeyboardKey(VK_CONTROL))
		{
			// Jest zaznaczenie
			if (m_SelEnd > m_SelBegin)
			{
				// Kursor na lewo od zaznaczenia
				m_CursorPos = m_SelBegin;
				// Zaznaczenia nie ma
				m_SelBegin = m_SelEnd = 0;
				// I dalej tak jakby nigdy nic...
			}
			m_CursorPos = GetCtrlLeftPos(m_CursorPos);
		}
		// Zwyk³e Lewo
		else
		{
			// Jest zaznaczenie
			if (m_SelEnd > m_SelBegin)
			{
				// Kursor na lewo od zaznacznia
				m_CursorPos = m_SelBegin;
				// Zaznaczenia nie bêdzie
				m_SelBegin = m_SelEnd = 0;
			}
			// Nie ma zaznaczenia
			else
			{
				// Kursor w lewo
				if (m_CursorPos > 0)
					m_CursorPos--;
			}
		}
	}
	m_MouseDown = false;
	Update(false);
}

void Edit::KeyRight()
{
	// Shift
	if (frame::GetKeyboardKey(VK_SHIFT))
	{
		// Ctrl+Shift+Prawo
		if (frame::GetKeyboardKey(VK_CONTROL))
		{
			// Jest zaznaczenie
			if (m_SelEnd > m_SelBegin)
			{
				// W praktyce powinno byæ zawsze m_CursorPos == m_Begin lub m_CursorPos == m_SelEnd
				// ale kto wie... wiêc na wszelki wypadek nie sprawdzam tylko obs³uguje te¿ inne wypadki.

				// Kursor jest na na lewej krawêdzi zaznaczenia - zmniejsz zaznaczenie w prawo
				if (m_CursorPos <= m_SelBegin)
				{
					if (m_CursorPos < m_Text.length())
						m_CursorPos = m_SelBegin = GetCtrlRightPos(m_CursorPos);
					// Jeœli siê zamieni³y kolejnoœci¹ pocz¹tek i koniec zaznaczenia
					if (m_SelEnd < m_SelBegin)
						std::swap(m_SelBegin, m_SelEnd);
				}
				// Kursor jest na prawej krawêdzi zaznaczenia - rozszerz zaznaczenie w prawo
				else
				{
					if (m_CursorPos < m_Text.length()) // To powinno byæ spe³nione zawsze
						m_CursorPos = m_SelEnd = GetCtrlRightPos(m_CursorPos);
					// Jeœli siê zamieni³y kolejnoœci¹ pocz¹tek i koniec zaznaczenia
					if (m_SelEnd < m_SelBegin)
						std::swap(m_SelBegin, m_SelEnd);
				}
			}
			// Nie ma zaznaczenia
			else
			{
				// Zaznacz znak na prawo od kursora i kursor w prawo
				if (m_CursorPos < m_Text.length())
				{
					m_SelBegin = m_CursorPos;
					m_CursorPos = GetCtrlRightPos(m_CursorPos);
					m_SelEnd = m_CursorPos;
				}
			}
		}
		// Shift+Prawo
		else
		{
			// Jest zaznaczenie
			if (m_SelEnd > m_SelBegin)
			{
				// W praktyce powinno byæ zawsze m_CursorPos == m_Begin lub m_CursorPos == m_SelEnd
				// ale kto wie... wiêc na wszelki wypadek nie sprawdzam tylko obs³uguje te¿ inne wypadki.

				// Kursor jest na na lewej krawêdzi zaznaczenia - zmniejszy zaznaczenie w prawo
				if (m_CursorPos <= m_SelBegin)
				{
					if (m_CursorPos < m_Text.length())
						m_CursorPos = m_SelBegin = m_CursorPos+1;
				}
				// Kursor jest na prawej krawêdzi zaznaczenia - rozszerz zaznaczenie w prawo
				else
				{
					if (m_CursorPos < m_Text.length()) // To powinno byæ spe³nione zawsze
						m_CursorPos = m_SelEnd = m_CursorPos+1;
				}
			}
			// Nie ma zaznaczenia
			else
			{
				// Zaznacz znak na prawo od kursora i kursor w prawo
				if (m_CursorPos < m_Text.length())
				{
					m_SelBegin = m_CursorPos;
					m_CursorPos++;
					m_SelEnd = m_CursorPos;
				}
			}
		}
	}
	// Nie Shift
	else
	{
		// Ctrl+Prawo
		if (frame::GetKeyboardKey(VK_CONTROL))
		{
			// Jest zaznaczenie
			if (m_SelEnd > m_SelBegin)
			{
				// Kursor na prawo od zaznaczenia
				m_CursorPos = m_SelEnd;
				// Zaznaczenia nie ma
				m_SelBegin = m_SelEnd = 0;
				// I dalej tak jakby nigdy nic...
			}
			m_CursorPos = GetCtrlRightPos(m_CursorPos);
		}
		// Zwyk³e Prawo
		else
		{
			// Jest zaznaczenie
			if (m_SelEnd > m_SelBegin)
			{
				// Kursor na prawo od zaznaczenia
				m_CursorPos = m_SelEnd;
				// Zaznaczenia nie bêdzie
				m_SelBegin = m_SelEnd = 0;
			}
			// Nie ma zaznaczenia
			else
			{
				// Kursor w prawo
				if (m_CursorPos < m_Text.length())
					m_CursorPos++;
			}
		}
	}
	m_MouseDown = false;
	Update(false);
}

void Edit::KeyHome()
{
	// Shift+Home
	if (frame::GetKeyboardKey(VK_SHIFT))
	{
		// Jest zaznaczenie
		if (m_SelEnd > m_SelBegin)
		{
			// W praktyce powinno byæ zawsze m_CursorPos == m_Begin lub m_CursorPos == m_SelEnd
			// ale kto wie... wiêc na wszelki wypadek nie sprawdzam tylko obs³uguje te¿ inne wypadki.

			// Kursor jest na na lewej krawêdzi zaznaczenia - rozci¹gnij zaznaczenie w lewo do pocz¹tku
			if (m_CursorPos <= m_SelBegin)
			{
				m_SelBegin = m_CursorPos = 0;
			}
			// Kursor jest na prawej krawêdzi zaznaczenia - odwróæ zaznaczenie od pocz¹tku do bie¿¹cego pocz¹tku
			else
			{
				m_SelEnd = m_SelBegin;
				m_SelBegin = m_CursorPos = 0;
			}
		}
		// Nie ma zaznaczenia
		else
		{
			// Zaznaczenie bêdzie od pocz¹tku do tego miejsca
			m_SelBegin = 0;
			m_SelEnd = m_CursorPos;
			// Kursor na pocz¹tek
			m_CursorPos = 0;
		}
	}
	// Zwyk³y Home
	else
	{
		// Kursor na pocz¹tek
		m_CursorPos = 0;
		// Zaznaczenia nie bêdzie
		m_SelBegin = m_SelEnd = 0;
	}
	m_MouseDown = false;
	Update(false);
}

void Edit::KeyEnd()
{
	// Shift+End
	if (frame::GetKeyboardKey(VK_SHIFT))
	{
		// Jest zaznaczenie
		if (m_SelEnd > m_SelBegin)
		{
			// W praktyce powinno byæ zawsze m_CursorPos == m_Begin lub m_CursorPos == m_SelEnd
			// ale kto wie... wiêc na wszelki wypadek nie sprawdzam tylko obs³uguje te¿ inne wypadki.

			// Kursor jest na na lewej krawêdzi zaznaczenia - odwróæ zaznaczenie od bie¿¹cego koñca do koñca tekstu
			if (m_CursorPos <= m_SelBegin)
			{
				m_SelBegin = m_SelEnd;
				m_SelEnd = m_CursorPos = m_Text.length();
			}
			// Kursor jest na prawej krawêdzi zaznaczenia - rozci¹gnij zaznaczenie w prawo do koñca
			else
			{
				m_SelEnd = m_CursorPos = m_Text.length();
			}
		}
		// Nie ma zaznaczenia
		else
		{
			// Zaznaczenie bêdzie od tego miejsca do koñca
			// Kursor na koniec
			m_SelBegin = m_CursorPos;
			m_SelEnd = m_CursorPos = m_Text.length();
		}
	}
	// Zwyk³y End
	else
	{
		// Kursor na koniec
		m_CursorPos = m_Text.length();
		// Zaznaczenia nie bêdzie
		m_SelBegin = m_SelEnd = 0;
	}
	m_MouseDown = false;
	Update(false);
}

void Edit::KeyDelete()
{
	// Jest zaznaczenie
	if (m_SelEnd > m_SelBegin)
	{
		// Usuñ zaznaczenie
		m_Text.erase(m_SelBegin, m_SelEnd-m_SelBegin);
		// Kursor w miejscu usuniêcia
		m_CursorPos = m_SelBegin;
		// Nie ma zaznaczenia
		m_SelBegin = m_SelEnd = 0;
	}
	// Nie ma zaznaczenia
	else
	{
		// Usuñ znak za kursorem
		if (m_CursorPos < m_Text.length())
			m_Text.erase(m_CursorPos, 1);
	}
	m_MouseDown = false;
	Update(true, ACTION_DELETE);
}

void Edit::KeyEnter()
{
	if (OnEnter)
		OnEnter(this);
}

void Edit::KeyBackspace()
{
	// Jest zaznaczenie
	if (m_SelEnd > m_SelBegin)
	{
		// Usuñ zaznaczenie
		m_Text.erase(m_SelBegin, m_SelEnd-m_SelBegin);
		// Kursor w miejscu usuniêcia
		m_CursorPos = m_SelBegin;
	}
	// Nie ma zaznaczenia
	else
	{
		// Usuñ znak przed kursorem
		if (m_CursorPos > 0)
		{
			m_Text.erase(m_CursorPos-1, 1);
			// Kursor w lewo
			m_CursorPos--;
			// By³a zmiana tekstu
		}
	}
	// Nie ma zaznaczenia
	m_SelBegin = m_SelEnd = 0;
	m_MouseDown = false;
	Update(true, ACTION_DELETE);
}

void Edit::KeyChar(char Ch)
{
	if (OnValidateChar(Ch))
	{
		// By³o zaznaczenie
		if (m_SelEnd > m_SelBegin)
		{
			// Tu d³ugoœci ju¿ nie pilnujemy -tekst na pewno siê nie wyd³u¿y.
			// Usuñ zaznaczenie
			m_Text.erase(m_SelBegin, m_SelEnd-m_SelBegin);
			// Kursor w miejscu usuniêcia
			m_CursorPos = m_SelBegin;
			// Nie ma zaznaczenia
			m_SelBegin = m_SelEnd = 0;
			// Wstaw znak w miejscu kursora
			m_Text.insert(m_CursorPos, CharToStrR(Ch));
			// Kursor w prawo
			m_CursorPos++;
		}
		// Nie by³o zaznaczenia
		else
		{
			if (m_Text.length() < m_MaxLength)
			{
				// Wstaw znak w miejscu kursora
				m_Text.insert(m_CursorPos, CharToStrR(Ch));
				// Kursor w prawo
				m_CursorPos++;
			}
		}
		m_MouseDown = false;
		Update(true, ACTION_TYPE);
	}
}

void Edit::KeyCtrlX()
{
	// Nie mo¿na wycinaæ has³a do schowka
	if (m_Password)
		return;

	// Coœ jest zaznaczone
	if (m_SelEnd > m_SelBegin)
	{
		// Skopiuj to coœ do schowka
		if (SystemClipboard::SetText(m_Text.substr(m_SelBegin, m_SelEnd)))
		{
			// Skasuj ten tekst
			m_Text.erase(m_SelBegin, m_SelEnd-m_SelBegin);
			// Kursor zostaje w miejscu pocz¹tku zaznaczenia
			m_CursorPos = m_SelBegin;
			// Zaznaczenia nie ma
			m_SelBegin = m_SelEnd = 0;
			// Zosta³o zmienione
			m_MouseDown = false;
			Update(true);
		}
	}
}

void Edit::KeyCtrlC()
{
	// Nie mo¿na kopiowaæ has³a do schowka
	if (m_Password)
		return;

	// Coœ jest zaznaczone
	if (m_SelEnd > m_SelBegin)
	{
		// Skopiuj to coœ do schowka i tyle
		SystemClipboard::SetText(m_Text.substr(m_SelBegin, m_SelEnd-m_SelBegin));
	}
}

void Edit::KeyCtrlV()
{
	// Pobierz tekst ze schowka (tylko pierwsza linia)
	string ClipText;
	if (SystemClipboard::GetFirstLineText(&ClipText))
	{
		// Walidacja wszystkich znaków
		for (size_t i = 0; i < ClipText.length(); i++)
			if (!OnValidateChar(ClipText[i]))
				return;

		// By³o zaznaczenie
		if (m_SelEnd > m_SelBegin)
		{
			// Limit d³ugoœci
			if (m_Text.length()-(m_SelEnd-m_SelBegin)+ClipText.length() <= m_MaxLength)
			{
				// Usuñ zaznaczenie
				m_Text.erase(m_SelBegin, m_SelEnd-m_SelBegin);
				// Kursor w miejscu usuniêcia
				m_CursorPos = m_SelBegin;
				// Nie ma zaznaczenia
				m_SelBegin = m_SelEnd = 0;
				// Wstaw ³añcuch w miejscu kursora
				m_Text.insert(m_CursorPos, ClipText);
				// Kursor w prawo
				m_CursorPos += ClipText.length();
			}
		}
		// Nie by³o zaznaczenia
		else
		{
			// Limit d³ugoœci
			if (m_Text.length()+ClipText.length() <= m_MaxLength)
			{
				// Wstaw ³añcuch w miejscu kursora
				m_Text.insert(m_CursorPos, ClipText);
				// Kursor w prawo
				m_CursorPos += ClipText.length();
			}
		}
		m_MouseDown = false;
		Update(true);
	}
}

void Edit::KeyCtrlA()
{
	m_SelBegin = 0;
	m_SelEnd = m_CursorPos = m_Text.length();
	Update(false);
}

void Edit::KeyCtrlZ()
{
	if (m_Undo.size() >= 2)
	{
		// Zapamiêtaj stary bie¿¹cy stan do Redo
		m_Redo.push_back(STATE(m_Text, m_CursorPos, m_SelBegin, m_SelEnd, ACTION_UNDO));

		// Przywróæ stan z listy Undo (przedostatni, bo ostatni jest równy bie¿¹cemu)
		m_Undo.pop_back();
		m_Text = m_Undo.back().Text;
		m_CursorPos = m_Undo.back().CursorPos;
		m_SelBegin = m_Undo.back().SelBegin;
		m_SelEnd = m_Undo.back().SelEnd;
		m_MouseDown = false;
		Update(true, ACTION_UNDO);
	}
}

void Edit::KeyCtrlY()
{
	if (!m_Redo.empty())
	{
		// Przywróæ stan z listy Redo
		m_Text = m_Redo.back().Text;
		m_CursorPos = m_Redo.back().CursorPos;
		m_SelBegin = m_Redo.back().SelBegin;
		m_SelEnd = m_Redo.back().SelEnd;
		m_MouseDown = false;
		m_Redo.pop_back();
		Update(true, ACTION_REDO);
	}
}

void Edit::Update(bool TextChanged, ACTION Action)
{
	if (TextChanged)
		m_TextWidth = GetTextWidth();

	if (m_Password)
	{
		m_SelBeginPos = m_SelBegin * (EDIT_PASSWORD_DOT_SIZE+EDIT_PASSWORD_DOT_SPACE);
		m_SelEndPos = m_SelEnd * (EDIT_PASSWORD_DOT_SIZE+EDIT_PASSWORD_DOT_SPACE);
	}
	else
	{
		m_SelBeginPos = g_Font->GetTextWidth(m_Text, 0, m_SelBegin, g_FontSize);
		m_SelEndPos = m_SelBeginPos + g_Font->GetTextWidth(m_Text, m_SelBegin, m_SelEnd, g_FontSize);
	}

	m_CursorBeginPos = GetTextWidth(0, m_CursorPos);
	m_CursorEndPos = m_CursorBeginPos + EDIT_CURSOR_WIDTH;

	// == Aktualizacja ScrollX ==
	// ¯eby tekst by³ nie za daleko w t¹ ani w t¹ i ¿eby kursor by³ widoczny.

	float ClientWidth = GetClientWidth();

	// Do granic tekstu
	m_ScrollX = std::min(m_ScrollX, m_TextWidth-ClientWidth);
	m_ScrollX = std::max(m_ScrollX, 0.0f);

	// ¯eby by³o widaæ kursor
	// YEAH, ale siê musia³em namyœleæ nad tym wzorem - nareszcie dzia³a :D
	m_ScrollX = std::min(m_ScrollX, m_CursorBeginPos);
	m_ScrollX = std::max(m_ScrollX, m_CursorEndPos-ClientWidth);

	// == Aktualizacja czasu ostatniej edycji - dla migania kursora ==
	m_LastEditTime = frame::Timer1.GetTime();

	// == Sygna³ o zmianie tekstu ==
	if (TextChanged)
	{
		if (OnChange)
			OnChange(this);
	}

	// Zapamiêtanie stanu dla UNDO
	if (Action != ACTION_UNDO)
	{
		if (TextChanged)
		{
			// Uaktualnij dane ostatniego stanu
			if (!m_Undo.empty() && m_Undo.back().Action == Action &&
				Action != ACTION_UNKNOWN && Action != ACTION_UNDO && Action != ACTION_REDO)
			{
				m_Undo.back().Text = m_Text;
				m_Undo.back().CursorPos = m_CursorPos;
				m_Undo.back().SelBegin = m_SelBegin;
				m_Undo.back().SelEnd = m_SelEnd;
			}
			// Zapamiêtaj nowy bie¿¹cy stan
			else
			{
				m_Undo.push_back(STATE(m_Text, m_CursorPos, m_SelBegin, m_SelEnd, Action));
				if (m_Undo.size() > EDIT_UNDO_LIMIT)
					m_Undo.pop_front();
			}
		}
		else
		{
			// Uaktualnij pozycje dla ostatniego stanu
			if (!m_Undo.empty())
			{
				m_Undo.back().CursorPos = m_CursorPos;
				m_Undo.back().SelBegin = m_SelBegin;
				m_Undo.back().SelEnd = m_SelEnd;
			}
		}

		// Wyczyœæ listê REDO
		if (Action != ACTION_UNDO && Action != ACTION_REDO)
			m_Redo.clear();
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa List

void List::OnRectChange()
{
	Update();
}

void List::OnDraw(const VEC2 &Translation)
{

	DrawBegin();

	// T³o
	DrawBackground();

	// Nag³ówek
	gfx2d::g_Canvas->PushClipRect(m_HeaderRect);
	DrawHeader(m_HeaderRect);
	gfx2d::g_Canvas->PopClipRect();

	// Elementy
	gfx2d::g_Canvas->PushClipRect(m_ClientRect);
	RECTF R;
	float ItemHeight = GetItemHeight();
	size_t PassCount = GetDrawPassCount();
	bool Selected;
	for (size_t Pass = 0; Pass < PassCount; Pass++)
	{
		DrawPassBegin(Pass);
		R = m_ClientRect;
		R.top = R.top - frac(m_ScrollBar->GetPos()/ItemHeight)*ItemHeight;
		if (m_Model)
		{
			for (size_t i = static_cast<size_t>(m_ScrollBar->GetPos()/ItemHeight); i < m_Model->GuiList_GetItemCount(); i++)
			{
				R.bottom = R.top + ItemHeight;
				Selected = (i == GetModel()->GuiList_GetSelIndex());
				DrawItem(Pass, R, i);
				R.top = R.bottom;
				if (R.top > m_ClientRect.bottom)
					break;
			}
		}
		DrawPassEnd(Pass);
	}
	gfx2d::g_Canvas->PopClipRect();

	DrawEnd();
}

void List::OnMouseMove(const VEC2 &Pos)
{
	if (m_MouseDown)
		HandleMouseSelect(Pos);
}

void List::HandleMouseSelect(const VEC2 &Pos)
{
	if (m_Model == 0) return;

	size_t Index = HitTestItem(Pos.y - m_ClientRect.top);

	if (Index >= m_Model->GuiList_GetItemCount())
		Index = m_Model->GuiList_GetItemCount()-1;

	if (Index != m_Model->GuiList_GetSelIndex())
	{
		// Ustaw go na aktywny
		m_Model->GuiList_SetSelIndex(Index);
		// Zapewnij ¿e jest ca³y widoczny
		ScrollTo(Index);
		// Zmieniono zaznaczenie
		if (OnSelChange) OnSelChange(this);
	}
}

void List::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Button == frame::MOUSE_LEFT && Action == frame::MOUSE_DOWN)
	{
		if (PointInRect(Pos, m_ClientRect))
		{
			HandleMouseSelect(Pos);
			m_MouseDown = true;
			StartDrag();
		}
		else if (PointInRect(Pos, m_HeaderRect))
		{
			if (OnHeaderClick) OnHeaderClick(this, Pos);
		}
	}
	else if (Button == frame::MOUSE_LEFT && Action == frame::MOUSE_DBLCLK)
	{
		// Uwaga! Hack! Naprawia b³¹d, którego d³ugo siê naszuka³em.
		// Otó¿ podczas dwukrotnego klikniêcia nastêpuj¹ dla listy kolejno - DOWN, UP, DBLCLK, UP.
		// W reakcji na DOWN kontrolka robi StartDrag.
		// Potem w reakcji na ka¿d¹ inn¹ rzecz syste GUI powinien anulowaæ przeci¹ganie.
		// Problem w tym, ¿e zdarzenie DBLCLK nastêpuje dla systemu GUI wczeœniej,
		// ni¿ komunikat DOWN dociera do kontrolki listy.
		CancelDrag();
		if (PointInRect(Pos, m_ClientRect))
		{
			if (OnItemDoubleClick) OnItemDoubleClick(this, Pos, HitTestItem(Pos));
		}
		else if (PointInRect(Pos, m_HeaderRect))
		{
			if (OnHeaderDoubleClick) OnHeaderDoubleClick(this, Pos);
		}
	}
	else if (Button == frame::MOUSE_LEFT && Action == frame::MOUSE_UP)
	{
		// Nie wiem dlaczego to OnMouseButton z MOUSE_UP nie wywo³uje siê, kiedy kursor wyjedzie i zostanie
		// puszczony poza obszarem kontrolki mimo, ¿e jest przecie¿ StartDrag.
		if (m_MouseDown)
		{
			HandleMouseSelect(Pos);
			m_MouseDown = false;
			if (OnItemClick) OnItemClick(this, Pos, HitTestItem(Pos));
		}
	}
	else if (Button == frame::MOUSE_RIGHT && Action == frame::MOUSE_DOWN)
	{
		if (PointInRect(Pos, m_ClientRect))
		{
			if (OnItemRightClick) OnItemRightClick(this, Pos, HitTestItem(Pos));
		}
		else if (PointInRect(Pos, m_HeaderRect))
		{
			if (OnHeaderRightClick) OnHeaderRightClick(this, Pos);
		}
	}
}

void List::OnMouseWheel(const VEC2 &Pos, float Delta)
{
	m_ScrollBar->SetPos(m_ScrollBar->GetPos()-Delta*m_ScrollBar->GetStep());
	ScrollBarScroll(this);
}

bool List::OnKeyDown(uint4 Key)
{
	switch (Key)
	{
	case VK_UP:
		KeyUp();
		return true;
	case VK_DOWN:
		KeyDown();
		return true;
	case VK_PRIOR:
		KeyPgup();
		return true;
	case VK_NEXT:
		KeyPgdn();
		return true;
	case VK_HOME:
		KeyHome();
		return true;
	case VK_END:
		KeyEnd();
		return true;
	case VK_DELETE:
		KeyDelete();
		return true;
	case VK_RETURN:
		KeyEnter();
		return true;
	case VK_ESCAPE:
		KeyEscape();
		return true;
	}

	return CompositeControl::OnKeyDown(Key);
}

void List::KeyDown()
{
	if (m_Model == 0) return;
	if (m_Model->GuiList_GetItemCount() == 0) return;

	// Pobierz aktywny element
	size_t FocusIndex = m_Model->GuiList_GetSelIndex();
	// Jeœli ¿aden nie by³ aktywny, niech bêdzie ¿e by³ przed-pierwszy
	if (FocusIndex >= m_Model->GuiList_GetItemCount())
		FocusIndex = MAXUINT4;
	// Jeœli to nie ostatni
	if (FocusIndex != m_Model->GuiList_GetItemCount()-1)
	{
		// Indeks na nastêpny
		FocusIndex++;
		// Ustaw go na aktywny
		m_Model->GuiList_SetSelIndex(FocusIndex);
		// Przewiñ na ten element
		ScrollTo(FocusIndex);
		// Zmieniono zaznaczenie
		if (OnSelChange) OnSelChange(this);
	}
}

void List::KeyUp()
{
	if (m_Model == 0) return;
	if (m_Model->GuiList_GetItemCount() == 0) return;

	// Pobierz aktywny element
	size_t FocusIndex = m_Model->GuiList_GetSelIndex();
	// Jeœli ¿aden nie by³ aktywny, niech bêdzie ¿e by³ za-ostatni
	if (FocusIndex >= m_Model->GuiList_GetItemCount())
		FocusIndex = m_Model->GuiList_GetItemCount();
	// Jeœli to nie pierwszy
	if (FocusIndex > 0)
	{
		// Indeks na poprzedni
		FocusIndex--;
		// Ustaw go na aktywny
		m_Model->GuiList_SetSelIndex(FocusIndex);
		// Przewiñ na ten element
		ScrollTo(FocusIndex);
		// Zmieniono zaznaczenie
		if (OnSelChange) OnSelChange(this);
	}
}

void List::KeyPgup()
{
	if (m_Model == 0) return;
	if (m_Model->GuiList_GetItemCount() == 0) return;

	size_t PageSize = static_cast<size_t>(m_ClientRect.Height() / GetItemHeight());
	size_t OldIndex = m_Model->GuiList_GetSelIndex();
	size_t NewIndex;
	// Jeœli ¿aden nie by³ aktywny
	if (OldIndex >= m_Model->GuiList_GetItemCount())
		NewIndex = m_Model->GuiList_GetItemCount()-1;
	else
	{
		if (OldIndex >= PageSize)
			NewIndex = OldIndex - PageSize;
		else
			NewIndex = 0;
	}

	if (NewIndex != OldIndex)
	{
		m_Model->GuiList_SetSelIndex(NewIndex);
		ScrollTo(NewIndex);
		if (OnSelChange) OnSelChange(this);
	}
}

void List::KeyPgdn()
{
	if (m_Model == 0) return;
	if (m_Model->GuiList_GetItemCount() == 0) return;

	size_t PageSize = static_cast<size_t>(m_ClientRect.Height() / GetItemHeight());
	size_t OldIndex = m_Model->GuiList_GetSelIndex();
	size_t NewIndex;
	// Jeœli ¿aden nie by³ aktywny
	if (OldIndex >= m_Model->GuiList_GetItemCount())
		NewIndex = 0;
	else
	{
		if (OldIndex+PageSize < m_Model->GuiList_GetItemCount())
			NewIndex = OldIndex + PageSize;
		else
			NewIndex = m_Model->GuiList_GetItemCount()-1;
	}

	if (NewIndex != OldIndex)
	{
		m_Model->GuiList_SetSelIndex(NewIndex);
		ScrollTo(NewIndex);
		if (OnSelChange) OnSelChange(this);
	}
}

void List::KeyHome()
{
	if (m_Model == 0) return;
	if (m_Model->GuiList_GetItemCount() == 0) return;

	if (m_Model->GuiList_GetSelIndex() != 0)
	{
		m_Model->GuiList_SetSelIndex(0);
		ScrollTo(0);
		if (OnSelChange) OnSelChange(this);
	}
}

void List::KeyEnd()
{
	if (m_Model == 0) return;
	if (m_Model->GuiList_GetItemCount() == 0) return;

	size_t NewIndex = m_Model->GuiList_GetItemCount()-1;
	if (m_Model->GuiList_GetSelIndex() != NewIndex)
	{
		m_Model->GuiList_SetSelIndex(NewIndex);
		ScrollTo(NewIndex);
		if (OnSelChange) OnSelChange(this);
	}
}

void List::KeyDelete()
{
	if (OnKeyDelete) OnKeyDelete(this);
}

void List::KeyEscape()
{
	if (OnKeyEscape) OnKeyEscape(this);
}

void List::KeyEnter()
{
	if (OnKeyEnter) OnKeyEnter(this);
}

bool List::OnKeyUp(uint4 Key)
{
	return CompositeControl::OnKeyUp(Key);
}

bool List::OnChar(char Ch)
{
	return CompositeControl::OnChar(Ch);
}

void List::GetClientRect(RECTF *Out)
{
	*Out = GetLocalRect();
}

List::List(CompositeControl *Parent, const RECTF &Rect, IListModel *Model) :
	CompositeControl(Parent, true, Rect),
	m_Model(Model),
	m_ScrollBar(0),
	m_MouseDown(false)
{
	m_ScrollBar = new ScrollBar(this, RECTF::ZERO, O_VERT, false);
	m_ScrollBar->OnScroll.bind(this, &List::ScrollBarScroll);
}

void List::Ctor()
{
	Update();
}

List::~List()
{
}

void List::SetModel(IListModel *NewModel)
{
	m_Model = NewModel;
	Update();
}

void List::Update()
{
	RECTF R = GetLocalRect();
	R.Extend(-GetMargin());
	float HeaderHeight = GetHeaderHeight();
	float ScrollBarSize = GetScrollBarSize();
	float ItemHeight = GetItemHeight();
	size_t ItemCount = (m_Model ? m_Model->GuiList_GetItemCount() : 0);

	// ScrollBar.Rect
	RECTF ScrollBarRect = R;
	ScrollBarRect.left = ScrollBarRect.right - ScrollBarSize;
	ScrollBarRect.top += HeaderHeight;
	m_ScrollBar->SetRect(ScrollBarRect);

	// ClientRect
	m_ClientRect = R;
	m_ClientRect.right -= ScrollBarSize;
	m_ClientRect.top += HeaderHeight;

	// HeaderRect
	m_HeaderRect = R;
	m_HeaderRect.bottom = m_HeaderRect.top + HeaderHeight;

	// ScrollBar.Doc
	m_ScrollBar->SetDoc(ItemCount*ItemHeight / m_ScrollBar->GetRect().Height());

	// ScrollBar.Min, Max, Range
	m_ScrollBar->SetRange(0.0f, std::max(0.0f, ItemCount*ItemHeight - m_ClientRect.Height()));
	m_ScrollBar->SetStep(ItemHeight);

	CheckScrollY();
}

void List::ScrollBarScroll(Control *c)
{
	CheckScrollY();
}

void List::CheckScrollY()
{
	size_t ItemCount = (m_Model ? m_Model->GuiList_GetItemCount() : 0);
	float ItemHeight = GetItemHeight();
	float MaxScrollY = ItemCount*ItemHeight - m_ClientRect.Height();

	if (m_ScrollBar->GetPos() < 0.0f)
		m_ScrollBar->SetPos(0.0f);
	if (m_ScrollBar->GetPos() > MaxScrollY)
		m_ScrollBar->SetPos(MaxScrollY);
}

size_t List::HitTestItem(float Y)
{
	if (m_Model == 0)
		return MAXUINT4;
	if (Y + m_ScrollBar->GetPos() < 0.0f)
		return 0;
	
	return static_cast<size_t>( (Y + m_ScrollBar->GetPos()) / GetItemHeight() );
}

size_t List::HitTestItem(const VEC2 &Pos)
{
	if (!PointInRect(Pos, m_ClientRect))
		return MAXUINT4;
	return HitTestItem(Pos.y-m_ClientRect.top);
}

void List::ScrollTo(size_t Index)
{
	if (m_Model == 0) return;
	Index = std::min(Index, m_Model->GuiList_GetItemCount()-1);
	float ItemHeight = GetItemHeight();

	float MinScroll = Index * ItemHeight;
	float MaxScroll = (Index+1)*ItemHeight - m_ClientRect.Height();

	if (m_ScrollBar->GetPos() < MaxScroll)
		m_ScrollBar->SetPos(MaxScroll);
	if (m_ScrollBar->GetPos() > MinScroll)
		m_ScrollBar->SetPos(MinScroll);
	
	CheckScrollY();
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa StringList

float StringList::GetMargin()
{
	return 2.0f;
}

float StringList::GetHeaderHeight()
{
	return 0.0f;
}

float StringList::GetScrollBarSize()
{
	return SCROLLBAR_SIZE;
}

float StringList::GetItemHeight()
{
	return g_FontSize + 2.0f * 2.0f;
}

void StringList::DrawBackground()
{
	gfx2d::g_Canvas->SetFont(gui::g_Font);
	gfx2d::g_Canvas->SetSprite(gui::g_Sprite);

	COLOR WhiteColor = GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY;
	gfx2d::g_Canvas->SetColor(WhiteColor);

	gfx2d::g_Canvas->DrawRect(GetLocalRect(), gfx2d::ComplexElement(3));
}

void StringList::DrawHeader(const RECTF &Rect)
{
}

void StringList::DrawItem(size_t Pass, const RECTF &Rect, size_t Index)
{
	gfx2d::g_Canvas->SetFont(gui::g_Font);
	gfx2d::g_Canvas->SetSprite(gui::g_Sprite);
	COLOR WhiteColor = GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY;
	bool Selected = Index == GetModel().GuiList_GetSelIndex();

	if (Pass == 0)
	{
		// Zaznaczenie
		if (Selected)
			gfx2d::g_Canvas->SetColor(g_SelBkgColor);
		else if (Index % 2)
			gfx2d::g_Canvas->SetColor(g_BkgColor2);
		else
			gfx2d::g_Canvas->SetColor(g_BkgColor);
		gfx2d::g_Canvas->DrawRect(Rect, 48);
	}
	else if (Pass == 1)
	{
		// Tekst
		if (Selected)
			gfx2d::g_Canvas->SetColor(g_SelFontColor);
		else
			gfx2d::g_Canvas->SetColor(g_FontColor);
		gfx2d::g_Canvas->DrawText_(Rect.left+4.0f, Rect.GetCenterY(), GetModel().Get(Index), gui::g_FontSize,
			res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
		// Focus
		if (IsFocused() && Selected)
		{
			gfx2d::g_Canvas->SetColor(g_FocusColor);
			gfx2d::g_Canvas->DrawRect(Rect, gfx2d::ComplexElement(2));
		}
	}
}

void StringList::DrawSingleItem(const RECTF &Rect, size_t Index)
{
	if (Index < GetModel().GuiList_GetItemCount())
	{
		gfx2d::g_Canvas->SetFont(gui::g_Font);
		gfx2d::g_Canvas->SetColor(gui::g_FontColor);
		gfx2d::g_Canvas->DrawText_(Rect.left+4.0f, Rect.GetCenterY(), GetModel().Get(Index), gui::g_FontSize,
			res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE, 1e7f);
	}
}

StringList::StringList(gui::CompositeControl *Parent, const RECTF &Rect) :
	List(Parent, Rect, 0)
{
	SetModel(&m_Model);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TextComboBox

bool TextComboBox::OnKeyDown(uint4 Key)
{
	switch (Key)
	{
	case VK_DOWN:
		ShowList();
		return true;
	}

	return CompositeControl::OnKeyDown(Key);
}

TextComboBox::TextComboBox(CompositeControl *Parent, const RECTF &Rect, StringList *List) :
	CompositeControl(Parent, false, Rect),
	m_Edit(0),
	m_Button(0),
	m_List(0)
{
	m_Edit = new Edit(this, RECTF::ZERO, false);

	m_Button = new SpriteButton(this, RECTF::ZERO, false);
	m_Button->SetSprite(gfx2d::ComplexElement(11), 8.0f, 8.0f);
	m_Button->OnClick.bind(this, &TextComboBox::ButtonClick);

	SetList(List);

	Update();
}

TextComboBox::~TextComboBox()
{
}

void TextComboBox::SetList(StringList *NewList)
{
	if (NewList != m_List)
	{
		m_List = NewList;

		if (m_List)
		{
			m_List->OnKeyEnter.bind(this, &TextComboBox::ListKeyEnter);
			m_List->OnKeyEscape.bind(this, &TextComboBox::ListKeyEscape);
			m_List->OnItemClick.bind(this, &TextComboBox::ListItemClick);
			m_List->SetVisible(false);
			m_List->SetLayer(LAYER_VOLATILE);
		}
	}
}

void TextComboBox::ShowList()
{
	VEC2 LeftBottomPos, PopupPos;
	GetAbsolutePos(&LeftBottomPos);
	LeftBottomPos.y += GetLocalRect().bottom;
	float ListWidth = m_List->GetRect().Width();
	float ListHeight = m_List->GetRect().Height();
	CalculatePopupPos(&PopupPos, LeftBottomPos, ListWidth, ListHeight);

	m_List->SetRect(RECTF(PopupPos.x, PopupPos.y, PopupPos.x+ListWidth, PopupPos.y+ListHeight));
	m_List->GetModel().GuiList_SetSelIndex(0);
	m_List->ScrollTo(0);
	m_List->Update(); // Czy na pewno potrzebne? W sumie nie wiem po co, wpisa³em od wszelkiego...
	m_List->SetVisible(true);
	m_List->Focus();
}

void TextComboBox::HideList()
{
	m_List->SetVisible(false);
	m_Edit->Focus();
}

void TextComboBox::Update()
{
	RECTF R = GetLocalRect();
	m_Edit->SetRect(RECTF(R.left, R.top, R.right-COMBOBOX_BUTTON_WIDTH, R.bottom));
	m_Button->SetRect(RECTF(R.right-COMBOBOX_BUTTON_WIDTH, R.top, R.right, R.bottom));
}

void TextComboBox::ButtonClick(Control *c)
{
	ShowList();
}

void TextComboBox::ListKeyEnter(Control *c)
{
	size_t Index = m_List->GetModel().GuiList_GetSelIndex();

	if (Index < m_List->GetModel().GuiList_GetItemCount())
		m_Edit->SetText(m_List->GetModel().Get(Index));

	m_List->SetVisible(false);
	m_Edit->Focus();
}

void TextComboBox::ListKeyEscape(Control *c)
{
	HideList();
}

void TextComboBox::ListItemClick(List *c, const VEC2 &Pos, size_t Index)
{
	if (m_List->GetModel().GuiList_GetItemCount() == 0)
		return;

	if (Index >= m_List->GetModel().GuiList_GetItemCount())
		Index = m_List->GetModel().GuiList_GetItemCount()-1;

	m_Edit->SetText(m_List->GetModel().Get(Index));

	m_List->SetVisible(false);
	m_Edit->Focus();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ListComboBox

void ListComboBox::OnDraw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->SetSprite(g_Sprite);
	gfx2d::g_Canvas->SetColor(GetRealEnabled() ? COLOR::WHITE : COLOR::GRAY);
	// Ramka
	RECTF R = GetLocalRect();
	R = RECTF(R.left, R.top, R.right-COMBOBOX_BUTTON_WIDTH, R.bottom);
	gfx2d::g_Canvas->DrawRect(R, gfx2d::ComplexElement(3));
	R.Extend(-2.0f);
	// Zawartoœæ
	size_t SelIndex = GetSelIndex();
	if (SelIndex != MAXUINT4)
		m_List->DrawSingleItem(R, SelIndex);
	// Focus
	if (IsFocused())
	{
		gfx2d::g_Canvas->SetColor(g_FocusColor);
		gfx2d::g_Canvas->DrawRect(R, gfx2d::ComplexElement(2));
	}
}

void ListComboBox::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Button == frame::MOUSE_LEFT && Action == frame::MOUSE_DOWN)
	{
		ShowList();
	}
}

bool ListComboBox::OnKeyDown(uint4 Key)
{
	switch (Key)
	{
	case VK_DOWN:
		ShowList();
		return true;
	}

	return CompositeControl::OnKeyDown(Key);
}

ListComboBox::ListComboBox(CompositeControl *Parent, const RECTF &Rect, bool Focusable, List *a_List) :
	CompositeControl(Parent, Focusable, Rect),
	m_Button(0),
	m_List(0)
{
	m_Button = new SpriteButton(this, RECTF::ZERO, false);
	m_Button->SetSprite(gfx2d::ComplexElement(11), 8.0f, 8.0f);
	m_Button->OnClick.bind(this, &ListComboBox::ButtonClick);

	SetList(a_List);

	Update();
}

ListComboBox::~ListComboBox()
{
}

void ListComboBox::SetList(List *NewList)
{
	if (NewList != m_List)
	{
		m_List = NewList;

		if (m_List)
		{
			m_List->OnKeyEnter.bind(this, &ListComboBox::ListKeyEnter);
			m_List->OnKeyEscape.bind(this, &ListComboBox::ListKeyEscape);
			m_List->OnItemClick.bind(this, &ListComboBox::ListItemClick);
			m_List->SetVisible(false);
			m_List->SetLayer(LAYER_VOLATILE);
		}
	}
}

void ListComboBox::ShowList()
{
	VEC2 LeftBottomPos, PopupPos;
	GetAbsolutePos(&LeftBottomPos);
	LeftBottomPos.y += GetLocalRect().bottom;
	float ListWidth = m_List->GetRect().Width();
	float ListHeight = m_List->GetRect().Height();
	CalculatePopupPos(&PopupPos, LeftBottomPos, ListWidth, ListHeight);

	m_List->SetRect(RECTF(PopupPos.x, PopupPos.y, PopupPos.x+ListWidth, PopupPos.y+ListHeight));
	m_List->ScrollTo(m_List->GetModel()->GuiList_GetSelIndex());
	m_List->Update(); // Czy na pewno potrzebne? W sumie nie wiem po co, wpisa³em od wszelkiego...
	m_List->SetVisible(true);
	m_List->Focus();
}

void ListComboBox::HideList()
{
	m_List->SetVisible(false);
	Focus();
}

void ListComboBox::Update()
{
	RECTF R = GetLocalRect();
	m_Button->SetRect(RECTF(R.right-COMBOBOX_BUTTON_WIDTH, R.top, R.right, R.bottom));
}

void ListComboBox::ButtonClick(Control *c)
{
	ShowList();
}

void ListComboBox::ListKeyEnter(Control *c)
{
	size_t Index = m_List->GetModel()->GuiList_GetSelIndex();

	m_List->SetVisible(false);
	Focus();
}

void ListComboBox::ListKeyEscape(Control *c)
{
	HideList();
}

void ListComboBox::ListItemClick(List *c, const VEC2 &Pos, size_t Index)
{
	if (m_List->GetModel()->GuiList_GetItemCount() == 0)
		return;

	m_List->SetVisible(false);
	Focus();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje globalne

void InitGuiControls(res::Font *StandardFont, gfx2d::Sprite *GuiSprite)
{
	g_Font = StandardFont;
	g_Sprite = GuiSprite;
}

uint4 GenerateId()
{
	static uint4 LastId = 1000000000;
	return LastId++;
}


} // namespace gui
