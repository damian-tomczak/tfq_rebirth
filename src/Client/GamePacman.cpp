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
#include "..\Engine\QMap.hpp"
#include "GamePacman.hpp"

using namespace engine;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy globalne

enum MOVEMENT_DIR
{
	DIR_NONE,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_UP,
	DIR_DOWN,
};

static VEC3 LIGHT_DIR;
static COLORF LIGHT_COLOR;
static COLORF AMBIENT_COLOR;

static const uint MAP_CX = 17;
static const uint MAP_CY = 17;
static const char FIELD_WALL  = '#';
static const char FIELD_GEM   = '.';
static const char FIELD_EMPTY = 'E';

static float GEM_ROT_SPEED;
static float PACMAN_STEP_TIME;
static float GHOST_STEP_TIME;

static float CAMERA_ANGLE_X;
static float CAMERA_ANGLE_Y;
static float CAMERA_DIST;
static float CAMERA_SMOOTH_TIME;
static float GEM_COLLECT_PARTICLE_EFFECT_TIME;
static float BEGIN_TIME;
static float WIN_TIME;
static float LOSE_TIME;

static const uint GHOST_COUNT = 4;
static const COLORF GHOST_TEAM_COLORS[GHOST_COUNT] = {
	COLORF(0.6f, 1.0f, 0.5f, 0.5f),
	COLORF(0.6f, 0.5f, 1.0f, 0.5f),
	COLORF(0.6f, 0.5f, 0.7f, 1.0f),
	COLORF(0.6f, 1.0f, 1.0f, 0.0f)
};
static const uint GHOST_START_X[GHOST_COUNT] = { 15, 11,  8,  9 };
static const uint GHOST_START_Y[GHOST_COUNT] = {  6, 10, 15,  3 };

float DirToX(MOVEMENT_DIR Dir)
{
	switch (Dir)
	{
	case DIR_LEFT:
		return -1.0f;
	case DIR_RIGHT:
		return 1.0f;
	default:
		return 0.0f;
	}
}

float DirToY(MOVEMENT_DIR Dir)
{
	switch (Dir)
	{
	case DIR_DOWN:
		return -1.0f;
	case DIR_UP:
		return 1.0f;
	default:
		return 0.0f;
	}
}

float DirToAngle(MOVEMENT_DIR Dir)
{
	switch (Dir)
	{
	case DIR_UP:
		return PI_2;
	case DIR_DOWN:
		return PI + PI_2;
	case DIR_LEFT:
		return PI;
	case DIR_RIGHT:
		return 0.0f;
	default:
		return 0.0f;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Map

class Map
{
public:
	Map(const string &MapFileName);

	char GetField(uint x, uint y) { return m_Fields[x][y]; }
	void CollectGem(uint x, uint y);

private:
	char m_Fields[MAP_CX][MAP_CY];
};

Map::Map(const string &MapFileName)
{
	ERR_TRY;

	common::FileStream File(MapFileName, FM_READ);
	common::CharReader Reader(&File);

	for (uint y = 0; y < MAP_CY; y++)
	{
		for (uint x = 0; x < MAP_CX; x++)
		{
			m_Fields[x][y] = Reader.MustReadChar();
			if (m_Fields[x][y] != FIELD_WALL && m_Fields[x][y] != FIELD_GEM && m_Fields[x][y] != FIELD_EMPTY)
				throw Error("B³êdny znak w pliku.");

			// Spacja
			if (x < MAP_CX - 1)
				Reader.MustSkipChar();
		}

		// Koniec wiersza
		if (y < MAP_CY - 1)
		{
			Reader.MustSkipChar(); // \r
			Reader.MustSkipChar(); // \n
		}
	}

	ERR_CATCH(Format("Nie mo¿na wczytaæ mapy z \"#\"") % MapFileName);
}

void Map::CollectGem(uint x, uint y)
{
	if (m_Fields[x][y] == FIELD_GEM)
		m_Fields[x][y] = FIELD_EMPTY;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Movement

class Movement
{
public:
	Movement(Map *a_Map, uint x, uint y, MOVEMENT_DIR Dir, float StepTime);

	uint Get_x() { return m_x; }
	uint Get_y() { return m_y; }
	MOVEMENT_DIR GetDir() { return m_Dir; }

	void Move();
	void Input(bool KeyLeft, bool KeyRight, bool KeyDown, bool KeyUp);
	float CalcWorldX();
	float CalcWorldZ();

private:
	Map *m_Map;

	uint m_x;
	uint m_y;
	MOVEMENT_DIR m_Dir;
	float m_StepTime;
	float m_MovementTime; // Dope³nia siê 0..1
};

Movement::Movement(Map *a_Map, uint x, uint y, MOVEMENT_DIR Dir, float StepTime) :
	m_Map(a_Map),
	m_x(x), m_y(y),
	m_Dir(Dir),
	m_StepTime(StepTime),
	m_MovementTime(1.0f)
{
}

void Movement::Move()
{
	if (m_Dir != DIR_NONE && m_MovementTime < 1.0f)
	{
		// Dalszy ruch
		m_MovementTime += frame::Timer2.GetDeltaTime() / m_StepTime;
		// Koniec ruchu
		if (m_MovementTime >= 1.0f)
		{
			// Rest czasu
			m_MovementTime = 1.0f;
			// Osi¹gniêcie pola docelowego,
			// sprawdzenie czy mo¿e iœæ dalej
			switch (m_Dir)
			{
			case DIR_LEFT:
				m_x--;
				if (m_x > 0 && m_Map->GetField(m_x-1, m_y) != FIELD_WALL)
					m_MovementTime = 0.0f;
				else
					m_Dir = DIR_NONE;
				break;
			case DIR_RIGHT:
				m_x++;
				if (m_x < MAP_CX - 1 && m_Map->GetField(m_x+1, m_y) != FIELD_WALL)
					m_MovementTime = 0.0f;
				else
					m_Dir = DIR_NONE;
				break;
			case DIR_DOWN:
				m_y--;
				if (m_y > 0 && m_Map->GetField(m_x, m_y-1) != FIELD_WALL)
					m_MovementTime = 0.0f;
				else
					m_Dir = DIR_NONE;
				break;
			case DIR_UP:
				m_y++;
				if (m_y < MAP_CY - 1 && m_Map->GetField(m_x, m_y+1) != FIELD_WALL)
					m_MovementTime = 0.0f;
				else
					m_Dir = DIR_NONE;
				break;
			}
		}
	}
}

void Movement::Input(bool KeyLeft, bool KeyRight, bool KeyDown, bool KeyUp)
{
	if (KeyLeft &&
		m_Dir != DIR_LEFT &&
		m_x > 0 &&
		m_Map->GetField(m_x-1, m_y) != FIELD_WALL)
	{
		m_MovementTime = 0.0f;
		m_Dir = DIR_LEFT;
	}
	else if (KeyDown &&
		m_Dir != DIR_DOWN &&
		m_y > 0 &&
		m_Map->GetField(m_x, m_y-1) != FIELD_WALL)
	{
		m_MovementTime = 0.0f;
		m_Dir = DIR_DOWN;
	}
	else if (KeyRight &&
		m_Dir != DIR_RIGHT &&
		m_x < MAP_CX-1 &&
		m_Map->GetField(m_x+1, m_y) != FIELD_WALL)
	{
		m_MovementTime = 0.0f;
		m_Dir = DIR_RIGHT;
	}
	else if (KeyUp &&
		m_Dir != DIR_UP &&
		m_y < MAP_CY-1 &&
		m_Map->GetField(m_x, m_y+1) != FIELD_WALL)
	{
		m_MovementTime = 0.0f;
		m_Dir = DIR_UP;
	}
}

float Movement::CalcWorldX()
{
	return (m_x + DirToX(m_Dir) * m_MovementTime + 0.5f) * 2.0f;
}

float Movement::CalcWorldZ()
{
	return (m_y + DirToY(m_Dir) * m_MovementTime + 0.5f) * 2.0f;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GamePacman_pimpl

class GamePacman_pimpl
{
public:
	GamePacman_pimpl();
	~GamePacman_pimpl();
	int CalcFrame();
	void OnResolutionChanged();
	Scene * GetScene() { return m_Scene.get(); }

private:
	scoped_ptr<Scene> m_Scene;
	Camera *m_Camera; // Zwolni go scena
	DirectionalLight *m_Light; // Zwolni go scena
	QMeshEntity *m_Pacman; // Zwolni go scena
	PARTICLE_DEF m_CollectParticleDef; // Definicja efektu cz¹steczkowego zbieranie kryszta³ka

	scoped_ptr<Map> m_Map;

	SmoothCD_obj<VEC3> m_CameraSmoothPos;

	// Encje odpowiadaj¹ce kryszta³kom i ich wspó³rzêdne (X,Y)
	std::vector<QMeshEntity*> m_Gems; // W razie pozostania niezebranych na koñcu ¿ycia obiektu, zwolni je scena
	std::vector< std::pair<uint, uint> > m_GemCoords;

	scoped_ptr<Movement> m_PacmanMovement;

	scoped_ptr<Movement> m_GhostMovements[GHOST_COUNT];
	QMeshEntity *m_Ghosts[GHOST_COUNT]; // Zwolni je scena

	// Efekty cz¹steczkowe zbierania kryszta³ka i ich czasy ¿ycia 1..0
	std::vector<ParticleEntity*> m_CollectParticleEntities; // W razie pozostania istniej¹cych w chwili niszczenia obiektu, zwolni je scena
	std::vector<float> m_CollectParticleEntityTimes;

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

	float GetCurrentAspectRatio();
	VEC3 CalcPacmanWorldPos();
	void Win();
	void Lose();
};


GamePacman_pimpl::GamePacman_pimpl()
{
	m_GameState = GAME_STATE_BEGIN;
	m_GameStateTime = 0.0f;

	g_Config->MustGetDataEx("Game/BeginTime", &BEGIN_TIME);
	g_Config->MustGetDataEx("Game/WinTime", &WIN_TIME);
	g_Config->MustGetDataEx("Game/LoseTime", &LOSE_TIME);
	g_Config->MustGetDataEx("Game/Pacman/LightDir", &LIGHT_DIR);
	g_Config->MustGetDataEx("Game/Pacman/LightColor", &LIGHT_COLOR);
	g_Config->MustGetDataEx("Game/Pacman/AmbientColor", &AMBIENT_COLOR);
	g_Config->MustGetDataEx("Game/Pacman/GemRotSpeed", &GEM_ROT_SPEED);
	g_Config->MustGetDataEx("Game/Pacman/PacmanStepTime", &PACMAN_STEP_TIME);
	g_Config->MustGetDataEx("Game/Pacman/GhostStepTime", &GHOST_STEP_TIME);
	g_Config->MustGetDataEx("Game/Pacman/CameraAngleX", &CAMERA_ANGLE_X);
	g_Config->MustGetDataEx("Game/Pacman/CameraAngleY", &CAMERA_ANGLE_Y);
	g_Config->MustGetDataEx("Game/Pacman/CameraDist", &CAMERA_DIST);
	g_Config->MustGetDataEx("Game/Pacman/CameraSmoothTime", &CAMERA_SMOOTH_TIME);
	g_Config->MustGetDataEx("Game/Pacman/GemCollectParticleEffectTime", &GEM_COLLECT_PARTICLE_EFFECT_TIME);

	m_Map.reset(new Map("Game\\PacMan Map.dat"));

	m_PacmanMovement.reset(new Movement(m_Map.get(), 1, 6, DIR_NONE, PACMAN_STEP_TIME));
	
	// AABB mapy
	BOX MainBounds;
	engine::QMap *Map = res::g_Manager->MustGetResourceEx<engine::QMap>("PacMan_Map");
	Map->Load();
	Map->GetBoundingBox(&MainBounds);

	// Scena
	m_Scene.reset(new engine::Scene(MainBounds));
	engine::g_Engine->SetActiveScene(m_Scene.get());

	// Bloom
	m_Scene->SetPpBloom(new PpBloom(1.0f, 0.8f));

	// PpColor dla GAME_STATE_BEGIN
	m_Scene->SetPpColor(new PpColor(COLOR(0xFF000000)));

	// Kamera
	m_Camera = new Camera(m_Scene.get(),
		CalcPacmanWorldPos(), CAMERA_ANGLE_X, CAMERA_ANGLE_Y, CAMERA_DIST, 0.5f, 100.0f, PI_4, GetCurrentAspectRatio());
	m_Scene->SetActiveCamera(m_Camera);
	m_CameraSmoothPos.SmoothTime = CAMERA_SMOOTH_TIME;
	m_CameraSmoothPos.Set(m_Camera->GetPos(), VEC3::ZERO);

	// T³o
	engine::SolidSky *sky_obj = new engine::SolidSky(0xFF000040);
	m_Scene->SetSky(sky_obj);

	// Materia³y

	OpaqueMaterial *MapFloorMat = new OpaqueMaterial(m_Scene.get(), "MapFloor", "PacmanMapFloor");
	MapFloorMat->SetPerPixel(false);
	MapFloorMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);

	OpaqueMaterial *MapKlocekMat = new OpaqueMaterial(m_Scene.get(), "MapKlocek", "PacmanMapKlocek");
	MapKlocekMat->SetPerPixel(false);
	MapKlocekMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);

	TranslucentMaterial *GemMat = new TranslucentMaterial(m_Scene.get(), "Gem");
	GemMat->SetDiffuseColor(COLORF(0.2f, 0.5f, 0.5f, 1.0f));
	GemMat->SetTwoSided(true);
	GemMat->SetEnvironmentalTextureName("Gemstone");
	GemMat->SetFresnelColor(COLORF::WHITE);
	GemMat->SetFresnelPower(2.0f);

	OpaqueMaterial *PacmanMat = new OpaqueMaterial(m_Scene.get(), "Pacman", "PacmanTexture");
	PacmanMat->SetPerPixel(false);
	PacmanMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);

	TranslucentMaterial *GhostMaterial = new TranslucentMaterial(m_Scene.get(), "Ghost");
	GhostMaterial->SetColorMode(SolidMaterial::COLOR_MODULATE);
	GhostMaterial->SetDiffuseColor(COLORF::WHITE);
	GhostMaterial->SetFresnelColor(COLORF(0.5f, 1.0f, 1.0f, 1.0f));
	GhostMaterial->SetFresnelPower(1.0f);
	GhostMaterial->SetTwoSided(false);
	GhostMaterial->SetAlphaMode(TranslucentMaterial::ALPHA_MODULATE);
	GhostMaterial->SetBlendMode(BaseMaterial::BLEND_ADD);

	// Mapa
	m_Scene->SetQMap("PacMan_Map");
	
	// Œwiat³o
	m_Light = new DirectionalLight(m_Scene.get(), LIGHT_COLOR, LIGHT_DIR);
	m_Scene->SetAmbientColor(AMBIENT_COLOR);
	
	// Bohater
	m_Pacman = new QMeshEntity(m_Scene.get(), "PacmanModel");
	m_Pacman->SetPos(VEC3(15, 1, 15));
	m_Pacman->SetSize(0.9f);

	// Kryszta³ki
	for (uint y = 0; y < MAP_CY; y++)
	{
		for (uint x = 0; x < MAP_CX; x++)
		{
			if (m_Map->GetField(x, y) == FIELD_GEM)
			{
				QMeshEntity *e = new QMeshEntity(m_Scene.get(), "Gem");
				e->SetPos(VEC3(
					(x+0.5f)*2.0f,
					1.0f,
					(y+0.5f)*2.0f) );
				m_Gems.push_back(e);
				e->SetSize(0.9f);
				m_GemCoords.push_back(std::pair<uint, uint>(x, y));
			}
		}
	}

	// Duchy
	for (uint i = 0; i < GHOST_COUNT; i++)
	{
		m_Ghosts[i] = new QMeshEntity(m_Scene.get(), "GhostModel");
		m_Ghosts[i]->SetTeamColor(GHOST_TEAM_COLORS[i]);

		m_GhostMovements[i].reset(new Movement(m_Map.get(), GHOST_START_X[i], GHOST_START_Y[i], DIR_NONE, GHOST_STEP_TIME));
	}

	// Particles - efekt zbierania kryszta³ków
	m_CollectParticleDef.Zero();
	m_CollectParticleDef.CircleDegree = 0;
	m_CollectParticleDef.PosA2_C = VEC3(0.0f, -4.0f, 0.0f);
	m_CollectParticleDef.PosA1_C = VEC3(-2.0f, 0.0f, -2.0f);
	m_CollectParticleDef.PosA1_R = VEC3(4.0f, 0.0f, 4.0f);
	m_CollectParticleDef.PosA0_C = VEC3(0.0f, 2.0f, 0.0f);
	m_CollectParticleDef.ColorA0_C = VEC4(1.0f, 1.0f, 1.0f, 1.0f);
	m_CollectParticleDef.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, -1.0f);
	m_CollectParticleDef.OrientationA0_R = PI_X_2;
	m_CollectParticleDef.OrientationA1_C = -PI;
	m_CollectParticleDef.OrientationA1_R = PI_X_2;
	m_CollectParticleDef.SizeA0_C = 0.4f;
	m_CollectParticleDef.SizeA0_R = 0.4f;
	// To 1.1 to taki hack, inaczej z nieznanych przyczyn czasami czas siê zd¹¿y zawin¹æ i
	// miga znowu pocz¹tkowa klatka efektu zanim encja efektu zostanie zniszczona.
	m_CollectParticleDef.TimePeriod_C = GEM_COLLECT_PARTICLE_EFFECT_TIME * 1.1f;
}

GamePacman_pimpl::~GamePacman_pimpl()
{
	g_Engine->SetActiveScene(NULL);
	m_Scene.reset();
}

int GamePacman_pimpl::CalcFrame()
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

	////// Obrót kryszta³ków
	QUATERNION Quaternion;
	QuaternionRotationY(&Quaternion, t * GEM_ROT_SPEED);
	for (uint gi = 0; gi < m_Gems.size(); gi++)
	{
		m_Gems[gi]->SetOrientation(Quaternion);
		m_Gems[gi]->SetPos(VEC3(
			m_Gems[gi]->GetPos().x,
			WaveformSine(t+(float)m_GemCoords[gi].first-(float)m_GemCoords[gi].second, 1.0f, 0.5f, 0.3423f, 0.0f),
			m_Gems[gi]->GetPos().z ));
	}

	////// Efekty cz¹steczkowe
	for (uint i = m_CollectParticleEntityTimes.size(); i--; )
	{
		m_CollectParticleEntityTimes[i] -= dt / GEM_COLLECT_PARTICLE_EFFECT_TIME;
		if (m_CollectParticleEntityTimes[i] <= 0.0f)
		{
			delete m_CollectParticleEntities[i];
			m_CollectParticleEntities.erase(m_CollectParticleEntities.begin() + i);
			m_CollectParticleEntityTimes.erase(m_CollectParticleEntityTimes.begin() + i);
		}
	}

	// Obliczenia rozgrywki
	if (m_GameState == GAME_STATE_NONE || m_GameState == GAME_STATE_BEGIN)
	{
		////// Ruch pacmana
		// Idzie
		m_PacmanMovement->Move();
		// Klawisz
		m_PacmanMovement->Input(
			frame::GetKeyboardKey(VK_LEFT),
			frame::GetKeyboardKey(VK_RIGHT),
			frame::GetKeyboardKey(VK_DOWN),
			frame::GetKeyboardKey(VK_UP) );
		// Zbieranie kryszta³ków
		if (m_Map->GetField(m_PacmanMovement->Get_x(), m_PacmanMovement->Get_y()) == FIELD_GEM)
		{
			uint x = m_PacmanMovement->Get_x();
			uint y = m_PacmanMovement->Get_y();
			// Utwórz efekt cz¹steczkowy
			ParticleEntity *pe = new ParticleEntity(
				m_Scene.get(),
				"GemParticleTexture",
				BaseMaterial::BLEND_LERP,
				m_CollectParticleDef,
				10,
				3.0f);
			pe->SetPos(VEC3(m_PacmanMovement->CalcWorldX(), 1.0f, m_PacmanMovement->CalcWorldZ()));
			m_CollectParticleEntities.push_back(pe);
			m_CollectParticleEntityTimes.push_back(1.0f);
			// Usuñ kryszta³ek
			m_Map->CollectGem(x, y);
			// Usuñ encjê kryszta³ka
			for (uint i = 0; i < m_GemCoords.size(); i++)
			{
				assert(m_Gems.size() == m_GemCoords.size());
				if (m_GemCoords[i].first == x && m_GemCoords[i].second == y)
				{
					delete m_Gems[i];
					m_Gems.erase(m_Gems.begin() + i);
					m_GemCoords.erase(m_GemCoords.begin() + i);
					break;
				}
			}
			// Wygrana
			if (m_Gems.empty())
				Win();
		}

		////// Pacman i Kamera
		// Pozycja pacmana
		VEC3 PacmanPos = CalcPacmanWorldPos();
		m_Pacman->SetPos(PacmanPos);
		// Pozycja kamery
		m_CameraSmoothPos.Update(PacmanPos, dt);
		m_Camera->SetPos(m_CameraSmoothPos.Pos);
		// Orientajca pacmana
		if (m_PacmanMovement->GetDir() != DIR_NONE)
		{
			QUATERNION PacmanQuaternion;
			QuaternionRotationY(&PacmanQuaternion, PI - DirToAngle(m_PacmanMovement->GetDir()));
			m_Pacman->SetOrientation(PacmanQuaternion);
		}

		////// Duszki
		for (uint i = 0; i < GHOST_COUNT; i++)
		{
			// Ruch
			m_GhostMovements[i]->Move();
			// AI
			bool KeyLeft = false, KeyRight = false, KeyDown = false, KeyUp = false;
			if (m_GhostMovements[i]->GetDir() == DIR_NONE)
			{
				switch (g_Rand.RandUint(5))
				{
				case 0: KeyLeft  = true; break;
				case 1: KeyRight = true; break;
				case 2: KeyDown  = true; break;
				case 3: KeyUp    = true; break;
				}
			}
			m_GhostMovements[i]->Input(KeyLeft, KeyRight, KeyDown, KeyUp);
			// Przegrana
			if (m_GhostMovements[i]->Get_x() == m_PacmanMovement->Get_x() &&
				m_GhostMovements[i]->Get_y() == m_PacmanMovement->Get_y())
			{
				Lose();
			}
			// Pozycja modelu
			VEC3 GhostPos = VEC3(m_GhostMovements[i]->CalcWorldX(), 1.0f, m_GhostMovements[i]->CalcWorldZ());
			m_Ghosts[i]->SetPos(GhostPos);
			// Orientacja modelu
			if (m_GhostMovements[i]->GetDir() != DIR_NONE)
			{
				QUATERNION GhostQuaternion;
				QuaternionRotationY(&GhostQuaternion, -DirToAngle(m_GhostMovements[i]->GetDir()));
				m_Ghosts[i]->SetOrientation(GhostQuaternion);
			}
		}
	}

	return 0;
}

void GamePacman_pimpl::OnResolutionChanged()
{
	m_Camera->SetAspect(GetCurrentAspectRatio());
}

float GamePacman_pimpl::GetCurrentAspectRatio()
{
	return (float)frame::Settings.BackBufferWidth / (float)frame::Settings.BackBufferHeight;
}

VEC3 GamePacman_pimpl::CalcPacmanWorldPos()
{
	return VEC3(
		m_PacmanMovement->CalcWorldX(),
		1.0f,
		m_PacmanMovement->CalcWorldZ());
}

void GamePacman_pimpl::Win()
{
	LOG(LOG_GAME, "Win");

	m_GameState = GAME_STATE_WIN;
	m_GameStateTime = 0.0f;

	m_Scene->SetPpColor(new PpColor(COLOR(0x00000000u)));
}

void GamePacman_pimpl::Lose()
{
	LOG(LOG_GAME, "Loss");

	m_GameState = GAME_STATE_LOSE;
	m_GameStateTime = 0.0f;

	m_Scene->SetPpBloom(NULL); // Wy³¹czyæ PpBloom, bo Ÿle wspó³pracuje z PpFunction
	m_Scene->SetPpFunction(new PpFunction(VEC3::ONE, VEC3::ZERO, 0.0f));
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GamePacman

GamePacman::GamePacman() :
	pimpl(new GamePacman_pimpl)
{
}

GamePacman::~GamePacman()
{
}

int GamePacman::CalcFrame()
{
	return pimpl->CalcFrame();
}

void GamePacman::OnResolutionChanged()
{
	pimpl->OnResolutionChanged();
}

engine::Scene * GamePacman::GetScene()
{
	return pimpl->GetScene();
}
