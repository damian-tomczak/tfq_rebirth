/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef GUI_PROPERTY_GRID_WINDOW_H_
#define GUI_PROPERTY_GRID_WINDOW_H_

#include <list>
#include "GUI_Controls.hpp"

namespace gui
{

class PropertyGridWindow;

class PropertyBase : public gui::CompositeControl
{
public:
	// Zdarzenie zmiany wartoœci uogólnione - beztypowe
	typedef fastdelegate::FastDelegate1<PropertyBase*> CHANGED_EVENT;
	CHANGED_EVENT OnChanged;

	PropertyBase(PropertyGridWindow *Parent, CHANGED_EVENT OnChanged = NULL);
};

template <typename T>
class PropertyBase_t : public PropertyBase
{
protected:
	// Wywo³ywaæ w reakcji na zmienê wartoœci, a on sam obs³u¿y OnChanged, OnSetValue, DirectValue
	void ValueChanged(const T &x) {
		if (DirectValue) *DirectValue = x;
		if (OnSetValue) OnSetValue(x);
		if (OnChanged) OnChanged(this);
	}

	// Zaimplementowaæ celem uaktualnienia pamiêtanej gdzieœ wartoœci oraz kontrolek GUI.
	// ValueChanged wywo³a siê samo po tym.
	virtual void ValueSet(const T &x) = 0;

public:
	// Zdarzenie zmiany wartoœci typowane
	typedef fastdelegate::FastDelegate1<const T&> SET_VALUE_EVENT;
	SET_VALUE_EVENT OnSetValue;

	// Zmienna która ma byæ bezpoœrednio zmieniana w reakcji na zmianê wartoœci
	T * DirectValue;

	PropertyBase_t(PropertyGridWindow *Parent, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) : PropertyBase(Parent, OnChanged), OnSetValue(OnSetValue), DirectValue(NULL) { }

	// Zmiana wartoœci
	// Wywo³a te¿ zdarzenia.
	void SetValue(const T &x) {
		ValueSet(x);
		ValueChanged(x);
	}

	// Pobranie wartoœci
	virtual T GetValue() = 0;
};

class BoolProperty : public PropertyBase_t<bool>
{
private:
	gui::CheckBox *m_CheckBox;

	void CheckBoxClick(gui::Control *c);

protected:
	// ======== Implementacja Control ========
	virtual void OnRectChange();

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const bool &x);

public:
	BoolProperty(PropertyGridWindow *Parent, bool InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	BoolProperty(PropertyGridWindow *Parent, bool *DirectValue);

	virtual bool GetValue() { return m_CheckBox->StateToBool(m_CheckBox->GetState()); }
};

class StringProperty : public PropertyBase_t<string>
{
private:
	gui::Edit *m_Edit;

	void EditChange(gui::Control *c);

protected:
	// ======== Implementacja Control ========
	virtual void OnRectChange();

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const string &x);

public:
	StringProperty(PropertyGridWindow *Parent, const string &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	StringProperty(PropertyGridWindow *Parent, string *DirectValue);

	virtual string GetValue() { return m_Edit->GetText(); }
};

// Wartoœæ rzeczywista, suwak
class FloatProperty_TrackBar : public PropertyBase_t<float>
{
private:
	gui::TrackBar *m_TrackBar;
	gui::Label *m_Label;

	void TrackBarValueChange(gui::Control *c);

protected:
	void UpdateLabelText();

	// ======== Implementacja Control ========
	virtual void OnRectChange();

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const float &x);

	// ======== Dla klas potomnych ========
	// Opcjonalnie zast¹piæ
	virtual void FormatLabelText(string *Out);

public:
	FloatProperty_TrackBar(PropertyGridWindow *Parent, float Min, float Max, float Step, float InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	FloatProperty_TrackBar(PropertyGridWindow *Parent, float Min, float Max, float Step, float *DirectValue);

	virtual float GetValue() { return m_TrackBar->GetValueF(); }

	void SetRange(float Min, float Max, float Step);
};

// W³aœciwoœæ typu float wyra¿ona w procentach, zakres to 0..1. Suwak.
class PercentProperty : public FloatProperty_TrackBar
{
protected:
	// ======== Implementacja FloatProperty ========
	virtual void FormatLabelText(string *Out);

public:
	PercentProperty(PropertyGridWindow *Parent, float InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	PercentProperty(PropertyGridWindow *Parent, float *DirectValue);
};

class IntProperty_TrackBar : public PropertyBase_t<int>
{
private:
	gui::TrackBar *m_TrackBar;
	gui::Label *m_Label;

	void TrackBarValueChange(gui::Control *c);

protected:
	void UpdateLabelText();

	// ======== Implementacja Control ========
	virtual void OnRectChange();

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const int &x);

public:
	IntProperty_TrackBar(PropertyGridWindow *Parent, int Min, int Max, int Step, int InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	IntProperty_TrackBar(PropertyGridWindow *Parent, int Min, int Max, int Step, int *DirectValue);

	virtual int GetValue() { return m_TrackBar->GetValueI(); }

	void SetRange(int Min, int Max, int Step);
};

// Para liczb rzeczywistych - dwa suwaki i Label
class FloatPairProperty : public PropertyBase_t<VEC2>
{
private:
	gui::TrackBar *m_TrackBars[2];
	gui::Label *m_Label;

	void TrackBarValueChange(gui::Control *c);
	void FormatLabelText(string *Out);

protected:
	void UpdateLabelText();

	// ======== Implementacja Control ========
	virtual void OnRectChange();

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const VEC2 &x);

public:
	FloatPairProperty(PropertyGridWindow *Parent, const VEC2 &Min, const VEC2 &Max, const VEC2 &Step, const VEC2 &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	FloatPairProperty(PropertyGridWindow *Parent, const VEC2 &Min, const VEC2 &Max, const VEC2 &Step, VEC2 *DirectValue);

	virtual VEC2 GetValue() { return VEC2(m_TrackBars[0]->GetValueF(), m_TrackBars[1]->GetValueF()); }

	void SetRange(const VEC2 &Min, const VEC2 &Max, const VEC2 &Step);
};

// Wartoœæ dowolnego typu, który ma konwersje SthToStr<T> i StrToSth<T> - jako pole edycyjne.
template <typename T>
class SthProperty_Edit : public PropertyBase_t<T>
{
private:
	gui::Edit *m_Edit;
	T m_Value;

	void EditChange(gui::Control *c) {
		T v;
		if (StrToSth<T>(&v, m_Edit->GetText())) {
			m_Value = v;
			ValueChanged(m_Value);
		}
	}

protected:
	// ======== Implementacja Control ========
	virtual void OnRectChange() {
		RECTF ClientRect; GetClientRect(&ClientRect);
		ClientRect -= VEC2(ClientRect.left, ClientRect.top);
		m_Edit->SetRect(ClientRect);
	}

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const T &x) {
		m_Value = x;
		string s; SthToStr<T>(&s, x);
		m_Edit->SetText(s);
	}

public:
	SthProperty_Edit(PropertyGridWindow *Parent, const T &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) :
		PropertyBase_t<T>(Parent, OnChanged, OnSetValue), m_Value(InitialValue)
	{
		m_Edit = new gui::Edit(this, RECTF::ZERO);
		string s; SthToStr<T>(&s, InitialValue);
		m_Edit->SetText(s);
		m_Edit->OnChange.bind(this, &SthProperty_Edit<T>::EditChange);

		OnRectChange();
	}
	SthProperty_Edit(PropertyGridWindow *Parent, T *DirectValue) :
		PropertyBase_t<T>(Parent, NULL, NULL), m_Value(*DirectValue)
	{
		m_Edit = new gui::Edit(this, RECTF::ZERO);
		string s; SthToStr<T>(&s, *DirectValue);
		m_Edit->SetText(s);
		m_Edit->OnChange.bind(this, &SthProperty_Edit<T>::EditChange);

		this->DirectValue = DirectValue;

		OnRectChange();
	}

	virtual T GetValue() { return m_Value; }
};

class EnumProperty : public PropertyBase_t<uint4>
{
public:
	typedef std::vector< std::pair<string, uint4> > VALUE_VECTOR;

private:
	VALUE_VECTOR m_ValueVector;
	scoped_ptr<gui::StringList, ReleasePolicy> m_List;
	gui::ListComboBox *m_ComboBox;

	uint4 ValueToSelIndex(uint4 Value);
	uint4 SelIndexToValue(uint4 Index) { return m_ValueVector[Index].second; }

	void ListSelChange(gui::Control *c);

protected:
	// ======== Implementacja Control ========
	virtual void OnRectChange();

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const uint4 &x);

public:
	EnumProperty(PropertyGridWindow *Parent, const VALUE_VECTOR &ValueVector, uint4 InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	EnumProperty(PropertyGridWindow *Parent, const VALUE_VECTOR &ValueVector, uint4 *DirectValue);

	virtual uint4 GetValue() { return SelIndexToValue(m_List->GetModel().GuiList_GetSelIndex()); }
};

class Vec3PosProperty : public PropertyBase_t<VEC3>
{
private:
	VEC3 m_Value;
	gui::Edit *m_Edit[3];

	void SetEditText(const VEC3 &v);

	void EditChange(gui::Control *c);

protected:
	// ======== Implementacja Control ========
	virtual void OnRectChange();

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const VEC3 &x);

public:
	Vec3PosProperty(PropertyGridWindow *Parent, const VEC3 &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	Vec3PosProperty(PropertyGridWindow *Parent, VEC3 *DirectValue);

	virtual VEC3 GetValue() { return m_Value; }
};

class Vec3DirProperty : public PropertyBase_t<VEC3>
{
private:
	VEC3 m_Value;
	gui::TrackBar *m_TrackBar[2];
	gui::Label *m_Label;

	void SetLabelText(const VEC3 &v);
	void AnglesToDir(VEC3 *Out, float Yaw, float Pitch);
	void DirToAngles(float *OutYaw, float *OutPitch, const VEC3 &Dir);

	void TrackBarValueChange(gui::Control *c);

protected:
	// ======== Implementacja Control ========
	virtual void OnRectChange();

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const VEC3 &x);

public:
	Vec3DirProperty(PropertyGridWindow *Parent, const VEC3 &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	Vec3DirProperty(PropertyGridWindow *Parent, VEC3 *DirectValue);

	virtual VEC3 GetValue() { return m_Value; }
};

template <typename T>
class ColorBaseProperty : public PropertyBase_t<T>
{
private:
	void TrackBarValueChange(gui::Control *c) { TrackBarValueChangeV(); }

protected:
	gui::TrackBar *m_TrackBar[4];
	gui::Label *m_Label;

	void CreateControls() {
		for (size_t i = 0; i < 4; i++) {
			m_TrackBar[i] = new gui::TrackBar(this, RECTF::ZERO, O_HORIZ, true, true);
			m_TrackBar[i]->SetRangeF(0.f, 1.f, 1.f/256.f);
			m_TrackBar[i]->OnValueChange.bind(this, &ColorBaseProperty<T>::TrackBarValueChange);
		}
		m_Label = new gui::Label(this, RECTF::ZERO, string(), res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE);
	}

	// ======== Implementacja Control ========
	virtual void OnRectChange() {
		RECTF ClientRect; GetClientRect(&ClientRect);
		const float DIV = 0.6f;
		float y1 = 0.f; float y2 = ClientRect.Height();
		float x1 = 0.f;
		float x10 = ClientRect.Width() - gui::CONTROL_HEIGHT - gui::MARGIN;
		float x9 = ClientRect.Width() * DIV + gui::MARGIN*0.5f;
		float x8 = x9 - gui::MARGIN;
		float x4 = x8*0.5f - gui::MARGIN*0.5f;
		float x5 = x4 + gui::MARGIN;
		float x2 = x4*0.5f - gui::MARGIN*0.5f;
		float x3 = x2 + gui::MARGIN;
		float x6 = x5 + (x8-x5)*0.5f - gui::MARGIN*0.5f;
		float x7 = x6 + gui::MARGIN;

		m_TrackBar[0]->SetRect(RECTF(x1, y1, x2, y2));
		m_TrackBar[1]->SetRect(RECTF(x3, y1, x4, y2));
		m_TrackBar[2]->SetRect(RECTF(x5, y1, x6, y2));
		m_TrackBar[3]->SetRect(RECTF(x7, y1, x8, y2));
		m_Label->SetRect(RECTF(x9, y1, x10, y2));
	}

	// ======== Dla klas potomnych ========
	virtual void TrackBarValueChangeV() = 0;

public:
	ColorBaseProperty(PropertyGridWindow *Parent, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue) : PropertyBase_t<T>(Parent, OnChanged, OnSetValue) { }
	ColorBaseProperty(PropertyGridWindow *Parent, T *DirectValue) : PropertyBase_t<T>(Parent, NULL, NULL) { }
};

class ColorfProperty : public ColorBaseProperty<COLORF>
{
private:
	COLORF m_Value;

	void SetLabelText(const COLORF &v);
	
protected:
	// ======== Implementacja Control ========
	virtual void OnDraw(const VEC2 &Translation);

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const COLORF &x);

	// ======== Implementacja ColorBaseProperty<> ========
	virtual void TrackBarValueChangeV();

public:
	ColorfProperty(PropertyGridWindow *Parent, const COLORF &InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	ColorfProperty(PropertyGridWindow *Parent, COLORF *DirectValue);

	virtual COLORF GetValue() { return m_Value; }
};

class ColorProperty : public ColorBaseProperty<COLOR>
{
private:
	COLOR m_Value;

	void SetLabelText(COLOR v);
	
protected:
	// ======== Implementacja Control ========
	virtual void OnDraw(const VEC2 &Translation);

	// ======== Implementacja PropertyBase_t ========
	virtual void ValueSet(const COLOR &x);

	// ======== Implementacja ColorBaseProperty<> ========
	virtual void TrackBarValueChangeV();

public:
	ColorProperty(PropertyGridWindow *Parent, COLOR InitialValue, CHANGED_EVENT OnChanged, SET_VALUE_EVENT OnSetValue);
	ColorProperty(PropertyGridWindow *Parent, COLOR *DirectValue);

	virtual COLOR GetValue() { return m_Value; }
};

/*
- Jeœli chodzi o dodawanie, usuwanie, przestawianie w³aœciwoœci, to demonem szybkoœci nie jest :)
*/

class PropertyGridWindow : public gui::Window
{
private:
	struct PROPERTY_INFO
	{
		uint4 Id;
		// To nie s¹ inteligentne wskaŸniki, bo kontrolki potomne same siê zwalniaj¹.
		gui::Label *Label;
		PropertyBase *Property;
		shared_ptr<gui::StringHint> Hint;

		PROPERTY_INFO(gui::CompositeControl *Parent, uint4 Id, const string &Name, PropertyBase *Property, const string &Hint = "") :
			Id(Id),
			Label(new gui::Label(Parent, RECTF::ZERO, Name, res::Font::FLAG_HLEFT | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE)),
			Property(Property),
			Hint(new gui::StringHint(Hint))
		{
			Label->SetHint(this->Hint.get());
		}
	};

	typedef std::list<PROPERTY_INFO> PROPERTY_INFO_LIST;

	PROPERTY_INFO_LIST m_Properties;

	void UpdateRects();
	PROPERTY_INFO_LIST::iterator FindProperty(uint4 Id);

protected:
	// ======== Implementacja Control ========
	virtual void OnRectChange() { UpdateRects(); }
	virtual void OnDraw(const VEC2 &Translation);

public:
	PropertyGridWindow(const RECTF &Rect, const string &Title);

	// Property - mo¿na podaæ NULL, wtedy jest sam napis, taki jakby separator.
	void AddProperty(uint4 Id, const string &Name, PropertyBase *Property, const string &Hint = "");
	void InsertProperty(uint4 Index, uint4 Id, const string &Name, PropertyBase *Property, const string &Hint = "");
	void DeleteProperty(uint4 Id);
	void ClearProperties();
	bool GetPropertyEnabled(uint4 Id);
	void SetPropertyEnabled(uint4 Id, bool Enabled);
	const string & GetPropertyText(uint4 Id);
	void SetPropertyText(uint4 Id, const string &Text);
	gui::PropertyBase * GetProperty(uint4 Id);
	template <typename T> gui::PropertyBase_t<T> * GetPropertyEx(uint4 Id) { return static_cast< PropertyBase_t<T>* >(GetProperty(Id)); }
	template <typename T> T GetPropertyValue(uint4 Id) { return static_cast< PropertyBase_t<T>* >(GetProperty(Id))->GetValue(); }
};

} // namespace gui

#endif
