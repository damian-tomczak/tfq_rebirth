/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "Framework.hpp"
#include "D3dUtils.hpp"


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa RenderTargetHelper

RenderTargetHelper::RenderTargetHelper(IDirect3DSurface9 *NewRenderTarget, IDirect3DSurface9 *NewDepthStencil) :
	m_OldRenderTarget(NULL),
	m_OldDepthStencil(NULL)
{
	ERR_TRY

	HRESULT hr;

	if (NewRenderTarget)
	{
		hr = frame::Dev->GetRenderTarget(0, &m_OldRenderTarget);
		if (FAILED(hr)) throw DirectXError(hr, "Dev->GetRenderTarget", __FILE__, __LINE__);

		hr = frame::Dev->SetRenderTarget(0, NewRenderTarget);
		if (FAILED(hr)) throw DirectXError(hr, "Dev->SetRenderTarget", __FILE__, __LINE__);
	}

	if (NewDepthStencil)
	{
		hr = frame::Dev->GetDepthStencilSurface(&m_OldDepthStencil);
		if (FAILED(hr)) throw DirectXError(hr, "Dev->GetDepthStencilSurface", __FILE__, __LINE__);

		hr = frame::Dev->SetDepthStencilSurface(NewDepthStencil);
		if (FAILED(hr)) throw DirectXError(hr, "Dev->SetDepthStencilSurface", __FILE__, __LINE__);
	}

	ERR_CATCH_FUNC
}

RenderTargetHelper::~RenderTargetHelper()
{
	if (m_OldDepthStencil)
	{
		frame::Dev->SetDepthStencilSurface(m_OldDepthStencil);
		m_OldDepthStencil->Release(); // Tak tak, to co pobrane GetDepthStencilSurface trzeba zwolniæ.
	}

	if (m_OldRenderTarget)
	{
		frame::Dev->SetRenderTarget(0, m_OldRenderTarget);
		m_OldRenderTarget->Release(); // Tak tak, to co pobrane GetRenderTarget trzeba zwolniæ.
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa D3dxBufferWrapper

D3dxBufferWrapper::D3dxBufferWrapper(size_t NumBytes)
{
	assert(NumBytes > 0);

	HRESULT hr = D3DXCreateBuffer(NumBytes, &m_Buf);
	if (FAILED(hr))
		throw DirectXError(hr, "Cannot create buffer.", __FILE__, __LINE__);
}

D3dxBufferWrapper::D3dxBufferWrapper(size_t NumBytes, const void *Data)
{
	assert(NumBytes > 0);

	HRESULT hr = D3DXCreateBuffer(NumBytes, &m_Buf);
	if (FAILED(hr))
		throw DirectXError(hr, "Cannot create buffer.", __FILE__, __LINE__);

	memcpy(m_Buf->GetBufferPointer(), Data, NumBytes);
}

D3dxBufferWrapper::~D3dxBufferWrapper()
{
	SAFE_RELEASE(m_Buf);
}

D3dxBufferWrapper * D3dxBufferWrapper::Clone() const
{
	return new D3dxBufferWrapper(GetNumBytes(), GetData());
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa VertexBufferLock

VertexBufferLock::VertexBufferLock(ID3DXMesh *Mesh, DWORD Flags) :
	m_Mesh(Mesh),
	m_VB(NULL)
{
	HRESULT hr = Mesh->LockVertexBuffer(Flags, &m_Data);
	if (FAILED(hr)) throw DirectXError(hr, "Cannot lock mesh vertex buffer.");
}

VertexBufferLock::VertexBufferLock(IDirect3DVertexBuffer9 *VB, DWORD Flags)	:
	m_Mesh(NULL),
	m_VB(VB)
{
	HRESULT hr = VB->Lock(0, 0, &m_Data, Flags);
	if (FAILED(hr)) throw DirectXError(hr, "Cannot lock vertex buffer.");
}

VertexBufferLock::VertexBufferLock(IDirect3DVertexBuffer9 *VB, DWORD Flags, UINT OffsetToLock, UINT SizeToLock) :
	m_Mesh(NULL),
	m_VB(VB)
{
	HRESULT hr = VB->Lock(OffsetToLock, SizeToLock, &m_Data, Flags);
	if (FAILED(hr)) throw DirectXError(hr, "Cannot lock vertex buffer.");
}

VertexBufferLock::~VertexBufferLock()
{
	if (m_VB)
		m_VB->Unlock();
	else if (m_Mesh)
		m_Mesh->UnlockVertexBuffer();
}

void VertexBufferLock::Unlock()
{
	if (m_VB)
	{
		m_VB->Unlock();
		m_VB = NULL;
	}
	else if (m_Mesh)
	{
		m_Mesh->UnlockVertexBuffer();
		m_Mesh = NULL;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa IndexBufferLock

IndexBufferLock::IndexBufferLock(ID3DXMesh *Mesh, DWORD Flags) :
	m_Mesh(Mesh),
	m_IB(NULL)
{
	HRESULT hr = Mesh->LockIndexBuffer(Flags, &m_Data);
	if (FAILED(hr)) throw DirectXError(hr, "Cannot lock mesh index buffer.");
}

IndexBufferLock::IndexBufferLock(IDirect3DIndexBuffer9 *IB, DWORD Flags)	:
	m_Mesh(NULL),
	m_IB(IB)
{
	HRESULT hr = IB->Lock(0, 0, &m_Data, Flags);
	if (FAILED(hr)) throw DirectXError(hr, "Cannot lock index buffer.");
}

IndexBufferLock::IndexBufferLock(IDirect3DIndexBuffer9 *IB, DWORD Flags, UINT OffsetToLock, UINT SizeToLock) :
	m_Mesh(NULL),
	m_IB(IB)
{
	HRESULT hr = IB->Lock(OffsetToLock, SizeToLock, &m_Data, Flags);
	if (FAILED(hr)) throw DirectXError(hr, "Cannot lock index buffer.");
}

IndexBufferLock::~IndexBufferLock()
{
	if (m_IB)
		m_IB->Unlock();
	else if (m_Mesh)
		m_Mesh->UnlockIndexBuffer();
}

void IndexBufferLock::Unlock()
{
	if (m_IB)
	{
		m_IB->Unlock();
		m_IB = NULL;
	}
	else if (m_Mesh)
	{
		m_Mesh->UnlockIndexBuffer();
		m_Mesh = NULL;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa MemoryTexture

MemoryTexture::MemoryTexture(const string &FileName)
{
	ERR_TRY;

	IDirect3DTexture9 *TexturePtr;
	HRESULT hr = D3DXCreateTextureFromFileEx(
		frame::Dev, // Device
		FileName.c_str(), // SrcFile
		D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, // Width, Height
		D3DX_FROM_FILE, // MipLevels
		0, // Usage
		D3DFMT_A8R8G8B8, // Format
		D3DPOOL_SCRATCH, // Pool
		D3DX_DEFAULT, // Filter
		D3DX_DEFAULT, // MipFilter
		0, // ColorKey
		NULL, // SrcInfo
		NULL, // Palette
		&TexturePtr); // Texture
	if (FAILED(hr))
		throw DirectXError(hr, "D3DXCreateTextureFromFileEx", __FILE__, __LINE__);
	m_Texture.reset(TexturePtr);

	D3DSURFACE_DESC Desc; m_Texture->GetLevelDesc(0, &Desc);
	if (Desc.Format != D3DFMT_A8R8G8B8 && Desc.Format != D3DFMT_X8R8G8B8)
		throw Error("B³êdny format wczytanej tekstury. Wymagany X8R8G8B8 lub A8R8G8B8 (32 bpp).", __FILE__, __LINE__);
	m_Width = Desc.Width;
	m_Height = Desc.Height;
	m_Format = Desc.Format;

	hr = m_Texture->LockRect(0, &m_LockedRect, NULL, D3DLOCK_READONLY);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na zablokowaæ tektsury.", __FILE__, __LINE__);

	ERR_CATCH_FUNC;
}

MemoryTexture::~MemoryTexture()
{
	m_Texture->UnlockRect(0);
}


namespace FVF
{

void FvfToString_Short(string *Out, DWORD Fvf)
{
	Out->clear();

	switch (Fvf & D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZ: *Out += 'X'; break;
	case D3DFVF_XYZW: *Out += 'W'; break;
	case D3DFVF_XYZRHW: *Out += 'R'; break;
	case D3DFVF_XYZB1: *Out += 'a'; break;
	case D3DFVF_XYZB2: *Out += 'b'; break;
	case D3DFVF_XYZB3: *Out += 'c'; break;
	case D3DFVF_XYZB4: *Out += 'd'; break;
	case D3DFVF_XYZB5: *Out += 'e'; break;
	}

	if (Fvf & D3DFVF_NORMAL)
		*Out += 'N';
	if (Fvf & D3DFVF_PSIZE)
		*Out += 'P';
	if (Fvf & D3DFVF_DIFFUSE)
		*Out += 'D';
	if (Fvf & D3DFVF_SPECULAR)
		*Out += 'S';

	size_t NumTexCoords = (Fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	for (size_t i = 0; i < NumTexCoords; i++)
	{
		size_t TexCoordSize = (Fvf >> (i*2 + 16)) & 3;
		switch (TexCoordSize)
		{
		case D3DFVF_TEXTUREFORMAT1: *Out += '1'; break;
		case D3DFVF_TEXTUREFORMAT2: *Out += '2'; break;
		case D3DFVF_TEXTUREFORMAT3: *Out += '3'; break;
		case D3DFVF_TEXTUREFORMAT4: *Out += '4'; break;
		}
	}

	if (Fvf & D3DFVF_LASTBETA_UBYTE4)
		*Out += 'u';
	if (Fvf & D3DFVF_LASTBETA_D3DCOLOR)
		*Out += 'l';
}

bool StringToFvf_Short(DWORD *Out, const string &s)
{
	*Out = 0;
	size_t TexCoordIndex = 0;

	for (size_t i = 0; i < s.length(); i++)
	{
		switch (s[i])
		{
		case 'X':
			*Out |= D3DFVF_XYZ;
			break;
		case 'W':
			*Out |= D3DFVF_XYZW;
			break;
		case 'R':
			*Out |= D3DFVF_XYZRHW;
			break;
		case 'a':
			*Out |= D3DFVF_XYZB1;
			break;
		case 'b':
			*Out |= D3DFVF_XYZB2;
			break;
		case 'c':
			*Out |= D3DFVF_XYZB3;
			break;
		case 'd':
			*Out |= D3DFVF_XYZB4;
			break;
		case 'e':
			*Out |= D3DFVF_XYZB5;
			break;
		case 'N':
			*Out |= D3DFVF_NORMAL;
			break;
		case 'P':
			*Out |= D3DFVF_PSIZE;
			break;
		case 'D':
			*Out |= D3DFVF_DIFFUSE;
			break;
		case 'S':
			*Out |= D3DFVF_SPECULAR;
			break;
		case '1':
			*Out |= D3DFVF_TEXCOORDSIZE1(TexCoordIndex);
			TexCoordIndex++;
			break;
		case '2':
			*Out |= D3DFVF_TEXCOORDSIZE2(TexCoordIndex);
			TexCoordIndex++;
			break;
		case '3':
			*Out |= D3DFVF_TEXCOORDSIZE3(TexCoordIndex);
			TexCoordIndex++;
			break;
		case '4':
			*Out |= D3DFVF_TEXCOORDSIZE4(TexCoordIndex);
			TexCoordIndex++;
			break;
		case 'u':
			*Out |= D3DFVF_LASTBETA_UBYTE4;
			break;
		case 'l':
			*Out |= D3DFVF_LASTBETA_D3DCOLOR;
			break;
		default:
			*Out = 0;
			return false;
		}
	}

	*Out |= (TexCoordIndex << D3DFVF_TEXCOUNT_SHIFT);

	return true;
}

size_t CalcNumComponents(DWORD Fvf)
{
	size_t R = 0;

	if (Fvf & D3DFVF_POSITION_MASK)
		R++;

	if (Fvf & D3DFVF_NORMAL)
		R++;
	if (Fvf & D3DFVF_PSIZE)
		R++;
	if (Fvf & D3DFVF_DIFFUSE)
		R++;
	if (Fvf & D3DFVF_SPECULAR)
		R++;

	size_t NumTexCoords = (Fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	R += NumTexCoords;

	return R;
}

size_t CalcVertexSize(DWORD Fvf)
{
	size_t R = 0;

	switch (Fvf & D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZ: R += 4*3; break;
	case D3DFVF_XYZW: R += 4*4; break;
	case D3DFVF_XYZRHW: R += 4*4; break;
	case D3DFVF_XYZB1: R += 4*4; break;
	case D3DFVF_XYZB2: R += 4*5; break;
	case D3DFVF_XYZB3: R += 4*6; break;
	case D3DFVF_XYZB4: R += 4*7; break;
	case D3DFVF_XYZB5: R += 4*8; break;
	}

	if (Fvf & D3DFVF_NORMAL)
		R += 4*3;
	if (Fvf & D3DFVF_PSIZE)
		R += 4*1;
	if (Fvf & D3DFVF_DIFFUSE)
		R += 4*1;
	if (Fvf & D3DFVF_SPECULAR)
		R += 4*1;

	size_t NumTexCoords = (Fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	for (size_t i = 0; i < NumTexCoords; i++)
	{
		size_t TexCoordSize = (Fvf >> (i*2 + 16)) & 3;
		switch (TexCoordSize)
		{
		case D3DFVF_TEXTUREFORMAT1: R += 4*1; break;
		case D3DFVF_TEXTUREFORMAT2: R += 4*2; break;
		case D3DFVF_TEXTUREFORMAT3: R += 4*3; break;
		case D3DFVF_TEXTUREFORMAT4: R += 4*4; break;
		}
	}

	return R;
}

void CalcComponentSizesAndOffsets(size_t *OutSizes, size_t *OutOffsets, DWORD Fvf)
{
	size_t I = 0;
	size_t off = 0;

	switch (Fvf & D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZ: OutOffsets[I] = off; OutSizes[I] = 4*3; off += OutSizes[I]; I++; break;
	case D3DFVF_XYZW: OutOffsets[I] = off; OutSizes[I] = 4*4; off += OutSizes[I]; I++; break;
	case D3DFVF_XYZRHW: OutOffsets[I] = off; OutSizes[I] = 4*4; off += OutSizes[I]; I++; break;
	case D3DFVF_XYZB1: OutOffsets[I] = off; OutSizes[I] = 4*4; off += OutSizes[I]; I++; break;
	case D3DFVF_XYZB2: OutOffsets[I] = off; OutSizes[I] = 4*5; off += OutSizes[I]; I++; break;
	case D3DFVF_XYZB3: OutOffsets[I] = off; OutSizes[I] = 4*6; off += OutSizes[I]; I++; break;
	case D3DFVF_XYZB4: OutOffsets[I] = off; OutSizes[I] = 4*7; off += OutSizes[I]; I++; break;
	case D3DFVF_XYZB5: OutOffsets[I] = off; OutSizes[I] = 4*8; off += OutSizes[I]; I++; break;
	}

	if (Fvf & D3DFVF_NORMAL)
	{
		OutOffsets[I] = off; OutSizes[I] = 4*3; off += OutSizes[I]; I++;
	}
	if (Fvf & D3DFVF_PSIZE)
	{
		OutOffsets[I] = off; OutSizes[I] = 4*1; off += OutSizes[I]; I++;
	}
	if (Fvf & D3DFVF_DIFFUSE)
	{
		OutOffsets[I] = off; OutSizes[I] = 4*1; off += OutSizes[I]; I++;
	}
	if (Fvf & D3DFVF_SPECULAR)
	{
		OutOffsets[I] = off; OutSizes[I] = 4*1; off += OutSizes[I]; I++;
	}

	size_t NumTexCoords = (Fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	for (size_t i = 0; i < NumTexCoords; i++)
	{
		size_t TexCoordSize = (Fvf >> (i*2 + 16)) & 3;
		switch (TexCoordSize)
		{
		case D3DFVF_TEXTUREFORMAT1: OutOffsets[I] = off; OutSizes[I] = 4*1; off += OutSizes[I]; I++; break;
		case D3DFVF_TEXTUREFORMAT2: OutOffsets[I] = off; OutSizes[I] = 4*2; off += OutSizes[I]; I++; break;
		case D3DFVF_TEXTUREFORMAT3: OutOffsets[I] = off; OutSizes[I] = 4*3; off += OutSizes[I]; I++; break;
		case D3DFVF_TEXTUREFORMAT4: OutOffsets[I] = off; OutSizes[I] = 4*4; off += OutSizes[I]; I++; break;
		}
	}
}

} // namespace FVF


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje globalne

void CalcCubeMapViewMatrix(MATRIX *Out, const VEC3 &EyePos, D3DCUBEMAP_FACES FaceType)
{
	VEC3 Forward, Up;

	switch(FaceType)
	{
	case D3DCUBEMAP_FACE_POSITIVE_X:
		Forward = VEC3( 1.0f, 0.0f, 0.0f );
		Up      = VEC3( 0.0f, 1.0f, 0.0f );
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_X:
		Forward = VEC3(-1.0f, 0.0f, 0.0f );
		Up      = VEC3( 0.0f, 1.0f, 0.0f );
		break;
	case D3DCUBEMAP_FACE_POSITIVE_Y:
		Forward = VEC3( 0.0f, 1.0f, 0.0f );
		Up   	 = VEC3( 0.0f, 0.0f,-1.0f );
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_Y:
		Forward = VEC3( 0.0f,-1.0f, 0.0f );
		Up   	 = VEC3( 0.0f, 0.0f, 1.0f );
		break;
	case D3DCUBEMAP_FACE_POSITIVE_Z:
		Forward = VEC3( 0.0f, 0.0f, 1.0f );
		Up   	 = VEC3( 0.0f, 1.0f, 0.0f );
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_Z:
		Forward = VEC3( 0.0f, 0.0f,-1.0f );
		Up   	 = VEC3( 0.0f, 1.0f, 0.0f );
		break;
	}

	LookAtLH(Out, EyePos, Forward, Up);
}

void GetCubeMapForwardDir(VEC3 *Out, D3DCUBEMAP_FACES FaceType)
{
	switch(FaceType)
	{
	case D3DCUBEMAP_FACE_POSITIVE_X:
		*Out = VEC3::POSITIVE_X;
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_X:
		*Out = VEC3::NEGATIVE_X;
		break;
	case D3DCUBEMAP_FACE_POSITIVE_Y:
		*Out = VEC3::POSITIVE_Y;
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_Y:
		*Out = VEC3::NEGATIVE_Y;
		break;
	case D3DCUBEMAP_FACE_POSITIVE_Z:
		*Out = VEC3::POSITIVE_Z;
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_Z:
		*Out = VEC3::NEGATIVE_Z;
		break;
	}
}

void GetCubeMapUpDir(VEC3 *Out, D3DCUBEMAP_FACES FaceType)
{
	switch(FaceType)
	{
	case D3DCUBEMAP_FACE_POSITIVE_X:
		*Out = VEC3::POSITIVE_Y;
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_X:
		*Out = VEC3::POSITIVE_Y;
		break;
	case D3DCUBEMAP_FACE_POSITIVE_Y:
		*Out = VEC3::NEGATIVE_Z;
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_Y:
		*Out = VEC3::POSITIVE_Z;
		break;
	case D3DCUBEMAP_FACE_POSITIVE_Z:
		*Out = VEC3::POSITIVE_Y;
		break;
	case D3DCUBEMAP_FACE_NEGATIVE_Z:
		*Out = VEC3::POSITIVE_Y;
		break;
	}
}

const DWORD VERTEX_X     ::FVF = D3DFVF_XYZ;
const DWORD VERTEX_XN    ::FVF = D3DFVF_XYZ | D3DFVF_NORMAL;
const DWORD VERTEX_XD    ::FVF = D3DFVF_XYZ                 | D3DFVF_DIFFUSE;
const DWORD VERTEX_XND   ::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE;
const DWORD VERTEX_X2    ::FVF = D3DFVF_XYZ                                  | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);
const DWORD VERTEX_XN2   ::FVF = D3DFVF_XYZ | D3DFVF_NORMAL                  | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);
const DWORD VERTEX_XD2   ::FVF = D3DFVF_XYZ                 | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);
const DWORD VERTEX_XND2  ::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);
const DWORD VERTEX_XN33  ::FVF = D3DFVF_XYZ | D3DFVF_NORMAL                  | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEXCOORDSIZE3(1);
const DWORD VERTEX_XN233 ::FVF = D3DFVF_XYZ | D3DFVF_NORMAL                  | D3DFVF_TEX3 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE3(1) | D3DFVF_TEXCOORDSIZE3(2);
const DWORD VERTEX_XND33 ::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEXCOORDSIZE3(1);
const DWORD VERTEX_XND233::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX3 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE3(1) | D3DFVF_TEXCOORDSIZE3(2);

const DWORD VERTEX_R  ::FVF = D3DFVF_XYZRHW;
const DWORD VERTEX_RD ::FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;
const DWORD VERTEX_R2 ::FVF = D3DFVF_XYZRHW                  | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);
const DWORD VERTEX_RD2::FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Konwersje typów Direct3D z ³añcuchami

const char * const D3DFMT_PREFIX = "D3DFMT_";
const char * const D3DMULTISAMPLE_TYPE_PREFIX = "D3DMULTISAMPLE_";
const char * const D3DSWAPEFFECT_PREFIX = "D3DSWAPEFFECT_";
const char * const D3DPRESENT_INTERVAL_PREFIX = "D3DPRESENT_INTERVAL_";
const char * const D3DADAPTER_PREFIX = "D3DADAPTER_";
const char * const D3DDEVTYPE_PREFIX = "D3DDEVTYPE_";
const char * const D3DERR_PREFIX = "D3DERR_";
const char * const D3DUSAGE_PREFIX = "D3DUSAGE_";
const char * const D3DPOOL_PREFIX = "D3DPOOL_";

// Dodaje flagê bitow¹ do ³añcucha konstruuj¹c ³añcuch typu "FLAGA1 | FLAGA2 | FLAGA3".
// Funkcja wewnêtrzna.
void AddFlag(string *InOut, const string &Prefix, bool Short, const string &Flag)
{
	if (!InOut->empty())
		InOut->append(" | ");
	if (!Short)
		InOut->append(Prefix);
	InOut->append(Flag);
}

void D3dfmtToStr(string *Out, D3DFORMAT Fmt, bool Short)
{
	if (Short)
		Out->clear();
	else
		*Out = D3DFMT_PREFIX;

	switch (Fmt)
	{
	case D3DFMT_D16_LOCKABLE: *Out += "D16_LOCKABLE"; break;
	case D3DFMT_D32: *Out += "D32"; break;
	case D3DFMT_D15S1: *Out += "D15S1"; break;
	case D3DFMT_D24S8: *Out += "D24S8"; break;
	case D3DFMT_D24X8: *Out += "D24X8"; break;
	case D3DFMT_D24X4S4: *Out += "D24X4S4"; break;
	case D3DFMT_D32F_LOCKABLE: *Out += "D32F_LOCKABLE"; break;
	case D3DFMT_D24FS8: *Out += "D24FS8"; break;
	case D3DFMT_D16: *Out += "D16"; break;
	case D3DFMT_VERTEXDATA: *Out += "VERTEXDATA"; break;
	case D3DFMT_INDEX16: *Out += "INDEX16"; break;
	case D3DFMT_INDEX32: *Out += "INDEX32"; break;
	case D3DFMT_DXT1: *Out += "DXT1"; break;
	case D3DFMT_DXT2: *Out += "DXT2"; break;
	case D3DFMT_DXT3: *Out += "DXT3"; break;
	case D3DFMT_DXT4: *Out += "DXT4"; break;
	case D3DFMT_DXT5: *Out += "DXT5"; break;
	case D3DFMT_R16F: *Out += "R16F"; break;
	case D3DFMT_G16R16F: *Out += "G16R16F"; break;
	case D3DFMT_A16B16G16R16F: *Out += "A16B16G16R16F"; break;
	case D3DFMT_MULTI2_ARGB8: *Out += "MULTI2_ARGB8"; break;
	case D3DFMT_G8R8_G8B8: *Out += "G8R8_G8B8"; break;
	case D3DFMT_R8G8_B8G8: *Out += "R8G8_B8G8"; break;
	case D3DFMT_UYVY: *Out += "UYVY"; break;
	case D3DFMT_YUY2: *Out += "YUY2"; break;
	case D3DFMT_R32F: *Out += "R32F"; break;
	case D3DFMT_G32R32F: *Out += "G32R32F"; break;
	case D3DFMT_A32B32G32R32F: *Out += "A32B32G32R32F"; break;
	case D3DFMT_L6V5U5: *Out += "L6V5U5"; break;
	case D3DFMT_X8L8V8U8: *Out += "X8L8V8U8"; break;
	case D3DFMT_A2W10V10U10: *Out += "A2W10V10U10"; break;
	case D3DFMT_V8U8: *Out += "V8U8"; break;
	case D3DFMT_Q8W8V8U8: *Out += "Q8W8V8U8"; break;
	case D3DFMT_V16U16: *Out += "V16U16"; break;
	case D3DFMT_Q16W16V16U16: *Out += "Q16W16V16U16"; break;
	case D3DFMT_CxV8U8: *Out += "CxV8U8"; break;
	case D3DFMT_R8G8B8: *Out += "R8G8B8"; break;
	case D3DFMT_A8R8G8B8: *Out += "A8R8G8B8"; break;
	case D3DFMT_X8R8G8B8: *Out += "X8R8G8B8"; break;
	case D3DFMT_R5G6B5: *Out += "R5G6B5"; break;
	case D3DFMT_X1R5G5B5: *Out += "X1R5G5B5"; break;
	case D3DFMT_A1R5G5B5: *Out += "A1R5G5B5"; break;
	case D3DFMT_A4R4G4B4: *Out += "A4R4G4B4"; break;
	case D3DFMT_R3G3B2: *Out += "R3G3B2"; break;
	case D3DFMT_A8: *Out += "A8"; break;
	case D3DFMT_A8R3G3B2: *Out += "A8R3G3B2"; break;
	case D3DFMT_X4R4G4B4: *Out += "X4R4G4B4"; break;
	case D3DFMT_A2B10G10R10: *Out += "A2B10G10R10"; break;
	case D3DFMT_A8B8G8R8: *Out += "A8B8G8R8"; break;
	case D3DFMT_X8B8G8R8: *Out += "X8B8G8R8"; break;
	case D3DFMT_G16R16: *Out += "G16R16"; break;
	case D3DFMT_A2R10G10B10: *Out += "A2R10G10B10"; break;
	case D3DFMT_A16B16G16R16: *Out += "A16B16G16R16"; break;
	case D3DFMT_A8P8: *Out += "A8P8"; break;
	case D3DFMT_P8: *Out += "P8"; break;
	case D3DFMT_L8: *Out += "L8"; break;
	case D3DFMT_L16: *Out += "L16"; break;
	case D3DFMT_A8L8: *Out += "A8L8"; break;
	case D3DFMT_A4L4: *Out += "A4L4"; break;
	case D3DFMT_UNKNOWN: *Out += "UNKNOWN"; break;
	default:
		UintToStr(Out, static_cast<uint4>(Fmt));
	}
}

void D3dmultisampletypeToStr(string *Out, D3DMULTISAMPLE_TYPE MultiSampleType, bool Short)
{
	if (Short)
		Out->clear();
	else
		*Out = D3DMULTISAMPLE_TYPE_PREFIX;

	switch (MultiSampleType)
	{
	case D3DMULTISAMPLE_NONE: *Out += "NONE"; break;
	case D3DMULTISAMPLE_NONMASKABLE: *Out += "NONMASKABLE"; break;
	case D3DMULTISAMPLE_2_SAMPLES: *Out += "2_SAMPLES"; break;
	case D3DMULTISAMPLE_3_SAMPLES: *Out += "3_SAMPLES"; break;
	case D3DMULTISAMPLE_4_SAMPLES: *Out += "4_SAMPLES"; break;
	case D3DMULTISAMPLE_5_SAMPLES: *Out += "5_SAMPLES"; break;
	case D3DMULTISAMPLE_6_SAMPLES: *Out += "6_SAMPLES"; break;
	case D3DMULTISAMPLE_7_SAMPLES: *Out += "7_SAMPLES"; break;
	case D3DMULTISAMPLE_8_SAMPLES: *Out += "8_SAMPLES"; break;
	case D3DMULTISAMPLE_9_SAMPLES: *Out += "9_SAMPLES"; break;
	case D3DMULTISAMPLE_10_SAMPLES: *Out += "10_SAMPLES"; break;
	case D3DMULTISAMPLE_11_SAMPLES: *Out += "11_SAMPLES"; break;
	case D3DMULTISAMPLE_12_SAMPLES: *Out += "12_SAMPLES"; break;
	case D3DMULTISAMPLE_13_SAMPLES: *Out += "13_SAMPLES"; break;
	case D3DMULTISAMPLE_14_SAMPLES: *Out += "14_SAMPLES"; break;
	case D3DMULTISAMPLE_15_SAMPLES: *Out += "15_SAMPLES"; break;
	case D3DMULTISAMPLE_16_SAMPLES: *Out += "16_SAMPLES"; break;
	case D3DMULTISAMPLE_FORCE_DWORD: *Out = "FORCE_DWORD"; break;
	default:
		UintToStr(Out, static_cast<uint4>(MultiSampleType));
	}
}

void D3dswapeffectToStr(string *Out, D3DSWAPEFFECT SwapEffect, bool Short)
{
	if (Short)
		Out->clear();
	else
		*Out = D3DSWAPEFFECT_PREFIX;

	switch (SwapEffect)
	{
	case D3DSWAPEFFECT_DISCARD: *Out += "DISCARD"; break;
	case D3DSWAPEFFECT_FLIP: *Out += "FLIP"; break;
	case D3DSWAPEFFECT_COPY: *Out += "COPY"; break;
	case D3DSWAPEFFECT_FORCE_DWORD: *Out += "FORCE_DWORD"; break;
	default:
		UintToStr(Out, static_cast<uint4>(SwapEffect));
	}
}

void D3dpresentationinvervalToStr(string *Out, uint4 PresentationInverval, bool Short)
{
	if (Short)
		Out->clear();
	else
		*Out = D3DPRESENT_INTERVAL_PREFIX;

	switch (PresentationInverval)
	{
	case D3DPRESENT_INTERVAL_DEFAULT: *Out += "DEFAULT"; break;
	case D3DPRESENT_INTERVAL_ONE: *Out += "ONE"; break;
	case D3DPRESENT_INTERVAL_TWO: *Out += "TWO"; break;
	case D3DPRESENT_INTERVAL_THREE: *Out += "THREE"; break;
	case D3DPRESENT_INTERVAL_FOUR: *Out += "FOUR"; break;
	case D3DPRESENT_INTERVAL_IMMEDIATE: *Out += "IMMEDIATE"; break;
	default:
		UintToStr(Out, PresentationInverval);
	}
}

void D3dadapterToStr(string *Out, uint4 Adapter, bool Short)
{
	if (Short)
		Out->clear();
	else
		*Out = D3DADAPTER_PREFIX;

	if (Adapter == D3DADAPTER_DEFAULT)
		*Out += "DEFAULT";
	else
		UintToStr(Out, Adapter);
}

void D3ddevtypeToStr(string *Out, D3DDEVTYPE DevType, bool Short)
{
	if (Short)
		Out->clear();
	else
		*Out = D3DDEVTYPE_PREFIX;

	switch (DevType)
	{
	case D3DDEVTYPE_HAL: *Out += "HAL"; break;
	case D3DDEVTYPE_NULLREF: *Out += "NULLREF"; break;
	case D3DDEVTYPE_REF: *Out += "REF"; break;
	case D3DDEVTYPE_SW: *Out += "SW"; break;
	case D3DDEVTYPE_FORCE_DWORD: *Out += "FORCE_DWORD"; break;
	default:
		UintToStr(Out, static_cast<uint4>(DevType));
	}
}

void D3dvertexprocessingToStr(string *Out, uint4 VertexProcessing, bool Short)
{
	if (Short)
	{
		if (VertexProcessing == D3DCREATE_HARDWARE_VERTEXPROCESSING)
			*Out = "HARDWARE";
		else if (VertexProcessing == D3DCREATE_MIXED_VERTEXPROCESSING)
			*Out = "MIXED";
		else if (VertexProcessing == D3DCREATE_SOFTWARE_VERTEXPROCESSING)
			*Out = "SOFTWARE";
		else if (VertexProcessing == 0)
			*Out = "AUTO";
		else
			UintToStr(Out, VertexProcessing);
	}
	else
	{
		if (VertexProcessing == D3DCREATE_HARDWARE_VERTEXPROCESSING)
			*Out = "D3DCREATE_HARDWARE_VERTEXPROCESSING";
		else if (VertexProcessing == D3DCREATE_MIXED_VERTEXPROCESSING)
			*Out = "D3DCREATE_MIXED_VERTEXPROCESSING";
		else if (VertexProcessing == D3DCREATE_SOFTWARE_VERTEXPROCESSING)
			*Out = "D3DCREATE_SOFTWARE_VERTEXPROCESSING";
		else if (VertexProcessing == 0)
			*Out = "AUTO";
		else
			UintToStr(Out, VertexProcessing);
	}
}

void D3dValidateDeviceResultToStr(string *Out, HRESULT Result, bool Short)
{
	if (Short)
		Out->clear();
	else
		*Out = D3DERR_PREFIX;

	switch (Result)
	{
	case D3DERR_CONFLICTINGRENDERSTATE:
		*Out += "CONFLICTINGRENDERSTATE"; break;
	case D3DERR_CONFLICTINGTEXTUREFILTER:
		*Out += "CONFLICTINGTEXTUREFILTER"; break;
	case D3DERR_DEVICELOST:
		*Out += "DEVICELOST"; break;
	case D3DERR_DRIVERINTERNALERROR:
		*Out += "DRIVERINTERNALERROR"; break;
	case D3DERR_TOOMANYOPERATIONS:
		*Out += "TOOMANYOPERATIONS"; break;
	case D3DERR_UNSUPPORTEDALPHAARG:
		*Out += "UNSUPPORTEDALPHAARG"; break;
	case D3DERR_UNSUPPORTEDALPHAOPERATION:
		*Out += "UNSUPPORTEDALPHAOPERATION"; break;
	case D3DERR_UNSUPPORTEDCOLORARG:
		*Out += "UNSUPPORTEDCOLORARG"; break;
	case D3DERR_UNSUPPORTEDCOLOROPERATION:
		*Out += "UNSUPPORTEDCOLOROPERATION"; break;
	case D3DERR_UNSUPPORTEDFACTORVALUE:
		*Out += "UNSUPPORTEDFACTORVALUE"; break;
	case D3DERR_UNSUPPORTEDTEXTUREFILTER:
		*Out += "UNSUPPORTEDTEXTUREFILTER"; break;
	case D3DERR_WRONGTEXTUREFORMAT:
		*Out += "WRONGTEXTUREFORMAT"; break;
	default:
		*Out = "Nieznany";
	}
}

void D3dUsageToStr(string *Out, uint4 Usage, bool Short)
{
	if (Usage == 0)
		*Out = "0";
	else
	{
		string Prefix = D3DUSAGE_PREFIX;

		if (Usage & D3DUSAGE_AUTOGENMIPMAP)
			AddFlag(Out, Prefix, Short, "AUTOGENMIPMAP");
		if (Usage & D3DUSAGE_DEPTHSTENCIL)
			AddFlag(Out, Prefix, Short, "DEPTHSTENCIL");
		if (Usage & D3DUSAGE_DMAP)
			AddFlag(Out, Prefix, Short, "DMAP");
		if (Usage & D3DUSAGE_DONOTCLIP)
			AddFlag(Out, Prefix, Short, "DONOTCLIP");
		if (Usage & D3DUSAGE_DYNAMIC)
			AddFlag(Out, Prefix, Short, "DYNAMIC");
		if (Usage & D3DUSAGE_NPATCHES)
			AddFlag(Out, Prefix, Short, "NPATCHES");
		if (Usage & D3DUSAGE_POINTS)
			AddFlag(Out, Prefix, Short, "POINTS");
		if (Usage & D3DUSAGE_RENDERTARGET)
			AddFlag(Out, Prefix, Short, "RENDERTARGET");
		if (Usage & D3DUSAGE_RTPATCHES)
			AddFlag(Out, Prefix, Short, "RTPATCHES");
		if (Usage & D3DUSAGE_SOFTWAREPROCESSING)
			AddFlag(Out, Prefix, Short, "SOFTWAREPROCESSING");
		if (Usage & D3DUSAGE_WRITEONLY)
			AddFlag(Out, Prefix, Short, "WRITEONLY");
	}
}

void D3dPoolToStr(string *Out, D3DPOOL Pool, bool Short)
{
	if (Short)
		Out->clear();
	else
		*Out = D3DPOOL_PREFIX;

	switch (Pool)
	{
	case D3DPOOL_DEFAULT:
		*Out += "DEFAULT"; break;
	case D3DPOOL_MANAGED:
		*Out += "MANAGED"; break;
	case D3DPOOL_SYSTEMMEM:
		*Out += "SYSTEMMEM"; break;
	case D3DPOOL_SCRATCH:
		*Out += "SCRATCH"; break;
	default:
		UintToStr(Out, (uint4)Pool);
	}
}

bool StrToD3dfmt(D3DFORMAT *Out, const string &s)
{
	string tmp;
	if (StrBegins(s, D3DFMT_PREFIX))
		tmp = s.substr(strlen(D3DFMT_PREFIX));
	else
		tmp = s;

	if (tmp == "A2R10G10B10") *Out = D3DFMT_A2R10G10B10;
	else if (tmp == "A8R8G8B8") *Out = D3DFMT_A8R8G8B8;
	else if (tmp == "X8R8G8B8") *Out = D3DFMT_X8R8G8B8;
	else if (tmp == "A1R5G5B5") *Out = D3DFMT_A1R5G5B5;
	else if (tmp == "X1R5G5B5") *Out = D3DFMT_X1R5G5B5;
	else if (tmp == "R5G6B5") *Out = D3DFMT_R5G6B5;
	else if (tmp == "D16_LOCKABLE") *Out = D3DFMT_D16_LOCKABLE;
	else if (tmp == "D32") *Out = D3DFMT_D32;
	else if (tmp == "D15S1") *Out = D3DFMT_D15S1;
	else if (tmp == "D24S8") *Out = D3DFMT_D24S8;
	else if (tmp == "D24X8") *Out = D3DFMT_D24X8;
	else if (tmp == "D24X4S4") *Out = D3DFMT_D24X4S4;
	else if (tmp == "D32F_LOCKABLE") *Out = D3DFMT_D32F_LOCKABLE;
	else if (tmp == "D24FS8") *Out = D3DFMT_D24FS8;
	else if (tmp == "D16") *Out = D3DFMT_D16;
	else if (tmp == "VERTEXDATA") *Out = D3DFMT_VERTEXDATA;
	else if (tmp == "INDEX16") *Out = D3DFMT_INDEX16;
	else if (tmp == "INDEX32") *Out = D3DFMT_INDEX32;
	else if (tmp == "DXT1") *Out = D3DFMT_DXT1;
	else if (tmp == "DXT2") *Out = D3DFMT_DXT2;
	else if (tmp == "DXT3") *Out = D3DFMT_DXT3;
	else if (tmp == "DXT4") *Out = D3DFMT_DXT4;
	else if (tmp == "DXT5") *Out = D3DFMT_DXT5;
	else if (tmp == "R16F") *Out = D3DFMT_R16F;
	else if (tmp == "G16R16F") *Out = D3DFMT_G16R16F;
	else if (tmp == "A16B16G16R16F") *Out = D3DFMT_A16B16G16R16F;
	else if (tmp == "MULTI2_ARGB8") *Out = D3DFMT_MULTI2_ARGB8;
	else if (tmp == "G8R8_G8B8") *Out = D3DFMT_G8R8_G8B8;
	else if (tmp == "R8G8_B8G8") *Out = D3DFMT_R8G8_B8G8;
	else if (tmp == "UYVY") *Out = D3DFMT_UYVY;
	else if (tmp == "YUY2") *Out = D3DFMT_YUY2;
	else if (tmp == "R32F") *Out = D3DFMT_R32F;
	else if (tmp == "G32R32F") *Out = D3DFMT_G32R32F;
	else if (tmp == "A32B32G32R32F") *Out = D3DFMT_A32B32G32R32F;
	else if (tmp == "L6V5U5") *Out = D3DFMT_L6V5U5;
	else if (tmp == "X8L8V8U8") *Out = D3DFMT_X8L8V8U8;
	else if (tmp == "A2W10V10U10") *Out = D3DFMT_A2W10V10U10;
	else if (tmp == "V8U8") *Out = D3DFMT_V8U8;
	else if (tmp == "Q8W8V8U8") *Out = D3DFMT_Q8W8V8U8;
	else if (tmp == "V16U16") *Out = D3DFMT_V16U16;
	else if (tmp == "Q16W16V16U16") *Out = D3DFMT_Q16W16V16U16;
	else if (tmp == "CxV8U8") *Out = D3DFMT_CxV8U8;
	else if (tmp == "R8G8B8") *Out = D3DFMT_R8G8B8;
	else if (tmp == "A8R8G8B8") *Out = D3DFMT_A8R8G8B8;
	else if (tmp == "X8R8G8B8") *Out = D3DFMT_X8R8G8B8;
	else if (tmp == "R5G6B5") *Out = D3DFMT_R5G6B5;
	else if (tmp == "X1R5G5B5") *Out = D3DFMT_X1R5G5B5;
	else if (tmp == "A1R5G5B5") *Out = D3DFMT_A1R5G5B5;
	else if (tmp == "A4R4G4B4") *Out = D3DFMT_A4R4G4B4;
	else if (tmp == "R3G3B2") *Out = D3DFMT_R3G3B2;
	else if (tmp == "A8") *Out = D3DFMT_A8;
	else if (tmp == "A8R3G3B2") *Out = D3DFMT_A8R3G3B2;
	else if (tmp == "X4R4G4B4") *Out = D3DFMT_X4R4G4B4;
	else if (tmp == "A2B10G10R10") *Out = D3DFMT_A2B10G10R10;
	else if (tmp == "A8B8G8R8") *Out = D3DFMT_A8B8G8R8;
	else if (tmp == "X8B8G8R8") *Out = D3DFMT_X8B8G8R8;
	else if (tmp == "G16R16") *Out = D3DFMT_G16R16;
	else if (tmp == "A2R10G10B10") *Out = D3DFMT_A2R10G10B10;
	else if (tmp == "A16B16G16R16") *Out = D3DFMT_A16B16G16R16;
	else if (tmp == "A8P8") *Out = D3DFMT_A8P8;
	else if (tmp == "P8") *Out = D3DFMT_P8;
	else if (tmp == "L8") *Out = D3DFMT_L8;
	else if (tmp == "L16") *Out = D3DFMT_L16;
	else if (tmp == "A8L8") *Out = D3DFMT_A8L8;
	else if (tmp == "A4L4") *Out = D3DFMT_A4L4;
	else if (tmp == "UNKNOWN") *Out = D3DFMT_UNKNOWN;
	else
		return StrToUint(reinterpret_cast<uint4*>(Out), s) == 0;

	return true;
}

bool StrToD3dmultisampletype(D3DMULTISAMPLE_TYPE *Out, const string &s)
{
	string tmp;
	if (StrBegins(s, D3DMULTISAMPLE_TYPE_PREFIX))
		tmp = s.substr(strlen(D3DMULTISAMPLE_TYPE_PREFIX));
	else
		tmp = s;

	if (tmp == "NONE") *Out = D3DMULTISAMPLE_NONE;
	else if (tmp == "NONMASKABLE") *Out = D3DMULTISAMPLE_NONMASKABLE;
	else if (tmp == "2_SAMPLES") *Out = D3DMULTISAMPLE_2_SAMPLES;
	else if (tmp == "3_SAMPLES") *Out = D3DMULTISAMPLE_3_SAMPLES;
	else if (tmp == "4_SAMPLES") *Out = D3DMULTISAMPLE_4_SAMPLES;
	else if (tmp == "5_SAMPLES") *Out = D3DMULTISAMPLE_5_SAMPLES;
	else if (tmp == "6_SAMPLES") *Out = D3DMULTISAMPLE_6_SAMPLES;
	else if (tmp == "7_SAMPLES") *Out = D3DMULTISAMPLE_7_SAMPLES;
	else if (tmp == "8_SAMPLES") *Out = D3DMULTISAMPLE_8_SAMPLES;
	else if (tmp == "9_SAMPLES") *Out = D3DMULTISAMPLE_9_SAMPLES;
	else if (tmp == "10_SAMPLES") *Out = D3DMULTISAMPLE_10_SAMPLES;
	else if (tmp == "11_SAMPLES") *Out = D3DMULTISAMPLE_11_SAMPLES;
	else if (tmp == "12_SAMPLES") *Out = D3DMULTISAMPLE_12_SAMPLES;
	else if (tmp == "13_SAMPLES") *Out = D3DMULTISAMPLE_13_SAMPLES;
	else if (tmp == "14_SAMPLES") *Out = D3DMULTISAMPLE_14_SAMPLES;
	else if (tmp == "15_SAMPLES") *Out = D3DMULTISAMPLE_15_SAMPLES;
	else if (tmp == "16_SAMPLES") *Out = D3DMULTISAMPLE_16_SAMPLES;
	else if (tmp == "FORCE_DWORD") *Out = D3DMULTISAMPLE_FORCE_DWORD;
	else
		return StrToUint(reinterpret_cast<uint4*>(Out), s) == 0;

	return true;
}

bool StrToD3dswapeffect(D3DSWAPEFFECT *Out, const string &s)
{
	string tmp;
	if (StrBegins(s, D3DSWAPEFFECT_PREFIX))
		tmp = s.substr(strlen(D3DSWAPEFFECT_PREFIX));
	else
		tmp = s;

	if (tmp == "DISCARD") *Out = D3DSWAPEFFECT_DISCARD;
	else if (tmp == "FLIP") *Out = D3DSWAPEFFECT_FLIP;
	else if (tmp == "COPY") *Out = D3DSWAPEFFECT_COPY;
	else if (tmp == "FORCE_DWORD") *Out = D3DSWAPEFFECT_FORCE_DWORD;
	else
		return StrToUint(reinterpret_cast<uint4*>(Out), s) == 0;

	return true;
}

bool StrToD3dpresentationinterval(uint4 *Out, const string &s)
{
	string tmp;
	if (StrBegins(s, D3DPRESENT_INTERVAL_PREFIX))
		tmp = s.substr(strlen(D3DPRESENT_INTERVAL_PREFIX));
	else
		tmp = s;

	if (tmp == "DEFAULT") *Out = D3DPRESENT_INTERVAL_DEFAULT;
	else if (tmp == "ONE") *Out = D3DPRESENT_INTERVAL_ONE;
	else if (tmp == "TWO") *Out = D3DPRESENT_INTERVAL_TWO;
	else if (tmp == "THREE") *Out = D3DPRESENT_INTERVAL_THREE;
	else if (tmp == "FOUR") *Out = D3DPRESENT_INTERVAL_FOUR;
	else if (tmp == "IMMEDIATE") *Out = D3DPRESENT_INTERVAL_IMMEDIATE;
	else
		return StrToUint(Out, s) == 0;

	return true;
}

bool StrToD3dadapter(uint4 *Out, const string &s)
{
	if (s == "DEFAULT" || s == string(D3DADAPTER_PREFIX) + "DEFAULT")
		*Out =  D3DADAPTER_DEFAULT;
	else
		return StrToUint(reinterpret_cast<uint4*>(Out), s) == 0;

	return true;
}

bool StrToD3ddevtype(D3DDEVTYPE *Out, const string &s)
{
	string tmp;
	if (StrBegins(s, D3DDEVTYPE_PREFIX))
		tmp = s.substr(strlen(D3DDEVTYPE_PREFIX));
	else
		tmp = s;

	if (tmp == "HAL") *Out = D3DDEVTYPE_HAL;
	else if (tmp == "NULLREF") *Out = D3DDEVTYPE_NULLREF;
	else if (tmp == "REF") *Out = D3DDEVTYPE_REF;
	else if (tmp == "SW") *Out = D3DDEVTYPE_SW;
	else if (tmp == "FORCE_DWORD") *Out = D3DDEVTYPE_FORCE_DWORD;
	else
		return StrToUint(reinterpret_cast<uint4*>(Out), s) == 0;

	return true;
}

bool StrToD3dvertexprocessing(uint4 *Out, const string &s)
{
	if (s == "HARDWARE" || s == "D3DCREATE_HARDWARE_VERTEXPROCESSING")
		*Out = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else if (s == "MIXED" || s == "D3DCREATE_MIXED_VERTEXPROCESSING")
		*Out = D3DCREATE_MIXED_VERTEXPROCESSING;
	else if (s == "SOFTWARE" || s == "D3DCREATE_SOFTWARE_VERTEXPROCESSING")
		*Out = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	else if (s == "AUTO")
		*Out = 0;
	else
		return StrToUint(reinterpret_cast<uint4*>(Out), s) == 0;

	return true;
}

bool StrToD3dUsage(uint4 *Out, const string &s)
{
	*Out = 0;

	if (s == "0") return true;

	const string Delimiter = "|";
	const string Prefix = D3DUSAGE_PREFIX;

	string Tmp;
	size_t Pos = 0;
	while (Split(s, Delimiter, &Tmp, &Pos))
	{
		Trim(&Tmp);
		if (StrBegins(Tmp, Prefix))
			Tmp = Tmp.substr(Prefix.length());

		if (Tmp == "AUTOGENMIPMAP")
			*Out |= D3DUSAGE_AUTOGENMIPMAP;
		else if (Tmp == "DEPTHSTENCIL")
			*Out |= D3DUSAGE_DEPTHSTENCIL;
		else if (Tmp == "DMAP")
			*Out |= D3DUSAGE_DMAP;
		else if (Tmp == "DONOTCLIP")
			*Out |= D3DUSAGE_DONOTCLIP;
		else if (Tmp == "DYNAMIC")
			*Out |= D3DUSAGE_DYNAMIC;
		else if (Tmp == "NPATCHES")
			*Out |= D3DUSAGE_NPATCHES;
		else if (Tmp == "POINTS")
			*Out |= D3DUSAGE_POINTS;
		else if (Tmp == "RENDERTARGET")
			*Out |= D3DUSAGE_RENDERTARGET;
		else if (Tmp == "RTPATCHES")
			*Out |= D3DUSAGE_RTPATCHES;
		else if (Tmp == "SOFTWAREPROCESSING")
			*Out |= D3DUSAGE_SOFTWAREPROCESSING;
		else if (Tmp == "WRITEONLY")
			*Out |= D3DUSAGE_WRITEONLY;
		else
			return false;
	}

	return true;
}

bool StrToD3dPool(D3DPOOL *Out, const string &s)
{
	string tmp;
	if (StrBegins(s, D3DPOOL_PREFIX))
		tmp = s.substr(strlen(D3DPOOL_PREFIX));
	else
		tmp = s;

	if (tmp == "DEFAULT") *Out = D3DPOOL_DEFAULT;
	else if (tmp == "MANAGED") *Out = D3DPOOL_MANAGED;
	else if (tmp == "SYSTEMMEM") *Out = D3DPOOL_SYSTEMMEM;
	else if (tmp == "SCRATCH") *Out = D3DPOOL_SCRATCH;
	else
		return StrToUint(reinterpret_cast<uint4*>(Out), s) == 0;

	return true;
}
