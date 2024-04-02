/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include <unordered_map>
#include "Framework.hpp"
#include "D3dUtils.hpp"
#include "Multishader.hpp"

namespace res
{


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Multishader

class Multishader_pimpl
{
public:
	// Hash => Dane wczytanego efektu
	typedef std::unordered_map<uint4, Multishader::SHADER_INFO> SHADER_MAP;

	string m_SourceFileName;
	string m_CacheFileNameMask;
	SHADER_MAP m_Shaders;
	uint4 m_MacroCount;
	STRING_VECTOR m_MacroNames;
	std::vector<uint> m_MacroBits;
	uint4 m_ParamCount;
	STRING_VECTOR m_ParamNames;

	// Wczytany do pamiêci plik Ÿród³owy MainShader.fx lub NULL jeœli nie wczytany
	// Jest wczytywany przy pierwszym u¿yciu.
	scoped_ptr<MemoryStream> m_ShaderSource;

	// Zwalanie wszystkie wczytane efekty
	void FreeEffects();
	// Wczytuje do pamiêci m_ShaderSource, jeœli jeszcze nie wczytane
	void EnsureShaderSource();
};


void Multishader_pimpl::FreeEffects()
{
	for (SHADER_MAP::iterator srit = m_Shaders.begin(); srit != m_Shaders.end(); ++srit)
		SAFE_RELEASE(srit->second.Effect);
	m_Shaders.clear();
}

void Multishader_pimpl::EnsureShaderSource()
{
	ERR_TRY;

	if (m_ShaderSource == NULL)
	{
		LOG(LOG_RESMNGR, "Multishader: Loading shader source: " + m_SourceFileName);

		FileStream File(m_SourceFileName, FM_READ, false);
		m_ShaderSource.reset(new MemoryStream(File.GetSize()));
		m_ShaderSource->CopyFromToEnd(&File);
	}

	ERR_CATCH_FUNC;
}


Multishader::SHADER_INFO::SHADER_INFO(ID3DXEffect *Effect, uint4 ParamCount, const STRING_VECTOR &ParamNames) :
	Effect(Effect)
{
#define GUARD(name, expr) { if((expr) == NULL) throw DirectXError(hr, "Nie mo¿na pobraæ parametru: " + string(name)); }

	// Pobierz uchwyty do parametrów
	HRESULT hr = S_OK;

	Params.resize(ParamCount);
	for (size_t i = 0; i < ParamCount; i++)
	{
		GUARD( ParamNames[i], Params[i] = Effect->GetParameterByName(NULL, ParamNames[i].c_str()) );
	}

#undef GUARD
}

void Multishader::RegisterResourceType()
{
	g_Manager->RegisterResourceType("Multishader", &Multishader::Create);
}

IResource * Multishader::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	string SourceFileName, CacheFileNameMask;
	STRING_VECTOR MacroNames, ParamNames;
	std::vector<uint> MacroBits;

	Params.AssertIdentifier("Source");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	SourceFileName = Params.GetString();
	Params.Next();

	Params.AssertIdentifier("Cache");
	Params.Next();
	Params.AssertSymbol('=');
	Params.Next();
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	CacheFileNameMask = Params.GetString();
	Params.Next();

	Params.AssertIdentifier("Macros");
	Params.Next();
	Params.AssertSymbol('{');
	Params.Next();
	while (!Params.QuerySymbol('}'))
	{
		Params.AssertToken(Tokenizer::TOKEN_STRING);
		MacroNames.push_back(Params.GetString());
		Params.Next();

		Params.AssertToken(Tokenizer::TOKEN_INTEGER);
		MacroBits.push_back(Params.MustGetUint4());
		Params.Next();

		if (Params.QuerySymbol(','))
			Params.Next();
	}
	Params.Next();

	Params.AssertIdentifier("Params");
	Params.Next();
	Params.AssertSymbol('{');
	Params.Next();
	while (!Params.QuerySymbol('}'))
	{
		Params.AssertToken(Tokenizer::TOKEN_STRING);
		ParamNames.push_back(Params.GetString());
		Params.Next();

		if (Params.QuerySymbol(','))
			Params.Next();
	}
	Params.Next();

	Params.AssertSymbol(';');
	Params.Next();

	return new Multishader(
		Name,
		Group,
		SourceFileName,
		CacheFileNameMask,
		MacroNames,
		(MacroBits.empty() ? NULL : &MacroBits[0]),
		ParamNames);
}

Multishader::Multishader(
	const string &Name, const string &Group,
	const string &SourceFileName, const string &CacheFileNameMask,
	uint MacroCount, const char *MacroNames[], const uint MacroBits[],
	uint ParamCount, const char *ParamNames[])
:
	D3dResource(Name, Group)
{
	pimpl.reset(new Multishader_pimpl);
	pimpl->m_SourceFileName = SourceFileName;
	pimpl->m_CacheFileNameMask = CacheFileNameMask;
	pimpl->m_MacroCount = MacroCount;
	pimpl->m_ParamCount = ParamCount;
	for (uint i = 0; i < MacroCount; i++)
	{
		pimpl->m_MacroNames.push_back(MacroNames[i]);
		pimpl->m_MacroBits.push_back(MacroBits[i]);
	}
	for (uint i = 0; i < ParamCount; i++)
		pimpl->m_ParamNames.push_back(ParamNames[i]);
}

Multishader::Multishader(
	const string &Name, const string &Group,
	const string &SourceFileName, const string &CacheFileNameMask,
	const STRING_VECTOR &MacroNames, const uint MacroBits[],
	const STRING_VECTOR &ParamNames)
:
	D3dResource(Name, Group)
{
	ERR_TRY;

	pimpl.reset(new Multishader_pimpl);
	pimpl->m_SourceFileName = SourceFileName;
	pimpl->m_CacheFileNameMask = CacheFileNameMask;
	pimpl->m_MacroCount = MacroNames.size();
	pimpl->m_ParamCount = ParamNames.size();
	for (uint i = 0; i < MacroNames.size(); i++)
	{
		pimpl->m_MacroNames.push_back(MacroNames[i]);
		pimpl->m_MacroBits.push_back(MacroBits[i]);
	}
	for (uint i = 0; i < ParamNames.size(); i++)
		pimpl->m_ParamNames.push_back(ParamNames[i]);

	ERR_CATCH_FUNC;
}

Multishader::~Multishader()
{
	pimpl->FreeEffects();
	pimpl.reset();
}

uint Multishader::GetShaderCount()
{
	return pimpl->m_Shaders.size();
}

void Multishader::MacrosToStr(string *Out, uint Macros[])
{
	Out->clear();

	for (size_t mi = 0; mi < pimpl->m_MacroCount; mi++)
	{
		if (mi > 0)
			*Out += ", ";
		Out->append(Format("#=#") % pimpl->m_MacroNames[mi] % Macros[mi]);
	}
}

const Multishader::SHADER_INFO & Multishader::GetShader(uint Macros[])
{
	assert(IsLoaded() && "Multishader::GetShader podczas gdy zasób nie jest IsLoaded.");

	try
	{
		uint4 Hash = 0;
		for (size_t i = 0; i < pimpl->m_MacroCount; i++)
			Hash |= Macros[i] << pimpl->m_MacroBits[i];

		// Poszukaj w mapie
		Multishader_pimpl::SHADER_MAP::iterator sit = pimpl->m_Shaders.find(Hash);

		// Shader nie znaleziony - wygeneruj
		if (sit == pimpl->m_Shaders.end())
		{
			assert(!frame::GetDeviceLost() && "Multishader::GetShader podczas gry urz¹dzenie D3D jest utracone.");

			// Skonstruuj nazwê pliku z cache efektu
			string HashHexStr; UintToStr2(&HashHexStr, Hash, 8, 16);
			string CacheFileName = Format(pimpl->m_CacheFileNameMask) % HashHexStr;

			bool ReadFromCache = false;
			// Plik z z cache efektu istnieje
			if (GetFileItemType(CacheFileName) == IT_FILE)
			{
				// Pobierz datê modyfikacji shadera
				DATETIME ShaderFileTime;
				MustGetFileItemInfo(pimpl->m_SourceFileName, NULL, NULL, &ShaderFileTime, NULL, NULL);
				// Pobierz datê modyfikacji pliku cache
				DATETIME CacheFileTime;
				MustGetFileItemInfo(CacheFileName, NULL, NULL, &CacheFileTime, NULL, NULL);
				// Porównaj daty
				if (ShaderFileTime < CacheFileTime)
				{
					LOG(LOG_RESMNGR, Format("Multishader: Loading from cache. Shader=#, Hash=#") % GetName() % HashHexStr);

					// Wczytaj go
					ID3DXBuffer *ErrBufPtr;
					ID3DXEffect *Effect;
					HRESULT hr = D3DXCreateEffectFromFile(
						frame::Dev,
						CacheFileName.c_str(),
						NULL,
						NULL,
						D3DXFX_DONOTSAVESTATE,
						NULL,
						&Effect,
						&ErrBufPtr);
					// Ale hack! Tutaj przyda³by siê inteligentny wskaŸnik robi¹cy w destruktorze Release.
					scoped_ptr<D3dxBufferWrapper> ErrBuf;
					if (ErrBufPtr)
						ErrBuf.reset(new D3dxBufferWrapper(ErrBufPtr));
					if (FAILED(hr))
					{
						string Msg;
						if (ErrBuf != NULL)
							Msg.append((char*)ErrBuf->GetData(), ErrBuf->GetNumBytes());

						if (Msg.empty())
							throw DirectXError(hr, "Nie mo¿na wczytaæ efektu silnika z pliku \""+CacheFileName+"\"", __FILE__, __LINE__);
						else
							throw DirectXError(hr, "Nie mo¿na wczytaæ efektu silnika z pliku \""+CacheFileName+"\": "+Msg, __FILE__, __LINE__);
					}
					sit = pimpl->m_Shaders.insert(std::make_pair(Hash, SHADER_INFO(Effect, pimpl->m_ParamCount, pimpl->m_ParamNames))).first;
					ReadFromCache = true;
				}
			}
			// Plik z cache efektu nie istnieje lub jest przestarza³y - skompiluj od nowa
			if (!ReadFromCache)
			{
				// Wczytaj Ÿród³o shadera
				pimpl->EnsureShaderSource();

				LOG(LOG_RESMNGR, Format("Multishader: Compiling. Shader=#, Hash=#") % GetName() % HashHexStr);

				// Sformu³uj makra
				std::vector<D3DXMACRO> D3dMacros;
				D3dMacros.resize(pimpl->m_MacroCount+1);
				STRING_VECTOR MacroValues;
				MacroValues.resize(pimpl->m_MacroCount);
				uint4 mi;
				for (mi = 0; mi < pimpl->m_MacroCount; mi++)
				{
					UintToStr(&MacroValues[mi], Macros[mi]);
					D3dMacros[mi].Name = pimpl->m_MacroNames[mi].c_str();
					D3dMacros[mi].Definition = MacroValues[mi].c_str();
				}
				D3dMacros[mi].Name = NULL;
				D3dMacros[mi].Definition = NULL;

				// Utwórz "kompilator"
				scoped_ptr<ID3DXEffectCompiler, ReleasePolicy> Compiler;
				{
					ID3DXBuffer *ErrBufPtr;
					ID3DXEffectCompiler *CompilerPtr;
					HRESULT hr = D3DXCreateEffectCompiler(
						pimpl->m_ShaderSource->Data(),
						pimpl->m_ShaderSource->GetSize(),
						&D3dMacros[0],
						NULL,
						0,
						&CompilerPtr,
						&ErrBufPtr);
					scoped_ptr<D3dxBufferWrapper> ErrBuf;
					if (ErrBufPtr)
						ErrBuf.reset(new D3dxBufferWrapper(ErrBufPtr));
					if (CompilerPtr)
						Compiler.reset(CompilerPtr);
					if (FAILED(hr))
					{
						string Msg;
						if (ErrBuf != NULL)
							Msg.append((char*)ErrBuf->GetData(), ErrBuf->GetNumBytes());

						if (Msg.empty())
							throw DirectXError(hr, "Nie mo¿na wczytaæ (D3DXCreateEffectCompilerFromFile) efektu silnika z pliku \""+string(pimpl->m_SourceFileName)+"\"", __FILE__, __LINE__);
						else
							throw DirectXError(hr, "Nie mo¿na wczytaæ (D3DXCreateEffectCompilerFromFile) efektu silnika z pliku \""+string(pimpl->m_SourceFileName)+"\": "+Msg, __FILE__, __LINE__);
					}
				}

				// Skompiluj efekt do bufora
				scoped_ptr<D3dxBufferWrapper> EffectBuf;
				{
					ID3DXBuffer *EffectBufPtr;
					ID3DXBuffer *ErrBufPtr;
					HRESULT hr = Compiler->CompileEffect(0, &EffectBufPtr, &ErrBufPtr);
					scoped_ptr<D3dxBufferWrapper> ErrBuf;
					if (ErrBufPtr)
						ErrBuf.reset(new D3dxBufferWrapper(ErrBufPtr));
					if (FAILED(hr))
					{
						string Msg;
						if (ErrBuf != NULL)
							Msg.append((char*)ErrBuf->GetData(), ErrBuf->GetNumBytes());

						if (Msg.empty())
							throw DirectXError(hr, "Nie mo¿na wczytaæ (ID3DXEffectCompiler::CompileEffect) efektu silnika z pliku \""+string(pimpl->m_SourceFileName)+"\"", __FILE__, __LINE__);
						else
							throw DirectXError(hr, "Nie mo¿na wczytaæ (ID3DXEffectCompiler::CompileEffect) efektu silnika z pliku \""+string(pimpl->m_SourceFileName)+"\": "+Msg, __FILE__, __LINE__);
					}
					EffectBuf.reset(new D3dxBufferWrapper(EffectBufPtr));
				}

				// Zapisz skompilowany efekt do pliku cache
				SaveDataToFile(CacheFileName, EffectBuf->GetData(), EffectBuf->GetNumBytes());

				// Wczytaj skompilowany efekt jako efekt
				ID3DXEffect *Effect;
				{
					ID3DXBuffer *ErrBufPtr;
					HRESULT hr = D3DXCreateEffect(
						frame::Dev,
						EffectBuf->GetData(),
						EffectBuf->GetNumBytes(),
						NULL,
						NULL,
						D3DXFX_DONOTSAVESTATE,
						NULL,
						&Effect,
						&ErrBufPtr);
					// Ale hack! Tutaj przyda³by siê inteligentny wskaŸnik robi¹cy w destruktorze Release.
					scoped_ptr<D3dxBufferWrapper> ErrBuf;
					if (ErrBufPtr)
						ErrBuf.reset(new D3dxBufferWrapper(ErrBufPtr));
					if (FAILED(hr))
					{
						string Msg;
						if (ErrBuf != NULL)
							Msg.append((char*)ErrBuf->GetData(), ErrBuf->GetNumBytes());

						if (Msg.empty())
							throw DirectXError(hr, "Nie mo¿na wczytaæ (D3DXCreateEffect) efektu silnika z pamiêci", __FILE__, __LINE__);
						else
							throw DirectXError(hr, "Nie mo¿na wczytaæ (D3DXCreateEffect) efektu silnika z pamiêci: "+Msg, __FILE__, __LINE__);
					}
				}

				sit = pimpl->m_Shaders.insert(std::make_pair(Hash, SHADER_INFO(Effect, pimpl->m_ParamCount, pimpl->m_ParamNames))).first;
			}
		}

		// Zwróæ bie¿¹cy shader
		return sit->second;
	}
	catch(Error &e)
	{
		string MacrosStr; MacrosToStr(&MacrosStr, Macros);
		e.Push("Nie mo¿na pobraæ shadera \"" + GetName() + "\" dla ustawieñ: " + MacrosStr, __FILE__, __LINE__);
		throw;
	}
}

void Multishader::OnLoad()
{
	D3dResource::OnLoad();
}

void Multishader::OnUnload()
{
	pimpl->FreeEffects();
	pimpl->m_ShaderSource.reset();

	D3dResource::OnUnload();
}

void Multishader::OnDeviceCreate()
{
}

void Multishader::OnDeviceDestroy()
{
}

void Multishader::OnDeviceRestore()
{
}

void Multishader::OnDeviceInvalidate()
{
	pimpl->FreeEffects();
}


} // namespace res
