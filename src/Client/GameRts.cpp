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
#include "..\Engine\Water.hpp"
#include "..\Engine\Grass.hpp"
#include "GameRts.hpp"

using namespace engine;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy globalne

static const float AI3_PERIOD = 0.333333f; // [s]
static const float WIN_CHECK_PERIOD = 2.0f;

static VEC3 LIGHT_DIR;
static COLORF LIGHT_COLOR;
static COLORF AMBIENT_COLOR;

static float CAMERA_ANGLE_X;
static float CAMERA_ANGLE_Y;
static float CAMERA_DIST;
static float CAMERA_SMOOTH_TIME;

static VEC2 WIND_VEC;

static float BEGIN_TIME;
static float WIN_TIME;
static float LOSE_TIME;
static float SHADOW_FACTOR;
static VEC2 CASTLE_POS;
static float CASTLE_SIZE;
static RECTF TEAM1_LOC, TEAM2_LOC;
static uint TEAM1_UNITS, TEAM2_UNITS;
static float UNIT_SIZE;
static float ATTACK_TIME, ATTACK_ANIM_SPEED;
static float ANIM_BLEND_TIME;
static float CAM_SPEED;
static float UNIT_RADIUS;
static float UNIT_SPEED;
static float SELECTION_TIME;
static float FORMATION_HALFSIZE;
static float ATTACK_DISTANCE;
static float ATTACK_DAMAGE;
static float ATTACK_PARTICLE_EFFECT_TIME;

static engine::Scene *g_Scene;
static engine::Terrain *g_TerrainObj;
static PARTICLE_DEF g_AttackParticleDef; // Definicja efektu cz¹steczkowego walki

class Unit;
typedef std::list< shared_ptr<Unit> > UNIT_LIST;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Unit

/*
- Klasa sama utrzymuje obiekt encji silnika.
- Team:
  0 = gracz
  1 = komputer
*/
class Unit
{
public:
	Unit(uint Index, const VEC2 &Pos, uint Team);
	~Unit();

	uint GetTeam() { return m_Team; }
	bool IsDead() { return m_AI1_State == AI1_DEAD; }
	const VEC2 & GetPos() { return m_Pos; }
	bool IsHisEntity(Entity *E) { return m_Entity == E; }

	void Update(const UNIT_LIST &UnitList);
	void Command(const VEC2 &Dest, bool ForceWalk);
	void Damage();

private:
	uint m_Index;
	VEC2 m_Pos;
	uint m_Team;
	float m_Angle;
	QMeshEntity *m_Entity;
	float m_Life; // 0..1

	// Czas dope³nia siê 1..0
	ParticleEntity *m_ParticleEntity;
	float m_ParticleEntityTimeout;

	////// AI, Level 1
	// - Steruje animacj¹ modelu.
	// - Nie przesuwa ani nie obraca modelu.
	enum AI1_STATE
	{
		AI1_NONE,
		AI1_WALK,
		AI1_ATTACK,
		AI1_DEAD,
	};
	AI1_STATE m_AI1_State;
	// Czas, który siê dope³nia 1..0 do koñca stanu jeœli ma czas trwania (AI1_ATTACK)
	float m_AI1_Timeout;
	AI1_STATE m_AI1_NextState;
	void AI1_Update();
	void AI1_Attack();
	void AI1_Die();
	void AI1_Walk();
	void AI1_Stop();

	////// AI, Level 2
	// - Odpowiada za cel wy¿szego poziomu, tzn. pójœcie gdzieœ lub atakowanie czegoœ.
	// - Nie uwzglêdnia przyczyn tego celu.
	// - Zmienia orientacjê i pozycjê jednostki.
	enum AI2_STATE
	{
		AI2_NONE,
		AI2_WALK, // Pójœcie do lokacji m_AI2_Pos
		AI2_ATTACK, // Atakowanie m_AI2_Obj
	};
	AI2_STATE m_AI2_State;
	VEC2 m_AI2_Pos;
	Unit *m_AI2_Obj;
	void AI2_Update(const UNIT_LIST &UnitList);

	////// AI, Level 3
	// - Ustawiany z zewn¹trz.
	// - Przeliczany tylko co jakiœ czas.
	VEC2 m_AI3_Pos;
	bool m_AI3_ForceWalk;
	float m_AI3_Timeout;
	void AI3_Update(const UNIT_LIST &UnitList);
	
	// Aktualizuje pozycjê i orientacjê modelu stosownie do stanu jednostki
	void UpdateEntityParams();
	// Aktualizuje animacjê modelu stosownie do stanu jedonstki z czêœci AI1
	void UpdateEntityAnim();
	void CreateParticleEffect();
};

Unit::Unit(uint Index, const VEC2 &Pos, uint Team) :
	m_Index(Index),
	m_Pos(Pos),
	m_Team(Team),
	m_Angle(g_Rand.RandFloat(PI_X_2)),
	m_Life(1.0f),
	m_AI1_State(AI1_NONE),
	m_AI1_NextState(AI1_NONE),
	m_AI2_State(AI2_NONE),
	m_AI3_Pos(Pos),
	m_AI3_ForceWalk(false),
	// Losowa faza, ¿eby nie wszystkie liczy³y siê w tej samej klatce, bo to by mog³o przycinaæ grê.
	m_AI3_Timeout(g_Rand.RandFloat(AI3_PERIOD)),
	m_ParticleEntity(NULL),
	m_ParticleEntityTimeout(0.0f)
{
	m_Entity = new QMeshEntity(g_Scene, "SwordsmanModel");
	m_Entity->SetTeamColor(m_Team == 1 ? COLORF::BLUE : COLORF::RED);
	m_Entity->SetSize(UNIT_SIZE);
	UpdateEntityParams();
}

Unit::~Unit()
{
	delete m_ParticleEntity;
	delete m_Entity;
}

void Unit::Update(const UNIT_LIST &UnitList)
{
	float dt = frame::Timer2.GetDeltaTime();

	if (m_ParticleEntity != NULL)
	{
		m_ParticleEntityTimeout -= dt / ATTACK_PARTICLE_EFFECT_TIME;
		if (m_ParticleEntityTimeout <= 0.0f)
		{
			m_ParticleEntityTimeout = 0.0f;
			SAFE_DELETE(m_ParticleEntity);
		}
	}

	if (!IsDead())
	{
		m_AI3_Timeout -= dt / AI3_PERIOD;
		if (m_AI3_Timeout <= 0.0f)
		{
			AI3_Update(UnitList);
			m_AI3_Timeout = AI3_PERIOD;
		}

		AI2_Update(UnitList);
	}
	AI1_Update();
}

void Unit::Command(const VEC2 &Dest, bool ForceWalk)
{
	if (IsDead()) return;

	m_AI3_Pos = Dest;
	m_AI3_ForceWalk = ForceWalk;

	m_AI3_Pos.x += POISSON_DISC_2D[m_Index].x * FORMATION_HALFSIZE * 2.0f - FORMATION_HALFSIZE;
	m_AI3_Pos.y += POISSON_DISC_2D[m_Index].y * FORMATION_HALFSIZE * 2.0f - FORMATION_HALFSIZE;
}

void Unit::Damage()
{
	if (m_Life > 0.0f)
	{
		m_Life -= ATTACK_DAMAGE + g_Rand.RandNormal(ATTACK_DAMAGE * 0.333f);
		if (m_Life <= 0.0f)
			AI1_Die();
	}
}

void Unit::AI1_Update()
{
	if (m_AI1_State == AI1_ATTACK)
	{
		m_AI1_Timeout -= frame::Timer2.GetDeltaTime() / ATTACK_TIME;
		if (m_AI1_Timeout <= 0.0f)
		{
			m_AI1_State = m_AI1_NextState;
			m_AI1_Timeout = 1.0f;
			UpdateEntityAnim();
		}
	}
}

void Unit::AI1_Attack()
{
	if (m_AI1_State == AI1_NONE || m_AI1_State == AI1_WALK)
	{
		m_AI1_NextState = m_AI1_State;
		m_AI1_State = AI1_ATTACK;
		m_AI1_Timeout = 1.0f;
		UpdateEntityAnim();
	}
}

void Unit::AI1_Die()
{
	if (m_AI1_State != AI1_DEAD)
	{
		m_AI1_State = AI1_DEAD;
		UpdateEntityAnim();
	}
}

void Unit::AI1_Walk()
{
	if (m_AI1_State == AI1_NONE)
	{
		m_AI1_State = AI1_WALK;
		UpdateEntityAnim();
	}
	else if (m_AI1_State == AI1_ATTACK)
		m_AI1_NextState = AI1_WALK;
}

void Unit::AI1_Stop()
{
	if (m_AI1_State == AI1_WALK)
	{
		m_AI1_State = AI1_NONE;
		UpdateEntityAnim();
	}
	else if (m_AI1_State == AI1_ATTACK)
		m_AI1_NextState = AI1_NONE;
}

void Unit::AI2_Update(const UNIT_LIST &UnitList)
{
	float dt = frame::Timer2.GetDeltaTime();

	// Rozsuwanie jednostek za bliskich
	{
		VEC2 Dir = VEC2::ZERO;
		for (UNIT_LIST::const_iterator it = UnitList.begin(); it != UnitList.end(); ++it)
		{
			if (!(*it)->IsDead())
			{
				if (DistanceSq(GetPos(), (*it)->GetPos()) < UNIT_RADIUS * UNIT_RADIUS * (2.1f * 2.1f))
					Dir += GetPos() - (*it)->GetPos();
			}
		}
		if (LengthSq(Dir) > 0.001f)
		{
			Normalize(&Dir);
			m_Pos += Dir * dt * 1.0f;
		}
	}

	if (m_AI2_State == AI2_NONE)
	{
		AI1_Stop();
		return;
	}

	// AI2_WALK || AI2_ATTACK

	if (m_AI2_State == AI2_ATTACK)
	{
		// Atakuje i stoi ju¿ przy jednostce atakowanej - uderzenie
		if (DistanceSq(GetPos(), m_AI2_Obj->GetPos()) <= (2.1f*2.1f) * UNIT_RADIUS*UNIT_RADIUS)
		{
			m_Angle = atan2f(
				m_AI2_Obj->GetPos().x - GetPos().x,
				m_AI2_Obj->GetPos().y - GetPos().y);
			// Hack, bo AI2 odnosi siê wprost do danych AI1 - ale trudno...
			if (m_AI1_State != AI1_ATTACK)
			{
				CreateParticleEffect();
				m_AI2_Obj->Damage();
			}
			AI1_Attack();
			UpdateEntityParams();
			return;
		}
	}

	// Nie atakuje lub nie stoi przy jednostce atakowanej - idzie
	const VEC2 & DestPos = (m_AI2_State == AI2_ATTACK ? m_AI2_Obj->GetPos() : m_AI2_Pos);
	// Ju¿ dosz³a
	if (fabsf(DestPos.x - GetPos().x) <= 0.1f && fabsf(DestPos.y - GetPos().y) <= 0.1f)
	{
		m_AI2_State = AI2_NONE;
		AI1_Stop();
		return;
	}
	VEC2 Dir = DestPos - GetPos();
	if (LengthSq(Dir) > 0.0001f)
	{
		Normalize(&Dir);
		m_Angle = atan2f(Dir.x, Dir.y);
		m_Pos += Dir * UNIT_SPEED * dt;
	}

	UpdateEntityParams();
}

void Unit::AI3_Update(const UNIT_LIST &UnitList)
{
	// Jednostka musi iœæ do celu
	if (m_AI3_ForceWalk)
	{
		// Ju¿ jest u celu
		if (fabsf(GetPos().x - m_AI3_Pos.x) < 0.2f && fabsf(GetPos().y - m_AI3_Pos.y) < 0.2f)
		{
			// Nie musi ju¿ iœæ
			m_AI3_ForceWalk = false;

			// Jeœli nie stoi, to zatrzymaj
			if (m_AI2_State != AI2_NONE)
			{
				m_AI2_State = AI2_NONE;
				AI1_Stop();
			}
		}
		// Jeœli jeszcze nie jest u celu
		else
		{
			// Zaktualizuj pozycjê docelow¹
			m_AI2_Pos = m_AI3_Pos;
			// Jeœli nie idzie, ka¿ jej iœæ
			if (m_AI2_State != AI2_WALK)
			{
				m_AI2_State = AI2_WALK;
				AI1_Walk();
			}
		}
	}
	// Jednoska mo¿e iœæ lub atakowaæ
	else
	{
		float BestDistSq = MAXFLOAT, DistSq;
		Unit *BestUnit = NULL;
		// Poszukaj najbli¿szego celu
		for (UNIT_LIST::const_iterator it = UnitList.begin(); it != UnitList.end(); ++it)
		{
			if ((*it)->GetTeam() != GetTeam() && !(*it)->IsDead())
			{
				DistSq = DistanceSq(GetPos(), (*it)->GetPos());
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					BestUnit = (*it).get();
				}
			}
		}

		// Jeœli najbli¿szy cel jest blisko - atakuj
		if (BestDistSq <= ATTACK_DISTANCE*ATTACK_DISTANCE)
		{
			m_AI2_Obj = BestUnit;
			m_AI2_State = AI2_ATTACK;
			AI1_Walk(); // ???
		}
		// Najbli¿szy cel jest daleko - idŸ
		else
		{
			// KOD SKOPIOWANY Z PIERWSZEGO IFA TEJ METODY !!!

			// Ju¿ jest u celu
			if (fabsf(GetPos().x - m_AI3_Pos.x) < 0.2f && fabsf(GetPos().y - m_AI3_Pos.y) < 0.2f)
			{
				// Nie musi ju¿ iœæ
				m_AI3_ForceWalk = false;

				// Jeœli nie stoi, to zatrzymaj
				if (m_AI2_State != AI2_NONE)
				{
					m_AI2_State = AI2_NONE;
					AI1_Stop();
				}
			}
			// Jeœli jeszcze nie jest u celu
			else
			{
				// Zaktualizuj pozycjê docelow¹
				m_AI2_Pos = m_AI3_Pos;
				// Jeœli nie idzie, ka¿ jej iœæ
				if (m_AI2_State != AI2_WALK)
				{
					m_AI2_State = AI2_WALK;
					AI1_Walk();
				}
			}
		}
	}
}

void Unit::UpdateEntityParams()
{
	m_Entity->SetPos(VEC3(m_Pos.x, g_TerrainObj->GetHeight(m_Pos.x, m_Pos.y), m_Pos.y));

	QUATERNION q;
	QuaternionRotationY(&q, m_Angle + PI);
	m_Entity->SetOrientation(q);
}

void Unit::UpdateEntityAnim()
{
	switch (m_AI1_State)
	{
	case AI1_WALK:
		if (ANIM_BLEND_TIME > 0.001f)
			m_Entity->BlendAnimation("Walk", ANIM_BLEND_TIME, QMeshEntity::ANIM_MODE_LOOP, g_Rand.RandFloat(), 1.0f);
		else
			m_Entity->SetAnimation("Walk", QMeshEntity::ANIM_MODE_LOOP, g_Rand.RandFloat(), 1.0f);
		break;
	case AI1_ATTACK:
		if (ANIM_BLEND_TIME > 0.001f)
			m_Entity->BlendAnimation("Attack", ANIM_BLEND_TIME, QMeshEntity::ANIM_MODE_NORMAL, 0.0f, ATTACK_ANIM_SPEED);
		else
			m_Entity->SetAnimation("Attack", QMeshEntity::ANIM_MODE_NORMAL, 0.0f, ATTACK_ANIM_SPEED);
		break;
	case AI1_DEAD:
		if (ANIM_BLEND_TIME > 0.001f)
			m_Entity->BlendAnimation("Die", ANIM_BLEND_TIME, QMeshEntity::ANIM_MODE_NORMAL, 0.0f, 1.0f);
		else
			m_Entity->SetAnimation("Die", QMeshEntity::ANIM_MODE_NORMAL, 0.0f, 1.0f);
		break;
	default: // AI1_NONE
		if (ANIM_BLEND_TIME > 0.001f)
			m_Entity->BlendAnimation(string(), ANIM_BLEND_TIME, 0);
		else
			m_Entity->BlendAnimation(string(), 0);
		break;
	}
}

void Unit::CreateParticleEffect()
{
	SAFE_DELETE(m_ParticleEntity);

	m_ParticleEntityTimeout = 1.0f;
	m_ParticleEntity = new ParticleEntity(
		g_Scene,
		"SparkParticleTexture",
		engine::BaseMaterial::BLEND_ADD,
		g_AttackParticleDef,
		20,
		1.0f);
	m_ParticleEntity->SetPos(VEC3(
		m_Entity->GetPos().x + cosf(- m_Angle + PI_2) * UNIT_RADIUS,
		m_Entity->GetPos().y + 1.0f,
		m_Entity->GetPos().z + sinf(- m_Angle + PI_2) * UNIT_RADIUS));
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GameRts_pimpl

class GameRts_pimpl
{
public:
	GameRts_pimpl();
	~GameRts_pimpl();
	int CalcFrame();
	void OnResolutionChanged();
	void OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	Scene * GetScene() { return m_Scene.get(); }

private:
	Terrain *m_TerrainObj;
	scoped_ptr<Scene> m_Scene;
	// Zwolni go scena
	Camera *m_Camera;
	// Zwolni go scena
	DirectionalLight *m_Light;
	// Zwolniê go albo ja, albo scena jeœli ja nie zd¹¿ê
	// Alpha dope³nia siê 1..0, jak spadnie do 0, encja jest niszczona
	QuadEntity *m_SelectionEntity;
	float m_WinCheckTimeout;

	SmoothCD_obj<VEC3> m_CameraSmoothPos;

	// Bie¿¹cy stan rozgrywki
	enum GAME_STATE
	{
		GAME_STATE_NONE,
		GAME_STATE_BEGIN, // Istnieje w scenie PpColor
		GAME_STATE_WIN,   // Istnieje w scenie PpFunction
		GAME_STATE_LOSE,  // Istnieje w scenie PpColor
	};
	GAME_STATE m_GameState;
	// Czas bie¿acego stanu, dope³nia siê 0..1
	float m_GameStateTime;

	UNIT_LIST m_Units;

	float GetCurrentAspectRatio();
	void Win();
	void Lose();
	void MarkSelection(const VEC2 &Pos, const COLORF &Color);
	// Zwraca jednostkê, do której nale¿y podana encja lub NULL, jeœli do ¿adnej.
	Unit * EntityToUnit(Entity *E);
};


GameRts_pimpl::GameRts_pimpl() :
	m_SelectionEntity(NULL),
	m_WinCheckTimeout(1.0f)
{
	m_GameState = GAME_STATE_BEGIN;
	m_GameStateTime = 0.0f;

	// Wczytaj konfiguracjê

	g_Config->MustGetDataEx("Game/BeginTime", &BEGIN_TIME);
	g_Config->MustGetDataEx("Game/WinTime", &WIN_TIME);
	g_Config->MustGetDataEx("Game/LoseTime", &LOSE_TIME);
	g_Config->MustGetDataEx("Game/Rts/LightDir", &LIGHT_DIR);
	g_Config->MustGetDataEx("Game/Rts/LightColor", &LIGHT_COLOR);
	g_Config->MustGetDataEx("Game/Rts/AmbientColor", &AMBIENT_COLOR);
	g_Config->MustGetDataEx("Game/Rts/CameraAngleX", &CAMERA_ANGLE_X);
	g_Config->MustGetDataEx("Game/Rts/CameraAngleY", &CAMERA_ANGLE_Y);
	g_Config->MustGetDataEx("Game/Rts/CameraDist", &CAMERA_DIST);
	g_Config->MustGetDataEx("Game/Rts/CameraSmoothTime", &CAMERA_SMOOTH_TIME);
	g_Config->MustGetDataEx("Game/Rts/WindVec", &WIND_VEC);
	g_Config->MustGetDataEx("Game/Rts/ShadowFactor", &SHADOW_FACTOR);
	g_Config->MustGetDataEx("Game/Rts/CastlePos", &CASTLE_POS);
	g_Config->MustGetDataEx("Game/Rts/CastleSize", &CASTLE_SIZE);
	g_Config->MustGetDataEx("Game/Rts/Team1_Loc", &TEAM1_LOC);
	g_Config->MustGetDataEx("Game/Rts/Team2_Loc", &TEAM2_LOC);
	g_Config->MustGetDataEx("Game/Rts/Team1_Units", &TEAM1_UNITS);
	g_Config->MustGetDataEx("Game/Rts/Team2_Units", &TEAM2_UNITS);
	g_Config->MustGetDataEx("Game/Rts/UnitSize", &UNIT_SIZE);
	g_Config->MustGetDataEx("Game/Rts/AttackTime", &ATTACK_TIME);
	g_Config->MustGetDataEx("Game/Rts/AnimBlendTime", &ANIM_BLEND_TIME);
	g_Config->MustGetDataEx("Game/Rts/CamSpeed", &CAM_SPEED);
	g_Config->MustGetDataEx("Game/Rts/UnitRadius", &UNIT_RADIUS);
	g_Config->MustGetDataEx("Game/Rts/UnitSpeed", &UNIT_SPEED);
	g_Config->MustGetDataEx("Game/Rts/SelectionTime", &SELECTION_TIME);
	g_Config->MustGetDataEx("Game/Rts/FormationHalfsize", &FORMATION_HALFSIZE);
	g_Config->MustGetDataEx("Game/Rts/AttackDistance", &ATTACK_DISTANCE);
	g_Config->MustGetDataEx("Game/Rts/AttackDamage", &ATTACK_DAMAGE);
	g_Config->MustGetDataEx("Game/Rts/AttackParticleEffectTime", &ATTACK_PARTICLE_EFFECT_TIME);

	res::QMesh *SwordsmanMesh = res::g_Manager->MustGetResourceEx<res::QMesh>("SwordsmanModel");
	SwordsmanMesh->Load();
	ATTACK_ANIM_SPEED = SwordsmanMesh->GetAnimationByName("Attack")->GetLength() / ATTACK_TIME;

	// AABB mapy
	BOX MainBounds;
	m_TerrainObj = res::g_Manager->MustGetResourceEx<Terrain>("RtsTerrain");
	m_TerrainObj->Load();
	m_TerrainObj->GetBoundingBox(&MainBounds);
	g_TerrainObj = m_TerrainObj;

	// Scena
	m_Scene.reset(new engine::Scene(MainBounds));
	engine::g_Engine->SetActiveScene(m_Scene.get());
	g_Scene = m_Scene.get();

	// Wiatr
	m_Scene->SetWindVec(WIND_VEC);

	// Bloom
	m_Scene->SetPpBloom(new PpBloom(1.0f, 0.8f));
	m_Scene->SetPpToneMapping(new PpToneMapping(1.0f));

	// PpColor dla GAME_STATE_BEGIN
	m_Scene->SetPpColor(new PpColor(COLOR(0xFF000000)));

	// Kamera
	VEC2 StartCenter;
	TEAM1_LOC.GetCenter(&StartCenter);
	m_Camera = new Camera(m_Scene.get(),
		VEC3(StartCenter.x, 0.0f, StartCenter.y), CAMERA_ANGLE_X, CAMERA_ANGLE_Y, CAMERA_DIST, 2.0f, 50.0f, PI_4, GetCurrentAspectRatio());
	m_Scene->SetActiveCamera(m_Camera);
	m_CameraSmoothPos.SmoothTime = CAMERA_SMOOTH_TIME;
	m_CameraSmoothPos.Set(m_Camera->GetPos(), VEC3::ZERO);

	// T³o
	engine::SolidSky *sky_obj = new engine::SolidSky(0xFF000000);
	m_Scene->SetSky(sky_obj);

	// Teren
	TerrainWater *WaterObj = new TerrainWater(m_Scene.get(), m_TerrainObj, m_TerrainObj->ValueToHeight(80));
	WaterObj->SetDirToLight(VEC3(0.0f, 1.0f, 0.0f));
	Grass *GrassObj = new Grass(m_Scene.get(), m_TerrainObj,
		"Game\\Terrain\\GrassDesc.dat",
		"Game\\Terrain\\RTS Grass Density Map.tga");

	m_Scene->SetTerrain(new TerrainRenderer(
		m_Scene.get(),
		m_TerrainObj,
		GrassObj,
		WaterObj,
		"Game\\Terrain\\TreeDesc.dat",
		"Game\\Terrain\\RTS Tree Density Map.tga"));

	// Materia³y

	OpaqueMaterial *CastleMat = new OpaqueMaterial(m_Scene.get(), "Castle", "CastleTexture");
	CastleMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	CastleMat->SetPerPixel(false);
	CastleMat->SetTwoSided(true); // Taki hack - bo jedna beczka siê pierniczy, poza tym ¿eby flaga by³a z dwóch stron

	OpaqueMaterial *SwordsmanMat = new OpaqueMaterial(m_Scene.get(), "Swordsman", "SwordsmanTexture");
	SwordsmanMat->SetPerPixel(false);
	SwordsmanMat->SetColorMode(SolidMaterial::COLOR_LERP_ALPHA); // Team Color
	SwordsmanMat->SetSpecularPower(32.0f);
	SwordsmanMat->SetSpecularColor(COLORF::SILVER); // ¿eby nie lœni³o tak bardzo

	TranslucentMaterial *SelectionMat = new TranslucentMaterial(
		m_Scene.get(),
		"Selection",
		"SelectionTexture",
		BaseMaterial::BLEND_LERP,
		SolidMaterial::COLOR_MODULATE,
		TranslucentMaterial::ALPHA_MODULATE);

	// Œwiat³o
	m_Light = new DirectionalLight(m_Scene.get(), LIGHT_COLOR, LIGHT_DIR);
	m_Scene->SetAmbientColor(AMBIENT_COLOR);
	m_Light->SetCastShadow(true);
	m_Light->SetZFar(30.0f);
	m_Light->SetShadowFactor(SHADOW_FACTOR);

	// Budynek
	QMeshEntity *Castle = new QMeshEntity(m_Scene.get(), "CastleModel");
	Castle->SetPos(VEC3(CASTLE_POS.x, m_TerrainObj->GetHeight(CASTLE_POS.x, CASTLE_POS.y), CASTLE_POS.y));
	Castle->SetSize(CASTLE_SIZE);
	Castle->SetCastShadow(true);

	// Efekt cz¹steczkowy
	g_AttackParticleDef.Zero();
	g_AttackParticleDef.CircleDegree = 0;
	g_AttackParticleDef.PosA2_C = VEC3(0.0f, -2.0f, 0.0f);
	g_AttackParticleDef.PosA1_C = VEC3(-1.0f, 0.0f, -1.0f);
	g_AttackParticleDef.PosA1_R = VEC3(4.0f, 0.0f, 4.0f);
	g_AttackParticleDef.PosA0_C = VEC3(0.0f, 0.0f, 0.0f);
	g_AttackParticleDef.ColorA0_R = VEC4(0.0f, 0.0f, 0.5f, 0.0f);
	g_AttackParticleDef.ColorA0_C = VEC4(1.0f, 1.0f, 0.0f, 1.0f);
	g_AttackParticleDef.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, -1.0f);
	g_AttackParticleDef.OrientationA0_R = PI_X_2;
	g_AttackParticleDef.OrientationA1_C = -PI;
	g_AttackParticleDef.OrientationA1_R = PI_X_2;
	g_AttackParticleDef.SizeA0_C = 0.05f;
	g_AttackParticleDef.SizeA0_R = 0.05f;
	// To 1.1 to taki hack, inaczej z nieznanych przyczyn czasami czas siê zd¹¿y zawin¹æ i
	// miga znowu pocz¹tkowa klatka efektu zanim encja efektu zostanie zniszczona.
	g_AttackParticleDef.TimePeriod_C = ATTACK_PARTICLE_EFFECT_TIME * 1.1f;

	// Jednostki
	for (uint i = 0; i < TEAM1_UNITS; i++)
	{
		m_Units.push_back(shared_ptr<Unit>(new Unit(m_Units.size(), VEC2(
			POISSON_DISC_2D[i].x * TEAM1_LOC.Width()  + TEAM1_LOC.left,
			POISSON_DISC_2D[i].y * TEAM1_LOC.Height() + TEAM1_LOC.top), 0)));
	}
	for (uint i = 0; i < TEAM2_UNITS; i++)
	{
		m_Units.push_back(shared_ptr<Unit>(new Unit(m_Units.size(), VEC2(
			POISSON_DISC_2D[i].x * TEAM2_LOC.Width()  + TEAM2_LOC.left,
			POISSON_DISC_2D[i].y * TEAM2_LOC.Height() + TEAM2_LOC.top), 1)));
	}
}

GameRts_pimpl::~GameRts_pimpl()
{
	// Usuñ jednostki, ¿eby usunê³y swoje encje zanim usunê scenê, bo same chc¹ to zrobiæ.
	m_Units.clear();

	g_Engine->SetActiveScene(NULL);
	m_Scene.reset();
}

int GameRts_pimpl::CalcFrame()
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
		////// Przelicz jednostki
		for (UNIT_LIST::iterator it = m_Units.begin(); it != m_Units.end(); ++it)
			(*it)->Update(m_Units);

		////// Przelicz znaczek zaznaczenia
		if (m_SelectionEntity)
		{
			COLORF SelectionEntityTeamColor = m_SelectionEntity->GetTeamColor();
			SelectionEntityTeamColor.A -= dt / SELECTION_TIME;
			if (SelectionEntityTeamColor.A <= 0.0f)
			{
				SAFE_DELETE(m_SelectionEntity);
			}
			else
				m_SelectionEntity->SetTeamColor(SelectionEntityTeamColor);
		}

		////// Kamera

		VEC2 MousePos;
		frame::GetMousePos(&MousePos);
		VEC3 NewCamPos = m_CameraSmoothPos.Dest;
		if (frame::GetKeyboardKey(VK_LEFT) || frame::GetKeyboardKey('A') || MousePos.x < 4.0f)
			NewCamPos.x -= CAM_SPEED * dt;
		if (frame::GetKeyboardKey(VK_RIGHT) || frame::GetKeyboardKey('D') || MousePos.x >= frame::GetScreenWidth()-4.0f)
			NewCamPos.x += CAM_SPEED * dt;
		if (frame::GetKeyboardKey(VK_UP) || frame::GetKeyboardKey('W') || MousePos.y < 4.0f)
			NewCamPos.z += CAM_SPEED * dt;
		if (frame::GetKeyboardKey(VK_DOWN) || frame::GetKeyboardKey('S') || MousePos.y >= frame::GetScreenHeight()-4.0f)
			NewCamPos.z -= CAM_SPEED * dt;
		NewCamPos.x = minmax<float>(0.0f, NewCamPos.x, m_TerrainObj->GetCX() * m_TerrainObj->GetVertexDistance());
		NewCamPos.z = minmax<float>(0.0f, NewCamPos.z, m_TerrainObj->GetCZ() * m_TerrainObj->GetVertexDistance());
		m_CameraSmoothPos.Update(NewCamPos, dt);
		m_Camera->SetPos(m_CameraSmoothPos.Pos);

		////// Wygrana, przegrana

		m_WinCheckTimeout -= dt / WIN_CHECK_PERIOD;
		if (m_WinCheckTimeout < 0.0f)
		{
			m_WinCheckTimeout = 1.0f;

			uint Alive[2] = { 0, 0 };
			for (UNIT_LIST::iterator it = m_Units.begin(); it != m_Units.end(); ++it)
			{
				if (!(*it)->IsDead())
					Alive[(*it)->GetTeam()]++;
			}
			if (Alive[1] == 0)
				Win();
			else if (Alive[0] == 0)
				Lose();
		}
	}

	return 0;
}

void GameRts_pimpl::OnResolutionChanged()
{
	m_Camera->SetAspect(GetCurrentAspectRatio());
}

void GameRts_pimpl::OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Action != frame::MOUSE_DOWN && Action != frame::MOUSE_DBLCLK)
		return;

	VEC3 RayOrig, RayDir;
	m_Camera->CalcMouseRay(&RayOrig, &RayDir,
		Pos.x / frame::GetScreenWidth(),
		Pos.y / frame::GetScreenHeight());

	float T;
	Entity *E;
	Scene::COLLISION_RESULT CR = m_Scene->RayCollision(engine::COLLISION_PHYSICAL, RayOrig, RayDir, &T, &E);
	if (CR == Scene::COLLISION_RESULT_TERRAIN)
	{
		VEC2 Dest = VEC2(
			RayOrig.x + T * RayDir.x,
			RayOrig.z + T * RayDir.z);
		MarkSelection(Dest, COLORF::LIME);
		for (std::list< shared_ptr<Unit> >::iterator it = m_Units.begin(); it != m_Units.end(); ++it)
			if ((*it)->GetTeam() == 0)
				(*it)->Command(Dest, true);
	}
	else if (CR == Scene::COLLISION_RESULT_ENTITY)
	{
		assert(E != NULL);
		Unit *U = EntityToUnit(E);
		if (U != NULL && U->GetTeam() == 1 && !U->IsDead())
		{
			MarkSelection(U->GetPos(), COLORF::RED);
			for (std::list< shared_ptr<Unit> >::iterator it = m_Units.begin(); it != m_Units.end(); ++it)
				if ((*it)->GetTeam() == 0)
					(*it)->Command(U->GetPos(), false);
		}
	}
}

float GameRts_pimpl::GetCurrentAspectRatio()
{
	return (float)frame::Settings.BackBufferWidth / (float)frame::Settings.BackBufferHeight;
}

void GameRts_pimpl::Win()
{
	LOG(LOG_GAME, "Win");

	m_GameState = GAME_STATE_WIN;
	m_GameStateTime = 0.0f;

	m_Scene->SetPpColor(new PpColor(COLOR(0x00000000u)));
}

void GameRts_pimpl::Lose()
{
	LOG(LOG_GAME, "Loss");

	m_GameState = GAME_STATE_LOSE;
	m_GameStateTime = 0.0f;

	m_Scene->SetPpBloom(NULL); // Wy³¹czyæ PpBloom, bo Ÿle wspó³pracuje z PpFunction
	m_Scene->SetPpFunction(new PpFunction(VEC3::ONE, VEC3::ZERO, 0.0f));
}

void GameRts_pimpl::MarkSelection(const VEC2 &Pos, const COLORF &Color)
{
	SAFE_DELETE(m_SelectionEntity);

	m_SelectionEntity = new QuadEntity(m_Scene.get(), "Selection");
	m_SelectionEntity->SetHalfSize(VEC2(UNIT_RADIUS, UNIT_RADIUS));
	m_SelectionEntity->SetRightDir(VEC3::POSITIVE_X);
	m_SelectionEntity->SetUpDir(VEC3::POSITIVE_Z);
	m_SelectionEntity->SetTeamColor(COLORF(1.0f, Color.R, Color.G, Color.B));

	VEC3 EntityPos;
	EntityPos.x = Pos.x;
	EntityPos.z = Pos.y;
	EntityPos.y = m_TerrainObj->GetHeight(Pos.x, Pos.y) + 0.1f;
	m_SelectionEntity->SetPos(EntityPos);
}

Unit * GameRts_pimpl::EntityToUnit(Entity *E)
{
	for (UNIT_LIST::iterator it = m_Units.begin(); it != m_Units.end(); ++it)
	{
		if ((*it)->IsHisEntity(E))
			return (*it).get();
	}
	return NULL;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GameRts

GameRts::GameRts() :
	pimpl(new GameRts_pimpl)
{
}

GameRts::~GameRts()
{
}

int GameRts::CalcFrame()
{
	return pimpl->CalcFrame();
}

void GameRts::OnResolutionChanged()
{
	pimpl->OnResolutionChanged();
}

void GameRts::OnMouseMove(const common::VEC2 &Pos)
{
}

void GameRts::OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	pimpl->OnMouseButton(Pos, Button, Action);
}

void GameRts::OnMouseWheel(const common::VEC2 &Pos, float Delta)
{
}

engine::Scene * GameRts::GetScene()
{
	return pimpl->GetScene();
}
