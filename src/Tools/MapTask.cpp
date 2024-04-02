/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "PCH.hpp"
#include <map>
#include <typeinfo>
#include <algorithm>
#include "GlobalCode.hpp"
#include "MapTask.hpp"

float DEFAULT_OCTREE_K = 0.25f;
uint DEFAULT_MAX_OCTREE_DEPTH = 9;

const VEC3 BEST_AXIS_LOOKUP[] =
{
	VEC3(+1.0f, 0.0f, 0.0f), VEC3(0.0f, 0.0f, +1.0f), VEC3(0.0f, +1.0f, 0.0f),
	VEC3(-1.0f, 0.0f, 0.0f), VEC3(0.0f, 0.0f, -1.0f), VEC3(0.0f, +1.0f, 0.0f),
	VEC3(0.0f, 0.0f, -1.0f), VEC3(+1.0f, 0.0f, 0.0f), VEC3(0.0f, +1.0f, 0.0f),
	VEC3(0.0f, 0.0f, +1.0f), VEC3(-1.0f, 0.0f, 0.0f), VEC3(0.0f, +1.0f, 0.0f),
	VEC3(0.0f, +1.0f, 0.0f), VEC3(+1.0f, 0.0f, 0.0f), VEC3(0.0f, 0.0f, +1.0f),
	VEC3(0.0f, -1.0f, 0.0f), VEC3(+1.0f, 0.0f, 0.0f), VEC3(0.0f, 0.0f, -1.0f),
};


MapJob::MapJob() :
	Bounds(BOX::ZERO),
	OctreeK(DEFAULT_OCTREE_K),
	MaxOctreeDepth(DEFAULT_MAX_OCTREE_DEPTH)
{
}

MapJob::~MapJob()
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktury do formatu poœredniego

struct INTER_VERTEX
{
	VEC3 Pos;
	VEC2 Tex;
};

struct INTER_FACE
{
	uint VertexCount; // 3 lub 4
	INTER_VERTEX Vertices[4];
};

typedef std::vector<INTER_FACE> INTER_FACE_VECTOR;

// Mapuje nazwa-materia³u => œcianki
typedef std::map< string, shared_ptr<INTER_FACE_VECTOR> > INTER_MAP;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktury do formatu QMAP

struct QMAP_DRAW_VERTEX
{
	VEC3 Pos;
	VEC3 Normal;
	VEC2 Tex;
	VEC3 Tangent;
	VEC3 Binormal;
};

// Fragment geometrii nale¿¹cy do jednego wêz³a drzewa rysowania, rysowany jednym materia³em
struct QMAP_DRAW_FRAGMENT
{
	uint MaterialIndex;
	// Indeks pierwszy i za-ostatni do DrawIB - wyznaczaj¹ zakres rysowanych trójk¹tów
	uint IndexBegin;
	uint IndexEnd;
	// Indeks pierwszy i za-ostatni do DrawVB - wyznaczaj¹ zakres u¿ywanych wierzcho³ków
	// (niekoniecznie wszystkie z tego zakresu musz¹ byæ u¿ywane)
	uint VertexBegin;
	uint VertexEnd;
};

// Wêze³ drzewa Octree dla rysowania
struct QMAP_DRAW_NODE
{
	BOX Bounds;
	std::vector<QMAP_DRAW_FRAGMENT> Fragments;
	// Wszystkie 8 albo ¿aden
	shared_ptr<QMAP_DRAW_NODE> SubNodes[8];
};

struct QMAP_COLLISION_VERTEX
{
	VEC3 Pos;
};

// Wêze³ drzewa Octree do liczenia kolizji
struct QMAP_COLLISION_NODE
{
	BOX Bounds;
	// Indeks pierwszy i za-ostatni do CollisionIB - wyznaczaj¹ zakres rysowanych trójk¹tów
	uint IndexBegin;
	uint IndexEnd;
	// Wszystkie 8 albo ¿aden
	shared_ptr<QMAP_COLLISION_NODE> SubNodes[8];
};

struct QMAP
{
	STRING_VECTOR Materials;
	shared_ptr<QMAP_DRAW_NODE> DrawTree;
	std::vector<QMAP_DRAW_VERTEX> DrawVB;
	std::vector<uint4> DrawIB; // 32-bitowy!
	shared_ptr<QMAP_COLLISION_NODE> CollisionTree;
	std::vector<QMAP_COLLISION_VERTEX> CollisionVB;
	std::vector<uint4> CollisionIB;
	// TODO - œwiat³a
	// TODO - encje
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktury do formatu QMAP TMP

struct TMP_VERTEX
{
	VEC3 Pos;
};

struct TMP_FACE
{
	uint MaterialIndex;
	bool Smooth;
	uint NumVertices; // 3 lub 4
	uint VertexIndices[4];
	VEC2 TexCoords[4];
};

struct TMP_OBJECT
{
	string Name;
	VEC3 Position;
	VEC3 Orientation;
	VEC3 Size;
};

struct TMP_MESH : public TMP_OBJECT
{
	float AutoSmoothAngle;
	bool HasTexCoords;
	STRING_VECTOR Materials;
	std::vector<TMP_VERTEX> Vertices;
	std::vector<TMP_FACE> Faces;
};

struct TMP_LAMP : public TMP_OBJECT
{
	enum TYPE
	{
		TYPE_LAMP,
		TYPE_SPOT,
	};

	TYPE Type;
	float Dist;
	VEC3 Color; // R, G, B

	// Tylko dla SPOT
	float Softness;
	float Angle;
};

struct TMP_QMAP
{
	std::vector< shared_ptr<TMP_MESH> > Meshes;
	std::vector< shared_ptr<TMP_LAMP> > Lamps;
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktury do QMAP DESC

struct QMAP_DESC_MATERIAL
{
	bool Skip;
	string Name;
	bool TexGen;
	float TexScale;
	string NewName; // pusty jeœli nie ma zmiany nazwy

	QMAP_DESC_MATERIAL() :
		Skip(false),
		TexGen(false),
		TexScale(1.f)
	{
	}
};

typedef std::map< string, shared_ptr<QMAP_DESC_MATERIAL> > QMAP_DESC_MATERIAL_MAP;

struct QMAP_DESC
{
	QMAP_DESC_MATERIAL_MAP Materials;
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje

void ParseTmpMesh(TMP_QMAP *Map, Tokenizer &t)
{
	TMP_MESH *Mesh = new TMP_MESH;
	Map->Meshes.push_back(shared_ptr<TMP_MESH>(Mesh));

	// Nazwa
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Mesh->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	// Pozycja
	ParseVec3(&Mesh->Position, t);
	// Orientacja
	ParseVec3(&Mesh->Orientation, t);
	// Rozmiar
	ParseVec3(&Mesh->Size, t);

	// AutoSmoothAngle
	Mesh->AutoSmoothAngle = t.MustGetFloat();
	t.Next();
	// HasTexCoords
	Mesh->HasTexCoords = (t.MustGetUint4() > 0);
	t.Next();

	// Materia³y

	t.AssertKeyword(3);
	t.Next();

	t.AssertToken(Tokenizer::TOKEN_INTEGER);
	uint NumMaterials = t.MustGetUint4();
	Mesh->Materials.resize(NumMaterials);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	for (uint mi = 0; mi < NumMaterials; mi++)
	{
		t.AssertToken(Tokenizer::TOKEN_STRING);
		Mesh->Materials[mi] = t.GetString();
		t.Next();
	}

	t.AssertSymbol('}');
	t.Next();

	// Wierzcho³ki

	t.AssertKeyword(4);
	t.Next();

	t.AssertToken(Tokenizer::TOKEN_INTEGER);
	uint NumVertices = t.MustGetUint4();
	Mesh->Vertices.resize(NumVertices);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	for (uint vi = 0; vi < NumVertices; vi++)
	{
		Mesh->Vertices[vi].Pos.x = t.MustGetFloat();
		t.Next();
		t.AssertSymbol(',');
		t.Next();
		Mesh->Vertices[vi].Pos.y = t.MustGetFloat();
		t.Next();
		t.AssertSymbol(',');
		t.Next();
		Mesh->Vertices[vi].Pos.z = t.MustGetFloat();
		t.Next();
	}

	t.AssertSymbol('}');
	t.Next();

	// Œcianki (faces)

	t.AssertKeyword(5); // faces
	t.Next();

	t.AssertToken(Tokenizer::TOKEN_INTEGER);
	uint NumFaces = t.MustGetUint4();
	Mesh->Faces.resize(NumFaces);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	for (uint fi = 0; fi < NumFaces; fi++)
	{
		TMP_FACE & f = Mesh->Faces[fi];

		t.AssertToken(Tokenizer::TOKEN_INTEGER);
		f.NumVertices = t.MustGetUint4();
		if (f.NumVertices != 3 && f.NumVertices != 4)
			throw Error("B³êdna liczba wierzcho³ków w œciance.");
		t.Next();

		t.AssertToken(Tokenizer::TOKEN_INTEGER);
		f.MaterialIndex = t.MustGetUint4();
		t.Next();

		t.AssertToken(Tokenizer::TOKEN_INTEGER);
		f.Smooth = (t.MustGetUint4() > 0);
		t.Next();

		// Wierzcho³ki tej œcianki
		for (uint vi = 0; vi < f.NumVertices; vi++)
		{
			// Indeks wierzcho³ka
			t.AssertToken(Tokenizer::TOKEN_INTEGER);
			f.VertexIndices[vi] = t.MustGetUint4();
			t.Next();

			// Wspó³rzêdne tekstury
			if (Mesh->HasTexCoords)
			{
				f.TexCoords[vi].x = t.MustGetFloat();
				t.Next();
				t.AssertSymbol(',');
				t.Next();
				f.TexCoords[vi].y = t.MustGetFloat();
				t.Next();
			}
			else
				f.TexCoords[vi] = VEC2::ZERO;
		}
	}

	t.AssertSymbol('}');
	t.Next();

	// Koniec obiektu mesh (to poprzednie to by³ koniec sekcji faces)
	t.AssertSymbol('}');
	t.Next();
}

void ParseTmpLamp(TMP_QMAP *Map, Tokenizer &t)
{
	TMP_LAMP *Lamp = new TMP_LAMP;
	Map->Lamps.push_back(shared_ptr<TMP_LAMP>(Lamp));

	// Nazwa
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Lamp->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	// Pozycja
	ParseVec3(&Lamp->Position, t);
	// Orientacja
	ParseVec3(&Lamp->Orientation, t);
	// Rozmiar
	ParseVec3(&Lamp->Size, t);

	// Typ
	t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
	if (t.GetString() == "Lamp")
		Lamp->Type = TMP_LAMP::TYPE_LAMP;
	else if (t.GetString() == "Spot")
		Lamp->Type = TMP_LAMP::TYPE_SPOT;
	else
		t.CreateError("B³êdny typ lampy");
	t.Next();

	// Parametry wspólne
	Lamp->Dist = t.MustGetFloat();
	t.Next();
	ParseVec3(&Lamp->Color, t);

	// Parametry tylko dla Spot
	if (Lamp->Type == TMP_LAMP::TYPE_SPOT)
	{
		Lamp->Softness = t.MustGetFloat();
		t.Next();
		Lamp->Angle = t.MustGetFloat();
		t.Next();
	}

	t.AssertSymbol('}');
	t.Next();
}

void ParseTmpObject(TMP_QMAP *Map, Tokenizer &t)
{
	// mesh
	if (t.QueryKeyword(2))
	{
		t.Next();
		ParseTmpMesh(Map, t);
	}
	// lamp
	else if (t.QueryKeyword(7))
	{
		t.Next();
		ParseTmpLamp(Map, t);
	}
	else
		t.CreateError();
}

void LoadQmapTmpFile(TMP_QMAP *Out, const string &FileName)
{
	Writeln("Loading QMAP TMP file \"" + FileName + "\"...");

	FileStream input_file(FileName, FM_READ);
	Tokenizer tokenizer(&input_file, 0);

	tokenizer.RegisterKeyword(1, "objects");
	tokenizer.RegisterKeyword(2, "mesh");
	tokenizer.RegisterKeyword(3, "materials");
	tokenizer.RegisterKeyword(4, "vertices");
	tokenizer.RegisterKeyword(5, "faces");
	tokenizer.RegisterKeyword(7, "lamp");

	tokenizer.Next();

	// Nag³ówek

	tokenizer.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
	if (tokenizer.GetString() != "QMAP") tokenizer.CreateError("B³êdny nag³ówek");
	tokenizer.Next();

	tokenizer.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
	if (tokenizer.GetString() != "TMP") tokenizer.CreateError("B³êdny nag³ówek");
	tokenizer.Next();

	tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
	if (tokenizer.MustGetUint4() != 10) tokenizer.CreateError("B³êdna wersja");
	tokenizer.Next();

	// Pocz¹tek

	tokenizer.AssertKeyword(1); // objects
	tokenizer.Next();

	tokenizer.AssertSymbol('{');
	tokenizer.Next();

	// Obiekty

	while (!tokenizer.QuerySymbol('}'))
		ParseTmpObject(Out, tokenizer);
	tokenizer.Next();

	tokenizer.AssertEOF();
}

void ParseDescMaterial(QMAP_DESC_MATERIAL *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
		if (t.GetString() == "skip")
		{
			t.Next();

			Out->Skip = true;

			t.AssertSymbol(';');
			t.Next();
		}
		else if (t.GetString() == "tex")
		{
			t.Next();

			t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
			if (t.GetString() == "gen")
			{
				t.Next();

				Out->TexGen = true;

				t.AssertSymbol(';');
				t.Next();
			}
			else if (t.GetString() == "scale")
			{
				t.Next();

				Out->TexScale = t.MustGetFloat();
				t.Next();

				t.AssertSymbol(';');
				t.Next();
			}
			else
				t.CreateError(Format("Nieznana komenda do \"tex\": \"#\"") % t.GetString());
		}
		else if (t.GetString() == "name")
		{
			t.Next();

			t.AssertToken(Tokenizer::TOKEN_STRING);
			Out->NewName = t.GetString();
			t.Next();

			t.AssertSymbol(';');
			t.Next();
		}
		else
			t.CreateError(Format("Nieznana komenda \"#\"") % t.GetString());
	}
	t.Next();
}

void LoadDescFile(QMAP_DESC *Out, const string &FileName)
{
	Writeln("Loading QMAP DESC file \"" + FileName + "\"...");

	FileStream input_file(FileName, FM_READ);
	Tokenizer tokenizer(&input_file, 0);
	tokenizer.Next();

	// Nag³ówek

	tokenizer.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
	if (tokenizer.GetString() != "QMAP") tokenizer.CreateError("B³êdny nag³ówek");
	tokenizer.Next();

	tokenizer.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
	if (tokenizer.GetString() != "DESC") tokenizer.CreateError("B³êdny nag³ówek");
	tokenizer.Next();

	tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
	if (tokenizer.MustGetUint4() != 10) tokenizer.CreateError("B³êdna wersja");
	tokenizer.Next();

	// Pocz¹tek

	tokenizer.AssertIdentifier("materials");
	tokenizer.Next();

	tokenizer.AssertSymbol('{');
	tokenizer.Next();

	// Materia³y

	while (!tokenizer.QuerySymbol('}'))
	{
		shared_ptr<QMAP_DESC_MATERIAL> Material(new QMAP_DESC_MATERIAL);
		ParseDescMaterial(Material.get(), tokenizer);
		Out->Materials.insert(std::make_pair(Material->Name, Material));
	}
	tokenizer.Next();

	tokenizer.AssertEOF();
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

void GenerateInterFaceTexCoords(INTER_FACE &Face)
{
	VEC2 offset;
	VEC3 tex_u, tex_v;
	PLANE Plane;
	PointsToPlane(&Plane, Face.Vertices[0].Pos, Face.Vertices[1].Pos, Face.Vertices[2].Pos);
	Normalize(&Plane);
	const VEC3 &plane_normal = Plane.GetNormal();
	ComputeBestAxes(plane_normal, &tex_u, &tex_v);

	for (size_t pi = 0; pi < Face.VertexCount; pi++)
	{
		Face.Vertices[pi].Tex.x = (Dot(Face.Vertices[pi].Pos, tex_u));
		Face.Vertices[pi].Tex.y = (Dot(Face.Vertices[pi].Pos, tex_v));
		if (pi == 0)
		{
			offset.x = floorf(Face.Vertices[pi].Tex.x);
			offset.y = floorf(Face.Vertices[pi].Tex.y);
		}
		// Zakomentowa³em, bo to powodowa³o buraki kiedy œcianka sk³ada³a siê z wielu trójk¹tów i u¿yte na niej zosta³o TexGen + TexScale
		//Face.Vertices[pi].Tex -= offset;
	}
}

void BuildInterMapFromMesh(INTER_MAP &Out, const TMP_MESH &TmpMesh, const QMAP_DESC &Desc)
{
	// Utwórz transformacjê
	MATRIX ObjectTransform;
	AssemblyBlenderObjectMatrix(&ObjectTransform, TmpMesh.Position, TmpMesh.Orientation, TmpMesh.Size);

	// Utwórz listê opisów dla poszczególnych materia³ów siatki
	// (mog¹ byæ NULL-e)
	std::vector<QMAP_DESC_MATERIAL*> MaterialDescs;
	for (uint mi = 0; mi < TmpMesh.Materials.size(); mi++)
	{
		QMAP_DESC_MATERIAL_MAP::const_iterator mit = Desc.Materials.find(TmpMesh.Materials[mi]);
		if (mit == Desc.Materials.end())
			MaterialDescs.push_back(NULL);
		else
			MaterialDescs.push_back(mit->second.get());
	}

	const string EmptyString;

	// Dla ka¿dej œcianki
	for (uint i = 0; i < TmpMesh.Faces.size(); i++)
	{
		const TMP_FACE & TmpFace = TmpMesh.Faces[i];

		const string *MaterialName;
		QMAP_DESC_MATERIAL *MaterialDesc;
		if (TmpFace.MaterialIndex < TmpMesh.Materials.size())
		{
			MaterialName = &TmpMesh.Materials[TmpFace.MaterialIndex];
			MaterialDesc = MaterialDescs[TmpFace.MaterialIndex];
		}
		else
		{
			MaterialName = &EmptyString;
			MaterialDesc = NULL;
		}

		// Materia³ ka¿e pomin¹æ t¹ œciankê
		if (MaterialDesc != NULL && MaterialDesc->Skip)
			continue;

		// Nowa nazwa materia³u
		if (MaterialDesc != NULL && !MaterialDesc->NewName.empty())
			MaterialName = &MaterialDesc->NewName;

		// ZnajdŸ wektor œcianek odpowiadaj¹cy jej materia³owi
		INTER_MAP::iterator iit = Out.find(*MaterialName);
		if (iit == Out.end())
			iit = Out.insert(std::make_pair(*MaterialName, shared_ptr<INTER_FACE_VECTOR>(new INTER_FACE_VECTOR))).first;
		INTER_FACE_VECTOR *InterFaceVector = (*iit).second.get();

		// Przepisz wierzcho³ki
		INTER_FACE InterFace;
		InterFace.VertexCount = TmpFace.NumVertices;
		for (uint vi = 0; vi < InterFace.VertexCount; vi++)
		{
			Transform(&InterFace.Vertices[vi].Pos, TmpMesh.Vertices[TmpFace.VertexIndices[vi]].Pos, ObjectTransform);
			BlenderToDirectxTransform(&InterFace.Vertices[vi].Pos);
		}

		// Zamieñ kolejnoœæ wierzcho³ków na uk³ad lewoskrêtny
		if (InterFace.VertexCount == 3)
		{
			std::swap(InterFace.Vertices[0], InterFace.Vertices[2]);
		}
		else
		{
			std::swap(InterFace.Vertices[0], InterFace.Vertices[3]);
			std::swap(InterFace.Vertices[1], InterFace.Vertices[2]);
		}

		// Wygeneruj lub przepisz wspó³rzêdne tekstury
		if (!TmpMesh.HasTexCoords || (MaterialDesc != NULL && MaterialDesc->TexGen))
			GenerateInterFaceTexCoords(InterFace);
		else
		{
			if (InterFace.VertexCount == 3)
			{
				// Zamiana z uk³adu Blendera na DirectX
				InterFace.Vertices[0].Tex.x =       TmpFace.TexCoords[2].x;
				InterFace.Vertices[0].Tex.y = 1.f - TmpFace.TexCoords[2].y;
				InterFace.Vertices[1].Tex.x =       TmpFace.TexCoords[1].x;
				InterFace.Vertices[1].Tex.y = 1.f - TmpFace.TexCoords[1].y;
				InterFace.Vertices[2].Tex.x =       TmpFace.TexCoords[0].x;
				InterFace.Vertices[2].Tex.y = 1.f - TmpFace.TexCoords[0].y;
			}
			else // 4
			{
				InterFace.Vertices[0].Tex.x =       TmpFace.TexCoords[3].x;
				InterFace.Vertices[0].Tex.y = 1.f - TmpFace.TexCoords[3].y;
				InterFace.Vertices[1].Tex.x =       TmpFace.TexCoords[2].x;
				InterFace.Vertices[1].Tex.y = 1.f - TmpFace.TexCoords[2].y;
				InterFace.Vertices[2].Tex.x =       TmpFace.TexCoords[1].x;
				InterFace.Vertices[2].Tex.y = 1.f - TmpFace.TexCoords[1].y;
				InterFace.Vertices[3].Tex.x =       TmpFace.TexCoords[0].x;
				InterFace.Vertices[3].Tex.y = 1.f - TmpFace.TexCoords[0].y;
			}
		}

		// Skalowanie wsp. tekstury
		if (MaterialDesc != NULL && MaterialDesc->TexScale != 1.f)
		{
			for (uint vi = 0; vi < InterFace.VertexCount; vi++)
				InterFace.Vertices[vi].Tex *= MaterialDesc->TexScale;
		}

		// Dodaj œciankê
		InterFaceVector->push_back(InterFace);
	}
}

void BuildInterMap(INTER_MAP &Out, const TMP_QMAP &TmpQmap, const QMAP_DESC &Desc)
{
	for (uint i = 0; i < TmpQmap.Meshes.size(); i++)
		BuildInterMapFromMesh(Out, *TmpQmap.Meshes[i].get(), Desc);
}

typedef std::deque<INTER_FACE*> INTER_FACE_DEQUE;
typedef std::vector<INTER_FACE_DEQUE> INTER_FACE_COLLECTION; // dla ka¿dego materia³u kolekcja œcianek

void FindMainBounds(BOX &Out, const INTER_MAP &InterMap)
{
	uint fc = 0;

	Out.p1 = VEC3(MAXFLOAT, MAXFLOAT, MAXFLOAT);
	Out.p2 = VEC3(MINFLOAT, MINFLOAT, MINFLOAT);

	for (INTER_MAP::const_iterator iit = InterMap.begin(); iit != InterMap.end(); ++iit)
	{
		for (uint fi = 0; fi < iit->second->size(); fi++)
		{
			const INTER_FACE &InterFace = (*iit->second)[fi];
			for (uint vi = 0; vi < InterFace.VertexCount; vi++)
			{
				Min(&Out.p1, Out.p1, InterFace.Vertices[vi].Pos);
				Max(&Out.p2, Out.p2, InterFace.Vertices[vi].Pos);
				fc++;
			}
		}
	}

	if (fc == 0)
		Out = BOX::ZERO;
}

// Wyznacza AABB podwêz³ów swobodnego octree
void BuildSubBounds(BOX OutSubBounds[8], const BOX &Bounds, float k)
{
	VEC3 BoundsCenter; Bounds.CalcCenter(&BoundsCenter);
	float x1 = Lerp(BoundsCenter.x, Bounds.p1.x, k);
	float y1 = Lerp(BoundsCenter.y, Bounds.p1.y, k);
	float z1 = Lerp(BoundsCenter.z, Bounds.p1.z, k);
	float x2 = Lerp(BoundsCenter.x, Bounds.p2.x, k);
	float y2 = Lerp(BoundsCenter.y, Bounds.p2.y, k);
	float z2 = Lerp(BoundsCenter.z, Bounds.p2.z, k);
	OutSubBounds[0] = BOX(Bounds.p1.x, Bounds.p1.y, Bounds.p1.z, x2,          y2,          z2);
	OutSubBounds[1] = BOX(x1,          Bounds.p1.y, Bounds.p1.z, Bounds.p2.x, y2,          z2);
	OutSubBounds[2] = BOX(Bounds.p1.x, y1,          Bounds.p1.z, x2,          Bounds.p2.y, z2);
	OutSubBounds[3] = BOX(x1,          y1,          Bounds.p1.z, Bounds.p2.x, Bounds.p2.y, z2);
	OutSubBounds[4] = BOX(Bounds.p1.x, Bounds.p1.y, z1,          x2,          y2,          Bounds.p2.z);
	OutSubBounds[5] = BOX(x1,          Bounds.p1.y, z1,          Bounds.p2.x, y2,          Bounds.p2.z);
	OutSubBounds[6] = BOX(Bounds.p1.x, y1,          z1,          x2,          Bounds.p2.y, Bounds.p2.z);
	OutSubBounds[7] = BOX(x1,          y1,          z1,          Bounds.p2.x, Bounds.p2.y, Bounds.p2.z);
}

bool DrawNodeRequiresSubdivide(const INTER_FACE_COLLECTION &FaceCollection)
{
	// Ka¿de 500 œcianek innym materia³em to 1 do wagi
	uint Weight = 0;
	for (uint mi = 0; mi < FaceCollection.size(); mi++)
		Weight += ceil_div<uint>(FaceCollection[mi].size(), 500); // MAGIC NUMBER! :D
	return (Weight > 2); // MAGIC NUMBER! :D
}

bool CollisionNodeRequiresSubdivide(const INTER_FACE_DEQUE &FaceCollection)
{
	return (FaceCollection.size() > 10); // MAGIC NUMBER! :D
}

bool InterFaceInBox(const INTER_FACE &InterFace, const BOX &Box)
{
	if (InterFace.VertexCount == 3)
		return
			TriangleInBox(InterFace.Vertices[0].Pos, InterFace.Vertices[1].Pos, InterFace.Vertices[2].Pos, Box);
	else // 4
		return
			TriangleInBox(InterFace.Vertices[0].Pos, InterFace.Vertices[1].Pos, InterFace.Vertices[2].Pos, Box) &&
			TriangleInBox(InterFace.Vertices[0].Pos, InterFace.Vertices[2].Pos, InterFace.Vertices[3].Pos, Box);
}

// Oj coœ mi siê widzi ¿e Ÿle liczy :(
void CalcTangents(
	const INTER_VERTEX &p1, const INTER_VERTEX &p2, const INTER_VERTEX &p3,
	VEC3 *Normal, VEC3 *Tangent, VEC3 *Binormal)
{
	// Te magiczne wzory dosta³em od Skalniaka, który z kolei dosta³ je od Krzyœka K. - pozdro :)

	float s1 = -(p2.Tex.x - p1.Tex.x);
	float s2 = -(p3.Tex.x - p1.Tex.x);
	float t1 = -(p2.Tex.y - p1.Tex.y);
	float t2 = -(p3.Tex.y - p1.Tex.y);

	VEC3 v1, v2;
	v1 = p2.Pos - p1.Pos;
	v2 = p3.Pos - p1.Pos;

	*Tangent = v1*t2 - v2*t1;
	*Binormal = v2*s1 - v1*s2;

	// Krzysiek K. powiada: ma byæ wersja 1, czyli z p³aszczyzny, a nie z cross productu tych dwóch.
	// Powód: Bo nie mo¿emy czyniæ za³o¿eñ co do skrêtnoœci tego uk³adu, tekstura mo¿e byæ na³o¿ona
	// prosto lub odbita i by normalna mia³a przeciwny zwrot.

	// normal - version 1 (from plane)
	PLANE Plane;
	PointsToPlane(&Plane, p1.Pos, p2.Pos, p3.Pos);
	Normalize(&Plane);
	*Normal = Plane.GetNormal();
	// normal - version 2 (from tangent and binormal)
	// minus or not minus, this is the question...
//	face->Normal = -VecCross(face->Tangent, face->Binormal);
//	Normalize(&face->Normal);
	// additional option for version 2 - has no sense, but I've tried to fix some problems with this :)
//	if (Dot(face->Normal, face->Plane.GetNormal()) < 0.0f)
//		face->Normal = -face->Normal;

	// From Krzysiek K.: ¿eby tangent by³ prostopad³y do normalnej
	*Tangent -= *Normal * Dot(*Normal, *Tangent);
	Normalize(Tangent);
	Normalize(Binormal);
	Normalize(Normal);

	// check
//	assert(around(VecCross(face->Tangent, face->Binormal), face->Plane.GetNormal(), 1e-3f) ||
//		around(VecCross(face->Tangent, face->Binormal), -face->Plane.GetNormal(), 1e-3f));
}

void CreateDrawFragment(QMAP_DRAW_FRAGMENT &OutFragment, uint MaterialIndex, const INTER_FACE_DEQUE &Faces, QMAP &Qmap)
{
	assert(Faces.size() > 0);

	OutFragment.MaterialIndex = MaterialIndex;

	// Indeks pierwszego elementu VB i IB jakiego u¿yjemy
	uint vi;
	OutFragment.VertexBegin = Qmap.DrawVB.size();
	vi = OutFragment.VertexBegin;
	OutFragment.IndexBegin = Qmap.DrawIB.size();

	VEC3 Normal, Tangent, Binormal;
	QMAP_DRAW_VERTEX v;

	// Dla kolejnych œcianek
	for (uint fi = 0; fi < Faces.size(); fi++)
	{
		const INTER_FACE &InterFace = *Faces[fi];

		// Wylicz tangenty
		CalcTangents(InterFace.Vertices[0], InterFace.Vertices[1], InterFace.Vertices[2], &Normal, &Tangent, &Binormal);
		v.Normal = Normal;
		v.Tangent = Tangent;
		v.Binormal = Binormal;

		// Przepisz wierzcho³ki do VB i indeksy do IB
		v.Pos = InterFace.Vertices[0].Pos;
		v.Tex = InterFace.Vertices[0].Tex;
		Qmap.DrawVB.push_back(v);
		v.Pos = InterFace.Vertices[1].Pos;
		v.Tex = InterFace.Vertices[1].Tex;
		Qmap.DrawVB.push_back(v);
		v.Pos = InterFace.Vertices[2].Pos;
		v.Tex = InterFace.Vertices[2].Tex;
		Qmap.DrawVB.push_back(v);
		if (InterFace.VertexCount == 4)
		{
			v.Pos = InterFace.Vertices[3].Pos;
			v.Tex = InterFace.Vertices[3].Tex;
			Qmap.DrawVB.push_back(v);
		}

		Qmap.DrawIB.push_back(vi  );
		Qmap.DrawIB.push_back(vi+1);
		Qmap.DrawIB.push_back(vi+2);
		if (InterFace.VertexCount == 4)
		{
			Qmap.DrawIB.push_back(vi  );
			Qmap.DrawIB.push_back(vi+2);
			Qmap.DrawIB.push_back(vi+3);
		}

		vi += InterFace.VertexCount;
	}

	// Indeks za-ostatniego elementu VB i IB jakiego u¿yliœmy
	OutFragment.VertexEnd = Qmap.DrawVB.size();
	OutFragment.IndexEnd = Qmap.DrawIB.size();
}

void CreateCollisionFragment(QMAP_COLLISION_NODE &OutNode, const INTER_FACE_DEQUE &Faces, QMAP &Qmap)
{
	// Indeks pierwszego elementu IB jakiego u¿yjemy
	uint vi = Qmap.CollisionVB.size();
	OutNode.IndexBegin = Qmap.CollisionIB.size();

	QMAP_COLLISION_VERTEX v;

	// Dla kolejnych œcianek
	for (uint fi = 0; fi < Faces.size(); fi++)
	{
		const INTER_FACE &InterFace = *Faces[fi];

		// Przepisz wierzcho³ki do VB i indeksy do IB
		v.Pos = InterFace.Vertices[0].Pos;
		Qmap.CollisionVB.push_back(v);
		v.Pos = InterFace.Vertices[1].Pos;
		Qmap.CollisionVB.push_back(v);
		v.Pos = InterFace.Vertices[2].Pos;
		Qmap.CollisionVB.push_back(v);
		if (InterFace.VertexCount == 4)
		{
			v.Pos = InterFace.Vertices[3].Pos;
			Qmap.CollisionVB.push_back(v);
		}

		Qmap.CollisionIB.push_back(vi  );
		Qmap.CollisionIB.push_back(vi+1);
		Qmap.CollisionIB.push_back(vi+2);
		if (InterFace.VertexCount == 4)
		{
			Qmap.CollisionIB.push_back(vi  );
			Qmap.CollisionIB.push_back(vi+2);
			Qmap.CollisionIB.push_back(vi+3);
		}

		vi += InterFace.VertexCount;
	}

	// Indeks za-ostatniego elementu IB jakiego u¿yliœmy
	OutNode.IndexEnd = Qmap.CollisionIB.size();
}

// Funkcja rekurencyjna, buduj¹ca Draw Octree
void ProcessDrawNode(uint Level, QMAP_DRAW_NODE &DrawNode, INTER_FACE_COLLECTION &FaceCollection, QMAP &Qmap, const MapJob &Job)
{
	// S¹ podwêz³y
	if (Level < Job.MaxOctreeDepth && DrawNodeRequiresSubdivide(FaceCollection))
	{
		// Œcianki dla wêz³ów potomnych - pocz¹tkowo puste listy
		INTER_FACE_COLLECTION FaceCollectionSub[8];
		for (uint si = 0; si < 8; si++)
			FaceCollectionSub[si].resize(FaceCollection.size());

		// Granice wêz³ów potomnych
		BOX SubBounds[8];
		BuildSubBounds(SubBounds, DrawNode.Bounds, Job.OctreeK);

		// Dla ka¿dego materia³u
		for (uint mi = 0; mi < FaceCollection.size(); mi++)
		{
			// Dla ka¿dej œcianki
			for (uint fi = 0; fi < FaceCollection[mi].size(); fi++)
			{
				// Dla ka¿dego podwêz³a
				for (uint si = 0; si < 8; si++)
				{
					// Jeœli œcianka zawiera siê w ca³oœci w tym podwêŸle, przenieœ j¹ do niego
					if (InterFaceInBox(*FaceCollection[mi][fi], SubBounds[si]))
					{
						FaceCollectionSub[si][mi].push_back(FaceCollection[mi][fi]);
						FaceCollection[mi][fi] = NULL;
						break;
					}
				}
			}
		}

		// Jeœli jakieœ œcianki zosta³y odes³ane do podwêz³ów
		uint SubFaceC = 0;
		for (uint si = 0; si < 8; si++)
			for (uint mi = 0; mi < FaceCollection.size(); mi++)
				SubFaceC += FaceCollectionSub[si][mi].size();
		if (SubFaceC > 0)
		{
			// Utwórz wêz³y potomne
			for (uint si = 0; si < 8; si++)
			{
				DrawNode.SubNodes[si].reset(new QMAP_DRAW_NODE);
				DrawNode.SubNodes[si]->Bounds = SubBounds[si];
				ProcessDrawNode(Level+1, *DrawNode.SubNodes[si], FaceCollectionSub[si], Qmap, Job);
			}
			// Skasuj z listy wyzerowane chwilowo wskaŸniki do œcianek, które przenieœliœmy do podwêz³ów
			for (uint mi = 0; mi < FaceCollection.size(); mi++)
			{
				INTER_FACE_DEQUE::iterator end_it = std::remove(FaceCollection[mi].begin(), FaceCollection[mi].end(), (INTER_FACE*)NULL);
				FaceCollection[mi].erase(end_it, FaceCollection[mi].end());
			}
		}
	}

	// Przepisz pozosta³e œcianki do tego wêz³a
	for (uint mi = 0; mi < FaceCollection.size(); mi++)
	{
		if (!FaceCollection[mi].empty())
		{
			QMAP_DRAW_FRAGMENT Fragment;
			CreateDrawFragment(Fragment, mi, FaceCollection[mi], Qmap);
			DrawNode.Fragments.push_back(Fragment);
		}
	}
}

// Funkcja rekurencyjna, buduj¹ca Collision Octree
void ProcessCollisionNode(uint Level, QMAP_COLLISION_NODE &CollisionNode, INTER_FACE_DEQUE &FaceCollection, QMAP &Qmap, const MapJob &Job)
{
	// S¹ podwêz³y
	if (Level < Job.MaxOctreeDepth && CollisionNodeRequiresSubdivide(FaceCollection))
	{
		// Œcianki dla wêz³ów potomnych - pocz¹tkowo puste listy
		INTER_FACE_DEQUE FaceCollectionSub[8];

		// Granice wêz³ów potomnych
		BOX SubBounds[8];
		BuildSubBounds(SubBounds, CollisionNode.Bounds, Job.OctreeK);

		// Dla ka¿dej œcianki
		for (uint fi = 0; fi < FaceCollection.size(); fi++)
		{
			// Dla ka¿dego podwêz³a
			for (uint si = 0; si < 8; si++)
			{
				// Jeœli œcianka zawiera siê w ca³oœci w tym podwêŸle, przenieœ j¹ do niego
				if (InterFaceInBox(*FaceCollection[fi], SubBounds[si]))
				{
					FaceCollectionSub[si].push_back(FaceCollection[fi]);
					FaceCollection[fi] = NULL;
					break;
				}
			}
		}

		// Jeœli jakieœ œcianki zosta³y odes³ane do podwêz³ów
		uint SubFaceC = 0;
		for (uint si = 0; si < 8; si++)
			SubFaceC += FaceCollectionSub[si].size();
		if (SubFaceC > 0)
		{
			// Utwórz wêz³y potomne
			for (uint si = 0; si < 8; si++)
			{
				CollisionNode.SubNodes[si].reset(new QMAP_COLLISION_NODE);
				CollisionNode.SubNodes[si]->Bounds = SubBounds[si];
				ProcessCollisionNode(Level+1, *CollisionNode.SubNodes[si], FaceCollectionSub[si], Qmap, Job);
			}
			// Skasuj z listy wyzerowane chwilowo wskaŸniki do œcianek, które przenieœliœmy do podwêz³ów
			INTER_FACE_DEQUE::iterator end_it = std::remove(FaceCollection.begin(), FaceCollection.end(), (INTER_FACE*)NULL);
			FaceCollection.erase(end_it, FaceCollection.end());
		}
	}

	// Przepisz pozosta³e œcianki do tego wêz³a
	CreateCollisionFragment(CollisionNode, FaceCollection, Qmap);
}

void BuildFinalMap(QMAP &Out, const INTER_MAP &InterMap, const QMAP_DESC &Desc, const MapJob &Job)
{
	// Lista materia³ów
	for (INTER_MAP::const_iterator iit = InterMap.begin(); iit != InterMap.end(); ++iit)
		Out.Materials.push_back(iit->first);

	// G³ówny AABB
	BOX MainBounds;
	if (Job.Bounds == BOX::ZERO)
		FindMainBounds(MainBounds, InterMap);
	else
		MainBounds = Job.Bounds;

	// Utwórz listê opisów dla poszczególnych materia³ów siatki
	// (mog¹ byæ NULL-e)
	std::vector<QMAP_DESC_MATERIAL*> MaterialDescs;
	for (INTER_MAP::const_iterator iit = InterMap.begin(); iit != InterMap.end(); ++iit)
	{
		QMAP_DESC_MATERIAL_MAP::const_iterator mit = Desc.Materials.find(iit->first);
		if (mit == Desc.Materials.end())
			MaterialDescs.push_back(NULL);
		else
			MaterialDescs.push_back(mit->second.get());
	}

	// Octree rysowania
	{
		INTER_FACE_COLLECTION FaceCollection;
		FaceCollection.resize(Out.Materials.size());
		uint mi = 0;
		for (INTER_MAP::const_iterator iit = InterMap.begin(); iit != InterMap.end(); ++iit)
		{
			for (uint fi = 0; fi < iit->second->size(); fi++)
				FaceCollection[mi].push_back(&(*iit->second)[fi]);
			mi++;
		}
		Out.DrawTree.reset(new QMAP_DRAW_NODE);
		Out.DrawTree->Bounds = MainBounds;
		ProcessDrawNode(0, *Out.DrawTree, FaceCollection, Out, Job);
	}

	// Octree kolizji
	{
		INTER_FACE_DEQUE FaceDeque;
		uint mi = 0;
		for (INTER_MAP::const_iterator iit = InterMap.begin(); iit != InterMap.end(); ++iit)
		{
			for (uint fi = 0; fi < iit->second->size(); fi++)
				FaceDeque.push_back(&(*iit->second)[fi]);
			mi++;
		}
		Out.CollisionTree.reset(new QMAP_COLLISION_NODE);
		Out.CollisionTree->Bounds = MainBounds;
		ProcessCollisionNode(0, *Out.CollisionTree, FaceDeque, Out, Job);
	}
}

void ConvertQmapTmpToQmap(QMAP *Out, const TMP_QMAP &QmapTmp, const QMAP_DESC &Desc, const MapJob &Job)
{
	Writeln("Processing map...");

	// Przetwórz geometriê do struktury tymczasowej, uwzglêdnij Desc
	INTER_MAP InterMap;
	BuildInterMap(InterMap, QmapTmp, Desc);

	// Podziel geometriê na octree, przepisz to struktury finalnej
	BuildFinalMap(*Out, InterMap, Desc, Job);
}

void LoadQmapTmp(QMAP *Out, const string &FileName, const string &DescFileName, const MapJob &Job)
{
	TMP_QMAP Tmp;
	LoadQmapTmpFile(&Tmp, FileName);

	QMAP_DESC Desc;
	if (!DescFileName.empty())
		LoadDescFile(&Desc, DescFileName);

	ConvertQmapTmpToQmap(Out, Tmp, Desc, Job);
}

void SaveDrawNode(Stream &F, const QMAP_DRAW_NODE &Node)
{
	F.WriteEx(Node.Bounds);

	if (Node.SubNodes[0] != NULL)
	{
		F.WriteEx((uint1)1);
		SaveDrawNode(F, *Node.SubNodes[0]);
		SaveDrawNode(F, *Node.SubNodes[1]);
		SaveDrawNode(F, *Node.SubNodes[2]);
		SaveDrawNode(F, *Node.SubNodes[3]);
		SaveDrawNode(F, *Node.SubNodes[4]);
		SaveDrawNode(F, *Node.SubNodes[5]);
		SaveDrawNode(F, *Node.SubNodes[6]);
		SaveDrawNode(F, *Node.SubNodes[7]);
	}
	else
		F.WriteEx((uint1)0);

	F.WriteEx((uint4)Node.Fragments.size());

	for (uint fi = 0; fi < Node.Fragments.size(); fi++)
	{
		F.WriteEx(Node.Fragments[fi].MaterialIndex);
		F.WriteEx(Node.Fragments[fi].IndexBegin);
		F.WriteEx(Node.Fragments[fi].IndexEnd);
		F.WriteEx(Node.Fragments[fi].VertexBegin);
		F.WriteEx(Node.Fragments[fi].VertexEnd);
	}
}

void SaveCollisionNode(Stream &F, const QMAP_COLLISION_NODE &Node)
{
	F.WriteEx(Node.Bounds);

	if (Node.SubNodes[0] != NULL)
	{
		F.WriteEx((uint1)1);
		SaveCollisionNode(F, *Node.SubNodes[0]);
		SaveCollisionNode(F, *Node.SubNodes[1]);
		SaveCollisionNode(F, *Node.SubNodes[2]);
		SaveCollisionNode(F, *Node.SubNodes[3]);
		SaveCollisionNode(F, *Node.SubNodes[4]);
		SaveCollisionNode(F, *Node.SubNodes[5]);
		SaveCollisionNode(F, *Node.SubNodes[6]);
		SaveCollisionNode(F, *Node.SubNodes[7]);
	}
	else
		F.WriteEx((uint1)0);

	F.WriteEx(Node.IndexBegin);
	F.WriteEx(Node.IndexEnd);
}

// Zapisuje QMAP do pliku
void SaveQmap(const QMAP &Qmap, const string &FileName)
{
	ERR_TRY;

	Writeln("Saving QMAP file \"" + FileName + "\"...");

	FileStream F(FileName, FM_WRITE);

	F.WriteStringF("TFQMAP10");

	F.WriteEx((uint4)Qmap.Materials.size());
	for (uint mi = 0; mi < (uint4)Qmap.Materials.size(); mi++)
		F.WriteString1(Qmap.Materials[mi]);

	F.WriteEx((uint4)Qmap.DrawVB.size());
	// Uwaga! Czy budowa bitowa jest dok³adnie taka jak trzeba?
	// Jeœli coœ siê bêdzie sypaæ to pierwsze co trzeba zrobiæ to przepisaæ tu zapis na poszczególne
	// pola poszczególnych wierzcho³ków.
	F.Write(&Qmap.DrawVB[0], Qmap.DrawVB.size()*sizeof(QMAP_DRAW_VERTEX));

	F.WriteEx((uint4)Qmap.DrawIB.size());
	F.Write(&Qmap.DrawIB[0], Qmap.DrawIB.size()*sizeof(uint4));

	SaveDrawNode(F, *Qmap.DrawTree);

	F.WriteEx((uint4)Qmap.CollisionVB.size());
	F.Write(&Qmap.CollisionVB[0], Qmap.CollisionVB.size()*sizeof(QMAP_COLLISION_VERTEX));

	F.WriteEx((uint4)Qmap.CollisionIB.size());
	F.Write(&Qmap.CollisionIB[0], Qmap.CollisionIB.size()*sizeof(uint4));

	SaveCollisionNode(F, *Qmap.CollisionTree);

	ERR_CATCH_FUNC("Nie mo¿na zapisaæ modelu do pliku: " + FileName);
}

void InputTask(QMAP &Qmap, MapJob &Job, InputMapTask &Task)
{
	// Wykryj typ
	string InputFileNameU; ExtractFileName(&InputFileNameU, Task.FileName);
	UpperCase(&InputFileNameU);

	QMAP Mesh;

	// Plik wejœciowy to QMSH.TMP
	if (StrEnds(InputFileNameU, ".QMAP.TMP"))
		LoadQmapTmp(&Qmap, Task.FileName, Job.DescFileName, Job);
	else
		throw Error("Nierozpoznany typ pliku wejœciowego (nieznane rozszerzenie): " + Task.FileName);
}

void OutputTask(QMAP &Qmap, MapJob &Job, OutputMapTask &Task)
{
	SaveQmap(Qmap, Task.FileName);
}

// Funkcja rekurencyjna
void CalcDrawTreeStats(uint Level, const QMAP_DRAW_NODE &Node, uint *NodeCount, std::vector<uint> *TrianglesPerLevel)
{
	(*NodeCount)++;

	if (Level >= TrianglesPerLevel->size())
	TrianglesPerLevel->resize(Level+1);
	for (uint fi = 0; fi < Node.Fragments.size(); fi++)
	{
		assert((Node.Fragments[fi].IndexEnd - Node.Fragments[fi].IndexBegin) % 3 == 0);
		(*TrianglesPerLevel)[Level] += (Node.Fragments[fi].IndexEnd - Node.Fragments[fi].IndexBegin) / 3;
	}

	if (Node.SubNodes[0] != NULL)
	{
		for (uint si = 0; si < 8; si++)
			CalcDrawTreeStats(Level+1, *Node.SubNodes[si], NodeCount, TrianglesPerLevel);
	}
}

// Funkcja rekurencyjna
void CalcCollisionTreeStats(uint Level, const QMAP_COLLISION_NODE &Node, uint *NodeCount, std::vector<uint> *TrianglesPerLevel)
{
	(*NodeCount)++;

	if (Level >= TrianglesPerLevel->size())
	TrianglesPerLevel->resize(Level+1);
	assert((Node.IndexEnd - Node.IndexBegin) % 3 == 0);
	(*TrianglesPerLevel)[Level] += (Node.IndexEnd - Node.IndexBegin) / 3;

	if (Node.SubNodes[0] != NULL)
	{
		for (uint si = 0; si < 8; si++)
			CalcCollisionTreeStats(Level+1, *Node.SubNodes[si], NodeCount, TrianglesPerLevel);
	}
}

void InfoTask(QMAP &Qmap, MapJob &Job, InfoMapTask &Task)
{
	Writeln("Info:");
	Writeln(Format("  Bounds=#") % Qmap.DrawTree->Bounds);

	Writeln(Format("  Materials=#") % Qmap.Materials.size());
	if (Task.Detailed)
	{
		for (uint mi = 0; mi < Qmap.Materials.size(); mi++)
			Writeln(Format("    Name=\"#\"") % Qmap.Materials[mi]);
	}

	assert(Qmap.DrawIB.size() % 3 == 0);
	Writeln(Format("  DrawVertices=#, DrawTriangles=#") % Qmap.DrawVB.size() % (Qmap.DrawIB.size()/3));
	if (Task.Detailed)
	{
		uint NodeCount = 0;
		std::vector<uint> TrianglesPerLevel;
		CalcDrawTreeStats(0, *Qmap.DrawTree, &NodeCount, &TrianglesPerLevel);
		Writeln(Format("    DrawTreeNodes=#, MaxLevel=#") % NodeCount % TrianglesPerLevel.size());
		Write("    TrianglesPerLevel:");
		for (uint i = 0; i < TrianglesPerLevel.size(); i++)
			Write(Format(" #") % TrianglesPerLevel[i]);
		Writeln();
	}

	assert(Qmap.CollisionIB.size() % 3 == 0);
	Writeln(Format("  CollisionVertices=#, CollisionTriangles=#") % Qmap.CollisionVB.size() % (Qmap.CollisionIB.size()/3));
	if (Task.Detailed)
	{
		uint NodeCount = 0;
		std::vector<uint> TrianglesPerLevel;
		CalcCollisionTreeStats(0, *Qmap.CollisionTree, &NodeCount, &TrianglesPerLevel);
		Writeln(Format("    CollisionTreeNodes=#, MaxLevel=#") % NodeCount % TrianglesPerLevel.size());
		Write("    TrianglesPerLevel:");
		for (uint i = 0; i < TrianglesPerLevel.size(); i++)
			Write(Format(" #") % TrianglesPerLevel[i]);
		Writeln();
	}
}

void DoMapJob(MapJob &Job)
{
	scoped_ptr<QMAP> Map;

	for (uint ti = 0; ti < Job.Tasks.size(); ti++)
	{
		MapTask &t = *Job.Tasks[ti].get();

		if (typeid(t) == typeid(InputMapTask))
		{
			Map.reset(new QMAP);
			InputTask(*Map.get(), Job, (InputMapTask&)t);
		}
		else if (typeid(t) == typeid(OutputMapTask))
		{
			if (Map == NULL)
				throw Error("No mesh loaded.");
			OutputTask(*Map.get(), Job, (OutputMapTask&)t);
		}
		else if (typeid(t) == typeid(InfoMapTask))
		{
			if (Map == NULL)
				throw Error("No mesh loaded.");
			InfoTask(*Map.get(), Job, (InfoMapTask&)t);
		}
		else
			throw Error("Nienznany typ zadania.");
	}
}
