/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "Gfx2D.hpp"
#include "GUI.hpp"

#include <set>
#include <queue>

namespace gui
{

typedef std::set<Control*> CONTROL_SET;

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// STA£E

// Ustawiæ na true, jeœli do rysowania nie u¿ywamy clippingu i kontrolki które
// nie mieszcz¹ siê w ca³oœci w obszarze klienta kontrolki nadrzêdnej maj¹ byæ
// uwa¿ane za niewidzialne - nierysowane, niehittestowane itd.
const bool NO_CLIP = false;

// Czas oczekiwania przed wyœwietleniem dymka [s]
const float HINT_DELAY_LONG = 0.7f;
// Czas, przez jaki musi nie byæ dymka ¿eby potem od nowa musia³ siê przed
// pokazaniem jakiegoœ dymka liczyæ ten pierwszy czas [s]
const float HINT_DELAY_SHORT = 0.1f;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// KOMUNIKATY

struct MESSAGE
{
public:
	enum TYPE
	{
		ON_ENABLE,
		ON_DISABLE,
		ON_SHOW,
		ON_HIDE,
		ON_FOCUS_ENTER,
		ON_FOCUS_LEAVE,
		ON_RECT_CHANGE,
		ON_MOUSE_ENTER,
		ON_MOUSE_LEAVE,
		ON_MOUSE_MOVE,
		ON_MOUSE_BUTTON,
		ON_MOUSE_WHEEL,
		ON_DRAG_ENTER,
		ON_DRAG_LEAVE,
		ON_DRAG_OVER,
		ON_DRAG_DROP,
		ON_DRAG_CANCEL,
		ON_DRAG_FINISHED,
		ON_KEY_DOWN,
		ON_KEY_UP,
		ON_KEY_CHAR
	};

	Control *m_Dest;
	TYPE m_Type;
	// Pola wykorzystywane ró¿ne, zale¿nie od typu
	VEC2 m_Pos;
	frame::MOUSE_BUTTON m_MouseButton;
	frame::MOUSE_ACTION m_MouseAction;
	float m_Delta;
	Control *m_Control;
	DRAG_DATA_SHARED_PTR m_DragData;
	bool m_B;
	char m_C;
	uint4 m_U;
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// KLASA GuiManager_pimpl

class GuiManager_pimpl
{
public:
	// Indeksowana enumem LAYER.
	CONTROL_VECTOR m_Controls[3];
	// Kolekcja wszystkich kontrolek jakie s¹ na wszystkich warstwach na ka¿dym poziomie drzewa
	CONTROL_SET m_AllControls;
	// Aktualna pozycja myszy we wsp. ekranu
	VEC2 m_MousePos;
	// Kontrolka, nad któr¹ aktualnie jest kursor myszy
	Control *m_MouseOverControl;
	
	// Czy przeci¹ganie jest rozpoczête
	bool m_Drag;
	// Kontrolka, która jest Ÿród³em trwaj¹cego aktualnie lub mog¹cego siê rozpocz¹æ przeci¹gania
	Control *m_DragControl;
	// Klawisz myszy, którego wciœniêcie spowodowa³o lub mo¿e spowodowaæ rozpoczêcie przeci¹gania przez kontrolkê m_DragControl
	frame::MOUSE_BUTTON m_DragButton;
	// Dane przeci¹gania
	DRAG_DATA_SHARED_PTR m_DragData;
	// Kolejka kominikatów
	std::queue<MESSAGE> m_MessageQueue;
	// Kontrolki do usuniêcia
	// UWAGA! Tu by³ b³¹d, którego siê naszuka³em. To by³ std::set, a ja traktowa³em to jako kolejkê
	// usuwaj¹c w DeleteControls zawsze pierwszy z nich, podczas gry przecie¿ podczas usuwania mog³y siê
	// dodaæ inne. Kolejnoœæ musi by¿ zachowana.
	CONTROL_VECTOR m_ControlsToRelease;

	// Kontrolka, która aktualnie ma ognisko
	Control *m_FocusedControl;
	Cursor *m_CurrentCursor;
	Hint *m_CurrentHint;
	// Wa¿ne tylko kiedy m_CurrentHint != 0
	float m_CurrentHintStartTime;
	float m_LastHintDrawTime;
	VEC2 m_CurrentHintPos;
	bool m_CurrentHintPosSet;

	GuiManager_pimpl();
	~GuiManager_pimpl();

	// Wstawia kontrolkê do AllControls rejestruj¹c j¹ w ten sposób w zbiorze wszystkich kontrolek
	// Jeœli ju¿ jest wstawina, wywala asercjê
	void AllControls_Insert(Control *c);
	// Usuwa kontrolkê z AllControls wyrejestrowuj¹c j¹ w ten sposób ze zbioru wszystkich kontrolek
	// Jeœli jej tam nie ma, wywala asercjê
	void AllControls_Delete(Control *c);

	void BringControlToFront(Control *c);
	void SendControlToBack(Control *c);
	void AddControl(Control *c);
	void DeleteControl(Control *c);
	void SetControlLayer(Control *c, LAYER Layer);
	// Robi hit-testing. Sprawdza, czy zmieni³o siê okno, nad którym jest kursor myszy.
	// Odczytuje i ewentualnie zmienia MouseOverWindow.
	// Wysy³a (jeœli siê zmieni³o) zdarzenia OnMouseEnter i OnMouseLeave.
	// Przez LocalPos, jeœli != 0, zwraca pozycjê bie¿¹c¹ myszy we wsp. lokalnych nowej kontrolki mouse-over.
	void HandlePossibleMouseOverChange(VEC2 *LocalPos);
	void DragCancel();
	// Daje ognisko podanej kontrolce
	void SetFocus(Control *c);
	// Daje ognisko kontrolce nastêpnej za podan¹
	void SetFocusNext(Control *c);
	// Daje ognisko kontrolce poprzedniej przed podan¹
	void SetFocusPrev(Control *c);
	void HideVolatileControls();
	void ChangeCurrentCursor(Cursor *NewCursor);
	void ChangeCurrentHint(Hint *NewHint);
	void ReleaseControl(Control *c);

	// Komunikaty - dodanie do kolejki
	void SendEnable(Control *Dest);
	void SendDisable(Control *Dest);
	void SendShow(Control *Dest);
	void SendHide(Control *Dest);
	void SendFocusEnter(Control *Dest);
	void SendFocusLeave(Control *Dest);
	void SendRectChange(Control *Dest);
	void SendMouseEnter(Control *Dest);
	void SendMouseLeave(Control *Dest);
	void SendMouseMove(Control *Dest, const VEC2 &Pos);
	void SendMouseButton(Control *Dest, const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	void SendMouseWheel(Control *Dest, const VEC2 &Pos, float Delta);
	void SendDragEnter(Control *Dest, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData);
	void SendDragLeave(Control *Dest, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData);
	void SendDragOver(Control *Dest, const VEC2 &Pos, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData);
	void SendDragDrop(Control *Dest, const VEC2 &Pos, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData);
	void SendDragCancel(Control *Dest, const VEC2 &Pos, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData);
	void SendDragFinished(Control *Dest, bool Success, Control *DropControl, DRAG_DATA_SHARED_PTR a_DragData);
	void SendKeyDown(Control *Dest, uint4 Key);
	void SendKeyUp(Control *Dest, uint4 Key);
	void SendChar(Control *Dest, char Ch);
};


GuiManager_pimpl::GuiManager_pimpl() :
	m_MousePos(VEC2::ZERO),
	m_MouseOverControl(0),
	m_Drag(false),
	m_DragControl(0),
	m_CurrentCursor(0),
	m_CurrentHint(0),
	m_CurrentHintStartTime(0.0f),
	m_LastHintDrawTime(0.0f),
	m_CurrentHintPos(VEC2::ZERO),
	m_CurrentHintPosSet(false),
	m_FocusedControl(0)
{
}

GuiManager_pimpl::~GuiManager_pimpl()
{
}

void GuiManager_pimpl::BringControlToFront(Control *c)
{
	uint4 Index;
	for (uint4 Layer = 0; Layer < LAYER_COUNT; Layer++)
	{
		Index = g_GuiManager->FindControl((LAYER)Layer, c);
		if (Index != MAXUINT4 && Index < m_Controls[Layer].size()-1)
		{
			m_Controls[Layer].erase(m_Controls[Layer].begin()+Index);
			m_Controls[Layer].push_back(c);
			break;
		}
	}
}

void GuiManager_pimpl::SendControlToBack(Control *c)
{
	uint4 Index;
	for (uint4 Layer = 0; Layer < LAYER_COUNT; Layer++)
	{
		Index = g_GuiManager->FindControl((LAYER)Layer, c);
		if (Index != MAXUINT4 && Index > 0)
		{
			m_Controls[Layer].erase(m_Controls[Layer].begin()+Index);
			m_Controls[Layer].insert(m_Controls[Layer].begin(), c);
		}
	}
}

void GuiManager_pimpl::DragCancel()
{
	assert(m_Drag);
	assert(m_DragControl);

	// Kontrolka, nad któr¹ faktycznie jest myszka, dostaje OnDragCancel
	if (m_MouseOverControl && m_MouseOverControl->GetRealEnabled())
	{
		VEC2 MouseOverLocalPos;
		m_MouseOverControl->ScreenToClient(&MouseOverLocalPos, m_MousePos);
		SendDragCancel(m_MouseOverControl, MouseOverLocalPos, m_DragControl, m_DragData);
	}
	// Kontrolka przeci¹gana dostaje OnDragFinished
	// Docelowa jest przy tym równa 0, bo przyczyna anulowania jest systemowa a nie odmowa docelowej
	SendDragFinished(m_DragControl, false, 0, m_DragData);

	// Jeœli inna jest kontrolka Ÿród³owa ni¿ docelowa
	if (m_DragControl != m_MouseOverControl)
	{
		// OnMouseLeave Ÿród³owej, OnMouseEnter docelowej
		SendMouseLeave(m_DragControl);
		if (m_MouseOverControl && m_MouseOverControl->GetRealEnabled())
			SendMouseEnter(m_MouseOverControl);

		m_Drag = false;
		m_DragControl = 0;
		m_DragData.reset();

		// Zmiana kursora
		if (m_MouseOverControl)
			ChangeCurrentCursor(m_MouseOverControl->GetCursor());
		else
			ChangeCurrentCursor(0);
	}
	else
	{
		m_Drag = false;
		m_DragControl = 0;
		m_DragData.reset();
	}
}

void GuiManager_pimpl::SetFocus(Control *c)
{
	if (c != m_FocusedControl && c->GetRealEnabled() && c->GetRealVisible() && c->GetFocusable())
	{
		// Kontrolka maj¹ca poprzednio ognisko - OnFocusLeave
		if (m_FocusedControl)
			SendFocusLeave(m_FocusedControl);
		// Nowa kontrolka
		m_FocusedControl = c;
		// Nowa kontrolka - OnFocusEnter
		if (m_FocusedControl)
			SendFocusEnter(m_FocusedControl);
	}

	// Jeœli nowa kontrolka le¿y poza warstw¹ VOLATILE lub nie ma ogniska ¿adna, ukryj wszystkie okna tej warstwy
	bool HideVolatile = (m_FocusedControl == 0);
	if (!HideVolatile)
	{
		LAYER Layer;
		g_GuiManager->FindControl(m_FocusedControl, &Layer, 0);
		HideVolatile = (Layer != LAYER_VOLATILE);
	}
	if (HideVolatile)
		HideVolatileControls();
}

void GuiManager_pimpl::SetFocusNext(Control *c)
{
	assert(c);

	Control *NewC = 0;
	CompositeControl *cc;

	// Jeœli to jest CompositeControl, przejrzeæ wszystkie dzieci od pocz¹tku
	if (dynamic_cast<CompositeControl*>(c))
	{
		cc = static_cast<CompositeControl*>(c);
		for (size_t i = 0; i < cc->m_SubControls.size(); i++)
		{
			NewC = cc->m_SubControls[i]->FindFirstFocusableControl();
			if (NewC)
				break;
		}
	}

	// Jeœli nie znaleziono ni¿ej, szukamy wy¿ej i w prawo
	// cc bêdzie zawsze parentem c
	if (NewC == 0)
	{
		for (;;)
		{
			cc = c->GetParent();
			if (cc == 0)
				break;
			NewC = cc->FindNextFocusableControl(c);
			if (NewC)
				break;
			c = cc;
		}
	}

	// Jeœli nadal nie znaleziono, szukamy wszêdzie - pierwszej w ca³ym drzewie
	if (NewC == 0)
		NewC = c->GetTopLevelParent()->FindFirstFocusableControl();

	// Jeœli znaleziono - ognisko!
	if (NewC)
		SetFocus(NewC);
}

void GuiManager_pimpl::SetFocusPrev(Control *c)
{
	assert(c);

	Control *NewC = 0;
	CompositeControl *cc;

	// Jeœli to jest CompositeControl, przejrzeæ wszystkie dzieci od koñca
	if (dynamic_cast<CompositeControl*>(c))
	{
		cc = static_cast<CompositeControl*>(c);
		for (size_t i = cc->m_SubControls.size(); i--; )
		{
			NewC = cc->m_SubControls[i]->FindLastFocusableControl();
			if (NewC)
				break;
		}
	}

	// Jeœli nie znaleziono ni¿ej, szukamy wy¿ej i w lewo
	// cc bêdzie zawsze parentem c
	if (NewC == 0)
	{
		for (;;)
		{
			cc = c->GetParent();
			if (cc == 0)
				break;
			NewC = cc->FindPrevFocusableControl(c);
			if (NewC)
				break;
			c = cc;
		}
	}

	// Jeœli nadal nie znaleziono, szukamy wszêdzie - ostatniej w ca³ym drzewie
	if (NewC == 0)
		NewC = c->GetTopLevelParent()->FindLastFocusableControl();

	// Jeœli znaleziono - ognisko!
	if (NewC)
		SetFocus(NewC);
}

void GuiManager_pimpl::HideVolatileControls()
{
	for (size_t i = 0; i < m_Controls[LAYER_VOLATILE].size(); i++)
		m_Controls[LAYER_VOLATILE][i]->SetVisible(false);
}

void GuiManager_pimpl::ChangeCurrentCursor(Cursor *NewCursor)
{
	bool Changed = (NewCursor != m_CurrentCursor);

	m_CurrentCursor = NewCursor;

	if (Changed && g_GuiManager->OnCursorChange)
		g_GuiManager->OnCursorChange(m_CurrentCursor);
}

void GuiManager_pimpl::ChangeCurrentHint(Hint *NewHint)
{
	bool Changed = (NewHint != m_CurrentHint);

	if (dynamic_cast<PopupHint*>(NewHint))
	{
		if (m_LastHintDrawTime + HINT_DELAY_SHORT >= frame::Timer1.GetTime())
			m_CurrentHintStartTime = frame::Timer1.GetTime();
		else
			m_CurrentHintStartTime = frame::Timer1.GetTime() + HINT_DELAY_LONG;

		m_CurrentHintPosSet = false;
	}

	m_CurrentHint = NewHint;

	if (Changed && g_GuiManager->OnHintChange)
		g_GuiManager->OnHintChange(m_CurrentHint);
}

void GuiManager_pimpl::ReleaseControl(Control *c)
{
	m_ControlsToRelease.push_back(c);
}

void GuiManager_pimpl::SendEnable(Control *Dest)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_ENABLE;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendDisable(Control *Dest)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_DISABLE;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendShow(Control *Dest)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_SHOW;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendHide(Control *Dest)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_HIDE;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendFocusEnter(Control *Dest)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_FOCUS_ENTER;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendFocusLeave(Control *Dest)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_FOCUS_LEAVE;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendRectChange(Control *Dest)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_RECT_CHANGE;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendMouseEnter(Control *Dest)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_MOUSE_ENTER;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendMouseLeave(Control *Dest)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_MOUSE_LEAVE;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendMouseMove(Control *Dest, const VEC2 &Pos)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_MOUSE_MOVE;
	msg.m_Pos = Pos;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendMouseButton(Control *Dest, const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_MOUSE_BUTTON;
	msg.m_Pos = Pos;
	msg.m_MouseButton = Button;
	msg.m_MouseAction = Action;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendMouseWheel(Control *Dest, const VEC2 &Pos, float Delta)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_MOUSE_WHEEL;
	msg.m_Pos = Pos;
	msg.m_Delta = Delta;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendDragEnter(Control *Dest, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_DRAG_ENTER;
	msg.m_Control = DragControl;
	msg.m_DragData = a_DragData;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendDragLeave(Control *Dest, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_DRAG_LEAVE;
	msg.m_Control = DragControl;
	msg.m_DragData = a_DragData;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendDragOver(Control *Dest, const VEC2 &Pos, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_DRAG_OVER;
	msg.m_Pos = Pos;
	msg.m_Control = DragControl;
	msg.m_DragData = a_DragData;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendDragDrop(Control *Dest, const VEC2 &Pos, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_DRAG_DROP;
	msg.m_Pos = Pos;
	msg.m_Control = DragControl;
	msg.m_DragData = a_DragData;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendDragCancel(Control *Dest, const VEC2 &Pos, Control *DragControl, DRAG_DATA_SHARED_PTR a_DragData)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_DRAG_CANCEL;
	msg.m_Pos = Pos;
	msg.m_Control = DragControl;
	msg.m_DragData = a_DragData;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendDragFinished(Control *Dest, bool Success, Control *DropControl, DRAG_DATA_SHARED_PTR a_DragData)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_DRAG_FINISHED;
	msg.m_B = Success;
	msg.m_Control = DropControl;
	msg.m_DragData = a_DragData;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendKeyDown(Control *Dest, uint4 Key)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_KEY_DOWN;
	msg.m_U = Key;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendKeyUp(Control *Dest, uint4 Key)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_KEY_UP;
	msg.m_U = Key;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::SendChar(Control *Dest, char Ch)
{
	MESSAGE msg;
	msg.m_Dest = Dest;
	msg.m_Type = MESSAGE::ON_KEY_CHAR;
	msg.m_C = Ch;
	m_MessageQueue.push(msg);
}

void GuiManager_pimpl::AddControl(Control *c)
{
	m_Controls[LAYER_NORMAL].push_back(c);
}

void GuiManager_pimpl::DeleteControl(Control *c)
{
	uint4 Index;
	for (uint4 Layer = 0; Layer < LAYER_COUNT; Layer++)
	{
		Index = g_GuiManager->FindControl((LAYER)Layer, c);
		if (Index != MAXUINT4 && Index < m_Controls[Layer].size())
		{
			m_Controls[Layer].erase(m_Controls[Layer].begin()+Index);
			return;
		}
	}

	assert(0 && "gui::GuiManager::DeleteControl: Usuwana kontrolka nie znaleziona poœród kontrolek g³ównych na ¿adnej warstwie.");
}

void GuiManager_pimpl::SetControlLayer(Control *c, LAYER Layer)
{
	// ZnajdŸ warstwê
	uint4 Index;
	for (uint4 li = 0; li < LAYER_COUNT; li++)
	{
		// Znaleziona
		if ( (Index = g_GuiManager->FindControl((LAYER)li, c)) != MAXUINT4 )
		{
			// Jeœli przenosimy na inn¹ warstwê
			if (li != Layer)
			{
				// Usuñ ze starej
				m_Controls[li].erase(m_Controls[li].begin()+Index);
				// Dodaj na wierzch do nowej
				m_Controls[Layer].push_back(c);
			}
			return;
		}
	}
	// Nie znaleziona - coœ nie tak
	assert(0 && "gui::GuiManager::SetControlLayer: Funkcja wywo³ana dla kontrolki nie znalezionej na g³ównym poziomie na ¿adnej warstwie - niemo¿liwe!");
}

void GuiManager_pimpl::HandlePossibleMouseOverChange(VEC2 *LocalPos)
{
	Control *NewMouseOverControl = g_GuiManager->HitTest(m_MousePos, LocalPos);

	// Jeœli zmieni³o siê okno, nad którym jest kursor myszy
	if (NewMouseOverControl != m_MouseOverControl)
	{
		// Jeœli by³o stare okno i jest poœrednio aktywne i widoczne
		if (m_MouseOverControl && m_MouseOverControl->GetRealVisible() && m_MouseOverControl->GetRealEnabled())
		{
			// Zdarzenie wyjœcia myszy
			if (m_Drag) // dawniej: (m_DragControl)
				SendDragLeave(m_MouseOverControl, m_DragControl, m_DragData);
			else
				SendMouseLeave(m_MouseOverControl);
		}
		// Nowe okno, nad którym jest kursor myszy
		m_MouseOverControl = NewMouseOverControl;
		// Jeœli jest nowe okno i jest poœrednio aktywne (widoczne na pewno jest, bo by nie przesz³o hit-testu)
		if (m_MouseOverControl && m_MouseOverControl->GetRealEnabled())
		{
			// Zdarzenie wejœcia myszy
			if (m_Drag) // dawniej: (m_DragControl)
				SendDragEnter(m_MouseOverControl, m_DragControl, m_DragData);
			else
				SendMouseEnter(m_MouseOverControl);
		}

		// Kursor nowego okna
		if (!m_Drag)
		{
			if (m_MouseOverControl)
				ChangeCurrentCursor(m_MouseOverControl->GetCursor());
			else
				ChangeCurrentCursor(0);
		}

		// Hint nowego okna
		if (m_MouseOverControl)
			ChangeCurrentHint(m_MouseOverControl->GetHint());
		else
			ChangeCurrentHint(0);
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Hint

Hint::~Hint()
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PopupHint

PopupHint::~PopupHint()
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Cursor

Cursor::~Cursor()
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa DragData

DragData::~DragData()
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Control

void Control::UpdateRealEnabledVisible()
{
	bool NewRealVisible = m_Visible;
	bool NewRealEnabled = m_Enabled;
	if (m_Parent != 0)
	{
		NewRealVisible = NewRealVisible && m_Parent->GetRealVisible();
		NewRealEnabled = NewRealEnabled && m_Parent->GetRealEnabled();
	}

	// Wyœlij komunikaty o aktywowaniu/deaktywowaniu, pokazaniu/ukryciu

	if (NewRealVisible && !m_RealVisible)
		g_GuiManager->pimpl->SendShow(this);
	else if (!NewRealVisible && m_RealVisible)
		g_GuiManager->pimpl->SendHide(this);

	if (NewRealEnabled && !m_RealEnabled)
		g_GuiManager->pimpl->SendEnable(this);
	else if (!NewRealEnabled && m_RealEnabled)
		g_GuiManager->pimpl->SendDisable(this);

	m_RealVisible = NewRealVisible;
	m_RealEnabled = NewRealEnabled;

	// Jeœli sta³a siê nieaktywna lub niewidoczna
	if (!NewRealVisible || !NewRealEnabled)
	{
		// Jeœli trwa przeci¹ganie i przeci¹gana jest w³aœnie ta kontrolka
		if (g_GuiManager->pimpl->m_Drag && g_GuiManager->pimpl->m_DragControl == this)
		{
			// Kontrolka, nad któr¹ faktycznie jest myszka, dostaje OnDragCancel
			if (g_GuiManager->pimpl->m_MouseOverControl && g_GuiManager->pimpl->m_MouseOverControl != this && g_GuiManager->pimpl->m_MouseOverControl->GetRealEnabled() && g_GuiManager->pimpl->m_MouseOverControl->GetRealVisible())
			{
				VEC2 MouseOverLocalPos;
				g_GuiManager->pimpl->m_MouseOverControl->ScreenToClient(&MouseOverLocalPos, g_GuiManager->pimpl->m_MousePos);
				g_GuiManager->pimpl->SendDragCancel(g_GuiManager->pimpl->m_MouseOverControl, MouseOverLocalPos, g_GuiManager->pimpl->m_DragControl, g_GuiManager->pimpl->m_DragData);
			}
			// Kontrolka przeci¹gana, czyli ta, nie dostaje OnDragFinished, bo sta³a siê nieaktywna lub niewidzialna i takich rzeczy w takim stanie nie dostaje.
			// Koniec przeci¹gania
			g_GuiManager->pimpl->m_Drag = false;
			g_GuiManager->pimpl->m_DragControl = 0;
			g_GuiManager->pimpl->m_DragData.reset();
		}
		
		// Jeœli ta kontrolka mia³a ognisko
		if (g_GuiManager->pimpl->m_FocusedControl == this)
		{
			// Ta kontrolka nie dostanie OnFocusLeave
			g_GuiManager->pimpl->m_FocusedControl = 0;
			// Dajemy ognisko nastêpnej kontrolce po tej
			g_GuiManager->pimpl->SetFocusNext(this);
		}
	}
}

Control * Control::FindFirstFocusableControl()
{
	return (GetRealEnabled() && GetRealVisible() && GetFocusable()) ? this : 0;
}

Control * Control::FindLastFocusableControl()
{
	return (GetRealEnabled() && GetRealVisible() && GetFocusable()) ? this : 0;
}

Control::Control(CompositeControl *Parent, bool Focusable, const RECTF &Rect) :
	m_Parent(Parent),
	m_Visible(true),
	m_Enabled(true),
	m_Focusable(Focusable),
	m_Rect(Rect),
	m_LocalRect(0.0f, 0.0f, Rect.Width(), Rect.Height()),
	m_Cursor(0),
	m_Hint(0)
{
	// Tak, wiem, to konstruktor klasy bazowej, tu wywo³a siê Control::UpdateRealEnabledVisible i
	// polimorfizm tej wirtualnej metody nie zadzia³a. Trudno, tak jest OK.
	UpdateRealEnabledVisible();
	g_GuiManager->pimpl->AllControls_Insert(this);
	if (Parent == 0)
		g_GuiManager->pimpl->AddControl(this);
	else
		Parent->AddSubControl(this);

	g_GuiManager->pimpl->HandlePossibleMouseOverChange(0);
}

Control::~Control()
{
	// Usuñ siê z kolekcji wszystkich kontrolek
	g_GuiManager->pimpl->AllControls_Delete(this);
	// Usuñ siê z kolekcji podkontrolek kontrolki nadrzêdnej lub, jeœli to g³ównego poziomu, z listy kontrolek g³ównych
	if (m_Parent == 0)
		g_GuiManager->pimpl->DeleteControl(this);
	else
		m_Parent->DeleteSubControl(this);

	// Jeœli trwa przeci¹ganie i przeci¹gana jest w³aœnie ta kontrolka
	if (g_GuiManager->pimpl->m_Drag && g_GuiManager->pimpl->m_DragControl == this)
	{
		// Kontrolka, nad któr¹ faktycznie jest myszka, dostaje OnDragCancel
		if (g_GuiManager->pimpl->m_MouseOverControl && g_GuiManager->pimpl->m_MouseOverControl != this && g_GuiManager->pimpl->m_MouseOverControl->GetRealEnabled() && g_GuiManager->pimpl->m_MouseOverControl->GetRealVisible())
		{
			VEC2 MouseOverLocalPos;
			g_GuiManager->pimpl->m_MouseOverControl->ScreenToClient(&MouseOverLocalPos, g_GuiManager->pimpl->m_MousePos);
			g_GuiManager->pimpl->SendDragCancel(g_GuiManager->pimpl->m_MouseOverControl, MouseOverLocalPos, g_GuiManager->pimpl->m_DragControl, g_GuiManager->pimpl->m_DragData);
		}
		// Kontrolka przeci¹gana, czyli ta, nie dostaje OnDragFinished, bo i po co jej teraz.
		// Koniec przeci¹gania
		g_GuiManager->pimpl->m_Drag = false;
		g_GuiManager->pimpl->m_DragControl = 0;
		g_GuiManager->pimpl->m_DragData.reset();
	}

	// Jeœli ta kontrolka mia³a ognisko
	if (g_GuiManager->pimpl->m_FocusedControl == this)
	{
		// Przenieœ ognisko na nic
		// Ta kontrolka nie dostanie OnFocusLeave
		// Nie na nastêpn¹ jak przy deaktywacji (enabled na false) czy ukrywaniu (visible na false),
		// bo tutaj jak siê kaskadowo usuwa ca³a kontrolka nadrzêdna wraz z podkontrolkami to zaczyna
		// od tych na dole, dopiero potem ta na górze, odwrotnie ni¿ tam, wiêc by ognisko wêdrowa³o
		// do nastêpnej, jeszcze nastêpnej itd. zanim wyjdzie poza usuwany obszar, o ile w ogóle
		// by siê nie posypa³o jakoœtam...
		g_GuiManager->pimpl->m_FocusedControl = 0;
		g_GuiManager->pimpl->SetFocus(0);
	}

	// Niby nie trzebaby, bo ja nadal istniejê, a ju¿ mnie nie ma na listach, wiêc zadzia³a³oby.
	// Ale po co generowaæ w takim momencie OnMouseLeave.
	if (g_GuiManager->pimpl->m_MouseOverControl == this)
		g_GuiManager->pimpl->m_MouseOverControl = 0;

	g_GuiManager->pimpl->HandlePossibleMouseOverChange(0);
}

bool Control::OnKeyDown(uint4 Key)
{
	if (Key == VK_TAB)
	{
		if (frame::GetKeyboardKey(VK_SHIFT))
			FocusPrev();
		else
			FocusNext();
		return true;
	}

	return false;
}

bool Control::OnKeyUp(uint4 Key)
{
	return false;
}

bool Control::OnChar(char Ch)
{
	return false;
}

void Control::Draw(const VEC2 &Translation)
{
	VEC2 t; GetRect().LeftTop(&t);
	gfx2d::g_Canvas->PushTranslation(t);
	gfx2d::g_Canvas->PushClipRect(GetLocalRect());

	// Narysuj siê
	OnDraw(Translation);

	gfx2d::g_Canvas->PopClipRect();
	gfx2d::g_Canvas->PopTranslation();
}

void Control::SetVisible(bool Visible)
{
	m_Visible = Visible;
	UpdateRealEnabledVisible();
	g_GuiManager->pimpl->HandlePossibleMouseOverChange(0);
}

void Control::SetEnabled(bool Enabled)
{
	m_Enabled = Enabled;
	UpdateRealEnabledVisible();
}

void Control::SetFocusable(bool Focusable)
{
	m_Focusable = Focusable;

	// Jeœli ta kontrolka mia³a ognisko - przenieœ na nastêpn¹
	if (g_GuiManager->pimpl->m_FocusedControl == this)
		g_GuiManager->pimpl->SetFocusNext(this);
}

void Control::SetCursor(Cursor *c)
{
	m_Cursor = c;

	if (g_GuiManager->pimpl->m_Drag == false && g_GuiManager->pimpl->m_MouseOverControl == this)
		g_GuiManager->pimpl->ChangeCurrentCursor(c);
}

void Control::CursorChanged()
{
	if (g_GuiManager->pimpl->m_Drag == false && g_GuiManager->pimpl->m_MouseOverControl == this)
	{
		if (g_GuiManager->OnCursorChange)
			g_GuiManager->OnCursorChange(g_GuiManager->GetCurrentCursor());
	}
}

void Control::SetHint(Hint *h)
{
	m_Hint = h;

	if (g_GuiManager->pimpl->m_MouseOverControl == this)
		g_GuiManager->pimpl->ChangeCurrentHint(h);
}

void Control::HintChanged()
{
	if (g_GuiManager->pimpl->m_MouseOverControl == this)
	{
		if (g_GuiManager->OnHintChange)
			g_GuiManager->OnHintChange(g_GuiManager->GetCurrentHint());
	}
}

void Control::Release()
{
	g_GuiManager->pimpl->ReleaseControl(this);
}

Control * Control::GetTopLevelParent()
{
	Control *c = this, *cp;
	for (;;)
	{
		assert(g_GuiManager->ControlExists(c));

		cp = c->GetParent();
		if (cp == 0)
			return c;
		c = cp;
	}
}

void Control::BringToFront()
{
	if (GetParent() == 0)
		g_GuiManager->pimpl->BringControlToFront(this);

	g_GuiManager->pimpl->HandlePossibleMouseOverChange(0);
}

void Control::SendToBack()
{
	if (GetParent() == 0)
		g_GuiManager->pimpl->SendControlToBack(this);

	g_GuiManager->pimpl->HandlePossibleMouseOverChange(0);
}

void Control::SetRect(const RECTF &Rect)
{
	m_Rect = Rect;
	m_LocalRect = RECTF(0.0f, 0.0f, Rect.Width(), Rect.Height());
	g_GuiManager->pimpl->HandlePossibleMouseOverChange(0);
	g_GuiManager->pimpl->SendRectChange(this);
}

void Control::GetAbsolutePos(VEC2 *Out)
{
	Out->x = m_Rect.left;
	Out->y = m_Rect.top;

	CompositeControl *Parent = GetParent();
	RECTF ClientRect;

	while (Parent != 0)
	{
		Parent->GetClientRect(&ClientRect);
		Out->x += ClientRect.left;
		Out->y += ClientRect.top;

		const RECTF &Rect = Parent->GetRect();
		Out->x += Rect.left;
		Out->y += Rect.top;

		Parent = Parent->GetParent();
	}
}

void Control::ClientToScreen(VEC2 *Out, const VEC2 &v)
{
	VEC2 t;
	GetAbsolutePos(&t);

	*Out = v + t;
}

void Control::ClientToScreen(RECTF *Out, const RECTF &r)
{
	VEC2 t;
	GetAbsolutePos(&t);

	Out->left = r.left + t.x;
	Out->right = r.right + t.x;
	Out->top = r.top + t.y;
	Out->bottom = r.bottom + t.y;
}

void Control::ScreenToClient(VEC2 *Out, const VEC2 &v)
{
	VEC2 t;
	GetAbsolutePos(&t);

	*Out = v - t;
}

void Control::ScreenToClient(RECTF *Out, const RECTF &r)
{
	VEC2 t;
	GetAbsolutePos(&t);

	Out->left = r.left - t.x;
	Out->right = r.right - t.x;
	Out->top = r.top - t.y;
	Out->bottom = r.bottom - t.y;
}

Control * Control::HitTest(const VEC2 &Pos, VEC2 *LocalPos)
{
	if (OnHitTest(Pos))
	{
		if (LocalPos)
			*LocalPos = Pos;
		return this;
	}
	else
		return 0;
}

void Control::SetLayer(LAYER Layer)
{
	// Zmieniæ warstwê mo¿na tylko dla kontrolki najwy¿szego poziomu
	if (m_Parent == 0)
	{
		g_GuiManager->pimpl->SetControlLayer(this, Layer);
		g_GuiManager->pimpl->HandlePossibleMouseOverChange(0);
	}
}

bool Control::IsMouseOver()
{
	return g_GuiManager->GetMouseOverControl() == this;
}

bool Control::IsFocused()
{
	return g_GuiManager->GetFocusedControl() == this;
}

void Control::Focus()
{
	g_GuiManager->pimpl->SetFocus(this);
}

void Control::FocusNext()
{
	g_GuiManager->pimpl->SetFocusNext(this);
}

void Control::FocusPrev()
{
	g_GuiManager->pimpl->SetFocusPrev(this);
}

void Control::StartDrag(DRAG_DATA_SHARED_PTR a_DragData)
{
	if (!g_GuiManager->pimpl->m_Drag && g_GuiManager->pimpl->m_DragControl == this && IsMouseOver() && GetRealEnabled() && GetRealVisible())
	{
		g_GuiManager->pimpl->m_Drag = true;
		g_GuiManager->pimpl->m_DragData = a_DragData;

		// Zmiana kursora
		g_GuiManager->pimpl->ChangeCurrentCursor(a_DragData.get());
	}
}

void Control::CancelDrag()
{
	// Jeœli trwa przeci¹ganie tej kontrolki
	if (g_GuiManager->IsDrag() && g_GuiManager->GetDragControl() == this)
		g_GuiManager->pimpl->DragCancel();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa CompositeControl

void CompositeControl::UpdateRealEnabledVisible()
{
	// Siebie
	Control::UpdateRealEnabledVisible();

	// Rekurencyjnie podkontrolki
	for (CONTROL_VECTOR::iterator it = m_SubControls.begin(); it != m_SubControls.end(); ++it)
		(*it)->UpdateRealEnabledVisible();
}

CompositeControl::CompositeControl(CompositeControl *Parent, bool Focusable, const RECTF &Rect) :
	Control(Parent, Focusable, Rect),
	m_InDestructor(false)
{
}

CompositeControl::~CompositeControl()
{
	m_InDestructor = true;

	// Usuñ wszystkie podkontrolki
	for (CONTROL_VECTOR::reverse_iterator rit = m_SubControls.rbegin(); rit != m_SubControls.rend(); ++rit)
		delete *rit;
	//m_SubControls.clear();

	m_InDestructor = false;
}

void CompositeControl::Draw(const VEC2 &Translation)
{
	gfx2d::g_Canvas->PushTranslation(VEC2(GetRect().left, GetRect().top));
	gfx2d::g_Canvas->PushClipRect(GetLocalRect());

	// Narysuj siê
	OnDraw(Translation);

	// Narysuj pod-kontrolki
	RECTF ClientRect;
	GetClientRect(&ClientRect);
	gfx2d::g_Canvas->PushClipRect(ClientRect);
	gfx2d::g_Canvas->PushTranslation(VEC2(ClientRect.left, ClientRect.top));

	VEC2 ClientTranslation;
	ClientTranslation.x = Translation.x + ClientRect.left;
	ClientTranslation.y = Translation.y + ClientRect.top;
	RECTF LocalClientRect = RECTF(0.0f, 0.0f, ClientRect.Width(), ClientRect.Height());

	for (CONTROL_VECTOR::iterator it = m_SubControls.begin(); it != m_SubControls.end(); ++it)
	{
		// Jeœli niewidzialna - nie rozwa¿amy
		if (!(*it)->GetVisible())
			continue;
		// Jeœli NO_CLIP i nie mieœci siê w ca³oœci
		const RECTF & LocalRect = (*it)->GetRect();
		if (NO_CLIP)
		{
			if (!RectInRect(LocalRect, LocalClientRect))
				continue;
		}
		// Lub jeœli nie widaæ go w ogóle
		else
		{
			if (!OverlapRect(LocalRect, LocalClientRect))
				continue;
		}
		// Narysuj
		(*it)->Draw(VEC2(ClientTranslation.x + LocalRect.left, ClientTranslation.y + LocalRect.top));
	}

	gfx2d::g_Canvas->PopTranslation();
	gfx2d::g_Canvas->PopClipRect();

	gfx2d::g_Canvas->PopClipRect();
	gfx2d::g_Canvas->PopTranslation();
}

uint4 CompositeControl::FindSubControl(Control *SubControl)
{
	for (size_t i = m_SubControls.size(); i--; )
	{
		if (m_SubControls[i] == SubControl)
			return i;
	}
	return MAXUINT4;
}

void CompositeControl::MoveSubControlToBegin(uint4 Index)
{
	assert(Index < m_SubControls.size());

	Control *c = m_SubControls[Index];
	m_SubControls.erase(m_SubControls.begin()+Index);
	m_SubControls.insert(m_SubControls.begin(), c);
}

void CompositeControl::MoveSubControlToBegin(Control *SubControl)
{
	uint4 Index = FindSubControl(SubControl);
	if (Index != MAXUINT4)
		MoveSubControlToBegin(Index);
}

void CompositeControl::MoveSubControlToEnd(uint4 Index)
{
	assert(Index < m_SubControls.size());

	Control *c = m_SubControls[Index];
	m_SubControls.erase(m_SubControls.begin()+Index);
	m_SubControls.push_back(c);
}

void CompositeControl::MoveSubControlToEnd(Control *SubControl)
{
	uint4 Index = FindSubControl(SubControl);
	if (Index != MAXUINT4)
		MoveSubControlToEnd(Index);
}

void CompositeControl::ExchangeSubControls(uint4 Index1, uint4 Index2)
{
	assert(Index1 < m_SubControls.size());
	assert(Index2 < m_SubControls.size());

	if (Index1 == Index2) return;

	std::swap(m_SubControls[Index1], m_SubControls[Index2]);
}

void CompositeControl::ExchangeSubControls(Control *SubControl1, Control *SubControl2)
{
	uint4 Index1 = FindSubControl(SubControl1);
	uint4 Index2 = FindSubControl(SubControl2);
	if (Index1 != MAXUINT4 && Index2 != MAXUINT4)
		ExchangeSubControls(Index1, Index2);
}

void CompositeControl::AddSubControl(Control *SubControl)
{
	m_SubControls.push_back(SubControl);
}

void CompositeControl::DeleteSubControl(Control *SubControl)
{
	if (m_InDestructor) return;

	uint4 Index = FindSubControl(SubControl);
	assert(Index != MAXUINT4 && Index < m_SubControls.size());
	m_SubControls.erase(m_SubControls.begin()+Index);
}

Control * CompositeControl::FindFirstFocusableControl()
{
	Control *R = Control::FindFirstFocusableControl();
	if (R)
		return R;

	for (size_t i = 0; i < m_SubControls.size(); i++)
	{
		R = m_SubControls[i]->FindFirstFocusableControl();
		if (R)
			return R;
	}

	return 0;
}

Control * CompositeControl::FindLastFocusableControl()
{
	Control *R = Control::FindLastFocusableControl();
	if (R)
		return R;

	for (size_t i = m_SubControls.size(); i--; )
	{
		R = m_SubControls[i]->FindLastFocusableControl();
		if (R)
			return R;
	}

	return 0;
}

Control * CompositeControl::FindNextFocusableControl(Control *c)
{
	bool OK = false;

	Control *R;

	for (size_t i = 0; i < m_SubControls.size(); i++)
	{
		if (OK)
		{
			R = m_SubControls[i]->FindFirstFocusableControl();
			if (R)
				return R;
		}

		if (m_SubControls[i] == c)
			OK = true;
	}

	assert(OK && "gui::CompositeControl::FindNextFocusableControl: podana zosta³a kontrolka która nie jest potomn¹ bie¿¹cej.");

	return 0;
}

Control * CompositeControl::FindPrevFocusableControl(Control *c)
{
	bool OK = false;

	Control *R;

	for (size_t i = m_SubControls.size(); i--; )
	{
		if (OK)
		{
			R = m_SubControls[i]->FindLastFocusableControl();
			if (R)
				return R;
		}

		if (m_SubControls[i] == c)
			OK = true;
	}

	assert(OK && "gui::CompositeControl::FindPrevFocusableControl: podana zosta³a kontrolka która nie jest potomn¹ bie¿¹cej.");

	return 0;
}

bool CompositeControl::IsParent(Control *c)
{
	CompositeControl *p = c->GetParent();
	for (;;)
	{
		if (p == 0)
			return false;
		if (p == this)
			return true;
		p = p->GetParent();
	}
}

Control * CompositeControl::HitTest(const VEC2 &Pos, VEC2 *LocalPos)
{
	// UWAGA! Ta instrukcja to rozwi¹zanie wielkiego problemu, nad którym siedzia³em jeden
	// wieczór i drugie pó³ dnia. Program siê sypa³ podczas usuwania kontrolki kontenera
	// maj¹cej podkontrolki, ale tylko w trybie Release i przez Ctrl+F5. Nie wiem dok³adnie
	// co to by³o ani jak to rozwi¹zaæ, ale drukowaniami kontrolnymi doszed³em tutaj
	// i ta instrukcja pomog³a :)
	if (m_InDestructor)
		return 0;

	// Przejrzyj podokna
	RECTF ClientRect; GetClientRect(&ClientRect);
	if (PointInRect(Pos, ClientRect))
	{
		RECTF LocalClientRect = RECTF(0.0f, 0.0f, ClientRect.Width(), ClientRect.Height());
		VEC2 ClientPos;
		ClientPos.x = Pos.x - ClientRect.left;
		ClientPos.y = Pos.y - ClientRect.top;
		CONTROL_VECTOR::reverse_iterator rit;
		VEC2 lp;
		Control *R;
		// Kontrolki od ostatniej (najbardziej wierzchniej)
		for (rit = m_SubControls.rbegin(); rit != m_SubControls.rend(); ++rit)
		{
			// Jeœli niewidzialna - nie rozwa¿amy
			if (!(*rit)->GetVisible())
				continue;
			// Prostok¹t
			const RECTF & LocalRect = (*rit)->GetRect();
			// Jeœli nie mieœci siê w ca³oœci
			if (NO_CLIP)
			{
				if (!RectInRect(LocalRect, LocalClientRect))
					continue;
			}
			if (PointInRect(ClientPos, LocalRect))
			{
				lp.x = ClientPos.x - LocalRect.left;
				lp.y = ClientPos.y - LocalRect.top;
				R = (*rit)->HitTest(lp, LocalPos);
				if (R)
					return R;
			}
		}
	}
	// Nie znaleziono - przetestuj sam siebie, jak zwyk³a kontrolka
	return Control::HitTest(Pos, LocalPos);
}

void CompositeControl::GetClientRect(RECTF *Out)
{
	Out->left = 0.0f;
	Out->top = 0.0f;
	const RECTF & Rect = GetRect();
	Out->right = Rect.Width();
	Out->bottom = Rect.Height();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GuiManager

void GuiManager_pimpl::AllControls_Insert(Control *c)
{
	m_AllControls.insert(c);
}

void GuiManager_pimpl::AllControls_Delete(Control *c)
{
	CONTROL_SET::iterator it = m_AllControls.find(c);
	assert(it != m_AllControls.end());
	m_AllControls.erase(it);
}

GuiManager::GuiManager() :
	pimpl(new GuiManager_pimpl)
{
}

GuiManager::~GuiManager()
{
	DeleteControls();

	// Œwiadomie nie kasujê kontrolek g³ównego poziomu. Maj¹ zostaæ skasowane przez ich prawdziwych w³aœcicieli.

	for (uint4 li = 0; li < LAYER_COUNT; li++)
		assert(pimpl->m_Controls[li].empty() && "Nieusuniête kontrolki g³ównego poziomu. Zapomnia³eœ jak¹œ usun¹æ, a musisz to robiæ sam!");

	assert(pimpl->m_AllControls.empty() && "Nieusuniête kontrolki nie g³ównego poziomu - niemo¿liwe!");
}

bool GuiManager::OnKeyDown(uint4 Key)
{
	// Komunikat do kontrolki, która ma ognisko
	if (pimpl->m_FocusedControl)
		pimpl->SendKeyDown(pimpl->m_FocusedControl, Key);
	// ¯adna nie ma ogniska - od razu jako nieobs³u¿ony
	else
	{
		if (OnUnhandledKeyDown)
			OnUnhandledKeyDown(Key);
	}

	return true;
}

bool GuiManager::OnKeyUp(uint4 Key)
{
	// Komunikat do kontrolki, która ma ognisko
	if (pimpl->m_FocusedControl)
		pimpl->SendKeyUp(pimpl->m_FocusedControl, Key);
	// ¯adna nie ma ogniska - od razu jako nieobs³u¿ony
	else
	{
		if (OnUnhandledKeyUp)
			OnUnhandledKeyUp(Key);
	}

	return true;
}

bool GuiManager::OnChar(char Ch)
{
	// Komunikat do kontrolki, która ma ognisko
	if (pimpl->m_FocusedControl)
		pimpl->SendChar(pimpl->m_FocusedControl, Ch);
	// ¯adna nie ma ogniska - od razu jako nieobs³u¿ony
	else
	{
		if (OnUnhandledChar)
			OnUnhandledChar(Ch);
	}

	return true;
}

bool GuiManager::OnMouseMove(const VEC2 &Pos)
{
	// Nowa pozycja
	pimpl->m_MousePos = Pos;
	VEC2 LocalPos;
	pimpl->HandlePossibleMouseOverChange(&LocalPos);

	// Coœ jest przeci¹gane
	if (pimpl->m_Drag)
	{
		assert(pimpl->m_DragControl);
		// Ruch myszy dla kontrolki Ÿród³owej
		if (pimpl->m_DragControl->GetRealEnabled())
		{
			VEC2 DragControlLocalPos;
			pimpl->m_DragControl->ScreenToClient(&DragControlLocalPos, Pos);
			pimpl->SendMouseMove(pimpl->m_DragControl, DragControlLocalPos);
		}
		// Ruch przeci¹gania nad kontrolk¹ docelow¹
		if (pimpl->m_MouseOverControl && pimpl->m_MouseOverControl->GetRealEnabled())
			pimpl->SendDragOver(pimpl->m_MouseOverControl, LocalPos, pimpl->m_DragControl, pimpl->m_DragData);
		return true;
	}
	// Nic nie jest przeci¹gane
	else
	{
		// Ruch myszy w zwi¹zku z kontrolk¹, nad któr¹ jest mysz
		if (pimpl->m_MouseOverControl)
		{
			if (pimpl->m_MouseOverControl->GetRealEnabled())
				pimpl->SendMouseMove(pimpl->m_MouseOverControl, LocalPos);
			return true;
		}
		else
			return false;
	}
}

bool GuiManager::OnMouseButton(const VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	// Pos olewam! To jest OnMouseButton a nie OnMouseMove, nie obs³ugujê tu zmiany pozycji.
	VEC2 LocalPos;
	bool R;

	VEC2 MouseOverLocalPos;
	if (pimpl->m_MouseOverControl)
		pimpl->m_MouseOverControl->ScreenToClient(&MouseOverLocalPos, Pos);

	// Trwa przeci¹ganie
	if (pimpl->m_Drag)
	{
		if (pimpl->m_DragControl->GetRealEnabled())
		{
			// Normalny komunikat myszy dla kontrolki przeci¹ganej
			VEC2 DragLocalPos;
			pimpl->m_DragControl->ScreenToClient(&DragLocalPos, Pos);
			pimpl->SendMouseButton(pimpl->m_DragControl, DragLocalPos, Button, Action);
		}

		// Zwolnienie tego klawisza którym przeci¹gamy - drop
		if (Action == frame::MOUSE_UP && Button == pimpl->m_DragButton)
		{
			// Kontrolka docelowa dostaje OnDragDrop
			bool Result = false;
			if (pimpl->m_MouseOverControl && pimpl->m_MouseOverControl->GetRealEnabled())
			{
				pimpl->SendDragDrop(pimpl->m_MouseOverControl, MouseOverLocalPos, pimpl->m_DragControl, pimpl->m_DragData);
				Result = true;
			}
			// Kontrolka Ÿród³owa dostaje OnDragFinished
			pimpl->SendDragFinished(pimpl->m_DragControl, Result, pimpl->m_MouseOverControl, pimpl->m_DragData);
		}
		// Coœ innego - przerwanie przeci¹gania
		else
		{
			// Kontrolka, nad któr¹ faktycznie jest myszka, dostaje OnDragCancel
			if (pimpl->m_MouseOverControl && pimpl->m_MouseOverControl->GetRealEnabled())
				pimpl->SendDragCancel(pimpl->m_MouseOverControl, MouseOverLocalPos, pimpl->m_DragControl, pimpl->m_DragData);
			// Kontrolka przeci¹gana dostaje OnDragFinished
			// Docelowa jest przy tym równa 0.
			pimpl->SendDragFinished(pimpl->m_DragControl, false, 0, pimpl->m_DragData);
		}

		// Tak czy siak, koniec przeci¹gania

		// Jeœli inna jest kontrolka Ÿród³owa ni¿ docelowa
		if (pimpl->m_DragControl != pimpl->m_MouseOverControl)
		{
			// OnMouseLeave Ÿród³owej, OnMouseEnter docelowej
			pimpl->SendMouseLeave(pimpl->m_DragControl);
			if (pimpl->m_MouseOverControl && pimpl->m_MouseOverControl->GetRealEnabled())
				pimpl->SendMouseEnter(pimpl->m_MouseOverControl);

			pimpl->m_Drag = false;
			pimpl->m_DragControl = 0;
			pimpl->m_DragData.reset();
		}
		else
		{
			pimpl->m_Drag = false;
			pimpl->m_DragControl = 0;
			pimpl->m_DragData.reset();
		}

		// Uwaga! Tu by³ b³¹d.
		// A przecie¿ zmiana kursora dotyczy tak¿e sytuacji kiedy upuœciliœmy nad t¹ sam¹ kontrolk¹
		// z której przeci¹gamy - to nie ma znaczenia. Teraz ju¿ poprawi³em.

		// Zmiana kursora
		if (pimpl->m_MouseOverControl)
			pimpl->ChangeCurrentCursor(pimpl->m_MouseOverControl->GetCursor());
		else
			pimpl->ChangeCurrentCursor(0);

		R = true;
	}
	// Nie trwa przeci¹ganie
	else
	{
		// Jeœli kursor jest nad czymœ
		if (pimpl->m_MouseOverControl)
		{
			// Jeœli to coœ jest aktywne
			if (pimpl->m_MouseOverControl->GetRealEnabled())
			{
				// Wciœniêcie klawisza - mo¿e siê rozpocz¹æ przeci¹ganie
				if (Action == frame::MOUSE_DOWN || Action == frame::MOUSE_DBLCLK)
				{
					pimpl->m_DragControl = pimpl->m_MouseOverControl;
					pimpl->m_DragButton = Button;
				}

				// Komunikat do kontrolki, nad któr¹ naprawdê jest kursor myszy
				pimpl->SendMouseButton(pimpl->m_MouseOverControl, MouseOverLocalPos, Button, Action);
			}

			R = true;
		}
		else
			R = false;
	}

	// Kontrolka, nad któr¹ faktycznie jest mysz, dostaje ognisko
	// NEW: Ale tylko jeœli to wciœniêcie klawisza myszy.
	if (Action == frame::MOUSE_DOWN && pimpl->m_MouseOverControl)
	{
		// Nawet jeœli ogniska nie bêdzie (to siê oka¿e dopiero wewn¹trz SetFocus), wrzuæ to okno na wierzch
		pimpl->m_MouseOverControl->GetTopLevelParent()->BringToFront();
		// oraz jeœli to nie okno z warstwy Volatile, schowaj wszystkie z tej warstwy
		LAYER Layer;
		g_GuiManager->FindControl(pimpl->m_MouseOverControl->GetTopLevelParent(), &Layer, 0);
		if (Layer != LAYER_VOLATILE)
			pimpl->HideVolatileControls();

		pimpl->SetFocus(pimpl->m_MouseOverControl);
	}

	return R;
}

bool GuiManager::OnMouseWheel(const VEC2 &Pos, float Delta)
{
	// Pos olewam! To jest OnMouseButton a nie OnMouseMove, nie obs³ugujê tu zmiany pozycji.
	// Przeci¹ganie te¿ mnie nie obchodzi.

	if (pimpl->m_MouseOverControl && pimpl->m_MouseOverControl->GetRealEnabled())
	{
		VEC2 LocalPos;
		pimpl->m_MouseOverControl->ScreenToClient(&LocalPos, Pos);
		pimpl->SendMouseWheel(pimpl->m_MouseOverControl, LocalPos, Delta);
		return true;
	}
	else
		return false;
}

void GuiManager::OnFrame()
{
	// Kolejka komunikatów
	while (!pimpl->m_MessageQueue.empty())
	{
		// Pobierz nastêpny komunikat
		MESSAGE & msg = pimpl->m_MessageQueue.front();
		// Jeœli kontrolka docelowa jeszcze istnieje
		if (ControlExists(msg.m_Dest))
		{
			// Zale¿nie od rodzaju
			switch (msg.m_Type)
			{
			case MESSAGE::ON_ENABLE:
				msg.m_Dest->OnEnable();
				break;
			case MESSAGE::ON_DISABLE:
				msg.m_Dest->OnDisable();
				break;
			case MESSAGE::ON_SHOW:
				msg.m_Dest->OnShow();
				break;
			case MESSAGE::ON_HIDE:
				msg.m_Dest->OnHide();
				break;
			case MESSAGE::ON_FOCUS_ENTER:
				msg.m_Dest->OnFocusEnter();
				break;
			case MESSAGE::ON_FOCUS_LEAVE:
				msg.m_Dest->OnFocusLeave();
				break;
			case MESSAGE::ON_RECT_CHANGE:
				msg.m_Dest->OnRectChange();
				break;
			case MESSAGE::ON_MOUSE_ENTER:
				msg.m_Dest->OnMouseEnter();
				break;
			case MESSAGE::ON_MOUSE_LEAVE:
				msg.m_Dest->OnMouseLeave();
				break;
			case MESSAGE::ON_MOUSE_MOVE:
				msg.m_Dest->OnMouseMove(msg.m_Pos);
				break;
			case MESSAGE::ON_MOUSE_BUTTON:
				msg.m_Dest->OnMouseButton(msg.m_Pos, msg.m_MouseButton, msg.m_MouseAction);
				break;
			case MESSAGE::ON_MOUSE_WHEEL:
				msg.m_Dest->OnMouseWheel(msg.m_Pos, msg.m_Delta);
				break;
			case MESSAGE::ON_DRAG_ENTER:
				msg.m_Dest->OnDragEnter(msg.m_Control, msg.m_DragData);
				break;
			case MESSAGE::ON_DRAG_LEAVE:
				msg.m_Dest->OnDragLeave(msg.m_Control, msg.m_DragData);
				break;
			case MESSAGE::ON_DRAG_OVER:
				msg.m_Dest->OnDragOver(msg.m_Pos, msg.m_Control, msg.m_DragData);
				break;
			case MESSAGE::ON_DRAG_DROP:
				msg.m_Dest->OnDragDrop(msg.m_Pos, msg.m_Control, msg.m_DragData);
				break;
			case MESSAGE::ON_DRAG_CANCEL:
				msg.m_Dest->OnDragCancel(msg.m_Pos, msg.m_Control, msg.m_DragData);
				break;
			case MESSAGE::ON_DRAG_FINISHED:
				msg.m_Dest->OnDragFinished(msg.m_B, msg.m_Control, msg.m_DragData);
				break;
			// Teraz bêdzie obs³ugiwanie komunikatów z klawiatury
			// Od danej kontrolki w górê drzewa
			// Wszystkie kontrolki nadrzêdne do m_Dest s¹ Visible i Enabled, bo inaczej ta nie mia³aby ogniska i nie trafi³yby do niej komunikaty z klawiatury.
			// Natomiast nie musz¹ byæ Focusable i o to chodzi! W ten sposób kontrolki które same w sobie nie s¹ Focusable mog¹ obs³ugiwaæ klawisze takie jak Tab itp.
			case MESSAGE::ON_KEY_DOWN:
				{
					Control *c = msg.m_Dest;
					bool Processed = false;
					do
					{
						if (c->OnKeyDown(msg.m_U))
						{
							Processed = true;
							break;
						}
						c = c->GetParent();
					} while (c != 0 && ControlExists(c));
					// Dosz³o a¿ tutaj - nieobs³u¿one a¿ do korzenia
					if (!Processed && OnUnhandledKeyDown)
						OnUnhandledKeyDown(msg.m_U);
				}
				break;
			case MESSAGE::ON_KEY_UP:
				{
					Control *c = msg.m_Dest;
					bool Processed = false;
					do
					{
						if (c->OnKeyUp(msg.m_U))
						{
							Processed = true;
							break;
						}
						c = c->GetParent();
					} while (c != 0 && ControlExists(c));
					// Dosz³o a¿ tutaj - nieobs³u¿one a¿ do korzenia
					if (!Processed && OnUnhandledKeyUp)
						OnUnhandledKeyUp(msg.m_U);
				}
				break;
			case MESSAGE::ON_KEY_CHAR:
				{
					Control *c = msg.m_Dest;
					do
					{
						if (c->OnChar(msg.m_C))
							break;
						c = c->GetParent();
					} while (c != 0 && ControlExists(c));
					// Dosz³o a¿ tutaj - nieobs³u¿one a¿ do korzenia
					if (OnUnhandledChar)
						OnUnhandledChar(msg.m_C);
				}
				break;
			default:
				assert("Nieznany typ komunikatu gui::MESSAGE::TYPE");
				break;
			}
		}
		// Usuñ ten komunikat
		pimpl->m_MessageQueue.pop();
	}

	// Usuñ kontrolki do usuniêcia
	DeleteControls();
}

void GuiManager::DeleteControls()
{
	while (!pimpl->m_ControlsToRelease.empty())
	{
		if (ControlExists(*pimpl->m_ControlsToRelease.begin()))
			delete *pimpl->m_ControlsToRelease.begin();
		pimpl->m_ControlsToRelease.erase(pimpl->m_ControlsToRelease.begin());
	}
}

void GuiManager::OnDraw()
{
	CONTROL_VECTOR::iterator it;
	VEC2 LocalPos;
	// Warstwy od najbardziej spodniej
	for (uint4 Layer = 0; Layer < LAYER_COUNT; Layer++)
	{
		// Kontrolki od pierwszej (najbardziej spodniej)
		for (it = pimpl->m_Controls[Layer].begin(); it != pimpl->m_Controls[Layer].end(); ++it)
		{
			// Jeœli niewidzialna - nie rozwa¿amy
			if (!(*it)->GetVisible())
				continue;
			// Prostok¹t
			const RECTF & LocalRect = (*it)->GetRect();
			(*it)->Draw(VEC2(LocalRect.left, LocalRect.top));
		}
	}
	
	// Hint
	if (dynamic_cast<PopupHint*>(pimpl->m_CurrentHint) && frame::Timer1.GetTime() >= pimpl->m_CurrentHintStartTime)
	{
		if (!pimpl->m_CurrentHintPosSet)
		{
			pimpl->m_CurrentHintPos = pimpl->m_MousePos;
			pimpl->m_CurrentHintPosSet = true;
		}
		static_cast<PopupHint*>(pimpl->m_CurrentHint)->OnDraw(pimpl->m_CurrentHintPos);
		pimpl->m_LastHintDrawTime = frame::Timer1.GetTime();
	}
}

uint4 GuiManager::GetControlCount(LAYER Layer)
{
	return pimpl->m_Controls[Layer].size();
}

Control * GuiManager::GetControl(LAYER Layer, uint4 Index)
{
	if (Index < pimpl->m_Controls[Layer].size())
		return pimpl->m_Controls[Layer][Index];
	else
		return 0;
}

uint4 GuiManager::FindControl(LAYER Layer, Control *Control)
{
	for (size_t i = pimpl->m_Controls[Layer].size(); i--; )
	{
		if (pimpl->m_Controls[Layer][i] == Control)
			return i;
	}
	return MAXUINT4;
}

void GuiManager::FindControl(Control *c, LAYER *Layer, uint4 *Index)
{
	for (uint4 l = 0; l < LAYER_COUNT; l++)
	{
		for (uint4 i = 0; i < pimpl->m_Controls[l].size(); i++)
		{
			if (pimpl->m_Controls[l][i] == c)
			{
				if (Layer)
					*Layer = (LAYER)l;
				if (Index)
					*Index = i;
				return;
			}
		}
	}
}

bool GuiManager::ControlExists(Control *c)
{
	return pimpl->m_AllControls.find(c) != pimpl->m_AllControls.end();
}

Control * GuiManager::HitTest(const VEC2 &Pos, VEC2 *LocalPos)
{
	CONTROL_VECTOR::reverse_iterator rit;
	VEC2 lp;
	Control *R;
	// Warstwy od najbardziej wierzchniej
	for (uint4 Layer = LAYER_COUNT; Layer--; )
	{
		// Kontrolki od ostatniej (najbardziej wierzchniej)
		for (rit = pimpl->m_Controls[Layer].rbegin(); rit != pimpl->m_Controls[Layer].rend(); ++rit)
		{
			// Jeœli niewidzialna - nie rozwa¿amy
			if (!(*rit)->GetVisible())
				continue;
			// Prostok¹t
			const RECTF & LocalRect = (*rit)->GetRect();
			if (PointInRect(Pos, LocalRect))
			{
				lp.x = Pos.x - LocalRect.left;
				lp.y = Pos.y - LocalRect.top;
				R = (*rit)->HitTest(lp, LocalPos);
				if (R)
					return R;
			}
		}
	}
	// Nie znaleziono
	return 0;
}

const VEC2 & GuiManager::GetMousePos()
{
	return pimpl->m_MousePos;
}

Control * GuiManager::GetMouseOverControl()
{
	return pimpl->m_MouseOverControl;
}

bool GuiManager::IsDrag()
{
	return pimpl->m_Drag;
}

Control * GuiManager::GetDragControl()
{
	return pimpl->m_DragControl;
}

frame::MOUSE_BUTTON GuiManager::GetDragButton()
{
	return pimpl->m_DragButton;
}

DRAG_DATA_SHARED_PTR GuiManager::GetDragData()
{
	return pimpl->m_DragData;
}

Control * GuiManager::GetFocusedControl()
{
	return pimpl->m_FocusedControl;
}

void GuiManager::ResetFocus()
{
	pimpl->SetFocus(0);
}

Cursor * GuiManager::GetCurrentCursor()
{
	return pimpl->m_CurrentCursor;
}

Hint * GuiManager::GetCurrentHint()
{
	return pimpl->m_CurrentHint;
}

void GuiManager::HideVolatileControls()
{
	pimpl->HideVolatileControls();
}

GuiManager *g_GuiManager = NULL;


} // namespace gui
