/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef ENTITIES_H_
#define ENTITIES_H_

#include "Engine.hpp"
#include "..\Framework\QMesh.hpp"

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Uterm (Universal Term)

// Universal Term - funkcja: v = A2*t^2 + A1*t + A0 + B2*sin(B1*t+B0)
// Wersja dla pojedynczej warto�ci typu float.
class Uterm
{
public:
	// Tworzy nowy, pusty (zainicjalizowany zerami)
	Uterm() : A2(0.0f), A1(0.0f), A0(0.0f), B2(0.0f), B1(0.0f), B0(0.0f), Type(0) { }
	// Tworzy nowy zainicjalizowany podanymi warto�ciami
	Uterm(float A2, float A1, float A0, float B2, float B1, float B0) :
		A2(A2), A1(A1), A0(A0), B2(B2), B1(B1), B0(B0)
	{
		Type = 0;
		if (A2 != 0.0f) Type |= 0x01;
		if (A1 != 0.0f) Type |= 0x02;
		if (B2 != 0.0f) Type |= 0x08;
	}

	float GetA2() { return A2; }
	float GetA1() { return A1; }
	float GetA0() { return A0; }
	float GetB2() { return B2; }
	float GetB1() { return B1; }
	float GetB0() { return B0; }

	void SetA2(float A2) { this->A2 = A2; if (A2 == 0.0f) Type &= ~0x01; else Type |= 0x01; }
	void SetA1(float A1) { this->A1 = A1; if (A1 == 0.0f) Type &= ~0x02; else Type |= 0x02; }
	void SetA0(float A0) { this->A0 = A0; }
	void SetB2(float B2) { this->B2 = B2; if (B2 == 0.0f) Type &= ~0x08; else Type |= 0x08; }
	void SetB1(float B1) { this->B1 = B1; }
	void SetB0(float B0) { this->B0 = B0; }
	void SetA(float A2, float A1, float A0) { SetA2(A2); SetA1(A1); SetA0(A0); }
	void SetB(float B2, float B1, float B0) { SetB2(B2); SetB1(B1); SetB0(B0); }
	void Set(float A2, float A1, float A0, float B2, float B1, float B0) { SetA(A2, A1, A0); SetB(B2, B1, B0); }
	void ResetA() { A2 = A1 = A0 = 0.0f; Type &= ~0x07; }
	void ResetB() { B2 = B1 = B0 = 0.0f; Type &= ~0x08; }
	void Reset() { A2 = A1 = A0 = B2 = B1 = B0 = 0.0f; Type = 0; }

	float Eval(float t)
	{
		float v = A0;
		if ((Type & 0x02) != 0) v += A1 * t;
		if ((Type & 0x01) != 0) v += A2 * t * t;
		if ((Type & 0x08) != 0) v += B2 * sinf(B1 * t + B0);
		return v;
	}

private:
	float A2, A1, A0, B2, B1, B0;
	uint Type;
};

// Universal Term - funkcja: v = A2*t^2 + A1*t + A0 + B2*sin(B1*t+B0)
// Wersja dla wektora 2D.
class Uterm2
{
public:
	// Tworzy nowy, pusty (zainicjalizowany zerami)
	Uterm2() : A2(VEC2::ZERO), A1(VEC2::ZERO), A0(VEC2::ZERO), B2(VEC2::ZERO), B1(VEC2::ZERO), B0(VEC2::ZERO), Type(0) { }
	// Tworzy nowy zainicjalizowany podanymi warto�ciami
	Uterm2(const VEC2 &A2, const VEC2 &A1, const VEC2 &A0, const VEC2 &B2, const VEC2 &B1, const VEC2 &B0) :
		A2(A2), A1(A1), A0(A0), B2(B2), B1(B1), B0(B0)
	{
		Type = 0;
		if (A2 != VEC2::ZERO) Type |= 0x01;
		if (A1 != VEC2::ZERO) Type |= 0x02;
		if (B2 != VEC2::ZERO) Type |= 0x08;
	}

	const VEC2 & GetA2() { return A2; }
	const VEC2 & GetA1() { return A1; }
	const VEC2 & GetA0() { return A0; }
	const VEC2 & GetB2() { return B2; }
	const VEC2 & GetB1() { return B1; }
	const VEC2 & GetB0() { return B0; }

	void SetA2(const VEC2 &A2) { this->A2 = A2; if (A2 == VEC2::ZERO) Type &= ~0x01; else Type |= 0x01; }
	void SetA1(const VEC2 &A1) { this->A1 = A1; if (A1 == VEC2::ZERO) Type &= ~0x02; else Type |= 0x02; }
	void SetA0(const VEC2 &A0) { this->A0 = A0; }
	void SetB2(const VEC2 &B2) { this->B2 = B2; if (B2 == VEC2::ZERO) Type &= ~0x08; else Type |= 0x08; }
	void SetB1(const VEC2 &B1) { this->B1 = B1; }
	void SetB0(const VEC2 &B0) { this->B0 = B0; }
	void SetA(const VEC2 &A2, const VEC2 &A1, const VEC2 &A0) { SetA2(A2); SetA1(A1); SetA0(A0); }
	void SetB(const VEC2 &B2, const VEC2 &B1, const VEC2 &B0) { SetB2(B2); SetB1(B1); SetB0(B0); }
	void Set(const VEC2 &A2, const VEC2 &A1, const VEC2 &A0, const VEC2 &B2, const VEC2 &B1, const VEC2 &B0) { SetA(A2, A1, A0); SetB(B2, B1, B0); }
	void ResetA() { A2 = A1 = A0 = VEC2::ZERO; Type &= ~0x07; }
	void ResetB() { B2 = B1 = B0 = VEC2::ZERO; Type &= ~0x08; }
	void Reset() { A2 = A1 = A0 = B2 = B1 = B0 = VEC2::ZERO; Type = 0; }

	void Eval(VEC2 *Out, float t)
	{
		*Out = A0;
		if ((Type & 0x02) != 0) *Out += A1 * t;
		if ((Type & 0x01) != 0) *Out += A2 * t * t;
		if ((Type & 0x08) != 0)
		{
			Out->x += B2.x * sinf(B1.x * t + B0.x);
			Out->y += B2.y * sinf(B1.y * t + B0.y);
		}
	}

private:
	VEC2 A2, A1, A0, B2, B1, B0;
	uint Type;
};

// Universal Term - funkcja: v = A2*t^2 + A1*t + A0 + B2*sin(B1*t+B0)
// Wersja dla wektora 3D.
class Uterm3
{
public:
	// Tworzy nowy, pusty (zainicjalizowany zerami)
	Uterm3() : A2(VEC3::ZERO), A1(VEC3::ZERO), A0(VEC3::ZERO), B2(VEC3::ZERO), B1(VEC3::ZERO), B0(VEC3::ZERO), Type(0) { }
	// Tworzy nowy zainicjalizowany podanymi warto�ciami
	Uterm3(const VEC3 &A2, const VEC3 &A1, const VEC3 &A0, const VEC3 &B2, const VEC3 &B1, const VEC3 &B0) :
		A2(A2), A1(A1), A0(A0), B2(B2), B1(B1), B0(B0)
	{
		Type = 0;
		if (A2 != VEC3::ZERO) Type |= 0x01;
		if (A1 != VEC3::ZERO) Type |= 0x02;
		if (B2 != VEC3::ZERO) Type |= 0x08;
	}

	const VEC3 & GetA2() { return A2; }
	const VEC3 & GetA1() { return A1; }
	const VEC3 & GetA0() { return A0; }
	const VEC3 & GetB2() { return B2; }
	const VEC3 & GetB1() { return B1; }
	const VEC3 & GetB0() { return B0; }

	void SetA2(const VEC3 &A2) { this->A2 = A2; if (A2 == VEC3::ZERO) Type &= ~0x01; else Type |= 0x01; }
	void SetA1(const VEC3 &A1) { this->A1 = A1; if (A1 == VEC3::ZERO) Type &= ~0x02; else Type |= 0x02; }
	void SetA0(const VEC3 &A0) { this->A0 = A0; }
	void SetB2(const VEC3 &B2) { this->B2 = B2; if (B2 == VEC3::ZERO) Type &= ~0x08; else Type |= 0x08; }
	void SetB1(const VEC3 &B1) { this->B1 = B1; }
	void SetB0(const VEC3 &B0) { this->B0 = B0; }
	void SetA(const VEC3 &A2, const VEC3 &A1, const VEC3 &A0) { SetA2(A2); SetA1(A1); SetA0(A0); }
	void SetB(const VEC3 &B2, const VEC3 &B1, const VEC3 &B0) { SetB2(B2); SetB1(B1); SetB0(B0); }
	void Set(const VEC3 &A2, const VEC3 &A1, const VEC3 &A0, const VEC3 &B2, const VEC3 &B1, const VEC3 &B0) { SetA(A2, A1, A0); SetB(B2, B1, B0); }
	void ResetA() { A2 = A1 = A0 = VEC3::ZERO; Type &= ~0x07; }
	void ResetB() { B2 = B1 = B0 = VEC3::ZERO; Type &= ~0x08; }
	void Reset() { A2 = A1 = A0 = B2 = B1 = B0 = VEC3::ZERO; Type = 0; }

	void Eval(VEC3 *Out, float t)
	{
		*Out = A0;
		if ((Type & 0x02) != 0) *Out += A1 * t;
		if ((Type & 0x01) != 0) *Out += A2 * t * t;
		if ((Type & 0x08) != 0)
		{
			Out->x += B2.x * sinf(B1.x * t + B0.x);
			Out->y += B2.y * sinf(B1.y * t + B0.y);
			Out->z += B2.z * sinf(B1.z * t + B0.z);
		}
	}

private:
	VEC3 A2, A1, A0, B2, B1, B0;
	uint Type;
};

// Universal Term - funkcja: v = A2*t^2 + A1*t + A0 + B2*sin(B1*t+B0)
// Wersja dla wektora 4D.
class Uterm4
{
public:
	// Tworzy nowy, pusty (zainicjalizowany zerami)
	Uterm4() : A2(VEC4::ZERO), A1(VEC4::ZERO), A0(VEC4::ZERO), B2(VEC4::ZERO), B1(VEC4::ZERO), B0(VEC4::ZERO), Type(0) { }
	// Tworzy nowy zainicjalizowany podanymi warto�ciami
	Uterm4(const VEC4 &A2, const VEC4 &A1, const VEC4 &A0, const VEC4 &B2, const VEC4 &B1, const VEC4 &B0) :
		A2(A2), A1(A1), A0(A0), B2(B2), B1(B1), B0(B0)
	{
		Type = 0;
		if (A2 != VEC4::ZERO) Type |= 0x01;
		if (A1 != VEC4::ZERO) Type |= 0x02;
		if (B2 != VEC4::ZERO) Type |= 0x08;
	}

	const VEC4 & GetA2() { return A2; }
	const VEC4 & GetA1() { return A1; }
	const VEC4 & GetA0() { return A0; }
	const VEC4 & GetB2() { return B2; }
	const VEC4 & GetB1() { return B1; }
	const VEC4 & GetB0() { return B0; }

	void SetA2(const VEC4 &A2) { this->A2 = A2; if (A2 == VEC4::ZERO) Type &= ~0x01; else Type |= 0x01; }
	void SetA1(const VEC4 &A1) { this->A1 = A1; if (A1 == VEC4::ZERO) Type &= ~0x02; else Type |= 0x02; }
	void SetA0(const VEC4 &A0) { this->A0 = A0; }
	void SetB2(const VEC4 &B2) { this->B2 = B2; if (B2 == VEC4::ZERO) Type &= ~0x08; else Type |= 0x08; }
	void SetB1(const VEC4 &B1) { this->B1 = B1; }
	void SetB0(const VEC4 &B0) { this->B0 = B0; }
	void SetA(const VEC4 &A2, const VEC4 &A1, const VEC4 &A0) { SetA2(A2); SetA1(A1); SetA0(A0); }
	void SetB(const VEC4 &B2, const VEC4 &B1, const VEC4 &B0) { SetB2(B2); SetB1(B1); SetB0(B0); }
	void Set(const VEC4 &A2, const VEC4 &A1, const VEC4 &A0, const VEC4 &B2, const VEC4 &B1, const VEC4 &B0) { SetA(A2, A1, A0); SetB(B2, B1, B0); }
	void ResetA() { A2 = A1 = A0 = VEC4::ZERO; Type &= ~0x07; }
	void ResetB() { B2 = B1 = B0 = VEC4::ZERO; Type &= ~0x08; }
	void Reset() { A2 = A1 = A0 = B2 = B1 = B0 = VEC4::ZERO; Type = 0; }

	void Eval(VEC4 *Out, float t)
	{
		*Out = A0;
		if ((Type & 0x02) != 0) *Out += A1 * t;
		if ((Type & 0x01) != 0) *Out += A2 * t * t;
		if ((Type & 0x08) != 0)
		{
			Out->x += B2.x * sinf(B1.x * t + B0.x);
			Out->y += B2.y * sinf(B1.y * t + B0.y);
			Out->z += B2.z * sinf(B1.z * t + B0.z);
			Out->w += B2.w * sinf(B1.w * t + B0.w);
		}
	}

private:
	VEC4 A2, A1, A0, B2, B1, B0;
	uint Type;
};


namespace engine
{

// Prezentuje model z zasobu typu res::QMesh (z pliku QMSH)
class QMeshEntity : public MaterialEntity
{
public:
	enum ANIM_MODE
	{
		// Odtwarzanie normalne - do przodu
		ANIM_MODE_NORMAL    = 0x00,
		// Odtwarzanie od ko�ca
		ANIM_MODE_BACKWARDS = 0x01,
		// Odtwarzanie do ko�ca a potem z powrotem
		ANIM_MODE_BIDI      = 0x02,

		// Odtwarzanie jeden raz
		ANIM_MODE_ONCE      = 0x00,
		// Odtwarzanie powtarzane w k�ko
		ANIM_MODE_LOOP      = 0x10,
	};

	QMeshEntity(Scene *OwnerScene);
	QMeshEntity(Scene *OwnerScene, const string &MeshResName);

	const string & GetMeshResName() { return m_MeshResName; }
	void SetMeshResName(const string &MeshResName);

	// Sterowanie animacj�
	// Mode - flagi ANIM_MODE
	// Nazwa nieprawid�owa (np. pusta) - brak animacji

	// Ustawia natychmiast bie��c� animacj�
	void SetAnimation(const string &AnimationName, uint Mode = 0, float StartTime = 0.f, float Speed = 1.f);
	// Ustawia now� animacj�, kt�ra zacznie si� natychmiast, ale zostanie zmieszana z bie��c�
	void BlendAnimation(const string &AnimationName, float BlendTime, uint Mode = 0, float StartTime = 0.f, float Speed = 1.f);
	// Ustawia aniamacj� do odegrania kiedy bie��ca si� sko�czy
	void QueueAnimation(const string &AnimationName, uint Mode = 0, float StartTime = 0.f, float Speed = 1.f);
	// Ustawia aniamacj� do odegrania kiedy bie��ca si� sko�czy i zmieszania z ni� p�ynnie
	void QueueBlendAnimation(const string &AnimationName, float BlendTime, uint Mode = 0, float StartTime = 0.f, float Speed = 1.f);
	// Czy�ci stan animacji tak �e nie b�dzie �adnej
	void ResetAnimation();
	// Ustawia czas bie��cej animacji
	void SetAnimationTime(float Time);
	// Ustawia pr�dko�� bie��cej animacji
	void SetAnimationSpeed(float Speed);
	// Ustawia tryb bie��cej animacji
	void SetAnimationMode(uint Mode);

	// W�asne materia�y dla fragment�w, ustawiane per encja, nadpisuj�ce te z zasobu siatki
	void SetCustomMaterial(uint SubmeshIndex, const string &MaterialName);
	void SetAllCustomMaterials(const string &MaterialName);
	void ResetCustomMaterial(uint SubmeshIndex);
	void ResetAllCustomMaterials();

	// ======== Implementacja Entity ========
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT);
	virtual const MATRIX & GetBoneMatrix(const string &BoneName);
	virtual void Update();

	// ======== Implementacja MaterialEntity ========
	virtual uint GetFragmentCount();
	virtual void GetFragmentData(uint Index, uint *OutId, BaseMaterial **OutMaterial);
	virtual void DrawFragmentGeometry(uint Id, BaseMaterial *Mat, const ParamsCamera &Cam);
	virtual bool GetSkinningData(uint *OutBoneCount, const MATRIX **OutBoneMatrices);

private:
	string m_MeshResName;
	res::QMesh *m_MeshRes;

	// Pole wype�niane przez EnsureMeshAndMaterials
	// Lista m_Material pusta wskazuje na to �e to pole nie jest wype�nione.
	std::vector<BaseMaterial*> m_Materials;
	// W�asne materia�y nadpisuj�ce te pobrane z zasobu siatki
	// D�ugo�� jest nieokre�lona, mo�e by� 0, mniejsza lub wi�ksza ni� liczba materia��w w siatce.
	// �a�cuch pusty oznacza brak nadpisania materia�u dla podsiatki o danym indeksie.
	STRING_VECTOR m_CustomMaterialNames;

	// Animacja poprzednia - blending z bie��c� (MAXUINT4 je�li nie ma)
	uint m_PrevAnim;
	float m_PrevAnimTime;
	float m_PrevAnimSpeed;
	uint m_PrevAnimMode;
	// Animacja bie��ca (MAXUINT4 je�li nie ma)
	uint m_CurrAnim;
	float m_CurrAnimTime;
	float m_CurrAnimSpeed;
	uint m_CurrAnimMode;
	// Animacja nast�pna do odegrania kiedy bie��ca si� sko�czy (NULL je�li nie ma)
	uint m_NextAnim; // Je�li != MAXUINT4, m_CurrAnim te� zawsze != MAXUINT4
	float m_NextAnimTime;
	float m_NextAnimSpeed;
	uint m_NextAnimMode;
	bool m_NextAnimBlend;
	float m_NextAnimBlendTime;
	// Parametry do blendingu mi�dzy PrevAnim a CurrAnim (dzia�aj� kiedy m_PrevAnim != NULL)
	float m_BlendTime;
	float m_BlendEndTime;

	// Przypisuje do m_MeshRes oraz zwraca zas�b siatki lub NULL je�li brak.
	res::QMesh * GetMeshRes();
	// Wylicza je�li nie ma wyliczonych macierze ModelToBoneMat
	void EnsureModelToBoneMat();
	// Zapewnia, �e m_MeshRes jest pobrany i LOADED
	// Je�li nie mo�na, to zwraca false.
	bool EnsureMeshRes();
	// Zapewnia �e MeshRes zostaje LOAD, a m_Materials wype�nione.
	// Je�li nie ma aktualnie QMesh powi�zanej z t� encj�, zwraca false.
	bool EnsureMeshAndMaterials();
	float ModeToLengthFactor(uint Mode) { return ((Mode & ANIM_MODE_BIDI) != 0) ? 2.f : 1.f; }
	float ProcessAnimTime(float AnimTime, uint Mode, float AnimLength);
};

// Prosty model prostopad�o�cianu, dla test�w
// Ma wsp�rz�dne tekstury, normalne i tangenty.
// Rozmiary: -1,-1,-1;1,1,1
// Je�li brak ustawionego materia�u, nie jest rysowany.
class BoxEntity : public MaterialEntity
{
public:
	BoxEntity(Scene *OwnerScene);
	BoxEntity(Scene *OwnerScene, const string &MaterialName);

	const string & GetMaterialName() { return m_MaterialName; }
	void SetMaterialName(const string &MaterialName) { m_MaterialName = MaterialName; m_Material = NULL; }

	// ======== Implementacja Entity ========
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT);

	// ======== Implementacja MaterialEntity ========
	virtual uint GetFragmentCount();
	virtual void GetFragmentData(uint Index, uint *OutId, BaseMaterial **OutMaterial);
	virtual void DrawFragmentGeometry(uint Id, BaseMaterial *Mat, const ParamsCamera &Cam);

private:
	string m_MaterialName;
	// NULL je�li nie pobrany. Pobierany przy pierwszym u�yciu.
	BaseMaterial *m_Material;

	// Zapewnia, �e b�dzie wype�nione m_Material.
	// Je�li nie ma ustawionego materia�u i nie mo�na by�o go wype�ni�, zwraca false.
	bool EnsureMaterial();
};

/*
# Reprezentuje encj� wy�wietlan� jako prostok�t.
# Jest z�o�ony z dw�ch tr�jk�t�w rysowancyh za pomoc� DrawIndexedPrimitiveUP.
# Mo�e te� s�u�y� jako Billboard.
# Obraca si� zawsze wok� �rodka.
  Mo�e kiedy� dorobi� opcj�, �eby obraca�a si� wok� podstawy na dole.

Opis parametr�w:
# DegreesOfFreedom (domy�lnie: 0)
  0 = To b�dzie zwyk�y quad, rysowany zawsze w tej samej pozycji.
  1 = To b�dzie billboard obracaj�cy si� w stron� kamery tylko wok� osi Up.
  2 = To b�dzie billboard obracaj�cy si� w strone kamery w pe�ni, wok� dw�ch osi.
# UseRealDir (domy�lnie: false)
  false = Jako kierunek brany jest kierunek patrzenia kamery.
    Wszystkie billboardy s� zwr�cone tak samo. Dzia�a szybciej.
  true = Jako kierunek brany jest prawdziwy kierunek od kamery do tej encji.
    Billboardy s� zwr�cone w stron� kamery, otaczaj� j�. Dzia�a nieco wolniej.
# RightDir, UpDir (domy�lnie: [1,0,0], [0,1,0])
  Kierunki uznawane za:
  > kierunek w prawo (liczy si� tylko kiedy DegreesOfFreedom < 1)
  > kierunek do g�ry (liczy si� tylko kiedy DegreesOfFreedom < 2)
  Trzeba koniecznie podawa� znormalizowane.
# Tex (domy�lnie: (0,0,1,1))
  Wsp�rz�dne tekstury u�ywane do teksturowania quada.
  Left i Top to lewy g�rny r�g quada.
# HalfSize (domy�lnie: (1,1))
  Wymiary quada - po�owa szeroko�ci i po�owa wysoko�ci.
*/
class QuadEntity : public MaterialEntity
{
public:
	QuadEntity(Scene *OwnerScene);
	QuadEntity(Scene *OwnerScene, const string &MaterialName);

	const string & GetMaterialName() { return m_MaterialName; }
	uint GetDegreesOfFreedom() { return m_DegreesOfFreedom; }
	bool GetUseRealDir() { return m_UseRealDir; }
	const VEC3 & GetRightDir() { return m_RightDir; }
	const VEC3 & GetUpDir() { return m_UpDir; }
	const RECTF & GetTex() { return m_Tex; }
	const VEC2 & GetHalfSize() { return m_HalfSize; }

	void SetMaterialName(const string &MaterialName) { m_MaterialName = MaterialName; m_Material = NULL; }
	void SetDegreesOfFreedom(uint DegreesOfFreedom);
	void SetUseRealDir(bool UseRealDir);
	void SetRightDir(const VEC3 &RightDir);
	void SetUpDir(const VEC3 &UpDir);
	void SetTex(const RECTF &Tex);
	void SetHalfSize(const VEC2 &HalfSize);

	// ======== Implementacja Entity ========
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT);

	// ======== Implementacja MaterialEntity ========
	virtual uint GetFragmentCount();
	virtual void GetFragmentData(uint Index, uint *OutId, BaseMaterial **OutMaterial);
	virtual void DrawFragmentGeometry(uint Id, BaseMaterial *Mat, const ParamsCamera &Cam);

private:
	// ============ Dane g��wne ========
	string m_MaterialName;
	// 0, 1, 2
	uint m_DegreesOfFreedom;
	bool m_UseRealDir;
	// Znormalizowany
	VEC3 m_RightDir;
	// Znormalizowany
	VEC3 m_UpDir;
	RECTF m_Tex;
	VEC2 m_HalfSize;

	// ======== Dane generowane ========
	// NULL je�li nie pobrany. Pobierany przy pierwszym u�yciu.
	BaseMaterial *m_Material;
	// Pos, Normal, Tangent, Binormal: wyliczone na sta�e kiedy DegreesOfFreedom == 0, inaczej wyliczane w ka�dej klatce.
	// Wsp�rz�dne tekstury zawsze wyliczone na sta�e.
	VERTEX_XN233 m_VB[4];

	// Zapewnia, �e b�dzie wype�nione m_Material.
	// Je�li nie ma ustawionego materia�u i nie mo�na by�o go wype�ni�, zwraca false.
	bool EnsureMaterial();
	void UpdateRadius();
	void UpdateVB();
	void FillVbForFrame();
	void FillVbPosNormals(const VEC3 &Right, const VEC3 &Up);
};

class TextEntity : public CustomEntity
{
public:
	TextEntity(Scene *OwnerScene);
	TextEntity(Scene *OwnerScene, const string &Text, const string &FontName, COLOR Color, float FontSize, uint FontFlags);

	uint GetDegreesOfFreedom() { return m_DegreesOfFreedom; }
	bool GetUseRealDir() { return m_UseRealDir; }
	const VEC3 & GetRightDir() { return m_RightDir; }
	const VEC3 & GetUpDir() { return m_UpDir; }
	const string & GetText() { return m_Text; }
	const string & GetFontName() { return m_FontName; }
	float GetFontSize() { return m_FontSize; }
	uint GetFontFlags() { return m_FontFlags; }
	float GetTextWidth() { return m_TextWidth; }
	COLOR GetColor() { return m_Color; }
	BaseMaterial::BLEND_MODE GetBlendMode() { return m_BlendMode; }
	bool GetTwoSided() { return m_TwoSided; }
	uint GetColisionType() { return m_CollisionType; }

	void SetDegreesOfFreedom(uint DegreesOfFreedom);
	void SetUseRealDir(bool UseRealDir);
	void SetRightDir(const VEC3 &RightDir);
	void SetUpDir(const VEC3 &UpDir);
	void SetText(const string &Text);
	void SetFontName(const string &FontName);
	void SetFontSize(float FontSize);
	void SetFontFlags(uint FontFlags);
	void SetTextWidth(float TextWidth);
	void SetColor(COLOR Color);
	void SetBlendMode(BaseMaterial::BLEND_MODE BlendMode);
	void SetTwoSided(bool TwoSided);
	void SetCollisionType(uint CollisionType);

	// ======== Implementacja Entity ========
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT);

	// ======== Implementacja MaterialEntity ========
	virtual void Draw(const ParamsCamera &Cam);

private:
	// ============ Dane g��wne ========
	// 0, 1, 2
	uint m_DegreesOfFreedom;
	bool m_UseRealDir;
	// Znormalizowany
	VEC3 m_RightDir;
	// Znormalizowany
	VEC3 m_UpDir;
	string m_Text;
	string m_FontName;
	float m_FontSize;
	uint m_FontFlags;
	float m_TextWidth;
	COLOR m_Color;
	BaseMaterial::BLEND_MODE m_BlendMode;
	bool m_TwoSided;
	uint m_CollisionType;

	// ======== Dane generowane ========
	// Load()-owa� przed ka�dym u�yciem!
	res::Font *m_Font;
	// Rozmiary wzgl�dem �rodka lokalnego uk�adu tego obiektu
	VEC2 m_MinExtent;
	VEC2 m_MaxExtent;

	void ExtentChanged();
	// Wywo�uje tak�e ExtentChanged
	void FontNameChanged();
};

/*
Abstrakcyjna klasa bazowa dla encji prezentuj�cych w�ski, pod�u�ny, oteksturowany i pokryty materia�em pasek.
Nigdy nie robi kolizji.
Format wierzcho�ka jest X2. Nie ma wi�c tangent�w ani nawet normalnych - nie u�ywa� o�wietlenia!
Encja ma domy�lnie ustawione UseLighting==false, CastShadow==false, ReceiveShadow==false.
Zadaniem klas pochodnych jest zarz�dza� SetRadius.
Tworz�c materia� dla StripEntity nie zapomnie�, �eby by� TwoSided!
*/
class StripeEntity : public MaterialEntity
{
public:
	StripeEntity(Scene *OwnerScene);
	StripeEntity(Scene *OwnerScene, const string &MaterialName);
	~StripeEntity();

	const string & GetMaterialName() { return m_MaterialName; }
	float GetHalfWidth() { return m_HalfWidth; }
	const RECTF & GetTex() { return m_Tex; }
	float GetTexAnimVel() { return m_TexAnimVel; }
	float GetTexRandomPeriod() { return m_TexRandomPeriod; }
	void SetMaterialName(const string &MaterialName) { m_MaterialName = MaterialName; m_Material = NULL; }
	void SetHalfWidth(float HalfWidth) { m_HalfWidth = HalfWidth; }
	void SetTex(const RECTF &Tex) { m_Tex = Tex; }
	void SetTexAnimVel(float TexAnimVel) { m_TexAnimVel = TexAnimVel; }
	void SetTexRandomPeriod(float TexRandomPeriod) { m_TexRandomPeriod = TexRandomPeriod; }

	// ======== Implementacja Entity ========
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT) { return false; }
	virtual void Update();

	// ======== Implementacja MaterialEntity ========
	virtual uint GetFragmentCount();
	virtual void GetFragmentData(uint Index, uint *OutId, BaseMaterial **OutMaterial);
	virtual void DrawFragmentGeometry(uint Id, BaseMaterial *Mat, const ParamsCamera &Cam);

protected:
	// ======== Dla klas pochodnych ========
	// Ma zwr�ci� wska�nik do jakiej� swojej tablicy z ci�giem punkt�w i ich liczb�.
	virtual const VEC3 * GetStripPoints(uint *OutPointCount, bool *OutClosed) = 0;

private:
	// Wsp�lne dla wszystkich obiekt�w tej klasy
	// Tworzone przy tworzeniu pierwszego. Blokowane w konstruktorze, odblokowywane w destruktorze.
	static res::D3dVertexBuffer *m_VB;

	// ======== Ustawiane ========
	string m_MaterialName;
	float m_HalfWidth;
	RECTF m_Tex;
	float m_TexAnimVel;
	float m_TexRandomPeriod;

	// ======== Wyliczane ========
	// Pobierany przy pierwszym u�yciu
	BaseMaterial *m_Material;
	float m_LastRandomTexTime;

	// Tworzy zas�b m_VB, je�li nie s� utworzone. Blokuje je.
	void CreateBufferIfNecessary();
	// Pobiera materia� do m_Material.
	// Je�li nie ma materia�u, zwraca false.
	bool EnsureMaterial();
};

/*
Pasek - �amana z�o�ona z prostych odcink�w ��cz�cych kolejne punkty.
*/
class LineStripeEntity : public StripeEntity
{
public:
	LineStripeEntity(Scene *OwnerScene);
	LineStripeEntity(Scene *OwnerScene, const string &MaterialName);
	// Prosty odcinek z dw�ch punkt�w
	LineStripeEntity(Scene *OwnerScene, const string &MaterialName, const VEC3 &P1, const VEC3 &P2);
	// Tablica punkt�w, kopiowana przez konstruktor do wewn�trznych struktur obiektu.
	LineStripeEntity(Scene *OwnerScene, const string &MaterialName, const VEC3 *Points, uint PointCount);

	uint GetPointCount() { return m_Points.size(); }
	const VEC3 & GetPoint(uint Index) { return m_Points[Index]; }
	const VEC3 * GetPoints() { return &m_Points[0]; }

	// Ustawia pojedynczy punkt. Indeks musi by� w zakresie liczebno�ci.
	void SetPoint(uint Index, const VEC3 &Point);
	// Ustawia wszystkie punkty wraz z ich liczebno�ci�.
	void SetPoints(const VEC3 *Points, uint PointCount);

protected:
	// ======== Implementacja LineStripeEntity ========
	virtual const VEC3 * GetStripPoints(uint *OutPointCount, bool *OutClosed);

private:
	std::vector<VEC3> m_Points;

	// Aktualizuje Radius tej encji, je�li to konieczne
	void UpdateRadius();
};

/*
Okr�g lub elipsa wyznaczona przez dwa wektory promieni.
*/
class EllipseStripeEntity : public StripeEntity
{
public:
	EllipseStripeEntity(Scene *OwnerScene);
	EllipseStripeEntity(Scene *OwnerScene, const string &MaterialName, uint SegmentCount, const VEC3 &R1, const VEC3 &R2);

	uint GetSegmentCount() { return m_SegmentCount; }
	const VEC3 & GetR1() { return m_R1; }
	const VEC3 & GetR2() { return m_R2; }

	void SetSegmentCount(uint SegmentCount);
	void SetR1(const VEC3 &R1);
	void SetR2(const VEC3 &R2);

protected:
	// ======== Implementacja LineStripeEntity ========
	virtual const VEC3 * GetStripPoints(uint *OutPointCount, bool *OutClosed);

private:
	// Dane bezpo�rednio ustawiane
	uint m_SegmentCount;
	VEC3 m_R1;
	VEC3 m_R2;

	// Dane wyliczane
	bool m_PointsCalculated;
	std::vector<VEC3> m_Points;

	// Aktualizuje Radius tej encji, je�li to konieczne
	void UpdateRadius();
	// Aktualizuje m_Points
	void UpdatePoints();
};

/*
Wykres funkcji Uterm.
- Closed oznacza, �e ostatni punkt ma by� po��czony z pierwszym (nie musz� ani nie powinny le�e� w tym samym miejscu).
*/
class UtermStripeEntity : public StripeEntity
{
public:
	UtermStripeEntity(Scene *OwnerScene);
	UtermStripeEntity(Scene *OwnerScene, const string &MaterialName, uint SegmentCount, const Uterm3 &Term);

	uint GetSegmentCount() { return m_SegmentCount; }
	const Uterm3 & GetTerm() { return m_Term; }
	bool GetClosed() { return m_Closed; }

	void SetSegmentCount(uint SegmentCount);
	void SetTerm(const Uterm3 &Term);
	void SetClosed(bool Closed) { m_Closed = Closed; }

protected:
	// ======== Implementacja LineStripeEntity ========
	virtual const VEC3 * GetStripPoints(uint *OutPointCount, bool *OutClosed);

private:
	// Dane bezpo�rednio ustawiane
	uint m_SegmentCount;
	Uterm3 m_Term;
	bool m_Closed;

	// Dane wyliczane
	std::vector<VEC3> m_Points;

	// Aktualizuje m_Points oraz Radius tej encji
	void UpdatePoints();
};

/*
Pasek w kszta�cie krzywej - jednego z dost�pnych rodzaj�w.
*/
class CurveStripeEntity : public StripeEntity
{
public:
	enum CURVE_TYPE
	{
		// Krzywa kwadratowa (parabola).
		// Ka�dy segment u�ywa po 3 punkty, czyli kolejne segmenty wyznaczaj� punkty: 0-1-2, 2-3-4, 4-5-6 itd.
		// Domkni�cie wykonuje si� dodaj�c na ko�cu jeden dowolny punkt i dalej powtarzaj�c punkt pierwszy.
		CURVE_TYPE_QUADRATIC,
		// Ci�g krzywych sze�ciennych (Beziera).
		// Ka�dy segment u�ywa po 4 punkty, czyli kolejne segmenty wyznaczaj� punkty: 0-1-2-3, 3-4-5-6 itd.
		// Domkni�cie wykonuje si� dodaj�c na ko�cu dwa dowolne punkty i dalej powtarzaj�c punkt pierwszy.
		CURVE_TYPE_BEZIER,
		// Krzywa B-spline, 3 stopnia, wielomianowa, uniform
		// B�dzie si� rozci�ga�a od mniej wi�cej drugiego do mniej wi�cej przedostatniego punktu.
		// Aby krzywa zaczyna�a si� w punkcie pierwszym albo ko�czy�a w ostatnim, nale�y ten punkt powt�rzy� 3 razy.
		// Aby zrobi� zamkni�cie, nale�y na koniec doda� jeszcze raz trzy pierwsze punkty.
		// Aby przyci�gn�� krzyw� w pobli�e danego punktu, ale nadal wyg�adzon�, nale�y powt�rzy� punkt 2 razy.
		// Aby stworzy� ostry r�g w danym punkcie, nale�y powt�rzy� punkt 3 razy.
		CURVE_TYPE_BSPLINE,
	};

	CurveStripeEntity(Scene *OwnerScene);
	CurveStripeEntity(Scene *OwnerScene, const string &MaterialName, CURVE_TYPE CurveType, uint PointsPerSegment);

	// Ustawia typ, liczb� i tre�� punkt�w kontrolnych - pe�na konfiguracja krzywej.
	void SetupQuadratic(const VEC3 *ControlPoints, uint ControlPointCount);
	void SetupBezier(const VEC3 *ControlPoints, uint ControlPointCount);
	void SetupBspline(const VEC3 *ControlPoints, uint ControlPointCount);

	CURVE_TYPE GetCurveType() { return m_CurveType; }
	uint GetPointsPerSegment() { return m_PointsPerSegment; }
	uint GetControlPointCount() { return m_ControlPoints.size(); }
	const VEC3 * GetAllControlPoints() { return &m_ControlPoints[0]; }
	const VEC3 * GetControlPoints(uint StartIndex) { return &m_ControlPoints[StartIndex]; }
	const VEC3 & GetControlPoint(uint Index) { return m_ControlPoints[Index]; }

	void SetCurveType(CURVE_TYPE CurveType);
	void SetPointsPerSegment(uint PointsPerSegment);
	void SetControlPointCount(uint ControlPointCount);
	void SetAllControlPoints(const VEC3 *ControlPoints);
	void SetControlPoints(uint StartIndex, uint Count, const VEC3 *ControlPoints);
	void SetControlPoint(uint Index, const VEC3 &ControlPoint);

protected:
	// ======== Implementacja LineStripeEntity ========
	virtual const VEC3 * GetStripPoints(uint *OutPointCount, bool *OutClosed);

private:
	// Dane bezpo�rednio ustawiane
	CURVE_TYPE m_CurveType;
	uint m_PointsPerSegment;
	std::vector<VEC3> m_ControlPoints;

	// Dane wyliczane
	bool m_PointsCalculated;
	std::vector<VEC3> m_Points;

	// Aktualizuje Radius tej encji, je�li to konieczne
	void UpdateRadius();
	// Aktualizuje m_Points
	void UpdatePoints();
};

// Kolory s� w RGBA.
struct PARTICLE_DEF
{
	// Rodzaj emitera. Parametry emitera opisuj� PosA0.
	// 0 = brak okr�g�w, emiter jest punktem, lini�, prostok�tem lub prostopad�o�cianem.
	// 1 = okr�g w osi XZ, emiter jest okr�giem/kul� lub cylindrem.
	//     PosA0.x to promie�, PosA0.z to k�t
	// 2 = emiter jest kul�.
	//     PosA0.x to promie�, PosA0.y to k�t 1 (d�ugo�� geograficzna), PosA0.z to k�t 2 (szeroko�� geograficzna)
	uint CircleDegree;
	VEC3 PosA2_C, PosA2_P, PosA2_R;
	VEC3 PosA1_C, PosA1_P, PosA1_R;
	VEC3 PosA0_C, PosA0_P, PosA0_R;
	VEC3 PosB2_C, PosB2_P, PosB2_R;
	VEC3 PosB1_C, PosB1_P, PosB1_R;
	VEC3 PosB0_C, PosB0_P, PosB0_R;
	VEC4 ColorA2_C, ColorA2_P, ColorA2_R;
	VEC4 ColorA1_C, ColorA1_P, ColorA1_R;
	VEC4 ColorA0_C, ColorA0_P, ColorA0_R;
	float OrientationA1_C, OrientationA1_P, OrientationA1_R;
	float OrientationA0_C, OrientationA0_P, OrientationA0_R;
	float SizeA1_C, SizeA1_P, SizeA1_R;
	float SizeA0_C, SizeA0_P, SizeA0_R;
	float TimePeriod_C, TimePeriod_P, TimePeriod_R;
	float TimePhase_C, TimePhase_P, TimePhase_R;

	// Zeruje wszystkie sk�adowe
	void Zero();
};

class ParticleEntity : public CustomEntity
{
public:
	ParticleEntity(Scene *OwnerScene, const string &TextureName, BaseMaterial::BLEND_MODE BlendMode, const PARTICLE_DEF &Def, uint MaxParticleCount, float BoundingSphereRadius);
	~ParticleEntity();

	const string & GetTextureName() { return m_TextureName; }
	BaseMaterial::BLEND_MODE GetBlendMode() { return m_BlendMode; }
	const PARTICLE_DEF & GetDef() { return m_Def; }
	bool GetUseLOD() { return m_UseLOD; }

	void SetTextureName(const string &TextureName) { m_TextureName = TextureName; m_Texture = NULL; }
	void SetBlendMode(BaseMaterial::BLEND_MODE BlendMode) { m_BlendMode = BlendMode; }
	void SetUseLOD(bool UseLOD) { m_UseLOD = UseLOD; }

	// ======== Implementacja Entity ========
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT) { return false; }

	// ======== Implementacja CustomEntity ========
	virtual void Draw(const ParamsCamera &Cam);

private:
	// ======== Wpisywane ========
	string m_TextureName;
	BaseMaterial::BLEND_MODE m_BlendMode;
	PARTICLE_DEF m_Def;
	uint m_MaxParticleCount;
	bool m_UseLOD;
	float m_CreationTime;

	// ======== Wyliczane ========
	// Tworzony w konstruktorze, niszczony w destruktorze, Load-owany w razie potrzeby.
	scoped_ptr<res::D3dVertexBuffer> m_VB;
	scoped_ptr<res::D3dIndexBuffer> m_IB;
	// Wska�nik pobierany przy pierwszym u�yciu.
	res::D3dTexture *m_Texture;
	res::D3dEffect *m_Shader;
	VEC4 m_ShaderData[25];

	// Pobiera do m_Texture wska�nik do tekstury, je�li jeszcze nie by� pobrany. Robi te� Load.
	// Je�li si� nie uda, zwraca false.
	bool EnsureTexture();
	bool EnsureShader();
	// Blokuje m_VB i wpisuje do niego dane.
	// m_VB musi istnie� i by� Loaded.
	void FillVB();
	void FillIB();
	void GenParticleStartPos(VEC3 *Out, float f);
	// Inicjalizuje m_ShaderData (to co sta�e) na podstawie m_Def
	void InitShaderData();
};

} // namespace engine

#endif
