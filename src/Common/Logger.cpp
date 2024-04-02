/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Logger - Mechanizm logowania komunikat�w
 * Dokumentacja: Patrz plik doc/Logger.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include "Math.hpp"
#include <ctime>
#include <iostream>
#include <deque>
#include "Threads.hpp"
#include "Files.hpp"
#include "Logger.hpp"


namespace common
{

typedef std::pair<uint4, string> MESSAGE_PAIR;

struct PREFIX_INFO
{
	string Date;
	string Time;
	string CustomPrefixInfo[3];
};

const uint4 MAX_QUEUE_SIZE = 1024;

string HtmlSpecialChars(const string &s)
{
	string s1, s2;
	Replace(&s1, s, "&", "&amp;");
	Replace(&s2, s1, "<", "&lt;");
	Replace(&s1, s2, ">", "&gt;");
	Replace(&s2, s1, "\"", "&quot;");
	Replace(&s1, s2, "'", "&apos;");
	Replace(&s2, s1, "\r", string());
	Replace(&s1, s2, "\n", "\n<br>");
	return s1;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Log

class ILog::ILog_pimpl
{
public:
	typedef std::vector<MESSAGE_PAIR> TYPE_PREFIX_MAPPING_VECTOR;

	ILog *impl;
	string m_PrefixFormat;
	TYPE_PREFIX_MAPPING_VECTOR m_TypePrefixMapping;

	// dla loggera
	void Log(uint4 Type, const string &Message, const PREFIX_INFO &PrefixInfo);
};

void ILog::ILog_pimpl::Log(uint4 Type, const string &Message, const PREFIX_INFO &PrefixInfo)
{
	// U�o�enie prefiksu
	string Prefix1, Prefix2;
	Replace(&Prefix1, m_PrefixFormat, "%D", PrefixInfo.Date);
	Replace(&Prefix2, Prefix1, "%T", PrefixInfo.Time);
	Replace(&Prefix1, Prefix2, "%1", PrefixInfo.CustomPrefixInfo[0]);
	Replace(&Prefix2, Prefix1, "%2", PrefixInfo.CustomPrefixInfo[1]);
	Replace(&Prefix1, Prefix2, "%3", PrefixInfo.CustomPrefixInfo[2]);
	Replace(&Prefix2, Prefix1, "%%", "%");

	// U�o�enie prefiksu typu
	string TypePrefix;
	for (
		TYPE_PREFIX_MAPPING_VECTOR::const_iterator cit = m_TypePrefixMapping.begin();
		cit != m_TypePrefixMapping.end();
		++cit)
	{
		if (Type & cit->first)
		{
			TypePrefix = cit->second;
			break;
		}
	}

	// Przes�anie do zalogowania
	impl->OnLog(Type, Prefix2, TypePrefix, Message);
}

ILog::ILog() :
	pimpl(new ILog_pimpl())
{
	pimpl->impl = this;
}

ILog::~ILog()
{
}

void ILog::AddTypePrefixMapping(uint4 Mask, const string &Prefix)
{
	pimpl->m_TypePrefixMapping.push_back(std::make_pair(Mask, Prefix));
}

void ILog::SetPrefixFormat(const string &PrefixFormat)
{
	pimpl->m_PrefixFormat = PrefixFormat;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Logger

class LoggerThread;

class Logger_pimpl
{
	DECLARE_NO_COPY_CLASS(Logger_pimpl)

public:
	struct QUEUE_ITEM
	{
		// MAXUINT4 == zwyk�y komunikat
		// 0..2 == custom prefix info
		uint4 What;
		// Tylko je�li zwyk�y komunikat
		uint4 Type;
		// Tre�� custom prefix info lub komunikatu
		string Message;
	};

	typedef std::vector< std::pair<uint4, ILog*> > LOG_MAPPING_VECTOR;

	Mutex m_Mutex;
	LOG_MAPPING_VECTOR m_LogMapping;
	string m_CustomPrefixInfo[3];

	Logger_pimpl() : m_Mutex(Mutex::FLAG_RECURSIVE) { }

	bool m_UseQueue;
	// ----- U�ywane tylko je�li u�ywane jest kolejkowanie, st�d wska�niki -----

	// Sygnalizowany kiedy kolejka jest niepusta lub trzeba zako�czy� w�tek.
	// W�tek g��wny sygnalizuje, w�tek roboczy czeka.
	scoped_ptr<Cond> m_QueueNotEmptyOrExit;
	// Sygnalizowany kiedy kolejka jest niepe�na.
	// W�tek roboczy sygnalizuje, w�tek g��wny czeka.
	scoped_ptr<Cond> m_QueueNotFull;
	// Muteks do zabezpieczania operacji na kolejce lub na fladze zako�czenia
	scoped_ptr<Mutex> m_QueueMutex;
	// A oto i rzeczona kolejka
	// Nie queue, bo trzeba te� sprawdza� rozmiar.
	scoped_ptr< std::deque<QUEUE_ITEM> > m_Queue;
	// Oraz flaga zako�czenia
	bool m_ThreadEnd;
	// Uchwyt do w�tku, co by si� da�o poczeka� na jego zako�czenie
	scoped_ptr<LoggerThread> m_Thread;

	void Log(uint4 Type, const string &Message);
	void SetCustomPrefixInfo(int Index, const string &Info);
	// Funkcja do w�tku
	void ThreadFunc();
};

// Zrobione brzydko, bo to jest dorabiane ju� po sprawie
class LoggerThread : public Thread
{
private:
	Logger_pimpl *m_Pimpl;

protected:
	virtual void Run() { m_Pimpl->ThreadFunc(); }

public:
	LoggerThread(Logger_pimpl *Pimpl) : m_Pimpl(Pimpl) { }
};

void Logger_pimpl::Log(uint4 Type, const string &Message)
{
	MUTEX_LOCK(&m_Mutex);

	PREFIX_INFO PrefixInfo;
	bool PrefixGenerated = false;

	// Znajd� odpwiednie loggery
	for (
		Logger_pimpl::LOG_MAPPING_VECTOR::const_iterator cit = m_LogMapping.begin();
		cit != m_LogMapping.end();
		++cit)
	{
		// Je�li maska pasuje
		if (Type & cit->first)
		{
			// Je�li jeszcze nie by� wygenerowany, wygeneruj prefiks
			if (!PrefixGenerated)
			{
				time_t Time1; time(&Time1);
				tm Time2 = *localtime(&Time1);
				PrefixInfo.Date = Format("#-#-#") %
					IntToStr2R(Time2.tm_year+1900, 4) %
					IntToStr2R(Time2.tm_mon+1, 2) %
					IntToStr2R(Time2.tm_mday, 2);
				PrefixInfo.Time = Format("#:#:#") %
					IntToStr2R(Time2.tm_hour, 2) %
					IntToStr2R(Time2.tm_min, 2) %
					IntToStr2R(Time2.tm_sec, 2);
				PrefixInfo.CustomPrefixInfo[0] = m_CustomPrefixInfo[0];
				PrefixInfo.CustomPrefixInfo[1] = m_CustomPrefixInfo[1];
				PrefixInfo.CustomPrefixInfo[2] = m_CustomPrefixInfo[2];
				PrefixGenerated = true;
			}

			// Prze�lij do zalogowania
			cit->second->pimpl->Log(Type, Message, PrefixInfo);
		}
	}
}

void Logger_pimpl::SetCustomPrefixInfo(int Index, const string &Info)
{
	MUTEX_LOCK(&m_Mutex);

	m_CustomPrefixInfo[Index] = Info;
}

void Logger_pimpl::ThreadFunc()
{
	QUEUE_ITEM QueueItem;
	for (;;)
	{
		try
		{
			// we� polecenie lub zako�cz dzia�anie
			{
				// Lock �eby mie� dost�p do m_Queue i m_ThreadEnd
				MUTEX_LOCK(m_QueueMutex.get());
				// Czekamy na jedno z tych zdarze�
				while (m_Queue->empty() && !m_ThreadEnd)
					m_QueueNotEmptyOrExit->Wait(m_QueueMutex.get());
				// Doczekali�my si� - do dzie�a!
				// - koniec w�tku - ale �eby naprawd� sko�czy�, kolejka musi byc pusta!
				if (m_ThreadEnd && m_Queue->empty())
					break;
				// - skoro nie, to nowy komunikat w kolejce - pobra� i zostawi� kolejk� w spokoju
				QueueItem = m_Queue->front();
				m_Queue->pop_front();
				m_QueueNotFull->Signal();
			}
			// Zr�b co m�wi item (on tam sobie ju� zablokuje co trzeba)
			if (QueueItem.What == MAXUINT4)
				Log(QueueItem.Type, QueueItem.Message);
			else
				SetCustomPrefixInfo(QueueItem.What, QueueItem.Message);
		}
		catch (...)
		{
			// Przemilcz wyj�tek, �eby nie wylecia� poza w�tek (ale wierszyk :D)
		}
	}
}

Logger::Logger(bool UseQueue) :
	pimpl(new Logger_pimpl)
{
	pimpl->m_UseQueue = UseQueue;

	if (UseQueue)
	{
		pimpl->m_QueueNotEmptyOrExit.reset(new Cond);
		pimpl->m_QueueNotFull.reset(new Cond);
		pimpl->m_QueueMutex.reset(new Mutex(Mutex::FLAG_RECURSIVE));
		pimpl->m_Queue.reset(new std::deque<Logger_pimpl::QUEUE_ITEM>());
		pimpl->m_ThreadEnd = false;

		// Odpal w�tek
		// Heh! Czuj� si� jak Korea P�n. przed pr�b� nuklearn� :)
		// W ko�cu to m�j pierwszy w �yciu w�tek producent-konsument...
		pimpl->m_Thread.reset(new LoggerThread(pimpl.get()));
		pimpl->m_Thread->Start();
	}
}

Logger::~Logger()
{
	if (pimpl->m_UseQueue)
	{
		{
			MUTEX_LOCK(pimpl->m_QueueMutex.get());
			pimpl->m_ThreadEnd = true;
			pimpl->m_QueueNotEmptyOrExit->Signal();
		}
		pimpl->m_Thread->Join();
	}
}

void Logger::AddLogMapping(uint4 Mask, ILog *Log)
{
	pimpl->m_LogMapping.push_back(std::make_pair(Mask, Log));
}

void Logger::AddTypePrefixMapping(uint4 Mask, const string &Prefix)
{
	for (
		Logger_pimpl::LOG_MAPPING_VECTOR::const_iterator cit = pimpl->m_LogMapping.begin();
		cit != pimpl->m_LogMapping.end();
		++cit)
	{
		cit->second->AddTypePrefixMapping(Mask, Prefix);
	}
}

void Logger::SetPrefixFormat(const string &PrefixFormat)
{
	for (
		Logger_pimpl::LOG_MAPPING_VECTOR::const_iterator cit = pimpl->m_LogMapping.begin();
		cit != pimpl->m_LogMapping.end();
		++cit)
	{
		cit->second->SetPrefixFormat(PrefixFormat);
	}
}

void Logger::SetCustomPrefixInfo(int Index, const string &Info)
{
	assert(Index >= 0 && Index < 3);

	if (pimpl->m_UseQueue)
	{
		MUTEX_LOCK(pimpl->m_QueueMutex.get());
		while (pimpl->m_Queue->size() == MAX_QUEUE_SIZE)
			pimpl->m_QueueNotFull->Wait(pimpl->m_QueueMutex.get());

		Logger_pimpl::QUEUE_ITEM QueueItem;
		QueueItem.What = Index;
		QueueItem.Message = Info;
		pimpl->m_Queue->push_back(QueueItem);
		pimpl->m_QueueNotEmptyOrExit->Signal();
	}
	else
		pimpl->SetCustomPrefixInfo(Index, Info);
}

void Logger::Log(uint4 Type, const string &Message)
{
	if (pimpl->m_UseQueue)
	{
		MUTEX_LOCK(pimpl->m_QueueMutex.get());
		while (pimpl->m_Queue->size() == MAX_QUEUE_SIZE)
			pimpl->m_QueueNotFull->Wait(pimpl->m_QueueMutex.get());

		Logger_pimpl::QUEUE_ITEM QueueItem;
		QueueItem.What = MAXUINT4;
		QueueItem.Type = Type;
		QueueItem.Message = Message;
		pimpl->m_Queue->push_back(QueueItem);
		pimpl->m_QueueNotEmptyOrExit->Signal();
	}
	else
		pimpl->Log(Type, Message);
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TextFileLog

class TextFileLog::TextFileLog_pimpl
{
public:
	LOG_FILE_MODE m_Mode;
	string m_FileName;
	string m_EOL;
	EOLMODE m_EolMode;
	scoped_ptr<common::FileStream> m_File;
};

void TextFileLog::OnLog(uint4 Type, const string &Prefix, const string &TypePrefix, const string &Message)
{
	// Otwarcie
	if (pimpl->m_Mode == FILE_MODE_REOPEN)
	{
		assert(pimpl->m_File.get() == 0);
		pimpl->m_File.reset(new FileStream(pimpl->m_FileName, common::FM_APPEND, false));
	}

	assert(pimpl->m_File.get());

	// Zapisanie
	string s;
	ReplaceEOL(&s, Prefix, pimpl->m_EolMode);
	pimpl->m_File->WriteStringF(s);
	ReplaceEOL(&s, TypePrefix, pimpl->m_EolMode);
	pimpl->m_File->WriteStringF(s);
	ReplaceEOL(&s, Message, pimpl->m_EolMode);
	pimpl->m_File->WriteStringF(s);
	pimpl->m_File->WriteStringF(pimpl->m_EOL);

	// Flush
	if (pimpl->m_Mode == FILE_MODE_FLUSH)
		pimpl->m_File->Flush();
	// Zamkni�cie
	else if (pimpl->m_Mode == FILE_MODE_REOPEN)
		pimpl->m_File.reset(0);
}

TextFileLog::TextFileLog(const string &FileName, LOG_FILE_MODE Mode, EOLMODE EolMode, bool Append, const string &StartText) :
	pimpl(new TextFileLog_pimpl())
{
	pimpl->m_Mode = Mode;
	pimpl->m_FileName = FileName;
	EolModeToStr(&pimpl->m_EOL, EolMode);
	pimpl->m_EolMode = EolMode;

	// Otwarcie pliku
	// Bo ma pozosta� otwarty
	// Lub bo ma zosta� zapisany pocz�tkowy tekst
	// Lub bo nie ma by� dopisywany, wi�c trzeba go wyczy�ci�.
	if (Mode != FILE_MODE_REOPEN || !StartText.empty() || !Append)
	{
		pimpl->m_File.reset(new FileStream(
			FileName,
			Append ? common::FM_APPEND : common::FM_WRITE,
			false));

		// Dopisanie tekstu startowego
		if (!StartText.empty())
		{
			pimpl->m_File->WriteStringF(StartText);
			pimpl->m_File->WriteStringF(pimpl->m_EOL);
		}

		// Zamkni�cie pliku
		if (Mode == FILE_MODE_REOPEN)
			pimpl->m_File.reset(0);
	}
}

TextFileLog::~TextFileLog()
{
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa HtmlFileLog

HtmlFileLog::STYLE::STYLE(bool Bold, bool Italic, uint FontColor, uint BackgroundColor) :
	BackgroundColor(BackgroundColor),
	FontColor(FontColor),
	Bold(Bold),
	Italic(Italic)
{
}

class HtmlFileLog::HtmlFileLog_pimpl
{
public:
	typedef std::vector< std::pair<uint4, STYLE> > STYLE_MAPPING_VECTOR;

	LOG_FILE_MODE m_Mode;
	string m_FileName;
	scoped_ptr<FileStream> m_File;
	STYLE_MAPPING_VECTOR m_StyleMapping;

	string ColorToHtml(uint4 Color);
};

string HtmlFileLog::HtmlFileLog_pimpl::ColorToHtml(uint4 Color)
{
	string s;
	UintToStr(&s, Color, 16);
	while (s.length() < 6)
		s = "0" + s;
	return "#" + s;
}

void HtmlFileLog::OnLog(uint4 Type, const string &Prefix, const string &TypePrefix, const string &Message)
{
	// Otwarcie
	if (pimpl->m_Mode == FILE_MODE_REOPEN)
	{
		assert(pimpl->m_File.get() == 0);
		pimpl->m_File.reset(new FileStream(pimpl->m_FileName, common::FM_APPEND, false));
	}

	assert(pimpl->m_File.get());

	// Styl
	STYLE Style;
	for(
		HtmlFileLog_pimpl::STYLE_MAPPING_VECTOR::const_iterator cit = pimpl->m_StyleMapping.begin();
		cit != pimpl->m_StyleMapping.end();
		++cit)
	{
		if (Type & cit->first)
		{
			Style = cit->second;
			break;
		}
	}
	string StyleStr;
	if (Style.BackgroundColor != 0xFFFFFF)
		StyleStr += "background-color:" + pimpl->ColorToHtml(Style.BackgroundColor) + ";";
	if (Style.FontColor != 0x000000)
		StyleStr += "color:" + pimpl->ColorToHtml(Style.FontColor) + ";";
	if (Style.Bold)
		StyleStr += "font-weight:bold;";
	if (Style.Italic)
		StyleStr += "font-style:italic;";

	// Zapisanie
	string Code = "<div style=\""+HtmlSpecialChars(StyleStr)+"\"><b>";
	Code += HtmlSpecialChars(Prefix);
	Code += HtmlSpecialChars(TypePrefix);
	Code += "</b>";
	Code += HtmlSpecialChars(Message);
	Code += "</div>\n";
	pimpl->m_File->WriteStringF(Code);

	// Flush
	if (pimpl->m_Mode == FILE_MODE_FLUSH)
		pimpl->m_File->Flush();
	// Zamkni�cie
	else if (pimpl->m_Mode == FILE_MODE_REOPEN)
		pimpl->m_File.reset(0);
}

HtmlFileLog::HtmlFileLog(const string &FileName, LOG_FILE_MODE Mode, bool Append, const string &StartText) :
	pimpl(new HtmlFileLog_pimpl())
{
	pimpl->m_Mode = Mode;
	pimpl->m_FileName = FileName;

	bool Exists = (GetFileItemType(FileName) == common::IT_FILE);

	// Otwarcie pliku
	// Bo ma pozosta� otwarty
	// Lub bo ma zosta� zapisany pocz�tkowy tekst
	// Lub bo nie ma by� dopisywany, wi�c trzeba go wyczy�ci�.
	// Lub bo trzeba zapisa� nag��wek HTML
	if (Mode != FILE_MODE_REOPEN || !StartText.empty() || !Append || !Exists)
	{
		// Otwarcie
		pimpl->m_File.reset(new FileStream(
			FileName,
			Append ? common::FM_APPEND : common::FM_WRITE,
			false));

		// Zapisanie nag��wka HTML
		if (!Exists || !Append)
		{
			string Head;
			string FileName2; ExtractFileName(&FileName2, FileName);
			string FileName3; ChangeFileExt(&FileName3, FileName2, string());
			Head += "<html>\n";
			Head += "<head>\n";
			Head += "	<title>Log - " + FileName3 + "</title>\n";
			Head += "</head>\n";
			Head += "<body style=\"font-family:&quot;Courier New&quot;,Courier,monospace; font-size:9pt\">\n\n";
			pimpl->m_File->WriteStringF(Head);
		}

		// Dopisanie tekstu startowego
		if (!StartText.empty())
			pimpl->m_File->WriteStringF("\n<p>" + HtmlSpecialChars(StartText) + "</p>\n\n");

		// Zamkni�cie pliku
		if (Mode == FILE_MODE_REOPEN)
			pimpl->m_File.reset(0);
	}
}

HtmlFileLog::~HtmlFileLog()
{
}

void HtmlFileLog::AddStyleMapping(uint4 Mask, const STYLE &Style)
{
	pimpl->m_StyleMapping.push_back(std::make_pair(Mask, Style));
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa OstreamLog

class OstreamLog::Pimpl
{
public:
	std::ostream *m_Ostream;
};

void OstreamLog::OnLog(uint4 Type, const string &Prefix, const string &TypePrefix, const string &Message)
{
	*pimpl->m_Ostream << Prefix << TypePrefix << Message << std::endl;
}

OstreamLog::OstreamLog(std::ostream *Ostream) :
	pimpl(new Pimpl())
{
	pimpl->m_Ostream = Ostream;
}

OstreamLog::~OstreamLog()
{
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Elementy globalne

Logger *g_Logger = 0;

void CreateLogger(bool UseQueue)
{
	if (g_Logger == 0)
		g_Logger = new Logger(UseQueue);
}

void DestroyLogger()
{
	SAFE_DELETE(g_Logger);
}

Logger & GetLogger()
{
	assert(g_Logger);
	return *g_Logger;
}

bool IsLogger()
{
	return (g_Logger != 0);
}

} // namespace common
