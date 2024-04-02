/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef INDOOR_MAP_H_
#define INDOOR_MAP_H_

#include "..\Framework\Res_d3d.hpp"

namespace engine
{

struct QMap_pimpl;

class QMap : public res::D3dResource
{
public:
	struct DRAW_VERTEX
	{
		VEC3 Pos;
		VEC3 Normal;
		VEC2 Tex;
		VEC3 Tangent;
		VEC3 Binormal;

		enum
		{
			FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX3 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE3(1) | D3DFVF_TEXCOORDSIZE3(2),
		};
	};

	struct DRAW_FRAGMENT
	{
		uint MaterialIndex;
		uint IndexBegin;
		uint IndexEnd;
		uint VertexBegin;
		uint VertexEnd;
	};

	struct DRAW_NODE
	{
		common::BOX Bounds;
		std::vector<DRAW_FRAGMENT> Fragments;
		DRAW_NODE *SubNodes[8];
	};

	struct COLLISION_NODE
	{
		common::BOX Bounds;
		uint IndexBegin;
		uint IndexEnd;
		COLLISION_NODE *SubNodes[8];
	};

	static res::IResource * Create(const string &Name, const string &Group, Tokenizer &Params);
	static void RegisterResourceType();

	QMap(const string &Name, const string &Group, const string &FileName);
	~QMap();

	// Zwraca info o zasobie
	const string & GetFileName();

	// Zwraca dane zasobu - dostêpne kiedy zasób jest wczytany
	uint GetMaterialCount();
	const string & GetMaterialName(uint Index);
	uint GetDrawNodeCount();
	uint GetDrawVertexCount();
	uint GetDrawIndexCount();
	DRAW_NODE * GetDrawTree();
	uint GetCollisionNodeCount();
	uint GetCollisionVertexCount();
	uint GetCollisionIndexCount();
	const VEC3 * GetCollisionVB();
	uint4 * GetCollisionIB();
	COLLISION_NODE * GetCollisionTree();
	void GetBoundingBox(BOX *Out);

	// Zwraca zasoby D3D - dostêpne tylko kiedy zasób jest wczytany a urz¹dzenie D3D nieutracone
	IDirect3DVertexBuffer9 * GetDrawVB();
	IDirect3DIndexBuffer9 * GetDrawIB();

protected:
	// ======== Implementacja IResource ========
	virtual void OnLoad();
	virtual void OnUnload();

	// ======== Implementacja D3dResource ========
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

private:
	scoped_ptr<QMap_pimpl> pimpl;
};

} // namespace engine

#endif
