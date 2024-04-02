/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * ZlibUtils - Modu� wspmagaj�cy biblioteki zlib [modu� przeno�ny, wymaga biblioteki zlib]
 * Dokumentacja: Patrz plik doc/ZlibUtils.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
extern "C" {
	#include <errno.h>
	#include <zlib.h>
	#include <string.h> // dla strnlen
}
#include "DateTime.hpp"
#include "ZlibUtils.hpp"


namespace common
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ZlibCompressionStreamError

ZlibError::ZlibError(int Code, const string &Msg, const string &File, int Line)
{
	if (Code == Z_ERRNO)
		Push("(zlib(errno)," + IntToStrR(errno) + ") " + strerror(errno)); // jak w err.Errno.ctor
	else
		Push(Format("(zlib,#) #") % Code % zError(Code));

	Push(Msg, File, Line);
}


const int ZLIB_DEFAULT_LEVEL = Z_DEFAULT_COMPRESSION;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ZlibCompressionStream_pimpl

class ZlibCompressionStream_pimpl
{
private:
	Stream *m_Stream;
	z_stream m_ZStream;
	std::vector<char> m_OutBuf;
	// Tu jest tutaj, bo cho� tego panowie szanowni autorzy zliba dziady pi*** nie udokumentowali,
	// to header podany w deflateSetHeader musi istnie� przy wywo�aniu deflate, bo dopiero wtedy nag��wek
	// si� zapisuje, a deflateSetHeader tylko zachowuje sobie w state wska�nik do niego!!!
	gz_header m_Header;
	std::vector<char> m_Header_FileName;
	std::vector<char> m_Header_Comment;

public:
	// Wersja ZlibError
	ZlibCompressionStream_pimpl(Stream *a_Stream, int Level);
	// Wersja Gzip
	ZlibCompressionStream_pimpl(Stream *a_Stream, const string *FileName, const string *Comment, int Level);
	~ZlibCompressionStream_pimpl();

	void Write(const void *Data, size_t Size);
	void Flush();
};

ZlibCompressionStream_pimpl::ZlibCompressionStream_pimpl(Stream *a_Stream, int Level) :
	m_Stream(a_Stream)
{
	m_OutBuf.resize(BUFFER_SIZE);

	m_ZStream.zalloc = Z_NULL;
	m_ZStream.zfree = Z_NULL;
	m_ZStream.opaque = Z_NULL;

	int R = deflateInit(&m_ZStream, Level);
	if (R != Z_OK)
		throw ZlibError(R, "Nie mo�na zainicjalizowa� kompresji zlib", __FILE__, __LINE__);
}

ZlibCompressionStream_pimpl::ZlibCompressionStream_pimpl(Stream *a_Stream, const string *FileName, const string *Comment, int Level) :
	m_Stream(a_Stream)
{
	m_OutBuf.resize(BUFFER_SIZE);

	m_ZStream.zalloc = Z_NULL;
	m_ZStream.zfree = Z_NULL;
	m_ZStream.opaque = Z_NULL;

	int R = deflateInit2(&m_ZStream, Level, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
	if (R != Z_OK)
		throw ZlibError(R, "Nie mo�na zainicjalizowa� kompresji gzip", __FILE__, __LINE__);

	// Nag��wek gzip
	ZeroMem(&m_Header, sizeof(m_Header));
	m_Header.time = (uLong)Now().GetTicks();
#ifdef WIN32
	m_Header.os = 0;
#else
	m_Header.os = 3;
#endif
	m_Header.extra = Z_NULL;
	if (FileName != NULL && !FileName->empty())
	{
		m_Header_FileName.resize(FileName->length()+1);
		std::copy(FileName->begin(), FileName->end(), m_Header_FileName.begin());
		m_Header_FileName[FileName->length()] = '\0';
		m_Header.name = (Bytef*)&m_Header_FileName[0];
	}
	else
		m_Header.name = Z_NULL;
	if (Comment != NULL && !Comment->empty())
	{
		m_Header_Comment.resize(Comment->length()+1);
		std::copy(Comment->begin(), Comment->end(), m_Header_Comment.begin());
		m_Header_Comment[Comment->length()] = '\0';
		m_Header.comment = (Bytef*)&m_Header_Comment[0];
	}
	else
		m_Header.comment = Z_NULL;

	R = deflateSetHeader(&m_ZStream, &m_Header);
	if (R != Z_OK)
		throw ZlibError(R, "Nie mo�na ustawi� nag��wka gzip", __FILE__, __LINE__);
}

ZlibCompressionStream_pimpl::~ZlibCompressionStream_pimpl()
{
	try
	{
		char Foo = '\0';
		m_ZStream.next_in = (Bytef*)&Foo;
		m_ZStream.avail_in = 0;

		int R;
		do
		{
			m_ZStream.next_out = (Bytef*)&m_OutBuf[0];
			m_ZStream.avail_out = BUFFER_SIZE;

			R = deflate(&m_ZStream, Z_FINISH);

			if (m_ZStream.avail_out < BUFFER_SIZE)
				m_Stream->Write(&m_OutBuf[0], BUFFER_SIZE - m_ZStream.avail_out);
		}
		while (R != Z_STREAM_END);

		deflateEnd(&m_ZStream);
	}
	catch (...)
	{
		assert(0 && "ZlibCompressionStream.dtor - wyj�tek");
	}
}

void ZlibCompressionStream_pimpl::Write(const void *Data, size_t Size)
{
	if (Size == 0) return;

	m_ZStream.next_in = (Bytef*)Data;
	m_ZStream.avail_in = Size;

	do
	{
		m_ZStream.next_out = (Bytef*)&m_OutBuf[0];
		m_ZStream.avail_out = BUFFER_SIZE;

		deflate(&m_ZStream, Z_NO_FLUSH); // Nie sprawdzamy b��du, bo podobno nie trzeba a nawet nie nale�y, bo zdarza si� Z_BUF_ERROR.

		if (m_ZStream.avail_out < BUFFER_SIZE)
			m_Stream->Write(&m_OutBuf[0], BUFFER_SIZE - m_ZStream.avail_out);
	}
	while (m_ZStream.avail_out == 0); // Tak, dziwne, ale w�a�nie taki warunek + ten assert jest w przyk�adzie w zlib.
	assert(m_ZStream.avail_in == 0);
}

void ZlibCompressionStream_pimpl::Flush()
{
	char Foo = '\0';
	m_ZStream.next_in = (Bytef*)&Foo;
	m_ZStream.avail_in = 0;

	do
	{
		m_ZStream.next_out = (Bytef*)&m_OutBuf[0];
		m_ZStream.avail_out = BUFFER_SIZE;

		deflate(&m_ZStream, Z_SYNC_FLUSH);

		if (m_ZStream.avail_out < BUFFER_SIZE)
			m_Stream->Write(&m_OutBuf[0], BUFFER_SIZE - m_ZStream.avail_out);
	}
	while (m_ZStream.avail_out == 0);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ZlibCompressionStream

ZlibCompressionStream::ZlibCompressionStream(Stream *a_Stream, int Level) :
	OverlayStream(a_Stream),
	pimpl(new ZlibCompressionStream_pimpl(a_Stream, Level))
{
}

ZlibCompressionStream::~ZlibCompressionStream()
{
	pimpl.reset();
}

void ZlibCompressionStream::Write(const void *Data, size_t Size)
{
	pimpl->Write(Data, Size);
}

void ZlibCompressionStream::Flush()
{
	pimpl->Flush();
}

size_t ZlibCompressionStream::CompressLength(size_t DataLength)
{
	return compressBound(DataLength);
}

size_t ZlibCompressionStream::Compress(void *OutData, size_t BufLength, const void *Data, size_t DataLength, int Level)
{
	uLongf DestLen = BufLength;
	int R = compress2((Bytef*)OutData, &DestLen, (const Bytef*)Data, DataLength, Level);
	if (R != Z_OK)
		throw ZlibError(R, "Nie mo�na skompresowa� danych zlib", __FILE__, __LINE__);
	return DestLen;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GzipCompressionStream

GzipCompressionStream::GzipCompressionStream(Stream *a_Stream, const string *FileName, const string *Comment, int Level) :
	OverlayStream(a_Stream),
	pimpl(new ZlibCompressionStream_pimpl(a_Stream, FileName, Comment, Level))
{
}

GzipCompressionStream::~GzipCompressionStream()
{
	pimpl.reset();
}

void GzipCompressionStream::Write(const void *Data, size_t Size)
{
	pimpl->Write(Data, Size);
}

void GzipCompressionStream::Flush()
{
	pimpl->Flush();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ZlibDecompressionStream_pimpl

class ZlibDecompressionStream_pimpl
{
private:
	Stream *m_Stream;
	z_stream m_ZStream;
	std::vector<char> m_InBuf;
	size_t m_InBufLen;
	bool m_OutEnd;
	gz_header m_Header;
	std::vector<char> m_Header_FileName;
	std::vector<char> m_Header_Comment;

	void EnsureBuf(Stream *a_Stream);

public:
	// Wersja zlib
	ZlibDecompressionStream_pimpl(Stream *a_Stream, bool Gzip);
	// Wersja gzip
	~ZlibDecompressionStream_pimpl();

	size_t Read(void *Out, size_t MaxLength);
	bool End();

	bool GetHeaderFileName(string *OutFileName);
	bool GetHeaderComment(string *OutComment);
};

void ZlibDecompressionStream_pimpl::EnsureBuf(Stream *a_Stream)
{
	assert(m_InBufLen == 0);
	m_InBufLen = a_Stream->Read(&m_InBuf[0], BUFFER_SIZE);
}

ZlibDecompressionStream_pimpl::ZlibDecompressionStream_pimpl(Stream *a_Stream, bool Gzip) :
	m_Stream(a_Stream)
{
	m_InBuf.resize(BUFFER_SIZE);
	m_InBufLen = 0;

	m_ZStream.zalloc = Z_NULL;
	m_ZStream.zfree = Z_NULL;
	m_ZStream.opaque = Z_NULL;

	EnsureBuf(m_Stream);
	m_ZStream.next_in = (Bytef*)&m_InBuf[0];
	m_ZStream.avail_in = m_InBufLen;

	// Tak te� mo�e by�
	//m_ZStream.next_in = 0;
	//m_ZStream.avail_in = Z_NULL;

	if (Gzip)
	{
		int R = inflateInit2(&m_ZStream, 15 + 16);
		if (R != Z_OK)
			throw ZlibError(R, "Nie mo�na zainicjalizowa� dekompresji gzip", __FILE__, __LINE__);

		// Odczytanie nag��wka
		m_Header_FileName.resize(256);
		m_Header_Comment.resize(256);
		m_Header.extra = Z_NULL;
		m_Header.extra_len = 0;
		m_Header.name = (Bytef*)&m_Header_FileName[0];
		m_Header.name_max = 256;
		m_Header.comment = (Bytef*)&m_Header_Comment[0];
		m_Header.comm_max = 256;
		R = inflateGetHeader(&m_ZStream, &m_Header);
		if (R != Z_OK)
			throw ZlibError(R, "zlib.inflateGetHeader", __FILE__, __LINE__);
	}
	else
	{
		int R = inflateInit(&m_ZStream);
		if (R != Z_OK)
			throw ZlibError(R, "Nie mo�na zainicjalizowa� dekompresji zlib", __FILE__, __LINE__);
	}

	char Foo = '\0';
	m_ZStream.next_out = (Bytef*)&Foo;
	m_ZStream.avail_out = 0;
	
	int R = inflate(&m_ZStream, Z_NO_FLUSH);
	m_OutEnd = (R == Z_STREAM_END);
	// Tego jednak nie, bo tutaj je�li nie Z_STREAM_END, to zwraca Z_BUF_ERROR, bo mamy avail_in=0 i avail_out=0
//	if (R != Z_OK && R != Z_STREAM_END)
//		throw ZlibError(R, "Nie mo�na rozpocz�� dekompresji zlib", __FILE__, __LINE__);
}

ZlibDecompressionStream_pimpl::~ZlibDecompressionStream_pimpl()
{
	inflateEnd(&m_ZStream);
}

size_t ZlibDecompressionStream_pimpl::Read(void *Out, size_t MaxLength)
{
	if (m_OutEnd == true || MaxLength == 0)
		return 0;

	// avail_out wskazuje na liczb� pozosta�ych bajt�w do odczytania w tym wywo�aniu Read.
	//   Wi�c (MaxLength - avail_out) to liczba zwr�conych przez zlib bajt�w w tym wywo�aniu Read.
	// avail_in to liczba niepobranych jeszcze przez zlib danych z bufora InBuf.
	//   Wi�c (InBufLen - avail_in) to liczba przetworznych ju� przez zlib bajt�w danego bufora InBuf.

	// Miejsce do pisania
	m_ZStream.avail_out = MaxLength;
	m_ZStream.next_out = (Bytef*)Out;

	for (;;)
	{
		// Przeczytane ju� pe�ne MaxLength bajt�w
		if (m_ZStream.avail_out == 0)
			return MaxLength;

		// Zapewnij dane do czytania
		if (m_ZStream.avail_in == 0)
		{
			EnsureBuf(m_Stream);
			// Koniec danych wej�ciowych, a wci�� nie koniec strumienia zlib - niew�tpliwy b��d, niedoko�czone dane
			if (m_InBufLen == 0)
				throw Error("B��d strumienia dekompresji zlib: Niedoko�czone dane.", __FILE__, __LINE__);
			m_ZStream.next_in = (Bytef*)&m_InBuf[0];
			m_ZStream.avail_in = m_InBufLen;
		}

		int R = inflate(&m_ZStream, Z_NO_FLUSH);

		// ZlibError wykry� koniec strumienia
		if (R == Z_STREAM_END)
		{
			m_OutEnd = true;
			return MaxLength - m_ZStream.avail_out;
		}
		// B��d
		else if (R != Z_OK)
			throw ZlibError(R, "B��d dekompresji strumienia zlib", __FILE__, __LINE__);
	}
}

bool ZlibDecompressionStream_pimpl::End()
{
	return m_OutEnd;
}

bool ZlibDecompressionStream_pimpl::GetHeaderFileName(string *OutFileName)
{
	if (m_Header.done != 1) return false;
	if (m_Header.name == Z_NULL) return false;

	OutFileName->assign(&m_Header_FileName[0], strnlen(&m_Header_FileName[0], 256));
	return true;
}

bool ZlibDecompressionStream_pimpl::GetHeaderComment(string *OutComment)
{
	if (m_Header.done != 1) return false;
	if (m_Header.comment == Z_NULL) return false;

	OutComment->assign(&m_Header_Comment[0], strnlen(&m_Header_Comment[0], 256));
	return true;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ZlibDecompressionStream

ZlibDecompressionStream::ZlibDecompressionStream(Stream *a_Stream) :
	OverlayStream(a_Stream),
	pimpl(new ZlibDecompressionStream_pimpl(a_Stream, false))
{
}

ZlibDecompressionStream::~ZlibDecompressionStream()
{
	pimpl.reset();
}

size_t ZlibDecompressionStream::Read(void *Out, size_t MaxLength)
{
	return pimpl->Read(Out, MaxLength);
}

bool ZlibDecompressionStream::End()
{
	return pimpl->End();
}

size_t ZlibDecompressionStream::Decompress(void *OutData, size_t BufLength, const void *Data, size_t DataLength)
{
	uLongf DestLen = BufLength;
	int R = uncompress((Bytef*)OutData, &DestLen, (Bytef*)Data, DataLength);
	if (R != Z_OK)
		throw ZlibError(R, "Nie mo�na rozkompresowa� danych zlib", __FILE__, __LINE__);
	return DestLen;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GzipDecompressionStream

GzipDecompressionStream::GzipDecompressionStream(Stream *a_Stream) :
	OverlayStream(a_Stream),
	pimpl(new ZlibDecompressionStream_pimpl(a_Stream, true))
{
}

GzipDecompressionStream::~GzipDecompressionStream()
{
	pimpl.reset();
}

size_t GzipDecompressionStream::Read(void *Out, size_t MaxLength)
{
	return pimpl->Read(Out, MaxLength);
}

bool GzipDecompressionStream::End()
{
	return pimpl->End();
}

bool GzipDecompressionStream::GetHeaderFileName(string *OutFileName)
{
	return pimpl->GetHeaderFileName(OutFileName);
}

bool GzipDecompressionStream::GetHeaderComment(string *OutComment)
{
	return pimpl->GetHeaderComment(OutComment);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa GzipFileStream

class GzipFileStream_pimpl
{
public:
	gzFile File;
	char OneCharBuf;
	bool m_End;
};

GzipFileStream::GzipFileStream(const string &FileName, GZIP_FILE_MODE Mode, int Level) :
	pimpl(new GzipFileStream_pimpl)
{
	char ModeSz[4];
	ModeSz[0] = (Mode == GZFM_WRITE ? 'w' : 'r');
	ModeSz[1] = 'b';
	if (Level == Z_DEFAULT_COMPRESSION)
		ModeSz[2] = '\0';
	else
	{
		ModeSz[2] = (char)(Level + '0');
		ModeSz[3] = '\0';
	}

	pimpl->File = gzopen(FileName.c_str(), ModeSz);

	if (pimpl->File == NULL)
	{
		if (errno == 0)
			throw ZlibError(Z_MEM_ERROR, "Nie mo�na otworzy� pliku gzip: " + FileName, __FILE__, __LINE__);
		else
			throw ErrnoError("Nie mo�na otworzy� pliku gzip: " + FileName, __FILE__, __LINE__);
	}

	if (Mode == GZFM_READ)
	{
		int R = gzread(pimpl->File, &pimpl->OneCharBuf, 1);
		if (R == -1)
			throw Error("Nie mo�na odczyta� pierwszego bajtu z pliku gzip", __FILE__, __LINE__);
		pimpl->m_End = (R == 0);
	}
}

GzipFileStream::~GzipFileStream()
{
	gzclose(pimpl->File);
}

void GzipFileStream::Write(const void *Data, size_t Size)
{
	size_t Written = gzwrite(pimpl->File, Data, Size);
	if (Written != Size)
		throw Error(Format("Nie mo�na zapisa� do pliku gzip - zapisano #/# B") % Written % Size, __FILE__, __LINE__);
}

void GzipFileStream::Flush()
{
	int R = gzflush(pimpl->File, Z_SYNC_FLUSH);
	if (R != Z_OK)
		throw ZlibError(R, "zlib.GzipFileStream.Flush", __FILE__, __LINE__);
}

size_t GzipFileStream::Read(void *Out, size_t MaxLength)
{
	if (pimpl->m_End) return 0;
	if (MaxLength == 0) return 0;

	char *Out2 = (char*)Out;
	*Out2 = pimpl->OneCharBuf;
	MaxLength--;
	Out2++;

	int BytesRead = gzread(pimpl->File, Out2, MaxLength);

	if (BytesRead == -1)
		throw Error(Format("Nie mo�na odczyta� # bajt�w z pliku gzip") % (MaxLength+1), __FILE__, __LINE__);
	else if ((size_t)BytesRead < MaxLength)
		pimpl->m_End = 0;
	else
	{
		int R = gzread(pimpl->File, &pimpl->OneCharBuf, 1);
		if (R == -1)
			throw Error("Nie mo�na odczyta� bajtu z pliku gzip", __FILE__, __LINE__);
		pimpl->m_End = (R == 0);
	}

	return (size_t)BytesRead + 1;
}

bool GzipFileStream::End()
{
	return pimpl->m_End;
}

} // namespace common
