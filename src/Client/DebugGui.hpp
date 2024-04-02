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

// Klasa g³ówna - reprezentuje ca³e testowe GUI aplikacji
class DebugGui
{
private:
	scoped_ptr<DebugGui_pimpl> pimpl;

public:
	DebugGui(fastdelegate::FastDelegate1<uint> GameSelectHandler);
	~DebugGui();

	// Zmieni³a siê rozdzielczoœæ ekranu
	void ResolutionChanged();
	// Klawisz - reaguje i zwraca true jeœli to by³ skrót z menu
	bool Shortcut(uint4 Key);
	// Schowaæ menu! Np. kiedy klikniêto gdzieœ na silnik.
	void HideMenu();
	// Czy statystyki maj¹ byæ pokazane
	bool GetShowStats();
	// Czy tekstury pomocnicze maj¹ byæ pokazane
	bool GetShowTextures();
	// Aplikacja ma wywo³uj¹c t¹ funkcjê powiadomiæ o zdarzeniu zmiany ustawieñ graficznych
	void DisplaySettingsChanged(bool Succeeded);

	// Zwraca okno Propery Grid o podanym identyfikatorze.
	gui::PropertyGridWindow * GetPropertyGridWindow(uint4 Id);
};

extern DebugGui *g_DebugGui;

#endif
