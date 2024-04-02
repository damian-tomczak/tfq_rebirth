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

	// Ma za zadanie od pocz¹tku do koñca odrysowaæ niebo ustawiaj¹c wszystkie stany itd.
	// Mo¿e korzystaæ z Z-bufora jeœli chce, po tym i tak Z-bufor jest czyszczony.
	// Ale jeœli korzysta z Z-bufora, musi go sobie sama wyczyœciæ.
	virtual void Draw(const ParamsCamera &Cam) = 0;
	// Jeœli to niebo ma s³oñce, zwraca true i przez SunDir znormalizowany kierunek DO s³oñca
	virtual bool GetSunDir(VEC3 *OutSunDir) = 0;
};

// Zarysowanie ekranu na jednolity kolor - najprostsze mo¿liwe t³o
/*
Kana³ alfa koloru nie ma znaczenia.
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

// Skybox z³o¿ony z 6 tekstur.
/*
Oprócz tych 6 tekstur u¿ywa dodatkowo zasobu efektu "SkyboxEffect".
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
  - Czêœæ u³amkowa to pora dnia, od pó³nocy (0) poprzez po³udnie (0.5) do pó³nocy dnia nastêpnego (1)
  - Czêœæ ca³kowita to numer dnia od chwili 0.
- U¿ywa zasobów: ComplexSkyShader, ComplexSkyCloudsShader, Sky_Stars, Sky_Glow, NoiseTexture
- CloudsThreshold to próg od którego widaæ chmury, a wiêc jak du¿y procent nieba przykrywaj¹.
  Zakres: 0..1
  - 0.0 oznacza praktycznie ca³e niebo pokryte chmurami
  - 1.0 oznacza ca³kowity brak chmur
- CloudsSharpness to ostroœæ krawêdzi chmur. Wp³ywa tak¿e nieznacznie na ich widocznoœæ.
  Zakres: 1..INF
  - 1.0 oznacza chmury tak delikatne, ¿e prawie niewidoczne
  - 10.0 czy 20.0 oznacza ju¿ chmury bardzo nieprzezroczyste z doœæ ostrymi krawêdziami
*/
class ComplexSky : public BaseSky
{
public:
	struct BACKGROUND_DESC
	{
		// Od 0 (pó³noc) poprzez 0.25 (rano), 0.5 (po³udnie), 0.75 (wieczór) a¿ po 1.0 (pó³noc)
		// first to kolor horyzontu, second to kolor zenitu.
		RoundInterpolator<COLORF_PAIR> Colors;

		// Wype³nia t¹ strukturê danymi wczytanymi z podanego tokenizera.
		void LoadFromTokenizer(Tokenizer &t);
	};

	struct CLOUD_COLORS
	{
		// Od 0 (pó³noc) poprzez 0.25 (rano), 0.5 (po³udnie), 0.75 (wieczór) a¿ po 1.0 (pó³noc)
		RoundInterpolator<COLORF_PAIR> Colors;

		// Wype³nia t¹ strukturê danymi wczytanymi z podanego tokenizera.
		void LoadFromTokenizer(Tokenizer &t);
	};

	struct CELESTIAL_OBJECT_DESC
	{
		string TextureName;
		// K¹t w radianach.
		float Size;
		// Okres obiegu w dobach.
		float Period;
		// Pozycja pocz¹tkowa dla czasu 0.
		float Phase;
		// Znormalizowany kierunek prostopad³y do powierzchni okrêgu orbity. NIE MO¯E byæ pionowy (X=0 i Z=0).
		VEC3 OrbitNormal;
		// Kolor przez który nale¿y mno¿yæ teksturê zale¿nie od wysokoœci. Kana³ alfa dzia³a.
		COLORF HorizonColor, ZenithColor;
		// Kolor przez który nale¿y mno¿yæ teksturê zale¿nie od pory dnia. Kana³ alfa dzia³a.
		COLORF MidnightColor, MiddayColor;

		// Wype³nia t¹ strukturê danymi wczytanymi z podanego tokenizera.
		void LoadFromTokenizer(Tokenizer &t);
	};

	// Tworzy nowe niebo zainicjalizowane wartoœciami domyœlnymi
	ComplexSky(Scene *OwnerScene);
	// Tworzy niebo wczytane z formatu tekstowego
	ComplexSky(Scene *OwnerScene, Tokenizer &t);
	virtual ~ComplexSky();

	// Wczytuje ca³¹ definicjê nieba z podanego tokenizera
	void LoadFromTokenizer(Tokenizer &t);

	// Bie¿¹cy czas
	float GetTime();
	void SetTime(float Time);

	// Kolory t³a
	const BACKGROUND_DESC & GetBackgroundDesc();
	void SetBackgroundDesc(const BACKGROUND_DESC &BackgroundDesc);
	// Dodatkowy mno¿nik kolorów t³a do sterowania zale¿nie od pogody.
	const COLORF & GetBackgroundWeatherFactor();
	void SetBackgroundWeatherFactor(const COLORF &BackgroundWeatherFactor);

	// S³oñce
	// Jest zawsze dok³adnie jedno s³oñce.
	const CELESTIAL_OBJECT_DESC & GetSunDesc();
	void SetSunDesc(const CELESTIAL_OBJECT_DESC &Desc);
	// Zwraca znormalizowany kierunek DO s³oñca dla bie¿¹cego czasu
	const VEC3 & GetCurrentSunDir();
	const COLORF & GetCurrentSunColor();

	// Ksiê¿yce
	// Jest dowolna liczba ksiê¿yców.
	uint GetMoonCount();
	const CELESTIAL_OBJECT_DESC & GetMoonDesc(uint Index);
	void SetMoonDesc(uint Index, CELESTIAL_OBJECT_DESC &Desc);
	void AddMoon(CELESTIAL_OBJECT_DESC &Desc);
	void RemoveMoon(uint Index);
	void ClearMoons();
	// Zwraca znormalizowany kierunek DO danego ksiê¿yca dla bie¿¹cego czasu
	const VEC3 & GetCurrentMoonDir(uint Index);

	// Chmury
	float GetCloudsThreshold();
	void SetCloudsThreshold(float CloudsThreshold);
	float GetCloudsSharpness();
	void SetCloudsSharpness(float CloudsSharpness);
	const CLOUD_COLORS & GetCloudColors();
	void SetCloudColors(const CLOUD_COLORS &CloudColors);
	// Dodatkowe mno¿niki kolorów chmur do sterowania zale¿nie od pogody.
	const COLORF_PAIR & GetCloudColorWeatherFactors();
	void SetCloudColorWeatherFactors(const COLORF_PAIR &CloudColorWeatherFactors);
	float GetCloudsScaleInv();
	void SetCloudsScaleInv(float CloudsScaleInv);

	// Zwraca wyliczony kolor horyzontu/zenitu dla bie¿acego czasu
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
