/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef QMESH_H_
#define QMESH_H_

#include "Res_d3d.hpp"

namespace res
{

struct QMesh_pimpl;

// Siatka modelu wczytana z pliku QMSH
class QMesh : public D3dResource
{
public:
	struct SUBMESH
	{
		string Name;
		string MaterialName;
		uint2 FirstTriangle;
		uint2 NumTriangles;
		uint2 MinVertexIndexUsed;   // odpowiednik parametru DrawIndexedPrimitive - MinIndex (tylko wyra�ony w tr�jk�tach)
		uint2 NumVertexIndicesUsed; // odpowiednik parametru DrawIndexedPrimitive - NumVertices (tylko wyra�ony w tr�jk�tach)
	};

	// Ko�� o indeksie 0 te� istnieje, jest dodatkowa, pusta (nazwa pusta, macierz identyczno�ciowa)
	// Liczba ko�ci jest liczona ��cznie z ko�ci� zerow�.
	struct BONE
	{
		uint2 Index;
		string Name;
		uint2 ParentIndex;
		common::MATRIX Matrix; // Macierz przeszta�caj�ca ze wsp. danej ko�ci do wsp. ko�ci nadrz�dnej w pozycji spoczynkowej, ��cznie z translacj�
		std::vector<uint2> Children; // Indeksy podko�ci
	};

	class Animation_pimpl;

	class Animation
	{
	public:
		Animation();
		~Animation();

		const string & GetName();
		float GetLength(); // W sekundach
		uint GetKeyframeCount();

		float GetKeyframeTime(uint KeyframeIndex);
		// Zwracaj� przekszta�cenie z danej klatki kluczowej
		const common::VEC3 &       GetKeyframeTranslation(uint BoneIndex, uint KeyframeIndex);
		const common::QUATERNION & GetKeyframeRotation   (uint BoneIndex, uint KeyframeIndex);
		float                      GetKeyframeScaling    (uint BoneIndex, uint KeyframeIndex);

		// Obliczaj� zinterpolowane przekszta�cenie. Niekoniecznie grzesz� szybko�ci�.
		void GetTimeTranslation(common::VEC3       *Out, uint BoneIndex, float Time);
		void GetTimeRotation   (common::QUATERNION *Out, uint BoneIndex, float Time);
		void GetTimeScaling    (float              *Out, uint BoneIndex, float Time);

	private:
		scoped_ptr<Animation_pimpl> pimpl;
		friend class QMesh;
	};

	static void RegisterResourceType();
	static IResource * Create(const string &Name, const string &Group, Tokenizer &Params);

	QMesh(const string &Name, const string &Group, const string &FileName);
	virtual ~QMesh();

	// Zwraca info o zasobie
	const string & GetFileName();

	// Zwraca info o siatce - tylko je�li siatka jest wczytana
	bool HasSkinning();
	bool HasTangent();
	uint GetFVF();
	uint GetVertexSize();
	uint GetSubmeshCount();
	uint GetVertexCount();
	uint GetTriangleCount();
	float GetBoundingSphereRadius();
	const BOX & GetBoundingBox();
	const SUBMESH & GetSubmesh(uint Index);
	IDirect3DVertexBuffer9 * GetVB();
	IDirect3DIndexBuffer9 * GetIB();

	uint GetBoneCount();
	uint GetAnimationCount();
	const BONE & GetBone(uint Index);
	Animation & GetAnimation(uint Index);
	// Je�li nie znajdzie, zwraca NULL.
	const BONE * GetBoneByName(const string &BoneName);
	// Je�li nie znajdzie, zwraca MAXUINT4.
	uint GetAnimationIndexByName(const string &AnimationName);
	// Je�li nie znajdzie, zwraca NULL.
	Animation * GetAnimationByName(const string &AnimationName);
	// Zwraca wyliczane przy pierwszym pobraniu macierze przekszta�caj�ce wsp�rz�dne z g�wnych modelu
	// do lokalnych danej ko�ci w Bind Pose. Macierzy w tablicy jest tyle, ile ko�ci.
	// Je�li nie ma skinningu albo jest 0 ko�ci, zwraca NULL.
	const MATRIX * GetModelToBoneMatrices();
	// Zwraca macierze ko�ci dla podanej pozycji.
	// One s� wyliczane na ��danie, cache'owane, ale mog� znikn�� - nie zapami�tywa� wska�nika ani
	// referencji do tej tablicy, mog� si� uniewa�ni� po jakim� ponownym u�yciu obiektu tej klasy.
	const MATRIX * GetBoneMatrices(float Accuracy, uint Animation, float Time);
	// Animation2 == MAXUINT oznacza, �e nie ma interpolacji, jest tylko pierwsza animacja - jak w funkcji powy�ej.
	// Time2 i LerpT si� w�wczas nie licz�.
	const MATRIX * GetBoneMatrices(float Accuracy, uint Animation1, float Time1, uint Animation2, float Time2, float LerpT);
	
	// Kolizja promienia w uk�adzie lokalnym modelu z modelem w pozycji spoczynkowej (bez animacji)
	// Mo�na sprawdzi� kolizj� tylko z fragmentem modelu - wybranymi tr�jk�tami.
	// TriangleEnd = MAXUINT4 oznacza, �eby sprawdzi� do ko�ca.
	bool RayCollision(float *OutT, const VEC3 &RayOrig, const VEC3 &RayDir, bool TwoSided, uint TriangleBegin = 0, uint TriangleEnd = MAXUINT4);
	// Jak wy�ej, ale kolizja promienia z modelem w ustalonej pozycji animacji,
	bool RayCollision_Bones(float *OutT, const VEC3 &RayOrig, const VEC3 &RayDir, bool TwoSided, float Accuracy, uint Animation, float Time, uint TriangleBegin = 0, uint TriangleEnd = MAXUINT4);
	// Animation2 == MAXUINT oznacza, �e nie ma interpolacji, jest tylko pierwsza animacja - jak w funkcji powy�ej.
	// Time2 i LerpT si� w�wczas nie licz�.
	bool RayCollision_Bones(float *OutT, const VEC3 &RayOrig, const VEC3 &RayDir, bool TwoSided, float Accuracy, uint Animation1, float Time1, uint Animation2, float Time2, float LerpT, uint TriangleBegin = 0, uint TriangleEnd = MAXUINT4);

protected:
	// ======== Implementacja IResource ========
	virtual void OnLoad();
	virtual void OnUnload();

	// ======== Implementacja D3dResource ========
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual void OnDeviceRestore();
	virtual void OnDeviceInvalidate();

private:
	scoped_ptr<QMesh_pimpl> pimpl;
};


} // namespace res

#endif
