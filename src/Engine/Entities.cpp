/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "Engine.hpp" // Bo potrzebny do materia³ów itp.
#include "Entities.hpp"

namespace engine
{

/*
Wydajnoœæ wydajnoœci¹, ale pisanie Final Demo dowiod³o, ¿e tutaj nie mo¿na sobie
jednak pozwoliæ na ¿aden margines b³êdu. Dlaczego? To skomplikowany b³¹d...
Rysowanie przebiega tak:
- Przebieg Ambient:   Model 1, Model 2, Model 3, Model 4
- Przebieg œwiat³a 1: Model 1, Model 2, Model 3, Model 4
- Przebieg œwiat³a 2 itd...
Za ka¿dym rysowaniem modelu pobierany jest zestaw macierzy dla jego bie¿¹cej
klatki animacji. Tam u¿ywana by³a ta tolerancja. Pamiêæ podrêczna ostatnio
generowanych zestawów macierzy jest, o ile pamiêtam, utrzymywana w kolejnoœci
ostatniego u¿ycia, wiêc kolejnoœæ elementów zmienia siê podczas rysowania. Tote¿
mo¿liwe jest, ¿e do narysowania tego samego modelu w dwóch ró¿nych przebiegach
u¿yte zostan¹ dwie ró¿ne klatki animacji w granicach tej tolerancji, a to
owocuje b³êdami w obrazie. Dlaczego? Bo obraz jest rysowany w wielu przebiegach,
z addytywnym blendingiem i z u¿yciem Z-bufora. Wartoœci Z w danym miejscu mog¹
siê tak niefortunnie u³o¿yæ, ¿e ta z przebiegu Ambient jest mniejsza ni¿ ta
z przebiegu œwiat³a i piksel pozostaje ciemny. Co nale¿a³o dowieœæ :)
*/
//const float ANIMATION_ACCURACY = 1.0f / 50.0f; // W sekundach
const float ANIMATION_ACCURACY = 0.0001f;


void CalcBillboardDirections(
	VEC3 *OutRight,
	VEC3 *OutUp,
	uint DegreesOfFreedom,
	bool UseRealDir,
	const VEC3 &ObjectPos,
	const VEC3 &DefinedRight,
	const VEC3 &DefinedUp,
	const VEC3 &EyePos,
	const VEC3 &CamRightDir,
	const VEC3 &CamRealUpDir)
{
	if (DegreesOfFreedom == 0)
	{
		*OutRight = DefinedRight;
		*OutUp = DefinedUp;
	}
	else if (DegreesOfFreedom == 1)
	{
		*OutUp = DefinedUp;

		if (UseRealDir)
		{
			VEC3 Forward = ObjectPos - EyePos;
			Normalize(&Forward);
			Cross(OutRight, Forward, DefinedUp);
			*OutRight = - *OutRight;
			Normalize(OutRight);
		}
		else
			*OutRight = CamRightDir;
	}
	else // (DegreesOfFreedom == 2)
	{
		if (UseRealDir)
		{
			VEC3 Forward = ObjectPos - EyePos;
			Normalize(&Forward);

			// Tu Up wykorzystany tylko tymczasowo, wyliczam Right
			*OutUp = VEC3::POSITIVE_Y;
			if (fabsf(Dot(*OutUp, Forward)) > 0.99f)
				*OutUp = VEC3::POSITIVE_Z;
			Cross(OutRight, Forward, *OutUp);
			*OutRight = - *OutRight;
			Normalize(OutRight);

			// Tu wyliczam docelowe Up
			Cross(OutUp, Forward, *OutRight);
			Normalize(OutUp);
		}
		else
		{
			*OutRight = CamRightDir;
			*OutUp = CamRealUpDir;
		}
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa QMeshEntity

QMeshEntity::QMeshEntity(Scene *OwnerScene) :
	MaterialEntity(OwnerScene),
	m_MeshRes(NULL),
	m_PrevAnim(MAXUINT4),
	m_CurrAnim(MAXUINT4),
	m_NextAnim(MAXUINT4)
{
	SetRadius(0.f);
}

QMeshEntity::QMeshEntity(Scene *OwnerScene, const string &MeshResName) :
	MaterialEntity(OwnerScene),
	m_MeshResName(MeshResName),
	m_MeshRes(NULL),
	m_PrevAnim(MAXUINT4),
	m_CurrAnim(MAXUINT4),
	m_NextAnim(MAXUINT4)
{
	if (EnsureMeshRes())
		SetRadius(m_MeshRes->GetBoundingSphereRadius());
}

void QMeshEntity::SetMeshResName(const string &MeshResName)
{
	if (MeshResName != m_MeshResName)
	{
		m_MeshResName = MeshResName;
		m_MeshRes = NULL;
		m_PrevAnim = MAXUINT4;
		m_CurrAnim = MAXUINT4;
		m_NextAnim = MAXUINT4;

		if (EnsureMeshRes())
			SetRadius(m_MeshRes->GetBoundingSphereRadius());
		else
			SetRadius(0.f);
	}
}

void QMeshEntity::SetAnimation(const string &AnimationName, uint Mode, float StartTime, float Speed)
{
	if (!EnsureMeshRes()) return;

	m_PrevAnim = MAXUINT4;
	m_NextAnim = MAXUINT4;

	m_CurrAnim = m_MeshRes->GetAnimationIndexByName(AnimationName);
	if (m_CurrAnim != MAXUINT4)
	{
		m_CurrAnimMode = Mode;
		m_CurrAnimTime = StartTime;
		m_CurrAnimSpeed = Speed;
	}
}

void QMeshEntity::BlendAnimation(const string &AnimationName, float BlendTime, uint Mode, float StartTime, float Speed)
{
	if (BlendTime <= 0.f)
		SetAnimation(AnimationName, Mode, StartTime, Speed);

	if (!EnsureMeshRes()) return;

	m_NextAnim = MAXUINT4;
	
	m_PrevAnim = m_CurrAnim;
	if (m_PrevAnim != MAXUINT4)
	{
		m_PrevAnimTime = m_CurrAnimTime;
		m_PrevAnimSpeed = m_CurrAnimSpeed;
		m_PrevAnimMode = m_CurrAnimMode;
	}

	m_CurrAnim = m_MeshRes->GetAnimationIndexByName(AnimationName);
	if (m_CurrAnim != MAXUINT4)
	{
		m_CurrAnimMode = Mode;
		m_CurrAnimTime = StartTime;
		m_CurrAnimSpeed = Speed;
	}

	m_BlendTime = 0.f;
	m_BlendEndTime = BlendTime;
}

void QMeshEntity::QueueAnimation(const string &AnimationName, uint Mode, float StartTime, float Speed)
{
	if (!EnsureMeshRes()) return;

	if (m_CurrAnim == MAXUINT4)
		SetAnimation(AnimationName, Mode, StartTime, Speed);
	else
	{
		m_NextAnim = m_MeshRes->GetAnimationIndexByName(AnimationName);
		if (m_NextAnim != MAXUINT4)
		{
			m_NextAnimMode = Mode;
			m_NextAnimTime = StartTime;
			m_NextAnimSpeed = Speed;
			m_NextAnimBlend = false;
		}
	}
}

void QMeshEntity::QueueBlendAnimation(const string &AnimationName, float BlendTime, uint Mode, float StartTime, float Speed)
{
	if (BlendTime <= 0.f)
		QueueAnimation(AnimationName, Mode, StartTime, Speed);

	if (!EnsureMeshRes()) return;

	if (m_CurrAnim == MAXUINT4)
		BlendAnimation(AnimationName, BlendTime, Mode, StartTime, Speed);
	else
	{
		m_NextAnim = m_MeshRes->GetAnimationIndexByName(AnimationName);
		if (m_NextAnim != MAXUINT4)
		{
			m_NextAnimMode = Mode;
			m_NextAnimTime = StartTime;
			m_NextAnimSpeed = Speed;
			m_NextAnimBlend = true;
			m_NextAnimBlendTime = BlendTime;
		}
	}
}

void QMeshEntity::ResetAnimation()
{
	m_PrevAnim = m_NextAnim = m_CurrAnim = MAXUINT4;
}

void QMeshEntity::SetAnimationTime(float Time)
{
	m_CurrAnimTime = Time;
}

void QMeshEntity::SetAnimationSpeed(float Speed)
{
	m_CurrAnimSpeed = Speed;
}

void QMeshEntity::SetAnimationMode(uint Mode)
{
	m_CurrAnimMode = Mode;
}

void QMeshEntity::SetCustomMaterial(uint SubmeshIndex, const string &MaterialName)
{
	if (m_CustomMaterialNames.size() <= SubmeshIndex)
		m_CustomMaterialNames.resize(SubmeshIndex+1);
	m_CustomMaterialNames[SubmeshIndex] = MaterialName;

	// Zresetuj cache wskaŸników do materia³ów
	m_Materials.clear();
}

void QMeshEntity::SetAllCustomMaterials(const string &MaterialName)
{
	if (!EnsureMeshRes())
		return;

	m_CustomMaterialNames.resize(m_MeshRes->GetSubmeshCount());
	for (uint si = 0; si < m_CustomMaterialNames.size(); si++)
		m_CustomMaterialNames[si] = MaterialName;

	// Zresetuj cache wskaŸników do materia³ów
	m_Materials.clear();
}

void QMeshEntity::ResetCustomMaterial(uint SubmeshIndex)
{
	if (m_CustomMaterialNames.size() > SubmeshIndex)
		m_CustomMaterialNames[SubmeshIndex].clear();

	// Zresetuj cache wskaŸników do materia³ów
	m_Materials.clear();
}

void QMeshEntity::ResetAllCustomMaterials()
{
	m_CustomMaterialNames.clear();

	// Zresetuj cache wskaŸników do materia³ów
	m_Materials.clear();
}

void QMeshEntity::Update()
{
	if (!EnsureMeshRes()) return;

	float dt = frame::Timer2.GetDeltaTime();

	if (m_CurrAnim != MAXUINT4)
	{
		// Przesuniêcie do przodu czasu
		m_CurrAnimTime += dt * m_CurrAnimSpeed;

		// Koniec bie¿¹cej animacji i pocz¹tek nastêpnej
		if (m_NextAnim != MAXUINT4 && m_CurrAnimTime > m_MeshRes->GetAnimation(m_CurrAnim).GetLength() * ModeToLengthFactor(m_CurrAnimMode))
		{
			// Next chce byæ blendowany: prev = curr, curr = next
			// Next nie chce byæ blendowany: prev zostaje taki jaki by³
			if (m_NextAnimBlend)
			{
				m_PrevAnim = m_CurrAnim; LOG(1024, Format("Assigning Prev=Curr = #") % m_MeshRes->GetAnimation(m_PrevAnim).GetName());
				if (m_PrevAnim != MAXUINT4)
				{
					m_PrevAnimTime = m_CurrAnimTime;
					m_PrevAnimSpeed = m_CurrAnimSpeed;
					m_PrevAnimMode = m_CurrAnimMode;
				}
				m_BlendTime = 0.f;
				m_BlendEndTime = m_NextAnimBlendTime;
			}

			m_CurrAnim = m_NextAnim;
			if (m_CurrAnim != MAXUINT4)
			{
				m_CurrAnimMode = m_NextAnimMode;
				m_CurrAnimTime = m_NextAnimTime;
				m_CurrAnimSpeed = m_NextAnimSpeed;
			}
			m_NextAnim = MAXUINT4;
		}

		// Posuniêcie do przodu blendingu
		if (m_PrevAnim != MAXUINT4)
		{
			m_PrevAnimTime += dt * m_PrevAnimSpeed;
			m_BlendTime += dt;
			if (m_BlendTime > m_BlendEndTime)
				m_PrevAnim = MAXUINT4;
		}
	}
}

bool QMeshEntity::EnsureMeshAndMaterials()
{
	if (GetMeshRes() == NULL)
		return false;

	GetMeshRes()->Load();

	if (m_Materials.empty())
	{
		m_Materials.resize(GetMeshRes()->GetSubmeshCount());
		for (uint si = 0; si < GetMeshRes()->GetSubmeshCount(); si++)
		{
			string MatName;
			if (m_CustomMaterialNames.size() > si && !m_CustomMaterialNames[si].empty())
				m_Materials[si] = GetOwnerScene()->GetMaterialCollection().MustGetByName(m_CustomMaterialNames[si]);
			else
				m_Materials[si] = GetOwnerScene()->GetMaterialCollection().MustGetByName(GetMeshRes()->GetSubmesh(si).MaterialName);
		}
	}

	return true;
}

uint QMeshEntity::GetFragmentCount()
{
	if (EnsureMeshAndMaterials())
		return m_Materials.size();
	else
		return 0;
}

void QMeshEntity::GetFragmentData(uint Index, uint *OutId, BaseMaterial **OutMaterial)
{
	assert(EnsureMeshAndMaterials());

	*OutId = Index;
	*OutMaterial = m_Materials[Index];
}

void QMeshEntity::DrawFragmentGeometry(uint Id, BaseMaterial *Mat, const ParamsCamera &Cam)
{
	assert(EnsureMeshAndMaterials());

	frame::Dev->SetFVF(GetMeshRes()->GetFVF());
	frame::Dev->SetStreamSource(0, GetMeshRes()->GetVB(), 0, GetMeshRes()->GetVertexSize());
	frame::Dev->SetIndices(GetMeshRes()->GetIB());

	assert(Id < GetMeshRes()->GetSubmeshCount());
	const res::QMesh::SUBMESH & submesh = GetMeshRes()->GetSubmesh(Id);

	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST,
		0,
		submesh.MinVertexIndexUsed,
		submesh.NumVertexIndicesUsed,
		submesh.FirstTriangle*3,
		submesh.NumTriangles) );
	frame::RegisterDrawCall(submesh.NumTriangles);
}

bool QMeshEntity::GetSkinningData(uint *OutBoneCount, const MATRIX **OutBoneMatrices)
{
	if (!EnsureMeshAndMaterials())
		return false;

	if (!m_MeshRes->HasSkinning())
		return false;
	// Nie ma animacji
	if (m_CurrAnim == MAXUINT4)
		return false;

	// Jest blending miêdzy dwiema animacjami
	if (m_PrevAnim != MAXUINT4)
	{
		res::QMesh::Animation &Anim1 = m_MeshRes->GetAnimation(m_PrevAnim);
		res::QMesh::Animation &Anim2 = m_MeshRes->GetAnimation(m_CurrAnim);
		float AnimTime1 = ProcessAnimTime(m_PrevAnimTime, m_PrevAnimMode, Anim1.GetLength());
		float AnimTime2 = ProcessAnimTime(m_CurrAnimTime, m_CurrAnimMode, Anim2.GetLength());
		assert(m_BlendEndTime > 0.f);
		float LerpT = m_BlendTime / m_BlendEndTime;

		*OutBoneMatrices = m_MeshRes->GetBoneMatrices(ANIMATION_ACCURACY*std::min(m_PrevAnimSpeed, m_CurrAnimSpeed), m_PrevAnim, AnimTime1, m_CurrAnim, AnimTime2, LerpT);
	}
	// Jest jedna animacja
	else
	{
		res::QMesh::Animation &Anim = m_MeshRes->GetAnimation(m_CurrAnim);
		float AnimTime = ProcessAnimTime(m_CurrAnimTime, m_CurrAnimMode, Anim.GetLength());

		*OutBoneMatrices = m_MeshRes->GetBoneMatrices(ANIMATION_ACCURACY*m_CurrAnimSpeed, m_CurrAnim, AnimTime);
	}

	*OutBoneCount = m_MeshRes->GetBoneCount();
	return true;
}

bool QMeshEntity::RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT)
{
	if (!EnsureMeshAndMaterials())
		return false;

	*OutT = MAXFLOAT;
	float t;
	bool Found = true;
	bool TwoSided;

	// Nie ma animacji
	if (!m_MeshRes->HasSkinning() || m_CurrAnim == MAXUINT4)
	{
		// Dla ka¿dej podsiatki
		for (uint i = 0; i < m_Materials.size(); i++)
		{
			if (m_Materials[i]->CheckCollisionType(Type))
			{
				TwoSided = m_Materials[i]->GetTwoSided();
				const res::QMesh::SUBMESH & submesh = m_MeshRes->GetSubmesh(i);
				if (m_MeshRes->RayCollision(&t, RayOrig, RayDir, TwoSided, submesh.FirstTriangle, submesh.FirstTriangle + submesh.NumTriangles))
				{
					if (t >= 0.f && t < *OutT)
					{
						*OutT = t;
						Found = true;
					}
				}
			}
		}
	}
	// Jest animacja
	else
	{
		// Jest blending miêdzy dwiema animacjami
		if (m_PrevAnim != MAXUINT4)
		{
			res::QMesh::Animation &Anim1 = m_MeshRes->GetAnimation(m_PrevAnim);
			res::QMesh::Animation &Anim2 = m_MeshRes->GetAnimation(m_CurrAnim);
			float AnimTime1 = ProcessAnimTime(m_PrevAnimTime, m_PrevAnimMode, Anim1.GetLength());
			float AnimTime2 = ProcessAnimTime(m_CurrAnimTime, m_CurrAnimMode, Anim2.GetLength());
			assert(m_BlendEndTime > 0.f);
			float LerpT = m_BlendTime / m_BlendEndTime;

			// Dla ka¿dej podsiatki
			for (uint i = 0; i < m_Materials.size(); i++)
			{
				if (m_Materials[i]->CheckCollisionType(Type))
				{
					TwoSided = m_Materials[i]->GetTwoSided();
					const res::QMesh::SUBMESH & submesh = m_MeshRes->GetSubmesh(i);
					if (m_MeshRes->RayCollision_Bones(&t, RayOrig, RayDir, TwoSided, ANIMATION_ACCURACY*std::min(m_PrevAnimSpeed, m_CurrAnimSpeed), m_PrevAnim, AnimTime1, m_CurrAnim, AnimTime2, LerpT, submesh.FirstTriangle, submesh.FirstTriangle + submesh.NumTriangles))
					{
						if (t >= 0.f && t < *OutT)
						{
							*OutT = t;
							Found = true;
						}
					}
				}
			}
		}
		// Jest jedna animacja
		else
		{
			res::QMesh::Animation &Anim = m_MeshRes->GetAnimation(m_CurrAnim);
			float AnimTime = ProcessAnimTime(m_CurrAnimTime, m_CurrAnimMode, Anim.GetLength());

			// Dla ka¿dej podsiatki
			for (uint i = 0; i < m_Materials.size(); i++)
			{
				if (m_Materials[i]->CheckCollisionType(Type))
				{
					TwoSided = m_Materials[i]->GetTwoSided();
					const res::QMesh::SUBMESH & submesh = m_MeshRes->GetSubmesh(i);
					if (m_MeshRes->RayCollision_Bones(&t, RayOrig, RayDir, TwoSided, ANIMATION_ACCURACY*m_CurrAnimSpeed, m_CurrAnim, AnimTime, submesh.FirstTriangle, submesh.FirstTriangle + submesh.NumTriangles))
					{
						if (t >= 0.f && t < *OutT)
						{
							*OutT = t;
							Found = true;
						}
					}
				}
			}
		}
	}

	return Found;
}

const MATRIX & QMeshEntity::GetBoneMatrix(const string &BoneName)
{
	if (!EnsureMeshRes())
		return MATRIX::IDENTITY;

	const MATRIX *Matrices;
	uint BoneCount;
	if (!GetSkinningData(&BoneCount, &Matrices))
		return MATRIX::IDENTITY;

	for (uint bi = 0; bi < m_MeshRes->GetBoneCount(); bi++)
		if (m_MeshRes->GetBone(bi).Name == BoneName)
			return Matrices[bi];

	return MATRIX::IDENTITY;
}

res::QMesh * QMeshEntity::GetMeshRes()
{
	if (m_MeshRes != NULL)
		return m_MeshRes;
	if (m_MeshResName.empty())
		return NULL;

	m_MeshRes = res::g_Manager->MustGetResourceEx<res::QMesh>(m_MeshResName);
	return m_MeshRes;
}

bool QMeshEntity::EnsureMeshRes()
{
	if (GetMeshRes())
	{
		m_MeshRes->Load();
		return true;
	}
	else
		return false;
}

float QMeshEntity::ProcessAnimTime(float AnimTime, uint Mode, float AnimLength)
{
	if ((Mode & ANIM_MODE_LOOP) != 0)
		AnimTime = fmodf(AnimTime, AnimLength * ModeToLengthFactor(Mode));

	if ((Mode & ANIM_MODE_BACKWARDS) != 0)
		AnimTime = AnimLength - AnimTime;
	else if ((Mode & ANIM_MODE_BIDI) != 0)
	{
		if (AnimTime > AnimLength)
			AnimTime = 2.f * AnimLength - AnimTime;
	}

	return AnimTime;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa BoxEntity

const BOX BOX_ENTITY_BOX = BOX(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f);

const VERTEX_XN233 BOX_VB[] = {
	// Near
	VERTEX_XN233(VEC3(-1.0f, -1.0f, -1.0f), VEC3::NEGATIVE_Z, VEC2(0.0f, 1.0f), VEC3::POSITIVE_X, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3( 1.0f, -1.0f, -1.0f), VEC3::NEGATIVE_Z, VEC2(1.0f, 1.0f), VEC3::POSITIVE_X, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3(-1.0f,  1.0f, -1.0f), VEC3::NEGATIVE_Z, VEC2(0.0f, 0.0f), VEC3::POSITIVE_X, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3( 1.0f,  1.0f, -1.0f), VEC3::NEGATIVE_Z, VEC2(1.0f, 0.0f), VEC3::POSITIVE_X, VEC3::POSITIVE_Y),
	// Far
	VERTEX_XN233(VEC3( 1.0f, -1.0f,  1.0f), VEC3::POSITIVE_Z, VEC2(0.0f, 1.0f), VEC3::NEGATIVE_X, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3(-1.0f, -1.0f,  1.0f), VEC3::POSITIVE_Z, VEC2(1.0f, 1.0f), VEC3::NEGATIVE_X, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3( 1.0f,  1.0f,  1.0f), VEC3::POSITIVE_Z, VEC2(0.0f, 0.0f), VEC3::NEGATIVE_X, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3(-1.0f,  1.0f,  1.0f), VEC3::POSITIVE_Z, VEC2(1.0f, 0.0f), VEC3::NEGATIVE_X, VEC3::POSITIVE_Y),
	// Left
	VERTEX_XN233(VEC3(-1.0f, -1.0f,  1.0f), VEC3::NEGATIVE_X, VEC2(0.0f, 1.0f), VEC3::NEGATIVE_Z, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3(-1.0f, -1.0f, -1.0f), VEC3::NEGATIVE_X, VEC2(1.0f, 1.0f), VEC3::NEGATIVE_Z, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3(-1.0f,  1.0f,  1.0f), VEC3::NEGATIVE_X, VEC2(0.0f, 0.0f), VEC3::NEGATIVE_Z, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3(-1.0f,  1.0f, -1.0f), VEC3::NEGATIVE_X, VEC2(1.0f, 0.0f), VEC3::NEGATIVE_Z, VEC3::POSITIVE_Y),
	// Right
	VERTEX_XN233(VEC3( 1.0f, -1.0f, -1.0f), VEC3::POSITIVE_X, VEC2(0.0f, 1.0f), VEC3::POSITIVE_Z, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3( 1.0f, -1.0f,  1.0f), VEC3::POSITIVE_X, VEC2(1.0f, 1.0f), VEC3::POSITIVE_Z, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3( 1.0f,  1.0f, -1.0f), VEC3::POSITIVE_X, VEC2(0.0f, 0.0f), VEC3::POSITIVE_Z, VEC3::POSITIVE_Y),
	VERTEX_XN233(VEC3( 1.0f,  1.0f,  1.0f), VEC3::POSITIVE_X, VEC2(1.0f, 0.0f), VEC3::POSITIVE_Z, VEC3::POSITIVE_Y),
	// Bottom
	VERTEX_XN233(VEC3(-1.0f, -1.0f,  1.0f), VEC3::NEGATIVE_Y, VEC2(0.0f, 1.0f), VEC3::POSITIVE_X, VEC3::NEGATIVE_Z),
	VERTEX_XN233(VEC3( 1.0f, -1.0f,  1.0f), VEC3::NEGATIVE_Y, VEC2(1.0f, 1.0f), VEC3::POSITIVE_X, VEC3::NEGATIVE_Z),
	VERTEX_XN233(VEC3(-1.0f, -1.0f, -1.0f), VEC3::NEGATIVE_Y, VEC2(0.0f, 0.0f), VEC3::POSITIVE_X, VEC3::NEGATIVE_Z),
	VERTEX_XN233(VEC3( 1.0f, -1.0f, -1.0f), VEC3::NEGATIVE_Y, VEC2(1.0f, 0.0f), VEC3::POSITIVE_X, VEC3::NEGATIVE_Z),
	// Top
	VERTEX_XN233(VEC3(-1.0f,  1.0f, -1.0f), VEC3::POSITIVE_Y, VEC2(0.0f, 1.0f), VEC3::POSITIVE_X, VEC3::NEGATIVE_Z),
	VERTEX_XN233(VEC3( 1.0f,  1.0f, -1.0f), VEC3::POSITIVE_Y, VEC2(1.0f, 1.0f), VEC3::POSITIVE_X, VEC3::NEGATIVE_Z),
	VERTEX_XN233(VEC3(-1.0f,  1.0f,  1.0f), VEC3::POSITIVE_Y, VEC2(0.0f, 0.0f), VEC3::POSITIVE_X, VEC3::NEGATIVE_Z),
	VERTEX_XN233(VEC3( 1.0f,  1.0f,  1.0f), VEC3::POSITIVE_Y, VEC2(1.0f, 0.0f), VEC3::POSITIVE_X, VEC3::NEGATIVE_Z),
};

const uint2 BOX_IB[] = {
	// Near
	0, 2, 3, 0, 3, 1,
	// Far
	4, 6, 7, 4, 7, 5,
	// Left
	8, 10, 11, 8, 11, 9,
	// Right
	12, 14, 15, 12, 15, 13,
	// Bottom
	16, 18, 19, 16, 19, 17,
	// Top
	20, 22, 23, 20, 23, 21,
};

BoxEntity::BoxEntity(Scene *OwnerScene) :
	MaterialEntity(OwnerScene),
	m_Material(NULL)
{
	// Przek¹tna szeœcianu
	SetRadius(SQRT3);
}

BoxEntity::BoxEntity(Scene *OwnerScene, const string &MaterialName) :
	MaterialEntity(OwnerScene),
	m_MaterialName(MaterialName),
	m_Material(NULL)
{
	// Przek¹tna szeœcianu
	SetRadius(SQRT3);
}

bool BoxEntity::RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT)
{
	if (!EnsureMaterial())
		return false;
	if (!m_Material->CheckCollisionType(Type))
		return false;

	return RayToBox(OutT, RayOrig, RayDir, BOX_ENTITY_BOX);
}

uint BoxEntity::GetFragmentCount()
{
	if (!EnsureMaterial())
		return 0;

	return 1;
}

void BoxEntity::GetFragmentData(uint Index, uint *OutId, BaseMaterial **OutMaterial)
{
	assert(Index == 0);
	assert(EnsureMaterial());
	*OutId = 0;
	*OutMaterial = m_Material;
}

void BoxEntity::DrawFragmentGeometry(uint Id, BaseMaterial *Mat, const ParamsCamera &Cam)
{
	frame::Dev->SetFVF(VERTEX_XN233::FVF);
	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 24, 12, BOX_IB, D3DFMT_INDEX16, BOX_VB, sizeof(VERTEX_XN233)) );
	frame::RegisterDrawCall(12);
}

bool BoxEntity::EnsureMaterial()
{
	if (m_Material == NULL)
	{
		if (m_MaterialName.empty())
			return false;
		// Tu napisa³em tak, ¿e jeœli taki materia³ nie istnieje to tak jakby by³ nieustawiony a nie b³¹d programu.
		m_Material = GetOwnerScene()->GetMaterialCollection().GetByName(m_MaterialName);
		return (m_Material != NULL);
	}
	else
		return true;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa QuadEntity

const uint2 QUAD_IB[] = { 0, 2, 3, 0, 3, 1 };


QuadEntity::QuadEntity(Scene *OwnerScene) :
	MaterialEntity(OwnerScene),
	m_DegreesOfFreedom(0),
	m_UseRealDir(false),
	m_RightDir(VEC3::POSITIVE_X),
	m_UpDir(VEC3::POSITIVE_Y),
	m_Tex(RECTF(0.0f, 0.0f, 1.0f, 1.0f)),
	m_HalfSize(VEC2::ONE),
	m_Material(NULL)
{
	UpdateRadius();
}

QuadEntity::QuadEntity(Scene *OwnerScene, const string &MaterialName) :
	MaterialEntity(OwnerScene),
	m_MaterialName(MaterialName),
	m_DegreesOfFreedom(0),
	m_UseRealDir(false),
	m_RightDir(VEC3::POSITIVE_X),
	m_UpDir(VEC3::POSITIVE_Y),
	m_Tex(RECTF(0.0f, 0.0f, 1.0f, 1.0f)),
	m_HalfSize(VEC2::ONE),
	m_Material(NULL)
{
	UpdateRadius();
	UpdateVB();
}

void QuadEntity::SetDegreesOfFreedom(uint DegreesOfFreedom)
{
	if (DegreesOfFreedom != m_DegreesOfFreedom)
	{
		assert(DegreesOfFreedom < 3);
		m_DegreesOfFreedom = DegreesOfFreedom;
		UpdateVB();
	}
}

void QuadEntity::SetUseRealDir(bool UseRealDir)
{
	m_UseRealDir = UseRealDir;
}

void QuadEntity::SetRightDir(const VEC3 &RightDir)
{
	if (RightDir != m_RightDir)
	{
		m_RightDir = RightDir;
		UpdateVB();
	}
}

void QuadEntity::SetUpDir(const VEC3 &UpDir)
{
	if (UpDir != m_UpDir)
	{
		m_UpDir = UpDir;
		UpdateVB();
	}
}

void QuadEntity::SetTex(const RECTF &Tex)
{
	if (Tex != m_Tex)
	{
		m_Tex = Tex;
		UpdateVB();
	}
}

void QuadEntity::SetHalfSize(const VEC2 &HalfSize)
{
	if (HalfSize != m_HalfSize)
	{
		m_HalfSize = HalfSize;
		UpdateVB();
		UpdateRadius();
	}
}

bool QuadEntity::RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT)
{
	if (!EnsureMaterial())
		return false;
	if (!m_Material->CheckCollisionType(Type))
		return false;

	FillVbForFrame();

	if (RayToTriangle(RayOrig, RayDir, m_VB[0].Pos, m_VB[2].Pos, m_VB[3].Pos, !m_Material->GetTwoSided(), OutT))
		return true;
	if (RayToTriangle(RayOrig, RayDir, m_VB[0].Pos, m_VB[3].Pos, m_VB[1].Pos, !m_Material->GetTwoSided(), OutT))
		return true;

	return false;
}

uint QuadEntity::GetFragmentCount()
{
	return EnsureMaterial() ? 1 : 0;
}

void QuadEntity::GetFragmentData(uint Index, uint *OutId, BaseMaterial **OutMaterial)
{
	assert(Index == 0);
	assert(EnsureMaterial());
	*OutId = 0;
	*OutMaterial = m_Material;
}

void QuadEntity::DrawFragmentGeometry(uint Id, BaseMaterial *Mat, const ParamsCamera &Cam)
{
	if (!EnsureMaterial()) return;
	FillVbForFrame();

	frame::Dev->SetFVF(VERTEX_XN233::FVF);
	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, QUAD_IB, D3DFMT_INDEX16, m_VB, sizeof(VERTEX_XN233)) );
	frame::RegisterDrawCall(2);
}

bool QuadEntity::EnsureMaterial()
{
	if (m_Material == NULL)
	{
		if (m_MaterialName.empty())
			return false;
		// Tu napisa³em tak, ¿e jeœli taki materia³ nie istnieje to tak jakby by³ nieustawiony a nie b³¹d programu.
		m_Material = GetOwnerScene()->GetMaterialCollection().GetByName(m_MaterialName);
		return (m_Material != NULL);
	}
	else
		return true;
}

void QuadEntity::UpdateRadius()
{
	SetRadius(Length(m_HalfSize));
}

void QuadEntity::UpdateVB()
{
	// Wspó³rzêdne tekstury
	m_VB[0].Tex.x = m_Tex.left;
	m_VB[0].Tex.y = m_Tex.bottom;
	m_VB[1].Tex.x = m_Tex.right;
	m_VB[1].Tex.y = m_Tex.bottom;
	m_VB[2].Tex.x = m_Tex.left;
	m_VB[2].Tex.y = m_Tex.top;
	m_VB[3].Tex.x = m_Tex.right;
	m_VB[3].Tex.y = m_Tex.top;

	// Jeœli 0 stopni swobody, Pos, Normal, Tangent i Binormal
	if (GetDegreesOfFreedom() == 0)
		FillVbPosNormals(m_RightDir, m_UpDir);
}

void QuadEntity::FillVbForFrame()
{
	// Jeœli nie 0 stopni swobody, trzeba w ka¿dej klatce wype³niaæ Pos, Normal, Tangent i Binormal
	if (m_DegreesOfFreedom > 0)
	{
		VEC3 Right, Up;
		VEC3 EyePos, CamRightDir, CamRealUpDir;
		const MATRIX &InvWorld = GetInvWorldMatrix();
		const ParamsCamera &Cam = GetOwnerScene()->GetActiveCamera()->GetParams();
		TransformNormal(&CamRightDir, Cam.GetRightDir(), InvWorld);
		TransformNormal(&CamRealUpDir, Cam.GetRealUpDir(), InvWorld);
		Transform(&EyePos, Cam.GetEyePos(), InvWorld);
		CalcBillboardDirections(&Right, &Up, m_DegreesOfFreedom, m_UseRealDir, VEC3::ZERO, m_RightDir, m_UpDir, EyePos, CamRightDir, CamRealUpDir);
		FillVbPosNormals(Right, Up);
	}
}

void QuadEntity::FillVbPosNormals(const VEC3 &Right, const VEC3 &Up)
{
	// Pos
	m_VB[0].Pos = - Right * m_HalfSize.x - Up * m_HalfSize.y;
	m_VB[1].Pos =   Right * m_HalfSize.x - Up * m_HalfSize.y;
	m_VB[2].Pos = - Right * m_HalfSize.x + Up * m_HalfSize.y;
	m_VB[3].Pos =   Right * m_HalfSize.x + Up * m_HalfSize.y;

	// Normal
	VEC3 Normal;
	Cross(&Normal, Right, Up);
	m_VB[0].Normal = m_VB[1].Normal = m_VB[2].Normal = m_VB[3].Normal = - Normal;

	// Tangent, Binormal
	m_VB[0].Tangent  = m_VB[1].Tangent  = m_VB[2].Tangent  = m_VB[3].Tangent  = Right * (m_Tex.right > m_Tex.left ?  1.0f : -1.0f);
	m_VB[0].Binormal = m_VB[1].Binormal = m_VB[2].Binormal = m_VB[3].Binormal = Up    * (m_Tex.bottom > m_Tex.top ? -1.0f :  1.0f);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TextEntity

TextEntity::TextEntity(Scene *OwnerScene) :
	CustomEntity(OwnerScene),
	m_DegreesOfFreedom(0),
	m_UseRealDir(false),
	m_RightDir(VEC3::POSITIVE_X),
	m_UpDir(VEC3::POSITIVE_Y),
	m_Text("Text"),
	m_FontSize(0.1f),
	m_FontFlags(res::Font::FLAG_WRAP_SINGLELINE | res::Font::FLAG_HCENTER | res::Font::FLAG_VMIDDLE),
	m_TextWidth(1.0f),
	m_Color(COLOR::WHITE),
	m_BlendMode(BaseMaterial::BLEND_LERP),
	m_TwoSided(false),
	m_CollisionType(0),
	m_Font(NULL)
{
	ExtentChanged();
}

TextEntity::TextEntity(Scene *OwnerScene, const string &Text, const string &FontName, COLOR Color, float FontSize, uint FontFlags) :
	CustomEntity(OwnerScene),
	m_DegreesOfFreedom(0),
	m_UseRealDir(false),
	m_RightDir(VEC3::POSITIVE_X),
	m_UpDir(VEC3::POSITIVE_Y),
	m_Text(Text),
	m_FontName(FontName),
	m_FontSize(FontSize),
	m_FontFlags(FontFlags),
	m_TextWidth(1.0f),
	m_Color(Color),
	m_BlendMode(BaseMaterial::BLEND_LERP),
	m_TwoSided(false),
	m_CollisionType(0),
	m_Font(NULL)
{
	FontNameChanged();
}

void TextEntity::SetDegreesOfFreedom(uint DegreesOfFreedom)
{
	assert(m_DegreesOfFreedom < 3);
	m_DegreesOfFreedom = DegreesOfFreedom;
}

void TextEntity::SetUseRealDir(bool UseRealDir)
{
	m_UseRealDir = UseRealDir;
}

void TextEntity::SetRightDir(const VEC3 &RightDir)
{
	m_RightDir = RightDir;
}

void TextEntity::SetUpDir(const VEC3 &UpDir)
{
	m_UpDir = UpDir;
}

void TextEntity::SetText(const string &Text)
{
	if (Text != m_Text)
	{
		m_Text = Text;
		ExtentChanged();
	}
}

void TextEntity::SetFontName(const string &FontName)
{
	if (FontName != m_FontName)
	{
		m_FontName = FontName;
		FontNameChanged();
	}
}

void TextEntity::SetFontSize(float FontSize)
{
	if (FontSize != m_FontSize)
	{
		m_FontSize = FontSize;
		ExtentChanged();
	}
}

void TextEntity::SetFontFlags(uint FontFlags)
{
	if (FontFlags != m_FontFlags)
	{
		m_FontFlags = FontFlags;
		ExtentChanged();
	}
}

void TextEntity::SetTextWidth(float TextWidth)
{
	if (TextWidth != m_TextWidth)
	{
		m_TextWidth = TextWidth;
		ExtentChanged();
	}
}

void TextEntity::SetColor(COLOR Color)
{
	m_Color = Color;
}

void TextEntity::SetBlendMode(BaseMaterial::BLEND_MODE BlendMode)
{
	m_BlendMode = BlendMode;
}

void TextEntity::SetTwoSided(bool TwoSided)
{
	m_TwoSided = TwoSided;
}

void TextEntity::SetCollisionType(uint CollisionType)
{
	m_CollisionType = CollisionType;
}

bool TextEntity::RayCollision(COLLISION_TYPE Type, const VEC3 &RayOrig, const VEC3 &RayDir, float *OutT)
{
	if (m_Font == NULL) return false;
	if (m_Text.empty()) return false;
	if ((m_CollisionType & Type) == 0) return false;

	VEC3 Right, Up;
	VEC3 EyePos, CamRightDir, CamRealUpDir;
	const MATRIX &InvWorld = GetInvWorldMatrix();
	const ParamsCamera &Cam = GetOwnerScene()->GetActiveCamera()->GetParams();
	TransformNormal(&CamRightDir, Cam.GetRightDir(), InvWorld);
	TransformNormal(&CamRealUpDir, Cam.GetRealUpDir(), InvWorld);
	Transform(&EyePos, Cam.GetEyePos(), InvWorld);
	CalcBillboardDirections(&Right, &Up, m_DegreesOfFreedom, m_UseRealDir, VEC3::ZERO, m_RightDir, m_UpDir, EyePos, CamRightDir, CamRealUpDir);

	VEC3 Corners[4];
	Corners[0] = Right * m_MinExtent.x + Up * m_MinExtent.y;
	Corners[1] = Right * m_MaxExtent.x + Up * m_MinExtent.y;
	Corners[2] = Right * m_MinExtent.x + Up * m_MaxExtent.y;
	Corners[3] = Right * m_MaxExtent.x + Up * m_MaxExtent.y;

	if (RayToTriangle(RayOrig, RayDir, Corners[0], Corners[2], Corners[3], !m_TwoSided, OutT))
		return true;
	if (RayToTriangle(RayOrig, RayDir, Corners[0], Corners[3], Corners[1], !m_TwoSided, OutT))
		return true;

	return false;
}

void TextEntity::Draw(const ParamsCamera &Cam)
{
	ERR_TRY;

	if (m_Font == NULL) return;
	if (m_Text.empty()) return;
	if (m_Color.A == 0) return;

	m_Font->Load();

	VEC3 Right, Up;
	VEC3 EyePos, CamRightDir, CamRealUpDir;
	const MATRIX &InvWorld = GetInvWorldMatrix();
	TransformNormal(&CamRightDir, Cam.GetRightDir(), InvWorld);
	TransformNormal(&CamRealUpDir, Cam.GetRealUpDir(), InvWorld);
	Transform(&EyePos, Cam.GetEyePos(), InvWorld);
	CalcBillboardDirections(&Right, &Up, m_DegreesOfFreedom, m_UseRealDir, VEC3::ZERO, m_RightDir, m_UpDir, EyePos, CamRightDir, CamRealUpDir);

	frame::RestoreDefaultState();
	frame::Dev->SetRenderState(D3DRS_CULLMODE, m_TwoSided ? D3DCULL_NONE : D3DCULL_CCW);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	frame::Dev->SetRenderState(D3DRS_TEXTUREFACTOR, m_Color.ARGB);
	frame::Dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

	frame::Dev->SetTexture(0, m_Font->GetTexture());
	frame::Dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	frame::Dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	frame::Dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	frame::Dev->SetTransform(D3DTS_WORLD,      &math_cast<D3DXMATRIX>(GetWorldMatrix()));
	frame::Dev->SetTransform(D3DTS_VIEW,       &math_cast<D3DXMATRIX>(Cam.GetMatrices().GetView()));
	frame::Dev->SetTransform(D3DTS_PROJECTION, &math_cast<D3DXMATRIX>(Cam.GetMatrices().GetProj()));

	switch (m_BlendMode)
	{
	case WireFrameMaterial::BLEND_LERP:
		frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		break;
	case WireFrameMaterial::BLEND_ADD:
		frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		break;
	case WireFrameMaterial::BLEND_SUB:
		frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
		frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		break;
	default:
		assert(0);
	}

	m_Font->Draw3d(VEC3::ZERO, Right, -Up, m_Text, m_FontSize, m_FontFlags, m_TextWidth);

	ERR_CATCH_FUNC;
}

void TextEntity::ExtentChanged()
{
	if (m_Font == NULL)
		m_MinExtent = m_MaxExtent = VEC2::ZERO;
	else
	{
		float w, h;
		m_Font->Load();
		m_Font->GetTextExtent(&w, &h, m_Text, m_FontSize, m_FontFlags, m_TextWidth);

		if ((m_FontFlags & res::Font::FLAG_HLEFT) != 0)
		{
			m_MinExtent.x = 0.0f;
			m_MaxExtent.x = w;
		}
		else if ((m_FontFlags & res::Font::FLAG_HRIGHT) != 0)
		{
			m_MinExtent.x = - w;
			m_MaxExtent.x = 0.0f;
		}
		else
		{
			assert( ((m_FontFlags & res::Font::FLAG_HCENTER) != 0) && "TextEntity: Invalid FontFlags, missing FLAG_H" );
			m_MinExtent.x = - w * 0.5f;
			m_MaxExtent.x =   w * 0.5f;
		}

		if ((m_FontFlags & res::Font::FLAG_VBOTTOM) != 0)
		{
			m_MinExtent.y = 0.0f;
			m_MaxExtent.y = h;
		}
		else if ((m_FontFlags & res::Font::FLAG_VTOP) != 0)
		{
			m_MinExtent.y = - h;
			m_MaxExtent.y = 0.0f;
		}
		else
		{
			assert( ((m_FontFlags & res::Font::FLAG_VMIDDLE) != 0) && "TextEntity: Invalid FontFlags, missing FLAG_V" );
			m_MinExtent.y = - h * 0.5f;
			m_MaxExtent.y =   h * 0.5f;
		}
	}

	SetRadius(std::max(Length(m_MinExtent), Length(m_MaxExtent)));
}

void TextEntity::FontNameChanged()
{
	if (m_FontName.empty())
		m_Font = NULL;
	else
		// Mo¿e zwróciæ NULL - potrakujê to tak jakby m_FontName == "".
		m_Font = res::g_Manager->MustGetResourceEx<res::Font>(m_FontName);

	ExtentChanged();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa StripeEntity

const float STRIP_DEFAULT_HALF_WIDTH = 0.05f;
const uint STRIP_MAX_KEYPOINTS = 500;
const uint STRIP_VB_LENGTH = STRIP_MAX_KEYPOINTS * 2;

res::D3dVertexBuffer *StripeEntity::m_VB = NULL;


StripeEntity::StripeEntity(Scene *OwnerScene) :
	MaterialEntity(OwnerScene),
	m_Material(NULL),
	m_HalfWidth(STRIP_DEFAULT_HALF_WIDTH),
	m_Tex(RECTF(0.0f, 0.0f, 1.0f, 1.0f)),
	m_TexAnimVel(0.0f),
	m_TexRandomPeriod(MAXFLOAT),
	m_LastRandomTexTime(frame::Timer2.GetTime())
{
	CreateBufferIfNecessary();

	SetUseLighting(false);
	SetCastShadow(false);
	SetReceiveShadow(false);
}

StripeEntity::StripeEntity(Scene *OwnerScene, const string &MaterialName) :
	MaterialEntity(OwnerScene),
	m_MaterialName(MaterialName),
	m_Material(NULL),
	m_HalfWidth(STRIP_DEFAULT_HALF_WIDTH),
	m_Tex(RECTF(0.0f, 0.0f, 1.0f, 1.0f)),
	m_TexAnimVel(0.0f),
	m_TexRandomPeriod(MAXFLOAT),
	m_LastRandomTexTime(frame::Timer2.GetTime())
{
	CreateBufferIfNecessary();

	SetUseLighting(false);
	SetCastShadow(false);
	SetReceiveShadow(false);
}

StripeEntity::~StripeEntity()
{
	m_VB->Unlock();
}

void StripeEntity::Update()
{
	// Losowanie wspó³rzêdnych tekstury
	if (m_TexRandomPeriod < MAXFLOAT && m_LastRandomTexTime + m_TexRandomPeriod < frame::Timer2.GetTime())
	{
		float dty = g_Rand.RandFloat();
		m_Tex.top += dty;
		m_Tex.bottom += dty;

		m_LastRandomTexTime = frame::Timer2.GetTime();
	}
	// Animacja wspó³rzêdnych tekstury
	else if (m_TexAnimVel != 0.0f)
	{
		float dty = frame::Timer2.GetDeltaTime() * m_TexAnimVel;
		m_Tex.top += dty;
		m_Tex.bottom += dty;
	}
}

uint StripeEntity::GetFragmentCount()
{
	return (EnsureMaterial() ? 1 : 0);
}

void StripeEntity::GetFragmentData(uint Index, uint *OutId, BaseMaterial **OutMaterial)
{
	assert(Index == 0);
	assert(EnsureMaterial());

	*OutId = 0;
	*OutMaterial = m_Material;
}

void StripeEntity::DrawFragmentGeometry(uint Id, BaseMaterial *Mat, const ParamsCamera &Cam)
{
	ERR_TRY;

	assert(Id == 0);

	uint PointCount;
	bool Closed;
	const VEC3 *Points = GetStripPoints(&PointCount, &Closed);

	if (PointCount < 2) return;
	uint ClosedBonus = (Closed ? 1 : 0);

	VEC3 ForwardDir, RightDir;
	TransformNormal(&ForwardDir, Cam.GetForwardDir(), GetInvWorldMatrix());
	TransformNormal(&RightDir, Cam.GetRightDir(), GetInvWorldMatrix());

	uint vb_offset_in_bytes;
	// Nawias do zablokowania VB
	{
		res::D3dVertexBuffer::Locker vb_lock(m_VB, false, (PointCount + ClosedBonus) * 2);
		vb_offset_in_bytes = vb_lock.GetOffsetInBytes();
		VERTEX_X2 *Vertices = (VERTEX_X2*)vb_lock.GetData();
		VERTEX_X2 *VerticesBegin = Vertices;
		VEC3 Dir, NewDir, PrevDir, V;
		float ty = m_Tex.top;
		float ty_step = (m_Tex.bottom - m_Tex.top) / (PointCount - 1 + ClosedBonus);
		for (uint pi = 0; pi < PointCount; pi++)
		{
			// Krzywa zamkniêta
			if (Closed)
			{
				// Ostatni punkt
				if (pi == PointCount - 1)
					V = Points[0] - Points[pi];
				// Nie ostatni
				else
					V = Points[pi+1] - Points[pi];

				Normalize(&V); // Ta normalizacja jest wa¿na ¿eby mo¿na by³o dalej porównywaæ wynik Dot tego wektora! - naszuka³em siê tego b³êdu.

				if (fabsf(Dot(V, ForwardDir)) > 0.999f)
					Cross(&NewDir, V, RightDir);
				else
					Cross(&NewDir, V, ForwardDir);

				// Pierwszy punkt
				if (pi == 0)
				{
					// Trzeba wyliczyæ PrevDir
					V = Points[0] - Points[PointCount-1];
					Normalize(&V); // Ta normalizacja jest wa¿na ¿eby mo¿na by³o dalej porównywaæ wynik Dot tego wektora! - naszuka³em siê tego b³êdu.

					if (fabsf(Dot(V, ForwardDir)) > 0.999f)
						Cross(&PrevDir, V, RightDir);
					else
						Cross(&PrevDir, V, ForwardDir);
				}

				// Zabezpieczenie przeciwko zawijaniu siê - dzia³a!
				if (Dot(NewDir, PrevDir) < 0.0f)
					NewDir = -NewDir;
				Dir = PrevDir + NewDir;
			}
			// Krzywa otwarta
			else
			{
				// Ostatni punkt
				if (pi == PointCount - 1)
					Dir = PrevDir;
				// Nieostatni punkt
				else
				{
					V = Points[pi+1] - Points[pi];
					Normalize(&V); // Ta normalizacja jest wa¿na ¿eby mo¿na by³o dalej porównywaæ wynik Dot tego wektora! - naszuka³em siê tego b³êdu.

					if (fabsf(Dot(V, ForwardDir)) > 0.999f)
						Cross(&NewDir, V, RightDir);
					else
						Cross(&NewDir, V, ForwardDir);

					// Pierwszy punkt
					if (pi == 0)
						Dir = NewDir;
					// Nie pierwszy i nie ostatni
					else
					{
						// Zabezpieczenie przeciwko zawijaniu siê - dzia³a!
						if (Dot(NewDir, PrevDir) < 0.0f)
							NewDir = -NewDir;

						Dir = PrevDir + NewDir;
					}
				}
			}

			Normalize(&Dir);
			Dir *= m_HalfWidth;

			*Vertices = VERTEX_X2(Points[pi] - Dir, VEC2(m_Tex.left,  ty));
			Vertices++;
			*Vertices = VERTEX_X2(Points[pi] + Dir, VEC2(m_Tex.right, ty));
			Vertices++;

			ty += ty_step;
			PrevDir = NewDir;
		}

		// Zamkniêcie
		if (Closed)
		{
			Vertices->Pos = VerticesBegin[0].Pos;
			Vertices->Tex = VEC2(m_Tex.left,  ty);
			Vertices++;
			Vertices->Pos = VerticesBegin[1].Pos;
			Vertices->Tex = VEC2(m_Tex.right, ty);
		}
	}

	uint TriangleCount = (PointCount - 1 + ClosedBonus) * 2;
	frame::Dev->SetFVF(VERTEX_X2::FVF);
	frame::Dev->SetStreamSource(0, m_VB->GetVB(), vb_offset_in_bytes, sizeof(VERTEX_X2));
	frame::Dev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, TriangleCount);
	frame::RegisterDrawCall(TriangleCount);

	ERR_CATCH_FUNC;
}

void StripeEntity::CreateBufferIfNecessary()
{
	ERR_TRY;

	if (m_VB == NULL)
		m_VB = new res::D3dVertexBuffer(
			"StripeEntity_VB",
			string(),
			STRIP_VB_LENGTH,
			D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
			VERTEX_X2::FVF,
			D3DPOOL_DEFAULT);

	m_VB->Lock();

	ERR_CATCH_FUNC;
}

bool StripeEntity::EnsureMaterial()
{
	if (m_MaterialName.empty())
		return false;
	m_Material = GetOwnerScene()->GetMaterialCollection().GetByName(m_MaterialName);
	return (m_Material != NULL);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa LineStripeEntity

LineStripeEntity::LineStripeEntity(Scene *OwnerScene) :
	StripeEntity(OwnerScene)
{
}

LineStripeEntity::LineStripeEntity(Scene *OwnerScene, const string &MaterialName) :
	StripeEntity(OwnerScene, MaterialName)
{
}

LineStripeEntity::LineStripeEntity(Scene *OwnerScene, const string &MaterialName, const VEC3 &P1, const VEC3 &P2) :
	StripeEntity(OwnerScene, MaterialName)
{
	m_Points.push_back(P1);
	m_Points.push_back(P2);

	UpdateRadius();
}

LineStripeEntity::LineStripeEntity(Scene *OwnerScene, const string &MaterialName, const VEC3 *Points, uint PointCount) :
	StripeEntity(OwnerScene, MaterialName)
{
	m_Points.reserve(PointCount);
	for (uint i = 0; i < PointCount; i++)
		m_Points.push_back(Points[i]);

	UpdateRadius();
}

void LineStripeEntity::SetPoint(uint Index, const VEC3 &Point)
{
	assert(Index < m_Points.size());
	m_Points[Index] = Point;
	UpdateRadius();
}

void LineStripeEntity::SetPoints(const VEC3 *Points, uint PointCount)
{
	m_Points.resize(PointCount);
	CopyMem(&m_Points[0], Points, PointCount * sizeof(VEC3));
	UpdateRadius();
}
	
const VEC3 * LineStripeEntity::GetStripPoints(uint *OutPointCount, bool *OutClosed)
{
	*OutPointCount = m_Points.size();
	*OutClosed = false;
	return &m_Points[0];
}

void LineStripeEntity::UpdateRadius()
{
	float NewRadius;
	if (m_Points.size() == 0)
		NewRadius = 0.0f;
	else
	{
		float lsq = LengthSq(m_Points[0]);
		for (uint i = 1; i < m_Points.size(); i++)
			lsq = std::max(lsq, LengthSq(m_Points[i]));
		NewRadius = sqrtf(lsq) + GetHalfWidth();
	}

	// Zmieñ tylko jeœli trzeba zwiêkszyæ albo jeœli trzeba zmniejszyæ co najmniej dwa razy.
	if (GetRadius() < NewRadius || GetRadius() > NewRadius * 2.0f)
		SetRadius(NewRadius);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa EllipseStripeEntity

const uint ELLIPSE_STRIP_ENTITY_DEFAULT_SEGMENT_COUNT = 16;

EllipseStripeEntity::EllipseStripeEntity(Scene *OwnerScene) :
	StripeEntity(OwnerScene),
	m_SegmentCount(ELLIPSE_STRIP_ENTITY_DEFAULT_SEGMENT_COUNT),
	m_R1(VEC3::POSITIVE_X),
	m_R2(VEC3::POSITIVE_Y),
	m_PointsCalculated(false)
{
	UpdateRadius();
}

EllipseStripeEntity::EllipseStripeEntity(Scene *OwnerScene, const string &MaterialName, uint SegmentCount, const VEC3 &R1, const VEC3 &R2) :
	StripeEntity(OwnerScene, MaterialName),
	m_SegmentCount(SegmentCount),
	m_R1(R1),
	m_R2(R2),
	m_PointsCalculated(false)
{
	UpdateRadius();
}

void EllipseStripeEntity::SetSegmentCount(uint SegmentCount)
{
	m_SegmentCount = SegmentCount;
	m_PointsCalculated = false;
}

void EllipseStripeEntity::SetR1(const VEC3 &R1)
{
	m_R1 = R1;
	m_PointsCalculated = false;
	UpdateRadius();
}

void EllipseStripeEntity::SetR2(const VEC3 &R2)
{
	m_R2 = R2;
	m_PointsCalculated= false;
	UpdateRadius();
}

const VEC3 * EllipseStripeEntity::GetStripPoints(uint *OutPointCount, bool *OutClosed)
{
	if (!m_PointsCalculated)
	{
		UpdatePoints();
		m_PointsCalculated = true;
	}

	*OutPointCount = m_Points.size();
	*OutClosed = true;
	return &m_Points[0];
}

void EllipseStripeEntity::UpdateRadius()
{
	float NewRadius = GetHalfWidth() + std::max(
		Length(m_R1),
		Length(m_R2));
	// Zmieñ tylko jeœli trzeba zwiêkszyæ albo jeœli trzeba zmniejszyæ co najmniej dwa razy.
	if (GetRadius() < NewRadius || GetRadius() > NewRadius * 2.0f)
		SetRadius(NewRadius);
}

void EllipseStripeEntity::UpdatePoints()
{
	m_Points.resize(m_SegmentCount + 1);

	float a = 0.0f, a_step = PI_X_2 / (float)(m_SegmentCount + 1);
	for (uint i = 0; i <= m_SegmentCount; i++)
	{
		m_Points[i] = cosf(a) * m_R1 + sinf(a) * m_R2;
		a += a_step;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa UtermStripeEntity

const uint UTERM_STRIP_ENTITY_DEFAULT_SEGMENT_COUNT = 32;

UtermStripeEntity::UtermStripeEntity(Scene *OwnerScene) :
	StripeEntity(OwnerScene),
	m_SegmentCount(UTERM_STRIP_ENTITY_DEFAULT_SEGMENT_COUNT),
	m_Closed(false)
{
	UpdatePoints();
}

UtermStripeEntity::UtermStripeEntity(Scene *OwnerScene, const string &MaterialName, uint SegmentCount, const Uterm3 &Term) :
	StripeEntity(OwnerScene, MaterialName),
	m_SegmentCount(SegmentCount),
	m_Term(Term),
	m_Closed(false)
{
	UpdatePoints();
}

void UtermStripeEntity::SetSegmentCount(uint SegmentCount)
{
	m_SegmentCount = SegmentCount;
	UpdatePoints();
}

void UtermStripeEntity::SetTerm(const Uterm3 &Term)
{
	m_Term = Term;
	UpdatePoints();
}

const VEC3 * UtermStripeEntity::GetStripPoints(uint *OutPointCount, bool *OutClosed)
{
	*OutPointCount = m_Points.size();
	*OutClosed = m_Closed;
	return &m_Points[0];
}

void UtermStripeEntity::UpdatePoints()
{
	m_Points.resize(m_SegmentCount + 1);

	float t = 0.0f, t_step = 1.0f / (float)(m_SegmentCount + 1);
	for (uint i = 0; i <= m_SegmentCount; i++)
	{
		m_Term.Eval(&m_Points[i], t);
		t += t_step;
	}

	float lsq = LengthSq(m_Points[0]);
	for (uint i = 1; i <= m_SegmentCount; i++)
		lsq = std::max(lsq, LengthSq(m_Points[i]));
	float NewRadius = sqrtf(lsq) + GetHalfWidth();
	// Zmieñ tylko jeœli trzeba zwiêkszyæ albo jeœli trzeba zmniejszyæ co najmniej dwa razy.
	if (GetRadius() < NewRadius || GetRadius() > NewRadius * 2.0f)
		SetRadius(NewRadius);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa CurveStripeEntity

const uint CURVE_STRIP_ENTITY_DEFAULT_POINTS_PER_SEGMENT = 16;

CurveStripeEntity::CurveStripeEntity(Scene *OwnerScene) :
	StripeEntity(OwnerScene),
	m_CurveType(CURVE_TYPE_BSPLINE),
	m_PointsPerSegment(CURVE_STRIP_ENTITY_DEFAULT_POINTS_PER_SEGMENT),
	m_PointsCalculated(false)
{
}

CurveStripeEntity::CurveStripeEntity(Scene *OwnerScene, const string &MaterialName, CURVE_TYPE CurveType, uint PointsPerSegment) :
	StripeEntity(OwnerScene, MaterialName),
	m_CurveType(CurveType),
	m_PointsPerSegment(PointsPerSegment),
	m_PointsCalculated(false)
{
}

void CurveStripeEntity::SetupQuadratic(const VEC3 *ControlPoints, uint ControlPointCount)
{
	m_CurveType = CURVE_TYPE_QUADRATIC;
	m_ControlPoints.resize(ControlPointCount);
	CopyMem(&m_ControlPoints[0], ControlPoints, ControlPointCount * sizeof(VEC3));

	m_PointsCalculated = false;
	UpdateRadius();
}

void CurveStripeEntity::SetupBezier(const VEC3 *ControlPoints, uint ControlPointCount)
{
	m_CurveType = CURVE_TYPE_BEZIER;
	m_ControlPoints.resize(ControlPointCount);
	CopyMem(&m_ControlPoints[0], ControlPoints, ControlPointCount * sizeof(VEC3));

	m_PointsCalculated = false;
	UpdateRadius();
}

void CurveStripeEntity::SetupBspline(const VEC3 *ControlPoints, uint ControlPointCount)
{
	m_CurveType = CURVE_TYPE_BSPLINE;
	m_ControlPoints.resize(ControlPointCount);
	CopyMem(&m_ControlPoints[0], ControlPoints, ControlPointCount * sizeof(VEC3));

	m_PointsCalculated = false;
	UpdateRadius();
}

void CurveStripeEntity::SetCurveType(CURVE_TYPE CurveType)
{
	m_CurveType = CurveType;
	m_PointsCalculated = false;
}

void CurveStripeEntity::SetPointsPerSegment(uint PointsPerSegment)
{
	m_PointsPerSegment = PointsPerSegment;
	m_PointsCalculated = false;
}

void CurveStripeEntity::SetControlPointCount(uint ControlPointCount)
{
	m_ControlPoints.resize(ControlPointCount);
	m_PointsCalculated = false;
	UpdateRadius();
}

void CurveStripeEntity::SetAllControlPoints(const VEC3 *ControlPoints)
{
	assert(m_ControlPoints.size() > 0);
	CopyMem(&m_ControlPoints[0], ControlPoints, m_ControlPoints.size() * sizeof(VEC3));
	m_PointsCalculated = false;
	UpdateRadius();
}

void CurveStripeEntity::SetControlPoints(uint StartIndex, uint Count, const VEC3 *ControlPoints)
{
	assert(m_ControlPoints.size() > StartIndex + Count);
	CopyMem(&m_ControlPoints[StartIndex], ControlPoints, Count * sizeof(VEC3));
	m_PointsCalculated = false;
	UpdateRadius();
}

void CurveStripeEntity::SetControlPoint(uint Index, const VEC3 &ControlPoint)
{
	assert(Index < m_ControlPoints.size());
	m_ControlPoints[Index] = ControlPoint;
	m_PointsCalculated = false;
	UpdateRadius();
}

const VEC3 * CurveStripeEntity::GetStripPoints(uint *OutPointCount, bool *OutClosed)
{
	if (!m_PointsCalculated)
	{
		UpdatePoints();
		m_PointsCalculated = true;
	}

	*OutPointCount = m_Points.size();
	*OutClosed = false;
	return &m_Points[0];
}

void CurveStripeEntity::UpdateRadius()
{
	float NewRadius;

	if (m_ControlPoints.empty())
		NewRadius = 0.0f;
	else
	{
		float lsq = LengthSq(m_ControlPoints[0]);
		for (uint i = 1; i < m_ControlPoints.size(); i++)
			lsq = std::max(lsq, LengthSq(m_ControlPoints[i]));
		NewRadius = sqrtf(lsq) + GetHalfWidth();
	}

	// Zmieñ tylko jeœli trzeba zwiêkszyæ albo jeœli trzeba zmniejszyæ co najmniej dwa razy.
	if (GetRadius() < NewRadius || GetRadius() > NewRadius * 2.0f)
		SetRadius(NewRadius);
}

void CurveStripeEntity::UpdatePoints()
{
	switch (m_CurveType)
	{
	case CURVE_TYPE_QUADRATIC:
		{
			m_Points.clear();
			m_Points.reserve(m_ControlPoints.size() / 2 * m_PointsPerSegment);
			float t, ti;
			float Step = 1.0f / (float)m_PointsPerSegment;
			VEC3 p;
			uint i, ic;
			for (uint pi = 2; pi < m_ControlPoints.size(); pi += 2)
			{
				t  = 0.0f;
				ti = 1.0f;
				ic = m_PointsPerSegment + (pi == m_ControlPoints.size()-1 ? 1 : 0);
				for (i = 0; i < ic; i++)
				{
					p =
						m_ControlPoints[pi-2] * ti*ti +
						m_ControlPoints[pi-1] * 2.0f*t*ti +
						m_ControlPoints[pi  ] * t*t;

					m_Points.push_back(p);

					t += Step;
					ti -= Step;
				}
			}
		}
		break;

	case CURVE_TYPE_BEZIER:
		{
			m_Points.clear();
			m_Points.reserve(m_ControlPoints.size() / 3 * m_PointsPerSegment);
			float t, ti, tq, tiq;
			float Step = 1.0f / (float)m_PointsPerSegment;
			VEC3 p;
			uint i, ic;
			for (uint pi = 3; pi < m_ControlPoints.size(); pi += 3)
			{
				t  = 0.0f;
				ti = 1.0f;
				ic = m_PointsPerSegment + (pi == m_ControlPoints.size()-1 ? 1 : 0);
				for (i = 0; i < ic; i++)
				{
					tq = t * t;
					tiq = ti * ti;
					p =
						m_ControlPoints[pi-3] * tiq * ti +
						m_ControlPoints[pi-2] * 3.0f * t * tiq +
						m_ControlPoints[pi-1] * 3.0f * tq * ti +
						m_ControlPoints[pi  ] * tq * t;

					m_Points.push_back(p);

					t += Step;
					ti -= Step;
				}
			}
		}
		break;

	case CURVE_TYPE_BSPLINE:
		{
			m_Points.clear();
			m_Points.reserve(m_ControlPoints.size() * m_PointsPerSegment);
			float t, ti, tq, tc;
			float Step = 1.0f / (float)m_PointsPerSegment;
			VEC3 p;
			uint i, ic;
			for (uint pi = 3; pi < m_ControlPoints.size(); pi++)
			{
				t = 0.0f;
				ti = 1.0f;
				ic = m_PointsPerSegment + (pi == m_ControlPoints.size()-1 ? 1 : 0);
				for (i = 0; i < ic; i++)
				{
					tq = t * t;
					tc = tq * t;
					p = (
						m_ControlPoints[pi-3] * (ti * ti * ti) +
						m_ControlPoints[pi-2] * (3.0f * tc - 6.0f * tq + 4.0f) +
						m_ControlPoints[pi-1] * (-3.0f * tc + 3.0f * tq + 3.0f * t + 1.0f) +
						m_ControlPoints[pi  ] * tc
					) * 0.16666666666666666666666666666667f; // Inaczej /6

					m_Points.push_back(p);

					t += Step;
					ti -= Step;
				}
			}
		}
		break;

	default:
		assert(0 && "Invalid CurveStripeEntity::m_CurveType.");
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ParticleEntity

enum PARTICLE_SHADER_PARAM
{
	PSP_WORLD_VIEW_PROJ,
	PSP_RIGHT_DIR,
	PSP_UP_DIR,
	PSP_TEXTURE,
	PSP_DATA,
};

struct PARTICLE_VERTEX
{
	// Pozycja pocz¹tkowa
	VEC3 StartPos;
	// Zakodowane dane na temat poszczególnego wierzcho³ka danej cz¹steczki
	// VertexFactors.r = K¹t, mapowaæ 0..1 na 1/4*PI .. 7/4*PI
	// VertexFactors.g = Tex.x (0 lub 1)
	// VertexFactors.b = Tex.y (0 lub 1)
	COLOR VertexFactors;
	// ParticleFactors.x = czynnik liniowy 0..1 zale¿ny od numeru cz¹steczki
	// ParticleFactors.y = czynnik losowy 0..1
	VEC2 ParticleFactors;

	static const uint FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);
};

void PARTICLE_DEF::Zero()
{
	CircleDegree = 0;
	PosA2_C = PosA2_P = PosA2_R = VEC3::ZERO;
	PosA1_C = PosA1_P = PosA1_R = VEC3::ZERO;
	PosA0_C = PosA0_P = PosA0_R = VEC3::ZERO;
	PosB2_C = PosB2_P = PosB2_R = VEC3::ZERO;
	PosB1_C = PosB1_P = PosB1_R = VEC3::ZERO;
	PosB0_C = PosB0_P = PosB0_R = VEC3::ZERO;
	ColorA2_C = ColorA2_P = ColorA2_R = VEC4::ZERO;
	ColorA1_C = ColorA1_P = ColorA1_R = VEC4::ZERO;
	ColorA0_C = ColorA0_P = ColorA0_R = VEC4::ZERO;
	OrientationA1_C = OrientationA1_P = OrientationA1_R = 0.0f;
	OrientationA0_C = OrientationA0_P = OrientationA0_R = 0.0f;
	SizeA1_C = SizeA1_P = SizeA1_R = 0.0f;
	SizeA0_C = SizeA0_P = SizeA0_R = 0.0f;
	TimePeriod_C = TimePeriod_P = TimePeriod_R = 0.0f;
	TimePhase_C = TimePhase_P = TimePhase_R = 0.0f;
};

ParticleEntity::ParticleEntity(Scene *OwnerScene, const string &TextureName, BaseMaterial::BLEND_MODE BlendMode, const PARTICLE_DEF &Def, uint MaxParticleCount, float BoundingSphereRadius) :
	CustomEntity(OwnerScene),
	m_TextureName(TextureName),
	m_BlendMode(BlendMode),
	m_Def(Def),
	m_MaxParticleCount(MaxParticleCount),
	m_UseLOD(true),
	m_Texture(NULL),
	m_Shader(NULL),
	m_CreationTime(frame::Timer2.GetTime())
{
	ERR_TRY;

	SetRadius(BoundingSphereRadius);

	InitShaderData();

	m_VB.reset(new res::D3dVertexBuffer(
		string(),
		string(),
		MaxParticleCount * 4,
		D3DUSAGE_WRITEONLY,
		PARTICLE_VERTEX::FVF,
		D3DPOOL_MANAGED));
	m_IB.reset(new res::D3dIndexBuffer(
		string(),
		string(),
		MaxParticleCount * 6,
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED));

	ERR_CATCH_FUNC;
}

ParticleEntity::~ParticleEntity()
{
}

void ParticleEntity::Draw(const ParamsCamera &Cam)
{
	ERR_TRY;

	uint ParticleCount;
	if (m_UseLOD)
	{
		float LOD_Factor = minmax(0.0f, 1.0f - Distance(Cam.GetEyePos(), GetPos()) / Cam.GetZFar(), 1.0f);
		ParticleCount = (uint)(LOD_Factor * m_MaxParticleCount);
		if (ParticleCount == 0) return;
	}
	else
		ParticleCount = m_MaxParticleCount;

	// Zapewnij teksturê i shader
	if (!EnsureTexture()) return;
	if (!EnsureShader()) return;

	// Zapewnij i wype³nij bufory
	m_VB->Load();
	m_IB->Load();
	if (!m_VB->IsFilled())
		FillVB();
	if (!m_IB->IsFilled())
		FillIB();

	frame::RestoreDefaultState();

	frame::Dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	switch (m_BlendMode)
	{
	case WireFrameMaterial::BLEND_LERP:
		frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		break;
	case WireFrameMaterial::BLEND_ADD:
		frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		break;
	case WireFrameMaterial::BLEND_SUB:
		frame::Dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
		frame::Dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		frame::Dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		break;
	default:
		assert(0);
	}

	frame::Dev->SetStreamSource(0, m_VB->GetVB(), 0, sizeof(PARTICLE_VERTEX));
	frame::Dev->SetIndices(m_IB->GetIB());
	frame::Dev->SetFVF(PARTICLE_VERTEX::FVF);

	uint VertexCount = ParticleCount * 4;
	uint PrimitiveCount = ParticleCount * 2;

	const MATRIX &WorldInv = GetInvWorldMatrix();
	VEC3 RightDir, UpDir;
	TransformNormal(&RightDir, Cam.GetRightDir(), WorldInv);
	TransformNormal(&UpDir, Cam.GetRealUpDir(), WorldInv);

	m_ShaderData[24].w = frame::Timer2.GetTime() - m_CreationTime;

	m_Shader->Get()->SetTexture(m_Shader->GetParam(PSP_TEXTURE), m_Texture->GetBaseTexture());
	m_Shader->Get()->SetMatrix(m_Shader->GetParam(PSP_WORLD_VIEW_PROJ), &math_cast<D3DXMATRIX>(GetWorldMatrix()*Cam.GetMatrices().GetViewProj()));
	m_Shader->Get()->SetVector(m_Shader->GetParam(PSP_RIGHT_DIR), &D3DXVECTOR4(RightDir.x, RightDir.y, RightDir.z, 0.0f));
	m_Shader->Get()->SetVector(m_Shader->GetParam(PSP_UP_DIR), &D3DXVECTOR4(UpDir.x, UpDir.y, UpDir.z, 0.0f));
	m_Shader->Get()->SetVectorArray(m_Shader->GetParam(PSP_DATA), (const D3DXVECTOR4*)m_ShaderData, 25);

	m_Shader->Begin(false);
	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, VertexCount, 0, PrimitiveCount) );
	frame::RegisterDrawCall(PrimitiveCount);
	m_Shader->End();
	
	ERR_CATCH_FUNC;
}

bool ParticleEntity::EnsureTexture()
{
	if (m_Texture == NULL)
	{
		if (m_TextureName.empty())
			return false;
		m_Texture = res::g_Manager->MustGetResourceEx<res::D3dTexture>(m_TextureName);
	}

	if (m_Texture != NULL)
	{
		m_Texture->Load();
		return true;
	}
	else return false;
}

bool ParticleEntity::EnsureShader()
{
	if (m_Shader == NULL)
		m_Shader = res::g_Manager->MustGetResourceEx<res::D3dEffect>("ParticleShader");

	if (m_Shader != NULL)
	{
		m_Shader->Load();
		return true;
	}
	else return false;
}

void ParticleEntity::FillVB()
{
	ERR_TRY;

	// Randomizacja indeksu
	std::vector<uint> IndexRandomizer;
	IndexRandomizer.resize(m_MaxParticleCount);
	for (uint i = 0; i < m_MaxParticleCount; i++)
		IndexRandomizer[i] = i;
	for (uint i = 0; i < m_MaxParticleCount; i++)
		std::swap(IndexRandomizer[i], IndexRandomizer[g_Rand.RandUint(m_MaxParticleCount)]);

	{
		res::D3dVertexBuffer::Locker vb_lock(m_VB.get(), true, 0);
		PARTICLE_VERTEX *v = (PARTICLE_VERTEX*)vb_lock.GetData();

		for (uint i = 0; i < m_MaxParticleCount; i++)
		{
			float f = (float)IndexRandomizer[i] / (float)(m_MaxParticleCount-1);

			// Pierwszy
			GenParticleStartPos(&v->StartPos, f);
			v->ParticleFactors.x = f;
			v->ParticleFactors.y = g_Rand.RandFloat();
			v->VertexFactors.R = 0;
			v->VertexFactors.G = 255;
			v->VertexFactors.B = 255;
			v++;

			// Trzy kolejne
			*v = v[-1];
			v->VertexFactors.R = 85;
			v->VertexFactors.G = 255;
			v->VertexFactors.B = 0;
			v++;

			*v = v[-1];
			v->VertexFactors.R = 171;
			v->VertexFactors.G = 0;
			v->VertexFactors.B = 0;
			v++;

			*v = v[-1];
			v->VertexFactors.R = 255;
			v->VertexFactors.G = 0;
			v->VertexFactors.B = 255;
			v++;
		}
	}

	m_VB->SetFilled();

	ERR_CATCH_FUNC;
}

void ParticleEntity::FillIB()
{
	ERR_TRY;

	{
		res::D3dIndexBuffer::Locker ib_lock(m_IB.get(), true, 0);
		uint2 *Indices = (uint2*)ib_lock.GetData();

		uint ii = 0;
		for (uint pi = 0; pi < m_MaxParticleCount; pi++)
		{
			*Indices = ii;     Indices++;
			*Indices = ii + 1; Indices++;
			*Indices = ii + 2; Indices++;
			*Indices = ii;     Indices++;
			*Indices = ii + 2; Indices++;
			*Indices = ii + 3; Indices++;

			ii += 4;
		}
	}

	m_IB->SetFilled();

	ERR_CATCH_FUNC;
}

void ParticleEntity::GenParticleStartPos(VEC3 *Out, float f)
{
	VEC3 PosA0 = m_Def.PosA0_C + m_Def.PosA0_P * f;

	if (m_Def.CircleDegree == 0)
	{
		PosA0.x += m_Def.PosA0_R.x * g_Rand.RandFloat();
		PosA0.y += m_Def.PosA0_R.y * g_Rand.RandFloat();
		PosA0.z += m_Def.PosA0_R.z * g_Rand.RandFloat();

		*Out = PosA0;
	}
	else if (m_Def.CircleDegree == 1)
	{
		PosA0.x += m_Def.PosA0_R.x * sqrtf(g_Rand.RandFloat());
		PosA0.y += m_Def.PosA0_R.y * g_Rand.RandFloat();
		PosA0.z += m_Def.PosA0_R.z * g_Rand.RandFloat();

		Out->y = PosA0.y;
		Out->x = cosf(PosA0.z) * PosA0.x;
		Out->z = sinf(PosA0.z) * PosA0.x;
	}
	else // (m_Def.CircleDegree == 2)
	{
		PosA0.x += m_Def.PosA0_R.x * powf(g_Rand.RandFloat(), 1.0f/3.0f);
		PosA0.y += m_Def.PosA0_R.y * g_Rand.RandFloat();
		PosA0.z += m_Def.PosA0_R.z * g_Rand.RandFloat();

		SphericalToCartesian(Out, PosA0.y, PosA0.z, PosA0.x);
	}
}

void ParticleEntity::InitShaderData()
{
	m_ShaderData[ 0] = m_Def.ColorA2_C;
	m_ShaderData[ 1] = m_Def.ColorA2_P;
	m_ShaderData[ 2] = m_Def.ColorA2_R;
	m_ShaderData[ 3] = m_Def.ColorA1_C;
	m_ShaderData[ 4] = m_Def.ColorA1_P;
	m_ShaderData[ 5] = m_Def.ColorA1_R;
	m_ShaderData[ 6] = m_Def.ColorA0_C;
	m_ShaderData[ 7] = m_Def.ColorA0_P;
	m_ShaderData[ 8] = m_Def.ColorA0_R;

	m_ShaderData[ 9].x = m_Def.PosA2_C.x;
	m_ShaderData[ 9].y = m_Def.PosA2_C.y;
	m_ShaderData[ 9].z = m_Def.PosA2_C.z;
	m_ShaderData[ 9].w = m_Def.OrientationA1_C;

	m_ShaderData[10].x = m_Def.PosA2_P.x;
	m_ShaderData[10].y = m_Def.PosA2_P.y;
	m_ShaderData[10].z = m_Def.PosA2_P.z;
	m_ShaderData[10].w = m_Def.OrientationA1_P;

	m_ShaderData[11].x = m_Def.PosA2_R.x;
	m_ShaderData[11].y = m_Def.PosA2_R.y;
	m_ShaderData[11].z = m_Def.PosA2_R.z;
	m_ShaderData[11].w = m_Def.OrientationA1_R;

	m_ShaderData[12].x = m_Def.PosA1_C.x;
	m_ShaderData[12].y = m_Def.PosA1_C.y;
	m_ShaderData[12].z = m_Def.PosA1_C.z;
	m_ShaderData[12].w = m_Def.OrientationA0_C;

	m_ShaderData[13].x = m_Def.PosA1_P.x;
	m_ShaderData[13].y = m_Def.PosA1_P.y;
	m_ShaderData[13].z = m_Def.PosA1_P.z;
	m_ShaderData[13].w = m_Def.OrientationA0_P;

	m_ShaderData[14].x = m_Def.PosA1_R.x;
	m_ShaderData[14].y = m_Def.PosA1_R.y;
	m_ShaderData[14].z = m_Def.PosA1_R.z;
	m_ShaderData[14].w = m_Def.OrientationA0_R;

	m_ShaderData[15].x = m_Def.PosB2_C.x;
	m_ShaderData[15].y = m_Def.PosB2_C.y;
	m_ShaderData[15].z = m_Def.PosB2_C.z;
	m_ShaderData[15].w = m_Def.SizeA1_C;

	m_ShaderData[16].x = m_Def.PosB2_P.x;
	m_ShaderData[16].y = m_Def.PosB2_P.y;
	m_ShaderData[16].z = m_Def.PosB2_P.z;
	m_ShaderData[16].w = m_Def.SizeA1_P;

	m_ShaderData[17].x = m_Def.PosB2_R.x;
	m_ShaderData[17].y = m_Def.PosB2_R.y;
	m_ShaderData[17].z = m_Def.PosB2_R.z;
	m_ShaderData[17].w = m_Def.SizeA1_R;

	m_ShaderData[18].x = m_Def.PosB1_C.x;
	m_ShaderData[18].y = m_Def.PosB1_C.y;
	m_ShaderData[18].z = m_Def.PosB1_C.z;
	m_ShaderData[18].w = m_Def.SizeA0_C;

	m_ShaderData[19].x = m_Def.PosB1_P.x;
	m_ShaderData[19].y = m_Def.PosB1_P.y;
	m_ShaderData[19].z = m_Def.PosB1_P.z;
	m_ShaderData[19].w = m_Def.SizeA0_P;

	m_ShaderData[20].x = m_Def.PosB1_R.x;
	m_ShaderData[20].y = m_Def.PosB1_R.y;
	m_ShaderData[20].z = m_Def.PosB1_R.z;
	m_ShaderData[20].w = m_Def.SizeA0_R;

	m_ShaderData[21].x = m_Def.PosB0_C.x;
	m_ShaderData[21].y = m_Def.PosB0_C.y;
	m_ShaderData[21].z = m_Def.PosB0_C.z;
	m_ShaderData[21].w = m_Def.TimePeriod_C;

	m_ShaderData[22].x = m_Def.PosB0_P.x;
	m_ShaderData[22].y = m_Def.PosB0_P.y;
	m_ShaderData[22].z = m_Def.PosB0_P.z;
	m_ShaderData[22].w = m_Def.TimePeriod_P;

	m_ShaderData[23].x = m_Def.PosB0_R.x;
	m_ShaderData[23].y = m_Def.PosB0_R.y;
	m_ShaderData[23].z = m_Def.PosB0_R.z;
	m_ShaderData[23].w = m_Def.TimePeriod_R;

	m_ShaderData[24].x = m_Def.TimePhase_C;
	m_ShaderData[24].y = m_Def.TimePhase_P;
	m_ShaderData[24].z = m_Def.TimePhase_R;
	//m_ShaderData[24].w =
}


} // namespace engine
