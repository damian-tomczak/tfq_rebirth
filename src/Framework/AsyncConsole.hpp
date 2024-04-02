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
	// Tworzy konsolê
	// Tworzyæ tylko pojedynczy obiekt tej klasy i przypisywaæ go do g_AsyncConsole.
	AsyncConsole();
	// Zamyka konsolê
	~AsyncConsole();

	// Zmienia tytu³ okna konsoli
	void SetTitle(const string &Title);
	// Zmienia bie¿¹cy kolor wyjœcia
	// Podawaæ kombinacjê sta³ych FOREGROUND_ i BACKGROUND_ z WinAPI
	// udokumentowanych w opisie funkcji SetConsoleTextAttribute.
	void SetOutputColor(uint2 Color);
	// Zmienia kolor u¿ywany do wejœcia
	void SetInputColor(uint2 Color);

	// Zapisuje podany tekst na wyjœcie konsoli
	void Write(const string &s);
	// Zapisuje linijkê tekstu na wyjœcie konsoli
	void Writeln(const string &s);

	// Zwraca true jeœli kolejka poleceñ jest pusta
	bool InputQueueEmpty();
	// Zwraca true i pierwsze polecenie z kolejki, jeœli nie jest pusta
	// Usuwa to polecenie.
	// Jesli kolejka jest pusta, zwraca false.
	bool GetInput(string *s);
	// Czeka na wejœcie zatrzumuj¹c w¹tek, który to wywo³a
	void WaitForInput(string *s);
	// Czeka na wejœcie i go ignoruje
	void WaitForInputAndIgnore() { string s; WaitForInput(&s); }
	// Zwraca uchwyt do eventa, ¿eby mo¿na sobie urz¹dziæ czekanie na niego we
	// w³asnym zakresie
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

// Tu mo¿na umieœciæ dla wygody ten pojedynczy obiekt tej klasy
extern common::scoped_ptr<AsyncConsole> g_AsyncConsole;

#endif
