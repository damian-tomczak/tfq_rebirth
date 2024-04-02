/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "Engine.hpp"
#include "Camera.hpp"

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa MatrixCamera

MatrixCamera::MatrixCamera(const MATRIX &View, const MATRIX &Proj) :
	m_View(View),
	m_Proj(Proj)
{
	Changed();
}

const MATRIX & MatrixCamera::GetViewInv() const
{
	if (!m_ViewInvIs)
	{
		Inverse(&m_ViewInv, GetView());
		m_ViewInvIs = true;
	}
	return m_ViewInv;
}

const MATRIX & MatrixCamera::GetProjInv() const
{
	if (!m_ProjInvIs)
	{
		Inverse(&m_ProjInv, GetProj());
		m_ProjInvIs = true;
	}
	return m_ProjInv;
}

const MATRIX & MatrixCamera::GetViewProj() const
{
	if (!m_ViewProjIs)
	{
		m_ViewProj = GetView() * GetProj();
		m_ViewProjIs = true;
	}
	return m_ViewProj;
}

const MATRIX & MatrixCamera::GetViewProjInv() const
{
	if (!m_ViewProjInvIs)
	{
		Inverse(&m_ViewProjInv, GetViewProj());
		m_ViewProjInvIs = true;
	}
	return m_ViewProjInv;
}

const FRUSTUM_PLANES & MatrixCamera::GetFrustumPlanes() const
{
	if (!m_FrustumPlanesIs)
	{
		m_FrustumPlanes.Set(GetViewProj()); // Tutaj nie ma INV!
		m_FrustumPlanes.Normalize();
		m_FrustumPlanesIs = true;
	}
	return m_FrustumPlanes;
}

const FRUSTUM_POINTS & MatrixCamera::GetFrustumPoints() const
{
	if (!m_FrustumPointsIs)
	{
		// Metoda 1
		m_FrustumPoints.Set(GetViewProjInv()); // Tutaj jest INV!

		// Metoda 2
		//m_FrustumPoints.Create(GetFrustumPlanes());

		m_FrustumPointsIs = true;
	}
	return m_FrustumPoints;
}

const BOX & MatrixCamera::GetFrustumBox() const
{
	if (!m_FrustumBoxIs)
	{
		GetFrustumPoints().CalcBoundingBox(&m_FrustumBox);
		m_FrustumBoxIs = true;
	}
	return m_FrustumBox;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ParamsCamera

ParamsCamera::ParamsCamera(const VEC3 &EyePos, const VEC3 &ForwardDir, const VEC3 &UpDir, float FovY, float Aspect, float ZNear, float ZFar) :
	m_EyePos(EyePos),
	m_ForwardDir(ForwardDir),
	m_UpDir(UpDir),
	m_FovY(FovY),
	m_Aspect(Aspect),
	m_ZNear(ZNear),
	m_ZFar(ZFar)
{
	Changed();
}

void ParamsCamera::Set(const VEC3 &EyePos, const VEC3 &ForwardDir, const VEC3 &UpDir, float FovY, float Aspect, float ZNear, float ZFar)
{
	m_EyePos = EyePos;
	m_ForwardDir = ForwardDir;
	m_UpDir = UpDir;
	m_FovY = FovY;
	m_Aspect = Aspect;
	m_ZNear = ZNear;
	m_ZFar = ZFar;
	Changed();
}

const MatrixCamera & ParamsCamera::GetMatrices() const
{
	if (!m_MatricesIs)
	{
		MATRIX View, Proj;

		LookAtLH(&View, GetEyePos(), GetForwardDir(), GetUpDir());
		PerspectiveFovLH(&Proj, GetFovY(), GetAspect(), GetZNear(), GetZFar());

		m_Matrices.Set(View, Proj);
		m_MatricesIs = true;
	}
	return m_Matrices;
}

const VEC3 & ParamsCamera::GetRightDir() const
{
	if (!m_RightDirIs)
	{
		// WERSJA 1
		Cross(&m_RightDir, GetUpDir(), GetForwardDir());
		Normalize(&m_RightDir);

		// WERSJA 2
		//m_RightDir = (const VEC3 &)GetMatrices().GetViewInv()._11;

		m_RightDirIs = true;
	}
	return m_RightDir;
}

const VEC3 & ParamsCamera::GetRealUpDir() const
{
	return (const VEC3 &)GetMatrices().GetViewInv()._21;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Camera

Camera::Camera(engine::Scene *Owner) :
	m_Owner(Owner)
{
	m_Owner->RegisterCamera(this);
	Changed();
}

Camera::Camera(engine::Scene *Owner, const VEC3 &Pos, float AngleX, float AngleY, float CameraDist, float ZNear, float ZFar, float FovY, float Aspect) :
	m_Owner(Owner),
	m_Pos(Pos),
	m_ZNear(ZNear),
	m_ZFar(ZFar),
	m_FovY(FovY),
	m_Aspect(Aspect),
	m_Mode(MODE_CHARACTER),
	m_AngleX(AngleX),
	m_AngleY(AngleY),
	m_CameraDist(CameraDist),
	m_Orientation(QUATERNION::IDENTITY)
{
	Changed();
}

Camera::Camera(engine::Scene *Owner, const VEC3 &Pos, const QUATERNION &Orientation, float ZNear, float ZFar, float FovY, float Aspect) :
	m_Owner(Owner),
	m_Pos(Pos),
	m_ZNear(ZNear),
	m_ZFar(ZFar),
	m_FovY(FovY),
	m_Aspect(Aspect),
	m_Mode(MODE_QUATERNION),
	m_AngleX(0.f),
	m_AngleY(0.f),
	m_CameraDist(0.f),
	m_Orientation(Orientation)
{
}

Camera::~Camera()
{
	m_Owner->UnregisterCamera(this);
}

void Camera::Set(const VEC3 &Pos, float AngleX, float AngleY, float CameraDist, float ZNear, float ZFar, float FovY, float Aspect)
{
	m_Pos = Pos;
	m_ZNear = ZNear;
	m_ZFar = ZFar;
	m_FovY = FovY;
	m_Aspect = Aspect;
	m_Mode = MODE_CHARACTER;
	m_AngleX = AngleX;
	m_AngleY = AngleY;
	m_CameraDist = CameraDist;
	Changed();
}

void Camera::Set(const VEC3 &Pos, const QUATERNION &Orientation, float ZNear, float ZFar, float FovY, float Aspect)
{
	m_Pos = Pos;
	m_ZNear = ZNear;
	m_ZFar = ZFar;
	m_FovY = FovY;
	m_Aspect = Aspect;
	m_Mode = MODE_CHARACTER;
	m_Orientation = Orientation;
	Changed();
}

const ParamsCamera & Camera::GetParams() const
{
	if (!m_ParamsIs)
	{
		if (m_Mode == MODE_CHARACTER)
		{
			VEC3 EyePos, ForwardDir, UpDir = VEC3::POSITIVE_Y;

			// Wersja 1 lub 2
			//MATRIX Rot;

			// Wersja 1
			//RotationYawPitchRoll(&Rot, GetAngleY(), GetAngleX(), 0.f);

			// Wersja 2 - Rozpisane dla optymalizacji.
			//float sy = sinf(GetAngleY()), cy = cosf(GetAngleY());
			//float sp = sinf(GetAngleX()), cp = cosf(GetAngleX());
			//Rot._11 = cy;    Rot._12 = 0.0f; Rot._13 = -sy;   Rot._14 = 0.0f;
			//Rot._21 = sy*sp; Rot._22 = cp;   Rot._23 = cy*sp; Rot._24 = 0.0f;
			//Rot._31 = sy*cp; Rot._32 = -sp;  Rot._33 = cy*cp; Rot._34 = 0.0f;
			//Rot._41 = 0.0f;  Rot._42 = 0.0f; Rot._43 = 0.0f;  Rot._44 = 1.0f;
			//VEC3 v = VEC3::POSITIVE_Z;

			// Wersja 1 lub 2
			//Transform(&ForwardDir, v, Rot);

			// Wersja 3
			SphericalToCartesian(&ForwardDir, GetAngleY() - PI_2, -GetAngleX(), 1.f);

			// FPP
			if (GetCameraDist() == 0.f)
				EyePos = GetPos();
			// TPP
			else
				EyePos = GetPos() - ForwardDir * GetCameraDist();

			m_Params.Set(EyePos, ForwardDir, UpDir, GetFovY(), GetAspect(), GetZNear(), GetZFar());
		}
		else // m_Params == PARAMS_QUATERNION
		{
			MATRIX RotMat;
			QuaternionToRotationMatrix(&RotMat, m_Orientation);
			VEC3 Forward, Up;
			Transform(&Forward, VEC3::POSITIVE_Z, RotMat);
			Transform(&Up,      VEC3::POSITIVE_Y, RotMat);
			m_Params.Set(GetPos(), Forward, Up, GetFovY(), GetAspect(), GetZNear(), GetZFar());
		}

		m_ParamsIs = true;
	}
	return m_Params;
}

const MatrixCamera & Camera::GetMatrices() const
{
	return GetParams().GetMatrices();
}

void Camera::CalcMouseRay(VEC3 *OutRayOrig, VEC3 *OutRayDir, float MouseX, float MouseY) const
{
	VEC3 PtNear_Proj = VEC3(MouseX * 2.f - 1.f, (1.f - MouseY) * 2.f - 1.f, 0.f);
	VEC3 PtFar_Proj = VEC3(PtNear_Proj.x, PtNear_Proj.y, 1.f);

	const MATRIX & ViewProjInv = GetMatrices().GetViewProjInv();

	VEC3 PtNear_World, PtFar_World;
	TransformCoord(&PtNear_World, PtNear_Proj, ViewProjInv);
	TransformCoord(&PtFar_World,  PtFar_Proj,  ViewProjInv);

	*OutRayOrig = PtNear_World;
	*OutRayDir = PtFar_World - PtNear_World;
	Normalize(OutRayDir);
}
