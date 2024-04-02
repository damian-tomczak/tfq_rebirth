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

// Opisuje poszczeg�lny poziom rozga��zienia drzewa.
// Pola -V to sk�adnik losowy, tak �e w�a�ciwa wielko�� jakiego� X jest z zakresu (X-XV, X+XV).
struct TREE_LEVEL_DESC
{
	// Czy jest widoczny? Nawet je�li nie, nadal mo�e pos�u�y� do generowania li�ci.
	bool Visible;

	////// Parametry rozga��ziania
	// Liczba podga��zi
	uint SubbranchCount;
	// Zakres d�ugo�ci w kt�rym s� rozmieszczone odga��zienia [procent od, procent do]
	float SubbranchRangeMin, SubbranchRangeMax;
	// K�t odga��zienia
	float SubbranchAngle, SubbranchAngleV;

	////// D�ugo�� ga��zki
	// Sk�adnik sta�y
	float Length, LengthV;
	// Sk�adnik dodatkowy: Procent wzgl�dem pozosta�ej d�ugo�ci ga��zi nadrz�dnej
	float LengthToParent;

	////// Grubo�� ga��zki
	// Grubo�� pocz�tkowa
	float Radius, RadiusV;
	// Grubo�� ko�cowa jako procent grubo�ci pocz�tkowej
	float RadiusEnd;

	////// Li�cie
	// Liczba li�ci
	float LeafCount, LeafCountV;
	// Zakres d�ugo�ci w kt�rym rozmieszczone s� li�cie
	float LeafRangeMin, LeafRangeMax;

	void LoadFromTokenizer(Tokenizer &t);
};

// Opis ca�ego drzewa.
struct TREE_DESC
{
	////// Og�lne
	// Po�owa wielko�ci drzewa w osi X i Z
	float HalfWidth;
	// Po�owa wielko�ci drzewa w osi Y
	float HalfHeight;

	////// Poszczeg�lne poziomy
	// Mo�e ich by� mniej, wystarczy na jednym z nich wpisa� SubbranchCount = 0.
	TREE_LEVEL_DESC Levels[TREE_MAX_LEVELS];
	// Opisuje korze�.
	// Tu parametry rozga��zienia dotycz� rozga��zienia pnia w korze�.
	TREE_LEVEL_DESC Roots;

	////// Teksturowanie
	// Rozci�gni�cie tekstury kory na d�ugo��
	float BarkTexScaleY;

	////// Li�cie
	// Rozmiar quada li�ci - po�owa szeroko�ci i wysoko�ci
	float LeafHalfSizeCX, LeafHalfSizeCY;
	float LeafHalfSizeCXV, LeafHalfSizeCYV;
	// Czy dozwolone jest odbijanie tekstury li�ci do g�ry nogami?
	// Je�li nie, odbijana b�dzie tylko wzgl�dem osi Y.
	bool AllowTexFlipX;

	void LoadFromTokenizer(Tokenizer &t);
};

// Opisuje tekstur� drzewa
/*
Jako wej�cie podaje si� prostok�ty na tej teksturze w pikselach (RECTI).
Jako wyj�cie otrzymuje si� wsp�rz�dne tekstury w zakresie 0..1 (RECTF).
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

	// Zwraca wska�nik do tekstury, LOADED, lub NULL je�li nie znaleziono.
	res::D3dTexture * GetTexture() const;

private:
	string m_TextureName;
	RECTF m_Root;
	RECTF m_Leaves;
	uint m_TextureCX;
	uint m_TextureCY;

	mutable res::D3dTexture *m_Texture; // Pobierana przy pierwszym u�yciu
};


class Tree_pimpl;

/*
Klasa reprezentuje gatunek drzewa.
Tworzy sobie i u�ywa zasoby D3D typu bufory VB i IB, dlatego one musz� ju� istnie�.
Musi te� istnie� podana tekstura.
Obiekt tworzy si� na podstawie opisu drzewa TREE_DESC, ten opis kopiuje sobie do
wn�trza i w czasie pracy klasy pozostaje niezmienny.
*/
class Tree
{
public:
	Tree(Scene *OwnerScene, const TREE_DESC &Desc, const TreeTextureDesc &TextureDesc, uint KindCount, uint KindRandomSeeds[]);
	~Tree();

	const TREE_DESC & GetDesc();
	const TreeTextureDesc & GetTextureDesc();
	uint GetKindCount();

	// DirToLight, LightColor - parametry �wiat�a kierunkowego.
	// Je�li DirToLight albo LightColor == NULL, o�wietlenia nie ma.
	// DirToLight musi by� znormalizowany.
	// Je�li FogStart albo FogColor == NULL, mg�a jest wy��czona.
	// Przekazane wska�niki, je�li nie NULL, musz� pozosta� aktualne a� do End.
	void DrawBegin(const ParamsCamera &Cam, const VEC3 *DirToLight, const COLORF *LightColor, const float *FogStart, const COLORF *FogColor);
	void DrawBegin_ShadowMap(const MATRIX &ViewProj, const VEC3 &LightDir);
	void DrawEnd();

	////// Wywo�ywa� mi�dzy DrawBegin a DrawEnd
	// Macierz nie powinna zawiera� zbyt du�ego skalowania, bo b�dzie burak - billboardy li�ci nie b�d� skalowane.
	// InvWorldMat - opcjonalny, je�li nie podany to zostanie wyliczony.
	void DrawTree(uint Kind, const MATRIX &WorldMat, const MATRIX *InvWorldMat, const COLORF &Color);

private:
	scoped_ptr<Tree_pimpl> pimpl;
};

} // namespace engine

#endif
