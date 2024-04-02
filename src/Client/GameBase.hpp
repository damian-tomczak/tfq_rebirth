/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#pragma once

#include "..\Framework\Framework.hpp"

namespace engine
{
	class Scene;
}

class GameBase
{
public:
	virtual ~GameBase() { }

	// Zwraca  0 - dalej trwa gra
	// Zwraca  1 - wygrana
	// Zwraca -1 - przegrana
	virtual int CalcFrame() = 0;
	virtual void Draw2D() = 0;
	virtual void OnResolutionChanged() = 0;
	virtual void OnMouseMove(const common::VEC2 &Pos) = 0;
	virtual void OnMouseButton(const common::VEC2 &Pos, frame::MOUSE_BUTTON Button, frame::MOUSE_ACTION Action) = 0;
	virtual void OnMouseWheel(const common::VEC2 &Pos, float Delta) = 0;

protected:
	// Ma zwróciæ obiekt sceny lub NULL, jeœli aktualnie nie ma
	virtual engine::Scene * GetScene() = 0;
};

extern const VEC3 SEPIA_FUNCTION_B;
extern const uint4 LOG_GAME;
