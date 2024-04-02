/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef ENGINE_PP_H_
#define ENGINE_PP_H_

// C++ Sux!
#include "..\Framework\Res_d3d.hpp"

namespace engine
{
// Abstrakcyjna klasa bazowa dla efekt�w postprocessingu
/* Nie wiem w sumie po co, i tak nie skorzystam tu z polimorfizmu :) */
class PpEffect
{
private:
	res::D3dEffect *m_BlurPassEffect;

// Dla klas potomnych efekt�w
protected:
	void BlurPass(float SampleDistance, IDirect3DSurface9 *Dst, IDirect3DTexture9 *Src, uint4 CX, uint4 CY);

// Dla u�ytkownika (modu� Engine)
public:
	PpEffect() : m_BlurPassEffect(NULL) { }

	// Przerysowanie podanej tekstury na podan� powierzchni�
	// Fullscreen Quad, Pixel Perfect, Fixed Pipeline.
	// Dst=NULL oznacza, �e nie zmieniamy Render Target.
	// - Rysuje na podany prostok�t
	static void Redraw(IDirect3DSurface9 *Dst, IDirect3DTexture9 *Src, const RECTF &DstRect);
	// - Rysuje na ca�y ekran
	static void Redraw(IDirect3DSurface9 *Dst, IDirect3DTexture9 *Src, uint4 DstCX, uint4 DstCY);
	// - Rysuje env map�
	static void RedrawCube(IDirect3DCubeTexture9 *Src, const RECTF &DstRect);

	// Czy�ci kana� alfa bie��cego render targeta rysuj�c Fullscreen Quad, Fixed Pipeline, ColorWriteEnable=ALPHA
	static void ClearAlpha(uint1 Alpha, uint4 ScreenCX, uint4 ScreenCY);
};

// Zamalowanie ca�ego ekranu w p�przezroczysty spos�b na wybrany kolor
/*
- Color.A = 0 oznacza efekt wy��czony.
- Spos�b u�ycia:
  - wywo�a� Draw celem narysowania fullscreen quada na ko�cu renderingu.
*/
class PpColor : public PpEffect
{
private:
	COLOR m_Color;

public:
	PpColor();
	PpColor(COLOR Color);

	// Rysuje Fullscreen Quad na bie��cym Render Target, Fixed Pipeline
	void Draw(uint4 RenderTargetCX, uint4 RenderTargetCY);

	// Ca�y kolor, ��cznie z kana�em alfa
	COLOR GetColor() { return m_Color; }
	void SetColor(COLOR Color) { m_Color = Color; }
	void SetColor(const COLORF &Color) { m_Color = ColorfToColor(Color); }

	// Kolor bez kana�u alfa
	void SetColorNoAlpha(COLOR Color) { m_Color.ARGB = Color.ARGB & 0x00FFFFFF | Color.ARGB & 0xFF000000; }
	
	// Sam kana� alfa
	float GetAlpha() { return m_Color.A / 255.f; }
	void SetAlpha(uint1 Alpha) { m_Color.A = Alpha; }
	void SetAlpha(float Alpha) { m_Color.A = (uint1)((Alpha * 255.f) + 0.5f); }
};

// Zamalowanie ca�ego ekranu p�przezroczyst� tekstur�
/*
- Color.A = 0 lub TextureName = "" oznacza efekt wy��czony.
- Macierz przekszta�ca wsp�rz�dne tekstury, kt�re natywnie s� takie niezale�nie od rozdzielczo�ci i proporcji ekranu:
  (0,0)  ________  (1,0)
        |        |
        |        |
        |________|
  (0,1)            (1,1)
- Spos�b u�ycia:
  - wywo�a� Draw celem narysowania fullscreen quada na ko�cu renderingu.
*/
class PpTexture : public PpEffect
{
private:
	string m_TextureName;
	// NULL je�li TextureName=="" lub je�li wska�nik jeszcze nie pobrany
	res::D3dTexture *m_Texture;
	COLOR m_Color;
	MATRIX m_Matrix;

public:
	PpTexture();
	PpTexture(const string &TextureName, COLOR Color);

	// Rysuje Fullscreen Quad na bie��cym Render Target, Fixed Pipeline
	void Draw(uint4 RenderTargetCX, uint4 RenderTargetCY);

	// Ca�y kolor, ��cznie z kana�em alfa
	COLOR GetColor() { return m_Color; }
	void SetColor(COLOR Color) { m_Color = Color; }
	void SetColor(const COLORF &Color) { m_Color = ColorfToColor(Color); }

	// Kolor bez kana�u alfa
	void SetColorNoAlpha(COLOR Color) { m_Color.ARGB = Color.ARGB & 0x00FFFFFF | Color.ARGB & 0xFF000000; }
	
	// Sam kana� alfa
	float GetAlpha() { return m_Color.A / 255.f; }
	void SetAlpha(uint1 Alpha) { m_Color.A = Alpha; }
	void SetAlpha(float Alpha) { m_Color.A = (uint1)((Alpha * 255.f) + 0.5f); }

	// Tekstura
	const string & GetTextureName() { return m_TextureName; }
	void SetTextureName(const string &TextureName) { m_TextureName = TextureName; m_Texture = NULL; }

	// Macierz
	const MATRIX & GetMatrix() { return m_Matrix; }
	void SetMatrix(const MATRIX &Matrix) { m_Matrix = Matrix; }
};

// Przekszta�cenie ko�cowego rysowanego koloru przez wz�r matematyczny.
/*
- Wz�r to:
  1. Kolor = lerp(Kolor, Kolor_w_grayscale, GrayscaleFactor)
  2. Kolor = Kolor * A_factor + B_factor;
- Proste, a daje ogromne mo�liwo�ci!
- Spos�b u�ycia:
  - Wyrenderowa� scen� do tekstury wielko�ci ekranu.
  - Przerenderowa� t� tekstur� na ekran z u�yciem PpShader.
    Poda� shader managerowi ten obiekt, a on si� zajmie reszt�.
*/
class PpFunction : public PpEffect
{
private:
	float m_GrayscaleFactor;
	VEC3 m_AFactor, m_BFactor;

public:
	PpFunction();
	PpFunction(const VEC3 &AFactor, const VEC3 &BFactor, float GrayscaleFactor);

	float GetGrayscaleFactor() { return m_GrayscaleFactor; }
	void SetGrayscaleFactor(float GrayscaleFactor) { m_GrayscaleFactor = GrayscaleFactor; }

	const VEC3 & GetAFactor() { return m_AFactor; }
	const VEC3 & GetBFactor() { return m_BFactor; }
	void SetAFactor(const VEC3 &AFactor) { m_AFactor = AFactor; }
	void SetBFactor(const VEC3 &BFactor) { m_BFactor = BFactor; }

	// Zwraca true, je�li wsp�czynniki A lub B s� inne ni� domy�lne i trzeba stosowa� funkcj� przekszta�caj�c�
	bool IsLinearTransformRequired() { return (m_AFactor != VEC3(1.f, 1.f, 1.f) || m_BFactor != VEC3::ZERO); }
};

// Tone Mapping - cz�� Fake HDR �ciemniaj�ca scen� zale�nie od wyliczonej �redniej jasno�ci na scenie.
/*
- Spos�b u�ycia:
  - Wyrenderowa� scen� do tekstury.
  - Poda� t� tekstur� do funkcji CalcBrightness, wraz z narz�dziowymi teksturami pomocniczymi.
  - Przerysowa� tekstur� g��wn� na ekran z u�yciem PpShader.
    Poda� shader managerowi ten obiekt, a on zajmie si� reszt�.
*/
class PpToneMapping : public PpEffect
{
private:
	float m_Intensity;
	SmoothCD_obj<float> m_Brightness;
	float m_LastMeasurementTime;
	// Zapami�tane parametry do pr�bkowania
	// - Je�li SampleCX = SampleCY = 0, to nie s� zapami�tane �adne.
	uint4 m_SampleCX;
	uint4 m_SampleCY;
	// - Rozmiar to zawsze TONE_MAPPING_SAMPLE_COUNT
	std::vector< std::pair<uint4, uint4> > m_Samples;

	float MeasureBrightness(res::D3dTextureSurface *RamTexture);

public:
	PpToneMapping(float Intensity = 1.f);

	float GetIntensity() { return m_Intensity; }
	void SetIntensity(float Intensity) { m_Intensity = Intensity; }

	void CalcBrightness(res::D3dTextureSurface *ScreenTexture, res::D3dTextureSurface *ToneMappingVramTexture, res::D3dTextureSurface *ToneMappingRamTexture);
	float GetLastBrighness() { return m_Brightness.Pos; }

	// Czy�ci stan wyg�adzania jasno�ci sprowadzaj�� go natychmiast do docelowego
	void ResetSmooth() { m_Brightness.Pos = m_Brightness.Dest; }
};

// Bloom - cz�� Fake HDR dodaj�ca do sceny rozmyty obraz jasnych miejsc, powoduje rozb�ysk
/*
- Spos�b u�ycia:
  - Render sceny do tekstury g��wnej Screen.
  - Wywo�a� CreateBloom. Rozmyty obraz sceny znajdzie si� w BlurTexture1.
  - Narysowa� tekstur� g��wn� na ekranie. �adne specjalne ustawienia PpShadera nie s� potrzebne.
  - Wywo�a� DrawBloom celem narysowania na ekranie tego co w BlurTexture1.
*/
class PpBloom : public PpEffect
{
private:
	float m_Down;
	float m_Intensity;

	// NULL je�li jeszcze nie pobrany.
	res::D3dEffect *m_BrightPassEffect;

	void BrightPass(IDirect3DSurface9 *Dst, uint4 DstCX, uint4 DstCY, IDirect3DTexture9 *Src);

public:
	PpBloom(float Intensity = 1.f, float Down = 0.5f);

	float GetDown() { return m_Down; }
	float GetIntensity() { return m_Intensity; }
	void SetDown(float Down) { m_Down = Down; }
	void SetIntensity(float Intensity) { m_Intensity = Intensity; }

	void CreateBloom(res::D3dTextureSurface *ScreenTexture, res::D3dTextureSurface *BlurTexture1, res::D3dTextureSurface *BlurTexture2);
	void DrawBloom(res::D3dTextureSurface *BlurTexture1, uint4 ScreenCX, uint4 ScreenCY);
};

// Feedback, czyli sprz�enie zwrotne
/*
- Spos�b u�ycia:
  - Narysowa� scen� na ekranie.
    Je�li rysujemy przez tekstur� to niewa�ne - wszystko ma si� odby� ju� po przerysowaniu na ekran.
  - Wywo�a� Draw podaj�c powierzchni� g��wnego tylnego bufora ekranu oraz tekstur� pomocnicz� Feedback.
    Ta metoda sama spisze z jednej do drugiej w t� i z powrotem.
- Intensity = 0..1, ale mo�na te� poda� wi�cej.
*/
class PpFeedback : public PpEffect
{
public:
	enum MODE
	{
		M_DRUNK,
		M_MOTION_BLUR,
	};
	
private:
	MODE m_Mode;
	float m_Intensity;
	VEC3 m_LastCameraPos;
	bool m_LastCameraPosInited;
	// Zapami�tywana tylko po to by w destruktorze wywo�a� ResetFilled - taki ma�y hack
	res::D3dTextureSurface *m_FeedbackTexture;

	float m_LastWriteTime;

	// Wylicza co jaki czas renderowa� od nowa do tekstury
	float CalcDelay();
	// Wylicza macierz transformacji tekstury
	void CalcTextureMatrix(MATRIX *OutTextureMatrix, const VEC3 &EyePos, const MATRIX &CameraViewProj);

	// Rysuje na ekranie tekstur� Feedback odpowiednio przekszta�con�
	void DrawFeedback(
		res::D3dTextureSurface *FeedbackTexture, uint4 ScreenCX, uint4 ScreenCY,
		const VEC3 &EyePos, const MATRIX &CameraViewProj);

public:
	PpFeedback(MODE Mode, float Intensity = 1.f);
	~PpFeedback();

	void Draw(
		IDirect3DSurface9 *BackBufferSurface, uint4 ScreenCX, uint4 ScreenCY,
		res::D3dTextureSurface *FeedbackTexture,
		const VEC3 &EyePos, const MATRIX &CameraViewProj);

	float GetIntensity() { return m_Intensity; }
	void SetIntensity(float Intensity) { m_Intensity = Intensity; }

	// Resetuje zapami�tan� pozycj� kamery
	// U�ywa� kiedy kamera przeskakuje w zupe�nie inne miejsce, �eby si� nie zrobi� brzydki efekt.
	void ResetCameraPos() { m_LastCameraPosInited = false; }
};

// Lens Flare - odblaski od �wiat�a s�o�ca na soczewce
/*
- Spos�b u�ycia:
  - Korzystaj�c z pozycji kamery rzuci� promienie i sprawdzi� kolizj� z czymkolwiek na odleg�o�� ZFar w kierunku GetDirToLight.
    Zliczy� procent promieni kt�re przesz�y i poda� jako VisibleFactor (0..1).
  - Po wyrenderowaniu obrazu na ekranie wywo�a� Draw.
    Ta funkcja dorysuje na ekranie addytywnie zblendowane flary za pomoc� Fixed Pipeline i specjalnych tekstur.
*/
class PpLensFlare : public PpEffect
{
private:
	VEC3 m_DirToLight;
	SmoothCD_obj<float> m_Intensity;
	float m_CustomIntensity;
	// LensFlare01, LensFlare02, LensFlare03, LensFlare_Halo
	// Zawsze pobrane i za-LOCK-owane.
	res::D3dTexture *m_Textures[4];

	// Pos - pozycja s�o�ca we wsp. jednorodnych po transformacji rzutowania do -1..1
	void DrawLensFlare(const VEC2 &Pos, float Intensity, uint4 ScreenCX, uint4 ScreenCY);

public:
	// Nie musi by� znormalizowany - sam znormalizuje.
	PpLensFlare(const VEC3 &DirToLight);
	~PpLensFlare();

	const VEC3 & GetDirToLight() { return m_DirToLight; }
	void SetDirToLight(const VEC3 &DirToLight);

	// W�asny mno�nik intensywno�ci (0..1, domy�lnie 1).
	float GetCustomIntensity() { return m_CustomIntensity; }
	void SetCustomIntensity(float Intensity) { m_CustomIntensity = Intensity; }

	void Draw(float VisibleFactor, uint4 ScreenCX, uint4 ScreenCY, const MATRIX &CamView, const MATRIX &CamProj);
};

// Usuni�te... :)

} // namespace engine

#endif
