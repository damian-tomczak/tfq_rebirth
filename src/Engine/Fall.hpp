/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef FALL_H_
#define FALL_H_

class ParamsCamera;

namespace engine
{

class Scene;

// Struktura opisuj¹ca efekt opadów
struct FALL_EFFECT_DESC
{
	// G³ówny kolor do przekolorowania wszystkich rysowanych tekstur
	COLORF MainColor;
	// Tekstura pojedynczej cz¹steczki
	string ParticleTextureName;
	// Czy do rysowania quadów poszczególnych cz¹stek opadów u¿ywaæ prawdziwego wektora kamery do góry?
	// Wpisz true, jeœli to s¹ kwadratowe cz¹steczki, np. œnieg
	// Wpisz false, jeœli to s¹ pod³u¿ne cz¹steczki, np. deszcz
	bool UseRealUpDir;
	// Rozmiary po³ówkowe quada cz¹steczki
	VEC2 ParticleHalfSize;
	// Dwa wektory, ka¿dy obrazuje kierunek spadania cz¹steczek, bez uwzglêdnienia wiatru
	// Wiatr jest do tego dodawany dodatkowo, poprzez œcinanie (Shear).
	// Ka¿da cz¹steczka ma kierunek interpolowany losowo miêdzy tymi dwoma wektorami.
	VEC3 MovementVec1, MovementVec2;
	// Tekstura p³aszczyzny z cz¹steczkami
	string PlaneTextureName;
	// Skalowanie tekstury PlaneTexture
	VEC2 PlaneTexScale;
	// Kolor tekstury szumu
	COLORF NoiseColor;

	FALL_EFFECT_DESC() { }
	FALL_EFFECT_DESC(const COLORF &MainColor, const string &ParticleTextureName, bool UseRealUpDir, const VEC2 &ParticleHalfSize, const VEC3 &MovementVec1, const VEC3 &MovementVec2, const string &PlaneTextureName, const VEC2 &PlaneTexScale, const COLORF &NoiseColor) : MainColor(MainColor), ParticleTextureName(ParticleTextureName), UseRealUpDir(UseRealUpDir), ParticleHalfSize(ParticleHalfSize), MovementVec1(MovementVec1), MovementVec2(MovementVec2), PlaneTextureName(PlaneTextureName), PlaneTexScale(PlaneTexScale), NoiseColor(NoiseColor) { }
};

class Fall_pimpl;

/*
Opadami steruj¹ nastêpuj¹ce parametry:
- EffectDesc - opis efektu
  Mo¿na go zmieniaæ, ale powinien raczej pozostaæ niezmienny.
- Intensity
  Intensywnoœæ opadów 0..1.
U¿ywanie klasy:
- Wype³niæ FALL_EFFECT_DESC.
- Stworzyæ obiekt tej klasy.
- Podaæ go do obiekty klasy Scene do metody SetFall.
*/
class Fall
{
public:
	Fall(Scene *OwnerScene, const FALL_EFFECT_DESC &EffectDesc);
	~Fall();

	const FALL_EFFECT_DESC & GetEffectDesc();
	void SetEffectDesc(const FALL_EFFECT_DESC &EffectDesc);

	float GetIntensity();
	void SetIntensity(float Intensity);

	void Draw(const ParamsCamera &Cam);

private:
	scoped_ptr<Fall_pimpl> pimpl;
};

} // namespace engine

#endif
