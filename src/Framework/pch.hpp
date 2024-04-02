/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef PCH_H_
#define PCH_H_

//======================================
// CommonLib (ca³y!)

#include "..\Common\Base.hpp"
#include "..\Common\Math.hpp"
#include "..\Common\Config.hpp"
#include "..\Common\DateTime.hpp"
#include "..\Common\Dator.hpp"
#include "..\Common\Error.hpp"
#include "..\Common\Files.hpp"
#include "..\Common\FreeList.hpp"
#include "..\Common\Logger.hpp"
#include "..\Common\Math.hpp"
#include "..\Common\Profiler.hpp"
#include "..\Common\Stream.hpp"
#include "..\Common\Threads.hpp"
#include "..\Common\Tokenizer.hpp"
//#include "..\Common\ZlibUtils.hpp" // Na razie nie u¿ywam Zlib

using namespace common;

//======================================
// External

#include "../../doc/External/FastDelegate.h"

//======================================
// C++

#include <list>
#include <set>
#include <map>
#include <algorithm>

//======================================
// Systemowe

#include <windows.h>
#include <d3dx9.h>
// Visual Leak Detector
//#include <vld.h>

//======================================
// Mój kod

// Interpoluje dan¹ wielkoœæ miêdzy wartoœciami kluczowymi dla parametru w zakresie 0..1 z zawiniêciem.
// - Dzia³a dla typów T, dla których zdefiniowana jest funkcja interpoluj¹ca (t = 0..1):
//   void RoundInterpolatorLerp(T *Out, const T &v1, const T &v2, float t);
// - Jako parametr t, zarówno w Items jak i do metody Calc, podawaæ liczbê z zakresu 0..1.
// - Wartoœci Items mo¿na swobodnie zmieniaæ.
// - Dla pustej tablicy Items zwraca zawsze T().
template <typename T>
class RoundInterpolator
{
public:
	struct ITEM
	{
		float t;
		T Value;

		ITEM() { }
		ITEM(float t, const T &Value) : t(t), Value(Value) { }
	};

	std::vector<ITEM> Items;

	RoundInterpolator() : m_LastUsedIndex(0) { }
	void Calc(T *Out, float t) const;

private:
	mutable uint m_LastUsedIndex;
};

template <typename T>
void RoundInterpolator<T>::Calc(T *Out, float t) const
{
	assert(t >= 0.0f && t <= 1.0f);

	if (Items.empty())
	{
		*Out = T();
		return;
	}

	// ZnajdŸ index poprzedniego i nastêpnego Item

	// Na pocz¹tek weŸ zapamiêtany indeks i sprawdŸ czy jest poprawny, jeœli nie to wróæ do zerowego.
	uint Index = m_LastUsedIndex;
	// - Indeks poza zakresem
	if (Index >= Items.size())
		Index = 0;
	// - Z³y czas po lewej
	if (Items[Index].t > t)
		Index = 0;

	// PrzechodŸ dalej
	while (Index < Items.size()-1 && Items[Index+1].t < t)
		Index++;

	// Zapamiêtaj indeks
	m_LastUsedIndex = Index;

	if (Index < Items.size()-1)
	{
		float my_t = (t - Items[Index].t) / (Items[Index+1].t - Items[Index].t);
		RoundInterpolatorLerp(Out, Items[Index].Value, Items[Index+1].Value, my_t);
	}
	else
	{
		float my_t = (t - Items[Index].t) / (Items[0].t + 1.0f - Items[Index].t);
		RoundInterpolatorLerp(Out, Items[Index].Value, Items[0].Value, my_t);
	}
}

inline void RoundInterpolatorLerp(float *Out, float v1, float v2, float t) { *Out = Lerp(v1, v2, t); }
inline void RoundInterpolatorLerp(VEC2 *Out, const VEC2 &v1, const VEC2 &v2, float t) { Lerp(Out, v1, v2, t); }
inline void RoundInterpolatorLerp(VEC3 *Out, const VEC3 &v1, const VEC3 &v2, float t) { Lerp(Out, v1, v2, t); }
inline void RoundInterpolatorLerp(VEC4 *Out, const VEC4 &v1, const VEC4 &v2, float t) { Lerp(Out, v1, v2, t); }
inline void RoundInterpolatorLerp(QUATERNION *Out, const QUATERNION &v1, const QUATERNION &v2, float t) { Slerp(Out, v1, v2, t); }
inline void RoundInterpolatorLerp(COLORF *Out, const COLORF &v1, const COLORF &v2, float t) { Lerp(Out, v1, v2, t); }

typedef std::pair<COLORF, COLORF> COLORF_PAIR;
inline void RoundInterpolatorLerp(COLORF_PAIR *Out, const COLORF_PAIR &v1, const COLORF_PAIR &v2, float t)
{
	RoundInterpolatorLerp(&Out->first, v1.first, v2.first, t);
	RoundInterpolatorLerp(&Out->second, v1.second, v2.second, t);
}


#endif
