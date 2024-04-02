/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef CAMERA_H_
#define CAMERA_H_

// Kamera skonstruowana z macierzy widoku i rzutowania
class MatrixCamera
{
private:
	MATRIX m_View;
	MATRIX m_Proj;

	mutable MATRIX m_ViewInv; mutable bool m_ViewInvIs;
	mutable MATRIX m_ProjInv; mutable bool m_ProjInvIs;
	mutable MATRIX m_ViewProj; mutable bool m_ViewProjIs;
	mutable MATRIX m_ViewProjInv; mutable bool m_ViewProjInvIs;
	mutable FRUSTUM_PLANES m_FrustumPlanes; mutable bool m_FrustumPlanesIs;
	mutable FRUSTUM_POINTS m_FrustumPoints; mutable bool m_FrustumPointsIs;
	mutable BOX m_FrustumBox; mutable bool m_FrustumBoxIs;

	void Changed()
	{
		m_ViewInvIs = false;
		m_ProjInvIs = false;
		m_ViewProjIs = m_ViewProjInvIs = false;
		m_ViewProjIs = m_ViewProjInvIs = m_FrustumPlanesIs = m_FrustumPointsIs = m_FrustumBoxIs = false;
	}

public:
	// Tworzy NIEZAINICJALIZOWAN¥
	MatrixCamera() { Changed(); }
	// Tworzy zainicjalizowan¹
	MatrixCamera(const MATRIX &View, const MATRIX &Proj);

	void Set(const MATRIX &View, const MATRIX &Proj) { m_View = View; m_Proj = Proj; Changed(); }
	void SetView(const MATRIX &View) { m_View = View; Changed(); }
	void SetProj(const MATRIX &Proj) { m_Proj = Proj; Changed(); }

	const MATRIX & GetView() const { return m_View; }
	const MATRIX & GetProj() const { return m_Proj; }

	const MATRIX & GetViewInv() const;
	const MATRIX & GetProjInv() const;
	const MATRIX & GetViewProj() const;
	const MATRIX & GetViewProjInv() const;
	const FRUSTUM_PLANES & GetFrustumPlanes() const;
	const FRUSTUM_POINTS & GetFrustumPoints() const;
	const BOX & GetFrustumBox() const;
};

// Kamera skonstruowana z parametrów potrzebnych do utworzenia macierzy widoku i rzutowania
class ParamsCamera
{
private:
	VEC3 m_EyePos;
	VEC3 m_ForwardDir;
	VEC3 m_UpDir;
	float m_FovY;
	float m_Aspect;
	float m_ZNear;
	float m_ZFar;

	mutable MatrixCamera m_Matrices; mutable bool m_MatricesIs;
	mutable VEC3 m_RightDir; mutable bool m_RightDirIs;

	void Changed()
	{
		m_MatricesIs = m_RightDirIs = false;
	}

public:
	// Tworzy NIEZAINICJALIZOWAN¥
	ParamsCamera() { Changed(); }
	// Tworzy zainicjalizowan¹
	ParamsCamera(const VEC3 &EyePos, const VEC3 &ForwardDir, const VEC3 &UpDir, float FovY, float Aspect, float ZNear, float ZFar);

	void Set(const VEC3 &EyePos, const VEC3 &ForwardDir, const VEC3 &UpDir, float FovY, float Aspect, float ZNear, float ZFar);
	void SetEyePos(const VEC3 &EyePos) { m_EyePos = EyePos; Changed(); }
	void SetForwardDir(const VEC3 &ForwardDir) { m_ForwardDir = ForwardDir; Changed(); }
	void SetUpDir(const VEC3 &UpDir) { m_UpDir = UpDir; Changed(); }
	void SetFovY(float FovY) { m_FovY = FovY; Changed(); }
	void SetAspect(float Aspect) { m_Aspect = Aspect; Changed(); }
	void SetZNear(float ZNear) { m_ZNear = ZNear; Changed(); }
	void SetZFar(float ZFar) { m_ZFar = ZFar; Changed(); }
	void SetZRange(float ZNear, float ZFar) { m_ZNear = ZNear; m_ZFar = ZFar; Changed(); }

	const VEC3 & GetEyePos() const { return m_EyePos; }
	const VEC3 & GetForwardDir() const { return m_ForwardDir; }
	const VEC3 & GetUpDir() const { return m_UpDir; }
	float GetFovY() const { return m_FovY; }
	float GetAspect() const { return m_Aspect; }
	float GetZNear() const { return m_ZNear; }
	float GetZFar() const { return m_ZFar; }

	// Nigdy nie zapamiêtywaæ na d³u¿ej wskaŸnika albo referencji do tego co zwróci,
	// bo mo¿e byæ nieaktualne! - jest aktualizowane dopiero przy pobraniu.
	const MatrixCamera & GetMatrices() const;
	const VEC3 & GetRightDir() const;
	const VEC3 & GetRealUpDir() const;
};

namespace engine {
	class Scene;
}

// Klasa podstawowa kamery
/*
Kamera mo¿e pracowaæ w dwóch trybach:
- Tryb w którym orientacja jest okreœlania k¹tami Yaw i Pitch, jak w widoku FPP
  czy TPP. Dodatkowo posiada odsuniêcie punktu widzenia od okreœlonego punktu
  œrodkowego.
- Tryb, w którym kamera znajduje siê w punkcie œrodkowym, a orientacjê okreœla
  kwaternion.
*/
class Camera
{
public:
	enum MODE
	{
		MODE_CHARACTER,
		MODE_QUATERNION,
	};

	// Tworzy NIEZAINICJALIZOWAN¥
	Camera(engine::Scene *Owner);
	// Tworzy zainicjalizowan¹ w trybie CHARACTER
	Camera(engine::Scene *Owner, const VEC3 &Pos, float AngleX, float AngleY, float CameraDist, float ZNear, float ZFar, float FovY, float Aspect);
	// Tworzy zainicjalizowan¹ w trybie QUATERION
	Camera(engine::Scene *Owner, const VEC3 &Pos, const QUATERNION &Orientation, float ZNear, float ZFar, float FovY, float Aspect);
	~Camera();

	// Wype³nia od nowa wszystkie dane, ustawia tryb CHARACTER
	void Set(const VEC3 &Pos, float AngleX, float AngleY, float CameraDist, float ZNear, float ZFar, float FovY, float Aspect);
	// Wype³nia od nowa wszystkie dane, ustawia tryb QUATERNION
	void Set(const VEC3 &Pos, const QUATERNION &Orientation, float ZNear, float ZFar, float FovY, float Aspect);

	void SetPos(const VEC3 &Pos) { m_Pos = Pos; Changed(); }
	void SetZNear(float ZNear) { m_ZNear = ZNear; Changed(); }
	void SetZFar(float ZFar) { m_ZFar = ZFar; Changed(); }
	void SetZRange(float ZNear, float ZFar) { m_ZNear = ZNear; m_ZFar = ZFar; Changed(); }
	void SetFovY(float FovY) { m_FovY = FovY; Changed(); }
	void SetAspect(float Aspect) { m_Aspect = Aspect; Changed(); }
	void SetMode(MODE Mode) { m_Mode = Mode; Changed(); }
	// Dane tylko dla MODE_CHARACTER
	void SetAngleX(float AngleX) { m_AngleX = minmax(-PI_2+1e-3f, AngleX, +PI_2-1e-3f); Changed(); }
	void SetAngleY(float AngleY) { m_AngleY = NormalizeAngle(AngleY); Changed(); }
	void SetCameraDist(float CameraDist) { m_CameraDist = CameraDist; Changed(); }
	// Dane tylko dla MODE_QUATERNION
	void SetOrientation(const QUATERNION &Orientation) { m_Orientation = Orientation; Changed(); }

	void Move(const VEC3 &DeltaPos) { SetPos(GetPos() + DeltaPos); }
	void RotateX(float DeltaAngleX) { SetAngleX(GetAngleX() + DeltaAngleX); }
	void RotateY(float DeltaAngleY) { SetAngleY(GetAngleY() + DeltaAngleY); }

	engine::Scene * GetOwner() const { return m_Owner; }
	const VEC3 & GetPos() const { return m_Pos; }
	float GetZNear() const { return m_ZNear; }
	float GetZFar() const { return m_ZFar; }
	float GetFovY() const { return m_FovY; }
	float GetAspect() const { return m_Aspect; }
	MODE GetMode() const { return m_Mode; }
	// Dane tylko dla MODE_CHARACTER
	float GetAngleX() const { return m_AngleX; }
	float GetAngleY() const { return m_AngleY; }
	float GetCameraDist() const { return m_CameraDist; }
	// Dane tylko dla MODE_QUATERNION
	const QUATERNION & GetOrientation() const { return m_Orientation; }

	// Nigdy nie zapamiêtywaæ na d³u¿ej wskaŸnika albo referencji do tego co zwróci,
	// bo mo¿e byæ nieaktualne! - jest aktualizowane dopiero przy pobraniu.
	const ParamsCamera & GetParams() const;
	const MatrixCamera & GetMatrices() const;

	// Wyznacza promieñ we wsp. globalnych œwiata wychodz¹cy z ekranu
	// Wspo³rzêdne myszki nale¿y podaæ tak ¿e X zmienia siê w prawo 0..1, a Y w dó³ 0..1
	// RayDir wychodzi znormalizowane
	void CalcMouseRay(VEC3 *OutRayOrig, VEC3 *OutRayDir, float MouseX, float MouseY) const;

private:
	engine::Scene *m_Owner;
	// Pozycja gracza
	VEC3 m_Pos;
	// Odleg³oœci do rzutowania
	float m_ZNear, m_ZFar;
	// K¹t widzenia
	float m_FovY;
	// Stosunek szerokoœci do wysokoœci rzutowania
	float m_Aspect;

	MODE m_Mode;

	// ======== Parametry dla MODE_CHARACTER ========
	// Nachylenie do góry / w dó³
	float m_AngleX;
	// Ten zwyk³y, g³ówny obrót
	float m_AngleY;
	// Odleg³oœæ kamery TPP od gracza
	float m_CameraDist;
	// ======== Parametry dla MODE_QUATERNION ========
	QUATERNION m_Orientation;

	mutable ParamsCamera m_Params; mutable bool m_ParamsIs;

	void Changed() { m_ParamsIs = false; }
};

#endif
