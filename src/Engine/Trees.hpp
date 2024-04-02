/* Trees
 * The Final Quest
 * Autor: Adam Sawicki
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef TREES_H_
#define TREES_H_

#include "Engine.hpp"

class ParamsCamera;

namespace engine
{

class Scene;

const uint TREE_MAX_LEVELS = 4;

// Opisuje poszczególny poziom rozga³êzienia drzewa.
// Pola -V to sk³adnik losowy, tak ¿e w³aœciwa wielkoœæ jakiegoœ X jest z zakresu (X-XV, X+XV).
struct TREE_LEVEL_DESC
{
	// Czy jest widoczny? Nawet jeœli nie, nadal mo¿e pos³u¿yæ do generowania liœci.
	bool Visible;

	////// Parametry rozga³êziania
	// Liczba podga³êzi
	uint SubbranchCount;
	// Zakres d³ugoœci w którym s¹ rozmieszczone odga³êzienia [procent od, procent do]
	float SubbranchRangeMin, SubbranchRangeMax;
	// K¹t odga³êzienia
	float SubbranchAngle, SubbranchAngleV;

	////// D³ugoœæ ga³¹zki
	// Sk³adnik sta³y
	float Length, LengthV;
	// Sk³adnik dodatkowy: Procent wzglêdem pozosta³ej d³ugoœci ga³êzi nadrzêdnej
	float LengthToParent;

	////// Gruboœæ ga³¹zki
	// Gruboœæ pocz¹tkowa
	float Radius, RadiusV;
	// Gruboœæ koñcowa jako procent gruboœci pocz¹tkowej
	float RadiusEnd;

	////// Liœcie
	// Liczba liœci
	float LeafCount, LeafCountV;
	// Zakres d³ugoœci w którym rozmieszczone s¹ liœcie
	float LeafRangeMin, LeafRangeMax;

	void LoadFromTokenizer(Tokenizer &t);
};

// Opis ca³ego drzewa.
struct TREE_DESC
{
	////// Ogólne
	// Po³owa wielkoœci drzewa w osi X i Z
	float HalfWidth;
	// Po³owa wielkoœci drzewa w osi Y
	float HalfHeight;

	////// Poszczególne poziomy
	// Mo¿e ich byæ mniej, wystarczy na jednym z nich wpisaæ SubbranchCount = 0.
	TREE_LEVEL_DESC Levels[TREE_MAX_LEVELS];
	// Opisuje korzeñ.
	// Tu parametry rozga³êzienia dotycz¹ rozga³êzienia pnia w korzeñ.
	TREE_LEVEL_DESC Roots;

	////// Teksturowanie
	// Rozci¹gniêcie tekstury kory na d³ugoœæ
	float BarkTexScaleY;

	////// Liœcie
	// Rozmiar quada liœci - po³owa szerokoœci i wysokoœci
	float LeafHalfSizeCX, LeafHalfSizeCY;
	float LeafHalfSizeCXV, LeafHalfSizeCYV;
	// Czy dozwolone jest odbijanie tekstury liœci do góry nogami?
	// Jeœli nie, odbijana bêdzie tylko wzglêdem osi Y.
	bool AllowTexFlipX;

	void LoadFromTokenizer(Tokenizer &t);
};

// Opisuje teksturê drzewa
/*
Jako wejœcie podaje siê prostok¹ty na tej teksturze w pikselach (RECTI).
Jako wyjœcie otrzymuje siê wspó³rzêdne tekstury w zakresie 0..1 (RECTF).
*/
class TreeTextureDesc
{
public:
	TreeTextureDesc() { }
	TreeTextureDesc(const string &TextureName, uint TextureCX, uint TextureCY, const RECTI &Root, const RECTI &Leaves) { Set(TextureName, TextureCX, TextureCY, Root, Leaves); }
	void Set(const string &TextureName, uint TextureCX, uint TextureCY, const RECTI &Root, const RECTI &Leaves);
	void LoadFromTokenizer(Tokenizer &t);

	const string & GetTextureName() const { return m_TextureName; }
	const RECTF & GetRoot() const { return m_Root; }
	const RECTF & GetLeaves() const { return m_Leaves; }
	uint GetTextureWidth() const { return m_TextureCX; }
	uint GetTextureHeight() const { return m_TextureCY; }

	// Zwraca wskaŸnik do tekstury, LOADED, lub NULL jeœli nie znaleziono.
	res::D3dTexture * GetTexture() const;

private:
	string m_TextureName;
	RECTF m_Root;
	RECTF m_Leaves;
	uint m_TextureCX;
	uint m_TextureCY;

	mutable res::D3dTexture *m_Texture; // Pobierana przy pierwszym u¿yciu
};


class Tree_pimpl;

/*
Klasa reprezentuje gatunek drzewa.
Tworzy sobie i u¿ywa zasoby D3D typu bufory VB i IB, dlatego one musz¹ ju¿ istnieæ.
Musi te¿ istnieæ podana tekstura.
Obiekt tworzy siê na podstawie opisu drzewa TREE_DESC, ten opis kopiuje sobie do
wnêtrza i w czasie pracy klasy pozostaje niezmienny.
*/
class Tree
{
public:
	Tree(Scene *OwnerScene, const TREE_DESC &Desc, const TreeTextureDesc &TextureDesc, uint KindCount, uint KindRandomSeeds[]);
	~Tree();

	const TREE_DESC & GetDesc();
	const TreeTextureDesc & GetTextureDesc();
	uint GetKindCount();

	// DirToLight, LightColor - parametry œwiat³a kierunkowego.
	// Jeœli DirToLight albo LightColor == NULL, oœwietlenia nie ma.
	// DirToLight musi byæ znormalizowany.
	// Jeœli FogStart albo FogColor == NULL, mg³a jest wy³¹czona.
	// Przekazane wskaŸniki, jeœli nie NULL, musz¹ pozostaæ aktualne a¿ do End.
	void DrawBegin(const ParamsCamera &Cam, const VEC3 *DirToLight, const COLORF *LightColor, const float *FogStart, const COLORF *FogColor);
	void DrawBegin_ShadowMap(const MATRIX &ViewProj, const VEC3 &LightDir);
	void DrawEnd();

	////// Wywo³ywaæ miêdzy DrawBegin a DrawEnd
	// Macierz nie powinna zawieraæ zbyt du¿ego skalowania, bo bêdzie burak - billboardy liœci nie bêd¹ skalowane.
	// InvWorldMat - opcjonalny, jeœli nie podany to zostanie wyliczony.
	void DrawTree(uint Kind, const MATRIX &WorldMat, const MATRIX *InvWorldMat, const COLORF &Color);

private:
	scoped_ptr<Tree_pimpl> pimpl;
};

} // namespace engine

#endif
