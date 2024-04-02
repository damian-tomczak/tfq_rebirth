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
// Abstrakcyjna klasa bazowa dla efektów postprocessingu
/* Nie wiem w sumie po co, i tak nie skorzystam tu z polimorfizmu :) */
class PpEffect
{
private:
	res::D3dEffect *m_BlurPassEffect;

// Dla klas potomnych efektów
protected:
	void BlurPass(float SampleDistance, IDirect3DSurface9 *Dst, IDirect3DTexture9 *Src, uint4 CX, uint4 CY);

// Dla u¿ytkownika (modu³ Engine)
public:
	PpEffect() : m_BlurPassEffect(NULL) { }

	// Przerysowanie podanej tekstury na podan¹ powierzchniê
	// Fullscreen Quad, Pixel Perfect, Fixed Pipeline.
	// Dst=NULL oznacza, ¿e nie zmieniamy Render Target.
	// - Rysuje na podany prostok¹t
	static void Redraw(IDirect3DSurface9 *Dst, IDirect3DTexture9 *Src, const RECTF &DstRect);
	// - Rysuje na ca³y ekran
	static void Redraw(IDirect3DSurface9 *Dst, IDirect3DTexture9 *Src, uint4 DstCX, uint4 DstCY);
	// - Rysuje env mapê
	static void RedrawCube(IDirect3DCubeTexture9 *Src, const RECTF &DstRect);

	// Czyœci kana³ alfa bie¿¹cego render targeta rysuj¹c Fullscreen Quad, Fixed Pipeline, ColorWriteEnable=ALPHA
	static void ClearAlpha(uint1 Alpha, uint4 ScreenCX, uint4 ScreenCY);
};

// Zamalowanie ca³ego ekranu w pó³przezroczysty sposób na wybrany kolor
/*
- Color.A = 0 oznacza efekt wy³¹czony.
- Sposób u¿ycia:
  - wywo³aæ Draw celem narysowania fullscreen quada na koñcu renderingu.
*/
class PpColor : public PpEffect
{
private:
	COLOR m_Color;

public:
	PpColor();
	PpColor(COLOR Color);

	// Rysuje Fullscreen Quad na bie¿¹cym Render Target, Fixed Pipeline
	void Draw(uint4 RenderTargetCX, uint4 RenderTargetCY);

	// Ca³y kolor, ³¹cznie z kana³em alfa
	COLOR GetColor() { return m_Color; }
	void SetColor(COLOR Color) { m_Color = Color; }
	void SetColor(const COLORF &Color) { m_Color = ColorfToColor(Color); }

	// Kolor bez kana³u alfa
	void SetColorNoAlpha(COLOR Color) { m_Color.ARGB = Color.ARGB & 0x00FFFFFF | Color.ARGB & 0xFF000000; }
	
	// Sam kana³ alfa
	float GetAlpha() { return m_Color.A / 255.f; }
	void SetAlpha(uint1 Alpha) { m_Color.A = Alpha; }
	void SetAlpha(float Alpha) { m_Color.A = (uint1)((Alpha * 255.f) + 0.5f); }
};

// Zamalowanie ca³ego ekranu pó³przezroczyst¹ tekstur¹
/*
- Color.A = 0 lub TextureName = "" oznacza efekt wy³¹czony.
- Macierz przekszta³ca wspó³rzêdne tekstury, które natywnie s¹ takie niezale¿nie od rozdzielczoœci i proporcji ekranu:
  (0,0)  ________  (1,0)
        |        |
        |        |
        |________|
  (0,1)            (1,1)
- Sposób u¿ycia:
  - wywo³aæ Draw celem narysowania fullscreen quada na koñcu renderingu.
*/
class PpTexture : public PpEffect
{
private:
	string m_TextureName;
	// NULL jeœli TextureName=="" lub jeœli wskaŸnik jeszcze nie pobrany
	res::D3dTexture *m_Texture;
	COLOR m_Color;
	MATRIX m_Matrix;

public:
	PpTexture();
	PpTexture(const string &TextureName, COLOR Color);

	// Rysuje Fullscreen Quad na bie¿¹cym Render Target, Fixed Pipeline
	void Draw(uint4 RenderTargetCX, uint4 RenderTargetCY);

	// Ca³y kolor, ³¹cznie z kana³em alfa
	COLOR GetColor() { return m_Color; }
	void SetColor(COLOR Color) { m_Color = Color; }
	void SetColor(const COLORF &Color) { m_Color = ColorfToColor(Color); }

	// Kolor bez kana³u alfa
	void SetColorNoAlpha(COLOR Color) { m_Color.ARGB = Color.ARGB & 0x00FFFFFF | Color.ARGB & 0xFF000000; }
	
	// Sam kana³ alfa
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

// Przekszta³cenie koñcowego rysowanego koloru przez wzór matematyczny.
/*
- Wzór to:
  1. Kolor = lerp(Kolor, Kolor_w_grayscale, GrayscaleFactor)
  2. Kolor = Kolor * A_factor + B_factor;
- Proste, a daje ogromne mo¿liwoœci!
- Sposób u¿ycia:
  - Wyrenderowaæ scenê do tekstury wielkoœci ekranu.
  - Przerenderowaæ t¹ teksturê na ekran z u¿yciem PpShader.
    Podaæ shader managerowi ten obiekt, a on siê zajmie reszt¹.
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

	// Zwraca true, jeœli wspó³czynniki A lub B s¹ inne ni¿ domyœlne i trzeba stosowaæ funkcjê przekszta³caj¹c¹
	bool IsLinearTransformRequired() { return (m_AFactor != VEC3(1.f, 1.f, 1.f) || m_BFactor != VEC3::ZERO); }
};

// Tone Mapping - czêœæ Fake HDR œciemniaj¹ca scenê zale¿nie od wyliczonej œredniej jasnoœci na scenie.
/*
- Sposób u¿ycia:
  - Wyrenderowaæ scenê do tekstury.
  - Podaæ t¹ teksturê do funkcji CalcBrightness, wraz z narzêdziowymi teksturami pomocniczymi.
  - Przerysowaæ teksturê g³ówn¹ na ekran z u¿yciem PpShader.
    Podaæ shader managerowi ten obiekt, a on zajmie siê reszt¹.
*/
class PpToneMapping : public PpEffect
{
private:
	float m_Intensity;
	SmoothCD_obj<float> m_Brightness;
	float m_LastMeasurementTime;
	// Zapamiêtane parametry do próbkowania
	// - Jeœli SampleCX = SampleCY = 0, to nie s¹ zapamiêtane ¿adne.
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

	// Czyœci stan wyg³adzania jasnoœci sprowadzaj¹æ go natychmiast do docelowego
	void ResetSmooth() { m_Brightness.Pos = m_Brightness.Dest; }
};

// Bloom - czêœæ Fake HDR dodaj¹ca do sceny rozmyty obraz jasnych miejsc, powoduje rozb³ysk
/*
- Sposób u¿ycia:
  - Render sceny do tekstury g³ównej Screen.
  - Wywo³aæ CreateBloom. Rozmyty obraz sceny znajdzie siê w BlurTexture1.
  - Narysowaæ teksturê g³ówn¹ na ekranie. ¯adne specjalne ustawienia PpShadera nie s¹ potrzebne.
  - Wywo³aæ DrawBloom celem narysowania na ekranie tego co w BlurTexture1.
*/
class PpBloom : public PpEffect
{
private:
	float m_Down;
	float m_Intensity;

	// NULL jeœli jeszcze nie pobrany.
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

// Feedback, czyli sprzê¿enie zwrotne
/*
- Sposób u¿ycia:
  - Narysowaæ scenê na ekranie.
    Jeœli rysujemy przez teksturê to niewa¿ne - wszystko ma siê odbyæ ju¿ po przerysowaniu na ekran.
  - Wywo³aæ Draw podaj¹c powierzchniê g³ównego tylnego bufora ekranu oraz teksturê pomocnicz¹ Feedback.
    Ta metoda sama spisze z jednej do drugiej w t¹ i z powrotem.
- Intensity = 0..1, ale mo¿na te¿ podaæ wiêcej.
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
	// Zapamiêtywana tylko po to by w destruktorze wywo³aæ ResetFilled - taki ma³y hack
	res::D3dTextureSurface *m_FeedbackTexture;

	float m_LastWriteTime;

	// Wylicza co jaki czas renderowaæ od nowa do tekstury
	float CalcDelay();
	// Wylicza macierz transformacji tekstury
	void CalcTextureMatrix(MATRIX *OutTextureMatrix, const VEC3 &EyePos, const MATRIX &CameraViewProj);

	// Rysuje na ekranie teksturê Feedback odpowiednio przekszta³con¹
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

	// Resetuje zapamiêtan¹ pozycjê kamery
	// U¿ywaæ kiedy kamera przeskakuje w zupe³nie inne miejsce, ¿eby siê nie zrobi³ brzydki efekt.
	void ResetCameraPos() { m_LastCameraPosInited = false; }
};

// Lens Flare - odblaski od œwiat³a s³oñca na soczewce
/*
- Sposób u¿ycia:
  - Korzystaj¹c z pozycji kamery rzuciæ promienie i sprawdziæ kolizjê z czymkolwiek na odleg³oœæ ZFar w kierunku GetDirToLight.
    Zliczyæ procent promieni które przesz³y i podaæ jako VisibleFactor (0..1).
  - Po wyrenderowaniu obrazu na ekranie wywo³aæ Draw.
    Ta funkcja dorysuje na ekranie addytywnie zblendowane flary za pomoc¹ Fixed Pipeline i specjalnych tekstur.
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

	// Pos - pozycja s³oñca we wsp. jednorodnych po transformacji rzutowania do -1..1
	void DrawLensFlare(const VEC2 &Pos, float Intensity, uint4 ScreenCX, uint4 ScreenCY);

public:
	// Nie musi byæ znormalizowany - sam znormalizuje.
	PpLensFlare(const VEC3 &DirToLight);
	~PpLensFlare();

	const VEC3 & GetDirToLight() { return m_DirToLight; }
	void SetDirToLight(const VEC3 &DirToLight);

	// W³asny mno¿nik intensywnoœci (0..1, domyœlnie 1).
	float GetCustomIntensity() { return m_CustomIntensity; }
	void SetCustomIntensity(float Intensity) { m_CustomIntensity = Intensity; }

	void Draw(float VisibleFactor, uint4 ScreenCX, uint4 ScreenCY, const MATRIX &CamView, const MATRIX &CamProj);
};

// Usuniête... :)

} // namespace engine

#endif
