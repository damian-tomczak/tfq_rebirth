/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef ENGINE_H_
#define ENGINE_H_

#include <bitset> // Dla RunningOptimizer
#include "..\Framework\Res_d3d.hpp"
#include "..\Framework\D3dUtils.hpp"
#include "Camera.hpp"

class ParamsCamera;
class Camera;

extern const uint LOG_ENGINE;

namespace res
{
	class Terrain;
};

namespace engine
{

class QMap;
class Terrain;
class PpEffect;
class PpColor;
class PpTexture;
class PpFunction;
class PpToneMapping;
class PpBloom;
class PpFeedback;
class PpLensFlare;
class BaseSky;
class Tree;
class Grass;
class TerrainWater;
class Fall;

class QMapRenderer;
class TerrainRenderer;
class EntityOctree;
struct ENTITY_OCTREE_NODE;
class Scene;
class Entity;
class RunningOptimizer;
typedef std::set<Scene*> SCENE_SET;
typedef std::set<Camera*> CAMERA_SET;
typedef std::set<Entity*> ENTITY_SET;
typedef std::vector<Entity*> ENTITY_VECTOR;
struct DRAW_DATA;
struct ENTITY_DRAW_PARAMS;
struct SCENE_DRAW_PARAMS;
struct TerrainMaterial;
class Engine;


// Zgodne z makrem PASS w dokumentacji - MainShader.txt.
// Fucking C++!!! - muszê to tutaj wywaliæ do nag³ówka.
enum PASS
{
	PASS_BASE        = 0,
	PASS_FOG         = 1,
	PASS_WIREFRAME   = 2,
	PASS_TRANSLUCENT = 3,
	PASS_DIRECTIONAL = 4,
	PASS_POINT       = 5,
	PASS_SPOT        = 6,
	PASS_SHADOW_MAP  = 7,
	PASS_TERRAIN     = 8,
};

enum COLLISION_TYPE
{
	// Test fizyczny (np. strza³)
	COLLISION_PHYSICAL = 0x01,
	// Test optyczny (promieñ œwiat³a)
	// Obiekty które maj¹ Visible==false nie robi¹ nigdy kolizji w tym trybie.
	COLLISION_OPTICAL  = 0x02,
	// Obydwa na raz
	COLLISION_BOTH     = 0x03,
};

class BaseMaterial
{
public:
	enum TYPE
	{
		TYPE_WIREFRAME,
		TYPE_TRANSLUCENT,
		TYPE_OPAQUE,
		TYPE_TERRAIN,
	};

	enum BLEND_MODE
	{
		BLEND_LERP, // Normalna interpolacja
		BLEND_ADD,  // Mieszanie dodaj¹ce
		BLEND_SUB,  // Mieszanie odejmuj¹ce
	};

	BaseMaterial(Scene *OwnerScene, const string &Name);
	virtual ~BaseMaterial();

	Scene * GetOwnerScene() { return m_OwnerScene; }
	virtual TYPE GetType() = 0;
	// Zwraca true, jeœli ten materia³ koliduje z podanym typem kolizji
	bool CheckCollisionType(uint CollisionType) { return (m_CollisionType & CollisionType) != 0; }

	const string & GetName() { return m_Name; }
	bool GetTwoSided() { return m_TwoSided; }
	uint GetCollisionType() { return m_CollisionType; }

	void SetTwoSided(bool TwoSided) { m_TwoSided = TwoSided; }
	void SetCollisionType(uint CollisionType) { m_CollisionType = CollisionType; }

private:
	Scene *m_OwnerScene;
	string m_Name;
	// Jeœli true, wy³¹cza Backface Culling
	bool m_TwoSided;
	// Typ kolizji - flaga bitowa dla sta³ych COLLISION_TYPE
	uint m_CollisionType;
};

// Materia³ w trybie szkieletowym
class WireFrameMaterial : public BaseMaterial
{
public:
	enum COLOR_MODE
	{
		COLOR_MATERIAL, // Kolor i kana³ Alfa brany tylko z materia³u
		COLOR_ENTITY,   // Kolor i kana³ Alfa brany tylko z Team Color encji
		COLOR_MODULATE, // Kolor i kana³ Alfa brany z materia³u RAZY z Team Color encji
	};

	// Tworzy zainicjalizowany wartoœciami domyœlnymi
	WireFrameMaterial(Scene *OwnerScene, const string &Name);
	// Tworzy zainicjalizowany wartoœciami domyœlnymi i podanymi
	WireFrameMaterial(Scene *OwnerScene, const string &Name, const COLORF &Color, BLEND_MODE BlendMode = BLEND_LERP, COLOR_MODE ColorMode = COLOR_MATERIAL);
	virtual ~WireFrameMaterial();

	virtual TYPE GetType() { return TYPE_WIREFRAME; }

	const COLORF & GetColor() { return m_Color; }
	BLEND_MODE GetBlendMode() { return m_BlendMode; }
	COLOR_MODE GetColorMode() { return m_ColorMode; }

	void SetColor(const COLORF &Color) { m_Color = Color; }
	void SetBlendMode(BLEND_MODE BlendMode) { m_BlendMode = BlendMode; }
	void SetColorMode(COLOR_MODE ColorMode) { m_ColorMode = ColorMode; }

private:
	COLORF m_Color;
	BLEND_MODE m_BlendMode;
	COLOR_MODE m_ColorMode;
};

class SolidMaterial : public BaseMaterial
{
public:
	enum COLOR_MODE
	{
		COLOR_MATERIAL, // Kolor brany w ca³oœci z materia³u
		COLOR_MODULATE, // Kolor brany z materia³u RAZY z Team Color encji
		COLOR_LERP_ALPHA, // Kolor brany z materia³u lub z encji, zaleznie od wartoœci alfa na teksturze/kolorze Diffuse (interpolacja)
	};

	// Tworzy zainicjalizowany wartoœciami domyœlnymi
	SolidMaterial(Scene *OwnerScene, const string &Name);
	// Tworzy zainicjalizowany wartoœciami domyœlnymi i podanymi
	SolidMaterial(Scene *OwnerScene, const string &Name, const string &DiffuseTextureName, COLOR_MODE ColorMode = COLOR_MATERIAL);

	const string & GetDiffuseTextureName() { return m_DiffuseTextureName; }
	const COLORF & GetDiffuseColor() { return m_DiffuseColor; }
	const string & GetEmissiveTextureName() { return m_EmissiveTextureName; }
	const string & GetEnvironmentalTextureName() { return m_EnvironmentalTextureName; }
	bool GetTextureAnimation() { return m_TextureAnimation; }
	COLOR_MODE GetColorMode() { return m_ColorMode; }
	const COLORF & GetFresnelColor() { return m_FresnelColor; }
	float GetFresnelPower() { return m_FresnelPower; }

	void SetDiffuseTextureName(const string &DiffuseTextureName) { m_DiffuseTextureName = DiffuseTextureName; m_DiffuseTexture = NULL; }
	void SetDiffuseColor(const COLORF &DiffuseColor) { m_DiffuseColor = DiffuseColor; }
	void SetEmissiveTextureName(const string &EmissiveTextureName) { m_EmissiveTextureName = EmissiveTextureName; m_EmissiveTexture = NULL; }
	void SetEnvironmentalTextureName(const string &EnvironmentalTextureName) { m_EnvironmentalTextureName = EnvironmentalTextureName; m_EnvironmentalTexture = NULL; }
	void SetTextureAnimation(bool TextureAnimation) { m_TextureAnimation = TextureAnimation; }
	void SetColorMode(COLOR_MODE ColorMode) { m_ColorMode = ColorMode; }
	void SetFresnelColor(const COLORF &FresnelColor) { m_FresnelColor = FresnelColor; }
	void SetFresnelPower(float FresnelPower) { m_FresnelPower = FresnelPower; }

	bool IsDiffuseTexture() { return !m_DiffuseTextureName.empty(); }

	// Zwraca zasób danej tekstury, LOADED. Jeœli nie u¿ywa danej tekstury, zwraca NULL.
	res::D3dTexture * GetDiffuseTexture();
	res::D3dTexture * GetEmissiveTexture();
	res::D3dCubeTexture * GetEnvironmentalTexture();

private:
	// Nazwa zasobu tekstury g³ównej z kolorem.
	// Jeœli pusty, wtedy nie u¿ywa tekstury, u¿ywa koloru.
	string m_DiffuseTextureName;
	// Kolor g³ówny jednolity materia³u.
	// Wa¿ny tylko kiedy m_DiffuseTexture pusty.
	COLORF m_DiffuseColor;
	// Dodatkowa tekstura emisji
	// Jeœli pusty, nie ma tekstury Emissive.
	string m_EmissiveTextureName;
	// Dodatkowa tekstura do mapowania œrodowiskowego. Mo¿e byæ pusty.
	string m_EnvironmentalTextureName;
	// Czy tekstura ma byæ animowana
	// Wspó³rzêdne tekstury mno¿one razy macierz tekstury pamiêtana dla encji.
	bool m_TextureAnimation;
	COLOR_MODE m_ColorMode;
	// Kolor podœwietlenia Fresnel Term.
	// W nim te¿ zakodowana jest jasnoœæ efektu.
	// Czarny oznacza Fresnel Term wy³¹czony.
	COLORF m_FresnelColor;
	// Wyk³adnik - intensywnoœæ efektu.
	// Domyœlnie jest 1. Wartoœæ 0..1 powoduje wiêksze pokrycie modelu. Wartoœæ 0..kilka powoduje podœwietlenie tylko na brzegach.
	float m_FresnelPower;

	res::D3dTexture *m_DiffuseTexture;
	res::D3dTexture *m_EmissiveTexture;
	res::D3dCubeTexture *m_EnvironmentalTexture;
};

// Materia³ pó³przezroczysty
class TranslucentMaterial : public SolidMaterial
{
public:
	enum ALPHA_MODE
	{
		ALPHA_MATERIAL, // Kana³ Alfa brany tylko z materia³u
		ALPHA_ENTITY,   // Kana³ Alfa brany tylko z Team Color encji
		ALPHA_MODULATE, // Kana³ Alfa brany z materia³u RAZY z Team Color encji
	};

	TranslucentMaterial(Scene *OwnerScene, const string &Name);
	TranslucentMaterial(Scene *OwnerScene, const string &Name, const string &DiffuseTextureName, BLEND_MODE BlendMode = BLEND_LERP, COLOR_MODE ColorMode = COLOR_MATERIAL, ALPHA_MODE AlphaMode = ALPHA_MATERIAL);
	virtual ~TranslucentMaterial() { }

	virtual TYPE GetType() { return TYPE_TRANSLUCENT; }

	BLEND_MODE GetBlendMode() { return m_BlendMode; }
	ALPHA_MODE GetAlphaMode() { return m_AlphaMode; }
	
	void SetBlendMode(BLEND_MODE BlendMode) { m_BlendMode = BlendMode; }
	void SetAlphaMode(ALPHA_MODE AlphaMode) { m_AlphaMode = AlphaMode; }

private:
	BLEND_MODE m_BlendMode;
	ALPHA_MODE m_AlphaMode;
};

// Materia³ nieprzezroczysty
class OpaqueMaterial : public SolidMaterial
{
public:
	enum SPECULAR_MODE
	{
		// Brak speculara (domyœlnie)
		SPECULAR_NONE,
		// Zwyk³y specular
		SPECULAR_NORMAL,
		// Anisotropic Lighting brany z wektora Tangent
		// (Dzia³a tylko kiedy PerPixelLighting == true && NormalTextureName == "")
		SPECULAR_ANISO_TANGENT,
		// Anisotropic Lighting brany z wektora Binormal
		// (Dzia³a tylko kiedy PerPixelLighting == true && NormalTextureName == "")
		SPECULAR_ANISO_BINORMAL,
	};

	// Tworzy zainicjalizowany wartoœciami domyœlnymi
	OpaqueMaterial(Scene *OwnerScene, const string &Name);
	// Tworzy zainicjalizowany wartoœciami domyœlnymi i podanymi
	OpaqueMaterial(Scene *OwnerScene, const string &Name, const string &DiffuseTextureName, COLOR_MODE ColorMode = COLOR_MATERIAL);
	virtual ~OpaqueMaterial() { }

	virtual TYPE GetType() { return TYPE_OPAQUE; }

	uint1 GetAlphaTesting() { return m_AlphaTesting; }
	bool GetHalfLambert() { return m_HalfLambert; }
	bool GetPerPixel() { return m_PerPixel; }
	const string & GetNormalTextureName() { return m_NormalTextureName; }
	// Zwraca zasób tekstury, LOADED. Jeœli nie u¿ywa takiej tekstury, zwraca NULL.
	res::D3dTexture * GetNormalTexture();
	SPECULAR_MODE GetSpecularMode() { return m_SpecularMode; }
	const COLORF & GetSpecularColor() { return m_SpecularColor; }
	float GetSpecularPower() { return m_SpecularPower; }
	bool GetGlossMapping() { return m_GlossMapping; }

	void SetAlphaTesting(uint1 AlphaTesting) { m_AlphaTesting = AlphaTesting; }
	void SetHalfLambert(bool HalfLambert) { m_HalfLambert = HalfLambert; }
	void SetPerPixel(bool PerPixel) { m_PerPixel = PerPixel; }
	void SetNormalTextureName(const string &NormalTextureName) { m_NormalTextureName = NormalTextureName; m_NormalTexture = NULL; }
	void SetSpecularMode(SPECULAR_MODE SpecularMode) { m_SpecularMode = SpecularMode; }
	void SetSpecularColor(const COLORF &SpecularColor) { m_SpecularColor = SpecularColor; }
	void SetSpecularPower(float SpecularPower) { m_SpecularPower = SpecularPower; }
	void SetGlossMapping(bool GlossMapping) { m_GlossMapping = GlossMapping; }

private:
	// Alfa-testing - piksel jest widoczny jeœli kana³ alfa tekstury Diffuse jest wiêkszy lub równy podanej wartoœci
	// Brak alfa testingu to 0 (domyœlnie).
	uint1 m_AlphaTesting;
	bool m_HalfLambert;
	bool m_PerPixel;
	string m_NormalTextureName;
	res::D3dTexture *m_NormalTexture;
	// Tryb speculara (patrz opis enuma)
	SPECULAR_MODE m_SpecularMode;
	// Kolor speculara
	// - COLORF::WHITE = pe³ny kolor œwiat³a (domyœlnie)
	// (SpecularMode != SM_NONE)
	COLORF m_SpecularColor;
	// Wyk³adnik speculara
	// (SpecularMode != SM_NONE)
	float m_SpecularPower;
	// Czy specular ma byæ mno¿ony przez kana³ alfa tekstury Diffuse
	// (SpecularMode != SM_NONE)
	bool m_GlossMapping;
};

class MaterialCollection
{
public:
	MaterialCollection();
	~MaterialCollection();

	bool Exists(const string &Name);
	bool Exists(BaseMaterial *Material);
	// Zwraca materia³ o podanej nazwie lub NULL jeœli nie znajdzie.
	BaseMaterial * GetByName(const string &Name);
	// Zwraca materia³ o podanej nazwie. Jeœli nie znajdzie, rzuca wyj¹tek.
	BaseMaterial * MustGetByName(const string &Name);
	// Zwraca materia³ podanego typu o podanej nazwie lub NULL jeœli nie znajdzie albo jeœli jest innego typu
	template <typename T> T * GetByNameEx(const string &Name)
	{
		BaseMaterial *M = GetByName(Name);
		if (M == NULL) return NULL;
		return dynamic_cast<T*>(M);
	}
	// Zwraca materia³ podanego typu o podanej nazwie. Jeœli nie znajdzie lub nie jest tego typu, rzuca wyj¹tek.
	template <typename T> T * MustGetByNameEx(const string &Name)
	{
		BaseMaterial *M = MustGetByName(Name);
		if (typeid(*M) != typeid(T)) throw Error(Format("engine::MaterialCollection::MustGetByNameEx: B³êdny typ materia³u. Nazwa=#, oczekiwany=#, znaleziony=#") % Name % typeid(T).name() % typeid(*M).name(), __FILE__, __LINE__);
		return static_cast<T*>(M);
	}

	// ======== Tylko dla klasy BaseMaterial i pochodnych ========
	void RegisterMaterial(const string &Name, BaseMaterial *Material);
	void UnregisterMaterial(const string &Name, BaseMaterial *Material);

private:
	typedef std::map<string, BaseMaterial*> MATERIAL_MAP;

	MATERIAL_MAP m_Map;
	bool m_Destroying;
};

// Abstrakcyjna klasa bazowa wszystkich œwiate³.
class BaseLight
{
public:
	// Tworzy zainicjalizowany wartoœciami domyœlnymi
	BaseLight(Scene *OwnerScene);
	// Tworzy zainicjalizowany wartoœciami domyœlnymi i podanymi
	BaseLight(Scene *OwnerScene, const COLORF &Color);
	virtual ~BaseLight() { }

	Scene * GetOwnerScene() { return m_OwnerScene; }
	bool GetEnabled() { return m_Enabled; }
	bool GetCastShadow() { return m_CastShadow; }
	bool GetCastSpecular() { return m_CastSpecular; }
	bool GetHalfLambert() { return m_HalfLambert; }
	const COLORF & GetColor() { return m_Color; }
	float GetShadowFactor() { return m_ShadowFactor; }

	void SetEnabled(bool Enabled) { m_Enabled = Enabled; }
	void SetCastShadow(bool CastShadow) { m_CastShadow = CastShadow; }
	void SetCastSpecular(bool CastSpecular) { m_CastSpecular = CastSpecular; }
	void SetHalfLambert(bool HalfLambert) { m_HalfLambert = HalfLambert; }
	void SetColor(const COLORF &Color) { m_Color = Color; }
	void SetShadowFactor(float ShadowFactor) { m_ShadowFactor = ShadowFactor; }

private:
	Scene *m_OwnerScene;
	bool m_Enabled;
	bool m_CastShadow;
	bool m_CastSpecular;
	bool m_HalfLambert;
	COLORF m_Color;
	// Wspó³czynnik œciemniania - jaki procent jasnoœci ma mieæ obszar w cieniu (domyœlnie: 0)
	float m_ShadowFactor;
};

class DirectionalLight : public BaseLight
{
public:
	// Tworzy zainicjalizowany wartoœciami domyœlnymi
	DirectionalLight(Scene *OwnerScene);
	// Tworzy zainicjalizowany wartoœciami domyœlnymi i podanymi
	// Dir ma byæ znormalizowany
	DirectionalLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Dir);
	virtual ~DirectionalLight();

	const VEC3 & GetDir() { return m_Dir; }
	float GetZFar() { return m_ZFar; }

	// Dir ma byæ znormalizowany
	void SetDir(const VEC3 &Dir) { m_Dir = Dir; }
	void SetZFar(float ZFar) { m_ZFar = ZFar; }

private:
	VEC3 m_Dir; // Znormalizowany
	// Mo¿e byæ dowolnie du¿e, np. MAXFLOAT.
	// Jeœli mniejsze od ZFar kamery, ogranicza zasiêg shadow mapowania tego œwiat³a.
	float m_ZFar;
};

// Abstrakcyjna klasa bazowa œwiate³ posiadaj¹cych pozycjê
class PositionedLight : public BaseLight
{
public:
	// Tworzy zainicjalizowany wartoœciami domyœlnymi
	PositionedLight(Scene *OwnerScene);
	// Tworzy zainicjalizowany wartoœciami domyœlnymi i podanymi
	PositionedLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Pos);

	const VEC3 & GetPos() { return m_Pos; }
	void SetPos(const VEC3 &Pos) { if (Pos != m_Pos) { m_Pos = Pos; OnPosChange(); } }

	// Zwraca prostok¹t dla Scissor Test dla obszaru widocznego na ekranie oœwietlonego zasiêgiem tego œwiat³a.
	// Dodatkowo, zwraca procent ekranu objêty tym testem, 0..1.
	float GetScissorRect(RECT *OutRect, const MATRIX &View, const MATRIX &Proj, uint RenderTargetCX, uint RenderTargetCY);

protected:
	// ======== Dla klas potomnych ========
	virtual void OnPosChange() = 0;
	// Ma zwróciæ tablicê 8 wierzcho³ków dowolnie zorientowanego prostopad³oœcianu otaczaj¹cego bry³ê zasiêgu œwiat³a
	virtual const VEC3 * GetBoundingBox() = 0;

private:
	VEC3 m_Pos;
};

class SpotLight : public PositionedLight
{
public:
	// Tworzy zainicjalizowany wartoœciami domyœlnymi
	SpotLight(Scene *OwnerScene);
	// Tworzy zainicjalizowany wartoœciami domyœlnymi i podanymi
	// Dir ma byæ znormalizowany
	SpotLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Pos, const VEC3 &Dir, float Dist, float Fov);
	virtual ~SpotLight();

	const VEC3 & GetDir() { return m_Dir; }
	float GetDist() { return m_Dist; }
	float GetFov() { return m_Fov; }
	bool GetSmooth() { return m_Smooth; }
	float GetZNear() { return m_ZNear; }

	// Dir ma byæ znormalizowany
	void SetDir(const VEC3 &Dir) { if (Dir != m_Dir) { m_Dir = Dir; ResetIs(); } }
	void SetDist(float Dist) { if (Dist != m_Dist) { m_Dist = Dist; ResetIs(); } }
	void SetFov(float Fov) { if (Fov != m_Fov) { m_Fov = Fov; ResetIs(); } }
	void SetSmooth(bool Smooth) { m_Smooth = Smooth; }
	void SetZNear(float ZNear) { if (ZNear != m_ZNear) { m_ZNear = ZNear; ResetIs(); } }

	const FRUSTUM_PLANES & GetFrustumPlanes() { return GetParamsCamera().GetMatrices().GetFrustumPlanes(); }
	const FRUSTUM_POINTS & GetFrustumPoints() { return GetParamsCamera().GetMatrices().GetFrustumPoints(); }
	const ParamsCamera & GetParamsCamera() { EnsureParamsCamera(); return m_ParamsCamera; }

	bool SphereInRange(const VEC3 &SphereCenter, float SphereRadius);
	bool BoxInRange(const BOX &Box);

protected:
	// ======== Implementacja PositionedLight ========
	virtual void OnPosChange() { ResetIs(); }
	virtual const VEC3 * GetBoundingBox() { EnsureBoundingBox(); return m_BoundingBox; }

private:
	VEC3 m_Dir; // Znormalizowany
	float m_Dist; // Zasiêg
	float m_Fov; // K¹t patrzenia w radianach
	bool m_Smooth; // true, jeœli zanikanie wraz z k¹tem wyg³adzone, false jeœli ostra okr¹g³a krawêdŸ
	float m_ZNear; // U¿ywane do Shadow Mappingu

	VEC3 m_BoundingBox[8]; bool m_BoundingBoxIs;
	ParamsCamera m_ParamsCamera; bool m_ParamsCameraIs;

	void ResetIs()
	{
		m_BoundingBoxIs = false;
		m_ParamsCameraIs = false;
	}
	void EnsureBoundingBox();
	void EnsureParamsCamera();
};

/*
Spamiêtuje i uniewa¿nia osobno Shadow Mapy dla ka¿dej z 6 œcian.
*/
class PointLight : public PositionedLight
{
public:
	// Tworzy zainicjalizowany wartoœciami domyœlnymi
	PointLight(Scene *OwnerScene);
	// Tworzy zainicjalizowany wartoœciami domyœlnymi i podanymi
	// Dir ma byæ znormalizowany
	PointLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Pos, float Dist);
	virtual ~PointLight();

	float GetDist() { return m_Dist; }
	float GetZNear() { return m_ZNear; }

	void SetDist(float Dist) { if (Dist != m_Dist) { m_Dist = Dist; m_ParamsCamerasIs = false; m_BoundingBoxIs = false; } }
	void SetZNear(float ZNear) { if (ZNear != m_ZNear) { m_ZNear = ZNear; m_ParamsCamerasIs = false; m_BoundingBoxIs = false; } }

	const ParamsCamera & GetParamsCamera(D3DCUBEMAP_FACES Face) { EnsureParamsCameras(); return m_ParamsCameras[Face]; }
	const FRUSTUM_PLANES & GetSideFrustum(D3DCUBEMAP_FACES Face) { return GetParamsCamera(Face).GetMatrices().GetFrustumPlanes(); }

	bool SphereInRange(const VEC3 &SphereCenter, float SphereRadius);
	bool BoxInRange(const BOX &Box);

protected:
	// ======== Implementacja PositionedLight ========
	virtual void OnPosChange() { m_ParamsCamerasIs = false; m_BoundingBoxIs = false; }
	virtual const VEC3 * GetBoundingBox() { EnsureBoundingBox(); return m_BoundingBox; }

private:
	float m_Dist; // Zasiêg
	float m_ZNear; // U¿ywane do Shadow Mappingu
	ParamsCamera m_ParamsCameras[6]; bool m_ParamsCamerasIs;
	VEC3 m_BoundingBox[8]; bool m_BoundingBoxIs;

	void EnsureBoundingBox();
	void EnsureParamsCameras();
};

// Abstracyjna klasa bazowa dla wszelkich obiektów graficznych silnika.
/*
- Encje podczas tworzenia same dodaj¹ siê do sceny.
- Encje mo¿na usuwaæ rêcznie (zwyk³e delete), wówczas same usuwaj¹ siê ze sceny, lub mo¿na ich
  nie usuwaæ i wtedy usuwa je scena podczas swojej destrukcji.
- Sferê otaczaj¹c¹ encjê otrzymuje siê z GetWorldPos i GetWorldRadius. Jest to szybkie,
  bo wyliczane tylko przy pierwszym odczytaniu od ostatniej zmiany.
*/
class Entity
{
public:
	enum TYPE
	{
		TYPE_MATERIAL,
		TYPE_CUSTOM,
		TYPE_HEAT,
		TYPE_TREE,
	};

	typedef ENTITY_SET::iterator EntityIterator;

	// Tworzy encjê i dodaje j¹ do sceny
	Entity(Scene *OwnerScene);
	// Usuwa encjê i wyrejestrowuje j¹ ze sceny
	virtual ~Entity();

	// ======== W³aœciciel, nad-encja i pod-encje ========
	Scene * GetOwnerScene() { return m_OwnerScene; }
	Entity * GetParent() { return m_Parent; }
	uint GetSubEntitiesCount() { return m_SubEntities.size(); }
	EntityIterator GetSubEntitiesBegin() { return m_SubEntities.begin(); }
	EntityIterator GetSubEntitiesEnd() { return m_SubEntities.end(); }
	// Zmienia encjê nadrzêdn¹
	// Parent - mo¿na podaæ NULL, wówczas encja staje siê g³ównego poziomu
	void SetParent(Entity *Parent);
	// Zmienia encjê nadrzêdn¹, pod³¹cza t¹ encjê do konkretnej koœci encji nadrzêdnej
	void SetParentBone(Entity *Parent, const string &ParentBone);

	// Pozycja œrodka encji
	const VEC3 & GetPos() { return m_Pos; }
	void SetPos(const VEC3 &Pos) { m_Pos = Pos; OnTransformChange(); }
	void ChangePos(const VEC3 &PosDiff) { m_Pos += PosDiff; OnTransformChange(); }
	void ResetPos() { m_Pos = VEC3::ZERO; OnTransformChange(); }
	// Orientacja (obrót)
	const QUATERNION & GetOrientation() { return m_Orientation; }
	void SetOrientation(const QUATERNION &Orientation) { m_Orientation = Orientation; m_IsOrientation = (m_Orientation != QUATERNION::IDENTITY); OnTransformChange(); }
	void ResetOriantation() { m_Orientation = QUATERNION::IDENTITY; m_IsOrientation = false; OnTransformChange(); }
	// Skalowanie
	float GetSize() { return m_Size; }
	void SetSize(float Size) { m_Size = Size; OnTransformChange(); }
	// Promieñ sfery otaczaj¹cej
	float GetRadius() { return m_Radius; }
	void SetRadius(float Radius) { m_Radius = Radius; OnTransformChange(); }
	// Widocznoœæ obiektu
	bool GetVisible() { return m_Visible; }
	void SetVisible(bool Visible) { m_Visible = Visible; m_WorldVisibleCalculated = false; }
	// Tag - flaga bitowa do u¿ytku w³asnego
	uint4 GetTag() { return m_Tag; }
	void SetTag(uint4 Tag) { m_Tag = Tag; }

	// Zwraca macierz lokaln¹ tego obiektu
	const MATRIX & GetLocalMatrix() { EnsureLocalMatrix(); return m_LocalMatrix; }
	// Zwraca odwrotnoœæ macierzy lokalnej tego obiektu
	const MATRIX & GetInvLocalMatrix() { EnsureInvLocalMatrix(); return m_InvLocalMatrix; }
	// Zwraca macierz œwiata tego obiektu
	const MATRIX & GetWorldMatrix() { EnsureWorldMatrix(); return m_WorldMatrix; }
	// Zwraca odwrotnoœæ macierzy œwiata tego obiektu
	const MATRIX & GetInvWorldMatrix() { EnsureInvWorldMatrix(); return m_InvWorldMatrix; }
	// Zwraca pozycjê œrodka encji we wsp. globalnych œwiata
	const VEC3 & GetWorldPos() { EnsureWorldPos(); return m_WorldPos; }
	// Zwraca skalowanie we wsp. globalnych œwiata
	float GetWorldSize() { EnsureWorldSize(); return m_WorldSize; }
	// Zwraca promieñ sfery otaczaj¹cej we wsp. globalnych œwiata
	float GetWorldRadius() { EnsureWorldRadius(); return m_WorldRadius; }
	// Zwraca czy obiekt jest bezwzglêdnie widoczny, tzn. jest widoczny on i jego podencje
	bool GetWorldVisible() { EnsureWorldVisible(); return m_WorldVisible; }

	// ======== Dla klas pochodnych ========
	virtual TYPE GetType() = 0;
	// Kolizja promienia o parametrach podanych we wsp. lokalnych modelu
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT) = 0;
	// Ma zwróciæ macierz koœci o podanej nazwie
	// Tzn. przekszta³cenie ze wsp. lokalnych tego modelu w Bind Pose do te¿ wsp. lokalnych ale w bie¿¹cej pozycji
	// Jeœli takiej koœci nie ma, ma zwróciæ MATRIX::IDENTITY.
	virtual const MATRIX & GetBoneMatrix(const string &BoneName) { return MATRIX::IDENTITY; }
	// Ma siê policzyæ
	virtual void Update() { }

	// ======== Dla innych obiektów klasy Entity ========
	void RegisterSubEntity(Entity *SubEntity);
	void UnregisterSubEntity(Entity *SubEntity);

	// ======== Dla klasy EntityOctree ========
	ENTITY_OCTREE_NODE * GetOctreeNode() { return m_OctreeNode; }
	void SetOctreeNode(ENTITY_OCTREE_NODE *Node) { m_OctreeNode = Node; }

private:
	bool m_Destroying;
	Scene *m_OwnerScene;
	ENTITY_OCTREE_NODE *m_OctreeNode;
	Entity *m_Parent; // NULL jeœli to encja poziomu g³ównego
	string m_ParentBone; // Nazwa koœci obiektu nadrzêdnego, do której podczepiona jest ta encja. £añcuch pusty jeœli brak.
	ENTITY_SET m_SubEntities;
	VEC3 m_Pos;
	QUATERNION m_Orientation; // K¹ty Eulera
	bool m_IsOrientation; // true jeœli orientacja jest niezerowa lub niewiadomo jaka
	float m_Size;
	float m_Radius;
	// Macierz przekszta³caj¹ca ze wsp. tego obiektu do wsp. obiektu nadrzêdnego
	MATRIX m_LocalMatrix; bool m_LocalMatrixCalculated;
	MATRIX m_InvLocalMatrix; bool m_InvLocalMatrixCalculated;
	// Macierz przekszta³caj¹ca ze wsp. tego obiektu do wsp. globalnych œwiata
	MATRIX m_WorldMatrix; bool m_WorldMatrixCalculated;
	MATRIX m_InvWorldMatrix; bool m_InvWorldMatrixCalculated;
	// Pozycja œrodka we wsp. globalnych œwiata
	VEC3 m_WorldPos; bool m_WorldPosCalculated;
	// Zebrane skalowanie we wsp. œwiata
	float m_WorldSize; bool m_WorldSizeCalculated;
	float m_WorldRadius; bool m_WorldRadiusCalculated;
	// Zebrana widocznoœæ (Visible jest dziedziczone na pod-encje)
	bool m_WorldVisible; bool m_WorldVisibleCalculated;
	bool m_Visible;
	uint4 m_Tag;

	// Któryœ z parametrów transformacji siê zmieni³ - wyliczone dane s¹ ju¿ nieaktualne
	void OnTransformChange();
	// Wylicza w razie potrzeby dane dane tak, ¿eby na pewno by³y wyliczone.
	void EnsureLocalMatrix();
	void EnsureInvLocalMatrix();
	void EnsureWorldMatrix();
	void EnsureInvWorldMatrix();
	void EnsureWorldPos();
	void EnsureWorldSize();
	void EnsureWorldRadius();
	void EnsureWorldVisible();
};

struct ENTITY_FRAGMENT
{
	// WskaŸnik do encji
	Entity *E;
	// Identyfikator fragmentu do u¿ytku w³asnego encji
	uint FragmentId;
	// WskaŸnik do materia³u lub NULL jeœli to CustomEntity
	BaseMaterial *M;
	// WorldPos encji
	VEC3 Pos;

	ENTITY_FRAGMENT(Entity *E, uint FragmentId, BaseMaterial *M, const VEC3 &Pos) : E(E), FragmentId(FragmentId), M(M), Pos(Pos) { }
};

/*
- Tylko encje tego typu mog¹ podlegaæ oœwietleniu albo rzucaæ cieñ.
- Team Color to kolor pamiêtany dla poszczególnych encji.
  Nie jest dziedziczony do encji podrzêdnych.
  Mo¿e wp³ywaæ na kolor rysowanego obiektu, zale¿nie od ustawieñ materia³u.
*/
class MaterialEntity : public Entity
{
public:
	MaterialEntity(Scene *OwnerScene);
	virtual ~MaterialEntity() { }

	const COLORF & GetTeamColor() { return m_TeamColor; }
	const MATRIX & GetTextureMatrix() { return m_TextureMatrix; }
	bool GetUseLighting() { return m_UseLighting; }
	bool GetCastShadow() { return m_CastShadow; }
	bool GetReceiveShadow() { return m_ReceiveShadow; }

	void SetTeamColor(const COLORF &TeamColor) { m_TeamColor = TeamColor; }
	void SetTextureMatrix(const MATRIX &TextureMatrix) { m_TextureMatrix = TextureMatrix; }
	void SetUseLighting(bool UseLighting) { m_UseLighting = UseLighting; }
	void SetCastShadow(bool CastShadow) { m_CastShadow = CastShadow; }
	void SetReceiveShadow(bool ReceiveShadow) { m_ReceiveShadow = ReceiveShadow; }

	// ======== Implementacja Entity ========
	virtual TYPE GetType() { return TYPE_MATERIAL; }

	// ======== Dla klas potomnych ========
	// Zwracanie informacji o fragmentach
	virtual uint GetFragmentCount() = 0;
	virtual void GetFragmentData(uint Index, uint *OutId, BaseMaterial **OutMaterial) = 0;
	// Narysowanie fragmentu
	// Stany ju¿ ma ustawione. Jej zadaniem jest tylko wykonaæ SetStreamSource, SetIndices, SetFVF, Draw[Indexed]Primitive[UP] oraz frame::RegisterDrawCall.
	virtual void DrawFragmentGeometry(uint Id, BaseMaterial *Mat, const ParamsCamera &Cam) = 0;
	// Zwraca bie¿¹ce dane do Skinningu encji.
	// Jeœli zwróci false, nie stosuje Skinningu.
	// Jeœli zwróci true, stosuje Skinning. Ma wtedy zwróciæ BoneCount i BoneMatrices.
	// OutBoneMatrices to wskaŸnik do tablicy - ma zwróciæ wskaŸnik do swojej wewnêtrznej struktury danych z tymi macierzami.
	virtual bool GetSkinningData(uint *OutBoneCount, const MATRIX **OutBoneMatrices) { return false; }

	// Dla klasy Scene
	void DrawFragment(PASS Pass, const ENTITY_FRAGMENT &Fragment, const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam, BaseLight *Light);
	ENTITY_FRAGMENT GetFragment(uint Index);

private:
	COLORF m_TeamColor;
	MATRIX m_TextureMatrix;
	bool m_UseLighting;
	bool m_CastShadow;
	bool m_ReceiveShadow;
};

class CustomEntity : public Entity
{
public:
	CustomEntity(Scene *OwnerScene) : Entity(OwnerScene) { }
	virtual ~CustomEntity() { }

	// ======== Implementacja Entity ========
	virtual TYPE GetType() { return TYPE_CUSTOM; }

	// ======== Dla klas potomnych ========
	virtual void Draw(const ParamsCamera &Cam) = 0;

private:
};

class HeatEntity : public Entity
{
public:
	HeatEntity(Scene *OwnerScene) : Entity(OwnerScene) { }
	virtual ~HeatEntity() { }

	// ======== Implementacja Entity ========
	virtual TYPE GetType() { return TYPE_HEAT; }

	////// Dla klasy Scene
	void Draw(const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam);

protected:
	// ======== Dla klas potomnych ========
	virtual void DrawGeometry() = 0;
};

class TreeEntity : public Entity
{
public:
	TreeEntity(Scene *OwnerScene);
	TreeEntity(Scene *OwnerScene, Tree *a_Tree, uint Kind = 0, const COLORF &Color = COLORF::WHITE);
	virtual ~TreeEntity() { }

	Tree * GetTree() { return m_Tree; }
	uint GetKind() { return m_Kind; }
	const COLORF & GetColor() { return m_Color; }
	void SetTree(Tree *a_Tree);
	void SetKind(uint Kind) { m_Kind = Kind; }
	void SetColor(const COLORF &Color) { m_Color = Color; }

	// ======== Implementacja Entity ========
	virtual TYPE GetType() { return TYPE_TREE; }
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT);

private:
	Tree *m_Tree;
	uint m_Kind;
	COLORF m_Color;
};

class HeatSphere : public HeatEntity
{
public:
	// - Trzy promienie w trzech ró¿nych kierunkach.
	// - Intensity to intensywnoœæ efektu, 0..1.
	HeatSphere(Scene *OwnerScene, const VEC3 &Radius, float Intensity = 1.f);
	~HeatSphere();

	// ======== Implementacja Entity ========
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT) { return false; }
	virtual void Update() { }

protected:
	// ======== Implementacja HeatEntity ========
	void DrawGeometry();

private:
	float m_Intensity;
	VEC3 m_Radius;
	res::D3dVertexBuffer *m_VB;
	res::D3dIndexBuffer *m_IB;

	void FillBuffers();
	void DrawHeatGeometry();
};

struct STATS
{
	string RunningOptimizerOptions;
	uint Passes;
	uint Entities[2]; // Wybrane z Octree + Frustum Culling, wybrane z Occlusion Query
	uint PointLights[2]; // Wybrane z Frustum Culling, wybrane z Occlusion Query
	uint SpotLights[2]; // Wybrane z Frustum Culling, wybrane z Occlusion Query
	uint MapFragments;
	uint TerrainPatches;
	uint Trees;
	uint MainShaders, PpShaders;

	// Inicjalizuje zerami
	STATS();
};

// Deskryptor z danymi drzewa przeznaczonego do narysowania
struct TREE_DRAW_DESC
{
	Tree *tree;
	float scaling;
	MATRIX world;
	MATRIX world_inv;
	uint kind;
	COLORF color;

	// Porównywanie, ¿eby sortowaæ grupuj¹c wg obiektów klasy Tree.
	bool operator < (const TREE_DRAW_DESC &desc2) { return (tree < desc2.tree); }
};

typedef std::vector<TREE_DRAW_DESC> TREE_DRAW_DESC_VECTOR;

/*
W chwili tworzenia i przez ca³y czas istnienia obiektu tej klasy istnieæ musi
podany do konstruktora teren oraz tekstury u¿ywane przez ten teren.
*/
class TerrainRenderer
{
public:
	// TreeDescFileName - opcjonalny, mo¿e byæ pusty ³añcuch. Opis formatu: plik Trees.txt.
	// TreeDensityMapFileName - opcjonalny
	// GrassObj, WaterObj - opcjonalny, mo¿e byæ NULL.
	//   Jeœli podane, przechodz¹ na w³asnoœæ TerrainRenderer i on je zwolni.
	TerrainRenderer(Scene *OwnerScene, Terrain *terrain, Grass *GrassObj, TerrainWater *WaterObj,
		const string &TreeDescFileName, const string &TreeDensityMapFileName);
	~TerrainRenderer();

	Terrain * GetTerrain() { return m_Terrain; }

	Tree & GetTreeByIndex(uint Index) { assert(Index < m_Trees.size()); return *m_Trees[Index].Tree_.get(); }
	// Jeœli nie znajdzie, zwraca NULL.
	Tree * GetTreeByName(const string &Name);
	// Jeœli nie znajdzie, rzuca wyj¹tek.
	Tree & MustGetTreeByName(const string &Name);
	// Dodaje do wektora deskryptory drzew zawartych we frustumie podanej kamery
	void GetTreesInFrustum(TREE_DRAW_DESC_VECTOR *InOut, const ParamsCamera &Cam, bool FrustumCulling);
	void GetTreesCastingDirectionalShadow(TREE_DRAW_DESC_VECTOR *InOut, const FRUSTUM_PLANES &CamFrustum, const BOX &CamBox, const VEC3 &LightDir);

	////// Dla klasy Scene

	void DrawPatch(PASS Pass, uint PatchIndex, const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam, BaseLight *Light);
	void DrawGrass(const ParamsCamera &Cam, const COLORF &Color);
	void DrawWater(const ParamsCamera &Cam);

private:
	// Opis gatunku drzewa - dodatkowe informacje towarzysz¹ce obiektowi Tree.
	// Oprócz nazwy, bo jest trzymana na zewn¹trz, w mapie m_TreeNames.
	struct TREE_DATA
	{
		shared_ptr<Tree> Tree_;
		uint KindCount;
		uint Index; // Indeks tego elementu w tablicy m_Trees.
		COLOR DensityMapColor; // A=0 oznacza brak reprezentacji drzewa jako koloru, A=255 oznacza, ¿e jest.

		TREE_DATA(uint Index, shared_ptr<Tree> Tree_, uint KindCount, COLOR DensityMapColor);
		~TREE_DATA();
	};

	// Cacheowane informacje o drzewach w danym sektorze
	struct PATCH
	{
		TREE_DRAW_DESC_VECTOR TreeDescs;
		uint px, pz;
	};

	Scene *m_OwnerScene;
	Terrain *m_Terrain;
	scoped_ptr<Grass> m_GrassObj; // NULL, jeœli nie ma
	scoped_ptr<TerrainWater> m_WaterObj; // NULL, jeœli nie ma
	scoped_ptr<TerrainMaterial> m_Material;

	////// Drzewa
	// Definicje drzew
	std::vector<TREE_DATA> m_Trees;
	// Mapa odwzorowuj¹ca nazwy drzew (tych które maj¹ nazwy) na ich indeksy w tablicy m_Trees
	std::map<string, uint> m_TreeNames;
	// Tree Density Map - indeksy rodzajów drzew z m_Trees na poszczególnych polach
	// Jeœli tablica jest pusta, i CX i CY = 0, nie ma TreeDensityMap
	// Element w tablicy równy 0xFF oznacza brak drzewa na danym polu.
	uint m_TreeDensityMapCX;
	uint m_TreeDensityMapCY;
	// Liczba patchów na X i na Z
	uint m_TreePatchesX;
	uint m_TreePatchesZ;
	std::vector<uint1> m_TreeDensityMap;
	// Cache. Ostatnio u¿ywane s¹ na koñcu.
	std::vector< shared_ptr<PATCH> > m_PatchCache;

	void LoadTreeDescFile(const string &TreeDescFileName);
	void LoadTreeDensityMap(const string &FileName);
	const PATCH & EnsurePatch(uint px, uint pz);
	// Patch ma wype³niony px i pz, funkcja ma za zadanie wygenerowaæ TreeDescs
	void GeneratePatch(PATCH *P);
};

// Bufor pamiêtaj¹cy okreœlon¹ liczbê ostatnich próbek.
// Daje dostêp do ich sumy, iloczynu i œredniej.
template <typename T>
class SampleBuffer
{
public:
	SampleBuffer(uint SampleCount);

	void AddSample(T Value);

	bool IsEmpty();
	T GetSum();
	T GetProduct();
	T GetAverage();

private:
	uint m_SampleCount;
	uint m_RealSampleCount;
	uint m_Index;
	std::vector<T> m_Samples;
};

template <typename T>
SampleBuffer<T>::SampleBuffer(uint SampleCount) :
	m_SampleCount(SampleCount),
	m_RealSampleCount(0),
	m_Index(0)
{
	m_Samples.resize(m_SampleCount);
}

template <typename T>
void SampleBuffer<T>::AddSample(T Value)
{
	m_Samples[m_Index] = Value;
	m_Index = (m_Index + 1) % m_SampleCount;
	if (m_RealSampleCount < m_SampleCount)
		m_RealSampleCount++;
}

template <typename T>
bool SampleBuffer<T>::IsEmpty()
{
	return (m_RealSampleCount == 0);
}

template <typename T>
T SampleBuffer<T>::GetSum()
{
	if (m_RealSampleCount == 0)
		return T();
	else if (m_RealSampleCount < m_SampleCount)
	{
		T Result = m_Samples[0];
		for (uint i = 1; i < m_RealSampleCount; i++)
			Result += m_Samples[i];
		return Result;
	}
	else
	{
		T Result = m_Samples[0];
		for (uint i = 1; i < m_SampleCount; i++)
			Result += m_Samples[i];
		return Result;
	}
}

template <typename T>
T SampleBuffer<T>::GetProduct()
{
	if (m_RealSampleCount == 0)
		return T();
	else if (m_RealSampleCount < m_SampleCount)
	{
		T Result = m_Samples[0];
		for (uint i = 1; i < m_RealSampleCount; i++)
			Result *= m_Samples[i];
		return Result;
	}
	else
	{
		T Result = m_Samples[0];
		for (uint i = 1; i < m_SampleCount; i++)
			Result *= m_Samples[i];
		return Result;
	}
}

template <typename T>
T SampleBuffer<T>::GetAverage()
{
	if (m_RealSampleCount == 0)
		return T();
	else if (m_RealSampleCount < m_SampleCount)
	{
		T Result = m_Samples[0];
		for (uint i = 1; i < m_RealSampleCount; i++)
			Result += m_Samples[i];
		return Result / m_RealSampleCount;
	}
	else
	{
		T Result = m_Samples[0];
		for (uint i = 1; i < m_SampleCount; i++)
			Result += m_Samples[i];
		return Result / m_SampleCount;
	}
}

// KLASA U¯YWANA WEWNÊTRZNIE
class RunningOptimizer
{
public:
	enum SETTING
	{
		// Czy sortowaæ fragmenty wg materia³u
		SETTING_MATERIAL_SORT,
		// Czy wykonywaæ Occlusion Query dla encji
		SETTING_ENTITY_OCCLUSION_QUERY,
		// Czy wykonywaæ Occlusion Query dla œwiate³
		SETTING_LIGHT_OCCLUSION_QUERY,
		// Czy wykonywaæ frustum culling dla drzew rysowanych na terenie
		SETTING_TREE_FRUSTUM_CULLING,
		// (Liczba ustawieñ)
		SETTING_COUNT
	};

	typedef std::bitset<SETTING_COUNT> SETTINGS;

	RunningOptimizer();

	// Wywo³ywaæ na pocz¹tku klatki. Robi dwie rzeczy:
	// 1. Zwraca ustawienia do u¿ycia w bie¿¹cej klatce
	// 2. Odbiera i analizuje czas jaki trwa³a poprzednia klatka
	void OnFrame(SETTINGS *OutNewSettings, float LastFrameTime);

	static void SettingsToStr(string *Out, const SETTINGS &Settings);

private:
	uint m_FrameNumber;
	SETTINGS m_CurrentSettings;
	// Indeks opcji, która ma byæ teraz lub jest testowana na zmianê
	uint m_SettingTestIndex;
	// Œredni czas klatek dla ustawieñ normalnych
	SampleBuffer<float> m_NormalFrameTimeSamples;
	// Dla ka¿dego ustawienia pamiêta sumê wartoœci 1 lub -1 zale¿nie czy siê op³aca, czy nie op³aca zmieniaæ na przeciwne
	int m_FlipSums[SETTING_COUNT];
	// Dla ka¿dego ustawienia pamiêta liczbê próbek dla wartoœci przeciwnej
	uint m_FlipCounts[SETTING_COUNT];
};

// Scena - przechowuje kolekcjê encji i organizuje ich rysowanie
class Scene
{
public:
	enum COLLISION_RESULT
	{
		COLLISION_RESULT_NONE,
		COLLISION_RESULT_MAP,
		COLLISION_RESULT_TERRAIN,
		COLLISION_RESULT_ENTITY,
	};

	typedef CAMERA_SET::iterator CameraIterator;
	typedef ENTITY_SET::iterator EntityIterator;

	Scene(const BOX &Bounds);
	~Scene();

	MaterialCollection & GetMaterialCollection() { return m_MaterialCollection; }

	const COLORF & GetAmbientColor() { return m_AmbientColor; }
	void SetAmbientColor(const COLORF &AmbientColor) { m_AmbientColor = AmbientColor; }

	// ======== Wiatr ========

	const VEC2 & GetWindVec() { return m_WindVec; }
	void SetWindVec(const VEC2 &WindVec) { m_WindVec = WindVec; }

	// ======== Mg³a ========

	bool GetFogEnabled() { return m_Fog_Enabled; }
	float GetFogStart() { return m_Fog_Start; }
	const COLORF & GetFogColor() { return m_Fog_Color; }
	void SetFogEnabled(bool FogEnabled) { m_Fog_Enabled = FogEnabled; }
	void SetFogStart(float FogStart) { m_Fog_Start = FogStart; }
	void SetFogColor(const COLORF &FogColor) { m_Fog_Color = FogColor; }

	// ======== Niebo ========

	// Przekazany obiekt przechodzi na w³asnoœæ sceny - ona go usunie.
	void SetSky(BaseSky *Sky);
	BaseSky * GetSky();
	void ResetSky();

	// ======== Opady ========

	// Przekazany obiekt przechodzi na w³asnoœæ sceny - ona go usunie.
	void SetFall(Fall *a_Fall);
	Fall * GetFall();
	void ResetFall();

	// ======== Mapa ========
	// W ka¿dej chwili w scenie mo¿e byæ 0 lub 1 mapa.

	void SetQMap(QMap *Map);
	void SetQMap(const string &QMapName);
	void ResetQMap();
	bool GetMapUseLighting() { return m_MapUseLighting; }
	bool GetMapCastShadow() { return m_MapCastShadow; }
	bool GetMapReceiveShadow() { return m_MapReceiveShadow; }
	void SetMapUseLighting(bool MapUseLighting) { m_MapUseLighting = MapUseLighting; }
	void SetMapCastShadow(bool MapCastShadow) { m_MapCastShadow = MapCastShadow; }
	void SetMapReceiveShadow(bool MapReceiveShadow) { m_MapReceiveShadow = MapReceiveShadow; }

	// ======== Teren ========
	// W ka¿dej chwili w scenie mo¿e byæ 0 lub 1 teren.

	// Obiekt przechodzi na w³asnoœæ sceny i zostanie przez ni¹ zwolniony.
	void SetTerrain(TerrainRenderer *terrain_renderer);
	void ResetTerrain();
	TerrainRenderer * GetTerrain() { return m_TerrainRenderer.get(); }

	// ======== Kamery ========
	// Kamery tworzy siê i usuwa po prostu new Camera() i delete MyCam, podaj¹c do konstruktora wskaŸnik do sceny.

	// Ustawia kamerê aktywn¹.
	// Mo¿na podaæ NULL - wtedy scena nie jest renderowana.
	// Podana kamera musi nale¿eæ do tej sceny.
	void SetActiveCamera(Camera *cam);
	Camera * GetActiveCamera() { return m_ActiveCamera; }
	uint GetCameraCount() { return m_Cameras.size(); }
	CameraIterator GetCameraBegin() { return m_Cameras.begin(); }
	CameraIterator GetCameraEnd() { return m_Cameras.end(); }

	// ======== Encje ========

	// Dostêp do kolekcji encji g³ównego poziomu
	uint GetRootEntityCount() { return m_RootEntities.size(); }
	EntityIterator GetRootEntityBegin() { return m_RootEntities.begin(); }
	EntityIterator GetRootEntityEnd() { return m_RootEntities.end(); }

	// ======== Œwiat³a ========
	// Œwiat³a tworzy siê i usuwa po prostu new TypŒwiat³a() i delete Œwiat³o, podaj¹c do konstruktora wskaŸnik sceny.
	// Na raz mo¿e istnieæ w scenie co najwy¿ej jedno œwiat³o DirectionalLight.

	// ======== Postprocessing ========

	// Ustawiane tu obiekty efektów postprocessingu przechodz¹ na w³asnoœæ obiektu sceny.

	void SetPpColor(PpColor *e);
	PpColor * GetPpColor();
	void SetPpTexture(PpTexture *e);
	PpTexture * GetPpTexture();
	void SetPpFunction(PpFunction *e);
	PpFunction * GetPpFunction();
	void SetPpToneMapping(PpToneMapping *e);
	PpToneMapping * GetPpToneMapping();
	void SetPpBloom(PpBloom *e);
	PpBloom * GetPpBloom();
	void SetPpFeedback(PpFeedback *e);
	PpFeedback * GetPpFeedback();
	void SetPpLensFlare(PpLensFlare *e);
	PpLensFlare * GetPpLensFlare();

	// ======== G³ówne =========

	void Update();
	void Draw(STATS *OutStats);
	// OutT ustawiane na sensown¹ wartoœæ tylko jeœli zwróci³a != COLLISION_RESULT_NONE.
	// OutEntity ustawiane na sensown¹ wartoœæ tylko jeœli zwróci³a == COLLISION_RESULT_ENTITY.
	COLLISION_RESULT RayCollision(COLLISION_TYPE CollisionType, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT, Entity **OutEntity);

	// ======== Tylko dla klasy Camera ========
	void RegisterCamera(Camera *cam);
	void UnregisterCamera(Camera *cam);

	// ======== Tylko dla klasy Entity ========
	void RegisterEntity(Entity *e);
	void UnregisterEntity(Entity *e);
	void RegisterRootEntity(Entity *e);
	void UnregisterRootEntity(Entity *e);
	void OnEntityParamsChange(Entity *e);

	// ======== Tylko dla klas œwiate³ ========
	void RegisterDirectionalLight(DirectionalLight *l);
	void UnregisterDirectionalLight(DirectionalLight *l);
	void RegisterSpotLight(SpotLight *l);
	void UnregisterSpotLight(SpotLight *l);
	void RegisterPointLight(PointLight *l);
	void UnregisterPointLight(PointLight *l);

private:
	bool m_Destroying;

	COLORF m_AmbientColor;
	MaterialCollection m_MaterialCollection;
	scoped_ptr<RunningOptimizer> m_RunningOptimizer;

	////// Wiatr
	// Kierunek i d³ugoœæ okreœlaj¹ kierunek w osi XZ i si³ê (prêdkoœæ) wiatru
	VEC2 m_WindVec;

	////// Mapa

	scoped_ptr<QMapRenderer> m_QMapRenderer;
	bool m_MapUseLighting;
	bool m_MapCastShadow;
	bool m_MapReceiveShadow;

	////// Teren

	scoped_ptr<TerrainRenderer> m_TerrainRenderer;

	////// Mg³a
	// Mg³a jest zawsze liniowa, od FogStart*ZFar do ZFar

	bool m_Fog_Enabled;
	float m_Fog_Start; // Odleg³oœæ pocz¹tku mg³y, w procentach ZFar
	COLORF m_Fog_Color;

	////// Niebo

	// NULL jeœli brak
	scoped_ptr<BaseSky> m_Sky;

	////// Opady

	// NULL jeœli brak
	scoped_ptr<Fall> m_Fall;

	////// Kamery

	CAMERA_SET m_Cameras;
	Camera * m_ActiveCamera;

	////// Encje

	// Drzewo octree wszystkich encji
	scoped_ptr<EntityOctree> m_Octree;
	// Zbiór wszystkich encji
	ENTITY_SET m_AllEntities;
	// Encje g³ównego poziomu
	ENTITY_SET m_RootEntities;
	
	////// Œwiat³a

	// Co najwy¿ej jedno. Jeœli brak - NULL.
	DirectionalLight *m_DirectionalLight;
	std::list<SpotLight*> m_SpotLights;
	std::list<PointLight*> m_PointLights;

	////// Postprocessing

	scoped_ptr<PpColor> m_PpColor;
	scoped_ptr<PpTexture> m_PpTexture;
	scoped_ptr<PpFunction> m_PpFunction;
	scoped_ptr<PpToneMapping> m_PpToneMapping;
	scoped_ptr<PpBloom> m_PpBloom;
	scoped_ptr<PpFeedback> m_PpFeedback;
	scoped_ptr<PpLensFlare> m_PpLensFlare;

	void DoShadowMapping_Spot(SpotLight &spot_light, float ScreenSizePercent, res::D3dTextureSurface **OutShadowMap, MATRIX *OutShadowMapMatrix, bool OptimizeSetting_MaterialSort);
	void DoShadowMapping_Point(PointLight &point_light, float ScreenSizePercent, res::D3dCubeTextureSurface **OutShadowMap, MATRIX *OutShadowMapMatrix, bool OptimizeSetting_MaterialSort);
	void DoShadowMapping_Directional(DirectionalLight &directional_light, res::D3dTextureSurface **OutShadowMap, MATRIX *OutShadowMapMatrix, bool OptimizeSetting_MaterialSort);
	void CreateDrawData(DRAW_DATA *OutDrawData, const RunningOptimizer::SETTINGS &OptimizerSettings);
	void DrawHeatEntities(const DRAW_DATA &DrawData);
	void DrawFullscreenQuad(uint4 ScreenCX, uint4 ScreenCY, bool FillPerturbationTex);
	void DrawMain();
	// Wykonuje Occlusion Query dla encji w MaterialEntities, CustomEntities, HeatEntities,
	// Te które nie s¹ widoczne usuwa z wektorów.
	void OcclusionQuery_Entities(DRAW_DATA &DrawData);
	// Wykonuje Occlusion Query dla zasiêgów œwiate³ z SpotLights i PointLights.
	// Te które nie s¹ widoczne usuwa z wektorów.
	void OcclusionQuery_Lights(DRAW_DATA &DrawData);
	void BuildEntityFragments(DRAW_DATA &DrawData);
	void DrawAll(DRAW_DATA &DrawData, STATS *OutStats, const RunningOptimizer::SETTINGS &OptimizerSettings);
	float QuerySunVisibleFactor(const ParamsCamera &Cam, const VEC3 &SunDir);
	void SortEntityVector_FarToNear(ENTITY_VECTOR *Vec, const VEC3 &EyePos);
	void SortEntityVector_NearToFar(ENTITY_VECTOR *Vec, const VEC3 &EyePos);
};

class EngineServices;

class Engine
{
public:
	typedef SCENE_SET::iterator SceneIterator;

	Engine();
	~Engine();

	// ======== Sceny ========

	// Scena aktywna mo¿e byæ NULL - wtedy nic siê nie rysuje.
	Scene * GetActiveScene() { return m_ActiveScene; }
	void SetActiveScene(Scene *s) { m_ActiveScene = s; }
	uint GetSceneCount() { return m_Scenes.size(); }
	SceneIterator GetSceneBegin() { return m_Scenes.begin(); }
	SceneIterator GetSceneEnd() { return m_Scenes.end(); }

	// ======== Konfiguracja ========

	enum OPTION
	{
		////// Czy jest oœwietlenie
		O_LIGHTING, // : bool
		////// Czy s¹ cienie (Shadow Mapping)
		O_SM_ENABLED, // : bool
		////// Przesuniêcie do Shadow Mappingu
		O_SM_EPSILON, // : float
		////// Czy jest Post-Processing
		O_PP_ENABLED, // : bool
		O_SM_MAX_SIZE, // : uint, read-only
		O_SM_MAX_CUBE_SIZE, // : uint, read-only
		////// Trawa
		O_GRASS_ZFAR,
	};

	bool  GetConfigBool (OPTION Option);
	float GetConfigFloat(OPTION Option);
	uint  GetConfigUint (OPTION Option);
	void SetConfigBool (OPTION Option, bool v);
	void SetConfigFloat(OPTION Option, float v);
	D3DFORMAT GetShadowMapFormat() { return m_SM_Format; }

	// ======== G³ówne ========

	void OnLostDevice();
	void OnResetDevice();

	// Zwraca ³añcuch z danymi kontrolnymi pracy silnika
	void GetInfo(string *Out);

	// Wywo³ywaæ co klatkê w celu narysowania
	void Draw();
	// Rysuje tekstury pomocnicze - Debug
	// Sam sobie ustawi wszelkie stany
	void DrawSpecialTextures();
	// Wywo³ywaæ co klatkê w celu obliczeñ
	void Update();

	// ======== Tylko dla klasy Scene ========
	void RegisterScene(Scene *s);
	void UnregisterScene(Scene *s);
	EngineServices * GetServices() { return m_Services.get(); }

private:
	// Zapamiêtane capsy pobrane z bie¿¹cego urz¹dzenia D3D
	D3DCAPS9 m_DeviceCaps;
	bool m_Destroying;
	SCENE_SET m_Scenes;
	Scene *m_ActiveScene; // Mo¿e byæ NULL
	scoped_ptr<EngineServices> m_Services;

	bool m_Lighting;
	bool m_SM_Enabled;
	float m_SM_Epsilon;
	bool m_PP_Enabled;
	uint m_SM_MaxSize;
	uint m_SM_MaxCubeSize;
	D3DFORMAT m_SM_Format;
	float m_Grass_ZFar;
	STATS m_Stats;

	void LoadConfig();
	// Wywo³ywaæ po utworzeniu urz¹dzenia
	// Sprawdza mo¿liwoœci karty graficznej, jeœli nie spe³niaj¹ minimum to rzuca wyj¹tek.
	void ValidateDeviceCaps();
};

extern Engine *g_Engine;

} // namespace engine

#endif
