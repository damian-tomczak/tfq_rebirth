/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\Multishader.hpp"
#include "Engine_pp.hpp"
#include "QMap.hpp"
#include "Terrain.hpp"
#include "Sky.hpp"
#include "Trees.hpp"
#include "Grass.hpp"
#include "Water.hpp"
#include "Fall.hpp"
#include "IcosphereRes.hpp"
#include "Engine.hpp"

const uint LOG_ENGINE = 0x04;


namespace engine
{

const uint MAX_REAL_OCCLUSION_QUERIES = 50;
const D3DFORMAT SHADOW_MAPPING_DEPTH_STENCIL_FORMAT = D3DFMT_D16;
// Ile ró¿nych wielkoœci Shadow Map trzymaæ
const uint SHADOW_MAP_LEVELS = 4;

// Czas co jaki robiæ porz¹dki w silniku [s]
const float MAINTENANCE_TIME = 10.f;
// Czas po jakim zwalniaæ nieu¿ywan¹ teksturê pomocnicz¹ [s]
const float UNUSED_TEXTURE_FREE_TIME = 30.f;

// Dzielniki rozmiaru tekstury specjalnej
const uint4 ST_TONE_MAPPING_DIV = 50;
const uint4 ST_BLUR_DIV = 4;
const uint4 ST_FEEDBACK_DIV = 2;

const float DIRECTIONAL_LIGHT_SHADOW_MAP_EYE_OFFSET = 30.f;
const float DIRECTIONAL_LIGHT_SHADOW_ATTEN_START_PERCENT = 0.5f;

enum HEAT_EFFECT_PARAMS
{
	HEP_WorldViewProj,
	HEP_DirToCam,
	HEP_ZFar
};

// Funkcja sprawdza kolizje sfery poruszaj¹cej siê o dowolnie daleki odcinek w kierunku SphereVel (ale tylko do przodu).
// Nie wykrywa parametru T dla kolizji.
// Jest wymyœlona przeze mnie i bez gwarancji/dowodu, ¿e dobrze dzia³a (aczkolwiek by³a testowana).
// Dla w miarê szeœciennych prostopad³oœcianów (i du¿ych sfer) powinna dzia³aæ, gorzej z bardzo w¹skimi i pod³u¿nymi.
bool SweptSphereToBox(const VEC3 &SphereCenter, float SphereRadius, const VEC3 &SphereVel, const BOX &Box)
{
	VEC3 BoxCenter; Box.CalcCenter(&BoxCenter);
	float ClosestPointT = std::max(0.f, ClosestPointOnLine(BoxCenter, SphereCenter, SphereVel) / LengthSq(SphereVel));
	VEC3 ClosestPoint = SphereCenter + SphereVel * ClosestPointT;
	return SphereToBox(ClosestPoint, SphereRadius, Box);
}

void FindDirectionalViewProjMatrix(MATRIX *OutViewProj, const BOX &CamBox, const VEC3 &LightDir)
{
	// Wierzcho³ek CamBox w kierunku Ÿród³a œwiat³a
	VEC3 CamBoxPt;
	CamBoxPt.x = (LightDir.x < 0.f ? CamBox.p1.x : CamBox.p2.x);
	CamBoxPt.y = (LightDir.y < 0.f ? CamBox.p1.y : CamBox.p2.y);
	CamBoxPt.z = (LightDir.z < 0.f ? CamBox.p1.z : CamBox.p2.z);

	// P³aszczyzna przechodz¹ca przez ten wierzcho³ek i prostopad³a do kierunku œwiat³a (zwrot niewa¿ny)
	PLANE Plane;
	PointNormalToPlane(&Plane, CamBoxPt, LightDir);
	//Normalize(&Plane); // Raczej niepotrzebne

	// Punkt przeciêcia tej p³aszczyzny z promieniem œwiat³a wychodz¹cym z œrodka boksa
	VEC3 RayOrig;
	CamBox.CalcCenter(&RayOrig);
	float Foo, T;
	if (!RayToPlane(RayOrig, -LightDir, Plane, &T, &Foo))
	{
		// Coœ nie tak!
		Identity(OutViewProj);
		return;
	}

	// Punkt przeciêcia p³aszczyzny z promieniem bêdzie punktem z którego patrzy kamera
	// Ale przesuniêty o rozszerzenie - hardcoded magic number
	VEC3 EyePos = RayOrig - LightDir * T;

	// Wektor do góry
	VEC3 UpDir;
	if (fabsf(Dot(LightDir, VEC3::POSITIVE_Y)) > 0.99f)
		UpDir = VEC3::POSITIVE_Z;
	else
		UpDir = VEC3::POSITIVE_Y;

	// Macierz widoku
	MATRIX LightView;
	LookAtLH(&LightView, EyePos, LightDir, UpDir);

	// ZnajdŸ AABB otaczaj¹cy to na co rzucamy cieñ ale w przestrzeni ju¿ kamery œwiat³a
	BOX TransformedBox;
	TransformBox(&TransformedBox, CamBox, LightView);

	// Macierz rzutowania
	MATRIX LightProj;
	OrthoOffCenterLH(&LightProj,
		TransformedBox.p1.x, TransformedBox.p2.x,
		TransformedBox.p1.y, TransformedBox.p2.y,
		TransformedBox.p1.z - DIRECTIONAL_LIGHT_SHADOW_MAP_EYE_OFFSET, TransformedBox.p2.z);

	*OutViewProj = LightView * LightProj;
}

struct TerrainMaterial : public BaseMaterial
{
	res::D3dTexture *Textures[TERRAIN_FORMS_PER_PATCH];
	float TexScale[TERRAIN_FORMS_PER_PATCH];

	TerrainMaterial(Scene *OwnerScene);
	virtual TYPE GetType() { return TYPE_TERRAIN; }
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasy EntityDistanceCompare_*

class EntityDistanceCompare_Less
{
private:
	VEC3 m_Eye;

public:
	EntityDistanceCompare_Less(const VEC3 &Eye) : m_Eye(Eye) { }

	bool operator () (Entity *e1, Entity *e2)
	{
		return DistanceSq(e1->GetWorldPos(), m_Eye) < DistanceSq(e2->GetWorldPos(), m_Eye);
	}
};

class EntityDistanceCompare_Greater
{
private:
	VEC3 m_Eye;

public:
	EntityDistanceCompare_Greater(const VEC3 &Eye) : m_Eye(Eye) { }

	bool operator () (Entity *e1, Entity *e2)
	{
		return DistanceSq(e1->GetWorldPos(), m_Eye) > DistanceSq(e2->GetWorldPos(), m_Eye);
	}
};

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktura STATS

STATS::STATS() :
	Passes(0),
	MapFragments(0),
	TerrainPatches(0),
	Trees(0),
	MainShaders(0),
	PpShaders(0)
{
	Entities[0] = Entities[1] = 0;
	PointLights[0] = PointLights[1] = 0;
	SpotLights[0] = SpotLights[1] = 0;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa EngineServices

enum PP_SHADER_MACRO
{
	PSM_GRAYSCALE,
	PSM_FUNCTION,
	PSM_TONE_MAPPING,
	PSM_HEAT_HAZE,
	PSM_COUNT
};

enum PP_SHADER_PARAM
{
	PSP_Texture,

	PSP_GrayscaleFactor,
	PSP_AFactor,
	PSP_BFactor,

	PSP_LuminanceFactor,

	PSP_PerturbationMap,

	PSP_COUNT
};

enum MAIN_SHADER_MACRO
{
	MSM_PASS,
	MSM_EMISSIVE,
	MSM_AMBIENT_MODE,
	MSM_DIFFUSE_TEXTURE,
	MSM_COLOR_MODE,
	MSM_TEXTURE_ANIMATION,
	MSM_ENVIRONMENTAL_MAPPING,
	MSM_ALPHA_TESTING,
	MSM_USE_FOG,
	MSM_ALPHA_MODE,
	MSM_FRESNEL_TERM,
	MSM_HALF_LAMBERT,
	MSM_PER_PIXEL,
	MSM_NORMAL_TEXTURE,
	MSM_SPECULAR,
	MSM_GLOSS_MAP,
	MSM_SPOT_SMOOTH,
	MSM_SHADOW_MAP_MODE,
	MSM_VARIABLE_SHADOW_FACTOR,
	MSM_POINT_SHADOW_MAP,
	MSM_SKINNING,
	MSM_TERRAIN,

	MSM_COUNT,
};

enum MAIN_SHADER_PARAM
{
	MSP_WorldViewProj,
	MSP_EmissiveTexture,
	MSP_DiffuseTexture,
	MSP_DiffuseColor,
	MSP_TeamColor,
	MSP_AmbientColor,
	MSP_TextureMatrix,
	MSP_CameraPos,
	MSP_EnvironmentalTexture,
	MSP_FogColor,
	MSP_FogFactors,
	MSP_FresnelColor,
	MSP_FresnelPower,
	MSP_LightColor,
	MSP_DirToLight,
	MSP_NormalTexture,
	MSP_SpecularColor,
	MSP_SpecularPower,
	MSP_LightPos,
	MSP_LightRangeSq,
	MSP_LightCosFov2,
	MSP_LightCosFov2Factor,
	MSP_ShadowFactor,
	MSP_ShadowFactorA,
	MSP_ShadowFactorB,
	MSP_ShadowMapTexture,
	MSP_ShadowMapMatrix,
	MSP_ShadowMapSize,
	MSP_ShadowMapSizeRcp,
	MSP_ShadowEpsilon,
	MSP_LightRangeSq_World,
	MSP_BoneMatrices,
	MSP_TerrainTexture0,
	MSP_TerrainTexture1,
	MSP_TerrainTexture2,
	MSP_TerrainTexture3,
	MSP_TerrainTexScale,

	MSP_COUNT
};

struct ENTITY_DRAW_PARAMS
{
	float WorldSize;
	const MATRIX & WorldMatrix;
	const MATRIX & WorldInvMatrix;
	const MATRIX & TextureMatrix;
	const COLORF & TeamColor;
	bool UseLighting;
	uint BoneCount;
	const MATRIX *BoneMatrices; // WskaŸnik do tablicy, NULL jeœli nie ma Skinningu

	ENTITY_DRAW_PARAMS(
		float WorldSize,
		const MATRIX &WorldMatrix,
		const MATRIX &WorldInvMatrix,
		const MATRIX &TextureMatrix,
		const COLORF &TeamColor,
		bool UseLighting,
		uint BoneCount,
		const MATRIX *BoneMatrices
	) :
		WorldSize(WorldSize),
		WorldMatrix(WorldMatrix),
		WorldInvMatrix(WorldInvMatrix),
		TextureMatrix(TextureMatrix),
		TeamColor(TeamColor),
		UseLighting(UseLighting),
		BoneCount(BoneCount),
		BoneMatrices(BoneMatrices)
	{
	}
};

struct SCENE_DRAW_PARAMS
{
	Scene *SceneObj;
	// NULL jeœli nieu¿ywana
	res::D3dTextureSurface *ShadowMapTexture;
	res::D3dCubeTextureSurface *ShadowMapCubeTexture;
	// NULL jeœli nieu¿ywana
	// U¿ywana w dwóch przypadkach:
	// - Kiedy rysujemy geometriê oœwietlon¹ danym œwiat³em i nak³adamy Shadow Mapê
	// - Kiedy rysujemy geometriê do Shadow Mapy Directional Light (wówczas nie ma kamery, tu jest g³ówne ViewProj)
	const MATRIX *ShadowMapMatrix;

	bool UseVariableShadowFactor;
	float ShadowFactorAttnStart;
	float ShadowFactorAttnEnd;

	SCENE_DRAW_PARAMS(Scene *SceneObj) : SceneObj(SceneObj), ShadowMapTexture(NULL), ShadowMapMatrix(NULL), UseVariableShadowFactor(false) { }
	SCENE_DRAW_PARAMS(Scene *SceneObj, res::D3dTextureSurface *ShadowMapTexture, const MATRIX *ShadowMapMatrix) : SceneObj(SceneObj), ShadowMapTexture(ShadowMapTexture), ShadowMapCubeTexture(NULL), ShadowMapMatrix(ShadowMapMatrix), UseVariableShadowFactor(false) { }
	SCENE_DRAW_PARAMS(Scene *SceneObj, res::D3dCubeTextureSurface *ShadowMapCubeTexture, const MATRIX *ShadowMapMatrix) : SceneObj(SceneObj), ShadowMapTexture(NULL), ShadowMapCubeTexture(ShadowMapCubeTexture), ShadowMapMatrix(ShadowMapMatrix), UseVariableShadowFactor(false) { }
};

/*
Pojedynczy obiekt tej klasy jest tworzony wraz z Engine i wraz z nim usuwany.
Mo¿emy byæ pewni, ¿e w momencie jego tworzenia Engine ma ju¿ wczytan¹
konfiguracjê - mo¿na jej u¿ywaæ.
*/
class EngineServices
{
public:
	enum SPECIAL_TEXTURE
	{
		ST_SCREEN,
		ST_TONE_MAPPING_VRAM,
		ST_TONE_MAPPING_RAM,
		ST_BLUR_1,
		ST_BLUR_2,
		ST_FEEDBACK,
		ST_COUNT
	};

	EngineServices(Engine *a_Engine);
	~EngineServices();

	// Zwraca zasób z siatk¹ sfery dla Occlusion Query. Jest Loaded.
	IcosphereRes * GetIcosphere() { m_Icosphere->Load(); return m_Icosphere.get(); }
	// Zwraca zasób z Occlusion Queries. Jest Loaded.
	res::OcclusionQueries * GetOcclusionQueries() { m_OcclusionQueries->Load(); return m_OcclusionQueries.get(); }
	// Zwraca zasób z powierzchni¹ Depth Stencil dla Shadow Mappingu. Jest Loaded.
	// Jest conajmniej tak du¿a, jak najwiêksza u¿ywana Shadow Mapa.
	res::D3dTextureSurface * GetShadowMappingDepthStencil() { m_SM_DepthStencil->Load(); return m_SM_DepthStencil.get(); }
	// Zwraca Shadow Mapê dla œwiat³a Directional - najwiêksz¹ jak¹ ma. Jest Loaded.
	res::D3dTextureSurface * GetDirectionalShadowMap();
	// Zwraca Shadow Mapê dla œwiat³a Spot o rozmiarze proporcjonalnym do SizeFactor. Jest Loaded.
	// SizeFactor = 0.0 .. 1.0, gdzie 0.0 to najmniejsza, 1.0 najwiêksza
	res::D3dTextureSurface * GetSpotShadowMap(float SizeFactor);
	// Zwraca Cube Shadow Mapê dla œwiat³a Point o rozmiarze proporcjonalnym do SizeFactor. Jest Loaded.
	// SizeFactor = 0.0 .. 1.0, gdzie 0.0 to najmniejsza, 1.0 najwiêksza
	res::D3dCubeTextureSurface * GetPointShadowMap(float SizeFactor);
	// £aduje (sama wywo³uje Load) i zwraca ¿¹dan¹ teksturê specjaln¹
	res::D3dTextureSurface * EnsureSpecialTexture(SPECIAL_TEXTURE Texture);
	// Jeœli dana tekstura specjalna jest utworzona i Loaded, zwraca j¹. Jeœli nie, zwraca NULL.
	// Do celów debugowania.
	res::D3dTextureSurface * TryGetSpecialTexture(SPECIAL_TEXTURE Texture);
	// Zwraca efekt do rysowania Occlusion Queries. Jest LOADED.
	res::D3dEffect * GetOcclusionQueryEffect() { m_OcclusionQueryEffect->Load(); return m_OcclusionQueryEffect; }
	// Zwraca zasób ostatnio u¿ywanej nie-cube shadow mapy (loaded) lub NULL
	res::D3dTextureSurface * GetLastUsedShadowMap();
	res::D3dCubeTextureSurface * GetLastUsedCubeShadowMap();
	// Generuje macierz dodatkow¹ przez któr¹ nale¿y pomno¿yæ macierz ViewProj u¿ywan¹ do renderowania do Shadow Mapy,
	// by u¿yæ jej do samplowania Shadow Mapy.
	void CalcShadowMapMatrix2(MATRIX *Out);
	// Funkcje statystyczne
	uint GetMainShaderCount();
	uint GetPpShaderCount();

	void SetupState_Pp(const IDirect3DTexture9 *ScreenTexture, PpFunction *E_Function, PpToneMapping *E_ToneMapping, bool HeatHaze);
	void SetupState_Material(PASS Pass, const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *cam, BaseMaterial &Material, const ENTITY_DRAW_PARAMS &EntityDrawParams, BaseLight *Light);
	void SetupState_Heat(const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam, HeatEntity *heat_entity);
	void UnsetupState();

	void Update();

	// ======== Dla klasy Engine ========
	void CreateSpecialTextures();
	void DestroySpecialTextures();
	void SetSpecialTexturesSize();

private:
	scoped_ptr<IcosphereRes> m_Icosphere;
	scoped_ptr<res::OcclusionQueries> m_OcclusionQueries;
	// Te tablice przechowuj¹ ka¿dy malej¹ce rozmiarami Shadow Mapy np. 512, 256, 64, 32.
	// S¹ SHADOW_MAP_LEVELS ró¿ne rozmiary.
	// Tworzone od razu, Load-owane przed ka¿dym u¿yciem.
	scoped_ptr<res::D3dTextureSurface> m_SM_Normal[SHADOW_MAP_LEVELS];
	scoped_ptr<res::D3dCubeTextureSurface> m_SM_Cube[SHADOW_MAP_LEVELS];
	// Depth Stencil Surface u¿ywany przez wszystkie Shadow Mapy
	scoped_ptr<res::D3dTextureSurface> m_SM_DepthStencil;
	// Czas ostatniego porz¹dkowania
	float m_MaintenanceLastTime;
	ID3DXEffect *m_StartedEffect;
	res::Multishader *m_MainShader;
	res::Multishader *m_PpShader;
	// Efekt do rysowania Heat Geometry. NULL jeœli jeszcze nie pobrany.
	res::D3dEffect *m_HeatEffect;
	// Tekstura narzêdziowa do Heat Haze. NULL, jeœli jeszcze nie pobrana.
	res::D3dTexture *m_PerturbationMap;
	// Zapisany w Resources.dat
	res::D3dEffect *m_OcclusionQueryEffect;
	res::D3dTextureSurface *m_LastUsedShadowMap;
	res::D3dCubeTextureSurface *m_LastUsedCubeShadowMap;

	// ======== Postprocessing ========

	scoped_ptr<res::D3dTextureSurface> m_SpecialTextures[ST_COUNT];
	float m_SpecialTexturesLastUseTime[ST_COUNT];

	void Maintenance();
};


EngineServices::EngineServices(Engine *a_Engine) :
	m_Icosphere(new IcosphereRes(string(), string())),
	m_OcclusionQueries(new res::OcclusionQueries(string(), string(), MAX_REAL_OCCLUSION_QUERIES)),
	m_MaintenanceLastTime(0.f),
	m_StartedEffect(NULL),
	m_MainShader(NULL),
	m_PpShader(NULL),
	m_HeatEffect(NULL),
	m_PerturbationMap(NULL),
	m_LastUsedShadowMap(NULL),
	m_LastUsedCubeShadowMap(NULL)
{
	m_MainShader = res::g_Manager->MustGetResourceEx<res::Multishader>("MainShader");
	m_PpShader = res::g_Manager->MustGetResourceEx<res::Multishader>("PpShader");
	m_PerturbationMap = res::g_Manager->MustGetResourceEx<res::D3dTexture>("PerturbationMap");
	m_OcclusionQueryEffect = res::g_Manager->MustGetResourceEx<res::D3dEffect>("OcclusionQueryEffect");

	uint Size;

	D3DFORMAT ShadowMapFormat = a_Engine->GetShadowMapFormat();

	// Normal
	const uint NormalSize = a_Engine->GetConfigUint(Engine::O_SM_MAX_SIZE);
	Size = NormalSize;
	for (uint i = 0; i < SHADOW_MAP_LEVELS; i++)
	{
		m_SM_Normal[i].reset(new res::D3dTextureSurface(
			Format("ShadowMap_Normal_#") % i,
			string(),
			true,
			Size, Size,
			D3DPOOL_DEFAULT,
			D3DUSAGE_RENDERTARGET,
			ShadowMapFormat));
		Size /= 2;
	}

	// Cube
	const uint CubeSize = a_Engine->GetConfigUint(Engine::O_SM_MAX_CUBE_SIZE);
	Size = CubeSize;
	for (uint i = 0; i < SHADOW_MAP_LEVELS; i++)
	{
		m_SM_Cube[i].reset(new res::D3dCubeTextureSurface(
			Format("ShadowMap_Cube_#") % i,
			string(),
			Size,
			D3DPOOL_DEFAULT,
			D3DUSAGE_RENDERTARGET,
			ShadowMapFormat));
		Size /= 2;
	}

	// Depth Stencil Surface do Shadow Mappingu
	uint DepthStencilSize = std::max(NormalSize, CubeSize);
	m_SM_DepthStencil.reset(new res::D3dTextureSurface(
		"ShadowMapping_DepthStencil",
		string(),
		false,
		DepthStencilSize, DepthStencilSize,
		D3DPOOL_DEFAULT,
		D3DUSAGE_DEPTHSTENCIL,
		SHADOW_MAPPING_DEPTH_STENCIL_FORMAT));

	// Tekstury specjalne
	for (size_t i = 0; i < ST_COUNT; i++)
		m_SpecialTexturesLastUseTime[i] = 0.f;
	if (a_Engine->GetConfigBool(Engine::O_PP_ENABLED))
		CreateSpecialTextures();
	SetSpecialTexturesSize();
}

EngineServices::~EngineServices()
{
	DestroySpecialTextures();
}

res::D3dTextureSurface * EngineServices::GetDirectionalShadowMap()
{
	m_LastUsedShadowMap = m_SM_Normal[0].get();
	m_SM_Normal[0]->Load();
	return m_SM_Normal[0].get();
}

res::D3dTextureSurface * EngineServices::GetSpotShadowMap(float SizeFactor)
{
	// Wybierz poziom
	// Ten magiczny wzór odwzorowuje przedzia³ SizeFactor = 1.0 .. 0.0 na równe przedzia³y 0,1,2,3
	uint Level = (uint)minmax<int>(0, round((1.0f - SizeFactor) * (float)SHADOW_MAP_LEVELS - 0.5f), 3);

	m_LastUsedShadowMap = m_SM_Normal[Level].get();
	m_SM_Normal[Level]->Load();
	return m_SM_Normal[Level].get();
}

res::D3dCubeTextureSurface * EngineServices::GetPointShadowMap(float SizeFactor)
{
	// Wybierz poziom
	// Ten magiczny wzór odwzorowuje przedzia³ SizeFactor = 1.0 .. 0.0 na równe przedzia³y 0,1,2,3
	uint Level = (uint)minmax<int>(0, round((1.0f - SizeFactor) * (float)SHADOW_MAP_LEVELS - 0.5f), 3);

	m_LastUsedCubeShadowMap = m_SM_Cube[Level].get();
	m_SM_Cube[Level]->Load();
	return m_SM_Cube[Level].get();
}

res::D3dTextureSurface * EngineServices::EnsureSpecialTexture(SPECIAL_TEXTURE Texture)
{
	uint4 Index = (uint4)Texture;

	assert(g_Engine->GetConfigBool(Engine::O_PP_ENABLED));
	assert(m_SpecialTextures[Index] != NULL);

	m_SpecialTexturesLastUseTime[Index] = frame::Timer1.GetTime();

	m_SpecialTextures[Index]->Load();
	return m_SpecialTextures[Index].get();
}

res::D3dTextureSurface * EngineServices::TryGetSpecialTexture(SPECIAL_TEXTURE Texture)
{
	uint4 Index = (uint4)Texture;

	if (m_SpecialTextures[Index] != NULL && m_SpecialTextures[Index]->IsLoaded())
		return m_SpecialTextures[Index].get();
	else
		return NULL;
}

res::D3dTextureSurface * EngineServices::GetLastUsedShadowMap()
{
	if (m_LastUsedShadowMap != NULL && m_LastUsedShadowMap->IsLoaded())
		return m_LastUsedShadowMap;
	else
		return NULL;
}

res::D3dCubeTextureSurface * EngineServices::GetLastUsedCubeShadowMap()
{
	if (m_LastUsedCubeShadowMap != NULL && m_LastUsedCubeShadowMap->IsLoaded())
		return m_LastUsedCubeShadowMap;
	else
		return NULL;
}

uint EngineServices::GetMainShaderCount()
{
	if (m_MainShader == NULL || !m_MainShader->IsLoaded())
		return 0;
	return m_MainShader->GetShaderCount();
}

uint EngineServices::GetPpShaderCount()
{
	if (m_PpShader == NULL || !m_PpShader->IsLoaded())
		return 0;
	return m_PpShader->GetShaderCount();
}

void EngineServices::CalcShadowMapMatrix2(MATRIX *Out)
{
	float Epsilon = g_Engine->GetConfigFloat(Engine::O_SM_EPSILON);
	*Out = MATRIX(
		0.5f,  0.0f,  0.0f,    0.0f,
		0.0f, -0.5f,  0.0f,    0.0f,
		0.0f,  0.0f,  1.0f,    0.0f,
		0.5f,  0.5f, -Epsilon, 1.0f); // Jeœli do SM renderowane przednie œciany
}

void EngineServices::SetupState_Pp(const IDirect3DTexture9 *ScreenTexture, PpFunction *E_Function, PpToneMapping *E_ToneMapping, bool HeatHaze)
{
	ERR_TRY

	m_PpShader->Load();

	// ======== Sformu³uj makra ========

	uint4 Macros[PSM_COUNT];
	for (size_t mi = 0; mi < PSM_COUNT; mi++)
		Macros[mi] = 0;

	if (E_Function)
	{
		if (E_Function->GetGrayscaleFactor() > 0.001f)
		{
			if (E_Function->GetGrayscaleFactor() >= 0.999f)
				Macros[PSM_GRAYSCALE] = 1;
			else
				Macros[PSM_GRAYSCALE] = 2;
		}

		if (E_Function->IsLinearTransformRequired())
			Macros[PSM_FUNCTION] = 1;
	}

	if (E_ToneMapping)
		Macros[PSM_TONE_MAPPING] = 1;

	if (HeatHaze)
		Macros[PSM_HEAT_HAZE] = 1;

	// ======== Pobierz i w razie potrzeby wczytaj efekt o podanym hashu ========

	// SprawdŸ, czy te ustawienia s¹ dozwolone
	//if (!ValidatePpShaderMacros(Macros))
	//{
	//	string Msg;
	//	m_PpShader->MacrosToStr(&Msg, Macros);
	//	throw Error("B³êdna kombinacja ustawieñ shadera preprocessingu: " + Msg);
	//}

	const res::Multishader::SHADER_INFO & ShaderInfo = m_PpShader->GetShader(Macros);

	m_StartedEffect = ShaderInfo.Effect;

	// ======== Wpisz parametry ========

#ifdef DEBUG
	#define GUARD(name, expr) { if (FAILED(expr)) throw Error("Nie mo¿na ustawiæ parametru: " + string(name)); }
#else
	#define GUARD(name, expr) { (expr); }
#endif

	GUARD( "Texture", m_StartedEffect->SetTexture(ShaderInfo.Params[PSP_Texture], (IDirect3DBaseTexture9*)ScreenTexture) );

	if (Macros[PSM_GRAYSCALE] == 2)
	{
		GUARD( "GrayscaleFactor", m_StartedEffect->SetFloat(ShaderInfo.Params[PSP_GrayscaleFactor], E_Function->GetGrayscaleFactor()) );
	}
	if (Macros[PSM_FUNCTION] == 1)
	{
		GUARD( "AFactor", m_StartedEffect->SetVector(ShaderInfo.Params[PSP_AFactor], &D3DXVECTOR4(E_Function->GetAFactor().x, E_Function->GetAFactor().y, E_Function->GetAFactor().z, 0.f)) );
		GUARD( "BFactor", m_StartedEffect->SetVector(ShaderInfo.Params[PSP_BFactor], &D3DXVECTOR4(E_Function->GetBFactor().x, E_Function->GetBFactor().y, E_Function->GetBFactor().z, 0.f)) );
	}

	if (Macros[PSM_TONE_MAPPING] == 1)
	{
		// Luminance = 0..1, oko³o 0.5, tym wiêksze maksymalne odchylenie od 0.5 jak du¿e Intensity
		float Luminance = (E_ToneMapping->GetLastBrighness() - 0.5f) * E_ToneMapping->GetIntensity() + 0.5f;
		// LuminanceFactor = 0.5 .. 1.5
		float LuminanceFactor = (1.f - Luminance) + 0.5f;

		GUARD( "AlphaDivLuminance", m_StartedEffect->SetFloat(ShaderInfo.Params[PSP_LuminanceFactor], LuminanceFactor) );
	}

	if (Macros[PSM_HEAT_HAZE] == 1)
	{
		m_PerturbationMap->Load();
		GUARD( "PerturbationMap", m_StartedEffect->SetTexture(ShaderInfo.Params[PSP_PerturbationMap], m_PerturbationMap->GetTexture()) );
	}

	// ======== Pozosta³e bezpoœrednie ustawienia urz¹dzenia ========

	// (brak)

	// ======== Wio! ========

	UINT Foo;
	m_StartedEffect->Begin(&Foo, D3DXFX_DONOTSAVESTATE);
	m_StartedEffect->BeginPass(0);

	ERR_CATCH_FUNC
}

void EngineServices::SetupState_Material(PASS Pass, const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *cam, BaseMaterial &Material, const ENTITY_DRAW_PARAMS &EntityDrawParams, BaseLight *Light)
{
	ERR_TRY;

	m_MainShader->Load();

	BaseMaterial::TYPE MatType = Material.GetType();

	// ======== Sformu³uj makra ========

	WireFrameMaterial *WfMat = NULL;
	SolidMaterial *SolidMat = NULL;
	OpaqueMaterial *OpMat = NULL;
	TranslucentMaterial *TransMat = NULL;
	TerrainMaterial *TerrainMat = NULL;

	DirectionalLight *dir_light = dynamic_cast<DirectionalLight*>(Light);
	PointLight *point_light = dynamic_cast<PointLight*>(Light);
	SpotLight *spot_light = dynamic_cast<SpotLight*>(Light);

	uint Macros[MSM_COUNT];
	ZeroMem(Macros, MSM_COUNT * sizeof(uint));

	Macros[MSM_PASS] = (uint)Pass;

	if (EntityDrawParams.BoneMatrices != NULL)
		Macros[MSM_SKINNING] = 1;

	if (MatType == BaseMaterial::TYPE_OPAQUE || MatType == BaseMaterial::TYPE_TERRAIN)
	{
		SolidMat = static_cast<SolidMaterial*>(&Material);
		OpMat = dynamic_cast<OpaqueMaterial*>(&Material);
		TerrainMat = dynamic_cast<TerrainMaterial*>(&Material);

		if (TerrainMat != NULL)
			Macros[MSM_TERRAIN] = 1;

		if (OpMat != NULL && OpMat->GetAlphaTesting() > 0)
			Macros[MSM_ALPHA_TESTING] = 1;

		if (Pass == PASS_BASE)
		{
			if (!g_Engine->GetConfigBool(Engine::O_LIGHTING) || !EntityDrawParams.UseLighting || SceneDrawParams.SceneObj->GetAmbientColor() == COLORF::WHITE)
				Macros[MSM_AMBIENT_MODE] = 1;
			else if (SceneDrawParams.SceneObj->GetAmbientColor() != COLORF::BLACK)
				Macros[MSM_AMBIENT_MODE] = 2;

			if (OpMat != NULL)
			{
				if (OpMat->GetEmissiveTexture() != NULL)
					Macros[MSM_EMISSIVE] = 1;
				if (OpMat->IsDiffuseTexture())
					Macros[MSM_DIFFUSE_TEXTURE] = 1;
				Macros[MSM_COLOR_MODE] = (uint)OpMat->GetColorMode();
				if (OpMat->GetTextureAnimation())
					Macros[MSM_TEXTURE_ANIMATION] = 1;
				if (OpMat->GetEnvironmentalTexture() != NULL)
					Macros[MSM_ENVIRONMENTAL_MAPPING] = 1;
				if (OpMat->GetFresnelColor() != COLORF::BLACK)
					Macros[MSM_FRESNEL_TERM] = 1;
			}
		}
		else if (Pass == PASS_FOG)
		{
		}
		else if (Pass == PASS_DIRECTIONAL || Pass == PASS_POINT || Pass == PASS_SPOT)
		{
			if (Light->GetHalfLambert() || OpMat != NULL && OpMat->GetHalfLambert())
				Macros[MSM_HALF_LAMBERT] = 1;

			if (OpMat != NULL)
			{
				if (OpMat->IsDiffuseTexture())
					Macros[MSM_DIFFUSE_TEXTURE] = 1;
				Macros[MSM_COLOR_MODE] = (uint)OpMat->GetColorMode();
				if (OpMat->GetTextureAnimation())
					Macros[MSM_TEXTURE_ANIMATION] = 1;
				if (OpMat->GetPerPixel())
					Macros[MSM_PER_PIXEL] = 1;
				if (OpMat->GetPerPixel() && !OpMat->GetNormalTextureName().empty())
					Macros[MSM_NORMAL_TEXTURE] = 1;
				if (Light->GetCastSpecular())
					Macros[MSM_SPECULAR] = (uint)OpMat->GetSpecularMode();
				if (OpMat->GetGlossMapping())
					Macros[MSM_GLOSS_MAP] = 1;
			}

			if (Pass == PASS_SPOT)
			{
				if (spot_light->GetSmooth())
					Macros[MSM_SPOT_SMOOTH] = 1;
			}

			if (SceneDrawParams.ShadowMapTexture != NULL || SceneDrawParams.ShadowMapCubeTexture != NULL)
			{
				if (g_Engine->GetShadowMapFormat() == D3DFMT_A8R8G8B8)
					Macros[MSM_SHADOW_MAP_MODE] = 2;
				else
					Macros[MSM_SHADOW_MAP_MODE] = 1;
			}

			if (Pass == PASS_DIRECTIONAL && Macros[MSM_SHADOW_MAP_MODE] != 0)
			{
				if (SceneDrawParams.UseVariableShadowFactor)
					Macros[MSM_VARIABLE_SHADOW_FACTOR] = 1;
			}

			if (Pass == PASS_POINT && Macros[MSM_SHADOW_MAP_MODE] != 0)
				Macros[MSM_POINT_SHADOW_MAP] = 1;
		}
		else if (Pass == PASS_SHADOW_MAP)
		{
			if (g_Engine->GetShadowMapFormat() == D3DFMT_A8R8G8B8)
				Macros[MSM_SHADOW_MAP_MODE] = 2;
			else
				Macros[MSM_SHADOW_MAP_MODE] = 1;

			if (point_light != NULL)
				Macros[MSM_POINT_SHADOW_MAP] = 1;

			if (Macros[MSM_ALPHA_TESTING])
			{
				assert(OpMat != NULL);
				if (OpMat->IsDiffuseTexture())
					Macros[MSM_DIFFUSE_TEXTURE] = 1;
				if (OpMat->GetTextureAnimation())
					Macros[MSM_TEXTURE_ANIMATION] = 1;
			}
		}
		else
			assert(0);
	}
	else if (MatType == BaseMaterial::TYPE_WIREFRAME)
	{
		assert(Pass == PASS_WIREFRAME);

		WfMat = static_cast<WireFrameMaterial*>(&Material);
		assert(Pass == PASS_WIREFRAME);

		Macros[MSM_COLOR_MODE] = (uint)WfMat->GetColorMode();

		if (SceneDrawParams.SceneObj->GetFogEnabled())
			Macros[MSM_USE_FOG] = 1;
	}
	else if (MatType == BaseMaterial::TYPE_TRANSLUCENT)
	{
		assert(Pass == PASS_TRANSLUCENT);

		SolidMat = static_cast<SolidMaterial*>(&Material);
		TransMat = static_cast<TranslucentMaterial*>(&Material);

		if (TransMat->GetEmissiveTexture() != NULL)
			Macros[MSM_EMISSIVE] = 1;
		if (TransMat->IsDiffuseTexture())
			Macros[MSM_DIFFUSE_TEXTURE] = 1;
		Macros[MSM_COLOR_MODE] = (uint)TransMat->GetColorMode();
		if (TransMat->GetTextureAnimation())
			Macros[MSM_TEXTURE_ANIMATION] = 1;
		if (TransMat->GetEnvironmentalTexture() != NULL)
			Macros[MSM_ENVIRONMENTAL_MAPPING] = 1;
		if (SceneDrawParams.SceneObj->GetFogEnabled())
			Macros[MSM_USE_FOG] = 1;
		Macros[MSM_ALPHA_MODE] = (uint)TransMat->GetAlphaMode();
		if (TransMat->GetFresnelColor() != COLORF::BLACK)
			Macros[MSM_FRESNEL_TERM] = 1;
	}
	else
		assert(0);

	// ======== Pobierz i w razie potrzeby wczytaj efekt o podanym hashu ========

	// SprawdŸ, czy te ustawienia s¹ dozwolone
	//if (!ValidatePpShaderMacros(Macros))
	//{
	//	string Msg;
	//	m_PpShader->MacrosToStr(&Msg, Macros);
	//	throw Error("B³êdna kombinacja ustawieñ shadera preprocessingu: " + Msg);
	//}

	const res::Multishader::SHADER_INFO & ShaderInfo = m_MainShader->GetShader(Macros);

	m_StartedEffect = ShaderInfo.Effect;

	// ======== Wpisz parametry ========

#ifdef DEBUG
	#define GUARD(name, expr) { if (FAILED(expr)) throw Error("Nie mo¿na ustawiæ parametru: " + string(name)); }
#else
	#define GUARD(name, expr) { (expr); }
#endif

	assert(cam != NULL || SceneDrawParams.ShadowMapMatrix != NULL);
	const MATRIX &ViewProj = (cam != NULL ? cam->GetMatrices().GetViewProj() : *SceneDrawParams.ShadowMapMatrix);
	GUARD( "WorldViewProj", m_StartedEffect->SetMatrix(ShaderInfo.Params[MSP_WorldViewProj], &math_cast<D3DXMATRIX>(EntityDrawParams.WorldMatrix*ViewProj)) );

	if (Macros[MSM_SKINNING])
	{
		assert(EntityDrawParams.BoneMatrices != NULL);
		assert(EntityDrawParams.BoneCount > 0);

		GUARD( "BoneMatrices", m_StartedEffect->SetMatrixArray(ShaderInfo.Params[MSP_BoneMatrices], (const D3DXMATRIX*)EntityDrawParams.BoneMatrices, EntityDrawParams.BoneCount) );
	}

	if (MatType == BaseMaterial::TYPE_OPAQUE || MatType == BaseMaterial::TYPE_TERRAIN)
	{
		if (Macros[MSM_TERRAIN] == 1)
		{
			assert(TerrainMat != NULL);

			for (uint ti = 0; ti < TERRAIN_FORMS_PER_PATCH; ti++)
			{
				res::D3dTexture *texture = TerrainMat->Textures[ti];
				texture->Load();
				GUARD( "TerrainTexture#", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_TerrainTexture0+ti], texture->GetBaseTexture()) );
			}

			D3DXVECTOR4 TexScaleVec;
			TexScaleVec.x = TerrainMat->TexScale[0];
			TexScaleVec.y = TerrainMat->TexScale[1];
			TexScaleVec.z = TerrainMat->TexScale[2];
			TexScaleVec.w = TerrainMat->TexScale[3];
			GUARD( "TerrainTexScale", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_TerrainTexScale], &TexScaleVec) );
		}

		if (Pass == PASS_BASE)
		{
			if (Macros[MSM_AMBIENT_MODE] == 2)
			{
				GUARD( "AmbientColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_AmbientColor], &D3DXVECTOR4(SceneDrawParams.SceneObj->GetAmbientColor().R, SceneDrawParams.SceneObj->GetAmbientColor().G, SceneDrawParams.SceneObj->GetAmbientColor().B, SceneDrawParams.SceneObj->GetAmbientColor().A)) );
			}

			if (MatType == BaseMaterial::TYPE_OPAQUE)
			{
				if (Macros[MSM_EMISSIVE] == 1)
				{
					GUARD( "EmissiveTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_EmissiveTexture], OpMat->GetEmissiveTexture()->GetTexture()) );
				}

				if (Macros[MSM_DIFFUSE_TEXTURE] == 1 && (Macros[MSM_AMBIENT_MODE] != 0 || Macros[MSM_ALPHA_TESTING] == 1))
				{
					assert(OpMat->GetDiffuseTexture() != NULL);
					GUARD( "DiffuseTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_DiffuseTexture], OpMat->GetDiffuseTexture()->GetTexture()) );
				}

				if (Macros[MSM_AMBIENT_MODE] != 0 && Macros[MSM_DIFFUSE_TEXTURE] == 0)
				{
					GUARD( "DiffuseColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_DiffuseColor], &D3DXVECTOR4(OpMat->GetDiffuseColor().R, OpMat->GetDiffuseColor().G, OpMat->GetDiffuseColor().B, OpMat->GetDiffuseColor().A)) );
				}

				if (Macros[MSM_TEXTURE_ANIMATION] == 1)
				{
					GUARD( "TextureMatrix", m_StartedEffect->SetMatrix(ShaderInfo.Params[MSP_TextureMatrix], &math_cast<D3DXMATRIX>(EntityDrawParams.TextureMatrix)) );
				}

				if (Macros[MSM_ENVIRONMENTAL_MAPPING] == 1)
				{
					assert(OpMat->GetEnvironmentalTexture() != NULL);
					GUARD( "EnvironmentalTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_EnvironmentalTexture], OpMat->GetEnvironmentalTexture()->GetCubeTexture()) );
				}

				if (Macros[MSM_FRESNEL_TERM] == 1)
				{
					GUARD( "FresnelColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_FresnelColor], &D3DXVECTOR4(OpMat->GetFresnelColor().R, OpMat->GetFresnelColor().G, OpMat->GetFresnelColor().B, OpMat->GetFresnelColor().A)) );
					GUARD( "FresnelPower", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_FresnelPower], OpMat->GetFresnelPower()) );
				}
			}
		}
		else if (Pass == PASS_FOG)
		{
		}
		else if (Pass == PASS_DIRECTIONAL || Pass == PASS_POINT || Pass == PASS_SPOT)
		{
			if (Macros[MSM_SHADOW_MAP_MODE] != 0)
			{
				assert(SceneDrawParams.ShadowMapTexture != NULL || SceneDrawParams.ShadowMapCubeTexture != NULL);
				assert(Light != NULL);

				float ShadowMapSize = 0.f;

				if (SceneDrawParams.ShadowMapTexture != NULL)
				{
					assert(SceneDrawParams.ShadowMapTexture->IsLoaded());
					GUARD( "ShadowMapTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_ShadowMapTexture], SceneDrawParams.ShadowMapTexture->GetTexture()) );
					ShadowMapSize = (float)SceneDrawParams.ShadowMapTexture->GetWidth();
				}
				else
				{
					assert(SceneDrawParams.ShadowMapCubeTexture->IsLoaded());
					GUARD( "ShadowMapTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_ShadowMapTexture], SceneDrawParams.ShadowMapCubeTexture->GetTexture()) );
					ShadowMapSize = (float)SceneDrawParams.ShadowMapCubeTexture->GetSize();
				}

				float ShadowMapSizeRcp = 1.f / ShadowMapSize;

				GUARD( "ShadowMapMatrix", m_StartedEffect->SetMatrix(ShaderInfo.Params[MSP_ShadowMapMatrix], &math_cast<D3DXMATRIX>(EntityDrawParams.WorldMatrix*(*SceneDrawParams.ShadowMapMatrix))) );
				GUARD( "ShadowMapSize", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_ShadowMapSize], ShadowMapSize) );
				GUARD( "ShadowMapSizeRcp", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_ShadowMapSizeRcp], ShadowMapSizeRcp) );
				GUARD( "ShadowFactor", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_ShadowFactor], Light->GetShadowFactor()) );

				if (Macros[MSM_VARIABLE_SHADOW_FACTOR] == 1)
				{
					float Inv_End_Minus_Start = 1.f / (SceneDrawParams.ShadowFactorAttnEnd - SceneDrawParams.ShadowFactorAttnStart);
					float A = Inv_End_Minus_Start;
					float B = - SceneDrawParams.ShadowFactorAttnStart * Inv_End_Minus_Start;
					GUARD( "ShadowFactorA", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_ShadowFactorA], A) );
					GUARD( "ShadowFactorB", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_ShadowFactorB], B) );
				}

				if (Pass == PASS_POINT)
				{
					GUARD( "ShadowEpsilon", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_ShadowEpsilon], g_Engine->GetConfigFloat(Engine::O_SM_EPSILON)) );
					GUARD( "LightRangeSq_World", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_LightRangeSq_World], point_light->GetDist()*point_light->GetDist()) );
				}
			}

			if (MatType == BaseMaterial::TYPE_OPAQUE)
			{
				if (Macros[MSM_DIFFUSE_TEXTURE] == 1)
				{
					assert(OpMat->GetDiffuseTexture() != NULL);
					GUARD( "DiffuseTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_DiffuseTexture], OpMat->GetDiffuseTexture()->GetTexture()) );
				}

				if (Macros[MSM_DIFFUSE_TEXTURE] == 0)
					GUARD( "DiffuseColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_DiffuseColor], &D3DXVECTOR4(OpMat->GetDiffuseColor().R, OpMat->GetDiffuseColor().G, OpMat->GetDiffuseColor().B, OpMat->GetDiffuseColor().A)) );

				if (Macros[MSM_TEXTURE_ANIMATION] == 1)
					GUARD( "TextureMatrix", m_StartedEffect->SetMatrix(ShaderInfo.Params[MSP_TextureMatrix], &math_cast<D3DXMATRIX>(EntityDrawParams.TextureMatrix)) );

				if (Macros[MSM_NORMAL_TEXTURE] == 1)
				{
					assert(OpMat->GetNormalTexture() != NULL);
					GUARD( "NormalTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_NormalTexture], OpMat->GetNormalTexture()->GetTexture()) );
				}

				if (Macros[MSM_SPECULAR] != 0)
				{
					GUARD( "SpecularColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_SpecularColor], &D3DXVECTOR4(OpMat->GetSpecularColor().R, OpMat->GetSpecularColor().G, OpMat->GetSpecularColor().B, OpMat->GetSpecularColor().A)) );
					GUARD( "SpecularPower", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_SpecularPower], OpMat->GetSpecularPower()) );
				}
			}

			GUARD( "LightColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_LightColor], &D3DXVECTOR4(Light->GetColor().R, Light->GetColor().G, Light->GetColor().B, Light->GetColor().A)) );

			if (Pass == PASS_DIRECTIONAL)
			{
				VEC3 DirToLight_Model;
				TransformNormal(&DirToLight_Model, -dir_light->GetDir(), EntityDrawParams.WorldInvMatrix);
				Normalize(&DirToLight_Model);
				GUARD( "DirToLight", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_DirToLight], &D3DXVECTOR4(DirToLight_Model.x, DirToLight_Model.y, DirToLight_Model.z, 0.f)) );
			}
			else if (Pass == PASS_POINT)
			{
				VEC3 LightPos_Model;
				Transform(&LightPos_Model, point_light->GetPos(), EntityDrawParams.WorldInvMatrix);
				GUARD( "LightPos", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_LightPos], &D3DXVECTOR4(LightPos_Model.x, LightPos_Model.y, LightPos_Model.z, 1.f)) );

				float LightRange_Model = point_light->GetDist() / EntityDrawParams.WorldSize;
				GUARD( "LightRangeSq", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_LightRangeSq], LightRange_Model*LightRange_Model) );
			}
			else if (Pass == PASS_SPOT)
			{
				VEC3 DirToLight_Model;
				TransformNormal(&DirToLight_Model, -spot_light->GetDir(), EntityDrawParams.WorldInvMatrix);
				Normalize(&DirToLight_Model);
				GUARD( "DirToLight", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_DirToLight], &D3DXVECTOR4(DirToLight_Model.x, DirToLight_Model.y, DirToLight_Model.z, 0.f)) );

				VEC3 LightPos_Model;
				Transform(&LightPos_Model, spot_light->GetPos(), EntityDrawParams.WorldInvMatrix);
				GUARD( "LightPos", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_LightPos], &D3DXVECTOR4(LightPos_Model.x, LightPos_Model.y, LightPos_Model.z, 1.f)) );

				float LightRange_Model = spot_light->GetDist() / EntityDrawParams.WorldSize;
				GUARD( "LightRangeSq", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_LightRangeSq], LightRange_Model*LightRange_Model) );

				float LightCosFov2 = cosf(spot_light->GetFov() * 0.5f);
				float LightCosFov2Factor = 1.f / (1.f - LightCosFov2);
				GUARD( "LightCosFov2", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_LightCosFov2], LightCosFov2) );
				GUARD( "LightCosFov2Factor", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_LightCosFov2Factor], LightCosFov2Factor) );
			}
			else
				assert(0);
		}
		else if (Pass == PASS_SHADOW_MAP)
		{
			assert(MatType == BaseMaterial::TYPE_OPAQUE);

			if (Macros[MSM_POINT_SHADOW_MAP] == 1)
			{
				VEC3 LightPos_Model;
				Transform(&LightPos_Model, point_light->GetPos(), EntityDrawParams.WorldInvMatrix);
				GUARD( "LightPos", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_LightPos], &D3DXVECTOR4(LightPos_Model.x, LightPos_Model.y, LightPos_Model.z, 1.f)) );

				float LightRange_Model = point_light->GetDist() / EntityDrawParams.WorldSize;
				GUARD( "LightRangeSq", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_LightRangeSq], LightRange_Model*LightRange_Model) );
			}

			if (Macros[MSM_ALPHA_TESTING] == 1)
			{
				if (Macros[MSM_DIFFUSE_TEXTURE] == 1)
				{
					assert(OpMat->GetDiffuseTexture() != NULL);
					GUARD( "DiffuseTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_DiffuseTexture], OpMat->GetDiffuseTexture()->GetTexture()) );
				}
				else // (Macros[MSM_DIFFUSE_TEXTURE] == 0)
					GUARD( "DiffuseColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_DiffuseColor], &D3DXVECTOR4(OpMat->GetDiffuseColor().R, OpMat->GetDiffuseColor().G, OpMat->GetDiffuseColor().B, OpMat->GetDiffuseColor().A)) );
				if (Macros[MSM_TEXTURE_ANIMATION] == 1)
					GUARD( "TextureMatrix", m_StartedEffect->SetMatrix(ShaderInfo.Params[MSP_TextureMatrix], &math_cast<D3DXMATRIX>(EntityDrawParams.TextureMatrix)) );
			}
		}
		else
			assert(0);
	}
	else if (MatType == BaseMaterial::TYPE_WIREFRAME)
	{
		GUARD( "DiffuseColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_DiffuseColor], &D3DXVECTOR4(WfMat->GetColor().R, WfMat->GetColor().G, WfMat->GetColor().B, WfMat->GetColor().A)) );
	}
	else if (MatType == BaseMaterial::TYPE_TRANSLUCENT)
	{
		if (Macros[MSM_EMISSIVE] == 1)
			GUARD( "EmissiveTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_EmissiveTexture], TransMat->GetEmissiveTexture()->GetTexture()) );
		if (Macros[MSM_DIFFUSE_TEXTURE] == 1)
		{
			assert(TransMat->GetDiffuseTexture() != NULL);
			GUARD( "DiffuseTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_DiffuseTexture], TransMat->GetDiffuseTexture()->GetTexture()) );
		}
		if (Macros[MSM_DIFFUSE_TEXTURE] == 0)
			GUARD( "DiffuseColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_DiffuseColor], &D3DXVECTOR4(TransMat->GetDiffuseColor().R, TransMat->GetDiffuseColor().G, TransMat->GetDiffuseColor().B, TransMat->GetDiffuseColor().A)) );
		if (Macros[MSM_TEXTURE_ANIMATION] == 1)
			GUARD( "TextureMatrix", m_StartedEffect->SetMatrix(ShaderInfo.Params[MSP_TextureMatrix], &math_cast<D3DXMATRIX>(EntityDrawParams.TextureMatrix)) );
		if (Macros[MSM_ENVIRONMENTAL_MAPPING] == 1)
		{
			assert(TransMat->GetEnvironmentalTexture() != NULL);
			GUARD( "EnvironmentalTexture", m_StartedEffect->SetTexture(ShaderInfo.Params[MSP_EnvironmentalTexture], TransMat->GetEnvironmentalTexture()->GetCubeTexture()) );
		}
		if (Macros[MSM_FRESNEL_TERM] == 1)
		{
			GUARD( "FresnelColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_FresnelColor], &D3DXVECTOR4(TransMat->GetFresnelColor().R, TransMat->GetFresnelColor().G, TransMat->GetFresnelColor().B, TransMat->GetFresnelColor().A)) );
			GUARD( "FresnelPower", m_StartedEffect->SetFloat(ShaderInfo.Params[MSP_FresnelPower], TransMat->GetFresnelPower()) );
		}
	}
	else
		assert(0);

	// TeamColor
	if ((MatType == BaseMaterial::TYPE_OPAQUE && Macros[MSM_AMBIENT_MODE] != 0 && Macros[MSM_COLOR_MODE] != 0) ||
		(Pass == PASS_WIREFRAME && Macros[MSM_COLOR_MODE] != 0) ||
		(MatType == BaseMaterial::TYPE_TRANSLUCENT && (Macros[MSM_COLOR_MODE] != 0 || Macros[MSM_ALPHA_MODE] != 0)) ||
		((Pass == PASS_DIRECTIONAL || Pass == PASS_POINT || Pass == PASS_SPOT) && Macros[MSM_COLOR_MODE] != 0))
	{
		GUARD( "TeamColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_TeamColor], &D3DXVECTOR4(EntityDrawParams.TeamColor.R, EntityDrawParams.TeamColor.G, EntityDrawParams.TeamColor.B, EntityDrawParams.TeamColor.A)) );
	}

	// Mg³a
	if (Pass == PASS_FOG ||
		(Pass == PASS_WIREFRAME && Macros[MSM_USE_FOG] == 1) ||
		(MatType == BaseMaterial::TYPE_TRANSLUCENT && Macros[MSM_USE_FOG] == 1))
	{
		assert(SceneDrawParams.SceneObj->GetFogEnabled());
		assert(cam != NULL);

		GUARD( "FogColor", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_FogColor], &D3DXVECTOR4(SceneDrawParams.SceneObj->GetFogColor().R, SceneDrawParams.SceneObj->GetFogColor().G, SceneDrawParams.SceneObj->GetFogColor().B, SceneDrawParams.SceneObj->GetFogColor().A)) );

		float One_Minus_FogStart = 1.f - SceneDrawParams.SceneObj->GetFogStart();
		float FogA = 1.f / (cam->GetZFar() * One_Minus_FogStart);
		float FogB = - SceneDrawParams.SceneObj->GetFogStart() / One_Minus_FogStart;
		GUARD( "FogFactors", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_FogFactors], &D3DXVECTOR4(FogA, FogB, 0.f, 0.f)) );
	}

	// CameraPos
	if (Macros[MSM_ENVIRONMENTAL_MAPPING] == 1 || Macros[MSM_FRESNEL_TERM] == 1 || Macros[MSM_SPECULAR] != 0)
	{
		assert(cam != NULL);

		VEC3 CameraPos_Model;
		Transform(&CameraPos_Model, cam->GetEyePos(), EntityDrawParams.WorldInvMatrix);
		GUARD( "CameraPos", m_StartedEffect->SetVector(ShaderInfo.Params[MSP_CameraPos], &D3DXVECTOR4(CameraPos_Model.x, CameraPos_Model.y, CameraPos_Model.z, 1.f)) );
	}

	// ======== Pozosta³e bezpoœrednie ustawienia urz¹dzenia ========

	// ZEnable
	frame::Dev->SetRenderState(D3DRS_ZENABLE, TRUE);

	// FillMode
	if (MatType == BaseMaterial::TYPE_WIREFRAME)
		frame::Dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	else
		frame::Dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	//Backface-culling
	if (MatType == BaseMaterial::TYPE_TERRAIN)
		frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	if (MatType == BaseMaterial::TYPE_OPAQUE || MatType == BaseMaterial::TYPE_TRANSLUCENT)
	{
		if (SolidMat->GetTwoSided())
			frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		else
			frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	}
	else if (MatType == BaseMaterial::TYPE_WIREFRAME)
		frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// Alpha-testing i ZFunc
	if (MatType == BaseMaterial::TYPE_OPAQUE || MatType == BaseMaterial::TYPE_TERRAIN)
	{
		if (Macros[MSM_ALPHA_TESTING] == 1)
		{
			// Przebieg wype³niaj¹cy - prawdziwy Alpha-Testing
			if (Pass == PASS_BASE || Pass == PASS_SHADOW_MAP)
			{
				frame::Dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
				frame::Dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
				frame::Dev->SetRenderState(D3DRS_ALPHAREF, (uint4)OpMat->GetAlphaTesting());
				frame::Dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
			}
			// Przebieg wtórny - sztuczka! - porównywanie Z equal, a za to alfa-testing wy³¹czony
			else // Pass == PASS_FOG || Pass == PASS_DIRECTIONAL || Pass == PASS_POINT || Pass == PASS_SPOT
			{
				frame::Dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
				frame::Dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
			}
		}
		else
		{
			frame::Dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
			frame::Dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		}
	}
	else // TYPE_WIREFRAME || TYPE_TRANSLUCENT
	{
		frame::Dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		frame::Dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	}

	// Alpha-blending
	if (MatType == BaseMaterial::TYPE_OPAQUE || MatType == BaseMaterial::TYPE_TERRAIN)
	{
		frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		// Przebieg bazowy - wype³nij
		if (Pass == PASS_BASE || Pass == PASS_SHADOW_MAP)
			frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		// Mg³a - blenduj
		else if (Pass == PASS_FOG)
		{
			frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		}
		// Œwiat³o - 1 + 1
		else if (Pass == PASS_DIRECTIONAL || Pass == PASS_POINT || Pass == PASS_SPOT)
		{
			frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		}
		else
			assert(0);
	}
	else if (MatType == BaseMaterial::TYPE_WIREFRAME || MatType == BaseMaterial::TYPE_TRANSLUCENT)
	{
		BaseMaterial::BLEND_MODE BlendMode;
		if (MatType == BaseMaterial::TYPE_WIREFRAME)
			BlendMode = WfMat->GetBlendMode();
		else
			BlendMode = TransMat->GetBlendMode();

		switch (BlendMode)
		{
		case WireFrameMaterial::BLEND_LERP:
			frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			break;
		case WireFrameMaterial::BLEND_ADD:
			frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		case WireFrameMaterial::BLEND_SUB:
			frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
			frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		default:
			assert(0);
		}
	}
	else
		assert(0);

	// ZWriteEnable
	if (MatType == BaseMaterial::TYPE_OPAQUE || MatType == BaseMaterial::TYPE_TERRAIN)
	{
		if (Pass == PASS_BASE || Pass == PASS_SHADOW_MAP)
			frame::Dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		else if (Pass == PASS_FOG || Pass == PASS_DIRECTIONAL || Pass == PASS_POINT || Pass == PASS_SPOT)
			frame::Dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		else
			assert(0);
	}
	else if (MatType == BaseMaterial::TYPE_WIREFRAME || MatType == BaseMaterial::TYPE_TRANSLUCENT)
		frame::Dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	else
		assert(0);

	// ======== Wio! ========

	UINT Foo;
	m_StartedEffect->Begin(&Foo, D3DXFX_DONOTSAVESTATE);
	m_StartedEffect->BeginPass(0);

	ERR_CATCH_FUNC;
}

void EngineServices::UnsetupState()
{
	assert(m_StartedEffect != NULL);

	if (m_StartedEffect)
	{
		m_StartedEffect->EndPass();
		m_StartedEffect->End();
		m_StartedEffect = NULL;

		// Bo shadow mapa pozostaje zbindowana podczas gdy jest u¿ywana jako render target
		frame::Dev->SetTexture(0, NULL);
		frame::Dev->SetTexture(1, NULL);
		frame::Dev->SetTexture(2, NULL);
	}
}

void EngineServices::SetupState_Heat(const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam, HeatEntity *heat_entity)
{
	ERR_TRY

	// Pobierz efekt
	if (m_HeatEffect == NULL)
		m_HeatEffect = res::g_Manager->MustGetResourceEx<res::D3dEffect>("HeatEffect");
	m_HeatEffect->Load();

	m_StartedEffect = m_HeatEffect->Get();

	// Wpisz parametry
#ifdef DEBUG
	#define GUARD(name, expr) { if (FAILED(expr)) throw Error("Nie mo¿na ustawiæ parametru: " + string(name)); }
#else
	#define GUARD(name, expr) { (expr); }
#endif

	GUARD( "WorldViewProj", m_StartedEffect->SetMatrix(m_HeatEffect->GetParam(HEP_WorldViewProj), &math_cast<D3DXMATRIX>(heat_entity->GetWorldMatrix() * Cam->GetMatrices().GetViewProj())) );
	GUARD( "ZFar", m_StartedEffect->SetFloat(m_HeatEffect->GetParam(HEP_ZFar), Cam->GetZFar() * 0.5f) );

	// Ten DirToCam to tak naprawdê odwrócony kierunek patrzenia kamery, równoleg³y (kierunkowy).
	// Nie bierze pod uwagê pozycji encji ani tym bardziej pojedynczych jej wierzcho³ków.
	VEC3 DirToCam_model;
	TransformNormal(&DirToCam_model, -Cam->GetForwardDir(), heat_entity->GetInvWorldMatrix());
	Normalize(&DirToCam_model);
	GUARD( "DirToCam", m_StartedEffect->SetVector(m_HeatEffect->GetParam(HEP_DirToCam), &D3DXVECTOR4(DirToCam_model.x, DirToCam_model.y, DirToCam_model.z, 0.f)) );

#undef GUARD

	// Wio!

	UINT Foo;
	m_StartedEffect->Begin(&Foo, D3DXFX_DONOTSAVESTATE);
	m_StartedEffect->BeginPass(0);

	ERR_CATCH_FUNC
}

void EngineServices::Update()
{
	float Time = frame::Timer1.GetTime();

	// Porz¹dki
	if (Time > m_MaintenanceLastTime + MAINTENANCE_TIME)
	{
		Maintenance();
		m_MaintenanceLastTime = Time;
	}
}

void EngineServices::CreateSpecialTextures()
{
	assert(m_SpecialTextures[ST_SCREEN] == NULL);

	m_SpecialTextures[ST_SCREEN].reset(new res::D3dTextureSurface(
		"Engine_Special_Screen", string(), true, 0, 0, D3DPOOL_DEFAULT, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8));
	m_SpecialTextures[ST_TONE_MAPPING_VRAM].reset(new res::D3dTextureSurface(
		"Engine_Special_ToneMapping_VRAM", string(), false, 0, 0, D3DPOOL_DEFAULT, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8));
	m_SpecialTextures[ST_TONE_MAPPING_RAM].reset(new res::D3dTextureSurface(
		"Engine_Special_ToneMapping_RAM", string(), false, 0, 0, D3DPOOL_SYSTEMMEM, 0, D3DFMT_A8R8G8B8));
	m_SpecialTextures[ST_BLUR_1].reset(new res::D3dTextureSurface(
		"Engine_Special_Blur_1", string(), true, 0, 0, D3DPOOL_DEFAULT, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8));
	m_SpecialTextures[ST_BLUR_2].reset(new res::D3dTextureSurface(
		"Engine_Special_Blur_2", string(), true, 0, 0, D3DPOOL_DEFAULT, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8));
	m_SpecialTextures[ST_FEEDBACK].reset(new res::D3dTextureSurface(
		"Engine_Special_Feedback", string(), true, 0, 0, D3DPOOL_DEFAULT, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8));
}

void EngineServices::DestroySpecialTextures()
{
	for (size_t i = ST_COUNT; i--; )
		m_SpecialTextures[i].reset();
}

void EngineServices::SetSpecialTexturesSize()
{
	uint4 ScreenCX = frame::Settings.BackBufferWidth;
	uint4 ScreenCY = frame::Settings.BackBufferHeight;

	uint4 ToneMappingCX = std::max(4u, ScreenCX/ST_TONE_MAPPING_DIV);
	uint4 ToneMappingCY = std::max(4u, ScreenCY/ST_TONE_MAPPING_DIV);

	if (m_SpecialTextures[ST_SCREEN] != NULL)
		m_SpecialTextures[ST_SCREEN]->SetSize(ScreenCX, ScreenCY);
	if (m_SpecialTextures[ST_TONE_MAPPING_VRAM] != NULL)
		m_SpecialTextures[ST_TONE_MAPPING_VRAM]->SetSize(ToneMappingCX, ToneMappingCY);
	if (m_SpecialTextures[ST_TONE_MAPPING_RAM] != NULL)
		m_SpecialTextures[ST_TONE_MAPPING_RAM]->SetSize(ToneMappingCX, ToneMappingCY);
	if (m_SpecialTextures[ST_BLUR_1] != NULL)
		m_SpecialTextures[ST_BLUR_1]->SetSize(ScreenCX/ST_BLUR_DIV, ScreenCY/ST_BLUR_DIV);
	if (m_SpecialTextures[ST_BLUR_2] != NULL)
		m_SpecialTextures[ST_BLUR_2]->SetSize(ScreenCX/ST_BLUR_DIV, ScreenCY/ST_BLUR_DIV);
	if (m_SpecialTextures[ST_FEEDBACK] != NULL)
		m_SpecialTextures[ST_FEEDBACK]->SetSize(ScreenCX/ST_FEEDBACK_DIV, ScreenCY/ST_FEEDBACK_DIV);
}

void EngineServices::Maintenance()
{
	float Time = frame::Timer1.GetTime();

	// Kasowanie tekstur specjalnych
	for (size_t i = 0; i < ST_COUNT; i++)
	{
		if (m_SpecialTextures[i] != NULL && m_SpecialTextures[i]->IsLoaded())
		{
			if (Time > m_SpecialTexturesLastUseTime[i] + UNUSED_TEXTURE_FREE_TIME)
				m_SpecialTextures[i]->Unload();
		}
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa RunningOptimizer

// Co ile klatek jest klatka eksperymentalna
const uint EXPERIMENTAL_FRAME_PERIOD = 16;
// Ile czasów ostatnich klatek normalnych ma byæ pamiêtane do wyci¹gania œredniej
const uint NORMAL_FRAME_TIME_SAMPLE_COUNT = 16;
// Ile razy trzeba przetestowaæ zmianê opcji, ¿eby stwiedziæ czy op³aca siê j¹ przestawiaæ
const uint FLIP_TEST_COUNT = 2;

RunningOptimizer::RunningOptimizer() :
	m_FrameNumber(0),
	m_SettingTestIndex(0),
	m_NormalFrameTimeSamples(NORMAL_FRAME_TIME_SAMPLE_COUNT)
{
	for (uint i = 0; i < SETTING_COUNT; i++)
	{
		m_CurrentSettings[i] = true;
		m_FlipSums[i] = 0;
		m_FlipCounts[i] = 0;
	}
}

void RunningOptimizer::OnFrame(SETTINGS *OutNewSettings, float LastFrameTime)
{
	// Poprzednia klatka

	// By³a eksperymentalna
	if (m_FrameNumber % EXPERIMENTAL_FRAME_PERIOD == 0)
	{
		// SprawdŸ i zapisz czy op³aca siê przestawiæ t¹ opcjê
		int WorthFlip = (LastFrameTime < m_NormalFrameTimeSamples.GetAverage() ? 1 : -1);
		m_FlipSums[m_SettingTestIndex] += WorthFlip;
		m_FlipCounts[m_SettingTestIndex]++;

		// Jeœli zebra³o siê dostatecznie du¿o testów przestawiania tego ustawienia
		if (m_FlipCounts[m_SettingTestIndex] == FLIP_TEST_COUNT)
		{
			// Jeœli warto j¹ przestawiæ - przestaw
			if (m_FlipSums[m_SettingTestIndex] > 0)
				m_CurrentSettings.flip(m_SettingTestIndex);
			// Wyzeruj liczniki testowania przestawiania dla tego ustawienia
			m_FlipSums[m_SettingTestIndex] = 0;
			m_FlipCounts[m_SettingTestIndex] = 0;
		}

		// Nastêpna bêdzie testowana kolejna opcja
		m_SettingTestIndex = (m_SettingTestIndex + 1) % SETTING_COUNT;
	}
	// By³a zwyczajna
	else
	{
		// Zapisz czas zwyczajnej klatki do liczenia œredniej
		m_NormalFrameTimeSamples.AddSample(LastFrameTime);
	}

	// Nowa klatka

	m_FrameNumber++;

	// Jest eksperymentalna
	if (m_FrameNumber % EXPERIMENTAL_FRAME_PERIOD == 0)
	{
		// Zwróæ ustawienia bie¿¹ce
		*OutNewSettings = m_CurrentSettings;
		// Ale ze zmienion¹ konkretn¹ opcj¹
		OutNewSettings->flip(m_SettingTestIndex);
	}
	// Jest zwyczajna
	else
	{
		// Zwróæ ustawienia bie¿¹ce
		*OutNewSettings = m_CurrentSettings;
	}
}

void RunningOptimizer::SettingsToStr(string *Out, const SETTINGS &Settings)
{
	Out->resize(SETTING_COUNT);
	(*Out)[0] = (Settings[SETTING_MATERIAL_SORT] ? 'S' : '-');
	(*Out)[1] = (Settings[SETTING_ENTITY_OCCLUSION_QUERY] ? 'E' : '-');
	(*Out)[2] = (Settings[SETTING_LIGHT_OCCLUSION_QUERY] ? 'L' : '-');
	(*Out)[3] = (Settings[SETTING_TREE_FRUSTUM_CULLING] ? 'T' : '-');
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa EntityOctree

/*
Oto jak bêdzie wygl¹da³o zwalnianie sceny i wszystkiego co w niej (projekt):
- Wywo³uje siê Scene.dtor
- Scena ustawia sobie Destroying = true
- Scena usuwa wszystkie encje g³ównego poziomu (Root) - wywo³uje delete
- Dtor usuwanych encji wywo³uje Scene.UnregisterEntity, ale ze wzglêdu na
  Destroying = true nie powoduje to usuwania z drzewa Octree wszystkich encji.
- Dtor usuwanych encji wywo³uje rekurencyjnie delete encji podrzêdnych. W ten
  sposób usuwane s¹ wszystkie encje sceny.
- Drzewo Octree przechowuje teraz niewa¿ne wskaŸniki wszystkich encji, bo nie
  by³o aktualizowane podczas ich usuwania. Lista RootEntities w scenie podobnie.
- Usuwane jest drzewo Octree, które po prostu zwalnia sobie wszystkie wêz³y
  ignoruj¹c listy niewa¿nych ju¿ wskaŸników do encji w ka¿dym wêŸle.
*/

const uint ENTITY_OCTREE_DYNAMIC_NODE_FREELIST_BLOCK_CAPACITY = 50;
// Minimalna liczba encji w wêŸle, od której podejmowana jest próba rozbicia wêz³a na podwêz³y
const uint ENTITY_OCTREE_SPLIT_ENTITY_COUNT = 16;
// Maksymalna liczba encji w wêŸle i jego podwêz³¹ch, poni¿ej której podejmowana jest próba po³¹czenia wêz³a z jego podwêz³ami
const uint ENTITY_OCTREE_JOIN_ENTITY_COUNT = 8;
// Parametr rozszerzenia wielkoœci granic podwêz³ów swobodnego drzewa ósemkowego (0..1)
const float ENTITY_OCTREE_K = 0.25f;

struct ENTITY_OCTREE_NODE
{
	BOX Bounds;
	ENTITY_OCTREE_NODE *Parent;
	ENTITY_OCTREE_NODE *SubNodes[8]; // Albo wszystkie niezerowe albo ¿aden
	ENTITY_VECTOR Entities;
};

class EntityOctree
{
public:
	EntityOctree(const BOX &Bounds);
	~EntityOctree();

	void AddEntity(Entity *e);
	void RemoveEntity(Entity *e);
	void OnEntityParamsChange(Entity *e);

	// Znajduje i zwraca wszystkie encje koliduj¹ce z danym obiektem geometrycznym
	// - Tylko widoczne!
	void FindEntities_Frustum(ENTITY_VECTOR *OutEntities, const FRUSTUM_PLANES &Frustum);
	// Znajduje encje koliduj¹ce z zasiêgiem podanego œwiat³a Spot, które s¹ Visible, MaterialEntity i CastShadow
	void FindEntities_SpotLight(ENTITY_VECTOR *OutEntities, SpotLight &spot_light);
	// Znajduje encje koliduj¹ce z zasiêgiem podanego œwiat³a Point, które s¹ Visible, MaterialEntity i CastShadow
	void FindEntities_PointLight(ENTITY_VECTOR OutEntities[6], PointLight &point_light);
	// Znajduje encje koliduj¹ce z zasiêgiem podanego œwiat³a Directional, które s¹ Visible, MaterialEntity lub TreeEntity i CastShadow
	void FindEntities_DirectionalLight(ENTITY_VECTOR *OutEntities, const FRUSTUM_PLANES &CamFrustum, const BOX &CamBox, const VEC3 &LightDir);
	// Kolizja encji z promieniem (tylko do przodu)
	bool RayCollision(Entity **OutEntity, float *OutT, COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir);

private:
	DynamicFreeList<ENTITY_OCTREE_NODE> m_Memory;
	ENTITY_OCTREE_NODE *m_Root;

	// Wyznacza AABB podwêz³ów swobodneo drzewa ósemkowego
	void BuildSubBounds(BOX OutSubBounds[8], const BOX &Bounds);
	void DeleteNode(ENTITY_OCTREE_NODE *Node); // rekurencyjna
	void AddEntityToNode(ENTITY_OCTREE_NODE *Node, Entity *e, const VEC3 &SphereCenter, float SphereRadius);
	void SplitNode(ENTITY_OCTREE_NODE *Node);
	void TryJoin(ENTITY_OCTREE_NODE *Node);
	// - Tylko widoczne!
	void FindEntities_Frustum_Node(ENTITY_OCTREE_NODE *Node, ENTITY_VECTOR *OutEntities, const FRUSTUM_PLANES &Frustum, bool Inside);
	void FindEntities_SpotLight_Node(ENTITY_OCTREE_NODE *Node, ENTITY_VECTOR *OutEntities, const FRUSTUM_PLANES &Frustum, bool Inside);
	void FindEntities_PointLight_Node(ENTITY_OCTREE_NODE *Node, ENTITY_VECTOR OutEntities[8], const VEC3 &Pos, const float Dist, const FRUSTUM_PLANES *Frustums[6]);
	void FindEntities_DirectionalLight_Node(ENTITY_OCTREE_NODE *Node, ENTITY_VECTOR *OutEntities, const FRUSTUM_PLANES &CamFrustum, const BOX &CamBox, const VEC3 &LightDir);
	bool RayCollision_Node(Entity **OutEntity, float *InOutT, ENTITY_OCTREE_NODE *Node, COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir);
};


EntityOctree::EntityOctree(const BOX &Bounds) :
	m_Memory(ENTITY_OCTREE_DYNAMIC_NODE_FREELIST_BLOCK_CAPACITY),
	m_Root(NULL)
{
	m_Root = m_Memory.New();
	m_Root->Bounds = Bounds;
	m_Root->Parent = NULL;
	for (uint si = 0; si < 8; si++)
		m_Root->SubNodes[si] = NULL;
}

EntityOctree::~EntityOctree()
{
	// Rekurencyjnie zwolnij wêz³y drzewa, zignoruj zawarte w nich wskaŸniki do encji
	DeleteNode(m_Root);
}

void EntityOctree::AddEntity(Entity *e)
{
	// Pobierz bounding sphere
	VEC3 SphereCenter = e->GetWorldPos();
	float SphereRadius = e->GetWorldRadius();

	AddEntityToNode(m_Root, e, SphereCenter, SphereRadius);
}

void EntityOctree::RemoveEntity(Entity *e)
{
	// Uwaga! Byæ mo¿e encja e jest w tym momencie usuwana - nie korzystaæ z jej metod wirtualnych.

	ENTITY_OCTREE_NODE *Node = e->GetOctreeNode();
	assert(Node != NULL);

	// Usuñ t¹ encjê z listy wêz³a drzewa, do którego nale¿y
	ENTITY_VECTOR::iterator eit = std::find(Node->Entities.begin(), Node->Entities.end(), e);
	assert(eit != Node->Entities.end());
	Node->Entities.erase(eit);

	// SprawdŸ, czy nie warto po³¹czyæ podwêz³ów wêz³a nadrzêdnego do tego
	if (Node->Parent != NULL)
		TryJoin(Node->Parent);
}

void EntityOctree::OnEntityParamsChange(Entity *e)
{
	VEC3 SphereCenter = e->GetWorldPos();
	float SphereRadius = e->GetWorldRadius();
	ENTITY_OCTREE_NODE *Node = e->GetOctreeNode();
	// Encja nadal zawiera siê w swoim wêŸle
	if (SphereInBox(SphereCenter, SphereRadius, Node->Bounds))
	{
		// Jedyne co mo¿na spróbowaæ zrobiæ, to przenieœæ j¹ do jednego z podwêz³ów,
		// jeœli takie istniej¹ i jeœli zawiera siê w ca³oœci w jednym z nich
		if (Node->SubNodes[0] != NULL)
		{
			for (uint si = 0; si < 8; si++)
			{
				if (SphereInBox(SphereCenter, SphereRadius, Node->SubNodes[si]->Bounds))
				{
					// Usuñ encjê z listy encji tego wêz³a
					ENTITY_VECTOR::iterator eit = std::find(Node->Entities.begin(), Node->Entities.end(), e);
					assert(eit != Node->Entities.end());
					Node->Entities.erase(eit);

					// Rekurencyjnie dodaj t¹ encjê gdzieœ do poddrzewa tego podwêz³a
					// (Ta funkcja sama wywo³a e->SetOctreeNode.)
					AddEntityToNode(Node->SubNodes[si], e, SphereCenter, SphereRadius);

					break;
				}
			}
			// Jeœli pêtla przejdzie bez przerwania i encja nie trafi do ¿adnego z podwêz³ów, to nic.
		}
	}
	// Encja przesta³a zawieraæ siê w swoim wêŸle
	else
	{
		// ZnajdŸ najni¿szy wêze³ na œcie¿ce od bie¿¹cego w górê, w którym siê ta encja zawiera w ca³oœci (lub g³ówny)
		ENTITY_OCTREE_NODE *NewNode = Node;
		for (;;)
		{
			if (NewNode->Parent == NULL)
				break;
			NewNode = NewNode->Parent;
			if (SphereInBox(SphereCenter, SphereRadius, NewNode->Bounds))
				break;
		}

		// Usuñ j¹ z tego wêz³a, razem z ca³ym kodem na ³¹czenie wêz³ów itd.
		// (Funkcja RemoveEntity korzysta z GetOctreeNode, nie pobiera od nowa WorldBoundingBox, nie wywo³uje te¿ SetOctreeNode.)
		RemoveEntity(e);

		// Rekurencyjnie dodaj t¹ encjê gdzieœ do poddrzewa tego nowego wêz³a
		// (Ta funkcja sama wywo³a e->SetOctreeNode.)
		AddEntityToNode(NewNode, e, SphereCenter, SphereRadius);
	}
}

void EntityOctree::FindEntities_Frustum(ENTITY_VECTOR *OutEntities, const FRUSTUM_PLANES &Frustum)
{
	OutEntities->clear();
	FindEntities_Frustum_Node(m_Root, OutEntities, Frustum, false);
}

void EntityOctree::FindEntities_SpotLight(ENTITY_VECTOR *OutEntities, SpotLight &spot_light)
{
	OutEntities->clear();
	FindEntities_SpotLight_Node(m_Root, OutEntities, spot_light.GetFrustumPlanes(), false);
}

void EntityOctree::FindEntities_PointLight(ENTITY_VECTOR OutEntities[6], PointLight &point_light)
{
	for (uint i = 0; i < 6; i++)
		OutEntities[i].clear();

	const FRUSTUM_PLANES *Frustums[] = {
		&point_light.GetSideFrustum(D3DCUBEMAP_FACE_POSITIVE_X),
		&point_light.GetSideFrustum(D3DCUBEMAP_FACE_NEGATIVE_X),
		&point_light.GetSideFrustum(D3DCUBEMAP_FACE_POSITIVE_Y),
		&point_light.GetSideFrustum(D3DCUBEMAP_FACE_NEGATIVE_Y),
		&point_light.GetSideFrustum(D3DCUBEMAP_FACE_POSITIVE_Z),
		&point_light.GetSideFrustum(D3DCUBEMAP_FACE_NEGATIVE_Z)
	};

	FindEntities_PointLight_Node(m_Root, OutEntities, point_light.GetPos(), point_light.GetDist(), Frustums);
}

void EntityOctree::FindEntities_DirectionalLight(ENTITY_VECTOR *OutEntities, const FRUSTUM_PLANES &CamFrustum, const BOX &CamBox, const VEC3 &LightDir)
{
	OutEntities->clear();
	FindEntities_DirectionalLight_Node(m_Root, OutEntities, CamFrustum, CamBox, LightDir);
}

bool EntityOctree::RayCollision(Entity **OutEntity, float *OutT, COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir)
{
	*OutT = MAXFLOAT;
	return RayCollision_Node(OutEntity, OutT, m_Root, Type, RayOrig, RayDir);
}

void EntityOctree::BuildSubBounds(BOX OutSubBounds[8], const BOX &Bounds)
{
	VEC3 BoundsCenter; Bounds.CalcCenter(&BoundsCenter);
	float x1 = Lerp(BoundsCenter.x, Bounds.p1.x, ENTITY_OCTREE_K);
	float y1 = Lerp(BoundsCenter.y, Bounds.p1.y, ENTITY_OCTREE_K);
	float z1 = Lerp(BoundsCenter.z, Bounds.p1.z, ENTITY_OCTREE_K);
	float x2 = Lerp(BoundsCenter.x, Bounds.p2.x, ENTITY_OCTREE_K);
	float y2 = Lerp(BoundsCenter.y, Bounds.p2.y, ENTITY_OCTREE_K);
	float z2 = Lerp(BoundsCenter.z, Bounds.p2.z, ENTITY_OCTREE_K);
	OutSubBounds[0] = BOX(Bounds.p1.x, Bounds.p1.y, Bounds.p1.z, x2,          y2,          z2);
	OutSubBounds[1] = BOX(x1,          Bounds.p1.y, Bounds.p1.z, Bounds.p2.x, y2,          z2);
	OutSubBounds[2] = BOX(Bounds.p1.x, y1,          Bounds.p1.z, x2,          Bounds.p2.y, z2);
	OutSubBounds[3] = BOX(x1,          y1,          Bounds.p1.z, Bounds.p2.x, Bounds.p2.y, z2);
	OutSubBounds[4] = BOX(Bounds.p1.x, Bounds.p1.y, z1,          x2,          y2,          Bounds.p2.z);
	OutSubBounds[5] = BOX(x1,          Bounds.p1.y, z1,          Bounds.p2.x, y2,          Bounds.p2.z);
	OutSubBounds[6] = BOX(Bounds.p1.x, y1,          z1,          x2,          Bounds.p2.y, Bounds.p2.z);
	OutSubBounds[7] = BOX(x1,          y1,          z1,          Bounds.p2.x, Bounds.p2.y, Bounds.p2.z);
}

void EntityOctree::DeleteNode(ENTITY_OCTREE_NODE *Node)
{
	if (Node->SubNodes[0] != NULL)
	{
		for (uint i = 0; i < 8; i++)
			DeleteNode(Node->SubNodes[i]);
	}

	m_Memory.Delete(Node);
}

void EntityOctree::AddEntityToNode(ENTITY_OCTREE_NODE *Node, Entity *e, const VEC3 &SphereCenter, float SphereRadius)
{
	// Posiada podwêz³y
	if (Node->SubNodes[0] != NULL)
	{
		// Spróbuj wrzuciæ do jednego z podwêz³ów
		bool SentToSubnode = false;
		for (uint si = 0; si < 8; si++)
		{
			// Sfera w ca³oœci zawiera siê w obszarze podwêz³a
			if (SphereInBox(SphereCenter, SphereRadius, Node->SubNodes[si]->Bounds))
			{
				AddEntityToNode(Node->SubNodes[si], e, SphereCenter, SphereRadius);
				SentToSubnode = true;
				break;
			}
		}
		// Nie trafi³a do ¿adnego z podwêz³ów, a wêze³ ma podwêz³y - có¿, nie ma wyjœcia, musi le¿eæ tu
		if (!SentToSubnode)
		{
			Node->Entities.push_back(e);
			e->SetOctreeNode(Node);
		}
	}
	// Nie posiada podwêz³ów
	else
	{
		// Na pocz¹tek dodaj do tego wêz³a
		Node->Entities.push_back(e);
		e->SetOctreeNode(Node);
		// Zobacz, czy nie warto by³oby spróbowaæ rozbiæ tego podwêz³a
		if (Node->Entities.size() > ENTITY_OCTREE_SPLIT_ENTITY_COUNT)
			SplitNode(Node);
	}
}

void EntityOctree::SplitNode(ENTITY_OCTREE_NODE *Node)
{
	// Ten algorytm utworzy podwêz³y podanego wêz³a niezale¿nie czy uda siê
	// do nich przenieœæ jakiekolwiek z jego encji.

	// Wylicz bounding boksy podwêz³ów
	BOX SubBounds[8];
	BuildSubBounds(SubBounds, Node->Bounds);
	// Utwórz podwêz³y
	for (uint si = 0; si < 8; si++)
	{
		Node->SubNodes[si] = m_Memory.New();
		Node->SubNodes[si]->Bounds = SubBounds[si];
		Node->SubNodes[si]->Parent = Node;
		for (uint ssi = 0; ssi < 8; ssi++)
			Node->SubNodes[si]->SubNodes[ssi] = NULL;
	}
	// Przenieœ encje do podwêz³ów, wyzeruj wskaŸniki do nich na naszej liœcie
	uint MovedEntityCount = 0;
	VEC3 SphereCenter; float SphereRadius;
	for (uint ei = 0; ei < Node->Entities.size(); ei++)
	{
		SphereCenter = Node->Entities[ei]->GetWorldPos();
		SphereRadius = Node->Entities[ei]->GetWorldRadius();
		for (uint si = 0; si < 8; si++)
		{
			if (SphereInBox(SphereCenter, SphereRadius, Node->SubNodes[si]->Bounds))
			{
				Node->SubNodes[si]->Entities.push_back(Node->Entities[ei]);
				Node->Entities[ei]->SetOctreeNode(Node->SubNodes[si]);
				Node->Entities[ei] = NULL;
				MovedEntityCount++;
				break;
			}
		}
	}
	// Skasuj wyzerowane wskaŸniki
	if (MovedEntityCount > 0)
	{
		ENTITY_VECTOR::iterator end_it = std::remove(Node->Entities.begin(), Node->Entities.end(), (Entity*)NULL);
		Node->Entities.erase(end_it, Node->Entities.end());
	}
}

void EntityOctree::TryJoin(ENTITY_OCTREE_NODE *Node)
{
	assert(Node != NULL && Node->SubNodes[0] != NULL);

	// Policz encje w tym wêŸle i jego podwêz³ach
	uint EntitySum = Node->Entities.size();
	for (uint si = 0; si < 8; si++)
	{
		// Podwêze³ ma swoje podwêz³y - ten wêze³ nie nadaje siê do ³¹czenia
		if (Node->SubNodes[si]->SubNodes[0] != NULL)
			return;
		EntitySum += Node->SubNodes[si]->Entities.size();
	}
	// Liczba wêz³ów w sumie pretenduje do po³¹czenia
	if (EntitySum < ENTITY_OCTREE_JOIN_ENTITY_COUNT)
	{
		for (uint si = 0; si < 8; si++)
		{
			// Podwêze³ na pewno nie ma swoich podwêz³ów
			assert(Node->SubNodes[si]->SubNodes[0] == NULL);
			// Przenieœ encje z podwêz³a do tego wêz³a
			Node->Entities.insert(Node->Entities.end(), Node->SubNodes[si]->Entities.begin(), Node->SubNodes[si]->Entities.end());
			for (uint ei = 0; ei < Node->SubNodes[si]->Entities.size(); ei++)
				Node->SubNodes[si]->Entities[ei]->SetOctreeNode(Node);
			// Usuñ ten podwêze³
			m_Memory.Delete(Node->SubNodes[si]);
			Node->SubNodes[si] = NULL;
		}
	}
}

void EntityOctree::FindEntities_Frustum_Node(ENTITY_OCTREE_NODE *Node, ENTITY_VECTOR *OutEntities, const FRUSTUM_PLANES &Frustum, bool Inside)
{
	// Podwêz³y
	if (Node->SubNodes[0] != NULL)
	{
		for (uint si = 0; si < 8; si++)
		{
			if (Inside)
				FindEntities_Frustum_Node(Node->SubNodes[si], OutEntities, Frustum, true);
			else if (BoxInFrustum(Node->SubNodes[si]->Bounds, Frustum))
				FindEntities_Frustum_Node(Node->SubNodes[si], OutEntities, Frustum, true);
			else if (BoxToFrustum_Fast(Node->SubNodes[si]->Bounds, Frustum))
				FindEntities_Frustum_Node(Node->SubNodes[si], OutEntities, Frustum, false);
		}
	}

	// Encje tego wêz³a
	VEC3 SphereCenter; float SphereRadius;
	for (uint ei = 0; ei < Node->Entities.size(); ei++)
	{
		if (Node->Entities[ei]->GetWorldVisible())
		{
			SphereCenter = Node->Entities[ei]->GetWorldPos();
			SphereRadius = Node->Entities[ei]->GetWorldRadius();
			if (Inside || SphereToFrustum_Fast(SphereCenter, SphereRadius, Frustum))
				OutEntities->push_back(Node->Entities[ei]);
		}
	}
}

void EntityOctree::FindEntities_SpotLight_Node(ENTITY_OCTREE_NODE *Node, ENTITY_VECTOR *OutEntities, const FRUSTUM_PLANES &Frustum, bool Inside)
{
	// Podwêz³y
	if (Node->SubNodes[0] != NULL)
	{
		for (uint si = 0; si < 8; si++)
		{
			if (Inside)
				FindEntities_SpotLight_Node(Node->SubNodes[si], OutEntities, Frustum, true);
			else if (BoxInFrustum(Node->SubNodes[si]->Bounds, Frustum))
				FindEntities_SpotLight_Node(Node->SubNodes[si], OutEntities, Frustum, true);
			else if (BoxToFrustum_Fast(Node->SubNodes[si]->Bounds, Frustum))
				FindEntities_SpotLight_Node(Node->SubNodes[si], OutEntities, Frustum, false);
		}
	}

	// Encje tego wêz³a
	VEC3 SphereCenter; float SphereRadius;
	for (uint ei = 0; ei < Node->Entities.size(); ei++)
	{
		// 1. Visible
		if (Node->Entities[ei]->GetWorldVisible())
		{
			MaterialEntity *me = dynamic_cast<MaterialEntity*>(Node->Entities[ei]);
			// 2. MaterialEntity, 3. CastShadow
			if (me != NULL && me->GetCastShadow())
			{
				SphereCenter = Node->Entities[ei]->GetWorldPos();
				SphereRadius = Node->Entities[ei]->GetWorldRadius();
				// 4. Kolizja z frustumem
				if (Inside || SphereToFrustum_Fast(SphereCenter, SphereRadius, Frustum))
					OutEntities->push_back(Node->Entities[ei]);
			}
		}
	}
}

void EntityOctree::FindEntities_PointLight_Node(ENTITY_OCTREE_NODE *Node, ENTITY_VECTOR OutEntities[8], const VEC3 &Pos, const float Dist, const FRUSTUM_PLANES *Frustums[6])
{
	// Podwêz³y - tylko na bazie sfery
	if (Node->SubNodes[0] != NULL)
	{
		for (uint si = 0; si < 8; si++)
		{
			if (SphereToBox(Pos, Dist, Node->SubNodes[si]->Bounds))
			{
				FindEntities_PointLight_Node(Node->SubNodes[si], OutEntities, Pos, Dist, Frustums);
			}
		}
	}

	// Encje tego wêz³a - na bazie sfery i frustuma
	VEC3 SphereCenter; float SphereRadius;
	for (uint ei = 0; ei < Node->Entities.size(); ei++)
	{
		// 1. Visible
		if (Node->Entities[ei]->GetWorldVisible())
		{
			MaterialEntity *me = dynamic_cast<MaterialEntity*>(Node->Entities[ei]);
			// 2. MaterialEntity, 3. CastShadow
			if (me != NULL && me->GetCastShadow())
			{
				SphereCenter = Node->Entities[ei]->GetWorldPos();
				SphereRadius = Node->Entities[ei]->GetWorldRadius();
				// 4. Kolizja ze sfer¹
				if (SphereToSphere(SphereCenter, SphereRadius, Pos, Dist))
				{
					for (uint i = 0; i < 6; i++)
					{
						// 5. Kolizja z frustumem
						if (SphereToFrustum_Fast(SphereCenter, SphereRadius, *Frustums[i]))
							OutEntities[i].push_back(Node->Entities[ei]);
					}
				}
			}
		}
	}
}

void EntityOctree::FindEntities_DirectionalLight_Node(ENTITY_OCTREE_NODE *Node, ENTITY_VECTOR *OutEntities, const FRUSTUM_PLANES &CamFrustum, const BOX &CamBox, const VEC3 &LightDir)
{
	// Podwêz³y
	if (Node->SubNodes[0] != NULL)
	{
		float T;
		for (uint si = 0; si < 8; si++)
		{
			if (SweptBoxToBox(Node->SubNodes[si]->Bounds, CamBox, LightDir, &T) && T >= 0.f)
				FindEntities_DirectionalLight_Node(Node->SubNodes[si], OutEntities, CamFrustum, CamBox, LightDir);
		}
	}

	// Encje tego wêz³a
	VEC3 SphereCenter; float SphereRadius;
	float t;
	for (uint ei = 0; ei < Node->Entities.size(); ei++)
	{
		// 1. Visible
		if (Node->Entities[ei]->GetWorldVisible())
		{
			MaterialEntity *me = dynamic_cast<MaterialEntity*>(Node->Entities[ei]);
			TreeEntity *te = dynamic_cast<TreeEntity*>(Node->Entities[ei]);
			// 2. MaterialEntity lub TreeEntity, 3. CastShadow
			if (me != NULL && me->GetCastShadow() || te != NULL)
			{
				SphereCenter = Node->Entities[ei]->GetWorldPos();
				SphereRadius = Node->Entities[ei]->GetWorldRadius();
				// 4. Kolizja swept z frustumem
				// Test z frustumem by³by dok³adniejszy, ale Ÿle dzia³a.
				//if (SweptSphereToFrustum(SphereCenter, SphereRadius, LightDir, CamFrustum))
				// Test SweptSphereToBox te¿ niestety dzia³a Ÿle
				// Przy kamerze znajduj¹cej siê blisko œrodka sfery (wewn¹trz?) pod niektórymi k¹tami zwraca false i cieñ znika.
				//if (SweptSphereToBox(SphereCenter, SphereRadius, LightDir, CamBox))
				// To jest takie sobie, ale przynajmniej dzia³a dobrze.
				if (SweptBoxToBox(
					BOX(
						SphereCenter.x - SphereRadius, SphereCenter.y - SphereRadius, SphereCenter.z - SphereRadius,
						SphereCenter.x + SphereRadius, SphereCenter.y + SphereRadius, SphereCenter.z + SphereRadius),
					CamBox,
					LightDir,
					&t) && t >= 0.f)
				{
					OutEntities->push_back(Node->Entities[ei]);
				}
			}
		}
	}
}

bool EntityOctree::RayCollision_Node(Entity **OutEntity, float *InOutT, ENTITY_OCTREE_NODE *Node, COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir)
{
	float t;
	VEC3 RayOrig_model, RayDir_model;
	bool R = false;

	// Encje tego wêz³a
	VEC3 SphereCenter; float SphereRadius;
	for (uint ei = 0; ei < Node->Entities.size(); ei++)
	{
		Entity *e = Node->Entities[ei];

		// Kolizja optyczna i encja niewidoczna - pomiñ
		if (Type == COLLISION_OPTICAL && !Node->Entities[ei]->GetWorldVisible())
			continue;

		// Test z bounding sphere
		SphereCenter = e->GetWorldPos();
		SphereRadius = e->GetWorldRadius();
		if (RayToSphere(RayOrig, RayDir, SphereCenter, SphereRadius, &t) && t >= 0.f && t < *InOutT)
		{
			const MATRIX & InvWorld = Node->Entities[ei]->GetInvWorldMatrix();
			Transform(&RayOrig_model, RayOrig, InvWorld);
			TransformNormal(&RayDir_model, RayDir, InvWorld);
			if (Node->Entities[ei]->RayCollision(Type, RayOrig_model, RayDir_model, &t) && t < *InOutT)
			{
				*OutEntity = Node->Entities[ei];
				*InOutT = t;
				R = true;
			}
		}
	}

	// Podwêz³y
	if (Node->SubNodes[0] != NULL)
	{
		for (uint si = 0; si < 8; si++)
		{
			if (RayToBox(&t, RayOrig, RayDir, Node->SubNodes[si]->Bounds) && t >= 0.f && t < *InOutT)
			{
				if (RayCollision_Node(OutEntity, InOutT, Node->SubNodes[si], Type, RayOrig, RayDir))
					R = true;
			}
		}
	}

	return R;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa BaseMaterial

BaseMaterial::BaseMaterial(Scene *OwnerScene, const string &Name) :
	m_OwnerScene(OwnerScene),
	m_Name(Name),
	m_TwoSided(false),
	m_CollisionType(COLLISION_BOTH)
{
	m_OwnerScene->GetMaterialCollection().RegisterMaterial(m_Name, this);
}

BaseMaterial::~BaseMaterial()
{
	m_OwnerScene->GetMaterialCollection().UnregisterMaterial(m_Name, this);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa WireFrameMaterial

WireFrameMaterial::WireFrameMaterial(Scene *OwnerScene, const string &Name) :
	BaseMaterial(OwnerScene, Name),
	m_Color(COLORF::WHITE),
	m_BlendMode(BLEND_LERP),
	m_ColorMode(COLOR_MATERIAL)
{
}

WireFrameMaterial::WireFrameMaterial(Scene *OwnerScene, const string &Name, const COLORF &Color, BLEND_MODE BlendMode, COLOR_MODE ColorMode) :
	BaseMaterial(OwnerScene, Name),
	m_Color(Color),
	m_BlendMode(BlendMode),
	m_ColorMode(ColorMode)
{
}

WireFrameMaterial::~WireFrameMaterial()
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa SolidMaterial

SolidMaterial::SolidMaterial(Scene *OwnerScene, const string &Name) :
	BaseMaterial(OwnerScene, Name),
	m_DiffuseColor(COLORF::WHITE),
	m_TextureAnimation(false),
	m_ColorMode(COLOR_MATERIAL),
	m_DiffuseTexture(NULL),
	m_EmissiveTexture(NULL),
	m_EnvironmentalTexture(NULL),
	m_FresnelColor(COLORF::BLACK),
	m_FresnelPower(1.f)
{
}

SolidMaterial::SolidMaterial(Scene *OwnerScene, const string &Name, const string &DiffuseTextureName, COLOR_MODE ColorMode) :
	BaseMaterial(OwnerScene, Name),
	m_DiffuseTextureName(DiffuseTextureName),
	m_DiffuseColor(COLORF::WHITE),
	m_TextureAnimation(false),
	m_ColorMode(ColorMode),
	m_DiffuseTexture(NULL),
	m_EmissiveTexture(NULL),
	m_EnvironmentalTexture(NULL),
	m_FresnelColor(COLORF::BLACK),
	m_FresnelPower(1.f)
{
}

res::D3dTexture * SolidMaterial::GetDiffuseTexture()
{
	if (m_DiffuseTexture != NULL)
	{
		m_DiffuseTexture->Load();
		return m_DiffuseTexture;
	}
	if (!IsDiffuseTexture())
		return NULL;
	m_DiffuseTexture = res::g_Manager->MustGetResourceEx<res::D3dTexture>(GetDiffuseTextureName());
	m_DiffuseTexture->Load();
	return m_DiffuseTexture;
}

res::D3dTexture * SolidMaterial::GetEmissiveTexture()
{
	if (m_EmissiveTexture != NULL)
	{
		m_EmissiveTexture->Load();
		return m_EmissiveTexture;
	}
	if (m_EmissiveTextureName.empty())
		return NULL;
	m_EmissiveTexture = res::g_Manager->MustGetResourceEx<res::D3dTexture>(GetEmissiveTextureName());
	m_EmissiveTexture->Load();
	return m_EmissiveTexture;
}

res::D3dCubeTexture * SolidMaterial::GetEnvironmentalTexture()
{
	if (m_EnvironmentalTexture != NULL)
	{
		m_EnvironmentalTexture->Load();
		return m_EnvironmentalTexture;
	}
	if (m_EnvironmentalTextureName.empty())
		return NULL;
	m_EnvironmentalTexture = res::g_Manager->MustGetResourceEx<res::D3dCubeTexture>(GetEnvironmentalTextureName());
	m_EnvironmentalTexture->Load();
	return m_EnvironmentalTexture;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa OpaqueMaterial

OpaqueMaterial::OpaqueMaterial(Scene *OwnerScene, const string &Name) :
	SolidMaterial(OwnerScene, Name),
	m_AlphaTesting(0),
	m_HalfLambert(false),
	m_PerPixel(false),
	m_NormalTexture(NULL),
	m_SpecularMode(SPECULAR_NORMAL),
	m_SpecularColor(COLORF::WHITE),
	m_SpecularPower(16.f),
	m_GlossMapping(false)
{
}

OpaqueMaterial::OpaqueMaterial(Scene *OwnerScene, const string &Name, const string &DiffuseTextureName, COLOR_MODE ColorMode) :
	SolidMaterial(OwnerScene, Name, DiffuseTextureName, ColorMode),
	m_AlphaTesting(0),
	m_HalfLambert(false),
	m_PerPixel(false),
	m_NormalTexture(NULL),
	m_SpecularMode(SPECULAR_NORMAL),
	m_SpecularColor(COLORF::WHITE),
	m_SpecularPower(16.f),
	m_GlossMapping(false)
{
}

res::D3dTexture * OpaqueMaterial::GetNormalTexture()
{
	if (m_NormalTexture != NULL)
	{
		m_NormalTexture->Load();
		return m_NormalTexture;
	}
	if (m_NormalTextureName.empty())
		return NULL;
	m_NormalTexture = res::g_Manager->MustGetResourceEx<res::D3dTexture>(GetNormalTextureName());
	m_NormalTexture->Load();
	return m_NormalTexture;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TranslucentMaterial

TranslucentMaterial::TranslucentMaterial(Scene *OwnerScene, const string &Name) :
	SolidMaterial(OwnerScene, Name),
	m_BlendMode(BLEND_LERP),
	m_AlphaMode(ALPHA_MATERIAL)
{
}

TranslucentMaterial::TranslucentMaterial(Scene *OwnerScene, const string &Name, const string &DiffuseTextureName, BLEND_MODE BlendMode, COLOR_MODE ColorMode, ALPHA_MODE AlphaMode) :
	SolidMaterial(OwnerScene, Name, DiffuseTextureName, ColorMode),
	m_BlendMode(BlendMode),
	m_AlphaMode(AlphaMode)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa MaterialCollection

MaterialCollection::MaterialCollection() :
	m_Destroying(false)
{
}

MaterialCollection::~MaterialCollection()
{
	m_Destroying = true;

	// Usuñ wszystkie pozosta³e nieusuniête materia³y
	for (MATERIAL_MAP::iterator mit = m_Map.begin(); mit != m_Map.end(); ++mit)
		delete mit->second;
}

bool MaterialCollection::Exists(const string &Name)
{
	MATERIAL_MAP::iterator mit = m_Map.find(Name);
	return (mit != m_Map.end());
}

bool MaterialCollection::Exists(BaseMaterial *Material)
{
	for (MATERIAL_MAP::iterator mit = m_Map.begin(); mit != m_Map.end(); ++mit)
		if (mit->second == Material)
			return true;
	return false;
}

BaseMaterial * MaterialCollection::GetByName(const string &Name)
{
	MATERIAL_MAP::iterator mit = m_Map.find(Name);
	if (mit == m_Map.end())
		return NULL;
	return mit->second;
}

BaseMaterial * MaterialCollection::MustGetByName(const string &Name)
{
	MATERIAL_MAP::iterator mit = m_Map.find(Name);
	if (mit == m_Map.end())
		throw Error(Format("engine::MaterialCollection::MustGetByName: Materia³ nie istnieje. Nazwa=#") % Name);
	return mit->second;
}

void MaterialCollection::RegisterMaterial(const string &Name, BaseMaterial *Material)
{
	m_Map.insert(MATERIAL_MAP::value_type(Name, Material));
}

void MaterialCollection::UnregisterMaterial(const string &Name, BaseMaterial *Material)
{
	if (!m_Destroying)
	{
		MATERIAL_MAP::iterator mit = m_Map.find(Name);
		assert(mit != m_Map.end());
		m_Map.erase(mit);
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa BaseLight

BaseLight::BaseLight(Scene *OwnerScene) :
	m_OwnerScene(OwnerScene),
	m_Enabled(true),
	m_CastShadow(false),
	m_CastSpecular(true),
	m_HalfLambert(false),
	m_Color(COLORF::WHITE),
	m_ShadowFactor(0.f)
{
}

BaseLight::BaseLight(Scene *OwnerScene, const COLORF &Color) :
	m_OwnerScene(OwnerScene),
	m_Enabled(true),
	m_CastShadow(false),
	m_CastSpecular(true),
	m_HalfLambert(false),
	m_Color(Color),
	m_ShadowFactor(0.f)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa DirectionalLight

DirectionalLight::DirectionalLight(Scene *OwnerScene) :
	BaseLight(OwnerScene),
	m_Dir(VEC3::NEGATIVE_Y),
	m_ZFar(MAXFLOAT)
{
	GetOwnerScene()->RegisterDirectionalLight(this);
}

DirectionalLight::DirectionalLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Dir) :
	BaseLight(OwnerScene, Color),
	m_Dir(Dir),
	m_ZFar(MAXFLOAT)
{
	GetOwnerScene()->RegisterDirectionalLight(this);
}

DirectionalLight::~DirectionalLight()
{
	GetOwnerScene()->UnregisterDirectionalLight(this);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PositionedLight

PositionedLight::PositionedLight(Scene *OwnerScene)	:
	BaseLight(OwnerScene),
	m_Pos(VEC3::ZERO)
{
}

PositionedLight::PositionedLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Pos) :
	BaseLight(OwnerScene, Color),
	m_Pos(Pos)
{
}

float PositionedLight::GetScissorRect(RECT *OutRect, const MATRIX &View, const MATRIX &Proj, uint RenderTargetCX, uint RenderTargetCY)
{
	VEC3 ViewPoints[8], TransformedPoints[8];
	const VEC3 *Points = GetBoundingBox();

	for (uint i = 0; i < 8; i++)
		Transform(&ViewPoints[i], Points[i], View);

	// Jeœli choæ jeden punkt jest z ty³u kamery, ustaw ca³y prostok¹t
	bool PointBehind = false;
	for (uint i = 0; i < 8; i++)
	{
		if (ViewPoints[i].z <= 0.f)
		{
			PointBehind = true;
			break;
		}
	}

	if (PointBehind)
	{
		OutRect->left = 0;
		OutRect->top = 0;
		OutRect->right = RenderTargetCX;
		OutRect->bottom = RenderTargetCY;
	}
	else
	{
		for (uint i = 0; i < 8; i++)
			TransformCoord(&TransformedPoints[i], ViewPoints[i], Proj);

		RECTF Rect1;
		Rect1.left = Rect1.right  = TransformedPoints[0].x;
		Rect1.top  = Rect1.bottom = TransformedPoints[0].y;
		for (uint i = 1; i < 8; i++)
		{
			Rect1.left   = std::min(Rect1.left,   TransformedPoints[i].x);
			Rect1.right  = std::max(Rect1.right,  TransformedPoints[i].x);
			Rect1.top    = std::min(Rect1.top,    TransformedPoints[i].y);
			Rect1.bottom = std::max(Rect1.bottom, TransformedPoints[i].y);
		}

		OutRect->left   = minmax<int>( 0, (int)(( Rect1.left   * 0.5f + 0.5f) * (float)RenderTargetCX), RenderTargetCX );
		OutRect->right  = minmax<int>( 0, (int)(( Rect1.right  * 0.5f + 0.5f) * (float)RenderTargetCX), RenderTargetCX );
		OutRect->top    = minmax<int>( 0, (int)((-Rect1.bottom * 0.5f + 0.5f) * (float)RenderTargetCY), RenderTargetCY );
		OutRect->bottom = minmax<int>( 0, (int)((-Rect1.top    * 0.5f + 0.5f) * (float)RenderTargetCY), RenderTargetCY );
	}

	// Wzór: (Szerokoœæ+Wysokoœæ) / (MaxSzerokoœæ+MaxWysokoœæ) dzia³a lepiej ni¿
	// stosunek prawdziwej powierzchni: (Szerokoœæ*Wysokoœæ) / (MaxSzerokoœæ*MaxWysokoœæ),
	// bo ten dawa³ szybko bardzo ma³e liczby.
	// Chocia¿ czy na pewno???
	return (float)((OutRect->right-OutRect->left) + (OutRect->bottom-OutRect->top)) / (float)(RenderTargetCX + RenderTargetCY);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa SpotLight

SpotLight::SpotLight(Scene *OwnerScene) :
	PositionedLight(OwnerScene),
	m_Dir(VEC3::NEGATIVE_Y),
	m_Dist(10.f),
	m_Fov(PI_2),
	m_Smooth(false),
	m_ZNear(0.5f),
	m_BoundingBoxIs(false),
	m_ParamsCameraIs(false)
{
	GetOwnerScene()->RegisterSpotLight(this);
}

SpotLight::SpotLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Pos, const VEC3 &Dir, float Dist, float Fov) :
	PositionedLight(OwnerScene, Color, Pos),
	m_Dir(Dir),
	m_Dist(Dist),
	m_Fov(Fov),
	m_Smooth(false),
	m_ZNear(0.5f),
	m_BoundingBoxIs(false),
	m_ParamsCameraIs(false)
{
	GetOwnerScene()->RegisterSpotLight(this);
}

SpotLight::~SpotLight()
{
	GetOwnerScene()->UnregisterSpotLight(this);
}

bool SpotLight::SphereInRange(const VEC3 &SphereCenter, float SphereRadius)
{
	// Test sfera-sfera
	if (!SphereToSphere(SphereCenter, SphereRadius, GetPos(), GetDist()))
		return false;
	// Test sfera-frustum
	return SphereToFrustum_Fast(SphereCenter, SphereRadius, GetFrustumPlanes());
}

bool SpotLight::BoxInRange(const BOX &Box)
{
	if (!SphereToBox(GetPos(), GetDist(), Box))
		return false;
	return BoxToFrustum_Fast(Box, GetFrustumPlanes());
}

void SpotLight::EnsureBoundingBox()
{
	if (!m_BoundingBoxIs)
	{
		VEC3 Center = GetPos() + GetDir() * (GetDist() * 0.5f);
		float Radius = GetDist() * tanf(GetFov() * 0.5f); // Z tw. Pitagorasa

		// Wyznacz wektory po³ówkowe OBB otaczaj¹cego sto¿ek widzenia
		VEC3 Half1 = GetDir();
		VEC3 Up = VEC3::POSITIVE_Y;
		if (fabsf(Dot(Half1, Up)) > 0.9f)
			Up = VEC3::POSITIVE_Z;
		VEC3 Half2; Cross(&Half2, Half1, Up);
		VEC3 Half3; Cross(&Half3, Half1, Half2);
		Normalize(&Half2);
		Normalize(&Half3);
		Half1 *= GetDist() * 0.5f;
		Half2 *= Radius;
		Half3 *= Radius;

		m_BoundingBox[0] = Center - Half1 - Half2 - Half3;
		m_BoundingBox[1] = Center + Half1 - Half2 - Half3;
		m_BoundingBox[2] = Center - Half1 + Half2 - Half3;
		m_BoundingBox[3] = Center + Half1 + Half2 - Half3;
		m_BoundingBox[4] = Center - Half1 - Half2 + Half3;
		m_BoundingBox[5] = Center + Half1 - Half2 + Half3;
		m_BoundingBox[6] = Center - Half1 + Half2 + Half3;
		m_BoundingBox[7] = Center + Half1 + Half2 + Half3;

		m_BoundingBoxIs = true;
	}
}

void SpotLight::EnsureParamsCamera()
{
	if (!m_ParamsCameraIs)
	{
		VEC3 Up = VEC3::POSITIVE_Y;
		if (fabsf(Dot(GetDir(), Up)) > 0.9f)
			Up = VEC3::POSITIVE_Z;

		m_ParamsCamera.Set(GetPos(), GetDir(), Up, GetFov(), 1.f, GetZNear(), GetDist());

		m_ParamsCameraIs = true;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa PointLight

PointLight::PointLight(Scene *OwnerScene) :
	PositionedLight(OwnerScene),
	m_Dist(10.f),
	m_ZNear(0.5f),
	m_BoundingBoxIs(false),
	m_ParamsCamerasIs(false)
{
	GetOwnerScene()->RegisterPointLight(this);
}

PointLight::PointLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Pos, float Dist) :
	PositionedLight(OwnerScene, Color, Pos),
	m_Dist(Dist),
	m_ZNear(0.5f),
	m_BoundingBoxIs(false),
	m_ParamsCamerasIs(false)
{
	GetOwnerScene()->RegisterPointLight(this);
}

PointLight::~PointLight()
{
	GetOwnerScene()->UnregisterPointLight(this);
}

bool PointLight::SphereInRange(const VEC3 &SphereCenter, float SphereRadius)
{
	return SphereToSphere(SphereCenter, SphereRadius, GetPos(), GetDist());
}

bool PointLight::BoxInRange(const BOX &Box)
{
	return SphereToBox(GetPos(), GetDist(), Box);
}

void PointLight::EnsureBoundingBox()
{
	if (!m_BoundingBoxIs)
	{
		// Zwróæ rogi AABB otaczaj¹cego kulê

		BOX b;
		b.p1.x = GetPos().x - GetDist();
		b.p1.y = GetPos().y - GetDist();
		b.p1.z = GetPos().z - GetDist();
		b.p2.x = GetPos().x + GetDist();
		b.p2.y = GetPos().y + GetDist();
		b.p2.z = GetPos().z + GetDist();

		b.GetAllCorners(m_BoundingBox);

		m_BoundingBoxIs = true;
	}
}

void PointLight::EnsureParamsCameras()
{
	if (!m_ParamsCamerasIs)
	{
		VEC3 Forward, Up;
		for (uint i = 0; i < 6; i++)
		{
			GetCubeMapForwardDir(&Forward, (D3DCUBEMAP_FACES)i);
			GetCubeMapUpDir(&Up, (D3DCUBEMAP_FACES)i);

			m_ParamsCameras[i].Set(
				GetPos(),
				Forward,
				Up,
				PI_2,
				1.f,
				GetZNear(),
				GetDist());
		}

		m_ParamsCamerasIs = true;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Entity

void Entity::EnsureLocalMatrix()
{
	if (!m_LocalMatrixCalculated)
	{
		if (m_IsOrientation)
		{
			if (m_Size != 1.f)
			{
				MATRIX SM, RM;

				//Scaling(&SM, m_Size);
				//RotationYawPitchRoll(&RM, m_Orientation);
				//Translation(&TM, m_Pos);
				//m_LocalMatrix = SM * RM * TM;

				Scaling(&SM, m_Size);
				QuaternionToRotationMatrix(&RM, m_Orientation);
				RM._41 = m_Pos.x; RM._42 = m_Pos.y; RM._43 = m_Pos.z;
				m_LocalMatrix = SM * RM;
			}
			else
			{
				MATRIX RM;

				//RotationYawPitchRoll(&RM, m_Orientation);
				//Translation(&TM, m_Pos);
				//m_LocalMatrix = RM * TM;

				QuaternionToRotationMatrix(&m_LocalMatrix, m_Orientation);
				m_LocalMatrix._41 = m_Pos.x; m_LocalMatrix._42 = m_Pos.y; m_LocalMatrix._43 = m_Pos.z;
			}
		}
		else
		{
			if (m_Size != 1.f)
			{
				//MATRIX SM, TM;
				//Scaling(&SM, m_Size);
				//Translation(&TM, m_Pos);
				//m_LocalMatrix = SM * TM;

				Scaling(&m_LocalMatrix, m_Size);
				m_LocalMatrix._41 = m_Pos.x; m_LocalMatrix._42 = m_Pos.y; m_LocalMatrix._43 = m_Pos.z;
			}
			else
			{
				Translation(&m_LocalMatrix, m_Pos);
			}
		}

		m_LocalMatrixCalculated = true;
	}
}

void Entity::EnsureInvLocalMatrix()
{
	if (!m_InvLocalMatrixCalculated)
	{
		Inverse(&m_InvLocalMatrix, GetLocalMatrix());
		m_InvLocalMatrixCalculated = true;
	}
}

void Entity::EnsureWorldMatrix()
{
	if (!m_WorldMatrixCalculated)
	{
		// Nie ma parenta
		if (m_Parent == NULL)
			m_WorldMatrix = GetLocalMatrix();
		// Jest parent, nie ma koœci
		else if (m_ParentBone.empty())
			m_WorldMatrix = GetLocalMatrix() * m_Parent->GetWorldMatrix();
		// Jest parent i jest koœæ
		else
			// To jest doœæ nieoptymalne, ale có¿...
			m_WorldMatrix = GetLocalMatrix() * m_Parent->GetBoneMatrix(m_ParentBone) * m_Parent->GetWorldMatrix();

		m_WorldMatrixCalculated = true;
	}
}

void Entity::EnsureInvWorldMatrix()
{
	if (!m_InvWorldMatrixCalculated)
	{
		Inverse(&m_InvWorldMatrix, GetWorldMatrix());
		m_InvWorldMatrixCalculated = true;
	}
}

void Entity::EnsureWorldPos()
{
	if (!m_WorldPosCalculated)
	{
		Transform(&m_WorldPos, VEC3::ZERO, GetWorldMatrix());
		m_WorldPosCalculated = true;
	}
}

void Entity::EnsureWorldSize()
{
	if (!m_WorldSizeCalculated)
	{
		if (m_Parent == NULL)
			m_WorldSize = GetSize();
		else
			m_WorldSize = GetSize() * m_Parent->GetWorldSize();

		m_WorldSizeCalculated = true;
	}
}

void Entity::EnsureWorldRadius()
{
	if (!m_WorldRadiusCalculated)
	{
		m_WorldRadius = GetRadius() * GetWorldSize();

		m_WorldRadiusCalculated = true;
	}
}

void Entity::EnsureWorldVisible()
{
	if (!m_WorldVisibleCalculated)
	{
		if (m_Parent == NULL)
			m_WorldVisible = GetVisible();
		else
			m_WorldVisible = GetVisible() && m_Parent->GetWorldVisible();

		m_WorldVisibleCalculated = true;
	}
}

void Entity::OnTransformChange()
{
	// Zmiana orientacji, w przeciwieñstwie do zmiany pozycji czy rozmiaru nie powoduje wprawdzie
	// niebezpieczeñstwa ¿e encja wyjdzie ze swojego wêz³a Octree, ale mo¿e to spowodowaæ
	// dla swoich podencji, dlatego te¿ wywo³uje OnTransformChange.

	m_LocalMatrixCalculated = false;
	m_InvLocalMatrixCalculated = false;
	m_WorldMatrixCalculated = false;
	m_InvWorldMatrixCalculated = false;
	m_WorldPosCalculated = false;
	m_WorldSizeCalculated = false;
	m_WorldRadiusCalculated = false;
	m_WorldVisibleCalculated = false;

	// Poinformuj drzewo ósemkowe o mo¿liwej zmianie po³o¿enia
	m_OwnerScene->OnEntityParamsChange(this);

	// Uniewa¿nij pod-encje
	for (ENTITY_SET::iterator it = m_SubEntities.begin(); it != m_SubEntities.end(); ++it)
		(*it)->OnTransformChange();
}

Entity::Entity(Scene *OwnerScene) :
	m_Destroying(false),
	m_OwnerScene(OwnerScene),
	m_OctreeNode(NULL),
	m_Parent(NULL),
	m_Pos(VEC3::ZERO),
	m_Orientation(QUATERNION::IDENTITY),
	m_IsOrientation(false),
	m_Size(1.f),
	m_Radius(1.f),
	m_LocalMatrixCalculated(false),
	m_InvLocalMatrixCalculated(false),
	m_WorldMatrixCalculated(false),
	m_InvWorldMatrixCalculated(false),
	m_WorldPosCalculated(false),
	m_WorldSizeCalculated(false),
	m_WorldRadiusCalculated(false),
	m_WorldVisibleCalculated(false),
	m_Visible(true)
{
	m_OwnerScene->RegisterEntity(this);
	m_OwnerScene->RegisterRootEntity(this);
}

void Entity::SetParent(Entity *Parent)
{
	assert(Parent != this);

	if (Parent == m_Parent) return;

	if (m_Parent == NULL)
		m_OwnerScene->UnregisterRootEntity(this);
	else
		m_Parent->UnregisterSubEntity(this);

	m_Parent = Parent;
	m_ParentBone.clear();

	if (m_Parent == NULL)
		m_OwnerScene->RegisterRootEntity(this);
	else
		m_Parent->RegisterSubEntity(this);

	OnTransformChange();
}

void Entity::SetParentBone(Entity *Parent, const string &ParentBone)
{
	assert(Parent != this);

	if (Parent == m_Parent) return;

	if (m_Parent == NULL)
		m_OwnerScene->UnregisterRootEntity(this);
	else
		m_Parent->UnregisterSubEntity(this);

	m_Parent = Parent;
	m_ParentBone = ParentBone;

	if (m_Parent == NULL)
		m_OwnerScene->RegisterRootEntity(this);
	else
		m_Parent->RegisterSubEntity(this);

	OnTransformChange();
}

Entity::~Entity()
{
	m_Destroying = true;

	// Usuñ encje podrzêdne
	for (ENTITY_SET::iterator it = m_SubEntities.begin(); it != m_SubEntities.end(); ++it)
		delete *it;
	m_SubEntities.clear();

	// Wyrejestruj ze sceny i encji nadrzêdnej
	if (m_Parent == NULL)
		m_OwnerScene->UnregisterRootEntity(this);
	else
		m_Parent->UnregisterSubEntity(this);

	m_OwnerScene->UnregisterEntity(this);
}

void Entity::RegisterSubEntity(Entity *SubEntity)
{
	m_SubEntities.insert(SubEntity);
}

void Entity::UnregisterSubEntity(Entity *SubEntity)
{
	if (!m_Destroying)
	{
		ENTITY_SET::iterator it = m_SubEntities.find(SubEntity);
		assert(it != m_SubEntities.end());
		m_SubEntities.erase(it);
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa MaterialEntity

MaterialEntity::MaterialEntity(Scene *OwnerScene) :
	Entity(OwnerScene),
	m_TeamColor(COLORF::WHITE),
	m_TextureMatrix(MATRIX::IDENTITY),
	m_UseLighting(true),
	m_CastShadow(true),
	m_ReceiveShadow(true)
{
}

void MaterialEntity::DrawFragment(PASS Pass, const ENTITY_FRAGMENT &Fragment, const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam, BaseLight *Light)
{
	uint BoneCount = 0xDEADC0DE;
	const MATRIX *BoneMatrices = NULL;
	if (!GetSkinningData(&BoneCount, &BoneMatrices))
		BoneMatrices = NULL;

	g_Engine->GetServices()->SetupState_Material(
		Pass,
		SceneDrawParams,
		Cam,
		*Fragment.M,
		ENTITY_DRAW_PARAMS(GetWorldSize(), GetWorldMatrix(), GetInvWorldMatrix(), GetTextureMatrix(), GetTeamColor(), GetUseLighting(), BoneCount, BoneMatrices),
		Light);

	DrawFragmentGeometry(Fragment.FragmentId, Fragment.M, *Cam);

	g_Engine->GetServices()->UnsetupState();
}

ENTITY_FRAGMENT MaterialEntity::GetFragment(uint Index)
{
	uint Id;
	BaseMaterial *Mat;
	GetFragmentData(Index, &Id, &Mat);
	return ENTITY_FRAGMENT(this, Id, Mat, GetWorldPos());
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa wewnêtrzna QMapRenderer

/*
W chwili tworzenia i przez ca³y czas istnienia obiektu tej klasy istnieæ musi
podana do konstruktora mapa oraz materia³y u¿ywane przez t¹ mapê.
*/

class QMapRenderer
{
public:
	// Deskryptor fragmentu do narysowania
	struct FRAGMENT_DESC
	{
		const QMap::DRAW_FRAGMENT *MapFragment;
		OpaqueMaterial *Material;
		const BOX *Bounds;

		FRAGMENT_DESC(const QMap::DRAW_FRAGMENT *MapFragment, OpaqueMaterial *Material, const BOX *Bounds) : MapFragment(MapFragment), Material(Material), Bounds(Bounds) { }
		bool operator < (const FRAGMENT_DESC &d) { return Material < d.Material; } // Do sortowania
	};
	typedef std::vector<FRAGMENT_DESC> FRAGMENT_DESC_VECTOR;

	QMapRenderer(Scene *OwnerScene, QMap *Map);
	~QMapRenderer();

	// Zwraca kolekcjê fragmentów, które wymagaj¹ narysowania w podanym frustumie
	void CalcFragmentsInFrustum(FRAGMENT_DESC_VECTOR *OutFragments, const FRUSTUM_PLANES &FrustumPlanes);
	// Zwraca kolekcjê fragmentów, które wymagaj¹ narysowania w zasiêgu podanego œwiat³a Spot
	// Niestety tu zwracam ca³e fragmenty mimo ¿e pole Material i Bounds bêdzie nieu¿ywane.
	void CalcFragmentsInSpotLight(FRAGMENT_DESC_VECTOR *OutFragments, SpotLight &spot_light);
	// Zwraca kolekcjê fragmentów, które wymagaj¹ narysowania w zasiêgu podanego œwiat³a Point
	// Niestety tu zwracam ca³e fragmenty mimo ¿e pole Material i Bounds bêdzie nieu¿ywane.
	void CalcFragmentsInPointLight(FRAGMENT_DESC_VECTOR OutFragments[6], PointLight &point_light);
	// Zwraca kolekcjê fragmentów, które wymagaj¹ narysowania w zasiêgu podanego œwiat³a Directional
	// Niestety tu zwracam ca³e fragmenty mimo ¿e pole Material i Bounds bêdzie nieu¿ywane.
	void CalcFragmentsInDirectionalLight(FRAGMENT_DESC_VECTOR *OutFragments, const FRUSTUM_PLANES &CamFrustum, const BOX &CamBox, const VEC3 &LightDir);
	// Sortuje fragmenty wg materia³u
	void SortFragments(FRAGMENT_DESC_VECTOR *Fragments);
	// Rysuje podany fragment
	void DrawFragment(PASS Pass, const FRAGMENT_DESC &Fragment, const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam, BaseLight *Light);

	bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT);

private:
	Scene *m_OwnerScene;
	QMap *m_Map;
	// WskaŸniki do materia³ów odpowiadaj¹ce poszczególnym materia³om mapy
	std::vector<OpaqueMaterial*> m_Materials;
	// Wymiar 1 to materia³y - pozostaje zawsze zaalokowany
	// Wymiar 2 to poszczególne fragmenty - u¿ywany tylko tymczasowo
	std::vector< std::vector<const QMap::DRAW_FRAGMENT*> > m_DrawFragments;

	void CalcFragmentsInFrustum_Node(const QMap::DRAW_NODE *Node, FRAGMENT_DESC_VECTOR *OutFragments, const FRUSTUM_PLANES &FrustumPlanes, bool InsideFrustum);
	void CalcFragmentsInPointLight_Node(const QMap::DRAW_NODE *Node, FRAGMENT_DESC_VECTOR OutFragments[6], PointLight &point_light);
	void CalcFragmentsToSweptBox(const QMap::DRAW_NODE *Node, FRAGMENT_DESC_VECTOR *OutFragments, const BOX &CamBox, const VEC3 &LightDir);
	bool RayCollisionToNode(const QMap::COLLISION_NODE *Node, const VEC3 &RayOrig, const VEC3 &RayDir, float *InOutT);
};

QMapRenderer::QMapRenderer(Scene *OwnerScene, QMap *Map) :
	m_OwnerScene(OwnerScene),
	m_Map(Map)
{
	m_Map->Lock();

	// Pobierz wskaŸniki do materia³ów
	m_Materials.resize(m_Map->GetMaterialCount());
	for (uint mi = 0; mi < m_Map->GetMaterialCount(); mi++)
		m_Materials[mi] = OwnerScene->GetMaterialCollection().MustGetByNameEx<OpaqueMaterial>(m_Map->GetMaterialName(mi));
}

QMapRenderer::~QMapRenderer()
{
	m_Map->Unlock();
}

bool QMapRenderer::RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT)
{
	if (Type == 0) return false;
	*OutT = MAXFLOAT;
	return RayCollisionToNode(m_Map->GetCollisionTree(), RayOrig, RayDir, OutT);
}

void QMapRenderer::CalcFragmentsInFrustum(FRAGMENT_DESC_VECTOR *OutFragments, const FRUSTUM_PLANES &FrustumPlanes)
{
	OutFragments->clear();
	CalcFragmentsInFrustum_Node(m_Map->GetDrawTree(), OutFragments, FrustumPlanes, false);
}

void QMapRenderer::CalcFragmentsInSpotLight(FRAGMENT_DESC_VECTOR *OutFragments, SpotLight &spot_light)
{
	OutFragments->clear();
	// Tu te¿ da siê wykorzystaæ metodê t¹ sam¹ co do frustuma kamery, bo te¿ kolizja jest z frustumem.
	CalcFragmentsInFrustum_Node(m_Map->GetDrawTree(), OutFragments, spot_light.GetFrustumPlanes(), false);
}

void QMapRenderer::CalcFragmentsInPointLight(FRAGMENT_DESC_VECTOR OutFragments[6], PointLight &point_light)
{
	for (uint i = 0; i < 6; i++)
		OutFragments[i].clear();
	CalcFragmentsInPointLight_Node(m_Map->GetDrawTree(), OutFragments, point_light);
}

void QMapRenderer::CalcFragmentsInDirectionalLight(FRAGMENT_DESC_VECTOR *OutFragments, const FRUSTUM_PLANES &CamFrustum, const BOX &CamBox, const VEC3 &LightDir)
{
	OutFragments->clear();
	CalcFragmentsToSweptBox(m_Map->GetDrawTree(), OutFragments, CamBox, LightDir);
}

void QMapRenderer::SortFragments(FRAGMENT_DESC_VECTOR *Fragments)
{
	std::sort(Fragments->begin(), Fragments->end());
}

void QMapRenderer::DrawFragment(PASS Pass, const FRAGMENT_DESC &Fragment, const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam, BaseLight *Light)
{
	assert(m_Map->GetDrawVB() != NULL);
	assert(m_Map->GetDrawIB() != NULL);

	frame::Dev->SetStreamSource(0, m_Map->GetDrawVB(), 0, sizeof(QMap::DRAW_VERTEX));
	frame::Dev->SetIndices(m_Map->GetDrawIB());
	frame::Dev->SetFVF(QMap::DRAW_VERTEX::FVF);

	g_Engine->GetServices()->SetupState_Material(
		Pass,
		SceneDrawParams,
		Cam,
		*Fragment.Material,
		ENTITY_DRAW_PARAMS(1.f, MATRIX::IDENTITY, MATRIX::IDENTITY, MATRIX::IDENTITY, COLORF::WHITE, true, 0, NULL),
		Light);

	uint PrimitiveCount = (Fragment.MapFragment->IndexEnd - Fragment.MapFragment->IndexBegin) / 3;
	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST,
		0,
		Fragment.MapFragment->VertexBegin,
		Fragment.MapFragment->VertexEnd - Fragment.MapFragment->VertexBegin,
		Fragment.MapFragment->IndexBegin,
		PrimitiveCount) );
	frame::RegisterDrawCall(PrimitiveCount);

	g_Engine->GetServices()->UnsetupState();
}

void QMapRenderer::CalcFragmentsInFrustum_Node(const QMap::DRAW_NODE *Node, FRAGMENT_DESC_VECTOR *OutFragments, const FRUSTUM_PLANES &FrustumPlanes, bool InsideFrustum)
{
	// Tu jest taka sprytna sztuczka, ¿e jeœli jakis wêze³ w ca³oœci zawiera siê we frustumie,
	// to jego podwêz³y rekurencyjnie nie s¹ ju¿ testowane AABB-Frustum, bo te¿ na pewno siê
	// w nim zawieraj¹.

	// Dodaj te co s¹ w tym wêŸle
	for (uint fi = 0; fi < Node->Fragments.size(); fi++)
		OutFragments->push_back(FRAGMENT_DESC(&Node->Fragments[fi], m_Materials[Node->Fragments[fi].MaterialIndex], &Node->Bounds));

	// Rozpatrz podwêz³y
	if (Node->SubNodes[0] != NULL)
	{
		for (uint si = 0; si < 8; si++)
		{
			if (InsideFrustum || BoxInFrustum(Node->SubNodes[si]->Bounds, FrustumPlanes))
				CalcFragmentsInFrustum_Node(Node->SubNodes[si], OutFragments, FrustumPlanes, true);
			else if (BoxToFrustum_Fast(Node->SubNodes[si]->Bounds, FrustumPlanes))
				CalcFragmentsInFrustum_Node(Node->SubNodes[si], OutFragments, FrustumPlanes, false);
		}
	}
}

void QMapRenderer::CalcFragmentsInPointLight_Node(const QMap::DRAW_NODE *Node, FRAGMENT_DESC_VECTOR OutFragments[6], PointLight &point_light)
{
	// Realizowana jest jedynie kolizja ze sfer¹, bez sprawdzania 6 kierunków.

	// Dodaj te co s¹ w tym wêŸle
	for (uint fi = 0; fi < Node->Fragments.size(); fi++)
		for (uint i = 0; i < 6; i++)
			OutFragments[i].push_back(FRAGMENT_DESC(&Node->Fragments[fi], m_Materials[Node->Fragments[fi].MaterialIndex], &Node->Bounds));

	// Rozpatrz podwêz³y
	if (Node->SubNodes[0] != NULL)
	{
		for (uint si = 0; si < 8; si++)
		{
			if (SphereToBox(point_light.GetPos(), point_light.GetDist(), Node->SubNodes[si]->Bounds))
				CalcFragmentsInPointLight_Node(Node->SubNodes[si], OutFragments, point_light);
		}
	}
}

void QMapRenderer::CalcFragmentsToSweptBox(const QMap::DRAW_NODE *Node, FRAGMENT_DESC_VECTOR *OutFragments, const BOX &CamBox, const VEC3 &LightDir)
{
	// Dodaj te co s¹ w tym wêŸle
	for (uint fi = 0; fi < Node->Fragments.size(); fi++)
		OutFragments->push_back(FRAGMENT_DESC(&Node->Fragments[fi], m_Materials[Node->Fragments[fi].MaterialIndex], &Node->Bounds));

	// Rozpatrz podwêz³y
	if (Node->SubNodes[0] != NULL)
	{
		float T;
		for (uint si = 0; si < 8; si++)
		{
			if (SweptBoxToBox(Node->SubNodes[si]->Bounds, CamBox, LightDir, &T) && T >= 0.f)
				CalcFragmentsToSweptBox(Node->SubNodes[si], OutFragments, CamBox, LightDir);
		}
	}
}

bool QMapRenderer::RayCollisionToNode(const QMap::COLLISION_NODE *Node, const VEC3 &RayOrig, const VEC3 &RayDir, float *InOutT)
{
	float t;
	bool Found = false;
	const VEC3 *CollisionVB = m_Map->GetCollisionVB();
	const uint *CollisionIB = m_Map->GetCollisionIB();

	// Przejrzyj trójk¹ty z tego wêz³a
	for (uint vi = Node->IndexBegin; vi < Node->IndexEnd; vi += 3)
	{
		if (RayToTriangle(
			RayOrig,
			RayDir,
			CollisionVB[CollisionIB[vi  ]],
			CollisionVB[CollisionIB[vi+1]],
			CollisionVB[CollisionIB[vi+2]],
			true,
			&t))
		{
			if (t >= 0.f && t < *InOutT)
			{
				*InOutT = t;
				Found = true;
			}
		}
	}

	// Przejrzyj podwêz³y
	if (Node->SubNodes[0] != NULL)
	{
		for (uint si = 0; si < 8; si++)
		{
			if (RayToBox(&t, RayOrig, RayDir, Node->SubNodes[si]->Bounds)) // Otrzymane t jest nieu¿ywane.
			{
				if (RayCollisionToNode(Node->SubNodes[si], RayOrig, RayDir, InOutT))
					Found = true;
			}
		}
	}

	return Found;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TerrainRenderer

const uint TERRAIN_PATCH_SIZE = 32;
const uint TREE_PATCH_CACHE_SIZE = 32;

enum TERRAIN_EFFECT_PARAMS
{
	TEP_WORLDVIEWPROJ = 0,
	TEP_TEXTURE0,
	TEP_TEXTURE1,
	TEP_TEXTURE2,
	TEP_TEXTURE3,
	TEP_TEXSCALE,
};

TerrainRenderer::TREE_DATA::TREE_DATA(uint Index, shared_ptr<Tree> Tree_, uint KindCount, COLOR DensityMapColor) :
	Index(Index),
	Tree_(Tree_),
	KindCount(KindCount),
	DensityMapColor(DensityMapColor)
{
}

TerrainRenderer::TREE_DATA::~TREE_DATA()
{
}

TerrainMaterial::TerrainMaterial(Scene *OwnerScene) :
	BaseMaterial(OwnerScene, "TerrainMaterial")
{
	// Tak tylko dla formalnoœci
	for (uint i = 0; i < TERRAIN_FORMS_PER_PATCH; i++)
	{
		Textures[i] = NULL;
		TexScale[i] = 1.0f;
	}
}

TerrainRenderer::TerrainRenderer(Scene *OwnerScene, Terrain *terrain, Grass *GrassObj, TerrainWater *WaterObj,
	const string &TreeDescFileName, const string &TreeDensityMapFileName)
:
	m_OwnerScene(OwnerScene),
	m_Terrain(terrain),
	m_GrassObj(GrassObj),
	m_WaterObj(WaterObj)
{
	LOG(LOG_ENGINE, Format("Creating TerrainRenderer: Desc=\"#\", DensityMap=\"#\"") % TreeDescFileName % TreeDensityMapFileName);

	m_Terrain->Lock();

	m_Material.reset(new TerrainMaterial(OwnerScene));

	m_TreeDensityMapCX = m_TreeDensityMapCY = 0;
	m_TreePatchesX = m_TreePatchesZ = 0;

	if (!TreeDescFileName.empty())
	{
		LoadTreeDescFile(TreeDescFileName);
		if (!TreeDensityMapFileName.empty())
			LoadTreeDensityMap(TreeDensityMapFileName);
	}
}

TerrainRenderer::~TerrainRenderer()
{
	m_Terrain->Unlock();
}

void TerrainRenderer::DrawPatch(PASS Pass, uint PatchIndex, const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam, BaseLight *Light)
{
	string PatchFormTextureNames[TERRAIN_FORMS_PER_PATCH];
	m_Terrain->GetPatchFormTextureNames(PatchIndex, PatchFormTextureNames);
	for (uint ti = 0; ti < TERRAIN_FORMS_PER_PATCH; ti++)
		m_Material->Textures[ti] = res::g_Manager->MustGetResourceEx<res::D3dTexture>(PatchFormTextureNames[ti]);

	m_Terrain->GetPatchTexScale(PatchIndex, m_Material->TexScale);

/*	for (uint ti = 0; ti < TERRAIN_FORMS_PER_PATCH; ti++)
	{
		res::D3dTexture *texture = res::g_Manager->MustGetResourceEx<res::D3dTexture>(PatchFormTextureNames[ti]);
		texture->Load();
		m_Effect->Get()->SetTexture(m_Effect->GetParam(TEP_TEXTURE0+ti), texture->GetBaseTexture());
	}
	D3DXVECTOR4 TexScaleVec;
	TexScaleVec.x = TexScale[0];
	TexScaleVec.y = TexScale[1];
	TexScaleVec.z = TexScale[2];
	TexScaleVec.w = TexScale[3];
	m_Effect->Get()->SetVector(m_Effect->GetParam(TEP_TEXSCALE), &TexScaleVec);
	m_Effect->Get()->CommitChanges();*/

	g_Engine->GetServices()->SetupState_Material(
		Pass,
		SceneDrawParams,
		Cam,
		*m_Material.get(),
		ENTITY_DRAW_PARAMS(1.f, MATRIX::IDENTITY, MATRIX::IDENTITY, MATRIX::IDENTITY, COLORF::WHITE, true, 0, NULL),
		Light);

	// Ustaw VB, IB i FVF
	m_Terrain->SetupVbIbFvf();
	// Narysuj geometriê
	m_Terrain->DrawPatchGeometry(PatchIndex, Cam->GetEyePos(), Cam->GetZFar());

	g_Engine->GetServices()->UnsetupState();
}

void TerrainRenderer::DrawGrass(const ParamsCamera &Cam, const COLORF &Color)
{
	if (m_GrassObj != NULL)
		m_GrassObj->Draw(Cam, g_Engine->GetConfigFloat(Engine::O_GRASS_ZFAR), Color);
}

void TerrainRenderer::DrawWater(const ParamsCamera &Cam)
{
	if (m_WaterObj != NULL)
		m_WaterObj->Draw(Cam);
}

Tree * TerrainRenderer::GetTreeByName(const string &Name)
{
	std::map<string, uint>::iterator tit = m_TreeNames.find(Name);
	if (tit == m_TreeNames.end())
		return NULL;
	return &GetTreeByIndex(tit->second);
}

Tree & TerrainRenderer::MustGetTreeByName(const string &Name)
{
	Tree *ptr = GetTreeByName(Name);
	if (ptr == NULL)
		throw Error(Format("Nie znaleziono drzewa \"#\".") % Name, __FILE__, __LINE__);
	return *ptr;
}

void TerrainRenderer::GetTreesInFrustum(TREE_DRAW_DESC_VECTOR *InOut, const ParamsCamera &Cam, bool FrustumCulling)
{
	if (m_TreeDensityMapCX == 0 || m_TreeDensityMapCY == 0)
		return;
	assert(m_TreePatchesX > 0);
	assert(m_TreePatchesZ > 0);
	assert(!m_TreeDensityMap.empty());

	BOX FrustumBox; Cam.GetMatrices().GetFrustumPoints().CalcBoundingBox(&FrustumBox);
	float PatchCX, PatchCZ;
	PatchCX = PatchCZ = TERRAIN_PATCH_SIZE * m_Terrain->GetVertexDistance();

	int x1 = minmax(0, roundo(floorf(FrustumBox.p1.x / PatchCX)), (int)m_TreePatchesX-1);
	int z1 = minmax(0, roundo(floorf(FrustumBox.p1.z / PatchCZ)), (int)m_TreePatchesZ-1);
	int x2 = minmax(0, roundo(ceilf(FrustumBox.p2.x / PatchCX)), (int)m_TreePatchesX-1);
	int z2 = minmax(0, roundo(ceilf(FrustumBox.p2.z / PatchCZ)), (int)m_TreePatchesZ-1);
	int x, z;
	uint ti;
	float TreeHalfWidth, TreeHalfHeight; BOX TreeBox;
	for (z = z1; z <= z2; z++)
	{
		for (x = x1; x <= x2; x++)
		{
			const PATCH &Patch = EnsurePatch(x, z);
			for (ti = 0; ti < Patch.TreeDescs.size(); ti++)
			{
				const TREE_DRAW_DESC &Tree = Patch.TreeDescs[ti];
				if (!FrustumCulling)
					InOut->push_back(Tree);
				else
				{
					TreeHalfWidth  = Tree.tree->GetDesc().HalfWidth;
					TreeHalfHeight = Tree.tree->GetDesc().HalfHeight;
					// Frustum Culling
					TreeBox.p1.x = Tree.world._41 - TreeHalfWidth * Tree.scaling;
					TreeBox.p1.y = Tree.world._42;
					TreeBox.p1.z = Tree.world._43 - TreeHalfWidth * Tree.scaling;
					TreeBox.p2.x = Tree.world._41 + TreeHalfWidth * Tree.scaling;
					TreeBox.p2.y = Tree.world._42 + TreeHalfHeight * 2.0f * Tree.scaling;
					TreeBox.p2.z = Tree.world._43 + TreeHalfWidth * Tree.scaling;
					if (BoxToFrustum_Fast(TreeBox, Cam.GetMatrices().GetFrustumPlanes()))
						InOut->push_back(Tree);
				}
			}
		}
	}
}

void TerrainRenderer::GetTreesCastingDirectionalShadow(TREE_DRAW_DESC_VECTOR *InOut, const FRUSTUM_PLANES &CamFrustum, const BOX &CamBox, const VEC3 &LightDir)
{
	// Ta funkcja jest niedok³adna, ale nie mia³em pomys³u jak napisaæ dok³adny a wydajny algorytm
	// na wyznaczanie drzew rzucaj¹cych cieñ na podany frustum.

	if (m_TreeDensityMapCX == 0 || m_TreeDensityMapCY == 0)
		return;
	assert(m_TreePatchesX > 0);
	assert(m_TreePatchesZ > 0);
	assert(!m_TreeDensityMap.empty());

	float PatchCX, PatchCZ;
	PatchCX = PatchCZ = TERRAIN_PATCH_SIZE * m_Terrain->GetVertexDistance();

	BOX ExtendedCamBox = CamBox;
	ExtendedCamBox.Extend(2.0f); // HACK

	int x1 = minmax(0, roundo(floorf(CamBox.p1.x / PatchCX)), (int)m_TreePatchesX-1);
	int z1 = minmax(0, roundo(floorf(CamBox.p1.z / PatchCZ)), (int)m_TreePatchesZ-1);
	int x2 = minmax(0, roundo(ceilf(CamBox.p2.x / PatchCX)), (int)m_TreePatchesX-1);
	int z2 = minmax(0, roundo(ceilf(CamBox.p2.z / PatchCZ)), (int)m_TreePatchesZ-1);
	int x, z;
	uint ti;
	float TreeHalfWidth, TreeHalfHeight; BOX TreeBox;
	for (z = z1; z <= z2; z++)
	{
		for (x = x1; x <= x2; x++)
		{
			const PATCH &Patch = EnsurePatch(x, z);
			for (ti = 0; ti < Patch.TreeDescs.size(); ti++)
			{
				const TREE_DRAW_DESC &Tree = Patch.TreeDescs[ti];
				TreeHalfWidth  = Tree.tree->GetDesc().HalfWidth;
				TreeHalfHeight = Tree.tree->GetDesc().HalfHeight;

				TreeBox.p1.x = Tree.world._41 - TreeHalfWidth * Tree.scaling;
				TreeBox.p1.y = Tree.world._42;
				TreeBox.p1.z = Tree.world._43 - TreeHalfWidth * Tree.scaling;
				TreeBox.p2.x = Tree.world._41 + TreeHalfWidth * Tree.scaling;
				TreeBox.p2.y = Tree.world._42 + TreeHalfHeight * 2.0f * Tree.scaling;
				TreeBox.p2.z = Tree.world._43 + TreeHalfWidth * Tree.scaling;
				if (OverlapBox(ExtendedCamBox, TreeBox))
					InOut->push_back(Tree);
			}
		}
	}
}

void TerrainRenderer::LoadTreeDescFile(const string &TreeDescFileName)
{
	ERR_TRY;

	FileStream fs(TreeDescFileName, FM_READ);
	Tokenizer tok(&fs, 0);
	tok.Next();

	////// TreeTextureDesc

	std::map<string, TreeTextureDesc> TextureDescMap;

	tok.AssertIdentifier("Textures");
	tok.Next();
	tok.AssertSymbol('{');
	tok.Next();

	string TexDescName;
	TreeTextureDesc TexDesc;
	while (!tok.QuerySymbol('}'))
	{
		// Nazwa
		tok.AssertToken(Tokenizer::TOKEN_STRING);
		TexDescName = tok.GetString();
		tok.Next();

		// TreeTextureDesc
		TexDesc.LoadFromTokenizer(tok);

		TextureDescMap.insert(std::pair<string, TreeTextureDesc>(TexDescName, TexDesc));
	}
	tok.Next();

	////// TREE_DESC

	tok.AssertIdentifier("Trees");
	tok.Next();
	tok.AssertSymbol('{');
	tok.Next();

	string TreeName;
	uint KindCount;
	COLOR Color;
	shared_ptr<Tree> Tree_;
	TREE_DESC TreeDesc;
	std::vector<uint> Seeds;
	while (!tok.QuerySymbol('}'))
	{
		tok.AssertIdentifier("Tree");
		tok.Next();

		// TexDesc
		tok.AssertIdentifier("Texture");
		tok.Next();
		tok.AssertSymbol('=');
		tok.Next();
		tok.AssertToken(Tokenizer::TOKEN_STRING);
		std::map<string, TreeTextureDesc>::iterator tdit = TextureDescMap.find(tok.GetString());
		if (tdit == TextureDescMap.end())
			throw Error(Format("Nie odnaleziono definicji tekstury \"#\".") % tok.GetString());
		tok.Next();

		// Name
		if (tok.QueryIdentifier("Name"))
		{
			tok.Next();
			tok.AssertSymbol('=');
			tok.Next();
			tok.AssertToken(Tokenizer::TOKEN_STRING);
			TreeName = tok.GetString();
			tok.Next();
		}
		else
			TreeName.clear();

		// Kinds
		if (tok.QueryIdentifier("Kinds"))
		{
			tok.Next();
			tok.AssertSymbol('=');
			tok.Next();
			KindCount = tok.MustGetUint4();
			tok.Next();
		}
		else
			KindCount = 1;

		// DensityMapColor
		if (tok.QueryIdentifier("DensityMapColor"))
		{
			tok.Next();
			tok.AssertSymbol('=');
			tok.Next();
			Color.ARGB = tok.MustGetUint4();
			Color.A = 255;
			tok.Next();
		}
		else
			Color.ARGB = 0;

		// TREE_DESC
		TreeDesc.LoadFromTokenizer(tok);

		// Wylosuj ziarna pseudolosowe
		Seeds.resize(KindCount);
		for (uint si = 0; si < KindCount; si++)
			Seeds[si] = g_Rand.RandUint();

		// Utwórz Tree
		Tree_.reset(new Tree(m_OwnerScene, TreeDesc, tdit->second, KindCount, &Seeds[0]));

		// Dodaj do kolekcji
		uint Index = m_Trees.size();
		m_Trees.push_back(TREE_DATA(Index, Tree_, KindCount, Color));
		if (!TreeName.empty())
			m_TreeNames.insert(std::pair<string, uint>(TreeName, Index));
	}
	tok.Next();

	tok.AssertEOF();

	ERR_CATCH(Format("Nie mo¿na wczytaæ pliku z opisem drzew z \"#\".") % TreeDescFileName);
}

void TerrainRenderer::LoadTreeDensityMap(const string &FileName)
{
	// Tu jest kosztowny algorytm zamiany tekstury Form Mapy na tablicê, mo¿naby to keszowaæ.

	ERR_TRY;

	MemoryTexture mem_tex(FileName);

	m_TreeDensityMapCX = mem_tex.GetWidth();
	m_TreeDensityMapCY = mem_tex.GetHeight();
	m_TreePatchesX = ceil_div<uint>(m_TreeDensityMapCX, TERRAIN_PATCH_SIZE);
	m_TreePatchesZ = ceil_div<uint>(m_TreeDensityMapCY, TERRAIN_PATCH_SIZE);
	if (m_TreeDensityMapCX * m_TreeDensityMapCY == 0)
		throw Error("Mapa rozmieszczenia drzew jest pusta (rozmiar 0x0).", __FILE__, __LINE__);
	m_TreeDensityMap.resize(m_TreeDensityMapCX * m_TreeDensityMapCY);
	FillMem(&m_TreeDensityMap[0], m_TreeDensityMap.size(), 0xFF);

	// Tablica wszystkich TREE_DATA, tak jak m_Trees, ale wymieszane dla optymalizacji (kolejnoœæ nie ma znaczenia).
	if (m_Trees.empty())
		throw Error("Brak zdefiniowanych drzew posiadaj¹cych kolory.", __FILE__, __LINE__);
	if (m_Trees.size() > 255)
		throw Error("Zbyt wiele rodzajów drzew.", __FILE__, __LINE__);
	std::vector<TREE_DATA*> Trees;
	for (uint i = 0; i < m_Trees.size(); i++)
		if (m_Trees[i].DensityMapColor.A == 255)
			Trees.push_back(&m_Trees[i]);

	// Jazda!
	uint x, z, map_i, i;
	const COLOR * mem_tex_row;
	COLOR color;
	for (z = 0, map_i = 0; z < m_TreeDensityMapCY; z++)
	{
		mem_tex_row = mem_tex.GetRowData(z);

		for (x = 0; x < m_TreeDensityMapCX; x++)
		{
			color = mem_tex_row[x];

			// 1. Spróbuj dopasowaæ kolor bezpoœrednio
			for (i = 0; i < Trees.size(); i++)
			{
				if ( (Trees[i]->DensityMapColor.ARGB & 0x00FFFFFF) == (color.ARGB & 0x00FFFFFF) )
				{
					m_TreeDensityMap[map_i] = (uint1)Trees[i]->Index;

					// Optymalizacja, bo nastêpne piksele te¿ pewnie bêd¹ tego samego koloru wiêc znajdzie siê szybciej.
					if (i > 0)
						std::swap(Trees[i], Trees[0]);

					break;
				}
			}

			map_i++;
		}
	}

	ERR_CATCH(Format("Nie mo¿na wczytaæ mapy rozmieszczenia drzew z \"#\".") % FileName);
}

const TerrainRenderer::PATCH & TerrainRenderer::EnsurePatch(uint px, uint pz)
{
	////// Poszukaj czy ju¿ jest

	for (uint pi = m_PatchCache.size(); pi--; )
	{
		if (m_PatchCache[pi]->px == px && m_PatchCache[pi]->pz == pz)
		{
			// Zamieñ z ostatnim
			if (pi < m_PatchCache.size()-1)
				std::swap(m_PatchCache[pi], m_PatchCache[m_PatchCache.size()-1]);
			// Zwróæ znaleziony
			return *m_PatchCache[m_PatchCache.size()-1].get();
		}
	}

	////// Nie ma

	// Wygeneruj
	shared_ptr<PATCH> Patch(new PATCH);
	Patch->px = px;
	Patch->pz = pz;
	GeneratePatch(Patch.get());
	// Usuñ najdawniej u¿ywany jeœli za du¿o
	if (m_PatchCache.size() >= TREE_PATCH_CACHE_SIZE)
		m_PatchCache.erase(m_PatchCache.begin());
	// Dodaj na koniec
	m_PatchCache.push_back(Patch);
	// Zwróæ
	return *Patch.get();
}

void TerrainRenderer::GeneratePatch(PATCH *P)
{
	RandomGenerator rand(P->px * (TERRAIN_PATCH_SIZE-3) + P->pz);

	uint xc = std::min(m_TreeDensityMapCX, (P->px + 1) * TERRAIN_PATCH_SIZE);
	uint zc = std::min(m_TreeDensityMapCY, (P->pz + 1) * TERRAIN_PATCH_SIZE);
	uint x, z;
	TREE_DRAW_DESC desc;
	uint1 TreeIndex;
	float xf, zf;
	MATRIX tm, rm, sm;
	for (z = P->pz * TERRAIN_PATCH_SIZE; z < zc; z++)
	{
		for (x = P->px * TERRAIN_PATCH_SIZE; x < xc; x++)
		{
			TreeIndex = m_TreeDensityMap[z * m_TreeDensityMapCX + x];
			if (TreeIndex < 0xFF)
			{
				desc.tree = &GetTreeByIndex((uint)TreeIndex);
				desc.kind = rand.RandUint(desc.tree->GetKindCount());

				desc.color = COLORF(
					rand.RandFloat(0.8f, 1.0f),
					rand.RandFloat(0.8f, 1.0f),
					rand.RandFloat(0.8f, 1.0f),
					1.0f);
				desc.scaling = rand.RandFloat(0.9f, 1.1f);

				xf = ( (float)x + rand.RandFloat(-0.4f, 0.4f) ) * m_Terrain->GetVertexDistance();
				zf = ( (float)z + rand.RandFloat(-0.4f, 0.4f) ) * m_Terrain->GetVertexDistance();
				Translation(&tm, xf, m_Terrain->GetHeight(xf, zf), zf);
				RotationY(&rm, rand.RandFloat(PI_X_2));
				Scaling(&sm, desc.scaling);
				desc.world = sm * rm * tm;

				Inverse(&desc.world_inv, desc.world);

				P->TreeDescs.push_back(desc);
			}
		}
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa HeatEntity

void HeatEntity::Draw(const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam)
{
	ERR_TRY

	g_Engine->GetServices()->SetupState_Heat(SceneDrawParams, Cam, this);

	DrawGeometry();

	g_Engine->GetServices()->UnsetupState();

	ERR_CATCH(Format("HeatSphere.Draw: Type=#") % typeid(*this).name());
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TreeEntity

TreeEntity::TreeEntity(Scene *OwnerScene) :
	Entity(OwnerScene),
		m_Tree(NULL),
		m_Kind(0),
		m_Color(COLORF::WHITE)
{
}

TreeEntity::TreeEntity(Scene *OwnerScene, Tree *a_Tree, uint Kind, const COLORF &Color) :
	Entity(OwnerScene),
	m_Tree(a_Tree),
	m_Kind(Kind),
	m_Color(Color)
{
	if (a_Tree)
		SetRadius(Length(VEC3(a_Tree->GetDesc().HalfWidth, a_Tree->GetDesc().HalfHeight*2.0f, a_Tree->GetDesc().HalfWidth)));
}

void TreeEntity::SetTree(Tree *a_Tree)
{
	m_Tree = a_Tree;
	if (a_Tree)
		SetRadius(Length(VEC3(a_Tree->GetDesc().HalfWidth, a_Tree->GetDesc().HalfHeight*2.0f, a_Tree->GetDesc().HalfWidth)));
	// else - zostaje stary Radius
}

bool TreeEntity::RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT)
{
	if (GetTree() == NULL)
		return false;

	float Length = GetTree()->GetDesc().Levels[0].Length;
	float HalfWidth = GetTree()->GetDesc().Levels[0].Radius;

	BOX Box(-HalfWidth, 0.0f, -HalfWidth, HalfWidth, Length, HalfWidth);
	return RayToBox(OutT, RayOrig, RayDir, Box);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa HeatSphere

const uint4 HEAT_SPHERE_STACKS = 10;
const uint4 HEAT_SPHERE_SLICES = 10;


void HeatSphere::FillBuffers()
{
	ERR_TRY;

	uint4 Diffuse = (uint4)(m_Intensity * 255.f + 0.5f) << 24;

	uint4 x, y, i = 0;

	{
		res::D3dVertexBuffer::Locker vb_lock(m_VB, true, 0);
		VERTEX_XND *m_Vertices = (VERTEX_XND*)vb_lock.GetData();

		float ax = 0.f, ax_step = PI_X_2 / (float)HEAT_SPHERE_SLICES;
		float ay, ay_step = PI / (float)(HEAT_SPHERE_STACKS-1);
		float axs, axc, ays, ayc;
		VEC3 Right;
		VEC3 RadiusInv = VEC3(1.f / m_Radius.x, 1.f / m_Radius.y, 1.f / m_Radius.z);
		for (x = 0; x <= HEAT_SPHERE_SLICES; x++)
		{
			ay = -PI_2;
			axs = sinf(ax);
			axc = cosf(ax);

			for (y = 0; y < HEAT_SPHERE_STACKS; y++)
			{
				ays = sinf(ay);
				ayc = cosf(ay);

				m_Vertices[i].Pos = VEC3(
					m_Radius.x * ayc * axc,
					m_Radius.y * ays,
					m_Radius.z * ayc * axs);
				m_Vertices[i].Diffuse = Diffuse;
				// Dó³
				if (y == 0)
					m_Vertices[i].Normal = VEC3::NEGATIVE_Y;
				// Góra
				else if (y == HEAT_SPHERE_STACKS-1)
					m_Vertices[i].Normal = VEC3::POSITIVE_Y;
				// Gdzieœ w œrodku na Y
				else
				{
					m_Vertices[i].Normal.x = m_Vertices[i].Pos.x * RadiusInv.x;
					m_Vertices[i].Normal.y = m_Vertices[i].Pos.y * RadiusInv.y;
					m_Vertices[i].Normal.z = m_Vertices[i].Pos.z * RadiusInv.z;
					Normalize(&m_Vertices[i].Normal);
				}
				ay += ay_step;
				i++;
			}
			ax += ax_step;
		}
	}

	i = 0;

	{
		res::D3dIndexBuffer::Locker ib_lock(m_IB, true, 0);
		uint2 *m_Indices = (uint2*)ib_lock.GetData();

		for (x = 0; x < HEAT_SPHERE_SLICES; x++)
		{
			for (y = 0; y < HEAT_SPHERE_STACKS-1; y++)
			{
				m_Indices[i++] = (x  ) * HEAT_SPHERE_SLICES + (y  );
				m_Indices[i++] = (x  ) * HEAT_SPHERE_SLICES + (y+1);
				m_Indices[i++] = (x+1) * HEAT_SPHERE_SLICES + (y+1);
				m_Indices[i++] = (x  ) * HEAT_SPHERE_SLICES + (y  );
				m_Indices[i++] = (x+1) * HEAT_SPHERE_SLICES + (y+1);
				m_Indices[i++] = (x+1) * HEAT_SPHERE_SLICES + (y  );
			}
		}
	}

	m_VB->SetFilled();
	m_IB->SetFilled();

	ERR_CATCH_FUNC;
}

void HeatSphere::DrawHeatGeometry()
{
}

HeatSphere::HeatSphere(Scene *OwnerScene, const VEC3 &Radius, float Intensity) :
	HeatEntity(OwnerScene),
	m_Intensity(Intensity),
	m_Radius(Radius),
	m_VB(NULL),
	m_IB(NULL)
{
	ERR_TRY

	SetRadius(max3(Radius.x, Radius.y, Radius.z)); // To wa¿ne!

	uint4 vb_length = (HEAT_SPHERE_SLICES+1)*HEAT_SPHERE_STACKS;
	uint4 ib_length = HEAT_SPHERE_SLICES * (HEAT_SPHERE_STACKS-1) * 6;

	m_VB = new res::D3dVertexBuffer(string(), string(), vb_length, 0, VERTEX_XND::FVF, D3DPOOL_MANAGED);
	m_IB = new res::D3dIndexBuffer(string(), string(), ib_length, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED);

	ERR_CATCH_FUNC
}

HeatSphere::~HeatSphere()
{
	SAFE_DELETE(m_IB);
	SAFE_DELETE(m_VB);
}

void HeatSphere::DrawGeometry()
{
	if (!m_VB->IsFilled() || !m_IB->IsFilled())
		FillBuffers();

	uint4 PrimitiveCount = m_IB->GetLength() / 3;

	frame::Dev->SetStreamSource(0, m_VB->GetVB(), 0, m_VB->GetVertexSize());
	frame::Dev->SetIndices(m_IB->GetIB());
	frame::Dev->SetFVF(VERTEX_XND::FVF);

	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST,
		0,
		0,
		m_VB->GetLength(),
		0,
		PrimitiveCount) );
	frame::RegisterDrawCall(PrimitiveCount);

#ifdef DEBUG
	if (FAILED(hr)) throw DirectXError(hr, "HeatSphere.DrawHeatGeometry: DrawIndexedPrimitiveUP failed.", __FILE__, __LINE__);
#endif
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Scene

// Funktor do porównywania fragmentów encji wg materia³ow
struct EntityFragment_MaterialCompare
{
	bool operator () (const ENTITY_FRAGMENT &ef1, const ENTITY_FRAGMENT &ef2)
	{
		return ef1.M < ef2.M;
	}
};

class EntityFragment_DistanceCompare_Greater
{
private:
	VEC3 m_Eye;

public:
	EntityFragment_DistanceCompare_Greater(const VEC3 &Eye) : m_Eye(Eye) { }

	bool operator () (const ENTITY_FRAGMENT &ef1, const ENTITY_FRAGMENT &ef2)
	{
		return DistanceSq(ef1.Pos, m_Eye) > DistanceSq(ef2.Pos, m_Eye);
	}
};

struct DRAW_DATA
{
	// Etap 1

	std::vector<MaterialEntity*> MaterialEntities;
	std::vector<CustomEntity*> CustomEntities;
	std::vector<HeatEntity*> HeatEntities;
	std::vector<TREE_DRAW_DESC> Trees;
	QMapRenderer::FRAGMENT_DESC_VECTOR MapFragments;
	std::vector<uint> TerrainPatches;

	// Œwiat³a
	std::vector<SpotLight*> SpotLights;
	std::vector<PointLight*> PointLights;

	// Etap 2

	// Fragmenty MaterialEntity materia³em Opaque
	std::vector<ENTITY_FRAGMENT> OpaqueEntityFragments;
	// Fragmenty MaterialEntity materia³em Wireframe, Translucent oraz encje Custom w ca³oœci
	std::vector<ENTITY_FRAGMENT> TranslucentEntityFragments;

	void SortOpaqueEntityFragmentsByMaterial();
	// Sortuje od najdalszych do najbli¿szych
	void SortTranslucentEntityFragmentssByDistance(const VEC3 &EyePos);
	// Sortuje drzewa w Trees wg wskaŸników na obiekty klasy Tree
	void SortTrees();
};

void DRAW_DATA::SortOpaqueEntityFragmentsByMaterial()
{
	std::sort(OpaqueEntityFragments.begin(), OpaqueEntityFragments.end(), EntityFragment_MaterialCompare());
}

void DRAW_DATA::SortTranslucentEntityFragmentssByDistance(const VEC3 &EyePos)
{
	std::sort(TranslucentEntityFragments.begin(), TranslucentEntityFragments.end(), EntityFragment_DistanceCompare_Greater(EyePos));
}

void DRAW_DATA::SortTrees()
{
	std::sort(Trees.begin(), Trees.end());
}

Scene::Scene(const BOX &Bounds) :
	m_Destroying(false),
	m_RunningOptimizer(new RunningOptimizer),
	m_Fog_Enabled(false),
	m_Fog_Start(0.5f),
	m_Fog_Color(COLORF::BLACK),
	m_AmbientColor(COLORF::BLACK),
	m_ActiveCamera(NULL),
	m_Octree(new EntityOctree(Bounds)),
	m_DirectionalLight(NULL),
	m_MapUseLighting(true),
	m_MapCastShadow(true),
	m_MapReceiveShadow(true),
	m_WindVec(VEC2::ZERO)
{
}

Scene::~Scene()
{
	m_Destroying = true;

	// Zwolnij wszystkie œwiat³a, których wczeœniej nie zwolni³ u¿ytkownik
	SAFE_DELETE(m_DirectionalLight);
	for (std::list<SpotLight*>::reverse_iterator lit = m_SpotLights.rbegin(); lit != m_SpotLights.rend(); ++lit)
		delete *lit;
	m_SpotLights.clear();
	for (std::list<PointLight*>::reverse_iterator lit = m_PointLights.rbegin(); lit != m_PointLights.rend(); ++lit)
		delete *lit;
	m_PointLights.clear();

	// Zwolnij wszystkie encje, których wczeœniej nie zwolni³ u¿ytkownik
	// Usuwam tylko te g³ównego poziomu, one usun¹ swoje pod-encje
	for (ENTITY_SET::iterator it = m_RootEntities.begin(); it != m_RootEntities.end(); ++it)
		delete *it;
	m_RootEntities.clear();
	m_Octree.reset();

	// Zwolnij wszystkie kamery, których wczeœniej nie zwolni³ u¿ytkownik
	m_ActiveCamera = NULL;
	for (CAMERA_SET::iterator it = m_Cameras.begin(); it != m_Cameras.end(); ++it)
		delete *it;
	m_Cameras.clear();

	// Jeœli to by³a aktywna scena w silniku, przestaw na brak
	if (g_Engine->GetActiveScene() == this)
		g_Engine->SetActiveScene(NULL);
}

void Scene::SetSky(BaseSky *Sky)
{
	m_Sky.reset(Sky);
}

BaseSky * Scene::GetSky()
{
	return m_Sky.get();
}

void Scene::ResetSky()
{
	m_Sky.reset();
}

void Scene::SetFall(Fall *a_Fall)
{
	m_Fall.reset(a_Fall);
}

Fall * Scene::GetFall()
{
	return m_Fall.get();
}

void Scene::ResetFall()
{
	m_Fall.reset();
}

void Scene::SetQMap(QMap *Map)
{
	m_QMapRenderer.reset(new QMapRenderer(this, Map));
}

void Scene::SetQMap(const string &QMapName)
{
	SetQMap(res::g_Manager->MustGetResourceEx<QMap>(QMapName));
}

void Scene::ResetQMap()
{
	m_QMapRenderer.reset();
}

void Scene::SetTerrain(TerrainRenderer *terrain_renderer)
{
	m_TerrainRenderer.reset(terrain_renderer);
}

void Scene::ResetTerrain()
{
	m_TerrainRenderer.reset();
}

void Scene::SetActiveCamera(Camera *cam)
{

	if (cam == NULL)
		m_ActiveCamera = NULL;
	else
	{
		assert(cam->GetOwner() == this);
		m_ActiveCamera = cam;
	}
}

void Scene::SetPpColor(PpColor *e) { m_PpColor.reset(e); }
PpColor * Scene::GetPpColor() { return m_PpColor.get(); }
void Scene::SetPpTexture(PpTexture *e) { m_PpTexture.reset(e); }
PpTexture * Scene::GetPpTexture() { return m_PpTexture.get(); }
void Scene::SetPpFunction(PpFunction *e) { m_PpFunction.reset(e); }
PpFunction * Scene::GetPpFunction() { return m_PpFunction.get(); }
void Scene::SetPpToneMapping(PpToneMapping *e) { m_PpToneMapping.reset(e); }
PpToneMapping * Scene::GetPpToneMapping() { return m_PpToneMapping.get(); }
void Scene::SetPpBloom(PpBloom *e) { m_PpBloom.reset(e); }
PpBloom * Scene::GetPpBloom() { return m_PpBloom.get(); }
void Scene::SetPpFeedback(PpFeedback *e) { m_PpFeedback.reset(e); }
PpFeedback * Scene::GetPpFeedback() { return m_PpFeedback.get(); }
void Scene::SetPpLensFlare(PpLensFlare *e) { m_PpLensFlare.reset(e); }
PpLensFlare * Scene::GetPpLensFlare() { return m_PpLensFlare.get(); }

void Scene::Update()
{
	float dt = frame::Timer1.GetDeltaTime();

	for (ENTITY_SET::iterator eit = m_AllEntities.begin(); eit != m_AllEntities.end(); ++eit)
		(*eit)->Update();
}

void Scene::Draw(STATS *OutStats)
{
	RunningOptimizer::SETTINGS OptimizerSettings;
	m_RunningOptimizer->OnFrame(&OptimizerSettings, frame::Timer1.GetDeltaTime());
	m_RunningOptimizer->SettingsToStr(&OutStats->RunningOptimizerOptions, OptimizerSettings);

	if (m_ActiveCamera == NULL)
		return;

	DRAW_DATA DrawData;
	CreateDrawData(&DrawData, OptimizerSettings);

	uint4 ScreenCX = frame::Settings.BackBufferWidth;
	uint4 ScreenCY = frame::Settings.BackBufferHeight;

	// Inicjalizacja struktur z danymi do narysowania

	// W ogóle nie ma postprocessingu
	if (!g_Engine->GetConfigBool(Engine::O_PP_ENABLED))
	{
		// Prosto na ekran
		DrawAll(DrawData, OutStats, OptimizerSettings);
	}
	else
	{
		// Czy bêdzie potrzebny BackBuffer Surface?
		bool BackBufferSurfaceRequired =
			GetPpFeedback() != NULL;
		scoped_ptr<IDirect3DSurface9, ReleasePolicy> BackBufferSurface;
		if (BackBufferSurfaceRequired)
		{
			IDirect3DSurface9 *BackBufferSurfacePtr;
			frame::Dev->GetRenderTarget(0, &BackBufferSurfacePtr);
			BackBufferSurface.reset(BackBufferSurfacePtr);
		}

		// Czy bêdzie potrzebny render do osobnej tekstury?
		bool RenderToTextureRequired =
			GetPpFunction() != NULL ||
			GetPpToneMapping() != NULL ||
			GetPpBloom() != NULL ||
			!DrawData.HeatEntities.empty();

		VEC3 SunDir; bool UseLensFlare = false; float SunVisibleFactor;
		if (GetPpLensFlare() && m_Sky != NULL && m_Sky->GetSunDir(&SunDir))
			UseLensFlare = true;

		// Prosto na ekran
		if (!RenderToTextureRequired)
		{
			// Narysuj encje na ekran
			DrawAll(DrawData, OutStats, OptimizerSettings);
			// Occlusion Query dla Lens Flare
			if (UseLensFlare)
				SunVisibleFactor = QuerySunVisibleFactor(m_ActiveCamera->GetParams(), SunDir);
		}
		// Tekstura
		else
		{
			res::D3dTextureSurface *ScreenTexture = g_Engine->GetServices()->EnsureSpecialTexture(EngineServices::ST_SCREEN);

			// Render do tekstury
			{
				RenderTargetHelper rth(ScreenTexture->GetSurface(), NULL);

				DrawAll(DrawData, OutStats, OptimizerSettings);

				// Occlusion Query dla Lens Flare
				if (UseLensFlare)
					SunVisibleFactor = QuerySunVisibleFactor(m_ActiveCamera->GetParams(), SunDir);

				// Heat Haze
				if (!DrawData.HeatEntities.empty())
				{
					// Wyczyœæ kana³ alfa
					PpEffect::ClearAlpha(0, ScreenCX, ScreenCY);
					// Narysuj encje ciep³a
					DrawHeatEntities(DrawData);
				}

				ScreenTexture->SetFilled();
			}

			// Tone Mapping
			if (GetPpToneMapping())
			{
				GetPpToneMapping()->CalcBrightness(
					ScreenTexture,
					g_Engine->GetServices()->EnsureSpecialTexture(EngineServices::ST_TONE_MAPPING_VRAM),
					g_Engine->GetServices()->EnsureSpecialTexture(EngineServices::ST_TONE_MAPPING_RAM));
			}

			// Bloom
			if (GetPpBloom())
			{
				GetPpBloom()->CreateBloom(
					ScreenTexture,
					g_Engine->GetServices()->EnsureSpecialTexture(EngineServices::ST_BLUR_1),
					g_Engine->GetServices()->EnsureSpecialTexture(EngineServices::ST_BLUR_2));
			}

			// Przepisanie na ekran z shaderem PpShader
			// Tu zak³adam, ¿e zawsze jak jest render do tylnej tekstury to jest i koniecznoœæ u¿ycia shader PpShader.
			frame::RestoreDefaultState();
			g_Engine->GetServices()->SetupState_Pp(ScreenTexture->GetTexture(), GetPpFunction(), GetPpToneMapping(), !DrawData.HeatEntities.empty());
			DrawFullscreenQuad(ScreenCX, ScreenCY, !DrawData.HeatEntities.empty());
			g_Engine->GetServices()->UnsetupState();

			// Bloom
			if (GetPpBloom())
			{
				GetPpBloom()->DrawBloom(
					g_Engine->GetServices()->EnsureSpecialTexture(EngineServices::ST_BLUR_1),
					ScreenCX, ScreenCY);
			}
		}

		// Feedback
		if (GetPpFeedback())
		{
			GetPpFeedback()->Draw(
				BackBufferSurface.get(), ScreenCX, ScreenCY,
				g_Engine->GetServices()->EnsureSpecialTexture(EngineServices::ST_FEEDBACK),
				m_ActiveCamera->GetParams().GetEyePos(), m_ActiveCamera->GetMatrices().GetViewProj());
		}

		// Lens Flare
		if (UseLensFlare)
		{
			SunVisibleFactor *= std::max(0.0f, SunDir.y);
			if (SunVisibleFactor > 0.01f)
				GetPpLensFlare()->Draw(SunVisibleFactor, ScreenCX, ScreenCY, m_ActiveCamera->GetMatrices().GetView(), m_ActiveCamera->GetMatrices().GetProj());
		}

		// Postprocessing - efekt Texture
		if (m_PpTexture != NULL)
			m_PpTexture->Draw(ScreenCX, ScreenCY);

		// Postprocessing - efekt Color
		if (m_PpColor != NULL)
			m_PpColor->Draw(ScreenCX, ScreenCY);
	}

	OutStats->MainShaders = g_Engine->GetServices()->GetMainShaderCount();
	OutStats->PpShaders = g_Engine->GetServices()->GetPpShaderCount();
}

Scene::COLLISION_RESULT Scene::RayCollision(COLLISION_TYPE CollisionType, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT, Entity **OutEntity)
{
	COLLISION_RESULT R = COLLISION_RESULT_NONE;
	float t;
	*OutT = MAXFLOAT;

	// Mapa
	if (m_QMapRenderer != NULL)
	{
		if (m_QMapRenderer->RayCollision(CollisionType, RayOrig, RayDir, &t))
		{
			if (R == COLLISION_RESULT_NONE || t < *OutT)
			{
				R = COLLISION_RESULT_MAP;
				*OutT = t;
			}
		}
	}

	// Teren
	if (m_TerrainRenderer != NULL)
	{
		if (m_TerrainRenderer->GetTerrain()->RayCollision(RayOrig, RayDir, &t, *OutT))
		{
			if (R == COLLISION_RESULT_NONE || t < *OutT)
			{
				R = COLLISION_RESULT_TERRAIN;
				*OutT = t;
			}
		}
	}

	// Encje
	if (m_Octree->RayCollision(OutEntity, &t, CollisionType, RayOrig, RayDir) && t >= 0.f && t < *OutT)
	{
		R = COLLISION_RESULT_ENTITY;
		*OutT = t;
		// OutEntity ju¿ przypisane
	}

	return R;
}

void Scene::RegisterCamera(Camera *cam)
{
	assert(cam != NULL);
	assert(cam->GetOwner() == this);

	m_Cameras.insert(cam);
}

void Scene::UnregisterCamera(Camera *cam)
{
	assert(cam != NULL);
	assert(cam->GetOwner() == this);

	if (!m_Destroying)
	{
		if (cam == m_ActiveCamera)
			m_ActiveCamera = NULL;

		CAMERA_SET::iterator it = m_Cameras.find(cam);
		assert(it != m_Cameras.end());
		m_Cameras.erase(it);
	}
}

void Scene::RegisterEntity(Entity *e)
{
	m_AllEntities.insert(e);
	m_Octree->AddEntity(e);
}

void Scene::UnregisterEntity(Entity *e)
{
	if (!m_Destroying)
	{
		m_Octree->RemoveEntity(e);

		ENTITY_SET::iterator eit = m_AllEntities.find(e);
		assert(eit != m_AllEntities.end());
		m_AllEntities.erase(eit);
	}
}

void Scene::RegisterRootEntity(Entity *e)
{
	m_RootEntities.insert(e);
}

void Scene::UnregisterRootEntity(Entity *e)
{
	if (!m_Destroying)
	{
		ENTITY_SET::iterator it = m_RootEntities.find(e);
		assert(it != m_RootEntities.end());
		m_RootEntities.erase(it);
	}
}

void Scene::OnEntityParamsChange(Entity *e)
{
	assert(!m_Destroying);
	m_Octree->OnEntityParamsChange(e);
}

void Scene::RegisterDirectionalLight(DirectionalLight *l)
{
	if (m_DirectionalLight != NULL)
		throw Error("Co najwy¿ej jedno œwiat³o kierunkowe dozwolone w scenie.", __FILE__, __LINE__);
	m_DirectionalLight = l;
}

void Scene::UnregisterDirectionalLight(DirectionalLight *l)
{
	if (!m_Destroying)
	{
		assert(m_DirectionalLight == l);
		m_DirectionalLight = NULL;
	}
}

void Scene::RegisterSpotLight(SpotLight *l)
{
	m_SpotLights.push_back(l);
}

void Scene::UnregisterSpotLight(SpotLight *l)
{
	if (!m_Destroying)
	{
		std::list<SpotLight*>::iterator lit = std::find(m_SpotLights.begin(), m_SpotLights.end(), l);
		assert(lit != m_SpotLights.end());
		m_SpotLights.erase(lit);
	}
}

void Scene::RegisterPointLight(PointLight *l)
{
	m_PointLights.push_back(l);
}

void Scene::UnregisterPointLight(PointLight *l)
{
	if (!m_Destroying)
	{
		std::list<PointLight*>::iterator lit = std::find(m_PointLights.begin(), m_PointLights.end(), l);
		assert(lit != m_PointLights.end());
		m_PointLights.erase(lit);
	}
}

void Scene::DoShadowMapping_Spot(SpotLight &spot_light, float ScreenSizePercent, res::D3dTextureSurface **OutShadowMap, MATRIX *OutShadowMapMatrix, bool OptimizeSetting_MaterialSort)
{
	// Jeœli jest mapa i rzuca cieñ, wyznacz fragmenty mapy w zasiêgu pola widzenia œwiat³a
	QMapRenderer::FRAGMENT_DESC_VECTOR MapFragments;
	if (m_QMapRenderer != NULL && GetMapCastShadow())
	{
		m_QMapRenderer->CalcFragmentsInSpotLight(&MapFragments, spot_light);
		// Sortowanie
		if (OptimizeSetting_MaterialSort)
			m_QMapRenderer->SortFragments(&MapFragments);
	}

	// Wyznacz encje Visible, MaterialEntity, w zasiêgu pola widzenia œwiat³a i które rzucaj¹ cieñ
	ENTITY_VECTOR Entities;
	m_Octree->FindEntities_SpotLight(&Entities, spot_light);

	// Wyznacz te fragmenty tych encji, które s¹ materia³em OpaqueMaterial
	std::vector<ENTITY_FRAGMENT> EntityFragments;
	for (uint ei = 0; ei < Entities.size(); ei++)
	{
		MaterialEntity & me = static_cast<MaterialEntity&>(*Entities[ei]);
		uint FragmentCount = me.GetFragmentCount();
		for (uint fi = 0; fi < FragmentCount; fi++)
		{
			ENTITY_FRAGMENT ef = me.GetFragment(fi);
			assert(ef.E == &me);
			assert(ef.M != NULL);
			assert(ef.Pos == me.GetWorldPos());
			if (ef.M->GetType() == BaseMaterial::TYPE_OPAQUE)
				EntityFragments.push_back(ef);
		}
	}
	// Sortowanie
	if (OptimizeSetting_MaterialSort)
		std::sort(EntityFragments.begin(), EntityFragments.end(), EntityFragment_MaterialCompare());

	// Pobie¿ Shadow Mapê
	res::D3dTextureSurface *sm_render_target = g_Engine->GetServices()->GetSpotShadowMap(ScreenSizePercent);
	res::D3dTextureSurface *sm_depth_stencil = g_Engine->GetServices()->GetShadowMappingDepthStencil();

	{
		// Przestaw Render Target
		RenderTargetHelper _rth(sm_render_target->GetSurface(), sm_depth_stencil->GetSurface());

		// Wyczyœæ Shadow Mapê i jej Depth Stencil
		frame::Dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0);

		// Wyrenderuj fragmenty mapy do Shadow Mapy
		for (uint fi = 0; fi < MapFragments.size(); fi++)
			m_QMapRenderer->DrawFragment(PASS_SHADOW_MAP, MapFragments[fi], SCENE_DRAW_PARAMS(this), &spot_light.GetParamsCamera(), &spot_light);

		// Wyrenderuj nieprzezroczyste fragmenty encji matria³owych do Shadow Mapy
		for (uint fi = 0; fi < EntityFragments.size(); fi++)
			static_cast<MaterialEntity*>(EntityFragments[fi].E)->DrawFragment(PASS_SHADOW_MAP, EntityFragments[fi], SCENE_DRAW_PARAMS(this), &spot_light.GetParamsCamera(), &spot_light);
	}

	// Zwróæ dane
	MATRIX m2;
	g_Engine->GetServices()->CalcShadowMapMatrix2(&m2);
	*OutShadowMap = sm_render_target;
	*OutShadowMapMatrix = spot_light.GetParamsCamera().GetMatrices().GetViewProj() * m2;
}

void Scene::DoShadowMapping_Point(PointLight &point_light, float ScreenSizePercent, res::D3dCubeTextureSurface **OutShadowMap, MATRIX *OutShadowMapMatrix, bool OptimizeSetting_MaterialSort)
{
	// Jeœli jest mapa i rzuca cieñ, wyznacz fragmenty mapy w zasiêgu pola widzenia œwiat³a
	QMapRenderer::FRAGMENT_DESC_VECTOR MapFragments[6];
	if (m_QMapRenderer != NULL && GetMapCastShadow())
	{
		m_QMapRenderer->CalcFragmentsInPointLight(MapFragments, point_light);
		// Sortowanie
		if (OptimizeSetting_MaterialSort)
		{
			for (uint i = 0; i < 6; i++)
				m_QMapRenderer->SortFragments(&MapFragments[i]);
		}
	}

	// Wyznacz encje Visible, MaterialEntity, w zasiêgu pola widzenia œwiat³a i które rzucaj¹ cieñ
	ENTITY_VECTOR Entities[6];
	m_Octree->FindEntities_PointLight(Entities, point_light);

	// Wyznacz te fragmenty tych encji, które s¹ materia³em OpaqueMaterial
	std::vector<ENTITY_FRAGMENT> EntityFragments[6];
	for (uint i = 0; i < 6; i++)
	{
		for (uint ei = 0; ei < Entities[i].size(); ei++)
		{
			MaterialEntity & me = static_cast<MaterialEntity&>(*Entities[i][ei]);
			uint FragmentCount = me.GetFragmentCount();
			for (uint fi = 0; fi < FragmentCount; fi++)
			{
				ENTITY_FRAGMENT ef = me.GetFragment(fi);
				assert(ef.E == &me);
				assert(ef.M != NULL);
				assert(ef.Pos == me.GetWorldPos());
				if (ef.M->GetType() == BaseMaterial::TYPE_OPAQUE)
					EntityFragments[i].push_back(ef);
			}
		}
	}
	// Sortowanie
	if (OptimizeSetting_MaterialSort)
	{
		for (uint i = 0; i < 6; i++)
			std::sort(EntityFragments[i].begin(), EntityFragments[i].end(), EntityFragment_MaterialCompare());
	}

	// Pobie¿ Shadow Mapê
	res::D3dCubeTextureSurface *sm_render_target = g_Engine->GetServices()->GetPointShadowMap(ScreenSizePercent);
	res::D3dTextureSurface *sm_depth_stencil = g_Engine->GetServices()->GetShadowMappingDepthStencil();

	for (uint i = 0; i < 6; i++)
	{
		// Przestaw Render Target
		RenderTargetHelper _rth(sm_render_target->GetSurface((D3DCUBEMAP_FACES)i), sm_depth_stencil->GetSurface());

		// Wyczyœæ Shadow Mapê i jej Depth Stencil
		frame::Dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0);

		// Wyrenderuj fragmenty mapy do Shadow Mapy
		for (uint fi = 0; fi < MapFragments[i].size(); fi++)
			m_QMapRenderer->DrawFragment(PASS_SHADOW_MAP, MapFragments[i][fi], SCENE_DRAW_PARAMS(this), &point_light.GetParamsCamera((D3DCUBEMAP_FACES)i), &point_light);

		// Wyrenderuj nieprzezroczyste fragmenty encji matria³owych do Shadow Mapy
		for (uint fi = 0; fi < EntityFragments[i].size(); fi++)
			static_cast<MaterialEntity*>(EntityFragments[i][fi].E)->DrawFragment(PASS_SHADOW_MAP, EntityFragments[i][fi], SCENE_DRAW_PARAMS(this), &point_light.GetParamsCamera((D3DCUBEMAP_FACES)i), &point_light);
	}

	// Zwróæ dane
	*OutShadowMap = sm_render_target;
	Identity(OutShadowMapMatrix); // Bêdzie przez ni¹ mno¿ona macierz œwiata encji - i o to chodzi!
}

void Scene::DoShadowMapping_Directional(DirectionalLight &directional_light, res::D3dTextureSurface **OutShadowMap, MATRIX *OutShadowMapMatrix, bool OptimizeSetting_MaterialSort)
{
	// Zbuduj ewentualny nowy, mniejszy frustum kamery obejmuj¹cy tyle, co zasiêg tego œwiat³a z ograniczonym ZFar
	ParamsCamera NewCamera;
	const ParamsCamera *CameraPtr;

	if (directional_light.GetZFar() < m_ActiveCamera->GetParams().GetZFar())
	{
		NewCamera.Set(
			m_ActiveCamera->GetParams().GetEyePos(),
			m_ActiveCamera->GetParams().GetForwardDir(),
			m_ActiveCamera->GetParams().GetUpDir(),
			m_ActiveCamera->GetParams().GetFovY(),
			m_ActiveCamera->GetParams().GetAspect(),
			m_ActiveCamera->GetParams().GetZNear(),
			directional_light.GetZFar());
		CameraPtr = &NewCamera;
	}
	else
		CameraPtr = &m_ActiveCamera->GetParams();

	const FRUSTUM_PLANES &NewCamFrustum = CameraPtr->GetMatrices().GetFrustumPlanes();
	const BOX &NewCamBox = CameraPtr->GetMatrices().GetFrustumBox();

	// Jeœli jest mapa i rzuca cieñ, wyznacz fragmenty mapy w zasiêgu pola widzenia œwiat³a
	// Czyli koliduj¹ce w sposób Swept z bry³¹ nowej kamery.
	QMapRenderer::FRAGMENT_DESC_VECTOR MapFragments;
	if (m_QMapRenderer != NULL && GetMapCastShadow())
	{
		m_QMapRenderer->CalcFragmentsInDirectionalLight(&MapFragments, NewCamFrustum, NewCamBox, directional_light.GetDir());
		// Sortowanie
		if (OptimizeSetting_MaterialSort)
			m_QMapRenderer->SortFragments(&MapFragments);
	}

	// Wyznacz encje Visible, MaterialEntity lub TreeEntity, CastShadow, które koliduj¹ ze œwiat³em w sposób Swept.
	ENTITY_VECTOR Entities;
	m_Octree->FindEntities_DirectionalLight(&Entities, NewCamFrustum, NewCamBox, directional_light.GetDir());

	// Wyznacz te fragmenty tych encji, które s¹ materia³em OpaqueMaterial.
	// Oraz opisy drzew z TreeEntity.
	std::vector<ENTITY_FRAGMENT> EntityFragments;
	std::vector<TREE_DRAW_DESC> Trees;
	TREE_DRAW_DESC tree_desc;
	MaterialEntity *me;
	TreeEntity *te;
	for (uint ei = 0; ei < Entities.size(); ei++)
	{
		me = dynamic_cast<MaterialEntity*>(Entities[ei]);
		if (me)
		{
			uint FragmentCount = me->GetFragmentCount();
			for (uint fi = 0; fi < FragmentCount; fi++)
			{
				ENTITY_FRAGMENT ef = me->GetFragment(fi);
				assert(ef.E == me);
				assert(ef.M != NULL);
				assert(ef.Pos == me->GetWorldPos()); // Jeœli ta asercja siê sypie, to znaczy najpewniej, ¿e jako pozycja encji wpisany zosta³ NaN.
				if (ef.M->GetType() == BaseMaterial::TYPE_OPAQUE)
					EntityFragments.push_back(ef);
			}
		}
		else
		{
			te = dynamic_cast<TreeEntity*>(Entities[ei]);
			if (te)
			{
				tree_desc.tree = te->GetTree();
				if (tree_desc.tree != NULL)
				{
					tree_desc.kind = te->GetKind();
					tree_desc.scaling = te->GetWorldSize();
					tree_desc.world = te->GetWorldMatrix();
					tree_desc.world_inv = te->GetInvWorldMatrix();
					tree_desc.color = te->GetColor();
					Trees.push_back(tree_desc);
				}
			}
			else
				assert("WTF?! - DoShadowMapping_Directional, encja nie jest MaterialEntity ani TreeEntity.");
		}
	}

	// Drzewa z terenu
	if (m_TerrainRenderer != NULL)
		m_TerrainRenderer->GetTreesCastingDirectionalShadow(&Trees, NewCamFrustum, NewCamBox, directional_light.GetDir());

	// Sortowanie
	if (OptimizeSetting_MaterialSort)
		std::sort(EntityFragments.begin(), EntityFragments.end(), EntityFragment_MaterialCompare());
	std::sort(Trees.begin(), Trees.end());

	// U³ó¿ macierz ViewProj
	MATRIX ViewProj;
	FindDirectionalViewProjMatrix(&ViewProj, NewCamBox, directional_light.GetDir());

	// Pobie¿ Shadow Mapê
	res::D3dTextureSurface *sm_render_target = g_Engine->GetServices()->GetDirectionalShadowMap();
	res::D3dTextureSurface *sm_depth_stencil = g_Engine->GetServices()->GetShadowMappingDepthStencil();

	{
		// Przestaw Render Target
		RenderTargetHelper _rth(sm_render_target->GetSurface(), sm_depth_stencil->GetSurface());

		// Wyczyœæ Shadow Mapê i jej Depth Stencil
		frame::Dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0);

		// Wyrenderuj fragmenty mapy do Shadow Mapy
		for (uint fi = 0; fi < MapFragments.size(); fi++)
			m_QMapRenderer->DrawFragment(PASS_SHADOW_MAP, MapFragments[fi], SCENE_DRAW_PARAMS(this, (res::D3dTextureSurface*)NULL, &ViewProj), NULL, &directional_light);

		// Wyrenderuj nieprzezroczyste fragmenty encji matria³owych do Shadow Mapy
		for (uint fi = 0; fi < EntityFragments.size(); fi++)
			static_cast<MaterialEntity*>(EntityFragments[fi].E)->DrawFragment(PASS_SHADOW_MAP, EntityFragments[fi], SCENE_DRAW_PARAMS(this, (res::D3dTextureSurface*)NULL, &ViewProj), NULL, &directional_light);

		// Wyrenderuj drzewa do Shadow Mapy
		Tree *last_tree = NULL;
		for (uint ti = 0; ti < Trees.size(); ti++)
		{
			const TREE_DRAW_DESC &tree_desc = Trees[ti];
			assert(tree_desc.tree != NULL);
			if (tree_desc.tree != last_tree)
			{
				if (last_tree != NULL)
					last_tree->DrawEnd();
				last_tree = tree_desc.tree;
				last_tree->DrawBegin_ShadowMap(ViewProj, directional_light.GetDir());
			}
			last_tree->DrawTree(tree_desc.kind, tree_desc.world, &tree_desc.world_inv, tree_desc.color);
		}
		if (last_tree != NULL)
			last_tree->DrawEnd();
	}

	// Zwróæ dane
	MATRIX m2;
	g_Engine->GetServices()->CalcShadowMapMatrix2(&m2);
	*OutShadowMap = sm_render_target;
	*OutShadowMapMatrix = ViewProj * m2;
}

void Scene::CreateDrawData(DRAW_DATA *OutDrawData, const RunningOptimizer::SETTINGS &OptimizerSettings)
{
	OutDrawData->MaterialEntities.clear();
	OutDrawData->CustomEntities.clear();
	OutDrawData->HeatEntities.clear();
	OutDrawData->Trees.clear();

	ENTITY_VECTOR Entities;
	m_Octree->FindEntities_Frustum(&Entities, m_ActiveCamera->GetMatrices().GetFrustumPlanes());

	MaterialEntity *me;
	CustomEntity *ce;
	HeatEntity *he;
	TreeEntity *te;
	TREE_DRAW_DESC tree_desc;

	for (uint i = 0; i < Entities.size(); i++)
	{
		if ((me = dynamic_cast<MaterialEntity*>(Entities[i])) != NULL)
			OutDrawData->MaterialEntities.push_back(me);
		else if ((ce = dynamic_cast<CustomEntity*>(Entities[i])) != NULL)
			OutDrawData->CustomEntities.push_back(ce);
		else if ((he = dynamic_cast<HeatEntity*>(Entities[i])) != NULL)
			OutDrawData->HeatEntities.push_back(he);
		else if ((te = dynamic_cast<TreeEntity*>(Entities[i])) != NULL)
		{
			if (te->GetTree() != NULL)
			{
				tree_desc.tree = te->GetTree();
				tree_desc.scaling = te->GetWorldSize();
				tree_desc.kind = te->GetKind();
				tree_desc.world = te->GetWorldMatrix();
				tree_desc.world_inv = te->GetInvWorldMatrix();
				tree_desc.color = te->GetColor();
				OutDrawData->Trees.push_back(tree_desc);
			}
		}
		else
			assert(0 && "Scene::CreateDrawData: Nieznana klasa pochodna Entity.");
	}

	// Fragmenty mapy
	OutDrawData->MapFragments.clear();
	if (m_QMapRenderer != NULL)
	{
		m_QMapRenderer->CalcFragmentsInFrustum(&OutDrawData->MapFragments, m_ActiveCamera->GetMatrices().GetFrustumPlanes());
		m_QMapRenderer->SortFragments(&OutDrawData->MapFragments);
	}

	// Fragmenty terenu
	OutDrawData->TerrainPatches.clear();
	if (m_TerrainRenderer != NULL)
	{
		// Patche heightmapy
		m_TerrainRenderer->GetTerrain()->CalcVisiblePatches(
			&OutDrawData->TerrainPatches,
			m_ActiveCamera->GetMatrices().GetFrustumPlanes(),
			m_ActiveCamera->GetMatrices().GetFrustumPoints());
		// Drzewa
		m_TerrainRenderer->GetTreesInFrustum(&OutDrawData->Trees, m_ActiveCamera->GetParams(), OptimizerSettings[RunningOptimizer::SETTING_TREE_FRUSTUM_CULLING]);
	}

	// Œwiat³a
	OutDrawData->SpotLights.clear();
	OutDrawData->PointLights.clear();
	// Jeœli jest w ogóle w³¹czone oœwietlenie
	if (g_Engine->GetConfigBool(Engine::O_LIGHTING))
	{
		// Œwiat³a punktowe
		for (std::list<PointLight*>::iterator lit = m_PointLights.begin(); lit != m_PointLights.end(); ++lit)
		{
			PointLight & point_light = **lit;
			// Jeœli jest aktywne i nieczarne
			if (point_light.GetEnabled() && point_light.GetColor() != COLORF::BLACK)
			{
				// Jeœli jego zasiêg jest widoczny we frustumie widzenia
				if (SphereToFrustum_Fast(point_light.GetPos(), point_light.GetDist(), m_ActiveCamera->GetMatrices().GetFrustumPlanes()))
				{
					OutDrawData->PointLights.push_back(&point_light);
				}
			}
		}

		// Œwiat³a Spot
		for (std::list<SpotLight*>::iterator lit = m_SpotLights.begin(); lit != m_SpotLights.end(); ++lit)
		{
			SpotLight & spot_light = **lit;
			// Jeœli jest aktywne i nieczarne
			if (spot_light.GetEnabled() && spot_light.GetColor() != COLORF::BLACK)
			{
				// Jeœli jego zasiêg jest widoczny we frustumie widzenia
				if (SphereToFrustum_Fast(spot_light.GetPos(), spot_light.GetDist(), m_ActiveCamera->GetMatrices().GetFrustumPlanes()) &&
					FrustumToFrustum(spot_light.GetFrustumPlanes(), spot_light.GetFrustumPoints(), m_ActiveCamera->GetMatrices().GetFrustumPlanes(), m_ActiveCamera->GetMatrices().GetFrustumPoints()))
				{
					OutDrawData->SpotLights.push_back(&spot_light);
				}
			}
		}
	}
}

void Scene::DrawHeatEntities(const DRAW_DATA &DrawData)
{
	if (DrawData.HeatEntities.empty())
		return;

	SCENE_DRAW_PARAMS SceneDrawParams(this);

	for (uint hei = 0; hei < DrawData.HeatEntities.size(); hei++)
		DrawData.HeatEntities[hei]->Draw(SceneDrawParams, &m_ActiveCamera->GetParams());
}

void Scene::DrawFullscreenQuad(uint4 ScreenCX, uint4 ScreenCY, bool FillPerturbationTex)
{
	struct VERTEX
	{
		VEC4 Pos;
		VEC2 Tex;
		VEC2 PerturbationTex[2];
	};

	VERTEX V[4];
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

	// Wspó³rzêdne dla Perturbation Map
	if (FillPerturbationTex)
	{
		float Aspect = (float)ScreenCX / (float)ScreenCY;
		const float PERTURBATION_MAP_SCALING = 1.5f; // Wzglêdem wysokoœci ekranu
		const float PERTURBATION_MAP_SPEED = 0.1f;

		float PerturbationOffsetBase = frame::Timer1.GetTime() * PERTURBATION_MAP_SPEED;
		VEC2 PerturbationOffset;

		PerturbationOffset = POISSON_DISC_2D[0] - VEC2(0.5f, 0.5f);
		Normalize(&PerturbationOffset);
		PerturbationOffset *= PerturbationOffsetBase;
		PerturbationOffset.x = frac(PerturbationOffset.x);
		PerturbationOffset.y = frac(PerturbationOffset.y);
		V[0].PerturbationTex[0].x = PerturbationOffset.x;
		V[0].PerturbationTex[0].y = PerturbationOffset.y;
		V[1].PerturbationTex[0].x = V[0].PerturbationTex[0].x + PERTURBATION_MAP_SCALING * Aspect;
		V[1].PerturbationTex[0].y = V[0].PerturbationTex[0].y;
		V[2].PerturbationTex[0].x = V[0].PerturbationTex[0].x;
		V[2].PerturbationTex[0].y = V[0].PerturbationTex[0].y + PERTURBATION_MAP_SCALING;
		V[3].PerturbationTex[0].x = V[1].PerturbationTex[0].x;
		V[3].PerturbationTex[0].y = V[2].PerturbationTex[0].y;

		PerturbationOffset = POISSON_DISC_2D[1] - VEC2(0.5f, 0.5f);
		Normalize(&PerturbationOffset);
		PerturbationOffset *= PerturbationOffsetBase;
		PerturbationOffset.x = frac(PerturbationOffset.x);
		PerturbationOffset.y = frac(PerturbationOffset.y);
		V[0].PerturbationTex[1].x = PerturbationOffset.x;
		V[0].PerturbationTex[1].y = PerturbationOffset.y;
		V[1].PerturbationTex[1].x = V[0].PerturbationTex[1].x + PERTURBATION_MAP_SCALING * Aspect;
		V[1].PerturbationTex[1].y = V[0].PerturbationTex[1].y;
		V[2].PerturbationTex[1].x = V[0].PerturbationTex[1].x;
		V[2].PerturbationTex[1].y = V[0].PerturbationTex[1].y + PERTURBATION_MAP_SCALING;
		V[3].PerturbationTex[1].x = V[1].PerturbationTex[1].x;
		V[3].PerturbationTex[1].y = V[2].PerturbationTex[1].y;
	}

	frame::Dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX3);
	frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(VERTEX));
	frame::RegisterDrawCall(2);
}

void Scene::OcclusionQuery_Entities(DRAW_DATA &DrawData)
{
	// HACK!
	// Zabezpieczenie przeciwko zbyt du¿ej liczbie Queries, bo mi siê wtedy z niewiadomych przyczyn wysypuje
	// z b³êdem C++ Bad Alloc.
	if (DrawData.MaterialEntities.size() + DrawData.CustomEntities.size() + DrawData.HeatEntities.size() > 50)
		return;

	VEC3 EyePos = m_ActiveCamera->GetParams().GetEyePos();

	// U³ó¿ zaptania occlusion query dla poszczególnych typów encji
	std::vector<res::OcclusionQueries::Query*> MaterialQueries;
	std::vector<res::OcclusionQueries::Query*> CustomQueries;
	std::vector<res::OcclusionQueries::Query*> HeatQueries;

	MaterialQueries.resize(DrawData.MaterialEntities.size());
	CustomQueries.resize(DrawData.CustomEntities.size());
	HeatQueries.resize(DrawData.HeatEntities.size());

	res::D3dEffect *effect = g_Engine->GetServices()->GetOcclusionQueryEffect();
	IcosphereRes *icosphere = g_Engine->GetServices()->GetIcosphere();
	res::OcclusionQueries *occlusion_queries = g_Engine->GetServices()->GetOcclusionQueries();

	frame::RestoreDefaultState(); // Od wszelkiego...
	effect->Get()->SetMatrix(effect->GetParam(1), &math_cast<D3DXMATRIX>(m_ActiveCamera->GetMatrices().GetView()));
	effect->Get()->SetMatrix(effect->GetParam(2), &math_cast<D3DXMATRIX>(m_ActiveCamera->GetMatrices().GetProj()));
	effect->Begin(true);
	icosphere->SetState();

	MATRIX ms, mt;
	for (uint ei = 0; ei < DrawData.MaterialEntities.size(); ei++)
	{
		// utwórz zapytanie
		MaterialQueries[ei] = occlusion_queries->Create();
		// utwórz transformacjê dla sfery occlusion query
		Scaling(&ms, DrawData.MaterialEntities[ei]->GetWorldRadius());
		Translation(&mt, DrawData.MaterialEntities[ei]->GetWorldPos());
		// ustaw stany per-obiekt
		effect->Get()->SetMatrix(effect->GetParam(0), &math_cast<D3DXMATRIX>(ms*mt));
		effect->Get()->CommitChanges();
		// zapytaj zapytanie
		{
			res::OcclusionQueries::Issue query_issue_obj(MaterialQueries[ei]);
			icosphere->DrawGeometry();
		}
	}
	for (uint ei = 0; ei < DrawData.CustomEntities.size(); ei++)
	{
		// utwórz zapytanie
		CustomQueries[ei] = occlusion_queries->Create();
		// utwórz transformacjê dla sfery occlusion query
		Scaling(&ms, DrawData.CustomEntities[ei]->GetWorldRadius());
		Translation(&mt, DrawData.CustomEntities[ei]->GetWorldPos());
		// ustaw stany per-obiekt
		effect->Get()->SetMatrix(effect->GetParam(0), &math_cast<D3DXMATRIX>(ms*mt));
		effect->Get()->CommitChanges();
		// zapytaj zapytanie
		{
			res::OcclusionQueries::Issue query_issue_obj(CustomQueries[ei]);
			icosphere->DrawGeometry();
		}
	}
	for (uint ei = 0; ei < DrawData.HeatEntities.size(); ei++)
	{
		// utwórz zapytanie
		HeatQueries[ei] = occlusion_queries->Create();
		// utwórz transformacjê dla sfery occlusion query
		Scaling(&ms, DrawData.HeatEntities[ei]->GetWorldRadius());
		Translation(&mt, DrawData.HeatEntities[ei]->GetWorldPos());
		// ustaw stany per-obiekt
		effect->Get()->SetMatrix(effect->GetParam(0), &math_cast<D3DXMATRIX>(ms*mt));
		effect->Get()->CommitChanges();
		// zapytaj zapytanie
		{
			res::OcclusionQueries::Issue query_issue_obj(HeatQueries[ei]);
			icosphere->DrawGeometry();
		}
	}

	effect->End();

	float CameraZNear = m_ActiveCamera->GetParams().GetZNear();

	// Odbierz odpowiedzi, wyzeruj wskaŸniki na encje których nie widaæ
	// Uwaga! Tu jest dodatek ¿e jeœli oko jest wewn¹trz encji, to ona powinna byæ widoczna nawet jeœli akurat
	// nie narysowa³o siê nic ze sfery do Occlusion Query - to rozwi¹zuje pewien wystêpuj¹cy czasem b³¹d.
	// Uwaga2! Tu by³ jeszcze taki b³¹d, ¿e encja znika³a kiedy kamera nie by³a jeszcze we wnêtrzu jej sfery
	// otaczaj¹cej, ale p³aszczyzna Z-Near ju¿ przycina³a przedni¹ œciankê jej sfery do Occlusion Query (a tylna
	// by³a zas³oniêta). Dlatego do promienia trzeba dodawaæ CameraZNear.
	for (uint ei = 0; ei < DrawData.MaterialEntities.size(); ei++)
	{
		if (MaterialQueries[ei]->GetResult() == 0)
			if (!PointInSphere(EyePos, DrawData.MaterialEntities[ei]->GetWorldPos(), DrawData.MaterialEntities[ei]->GetWorldRadius() + CameraZNear))
				DrawData.MaterialEntities[ei] = NULL;
	}
	for (uint ei = 0; ei < DrawData.CustomEntities.size(); ei++)
	{
		if (CustomQueries[ei]->GetResult() == 0)
			if (!PointInSphere(EyePos, DrawData.CustomEntities[ei]->GetWorldPos(), DrawData.CustomEntities[ei]->GetWorldRadius() + CameraZNear))
				DrawData.CustomEntities[ei] = NULL;
	}
	for (uint ei = 0; ei < DrawData.HeatEntities.size(); ei++)
	{
		if (HeatQueries[ei]->GetResult() == 0)
			if (!PointInSphere(EyePos, DrawData.HeatEntities[ei]->GetWorldPos(), DrawData.HeatEntities[ei]->GetWorldRadius() + CameraZNear))
				DrawData.HeatEntities[ei] = NULL;
	}

	// Usuñ queries
	for (uint ei = 0; ei < DrawData.MaterialEntities.size(); ei++)
		occlusion_queries->Destroy(MaterialQueries[ei]);
	for (uint ei = 0; ei < DrawData.CustomEntities.size(); ei++)
		occlusion_queries->Destroy(CustomQueries[ei]);
	for (uint ei = 0; ei < DrawData.HeatEntities.size(); ei++)
		occlusion_queries->Destroy(HeatQueries[ei]);

	// Skasuj wyzerowane wskaŸniki
	std::vector<MaterialEntity*>::iterator mit1 = std::remove(DrawData.MaterialEntities.begin(), DrawData.MaterialEntities.end(), (MaterialEntity*)NULL);
	std::vector<CustomEntity*>::iterator mit2 = std::remove(DrawData.CustomEntities.begin(), DrawData.CustomEntities.end(), (CustomEntity*)NULL);
	std::vector<HeatEntity*>::iterator mit3 = std::remove(DrawData.HeatEntities.begin(), DrawData.HeatEntities.end(), (HeatEntity*)NULL);
	DrawData.MaterialEntities.erase(mit1, DrawData.MaterialEntities.end());
	DrawData.CustomEntities.erase(mit2, DrawData.CustomEntities.end());
	DrawData.HeatEntities.erase(mit3, DrawData.HeatEntities.end());
}

void Scene::OcclusionQuery_Lights(DRAW_DATA &DrawData)
{
	// HACK!
	// Zabezpieczenie przeciwko zbyt du¿ej liczbie Queries, bo mi siê wtedy z niewiadomych przyczyn wysypuje
	// z b³êdem C++ Bad Alloc.
	if (DrawData.SpotLights.size() + DrawData.PointLights.size() > 50)
		return;

	VEC3 EyePos = m_ActiveCamera->GetParams().GetEyePos();

	// U³ó¿ zaptania occlusion query dla poszczególnych typów œwiate³
	std::vector<res::OcclusionQueries::Query*> SpotQueries;
	std::vector<res::OcclusionQueries::Query*> PointQueries;

	SpotQueries.resize(DrawData.SpotLights.size());
	PointQueries.resize(DrawData.PointLights.size());

	res::D3dEffect *effect = g_Engine->GetServices()->GetOcclusionQueryEffect();
	IcosphereRes *icosphere = g_Engine->GetServices()->GetIcosphere();
	res::OcclusionQueries *occlusion_queries = g_Engine->GetServices()->GetOcclusionQueries();

	frame::RestoreDefaultState(); // Od wszelkiego...
	effect->Get()->SetMatrix(effect->GetParam(1), &math_cast<D3DXMATRIX>(m_ActiveCamera->GetMatrices().GetView()));
	effect->Get()->SetMatrix(effect->GetParam(2), &math_cast<D3DXMATRIX>(m_ActiveCamera->GetMatrices().GetProj()));
	effect->Begin(true);
	icosphere->SetState();

	MATRIX ms, mt;
	for (uint li = 0; li < DrawData.SpotLights.size(); li++)
	{
		// utwórz zapytanie
		SpotQueries[li] = occlusion_queries->Create();
		// utwórz transformacjê dla sfery occlusion query
		Scaling(&ms, DrawData.SpotLights[li]->GetDist());
		Translation(&mt, DrawData.SpotLights[li]->GetPos());
		// ustaw stany per-obiekt
		effect->Get()->SetMatrix(effect->GetParam(0), &math_cast<D3DXMATRIX>(ms*mt));
		effect->Get()->CommitChanges();
		// zapytaj zapytanie
		{
			res::OcclusionQueries::Issue query_issue_obj(SpotQueries[li]);
			icosphere->DrawGeometry();
		}
	}
	for (uint li = 0; li < DrawData.PointLights.size(); li++)
	{
		// utwórz zapytanie
		PointQueries[li] = occlusion_queries->Create();
		// utwórz transformacjê dla sfery occlusion query
		Scaling(&ms, DrawData.PointLights[li]->GetDist());
		Translation(&mt, DrawData.PointLights[li]->GetPos());
		// ustaw stany per-obiekt
		effect->Get()->SetMatrix(effect->GetParam(0), &math_cast<D3DXMATRIX>(ms*mt));
		effect->Get()->CommitChanges();
		// zapytaj zapytanie
		{
			res::OcclusionQueries::Issue query_issue_obj(PointQueries[li]);
			icosphere->DrawGeometry();
		}
	}

	effect->End();

	float CameraZNear = m_ActiveCamera->GetParams().GetZNear();

	// Odbierz odpowiedzi, wyzeruj wskaŸniki na encje których nie widaæ
	// Uwaga! Tu jest dodatek ¿e jeœli oko jest wewn¹trz encji, to ona powinna byæ widoczna nawet jeœli akurat
	// nie narysowa³o siê nic ze sfery do Occlusion Query - to rozwi¹zuje pewien wystêpuj¹cy czasem b³¹d.
	// Uwaga2! Tu by³ jeszcze taki b³¹d, ¿e encja znika³a kiedy kamera nie by³a jeszcze we wnêtrzu jej sfery
	// otaczaj¹cej, ale p³aszczyzna Z-Near ju¿ przycina³a przedni¹ œciankê jej sfery do Occlusion Query (a tylna
	// by³a zas³oniêta). Dlatego do promienia trzeba dodawaæ CameraZNear.
	for (uint li = 0; li < DrawData.SpotLights.size(); li++)
	{
		if (SpotQueries[li]->GetResult() == 0)
			if (!PointInSphere(EyePos, DrawData.SpotLights[li]->GetPos(), DrawData.SpotLights[li]->GetDist() + CameraZNear))
				DrawData.SpotLights[li] = NULL;
	}
	for (uint li = 0; li < DrawData.PointLights.size(); li++)
	{
		if (PointQueries[li]->GetResult() == 0)
			if (!PointInSphere(EyePos, DrawData.PointLights[li]->GetPos(), DrawData.PointLights[li]->GetDist() + CameraZNear))
				DrawData.PointLights[li] = NULL;
	}

	// Usuñ queries
	for (uint ei = 0; ei < DrawData.SpotLights.size(); ei++)
		occlusion_queries->Destroy(SpotQueries[ei]);
	for (uint ei = 0; ei < DrawData.PointLights.size(); ei++)
		occlusion_queries->Destroy(PointQueries[ei]);

	// Skasuj wyzerowane wskaŸniki
	std::vector<SpotLight*>::iterator lit1 = std::remove(DrawData.SpotLights.begin(), DrawData.SpotLights.end(), (SpotLight*)NULL);
	std::vector<PointLight*>::iterator lit2 = std::remove(DrawData.PointLights.begin(), DrawData.PointLights.end(), (PointLight*)NULL);
	DrawData.SpotLights.erase(lit1, DrawData.SpotLights.end());
	DrawData.PointLights.erase(lit2, DrawData.PointLights.end());
}

void Scene::BuildEntityFragments(DRAW_DATA &DrawData)
{
	// Fragmenty encji MaterialEntity
	for (uint ei = 0; ei < DrawData.MaterialEntities.size(); ei++)
	{
		MaterialEntity & me = *DrawData.MaterialEntities[ei];
		uint FragmentCount = me.GetFragmentCount();
		for (uint fi = 0; fi < FragmentCount; fi++)
		{
			ENTITY_FRAGMENT ef = me.GetFragment(fi);
			assert(ef.E == &me);
			assert(ef.M != NULL);
			assert(ef.Pos == me.GetWorldPos());
			if (ef.M->GetType() == BaseMaterial::TYPE_OPAQUE)
				DrawData.OpaqueEntityFragments.push_back(ef);
			else
				DrawData.TranslucentEntityFragments.push_back(ef);
		}
	}
	// Encje CustomEntity w ca³oœci
	for (uint ei = 0; ei < DrawData.CustomEntities.size(); ei++)
		DrawData.TranslucentEntityFragments.push_back(ENTITY_FRAGMENT(DrawData.CustomEntities[ei], 0, NULL, DrawData.CustomEntities[ei]->GetWorldPos()));
}

void Scene::DrawAll(DRAW_DATA &DrawData, STATS *OutStats, const RunningOptimizer::SETTINGS &OptimizerSettings)
{
	bool Engine_Lighting = g_Engine->GetConfigBool(Engine::O_LIGHTING);
	bool Engine_ShadowMapping = g_Engine->GetConfigBool(Engine::O_SM_ENABLED);

	// Narysuj niebo
	if (m_Sky != NULL)
		m_Sky->Draw(m_ActiveCamera->GetParams());

	// Wyczyœæ Z-Bufor
	frame::Dev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.f, 0);

	// Narysuj fragmenty mapy - przebieg Base
	if (m_QMapRenderer != NULL)
	{
		for (uint i = 0; i < DrawData.MapFragments.size(); i++)
			m_QMapRenderer->DrawFragment(PASS_BASE, DrawData.MapFragments[i], SCENE_DRAW_PARAMS(this), &m_ActiveCamera->GetParams(), NULL);
	}

	// Narysuj fragmenty terenu - przebieg Base
	if (m_TerrainRenderer != NULL)
	{
		for (uint pi = 0; pi < DrawData.TerrainPatches.size(); pi++)
			m_TerrainRenderer->DrawPatch(PASS_BASE, DrawData.TerrainPatches[pi], SCENE_DRAW_PARAMS(this), &m_ActiveCamera->GetParams(), NULL);
	}

	OutStats->TerrainPatches = DrawData.TerrainPatches.size();
	OutStats->Trees = DrawData.Trees.size();
	OutStats->MapFragments = DrawData.MapFragments.size();
	OutStats->Entities[0] = DrawData.MaterialEntities.size() + DrawData.CustomEntities.size() + DrawData.HeatEntities.size();
	OutStats->SpotLights[0] = DrawData.SpotLights.size();
	OutStats->PointLights[0] = DrawData.PointLights.size();

	// Occlusion Query
	if (OptimizerSettings[RunningOptimizer::SETTING_ENTITY_OCCLUSION_QUERY])
	{
		OcclusionQuery_Entities(DrawData);
		OutStats->Entities[1] = DrawData.MaterialEntities.size() + DrawData.CustomEntities.size() + DrawData.HeatEntities.size();
	}
	else
		OutStats->Entities[1] = OutStats->Entities[0];

	if (OptimizerSettings[RunningOptimizer::SETTING_LIGHT_OCCLUSION_QUERY])
	{
		OcclusionQuery_Lights(DrawData);
		OutStats->SpotLights[1] = DrawData.SpotLights.size();
		OutStats->PointLights[1] = DrawData.PointLights.size();
	}
	else
	{
		OutStats->SpotLights[1] = OutStats->SpotLights[0];
		OutStats->PointLights[1] = OutStats->PointLights[0];
	}

	// Spisz fragmenty encji materia³owych
	BuildEntityFragments(DrawData);

	// Posortuj nieprzezroczyste fragmenty encji materia³owych
	if (OptimizerSettings[RunningOptimizer::SETTING_MATERIAL_SORT])
		DrawData.SortOpaqueEntityFragmentsByMaterial();

	// Narysuj nieprzezroczyste fragmenty encji - przebieg Base
	for (uint fi = 0; fi < DrawData.OpaqueEntityFragments.size(); fi++)
	{
		const ENTITY_FRAGMENT & ef = DrawData.OpaqueEntityFragments[fi];
		assert(dynamic_cast<MaterialEntity*>(ef.E) != NULL);
		static_cast<MaterialEntity*>(ef.E)->DrawFragment(PASS_BASE, ef, SCENE_DRAW_PARAMS(this), &m_ActiveCamera->GetParams(), NULL);
	}

	// Posortuj drzewa do narysowania wg klasy Tree
	DrawData.SortTrees();

	// Narysuj drzewa - przebieg g³ówny
	if (!DrawData.Trees.empty())
	{
		float FogStart = GetFogStart();
		const float *FogStartPtr = (GetFogEnabled() ? &FogStart : NULL);
		const COLORF *FogColorPtr = (GetFogEnabled() ? &GetFogColor() : NULL);
		VEC3 DirToLight;
		if (m_DirectionalLight != NULL && Engine_Lighting)
			DirToLight = - m_DirectionalLight->GetDir();
		const VEC3 *DirToLightPtr = (Engine_Lighting && m_DirectionalLight != NULL ? &DirToLight : NULL);
		const COLORF *LightColorPtr = (Engine_Lighting && m_DirectionalLight != NULL ? &m_DirectionalLight->GetColor() : NULL);

		Tree *LastTree = NULL;
		for (uint ti = 0; ti < DrawData.Trees.size(); ti++)
		{
			const TREE_DRAW_DESC &TreeDrawDesc = DrawData.Trees[ti];
			if (TreeDrawDesc.tree != LastTree)
			{
				if (LastTree != NULL)
					LastTree->DrawEnd();
				LastTree = TreeDrawDesc.tree;
				LastTree->DrawBegin(m_ActiveCamera->GetParams(), DirToLightPtr, LightColorPtr, FogStartPtr, FogColorPtr);
			}
			LastTree->DrawTree(TreeDrawDesc.kind, TreeDrawDesc.world, &TreeDrawDesc.world_inv, TreeDrawDesc.color);
		}
		if (LastTree != NULL)
			LastTree->DrawEnd();
	}

	// Narysuj trawê
	if (m_TerrainRenderer != NULL)
	{
		COLORF Color = COLORF::WHITE;
		if (Engine_Lighting && m_DirectionalLight != NULL)
		{
			Color = m_DirectionalLight->GetColor() * minmax(0.0f, Dot((-m_DirectionalLight->GetDir()+VEC3::POSITIVE_Y)*0.5f, VEC3::POSITIVE_Y), 1.0f);
		}
		m_TerrainRenderer->DrawGrass(m_ActiveCamera->GetParams(), Color);
	}

	OutStats->Passes = 1;

	// Œwiat³a
	if (Engine_Lighting)
	{
		// Œwiat³o kierunkowe
		if (m_DirectionalLight != NULL && m_DirectionalLight->GetEnabled() && m_DirectionalLight->GetColor() != COLORF::BLACK)
		{
			// Shadow Mapping - jeœli jest
			res::D3dTextureSurface *ShadowMap = NULL;
			MATRIX ShadowMapMatrix;
			if (Engine_ShadowMapping && m_DirectionalLight->GetCastShadow())
			{
				DoShadowMapping_Directional(*m_DirectionalLight, &ShadowMap, &ShadowMapMatrix, OptimizerSettings[RunningOptimizer::SETTING_MATERIAL_SORT]);
				OutStats->Passes++;
			}

			SCENE_DRAW_PARAMS SceneDrawParams = SCENE_DRAW_PARAMS(this, ShadowMap, &ShadowMapMatrix);
			if (m_DirectionalLight->GetZFar() < m_ActiveCamera->GetParams().GetZFar())
			{
				SceneDrawParams.UseVariableShadowFactor = true;
				SceneDrawParams.ShadowFactorAttnEnd = m_DirectionalLight->GetZFar();
				SceneDrawParams.ShadowFactorAttnStart = m_DirectionalLight->GetZFar() * DIRECTIONAL_LIGHT_SHADOW_ATTEN_START_PERCENT;
			}

			// Wybierz fragmenty mapy do narysowania oœwietlone œwiat³em kierunkowym
			// Jeœli tylko mapa podlega oœwietleniu, bêd¹ to wszystkie widoczne fragmenty,
			// bo wszystkie s¹ rysowane materia³em OpaqueMaterial
			if (m_QMapRenderer != NULL && GetMapUseLighting())
			{
				for (uint fi = 0; fi < DrawData.MapFragments.size(); fi++)
				{
					// Od razu narysuj
					SceneDrawParams.ShadowMapTexture = (m_MapReceiveShadow ? ShadowMap : NULL);
					m_QMapRenderer->DrawFragment(PASS_DIRECTIONAL, DrawData.MapFragments[fi], SceneDrawParams, &m_ActiveCamera->GetParams(), m_DirectionalLight);
				}
			}

			// Narysuj fragmenty terenu
			if (m_TerrainRenderer != NULL)
			{
				for (uint pi = 0; pi < DrawData.TerrainPatches.size(); pi++)
					m_TerrainRenderer->DrawPatch(PASS_DIRECTIONAL, DrawData.TerrainPatches[pi], SceneDrawParams, &m_ActiveCamera->GetParams(), m_DirectionalLight);
			}

			// Wybierz fragmenty encji do narysowania oœwietlone œwiat³em kierunkowym
			for (uint fi = 0; fi < DrawData.OpaqueEntityFragments.size(); fi++)
			{
				const ENTITY_FRAGMENT & ef = DrawData.OpaqueEntityFragments[fi];
				if (static_cast<MaterialEntity*>(ef.E)->GetUseLighting())
				{
					// Od razu narysuj
					SceneDrawParams.ShadowMapTexture = (static_cast<MaterialEntity*>(ef.E)->GetReceiveShadow() ? ShadowMap : NULL);
					static_cast<MaterialEntity*>(ef.E)->DrawFragment(PASS_DIRECTIONAL, ef, SceneDrawParams, &m_ActiveCamera->GetParams(), m_DirectionalLight);
				}
			}

			OutStats->Passes++;
		}

		// Dla ka¿dego œwiat³a Point
		for (uint li = 0; li < DrawData.PointLights.size(); li++)
		{
			PointLight & point_light = *DrawData.PointLights[li];
			// Jeœli jest aktywne i nieczarne
			if (point_light.GetEnabled() && point_light.GetColor() != COLORF::BLACK)
			{
				// Jeœli jego zasiêg jest widoczny we frustumie widzenia
				if (SphereToFrustum_Fast(point_light.GetPos(), point_light.GetDist(), m_ActiveCamera->GetMatrices().GetFrustumPlanes()))
				{
					// Wyznacz prostok¹t Scissor Test
					RECT ScissorRect;
					float ScissorRectPercent = point_light.GetScissorRect(&ScissorRect, m_ActiveCamera->GetMatrices().GetView(), m_ActiveCamera->GetMatrices().GetProj(), frame::Settings.BackBufferWidth, frame::Settings.BackBufferHeight);

					// Shadow Mapping - jeœli jest
					res::D3dCubeTextureSurface *ShadowMap = NULL;
					MATRIX ShadowMapMatrix;
					if (Engine_ShadowMapping && point_light.GetCastShadow())
					{
						DoShadowMapping_Point(point_light, ScissorRectPercent, &ShadowMap, &ShadowMapMatrix, OptimizerSettings[RunningOptimizer::SETTING_MATERIAL_SORT]);
						OutStats->Passes += 6;
					}

					// W³¹cz Scissor Test
					ERR_GUARD_DIRECTX( frame::Dev->SetScissorRect(&ScissorRect) );
					ERR_GUARD_DIRECTX( frame::Dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE) );

					// Fragmenty mapy do narysowania oœwietlone tym œwiat³em
					// Wszystkie s¹ rysowane materia³em OpaqueMaterial.
					if (m_QMapRenderer != NULL && GetMapUseLighting())
					{
						for (uint fi = 0; fi < DrawData.MapFragments.size(); fi++)
						{
							// Jeœli AABB tego fragmentu mapy (wêz³a Octree do którego nale¿y) koliduje z zasiêgiem œwiat³a
							if (point_light.BoxInRange(*DrawData.MapFragments[fi].Bounds))
							{
								// Od razu narysuj
								m_QMapRenderer->DrawFragment(PASS_POINT, DrawData.MapFragments[fi], SCENE_DRAW_PARAMS(this, m_MapReceiveShadow ? ShadowMap : NULL, &ShadowMapMatrix), &m_ActiveCamera->GetParams(), &point_light);
							}
						}
					}

					// Narysuj fragmenty terenu oœwietlone tym œwiat³em
					if (m_TerrainRenderer != NULL)
					{
						for (uint pi = 0; pi < DrawData.TerrainPatches.size(); pi++)
						{
							// Jeœli AABB tego patcha terenu koliduje z zasiêgiem œwiat³a
							BOX PatchBox; m_TerrainRenderer->GetTerrain()->CalcPatchBoundingBox(&PatchBox, DrawData.TerrainPatches[pi]);
							if (point_light.BoxInRange(PatchBox))
								m_TerrainRenderer->DrawPatch(PASS_POINT, DrawData.TerrainPatches[pi], SCENE_DRAW_PARAMS(this, m_MapReceiveShadow ? ShadowMap : NULL, &ShadowMapMatrix), &m_ActiveCamera->GetParams(), &point_light);
						}
					}

					// Wybierz fragmenty encji do narysowania oœwietlone tym œwiat³em
					for (uint fi = 0; fi < DrawData.OpaqueEntityFragments.size(); fi++)
					{
						const ENTITY_FRAGMENT & ef = DrawData.OpaqueEntityFragments[fi];
						if (static_cast<MaterialEntity*>(ef.E)->GetUseLighting())
						{
							// Jeœli encja tego fragmentu jest w zasiêgu tego œwiat³a
							if (point_light.SphereInRange(ef.E->GetWorldPos(), ef.E->GetWorldRadius()))
							{
								// Od razu narysuj
								static_cast<MaterialEntity*>(ef.E)->DrawFragment(PASS_POINT, ef, SCENE_DRAW_PARAMS(this, static_cast<MaterialEntity*>(ef.E)->GetReceiveShadow() ? ShadowMap : NULL, &ShadowMapMatrix), &m_ActiveCamera->GetParams(), &point_light);
							}
						}
					}

					// Wy³¹cz Scissor Test
					frame::Dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

					OutStats->Passes++;
				}
			}
		}

		// Dla ka¿dego œwiat³a Spot
		for (uint li = 0; li < DrawData.SpotLights.size(); li++)
		{
			SpotLight & spot_light = *DrawData.SpotLights[li];
			// Jeœli jest aktywne i nieczarne
			if (spot_light.GetEnabled() && spot_light.GetColor() != COLORF::BLACK)
			{
				// Jeœli jego zasiêg jest widoczny we frustumie widzenia
				if (SphereToFrustum_Fast(spot_light.GetPos(), spot_light.GetDist(), m_ActiveCamera->GetMatrices().GetFrustumPlanes()) &&
					FrustumToFrustum(spot_light.GetFrustumPlanes(), spot_light.GetFrustumPoints(), m_ActiveCamera->GetMatrices().GetFrustumPlanes(), m_ActiveCamera->GetMatrices().GetFrustumPoints()))
				{
					// Wyznacz prostok¹t dla Scissor Test
					RECT ScissorRect;
					float ScissorRectPercent = spot_light.GetScissorRect(&ScissorRect, m_ActiveCamera->GetMatrices().GetView(), m_ActiveCamera->GetMatrices().GetProj(), frame::Settings.BackBufferWidth, frame::Settings.BackBufferHeight);

					// Shadow Mapping - jeœli jest
					res::D3dTextureSurface *ShadowMap = NULL;
					MATRIX ShadowMapMatrix;
					if (Engine_ShadowMapping && spot_light.GetCastShadow())
					{
						DoShadowMapping_Spot(spot_light, ScissorRectPercent, &ShadowMap, &ShadowMapMatrix, OptimizerSettings[RunningOptimizer::SETTING_MATERIAL_SORT]);
						OutStats->Passes++;
					}

					// Odpal Scissor Test
					ERR_GUARD_DIRECTX( frame::Dev->SetScissorRect(&ScissorRect) );
					ERR_GUARD_DIRECTX( frame::Dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE) );

					// Fragmenty mapy do narysowania oœwietlone tym œwiat³em
					// Wszystkie s¹ rysowane materia³em OpaqueMaterial.
					if (m_QMapRenderer != NULL && GetMapUseLighting())
					{
						for (uint fi = 0; fi < DrawData.MapFragments.size(); fi++)
						{
							// Jeœli AABB tego fragmentu mapy (wêz³a Octree do którego nale¿y) koliduje z zasiêgiem œwiat³a
							if (spot_light.BoxInRange(*DrawData.MapFragments[fi].Bounds))
							{
								// Od razu narysuj
								m_QMapRenderer->DrawFragment(PASS_SPOT, DrawData.MapFragments[fi], SCENE_DRAW_PARAMS(this, m_MapReceiveShadow ? ShadowMap : NULL, &ShadowMapMatrix), &m_ActiveCamera->GetParams(), &spot_light);
							}
						}
					}

					// Narysuj fragmenty terenu oœwietlone tym œwiat³em
					if (m_TerrainRenderer != NULL)
					{
						for (uint pi = 0; pi < DrawData.TerrainPatches.size(); pi++)
						{
							// Jeœli AABB tego patcha terenu koliduje z zasiêgiem œwiat³a
							BOX PatchBox; m_TerrainRenderer->GetTerrain()->CalcPatchBoundingBox(&PatchBox, DrawData.TerrainPatches[pi]);
							if (spot_light.BoxInRange(PatchBox))
								m_TerrainRenderer->DrawPatch(PASS_SPOT, DrawData.TerrainPatches[pi], SCENE_DRAW_PARAMS(this, m_MapReceiveShadow ? ShadowMap : NULL, &ShadowMapMatrix), &m_ActiveCamera->GetParams(), &spot_light);
						}
					}

					// Wybierz fragmenty encji do narysowania oœwietlone tym kierunkowym
					for (uint fi = 0; fi < DrawData.OpaqueEntityFragments.size(); fi++)
					{
						const ENTITY_FRAGMENT & ef = DrawData.OpaqueEntityFragments[fi];
						if (static_cast<MaterialEntity*>(ef.E)->GetUseLighting())
						{
							// Jeœli encja tego fragmentu jest w zasiêgu tego œwiat³a
							if (spot_light.SphereInRange(ef.E->GetWorldPos(), ef.E->GetWorldRadius()))
							{
								// Od razu narysuj
								static_cast<MaterialEntity*>(ef.E)->DrawFragment(PASS_SPOT, ef, SCENE_DRAW_PARAMS(this, static_cast<MaterialEntity*>(ef.E)->GetReceiveShadow() ? ShadowMap : NULL, &ShadowMapMatrix), &m_ActiveCamera->GetParams(), &spot_light);
							}
						}
					}

					// Wy³¹cz Scissor Test
					frame::Dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

					OutStats->Passes++;
				}
			}
		}
	}

	// Mg³a
	if (GetFogEnabled())
	{
		// Narysuj fragmenty mapy - przebieg 1 (Fog)
		if (m_QMapRenderer != NULL)
		{
			for (uint i = 0; i < DrawData.MapFragments.size(); i++)
				m_QMapRenderer->DrawFragment(PASS_FOG, DrawData.MapFragments[i], SCENE_DRAW_PARAMS(this), &m_ActiveCamera->GetParams(), NULL);
		}

		// Narysuj fragmenty terenu - przebieg Fog
		if (m_TerrainRenderer != NULL)
		{
			for (uint pi = 0; pi < DrawData.TerrainPatches.size(); pi++)
			{
				m_TerrainRenderer->DrawPatch(PASS_FOG, DrawData.TerrainPatches[pi], SCENE_DRAW_PARAMS(this), &m_ActiveCamera->GetParams(), NULL);
			}
		}

		// Narysuj nieprzezroczyste fragmenty encji - przebieg Fog
		for (uint fi = 0; fi < DrawData.OpaqueEntityFragments.size(); fi++)
		{
			const ENTITY_FRAGMENT & ef = DrawData.OpaqueEntityFragments[fi];
			assert(dynamic_cast<MaterialEntity*>(ef.E) != NULL);
			static_cast<MaterialEntity*>(ef.E)->DrawFragment(PASS_FOG, ef, SCENE_DRAW_PARAMS(this), &m_ActiveCamera->GetParams(), NULL);
		}

		OutStats->Passes++;
	}

	// Posortuj fragmenty pó³przezroczyste wg odleg³oœci
	DrawData.SortTranslucentEntityFragmentssByDistance(m_ActiveCamera->GetParams().GetEyePos());

	// Narysuj wszystkie fragmenty pó³przezroczyste od najdalszych do najbli¿szych
	for (uint fi = 0; fi < DrawData.TranslucentEntityFragments.size(); fi++)
	{
		const ENTITY_FRAGMENT & ef = DrawData.TranslucentEntityFragments[fi];
		if (ef.M == NULL)
		{
			// Materia³ NULL - to jest CustomEntity
			assert(dynamic_cast<CustomEntity*>(ef.E) != NULL);
			static_cast<CustomEntity*>(ef.E)->Draw(m_ActiveCamera->GetParams());
		}
		else
		{
			// Jest materia³ - to jest MaterialEntity, materia³ Translucent lub WireFrame
			assert(dynamic_cast<MaterialEntity*>(ef.E) != NULL);
			if (ef.M->GetType() == BaseMaterial::TYPE_WIREFRAME)
				static_cast<MaterialEntity*>(ef.E)->DrawFragment(PASS_WIREFRAME, ef, SCENE_DRAW_PARAMS(this), &m_ActiveCamera->GetParams(), NULL);
			else if (ef.M->GetType() == BaseMaterial::TYPE_TRANSLUCENT)
				static_cast<MaterialEntity*>(ef.E)->DrawFragment(PASS_TRANSLUCENT, ef, SCENE_DRAW_PARAMS(this), &m_ActiveCamera->GetParams(), NULL);
			else
				assert(0);
		}
	}

	// Narysuj wodê terenu
	if (m_TerrainRenderer != NULL)
		m_TerrainRenderer->DrawWater(m_ActiveCamera->GetParams());

	// Narysuj opady
	if (m_Fall != NULL)
		m_Fall->Draw(m_ActiveCamera->GetParams());
}

float Scene::QuerySunVisibleFactor(const ParamsCamera &Cam, const VEC3 &SunDir)
{
	MATRIX WorldMat;
	Translation(&WorldMat, Cam.GetEyePos());

	frame::RestoreDefaultState();

	MATRIX RotMat; LookAtLH(&RotMat, VEC3::ZERO, SunDir, VEC3::POSITIVE_Y);
	Transpose(&RotMat);

	const float Angle = 0.05f;
	const float Radius = Cam.GetZFar() * 0.99f;
	float Delta = Radius * tanf(Angle * 0.5f);
	VERTEX_X Vertices[4];
	VEC3 v;
	v = VEC3(-Delta, -Delta, Radius);
	Transform(&Vertices[0].Pos, v, RotMat);
	v = VEC3(-Delta,  Delta, Radius);
	Transform(&Vertices[1].Pos, v, RotMat);
	v = VEC3( Delta,  Delta, Radius);
	Transform(&Vertices[2].Pos, v, RotMat);
	v = VEC3( Delta, -Delta, Radius);
	Transform(&Vertices[3].Pos, v, RotMat);

	res::D3dEffect *Effect = g_Engine->GetServices()->GetOcclusionQueryEffect();
	res::OcclusionQueries::Query *Query = g_Engine->GetServices()->GetOcclusionQueries()->Create();

	Effect->Get()->SetMatrix(Effect->GetParam(0), &math_cast<D3DXMATRIX>(WorldMat));
	Effect->Get()->SetMatrix(Effect->GetParam(1), &math_cast<D3DXMATRIX>(Cam.GetMatrices().GetView()));
	Effect->Get()->SetMatrix(Effect->GetParam(2), &math_cast<D3DXMATRIX>(Cam.GetMatrices().GetProj()));
	Effect->Begin(true);

	{
		res::OcclusionQueries::Issue query_issue_obj(Query);

		frame::Dev->SetFVF(VERTEX_X::FVF);
		frame::Dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, Vertices, sizeof(VERTEX_X));
		frame::RegisterDrawCall(2);
	}

	Effect->End();

	float R = (Query->GetResult() > 16 ? 1.0f : 0.0f);

	g_Engine->GetServices()->GetOcclusionQueries()->Destroy(Query);

	return R;
}

void Scene::SortEntityVector_FarToNear(ENTITY_VECTOR *Vec, const VEC3 &EyePos)
{
	std::sort(Vec->begin(), Vec->end(), EntityDistanceCompare_Greater(EyePos));
}

void Scene::SortEntityVector_NearToFar(ENTITY_VECTOR *Vec, const VEC3 &EyePos)
{
	std::sort(Vec->begin(), Vec->end(), EntityDistanceCompare_Less(EyePos));
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Engine

Engine::Engine() :
	m_Destroying(false),
	m_ActiveScene(NULL)
{
	ERR_TRY;

	LoadConfig();

	ValidateDeviceCaps();

	// Koniecznie ju¿ po wczytaniu konfiguracji, bo Services mog¹ z niej korzystaæ!
	m_Services.reset(new EngineServices(this));

	ERR_CATCH_FUNC;
}

Engine::~Engine()
{
	m_Destroying = true;

	// Zniszcz pozosta³e sceny
	for (SCENE_SET::iterator it = m_Scenes.begin(); it != m_Scenes.end(); ++it)
		delete *it;
	m_Scenes.clear();
}

bool Engine::GetConfigBool(OPTION Option)
{
	switch (Option)
	{
	case O_LIGHTING:
		return m_Lighting;
	case O_SM_ENABLED:
		return m_SM_Enabled;
	case O_PP_ENABLED:
		return m_PP_Enabled;
	default:
		throw Error("Engine::GetConfigBool: Nieznana opcja", __FILE__, __LINE__);
	}
}

float Engine::GetConfigFloat(OPTION Option)
{
	switch (Option)
	{
	case O_SM_EPSILON:
		return m_SM_Epsilon;
	case O_GRASS_ZFAR:
		return m_Grass_ZFar;
	default:
		throw Error("Engine::GetConfigFloat: Nieznana opcja", __FILE__, __LINE__);
	}
}

uint Engine::GetConfigUint(OPTION Option)
{
	switch (Option)
	{
	case O_SM_MAX_SIZE:
		return m_SM_MaxSize;
	case O_SM_MAX_CUBE_SIZE:
		return m_SM_MaxCubeSize;
	default:
		throw Error("Engine::GetConfigUint: Nieznana opcja", __FILE__, __LINE__);
	}
}

void Engine::SetConfigBool(OPTION Option, bool v)
{
	switch (Option)
	{
	case O_LIGHTING:
		m_Lighting = v;
		break;
	case O_SM_ENABLED:
		m_SM_Enabled = v;
		break;
	case O_PP_ENABLED:
		{
			bool old = m_PP_Enabled;
			m_PP_Enabled = v;
			if (old && !v)
				GetServices()->DestroySpecialTextures();
			else if (!old && v)
			{
				GetServices()->CreateSpecialTextures();
				if (!frame::GetDeviceLost())
					GetServices()->SetSpecialTexturesSize();
			}
		}
		break;
	default:
		throw Error("Engine::SetConfigBool: Nieznana opcja", __FILE__, __LINE__);
	}
}

void Engine::SetConfigFloat(OPTION Option, float v)
{
	switch (Option)
	{
	case O_SM_EPSILON:
		m_SM_Epsilon = v;
		break;
	case O_GRASS_ZFAR:
		m_Grass_ZFar = v;
		break;
	default:
		throw Error("Engine::SetConfigFloat: Nieznana opcja", __FILE__, __LINE__);
	}
}

void Engine::OnLostDevice()
{
	ERR_TRY;

	ERR_CATCH_FUNC;
}

void Engine::OnResetDevice()
{
	ERR_TRY;

	// Tekstury specjalne
	GetServices()->SetSpecialTexturesSize();

	ERR_CATCH_FUNC;
}

void Engine::LoadConfig()
{
	ERR_TRY;

	string s;
	g_Config->MustGetDataEx("Engine/Lighting", &m_Lighting);
	g_Config->MustGetDataEx("Engine/ShadowMapping/Enabled", &m_SM_Enabled);
	g_Config->MustGetDataEx("Engine/ShadowMapping/Epsilon", &m_SM_Epsilon);
	g_Config->MustGetDataEx("Engine/Postprocessing/Enabled", &m_PP_Enabled);
	g_Config->MustGetDataEx("Engine/ShadowMapping/MaxSize", &m_SM_MaxSize);
	g_Config->MustGetDataEx("Engine/ShadowMapping/MaxCubeSize", &m_SM_MaxCubeSize);
	g_Config->MustGetData("Engine/ShadowMapping/Format", &s);
	ERR_GUARD_BOOL(StrToD3dfmt(&m_SM_Format, s) );
	g_Config->MustGetDataEx("Engine/Grass/ZFar", &m_Grass_ZFar);

	ERR_CATCH("Nie mo¿na wczytaæ konfiguracji silnika.");
}

void Engine::ValidateDeviceCaps()
{
	LOG(LOG_ENGINE, "Checking device capabilities...");

	frame::D3D->GetDeviceCaps(frame::GetAdapter(), frame::GetDeviceType(), &m_DeviceCaps);

	// Wymagania minimalne
	if (m_DeviceCaps.VertexShaderVersion < D3DVS_VERSION(2, 0) ||
		m_DeviceCaps.PixelShaderVersion < D3DPS_VERSION(2, 0))
	{
		throw Error("Karta graficzna nie spe³nia wymagañ. Minimalne wymagania: Vertex Shader 2.0, Pixel Shader 2.0.");
	}

	LOG(LOG_ENGINE, "Device capabilities OK.");
}

void Engine::GetInfo(string *Out)
{
	*Out = Format("Passes=#, SpotLights=#:#, PointLights=#:# Entities=#:#, MapFragments=#, TerrainPatches=#, Trees=#, MainShaders=#, PpShaders=#, Optimizer=#") %
		m_Stats.Passes %
		m_Stats.SpotLights[0] % m_Stats.SpotLights[1] %
		m_Stats.PointLights[0] % m_Stats.PointLights[1] %
		m_Stats.Entities[0] % m_Stats.Entities[1] %
		m_Stats.MapFragments %
		m_Stats.TerrainPatches % m_Stats.Trees %
		m_Stats.MainShaders % m_Stats.PpShaders %
		m_Stats.RunningOptimizerOptions;
}

void Engine::Draw()
{
	ERR_TRY

	if (m_ActiveScene != NULL)
		m_ActiveScene->Draw(&m_Stats);

	ERR_CATCH_FUNC
}

void Engine::DrawSpecialTextures()
{
	ERR_TRY

	const float MARGIN = 4.f;
	const float TEXTURE_CX = 160.f;

	std::vector<res::D3dTextureSurface*> Textures;
	std::vector<res::D3dCubeTextureSurface*> CubeTextures;
	Textures.push_back(GetServices()->GetLastUsedShadowMap());
	Textures.push_back(GetServices()->TryGetSpecialTexture(EngineServices::ST_BLUR_1));
	Textures.push_back(GetServices()->TryGetSpecialTexture(EngineServices::ST_FEEDBACK));
	Textures.push_back(GetServices()->TryGetSpecialTexture(EngineServices::ST_SCREEN));
	CubeTextures.push_back(GetServices()->GetLastUsedCubeShadowMap());

	float y = MARGIN;
	float TextureCY;

	for (size_t i = 0; i < Textures.size(); i++)
	{
		if (Textures[i] != NULL && Textures[i]->IsTexture())
		{
			TextureCY = Textures[i]->GetHeight() * TEXTURE_CX / Textures[i]->GetWidth();
			PpEffect::Redraw(NULL, Textures[i]->GetTexture(), RECTF(MARGIN, y, MARGIN+TEXTURE_CX, y+TextureCY));
			y += TextureCY + MARGIN;
		}
	}

	for (size_t i = 0; i < CubeTextures.size(); i++)
	{
		if (CubeTextures[i] != NULL)
		{
			TextureCY = TEXTURE_CX * 3.f / 4.f;
			PpEffect::RedrawCube(CubeTextures[i]->GetTexture(), RECTF(MARGIN, y, MARGIN+TEXTURE_CX, y+TextureCY));
			y += TextureCY + MARGIN;
		}
	}

	ERR_CATCH_FUNC
}

void Engine::Update()
{
	if (m_ActiveScene != NULL)
		m_ActiveScene->Update();

	GetServices()->Update();
}

void Engine::RegisterScene(Scene *s)
{
	m_Scenes.insert(s);
}

void Engine::UnregisterScene(Scene *s)
{
	if (!m_Destroying)
	{
		if (m_ActiveScene == s)
			m_ActiveScene = NULL;

		SCENE_SET::iterator it = m_Scenes.find(s);
		assert(it != m_Scenes.end());
		m_Scenes.erase(it);
	}
}

Engine *g_Engine = NULL;


} // namespace engine
