/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include <queue>
#include "AsyncConsole.hpp"



//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa AsyncConsole

class AsyncConsole_pimpl
{
public:
	AsyncConsole_pimpl();

	static const DWORD BUFFER_SIZE = 1024;

	HANDLE m_HandleIn;
	HANDLE m_HandleOut;
	CRITICAL_SECTION m_CS;
	CRITICAL_SECTION m_ReadCS;
	// Niesygnalizowany = kolejka pusta
	// Sygnalizowany = kolejka niepusta
	HANDLE m_Event;
	HANDLE m_ThreadHandle;
	WORD m_InputColor;
	WORD m_OutputColor;
	std::vector<char> m_Buffer;
	std::queue<string> m_Queue;

	static DWORD WINAPI ReadThreadProc_s(void *This);
	DWORD ReadThreadProc();
};

DWORD WINAPI AsyncConsole_pimpl::ReadThreadProc_s(void *This)
{
	return ((AsyncConsole_pimpl*)This)->ReadThreadProc();
}

DWORD AsyncConsole_pimpl::ReadThreadProc()
{
	DWORD CharactersRead;
	string s1, s2;

	while (true)
	{
		ReadConsole(m_HandleIn, &m_Buffer[0], BUFFER_SIZE, &CharactersRead, 0);

		EnterCriticalSection(&m_ReadCS);
		s1.assign(&m_Buffer[0], CharactersRead-2); // -2, bo bez koñca wiersza
		Charset_Convert(&s2, s1, CHARSET_IBM, CHARSET_WINDOWS);
		m_Queue.push(s2);
		SetEvent(m_Event);
		s1.clear();
		s2.clear();
		LeaveCriticalSection(&m_ReadCS);
	}

	return 0;
}

AsyncConsole_pimpl::AsyncConsole_pimpl() :
	m_HandleIn(0),
	m_HandleOut(0),
	m_ThreadHandle(0),
	m_Event(0)
{
}

AsyncConsole::AsyncConsole() :
	pimpl(new AsyncConsole_pimpl())
{
	pimpl->m_Buffer.resize(pimpl->BUFFER_SIZE+1);

	AllocConsole();

	pimpl->m_HandleIn = GetStdHandle(STD_INPUT_HANDLE);
	pimpl->m_HandleOut = GetStdHandle(STD_OUTPUT_HANDLE);

	InitializeCriticalSection(&pimpl->m_CS);
	InitializeCriticalSection(&pimpl->m_ReadCS);
	pimpl->m_Event = CreateEvent(0, TRUE, FALSE, 0);

	SetInputColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	SetOutputColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	pimpl->m_ThreadHandle = CreateThread(0, 0, &pimpl->ReadThreadProc_s, pimpl.get(), 0, 0);
}

AsyncConsole::~AsyncConsole()
{
	TerminateThread(pimpl->m_ThreadHandle, 0);
	CloseHandle(pimpl->m_Event);
	DeleteCriticalSection(&pimpl->m_ReadCS);
	DeleteCriticalSection(&pimpl->m_CS);
	FreeConsole();
}

void AsyncConsole::SetTitle(const string &Title)
{
	EnterCriticalSection(&pimpl->m_CS);

	SetConsoleTitle(Title.c_str());

	LeaveCriticalSection(&pimpl->m_CS);
}

void AsyncConsole::SetOutputColor(WORD Color)
{
	EnterCriticalSection(&pimpl->m_CS);

	pimpl->m_OutputColor = Color;

	LeaveCriticalSection(&pimpl->m_CS);
}

void AsyncConsole::SetInputColor(WORD Color)
{
	EnterCriticalSection(&pimpl->m_CS);

	pimpl->m_InputColor = Color;
	SetConsoleTextAttribute(pimpl->m_HandleOut, Color);

	LeaveCriticalSection(&pimpl->m_CS);
}
void AsyncConsole::Write(const string &s)
{
	string s2;
	Charset_Convert(&s2, s, CHARSET_WINDOWS, CHARSET_IBM);
	string s3;
	ReplaceEOL(&s3, s2, EOL_CRLF);

	EnterCriticalSection(&pimpl->m_CS);

	DWORD Foo;
	SetConsoleTextAttribute(pimpl->m_HandleOut, pimpl->m_OutputColor);
	WriteConsole(pimpl->m_HandleOut, s3.data(), (DWORD)s3.length(), &Foo, 0);
	SetConsoleTextAttribute(pimpl->m_HandleOut, pimpl->m_InputColor);

	LeaveCriticalSection(&pimpl->m_CS);
}

void AsyncConsole::Writeln(const string &s)
{
	string s2;
	Charset_Convert(&s2, s, CHARSET_WINDOWS, CHARSET_IBM);
	string s3;
	ReplaceEOL(&s3, s2, EOL_CRLF);

	EnterCriticalSection(&pimpl->m_CS);

	DWORD Foo;
	SetConsoleTextAttribute(pimpl->m_HandleOut, pimpl->m_OutputColor);
	WriteConsole(pimpl->m_HandleOut, s3.data(), (DWORD)s3.length(), &Foo, 0);
	WriteConsole(pimpl->m_HandleOut, "\r\n", 2, &Foo, 0);
	SetConsoleTextAttribute(pimpl->m_HandleOut, pimpl->m_InputColor);

	LeaveCriticalSection(&pimpl->m_CS);
}

bool AsyncConsole::InputQueueEmpty()
{
	EnterCriticalSection(&pimpl->m_ReadCS);
	bool R = pimpl->m_Queue.empty();
	LeaveCriticalSection(&pimpl->m_ReadCS);
	return R;
}

bool AsyncConsole::GetInput(string *s)
{
	EnterCriticalSection(&pimpl->m_ReadCS);
	bool R = pimpl->m_Queue.empty();
	if (!R)
	{
		*s = pimpl->m_Queue.front();
		pimpl->m_Queue.pop();
		if (pimpl->m_Queue.empty())
			ResetEvent(pimpl->m_Event);
	}
	LeaveCriticalSection(&pimpl->m_ReadCS);

	return !R;
}

void AsyncConsole::WaitForInput(string *s)
{
	WaitForSingleObject(pimpl->m_Event, INFINITE);

	EnterCriticalSection(&pimpl->m_ReadCS);
	assert(!pimpl->m_Queue.empty());
	*s = pimpl->m_Queue.front();
	pimpl->m_Queue.pop();
	if (pimpl->m_Queue.empty())
		ResetEvent(pimpl->m_Event);
	LeaveCriticalSection(&pimpl->m_ReadCS);

//	WaitForSingleObject(pimpl->m_Event, INFINITE);
//	assert(GetInput(s));
}

HANDLE AsyncConsole::GetWaitEvent()
{
	return pimpl->m_Event;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa AsyncConsoleLog

class AsyncConsoleLog::Pimpl
{
public:
	typedef std::pair<uint4, WORD> COLOR_MAPPING;
	typedef std::vector<COLOR_MAPPING> COLOR_MAPPING_VECTOR;

	COLOR_MAPPING_VECTOR m_ColorMapping;
};

void AsyncConsoleLog::OnLog(uint4 Type, const string &Prefix, const string &TypePrefix, const string &Message)
{
	// Jeœli nie ma async consoli, nie rób nic
	if (g_AsyncConsole == NULL)
		return;

	// ZnajdŸ kolor
	bool ColorFound = false;
	for (
		AsyncConsoleLog::Pimpl::COLOR_MAPPING_VECTOR::const_iterator cit = pimpl->m_ColorMapping.begin();
		cit != pimpl->m_ColorMapping.end();
		++cit)
	{
		if (cit->first & Type)
		{
			g_AsyncConsole->SetOutputColor(cit->second);
			ColorFound = true;
			break;
		}
	}
	if (!ColorFound)
		g_AsyncConsole->SetOutputColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	// Zaloguj
	string s;
	ReplaceEOL(&s, Prefix + TypePrefix + Message, EOL_CRLF);
	g_AsyncConsole->Writeln(s);
}

AsyncConsoleLog::AsyncConsoleLog() :
	pimpl(new Pimpl())
{
}

AsyncConsoleLog::~AsyncConsoleLog()
{
}

void AsyncConsoleLog::AddColorMapping(uint4 Mask, WORD Color)
{
	pimpl->m_ColorMapping.push_back(Pimpl::COLOR_MAPPING(Mask, Color));
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy globalne

scoped_ptr<AsyncConsole> g_AsyncConsole;
