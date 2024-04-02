/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Stream - Hierarchia klas strumieni
 * Dokumentacja: Patrz plik doc/Stream.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include <typeinfo>
#include <memory.h> // dla memcpy
#include "Error.hpp"
#include "Stream.hpp"


namespace common
{

const size_t BUFFER_SIZE = 4096;

const char * const ERRMSG_DECODE_INVALID_CHAR = "B��d dekodowania strumienia: Nieprawid�owy znak.";
const char * const ERRMSG_UNEXPECTED_END      = "B��d strumienia: Nieoczekiwany koniec danych.";

const char BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje globalne wewn�trzne

// 0 = normalny znak Base64
// 1 = znak '='
// 2 = znak nieznany (bia�y lub nie)
inline int Base64CharType(char Ch)
{
	if ( (Ch >= 'A' && Ch <= 'Z') || (Ch >= 'a' && Ch <= 'z') || (Ch >= '0' && Ch <= '9') || Ch == '+' || Ch == '/' )
		return 0;
	else if (Ch == '=')
		return 1;
	else
		return 2;
}

// Zwraca numer odpowiadaj�cy znakowi w base64.
// Je�li to znak '=', zwraca 0xFE.
// Je�li znak nieznany, zwraca 0xFF.
inline uint1 Base64CharToNumber(char Ch)
{
	if (Ch >= 'A' && Ch <= 'Z')
		return (uint1)(Ch - 'A');
	else if (Ch >= 'a' && Ch <= 'z')
		return (uint1)(Ch - ('a' - 26));
	else if (Ch >= '0' && Ch <= '9')
		return (uint1)(Ch - ('0' - 52));
	else if (Ch == '+')
		return 62;
	else if (Ch == '/')
		return 63;
	else if (Ch == '=')
		return 0xFE;
	else
		return 0xFF;
}

void _ThrowBufEndError(const char *File, int Line)
{
	throw Error("Napotkano koniec strumienia.", File, Line);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Stream

void Stream::Write(const void *Data, size_t Size)
{
	throw Error(Format("Strumie� klasy # nie obs�uguje zapisywnia") % typeid(this).name(), __FILE__, __LINE__);
}

void Stream::WriteString1(const string &s)
{
	typedef uint1 T;
	if (s.length() > static_cast<size_t>(std::numeric_limits<T>::max()))
		throw Error(Format("Nie mo�na zapisa� do strumienia �a�cucha - d�u�szy, ni� # znak�w") % static_cast<size_t>(std::numeric_limits<T>::max()), __FILE__, __LINE__);
	T Length = static_cast<T>(s.length());
	Write(&Length, sizeof(T));
	Write(s.data(), s.length());
}

void Stream::WriteString2(const string &s)
{
	typedef uint2 T;
	if (s.length() > static_cast<size_t>(std::numeric_limits<T>::max()))
		throw Error(Format("Nie mo�na zapisa� do strumienia �a�cucha - d�u�szy, ni� # znak�w") % static_cast<size_t>(std::numeric_limits<T>::max()), __FILE__, __LINE__);
	T Length = static_cast<T>(s.length());
	Write(&Length, sizeof(T));
	Write(s.data(), s.length());
}

void Stream::WriteString4(const string &s)
{
	size_t Length = s.length();
	Write(&Length, sizeof(Length));
	Write(s.data(), s.length());
}

void Stream::WriteStringF(const string &s)
{
	Write(s.data(), s.length());
}

void Stream::WriteBool(bool b)
{
	uint1 bt = (b ? 1 : 0);
	Write(&bt, sizeof(bt));
}

size_t Stream::Read(void *Data, size_t Size)
{
	throw Error(Format("Strumie� klasy # nie obs�uguje odczytywania") % typeid(this).name(), __FILE__, __LINE__);
}

void Stream::MustRead(void *Data, size_t Size)
{
	if (Size == 0) return;
	size_t i = Read(Data, Size);
	if (i != Size)
		throw Error(Format("Odczytano ze strumienia #/# B") % i % Size, __FILE__, __LINE__);
}

size_t Stream::Skip(size_t MaxLength)
{
	// Implementacja dla klasy Stream nie posiadaj�cej kursora.
	// Trzeba wczytywa� dane po kawa�ku.
	// MaxLength b�dzie zmniejszane. Oznacza liczb� pozosta�ych bajt�w do pomini�cia.

	if (MaxLength == 0)
		return 0;

	char Buf[BUFFER_SIZE];
	uint BlockSize, ReadSize, Sum = 0;
	while (MaxLength > 0)
	{
		BlockSize = std::min(MaxLength, BUFFER_SIZE);
		ReadSize = Read(Buf, BlockSize);
		Sum += ReadSize;
		MaxLength -= ReadSize;
		if (ReadSize < BlockSize)
			break;
	}
	return Sum;
}

void Stream::ReadString1(string *s)
{
	typedef uint1 T;
	T Length;
	MustRead(&Length, sizeof(T));
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[Length]);
	MustRead(Buf.get(), Length);
	s->clear();
	s->append(Buf.get(), Length);
}

void Stream::ReadString2(string *s)
{
	typedef uint2 T;
	T Length;
	MustRead(&Length, sizeof(T));
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[Length]);
	MustRead(Buf.get(), Length);
	s->clear();
	s->append(Buf.get(), Length);
}

void Stream::ReadString4(string *s)
{
	typedef uint4 T;
	T Length;
	MustRead(&Length, sizeof(T));
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[Length]);
	MustRead(Buf.get(), Length);
	s->clear();
	s->append(Buf.get(), Length);
}

void Stream::ReadStringF(string *s, size_t Length)
{
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[Length]);
	MustRead(Buf.get(), Length);
	s->clear();
	s->append(Buf.get(), Length);
}

void Stream::ReadStringToEnd(string *s)
{
	s->clear();
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[BUFFER_SIZE]);
	size_t Size = BUFFER_SIZE;
	do
	{
		Size = Read(Buf.get(), Size);
		if (Size > 0)
			s->append(Buf.get(), Size);
	} while (Size == BUFFER_SIZE);
}

void Stream::ReadBool(bool *b)
{
	uint1 bt;
	MustRead(&bt, sizeof(bt));
	*b = (bt != 0);
}

void Stream::MustSkip(size_t Length)
{
	if (Skip(Length) != Length)
		throw Error(Format("Nie mo�na pomin�� # bajt�w - wcze�niej napotkany koniec strumienia.") % Length, __FILE__, __LINE__);
}

bool Stream::End()
{
	throw Error(Format("Strumie� klasy # nie obs�uguje sprawdzania ko�ca") % typeid(this).name(), __FILE__, __LINE__);
}

size_t Stream::CopyFrom(Stream *s, size_t Size)
{
	if (Size == 0) return 0;
	// Size - liczba bajt�w, jaka zosta�a do odczytania
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[BUFFER_SIZE]);
	size_t ReqSize, ReadSize, BytesRead = 0;
	do
	{
		ReadSize = ReqSize = std::min(BUFFER_SIZE, Size);
		ReadSize = s->Read(Buf.get(), ReadSize);
		if (ReadSize > 0)
		{
			Write(Buf.get(), ReadSize);
			Size -= ReadSize;
			BytesRead += ReadSize;
		}
	} while (ReadSize == ReqSize && Size > 0);
	return BytesRead;
}

void Stream::MustCopyFrom(Stream *s, size_t Size)
{
	if (Size == 0) return;
	// Size - liczba bajt�w, jaka zosta�a do odczytania
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[BUFFER_SIZE]);
	size_t ReqSize;
	do
	{
		ReqSize = std::min(BUFFER_SIZE, Size);
		s->MustRead(Buf.get(), ReqSize);
		if (ReqSize > 0)
		{
			Write(Buf.get(), ReqSize);
			Size -= ReqSize;
		}
	} while (Size > 0);
}

void Stream::CopyFromToEnd(Stream *s)
{
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[BUFFER_SIZE]);
	size_t Size = BUFFER_SIZE;
	do
	{
		Size = s->Read(Buf.get(), Size);
		if (Size > 0)
			Write(Buf.get(), Size);
	} while (Size == BUFFER_SIZE);
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa SeekableStream

size_t SeekableStream::GetSize()
{
	throw Error(Format("Strumie� klasy # nie obs�uguje zwracania rozmiaru") % typeid(this).name(), __FILE__, __LINE__);
}

int SeekableStream::GetPos()
{
	throw Error(Format("Strumie� klasy # nie obs�uguje zwracania pozycji kursora") % typeid(this).name(), __FILE__, __LINE__);
}

void SeekableStream::SetPos(int pos)
{
	throw Error(Format("Strumie� klasy # nie obs�uguje ustawiania pozycji kursora") % typeid(this).name(), __FILE__, __LINE__);
}

void SeekableStream::SetPosFromCurrent(int pos)
{
	SetPos(GetPos()+pos);
}

void SeekableStream::SetPosFromEnd(int pos)
{
	SetPos((int)GetSize()+pos);
}

void SeekableStream::Rewind()
{
	SetPos(0);
}

void SeekableStream::SetSize(size_t Size)
{
	throw Error(Format("Strumie� klasy # nie obs�uguje ustawiania rozmiaru") % typeid(this).name(), __FILE__, __LINE__);
}

void SeekableStream::Truncate()
{
	SetSize((size_t)GetPos());
}

void SeekableStream::Clear()
{
	SetSize(0);
}

bool SeekableStream::End()
{
	return (GetPos() == (int)GetSize());
}

size_t SeekableStream::Skip(size_t MaxLength)
{
	uint Pos = GetPos();
	uint Size = GetSize();
	uint BytesToSkip = std::min(MaxLength, Size - Pos);
	SetPosFromCurrent((int)BytesToSkip);
	return BytesToSkip;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa CharWriter

CharWriter::~CharWriter()
{
	try
	{
		Flush();
	}
	catch (...)
	{
		assert(0 && "Wyj�tek z�apany w CharWriter::~CharWriter podczas wykonywania CharWriter::Flush.");
	}
}

void CharWriter::WriteString(const string &s)
{
	uint BlockLength, i, s_i = 0, Length = s.length();
	// Length b�dzie zmniejszany. Oznacza liczb� pozosta�ych do zapisania bajt�w.

	while (Length > 0)
	{
		// Bufor pe�ny
		if (m_BufIndex == BUFFER_SIZE)
			DoFlush();
		// Liczba bajt�w do dopisania do bufora
		BlockLength = std::min(BUFFER_SIZE - m_BufIndex, Length);
		// Dopisanie do bufora
		for (i = 0; i < BlockLength; i++, m_BufIndex++, s_i++)
			m_Buf[m_BufIndex] = s[s_i];
		Length -= BlockLength;
	}
}

void CharWriter::WriteData(const void *Data, size_t Length)
{
	const char *CharData = (const char*)Data;
	uint BlockLength;
	// Length b�dzie zmniejszany. Oznacza liczb� pozosta�ych do zapisania bajt�w.
	// CharData b�dzie przesuwany.

	while (Length > 0)
	{
		// Bufor pe�ny
		if (m_BufIndex == BUFFER_SIZE)
			DoFlush();
		// Liczba bajt�w do dopisania do bufora
		BlockLength = std::min(BUFFER_SIZE - m_BufIndex, Length);
		// Dopisanie do bufora
		memcpy(&m_Buf[m_BufIndex], CharData, BlockLength);

		CharData += BlockLength;
		m_BufIndex += BlockLength;
		Length -= BlockLength;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa CharReader

bool CharReader::EnsureNewChars()
{
	assert(m_BufBeg == m_BufEnd);
	uint ReadSize = m_Stream->Read(&m_Buf[0], BUFFER_SIZE);
	m_BufBeg = 0;
	m_BufEnd = ReadSize;
	return (ReadSize > 0);
}

size_t CharReader::ReadString(string *Out, size_t MaxLength)
{
	uint BlockSize, i, Sum = 0, Out_i = 0;
	Out->clear();

	// MaxLength b�dzie zmniejszane.
	// Tu nie mog� zrobi� Out->resize(MaxLength), bo jako MaxLength podaje si� cz�sto ogromne liczby.

	while (MaxLength > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				return Sum;
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, MaxLength);
		Out->resize(Out_i + BlockSize);
		for (i = 0; i < BlockSize; i++)
		{
			(*Out)[Out_i] = m_Buf[m_BufBeg];
			Out_i++;
			m_BufBeg++;
		}
		MaxLength -= BlockSize;
		Sum += BlockSize;
	}
	return Sum;
}

void CharReader::MustReadString(string *Out, size_t Length)
{
	uint BlockSize, i, Out_i = 0;
	Out->clear();
	Out->resize(Length);

	// Length b�dzie zmniejszane.

	while (Length > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				_ThrowBufEndError(__FILE__, __LINE__);
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, Length);
		for (i = 0; i < BlockSize; i++)
		{
			(*Out)[Out_i] = m_Buf[m_BufBeg];
			Out_i++;
			m_BufBeg++;
		}
		Length -= BlockSize;
	}
}

size_t CharReader::ReadString(char *Out, size_t MaxLength)
{
	uint BlockSize, i, Sum = 0;

	// MaxLength b�dzie zmniejszane.
	// Out b�dzie przesuwane.

	while (MaxLength > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				return Sum;
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, MaxLength);
		for (i = 0; i < BlockSize; i++)
		{
			*Out = m_Buf[m_BufBeg];
			Out++;
			m_BufBeg++;
		}
		MaxLength -= BlockSize;
		Sum += BlockSize;
	}
	return Sum;
}

void CharReader::MustReadString(char *Out, size_t Length)
{
	uint BlockSize, i;

	// Length b�dzie zmniejszane.
	// Out b�dzie przesuwane.

	while (Length > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				_ThrowBufEndError(__FILE__, __LINE__);
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, Length);
		for (i = 0; i < BlockSize; i++)
		{
			*Out = m_Buf[m_BufBeg];
			Out++;
			m_BufBeg++;
		}
		Length -= BlockSize;
	}
}

size_t CharReader::ReadData(void *Out, size_t MaxLength)
{
	char *OutChars = (char*)Out;
	uint BlockSize, i, Sum = 0;

	// MaxLength b�dzie zmniejszane.
	// OutChars b�dzie przesuwane.

	while (MaxLength > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				return Sum;
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, MaxLength);
		for (i = 0; i < BlockSize; i++)
		{
			*OutChars = m_Buf[m_BufBeg];
			OutChars++;
			m_BufBeg++;
		}
		MaxLength -= BlockSize;
		Sum += BlockSize;
	}
	return Sum;
}

void CharReader::MustReadData(void *Out, size_t Length)
{
	char *OutChars = (char*)Out;
	uint BlockSize, i;

	// Length b�dzie zmniejszane.
	// OutChars b�dzie przesuwane.

	while (Length > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				_ThrowBufEndError(__FILE__, __LINE__);
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, Length);
		for (i = 0; i < BlockSize; i++)
		{
			*OutChars = m_Buf[m_BufBeg];
			OutChars++;
			m_BufBeg++;
		}
		Length -= BlockSize;
	}
}

size_t CharReader::Skip(size_t MaxLength)
{
	uint BlockSize, Sum = 0;

	// MaxLength b�dzie zmniejszane.

	while (MaxLength > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				return Sum;
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, MaxLength);
		m_BufBeg += BlockSize;
		MaxLength -= BlockSize;
		Sum += BlockSize;
	}
	return Sum;
}

void CharReader::MustSkip(size_t Length)
{
	uint BlockSize;

	// Length b�dzie zmniejszane.

	while (Length > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				_ThrowBufEndError(__FILE__, __LINE__);
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, Length);
		m_BufBeg += BlockSize;
		Length -= BlockSize;
	}
}

bool CharReader::ReadLine(string *Out)
{
	Out->clear();
	char Ch;
	bool WasEol = false;
	while (ReadChar(&Ch))
	{
		if (Ch == '\r')
		{
			WasEol = true;
			if (PeekChar(&Ch) && Ch == '\n')
				MustSkipChar();
			break;
		}
		else if (Ch == '\n')
		{
			WasEol = true;
			break;
		}
		else
			(*Out) += Ch;
	}
	return !Out->empty() || WasEol;
}

void CharReader::MustReadLine(string *Out)
{
	Out->clear();
	char Ch;
	while (ReadChar(&Ch))
	{
		if (Ch == '\r')
		{
			if (PeekChar(&Ch) && Ch == '\n')
				MustSkipChar();
			break;
		}
		else if (Ch == '\n')
			break;
		else
			(*Out) += Ch;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa MemoryStream

MemoryStream::MemoryStream(size_t Size, void *Data)
{
	m_Data = (char*)Data;
	m_Size = Size;
	m_Pos = 0;
	m_InternalAlloc = (m_Data == 0);

	if (m_InternalAlloc)
		m_Data = new char[m_Size];
}

MemoryStream::~MemoryStream()
{
	if (m_InternalAlloc)
		delete [] m_Data;
}

void MemoryStream::Write(const void *Data, size_t Size)
{
	if (m_Pos >= 0 && m_Pos + (int)Size <= (int)m_Size)
	{
		memcpy(&m_Data[m_Pos], Data, Size);
		m_Pos += (int)Size;
	}
	else
		throw Error(Format("Nie mo�na zapisa� # bajt�w do strumienia pami�ciowego - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Size, __FILE__, __LINE__);
}

size_t MemoryStream::Read(void *Data, size_t Size)
{
	if (m_Pos >= 0 && m_Pos <= (int)m_Size)
	{
		Size = std::min(Size, m_Size-m_Pos);
		memcpy(Data, &m_Data[m_Pos], Size);
		m_Pos += (int)Size;
	}
	else
		throw Error(Format("Nie mo�na odczyta� # bajt�w ze strumienia pami�ciowego - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Size, __FILE__, __LINE__);
	return Size;
}

void MemoryStream::MustRead(void *Data, size_t Size)
{
	if (m_Pos >= 0 && m_Pos + (int)Size <= (int)m_Size)
	{
		memcpy(Data, &m_Data[m_Pos], Size);
		m_Pos += (int)Size;
	}
	else
		throw Error(Format("Nie mo�na odczyta� (2) # bajt�w ze strumienia pami�ciowego - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Size, __FILE__, __LINE__);
}

size_t MemoryStream::GetSize()
{
	return m_Size;
}

int MemoryStream::GetPos()
{
	return m_Pos;
}

void MemoryStream::SetPos(int pos)
{
	m_Pos = pos;
}

void MemoryStream::Rewind()
{
	m_Pos = 0;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa VectorStream

void VectorStream::Reserve(size_t NewCapacity)
{
	m_Capacity = NewCapacity;
	char *NewData = new char[m_Capacity];
	memcpy(NewData, m_Data, m_Size);
	delete [] m_Data;
	m_Data = NewData;
}

VectorStream::VectorStream()
{
	m_Size = 0;
	m_Capacity = 8;
	m_Data = new char[8];
	m_Pos = 0;
}

VectorStream::~VectorStream()
{
	delete [] m_Data;
}

void VectorStream::Write(const void *Data, size_t Size)
{
	if (m_Capacity < m_Pos + Size)
		Reserve(std::max(m_Pos + Size, m_Capacity + m_Capacity / 4));
	if (m_Size < m_Pos + Size)
		m_Size = m_Pos + Size;
	memcpy(&m_Data[m_Pos], Data, Size);
	m_Pos += (int)Size;
}

size_t VectorStream::Read(void *Data, size_t Size)
{
	if (m_Pos >= 0 && m_Pos <= (int)m_Size)
	{
		Size = std::min(Size, m_Size-m_Pos);
		memcpy(Data, &m_Data[m_Pos], Size);
		m_Pos += (int)Size;
	}
	else
		throw Error(Format("Nie mo�na odczyta� # bajt�w ze strumienia VectorStream - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Size, __FILE__, __LINE__);
	return Size;
}

void VectorStream::MustRead(void *Data, size_t Size)
{
	if (m_Pos >= 0 && m_Pos + (int)Size <= (int)m_Size)
	{
		memcpy(Data, &m_Data[m_Pos], Size);
		m_Pos += (int)Size;
	}
	else
		throw Error(Format("Nie mo�na odczyta� (2) # bajt�w ze strumienia VectorStream - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Size, __FILE__, __LINE__);
}

void VectorStream::SetSize(size_t Size)
{
	// zmniejszamy
	if (Size < m_Size)
	{
		m_Size = Size;
		// je�li po�owa miejsca si� marnuje - pomniejsz
		if (m_Capacity >= Size*2)
		{
			size_t NewCapacity = std::max((size_t)8, m_Size);
			if (NewCapacity != m_Capacity)
				Reserve(NewCapacity);
		}
	}
	// zwi�kszamy
	else if (Size > m_Size)
	{
		m_Size = Size;
		// je�li brakuje miejsca - powi�ksz
		if (m_Size > m_Capacity)
			Reserve(std::max(m_Size, m_Capacity + m_Capacity / 4));
	}
}

void VectorStream::SetCapacity(size_t Capacity)
{
	if (Capacity < m_Size || Capacity == 0)
		throw Error(Format("Nie mo�na zmieni� pojemno�ci strumienia VectorStream (rozmiar: #, ��dana pojemno��: #)") % m_Size % Capacity, __FILE__, __LINE__);

	if (Capacity != m_Capacity)
		Reserve(Capacity);
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa StringStream

StringStream::StringStream(string *Data)
{
	m_Data = Data;
	m_Pos = 0;
	m_InternalAlloc = (m_Data == 0);

	if (m_InternalAlloc)
		m_Data = new string;
}

StringStream::~StringStream()
{
	if (m_InternalAlloc)
		delete m_Data;
}

void StringStream::Write(const void *Data, size_t Size)
{
	if (m_Pos + Size > m_Data->length())
		m_Data->resize(m_Pos + Size);

	for (size_t i = 0; i < Size; i++)
		(*m_Data)[m_Pos+i] = reinterpret_cast<const char*>(Data)[i];

	m_Pos += (int)Size;
}

size_t StringStream::Read(void *Data, size_t Size)
{
	if (m_Pos < 0)
		throw Error(Format("Nie mo�na zapisa� # bajt�w do strumienia StringStream - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Data->length(), __FILE__, __LINE__);

	Size = std::min(Size, m_Data->length()-m_Pos);
	for (size_t i = 0; i < Size; i++)
	{
		reinterpret_cast<char*>(Data)[i] = (*m_Data)[m_Pos];
		m_Pos++;
	}
	return Size;
}

void StringStream::MustRead(void *Data, size_t Size)
{
	if (m_Pos < 0 || m_Pos + Size > m_Data->length())
		throw Error(Format("Nie mo�na odczyta� (2) # bajt�w ze strumienia StringStream - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Data->length(), __FILE__, __LINE__);

	for (size_t i = 0; i < Size; i++)
		reinterpret_cast<char*>(Data)[i] = (*m_Data)[m_Pos+i];

	m_Pos += (int)Size;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa CounterOverlayStream

CounterOverlayStream::CounterOverlayStream(Stream *a_Stream) :
	OverlayStream(a_Stream),
	m_WriteCounter(0),
	m_ReadCounter(0)
{
}

void CounterOverlayStream::Write(const void *Data, size_t Size)
{
	GetStream()->Write(Data, Size);
	m_WriteCounter += Size;
}

size_t CounterOverlayStream::Read(void *Data, size_t Size)
{
	size_t BytesRead = GetStream()->Read(Data, Size);
	m_ReadCounter += BytesRead;
	return BytesRead;
}

void CounterOverlayStream::MustRead(void *Data, size_t Size)
{
	GetStream()->MustRead(Data, Size);
	m_ReadCounter += Size;
}

void CounterOverlayStream::Flush()
{
	GetStream()->Flush();
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa LimitOverlayStream

LimitOverlayStream::LimitOverlayStream(Stream *a_Stream, uint4 WriteLimit, uint4 ReadLimit) :
	OverlayStream(a_Stream),
	m_WriteLimit(WriteLimit),
	m_ReadLimit(ReadLimit)
{
}

const char * const LIMIT_OVERLAY_WRITE_ERRMSG = "LimitOverlayStream: Limit zapisu przekroczony";

void LimitOverlayStream::Write(const void *Data, size_t Size)
{
	if (m_WriteLimit == 0)
		throw Error(LIMIT_OVERLAY_WRITE_ERRMSG);
	else if (Size <= m_WriteLimit)
	{
		GetStream()->Write(Data, Size);
		m_WriteLimit -= Size;
	}
	else
	{
		GetStream()->Write(Data, m_WriteLimit);
		m_WriteLimit = 0;
		throw Error(LIMIT_OVERLAY_WRITE_ERRMSG);
	}
}

size_t LimitOverlayStream::Read(void *Data, size_t Size)
{
	if (m_ReadLimit == 0)
		return 0;
	else if (Size <= m_ReadLimit)
	{
		size_t BytesRead = GetStream()->Read(Data, Size);
		m_ReadLimit -= BytesRead;
		return BytesRead;
	}
	else
	{
		size_t BytesRead = GetStream()->Read(Data, m_ReadLimit);
		m_ReadLimit -= BytesRead;
		return BytesRead;
	}
}

void LimitOverlayStream::Flush()
{
	GetStream()->Flush();
}

void LimitOverlayStream::SetWriteLimit(uint4 WriteLimit)
{
	m_WriteLimit = WriteLimit;
}

void LimitOverlayStream::SetReadLimit(uint4 ReadLimit)
{
	m_ReadLimit = ReadLimit;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa MultiWriterStream

MultiWriterStream::MultiWriterStream(Stream *a_Streams[], uint StreamCount)
{
	m_Streams.resize(StreamCount);
	for (uint i = 0; i < StreamCount; i++)
		m_Streams[i] = a_Streams[i];
}

void MultiWriterStream::RemoveStream(Stream *a_Stream)
{
	for (std::vector<Stream*>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
	{
		if (*it == a_Stream)
		{
			m_Streams.erase(it);
			break;
		}
	}
}

void MultiWriterStream::Write(const void *Data, size_t Size)
{
	ERR_TRY;

	for (uint i = 0; i < m_Streams.size(); i++)
		m_Streams[i]->Write(Data, Size);

	ERR_CATCH_FUNC;
}

void MultiWriterStream::Flush()
{
	ERR_TRY;

	for (uint i = 0; i < m_Streams.size(); i++)
		m_Streams[i]->Flush();

	ERR_CATCH_FUNC;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Hash_Calc

void Hash_Calc::Write(const void *Data, size_t Size)
{
	uint1 *ByteData = (uint1*)Data; // B�dzie przesuwany

	for (size_t i = 0; i < Size; i++)
	{
		m_Hash += *ByteData;
		m_Hash += (m_Hash << 10);
		m_Hash ^= (m_Hash >> 6);

		ByteData++;
	}
}

uint4 Hash_Calc::Finish()
{
	m_Hash += (m_Hash << 3);
	m_Hash ^= (m_Hash >> 11);
	m_Hash += (m_Hash << 15);

	return m_Hash;
}

uint4 Hash_Calc::Calc(const void *Buf, uint4 BufLen)
{
	uint4 Hash = 0;

	uint1 *ByteData = (uint1*)Buf; // B�dzie przesuwany

	for (size_t i = 0; i < BufLen; i++)
	{
		Hash += *ByteData;
		Hash += (Hash << 10);
		Hash ^= (Hash >> 6);

		ByteData++;
	}

	Hash += (Hash << 3);
	Hash ^= (Hash >> 11);
	Hash += (Hash << 15);

	return Hash;
}

uint4 Hash_Calc::Calc(const string &s)
{
	uint4 Hash = 0;

	for (size_t i = 0; i < s.length(); i++)
	{
		Hash += (uint1)s[i];
		Hash += (Hash << 10);
		Hash ^= (Hash >> 6);
	}

	Hash += (Hash << 3);
	Hash ^= (Hash >> 11);
	Hash += (Hash << 15);

	return Hash;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa CRC32_Calc

/*
CRC32 - 32bit Cyclic Redundancy Check
Sprawdzona, dzia�a dobrze, zgodnie ze standardem.
Na podstawie:
  "eXtensible Data Stream 3.0", Mark T. Price
  Appendix B. Reference CRC-32 Implementation
*/

const uint4 CRC32_TABLE[256] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
	0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
	0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
	0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
	0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
	0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
	0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
	0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
	0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
	0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
	0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
	0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
	0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
	0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
	0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
	0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
	0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
	0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
	0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
	0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
	0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
	0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

void CRC32_Calc::Write(const void *Data, size_t Size)
{
	uint4 k;
	uint1 *DataBytes = (uint1*)Data;
	while (Size > 0)
	{
		k = (m_CRC ^ (*DataBytes)) & 0x000000FF;
		m_CRC = ((m_CRC >> 8) & 0x00FFFFFF) ^ CRC32_TABLE[k];

		DataBytes++;
		Size--;
	}
}

uint CRC32_Calc::Calc(const void *Data, size_t DataLength)
{
	uint4 crc = 0xFFFFFFFF; // preconditioning sets non zero value

	// loop through the buffer and calculate CRC
	for(uint4 i = 0; i < DataLength; ++i)
	{
		int k = (crc ^ ((uint1*)(Data))[i]) & 0x000000FF;
		crc = ((crc >> 8) & 0x00FFFFFF) ^ CRC32_TABLE[k];
	}

	crc = ~crc; // postconditioning

	// report results
	return crc;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Struktura MD5_SUM, klasa MD5_Calc itp.

/*
MD5 - Message-Digest algorithm 5
Sprawdzona, dzia�a dobrze, zgodnie ze standardem.
Implementacja na podstawie biblioteki:
  RFC 1321 compliant MD5 implementation
  Copyright (C) 2003-2006  Christophe Devine
  Licencja: GNU LGPL 2.1
  http://xyssl.org/code/source/md5/
*/

void MD5ToStr(string *Out, const MD5_SUM MD5)
{
	HexEncoder::Encode(Out, MD5.Data, 16);
}

bool StrToMD5(MD5_SUM *Out, const string &s)
{
	return ( HexDecoder::Decode(Out->Data, s) == 16 );
}

#define MD5_GET_UINT32_LE(n,b,i)                \
{                                               \
    (n) = ( (uint4) (b)[(i)    ]       )        \
        | ( (uint4) (b)[(i) + 1] <<  8 )        \
        | ( (uint4) (b)[(i) + 2] << 16 )        \
        | ( (uint4) (b)[(i) + 3] << 24 );       \
}

#define MD5_PUT_UINT32_LE(n,b,i)                \
{                                               \
    (b)[(i)    ] = (uint1) ( (n)       );       \
    (b)[(i) + 1] = (uint1) ( (n) >>  8 );       \
    (b)[(i) + 2] = (uint1) ( (n) >> 16 );       \
    (b)[(i) + 3] = (uint1) ( (n) >> 24 );       \
}

uint1 md5_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void MD5_Calc::Process(uint1 data[64])
{
    uint4 X[16], A, B, C, D;

    MD5_GET_UINT32_LE( X[0],  data,  0 );
    MD5_GET_UINT32_LE( X[1],  data,  4 );
    MD5_GET_UINT32_LE( X[2],  data,  8 );
    MD5_GET_UINT32_LE( X[3],  data, 12 );
    MD5_GET_UINT32_LE( X[4],  data, 16 );
    MD5_GET_UINT32_LE( X[5],  data, 20 );
    MD5_GET_UINT32_LE( X[6],  data, 24 );
    MD5_GET_UINT32_LE( X[7],  data, 28 );
    MD5_GET_UINT32_LE( X[8],  data, 32 );
    MD5_GET_UINT32_LE( X[9],  data, 36 );
    MD5_GET_UINT32_LE( X[10], data, 40 );
    MD5_GET_UINT32_LE( X[11], data, 44 );
    MD5_GET_UINT32_LE( X[12], data, 48 );
    MD5_GET_UINT32_LE( X[13], data, 52 );
    MD5_GET_UINT32_LE( X[14], data, 56 );
    MD5_GET_UINT32_LE( X[15], data, 60 );

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a,b,c,d,k,s,t)                                \
{                                                       \
    a += F(b,c,d) + X[k] + t; a = S(a,s) + b;           \
}

    A = state[0];
    B = state[1];
    C = state[2];
    D = state[3];

#define F(x,y,z) (z ^ (x & (y ^ z)))
    P( A, B, C, D,  0,  7, 0xD76AA478 );
    P( D, A, B, C,  1, 12, 0xE8C7B756 );
    P( C, D, A, B,  2, 17, 0x242070DB );
    P( B, C, D, A,  3, 22, 0xC1BDCEEE );
    P( A, B, C, D,  4,  7, 0xF57C0FAF );
    P( D, A, B, C,  5, 12, 0x4787C62A );
    P( C, D, A, B,  6, 17, 0xA8304613 );
    P( B, C, D, A,  7, 22, 0xFD469501 );
    P( A, B, C, D,  8,  7, 0x698098D8 );
    P( D, A, B, C,  9, 12, 0x8B44F7AF );
    P( C, D, A, B, 10, 17, 0xFFFF5BB1 );
    P( B, C, D, A, 11, 22, 0x895CD7BE );
    P( A, B, C, D, 12,  7, 0x6B901122 );
    P( D, A, B, C, 13, 12, 0xFD987193 );
    P( C, D, A, B, 14, 17, 0xA679438E );
    P( B, C, D, A, 15, 22, 0x49B40821 );
#undef F

#define F(x,y,z) (y ^ (z & (x ^ y)))
    P( A, B, C, D,  1,  5, 0xF61E2562 );
    P( D, A, B, C,  6,  9, 0xC040B340 );
    P( C, D, A, B, 11, 14, 0x265E5A51 );
    P( B, C, D, A,  0, 20, 0xE9B6C7AA );
    P( A, B, C, D,  5,  5, 0xD62F105D );
    P( D, A, B, C, 10,  9, 0x02441453 );
    P( C, D, A, B, 15, 14, 0xD8A1E681 );
    P( B, C, D, A,  4, 20, 0xE7D3FBC8 );
    P( A, B, C, D,  9,  5, 0x21E1CDE6 );
    P( D, A, B, C, 14,  9, 0xC33707D6 );
    P( C, D, A, B,  3, 14, 0xF4D50D87 );
    P( B, C, D, A,  8, 20, 0x455A14ED );
    P( A, B, C, D, 13,  5, 0xA9E3E905 );
    P( D, A, B, C,  2,  9, 0xFCEFA3F8 );
    P( C, D, A, B,  7, 14, 0x676F02D9 );
    P( B, C, D, A, 12, 20, 0x8D2A4C8A );
#undef F

#define F(x,y,z) (x ^ y ^ z)
    P( A, B, C, D,  5,  4, 0xFFFA3942 );
    P( D, A, B, C,  8, 11, 0x8771F681 );
    P( C, D, A, B, 11, 16, 0x6D9D6122 );
    P( B, C, D, A, 14, 23, 0xFDE5380C );
    P( A, B, C, D,  1,  4, 0xA4BEEA44 );
    P( D, A, B, C,  4, 11, 0x4BDECFA9 );
    P( C, D, A, B,  7, 16, 0xF6BB4B60 );
    P( B, C, D, A, 10, 23, 0xBEBFBC70 );
    P( A, B, C, D, 13,  4, 0x289B7EC6 );
    P( D, A, B, C,  0, 11, 0xEAA127FA );
    P( C, D, A, B,  3, 16, 0xD4EF3085 );
    P( B, C, D, A,  6, 23, 0x04881D05 );
    P( A, B, C, D,  9,  4, 0xD9D4D039 );
    P( D, A, B, C, 12, 11, 0xE6DB99E5 );
    P( C, D, A, B, 15, 16, 0x1FA27CF8 );
    P( B, C, D, A,  2, 23, 0xC4AC5665 );
#undef F

#define F(x,y,z) (y ^ (x | ~z))
    P( A, B, C, D,  0,  6, 0xF4292244 );
    P( D, A, B, C,  7, 10, 0x432AFF97 );
    P( C, D, A, B, 14, 15, 0xAB9423A7 );
    P( B, C, D, A,  5, 21, 0xFC93A039 );
    P( A, B, C, D, 12,  6, 0x655B59C3 );
    P( D, A, B, C,  3, 10, 0x8F0CCC92 );
    P( C, D, A, B, 10, 15, 0xFFEFF47D );
    P( B, C, D, A,  1, 21, 0x85845DD1 );
    P( A, B, C, D,  8,  6, 0x6FA87E4F );
    P( D, A, B, C, 15, 10, 0xFE2CE6E0 );
    P( C, D, A, B,  6, 15, 0xA3014314 );
    P( B, C, D, A, 13, 21, 0x4E0811A1 );
    P( A, B, C, D,  4,  6, 0xF7537E82 );
    P( D, A, B, C, 11, 10, 0xBD3AF235 );
    P( C, D, A, B,  2, 15, 0x2AD7D2BB );
    P( B, C, D, A,  9, 21, 0xEB86D391 );
#undef F

#undef P
#undef S

    state[0] += A;
    state[1] += B;
    state[2] += C;
    state[3] += D;
}

MD5_Calc::MD5_Calc()
{
	Reset();
}

void MD5_Calc::Write(const void *Buf, size_t BufLen)
{
	uint1 *ByteBuf = (uint1*)Buf;

	int fill;
	uint4 left;

	if(BufLen == 0)
		return;

	left = total[0] & 0x3F;
	fill = 64 - left;

	total[0] += BufLen;
	total[0] &= 0xFFFFFFFF;

	if(total[0] < BufLen)
		total[1]++;

	if(left && (int)BufLen >= fill)
	{
		memcpy((void*)(buffer + left), (void*)ByteBuf, fill);
		Process(buffer);
		ByteBuf += fill;
		BufLen -= fill;
		left = 0;
	}

	while(BufLen >= 64)
	{
		Process(ByteBuf);
		ByteBuf += 64;
		BufLen -= 64;
	}

	if(BufLen > 0)
		memcpy((void*)(buffer + left), (void*)ByteBuf, BufLen);
}

void MD5_Calc::Finish(MD5_SUM *Out)
{
	uint4 last, padn;
	uint4 high, low;
	uint1 msglen[8];

	high = (total[0] >> 29) | (total[1] <<  3);
	low  = (total[0] <<  3);

	MD5_PUT_UINT32_LE(low,  msglen, 0);
	MD5_PUT_UINT32_LE(high, msglen, 4);

	last = total[0] & 0x3F;
	padn = (last < 56) ? (56 - last) : (120 - last);

	Write(md5_padding, padn);
	Write(msglen, 8);

	MD5_PUT_UINT32_LE(state[0], Out->Data,  0);
	MD5_PUT_UINT32_LE(state[1], Out->Data,  4);
	MD5_PUT_UINT32_LE(state[2], Out->Data,  8);
	MD5_PUT_UINT32_LE(state[3], Out->Data, 12);
}

void MD5_Calc::Reset()
{
	total[0] = 0;
	total[1] = 0;
	state[0] = 0x67452301;
	state[1] = 0xEFCDAB89;
	state[2] = 0x98BADCFE;
	state[3] = 0x10325476;
}

void MD5_Calc::Calc(MD5_SUM *Out, const void *Buf, uint4 BufLen)
{
	MD5_Calc md5;
	md5.Write(Buf, BufLen);
	md5.Finish(Out);
}

#undef MD5_PUT_UINT32_LE
#undef MD5_GET_UINT32_LE


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa XorCoder

XorCoder::XorCoder(Stream *a_Stream, char KeyByte) :
	OverlayStream(a_Stream),
	m_EncodeKeyIndex(0),
	m_DecodeKeyIndex(0)
{
	m_Buf.resize(BUFFER_SIZE);

	m_Key.push_back(KeyByte);
}

XorCoder::XorCoder(Stream *a_Stream, const void *KeyData, size_t KeyLength) :
	OverlayStream(a_Stream),
	m_EncodeKeyIndex(0),
	m_DecodeKeyIndex(0)
{
	m_Buf.resize(BUFFER_SIZE);

	m_Key.resize(KeyLength);
	memcpy(&m_Key[0], KeyData, KeyLength);
}

XorCoder::XorCoder(Stream *a_Stream, const string &Key) :
	OverlayStream(a_Stream),
	m_EncodeKeyIndex(0),
	m_DecodeKeyIndex(0)
{
	m_Buf.resize(BUFFER_SIZE);

	m_Key.resize(Key.length());
	for (uint i = 0; i < Key.length(); i++)
		m_Key[i] = Key[i];
}

void XorCoder::Write(const void *Data, size_t Size)
{
	ERR_TRY;

	const char *Bytes = (const char*)Data;
	uint BlockSize;

	while (Size > 0)
	{
		BlockSize = std::min(Size, BUFFER_SIZE);

		if (m_Key.size() == 1)
		{
			for (size_t i = 0; i < BlockSize; i++)
				m_Buf[i] = Bytes[i] ^ m_Key[0];
		}
		else
		{
			for (size_t i = 0; i < BlockSize; i++)
			{
				m_Buf[i] = Bytes[i] ^ m_Key[m_EncodeKeyIndex];
				m_EncodeKeyIndex = (m_EncodeKeyIndex + 1) % m_Key.size();
			}
		}

		GetStream()->Write(&m_Buf[0], BlockSize);

		Bytes += BlockSize;
		Size -= BlockSize;
	}

	ERR_CATCH_FUNC;
}

size_t XorCoder::Read(void *Data, size_t Size)
{
	ERR_TRY;

	char * Bytes = (char*)Data;
	uint BlockSize, ReadSize, Sum = 0;

	while (Size > 0)
	{
		BlockSize = std::min(Size, BUFFER_SIZE);

		ReadSize = GetStream()->Read(&m_Buf[0], BlockSize);

		if (m_Key.size() == 1)
		{
			for (size_t i = 0; i < BlockSize; i++)
				Bytes[i] = m_Buf[i] ^ m_Key[0];
		}
		else
		{
			for (size_t i = 0; i < BlockSize; i++)
			{
				Bytes[i] = m_Buf[i] ^ m_Key[m_DecodeKeyIndex];
				m_DecodeKeyIndex = (m_DecodeKeyIndex + 1) % m_Key.size();
			}
		}

		Sum += ReadSize;
		if (ReadSize < BlockSize)
			return Sum;
		Bytes += ReadSize;
		Size -= ReadSize;
	}

	return Sum;

	ERR_CATCH_FUNC;
}

void XorCoder::Code(void *Out, const void *Data, size_t DataLength, const void *Key, size_t KeyLength)
{
	char *OutB = (char*)Out;
	const char *DataB = (const char*)Data;
	const char *KeyB = (const char*)Key;

	if (KeyLength == 1)
	{
		for (size_t i = 0; i < DataLength; i++)
		{
			*OutB = *DataB ^ *KeyB;
			OutB++;
			DataB++;
		}
	}
	else
	{
		size_t KeyIndex = 0;
		for (size_t i = 0; i < DataLength; i++)
		{
			*OutB = *DataB ^ KeyB[KeyIndex];
			OutB++;
			DataB++;
			KeyIndex = (KeyIndex + 1) % KeyLength;
		}
	}
}

void XorCoder::Code(string *Out, const string &Data, const string &Key)
{
	Out->resize(Data.length());

	if (Key.length() == 1)
	{
		for (size_t i = 0; i < Data.length(); i++)
			(*Out)[i] = Data[i] ^ Key[0];
	}
	else
	{
		size_t KeyIndex = 0;
		for (size_t i = 0; i < Data.length(); i++)
		{
			(*Out)[i] = Data[i] ^ Key[KeyIndex];
			KeyIndex = (KeyIndex + 1) % Key.length();
		}
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa BinEncoder

void BinEncoder::Write(const void *Data, size_t Size)
{
	ERR_TRY;

	const uint1 *Bytes = (const uint1*)Data;
	uint1 Byte;
	char Octet[8];

	for (uint i = 0; i < Size; i++)
	{
		Byte = *Bytes;
		Octet[0] = (Byte & 0x80) ? '1' : '0';
		Octet[1] = (Byte & 0x40) ? '1' : '0';
		Octet[2] = (Byte & 0x20) ? '1' : '0';
		Octet[3] = (Byte & 0x10) ? '1' : '0';
		Octet[4] = (Byte & 0x08) ? '1' : '0';
		Octet[5] = (Byte & 0x04) ? '1' : '0';
		Octet[6] = (Byte & 0x02) ? '1' : '0';
		Octet[7] = (Byte & 0x01) ? '1' : '0';
		m_CharWriter.WriteString(Octet, 8);
		Bytes++;
	}

	ERR_CATCH_FUNC;
}

void BinEncoder::Encode(char *Out, const void *Data, size_t DataLength)
{
	const uint1 *DataBytes = (const uint1*)Data;

	for (size_t i = 0; i < DataLength; i++)
	{
		*Out = (*DataBytes) & 0x80 ? '1' : '0'; Out++;
		*Out = (*DataBytes) & 0x40 ? '1' : '0'; Out++;
		*Out = (*DataBytes) & 0x20 ? '1' : '0'; Out++;
		*Out = (*DataBytes) & 0x10 ? '1' : '0'; Out++;
		*Out = (*DataBytes) & 0x08 ? '1' : '0'; Out++;
		*Out = (*DataBytes) & 0x04 ? '1' : '0'; Out++;
		*Out = (*DataBytes) & 0x02 ? '1' : '0'; Out++;
		*Out = (*DataBytes) & 0x01 ? '1' : '0'; Out++;
		DataBytes++;
	}
}

void BinEncoder::Encode(string *Out, const void *Data, size_t DataLength)
{
	const uint1 *DataBytes = (const uint1*)Data;
	Out->resize(DataLength*8);
	size_t Out_i = 0;

	for (size_t i = 0; i < DataLength; i++)
	{
		(*Out)[Out_i++] = (*DataBytes) & 0x80 ? '1' : '0';
		(*Out)[Out_i++] = (*DataBytes) & 0x40 ? '1' : '0';
		(*Out)[Out_i++] = (*DataBytes) & 0x20 ? '1' : '0';
		(*Out)[Out_i++] = (*DataBytes) & 0x10 ? '1' : '0';
		(*Out)[Out_i++] = (*DataBytes) & 0x08 ? '1' : '0';
		(*Out)[Out_i++] = (*DataBytes) & 0x04 ? '1' : '0';
		(*Out)[Out_i++] = (*DataBytes) & 0x02 ? '1' : '0';
		(*Out)[Out_i++] = (*DataBytes) & 0x01 ? '1' : '0';
		DataBytes++;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa BinDecoder

BinDecoder::BinDecoder(Stream *a_Stream, DECODE_TOLERANCE Tolerance) :
	OverlayStream(a_Stream),
	m_CharReader(a_Stream),
	m_Tolerance(Tolerance)
{
}

size_t BinDecoder::Read(void *Out, size_t Size)
{
	// Size b�dzie zmniejszane.

	ERR_TRY;

	uint1 *OutBytes = (uint1*)Out;
	uint1 Byte;
	uint Sum = 0;
	char Ch;

	if (m_Tolerance == DECODE_TOLERANCE_NONE)
	{
		while (Size > 0)
		{
			// Pierwszy bit - mo�e go nie by� je�li to koniec strumienia
			if (!m_CharReader.ReadChar(&Ch))
				break;
			if (Ch == '1')
				Byte = 0x80;
			else if (Ch == '0')
				Byte = 0x00;
			else
				throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);

			// Nast�pne 7 bit�w
			Ch = m_CharReader.MustReadChar();
			if (Ch == '1') Byte |= 0x40; else if (Ch != '0') throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			Ch = m_CharReader.MustReadChar();
			if (Ch == '1') Byte |= 0x20; else if (Ch != '0') throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			Ch = m_CharReader.MustReadChar();
			if (Ch == '1') Byte |= 0x10; else if (Ch != '0') throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			Ch = m_CharReader.MustReadChar();
			if (Ch == '1') Byte |= 0x08; else if (Ch != '0') throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			Ch = m_CharReader.MustReadChar();
			if (Ch == '1') Byte |= 0x04; else if (Ch != '0') throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			Ch = m_CharReader.MustReadChar();
			if (Ch == '1') Byte |= 0x02; else if (Ch != '0') throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			Ch = m_CharReader.MustReadChar();
			if (Ch == '1') Byte |= 0x01; else if (Ch != '0') throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
			Size--;
		}
	}
	else if (m_Tolerance == DECODE_TOLERANCE_WHITESPACE)
	{
		while (Size > 0)
		{
			// Pierwszy bit - mo�e go nie by� je�li to koniec strumienia
			for (;;)
			{
				if (!m_CharReader.ReadChar(&Ch))
					goto break_2_WHITESPACE;
				if (Ch == '1')
				{
					Byte = 0x80;
					break;
				}
				else if (Ch == '0')
				{
					Byte = 0x00;
					break;
				}
				else if (CharIsWhitespace(Ch))
				{
					// Nic - nowy obieg tej p�tli wewn�trznej
				}
				else
					throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			}

			// Nast�pne 7 bit�w
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x40; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x20; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x10; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x08; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x04; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x02; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x01; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
			Size--;
		}
break_2_WHITESPACE:
		;
	}
	else // (m_Tolerance == DECODE_TOLERANCE_ALL)
	{
		while (Size > 0)
		{
			// Pierwszy bit - mo�e go nie by� je�li to koniec strumienia
			for (;;)
			{
				if (!m_CharReader.ReadChar(&Ch))
					goto break_2_ALL;
				if (Ch == '1')
				{
					Byte = 0x80;
					break;
				}
				else if (Ch == '0')
				{
					Byte = 0x00;
					break;
				}
			}

			// Nast�pne 7 bit�w
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x40; break; } else if (Ch == '0') break;
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x20; break; } else if (Ch == '0') break;
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x10; break; } else if (Ch == '0') break;
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x08; break; } else if (Ch == '0') break;
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x04; break; } else if (Ch == '0') break;
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x02; break; } else if (Ch == '0') break;
			}
			for (;;) {
				Ch = m_CharReader.MustReadChar();
				if (Ch == '1') { Byte |= 0x01; break; } else if (Ch == '0') break;
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
			Size--;
		}
break_2_ALL:
		;
	}

	return Sum;

	ERR_CATCH_FUNC;
}

bool BinDecoder::DecodeLength(uint *OutLength, const string &s, DECODE_TOLERANCE Tolerance)
{
	// �adne dziwne znaki nie s� tolerowane - po prostu zlicz znaki
	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if ((s.length() & 0x07) != 0) // (s.length % 8) != 0
			return false;
		*OutLength = s.length() >> 3;
		return true;
	}
	// Pewne dodatkowe znaki s� tolerowane - trzeba wszystkie przejrze� i zliczy� same cyfry
	else
	{
		uint DigitCount = 0;
		for (uint i = 0; i < s.length(); i++)
		{
			if (s[i] == '1' || s[i] == '0')
				DigitCount++;
		}

		if ((DigitCount & 0x07) != 0) // (DigitCount % 8) != 0
			return false;
		*OutLength = DigitCount >> 3;
		return true;
	}
}

bool BinDecoder::DecodeLength(uint *OutLength, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance)
{
	// �adne dziwne znaki nie s� tolerowane - po prostu zlicz znaki
	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if ((s_Length & 0x07) != 0) // (s.length % 8) != 0
			return false;
		*OutLength = s_Length >> 3;
		return true;
	}
	// Pewne dodatkowe znaki s� tolerowane - trzeba wszystkie przejrze� i zliczy� same cyfry
	else
	{
		// s b�dzie przesuwany. s_Length b�dzie zmniejszany.

		uint DigitCount = 0;
		while (s_Length > 0)
		{
			if (*s == '1' || *s == '0')
				DigitCount++;
			s++;
			s_Length--;
		}

		if ((DigitCount & 0x07) != 0) // (DigitCount % 8) != 0
			return false;
		*OutLength = DigitCount >> 3;
		return true;
	}
}

size_t BinDecoder::Decode(void *OutData, const string &s, DECODE_TOLERANCE Tolerance)
{
	uint1 *OutBytes = (uint1*)OutData;
	uint1 Byte;
	uint s_i = 0, Sum = 0;
	char Ch;

	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if ((s.length() & 0x03) != 0) return MAXUINT4;

		while (s_i < s.length())
		{
			if (s[s_i] == '1') Byte = 0x80; else if (s[s_i] == '0') Byte = 0x00; else return MAXUINT4; s_i++;

			if (s[s_i] == '1') Byte |= 0x40; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x20; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x10; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x08; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x04; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x02; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x01; else if (s[s_i] != '0') return MAXUINT4; s_i++;

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
	}
	else if (Tolerance == DECODE_TOLERANCE_WHITESPACE)
	{
		for (;;)
		{
			// Pierwszy bit - mo�e go nie by� je�li to koniec strumienia
			for (;;)
			{
				if (s_i == s.length())
					goto break_2_WHITESPACE;
				Ch = s[s_i++];
				if (Ch == '1')
				{
					Byte = 0x80;
					break;
				}
				else if (Ch == '0')
				{
					Byte = 0x00;
					break;
				}
				else if (CharIsWhitespace(Ch))
				{
					// Nic - nowy obieg tej p�tli wewn�trznej
				}
				else
					return MAXUINT4;
			}

			// Nast�pne 7 bit�w
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x40; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x20; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x10; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x08; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x04; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x02; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x01; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
break_2_WHITESPACE:
		;
	}
	else // (m_Tolerance == DECODE_TOLERANCE_ALL)
	{
		for (;;)
		{
			// Pierwszy bit - mo�e go nie by� je�li to koniec strumienia
			for (;;)
			{
				if (s_i == s.length())
					goto break_2_ALL;
				Ch = s[s_i++];
				if (Ch == '1')
				{
					Byte = 0x80;
					break;
				}
				else if (Ch == '0')
				{
					Byte = 0x00;
					break;
				}
			}

			// Nast�pne 7 bit�w
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x40; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x20; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x10; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x08; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x04; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x02; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s.length()) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x01; break; } else if (Ch == '0') break;
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
break_2_ALL:
		;
	}

	return Sum;
}

size_t BinDecoder::Decode(void *OutData, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance)
{
	uint1 *OutBytes = (uint1*)OutData;
	uint1 Byte;
	uint s_i = 0, Sum = 0;
	char Ch;

	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if ((s_Length & 0x03) != 0) return MAXUINT4;

		while (s_i < s_Length)
		{
			if (s[s_i] == '1') Byte = 0x80; else if (s[s_i] == '0') Byte = 0x00; else return MAXUINT4; s_i++;

			if (s[s_i] == '1') Byte |= 0x40; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x20; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x10; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x08; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x04; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x02; else if (s[s_i] != '0') return MAXUINT4; s_i++;
			if (s[s_i] == '1') Byte |= 0x01; else if (s[s_i] != '0') return MAXUINT4; s_i++;

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
	}
	else if (Tolerance == DECODE_TOLERANCE_WHITESPACE)
	{
		for (;;)
		{
			// Pierwszy bit - mo�e go nie by� je�li to koniec strumienia
			for (;;)
			{
				if (s_i == s_Length)
					goto break_2_WHITESPACE;
				Ch = s[s_i++];
				if (Ch == '1')
				{
					Byte = 0x80;
					break;
				}
				else if (Ch == '0')
				{
					Byte = 0x00;
					break;
				}
				else if (CharIsWhitespace(Ch))
				{
					// Nic - nowy obieg tej p�tli wewn�trznej
				}
				else
					return MAXUINT4;
			}

			// Nast�pne 7 bit�w
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x40; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x20; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x10; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x08; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x04; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x02; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x01; break; } else if (Ch == '0') break; else if (!CharIsWhitespace(Ch)) return MAXUINT4;
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
break_2_WHITESPACE:
		;
	}
	else // (m_Tolerance == DECODE_TOLERANCE_ALL)
	{
		for (;;)
		{
			// Pierwszy bit - mo�e go nie by� je�li to koniec strumienia
			for (;;)
			{
				if (s_i == s_Length)
					goto break_2_ALL;
				Ch = s[s_i++];
				if (Ch == '1')
				{
					Byte = 0x80;
					break;
				}
				else if (Ch == '0')
				{
					Byte = 0x00;
					break;
				}
			}

			// Nast�pne 7 bit�w
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x40; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x20; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x10; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x08; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x04; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x02; break; } else if (Ch == '0') break;
			}
			for (;;) {
				if (s_i == s_Length) return MAXUINT4;
				Ch = s[s_i++];
				if (Ch == '1') { Byte |= 0x01; break; } else if (Ch == '0') break;
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
break_2_ALL:
		;
	}

	return Sum;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa HexEncoder

const char * const HEX_DIGITS_U = "0123456789ABCDEF";
const char * const HEX_DIGITS_L = "0123456789abcdef";

void HexEncoder::Write(const void *Data, size_t Size)
{
	ERR_TRY;

	const uint1 *Bytes = (const uint1*)Data;
	uint1 Byte;

	if (m_UpperCase)
	{
		for (uint i = 0; i < Size; i++)
		{
			Byte = *Bytes;
			m_CharWriter.WriteChar(HEX_DIGITS_U[(Byte & 0xF0) >> 4]);
			m_CharWriter.WriteChar(HEX_DIGITS_U[Byte & 0x0F]);
			Bytes++;
		}
	}
	else
	{
		for (uint i = 0; i < Size; i++)
		{
			Byte = *Bytes;
			m_CharWriter.WriteChar(HEX_DIGITS_L[(Byte & 0xF0) >> 4]);
			m_CharWriter.WriteChar(HEX_DIGITS_L[Byte & 0x0F]);
			Bytes++;
		}
	}

	ERR_CATCH_FUNC;
}

void HexEncoder::Encode(char *Out, const void *Data, size_t DataLength, bool UpperCase)
{
	// DataLength b�dzie zmniejszane.
	// Out i InBytes b�d� przesuwane.

	const uint1 *InBytes = (const uint1*)Data;
	uint1 Byte;

	if (UpperCase)
	{
		while (DataLength > 0)
		{
			Byte = *InBytes;
			*Out = HEX_DIGITS_U[(Byte & 0xF0) >> 4]; Out++;
			*Out = HEX_DIGITS_U[ Byte & 0x0F      ]; Out++;
			InBytes++;
			DataLength--;
		}
	}
	else
	{
		while (DataLength > 0)
		{
			Byte = *InBytes;
			*Out = HEX_DIGITS_L[(Byte & 0xF0) >> 4]; Out++;
			*Out = HEX_DIGITS_L[ Byte & 0x0F      ]; Out++;
			InBytes++;
			DataLength--;
		}
	}
}

void HexEncoder::Encode(string *Out, const void *Data, size_t DataLength, bool UpperCase)
{
	// DataLength b�dzie zmniejszane.
	// InBytes b�dzie przesuwane.
	Out->resize(DataLength * 2);

	const uint1 *InBytes = (const uint1*)Data;
	uint1 Byte;
	uint Out_i = 0;

	if (UpperCase)
	{
		while (DataLength > 0)
		{
			Byte = *InBytes;
			(*Out)[Out_i] = HEX_DIGITS_U[(Byte & 0xF0) >> 4]; Out_i++;
			(*Out)[Out_i] = HEX_DIGITS_U[ Byte & 0x0F      ]; Out_i++;
			InBytes++;
			DataLength--;
		}
	}
	else
	{
		while (DataLength > 0)
		{
			Byte = *InBytes;
			(*Out)[Out_i] = HEX_DIGITS_L[(Byte & 0xF0) >> 4]; Out_i++;
			(*Out)[Out_i] = HEX_DIGITS_L[ Byte & 0x0F      ]; Out_i++;
			InBytes++;
			DataLength--;
		}
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa HexDecoder

HexDecoder::HexDecoder(Stream *a_Stream, DECODE_TOLERANCE Tolerance) :
	OverlayStream(a_Stream),
	m_CharReader(a_Stream),
	m_Tolerance(Tolerance)
{
}

size_t HexDecoder::Read(void *Out, size_t Size)
{
	// Size b�dzie zmniejszane.

	ERR_TRY;

	uint1 *OutBytes = (uint1*)Out;
	uint1 Byte, HexNumber;
	uint Sum = 0;
	char Ch;

	if (m_Tolerance == DECODE_TOLERANCE_NONE)
	{
		while (Size > 0)
		{
			// Pierwsza cyfra - mo�e jej nie by� je�li to koniec strumienia
			if (!m_CharReader.ReadChar(&Ch))
				break;
			HexNumber = HexDigitToNumber(Ch);
			if (HexNumber == 0xFF)
				throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			Byte = HexNumber << 4;

			// Druga cyfra
			HexNumber = HexDigitToNumber(m_CharReader.MustReadChar());
			if (HexNumber == 0xFF)
				throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
			Byte |= HexNumber;

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
			Size--;
		}
	}
	else if (m_Tolerance == DECODE_TOLERANCE_WHITESPACE)
	{
		while (Size > 0)
		{
			// Pierwsza cyfra - mo�e jej nie by� je�li to koniec strumienia
			for (;;)
			{
				if (!m_CharReader.ReadChar(&Ch))
					goto break_2_WHITESPACE;
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber == 0xFF)
				{
					if (!CharIsWhitespace(Ch))
						throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
				}
				else
				{
					Byte = HexNumber << 4;
					break;
				}
			}

			// Druga cyfra
			for (;;)
			{
				Ch = m_CharReader.MustReadChar();
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber == 0xFF)
				{
					if (!CharIsWhitespace(Ch))
						throw Error(ERRMSG_DECODE_INVALID_CHAR, __FILE__, __LINE__);
				}
				else
				{
					Byte |= HexNumber;
					break;
				}
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
			Size--;
		}
break_2_WHITESPACE:
		;
	}
	else // (m_Tolerance == DECODE_TOLERANCE_ALL)
	{
		while (Size > 0)
		{
			// Pierwsza cyfra - mo�e jej nie by� je�li to koniec strumienia
			for (;;)
			{
				if (!m_CharReader.ReadChar(&Ch))
					goto break_2_ALL;
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber != 0xFF)
				{
					Byte = HexNumber << 4;
					break;
				}
			}

			// Druga cyfra
			for (;;)
			{
				Ch = m_CharReader.MustReadChar();
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber != 0xFF)
				{
					Byte |= HexNumber;
					break;
				}
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
			Size--;
		}
break_2_ALL:
		;
	}

	return Sum;

	ERR_CATCH_FUNC;
}

bool HexDecoder::DecodeLength(uint *OutLength, const string &s, DECODE_TOLERANCE Tolerance)
{
	// �adne dziwne znaki nie s� tolerowane - po prostu zlicz znaki
	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if ((s.length() & 0x07) != 0) // (s.length % 8) != 0
			return false;
		*OutLength = s.length() >> 3;
		return true;
	}
	// Pewne dodatkowe znaki s� tolerowane - trzeba wszystkie przejrze� i zliczy� same cyfry
	else
	{
		uint DigitCount = 0;
		for (uint i = 0; i < s.length(); i++)
		{
			if (s[i] == '1' || s[i] == '0')
				DigitCount++;
		}

		if ((DigitCount & 0x07) != 0) // (DigitCount % 8) != 0
			return false;
		*OutLength = DigitCount >> 3;
		return true;
	}
}

bool HexDecoder::DecodeLength(uint *OutLength, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance)
{
	// �adne dziwne znaki nie s� tolerowane - po prostu zlicz znaki
	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if ((s_Length & 0x01) != 0) // (s.length % 2) != 0
			return false;
		*OutLength = s_Length >> 1;
		return true;
	}
	// Pewne dodatkowe znaki s� tolerowane - trzeba wszystkie przejrze� i zliczy� same cyfry
	else
	{
		// s b�dzie przesuwany. s_Length b�dzie zmniejszany.

		uint DigitCount = 0;
		while (s_Length > 0)
		{
			if (CharIsHexDigit(*s))
				DigitCount++;
			s++;
			s_Length--;
		}

		if ((DigitCount & 0x01) != 0) // (DigitCount % 2) != 0
			return false;
		*OutLength = DigitCount >> 1;
		return true;
	}
}

size_t HexDecoder::Decode(void *OutData, const string &s, DECODE_TOLERANCE Tolerance)
{
	uint1 *OutBytes = (uint1*)OutData;
	uint1 Byte, HexNumber;
	uint s_i = 0, Sum = 0;
	char Ch;

	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if ((s.length() & 0x01) != 0) return MAXUINT4;

		while (s_i < s.length())
		{
			HexNumber = HexDigitToNumber(s[s_i++]);
			if (HexNumber == 0xFF) return MAXUINT4;
			Byte = HexNumber << 4;

			HexNumber = HexDigitToNumber(s[s_i++]);
			if (HexNumber == 0xFF) return MAXUINT4;
			Byte |= HexNumber;

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
	}
	else if (Tolerance == DECODE_TOLERANCE_WHITESPACE)
	{
		for (;;)
		{
			// Pierwsza cyfra - mo�e jej nie by� je�li to koniec strumienia
			for (;;)
			{
				if (s_i == s.length())
					goto break_2_WHITESPACE;
				Ch = s[s_i++];
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber == 0xFF)
				{
					if (!CharIsWhitespace(Ch))
						return MAXUINT4;
				}
				else
				{
					Byte = HexNumber << 4;
					break;
				}
			}

			// Druga cyfra
			for (;;)
			{
				if (s_i == s.length())
					return MAXUINT4;
				Ch = s[s_i++];
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber == 0xFF)
				{
					if (!CharIsWhitespace(Ch))
						return MAXUINT4;
				}
				else
				{
					Byte |= HexNumber;
					break;
				}
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
break_2_WHITESPACE:
		;
	}
	else // (m_Tolerance == DECODE_TOLERANCE_ALL)
	{
		for (;;)
		{
			// Pierwsza cyfra - mo�e jej nie by� je�li to koniec strumienia
			for (;;)
			{
				if (s_i == s.length())
					goto break_2_ALL;
				Ch = s[s_i++];
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber != 0xFF)
				{
					Byte = HexNumber << 4;
					break;
				}
			}

			// Druga cyfra
			for (;;)
			{
				if (s_i == s.length())
					return MAXUINT4;
				Ch = s[s_i++];
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber != 0xFF)
				{
					Byte |= HexNumber;
					break;
				}
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
break_2_ALL:
		;
	}

	return Sum;
}

size_t HexDecoder::Decode(void *OutData, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance)
{
	uint1 *OutBytes = (uint1*)OutData;
	uint1 Byte, HexNumber;
	uint s_i = 0, Sum = 0;
	char Ch;

	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if ((s_Length & 0x01) != 0) return MAXUINT4;

		while (s_i < s_Length)
		{
			HexNumber = HexDigitToNumber(s[s_i++]);
			if (HexNumber == 0xFF) return MAXUINT4;
			Byte = HexNumber << 4;

			HexNumber = HexDigitToNumber(s[s_i++]);
			if (HexNumber == 0xFF) return MAXUINT4;
			Byte |= HexNumber;

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
	}
	else if (Tolerance == DECODE_TOLERANCE_WHITESPACE)
	{
		for (;;)
		{
			// Pierwsza cyfra - mo�e jej nie by� je�li to koniec strumienia
			for (;;)
			{
				if (s_i == s_Length)
					goto break_2_WHITESPACE;
				Ch = s[s_i++];
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber == 0xFF)
				{
					if (!CharIsWhitespace(Ch))
						return MAXUINT4;
				}
				else
				{
					Byte = HexNumber << 4;
					break;
				}
			}

			// Druga cyfra
			for (;;)
			{
				if (s_i == s_Length)
					return MAXUINT4;
				Ch = s[s_i++];
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber == 0xFF)
				{
					if (!CharIsWhitespace(Ch))
						return MAXUINT4;
				}
				else
				{
					Byte |= HexNumber;
					break;
				}
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
break_2_WHITESPACE:
		;
	}
	else // (m_Tolerance == DECODE_TOLERANCE_ALL)
	{
		for (;;)
		{
			// Pierwsza cyfra - mo�e jej nie by� je�li to koniec strumienia
			for (;;)
			{
				if (s_i == s_Length)
					goto break_2_ALL;
				Ch = s[s_i++];
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber != 0xFF)
				{
					Byte = HexNumber << 4;
					break;
				}
			}

			// Druga cyfra
			for (;;)
			{
				if (s_i == s_Length)
					return MAXUINT4;
				Ch = s[s_i++];
				HexNumber = HexDigitToNumber(Ch);
				if (HexNumber != 0xFF)
				{
					Byte |= HexNumber;
					break;
				}
			}

			*OutBytes = Byte;
			OutBytes++;
			Sum++;
		}
break_2_ALL:
		;
	}

	return Sum;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Base64Encoder

void Base64Encoder::DoFinish()
{
	// Ile pozosta�ych bajt�w zalega jeszcze w buforze
	switch (m_BufIndex)
	{
	case 1:
		m_CharWriter.WriteChar( BASE64_CHARS[ m_Buf[0] >> 2 ] );
		m_CharWriter.WriteChar( BASE64_CHARS[ (m_Buf[0] & 0x3) << 4 ] );
		m_CharWriter.WriteChar( '=' );
		m_CharWriter.WriteChar( '=' );
		break;
	case 2:
		m_CharWriter.WriteChar( BASE64_CHARS[ m_Buf[0] >> 2 ] );
		m_CharWriter.WriteChar( BASE64_CHARS[ ((m_Buf[0] & 0x3) << 4) | (m_Buf[1] >> 4) ] );
		m_CharWriter.WriteChar( BASE64_CHARS[ (m_Buf[1] & 0xF) << 2 ] );
		m_CharWriter.WriteChar( '=' );
		break;
	}

	m_CharWriter.Flush();
}

Base64Encoder::~Base64Encoder()
{
	try
	{
		if (!m_Finished) DoFinish();
	}
	catch (...)
	{
		assert(0 && "Wyj�tek z�apany w Base64Encoder::~Base64Encoder podczas wykonywania DoFinish.");
	}
}

void Base64Encoder::Write(const void *Data, size_t Size)
{
	// B�d� zmniejsza� Size i przesuwa� ByteData

	assert(!m_Finished && "Base64Encoder::Write: Write after Finish.");

	ERR_TRY;

	uint1 *ByteData = (uint1*)Data;

	// P�tla przetwarza kolejne bajty
	while (Size > 0)
	{
		if (m_BufIndex == 2)
		{
			// Bufor pe�ny - mamy trzy bajty - dwa w m_Buf, trzeci w *ByteData. Czas si� ich pozby�.
			m_CharWriter.WriteChar( BASE64_CHARS[ m_Buf[0] >> 2 ] );
			m_CharWriter.WriteChar( BASE64_CHARS[ ((m_Buf[0] & 0x3) << 4) | (m_Buf[1] >> 4) ] );
			m_CharWriter.WriteChar( BASE64_CHARS[ ((m_Buf[1] & 0xF) << 2) | (*ByteData >> 6) ] );
			m_CharWriter.WriteChar( BASE64_CHARS[ (*ByteData & 0x3F) ] );
			m_BufIndex = 0;
		}
		else
			m_Buf[m_BufIndex++] = *ByteData;

		Size--;
		ByteData++;
	}

	ERR_CATCH_FUNC;
}

size_t Base64Encoder::EncodeLength(size_t DataLength)
{
	return ceil_div<size_t>(DataLength, 3) * 4;
}

size_t Base64Encoder::Encode(char *Out, const void *Data, size_t DataLength)
{
	uint1 *ByteData = (uint1*)Data;
	size_t BlockCount = DataLength / 3;
	size_t RemainingBytes = DataLength % 3;
	size_t OutLength = ceil_div<size_t>(DataLength, 3) * 4;

	size_t OutIndex = 0;

	while (BlockCount > 0)
	{
		Out[OutIndex++] = BASE64_CHARS[ ByteData[0] >> 2 ];
		Out[OutIndex++] = BASE64_CHARS[ ((ByteData[0] & 0x3) << 4) | (ByteData[1] >> 4) ];
		Out[OutIndex++] = BASE64_CHARS[ ((ByteData[1] & 0xF) << 2) | (ByteData[2] >> 6) ];
		Out[OutIndex++] = BASE64_CHARS[ (ByteData[2] & 0x3F) ];

		ByteData += 3;
		BlockCount--;
	}

	switch (RemainingBytes)
	{
	case 1:
		Out[OutIndex++] = BASE64_CHARS[ ByteData[0] >> 2 ];
		Out[OutIndex++] = BASE64_CHARS[ (ByteData[0] & 0x3) << 4 ];
		Out[OutIndex++] = '=';
		Out[OutIndex++] = '=';
		break;
	case 2:
		Out[OutIndex++] = BASE64_CHARS[ ByteData[0] >> 2 ];
		Out[OutIndex++] = BASE64_CHARS[ ((ByteData[0] & 0x3) << 4) | (ByteData[1] >> 4) ];
		Out[OutIndex++] = BASE64_CHARS[ (ByteData[1] & 0xF) << 2 ];
		Out[OutIndex++] = '=';
		break;
	}

	return OutLength;
}

size_t Base64Encoder::Encode(string *Out, const void *Data, size_t DataLength)
{
	uint1 *ByteData = (uint1*)Data;
	size_t BlockCount = DataLength / 3;
	size_t RemainingBytes = DataLength % 3;
	size_t OutLength = ceil_div<size_t>(DataLength, 3) * 4;

	Out->clear();
	Out->resize(OutLength);
	size_t OutIndex = 0;

	while (BlockCount > 0)
	{
		(*Out)[OutIndex++] = BASE64_CHARS[ ByteData[0] >> 2 ];
		(*Out)[OutIndex++] = BASE64_CHARS[ ((ByteData[0] & 0x3) << 4) | (ByteData[1] >> 4) ];
		(*Out)[OutIndex++] = BASE64_CHARS[ ((ByteData[1] & 0xF) << 2) | (ByteData[2] >> 6) ];
		(*Out)[OutIndex++] = BASE64_CHARS[ (ByteData[2] & 0x3F) ];

		ByteData += 3;
		BlockCount--;
	}

	switch (RemainingBytes)
	{
	case 1:
		(*Out)[OutIndex++] = BASE64_CHARS[ ByteData[0] >> 2 ];
		(*Out)[OutIndex++] = BASE64_CHARS[ (ByteData[0] & 0x3) << 4 ];
		(*Out)[OutIndex++] = '=';
		(*Out)[OutIndex++] = '=';
		break;
	case 2:
		(*Out)[OutIndex++] = BASE64_CHARS[ ByteData[0] >> 2 ];
		(*Out)[OutIndex++] = BASE64_CHARS[ ((ByteData[0] & 0x3) << 4) | (ByteData[1] >> 4) ];
		(*Out)[OutIndex++] = BASE64_CHARS[ (ByteData[1] & 0xF) << 2 ];
		(*Out)[OutIndex++] = '=';
		break;
	}

	return OutLength;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Base64Decoder

bool Base64Decoder::ReadNextBuf()
{
	uint1 Numbers[4];

	if (m_Tolerance == DECODE_TOLERANCE_NONE)
	{
		char Chs[4];
		if (!m_CharReader.ReadChar(&Chs[0]))
			return false;
		Chs[1] = m_CharReader.MustReadChar();
		Chs[2] = m_CharReader.MustReadChar();
		Chs[3] = m_CharReader.MustReadChar();

		Numbers[0] = Base64CharToNumber(Chs[0]);
		Numbers[1] = Base64CharToNumber(Chs[1]);
		Numbers[2] = Base64CharToNumber(Chs[2]);
		Numbers[3] = Base64CharToNumber(Chs[3]);

		// Ko�czy si� na "=" lub "=="
		if (Numbers[3] == 0xFE)
		{
			// Ko�czy si� na "=="
			if (Numbers[2] == 0xFE)
			{
				if (Numbers[0] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
				if (Numbers[1] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);

				m_Buf[0] = (Numbers[0] << 2) | (Numbers[1] >> 4);
				m_BufLength = 1;
				m_Finished = true;
				return true;
			}
			// Ko�czy si� na "="
			else
			{
				if (Numbers[0] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
				if (Numbers[1] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
				if (Numbers[2] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);

				m_Buf[1] = (Numbers[0] << 2) | (Numbers[1] >> 4);
				m_Buf[0] = (Numbers[1] << 4) | (Numbers[2] >> 2);
				m_BufLength = 2;
				m_Finished = true;
				return true;
			}
		}

		// B��dne znaki lub '=' tam gdzie nie trzeba.
		if (Numbers[0] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
		if (Numbers[1] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
		if (Numbers[2] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
		if (Numbers[3] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);

		m_Buf[2] = (Numbers[0] << 2) | (Numbers[1] >> 4);
		m_Buf[1] = (Numbers[1] << 4) | (Numbers[2] >> 2);
		m_Buf[0] = (Numbers[2] << 6) | Numbers[3];
		m_BufLength = 3;
		return true;
	}
	else
	{
		// Wczytaj 4 znaki
		uint NumberIndex = 0; // do indeksowania Numbers
		char Ch;

		for (;;)
		{
			// Wczytaj znak
			if (!m_CharReader.ReadChar(&Ch))
			{
				// koniec danych wej�ciowych

				// Jeste�my na granicy czw�rki znak�w - OK, koniec
				if (NumberIndex == 0)
					return false;
				// Nie jeste�my na granicy czw�rki znak�w - b��d, niedoko�czone dane
				else
					throw Error(ERRMSG_UNEXPECTED_END);
			}

			// Nieznany znak
			if (Base64CharType(Ch) == 2)
			{
				// Opcjonalny b��d
				if (m_Tolerance == DECODE_TOLERANCE_WHITESPACE && !CharIsWhitespace(Ch))
					throw Error(ERRMSG_DECODE_INVALID_CHAR);
			}
			// Znak znany - cyfra base64 lub '='
			else
			{
				Numbers[NumberIndex++] = Base64CharToNumber(Ch);

				// To czwarty z czw�rki znak�w
				if (NumberIndex == 4)
				{
					// Przetw�rz t� czw�rk�

					// Ko�czy si� na "=" lub "=="
					if (Numbers[3] == 0xFE)
					{
						// Ko�czy si� na "=="
						if (Numbers[2] == 0xFE)
						{
							if (Numbers[0] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
							if (Numbers[1] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);

							m_Buf[0] = (Numbers[0] << 2) | (Numbers[1] >> 4); //OutBytes++;
							m_BufLength = 1;
							m_Finished = true;
							return true;
						}
						// Ko�czy si� na "="
						else
						{
							if (Numbers[0] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
							if (Numbers[1] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
							if (Numbers[2] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);

							m_Buf[1] = (Numbers[0] << 2) | (Numbers[1] >> 4);
							m_Buf[0] = (Numbers[1] << 4) | (Numbers[2] >> 2);
							m_BufLength = 2;
							m_Finished = true;
							return true;
						}
					}

					// Nie ko�czy si� - normalne znaki

					// B��dne znaki lub '=' tam gdzie nie trzeba.
					if (Numbers[0] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
					if (Numbers[1] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
					if (Numbers[2] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);
					if (Numbers[3] >= 0xFE) throw Error(ERRMSG_DECODE_INVALID_CHAR);

					m_Buf[2] = (Numbers[0] << 2) | (Numbers[1] >> 4);
					m_Buf[1] = (Numbers[1] << 4) | (Numbers[2] >> 2);
					m_Buf[0] = (Numbers[2] << 6) | Numbers[3];

					m_BufLength = 3;
					return true;
				}
			}
		}
	}
}

Base64Decoder::Base64Decoder(Stream *a_Stream, DECODE_TOLERANCE Tolerance) :
	OverlayStream(a_Stream),
	m_CharReader(a_Stream),
	m_Tolerance(Tolerance),
	m_BufLength(0),
	m_Finished(false)
{
}

size_t Base64Decoder::Read(void *Out, size_t Size)
{
	ERR_TRY;

	uint1 *OutBytes = (uint1*)Out;
	// Size b�dzie zmniejszany. OutBytes b�dzie przesuwany.

	size_t Sum = 0;
	while (Size > 0)
	{
		if (!GetNextByte(OutBytes))
			break;
		OutBytes++;
		Size--;
		Sum++;
	}

	return Sum;

	ERR_CATCH_FUNC;
}

bool Base64Decoder::End()
{
	return (m_BufLength == 0) && (m_Finished || m_CharReader.End());
}

bool Base64Decoder::DecodeLength(uint *OutLength, const string &s, DECODE_TOLERANCE Tolerance)
{
	// S� tylko dok�adnie te znaki co trzeba
	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if (s.empty()) { *OutLength = 0; return true; }
		if ((s.length() & 3) != 0) return false; // length % 4 != 0
		*OutLength = s.length() * 3 / 4;
		if (s[s.length()-2] == '=')
			(*OutLength) -= 2;
		else if (s[s.length()-1] == '=')
			(*OutLength) -= 1;
		return true;
	}
	// Mog� by� te� inne znaki - trzeba wszystkie przejrze� i policzy�
	else
	{
		uint EqualCount = 0;
		uint DigitCount = 0;
		*OutLength = 0;
		for (uint i = 0; i < s.length(); i++)
		{
			switch (Base64CharType(s[i]))
			{
			case 0:
				DigitCount++;
				break;
			case 1:
				EqualCount++;
				break;
			}
		}

		if (((DigitCount+EqualCount) & 3) != 0) return false; // (DigitCount+EqualCount) % 4 != 0
		*OutLength = (DigitCount+EqualCount) * 3 / 4;
		switch (EqualCount)
		{
		case 0:
			break;
		case 2:
			(*OutLength) -= 2;
			break;
		case 1:
			(*OutLength) -= 1;
			break;
		default:
			return false;
		}
		return true;
	}
}

bool Base64Decoder::DecodeLength(uint *OutLength, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance)
{
	// S� tylko dok�adnie te znaki co trzeba
	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if (s_Length == 0) { *OutLength = 0; return true; }
		if ((s_Length & 3) != 0) return false; // length % 4 != 0
		*OutLength = s_Length * 3 / 4;
		if (s[s_Length-2] == '=')
			(*OutLength) -= 2;
		else if (s[s_Length-1] == '=')
			(*OutLength) -= 1;
		return true;
	}
	// Mog� by� te� inne znaki - trzeba wszystkie przejrze� i policzy�
	else
	{
		uint EqualCount = 0;
		uint DigitCount = 0;
		*OutLength = 0;
		for (uint i = 0; i < s_Length; i++)
		{
			switch (Base64CharType(s[i]))
			{
			case 0:
				DigitCount++;
				break;
			case 1:
				EqualCount++;
				break;
			}
		}

		if (((DigitCount+EqualCount) & 3) != 0) return false; // (DigitCount+EqualCount) % 4 != 0
		*OutLength = (DigitCount+EqualCount) * 3 / 4;
		switch (EqualCount)
		{
		case 0:
			break;
		case 2:
			(*OutLength) -= 2;
			break;
		case 1:
			(*OutLength) -= 1;
			break;
		default:
			return false;
		}
		return true;
	}
}

size_t Base64Decoder::Decode(void *OutData, const string &s, DECODE_TOLERANCE Tolerance)
{
	uint1 *OutBytes = (uint1*)OutData;
	uint s_i = 0, Sum = 0;
	uint1 Numbers[4];

	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if ((s.length() & 3) != 0) return MAXUINT4;

		while (s_i < s.length())
		{
			Numbers[0] = Base64CharToNumber(s[s_i++]);
			Numbers[1] = Base64CharToNumber(s[s_i++]);
			Numbers[2] = Base64CharToNumber(s[s_i++]);
			Numbers[3] = Base64CharToNumber(s[s_i++]);

			if (s_i == s.length())
			{
				// Ko�czy si� na "=" lub "=="
				if (Numbers[3] == 0xFE)
				{
					// Ko�czy si� na "=="
					if (Numbers[2] == 0xFE)
					{
						if (Numbers[0] >= 0xFE) return MAXUINT4;
						if (Numbers[1] >= 0xFE) return MAXUINT4;

						*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); //OutBytes++;
						Sum += 1;
						break;
					}
					// Ko�czy si� na "="
					else
					{
						if (Numbers[0] >= 0xFE) return MAXUINT4;
						if (Numbers[1] >= 0xFE) return MAXUINT4;
						if (Numbers[2] >= 0xFE) return MAXUINT4;

						*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); OutBytes++;
						*OutBytes = (Numbers[1] << 4) | (Numbers[2] >> 2); //OutBytes++;
						Sum += 2;
						break;
					}
				}
			}

			// B��dne znaki lub '=' tam gdzie nie trzeba.
			if (Numbers[0] >= 0xFE) return MAXUINT4;
			if (Numbers[1] >= 0xFE) return MAXUINT4;
			if (Numbers[2] >= 0xFE) return MAXUINT4;
			if (Numbers[3] >= 0xFE) return MAXUINT4;

			*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); OutBytes++;
			*OutBytes = (Numbers[1] << 4) | (Numbers[2] >> 2); OutBytes++;
			*OutBytes = (Numbers[2] << 6) | Numbers[3]; OutBytes++;

			Sum += 3;
		}
	}
	else
	{
		// Wczytuj po 4 znaki
		uint NumberIndex = 0; // do indeksowania Numbers
		char Ch;

		for (;;)
		{
			// koniec danych wej�ciowych
			if (s_i == s.length())
			{
				// Jeste�my na granicy czw�rki znak�w - OK, koniec
				if (NumberIndex == 0)
					break;
				// Nie jeste�my na granicy czw�rki znak�w - b��d, niedoko�czone dane
				else
					return MAXUINT4;
			}

			// Wczytaj znak
			Ch = s[s_i++];
			// Nieznany znak
			if (Base64CharType(Ch) == 2)
			{
				// Opcjonalny b��d
				if (Tolerance == DECODE_TOLERANCE_WHITESPACE && !CharIsWhitespace(Ch))
					return MAXUINT4;
			}
			// Znak znany - cyfra base64 lub '='
			else
			{
				Numbers[NumberIndex++] = Base64CharToNumber(Ch);

				// To czwarty z czw�rki znak�w
				if (NumberIndex == 4)
				{
					// Przetw�rz t� czw�rk�

					// Ko�czy si� na "=" lub "=="
					if (Numbers[3] == 0xFE)
					{
						// Ko�czy si� na "=="
						if (Numbers[2] == 0xFE)
						{
							if (Numbers[0] >= 0xFE) return MAXUINT4;
							if (Numbers[1] >= 0xFE) return MAXUINT4;

							*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); //OutBytes++;
							Sum += 1;
							break;
						}
						// Ko�czy si� na "="
						else
						{
							if (Numbers[0] >= 0xFE) return MAXUINT4;
							if (Numbers[1] >= 0xFE) return MAXUINT4;
							if (Numbers[2] >= 0xFE) return MAXUINT4;

							*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); OutBytes++;
							*OutBytes = (Numbers[1] << 4) | (Numbers[2] >> 2); //OutBytes++;
							Sum += 2;
							break;
						}
					}

					// Nie ko�czy si� - normalne znaki

					// B��dne znaki lub '=' tam gdzie nie trzeba.
					if (Numbers[0] >= 0xFE) return MAXUINT4;
					if (Numbers[1] >= 0xFE) return MAXUINT4;
					if (Numbers[2] >= 0xFE) return MAXUINT4;
					if (Numbers[3] >= 0xFE) return MAXUINT4;

					*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); OutBytes++;
					*OutBytes = (Numbers[1] << 4) | (Numbers[2] >> 2); OutBytes++;
					*OutBytes = (Numbers[2] << 6) | Numbers[3]; OutBytes++;

					Sum += 3;

					NumberIndex = 0;
				}
			}
		}
	}

	return Sum;
}

size_t Base64Decoder::Decode(void *OutData, const char *s, uint s_Length, DECODE_TOLERANCE Tolerance)
{
	uint1 *OutBytes = (uint1*)OutData;
	uint s_i = 0, Sum = 0;
	uint1 Numbers[4];

	if (Tolerance == DECODE_TOLERANCE_NONE)
	{
		if ((s_Length & 3) != 0) return MAXUINT4;

		while (s_i < s_Length)
		{
			Numbers[0] = Base64CharToNumber(s[s_i++]);
			Numbers[1] = Base64CharToNumber(s[s_i++]);
			Numbers[2] = Base64CharToNumber(s[s_i++]);
			Numbers[3] = Base64CharToNumber(s[s_i++]);

			if (s_i == s_Length)
			{
				// Ko�czy si� na "=" lub "=="
				if (Numbers[3] == 0xFE)
				{
					// Ko�czy si� na "=="
					if (Numbers[2] == 0xFE)
					{
						if (Numbers[0] >= 0xFE) return MAXUINT4;
						if (Numbers[1] >= 0xFE) return MAXUINT4;

						*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); //OutBytes++;
						Sum += 1;
						break;
					}
					// Ko�czy si� na "="
					else
					{
						if (Numbers[0] >= 0xFE) return MAXUINT4;
						if (Numbers[1] >= 0xFE) return MAXUINT4;
						if (Numbers[2] >= 0xFE) return MAXUINT4;

						*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); OutBytes++;
						*OutBytes = (Numbers[1] << 4) | (Numbers[2] >> 2); //OutBytes++;
						Sum += 2;
						break;
					}
				}
			}

			// B��dne znaki lub '=' tam gdzie nie trzeba.
			if (Numbers[0] >= 0xFE) return MAXUINT4;
			if (Numbers[1] >= 0xFE) return MAXUINT4;
			if (Numbers[2] >= 0xFE) return MAXUINT4;
			if (Numbers[3] >= 0xFE) return MAXUINT4;

			*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); OutBytes++;
			*OutBytes = (Numbers[1] << 4) | (Numbers[2] >> 2); OutBytes++;
			*OutBytes = (Numbers[2] << 6) | Numbers[3]; OutBytes++;

			Sum += 3;
		}
	}
	else
	{
		// Wczytuj po 4 znaki
		uint NumberIndex = 0; // do indeksowania Numbers
		char Ch;

		for (;;)
		{
			// koniec danych wej�ciowych
			if (s_i == s_Length)
			{
				// Jeste�my na granicy czw�rki znak�w - OK, koniec
				if (NumberIndex == 0)
					break;
				// Nie jeste�my na granicy czw�rki znak�w - b��d, niedoko�czone dane
				else
					return MAXUINT4;
			}

			// Wczytaj znak
			Ch = s[s_i++];
			// Nieznany znak
			if (Base64CharType(Ch) == 2)
			{
				// Opcjonalny b��d
				if (Tolerance == DECODE_TOLERANCE_WHITESPACE && !CharIsWhitespace(Ch))
					return MAXUINT4;
			}
			// Znak znany - cyfra base64 lub '='
			else
			{
				Numbers[NumberIndex++] = Base64CharToNumber(Ch);

				// To czwarty z czw�rki znak�w
				if (NumberIndex == 4)
				{
					// Przetw�rz t� czw�rk�

					// Ko�czy si� na "=" lub "=="
					if (Numbers[3] == 0xFE)
					{
						// Ko�czy si� na "=="
						if (Numbers[2] == 0xFE)
						{
							if (Numbers[0] >= 0xFE) return MAXUINT4;
							if (Numbers[1] >= 0xFE) return MAXUINT4;

							*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); //OutBytes++;
							Sum += 1;
							break;
						}
						// Ko�czy si� na "="
						else
						{
							if (Numbers[0] >= 0xFE) return MAXUINT4;
							if (Numbers[1] >= 0xFE) return MAXUINT4;
							if (Numbers[2] >= 0xFE) return MAXUINT4;

							*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); OutBytes++;
							*OutBytes = (Numbers[1] << 4) | (Numbers[2] >> 2); //OutBytes++;
							Sum += 2;
							break;
						}
					}

					// Nie ko�czy si� - normalne znaki

					// B��dne znaki lub '=' tam gdzie nie trzeba.
					if (Numbers[0] >= 0xFE) return MAXUINT4;
					if (Numbers[1] >= 0xFE) return MAXUINT4;
					if (Numbers[2] >= 0xFE) return MAXUINT4;
					if (Numbers[3] >= 0xFE) return MAXUINT4;

					*OutBytes = (Numbers[0] << 2) | (Numbers[1] >> 4); OutBytes++;
					*OutBytes = (Numbers[1] << 4) | (Numbers[2] >> 2); OutBytes++;
					*OutBytes = (Numbers[2] << 6) | Numbers[3]; OutBytes++;

					Sum += 3;

					NumberIndex = 0;
				}
			}
		}
	}

	return Sum;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa RingBuffer

RingBuffer::RingBuffer(uint Capacity) :
	m_Capacity(Capacity),
	m_Size(0),
	m_BegIndex(0),
	m_EndIndex(0)
{
	m_Buf.resize(m_Capacity);
}

RingBuffer::~RingBuffer()
{
}

void RingBuffer::Write(const void *Data, size_t Size)
{
	// Nie zmie�ci si� w buforze
	if (m_Size + Size > m_Capacity)
		throw Error(Format("B��d zapisu do RingBuffer: Nie mo�na zapisa� # B - dane si� nie zmieszcz�.") % Size, __FILE__, __LINE__);

	// Zmie�ci si� ca�ej na ko�cu
	if (Size <= m_Capacity - m_EndIndex)
	{
		CopyMem(&m_Buf[m_EndIndex], Data, Size);
		m_EndIndex += Size;
	}
	// Nie zmie�ci si� na ko�cu
	else {
		uint PartSize = m_Capacity - m_EndIndex;

		// Przepisz na koniec ile si� zmie�ci
		CopyMem(&m_Buf[m_EndIndex], Data, PartSize);
		// Przepisz reszt� na pocz�tek
		CopyMem(&m_Buf[0], ((const char*)Data + PartSize), Size - PartSize);

		m_EndIndex = Size - PartSize;
	}
	m_Size += Size;
}

size_t RingBuffer::Read(void *Out, size_t MaxLength)
{
	if (MaxLength == 0) return 0;

	// Ustal ile bajt�w odczyta�
	uint Length = std::min(MaxLength, m_Size);
	// Wszystko b�dzie w jednym kawa�ku
	if (m_Capacity - m_BegIndex >= Length)
	{
		CopyMem(Out, &m_Buf[m_BegIndex], Length);
		m_BegIndex += Length;
	}
	// B�dzie w dw�ch kawa�kach
	else
	{
		uint PartSize = m_Capacity - m_BegIndex;

		// Kawa�ek z ko�ca
		CopyMem(Out, &m_Buf[m_BegIndex], PartSize);
		// Kawa�ek z pocz�tku
		CopyMem(((char*)Out + PartSize), &m_Buf[0], Length - PartSize);

		m_BegIndex = Length - PartSize;
	}
	m_Size -= Length;
	return Length;
}

void RingBuffer::MustRead(void *Out, size_t Length)
{
	if (Length == 0) return;

	if (m_Size < Length)
		throw Error(Format("Nie mo�na odczyta� # B z RingBuffer - nie ma tylu w buforze, jest tylko # B.") % Length % m_Size, __FILE__, __LINE__);

	// Wszystko b�dzie w jednym kawa�ku
	if (m_Capacity - m_BegIndex >= Length)
	{
		CopyMem(Out, &m_Buf[m_BegIndex], Length);
		m_BegIndex += Length;
	}
	// B�dzie w dw�ch kawa�kach
	else
	{
		uint PartSize = m_Capacity - m_BegIndex;

		// Kawa�ek z ko�ca
		CopyMem(Out, &m_Buf[m_BegIndex], PartSize);
		// Kawa�ek z pocz�tku
		CopyMem(((char*)Out + PartSize), &m_Buf[0], Length - PartSize);

		m_BegIndex = Length - PartSize;
	}
	m_Size -= Length;
}

bool RingBuffer::End()
{
	return IsEmpty();
}

size_t RingBuffer::Skip(size_t MaxLength)
{
	if (MaxLength == 0) return 0;

	// Ustal ile bajt�w pomin��
	uint Length = std::min(MaxLength, m_Size);
	// Wszystko jest w jednym kawa�ku
	if (m_Capacity - m_BegIndex >= Length)
		m_BegIndex += Length;
	// Jest w dw�ch kawa�kach
	else
	{
		uint PartSize = m_Capacity - m_BegIndex;
		m_BegIndex = Length - PartSize;
	}
	m_Size -= Length;
	return Length;
}


} // namespace common
