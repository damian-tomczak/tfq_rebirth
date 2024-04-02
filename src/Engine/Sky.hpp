/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef SKY_H_
#define SKY_H_

class ParamsCamera;

namespace engine
{

class Scene;

class SolidSky_pimpl;
class SkyboxSky_pimpl;
class ComplexSky_pimpl;


// Abstrakcyjna klasa bazowa nieba
class BaseSky
{
public:
	virtual ~BaseSky() { }

	// Ma za zadanie od pocz�tku do ko�ca odrysowa� niebo ustawiaj�c wszystkie stany itd.
	// Mo�e korzysta� z Z-bufora je�li chce, po tym i tak Z-bufor jest czyszczony.
	// Ale je�li korzysta z Z-bufora, musi go sobie sama wyczy�ci�.
	virtual void Draw(const ParamsCamera &Cam) = 0;
	// Je�li to niebo ma s�o�ce, zwraca true i przez SunDir znormalizowany kierunek DO s�o�ca
	virtual bool GetSunDir(VEC3 *OutSunDir) = 0;
};

// Zarysowanie ekranu na jednolity kolor - najprostsze mo�liwe t�o
/*
Kana� alfa koloru nie ma znaczenia.
*/
class SolidSky : public BaseSky
{
public:
	SolidSky();
	SolidSky(COLOR Color);
	virtual ~SolidSky();

	COLOR GetColor();
	void SetColor(COLOR Color);

	// ======== Implementacja BaseSky ========
	virtual void Draw(const ParamsCamera &Cam);
	virtual bool GetSunDir(VEC3 *OutSunDir) { return false; }

private:
	scoped_ptr<SolidSky_pimpl> pimpl;
};

// Skybox z�o�ony z 6 tekstur.
/*
Opr�cz tych 6 tekstur u�ywa dodatkowo zasobu efektu "SkyboxEffect".
*/
class SkyboxSky : public BaseSky
{
public:
	enum SIDE
	{
		SIDE_LEFT,
		SIDE_RIGHT,
		SIDE_BOTTOM,
		SIDE_TOP,
		SIDE_NEAR,
		SIDE_FAR
	};

	SkyboxSky();
	SkyboxSky(const string *TextureNames);
	virtual ~SkyboxSky();

	string GetSideTextureName(SIDE Side);
	void SetSideTextureName(SIDE Side, const string &TextureName);
	void SetSideTextureNames(const string *TextureNames);

	COLOR GetColor();
	void SetColor(COLOR Color);

	bool GetSunEnabled();
	void SetSunEnabled(bool SunEnabled);
	const VEC3 & GetSunDir();
	void SetSunDir(const VEC3 &SunDir);

	// ======== Implementacja BaseSky ========
	virtual void Draw(const ParamsCamera &Cam);
	virtual bool GetSunDir(VEC3 *OutSunDir);

private:
	scoped_ptr<SkyboxSky_pimpl> pimpl;
};

// Skomplikowane, realistyczne niebo
/*
- Time:
  - Cz�� u�amkowa to pora dnia, od p�nocy (0) poprzez po�udnie (0.5) do p�nocy dnia nast�pnego (1)
  - Cz�� ca�kowita to numer dnia od chwili 0.
- U�ywa zasob�w: ComplexSkyShader, ComplexSkyCloudsShader, Sky_Stars, Sky_Glow, NoiseTexture
- CloudsThreshold to pr�g od kt�rego wida� chmury, a wi�c jak du�y procent nieba przykrywaj�.
  Zakres: 0..1
  - 0.0 oznacza praktycznie ca�e niebo pokryte chmurami
  - 1.0 oznacza ca�kowity brak chmur
- CloudsSharpness to ostro�� kraw�dzi chmur. Wp�ywa tak�e nieznacznie na ich widoczno��.
  Zakres: 1..INF
  - 1.0 oznacza chmury tak delikatne, �e prawie niewidoczne
  - 10.0 czy 20.0 oznacza ju� chmury bardzo nieprzezroczyste z do�� ostrymi kraw�dziami
*/
class ComplexSky : public BaseSky
{
public:
	struct BACKGROUND_DESC
	{
		// Od 0 (p�noc) poprzez 0.25 (rano), 0.5 (po�udnie), 0.75 (wiecz�r) a� po 1.0 (p�noc)
		// first to kolor horyzontu, second to kolor zenitu.
		RoundInterpolator<COLORF_PAIR> Colors;

		// Wype�nia t� struktur� danymi wczytanymi z podanego tokenizera.
		void LoadFromTokenizer(Tokenizer &t);
	};

	struct CLOUD_COLORS
	{
		// Od 0 (p�noc) poprzez 0.25 (rano), 0.5 (po�udnie), 0.75 (wiecz�r) a� po 1.0 (p�noc)
		RoundInterpolator<COLORF_PAIR> Colors;

		// Wype�nia t� struktur� danymi wczytanymi z podanego tokenizera.
		void LoadFromTokenizer(Tokenizer &t);
	};

	struct CELESTIAL_OBJECT_DESC
	{
		string TextureName;
		// K�t w radianach.
		float Size;
		// Okres obiegu w dobach.
		float Period;
		// Pozycja pocz�tkowa dla czasu 0.
		float Phase;
		// Znormalizowany kierunek prostopad�y do powierzchni okr�gu orbity. NIE MO�E by� pionowy (X=0 i Z=0).
		VEC3 OrbitNormal;
		// Kolor przez kt�ry nale�y mno�y� tekstur� zale�nie od wysoko�ci. Kana� alfa dzia�a.
		COLORF HorizonColor, ZenithColor;
		// Kolor przez kt�ry nale�y mno�y� tekstur� zale�nie od pory dnia. Kana� alfa dzia�a.
		COLORF MidnightColor, MiddayColor;

		// Wype�nia t� struktur� danymi wczytanymi z podanego tokenizera.
		void LoadFromTokenizer(Tokenizer &t);
	};

	// Tworzy nowe niebo zainicjalizowane warto�ciami domy�lnymi
	ComplexSky(Scene *OwnerScene);
	// Tworzy niebo wczytane z formatu tekstowego
	ComplexSky(Scene *OwnerScene, Tokenizer &t);
	virtual ~ComplexSky();

	// Wczytuje ca�� definicj� nieba z podanego tokenizera
	void LoadFromTokenizer(Tokenizer &t);

	// Bie��cy czas
	float GetTime();
	void SetTime(float Time);

	// Kolory t�a
	const BACKGROUND_DESC & GetBackgroundDesc();
	void SetBackgroundDesc(const BACKGROUND_DESC &BackgroundDesc);
	// Dodatkowy mno�nik kolor�w t�a do sterowania zale�nie od pogody.
	const COLORF & GetBackgroundWeatherFactor();
	void SetBackgroundWeatherFactor(const COLORF &BackgroundWeatherFactor);

	// S�o�ce
	// Jest zawsze dok�adnie jedno s�o�ce.
	const CELESTIAL_OBJECT_DESC & GetSunDesc();
	void SetSunDesc(const CELESTIAL_OBJECT_DESC &Desc);
	// Zwraca znormalizowany kierunek DO s�o�ca dla bie��cego czasu
	const VEC3 & GetCurrentSunDir();
	const COLORF & GetCurrentSunColor();

	// Ksi�yce
	// Jest dowolna liczba ksi�yc�w.
	uint GetMoonCount();
	const CELESTIAL_OBJECT_DESC & GetMoonDesc(uint Index);
	void SetMoonDesc(uint Index, CELESTIAL_OBJECT_DESC &Desc);
	void AddMoon(CELESTIAL_OBJECT_DESC &Desc);
	void RemoveMoon(uint Index);
	void ClearMoons();
	// Zwraca znormalizowany kierunek DO danego ksi�yca dla bie��cego czasu
	const VEC3 & GetCurrentMoonDir(uint Index);

	// Chmury
	float GetCloudsThreshold();
	void SetCloudsThreshold(float CloudsThreshold);
	float GetCloudsSharpness();
	void SetCloudsSharpness(float CloudsSharpness);
	const CLOUD_COLORS & GetCloudColors();
	void SetCloudColors(const CLOUD_COLORS &CloudColors);
	// Dodatkowe mno�niki kolor�w chmur do sterowania zale�nie od pogody.
	const COLORF_PAIR & GetCloudColorWeatherFactors();
	void SetCloudColorWeatherFactors(const COLORF_PAIR &CloudColorWeatherFactors);
	float GetCloudsScaleInv();
	void SetCloudsScaleInv(float CloudsScaleInv);

	// Zwraca wyliczony kolor horyzontu/zenitu dla bie�acego czasu
	const COLORF & GetCurrentHorizonColor();
	const COLORF & GetCurrentZenithColor();

	// ======== Implementacja BaseSky ========
	virtual void Draw(const ParamsCamera &Cam);
	virtual bool GetSunDir(VEC3 *OutSunDir);

private:
	scoped_ptr<ComplexSky_pimpl> pimpl;
};


} // namespace engine

#endif
