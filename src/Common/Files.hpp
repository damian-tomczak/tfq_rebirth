/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Files - Obs³uga plików i systemu plików
 * Dokumentacja: Patrz plik doc/Files.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_FILES_H_
#define COMMON_FILES_H_

#include "Stream.hpp"


namespace common
{

struct DATETIME;

// Rodzaj elementu systemu plików
enum FILE_ITEM_TYPE
{
	IT_NONE, // B³¹d, brak, koniec itp.
	IT_DIR,  // Katalog
	IT_FILE, // Plik
};

// Tryb otwarcia pliku
enum FILE_MODE
{
	// zapis: tak, odczyt: nie
	// pozycja: 0
	// nie istnieje: tworzy, istnieje: opró¿nia
	// SetPos: dzia³a
	FM_WRITE,
	// zapis: tak, odczyt: tak
	// pozycja: 0
	// nie istnieje: tworzy, istnieje: opró¿nia
	// SetPos: dzia³a
	FM_WRITE_PLUS,
	// zapis: nie, odczyt: tak
	// pozycja: 0
	// nie istnieje: b³¹d, istnieje: otwiera
	// SetPos: dzia³a
	FM_READ,
	// zapis: tak, odczyt: tak
	// pozycja: 0
	// nie istnieje: b³¹d, istnieje: otwiera
	// SetPos: dzia³a
	FM_READ_PLUS,
	// zapis: tak, odczyt: nie
	// pozycja: koniec
	// nie istnieje: tworzy, istnieje: otwiera
	// SetPos: w Linuksie nie dzia³a!
	FM_APPEND,
	// zapis: tak, odczyt: tak
	// pozycja: koniec
	// nie istnieje: tworzy, istnieje: otwiera
	// SetPos: w Linuksie nie dzia³a!
	FM_APPEND_PLUS,
};

class File_pimpl;

// Strumieñ plikowy
class FileStream : public SeekableStream
{
private:
	scoped_ptr<File_pimpl> pimpl;

public:
	FileStream(const string &FileName, FILE_MODE FileMode, bool Lock = true);
	virtual ~FileStream();

	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Data, size_t Size);
	virtual void Flush();
	virtual size_t GetSize();
	virtual int GetPos();
	virtual void SetPos(int pos);
	virtual void SetPosFromCurrent(int pos);
	virtual void SetPosFromEnd(int pos);
	virtual void SetSize(size_t Size);
	virtual void Truncate();
	virtual bool End();
};

class DirLister_pimpl;

// Klasa do listowania zawartoœci katalogu.
// W przypadku b³êdów rzuca wyj¹tki.
// Nie listuje "." ani "..".
class DirLister
{
private:
	scoped_ptr<DirLister_pimpl> pimpl;

public:
	DirLister(const string &Dir);
	~DirLister();

	// Pobiera pierwszy/nastêpny element (plik lub katalog) i zwraca go przez parametry.
	// Jeœli nie uda³o siê, bo ju¿ koniec, zawraca false.
	// Name to sama nazwa, nie pe³na œcie¿ka.
	// Type to DIR lub FILE.
	bool ReadNext(string *OutName, FILE_ITEM_TYPE *OutType);
};


// Zapisuje podany ³añcuch jako treœæ pliku
void SaveStringToFile(const string &FileName, const string &Data);
// Zapisuje podany ³añcuch jako treœæ pliku
void SaveDataToFile(const string &FileName, const void *Data, size_t NumBytes);
// Wczytuje ca³¹ zawartoœæ pliku do ³añcucha
void LoadStringFromFile(const string &FileName, string *Data);

// Zwraca wybrane informacje na temat pliku/katalogu
// Jako parametry wyjœciowe mo¿na podawaæ NULL.
// Jeœli to katalog, zwrócony rozmiar jest niezdefiniowany.
bool GetFileItemInfo(const string &Path, FILE_ITEM_TYPE *OutType, uint *OutSize, DATETIME *OutModificationTime, DATETIME *OutCreationTime = NULL, DATETIME *OutAccessTime = NULL);
void MustGetFileItemInfo(const string &Path, FILE_ITEM_TYPE *OutType, uint *OutSize, DATETIME *OutModificationTime, DATETIME *OutCreationTime = NULL, DATETIME *OutAccessTime = NULL);
// Zwraca typ - czy to jest plik, katalog czy b³¹d/nie-istnieje.
// Jeœli b³¹d lub nie istnieje, zwraca IT_NONE.
FILE_ITEM_TYPE GetFileItemType(const string &Path);

// Ustawia czas dostêpu i modyfikacji podanego pliku na bie¿¹cy
bool UpdateFileTimeToNow(const string &FileName);
void MustUpdateFileTimeToNow(const string &FileName);
// Ustawia czas dostêpu i modyfikacji podanego pliku na podane
bool UpdateFileTime(const string &FileName, const DATETIME &ModificationTime, const DATETIME &AccessTime);
void MustUpdateFileTime(const string &FileName, const DATETIME &ModificationTime, const DATETIME &AccessTime);

// Tworzy katalog
bool CreateDirectory(const string &Path);
void MustCreateDirectory(const string &Path);
// Usuwa katalog
// Katalog musi byæ pusty.
bool DeleteDirectory(const string &Path);
void MustDeleteDirectory(const string &Path);
// Tworzy ca³¹ œcie¿kê katalogów a¿ po podanego, np. "C:\A\B\C" nawet jeœli nie istnia³ nawet "C:\A"
bool CreateDirectoryChain(const string &Path);
void MustCreateDirectoryChain(const string &Path);

// Usuwa plik
bool DeleteFile(const string &FileName);
void MustDeleteFile(const string &FileName);
// Przenosi lub zmienia nazwê pliku lub katalogu
// Katalogi mo¿na przenosiæ tylko w ramach jednej partycji. Pliki niekoniecznie.
bool MoveItem(const string &OldPath, const string &NewPath);
void MustMoveItem(const string &OldPath, const string &NewPath);

} // namespace common

#endif
