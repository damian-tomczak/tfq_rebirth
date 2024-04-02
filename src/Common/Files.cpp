/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Files - Obs³uga plików i systemu plików
 * Dokumentacja: Patrz plik doc/Files.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h> // dla unlink [Win, Linux ma go w unistd.h], rename [obydwa]
#ifdef WIN32
	extern "C" {
		#include <sys/utime.h> // dla utime
		#include <direct.h> // dla mkdir (Linux ma go w sys/stat.h + sys/types.h)
	}
	#include <windows.h>
#else
	extern "C" {
		#include <sys/time.h> // dla utime
		#include <sys/file.h> // dla flock
		#include <dirent.h>
		#include <utime.h> // dla utime
	}
#endif
#include <stack>

#include "Error.hpp"
#include "Files.hpp"
#include "DateTime.hpp"


namespace common
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa FileStream

#ifdef WIN32

	class File_pimpl
	{
	public:
		HANDLE m_File;
	};

	FileStream::FileStream(const string &FileName, FILE_MODE FileMode, bool Lock) :
		pimpl(new File_pimpl)
	{
		uint4 DesiredAccess, ShareMode, CreationDisposition;
		switch (FileMode)
		{
		case FM_WRITE:
			DesiredAccess = GENERIC_WRITE;
			ShareMode = 0;
			CreationDisposition = CREATE_ALWAYS;
			break;
		case FM_WRITE_PLUS:
			DesiredAccess = GENERIC_WRITE | GENERIC_READ;
			ShareMode = 0;
			CreationDisposition = CREATE_ALWAYS;
			break;
		case FM_READ:
			DesiredAccess = GENERIC_READ;
			ShareMode = FILE_SHARE_READ;
			CreationDisposition = OPEN_EXISTING;
			break;
		case FM_READ_PLUS:
			DesiredAccess = GENERIC_WRITE | GENERIC_READ;
			ShareMode = 0;
			CreationDisposition = OPEN_EXISTING;
			break;
		case FM_APPEND:
			DesiredAccess = GENERIC_WRITE;
			ShareMode = 0;
			CreationDisposition = OPEN_ALWAYS;
			break;
		case FM_APPEND_PLUS:
			DesiredAccess = GENERIC_WRITE | GENERIC_READ;
			ShareMode = 0;
			CreationDisposition = OPEN_ALWAYS;
			break;
		}
		if (!Lock)
			ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

		pimpl->m_File = CreateFileA(FileName.c_str(), DesiredAccess, ShareMode, 0, CreationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
		if (pimpl->m_File == INVALID_HANDLE_VALUE)
			throw Win32Error("Nie mo¿na otworzyæ pliku: "+FileName, __FILE__, __LINE__);

		if (FileMode == FM_APPEND || FileMode == FM_APPEND_PLUS)
			SetPosFromEnd(0);
	}

	FileStream::~FileStream()
	{
		CloseHandle(pimpl->m_File);
	}

	void FileStream::Write(const void *Data, size_t Size)
	{
		uint4 WrittenSize;
		if (WriteFile(pimpl->m_File, Data, Size, (LPDWORD)&WrittenSize, 0) == 0)
			throw Win32Error(Format("Nie mo¿na zapisaæ # B do pliku") % Size, __FILE__, __LINE__);
		if (WrittenSize != Size)
			throw Error(Format("Nie mo¿na zapisaæ do pliku - zapisanych bajtów #/#") % WrittenSize % Size, __FILE__, __LINE__);
	}

	size_t FileStream::Read(void *Data, size_t Size)
	{
		uint4 ReadSize;
		if (ReadFile(pimpl->m_File, Data, Size, (LPDWORD)&ReadSize, 0) == 0)
			throw Win32Error(Format("Nie mo¿na odczytaæ # B z pliku") % Size, __FILE__, __LINE__);
		return ReadSize;
	}

	void FileStream::Flush()
	{
		FlushFileBuffers(pimpl->m_File);
	}

	size_t FileStream::GetSize()
	{
		return (size_t)GetFileSize(pimpl->m_File, 0);
	}

	int FileStream::GetPos()
	{
		// Fuj! Ale to nieeleganckie. Szkoda, ¿e nie ma lepszego sposobu.
		uint4 r = SetFilePointer(pimpl->m_File, 0, 0, FILE_CURRENT);
		if (r == INVALID_SET_FILE_POINTER)
			throw Win32Error("Nie mo¿na odczytaæ pozycji w strumieniu plikowym", __FILE__, __LINE__);
		return (int)r;
	}

	void FileStream::SetPos(int pos)
	{
		if (SetFilePointer(pimpl->m_File, pos, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			throw Win32Error(Format("Nie mo¿na przesun¹æ pozycji w strumieniu plikowym do # od pocz¹tku") % pos, __FILE__, __LINE__);
	}

	void FileStream::SetPosFromCurrent(int pos)
	{
		if (SetFilePointer(pimpl->m_File, pos, 0, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
			throw Win32Error(Format("Nie mo¿na przesun¹æ pozycji w strumieniu plikowym do # od bie¿¹cej") % pos, __FILE__, __LINE__);
	}

	void FileStream::SetPosFromEnd(int pos)
	{
		if (SetFilePointer(pimpl->m_File, pos, 0, FILE_END) == INVALID_SET_FILE_POINTER)
			throw Win32Error(Format("Nie mo¿na przesun¹æ pozycji w strumieniu plikowym do # od koñca") % pos, __FILE__, __LINE__);
	}

	void FileStream::SetSize(size_t Size)
	{
		SetPos((int)Size);
		Truncate();
	}

	void FileStream::Truncate()
	{
		if (SetEndOfFile(pimpl->m_File) == 0)
			throw Win32Error("Nie mo¿na obci¹æ pliku", __FILE__, __LINE__);
	}

	bool FileStream::End()
	{
		// Fuj! Ale to nieeleganckie. Szkoda, ¿e nie ma lepszego sposobu.
		return ((int)GetSize() == GetPos());
	}

#else

	class File_pimpl
	{
	public:
		FILE *m_File;
		bool m_Lock;
	};

	FileStream::FileStream(const string &FileName, FILE_MODE FileMode, bool Lock) :
		pimpl(new File_pimpl)
	{
		pimpl->m_Lock = Lock;

		const char *m = 0;

		switch (FileMode)
		{
		case FM_WRITE:
			m = "wb";
			break;
		case FM_WRITE_PLUS:
			m = "w+b";
			break;
		case FM_READ:
			m = "rb";
			break;
		case FM_READ_PLUS:
			m = "r+b";
			break;
		case FM_APPEND:
			m = "ab";
			break;
		case FM_APPEND_PLUS:
			m = "a+b";
			break;
		}

		pimpl->m_File = fopen(FileName.c_str(), m);
		if (pimpl->m_File == 0)
			throw ErrnoError(Format("Nie mo¿na otworzyæ pliku \"#\" w trybie \"#\"") % FileName % m, __FILE__, __LINE__);
		if (Lock)
		{
			if (flock(fileno(pimpl->m_File), LOCK_EX | LOCK_NB) != 0)
				throw ErrnoError(Format("Nie mo¿na otworzyæ pliku \"#\" w trybie \"#\" - b³ad podczas blokowania") % FileName % m, __FILE__, __LINE__);
		}
	}

	FileStream::~FileStream()
	{
		if (pimpl->m_File != 0)
		{
			// Kiedy deskryptor int zostaje zamkniêty, podobno blokada sama siê zwalania. Ale kto tam tego Linuksa wie... :)
			if (pimpl->m_Lock)
				flock(fileno(pimpl->m_File), LOCK_UN);
			fclose(pimpl->m_File);
		}
	}

	void FileStream::Write(const void *Data, size_t Size)
	{
		size_t Written = fwrite(Data, 1, Size, pimpl->m_File);
		if (Written != Size || ferror(pimpl->m_File) != 0)
			throw Error(Format("Nie mo¿na zapisaæ do pliku - zapisano #/# B") % Written % Size, __FILE__, __LINE__);
	}

	size_t FileStream::Read(void *Data, size_t Size)
	{
		size_t BytesRead = fread(Data, 1, Size, pimpl->m_File);
		if (ferror(pimpl->m_File) != 0)
			throw Error(Format("Nie mo¿na odczytaæ z pliku - odczytano #/# B") % BytesRead % Size, __FILE__, __LINE__);
		return BytesRead;
	}

	void FileStream::Flush()
	{
		fflush(pimpl->m_File);
	}

	size_t FileStream::GetSize()
	{
		// Wersja 1
		int SavedPos = GetPos();
		SetPosFromEnd(0);
		int EndPos = GetPos();
		SetPos(SavedPos);
		return (size_t)EndPos;

		// Ta wersja jest z³a bo mo¿e nie pokazywaæ aktualnego rozmiaru, bo jest bufor
/*		// Wersja 2
		struct stat s;
		fstat(fileno(pimpl->m_File), &s);
		return s.st_size;*/
	}

	int FileStream::GetPos()
	{
		int r = ftell(pimpl->m_File);
		if (r < 0)
			throw ErrnoError("Nie mo¿na odczytaæ pozycji z pliku", __FILE__, __LINE__);
		return r;
	}

	void FileStream::SetPos(int pos)
	{
		int r = fseek(pimpl->m_File, pos, SEEK_SET);
		if (r < 0)
			throw ErrnoError(Format("Nie mo¿na ustawiæ pozycji w pliku na: # B od pocz¹tku") % pos, __FILE__, __LINE__);
	}

	void FileStream::SetPosFromCurrent(int pos)
	{
		int r = fseek(pimpl->m_File, pos, SEEK_CUR);
		if (r < 0)
			throw ErrnoError(Format("Nie mo¿na ustawiæ pozycji w pliku na: # B od bie¿¹cej") % pos, __FILE__, __LINE__);
	}

	void FileStream::SetPosFromEnd(int pos)
	{
		int r = fseek(pimpl->m_File, pos, SEEK_END);
		if (r < 0)
			throw ErrnoError(Format("Nie mo¿na ustawiæ pozycji w pliku na: # B od koñca") % pos, __FILE__, __LINE__);
	}

	void FileStream::SetSize(size_t Size)
	{
		ftruncate(fileno(pimpl->m_File), Size);
	}

	void FileStream::Truncate()
	{
		SeekableStream::Truncate();
	}

	bool FileStream::End()
	{
		// Fuj! Ale to nieeleganckie. Szkoda, ¿e nie ma lepszego sposobu.
		// UWAGA! Jak by³o zamienione miejscami, najpierw wyliczane GetSize
		// a potem GetPos to za pierwszym wyliczaniem wychodzi³o false a dopiero
		// za drugim true - LOL!
		return (GetPos() == (int)GetSize());
	}

#endif


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa DirLister

class DirLister_pimpl
{
private:
	string m_Dir;

	#ifdef WIN32
		WIN32_FIND_DATAA m_FindData;
		HANDLE m_Handle;
		bool m_ReachedEnd;

		void GetFirst();
		void GetNext();

	#else
		DIR *m_DirHandle;

	#endif

public:
	DirLister_pimpl(const string &Dir);
	~DirLister_pimpl();

	bool ReadNext(string *OutName, FILE_ITEM_TYPE *OutType);
};

#ifdef WIN32
	void DirLister_pimpl::GetFirst()
	{
		string Path;
		IncludeTrailingPathDelimiter(&Path, m_Dir);
		Path += '*';
		m_Handle = FindFirstFileA(Path.c_str(), &m_FindData);

		if (m_Handle == INVALID_HANDLE_VALUE)
		{
			if (GetLastError() == ERROR_NO_MORE_FILES)
				m_ReachedEnd = true;
			else
				throw Win32Error("Nie mo¿na rozpocz¹æ listowania katalogu: " + m_Dir, __FILE__, __LINE__);
		}
		else
			m_ReachedEnd = false;

		while (!m_ReachedEnd && (strcmp(m_FindData.cFileName, ".") == 0 || strcmp(m_FindData.cFileName, "..") == 0))
			GetNext();
	}

	void DirLister_pimpl::GetNext()
	{
		if (FindNextFileA(m_Handle, &m_FindData) == 0)
		{
			if (GetLastError() == ERROR_NO_MORE_FILES)
				m_ReachedEnd = true;
			else
				throw Win32Error("Nie mo¿na kontynuowaæ listowania katalogu: " + m_Dir, __FILE__, __LINE__);
		}

		while (!m_ReachedEnd && (strcmp(m_FindData.cFileName, ".") == 0 || strcmp(m_FindData.cFileName, "..") == 0))
			GetNext();
	}

	DirLister_pimpl::DirLister_pimpl(const string &Dir) :
		m_Dir(Dir)
	{
		GetFirst();
	}

	DirLister_pimpl::~DirLister_pimpl()
	{
		if (m_Handle != INVALID_HANDLE_VALUE)
			FindClose(m_Handle);
	}

	bool DirLister_pimpl::ReadNext(string *OutName, FILE_ITEM_TYPE *OutType)
	{
		if (m_ReachedEnd)
			return false;

		*OutName = m_FindData.cFileName;
		*OutType = ( ((m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) ? IT_DIR : IT_FILE );

		GetNext();

		return true;
	}

#else
	DirLister_pimpl::DirLister_pimpl(const string &Dir) :
		m_Dir(Dir)
	{
		m_DirHandle = opendir(Dir.c_str());
		if (m_DirHandle == NULL)
			throw ErrnoError("Nie mo¿na rozpocz¹æ listowania katalogu: " + m_Dir, __FILE__, __LINE__);
	}

	DirLister_pimpl::~DirLister_pimpl()
	{
		if (m_DirHandle != NULL)
			closedir(m_DirHandle);
	}

	bool DirLister_pimpl::ReadNext(string *OutName, FILE_ITEM_TYPE *OutType)
	{
		dirent DirEnt, *DirEntPtr;
		if ( readdir_r(m_DirHandle, &DirEnt, &DirEntPtr) != 0 )
			throw ErrnoError("Nie mo¿na kontynuowaæ listowania katalogu: " + m_Dir, __FILE__, __LINE__);
		if (DirEntPtr == NULL)
			return false;
		if (strcmp(DirEnt.d_name, ".") == 0 || strcmp(DirEnt.d_name, "..") == 0)
			return ReadNext(OutName, OutType);
		*OutName = DirEnt.d_name;
		*OutType = ( (DirEnt.d_type == DT_DIR) ? IT_DIR : IT_FILE );
		return true;
	}

#endif

DirLister::DirLister(const string &Dir) :
	pimpl(new DirLister_pimpl(Dir))
{
}

DirLister::~DirLister()
{
}

bool DirLister::ReadNext(string *OutName, FILE_ITEM_TYPE *OutType)
{
	return pimpl->ReadNext(OutName, OutType);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje globalne

void SaveStringToFile(const string &FileName, const string &Data)
{
	try
	{
		FileStream f(FileName, FM_WRITE);
		f.WriteStringF(Data);
	}
	catch (Error &e)
	{
		e.Push(Format("Nie mo¿na zapisaæ ³añcucha do pliku: \"#\"") % FileName, __FILE__, __LINE__);
		throw;
	}
}

void SaveDataToFile(const string &FileName, const void *Data, size_t NumBytes)
{
	try
	{
		FileStream f(FileName, FM_WRITE);
		f.Write(Data, NumBytes);
	}
	catch (Error &e)
	{
		e.Push(Format("Nie mo¿na zapisaæ danych binarnych do pliku: \"#\"") % FileName, __FILE__, __LINE__);
		throw;
	}
}

void LoadStringFromFile(const string &FileName, string *Data)
{
	try
	{
		FileStream f(FileName, FM_READ);
		f.ReadStringToEnd(Data);
	}
	catch (Error &e)
	{
		e.Push(Format("Nie mo¿na wczytaæ ³añcucha z pliku: \"#\"") % FileName, __FILE__, __LINE__);
		throw;
	}
}

bool GetFileItemInfo(const string &Path, FILE_ITEM_TYPE *OutType, uint *OutSize, DATETIME *OutModificationTime, DATETIME *OutCreationTime, DATETIME *OutAccessTime)
{
	struct stat S;
	if ( stat(Path.c_str(), &S) != 0 )
		return false;

#ifdef WIN32
	if (OutType != NULL)
		*OutType = ( ((S.st_mode & S_IFDIR) != 0) ? IT_DIR : IT_FILE );
#else
	if (OutType != NULL)
		*OutType = ( S_ISDIR(S.st_mode) ? IT_DIR : IT_FILE );
#endif

	if (OutSize != NULL)
		*OutSize = S.st_size;
	if (OutModificationTime != NULL)
		*OutModificationTime = DATETIME(S.st_mtime);
	if (OutCreationTime != NULL)
		*OutCreationTime = DATETIME(S.st_ctime);
	if (OutAccessTime != NULL)
		*OutAccessTime = DATETIME(S.st_atime);

	return true;
}

void MustGetFileItemInfo(const string &Path, FILE_ITEM_TYPE *OutType, uint *OutSize, DATETIME *OutModificationTime, DATETIME *OutCreationTime, DATETIME *OutAccessTime)
{
	if (!GetFileItemInfo(Path, OutType, OutSize, OutModificationTime, OutCreationTime, OutAccessTime))
		throw ErrnoError("Nie mo¿na pobraæ informacji o: " + Path, __FILE__, __LINE__);
}

FILE_ITEM_TYPE GetFileItemType(const string &Path)
{
#ifdef WIN32
	// szukanie pliku
	DWORD Attr = GetFileAttributesA(Path.c_str());
	// pliku nie ma
	if (Attr == INVALID_FILE_ATTRIBUTES)
		return IT_NONE;
	// to jest katalog
	if ((Attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
		return IT_DIR;
	// nie - to jest plik
	else
		return IT_FILE;

#else
	struct stat s;
	if ( stat(Path.c_str(), &s) != 0 )
		return IT_NONE;
	if (S_ISDIR(s.st_mode))
		return IT_DIR;
	if (S_ISREG(s.st_mode))
		return IT_FILE;
	return IT_NONE;

#endif
}

bool UpdateFileTimeToNow(const string &FileName)
{
	return ( utime(FileName.c_str(), NULL) == 0 );
}

void MustUpdateFileTimeToNow(const string &FileName)
{
	if (!UpdateFileTimeToNow(FileName))
		throw ErrnoError("Nie mo¿na ustawiæ na bie¿¹cy czasu pliku: " + FileName, __FILE__, __LINE__);
}

bool UpdateFileTime(const string &FileName, const DATETIME &ModificationTime, const DATETIME &AccessTime)
{
	utimbuf B;
	B.modtime = ModificationTime.GetTicks();
	B.actime = AccessTime.GetTicks();
	return ( utime(FileName.c_str(), &B) == 0 );
}

void MustUpdateFileTime(const string &FileName, const DATETIME &ModificationTime, const DATETIME &AccessTime)
{
	if (!UpdateFileTime(FileName, ModificationTime, AccessTime))
		throw ErrnoError("Nie mo¿na ustawiæ czasu pliku: " + FileName, __FILE__, __LINE__);
}

bool CreateDirectory(const string &Path)
{
#ifdef WIN32
	return ( mkdir(Path.c_str()) == 0 );
#else
	return ( mkdir(Path.c_str(), 0777) == 0 );
#endif
}

void MustCreateDirectory(const string &Path)
{
	if (!CreateDirectory(Path))
		throw ErrnoError("Nie mo¿na utworzyæ katalogu: " + Path, __FILE__, __LINE__);
}

bool DeleteDirectory(const string &Path)
{
	return ( rmdir(Path.c_str()) == 0 );
}

void MustDeleteDirectory(const string &Path)
{
	if (!DeleteDirectory(Path))
		throw ErrnoError("Nie mo¿na usun¹æ katalogu: " + Path, __FILE__, __LINE__);
}

bool CreateDirectoryChain(const string &Path)
{
	if (GetFileItemType(Path) == IT_DIR)
		return true;

	size_t i2, i = Path.length()-1;
	std::stack<size_t> DirSepStack;
	for (;;)
	{
		i2 = Path.find_last_of(DIR_SEP, i-1);
		if (i2 == string::npos)
			break;
		if (GetFileItemType(Path.substr(0, i2)) == IT_DIR)
			break;
		DirSepStack.push(i2);
		i = i2;
	}

	while (!DirSepStack.empty())
	{
		if ( !CreateDirectory(Path.substr(0, DirSepStack.top())) )
			return false;
		DirSepStack.pop();
	}
	if ( !CreateDirectory(Path) )
		return false;
	return true;
}

void MustCreateDirectoryChain(const string &Path)
{
	if (GetFileItemType(Path) == IT_DIR)
		return;

	size_t i2, i = Path.length()-1;
	std::stack<size_t> DirSepStack;
	for (;;)
	{
		i2 = Path.find_last_of(DIR_SEP, i-1);
		if (i2 == string::npos)
			break;
		if (GetFileItemType(Path.substr(0, i2)) == IT_DIR)
			break;
		DirSepStack.push(i2);
		i = i2;
	}

	while (!DirSepStack.empty())
	{
		MustCreateDirectory(Path.substr(0, DirSepStack.top()));
		DirSepStack.pop();
	}
	MustCreateDirectory(Path);
}

bool DeleteFile(const string &FileName)
{
	return ( unlink(FileName.c_str()) == 0 );
}

void MustDeleteFile(const string &FileName)
{
	if (!DeleteFile(FileName))
		throw ErrnoError("Nie mo¿na usun¹æ pliku: " + FileName, __FILE__, __LINE__);
}

bool MoveItem(const string &OldPath, const string &NewPath)
{
	return ( rename(OldPath.c_str(), NewPath.c_str()) == 0 );
}

void MustMoveItem(const string &OldPath, const string &NewPath)
{
	if (!MoveItem(OldPath, NewPath))
		throw ErrnoError("Nie mo¿na przenieœæ elementu z \"" + OldPath + "\" do \"" + NewPath + "\"", __FILE__, __LINE__);
}

} // namespace common

