/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\ResMngr.hpp"
#include "..\Framework\Gfx2D.hpp"
#include "..\Engine\Engine.hpp"
#include "..\Engine\Entities.hpp"
#include "..\Engine\Engine_pp.hpp"
#include "..\Engine\Sky.hpp"
#include "..\Engine\Water.hpp"
#include "..\Engine\Terrain.hpp"
#include "..\Engine\Grass.hpp"
#include "..\Engine\Fall.hpp"
#include "GameRpg.hpp"

using namespace engine;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy globalne

#define SOLVE_LINEAR_A(x1, y1, x2, y2) ( ( (y2) - (y1) ) / ( (x2) - (x1) ) )
#define SOLVE_LINEAR_B(x1, y1, x2, y2) ( ( (x2)*(y1) - (y2)*(x1)  ) / ( (x2) - (x1) ) )

static const float PERIODIC_CALC_PERIOD = 0.05f;

static float BEGIN_TIME;
static float WIN_TIME;
static float LOSE_TIME;

static float CAMERA_DIST;
static float CAMERA_RIGHT;
static float CAMERA_ANGLE_SMOOTH_TIME;
static VEC3 PLAYER_START_POS;
static float PLAYER_MOVE_SPEED;
static float PLAYER_ROT_SPEED;
static float PLAYER_MOVE_SMOOTH_TIME;
static COLORF AMBIENT_COLOR;
static COLORF LIGHT_COLOR;
static COLORF DARK_LIGHT_COLOR;
static VEC3 LIGHT_DIR;
static float DAY_TIME;
static VEC2 WIND;
static VEC2 DARK_WIND;
static VEC2 CAMPFIRE_POS;
static float WORM_COUNT;
static float WORM_RADIUS;
static float WORM_SIZE;
static float WORM_SPEED;
static string WORM_TEXT;
static COLOR WORM_TEXT_COLOR;
static float WORM_TEXT_SIZE;
static VEC2 NPC_POS;
static uint1 WATER_HEIGHT;
static float TIME_TO_WIN;
static float DISTANCE_TO_WIN_SQ;
static string WIN_TEXT;
static float WIN_TEXT_SIZE;

const FALL_EFFECT_DESC RAIN_DESC = FALL_EFFECT_DESC(
	COLORF(0.7f, 0.6f, 0.6f, 0.8f), // MainColor
	"RainDrop", // ParticleTextureName
	false, // UseRealUpDir
	VEC2(0.003f, 0.02f), // ParticleHalfSize
	VEC3(0.0f, -0.8f, 0.0f), // MovementVec1
	VEC3(-0.0f, -0.8f, -0.0f), // MovementVec2
	"RainPlane", // PlaneTextureName
	VEC2(100.0f / 7.0f, 1.0f), // PlaneTexScale
	COLORF(0.5f, 0.6f, 0.6f, 0.8f)); // NoiseColor
const FALL_EFFECT_DESC SNOW_DESC = FALL_EFFECT_DESC(
	COLORF(0.7f, 1.0f, 1.0f, 1.0f), // MainColor
	"SnowFlake", // ParticleTextureName
	true, // UseRealUpDir
	VEC2(0.005f, 0.005f), // ParticleHalfSize
	VEC3(0.1f, -0.3f, 0.1f), // MovementVec1
	VEC3(-0.1f, -0.2f, -0.1f), // MovementVec2
	"SnowPlane", // PlaneTextureName
	VEC2(500.0f / 7.0f, 5.0f), // PlaneTexScale
	COLORF(0.5f, 1.0f, 1.0f, 1.0f)); // NoiseColor


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GameRpg_pimpl

class GameRpg_pimpl
{
public:
	GameRpg_pimpl();
	~GameRpg_pimpl();
	int CalcFrame();
	void Draw2D();
	void OnResolutionChanged();
	void OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	void OnMouseMove(const common::VEC2 &Pos);
	Scene * GetScene() { return m_Scene.get(); }

private:
	// Bie¿¹cy stan rozgrywki
	enum GAME_STATE
	{
		GAME_STATE_NONE,
		GAME_STATE_BEGIN, // Istnieje w scenie PpColor
		GAME_STATE_WIN,   // Istnieje w scenie PpFunction
		GAME_STATE_LOSE,  // Istnieje w scenie PpColor
	};

	enum FALL_MODE
	{
		FALL_MODE_NONE,
		FALL_MODE_RAIN,
		FALL_MODE_SNOW,
	};

	scoped_ptr<Scene> m_Scene;
	// Zwolni go scena
	Camera *m_Camera;
	SmoothCD_obj<VEC3> m_PlayerPos;
	SmoothCD_obj<VEC2> m_CameraAngles;
	// Zwolni go scena
	DirectionalLight *m_Light;
	// WskaŸnik pobrany z zasobów
	Terrain *m_TerrainObj;
	float m_LastPeriodicCalcTime;
	// Bie¿acy stan wyœwietlanych opadów.
	// Odzwierciedla opady zainstalowane obecnie w scenie.
	FALL_MODE m_FallMode;
	ComplexSky *m_SkyObj;
	std::vector<QMeshEntity*> m_Worms;
	std::vector<TextEntity*> m_WormTexts;
	QMeshEntity *m_Exclamation;
	QMeshEntity *m_MainCharacter;
	bool m_Running;
	// Czas spêdzony w polu wygranej
	float m_TimeToWin;
	// True, jeœli m_TimeToWin zosta³ zmniejszony
	bool m_Winning;
	UtermStripeEntity *m_Stripes[2];

	GAME_STATE m_GameState;
	// Czas bie¿acego stanu, dope³nia siê 0..1
	float m_GameStateTime;

	float GetCurrentAspectRatio();
	void Win();
	void Lose();
	void CreateMaterials();
	void CreateCampfire();
	void CreateNPC();
	// Obliczenia wywo³ywane nie co klatkê, chocia¿ te¿ czêsto - co PERIODIC_CALC_PERIOD sekund.
	void PeriodicCalc();
	// Aktualizuje kolor œwiat³a itp. parametry na podstawie podanej intensywnoœci opadów
	void UpdateWeatherParams(float Intensity);
};


GameRpg_pimpl::GameRpg_pimpl() :
	m_Camera(NULL),
	m_Light(NULL),
	m_TerrainObj(NULL),
	m_LastPeriodicCalcTime(frame::Timer2.GetTime() - PERIODIC_CALC_PERIOD),
	m_FallMode(FALL_MODE_NONE),
	m_SkyObj(NULL),
	m_Exclamation(NULL),
	m_MainCharacter(NULL),
	m_Running(false),
	m_Winning(false)
{
	m_GameState = GAME_STATE_BEGIN;
	m_GameStateTime = 0.0f;
	m_Stripes[0] = m_Stripes[1] = NULL;

	// Wczytaj konfiguracjê

	g_Config->MustGetDataEx("Game/BeginTime", &BEGIN_TIME);
	g_Config->MustGetDataEx("Game/WinTime", &WIN_TIME);
	g_Config->MustGetDataEx("Game/LoseTime", &LOSE_TIME);

	g_Config->MustGetDataEx("Game/RPG/CameraDist", &CAMERA_DIST);
	g_Config->MustGetDataEx("Game/RPG/CameraRight", &CAMERA_RIGHT);
	g_Config->MustGetDataEx("Game/RPG/PlayerStartPos", &PLAYER_START_POS);
	g_Config->MustGetDataEx("Game/RPG/CameraAngleSmoothTime", &m_CameraAngles.SmoothTime);
	g_Config->MustGetDataEx("Game/RPG/PlayerMoveSmoothTime", &PLAYER_MOVE_SMOOTH_TIME);
	g_Config->MustGetDataEx("Game/RPG/PlayerMoveSpeed", &PLAYER_MOVE_SPEED);
	g_Config->MustGetDataEx("Game/RPG/PlayerRotSpeed", &PLAYER_ROT_SPEED);
	g_Config->MustGetDataEx("Game/RPG/AmbientColor", &AMBIENT_COLOR);
	g_Config->MustGetDataEx("Game/RPG/LightColor", &LIGHT_COLOR);
	g_Config->MustGetDataEx("Game/RPG/DarkLightColor", &DARK_LIGHT_COLOR);
	g_Config->MustGetDataEx("Game/RPG/LightDir", &LIGHT_DIR);
	g_Config->MustGetDataEx("Game/RPG/DayTime", &DAY_TIME);
	g_Config->MustGetDataEx("Game/RPG/Wind", &WIND);
	g_Config->MustGetDataEx("Game/RPG/DarkWind", &DARK_WIND);
	g_Config->MustGetDataEx("Game/RPG/CampfirePos", &CAMPFIRE_POS);
	g_Config->MustGetDataEx("Game/RPG/WormCount", &WORM_COUNT);
	g_Config->MustGetDataEx("Game/RPG/WormRadius", &WORM_RADIUS);
	g_Config->MustGetDataEx("Game/RPG/WormSize", &WORM_SIZE);
	g_Config->MustGetDataEx("Game/RPG/WormSpeed", &WORM_SPEED);
	g_Config->MustGetDataEx("Game/RPG/WormText", &WORM_TEXT);
	g_Config->MustGetDataEx("Game/RPG/WormTextColor", &WORM_TEXT_COLOR);
	g_Config->MustGetDataEx("Game/RPG/WormTextSize", &WORM_TEXT_SIZE);
	g_Config->MustGetDataEx("Game/RPG/NPCPos", &NPC_POS);
	g_Config->MustGetDataEx("Game/RPG/WaterHeight", &WATER_HEIGHT);
	g_Config->MustGetDataEx("Game/RPG/TimeToWin", &TIME_TO_WIN);
	g_Config->MustGetDataEx("Game/RPG/DistanceToWin", &DISTANCE_TO_WIN_SQ);
	g_Config->MustGetDataEx("Game/RPG/WinText", &WIN_TEXT);
	g_Config->MustGetDataEx("Game/RPG/WinTextSize", &WIN_TEXT_SIZE);
	DISTANCE_TO_WIN_SQ *= DISTANCE_TO_WIN_SQ;

	Normalize(&LIGHT_DIR);
	m_CameraAngles.Set(VEC2(0.0f, PI), VEC2::ZERO);
	m_TimeToWin = TIME_TO_WIN;

	// Teren
	m_TerrainObj = res::g_Manager->MustGetResourceEx<Terrain>("RpgTerrain");
	m_TerrainObj->Lock();

	// AABB sceny
	BOX MainBounds;
	m_TerrainObj->GetBoundingBox(&MainBounds);

	// Scena
	m_Scene.reset(new engine::Scene(MainBounds));
	engine::g_Engine->SetActiveScene(m_Scene.get());

	// Wiatr ustawi UpdateWeatherParams.

	// Niebo
	ERR_TRY;
	{
		FileStream fs("Game\\Terrain\\RPG Sky.dat", FM_READ);
		Tokenizer t(&fs, 0);
		t.Next();
		m_SkyObj = new engine::ComplexSky(m_Scene.get(), t);
		m_SkyObj ->SetTime(DAY_TIME);
		m_Scene->SetSky(m_SkyObj);
		t.AssertEOF();
	}
	ERR_CATCH("Nie mo¿na wczytaæ nieba.");

	// Lens Flare
	m_Scene->SetPpLensFlare(new PpLensFlare(m_SkyObj->GetCurrentSunDir()));

	// Mg³a
	m_Scene->SetFogEnabled(true);
	m_Scene->SetFogColor(m_SkyObj->GetCurrentHorizonColor());
	m_Scene->SetFogStart(0.75f);

	// Bloom, Tone Mapping
	m_Scene->SetPpBloom(new PpBloom(0.5f, 0.7f));
	m_Scene->SetPpToneMapping(new PpToneMapping(1.0f));

	// PpColor dla GAME_STATE_BEGIN
	m_Scene->SetPpColor(new PpColor(COLOR(0xFF000000)));

	// Kamera
	m_PlayerPos.Set(PLAYER_START_POS, VEC3::ZERO);
	m_PlayerPos.SmoothTime = PLAYER_MOVE_SMOOTH_TIME;
	m_Camera = new Camera(m_Scene.get(),
		PLAYER_START_POS, m_CameraAngles.Pos.x, m_CameraAngles.Pos.y, CAMERA_DIST, 0.5f, 60.0f, PI_4, GetCurrentAspectRatio());
	m_Scene->SetActiveCamera(m_Camera);

	// Materia³y
	CreateMaterials();

	// Teren

	TerrainWater *WaterObj = new TerrainWater(m_Scene.get(), m_TerrainObj, m_TerrainObj->ValueToHeight(WATER_HEIGHT));
	WaterObj->SetDirToLight(-LIGHT_DIR);

	Grass *GrassObj = new Grass(m_Scene.get(), m_TerrainObj,
		"Game\\Terrain\\GrassDesc.dat",
		"Game\\Terrain\\RPG Grass Density Map.tga");

	m_Scene->SetTerrain(new TerrainRenderer(
		m_Scene.get(),
		m_TerrainObj,
		GrassObj,
		WaterObj,
		"Game\\Terrain\\TreeDesc.dat",
		"Game\\Terrain\\RPG Tree Density Map.tga"));

	////// Œwiat³a

	m_Scene->SetAmbientColor(AMBIENT_COLOR);

	m_Light = new DirectionalLight(m_Scene.get(), LIGHT_COLOR, LIGHT_DIR);
	m_Light->SetCastShadow(true);
	m_Light->SetZFar(20.0f);
	m_Light->SetShadowFactor(0.5f);

	// Ognisko
	CreateCampfire();
	
	// NPC
	CreateNPC();

	// Bohater
	m_MainCharacter = new QMeshEntity(m_Scene.get(), "TinyMesh");
	m_MainCharacter->SetAnimation("Stand", QMeshEntity::ANIM_MODE_LOOP);
	m_MainCharacter->SetReceiveShadow(false);

	// Obliczenia pocz¹tkowe

	UpdateWeatherParams(0.0f);
}

GameRpg_pimpl::~GameRpg_pimpl()
{
	if (m_TerrainObj)
		m_TerrainObj->Unlock();

	g_Engine->SetActiveScene(NULL);
	m_Scene.reset();
}

int GameRpg_pimpl::CalcFrame()
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

	// Poruszanie siê robaków
	if (m_Worms.size() == WORM_COUNT && m_WormTexts.size() == WORM_COUNT) // Na wszelki wypadek.
	{
		float a = t * WORM_SPEED, a_step = PI_X_2 / (float)WORM_COUNT;
		for (uint i = 0; i < WORM_COUNT; i++)
		{
			VEC3 Pos;
			QUATERNION Quat;
			Pos.x = CAMPFIRE_POS.x + cosf(a) * WORM_RADIUS;
			Pos.z = CAMPFIRE_POS.y + sinf(a) * WORM_RADIUS;
			Pos.y = m_TerrainObj->GetHeight(Pos.x, Pos.z) + WORM_SIZE;
			QuaternionRotationY(&Quat, - a + PI_2);

			m_Worms[i]->SetPos(Pos);
			m_Worms[i]->SetOrientation(Quat);

			Pos.y += WORM_SIZE + WORM_SIZE;
			m_WormTexts[i]->SetPos(Pos);

			a += a_step;
		}
	}

	if (m_LastPeriodicCalcTime + PERIODIC_CALC_PERIOD < t)
	{
		PeriodicCalc();
		m_LastPeriodicCalcTime = t;
	}

	if (m_GameState == GAME_STATE_NONE || m_GameState == GAME_STATE_BEGIN)
	{

		////// Chodzenie
		float Forward = 0.0f, Right = 0.0f;
		if (frame::GetKeyboardKey('W')) Forward += 1.0f;
		if (frame::GetKeyboardKey('S')) Forward -= 1.0f;
		if (frame::GetKeyboardKey('A')) Right -= 1.0f;
		if (frame::GetKeyboardKey('D')) Right += 1.0f;
#ifdef _DEBUG
		if (frame::GetKeyboardKey(VK_SHIFT)) { Forward *= 5.0f; Right *= 5.0f; }
#endif

		// Animacja postaci
		if (Forward != 0.0f)
		{
			if (!m_Running)
			{
				m_MainCharacter->BlendAnimation("Run", 0.5f, QMeshEntity::ANIM_MODE_LOOP);
				m_Running = true;
			}
		}
		else
		{
			if (m_Running)
			{
				m_MainCharacter->BlendAnimation("Stand", 0.5f, QMeshEntity::ANIM_MODE_LOOP);
				m_Running = false;
			}
		}

		VEC3 NewPos = m_PlayerPos.Dest;
		if (Forward != 0.0f)
		{
			float Angle = - m_Camera->GetAngleY() + PI_2;
			VEC3 ForwardDir = VEC3(cosf(Angle), 0.0f, sinf(Angle));
			NewPos += ForwardDir * Forward * PLAYER_MOVE_SPEED * dt;
		}
		if (Right != 0.0f)
		{
			float Angle = - m_Camera->GetAngleY();
			VEC3 RightDir = VEC3(cosf(Angle), 0.0f, sinf(Angle));
			NewPos += RightDir * Right * PLAYER_MOVE_SPEED * dt;
		}

		if (NewPos.x < 0.0f ||
			NewPos.z < 0.0f ||
			NewPos.x >= m_TerrainObj->GetCX() * m_TerrainObj->GetVertexDistance() ||
			NewPos.z >= m_TerrainObj->GetCZ() * m_TerrainObj->GetVertexDistance() ||
			m_TerrainObj->GetHeight(NewPos.x, NewPos.z) <= m_TerrainObj->ValueToHeight(WATER_HEIGHT))
		{
			// Wróæ do oryginalnej pozycji
			NewPos = m_PlayerPos.Dest;
		}

		NewPos.y = m_TerrainObj->GetHeight(NewPos.x, NewPos.z) + 2.0f;

		m_PlayerPos.Update(NewPos, dt);

		// Model bohatera
		QUATERNION MainCharQ;
		EulerAnglesToQuaternionO2I(&MainCharQ, m_Camera->GetAngleY() + PI, 0.0f, 0.0f);
		m_MainCharacter->SetPos(m_PlayerPos.Pos + VEC3(0.0f, -1.0f, 0.0f));
		m_MainCharacter->SetOrientation(MainCharQ);

		// Kamera
		m_CameraAngles.Update(m_CameraAngles.Dest, dt);
		m_Camera->SetAngleX(m_CameraAngles.Pos.x);
		m_Camera->SetAngleY(m_CameraAngles.Pos.y);

		m_Camera->SetCameraDist(std::min(CAMERA_DIST, CAMERA_DIST * (PI_2 + m_Camera->GetAngleX())));

		m_Camera->SetPos(m_PlayerPos.Pos + m_Camera->GetParams().GetRightDir() * CAMERA_RIGHT);

		// Wygrywanie
		if (DistanceSq(VEC2(m_PlayerPos.Pos.x, m_PlayerPos.Pos.z), NPC_POS) <= DISTANCE_TO_WIN_SQ)
			m_Winning = true;
		if (m_Winning)
		{
			m_Exclamation->SetVisible(false);

			m_TimeToWin -= dt;
			if (m_TimeToWin <= 0.0f)
				Win();
		}
	}

	return 0;
}

void GameRpg_pimpl::Draw2D()
{
	if (m_Winning)
	{
		gfx2d::g_Canvas->SetFont(res::g_Manager->MustGetResourceExf<res::Font>("Font02"));
		gfx2d::g_Canvas->SetColor(COLOR::LIME);
		gfx2d::g_Canvas->PushAlpha(minmax(0.0f, (1.0f - (m_TimeToWin / TIME_TO_WIN)) * 2.0f, 1.0f));
		gfx2d::g_Canvas->DrawText_(
			frame::GetScreenWidth() * 0.5f,
			frame::GetScreenHeight() * 0.3333f,
			WIN_TEXT, WIN_TEXT_SIZE,
			res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_SINGLELINE, 0.0f);
		gfx2d::g_Canvas->PopAlpha();
	}
}

void GameRpg_pimpl::OnResolutionChanged()
{
	m_Camera->SetAspect(GetCurrentAspectRatio());
}

void GameRpg_pimpl::OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Action == frame::MOUSE_DOWN || Action == frame::MOUSE_DBLCLK)
	{
		ShowCursor(FALSE);
		frame::SetMouseMode(true);
	}
	else // MOUSE_UP
	{
		frame::SetMouseMode(false);
		ShowCursor(TRUE);
	}
}

void GameRpg_pimpl::OnMouseMove(const common::VEC2 &Pos)
{
	if (frame::GetMouseMode())
	{
		m_CameraAngles.Dest = VEC2(
			minmax(-PI_2, m_CameraAngles.Dest.x + Pos.y * PLAYER_ROT_SPEED, PI_2),
			m_CameraAngles.Dest.y + Pos.x * PLAYER_ROT_SPEED);
		//m_Camera->SetAngleX(m_Camera->GetAngleX() + Pos.y * PLAYER_ROT_SPEED);
		//m_Camera->SetAngleY(m_Camera->GetAngleY() + Pos.x * PLAYER_ROT_SPEED);
	}
}

float GameRpg_pimpl::GetCurrentAspectRatio()
{
	return (float)frame::Settings.BackBufferWidth / (float)frame::Settings.BackBufferHeight;
}

void GameRpg_pimpl::Win()
{
	LOG(LOG_GAME, "Win");

	m_GameState = GAME_STATE_WIN;
	m_GameStateTime = 0.0f;

	m_Scene->SetPpColor(new PpColor(COLOR(0x00000000u)));
}

void GameRpg_pimpl::Lose()
{
	LOG(LOG_GAME, "Loss");

	m_GameState = GAME_STATE_LOSE;
	m_GameStateTime = 0.0f;

	m_Scene->SetPpBloom(NULL); // Wy³¹czyæ PpBloom, bo Ÿle wspó³pracuje z PpFunction
	m_Scene->SetPpFunction(new PpFunction(VEC3::ONE, VEC3::ZERO, 0.0f));
}

void GameRpg_pimpl::CreateMaterials()
{
	// Wood - do ogniska
	OpaqueMaterial *CampfireMat = new OpaqueMaterial(m_Scene.get(), "Wood", "Wood");
	CampfireMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	CampfireMat->SetPerPixel(false);

	// Worm
	OpaqueMaterial *WormMat = new OpaqueMaterial(m_Scene.get(), "Worm", "WormTexture");
	WormMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	WormMat->SetPerPixel(false);

	// Bohater
	OpaqueMaterial *MainCharacterMat = new OpaqueMaterial(m_Scene.get(), "Tiny", "TinyTexture");
	MainCharacterMat->SetPerPixel(true);
	MainCharacterMat->SetSpecularMode(OpaqueMaterial::SPECULAR_ANISO_TANGENT);
	MainCharacterMat->SetGlossMapping(true);
	MainCharacterMat->SetSpecularColor(COLORF(0.4f, 0.4f, 0.4f));

	// Dwarf
	OpaqueMaterial *DwarfMat;
	DwarfMat = new OpaqueMaterial(m_Scene.get(), "DwarfArmor", "DwarfArmor");
	DwarfMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	DwarfMat->SetPerPixel(false);
	DwarfMat = new OpaqueMaterial(m_Scene.get(), "DwarfBeard", "DwarfBeard");
	DwarfMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	DwarfMat->SetPerPixel(false);
	DwarfMat = new OpaqueMaterial(m_Scene.get(), "DwarfBody", "DwarfBody");
	DwarfMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	DwarfMat->SetPerPixel(false);
	DwarfMat = new OpaqueMaterial(m_Scene.get(), "DwarfHead", "DwarfHead");
	DwarfMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	DwarfMat->SetPerPixel(false);
	DwarfMat = new OpaqueMaterial(m_Scene.get(), "DwarfWeapon", "DwarfWeapon");
	DwarfMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	DwarfMat->SetPerPixel(false);

	// Wykrzyknik
	OpaqueMaterial *ExclamationMat = new OpaqueMaterial(m_Scene.get(), "Exclamation");
	ExclamationMat->SetPerPixel(false);
	ExclamationMat->SetDiffuseColor(COLORF::YELLOW);
	ExclamationMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);

	TranslucentMaterial *ExclamationTransparentMat = new TranslucentMaterial(m_Scene.get(), "ExclamationTrans");
	ExclamationTransparentMat->SetDiffuseColor(COLORF(0.5f, 1.0f, 1.0f, 0.0f));
	ExclamationTransparentMat->SetBlendMode(BaseMaterial::BLEND_ADD);

	// Wst¹¿ka
	TranslucentMaterial *StripeMat = new TranslucentMaterial(m_Scene.get(), "Stripe",
		"LaserStripeTexture",
		BaseMaterial::BLEND_ADD,
		SolidMaterial::COLOR_MODULATE,
		TranslucentMaterial::ALPHA_MODULATE);
	StripeMat->SetTwoSided(true);

	TranslucentMaterial *RainbowStripeMat = new TranslucentMaterial(m_Scene.get(), "RainbowStripe",
		"RainbowTexture",
		BaseMaterial::BLEND_ADD,
		SolidMaterial::COLOR_MODULATE,
		TranslucentMaterial::ALPHA_MODULATE);
	RainbowStripeMat->SetTwoSided(true);

}

void GameRpg_pimpl::CreateCampfire()
{
	VEC3 Pos = VEC3(CAMPFIRE_POS.x, m_TerrainObj->GetHeight(CAMPFIRE_POS.x, CAMPFIRE_POS.y) + 0.2f, CAMPFIRE_POS.y);

	QMeshEntity *Campfire = new QMeshEntity(m_Scene.get(), "Campfire");
	Campfire->SetPos(Pos);
	Campfire->SetCastShadow(false);
	Campfire->SetReceiveShadow(false);

	// Ogieñ
	PARTICLE_DEF Def3;
	Def3.Zero();
	Def3.CircleDegree = 1;
	Def3.PosA0_R = VEC3(0.3f, 0.0f, PI_X_2);
	Def3.PosA1_C = VEC3(-0.2f, 0.5f, -0.2f);
	Def3.PosA1_R = VEC3(0.4f, 1.5f, 0.4f);
	Def3.ColorA0_C = VEC4(1.0f, 1.0f, 1.0f, 1.0f);
	Def3.ColorA0_R = VEC4(0.0f, 0.0f, 0.0f, 0.0f);
	Def3.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, -1.0f);
	Def3.SizeA0_C = 0.1f;
	Def3.SizeA0_R = 0.4f;
	Def3.TimePeriod_C = 1.0f;
	Def3.TimePhase_R = 2.346f;
	Def3.OrientationA1_R = 1.0f;
	ParticleEntity *particle_entity3 = new ParticleEntity(m_Scene.get(), "FireParticle", BaseMaterial::BLEND_ADD, Def3, 300, 3.0f);
	particle_entity3->SetPos(Pos);

	// Heat Haze od ognia
	HeatSphere *heat_sphere = new HeatSphere(m_Scene.get(), VEC3(1.f, 1.f, 1.f));
	heat_sphere->SetPos(Pos + VEC3(0.f, 2.0f, 0.f));

	// Dym
	PARTICLE_DEF Def4;
	Def4.Zero();
	Def4.CircleDegree = 1;
	Def4.PosA0_C = VEC3(0.0f, -0.1f, 0.0f);
	Def4.PosA0_R = VEC3(0.5f, 0.2f, PI_X_2);
	Def4.PosA1_C = VEC3(-0.2f, 0.5f, -0.2f);
	Def4.PosA1_R = VEC3(0.4f, 1.0f, 0.4f);
	Def4.ColorA0_C = VEC4(1.0f, 1.0f, 1.0f, 0.0f);
	Def4.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, 2.0f);
	Def4.ColorA2_C = VEC4(0.0f, 0.0f, 0.0f, -2.0f);
	Def4.SizeA0_C = 0.2f;
	Def4.SizeA0_R = 0.4f;
	Def4.TimePeriod_C = 1.0f;
	Def4.TimePhase_R = 2.346f;
	Def4.OrientationA1_R = 1.0f;
	ParticleEntity *particle_entity4 = new ParticleEntity(m_Scene.get(), "SmokeParticle", BaseMaterial::BLEND_SUB, Def4, 30, 3.0f);
	particle_entity4->SetPos(Pos + VEC3(0.0f, 1.5f, 0.0f));

	// Robaki
	float a = 0.0f, a_step = PI_X_2 / (float)WORM_COUNT;
	for (uint i = 0; i < WORM_COUNT; i++)
	{
		QMeshEntity *WormEntity = new QMeshEntity(m_Scene.get(), "WormMesh");
		m_Worms.push_back(WormEntity);
		WormEntity->SetSize(WORM_SIZE);
		WormEntity->SetAnimation("Walk", QMeshEntity::ANIM_MODE_LOOP);
		WormEntity->SetCastShadow(true);
		WormEntity->SetReceiveShadow(true);

		TextEntity *Txt = new TextEntity(m_Scene.get(), WORM_TEXT, "Font02", WORM_TEXT_COLOR, WORM_TEXT_SIZE,
			res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE | res::Font::FLAG_WRAP_NORMAL);
		m_WormTexts.push_back(Txt);
		Txt->SetBlendMode(BaseMaterial::BLEND_LERP);
		Txt->SetDegreesOfFreedom(2);

		// Pozycjê i orientacjê ustawi CalcFrame
		a += a_step;
	}
}

void GameRpg_pimpl::CreateNPC()
{
	// Model
	QMeshEntity *NPC = new QMeshEntity(m_Scene.get(), "DwarfMesh");
	VEC3 NPCPos;
	QUATERNION NPCQuat;
	NPCPos.x = NPC_POS.x;
	NPCPos.z = NPC_POS.y;
	NPCPos.y = m_TerrainObj->GetHeight(NPCPos.x, NPCPos.z) + 1.0f;
	QuaternionRotationY(&NPCQuat, PI);
	NPC->SetPos(NPCPos);
	NPC->SetOrientation(NPCQuat);
	NPC->SetReceiveShadow(false);

	// Wykrzyknik nad g³ow¹
	m_Exclamation = new QMeshEntity(m_Scene.get(), "ExclamationMesh");
	NPCPos.y += 1.2f;
	m_Exclamation->SetPos(NPCPos);
	m_Exclamation->SetCastShadow(false);
	m_Exclamation->SetReceiveShadow(false);
	m_Exclamation->SetSize(0.2f);

	// Efekty wst¹¿kowe i cz¹steczkowe
	
	PARTICLE_DEF ParticleDef;
	ParticleDef.Zero();
	ParticleDef.CircleDegree = 1;
	ParticleDef.PosA0_C = VEC3( 0.0f, -2.0f, 0.0f);
	ParticleDef.PosA0_R = VEC3( 0.5f, -2.0f, PI_X_2);
	ParticleDef.PosA2_C = VEC3( 0.0f,  4.0f,  0.0f);
	ParticleDef.PosA1_C = VEC3( 0.0f,  0.0f,  0.0f);
	ParticleDef.PosA1_R = VEC3( 0.0f,  0.0f,  0.0f);
	ParticleDef.PosA0_C.y = -1.8f;
	ParticleDef.TimePeriod_C = 10.0f;
	ParticleDef.TimePhase_P = 1.0f;
	ParticleDef.SizeA0_C = 0.5f;
	ParticleDef.ColorA0_C = VEC4(0.0f, 0.0f, 0.0f,  1.0f);
	ParticleDef.ColorA0_R = VEC4(1.0f, 1.0f, 1.0f,  0.0f);
	ParticleDef.ColorA1_R = VEC4(0.5f, 0.5f, 0.5f,  0.0f);
	ParticleDef.ColorA2_C = VEC4(0.0f, 0.0f, 0.0f, -1.0f);

	ParticleEntity *particle_entity;
	VEC3 Pos;

	Pos = NPCPos;
	Pos.x -= 4.0f;
	Pos.z -= 2.0f;
	Pos.y = m_TerrainObj->GetHeight(Pos.x, Pos.z) + 2.0f;
	m_Stripes[0] = new UtermStripeEntity(m_Scene.get(), "Stripe", 64, Uterm3(
		VEC3(0, 4, 0),
		VEC3(0, 0, 0),
		VEC3(0, -2, 0),
		VEC3(0.5f, 0, 0.5f),
		VEC3(20.f, 0, -20.f),
		VEC3(0, 0, PI_2)));
	m_Stripes[0]->SetPos(Pos);
	m_Stripes[0]->SetTex(RECTF(0.0f, 0.0f, 1.0f, 10.0f));
	m_Stripes[0]->SetTeamColor(COLORF(0.5f, 0.25f, 0.99f, 0.75f));

	particle_entity = new ParticleEntity(m_Scene.get(), "MainParticle", BaseMaterial::BLEND_LERP, ParticleDef, 50, 4.0f);
	particle_entity->SetPos(Pos);

	Pos = NPCPos;
	Pos.x += 4.0f;
	Pos.z -= 2.0f;
	Pos.y = m_TerrainObj->GetHeight(Pos.x, Pos.z) + 2.0f;
	m_Stripes[1] = new UtermStripeEntity(m_Scene.get(), "Stripe", 64, Uterm3(
		VEC3(0, 4, 0),
		VEC3(0, 0, 0),
		VEC3(0, -2, 0),
		VEC3(0.5f, 0, 0.5f),
		VEC3(30.f, 0, -30.f),
		VEC3(0, 0, PI_2)));
	m_Stripes[1]->SetPos(Pos);
	m_Stripes[1]->SetTex(RECTF(0.0f, 0.0f, 1.0f, 10.0f));
	m_Stripes[1]->SetTeamColor(COLORF(0.5f, 0.25f, 0.99f, 0.75f));

	particle_entity = new ParticleEntity(m_Scene.get(), "MainParticle", BaseMaterial::BLEND_LERP, ParticleDef, 50, 4.0f);
	particle_entity->SetPos(Pos);

	CurveStripeEntity *cse = new CurveStripeEntity(m_Scene.get(), "RainbowStripe", CurveStripeEntity::CURVE_TYPE_QUADRATIC, 16);
	VEC3 ControlPoints[3];
	ControlPoints[0] = VEC3(-4.0f, 1.0f, -2.0f);
	ControlPoints[2] = VEC3( 4.0f, 1.0f, -2.0f);
	ControlPoints[1] = (ControlPoints[0] + ControlPoints[2]) * 0.5f;
	ControlPoints[1].y += 5.0f;
	cse->SetControlPointCount(3);
	cse->SetAllControlPoints(ControlPoints);
	cse->SetPos(NPCPos);
	cse->SetHalfWidth(0.3333f);
	cse->SetTex(RECTF(0.0f, 0.0f, 1.0f, 10.0f));
	cse->SetTeamColor(COLORF(0.2f, 0.25f, 0.99f, 0.75f));
}

void GameRpg_pimpl::PeriodicCalc()
{
	// Pogoda

	float Z = m_PlayerPos.Dest.z;

	// Powinien byæ deszcz
	if (Z > 416.0f)
	{
		// Funkcja liniowa f(416) = 0, f(466) = 1
		//float Intensity = minmax<float>(0.0f, 0.02f * Z - 8.32f, 1.0f);
		float Intensity = minmax<float>(0.0f,
			SOLVE_LINEAR_A(416.0f, 0.0f, 466.0f, 1.0f) * Z +
			SOLVE_LINEAR_B(416.0f, 0.0f, 466.0f, 1.0f), 1.0f);

		if (m_FallMode == FALL_MODE_NONE)
			m_Scene->SetFall(new Fall(m_Scene.get(), RAIN_DESC));
		else if (m_FallMode == FALL_MODE_SNOW)
			m_Scene->GetFall()->SetEffectDesc(RAIN_DESC);

		m_Scene->GetFall()->SetIntensity(Intensity);
		UpdateWeatherParams(Intensity);
		m_FallMode = FALL_MODE_RAIN;
	}
	// Powinien byæ œnieg
	else if (Z >= 206.0f && Z < 312.0f)
	{
		float Intensity;
		if (Z < 262.0f)
			// Funkcja liniowa f(206) = 0, f(262) = 1
			//Intensity = minmax<float>(0.0f, 0.0178f * Z - 3.6785f, 1.0f);
			Intensity = minmax<float>(0.0f,
				SOLVE_LINEAR_A(222.0f, 0.0f, 262.0f, 1.5f) * Z +
				SOLVE_LINEAR_B(222.0f, 0.0f, 262.0f, 1.5f), 1.0f);
		else
			// Funkcja liniowa f(262) = 1, f(312) = 0
			//Intensity = minmax<float>(0.0f, -0.02f * Z + 6.24f, 1.0f);
			Intensity = minmax<float>(0.0f,
				SOLVE_LINEAR_A(262.0f, 1.5f, 320.0f, 0.0f) * Z +
				SOLVE_LINEAR_B(262.0f, 1.5f, 320.0f, 0.0f), 1.0f);

		if (m_FallMode == FALL_MODE_NONE)
			m_Scene->SetFall(new Fall(m_Scene.get(), SNOW_DESC));
		else if (m_FallMode == FALL_MODE_RAIN)
			m_Scene->GetFall()->SetEffectDesc(SNOW_DESC);

		m_Scene->GetFall()->SetIntensity(Intensity);
		UpdateWeatherParams(Intensity);
		m_FallMode = FALL_MODE_SNOW;
	}
	// Powinno byæ nic
	else
	{
		if (m_FallMode != FALL_MODE_NONE)
		{
			m_Scene->ResetFall();
			UpdateWeatherParams(0.0f);
			m_FallMode = FALL_MODE_NONE;
		}
	}
}

void GameRpg_pimpl::UpdateWeatherParams(float Intensity)
{
	// Kolor œwiat³a
	COLORF LightColor;
	Lerp(&LightColor, LIGHT_COLOR, DARK_LIGHT_COLOR, Intensity);
	m_Light->SetColor(LightColor);

	// Intensywnoœæ chmur
	m_SkyObj->SetCloudsThreshold(Intensity * -0.45f + 0.95f);

	// Kolor chmur
	COLORF CloudColorWeatherFactor = COLORF::WHITE * (1.0f - Intensity * 0.4f);
	m_SkyObj->SetCloudColorWeatherFactors(COLORF_PAIR(CloudColorWeatherFactor, CloudColorWeatherFactor));

	// Kolor mg³y
	COLORF FogColor;
	Lerp(&FogColor, m_SkyObj->GetCurrentHorizonColor(), COLORF(0.2f, 0.2f, 0.2f), Intensity);
	m_Scene->SetFogColor(FogColor);

	// Dystans mg³y
	m_Scene->SetFogStart(Lerp(0.75f, 0.25f, Intensity));

	// Wektor wiatru
	VEC2 WindVec;
	Lerp(&WindVec, WIND, DARK_WIND, Intensity);
	m_Scene->SetWindVec(WindVec);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GameRpg

GameRpg::GameRpg() :
	pimpl(new GameRpg_pimpl)
{
}

GameRpg::~GameRpg()
{
}

int GameRpg::CalcFrame()
{
	return pimpl->CalcFrame();
}

void GameRpg::Draw2D()
{
	pimpl->Draw2D();
}

void GameRpg::OnResolutionChanged()
{
	pimpl->OnResolutionChanged();
}

void GameRpg::OnMouseMove(const common::VEC2 &Pos)
{
	pimpl->OnMouseMove(Pos);
}

void GameRpg::OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	pimpl->OnMouseButton(Pos, Button, Action);
}

void GameRpg::OnMouseWheel(const common::VEC2 &Pos, float Delta)
{
}

engine::Scene * GameRpg::GetScene()
{
	return pimpl->GetScene();
}
