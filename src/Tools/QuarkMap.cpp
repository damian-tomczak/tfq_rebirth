/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "Base.hpp"
#include <vector>
#include <map>
#include <iostream>
#include <cassert>
#include <boost/smart_ptr.hpp>
#include "Error.hpp"
#include "Stream.hpp"
#include "Math.hpp"
#include "Map.hpp"
#include "QuarkMap.hpp"

using namespace base;
using namespace math;

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Rzeczy podstawowe

const float EPSILON = 1e-3f;

const VEC3 BEST_AXIS_LOOKUP[] =
{
	VEC3(+1.0f, 0.0f, 0.0f), VEC3(0.0f, 0.0f, +1.0f), VEC3(0.0f, +1.0f, 0.0f),
	VEC3(-1.0f, 0.0f, 0.0f), VEC3(0.0f, 0.0f, -1.0f), VEC3(0.0f, +1.0f, 0.0f),
	VEC3(0.0f, 0.0f, -1.0f), VEC3(+1.0f, 0.0f, 0.0f), VEC3(0.0f, +1.0f, 0.0f),
	VEC3(0.0f, 0.0f, +1.0f), VEC3(-1.0f, 0.0f, 0.0f), VEC3(0.0f, +1.0f, 0.0f),
	VEC3(0.0f, +1.0f, 0.0f), VEC3(+1.0f, 0.0f, 0.0f), VEC3(0.0f, 0.0f, +1.0f),
	VEC3(0.0f, -1.0f, 0.0f), VEC3(+1.0f, 0.0f, 0.0f), VEC3(0.0f, 0.0f, -1.0f),

	// old, stolen from somewhere
	//VEC3(0.0f, 0.0f, 1.0f), VEC3(1.0f, 0.0f, 0.0f), VEC3(0.0f, -1.0f, 0.0f), // Floor
	//VEC3(0.0f, 0.0f, -1.0f), VEC3(1.0f, 0.0f, 0.0f), VEC3(0.0f, -1.0f, 0.0f), // Ceiling
	//VEC3(1.0f, 0.0f, 0.0f), VEC3(0.0f, 1.0f, 0.0f), VEC3(0.0f, 0.0f, -1.0f), // West
	//VEC3(-1.0f, 0.0f, 0.0f), VEC3(0.0f, 1.0f, 0.0f), VEC3(0.0f, 0.0f, -1.0f), // East
	//VEC3(0.0f, 1.0f, 0.0f), VEC3(1.0f, 0.0f, 0.0f), VEC3(0.0f, 0.0f, -1.0f), // South
	//VEC3(0.0f, -1.0f, 0.0f), VEC3(1.0f, 0.0f, 0.0f), VEC3(0.0f, 0.0f, -1.0f), // North
};

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktura QUARK_MAP

struct QUARK_MAP
{
public:
	struct PLANE
	{
		VEC3 v1, v2, v3;
		string tex_name;
		VEC2 tex_scale;
	};

	struct BRUSH
	{
		std::vector<PLANE*> planes;

		~BRUSH();
	};

	struct PROPERTY
	{
		string name;
		string value;

		PROPERTY() { }
		PROPERTY(const string &name, const string &value) : name(name), value(value) { }
	};

	struct ENTITY
	{
		string class_name;
		std::vector<PROPERTY*> properties;

		ENTITY() { }
		ENTITY(const string &class_name) : class_name(class_name) { }
		~ENTITY();

		bool PropertyExists(const string &name)
		{
			for (size_t pi = 0; pi < properties.size(); pi++)
			{
				if (properties[pi]->name == name)
					return true;
			}
			return false;
		}
		string GetProperty(const string &name)
		{
			for (size_t pi = 0; pi < properties.size(); pi++)
			{
				if (properties[pi]->name == name)
					return properties[pi]->value;
			}
			return string();
		}
	};

private:
	enum PARSE_STATE {
		PARSE_NONE, // -> CLASSNAME
		PARSE_WORLDSPAWN, // -> BRUSH, NONE
		PARSE_BRUSH, // -> WORLDSPAWN
		PARSE_CLASSNAME, // -> ENTITY, WORLDSPAWN
		PARSE_ENTITY, // -> NONE
	};

	string parse_doc;
	size_t parse_index;

	bool Parse_EOF();
	bool Parse_GetCh(char *ch);
	bool Parse_Whitespace();
	bool Parse_EOL();
	bool Parse_Particular(char ch);
	bool Parse_Digit(char *d);
	bool Parse_Float(float *x);
	bool Parse_String(string *s, bool quot);
	bool Parse_Comment();
	bool Parse_Vec3(VEC3 *v);

public:
	std::vector<PROPERTY*> properties;
	std::vector<BRUSH*> brushes;
	std::vector<ENTITY*> entities;

	void Load(const string &FileName);
	~QUARK_MAP();
};

QUARK_MAP::BRUSH::~BRUSH()
{
	for (size_t i = 0; i < planes.size(); i++)
		delete planes[i];
}

QUARK_MAP::ENTITY::~ENTITY()
{
	for (size_t i = 0; i < properties.size(); i++)
		delete properties[i];
}

bool QUARK_MAP::Parse_EOF()
{
	return (parse_index >= parse_doc.size());
}

bool QUARK_MAP::Parse_GetCh(char *ch)
{
	if (Parse_EOF())
		return false;
	else
	{
		*ch = parse_doc[parse_index];
		return true;
	}
}

bool QUARK_MAP::Parse_Whitespace()
{
	char ch; bool was = false;
	while (Parse_GetCh(&ch) && (ch == ' ' || ch == '\t'))
	{
		parse_index++; was = true;
	}
	return was;
}

bool QUARK_MAP::Parse_EOL()
{
	char ch; bool was = false;
	if (Parse_GetCh(&ch) && ch == '\r')
	{
		parse_index++; was = true;
	}
	if (Parse_GetCh(&ch) && ch == '\n')
	{
		parse_index++; was = true;
	}
	return was;
}

bool QUARK_MAP::Parse_Particular(char ch)
{
	char ch2;
	if (Parse_GetCh(&ch2) && ch2 == ch)
	{
		parse_index++;
		return true;
	}
	else
		return false;
}

bool QUARK_MAP::Parse_Digit(char *d)
{
	char ch;
	if (Parse_GetCh(&ch) && (ch >= '0' && ch <= '9'))
	{
		parse_index++;
		*d = ch;
		return true;
	}
	else
		return false;
}

bool QUARK_MAP::Parse_Float(float *x)
{
	string s; char ch; bool was_digit = false;

	if (Parse_Particular('-'))
		s += '-';
	else if (Parse_Particular('+'))
		s += '+';
	while (Parse_Digit(&ch))
	{
		s += ch; was_digit = true;
	}
	if (Parse_Particular('.'))
		s += '.';
	while (Parse_Digit(&ch))
	{
		s += ch; was_digit = true;
	}
	if (Parse_Particular('e') || Parse_Particular('E'))
	{
		s += 'e';
		if (Parse_Particular('-'))
			s += '-';
		else if (Parse_Particular('+'))
			s += '+';
		while (Parse_Digit(&ch)) {
			s += ch; was_digit = true;
		}
	}

	if (was_digit)
	{
		*x = StrToFloat(s);
		return true;
	}
	else
		return false;
}

bool QUARK_MAP::Parse_String(string *s, bool quot)
{
	s->clear();
	char ch;

	if (quot) {
		if (Parse_Particular('"'))
		{
			for (;;)
			{
				if (!Parse_GetCh(&ch))
					return false;
				if (ch == '\r' || ch == '\n')
					return false;
				if (ch == '"')
				{
					parse_index++;
					return (!s->empty());
				}
				else
				{
					*s += ch;
					parse_index++;
				}
			}
		}
		else
			return false;
	}

	else
	{
		for (;;)
		{
			if (!Parse_GetCh(&ch))
				return false;
			if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t')
				return (!s->empty());
			else
			{
				*s += ch; parse_index++;
			}
		}
	}
}

bool QUARK_MAP::Parse_Comment()
{
	bool ok = false;

	if (Parse_Particular(';'))
		ok = true;
	else
	{
		size_t index = parse_index;
		if (Parse_Particular('/') && Parse_Particular('/'))
			ok = true;
		else
			parse_index = index;
	}

	if (ok)
	{
		char ch;
		while (Parse_GetCh(&ch) && (ch != '\r' && ch != '\n'))
			parse_index++;
		return true;
	}
	else
		return false;
}

bool QUARK_MAP::Parse_Vec3(VEC3 *v)
{
	size_t index = parse_index;
	VEC3 tmp;

	if (!Parse_Particular('(')) { parse_index = index; return false; }
	Parse_Whitespace();
	if (!Parse_Float(&tmp.x)) { parse_index = index; return false; }
	if (!Parse_Whitespace()) { parse_index = index; return false; }
	if (!Parse_Float(&tmp.y)) { parse_index = index; return false; }
	if (!Parse_Whitespace()) { parse_index = index; return false; }
	if (!Parse_Float(&tmp.z)) { parse_index = index; return false; }
	Parse_Whitespace();
	if (!Parse_Particular(')')) { parse_index = index; return false; }

	// Coordinates system conversion
	v->x = tmp.x;
	v->y = tmp.y;
	v->z = tmp.z;
//	v->x = -tmp.y;
//	v->y = tmp.z;
//	v->z = -tmp.x;

	return true;
}

void QUARK_MAP::Load(const string &FileName)
{
	try
	{
		uint4 line_index = 0;
		PARSE_STATE state = PARSE_NONE;
		ENTITY *entity = 0; // only for PARSE_ENTITY
		BRUSH *brush = 0; // only for PARSE_BRUSH
		PLANE *plane;

		LoadStringFromFile(FileName, &parse_doc);
		parse_index = 0;

		// For each line
		while (!Parse_EOF())
		{
			line_index++;
			// Whitespace at the beginning
			Parse_Whitespace();
			// Empty line
			if ( (Parse_Comment() && Parse_EOL()) || Parse_EOL() )
				continue;
			// Content depending on current state
			if (state == PARSE_NONE)
			{
				if (!Parse_Particular('{'))
					throw Error("Parsing error ('{' expected) in line: "+UintToStr(line_index));
				state = PARSE_CLASSNAME;
			}
			else if (state == PARSE_ENTITY || state == PARSE_CLASSNAME)
			{
				if (state == PARSE_ENTITY && Parse_Particular('}'))
				{
					state = PARSE_NONE;
					entity = 0;
				}
				else
				{
					string name, value;
					if (!Parse_String(&name, true))
						throw Error("Parsing error (quoted string as property name expected) in line: "+UintToStr(line_index));
					if (!Parse_Whitespace())
						throw Error("Parsing error (separating whitespace expected) in line: "+UintToStr(line_index));
					if (!Parse_String(&value, true))
						throw Error("Parsing error (quoted string as property value expected) in line: "+UintToStr(line_index));
					if (state == PARSE_CLASSNAME)
					{
						if (name != "classname") {
							throw Error("Parsing error (\"classname\" expected) in line: "+UintToStr(line_index));
						}
						if (value == "worldspawn")
							state = PARSE_WORLDSPAWN;
						else
						{
							entities.push_back(entity = new ENTITY(value));
							state = PARSE_ENTITY;
						}
					}
					else
					{ // PARSE_ENTITY
						entity->properties.push_back(new PROPERTY(name, value));
					}
				}
			}
			else if (state == PARSE_WORLDSPAWN)
			{
				if (Parse_Particular('{'))
				{
					brushes.push_back(brush = new BRUSH);
					state = PARSE_BRUSH;
				}
				else if (Parse_Particular('}'))
				{
					state = PARSE_NONE;
				}
				else
				{
					string name, value;
					if (!Parse_String(&name, true))
						throw Error("Parsing error (quoted string as property name expected) in line: "+UintToStr(line_index));
					if (!Parse_Whitespace())
						throw Error("Parsing error (separating whitespace expected) in line: "+UintToStr(line_index));
					if (!Parse_String(&value, true))
						throw Error("Parsing error (quoted string as property value expected) in line: "+UintToStr(line_index));
					properties.push_back(new PROPERTY(name, value));
				}
			}
			else
			{ // PARSE_BRUSH
				if (Parse_Particular('}'))
				{
					state = PARSE_WORLDSPAWN;
					brush = 0;
				}
				else
				{
					brush->planes.push_back(plane = new PLANE);
					if (!Parse_Vec3(&plane->v1))
						throw Error("Parsing error (vector 1/3 expected) in line: "+UintToStr(line_index));
					if (!Parse_Whitespace())
						throw Error("Parsing error (separating whitespace expected) in line: "+UintToStr(line_index));
					if (!Parse_Vec3(&plane->v2))
						throw Error("Parsing error (vector 2/3 expected) in line: "+UintToStr(line_index));
					if (!Parse_Whitespace())
						throw Error("Parsing error (separating whitespace expected) in line: "+UintToStr(line_index));
					if (!Parse_Vec3(&plane->v3))
						throw Error("Parsing error (vector 3/3 expected) in line: "+UintToStr(line_index));
					if (!Parse_Whitespace())
						throw Error("Parsing error (separating whitespace expected) in line: "+UintToStr(line_index));
					// texture name
					if (!Parse_String(&plane->tex_name, false))
						throw Error("Parsing error (texture name expected) in line: "+UintToStr(line_index));
					Parse_Whitespace();
					float floats[11];
					// model 1:
					// [ tx1 ty1 tz1 toffs1 ] [ tx2 ty2 tz2 toffs2 ] rotation scaleX scaleY
					if (Parse_Particular('['))
					{
						// first [ ] section
						Parse_Whitespace();
						if (!Parse_Float(&floats[0])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[1])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[2])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[3])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Particular(']')) { throw Error("Parsing error (']' expected) in line: "+UintToStr(line_index)); }
						// second [ ] section
						Parse_Whitespace();
						if (!Parse_Particular('[')) { throw Error("Parsing error ('[' expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[4])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[5])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[6])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[7])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Particular(']')) { throw Error("Parsing error (']' expected) in line: "+UintToStr(line_index)); }
						// last 3 numbers
						Parse_Whitespace();
						if (!Parse_Float(&floats[8])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[9])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[10])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }

						plane->tex_scale.x = floats[9];
						plane->tex_scale.y = floats[10];
					}
					// model 2:
					// toffs1 toffs2 rotation scaleX scaleY
					// model 3:
					// toffs1 toffs2 rotation scaleX scaleY ? ? ?
					else
					{
						// 5 numbers
						if (!Parse_Float(&floats[0])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[1])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[2])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[3])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						Parse_Whitespace();
						if (!Parse_Float(&floats[4])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						// optional 3 numbers
						Parse_Whitespace();
						if (Parse_Float(&floats[5]))
						{
							Parse_Whitespace();
							if (!Parse_Float(&floats[6])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
							Parse_Whitespace();
							if (!Parse_Float(&floats[7])) { throw Error("Parsing error (float expected) in line: "+UintToStr(line_index)); }
						}

						plane->tex_scale.x = floats[3];
						plane->tex_scale.y = floats[4];
					}
				}
			}
			// Whitespace at the end
			Parse_Whitespace();
			// Commend at the end
			Parse_Comment();
			// EOL
			if (!Parse_EOL() && !Parse_EOF())
				throw Error("Parsing error (end of line expected) in line: "+UintToStr(line_index));
		}
	}
	catch (Error &e)
	{
		e.Push("Cannot load map from file: " + FileName);
		throw;
	}
}

QUARK_MAP::~QUARK_MAP()
{
	size_t i;
	for (i = 0; i < brushes.size(); i++)
		delete brushes[i];
	for (i = 0; i < entities.size(); i++)
		delete entities[i];
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktura INTERMEDIATE_MAP

struct INTERMEDIATE_MAP
{
public:
	struct FACE
	{
		std::vector<VEC3> Points;
		std::vector<VEC2> TexCoords;
		PLANE Plane;
		string MaterialName;
		VEC3 Tangent;
		VEC3 Binormal;
		VEC3 Normal;
	};

	struct BRUSH
	{
		std::vector<FACE*> Faces;
		~BRUSH();
	};

private:

public:
	std::vector<BRUSH*> Brushes;

	~INTERMEDIATE_MAP();
	uint4 GetFaceCount();
	uint4 GetPointCount();
};

INTERMEDIATE_MAP::BRUSH::~BRUSH()
{
	for (size_t i = 0; i < Faces.size(); i++)
		delete Faces[i];
}

INTERMEDIATE_MAP::~INTERMEDIATE_MAP()
{
	for (size_t i = 0; i < Brushes.size(); i++)
		delete Brushes[i];
}

uint4 INTERMEDIATE_MAP::GetFaceCount()
{
	uint4 R = 0;
	for (size_t i = 0; i < Brushes.size(); i++)
		R += Brushes[i]->Faces.size();
	return R;
}

uint4 INTERMEDIATE_MAP::GetPointCount()
{
	uint4 R = 0;
	size_t bi, fi;
	for (bi = 0; bi < Brushes.size(); bi++)
		for (fi = 0; fi < Brushes[bi]->Faces.size(); fi++)
			R += Brushes[bi]->Faces[fi]->Points.size();
	return R;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktura MapScript

struct MapScript
{
public:
	struct Material
	{
		string m_OutputName;
		float m_TexScale;
		bool m_Delete; // wtedy reszta pól niewa¿na
	};
	// owzorowanie InputName na resztê danych
	typedef std::map<string, Material> MATERIAL_MAP;

private:
	void LoadMaterials(xnl::Reader &x);

public:
	float m_Scale;
	float m_TexScale;
	MATERIAL_MAP m_Materials;

	MapScript();
	void Load(const string &FileName);
	// zwraca materia³ o podanej nazwie wejœiowej lub 0 jeœli nie znaleziono
	const Material * FindMaterial(const string &InputName);
};

MapScript *g_Script = 0;

MapScript::MapScript() : m_Scale(1.0f), m_TexScale(1.0f)
{
}

void MapScript::LoadMaterials(xnl::Reader &x)
{
	string s;

	x.MustReadSectionBegin();
	while (!x.ReadSectionEnd())
	{
		string InputMatName;
		Material mat = {"", 1.0, false};

		x.MustReadElement(&InputMatName);
		x.MustReadAttrSeparator();
		x.MustReadElement(&s);
		// materia³ usuwany
		if (s == "del")
			mat.m_Delete = true;
		// inny, normalny materia³
		else
		{
			mat.m_OutputName = s;
			// dodatkowe opcje
			if (x.ReadSectionBegin())
			{
				while (!x.ReadSectionEnd())
				{
					x.MustReadElement(&s);
					x.MustReadAttrSeparator();
					if (s == "TexScale")
					{
						x.MustReadElement(&s);
						mat.m_TexScale = StrToFloat(s);
					}
					else
						throw Error("B³êdna opcja materia³u: "+s, __FILE__, __LINE__);
				}
			}
		}

		m_Materials.insert(std::make_pair(InputMatName, mat));
	}
}

void MapScript::Load(const string &FileName)
{
	ERR_TRY;

	string Doc;
	LoadStringFromFile(FileName, &Doc);

	try
	{
		string s;
		xnl::Reader x(&Doc);
		
		// nag³ówek
		x.MustReadElement(&s);
		if (s != "MapScript") throw Error("B³êdny nag³ówek pliku", __FILE__, __LINE__);
		x.MustReadElement(&s);
		if (s != "1") throw Error("B³êdna wersja pliku", __FILE__, __LINE__);

		// treœæ
		while (!x.QueryEOF())
		{
			x.MustReadElement(&s);
			x.MustReadAttrSeparator();
			if (s == "Scale")
			{
				x.MustReadElement(&s);
				m_Scale = StrToFloat(s);
			}
			else if (s == "TexScale")
			{
				x.MustReadElement(&s);
				m_TexScale = StrToFloat(s);
			}
			else if (s == "Materials")
				LoadMaterials(x);
			else
				throw Error("B³êdny atrybut: "+s);
		}
	}
	catch (xnl::Error &e)
	{
		throw XNL2(e, "B³¹d podczas przetwarzania skryptu kompilacji mapy", __FILE__, __LINE__);
	}

	ERR_CATCH("Nie mo¿na wczytaæ skryptu kompilacji mapy z pliku: "+FileName);
}

const MapScript::Material * MapScript::FindMaterial(const string &InputName)
{
	MATERIAL_MAP::iterator it = m_Materials.find(InputName);
	if (it == m_Materials.end())
		return 0;
	else
		return &it->second;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Kompilacja etap 1

class PointSorter {
private:
	VEC3 m_Normal;
	VEC3 m_Origin;
	VEC3 m_Base;
public:
	PointSorter(const VEC3 &normal, const VEC3 &origin, const VEC3 &base): m_Normal(normal), m_Origin(origin)
	{
		m_Base = base - m_Origin;
		Normalize(&m_Base);
	}
	bool operator() (const VEC3 &v1, const VEC3 &v2);
};

bool PointSorter::operator() (const VEC3 &v1, const VEC3 &v2)
{
	VEC3 w1 = v1 - m_Origin;
	VEC3 w2 = v2 - m_Origin;
	Normalize(&w1);
	Normalize(&w2);

	float dot1 = Dot(m_Base, w1);
	float dot2 = Dot(m_Base, w2);

	VEC3 tmp;
	Cross(&tmp, m_Base, w1);
	if (Dot(tmp, m_Normal) < 0.0f)
		dot1 = PI_X_2 - dot1;
	Cross(&tmp, m_Base, w2);
	if (Dot(tmp, m_Normal) < 0.0f)
		dot2 = PI_X_2 - dot2;

	return (dot1 > dot2);
}

bool InTheWorld(const VEC3 &pos)
{
	// magic number :P
	static const float WORLD = 1.0e5f;

	if (pos.x > +WORLD || pos.x < -WORLD) return false;
	if (pos.y > +WORLD || pos.y < -WORLD) return false;
	if (pos.z > +WORLD || pos.z < -WORLD) return false;
	return true;
}

VEC3 MakeOrigin(const std::vector<VEC3> &Vectors)
{
	VEC3 o = VEC3(0.0f, 0.0f, 0.0f);
	for (size_t vi = 0; vi < Vectors.size(); vi++)
	{
		o += Vectors[vi];
	}
	return o * (1.0f / Vectors.size());
}

void SortVectors(std::vector<VEC3> *Vectors, const VEC3 &N)
{
	std::sort(Vectors->begin(), Vectors->end(), PointSorter(N, MakeOrigin(*Vectors), (*Vectors)[0]));
}

void ComputeBestAxes(const VEC3 &PlaneNormal, VEC3 *tex_u, VEC3 *tex_v)
{
	int iBestAxis = 0;
	float fDot, fBest = 0.0f;

	for (int i = 0; i < 6; i++)
	{
		fDot = Dot(PlaneNormal, BEST_AXIS_LOOKUP[i*3]);
		if (fDot > fBest)
		{
			fBest = fDot;
			iBestAxis = i*3;
		}
	}

	*tex_u = BEST_AXIS_LOOKUP[++iBestAxis];
	*tex_v = BEST_AXIS_LOOKUP[++iBestAxis];
}

void ComputeTexCoords(INTERMEDIATE_MAP::FACE *face, const MapScript::Material *script_material)
{
	// Wersja uwzglêdniaj¹ca translacjê i skalowanie z QuArKa

	//// TODO: TEMP
	//float tex_width = 256.0f;
	//float tex_height = 256.0f;

	//float W = 1.0f / tex_width;
	//float H = 1.0f / tex_height;
	//assert(map_plane.tex_scale.x != 0.0f);
	//assert(map_plane.tex_scale.y != 0.0f);
	//float SX = 1.0f / map_plane.tex_scale.x;
	//float SY = 1.0f / map_plane.tex_scale.y;

	//VEC2 offset;

	//VEC3 tex_u, tex_v;
	//VEC3 plane_normal = face->Plane.GetNormal();
	//ComputeBestAxes(plane_normal, &tex_u, &tex_v);

	//face->TexCoords.resize(face->Points.size());

	//for (size_t pi = 0; pi < face->Points.size(); pi++)
	//{
	//	face->TexCoords[pi].x = (Dot(face->Points[pi], tex_u) * W * SX);
	//	face->TexCoords[pi].y = (Dot(face->Points[pi], tex_v) * H * SY);
	//	if (pi == 0)
	//	{
	//		offset.x = floorf(face->TexCoords[pi].x);
	//		offset.y = floorf(face->TexCoords[pi].y);
	//	}
	//	face->TexCoords[pi] -= offset;
	//}

	// moja wersja, nieuwzglêdniaj¹ca nic z QuArKa, sama mapuje, tak jak u Krzyœka K.

	assert(face->Points.size() > 2);

	// tex coords
	VEC2 offset;
	VEC3 tex_u, tex_v;
	VEC3 plane_normal = face->Plane.GetNormal();
	ComputeBestAxes(plane_normal, &tex_u, &tex_v);

	face->TexCoords.resize(face->Points.size());
	for (size_t pi = 0; pi < face->Points.size(); pi++)
	{
		face->TexCoords[pi].x = (Dot(face->Points[pi], tex_u) * g_Script->m_TexScale * script_material->m_TexScale);
		face->TexCoords[pi].y = (Dot(face->Points[pi], tex_v) * g_Script->m_TexScale * script_material->m_TexScale);
		if (pi == 0)
		{
			offset.x = floorf(face->TexCoords[pi].x);
			offset.y = floorf(face->TexCoords[pi].y);
		}
		face->TexCoords[pi] -= offset;
	}
}

void ComputeVectors(INTERMEDIATE_MAP::FACE *face)
{
	// Te magiczne wzory dosta³em od Skalniaka, który z kolei dosta³ je od Krzyœka K. - pozdro :)

	float s1 = -(face->TexCoords[1].x-face->TexCoords[0].x);
	float s2 = -(face->TexCoords[2].x-face->TexCoords[0].x);
	float t1 = -(face->TexCoords[1].y-face->TexCoords[0].y);
	float t2 = -(face->TexCoords[2].y-face->TexCoords[0].y);

	VEC3 v1, v2;
	v1 = face->Points[1] - face->Points[0];
	v2 = face->Points[2] - face->Points[0];

	face->Tangent = v1*t2 - v2*t1;
	face->Binormal = v2*s1 - v1*s2;

	// Krzysiek K. powiada: ma byæ wersja 1, czyli z p³aszczyzny, a nie z cross productu tych dwóch.
	// Powód: Bo nie mo¿emy czyniæ za³o¿eñ co do skrêtnoœci tego uk³adu, tekstura mo¿e byæ na³o¿ona
	// prosto lub odbita i by normalna mia³a przeciwny zwrot.

	// normal - version 1 (from plane)
	face->Normal = face->Plane.GetNormal();
	// normal - version 2 (from tangent and binormal)
	// minus or not minus, this is the question...
//	face->Normal = -VecCross(face->Tangent, face->Binormal);
//	Normalize(&face->Normal);
	// additional option for version 2 - has no sense, but I've tried to fix some problems with this :)
//	if (Dot(face->Normal, face->Plane.GetNormal()) < 0.0f)
//		face->Normal = -face->Normal;

	// From Krzysiek K.: ¿eby tangent by³ prostopad³y do normalnej
	face->Tangent -= face->Normal * Dot(face->Normal, face->Tangent);
	Normalize(&face->Tangent);


	// check
//	assert(around(VecCross(face->Tangent, face->Binormal), face->Plane.GetNormal(), 1e-3f) ||
//		around(VecCross(face->Tangent, face->Binormal), -face->Plane.GetNormal(), 1e-3f));
}

void OrthogonalizeVectors(INTERMEDIATE_MAP::FACE *face)
{
	if(Length(face->Normal) <= 0.00001f)
		face->Normal = VEC3(0,0,1);

	assert( Length(face->Normal) > 0.00001f && 
		"found zero length normal when calculating tangent basis!,\
		if you are not using mesh mender to compute normals, you\
		must still pass in valid normals to be used when calculating\
		tangents and binormals.");

	//now with T and B and N we can get from tangent space to object space
	//but we want to go the other way, so we need the inverse
	//of the T, B,N matrix
	//we can use the Gram-Schmidt algorithm to find the newTangent and the newBinormal
	//newT = T - (N dot T)N
	//newB = B - (N dot B)N - (newT dot B)newT

	//NOTE: this should maybe happen with the final smoothed N, T, and B
	//will try it here and see what the results look like

	VEC3 tmpTan = face->Tangent;
	VEC3 tmpNorm = face->Normal;
	VEC3 tmpBin = face->Binormal;


	VEC3 newT = tmpTan - (tmpNorm * Dot(tmpNorm, tmpTan) );
	VEC3 newB = tmpBin - (tmpNorm * Dot(tmpNorm, tmpBin) )
						- (newT * Dot(newT, tmpBin) );

	Normalize(&face->Tangent, newT);
	Normalize(&face->Binormal, newB);		

	//this is where we can do a final check for zero length vectors
	//and set them to something appropriate
	float lenTan = Length(face->Tangent);
	float lenBin = Length(face->Binormal);

	if( (lenTan <= 0.001f) || (lenBin <= 0.001f)  ) //should be approx 1.0f
	{	
		//the tangent space is ill defined at this vertex
		//so we can generate a valid one based on the normal vector,
		//which I'm assuming is valid!

		if(lenTan > 0.5f)
		{
			//the tangent is valid, so we can just use that
			//to calculate the binormal
			Cross(&face->Binormal, face->Normal, face->Tangent);

		}
		else if(lenBin > 0.5)
		{
			//the binormal is good and we can use it to calculate
			//the tangent
			Cross(&face->Tangent, face->Binormal, face->Normal);
		}
		else
		{
			//both vectors are invalid, so we should create something
			//that is at least valid if not correct
			VEC3 xAxis( 1.0f , 0.0f , 0.0f);
			VEC3 yAxis( 0.0f , 1.0f , 0.0f);
			//I'm checking two possible axis, because the normal could be one of them,
			//and we want to chose a different one to start making our valid basis.
			//I can find out which is further away from it by checking the dot product
			VEC3 startAxis;

			if( Dot(xAxis, face->Normal) < Dot(yAxis, face->Normal) )
			{
				//the xAxis is more different than the yAxis when compared to the normal
				startAxis = xAxis;
			}
			else
			{
				//the yAxis is more different than the xAxis when compared to the normal
				startAxis = yAxis;
			}

			Cross(&face->Tangent, face->Normal, startAxis);
			Cross(&face->Binormal, face->Normal, face->Tangent);

		}
	}
	else
	{
		//one final sanity check, make sure that they tangent and binormal are different enough
		if( Dot(face->Binormal, face->Tangent)  > 0.999f )
		{
			//then they are too similar lets make them more different
			Cross(&face->Binormal, face->Normal, face->Tangent);
		}
	}
}

void Q_to_I_Points(const QUARK_MAP::BRUSH &qbrush, INTERMEDIATE_MAP::BRUSH &ibrush)
{
	size_t fi, pi, pi_i, pi_j, pi_k, pi_l, pti;
	VEC3 normal, pos;
	bool legal, was;
	size_t main_count = qbrush.planes.size();

	// Create an array list of faces, each face has an array list of vectors
	ibrush.Faces.resize(qbrush.planes.size());
	for (fi = 0; fi < main_count; fi++)
		ibrush.Faces[fi] = new INTERMEDIATE_MAP::FACE;

	// Convert each plane in the brush from three vectors to ax + by + cz + d = 0 format
	PLANE plane_tmp;
	for (pi = 0; pi < qbrush.planes.size(); pi++)
	{
		PointsToPlane(&plane_tmp, qbrush.planes[pi]->v1, qbrush.planes[pi]->v2, qbrush.planes[pi]->v3);
		Normalize(&ibrush.Faces[pi]->Plane, plane_tmp);
	}

	// Cycle through all plane combinations
	for (pi_i = 0; pi_i < main_count; pi_i++)
	{
		for (pi_j = pi_i+1; pi_j < main_count; pi_j++)
		{
			for (pi_k = pi_j+1; pi_k < main_count; pi_k++)
			{
				// If none of the planes are the same
				if(pi_i != pi_j && pi_i != pi_k && pi_j != pi_k)
				{
					// Jeœli te 3 p³aszyczny przecinaj¹ siê i punkt przeciêcia jest w tym œwiecie a nie gdzieœ hen daleko
					if (Intersect3Planes(ibrush.Faces[pi_i]->Plane, ibrush.Faces[pi_j]->Plane, ibrush.Faces[pi_k]->Plane, &pos) && InTheWorld(pos))
					{
						legal = true;
						// Make sure the new vector is in the brush
						for (pi_l = 0; pi_l < main_count; pi_l++)
						{
							// Don't check the planes that make up the vector
							if (pi_l != pi_i && pi_l != pi_j && pi_l != pi_k)
							{
								if (DotCoord(ibrush.Faces[pi_l]->Plane, pos) < -EPSILON)
								{
									legal = false;
									break;
								}
							}
						}
						// If it is in the brush add the vector to the face's of the three planes
						// that made the vector if the vector is not already in that face
						if (legal)
						{
							// i
							was = false;
							for (pti = 0; pti < ibrush.Faces[pi_i]->Points.size(); pti++)
							{
								if (around(ibrush.Faces[pi_i]->Points[pti], pos, EPSILON))
								{
									was = true;
									break;
								}
							}
							if (!was)
								ibrush.Faces[pi_i]->Points.push_back(pos);
							// j
							was = false;
							for (pti = 0; pti < ibrush.Faces[pi_j]->Points.size(); pti++)
							{
								if (around(ibrush.Faces[pi_j]->Points[pti], pos, EPSILON))
								{
									was = true;
									break;
								}
							}
							if (!was)
								ibrush.Faces[pi_j]->Points.push_back(pos);
							// i
							was = false;
							for (pti = 0; pti < ibrush.Faces[pi_k]->Points.size(); pti++)
							{
								if (around(ibrush.Faces[pi_k]->Points[pti], pos, EPSILON))
								{
									was = true;
									break;
								}
							}
							if (!was)
								ibrush.Faces[pi_k]->Points.push_back(pos);
						}
					}
				}
 			}
		}
	}

	// Get the origin of the brush by averaging all vectors in all faces
	VEC3 origin = VEC3(0.0f, 0.0f, 0.0f);
	{
		float count = 0.0f;
		for (fi = 0; fi < main_count; fi++)
		{
			for (pti = 0; pti < ibrush.Faces[fi]->Points.size(); pti++)
			{
				origin += ibrush.Faces[fi]->Points[pti];
				count += 1.0f;
			}
		}
		origin *= (1.0f / count);
	}

	// Sort vectors
	for (fi = 0; fi < main_count; fi++)
		SortVectors(&ibrush.Faces[fi]->Points, ibrush.Faces[fi]->Plane.GetNormal());
}

void ComputePlane(INTERMEDIATE_MAP::FACE &iface)
{
	PLANE plane_tmp;
	PointsToPlane(&plane_tmp, iface.Points[0], iface.Points[1], iface.Points[2]);
	Normalize(&iface.Plane, plane_tmp);
}

void QBrushToIBrush(const QUARK_MAP::BRUSH &qbrush, INTERMEDIATE_MAP::BRUSH &ibrush)
{
	// utwórz œcianki, oblicz wspó³rzêdne punktów
	Q_to_I_Points(qbrush, ibrush);

	size_t FaceCount = qbrush.planes.size();
	size_t qfi, ifi, pti;
	INTERMEDIATE_MAP::FACE *iface;

	const MapScript::Material *script_material;

	// dla ka¿dej œcianki
	for (qfi = 0, ifi = 0; qfi < qbrush.planes.size(); qfi++)
	{
		// znajdŸ jej materia³
		script_material = g_Script->FindMaterial(qbrush.planes[qfi]->tex_name);

		// jeœli materia³u nie znaleziono - b³¹d
		if (script_material == 0)
			throw Error("Nieznany materia³: " + qbrush.planes[qfi]->tex_name);

		// jeœli materia³ do usuniêcia - usuñ t¹ œciankê
		if (script_material->m_Delete)
		{
			delete ibrush.Faces[ifi];
			ibrush.Faces.erase(ibrush.Faces.begin()+ifi);
		}
		else
		{
			iface = ibrush.Faces[ifi];
			// dla ka¿dego wierzcho³ka
			for (pti = 0; pti < iface->Points.size(); pti++)
			{
				// skonwertuj uk³ad wspó³rzêdnych
				iface->Points[pti] = VEC3(
					-iface->Points[pti].y,
					+iface->Points[pti].z,
					+iface->Points[pti].x);
				// przeskaluj wierzcho³ki
				iface->Points[pti] *= g_Script->m_Scale;
			}
			// oblicz p³aszczyznê
			ComputePlane(*iface);
			// przypisz materia³
			iface->MaterialName = script_material->m_OutputName;
			// utwórz i oblicz wspó³rzêdne tekstury
			ComputeTexCoords(iface, script_material);
			// oblicz normal, tangent i binormal
			ComputeVectors(iface);
			// Krzysiek K. powiada: wektory nie musz¹ byæ nawet prostopad³e do siebie!
			// Choæ przyda³oby siê ¿eby by³y prostopad³e do normalnej (jak to zrobiæ???).
	//		OrthogonalizeVectors(iface);

			ifi++;
		}
	}
}

void Compile1(const QUARK_MAP &qmap, INTERMEDIATE_MAP &imap)
{
	INTERMEDIATE_MAP::BRUSH *ibrush;

	for (size_t qbi = 0; qbi < qmap.brushes.size(); qbi++)
	{
		ibrush = new INTERMEDIATE_MAP::BRUSH;
		QBrushToIBrush(*qmap.brushes[qbi], *ibrush);

		if (ibrush->Faces.size() > 0)
			imap.Brushes.push_back(ibrush);
		else
			delete ibrush;
	}
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Kompilacja etap 2

void Compile2(const INTERMEDIATE_MAP &imap, COMPILED_MAP &cmap)
{
	size_t ibi, ifi, ipi, cpi;
	INTERMEDIATE_MAP::FACE *iface;
	COMPILED_MAP::MATERIAL_MAP::iterator cmat_it;
	COMPILED_MAP::MATERIAL *cmat;
	COMPILED_MAP::VERTEX cv;
	VEC3 normal;

	// dla ka¿dej bry³y imap
	for (ibi = 0; ibi < imap.Brushes.size(); ibi++)
	{
		// dla ka¿dej œcianki tej bry³y
		for (ifi = 0; ifi < imap.Brushes[ibi]->Faces.size(); ifi++)
		{
			iface = imap.Brushes[ibi]->Faces[ifi];

			// normalna tej œcianki
			normal = iface->Normal;

			// wybierz materia³, do którego trafi (jeœli nie ma to stwórz)
			cmat_it = cmap.Materials.find(iface->MaterialName);
			if (cmat_it == cmap.Materials.end())
			{
				cmap.Materials.insert(std::make_pair(iface->MaterialName, cmat = new COMPILED_MAP::MATERIAL));
				cmat->MaterialName = iface->MaterialName;
			}
			else
				cmat = cmat_it->second;

			// u³ó¿ z wielok¹ta œciany trójk¹ty i indeksy

			// TODO: To mo¿na zrobiæ lepiej, ¿eby wierzcho³ki le¿¹ce w tym samym
			// brushu (albo w ogóle, ale to siê raczej nie zdarzy) i maj¹ce ten
			// sam materia³ i w tym samym miejscu i z tymi samymi wsp. tekstury
			// zapisywaæ tylko raz.

			// sciana musi miec co najmniej 3 wierzcholki
			assert(iface->Points.size() >= 3);
			// kazda wspolrzedna tekstury musi odpowiadac wierzcholkowi
			assert(iface->Points.size() == iface->TexCoords.size());

			// pierwszy trojkat dodam recznie
			cmat->Indices.push_back(cpi = (uint2)cmat->Vertices.size());
			cv.Pos = iface->Points[0];
			cv.Normal = normal;
			cv.Tex = iface->TexCoords[0];
			cv.Tangent = iface->Tangent;
			cv.Binormal = iface->Binormal;
			cmat->Vertices.push_back(cv);

			cmat->Indices.push_back((uint2)cmat->Vertices.size());
			cv.Pos = iface->Points[1];
			cv.Normal = normal;
			cv.Tex = iface->TexCoords[1];
			cv.Tangent = iface->Tangent;
			cv.Binormal = iface->Binormal;
			cmat->Vertices.push_back(cv);

			cmat->Indices.push_back((uint2)cmat->Vertices.size());
			cv.Pos = iface->Points[2];
			cv.Normal = normal;
			cv.Tex = iface->TexCoords[2];
			cv.Tangent = iface->Tangent;
			cv.Binormal = iface->Binormal;
			cmat->Vertices.push_back(cv);

			// ka¿dy nastêpny tworzy trójk¹t z poprzednim i z pierwszym
			for (ipi = 3; ipi < iface->Points.size(); ipi++)
			{
				cmat->Indices.push_back(cpi);
				cmat->Indices.push_back((uint2)cmat->Vertices.size()-1);
				cmat->Indices.push_back((uint2)cmat->Vertices.size()-0);

				cv.Pos = iface->Points[ipi];
				cv.Tex = iface->TexCoords[ipi];
				cv.Tangent = iface->Tangent;
				cv.Binormal = iface->Binormal;
				cmat->Vertices.push_back(cv);
			}
		}
	}
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Publiczne

void CompileMap(const QUARK_MAP &qmap, COMPILED_MAP &cmap)
{
	INTERMEDIATE_MAP imap;

	std::cout << "Compilation (stage 1)..." << std::endl;
	Compile1(qmap, imap);
	std::cout << (Format("  Compiled. Brushes=#, Faces=#, Points=#") %
		imap.Brushes.size() %
		imap.GetFaceCount() %
		imap.GetPointCount()).str() << std::endl;

	std::cout << "Compilation (stage 2)..." << std::endl;
	Compile2(imap, cmap);
	std::cout << (Format("  Compiled. Materials=#, Vertices=#, Indices=#") %
		cmap.Materials.size() %
		cmap.GetVertexCount() %
		cmap.GetIndexCount()).str() << std::endl;
}

void QuarkCompileMap(const std::vector<string> &Params)
{
	// check parameter count
	if (Params.size() != 3)
	{
		std::cout << "Syntax:\n"
			"  QuArK <SrcFile> <DestFile> <ScriptFile>\n";
		return;
	}

	// parse parameters
	string SrcFileName = Params[0];
	string DstFileName = Params[1];
	string ScriptFileName = Params[2];

	// do processing
	std::cout << "Loading Map Script: " << ScriptFileName << "..." << std::endl;
	g_Script = new MapScript;
	scoped_ptr<MapScript> script_guard(g_Script);
	g_Script->Load(ScriptFileName);

	std::cout << "Loading QuArK map: " << SrcFileName << "..." << std::endl;
	QUARK_MAP qmap;
	qmap.Load(SrcFileName);
	std::cout << (Format("  Loaded. Properties=#, Brushes=#, Entities=#") %
		qmap.properties.size() %
		qmap.brushes.size() %
		qmap.entities.size()).str() << std::endl;

	COMPILED_MAP cmap;
	CompileMap(qmap, cmap);

	std::cout << "Saving compiled map: " << DstFileName << "..." << std::endl;
	cmap.Save(DstFileName);

	std::cout << "All done." << std::endl;
}
