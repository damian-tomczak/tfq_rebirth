/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#pragma once

// Parsuje wektor w postaci: x, y
void ParseVec2(VEC2 *Out, Tokenizer &t);
// Parsuje wektor w postaci: x, y, z
void ParseVec3(VEC3 *Out, Tokenizer &t);
// Parsuje macierz 3x3 w postaci trzech wierszy, w ka�dym: ai1, ai2, ai3;
// Ostatni wiersz i ostatni� kolumn� wype�nia jak w identyczno�ciowej
void ParseMatrix3x3(MATRIX *Out, Tokenizer &t);
// Parsuje macierz 4x4 w postaci czterech wierszy, w ka�dym: ai1, ai2, ai3, ai4;
void ParseMatrix4x4(MATRIX *Out, Tokenizer &t);

// Przekszta�ca punkt lub wektor ze wsp�rz�dnych Blendera do DirectX
void BlenderToDirectxTransform(VEC3 *Out, const VEC3 &In);
void BlenderToDirectxTransform(VEC3 *InOut);
// Przekszta�ca macierz przekszta�caj�c� punkty/wektory z takiej dzia�aj�cej na wsp�rz�dnych Blendera
// na tak� dzia�aj�c� na wsp�rz�dnych DirectX-a (i odwrotnie te�).
void BlenderToDirectxTransform(MATRIX *Out, const MATRIX &In);
void BlenderToDirectxTransform(MATRIX *InOut);
// Sk�ada translacj�, rotacje i skalowanie w macierz w uk�adzie Blendera,
// kt�ra wykonuje te operacje transformuj�c ze wsp. lokalnych obiektu o podanych
// parametrach do wsp. globalnych Blendera
void AssemblyBlenderObjectMatrix(MATRIX *Out, const VEC3 &Loc, const VEC3 &Rot, const VEC3 &Size);

// Zwraca urz�dzenie D3D typu NULLREF. W razie potrzeby tworzy.
IDirect3DDevice9 * EnsureDev();
