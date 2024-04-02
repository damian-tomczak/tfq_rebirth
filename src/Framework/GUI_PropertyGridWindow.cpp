/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "GUI_PropertyGridWindow.hpp"

namespace gui
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PropertyBase

PropertyBase::PropertyBase(PropertyGridWindow *Parent, CHANGED_EVENT OnChanged) :
	gui::CompositeControl(Parent, false, RECTF::ZERO), OnChanged(OnChanged)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa BoolProperty

const char * const BOOL_PROPERTY_CHECKBOX_TEXT = "Tak";

void BoolProperty::CheckBoxClick(gui::Control *c)
{
	ValueChanged(m_CheckBox->StateToBool(m_CheckBox->GetState()));
}

void BoolProperty::OnRectChange()
{
	RECTF ClientRect; GetClientRect(&ClientRect);
	ClientRect -= VEC2(ClientRect.left, ClientRect.top);

	m_CheckBox->SetRect(ClientRect);
}

void BoolProperty::ValueSet(const bool &x)
{
	m_CheckBox->SetState(m_CheckBox->BoolToState(x));
}

BoolProperty::BoolProperty(PropertyGridWindow *Parent, bool InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	PropertyBase_t<bool>(Parent, OnChanged, OnSetValue)
{
	m_CheckBox = new gui::CheckBox(this, RECTF::ZERO, gui::CheckBox::BoolToState(InitialValue), BOOL_PROPERTY_CHECKBOX_TEXT, true);
	m_CheckBox->OnClick.bind(this, &BoolProperty::CheckBoxClick);

	OnRectChange();
}

BoolProperty::BoolProperty(PropertyGridWindow *Parent, bool *DirectValue) :
	PropertyBase_t<bool>(Parent, NULL, NULL)
{
	m_CheckBox = new gui::CheckBox(this, RECTF::ZERO, gui::CheckBox::BoolToState(*DirectValue), BOOL_PROPERTY_CHECKBOX_TEXT, true);
	m_CheckBox->OnClick.bind(this, &BoolProperty::CheckBoxClick);

	this->DirectValue = DirectValue;

	OnRectChange();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa StringProperty

void StringProperty::EditChange(gui::Control *c)
{
	ValueChanged(m_Edit->GetText());
}

void StringProperty::OnRectChange()
{
	RECTF ClientRect; GetClientRect(&ClientRect);
	ClientRect -= VEC2(ClientRect.left, ClientRect.top);

	m_Edit->SetRect(ClientRect);
}

void StringProperty::ValueSet(const string &x)
{
	m_Edit->SetText(x);
}

StringProperty::StringProperty(PropertyGridWindow *Parent, const string &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	PropertyBase_t<string>(Parent, OnChanged, OnSetValue)
{
	m_Edit = new gui::Edit(this, RECTF::ZERO);
	m_Edit->SetText(InitialValue);
	m_Edit->OnChange.bind(this, &StringProperty::EditChange);

	OnRectChange();
}

StringProperty::StringProperty(PropertyGridWindow *Parent, string *DirectValue) :
	PropertyBase_t<string>(Parent, NULL, NULL)
{
	m_Edit = new gui::Edit(this, RECTF::ZERO);
	m_Edit->SetText(*DirectValue);
	m_Edit->OnChange.bind(this, &StringProperty::EditChange);

	this->DirectValue = DirectValue;

	OnRectChange();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa FloatProperty_TrackBar

const float FLOAT_PROPERTY_LABEL_CX = 52.f;

void FloatProperty_TrackBar::UpdateLabelText()
{
	string s;
	FormatLabelText(&s);
	m_Label->SetText(s);
}

void FloatProperty_TrackBar::TrackBarValueChange(gui::Control *c)
{
	ValueChanged(m_TrackBar->GetValueF());
	UpdateLabelText();
}

void FloatProperty_TrackBar::OnRectChange()
{
	RECTF ClientRect; GetClientRect(&ClientRect);
	ClientRect -= VEC2(ClientRect.left, ClientRect.top);

	m_TrackBar->SetRect(RECTF(ClientRect.left, ClientRect.top, ClientRect.right-FLOAT_PROPERTY_LABEL_CX, ClientRect.bottom));
	m_Label->SetRect(RECTF(ClientRect.right-FLOAT_PROPERTY_LABEL_CX+MARGIN, ClientRect.top, ClientRect.right, ClientRect.bottom));
}

void FloatProperty_TrackBar::ValueSet(const float &x)
{
	m_TrackBar->SetValueF(x);
	UpdateLabelText();
}

void FloatProperty_TrackBar::FormatLabelText(string *Out)
{
	FloatToStr(Out, m_TrackBar->GetValueF(), 'g', 3);
}

FloatProperty_TrackBar::FloatProperty_TrackBar(PropertyGridWindow *Parent, float Min, float Max, float Step, float InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	PropertyBase_t<float>(Parent, OnChanged, OnSetValue)
{
	m_TrackBar = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
	m_TrackBar->SetRangeF(Min, Max, Step);
	m_TrackBar->SetValueF(InitialValue);
	m_TrackBar->OnValueChange.bind(this, &FloatProperty_TrackBar::TrackBarValueChange);

	m_Label = new gui::Label(this, RECTF::ZERO, string(), res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
	UpdateLabelText();

	OnRectChange();
}

FloatProperty_TrackBar::FloatProperty_TrackBar(PropertyGridWindow *Parent, float Min, float Max, float Step, float *DirectValue) :
	PropertyBase_t<float>(Parent, OnChanged, OnSetValue)
{
	m_TrackBar = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
	m_TrackBar->SetRangeF(Min, Max, Step);
	m_TrackBar->SetValueF(*DirectValue);
	m_TrackBar->OnValueChange.bind(this, &FloatProperty_TrackBar::TrackBarValueChange);

	m_Label = new gui::Label(this, RECTF::ZERO, string(), res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
	UpdateLabelText();

	this->DirectValue = DirectValue;

	OnRectChange();
}

void FloatProperty_TrackBar::SetRange(float Min, float Max, float Step)
{
	m_TrackBar->SetRangeF(Min, Max, Step);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PercentProperty

void PercentProperty::FormatLabelText(string *Out)
{
	IntToStr(Out, (int)(GetValue()*100.f+0.5f));
	Out->append("%");
}

PercentProperty::PercentProperty(PropertyGridWindow *Parent, float InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	FloatProperty_TrackBar(Parent, 0.f, 1.f, 0.01f, InitialValue, OnChanged, OnSetValue)
{
	UpdateLabelText();
}

PercentProperty::PercentProperty(PropertyGridWindow *Parent, float *DirectValue) :
	FloatProperty_TrackBar(Parent, 0.f, 1.f, 0.01f, DirectValue)
{
	UpdateLabelText();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa IntProperty_TrackBar

const float INT_PROPERTY_LABEL_CX = 52.f;

void IntProperty_TrackBar::UpdateLabelText()
{
	string s; IntToStr(&s, m_TrackBar->GetValueI());
	m_Label->SetText(s);
}

void IntProperty_TrackBar::TrackBarValueChange(gui::Control *c)
{
	ValueChanged(m_TrackBar->GetValueI());
	UpdateLabelText();
}

void IntProperty_TrackBar::OnRectChange()
{
	RECTF ClientRect; GetClientRect(&ClientRect);
	ClientRect -= VEC2(ClientRect.left, ClientRect.top);

	m_TrackBar->SetRect(RECTF(ClientRect.left, ClientRect.top, ClientRect.right-INT_PROPERTY_LABEL_CX, ClientRect.bottom));
	m_Label->SetRect(RECTF(ClientRect.right-INT_PROPERTY_LABEL_CX+MARGIN, ClientRect.top, ClientRect.right, ClientRect.bottom));
}

void IntProperty_TrackBar::ValueSet(const int &x)
{
	m_TrackBar->SetValueI(x);
	UpdateLabelText();
}

IntProperty_TrackBar::IntProperty_TrackBar(PropertyGridWindow *Parent, int Min, int Max, int Step, int InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	PropertyBase_t<int>(Parent, OnChanged, OnSetValue)
{
	m_TrackBar = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, false, true);
	m_TrackBar->SetRangeI(Min, Max, Step);
	m_TrackBar->SetValueI(InitialValue);
	m_TrackBar->OnValueChange.bind(this, &IntProperty_TrackBar::TrackBarValueChange);

	m_Label = new gui::Label(this, RECTF::ZERO, string(), res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
	UpdateLabelText();

	OnRectChange();
}

IntProperty_TrackBar::IntProperty_TrackBar(PropertyGridWindow *Parent, int Min, int Max, int Step, int *DirectValue) :
	PropertyBase_t<int>(Parent, OnChanged, OnSetValue)
{
	m_TrackBar = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, false, true);
	m_TrackBar->SetRangeI(Min, Max, Step);
	m_TrackBar->SetValueI(*DirectValue);
	m_TrackBar->OnValueChange.bind(this, &IntProperty_TrackBar::TrackBarValueChange);

	m_Label = new gui::Label(this, RECTF::ZERO, string(), res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
	UpdateLabelText();

	this->DirectValue = DirectValue;

	OnRectChange();
}

void IntProperty_TrackBar::SetRange(int Min, int Max, int Step)
{
	m_TrackBar->SetRangeI(Min, Max, Step);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa FloatPairProperty

const float FLOAT_PAIR_PROPERTY_LABEL_CX = 64.f;

void FloatPairProperty::TrackBarValueChange(gui::Control *c)
{
	ValueChanged(VEC2(m_TrackBars[0]->GetValueF(), m_TrackBars[1]->GetValueF()));
	UpdateLabelText();
}

void FloatPairProperty::FormatLabelText(string *Out)
{
	string s1, s2;
	FloatToStr(&s1, m_TrackBars[0]->GetValueF(), 'g', 2);
	FloatToStr(&s2, m_TrackBars[1]->GetValueF(), 'g', 2);
	*Out = s1 + "," + s2;
}

void FloatPairProperty::UpdateLabelText()
{
	string s;
	FormatLabelText(&s);
	m_Label->SetText(s);
}

void FloatPairProperty::OnRectChange()
{
	RECTF ClientRect; GetClientRect(&ClientRect);
	ClientRect -= VEC2(ClientRect.left, ClientRect.top);

	float x1 = 0.f;
	float x6 = ClientRect.right;
	float x5 = x6 - FLOAT_PAIR_PROPERTY_LABEL_CX;
	float x4 = x5 - MARGIN;
	float x2 = (x5 - MARGIN) * 0.5f;
	float x3 = x2 + MARGIN;

	m_TrackBars[0]->SetRect(RECTF(x1, ClientRect.top, x2, ClientRect.bottom));
	m_TrackBars[1]->SetRect(RECTF(x3, ClientRect.top, x4, ClientRect.bottom));
	m_Label->SetRect(       RECTF(x5, ClientRect.top, x6, ClientRect.bottom));
}

void FloatPairProperty::ValueSet(const VEC2 &x)
{
	m_TrackBars[0]->SetValueF(x.x);
	m_TrackBars[1]->SetValueF(x.y);
	UpdateLabelText();
}

FloatPairProperty::FloatPairProperty(PropertyGridWindow *Parent, const VEC2 &Min, const VEC2 &Max, const VEC2 &Step, const VEC2 &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	PropertyBase_t<VEC2>(Parent, OnChanged, OnSetValue)
{
	m_TrackBars[0] = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
	m_TrackBars[0]->SetRangeF(Min.x, Max.x, Step.x);
	m_TrackBars[0]->SetValueF(InitialValue.x);
	m_TrackBars[0]->OnValueChange.bind(this, &FloatPairProperty::TrackBarValueChange);

	m_TrackBars[1] = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
	m_TrackBars[1]->SetRangeF(Min.y, Max.y, Step.y);
	m_TrackBars[1]->SetValueF(InitialValue.y);
	m_TrackBars[1]->OnValueChange.bind(this, &FloatPairProperty::TrackBarValueChange);

	m_Label = new gui::Label(this, RECTF::ZERO, string(), res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
	UpdateLabelText();

	OnRectChange();
}

FloatPairProperty::FloatPairProperty(PropertyGridWindow *Parent, const VEC2 &Min, const VEC2 &Max, const VEC2 &Step, VEC2 *DirectValue) :
	PropertyBase_t<VEC2>(Parent, OnChanged, OnSetValue)
{
	m_TrackBars[0] = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
	m_TrackBars[0]->SetRangeF(Min.x, Max.x, Step.x);
	m_TrackBars[0]->SetValueF(DirectValue->x);
	m_TrackBars[0]->OnValueChange.bind(this, &FloatPairProperty::TrackBarValueChange);

	m_TrackBars[1] = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
	m_TrackBars[1]->SetRangeF(Min.y, Max.y, Step.y);
	m_TrackBars[1]->SetValueF(DirectValue->y);
	m_TrackBars[1]->OnValueChange.bind(this, &FloatPairProperty::TrackBarValueChange);

	m_Label = new gui::Label(this, RECTF::ZERO, string(), res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
	UpdateLabelText();

	this->DirectValue = DirectValue;

	OnRectChange();
}

void FloatPairProperty::SetRange(const VEC2 &Min, const VEC2 &Max, const VEC2 &Step)
{
	m_TrackBars[0]->SetRangeF(Min.x, Max.x, Step.x);
	m_TrackBars[1]->SetRangeF(Min.y, Max.y, Step.y);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa EnumProperty

uint4 EnumProperty::ValueToSelIndex(uint4 Value)
{
	// Optymalizacja
	if (Value < m_ValueVector.size() && m_ValueVector[Value].second == Value)
		return Value;
	if (Value > 0 && Value-1 < m_ValueVector.size() && m_ValueVector[Value-1].second == Value)
		return Value-1;

	for (size_t i = 0; i < m_ValueVector.size(); i++)
	{
		if (m_ValueVector[i].second == Value)
			return i;
	}

	return MAXUINT4;
}

void EnumProperty::ListSelChange(gui::Control *c)
{
	ValueChanged(SelIndexToValue(m_List->GetModel().GuiList_GetSelIndex()));
}

void EnumProperty::OnRectChange()
{
	RECTF ClientRect; GetClientRect(&ClientRect);
	ClientRect -= VEC2(ClientRect.left, ClientRect.top);

	m_ComboBox->SetRect(ClientRect);
	m_List->SetRect(RECTF(0.f, 0.f, ClientRect.Width(), CONTROL_HEIGHT*5.f));
}

void EnumProperty::ValueSet(const uint4 &x)
{
	m_List->GetModel().GuiList_SetSelIndex(ValueToSelIndex(x));
}

EnumProperty::EnumProperty(PropertyGridWindow *Parent, const VALUE_VECTOR &ValueVector, uint4 InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	PropertyBase_t<uint4>(Parent, OnChanged, OnSetValue),
	m_ValueVector(ValueVector)
{
	m_List.reset(new gui::StringList(NULL, RECTF::ZERO));
	for (size_t i = 0; i < ValueVector.size(); i++)
		m_List->GetModel().Add(ValueVector[i].first);
	m_List->GetModel().GuiList_SetSelIndex(ValueToSelIndex(InitialValue));
	m_List->OnSelChange.bind(this, &EnumProperty::ListSelChange);

	m_ComboBox = new gui::ListComboBox(this, RECTF::ZERO, true, m_List.get());

	OnRectChange();
}

EnumProperty::EnumProperty(PropertyGridWindow *Parent, const VALUE_VECTOR &ValueVector, uint4 *DirectValue) :
	PropertyBase_t<uint4>(Parent, OnChanged, OnSetValue),
	m_ValueVector(ValueVector)
{
	m_List.reset(new gui::StringList(NULL, RECTF::ZERO));
	for (size_t i = 0; i < ValueVector.size(); i++)
		m_List->GetModel().Add(ValueVector[i].first);
	m_List->GetModel().GuiList_SetSelIndex(ValueToSelIndex(*DirectValue));
	m_List->OnSelChange.bind(this, &EnumProperty::ListSelChange);

	m_ComboBox = new gui::ListComboBox(this, RECTF::ZERO, true, m_List.get());

	this->DirectValue = DirectValue;

	OnRectChange();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Vec3PosProperty

void Vec3PosProperty::SetEditText(const VEC3 &v)
{
	string s;
	for (size_t i = 0; i < 3; i++)
	{
		FloatToStr(&s, v[i], 'g', 3);
		m_Edit[i]->SetText(s);
	}
}

void Vec3PosProperty::EditChange(gui::Control *c)
{
	for (size_t i = 0; i < 3; i++)
	{
		float v;
		if (StrToSth<float>(&v, m_Edit[i]->GetText()))
			m_Value[i] = v;
	}
	ValueChanged(m_Value);
}

void Vec3PosProperty::OnRectChange()
{
	RECTF ClientRect; GetClientRect(&ClientRect);
	ClientRect -= VEC2(ClientRect.left, ClientRect.top);

	float EditCX = (ClientRect.Width() - MARGIN * 2.f) / 3.f;
	RECTF r = ClientRect;
	r.right = r.left + EditCX;
	m_Edit[0]->SetRect(r);

	r.left = r.right + MARGIN;
	r.right = r.left + EditCX;
	m_Edit[1]->SetRect(r);

	r.left = r.right + MARGIN;
	r.right = r.left + EditCX;
	m_Edit[2]->SetRect(r);
}

void Vec3PosProperty::ValueSet(const VEC3 &x)
{
	m_Value = x;
	SetEditText(x);
}

Vec3PosProperty::Vec3PosProperty(PropertyGridWindow *Parent, const VEC3 &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	PropertyBase_t<VEC3>(Parent, OnChanged, OnSetValue),
	m_Value(InitialValue)
{
	for (size_t i = 0; i < 3; i++)
		m_Edit[i] = new gui::Edit(this, RECTF::ZERO);
	SetEditText(InitialValue);
	for (size_t i = 0; i < 3; i++)
		m_Edit[i]->OnChange.bind(this, &Vec3PosProperty::EditChange);

	OnRectChange();
}

Vec3PosProperty::Vec3PosProperty(PropertyGridWindow *Parent, VEC3 *DirectValue) :
	PropertyBase_t<VEC3>(Parent, NULL, NULL),
	m_Value(*DirectValue)
{
	for (size_t i = 0; i < 3; i++)
		m_Edit[i] = new gui::Edit(this, RECTF::ZERO);
	SetEditText(*DirectValue);
	for (size_t i = 0; i < 3; i++)
		m_Edit[i]->OnChange.bind(this, &Vec3PosProperty::EditChange);

	this->DirectValue = DirectValue;

	OnRectChange();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Vec3DirProperty

void Vec3DirProperty::SetLabelText(const VEC3 &v)
{
	string s, tmp;

	FloatToStr(&tmp, v.x, 'f', 2);
	s += tmp;
	s += ',';
	FloatToStr(&tmp, v.y, 'f', 2);
	s += tmp;
	s += ',';
	FloatToStr(&tmp, v.z, 'f', 2);
	s += tmp;

	m_Label->SetText(s);
}

void Vec3DirProperty::AnglesToDir(VEC3 *Out, float Yaw, float Pitch)
{
	SphericalToCartesian(Out, Yaw, Pitch, 1.f);
}

void Vec3DirProperty::DirToAngles(float *OutYaw, float *OutPitch, const VEC3 &Dir)
{
	CartesianToSpherical(OutYaw, OutPitch, NULL, Dir);
}

void Vec3DirProperty::TrackBarValueChange(gui::Control *c)
{
	AnglesToDir(&m_Value, m_TrackBar[0]->GetValueF(), m_TrackBar[1]->GetValueF());
	SetLabelText(m_Value);
	ValueChanged(m_Value);
}

void Vec3DirProperty::OnRectChange()
{
	RECTF ClientRect; GetClientRect(&ClientRect);

	const float DIV = 0.5f;

	float y1 = 0.f;
	float y2 = ClientRect.Height();
	float x1 = 0.f;
	float x6 = ClientRect.Width();
	float x4 = x6 * DIV - gui::MARGIN * 0.5f;
	float x5 = x4 + gui::MARGIN;
	float x2 = x4 * 0.5f - gui::MARGIN * 0.5f;
	float x3 = x2 + gui::MARGIN;

	m_TrackBar[0]->SetRect(RECTF(x1, y1, x2, y2));
	m_TrackBar[1]->SetRect(RECTF(x3, y1, x4, y2));
	m_Label->SetRect(RECTF(x5, y1, x6, y2));
}

void Vec3DirProperty::ValueSet(const VEC3 &x)
{
	Normalize(&m_Value, x);
	float Yaw, Pitch;
	DirToAngles(&Yaw, &Pitch, m_Value);

	m_TrackBar[0]->SetValueF(Yaw);
	m_TrackBar[1]->SetValueF(Pitch);

	SetLabelText(m_Value);
}

Vec3DirProperty::Vec3DirProperty(PropertyGridWindow *Parent, const VEC3 &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	PropertyBase_t<VEC3>(Parent, OnChanged, OnSetValue)
{
	Normalize(&m_Value, InitialValue);
	float Yaw, Pitch;
	DirToAngles(&Yaw, &Pitch, m_Value);

	m_TrackBar[0] = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
	m_TrackBar[1] = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
	m_TrackBar[0]->SetRangeF(0.f, PI_X_2, PI_X_2/36.f);
	m_TrackBar[1]->SetRangeF(-PI_2, PI_2, PI/18.f);
	m_TrackBar[0]->SetValueF(Yaw);
	m_TrackBar[1]->SetValueF(Pitch);
	m_TrackBar[0]->OnValueChange.bind(this, &Vec3DirProperty::TrackBarValueChange);
	m_TrackBar[1]->OnValueChange.bind(this, &Vec3DirProperty::TrackBarValueChange);

	m_Label = new gui::Label(this, RECTF::ZERO, string(), res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
	SetLabelText(m_Value);

	OnRectChange();
}

Vec3DirProperty::Vec3DirProperty(PropertyGridWindow *Parent, VEC3 *DirectValue) :
	PropertyBase_t<VEC3>(Parent, NULL, NULL)
{
	Normalize(&m_Value, *DirectValue);
	float Yaw, Pitch;
	DirToAngles(&Yaw, &Pitch, m_Value);

	m_TrackBar[0] = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
	m_TrackBar[1] = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
	m_TrackBar[0]->SetRangeF(0.f, PI_X_2, PI_X_2/36.f);
	m_TrackBar[1]->SetRangeF(-PI_2, PI_2, PI/18.f);
	m_TrackBar[0]->SetValueF(Yaw);
	m_TrackBar[1]->SetValueF(Pitch);
	m_TrackBar[0]->OnValueChange.bind(this, &Vec3DirProperty::TrackBarValueChange);
	m_TrackBar[1]->OnValueChange.bind(this, &Vec3DirProperty::TrackBarValueChange);

	m_Label = new gui::Label(this, RECTF::ZERO, string(), res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
	SetLabelText(m_Value);

	this->DirectValue = DirectValue;

	OnRectChange();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ColorfProperty

void ColorfProperty::SetLabelText(const COLORF &v)
{
	string s;
	UintToStr2(&s, ColorfToColor(v).ARGB, 8, 16);
	m_Label->SetText(s);
}

void ColorfProperty::TrackBarValueChangeV()
{
	m_Value.A = m_TrackBar[0]->GetValueF();
	m_Value.R = m_TrackBar[1]->GetValueF();
	m_Value.G = m_TrackBar[2]->GetValueF();
	m_Value.B = m_TrackBar[3]->GetValueF();

	SetLabelText(m_Value);
	ValueChanged(m_Value);
}

void ColorfProperty::OnDraw(const VEC2 &Translation)
{
	RECTF r = RECTF(GetRect().Width()-gui::CONTROL_HEIGHT, 0.f, GetRect().Width(), GetRect().Height());
	//r += Translation;

	gfx2d::g_Canvas->SetSprite(NULL);
	gfx2d::g_Canvas->SetColor(ColorfToColor(m_Value));
	gfx2d::g_Canvas->DrawRect(r);
}

void ColorfProperty::ValueSet(const COLORF &x)
{
	m_Value = x;

	m_TrackBar[0]->SetValueF(x.R); m_TrackBar[1]->SetValueF(x.R);
	m_TrackBar[2]->SetValueF(x.G); m_TrackBar[3]->SetValueF(x.B);

	SetLabelText(x);
}

ColorfProperty::ColorfProperty(PropertyGridWindow *Parent, const COLORF &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	ColorBaseProperty<COLORF>(Parent, OnChanged, OnSetValue), m_Value(InitialValue)
{
	CreateControls();
	m_TrackBar[0]->SetValueF(InitialValue.A); m_TrackBar[1]->SetValueF(InitialValue.R);
	m_TrackBar[2]->SetValueF(InitialValue.G); m_TrackBar[3]->SetValueF(InitialValue.B);
	SetLabelText(m_Value);
	OnRectChange();
}

ColorfProperty::ColorfProperty(PropertyGridWindow *Parent, COLORF *DirectValue) :
	ColorBaseProperty<COLORF>(Parent, DirectValue), m_Value(*DirectValue)
{
	CreateControls();
	m_TrackBar[0]->SetValueF(DirectValue->A); m_TrackBar[1]->SetValueF(DirectValue->R);
	m_TrackBar[2]->SetValueF(DirectValue->G); m_TrackBar[3]->SetValueF(DirectValue->B);
	SetLabelText(m_Value);
	this->DirectValue = DirectValue;
	OnRectChange();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ColorProperty

void ColorProperty::SetLabelText(COLOR v)
{
	string s;
	UintToStr2(&s, v.ARGB, 8, 16);
	m_Label->SetText(s);
}

void ColorProperty::TrackBarValueChangeV()
{
	COLORF c = COLORF(m_TrackBar[0]->GetValueF(), m_TrackBar[1]->GetValueF(), m_TrackBar[2]->GetValueF(), m_TrackBar[3]->GetValueF());
	m_Value = ColorfToColor(c);

	SetLabelText(m_Value);
	ValueChanged(m_Value);
}

void ColorProperty::OnDraw(const VEC2 &Translation)
{
	RECTF r = RECTF(GetRect().Width()-gui::CONTROL_HEIGHT, 0.f, GetRect().Width(), GetRect().Height());
	//r += Translation;

	gfx2d::g_Canvas->SetSprite(NULL);
	gfx2d::g_Canvas->SetColor(m_Value);
	gfx2d::g_Canvas->DrawRect(r);
}

void ColorProperty::ValueSet(const COLOR &x)
{
	m_Value = x;
	
	COLORF c; ColorToColorf(&c, x);

	m_TrackBar[0]->SetValueF(c.R); m_TrackBar[1]->SetValueF(c.R);
	m_TrackBar[2]->SetValueF(c.G); m_TrackBar[3]->SetValueF(c.B);

	SetLabelText(x);
}

ColorProperty::ColorProperty(PropertyGridWindow *Parent, COLOR InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
	ColorBaseProperty<COLOR>(Parent, OnChanged, OnSetValue), m_Value(InitialValue)
{
	CreateControls();
	COLORF c; ColorToColorf(&c, InitialValue);
	m_TrackBar[0]->SetValueF(c.A); m_TrackBar[1]->SetValueF(c.R);
	m_TrackBar[2]->SetValueF(c.G); m_TrackBar[3]->SetValueF(c.B);
	SetLabelText(m_Value);
	OnRectChange();
}

ColorProperty::ColorProperty(PropertyGridWindow *Parent, COLOR *DirectValue) :
	ColorBaseProperty<COLOR>(Parent, DirectValue), m_Value(*DirectValue)
{
	CreateControls();
	COLORF c; ColorToColorf(&c, *DirectValue);
	m_TrackBar[0]->SetValueF(c.A); m_TrackBar[1]->SetValueF(c.R);
	m_TrackBar[2]->SetValueF(c.G); m_TrackBar[3]->SetValueF(c.B);
	SetLabelText(m_Value);
	this->DirectValue = DirectValue;
	OnRectChange();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PropertyGridWindow

PropertyGridWindow::PropertyGridWindow(const RECTF &Rect, const string &Title) :
	gui::Window(Rect, Title)
{
	UpdateRects();
}

void PropertyGridWindow::UpdateRects()
{
	RECTF ClientRect; GetClientRect(&ClientRect);
	ClientRect -= VEC2(ClientRect.left, ClientRect.top);
	ClientRect.Extend(-MARGIN);

	float x1 = ClientRect.left;
	float x2 = ClientRect.GetCenterX() - MARGIN * 0.5f;
	float x3 = x2 + MARGIN;
	float x4 = ClientRect.right;
	float y1 = ClientRect.top;

	RECTF r;

	for (PROPERTY_INFO_LIST::iterator it = m_Properties.begin(); it != m_Properties.end(); ++it)
	{
		r.top = y1;
		r.bottom = y1 + CONTROL_HEIGHT;

		r.left = x1;
		r.right = x2;
		it->Label->SetRect(r);

		if (it->Property != NULL)
		{
			r.left = x3;
			r.right = x4;
			it->Property->SetRect(r);
		}

		y1 += CONTROL_HEIGHT + MARGIN;
	}
}

void PropertyGridWindow::OnDraw(const VEC2 &Translation)
{
	gui::Window::OnDraw(Translation);

	RECTF cr; GetClientRect(&cr);
	float center_x = cr.GetCenterX();

	const float LINE_HALFWIDTH = 2.f / 2.f;

	gfx2d::g_Canvas->SetSprite(NULL);
	gfx2d::g_Canvas->SetColor(gui::g_BkgColor2);

	// Kreska pionowa
	gfx2d::g_Canvas->DrawRect(RECTF(center_x-LINE_HALFWIDTH, cr.top, center_x+LINE_HALFWIDTH, cr.bottom));

	// Kreski poziome
	float y = cr.top + gui::MARGIN * 0.5f;
	for (size_t i = 0; i < m_Properties.size(); i++)
	{
		y += gui::CONTROL_HEIGHT + gui::MARGIN;
		gfx2d::g_Canvas->DrawRect(RECTF(cr.left, y-LINE_HALFWIDTH, cr.right, y+LINE_HALFWIDTH));
	}
}

PropertyGridWindow::PROPERTY_INFO_LIST::iterator PropertyGridWindow::FindProperty(uint4 Id)
{
	for (PROPERTY_INFO_LIST::iterator it = m_Properties.begin(); it != m_Properties.end(); ++it)
	{
		if (it->Id == Id)
			return it;
	}

	return m_Properties.end();
}

void PropertyGridWindow::AddProperty(uint4 Id, const string &Name, PropertyBase *Property, const string &Hint)
{
	m_Properties.push_back(PROPERTY_INFO(this, Id, Name, Property, Hint));

	UpdateRects();
}

void PropertyGridWindow::InsertProperty(uint4 Index, uint4 Id, const string &Name, PropertyBase *Property, const string &Hint)
{
	PROPERTY_INFO_LIST::iterator it = m_Properties.begin();
	while (it != m_Properties.end() && Index > 0)
		++it;

	m_Properties.insert(it, PROPERTY_INFO(this, Id, Name, Property, Hint));

	UpdateRects();
}

void PropertyGridWindow::DeleteProperty(uint4 Id)
{
	PROPERTY_INFO_LIST::iterator it = FindProperty(Id);

	if (it != m_Properties.end())
	{
		m_Properties.erase(it);
		it->Label->Release();
		if (it->Property != NULL)
			it->Property->Release();
	}

	UpdateRects();
}

void PropertyGridWindow::ClearProperties()
{
	for (PROPERTY_INFO_LIST::iterator it = m_Properties.begin(); it != m_Properties.end(); ++it)
	{
		it->Label->Release();
		if (it->Property != NULL)
			it->Property->Release();
	}

	m_Properties.clear();

	UpdateRects();
}

bool PropertyGridWindow::GetPropertyEnabled(uint4 Id)
{
	PROPERTY_INFO_LIST::iterator it = FindProperty(Id);
	if (it != m_Properties.end())
		return it->Property->GetEnabled();
	else
		return false;
}

void PropertyGridWindow::SetPropertyEnabled(uint4 Id, bool Enabled)
{
	PROPERTY_INFO_LIST::iterator it = FindProperty(Id);
	if (it != m_Properties.end())
		it->Property->SetEnabled(Enabled);
}

const string & PropertyGridWindow::GetPropertyText(uint4 Id)
{
	PROPERTY_INFO_LIST::iterator it = FindProperty(Id);
	assert(it != m_Properties.end());
	return it->Label->GetText();
}

void PropertyGridWindow::SetPropertyText(uint4 Id, const string &Text)
{
	PROPERTY_INFO_LIST::iterator it = FindProperty(Id);
	if (it != m_Properties.end())
		it->Label->SetText(Text);
}

gui::PropertyBase * PropertyGridWindow::GetProperty(uint4 Id)
{
	PROPERTY_INFO_LIST::iterator it = FindProperty(Id);
	assert(it != m_Properties.end());
	return it->Property;
}

} // namespace gui

/*

Hierarchia klas
===============

- PropertyBase
  Abstrakcyjna klasa bazowa dla wszystkich w³aœciwoœci. Beztypowa.
  Definiuje zdarzenie OnChanged typu CHANGE_EVENT.
- PropertyBase_t<T>
  Abstrakcyjna klasa bazowa dla wszystkich w³aœciwoœci, których wartoœæ jest typu T.
  Definiuje zdarzenie OnSetValue typu SET_VALUE_EVENT.
  Definiuje tak¿e pole wskaŸnikowe DirectValue.
- Konkretna klasa w³aœciwoœci


Jak u¿ywaæ w³aœciwoœci?
=======================

- Musi ju¿ byæ utworzone PropertyGridWindow.
- W³aœciwoœæ zawsze sama przechowuje wartoœæ danego typu.
- S¹ ró¿ne metody:

1. W³aœciwoœæ pozostawiona sama sobie.
- Utworzyæ w³aœciwoœæ u¿ywaj¹c konstrukrora, do którego podajemy:
  - Wartoœæ pocz¹tkow¹
  - WskaŸniki do zdarzeñ NULL
- U¿ytkownik mo¿e sobie manipulowaæ wartoœci¹.
- Odczytujemy wartoœæ przez GetValue.
- Zmieniamy wartoœæ przez SetValue.

2. Powiadomianie siê o zdarzeniach.
- Utworzyæ w³aœciwoœæ u¿ywaj¹c konstruktora, do którego podajemy:
  - wartoœæ pocz¹tkow¹
  - wskaŸniki do zdarzeñ (jedno lub dwa)
- Poprzez zdarzenia zostajemy powiadomieni o zmianie wartoœci.

3. Bezpoœrednia manipulacja wartoœci¹:
- Utworzyæ w³aœciwoœæ u¿ywaj¹c konstruktora, do którego podajemy:
  - WskaŸnik do wartoœci, na której ona ma bezpoœrednio operowaæ.
- W³aœciwoœæ sama odczytuje i zapisuje wartoœæ pod tym wskaŸnikiem.

Oczywiœcie metody te mo¿na ³¹czyæ, a pola zdarzeñ czy wskaŸnik DirectValue
ustawiaæ, zerowaæ i zmieniaæ tak¿e w czasie pracy.
*/
