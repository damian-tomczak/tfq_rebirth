/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "PCH.hpp"
#include <algorithm>
#include <typeinfo>
#include "..\..\doc\External\NVMeshMender.h"
#include "GlobalCode.hpp"
#include "MeshTask.hpp"

MeshJob::MeshJob() :
	Tangents(false),
	MaxSmoothAngle(30.f),
	CalcNormals(true),
	RespectExistingSplits(false),
	FixCylindricalWrapping(false)
{
}

MeshJob::~MeshJob()
{
}

TransformMeshTask::TransformMeshTask(uint Type, const string &Params) :
	Type(Type)
{
	string Data;
	// Bez parametrów
	if (Type == 3007 || Type == 3008)
	{
		Data = Params;
		//ObjectName.clear(); // Jest puste.
	}
	else
	{
		SubParameters subparams(Params);
		Data = subparams.GetKey(0);
		if (subparams.GetCount() == 2)
			ObjectName = subparams.GetValue(1);
	}

	switch (Type)
	{
	case 3001:
		{
			VEC3 v;
			MustStrToSth(&v, Data);
			Translation(&Transform, v);
		}
		break;
	case 3002:
		{
			VEC3 v;
			MustStrToSth(&v, Data);
			RotationYawPitchRoll(&Transform, DegToRad(v.x), DegToRad(v.y), DegToRad(v.z));
		}
		break;
	case 3003:
		{
			QUATERNION q;
			MustStrToSth(&q, Data);
			Normalize(&q); // Potrzebne?
			QuaternionToRotationMatrix(&Transform, q);
		}
		break;
	case 3004:
		{
			uint SemicolonPos = Data.find(';');
			if (SemicolonPos == string::npos)
				ThrowCmdLineSyntaxError();
			VEC3 Axis; float Angle;
			MustStrToSth(&Axis, Data.substr(0, SemicolonPos));
			MustStrToSth(&Angle, Data.substr(SemicolonPos+1));
			Normalize(&Axis);
			Angle = DegToRad(Angle);
			QUATERNION q;
			AxisToQuaternion(&q, Axis, Angle);
			Normalize(&q); // Potrzebne?
			QuaternionToRotationMatrix(&Transform, q);
		}
		break;
	case 3005:
		{
			// Pojedyncza liczba
			if (Data.find(',') == string::npos)
			{
				float s;
				MustStrToSth(&s, Data);
				Scaling(&Transform, s);
			}
			// X,Y,Z
			else
			{
				VEC3 s;
				MustStrToSth(&s, Data);
				Scaling(&Transform, s);
			}
		}
		break;
	case 3006:
		{
			MustStrToSth(&Transform, Data);
		}
		break;
	case 3010:
	case 3011:
		{
			MustStrToSth(&Box, Data);
		}
		break;
	// Bezparametrowe
	case 3007:
	case 3008:
		break;
	default:
		assert(0);
	}
}


TransformTexMeshTask::TransformTexMeshTask(uint Type, const string &Params) :
	Type(Type)
{
	SubParameters subparams(Params);
	string Data = subparams.GetKey(0);
	if (subparams.GetCount() == 2)
		ObjectName = subparams.GetValue(1);

	switch (Type)
	{
	case 4001:
		{
			VEC2 v;
			MustStrToSth(&v, Data);
			Translation(&Transform, v.x, v.y, 0.0f);
		}
		break;
	case 4002:
		{
			// Pojedyncza liczba
			if (Data.find(',') == string::npos)
			{
				float s;
				MustStrToSth(&s, Data);
				Scaling(&Transform, s, s, 1.0f);
			}
			// X,Y
			else
			{
				VEC2 s;
				MustStrToSth(&s, Data);
				Scaling(&Transform, s.x, s.y, 1.0f);
			}
		}
		break;
	case 4003:
		{
			MustStrToSth(&Transform, Data);
		}
		break;
	default:
		assert(0);
	}
}

RenameObjectMeshTask::RenameObjectMeshTask(const string &Params)
{
	SubParameters sp(Params);
	OldName = sp.GetKey(0);
	NewName = sp.GetValue(0);
}

RenameMaterialMeshTask::RenameMaterialMeshTask(const string &Params)
{
	SubParameters sp(Params);
	OldName = sp.GetKey(0);
	NewName = sp.GetValue(0);
}

SetMaterialMeshTask::SetMaterialMeshTask(const string &Params)
{
	SubParameters sp(Params);
	ObjectName = sp.GetKey(0);
	NewMaterialName = sp.GetValue(0);
}

RenameBoneMeshTask::RenameBoneMeshTask(const string &Params)
{
	SubParameters sp(Params);
	OldName = sp.GetKey(0);
	NewName = sp.GetValue(0);
}

RenameAnimationMeshTask::RenameAnimationMeshTask(const string &Params)
{
	SubParameters sp(Params);
	OldName = sp.GetKey(0);
	NewName = sp.GetValue(0);
}

ScaleTimeMeshTask::ScaleTimeMeshTask(const string &Params)
{
	SubParameters subparams(Params);
	string Data = subparams.GetKey(0);
	if (subparams.GetCount() == 2)
		AnimationName = subparams.GetValue(1);

	MustStrToSth(&Scale, Data);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktury do formatu QMSH

struct QMSH_VERTEX
{
	VEC3 Pos;
	VEC3 Normal;
	VEC2 Tex;
	VEC3 Tangent;      // Aktualny tylko jeœli Flags & FLAG_TANGENTS
	VEC3 Binormal;     // Aktualny tylko jeœli Flags & FLAG_TANGENTS
	float Weight1;     // Aktualny tylko jeœli Flags & FLAG_SKINNING
	uint BoneIndices; // Aktualny tylko jeœli Flags & FLAG_SKINNING
};

struct QMSH_SUBMESH
{
	string Name;
	string MaterialName;
	uint FirstTriangle;
	uint NumTriangles;
	// Wyznaczaj¹ zakres. Ten zakres, wy³¹cznie ten i w ca³oœci ten powinien byæ przeznaczony na ten obiekt (Submesh).
	uint MinVertexIndexUsed;   // odpowiednik parametru DrawIndexedPrimitive - MinIndex (tylko wyra¿ony w trójk¹tach)
	uint NumVertexIndicesUsed; // odpowiednik parametru DrawIndexedPrimitive - NumVertices (tylko wyra¿ony w trójk¹tach)
};

struct QMSH_BONE
{
	string Name;
	// Koœci indeksowane s¹ od 1, 0 jest zarezerwowane dla braku koœci (czyli ¿e nie ma parenta)
	uint ParentIndex;
	// Macierz przeszta³caj¹ca ze wsp. danej koœci do wsp. koœci nadrzêdnej w Bind Pose, ³¹cznie z translacj¹
	MATRIX Matrix;

	// Indeksy podkoœci, indeksowane równie¿ od 1
	// Tylko runtime, tego nie ma w pliku
	std::vector<uint> Children;
};

struct QMSH_KEYFRAME_BONE
{
	VEC3 Translation;
	QUATERNION Rotation;
	float Scaling;
};

struct QMSH_KEYFRAME
{
	float Time; // W sekundach
	std::vector<QMSH_KEYFRAME_BONE> Bones; // Liczba musi byæ równa liczbie koœci
};

struct QMSH_ANIMATION
{
	string Name;
	float Length; // W sekundach
	std::vector< shared_ptr<QMSH_KEYFRAME> > Keyframes;
};

struct QMSH
{
	static const uint FLAG_TANGENTS = 0x01;
	static const uint FLAG_SKINNING = 0x02;

	uint Flags;
	std::vector<QMSH_VERTEX> Vertices;
	std::vector<uint2> Indices;
	std::vector< shared_ptr<QMSH_SUBMESH> > Submeshes;
	std::vector< shared_ptr<QMSH_BONE> > Bones; // Tylko kiedy Flags & SKINNING
	std::vector< shared_ptr<QMSH_ANIMATION> > Animations; // Tylko kiedy FLags & SKINNING

	// Bry³y otaczaj¹ce
	// Dotycz¹ wierzcho³ków w pozycji spoczynkowej.
	bool BoundingVolumesCalculated;
	float BoundingSphereRadius;
	BOX BoundingBox;

	QMSH() : BoundingVolumesCalculated(false) { }
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktury do formatu QMSH TMP

namespace tmp
{

struct VERTEX
{
	VEC3 Pos;
};

struct FACE
{
	uint MaterialIndex;
	bool Smooth;
	uint NumVertices; // 3 lub 4
	uint VertexIndices[4];
	VEC2 TexCoords[4];
};

struct VERTEX_IN_GROUP
{
	uint Index;
	float Weight;
};

struct VERTEX_GROUP
{
	string Name;
	std::vector<VERTEX_IN_GROUP> VerticesInGroup;
};

struct MESH
{
	string Name;
	string ParentArmature; // £añcuch pusty jeœli nie ma
	string ParentBone; // £añcuch pusty jeœli nie ma
	VEC3 Position;
	VEC3 Orientation;
	VEC3 Size;
	float AutoSmoothAngle;
	STRING_VECTOR Materials;
	std::vector<VERTEX> Vertices;
	std::vector<FACE> Faces;
	std::vector< shared_ptr<VERTEX_GROUP> > VertexGroups;
};

struct BONE
{
	string Name;
	string Parent; // £añcuch pusty jeœli nie ma parenta
	VEC3 Head_Bonespace;
	VEC3 Head_Armaturespace;
	float HeadRadius;
	VEC3 Tail_Bonespace;
	VEC3 Tail_Armaturespace;
	float TailRadius;
	float Roll_Bonespace;
	float Roll_Armaturespace;
	float Length;
	float Weight;
	MATRIX Matrix_Bonespace; // Liczy siê tylko minor 3x3, reszta jest jak w jednostkowej
	MATRIX Matrix_Armaturespace; // Jest pe³na 4x4
};

struct ARMATURE
{
	string Name;
	VEC3 Position;
	VEC3 Orientation;
	VEC3 Size;
	bool VertexGroups;
	bool Envelopes;
	std::vector< shared_ptr<BONE> > Bones;
};

enum INTERPOLATION_METHOD
{
	INTERPOLATION_CONST,
	INTERPOLATION_LINEAR,
	INTERPOLATION_BEZIER,
};

enum EXTEND_METHOD
{
	EXTEND_CONST,
	EXTEND_EXTRAP,
	EXTEND_CYCLIC,
	EXTEND_CYCLIC_EXTRAP,
};

struct CURVE
{
	string Name;
	INTERPOLATION_METHOD Interpolation;
	EXTEND_METHOD Extend;
	std::vector<VEC2> Points;
};

struct CHANNEL
{
	string Name;
	std::vector< shared_ptr<CURVE> > Curves;
};

struct ACTION
{
	string Name;
	std::vector< shared_ptr<CHANNEL> > Channels;
};

struct QMSH
{
	std::vector< shared_ptr<MESH> > Meshes; // Dowolna iloœæ
	shared_ptr<ARMATURE> Armature; // Nie ma albo jest jeden
	std::vector< shared_ptr<ACTION> > Actions; // Dowolna iloœæ
	int FPS;
};

// Przechowuje dane na temat wybranej koœci we wsp. globalnych modelu w uk³adzie DirectX
struct BONE_INTER_DATA
{
	VEC3 HeadPos;
	VEC3 TailPos;
	float HeadRadius;
	float TailRadius;
	float Length;
	uint TmpBoneIndex; // Indeks tej koœci w Armature w TMP, liczony od 0 - stanowi mapowanie nowych na stare koœci
};

void ParseVertexGroup(VERTEX_GROUP *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		VERTEX_IN_GROUP vig;

		vig.Index = t.MustGetUint4();
		t.Next();

		vig.Weight = t.MustGetFloat();
		t.Next();

		Out->VerticesInGroup.push_back(vig);
	}
	t.Next();
}

void ParseBone(BONE *Out, Tokenizer &t)
{
	t.AssertKeyword(8); // bone
	t.Next();

	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Parent = t.GetString();
	t.Next();

	// head

	t.AssertKeyword(9); // head
	t.Next();

	ParseVec3(&Out->Head_Bonespace, t);
	ParseVec3(&Out->Head_Armaturespace, t);

	Out->HeadRadius = t.MustGetFloat();
	t.Next();

	// tail

	t.AssertKeyword(10); // tail
	t.Next();

	ParseVec3(&Out->Tail_Bonespace, t);
	ParseVec3(&Out->Tail_Armaturespace, t);

	Out->TailRadius = t.MustGetFloat();
	t.Next();

	// Reszta

	Out->Roll_Bonespace = t.MustGetFloat();
	t.Next();

	Out->Roll_Armaturespace = t.MustGetFloat();
	t.Next();

	Out->Length = t.MustGetFloat();
	t.Next();

	Out->Weight = t.MustGetFloat();
	t.Next();

	// Macierze

	ParseMatrix3x3(&Out->Matrix_Bonespace, t);
	ParseMatrix4x4(&Out->Matrix_Armaturespace, t);

	t.AssertSymbol('}');
	t.Next();
}

void ParseInterpolationMethod(INTERPOLATION_METHOD *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);

	if (t.GetString() == "CONST")
		*Out = INTERPOLATION_CONST;
	else if (t.GetString() == "LINEAR")
		*Out = INTERPOLATION_LINEAR;
	else if (t.GetString() == "BEZIER")
		*Out = INTERPOLATION_BEZIER;
	else
		t.CreateError();

	t.Next();
}

void ParseExtendMethod(EXTEND_METHOD *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);

	if (t.GetString() == "CONST")
		*Out = EXTEND_CONST;
	else if (t.GetString() == "EXTRAP")
		*Out = EXTEND_EXTRAP;
	else if (t.GetString() == "CYCLIC")
		*Out = EXTEND_CYCLIC;
	else if (t.GetString() == "CYCLIC_EXTRAP")
		*Out = EXTEND_CYCLIC_EXTRAP;
	else
		t.CreateError();

	t.Next();
}

void ParseCurve(CURVE *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	ParseInterpolationMethod(&Out->Interpolation, t);
	ParseExtendMethod(&Out->Extend, t);

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		VEC2 pt;
		ParseVec2(&pt, t);
		Out->Points.push_back(pt);
	}
	t.Next();
}

void ParseChannel(CHANNEL *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		shared_ptr<CURVE> curve(new CURVE);
		ParseCurve(curve.get(), t);
		Out->Curves.push_back(curve);
	}
	t.Next();
}

void ParseAction(ACTION *Out, Tokenizer &t)
{
	t.AssertToken(Tokenizer::TOKEN_STRING);
	Out->Name = t.GetString();
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while (!t.QuerySymbol('}'))
	{
		shared_ptr<CHANNEL> channel(new CHANNEL);
		ParseChannel(channel.get(), t);
		Out->Channels.push_back(channel);
	}
	t.Next();
}

} // namespace tmp


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje

void CalcBoundingVolumes(const QMSH &Qmsh, float *OutSphereRadius, BOX *OutBox)
{
	if (Qmsh.Vertices.empty())
	{
		*OutSphereRadius = 0.f;
		*OutBox = BOX(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
	}
	else
	{
		float MaxDistSq = LengthSq(Qmsh.Vertices[0].Pos);
		OutBox->p1 = OutBox->p2 = Qmsh.Vertices[0].Pos;

		for (uint i = 1; i < Qmsh.Vertices.size(); i++)
		{
			MaxDistSq = std::max(MaxDistSq, LengthSq(Qmsh.Vertices[i].Pos));
			Min(&OutBox->p1, OutBox->p1, Qmsh.Vertices[i].Pos);
			Max(&OutBox->p2, OutBox->p2, Qmsh.Vertices[i].Pos);
		}

		*OutSphereRadius = sqrtf(MaxDistSq);
	}
}

// Wylicza parametry bry³ otaczaj¹cych siatkê
void CalcBoundingVolumes(QMSH &Qmsh)
{
	Writeln("Calculating bounding volumes...");

	CalcBoundingVolumes(Qmsh, &Qmsh.BoundingSphereRadius, &Qmsh.BoundingBox);

	Qmsh.BoundingVolumesCalculated = true;
}

// Wylicza w razie potrzeby parametry bry³ otaczaj¹cych siatkê
void EnsureBoundingVolumes(QMSH &Qmsh)
{
	if (!Qmsh.BoundingVolumesCalculated)
		CalcBoundingVolumes(Qmsh);
}

// Uniewa¿nia bry³y otaczaj¹ce siatkê
void InvalidateBoundingVolumes(QMSH &Qmsh)
{
	Qmsh.BoundingVolumesCalculated = false;
}

// Liczy kolizjê punktu z bry³¹ wyznaczon¹ przez dwie po³¹czone kule, ka¿da ma swój œrodek i promieñ
// Wygl¹ta to coœ jak na koñcach dwie pó³kule a w œrodku walec albo œciêty sto¿ek.
// Ka¿dy koniec ma dwa promienie. Zwraca wp³yw koœci na ten punkt, tzn.:
// - Jeœli punkt le¿y w promieniu promienia wewnêtrznego, zwraca 1.
// - Jeœli punkt le¿y w promieniu promienia zewnêtrznego, zwraca 1..0, im dalej tym mniej.
// - Jeœli punkt le¿y poza promieniem zewnêtrznym, zwraca 0.
float PointToBone(
	const VEC3 &Pt,
	const VEC3 &Center1, float InnerRadius1, float OuterRadius1,
	const VEC3 &Center2, float InnerRadius2, float OuterRadius2)
{
	float T = ClosestPointOnLine(Pt, Center1, Center2-Center1) / DistanceSq(Center1, Center2);
	if (T <= 0.f)
	{
		float D = Distance(Pt, Center1);
		if (D <= InnerRadius1)
			return 1.f;
		else if (D < OuterRadius1)
			return 1.f - (D - InnerRadius1) / (OuterRadius1 - InnerRadius1);
		else
			return 0.f;
	}
	else if (T >= 1.f)
	{
		float D = Distance(Pt, Center2);
		if (D <= InnerRadius2)
			return 1.f;
		else if (D < OuterRadius2)
			return 1.f - (D - InnerRadius2) / (OuterRadius2 - InnerRadius2);
		else
			return 0.f;
	}
	else
	{
		float InterInnerRadius = Lerp(InnerRadius1, InnerRadius2, T);
		float InterOuterRadius = Lerp(OuterRadius1, OuterRadius2, T);
		VEC3 InterCenter; Lerp(&InterCenter, Center1, Center2, T);

		float D = Distance(Pt, InterCenter);
		if (D <= InterInnerRadius)
			return 1.f;
		else if (D < InterOuterRadius)
			return 1.f - (D - InterInnerRadius) / (InterOuterRadius - InterInnerRadius);
		else
			return 0.f;
	}
}

void LoadQmshTmpFile(tmp::QMSH *Out, const string &FileName)
{
	FileStream input_file(FileName, FM_READ);
	Tokenizer tokenizer(&input_file, 0);

	tokenizer.RegisterKeyword( 1, "objects");
	tokenizer.RegisterKeyword( 2, "mesh");
	tokenizer.RegisterKeyword( 3, "materials");
	tokenizer.RegisterKeyword( 4, "vertices");
	tokenizer.RegisterKeyword( 5, "faces");
	tokenizer.RegisterKeyword( 6, "vertex_groups");
	tokenizer.RegisterKeyword( 7, "armature");
	tokenizer.RegisterKeyword( 8, "bone");
	tokenizer.RegisterKeyword( 9, "head");
	tokenizer.RegisterKeyword(10, "tail");
	tokenizer.RegisterKeyword(11, "actions");
	tokenizer.RegisterKeyword(12, "params");
	tokenizer.RegisterKeyword(13, "fps");

	tokenizer.Next();

	// Nag³ówek

	tokenizer.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
	if (tokenizer.GetString() != "QMSH") tokenizer.CreateError("B³êdny nag³ówek");
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

	for (;;)
	{
		if (tokenizer.GetToken() == Tokenizer::TOKEN_SYMBOL && tokenizer.GetChar() == '}')
		{
			tokenizer.Next();
			break;
		}

		tokenizer.AssertToken(Tokenizer::TOKEN_KEYWORD);
		// mesh
		if (tokenizer.GetId() == 2)
		{
			tokenizer.Next();

			shared_ptr<tmp::MESH> object(new tmp::MESH);

			// Nazwa
			tokenizer.AssertToken(Tokenizer::TOKEN_STRING);
			object->Name = tokenizer.GetString();
			tokenizer.Next();

			// '{'
			tokenizer.AssertSymbol('{');
			tokenizer.Next();

			// Parametry obiektu

			ParseVec3(&object->Position, tokenizer);
			ParseVec3(&object->Orientation, tokenizer);
			ParseVec3(&object->Size, tokenizer);

			tokenizer.AssertToken(Tokenizer::TOKEN_STRING);
			object->ParentArmature = tokenizer.GetString();
			tokenizer.Next();

			tokenizer.AssertToken(Tokenizer::TOKEN_STRING);
			object->ParentBone = tokenizer.GetString();
			tokenizer.Next();

			object->AutoSmoothAngle = tokenizer.MustGetFloat();
			tokenizer.Next();

			// Materia³y

			tokenizer.AssertKeyword(3);
			tokenizer.Next();

			tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
			uint NumMaterials = tokenizer.MustGetUint4();
			object->Materials.resize(NumMaterials);
			tokenizer.Next();

			tokenizer.AssertSymbol('{');
			tokenizer.Next();

			for (uint mi = 0; mi < NumMaterials; mi++)
			{
				tokenizer.AssertToken(Tokenizer::TOKEN_STRING);
				object->Materials[mi] = tokenizer.GetString();
				tokenizer.Next();
			}

			tokenizer.AssertSymbol('}');
			tokenizer.Next();

			// Wierzcho³ki

			tokenizer.AssertKeyword(4);
			tokenizer.Next();

			tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
			uint NumVertices = tokenizer.MustGetUint4();
			object->Vertices.resize(NumVertices);
			tokenizer.Next();

			tokenizer.AssertSymbol('{');
			tokenizer.Next();

			for (uint vi = 0; vi < NumVertices; vi++)
			{
				object->Vertices[vi].Pos.x = tokenizer.MustGetFloat();
				tokenizer.Next();
				tokenizer.AssertSymbol(',');
				tokenizer.Next();
				object->Vertices[vi].Pos.y = tokenizer.MustGetFloat();
				tokenizer.Next();
				tokenizer.AssertSymbol(',');
				tokenizer.Next();
				object->Vertices[vi].Pos.z = tokenizer.MustGetFloat();
				tokenizer.Next();
			}

			tokenizer.AssertSymbol('}');
			tokenizer.Next();

			// Œcianki (faces)

			tokenizer.AssertKeyword(5); // faces
			tokenizer.Next();

			tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
			uint NumFaces = tokenizer.MustGetUint4();
			object->Faces.resize(NumFaces);
			tokenizer.Next();

			tokenizer.AssertSymbol('{');
			tokenizer.Next();

			for (uint fi = 0; fi < NumFaces; fi++)
			{
				tmp::FACE & f = object->Faces[fi];

				tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
				f.NumVertices = tokenizer.MustGetUint4();
				if (f.NumVertices != 3 && f.NumVertices != 4)
					throw Error("B³êdna liczba wierzcho³ków w œciance.");
				tokenizer.Next();

				tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
				f.MaterialIndex = tokenizer.MustGetUint4();
				tokenizer.Next();

				tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
				f.Smooth = (tokenizer.MustGetUint4() > 0);
				tokenizer.Next();

				// Wierzcho³ki tej œcianki
				for (uint vi = 0; vi < f.NumVertices; vi++)
				{
					// Indeks wierzcho³ka
					tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
					f.VertexIndices[vi] = tokenizer.MustGetUint4();
					tokenizer.Next();

					// Wspó³rzêdne tekstury
					f.TexCoords[vi].x = tokenizer.MustGetFloat();
					tokenizer.Next();
					tokenizer.AssertSymbol(',');
					tokenizer.Next();
					f.TexCoords[vi].y = tokenizer.MustGetFloat();
					tokenizer.Next();
				}
			}

			tokenizer.AssertSymbol('}');
			tokenizer.Next();

			// Grupy wierzcho³ków

			tokenizer.AssertKeyword(6); // vertex_groups
			tokenizer.Next();

			tokenizer.AssertSymbol('{');
			tokenizer.Next();

			while (!tokenizer.QuerySymbol('}'))
			{
				shared_ptr<tmp::VERTEX_GROUP> VertexGroup(new tmp::VERTEX_GROUP);
				ParseVertexGroup(VertexGroup.get(), tokenizer);
				object->VertexGroups.push_back(VertexGroup);
			}
			tokenizer.Next();

			// Obiekt gotowy

			Out->Meshes.push_back(object);
		}
		// armature
		else if (tokenizer.GetId() == 7)
		{
			tokenizer.Next();

			if (Out->Armature != NULL)
				tokenizer.CreateError("Multiple armature objects not allowed.");

			Out->Armature.reset(new tmp::ARMATURE);
			tmp::ARMATURE & arm = *Out->Armature.get();

			tokenizer.AssertToken(Tokenizer::TOKEN_STRING);
			arm.Name = tokenizer.GetString();
			tokenizer.Next();

			tokenizer.AssertSymbol('{');
			tokenizer.Next();

			// Parametry ogólne

			ParseVec3(&arm.Position, tokenizer);
			ParseVec3(&arm.Orientation, tokenizer);
			ParseVec3(&arm.Size, tokenizer);

			tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
			arm.VertexGroups = (tokenizer.MustGetUint4() > 0);
			tokenizer.Next();

			tokenizer.AssertToken(Tokenizer::TOKEN_INTEGER);
			arm.Envelopes = (tokenizer.MustGetUint4() > 0);
			tokenizer.Next();

			// Koœci

			while (!tokenizer.QuerySymbol('}'))
			{
				shared_ptr<tmp::BONE> bone(new tmp::BONE);
				ParseBone(bone.get(), tokenizer);
				arm.Bones.push_back(bone);
			}
		}
		else
			tokenizer.CreateError();

		tokenizer.AssertSymbol('}');
		tokenizer.Next();
	}

	// actions

	tokenizer.AssertKeyword(11);
	tokenizer.Next();

	tokenizer.AssertSymbol('{');
	tokenizer.Next();

	while (!tokenizer.QuerySymbol('}'))
	{
		shared_ptr<tmp::ACTION> action(new tmp::ACTION);
		ParseAction(action.get(), tokenizer);
		Out->Actions.push_back(action);
	}
	tokenizer.Next();

	// params

	tokenizer.AssertKeyword(12);
	tokenizer.Next();

	tokenizer.AssertSymbol('{');
	tokenizer.Next();

	while (!tokenizer.QuerySymbol('}'))
	{
		tokenizer.AssertToken(Tokenizer::TOKEN_KEYWORD);
		// fps
		if (tokenizer.GetId() == 13)
		{
			tokenizer.Next();

			tokenizer.AssertSymbol('=');
			tokenizer.Next();

			Out->FPS = tokenizer.MustGetInt4();
			tokenizer.Next();
		}
		else
			tokenizer.CreateError();
	}
	tokenizer.Next();

	tokenizer.AssertEOF();
}

// Przekszta³ca pozycje, wektory i wspó³rzêdne tekstur z uk³adu Blendera do uk³adu DirectX
// Uwzglêdnia te¿ przekszta³cenia ca³ych obiektów wprowadzaj¹c je do przekszta³ceñ poszczególnych wierzcho³ków.
// Zamienia te¿ kolejnoœæ wierzcho³ków w œciankach co by by³y odwrócone w³aœciw¹ stron¹.
// NIE zmienia danych koœci ani animacji
void TransformQmshTmpCoords(tmp::QMSH *InOut)
{
	for (uint oi = 0; oi < InOut->Meshes.size(); oi++)
	{
		tmp::MESH & o = *InOut->Meshes[oi].get();

		// Zbuduj macierz przekszta³cania tego obiektu
		MATRIX Mat;
		AssemblyBlenderObjectMatrix(&Mat, o.Position, o.Orientation, o.Size);
		// Jeœli obiekt jest sparentowany do Armature
		// To jednak nic, bo te wspó³rzêdne s¹ ju¿ wyeksportowane w uk³adzie globalnym modelu a nie wzglêdem parenta.
		if (!o.ParentArmature.empty())
		{
			assert(InOut->Armature != NULL);
			MATRIX ArmatureMat;
			AssemblyBlenderObjectMatrix(&ArmatureMat, InOut->Armature->Position, InOut->Armature->Orientation, InOut->Armature->Size);
			Mat = Mat * ArmatureMat;
		}

		// Przekszta³æ wierzcho³ki
		for (uint vi = 0; vi < o.Vertices.size(); vi++)
		{
			tmp::VERTEX & v = o.Vertices[vi];

			// Przekszta³æ pozycjê przez macierz obiektu
			VEC3 TmpV;
			Transform(&TmpV, v.Pos, Mat);

			// Przekszta³æ uk³ad wspó³rzêdnych
			BlenderToDirectxTransform(&v.Pos, TmpV);
		}

		// Przekszta³æ œcianki
		for (uint fi = 0; fi < o.Faces.size(); fi++)
		{
			tmp::FACE & f = o.Faces[fi];

			// Wsp. tekstur
			for (uint vi = 0; vi < f.NumVertices; vi++)
				f.TexCoords[vi].y = 1.0f - f.TexCoords[vi].y;

			// Zamieñ kolejnoœæ wierzcho³ków w œciance
			if (f.NumVertices == 3)
			{
				std::swap(f.VertexIndices[0], f.VertexIndices[2]);
				std::swap(f.TexCoords[0], f.TexCoords[2]);
			}
			else if (f.NumVertices == 4)
			{
				std::swap(f.VertexIndices[0], f.VertexIndices[3]);
				std::swap(f.VertexIndices[1], f.VertexIndices[2]);
				std::swap(f.TexCoords[0], f.TexCoords[3]);
				std::swap(f.TexCoords[1], f.TexCoords[2]);
			}
			else
				assert(0 && "tmp::FACE::NumVertices nieprawid³owe.");
		}
	}
}

// Szuka takiego wierzcho³ka w NvVertices.
// Jeœli nie znajdzie, dodaje nowy.
// Czy znalaz³ czy doda³, zwraca jego indeks w NvVertices.
uint TmpConvert_AddVertex(
	std::vector<MeshMender::Vertex> &NvVertices,
	std::vector<unsigned int> &MappingNvToTmpVert,
	uint TmpVertIndex,
	const tmp::VERTEX &TmpVertex,
	const VEC2 &TexCoord)
{
	for (uint i = 0; i < NvVertices.size(); i++)
	{
		// Pozycja identyczna...
		if (math_cast<VEC3>(NvVertices[i].pos) == TmpVertex.Pos &&
			// TexCoord mniej wiêcej...
			float_equal(TexCoord.x, NvVertices[i].s) && float_equal(TexCoord.y, NvVertices[i].t))
		{
			return i;
		}
	}

	// Nie znaleziono
	MeshMender::Vertex v;
	v.pos = math_cast<D3DXVECTOR3>(TmpVertex.Pos);
	v.s = TexCoord.x;
	v.t = TexCoord.y;
	NvVertices.push_back(v);
	MappingNvToTmpVert.push_back(TmpVertIndex);

	return NvVertices.size() - 1;
}

// Funkcja rekurencyjna
void TmpToQmsh_Bone(
	QMSH_BONE *Out,
	uint OutIndex,
	QMSH *OutQmsh,
	const tmp::BONE &TmpBone,
	const tmp::ARMATURE &TmpArmature,
	uint ParentIndex,
	const MATRIX &WorldToParent)
{
	Out->Name = TmpBone.Name;
	Out->ParentIndex = ParentIndex;

	// TmpBone.Matrix_Armaturespace zawiera przekszta³cenie ze wsp. lokalnych tej koœci do wsp. armature w uk³. Blender
	// BoneToArmature zawiera przekszta³cenie ze wsp. lokalnych tej koœci do wsp. armature w uk³. DirectX
	MATRIX BoneToArmature;
	BlenderToDirectxTransform(&BoneToArmature, TmpBone.Matrix_Armaturespace);
	// Out->Matrix bêdzie zwiera³o przekszta³cenie ze wsp. tej koœci do jej parenta w uk³. DirectX
	Out->Matrix = BoneToArmature * WorldToParent;
	MATRIX ArmatureToBone;
	Inverse(&ArmatureToBone, BoneToArmature);

	// Koœci potomne
	for (uint bi = 0; bi < TmpArmature.Bones.size(); bi++)
	{
		const tmp::BONE & TmpSubBone = *TmpArmature.Bones[bi].get();
		if (TmpSubBone.Parent == TmpBone.Name)
		{
			shared_ptr<QMSH_BONE> SubBone(new QMSH_BONE);
			OutQmsh->Bones.push_back(SubBone);
			Out->Children.push_back(OutQmsh->Bones.size());

			TmpToQmsh_Bone(
				SubBone.get(),
				OutQmsh->Bones.size(), // Bez -1, bo indeksujemy od 0
				OutQmsh,
				TmpSubBone,
				TmpArmature,
				OutIndex,
				ArmatureToBone);
		}
	}
}

void TmpToQmsh_Bones(
	QMSH *Out,
	std::vector<tmp::BONE_INTER_DATA> *OutBoneInterData,
	const tmp::QMSH &QmshTmp,
	const MeshJob &Job)
{
	Writeln("Processing bones...");
	const tmp::ARMATURE & TmpArmature = *QmshTmp.Armature.get();

	// Ta macierz przekszta³ca wsp. z lokalnych obiektu Armature do globalnych œwiata.
	// Bêdzie ju¿ w uk³adzie DirectX.
	MATRIX ArmatureToWorldMat;
	AssemblyBlenderObjectMatrix(&ArmatureToWorldMat, TmpArmature.Position, TmpArmature.Orientation, TmpArmature.Size);
	BlenderToDirectxTransform(&ArmatureToWorldMat);
	//MATRIX WorldToArmatureMat;
	//Inverse(&WorldToArmatureMat, ArmatureToWorldMat);

	// Dla ka¿dej koœci g³ównego poziomu
	for (uint bi = 0; bi < TmpArmature.Bones.size(); bi++)
	{
		const tmp::BONE & TmpBone = *TmpArmature.Bones[bi].get();
		if (TmpBone.Parent.empty())
		{
			// Utwórz koœæ QMSH
			shared_ptr<QMSH_BONE> Bone(new QMSH_BONE);
			Out->Bones.push_back(Bone);

			Bone->Name = TmpBone.Name;
			Bone->ParentIndex = 0; // Brak parenta

			TmpToQmsh_Bone(
				Bone.get(),
				Out->Bones.size(), // Bez -1, bo indeksujemy od 0
				Out,
				TmpBone,
				TmpArmature,
				0, // Nie ma parenta
				ArmatureToWorldMat);
		}
	}

	// Sprawdzenie liczby koœci
	if (Out->Bones.size() != QmshTmp.Armature->Bones.size())
		Warning(Format("Skipped # invalid bones.") % (QmshTmp.Armature->Bones.size() - Out->Bones.size()), 0);

	// Policzenie Bone Inter Data
	OutBoneInterData->resize(Out->Bones.size());
	for (uint bi = 0; bi < Out->Bones.size(); bi++)
	{
		QMSH_BONE & OutBone = *Out->Bones[bi].get();
		for (uint bj = 0; bj <= TmpArmature.Bones.size(); bj++) // To <= to czeœæ tego zabezpieczenia
		{
			assert(bj < TmpArmature.Bones.size()); // zabezpieczenie ¿e poœród koœci Ÿród³owych zawsze znajdzie siê te¿ ta docelowa

			const tmp::BONE & InBone = *TmpArmature.Bones[bj].get();
			if (OutBone.Name == InBone.Name)
			{
				tmp::BONE_INTER_DATA & bid = (*OutBoneInterData)[bi];

				bid.TmpBoneIndex = bj;

				VEC3 v;
				BlenderToDirectxTransform(&v, InBone.Head_Armaturespace);
				Transform(&bid.HeadPos, v, ArmatureToWorldMat);
				BlenderToDirectxTransform(&v, InBone.Tail_Armaturespace);
				Transform(&bid.TailPos, v, ArmatureToWorldMat);

				if (!float_equal(TmpArmature.Size.x, TmpArmature.Size.y) || !float_equal(TmpArmature.Size.y, TmpArmature.Size.z))
					Warning("Non uniform scaling of Armature object may give invalid bone envelopes.", 2342235);

				float ScaleFactor = (TmpArmature.Size.x + TmpArmature.Size.y + TmpArmature.Size.z) / 3.f;

				bid.HeadRadius = InBone.HeadRadius * ScaleFactor;
				bid.TailRadius = InBone.TailRadius * ScaleFactor;

				bid.Length = Distance(bid.HeadPos, bid.TailPos);

/*				Writeln(Format("BoneInterData: Name=#\r\n  Head=#, HeadRadius=#\r\n  Tail=#, TailRadius=#") %
					OutBone.Name %
					(*OutBoneInterData)[bi].HeadPos % (*OutBoneInterData)[bi].HeadRadius %
					(*OutBoneInterData)[bi].TailPos % (*OutBoneInterData)[bi].TailRadius);*/

				break;
			}
		}
	}

	// Sprawdzenie liczby koœci
	if (Out->Bones.size() > 32)
		throw Error("Too many bones - maximum 32 allowed.");
}

// Zwraca wartoœæ krzywej w podanej klatce
// TmpCurve mo¿e byæ NULL.
// PtIndex - indeks nastêpnego punktu
// NextFrame - jeœli nastêpna klatka tej krzywej istnieje i jest mniejsza ni¿ NextFrame, ustawia NextFrame na ni¹
float CalcTmpCurveValue(
	float DefaultValue,
	const tmp::CURVE *TmpCurve,
	uint *InOutPtIndex,
	float Frame,
	float *InOutNextFrame)
{
	// Nie ma krzywej
	if (TmpCurve == NULL)
		return DefaultValue;
	// W ogóle nie ma punktów
	if (TmpCurve->Points.empty())
		return DefaultValue;
	// To jest ju¿ koniec krzywej - przed³u¿enie ostatniej wartoœci
	if (*InOutPtIndex >= TmpCurve->Points.size())
		return TmpCurve->Points[TmpCurve->Points.size()-1].y;

	const VEC2 & NextPt = TmpCurve->Points[*InOutPtIndex];

	// - Przestawiæ PtIndex (opcjonalnie)
	// - Przestawiæ NextFrame
	// - Zwróciæ wartoœæ

	// Jesteœmy dok³adnie w tym punkcie (lub za, co nie powinno mieæ miejsca)
	if (float_equal(NextPt.x, Frame) || (Frame > NextPt.x))
	{
		(*InOutPtIndex)++;
		if (*InOutPtIndex < TmpCurve->Points.size())
			*InOutNextFrame = std::min(*InOutNextFrame, TmpCurve->Points[*InOutPtIndex].x);
		return NextPt.y;
	}
	// Jesteœmy przed pierwszym punktem - przed³u¿enie wartoœci sta³ej w lewo
	else if (*InOutPtIndex == 0)
	{
		*InOutNextFrame = std::min(*InOutNextFrame, NextPt.x);
		return NextPt.y;
	}
	// Jesteœmy miêdzy punktem poprzednim a tym
	else
	{
		*InOutNextFrame = std::min(*InOutNextFrame, NextPt.x);
		const VEC2 & PrevPt = TmpCurve->Points[*InOutPtIndex - 1];
		// Interpolacja liniowa wartoœci wynikowej
		float t = (Frame - PrevPt.x) / (NextPt.x - PrevPt.x);
		return Lerp(PrevPt.y, NextPt.y, t);
	}
}

void TmpToQmsh_Animation(
	QMSH_ANIMATION *OutAnimation,
	const tmp::ACTION &TmpAction,
	const QMSH &Qmsh,
	const tmp::QMSH &QmshTmp)
{
	OutAnimation->Name = TmpAction.Name;

	uint BoneCount = Qmsh.Bones.size();
	// Wspó³czynnik do przeliczania klatek na sekundy
	if (QmshTmp.FPS == 0)
		throw Error("FPS cannot be zero.");
	float TimeFactor = 1.f / (float)QmshTmp.FPS;

	// Lista wskaŸników do krzywych dotycz¹cych poszczególnych parametrów danej koœci QMSH
	// (Mog¹ byæ NULL)

	struct CHANNEL_POINTERS {
		const tmp::CURVE *LocX, *LocY, *LocZ, *QuatX, *QuatY, *QuatZ, *QuatW, *SizeX, *SizeY, *SizeZ;
		uint LocX_index, LocY_index, LocZ_index, QuatX_index, QuatY_index, QuatZ_index, QuatW_index, SizeX_index, SizeY_index, SizeZ_index;
	};
	std::vector<CHANNEL_POINTERS> BoneChannelPointers;
	BoneChannelPointers.resize(BoneCount);
	bool WarningInterpolation = false, WarningExtend = false;
	float EarliestNextFrame = MAXFLOAT;
	// Dla kolejnych koœci
	for (uint bi = 0; bi < BoneCount; bi++)
	{
		BoneChannelPointers[bi].LocX = NULL;  BoneChannelPointers[bi].LocX_index = 0;
		BoneChannelPointers[bi].LocY = NULL;  BoneChannelPointers[bi].LocY_index = 0;
		BoneChannelPointers[bi].LocZ = NULL;  BoneChannelPointers[bi].LocZ_index = 0;
		BoneChannelPointers[bi].QuatX = NULL; BoneChannelPointers[bi].QuatX_index = 0;
		BoneChannelPointers[bi].QuatY = NULL; BoneChannelPointers[bi].QuatY_index = 0;
		BoneChannelPointers[bi].QuatZ = NULL; BoneChannelPointers[bi].QuatZ_index = 0;
		BoneChannelPointers[bi].QuatW = NULL; BoneChannelPointers[bi].QuatW_index = 0;
		BoneChannelPointers[bi].SizeX = NULL; BoneChannelPointers[bi].SizeX_index = 0;
		BoneChannelPointers[bi].SizeY = NULL; BoneChannelPointers[bi].SizeY_index = 0;
		BoneChannelPointers[bi].SizeZ = NULL; BoneChannelPointers[bi].SizeZ_index = 0;

		//ZnajdŸ kana³ odpowiadaj¹cy tej koœci
		const string & BoneName = Qmsh.Bones[bi]->Name;
		for (uint ci = 0; ci < TmpAction.Channels.size(); ci++)
		{
			if (TmpAction.Channels[ci]->Name == BoneName)
			{
				// Kana³ znaleziony - przejrzyj jego krzywe
				const tmp::CHANNEL &TmpChannel = *TmpAction.Channels[ci].get();
				for (uint curvi = 0; curvi < TmpChannel.Curves.size(); curvi++)
				{
					const tmp::CURVE *TmpCurve = TmpChannel.Curves[curvi].get();
					// Zidentyfikuj po nazwie
					if (TmpCurve->Name == "LocX")
					{
						BoneChannelPointers[bi].LocX = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "LocY")
					{
						BoneChannelPointers[bi].LocY = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "LocZ")
					{
						BoneChannelPointers[bi].LocZ = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "QuatX")
					{
						BoneChannelPointers[bi].QuatX = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "QuatY")
					{
						BoneChannelPointers[bi].QuatY = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "QuatZ")
					{
						BoneChannelPointers[bi].QuatZ = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "QuatW")
					{
						BoneChannelPointers[bi].QuatW = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "ScaleX")
					{
						BoneChannelPointers[bi].SizeX = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "ScaleY")
					{
						BoneChannelPointers[bi].SizeX = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if (TmpCurve->Name == "ScaleW")
					{
						BoneChannelPointers[bi].SizeX = TmpCurve;
						if (!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
				}

				break;
			}
		}

		if (BoneChannelPointers[bi].LocX != NULL && BoneChannelPointers[bi].LocX->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].LocY != NULL && BoneChannelPointers[bi].LocY->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].LocY != NULL && BoneChannelPointers[bi].LocY->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatX != NULL && BoneChannelPointers[bi].QuatX->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatY != NULL && BoneChannelPointers[bi].QuatY->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatZ != NULL && BoneChannelPointers[bi].QuatZ->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatW != NULL && BoneChannelPointers[bi].QuatW->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].SizeX != NULL && BoneChannelPointers[bi].SizeX->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].SizeY != NULL && BoneChannelPointers[bi].SizeY->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].SizeZ != NULL && BoneChannelPointers[bi].SizeZ->Interpolation == tmp::INTERPOLATION_CONST)
		{
			WarningInterpolation = true;
		}
		if (BoneChannelPointers[bi].LocX != NULL && BoneChannelPointers[bi].LocX->Extend != tmp::EXTEND_CONST ||
			BoneChannelPointers[bi].LocY != NULL && BoneChannelPointers[bi].LocY->Extend != tmp::EXTEND_CONST ||
			BoneChannelPointers[bi].LocY != NULL && BoneChannelPointers[bi].LocY->Extend != tmp::EXTEND_CONST ||
			BoneChannelPointers[bi].QuatX != NULL && BoneChannelPointers[bi].QuatX->Extend != tmp::EXTEND_CONST ||
			BoneChannelPointers[bi].QuatY != NULL && BoneChannelPointers[bi].QuatY->Extend != tmp::EXTEND_CONST ||
			BoneChannelPointers[bi].QuatZ != NULL && BoneChannelPointers[bi].QuatZ->Extend != tmp::EXTEND_CONST ||
			BoneChannelPointers[bi].QuatW != NULL && BoneChannelPointers[bi].QuatW->Extend != tmp::EXTEND_CONST ||
			BoneChannelPointers[bi].SizeX != NULL && BoneChannelPointers[bi].SizeX->Extend != tmp::EXTEND_CONST ||
			BoneChannelPointers[bi].SizeY != NULL && BoneChannelPointers[bi].SizeY->Extend != tmp::EXTEND_CONST ||
			BoneChannelPointers[bi].SizeZ != NULL && BoneChannelPointers[bi].SizeZ->Extend != tmp::EXTEND_CONST)
		{
			WarningExtend = true;
		}
	}

	// Ostrze¿enia
	if (WarningInterpolation)
		Warning("Constant IPO interpolation mode not supported.", 2352634);
	if (WarningExtend)
		Warning("IPO extend modes other than constant not supported.", 2353464);

	// Wygenerowanie klatek kluczowych
	// (Nie ma ani jednej krzywej albo ¿adna nie ma punktów - nie bêdzie klatek kluczowych)
	while (EarliestNextFrame < MAXFLOAT)
	{
		float NextNext = MAXFLOAT;
		shared_ptr<QMSH_KEYFRAME> Keyframe(new QMSH_KEYFRAME);
		Keyframe->Time = (EarliestNextFrame - 1) * TimeFactor; // - 1, bo Blender liczy klatki od 1
		Keyframe->Bones.resize(BoneCount);
		for (uint bi = 0; bi < BoneCount; bi++)
		{
			Keyframe->Bones[bi].Translation.x = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocX, &BoneChannelPointers[bi].LocX_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Translation.z = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocY, &BoneChannelPointers[bi].LocY_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Translation.y = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocZ, &BoneChannelPointers[bi].LocZ_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.x =   CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatX, &BoneChannelPointers[bi].QuatX_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.z =   CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatY, &BoneChannelPointers[bi].QuatY_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.y =   CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatZ, &BoneChannelPointers[bi].QuatZ_index, EarliestNextFrame, &NextNext);
			Keyframe->Bones[bi].Rotation.w = - CalcTmpCurveValue(1.f, BoneChannelPointers[bi].QuatW, &BoneChannelPointers[bi].QuatW_index, EarliestNextFrame, &NextNext);
			float ScalingX = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeX, &BoneChannelPointers[bi].SizeX_index, EarliestNextFrame, &NextNext);
			float ScalingY = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeY, &BoneChannelPointers[bi].SizeY_index, EarliestNextFrame, &NextNext);
			float ScalingZ = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeZ, &BoneChannelPointers[bi].SizeZ_index, EarliestNextFrame, &NextNext);
			if (!float_equal(ScalingX, ScalingY) || !float_equal(ScalingY, ScalingZ))
				Warning("Non uniform scaling of bone in animation IPO not supported.", 537536);
			Keyframe->Bones[bi].Scaling = (ScalingX + ScalingY + ScalingZ) / 3.f;
		}
		OutAnimation->Keyframes.push_back(Keyframe);

		EarliestNextFrame = NextNext;
	}

	// Czas trwania animacji, skalowanie czasu
	if (OutAnimation->Keyframes.empty())
		OutAnimation->Length = 0.f;
	else if (OutAnimation->Keyframes.size() == 1)
	{
		OutAnimation->Length = 0.f;
		OutAnimation->Keyframes[0]->Time = 0.f;
	}
	else
	{
		float TimeOffset = -OutAnimation->Keyframes[0]->Time;
		for (uint ki = 0; ki < OutAnimation->Keyframes.size(); ki++)
			OutAnimation->Keyframes[ki]->Time += TimeOffset;
		OutAnimation->Length = OutAnimation->Keyframes[OutAnimation->Keyframes.size()-1]->Time;
	}

	if (OutAnimation->Keyframes.size() > (uint4)MAXUINT2)
		Error(Format("Too many keyframes in animation \"#\"") % OutAnimation->Name);
}

void TmpToQmsh_Animations(QMSH *OutQmsh, const tmp::QMSH &QmshTmp, const MeshJob &Job)
{
	Writeln("Processing animations...");

	for (uint i = 0; i < QmshTmp.Actions.size(); i++)
	{
		shared_ptr<QMSH_ANIMATION> Animation(new QMSH_ANIMATION);
		TmpToQmsh_Animation(Animation.get(), *QmshTmp.Actions[i].get(), *OutQmsh, QmshTmp);
		OutQmsh->Animations.push_back(Animation);
	}

	if (OutQmsh->Animations.size() > (uint4)MAXUINT2)
		Error("Too many animations");
}

struct INTERMEDIATE_VERTEX_SKIN_DATA
{
	uint1 Index1;
	uint1 Index2;
	float Weight1;
};

void CalcVertexSkinData(
	std::vector<INTERMEDIATE_VERTEX_SKIN_DATA> *Out,
	const tmp::MESH &TmpMesh,
	const QMSH &Qmsh,
	const tmp::QMSH &QmshTmp,
	const std::vector<tmp::BONE_INTER_DATA> &BoneInterData)
{
	Out->resize(TmpMesh.Vertices.size());

	// Kolejny algorytm pisany z wielk¹ nadziej¹ ¿e zadzia³a, wolny ale co z tego, lepszy taki
	// ni¿ ¿aden, i tak siê nad nim namyœla³em ca³y dzieñ, a drugi poprawia³em bo siê wszystko pomiesza³o!

	// Nie ma skinningu
	if (TmpMesh.ParentArmature.empty())
	{
		for (uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			(*Out)[vi].Index1 = 0;
			(*Out)[vi].Index2 = 0;
			(*Out)[vi].Weight1 = 1.f;
		}
	}
	// Jest skinning ca³ej siatki do jednej koœci
	else if (!TmpMesh.ParentBone.empty())
	{
		// ZnajdŸ indeks tej koœci
		uint1 BoneIndex = 0;
		for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			if (Qmsh.Bones[bi]->Name == TmpMesh.ParentBone)
			{
				BoneIndex = bi + 1;
				break;
			}
		}
		// Nie znaleziono
		if (BoneIndex == 0)
			Warning("Object parented to non existing bone.", 13497325);
		// Przypisz t¹ koœæ do wszystkich wierzcho³ków
		for (uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			(*Out)[vi].Index1 = BoneIndex;
			(*Out)[vi].Index2 = BoneIndex;
			(*Out)[vi].Weight1 = 1.f;
		}
	}
	// Vertex Group lub Envelope
	else
	{
		// U³ó¿ szybk¹ listê wierzcho³ków z wagami dla ka¿dej koœci

		// Typ reprezentuje dla danej koœci grupê wierzcho³ków: Indeks wierzcho³ka => Waga
		typedef std::map<uint, float> INFLUENCE_MAP;
		std::vector<INFLUENCE_MAP> BoneInfluences;
		BoneInfluences.resize(Qmsh.Bones.size());
		// Dla ka¿dej koœci
		for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			// ZnajdŸ odpowiadaj¹c¹ jej grupê wierzcho³ków w przetwarzanej w tym wywo³aniu funkcji siatce
			uint VertexGroupIndex = TmpMesh.VertexGroups.size();
			for (uint gi = 0; gi < TmpMesh.VertexGroups.size(); gi++)
			{
				if (TmpMesh.VertexGroups[gi]->Name == Qmsh.Bones[bi]->Name)
				{
					VertexGroupIndex = gi;
					break;
				}
			}
			// Jeœli znaleziono, przepisz wszystkie wierzcho³ki
			if (VertexGroupIndex < TmpMesh.VertexGroups.size())
			{
				const tmp::VERTEX_GROUP & VertexGroup = *TmpMesh.VertexGroups[VertexGroupIndex].get();
				for (uint vi = 0; vi < VertexGroup.VerticesInGroup.size(); vi++)
				{
					BoneInfluences[bi].insert(INFLUENCE_MAP::value_type(
						VertexGroup.VerticesInGroup[vi].Index,
						VertexGroup.VerticesInGroup[vi].Weight));
				}
			}
		}

		// Dla ka¿dego wierzcho³ka
		for (uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			// Zbierz wszystkie wp³ywaj¹ce na niego koœci
			struct INFLUENCE
			{
				uint BoneIndex; // Uwaga! Wy¿ej by³o VertexIndex -> Weight, a tutaj jest BoneIndex -> Weight - ale masakra !!!
				float Weight;

				bool operator > (const INFLUENCE &Influence) const { return Weight > Influence.Weight; }
			};
			std::vector<INFLUENCE> VertexInfluences;

			// Wp³yw przez Vertex Groups
			if (QmshTmp.Armature->VertexGroups)
			{
				for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					INFLUENCE_MAP::iterator biit = BoneInfluences[bi].find(vi);
					if (biit != BoneInfluences[bi].end())
					{
						INFLUENCE Influence = { bi + 1, biit->second };
						VertexInfluences.push_back(Influence);
					}
				}
			}

			// ¯adna koœæ na niego nie wp³ywa - wyp³yw z Envelopes
			if (VertexInfluences.empty() && QmshTmp.Armature->Envelopes)
			{
				// U³ó¿ listê wp³ywaj¹cych koœci
				for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					// Nie jestem pewny czy to jest dok³adnie algorytm u¿ywany przez Blender,
					// nie jest nigdzie dok³adnie opisany, ale z eksperymentów podejrzewam, ¿e tak.
					// Koœæ w promieniu - wp³yw maksymalny.
					// Koœæ w zakresie czegoœ co nazwa³em Extra Envelope te¿ jest wp³yw (podejrzewam ¿e s³abnie z odleg³oœci¹)
					// Promieñ Extra Envelope, jak uda³o mi siê wreszcie ustaliæ, jest niezale¿ny od Radius w sensie
					// ¿e rozci¹ga siê ponad Radius zawsze na tyle ile wynosi (BoneLength / 4)

					// Dodatkowy promieñ zewnêtrzny
					float ExtraEnvelopeRadius = BoneInterData[bi].Length * 0.25f;

					// Pozycja wierzcho³ka ju¿ jest w uk³adzie globalnym modelu i w konwencji DirectX
					float W = PointToBone(
						TmpMesh.Vertices[vi].Pos,
						BoneInterData[bi].HeadPos, BoneInterData[bi].HeadRadius, BoneInterData[bi].HeadRadius + ExtraEnvelopeRadius,
						BoneInterData[bi].TailPos, BoneInterData[bi].TailRadius, BoneInterData[bi].TailRadius + ExtraEnvelopeRadius);

					if (W > 0.f)
					{
						INFLUENCE Influence = { bi + 1, W };
						VertexInfluences.push_back(Influence);
					}
				}
			}
			// Jakieœ koœci na niego wp³ywaj¹ - weŸ z tych koœci

			// Zero koœci
			if (VertexInfluences.empty())
			{
				(*Out)[vi].Index1 = 0;
				(*Out)[vi].Index2 = 0;
				(*Out)[vi].Weight1 = 1.f;
			}
			// Tylko jedna koœæ
			else if (VertexInfluences.size() == 1)
			{
				(*Out)[vi].Index1 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Index2 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Weight1 = 1.f;
			}
			// Dwie lub wiêcej koœci na liœcie wp³ywaj¹cych na ten wierzcho³ek
			else
			{
				// Posortuj wp³ywy na wierzcho³ek malej¹co, czyli od najwiêkszej wagi
				std::sort(VertexInfluences.begin(), VertexInfluences.end(), std::greater<INFLUENCE>());
				// WeŸ dwie najwa¿niejsze koœci
				(*Out)[vi].Index1 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Index2 = VertexInfluences[1].BoneIndex;
				// Oblicz wagê pierwszej
				// WA¯NY WZÓR NA ZNORMALIZOWAN¥ WAGÊ PIERWSZ¥ Z DWÓCH DOWOLNYCH WAG !!!
				(*Out)[vi].Weight1 = VertexInfluences[0].Weight / (VertexInfluences[0].Weight + VertexInfluences[1].Weight);
			}
		}
	}
}

void ConvertQmshTmpToQmsh(QMSH *Out, const tmp::QMSH &QmshTmp, const MeshJob &Job)
{
	// BoneInterData odpowiada koœciom wyjœciowym QMSH.Bones, nie wejœciowym z TMP.Armature!!!
	std::vector<tmp::BONE_INTER_DATA> BoneInterData;

	Out->Flags = 0;
	if (Job.Tangents)
		Out->Flags |= QMSH::FLAG_TANGENTS;
	if (QmshTmp.Armature != NULL)
	{
		Out->Flags |= QMSH::FLAG_SKINNING;
		// Przetwórz koœci
		TmpToQmsh_Bones(Out, &BoneInterData, QmshTmp, Job);
		// Przetwórz animacje
		TmpToQmsh_Animations(Out, QmshTmp, Job);
	}

	Write("Processing mesh (NVMeshMender and more)...");

	struct NV_FACE
	{
		uint MaterialIndex;
		NV_FACE(uint MaterialIndex) : MaterialIndex(MaterialIndex) { }
	};

	// Dla kolejnych obiektów TMP
	for (uint oi = 0; oi < QmshTmp.Meshes.size(); oi++)
	{
		tmp::MESH & o = *QmshTmp.Meshes[oi].get();

		// Policz wp³yw koœci na wierzcho³ki tej siatki
		std::vector<INTERMEDIATE_VERTEX_SKIN_DATA> VertexSkinData;
		if ((Out->Flags & QMSH::FLAG_SKINNING) != 0)
			CalcVertexSkinData(&VertexSkinData, o, *Out, QmshTmp, BoneInterData);

		// Skonstruuj struktury poœrednie
		// UWAGA! Z³o¿onoœæ kwadratowa.
		// Myœla³em nad tym ca³y dzieñ i nic m¹drego nie wymyœli³em jak to ³adniej zrobiæ :)

		bool MaterialIndicesUse[16];
		ZeroMem(&MaterialIndicesUse, 16*sizeof(bool));

		std::vector<MeshMender::Vertex> NvVertices;
		std::vector<unsigned int> MappingNvToTmpVert;
		std::vector<unsigned int> NvIndices;
		std::vector<NV_FACE> NvFaces;
		std::vector<unsigned int> NvMappingOldToNewVert;

		NvVertices.reserve(o.Vertices.size());
		NvIndices.reserve(o.Faces.size()*4);
		NvFaces.reserve(o.Faces.size());
		NvMappingOldToNewVert.reserve(o.Vertices.size());

		// Dla kolejnych œcianek TMP

		for (uint fi = 0; fi < o.Faces.size(); fi++)
		{
			tmp::FACE & f = o.Faces[fi];

			assert((f.NumVertices == 3 || f.NumVertices == 4) && "tmp::FACE::NumVertices nieprawid³owe.");

			if (f.MaterialIndex >= 16)
				throw Error("Material index out of range.");
			MaterialIndicesUse[f.MaterialIndex] = true;

			// Pierwszy trójk¹t
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[0], o.Vertices[f.VertexIndices[0]], f.TexCoords[0]));
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[1], o.Vertices[f.VertexIndices[1]], f.TexCoords[1]));
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[2], o.Vertices[f.VertexIndices[2]], f.TexCoords[2]));
			// (wierzcho³ki dodane/zidenksowane, indeksy dodane - zostaje œcianka)
			NvFaces.push_back(NV_FACE(f.MaterialIndex));

			// Drugi trójk¹t
			if (f.NumVertices == 4)
			{
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[0], o.Vertices[f.VertexIndices[0]], f.TexCoords[0]));
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[2], o.Vertices[f.VertexIndices[2]], f.TexCoords[2]));
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[3], o.Vertices[f.VertexIndices[3]], f.TexCoords[3]));
				NvFaces.push_back(NV_FACE(f.MaterialIndex));
			}
		}

		// Struktury poœrednie chyba ju¿ poprawnie wype³nione, czas odpaliæ NVMeshMender!

		float MinCreaseCosAngle = cosf(DegToRad(o.AutoSmoothAngle)); // AutoSmoothAngle w przeciwieñstwie do k¹tów Eulera jest w stopniach!
		float WeightNormalsByArea = 1.0f;
		bool CalcNormals = true; // Musi byæ, bo w ogóle nie ma normalnych w QMSH TMP!
		bool RespectExistingSplits = Job.RespectExistingSplits;
		bool FixCylindricalWrapping = Job.FixCylindricalWrapping;

		MeshMender mender;
		if (!mender.Mend(
			NvVertices,
			NvIndices,
			NvMappingOldToNewVert,
			MinCreaseCosAngle, MinCreaseCosAngle, MinCreaseCosAngle,
			WeightNormalsByArea,
			CalcNormals ? MeshMender::CALCULATE_NORMALS : MeshMender::DONT_CALCULATE_NORMALS,
			RespectExistingSplits ? MeshMender::RESPECT_SPLITS : MeshMender::DONT_RESPECT_SPLITS,
			FixCylindricalWrapping ? MeshMender::FIX_CYLINDRICAL : MeshMender::DONT_FIX_CYLINDRICAL))
		{
			throw Error("NVMeshMender failed.");
		}

		assert(NvIndices.size() % 3 == 0);

		if (NvVertices.size() > (uint)MAXUINT2)
			throw Error(Format("Too many vertices in object \"#\": #") % o.Name % NvVertices.size());

		// Skonstruuj obiekty wyjœciowe QMSH

		// Liczba u¿ytych materia³ow
		uint NumMatUse = 0;
		for (uint mi = 0; mi < 16; mi++)
			if (MaterialIndicesUse[mi])
				NumMatUse++;

		// Wierzcho³ki (to bêdzie wspólne dla wszystkich podsiatek QMSH powsta³ych z bie¿¹cego obiektu QMSH TMP)
		uint MinVertexIndexUsed = Out->Vertices.size();
		uint NumVertexIndicesUsed = NvVertices.size();
		Out->Vertices.reserve(Out->Vertices.size() + NumVertexIndicesUsed);
		for (uint vi = 0; vi < NvVertices.size(); vi++)
		{
			QMSH_VERTEX v;
			v.Pos = math_cast<VEC3>(NvVertices[vi].pos);
			v.Tex.x = NvVertices[vi].s;
			v.Tex.y = NvVertices[vi].t;
			v.Normal = math_cast<VEC3>(NvVertices[vi].normal);
			v.Tangent = math_cast<VEC3>(NvVertices[vi].tangent);
			v.Binormal = math_cast<VEC3>(NvVertices[vi].binormal);
			if (QmshTmp.Armature != NULL)
			{
				const INTERMEDIATE_VERTEX_SKIN_DATA &ivsd = VertexSkinData[MappingNvToTmpVert[NvMappingOldToNewVert[vi]]];
				v.BoneIndices = (ivsd.Index1) | (ivsd.Index2 << 8);
				v.Weight1 = ivsd.Weight1;
			}
			Out->Vertices.push_back(v);
		}

		// Dla wszystkich u¿ytych materia³ów
		for (uint mi = 0; mi < 16; mi++)
		{
			if (MaterialIndicesUse[mi])
			{
				shared_ptr<QMSH_SUBMESH> submesh(new QMSH_SUBMESH);

				// Nazwa podsiatki
				// (Jeœli wiêcej u¿ytych materia³ów, to obiekt jest rozbijany na wiele podsiatek i do nazwy dodawany jest materia³)
				submesh->Name = (NumMatUse == 1 ? o.Name : o.Name + "." + o.Materials[mi]);
				// Nazwa materia³u
				if (mi < o.Materials.size())
					submesh->MaterialName = o.Materials[mi];
				else
					submesh->MaterialName = string();

				// Indeksy
				submesh->MinVertexIndexUsed = MinVertexIndexUsed;
				submesh->NumVertexIndicesUsed = NumVertexIndicesUsed;
				submesh->FirstTriangle = Out->Indices.size() / 3;

				// Dodaj indeksy
				submesh->NumTriangles = 0;
				Out->Indices.reserve(Out->Indices.size() + NvIndices.size());
				for (uint fi = 0, ii = 0; fi < NvFaces.size(); fi++, ii += 3)
				{
					if (NvFaces[fi].MaterialIndex == mi)
					{
						Out->Indices.push_back((uint2)(NvIndices[ii    ] + MinVertexIndexUsed));
						Out->Indices.push_back((uint2)(NvIndices[ii + 1] + MinVertexIndexUsed));
						Out->Indices.push_back((uint2)(NvIndices[ii + 2] + MinVertexIndexUsed));
						submesh->NumTriangles++;
					}
				}

				// Podsiatka gotowa
				Out->Submeshes.push_back(submesh);
			}
		}

		Write(".");
	}

	Writeln();
}

void LoadQmshTmp(QMSH *Out, const string &FileName, const MeshJob &Job)
{
	// Wczytaj plik QMSH.TMP
	Writeln("Loading QMSH TMP file \"" + FileName + "\"...");

	tmp::QMSH Tmp;
	LoadQmshTmpFile(&Tmp, FileName);

	// Przekszta³æ uk³ad wspó³rzêdnych
	TransformQmshTmpCoords(&Tmp);

	// Przekonwertuj do QMSH
	ConvertQmshTmpToQmsh(Out, Tmp, Job);
}

void LoadQmsh(QMSH &Qmsh, const string &FileName, const MeshJob &Job)
{
	Writeln("Loading QMSH file \"" + FileName + "\"...");

	ERR_TRY;

	FileStream F(FileName, FM_READ);

	// Nag³ówek
	uint2 VertexCount, TriangleCount, SubmeshCount, BoneCount, AnimationCount;
	string Header;
	F.ReadStringF(&Header, 8);
	if (Header != "TFQMSH10")
		throw Error("Invalid file header.");
	uint1 Flags; F.ReadEx(&Flags); Qmsh.Flags = Flags;
	F.ReadEx(&VertexCount);
	F.ReadEx(&TriangleCount);
	F.ReadEx(&SubmeshCount);
	F.ReadEx(&BoneCount);
	F.ReadEx(&AnimationCount);

	// Bry³y otaczaj¹ce
	F.ReadEx(&Qmsh.BoundingSphereRadius);
	F.ReadEx(&Qmsh.BoundingBox);
	Qmsh.BoundingVolumesCalculated = true;

	// Offsety - pomiñ
	F.Skip(5 * sizeof(uint4));

	// Wierzcho³ki
	Qmsh.Vertices.resize(VertexCount);
	{ uint1 Zero; F.ReadEx(&Zero); if (Zero != 0) throw Error("File is corrupted at vertex buffer beginning."); }
	for (uint vi = 0; vi < VertexCount; vi++)
	{
		QMSH_VERTEX & v = Qmsh.Vertices[vi];

		F.ReadEx(&v.Pos);

		if ((Qmsh.Flags & QMSH::FLAG_SKINNING) != 0)
		{
			F.ReadEx(&v.Weight1);
			F.ReadEx(&v.BoneIndices);
		}

		F.ReadEx(&v.Normal);
		F.ReadEx(&v.Tex);

		if ((Qmsh.Flags & QMSH::FLAG_TANGENTS) != 0)
		{
			F.ReadEx(&v.Tangent);
			F.ReadEx(&v.Binormal);
		}
	}

	// Indeksy (trójk¹ty)
	Qmsh.Indices.resize(TriangleCount * 3);
	{ uint1 Zero; F.ReadEx(&Zero); if (Zero != 0) throw Error("File is corrupted at vertex buffer beginning."); }
	F.MustRead(&Qmsh.Indices[0], sizeof(uint2)*Qmsh.Indices.size());

	// Podsiatki
	{ uint1 Zero; F.ReadEx(&Zero); if (Zero != 0) throw Error("File is corrupted at vertex buffer beginning."); }
	for (uint si = 0; si < SubmeshCount; si++)
	{
		shared_ptr<QMSH_SUBMESH> Submesh(new QMSH_SUBMESH);
		Qmsh.Submeshes.push_back(Submesh);
		QMSH_SUBMESH & s = *Submesh.get();

		uint2 u;
		F.ReadEx(&u); s.FirstTriangle = u;
		F.ReadEx(&u); s.NumTriangles = u;
		F.ReadEx(&u); s.MinVertexIndexUsed = u;
		F.ReadEx(&u); s.NumVertexIndicesUsed = u;
		F.ReadString1(&s.Name);
		F.ReadString1(&s.MaterialName);
	}

	// Koœci
	if ((Qmsh.Flags & QMSH::FLAG_SKINNING) != 0)
	{
		{ uint1 Zero; F.ReadEx(&Zero); if (Zero != 0) throw Error("File is corrupted at vertex buffer beginning."); }
		for (uint bi = 0; bi < BoneCount; bi++)
		{
			shared_ptr<QMSH_BONE> Bone(new QMSH_BONE);
			Qmsh.Bones.push_back(Bone);
			QMSH_BONE &b = *Bone.get();

			uint2 ParentIndex;
			F.ReadEx(&ParentIndex); b.ParentIndex = ParentIndex;

			F.ReadEx(&b.Matrix._11);
			F.ReadEx(&b.Matrix._12);
			F.ReadEx(&b.Matrix._13);
			F.ReadEx(&b.Matrix._21);
			F.ReadEx(&b.Matrix._22);
			F.ReadEx(&b.Matrix._23);
			F.ReadEx(&b.Matrix._31);
			F.ReadEx(&b.Matrix._32);
			F.ReadEx(&b.Matrix._33);
			F.ReadEx(&b.Matrix._41);
			F.ReadEx(&b.Matrix._42);
			F.ReadEx(&b.Matrix._43);

			F.ReadString1(&b.Name);
		}
	}
	else
	{
		if (BoneCount > 0)
			Warning("Bone count not zero while flags indicate no skinning data.");
	}

	// Animacje
	if ((Qmsh.Flags & QMSH::FLAG_SKINNING) != 0)
	{
		{ uint1 Zero; F.ReadEx(&Zero); if (Zero != 0) throw Error("File is corrupted at vertex buffer beginning."); }
		for (uint ai = 0; ai < AnimationCount; ai++)
		{
			shared_ptr<QMSH_ANIMATION> AnimationPtr(new QMSH_ANIMATION);
			Qmsh.Animations.push_back(AnimationPtr);
			QMSH_ANIMATION & Animation = *AnimationPtr.get();

			uint2 KeyframeCount;
			F.ReadString1(&Animation.Name);
			F.ReadEx(&Animation.Length);
			F.ReadEx((uint2*)&KeyframeCount);

			for (uint ki = 0; ki < KeyframeCount; ki++)
			{
				shared_ptr<QMSH_KEYFRAME> KeyframePtr(new QMSH_KEYFRAME);
				Animation.Keyframes.push_back(KeyframePtr);
				QMSH_KEYFRAME &Keyframe = *KeyframePtr.get();

				F.ReadEx(&Keyframe.Time);
				Keyframe.Bones.resize(Qmsh.Bones.size());
				for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					F.ReadEx(&Keyframe.Bones[bi].Translation);
					F.ReadEx(&Keyframe.Bones[bi].Rotation);
					F.ReadEx(&Keyframe.Bones[bi].Scaling);
				}
			}
		}
	}
	else
	{
		if (AnimationCount > 0)
			Warning("Animation count not zero while flags indicate no skinning data.");
	}

	ERR_CATCH_FUNC("Cannot load mesh from file: " + FileName);
}

// Zapisuje QMSH do pliku
void SaveQmsh(const QMSH &Qmsh, const string &FileName)
{
	ERR_TRY;

	Writeln("Saving QMSH file \"" + FileName + "\"...");

	FileStream F(FileName, FM_WRITE);

	assert(Qmsh.Flags < 0xFF);
	assert(Qmsh.Indices.size() % 3 == 0);

	// Nag³ówek
	F.WriteStringF("TFQMSH10");
	F.WriteEx((uint1)Qmsh.Flags);
	F.WriteEx((uint2)Qmsh.Vertices.size());
	F.WriteEx((uint2)(Qmsh.Indices.size()/3));
	F.WriteEx((uint2)Qmsh.Submeshes.size());
	F.WriteEx((uint2)Qmsh.Bones.size());
	F.WriteEx((uint2)Qmsh.Animations.size());

	// Bry³y otaczaj¹ce
	assert(Qmsh.BoundingVolumesCalculated);
	F.WriteEx(Qmsh.BoundingSphereRadius);
	F.WriteEx(Qmsh.BoundingBox);

	// Ta czêœæ nag³ówka to offsety - jest uzupe³niana na koñcu
	int HeaderOffsetsPos = F.GetPos();
	F.WriteEx(0xDEADC0DE);
	F.WriteEx(0xDEADC0DE);
	F.WriteEx(0xDEADC0DE);
	F.WriteEx(0xDEADC0DE);
	F.WriteEx(0xDEADC0DE);

	// Wierzcho³ki
	int VerticesPos = F.GetPos();
	F.WriteEx((uint1)0);
	for (uint vi = 0; vi < Qmsh.Vertices.size(); vi++)
	{
		const QMSH_VERTEX & v = Qmsh.Vertices[vi];

		F.WriteEx(v.Pos);

		if ((Qmsh.Flags & QMSH::FLAG_SKINNING) != 0)
		{
			F.WriteEx(v.Weight1);
			F.WriteEx(v.BoneIndices);
		}

		F.WriteEx(v.Normal);
		F.WriteEx(v.Tex);

		if ((Qmsh.Flags & QMSH::FLAG_TANGENTS) != 0)
		{
			F.WriteEx(v.Tangent);
			F.WriteEx(v.Binormal);
		}
	}

	// Indeksy (trójk¹ty)
	int TrianglesPos = F.GetPos();
	F.WriteEx((uint1)0);
	F.Write(&Qmsh.Indices[0], sizeof(uint2)*Qmsh.Indices.size());

	// Podsiatki
	int SubmeshesPos = F.GetPos();
	F.WriteEx((uint1)0);
	for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
	{
		const QMSH_SUBMESH & s = *Qmsh.Submeshes[si].get();

		F.WriteEx((uint2)s.FirstTriangle);
		F.WriteEx((uint2)s.NumTriangles);
		F.WriteEx((uint2)s.MinVertexIndexUsed);
		F.WriteEx((uint2)s.NumVertexIndicesUsed);
		F.WriteString1(s.Name);
		F.WriteString1(s.MaterialName);
	}

	// Koœci
	int BonesPos;
	if ((Qmsh.Flags & QMSH::FLAG_SKINNING) != 0)
	{
		BonesPos = F.GetPos();
		F.WriteEx((uint1)0);
		for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			F.WriteEx((uint2)Qmsh.Bones[bi]->ParentIndex);

			F.WriteEx(Qmsh.Bones[bi]->Matrix._11);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._12);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._13);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._21);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._22);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._23);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._31);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._32);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._33);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._41);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._42);
			F.WriteEx(Qmsh.Bones[bi]->Matrix._43);

			F.WriteString1(Qmsh.Bones[bi]->Name);
		}
	}
	else
		BonesPos = 0;

	// Animacje
	int AnimationsPos;
	if ((Qmsh.Flags & QMSH::FLAG_SKINNING) != 0)
	{
		AnimationsPos = F.GetPos();
		F.WriteEx((uint1)0);
		for (uint ai = 0; ai < Qmsh.Animations.size(); ai++)
		{
			const QMSH_ANIMATION & Animation = *Qmsh.Animations[ai].get();

			F.WriteString1(Animation.Name);
			F.WriteEx(Animation.Length);
			F.WriteEx((uint2)Animation.Keyframes.size());

			for (uint ki = 0; ki < Animation.Keyframes.size(); ki++)
			{
				const QMSH_KEYFRAME & Keyframe = *Animation.Keyframes[ki].get();

				F.WriteEx(Keyframe.Time);

				for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					F.WriteEx(Keyframe.Bones[bi].Translation);
					F.WriteEx(Keyframe.Bones[bi].Rotation);
					F.WriteEx(Keyframe.Bones[bi].Scaling);
				}
			}
		}
	}
	else
		AnimationsPos = 0;

	// Indeksy
	F.SetPos(HeaderOffsetsPos);
	F.WriteEx(VerticesPos);
	F.WriteEx(TrianglesPos);
	F.WriteEx(SubmeshesPos);
	F.WriteEx(BonesPos);
	F.WriteEx(AnimationsPos);

	ERR_CATCH_FUNC("Cannot save mesh to file: " + FileName);
}

void InputTask(QMSH &Qmsh, MeshJob &Job, InputMeshTask &Task)
{
	// Wykryj typ
	string InputFileNameU; ExtractFileName(&InputFileNameU, Task.FileName);
	UpperCase(&InputFileNameU);

	QMSH Mesh;

	// Plik wejœciowy to QMSH.TMP
	if (StrEnds(InputFileNameU, ".QMSH.TMP"))
		LoadQmshTmp(&Qmsh, Task.FileName, Job);
	// Plik wejœciowy to QMSH
	else if (StrEnds(InputFileNameU, ".QMSH"))
		LoadQmsh(Qmsh, Task.FileName, Job);
	else
		throw Error("Nierozpoznany typ pliku wejœciowego (nieznane rozszerzenie): " + Task.FileName);
}

void OutputTask(QMSH &Qmsh, MeshJob &Job, OutputMeshTask &Task)
{
	EnsureBoundingVolumes(Qmsh);

	SaveQmsh(Qmsh, Task.FileName);
}

void InfoTask(QMSH &Qmsh, MeshJob &Job, InfoMeshTask &Task)
{
	EnsureBoundingVolumes(Qmsh);

	assert(Qmsh.Indices.size() % 3 == 0);

	Writeln("Info:");
	Writeln(Format("  Flags: Tangents=#, Skinning=#") %
		((Qmsh.Flags & QMSH::FLAG_TANGENTS) != 0) %
		((Qmsh.Flags & QMSH::FLAG_SKINNING) != 0));
	Writeln(Format("  Stats: Submeshes=#, Vertices=#, Triangles=#") %
		Qmsh.Submeshes.size() %
		Qmsh.Vertices.size() %
		(Qmsh.Indices.size() / 3));
	Writeln(Format("  BoundingSphereRadius=#") % Qmsh.BoundingSphereRadius);
	Writeln(Format("  BoundingBox=#") % Qmsh.BoundingBox);
	if ((Qmsh.Flags & QMSH::FLAG_SKINNING) != 0)
	{
		Writeln(Format("  Skinning: Bones=#, Animations=#") %
			Qmsh.Bones.size() %
			Qmsh.Animations.size());
	}
	else
	{
		assert(Qmsh.Bones.empty());
		assert(Qmsh.Animations.empty());
	}

	if (Task.Detailed)
	{
		for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
		{
			Writeln(Format("  Submesh: Name=\"#\", Material=\"#\", Triangles=#") %
				Qmsh.Submeshes[si]->Name %
				Qmsh.Submeshes[si]->MaterialName %
				Qmsh.Submeshes[si]->NumTriangles);
		}
		for (uint ai = 0; ai < Qmsh.Animations.size(); ai++)
		{
			Writeln(Format("  Animation: Name=\"#\", Length=# s, Keyframes=#") %
				Qmsh.Animations[ai]->Name %
				Qmsh.Animations[ai]->Length %
				Qmsh.Animations[ai]->Keyframes.size());
		}
	}
}

// Oblicza œredni¹ pozycji wszystkich wierzcho³ków
void CalcAveragePos(VEC3 *OutAveragePos, const std::vector<QMSH_VERTEX> &Vertices)
{
	if (Vertices.empty())
	{
		*OutAveragePos = VEC3::ZERO;
		return;
	}

	*OutAveragePos = Vertices[0].Pos;

	for (uint i = 1; i < Vertices.size(); i++)
		*OutAveragePos += Vertices[i].Pos;

	*OutAveragePos /= (float)Vertices.size();
}

class VecComp_x_less { public: bool operator () (const VEC3 &v1, const VEC3 &v2) { return v1.x < v2.x; } };
class VecComp_y_less { public: bool operator () (const VEC3 &v1, const VEC3 &v2) { return v1.y < v2.y; } };
class VecComp_z_less { public: bool operator () (const VEC3 &v1, const VEC3 &v2) { return v1.z < v2.z; } };

// Oblicza medianê pozycji wszystkich wierzcho³ków
void CalcMedianPos(VEC3 *OutMedianPos, const std::vector<QMSH_VERTEX> &Vertices)
{
	if (Vertices.empty())
	{
		*OutMedianPos = VEC3::ZERO;
		return;
	}

	// Przepisz wszystkie pozycje do wektora
	std::vector<VEC3> Positions;
	Positions.resize(Vertices.size());
	for (uint i = 0; i < Vertices.size(); i++)
		Positions[i] = Vertices[i].Pos;

	// Wybierz element n/2-gi
	uint Pos = Vertices.size() / 2;

	std::nth_element(Positions.begin(), Positions.begin() + Pos, Positions.end(), VecComp_x_less());
	OutMedianPos->x = Positions[Pos].x;
	std::nth_element(Positions.begin(), Positions.begin() + Pos, Positions.end(), VecComp_y_less());
	OutMedianPos->y = Positions[Pos].y;
	std::nth_element(Positions.begin(), Positions.begin() + Pos, Positions.end(), VecComp_z_less());
	OutMedianPos->z = Positions[Pos].z;
}

// Podana nazwa obiektu, znajduje zakres wierzcho³ków z tablicy wierzcho³ków nale¿¹cych do tego obiektu.
// Jeœli nazwa pusta, zwraca pe³ny zakres wszystkich wierzcho³ków.
// Jeœli nazwa niepusta ale obiekt nieznaleziony, rzuca wyj¹tek.
void FindObjectVertexRange(const QMSH &Qmsh, uint *OutVertexBeg, uint *OutVertexEnd, const string &ObjectName)
{
	*OutVertexBeg = 0;
	*OutVertexEnd = Qmsh.Vertices.size();

	if (!ObjectName.empty())
	{
		bool Found = false;
		for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
		{
			if (Qmsh.Submeshes[si]->Name == ObjectName)
			{
				*OutVertexBeg = Qmsh.Submeshes[si]->MinVertexIndexUsed;
				*OutVertexEnd = Qmsh.Submeshes[si]->MinVertexIndexUsed + Qmsh.Submeshes[si]->NumVertexIndicesUsed;
				Found = true;
				break;
			}
		}
		if (!Found)
			throw Error(Format("Object \"#\" not found.") % ObjectName);
	}
}

void TransformTask(QMSH &Qmsh, MeshJob &Job, TransformMeshTask &Task)
{
	// Uzupe³nij transformacjê, jeœli trzeba
	MATRIX TransformMatrix = Task.Transform;
	// - CenterOnAverage
	if (Task.Type == 3007)
	{
		VEC3 Average;
		CalcAveragePos(&Average, Qmsh.Vertices);
		Translation(&TransformMatrix, -Average);
	}
	// - CenterOnMedian
	else if (Task.Type == 3008)
	{
		VEC3 Median;
		CalcMedianPos(&Median, Qmsh.Vertices);
		Translation(&TransformMatrix, -Median);
	}
	// - TransformToBox
	else if (Task.Type == 3010)
	{
		EnsureBoundingVolumes(Qmsh);
		MATRIX Trans1, Scal, Trans2;
		Translation(&Trans1, -Qmsh.BoundingBox.p1);
		Scaling(&Scal,
			(Task.Box.p2.x - Task.Box.p1.x) / (Qmsh.BoundingBox.p2.x - Qmsh.BoundingBox.p1.x),
			(Task.Box.p2.y - Task.Box.p1.y) / (Qmsh.BoundingBox.p2.y - Qmsh.BoundingBox.p1.y),
			(Task.Box.p2.z - Task.Box.p1.z) / (Qmsh.BoundingBox.p2.z - Qmsh.BoundingBox.p1.z));
		Translation(&Trans2, Task.Box.p1);
		TransformMatrix = Trans1 * Scal * Trans2;
	}
	else if (Task.Type == 3011)
	{
		EnsureBoundingVolumes(Qmsh);
		MATRIX Trans1, Scal, Trans2;
		Translation(&Trans1, -Qmsh.BoundingBox.p1);
		Scaling(&Scal,
			min3(
				(Task.Box.p2.x - Task.Box.p1.x) / (Qmsh.BoundingBox.p2.x - Qmsh.BoundingBox.p1.x),
				(Task.Box.p2.y - Task.Box.p1.y) / (Qmsh.BoundingBox.p2.y - Qmsh.BoundingBox.p1.y),
				(Task.Box.p2.z - Task.Box.p1.z) / (Qmsh.BoundingBox.p2.z - Qmsh.BoundingBox.p1.z)));
		Translation(&Trans2, Task.Box.p1);
		TransformMatrix = Trans1 * Scal * Trans2;
	}

	MATRIX TransformInverseTranspose;
	Inverse(&TransformInverseTranspose, TransformMatrix);
	Transpose(&TransformInverseTranspose);

	// Wyznacz zakres wierzcho³ków do przekszta³cenia
	uint VertexBeg, VertexEnd;
	FindObjectVertexRange(Qmsh, &VertexBeg, &VertexEnd, Task.ObjectName);

	// WprowadŸ przekszta³cenie do wierzcho³ków
	Writeln("Transforming vertex positions...");
	VEC3 TmpVec;
	for (uint vi = VertexBeg; vi < VertexEnd; vi++)
	{
		// Pozycja
		TransformCoord(&TmpVec, Qmsh.Vertices[vi].Pos, TransformMatrix);
		Qmsh.Vertices[vi].Pos = TmpVec;
		// Normalna
		TransformNormal(&TmpVec, Qmsh.Vertices[vi].Normal, TransformInverseTranspose);
		Qmsh.Vertices[vi].Normal;
		// Tangent i Binormal
		if ((Qmsh.Flags & QMSH::FLAG_TANGENTS) != 0)
		{
			TransformNormal(&TmpVec, Qmsh.Vertices[vi].Tangent, TransformInverseTranspose);
			Qmsh.Vertices[vi].Tangent;
			TransformNormal(&TmpVec, Qmsh.Vertices[vi].Binormal, TransformInverseTranspose);
			Qmsh.Vertices[vi].Binormal;
		}
	}

	// WprowadŸ przekszta³cenia do koœci
	// Tylko jeœli przekszta³cenie dotyczy ca³ego obiektu.
	if (Task.ObjectName.empty() && (Qmsh.Flags & QMSH::FLAG_SKINNING) != 0)
	{
		for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			// Jeœli to koœæ g³ównego poziomu, jej transformacja jest transformacj¹ ze wsp. tej koœci do
			// wsp. g³ównych modelu - dodaj now¹ transformacjê.
			if (Qmsh.Bones[bi]->ParentIndex == 0)
			{
				Qmsh.Bones[bi]->Matrix = Qmsh.Bones[bi]->Matrix * TransformMatrix;
			}
		}
	}

	InvalidateBoundingVolumes(Qmsh);
}

void TransformTexTask(QMSH &Qmsh, MeshJob &Job, TransformTexMeshTask &Task)
{
	// Wyznacz zakres wierzcho³ków do przekszta³cenia
	uint VertexBeg, VertexEnd;
	FindObjectVertexRange(Qmsh, &VertexBeg, &VertexEnd, Task.ObjectName);

	Writeln("Transforming texture coordinates...");

	VEC2 TmpVec;
	for (uint vi = VertexBeg; vi < VertexEnd; vi++)
	{
		TransformCoord(&TmpVec, Qmsh.Vertices[vi].Tex, Task.Transform);
		Qmsh.Vertices[vi].Tex = TmpVec;
	}
}

void NormalizeNormalsTask(QMSH &Qmsh, MeshJob &Job, NormalizeNormalsMeshTask &Task)
{
	Writeln("Normalizing normals...");

	for (uint vi = 0; vi < Qmsh.Vertices.size(); vi++)
	{
		QMSH_VERTEX &v = Qmsh.Vertices[vi];
		if (v.Normal != VEC3::ZERO)
			Normalize(&v.Normal);
		if ((Qmsh.Flags & QMSH::FLAG_TANGENTS) != 0)
		{
			if (v.Tangent != VEC3::ZERO)
				Normalize(&v.Tangent);
			if (v.Binormal != VEC3::ZERO)
				Normalize(&v.Binormal);
		}
	}
}

void ZeroNormalsTask(QMSH &Qmsh, MeshJob &Job, ZeroNormalsMeshTask &Task)
{
	Writeln("Zeroing normals...");

	for (uint vi = 0; vi < Qmsh.Vertices.size(); vi++)
	{
		QMSH_VERTEX &v = Qmsh.Vertices[vi];
		v.Normal = VEC3::ZERO;

		if ((Qmsh.Flags & QMSH::FLAG_TANGENTS) != 0)
		{
			v.Tangent = VEC3::ZERO;
			v.Binormal = VEC3::ZERO;
		}
	}
}

void DeleteTangentsTask(QMSH &Qmsh, MeshJob &Job, DeleteTangentsMeshTask &Task)
{
	Writeln("Deleting tangents...");

	Qmsh.Flags &= ~QMSH::FLAG_TANGENTS;
}

void DeleteSkinningTask(QMSH &Qmsh, MeshJob &Job, DeleteSkinningMeshTask &Task)
{
	Writeln("Deleting all skinning data...");

	Qmsh.Flags &= ~QMSH::FLAG_SKINNING;
	Qmsh.Bones.clear();
	Qmsh.Animations.clear();
}

void DeleteObjectTask(QMSH &Qmsh, MeshJob &Job, DeleteObjectMeshTask &Task)
{
	Writeln(Format("Deleting object \"#\"...") % Task.Name);

	for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
	{
		// Znaleziony - dalej ju¿ po si nie pójdziemy
		if (Qmsh.Submeshes[si]->Name == Task.Name)
		{
			QMSH_SUBMESH &Submesh = *Qmsh.Submeshes[si].get();
			// Usuñ indeksy
			uint IndexOffset = Submesh.NumTriangles * 3;
			Qmsh.Indices.erase(
				Qmsh.Indices.begin() + Submesh.FirstTriangle * 3,
				Qmsh.Indices.begin() + (Submesh.FirstTriangle + Submesh.NumTriangles) * 3);
			// Usuñ wierzcho³ki
			uint VertexOffset = Submesh.NumVertexIndicesUsed;
			Qmsh.Vertices.erase(
				Qmsh.Vertices.begin() + Submesh.MinVertexIndexUsed,
				Qmsh.Vertices.begin() + Submesh.MinVertexIndexUsed + Submesh.NumVertexIndicesUsed);
			// Przesuñ wszystkie pozosta³e indeksy (te z tej podsiatki s¹ ju¿ usuniête)
			for (uint ii = Submesh.FirstTriangle*3; ii < Qmsh.Indices.size(); ii++)
				Qmsh.Indices[ii] -= (uint2)VertexOffset;
			// Przesuñ numery wierzcho³ków i indeksów w pozosta³ych obiektach (bie¿¹cy jeszcze nieusuniêty)
			for (uint si2 = si+1; si2 < Qmsh.Submeshes.size(); si2++)
			{
				Qmsh.Submeshes[si2]->FirstTriangle -= IndexOffset / 3;
				Qmsh.Submeshes[si2]->MinVertexIndexUsed -= VertexOffset;
			}
			// Usuñ bie¿¹cy obiekt
			Qmsh.Submeshes.erase(Qmsh.Submeshes.begin() + si);
			// Koniec
			InvalidateBoundingVolumes(Qmsh);
			return;
		}
	}

	throw Error(Format("Object \"#\" not found.") % Task.Name);
}

void RenameObjectTask(QMSH &Qmsh, MeshJob &Job, RenameObjectMeshTask &Task)
{
	Writeln(Format("Renaming object \"#\" to \"#\".") % Task.OldName % Task.NewName);

	for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
	{
		if (Qmsh.Submeshes[si]->Name == Task.OldName)
		{
			Qmsh.Submeshes[si]->Name = Task.NewName;
			return;
		}
	}

	throw Error(Format("Object \"#\" not found.") % Task.OldName);
}

void RenameMaterialTask(QMSH &Qmsh, MeshJob &Job, RenameMaterialMeshTask &Task)
{
	Writeln(Format("Renaming material \"#\" to \"#\".") % Task.OldName % Task.NewName);

	for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
	{
		if (Qmsh.Submeshes[si]->MaterialName == Task.OldName)
			Qmsh.Submeshes[si]->MaterialName = Task.NewName;
	}
}

void SetMaterialTask(QMSH &Qmsh, MeshJob &Job, SetMaterialMeshTask &Task)
{
	Writeln(Format("Setting material of \"#\" to \"#\".") % Task.ObjectName % Task.NewMaterialName);

	for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
	{
		if (Qmsh.Submeshes[si]->Name == Task.ObjectName)
			Qmsh.Submeshes[si]->MaterialName = Task.NewMaterialName;
	}
}

void SetAllMaterialsTask(QMSH &Qmsh, MeshJob &Job, SetAllMaterialsMeshTask &Task)
{
	Writeln(Format("Setting all materials to \"#\".") % Task.NewMaterialName);

	for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
		Qmsh.Submeshes[si]->MaterialName = Task.NewMaterialName;
}

void RenameBoneTask(QMSH &Qmsh, MeshJob &Job, RenameBoneMeshTask &Task)
{
	Writeln(Format("Renaming bone \"#\" to \"#\".") % Task.OldName % Task.NewName);

	uint c = 0;
	for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
	{
		if (Qmsh.Bones[bi]->Name == Task.OldName)
		{
			Qmsh.Bones[bi]->Name = Task.NewName;
			c++;
		}
	}

	if (c == 0)
		throw Error(Format("Bone \"#\" not found.") % Task.OldName);
}

void RenameAnimationTask(QMSH &Qmsh, MeshJob &Job, RenameAnimationMeshTask &Task)
{
	Writeln(Format("Renaming animation \"#\" to \"#\".") % Task.OldName % Task.NewName);

	uint c = 0;
	for (uint ai = 0; ai < Qmsh.Animations.size(); ai++)
	{
		if (Qmsh.Animations[ai]->Name == Task.OldName)
		{
			Qmsh.Animations[ai]->Name = Task.NewName;
			c++;
		}
	}

	if (c == 0)
		throw Error(Format("Animation \"#\" not found.") % Task.OldName);
}

void DeleteAnimationTask(QMSH &Qmsh, MeshJob &Job, DeleteAnimationMeshTask &Task)
{
	Writeln(Format("Deleting animation \"#\".") % Task.Name);

	for (uint ai = 0; ai < Qmsh.Animations.size(); ai++)
	{
		if (Qmsh.Animations[ai]->Name == Task.Name)
		{
			Qmsh.Animations.erase(Qmsh.Animations.begin() + ai);
			return;
		}
	}

	throw Error(Format("Animation \"#\" not found.") % Task.Name);
}

void DeleteAllAnimationsTask(QMSH &Qmsh, MeshJob &Job, DeleteAllAnimationsMeshTask &Task)
{
	Writeln("Deleting all animations.");

	Qmsh.Animations.clear();
}

void ScaleTimeTask(QMSH &Qmsh, MeshJob &Job, ScaleTimeMeshTask &Task)
{
	Writeln("Scaling animation time...");

	uint Counter = 0;
	for (uint ai = 0; ai < Qmsh.Animations.size(); ai++)
	{
		if (Task.AnimationName.empty() || Qmsh.Animations[ai]->Name == Task.AnimationName)
		{
			Qmsh.Animations[ai]->Length *= Task.Scale;
			Counter++;
			for (uint ki = 0; ki < Qmsh.Animations[ai]->Keyframes.size(); ki++)
				Qmsh.Animations[ai]->Keyframes[ki]->Time *= Task.Scale;
		}
	}

	if (!Task.AnimationName.empty() && Counter == 0)
		throw Error(Format("Animation \"#\" not found.") % Task.AnimationName);
}

void OptimizeAnimationsTask(QMSH &Qmsh, MeshJob &Job, OptimizeAnimationsMeshTask &Task)
{
	Writeln("Optimizing animations...");

	uint Counter = 0;
	VEC3 TmpVec;
	float TmpF;
	QUATERNION TmpQuat;
	float Epsilon, LerpT;

	for (uint ai = 0; ai < Qmsh.Animations.size(); ai++)
	{
		QMSH_ANIMATION &Animation = *Qmsh.Animations[ai].get();
		// Dla ka¿dego niepierwszego i nieostatniego Keyframe
		uint ki = 1;
		while (ki < Animation.Keyframes.size() - 1)
		{
			QMSH_KEYFRAME &Keyframe = *Animation.Keyframes[ki].get();
			// Czy ta klatka ma znaczenie
			bool HasMeaning = false;
			LerpT = (Keyframe.Time - Animation.Keyframes[ki-1]->Time) / (Animation.Keyframes[ki+1]->Time - Animation.Keyframes[ki-1]->Time);
			// Szukaj po wszystkich jej koœciach
			for (uint bi = 0; bi < Keyframe.Bones.size(); bi++)
			{
				// Jaka by tu by³a translacja z interpolacji liniowej s¹siednich Keyframe
				Lerp(&TmpVec,
					Animation.Keyframes[ki-1]->Bones[bi].Translation,
					Animation.Keyframes[ki+1]->Bones[bi].Translation,
					LerpT);
				Epsilon = 0.001f * min3(
					fabsf(Animation.Keyframes[ki+1]->Bones[bi].Translation.x - Animation.Keyframes[ki-1]->Bones[bi].Translation.x),
					fabsf(Animation.Keyframes[ki+1]->Bones[bi].Translation.y - Animation.Keyframes[ki-1]->Bones[bi].Translation.y),
					fabsf(Animation.Keyframes[ki+1]->Bones[bi].Translation.z - Animation.Keyframes[ki-1]->Bones[bi].Translation.z));
				if (!around(Keyframe.Bones[bi].Translation.x, TmpVec.x, Epsilon) ||
					!around(Keyframe.Bones[bi].Translation.y, TmpVec.y, Epsilon) ||
					!around(Keyframe.Bones[bi].Translation.z, TmpVec.z, Epsilon))
				{
					HasMeaning = true;
					break;
				}
				// Jakie by tu by³o skalowanie z interpolacji liniowej s¹siednich Keyframe
				TmpF = Lerp(
					Animation.Keyframes[ki-1]->Bones[bi].Scaling,
					Animation.Keyframes[ki+1]->Bones[bi].Scaling,
					LerpT);
				Epsilon = 0.001f * fabsf(Animation.Keyframes[ki+1]->Bones[bi].Scaling - Animation.Keyframes[ki-1]->Bones[bi].Scaling);
				if (!around(Keyframe.Bones[bi].Scaling, TmpF, Epsilon))
				{
					HasMeaning = true;
					break;
				}
				// Jaka by tu by³a rotacja z interpolacji sferycznej s¹siednich Keyframe
				Slerp(&TmpQuat,
					Animation.Keyframes[ki-1]->Bones[bi].Rotation,
					Animation.Keyframes[ki+1]->Bones[bi].Rotation,
					LerpT);
				Epsilon = 0.001f * std::min(min3(
					fabsf(Animation.Keyframes[ki+1]->Bones[bi].Rotation.x - Animation.Keyframes[ki-1]->Bones[bi].Rotation.x),
					fabsf(Animation.Keyframes[ki+1]->Bones[bi].Rotation.y - Animation.Keyframes[ki-1]->Bones[bi].Rotation.y),
					fabsf(Animation.Keyframes[ki+1]->Bones[bi].Rotation.z - Animation.Keyframes[ki-1]->Bones[bi].Rotation.z)),
					fabsf(Animation.Keyframes[ki+1]->Bones[bi].Rotation.w - Animation.Keyframes[ki-1]->Bones[bi].Rotation.w));
				if (!around(Keyframe.Bones[bi].Rotation.x, TmpQuat.x, Epsilon) ||
					!around(Keyframe.Bones[bi].Rotation.y, TmpQuat.y, Epsilon) ||
					!around(Keyframe.Bones[bi].Rotation.z, TmpQuat.z, Epsilon) ||
					!around(Keyframe.Bones[bi].Rotation.w, TmpQuat.w, Epsilon))
				{
					HasMeaning = true;
					break;
				}
			}
			// Ta klatka ma znaczenie - zostaw j¹
			if (HasMeaning)
				ki++;
			// Ta klatka nie ma znaczenia - skasuj j¹
			else
			{
				Animation.Keyframes.erase(Animation.Keyframes.begin() + ki);
				Counter++;
			}
		}
	}

	if (Counter == 0)
		Writeln("  No keyframes deleted.");
	else
		Writeln(Format("  Deleted # keyframes.") % Counter);
}

void RecalcBoundingsTask(QMSH &Qmsh, MeshJob &Job, RecalcBoundingsMeshTask &Task)
{
	CalcBoundingVolumes(Qmsh);
}

void FlipNormalsTask(QMSH &Qmsh, MeshJob &Job, FlipNormalsMeshTask &Task)
{
	Writeln("Flipping normals...");

	for (uint vi = 0; vi < Qmsh.Vertices.size(); vi++)
	{
		Qmsh.Vertices[vi].Normal = -Qmsh.Vertices[vi].Normal;
		Qmsh.Vertices[vi].Tangent = -Qmsh.Vertices[vi].Tangent;
		Qmsh.Vertices[vi].Binormal = -Qmsh.Vertices[vi].Binormal;
	}
}

void FlipFacesTask(QMSH &Qmsh, MeshJob &Job, FlipFacesMeshTask &Task)
{
	Writeln("Flipping faces...");

	assert(Qmsh.Indices.size() % 3 == 0);
	for (uint ii = 0; ii < Qmsh.Indices.size(); ii += 3)
		std::swap(Qmsh.Indices[ii], Qmsh.Indices[ii+2]);
}

void RemoveAnimationMovementTask(QMSH &Qmsh, MeshJob &Job, RemoveAnimationMovementMeshTask &Task)
{
	Writeln("Removing animation movement...");

	VEC3 TranslationA;

	// Dla ka¿dej animacji
	for (uint ai = 0; ai < Qmsh.Animations.size(); ai++)
	{
		// Jeœli ma co namjmniej 3 klatki
		if (Qmsh.Animations[ai]->Keyframes.size() > 2)
		{
			// Dla ka¿dej koœci
			for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
			{
				// Jeœli to koœæ g³ówna
				if (Qmsh.Bones[bi]->ParentIndex == 0)
				{
					// Oblicz wspó³czynnik nachylenia równania liniowego translacji na podstawie ró¿nicy translacji
					// klatki pierwszej i ostatniej.
					TranslationA =
						Qmsh.Animations[ai]->Keyframes[Qmsh.Animations[ai]->Keyframes.size()-1]->Bones[bi].Translation -
						Qmsh.Animations[ai]->Keyframes[0                                      ]->Bones[bi].Translation;
					TranslationA /= Qmsh.Animations[ai]->Length;
					// Zmieñ translacjê wszystkich klatek
					for (uint ki = 0; ki < Qmsh.Animations[ai]->Keyframes.size(); ki++)
					{
						Qmsh.Animations[ai]->Keyframes[ki]->Bones[bi].Translation -= TranslationA *
							Qmsh.Animations[ai]->Keyframes[ki]->Time;
					}
				}
			}
		}
	}
}

void ValidateTask(QMSH &Qmsh, MeshJob &Job, ValidateMeshTask &Task)
{
	Writeln("Validating:");

	uint Problems = 0;

	// Proste testy

	if ((Qmsh.Flags & ~0x03) != 0)
	{
		Problems++;
		Writeln(Format("  Corrupted vertex format flags: #") % UintToStrR(Qmsh.Flags, 16));
	}

	// Wierzcho³ki

	uint ZeroNormals = 0, ZeroTangents = 0, ZeroBinormals = 0;
	uint UnnormalizedNormals = 0, UnnormalizedTangents = 0, UnnormalizedBinormals = 0;
	uint InvalidBoneIndices = 0, OutOfRangeBoneIndices = 0;
	float TmpLength;
	for (uint vi = 0; vi < Qmsh.Vertices.size(); vi++)
	{
		const QMSH_VERTEX &Vertex = Qmsh.Vertices[vi];
		// Normalna - zerowa, niejedynkowa
		TmpLength = Length(Vertex.Normal);
		if (TmpLength < 0.001f)
			ZeroNormals++;
		else if (!around(TmpLength, 1.0f, 0.001f))
			UnnormalizedNormals++;
		// Tangent, Binormal - jak wy¿ej
		if ((Qmsh.Flags & QMSH::FLAG_TANGENTS) != 0)
		{
			TmpLength = Length(Vertex.Tangent);
			if (TmpLength < 0.001f)
				ZeroTangents++;
			else if (!around(TmpLength, 1.0f, 0.001f))
				UnnormalizedTangents++;
			TmpLength = Length(Vertex.Binormal);
			if (TmpLength < 0.001f)
				ZeroBinormals++;
			else if (!around(TmpLength, 1.0f, 0.001f))
				UnnormalizedBinormals++;
		}
		// Indeksy koœci
		if ((Qmsh.Flags & QMSH::FLAG_SKINNING) != 0)
		{
			if ((Vertex.BoneIndices & 0xFFFF0000) != 0)
				InvalidBoneIndices++;
			// Nie >=, bo licz¹ siê od 1, bo 0 to brak koœci
			else if ( (Vertex.BoneIndices & 0xFF) > Qmsh.Bones.size() ||
				((Vertex.BoneIndices >> 8) & 0xFF) > Qmsh.Bones.size())
				OutOfRangeBoneIndices++;
		}
	}

	if (ZeroNormals > 0 || ZeroTangents > 0 || ZeroBinormals > 0)
	{
		Problems++;
		if ((Qmsh.Flags & QMSH::FLAG_TANGENTS) != 0)
			Writeln(Format("  Zero: Normals=#, Tangents=#, Binormals=#") % ZeroNormals % ZeroTangents % ZeroBinormals);
		else
			Writeln(Format("  Zero normals=#") % ZeroNormals);
	}
	if (UnnormalizedNormals > 0 || UnnormalizedTangents > 0 || UnnormalizedBinormals > 0)
	{
		Problems++;
		if ((Qmsh.Flags & QMSH::FLAG_TANGENTS) != 0)
			Writeln(Format("  Unnormalized: Normals=#, Tangents=#, Binormals=#") % UnnormalizedNormals % UnnormalizedTangents % UnnormalizedBinormals);
		else
			Writeln(Format("  Unnormalized normals=#") % UnnormalizedNormals);
	}
	if (InvalidBoneIndices > 0 || OutOfRangeBoneIndices > 0)
	{
		Problems++;
		Writeln(Format("  CorruptedBoneIndices=#, OutOfRangeBoneIndices=#") % InvalidBoneIndices % OutOfRangeBoneIndices);
	}

	// Indeksy

	uint DegeneratedTriangles = 0, IndicesOutOfRange = 0;
	bool TriangleCorrect;
	for (uint ii = 0; ii < Qmsh.Indices.size(); ii += 3)
	{
		TriangleCorrect = true;
		if (Qmsh.Indices[ii  ] >= Qmsh.Vertices.size())
		{
			IndicesOutOfRange++;
			TriangleCorrect = false;
		}
		if (Qmsh.Indices[ii+1] >= Qmsh.Vertices.size())
		{
			IndicesOutOfRange++;
			TriangleCorrect = false;
		}
		if (Qmsh.Indices[ii+2] >= Qmsh.Vertices.size())
		{
			IndicesOutOfRange++;
			TriangleCorrect = false;
		}

		if (TriangleCorrect && TriangleArea(
			Qmsh.Vertices[Qmsh.Indices[ii  ]].Pos,
			Qmsh.Vertices[Qmsh.Indices[ii+1]].Pos,
			Qmsh.Vertices[Qmsh.Indices[ii+2]].Pos) == 0.f)
		{
			DegeneratedTriangles++;
		}
	}

	if (IndicesOutOfRange > 0)
	{
		Problems++;
		Writeln(Format("  Out of range vertex indices: #") % IndicesOutOfRange);
	}
	if (DegeneratedTriangles > 0)
	{
		Problems++;
		Writeln(Format("  Degenerated triangles: ") % DegeneratedTriangles);
	}

	// Bounding Volumes
	if (Qmsh.BoundingVolumesCalculated)
	{
		bool InvalidSphere = false, InvalidBox = false;
		float OldBoundingSphereRadius = Qmsh.BoundingSphereRadius;
		BOX OldBoundingBox = Qmsh.BoundingBox;
		float NewBoundingSphereRadius;
		BOX NewBoundingBox;
		CalcBoundingVolumes(Qmsh, &NewBoundingSphereRadius, &NewBoundingBox);
		if (!float_equal(OldBoundingSphereRadius, NewBoundingSphereRadius))
			InvalidSphere = true;
		if (!VecEqual(OldBoundingBox.p1, NewBoundingBox.p1) ||
			!VecEqual(OldBoundingBox.p2, NewBoundingBox.p2))
			InvalidBox = true;
		if (InvalidSphere || InvalidBox)
		{
			Problems++;
			Writeln(Format("  BoundingSphereCorrect=#, BoundingBoxCorrect=#") % (!InvalidSphere) % (!InvalidBox));
		}
	}

	// Submeshes

	uint OutOfRangeIndices = 0;
	for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
	{
		for (uint ii = Qmsh.Submeshes[si]->FirstTriangle * 3;
			ii < (Qmsh.Submeshes[si]->FirstTriangle + Qmsh.Submeshes[si]->NumTriangles) * 3;
			ii++)
		{
			if (Qmsh.Indices[ii] < Qmsh.Submeshes[si]->MinVertexIndexUsed ||
				Qmsh.Indices[ii] >= Qmsh.Submeshes[si]->MinVertexIndexUsed + Qmsh.Submeshes[si]->NumVertexIndicesUsed)
			{
				OutOfRangeIndices++;
			}
		}
	}

	if (OutOfRangeIndices > 0)
	{
		Problems++;
		Writeln(Format("  Submesh indices out of declared range: #") % OutOfRangeIndices);
	}

	// Koœci

	uint InvalidBoneParentIndex = 0;
	for (uint bi = 0; bi < Qmsh.Bones.size(); bi++)
	{
		// Nie >=, bo liczone s¹ od 1, koœæ 0 to brak koœci
		if (Qmsh.Bones[bi]->ParentIndex > Qmsh.Bones.size())
			InvalidBoneParentIndex++;
	}

	if (InvalidBoneParentIndex > 0)
	{
		Problems++;
		Writeln(Format("  Invalid parent bone indices: #") % InvalidBoneParentIndex);
	}

	// Animacje

	uint NegativeAnimationLength = 0, InvalidAnimationLength = 0;
	uint NegativeKeyframeTime = 0, InvalidKeyframeTime = 0;
	uint UnnormalizedQuaternions = 0;
	QUATERNION TmpQuat;
	for (uint ai = 0; ai < Qmsh.Animations.size(); ai++)
	{
		const QMSH_ANIMATION &Animation = *Qmsh.Animations[ai].get();
		if (Animation.Length < 0.f)
			NegativeAnimationLength++;
		if (Animation.Keyframes.empty())
		{
			if (Animation.Length != 0.f)
				InvalidAnimationLength++;
		}
		else
		{
			if (Animation.Keyframes[Animation.Keyframes.size()-1]->Time != Animation.Length)
				InvalidAnimationLength++;
		}
		for (uint ki = 0; ki < Animation.Keyframes.size(); ki++)
		{
			const QMSH_KEYFRAME &Keyframe = *Animation.Keyframes[ki].get();
			if (Keyframe.Time < 0.f)
				NegativeKeyframeTime++;
			if (ki > 0 && Keyframe.Time <= Animation.Keyframes[ki-1]->Time)
				InvalidKeyframeTime++;

			for (uint bi = 0; bi < Keyframe.Bones.size(); bi++)
			{
				Normalize(&TmpQuat, Keyframe.Bones[bi].Rotation);
				if (!QuaternionEqual(TmpQuat, Keyframe.Bones[bi].Rotation))
					UnnormalizedQuaternions++;
			}
		}
	}

	if (NegativeAnimationLength > 0)
	{
		Problems++;
		Writeln(Format("  Animations with negative length: #") % NegativeAnimationLength);
	}
	if (InvalidAnimationLength > 0)
	{
		Problems++;
		Writeln(Format("  Animations with invalid length: #") % InvalidAnimationLength);
	}
	if (NegativeKeyframeTime > 0)
	{
		Problems++;
		Writeln(Format("  Keyframes with negative time: #") % NegativeKeyframeTime);
	}
	if (InvalidKeyframeTime > 0)
	{
		Problems++;
		Writeln(Format("  Keyframes with invalid time: #") % InvalidKeyframeTime);
	}
	if (UnnormalizedQuaternions > 0)
	{
		Problems++;
		Writeln(Format("  Unnormalized rotation quaternions in keyframes: #") % UnnormalizedQuaternions);
	}

	// Koniec
	if (Problems == 0)
		Writeln("  No problems found.");
}

void NormalizeQuaternionsTask(QMSH &Qmsh, MeshJob &Job, NormalizeQuaternionsMeshTask &Task)
{
	Writeln("Normalizing quaternions...");

	for (uint ai = 0; ai < Qmsh.Animations.size(); ai++)
	{
		QMSH_ANIMATION &Animation = *Qmsh.Animations[ai].get();
		for (uint ki = 0; ki < Animation.Keyframes.size(); ki++)
		{
			QMSH_KEYFRAME &Keyframe = *Animation.Keyframes[ki].get();
			for (uint bi = 0; bi < Keyframe.Bones.size(); bi++)
			{
				Normalize(&Keyframe.Bones[bi].Rotation);
			}
		}
	}
}

void MendTask(QMSH &Qmsh, MeshJob &Job, MendMeshTask &Task)
{
	Writeln("Running NVMeshMender...");

	// Nowy VB i IB
	std::vector<QMSH_VERTEX> NewVertices;
	std::vector<uint2> NewIndices;

	// WprowadŸ do siatki ustawienie /Tangents
	if (Job.Tangents)
		Qmsh.Flags |= QMSH::FLAG_TANGENTS;
	else
		Qmsh.Flags &= ~QMSH::FLAG_TANGENTS;

	// Dla kolejnych obiektów
	for (uint si = 0; si < Qmsh.Submeshes.size(); si++)
	{
		QMSH_SUBMESH &Submesh = *Qmsh.Submeshes[si].get();
		uint OldVertexOffset = Submesh.MinVertexIndexUsed;

		// Wype³nij tymczasow¹ tablicê wierzcho³ków
		std::vector<MeshMender::Vertex> NvVertices;
		NvVertices.resize(Submesh.NumVertexIndicesUsed);
		for (uint vi = 0; vi < Submesh.NumVertexIndicesUsed; vi++)
		{
			NvVertices[vi].pos = math_cast<D3DXVECTOR3>(Qmsh.Vertices[Submesh.MinVertexIndexUsed + vi].Pos);
			NvVertices[vi].normal = math_cast<D3DXVECTOR3>(Qmsh.Vertices[Submesh.MinVertexIndexUsed + vi].Normal);
			NvVertices[vi].s = Qmsh.Vertices[Submesh.MinVertexIndexUsed + vi].Tex.x;
			NvVertices[vi].t = Qmsh.Vertices[Submesh.MinVertexIndexUsed + vi].Tex.y;
			// tangent i binormal ma zostaæ niewype³niony
		}

		// Wype³nij tymczasow¹ tablicê indeksów
		std::vector<unsigned int> NvIndices;
		NvIndices.resize(Submesh.NumTriangles * 3);
		for (uint ii = 0; ii < Submesh.NumTriangles * 3; ii++)
			NvIndices[ii] = Qmsh.Indices[Submesh.FirstTriangle * 3 + ii] - Submesh.MinVertexIndexUsed;

		std::vector<unsigned int> NvMappingOldToNewVert;

		// Wype³nij pozosta³e parametry
		float MinCreaseCosAngle = cosf(DegToRad(Job.MaxSmoothAngle));

		// Odpal NvMeshMender!
		MeshMender mender;
		if (!mender.Mend(
			NvVertices,
			NvIndices,
			NvMappingOldToNewVert,
			MinCreaseCosAngle, MinCreaseCosAngle, MinCreaseCosAngle,
			1.0f,
			Job.CalcNormals ? MeshMender::CALCULATE_NORMALS : MeshMender::DONT_CALCULATE_NORMALS,
			Job.RespectExistingSplits ? MeshMender::RESPECT_SPLITS : MeshMender::DONT_RESPECT_SPLITS,
			Job.FixCylindricalWrapping ? MeshMender::FIX_CYLINDRICAL : MeshMender::DONT_FIX_CYLINDRICAL))
		{
			throw Error("NVMeshMender failed.");
		}

		// Ustaw nowe parametry podsiatki

		assert(NewIndices.size() % 3 == 0);
		Submesh.FirstTriangle = NewIndices.size() / 3;
		assert(NvIndices.size() % 3 == 0);
		Submesh.NumTriangles = NvIndices.size() / 3;

		Submesh.MinVertexIndexUsed = NewVertices.size();
		Submesh.NumVertexIndicesUsed = NvVertices.size();

		// Przepisz nowe tymczasowe indeksy i wierzcho³ki do nowych VB i IB

		NewIndices.reserve(NewIndices.size() + NvIndices.size());
		for (uint ii = 0; ii < NvIndices.size(); ii++)
		{
			NewIndices.push_back((uint2)(NvIndices[ii] + NewVertices.size()));
		}

		NewVertices.reserve(NewVertices.size() + NvVertices.size());
		for (uint vi = 0; vi < NvVertices.size(); vi++)
		{
			QMSH_VERTEX v;

			v.Pos = math_cast<VEC3>(NvVertices[vi].pos);
			v.Tex.x = NvVertices[vi].s;
			v.Tex.y = NvVertices[vi].t;
			v.Normal = math_cast<VEC3>(NvVertices[vi].normal);

			if ((Qmsh.Flags & QMSH::FLAG_TANGENTS) != 0)
			{
				v.Tangent = math_cast<VEC3>(NvVertices[vi].tangent);
				v.Binormal = math_cast<VEC3>(NvVertices[vi].binormal);
			}
			else
			{
				v.Tangent = VEC3::ZERO;
				v.Binormal = VEC3::ZERO;
			}

			if ((Qmsh.Flags & QMSH::FLAG_SKINNING) != 0)
			{
				assert(NvMappingOldToNewVert[vi] < Qmsh.Vertices.size());
				v.Weight1 = Qmsh.Vertices[NvMappingOldToNewVert[vi] + OldVertexOffset].Weight1;
				v.BoneIndices = Qmsh.Vertices[NvMappingOldToNewVert[vi] + OldVertexOffset].BoneIndices;
			}
			else
			{
				v.Weight1 = 0.f;
				v.BoneIndices = 0;
			}

			NewVertices.push_back(v);
		}
	}

	// Zamieñ stary z nowym VB i IB
	std::swap(Qmsh.Vertices, NewVertices);
	std::swap(Qmsh.Indices, NewIndices);
}

void DoMeshJob(MeshJob &Job)
{
	scoped_ptr<QMSH> Mesh;

	for (uint ti = 0; ti < Job.Tasks.size(); ti++)
	{
		MeshTask &t = *Job.Tasks[ti].get();

		// Zadanie nie wymagaj¹ce wczytanej siatki - wejœciowe
		if (typeid(t) == typeid(InputMeshTask))
		{
			Mesh.reset(new QMSH);
			InputTask(*Mesh.get(), Job, (InputMeshTask&)t);
		}
		// Zadania wymagaj¹ce wczytanej siatki
		else
		{
			if (Mesh == NULL)
				throw Error("No mesh loaded.");

			if (typeid(t) == typeid(OutputMeshTask))
				OutputTask(*Mesh.get(), Job, (OutputMeshTask&)t);
			else if (typeid(t) == typeid(InfoMeshTask))
				InfoTask(*Mesh.get(), Job, (InfoMeshTask&)t);
			else if (typeid(t) == typeid(TransformMeshTask))
				TransformTask(*Mesh.get(), Job, (TransformMeshTask&)t);
			else if (typeid(t) == typeid(TransformTexMeshTask))
				TransformTexTask(*Mesh.get(), Job, (TransformTexMeshTask&)t);
			else if (typeid(t) == typeid(NormalizeNormalsMeshTask))
				NormalizeNormalsTask(*Mesh.get(), Job, (NormalizeNormalsMeshTask&)t);
			else if (typeid(t) == typeid(ZeroNormalsMeshTask))
				ZeroNormalsTask(*Mesh.get(), Job, (ZeroNormalsMeshTask&)t);
			else if (typeid(t) == typeid(DeleteTangentsMeshTask))
				DeleteTangentsTask(*Mesh.get(), Job, (DeleteTangentsMeshTask&)t);
			else if (typeid(t) == typeid(DeleteSkinningMeshTask))
				DeleteSkinningTask(*Mesh.get(), Job, (DeleteSkinningMeshTask&)t);
			else if (typeid(t) == typeid(DeleteObjectMeshTask))
				DeleteObjectTask(*Mesh.get(), Job, (DeleteObjectMeshTask&)t);
			else if (typeid(t) == typeid(RenameObjectMeshTask))
				RenameObjectTask(*Mesh.get(), Job, (RenameObjectMeshTask&)t);
			else if (typeid(t) == typeid(RenameMaterialMeshTask))
				RenameMaterialTask(*Mesh.get(), Job, (RenameMaterialMeshTask&)t);
			else if (typeid(t) == typeid(SetMaterialMeshTask))
				SetMaterialTask(*Mesh.get(), Job, (SetMaterialMeshTask&)t);
			else if (typeid(t) == typeid(SetAllMaterialsMeshTask))
				SetAllMaterialsTask(*Mesh.get(), Job, (SetAllMaterialsMeshTask&)t);
			else if (typeid(t) == typeid(RenameBoneMeshTask))
				RenameBoneTask(*Mesh.get(), Job, (RenameBoneMeshTask&)t);
			else if (typeid(t) == typeid(RenameAnimationMeshTask))
				RenameAnimationTask(*Mesh.get(), Job, (RenameAnimationMeshTask&)t);
			else if (typeid(t) == typeid(DeleteAnimationMeshTask))
				DeleteAnimationTask(*Mesh.get(), Job, (DeleteAnimationMeshTask&)t);
			else if (typeid(t) == typeid(DeleteAllAnimationsMeshTask))
				DeleteAllAnimationsTask(*Mesh.get(), Job, (DeleteAllAnimationsMeshTask&)t);
			else if (typeid(t) == typeid(ScaleTimeMeshTask))
				ScaleTimeTask(*Mesh.get(), Job, (ScaleTimeMeshTask&)t);
			else if (typeid(t) == typeid(OptimizeAnimationsMeshTask))
				OptimizeAnimationsTask(*Mesh.get(), Job, (OptimizeAnimationsMeshTask&)t);
			else if (typeid(t) == typeid(RecalcBoundingsMeshTask))
				RecalcBoundingsTask(*Mesh.get(), Job, (RecalcBoundingsMeshTask&)t);
			else if (typeid(t) == typeid(FlipNormalsMeshTask))
				FlipNormalsTask(*Mesh.get(), Job, (FlipNormalsMeshTask&)t);
			else if (typeid(t) == typeid(FlipFacesMeshTask))
				FlipFacesTask(*Mesh.get(), Job, (FlipFacesMeshTask&)t);
			else if (typeid(t) == typeid(RemoveAnimationMovementMeshTask))
				RemoveAnimationMovementTask(*Mesh.get(), Job, (RemoveAnimationMovementMeshTask&)t);
			else if (typeid(t) == typeid(ValidateMeshTask))
				ValidateTask(*Mesh.get(), Job, (ValidateMeshTask&)t);
			else if (typeid(t) == typeid(NormalizeQuaternionsMeshTask))
				NormalizeQuaternionsTask(*Mesh.get(), Job, (NormalizeQuaternionsMeshTask&)t);
			else if (typeid(t) == typeid(MendMeshTask))
				MendTask(*Mesh.get(), Job, (MendMeshTask&)t);
			else
				throw Error("Nienznany typ zadania.");
		}
	}
}
