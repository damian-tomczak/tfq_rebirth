/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\D3dUtils.hpp"
#include "IcosphereRes.hpp"

namespace engine
{

const uint g_VertexCount = 42;
const uint g_IndexCount  = 240;
const uint g_PrimitiveCount = g_IndexCount / 3;

const VEC3 g_Vertices[] = {
	VEC3(0.000000f, -1.000000f, 0.000000f),
	VEC3(0.723600f, -0.447215f, -0.525720f),
	VEC3(-0.276385f, -0.447215f, -0.850640f),
	VEC3(-0.894425f, -0.447215f, 0.000000f),
	VEC3(-0.276385f, -0.447215f, 0.850640f),
	VEC3(0.723600f, -0.447215f, 0.525720f),
	VEC3(0.276385f, 0.447215f, -0.850640f),
	VEC3(-0.723600f, 0.447215f, -0.525720f),
	VEC3(-0.723600f, 0.447215f, 0.525720f),
	VEC3(0.276385f, 0.447215f, 0.850640f),
	VEC3(0.894425f, 0.447215f, 0.000000f),
	VEC3(0.000000f, 1.000000f, 0.000000f),
	VEC3(0.425323f, -0.850654f, -0.309011f),
	VEC3(-0.162456f, -0.850654f, -0.499995f),
	VEC3(0.262869f, -0.525738f, -0.809012f),
	VEC3(0.425323f, -0.850654f, 0.309011f),
	VEC3(0.850648f, -0.525736f, 0.000000f),
	VEC3(-0.525730f, -0.850652f, 0.000000f),
	VEC3(-0.688189f, -0.525736f, -0.499997f),
	VEC3(-0.162456f, -0.850654f, 0.499995f),
	VEC3(-0.688189f, -0.525736f, 0.499997f),
	VEC3(0.262869f, -0.525738f, 0.809012f),
	VEC3(0.951058f, 0.000000f, 0.309013f),
	VEC3(0.951058f, 0.000000f, -0.309013f),
	VEC3(0.587786f, 0.000000f, -0.809017f),
	VEC3(0.000000f, 0.000000f, -1.000000f),
	VEC3(-0.587786f, 0.000000f, -0.809017f),
	VEC3(-0.951058f, 0.000000f, -0.309013f),
	VEC3(-0.951058f, 0.000000f, 0.309013f),
	VEC3(-0.587786f, 0.000000f, 0.809017f),
	VEC3(0.000000f, 0.000000f, 1.000000f),
	VEC3(0.587786f, 0.000000f, 0.809017f),
	VEC3(0.688189f, 0.525736f, -0.499997f),
	VEC3(-0.262869f, 0.525738f, -0.809012f),
	VEC3(-0.850648f, 0.525736f, 0.000000f),
	VEC3(-0.262869f, 0.525738f, 0.809012f),
	VEC3(0.688189f, 0.525736f, 0.499997f),
	VEC3(0.525730f, 0.850652f, 0.000000f),
	VEC3(0.162456f, 0.850654f, -0.499995f),
	VEC3(-0.425323f, 0.850654f, -0.309011f),
	VEC3(-0.425323f, 0.850654f, 0.309011f),
	VEC3(0.162456f, 0.850654f, 0.499995f),
};

const uint2 g_Indices[] = {
	1, 12, 14,
	13, 14, 12,
	14, 13, 2,
	12, 0, 13,
	12, 1, 16,
	16, 15, 12,
	15, 16, 5,
	15, 0, 12,
	2, 13, 18,
	17, 18, 13,
	18, 17, 3,
	13, 0, 17,
	3, 17, 20,
	19, 20, 17,
	20, 19, 4,
	17, 0, 19,
	4, 19, 21,
	15, 21, 19,
	21, 15, 5,
	19, 0, 15,
	16, 1, 23,
	23, 22, 16,
	22, 23, 10,
	5, 16, 22,
	14, 2, 25,
	25, 24, 14,
	24, 25, 6,
	1, 14, 24,
	18, 3, 27,
	27, 26, 18,
	26, 27, 7,
	2, 18, 26,
	20, 4, 29,
	29, 28, 20,
	28, 29, 8,
	3, 20, 28,
	21, 5, 31,
	31, 30, 21,
	30, 31, 9,
	4, 21, 30,
	10, 23, 32,
	24, 32, 23,
	32, 24, 6,
	23, 1, 24,
	6, 25, 33,
	26, 33, 25,
	33, 26, 7,
	25, 2, 26,
	7, 27, 34,
	28, 34, 27,
	34, 28, 8,
	27, 3, 28,
	8, 29, 35,
	30, 35, 29,
	35, 30, 9,
	29, 4, 30,
	9, 31, 36,
	22, 36, 31,
	36, 22, 10,
	31, 5, 22,
	32, 6, 38,
	38, 37, 32,
	37, 38, 11,
	10, 32, 37,
	33, 7, 39,
	39, 38, 33,
	38, 39, 11,
	6, 33, 38,
	34, 8, 40,
	40, 39, 34,
	39, 40, 11,
	7, 34, 39,
	35, 9, 41,
	41, 40, 35,
	40, 41, 11,
	8, 35, 40,
	36, 10, 37,
	37, 41, 36,
	41, 37, 11,
	9, 36, 41,
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa IcosphereRes

struct IcosphereRes_pimpl
{
	scoped_ptr<IDirect3DVertexBuffer9, ReleasePolicy> VB;
	scoped_ptr<IDirect3DIndexBuffer9, ReleasePolicy> IB;
};


IcosphereRes::IcosphereRes(const string &Name, const string &Group) :
	res::D3dResource(Name, Group),
	pimpl(new IcosphereRes_pimpl)
{
}

IcosphereRes::~IcosphereRes()
{
	pimpl.reset();
}

IDirect3DVertexBuffer9 * IcosphereRes::GetVB()
{
	return pimpl->VB.get();
}

IDirect3DIndexBuffer9 * IcosphereRes::GetIB()
{
	return pimpl->IB.get();
}

uint IcosphereRes::GetVertexCount()
{
	return g_VertexCount;
}

uint IcosphereRes::GetFVF()
{
	return D3DFVF_XYZ;
}

uint IcosphereRes::GetIndexCount()
{
	return g_IndexCount;
}

void IcosphereRes::SetState()
{
	assert(pimpl->VB != NULL && pimpl->IB != NULL);

	frame::Dev->SetStreamSource(0, pimpl->VB.get(), 0, sizeof(VEC3));
	frame::Dev->SetIndices(pimpl->IB.get());
	frame::Dev->SetFVF(D3DFVF_XYZ);
}

void IcosphereRes::DrawGeometry()
{
	assert(pimpl->VB != NULL && pimpl->IB != NULL);

	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, g_VertexCount, 0, g_PrimitiveCount) );
	frame::RegisterDrawCall(g_PrimitiveCount);
}

void IcosphereRes::OnDeviceCreate()
{
	ERR_TRY;

	HRESULT hr;
	LOG(0x08, Format("IcosphereRes: \"#\" Creating buffers.") % GetName());

	// Wierzcho³ki

	uint vb_size = g_VertexCount * sizeof(VEC3);
	IDirect3DVertexBuffer9 *vb;

	hr = frame::Dev->CreateVertexBuffer(
		vb_size,
		D3DUSAGE_WRITEONLY,
		GetFVF(),
		D3DPOOL_MANAGED,
		&vb,
		NULL);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na utworzyæ bufora wierzcho³ków.");
	pimpl->VB.reset(vb);

	{
		VertexBufferLock vb_lock(vb, 0);
		CopyMem(vb_lock.GetData(), g_Vertices, vb_size);
	}

	// Indeksy

	uint ib_size = g_IndexCount * sizeof(uint2);
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
		CopyMem(ib_lock.GetData(), g_Indices, ib_size);
	}

	ERR_CATCH("Nie mo¿na utworzyæ zasobu IcosphereRes.");
}

void IcosphereRes::OnDeviceDestroy()
{
	pimpl->IB.reset();
	pimpl->VB.reset();
}

void IcosphereRes::OnDeviceRestore()
{
}

void IcosphereRes::OnDeviceInvalidate()
{
}


} // namespace engine

/*
Ten modu³ oferuje specyficzny bezparametrowy zasób. Jest nim Icosphere - siatka
w kszta³cie sfery, niskopoligonowa, w formacie wierzcho³ków zawieraj¹cym
wy³¹cznie pozycje (bez normalnych czy wspó³rzêdnych tekstur), umieszczana w
sprzêtowych buforach VB i IB. Nie ma sensu tworzyæ wielu zasobów tej klasy -
wystarczy jeden. Promieñ wynosi 1.
*/
