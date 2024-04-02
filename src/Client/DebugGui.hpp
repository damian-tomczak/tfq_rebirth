/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef DEBUG_GUI_H_
#define DEBUG_GUI_H_

class DebugGui_pimpl;
namespace gui
{
	class PropertyGridWindow;
}

// Klasa g��wna - reprezentuje ca�e testowe GUI aplikacji
class DebugGui
{
private:
	scoped_ptr<DebugGui_pimpl> pimpl;

public:
	DebugGui(fastdelegate::FastDelegate1<uint> GameSelectHandler);
	~DebugGui();

	// Zmieni�a si� rozdzielczo�� ekranu
	void ResolutionChanged();
	// Klawisz - reaguje i zwraca true je�li to by� skr�t z menu
	bool Shortcut(uint4 Key);
	// Schowa� menu! Np. kiedy klikni�to gdzie� na silnik.
	void HideMenu();
	// Czy statystyki maj� by� pokazane
	bool GetShowStats();
	// Czy tekstury pomocnicze maj� by� pokazane
	bool GetShowTextures();
	// Aplikacja ma wywo�uj�c t� funkcj� powiadomi� o zdarzeniu zmiany ustawie� graficznych
	void DisplaySettingsChanged(bool Succeeded);

	// Zwraca okno Propery Grid o podanym identyfikatorze.
	gui::PropertyGridWindow * GetPropertyGridWindow(uint4 Id);
};

extern DebugGui *g_DebugGui;

#endif
