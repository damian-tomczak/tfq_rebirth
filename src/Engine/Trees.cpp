/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "..\Framework\Framework.hpp"
#include "..\Framework\Res_d3d.hpp"
#include "..\Framework\Multishader.hpp"
#include "Engine.hpp"
#include "Camera.hpp"
#include "Trees.hpp"

namespace engine
{

const float WIND_FACTOR = 1.0f;

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktura TREE_LEVEL_DESC

void TREE_LEVEL_DESC::LoadFromTokenizer(Tokenizer &t)
{
	ERR_TRY;

	t.AssertSymbol('{');
	t.Next();

	this->Visible = t.MustGetUint4() > 0;
	t.Next();

	this->SubbranchCount = t.MustGetUint4();
	t.Next();

	this->SubbranchRangeMin = t.MustGetFloat();
	t.Next();
	this->SubbranchRangeMax = t.MustGetFloat();
	t.Next();

	this->SubbranchAngle = t.MustGetFloat();
	t.Next();
	this->SubbranchAngleV = t.MustGetFloat();
	t.Next();

	this->Length = t.MustGetFloat();
	t.Next();
	this->LengthV = t.MustGetFloat();
	t.Next();
	this->LengthToParent = t.MustGetFloat();
	t.Next();

	this->Radius = t.MustGetFloat();
	t.Next();
	this->RadiusV = t.MustGetFloat();
	t.Next();
	this->RadiusEnd = t.MustGetFloat();
	t.Next();

	this->LeafCount = t.MustGetFloat();
	t.Next();
	this->LeafCountV = t.MustGetFloat();
	t.Next();

	this->LeafRangeMin = t.MustGetFloat();
	t.Next();
	this->LeafRangeMax = t.MustGetFloat();
	t.Next();

	t.AssertSymbol('}');
	t.Next();

	ERR_CATCH("Nie mo¿na odczytaæ definicji poziomu drzewa.");
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktura TREE_DESC

void TREE_DESC::LoadFromTokenizer(Tokenizer &t)
{
	ERR_TRY;

	t.AssertSymbol('{');
	t.Next();

	this->HalfWidth = t.MustGetFloat();
	t.Next();
	this->HalfHeight = t.MustGetFloat();
	t.Next();

	this->BarkTexScaleY = t.MustGetFloat();
	t.Next();

	this->LeafHalfSizeCX = t.MustGetFloat();
	t.Next();
	this->LeafHalfSizeCXV = t.MustGetFloat();
	t.Next();
	this->LeafHalfSizeCY = t.MustGetFloat();
	t.Next();
	this->LeafHalfSizeCYV = t.MustGetFloat();
	t.Next();

	this->AllowTexFlipX = t.MustGetUint4() > 0;
	t.Next();

	t.AssertIdentifier("Roots");
	t.Next();
	this->Roots.LoadFromTokenizer(t);

	uint Level = 0;
	ZeroMem(this->Levels, sizeof(TREE_LEVEL_DESC) * TREE_MAX_LEVELS);
	while (!t.QuerySymbol('}') && Level < TREE_MAX_LEVELS)
	{
		this->Levels[Level].LoadFromTokenizer(t);
		Level++;
	}
	t.Next();

	ERR_CATCH("Nie mo¿na odczytaæ definicji drzewa.");
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TreeTextureDesc

void TreeTextureDesc::Set(const string &TextureName, uint TextureCX, uint TextureCY, const RECTI &Root, const RECTI &Leaves)
{
	m_TextureName = TextureName;
	m_TextureCX = TextureCX;
	m_TextureCY = TextureCY;
	float TextureCX_f = (float)TextureCX;
	float TextureCY_f = (float)TextureCY;
	m_Root.left   = ((float)Root.left   + 0.5f) / TextureCX_f;
	m_Root.top    = ((float)Root.top    + 0.5f) / TextureCY_f;
	m_Root.right  = ((float)Root.right  + 0.5f) / TextureCX_f;
	m_Root.bottom = ((float)Root.bottom + 0.5f) / TextureCY_f;
	m_Leaves.left   = ((float)Leaves.left   + 0.5f) / TextureCX_f;
	m_Leaves.top    = ((float)Leaves.top    + 0.5f) / TextureCY_f;
	m_Leaves.right  = ((float)Leaves.right  + 0.5f) / TextureCX_f;
	m_Leaves.bottom = ((float)Leaves.bottom + 0.5f) / TextureCY_f;

	m_Texture = NULL;
}

void TreeTextureDesc::LoadFromTokenizer(Tokenizer &t)
{
	ERR_TRY;

	t.AssertSymbol('{'); t.Next();

	t.AssertToken(Tokenizer::TOKEN_STRING);
	string TextureName = t.GetString(); t.Next();

	uint TextureCX = t.MustGetUint4(); t.Next();
	uint TextureCY = t.MustGetUint4(); t.Next();

	RECTI Root;
	Root.left   = t.MustGetInt4(); t.Next();
	Root.top    = t.MustGetInt4(); t.Next();
	Root.right  = t.MustGetInt4(); t.Next();
	Root.bottom = t.MustGetInt4(); t.Next();

	RECTI Leaves;
	Leaves.left   = t.MustGetInt4(); t.Next();
	Leaves.top    = t.MustGetInt4(); t.Next();
	Leaves.right  = t.MustGetInt4(); t.Next();
	Leaves.bottom = t.MustGetInt4(); t.Next();

	t.AssertSymbol('}'); t.Next();

	Set(TextureName, TextureCX, TextureCY, Root, Leaves);

	ERR_CATCH("Nie mo¿na odczytaæ definicji tekstury drzewa.");
}

res::D3dTexture * TreeTextureDesc::GetTexture() const
{
	if (m_Texture == NULL && !m_TextureName.empty())
		m_Texture = res::g_Manager->GetResourceEx<res::D3dTexture>(m_TextureName);
	if (m_Texture != NULL)
		m_Texture->Load();
	return m_Texture;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Tree_pimpl i inne szczegó³y implementacji klasy Tree

enum TREE_SHADER_MACROS
{
	TSM_FOG,
	TSM_LIGHTING,
	TSM_RENDER_TO_SHADOW_MAP,
	TSM_SHADOW_MAP_MODE,
	TSM_COUNT
};

enum TREE_SHADER_PARAMS
{
	TSP_WORLD_VIEW_PROJ = 0,
	TSP_TEXTURE,
	TSP_RIGHT_VEC,
	TSP_UP_VEC,
	TSP_DIR_TO_LIGHT,
	TSP_LIGHT_COLOR,
	TSP_FOG_COLOR,
	TSP_FOG_FACTORS,
	TSP_QUAD_WIND_FACTORS,
};

struct TREE_VERTEX
{
	VEC3 Pos;
	// R 0..255 oznacza rozsuniêcie quada na X, -2..2 * RightVec
	// G 0..255 oznacza rozsuniêcie quada na Y, -2..2 * UpVec
	// B 0..255 to losowa liczba charakterystyczna dla danego quada jako -1..1
	// A 0..255 to losowa liczba charakterystyczna dla danego quada jako -1..1
	VEC3 Normal;
	COLOR Diffuse;
	VEC2 Tex;
};

const uint TREE_VERTEX_FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);

// Funktor deterministycznego generatora liczb pseudolosowych 0..n-1 potrzebny dla STL-a do std::random_shuffle.
class RandomGeneratorForSTL
{
public:
	RandomGeneratorForSTL(RandomGenerator &Rand) : m_Rand(Rand) { }
	int operator () (int n) { return m_Rand.RandInt(0, n); }

private:
	RandomGenerator &m_Rand;
};


class Tree_pimpl
{
public:
	Scene *m_OwnerScene;
	uint m_KindCount;
	uint m_InstancesPerKind;
	TREE_DESC m_TreeDesc;
	TreeTextureDesc m_TextureDesc;
	std::vector<uint> m_VerticesPerTree; // Ile wierzcho³ków liczy pojedyncze drzewo danego rodzaju Kind
	std::vector<uint> m_IndicesPerTree; // Ile indeksów liczy pojedyncze drzewo danego rodzaju Kind
	std::vector<uint> m_KindBaseVertexIndex; // Od którego wierzcho³ka w VB zaczynaj¹ siê drzewa danego rodzaju
	std::vector<uint> m_KindBaseIndexIndex; // Od którego indeksu w IB zaczynaj¹ siê drzewa danego rodzaju

	scoped_ptr<res::D3dVertexBuffer> m_VB;
	scoped_ptr<res::D3dIndexBuffer> m_IB;
	float m_CurrentWindArgX1, m_CurrentWindArgZ1;
	float m_CurrentWindArgX2, m_CurrentWindArgZ2;
	float m_CurrentWindArgX3, m_CurrentWindArgZ3;

	res::Multishader *m_TreeShader;

	////// U¿ywane do rysowania, miêdzy DrawBegin i DrawEnd
	VEC3 m_Draw_RightVec;
	VEC3 m_Draw_UpVec;
	const COLORF *m_Draw_LightColor;
	uint m_Draw_Macros[TSM_COUNT];
	const ParamsCamera *m_Draw_Cam;
	MATRIX m_Draw_ViewProj;
	MATRIX m_Draw_ShearMat;
	const VEC3 *m_Draw_DirToLight;
	const res::Multishader::SHADER_INFO *m_Draw_ShaderInfo;

	void CalcBranchVectors(VEC3 *Out_dir_x, VEC3 *Out_dir_y, const VEC3 &vec_z);
	// Macierz LocalToWorld nie mo¿e zawieraæ skalowania, jedynie rotacje i translacje.
	void GenerateBranch_Cross(std::vector<TREE_VERTEX> &VB, std::vector<uint2> &IB, const MATRIX &LocalToWorld, const VEC3 &p1, float HalfSize1, const VEC3 &p2, float HalfSize2);
	void GenerateBranch_Cylinder(std::vector<TREE_VERTEX> &VB, std::vector<uint2> &IB, const MATRIX &LocalToWorld, const VEC3 &p1, float HalfSize1, const VEC3 &p2, float HalfSize2, uint Slices);
	void GenerateTreeBranch(std::vector<TREE_VERTEX> &VB, std::vector<uint2> &IB, std::vector<VEC3> *LeafPoints, const TREE_DESC &Desc, uint Level, const MATRIX &BranchToWorld, float RemainingParentLength, RandomGenerator &Rand);
	//void ClusterPoints(std::vector<VEC3> *Out, const std::vector<VEC3> &In, uint OutCount, RandomGenerator &Rand);
	void GenerateLeafQuads(std::vector<TREE_VERTEX> &VB, std::vector<uint2> &IB, const std::vector<VEC3> &Points, RandomGenerator &Rand);
	void GenerateLeaves(std::vector<TREE_VERTEX> &VB, std::vector<uint2> &IB, std::vector<VEC3> *LeafPoints, RandomGenerator &Rand);
	void GenerateTree(uint KindRandomSeeds[]);

	IDirect3DTexture9 * EnsureTexture();
	void PrepareShearMatrix();
	void PrepareWind(D3DXVECTOR4 *OutQuadWindFactors);
};

void Tree_pimpl::CalcBranchVectors(VEC3 *Out_dir_x, VEC3 *Out_dir_y, const VEC3 &vec_z)
{
	PerpedicularVectors(Out_dir_x, Out_dir_y, vec_z);
	Normalize(Out_dir_x);
	Normalize(Out_dir_y);
}

void Tree_pimpl::GenerateBranch_Cross(std::vector<TREE_VERTEX> &VB, std::vector<uint2> &IB, const MATRIX &LocalToWorld, const VEC3 &p1, float HalfSize1, const VEC3 &p2, float HalfSize2)
{
	VEC3 p1_world, p2_world;
	Transform(&p1_world, p1, LocalToWorld);
	Transform(&p2_world, p2, LocalToWorld);

	VEC3 vec_z = p2_world - p1_world;
	VEC3 dir_x, dir_y;
	CalcBranchVectors(&dir_x, &dir_y, vec_z);
	Normalize(&dir_x);
	Normalize(&dir_y);

	VEC3 vec_x_1 = dir_x * HalfSize1;
	VEC3 vec_x_2 = dir_x * HalfSize2;
	VEC3 vec_y_1 = dir_y * HalfSize1;
	VEC3 vec_y_2 = dir_y * HalfSize2;

	TREE_VERTEX v;
	v.Diffuse.ARGB = 0x80808080;
	uint2 base_i;

	float half_size_max = std::max(HalfSize1, HalfSize2);
	float TexScaleY = m_TreeDesc.BarkTexScaleY * Length(vec_z);
	float tex_x_center = (m_TextureDesc.GetRoot().right + m_TextureDesc.GetRoot().left) * 0.5f;
	float tex_x_radius = tex_x_center - m_TextureDesc.GetRoot().left;
	tex_x_radius = std::min(tex_x_radius, tex_x_radius * half_size_max * 10.0f * m_TreeDesc.BarkTexScaleY);
	float tex_x_radius_1 = tex_x_radius * (HalfSize1 / half_size_max);
	float tex_x_radius_2 = tex_x_radius * (HalfSize2 / half_size_max);

	base_i = VB.size();
	v.Pos = p1_world - vec_x_1;
	v.Normal = - dir_x;
	v.Tex = VEC2(tex_x_center - tex_x_radius_1, TexScaleY);
	VB.push_back(v);
	v.Pos = p2_world - vec_x_2;
	v.Normal = - dir_x;
	v.Tex = VEC2(tex_x_center - tex_x_radius_2, 0.0f);
	VB.push_back(v);
	v.Pos = p2_world + vec_x_2;
	v.Normal = dir_x;
	v.Tex = VEC2(tex_x_center + tex_x_radius_2, 0.0f);
	VB.push_back(v);
	v.Pos = p1_world + vec_x_1;
	v.Normal = dir_x;
	v.Tex = VEC2(tex_x_center + tex_x_radius_1, TexScaleY);
	VB.push_back(v);

	IB.push_back(base_i  );
	IB.push_back(base_i+1);
	IB.push_back(base_i+2);
	IB.push_back(base_i  );
	IB.push_back(base_i+2);
	IB.push_back(base_i+3);

	base_i = VB.size();
	v.Pos = p1_world - vec_y_1;
	v.Normal = - dir_y;
	v.Tex = VEC2(tex_x_center - tex_x_radius_1, TexScaleY);
	VB.push_back(v);
	v.Pos = p2_world - vec_y_2;
	v.Normal = - dir_y;
	v.Tex = VEC2(tex_x_center - tex_x_radius_2, 0.0f);
	VB.push_back(v);
	v.Pos = p2_world + vec_y_2;
	v.Normal = dir_y;
	v.Tex = VEC2(tex_x_center + tex_x_radius_2, 0.0f);
	VB.push_back(v);
	v.Pos = p1_world + vec_y_1;
	v.Normal = dir_y;
	v.Tex = VEC2(tex_x_center + tex_x_radius_1, TexScaleY);
	VB.push_back(v);

	IB.push_back(base_i  );
	IB.push_back(base_i+1);
	IB.push_back(base_i+2);
	IB.push_back(base_i  );
	IB.push_back(base_i+2);
	IB.push_back(base_i+3);
}

void Tree_pimpl::GenerateBranch_Cylinder(std::vector<TREE_VERTEX> &VB, std::vector<uint2> &IB, const MATRIX &LocalToWorld, const VEC3 &p1, float HalfSize1, const VEC3 &p2, float HalfSize2, uint Slices)
{
	VEC3 p1_world, p2_world;
	Transform(&p1_world, p1, LocalToWorld);
	Transform(&p2_world, p2, LocalToWorld);

	VEC3 vec_z = p2_world - p1_world;
	VEC3 dir_x, dir_y;
	CalcBranchVectors(&dir_x, &dir_y, vec_z);

	VEC3 vec_x_1 = dir_x * HalfSize1;
	VEC3 vec_x_2 = dir_x * HalfSize2;
	VEC3 vec_y_1 = dir_y * HalfSize1;
	VEC3 vec_y_2 = dir_y * HalfSize2;

	float TexScaleY = m_TreeDesc.BarkTexScaleY * Length(vec_z);
	uint2 base_i = VB.size();
	uint2 v_count = Slices * 2;

	float angle = 0.0f, angle_step = PI_X_2 / (float)Slices;
	float tex_x1 = m_TextureDesc.GetRoot().left, tex_x2 = m_TextureDesc.GetRoot().right, tex_x;
	float param = 0.0f, param_step = 1.0f / (float)Slices;
	TREE_VERTEX v;
	v.Diffuse.ARGB = 0x80808080;
	for (uint si = 0; si < Slices; si++)
	{
		tex_x = param * 2.0f;
		if (tex_x > 1.0f) tex_x = 2.0f - tex_x;
		tex_x = Lerp(tex_x1, tex_x2, tex_x);

		v.Normal = dir_x * cosf(angle) + dir_y * sinf(angle);
		Normalize(&v.Normal);
		v.Pos = p1_world + vec_x_1 * cosf(angle) + vec_y_1 * sinf(angle);
		v.Tex = VEC2(tex_x, TexScaleY);
		VB.push_back(v);
		v.Normal = dir_x * cosf(angle) + dir_y * sinf(angle);
		Normalize(&v.Normal);
		v.Pos = p2_world + vec_x_2 * cosf(angle) + vec_y_2 * sinf(angle);
		v.Tex = VEC2(tex_x, 0.0f);
		VB.push_back(v);

		IB.push_back(base_i + ((si  )*2    ) % v_count);
		IB.push_back(base_i + ((si  )*2 + 1) % v_count);
		IB.push_back(base_i + ((si+1)*2 + 1) % v_count);
		IB.push_back(base_i + ((si  )*2    ) % v_count);
		IB.push_back(base_i + ((si+1)*2 + 1) % v_count);
		IB.push_back(base_i + ((si+1)*2    ) % v_count);

		angle += angle_step;
		param += param_step;
	}
}

void Tree_pimpl::GenerateTreeBranch(std::vector<TREE_VERTEX> &VB, std::vector<uint2> &IB, std::vector<VEC3> *LeafPoints, const TREE_DESC &Desc, uint Level, const MATRIX &BranchToWorld, float RemainingParentLength, RandomGenerator &Rand)
{
	const TREE_LEVEL_DESC &LevelDesc = Desc.Levels[Level];

	float Length =
		LevelDesc.Length +
		Rand.RandFloat(-LevelDesc.LengthV, LevelDesc.LengthV) +
		RemainingParentLength * LevelDesc.LengthToParent;

	float HalfSize1 = LevelDesc.Radius + Rand.RandFloat(-LevelDesc.RadiusV, LevelDesc.RadiusV);
	float HalfSize2 = HalfSize1 * LevelDesc.RadiusEnd;

	////// Ga³¹Ÿ
	
	if (LevelDesc.Visible)
	{
		if (Level == 0)
			GenerateBranch_Cylinder(VB, IB, BranchToWorld, VEC3::ZERO, HalfSize1, VEC3(0.0f, 0.0f, Length), HalfSize2, 8);
		else
			GenerateBranch_Cross(VB, IB, BranchToWorld, VEC3::ZERO, HalfSize1, VEC3(0.0f, 0.0f, Length), HalfSize2);
	}

	////// Liœcie

	{
		VEC3 LeafPos = VEC3::ZERO, LeafPos_World;
		float LeafCountF = LevelDesc.LeafCount + Rand.RandFloat(-LevelDesc.LeafCountV, LevelDesc.LeafCountV);
		int LeafCount = round(LeafCountF);
		float Pos = Length * LevelDesc.LeafRangeMin;
		float PosStep;
		if (LeafCount == 1)
			PosStep = 0.0f;
		else
			PosStep = Length * (LevelDesc.LeafRangeMax - LevelDesc.LeafRangeMin) / (float)(LeafCount-1);

		for (int li = 0; li < LeafCount; li++, Pos += PosStep)
		{
			LeafPos.z = Pos + Rand.RandFloat(-PosStep*0.5f, PosStep*0.5f);
			Transform(&LeafPos_World, LeafPos, BranchToWorld);
			LeafPoints->push_back(LeafPos_World);
		}
	}

	////// Podga³êzie

	if (Level < TREE_MAX_LEVELS-1 && LevelDesc.SubbranchCount > 0)
	{
		std::vector<float> AnglesZ;
		AnglesZ.resize(LevelDesc.SubbranchCount);
		float angle_z = 0.0f, angle_z_step = PI_X_2 / (float)LevelDesc.SubbranchCount;
		for (uint sbi = 0; sbi < LevelDesc.SubbranchCount; sbi++, angle_z += angle_z_step)
			AnglesZ[sbi] = angle_z + Rand.RandFloat(angle_z_step*0.5f);
		std::random_shuffle(AnglesZ.begin(), AnglesZ.end(), RandomGeneratorForSTL(Rand));

		float BranchPos = Length * LevelDesc.SubbranchRangeMin;
		float BranchStep;
		if (LevelDesc.SubbranchRangeMax == LevelDesc.SubbranchRangeMin)
			BranchStep = 0.0f;
		else
			BranchStep = Length * (LevelDesc.SubbranchRangeMax - LevelDesc.SubbranchRangeMin) / (float)LevelDesc.SubbranchCount;

		float pos_z, angle_x, my_angle_z;
		MATRIX RotX, RotZ, Trans;

		for (uint sbi = 0; sbi < LevelDesc.SubbranchCount; sbi++)
		{
			pos_z = BranchPos + Rand.RandFloat(-BranchStep*0.5f, BranchStep*0.5f);
			pos_z = minmax(0.0f, pos_z, Length);
			angle_x = LevelDesc.SubbranchAngle + Rand.RandFloat(-LevelDesc.SubbranchAngleV, LevelDesc.SubbranchAngleV);
			my_angle_z = AnglesZ[sbi];

			RotationX(&RotX, angle_x);
			RotationZ(&RotZ, my_angle_z);
			Translation(&Trans, 0.0f, 0.0f, pos_z);

			GenerateTreeBranch(VB, IB, LeafPoints, Desc, Level + 1, RotX * RotZ * Trans * BranchToWorld, Length - pos_z, Rand);

			BranchPos += BranchStep;
			angle_z += angle_z_step;
		}
	}

	////// Korzenie

	if (Level == 0 && Desc.Roots.Visible && Desc.Roots.SubbranchCount > 0)
	{
		std::vector<float> AnglesZ;
		AnglesZ.resize(Desc.Roots.SubbranchCount);
		float angle_z = 0.0f, angle_z_step = PI_X_2 / (float)Desc.Roots.SubbranchCount;
		for (uint sbi = 0; sbi < Desc.Roots.SubbranchCount; sbi++, angle_z += angle_z_step)
			AnglesZ[sbi] = angle_z + Rand.RandFloat(angle_z_step*0.5f);
		std::random_shuffle(AnglesZ.begin(), AnglesZ.end(), RandomGeneratorForSTL(Rand));

		float BranchPos = Length * Desc.Roots.SubbranchRangeMin;
		float BranchStep;
		if (Desc.Roots.SubbranchRangeMax == Desc.Roots.SubbranchRangeMin)
			BranchStep = 0.0f;
		else
			BranchStep = Length * (Desc.Roots.SubbranchRangeMax - Desc.Roots.SubbranchRangeMin) / (float)Desc.Roots.SubbranchCount;

		float pos_z, angle_x, my_angle_z;
		MATRIX RotX, RotZ, Trans;

		for (uint sbi = 0; sbi < Desc.Roots.SubbranchCount; sbi++)
		{
			pos_z = BranchPos + Rand.RandFloat(BranchStep);
			pos_z = minmax(0.0f, pos_z, Length);
			angle_x = Desc.Roots.SubbranchAngle + Rand.RandFloat(-Desc.Roots.SubbranchAngleV, Desc.Roots.SubbranchAngleV);
			my_angle_z = AnglesZ[sbi];

			RotationX(&RotX, angle_x);
			RotationZ(&RotZ, my_angle_z);
			Translation(&Trans, 0.0f, 0.0f, pos_z);

			{
				float RootLength =
					Desc.Roots.Length +
					Rand.RandFloat(-Desc.Roots.LengthV, Desc.Roots.LengthV) +
					(Length - pos_z) * Desc.Roots.LengthToParent;

				MATRIX RootToWorld = RotX * RotZ * Trans * BranchToWorld;
				float HalfSize1 = Desc.Roots.Radius + Rand.RandFloat(-Desc.Roots.RadiusV, Desc.Roots.RadiusV);
				float HalfSize2 = HalfSize1 * Desc.Roots.RadiusEnd;
				
				GenerateBranch_Cylinder(VB, IB, RootToWorld, VEC3::ZERO, HalfSize1, VEC3(0.0f, 0.0f, RootLength), HalfSize2, 8);
			}

			BranchPos += BranchStep;
			angle_z += angle_z_step;
		}
	}
}

/*void Tree_pimpl::ClusterPoints(std::vector<VEC3> *Out, const std::vector<VEC3> &In, uint OutCount, RandomGenerator &Rand)
{
	Out->resize(OutCount);

	////// Przypadki zdegenerowane
	if (OutCount == 0)
		return;
	if (In.empty())
	{
		for (uint i = 0; i < OutCount; i++)
			(*Out)[i] = VEC3::ZERO;
		return;
	}
	if (OutCount >= In.size())
	{
		for (uint i = 0; i < OutCount; i++)
			(*Out)[i] = In[i % In.size()];
		return;
	}

	BOX Box; BoxBoundingPoints(&Box, &In[0], In.size());

	// Wylosuj pozycje pocz¹tkowe grup
	for (uint i = 0; i < OutCount; i++)
	{
		(*Out)[i].x = Rand.RandFloat(Box.p1.x, Box.p2.x);
		(*Out)[i].y = Rand.RandFloat(Box.p1.y, Box.p2.y);
		(*Out)[i].z = Rand.RandFloat(Box.p1.z, Box.p2.z);
	}

	std::vector<VEC3> NewAverages;
	std::vector<uint> NewAverageCounts;
	NewAverages.resize(OutCount);
	NewAverageCounts.resize(OutCount);

	uint Iter = 0;
	bool Changed = false;
	uint ii, oi, best_oi;
	float dist_sq, best_dist_sq;
	VEC3 NewPos;
	while (Changed && Iter < 32)
	{
		Changed = false;

		for (oi = 1; oi < OutCount; oi++)
		{
			NewAverages[oi] = VEC3::ZERO;
			NewAverageCounts[oi] = 0;
		}

		// Dla ka¿dego punktu w In znajdŸ grupê, do której nale¿y i od razu dolicz do œredniej pozycji punktów nale¿¹cych do grupy
		for (ii = 0; ii < In.size(); ii++)
		{
			best_oi = 0;
			best_dist_sq = DistanceSq(In[ii], (*Out)[0]);
			for (oi = 1; oi < Out->size(); oi++)
			{
				dist_sq = DistanceSq(In[ii], (*Out)[oi]);
				if (dist_sq < best_dist_sq)
				{
					best_oi = oi;
					best_dist_sq = dist_sq;
				}
			}

			NewAverages[best_oi] += In[ii];
			NewAverageCounts[best_oi] += 1;
		}

		// Przepisz œrednie pozycji przynale¿nych punktów z In jako nowe pozycje grup do Out
		for (oi = 0; oi < OutCount; oi++)
		{
			NewPos = NewAverages[oi] / (float)NewAverageCounts[oi];
			if (NewPos != (*Out)[oi])
			{
				(*Out)[oi] = NewPos;
				Changed = true;
			}
		}

		Iter++;
	}
}*/

void Tree_pimpl::GenerateLeafQuads(std::vector<TREE_VERTEX> &VB, std::vector<uint2> &IB, const std::vector<VEC3> &Points, RandomGenerator &Rand)
{
	uint base_vi;
	TREE_VERTEX v;
	v.Diffuse.ARGB = 0x80808080;

	RECTF LeafTex = m_TextureDesc.GetLeaves();
	float half_cx, half_cy; // zakres 0..2, odchylenie rozmiaru quada od normy równej 1
	float x1_quad, x2_quad, y1_quad, y2_quad; // zakres -2..2
	uint1 x1_byte, x2_byte, y1_byte, y2_byte; // zakres 0..255 odwzorowuj¹cy -2..2

	VEC3 Normal;
	uint1 Random11, Random12, Random21, Random22;

	for (uint pi = 0; pi < Points.size(); pi++)
	{
		base_vi = VB.size();

		Random11 = (uint1)Rand.RandUint(256);
		Random12 = (uint1)Rand.RandUint(256);
		Random21 = (uint1)Rand.RandUint(256);
		Random22 = (uint1)Rand.RandUint(256);

		if (Rand.RandBool())
			std::swap(LeafTex.left, LeafTex.right);
		if (m_TreeDesc.AllowTexFlipX && Rand.RandBool())
			std::swap(LeafTex.top, LeafTex.bottom);

		half_cx = 1.0f + Rand.RandFloat(-m_TreeDesc.LeafHalfSizeCXV, m_TreeDesc.LeafHalfSizeCXV) / m_TreeDesc.LeafHalfSizeCX;
		half_cy = 1.0f + Rand.RandFloat(-m_TreeDesc.LeafHalfSizeCYV, m_TreeDesc.LeafHalfSizeCYV) / m_TreeDesc.LeafHalfSizeCY;
		half_cx = minmax(0.0f, half_cx, 2.0f);
		half_cy = minmax(0.0f, half_cy, 2.0f);

		x1_quad = -half_cx;
		x2_quad =  half_cx;
		y1_quad = -half_cy;
		y2_quad =  half_cy;

		x1_byte = (uint1)( (x1_quad + 2.0f) / 4.0f * 255.0f );
		x2_byte = (uint1)( (x2_quad + 2.0f) / 4.0f * 255.0f );
		y1_byte = (uint1)( (y1_quad + 2.0f) / 4.0f * 255.0f );
		y2_byte = (uint1)( (y2_quad + 2.0f) / 4.0f * 255.0f );

		Normal = Points[pi];
		Normal.y -= m_TreeDesc.Levels[0].Length * 0.6f;
		Normalize(&Normal);
		// Podwy¿szenie Normal, ¿eby mia³o zawsze y >= 0.
		Normal.y = Normal.y * 0.5f + 0.5f;
		Normalize(&Normal);

		v.Pos = Points[pi];
		v.Normal = Normal;
		v.Tex = VEC2(LeafTex.left, LeafTex.top);
		v.Diffuse.R = x1_byte; v.Diffuse.G = y2_byte;
		v.Diffuse.B = Random11; v.Diffuse.A = Random12;
		VB.push_back(v);
		v.Pos = Points[pi];
		v.Normal = Normal;
		v.Tex = VEC2(LeafTex.left, LeafTex.bottom);
		v.Diffuse.R = x1_byte; v.Diffuse.G = y1_byte;
		v.Diffuse.B = Random11; v.Diffuse.A = Random12;
		VB.push_back(v);
		v.Pos = Points[pi];
		v.Normal = Normal;
		v.Tex = VEC2(LeafTex.right, LeafTex.bottom);
		v.Diffuse.R = x2_byte; v.Diffuse.G = y1_byte;
		v.Diffuse.B = Random21; v.Diffuse.A = Random22;
		VB.push_back(v);
		v.Pos = Points[pi];
		v.Normal = Normal;
		v.Tex = VEC2(LeafTex.right, LeafTex.top);
		v.Diffuse.R = x2_byte; v.Diffuse.G = y2_byte;
		v.Diffuse.B = Random21; v.Diffuse.A = Random22;
		VB.push_back(v);

		IB.push_back(base_vi  );
		IB.push_back(base_vi+1);
		IB.push_back(base_vi+2);
		IB.push_back(base_vi  );
		IB.push_back(base_vi+2);
		IB.push_back(base_vi+3);
	}
}

void Tree_pimpl::GenerateLeaves(std::vector<TREE_VERTEX> &VB, std::vector<uint2> &IB, std::vector<VEC3> *LeafPoints, RandomGenerator &Rand)
{
	std::vector<VEC3> p1, p2;

//LOG(1024, Format("GenerateLeaves: LOD=#, Leaves=#") % 0 % LeafPoints->size());
	GenerateLeafQuads(VB, IB, *LeafPoints, Rand);
}

/*void Tree_pimpl::GenerateBillboard()
{
	TREE_VERTEX v;
	v.Diffuse.ARGB = 0x80808080;
	RECTF BillbaordTex = m_TextureDesc.GetBillboard();

	VEC3 Pos = VEC3(0.0f, 1.0f, 0.0f);

	float half_cy = m_TreeDesc.HalfHeight;
	float half_cx = m_TreeDesc.HalfWidth;

	//if (Rand.RandBool())
	//	std::swap(BillbaordTex.left, BillbaordTex.right);

	float x1_quad = -half_cx;
	float x2_quad =  half_cx;
	float y1_quad = -half_cy;
	float y2_quad =  half_cy;

	uint1 x1_byte = (uint1)( (x1_quad + 2.0f) / 4.0f * 255.0f );
	uint1 x2_byte = (uint1)( (x2_quad + 2.0f) / 4.0f * 255.0f );
	uint1 y1_byte = (uint1)( (y1_quad + 2.0f) / 4.0f * 255.0f );
	uint1 y2_byte = (uint1)( (y2_quad + 2.0f) / 4.0f * 255.0f );

	VEC3 Normal = VEC3::ZERO;

	v.Pos = Pos;
	v.Normal = Normal;
	v.Tex = VEC2(BillbaordTex.left, BillbaordTex.top);
	v.Diffuse.R = x1_byte; v.Diffuse.G = y2_byte;
	m_BillboardVB.push_back(v);
	v.Pos = Pos;
	v.Normal = Normal;
	v.Tex = VEC2(BillbaordTex.left, BillbaordTex.bottom);
	v.Diffuse.R = x1_byte; v.Diffuse.G = y1_byte;
	m_BillboardVB.push_back(v);
	v.Pos = Pos;
	v.Normal = Normal;
	v.Tex = VEC2(BillbaordTex.right, BillbaordTex.bottom);
	v.Diffuse.R = x2_byte; v.Diffuse.G = y1_byte;
	m_BillboardVB.push_back(v);
	v.Pos = Pos;
	v.Normal = Normal;
	v.Tex = VEC2(BillbaordTex.right, BillbaordTex.top);
	v.Diffuse.R = x2_byte; v.Diffuse.G = y2_byte;
	m_BillboardVB.push_back(v);

	m_BillboardIB.push_back(0  );
	m_BillboardIB.push_back(0+1);
	m_BillboardIB.push_back(0+2);
	m_BillboardIB.push_back(0  );
	m_BillboardIB.push_back(0+2);
	m_BillboardIB.push_back(0+3);
}*/

const MATRIX ROOT_TO_WORLD = MATRIX(
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f,-1.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f);

void Tree_pimpl::GenerateTree(uint KindRandomSeeds[])
{
	ERR_TRY;

	// Wype³nij wzorcowe bufory VB i IB dla ka¿dego rodzaju Kind

	std::vector< std::vector< TREE_VERTEX > > PrototypeVertexBuffers;
	std::vector< std::vector< uint2 > > PrototypeIndexBuffers;
	PrototypeVertexBuffers.resize(m_KindCount);
	PrototypeIndexBuffers.resize(m_KindCount);
	m_VerticesPerTree.resize(m_KindCount);
	m_IndicesPerTree.resize(m_KindCount);
	m_KindBaseVertexIndex.resize(m_KindCount);
	m_KindBaseIndexIndex.resize(m_KindCount);

	for (uint ki = 0; ki < m_KindCount; ki++)
	{
		RandomGenerator Rand(KindRandomSeeds[ki]);
		std::vector<VEC3> LeafPoints;

		GenerateTreeBranch(PrototypeVertexBuffers[ki], PrototypeIndexBuffers[ki], &LeafPoints, m_TreeDesc, 0, ROOT_TO_WORLD, 0.0f, Rand);
		GenerateLeaves(PrototypeVertexBuffers[ki], PrototypeIndexBuffers[ki], &LeafPoints, Rand);

		m_VerticesPerTree[ki] = PrototypeVertexBuffers[ki].size();
		m_IndicesPerTree[ki] = PrototypeIndexBuffers[ki].size();

		if (ki == 0)
		{
			m_KindBaseVertexIndex[ki] = 0;
			m_KindBaseIndexIndex[ki] = 0;
		}
		else
		{
			m_KindBaseVertexIndex[ki] = m_KindBaseVertexIndex[ki-1] + m_VerticesPerTree[ki-1];
			m_KindBaseIndexIndex[ki] = m_KindBaseIndexIndex[ki-1] + m_IndicesPerTree[ki-1];
		}
	}

	// Utwórz i wype³nij bufory VB i IB

	uint VbLength = 0, IbLength = 0; // we wierzcho³kach i indeksach, NIE w bajtach
	for (uint ki = 0; ki < m_KindCount; ki++)
	{
		VbLength += m_VerticesPerTree[ki];
		IbLength += m_IndicesPerTree[ki];
	}

	m_VB.reset(new res::D3dVertexBuffer(string(), string(), VbLength, D3DUSAGE_WRITEONLY, TREE_VERTEX_FVF, D3DPOOL_MANAGED));
	m_IB.reset(new res::D3dIndexBuffer(string(), string(), IbLength, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED));
	m_VB->Lock();
	m_IB->Lock();

	{
		res::D3dVertexBuffer::Locker VbLock(m_VB.get(), true, 0);
		TREE_VERTEX *VertexPtr = (TREE_VERTEX*)VbLock.GetData();
		uint vi, vc;
		for (uint ki = 0; ki < m_KindCount; ki++)
		{
			vc = PrototypeVertexBuffers[ki].size();
			for (vi = 0; vi < vc; vi++)
			{
				*VertexPtr = PrototypeVertexBuffers[ki][vi];
				VertexPtr++;
			}
		}
	}

	{
		res::D3dIndexBuffer::Locker IbLock(m_IB.get(), true, 0);
		uint2 *IndexPtr = (uint2*)IbLock.GetData();
		uint index_i, index_c;
		for (uint ki = 0; ki < m_KindCount; ki++)
		{
			index_c = PrototypeIndexBuffers[ki].size();
			for (index_i = 0; index_i < index_c; index_i++)
			{
				*IndexPtr = PrototypeIndexBuffers[ki][index_i];
				IndexPtr++;
			}
		}
	}

	ERR_CATCH_FUNC;
}

IDirect3DTexture9 * Tree_pimpl::EnsureTexture()
{
	res::D3dTexture *Texture = m_TextureDesc.GetTexture();
	if (Texture == NULL)
		throw Error("Brak lub nie mo¿na pobraæ tekstury do narysowania drzewa.", __FILE__, __LINE__);
	Texture->Load();
	return Texture->GetTexture();
}

void Tree_pimpl::PrepareWind(D3DXVECTOR4 *OutQuadWindFactors)
{
	float WindVecLength = Length(m_OwnerScene->GetWindVec()) * WIND_FACTOR;

	// Przygotuj macierz œcinania
	float dt = frame::Timer2.GetDeltaTime();
	const float WindC1 = 0.5f, WindC2 = 0.5f; // Maj¹ siê sumowaæ do 1.
	float WindFreq = WindVecLength;
	m_CurrentWindArgX1 += dt * (PI_X_2 * WindFreq);
	m_CurrentWindArgZ1 += dt * (PI_X_2 * WindFreq);
	float ShearFactor = WindVecLength * 0.03f / m_TreeDesc.Levels[0].Length;
	float ShearX = ( WindC1 * sinf(m_CurrentWindArgX1) + WindC2 * sinf(0.7f * m_CurrentWindArgX1) ) * ShearFactor;
	float ShearZ = ( WindC1 * sinf(m_CurrentWindArgZ1) + WindC2 * sinf(0.7f * m_CurrentWindArgZ1) ) * ShearFactor;
	m_Draw_ShearMat = MATRIX(
		1.0f,   0.0f, 0.0f,   0.0f,
		ShearX, 1.0f, ShearZ, 0.0f,
		0.0f,   0.0f, 1.0f,   0.0f,
		0.0f,   0.0f, 0.0f,   1.0f);

	float WindFactorLength = Length(m_Draw_RightVec) * 0.2f;

	{
		m_CurrentWindArgX1 += dt * (PI_X_2 * WindFreq * 1.05f);
		m_CurrentWindArgZ1 += dt * (PI_X_2 * WindFreq * 1.05f);
		OutQuadWindFactors->x = ( ( WindC1 * sinf(m_CurrentWindArgX2) + WindC2 * sinf(0.7f * m_CurrentWindArgX2) ) ) * WindFactorLength;
		OutQuadWindFactors->y = ( ( WindC1 * sinf(m_CurrentWindArgZ2) + WindC2 * sinf(0.7f * m_CurrentWindArgZ2) ) ) * WindFactorLength;
	}

	{
		m_CurrentWindArgX3 += dt * (PI_X_2 * WindFreq * 1.05f);
		m_CurrentWindArgZ3 += dt * (PI_X_2 * WindFreq * 1.05f);
		OutQuadWindFactors->z = ( ( WindC1 * sinf(m_CurrentWindArgX3) + WindC2 * sinf(0.7f * m_CurrentWindArgX3) ) ) * WindFactorLength;
		OutQuadWindFactors->w = ( ( WindC1 * sinf(m_CurrentWindArgZ3) + WindC2 * sinf(0.7f * m_CurrentWindArgZ3) ) ) * WindFactorLength;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH

Tree::Tree(Scene *OwnerScene, const TREE_DESC &Desc, const TreeTextureDesc &TextureDesc, uint KindCount, uint KindRandomSeeds[]) :
	pimpl(new Tree_pimpl)
{
	assert(KindCount > 0);

	pimpl->m_OwnerScene = OwnerScene;
	pimpl->m_KindCount = KindCount;
	pimpl->m_TreeDesc = Desc;
	pimpl->m_TextureDesc = TextureDesc;

	pimpl->m_TreeShader = NULL;
	// Przypadkowe fazy pocz¹tkowe
	pimpl->m_CurrentWindArgX1 = 2.253f;
	pimpl->m_CurrentWindArgZ1 = -3.243f;
	pimpl->m_CurrentWindArgX2 = 2.253f;
	pimpl->m_CurrentWindArgZ2 = -3.243f;
	pimpl->m_CurrentWindArgX3 = -1.343f;
	pimpl->m_CurrentWindArgZ3 = 2.1324f;

	pimpl->GenerateTree(KindRandomSeeds);
}

Tree::~Tree()
{
	pimpl->m_IB->Unlock();
	pimpl->m_VB->Unlock();
}

const TREE_DESC & Tree::GetDesc()
{
	return pimpl->m_TreeDesc;
}

const TreeTextureDesc & Tree::GetTextureDesc()
{
	return pimpl->m_TextureDesc;
}

uint Tree::GetKindCount()
{
	return pimpl->m_KindCount;
}

void Tree::DrawBegin(const ParamsCamera &Cam, const VEC3 *DirToLight, const COLORF *LightColor, const float *FogStart, const COLORF *FogColor)
{
	pimpl->m_Draw_Cam = &Cam;
	pimpl->m_Draw_DirToLight = DirToLight;
	pimpl->m_Draw_LightColor = LightColor;
	pimpl->m_Draw_ViewProj = Cam.GetMatrices().GetViewProj();

	// Wczytaj teksturê
	res::D3dTexture *Texture = pimpl->m_TextureDesc.GetTexture();
	if (Texture == NULL)
		throw Error("Brak lub nie mo¿na pobraæ tekstury do narysowania drzewa.", __FILE__, __LINE__);
	Texture->Load();

	// Wczytaj multishader
	if (pimpl->m_TreeShader == NULL)
		pimpl->m_TreeShader = res::g_Manager->MustGetResourceEx<res::Multishader>("TreeShader");
	pimpl->m_TreeShader->Load();

	// Sformu³uj makra
	ZeroMem(pimpl->m_Draw_Macros, TSM_COUNT * sizeof(uint));
	if (DirToLight != NULL && LightColor != NULL)
		pimpl->m_Draw_Macros[TSM_LIGHTING] = 1;
	if (FogStart != NULL && FogColor != NULL)
		pimpl->m_Draw_Macros[TSM_FOG] = 1;

	// Wczytaj shader
	pimpl->m_Draw_ShaderInfo = &pimpl->m_TreeShader->GetShader(pimpl->m_Draw_Macros);

	// Przygotuj wektory w prawo i w dó³, we wsp. œwiata
	pimpl->m_Draw_RightVec = Cam.GetRightDir()  * pimpl->m_TreeDesc.LeafHalfSizeCX;
	pimpl->m_Draw_UpVec    = Cam.GetRealUpDir() * pimpl->m_TreeDesc.LeafHalfSizeCY;

	// Przygotuj wspó³czynniki wiatru
	D3DXVECTOR4 QuadWindFactors;
	pimpl->PrepareWind(&QuadWindFactors);

	// Ustaw parametry shadera wspólne dla wszystkich drzew
	ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetTexture(pimpl->m_Draw_ShaderInfo->Params[TSP_TEXTURE], pimpl->EnsureTexture()) );
	ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetVector(pimpl->m_Draw_ShaderInfo->Params[TSP_QUAD_WIND_FACTORS], &QuadWindFactors) );

	// Ustaw parametry mg³y
	if (pimpl->m_Draw_Macros[TSM_FOG])
	{
		float One_Minus_FogStart = 1.f - *FogStart;
		float FogA = 1.f / (Cam.GetZFar() * One_Minus_FogStart);
		float FogB = - *FogStart / One_Minus_FogStart;
		ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetVector(pimpl->m_Draw_ShaderInfo->Params[TSP_FOG_COLOR], &D3DXVECTOR4(FogColor->R, FogColor->G, FogColor->B, FogColor->A)) );
		ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetVector(pimpl->m_Draw_ShaderInfo->Params[TSP_FOG_FACTORS], &D3DXVECTOR4(FogA, FogB, 0.f, 0.f)) );
	}

	// Ustawienia urz¹dzenia
	frame::RestoreDefaultState();
	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	frame::Dev->SetRenderState(D3DRS_ALPHAREF, 0x00000080);

	// Ustaw VB, IB i FVF
	frame::Dev->SetStreamSource(0, pimpl->m_VB->GetVB(), 0, sizeof(TREE_VERTEX));
	frame::Dev->SetIndices(pimpl->m_IB->GetIB());
	frame::Dev->SetFVF(TREE_VERTEX_FVF);

	// Rozpocznij efekt
	uint FooPasses;
	pimpl->m_Draw_ShaderInfo->Effect->Begin(&FooPasses, D3DXFX_DONOTSAVESTATE);
	pimpl->m_Draw_ShaderInfo->Effect->BeginPass(0);
}

void Tree::DrawBegin_ShadowMap(const MATRIX &ViewProj, const VEC3 &LightDir)
{
	pimpl->m_Draw_Cam = NULL;
	pimpl->m_Draw_DirToLight = NULL;
	pimpl->m_Draw_LightColor = NULL;
	pimpl->m_Draw_ViewProj = ViewProj;

	// Wczytaj multishader
	if (pimpl->m_TreeShader == NULL)
		pimpl->m_TreeShader = res::g_Manager->MustGetResourceEx<res::Multishader>("TreeShader");
	pimpl->m_TreeShader->Load();

	// Sformu³uj makra
	ZeroMem(pimpl->m_Draw_Macros, TSM_COUNT * sizeof(uint));
	pimpl->m_Draw_Macros[TSM_RENDER_TO_SHADOW_MAP] = 1;
	if (g_Engine->GetShadowMapFormat() == D3DFMT_A8R8G8B8)
		pimpl->m_Draw_Macros[TSM_SHADOW_MAP_MODE] = 2;
	else
		pimpl->m_Draw_Macros[TSM_SHADOW_MAP_MODE] = 1;

	// Wczytaj shader
	pimpl->m_Draw_ShaderInfo = &pimpl->m_TreeShader->GetShader(pimpl->m_Draw_Macros);

	// Przygotuj wektory w prawo i w dó³, we wsp. œwiata
	VEC3 RightDir, UpDir;
	PerpedicularVectors(&RightDir, &UpDir, LightDir);
	Normalize(&RightDir);
	Normalize(&UpDir);
	pimpl->m_Draw_RightVec = RightDir * pimpl->m_TreeDesc.LeafHalfSizeCX;
	pimpl->m_Draw_UpVec    = UpDir    * pimpl->m_TreeDesc.LeafHalfSizeCY;

	// Przygotuj wspó³czynniki wiatru
	D3DXVECTOR4 QuadWindFactors;
	pimpl->PrepareWind(&QuadWindFactors);

	// Ustaw parametry shadera wspólne dla wszystkich drzew
	ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetTexture(pimpl->m_Draw_ShaderInfo->Params[TSP_TEXTURE], pimpl->EnsureTexture()) );
	ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetVector(pimpl->m_Draw_ShaderInfo->Params[TSP_QUAD_WIND_FACTORS], &QuadWindFactors) );

	// Ustawienia urz¹dzenia
	// UWAGA tutaj na stany! Nie mogê robiæ RestoreDefaultState bo siê render shadow mapy psuje
	// (bardzo dziwny b³¹d - drzewa s¹ przesuniête kiedy rozmiar shadow mapy przekracza rozmiar back buffera),
	// wiêc mog¹ tu zostaæ jakieœ dziwne niepo¿¹dane ustawienia.
	//frame::RestoreDefaultState();
	frame::Dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	frame::Dev->SetRenderState(D3DRS_ZENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	frame::Dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	frame::Dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	frame::Dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	frame::Dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	frame::Dev->SetRenderState(D3DRS_ALPHAREF, 0x00000080);

	// Ustaw VB, IB i FVF
	frame::Dev->SetStreamSource(0, pimpl->m_VB->GetVB(), 0, sizeof(TREE_VERTEX));
	frame::Dev->SetIndices(pimpl->m_IB->GetIB());
	frame::Dev->SetFVF(TREE_VERTEX_FVF);

	// Rozpocznij efekt
	uint FooPasses;
	pimpl->m_Draw_ShaderInfo->Effect->Begin(&FooPasses, D3DXFX_DONOTSAVESTATE);
	pimpl->m_Draw_ShaderInfo->Effect->BeginPass(0);
}

void Tree::DrawEnd()
{
	// Zakoñcz efekt
	pimpl->m_Draw_ShaderInfo->Effect->EndPass();
	pimpl->m_Draw_ShaderInfo->Effect->End();
}

void Tree::DrawTree(uint Kind, const MATRIX &WorldMat, const MATRIX *InvWorldMat, const COLORF &Color)
{
	assert(Kind < pimpl->m_KindCount);

	// Przygotuj odwrotn¹ macierz œwiata
	MATRIX WorldInv;
	if (InvWorldMat)
		WorldInv = *InvWorldMat;
	else
		Inverse(&WorldInv, WorldMat);

	// Wylicz wektory w prawo i w górê we wsp. lokalnych drzewa
	VEC3 MyRightVec, MyUpVec, MyLightDir;
	TransformNormal(&MyRightVec, pimpl->m_Draw_RightVec, WorldInv);
	TransformNormal(&MyUpVec, pimpl->m_Draw_UpVec, WorldInv);
	ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetVector(pimpl->m_Draw_ShaderInfo->Params[TSP_RIGHT_VEC], &D3DXVECTOR4(MyRightVec.x, MyRightVec.y, MyRightVec.z, 0.0f)) );
	ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetVector(pimpl->m_Draw_ShaderInfo->Params[TSP_UP_VEC], &D3DXVECTOR4(MyUpVec.x, MyUpVec.y, MyUpVec.z, 0.0f)) );

	// Ustaw macierz WorldViewProj
	ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetMatrix(pimpl->m_Draw_ShaderInfo->Params[TSP_WORLD_VIEW_PROJ],
		(const D3DXMATRIX*)&(pimpl->m_Draw_ShearMat * WorldMat * pimpl->m_Draw_ViewProj)) );

	// Ustaw dane oœwietlenia
	if (pimpl->m_Draw_Macros[TSM_LIGHTING])
	{
		TransformNormal(&MyLightDir, *pimpl->m_Draw_DirToLight, WorldInv);
		Normalize(&MyLightDir);
		ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetVector(pimpl->m_Draw_ShaderInfo->Params[TSP_LIGHT_COLOR], &D3DXVECTOR4(
			pimpl->m_Draw_LightColor->R * Color.R,
			pimpl->m_Draw_LightColor->G * Color.G,
			pimpl->m_Draw_LightColor->B * Color.B,
			pimpl->m_Draw_LightColor->A * Color.A)) );
		ERR_GUARD_DIRECTX_D( pimpl->m_Draw_ShaderInfo->Effect->SetVector(pimpl->m_Draw_ShaderInfo->Params[TSP_DIR_TO_LIGHT], &D3DXVECTOR4(MyLightDir.x, MyLightDir.y, MyLightDir.z, 0.0f)) );
	}

	pimpl->m_Draw_ShaderInfo->Effect->CommitChanges();

	uint PrimitiveCount = pimpl->m_IndicesPerTree[Kind] / 3;
	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST, // PrimitiveType
		pimpl->m_KindBaseVertexIndex[Kind], // BaseVertexIndex
		0, // MinVertexIndex (dla optymalizacji)
		pimpl->m_VerticesPerTree[Kind], // NumVertices (dla optymalizacji)
		pimpl->m_KindBaseIndexIndex[Kind], // StartIndex
		PrimitiveCount) ); // PrimitiveCount
	frame::RegisterDrawCall(PrimitiveCount);
}


} // namespace engine
