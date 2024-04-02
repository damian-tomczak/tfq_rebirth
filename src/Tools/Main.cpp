/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "PCH.hpp"
#include "MeshTask.hpp"
#include "MapTask.hpp"
#include "TextureTask.hpp"


void PrintIntro()
{
	Writeln("The Final Quest - Tools");
	Writeln("Uniwersalne narzêdzie konsolowe.");
	Writeln("Autor: Adam Sawicki <sawickiap@poczta.onet.pl>");
	Writeln("Po szczegó³y zobacz plik Tools.txt");
	Writeln();
}

int main(int argc, char **argv)
{
	try
	{
		CmdLineParser Parser(argc, argv);

		Parser.RegisterOpt(1, "Mesh", false);
		Parser.RegisterOpt(2, "Map", false);
		Parser.RegisterOpt(3, "Texture", false);
		Parser.RegisterOpt(1001, 'i', true);
		Parser.RegisterOpt(1002, 'o', true);
		Parser.RegisterOpt(1003, 'I', false);
		Parser.RegisterOpt(1004, 'v', false);
		Parser.RegisterOpt(1005, 'd', true);
		Parser.RegisterOpt(2001, "MaxSmoothAngle",         true);
		Parser.RegisterOpt(2002, "NoCalcNormals",          false);
		Parser.RegisterOpt(2003, "RespectExistingSplits",  false);
		Parser.RegisterOpt(2004, "FixCylindricalWrapping", false);
		Parser.RegisterOpt(2005, "Tangents",               false);
		Parser.RegisterOpt(2006, "Bounds",                 true);
		Parser.RegisterOpt(2007, "K",                      true);
		Parser.RegisterOpt(2008, "MaxOctreeDepth",         true);
		Parser.RegisterOpt(2009, "AlphaThreshold",         true);
		Parser.RegisterOpt(3001, "Translate",              true);
		Parser.RegisterOpt(3002, "RotateYawPitchRoll",     true);
		Parser.RegisterOpt(3003, "RotateQuaternion",       true);
		Parser.RegisterOpt(3004, "RotateAxisAngle",        true);
		Parser.RegisterOpt(3005, "Scale",                  true);
		Parser.RegisterOpt(3006, "Transform",              true);
		Parser.RegisterOpt(3007, "CenterOnAverage",        false);
		Parser.RegisterOpt(3008, "CenterOnMedian",         false);
		Parser.RegisterOpt(3010, "TransformToBox",         true);
		Parser.RegisterOpt(3011, "TransformToBoxProportional", true);
		Parser.RegisterOpt(4001, "TranslateTex", true);
		Parser.RegisterOpt(4002, "ScaleTex",     true);
		Parser.RegisterOpt(4003, "TransformTex", true);
		Parser.RegisterOpt(5001, "NormalizeNormals", false);
		Parser.RegisterOpt(5002, "ZeroNormals", false);
		Parser.RegisterOpt(5003, "DeleteTangents", false);
		Parser.RegisterOpt(5004, "DeleteSkinning", false);
		Parser.RegisterOpt(5005, "DeleteObject", true);
		Parser.RegisterOpt(5006, "RenameObject", true);
		Parser.RegisterOpt(5007, "RenameMaterial", true);
		Parser.RegisterOpt(5008, "SetMaterial", true);
		Parser.RegisterOpt(5009, "SetAllMaterials", true);
		Parser.RegisterOpt(5010, "RenameBone", true);
		Parser.RegisterOpt(5011, "RenameAnimation", true);
		Parser.RegisterOpt(5012, "DeleteAnimation", true);
		Parser.RegisterOpt(5013, "DeleteAllAnimations", false);
		Parser.RegisterOpt(5014, "ScaleTime", true);
		Parser.RegisterOpt(5015, "OptimizeAnimations", false);
		Parser.RegisterOpt(5016, "RecalcBoundings", false);
		Parser.RegisterOpt(5017, "FlipNormals", false);
		Parser.RegisterOpt(5018, "FlipFaces", false);
		Parser.RegisterOpt(5019, "RemoveAnimationMovement", false);
		Parser.RegisterOpt(5020, "Validate", false);
		Parser.RegisterOpt(5021, "NormalizeQuaternions", false);
		Parser.RegisterOpt(6000, "Mend", false);
		Parser.RegisterOpt(7001, "GenRand", true);
		Parser.RegisterOpt(7002, "Swizzle", true);
		Parser.RegisterOpt(7003, "SharpenAlpha", true);
		Parser.RegisterOpt(7004, "ClampTransparent", false);

		CmdLineParser::RESULT R = Parser.ReadNext();
		if (R == CmdLineParser::RESULT_END)
		{
			PrintIntro();
			return 0;
		}
		else if (R == CmdLineParser::RESULT_OPT)
		{
			// /Mesh
			if (Parser.GetOptId() == 1)
			{
				MeshJob Job;

				for (;;)
				{
					R = Parser.ReadNext();
					if (R == CmdLineParser::RESULT_END)
						break;
					else if (R == CmdLineParser::RESULT_OPT)
					{
						switch (Parser.GetOptId())
						{
						case 1001:
							{
								InputMeshTask *t = new InputMeshTask;
								t->FileName = Parser.GetParameter();
								Job.Tasks.push_back(shared_ptr<MeshTask>(t));
							}
							break;
						case 1002:
							{
								OutputMeshTask *t = new OutputMeshTask;
								t->FileName = Parser.GetParameter();
								Job.Tasks.push_back(shared_ptr<MeshTask>(t));
							}
							break;
						case 1003:
							{
								InfoMeshTask *t = new InfoMeshTask;
								t->Detailed = false;
								Job.Tasks.push_back(shared_ptr<MeshTask>(t));
							}
							break;
						case 1004:
							{
								InfoMeshTask *t = new InfoMeshTask;
								t->Detailed = true;
								Job.Tasks.push_back(shared_ptr<MeshTask>(t));
							}
							break;
						case 2001:
							if (StrToFloat(&Job.MaxSmoothAngle, Parser.GetParameter()) != 0)
								ThrowCmdLineSyntaxError();
							break;
						case 2002:
							Job.CalcNormals = false;
							break;
						case 2003:
							Job.RespectExistingSplits = true;
							break;
						case 2004:
							Job.FixCylindricalWrapping = true;
							break;
						case 2005:
							Job.Tangents = true;
							break;
						case 3001:
						case 3002:
						case 3003:
						case 3004:
						case 3005:
						case 3006:
						case 3010:
						case 3011:
							{
								TransformMeshTask *t = new TransformMeshTask(Parser.GetOptId(), Parser.GetParameter());
								Job.Tasks.push_back(shared_ptr<MeshTask>(t));
							}
							break;
						case 3007:
						case 3008:
							{
								TransformMeshTask *t = new TransformMeshTask(Parser.GetOptId(), string());
								Job.Tasks.push_back(shared_ptr<MeshTask>(t));
							}
							break;
						case 4001:
						case 4002:
						case 4003:
							{
								TransformTexMeshTask *t = new TransformTexMeshTask(Parser.GetOptId(), Parser.GetParameter());
								Job.Tasks.push_back(shared_ptr<MeshTask>(t));
							}
							break;
						case 5001:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new NormalizeNormalsMeshTask()));
							break;
						case 5002:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new ZeroNormalsMeshTask()));
							break;
						case 5003:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new DeleteTangentsMeshTask()));
							break;
						case 5004:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new DeleteSkinningMeshTask()));
							break;
						case 5005:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new DeleteObjectMeshTask(Parser.GetParameter())));
							break;
						case 5006:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new RenameObjectMeshTask(Parser.GetParameter())));
							break;
						case 5007:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new RenameMaterialMeshTask(Parser.GetParameter())));
							break;
						case 5008:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new SetMaterialMeshTask(Parser.GetParameter())));
							break;
						case 5009:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new SetAllMaterialsMeshTask(Parser.GetParameter())));
							break;
						case 5010:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new RenameBoneMeshTask(Parser.GetParameter())));
							break;
						case 5011:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new RenameAnimationMeshTask(Parser.GetParameter())));
							break;
						case 5012:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new DeleteAnimationMeshTask(Parser.GetParameter())));
							break;
						case 5013:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new DeleteAllAnimationsMeshTask()));
							break;

						case 5014:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new ScaleTimeMeshTask(Parser.GetParameter())));
							break;
						case 5015:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new OptimizeAnimationsMeshTask()));
							break;
						case 5016:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new RecalcBoundingsMeshTask()));
							break;
						case 5017:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new FlipNormalsMeshTask()));
							break;
						case 5018:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new FlipFacesMeshTask()));
							break;
						case 5019:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new RemoveAnimationMovementMeshTask()));
							break;
						case 5020:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new ValidateMeshTask()));
							break;
						case 5021:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new NormalizeQuaternionsMeshTask()));
							break;
						case 6000:
							Job.Tasks.push_back(shared_ptr<MeshTask>(new MendMeshTask()));
							break;
						default:
							ThrowCmdLineSyntaxError();
						}
					}
					else
						ThrowCmdLineSyntaxError();
				}
				DoMeshJob(Job);
			}
			// /Map
			else if (Parser.GetOptId() == 2)
			{
				MapJob Job;

				for (;;)
				{
					R = Parser.ReadNext();
					if (R == CmdLineParser::RESULT_END)
						break;
					else if (R == CmdLineParser::RESULT_OPT)
					{
						switch (Parser.GetOptId())
						{
						case 1001:
							{
								InputMapTask *t = new InputMapTask;
								t->FileName = Parser.GetParameter();
								Job.Tasks.push_back(shared_ptr<MapTask>(t));
							}
							break;
						case 1002:
							{
								OutputMapTask *t = new OutputMapTask;
								t->FileName = Parser.GetParameter();
								Job.Tasks.push_back(shared_ptr<MapTask>(t));
							}
							break;
						case 1003:
							{
								InfoMapTask *t = new InfoMapTask;
								t->Detailed = false;
								Job.Tasks.push_back(shared_ptr<MapTask>(t));
							}
							break;
						case 1004:
							{
								InfoMapTask *t = new InfoMapTask;
								t->Detailed = true;
								Job.Tasks.push_back(shared_ptr<MapTask>(t));
							}
							break;
						case 1005: // /d
							Job.DescFileName = Parser.GetParameter();
							break;
						case 2006: // /Bounds
							if (StrToSth<BOX>(&Job.Bounds, Parser.GetParameter()) != 0)
								ThrowCmdLineSyntaxError();
							break;
						case 2007: // /K
							if (StrToFloat(&Job.OctreeK, Parser.GetParameter()) != 0)
								ThrowCmdLineSyntaxError();
							if (Job.OctreeK < 0.f || Job.OctreeK >= 1.f)
								throw Error("K musi spe³niaæ: 0 <= K < 1");
							break;
						case 2008: // /MaxOctreeDepth
							if (StrToUint(&Job.MaxOctreeDepth, Parser.GetParameter()) != 0)
								ThrowCmdLineSyntaxError();
							break;
						default:
							ThrowCmdLineSyntaxError();
						}
					}
					else
						ThrowCmdLineSyntaxError();
				}
				DoMapJob(Job);
			}
			// /Texture
			else if (Parser.GetOptId() == 3)
			{
				TextureJob Job;

				for (;;)
				{
					R = Parser.ReadNext();
					if (R == CmdLineParser::RESULT_END)
						break;
					else if (R == CmdLineParser::RESULT_OPT)
					{
						switch (Parser.GetOptId())
						{
						case 2009: // /AlphaThreshold
							MustStrToSth<uint1>(&Job.AlphaThreshold, Parser.GetParameter());
							if (Job.AlphaThreshold == 0)
								throw Error("Alpha threshold must be greater than 0.");
							break;
						case 7001: // /GenRand
							Job.Tasks.push_back(shared_ptr<TextureTask>(new GenRandTextureTask(Parser.GetParameter())));
							break;
						case 1001: // /i
							{
								InputTextureTask *t = new InputTextureTask;
								t->FileName = Parser.GetParameter();
								Job.Tasks.push_back(shared_ptr<TextureTask>(t));
							}
							break;
						case 1002: // /o
							{
								OutputTextureTask *t = new OutputTextureTask;
								t->FileName = Parser.GetParameter();
								Job.Tasks.push_back(shared_ptr<TextureTask>(t));
							}
							break;
						case 1003: // /I
							{
								InfoTextureTask *t = new InfoTextureTask;
								t->Detailed = false;
								Job.Tasks.push_back(shared_ptr<TextureTask>(t));
							}
							break;
						case 1004: // /v
							{
								InfoTextureTask *t = new InfoTextureTask;
								t->Detailed = true;
								Job.Tasks.push_back(shared_ptr<TextureTask>(t));
							}
							break;
						case 7002: // /Swizzle
							Job.Tasks.push_back(shared_ptr<TextureTask>(new SwizzleTextureTask(Parser.GetParameter())));
							break;
						case 7003: // /SharpenAlpha
							Job.Tasks.push_back(shared_ptr<TextureTask>(new SharpenAlphaTextureTask(Parser.GetParameter())));
							break;
						case 7004: // /ClampTransparent
							Job.Tasks.push_back(shared_ptr<TextureTask>(new ClampTransparentTextureTask()));
							break;
						default:
							ThrowCmdLineSyntaxError();
						}
					}
					else
						ThrowCmdLineSyntaxError();
				}
				DoTextureJob(Job);
			}
			else
				ThrowCmdLineSyntaxError();
		}
		else
			ThrowCmdLineSyntaxError();

		Writeln("Done.");
		return 0;
	}
	catch (const Error &e)
	{
		string Msg;
		e.GetMessage_(&Msg, "  ");
		Writeln("ERROR:");
		Writeln(Msg);
		return -1;
	}
}
