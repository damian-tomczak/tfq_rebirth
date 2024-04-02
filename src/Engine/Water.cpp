/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\Res_d3d.hpp"
#include "..\Framework\Multishader.hpp"
#include "Engine.hpp"
#include "Camera.hpp"
#include "Terrain.hpp"
#include "Water.hpp"

namespace engine
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy globalne

const DWORD WATER_VERTEX_FVF = D3DFVF_XYZ;

enum WATER_SHADER_PARAMS
{
	WSP_WORLD_VIEW_PROJ = 0,
	WSP_NORMAL_TEXTURE,
	WSP_CAMERA_POS,
	WSP_DIR_TO_LIGHT,
	WSP_LIGHT_COLOR,
	WSP_CAUSTICS_TEXTURE,
	WSP_FOG_COLOR,
	WSP_FOG_FACTORS,
	WSP_SPECULAR_EXPONENT,
	WSP_WATER_COLOR,
	WSP_CAUSTICS_COLOR,
	WSP_HORIZON_COLOR,
	WSP_ZENITH_COLOR,
	WSP_TEX_OFFSETS,
	WSP_TEX_SCALES,
};

const float SPECULAR_EXPONENT = 64.0f;

const float DEFAULT_DATETIME = 0.5f;
const float DEFAULT_NORMAL_TEX_SCALE = 1.0f / 5.0f;
const float DEFAULT_CAUSTICS_TEX_SCALE = 1.0f / 2.0f;
const VEC3 DEFAULT_DIR_TO_LIGHT = VEC3(-0.8164f, 0.4082f, 0.4082f);
const COLORF DEFAULT_LIGHT_COLOR = COLORF::WHITE;
const COLORF DEFAULT_CAUSTICS_COLOR = COLORF(1.0f, 0.1f, 0.1f, 0.1f);
const COLORF DEFAULT_WATER_COLOR = COLORF(0.7f, 0.2f, 0.3f, 0.1f);
const COLORF DEFAULT_HORIZON_COLOR = COLORF(0.48f, 0.55f, 0.55f);
const COLORF DEFAULT_ZENITH_COLOR = COLORF(0.13f, 0.28f, 0.30f);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy WaterBase_pimpl

class WaterBase_pimpl
{
public:
	Scene *m_OwnerScene;
	VEC2 m_TexOffset1, m_TexOffset2;

	// Parametry wygl¹du wody
	float m_DateTime;
	float m_NormalTexScale;
	float m_CausticsTexScale;
	VEC3 m_DirToLight;
	COLORF m_LightColor;
	COLORF m_CausticsColor;
	RoundInterpolator<COLORF> m_WaterColor;
	RoundInterpolator<COLORF_PAIR> m_SkyColors;

	// Dane pobierane przy pierwszym u¿yciu
	static res::Multishader *m_Multishader;
	static res::D3dTexture *m_WaterNormalTexture;
	static res::D3dTexture *m_CausticsTexture;

	// U¿ywane miêdzy BeginWaterDraw i EndWaterDraw
	ID3DXEffect *m_StartedEffect;

	WaterBase_pimpl();
	// CameraPos - we wspó³rzêdnych lokalnych modelu
	void BeginWaterDraw(const VEC3 &CameraPos, float CameraZFar, const MATRIX &WorldViewProj);
	void EndWaterDraw();
};

res::Multishader *WaterBase_pimpl::m_Multishader = NULL;
res::D3dTexture *WaterBase_pimpl::m_WaterNormalTexture = NULL;
res::D3dTexture *WaterBase_pimpl::m_CausticsTexture = NULL;

WaterBase_pimpl::WaterBase_pimpl() :
	m_StartedEffect(NULL),
	m_TexOffset1(VEC2::ZERO),
	m_TexOffset2(VEC2::ZERO)
{
}

void WaterBase_pimpl::BeginWaterDraw(const VEC3 &CameraPos, float CameraZFar, const MATRIX &WorldViewProj)
{
	bool FogEnabled = m_OwnerScene->GetFogEnabled();
	VEC2 SceneWindVec = m_OwnerScene->GetWindVec();
	float SceneTime = frac(m_DateTime);
	float dt = frame::Timer2.GetDeltaTime();

	if (m_Multishader == NULL)
		m_Multishader = res::g_Manager->MustGetResourceEx<res::Multishader>("WaterShader");
	m_Multishader->Load();
	uint ShaderMacros[1];
	ShaderMacros[0] = FogEnabled ? 1 : 0;
	const res::Multishader::SHADER_INFO & ShaderInfo = m_Multishader->GetShader(ShaderMacros);

	if (m_WaterNormalTexture == NULL)
		m_WaterNormalTexture = res::g_Manager->MustGetResourceEx<res::D3dTexture>("Water_n");
	m_WaterNormalTexture->Load();

	if (m_CausticsTexture == NULL)
		m_CausticsTexture = res::g_Manager->MustGetResourceEx<res::D3dTexture>("Water_Caustics");
	m_CausticsTexture->Load();

	m_TexOffset1 += dt * VEC2(SceneWindVec.x * 0.10f, SceneWindVec.y * -0.09f);
	m_TexOffset2 += dt * VEC2(SceneWindVec.y * -0.08f, SceneWindVec.x * -0.12f);

	COLORF WaterColor;
	COLORF_PAIR SkyColor;
	m_WaterColor.Calc(&WaterColor, SceneTime);
	m_SkyColors.Calc(&SkyColor, SceneTime);

	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetMatrix(ShaderInfo.Params[WSP_WORLD_VIEW_PROJ], (const D3DXMATRIX*)&(WorldViewProj)) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetTexture(ShaderInfo.Params[WSP_NORMAL_TEXTURE], m_WaterNormalTexture->GetTexture()) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_CAMERA_POS], &D3DXVECTOR4(CameraPos.x, CameraPos.y, CameraPos.z, 1.0f)) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_DIR_TO_LIGHT], &D3DXVECTOR4(m_DirToLight.x, m_DirToLight.y, m_DirToLight.z, 0.0f)) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_LIGHT_COLOR], &D3DXVECTOR4(m_LightColor.R, m_LightColor.G, m_LightColor.B, m_LightColor.A)) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetTexture(ShaderInfo.Params[WSP_CAUSTICS_TEXTURE], m_CausticsTexture->GetTexture()) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetFloat(ShaderInfo.Params[WSP_SPECULAR_EXPONENT], SPECULAR_EXPONENT) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_WATER_COLOR], &D3DXVECTOR4(WaterColor.R, WaterColor.G, WaterColor.B, WaterColor.A)) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_CAUSTICS_COLOR], &D3DXVECTOR4(m_CausticsColor.R, m_CausticsColor.G, m_CausticsColor.B, m_CausticsColor.A)) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_HORIZON_COLOR], &D3DXVECTOR4(SkyColor.first.R, SkyColor.first.G, SkyColor.first.B, SkyColor.first.A)) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_ZENITH_COLOR], &D3DXVECTOR4(SkyColor.second.R, SkyColor.second.G, SkyColor.second.B, SkyColor.second.A)) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_TEX_OFFSETS], &D3DXVECTOR4(m_TexOffset1.x, m_TexOffset1.y, m_TexOffset2.x, m_TexOffset2.y)) );
	ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_TEX_SCALES], &D3DXVECTOR4(m_NormalTexScale, m_CausticsTexScale, 0.0f, 0.0f)) );

	if (FogEnabled)
	{
		float One_Minus_FogStart = 1.f - m_OwnerScene->GetFogStart();
		float FogA = 1.f / (CameraZFar * One_Minus_FogStart);
		float FogB = - m_OwnerScene->GetFogStart() / One_Minus_FogStart;
		const COLORF & FogColor = m_OwnerScene->GetFogColor();
		ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_FOG_COLOR], &D3DXVECTOR4(FogColor.R, FogColor.G, FogColor.B, FogColor.A)) );
		ERR_GUARD_DIRECTX_D( ShaderInfo.Effect->SetVector(ShaderInfo.Params[WSP_FOG_FACTORS], &D3DXVECTOR4(FogA, FogB, 0.f, 0.f)) );
	}

	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	frame::Dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	frame::Dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE); // ???
	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	UINT FooPasses;
	ShaderInfo.Effect->Begin(&FooPasses, D3DXFX_DONOTSAVESTATE);
	ShaderInfo.Effect->BeginPass(0);

	assert(m_StartedEffect == NULL);
	m_StartedEffect = ShaderInfo.Effect;
}

void WaterBase_pimpl::EndWaterDraw()
{
	assert(m_StartedEffect != NULL);

	m_StartedEffect->EndPass();
	m_StartedEffect->End();

	m_StartedEffect = NULL;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy WaterBase

WaterBase::WaterBase(Scene *a_Scene) :
	base_pimpl(new WaterBase_pimpl)
{
	base_pimpl->m_OwnerScene = a_Scene;

	// Domyœlne wartoœci parametrów
	base_pimpl->m_DateTime = DEFAULT_DATETIME;
	base_pimpl->m_NormalTexScale = DEFAULT_NORMAL_TEX_SCALE;
	base_pimpl->m_CausticsTexScale = DEFAULT_CAUSTICS_TEX_SCALE;
	base_pimpl->m_DirToLight = DEFAULT_DIR_TO_LIGHT;
	base_pimpl->m_LightColor = DEFAULT_LIGHT_COLOR;
	base_pimpl->m_CausticsColor = DEFAULT_CAUSTICS_COLOR;
	base_pimpl->m_WaterColor.Items.push_back(RoundInterpolator<COLORF>::ITEM(0.5f, DEFAULT_WATER_COLOR));
	base_pimpl->m_SkyColors.Items.push_back(RoundInterpolator<COLORF_PAIR>::ITEM(0.5f, COLORF_PAIR(
		DEFAULT_HORIZON_COLOR,
		DEFAULT_ZENITH_COLOR)));
}

float WaterBase::GetDateTime()
{
	return base_pimpl->m_DateTime;
}

float WaterBase::GetNormalTexScale()
{
	return base_pimpl->m_NormalTexScale;
}

float WaterBase::GetCausticsTexScale()
{
	return base_pimpl->m_CausticsTexScale;
}

const VEC3 & WaterBase::GetDirToLight()
{
	return base_pimpl->m_DirToLight;
}

const COLORF & WaterBase::GetLightColor()
{
	return base_pimpl->m_LightColor;
}

const COLORF & WaterBase::GetCausticsColor()
{
	return base_pimpl->m_CausticsColor;
}

void WaterBase::SetDateTime(float DateTime)
{
	base_pimpl->m_DateTime = DateTime;
}

void WaterBase::SetNormalTexScale(float NormalTexScale)
{
	base_pimpl->m_NormalTexScale = NormalTexScale;
}

void WaterBase::SetCausticsTexScale(float CausticsTexScale)
{
	base_pimpl->m_CausticsTexScale = CausticsTexScale;
}

void WaterBase::SetDirToLight(const VEC3 &DirToLight)
{
	base_pimpl->m_DirToLight = DirToLight;
}

void WaterBase::SetLightColor(const COLORF &LightColor)
{
	base_pimpl->m_LightColor = LightColor;
}

void WaterBase::SetCausticsColor(const COLORF &CausticsColor)
{
	base_pimpl->m_CausticsColor = CausticsColor;
}

RoundInterpolator<COLORF> & WaterBase::AccessWaterColor()
{
	return base_pimpl->m_WaterColor;
}

RoundInterpolator<COLORF_PAIR> & WaterBase::AccessSkyColors()
{
	return base_pimpl->m_SkyColors;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa WaterEntity_pimpl

class WaterEntity_pimpl
{
public:
	enum MODE
	{
		MODE_QUAD,
		MODE_POLYGON,
	};

	MODE m_Mode;
	float m_QuadHalfCX, m_QuadHalfCZ;
	// Jeœli to wielok¹t, punkty s¹ wpisane bezpoœrednio tu
	// Jeœli to quad, tutaj przechowywane s¹ punkty wygenerowane z m_QuadHalfCX, m_QuadHalfCZ
	std::vector<VEC3> m_Points;
	std::vector<uint2> m_Indices;

	void BuildQuadPoints();
	void UpdateRadius(Entity *e);
};

void WaterEntity_pimpl::BuildQuadPoints()
{
	assert(m_Mode == MODE_QUAD);
	m_Points.clear(); // na wszelki wypadek

	m_Points.push_back(VEC3(- m_QuadHalfCX, 0.0f, - m_QuadHalfCZ));
	m_Points.push_back(VEC3(- m_QuadHalfCX, 0.0f,   m_QuadHalfCZ));
	m_Points.push_back(VEC3(  m_QuadHalfCX, 0.0f,   m_QuadHalfCZ));
	m_Points.push_back(VEC3(  m_QuadHalfCX, 0.0f, - m_QuadHalfCZ));
}

void WaterEntity_pimpl::UpdateRadius(Entity *e)
{
	float NewRadius;

	if (m_Mode == MODE_QUAD)
		NewRadius = Length(VEC2(m_QuadHalfCX, m_QuadHalfCZ));
	else // m_Mode == MODE_POLYGON
	{
		if (m_Points.size() == 0)
			NewRadius = 0.0f;
		else
		{
			float lsq = LengthSq(m_Points[0]);
			for (uint i = 1; i < m_Points.size(); i++)
				lsq = std::max(lsq, LengthSq(m_Points[i]));
			NewRadius = sqrtf(lsq);
		}
	}

	// Zmieñ tylko jeœli trzeba zwiêkszyæ albo jeœli trzeba zmniejszyæ co najmniej dwa razy.
	if (e->GetRadius() < NewRadius || e->GetRadius() > NewRadius * 2.0f)
		e->SetRadius(NewRadius);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa WaterEntity

WaterEntity::WaterEntity(Scene *OwnerScene, float QuadHalfCX, float QuadHalfCZ) :
	WaterBase(OwnerScene),
	CustomEntity(OwnerScene),
	pimpl(new WaterEntity_pimpl)
{
	pimpl->m_Mode = WaterEntity_pimpl::MODE_QUAD;
	pimpl->m_QuadHalfCX = QuadHalfCX;
	pimpl->m_QuadHalfCZ = QuadHalfCZ;
	pimpl->BuildQuadPoints();
	pimpl->UpdateRadius(this);
}

WaterEntity::WaterEntity(Scene *OwnerScene, const VEC3 PolygonPoints[], uint PolygonPointCount) :
	WaterBase(OwnerScene),
	CustomEntity(OwnerScene),
	pimpl(new WaterEntity_pimpl)
{
	pimpl->m_Mode = WaterEntity_pimpl::MODE_POLYGON;
	for (uint i = 0; i < PolygonPointCount; i++)
		pimpl->m_Points.push_back(PolygonPoints[i]);
	pimpl->UpdateRadius(this);
}

WaterEntity::~WaterEntity()
{
}

void WaterEntity::SetQuad(float QuadHalfCX, float QuadHalfCZ)
{
	pimpl->m_Mode = WaterEntity_pimpl::MODE_QUAD;
	pimpl->m_QuadHalfCX = QuadHalfCX;
	pimpl->m_QuadHalfCZ = QuadHalfCZ;
	pimpl->m_Points.clear();
	pimpl->BuildQuadPoints();
	pimpl->UpdateRadius(this);
}

void WaterEntity::SetPolygon(const VEC3 PolygonPoints[], uint PolygonPointCount)
{
	pimpl->m_Mode = WaterEntity_pimpl::MODE_POLYGON;
	pimpl->m_Points.clear();
	for (uint i = 0; i < PolygonPointCount; i++)
		pimpl->m_Points.push_back(PolygonPoints[i]);
	pimpl->UpdateRadius(this);
}

void WaterEntity::Draw(const ParamsCamera &Cam)
{
	VEC3 CameraPos;
	Transform(&CameraPos, Cam.GetEyePos(), GetInvWorldMatrix());

	base_pimpl->BeginWaterDraw(CameraPos, Cam.GetZFar(), GetWorldMatrix()*Cam.GetMatrices().GetViewProj());

	uint PrimitiveCount = pimpl->m_Points.size() - 2;
	frame::Dev->SetFVF(WATER_VERTEX_FVF);
	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, PrimitiveCount, &pimpl->m_Points[0], sizeof(VEC3));
	frame::RegisterDrawCall(PrimitiveCount);

	base_pimpl->EndWaterDraw();
}

bool WaterEntity::RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT)
{
	if (Type == COLLISION_OPTICAL)
		return false;

	PLANE Plane;
	float FooVD;

	if (pimpl->m_Mode == WaterEntity_pimpl::MODE_QUAD)
	{
		PointNormalToPlane(&Plane, VEC3::ZERO, VEC3::POSITIVE_Y);
		if (RayToPlane(RayOrig, RayDir, Plane, OutT, &FooVD))
		{
			VEC3 CollisionPoint = RayOrig + RayDir * (*OutT);
			if (CollisionPoint.x >= - pimpl->m_QuadHalfCX &&
				CollisionPoint.z >= - pimpl->m_QuadHalfCZ &&
				CollisionPoint.x <=   pimpl->m_QuadHalfCX &&
				CollisionPoint.z <=   pimpl->m_QuadHalfCZ)
			{
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	else // pimpl->m_Mode == WaterEntity_pimpl::MODE_POLYGON
		return RayToPolygon(RayOrig, RayDir, &pimpl->m_Points[0], pimpl->m_Points.size(), false, OutT);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TerrainWater_pimpl

class TerrainWater_pimpl
{
public:
	// Bufory s¹ NULL, jeœli w ogóle nie ma wody
	scoped_ptr<res::D3dVertexBuffer> m_VB;
	scoped_ptr<res::D3dIndexBuffer> m_IB;
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TerrainWater

TerrainWater::TerrainWater(Scene *OwnerScene, Terrain *TerrainObj, float Height) :
	WaterBase(OwnerScene),
	pimpl(new TerrainWater_pimpl)
{
	ERR_TRY;

	std::vector<VEC3> Vertices;
	std::vector<uint2> Indices;

	// Utwórz geometriê quadów wody dla tych sektorów terenu, w których choæ trochê jest zanurzone poni¿ej poziomu wody
	uint TerrainPatchCount = TerrainObj->GetPatchCount();
	BOX TerrainPatchBoundingBox;
	for (uint pi = 0; pi < TerrainPatchCount; pi++)
	{
		TerrainObj->CalcPatchBoundingBox(&TerrainPatchBoundingBox, pi);
		if (TerrainPatchBoundingBox.p1.y < Height)
		{
			Indices.push_back(Vertices.size()    );
			Indices.push_back(Vertices.size() + 1);
			Indices.push_back(Vertices.size() + 2);
			Indices.push_back(Vertices.size()    );
			Indices.push_back(Vertices.size() + 2);
			Indices.push_back(Vertices.size() + 3);

			Vertices.push_back(VEC3(TerrainPatchBoundingBox.p1.x, Height, TerrainPatchBoundingBox.p1.z));
			Vertices.push_back(VEC3(TerrainPatchBoundingBox.p1.x, Height, TerrainPatchBoundingBox.p2.z));
			Vertices.push_back(VEC3(TerrainPatchBoundingBox.p2.x, Height, TerrainPatchBoundingBox.p2.z));
			Vertices.push_back(VEC3(TerrainPatchBoundingBox.p2.x, Height, TerrainPatchBoundingBox.p1.z));
		}
	}

	// Utwórz i wype³nij VB i IB
	if (!Vertices.empty())
	{
		assert(!Indices.empty());

		pimpl->m_VB.reset(new res::D3dVertexBuffer(string(), string(), Vertices.size(), D3DUSAGE_WRITEONLY, WATER_VERTEX_FVF, D3DPOOL_MANAGED));
		pimpl->m_VB->Lock();
		{
			res::D3dVertexBuffer::Locker vb_lock(pimpl->m_VB.get(), true, 0);
			CopyMem(vb_lock.GetData(), &Vertices[0], Vertices.size() * sizeof(VEC3));
		}

		pimpl->m_IB.reset(new res::D3dIndexBuffer(string(), string(), Indices.size(), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED));
		pimpl->m_IB->Lock();
		{
			res::D3dIndexBuffer::Locker ib_lock(pimpl->m_IB.get(), true, 0);
			CopyMem(ib_lock.GetData(), &Indices[0], Indices.size() * sizeof(uint2));
		}
	}

	ERR_CATCH_FUNC;
}

TerrainWater::~TerrainWater()
{
	pimpl->m_IB->Unlock();
	pimpl->m_VB->Unlock();
}

void TerrainWater::Draw(const ParamsCamera &Cam)
{
	if (pimpl->m_VB != NULL)
	{
		assert(pimpl->m_IB != NULL);

		// Tutaj woda jest rysowana wprost w przestrzeni œwiata, nie ma przestrzeni lokalnej modelu.

		base_pimpl->BeginWaterDraw(Cam.GetEyePos(), Cam.GetZFar(), Cam.GetMatrices().GetViewProj());

		uint PrimitiveCount = pimpl->m_IB->GetLength() / 3;
		frame::Dev->SetFVF(WATER_VERTEX_FVF);
		frame::Dev->SetStreamSource(0, pimpl->m_VB->GetVB(), 0, sizeof(VEC3));
		frame::Dev->SetIndices(pimpl->m_IB->GetIB());
		ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, pimpl->m_VB->GetLength(), 0, PrimitiveCount) );
		frame::RegisterDrawCall(PrimitiveCount);

		base_pimpl->EndWaterDraw();
	}
}


} // namespace engine
