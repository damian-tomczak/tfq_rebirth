/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\D3dUtils.hpp"
#include "QMap.hpp"

namespace engine
{

struct QMap_pimpl
{
	string FileName;
	uint DrawVBOffset;
	uint DrawIBOffset;

	STRING_VECTOR Materials;

	uint DrawNodeCount;
	uint DrawVertexCount;
	uint DrawIndexCount;
	scoped_ptr<IDirect3DVertexBuffer9, ReleasePolicy> DrawVB;
	scoped_ptr<IDirect3DIndexBuffer9, ReleasePolicy> DrawIB;
	DynamicFreeList<QMap::DRAW_NODE> DrawTreeMemory;
	QMap::DRAW_NODE *DrawTree;

	uint CollisionNodeCount;
	std::vector<VEC3> CollisionVB;
	std::vector<uint4> CollisionIB;
	DynamicFreeList<QMap::COLLISION_NODE> CollisionTreeMemory;
	QMap::COLLISION_NODE *CollisionTree;

	QMap_pimpl();
	~QMap_pimpl();

	void DeleteDrawNode(QMap::DRAW_NODE *Node);
	void DeleteCollisionNode(QMap::COLLISION_NODE *Node);
	void ClearTrees();
	void LoadMaterials(Stream &F);
	void LoadDrawTreeNode(Stream &F, QMap::DRAW_NODE *Node);
	void LoadCollisionTreeNode(Stream &F, QMap::COLLISION_NODE *Node);
};

QMap_pimpl::QMap_pimpl() :
	DrawTreeMemory(1000),
	DrawTree(NULL),
	CollisionTreeMemory(1000),
	CollisionTree(NULL)
{
}

QMap_pimpl::~QMap_pimpl()
{
	ClearTrees();
}

void QMap_pimpl::DeleteDrawNode(QMap::DRAW_NODE *Node)
{
	if (Node->SubNodes[0] != NULL)
	{
		for (uint i = 0; i < 8; i++)
			DeleteDrawNode(Node->SubNodes[i]);
	}
	DrawTreeMemory.Delete(Node);
}

void QMap_pimpl::DeleteCollisionNode(QMap::COLLISION_NODE *Node)
{
	if (Node->SubNodes[0] != NULL)
	{
		for (uint i = 0; i < 8; i++)
			DeleteCollisionNode(Node->SubNodes[i]);
	}
	CollisionTreeMemory.Delete(Node);
}

void QMap_pimpl::ClearTrees()
{
	if (DrawTree != NULL)
	{
		DeleteDrawNode(DrawTree);
		DrawTree = NULL;
	}

	if (CollisionTree != NULL)
	{
		DeleteCollisionNode(CollisionTree);
		CollisionTree = NULL;
	}
}

void QMap_pimpl::LoadMaterials(Stream &F)
{
	uint MaterialCount;
	F.ReadEx(&MaterialCount);
	Materials.resize(MaterialCount);
	for (uint mi = 0; mi < MaterialCount; mi++)
		F.ReadString1(&Materials[mi]);
}

// Funkcja rekurencyjna
void QMap_pimpl::LoadDrawTreeNode(Stream &F, QMap::DRAW_NODE *Node)
{
	DrawNodeCount++;

	F.ReadEx(&Node->Bounds);

	uint1 HasSubNodes;
	F.ReadEx(&HasSubNodes);
	for (uint si = 0; si < 8; si++)
		Node->SubNodes[si] = NULL;
	if (HasSubNodes > 0)
	{
		for (uint si = 0; si < 8; si++)
		{
			Node->SubNodes[si] = DrawTreeMemory.New();
			LoadDrawTreeNode(F, Node->SubNodes[si]);
		}
	}

	uint4 FragmentCount;
	F.ReadEx(&FragmentCount);
	Node->Fragments.resize(FragmentCount);
	for (uint fi = 0; fi < FragmentCount; fi++)
	{
		F.ReadEx(&Node->Fragments[fi].MaterialIndex);
		F.ReadEx(&Node->Fragments[fi].IndexBegin);
		F.ReadEx(&Node->Fragments[fi].IndexEnd);
		F.ReadEx(&Node->Fragments[fi].VertexBegin);
		F.ReadEx(&Node->Fragments[fi].VertexEnd);
	}
}

// Funkcja rekurencyjna
void QMap_pimpl::LoadCollisionTreeNode(Stream &F, QMap::COLLISION_NODE *Node)
{
	CollisionNodeCount++;

	F.ReadEx(&Node->Bounds);

	uint1 HasSubNodes;
	F.ReadEx(&HasSubNodes);
	for (uint si = 0; si < 8; si++)
		Node->SubNodes[si] = NULL;
	if (HasSubNodes > 0)
	{
		for (uint si = 0; si < 8; si++)
		{
			Node->SubNodes[si] = CollisionTreeMemory.New();
			LoadCollisionTreeNode(F, Node->SubNodes[si]);
		}
	}

	F.ReadEx(&Node->IndexBegin);
	F.ReadEx(&Node->IndexEnd);
}


res::IResource * QMap::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	string FileName = Params.GetString();
	Params.Next();

	Params.AssertSymbol(';');
	Params.Next();

	return new QMap(Name, Group, FileName);
}

void QMap::RegisterResourceType()
{
	res::g_Manager->RegisterResourceType("QMap", &QMap::Create);
}

QMap::QMap(const string &Name, const string &Group, const string &FileName) :
	D3dResource(Name, Group),
	pimpl(new QMap_pimpl)
{
	pimpl->FileName = FileName;
}

QMap::~QMap()
{
	pimpl.reset();
}

const string & QMap::GetFileName()
{
	return pimpl->FileName;
}

uint QMap::GetMaterialCount()
{
	return pimpl->Materials.size();
}

const string & QMap::GetMaterialName(uint Index)
{
	assert(Index < pimpl->Materials.size());
	return pimpl->Materials[Index];
}

uint QMap::GetDrawNodeCount()
{
	return pimpl->DrawNodeCount;
}

uint QMap::GetDrawVertexCount()
{
	return pimpl->DrawVertexCount;
}

uint QMap::GetDrawIndexCount()
{
	return pimpl->DrawIndexCount;
}

QMap::DRAW_NODE * QMap::GetDrawTree()
{
	return pimpl->DrawTree;
}

uint QMap::GetCollisionNodeCount()
{
	return pimpl->CollisionNodeCount;
}

uint QMap::GetCollisionVertexCount()
{
	return pimpl->CollisionVB.size();
}

uint QMap::GetCollisionIndexCount()
{
	return pimpl->CollisionIB.size();
}

const VEC3 * QMap::GetCollisionVB()
{
	return &pimpl->CollisionVB[0];
}

uint4 * QMap::GetCollisionIB()
{
	return &pimpl->CollisionIB[0];
}

QMap::COLLISION_NODE * QMap::GetCollisionTree()
{
	return pimpl->CollisionTree;
}

void QMap::GetBoundingBox(BOX *Out)
{
	*Out = GetDrawTree()->Bounds;
}

IDirect3DVertexBuffer9 * QMap::GetDrawVB()
{
	return pimpl->DrawVB.get();
}

IDirect3DIndexBuffer9 * QMap::GetDrawIB()
{
	return pimpl->DrawIB.get();
}

void QMap::OnLoad()
{
	ERR_TRY;
	{
		common::FileStream F(pimpl->FileName, common::FM_READ);

		LOG(0x08, Format("QMap: \"#\" Loading from \"#\"") % GetName() % GetFileName());

		// Nag³ówek
		char Header[9];
		F.MustRead(Header, 8);
		Header[8] = '\0';
		if (Header != string("TFQMAP10"))
			throw Error("B³êdny nag³ówek.");

		// Materia³y
		pimpl->LoadMaterials(F);

		// DrawVB
		F.ReadEx(&pimpl->DrawVertexCount);
		pimpl->DrawVBOffset = F.GetPos();
		F.Skip(pimpl->DrawVertexCount * sizeof(DRAW_VERTEX));

		// DrawIB
		F.ReadEx(&pimpl->DrawIndexCount);
		pimpl->DrawIBOffset = F.GetPos();
		F.Skip(pimpl->DrawIndexCount * sizeof(uint4));

		// DrawTree
		pimpl->DrawNodeCount = 0;
		pimpl->DrawTree = pimpl->DrawTreeMemory.New();
		pimpl->LoadDrawTreeNode(F, pimpl->DrawTree);

		// CollisionVB
		uint4 CollisionVertexCount;
		F.ReadEx(&CollisionVertexCount);
		pimpl->CollisionVB.resize(CollisionVertexCount);
		F.MustRead(&pimpl->CollisionVB[0], CollisionVertexCount * sizeof(VEC3));

		// CollisionIB
		uint4 CollisionIndexCount;
		F.ReadEx(&CollisionIndexCount);
		pimpl->CollisionIB.resize(CollisionIndexCount);
		F.MustRead(&pimpl->CollisionIB[0], CollisionIndexCount * sizeof(uint4));

		// CollisionTree
		pimpl->CollisionNodeCount = 0;
		pimpl->CollisionTree = pimpl->CollisionTreeMemory.New();
		pimpl->LoadCollisionTreeNode(F, pimpl->CollisionTree);
	}
	ERR_CATCH("Nie mo¿na wczytaæ mapy z pliku \"" + pimpl->FileName + "\"");

	D3dResource::OnLoad();
}

void QMap::OnUnload()
{
	D3dResource::OnUnload();

	// Usuñ wszystko co zajmuje pamiêæ
	pimpl->Materials.clear();
	pimpl->CollisionVB.clear();
	pimpl->CollisionIB.clear();
	pimpl->ClearTrees();
}

void QMap::OnDeviceCreate()
{
	ERR_TRY;

	HRESULT hr;
	LOG(0x08, Format("QMap: \"#\" Loading mesh from \"#\"") % GetName() % GetFileName());

	common::FileStream File(pimpl->FileName, common::FM_READ);

	// Wierzcho³ki

	uint vb_size = pimpl->DrawVertexCount * sizeof(DRAW_VERTEX);
	IDirect3DVertexBuffer9 *vb;

	hr = frame::Dev->CreateVertexBuffer(
		vb_size,
		D3DUSAGE_WRITEONLY,
		DRAW_VERTEX::FVF,
		D3DPOOL_MANAGED,
		&vb,
		NULL);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na utworzyæ bufora wierzcho³ków.");
	pimpl->DrawVB.reset(vb);

	{
		VertexBufferLock vb_lock(vb, 0);

		File.SetPos((int)pimpl->DrawVBOffset);
		File.MustRead(vb_lock.GetData(), vb_size);
	}

	// Indeksy

	uint ib_size = pimpl->DrawIndexCount * sizeof(uint4);
	IDirect3DIndexBuffer9 *ib;

	hr = frame::Dev->CreateIndexBuffer(
		ib_size,
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX32,
		D3DPOOL_MANAGED,
		&ib,
		NULL);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na utworzyæ bufora indeksów.");
	pimpl->DrawIB.reset(ib);

	{
		IndexBufferLock ib_lock(ib, 0);

		File.SetPos((int)pimpl->DrawIBOffset);
		File.MustRead(ib_lock.GetData(), ib_size);
	}

	ERR_CATCH("Nie mo¿na wczytaæ siatki z pliku \"" + pimpl->FileName + "\"");
}

void QMap::OnDeviceDestroy()
{
	pimpl->DrawIB.reset();
	pimpl->DrawVB.reset();
}

void QMap::OnDeviceRestore()
{
}

void QMap::OnDeviceInvalidate()
{
}


} // namespace engine
