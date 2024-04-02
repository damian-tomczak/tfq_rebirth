/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\Res_d3d.hpp"
#include "..\Framework\D3dUtils.hpp"
#include "Engine.hpp"
#include "Camera.hpp"
#include "Fall.hpp"

namespace engine
{

const DWORD VERTEX_FVF       = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);
const DWORD PLANE_VERTEX_FVF = D3DFVF_XYZ |                  D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);

const uint QUAD_COUNT = 1000;
const uint PARTICLES_PRIMITIVE_COUNT = QUAD_COUNT * 2;
const uint PARTICLES_VERTEX_COUNT    = QUAD_COUNT * 4;
const uint PARTICLES_INDEX_COUNT     = QUAD_COUNT * 6;

float FALL_WIND_FACTOR = 1.0f;
const float RAIN_BOX_SIZE = 5.0f;

// Z ilu odcinków sk³ada siê wielok¹t symuluj¹cy walec wokó³ kamery
const uint CIRCLES_SEGMENT_COUNT = 8;
const uint CIRCLES_COUNT = 4;
const float CIRCLES_RADIUS[CIRCLES_COUNT] = { 0.07f, 0.14f, 0.28f, 0.56f };
const uint CIRCLES_VERTEX_COUNT = CIRCLES_COUNT * (CIRCLES_SEGMENT_COUNT+1) * 2;
const uint CIRCLES_PRIMITIVE_COUNT = CIRCLES_COUNT * CIRCLES_SEGMENT_COUNT * 2;
const uint CIRCLES_INDEX_COUNT = CIRCLES_PRIMITIVE_COUNT * 3;

const uint NOISE_QUAD_COUNT = 20;
const uint NOISE_TRIANGLE_COUNT = NOISE_QUAD_COUNT * 2;
const uint NOISE_VERTEX_COUNT = NOISE_QUAD_COUNT * 4;
const uint NOISE_INDEX_COUNT = NOISE_QUAD_COUNT * 6;
const float NOISE_HALF_CX = 0.5f;
const float NOISE_HALF_CY = 0.25f;

struct VERTEX
{
	VEC3 Pos;
	// A (0..255) = rozsuniêcie billboardu wg wektora w prawo (-1..1)
	// R (0..255) = rozsuniêcie billboardu wg wektora w górê  (-1..1)
	// G (0..255) = losowa liczba charakterystyczna dla quada (0..1)
	// B (0..255) = losowa liczba charakterystyczna dla quada (0..1) - obecnie nieu¿ywany
	COLOR Diffuse;
	VEC2 Tex;
};

typedef VERTEX_X2 PLANE_VERTEX;

enum FALL_SHADER_PARAMS
{
	FSP_TEXTURE,
	FSP_WORLD_VIEW_PROJ,
	FSP_RIGHT_VEC,
	FSP_UP_VEC,
	FSP_MOVEMENT_1,
	FSP_MOVEMENT_2,
	FSP_MAIN_COLOR,
	FSP_TRANSLATION,
};

enum FALL_SHADER_PLANES_PARAMS
{
	FSPP_TEXTURE,
	FSPP_WORLD_VIEW_PROJ,
	FSPP_MAIN_COLOR,
	FSPP_TEX_OFFSET,
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Fall_pimpl

class Fall_pimpl
{
public:
	Fall_pimpl(Scene *OwnerScene, const FALL_EFFECT_DESC &EffectDesc);
	~Fall_pimpl();

	void SetEffectDesc(const FALL_EFFECT_DESC &EffectDesc);

	void Update();
	void Draw(const ParamsCamera &Cam);

	//////

	Scene *m_OwnerScene;
	FALL_EFFECT_DESC m_EffectDesc;
	float m_Intensity;

	// Wype³niane na pocz¹tku
	scoped_ptr<res::D3dVertexBuffer> m_ParticlesVB;
	scoped_ptr<res::D3dIndexBuffer> m_ParticlesIB;
	scoped_ptr<res::D3dVertexBuffer> m_NoiseVB;
	scoped_ptr<res::D3dIndexBuffer> m_NoiseIB;
	// Wype³niane przy pierwszym u¿yciu
	scoped_ptr<res::D3dVertexBuffer> m_CirclesVB;
	scoped_ptr<res::D3dIndexBuffer> m_CirclesIB;
	////// Pobierane przy pierwszym u¿yciu
	res::D3dEffect *m_FallShader;
	res::D3dEffect *m_FallPlanesShader;
	res::D3dTexture *m_ParticleTexture;
	res::D3dTexture *m_PlanesTexture;
	res::D3dTexture *m_NoiseTexture;

	VEC3 m_Movement1, m_Movement2;

	void GenerateParticlePos(VEC3 *Out, uint QuadIndex);
	void DrawParticles(const ParamsCamera &Cam, const MATRIX &ShearMat);
	void DrawPlanes(const ParamsCamera &Cam, const MATRIX &ShearMat);
	void DrawNoise(const ParamsCamera &Cam, const MATRIX &ShearMat);

	void CreateCircleBuffers();
	void CreateParticlesBuffers();
	void CreateNoiseBuffers();
	void ResetCircleBuffers();

	uint IntensityToQuadCount()
	{
		// f(0.0) = 0
		// f(0.5) = 1
		return std::min<uint>( (uint)(m_Intensity * 2.0f * QUAD_COUNT), QUAD_COUNT );
	}
	uint IntensityToCircleCount()
	{
		// f(0.5) = 0
		// f(1.0) = 1
		return (uint)minmax<int>( 0, round((m_Intensity * 2.0f - 1.0f) * CIRCLES_COUNT), CIRCLES_COUNT );
	}
	uint IntensityToNoiseQuadCount()
	{
		// f(0.7) = 0
		// f(1.0) = 1
		return (uint)minmax<int>( 0, round((m_Intensity * 3.333f - 2.333f) * NOISE_QUAD_COUNT), NOISE_QUAD_COUNT );
	}
};

Fall_pimpl::Fall_pimpl(Scene *OwnerScene, const FALL_EFFECT_DESC &EffectDesc) :
	m_OwnerScene(OwnerScene),
	m_EffectDesc(EffectDesc),
	m_Intensity(1.0f),
	m_FallShader(NULL),
	m_FallPlanesShader(NULL),
	m_ParticleTexture(NULL),
	m_PlanesTexture(NULL),
	m_NoiseTexture(NULL),
	m_Movement1(VEC3::ZERO),
	m_Movement2(VEC3::ZERO)
{
	assert(frame::Dev && !frame::GetDeviceLost());

	CreateParticlesBuffers();
	CreateNoiseBuffers();
}

Fall_pimpl::~Fall_pimpl()
{
	if (m_NoiseIB != NULL) m_NoiseIB->Unlock();
	if (m_NoiseVB != NULL) m_NoiseVB->Unlock();

	if (m_CirclesIB != NULL) m_CirclesIB->Unlock();
	if (m_CirclesVB != NULL) m_CirclesVB->Unlock();

	if (m_ParticlesIB != NULL) m_ParticlesIB->Unlock();
	if (m_ParticlesVB != NULL) m_ParticlesVB->Unlock();
}

void Fall_pimpl::SetEffectDesc(const FALL_EFFECT_DESC &EffectDesc)
{
	m_EffectDesc = EffectDesc;

	m_ParticleTexture = NULL;
	m_PlanesTexture = NULL;
	ResetCircleBuffers();
}

void Fall_pimpl::Update()
{
	float dt = frame::Timer2.GetDeltaTime();

	m_Movement1 += m_EffectDesc.MovementVec1 * dt;
	m_Movement2 += m_EffectDesc.MovementVec2 * dt;
}

void Fall_pimpl::Draw(const ParamsCamera &Cam)
{
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA); // mimo ¿e nie ma sortowania
	frame::Dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	frame::Dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	float ShearX = m_OwnerScene->GetWindVec().x * FALL_WIND_FACTOR;
	float ShearZ = m_OwnerScene->GetWindVec().y * FALL_WIND_FACTOR;
	MATRIX ShearMat = MATRIX(
		1.0f,   0.0f, 0.0f,   0.0f,
		ShearX, 1.0f, ShearZ, 0.0f,
		0.0f,   0.0f, 1.0f,   0.0f,
		0.0f,   0.0f, 0.0f,   1.0f);

	DrawParticles(Cam, ShearMat);
	DrawPlanes(Cam, ShearMat);
	DrawNoise(Cam, ShearMat);
}

void Fall_pimpl::GenerateParticlePos(VEC3 *Out, uint QuadIndex)
{
	// Tu jest mój autorski algorytm na generowanie losowych, ale rozmieszczonych w miarê sensownie
	// punktów w iloœci wiêkszej, ni¿ posiada POISSON_DISC z modu³u Math, jednak wci¹¿ z niego korzystaj¹c.

	uint Section = QuadIndex / POISSON_DISC_3D_COUNT;

	*Out = POISSON_DISC_3D[QuadIndex % POISSON_DISC_3D_COUNT];

	Out->x += (float)Section * 0.2465f;
	Out->y += (float)Section * 0.3244f;
	Out->z += (float)Section * 0.7583f;

	Out->x = frac(Out->x);
	Out->y = frac(Out->y);
	Out->z = frac(Out->z);
}

void Fall_pimpl::DrawParticles(const ParamsCamera &Cam, const MATRIX &ShearMat)
{
	uint PrimitiveCount = IntensityToQuadCount() * 2;
	if (PrimitiveCount == 0) return;

	if (m_FallShader == NULL)
		m_FallShader = res::g_Manager->MustGetResourceEx<res::D3dEffect>("FallShader");
	m_FallShader->Load();

	if (m_ParticleTexture == NULL)
		m_ParticleTexture = res::g_Manager->MustGetResourceEx<res::D3dTexture>(m_EffectDesc.ParticleTextureName);
	m_ParticleTexture->Load();

	VEC3 RightVec = Cam.GetRightDir() * m_EffectDesc.ParticleHalfSize.x;
	VEC3 UpVec;
	if (m_EffectDesc.UseRealUpDir)
		UpVec = Cam.GetRealUpDir() * m_EffectDesc.ParticleHalfSize.y;
	else
		UpVec = Cam.GetUpDir() * m_EffectDesc.ParticleHalfSize.y;

	MATRIX Scale, Translate;
	Scaling(&Scale, RAIN_BOX_SIZE);
	Translation(&Translate, Cam.GetEyePos());
	MATRIX WorldMat = Scale * ShearMat * Translate;

	VEC3 TranslationVec = - Cam.GetEyePos()
		* 0.5f // bo jest mno¿one * 2 - 1 ¿eby rozci¹gn¹æ zakres 0..1 do -1..1
		/ RAIN_BOX_SIZE;

	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetTexture(m_FallShader->GetParam(FSP_TEXTURE), m_ParticleTexture->GetTexture()) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetMatrix(m_FallShader->GetParam(FSP_WORLD_VIEW_PROJ), (D3DXMATRIX*)&(WorldMat * Cam.GetMatrices().GetViewProj())) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_RIGHT_VEC), &D3DXVECTOR4(RightVec.x, RightVec.y, RightVec.z, 0.0f)) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_UP_VEC), &D3DXVECTOR4(UpVec.x, UpVec.y, UpVec.z, 0.0f)) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_MOVEMENT_1), &D3DXVECTOR4(m_Movement1.x, m_Movement1.y, m_Movement1.z, 1.0f)) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_MOVEMENT_2), &D3DXVECTOR4(m_Movement2.x, m_Movement2.y, m_Movement2.z, 1.0f)) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_MAIN_COLOR), &D3DXVECTOR4(m_EffectDesc.MainColor.R, m_EffectDesc.MainColor.G, m_EffectDesc.MainColor.B, m_EffectDesc.MainColor.A)) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_TRANSLATION), &D3DXVECTOR4(TranslationVec.x, TranslationVec.y, TranslationVec.z, 1.0f)) );

	m_FallShader->Begin(false);

	frame::Dev->SetStreamSource(0, m_ParticlesVB->GetVB(), 0, sizeof(VERTEX));
	frame::Dev->SetIndices(m_ParticlesIB->GetIB());
	frame::Dev->SetFVF(VERTEX_FVF);

	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, PARTICLES_VERTEX_COUNT, 0, PrimitiveCount) );
	frame::RegisterDrawCall(PrimitiveCount);

	m_FallShader->End();
}

void Fall_pimpl::DrawPlanes(const ParamsCamera &Cam, const MATRIX &ShearMat)
{
	uint PrimitiveCount = IntensityToCircleCount() * CIRCLES_SEGMENT_COUNT * 2;
	if (PrimitiveCount == 0) return;

	if (m_CirclesVB == NULL)
		CreateCircleBuffers();
	
	if (m_FallPlanesShader == NULL)
		m_FallPlanesShader = res::g_Manager->MustGetResourceEx<res::D3dEffect>("FallShader_Planes");
	m_FallPlanesShader->Load();

	if (m_PlanesTexture == NULL)
		m_PlanesTexture = res::g_Manager->MustGetResourceEx<res::D3dTexture>(m_EffectDesc.PlaneTextureName);
	m_PlanesTexture->Load();

	MATRIX ScaleMat, Translate;
	Scaling(&ScaleMat, Cam.GetZFar(), 1.0f, Cam.GetZFar());
	Translation(&Translate, Cam.GetEyePos());
	MATRIX WorldMat = ScaleMat * ShearMat * Translate;

	float TexOffset = frame::Timer2.GetTime() * (m_EffectDesc.MovementVec1.y + m_EffectDesc.MovementVec2.y) * m_EffectDesc.PlaneTexScale.y;

	ERR_GUARD_DIRECTX_D( m_FallPlanesShader->Get()->SetTexture(m_FallPlanesShader->GetParam(FSPP_TEXTURE), m_PlanesTexture->GetTexture()) );
	ERR_GUARD_DIRECTX_D( m_FallPlanesShader->Get()->SetMatrix(m_FallPlanesShader->GetParam(FSPP_WORLD_VIEW_PROJ), (D3DXMATRIX*)&(WorldMat * Cam.GetMatrices().GetViewProj())) );
	ERR_GUARD_DIRECTX_D( m_FallPlanesShader->Get()->SetVector(m_FallPlanesShader->GetParam(FSPP_MAIN_COLOR), &D3DXVECTOR4(m_EffectDesc.MainColor.R, m_EffectDesc.MainColor.G, m_EffectDesc.MainColor.B, m_EffectDesc.MainColor.A)) );
	ERR_GUARD_DIRECTX_D( m_FallPlanesShader->Get()->SetFloat(m_FallPlanesShader->GetParam(FSPP_TEX_OFFSET), TexOffset) );

	m_FallPlanesShader->Begin(false);

	frame::Dev->SetFVF(PLANE_VERTEX_FVF);
	frame::Dev->SetStreamSource(0, m_CirclesVB->GetVB(), 0, sizeof(PLANE_VERTEX));
	frame::Dev->SetIndices(m_CirclesIB->GetIB());

	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, CIRCLES_VERTEX_COUNT, 0, PrimitiveCount) );
	frame::RegisterDrawCall(PrimitiveCount);

	m_FallPlanesShader->End();
}

void Fall_pimpl::DrawNoise(const ParamsCamera &Cam, const MATRIX &ShearMat)
{
	uint PrimitiveCount = IntensityToNoiseQuadCount() * 2;
	if (PrimitiveCount == 0) return;

	if (m_FallShader == NULL)
		m_FallShader = res::g_Manager->MustGetResourceEx<res::D3dEffect>("FallShader");
	m_FallShader->Load();

	if (m_NoiseTexture == NULL)
		m_NoiseTexture = res::g_Manager->MustGetResourceEx<res::D3dTexture>("FallNoise");
	m_NoiseTexture->Load();

	VEC3 RightVec = Cam.GetRightDir() * NOISE_HALF_CX;
	VEC3 UpVec = Cam.GetRealUpDir() * NOISE_HALF_CY;

	MATRIX Scale, Translate;
	Scaling(&Scale, RAIN_BOX_SIZE);
	Translation(&Translate, Cam.GetEyePos());
	MATRIX WorldMat = Scale * ShearMat * Translate;

	VEC3 TranslationVec = - Cam.GetEyePos()
		* 0.5f // bo jest mno¿one * 2 - 1 ¿eby rozci¹gn¹æ zakres 0..1 do -1..1
		/ RAIN_BOX_SIZE;

	VEC3 Movement1 = m_Movement1;
	VEC3 Movement2 = m_Movement2;
	COLORF MainColor = m_EffectDesc.NoiseColor;

	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetTexture(m_FallShader->GetParam(FSP_TEXTURE), m_NoiseTexture->GetTexture()) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetMatrix(m_FallShader->GetParam(FSP_WORLD_VIEW_PROJ), (D3DXMATRIX*)&(WorldMat * Cam.GetMatrices().GetViewProj())) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_RIGHT_VEC), &D3DXVECTOR4(RightVec.x, RightVec.y, RightVec.z, 0.0f)) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_UP_VEC), &D3DXVECTOR4(UpVec.x, UpVec.y, UpVec.z, 0.0f)) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_MOVEMENT_1), &D3DXVECTOR4(Movement1.x, Movement1.y, Movement1.z, 1.0f)) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_MOVEMENT_2), &D3DXVECTOR4(Movement2.x, Movement2.y, Movement2.z, 1.0f)) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_MAIN_COLOR), &D3DXVECTOR4(MainColor.R, MainColor.G, MainColor.B, MainColor.A)) );
	ERR_GUARD_DIRECTX_D( m_FallShader->Get()->SetVector(m_FallShader->GetParam(FSP_TRANSLATION), &D3DXVECTOR4(TranslationVec.x, TranslationVec.y, TranslationVec.z, 1.0f)) );

	m_FallShader->Begin(false);

	frame::Dev->SetFVF(VERTEX_FVF);
	frame::Dev->SetStreamSource(0, m_NoiseVB->GetVB(), 0, sizeof(VERTEX));
	frame::Dev->SetIndices(m_NoiseIB->GetIB());

	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, NOISE_VERTEX_COUNT, 0, PrimitiveCount) );
	frame::RegisterDrawCall(PrimitiveCount);

	m_FallShader->End();
}

void Fall_pimpl::CreateCircleBuffers()
{
	ERR_TRY;

	// Zawartoœæ VB jest zale¿na od m_EffectDesc !!!
	// Zawartoœæ IB wprawdzie nie, ale olaæ ten szczegó³...

	assert(m_CirclesVB == NULL && m_CirclesIB == NULL);

	m_CirclesVB.reset(new res::D3dVertexBuffer(string(), string(), CIRCLES_VERTEX_COUNT, D3DUSAGE_WRITEONLY, PLANE_VERTEX_FVF, D3DPOOL_MANAGED));
	m_CirclesIB.reset(new res::D3dIndexBuffer(string(), string(), CIRCLES_INDEX_COUNT, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED));

	m_CirclesVB->Lock();
	m_CirclesIB->Lock();

	// Wype³nij VB
	{
		res::D3dVertexBuffer::Locker vb_lock(m_CirclesVB.get(), true, 0);
		PLANE_VERTEX *VertexPtr = (PLANE_VERTEX*)vb_lock.GetData();

		uint vi = 0;
		float angle, angle_step = PI_X_2 / (float)CIRCLES_SEGMENT_COUNT;
		PLANE_VERTEX v;
		for (uint circle_i = 0; circle_i < CIRCLES_COUNT; circle_i++)
		{
			angle = 0.0f;

			for (uint i = 0; i <= CIRCLES_SEGMENT_COUNT; i++)
			{
				VertexPtr[vi].Pos = VEC3(cosf(angle)*CIRCLES_RADIUS[circle_i], - RAIN_BOX_SIZE, sinf(angle)*CIRCLES_RADIUS[circle_i]);
				VertexPtr[vi].Tex.x = m_EffectDesc.PlaneTexScale.x * angle * CIRCLES_RADIUS[circle_i];
				VertexPtr[vi].Tex.y = m_EffectDesc.PlaneTexScale.y;
				vi++;
				VertexPtr[vi].Pos = VEC3(cosf(angle)*CIRCLES_RADIUS[circle_i],   RAIN_BOX_SIZE, sinf(angle)*CIRCLES_RADIUS[circle_i]);
				VertexPtr[vi].Tex.x = m_EffectDesc.PlaneTexScale.x * angle * CIRCLES_RADIUS[circle_i];
				VertexPtr[vi].Tex.y = - m_EffectDesc.PlaneTexScale.y;
				vi++;

				angle += angle_step;
			}
		}
	}

	// Wype³nij IB
	{
		res::D3dIndexBuffer::Locker ib_lock(m_CirclesIB.get(), true, 0);
		uint2 *IndexPtr = (uint2*)ib_lock.GetData();

		uint ii = 0;
		float angle, angle_step = PI_X_2 / (float)CIRCLES_SEGMENT_COUNT;
		for (uint circle_i = 0; circle_i < CIRCLES_COUNT; circle_i++)
		{
			uint2 base_vi = circle_i * (CIRCLES_SEGMENT_COUNT+1) * 2;
			angle = 0.0f;

			for (uint i = 0; i < CIRCLES_SEGMENT_COUNT; i++)
			{
				IndexPtr[ii] = base_vi + i       * 2;     ii++;
				IndexPtr[ii] = base_vi + i       * 2 + 1; ii++;
				IndexPtr[ii] = base_vi + (i + 1) * 2 + 1; ii++;

				IndexPtr[ii] = base_vi + i       * 2;     ii++;
				IndexPtr[ii] = base_vi + (i + 1) * 2;     ii++;
				IndexPtr[ii] = base_vi + (i + 1) * 2 + 1; ii++;

				angle += angle_step;
			}
		}
	}

	ERR_CATCH_FUNC;
}

void Fall_pimpl::CreateParticlesBuffers()
{
	ERR_TRY;

	m_ParticlesVB.reset(new res::D3dVertexBuffer(string(), string(), PARTICLES_VERTEX_COUNT, D3DUSAGE_WRITEONLY, VERTEX_FVF, D3DPOOL_MANAGED));
	m_ParticlesIB.reset(new res::D3dIndexBuffer(string(), string(), PARTICLES_INDEX_COUNT, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED));

	m_ParticlesVB->Lock();
	m_ParticlesIB->Lock();

	// Wype³nij VB
	{
		res::D3dVertexBuffer::Locker vb_lock(m_ParticlesVB.get(), true, 0);
		VERTEX *VertexPtr = (VERTEX*)vb_lock.GetData();
		uint Random1, Random2;
		for (uint qi = 0; qi < QUAD_COUNT; qi++)
		{
			GenerateParticlePos(&VertexPtr[0].Pos, qi);
			VertexPtr[1].Pos = VertexPtr[0].Pos;
			VertexPtr[2].Pos = VertexPtr[0].Pos;
			VertexPtr[3].Pos = VertexPtr[0].Pos;
			
			Random1 = (uint1)g_Rand.RandUint(256);
			Random2 = (uint1)g_Rand.RandUint(256);

			VertexPtr[0].Diffuse.A =   0;
			VertexPtr[0].Diffuse.R =   0;
			VertexPtr[0].Diffuse.G = Random1;
			VertexPtr[0].Diffuse.B = Random2;
			VertexPtr[1].Diffuse.A =   0;
			VertexPtr[1].Diffuse.R = 255;
			VertexPtr[1].Diffuse.G = Random1;
			VertexPtr[1].Diffuse.B = Random2;
			VertexPtr[2].Diffuse.A = 255;
			VertexPtr[2].Diffuse.R = 255;
			VertexPtr[2].Diffuse.G = Random1;
			VertexPtr[2].Diffuse.B = Random2;
			VertexPtr[3].Diffuse.A = 255;
			VertexPtr[3].Diffuse.R =   0;
			VertexPtr[3].Diffuse.G = Random1;
			VertexPtr[3].Diffuse.B = Random2;

			VertexPtr[0].Tex = VEC2(0.0f, 1.0f);
			VertexPtr[1].Tex = VEC2(0.0f, 0.0f);
			VertexPtr[2].Tex = VEC2(1.0f, 0.0f);
			VertexPtr[3].Tex = VEC2(1.0f, 1.0f);

			VertexPtr += 4;
		}
	}

	// Wype³nij IB
	{
		res::D3dIndexBuffer::Locker ib_lock(m_ParticlesIB.get(), true, 0);
		uint2 *IndexPtr = (uint2*)ib_lock.GetData();
		for (uint qi = 0, vi = 0; qi < QUAD_COUNT; qi++)
		{
			*IndexPtr = vi    ; IndexPtr++;
			*IndexPtr = vi + 1; IndexPtr++;
			*IndexPtr = vi + 2; IndexPtr++;
			*IndexPtr = vi    ; IndexPtr++;
			*IndexPtr = vi + 2; IndexPtr++;
			*IndexPtr = vi + 3; IndexPtr++;
			vi += 4;
		}
	}

	ERR_CATCH_FUNC;
}

void Fall_pimpl::CreateNoiseBuffers()
{
	ERR_TRY;

	m_NoiseVB.reset(new res::D3dVertexBuffer(string(), string(), NOISE_VERTEX_COUNT, D3DUSAGE_WRITEONLY, VERTEX_FVF, D3DPOOL_MANAGED));
	m_NoiseIB.reset(new res::D3dIndexBuffer(string(), string(), NOISE_INDEX_COUNT, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED));

	m_NoiseVB->Lock();
	m_NoiseIB->Lock();

	// Wype³nij VB i IB
	{
		res::D3dVertexBuffer::Locker vb_lock(m_NoiseVB.get(), true, 0);
		res::D3dIndexBuffer::Locker ib_lock(m_NoiseIB.get(), true, 0);
		VERTEX *va = (VERTEX*)vb_lock.GetData();
		uint2 *ia = (uint2*)ib_lock.GetData();

		uint Random1, Random2;
		for (uint qi = 0; qi < NOISE_QUAD_COUNT; qi++)
		{
			Random1 = (uint1)g_Rand.RandUint(256);
			Random2 = (uint1)g_Rand.RandUint(256);

			va[qi * 4    ].Pos = POISSON_DISC_3D[qi];
			va[qi * 4 + 1].Pos = va[qi * 4].Pos;
			va[qi * 4 + 2].Pos = va[qi * 4].Pos;
			va[qi * 4 + 3].Pos = va[qi * 4].Pos;

			va[qi * 4    ].Tex = VEC2(0.0f, 1.0f);
			va[qi * 4 + 1].Tex = VEC2(0.0f, 0.0f);
			va[qi * 4 + 2].Tex = VEC2(1.0f, 0.0f);
			va[qi * 4 + 3].Tex = VEC2(1.0f, 1.0f);

			va[qi * 4    ].Diffuse = COLOR(  0,   0, Random1, Random2);
			va[qi * 4 + 1].Diffuse = COLOR(  0, 255, Random1, Random2);
			va[qi * 4 + 2].Diffuse = COLOR(255, 255, Random1, Random2);
			va[qi * 4 + 3].Diffuse = COLOR(255,   0, Random1, Random2);

			ia[qi * 6    ] = qi * 4    ;
			ia[qi * 6 + 1] = qi * 4 + 1;
			ia[qi * 6 + 2] = qi * 4 + 2;
			ia[qi * 6 + 3] = qi * 4    ;
			ia[qi * 6 + 4] = qi * 4 + 2;
			ia[qi * 6 + 5] = qi * 4 + 3;
		}
	}

	ERR_CATCH_FUNC;
}

void Fall_pimpl::ResetCircleBuffers()
{
	if (m_CirclesIB != NULL)
	{
		m_CirclesIB->Unlock();
		m_CirclesIB.reset();
	}
	if (m_CirclesVB != NULL)
	{
		m_CirclesVB->Unlock();
		m_CirclesVB.reset();
	}
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Fall

Fall::Fall(Scene *OwnerScene, const FALL_EFFECT_DESC &EffectDesc) :
	pimpl(new Fall_pimpl(OwnerScene, EffectDesc))
{
}

Fall::~Fall()
{
}

const FALL_EFFECT_DESC & Fall::GetEffectDesc()
{
	return pimpl->m_EffectDesc;
}

void Fall::SetEffectDesc(const FALL_EFFECT_DESC &EffectDesc)
{
	pimpl->SetEffectDesc(EffectDesc);
}

float Fall::GetIntensity()
{
	return pimpl->m_Intensity;
}

void Fall::SetIntensity(float Intensity)
{
	assert(Intensity >= 0.0f && Intensity <= 1.0f);
	pimpl->m_Intensity = Intensity;
}

void Fall::Draw(const ParamsCamera &Cam)
{
	pimpl->Update();
	pimpl->Draw(Cam);
}


} // namespace engine
