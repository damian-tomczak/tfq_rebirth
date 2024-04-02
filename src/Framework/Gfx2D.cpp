/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "Framework.hpp"
#include "Gfx2D.hpp"

namespace gfx2d
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Sprite

Sprite::Sprite(const string &TextureName, const string &EffectName) :
	m_Effect(0),
	m_Texture(0),
	m_SimpleElementsMatrix(true),
	m_MatrixWidth(0), m_MatrixHeight(0),
	m_MatrixStartX(0), m_MatrixStartY(0),
	m_MatrixSpaceX(0), m_MatrixSpaceY(0)
{
	ERR_TRY;

	m_Effect = res::g_Manager->MustGetResourceEx<res::D3dEffect>(EffectName);
	m_Texture = res::g_Manager->MustGetResourceEx<res::D3dTexture>(TextureName);

	ERR_CATCH_FUNC;
}

Sprite::Sprite(res::D3dTexture *Texture, res::D3dEffect *Effect) :
	m_Effect(Effect),
	m_Texture(Texture),
	m_SimpleElementsMatrix(true),
	m_MatrixWidth(0), m_MatrixHeight(0),
	m_MatrixStartX(0), m_MatrixStartY(0),
	m_MatrixSpaceX(0), m_MatrixSpaceY(0)
{
}

void Sprite::SetSimpleElementsMatrix(uint4 ColCount, uint4 Width, uint4 Height, uint4 StartX, uint4 StartY, uint4 SpaceX, uint4 SpaceY)
{
	m_SimpleElementsMatrix = true;
	m_MatrixColCount = ColCount;
	m_MatrixWidth = Width;
	m_MatrixHeight = Height;
	m_MatrixStartX = StartX;
	m_MatrixStartY = StartY;
	m_MatrixSpaceX = SpaceX;
	m_MatrixSpaceY = SpaceY;
}

void Sprite::SetSimpleElementsCustomRects()
{
	m_SimpleElementsMatrix = false;
}

void Sprite::AddSimpleElementsCustomRect(uint4 index, const RECTI &rect)
{
	if (m_CustomRects.size() < index+1)
		m_CustomRects.resize(index+1);

	m_CustomRects[index] = rect;
}

void Sprite::AddComplexElement(uint4 index, const COMPLEX_ELEMENT &ComplexElement)
{
	if (m_ComplexElements.size() < index+1)
		m_ComplexElements.resize(index+1);

	m_ComplexElements[index] = ComplexElement;
}

void Sprite::GetTexCoords(uint4 Index, float *TexLeft, float *TexRight, float *TexTop, float *TexBottom)
{
	assert(Index == INDEX_ALL || Index < 1000000000);

	// Ca³oœæ tekstury
	if (Index == INDEX_ALL)
	{
		*TexLeft = *TexTop = 0.0f;
		*TexRight = *TexBottom = 1.0f;
	}
	// Element prosty
	else
	{
		// Tu potrzebna bêdzie szerokoœæ i wysokoœæ tekstury
		m_Texture->Load();
		float TextureWidth = static_cast<float>(m_Texture->GetWidth());
		float TextureHeight = static_cast<float>(m_Texture->GetHeight());

		// Elementy proste s¹ macierz¹
		if (m_SimpleElementsMatrix)
		{
			uint4 x = Index % m_MatrixColCount;
			uint4 y = Index / m_MatrixColCount;

			if (x > 0) x = x*(m_MatrixWidth+m_MatrixSpaceX);
			if (y > 0) y = y*(m_MatrixHeight+m_MatrixSpaceY);

			x += m_MatrixStartX;
			y += m_MatrixStartY;

			*TexLeft = static_cast<float>(x) / TextureWidth;
			*TexTop = static_cast<float>(y) / TextureHeight;

			x += m_MatrixWidth;
			y += m_MatrixHeight;

			*TexRight = static_cast<float>(x) / TextureWidth;
			*TexBottom = static_cast<float>(y) / TextureHeight;
		}
		// Elementy proste s¹ w³asn¹ list¹ prostok¹tów
		else
		{
			assert(Index < m_CustomRects.size());

			*TexLeft = static_cast<float>(m_CustomRects[Index].left) / TextureWidth;
			*TexTop = static_cast<float>(m_CustomRects[Index].top) / TextureHeight;
			*TexRight = static_cast<float>(m_CustomRects[Index].right) / TextureWidth;
			*TexBottom = static_cast<float>(m_CustomRects[Index].bottom) / TextureHeight;
		}
	}
}

void Sprite::ProcessFlags(VEC2 *tex11, VEC2 *tex21, VEC2 *tex12, VEC2 *tex22, float tx1, float ty1, float tx2, float ty2, uint4 Flags)
{
	*tex11 = VEC2(tx1, ty1);
	*tex21 = VEC2(tx2, ty1);
	*tex12 = VEC2(tx1, ty2);
	*tex22 = VEC2(tx2, ty2);

	if (Flags & FLAG_FLIP_V)
	{
		std::swap(tex11->x, tex21->x);
		std::swap(tex12->x, tex22->x);
	}
	if (Flags & FLAG_FLIP_H)
	{
		std::swap(tex11->y, tex12->y);
		std::swap(tex21->y, tex22->y);
	}

	if (Flags & FLAG_ROTATE_CW)
	{
		VEC2 tmp = *tex11;
		*tex11 = *tex12;
		*tex12 = *tex22;
		*tex22 = *tex21;
		*tex21 = tmp;
	}
	else if (Flags & FLAG_ROTATE_CCW)
	{
		VEC2 tmp = *tex11;
		*tex11 = *tex21;
		*tex21 = *tex22;
		*tex22 = *tex12;
		*tex12 = tmp;
	}
	else if (Flags & FLAG_ROTATE_UD)
	{
		std::swap(*tex11, *tex22);
		std::swap(*tex12, *tex21);
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa SpriteRepository

class SpriteRepository_pimpl
{
private:
	void ParseFlags(Tokenizer &t, uint4 *Flags);
	void ReadComplexElementData(Tokenizer &t, uint4 *Index, float *Width, float *Height, uint4 *Flags);
	void ReadComplexElementData2(Tokenizer &t, uint4 *Index, uint4 *Flags);

public:
	typedef std::map<string, shared_ptr<Sprite> > SPRITE_MAP;
	SPRITE_MAP m_Sprites;

	void Create(const string &name, Sprite *sprite);
	// Mo¿e siê skoñczyæ '}' lub EOF, tego ju¿ ma nie skonsumowaæ.
	Sprite * CreateFromDocument(const string &Name, Tokenizer &t);
};

void SpriteRepository_pimpl::Create(const string &name, Sprite *sprite)
{
	if (m_Sprites.insert(std::make_pair(name, shared_ptr<Sprite>(sprite))).second == false)
		throw Error("Nie mo¿na dodaæ sprita o nazwie: " + name, __FILE__, __LINE__);
}

void SpriteRepository_pimpl::ParseFlags(Tokenizer &t, uint4 *Flags)
{
	while (t.QueryToken(Tokenizer::TOKEN_IDENTIFIER))
	{
		if (t.GetString() == "CW")
		{
			t.Next();
			*Flags |= FLAG_ROTATE_CW;
		}
		else if (t.GetString() == "CCW")
		{
			t.Next();
			*Flags |= FLAG_ROTATE_CCW;
		}
		else if (t.GetString() == "UD")
		{
			t.Next();
			*Flags |= FLAG_ROTATE_UD;
		}
		else if (t.GetString() == "H")
		{
			t.Next();
			*Flags |= FLAG_FLIP_H;
		}
		else if (t.GetString() == "V")
		{
			t.Next();
			*Flags |= FLAG_FLIP_V;
		}
		else
			break;
	}
}

void SpriteRepository_pimpl::ReadComplexElementData(Tokenizer &t, uint4 *Index, float *Width, float *Height, uint4 *Flags)
{
	// Index
	*Index = t.MustGetUint4();
	t.Next();

	// Szerokoœæ
	*Width = t.MustGetFloat();
	t.Next();
	if (t.QuerySymbol('%'))
	{
		t.Next();
		*Width = - (*Width) * 0.01f;
	}

	// Wysokoœæ
	*Height = t.MustGetFloat();
	t.Next();
	if (t.QuerySymbol('%'))
	{
		t.Next();
		*Height = - (*Height) * 0.01f;
	}

	// Flagi
	ParseFlags(t, Flags);
}

void SpriteRepository_pimpl::ReadComplexElementData2(Tokenizer &t, uint4 *Index, uint4 *Flags)
{
	// Index
	*Index = t.MustGetUint4();
	t.Next();

	// Flags
	ParseFlags(t, Flags);
}

Sprite * SpriteRepository_pimpl::CreateFromDocument(const string &Name, Tokenizer &t)
{
	string s;

	// Efekt
	t.AssertIdentifier("effect");
	t.Next();
	t.AssertToken(Tokenizer::TOKEN_STRING);
	string EffectName = t.GetString();
	t.Next();

	// Tekstura
	t.AssertIdentifier("texture");
	t.Next();
	t.AssertToken(Tokenizer::TOKEN_STRING);
	string TextureName = t.GetString();
	t.Next();

	// Utwórz
	Sprite *sprite = new Sprite(TextureName, EffectName);

	try
	{
		while (!t.QuerySymbol('}') && !t.QueryEOF())
		{
			t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
			if (t.GetString() == "matrix")
			{
				t.Next();

				t.AssertSymbol('{');
				t.Next();

				uint ColCount = t.MustGetUint4();
				t.Next();

				uint ElementWidth = t.MustGetUint4();
				t.Next();

				uint ElementHeight = t.MustGetUint4();
				t.Next();

				uint SpaceX = 0, SpaceY = 0, StartX = 0, StartY = 0;
				if (!t.QuerySymbol('}'))
				{
					SpaceX = t.MustGetUint4();
					t.Next();

					SpaceY = t.MustGetUint4();
					t.Next();

					if (!t.QuerySymbol('}'))
					{
						StartX = t.MustGetUint4();
						t.Next();

						StartY = t.MustGetUint4();
						t.Next();
					}
				}

				t.AssertSymbol('}');
				t.Next();

				sprite->SetSimpleElementsMatrix(ColCount, ElementWidth, ElementHeight, StartX, StartY, SpaceX, SpaceY);
			}
			else if (t.GetString() == "rects")
			{
				t.Next();

				sprite->SetSimpleElementsCustomRects();

				t.AssertSymbol('{');
				t.Next();

				uint4 Index;
				RECTI Recti;
				while (!t.QuerySymbol('}'))
				{
					// Indeks
					Index = t.MustGetUint4();
					t.Next();

					// Nawias
					t.AssertSymbol('{');
					t.Next();
					
					// left
					t.AssertIdentifier("left");
					t.Next();
					t.AssertSymbol('=');
					t.Next();
					Recti.left = t.MustGetInt4();
					t.Next();

					// top
					t.AssertIdentifier("top");
					t.Next();
					t.AssertSymbol('=');
					t.Next();
					Recti.top = t.MustGetInt4();
					t.Next();

					// right, bottom
					if (t.QueryIdentifier("right"))
					{
						t.Next();
						t.AssertSymbol('=');
						t.Next();
						Recti.right = t.MustGetInt4();
						t.Next();

						// bottom
						t.AssertIdentifier("bottom");
						t.Next();
						t.AssertSymbol('=');
						t.Next();
						Recti.bottom = t.MustGetInt4();
						t.Next();
					}
					// width, height
					else
					{
						t.AssertIdentifier("width");
						t.Next();
						t.AssertSymbol('=');
						t.Next();
						Recti.right = Recti.left + t.MustGetInt4();
						t.Next();

						// height
						t.AssertIdentifier("height");
						t.Next();
						t.AssertSymbol('=');
						t.Next();
						Recti.bottom = Recti.top + t.MustGetInt4();
						t.Next();
					}

					t.AssertSymbol('}');
					t.Next();

					sprite->AddSimpleElementsCustomRect(Index, Recti);
				}

				t.AssertSymbol('}');
				t.Next();
			}
			else if (t.GetString() == "complex")
			{
				t.Next();

				t.AssertSymbol('{');
				t.Next();

				uint4 Index;
				Sprite::COMPLEX_ELEMENT ce;
				while (!t.QuerySymbol('}'))
				{
					// Inicjalizacja
					ce.CenterIndex = gfx2d::INDEX_NONE; ce.CenterFlags = 0; ce.CenterWidth = 0.0f; ce.CenterHeight = 0.0f;
					ce.LeftIndex = gfx2d::INDEX_NONE; ce.LeftFlags = 0; ce.LeftWidth = 0.0f; ce.LeftHeight = 0.0f;
					ce.RightIndex = gfx2d::INDEX_NONE; ce.RightFlags = 0; ce.RightWidth = 0.0f; ce.RightHeight = 0.0f;
					ce.TopIndex = gfx2d::INDEX_NONE; ce.TopFlags = 0; ce.TopWidth = 0.0f; ce.TopHeight = 0.0f;
					ce.BottomIndex = gfx2d::INDEX_NONE; ce.BottomFlags = 0; ce.BottomWidth = 0.0f; ce.BottomHeight = 0.0f;
					ce.LeftTopIndex = gfx2d::INDEX_NONE; ce.LeftTopFlags = 0;
					ce.RightTopIndex = gfx2d::INDEX_NONE; ce.RightTopFlags = 0;
					ce.LeftBottomIndex = gfx2d::INDEX_NONE; ce.LeftBottomFlags = 0;
					ce.RightBottomIndex = gfx2d::INDEX_NONE; ce.RightBottomFlags = 0;

					// Indeks
					Index = t.MustGetUint4();
					t.Next();

					// Pocz¹tek
					t.AssertSymbol('{');
					t.Next();

					// center
					t.AssertIdentifier("center");
					t.Next();
					t.AssertSymbol('=');
					t.Next();
					ReadComplexElementData(t, &ce.CenterIndex, &ce.CenterWidth, &ce.CenterHeight, &ce.CenterFlags);
					// Dalsze dane
					while (!t.QuerySymbol('}'))
					{
						t.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
						if (t.GetString() == "left")
						{
							t.Next();
							t.AssertSymbol('=');
							t.Next();
							ReadComplexElementData(t, &ce.LeftIndex, &ce.LeftWidth, &ce.LeftHeight, &ce.LeftFlags);
						}
						else if (t.GetString() == "right")
						{
							t.Next();
							t.AssertSymbol('=');
							t.Next();
							ReadComplexElementData(t, &ce.RightIndex, &ce.RightWidth, &ce.RightHeight, &ce.RightFlags);
						}
						else if (t.GetString() == "top")
						{
							t.Next();
							t.AssertSymbol('=');
							t.Next();
							ReadComplexElementData(t, &ce.TopIndex, &ce.TopWidth, &ce.TopHeight, &ce.TopFlags);
						}
						else if (t.GetString() == "bottom")
						{
							t.Next();
							t.AssertSymbol('=');
							t.Next();
							ReadComplexElementData(t, &ce.BottomIndex, &ce.BottomWidth, &ce.BottomHeight, &ce.BottomFlags);
						}
						else if (t.GetString() == "left_top")
						{
							t.Next();
							t.AssertSymbol('=');
							t.Next();
							ReadComplexElementData2(t, &ce.LeftTopIndex, &ce.LeftTopFlags);
						}
						else if (t.GetString() == "right_top")
						{
							t.Next();
							t.AssertSymbol('=');
							t.Next();
							ReadComplexElementData2(t, &ce.RightTopIndex, &ce.RightTopFlags);
						}
						else if (t.GetString() == "left_bottom")
						{
							t.Next();
							t.AssertSymbol('=');
							t.Next();
							ReadComplexElementData2(t, &ce.LeftBottomIndex, &ce.LeftBottomFlags);
						}
						else if (t.GetString() == "right_bottom")
						{
							t.Next();
							t.AssertSymbol('=');
							t.Next();
							ReadComplexElementData2(t, &ce.RightBottomIndex, &ce.RightBottomFlags);
						}
						else
							t.CreateError();
					}
					t.Next();

					sprite->AddComplexElement(Index, ce);
				}
				t.Next();
			}
			else
				t.CreateError("Oczekiwano \"matrix\" lub \"rects\" lub \"complex\".");
		}
		// Koñcz¹cego tokena, czy to EOF czy '}', ju¿ nie parsujemy
	}
	catch (...)
	{
		delete sprite;
		throw;
	}

	Create(Name, sprite);
	return sprite;
}


SpriteRepository::SpriteRepository() :
	pimpl(new SpriteRepository_pimpl)
{
}

SpriteRepository::~SpriteRepository()
{
}

void SpriteRepository::CreateSpritesFromFile(const string &FileName)
{
	ERR_TRY;

	FileStream file(FileName, FM_READ);
	Tokenizer t(&file, 0);
	t.Next();

	string Name;
	while (t.GetToken() != Tokenizer::TOKEN_EOF)
	{
		// Nazwa
		t.AssertToken(Tokenizer::TOKEN_STRING);
		Name = t.GetString();
		t.Next();

		// Nawias
		t.AssertSymbol('{');
		t.Next();

		// Parametry
		pimpl->CreateFromDocument(Name, t);

		// Nawias
		t.AssertSymbol('}');
		t.Next();
	}

	ERR_CATCH("Nie mo¿na wczytaæ spritów z pliku: " + FileName);
}

Sprite * SpriteRepository::CreateSpriteFromString(const string &Name, const string &Params)
{
	ERR_TRY;

	Tokenizer t(&Params, 0);
	return pimpl->CreateFromDocument(Name, t);

	ERR_CATCH("gfx2d::SpriteRepository::CreateSpriteFromString: B³¹d podczas parsowania definicji sprita: " + Name);
}

void SpriteRepository::CreateSprite(const string &name, Sprite *sprite)
{
	pimpl->Create(name, sprite);
}

bool SpriteRepository::DestroySprite(const string &name)
{
	SpriteRepository_pimpl::SPRITE_MAP::iterator it = pimpl->m_Sprites.find(name);
	if (it == pimpl->m_Sprites.end())
		return false;
	pimpl->m_Sprites.erase(it);
	return true;
}

bool SpriteRepository::DestroySprite(Sprite *sprite)
{
	for (SpriteRepository_pimpl::SPRITE_MAP::iterator it = pimpl->m_Sprites.begin();
		it != pimpl->m_Sprites.end();
		++it)
	{
		if (it->second.get() == sprite)
		{
			pimpl->m_Sprites.erase(it);
			return true;
		}
	}
	return false;
}

void SpriteRepository::MustDestroySprite(const string &name)
{
	if (!DestroySprite(name))
		throw Error("Nie mo¿na usun¹æ sprita o nazwie: " + name, __FILE__, __LINE__);
}

void SpriteRepository::MustDestroySprite(Sprite *sprite)
{
	if (!DestroySprite(sprite))
		throw Error("Nie mo¿na usun¹æ sprita.", __FILE__, __LINE__);
}

Sprite * SpriteRepository::GetSprite(const string &name)
{
	SpriteRepository_pimpl::SPRITE_MAP::iterator it = pimpl->m_Sprites.find(name);
	if (it == pimpl->m_Sprites.end())
		return 0;
	return it->second.get();
}

Sprite * SpriteRepository::MustGetSprite(const string &name)
{
	SpriteRepository_pimpl::SPRITE_MAP::iterator it = pimpl->m_Sprites.find(name);
	if (it == pimpl->m_Sprites.end())
		throw Error("gfx2d::SpriteRepository::MustGetSprite: Nie znaleziono sprita o nazwie: " + name, __FILE__, __LINE__);
	return it->second.get();
}

SpriteRepository *g_SpriteRepository = NULL;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Canvas

ID3DXEffect * Canvas::GetPlainColorEffect()
{
	if (m_PlainColorEffect == 0)
		m_PlainColorEffect = res::g_Manager->MustGetResourceEx<res::D3dEffect>("PlainColor2d");
	
	m_PlainColorEffect->Load();
	
	return m_PlainColorEffect->Get();
}

ID3DXEffect * Canvas::GetFontEffect()
{
	if (m_FontEffect == 0)
		m_FontEffect = res::g_Manager->MustGetResourceEx<res::D3dEffect>("FontEffect");
	
	m_FontEffect->Load();
	
	return m_FontEffect->Get();
}

void Canvas::AddRect(const RECTF &rect, SPRITE_VERTEX v[])
{
	// Jest clipping
	RECTF ClipRect;
	if (GetCurrentClipRect(&ClipRect))
	{
		if (!OverlapRect(rect, ClipRect))
			return;

		// Quad le¿y ca³kowicie w zakresie - idzie dalej od razu
		// Nie le¿y - wchodzi tutaj i modyfikuje t¹ tablicê, po czym idzie dalej
		if (!RectInRect(rect, ClipRect))
		{
			// ZnajdŸ czêœæ wspóln¹
			// Nie wiem jak to mo¿liwe ¿eby tutaj Intersection zwróci³o false (chyba ¿e jest kwestia
			// ostrych/nieostrych nierównoœci), ale jak wrzuci³em asercjê zamiast returna to zdarza³o
			// mu siê wywaliæ.
			RECTF is;
			if (!Intersection(&is, rect, ClipRect))
				return;

			// == Wersja 1 ==
			// To tutaj jest stare i dzia³a Ÿle dla obróconych tekstur, bo zak³ada, ¿e
			// wspó³rzêdna Tex.x wzrasta wraz z Pos.x, a Tex.y wraz z Pos.y.

			// ZnajdŸ wspó³rzêdne tekstury (przez znalezienie wspó³czynników funkcji liniowej)
			//float tx_a, tx_b, ty_a, ty_b;

			//tx_a = (v[1].Tex.x - v[0].Tex.x) / (v[1].Pos.x - v[0].Pos.x);
			//tx_b = v[0].Tex.x - tx_a * v[0].Pos.x;
			//ty_a = (v[2].Tex.y - v[0].Tex.y) / (v[2].Pos.y - v[0].Pos.y);
			//ty_b = v[0].Tex.y - ty_a * v[0].Pos.y;
			//
			//// Uaktualnij pozycje wierzcho³ków
			//v[0].Pos.x = v[2].Pos.x = is.left;
			//v[1].Pos.x = v[3].Pos.x = is.right;
			//v[0].Pos.y = v[1].Pos.y = is.top;
			//v[2].Pos.y = v[3].Pos.y = is.bottom;

			//// Uaktualnij wspó³rzêdne tekstury
			//v[0].Tex.x = v[2].Tex.x = tx_a * v[0].Pos.x + tx_b;
			//v[1].Tex.x = v[3].Tex.x = tx_a * v[1].Pos.x + tx_b;
			//v[0].Tex.y = v[1].Tex.y = ty_a * v[0].Pos.y + ty_b;
			//v[2].Tex.y = v[3].Tex.y = ty_a * v[2].Pos.y + ty_b;

			// == Wersja 2 ==
			// Tutaj do znalezienia jest macierz przekszta³caj¹ca wspó³rzêdne Pos na Tex, o taka:
			//             | a11, a12, 0 |
			// [x, y, 1] * | a21, a22, 0 | = [tx, ty, 1]
			//             | a32, a32, 1 |

			// Metod¹ podstawiania, za pomoc¹ kartki i d³ugopisu, w wyniku rozwi¹zania uk³adu
			// 6 równañ z 6 niewiadomymi (przy wykorzystaniu 3 spoœród 4 danych wierzcho³ków, ich Pos i Tex)
			// otrzymano (ten kod ju¿ dzia³a):

			//// ZnajdŸ wspó³rzêdne tekstury (przez znalezienie wspó³czynników macierzy)
			//float x1 = v[0].Pos.x; float y1 = v[0].Pos.y;
			//float x2 = v[1].Pos.x; float y2 = v[1].Pos.y;
			//float x3 = v[2].Pos.x; float y3 = v[2].Pos.y;
			//float tx1 = v[0].Tex.x; float ty1 = v[0].Tex.y;
			//float tx2 = v[1].Tex.x; float ty2 = v[1].Tex.y;
			//float tx3 = v[2].Tex.x; float ty3 = v[2].Tex.y;

			//float a21 = ( (x2-x1)*(tx3-tx1) - (x3-x1)*(tx2-tx1) ) / ( (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1) );
			//float a22 = ( (x2-x1)*(ty3-ty1) - (x3-x1)*(ty2-ty1) ) / ( (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1) );
			//float a11 = ( tx2-tx1-a21*(y2-y1) ) / (x2-x1);
			//float a12 = ( ty2-ty1-a22*(y2-y1) ) / (x2-x1);
			//float a31 = tx1 - a11*x1 - a21*y1;
			//float a32 = ty1 - a12*x1 - a22*y1;

			//// Uaktualnij pozycje wierzcho³ków
			//v[0].Pos.x = v[2].Pos.x = is.left;
			//v[1].Pos.x = v[3].Pos.x = is.right;
			//v[0].Pos.y = v[1].Pos.y = is.top;
			//v[2].Pos.y = v[3].Pos.y = is.bottom;

			//// Uaktualnij wspó³rzêdne tekstury
			//v[0].Tex.x = a11*v[0].Pos.x + a21*v[0].Pos.y + a31;
			//v[0].Tex.y = a12*v[0].Pos.x + a22*v[0].Pos.y + a32;
			//v[1].Tex.x = a11*v[1].Pos.x + a21*v[1].Pos.y + a31;
			//v[1].Tex.y = a12*v[1].Pos.x + a22*v[1].Pos.y + a32;
			//v[2].Tex.x = a11*v[2].Pos.x + a21*v[2].Pos.y + a31;
			//v[2].Tex.y = a12*v[2].Pos.x + a22*v[2].Pos.y + a32;
			//v[3].Tex.x = a11*v[3].Pos.x + a21*v[3].Pos.y + a31;
			//v[3].Tex.y = a12*v[3].Pos.x + a22*v[3].Pos.y + a32;

			// == Wersja 2 zoptymalizowana ==

			// ZnajdŸ wspó³rzêdne tekstury (przez znalezienie wspó³czynników macierzy)
			float TmpDiv1 = (v[1].Pos.x-v[0].Pos.x)*(v[2].Pos.y-v[0].Pos.y) - (v[2].Pos.x-v[0].Pos.x)*(v[1].Pos.y-v[0].Pos.y);
			float TmpMul1 = v[1].Pos.x-v[0].Pos.x;
			float TmpMul2 = v[2].Pos.x-v[0].Pos.x;
			float a21 = ( TmpMul1*(v[2].Tex.x-v[0].Tex.x) - TmpMul2*(v[1].Tex.x-v[0].Tex.x) ) / TmpDiv1;
			float a22 = ( TmpMul1*(v[2].Tex.y-v[0].Tex.y) - TmpMul2*(v[1].Tex.y-v[0].Tex.y) ) / TmpDiv1;
			float TmpDiv2 = v[1].Pos.x-v[0].Pos.x;
			float TmpMul3 = v[1].Pos.y-v[0].Pos.y;
			float a11 = ( v[1].Tex.x-v[0].Tex.x-a21*TmpMul3 ) / TmpDiv2;
			float a12 = ( v[1].Tex.y-v[0].Tex.y-a22*TmpMul3 ) / TmpDiv2;
			float a31 = v[0].Tex.x - a11*v[0].Pos.x - a21*v[0].Pos.y;
			float a32 = v[0].Tex.y - a12*v[0].Pos.x - a22*v[0].Pos.y;

			// Uaktualnij pozycje wierzcho³ków
			v[0].Pos.x = v[2].Pos.x = is.left;
			v[1].Pos.x = v[3].Pos.x = is.right;
			v[0].Pos.y = v[1].Pos.y = is.top;
			v[2].Pos.y = v[3].Pos.y = is.bottom;

			// Uaktualnij wspó³rzêdne tekstury
			v[0].Tex.x = a11*v[0].Pos.x + a21*v[0].Pos.y + a31;
			v[0].Tex.y = a12*v[0].Pos.x + a22*v[0].Pos.y + a32;
			v[1].Tex.x = a11*v[1].Pos.x + a21*v[1].Pos.y + a31;
			v[1].Tex.y = a12*v[1].Pos.x + a22*v[1].Pos.y + a32;
			v[2].Tex.x = a11*v[2].Pos.x + a21*v[2].Pos.y + a31;
			v[2].Tex.y = a12*v[2].Pos.x + a22*v[2].Pos.y + a32;
			v[3].Tex.x = a11*v[3].Pos.x + a21*v[3].Pos.y + a31;
			v[3].Tex.y = a12*v[3].Pos.x + a22*v[3].Pos.y + a32;
		}
	}
	// Nie ma clippingu lub ca³kowicie wewn¹trz - przepisz tak jak jest

	uint4 Index = m_SpriteVB.size();

	m_SpriteVB.push_back(v[0]);
	m_SpriteVB.push_back(v[1]);
	m_SpriteVB.push_back(v[2]);
	m_SpriteVB.push_back(v[3]);

	m_SpriteIB.push_back(Index+0);
	m_SpriteIB.push_back(Index+1);
	m_SpriteIB.push_back(Index+2);
	m_SpriteIB.push_back(Index+2);
	m_SpriteIB.push_back(Index+1);
	m_SpriteIB.push_back(Index+3);

	m_SpriteQuadCount++;
}

void Canvas::GenerateComplexElementRect(const VEC2 &CurrentTranslation, SPRITE_VERTEX v[], const RECTF &rect, uint4 Index, uint4 Flags, float Width, float Height)
{
	if (!rect.IsValid())
		return;

	PushClipRect(rect);
	RECTF RealRect = rect + CurrentTranslation;

	float TexLeft, TexRight, TexTop, TexBottom;
	m_CurrentSprite->GetTexCoords(Index, &TexLeft, &TexRight, &TexTop, &TexBottom);

	// Obliczenie prawdziwej szerokoœci i wysokoœci pojedynczego powtórzenia w jednostkach
	if (Width == 0.0f)
		Width = RealRect.Width();
	else if (Width < 0.0f)
		Width = -Width * RealRect.Width();
	if (Height == 0.0f)
		Height = RealRect.Height();
	else if (Height < 0.0f)
		Height = -Height * RealRect.Height();

	// Narysowanie wszystkich powtórzeñ
	float x, y;
	for (x = RealRect.left; x < RealRect.right; x += Width)
	{
		for (y = RealRect.top; y < RealRect.bottom; y += Height)
		{
			// To tutaj, a nie na zewn¹trz, bo AddRect modyfikuje sobie tak Pos jak i Tex w tablicy v podczas obcinania.
			m_CurrentSprite->ProcessFlags(&v[0].Tex, &v[1].Tex, &v[2].Tex, &v[3].Tex, TexLeft, TexTop, TexRight, TexBottom, Flags);
			v[0].Pos = VEC3(x, y, 1.0f);
			v[1].Pos = VEC3(x+Width, y, 1.0f);
			v[2].Pos = VEC3(x, y+Height, 1.0f);
			v[3].Pos = VEC3(x+Width, y+Height, 1.0f);
			AddRect(RECTF(x, y, x+Width, y+Height), v);
		}
	}

	PopClipRect();
}

Canvas::Canvas() :
	m_CurrentColor(COLOR::WHITE),
	m_CurrentSprite(0),
	m_SpriteQuadCount(0),
	m_PlainColorEffect(0),
	m_FontEffect(0),
	m_CurrentFont(0),
	m_Action(ACTION_NONE)
{
}

void Canvas::OnCreate()
{
}

void Canvas::OnDestroy()
{
	// Zakomentowa³em, bo kiedy podczas rysowania wyst¹pi b³¹d to bêd¹ nieopró¿nione,
	// a mimo tego nie powinno siê wysypaæ tylko wy³¹czyæ
	//assert( (m_SpriteVB.empty() && m_SpriteIB.empty()) && "Unflushed data in sprite buffer." );

	m_SpriteIB.clear();
	m_SpriteVB.clear();
	m_SpriteQuadCount = 0;
}

void Canvas::OnLostDevice()
{
	// Zakomentowa³em, bo kiedy podczas rysowania wyst¹pi b³¹d to bêd¹ nieopró¿nione,
	// a mimo tego nie powinno siê wysypaæ tylko wy³¹czyæ
	//assert( (m_SpriteVB.empty() && m_SpriteIB.empty()) && "Unflushed data in sprite buffer." );

	m_SpriteIB.clear();
	m_SpriteVB.clear();
	m_SpriteQuadCount = 0;
}

void Canvas::OnResetDevice()
{
}

void Canvas::BeginAction()
{
	ERR_TRY;

	switch (m_Action)
	{
	case ACTION_TEXT:
		{
			ID3DXEffect *FontEffect = GetFontEffect();
			UINT Foo;
			FontEffect->Begin(&Foo, D3DXFX_DONOTSAVESTATE);
			FontEffect->SetTexture(FontEffect->GetParameterByName(0, "Texture1"), m_CurrentFont->GetTexture());
			FontEffect->BeginPass(0);
			m_FontEffectColor = FontEffect->GetParameterByName(0, "Color1");
			m_FontEffectTexture = FontEffect->GetParameterByName(0, "Texture1");
		}
		break;
	}

	ERR_CATCH_FUNC;
}

void Canvas::EndAction()
{
	ERR_TRY;

	switch (m_Action)
	{
	case ACTION_SPRITES:
		if (m_SpriteQuadCount > 0)
		{
			ID3DXEffect *Effect;
			UINT Passes;

			// Jednolity kolor
			if (m_CurrentSprite == 0)
			{
				Effect = GetPlainColorEffect();
				Effect->Begin(&Passes, D3DXFX_DONOTSAVESTATE);
				Effect->BeginPass(0);
			}
			// Sprite
			else
			{
				assert(m_CurrentSprite->m_Texture);
				assert(m_CurrentSprite->m_Effect);

				m_CurrentSprite->m_Texture->Load();
				m_CurrentSprite->m_Effect->Load();

				UINT Passes;
				Effect = m_CurrentSprite->m_Effect->Get();
				Effect->SetTexture(Effect->GetParameterByName(0, "Texture"), m_CurrentSprite->m_Texture->GetTexture());
				Effect->Begin(&Passes, D3DXFX_DONOTSAVESTATE);
				Effect->BeginPass(0);
			}

			// Narysuj
			UINT PrimitiveCount = m_SpriteQuadCount * 2;
			frame::Dev->SetFVF(SPRITE_VERTEX::FVF);
			HRESULT hr = frame::Dev->DrawIndexedPrimitiveUP(
				D3DPT_TRIANGLELIST,
				0,
				m_SpriteVB.size(),
				PrimitiveCount,
				&m_SpriteIB[0],
				D3DFMT_INDEX16,
				&m_SpriteVB[0],
				sizeof(SPRITE_VERTEX));
			if (FAILED(hr)) throw DirectXError(hr, "Nie mo¿na narysowaæ spritów", __FILE__, __LINE__);
			frame::RegisterDrawCall(PrimitiveCount);

			// Koniec rysowania
			Effect->EndPass();
			Effect->End();

			m_SpriteIB.clear();
			m_SpriteVB.clear();
			m_SpriteQuadCount = 0;
		}
		break;
	case ACTION_TEXT:
		{
			ID3DXEffect *FontEffect = GetFontEffect();
			FontEffect->EndPass();
			FontEffect->End();
		}
		break;
	}

	ERR_CATCH_FUNC;
}

void Canvas::StartAction(ACTION NewAction)
{
	if (NewAction != m_Action)
	{
		EndAction();
		m_Action = NewAction;
		BeginAction();
	}
}

void Canvas::Flush()
{
	EndAction();
	m_Action = ACTION_NONE;
	BeginAction();
}

bool Canvas::GetCurrentClipRect(RECTF *Out)
{
	if (m_ClipRects.empty())
		return false;
	if (m_ClipRectsEnabled.top() == false)
		return false;
	*Out = m_ClipRects.top();
	return true;
}

float Canvas::GetCurrentAlpha()
{
	if (m_Alphas.empty())
		return 1.0f;
	else
		return m_Alphas.top();
}

void Canvas::GetCurrentTranslation(VEC2 *Out)
{
	if (m_Translations.empty())
		*Out = VEC2::ZERO;
	else
		*Out = m_Translations.top();
}

void Canvas::SetColor(COLOR color)
{
	m_CurrentColor = color;
}

void Canvas::SetSprite(Sprite *sprite)
{
	if (sprite != m_CurrentSprite)
	{
		if (m_Action == ACTION_SPRITES && sprite != m_CurrentSprite)
			Flush();
		m_CurrentSprite = sprite;
	}
}

void Canvas::SetFont(res::Font *font)
{
	if (font != m_CurrentFont)
	{
		if (m_Action == ACTION_TEXT)
			Flush();
		m_CurrentFont = font;
	}
}

void Canvas::PushClipRect(const RECTF &rect)
{
	assert(m_ClipRects.size() == m_ClipRectsEnabled.size());
	assert(m_ClipRects.size() < 1000);

	VEC2 t; GetCurrentTranslation(&t);
	RECTF real_rect = RECTF(rect.left+t.x, rect.top+t.y, rect.right+t.x, rect.bottom+t.y);

	if (m_ClipRects.empty())
		m_ClipRects.push(real_rect);
	else
	{
		if (m_ClipRectsEnabled.top() == false)
			m_ClipRects.push(real_rect);
		else
		{
			RECTF NewRect;
			if (Intersection(&NewRect, m_ClipRects.top(), real_rect))
				m_ClipRects.push(NewRect);
			else
				// Brak czêœci wspólnej
				m_ClipRects.push(RECTF::ZERO);
		}
	}

	m_ClipRectsEnabled.push(true);
}

void Canvas::PushClipCancel()
{
	assert(m_ClipRects.size() == m_ClipRectsEnabled.size());
	assert(m_ClipRects.size() < 1000);

	m_ClipRects.push(RECTF::ZERO);
	m_ClipRectsEnabled.push(false);
}

void Canvas::PopClipRect()
{
	assert(!m_ClipRects.empty());

	m_ClipRectsEnabled.pop();
	m_ClipRects.pop();
}

void Canvas::PushTranslation(const VEC2 &v)
{
	assert(m_Translations.size() < 1000);

	if (m_Translations.empty())
		m_Translations.push(v);
	else
		m_Translations.push(m_Translations.top() + v);
}

void Canvas::PushTranslationCancel()
{
	assert(m_Translations.size() < 1000);

	m_Translations.push(VEC2::ZERO);
}

void Canvas::PopTranslation()
{
	assert(!m_Translations.empty());

	m_Translations.pop();
}

void Canvas::PushAlpha(float Alpha)
{
	assert(m_Alphas.size() < 1000);

	if (m_Alphas.empty())
		m_Alphas.push(Alpha);
	else
		m_Alphas.push(m_Alphas.top() * Alpha);
}

void Canvas::PushAlphaCancel()
{
	assert(m_Alphas.size() < 1000);

	m_Alphas.push(1.0f);
}

void Canvas::PopAlpha()
{
	assert(!m_Alphas.empty());

	m_Alphas.pop();
}

void Canvas::DrawRect(const RECTF &rect, uint4 Index)
{
	ERR_TRY;

	StartAction(ACTION_SPRITES);

	SPRITE_VERTEX v[4];

	// uzupe³nij kolor
	m_CurrentColor.A = uint1(GetCurrentAlpha() * 255.0f);
	v[0].Color = v[1].Color = v[2].Color = v[3].Color = m_CurrentColor;

	VEC2 t; GetCurrentTranslation(&t);

	// Jednolity kolor
	if (m_CurrentSprite == 0)
	{
		// Tex zostaje niezainicjalizowane
		v[0].Pos = VEC3(t.x+rect.left, t.y+rect.top, 1.0f);
		v[1].Pos = VEC3(t.x+rect.right, t.y+rect.top, 1.0f);
		v[2].Pos = VEC3(t.x+rect.left, t.y+rect.bottom, 1.0f);
		v[3].Pos = VEC3(t.x+rect.right, t.y+rect.bottom, 1.0f);
		AddRect(RECTF(rect.left+t.x, rect.top+t.y, rect.right+t.x, rect.bottom+t.y), v);
	}
	// Sprite
	else
	{
		// Nic
		if (Index == INDEX_NONE)
			return;
		// Ca³oœæ lub element prosty
		else if (Index == INDEX_ALL || Index < 1000000000)
		{
			float TexLeft, TexRight, TexTop, TexBottom;
			m_CurrentSprite->GetTexCoords(Index, &TexLeft, &TexRight, &TexTop, &TexBottom);
			v[0].Pos = VEC3(t.x+rect.left, t.y+rect.top, 1.0f);
			v[1].Pos = VEC3(t.x+rect.right, t.y+rect.top, 1.0f);
			v[2].Pos = VEC3(t.x+rect.left, t.y+rect.bottom, 1.0f);
			v[3].Pos = VEC3(t.x+rect.right, t.y+rect.bottom, 1.0f);
			v[0].Tex = VEC2(TexLeft, TexTop);
			v[1].Tex = VEC2(TexRight, TexTop);
			v[2].Tex = VEC2(TexLeft, TexBottom);
			v[3].Tex = VEC2(TexRight, TexBottom);
			AddRect(RECTF(rect.left+t.x, rect.top+t.y, rect.right+t.x, rect.bottom+t.y), v);
		}
		// Element z³o¿ony
		else
		{
			Index -= 1000000000;
			assert(Index < m_CurrentSprite->m_ComplexElements.size());
			Sprite::COMPLEX_ELEMENT & ce = m_CurrentSprite->m_ComplexElements[Index];

			// Czy s¹ poszczególne paski
			bool IsLeft = ce.LeftIndex != INDEX_NONE && ce.LeftWidth != 0.0f;
			bool IsRight = ce.RightIndex != INDEX_NONE && ce.RightWidth != 0.0f;
			bool IsTop = ce.TopIndex != INDEX_NONE && ce.TopHeight != 0.0f;
			bool IsBottom = ce.BottomIndex != INDEX_NONE && ce.BottomHeight != 0.0f;

			// Rozmiary pasków
			float LeftWidth = 0.0f, RightWidth = 0.0f, TopHeight = 0.0f, BottomHeight = 0.0f;
			if (IsLeft)
			{
				if (ce.LeftWidth > 0.0f)
					LeftWidth = ce.LeftWidth;
				else
					LeftWidth = -ce.LeftWidth * rect.Width();
			}
			if (IsRight)
			{
				if (ce.RightWidth > 0.0f)
					RightWidth = ce.RightWidth;
				else
					RightWidth = -ce.RightWidth * rect.Width();
			}
			if (IsTop)
			{
				if (ce.TopHeight > 0.0f)
					TopHeight = ce.TopHeight;
				else
					TopHeight = -ce.TopHeight * rect.Height();
			}
			if (IsBottom)
			{
				if (ce.BottomHeight > 0.0f)
					BottomHeight = ce.BottomHeight;
				else
					BottomHeight = -ce.BottomHeight * rect.Height();
			}

			// Wygenerowanie prostok¹tów elementu z³o¿onego

			if (IsLeft)
				GenerateComplexElementRect(t, v,
					RECTF(rect.left, rect.top+TopHeight, rect.left+LeftWidth, rect.bottom-BottomHeight),
					ce.LeftIndex,
					ce.LeftFlags,
					0.0f,
					ce.LeftHeight);
			if (IsRight)
				GenerateComplexElementRect(t, v,
					RECTF(rect.right-RightWidth, rect.top+TopHeight, rect.right, rect.bottom-BottomHeight),
					ce.RightIndex,
					ce.RightFlags,
					0.0f,
					ce.RightHeight);
			if (IsTop)
				GenerateComplexElementRect(t, v,
					RECTF(rect.left+LeftWidth, rect.top, rect.right-RightWidth, rect.top+TopHeight),
					ce.TopIndex,
					ce.TopFlags,
					ce.TopWidth,
					0.0f);
			if (IsBottom)
				GenerateComplexElementRect(t, v,
					RECTF(rect.left+LeftWidth, rect.bottom-BottomHeight, rect.right-RightWidth, rect.bottom),
					ce.BottomIndex,
					ce.BottomFlags,
					ce.BottomWidth,
					0.0f);

			if (IsLeft && IsTop)
				GenerateComplexElementRect(t, v,
					RECTF(rect.left, rect.top, rect.left+LeftWidth, +rect.top+TopHeight),
					ce.LeftTopIndex,
					ce.LeftTopFlags,
					0.0f,
					0.0f);
			if (IsRight && IsTop)
				GenerateComplexElementRect(t, v,
					RECTF(+rect.right-RightWidth, +rect.top, +rect.right, +rect.top+TopHeight),
					ce.RightTopIndex,
					ce.RightTopFlags,
					0.0f,
					0.0f);
			if (IsLeft && IsBottom)
				GenerateComplexElementRect(t, v,
					RECTF(+rect.left, +rect.bottom-BottomHeight, +rect.left+LeftWidth, +rect.bottom),
					ce.LeftBottomIndex,
					ce.LeftBottomFlags,
					0.0f,
					0.0f);
			if (IsRight && IsBottom)
				GenerateComplexElementRect(t, v,
					RECTF(+rect.right-RightWidth, +rect.bottom-BottomHeight, +rect.right, +rect.bottom),
					ce.RightBottomIndex,
					ce.RightBottomFlags,
					0.0f,
					0.0f);

			GenerateComplexElementRect(t, v,
				RECTF(+rect.left+LeftWidth, +rect.top+TopHeight, +rect.right-RightWidth, +rect.bottom-BottomHeight),
				ce.CenterIndex,
				ce.CenterFlags,
				ce.CenterWidth,
				ce.CenterHeight);
		}
	}

	ERR_CATCH_FUNC;
}

void Canvas::DrawText_(float X, float Y, const string &Text, size_t TextBegin, size_t TextEnd, float Size, uint4 Flags, float Width)
{
	ERR_TRY;

	if (m_CurrentFont == 0) return;

	StartAction(ACTION_TEXT);

	// uzupe³nij kolor
	m_CurrentColor.A = uint1(GetCurrentAlpha() * 255.0f);

	// Pobierz translation i clip-rect
	VEC2 Translation;
	RECTF ClipRect;
	bool IsClipRect;
	GetCurrentTranslation(&Translation);
	IsClipRect = GetCurrentClipRect(&ClipRect);

	m_CurrentFont->Load();

	// Efekt jest rozpoczêty - uaktualnij kolor
	ID3DXEffect *FontEffect = GetFontEffect();
	FontEffect->SetValue(m_FontEffectColor, &m_CurrentColor, sizeof(DWORD));
	FontEffect->SetTexture(m_FontEffectTexture, m_CurrentFont->GetTexture());
	FontEffect->CommitChanges();

	if (IsClipRect)
	{
		m_CurrentFont->Draw(
			Translation.x + X, Translation.y + Y,
			Text, TextBegin, TextEnd, Size, Flags, Width, ClipRect);
	}
	else
	{
		m_CurrentFont->Draw(
			Translation.x + X, Translation.y + Y,
			Text, TextBegin, TextEnd, Size, Flags, Width);
	}
	
	ERR_CATCH_FUNC;
}

Canvas *g_Canvas = NULL;


} // namespace gfx2d
