/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#pragma once

// Abstrakcyjna klasa bazowa dla zadañ
struct MeshTask : public Task
{
public:
	virtual ~MeshTask() { } // Klasa jest polimorficzna
};

// Wczytanie pliku
// Typ rozpoznawany po rozszerzeniu.
struct InputMeshTask : public MeshTask
{
	string FileName;
};

// Zapisanie pliku
// Typ rozpoznawany po rozszerzeniu.
struct OutputMeshTask : public MeshTask
{
	string FileName;
};

// Pokazanie informacji o siatce
struct InfoMeshTask : public MeshTask
{
	bool Detailed;
};

struct TransformMeshTask : public MeshTask
{
	uint Type;
	// Transformacja do wykonania.
	// Nie dotyczy CenterOnAverage, CenterOnMedian, TransformToBox, TransformToBoxProportional
	MATRIX Transform;
	// Tylko jeœli CenterToBox
	BOX Box;
	// Pusty, jeœli dotyczy ca³ego modelu
	string ObjectName;

	// Typy:
	// 3001 - Translate
	// 3002 - RotateYawPitchRoll
	// 3003 - RotateQuaternion
	// 3004 - RotateAxisAngle
	// 3005 - Scale
	// 3006 - Transform
	// 3007 - CenterOnAverage
	// 3008 - CenterOnMedian
	// 3010 - TransformToBox
	// 3011 - TransformToBoxProportional
	TransformMeshTask(uint Type, const string &Params);
};

struct TransformTexMeshTask : public MeshTask
{
	uint Type;
	// Transformacja do wykonania.
	MATRIX Transform;
	// Pusty, jeœli dotyczy ca³ego modelu
	string ObjectName;

	// Typy:
	// 4001 - TranslateTex
	// 4002 - ScaleTex
	// 4003 - TransformTex
	TransformTexMeshTask(uint Type, const string &Params);
};

struct NormalizeNormalsMeshTask : public MeshTask { };
struct ZeroNormalsMeshTask : public MeshTask { };
struct DeleteTangentsMeshTask : public MeshTask { };
struct DeleteSkinningMeshTask : public MeshTask { };

struct DeleteObjectMeshTask : public MeshTask
{
	string Name;
	DeleteObjectMeshTask(const string &Name) : Name(Name) { }
};

struct RenameObjectMeshTask : public MeshTask
{
	string OldName;
	string NewName;
	RenameObjectMeshTask(const string &Params);
};

struct RenameMaterialMeshTask : public MeshTask
{
	string OldName;
	string NewName;
	RenameMaterialMeshTask(const string &Params);
};

struct SetMaterialMeshTask : public MeshTask
{
	string ObjectName;
	string NewMaterialName;
	SetMaterialMeshTask(const string &Params);
};

struct SetAllMaterialsMeshTask : public MeshTask
{
	string NewMaterialName;
	SetAllMaterialsMeshTask(const string &Params) : NewMaterialName(Params) { }
};

struct RenameBoneMeshTask : public MeshTask
{
	string OldName;
	string NewName;
	RenameBoneMeshTask(const string &Params);
};

struct RenameAnimationMeshTask : public MeshTask
{
	string OldName;
	string NewName;
	RenameAnimationMeshTask(const string &Params);
};

struct DeleteAnimationMeshTask : public MeshTask
{
	string Name;
	DeleteAnimationMeshTask(const string &Params) : Name(Params) { }
};

struct DeleteAllAnimationsMeshTask : public MeshTask { };

struct ScaleTimeMeshTask : public MeshTask
{
	float Scale;
	// Puste jeœli wszystkie animacje
	string AnimationName;
	ScaleTimeMeshTask(const string &Params);
};

struct OptimizeAnimationsMeshTask : public MeshTask { };
struct RecalcBoundingsMeshTask : public MeshTask { };
struct FlipNormalsMeshTask : public MeshTask { };
struct FlipFacesMeshTask : public MeshTask { };
struct RemoveAnimationMovementMeshTask : public MeshTask { };
struct ValidateMeshTask : public MeshTask { };
struct MendMeshTask : public MeshTask { };
struct NormalizeQuaternionsMeshTask : public MeshTask { };

struct MeshJob
{
	std::vector< common::shared_ptr<MeshTask> > Tasks;

	// Dane dla NVMeshMender
	bool Tangents;
	float MaxSmoothAngle; // W stopniach
	bool CalcNormals;
	bool RespectExistingSplits;
	bool FixCylindricalWrapping;

	MeshJob();
	~MeshJob();
};

void DoMeshJob(MeshJob &Job);
