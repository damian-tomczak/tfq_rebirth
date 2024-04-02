/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef GFX_2D_
#define GFX_2D_

#include <stack>
#include "Framework.hpp"
#include "ResMngr.hpp"
#include "Res_d3d.hpp"

namespace gfx2d
{

// Indeks elementu sprita oznaczaj�cy ca�o�� tekstury
const uint4 INDEX_ALL  = MAXUINT4;
// Indeks elementu sprita oznaczaj�cy brak elementu w danym miejscu
const uint4 INDEX_NONE = MAXUINT4 - 1;

// == Flagi do element�w sprit�w ==

// Mo�na tylko jedn� z tych trzech (lub �adn�):
static const uint4 FLAG_ROTATE_CW  = 0x00000001; // Obr�t o 90 stopni w prawo
static const uint4 FLAG_ROTATE_CCW = 0x00000002; // Obr�t o 90 stopni w lewo
static const uint4 FLAG_ROTATE_UD  = 0x00000004; // Obr�t o 180 stopni
// Mo�na dowolne z tych dw�ch:
static const uint4 FLAG_FLIP_V     = 0x00000010; // Odbicie pionowe |
static const uint4 FLAG_FLIP_H     = 0x00000020; // Odbicie poziome -


// Zamienia indeks na oznaczaj�cy element z�o�ony sprita
inline uint4 ComplexElement(uint4 Index)
{
	return Index + 1000000000;
}


class Sprite
{
public:
	struct COMPLEX_ELEMENT
	{
		uint4 CenterIndex;
		uint4 CenterFlags;
		float CenterWidth, CenterHeight;
		uint4 LeftTopIndex, RightTopIndex, LeftBottomIndex, RightBottomIndex;
		uint4 LeftTopFlags, RightTopFlags, LeftBottomFlags, RightBottomFlags;
		uint4 LeftIndex, RightIndex, TopIndex, BottomIndex;
		uint4 LeftFlags, RightFlags, TopFlags, BottomFlags;
		float LeftWidth, RightWidth, TopWidth, BottomWidth;
		float LeftHeight, RightHeight, TopHeight, BottomHeight;
	};

	// Zasoby
	res::D3dEffect *m_Effect;
	res::D3dTexture *m_Texture;

	// Tryb element�w prostych
	// True = macierz
	// False = w�asne prostok�ty
	bool m_SimpleElementsMatrix;
	// Tylko je�li elementy proste to macierz - parametry tej macierzy
	uint4 m_MatrixColCount;
	uint4 m_MatrixWidth, m_MatrixHeight;
	uint4 m_MatrixStartX, m_MatrixStartY;
	uint4 m_MatrixSpaceX, m_MatrixSpaceY;
	// Tylko je�li elementy proste to w�asne prostok�ty
	std::vector<RECTI> m_CustomRects;

	// Elementy z�o�one
	std::vector<COMPLEX_ELEMENT> m_ComplexElements;

	Sprite(const string &TextureName, const string &EffectName);
	Sprite(res::D3dTexture *Texture, res::D3dEffect *Effect);

	// Ustawia tryb element�w prostych na macierzowy i ustawia parametry macierzy
	void SetSimpleElementsMatrix(uint4 ColCount, uint4 Width, uint4 Height, uint4 StartX, uint4 StartY, uint4 SpaceX, uint4 SpaceY);
	// Ustawia tryb element�w prostych na prostok�ty w�asne
	void SetSimpleElementsCustomRects();
	// U�yj tej funkcji by doda� prostok�t w�asny do element�w prostych
	void AddSimpleElementsCustomRect(uint4 index, const RECTI &rect);
	// U�yj tej funkcji by doda� element z�o�ony
	void AddComplexElement(uint4 index, const COMPLEX_ELEMENT &ComplexElement);

	// Zamienia indeks prostego elementu (mo�na te� poda� INDEX_ALL) na wsp�rz�dne tekstury
	void GetTexCoords(uint4 Index, float *TexLeft, float *TexRight, float *TexTop, float *TexBottom);
	// Przetwarza podane wsp�rz�dne tekstur na docelowe zgodnie z podanymi flagami obrotu/odbicia
	void ProcessFlags(VEC2 *tex11, VEC2 *tex21, VEC2 *tex12, VEC2 *tex22, float tx1, float ty1, float tx2, float ty2, uint4 Flags);
};


class SpriteRepository_pimpl;

class SpriteRepository
{
private:
	scoped_ptr<SpriteRepository_pimpl> pimpl;

public:
	SpriteRepository();
	~SpriteRepository();

	// Wczytuje definicje sprit�w z podanego pliku
	void CreateSpritesFromFile(const string &FileName);
	// Wczytuje definicj� pojedynczego sprita z �a�cucha
	Sprite * CreateSpriteFromString(const string &Name, const string &Params);
	// W�asno�� przechodzi na ten obiekt, on zwolni podany sprite
	void CreateSprite(const string &name, Sprite *sprite);
	bool DestroySprite(const string &name);
	bool DestroySprite(Sprite *sprite);
	void MustDestroySprite(const string &name);
	void MustDestroySprite(Sprite *sprite);

	// Je�li nie znajdzie, zwraca 0
	Sprite * GetSprite(const string &name);
	// Je�li nie znajdzie, rzuca wyj�tek
	Sprite * MustGetSprite(const string &name);
};

extern SpriteRepository *g_SpriteRepository;


class Canvas : public frame::IFrameObject
{
private:
	enum ACTION
	{
		ACTION_NONE,
		ACTION_SPRITES,
		ACTION_TEXT
	};

	struct SPRITE_VERTEX
	{
		static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);
		VEC3 Pos;
		COLOR Color;
		VEC2 Tex;
	};

	res::D3dEffect *m_PlainColorEffect;
	res::D3dEffect *m_FontEffect;
	D3DXHANDLE m_FontEffectColor;
	D3DXHANDLE m_FontEffectTexture;
	// Zwraca ten efekt - przy pierwszym u�yciu go pobiera, przy ka�dym Loaduje
	ID3DXEffect * GetPlainColorEffect();
	// Zwraca ten efekt - przy pierwszym u�yciu go pobiera, przy ka�dym Loaduje
	ID3DXEffect * GetFontEffect();

	// Stos prostok�t�w ograniczaj�cych
	std::stack<RECTF> m_ClipRects;
	// Czy bie��cy prostok�t ograniczaj�cy dzia�a (ogranicza)
	std::stack<bool> m_ClipRectsEnabled;
	// Stos translacji
	std::stack<VEC2> m_Translations;
	// Stos przezroczysto�ci (0..1)
	std::stack<float> m_Alphas;

	// Bie��ce ustawienia
	COLOR m_CurrentColor;
	Sprite *m_CurrentSprite;
	res::Font *m_CurrentFont;

	// Co si� aktualnie robi
	ACTION m_Action;
	// == Je�li ACTION_SPRITES ==
	// Elementy czekaj�ce na narysowanie
	std::vector<SPRITE_VERTEX> m_SpriteVB;
	std::vector<uint2> m_SpriteIB;
	// Liczba element�w oczekuj�cych na narysowanie w quadach
	uint4 m_SpriteQuadCount;

	// Funkcja mo�e sobie zmieni� v - wykorzystuje go te� jako tymczasow� zmienn�
	void AddRect(const RECTF &rect, SPRITE_VERTEX v[]);
	// T� magiczn� funkcj� trudno opisa�
	// Je�li prostok�t jest nieprawid�owy, nic nie robi.
	// Korzysta z m_CurrentSprite.
	// Rect ma by� nieprzesuni�ty o bie�ac� translacj�.
	// Width i Height mog� by�:
	// - 0.0f = oznacza ca�� szeroko��
	// < 0 = w procentach szeroko�ci/wysoko�ci podanej w rect
	// > 0 = w jednostkach
	// U�ywa AddRect �eby wygenerowa� jeden lub wi�cej quad�w z siatk� powt�rzonej tekstury.
	// Pozwala sobie u�ywa� Push- i PopClipRect.
	void GenerateComplexElementRect(const VEC2 &CurrentTranslation, SPRITE_VERTEX v[], const RECTF &rect, uint4 Index, uint4 Flags, float Width, float Height);
	// Podejmuje dzia�ania zwi�zane z rozpocz�ciem akcji wpisanej do m_Action
	void BeginAction();
	// Podejmuje dzia�ania zwi�zane z rozpocz�ciem akcji pozostaj�cej w m_Action
	void EndAction();
	void StartAction(ACTION NewAction);

public:
	Canvas();

	// ======== Implementacja frame::IFrameObject ========
	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnLostDevice();
	virtual void OnResetDevice();

	void Flush();

	// Zmienia kolor rysowanych prostok�t�w i tekstu
	void SetColor(COLOR color);
	// Zmienia sprite, kt�rym rysowane s� prostok�ty
	// 0 oznacza rysowanie jednolitym kolorem
	void SetSprite(Sprite *sprite);
	// Zmienia czcionk�, kt�r� rysowany jest tekst
	void SetFont(res::Font *font);

	// Zwraca bie��cy prostok�t ograinczaj�cy lub false je�li aktualnie nie ma ograniczenia
	bool GetCurrentClipRect(RECTF *Out);
	void PushClipRect(const RECTF &rect);
	void PushClipCancel();
	void PopClipRect();

	// Zwraca bie��c� translacj�
	void GetCurrentTranslation(VEC2 *Out);
	void PushTranslation(const VEC2 &v);
	void PushTranslationCancel();
	void PopTranslation();

	// Zwraca bie��cy kana� alfa 0..1
	float GetCurrentAlpha();
	void PushAlpha(float Alpha);
	void PushAlphaCancel();
	void PopAlpha();

	void DrawRect(const RECTF &rect, uint4 Index = INDEX_ALL);
	// == Rysowanie tekstu ==
	// Flags jak w dokumentacji do res::Font.
	void DrawText_(float X, float Y, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width);
	void DrawText_(float X, float Y, const string &Text, float Size, uint4 Flags, float Width) { DrawText_(X, Y, Text, 0, Text.length(), Size, Flags, Width); }
	// Wersja bardzo prosta - do pojedynczej linii
	void DrawTextSL(float X, float Y, const string &Text, float Size) { DrawText_(X, Y, Text, Size, res::Font::FLAG_WRAP_SINGLELINE | res::Font::FLAG_HLEFT | res::Font::FLAG_VTOP, 0.0f); }
};

extern Canvas *g_Canvas;

} // namespace gfx2d

#endif
