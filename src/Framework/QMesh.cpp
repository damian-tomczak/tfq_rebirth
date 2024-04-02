/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "Framework.hpp"
#include "D3dUtils.hpp"
#include "QMesh.hpp"

namespace res
{


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa QMesh

struct QMSH_HEADER
{
	char Format[6];
	char Version[2];
	uint1 Flags;
	uint2 NumVertices;
	uint2 NumTriangles;
	uint2 NumSubmeshes;
	uint2 NumBones;
	uint2 NumAnimations;
	float BoundingSphereRadius;
	BOX BoundingBox;
	uint4 VerticesOffset;
	uint4 TrianglesOffset;
	uint4 SubmeshesOffset;
	uint4 BonesOffset;
	uint4 AnimationsOffset;
};

struct QMSH_KEYFRAME_BONE
{
	VEC3 Translation;
	QUATERNION Rotation;
	float Scaling;
};

struct QMSH_KEYFRAME
{
	float Time;
	// Dla ka¿dej koœci
	std::vector<QMSH_KEYFRAME_BONE> Bones;
};

class QMesh::Animation_pimpl
{
public:
	string Name;
	float Length;
	std::vector< shared_ptr<QMSH_KEYFRAME> > Keyframes;

	void LoadFromFile(common::Stream &File, uint BoneCount);
};

void QMesh::Animation_pimpl::LoadFromFile(common::Stream &File, uint BoneCount)
{
	File.ReadString1(&Name);
	File.ReadEx(&Length);

	uint2 KeyframeCount;
	File.ReadEx(&KeyframeCount);

	for (uint2 ki = 0; ki < KeyframeCount; ki++)
	{
		shared_ptr<QMSH_KEYFRAME> Keyframe(new QMSH_KEYFRAME);
		Keyframes.push_back(Keyframe);

		File.ReadEx(&Keyframe->Time);
		Keyframe->Bones.resize(BoneCount);
		for (uint bi = 0; bi < BoneCount; bi++)
		{
			File.ReadEx(&Keyframe->Bones[bi].Translation);
			File.ReadEx(&Keyframe->Bones[bi].Rotation);
			File.ReadEx(&Keyframe->Bones[bi].Scaling);
		}
	}
}

QMesh::Animation::Animation() :
	pimpl(new QMesh::Animation_pimpl)
{
}

QMesh::Animation::~Animation()
{
	pimpl.reset();
}

const string & QMesh::Animation::GetName()
{
	return pimpl->Name;
}

float QMesh::Animation::GetLength()
{
	return pimpl->Length;
}

uint QMesh::Animation::GetKeyframeCount()
{
	return pimpl->Keyframes.size();
}

float QMesh::Animation::GetKeyframeTime(uint KeyframeIndex)
{
	return pimpl->Keyframes[KeyframeIndex]->Time;
}

const common::VEC3 & QMesh::Animation::GetKeyframeTranslation(uint BoneIndex, uint KeyframeIndex)
{
	assert(KeyframeIndex < pimpl->Keyframes.size());

	if (BoneIndex == 0) return VEC3::ZERO;

	return pimpl->Keyframes[KeyframeIndex]->Bones[BoneIndex - 1].Translation;
}

const common::QUATERNION & QMesh::Animation::GetKeyframeRotation(uint BoneIndex, uint KeyframeIndex)
{
	assert(KeyframeIndex < pimpl->Keyframes.size());

	if (BoneIndex == 0) return QUATERNION::IDENTITY;

	return pimpl->Keyframes[KeyframeIndex]->Bones[BoneIndex - 1].Rotation;
}

float QMesh::Animation::GetKeyframeScaling(uint BoneIndex, uint KeyframeIndex)
{
	assert(KeyframeIndex < pimpl->Keyframes.size());

	if (BoneIndex == 0) return 1.f;

	return pimpl->Keyframes[KeyframeIndex]->Bones[BoneIndex - 1].Scaling;
}

void QMesh::Animation::GetTimeTranslation(common::VEC3 *Out, uint BoneIndex, float Time)
{
	if (BoneIndex == 0)
		*Out = VEC3::ZERO;
	else if (pimpl->Keyframes.empty())
		*Out = VEC3::ZERO;
	else if (pimpl->Keyframes.size() == 1)
		*Out = pimpl->Keyframes[0]->Bones[BoneIndex - 1].Translation;
	// Czas jeszcze przed pierwsz¹ klatk¹
	else if (Time < pimpl->Keyframes[0]->Time)
		*Out = pimpl->Keyframes[0]->Bones[BoneIndex - 1].Translation;
	else
	{
		for (uint ki = 0; ki < pimpl->Keyframes.size(); ki++)
		{
			const QMSH_KEYFRAME & Keyframe2 = *pimpl->Keyframes[ki].get();

			if (Keyframe2.Time > Time)
			{
				// To jest dok³adnie ta klatka lub ta jest pierwsza - pobierz z tej
				if (float_equal(Keyframe2.Time, Time) || ki == 0)
					*Out = Keyframe2.Bones[BoneIndex - 1].Translation;
				// Zinterpoluj miêdzy t¹ a poprzedni¹
				else
				{
					const QMSH_KEYFRAME & Keyframe1 = *pimpl->Keyframes[ki-1].get();
					float t = (Time - Keyframe1.Time) / (Keyframe2.Time - Keyframe1.Time);
					Lerp(Out, Keyframe1.Bones[BoneIndex - 1].Translation, Keyframe2.Bones[BoneIndex - 1].Translation, t);
				}
				return;
			}
		}
		// Jesteœmy za ostatni¹ klatk¹
		*Out = pimpl->Keyframes[pimpl->Keyframes.size()-1]->Bones[BoneIndex - 1].Translation;
	}
}

void QMesh::Animation::GetTimeRotation(common::QUATERNION *Out, uint BoneIndex, float Time)
{
	if (BoneIndex == 0)
		*Out = QUATERNION::IDENTITY;
	else if (pimpl->Keyframes.empty())
		*Out = common::QUATERNION::IDENTITY;
	else if (pimpl->Keyframes.size() == 1)
		*Out = pimpl->Keyframes[0]->Bones[BoneIndex - 1].Rotation;
	// Czas jeszcze przed pierwsz¹ klatk¹
	else if (Time < pimpl->Keyframes[0]->Time)
		*Out = pimpl->Keyframes[0]->Bones[BoneIndex - 1].Rotation;
	else
	{
		for (uint ki = 0; ki < pimpl->Keyframes.size(); ki++)
		{
			const QMSH_KEYFRAME & Keyframe2 = *pimpl->Keyframes[ki].get();

			if (Keyframe2.Time > Time)
			{
				// To jest dok³adnie ta klatka lub ta jest pierwsza - pobierz z tej
				if (float_equal(Keyframe2.Time, Time) || ki == 0)
					*Out = Keyframe2.Bones[BoneIndex - 1].Rotation;
				// Zinterpoluj miêdzy t¹ a poprzedni¹
				else
				{
					const QMSH_KEYFRAME & Keyframe1 = *pimpl->Keyframes[ki-1].get();
					float t = (Time - Keyframe1.Time) / (Keyframe2.Time - Keyframe1.Time);
					Slerp(Out, Keyframe1.Bones[BoneIndex - 1].Rotation, Keyframe2.Bones[BoneIndex - 1].Rotation, t);
				}
				return;
			}
		}
		// Jesteœmy za ostatni¹ klatk¹
		*Out = pimpl->Keyframes[pimpl->Keyframes.size()-1]->Bones[BoneIndex - 1].Rotation;
	}
}

void QMesh::Animation::GetTimeScaling(float *Out, uint BoneIndex, float Time)
{
	if (BoneIndex == 0)
		*Out = 1.f;
	if (pimpl->Keyframes.empty())
		*Out = 1.f;
	else if (pimpl->Keyframes.size() == 1)
		*Out = pimpl->Keyframes[0]->Bones[BoneIndex - 1].Scaling;
	// Czas jeszcze przed pierwsz¹ klatk¹
	else if (Time < pimpl->Keyframes[0]->Time)
		*Out = pimpl->Keyframes[0]->Bones[BoneIndex - 1].Scaling;
	else
	{
		for (uint ki = 0; ki < pimpl->Keyframes.size(); ki++)
		{
			const QMSH_KEYFRAME & Keyframe2 = *pimpl->Keyframes[ki].get();

			if (Keyframe2.Time > Time)
			{
				// To jest dok³adnie ta klatka lub ta jest pierwsza - pobierz z tej
				if (float_equal(Keyframe2.Time, Time) || ki == 0)
					*Out = Keyframe2.Bones[BoneIndex - 1].Scaling;
				// Zinterpoluj miêdzy t¹ a poprzedni¹
				else
				{
					const QMSH_KEYFRAME & Keyframe1 = *pimpl->Keyframes[ki-1].get();
					float t = (Time - Keyframe1.Time) / (Keyframe2.Time - Keyframe1.Time);
					*Out = Lerp(Keyframe1.Bones[BoneIndex - 1].Scaling, Keyframe2.Bones[BoneIndex - 1].Scaling, t);
				}
				return;
			}
		}
		// Jesteœmy za ostatni¹ klatk¹
		*Out = pimpl->Keyframes[pimpl->Keyframes.size()-1]->Bones[BoneIndex - 1].Scaling;
	}
}

typedef std::vector<MATRIX> MATRIX_VECTOR;

struct BoneMatrixCacheEntry
{
	uint LastUseFrameNumber;
	// Numer animacji.
	uint Animation1;
	// Czas w tej animacji.
	float Time1;
	// Numer drugiej animacji.
	// MAXUINT2 jeœli jest tylko jedna animacja, nie interpolacja miêdzy dwiema
	uint Animation2;
	// Czas w drugiej animacji.
	// Wa¿ne tylko jeœli Animation2 != MAXUINT2
	float Time2;
	// Interpolacja miêdzy Anim1 i Anim2.
	// Wa¿ne tylko jeœli Animation2 != MAXUINT2
	float LerpT;
	// Macierze w³aœciwe - po jednej dla ka¿dej koœci.
	// £¹cznie z macierz¹ identycznoœciow¹ dla koœci 0.
	MATRIX_VECTOR BoneMatrices;

	bool ParamsCompilant(float Accuracy, uint Animation1, float Time1, uint Animation2, float Time2, float LerpT);
};

bool BoneMatrixCacheEntry::ParamsCompilant(float Accuracy, uint Animation1, float Time1, uint Animation2, float Time2, float LerpT)
{
	if (Animation1 != this->Animation1)
		return false;
	if (Animation2 != this->Animation2)
		return false;
	if (fabsf(Time1 - this->Time1) > Accuracy)
		return false;
	if (Animation2 != MAXUINT4)
	{
		if (fabsf(Time2 - this->Time2) > Accuracy)
			return false;
		if (fabsf(LerpT - this->LerpT) > 0.001f) // No bo co mia³em tu podaæ? Przecie¿ nie Accuracy, bo Accuracy jest w sekundach a LerpT w niewiadomo czym.
			return false;
	}
	return true;
}

typedef std::list< shared_ptr< BoneMatrixCacheEntry > > BONE_MATRIX_CACHE_ENTRY_LIST;

// Maksymalna liczba zestawów macierzy w cache.
// Zastrze¿eniem ¿e nie usuwa siê macierzy z tej ani z poprzedniej klatki.
const uint MAX_BONE_MATRIX_CACHE_ENTRIES = 8;

struct SOFTWARE_SKINNING_VB
{
	bool Filled;
	uint Animation1;
	uint Animation2;
	float Time1;
	float Time2;
	float LerpT;
	std::vector<VEC3> Vertices;

	bool CheckCache(float Accuracy, uint Animation1, float Time1, uint Animation2, float Time2, float LerpT);
	void Clear();
};

bool SOFTWARE_SKINNING_VB::CheckCache(float Accuracy, uint Animation1, float Time1, uint Animation2, float Time2, float LerpT)
{
	if (!Filled)
		return false;
	if (Animation1 != this->Animation1)
		return false;
	if (Animation2 != this->Animation2)
		return false;
	if (fabsf(Time1 - this->Time1) > Accuracy)
		return false;
	if (Animation2 != MAXUINT4)
	{
		if (fabsf(Time2 - this->Time2) > Accuracy)
			return false;
		if (fabsf(LerpT - this->LerpT) > 0.001f) // No bo co mia³em tu podaæ? Przecie¿ nie Accuracy, bo Accuracy jest w sekundach a LerpT w niewiadomo czym.
			return false;
	}
	return true;
}

void SOFTWARE_SKINNING_VB::Clear()
{
	Filled = false;
	Vertices.clear();
}


struct QMesh_pimpl
{
	string FileName;

	// Ustawione miêdzy Load i Unload
	uint FVF;
	uint VertexSize;
	scoped_ptr<QMSH_HEADER> Header;
	std::vector< shared_ptr<QMesh::SUBMESH> > Submeshes;
	std::vector< shared_ptr<QMesh::BONE> > Bones;
	std::vector< shared_ptr<QMesh::Animation> > Animations;
	std::vector<char> VB_Data;
	std::vector<uint2> IB_Data;

	// Ustawione miêdzy OnDeviceCreate a OnDeviceDestroy
	scoped_ptr<IDirect3DVertexBuffer9, ReleasePolicy> VB;
	scoped_ptr<IDirect3DIndexBuffer9, ReleasePolicy> IB;

	// Wyliczane przy pierwszym u¿yciu
	std::vector<MATRIX> ModelToBoneMatrices;
	// Cache macierzy koœci!
	// Zawsze posortowany rosn¹co wg numeru klatki
	BONE_MATRIX_CACHE_ENTRY_LIST BoneMatrixCache;
	SOFTWARE_SKINNING_VB SoftwareSkinningVb;

	// Jeœli istnieje w cache tablica macierzy dla podanych parametrów z podan¹ dok³adnoœci¹ w sekundach,
	// zwraca wskaŸnik do niej i aktualizuje jej znacznik czasu.
	// Jeœli nie istnieje, zwraca NULL.
	MATRIX * TryGetBoneMatricesFromCache(float Accuracy, uint Animation, float Time);
	MATRIX * TryGetBoneMatricesFromCache(float Accuracy, uint Animation1, float Time1, uint Animation2, float Time2, float LerpT);
	// Dodaje utworzony wczeœniej wpis do cache.
	// Jeœli trzeba, kasuje stare wpisy.
	void AddBoneMatrixCacheEntry(shared_ptr<BoneMatrixCacheEntry> Entry);
};

MATRIX * QMesh_pimpl::TryGetBoneMatricesFromCache(float Accuracy, uint Animation, float Time)
{
	return TryGetBoneMatricesFromCache(Accuracy, Animation, Time, MAXUINT4, 0.f, 0.f);
}

MATRIX * QMesh_pimpl::TryGetBoneMatricesFromCache(float Accuracy, uint Animation1, float Time1, uint Animation2, float Time2, float LerpT)
{
	for (BONE_MATRIX_CACHE_ENTRY_LIST::reverse_iterator eit = BoneMatrixCache.rbegin(); eit != BoneMatrixCache.rend(); ++eit)
	{
		if ((*eit)->ParamsCompilant(Accuracy, Animation1, Time1, Animation2, Time2, LerpT))
		{
			shared_ptr<BoneMatrixCacheEntry> Entry = *eit;
			BoneMatrixCache.erase(--eit.base());
			Entry->LastUseFrameNumber = frame::GetFrameNumber();
			BoneMatrixCache.push_back(Entry);
			return &Entry->BoneMatrices[0];
		}
	}
	return NULL;
}

void QMesh_pimpl::AddBoneMatrixCacheEntry(shared_ptr<BoneMatrixCacheEntry> Entry)
{
	BoneMatrixCache.push_back(Entry);
	while (BoneMatrixCache.size() > MAX_BONE_MATRIX_CACHE_ENTRIES)
	{
		if (BoneMatrixCache.front()->LastUseFrameNumber > frame::GetFrameNumber()-1)
			break;
		BoneMatrixCache.erase(BoneMatrixCache.begin());
	}
}

void QMesh::RegisterResourceType()
{
	g_Manager->RegisterResourceType("QMesh", &QMesh::Create);
}

IResource * QMesh::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	string FileName = Params.GetString();
	Params.Next();

	Params.AssertSymbol(';');
	Params.Next();

	return new QMesh(Name, Group, FileName);
}

QMesh::QMesh(const string &Name, const string &Group, const string &FileName) :
	D3dResource(Name, Group),
	pimpl(new QMesh_pimpl)
{
	pimpl->FileName = FileName;
	pimpl->FVF = 0;
	pimpl->VertexSize = 0;
}

QMesh::~QMesh()
{
	pimpl.reset();
}

void QMesh::OnLoad()
{
	ERR_TRY;
	{
		common::FileStream File(pimpl->FileName, common::FM_READ);

		LOG(LOG_RESMNGR, Format("QMesh: \"#\" Loading header from \"#\"") % GetName() % GetFileName());

		// Wczytaj nag³ówek

		if (pimpl->Header == NULL)
		{
			pimpl->Header.reset(new QMSH_HEADER);

			QMSH_HEADER & h = *pimpl->Header.get();

			File.MustRead(h.Format, 6);
			File.MustRead(h.Version, 2);
			File.ReadEx(&h.Flags);
			File.ReadEx(&h.NumVertices);
			File.ReadEx(&h.NumTriangles);
			File.ReadEx(&h.NumSubmeshes);
			File.ReadEx(&h.NumBones);
			File.ReadEx(&h.NumAnimations);
			File.ReadEx(&h.BoundingSphereRadius);
			File.ReadEx(&h.BoundingBox);
			File.ReadEx(&h.VerticesOffset);
			File.ReadEx(&h.TrianglesOffset);
			File.ReadEx(&h.SubmeshesOffset);
			File.ReadEx(&h.BonesOffset);
			File.ReadEx(&h.AnimationsOffset);

			if (strncmp(pimpl->Header->Format, "TFQMSH", 6) != 0)
				throw Error("B³êdny nag³ówek.");
			if (strncmp(pimpl->Header->Version, "10", 2) != 0)
				throw Error("B³êdna wersja.");
		}

		// Wylicz format i rozmiar wierzcho³ka

		pimpl->FVF = D3DFVF_NORMAL | D3DFVF_TEXCOORDSIZE2(0);

		if ((pimpl->Header->Flags & 0x01) != 0)
			pimpl->FVF |= D3DFVF_TEXCOORDSIZE3(1) | D3DFVF_TEXCOORDSIZE3(2) | D3DFVF_TEX3;
		else
			pimpl->FVF |= D3DFVF_TEX1;

		if ((pimpl->Header->Flags & 0x02) != 0)
			pimpl->FVF |= D3DFVF_XYZB2 | D3DFVF_LASTBETA_UBYTE4;
		else
			pimpl->FVF |= D3DFVF_XYZ;

		pimpl->VertexSize = FVF::CalcVertexSize(pimpl->FVF);

		// Wczytaj dane VB
		{
			File.SetPos((int)pimpl->Header->VerticesOffset);
			uint1 ZeroByte; File.ReadEx(&ZeroByte); if (ZeroByte != 0) throw Error("B³¹d w pliku.");

			uint vb_size = pimpl->Header->NumVertices * pimpl->VertexSize;
			pimpl->VB_Data.resize(vb_size);
			File.MustRead(&pimpl->VB_Data[0], vb_size);
		}

		// Wczytaj dane IB
		{
			File.SetPos((int)pimpl->Header->TrianglesOffset);
			uint1 ZeroByte; File.ReadEx(&ZeroByte); if (ZeroByte != 0) throw Error("B³¹d w pliku.");

			uint index_count = pimpl->Header->NumTriangles * 3;
			pimpl->IB_Data.resize(index_count);
			File.MustRead(&pimpl->IB_Data[0], index_count * sizeof(uint2));
		}

		// Wczytaj info o podsiatkach

		File.SetPos((int)pimpl->Header->SubmeshesOffset);
		uint1 ZeroByte; File.ReadEx(&ZeroByte); if (ZeroByte != 0) throw Error("B³¹d w pliku.");

		for (uint si = 0; si < pimpl->Header->NumSubmeshes; si++)
		{
			shared_ptr<SUBMESH> s(new SUBMESH);
			File.ReadEx(&s->FirstTriangle);
			File.ReadEx(&s->NumTriangles);
			File.ReadEx(&s->MinVertexIndexUsed);
			File.ReadEx(&s->NumVertexIndicesUsed);
			File.ReadString1(&s->Name);
			File.ReadString1(&s->MaterialName);
			pimpl->Submeshes.push_back(s);
		}

		if ((pimpl->Header->Flags & 0x02) != 0)
		{
			// Wczytaj koœci

			File.SetPos((int)pimpl->Header->BonesOffset);
			uint1 ZeroByte; File.ReadEx(&ZeroByte); if (ZeroByte != 0) throw Error("B³¹d w pliku.");

			// Koœæ zerowa - pusta
			shared_ptr<BONE> b(new BONE); // Children pozostaje puste, Name pozostaje puste
			b->Index = 0;
			b->ParentIndex = 0; // Sama na siebie! Uwaga ¿eby siê nie zapêtliæ :)
			Identity(&b->Matrix);
			pimpl->Bones.push_back(b);

			for (uint bi = 0; bi < pimpl->Header->NumBones; bi++)
			{
				shared_ptr<BONE> b(new BONE);

				b->Index = (bi + 1);
				File.ReadEx(&b->ParentIndex);

				File.ReadEx(&b->Matrix._11);
				File.ReadEx(&b->Matrix._12);
				File.ReadEx(&b->Matrix._13);
				b->Matrix._14 = 0.f;
				File.ReadEx(&b->Matrix._21);
				File.ReadEx(&b->Matrix._22);
				File.ReadEx(&b->Matrix._23);
				b->Matrix._24 = 0.f;
				File.ReadEx(&b->Matrix._31);
				File.ReadEx(&b->Matrix._32);
				File.ReadEx(&b->Matrix._33);
				b->Matrix._34 = 0.f;
				File.ReadEx(&b->Matrix._41);
				File.ReadEx(&b->Matrix._42);
				File.ReadEx(&b->Matrix._43);
				b->Matrix._44 = 1.f;

				File.ReadString1(&b->Name);

				pimpl->Bones.push_back(b);

				// Dodaj do listy dzieci parenta (parent na pewno jest przed t¹ koœci¹, tak jest w specyfikacji formatu)
				pimpl->Bones[b->ParentIndex]->Children.push_back(bi + 1); // inaczej mówi¹c, (pimpl->Bones.size() - 1 + 1)
			}

			// Wczytaj animacje

			File.SetPos((int)pimpl->Header->AnimationsOffset);
			File.ReadEx(&ZeroByte); if (ZeroByte != 0) throw Error("B³¹d w pliku.");

			for (uint ai = 0; ai < pimpl->Header->NumAnimations; ai++)
			{
				shared_ptr<QMesh::Animation> a(new QMesh::Animation);
				pimpl->Animations.push_back(a);
				a->pimpl->LoadFromFile(File, pimpl->Header->NumBones);
			}
		}
	}
	ERR_CATCH("Nie mo¿na wczytaæ siatki z pliku \"" + pimpl->FileName + "\"");

	D3dResource::OnLoad();
}

void QMesh::OnUnload()
{
	D3dResource::OnUnload();

	pimpl->SoftwareSkinningVb.Clear();
	pimpl->BoneMatrixCache.clear();

	pimpl->IB_Data.clear();
	pimpl->VB_Data.clear();
	pimpl->FVF = 0;
	pimpl->VertexSize = 0;
	pimpl->Header.reset();
}

void QMesh::OnDeviceCreate()
{
	ERR_TRY;

	HRESULT hr;
	assert(pimpl->Header != NULL);
	LOG(LOG_RESMNGR, Format("QMesh: \"#\" Loading data from \"#\"") % GetName() % GetFileName());

	// Wierzcho³ki

	uint vb_size = pimpl->Header->NumVertices * pimpl->VertexSize;
	IDirect3DVertexBuffer9 *vb;

	hr = frame::Dev->CreateVertexBuffer(
		vb_size,
		D3DUSAGE_WRITEONLY,
		pimpl->FVF,
		D3DPOOL_MANAGED,
		&vb,
		NULL);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na utworzyæ bufora wierzcho³ków.");
	pimpl->VB.reset(vb);

	{
		VertexBufferLock vb_lock(vb, 0);
		CopyMem(vb_lock.GetData(), &pimpl->VB_Data[0], vb_size);
	}

	// Indeksy

	uint ib_size = pimpl->Header->NumTriangles * 3 * sizeof(uint2);
	IDirect3DIndexBuffer9 *ib;

	hr = frame::Dev->CreateIndexBuffer(
		ib_size,
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED,
		&ib,
		NULL);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na utworzyæ bufora indeksów.");
	pimpl->IB.reset(ib);

	{
		IndexBufferLock ib_lock(ib, 0);
		CopyMem(ib_lock.GetData(), &pimpl->IB_Data[0], ib_size);
	}

	ERR_CATCH("Nie mo¿na wczytaæ siatki z pliku \"" + pimpl->FileName + "\"");
}

void QMesh::OnDeviceDestroy()
{
	pimpl->IB.reset();
	pimpl->VB.reset();
}

void QMesh::OnDeviceRestore()
{
}

void QMesh::OnDeviceInvalidate()
{
}

const QMesh::BONE * QMesh::GetBoneByName(const string &BoneName)
{
	for (uint bi = 0; bi < pimpl->Bones.size(); bi++)
		if (pimpl->Bones[bi]->Name == BoneName)
			return pimpl->Bones[bi].get();
	return NULL;
}

uint QMesh::GetAnimationIndexByName(const string &AnimationName)
{
	for (uint ai = 0; ai < pimpl->Animations.size(); ai++)
		if (pimpl->Animations[ai]->GetName() == AnimationName)
			return ai;
	return MAXUINT4;
}

QMesh::Animation * QMesh::GetAnimationByName(const string &AnimationName)
{
	for (uint ai = 0; ai < pimpl->Animations.size(); ai++)
		if (pimpl->Animations[ai]->GetName() == AnimationName)
			return pimpl->Animations[ai].get();
	return NULL;
}

const MATRIX * QMesh::GetModelToBoneMatrices()
{
	if (GetBoneCount() == 0)
		return NULL;

	if (pimpl->ModelToBoneMatrices.empty())
	{
		pimpl->ModelToBoneMatrices.resize(GetBoneCount());
		Identity(&pimpl->ModelToBoneMatrices[0]);
		for (uint i = 1; i < GetBoneCount(); i++)
		{
			const BONE & Bone = GetBone(i);
			// Macierz z koœci nadrzêdnej do danej koœci
			Inverse(&pimpl->ModelToBoneMatrices[i], Bone.Matrix);
			// Jeœli to koœæ g³ówna, to ju¿ jest jednoczeœnie z modelu do danej koœci.
			// Jeœli to nie koœæ g³ówna, to z modelu do danej koœci = z modelu do koœci nadrzêdnej * z koœci nadrzêdnej do danej
			if (Bone.ParentIndex > 0)
				pimpl->ModelToBoneMatrices[i] = pimpl->ModelToBoneMatrices[Bone.ParentIndex] * pimpl->ModelToBoneMatrices[i];
		}
	}

	return &pimpl->ModelToBoneMatrices[0];
}

const MATRIX * QMesh::GetBoneMatrices(float Accuracy, uint Animation, float Time)
{
	return GetBoneMatrices(Accuracy, Animation, Time, MAXUINT4, 0.f, 0.f);
}

const MATRIX * QMesh::GetBoneMatrices(float Accuracy, uint Animation1, float Time1, uint Animation2, float Time2, float LerpT)
{
	// Nie ma i nie bêdzie.
	if (!HasSkinning() || GetBoneCount() == 0)
		return NULL;

	// Jest w cache - zwróæ od razu
	const MATRIX * CacheMatrices = pimpl->TryGetBoneMatricesFromCache(Accuracy, Animation1, Time1, Animation2, Time2, LerpT);
	if (CacheMatrices != NULL)
		return CacheMatrices;

	// Wygeneruj
	shared_ptr<BoneMatrixCacheEntry> Entry(new BoneMatrixCacheEntry);
	Entry->Animation1 = Animation1;
	Entry->Animation2 = Animation2;
	Entry->Time1 = Time1;
	Entry->Time2 = Time2;
	Entry->LerpT = LerpT;
	Entry->LastUseFrameNumber = frame::GetFrameNumber();
	Entry->BoneMatrices.resize(GetBoneCount());

	// Macierze przekszta³caj¹ce ze wsp. danej koœci do wsp. koœci nadrzêdnej w ustalonej pozycji
	MATRIX BoneToParentPoseMat[32];
	Identity(&BoneToParentPoseMat[0]);

	// Tylko jedna animacja
	if (Animation2 == MAXUINT4 || LerpT < 0.001f || LerpT >= 0.999f)
	{
		uint AnimIndex;
		float AnimTime;
		if (Animation2 == MAXUINT4 || LerpT < 0.001f)
		{
			AnimIndex = Animation1;
			AnimTime = Time1;
		}
		else // (LerpT >= 0.999f)
		{
			AnimIndex = Animation2;
			AnimTime = Time2;
		}

		Animation & Anim = GetAnimation(AnimIndex);

		for (uint i = 1; i < GetBoneCount(); i++)
		{
			float Scal; Anim.GetTimeScaling(&Scal, i, AnimTime);
			MATRIX ScalMat; Scaling(&ScalMat, Scal);

			QUATERNION Rot; Anim.GetTimeRotation(&Rot, i, AnimTime);
			MATRIX RotMat; QuaternionToRotationMatrix(&RotMat, Rot);

			VEC3 Trans; Anim.GetTimeTranslation(&Trans, i, AnimTime);
			MATRIX TransMat; Translation(&TransMat, Trans);

			BoneToParentPoseMat[i] = ScalMat * RotMat * TransMat;

			// Do tego oryginalne przekszta³cenie do nadrzêdnej
			BoneToParentPoseMat[i] *= GetBone(i).Matrix;
		}
	}
	// Dwie animacje
	else
	{
		QMesh::Animation & Anim1 = GetAnimation(Animation1);
		QMesh::Animation & Anim2 = GetAnimation(Animation2);
		float AnimTime1 = Time1; // Alias, tak dla jaj, bo to stary kod
		float AnimTime2 = Time2; // Alias, tak dla jaj, bo to stary kod

		for (uint i = 1; i < GetBoneCount(); i++)
		{
			float Scal1; Anim1.GetTimeScaling(&Scal1, i, AnimTime1);
			float Scal2; Anim2.GetTimeScaling(&Scal2, i, AnimTime2);
			float Scal = Lerp(Scal1, Scal2, LerpT);
			MATRIX ScalMat; Scaling(&ScalMat, Scal);

			QUATERNION Rot1; Anim1.GetTimeRotation(&Rot1, i, AnimTime1);
			QUATERNION Rot2; Anim2.GetTimeRotation(&Rot2, i, AnimTime2);
			QUATERNION Rot; Slerp(&Rot, Rot1, Rot2, LerpT);
			MATRIX RotMat; QuaternionToRotationMatrix(&RotMat, Rot);

			VEC3 Trans1; Anim1.GetTimeTranslation(&Trans1, i, AnimTime1);
			VEC3 Trans2; Anim2.GetTimeTranslation(&Trans2, i, AnimTime2);
			VEC3 Trans; Lerp(&Trans, Trans1, Trans2, LerpT);
			MATRIX TransMat; Translation(&TransMat, Trans);

			BoneToParentPoseMat[i] = ScalMat * RotMat * TransMat;

			// Do tego oryginalne przekszta³cenie do nadrzêdnej
			BoneToParentPoseMat[i] *= GetBone(i).Matrix;
		}
	}

	// Macierze przekszta³caj¹ce ze wsp. danej koœci do wsp. modelu w ustalonej pozycji
	// (To obliczenie nale¿a³oby po³¹czyæ z poprzednim)
	MATRIX BoneToModelPoseMat[32];
	Identity(&BoneToModelPoseMat[0]);
	for (uint i = 1; i < GetBoneCount(); i++)
	{
		const QMesh::BONE & Bone = GetBone(i);
		// Jeœli to koœæ g³ówna, przekszta³cenie z danej koœci do nadrzêdnej = z danej koœci do modelu
		// Jeœli to nie koœæ g³ówna, przekszta³cenie z danej koœci do modelu = z danej koœci do nadrzêdnej * z nadrzêdnej do modelu
		if (Bone.ParentIndex == 0)
			BoneToModelPoseMat[i] = BoneToParentPoseMat[i];
		else
			BoneToModelPoseMat[i] = BoneToParentPoseMat[i] * BoneToModelPoseMat[Bone.ParentIndex];
	}

	// Macierze zebrane koœci - przekszta³caj¹ce z modelu do koœci w pozycji spoczynkowej * z koœci do modelu w pozycji bie¿¹cej
	Identity(&Entry->BoneMatrices[0]);
	for (uint i = 1; i < GetBoneCount(); i++)
		Entry->BoneMatrices[i] = GetModelToBoneMatrices()[i] * BoneToModelPoseMat[i];

	pimpl->AddBoneMatrixCacheEntry(Entry);
	return &Entry->BoneMatrices[0];
}

bool QMesh::RayCollision(float *OutT, const VEC3 &RayOrig, const VEC3 &RayDir, bool TwoSided, uint TriangleBegin, uint TriangleEnd)
{
	float t;
	*OutT = MAXFLOAT;
	bool Found = false;

	uint VertexStride = pimpl->VertexSize;
	uint NumTriangles = pimpl->Header->NumTriangles;
	uint2 *IB_Data = &pimpl->IB_Data[0];
	char *VB_Data = &pimpl->VB_Data[0];
	uint IndexBegin = TriangleBegin * 3;
	uint IndexEnd = (TriangleEnd == MAXUINT4 ? (pimpl->Header->NumTriangles * 3) : (TriangleEnd * 3));

	for (uint vi = IndexBegin; vi < IndexEnd; vi += 3)
	{
		if (RayToTriangle(
			RayOrig,
			RayDir,
			*(const VEC3*)&VB_Data[IB_Data[vi  ]*VertexStride],
			*(const VEC3*)&VB_Data[IB_Data[vi+1]*VertexStride],
			*(const VEC3*)&VB_Data[IB_Data[vi+2]*VertexStride],
			!TwoSided,
			&t))
		{
			if (t >= 0.f && t < *OutT)
			{
				*OutT = t;
				Found = true;
			}
		}
	}

	return Found;
}

bool QMesh::RayCollision_Bones(float *OutT, const VEC3 &RayOrig, const VEC3 &RayDir, bool TwoSided, float Accuracy, uint Animation, float Time, uint TriangleBegin, uint TriangleEnd)
{
	// Nie ma skinningu - zwyk³a kolizja
	if (!HasSkinning())
		return RayCollision(OutT, RayOrig, RayDir, TwoSided, TriangleBegin, TriangleEnd);

	return RayCollision_Bones(OutT, RayOrig, RayDir, TwoSided, Accuracy, Animation, Time, MAXUINT4, 0.f, 0.f, TriangleBegin, TriangleEnd);
}

bool QMesh::RayCollision_Bones(float *OutT, const VEC3 &RayOrig, const VEC3 &RayDir, bool TwoSided, float Accuracy, uint Animation1, float Time1, uint Animation2, float Time2, float LerpT, uint TriangleBegin, uint TriangleEnd)
{
	// Nie ma skinningu - zwyk³a kolizja
	if (!HasSkinning())
		return RayCollision(OutT, RayOrig, RayDir, TwoSided, TriangleBegin, TriangleEnd);

	// Jeœli przetworzony VB nie jest w cache, wygeneruj VB
	if (!pimpl->SoftwareSkinningVb.CheckCache(Accuracy, Animation1, Time1, Animation2, Time2, LerpT))
	{
		// Wygeneruj macierze dla tej animacji
		const MATRIX *BoneMatrices = GetBoneMatrices(Accuracy, Animation1, Time1, Animation2, Time2, LerpT);

		// Zainicjalizuj strukturê SoftwareSkinningVb
		pimpl->SoftwareSkinningVb.Filled = true;
		pimpl->SoftwareSkinningVb.Animation1 = Animation1;
		pimpl->SoftwareSkinningVb.Animation2 = Animation2;
		pimpl->SoftwareSkinningVb.Time1 = Time1;
		pimpl->SoftwareSkinningVb.Time2 = Time2;
		pimpl->SoftwareSkinningVb.LerpT = LerpT;
		pimpl->SoftwareSkinningVb.Vertices.resize(pimpl->Header->NumVertices);

		// Przetwórz wierzcho³ki
		uint VertexCount = pimpl->Header->NumVertices;
		uint VertexStride = pimpl->VertexSize;
		uint BoneInfoOffset = sizeof(VEC3); // Pos
		const char *VB_Data = &pimpl->VB_Data[0];
		VEC3 TempPos;
		for (uint vi = 0; vi < VertexCount; vi++)
		{
			// Kod analogiczny do tego z Vertex Shadera (MainShader.fx, funkcja SkinPos)
			const VEC3 & InputPos = *(const VEC3*)&VB_Data[vi * VertexStride];
			float Weight1 = *(const float*)&VB_Data[vi * VertexStride + BoneInfoOffset];
			DWORD BoneIndices = *(const DWORD*)&VB_Data[vi * VertexStride + BoneInfoOffset + sizeof(float)];
			VEC3 & OutputPos = pimpl->SoftwareSkinningVb.Vertices[vi];
			assert((BoneIndices & 0xFF) < GetBoneCount());
			assert((BoneIndices & 0xFF) < GetBoneCount());
			Transform(&OutputPos, InputPos, BoneMatrices[BoneIndices & 0xFF]);
			Transform(&TempPos,   InputPos, BoneMatrices[(BoneIndices >> 8) & 0xFF]);
			OutputPos = OutputPos * Weight1 + TempPos * (1.0f - Weight1);
		}
	}

	// Testuj!

	float t;
	*OutT = MAXFLOAT;
	bool Found = false;

	uint NumTriangles = pimpl->Header->NumTriangles;
	uint2 *IB_Data = &pimpl->IB_Data[0];
	const VEC3 *VB_Data = &pimpl->SoftwareSkinningVb.Vertices[0];
	uint IndexBegin = TriangleBegin * 3;
	uint IndexEnd = (TriangleEnd == MAXUINT4 ? (pimpl->Header->NumTriangles * 3) : (TriangleEnd * 3));

	// Przejrzyj wszystkie trójk¹ty
	for (uint vi = IndexBegin; vi < IndexEnd; vi += 3)
	{
		if (RayToTriangle(
			RayOrig,
			RayDir,
			VB_Data[IB_Data[vi  ]],
			VB_Data[IB_Data[vi+1]],
			VB_Data[IB_Data[vi+2]],
			!TwoSided,
			&t))
		{
			if (t >= 0.f && t < *OutT)
			{
				*OutT = t;
				Found = true;
			}
		}
	}

	return Found;
}

const string & QMesh::GetFileName() { return pimpl->FileName; }
bool QMesh::HasSkinning()           { assert(pimpl->Header != NULL); return (pimpl->Header->Flags & 0x02) != 0; }
bool QMesh::HasTangent()            { assert(pimpl->Header != NULL); return (pimpl->Header->Flags & 0x01) != 0; }
uint QMesh::GetFVF()                { return pimpl->FVF; }
uint QMesh::GetVertexSize()         { return pimpl->VertexSize; }
uint QMesh::GetSubmeshCount()       { return pimpl->Submeshes.size(); }
float QMesh::GetBoundingSphereRadius() { assert(pimpl->Header != NULL); return pimpl->Header->BoundingSphereRadius; }
const BOX & QMesh::GetBoundingBox() { assert(pimpl->Header != NULL); return pimpl->Header->BoundingBox; }
uint QMesh::GetVertexCount()        { assert(pimpl->Header != NULL); return pimpl->Header->NumVertices; }
uint QMesh::GetTriangleCount()      { assert(pimpl->Header != NULL); return pimpl->Header->NumTriangles; }
uint QMesh::GetBoneCount()          { assert(pimpl->Header != NULL); return pimpl->Header->NumBones + 1; }
uint QMesh::GetAnimationCount()     { assert(pimpl->Header != NULL); return pimpl->Header->NumAnimations; }
const QMesh::SUBMESH & QMesh::GetSubmesh(uint Index) { assert(Index < GetSubmeshCount()); return *pimpl->Submeshes[Index].get(); }
IDirect3DVertexBuffer9 * QMesh::GetVB() { return pimpl->VB.get(); }
IDirect3DIndexBuffer9 * QMesh::GetIB() { return pimpl->IB.get(); }
const QMesh::BONE & QMesh::GetBone(uint Index) { assert(Index < GetBoneCount()); return *pimpl->Bones[Index].get(); }
QMesh::Animation & QMesh::GetAnimation(uint Index) { assert(Index < GetAnimationCount()); return *pimpl->Animations[Index].get(); }


} // namespace res
