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

// Indeks elementu sprita oznaczaj¹cy ca³oœæ tekstury
const uint4 INDEX_ALL  = MAXUINT4;
// Indeks elementu sprita oznaczaj¹cy brak elementu w danym miejscu
const uint4 INDEX_NONE = MAXUINT4 - 1;

// == Flagi do elementów spritów ==

// Mo¿na tylko jedn¹ z tych trzech (lub ¿adn¹):
static const uint4 FLAG_ROTATE_CW  = 0x00000001; // Obrót o 90 stopni w prawo
static const uint4 FLAG_ROTATE_CCW = 0x00000002; // Obrót o 90 stopni w lewo
static const uint4 FLAG_ROTATE_UD  = 0x00000004; // Obrót o 180 stopni
// Mo¿na dowolne z tych dwóch:
static const uint4 FLAG_FLIP_V     = 0x00000010; // Odbicie pionowe |
static const uint4 FLAG_FLIP_H     = 0x00000020; // Odbicie poziome -


// Zamienia indeks na oznaczaj¹cy element z³o¿ony sprita
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

	// Tryb elementów prostych
	// True = macierz
	// False = w³asne prostok¹ty
	bool m_SimpleElementsMatrix;
	// Tylko jeœli elementy proste to macierz - parametry tej macierzy
	uint4 m_MatrixColCount;
	uint4 m_MatrixWidth, m_MatrixHeight;
	uint4 m_MatrixStartX, m_MatrixStartY;
	uint4 m_MatrixSpaceX, m_MatrixSpaceY;
	// Tylko jeœli elementy proste to w³asne prostok¹ty
	std::vector<RECTI> m_CustomRects;

	// Elementy z³o¿one
	std::vector<COMPLEX_ELEMENT> m_ComplexElements;

	Sprite(const string &TextureName, const string &EffectName);
	Sprite(res::D3dTexture *Texture, res::D3dEffect *Effect);

	// Ustawia tryb elementów prostych na macierzowy i ustawia parametry macierzy
	void SetSimpleElementsMatrix(uint4 ColCount, uint4 Width, uint4 Height, uint4 StartX, uint4 StartY, uint4 SpaceX, uint4 SpaceY);
	// Ustawia tryb elementów prostych na prostok¹ty w³asne
	void SetSimpleElementsCustomRects();
	// U¿yj tej funkcji by dodaæ prostok¹t w³asny do elementów prostych
	void AddSimpleElementsCustomRect(uint4 index, const RECTI &rect);
	// U¿yj tej funkcji by dodaæ element z³o¿ony
	void AddComplexElement(uint4 index, const COMPLEX_ELEMENT &ComplexElement);

	// Zamienia indeks prostego elementu (mo¿na te¿ podaæ INDEX_ALL) na wspó³rzêdne tekstury
	void GetTexCoords(uint4 Index, float *TexLeft, float *TexRight, float *TexTop, float *TexBottom);
	// Przetwarza podane wspó³rzêdne tekstur na docelowe zgodnie z podanymi flagami obrotu/odbicia
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

	// Wczytuje definicje spritów z podanego pliku
	void CreateSpritesFromFile(const string &FileName);
	// Wczytuje definicjê pojedynczego sprita z ³añcucha
	Sprite * CreateSpriteFromString(const string &Name, const string &Params);
	// W³asnoœæ przechodzi na ten obiekt, on zwolni podany sprite
	void CreateSprite(const string &name, Sprite *sprite);
	bool DestroySprite(const string &name);
	bool DestroySprite(Sprite *sprite);
	void MustDestroySprite(const string &name);
	void MustDestroySprite(Sprite *sprite);

	// Jeœli nie znajdzie, zwraca 0
	Sprite * GetSprite(const string &name);
	// Jeœli nie znajdzie, rzuca wyj¹tek
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
	// Zwraca ten efekt - przy pierwszym u¿yciu go pobiera, przy ka¿dym Loaduje
	ID3DXEffect * GetPlainColorEffect();
	// Zwraca ten efekt - przy pierwszym u¿yciu go pobiera, przy ka¿dym Loaduje
	ID3DXEffect * GetFontEffect();

	// Stos prostok¹tów ograniczaj¹cych
	std::stack<RECTF> m_ClipRects;
	// Czy bie¿¹cy prostok¹t ograniczaj¹cy dzia³a (ogranicza)
	std::stack<bool> m_ClipRectsEnabled;
	// Stos translacji
	std::stack<VEC2> m_Translations;
	// Stos przezroczystoœci (0..1)
	std::stack<float> m_Alphas;

	// Bie¿¹ce ustawienia
	COLOR m_CurrentColor;
	Sprite *m_CurrentSprite;
	res::Font *m_CurrentFont;

	// Co siê aktualnie robi
	ACTION m_Action;
	// == Jeœli ACTION_SPRITES ==
	// Elementy czekaj¹ce na narysowanie
	std::vector<SPRITE_VERTEX> m_SpriteVB;
	std::vector<uint2> m_SpriteIB;
	// Liczba elementów oczekuj¹cych na narysowanie w quadach
	uint4 m_SpriteQuadCount;

	// Funkcja mo¿e sobie zmieniæ v - wykorzystuje go te¿ jako tymczasow¹ zmienn¹
	void AddRect(const RECTF &rect, SPRITE_VERTEX v[]);
	// T¹ magiczn¹ funkcjê trudno opisaæ
	// Jeœli prostok¹t jest nieprawid³owy, nic nie robi.
	// Korzysta z m_CurrentSprite.
	// Rect ma byæ nieprzesuniêty o bie¿ac¹ translacjê.
	// Width i Height mog¹ byæ:
	// - 0.0f = oznacza ca³¹ szerokoœæ
	// < 0 = w procentach szerokoœci/wysokoœci podanej w rect
	// > 0 = w jednostkach
	// U¿ywa AddRect ¿eby wygenerowaæ jeden lub wiêcej quadów z siatk¹ powtórzonej tekstury.
	// Pozwala sobie u¿ywaæ Push- i PopClipRect.
	void GenerateComplexElementRect(const VEC2 &CurrentTranslation, SPRITE_VERTEX v[], const RECTF &rect, uint4 Index, uint4 Flags, float Width, float Height);
	// Podejmuje dzia³ania zwi¹zane z rozpoczêciem akcji wpisanej do m_Action
	void BeginAction();
	// Podejmuje dzia³ania zwi¹zane z rozpoczêciem akcji pozostaj¹cej w m_Action
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

	// Zmienia kolor rysowanych prostok¹tów i tekstu
	void SetColor(COLOR color);
	// Zmienia sprite, którym rysowane s¹ prostok¹ty
	// 0 oznacza rysowanie jednolitym kolorem
	void SetSprite(Sprite *sprite);
	// Zmienia czcionkê, któr¹ rysowany jest tekst
	void SetFont(res::Font *font);

	// Zwraca bie¿¹cy prostok¹t ograinczaj¹cy lub false jeœli aktualnie nie ma ograniczenia
	bool GetCurrentClipRect(RECTF *Out);
	void PushClipRect(const RECTF &rect);
	void PushClipCancel();
	void PopClipRect();

	// Zwraca bie¿¹c¹ translacjê
	void GetCurrentTranslation(VEC2 *Out);
	void PushTranslation(const VEC2 &v);
	void PushTranslationCancel();
	void PopTranslation();

	// Zwraca bie¿¹cy kana³ alfa 0..1
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
