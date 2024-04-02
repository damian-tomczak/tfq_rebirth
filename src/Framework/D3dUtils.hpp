/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef D3D_UTILS_H_
#define D3D_UTILS_H_

// Pomocnicze struktury wierzcho³ka
/*
S¹ nastêpuj¹ce:
VERTEX_X,   VERTEX_X2
VERTEX_XN,  VERTEX_XN2,  VERTEX_XN33,  VERTEX_XN233
VERTEX_XD,  VERTEX_XD2
VERTEX_XDN, VERTEX_XDN2, VERTEX_XDN33, VERTEX_XDN233
VERTEX_R,   VERTEX_R2
VERTEX_RD,  VERTEX_RD2
*/
struct VERTEX_X {
	VEC3 Pos;
	VERTEX_X() { }
	VERTEX_X(const VEC3 &Pos) : Pos(Pos) { }
	static const DWORD FVF;
};

struct VERTEX_XN {
	VEC3 Pos;
	VEC3 Normal;
	VERTEX_XN() { }
	VERTEX_XN(const VEC3 &Pos, const VEC3 &Normal) : Pos(Pos), Normal(Normal) { }
	static const DWORD FVF;
};

struct VERTEX_XD {
	VEC3 Pos;
	COLOR Diffuse;
	VERTEX_XD() { }
	VERTEX_XD(const VEC3 &Pos, COLOR Diffuse) : Pos(Pos), Diffuse(Diffuse) { }
	static const DWORD FVF;
};

struct VERTEX_XND {
	VEC3 Pos;
	VEC3 Normal;
	COLOR Diffuse;
	VERTEX_XND() { }
	VERTEX_XND(const VEC3 &Pos, const VEC3 &Normal, COLOR Diffuse) : Pos(Pos), Normal(Normal), Diffuse(Diffuse) { }
	static const DWORD FVF;
};


struct VERTEX_X2 {
	VEC3 Pos;
	VEC2 Tex;
	VERTEX_X2() { }
	VERTEX_X2(const VEC3 &Pos, const VEC2 &Tex) : Pos(Pos), Tex(Tex) { }
	static const DWORD FVF;
};

struct VERTEX_XN2 {
	VEC3 Pos;
	VEC3 Normal;
	VEC2 Tex;
	VERTEX_XN2() { }
	VERTEX_XN2(const VEC3 &Pos, const VEC3 &Normal, const VEC2 &Tex) : Pos(Pos), Normal(Normal), Tex(Tex) { }
	static const DWORD FVF;
};

struct VERTEX_XD2 {
	VEC3 Pos;
	COLOR Diffuse;
	VEC2 Tex;
	VERTEX_XD2() { }
	VERTEX_XD2(const VEC3 &Pos, COLOR Diffuse, const VEC2 &Tex) : Pos(Pos), Diffuse(Diffuse), Tex(Tex) { }
	static const DWORD FVF;
};

struct VERTEX_XND2 {
	VEC3 Pos;
	VEC3 Normal;
	COLOR Diffuse;
	VEC2 Tex;
	VERTEX_XND2() { }
	VERTEX_XND2(const VEC3 &Pos, const VEC3 &Normal, COLOR Diffuse, const VEC2 &Tex) : Pos(Pos), Normal(Normal), Diffuse(Diffuse), Tex(Tex) { }
	static const DWORD FVF;
};

struct VERTEX_XN33 {
	VEC3 Pos;
	VEC3 Normal;
	VEC3 Tangent;
	VEC3 Binormal;
	VERTEX_XN33() { }
	VERTEX_XN33(const VEC3 &Pos, const VEC3 &Normal, const VEC3 &Tangent, const VEC3 &Binormal) : Pos(Pos), Normal(Normal), Tangent(Tangent), Binormal(Binormal) { }
	static const DWORD FVF;
};

struct VERTEX_XN233 {
	VEC3 Pos;
	VEC3 Normal;
	VEC2 Tex;
	VEC3 Tangent;
	VEC3 Binormal;
	VERTEX_XN233() { }
	VERTEX_XN233(const VEC3 &Pos, const VEC3 &Normal, const VEC2 &Tex, const VEC3 &Tangent, const VEC3 &Binormal) : Pos(Pos), Normal(Normal), Tex(Tex), Tangent(Tangent), Binormal(Binormal) { }
	static const DWORD FVF;
};


////////////
struct VERTEX_XND33 {
	VEC3 Pos;
	VEC3 Normal;
	COLOR Diffuse;
	VEC3 Tangent;
	VEC3 Binormal;
	VERTEX_XND33() { }
	VERTEX_XND33(const VEC3 &Pos, const VEC3 &Normal, COLOR Diffuse, const VEC3 &Tangent, const VEC3 &Binormal) : Pos(Pos), Normal(Normal), Diffuse(Diffuse), Tangent(Tangent), Binormal(Binormal) { }
	static const DWORD FVF;
};

struct VERTEX_XND233 {
	VEC3 Pos;
	VEC3 Normal;
	COLOR Diffuse;
	VEC2 Tex;
	VEC3 Tangent;
	VEC3 Binormal;
	VERTEX_XND233() { }
	VERTEX_XND233(const VEC3 &Pos, const VEC3 &Normal, COLOR Diffuse, const VEC2 &Tex, const VEC3 &Tangent, const VEC3 &Binormal) : Pos(Pos), Normal(Normal), Diffuse(Diffuse), Tex(Tex), Tangent(Tangent), Binormal(Binormal) { }
	static const DWORD FVF;
};



struct VERTEX_R {
	VEC4 Pos;
	VERTEX_R() { }
	VERTEX_R(const VEC4 &Pos) : Pos(Pos) { }
	static const DWORD FVF;
};

struct VERTEX_RD {
	VEC4 Pos;
	COLOR Diffuse;
	VERTEX_RD() { }
	VERTEX_RD(const VEC4 &Pos, COLOR Diffuse) : Pos(Pos), Diffuse(Diffuse) { }
	static const DWORD FVF;
};

struct VERTEX_R2 {
	VEC4 Pos;
	VEC2 Tex;
	VERTEX_R2() { }
	VERTEX_R2(const VEC4 &Pos, const VEC2 &Tex) : Pos(Pos), Tex(Tex) { }
	static const DWORD FVF;
};

struct VERTEX_RD2 {
	VEC4 Pos;
	COLOR Diffuse;
	VEC2 Tex;
	VERTEX_RD2() { }
	VERTEX_RD2(const VEC4 &Pos, COLOR Diffuse, const VEC2 &Tex) : Pos(Pos), Diffuse(Diffuse), Tex(Tex) { }
	static const DWORD FVF;
};

/*
- Obiekt tej klasy w konstruktorze ustawia podany render target i depth stencil, a w destruktorze przywraca stary.
- W razie b³êdów rzuca wyj¹tki.
- Mo¿na podaæ do konstruktora NULL jeœli nie zmieniamy render targeta albo depth stencila.
*/
class RenderTargetHelper
{
private:
	IDirect3DSurface9 *m_OldRenderTarget;
	IDirect3DSurface9 *m_OldDepthStencil;

public:
	RenderTargetHelper(IDirect3DSurface9 *NewRenderTarget, IDirect3DSurface9 *NewDepthStencil);
	~RenderTargetHelper();
};

/*
- Wrapper na interfejs ID3DXBuffer.
- Sam zwalnia bufor.
- W razie b³êdów rzuca wyj¹tek.
*/
class D3dxBufferWrapper
{
private:
	ID3DXBuffer *m_Buf;

public:
	// Tworzy nowy niezainicjalizowany bufor
	D3dxBufferWrapper(size_t NumBytes);
	// Tworzy nowy i wype³nia bufor
	D3dxBufferWrapper(size_t NumBytes, const void *Data);
	// Przejmuje na swoj¹ w³asnoœæ istniej¹cy bufor
	D3dxBufferWrapper(ID3DXBuffer *Buffer) : m_Buf(Buffer) { assert(Buffer != NULL); }
	// Zwalnia
	~D3dxBufferWrapper();

	// Zwraca bufor D3DX
	ID3DXBuffer * GetBuffer() { return m_Buf; }
	// Zwraca rozmiar
	size_t GetNumBytes() const { return m_Buf->GetBufferSize(); }
	// Zwraca wskaŸnik z danymi
	void * GetData() { return m_Buf->GetBufferPointer(); }
	const void * GetData() const { return m_Buf->GetBufferPointer(); }
	// Klonuje bufor tworz¹c i zwracaj¹c drugi o takim samym rozmiarze i zawartoœci
	D3dxBufferWrapper * Clone() const;
};

/*
- Podczas tworzenia obiektu blokuje VB podanej siatki.
- Jeœli nie uda siê zablokowaæ, zg³asza wyj¹tek.
- Podczas niszczenia obiektu odblokowuje VB tej siatki.
- Unlock mo¿na wywo³aæ ¿eby odblokowaæ wczeœniej ni¿ przy zniszczeniu obiektu, ale nie trzeba.
*/
class VertexBufferLock
{
private:
	ID3DXMesh *m_Mesh; // albo to jest niezerowe...
	IDirect3DVertexBuffer9 *m_VB; // ...albo to
	void *m_Data;

public:
	VertexBufferLock(ID3DXMesh *Mesh, DWORD Flags);
	VertexBufferLock(IDirect3DVertexBuffer9 *VB, DWORD Flags);
	VertexBufferLock(IDirect3DVertexBuffer9 *VB, DWORD Flags, UINT OffsetToLock, UINT SizeToLock);
	~VertexBufferLock();

	void * GetData() { return m_Data; }
	void Unlock();
};

/*
- Podczas tworzenia obiektu blokuje IB podanej siatki.
- Jeœli nie uda siê zablokowaæ, zg³asza wyj¹tek.
- Podczas niszczenia obiektu odblokowuje IB tej siatki.
- Unlock mo¿na wywo³aæ ¿eby odblokowaæ wczeœniej ni¿ przy zniszczeniu obiektu, ale nie trzeba.
*/
class IndexBufferLock
{
private:
	ID3DXMesh *m_Mesh; // albo to jest niezerowe...
	IDirect3DIndexBuffer9 *m_IB; // ...albo to
	void *m_Data;

public:
	IndexBufferLock(ID3DXMesh *Mesh, DWORD Flags);
	IndexBufferLock(IDirect3DIndexBuffer9 *IB, DWORD Flags);
	IndexBufferLock(IDirect3DIndexBuffer9 *IB, DWORD Flags, UINT OffsetToLock, UINT SizeToLock);
	~IndexBufferLock();

	void * GetData() { return m_Data; }
	void Unlock();
};

/*
Klasa wczytuje teksturê z u¿yciem D3DX do pamiêci do póli D3DPOOL_SCRATCH i
blokuje daj¹c dostêp do jej pikseli. Przez ca³y czas istnienia obiektu tekstura
pozostaje zablokowana. Jest zawsze w formacie A8R8G8B8/X8R8G8B8.
*/
class MemoryTexture
{
public:
	MemoryTexture(const string &FileName);
	~MemoryTexture();

	uint GetWidth() { return m_Width; }
	uint GetHeight() { return m_Height; }
	D3DFORMAT GetFormat() { return m_Format; }
	const COLOR * GetRowData(uint Row) { return (const COLOR*)( (const char*)m_LockedRect.pBits + (Row * m_LockedRect.Pitch) ); }

private:
	scoped_ptr<IDirect3DTexture9, ReleasePolicy> m_Texture;
	uint m_Width, m_Height;
	D3DLOCKED_RECT m_LockedRect;
	D3DFORMAT m_Format;
};


namespace FVF
{

void FvfToString_Short(string *Out, DWORD Fvf);
bool StringToFvf_Short(DWORD *Out, const string &s);

// Zwraca rozmiary i offsety w bajtach, ale zawsze s¹ wielokrotnoœci¹ 4.
size_t CalcNumComponents(DWORD Fvf);
size_t CalcVertexSize(DWORD Fvf);
void CalcComponentSizesAndOffsets(size_t *OutSizes, size_t *OutOffsets, DWORD Fvf);

} // namespace FVF


// Generuje macierz widoku dla podanej œcianki cube mapy
void CalcCubeMapViewMatrix(MATRIX *Out, const VEC3 &EyePos, D3DCUBEMAP_FACES FaceType);
// Zwraca wektor kierunku patrzenia kamery dla podanej œcianki cube mapy
void GetCubeMapForwardDir(VEC3 *Out, D3DCUBEMAP_FACES FaceType);
void GetCubeMapUpDir(VEC3 *Out, D3DCUBEMAP_FACES FaceType);

// ==== Konwersje typów Direct3D z ³añcuchami ====
/*
- Short=true - bez przedrostka typu "D3DUSAGE_".
- Funkcje StrToXxx akceptuj¹ zarówno z, jak i bez przedrostka.
- D3dvertexprocessing: Wartoœæ 0 oznacza "AUTO".
- D3dValidateDeviceResult: Dotyczy rezultatów funkcji IDirect3DDevice9::ValidateDevice.
- D3dUsage: £añcuch to po³¹czenie flag typu "D3DUSAGE_DYNAMIC | D3DUSAGE_POINTS". Brak flag to ³añcuch "0".
*/

void D3dfmtToStr(string *Out, D3DFORMAT Fmt, bool Short = true);
void D3dmultisampletypeToStr(string *Out, D3DMULTISAMPLE_TYPE MultisampleType, bool Short = true);
void D3dswapeffectToStr(string *Out, D3DSWAPEFFECT SwapEffect, bool Short = true);
void D3dpresentationinvervalToStr(string *Out, uint4 PresentationInverval, bool Short = true);
void D3dadapterToStr(string *Out, uint4 Adapter, bool Short = true);
void D3ddevtypeToStr(string *Out, D3DDEVTYPE DevType, bool Short = true);
void D3dvertexprocessingToStr(string *Out, uint4 VertexProcessing, bool Short = true);
void D3dValidateDeviceResultToStr(string *Out, HRESULT Result, bool Short = true);
void D3dUsageToStr(string *Out, uint4 Usage, bool Short = true);
void D3dPoolToStr(string *Out, D3DPOOL Pool, bool Short = true);

bool StrToD3dfmt(D3DFORMAT *Out, const string &s);
bool StrToD3dmultisampletype(D3DMULTISAMPLE_TYPE *Out, const string &s);
bool StrToD3dswapeffect(D3DSWAPEFFECT *Out, const string &s);
bool StrToD3dpresentationinterval(uint4 *Out, const string &s);
bool StrToD3dadapter(uint4 *Out, const string &s);
bool StrToD3ddevtype(D3DDEVTYPE *Out, const string &s);
bool StrToD3dvertexprocessing(uint4 *Out, const string &s);
bool StrToD3dUsage(uint4 *Out, const string &s);
bool StrToD3dPool(D3DPOOL *Out, const string &s);

#endif
