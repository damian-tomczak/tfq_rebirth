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

// Struktura opisuj�ca efekt opad�w
struct FALL_EFFECT_DESC
{
	// G��wny kolor do przekolorowania wszystkich rysowanych tekstur
	COLORF MainColor;
	// Tekstura pojedynczej cz�steczki
	string ParticleTextureName;
	// Czy do rysowania quad�w poszczeg�lnych cz�stek opad�w u�ywa� prawdziwego wektora kamery do g�ry?
	// Wpisz true, je�li to s� kwadratowe cz�steczki, np. �nieg
	// Wpisz false, je�li to s� pod�u�ne cz�steczki, np. deszcz
	bool UseRealUpDir;
	// Rozmiary po��wkowe quada cz�steczki
	VEC2 ParticleHalfSize;
	// Dwa wektory, ka�dy obrazuje kierunek spadania cz�steczek, bez uwzgl�dnienia wiatru
	// Wiatr jest do tego dodawany dodatkowo, poprzez �cinanie (Shear).
	// Ka�da cz�steczka ma kierunek interpolowany losowo mi�dzy tymi dwoma wektorami.
	VEC3 MovementVec1, MovementVec2;
	// Tekstura p�aszczyzny z cz�steczkami
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
Opadami steruj� nast�puj�ce parametry:
- EffectDesc - opis efektu
  Mo�na go zmienia�, ale powinien raczej pozosta� niezmienny.
- Intensity
  Intensywno�� opad�w 0..1.
U�ywanie klasy:
- Wype�ni� FALL_EFFECT_DESC.
- Stworzy� obiekt tej klasy.
- Poda� go do obiekty klasy Scene do metody SetFall.
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
