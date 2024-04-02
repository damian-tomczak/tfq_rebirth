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
#include "..\Engine\QMap.hpp"
#include "..\Engine\Sky.hpp"
#include "GameFpp.hpp"

using namespace engine;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy globalne

// Set this to match application scale..
const float unitsPerMeter = 100.0f;

static float BEGIN_TIME;
static float WIN_TIME;
static float LOSE_TIME;
static VEC3 PLAYER_START_POS;
static float PLAYER_MOVE_SPEED;
static float PLAYER_ROT_SPEED;
static float PLAYER_MOVE_SMOOTH_TIME;
static float PLAYER_Y;
static COLORF AMBIENT_COLOR;
static COLORF FLASHING_POINT_LIGHT_COLOR;
static COLORF HALL_POINT_LIGHT_COLOR;
static COLORF SPOT_LIGHT_1_COLOR;
static COLORF SPOT_LIGHT_2_COLOR;
static float CAMERA_ABOVE_CENTER;
static VEC3 ELLIPSOID_RADIUS_VECTOR;
static float GRAVITY;
static float CLIMB;
static VEC3 MONSTER_POS;
static float MONSTER_SIZE;
static float MONSTER_ANGLE_Y;
static VEC3 GUN_OFFSET;
static float GUN_ANGLE_Z;
static float SHOT_PERIOD;
static float SHOT_GUN_ANGLE;
static float SHOT_GUN_OFFSET;
static float SHOT_GUN_ANIM_TIME;
static COLORF MISSILE_LIGHT_COLOR;
static float MISSILE_LIGHT_DIST;
static float MISSILE_QUAD_HALF_SIZE;
static COLORF MISSILE_QUAD_COLOR;
static float MISSILE_SPEED;
static float MISSILE_DAMAGE;
static float BLOOD_EFFECT_TIME;

/** Structure I use to pass in information about the move and in
which I store the result of the collision tests \br
èrÛd≥o: Improved Collision detection and Response, Kasper Fauerby */
struct CollisionPacket
{
	VEC3 eRadius; // ellipsoid radius
	// Information about the move being requested: (in R3)
	VEC3 R3Velocity;
	VEC3 R3Position;
	// Information about the move being requested: (in eSpace)
	VEC3 velocity;
	VEC3 normalizedVelocity;
	VEC3 basePoint;
	// Hit information
	bool foundCollision;
	float nearestDistance;
	VEC3 intersectionPoint;
};

struct FPP_COLLISION_FACE
{
	VEC3 Vertices[4];
};

struct MISSILE
{
	VEC3 m_Pos;
	VEC3 m_Dir; // Znormalizowany
	shared_ptr<QuadEntity> m_Entity;
	shared_ptr<PointLight> m_Light;

	MISSILE(Scene *OwnerScene, const VEC3 &Pos, const VEC3 &Dir);

	void OnPosUpdate();
};

MISSILE::MISSILE(Scene *OwnerScene, const VEC3 &Pos, const VEC3 &Dir) :
	m_Pos(Pos),
	m_Dir(Dir),
	m_Entity(new QuadEntity(OwnerScene, "Missile")),
	m_Light(new PointLight(OwnerScene, MISSILE_LIGHT_COLOR, Pos, MISSILE_LIGHT_DIST))
{
	m_Entity->SetPos(Pos);
	m_Entity->SetTeamColor(MISSILE_QUAD_COLOR);
	m_Entity->SetHalfSize(VEC2(MISSILE_QUAD_HALF_SIZE, MISSILE_QUAD_HALF_SIZE));
	m_Entity->SetUseLighting(false);
	m_Entity->SetDegreesOfFreedom(2);

	m_Light->SetZNear(0.5f);
	m_Light->SetCastShadow(true);
}

void MISSILE::OnPosUpdate()
{
	m_Entity->SetPos(m_Pos);
	m_Light->SetPos(m_Pos);
}

typedef std::vector<FPP_COLLISION_FACE> FPP_COLLISION_MAP;
typedef std::list<MISSILE> MISSILE_LIST;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GameFpp_pimpl

class GameFpp_pimpl
{
public:
	GameFpp_pimpl();
	~GameFpp_pimpl();
	int CalcFrame();
	void Draw2D();
	void OnResolutionChanged();
	void OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action);
	void OnMouseMove(const common::VEC2 &Pos);
	Scene * GetScene() { return m_Scene.get(); }

private:
	QMap *m_Map; // Pobrany w konstruktorze wskaünik do zasobu.
	scoped_ptr<Scene> m_Scene;
	// Zwolni go scena
	Camera *m_Camera;
	SmoothCD_obj<VEC3> m_PlayerPos;
	// Zwolni go scena
	DirectionalLight *m_Light;
	PointLight *m_FlashingPointLight;
	PointLight *m_HallPointLight;
	SpotLight *m_SpotLights[2];
	QMeshEntity *m_Monster;
	QMeshEntity *m_Gun;
	bool m_MouseDown;
	float m_LastShotTime;
	MISSILE_LIST m_Missiles;
	PARTICLE_DEF m_BloodParticleDef;
	ParticleEntity *m_BloodEffect;
	float m_BloodEffectStartTime; // Waøne tylko kiedy (m_BloodEffect != NULL).
	float m_MonsterLife;

	SmoothCD_obj<VEC3> m_CameraSmoothPos;
	FPP_COLLISION_MAP m_CollisionMap;

	// Bieøπcy stan rozgrywki
	enum GAME_STATE
	{
		GAME_STATE_NONE,
		GAME_STATE_BEGIN, // Istnieje w scenie PpColor
		GAME_STATE_WIN,   // Istnieje w scenie PpFunction
		GAME_STATE_LOSE,  // Istnieje w scenie PpColor
	};
	GAME_STATE m_GameState;
	// Czas bieøacego stanu, dope≥nia siÍ 0..1
	float m_GameStateTime;

	float GetCurrentAspectRatio();
	void Win();
	void Lose();
	void Shot();
	void CreateMaterials();
	void CreateCollisionMap();
	void CreateParticleDef();
	void UpdateGunOrientation();
	void CreateBlood(const VEC3 &Pos);

	bool checkPointInTriangle(const VEC3& point, const VEC3& pa, const VEC3& pb, const VEC3& pc);
	bool getLowestRoot(float a, float b, float c, float maxR, float* root);
	void checkTriangle(CollisionPacket* colPackage, const VEC3& p1, const VEC3& p2, const VEC3& p3);
	void checkCollision(CollisionPacket *collisionPackage);
	VEC3 collideWithWorld(const VEC3& pos, const VEC3& vel, CollisionPacket *collisionPackage, int collisionRecursionDepth, bool *Collision);
	void collideAndSlide(const VEC3 &vel, const VEC3 & gravity, VEC3 *OutPos);
};


GameFpp_pimpl::GameFpp_pimpl() :
	m_Map(NULL),
	m_FlashingPointLight(NULL),
	m_HallPointLight(NULL),
	m_Monster(NULL),
	m_Gun(NULL),
	m_MouseDown(false),
	m_LastShotTime(0.0f),
	m_BloodEffect(NULL),
	m_MonsterLife(1.0f)
{
	m_SpotLights[0] = m_SpotLights[1] = NULL;

	m_GameState = GAME_STATE_BEGIN;
	m_GameStateTime = 0.0f;

	// Wczytaj konfiguracjÍ

	g_Config->MustGetDataEx("Game/BeginTime", &BEGIN_TIME);
	g_Config->MustGetDataEx("Game/WinTime", &WIN_TIME);
	g_Config->MustGetDataEx("Game/LoseTime", &LOSE_TIME);
	g_Config->MustGetDataEx("Game/FPP/PlayerStartPos", &PLAYER_START_POS);
	g_Config->MustGetDataEx("Game/FPP/PlayerMoveSmoothTime", &PLAYER_MOVE_SMOOTH_TIME);
	g_Config->MustGetDataEx("Game/FPP/PlayerMoveSpeed", &PLAYER_MOVE_SPEED);
	g_Config->MustGetDataEx("Game/FPP/PlayerRotSpeed", &PLAYER_ROT_SPEED);
	g_Config->MustGetDataEx("Game/FPP/PlayerY", &PLAYER_Y);
	g_Config->MustGetDataEx("Game/FPP/AmbientColor", &AMBIENT_COLOR);
	g_Config->MustGetDataEx("Game/FPP/FlashingPointLightColor", &FLASHING_POINT_LIGHT_COLOR);
	g_Config->MustGetDataEx("Game/FPP/HallPointLightColor", &HALL_POINT_LIGHT_COLOR);
	g_Config->MustGetDataEx("Game/FPP/SpotLight1Color", &SPOT_LIGHT_1_COLOR);
	g_Config->MustGetDataEx("Game/FPP/SpotLight2Color", &SPOT_LIGHT_2_COLOR);
	g_Config->MustGetDataEx("Game/FPP/CameraAboveCenter", &CAMERA_ABOVE_CENTER);
	g_Config->MustGetDataEx("Game/FPP/EllipsoidRadiusVector", &ELLIPSOID_RADIUS_VECTOR);
	g_Config->MustGetDataEx("Game/FPP/Gravity", &GRAVITY);
	g_Config->MustGetDataEx("Game/FPP/Climb", &CLIMB);
	g_Config->MustGetDataEx("Game/FPP/MonsterPos", &MONSTER_POS);
	g_Config->MustGetDataEx("Game/FPP/MonsterSize", &MONSTER_SIZE);
	g_Config->MustGetDataEx("Game/FPP/MonsterAngleY", &MONSTER_ANGLE_Y);
	g_Config->MustGetDataEx("Game/FPP/GunOffset", &GUN_OFFSET);
	g_Config->MustGetDataEx("Game/FPP/GunAngleZ", &GUN_ANGLE_Z);
	g_Config->MustGetDataEx("Game/FPP/ShotPeriod", &SHOT_PERIOD);
	g_Config->MustGetDataEx("Game/FPP/ShotGunAngle", &SHOT_GUN_ANGLE);
	g_Config->MustGetDataEx("Game/FPP/ShotGunOffset", &SHOT_GUN_OFFSET);
	g_Config->MustGetDataEx("Game/FPP/ShotGunAnimTime", &SHOT_GUN_ANIM_TIME);
	g_Config->MustGetDataEx("Game/FPP/MissileLightColor", &MISSILE_LIGHT_COLOR);
	g_Config->MustGetDataEx("Game/FPP/MissileLightDist", &MISSILE_LIGHT_DIST);
	g_Config->MustGetDataEx("Game/FPP/MissileQuadHalfSize", &MISSILE_QUAD_HALF_SIZE);
	g_Config->MustGetDataEx("Game/FPP/MissileQuadColor", &MISSILE_QUAD_COLOR);
	g_Config->MustGetDataEx("Game/FPP/MissileSpeed", &MISSILE_SPEED);
	g_Config->MustGetDataEx("Game/FPP/MissileDamage", &MISSILE_DAMAGE);
	g_Config->MustGetDataEx("Game/FPP/BloodEffectTime", &BLOOD_EFFECT_TIME);

	PLAYER_START_POS.y = PLAYER_Y;

	// Mapa
	m_Map = res::g_Manager->MustGetResourceEx<engine::QMap>("FppMap");
	m_Map->Lock();

	CreateCollisionMap();

	// AABB sceny
	BOX MainBounds;
	m_Map->GetBoundingBox(&MainBounds);

	// Scena
	m_Scene.reset(new engine::Scene(MainBounds));
	engine::g_Engine->SetActiveScene(m_Scene.get());

	// Wiatr siÍ tutaj nie przyda.
	// T≥o siÍ tutaj nie przyda.

	m_Scene->SetSky(new SolidSky(COLOR::BLACK));

	// Mg≥a
	m_Scene->SetFogEnabled(true);
	m_Scene->SetFogColor(COLORF::BLACK);
	m_Scene->SetFogStart(0.8f);

	// Bloom, Tone Mapping
	m_Scene->SetPpBloom(new PpBloom(1.0f, 0.8f));
	m_Scene->SetPpToneMapping(new PpToneMapping(1.0f));

	// PpColor dla GAME_STATE_BEGIN
	m_Scene->SetPpColor(new PpColor(COLOR(0xFF000000)));

	// Kamera
	m_PlayerPos.Set(PLAYER_START_POS, VEC3::ZERO);
	m_PlayerPos.SmoothTime = PLAYER_MOVE_SMOOTH_TIME;
	m_Camera = new Camera(m_Scene.get(),
		PLAYER_START_POS, 0.0f, 0.0f, 0.0f, 0.5f, 100.0f, PI_4, GetCurrentAspectRatio());
	m_Scene->SetActiveCamera(m_Camera);

	// Materia≥y
	CreateMaterials();
	// Efekty czπsteczkowe
	CreateParticleDef();

	// Mapa w scenie
	m_Scene->SetQMap(m_Map);
	
	////// åwiat≥a

	m_Scene->SetAmbientColor(AMBIENT_COLOR);

	//DirectionalLight *l = new DirectionalLight(m_Scene.get(), COLORF(0.8f, 0.8f, 0.8f), VEC3(0.333f, -0.333f, 0.333f));

	m_FlashingPointLight = new PointLight(m_Scene.get(),
		FLASHING_POINT_LIGHT_COLOR,
		VEC3(29.5f, 3.5f, 1.0f),
		25.0f);
	m_FlashingPointLight->SetCastShadow(false);
	m_FlashingPointLight->SetCastSpecular(true);
	m_FlashingPointLight->SetZNear(1.0f);
	m_FlashingPointLight->SetShadowFactor(0.0f);

	m_HallPointLight = new PointLight(m_Scene.get(),
		HALL_POINT_LIGHT_COLOR,
		VEC3(-0.5f, 1.5f, 2.0f),
		7.0f);
	m_HallPointLight->SetCastSpecular(true);
	m_HallPointLight->SetCastShadow(false);

	m_SpotLights[0] = new SpotLight(m_Scene.get(),
		SPOT_LIGHT_1_COLOR,
		VEC3(17.0f, 2.0f, 39.0f),
		VEC3::NEGATIVE_Y,
		30.0f,
		PI_2 * 1.0f);
	m_SpotLights[0]->SetCastShadow(true);
	m_SpotLights[0]->SetZNear(0.5f);
	m_SpotLights[0]->SetSmooth(true);

	m_SpotLights[1] = new SpotLight(m_Scene.get(),
		SPOT_LIGHT_2_COLOR,
		VEC3(17.0f, 2.0f, 39.0f),
		VEC3::NEGATIVE_Y,
		30.0f,
		PI_2 * 1.0f);
	m_SpotLights[1]->SetCastShadow(true);
	m_SpotLights[1]->SetZNear(0.5f);
	m_SpotLights[1]->SetSmooth(true);

	// PotwÛr
	QUATERNION MonsterQuat;
	QuaternionRotationY(&MonsterQuat, MONSTER_ANGLE_Y);
	m_Monster = new QMeshEntity(m_Scene.get(), "MonsterModel");
	m_Monster->SetSize(MONSTER_SIZE);
	m_Monster->SetPos(MONSTER_POS);
	m_Monster->SetOrientation(MonsterQuat);
	m_Monster->SetCastShadow(true);
	m_Monster->SetReceiveShadow(false);
	m_Monster->SetAnimation("Threat", QMeshEntity::ANIM_MODE_LOOP, 0.0f, 1.0f);

	// Pistolet
	m_Gun = new QMeshEntity(m_Scene.get(), "GunModel");
	m_Gun->SetCastShadow(false);
	m_Gun->SetReceiveShadow(true);
	UpdateGunOrientation();

	// Sterowanie
	frame::SetMouseMode(true);
	ShowCursor(FALSE);
}

GameFpp_pimpl::~GameFpp_pimpl()
{
	ShowCursor(TRUE);
	frame::SetMouseMode(false);

	SAFE_DELETE(m_BloodEffect);
	m_Missiles.clear(); // Bo te struktury przechowujπ úwiat≥a i encje

	g_Engine->SetActiveScene(NULL);
	m_Scene.reset();

	if (m_Map)
		m_Map->Unlock();
}

bool isFrontFacingTo(const PLANE &plane, const VEC3& direction)
{
	return DotNormal(plane, direction) <= 0;
}

typedef unsigned int uint32;
#define in(a) ((uint32&) a)

/** Below is a function that determines if a point is inside a triangle or not. This
is used in the collision detection step. \br
èrÛd≥o: Improved Collision detection and Response, Kasper Fauerby */
bool GameFpp_pimpl::checkPointInTriangle(const VEC3& point, const VEC3& pa, const VEC3& pb, const VEC3& pc)
{
	VEC3 e10 = pb-pa;
	VEC3 e20 = pc-pa;
	float a = Dot(e10, e10);
	float b = Dot(e10, e20);
	float c = Dot(e20, e20);
	float ac_bb = (a*c) - (b*b);
	VEC3 vp(point.x-pa.x, point.y-pa.y, point.z-pa.z);
	float d = Dot(vp, e10);
	float e = Dot(vp, e20);
	float x = (d*c) - (e*b);
	float y = (e*a) - (d*b);
	float z = x+y - ac_bb;
	return (( in(z) & ~(in(x)|in(y)) ) & 0x80000000) != 0;
}

#undef in

/** Below is a snippet of code that solves a quadratic equation and returns the
lowest root, below a certain treshold (the maxR parameter) \br
èrÛd≥o: Improved Collision detection and Response, Kasper Fauerby */
bool GameFpp_pimpl::getLowestRoot(float a, float b, float c, float maxR, float* root)
{
	// Check if a solution exists
	float determinant = b*b - 4.0f*a*c;
	// If determinant is negative it means no solutions.
	if (determinant < 0.0f) return false;
	// calculate the two roots: (if determinant == 0 then
	// x1 == x2 but letís disregard that slight optimization)
	float sqrtD = sqrtf(determinant);
	float r1 = (-b - sqrtD) / (2.0f*a);
	float r2 = (-b + sqrtD) / (2.0f*a);
	// Sort so x1 <= x2
	if (r1 > r2)
	{
		float temp = r2;
		r2 = r1;
		r1 = temp;
	}
	// Get lowest root:
	if (r1 > 0 && r1 < maxR)
	{
		*root = r1;
		return true;
	}
	// It is possible that we want x2 - this can happen
	// if x1 < 0
	if (r2 > 0 && r2 < maxR)
	{
		*root = r2;
		return true;
	}
	// No (valid) solutions
	return false;
}

/** And below a function thatíll check a single triangle for collision.
Assumes: p1,p2 and p3 are given in ellisoid space \br
èrÛd≥o: Improved Collision detection and Response, Kasper Fauerby */
void GameFpp_pimpl::checkTriangle(CollisionPacket* colPackage, const VEC3& p1, const VEC3& p2, const VEC3& p3)
{
	// Make the plane containing this triangle.
	PLANE trianglePlane;
	PointsToPlane(&trianglePlane, p1, p2, p3);
	// Is triangle front-facing to the velocity vector?
	// We only check front-facing triangles
	// (your choice of course)

	if (isFrontFacingTo(trianglePlane, colPackage->normalizedVelocity))
	{
		// Get interval of plane intersection:
		float t0, t1;
		bool embeddedInPlane = false;
		// Calculate the signed distance from sphere
		// position to triangle plane
		float signedDistToTrianglePlane = DotCoord(trianglePlane, colPackage->basePoint);
		// cache this as weíre going to use it a few times below:
		float normalDotVelocity = Dot(trianglePlane.GetNormal(), colPackage->velocity);
		// if sphere is travelling parrallel to the plane:
		if (normalDotVelocity == 0.0f)
		{
			if (fabs(signedDistToTrianglePlane) >= 1.0f)
			{
				// Sphere is not embedded in plane.
				// No collision possible:
				return;
			}
			else
			{
				// sphere is embedded in plane.
				// It intersects in the whole range [0..1]
				embeddedInPlane = true;
				t0 = 0.0;
				t1 = 1.0f;
			}
		}
		else {
			// N dot D is not 0. Calculate intersection interval:
			t0 = (-1.0f-signedDistToTrianglePlane) / normalDotVelocity;
			t1 = ( 1.0f-signedDistToTrianglePlane) / normalDotVelocity;
			// Swap so t0 < t1
			if (t0 > t1)
			{
				float temp = t1;
				t1 = t0;
				t0 = temp;
			}
			// Check that at least one result is within range:
			if (t0 > 1.0f || t1 < 0.0f)
			{
				// Both t values are outside values [0,1]
				// No collision possible:
				return;
			}
			// Clamp to [0,1]
			if (t0 < 0.0) t0 = 0.0;
			if (t1 < 0.0) t1 = 0.0;
			if (t0 > 1.0f) t0 = 1.0f;
			if (t1 > 1.0f) t1 = 1.0f;
		}
		// OK, at this point we have two time values t0 and t1
		// between which the swept sphere intersects with the
		// triangle plane. If any collision is to occur it must
		// happen within this interval.
		VEC3 collisionPoint;
		bool foundCollison = false;
		float t = 1.0f;
		// First we check for the easy case - collision inside
		// the triangle. If this happens it must be at time t0
		// as this is when the sphere rests on the front side
		// of the triangle plane. Note, this can only happen if
		// the sphere is not embedded in the triangle plane.
		if (!embeddedInPlane)
		{
			VEC3 planeIntersectionPoint = (colPackage->basePoint-trianglePlane.GetNormal()) + t0*colPackage->velocity;
			if (checkPointInTriangle(planeIntersectionPoint, p1,p2,p3))
			{
				foundCollison = true;
				t = t0;
				collisionPoint = planeIntersectionPoint;
			}
		}
		// if we havenít found a collision already weíll have to
		// sweep sphere against points and edges of the triangle.
		// Note: A collision inside the triangle (the check above)
		// will always happen before a vertex or edge collision!
		// This is why we can skip the swept test if the above
		// gives a collision!
		if (foundCollison == false)
		{
			// some commonly used terms:
			VEC3 velocity = colPackage->velocity;
			VEC3 base = colPackage->basePoint;
			float velocitySquaredLength = LengthSq(velocity);
			float a,b,c; // Params for equation
			float newT;
			// For each vertex or edge a quadratic equation have to
			// be solved. We parameterize this equation as
			// a*t^2 + b*t + c = 0 and below we calculate the
			// parameters a,b and c for each test.
			// Check against points:
			a = velocitySquaredLength;
			// P1
			b = 2.0f*Dot(velocity, (base-p1));
			c = LengthSq(p1-base) - 1.0f;
			if (getLowestRoot(a,b,c, t, &newT)) {
				t = newT;
				foundCollison = true;
				collisionPoint = p1;
			}
			// P2
			b = 2.0f*Dot(velocity, base-p2);
			c = LengthSq(p2-base) - 1.0f;
			if (getLowestRoot(a,b,c, t, &newT)) {
				t = newT;
				foundCollison = true;
				collisionPoint = p2;
			}
			// P3
			b = 2.0f*Dot(velocity, (base-p3));
			c = LengthSq(p3-base) - 1.0f;
			if (getLowestRoot(a,b,c, t, &newT))
			{
				t = newT;
				foundCollison = true;
				collisionPoint = p3;
			}
			// Check agains edges:
			// p1 -> p2:
			VEC3 edge = p2-p1;
			VEC3 baseToVertex = p1 - base;
			float edgeSquaredLength = LengthSq(edge);
			float edgeDotVelocity = Dot(edge, velocity);
			float edgeDotBaseToVertex = Dot(edge, baseToVertex);
			// Calculate parameters for equation
			a = edgeSquaredLength*-velocitySquaredLength + edgeDotVelocity*edgeDotVelocity;
			b = edgeSquaredLength*(2.0f*Dot(velocity, baseToVertex))- 2.0f*edgeDotVelocity*edgeDotBaseToVertex;
			c = edgeSquaredLength*(1.0f-LengthSq(baseToVertex))+ edgeDotBaseToVertex*edgeDotBaseToVertex;
			// Does the swept sphere collide against infinite edge?
			if (getLowestRoot(a,b,c, t, &newT))
			{
				// Check if intersection is within line segment:
				float f=(edgeDotVelocity*newT-edgeDotBaseToVertex)/edgeSquaredLength;
				if (f >= 0.0 && f <= 1.0f)
				{
					// intersection took place within segment.
					t = newT;
					foundCollison = true;
					collisionPoint = p1 + f*edge;
				}
			}
			// p2 -> p3:
			edge = p3-p2;
			baseToVertex = p2 - base;
			edgeSquaredLength = LengthSq(edge);
			edgeDotVelocity = Dot(edge, velocity);
			edgeDotBaseToVertex = Dot(edge, baseToVertex);
			a = edgeSquaredLength*-velocitySquaredLength + edgeDotVelocity*edgeDotVelocity;
			b = edgeSquaredLength*(2.0f*Dot(velocity, baseToVertex))-2.0f*edgeDotVelocity*edgeDotBaseToVertex;
			c = edgeSquaredLength*(1.0f-LengthSq(baseToVertex))+edgeDotBaseToVertex*edgeDotBaseToVertex;
			if (getLowestRoot(a,b,c, t, &newT))
			{
				float f=(edgeDotVelocity*newT-edgeDotBaseToVertex)/edgeSquaredLength;
				if (f >= 0.0 && f <= 1.0f)
				{
					t = newT;
					foundCollison = true;
					collisionPoint = p2 + f*edge;
				}
			}
			// p3 -> p1:
			edge = p1-p3;
			baseToVertex = p3 - base;
			edgeSquaredLength = LengthSq(edge);
			edgeDotVelocity = Dot(edge, velocity);
			edgeDotBaseToVertex = Dot(edge, baseToVertex);
			a = edgeSquaredLength*-velocitySquaredLength +edgeDotVelocity*edgeDotVelocity;
			b = edgeSquaredLength*(2.0f*Dot(velocity, baseToVertex))-2.0f*edgeDotVelocity*edgeDotBaseToVertex;
			c = edgeSquaredLength*(1.0f-LengthSq(baseToVertex))+edgeDotBaseToVertex*edgeDotBaseToVertex;
			if (getLowestRoot(a,b,c, t, &newT))
			{
				float f=(edgeDotVelocity*newT-edgeDotBaseToVertex)/edgeSquaredLength;
				if (f >= 0.0 && f <= 1.0f)
				{
					t = newT;
					foundCollison = true;
					collisionPoint = p3 + f*edge;
				}
			}
		}
		// Set result:
		if (foundCollison == true)
		{
			// distance to collision: ítí is time of collision
			float distToCollision = t*Length(colPackage->velocity);
			// Does this triangle qualify for the closest hit?
			// it does if itís the first hit or the closest
			if (colPackage->foundCollision == false || distToCollision < colPackage->nearestDistance)
			{
				// Collision information nessesary for sliding
				colPackage->nearestDistance = distToCollision;
				colPackage->intersectionPoint = collisionPoint;
				colPackage->foundCollision = true;
			}
		}
	} // if not backface
}

void GameFpp_pimpl::checkCollision(CollisionPacket *collisionPackage)
{
	for (size_t i = 0; i < m_CollisionMap.size(); i++)
	{
		VEC3 v0 = VEC3(
			m_CollisionMap[i].Vertices[0].x / ELLIPSOID_RADIUS_VECTOR.x,
			m_CollisionMap[i].Vertices[0].y / ELLIPSOID_RADIUS_VECTOR.y,
			m_CollisionMap[i].Vertices[0].z / ELLIPSOID_RADIUS_VECTOR.z);
		VEC3 v1 = VEC3(
			m_CollisionMap[i].Vertices[1].x / ELLIPSOID_RADIUS_VECTOR.x,
			m_CollisionMap[i].Vertices[1].y / ELLIPSOID_RADIUS_VECTOR.y,
			m_CollisionMap[i].Vertices[1].z / ELLIPSOID_RADIUS_VECTOR.z);
		VEC3 v2 = VEC3(
			m_CollisionMap[i].Vertices[2].x / ELLIPSOID_RADIUS_VECTOR.x,
			m_CollisionMap[i].Vertices[2].y / ELLIPSOID_RADIUS_VECTOR.y,
			m_CollisionMap[i].Vertices[2].z / ELLIPSOID_RADIUS_VECTOR.z);
		VEC3 v3 = VEC3(
			m_CollisionMap[i].Vertices[3].x / ELLIPSOID_RADIUS_VECTOR.x,
			m_CollisionMap[i].Vertices[3].y / ELLIPSOID_RADIUS_VECTOR.y,
			m_CollisionMap[i].Vertices[3].z / ELLIPSOID_RADIUS_VECTOR.z);
		checkTriangle(collisionPackage, v0, v1, v2);
		checkTriangle(collisionPackage, v0, v2, v3);
	}
}

VEC3 GameFpp_pimpl::collideWithWorld(const VEC3& pos, const VEC3& vel, CollisionPacket *collisionPackage, int collisionRecursionDepth, bool *Collision)
{
	if (Collision) *Collision = false;
	// All hard-coded distances in this function is
	// scaled to fit the setting above..
	float unitScale = unitsPerMeter / 100.0f;
	float veryCloseDistance = 0.005f * unitScale;
	// do we need to worry?
	if (collisionRecursionDepth > 5)
		return pos;
	// Ok, we need to worry:
	collisionPackage->velocity = vel;
	Normalize(&collisionPackage->normalizedVelocity, vel);
	collisionPackage->basePoint = pos;
	collisionPackage->foundCollision = false;
	// Check for collision (calls the collision routines)
	// Application specific!!
	checkCollision(collisionPackage);
	// If no collision we just move along the velocity
	if (collisionPackage->foundCollision == false)
		return pos + vel;
	// *** Collision occured ***
	if (Collision) *Collision = true;
	// The original destination point
	VEC3 destinationPoint = pos + vel;
	VEC3 newBasePoint = pos;
	// only update if we are not already very close
	// and if so we only move very close to intersection..not
	// to the exact spot.
	if (collisionPackage->nearestDistance >= veryCloseDistance)
	{
		VEC3 V;
		Normalize(&V,vel);
		V *= collisionPackage->nearestDistance - veryCloseDistance;
		newBasePoint = collisionPackage->basePoint + V;
		// Adjust polygon intersection point (so sliding
		// plane will be unaffected by the fact that we
		// move slightly less than collision tells us)
		Normalize(&V);
		collisionPackage->intersectionPoint -= veryCloseDistance * V;
	}
	// Determine the sliding plane
	VEC3 slidePlaneOrigin = collisionPackage->intersectionPoint;
	VEC3 slidePlaneNormal = newBasePoint - collisionPackage->intersectionPoint;
	Normalize(&slidePlaneNormal, slidePlaneNormal);
	PLANE slidingPlane;
	PointNormalToPlane(&slidingPlane, slidePlaneOrigin, slidePlaneNormal);
	// Again, sorry about formatting.. but look carefully ;)
	VEC3 newDestinationPoint = destinationPoint - DotCoord(slidingPlane, destinationPoint) * slidePlaneNormal;
	// Generate the slide vector, which will become our new
	// velocity vector for the next iteration
	VEC3 newVelocityVector = newDestinationPoint - collisionPackage->intersectionPoint;
	// Recurse:
	// dont recurse if the new velocity is very small
	if (Length(newVelocityVector) < veryCloseDistance)
		return newBasePoint;
	return collideWithWorld(newBasePoint, newVelocityVector, collisionPackage, collisionRecursionDepth+1, 0);
}

void GameFpp_pimpl::collideAndSlide(const VEC3 &vel, const VEC3 & gravity, VEC3 *OutPos)
{
	CollisionPacket collisionPackage;
	collisionPackage.eRadius = ELLIPSOID_RADIUS_VECTOR;
	// Do collision detection:
	collisionPackage.R3Position = m_PlayerPos.Dest;
	collisionPackage.R3Velocity = vel;
	// calculate position and velocity in eSpace
	VEC3 eSpacePosition = VEC3(
		collisionPackage.R3Position.x / collisionPackage.eRadius.x,
		collisionPackage.R3Position.y / collisionPackage.eRadius.y,
		collisionPackage.R3Position.z / collisionPackage.eRadius.z);
	VEC3 eSpaceVelocity = VEC3(
		collisionPackage.R3Velocity.x / collisionPackage.eRadius.x,
		collisionPackage.R3Velocity.y / collisionPackage.eRadius.y,
		collisionPackage.R3Velocity.z / collisionPackage.eRadius.z);
	// Iterate until we have our final position.
	VEC3 finalPosition = collideWithWorld(eSpacePosition, eSpaceVelocity, &collisionPackage, 0, 0);
	// Add gravity pull:
	// To remove gravity uncomment from here .....
	// Set the new R3 position (convert back from eSpace to R3
	collisionPackage.R3Position = VEC3(
		finalPosition.x *collisionPackage.eRadius.x,
		finalPosition.y *collisionPackage.eRadius.y,
		finalPosition.z *collisionPackage.eRadius.z);
	collisionPackage.R3Velocity = gravity;
	eSpaceVelocity = VEC3(
		gravity.x / collisionPackage.eRadius.x,
		gravity.y / collisionPackage.eRadius.y,
		gravity.z / collisionPackage.eRadius.z);
	finalPosition = collideWithWorld(finalPosition, eSpaceVelocity, &collisionPackage, 0, 0);
	// ... to here
	// Convert final result back to R3:
	finalPosition = VEC3(
		finalPosition.x *collisionPackage.eRadius.x,
		finalPosition.y *collisionPackage.eRadius.y,
		finalPosition.z *collisionPackage.eRadius.z);
	// Move the entity (application specific function)
	*OutPos = finalPosition;
}

int GameFpp_pimpl::CalcFrame()
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

	////// åwiat≥a
	m_FlashingPointLight->SetColor(FLASHING_POINT_LIGHT_COLOR * (PerlinNoise1(frame::Timer2.GetTime() * 5.0f) * 0.5f + 0.5f));

	VEC3 LightDir;
	LightDir.y = -0.7f;
	LightDir.x = cosf(frame::Timer2.GetTime() * 1.0f);
	LightDir.z = sinf(frame::Timer2.GetTime() * 1.0f);
	Normalize(&LightDir);
	m_SpotLights[0]->SetDir(LightDir);
	LightDir.y = -0.7f;
	LightDir.x = cosf(frame::Timer2.GetTime() * 1.0f + PI);
	LightDir.z = sinf(frame::Timer2.GetTime() * 1.0f + PI);
	Normalize(&LightDir);
	m_SpotLights[1]->SetDir(LightDir);

	////// Przeliczenie strza≥Ûw
	BOX MainBounds;
	m_Map->GetBoundingBox(&MainBounds);
	for (MISSILE_LIST::iterator it = m_Missiles.begin(); it != m_Missiles.end(); )
	{
		// Kolizja
		float T;
		Entity *E;
		Scene::COLLISION_RESULT CR = m_Scene->RayCollision(engine::COLLISION_PHYSICAL, it->m_Pos, it->m_Dir, &T, &E);
		if (CR != Scene::COLLISION_RESULT_NONE && T >= 0.0f && T <= 1.0f)
		{
			// Trafienie w potwora
			if (CR == Scene::COLLISION_RESULT_ENTITY && E == m_Monster)
			{
				CreateBlood(it->m_Pos + it->m_Dir * T);
				if (m_GameState == GAME_STATE_NONE || m_GameState == GAME_STATE_BEGIN)
				{
					m_MonsterLife -= MISSILE_DAMAGE;
					if (m_MonsterLife <= 0.0f)
					{
						m_Monster->SetAnimation("Die", QMeshEntity::ANIM_MODE_ONCE);
						Win();
					}
				}
			}
			// Zniszcz pocisk
			it = m_Missiles.erase(it);
		}
		else
		{
			// PrzesuÒ
			it->m_Pos += it->m_Dir * MISSILE_SPEED * dt;
			// Wyszed≥ poza granice
			if (!PointInBox(it->m_Pos, MainBounds))
				// Zniszcz pocisk i koniec
				it = m_Missiles.erase(it);
			// Pocisk leci dalej
			else
			{
				it->OnPosUpdate();
				++it;
			}
		}
	}

	////// Efekt krwi
	if (m_BloodEffect != NULL && m_BloodEffectStartTime + BLOOD_EFFECT_TIME < t)
		SAFE_DELETE(m_BloodEffect);

	if (m_GameState == GAME_STATE_NONE || m_GameState == GAME_STATE_BEGIN)
	{

		////// Chodzenie
		float Forward = 0.0f, Right = 0.0f;
		if (frame::GetKeyboardKey('W')) Forward += 1.0f;
		if (frame::GetKeyboardKey('S')) Forward -= 1.0f;
		if (frame::GetKeyboardKey('A')) Right -= 0.5f;
		if (frame::GetKeyboardKey('D')) Right += 0.5f;
		//VEC3 NewPos = m_PlayerPos.Dest;
		VEC3 DeltaPos = m_Camera->GetParams().GetForwardDir() * Forward * PLAYER_MOVE_SPEED * dt;
		DeltaPos += m_Camera->GetParams().GetRightDir() * Right * PLAYER_MOVE_SPEED * dt;
		DeltaPos.y = 0.0f;

		VEC3 OutPos;
		collideAndSlide(DeltaPos + VEC3(0.0f, CLIMB, 0.0f)*dt, (VEC3(0.0f, GRAVITY, 0.0f)-VEC3(0.0f, CLIMB, 0.0f))*dt, &OutPos);
		// Wykrywanie NaN
		if (is_nan(OutPos.x) || is_nan(OutPos.y) || is_nan(OutPos.z))
		{
			LOG(LOG_GAME, "collideAndSlide returned NaN!");
			OutPos = m_PlayerPos.Dest;
		}
		m_PlayerPos.Update(OutPos, dt);

		m_Camera->SetPos(m_PlayerPos.Pos + VEC3(0.0f, CAMERA_ABOVE_CENTER, 0.0f));

		////// Wystrza≥
		if (m_MouseDown && m_LastShotTime + SHOT_PERIOD < t)
		{
			Shot();
			m_LastShotTime = t;
		}
	}

	////// Model pistoletu
	VEC3 GunPos = m_Camera->GetPos() +
		m_Camera->GetParams().GetForwardDir() * GUN_OFFSET.z +
		m_Camera->GetParams().GetRightDir() * GUN_OFFSET.x +
		VEC3(0.0f, GUN_OFFSET.y, 0.0f);
	// To * 1.1f tutaj jest po to, øeby zdπøy≥a siÍ uaktualniÊ orientacja juø po ca≥kowitym opuszczeniu.
	if (t - m_LastShotTime < SHOT_GUN_ANIM_TIME * 1.1f)
		GunPos.y += (1.0f - (t - m_LastShotTime) / SHOT_GUN_ANIM_TIME) * SHOT_GUN_OFFSET;
	m_Gun->SetPos(GunPos);
	UpdateGunOrientation();

	return 0;
}

void GameFpp_pimpl::Draw2D()
{
	RECTF R;
	R.left = R.right = frame::GetScreenWidth() * 0.5f;
	R.top = R.bottom = frame::GetScreenHeight() * 0.5f;
	float ExtendY = frame::GetScreenHeight() * 0.04f;
	float ExtendX = ExtendY;
	R.left -= ExtendX;
	R.right += ExtendX;
	R.top -= ExtendY;
	R.bottom += ExtendY;

	gfx2d::g_Canvas->SetSprite(gfx2d::g_SpriteRepository->MustGetSprite("Sight"));
	gfx2d::g_Canvas->SetColor(COLOR::WHITE);
	gfx2d::g_Canvas->PushAlpha(0.2f);

	gfx2d::g_Canvas->DrawRect(R);

	gfx2d::g_Canvas->PopAlpha();
}

void GameFpp_pimpl::OnResolutionChanged()
{
	m_Camera->SetAspect(GetCurrentAspectRatio());
}

void GameFpp_pimpl::OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	if (Action == frame::MOUSE_DOWN || Action == frame::MOUSE_DBLCLK)
		m_MouseDown = true;
	else // MOUSE_UP
		m_MouseDown = false;
}

void GameFpp_pimpl::OnMouseMove(const common::VEC2 &Pos)
{
	m_Camera->SetAngleX(m_Camera->GetAngleX() + Pos.y * PLAYER_ROT_SPEED);
	m_Camera->SetAngleY(m_Camera->GetAngleY() + Pos.x * PLAYER_ROT_SPEED);
	UpdateGunOrientation();
}

float GameFpp_pimpl::GetCurrentAspectRatio()
{
	return (float)frame::Settings.BackBufferWidth / (float)frame::Settings.BackBufferHeight;
}

void GameFpp_pimpl::Win()
{
	LOG(LOG_GAME, "Win");

	m_GameState = GAME_STATE_WIN;
	m_GameStateTime = 0.0f;

	m_Scene->SetPpColor(new PpColor(COLOR(0x00000000u)));
}

void GameFpp_pimpl::Lose()
{
	LOG(LOG_GAME, "Loss");

	m_GameState = GAME_STATE_LOSE;
	m_GameStateTime = 0.0f;

	m_Scene->SetPpBloom(NULL); // Wy≥πczyÊ PpBloom, bo üle wspÛ≥pracuje z PpFunction
	m_Scene->SetPpFunction(new PpFunction(VEC3::ONE, VEC3::ZERO, 0.0f));
}

void GameFpp_pimpl::Shot()
{
	VEC3 MissilePos = m_Camera->GetPos() + m_Camera->GetParams().GetForwardDir() * 1.0f;

	m_Missiles.push_back(MISSILE(m_Scene.get(), MissilePos, m_Camera->GetParams().GetForwardDir()));
}

void GameFpp_pimpl::CreateMaterials()
{
	OpaqueMaterial *TestMapMat;

	TestMapMat = new OpaqueMaterial(m_Scene.get(), "MapPodloga", "FppMapTex_Podloga");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetNormalTextureName("FppMapTex_Podloga_n");
	TestMapMat->SetGlossMapping(true);

	TestMapMat = new OpaqueMaterial(m_Scene.get(), "MapKompy", "FppMapTex_Kompy");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	TestMapMat->SetEmissiveTextureName("FppMapTex_Kompy_emissive");

	TestMapMat = new OpaqueMaterial(m_Scene.get(), "MapKrataStalowa", "FppMapTex_KrataStalowa");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetAlphaTesting(200);
	TestMapMat->SetNormalTextureName("FppMapTex_KrataStalowa_n");

	TestMapMat = new OpaqueMaterial(m_Scene.get(), "MapPipe", "FppMapTex_Pipe");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetNormalTextureName("FppMapTex_Pipe_n");

	TestMapMat = new OpaqueMaterial(m_Scene.get(), "MapKrata", "FppMapTex_Krata");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetNormalTextureName("FppMapTex_Krata_n");
	//TestMapMat->SetGlossMapping(true);
	//TestMapMat->SetSpecularColor(COLORF::SILVER);
	TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);

	TestMapMat = new OpaqueMaterial(m_Scene.get(), "MapLampy");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetEmissiveTextureName("FppMapTex_Lampy");
	//TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);

	TestMapMat = new OpaqueMaterial(m_Scene.get(), "MapPlakat", "FppMapTex_Plakat");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);

	TestMapMat = new OpaqueMaterial(m_Scene.get(), "MapMur", "FppMapTex_ScianyMur");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetNormalTextureName("FppMapTex_ScianyMur_n");
	TestMapMat->SetSpecularColor(COLORF::SILVER);

	TestMapMat = new OpaqueMaterial(m_Scene.get(), "MapSufit", "FppMapTex_Sufit");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetNormalTextureName("FppMapTex_Sufit_n");

	TestMapMat = new OpaqueMaterial(m_Scene.get(), "MapSzyby", "FppMapTex_Szyby");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetNormalTextureName("FppMapTex_Szyby_n");

/*	TestMapMat = new OpaqueMaterial(m_Scene.get(), "material1", "FppMapTex_HightechFloor01");
	TestMapMat->SetPerPixel(true);
	//TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	TestMapMat->SetNormalTextureName("FppMapTex_HightechFloor01_n");
	TestMapMat = new OpaqueMaterial(m_Scene.get(), "material2", "FppMapTex_HightechFloor02");
	TestMapMat->SetPerPixel(true);
	//TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	TestMapMat->SetNormalTextureName("FppMapTex_HightechFloor02_n");
	TestMapMat = new OpaqueMaterial(m_Scene.get(), "material3", "FppMapTex_HightechWall01");
	TestMapMat->SetPerPixel(true);
	//TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	TestMapMat->SetNormalTextureName("FppMapTex_HightechWall01_n");
	TestMapMat = new OpaqueMaterial(m_Scene.get(), "material4", "FppMapTex_HightechWall02");
	TestMapMat->SetPerPixel(true);
	//TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	TestMapMat->SetNormalTextureName("FppMapTex_HightechWall02_n");
	TestMapMat = new OpaqueMaterial(m_Scene.get(), "material5", "FppMapTex_HightechWall03");
	TestMapMat->SetPerPixel(true);
	TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	TestMapMat->SetNormalTextureName("FppMapTex_HightechWall03_n");
//TestMapMat = new OpaqueMaterial(m_Scene.get(), "material6", "FppMapTex_HightechWall04");
//TestMapMat->SetPerPixel(true);
//TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	TestMapMat = new OpaqueMaterial(m_Scene.get(), "material7", "FppMapTex_HightechWall05");
	TestMapMat->SetPerPixel(true);
	//TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	TestMapMat->SetNormalTextureName("FppMapTex_HightechWall05_n");
	TestMapMat = new OpaqueMaterial(m_Scene.get(), "material8", "FppMapTex_HightechWall06");
	TestMapMat->SetPerPixel(true);
	//TestMapMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	TestMapMat->SetNormalTextureName("FppMapTex_HightechWall06_n");*/

	OpaqueMaterial *GunMat = new OpaqueMaterial(m_Scene.get(), "Gun", "GunTexture");
	GunMat->SetPerPixel(true);
	GunMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);

	OpaqueMaterial *MonsterMat = new OpaqueMaterial(m_Scene.get(), "FppMonster", "MonsterTexture");
	MonsterMat->SetPerPixel(true);
	MonsterMat->SetSpecularMode(OpaqueMaterial::SPECULAR_NONE);
	MonsterMat->SetEmissiveTextureName("MonsterTexture_emissive");

	TranslucentMaterial *MissileMat = new TranslucentMaterial(m_Scene.get(), "Missile",
		"MissileParticle",
		BaseMaterial::BLEND_LERP,
		SolidMaterial::COLOR_MODULATE,
		TranslucentMaterial::ALPHA_MODULATE);
	MissileMat->SetTwoSided(true);
	MissileMat->SetCollisionType(0);
}

void GameFpp_pimpl::CreateCollisionMap()
{
	string CollisionStr, Element;
	g_Config->MustGetData("Game/FPP/CollisionMap", &CollisionStr);
	uint StrIndex = 0, VertexIndex = 0;
	FPP_COLLISION_FACE Face;
	while (Split(CollisionStr, ";", &Element, &StrIndex))
	{
		Trim(&Element);
		MustStrToSth(&Face.Vertices[VertexIndex], Element);
		VertexIndex++;
		if (VertexIndex == 4)
		{
			m_CollisionMap.push_back(Face);
			VertexIndex = 0;
		}
	}
	if (VertexIndex > 0)
		throw Error("B≥Ídne dane w konfiguracji w \"Game/FPP/CollisionMap\".", __FILE__, __LINE__);
}

void GameFpp_pimpl::CreateParticleDef()
{
	m_BloodParticleDef.Zero();
	m_BloodParticleDef.CircleDegree = 0;
	m_BloodParticleDef.PosA1_C = VEC3(-0.7f, -0.7f, -0.7f);
	m_BloodParticleDef.PosA1_R = VEC3( 1.4f,  1.4f,  1.4f);
	m_BloodParticleDef.PosA2_C = VEC3( 0.0f, -0.7f,  0.0f);
	m_BloodParticleDef.ColorA0_C = VEC4(0.4f, 0.0f, 0.0f, 0.7f);
	m_BloodParticleDef.ColorA1_C = VEC4(0.0f, 0.0f, 0.0f, -0.7f);
	m_BloodParticleDef.SizeA0_C = 0.5f;
	m_BloodParticleDef.OrientationA0_C = 0.0f;
	m_BloodParticleDef.OrientationA0_R = PI_X_2;
	m_BloodParticleDef.OrientationA1_C = -PI;
	m_BloodParticleDef.OrientationA1_R = PI_X_2;
	m_BloodParticleDef.TimePeriod_C = BLOOD_EFFECT_TIME * 1.1f;
}

void GameFpp_pimpl::UpdateGunOrientation()
{
	float t = frame::Timer2.GetTime();

	float ShotX = 0.0f;
	if (t - m_LastShotTime < SHOT_GUN_ANIM_TIME)
		ShotX = - (1.0f - (t - m_LastShotTime) / SHOT_GUN_ANIM_TIME) * SHOT_GUN_ANGLE;

	QUATERNION GunQuat;
	EulerAnglesToQuaternionO2I(&GunQuat,
		m_Camera->GetAngleY(),
		m_Camera->GetAngleX() * 0.7f + ShotX,
		GUN_ANGLE_Z);
	m_Gun->SetOrientation(GunQuat);
}

void GameFpp_pimpl::CreateBlood(const VEC3 &Pos)
{
	SAFE_DELETE(m_BloodEffect);

	m_BloodEffect = new ParticleEntity(
		m_Scene.get(),
		"MainParticle",
		BaseMaterial::BLEND_LERP,
		m_BloodParticleDef,
		64, 1.0f);
	m_BloodEffect->SetPos(Pos);
	m_BloodEffect->SetUseLOD(false);

	m_BloodEffectStartTime = frame::Timer2.GetTime();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GameFpp

GameFpp::GameFpp() :
	pimpl(new GameFpp_pimpl)
{
}

GameFpp::~GameFpp()
{
}

int GameFpp::CalcFrame()
{
	return pimpl->CalcFrame();
}

void GameFpp::Draw2D()
{
	pimpl->Draw2D();
}

void GameFpp::OnResolutionChanged()
{
	pimpl->OnResolutionChanged();
}

void GameFpp::OnMouseMove(const common::VEC2 &Pos)
{
	pimpl->OnMouseMove(Pos);
}

void GameFpp::OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action)
{
	pimpl->OnMouseButton(Pos, Button, Action);
}

void GameFpp::OnMouseWheel(const common::VEC2 &Pos, float Delta)
{
}

engine::Scene * GameFpp::GetScene()
{
	return pimpl->GetScene();
}
