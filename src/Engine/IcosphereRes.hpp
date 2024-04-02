/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef ICOSPHERE_RES_H_
#define ICOSPHERE_RES_H_

#include "..\Framework\Res_d3d.hpp"

namespace engine
{

struct IcosphereRes_pimpl;

class IcosphereRes : public res::D3dResource
{
public:
	IcosphereRes(const string &Name, const string &Group);
	~IcosphereRes();

	// Dostêpne tylko kiedy zasób jest wczytany, a urz¹dzenie D3D nieutracone
	IDirect3DVertexBuffer9 * GetVB();
	IDirect3DIndexBuffer9 *GetIB();
	static uint GetVertexCount();
	static uint GetIndexCount();
	static uint GetFVF();
	// Ustawia SetStreamSource, SetIndices i SetFVF
	void SetState();
	// Odrysowuje geometriê
	void DrawGeometry();

protected:
	// ======== Implementacja D3dResource ========
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

private:
	scoped_ptr<IcosphereRes_pimpl> pimpl;
};

} // namespace engine

#endif
