/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\Gfx2D.hpp"
#include "..\Framework\ResMngr.hpp"
#include "..\Framework\Res_d3d.hpp"
#include "..\Engine\Camera.hpp"
#include "MathHelpDrawing.hpp"


// D³ugoœæ wektora
const float MathHelpDrawing::VECTOR_LENGTH = 1.f;
// D³ugoœæ ramion grota wektora
const float MathHelpDrawing::ARROW_PART_LENGTH = 0.2f;
// Liczba wierzcho³ków wielok¹ta rysowanego jako punkt
const uint4 MathHelpDrawing::POINT_VERTEX_COUNT = 6;
// Promieñ punktu, wsp. œwiata
const float MathHelpDrawing::POINT_RADIUS = 0.1f;
// Nazwa zasobu efektu
const char * const MathHelpDrawing::EFFECT_NAME = "MathHelpEffect";
// Nazwa zasobu czcionki
const char * const MathHelpDrawing::FONT_NAME = "Font02";
// Przesuniêcie tekstu we wsp. ekranu
const VEC2 MathHelpDrawing::TEXT_OFFSET = VEC2(4.f, 4.f);
// Rozmiar czcionki
const float MathHelpDrawing::FONT_SIZE = 14.0f;
// Liczba odcinków przybli¿enia okrêgu rysowanego jako sfera
const uint4 SPHERE_VERTEX_COUNT = 24;


MathHelpDrawing::MathHelpDrawing() :
	m_Camera(NULL)
{
}

void MathHelpDrawing::Begin(const ParamsCamera *Camera)
{
	m_Camera = Camera;

	// Na wszelki wypadek
	m_SolidVertices.clear();
	m_LineVertices.clear();
	m_TextData.clear();
}

void MathHelpDrawing::End()
{
	assert(m_Camera);

	using frame::Dev;

	MATRIX ViewProjMatrix = m_Camera->GetMatrices().GetViewProj();

	frame::RestoreDefaultState();

	// Pobierz efekt

	res::D3dEffect *Effect = res::g_Manager->MustGetResourceEx<res::D3dEffect>(EFFECT_NAME);
	Effect->Load();
	ID3DXEffect *FX = Effect->Get();
	UINT Passes, PrimitiveCount;

	// Narysuj solid
	if (!m_SolidVertices.empty())
	{
		FX->SetTechnique("Solid");
		FX->SetMatrix("WorldViewProj", &math_cast<D3DXMATRIX>(ViewProjMatrix));
		FX->Begin(&Passes, D3DXFX_DONOTSAVESTATE);
		FX->BeginPass(0);

		PrimitiveCount = m_SolidVertices.size() / 3;

		Dev->SetFVF(VERTEX::FVF);
		Dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, PrimitiveCount, &m_SolidVertices[0], sizeof(VERTEX));
		frame::RegisterDrawCall(PrimitiveCount);
		
		FX->EndPass();
		FX->End();
	}

	// Narysuj linie
	if (!m_LineVertices.empty())
	{
		FX->SetTechnique("Lines");
		FX->SetMatrix("WorldViewProj", &math_cast<D3DXMATRIX>(ViewProjMatrix));
		FX->Begin(&Passes, D3DXFX_DONOTSAVESTATE);
		FX->BeginPass(0);

		PrimitiveCount = m_LineVertices.size() / 2;

		Dev->SetFVF(VERTEX::FVF);
		Dev->DrawPrimitiveUP(D3DPT_LINELIST, PrimitiveCount, &m_LineVertices[0], sizeof(VERTEX));
		frame::RegisterDrawCall(PrimitiveCount);
		
		FX->EndPass();
		FX->End();
	}

	// Narysuj tekst
	if (!m_TextData.empty())
	{
		frame::RestoreDefaultState();
		Dev->SetRenderState(D3DRS_LIGHTING, FALSE);
		Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
		Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

		D3DXMATRIX Proj2D; frame::GetMouseCoordsProjection(&Proj2D);
		Dev->SetTransform(D3DTS_PROJECTION, &Proj2D);

		gfx2d::g_Canvas->SetFont(res::g_Manager->MustGetResourceExf<res::Font>(FONT_NAME));

		VEC3 p;
		VEC2 p2;

		for (size_t ti = 0; ti < m_TextData.size(); ti++)
		{
			TransformCoord(&p, m_TextData[ti].Pos, ViewProjMatrix);
			if (p.z >= 0.f && p.z <= 1.f)
			{
				p2.x = (p.x + 1.f) * 0.5f * frame::GetScreenWidth() + TEXT_OFFSET.x;
				p2.y = (-p.y + 1.f) * 0.5f * frame::GetScreenHeight() + TEXT_OFFSET.y;
				gfx2d::g_Canvas->SetColor(m_TextData[ti].Color);
				gfx2d::g_Canvas->DrawText_(p2.x, p2.y, m_TextData[ti].Text, FONT_SIZE,
					res::Font::FLAG_HLEFT | res::Font::FLAG_VTOP | res::Font::FLAG_WRAP_NORMAL, 1.f);
			}
		}

		gfx2d::g_Canvas->Flush();
	}

	// Koniec - wyczyœæ

	m_SolidVertices.clear();
	m_LineVertices.clear();
	m_TextData.clear();
}

void MathHelpDrawing::DrawPoint(const VEC3 &Pos, COLOR Color, const string &Text)
{
	assert(m_Camera);

	VEC3 Right = m_Camera->GetRightDir() * POINT_RADIUS;
	VEC3 Up = m_Camera->GetRealUpDir() * POINT_RADIUS;

	float Angle = 0.f;
	float AngleStep = PI_X_2 / POINT_VERTEX_COUNT;
	VEC3 First, Next;
	First = Pos + cosf(Angle) * Right + sinf(Angle) * Up;
	m_SolidVertices.push_back(VERTEX(Pos, Color));
	m_SolidVertices.push_back(VERTEX(First, Color));
	Angle += AngleStep;

	for (uint4 i = 0; i < POINT_VERTEX_COUNT-1; i++)
	{
		Next = Pos + cosf(Angle) * Right + sinf(Angle) * Up;
		m_SolidVertices.push_back(VERTEX(Next, Color));
		m_SolidVertices.push_back(VERTEX(Next, Color));
		m_SolidVertices.push_back(VERTEX(Pos, Color));
		Angle += AngleStep;
	}

	m_SolidVertices.push_back(VERTEX(First, Color));

	if (!Text.empty())
		m_TextData.push_back(TEXT_DESC( Pos, Color, Text ));
}

void MathHelpDrawing::DrawLine(const VEC3 &p0, const VEC3 &p1, COLOR Color, const string &Text)
{
	m_LineVertices.push_back(VERTEX(p0, Color));
	m_LineVertices.push_back(VERTEX(p1, Color));

	if (!Text.empty())
		m_TextData.push_back(TEXT_DESC( (p0+p1)*0.5f, Color, Text ));
}

void MathHelpDrawing::DrawVector(const VEC3 &Pos, const VEC3 &Dir, COLOR Color, const string &Text)
{
	assert(m_Camera);

	VEC3 End = Pos + Dir * VECTOR_LENGTH;

	// Linia
	m_LineVertices.push_back(VERTEX(Pos, Color));
	m_LineVertices.push_back(VERTEX(End, Color));

	// Grot
	VEC3 va; Cross(&va, Dir, m_Camera->GetForwardDir()); Normalize(&va);
	VEC3 vb = -va;
	VEC3 vga = va * 0.25f - Dir * 0.75f;
	VEC3 vgb = vb * 0.25f - Dir * 0.75f;
	Normalize(&vga);
	Normalize(&vgb);
	vga *= ARROW_PART_LENGTH;
	vgb *= ARROW_PART_LENGTH;
	m_LineVertices.push_back(VERTEX(End, Color));
	m_LineVertices.push_back(VERTEX(End+vga, Color));
	m_LineVertices.push_back(VERTEX(End, Color));
	m_LineVertices.push_back(VERTEX(End+vgb, Color));

	// Napis
	if (!Text.empty())
		m_TextData.push_back(TEXT_DESC( End, Color, Text ));
}

void MathHelpDrawing::DrawBox(const BOX &Box, COLOR Color, const string &Text)
{
	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p1.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p1.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p2.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p2.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p1.y, Box.p2.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p1.y, Box.p2.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p2.y, Box.p2.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p2.y, Box.p2.z), Color));

	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p1.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p2.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p1.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p2.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p1.y, Box.p2.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p2.y, Box.p2.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p1.y, Box.p2.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p2.y, Box.p2.z), Color));

	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p1.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p1.y, Box.p2.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p1.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p1.y, Box.p2.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p2.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p1.x, Box.p2.y, Box.p2.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p2.y, Box.p1.z), Color));
	m_LineVertices.push_back(VERTEX(VEC3(Box.p2.x, Box.p2.y, Box.p2.z), Color));

	if (!Text.empty())
	{
		VEC3 Center; Box.CalcCenter(&Center);
		m_TextData.push_back(TEXT_DESC( Center, Color, Text ));
	}
}

void MathHelpDrawing::DrawSphere(const VEC3 &Center, float Radius, COLOR Color, const string &Text)
{
	assert(m_Camera);

	VEC3 Right = m_Camera->GetRightDir() * Radius;
	VEC3 Up = m_Camera->GetRealUpDir() * Radius;

	// Obwód ko³a
	float Angle = 0.f;
	float AngleStep = PI_X_2 / SPHERE_VERTEX_COUNT;
	VEC3 First, Next;
	First = Center + cosf(Angle) * Right + sinf(Angle) * Up;
	m_LineVertices.push_back(VERTEX(First, Color));
	Angle += AngleStep;

	for (uint4 i = 0; i < SPHERE_VERTEX_COUNT-1; i++)
	{
		Next = Center + cosf(Angle) * Right + sinf(Angle) * Up;
		m_LineVertices.push_back(VERTEX(Next, Color));
		m_LineVertices.push_back(VERTEX(Next, Color));
		Angle += AngleStep;
	}

	m_LineVertices.push_back(VERTEX(First, Color));

	// Napis
	if (!Text.empty())
		m_TextData.push_back(TEXT_DESC( Center, Color, Text ));
}

void MathHelpDrawing::DrawTriangle(const VEC3 &p1, const VEC3 &p2, const VEC3 &p3, COLOR Color, bool Solid, const string &Text)
{
	if (Solid)
	{
		m_SolidVertices.push_back(VERTEX(p1, Color));
		m_SolidVertices.push_back(VERTEX(p2, Color));
		m_SolidVertices.push_back(VERTEX(p3, Color));
	}
	else
	{
		DrawLine(p1, p2, Color);
		DrawLine(p2, p3, Color);
		DrawLine(p3, p1, Color);
	}

	if (!Text.empty())
		m_TextData.push_back(TEXT_DESC( (p1+p2+p3)/3.f, Color, Text ));
}

void MathHelpDrawing::DrawFrustum(const FRUSTUM_POINTS &Frustum, COLOR Color, const string &Text)
{
	DrawLine(Frustum[FRUSTUM_POINTS::NEAR_LEFT_TOP],     Frustum[FRUSTUM_POINTS::NEAR_RIGHT_TOP],    Color);
	DrawLine(Frustum[FRUSTUM_POINTS::NEAR_LEFT_BOTTOM],  Frustum[FRUSTUM_POINTS::NEAR_RIGHT_BOTTOM], Color);
	DrawLine(Frustum[FRUSTUM_POINTS::FAR_LEFT_TOP],      Frustum[FRUSTUM_POINTS::FAR_RIGHT_TOP],     Color);
	DrawLine(Frustum[FRUSTUM_POINTS::FAR_LEFT_BOTTOM],   Frustum[FRUSTUM_POINTS::FAR_RIGHT_BOTTOM],  Color);

	DrawLine(Frustum[FRUSTUM_POINTS::NEAR_LEFT_TOP],     Frustum[FRUSTUM_POINTS::NEAR_LEFT_BOTTOM],  Color);
	DrawLine(Frustum[FRUSTUM_POINTS::NEAR_RIGHT_TOP],    Frustum[FRUSTUM_POINTS::NEAR_RIGHT_BOTTOM], Color);
	DrawLine(Frustum[FRUSTUM_POINTS::FAR_LEFT_TOP],      Frustum[FRUSTUM_POINTS::FAR_LEFT_BOTTOM],   Color);
	DrawLine(Frustum[FRUSTUM_POINTS::FAR_RIGHT_TOP],     Frustum[FRUSTUM_POINTS::FAR_RIGHT_BOTTOM],  Color);

	DrawLine(Frustum[FRUSTUM_POINTS::NEAR_LEFT_TOP],     Frustum[FRUSTUM_POINTS::FAR_LEFT_TOP],      Color);
	DrawLine(Frustum[FRUSTUM_POINTS::NEAR_LEFT_BOTTOM],  Frustum[FRUSTUM_POINTS::FAR_LEFT_BOTTOM],   Color);
	DrawLine(Frustum[FRUSTUM_POINTS::NEAR_RIGHT_TOP],    Frustum[FRUSTUM_POINTS::FAR_RIGHT_TOP],     Color);
	DrawLine(Frustum[FRUSTUM_POINTS::NEAR_RIGHT_BOTTOM], Frustum[FRUSTUM_POINTS::FAR_RIGHT_BOTTOM],  Color);

	if (!Text.empty())
	{
		VEC3 c1 = (Frustum[FRUSTUM_POINTS::NEAR_LEFT_TOP] + Frustum[FRUSTUM_POINTS::NEAR_RIGHT_BOTTOM]) * 0.5f;
		VEC3 c2 = (Frustum[FRUSTUM_POINTS::FAR_RIGHT_TOP] + Frustum[FRUSTUM_POINTS::FAR_LEFT_BOTTOM])   * 0.5f;
		m_TextData.push_back(TEXT_DESC( (c1+c2)*0.5f, Color, Text ));
	}
}

void MathHelpDrawing::DrawPlane(const PLANE &Plane, COLOR Color, float Density)
{
	VEC3 Center;
	ClosestPointOnPlane(&Center, Plane, m_Camera->GetEyePos());

	VEC3 TmpVec = VEC3::POSITIVE_Z;
	if (fabsf(Plane.GetNormal().z) > 0.95f)
		TmpVec = VEC3::POSITIVE_Y;
	
	VEC3 Dir1; Cross(&Dir1, Plane.GetNormal(), TmpVec);
	VEC3 Dir2; Cross(&Dir2, Plane.GetNormal(), Dir1);
	Normalize(&Dir1);
	Normalize(&Dir2);

	float i1 = - m_Camera->GetZFar() * 0.5f;
	float i2 =   m_Camera->GetZFar() * 0.5f;

	for (float i = i1; i <= i2; i += Density)
		DrawLine(Center + Dir2 * i + Dir1 * i1, Center + Dir2 * i + Dir1 * i2, Color);
	for (float i = i1; i <= i2; i += Density)
		DrawLine(Center + Dir1 * i + Dir2 * i1, Center + Dir1 * i + Dir2 * i2, Color);
}

MathHelpDrawing *g_MathHelpDrawing = NULL;
