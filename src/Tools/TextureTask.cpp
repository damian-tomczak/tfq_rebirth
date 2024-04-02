/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "PCH.hpp"
#include "GlobalCode.hpp"
#include "TextureTask.hpp"


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje globalne

// Zmienia rozszerzenie pliku typu ".jpg" (wielkoœæ liter nieistotna) na format obrazka.
// Zwraca true jeœli siê uda³o.
bool ExtToImageFileFormat(D3DXIMAGE_FILEFORMAT *Out, const string &FileExt)
{
	string ExtL;
	LowerCase(&ExtL, FileExt);

	if (ExtL == ".bmp")
		*Out = D3DXIFF_BMP;
	else if (ExtL == ".jpg" || ExtL == ".jpeg")
		*Out = D3DXIFF_JPG;
	else if (ExtL == ".tga")
		*Out = D3DXIFF_TGA;
	else if (ExtL == ".png")
		*Out = D3DXIFF_PNG;
	else if (ExtL == ".dds")
		*Out = D3DXIFF_DDS;
	else if (ExtL == ".ppm")
		*Out = D3DXIFF_PPM;
	else if (ExtL == ".dib")
		*Out = D3DXIFF_DIB;
	else if (ExtL == ".hdr")
		*Out = D3DXIFF_HDR;
	else if (ExtL == ".pfm")
		*Out = D3DXIFF_PFM;
	else
		return false;

	return true;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasy zadañ

const char * const GEN_RAND_ERRMSG = "Invalid GenRand parameters.";
const char * const SWIZZLE_ERRMSG = "Invalid swizzle parameters.";
const char * const SHARPEN_ALPHA_ERRMSG = "Invalid SharpenAlpha parameters.";

GenRandTextureTask::GenRandTextureTask(const string &Params)
{
	size_t CommaPos = Params.find(',');
	if (CommaPos == string::npos)
		throw Error(GEN_RAND_ERRMSG);

	MustStrToSth<uint>(&TextureWidth, Params.substr(0, CommaPos));
	MustStrToSth<uint>(&TextureHeight, Params.substr(CommaPos+1));

	if (TextureWidth == 0 || TextureHeight == 0)
		throw Error("Texture size cannot be zero.", __FILE__, __LINE__);
}

SwizzleTextureTask::SwizzleTextureTask(const string &Params)
{
	if (Params.size() != 4)
		throw Error(SWIZZLE_ERRMSG);
	
	for (uint i = 0; i < 4; i++)
	{
		Swizzle[i] = Params[i];
		if (Swizzle[i] != 'A' && Swizzle[i] != 'R' && Swizzle[i] != 'G' && Swizzle[i] != 'B' && Swizzle[i] != '0' && Swizzle[i] != '1')
			throw Error(SWIZZLE_ERRMSG);
	}
}

SharpenAlphaTextureTask::SharpenAlphaTextureTask(const string &Params)
{
	MustStrToSth<COLOR>(&BackgroundColor, Params);
}

TextureJob::TextureJob() :
	AlphaThreshold(128)
{
}

TextureJob::~TextureJob()
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Texture

// Przechowuje zawsze utworzon¹ i zawsze zablokowan¹ teksturê
class Texture
{
public:
	Texture(TextureJob &Job, const InputTextureTask &Task);
	Texture(TextureJob &Job, const GenRandTextureTask &Task);
	~Texture();

	void OutputTask(TextureJob &Job, const OutputTextureTask &Task);
	void InfoTask(TextureJob &Job, const InfoTextureTask &Task);
	void SwizzleTask(TextureJob &Job, const SwizzleTextureTask &Task);
	void SharpenAlphaTask(TextureJob &Job, const SharpenAlphaTextureTask &Task);
	void ClampTransparentTask(TextureJob &Job, const ClampTransparentTextureTask &Task);

private:
	scoped_ptr<IDirect3DTexture9, ReleasePolicy> m_Texture;
	D3DSURFACE_DESC m_Desc;
	D3DLOCKED_RECT m_LockedRect;

	void UpdateDesc();
	void Lock();
	void Unlock();
	COLOR * GetLine(uint y) { return (COLOR*)( (char*)m_LockedRect.pBits + y*m_LockedRect.Pitch ); };
	COLOR & GetPixel_(uint x, uint y) { return GetLine(y)[x]; };
	// Tworzy teksturê. Nie pobiera DESC ani nie blokuje.
	void CreateTexture(uint Width, uint Height);
	// Wype³nia teksturê losowo
	void GenRand();
	/*// Calculated background color from transparent pixels.
	// Returned Alpha doesn't matter.
	void CalcBackgroundColor(COLORF *Out);*/
};

Texture::Texture(TextureJob &Job, const InputTextureTask &Task)
{
	m_LockedRect.pBits = NULL;

	Writeln(Format("Loading texture from \"#\"...") % Task.FileName);

	IDirect3DTexture9 *TexturePtr;
	HRESULT hr = D3DXCreateTextureFromFileEx(
		EnsureDev(), // Device
		Task.FileName.c_str(), // SrcFile
		D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, // Width, Height
		D3DX_FROM_FILE, // MipLevels
		0, // Usage
		D3DFMT_A8R8G8B8, // Format
		D3DPOOL_SCRATCH, // Pool
		D3DX_DEFAULT, // Filter
		D3DX_DEFAULT, // MipFilter
		0, // ColorKey
		NULL, // SrcInfo
		NULL, // Palette
		&TexturePtr); // Texture
	if (FAILED(hr)) throw DirectXError(hr, Format("Cannot load texture from file \"#\".") % Task.FileName, __FILE__, __LINE__);
	m_Texture.reset(TexturePtr);

	UpdateDesc();

	if (m_Desc.Format != D3DFMT_A8R8G8B8)
		// Tu by siê przyda³a konwersja D3DFORMAT -> string, ale to jest w module D3dUtils nale¿¹cym do Framework i wymagaj¹cym tamtejszego PCH.
		throw Error(Format("Could not load texture in format A8R8G8B8. Loaded in #.") % (uint)m_Desc.Format);
	if (m_Desc.Width == 0 || m_Desc.Height == 0)
		throw Error(Format("Image size is # x #.") % m_Desc.Width % m_Desc.Height, __FILE__, __LINE__);

	Lock();
}

Texture::Texture(TextureJob &Job, const GenRandTextureTask &Task)
{
	Writeln("Generating random texture...");

	m_LockedRect.pBits = NULL;

	CreateTexture(Task.TextureWidth, Task.TextureHeight);
	UpdateDesc();
	Lock();

	GenRand();
}

Texture::~Texture()
{
	Unlock();
}

void Texture::OutputTask(TextureJob &Job, const OutputTextureTask &Task)
{
	Writeln(Format("Saving texture to \"#\"...") % Task.FileName);

	D3DXIMAGE_FILEFORMAT Format;
	string Ext; ExtractFileExt(&Ext, Task.FileName);
	if (!ExtToImageFileFormat(&Format, Ext))
		throw Error(common::Format("Unrecognized file format from extension \"#\".") % Ext, __FILE__, __LINE__);

	Unlock();

	HRESULT hr = D3DXSaveTextureToFile(Task.FileName.c_str(), Format, m_Texture.get(), NULL);
	if (FAILED(hr)) throw DirectXError(hr, common::Format("Cannot save texture to file \"#\".") % Task.FileName, __FILE__, __LINE__);

	Lock();
}

void Texture::InfoTask(TextureJob &Job, const InfoTextureTask &Task)
{
	Writeln(Format("Info: Width=#, Height=#") % m_Desc.Width % m_Desc.Height);

	if (Task.Detailed)
	{
		COLOR *Pixel, *Line = GetLine(0);

		uint1 Min[4], Max[4];
		uint8 Sum[4];

		Min[0] = Line->A; Min[1] = Line->R; Min[2] = Line->G; Min[3] = Line->B;
		Max[0] = Line->A; Max[1] = Line->R; Max[2] = Line->G; Max[3] = Line->B;
		Sum[0] = Sum[1] = Sum[2] = Sum[3] = 0;

		uint x, y;
		for (y = 0; y < m_Desc.Height; y++)
		{
			Line = GetLine(y);
			for (x = 0; x < m_Desc.Width; x++)
			{
				Pixel = &Line[x];

				if (Pixel->A < Min[0]) Min[0] = Pixel->A;
				if (Pixel->R < Min[1]) Min[1] = Pixel->R;
				if (Pixel->G < Min[2]) Min[2] = Pixel->G;
				if (Pixel->B < Min[3]) Min[3] = Pixel->B;

				if (Pixel->A > Max[0]) Max[0] = Pixel->A;
				if (Pixel->R > Max[1]) Max[1] = Pixel->R;
				if (Pixel->G > Max[2]) Max[2] = Pixel->G;
				if (Pixel->B > Max[3]) Max[3] = Pixel->B;

				Sum[0] += (uint8)Pixel->A;
				Sum[1] += (uint8)Pixel->R;
				Sum[2] += (uint8)Pixel->G;
				Sum[3] += (uint8)Pixel->B;
			}
		}

		uint PixelCount = m_Desc.Width * m_Desc.Height;

		Writeln(Format("  A: Min=#, Max=#, Avg=#") % (uint)Min[0] % (uint)Max[0] % (Sum[0] / PixelCount));
		Writeln(Format("  R: Min=#, Max=#, Avg=#") % (uint)Min[1] % (uint)Max[1] % (Sum[1] / PixelCount));
		Writeln(Format("  G: Min=#, Max=#, Avg=#") % (uint)Min[2] % (uint)Max[2] % (Sum[2] / PixelCount));
		Writeln(Format("  B: Min=#, Max=#, Avg=#") % (uint)Min[3] % (uint)Max[3] % (Sum[3] / PixelCount));
	}
}

void Texture::SwizzleTask(TextureJob &Job, const SwizzleTextureTask &Task)
{
	Writeln("Swizzling texture...");

	uint x, y;
	COLOR *Pixel, *Line;
	COLOR Color;

	for (y = 0; y < m_Desc.Height; y++)
	{
		Line = GetLine(y);
		for (x = 0; x < m_Desc.Width; x++)
		{
			Pixel = &Line[x];
			Color = *Pixel;

			switch (Task.Swizzle[0])
			{
			case 'A': break;
			case 'R': Pixel->A = Color.R; break;
			case 'G': Pixel->A = Color.G; break;
			case 'B': Pixel->A = Color.B; break;
			case '0': Pixel->A = 0; break;
			case '1': Pixel->A = 255; break;
			}

			switch (Task.Swizzle[1])
			{
			case 'A': Pixel->R = Color.A; break;
			case 'R': break;
			case 'G': Pixel->R = Color.G; break;
			case 'B': Pixel->R = Color.B; break;
			case '0': Pixel->R = 0; break;
			case '1': Pixel->R = 255; break;
			}

			switch (Task.Swizzle[2])
			{
			case 'A': Pixel->G = Color.A; break;
			case 'R': Pixel->G = Color.R; break;
			case 'G': break;
			case 'B': Pixel->G = Color.B; break;
			case '0': Pixel->G = 0; break;
			case '1': Pixel->G = 255; break;
			}

			switch (Task.Swizzle[3])
			{
			case 'A': Pixel->B = Color.A; break;
			case 'R': Pixel->B = Color.R; break;
			case 'G': Pixel->B = Color.G; break;
			case 'B': break;
			case '0': Pixel->B = 0; break;
			case '1': Pixel->B = 255; break;
			}
		}
	}
}

void Texture::SharpenAlphaTask(TextureJob &Job, const SharpenAlphaTextureTask &Task)
{
	Writeln("Sharpening image transparency...");

	COLOR BackgroundColorB = Task.BackgroundColor;
	COLORF BackgroundColor; ColorToColorf(&BackgroundColor, BackgroundColorB);

	// Process all pixels
	uint x, y;
	COLOR *Pixel, *Line;
	COLORF Color;

	for (y = 0; y < m_Desc.Height; y++)
	{
		Line = GetLine(y);
		for (x = 0; x < m_Desc.Width; x++)
		{
			Pixel = &Line[x];
			if (Pixel->A < Job.AlphaThreshold)
			{
				*Pixel = BackgroundColorB;
				Pixel->A = 0;
			}
			else
			{
				ColorToColorf(&Color, *Pixel);

				// Here is he formula taken from:
				// http://www.vterrain.org/Plants/Alpha/index.html
				Color = BackgroundColor + (Color - BackgroundColor) / Color.A;

				*Pixel = ColorfToColor(Color);
				Pixel->A = 255;
			}
		}
	}
}

void Texture::ClampTransparentTask(TextureJob &Job, const ClampTransparentTextureTask &Task)
{
	Writeln("Clamping transparent areas...");

	uint x, y, x2, y2;
	COLOR *Pixel, *Line, *Pixel2;
	COLOR Color, BestColor;
	uint Dist, BestDist;

	for (y = 0; y < m_Desc.Height; y++)
	{
		Line = GetLine(y);
		for (x = 0; x < m_Desc.Width; x++)
		{
			Pixel = &Line[x];
			if (Pixel->A < Job.AlphaThreshold)
			{
				// Initialize
				BestColor = *Pixel;
				BestDist = MAXUINT4;
				// Left
				Dist = 0;
				for (x2 = x; x2--; Dist++)
				{
					Pixel2 = &GetPixel_(x2, y);
					if (Pixel2->A >= Job.AlphaThreshold)
					{
						BestDist = Dist;
						BestColor = *Pixel2;
						break;
					}
				}
				// Right
				Dist = 0;
				for (x2 = x; x2 < m_Desc.Width && Dist < BestDist; x2++, Dist++)
				{
					Pixel2 = &GetPixel_(x2, y);
					if (Pixel2->A >= Job.AlphaThreshold)
					{
						BestDist = Dist;
						BestColor = *Pixel2;
						break;
					}
				}
				// Top
				Dist = 0;
				for (y2 = y; (y2--) && Dist < BestDist; Dist++)
				{
					Pixel2 = &GetPixel_(x, y2);
					if (Pixel2->A >= Job.AlphaThreshold)
					{
						BestDist = Dist;
						BestColor = *Pixel2;
						break;
					}
				}
				// Bottom
				Dist = 0;
				for (y2 = y; y2 < m_Desc.Height && Dist < BestDist; y2++, Dist++)
				{
					Pixel2 = &GetPixel_(x, y2);
					if (Pixel2->A >= Job.AlphaThreshold)
					{
						BestDist = Dist;
						BestColor = *Pixel2;
						break;
					}
				}

				Pixel->R = BestColor.R;
				Pixel->G = BestColor.G;
				Pixel->B = BestColor.B;
			}
		}
	}
}

void Texture::UpdateDesc()
{
	m_Texture->GetLevelDesc(0, &m_Desc);
}

void Texture::Lock()
{
	assert(m_LockedRect.pBits == NULL);
	HRESULT hr = m_Texture->LockRect(0, &m_LockedRect, NULL, 0);
	if (FAILED(hr)) throw DirectXError(hr, "Cannot lock texture.", __FILE__, __LINE__);
}

void Texture::Unlock()
{
	assert(m_LockedRect.pBits != NULL);
	m_Texture->UnlockRect(0);
	m_LockedRect.pBits = NULL;
}

void Texture::CreateTexture(uint Width, uint Height)
{
	IDirect3DTexture9 *TexturePtr;
	HRESULT hr = EnsureDev()->CreateTexture(
		Width, // Width
		Height, // Height
		1, // Levels
		0, // Usage,
		D3DFMT_A8R8G8B8, // Format
		D3DPOOL_SCRATCH, // Pool
		&TexturePtr, // Texture
		NULL); // SharedHandle
	if (FAILED(hr)) throw DirectXError(hr, "Cannot create texture.", __FILE__, __LINE__);
	m_Texture.reset(TexturePtr);
}

void Texture::GenRand()
{
	uint x, y;
	COLOR *Pixel, *Line;

	srand(GetTickCount());

	for (y = 0; y < m_Desc.Height; y++)
	{
		Line = GetLine(y);
		for (x = 0; x < m_Desc.Width; x++)
		{
			Pixel = &Line[x];
			Pixel->ARGB = (uint)rand() ^ ((uint)rand() << 8) ^ ((uint)rand() << 16) << ((uint)rand() << 24);
		}
	}
}

/*void Texture::CalcBackgroundColor(COLORF *Out)
{
	*Out = COLORF::BLACK;
	float Sum = 0.0f;

	uint x, y;
	COLOR *Pixel, *Line;
	COLORF Color;
	float Weight;

	srand(GetTickCount());

	for (y = 0; y < m_Desc.Height; y++)
	{
		Line = GetLine(y);
		for (x = 0; x < m_Desc.Width; x++)
		{
			Pixel = &Line[x];
			ColorToColorf(&Color, *Pixel);

			Weight = 1.0f - Color.A;
			*Out += Color * Weight;
			Sum += Weight;
		}
	}

	*Out /= Weight;

	Writeln(Format("  Background color: #") % ColorfToColor(*Out));
}*/


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje globalne


void DoTextureJob(TextureJob &Job)
{
	scoped_ptr<Texture> t;

	for (uint ti = 0; ti < Job.Tasks.size(); ti++)
	{
		TextureTask *task = Job.Tasks[ti].get();

		if ((dynamic_cast<InputTextureTask*>(task)) != NULL)
			t.reset(new Texture(Job, *static_cast<InputTextureTask*>(task)));
		else if ((dynamic_cast<GenRandTextureTask*>(task)) != NULL)
			t.reset(new Texture(Job, *static_cast<GenRandTextureTask*>(task)));
		else if ((dynamic_cast<OutputTextureTask*>(task)) != NULL)
			t->OutputTask(Job, *static_cast<OutputTextureTask*>(task));
		else if ((dynamic_cast<InfoTextureTask*>(task)) != NULL)
			t->InfoTask(Job, *static_cast<InfoTextureTask*>(task));
		else if ((dynamic_cast<SwizzleTextureTask*>(task)) != NULL)
			t->SwizzleTask(Job, *static_cast<SwizzleTextureTask*>(task));
		else if ((dynamic_cast<SharpenAlphaTextureTask*>(task)) != NULL)
			t->SharpenAlphaTask(Job, *static_cast<SharpenAlphaTextureTask*>(task));
		else if ((dynamic_cast<ClampTransparentTextureTask*>(task)) != NULL)
			t->ClampTransparentTask(Job, *static_cast<ClampTransparentTextureTask*>(task));
		else
			throw Error("Unknown texture task type.");
	}
}
