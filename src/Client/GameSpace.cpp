/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\ResMngr.hpp"
#include "..\Engine\Engine.hpp"
#include "..\Engine\Entities.hpp"
#include "..\Engine\Engine_pp.hpp"
#include "..\Engine\Sky.hpp"
#include "..\Engine\Terrain.hpp"
#include "..\Engine\Grass.hpp"
#include "GameSpace.hpp"

using namespace engine;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy globalne

static const float AI_CALC_PERIOD = 0.1f;

static float BEGIN_TIME;
static float WIN_TIME;
static float LOSE_TIME;

static VEC3 LIGHT_DIR;
static COLORF LIGHT_COLOR;
static COLORF AMBIENT_COLOR;
static float CAMERA_ANGLE_X;
static float CAMERA_DIST;
static float CAMERA_SMOOTH_TIME;
static VEC2 WIND_VEC;
static float SHADOW_FACTOR;
static float DAY_TIME;
static float GAMEPLAY_Y;
static uint ROCK_COUNT;
static float ROCK_SIZE, ROCK_SIZE_V;
static float PLAYER_ACCEL;
static float FRICTION_FACTOR;
static float ORIENTATION_SMOOTH_TIME;
static float ORIENTATION_VEL;
static float CAMERA_ABOVE_PLAYER;
static float CAMERA_ANGLE_X_SMOOTH_TIME;
static float MAX_SPEED; // Wyliczane
static float SPEED_MOTION_BLUR_THRESHOLD;
static float MOTION_BLUR_THRESHOLD_TIME;
static float PLAYER_RADIUS; // Promieñ do kolizji pocisków wroga z graczem jako kul¹
static float MISSILE_SPEED; // Prêdkoœæ pocisków wroga
static float EXPLOSION_TIME; // Czas trwania efektu cz¹steczkowego wybuchu
static float BIG_EXPLOSION_TIME; // Czas trwania efektu du¿ego wybuchu
static float ENEMY_SHOT_PERIOD;
static float LASER_DAMAGE_PER_SECOND;
static float ENEMY_MISSILE_DAMAGE;
static float ENEMY_MISSILE_LIFE_TIME;

static engine::Scene *g_Scene;
static engine::Terrain *g_TerrainObj;

typedef std::list< shared_ptr<QuadEntity> > MISSILE_LIST;

PARTICLE_DEF g_ExplosionFireParticleDef;
PARTICLE_DEF g_ExplosionSmokeParticleDef;

void RotationMatrix2D(MATRIX *Out, float A)
{
	float ca = cosf(A), sa = sinf(A);

	Out->_11 =  ca;  Out->_12 = sa;   Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = -sa;  Out->_22 = ca;   Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = 0.0f; Out->_33 = 1.0f; Out->_34 = 0.0f;
	Out->_41 = 0.0f; Out->_42 = 0.0f; Out->_43 = 0.0f; Out->_44 = 1.0f;
}

inline float sign(float x)
{
	if (x > 0.0f) return  1.0f;
	if (x < 0.0f) return -1.0f;
	return 0.0f;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktura EXPLOSION

struct EXPLOSION
{
	shared_ptr<ParticleEntity> FireParticles;
	shared_ptr<ParticleEntity> SmokeParticles;
	float StartTime;

	EXPLOSION(Scene *SceneObj, const VEC3 &Pos);
	bool IsFinished();
};

typedef std::list<EXPLOSION> EXPLOSION_LIST;

EXPLOSION::EXPLOSION(Scene *SceneObj, const VEC3 &Pos)
{
	FireParticles.reset(new ParticleEntity(SceneObj,
		"FireParticle",
		BaseMaterial::BLEND_ADD,
		g_ExplosionFireParticleDef,
		32,
		1.0f));
	FireParticles->SetPos(Pos);

	SmokeParticles.reset(new ParticleEntity(SceneObj,
		"SmokeParticle",
		BaseMaterial::BLEND_SUB,
		g_ExplosionSmokeParticleDef,
		32,
		1.0f));
	SmokeParticles->SetPos(Pos);

	this->StartTime = frame::Timer2.GetTime();
}

bool EXPLOSION::IsFinished()
{
	return (frame::Timer2.GetTime() > this->StartTime + EXPLOSION_TIME);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GameSpace_pimpl

class GameSpace_pimpl
{
public:
	GameSpace_pimpl();
	~GameSpace_pimpl();
	int CalcFrame();
	void OnResolutionChanged();
	void OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	Scene * GetScene() { return m_Scene.get(); }

private:
	Terrain *m_TerrainObj;
	scoped_ptr<Scene> m_Scene;
	// Rozmiar planszy wyra¿ony w jednostkach œwiata
	float m_WorldCX, m_WorldCZ;
	// Zwolni go scena
	Camera *m_Camera;
	// Zwolni go scena
	DirectionalLight *m_Light;
	PARTICLE_DEF m_PlayerParticleDef;
	PARTICLE_DEF m_EnemyParticleDef;
	PARTICLE_DEF m_LaserHitParticleDef;
	PARTICLE_DEF m_LaserHitSmokeParticleDef;
	PARTICLE_DEF m_BigExplosionFireParticleDef;
	PARTICLE_DEF m_BigExplosionSmokeParticleDef;

	////// Bohater
	// > Encja modelu
	QMeshEntity *m_PlayerEntity;
	ParticleEntity *m_PlayerParticles;
	LineStripeEntity *m_PlayerLasers[2];
	ParticleEntity *m_PlayerLaserHitParticles;
	ParticleEntity *m_PlayerLaserHitSmokeParticles;
	// > Parametry prawdziwe
	VEC2 m_PlayerPos;
	float m_PlayerSpeed;
	SmoothCD_obj<float> m_PlayerAngleY;
	// > Parametry wizualne
	SmoothCD_obj<float> m_CameraAngleX;
	// Czas odk¹d prêdkoœæ jest powy¿ej SpeedMotionBlurThreshold
	float m_MotionBlurThresholdTime;
	bool m_MouseButtonDown;
	float m_PlayerLife;

	////// Przeciwnik
	// > Encja modelu
	QMeshEntity *m_EnemyEntity;
	ParticleEntity *m_EnemyParticles[2];
	// > Parametry prawdziwe
	VEC2 m_EnemyPos;
	float m_EnemySpeed;
	SmoothCD_obj<float> m_EnemyAngleY;
	float m_EnemyLife;

	////// Pociski przeciwnika
	// Obecnoœæ i pozycje tych encji to jednoczeœnie dane tych pocisków
	MISSILE_LIST m_EnemyMissiles;
	std::list<float> m_EnemyMissileTimes;
	EXPLOSION_LIST m_Explosions;
	bool m_EnemyKeyUp, m_EnemyKeyDown, m_EnemyKeyRight, m_EnemyKeyLeft, m_EnemyShoot;
	float m_LastAICalcTime;
	float m_LastEnemyShootTime;

	// Bie¿¹cy stan rozgrywki
	enum GAME_STATE
	{
		GAME_STATE_NONE,
		// Istnieje w scenie PpColor
		GAME_STATE_BEGIN,
		// Istnieje w scenie PpFunction
		GAME_STATE_WIN,
		// Istnieje w scenie PpColor
		GAME_STATE_LOSE,
		// Wygrana w toku, odgrywanie ostatecznej eksplozji
		// Istnieje w scenie PpColor, m_UltimateParticleEntity
		GAME_STATE_WIN_PENDING,
		// Przegrana w toku, odgrywanie ostatecznej eksplozji
		// Istnieje w scenie PpColor, m_UltimateParticleEntity
		GAME_STATE_LOSE_PENDING,
	};
	GAME_STATE m_GameState;
	// Czas bie¿acego stanu, dope³nia siê 0..1
	float m_GameStateTime;
	// Efekt cz¹steczkowy ostatecznej wygranej lub przegranej
	// [0] = ogieñ, [1] = dym
	ParticleEntity *m_UltimateParticleEntity[2];

	// U¿ywane przez konstruktor do stworzenia na scenie obiektów ska³
	void CreateMaterials();
	void CreateParticleDefs();
	void CreateRocks();
	void CalcEnemyAI(
		const VEC2 &PlayerDir, const VEC2 &PlayerRight,
		const VEC2 &EnemyDir, const VEC2 &EnemyRight,
		bool *OutKeyUp, bool *OutKeyDown, bool *OutKeyLeft, bool *OutKeyRight, bool *OutShoot);
	float GetCurrentAspectRatio();
	void Win();
	void Lose();
};


GameSpace_pimpl::GameSpace_pimpl() :
	m_GameState(GAME_STATE_BEGIN),
	m_GameStateTime(0.0f),
	m_PlayerEntity(NULL),
	m_PlayerParticles(NULL),
	m_EnemyEntity(NULL),
	m_MouseButtonDown(false),
	m_PlayerLaserHitParticles(NULL),
	m_PlayerLaserHitSmokeParticles(NULL),
	m_EnemyKeyUp(false), m_EnemyKeyDown(false), m_EnemyKeyRight(false), m_EnemyKeyLeft(false), m_EnemyShoot(false),
	m_LastAICalcTime(frame::Timer2.GetTime()),
	m_LastEnemyShootTime(frame::Timer2.GetTime()),
	m_PlayerLife(1.0f), m_EnemyLife(1.0f)
{
	m_EnemyParticles[0] = m_EnemyParticles[1] = NULL;
	m_PlayerLasers[0] = m_PlayerLasers[1] = NULL;
	m_UltimateParticleEntity[0] = m_UltimateParticleEntity[1] = NULL;

	// Wczytaj konfiguracjê

	g_Config->MustGetDataEx("Game/BeginTime", &BEGIN_TIME);
	g_Config->MustGetDataEx("Game/WinTime", &WIN_TIME);
	g_Config->MustGetDataEx("Game/LoseTime", &LOSE_TIME);
	g_Config->MustGetDataEx("Game/Space/LightDir", &LIGHT_DIR);
	g_Config->MustGetDataEx("Game/Space/LightColor", &LIGHT_COLOR);
	g_Config->MustGetDataEx("Game/Space/AmbientColor", &AMBIENT_COLOR);
	g_Config->MustGetDataEx("Game/Space/CameraAngleX", &CAMERA_ANGLE_X);
	g_Config->MustGetDataEx("Game/Space/CameraDist", &CAMERA_DIST);
	g_Config->MustGetDataEx("Game/Space/CameraSmoothTime", &CAMERA_SMOOTH_TIME);
	g_Config->MustGetDataEx("Game/Space/WindVec", &WIND_VEC);
	g_Config->MustGetDataEx("Game/Space/ShadowFactor", &SHADOW_FACTOR);
	g_Config->MustGetDataEx("Game/Space/DayTime", &DAY_TIME);
	g_Config->MustGetDataEx("Game/Space/GameplayY", &GAMEPLAY_Y);
	g_Config->MustGetDataEx("Game/Space/RockCount", &ROCK_COUNT);
	g_Config->MustGetDataEx("Game/Space/RockSize", &ROCK_SIZE);
	g_Config->MustGetDataEx("Game/Space/RockSizeV", &ROCK_SIZE_V);
	g_Config->MustGetDataEx("Game/Space/PlayerAccel", &PLAYER_ACCEL);
	g_Config->MustGetDataEx("Game/Space/FrictionFactor", &FRICTION_FACTOR);
	g_Config->MustGetDataEx("Game/Space/OrientationSmoothTime", &ORIENTATION_SMOOTH_TIME);
	g_Config->MustGetDataEx("Game/Space/OrientationVel", &ORIENTATION_VEL);
	g_Config->MustGetDataEx("Game/Space/CameraAbovePlayer", &CAMERA_ABOVE_PLAYER);
	g_Config->MustGetDataEx("Game/Space/CameraAngleXSmoothTime", &CAMERA_ANGLE_X_SMOOTH_TIME);
	g_Config->MustGetDataEx("Game/Space/SpeedMotionBlurThreshold", &SPEED_MOTION_BLUR_THRESHOLD);
	g_Config->MustGetDataEx("Game/Space/MotionBlurThresholdTime", &MOTION_BLUR_THRESHOLD_TIME);
	g_Config->MustGetDataEx("Game/Space/PlayerRadius", &PLAYER_RADIUS);
	g_Config->MustGetDataEx("Game/Space/MissileSpeed", &MISSILE_SPEED);
	g_Config->MustGetDataEx("Game/Space/ExplosionTime", &EXPLOSION_TIME);
	g_Config->MustGetDataEx("Game/Space/BigExplosionTime", &BIG_EXPLOSION_TIME);
	g_Config->MustGetDataEx("Game/Space/EnemyShotPeriod", &ENEMY_SHOT_PERIOD);
	g_Config->MustGetDataEx("Game/Space/LaserDamagePerSecond", &LASER_DAMAGE_PER_SECOND);
	g_Config->MustGetDataEx("Game/Space/EnemyMissileDamage", &ENEMY_MISSILE_DAMAGE);
	g_Config->MustGetDataEx("Game/Space/EnemyMissileLifeTime", &ENEMY_MISSILE_LIFE_TIME);

	// AABB mapy
	BOX MainBounds;
	m_TerrainObj = res::g_Manager->MustGetResourceEx<Terrain>("SpaceTerrain");
	m_TerrainObj->Load();
	m_TerrainObj->GetBoundingBox(&MainBounds);

	m_WorldCX = m_TerrainObj->GetCX() * m_TerrainObj->GetVertexDistance();
	m_WorldCZ = m_TerrainObj->GetCZ() * m_TerrainObj->GetVertexDistance();

	g_TerrainObj = m_TerrainObj;

	// Inicjalizacja pól - ci¹g dalszy
	MAX_SPEED = sqrtf( PLAYER_ACCEL / FRICTION_FACTOR );

	m_PlayerPos.x = m_WorldCX * 0.5f;
	m_PlayerPos.y = 0.0f;
	m_PlayerSpeed = 0.0f;
	m_PlayerAngleY.Set(0.0f, 0.0f);
	m_PlayerAngleY.SmoothTime = ORIENTATION_SMOOTH_TIME;
	m_CameraAngleX.Set(3.0f, 0.0f);
	m_CameraAngleX.SmoothTime = CAMERA_ANGLE_X_SMOOTH_TIME;
	m_MotionBlurThresholdTime = 0.0f;

	m_EnemyPos.x = m_WorldCX * 0.5f;
	m_EnemyPos.y = m_WorldCZ;
	m_EnemySpeed = 0.0f;
	m_EnemyAngleY.Set(PI, 0.0f);
	m_EnemyAngleY.SmoothTime = ORIENTATION_SMOOTH_TIME;

	// Scena
	m_Scene.reset(new engine::Scene(MainBounds));
	engine::g_Engine->SetActiveScene(m_Scene.get());
	g_Scene = m_Scene.get();

	// Wiatr
	m_Scene->SetWindVec(WIND_VEC);

	// Bloom, Tone Mapping
	//m_Scene->SetPpBloom(new PpBloom(1.0f, 0.8f));
	m_Scene->SetPpToneMapping(new PpToneMapping(1.0f));

	// PpColor dla GAME_STATE_BEGIN
	m_Scene->SetPpColor(new PpColor(COLOR(0xFF000000)));

	// Kamera
	VEC2 StartCenter = VEC2(m_WorldCX * 0.5f, m_WorldCZ * 0.5f);
	m_Camera = new Camera(m_Scene.get(),
		VEC3(StartCenter.x, GAMEPLAY_Y, StartCenter.y), CAMERA_ANGLE_X, 0.0f, CAMERA_DIST, 2.0f, 100.0f, PI_4, GetCurrentAspectRatio());
	m_Scene->SetActiveCamera(m_Camera);

	// T³o
	{
		//engine::SolidSky *sky_obj = new engine::SolidSky(0xFF000000);

		FileStream fs("Game\\Terrain\\Space Sky.dat", FM_READ);
		Tokenizer t(&fs, 0);
		t.Next();
		engine::ComplexSky *sky_obj = new engine::ComplexSky(m_Scene.get(), t);
		sky_obj->SetTime(DAY_TIME);
		m_Scene->SetSky(sky_obj);
		t.AssertEOF();
	}

	// Mg³a
	m_Scene->SetFogStart(0.8f);
	m_Scene->SetFogColor(COLORF(0.40f, 0.28f, 0.18f));
	//m_Scene->SetFogColor(COLORF(0.15f, 0.10f, 0.04f));
	m_Scene->SetFogEnabled(true);

	// Definicje efektów cz¹steczkowych
	CreateParticleDefs();

	// Teren
	m_Scene->SetTerrain(new TerrainRenderer(
		m_Scene.get(),
		m_TerrainObj,
		NULL,
		NULL,
		string(),
		string()));

	// Materia³y
	CreateMaterials();

	// Œwiat³o
	m_Light = new DirectionalLight(m_Scene.get(), LIGHT_COLOR, LIGHT_DIR);
	m_Scene->SetAmbientColor(AMBIENT_COLOR);
	m_Light->SetCastShadow(true);
	m_Light->SetZFar(50.0f);
	m_Light->SetShadowFactor(SHADOW_FACTOR);

	// Ska³y
	CreateRocks();

	// Bohater
	m_PlayerEntity = new QMeshEntity(m_Scene.get(), "Craft01Model");
	m_PlayerEntity->SetPos(VEC3(m_PlayerPos.x, GAMEPLAY_Y, m_PlayerPos.y));
	m_PlayerEntity->SetCastShadow(true);
	m_PlayerEntity->SetReceiveShadow(false);

	// Przeciwnik
	m_EnemyEntity = new QMeshEntity(m_Scene.get(), "Craft02Model");
	m_EnemyEntity->SetPos(VEC3(m_EnemyPos.x, GAMEPLAY_Y, m_EnemyPos.y));
	m_EnemyEntity->SetSize(2.0f);
	m_EnemyEntity->SetCastShadow(true);
	m_EnemyEntity->SetReceiveShadow(false);

	// Particles - efekt odrzutu od silników bohatera
	m_PlayerParticles = new ParticleEntity(
		m_Scene.get(),
		"MainParticle",
		BaseMaterial::BLEND_ADD,
		m_PlayerParticleDef,
		20,
		2.0f);
	m_PlayerParticles->SetUseLOD(false);

	// Particles - efekt odrzutu od silników przeciwnika
	m_EnemyParticles[0] = new ParticleEntity(
		m_Scene.get(),
		"MainParticle",
		BaseMaterial::BLEND_SUB,
		m_EnemyParticleDef,
		20,
		2.0f);
	m_EnemyParticles[1] = new ParticleEntity(
		m_Scene.get(),
		"MainParticle",
		BaseMaterial::BLEND_SUB,
		m_EnemyParticleDef,
		20,
		2.0f);
	m_EnemyParticles[0]->SetUseLOD(false);
	m_EnemyParticles[1]->SetUseLOD(false);

	////// Lasery
	m_PlayerLasers[0] = new LineStripeEntity(m_Scene.get(), "Laser");
	m_PlayerLasers[1] = new LineStripeEntity(m_Scene.get(), "Laser");
	m_PlayerLasers[0]->SetHalfWidth(0.05f);
	m_PlayerLasers[1]->SetHalfWidth(0.05f);
	m_PlayerLasers[0]->SetTeamColor(COLORF(0.5f, 1.0f, 1.0f, 0.8f));
	m_PlayerLasers[1]->SetTeamColor(COLORF(0.5f, 1.0f, 1.0f, 0.8f));
	m_PlayerLasers[0]->SetVisible(false);
	m_PlayerLasers[1]->SetVisible(false);

	////// Efekty trafienia laserem
	m_PlayerLaserHitParticles = new ParticleEntity(
		m_Scene.get(),
		"MainParticle",
		BaseMaterial::BLEND_ADD,
		m_LaserHitParticleDef,
		8,
		0.5f);
	m_PlayerLaserHitParticles->SetVisible(false);

	m_PlayerLaserHitSmokeParticles = new ParticleEntity(
		m_Scene.get(),
		"SmokeParticle",
		BaseMaterial::BLEND_SUB,
		m_LaserHitSmokeParticleDef,
		24,
		1.0f);
	m_PlayerLaserHitSmokeParticles->SetVisible(false);
}

GameSpace_pimpl::~GameSpace_pimpl()
{
	m_Explosions.clear();
	m_EnemyMissiles.clear();
	SAFE_DELETE(m_PlayerLasers[1]);
	SAFE_DELETE(m_PlayerLasers[0]);
	SAFE_DELETE(m_EnemyParticles[1]);
	SAFE_DELETE(m_EnemyParticles[0]);
	SAFE_DELETE(m_PlayerParticles);

	g_Engine->SetActiveScene(NULL);
	m_Scene.reset();
}

void GameSpace_pimpl::CreateMaterials()
{
	OpaqueMaterial *Craft01Mat = new OpaqueMaterial(m_Scene.get(), "Craft01", "Craft01Texture");
	Craft01Mat->SetPerPixel(true);
	Craft01Mat->SetNormalTextureName("Craft01NormalTexture");
	Craft01Mat->SetGlossMapping(true);

	OpaqueMaterial *Craft02Mat = new OpaqueMaterial(m_Scene.get(), "Craft02", "Craft02Texture");
	Craft02Mat->SetPerPixel(true);
	Craft02Mat->SetNormalTextureName("Craft02NormalTexture");
	//Craft02Mat->SetGlossMapping(true);

	OpaqueMaterial *RockMat = new OpaqueMaterial(m_Scene.get(), "Rock", "MarsTerrain2");
	RockMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	RockMat->SetPerPixel(true);

	TranslucentMaterial *LaserMat = new TranslucentMaterial(m_Scene.get(), "Laser",
		"LaserStripeTexture",
		BaseMaterial::BLEND_ADD,
		SolidMaterial::COLOR_MODULATE,
		TranslucentMaterial::ALPHA_MODULATE);
	LaserMat->SetTwoSided(true);

	TranslucentMaterial *MissileMat = new TranslucentMaterial(m_Scene.get(), "Missile",
		"MissileParticle",
		BaseMaterial::BLEND_LERP,
		SolidMaterial::COLOR_MODULATE,
		TranslucentMaterial::ALPHA_MODULATE);
	MissileMat->SetTwoSided(true);
}

void GameSpace_pimpl::CreateParticleDefs()
{
	// Efekt odrzutu od silników bohatera
	m_PlayerParticleDef.Zero();
	m_PlayerParticleDef.CircleDegree = 0;
	m_PlayerParticleDef.PosA1_P = VEC3(0.0f, 0.0f, -2.0f);
	m_PlayerParticleDef.PosA0_C = VEC3(0.0f, 0.0f, 0.0f);
	m_PlayerParticleDef.ColorA0_C = VEC4(1.0f, 1.0f, 0.81f, 0.2f);
	m_PlayerParticleDef.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, -0.2f);
	m_PlayerParticleDef.OrientationA0_R = PI_X_2;
	m_PlayerParticleDef.OrientationA1_C = -PI;
	m_PlayerParticleDef.OrientationA1_R = PI_X_2;
	m_PlayerParticleDef.SizeA0_C = 0.3f;
	m_PlayerParticleDef.SizeA0_R = 0.1f;
	// To 1.1 to taki hack, inaczej z nieznanych przyczyn czasami czas siê zd¹¿y zawin¹æ i
	// miga znowu pocz¹tkowa klatka efektu zanim encja efektu zostanie zniszczona.
	m_PlayerParticleDef.TimePeriod_C = 0.2f;
	m_PlayerParticleDef.TimePhase_P = 1.0f;

	// Efekt odrzutu od silników przeciwnika
	m_EnemyParticleDef.Zero();
	m_EnemyParticleDef.CircleDegree = 0;
	m_EnemyParticleDef.PosA1_P = VEC3(0.0f, 0.0f, -2.0f);
	m_EnemyParticleDef.PosA0_C = VEC3(0.0f, 0.0f, 0.0f);
	m_EnemyParticleDef.ColorA0_C = VEC4(1.0f - 1.0f, 1.0f - 0.3f, 1.0f - 0.3f, 1.0f);
	m_EnemyParticleDef.ColorA1_C = VEC4(1.0f - 0.0f, 1.0f - 0.0f, 1.0f - 0.0f, -1.0f);
	m_EnemyParticleDef.OrientationA0_R = PI_X_2;
	m_EnemyParticleDef.OrientationA1_C = -PI;
	m_EnemyParticleDef.OrientationA1_R = PI_X_2;
	m_EnemyParticleDef.SizeA0_C = 0.3f;
	m_EnemyParticleDef.SizeA0_R = 0.2f;
	// To 1.1 to taki hack, inaczej z nieznanych przyczyn czasami czas siê zd¹¿y zawin¹æ i
	// miga znowu pocz¹tkowa klatka efektu zanim encja efektu zostanie zniszczona.
	m_EnemyParticleDef.TimePeriod_C = 0.2f;
	m_EnemyParticleDef.TimePhase_P = 1.0f;

	// Efekty trafienia laserem
	m_LaserHitParticleDef.Zero();
	m_LaserHitParticleDef.CircleDegree = 0;
	m_LaserHitParticleDef.PosA0_R = VEC3( 0.2f,  0.2f,  0.2f);
	m_LaserHitParticleDef.PosA0_C = VEC3(-0.1f, -0.1f, -0.1f);
	m_LaserHitParticleDef.ColorA0_C = VEC4(1.0f, 1.0f, 0.8f, 0.2f);
	m_LaserHitParticleDef.SizeA0_C = 0.4f;
	m_LaserHitParticleDef.TimePeriod_C = 1.0f;

	m_LaserHitSmokeParticleDef.Zero();
	m_LaserHitSmokeParticleDef.CircleDegree = 0;
	m_LaserHitSmokeParticleDef.PosA0_R = VEC3( 0.2f,  0.2f,  0.2f);
	m_LaserHitSmokeParticleDef.PosA0_C = VEC3(-0.1f, -0.1f, -0.1f);
	m_LaserHitSmokeParticleDef.PosA1_C = VEC3(-0.2f, 0.5f, -0.2f);
	m_LaserHitSmokeParticleDef.PosA1_R = VEC3(0.4f, 1.0f, 0.4f);
	m_LaserHitSmokeParticleDef.ColorA0_C = VEC4(1.0f, 1.0f, 1.0f, 0.0f);
	m_LaserHitSmokeParticleDef.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, 1.0f);
	m_LaserHitSmokeParticleDef.ColorA2_C = VEC4(0.0f, 0.0f, 0.0f, -1.0f);
	m_LaserHitSmokeParticleDef.SizeA0_C = 0.2f;
	m_LaserHitSmokeParticleDef.SizeA0_R = 0.3f;
	m_LaserHitSmokeParticleDef.TimePeriod_C = 1.0f;
	m_LaserHitSmokeParticleDef.TimePhase_R = 2.346f;
	m_LaserHitSmokeParticleDef.OrientationA1_R = 1.0f;

	// Ma³y wybuch - do trafienia pociskiem
	g_ExplosionFireParticleDef.Zero();
	g_ExplosionFireParticleDef.CircleDegree = 0;
	g_ExplosionFireParticleDef.PosA0_C = VEC3(-0.1f, -0.1f, -0.1f);
	g_ExplosionFireParticleDef.PosA0_R = VEC3( 0.2f,  0.2f,  0.2f);
	g_ExplosionFireParticleDef.PosA1_C = VEC3(-0.4f, -0.4f, -0.4f);
	g_ExplosionFireParticleDef.PosA1_R = VEC3( 0.8f,  0.8f,  0.8f);
	g_ExplosionFireParticleDef.ColorA0_C = VEC4(1.0f, 1.0f, 1.0f, 1.0f);
	g_ExplosionFireParticleDef.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, -2.0f);
	g_ExplosionFireParticleDef.SizeA0_C = 0.2f;
	g_ExplosionFireParticleDef.SizeA0_R = 0.1f;
	g_ExplosionFireParticleDef.SizeA1_C = 0.3f;
	g_ExplosionFireParticleDef.TimePeriod_C = EXPLOSION_TIME * 1.1f;
	g_ExplosionFireParticleDef.OrientationA1_C = -2.0f;
	g_ExplosionFireParticleDef.OrientationA1_R = 4.0f;

	g_ExplosionSmokeParticleDef.Zero();
	g_ExplosionSmokeParticleDef.CircleDegree = 0;
	g_ExplosionSmokeParticleDef.PosA0_C = VEC3(-0.1f, -0.1f, -0.1f);
	g_ExplosionSmokeParticleDef.PosA0_R = VEC3( 0.2f,  0.2f,  0.2f);
	g_ExplosionSmokeParticleDef.PosA1_C = VEC3(-0.5f,  0.0f, -0.5f);
	g_ExplosionSmokeParticleDef.PosA1_R = VEC3( 1.0f,  1.0f,  1.0f);
	g_ExplosionSmokeParticleDef.ColorA0_C = VEC4(1.0f, 1.0f, 1.0f, 0.0f);
	g_ExplosionSmokeParticleDef.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, 1.0f);
	g_ExplosionSmokeParticleDef.ColorA2_C = VEC4(0.0f, 0.0f, 0.0f, -1.0f);
	g_ExplosionSmokeParticleDef.SizeA0_C = 0.2f;
	g_ExplosionSmokeParticleDef.SizeA0_R = 0.1f;
	g_ExplosionSmokeParticleDef.SizeA1_C = 0.4f;
	g_ExplosionSmokeParticleDef.TimePeriod_C = EXPLOSION_TIME * 1.1f;
	g_ExplosionSmokeParticleDef.OrientationA1_C = -2.0f;
	g_ExplosionSmokeParticleDef.OrientationA1_R = 4.0f;

	// Du¿y wybuch - do statku
	m_BigExplosionFireParticleDef.Zero();
	m_BigExplosionFireParticleDef.CircleDegree = 0;
	m_BigExplosionFireParticleDef.PosA0_C = VEC3(-0.5f, -0.5f, -0.5f);
	m_BigExplosionFireParticleDef.PosA0_R = VEC3( 1.0f,  1.0f,  1.0f);
	m_BigExplosionFireParticleDef.PosA1_C = VEC3(-2.4f, -2.4f, -2.4f);
	m_BigExplosionFireParticleDef.PosA1_R = VEC3( 3.2f,  3.2f,  3.2f);
	m_BigExplosionFireParticleDef.ColorA0_C = VEC4(1.0f, 1.0f, 1.0f, 1.0f);
	m_BigExplosionFireParticleDef.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, -2.0f);
	m_BigExplosionFireParticleDef.SizeA0_C = 1.0f;
	m_BigExplosionFireParticleDef.SizeA0_R = 0.5f;
	m_BigExplosionFireParticleDef.SizeA1_C = 1.5f;
	m_BigExplosionFireParticleDef.TimePeriod_C = BIG_EXPLOSION_TIME * 1.1f;
	m_BigExplosionFireParticleDef.OrientationA1_C = -2.0f;
	m_BigExplosionFireParticleDef.OrientationA1_R = 4.0f;

	m_BigExplosionSmokeParticleDef.Zero();
	m_BigExplosionSmokeParticleDef.CircleDegree = 0;
	m_BigExplosionSmokeParticleDef.PosA0_C = VEC3(-0.5f, -0.5f, -0.5f);
	m_BigExplosionSmokeParticleDef.PosA0_R = VEC3( 1.0f,  1.0f,  1.0f);
	m_BigExplosionSmokeParticleDef.PosA1_C = VEC3(-2.5f, -1.5f, -2.5f);
	m_BigExplosionSmokeParticleDef.PosA1_R = VEC3( 5.0f,  5.0f,  5.0f);
	m_BigExplosionSmokeParticleDef.ColorA0_C = VEC4(1.0f, 1.0f, 1.0f, 0.0f);
	m_BigExplosionSmokeParticleDef.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, 1.0f);
	m_BigExplosionSmokeParticleDef.ColorA2_C = VEC4(0.0f, 0.0f, 0.0f, -1.0f);
	m_BigExplosionSmokeParticleDef.SizeA0_C = 1.0f;
	m_BigExplosionSmokeParticleDef.SizeA0_R = 0.5f;
	m_BigExplosionSmokeParticleDef.SizeA1_C = 2.0f;
	m_BigExplosionSmokeParticleDef.TimePeriod_C = BIG_EXPLOSION_TIME * 1.1f;
	m_BigExplosionSmokeParticleDef.OrientationA1_C = -2.0f;
	m_BigExplosionSmokeParticleDef.OrientationA1_R = 4.0f;
}

int GameSpace_pimpl::CalcFrame()
{
	float t = frame::Timer2.GetTime();
	float dt = frame::Timer2.GetDeltaTime();

	////// Stan gry
	switch (m_GameState)
	{
	case GAME_STATE_BEGIN:
		m_GameStateTime += dt / BEGIN_TIME;
		if (m_GameStateTime >= 1.0f)
		{
			m_GameState = GAME_STATE_NONE;
			m_Scene->SetPpColor(NULL);
		}
		else
			m_Scene->GetPpColor()->SetAlpha(1.0f - m_GameStateTime);
		break;
	case GAME_STATE_WIN_PENDING:
		m_GameStateTime += dt / BIG_EXPLOSION_TIME;
		m_Scene->GetPpColor()->SetAlpha(std::max(0.0f, 0.5f-m_GameStateTime*3.0f));
		if (m_GameStateTime >= 1.0f)
		{
			SAFE_DELETE(m_UltimateParticleEntity[1]);
			SAFE_DELETE(m_UltimateParticleEntity[0]);

			m_GameState = GAME_STATE_WIN;
			m_GameStateTime = 0.0f;

			m_Scene->SetPpColor(new PpColor(COLOR(0x00000000u)));
		}
		break;
	case GAME_STATE_LOSE_PENDING:
		m_GameStateTime += dt / BIG_EXPLOSION_TIME;
		m_Scene->GetPpColor()->SetAlpha(std::max(0.0f, 0.5f-m_GameStateTime*3.0f));
		if (m_GameStateTime >= 1.0f)
		{
			SAFE_DELETE(m_UltimateParticleEntity[1]);
			SAFE_DELETE(m_UltimateParticleEntity[0]);

			m_GameState = GAME_STATE_LOSE;
			m_GameStateTime = 0.0f;

			m_Scene->SetPpBloom(NULL); // Wy³¹czyæ PpBloom, bo Ÿle wspó³pracuje z PpFunction
			m_Scene->SetPpFunction(new PpFunction(VEC3::ONE, VEC3::ZERO, 0.0f));
		}
		break;
	case GAME_STATE_WIN:
		m_GameStateTime += dt / WIN_TIME;
		if (m_GameStateTime >= 1.0f)
		{
			m_Scene->SetPpColor(NULL);
			return 1;
		}
		else
			m_Scene->GetPpColor()->SetAlpha(m_GameStateTime);
		break;
	case GAME_STATE_LOSE:
		m_GameStateTime += dt / LOSE_TIME;
		if (m_GameStateTime >= 1.0f)
		{
			m_Scene->SetPpFunction(NULL);
			return -1;
		}
		else
		{
			float Factor = std::min(1.0f, m_GameStateTime * 2.0f);
			VEC3 B; Lerp(&B, VEC3::ZERO, SEPIA_FUNCTION_B, Factor);
			m_Scene->GetPpFunction()->SetGrayscaleFactor(Factor);
			m_Scene->GetPpFunction()->SetBFactor(B);
		}
		break;
	}

	if (m_GameState == GAME_STATE_NONE || m_GameState == GAME_STATE_BEGIN)
	{
		VEC2 PlayerDir, PlayerRight;
		VEC2 EnemyDir, EnemyRight;
		////// Gracz
		{
			// Przyspieszenie
			MATRIX RM;
			RotationMatrix2D(&RM, -m_PlayerAngleY.Pos);
			float Accel = 0.0f;
			if (frame::GetKeyboardKey(VK_UP) || frame::GetKeyboardKey('W'))
				Accel += PLAYER_ACCEL;
			if (frame::GetKeyboardKey(VK_DOWN) || frame::GetKeyboardKey('S'))
				Accel -= PLAYER_ACCEL;
			TransformNormal(&PlayerDir, VEC2::POSITIVE_Y, RM);
			Perpedicular(&PlayerRight, PlayerDir);
			// Prêdkoœæ
			//m_PlayerSpeed = std::max(0.0f, m_PlayerSpeed + Accel * dt);
			m_PlayerSpeed += Accel * dt;
			// Tarcie - utrata prêdkoœci
			float Friction = m_PlayerSpeed * m_PlayerSpeed * FRICTION_FACTOR * -sign(m_PlayerSpeed);
			m_PlayerSpeed += Friction * dt;
			// Pozycja
			m_PlayerPos += PlayerDir * m_PlayerSpeed * dt;
			m_PlayerPos.x = minmax(0.0f, m_PlayerPos.x, m_WorldCX);
			m_PlayerPos.y = minmax(0.0f, m_PlayerPos.y, m_WorldCZ);
			// Orientacja
			float NewPlayerAngleY = m_PlayerAngleY.Dest;
			if (frame::GetKeyboardKey(VK_LEFT) || frame::GetKeyboardKey('A'))
				NewPlayerAngleY -= ORIENTATION_VEL * dt;
			if (frame::GetKeyboardKey(VK_RIGHT) || frame::GetKeyboardKey('D'))
				NewPlayerAngleY += ORIENTATION_VEL * dt;
			m_PlayerAngleY.Update(NewPlayerAngleY, dt);

			// Model gracza

			float PlayerVisualAngleZ = (m_PlayerAngleY.Pos - m_PlayerAngleY.Dest) * 2.0f;
			
			MATRIX PlayerRotMat;
			RotationYawPitchRoll(&PlayerRotMat, m_PlayerAngleY.Pos + PI, CAMERA_ANGLE_X, PlayerVisualAngleZ);
			QUATERNION PlayerQuat;
			RotationMatrixToQuaternion(&PlayerQuat, PlayerRotMat);
			m_PlayerEntity->SetOrientation(PlayerQuat);

			m_PlayerEntity->SetPos(VEC3(m_PlayerPos.x, GAMEPLAY_Y, m_PlayerPos.y));

			// Particles

			m_PlayerParticles->SetPos(VEC3(m_PlayerPos.x - PlayerDir.x, GAMEPLAY_Y + 0.1f, m_PlayerPos.y - PlayerDir.y));
			m_PlayerParticles->SetOrientation(PlayerQuat);

			// Kamera

			m_CameraAngleX.Update(0.5f - m_PlayerSpeed*0.010f, dt); // Magic Numbers :)

			m_Camera->SetPos(m_PlayerEntity->GetPos() + VEC3(0.0f, CAMERA_ABOVE_PLAYER, 0.0f));
			m_Camera->SetAngleY(m_PlayerAngleY.Pos);
			m_Camera->SetAngleX(m_CameraAngleX.Pos);
		}

		// Efekt Postprocessing Feedback
		bool MotionBlurEnabled = false;
		if (m_PlayerSpeed > MAX_SPEED * SPEED_MOTION_BLUR_THRESHOLD)
		{
			m_MotionBlurThresholdTime += dt;

			if (m_MotionBlurThresholdTime > MOTION_BLUR_THRESHOLD_TIME)
			{
				if (m_Scene->GetPpFeedback() == NULL)
					m_Scene->SetPpFeedback(new PpFeedback(PpFeedback::M_MOTION_BLUR, 1.0f));

				//float EffectFactor =
				//	m_PlayerSpeed / (MAX_SPEED * (1.0f - SPEED_MOTION_BLUR_THRESHOLD)) -
				//	SPEED_MOTION_BLUR_THRESHOLD / (1.0f - SPEED_MOTION_BLUR_THRESHOLD);
				m_Scene->GetPpFeedback()->SetIntensity(0.5f);

				MotionBlurEnabled = true;
			}
		}
		else
		{
			m_MotionBlurThresholdTime = 0.0f;
		}

		if (!MotionBlurEnabled)
		{
			if (m_Scene->GetPpFeedback() != NULL)
				m_Scene->SetPpFeedback(NULL);
		}

		////// Przeciwnik
		{
			MATRIX RM;
			RotationMatrix2D(&RM, -m_EnemyAngleY.Pos);
			TransformNormal(&EnemyDir, VEC2::POSITIVE_Y, RM);
			Perpedicular(&EnemyRight, EnemyDir);
			// AI
			if (m_LastAICalcTime + AI_CALC_PERIOD < frame::Timer2.GetTime())
			{
				m_LastAICalcTime = frame::Timer2.GetTime();
				CalcEnemyAI(PlayerDir, PlayerRight, EnemyDir, EnemyRight, &m_EnemyKeyUp, &m_EnemyKeyDown, &m_EnemyKeyLeft, &m_EnemyKeyRight, &m_EnemyShoot);
			}
			// Przyspieszenie
			float Accel = 0.0f;
			if (m_EnemyKeyUp)   Accel += PLAYER_ACCEL;
			if (m_EnemyKeyDown) Accel -= PLAYER_ACCEL;
			// Prêdkoœæ
			//m_EnemySpeed = std::max(0.0f, m_EnemySpeed + Accel * dt);
			m_EnemySpeed += Accel * dt;
			// Tarcie - utrata prêdkoœci
			float Friction = m_EnemySpeed * m_EnemySpeed * FRICTION_FACTOR * -sign(m_EnemySpeed);
			m_EnemySpeed += Friction * dt;
			// Pozycja
			m_EnemyPos += EnemyDir * m_EnemySpeed * dt;
			m_EnemyPos.x = minmax(0.0f, m_EnemyPos.x, m_WorldCX);
			m_EnemyPos.y = minmax(0.0f, m_EnemyPos.y, m_WorldCZ);
			// Orientacja
			float NewEnemyAngleY = m_EnemyAngleY.Dest;
			if (m_EnemyKeyLeft)  NewEnemyAngleY -= ORIENTATION_VEL * dt;
			if (m_EnemyKeyRight) NewEnemyAngleY += ORIENTATION_VEL * dt;
			m_EnemyAngleY.Update(NewEnemyAngleY, dt);

			// Model przeciwnika

			float EnemyVisualAngleZ = (m_EnemyAngleY.Pos - m_EnemyAngleY.Dest) * 2.0f;
			
			MATRIX EnemyRotMat;
			RotationYawPitchRoll(&EnemyRotMat, m_EnemyAngleY.Pos + PI, CAMERA_ANGLE_X, EnemyVisualAngleZ);
			QUATERNION EnemyQuat;
			RotationMatrixToQuaternion(&EnemyQuat, EnemyRotMat);
			m_EnemyEntity->SetOrientation(EnemyQuat);

			m_EnemyEntity->SetPos(VEC3(m_EnemyPos.x, GAMEPLAY_Y, m_EnemyPos.y));

			// Particles

			VEC2 V;
			V = m_EnemyPos - EnemyDir * 1.6f - EnemyRight * 0.5f;
			m_EnemyParticles[0]->SetPos(VEC3(V.x, GAMEPLAY_Y + 0.15f, V.y));
			V = m_EnemyPos - EnemyDir * 1.6f + EnemyRight * 0.5f;
			m_EnemyParticles[1]->SetPos(VEC3(V.x, GAMEPLAY_Y + 0.15f, V.y));
			m_EnemyParticles[0]->SetOrientation(EnemyQuat);
			m_EnemyParticles[1]->SetOrientation(EnemyQuat);
		}

		////// Strza³ gracza

		if (m_MouseButtonDown)
		{
			VEC2 MousePos;
			frame::GetMousePos(&MousePos);

			VEC3 RayOrig, RayDir;
			m_Camera->CalcMouseRay(&RayOrig, &RayDir,
				MousePos.x / frame::GetScreenWidth(),
				MousePos.y / frame::GetScreenHeight());

			bool LaserActive = false;
			VEC3 LaserDest;
			float T;
			Entity *E;
			Scene::COLLISION_RESULT CR = m_Scene->RayCollision(engine::COLLISION_PHYSICAL, RayOrig, RayDir, &T, &E);
			if (CR == Scene::COLLISION_RESULT_TERRAIN)
			{
				if (T >= 0.0f && T <= m_Camera->GetZFar())
				{
					LaserDest = RayOrig + RayDir * T;
					LaserActive = true;
				}
			}
			else if (CR == Scene::COLLISION_RESULT_ENTITY)
			{
				if (E != m_PlayerEntity && T >= 0.0f && T <= m_Camera->GetZFar())
				{
					LaserDest = RayOrig + RayDir * T;
					LaserActive = true;

					if (E == m_EnemyEntity)
					{
						m_EnemyLife -= LASER_DAMAGE_PER_SECOND * dt;
						if (m_EnemyLife <= 0.0f)
							Win();
					}
				}
			}

			m_PlayerLasers[0]->SetVisible(LaserActive);
			m_PlayerLasers[1]->SetVisible(LaserActive);
			m_PlayerLaserHitParticles->SetVisible(LaserActive);
			m_PlayerLaserHitSmokeParticles->SetVisible(LaserActive);

			if (LaserActive)
			{
				VEC3 LaserStripePoints[2];
				VEC2 TmpV;
				LaserStripePoints[1] = LaserDest;

				TmpV = m_PlayerPos + PlayerDir * 0.5f + PlayerRight * 0.9f;
				LaserStripePoints[0] = VEC3(TmpV.x, GAMEPLAY_Y - 0.15f, TmpV.y);
				m_PlayerLasers[0]->SetPoints(LaserStripePoints, 2);

				TmpV = m_PlayerPos + PlayerDir * 0.5f - PlayerRight * 0.9f;
				LaserStripePoints[0] = VEC3(TmpV.x, GAMEPLAY_Y - 0.15f, TmpV.y);
				m_PlayerLasers[1]->SetPoints(LaserStripePoints, 2);

				m_PlayerLaserHitParticles->SetPos(LaserDest);
				m_PlayerLaserHitSmokeParticles->SetPos(LaserDest);
			}
		}

		////// Wystrzelenie pocisku przeciwnika
		if (m_EnemyShoot && m_LastEnemyShootTime + ENEMY_SHOT_PERIOD < frame::Timer2.GetTime())
		{
			m_LastEnemyShootTime = frame::Timer2.GetTime();

			QuadEntity *qe = new QuadEntity(m_Scene.get(), "Missile");
			qe->SetDegreesOfFreedom(2);
			qe->SetUseRealDir(false);
			qe->SetHalfSize(VEC2(0.3f, 0.3f));
			qe->SetPos(VEC3(m_EnemyPos.x + EnemyDir.x * 1.0f, GAMEPLAY_Y, m_EnemyPos.y + EnemyDir.y * 1.0f));
			qe->SetTeamColor(COLORF(1.0f, 0.00f, 0.50f, 0.00f));
			m_EnemyMissiles.push_back( shared_ptr<QuadEntity>(qe) );
			m_EnemyMissileTimes.push_back(ENEMY_MISSILE_LIFE_TIME);
		}

		////// Pociski przeciwnika
		{
			MISSILE_LIST::iterator it;
			std::list<float>::iterator tit;
			for (it = m_EnemyMissiles.begin(), tit = m_EnemyMissileTimes.begin(); it != m_EnemyMissiles.end(); )
			{
				*tit -= dt;
				// Czas ¿ycia pocisku siê skoñczy³
				if (*tit <= 0.0f)
				{
					it = m_EnemyMissiles.erase(it);
					tit = m_EnemyMissileTimes.erase(tit);
				}
				else
				{
					VEC3 DirToPlayer = m_PlayerEntity->GetPos() - (*it)->GetPos();
					if (LengthSq(DirToPlayer) <= PLAYER_RADIUS*PLAYER_RADIUS)
					{
						// Eksplozja
						m_Explosions.push_back(EXPLOSION(m_Scene.get(), (*it)->GetPos()));
						// Strata ¿ycia
						m_PlayerLife -= ENEMY_MISSILE_DAMAGE;
						if (m_PlayerLife <= 0.0f)
							Lose();

						it = m_EnemyMissiles.erase(it);
						tit = m_EnemyMissileTimes.erase(tit);
					}
					else
					{
						Normalize(&DirToPlayer);
						(*it)->SetPos( (*it)->GetPos() + DirToPlayer * MISSILE_SPEED * dt );

						++it;
						++tit;
					}
				}
			}
		}
	}
	else if (m_GameState == GAME_STATE_WIN_PENDING || m_GameState == GAME_STATE_LOSE_PENDING)
	{
		m_Camera->SetPos(m_Camera->GetPos() + VEC3(
			g_Rand.RandFloat(-dt, dt) * 5.0f * (1.0f - m_GameStateTime),
			g_Rand.RandFloat(-dt, dt) * 5.0f * (1.0f - m_GameStateTime),
			g_Rand.RandFloat(-dt, dt) * 5.0f * (1.0f - m_GameStateTime)));
	}

	////// Eksplozje
	for (EXPLOSION_LIST::iterator it = m_Explosions.begin(); it != m_Explosions.end(); )
	{
		if (it->IsFinished())
			it = m_Explosions.erase(it);
		else
			++it;
	}

	return 0;
}

void GameSpace_pimpl::OnResolutionChanged()
{
	m_Camera->SetAspect(GetCurrentAspectRatio());
}

void GameSpace_pimpl::OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	// Wciœniêcie klawisza
	if (Action == frame::MOUSE_DOWN || Action == frame::MOUSE_DBLCLK)
		m_MouseButtonDown = true;
	// Zwolnienie klawisza myszy
	else if (Action == frame::MOUSE_UP)
	{
		m_MouseButtonDown = false;
		m_PlayerLaserHitSmokeParticles->SetVisible(false);
		m_PlayerLaserHitParticles->SetVisible(false);
		m_PlayerLasers[0]->SetVisible(false);
		m_PlayerLasers[1]->SetVisible(false);
	}
}

void GameSpace_pimpl::CreateRocks()
{
	string ModelName = "Rock1Model";
	VEC3 Pos;
	MATRIX RM;
	QUATERNION Q;

	for (uint i = 0; i < ROCK_COUNT; i++)
	{
		// W po³owie zmiana modelu na drugi
		if (i == ROCK_COUNT / 2)
			ModelName = "Rock2Model";

		Pos.x = POISSON_DISC_2D[i].x * m_WorldCX;
		Pos.z = POISSON_DISC_2D[i].y * m_WorldCZ;
		Pos.y = m_TerrainObj->GetHeight(Pos.x, Pos.z);

		RotationYawPitchRoll(&RM, g_Rand.RandFloat(PI_X_2), g_Rand.RandFloat(-PI_2*0.2f, PI_2*0.2f), 0.0f);
		RotationMatrixToQuaternion(&Q, RM);

		QMeshEntity *E = new QMeshEntity(m_Scene.get(), ModelName);
		E->SetPos(Pos);
		E->SetOrientation(Q);
		E->SetSize(ROCK_SIZE + g_Rand.RandFloat(-ROCK_SIZE_V, ROCK_SIZE_V));
	}
}

void GameSpace_pimpl::CalcEnemyAI(
	const VEC2 &PlayerDir, const VEC2 &PlayerRight,
	const VEC2 &EnemyDir, const VEC2 &EnemyRight,
	bool *OutKeyUp, bool *OutKeyDown, bool *OutKeyLeft, bool *OutKeyRight, bool *OutShoot)
{
	// Obrazuje, czy gracz jest zwrócony w stronê wroga (1.0), czy przeciwnie (-1.0)
	float PlayerTurnedToEnemy;
	VEC2 PlayerToEnemy = m_EnemyPos - m_PlayerPos;
	Normalize(&PlayerToEnemy);
	PlayerTurnedToEnemy = Dot(PlayerDir, PlayerToEnemy);

	// Obrazuje, czy wróg jest zwrócony w stronê gracza (1.0) czy przeciwnie (-1.0)
	float EnemyTurnedToPlayer;
	EnemyTurnedToPlayer = Dot(EnemyDir, -PlayerToEnemy);

	// Obrazuje odleg³oœæ od gracza: 0.0 - pozycja równa graczowi, 1.0 - granica zasiêgu kamery, > 1.0 - poza zasiêgiem
	float DistanceToPlayer = Distance(m_PlayerPos, m_EnemyPos) / m_Camera->GetZFar();

	// Jeœli wróg widzi gracza
	if (DistanceToPlayer < 1.0f && EnemyTurnedToPlayer > 0.0f)
	{
		// Ostateczna decyzja co robiæ:
		// > 0.0 - lecimy w kierunku gracza
		// < 0.0 - uciekamy od gracza
		float Decision = 0.0f;
		Decision += 2.0f * DistanceToPlayer - 1.0f;
		Decision += EnemyTurnedToPlayer;
		Decision -= PlayerTurnedToEnemy;

		*OutKeyDown = false;

		static const float SPEED_FACTOR = 1.0f;
		// Gonimy
		if (Decision > 0.0f)
		{
			float DesiredSpeed = (DistanceToPlayer - 0.2f) * MAX_SPEED * SPEED_FACTOR;
			*OutKeyUp = (m_EnemySpeed < DesiredSpeed);

			float cr = Cross(EnemyDir, -PlayerToEnemy);
			*OutKeyRight = (cr < -0.3f);
			*OutKeyLeft =  (cr >  0.3f);
		}
		// Uciekamy
		else
		{
			float DesiredSpeed = (SPEED_FACTOR - SPEED_FACTOR * DistanceToPlayer) * MAX_SPEED;
			*OutKeyUp = (m_EnemySpeed < DesiredSpeed);

			float cr = Cross(PlayerDir, PlayerToEnemy);
			*OutKeyRight = (cr <= 0.0f);
			*OutKeyLeft =  (cr >  0.0f);
		}

		// Strzelanie
		*OutShoot = (EnemyTurnedToPlayer > 0.5f) && DistanceToPlayer <= 0.8f;

		// Specjalne sterowanie do wydostawania siê z krawêdzi
		if (m_EnemyPos.x < 1.0f ||
			m_EnemyPos.y < 1.0f ||
			m_EnemyPos.x > m_WorldCX - 1.0f ||
			m_EnemyPos.y > m_WorldCZ - 1.0f)
		{
			*OutKeyRight = true;
			*OutKeyLeft = false;
		}
	}
	// Jeœli wróg nie widzi gracza
	else
	{
		*OutShoot = false;
		*OutKeyDown = false;
		*OutKeyLeft = false;
		*OutKeyRight = false;
		*OutKeyUp = true;
		// Prosty algorytm
		if (m_EnemyPos.y < m_WorldCZ * 0.5f)
			*OutKeyRight = true;
		else
			*OutKeyLeft = true;
	}
}

float GameSpace_pimpl::GetCurrentAspectRatio()
{
	return (float)frame::Settings.BackBufferWidth / (float)frame::Settings.BackBufferHeight;
}

void GameSpace_pimpl::Win()
{
	LOG(LOG_GAME, "Win");

	m_GameState = GAME_STATE_WIN_PENDING;
	m_GameStateTime = 0.0f;

	// Bia³y ekran
	m_Scene->SetPpColor(new PpColor(COLOR(0x80FFFFFF)));

	// Ukryj model przeciwnika
	m_EnemyEntity->SetVisible(false);
	m_EnemyParticles[0]->SetVisible(false);
	m_EnemyParticles[1]->SetVisible(false);
	// Uruchom efekt wybuchu
	m_UltimateParticleEntity[0] = new ParticleEntity(m_Scene.get(),
		"FireParticle",
		BaseMaterial::BLEND_ADD,
		m_BigExplosionFireParticleDef,
		32,
		1.0f);
	m_UltimateParticleEntity[0]->SetPos(m_EnemyEntity->GetPos());

	m_UltimateParticleEntity[1] = new ParticleEntity(m_Scene.get(),
		"SmokeParticle",
		BaseMaterial::BLEND_SUB,
		m_BigExplosionSmokeParticleDef,
		32,
		1.0f);
	m_UltimateParticleEntity[1]->SetPos(m_EnemyEntity->GetPos());
}

void GameSpace_pimpl::Lose()
{
	LOG(LOG_GAME, "Loss");

	m_GameState = GAME_STATE_LOSE_PENDING;
	m_GameStateTime = 0.0f;

	// Bia³y ekran
	m_Scene->SetPpColor(new PpColor(COLOR(0x80FFFFFF)));

	// Ukryj model gracza
	m_PlayerEntity->SetVisible(false);
	m_PlayerParticles->SetVisible(false);
	// Uruchom efekt wybuchu
	m_UltimateParticleEntity[0] = new ParticleEntity(m_Scene.get(),
		"FireParticle",
		BaseMaterial::BLEND_ADD,
		m_BigExplosionFireParticleDef,
		32,
		1.0f);
	m_UltimateParticleEntity[0]->SetPos(m_PlayerEntity->GetPos());

	m_UltimateParticleEntity[1] = new ParticleEntity(m_Scene.get(),
		"SmokeParticle",
		BaseMaterial::BLEND_SUB,
		m_BigExplosionSmokeParticleDef,
		32,
		1.0f);
	m_UltimateParticleEntity[1]->SetPos(m_PlayerEntity->GetPos());
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GameSpace

GameSpace::GameSpace() :
	pimpl(new GameSpace_pimpl)
{
}

GameSpace::~GameSpace()
{
}

int GameSpace::CalcFrame()
{
	return pimpl->CalcFrame();
}

void GameSpace::OnResolutionChanged()
{
	pimpl->OnResolutionChanged();
}

void GameSpace::OnMouseMove(const common::VEC2 &Pos)
{
}

void GameSpace::OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	pimpl->OnMouseButton(Pos, Button, Action);
}

void GameSpace::OnMouseWheel(const common::VEC2 &Pos, float Delta)
{
}

engine::Scene * GameSpace::GetScene()
{
	return pimpl->GetScene();
}
