/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\D3dUtils.hpp"
#include "Engine_pp.hpp"

namespace engine
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PpEffect

void PpEffect::BlurPass(float SampleDistance, IDirect3DSurface9 *Dst, IDirect3DTexture9 *Src, uint4 CX, uint4 CY)
{
	RenderTargetHelper rth(Dst, NULL);

	frame::RestoreDefaultState();

	if (m_BlurPassEffect == NULL)
		m_BlurPassEffect = res::g_Manager->MustGetResourceEx<res::D3dEffect>("PpBlurPass");
	m_BlurPassEffect->Load();
	ID3DXEffect *Effect = m_BlurPassEffect->Get();

	float InvCX = 1.f / CX;
	float InvCY = 1.f / CY;

	Effect->SetTexture(m_BlurPassEffect->GetParam(0), Src);

	UINT Foo;
	Effect->Begin(&Foo, D3DXFX_DONOTSAVESTATE);
	Effect->BeginPass(0);

	struct VERTEX
	{
		VEC4 Pos;
		VEC2 Tex[4];
	};

	VEC2 OriginalTex[4];
	OriginalTex[0] = VEC2(0.f, 0.f);
	OriginalTex[1] = VEC2(1.f, 0.f);
	OriginalTex[2] = VEC2(0.f, 1.f);
	OriginalTex[3] = VEC2(1.f, 1.f);

	VERTEX V[4];
	V[0].Pos = VEC4(0.f,       0.f,       0.5f, 1.f);
	V[1].Pos = VEC4((float)CX, 0.f,       0.5f, 1.f);
	V[2].Pos = VEC4(0.f,       (float)CY, 0.5f, 1.f);
	V[3].Pos = VEC4((float)CX, (float)CY, 0.5f, 1.f);

	for (int i = 0; i < 4; i++)
	{
		V[i].Tex[0] = OriginalTex[i] + VEC2(-SampleDistance*InvCX, -SampleDistance*InvCY);
		V[i].Tex[1] = OriginalTex[i] + VEC2( SampleDistance*InvCX, -SampleDistance*InvCY);
		V[i].Tex[2] = OriginalTex[i] + VEC2(-SampleDistance*InvCX,  SampleDistance*InvCY);
		V[i].Tex[3] = OriginalTex[i] + VEC2( SampleDistance*InvCX,  SampleDistance*InvCY);
	}

	// Offset, ¿eby by³o pixel perfect!
	VEC4 Offset = VEC4(-0.5f, -0.5f, 0.f, 0.f);
	V[0].Pos += Offset; V[1].Pos += Offset;
	V[2].Pos += Offset; V[3].Pos += Offset;

	HRESULT hr;

	D3DVIEWPORT9 Viewport = { 0, 0, CX, CY, 0.f, 1.f };
	if (FAILED(hr = frame::Dev->SetViewport(&Viewport)))
		throw DirectXError(hr, "PpEffect::BlurPass: SetViewport failed.", __FILE__, __LINE__);

	frame::Dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX4);
	if (FAILED(hr = frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(VERTEX))))
		throw DirectXError(hr, "PpEffect::BlurPass: DrawPrimitiveUP failed.", __FILE__, __LINE__);
	frame::RegisterDrawCall(2);

	Effect->EndPass();
	Effect->End();
}

void PpEffect::Redraw(IDirect3DSurface9 *Dst, IDirect3DTexture9 *Src, const RECTF &DstRect)
{
	RenderTargetHelper rth(Dst, NULL);

	frame::RestoreDefaultState();

	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	frame::Dev->SetTexture(0, Src);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	frame::Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	frame::Dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	VERTEX_R2 V[4];
	V[0].Pos = VEC4(DstRect.left,  DstRect.top,    0.f, 1.f);
	V[1].Pos = VEC4(DstRect.right, DstRect.top,    0.f, 1.f);
	V[2].Pos = VEC4(DstRect.left,  DstRect.bottom, 0.f, 1.f);
	V[3].Pos = VEC4(DstRect.right, DstRect.bottom, 0.f, 1.f);
	
	// Offset, ¿eby by³o pixel perfect!
	VEC4 Offset = VEC4(-0.5f, -0.5f, 0.f, 0.f);
	V[0].Pos += Offset; V[1].Pos += Offset;
	V[2].Pos += Offset; V[3].Pos += Offset;

	// Wspó³rzêdne tekstury
	V[0].Tex = VEC2(0.f, 0.f);
	V[1].Tex = VEC2(1.f, 0.f);
	V[2].Tex = VEC2(0.f, 1.f);
	V[3].Tex = VEC2(1.f, 1.f);

	HRESULT hr;

	frame::Dev->SetFVF(VERTEX_R2::FVF);
	if (FAILED(hr = frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(VERTEX_R2))))
		throw DirectXError(hr, "PpEffect::Redraw: DrawPrimitiveUP failed.", __FILE__, __LINE__);
	frame::RegisterDrawCall(2);

	frame::Dev->SetTexture(0, NULL);
}

void PpEffect::Redraw(IDirect3DSurface9 *Dst, IDirect3DTexture9 *Src, uint4 DstCX, uint4 DstCY)
{
	Redraw(Dst, Src, RECTF(0.f, 0.f, (float)DstCX, (float)DstCY));
}

void PpEffect::RedrawCube(IDirect3DCubeTexture9 *Src, const RECTF &DstRect)
{
	frame::RestoreDefaultState();

	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	frame::Dev->SetTexture(0, Src);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	frame::Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	frame::Dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	frame::Dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0));

	float ElementCX = DstRect.Width() / 4.f;
	float ElementCY = DstRect.Height() / 3.f;

	struct VERTEX { VEC4 Pos; VEC3 Tex; };

	RECTF Rect;
	VERTEX v[4];

	// positive y
	Rect.left = DstRect.left + ElementCX;
	Rect.right = Rect.left + ElementCX;
	Rect.top = DstRect.top;
	Rect.bottom = Rect.top + ElementCY;
	v[0].Pos = VEC4(Rect.left, Rect.top, 0.5f, 1.f);
	v[0].Tex = VEC3(-1.f,  1.f, -1.f);
	v[1].Pos = VEC4(Rect.right, Rect.top, 0.5f, 1.f);
	v[1].Tex = VEC3( 1.f,  1.f, -1.f);
	v[2].Pos = VEC4(Rect.left, Rect.bottom, 0.5f, 1.f);
	v[2].Tex = VEC3(-1.f,  1.f,  1.f);
	v[3].Pos = VEC4(Rect.right, Rect.bottom, 0.5f, 1.f);
	v[3].Tex = VEC3( 1.f,  1.f,  1.f);
	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VERTEX));
	frame::RegisterDrawCall(2);

	// negative x
	Rect.left = DstRect.left;
	Rect.right = Rect.left + ElementCX;
	Rect.top = DstRect.top + ElementCY;
	Rect.bottom = Rect.top + ElementCY;
	v[0].Pos = VEC4(Rect.left, Rect.top, 0.5f, 1.f);
	v[0].Tex = VEC3(-1.f,  1.f, -1.f);
	v[1].Pos = VEC4(Rect.right, Rect.top, 0.5f, 1.f);
	v[1].Tex = VEC3(-1.f,  1.f,  1.f);
	v[2].Pos = VEC4(Rect.left, Rect.bottom, 0.5f, 1.f);
	v[2].Tex = VEC3(-1.f, -1.f, -1.f);
	v[3].Pos = VEC4(Rect.right, Rect.bottom, 0.5f, 1.f);
	v[3].Tex = VEC3(-1.f, -1.f,  1.f);
	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VERTEX));
	frame::RegisterDrawCall(2);

	// positive z
	Rect.left = DstRect.left + ElementCX;
	Rect.right = Rect.left + ElementCX;
	Rect.top = DstRect.top + ElementCY;
	Rect.bottom = Rect.top + ElementCY;
	v[0].Pos = VEC4(Rect.left, Rect.top, 0.5f, 1.f);
	v[0].Tex = VEC3(-1.f,  1.f,  1.f);
	v[1].Pos = VEC4(Rect.right, Rect.top, 0.5f, 1.f);
	v[1].Tex = VEC3( 1.f,  1.f,  1.f);
	v[2].Pos = VEC4(Rect.left, Rect.bottom, 0.5f, 1.f);
	v[2].Tex = VEC3(-1.f, -1.f,  1.f);
	v[3].Pos = VEC4(Rect.right, Rect.bottom, 0.5f, 1.f);
	v[3].Tex = VEC3( 1.f, -1.f,  1.f);
	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VERTEX));
	frame::RegisterDrawCall(2);

	// positive x
	Rect.left = DstRect.left + ElementCX * 2.f;
	Rect.right = Rect.left + ElementCX;
	Rect.top = DstRect.top + ElementCY;
	Rect.bottom = Rect.top + ElementCY;
	v[0].Pos = VEC4(Rect.left, Rect.top, 0.5f, 1.f);
	v[0].Tex = VEC3( 1.f,  1.f,  1.f);
	v[1].Pos = VEC4(Rect.right, Rect.top, 0.5f, 1.f);
	v[1].Tex = VEC3( 1.f,  1.f, -1.f);
	v[2].Pos = VEC4(Rect.left, Rect.bottom, 0.5f, 1.f);
	v[2].Tex = VEC3( 1.f, -1.f,  1.f);
	v[3].Pos = VEC4(Rect.right, Rect.bottom, 0.5f, 1.f);
	v[3].Tex = VEC3( 1.f, -1.f, -1.f);
	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VERTEX));
	frame::RegisterDrawCall(2);

	// negtive z
	Rect.left = DstRect.left + ElementCX * 3.f;
	Rect.right = Rect.left + ElementCX;
	Rect.top = DstRect.top + ElementCY;
	Rect.bottom = Rect.top + ElementCY;
	v[0].Pos = VEC4(Rect.left, Rect.top, 0.5f, 1.f);
	v[0].Tex = VEC3( 1.f,  1.f, -1.f);
	v[1].Pos = VEC4(Rect.right, Rect.top, 0.5f, 1.f);
	v[1].Tex = VEC3(-1.f,  1.f, -1.f);
	v[2].Pos = VEC4(Rect.left, Rect.bottom, 0.5f, 1.f);
	v[2].Tex = VEC3( 1.f, -1.f, -1.f);
	v[3].Pos = VEC4(Rect.right, Rect.bottom, 0.5f, 1.f);
	v[3].Tex = VEC3(-1.f, -1.f, -1.f);
	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VERTEX));
	frame::RegisterDrawCall(2);

	// negative y
	Rect.left = DstRect.left + ElementCX;
	Rect.right = Rect.left + ElementCX;
	Rect.top = DstRect.top + ElementCY * 2.f;
	Rect.bottom = Rect.top + ElementCY;
	v[0].Pos = VEC4(Rect.left, Rect.top, 0.5f, 1.f);
	v[0].Tex = VEC3(-1.f, -1.f,  1.f);
	v[1].Pos = VEC4(Rect.right, Rect.top, 0.5f, 1.f);
	v[1].Tex = VEC3( 1.f, -1.f,  1.f);
	v[2].Pos = VEC4(Rect.left, Rect.bottom, 0.5f, 1.f);
	v[2].Tex = VEC3(-1.f, -1.f, -1.f);
	v[3].Pos = VEC4(Rect.right, Rect.bottom, 0.5f, 1.f);
	v[3].Tex = VEC3( 1.f, -1.f, -1.f);
	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VERTEX));
	frame::RegisterDrawCall(2);
}

void PpEffect::ClearAlpha(uint1 Alpha, uint4 ScreenCX, uint4 ScreenCY)
{
	frame::RestoreDefaultState();

	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_TEXTUREFACTOR, (uint4)Alpha << 24);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
	frame::Dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);
	frame::Dev->SetFVF(VERTEX_R::FVF);

	VERTEX_R V[4];
	V[0].Pos = VEC4(0.f,             0.f,             0.5f, 1.f);
	V[1].Pos = VEC4((float)ScreenCX, 0.f,             0.5f, 1.f);
	V[2].Pos = VEC4(0.f,             (float)ScreenCY, 0.5f, 1.f);
	V[3].Pos = VEC4((float)ScreenCX, (float)ScreenCY, 0.5f, 1.f);

	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(VERTEX_R));
	frame::RegisterDrawCall(2);

	// Na wszelki wypadek przywrócê
	frame::Dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PpColor

PpColor::PpColor() :
	m_Color(0x00000000u)
{
}

PpColor::PpColor(COLOR Color) :
	m_Color(Color)
{
}

void PpColor::Draw(uint4 RenderTargetCX, uint4 RenderTargetCY)
{
	if (m_Color.A == 0) return;

	frame::RestoreDefaultState();

	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	frame::Dev->SetRenderState(D3DRS_TEXTUREFACTOR, m_Color.ARGB);
	frame::Dev->SetTexture(0, NULL);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
	frame::Dev->SetFVF(VERTEX_R::FVF);

	// Fullscreen quad

	VERTEX_R V[4] = {
		VERTEX_R(VEC4(0.f,                   0.f,                   0.5f, 1.f)),
		VERTEX_R(VEC4((float)RenderTargetCX, 0.f,                   0.5f, 1.f)),
		VERTEX_R(VEC4(0.f,                   (float)RenderTargetCY, 0.5f, 1.f)),
		VERTEX_R(VEC4((float)RenderTargetCX, (float)RenderTargetCY, 0.5f, 1.f))
	};

	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(VERTEX_R));
	frame::RegisterDrawCall(2);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PpTexture

PpTexture::PpTexture() :
	m_Texture(NULL),
	m_Color(COLOR::WHITE),
	m_Matrix(MATRIX::IDENTITY)
{
}

PpTexture::PpTexture(const string &TextureName, COLOR Color) :
	m_TextureName(TextureName),
	m_Texture(NULL),
	m_Color(Color),
	m_Matrix(MATRIX::IDENTITY)
{
}

void PpTexture::Draw(uint4 RenderTargetCX, uint4 RenderTargetCY)
{
	if (m_Color.A == 0) return;
	if (m_TextureName.empty()) return;

	// Pobierz teksturê
	if (m_Texture == NULL)
		m_Texture = res::g_Manager->MustGetResourceEx<res::D3dTexture>(m_TextureName);
	m_Texture->Load();

	frame::RestoreDefaultState();

	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	frame::Dev->SetRenderState(D3DRS_TEXTUREFACTOR, m_Color.ARGB);
	frame::Dev->SetTexture(0, m_Texture->GetBaseTexture());
	frame::Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	frame::Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetFVF(VERTEX_R2::FVF);

	VERTEX_R2 V[4];
	V[0].Pos = VEC4(0.f,                   0.f,                   0.5f, 1.f);
	V[1].Pos = VEC4((float)RenderTargetCX, 0.f,                   0.5f, 1.f);
	V[2].Pos = VEC4(0.f,                   (float)RenderTargetCY, 0.5f, 1.f);
	V[3].Pos = VEC4((float)RenderTargetCX, (float)RenderTargetCY, 0.5f, 1.f);

	// Wylicz wspó³rzêdne tekstury
	Transform(&V[0].Tex, VEC2(0.f, 0.f), m_Matrix);
	Transform(&V[1].Tex, VEC2(1.f, 0.f), m_Matrix);
	Transform(&V[2].Tex, VEC2(0.f, 1.f), m_Matrix);
	Transform(&V[3].Tex, VEC2(1.f, 1.f), m_Matrix);

	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(VERTEX_R2));
	frame::RegisterDrawCall(2);
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PpFunction

PpFunction::PpFunction() :
	m_GrayscaleFactor(0.f),
	m_AFactor(VEC3::ONE),
	m_BFactor(VEC3::ZERO)
{
}

PpFunction::PpFunction(const VEC3 &AFactor, const VEC3 &BFactor, float GrayscaleFactor) :
	m_GrayscaleFactor(GrayscaleFactor),
	m_AFactor(AFactor),
	m_BFactor(BFactor)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PpToneMapping

const float TONE_MAPPING_BRIGHTNESS_SMOOTH_TIME = 0.5f;
// Co ile sekund testowaæ jasnoœæ, ¿eby nie robiæ tego co klatkê.
const float TONE_MAPPING_MEASUREMENT_TIME = 0.1f;
// Liczba sampli do próbkowania podczas pomiaru jasnoœci sceny
const uint4 TONE_MAPPING_SAMPLE_COUNT = 100;

float PpToneMapping::MeasureBrightness(res::D3dTextureSurface *RamTexture)
{
	// Za³o¿enie: Podana tekstura jest w formacie A8R8G8B8 lub X8R8G8B8

	// Ewentualna inicjalizacja macierzy wspó³rzêdnych próbkowania
	if (RamTexture->GetWidth() != m_SampleCX || RamTexture->GetHeight() != m_SampleCY)
	{
		m_SampleCX = RamTexture->GetWidth();
		m_SampleCY = RamTexture->GetHeight();

		uint4 cx1 = m_SampleCX - 1;
		uint4 cy1 = m_SampleCY - 1;

		for (size_t i = 0; i < TONE_MAPPING_SAMPLE_COUNT; i++)
		{
			m_Samples[i] = std::pair<uint4, uint4>(
				uint4(POISSON_DISC_2D[i].x * cx1),
				uint4(POISSON_DISC_2D[i].y * cy1)
			);
		}
	}

	D3DLOCKED_RECT lr;
	HRESULT hr = RamTexture->GetSurface()->LockRect(&lr, NULL, D3DLOCK_READONLY);
	if (FAILED(hr)) throw DirectXError(hr, "Nie mo¿na zablokowaæ powierzchni w pamiêci RAM (LockRect).", __FILE__, __LINE__);

	double Sum = 0.0;
	char *Data = (char*)lr.pBits;
	COLOR *ColorData, *Color;

	const double EPSILON = 0.01f;

	for (size_t i = 0; i < TONE_MAPPING_SAMPLE_COUNT; i++)
	{
		ColorData = (COLOR*)(Data + m_Samples[i].second * lr.Pitch);
		Color = &ColorData[m_Samples[i].first];
		// œrednia arytmetyczna
		//Sum += ColorToGrayscale(*Color);
		// œrednia logarytmiczna
		Sum += log(EPSILON + ColorToGrayscale(*Color));
	}

	RamTexture->GetSurface()->UnlockRect();

	// œrednia arytmetyczna
	//return (float)( Sum / TONE_MAPPING_SAMPLE_COUNT );
	// œrednia logarytmiczna
	return (float)( exp(Sum / TONE_MAPPING_SAMPLE_COUNT) );
}

PpToneMapping::PpToneMapping(float Intensity) :
	m_Intensity(Intensity),
	m_LastMeasurementTime(0.f),
	m_Brightness(0.5f, TONE_MAPPING_BRIGHTNESS_SMOOTH_TIME, 0.f),
	m_SampleCX(0), m_SampleCY(0)
{
	assert(TONE_MAPPING_SAMPLE_COUNT <= POISSON_DISC_2D_COUNT);
	m_Samples.resize(TONE_MAPPING_SAMPLE_COUNT);
}

void PpToneMapping::CalcBrightness(res::D3dTextureSurface *ScreenTexture, res::D3dTextureSurface *ToneMappingVramTexture, res::D3dTextureSurface *ToneMappingRamTexture)
{
	ERR_TRY;

	HRESULT hr;

	float Brightness;

	if (m_LastMeasurementTime + TONE_MAPPING_MEASUREMENT_TIME <= frame::Timer1.GetTime())
	{
		// Przerysuj tekstura -> ma³a tekstura VRAM
		// Metoda 1 - StretchRect
		RECT SrcRect = { 0, 0, ScreenTexture->GetWidth(), ScreenTexture->GetHeight() };
		RECT DstRect = { 0, 0, ToneMappingVramTexture->GetWidth(), ToneMappingVramTexture->GetHeight() };
		hr = frame::Dev->StretchRect(ScreenTexture->GetSurface(), &SrcRect, ToneMappingVramTexture->GetSurface(), &DstRect, D3DTEXF_POINT);
		if (FAILED(hr)) throw DirectXError(hr, "B³¹d podczas przepisywania z tekstury ScreenTexture do ToneMappingVramTexture (StretchRect).");
		ToneMappingVramTexture->SetFilled();
		// Metoda 2 - Redraw
		//Redraw(ToneMappingVramTexture->GetSurface(), ToneMappingVramTexture->GetWidth(), ToneMappingVramTexture->GetHeight(), ScreenTexture->GetTexture());

		// SprowadŸ j¹ do pamiêci - ma³a teksura VRAM -> ma³a tekstura RAM
		hr = frame::Dev->GetRenderTargetData(ToneMappingVramTexture->GetSurface(), ToneMappingRamTexture->GetSurface());
		// UWAGA! Bo to wysypywa³o mi program.
		if (hr != D3DERR_DEVICELOST)
		{
			if (FAILED(hr)) throw DirectXError(hr, "B³¹d podczas sprowadzania tekstury ToneMappingVramTexture do pamiêci RAM (GetRenderTargetData).", __FILE__, __LINE__);
			ToneMappingRamTexture->SetFilled();

			// Policz jasnoœæ
			Brightness = MeasureBrightness(ToneMappingRamTexture);

			m_LastMeasurementTime = frame::Timer1.GetTime();
		}
		else
			Brightness = m_Brightness.Dest;
	}
	else
		Brightness = m_Brightness.Dest;

	if (m_LastMeasurementTime == 0.f)
		m_Brightness.Set(Brightness, 0.f);
	else
		m_Brightness.Update(Brightness, frame::Timer1.GetDeltaTime());

	ERR_CATCH_FUNC;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PpBloom

enum BRIGHT_PASS_EFFECT_PARAMS
{
	BPEP_TEXTURE,
	BPEP_AFACTOR,
	BPEP_BFACTOR,
};

void PpBloom::BrightPass(IDirect3DSurface9 *Dst, uint4 DstCX, uint4 DstCY, IDirect3DTexture9 *Src)
{
	RenderTargetHelper rth(Dst, NULL);

	frame::RestoreDefaultState();

	if (m_BrightPassEffect == NULL)
		m_BrightPassEffect = res::g_Manager->MustGetResourceEx<res::D3dEffect>("PpBrightPass");
	m_BrightPassEffect->Load();
	ID3DXEffect *Effect = m_BrightPassEffect->Get();

	float AFactor = 0.f, BFactor = 0.f;
	if (m_Down != 1.f) // Bo jak 1 to dzielenie przez 0.
	{
		AFactor = 1.f / (1.f - m_Down);
		BFactor = - m_Down / (1.f - m_Down);
	}

	Effect->SetTexture(m_BrightPassEffect->GetParam(BPEP_TEXTURE), Src);
	Effect->SetFloat(m_BrightPassEffect->GetParam(BPEP_AFACTOR), AFactor);
	Effect->SetFloat(m_BrightPassEffect->GetParam(BPEP_BFACTOR), BFactor);

	UINT Foo;
	Effect->Begin(&Foo, D3DXFX_DONOTSAVESTATE);
	Effect->BeginPass(0);

	VERTEX_R2 V[4];
	V[0].Pos = VEC4(0.f,          0.f,          0.5f, 1.f);
	V[1].Pos = VEC4((float)DstCX, 0.f,          0.5f, 1.f);
	V[2].Pos = VEC4(0.f,          (float)DstCY, 0.5f, 1.f);
	V[3].Pos = VEC4((float)DstCX, (float)DstCY, 0.5f, 1.f);
	
	// Offset, ¿eby by³o pixel perfect!
	VEC4 Offset = VEC4(-0.5f, -0.5f, 0.f, 0.f);
	V[0].Pos += Offset; V[1].Pos += Offset;
	V[2].Pos += Offset; V[3].Pos += Offset;

	// Wspó³rzêdne tekstury
	V[0].Tex = VEC2(0.f, 0.f);
	V[1].Tex = VEC2(1.f, 0.f);
	V[2].Tex = VEC2(0.f, 1.f);
	V[3].Tex = VEC2(1.f, 1.f);

	HRESULT hr;

	D3DVIEWPORT9 Viewport = { 0, 0, DstCX, DstCY, 0.f, 1.f };
	if (FAILED(hr = frame::Dev->SetViewport(&Viewport)))
		throw DirectXError(hr, "PpEffect::BlurPass: SetViewport failed.", __FILE__, __LINE__);

	frame::Dev->SetFVF(VERTEX_R2::FVF);
	if (FAILED(hr = frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(VERTEX_R2))))
		throw DirectXError(hr, "PpBloom::BrightPass: DrawPrimitiveUP failed.", __FILE__, __LINE__);
	frame::RegisterDrawCall(2);

	Effect->EndPass();
	Effect->End();
}

PpBloom::PpBloom(float Intensity, float Down) :
	m_Down(Down),
	m_Intensity(Intensity),
	m_BrightPassEffect(NULL)
{
}

void PpBloom::CreateBloom(res::D3dTextureSurface *ScreenTexture, res::D3dTextureSurface *BlurTexture1, res::D3dTextureSurface *BlurTexture2)
{
	// BrightPass - ScreenTexture -> BlurTexture1
	BrightPass(BlurTexture1->GetSurface(), BlurTexture1->GetWidth(), BlurTexture1->GetHeight(), ScreenTexture->GetTexture());
	// BlurPass 1 - BlurTexture1 -> BlurTexture2
	BlurPass(1.5f, BlurTexture2->GetSurface(), BlurTexture1->GetTexture(), BlurTexture1->GetWidth(), BlurTexture1->GetHeight());
	// BlurPass 2 - BlurTexture2 -> BlurTexture1
	BlurPass(2.5f, BlurTexture1->GetSurface(), BlurTexture2->GetTexture(), BlurTexture1->GetWidth(), BlurTexture1->GetHeight());

	BlurTexture1->SetFilled();
	BlurTexture2->SetFilled();
}

void PpBloom::DrawBloom(res::D3dTextureSurface *BlurTexture1, uint4 ScreenCX, uint4 ScreenCY)
{
	frame::RestoreDefaultState();

	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); // Addytywnie
	frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	frame::Dev->SetTexture(0, BlurTexture1->GetTexture());
	frame::Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	frame::Dev->SetRenderState(D3DRS_TEXTUREFACTOR, (uint4)(m_Intensity*255.f) << 24);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	VERTEX_R2 V[4];
	V[0].Pos = VEC4(0.f,             0.f,             0.5f, 1.f);
	V[1].Pos = VEC4((float)ScreenCX, 0.f,             0.5f, 1.f);
	V[2].Pos = VEC4(0.f,             (float)ScreenCY, 0.5f, 1.f);
	V[3].Pos = VEC4((float)ScreenCX, (float)ScreenCY, 0.5f, 1.f);

	// Offset, ¿eby by³o pixel perfect!
	VEC4 Offset = VEC4(-0.5f, -0.5f, 0.f, 0.f);
	V[0].Pos += Offset; V[1].Pos += Offset;
	V[2].Pos += Offset; V[3].Pos += Offset;

	// Wspó³rzêdne tekstury
	V[0].Tex = VEC2(0.f, 0.f);
	V[1].Tex = VEC2(1.f, 0.f);
	V[2].Tex = VEC2(0.f, 1.f);
	V[3].Tex = VEC2(1.f, 1.f);

	frame::Dev->SetFVF(VERTEX_R2::FVF);
	HRESULT hr;
	if (FAILED(hr = frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(VERTEX_R2))))
		throw DirectXError(hr, "PpBloom::DrawBloom: DrawPrimitiveUP Failed.", __FILE__, __LINE__);
	frame::RegisterDrawCall(2);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PpFeedback

float PpFeedback::CalcDelay()
{
	if (m_Mode == M_DRUNK)
		return m_Intensity * 0.05f;
	else // m_Mode == M_MOTION_BLUR
		return 0.f;
}

void PpFeedback::CalcTextureMatrix(MATRIX *OutTextureMatrix, const VEC3 &EyePos, const MATRIX &CameraViewProj)
{
	// Radial blur
	//const float SCALE = 0.95f;
	//MATRIX m1, m2, m3, m4;
	//Translation(&m1, -0.5f, -0.5f, 0.f);
	//Scaling(&m2, SCALE, SCALE, 1.f);
	//Translation(&m3, 0.5f, 0.5f, 0.f);
	//MATRIX TextureMatrix = m1 * m2 * m3;

	if (m_Mode == M_DRUNK)
		Identity(OutTextureMatrix);
	else // m_Mode == M_MOTION_BLUR
	{
		if (!m_LastCameraPosInited)
		{
			m_LastCameraPos = EyePos;
			m_LastCameraPosInited = true;
		}
		VEC3 PlayerDir = (EyePos - m_LastCameraPos) / frame::Timer1.GetDeltaTime(); // Bo prêdkoœæ = Droga / Czas
		m_LastCameraPos = EyePos;
		// - Prêdkoœæ
		float Vel = Length(PlayerDir);
		// - Kierunek ruchu kamery w przestrzeni rzutowania
		VEC3 PlayerDirProj; TransformNormal(&PlayerDirProj, PlayerDir, CameraViewProj);
		// - parametry blura
		//const float SCALE = 1.f;
		const float SCALE = 1.f + (PlayerDirProj.z*0.002f*m_Intensity);
		MATRIX m1, m2, m3, m4;
		Translation(&m1, -0.5f, -0.5f, 0.f);
		Scaling(&m2, SCALE, SCALE, 1.f);
		//Translation(&m3, 0.5f, 0.5f, 0.f);
		Translation(&m3, 0.5f - PlayerDirProj.x*0.0005f*m_Intensity, 0.5f + PlayerDirProj.y*0.0005f*m_Intensity, 0.f);
		*OutTextureMatrix = m1 * m2 * m3;
	}
}

void PpFeedback::DrawFeedback(
	res::D3dTextureSurface *FeedbackTexture, uint4 ScreenCX, uint4 ScreenCY,
	const VEC3 &EyePos, const MATRIX &CameraViewProj)
{
	// Wartoœæ alfa tego koloru to parametr efektu, ale ustali³em ¿e taki jest najlepszy OK do wszystkiego.
	// Kolor zmieniaæ nie ma sensu - brzydko wychodzi.
	const COLOR Color = COLOR(0xA0FFFFFF);

	MATRIX TextureMatrix;
	CalcTextureMatrix(&TextureMatrix, EyePos, CameraViewProj);

	frame::RestoreDefaultState();

	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	frame::Dev->SetRenderState(D3DRS_TEXTUREFACTOR, Color.ARGB);
	frame::Dev->SetTexture(0, FeedbackTexture->GetTexture());
	frame::Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	frame::Dev->SetFVF(VERTEX_R2::FVF);

	VERTEX_R2 V[4];
	V[0].Pos = VEC4(0.f,             0.f,             0.5f, 1.f);
	V[1].Pos = VEC4((float)ScreenCX, 0.f,             0.5f, 1.f);
	V[2].Pos = VEC4(0.f,             (float)ScreenCY, 0.5f, 1.f);
	V[3].Pos = VEC4((float)ScreenCX, (float)ScreenCY, 0.5f, 1.f);

	// Wylicz wspó³rzêdne tekstury
	Transform(&V[0].Tex, VEC2(0.f, 0.f), TextureMatrix);
	Transform(&V[1].Tex, VEC2(1.f, 0.f), TextureMatrix);
	Transform(&V[2].Tex, VEC2(0.f, 1.f), TextureMatrix);
	Transform(&V[3].Tex, VEC2(1.f, 1.f), TextureMatrix);

	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(VERTEX_R2));
	frame::RegisterDrawCall(2);
}

PpFeedback::PpFeedback(MODE Mode, float Intensity) :
	m_Mode(Mode),
	m_Intensity(Intensity),
	m_LastWriteTime(0.f),
	m_LastCameraPosInited(false),
	m_FeedbackTexture(NULL)
{
}

PpFeedback::~PpFeedback()
{
	/*
	To jest potrzebne, poniewa¿ bez tego wystêpuje nieprzyjemny b³¹d:
	Gra tworzy efekt PpFeedback, u¿ywa go, potem zwalnia.
	Nastêpnie po jakimœ czasie znów tworzy efekt PpFeedback.
	U¿ywana do niego tekstura nie zd¹¿y³a zostaæ zwolniona.
	Gdyby mia³a status Filled, zosta³aby pokazana przed ponownym wype³nieniem i
	przez jedn¹ klatkê mign¹³by nieaktualny obraz pochodz¹cy z ostatniego u¿ycia poprzedniego efektu.
	*/
	if (m_FeedbackTexture != NULL)
		m_FeedbackTexture->ResetFilled();
}

void PpFeedback::Draw(
	IDirect3DSurface9 *BackBufferSurface, uint4 ScreenCX, uint4 ScreenCY,
	res::D3dTextureSurface *FeedbackTexture,
	const VEC3 &EyePos, const MATRIX &CameraViewProj)

{
	m_FeedbackTexture = FeedbackTexture;

	ERR_TRY

	if (FeedbackTexture->IsFilled())
		DrawFeedback(FeedbackTexture, ScreenCX, ScreenCY, EyePos, CameraViewProj);
	
	// Przepisz obraz z ekranu do tekstury feedback
	if (m_LastWriteTime + CalcDelay() < frame::Timer1.GetTime())
	{
		RECT SrcRect = { 0, 0, ScreenCX, ScreenCY };
		RECT DstRect = { 0, 0, FeedbackTexture->GetWidth(), FeedbackTexture->GetHeight() };
		HRESULT hr = frame::Dev->StretchRect(BackBufferSurface, &SrcRect, FeedbackTexture->GetSurface(), &DstRect, D3DTEXF_LINEAR);
		if (FAILED(hr)) throw DirectXError(hr, "Nie mo¿na skopiowaæ ekranu do tekstury Feedback (StretchRect).");
		FeedbackTexture->SetFilled();

		m_LastWriteTime = frame::Timer1.GetTime();
	}

	ERR_CATCH_FUNC
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PpLensFlare

const char * const LENS_FLARE_TEXTURE_NAMES[4] = {
	"LensFlare01",
	"LensFlare02",
	"LensFlare03",
	"LensFlare_Halo",
};

// Liczba soczewek
const uint4 LENS_FLARE_COUNT = 7;
// Indeksy tekstur soczewek (3 to odblask s³oñca)
const uint4 LENS_FLARE_TEXTURE_INDICES[LENS_FLARE_COUNT] = {
	3, 1, 2, 0, 2, 1, 2
};
// Pozycje soczewek na osi (-1..1)
const float LENS_FLARE_POSITIONS[LENS_FLARE_COUNT] = {
	1.0f, 0.65f, 0.5f, 0.2f, -0.3f, -0.6f, -0.9f
};
// Promienie soczewek
const float LENS_FLARE_HALFSIZES[LENS_FLARE_COUNT] = {
	0.5f, 0.05f, 0.05f, 0.07f, 0.07f, 0.04f, 0.1f
};
// Kolory soczewek, kana³ alfa musz¹ mieæ 0.
const uint4 LENS_FLARE_COLORS[LENS_FLARE_COUNT] = {
	0x00FFFFFF, 0x00ff9900, 0x00FFFFFF, 0x00FFA080, 0x00ffd500, 0x00FFFFFF, 0x00FFFF80
};
// Granica dla trapezoidu wyznaczaj¹cego intensywnoœæ flary od pozycji na ekranie wzglêdem wsp. rzutowania -1..1
const float LENS_FLARE_TRAPEZOID_OUTER = 1.3f;
const float LENS_FLARE_TRAPEZOID_INNER = 0.8f;
// Wyg³adzanie funkcj¹ SmoothCD zmian intensywnoœci flary w czasie
const float LENS_FLARE_INTENSITY_SMOOTH_TIME = 0.07f;


void PpLensFlare::DrawLensFlare(const VEC2 &Pos, float Intensity, uint4 ScreenCX, uint4 ScreenCY)
{
	VEC2 Pos_p = VEC2(
		( Pos.x * 0.5f + 0.5f) * ScreenCX,
		(-Pos.y * 0.5f + 0.5f) * ScreenCY);
	VEC2 Center = VEC2(ScreenCX * 0.5f, ScreenCY * 0.5f);
	VEC2 VecToSun = Pos_p - Center;

	frame::RestoreDefaultState();

	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA); // Addytywnie
	frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	frame::Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetFVF(VERTEX_R2::FVF);

	VERTEX_R2 V[4] = {
		VERTEX_R2(VEC4(0.f, 0.f, 0.5f, 1.f), VEC2(0.f, 0.f)),
		VERTEX_R2(VEC4(0.f, 0.f, 0.5f, 1.f), VEC2(1.f, 0.f)),
		VERTEX_R2(VEC4(0.f, 0.f, 0.5f, 1.f), VEC2(0.f, 1.f)),
		VERTEX_R2(VEC4(0.f, 0.f, 0.5f, 1.f), VEC2(1.f, 1.f))
	};

	VEC2 CurrentPos;
	float CurrentSize;
	uint4 ColorAlpha = (uint4)(Intensity*255.f) << 24;

	for (size_t i = 0; i < LENS_FLARE_COUNT; i++)
	{
		// Halo rysowane z pe³n¹ intensywnoœci¹, wszystkie nastêpne z dwa razy mniejsz¹.
		if (i == 1)
			ColorAlpha = (uint4)(Intensity*128.f) << 24;

		CurrentPos = Center + VecToSun * LENS_FLARE_POSITIONS[i];
		CurrentSize = LENS_FLARE_HALFSIZES[i] * ScreenCY;
		if (i == 0)
			CurrentSize *= Intensity;
		V[0].Pos.x = CurrentPos.x - CurrentSize; V[0].Pos.y = CurrentPos.y - CurrentSize;
		V[1].Pos.x = CurrentPos.x + CurrentSize; V[1].Pos.y = CurrentPos.y - CurrentSize;
		V[2].Pos.x = CurrentPos.x - CurrentSize; V[2].Pos.y = CurrentPos.y + CurrentSize;
		V[3].Pos.x = CurrentPos.x + CurrentSize; V[3].Pos.y = CurrentPos.y + CurrentSize;

		frame::Dev->SetTexture(0, m_Textures[LENS_FLARE_TEXTURE_INDICES[i]]->GetTexture());
		frame::Dev->SetRenderState(D3DRS_TEXTUREFACTOR, LENS_FLARE_COLORS[i] | ColorAlpha);

		frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(VERTEX_R2));
		frame::RegisterDrawCall(2);
	}
}

PpLensFlare::PpLensFlare(const VEC3 &DirToLight) :
	m_DirToLight(DirToLight),
	m_Intensity(0.f, LENS_FLARE_INTENSITY_SMOOTH_TIME, 0.f),
	m_CustomIntensity(1.0f)
{
	Normalize(&m_DirToLight);

	for (size_t i = 0; i < 4; i++)
		m_Textures[i] = NULL;

	for (size_t i = 0; i < 4; i++)
	{
		m_Textures[i] = res::g_Manager->MustGetResourceEx<res::D3dTexture>(LENS_FLARE_TEXTURE_NAMES[i]);
		m_Textures[i]->Lock();
	}
}

PpLensFlare::~PpLensFlare()
{
	for (size_t i = 4; i--; )
	{
		m_Textures[i]->Unlock();
	}
}

void PpLensFlare::SetDirToLight(const VEC3 &DirToLight)
{
	m_DirToLight = DirToLight;
	Normalize(&m_DirToLight);
}

void PpLensFlare::Draw(float VisibleFactor, uint4 ScreenCX, uint4 ScreenCY, const MATRIX &CamView, const MATRIX &CamProj)
{
	ERR_TRY

	bool NeedDraw = false;
	float CurrentIntensity = 0.f;
	VEC2 Pos;
	VEC3 DirToLight_cam;

	TransformNormal(&DirToLight_cam, m_DirToLight, CamView);
	// Jeœli s³oñce nie jest z ty³u
	if (DirToLight_cam.z > 0.f)
	{
		VEC3 DirToLight_proj;
		TransformCoord(&DirToLight_proj, DirToLight_cam, CamProj);
		Pos = VEC2(DirToLight_proj.x, DirToLight_proj.y);
		NeedDraw = true;

		float IntensityForX = Trapezoidal(DirToLight_proj.x, -LENS_FLARE_TRAPEZOID_OUTER, -LENS_FLARE_TRAPEZOID_INNER, +LENS_FLARE_TRAPEZOID_INNER, +LENS_FLARE_TRAPEZOID_OUTER);
		float IntensityForY = Trapezoidal(DirToLight_proj.y, -LENS_FLARE_TRAPEZOID_OUTER, -LENS_FLARE_TRAPEZOID_INNER, +LENS_FLARE_TRAPEZOID_INNER, +LENS_FLARE_TRAPEZOID_OUTER);
		if (IntensityForX > 0.f && IntensityForY > 0.f)
			CurrentIntensity = IntensityForX * IntensityForY * m_CustomIntensity * VisibleFactor;
	}

	m_Intensity.Update(CurrentIntensity, frame::Timer1.GetDeltaTime());

	if (NeedDraw && m_Intensity.Pos > 0.01f)
		DrawLensFlare(Pos, m_Intensity.Pos, ScreenCX, ScreenCY);

	ERR_CATCH_FUNC
}

// Usuniête... :)

} // namespace engine
