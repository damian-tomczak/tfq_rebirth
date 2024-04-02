/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef MATH_HELP_DRAWING_H_
#define MATH_HELP_DRAWING_H_

class ParamsCamera;

/*
S³u¿y do rysowania obiektów geometrycznych celem ich pokazania, debugowania
itp. Nie jest demonem szybkoœci.
*/
class MathHelpDrawing
{
private:
	struct VERTEX
	{
		VEC3 Pos;
		COLOR Diffuse;

		static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;

		VERTEX() { }
		VERTEX(const VEC3 &Pos, COLOR Diffuse) : Pos(Pos), Diffuse(Diffuse) { }
	};

	struct TEXT_DESC
	{
		VEC3 Pos;
		COLOR Color;
		string Text;

		TEXT_DESC() { }
		TEXT_DESC(const VEC3 &Pos, COLOR Color, const string &Text) : Pos(Pos), Color(Color), Text(Text) { }
	};

	static const float VECTOR_LENGTH;
	static const float ARROW_PART_LENGTH;
	static const uint4 POINT_VERTEX_COUNT;
	static const float POINT_RADIUS;
	static const char * const EFFECT_NAME;
	static const char * const FONT_NAME;
	static const VEC2 TEXT_OFFSET;
	static const float FONT_SIZE;

	const ParamsCamera *m_Camera;

	// Rysowanie zape³nionej kolorem geometii - nieindeksowane Triangle List
	std::vector<VERTEX> m_SolidVertices;
	// Rysowanie linii - nieindeksowane Line List
	std::vector<VERTEX> m_LineVertices;
	// Rysowanie tekstu
	std::vector<TEXT_DESC> m_TextData;
	

public:
	MathHelpDrawing();

	void Begin(const ParamsCamera *Camera);
	void End();

	void DrawPoint(const VEC3 &Pos, COLOR Color) { DrawPoint(Pos, Color, string()); }
	void DrawPoint(const VEC3 &Pos, COLOR Color, const string &Text);

	void DrawLine(const VEC3 &p0, const VEC3 &p1, COLOR Color) { DrawLine(p0, p1, Color, string()); }
	void DrawLine(const VEC3 &p0, const VEC3 &p1, COLOR Color, const string &Text);

	void DrawVector(const VEC3 &Pos, const VEC3 &Dir, COLOR Color) { DrawVector(Pos, Dir, Color, string()); }
	void DrawVector(const VEC3 &Pos, const VEC3 &Dir, COLOR Color, const string &Text);

	void DrawBox(const BOX &Box, COLOR Color) { DrawBox(Box, Color, string()); }
	void DrawBox(const BOX &Box, COLOR Color, const string &Text);

	void DrawSphere(const VEC3 &Center, float Radius, COLOR Color) { DrawSphere(Center, Radius, Color, string()); }
	void DrawSphere(const VEC3 &Center, float Radius, COLOR Color, const string &Text);

	void DrawTriangle(const VEC3 &p1, const VEC3 &p2, const VEC3 &p3, COLOR Color, bool Solid) { DrawTriangle(p1, p2, p3, Color, Solid, string()); }
	void DrawTriangle(const VEC3 &p1, const VEC3 &p2, const VEC3 &p3, COLOR Color, bool Solid, const string &Text);

	void DrawFrustum(const FRUSTUM_POINTS &Frustum, COLOR Color) { DrawFrustum(Frustum, Color, string()); }
	void DrawFrustum(const FRUSTUM_POINTS &Frustum, COLOR Color, const string &Text);

	// P³aszczyzna musi byæ znormalizowana.
	// Density - odleg³oœæ miêdzy oczkami.
	void DrawPlane(const PLANE &Plane, COLOR Color, float Density);
};

extern MathHelpDrawing *g_MathHelpDrawing;


#endif
