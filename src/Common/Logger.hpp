/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Logger - Mechanizm logowania komunikatów
 * Dokumentacja: Patrz plik doc/Logger.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_LOGGER_H_
#define COMMON_LOGGER_H_

// Dziwaczna deklaracja zapowiadaj¹ca, ¿eby nie w³¹czaæ tu w nag³ówku <iostream>
namespace std
{
	template <typename T> struct char_traits;
	template <typename T1, typename T2> class basic_ostream;
	typedef basic_ostream<char, char_traits<char> > ostream;
};

namespace common
{

class ILog;
class Logger_pimpl;

class Logger
{
	friend void CreateLogger(bool);
	friend void DestroyLogger();
	friend class ILog;

private:
	scoped_ptr<Logger_pimpl> pimpl;

	Logger(bool UseQueue);
	~Logger();

public:
	// --- KONFIGURACJA ---
	// (Nie s¹ bezpieczne w¹tkowo.)

	// Rejestruje log w loggerze mapuj¹c do niego okreœlone typy komunikatów
	void AddLogMapping(uint4 Mask, ILog *Log);
	// Ustawia mapowanie prefiksów typu dla wszystkich zarejestrowanych logów
	void AddTypePrefixMapping(uint4 Mask, const string &Prefix);
	// Ustawia format prefiksu dla wszystkich zarejestrowanych logów
	void SetPrefixFormat(const string &PrefixFormat);

	// --- U¯YWANIE ---
	// (S¹ bezpieczne w¹tkowo.)

	// Ustawia w³asn¹ informacjê prefiksu
	// Indeks: 0..2
	void SetCustomPrefixInfo(int Index, const string &Info);
	// Loguje komunikat - najwa¿niejsza funkcja!
	void Log(uint4 Type, const string &Message);
};

class ILog
{
	friend class Logger_pimpl;

private:
	class ILog_pimpl;
	scoped_ptr<ILog_pimpl> pimpl;

protected:
	// Ma zalogowaæ podany komunikat tam gdzie trzeba
	virtual void OnLog(uint4 Type, const string &Prefix, const string &TypePrefix, const string &Message) = 0;

public:
	ILog();
	virtual ~ILog();

	// --- KONFIGURACJA ---

	// Dodaje mapowanie prefiksu typu dla tego loga
	void AddTypePrefixMapping(uint4 Mask, const string &Prefix);
	// Ustawia format prefiksu dla tego loga
	void SetPrefixFormat(const string &PrefixFormat);
};

// Tworzy logger
void CreateLogger(bool UseQueue);
// Usuwa logger
void DestroyLogger();
// Pobiera logger
Logger & GetLogger();
// Zwraca true, jeœli Logger jest zainicjowany
bool IsLogger();


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Konkretne loggery

// Tryby, w jakich mo¿e pracowaæ logger plikowy
enum LOG_FILE_MODE
{
	// Normalny - najszybszy
	FILE_MODE_NORMAL,
	// Po ka¿dym komunikacie FLUSH
	FILE_MODE_FLUSH,
	// Dla ka¿dego komunikatu osobne otwarcie pliku - najbardziej niezawodny
	FILE_MODE_REOPEN,
};

// Log do pliku TXT
class TextFileLog : public ILog
{
private:
	class TextFileLog_pimpl;
	scoped_ptr<TextFileLog_pimpl> pimpl;

protected:
	virtual void OnLog(uint4 Type, const string &Prefix, const string &TypePrefix, const string &Message);

public:
	TextFileLog(const string &FileName, LOG_FILE_MODE Mode, EOLMODE EolMode, bool Append = false, const string &StartText = "");
	virtual ~TextFileLog();
};

// Log do pliku HTML
class HtmlFileLog : public ILog
{
public:
	struct STYLE
	{
		uint4 BackgroundColor;
		uint4 FontColor;
		bool Bold;
		bool Italic;

		STYLE(bool Bold = false, bool Italic = false, uint FontColor = 0x000000, uint BackgroundColor = 0xFFFFFF);
	};

private:
	class HtmlFileLog_pimpl;
	scoped_ptr<HtmlFileLog_pimpl> pimpl;

protected:
	virtual void OnLog(uint4 Type, const string &Prefix, const string &TypePrefix, const string &Message);

public:
	HtmlFileLog(const string &FileName, LOG_FILE_MODE Mode, bool Append = false, const string &StartText = "");
	virtual ~HtmlFileLog();

	// --- KONFIGURACJA ---

	// Dodaje mapowanie typu komunikatu na styl
	void AddStyleMapping(uint4 Mask, const STYLE &Style);
};

class OstreamLog : public ILog
{
private:
	class Pimpl;
	scoped_ptr<Pimpl> pimpl;

protected:
	virtual void OnLog(uint4 Type, const string &Prefix, const string &TypePrefix, const string &Message);

public:
	OstreamLog(std::ostream *Ostream);
	virtual ~OstreamLog();
};

} // namespace common

// Skrót do ³atwego zalogowania ³añcucha
#define LOG(Type, s) { if (common::IsLogger()) common::GetLogger().Log((Type), (s)); else assert(0 && "LOG macro: Logger not initialized."); }

#endif
