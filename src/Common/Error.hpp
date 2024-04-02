/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Error - Klasy wyj�tk�w do obs�ugi b��d�w
 * Dokumentacja: Patrz plik doc/Error.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_ERROR_H_
#define COMMON_ERROR_H_

// Forward declarations
typedef long HRESULT;
#ifdef WIN32
	#include <windows.h>
#endif
#ifdef USE_DIRECTX
	struct ID3DXBuffer;
#endif

namespace common
{

// Klasa bazowa b��d�w
class Error
{
private:
	std::vector<string> m_Msgs;

protected:
	// Use this constructor if you need to compute something before
	// creating first error message and can't call constructor of this
	// base class on the constructor initialization list
	Error() { }

public:
	// Push a message on the stack
	// Use __FILE__ for file and __LINE__ for line
	void Push(const string &msg, const string &file = "", int line = 0);
	// Zwraca pe�ny opis b��du w jednym �a�cuchu
	// Indent - �a�cuch od kt�rego ma si� zaczyna� ka�dy komunikat (wci�cie).
	// EOL - �a�cuch oddzielaj�cy komunikaty (koniec wiersza). Nie do��czany na ko�cu.
	void GetMessage_(string *Msg, const string &Indent = "", const string &EOL = "\r\n") const;

	// General error creation
	Error(const string &msg, const string &file = "", int line = 0)
	{
		Push(msg, file, line);
	}
};

// Exception created from errno system error
class ErrnoError : public Error
{
public:
	// Create error with given code
	ErrnoError(int err_code, const string &msg, const string &file = "", int line = 0);
	// Create error with code extracted from errno
	ErrnoError(const string &msg, const string &file = "", int line = 0);
};

#ifdef USE_SDL
	// Exception with error message extracted from SDLError
	class SDLError : public Error
	{
	public:
		SDLError(const string &msg, const string &file = "", int line = 0);
	};
#endif

#ifdef USE_OPENGL
	// Exception with error message extracted from OpenGLError
	class OpenGLError : public Error
	{
	public:
		OpenGLError(const string &msg, const string &file = "", int line = 0);
	};
#endif

#ifdef USE_FMOD
	// Exception with error message extracted from FMOD sound library
	class FmodError : public Error
	{
	private:
		// Enum and/or Message can be 0
		void CodeToMessage(FMOD_RESULT code, string *Enum, string *Message);

	public:
		FmodError(FMOD_RESULT code, const string &msg, const string &file = "", int line = 0);
	};
#endif

#ifdef WIN32
	// Tworzy b��d Windows API na podstawie GetLastError
	class Win32Error : public Error
	{
	public:
		Win32Error(const string &msg = "", const string &file = "", int line = 0);
	};
#endif

#ifdef USE_DIRECTX
	class DirectXError : public Error
	{
	public:
		DirectXError(HRESULT hr, const string &Msg = "", const string &File = "", int Line = 0);
		// U�yj tej wersji je�li funkcja DX kt�r� kontroluje zwraca te� w przypadku b��du bufor z komunikatem
		// Bufor zostanie przez konstruktor tego wyj�tku sam zwolniony, je�li istnia�. Mo�e nie istnie�.
		DirectXError(HRESULT hr, ID3DXBuffer *Buf, const string &Msg = "", const string &File = "", int Line = 0);
	};
#endif

#ifdef USE_WINSOCK
	class WinSockError : public Error
	{
	private:
		void CodeToStr(int Code, string *Name, string *Message);

	public:
		// Tworzy komunikat b��du WinSockError
		int WinSockError(int Code, const string &Msg = "", const string &File = "", int Line = 0);
		// Tworzy komunikat b��du WinSockError, pobiera kod sam z WSAGetLastError()
		int WinSockError(const string &Msg = "", const string &File = "", int Line = 0);
	};
#endif

#ifdef USE_DEVIL
	class DevILError : public Error
	{
	public:
		DevILError(const string &Msg = "", const string &File = "", int a_Line = 0);
	};
#endif

#ifdef USE_AVI_FILE
	class AVIFileError : public Error
	{
	public:
		AVIFileError(HRESULT hr, const string &Msg = "", const string &File = "", int a_Line = 0);
	};
#endif

} // namespace common

// Use to push a message on the call stack
#define ERR_ADD_FUNC(exception) { (exception).Push(__FUNCSIG__, __FILE__, __LINE__); }
// Guard a function
#define ERR_TRY        { try {
#define ERR_CATCH(msg) } catch(Error &e) { e.Push((msg), __FILE__, __LINE__); throw; } }
#define ERR_CATCH_FUNC } catch(Error &e) { ERR_ADD_FUNC(e); throw; } }

#define ERR_GUARD_BOOL(Expr) { if ((Expr) == false) throw Error((#Expr), __FILE__, __LINE__); }
#define ERR_GUARD_DIRECTX(Expr) { HRESULT hr; if (FAILED(hr = (Expr))) throw DirectXError(hr, (#Expr), __FILE__, __LINE__); }

#ifdef _DEBUG
	#define ERR_GUARD_BOOL_D(Expr) ERR_GUARD_BOOL(Expr)
	#define ERR_GUARD_DIRECTX_D(Expr) ERR_GUARD_DIRECTX(Expr)
#else
	#define ERR_GUARD_BOOL_D(Expr) (Expr);
	#define ERR_GUARD_DIRECTX_D(Expr) (Expr);
#endif

// Rozszerzenie modu�u bazowego o wsparcie dla b��d�w
template <typename T>
inline void MustStrToSth(T *Sth, const string &Str)
{
	if (!StrToSth<T>(Sth, Str))
		throw common::Error("B��d konwersji �a�cucha: " + Str);
}

#endif
