/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include <cstring> // dla strncmp
#include "Framework.hpp"
#include "Res_d3d.hpp"
#include "D3DUtils.hpp"

namespace res
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// STA£E

// Czy po usuniêciu / utracie urz¹dzenia zasoby, które uleg³y utraceniu i mog¹
// zostaæ od³adowane maj¹ siê ponownie za³adowaæ mimo ¿e nie musz¹?
bool RELOAD = false;

const char * const FONT_SYNTAX_ERROR_MSG = "B³¹d sk³adni";
const uint4 FONT_VERTEX_FVF = D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// AbstractManaged

D3dResource::D3dResource(const string &Name, const string &Group) :
	IResource(Name, Group),
	m_Created(false),
	m_Restored(false)
{
}

D3dResource::~D3dResource()
{
}

void D3dResource::OnLoad()
{
	assert(frame::Dev != NULL);

	if (!frame::GetDeviceLost())
	{
		OnDeviceCreate();
		m_Created = true;
		OnDeviceRestore();
		m_Restored = true;
	}
}

void D3dResource::OnUnload()
{
	if (m_Restored)
	{
		m_Restored = false;
		OnDeviceInvalidate();
	}
	if (m_Created)
	{
		m_Created = false;
		OnDeviceDestroy();
	}
}

void D3dResource::OnEvent(uint4 Type, void *Params)
{
	switch (Type)
	{
	case EVENT_D3D_RESET_DEVICE:
		if (GetState() != ST_UNLOADED)
		{
			if (!m_Created)
			{
				OnDeviceCreate();
				m_Created = true;
			}
			if (!m_Restored)
			{
				OnDeviceRestore();
				m_Restored = true;
			}
		}
		break;

	case EVENT_D3D_LOST_DEVICE:
		if (m_Restored && GetState() != ST_UNLOADED)
		{
			m_Restored = false;
			OnDeviceInvalidate();
		}
		break;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// D3dTexture

void D3dTexture::OnDeviceCreate()
{
	m_Texture = 0;
	SetBaseTexture(0);

	LOG(LOG_RESMNGR, Format("D3dTexture: Loading \"#\" from: #") % GetName() % GetFileName());

	HRESULT hr = D3DXCreateTextureFromFileEx(
		frame::Dev, // Device
		GetFileName().c_str(), // SrcFile
		D3DX_DEFAULT, D3DX_DEFAULT, // Width, Height
		GetDisableMipMapping() ? 1 : D3DX_DEFAULT, // MipLevels
		0, // Usage
		D3DFMT_UNKNOWN, // Format
		D3DPOOL_MANAGED, // Pool
		D3DX_DEFAULT, // Filter
		D3DX_DEFAULT, // MipFilter
		math_cast<D3DCOLOR>(m_KeyColor), // ColorKey
		0, // SrcInfo
		0, // Palette
		&m_Texture); // Texture
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na wczytaæ tekstury z pliku: " + GetFileName(), __FILE__, __LINE__);
	SetBaseTexture(m_Texture);

	D3DSURFACE_DESC ld;
	m_Texture->GetLevelDesc(0, &ld);
	m_Width = ld.Width;
	m_Height = ld.Height;
}

void D3dTexture::OnDeviceDestroy()
{
	SAFE_RELEASE(m_Texture);
	SetBaseTexture(0);
}

void D3dTexture::OnDeviceRestore()
{
}

void D3dTexture::OnDeviceInvalidate()
{
}

D3dTexture::~D3dTexture()
{
	SAFE_RELEASE(m_Texture);
}

IResource * D3dTexture::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	// Nazwa pliku
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	string FileName = Params.GetString();
	Params.Next();

	// Kolor kluczowy
	COLOR KeyColor = COLOR(0x00000000u);
	if (Params.GetToken() == Tokenizer::TOKEN_IDENTIFIER && Params.GetString() == "KeyColor")
	{
		Params.Next();

		Params.AssertSymbol('=');
		Params.Next();

		Params.AssertToken(Tokenizer::TOKEN_INTEGER, Tokenizer::TOKEN_STRING);
		if (Params.GetToken() == Tokenizer::TOKEN_INTEGER)
			KeyColor.ARGB = Params.MustGetUint4();
		else
			StrToColor(&KeyColor, Params.GetString());
		KeyColor.ARGB |= 0xFF000000;
		Params.Next();
	}

	bool DisableMipMapping = false;
	if (Params.QueryIdentifier("DisableMipMapping"))
	{
		Params.Next();
		DisableMipMapping = true;
	}

	// Œrednik
	Params.AssertSymbol(';');
	Params.Next();

	return new D3dTexture(Name, Group, FileName, KeyColor, DisableMipMapping);
}

D3dTexture::D3dTexture(const string &Name, const string &Group, const string &FileName, COLOR KeyColor, bool DisableMipMapping) :
	D3dBaseTexture(Name, Group, FileName, DisableMipMapping),
	m_KeyColor(KeyColor),
	m_Texture(0),
	m_Width(0), m_Height(0)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// D3dCubeTexture

void D3dCubeTexture::OnDeviceCreate()
{
	m_CubeTexture = 0;
	SetBaseTexture(0);

	LOG(LOG_RESMNGR, Format("D3dCubeTexture: Loading \"#\" from: #") % GetName() % GetFileName());

	HRESULT hr = D3DXCreateCubeTextureFromFile(
		frame::Dev,
		GetFileName().c_str(),
		&m_CubeTexture);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na wczytaæ tekstury szeœciennej z pliku: " + GetFileName(), __FILE__, __LINE__);
	SetBaseTexture(m_CubeTexture);

	D3DSURFACE_DESC ld;
	m_CubeTexture->GetLevelDesc(0, &ld);
	m_Size = ld.Width;
}

void D3dCubeTexture::OnDeviceDestroy()
{
	SAFE_RELEASE(m_CubeTexture);
	SetBaseTexture(0);
}

void D3dCubeTexture::OnDeviceRestore()
{
}

void D3dCubeTexture::OnDeviceInvalidate()
{
}

D3dCubeTexture::~D3dCubeTexture()
{
	SAFE_RELEASE(m_CubeTexture);
}

IResource * D3dCubeTexture::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	string FileName = Params.GetString();
	Params.Next();

	Params.AssertSymbol(';');
	Params.Next();

	return new D3dCubeTexture(Name, Group, FileName);
}

D3dCubeTexture::D3dCubeTexture(const string &Name, const string &Group, const string &FileName) :
	D3dBaseTexture(Name, Group, FileName),
	m_CubeTexture(0),
	m_Size(0)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// D3dFont

void D3dFont::OnDeviceCreate()
{
	LOG(LOG_RESMNGR, Format("D3dFont: Creating \"#\"") % GetName());

	HRESULT hr = D3DXCreateFont(frame::Dev, -frame::UnitsToPixelsY((float)m_Size),
		0, (m_Bold ? FW_BOLD : FW_NORMAL), 1, m_Italic,
		EASTEUROPE_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		FF_DONTCARE, m_FaceName.c_str(), &m_Font);
	if (FAILED(hr))
		throw DirectXError(hr, Format("Nie mo¿na utworzyæ czcionki. Nazwa=#, Rozmiar=#") % m_FaceName % m_Size);
}

void D3dFont::OnDeviceDestroy()
{
	SAFE_RELEASE(m_Font);
}

void D3dFont::OnDeviceRestore()
{
	if (m_Font)
		m_Font->OnResetDevice();
}

void D3dFont::OnDeviceInvalidate()
{
	if (m_Font)
		m_Font->OnLostDevice();
}

IResource * D3dFont::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	// Nazwa
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	string FaceName = Params.GetString();
	Params.Next();

	// Rozmiar
	Params.AssertToken(Tokenizer::TOKEN_INTEGER);
	int Size = Params.MustGetInt4();
	Params.Next();

	// Bold, Italic
	bool Bold = false, Italic = false;
	while (Params.GetToken() == Tokenizer::TOKEN_IDENTIFIER)
	{
		if (Params.GetString() == "bold")
		{
			Bold = true;
			Params.Next();
		}
		else if (Params.GetString() == "italic")
		{
			Italic = true;
			Params.Next();
		}
		else
			break;
	}

	// Œrednik
	Params.AssertSymbol(';');
	Params.Next();

	return new D3dFont(Name, Group, FaceName, Size, Bold, Italic);
}

D3dFont::~D3dFont()
{
	SAFE_RELEASE(m_Font);
}

D3dFont::D3dFont(const string &Name, const string &Group, const string &FaceName, int Size, bool Bold, bool Italic) :
	D3dResource(Name, Group),
	m_FaceName(FaceName),
	m_Size(Size),
	m_Italic(Italic),
	m_Bold(Bold),
	m_Font(0)
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// D3dEffect

void D3dEffect::OnDeviceCreate()
{
	LOG(LOG_RESMNGR, Format("D3dEffect: Loading \"#\" from: #") % GetName() % GetFileName());

	ID3DXBuffer *ErrBufPtr;
	HRESULT hr = D3DXCreateEffectFromFile(frame::Dev, GetFileName().c_str(), 0, 0, D3DXFX_DONOTSAVESTATE, 0, &m_Effect, &ErrBufPtr);
	scoped_ptr<ID3DXBuffer, ReleasePolicy> ErrBuf(ErrBufPtr);
	if (FAILED(hr))
	{
		string Msg;
		if (ErrBuf != NULL)
			Msg.append((char*)ErrBuf->GetBufferPointer(), ErrBuf->GetBufferSize());

		if (Msg.empty())
			throw DirectXError(hr, "Nie mo¿na wczytaæ efektu \""+GetName()+"\" z pliku \""+GetFileName()+"\"", __FILE__, __LINE__);
		else
			throw DirectXError(hr, "Nie mo¿na wczytaæ efektu \""+GetName()+"\" z pliku \""+GetFileName()+"\": "+Msg, __FILE__, __LINE__);
	}

	LoadParamHandles();
}

void D3dEffect::OnDeviceDestroy()
{
	ClearParamHandles();
	SAFE_RELEASE(m_Effect);
}

void D3dEffect::OnDeviceRestore()
{
	if (m_Effect)
		m_Effect->OnResetDevice();
}

void D3dEffect::OnDeviceInvalidate()
{
	if (m_Effect)
		m_Effect->OnLostDevice();
}

D3dEffect::~D3dEffect()
{
	SAFE_RELEASE(m_Effect);
}

IResource * D3dEffect::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	// Nazwa pliku
	Params.AssertToken(Tokenizer::TOKEN_STRING);
	string FileName = Params.GetString();
	Params.Next();

	// Opcjonalne parametry
	STRING_VECTOR ParamNames;
	if (Params.GetToken() == Tokenizer::TOKEN_IDENTIFIER && Params.GetString() == "params")
	{
		Params.Next();

		Params.AssertSymbol('{');
		Params.Next();

		while (!Params.QuerySymbol('}'))
		{
			Params.AssertToken(Tokenizer::TOKEN_STRING);
			ParamNames.push_back(Params.GetString());
			Params.Next();

			if (!Params.QuerySymbol('}'))
			{
				Params.AssertSymbol(',');
				Params.Next();
			}
		}
		Params.Next();
	}

	// Œrednik
	Params.AssertSymbol(';');
	Params.Next();

	return new D3dEffect(Name, Group, FileName, &ParamNames);
}

void D3dEffect::ClearParamHandles()
{
	if (!m_ParamHandles.empty())
		ZeroMemory(&m_ParamHandles[0], m_ParamHandles.size()*sizeof(D3DXHANDLE));
}

void D3dEffect::LoadParamHandles()
{
	for (size_t i = 0; i < m_ParamHandles.size(); i++)
	{
		m_ParamHandles[i] = Get()->GetParameterByName(NULL, m_ParamNames[i].c_str());
		if (m_ParamHandles[i] == NULL)
			throw Error("D3dEffect::LoadParamHandles: Nie mo¿na pobraæ parametru efektu \"" + m_ParamNames[i] + "\".", __FILE__, __LINE__);
	}
}

D3dEffect::D3dEffect(const string &Name, const string &Group, const string &FileName, const STRING_VECTOR *ParamNames) :
	D3dResource(Name, Group),
	m_FileName(FileName),
	m_ParamNames(*ParamNames),
	m_Effect(0)
{
	m_ParamHandles.resize(m_ParamNames.size());
	ClearParamHandles();
}

void D3dEffect::SetParamNames(const STRING_VECTOR *ParamNames)
{
	if (ParamNames == NULL)
	{
		m_ParamHandles.clear();
		m_ParamNames.clear();
	}
	else
	{
		m_ParamNames = *ParamNames;
		m_ParamHandles.resize(m_ParamNames.size());
		if (Get())
			LoadParamHandles();
		else
			ClearParamHandles();
	}
}

void D3dEffect::Begin(bool SaveState)
{
	ID3DXEffect *e = Get();
	assert(e != NULL);

	UINT Passes;
	e->Begin(&Passes, SaveState ? 0 : D3DXFX_DONOTSAVESTATE);
	e->BeginPass(0);
}

void D3dEffect::End()
{
	ID3DXEffect *e = Get();
	assert(e != NULL);

	e->EndPass();
	e->End();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa FontDefParser

class FontDefParser
{
public:
	enum TOKEN_TYPE
	{
		TT_IDENTIFIER,
		TT_INTEGER,
		TT_STRING,
		TT_SYM_EQ,
		TT_SYM_COMMA,
		TT_EOL,
		TT_EOF
	};
	
	struct TOKEN
	{
		TOKEN_TYPE Type;
		string s;
		int i;
	};
	
private:
	string *m_Doc;
	size_t m_Index;
	TOKEN m_Token;
	
	bool IsEnd() { return m_Index == m_Doc->length(); }
	char NextChar() { return (*m_Doc)[m_Index]; }
	
public:
	FontDefParser(string *Doc) : m_Doc(Doc), m_Index(0) { }
	void NextToken();
	TOKEN & GetToken() { return m_Token; }
};

void FontDefParser::NextToken()
{
	// Bia³e znaki
	while (!IsEnd() && (NextChar() == ' ' || NextChar() == '\t'))
		m_Index++;

	// Koniec
	if (IsEnd())
	{
		m_Token.Type = TT_EOF;
		return;
	}
	
	// Nie koniec - patrzymy co jest grane
	
	// Koniec wiersza \n
	if (NextChar() == '\n')
	{
		m_Token.Type = TT_EOL;
		m_Index++;
	}
	// Koniec wiersza \r
	else if (NextChar() == '\r')
	{
		m_Token.Type = TT_EOL;
		m_Index++;
		// Pomiñ te¿ \n
		if (!IsEnd() && NextChar() == '\n')
			m_Index++;
	}
	// String
	else if (NextChar() == '"')
	{
		m_Token.Type = TT_STRING;
		m_Token.s.clear();
		m_Index++;
		for (;;)
		{
			if (IsEnd())
				throw Error("Nieoczekiwany koniec pliku wewn¹trz ³añcucha");
			else if (NextChar() == '\r' || NextChar() == '\n')
				throw Error("Nieoczekiwany koniec wiersza wewn¹trz ³añcucha");
			else if (NextChar() == '"')
			{
				m_Index++;
				break;
			}
			else
			{
				m_Token.s.append(CharToStrR(NextChar()));
				m_Index++;
			}
		}
	}
	// '='
	else if (NextChar() == '=')
	{
		m_Token.Type = TT_SYM_EQ;
		m_Index++;
	}
	// ','
	else if (NextChar() == ',')
	{
		m_Token.Type = TT_SYM_COMMA;
		m_Index++;
	}
	// integer
	else if (NextChar() >= '0' && NextChar() <= '9' || NextChar() == '-')
	{
		string s = CharToStrR(NextChar());
		m_Index++;
		
		while (!IsEnd() && NextChar() >= '0' && NextChar() <= '9')
		{
			s.append(CharToStrR(NextChar()));
			m_Index++;
		}
		
		m_Token.Type = TT_INTEGER;
		if (StrToInt(&m_Token.i, s) != 0)
			throw Error("B³êdna wartoœæ ca³kowita: " + s);
	}
	// identifier
	else if (
		(NextChar() >= 'a' && NextChar() <= 'z') ||
		(NextChar() >= 'A' && NextChar() <= 'Z') ||
		(NextChar() == '_'))
	{
		m_Token.Type = TT_IDENTIFIER;
		m_Token.s = CharToStrR(NextChar());
		m_Index++;

		while (!IsEnd() && (
			(NextChar() >= 'a' && NextChar() <= 'z') ||
			(NextChar() >= 'A' && NextChar() <= 'Z') ||
			(NextChar() == '_') ||
			(NextChar() >= '0' && NextChar() <= '9')))
		{
			m_Token.s.append(CharToStrR(NextChar()));
			m_Index++;
		}
	}
	else
		throw Error("Nieznany symbol: " + CharToStrR(NextChar()));
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Font

IDirect3DVertexBuffer9 *Font::m_VB = 0;
IDirect3DIndexBuffer9 *Font::m_IB = 0;
int Font::m_BufferRefCount = 0;


void Font::LoadDef()
{
	ERR_TRY;
	
	bool CharExists[256];
	int i;
	for (i = 0; i < 256; i++)
		CharExists[i] = false;
	
	string Doc;
	LoadStringFromFile(m_DefFileName, &Doc);
	
	FontDefParser parser(&Doc);
	
	float ScaleW, ScaleH, LineHeight;
	m_AvgCharWidth = 0.0f;
	int AvgCharWidth_Counter = 0;
	
	for (;;)
	{
		parser.NextToken();
		FontDefParser::TOKEN & Token = parser.GetToken();
		
		if (Token.Type == FontDefParser::TT_EOF)
			break;
		else if (Token.Type == FontDefParser::TT_IDENTIFIER)
		{
			if (Token.s == "scaleW")
			{
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_SYM_EQ) throw Error(FONT_SYNTAX_ERROR_MSG);
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_INTEGER) throw Error(FONT_SYNTAX_ERROR_MSG);
				ScaleW = 1.0f / static_cast<float>(parser.GetToken().i);
			}
			else if (Token.s == "scaleH")
			{
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_SYM_EQ) throw Error(FONT_SYNTAX_ERROR_MSG);
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_INTEGER) throw Error(FONT_SYNTAX_ERROR_MSG);
				ScaleH = 1.0f / static_cast<float>(parser.GetToken().i);
			}
			else if (Token.s == "lineHeight")
			{
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_SYM_EQ) throw Error(FONT_SYNTAX_ERROR_MSG);
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_INTEGER) throw Error(FONT_SYNTAX_ERROR_MSG);
				LineHeight = static_cast<float>(parser.GetToken().i);
			}
			else if (Token.s == "page")
			{
				// id
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_IDENTIFIER) throw Error(FONT_SYNTAX_ERROR_MSG);
				if (parser.GetToken().s != "id") throw Error(FONT_SYNTAX_ERROR_MSG);
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_SYM_EQ) throw Error(FONT_SYNTAX_ERROR_MSG);
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_INTEGER) throw Error(FONT_SYNTAX_ERROR_MSG);
				if (parser.GetToken().i != 0)
					throw Error("Czcionki u¿ywaj¹ce wiêcej ni¿ jednej tekstury nie s¹ obs³ugiwane.");
				// file
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_IDENTIFIER) throw Error(FONT_SYNTAX_ERROR_MSG);
				if (parser.GetToken().s != "file") throw Error(FONT_SYNTAX_ERROR_MSG);
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_SYM_EQ) throw Error(FONT_SYNTAX_ERROR_MSG);
				parser.NextToken();
				if (parser.GetToken().Type != FontDefParser::TT_STRING) throw Error(FONT_SYNTAX_ERROR_MSG);
				if (m_TextureFileName.empty())
				{
					string Path; ExtractFilePath(&Path, m_DefFileName);
					m_TextureFileName = Path + parser.GetToken().s;
				}
				parser.NextToken();
				// EOL
				if (parser.GetToken().Type != FontDefParser::TT_EOL) throw Error(FONT_SYNTAX_ERROR_MSG);
			}
			else if (Token.s == "char")
			{
				int ReadId, ReadX, ReadY, ReadWidth, ReadHeight, ReadOffsetX, ReadOffsetY, ReadAdvanceX, ReadPage, ReadChnl;
				for (;;)
				{
					parser.NextToken();
					if (parser.GetToken().Type == FontDefParser::TT_EOL)
						break;
					if (parser.GetToken().Type != FontDefParser::TT_IDENTIFIER) throw Error(FONT_SYNTAX_ERROR_MSG);
					string What = parser.GetToken().s;
					parser.NextToken();
					if (parser.GetToken().Type != FontDefParser::TT_SYM_EQ) throw Error(FONT_SYNTAX_ERROR_MSG);
					parser.NextToken();
					if (parser.GetToken().Type != FontDefParser::TT_INTEGER) throw Error(FONT_SYNTAX_ERROR_MSG);
					if (What == "id")
						ReadId = parser.GetToken().i;
					else if (What == "x")
						ReadX = parser.GetToken().i;
					else if (What == "y")
						ReadY = parser.GetToken().i;
					else if (What == "width")
						ReadWidth = parser.GetToken().i;
					else if (What == "height")
						ReadHeight = parser.GetToken().i;
					else if (What == "xoffset")
						ReadOffsetX = parser.GetToken().i;
					else if (What == "yoffset")
						ReadOffsetY = parser.GetToken().i;
					else if (What == "xadvance")
						ReadAdvanceX = parser.GetToken().i;
					else if (What == "page")
						ReadPage = parser.GetToken().i;
					else if (What == "chnl")
						ReadChnl = parser.GetToken().i;
					else
						throw Error(FONT_SYNTAX_ERROR_MSG);
				}
				
				if (ReadId < 0 || ReadId >= 256)
					throw Error("B³êdny id");
				
				CharExists[ReadId] = true;
				m_CharInfo[ReadId].tx1 = (ReadX-0.5f) * ScaleW;
				m_CharInfo[ReadId].ty1 = (ReadY-0.5f) * ScaleH;
				// Tutaj mam w¹tpliwoœci jak to ma byæ - -0.5, +0.5 czy jak?
				m_CharInfo[ReadId].tx2 = (ReadX+ReadWidth+0.5f) * ScaleW;
				m_CharInfo[ReadId].ty2 = (ReadY+ReadHeight+0.5f) * ScaleH;
				m_CharInfo[ReadId].Advance = ReadAdvanceX / LineHeight;
				m_CharInfo[ReadId].OffsetX = ReadOffsetX / LineHeight;
				m_CharInfo[ReadId].OffsetY = ReadOffsetY / LineHeight;
				m_CharInfo[ReadId].Width = ReadWidth / LineHeight;
				m_CharInfo[ReadId].Height = ReadHeight / LineHeight;
				
				m_AvgCharWidth += m_CharInfo[ReadId].Advance;
				AvgCharWidth_Counter++;
				//LOG(LOG_RESMNGR, Format("Char # tx1=# ty1=# tx2=# ty2=# Advance=# OffsetX=# OffsetY=# Width=# Height=#") % ReadId % m_CharInfo[ReadId].tx1 % m_CharInfo[ReadId].ty1 % m_CharInfo[ReadId].tx2 % m_CharInfo[ReadId].ty2 % m_CharInfo[ReadId].Advance % m_CharInfo[ReadId].OffsetX % m_CharInfo[ReadId].OffsetY % m_CharInfo[ReadId].Width % m_CharInfo[ReadId].Height);
			}
		}
	}
	
	// Znaki, których nie ma, zast¹p znakami zapytania
	if (!CharExists[(int)(uint1)'?'])
		throw Error("Nie ma znaku '?'");
	for (i = 0; i < 256; i++)
		if (!CharExists[i])
			m_CharInfo[i] = m_CharInfo[(int)(uint1)'?'];

	// Œrednia szerokoœæ znaku
	m_AvgCharWidth /= AvgCharWidth_Counter;

	// Wspó³rzêdne tekstury do kreœleñ
	{
		CHAR_INFO & CharInfo = m_CharInfo[(int)(uint1)'-'];
		m_LineTx1 = CharInfo.tx1 + 1.0f * ScaleW;
		m_LineTy1 = CharInfo.ty1 + 1.0f * ScaleH;
		m_LineTx2 = CharInfo.tx2 - 1.0f * ScaleW;
		m_LineTy2 = CharInfo.ty2 - 1.0f * ScaleH;
	}
	
	ERR_CATCH("Nie mo¿na wczytaæ definicji czcionki z pliku: " + string(m_DefFileName));
}

IResource * Font::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	string DefFileName, TextureFileName;

	Params.AssertToken(Tokenizer::TOKEN_STRING);
	DefFileName = Params.GetString();
	Params.Next();

	Params.AssertSymbol(',');
	Params.Next();

	Params.AssertToken(Tokenizer::TOKEN_STRING);
	TextureFileName = Params.GetString();
	Params.Next();

	Params.AssertSymbol(';');
	Params.Next();

	return new Font(Name, Group, DefFileName, TextureFileName);
}

void Font::OnDeviceCreate()
{
	m_Texture = 0;

	HRESULT hr = D3DXCreateTextureFromFile(frame::Dev, m_TextureFileName.c_str(), &m_Texture);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na wczytaæ tekstury czcionki z pliku: " + m_TextureFileName, __FILE__, __LINE__);
}

void Font::OnDeviceDestroy()
{
	// Na wszelki wypadek
	UnacquireBuffer();

	SAFE_RELEASE(m_Texture);
}

void Font::OnDeviceRestore()
{
	m_BufferPos = 0;
	m_VbIndex = 0;
	m_IbIndex = 0;
}

void Font::OnDeviceInvalidate()
{
	UnacquireBuffer();

	m_BufferPos = 0;
	m_VbIndex = 0;
	m_IbIndex = 0;
}

Font::~Font()
{
	UnacquireBuffer();

	SAFE_RELEASE(m_Texture);
}

Font::Font(const string &Name, const string &Group, const string &DefFileName, const string &TextureFileName) :
	D3dResource(Name, Group),
	m_DefFileName(DefFileName),
	m_TextureFileName(TextureFileName),
	m_Texture(0),
	m_BufferPos(0),
	m_VbIndex(0), m_IbIndex(0),
	m_LockedVB(0), m_LockedIB(0),
	m_BufferAcquired(false)
{
	LOG(LOG_RESMNGR, Format("Font: Creating \"#\"") % GetName());
	LoadDef();
}

float Font::GetTextWidth(const string &Text, size_t TextBegin, size_t TextEnd, float Size)
{
	float R = 0.0f;

	for (size_t i = TextBegin; i < TextEnd; i++)
		R += m_CharInfo[(int)(uint1)Text[i]].Advance * Size;

	return R;
}

bool Font::LineSplit(size_t *OutBegin, size_t *OutEnd, float *OutWidth, size_t *InOutIndex, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width)
{
	// Heh, piszê ten algorytm chyba trzeci raz w ¿yciu.
	// Za ka¿dym razem idzie mi szybciej i lepiej.
	// Ale zawsze i tak jest dla mnie ogromnie trudny i skomplikowany.

	if (*InOutIndex >= TextEnd)
		return false;

	*OutBegin = *InOutIndex;
	*OutWidth = 0.0f;

	// Pojedyncza linia - specjalny szybki tryb
	if (Flags & FLAG_WRAP_SINGLELINE)
	{
		while (*InOutIndex < TextEnd)
			*OutWidth += m_CharInfo[(int)(uint1)Text[(*InOutIndex)++]].Advance;
		*OutEnd = *InOutIndex;
		*OutWidth *= Size;

		return true;
	}

	char Ch;
	float CharWidth;
	// Zapamiêtany stan miejsca ostatniego wyst¹pienia spacji
	// Przyda siê na wypadek zawijania wierszy na granicy s³owa.
	size_t LastSpaceIndex = string::npos;
	float WidthWhenLastSpace;

	for (;;)
	{
		// Koniec tekstu
		if (*InOutIndex >= TextEnd)
		{
			*OutEnd = TextEnd;
			break;
		}

		// Pobierz znak
		Ch = Text[*InOutIndex];

		// Koniec wiersza
		if (Ch == '\n')
		{
			*OutEnd = *InOutIndex;
			(*InOutIndex)++;
			break;
		}
		// Koniec wiersza \r
		else if (Ch == '\r')
		{
			*OutEnd = *InOutIndex;
			(*InOutIndex)++;
			// Sekwencja \r\n - pomiñ \n
			if (*InOutIndex < TextEnd && Text[*InOutIndex] == '\n')
				(*InOutIndex)++;
			break;
		}
		// Inny znak
		else
		{
			// Szerokoœæ znaku
			CharWidth = GetCharWidth_(Text[*InOutIndex], Size);

			// Jeœli nie ma automatycznego zawijania wierszy lub
			// jeœli siê zmieœci lub
			// to jest pierwszy znak (zabezpieczenie przed nieskoñczonym zapêtleniem dla Width < szerokoœæ pierwszego znaku) -
			// dolicz go i ju¿
			if (Flags & FLAG_WRAP_NORMAL || *OutWidth + CharWidth <= Width || *InOutIndex == *OutBegin)
			{
				// Jeœli to spacja - zapamiêtaj dane
				if (Ch == ' ')
				{
					LastSpaceIndex = *InOutIndex;
					WidthWhenLastSpace = *OutWidth;
				}
				*OutWidth += CharWidth;
				(*InOutIndex)++;
			}
			// Jest automatyczne zawijanie wierszy i siê nie mieœci
			else
			{
				// Niemieszcz¹cy siê znak to spacja
				if (Ch == ' ')
				{
					*OutEnd = *InOutIndex;
					// Mo¿na go przeskoczyæ
					(*InOutIndex)++;
					break;
				}
				// Poprzedni znak za tym to spacja
				else if (*InOutIndex > *OutBegin && Text[(*InOutIndex)-1] == ' ')
				{
					// Koniec bêdzie na tej spacji
					*OutEnd = LastSpaceIndex;
					*OutWidth = WidthWhenLastSpace;
					break;
				}

				// Zawijanie wierszy na granicy s³owa
				if (Flags & FLAG_WRAP_WORD)
				{
					// By³a jakaœ spacja
					if (LastSpaceIndex != string::npos)
					{
						// Koniec bêdzie na tej spacji
						*OutEnd = LastSpaceIndex;
						*InOutIndex = LastSpaceIndex+1;
						*OutWidth = WidthWhenLastSpace;
						break;
					}
					// Nie by³o spacji - trudno, zawinie siê jak na granicy znaku
				}

				*OutEnd = *InOutIndex;
				break;
			}
		}
	}

	return true;
}

void Font::GetTextExtent(float *OutWidth, float *OutHeight, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width)
{
	size_t OBegin, OEnd, IOIndex = TextBegin;
	float OWidth;

	int LineCount = 0;
	*OutWidth = 0.0f;

	while (LineSplit(&OBegin, &OEnd, &OWidth, &IOIndex, Text, TextBegin, TextEnd, Size, Flags, Width))
	{
		LineCount++;
		if (OWidth > *OutWidth)
			*OutWidth = OWidth;
	}

	*OutHeight = LineCount * Size;
}

uint4 Font::GetQuadCountSL(const string &Text, size_t TextBegin, size_t TextEnd, uint4 Flags)
{
	uint4 R = 0;

	for (size_t i = TextBegin; i < TextEnd; i++)
	{
		if (Text[i] != ' ')
			R++;
	}

	if (Flags & FLAG_DOUBLE)
		R += 2;
	else if (Flags & FLAG_UNDERLINE)
		R++;
	if (Flags & FLAG_OVERLINE)
		R++;
	if (Flags & FLAG_STRIKEOUT)
		R++;

	return R;
}

uint4 Font::GetQuadCount(const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width)
{
	uint4 R = 0;
	size_t i, OBegin, OEnd, IOIndex = TextBegin;
	float OWidth;
	int LineCount = 0;

	while (LineSplit(&OBegin, &OEnd, &OWidth, &IOIndex, Text, TextBegin, TextEnd, Size, Flags, Width))
	{
		for (i = OBegin; i < OEnd; i++)
		{
			if (Text[i] != ' ')
				R++;
		}
		LineCount++;
	}

	if (Flags & FLAG_DOUBLE)
		R += 2 * LineCount;
	else if (Flags & FLAG_UNDERLINE)
		R += LineCount;
	if (Flags & FLAG_OVERLINE)
		R += LineCount;
	if (Flags & FLAG_STRIKEOUT)
		R += LineCount;

	return R;
}

bool Font::HitTestSL(size_t *OutIndex, float *OutPercent, float PosX, float HitX, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags)
{
	assert(Flags & FLAG_HLEFT || Flags & FLAG_HRIGHT || Flags & FLAG_HCENTER);

	// Do lewej
	if (Flags & FLAG_HLEFT)
	{
		float CharWidth, CurrentX = PosX, NewX;
		// Po lewej
		if (HitX < CurrentX)
			return false;
		// PrzejdŸ po znakach
		for (size_t i = TextBegin; i < TextEnd; i++)
		{
			// Szerokoœæ tego znaku
			CharWidth = GetCharWidth_(Text[i], Size);
			NewX = CurrentX + CharWidth;
			// Znaleziono
			if (HitX < NewX)
			{
				*OutIndex = i;
				*OutPercent = (HitX-CurrentX) / CharWidth;
				return true;
			}
			CurrentX = NewX;
		}
		// Nie znaleziono
		return false;
	}
	// Do prawej
	else if (Flags & FLAG_HRIGHT)
	{
		float CharWidth, CurrentX = PosX, NewX;
		// Po prawej
		if (HitX > CurrentX)
			return false;
		// PrzejdŸ po znakach
		for (size_t i = TextEnd; i-- > TextBegin; )
		{
			// Szerokoœæ tego znaku
			CharWidth = GetCharWidth_(Text[i], Size);
			NewX = CurrentX - CharWidth;
			// Znaleziono
			if (HitX > NewX)
			{
				*OutIndex = i;
				*OutPercent = (HitX-NewX) / CharWidth;
				return true;
			}
			CurrentX = NewX;
		}
		// Nie znaleziono
		return false;
	}
	// Do œrodka
	else // Flags & FLAG_HCENTER
	{
		// Oblicz szerokoœæ ca³ego wiersza
		float LineWidth = 0.0f;
		for (size_t j = TextBegin; j < TextEnd; j++)
			LineWidth += GetCharWidth_(Text[j], Size);
		// Oblicz pozycjê pocz¹tku
		PosX -= LineWidth * 0.5f;
		// Dalej to ju¿ jak przy dosuniêciu do lewej:

		float CharWidth, CurrentX = PosX, NewX;
		// Po lewej
		if (HitX < CurrentX)
			return false;
		// PrzejdŸ po znakach
		for (size_t i = TextBegin; i < TextEnd; i++)
		{
			// Szerokoœæ tego znaku
			CharWidth = GetCharWidth_(Text[i], Size);
			NewX = CurrentX + CharWidth;
			// Znaleziono
			if (HitX < NewX)
			{
				*OutIndex = i;
				*OutPercent = (HitX-CurrentX) / CharWidth;
				return true;
			}
			CurrentX = NewX;
		}
		// Nie znaleziono
		return false;
	}
}

bool Font::HitTest(size_t *OutIndex, float *OutPercentX, float *OutPercentY, float PosX, float PosY, float HitX, float HitY, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width)
{
	assert(Flags & FLAG_VTOP || Flags & FLAG_VBOTTOM || Flags & FLAG_VMIDDLE);

	// Do góry
	if (Flags & FLAG_VTOP)
	{
		size_t OBegin, OEnd, IOIndex = TextBegin;
		float OWidth;
		float CurrentY = PosY, NewY;
		// Na górze
		if (HitY < CurrentY)
			return false;
		// PrzejdŸ po liniach
		while (LineSplit(&OBegin, &OEnd, &OWidth, &IOIndex, Text, TextBegin, TextEnd, Size, Flags, Width))
		{
			NewY = CurrentY + Size;
			// Znaleziono
			if (HitY < NewY)
			{
				// SprawdŸ na X
				if (HitTestSL(OutIndex, OutPercentX, PosX, HitX, Text, OBegin, OEnd, Size, Flags))
				{
					*OutPercentY = (HitY-CurrentY) / Size;
					return true;
				}
				else
					return false;
			}
			CurrentY = NewY;
		}
		// Nie znaleziono
		return false;
	}
	// Do do³u lub do œrodka
	else
	{
		// Podziel na wiersze, spamiêtaj, dowiedz siê o liczbê wierszy
		size_t LineCount = 0;
		std::vector<size_t> Begins;
		std::vector<size_t> Ends;
		size_t OBegin, OEnd, IOIndex = TextBegin;
		float OWidth;
		while (LineSplit(&OBegin, &OEnd, &OWidth, &IOIndex, Text, TextBegin, TextEnd, Size, Flags, Width))
		{
			Begins.push_back(OBegin);
			Ends.push_back(OEnd);
			LineCount++;
		}
		// Oblicz nowe Y pocz¹tkowe
		if (Flags & FLAG_VBOTTOM)
			PosY -= LineCount * Size;
		else // Flags & FLAG_VMIDDLE
			PosY -= LineCount * Size * 0.5f;
		// Dalej to ju¿ jak przy dosuniêciu do góry:

		float CurrentY = PosY, NewY;
		// Na górze
		if (HitY < CurrentY)
			return false;
		// PrzejdŸ po liniach
		for (size_t Line = 0; Line < LineCount; Line++)
		{
			NewY = CurrentY + Size;
			// Znaleziono
			if (HitY < NewY)
			{
				// SprawdŸ na X
				if (HitTestSL(OutIndex, OutPercentX, PosX, HitX, Text, Begins[Line], Ends[Line], Size, Flags))
				{
					*OutPercentY = (HitY-CurrentY) / Size;
					return true;
				}
				else
					return false;
			}
			CurrentY = NewY;
		}
		// Nie znaleziono
		return false;
	}
}

void Font::Draw(float X, float Y, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width, const RECTF &BoundingRect)
{
	m_BoundingRect = BoundingRect;
#define RES_D3D_FONT_INC_DRAW_QUAD_FUNC DrawBoundedQuad2d
#define RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC DrawBoundedFilledRect2d
#include "Res_d3d_Font_Draw.inc"
#undef RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC
#undef RES_D3D_FONT_INC_DRAW_QUAD_FUNC
}

void Font::Draw(float X, float Y, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width)
{
#define RES_D3D_FONT_INC_DRAW_QUAD_FUNC DrawQuad2d
#define RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC DrawFilledRect2d
#include "Res_d3d_Font_Draw.inc"
#undef RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC
#undef RES_D3D_FONT_INC_DRAW_QUAD_FUNC
}

void Font::Draw3d(const VEC3 &Pos, const VEC3 &Right, const VEC3 &Down, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width)
{
	DrawBegin();

	assert(m_Texture);
	assert(m_VB);
	assert(m_IB);
	assert(GetState() != ST_UNLOADED);

	size_t LineBegin, LineEnd, LineIndex = TextBegin, i;
	float LineWidth;
	float StartX, CurrentX, CurrentY;
	float LineY1, LineY2;

	// Do góry
	if (Flags & FLAG_VTOP)
	{
		CurrentY = 0.0f;
		while (LineSplit(&LineBegin, &LineEnd, &LineWidth, &LineIndex, Text, TextBegin, TextEnd, Size, Flags, Width))
		{
			if (Flags & FLAG_HLEFT)
				StartX = CurrentX = 0.0f;
			else if (Flags & FLAG_HRIGHT)
				StartX = CurrentX = 0.0f - LineWidth;
			else // Flags & FLAG_HCENTER
				StartX = CurrentX = 0.0f - LineWidth * 0.5f;

			// Znaki
			for (i = LineBegin; i < LineEnd; i++)
			{
				CHAR_INFO & CharInfo = m_CharInfo[(int)(uint1)Text[i]];
				if (Text[i] != ' ')
				{
					DrawQuad3d(
						Pos, Right, Down,
						CurrentX + CharInfo.OffsetX*Size,
						CurrentY + CharInfo.OffsetY*Size,
						CurrentX + (CharInfo.OffsetX+CharInfo.Width)*Size,
						CurrentY + (CharInfo.OffsetY+CharInfo.Height)*Size,
						CharInfo.tx1, CharInfo.ty1, CharInfo.tx2, CharInfo.ty2);
				}
				CurrentX += CharInfo.Advance * Size;
			}

			// Kreœlenia
			if (Flags & (FLAG_UNDERLINE | FLAG_DOUBLE | FLAG_OVERLINE | FLAG_STRIKEOUT))
			{
				if (Flags & FLAG_UNDERLINE)
				{
					LineY2 = CurrentY + Size;
					LineY1 = LineY2 - Size*0.1f;
					DrawFilledRect3d(Pos, Right, Down, StartX, LineY1, StartX+LineWidth, LineY2);
				}
				else if (Flags & FLAG_DOUBLE)
				{
					LineY2 = CurrentY + Size;
					LineY1 = LineY2 - Size*0.2f/3.0f;
					DrawFilledRect3d(Pos, Right, Down, StartX, LineY1, StartX+LineWidth, LineY2);
					LineY2 -= Size*0.1f;
					LineY1 -= Size*0.1f;
					DrawFilledRect3d(Pos, Right, Down, StartX, LineY1, StartX+LineWidth, LineY2);
				}
				if (Flags & FLAG_OVERLINE)
				{
					LineY1 = CurrentY;
					LineY2 = LineY1 + Size*0.1f;
					DrawFilledRect3d(Pos, Right, Down, StartX, LineY1, StartX+LineWidth, LineY2);
				}
				if (Flags & FLAG_STRIKEOUT)
				{
					LineY1 = CurrentY + Size*0.5f;
					LineY2 = LineY1 + Size*0.1f;
					DrawFilledRect3d(Pos, Right, Down, StartX, LineY1, StartX+LineWidth, LineY2);
				}
			}

			CurrentY += Size;
		}
	}
	// Nie do góry
	else
	{
		// Podziel na wiersze, spamiêtaj, dowiedz siê o liczbê wierszy
		size_t LineCount = 0;
		std::vector<size_t> Begins;
		std::vector<size_t> Ends;
		std::vector<float> Widths;
		while (LineSplit(&LineBegin, &LineEnd, &LineWidth, &LineIndex, Text, TextBegin, TextEnd, Size, Flags, Width))
		{
			Begins.push_back(LineBegin);
			Ends.push_back(LineEnd);
			Widths.push_back(LineWidth);
			LineCount++;
		}
		// Oblicz nowe Y pocz¹tkowe
		if (Flags & FLAG_VBOTTOM)
			CurrentY = 0.0f - LineCount * Size;
		else // Flags & FLAG_VMIDDLE
			CurrentY = 0.0f - LineCount * Size * 0.5f;
		// Dalej to ju¿ jak przy dosuniêciu do góry:

		for (size_t Line = 0; Line < LineCount; Line++)
		{
			if (Flags & FLAG_HLEFT)
				StartX = CurrentX = 0.0f;
			else if (Flags & FLAG_HRIGHT)
				StartX = CurrentX = 0.0f - Widths[Line];
			else // Flags & FLAG_HCENTER
				StartX = CurrentX = 0.0f - Widths[Line] * 0.5f;

			// Znaki
			for (i = Begins[Line]; i < Ends[Line]; i++)
			{
				CHAR_INFO & CharInfo = m_CharInfo[(int)(uint1)Text[i]];
				if (Text[i] != ' ')
				{
					DrawQuad3d(
						Pos, Right, Down,
						CurrentX + CharInfo.OffsetX*Size,
						CurrentY + CharInfo.OffsetY*Size,
						CurrentX + (CharInfo.OffsetX+CharInfo.Width)*Size,
						CurrentY + (CharInfo.OffsetY+CharInfo.Height)*Size,
						CharInfo.tx1, CharInfo.ty1, CharInfo.tx2, CharInfo.ty2);
				}
				CurrentX += CharInfo.Advance * Size;
			}

			// Kreœlenia
			if (Flags & (FLAG_UNDERLINE | FLAG_DOUBLE | FLAG_OVERLINE | FLAG_STRIKEOUT))
			{
				if (Flags & FLAG_UNDERLINE)
				{
					LineY2 = CurrentY + Size;
					LineY1 = LineY2 - Size*0.1f;
					DrawFilledRect3d(Pos, Right, Down, StartX, LineY1, StartX+LineWidth, LineY2);
				}
				else if (Flags & FLAG_DOUBLE)
				{
					LineY2 = CurrentY + Size;
					LineY1 = LineY2 - Size*0.2f/3.0f;
					DrawFilledRect3d(Pos, Right, Down, StartX, LineY1, StartX+LineWidth, LineY2);
					LineY2 -= Size*0.1f;
					LineY1 -= Size*0.1f;
					DrawFilledRect3d(Pos, Right, Down, StartX, LineY1, StartX+LineWidth, LineY2);
				}
				if (Flags & FLAG_OVERLINE)
				{
					LineY1 = CurrentY;
					LineY2 = LineY1 + Size*0.1f;
					DrawFilledRect3d(Pos, Right, Down, StartX, LineY1, StartX+LineWidth, LineY2);
				}
				if (Flags & FLAG_STRIKEOUT)
				{
					LineY1 = CurrentY + Size*0.5f;
					LineY2 = LineY1 + Size*0.1f;
					DrawFilledRect3d(Pos, Right, Down, StartX, LineY1, StartX+LineWidth, LineY2);
				}
			}

			CurrentY += Size;
		}
	}

	DrawEnd();
}

void Font::DrawRect(float x1, float y1, float x2, float y2)
{
	DrawBegin();
	DrawFilledRect2d(x1, y1, x2, y2);
	DrawEnd();
}

void Font::DrawBoundedQuad2d(float x1, float y1, float x2, float y2, float tx1, float ty1, float tx2, float ty2)
{
	RECTF Rect = RECTF(x1, y1, x2, y2);
	if (!OverlapRect(Rect, m_BoundingRect))
		return;

	if (m_BufferPos == FONT_BUFFER_SIZE)
	{
		Unlock();
		Flush();
		Lock();
	}

	// Quad le¿y ca³kowicie w zakresie - idzie dalej od razu
	// Nie le¿y - wchodzi tutaj i modyfikuje parametry, po czym idzie dalej
	if (!RectInRect(Rect, m_BoundingRect))
	{
		// ZnajdŸ czêœæ wspóln¹
		// Nie wiem jak to mo¿liwe ¿eby tutaj Intersection zwróci³o false (chyba ¿e jest kwestia
		// ostrych/nieostrych nierównoœci), ale jak wrzuci³em asercjê zamiast returna to zdarza³o
		// mu siê wywaliæ.
		RECTF is;
		if (!Intersection(&is, Rect, m_BoundingRect))
			return;

		// ZnajdŸ wspó³rzêdne tekstury (przez znalezienie wspó³czynników funkcji liniowej)
		//float tx_a, tx_b, ty_a, ty_b;

		float tx_a = (tx2 - tx1) / (x2 - x1);
		float tx_b = tx1 - tx_a * x1;
		float ty_a = (ty2 - ty1) / (y2 - y1);
		float ty_b = ty1 - ty_a * y1;
		
		// Uaktualnij pozycje wierzcho³ków
		x1 = is.left;
		x2 = is.right;
		y1 = is.top;
		y2 = is.bottom;

		// Uaktualnij wspó³rzêdne tekstury
		tx1 = tx_a * x1 + tx_b;
		tx2 = tx_a * x2 + tx_b;
		ty1 = ty_a * y1 + ty_b;
		ty2 = ty_a * y2 + ty_b;
	}

	uint2 BaseVertexIndex = (uint2)m_VbIndex;

	m_LockedVB[m_VbIndex].Pos = VEC3(x1, y1, 1.0f);
	m_LockedVB[m_VbIndex].Tex = VEC2(tx1, ty1);
	m_VbIndex++;
	m_LockedVB[m_VbIndex].Pos = VEC3(x2, y1, 1.0f);
	m_LockedVB[m_VbIndex].Tex = VEC2(tx2, ty1);
	m_VbIndex++;
	m_LockedVB[m_VbIndex].Pos = VEC3(x1, y2, 1.0f);
	m_LockedVB[m_VbIndex].Tex = VEC2(tx1, ty2);
	m_VbIndex++;
	m_LockedVB[m_VbIndex].Pos = VEC3(x2, y2, 1.0f);
	m_LockedVB[m_VbIndex].Tex = VEC2(tx2, ty2);
	m_VbIndex++;

	m_LockedIB[m_IbIndex++] = BaseVertexIndex;
	m_LockedIB[m_IbIndex++] = BaseVertexIndex+1;
	m_LockedIB[m_IbIndex++] = BaseVertexIndex+3;
	m_LockedIB[m_IbIndex++] = BaseVertexIndex;
	m_LockedIB[m_IbIndex++] = BaseVertexIndex+3;
	m_LockedIB[m_IbIndex++] = BaseVertexIndex+2;

	m_BufferPos++;
}

void Font::DrawBegin()
{
	AcquireBuffer();
	Lock();
}

void Font::DrawEnd()
{
	Unlock();

	if (m_BufferPos > 0)
		Flush();
}

void Font::AcquireBuffer()
{
	if (!m_BufferAcquired)
	{
		if (m_BufferRefCount == 0)
		{
			HRESULT hr = frame::Dev->CreateVertexBuffer(
				FONT_BUFFER_SIZE*4*sizeof(VERTEX),
				D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
				FONT_VERTEX_FVF,
				D3DPOOL_DEFAULT,
				&m_VB, 0);
			if (FAILED(hr))
				throw DirectXError(hr, "Nie mo¿na utworzyæ bufora wierzcho³ków czcionki", __FILE__, __LINE__);

			hr = frame::Dev->CreateIndexBuffer(
				FONT_BUFFER_SIZE*6*sizeof(uint2),
				D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
				D3DFMT_INDEX16,
				D3DPOOL_DEFAULT,
				&m_IB, 0);
			if (FAILED(hr))
				throw DirectXError(hr, "Nie mo¿na utworzyæ bufora indeksów czcionki", __FILE__, __LINE__);
		}
		m_BufferRefCount++;
		m_BufferAcquired = true;
	}
}

void Font::UnacquireBuffer()
{
	if (m_BufferAcquired)
	{
		m_BufferAcquired = false;
		m_BufferRefCount--;
		if (m_BufferRefCount == 0)
		{
			SAFE_RELEASE(m_IB);
			SAFE_RELEASE(m_VB);
		}
	}
}

void Font::Lock()
{
	assert(m_VB);
	assert(m_IB);

	HRESULT hr = m_VB->Lock(0, 0, (void**)&m_LockedVB, D3DLOCK_DISCARD);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na zablokowaæ bufora wierzcho³ków do rysowania czcionki", __FILE__, __LINE__);

	hr = m_IB->Lock(0, 0, (void**)&m_LockedIB, D3DLOCK_DISCARD);
	if (FAILED(hr))
		throw DirectXError(hr, "Nie mo¿na zablokowaæ bufora wierzcho³ków do rysowania czcionki", __FILE__, __LINE__);
}

void Font::Unlock()
{
	if (m_LockedIB)
	{
		m_IB->Unlock();
		m_LockedIB = 0;
	}
	if (m_LockedVB)
	{
		m_VB->Unlock();
		m_LockedVB = 0;
	}
}

void Font::Flush()
{
	assert(m_VB);
	assert(m_IB);
	assert(m_LockedVB == 0);
	assert(m_LockedIB == 0);

	if (m_BufferPos == 0)
		return;

	frame::Dev->SetIndices(m_IB);
	frame::Dev->SetStreamSource(0, m_VB, 0, sizeof(VERTEX));
	frame::Dev->SetFVF(FONT_VERTEX_FVF);

	ERR_GUARD_DIRECTX_D( frame::Dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_BufferPos*4, 0, m_BufferPos*2) );
	frame::RegisterDrawCall(m_BufferPos*2);

	m_BufferPos = 0;
	m_VbIndex = 0;
	m_IbIndex = 0;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa D3dTextureSurface

void D3dTextureSurface::CreateSurface()
{
	assert(m_Width > 0 && m_Height > 0);
	assert(m_Texture == NULL);
	assert(m_Surface == NULL);

	// LOG
	string Usage_s, Format_s, Pool_s;
	D3dPoolToStr(&Pool_s, m_Pool);
	D3dUsageToStr(&Usage_s, m_Usage);
	D3dfmtToStr(&Format_s, m_Format);
	LOG(LOG_RESMNGR, Format("D3dTextureSurface.CreateSurface: Name=#, IsTexture=#, Width=#, Height=#, Pool=#, Usage=#, Format=#") % GetName() % m_IsTexture % m_Width % m_Height % Pool_s % Usage_s % Format_s);

	try
	{
		HRESULT hr;

		// Tekstura + surface
		if (m_IsTexture)
		{
			// Tekstura
			hr = frame::Dev->CreateTexture(m_Width, m_Height, 1, m_Usage, m_Format, D3DPOOL_DEFAULT, &m_Texture, NULL);
			if (FAILED(hr)) throw DirectXError(hr, "IDirect3DDevice9::CreateTexture");
			// Surface
			hr = m_Texture->GetSurfaceLevel(0, &m_Surface);
			if (FAILED(hr)) throw DirectXError(hr, "IDirect3DTexture9::GetSurfaceLevel");
		}
		// Sam surface
		else
		{
			if ((m_Usage & D3DUSAGE_RENDERTARGET) != 0)
			{
				hr = frame::Dev->CreateRenderTarget(m_Width, m_Height, m_Format, D3DMULTISAMPLE_NONE, 0, FALSE, &m_Surface, NULL);
				if (FAILED(hr)) throw DirectXError(hr, "IDirect3DTexture9::CreateRenderTarget");
			}
			else if ((m_Usage & D3DUSAGE_DEPTHSTENCIL) != 0)
			{
				hr = frame::Dev->CreateDepthStencilSurface(m_Width, m_Height, m_Format, D3DMULTISAMPLE_NONE, 0, FALSE, &m_Surface, NULL);
				if (FAILED(hr)) throw DirectXError(hr, "IDirect3DTexture9::CreateDepthStencilSurface");
			}
			else
			{
				hr = frame::Dev->CreateOffscreenPlainSurface(m_Width, m_Height, m_Format, m_Pool, &m_Surface, NULL);
				if (FAILED(hr)) throw DirectXError(hr, "IDirect3DTexture9::CreateOffscreenPlainSurface");
			}
		}
	}
	catch (Error &e)
	{
		string Usage_s, Format_s, Pool_s;
		D3dPoolToStr(&Pool_s, m_Pool);
		D3dUsageToStr(&Usage_s, m_Usage);
		D3dfmtToStr(&Format_s, m_Format);
		e.Push(Format("Nie mo¿na utworzyæ zasobu D3dTextureSurface. Name=#, IsTexture=#, Width=#, Height=#, Pool=#, Usage=#, Format=#") %
			GetName() % m_IsTexture % m_Width % m_Height % Pool_s % Usage_s % Format_s, __FILE__, __LINE__);
		throw;
	}
}

void D3dTextureSurface::DestroySurface()
{
	m_FilledFlag = false;

	SAFE_RELEASE(m_Surface);
	SAFE_RELEASE(m_Texture);

/*	uint SurfaceCount = 666, TextureCount = 666;
	if (m_Surface)
	{
		SurfaceCount = m_Surface->Release();
		//while (SurfaceCount-- > 0)
		//	m_Surface->Release();
		m_Surface = NULL;
	}
	if (m_Texture)
	{
		TextureCount = m_Texture->Release();
		//while (TextureCount-- > 0)
		//	m_Texture->Release();
		m_Texture = NULL;
	}

	string SurfaceCountS = UintToStrR(SurfaceCount);
	string TextureCountS = UintToStrR(TextureCount);
	if (SurfaceCountS == "666")
		SurfaceCountS = "-";
	if (TextureCountS == "666")
		TextureCountS = "-";

	LOG(1024, Format("D3dTextureSurface.DestroySurface: Name=#. SurfaceCount=#, TextureCount=#") % GetName() % SurfaceCountS % TextureCountS);*/
}

IResource * D3dTextureSurface::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	bool IsTexture; uint4 Width, Height; D3DPOOL Pool; uint4 Usage; D3DFORMAT Format;

	// IsTexture
	Params.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
	if (Params.GetString() == "texture")
		IsTexture = true;
	else if (Params.GetString() == "surface")
		IsTexture = false;
	else
		Params.CreateError("B³êdny identifikator. Oczekiwane \"texture\" lub \"surface\".");
	Params.Next();

	// Width
	Params.AssertToken(Tokenizer::TOKEN_INTEGER);
	Width = Params.MustGetUint4();
	Params.Next();

	// Height
	Params.AssertToken(Tokenizer::TOKEN_INTEGER);
	Height = Params.MustGetUint4();
	Params.Next();

	// Pool
	Params.AssertToken(Tokenizer::TOKEN_IDENTIFIER, Tokenizer::TOKEN_STRING);
	if (!StrToD3dPool(&Pool, Params.GetString())) Params.CreateError("B³êdna pula pamiêci.");
	Params.Next();

	// Usage
	Params.AssertToken(Tokenizer::TOKEN_IDENTIFIER, Tokenizer::TOKEN_STRING);
	if (!StrToD3dUsage(&Usage, Params.GetString())) Params.CreateError("B³êdna flaga u¿ycia.");
	Params.Next();

	// Format
	Params.AssertToken(Tokenizer::TOKEN_IDENTIFIER, Tokenizer::TOKEN_STRING);
	if (!StrToD3dfmt(&Format, Params.GetString())) Params.CreateError("B³êdny format.");
	Params.Next();

	// Œrednik
	Params.AssertSymbol(';');
	Params.Next();

	return new D3dTextureSurface(Name, Group, IsTexture, Width, Height, Pool, Usage, Format);
}

D3dTextureSurface::~D3dTextureSurface()
{
	DestroySurface();
}

void D3dTextureSurface::OnDeviceCreate()
{
	if (m_Pool != D3DPOOL_DEFAULT)
		CreateSurface();
}

void D3dTextureSurface::OnDeviceDestroy()
{
	if (m_Pool != D3DPOOL_DEFAULT)
		DestroySurface();

	if (!RELOAD && GetState() == ST_LOADED)
		Unloaded();
}

void D3dTextureSurface::OnDeviceRestore()
{
	if (m_Pool == D3DPOOL_DEFAULT)
		CreateSurface();
}

void D3dTextureSurface::OnDeviceInvalidate()
{
	if (m_Pool == D3DPOOL_DEFAULT)
	{
		DestroySurface();
	
		if (!RELOAD && GetState() == ST_LOADED)
			Unloaded();
	}
}

D3dTextureSurface::D3dTextureSurface(const string &Name, const string &Group, bool IsTexture, uint4 Width, uint4 Height, D3DPOOL Pool, uint4 Usage, D3DFORMAT Format) :
	D3dResource(Name, Group),
	m_IsTexture(IsTexture),
	m_Width(Width), m_Height(Height),
	m_Pool(Pool),
	m_Usage(Usage),
	m_Format(Format),
	m_Texture(NULL),
	m_Surface(NULL),
	m_FilledFlag(false)
{
}

void D3dTextureSurface::SetSize(uint4 Width, uint4 Height)
{
	if (Width == m_Width && Height == m_Height) return;

	// Jest w tej chwili ztworzona
	if (m_Surface)
	{
		DestroySurface();
		m_Width = Width;
		m_Height = Height;
		CreateSurface();
	}
	// Nie jest w tej chwili utworzona
	else
	{
		m_Width = Width;
		m_Height = Height;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa D3dCubeTextureSurface

void D3dCubeTextureSurface::CreateSurface()
{
	assert(m_Size > 0);
	assert(m_Texture == NULL);
	assert(m_Surfaces[0] == NULL);

	// LOG
	string Usage_s, Format_s, Pool_s;
	D3dPoolToStr(&Pool_s, m_Pool);
	D3dUsageToStr(&Usage_s, m_Usage);
	D3dfmtToStr(&Format_s, m_Format);
	LOG(LOG_RESMNGR, Format("D3dCubeTextureSurface.CreateSurface: Name=#, Size=#, Pool=#, Usage=#, Format=#") % GetName() % m_Size % Pool_s % Usage_s % Format_s);

	try
	{
		// Tekstura
		HRESULT hr = frame::Dev->CreateCubeTexture(m_Size, 1, m_Usage, m_Format, m_Pool, &m_Texture, NULL);
		if (FAILED(hr)) throw DirectXError(hr, "IDirect3DDevice9::CreateCubeTexture");

		// Surface
		for (size_t i = 0; i < 6; i++)
		{
			hr = m_Texture->GetCubeMapSurface((D3DCUBEMAP_FACES)i, 0, &m_Surfaces[i]);
			if (FAILED(hr)) throw DirectXError(hr, "IDirect3DCubeTexture9::GetCubeMapSurface", __FILE__, __LINE__);
		}
	}
	catch (Error &e)
	{
		string Usage_s, Format_s, Pool_s;
		D3dPoolToStr(&Pool_s, m_Pool);
		D3dUsageToStr(&Usage_s, m_Usage);
		D3dfmtToStr(&Format_s, m_Format);
		e.Push(Format("Nie mo¿na utworzyæ zasobu D3dCubeTextureSurface. Name=#, Size=#, Pool=#, Usage=#, Format=#") %
			GetName() % m_Size % Pool_s % Usage_s % Format_s, __FILE__, __LINE__);
		throw;
	}
}

void D3dCubeTextureSurface::DestroySurface()
{
	ResetAllFilled();
	for (size_t i = 6; i--; )
		SAFE_RELEASE(m_Surfaces[i]);
	SAFE_RELEASE(m_Texture);
}

IResource * D3dCubeTextureSurface::Create(const string &Name, const string &Group, Tokenizer &Params)
{
	uint4 Size; D3DPOOL Pool; uint4 Usage; D3DFORMAT Format;

	// Size
	Params.AssertToken(Tokenizer::TOKEN_INTEGER);
	Size = Params.MustGetUint4();
	Params.Next();

	// Pool
	Params.AssertToken(Tokenizer::TOKEN_IDENTIFIER, Tokenizer::TOKEN_STRING);
	if (!StrToD3dPool(&Pool, Params.GetString())) Params.CreateError("B³êdna pula pamiêci.");
	Params.Next();

	// Usage
	Params.AssertToken(Tokenizer::TOKEN_IDENTIFIER, Tokenizer::TOKEN_STRING);
	if (!StrToD3dUsage(&Usage, Params.GetString())) Params.CreateError("B³êdna flaga u¿ycia.");
	Params.Next();

	// Format
	Params.AssertToken(Tokenizer::TOKEN_IDENTIFIER, Tokenizer::TOKEN_STRING);
	if (!StrToD3dfmt(&Format, Params.GetString())) Params.CreateError("B³êdny format.");
	Params.Next();

	// Œrednik
	Params.AssertSymbol(';');
	Params.Next();

	return new D3dCubeTextureSurface(Name, Group, Size, Pool, Usage, Format);
}

D3dCubeTextureSurface::~D3dCubeTextureSurface()
{
	DestroySurface();
}

void D3dCubeTextureSurface::OnDeviceCreate()
{
	if (m_Pool != D3DPOOL_DEFAULT)
		CreateSurface();
}

void D3dCubeTextureSurface::OnDeviceDestroy()
{
	if (m_Pool != D3DPOOL_DEFAULT)
		DestroySurface();

	if (!RELOAD && GetState() == ST_LOADED)
		Unloaded();
}

void D3dCubeTextureSurface::OnDeviceRestore()
{
	if (m_Pool == D3DPOOL_DEFAULT)
		CreateSurface();
}

void D3dCubeTextureSurface::OnDeviceInvalidate()
{
	if (m_Pool == D3DPOOL_DEFAULT)
	{
		DestroySurface();
	
		if (!RELOAD && GetState() == ST_LOADED)
			Unloaded();
	}
}

D3dCubeTextureSurface::D3dCubeTextureSurface(const string &Name, const string &Group, uint4 Size, D3DPOOL Pool, uint4 Usage, D3DFORMAT Format) :
	D3dResource(Name, Group),
	m_Size(Size),
	m_Pool(Pool),
	m_Usage(Usage),
	m_Format(Format),
	m_Texture(NULL),
	m_FilledFlags(0)
{
	ZeroMemory(m_Surfaces, 6*sizeof(IDirect3DSurface9*));
}

void D3dCubeTextureSurface::GetSurfaces(IDirect3DSurface9 *OutSurfaces[])
{
	for (size_t i = 0; i < 6; i++)
		OutSurfaces[i] = m_Surfaces[i];
}

void D3dCubeTextureSurface::SetSize(uint4 Size)
{
	if (Size == m_Size) return;

	// Jest w tej chwili utworzona
	if (m_Texture)
	{
		DestroySurface();
		m_Size = Size;
		CreateSurface();
	}
	// Nie jest w tej chwili utworzona
	else
		m_Size = Size;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa D3dVertexBuffer::Lock

D3dVertexBuffer::Locker::Locker(D3dVertexBuffer *VertexBuffer, bool LockAll, uint4 FlagsOrLength)
{
	VertexBuffer->Load();
	m_VB = VertexBuffer->GetVB();

	uint4 SizeToLock, Flags;

	if (LockAll)
	{
		m_Offset = SizeToLock = 0;
		Flags = FlagsOrLength;
	}
	else
	{
		SizeToLock = FlagsOrLength;
		m_Offset = VertexBuffer->DynamicLock(SizeToLock);
		Flags = (m_Offset == 0 ? D3DLOCK_DISCARD : D3DLOCK_NOOVERWRITE);
	}

	m_OffsetInBytes = m_Offset*VertexBuffer->GetVertexSize();
	HRESULT hr = m_VB->Lock(m_OffsetInBytes, SizeToLock*VertexBuffer->GetVertexSize(), &m_Data, Flags);
	if (FAILED(hr)) throw DirectXError(hr, "D3dVertexBuffer::Locker::Locker: IDirect3DVertexBuffer9::Lock.", __FILE__, __LINE__);
}

D3dVertexBuffer::Locker::Locker(D3dVertexBuffer *VertexBuffer, uint4 OffsetToLock, uint4 SizeToLock, uint4 Flags)
{
	VertexBuffer->Load();
	m_VB = VertexBuffer->GetVB();
	m_Offset = OffsetToLock;
	m_OffsetInBytes = OffsetToLock*VertexBuffer->GetVertexSize();

	HRESULT hr = m_VB->Lock(m_OffsetInBytes, SizeToLock*VertexBuffer->GetVertexSize(), &m_Data, Flags);
	if (FAILED(hr)) throw DirectXError(hr, "D3dVertexBuffer::Locker::Locker: IDirect3DVertexBuffer9::Lock.", __FILE__, __LINE__);
}

D3dVertexBuffer::Locker::~Locker()
{
	m_VB->Unlock();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa D3dVertexBuffer

void D3dVertexBuffer::CreateBuffer()
{
	assert(m_Length > 0);
	assert(m_VB == NULL);

	HRESULT hr = frame::Dev->CreateVertexBuffer(m_Length*m_VertexSize, m_Usage, m_FVF, m_Pool, &m_VB, NULL);
	if (FAILED(hr)) throw DirectXError(hr, "D3dVertexBuffer::CreateBuffer: IDirect3DDevice9::CreateVertexBuffer", __FILE__, __LINE__);
}

void D3dVertexBuffer::DestroyBuffer()
{
	m_FilledFlag = false;
	m_NextOffset = 0;
	SAFE_RELEASE(m_VB);
}

uint4 D3dVertexBuffer::DynamicLock(uint4 Length)
{
	if (m_NextOffset > m_Length - Length)
	{
		m_NextOffset = Length;
		return 0;
	}
	else
	{
		uint4 R = m_NextOffset;
		m_NextOffset += Length;
		return R;
	}
}

D3dVertexBuffer::~D3dVertexBuffer()
{
	DestroyBuffer();
}

void D3dVertexBuffer::OnDeviceCreate()
{
	if (m_Pool != D3DPOOL_DEFAULT)
		CreateBuffer();
}

void D3dVertexBuffer::OnDeviceDestroy()
{
	if (m_Pool != D3DPOOL_DEFAULT)
	{
		DestroyBuffer();
		if (!RELOAD && GetState() == ST_LOADED)
			Unloaded();
	}
}

void D3dVertexBuffer::OnDeviceRestore()
{
	if (m_Pool == D3DPOOL_DEFAULT)
		CreateBuffer();
}

void D3dVertexBuffer::OnDeviceInvalidate()
{
	if (m_Pool == D3DPOOL_DEFAULT)
	{
		DestroyBuffer();
		if (!RELOAD && GetState() == ST_LOADED)
			Unloaded();
	}
}

D3dVertexBuffer::D3dVertexBuffer(const string &Name, const string &Group, uint4 Length, uint4 Usage, uint4 FVF, D3DPOOL Pool) :
	D3dResource(Name, Group),
	m_Length(Length),
	m_Usage(Usage),
	m_FVF(FVF),
	m_Pool(Pool),
	m_VB(NULL),
	m_FilledFlag(false),
	m_NextOffset(0)
{
	m_VertexSize = FVF::CalcVertexSize(FVF);
}

void D3dVertexBuffer::SetLength(uint4 Length)
{
	if (Length == m_Length) return;

	if (m_VB)
	{
		DestroyBuffer();
		m_Length = Length;
		CreateBuffer();
	}
	else
		m_Length = Length;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa D3dIndexBuffer::Lock

D3dIndexBuffer::Locker::Locker(D3dIndexBuffer *IndexBuffer, bool LockAll, uint4 FlagsOrLength)
{
	IndexBuffer->Load();
	m_IB = IndexBuffer->GetIB();

	uint4 SizeToLock, Flags;

	if (LockAll)
	{
		m_Offset = SizeToLock = 0;
		Flags = FlagsOrLength;
	}
	else
	{
		SizeToLock = FlagsOrLength;
		m_Offset = IndexBuffer->DynamicLock(SizeToLock);
		Flags = (m_Offset == 0 ? D3DLOCK_DISCARD : D3DLOCK_NOOVERWRITE);
	}

	m_OffsetInBytes = m_Offset*IndexBuffer->GetIndexSize();
	HRESULT hr = m_IB->Lock(m_OffsetInBytes, SizeToLock*IndexBuffer->GetIndexSize(), &m_Data, Flags);
	if (FAILED(hr)) throw DirectXError(hr, "D3dIndexBuffer::Lock::Lock: IDirect3DIndexBuffer9::Lock.", __FILE__, __LINE__);
}

D3dIndexBuffer::Locker::Locker(D3dIndexBuffer *IndexBuffer, uint4 OffsetToLock, uint4 SizeToLock, uint4 Flags)
{
	IndexBuffer->Load();
	m_IB = IndexBuffer->GetIB();
	m_Offset = OffsetToLock;
	m_OffsetInBytes = OffsetToLock*IndexBuffer->GetIndexSize();

	HRESULT hr = m_IB->Lock(m_OffsetInBytes, SizeToLock*IndexBuffer->GetIndexSize(), &m_Data, Flags);
	if (FAILED(hr)) throw DirectXError(hr, "D3dIndexBuffer::Lock::Lock: IDirect3DIndexBuffer9::Lock.", __FILE__, __LINE__);
}

D3dIndexBuffer::Locker::~Locker()
{
	m_IB->Unlock();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa D3dIndexBuffer

void D3dIndexBuffer::CreateBuffer()
{
	assert(m_Length > 0);
	assert(m_IB == NULL);

	HRESULT hr = frame::Dev->CreateIndexBuffer(m_Length*m_IndexSize, m_Usage, m_Format, m_Pool, &m_IB, NULL);
	if (FAILED(hr)) throw DirectXError(hr, "D3dIndexBuffer::CreateBuffer: IDirect3DDevice9::CreateIndexBuffer", __FILE__, __LINE__);
}

void D3dIndexBuffer::DestroyBuffer()
{
	m_FilledFlag = false;
	m_NextOffset = 0;
	SAFE_RELEASE(m_IB);
}

uint4 D3dIndexBuffer::DynamicLock(uint4 Length)
{
	if (m_NextOffset > m_Length - Length)
	{
		m_NextOffset = Length;
		return 0;
	}
	else
	{
		uint4 R = m_NextOffset;
		m_NextOffset += Length;
		return R;
	}
}

D3dIndexBuffer::~D3dIndexBuffer()
{
	DestroyBuffer();
}

void D3dIndexBuffer::OnDeviceCreate()
{
	if (m_Pool != D3DPOOL_DEFAULT)
		CreateBuffer();
}

void D3dIndexBuffer::OnDeviceDestroy()
{
	if (m_Pool != D3DPOOL_DEFAULT)
	{
		DestroyBuffer();
		if (!RELOAD && GetState() == ST_LOADED)
			Unloaded();
	}
}

void D3dIndexBuffer::OnDeviceRestore()
{
	if (m_Pool == D3DPOOL_DEFAULT)
		CreateBuffer();
}

void D3dIndexBuffer::OnDeviceInvalidate()
{
	if (m_Pool == D3DPOOL_DEFAULT)
	{
		DestroyBuffer();
		if (!RELOAD && GetState() == ST_LOADED)
			Unloaded();
	}
}

D3dIndexBuffer::D3dIndexBuffer(const string &Name, const string &Group, uint4 Length, uint4 Usage, D3DFORMAT Format, D3DPOOL Pool) :
	D3dResource(Name, Group),
	m_Length(Length),
	m_Usage(Usage),
	m_Format(Format),
	m_Pool(Pool),
	m_IB(NULL),
	m_FilledFlag(false),
	m_NextOffset(0)
{
	assert(m_Format == D3DFMT_INDEX16 || m_Format == D3DFMT_INDEX32);
	m_IndexSize = (m_Format == D3DFMT_INDEX16 ? 2 : 4);
}

void D3dIndexBuffer::SetLength(uint4 Length)
{
	if (Length == m_Length) return;

	if (m_IB)
	{
		DestroyBuffer();
		m_Length = Length;
		CreateBuffer();
	}
	else
		m_Length = Length;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa OcclusionQuery

class OcclusionQueries_pimpl
{
public:
	struct CELL
	{
		// Tworzone przy pierwszym u¿yciu, jeœli jeszcze nie utworzone to NULL.
		shared_ptr<IDirect3DQuery9, ReleasePolicy> D3dQuery;
		// Indeks nastêpnej wolnej komórki lub MAXUINT4 jeœli to ostatnia wolna.
		// Wa¿ne tylko jeœli Query=NULL
		uint NextFree;
		// Zapytanie po³¹czone z t¹ komórk¹, NULL jeœli komórka jest wolna
		OcclusionQueries::Query *Query;
	};

	std::vector<CELL> Cells;
	// Indeks pierwszej wolnej komórki lub MAXUINT4 jeœli wszystkie zajête
	uint FirstFree;
	FreeList<OcclusionQueries::Query> QueryMemory;

	OcclusionQueries_pimpl() : QueryMemory(100) { }
	uint AcquireCell(OcclusionQueries::Query *q);
	void ReleaseCell(uint Index);
	void ReleaseD3dQueries();
};

uint OcclusionQueries_pimpl::AcquireCell(OcclusionQueries::Query *q)
{
	// Nie ma ¿adnej wolnej komórki
	if (FirstFree == MAXUINT4)
	{
		bool Found = false;
		uint FooResult;
		// Spróbuj uwolniæ któreœ z komórek poprzez sprawdzanie czy jakieœ blokuj¹ce je query ju¿ siê doczeka³o
		for (uint i = 0; i < Cells.size(); i++)
		{
			assert(Cells[i].Query != NULL);
			if (Cells[i].Query->CheckResult(&FooResult))
				Found = true;
		}
		// Znalaz³o siê, a wiêc coœ siê na pewno zwolno³o (CheckResult wywo³a³o OnQueryGotResult, które wywo³a³o ReleaseCell)
		if (Found)
		{
			assert(FirstFree != MAXUINT4);
			// I tyle - koniec, FirstFree zawiera indeks wolnej komórki
		}
		else
		{
			// CheckResult nie pomog³o, poczekaj na któreœ z zapytañ
			assert(Cells[0].Query != NULL);
			Cells[0].Query->GetResult();
			assert(FirstFree != MAXUINT4);
		}
	}

	uint Index = FirstFree;
	FirstFree = Cells[Index].NextFree;
	Cells[Index].NextFree = MAXUINT4; // Tak dla przezornoœci, w sumie to niepotrzebne
	Cells[Index].Query = q;
	// Zapewnij IDirect3DQuery9
	if (Cells[Index].D3dQuery == NULL)
	{
		IDirect3DQuery9 *D3dQuery;
		HRESULT hr = frame::Dev->CreateQuery(D3DQUERYTYPE_OCCLUSION, &D3dQuery);
		if (FAILED(hr))
			throw DirectXError(hr, "Nie mo¿na utworzyæ IDirect3DQuery9 typu D3DQUERYTYPE_OCCLUSION.", __FILE__, __LINE__);
		Cells[Index].D3dQuery.reset(D3dQuery);
	}
	return Index;
}

void OcclusionQueries_pimpl::ReleaseCell(uint Index)
{
	assert(Cells[Index].Query != NULL);
	// Zwolnij
	Cells[Index].Query = NULL;
	// Dodaj do listy wolnych
	Cells[Index].NextFree = FirstFree;
	FirstFree = Index;
}

void OcclusionQueries_pimpl::ReleaseD3dQueries()
{
	for (uint i = 0; i < Cells.size(); i++)
	{
		assert(Cells[i].Query == NULL && "Niezwolnione OcclusionQueries::Query przed OnDeviceInvalidate.");
		Cells[i].D3dQuery.reset();
	}
}

class OcclusionQueries::Query_pimpl
{
public:
	OcclusionQueries *QueriesObj;
	// Indeks komórki q QueriesObj->pimpl->Cells
	uint CellIndex;
	IDirect3DQuery9 *D3dQuery;

	enum STATE
	{
		STATE_CREATED,
		STATE_ISSUING,
		STATE_ISSUED,
		STATE_RESULT
	};
	STATE State;
	uint Result;
};

OcclusionQueries::Query::Query()
{
}

OcclusionQueries::Query::~Query()
{
}

bool OcclusionQueries::Query::CheckResult(uint *OutPixelCount)
{
	if (pimpl->State == Query_pimpl::STATE_RESULT)
	{
		*OutPixelCount = pimpl->Result;
		return true;
	}
	else if (pimpl->State == Query_pimpl::STATE_ISSUED)
	{
		HRESULT hr = pimpl->D3dQuery->GetData(&pimpl->Result, sizeof(DWORD), 0);
		// Jeszcze nie gotowe
		if (hr == S_FALSE)
			return false;
		// Gotowe lub b³¹d
		else
		{
			pimpl->QueriesObj->OnQueryGotResult(pimpl->CellIndex);
			pimpl->CellIndex = MAXUINT4;
			pimpl->D3dQuery = NULL; // Niepotrzebne

			pimpl->State = Query_pimpl::STATE_RESULT;

			// B³¹d
			if (hr != S_OK)
				pimpl->Result = MAXUINT4;

			*OutPixelCount = pimpl->Result;
			return true;
		}
	}
	else
	{
		assert(0 && "OcclusionQueries::Query::CheckResult: Query w b³êdnym stanie.");
		*OutPixelCount = MAXUINT4;
		return true;
	}
}

uint OcclusionQueries::Query::GetResult()
{
	if (pimpl->State == Query_pimpl::STATE_RESULT)
		return pimpl->Result;
	else if (pimpl->State == Query_pimpl::STATE_ISSUED)
	{
		HRESULT hr;
		do
		{
			hr = pimpl->D3dQuery->GetData(&pimpl->Result, sizeof(DWORD), D3DGETDATA_FLUSH);
		}
		while (hr == S_FALSE);

		pimpl->QueriesObj->OnQueryGotResult(pimpl->CellIndex);
		pimpl->CellIndex = MAXUINT4;
		pimpl->D3dQuery = NULL; // Niepotrzebne
		pimpl->State = Query_pimpl::STATE_RESULT;
		if (hr != S_OK)
			pimpl->Result = MAXUINT4;
		return pimpl->Result;
	}
	else
	{
		assert(0 && "OcclusionQueries::Query::CheckResult: Query w b³êdnym stanie.");
		return MAXUINT4;
	}
}

void OcclusionQueries::Query::IssueBegin()
{
	assert(pimpl->State == Query_pimpl::STATE_CREATED && "OcclusionQueries::Query::IssueBegin: Query w b³êdnym stanie, u¿yte wczeœniej.");
	pimpl->D3dQuery->Issue(D3DISSUE_BEGIN);
	pimpl->State = Query_pimpl::STATE_ISSUING;
}

void OcclusionQueries::Query::IssueEnd()
{
	assert(pimpl->State == Query_pimpl::STATE_ISSUING && "OcclusionQueries::Query::IssueEnd: Query w b³êdnym stanie, zapewne u¿yte wczeœniej.");
	pimpl->D3dQuery->Issue(D3DISSUE_END);
	pimpl->State = Query_pimpl::STATE_ISSUED;
}

OcclusionQueries::OcclusionQueries(const string &Name, const string &Group, uint MaxRealQueries) :
	D3dResource(Name, Group)
{
	pimpl.reset(new OcclusionQueries_pimpl);

	pimpl->Cells.resize(MaxRealQueries);
	for (uint i = 0; i < MaxRealQueries; i++)
	{
		//pimpl->Cells[i].D3dQuery jest automatycznie NULL
		pimpl->Cells[i].NextFree = (i == (MaxRealQueries-1) ? MAXUINT4 : (i+1));
		pimpl->Cells[i].Query = NULL;
	}
	pimpl->FirstFree = 0;
}

OcclusionQueries::~OcclusionQueries()
{
	pimpl->ReleaseD3dQueries();
	pimpl.reset();
}

OcclusionQueries::Query * OcclusionQueries::Create()
{
	Query *q = pimpl->QueryMemory.New();
	q->pimpl.reset(new OcclusionQueries::Query_pimpl);
	q->pimpl->QueriesObj = this;
	q->pimpl->Result = MAXUINT4;
	q->pimpl->State = Query_pimpl::STATE_CREATED;
	q->pimpl->CellIndex = pimpl->AcquireCell(q);
	q->pimpl->D3dQuery = pimpl->Cells[q->pimpl->CellIndex].D3dQuery.get();
	return q;
}

void OcclusionQueries::Destroy(Query *q)
{
	if (q->pimpl->CellIndex != MAXUINT4)
		pimpl->ReleaseCell(q->pimpl->CellIndex);
	pimpl->QueryMemory.Delete(q);
}

void OcclusionQueries::OnQueryGotResult(uint CellIndex)
{
	pimpl->ReleaseCell(CellIndex);
}

void OcclusionQueries::OnDeviceCreate()
{
}

void OcclusionQueries::OnDeviceDestroy()
{
}

void OcclusionQueries::OnDeviceRestore()
{
}

void OcclusionQueries::OnDeviceInvalidate()
{
	pimpl->ReleaseD3dQueries();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// GLOBALNE

void RegisterTypesD3d()
{
	g_Manager->RegisterResourceType("texture", &D3dTexture::Create);
	g_Manager->RegisterResourceType("CubeTexture", &D3dCubeTexture::Create);
	g_Manager->RegisterResourceType("d3dxfont", &D3dFont::Create);
	g_Manager->RegisterResourceType("d3dxeffect", &D3dEffect::Create);
	g_Manager->RegisterResourceType("font", &Font::Create);
	g_Manager->RegisterResourceType("TextureSurface", &D3dTextureSurface::Create);
}

} // namespace res

/*
Wymagania:
- w³¹czony <d3dx9.h>
- w³¹czony "ResMngr.h"
- res::Manager ma otrzymywaæ takie eventy:
  - na OnDeviceCreate     - Type EVENT_D3D_DEVICE_CREATE,     Params 0
  - na OnDeviceDestroy    - Type EVENT_D3D_DEVICE_DESTROY,    Params 0
  - na OnDeviceRestore    - Type EVENT_D3D_DEVICE_RESTORE,    Params 0
  - na OnDeviceInvalidate - Type EVENT_D3D_DEVICE_INVALIDATE, Params 0
- trzeba wywo³aæ RegisterTypesD3d
*/
