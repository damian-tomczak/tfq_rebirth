/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef RES_D3D_H_
#define RES_D3D_H_

#include "ResMngr.hpp"

const uint LOG_RESMNGR = 0x08;

namespace res
{

const uint4 EVENT_D3D_BASE = 1000;
const uint4 EVENT_D3D_LOST_DEVICE  = EVENT_D3D_BASE + 1;
const uint4 EVENT_D3D_RESET_DEVICE = EVENT_D3D_BASE + 2;

// Rozmiar bufor�w wierzcho�k�w i indeks�w w quadach
const int FONT_BUFFER_SIZE = 2000;


// Abstrakcyjna klasa bazowa dla zasob�w D3D
class D3dResource : public IResource
{
private:
	bool m_Created;
	bool m_Restored;

protected:
	D3dResource(const string &Name, const string &Group);
	virtual ~D3dResource();

	// ======== Implementacja IResource ========
	// Uwaga! Je�li klasa pochodna chce te� napisa� w�asne wersje tych metod, musi
	// wywo�a� wersje z tej klasy.
	virtual void OnLoad();
	virtual void OnUnload();
	virtual void OnEvent(uint4 Type, void *Params);

	// ======== Dla klas potomnych ========
	// UWAGA! W destruktorze klasy potomnej te� trzeba wpisa� kod zwalniaj�cy,
	// czyli ten sam co w OnDeviceInvalidate i OnDeviceDestroy.
	// Dlaczego? Bo destruktor nie mo�e u�y� metody wirtualnej, bo nie zadzia�a metoda z klasy pochodnej.

	// Tu tworzy� zasoby puli MANAGED
	virtual void OnDeviceCreate() = 0;
	// Tu usuwa� zasoby puli MANAGED
	virtual void OnDeviceDestroy() = 0;
	// Tu tworzy� zasoby puli DEFAULT
	virtual void OnDeviceRestore() = 0;
	// Tu usuwa� zasoby puli DEFAULT
	virtual void OnDeviceInvalidate() = 0;
};

// Abstrakcyjna klasa bazowa dla zasob�w tekstur Direct3D
class D3dBaseTexture : public D3dResource
{
private:
	IDirect3DBaseTexture9 *m_BaseTexture;
	string m_FileName;
	bool m_DisableMipMapping;

protected:
	// Ustawia� po wczytaniu tekstury
	void SetBaseTexture(IDirect3DBaseTexture9 *BaseTexture) { m_BaseTexture = BaseTexture; }

public:
	D3dBaseTexture(const string &Name, const string &Group, const string &FileName, bool DisableMipMapping = false) : D3dResource(Name, Group), m_BaseTexture(0), m_FileName(FileName), m_DisableMipMapping(DisableMipMapping) { }

	// Zwraca tekstur� lub 0 je�li aktualnie nie wczytana
	IDirect3DBaseTexture9 * GetBaseTexture() { return m_BaseTexture; }
	const string & GetFileName() { return m_FileName; }
	bool GetDisableMipMapping() { return m_DisableMipMapping; }
};


// Zwyczajna tekstura Direct3D - IDirect3DTexture9
class D3dTexture : public D3dBaseTexture
{
	friend void RegisterTypesD3d();

private:
	IDirect3DTexture9 *m_Texture;
	uint4 m_Width, m_Height;
	COLOR m_KeyColor; // 0 je�li nie ma koloru kluczowego, 0xFF000000 + x je�li jest

	static IResource * Create(const string &Name, const string &Group, Tokenizer &Params);

protected:
	// == Implementacja D3dResource ==
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

public:
	D3dTexture(const string &Name, const string &Group, const string &FileName, COLOR KeyColor = COLOR(0x00000000u), bool DisableMipMapping = false);
	virtual ~D3dTexture();

	// Zwraca tekstur� lub 0 je�li aktualnie nie wczytana
	IDirect3DTexture9 * GetTexture() { return m_Texture; }
	// Zwraca rozmiary tekstury lub 0 je�li nie by�a jeszcze wczytywana
	uint4 GetWidth() { return m_Width; }
	uint4 GetHeight() { return m_Height; }
};

// Tekstura sze�cienna Direct3D - IDirect3DCubeTexture9
class D3dCubeTexture : public D3dBaseTexture
{
	friend void RegisterTypesD3d();

private:
	IDirect3DCubeTexture9 *m_CubeTexture;
	uint4 m_Size;

	static IResource * Create(const string &Name, const string &Group, Tokenizer &Params);

protected:
	// == Implementacja D3dResource ==
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

public:
	D3dCubeTexture(const string &Name, const string &Group, const string &FileName);
	virtual ~D3dCubeTexture();

	// Zwraca tekstur� lub 0 je�li aktualnie nie wczytana
	IDirect3DCubeTexture9 * GetCubeTexture() { return m_CubeTexture; }
	// Zwraca rozmiar tekstury lub 0 je�li nie by�a jeszcze wczytywana
	uint4 GetSize() { return m_Size; }
};


// Czcionka D3DX - ID3DXFont
class D3dFont : public D3dResource
{
	friend void RegisterTypesD3d();

private:
	string m_FaceName;
	int m_Size;
	bool m_Bold, m_Italic;
	ID3DXFont *m_Font; // 0 je�li aktualnie nie istnieje

	static IResource * Create(const string &Name, const string &Group, Tokenizer &Params);

protected:
	// == Implementacja D3dResource ==
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

public:
	D3dFont(const string &Name, const string &Group, const string &FaceName, int Size, bool Bold, bool Italic);
	virtual ~D3dFont();

	const string & GetFaceName() { return m_FaceName; }
	int GetSize() { return m_Size; }
	bool GetBold() { return m_Bold; }
	bool GetItalic() { return m_Italic; }

	// Zwraca czcionk� lub 0 je�li aktualnie nie utworzona
	ID3DXFont * Get() { return m_Font; }
};

// Efekt D3DX - ID3DXEffect
class D3dEffect : public D3dResource
{
	friend void RegisterTypesD3d();

private:
	string m_FileName;
	STRING_VECTOR m_ParamNames;
	std::vector<D3DXHANDLE> m_ParamHandles; // Zawsze taka sama d�ugo�� jak powy�szy
	ID3DXEffect *m_Effect; // 0 je�li akurat nie wczytany

	static IResource * Create(const string &Name, const string &Group, Tokenizer &Params);
	void ClearParamHandles();
	void LoadParamHandles();

protected:
	// == Implementacja D3dResource ==
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

public:
	// ParamNames: mo�na poda� NULL.
	D3dEffect(const string &Name, const string &Group, const string &FileName, const STRING_VECTOR *ParamNames);
	virtual ~D3dEffect();

	// Zwraca efekt lub 0 je�li aktualnie nie wczytany
	ID3DXEffect * Get() { return m_Effect; }
	const string & GetFileName() { return m_FileName; }

	size_t GetParamCount() { return m_ParamNames.size(); }
	const string & GetParamName(size_t Index) { assert(Index < GetParamCount()); return m_ParamNames[Index]; }
	D3DXHANDLE GetParam(size_t Index) { assert(Index < GetParamCount()); return m_ParamHandles[Index]; }
	// Ustawia tablic� nazw parametr�w.
	// Je�li trzeba, uchwyty parametr�w automatycznie si� pobior�.
	// ParamNames: Mo�e by� NULL.
	void SetParamNames(const STRING_VECTOR *ParamNames);

	// Wywo�uje Begin i BeginPass(0)
	void Begin(bool SaveState);
	// Wywo�uje EndPass i End
	void End();
};

// Czcionka w�asna
class Font : public D3dResource
{
	friend void RegisterTypesD3d();

public:
	// == Podzia� wiersza ==
	// Pojedyncza linia (dzia�a szybko)
	static const uint4 FLAG_WRAP_SINGLELINE = 1 << 0;
	// Normalny podzia� wiersza - tylko kiedy jest znak ko�ca wiersza;
	static const uint4 FLAG_WRAP_NORMAL = 1 << 1;
	// Automatyczny podzia� wiersza na granicy znaku
	static const uint4 FLAG_WRAP_CHAR = 1 << 2;
	// Automatyczny podzia� wiersza na granicy s�owa
	static const uint4 FLAG_WRAP_WORD = 1 << 3;
	// == Kre�lenia ==
	// > Domy�lnie: brak kre�lenia.
	// > Mo�na wybra� jedno podkre�lenia i dowoln� jego kombinacj� z nadkre�leniem i przekre�leniem.
	// Podkre�lenie zwyk�e pojedyncze
	static const uint4 FLAG_UNDERLINE = 1 << 4;
	// Podkre�lenie podw�jne
	static const uint4 FLAG_DOUBLE = 1 << 5;
	// Nadkre�lenie
	static const uint4 FLAG_OVERLINE = 1 << 6;
	// Przekre�lenie
	static const uint4 FLAG_STRIKEOUT = 1 << 7;
	// == Dosuni�cie poziome ==
	static const uint4 FLAG_HLEFT = 1 << 8;
	static const uint4 FLAG_HCENTER = 1 << 9;
	static const uint4 FLAG_HRIGHT = 1 << 10;
	// == Dosuni�cie pionowe ==
	static const uint4 FLAG_VTOP = 1 << 11;
	static const uint4 FLAG_VMIDDLE = 1 << 12;
	static const uint4 FLAG_VBOTTOM = 1 << 13;

private:
	// Informacje o danym znaku ASCII
	struct CHAR_INFO
	{
		// Wsp�rz�dne tekstury
		float tx1, ty1, tx2, ty2;
		// Post�p do nast�pnego znaku, wyra�ony wzgl�dem wysoko�ci r�wnej 1.0
		float Advance;
		// Przesuni�cie wzgl�dem pocz�tku rysowania, wyra�one wzgl�dem wysoko�ci r�wnej 1.0
		float OffsetX, OffsetY;
		// Szeroko�� i wysoko�� cz�ci znaku rysowanej z tekstury, wyra�ona wzgl�dem wysoko�ci r�wnej 1.0
		float Width, Height;
	};

	struct VERTEX
	{
		VEC3 Pos;
		VEC2 Tex;
	};

	static IDirect3DVertexBuffer9 *m_VB;
	static IDirect3DIndexBuffer9 *m_IB;
	static int m_BufferRefCount;

	string m_DefFileName;
	string m_TextureFileName;
	// Informacje o wszystkich znakach ASCII
	CHAR_INFO m_CharInfo[256];
	// �rednia szeroko�� znaku wzgl�dem wysoko�ci 1.0
	float m_AvgCharWidth;
	// Wsp�rz�dne tekstury do rysowania kre�le�
	float m_LineTx1, m_LineTy1, m_LineTx2, m_LineTy2;

	IDirect3DTexture9 *m_Texture;
	VERTEX *m_LockedVB;
	uint2 *m_LockedIB;
	bool m_BufferAcquired;
	int m_BufferPos; // w quadach
	int m_VbIndex; // zapami�tane �eby za ka�dym razem nie wylicza� z m_BufferPos - opymalizacja
	int m_IbIndex; // zapami�tane �eby za ka�dym razem nie wylicza� z m_BufferPos - opymalizacja
	RECTF m_BoundingRect; // u�ywane podczas rysowania (Draw) w trybie z Bounding rect

	static IResource * Create(const string &Name, const string &Group, Tokenizer &Params);
	
	// Wczytuje plik z definicj� czcionki, wype�nia m_TextureFileName je�li puste.
	void LoadDef();

	void AcquireBuffer();
	void UnacquireBuffer();
	// Mi�dzy DrawBegin a DrawEnd bufory s� zablokowane.
	void Lock();
	void Unlock();
	void Flush();
	void DrawBegin();
	void DrawEnd();
	inline void DrawQuad2d(float x1, float y1, float x2, float y2, float tx1, float ty1, float tx2, float ty2)
	{
		if (m_BufferPos == FONT_BUFFER_SIZE)
		{
			Unlock();
			Flush();
			Lock();
		}

		uint2 BaseVertexIndex = (uint2)m_VbIndex;

		m_LockedVB[m_VbIndex].Pos = VEC3(x1, y1, 1.0f);
		m_LockedVB[m_VbIndex].Tex = VEC2(tx1, ty1);
		m_VbIndex++;
		m_LockedVB[m_VbIndex].Pos = VEC3(x2, y1, 1.0f);
		m_LockedVB[m_VbIndex].Tex = VEC2(tx2, ty1);
		m_VbIndex++;
		m_LockedVB[m_VbIndex].Pos = VEC3(x1, y2, 1.0f);
		m_LockedVB[m_VbIndex].Tex = VEC2(tx1, ty2);
		m_VbIndex++;
		m_LockedVB[m_VbIndex].Pos = VEC3(x2, y2, 1.0f);
		m_LockedVB[m_VbIndex].Tex = VEC2(tx2, ty2);
		m_VbIndex++;

		m_LockedIB[m_IbIndex++] = BaseVertexIndex;
		m_LockedIB[m_IbIndex++] = BaseVertexIndex+1;
		m_LockedIB[m_IbIndex++] = BaseVertexIndex+3;
		m_LockedIB[m_IbIndex++] = BaseVertexIndex;
		m_LockedIB[m_IbIndex++] = BaseVertexIndex+3;
		m_LockedIB[m_IbIndex++] = BaseVertexIndex+2;

		m_BufferPos++;
	}
	inline void DrawQuad3d(const VEC3 &Pos, const VEC3 &Right, const VEC3 &Down, float x1, float y1, float x2, float y2, float tx1, float ty1, float tx2, float ty2)
	{
		if (m_BufferPos == FONT_BUFFER_SIZE)
		{
			Unlock();
			Flush();
			Lock();
		}

		uint2 BaseVertexIndex = (uint2)m_VbIndex;

		m_LockedVB[m_VbIndex].Pos = Pos + x1*Right + y1*Down;
		m_LockedVB[m_VbIndex].Tex = VEC2(tx1, ty1);
		m_VbIndex++;
		m_LockedVB[m_VbIndex].Pos = Pos + x2*Right + y1*Down;
		m_LockedVB[m_VbIndex].Tex = VEC2(tx2, ty1);
		m_VbIndex++;
		m_LockedVB[m_VbIndex].Pos = Pos + x1*Right + y2*Down;
		m_LockedVB[m_VbIndex].Tex = VEC2(tx1, ty2);
		m_VbIndex++;
		m_LockedVB[m_VbIndex].Pos = Pos + x2*Right + y2*Down;
		m_LockedVB[m_VbIndex].Tex = VEC2(tx2, ty2);
		m_VbIndex++;

		m_LockedIB[m_IbIndex++] = BaseVertexIndex;
		m_LockedIB[m_IbIndex++] = BaseVertexIndex+1;
		m_LockedIB[m_IbIndex++] = BaseVertexIndex+3;
		m_LockedIB[m_IbIndex++] = BaseVertexIndex;
		m_LockedIB[m_IbIndex++] = BaseVertexIndex+3;
		m_LockedIB[m_IbIndex++] = BaseVertexIndex+2;

		m_BufferPos++;
	}
	void DrawBoundedQuad2d(float x1, float y1, float x2, float y2, float tx1, float ty1, float tx2, float ty2);
	inline void DrawFilledRect2d(float x1, float y1, float x2, float y2)
	{
		DrawQuad2d(x1, y1, x2, y2, m_LineTx1, m_LineTy1, m_LineTx2, m_LineTy2);
	}
	inline void DrawFilledRect3d(const VEC3 &Pos, const VEC3 &Right, const VEC3 &Down, float x1, float y1, float x2, float y2)
	{
		DrawQuad3d(Pos, Right, Down, x1, y1, x2, y2, m_LineTx1, m_LineTy1, m_LineTx2, m_LineTy2);
	}
	inline void DrawBoundedFilledRect2d(float x1, float y1, float x2, float y2)
	{
		DrawBoundedQuad2d(x1, y1, x2, y2, m_LineTx1, m_LineTy1, m_LineTx2, m_LineTy2);
	}

protected:
	// == Implementacja D3dResource ==
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

public:
	// Je�li TextureFileName puste, tekstura do czcionki b�dzie poszukiwana w tym samym katalogu co plik z definicj� i pod tak� nazw� o jakiej m�wi plik z definicj�.
	Font(const string &Name, const string &Group, const string &DefFileName, const string &TextureFileName);
	virtual ~Font();

	const string & GetDefFileName() { return m_DefFileName; }
	const string & GetTextureFileName() { return m_TextureFileName; }
	IDirect3DTexture9 * GetTexture() { return m_Texture; }

	// == Informacje ==
	// Mo�na je pobiera� zawsze, niezale�nie czy czcionka jako zas�b jest wczytana.

	// Zwraca �redni� szeroko�� znaku (dobre do czcionek sta�ej szeroko�ci) wzgl�dem wysoko�ci 1.0
	float GetAvgCharWidth() { return m_AvgCharWidth; }
	// Zwraca �redni� szeroko�� znaku (dobre do czcionek sta�ej szeroko�ci) wzgl�dem podanej wielko�ci
	float GetAvgCharWidth(float Size) { return m_AvgCharWidth * Size; }
	// Zwraca szeroko�� podanego znaku wzgl�dem wysoko�ci 1.0
	float GetCharWidth_(char Ch) { return m_CharInfo[(int)(uint1)Ch].Advance; }
	// Zwraca szeroko�� podanego znaku wzgl�dem podanej wysoko�ci
	float GetCharWidth_(char Ch, float Size) { return m_CharInfo[(int)(uint1)Ch].Advance * Size; }
	// Zwraca szeroko�� podanego tekstu pisanego jednowierszowo
	float GetTextWidth(const string &Text, size_t TextBegin, size_t TextEnd, float Size);
	float GetTextWidth(const string &Text, float Size) { return GetTextWidth(Text, 0, Text.length(), Size); }
	// Podzia� na wiersze
	// Wywo�ywa� iteracyjnie by otrzymywa� kolejne wiersze
	// Parametr Index jest [in,out], powinien by� przy pierwszym wywo�aniu 0.
	// Parametry OutWidth, OutBegin i OutEnd s� [out].
	// Zwraca false, je�li ju� jest na ko�cu i nie uda�o si� wybra� kolejnej linii.
	// Flags: znaczenie ma tylko FLAG_WRAP_*.
	// Je�li nie ma automatycznego zawijania wierszy, Width jest bez znaczenia.
	bool LineSplit(size_t *OutBegin, size_t *OutEnd, float *OutWidth, size_t *InOutIndex, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width);
	bool LineSplit(size_t *OutBegin, size_t *OutEnd, float *OutWidth, size_t *InOutIndex, const string &Text, float Size, uint4 Flags, float Width) { return LineSplit(OutBegin, OutEnd, OutWidth, InOutIndex, Text, 0, Text.length(), Size, Flags, Width); }
	// Oblicza szeroko�� i wysoko�� tekstu rysowanego z podanymi parametrami
	void GetTextExtent(float *OutWidth, float *OutHeight, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width);
	void GetTextExtent(float *OutWidth, float *OutHeight, const string &Text, float Size, uint4 Flags, float Width) { GetTextExtent(OutWidth, OutHeight, Text, 0, Text.length(), Size, Flags, Width); }
	// Liczba quad�w potrzebnych do narysowania podanego tekstu jednowierszowo
	// Flags: znaczenie maj� tylko kre�lenia.
	uint4 GetQuadCountSL(const string &Text, size_t TextBegin, size_t TextEnd, uint4 Flags);
	uint4 GetQuadCountSL(const string &Text, uint4 Flags) { return GetQuadCountSL(Text, 0, Text.length(), Flags); }
	// Liczba quad�w potrzebnych do narysowania podanego tekstu
	uint4 GetQuadCount(const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width);
	uint4 GetQuadCount(const string &Text, float Size, uint4 Flags, float Width) { return GetQuadCount(Text, 0, Text.length(), Size, Flags, Width); }
	// Zwraca indeks znaku, w kt�ry trafia podana pozycja HitX (test jednowymiarowy, tekst jednowierszowy)
	// Je�li znaleziony, zwraca te� procent szeroko�ci w znaku w kt�ry si� trafi�o
	bool HitTestSL(size_t *OutIndex, float *OutPercent, float PosX, float HitX, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags);
	bool HitTestSL(size_t *OutIndex, float *OutPercent, float PosX, float HitX, const string &Text, float Size, uint4 Flags) { return HitTestSL(OutIndex, OutPercent, PosX, HitX, Text, 0, Text.length(), Size, Flags); }
	// Zwraca indeks znaku, w kt�ry trafia podana pozycja (HitX,HitY)
	// Je�li znaleziony, zwraca te� procent szeroko�ci i wysoko�ci w znaku w kt�ry si� trafi�o
	bool HitTest(size_t *OutIndex, float *OutPercentX, float *OutPercentY, float PosX, float PosY, float HitX, float HitY, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width);
	bool HitTest(size_t *OutIndex, float *OutPercentX, float *OutPercentY, float PosX, float PosY, float HitX, float HitY, const string &Text, float Size, uint4 Flags, float Width) { return HitTest(OutIndex, OutPercentX, OutPercentY, PosX, PosY, HitX, HitY, Text, 0, Text.length(), Size, Flags, Width); }
	
	// == Rysowanie ==
	// Dzia�a tylko kiedy zas�b jest wczytany.

	// WERSJE 2D:
	// Wersja z BoundingRect i TextBegin-TextEnd
	void Draw(float X, float Y, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width, const RECTF &BoundingRect);
	// Wersja z BoundingRect
	void Draw(float X, float Y, const string &Text, float Size, uint4 Flags, float Width, const RECTF &BoundingRect) { Draw(X, Y, Text, 0, Text.length(), Size, Flags, Width, BoundingRect); }
	// Wersja z TextBegin-TextEnd
	void Draw(float X, float Y, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width);
	// Wersja bez tego ani tego
	void Draw(float X, float Y, const string &Text, float Size, uint4 Flags, float Width) { Draw(X, Y, Text, 0, Text.length(), Size, Flags, Width); }
	// Wersja bardzo prosta - do pojedynczej linii
	void DrawSL(float X, float Y, const string &Text, float Size) { Draw(X, Y, Text, Size, FLAG_WRAP_SINGLELINE | FLAG_HLEFT | FLAG_VTOP, 0.0f); }
	// Rysowanie prostok�cika, np. do kursora
	void DrawRect(float x1, float y1, float x2, float y2);
	void DrawRect(const RECTF &Rect) { DrawRect(Rect.left, Rect.top, Rect.right, Rect.bottom); }

	// WERSJE 3D:
	void Draw3d(const VEC3 &Pos, const VEC3 &Right, const VEC3 &Down, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width);
	void Draw3d(const VEC3 &Pos, const VEC3 &Right, const VEC3 &Down, const string &Text, float Size, uint4 Flags, float Width) { Draw3d(Pos, Right, Down, Text, 0, Text.length(), Size, Flags, Width); }
};

// Reprezentuje pojedyncz� dynamiczn� surface lub tekstur� + jej surface.
/*
- IsTexture=true - b�dzie tekstura + surface; IsTexture=false - b�dzie sama surface
- Usage: W przypadku IsTexture=false oznacza:
  - D3DUSAGE_RENDERTARGET = Render Target Surface
  - D3DUSAGE_DEPTHSTENCIL = Depth-Stencil Surface
  - ka�da inna warto�� = Off-screen Plain Surface
- Flaga Filled jest do dyspozycji dla u�ytkownika.
  Przestawia si� tylko sama na false kiedy powierzchnia przestaje istnie�.
- Width, Height mog� by� 0 o ile w tym stanie nie �adujesz zasobu.
*/
class D3dTextureSurface : public D3dResource
{
	friend void RegisterTypesD3d();

private:
	bool m_IsTexture;
	uint4 m_Width, m_Height;
	D3DPOOL m_Pool;
	uint4 m_Usage;
	D3DFORMAT m_Format;
	IDirect3DTexture9 *m_Texture;
	IDirect3DSurface9 *m_Surface;
	bool m_FilledFlag;

	void CreateSurface();
	void DestroySurface();

protected:
	static IResource * Create(const string &Name, const string &Group, Tokenizer &Params);

	// ======== Implementacja D3dResource ========
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

public:
	D3dTextureSurface(const string &Name, const string &Group, bool IsTexture, uint4 Width, uint4 Height, D3DPOOL Pool, uint4 Usage, D3DFORMAT Format);
	virtual ~D3dTextureSurface();

	bool IsTexture() { return m_IsTexture; }
	uint4 GetWidth() { return m_Width; }
	uint4 GetHeight() { return m_Height; }
	D3DPOOL GetPool() { return m_Pool; }
	uint4 GetUsage() { return m_Usage; }
	D3DFORMAT GetFormat() { return m_Format; }

	// Zwraca wska�nik do tekstury lub NULL, je�li nie utworzona lub nie ma tekstury (jest sam surface)
	IDirect3DTexture9 * GetTexture() { return m_Texture; }
	// Zwraca wska�nik do surface lub NULL, je�li nie utworzona
	IDirect3DSurface9 * GetSurface() { return m_Surface; }

	// Flaga czy powierzchnia jest wype�niona
	bool IsFilled() { return m_FilledFlag; }
	void SetFilled() { assert(m_Surface != NULL); m_FilledFlag = true; }
	void ResetFilled() { m_FilledFlag = false; }

	// Zmienia wymiary. Je�li trzeba, tworzy od nowa tekstur� i surface.
	void SetSize(uint4 Width, uint4 Height);
};

// Reprezentuje pojedyncz� dynamiczn� tekstur� sze�cienn� + jej surface.
/*
- Flaga Filled jest do dyspozycji dla u�ytkownika.
  Przestawia si� tylko sama na false kiedy powierzchnia przestaje istnie�.
- Size mo�e by� 0 o ile w tym stanie nie �adujesz zasobu.
*/
class D3dCubeTextureSurface : public D3dResource
{
	friend void RegisterTypesD3d();

private:
	uint4 m_Size;
	D3DPOOL m_Pool;
	uint4 m_Usage;
	D3DFORMAT m_Format;
	IDirect3DCubeTexture9 *m_Texture;
	IDirect3DSurface9 *m_Surfaces[6];
	uint m_FilledFlags; // Flagi bitowe dla kolejnych bit�w: 1, 2, 4, 8, 16, 32

	void CreateSurface();
	void DestroySurface();

protected:
	static IResource * Create(const string &Name, const string &Group, Tokenizer &Params);

	// ======== Implementacja D3dResource ========
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

public:
	D3dCubeTextureSurface(const string &Name, const string &Group, uint4 Size, D3DPOOL Pool, uint4 Usage, D3DFORMAT Format);
	virtual ~D3dCubeTextureSurface();

	uint4 GetSize() { return m_Size; }
	D3DPOOL GetPool() { return m_Pool; }
	uint4 GetUsage() { return m_Usage; }
	D3DFORMAT GetFormat() { return m_Format; }

	// Zwraca wska�nik do tekstury lub NULL, je�li nie utworzona lub nie ma tekstury (jest sam surface)
	IDirect3DCubeTexture9 * GetTexture() { return m_Texture; }
	// Zwraca wska�nik do surface lub NULL, je�li nie utworzona
	IDirect3DSurface9 * GetSurface(D3DCUBEMAP_FACES FaceType) { return m_Surfaces[(uint4)FaceType]; }
	// Zwraca wska�nik do wszystkich 6 powierzchni na raz
	void GetSurfaces(IDirect3DSurface9 *OutSurfaces[]);

	// Flaga czy powierzchnia z danej strony jest wype�niona
	bool IsFilled(D3DCUBEMAP_FACES FaceType) { return (m_FilledFlags & (1 << (uint)FaceType)) != 0; }
	void SetFilled(D3DCUBEMAP_FACES FaceType) { assert(m_Texture != NULL); m_FilledFlags |= (1 << (uint)FaceType); }
	void ResetFilled(D3DCUBEMAP_FACES FaceType) { m_FilledFlags &= ~(1 << (uint)FaceType); }
	// Czy wszystkie powierzchnie s� wype�nione
	bool IsAllFilled() { return m_FilledFlags == 0x3F; }
	void SetAllFilled() { m_FilledFlags = 0x3F; }
	void ResetAllFilled() { m_FilledFlags = 0; }

	// Zmienia wymiary. Je�li trzeba, tworzy od nowa tekstur� i surface.
	void SetSize(uint4 Size);
};

// Reprezentuje bufor wierzcho�k�w.
/*
- D�ugo��, liczba wierzcho�k�w do zablokowania, wszystko wyra�ane jest w wierzcho�kach.
- Bufor sam zna rozmiar wierzcho�ka na podstawie FVF.
- Blokowa� najlepiej przez tworzenie obiekt�w klasy D3dVertexBuffer::Lock.
- Wykorzystywanie konstruktora D3dVertexBuffer::Lock(..., false, ...) uruchamia
  mechanizm automatycznego blokowania kolejnych porcji bufora wg algorytmu
  zalecanego w DX SDK.
- Zas�b tego typu nie ma reprezentacji �a�cuchowej w XNL i nie mo�na go tak
  tworzy� ani te� predefiniowa� w pliku XNL.
*/
class D3dVertexBuffer : public D3dResource
{
public:
	class Locker
	{
	private:
		IDirect3DVertexBuffer9 *m_VB;
		void *m_Data;
		uint4 m_Offset;
		uint4 m_OffsetInBytes;

	public:
		// Konstruktory same robi� Load() podanego zasobu bufora.
		// LockAll=true - blokuje ca�y bufor.
		//   FlagsOrLength znaczy Flags.
		// LockAll=false - Blokuje kolejne Length wierzcho�k�w dynamicznego bufora.
		//   FlagsOrLength znaczy Length.
		//   D3dVertexBuffer sam wszystkim zarz�dza i u�ywa D3DLOCK_NOOVERWRITE lub D3DLOCK_DISCARD.
		Locker(D3dVertexBuffer *VertexBuffer, bool LockAll, uint4 FlagsOrLength);
		// Blokuje wyznaczony obszar bufora wyra�ony w wierzcho�kach.
		Locker(D3dVertexBuffer *VertexBuffer, uint4 OffsetToLock, uint4 SizeToLock, uint4 Flags);
		// Destruktor sam odblokowuje
		~Locker();

		// Zwraca wska�nik do zablokowanych danych
		void * GetData() { return m_Data; }
		// Zwraca offset w wierzcho�kach od pocz�tku bufora do pocz�tku zablokowanych danych
		uint4 GetOffset() { return m_Offset; }
		// Zwraca offset w bajtach od pocz�tku bufora do pocz�tku zablokowanych danych
		uint4 GetOffsetInBytes() { return m_OffsetInBytes; }
	};

private:
	// W wierzcho�kach.
	uint4 m_Length;
	uint4 m_Usage;
	uint4 m_FVF;
	D3DPOOL m_Pool;
	IDirect3DVertexBuffer9 *m_VB;
	bool m_FilledFlag;
	// Do dynamicznego blokowania kawa�k�w bufora. W wierzcho�kach.
	uint4 m_NextOffset;
	// Rozmiar jednego wierzcho�ka w bajtach.
	uint4 m_VertexSize;

	void CreateBuffer();
	void DestroyBuffer();

	// ======== Dla klasy Lock ========
	// Length - ile wierzcho�k�w zablokowa�, w wierzcho�kach.
	// Zwraca offset do zablokowania w wierzcho�kach.
	// Zwraca =0 - trzeba zablokowa� z DISCARD
	// Zwraca >0 - trzeba zablokowa� z NOOVERWRITE
	uint4 DynamicLock(uint4 Length);

protected:
	// ======== Implementacja D3dResource ========
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

public:
	D3dVertexBuffer(const string &Name, const string &Group, uint4 Length, uint4 Usage, uint4 FVF, D3DPOOL Pool);
	virtual ~D3dVertexBuffer();

	uint4 GetLength() { return m_Length; }
	uint4 GetUsage() { return m_Usage; }
	uint4 GetFVF() { return m_FVF; }
	D3DPOOL GetPool() { return m_Pool; }
	uint4 GetVertexSize() { return m_VertexSize; }

	// Zwraca wska�nik do bufora. Zwraca NULL, je�li nie jest aktualnie utworzony.
	IDirect3DVertexBuffer9 * GetVB() { return m_VB; }

	// Flaga czy bufor jest wype�niony
	bool IsFilled() { return m_FilledFlag; }
	void SetFilled() { assert(m_VB != NULL); m_FilledFlag = true; }
	void ResetFilled() { m_FilledFlag = false; }

	// Zmienia d�ugo��. Je�li trzeba, tworzy od nowa bufor.
	void SetLength(uint4 Length);
};

// Reprezentuje bufor indeks�w.
/*
- D�ugo��, liczba indeks�w do zablokowania, wszystko wyra�ane jest w indeksach.
- Bufor sam zna rozmiar indeksu na podstawie formatu.
- Blokowa� najlepiej przez tworzenie obiekt�w klasy D3dIndexBuffer::Lock.
- Wykorzystywanie konstruktora D3dIndexBuffer::Lock(..., false, ...) uruchamia
  mechanizm automatycznego blokowania kolejnych porcji bufora wg algorytmu
  zalecanego w DX SDK.
- Zas�b tego typu nie ma reprezentacji �a�cuchowej w XNL i nie mo�na go tak
  tworzy� ani te� predefiniowa� w pliku XNL.
*/
class D3dIndexBuffer : public D3dResource
{
public:
	class Locker
	{
	private:
		IDirect3DIndexBuffer9 *m_IB;
		void *m_Data;
		uint4 m_Offset;
		uint4 m_OffsetInBytes;

	public:
		// Konstruktory same robi� Load() podanego zasobu bufora.
		// LockAll=true - blokuje ca�y bufor.
		//   FlagsOrLength znaczy Flags.
		// LockAll=false - Blokuje kolejne Length indeks�w dynamicznego bufora.
		//   FlagsOrLength znaczy Length.
		//   D3dIndexBuffer sam wszystkim zarz�dza i u�ywa D3DLOCK_NOOVERWRITE lub D3DLOCK_DISCARD.
		Locker(D3dIndexBuffer *IndexBuffer, bool LockAll, uint4 FlagsOrLength);
		// Blokuje wyznaczony obszar bufora wyra�ony w indeksach.
		Locker(D3dIndexBuffer *IndexBuffer, uint4 OffsetToLock, uint4 SizeToLock, uint4 Flags);
		// Destruktor sam odblokowuje
		~Locker();

		// Zwraca wska�nik do zablokowanych danych
		void * GetData() { return m_Data; }
		// Zwraca offset w indeksach od pocz�tku bufora do pocz�tku zablokowanych danych
		uint4 GetOffset() { return m_Offset; }
		// Zwraca offset w bajtach od pocz�tku bufora do pocz�tku zablokowanych danych
		uint4 GetOffsetInBytes() { return m_OffsetInBytes; }
	};

private:
	// W indeksach.
	uint4 m_Length;
	uint4 m_Usage;
	D3DFORMAT m_Format;
	D3DPOOL m_Pool;
	IDirect3DIndexBuffer9 *m_IB;
	bool m_FilledFlag;
	// Do dynamicznego blokowania kawa�k�w bufora. W wierzcho�kach.
	uint4 m_NextOffset;
	// Rozmiar jednego wierzcho�ka w bajtach.
	uint4 m_IndexSize;

	void CreateBuffer();
	void DestroyBuffer();

	// ======== Dla klasy Lock ========
	// Length - ile indeks�w zablokowa�, w indeksach.
	// Zwraca offset do zablokowania w indeksach.
	// Zwraca =0 - trzeba zablokowa� z DISCARD
	// Zwraca >0 - trzeba zablokowa� z NOOVERWRITE
	uint4 DynamicLock(uint4 Length);

protected:
	// ======== Implementacja D3dResource ========
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

public:
	D3dIndexBuffer(const string &Name, const string &Group, uint4 Length, uint4 Usage, D3DFORMAT Format, D3DPOOL Pool);
	virtual ~D3dIndexBuffer();

	uint4 GetLength() { return m_Length; }
	uint4 GetUsage() { return m_Usage; }
	D3DFORMAT GetFormat() { return m_Format; }
	D3DPOOL GetPool() { return m_Pool; }
	uint4 GetIndexSize() { return m_IndexSize; }

	// Zwraca wska�nik do bufora. Zwraca NULL, je�li nie jest aktualnie utworzony.
	IDirect3DIndexBuffer9 * GetIB() { return m_IB; }

	// Flaga czy bufor jest wype�niony
	bool IsFilled() { return m_FilledFlag; }
	void SetFilled() { assert(m_IB != NULL); m_FilledFlag = true; }
	void ResetFilled() { m_FilledFlag = false; }

	// Zmienia d�ugo��. Je�li trzeba, tworzy od nowa bufor.
	void SetLength(uint4 Length);
};

class OcclusionQueries_pimpl;

class OcclusionQueries : public D3dResource
{
public:
	class Query_pimpl;
	class Issue;

	class Query
	{
	public:
		// Sprawdza, czy s� ju� wyniki.
		// - Je�li s�, zwraca true i zwraca PixelCount.
		// - Je�li nie ma, zwraca false.
		// - Je�li b��d, zwraac true i PixelCount=MAXUINT4.
		// Wywo�ywa� tylko dla zapyta� ju� zapytanych.
		// Nie p�tli� si� na tej funkcji bo mo�e nigdy nie zwr�ci� true, tylko wywo�a� GetResult celem poczekania.
		bool CheckResult(uint *OutPixelCount);
		// Czeka i zwraca wynik zapytania (liczb� wyrenderowanych pikseli).
		// Je�li b��d, zwraca MAXUINT4.
		// Wywo�ywa� tylko dla zapyta� ju� zapytanych.
		uint GetResult();

		// ======== Tylko dla klasy OcclusionQueries ========
		Query();
		~Query();

	private:
		friend class OcclusionQueries;
		friend class OcclusionQueries::Issue;

		scoped_ptr<Query_pimpl> pimpl;

		void IssueBegin();
		void IssueEnd();
	};

	// Klasa wykonuj�ca zadawanie zapytania
	// Ka�de zapytanie mo�na zada� tylko raz, a wyniki odczytywa� dopiero po zadaniu.
	// Konstruktor rozpoczyna, destruktor ko�czy zadawanie zapytania.
	class Issue
	{
	public:
		Issue(Query *q) : m_Query(q) { m_Query->IssueBegin(); }
		~Issue() { m_Query->IssueEnd(); }

	private:
		Query *m_Query;
	};

	OcclusionQueries(const string &Name, const string &Group, uint MaxRealQueries);
	~OcclusionQueries();

	// Tworzy nowe zapytanie
	Query * Create();
	// Usuwa zapytanie
	void Destroy(Query *q);

	// ======== Tylko dla klasy Query ========
	void OnQueryGotResult(uint CellIndex);

protected:
	// ======== Implementacja D3dResource ========
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

private:
	scoped_ptr<OcclusionQueries_pimpl> pimpl;

	friend class Issue;
};

void RegisterTypesD3d();

} // namespace res

#endif
