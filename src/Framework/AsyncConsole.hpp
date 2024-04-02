/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef ASYNC_CONSOLE_H_
#define ASYNC_CONSOLE_H_

#include "..\Common\Logger.hpp"

typedef void* HANDLE;

class AsyncConsole_pimpl;

class AsyncConsole
{
private:
	common::scoped_ptr<AsyncConsole_pimpl> pimpl;

public:
	// Tworzy konsol�
	// Tworzy� tylko pojedynczy obiekt tej klasy i przypisywa� go do g_AsyncConsole.
	AsyncConsole();
	// Zamyka konsol�
	~AsyncConsole();

	// Zmienia tytu� okna konsoli
	void SetTitle(const string &Title);
	// Zmienia bie��cy kolor wyj�cia
	// Podawa� kombinacj� sta�ych FOREGROUND_ i BACKGROUND_ z WinAPI
	// udokumentowanych w opisie funkcji SetConsoleTextAttribute.
	void SetOutputColor(uint2 Color);
	// Zmienia kolor u�ywany do wej�cia
	void SetInputColor(uint2 Color);

	// Zapisuje podany tekst na wyj�cie konsoli
	void Write(const string &s);
	// Zapisuje linijk� tekstu na wyj�cie konsoli
	void Writeln(const string &s);

	// Zwraca true je�li kolejka polece� jest pusta
	bool InputQueueEmpty();
	// Zwraca true i pierwsze polecenie z kolejki, je�li nie jest pusta
	// Usuwa to polecenie.
	// Jesli kolejka jest pusta, zwraca false.
	bool GetInput(string *s);
	// Czeka na wej�cie zatrzumuj�c w�tek, kt�ry to wywo�a
	void WaitForInput(string *s);
	// Czeka na wej�cie i go ignoruje
	void WaitForInputAndIgnore() { string s; WaitForInput(&s); }
	// Zwraca uchwyt do eventa, �eby mo�na sobie urz�dzi� czekanie na niego we
	// w�asnym zakresie
	HANDLE GetWaitEvent();
};

class AsyncConsoleLog : public ILog
{
private:
	class Pimpl;
	scoped_ptr<Pimpl> pimpl;

protected:
	virtual void OnLog(uint4 Type, const string &Prefix, const string &TypePrefix, const string &Message);

public:
	AsyncConsoleLog();
	virtual ~AsyncConsoleLog();

	void AddColorMapping(uint4 Mask, uint2 Color);
};

// Tu mo�na umie�ci� dla wygody ten pojedynczy obiekt tej klasy
extern common::scoped_ptr<AsyncConsole> g_AsyncConsole;

#endif
