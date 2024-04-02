/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
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
	CurrentY = Y;
	while (LineSplit(&LineBegin, &LineEnd, &LineWidth, &LineIndex, Text, TextBegin, TextEnd, Size, Flags, Width))
	{
		if (Flags & FLAG_HLEFT)
			StartX = CurrentX = X;
		else if (Flags & FLAG_HRIGHT)
			StartX = CurrentX = X - LineWidth;
		else // Flags & FLAG_HCENTER
			StartX = CurrentX = X - LineWidth * 0.5f;

		// Znaki
		for (i = LineBegin; i < LineEnd; i++)
		{
			CHAR_INFO & CharInfo = m_CharInfo[(int)(uint1)Text[i]];
			if (Text[i] != ' ')
			{
				RES_D3D_FONT_INC_DRAW_QUAD_FUNC(
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
				RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC(StartX, LineY1, StartX+LineWidth, LineY2);
			}
			else if (Flags & FLAG_DOUBLE)
			{
				LineY2 = CurrentY + Size;
				LineY1 = LineY2 - Size*0.2f/3.0f;
				RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC(StartX, LineY1, StartX+LineWidth, LineY2);
				LineY2 -= Size*0.1f;
				LineY1 -= Size*0.1f;
				RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC(StartX, LineY1, StartX+LineWidth, LineY2);
			}
			if (Flags & FLAG_OVERLINE)
			{
				LineY1 = CurrentY;
				LineY2 = LineY1 + Size*0.1f;
				RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC(StartX, LineY1, StartX+LineWidth, LineY2);
			}
			if (Flags & FLAG_STRIKEOUT)
			{
				LineY1 = CurrentY + Size*0.5f;
				LineY2 = LineY1 + Size*0.1f;
				RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC(StartX, LineY1, StartX+LineWidth, LineY2);
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
		CurrentY = Y - LineCount * Size;
	else // Flags & FLAG_VMIDDLE
		CurrentY = Y - LineCount * Size * 0.5f;
	// Dalej to ju¿ jak przy dosuniêciu do góry:

	for (size_t Line = 0; Line < LineCount; Line++)
	{
		if (Flags & FLAG_HLEFT)
			StartX = CurrentX = X;
		else if (Flags & FLAG_HRIGHT)
			StartX = CurrentX = X - Widths[Line];
		else // Flags & FLAG_HCENTER
			StartX = CurrentX = X - Widths[Line] * 0.5f;

		// Znaki
		for (i = Begins[Line]; i < Ends[Line]; i++)
		{
			CHAR_INFO & CharInfo = m_CharInfo[(int)(uint1)Text[i]];
			if (Text[i] != ' ')
			{
				RES_D3D_FONT_INC_DRAW_QUAD_FUNC(
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
				RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC(StartX, LineY1, StartX+Widths[Line], LineY2);
			}
			else if (Flags & FLAG_DOUBLE)
			{
				LineY2 = CurrentY + Size;
				LineY1 = LineY2 - Size*0.2f/3.0f;
				RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC(StartX, LineY1, StartX+Widths[Line], LineY2);
				LineY2 -= Size*0.1f;
				LineY1 -= Size*0.1f;
				RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC(StartX, LineY1, StartX+Widths[Line], LineY2);
			}
			if (Flags & FLAG_OVERLINE)
			{
				LineY1 = CurrentY;
				LineY2 = LineY1 + Size*0.1f;
				RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC(StartX, LineY1, StartX+Widths[Line], LineY2);
			}
			if (Flags & FLAG_STRIKEOUT)
			{
				LineY1 = CurrentY + Size*0.5f;
				LineY2 = LineY1 + Size*0.1f;
				RES_D3D_FONT_INC_DRAW_FILLED_RECT_FUNC(StartX, LineY1, StartX+Widths[Line], LineY2);
			}
		}

		CurrentY += Size;
	}
}

DrawEnd();
