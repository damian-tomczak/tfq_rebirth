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
// Fucking C++!!! - musz� to tutaj wywali� do nag��wka.
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
	// Test fizyczny (np. strza�)
	COLLISION_PHYSICAL = 0x01,
	// Test optyczny (promie� �wiat�a)
	// Obiekty kt�re maj� Visible==false nie robi� nigdy kolizji w tym trybie.
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
		BLEND_ADD,  // Mieszanie dodaj�ce
		BLEND_SUB,  // Mieszanie odejmuj�ce
	};

	BaseMaterial(Scene *OwnerScene, const string &Name);
	virtual ~BaseMaterial();

	Scene * GetOwnerScene() { return m_OwnerScene; }
	virtual TYPE GetType() = 0;
	// Zwraca true, je�li ten materia� koliduje z podanym typem kolizji
	bool CheckCollisionType(uint CollisionType) { return (m_CollisionType & CollisionType) != 0; }

	const string & GetName() { return m_Name; }
	bool GetTwoSided() { return m_TwoSided; }
	uint GetCollisionType() { return m_CollisionType; }

	void SetTwoSided(bool TwoSided) { m_TwoSided = TwoSided; }
	void SetCollisionType(uint CollisionType) { m_CollisionType = CollisionType; }

private:
	Scene *m_OwnerScene;
	string m_Name;
	// Je�li true, wy��cza Backface Culling
	bool m_TwoSided;
	// Typ kolizji - flaga bitowa dla sta�ych COLLISION_TYPE
	uint m_CollisionType;
};

// Materia� w trybie szkieletowym
class WireFrameMaterial : public BaseMaterial
{
public:
	enum COLOR_MODE
	{
		COLOR_MATERIAL, // Kolor i kana� Alfa brany tylko z materia�u
		COLOR_ENTITY,   // Kolor i kana� Alfa brany tylko z Team Color encji
		COLOR_MODULATE, // Kolor i kana� Alfa brany z materia�u RAZY z Team Color encji
	};

	// Tworzy zainicjalizowany warto�ciami domy�lnymi
	WireFrameMaterial(Scene *OwnerScene, const string &Name);
	// Tworzy zainicjalizowany warto�ciami domy�lnymi i podanymi
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
		COLOR_MATERIAL, // Kolor brany w ca�o�ci z materia�u
		COLOR_MODULATE, // Kolor brany z materia�u RAZY z Team Color encji
		COLOR_LERP_ALPHA, // Kolor brany z materia�u lub z encji, zaleznie od warto�ci alfa na teksturze/kolorze Diffuse (interpolacja)
	};

	// Tworzy zainicjalizowany warto�ciami domy�lnymi
	SolidMaterial(Scene *OwnerScene, const string &Name);
	// Tworzy zainicjalizowany warto�ciami domy�lnymi i podanymi
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

	// Zwraca zas�b danej tekstury, LOADED. Je�li nie u�ywa danej tekstury, zwraca NULL.
	res::D3dTexture * GetDiffuseTexture();
	res::D3dTexture * GetEmissiveTexture();
	res::D3dCubeTexture * GetEnvironmentalTexture();

private:
	// Nazwa zasobu tekstury g��wnej z kolorem.
	// Je�li pusty, wtedy nie u�ywa tekstury, u�ywa koloru.
	string m_DiffuseTextureName;
	// Kolor g��wny jednolity materia�u.
	// Wa�ny tylko kiedy m_DiffuseTexture pusty.
	COLORF m_DiffuseColor;
	// Dodatkowa tekstura emisji
	// Je�li pusty, nie ma tekstury Emissive.
	string m_EmissiveTextureName;
	// Dodatkowa tekstura do mapowania �rodowiskowego. Mo�e by� pusty.
	string m_EnvironmentalTextureName;
	// Czy tekstura ma by� animowana
	// Wsp�rz�dne tekstury mno�one razy macierz tekstury pami�tana dla encji.
	bool m_TextureAnimation;
	COLOR_MODE m_ColorMode;
	// Kolor pod�wietlenia Fresnel Term.
	// W nim te� zakodowana jest jasno�� efektu.
	// Czarny oznacza Fresnel Term wy��czony.
	COLORF m_FresnelColor;
	// Wyk�adnik - intensywno�� efektu.
	// Domy�lnie jest 1. Warto�� 0..1 powoduje wi�ksze pokrycie modelu. Warto�� 0..kilka powoduje pod�wietlenie tylko na brzegach.
	float m_FresnelPower;

	res::D3dTexture *m_DiffuseTexture;
	res::D3dTexture *m_EmissiveTexture;
	res::D3dCubeTexture *m_EnvironmentalTexture;
};

// Materia� p�przezroczysty
class TranslucentMaterial : public SolidMaterial
{
public:
	enum ALPHA_MODE
	{
		ALPHA_MATERIAL, // Kana� Alfa brany tylko z materia�u
		ALPHA_ENTITY,   // Kana� Alfa brany tylko z Team Color encji
		ALPHA_MODULATE, // Kana� Alfa brany z materia�u RAZY z Team Color encji
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

// Materia� nieprzezroczysty
class OpaqueMaterial : public SolidMaterial
{
public:
	enum SPECULAR_MODE
	{
		// Brak speculara (domy�lnie)
		SPECULAR_NONE,
		// Zwyk�y specular
		SPECULAR_NORMAL,
		// Anisotropic Lighting brany z wektora Tangent
		// (Dzia�a tylko kiedy PerPixelLighting == true && NormalTextureName == "")
		SPECULAR_ANISO_TANGENT,
		// Anisotropic Lighting brany z wektora Binormal
		// (Dzia�a tylko kiedy PerPixelLighting == true && NormalTextureName == "")
		SPECULAR_ANISO_BINORMAL,
	};

	// Tworzy zainicjalizowany warto�ciami domy�lnymi
	OpaqueMaterial(Scene *OwnerScene, const string &Name);
	// Tworzy zainicjalizowany warto�ciami domy�lnymi i podanymi
	OpaqueMaterial(Scene *OwnerScene, const string &Name, const string &DiffuseTextureName, COLOR_MODE ColorMode = COLOR_MATERIAL);
	virtual ~OpaqueMaterial() { }

	virtual TYPE GetType() { return TYPE_OPAQUE; }

	uint1 GetAlphaTesting() { return m_AlphaTesting; }
	bool GetHalfLambert() { return m_HalfLambert; }
	bool GetPerPixel() { return m_PerPixel; }
	const string & GetNormalTextureName() { return m_NormalTextureName; }
	// Zwraca zas�b tekstury, LOADED. Je�li nie u�ywa takiej tekstury, zwraca NULL.
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
	// Alfa-testing - piksel jest widoczny je�li kana� alfa tekstury Diffuse jest wi�kszy lub r�wny podanej warto�ci
	// Brak alfa testingu to 0 (domy�lnie).
	uint1 m_AlphaTesting;
	bool m_HalfLambert;
	bool m_PerPixel;
	string m_NormalTextureName;
	res::D3dTexture *m_NormalTexture;
	// Tryb speculara (patrz opis enuma)
	SPECULAR_MODE m_SpecularMode;
	// Kolor speculara
	// - COLORF::WHITE = pe�ny kolor �wiat�a (domy�lnie)
	// (SpecularMode != SM_NONE)
	COLORF m_SpecularColor;
	// Wyk�adnik speculara
	// (SpecularMode != SM_NONE)
	float m_SpecularPower;
	// Czy specular ma by� mno�ony przez kana� alfa tekstury Diffuse
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
	// Zwraca materia� o podanej nazwie lub NULL je�li nie znajdzie.
	BaseMaterial * GetByName(const string &Name);
	// Zwraca materia� o podanej nazwie. Je�li nie znajdzie, rzuca wyj�tek.
	BaseMaterial * MustGetByName(const string &Name);
	// Zwraca materia� podanego typu o podanej nazwie lub NULL je�li nie znajdzie albo je�li jest innego typu
	template <typename T> T * GetByNameEx(const string &Name)
	{
		BaseMaterial *M = GetByName(Name);
		if (M == NULL) return NULL;
		return dynamic_cast<T*>(M);
	}
	// Zwraca materia� podanego typu o podanej nazwie. Je�li nie znajdzie lub nie jest tego typu, rzuca wyj�tek.
	template <typename T> T * MustGetByNameEx(const string &Name)
	{
		BaseMaterial *M = MustGetByName(Name);
		if (typeid(*M) != typeid(T)) throw Error(Format("engine::MaterialCollection::MustGetByNameEx: B��dny typ materia�u. Nazwa=#, oczekiwany=#, znaleziony=#") % Name % typeid(T).name() % typeid(*M).name(), __FILE__, __LINE__);
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

// Abstrakcyjna klasa bazowa wszystkich �wiate�.
class BaseLight
{
public:
	// Tworzy zainicjalizowany warto�ciami domy�lnymi
	BaseLight(Scene *OwnerScene);
	// Tworzy zainicjalizowany warto�ciami domy�lnymi i podanymi
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
	// Wsp�czynnik �ciemniania - jaki procent jasno�ci ma mie� obszar w cieniu (domy�lnie: 0)
	float m_ShadowFactor;
};

class DirectionalLight : public BaseLight
{
public:
	// Tworzy zainicjalizowany warto�ciami domy�lnymi
	DirectionalLight(Scene *OwnerScene);
	// Tworzy zainicjalizowany warto�ciami domy�lnymi i podanymi
	// Dir ma by� znormalizowany
	DirectionalLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Dir);
	virtual ~DirectionalLight();

	const VEC3 & GetDir() { return m_Dir; }
	float GetZFar() { return m_ZFar; }

	// Dir ma by� znormalizowany
	void SetDir(const VEC3 &Dir) { m_Dir = Dir; }
	void SetZFar(float ZFar) { m_ZFar = ZFar; }

private:
	VEC3 m_Dir; // Znormalizowany
	// Mo�e by� dowolnie du�e, np. MAXFLOAT.
	// Je�li mniejsze od ZFar kamery, ogranicza zasi�g shadow mapowania tego �wiat�a.
	float m_ZFar;
};

// Abstrakcyjna klasa bazowa �wiate� posiadaj�cych pozycj�
class PositionedLight : public BaseLight
{
public:
	// Tworzy zainicjalizowany warto�ciami domy�lnymi
	PositionedLight(Scene *OwnerScene);
	// Tworzy zainicjalizowany warto�ciami domy�lnymi i podanymi
	PositionedLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Pos);

	const VEC3 & GetPos() { return m_Pos; }
	void SetPos(const VEC3 &Pos) { if (Pos != m_Pos) { m_Pos = Pos; OnPosChange(); } }

	// Zwraca prostok�t dla Scissor Test dla obszaru widocznego na ekranie o�wietlonego zasi�giem tego �wiat�a.
	// Dodatkowo, zwraca procent ekranu obj�ty tym testem, 0..1.
	float GetScissorRect(RECT *OutRect, const MATRIX &View, const MATRIX &Proj, uint RenderTargetCX, uint RenderTargetCY);

protected:
	// ======== Dla klas potomnych ========
	virtual void OnPosChange() = 0;
	// Ma zwr�ci� tablic� 8 wierzcho�k�w dowolnie zorientowanego prostopad�o�cianu otaczaj�cego bry�� zasi�gu �wiat�a
	virtual const VEC3 * GetBoundingBox() = 0;

private:
	VEC3 m_Pos;
};

class SpotLight : public PositionedLight
{
public:
	// Tworzy zainicjalizowany warto�ciami domy�lnymi
	SpotLight(Scene *OwnerScene);
	// Tworzy zainicjalizowany warto�ciami domy�lnymi i podanymi
	// Dir ma by� znormalizowany
	SpotLight(Scene *OwnerScene, const COLORF &Color, const VEC3 &Pos, const VEC3 &Dir, float Dist, float Fov);
	virtual ~SpotLight();

	const VEC3 & GetDir() { return m_Dir; }
	float GetDist() { return m_Dist; }
	float GetFov() { return m_Fov; }
	bool GetSmooth() { return m_Smooth; }
	float GetZNear() { return m_ZNear; }

	// Dir ma by� znormalizowany
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
	float m_Dist; // Zasi�g
	float m_Fov; // K�t patrzenia w radianach
	bool m_Smooth; // true, je�li zanikanie wraz z k�tem wyg�adzone, false je�li ostra okr�g�a kraw�d�
	float m_ZNear; // U�ywane do Shadow Mappingu

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
Spami�tuje i uniewa�nia osobno Shadow Mapy dla ka�dej z 6 �cian.
*/
class PointLight : public PositionedLight
{
public:
	// Tworzy zainicjalizowany warto�ciami domy�lnymi
	PointLight(Scene *OwnerScene);
	// Tworzy zainicjalizowany warto�ciami domy�lnymi i podanymi
	// Dir ma by� znormalizowany
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
	float m_Dist; // Zasi�g
	float m_ZNear; // U�ywane do Shadow Mappingu
	ParamsCamera m_ParamsCameras[6]; bool m_ParamsCamerasIs;
	VEC3 m_BoundingBox[8]; bool m_BoundingBoxIs;

	void EnsureBoundingBox();
	void EnsureParamsCameras();
};

// Abstracyjna klasa bazowa dla wszelkich obiekt�w graficznych silnika.
/*
- Encje podczas tworzenia same dodaj� si� do sceny.
- Encje mo�na usuwa� r�cznie (zwyk�e delete), w�wczas same usuwaj� si� ze sceny, lub mo�na ich
  nie usuwa� i wtedy usuwa je scena podczas swojej destrukcji.
- Sfer� otaczaj�c� encj� otrzymuje si� z GetWorldPos i GetWorldRadius. Jest to szybkie,
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

	// Tworzy encj� i dodaje j� do sceny
	Entity(Scene *OwnerScene);
	// Usuwa encj� i wyrejestrowuje j� ze sceny
	virtual ~Entity();

	// ======== W�a�ciciel, nad-encja i pod-encje ========
	Scene * GetOwnerScene() { return m_OwnerScene; }
	Entity * GetParent() { return m_Parent; }
	uint GetSubEntitiesCount() { return m_SubEntities.size(); }
	EntityIterator GetSubEntitiesBegin() { return m_SubEntities.begin(); }
	EntityIterator GetSubEntitiesEnd() { return m_SubEntities.end(); }
	// Zmienia encj� nadrz�dn�
	// Parent - mo�na poda� NULL, w�wczas encja staje si� g��wnego poziomu
	void SetParent(Entity *Parent);
	// Zmienia encj� nadrz�dn�, pod��cza t� encj� do konkretnej ko�ci encji nadrz�dnej
	void SetParentBone(Entity *Parent, const string &ParentBone);

	// Pozycja �rodka encji
	const VEC3 & GetPos() { return m_Pos; }
	void SetPos(const VEC3 &Pos) { m_Pos = Pos; OnTransformChange(); }
	void ChangePos(const VEC3 &PosDiff) { m_Pos += PosDiff; OnTransformChange(); }
	void ResetPos() { m_Pos = VEC3::ZERO; OnTransformChange(); }
	// Orientacja (obr�t)
	const QUATERNION & GetOrientation() { return m_Orientation; }
	void SetOrientation(const QUATERNION &Orientation) { m_Orientation = Orientation; m_IsOrientation = (m_Orientation != QUATERNION::IDENTITY); OnTransformChange(); }
	void ResetOriantation() { m_Orientation = QUATERNION::IDENTITY; m_IsOrientation = false; OnTransformChange(); }
	// Skalowanie
	float GetSize() { return m_Size; }
	void SetSize(float Size) { m_Size = Size; OnTransformChange(); }
	// Promie� sfery otaczaj�cej
	float GetRadius() { return m_Radius; }
	void SetRadius(float Radius) { m_Radius = Radius; OnTransformChange(); }
	// Widoczno�� obiektu
	bool GetVisible() { return m_Visible; }
	void SetVisible(bool Visible) { m_Visible = Visible; m_WorldVisibleCalculated = false; }
	// Tag - flaga bitowa do u�ytku w�asnego
	uint4 GetTag() { return m_Tag; }
	void SetTag(uint4 Tag) { m_Tag = Tag; }

	// Zwraca macierz lokaln� tego obiektu
	const MATRIX & GetLocalMatrix() { EnsureLocalMatrix(); return m_LocalMatrix; }
	// Zwraca odwrotno�� macierzy lokalnej tego obiektu
	const MATRIX & GetInvLocalMatrix() { EnsureInvLocalMatrix(); return m_InvLocalMatrix; }
	// Zwraca macierz �wiata tego obiektu
	const MATRIX & GetWorldMatrix() { EnsureWorldMatrix(); return m_WorldMatrix; }
	// Zwraca odwrotno�� macierzy �wiata tego obiektu
	const MATRIX & GetInvWorldMatrix() { EnsureInvWorldMatrix(); return m_InvWorldMatrix; }
	// Zwraca pozycj� �rodka encji we wsp. globalnych �wiata
	const VEC3 & GetWorldPos() { EnsureWorldPos(); return m_WorldPos; }
	// Zwraca skalowanie we wsp. globalnych �wiata
	float GetWorldSize() { EnsureWorldSize(); return m_WorldSize; }
	// Zwraca promie� sfery otaczaj�cej we wsp. globalnych �wiata
	float GetWorldRadius() { EnsureWorldRadius(); return m_WorldRadius; }
	// Zwraca czy obiekt jest bezwzgl�dnie widoczny, tzn. jest widoczny on i jego podencje
	bool GetWorldVisible() { EnsureWorldVisible(); return m_WorldVisible; }

	// ======== Dla klas pochodnych ========
	virtual TYPE GetType() = 0;
	// Kolizja promienia o parametrach podanych we wsp. lokalnych modelu
	virtual bool RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT) = 0;
	// Ma zwr�ci� macierz ko�ci o podanej nazwie
	// Tzn. przekszta�cenie ze wsp. lokalnych tego modelu w Bind Pose do te� wsp. lokalnych ale w bie��cej pozycji
	// Je�li takiej ko�ci nie ma, ma zwr�ci� MATRIX::IDENTITY.
	virtual const MATRIX & GetBoneMatrix(const string &BoneName) { return MATRIX::IDENTITY; }
	// Ma si� policzy�
	virtual void Update() { }

	// ======== Dla innych obiekt�w klasy Entity ========
	void RegisterSubEntity(Entity *SubEntity);
	void UnregisterSubEntity(Entity *SubEntity);

	// ======== Dla klasy EntityOctree ========
	ENTITY_OCTREE_NODE * GetOctreeNode() { return m_OctreeNode; }
	void SetOctreeNode(ENTITY_OCTREE_NODE *Node) { m_OctreeNode = Node; }

private:
	bool m_Destroying;
	Scene *m_OwnerScene;
	ENTITY_OCTREE_NODE *m_OctreeNode;
	Entity *m_Parent; // NULL je�li to encja poziomu g��wnego
	string m_ParentBone; // Nazwa ko�ci obiektu nadrz�dnego, do kt�rej podczepiona jest ta encja. �a�cuch pusty je�li brak.
	ENTITY_SET m_SubEntities;
	VEC3 m_Pos;
	QUATERNION m_Orientation; // K�ty Eulera
	bool m_IsOrientation; // true je�li orientacja jest niezerowa lub niewiadomo jaka
	float m_Size;
	float m_Radius;
	// Macierz przekszta�caj�ca ze wsp. tego obiektu do wsp. obiektu nadrz�dnego
	MATRIX m_LocalMatrix; bool m_LocalMatrixCalculated;
	MATRIX m_InvLocalMatrix; bool m_InvLocalMatrixCalculated;
	// Macierz przekszta�caj�ca ze wsp. tego obiektu do wsp. globalnych �wiata
	MATRIX m_WorldMatrix; bool m_WorldMatrixCalculated;
	MATRIX m_InvWorldMatrix; bool m_InvWorldMatrixCalculated;
	// Pozycja �rodka we wsp. globalnych �wiata
	VEC3 m_WorldPos; bool m_WorldPosCalculated;
	// Zebrane skalowanie we wsp. �wiata
	float m_WorldSize; bool m_WorldSizeCalculated;
	float m_WorldRadius; bool m_WorldRadiusCalculated;
	// Zebrana widoczno�� (Visible jest dziedziczone na pod-encje)
	bool m_WorldVisible; bool m_WorldVisibleCalculated;
	bool m_Visible;
	uint4 m_Tag;

	// Kt�ry� z parametr�w transformacji si� zmieni� - wyliczone dane s� ju� nieaktualne
	void OnTransformChange();
	// Wylicza w razie potrzeby dane dane tak, �eby na pewno by�y wyliczone.
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
	// Wska�nik do encji
	Entity *E;
	// Identyfikator fragmentu do u�ytku w�asnego encji
	uint FragmentId;
	// Wska�nik do materia�u lub NULL je�li to CustomEntity
	BaseMaterial *M;
	// WorldPos encji
	VEC3 Pos;

	ENTITY_FRAGMENT(Entity *E, uint FragmentId, BaseMaterial *M, const VEC3 &Pos) : E(E), FragmentId(FragmentId), M(M), Pos(Pos) { }
};

/*
- Tylko encje tego typu mog� podlega� o�wietleniu albo rzuca� cie�.
- Team Color to kolor pami�tany dla poszczeg�lnych encji.
  Nie jest dziedziczony do encji podrz�dnych.
  Mo�e wp�ywa� na kolor rysowanego obiektu, zale�nie od ustawie� materia�u.
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
	// Stany ju� ma ustawione. Jej zadaniem jest tylko wykona� SetStreamSource, SetIndices, SetFVF, Draw[Indexed]Primitive[UP] oraz frame::RegisterDrawCall.
	virtual void DrawFragmentGeometry(uint Id, BaseMaterial *Mat, const ParamsCamera &Cam) = 0;
	// Zwraca bie��ce dane do Skinningu encji.
	// Je�li zwr�ci false, nie stosuje Skinningu.
	// Je�li zwr�ci true, stosuje Skinning. Ma wtedy zwr�ci� BoneCount i BoneMatrices.
	// OutBoneMatrices to wska�nik do tablicy - ma zwr�ci� wska�nik do swojej wewn�trznej struktury danych z tymi macierzami.
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
	// - Trzy promienie w trzech r�nych kierunkach.
	// - Intensity to intensywno�� efektu, 0..1.
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

	// Por�wnywanie, �eby sortowa� grupuj�c wg obiekt�w klasy Tree.
	bool operator < (const TREE_DRAW_DESC &desc2) { return (tree < desc2.tree); }
};

typedef std::vector<TREE_DRAW_DESC> TREE_DRAW_DESC_VECTOR;

/*
W chwili tworzenia i przez ca�y czas istnienia obiektu tej klasy istnie� musi
podany do konstruktora teren oraz tekstury u�ywane przez ten teren.
*/
class TerrainRenderer
{
public:
	// TreeDescFileName - opcjonalny, mo�e by� pusty �a�cuch. Opis formatu: plik Trees.txt.
	// TreeDensityMapFileName - opcjonalny
	// GrassObj, WaterObj - opcjonalny, mo�e by� NULL.
	//   Je�li podane, przechodz� na w�asno�� TerrainRenderer i on je zwolni.
	TerrainRenderer(Scene *OwnerScene, Terrain *terrain, Grass *GrassObj, TerrainWater *WaterObj,
		const string &TreeDescFileName, const string &TreeDensityMapFileName);
	~TerrainRenderer();

	Terrain * GetTerrain() { return m_Terrain; }

	Tree & GetTreeByIndex(uint Index) { assert(Index < m_Trees.size()); return *m_Trees[Index].Tree_.get(); }
	// Je�li nie znajdzie, zwraca NULL.
	Tree * GetTreeByName(const string &Name);
	// Je�li nie znajdzie, rzuca wyj�tek.
	Tree & MustGetTreeByName(const string &Name);
	// Dodaje do wektora deskryptory drzew zawartych we frustumie podanej kamery
	void GetTreesInFrustum(TREE_DRAW_DESC_VECTOR *InOut, const ParamsCamera &Cam, bool FrustumCulling);
	void GetTreesCastingDirectionalShadow(TREE_DRAW_DESC_VECTOR *InOut, const FRUSTUM_PLANES &CamFrustum, const BOX &CamBox, const VEC3 &LightDir);

	////// Dla klasy Scene

	void DrawPatch(PASS Pass, uint PatchIndex, const SCENE_DRAW_PARAMS &SceneDrawParams, const ParamsCamera *Cam, BaseLight *Light);
	void DrawGrass(const ParamsCamera &Cam, const COLORF &Color);
	void DrawWater(const ParamsCamera &Cam);

private:
	// Opis gatunku drzewa - dodatkowe informacje towarzysz�ce obiektowi Tree.
	// Opr�cz nazwy, bo jest trzymana na zewn�trz, w mapie m_TreeNames.
	struct TREE_DATA
	{
		shared_ptr<Tree> Tree_;
		uint KindCount;
		uint Index; // Indeks tego elementu w tablicy m_Trees.
		COLOR DensityMapColor; // A=0 oznacza brak reprezentacji drzewa jako koloru, A=255 oznacza, �e jest.

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
	scoped_ptr<Grass> m_GrassObj; // NULL, je�li nie ma
	scoped_ptr<TerrainWater> m_WaterObj; // NULL, je�li nie ma
	scoped_ptr<TerrainMaterial> m_Material;

	////// Drzewa
	// Definicje drzew
	std::vector<TREE_DATA> m_Trees;
	// Mapa odwzorowuj�ca nazwy drzew (tych kt�re maj� nazwy) na ich indeksy w tablicy m_Trees
	std::map<string, uint> m_TreeNames;
	// Tree Density Map - indeksy rodzaj�w drzew z m_Trees na poszczeg�lnych polach
	// Je�li tablica jest pusta, i CX i CY = 0, nie ma TreeDensityMap
	// Element w tablicy r�wny 0xFF oznacza brak drzewa na danym polu.
	uint m_TreeDensityMapCX;
	uint m_TreeDensityMapCY;
	// Liczba patch�w na X i na Z
	uint m_TreePatchesX;
	uint m_TreePatchesZ;
	std::vector<uint1> m_TreeDensityMap;
	// Cache. Ostatnio u�ywane s� na ko�cu.
	std::vector< shared_ptr<PATCH> > m_PatchCache;

	void LoadTreeDescFile(const string &TreeDescFileName);
	void LoadTreeDensityMap(const string &FileName);
	const PATCH & EnsurePatch(uint px, uint pz);
	// Patch ma wype�niony px i pz, funkcja ma za zadanie wygenerowa� TreeDescs
	void GeneratePatch(PATCH *P);
};

// Bufor pami�taj�cy okre�lon� liczb� ostatnich pr�bek.
// Daje dost�p do ich sumy, iloczynu i �redniej.
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

// KLASA U�YWANA WEWN�TRZNIE
class RunningOptimizer
{
public:
	enum SETTING
	{
		// Czy sortowa� fragmenty wg materia�u
		SETTING_MATERIAL_SORT,
		// Czy wykonywa� Occlusion Query dla encji
		SETTING_ENTITY_OCCLUSION_QUERY,
		// Czy wykonywa� Occlusion Query dla �wiate�
		SETTING_LIGHT_OCCLUSION_QUERY,
		// Czy wykonywa� frustum culling dla drzew rysowanych na terenie
		SETTING_TREE_FRUSTUM_CULLING,
		// (Liczba ustawie�)
		SETTING_COUNT
	};

	typedef std::bitset<SETTING_COUNT> SETTINGS;

	RunningOptimizer();

	// Wywo�ywa� na pocz�tku klatki. Robi dwie rzeczy:
	// 1. Zwraca ustawienia do u�ycia w bie��cej klatce
	// 2. Odbiera i analizuje czas jaki trwa�a poprzednia klatka
	void OnFrame(SETTINGS *OutNewSettings, float LastFrameTime);

	static void SettingsToStr(string *Out, const SETTINGS &Settings);

private:
	uint m_FrameNumber;
	SETTINGS m_CurrentSettings;
	// Indeks opcji, kt�ra ma by� teraz lub jest testowana na zmian�
	uint m_SettingTestIndex;
	// �redni czas klatek dla ustawie� normalnych
	SampleBuffer<float> m_NormalFrameTimeSamples;
	// Dla ka�dego ustawienia pami�ta sum� warto�ci 1 lub -1 zale�nie czy si� op�aca, czy nie op�aca zmienia� na przeciwne
	int m_FlipSums[SETTING_COUNT];
	// Dla ka�dego ustawienia pami�ta liczb� pr�bek dla warto�ci przeciwnej
	uint m_FlipCounts[SETTING_COUNT];
};

// Scena - przechowuje kolekcj� encji i organizuje ich rysowanie
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

	// ======== Mg�a ========

	bool GetFogEnabled() { return m_Fog_Enabled; }
	float GetFogStart() { return m_Fog_Start; }
	const COLORF & GetFogColor() { return m_Fog_Color; }
	void SetFogEnabled(bool FogEnabled) { m_Fog_Enabled = FogEnabled; }
	void SetFogStart(float FogStart) { m_Fog_Start = FogStart; }
	void SetFogColor(const COLORF &FogColor) { m_Fog_Color = FogColor; }

	// ======== Niebo ========

	// Przekazany obiekt przechodzi na w�asno�� sceny - ona go usunie.
	void SetSky(BaseSky *Sky);
	BaseSky * GetSky();
	void ResetSky();

	// ======== Opady ========

	// Przekazany obiekt przechodzi na w�asno�� sceny - ona go usunie.
	void SetFall(Fall *a_Fall);
	Fall * GetFall();
	void ResetFall();

	// ======== Mapa ========
	// W ka�dej chwili w scenie mo�e by� 0 lub 1 mapa.

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
	// W ka�dej chwili w scenie mo�e by� 0 lub 1 teren.

	// Obiekt przechodzi na w�asno�� sceny i zostanie przez ni� zwolniony.
	void SetTerrain(TerrainRenderer *terrain_renderer);
	void ResetTerrain();
	TerrainRenderer * GetTerrain() { return m_TerrainRenderer.get(); }

	// ======== Kamery ========
	// Kamery tworzy si� i usuwa po prostu new Camera() i delete MyCam, podaj�c do konstruktora wska�nik do sceny.

	// Ustawia kamer� aktywn�.
	// Mo�na poda� NULL - wtedy scena nie jest renderowana.
	// Podana kamera musi nale�e� do tej sceny.
	void SetActiveCamera(Camera *cam);
	Camera * GetActiveCamera() { return m_ActiveCamera; }
	uint GetCameraCount() { return m_Cameras.size(); }
	CameraIterator GetCameraBegin() { return m_Cameras.begin(); }
	CameraIterator GetCameraEnd() { return m_Cameras.end(); }

	// ======== Encje ========

	// Dost�p do kolekcji encji g��wnego poziomu
	uint GetRootEntityCount() { return m_RootEntities.size(); }
	EntityIterator GetRootEntityBegin() { return m_RootEntities.begin(); }
	EntityIterator GetRootEntityEnd() { return m_RootEntities.end(); }

	// ======== �wiat�a ========
	// �wiat�a tworzy si� i usuwa po prostu new Typ�wiat�a() i delete �wiat�o, podaj�c do konstruktora wska�nik sceny.
	// Na raz mo�e istnie� w scenie co najwy�ej jedno �wiat�o DirectionalLight.

	// ======== Postprocessing ========

	// Ustawiane tu obiekty efekt�w postprocessingu przechodz� na w�asno�� obiektu sceny.

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

	// ======== G��wne =========

	void Update();
	void Draw(STATS *OutStats);
	// OutT ustawiane na sensown� warto�� tylko je�li zwr�ci�a != COLLISION_RESULT_NONE.
	// OutEntity ustawiane na sensown� warto�� tylko je�li zwr�ci�a == COLLISION_RESULT_ENTITY.
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

	// ======== Tylko dla klas �wiate� ========
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
	// Kierunek i d�ugo�� okre�laj� kierunek w osi XZ i si�� (pr�dko��) wiatru
	VEC2 m_WindVec;

	////// Mapa

	scoped_ptr<QMapRenderer> m_QMapRenderer;
	bool m_MapUseLighting;
	bool m_MapCastShadow;
	bool m_MapReceiveShadow;

	////// Teren

	scoped_ptr<TerrainRenderer> m_TerrainRenderer;

	////// Mg�a
	// Mg�a jest zawsze liniowa, od FogStart*ZFar do ZFar

	bool m_Fog_Enabled;
	float m_Fog_Start; // Odleg�o�� pocz�tku mg�y, w procentach ZFar
	COLORF m_Fog_Color;

	////// Niebo

	// NULL je�li brak
	scoped_ptr<BaseSky> m_Sky;

	////// Opady

	// NULL je�li brak
	scoped_ptr<Fall> m_Fall;

	////// Kamery

	CAMERA_SET m_Cameras;
	Camera * m_ActiveCamera;

	////// Encje

	// Drzewo octree wszystkich encji
	scoped_ptr<EntityOctree> m_Octree;
	// Zbi�r wszystkich encji
	ENTITY_SET m_AllEntities;
	// Encje g��wnego poziomu
	ENTITY_SET m_RootEntities;
	
	////// �wiat�a

	// Co najwy�ej jedno. Je�li brak - NULL.
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
	// Te kt�re nie s� widoczne usuwa z wektor�w.
	void OcclusionQuery_Entities(DRAW_DATA &DrawData);
	// Wykonuje Occlusion Query dla zasi�g�w �wiate� z SpotLights i PointLights.
	// Te kt�re nie s� widoczne usuwa z wektor�w.
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

	// Scena aktywna mo�e by� NULL - wtedy nic si� nie rysuje.
	Scene * GetActiveScene() { return m_ActiveScene; }
	void SetActiveScene(Scene *s) { m_ActiveScene = s; }
	uint GetSceneCount() { return m_Scenes.size(); }
	SceneIterator GetSceneBegin() { return m_Scenes.begin(); }
	SceneIterator GetSceneEnd() { return m_Scenes.end(); }

	// ======== Konfiguracja ========

	enum OPTION
	{
		////// Czy jest o�wietlenie
		O_LIGHTING, // : bool
		////// Czy s� cienie (Shadow Mapping)
		O_SM_ENABLED, // : bool
		////// Przesuni�cie do Shadow Mappingu
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

	// ======== G��wne ========

	void OnLostDevice();
	void OnResetDevice();

	// Zwraca �a�cuch z danymi kontrolnymi pracy silnika
	void GetInfo(string *Out);

	// Wywo�ywa� co klatk� w celu narysowania
	void Draw();
	// Rysuje tekstury pomocnicze - Debug
	// Sam sobie ustawi wszelkie stany
	void DrawSpecialTextures();
	// Wywo�ywa� co klatk� w celu oblicze�
	void Update();

	// ======== Tylko dla klasy Scene ========
	void RegisterScene(Scene *s);
	void UnregisterScene(Scene *s);
	EngineServices * GetServices() { return m_Services.get(); }

private:
	// Zapami�tane capsy pobrane z bie��cego urz�dzenia D3D
	D3DCAPS9 m_DeviceCaps;
	bool m_Destroying;
	SCENE_SET m_Scenes;
	Scene *m_ActiveScene; // Mo�e by� NULL
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
	// Wywo�ywa� po utworzeniu urz�dzenia
	// Sprawdza mo�liwo�ci karty graficznej, je�li nie spe�niaj� minimum to rzuca wyj�tek.
	void ValidateDeviceCaps();
};

extern Engine *g_Engine;

} // namespace engine

#endif
