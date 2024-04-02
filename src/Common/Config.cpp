/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Config - Obs³uga plików konfiguracyjnych
 * Dokumentacja: Patrz plik doc/Config.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include <typeinfo>
#include <map>
#include "Error.hpp"
#include "Stream.hpp"
#include "Files.hpp"
#include "Tokenizer.hpp"
#include "Config.hpp"


namespace common
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Config_pimpl

typedef shared_ptr<Item> ITEM_SHARED_PTR;
typedef std::map<string, ITEM_SHARED_PTR> ITEM_MAP;

class Config_pimpl
{
public:
	ITEM_MAP ItemMap;

	void ReadConfig(Tokenizer &T, bool NestedConfig);
	void WriteConfig(CharWriter &W, uint Level);
};

void Config_pimpl::ReadConfig(Tokenizer &T, bool NestedConfig)
{
	string Key;
	for (;;)
	{
		// To jest config wewnêtrzny - zakoñczeniem bêdzie '}'
		if (NestedConfig)
		{
			if (T.GetToken() == Tokenizer::TOKEN_SYMBOL && T.GetChar() == '}')
			{
				T.Next();
				break;
			}
		}
		// To nie jest config wewnêtrzny - zakoñczeniem bêdzie koniec pliku
		else
		{
			if (T.GetToken() == Tokenizer::TOKEN_EOF)
				break;
		}

		// Klucz
		T.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
		Key = T.GetString();
		T.Next();

		// Jest znak równoœci
		if (T.GetToken() == Tokenizer::TOKEN_SYMBOL && T.GetChar() == '=')
		{
			T.Next();

			// '{' - wektor
			if (T.GetToken() == Tokenizer::TOKEN_SYMBOL && T.GetChar() == '{')
			{
				T.Next();

				string V;
				shared_ptr<List> SubList(new List);
				if (ItemMap.insert(std::make_pair(Key, SubList)).second == false)
					throw Error("Nie mo¿na dodaæ listy o nazwie: " + Key);

				bool First = true;
				for (;;)
				{
					if (T.GetToken() == Tokenizer::TOKEN_SYMBOL && T.GetChar() == '}')
					{
						T.Next();
						break;
					}
					else if (First || T.GetToken() == Tokenizer::TOKEN_SYMBOL && T.GetChar() == ',')
					{
						if (!First)
							T.Next();

						if (
							T.GetToken() == Tokenizer::TOKEN_IDENTIFIER ||
							T.GetToken() == Tokenizer::TOKEN_INTEGER || T.GetToken() == Tokenizer::TOKEN_FLOAT ||
							T.GetToken() == Tokenizer::TOKEN_CHAR || T.GetToken() == Tokenizer::TOKEN_STRING)
						{
							First = false;
							SubList->Data.push_back(T.GetString());
							T.Next();
						}
						else
							T.CreateError();
					}
					else
						T.CreateError();
				}
			}
			// Wartoœæ
			else if (
				T.GetToken() == Tokenizer::TOKEN_IDENTIFIER ||
				T.GetToken() == Tokenizer::TOKEN_INTEGER || T.GetToken() == Tokenizer::TOKEN_FLOAT ||
				T.GetToken() == Tokenizer::TOKEN_CHAR || T.GetToken() == Tokenizer::TOKEN_STRING)
			{
				shared_ptr<Value> SubValue(new Value(T.GetString()));
				if (ItemMap.insert(std::make_pair(Key, SubValue)).second == false)
					throw Error("Nie mo¿na dodaæ wartoœci o nazwie: " + Key);

				T.Next();
			}
		}
		// Podconfig
		else if (T.GetToken() == Tokenizer::TOKEN_SYMBOL && T.GetChar() == '{')
		{
			T.Next();

			shared_ptr<Config> SubConfig(new Config);
			if (ItemMap.insert(std::make_pair(Key, SubConfig)).second == false)
				throw Error("Nie mo¿na dodaæ podkonfiguracji o nazwie: " + Key);
			SubConfig->pimpl->ReadConfig(T, true);
		}
		else
			T.CreateError();
	}
}

void Config_pimpl::WriteConfig(CharWriter &W, uint Level)
{
	for (ITEM_MAP::iterator it = ItemMap.begin(); it != ItemMap.end(); ++it)
	{
		// Wciêcie
		for (uint i = 0; i < Level; i++)
			W.WriteChar('\t');
		// Nazwa
		W.WriteString(it->first);

		// Jeœli to podkonfiguracja
		if (Config *SubConfig = dynamic_cast<Config*>(it->second.get()))
		{
			W.WriteChar(' ');
			W.WriteChar('{');
			W.WriteString(EOL);
			SubConfig->pimpl->WriteConfig(W, Level + 1);
			// Wciêcie
			for (uint i = 0; i < Level; i++)
				W.WriteChar('\t');
			W.WriteChar('}');
			W.WriteString(EOL);
		}
		// Jeœli to nie podkonfiguracja
		else
		{
			W.WriteChar(' ');
			W.WriteChar('=');
			W.WriteChar(' ');
			// Jeœli to wartoœæ
			if (Value *SubValue = dynamic_cast<Value*>(it->second.get()))
			{
				string Escaped;
				Tokenizer::Escape(&Escaped, SubValue->Data, 0);
				W.WriteChar('\"');
				W.WriteString(Escaped);
				W.WriteChar('\"');
				W.WriteString(EOL);
			}
			// Jeœli to lista
			else if (List *SubList = dynamic_cast<List*>(it->second.get()))
			{
				W.WriteChar('{');
				W.WriteChar(' ');

				for (size_t Index = 0; Index < SubList->Data.size(); Index++)
				{
					string Escaped;
					Tokenizer::Escape(&Escaped, SubList->Data[Index], 0);
					W.WriteChar('\"');
					W.WriteString(Escaped);
					W.WriteChar('\"');
					if (Index < SubList->Data.size()-1)
						W.WriteChar(',');
					W.WriteChar(' ');
				}
				W.WriteChar('}');
				W.WriteString(EOL);
			}
			// B³¹d wewnêtrzny
			else
				assert(0 && "Impossible!");
		}
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Config

Config::Config() :
	pimpl(new Config_pimpl)
{
}

Config::~Config()
{
}

/*void Config::CreateError(const string &Msg)
{
	throw Error(Msg);
}*/

void Config::LoadFromString(const string &S)
{
	ERR_TRY;

	pimpl->ItemMap.clear();
	Tokenizer T(&S, Tokenizer::FLAG_MULTILINE_STRINGS);
	T.Next();
	pimpl->ReadConfig(T, false);

	ERR_CATCH("Nie mo¿na wczytaæ konfiguracji z ³añcucha.");
}

void Config::LoadFromStream(Stream *S)
{
	ERR_TRY;

	pimpl->ItemMap.clear();
	Tokenizer T(S, Tokenizer::FLAG_MULTILINE_STRINGS);
	T.Next();
	pimpl->ReadConfig(T, false);

	ERR_CATCH("Nie mo¿na wczytaæ konfiguracji ze strumienia.");
}

void Config::LoadFromFile(const string &FileName)
{
	ERR_TRY;

	pimpl->ItemMap.clear();
	FileStream File(FileName, FM_READ);
	Tokenizer T(&File, Tokenizer::FLAG_MULTILINE_STRINGS);
	T.Next();
	pimpl->ReadConfig(T, false);

	ERR_CATCH("Nie mo¿na wczytaæ konfiguracji z pliku: " + FileName);
}

void Config::SaveToString(string *S) const
{
	ERR_TRY;

	S->clear();
	StringStream StringStream(S);
	CharWriter Writer(&StringStream);
	pimpl->WriteConfig(Writer, 0);

	ERR_CATCH("Nie mo¿na zapisaæ konfiguracji do ³añcucha.");
}

void Config::SaveToStream(Stream *S) const
{
	ERR_TRY;

	CharWriter Writer(S);
	pimpl->WriteConfig(Writer, 0);

	ERR_CATCH("Nie mo¿na zapisaæ konfiguracji do ³añcucha.");
}

void Config::SaveToFile(const string &FileName) const
{
	ERR_TRY;

	FileStream File(FileName, FM_WRITE);
	CharWriter Writer(&File);
	pimpl->WriteConfig(Writer, 0);

	ERR_CATCH("Nie mo¿na zapisaæ konfiguracji do pliku: " + FileName);
}

Item * Config::GetItem(const string &Path)
{
	size_t SlashPos = Path.find('/');
	// Nie ma ju¿ w œcie¿ce nastêpnego slasha - to coœ w tym configu
	if (SlashPos == string::npos)
	{
		ITEM_MAP::iterator it = pimpl->ItemMap.find(Path);
		if (it == pimpl->ItemMap.end())
			return NULL;
		else
			return it->second.get();
	}
	// Jest nastêpny slash - podaj dalej
	else
	{
		string Begin = Path.substr(0, SlashPos);
		string Rest = Path.substr(SlashPos+1);
		ITEM_MAP::iterator it = pimpl->ItemMap.find(Begin);
		if (it == pimpl->ItemMap.end())
			return NULL;
		else
		{
			if (Config *SubConfig = dynamic_cast<Config*>(it->second.get()))
				return SubConfig->GetItem(Rest);
			else
				return NULL;
		}
	}
}

Item * Config::MustGetItem(const string &Path)
{
	Item * R = GetItem(Path);
	if (R == NULL)
		throw Error("Nie znaleziono w konfiguracji elementu o œcie¿ce: " + Path);
	return R;
}

bool Config::GetData(const string &Path, string *Data)
{
	Item *item = GetItem(Path);
	if (item == NULL)
		return false;
	if (Value *di = dynamic_cast<Value*>(item))
	{
		*Data = di->Data;
		return true;
	}
	else
		return false;
}

void Config::MustGetData(const string &Path, string *Data)
{
	if (!GetData(Path, Data))
		throw Error("Nie mo¿na odczytaæ z konfiguracji danych elementu o œcie¿ce: " + Path);
}

bool Config::ItemExists(const string &Path)
{
	size_t SlashPos = Path.find('/');
	// Nie ma ju¿ w œcie¿ce nastêpnego slasha - to coœ w tym configu
	if (SlashPos == string::npos)
		return pimpl->ItemMap.find(Path) != pimpl->ItemMap.end();
	// Jest nastêpny slash - podaj dalej
	else
	{
		string Begin = Path.substr(0, SlashPos);
		string Rest = Path.substr(SlashPos+1);
		ITEM_MAP::iterator it = pimpl->ItemMap.find(Begin);
		if (it == pimpl->ItemMap.end())
			return false;
		else
		{
			if (Config *SubConfig = dynamic_cast<Config*>(it->second.get()))
				return SubConfig->ItemExists(Rest);
			else
				return false;
		}
	}
}

void Config::ItemMustExists(const string &Path)
{
	if (!ItemExists(Path))
		throw Error("Element konfiguracji o œcie¿ce: \"" + Path + "\" nie istnieje.");
}

bool Config::SetItem(const string &Path, Item *a_Item, uint4 CreateMode)
{
	size_t SlashPos = Path.find('/');
	// Nie ma ju¿ w œcie¿ce nastêpnego slasha - to coœ w tym configu
	if (SlashPos == string::npos)
	{
		ITEM_MAP::iterator it = pimpl->ItemMap.find(Path);
		// Element nie istnieje
		if (it == pimpl->ItemMap.end())
		{
			// Musi istnieæ - b³¹d
			if ((CreateMode & CM_MUST_EXIST))
				return false;
			// Nie musi - tworzymy
			pimpl->ItemMap.insert(std::make_pair(Path, shared_ptr<Item>(a_Item)));
			return true;
		}
		// Element istnieje
		else
		{
			// Musi nie istnieæ - b³¹d
			if ((CreateMode & CM_MUST_NOT_EXIST))
				return false;
			// Musi byæ tego samego typu i jest innego - b³¹d
			if ( ((CreateMode & CM_SAME_TYPE)) && (typeid(a_Item) != typeid(it->second.get())) )
				return false;
			// OK - ustawiamy (stary sam siê zwolni)
			it->second.reset(a_Item);
			return true;
		}
	}
	// Jest nastêpny slash - podaj dalej
	else
	{
		string Begin = Path.substr(0, SlashPos);
		string Rest = Path.substr(SlashPos+1);
		ITEM_MAP::iterator it = pimpl->ItemMap.find(Begin);
		if (it == pimpl->ItemMap.end())
		{
			if ((CreateMode & CM_CREATE_PATH))
			{
				Config *SubConfig = new Config();
				pimpl->ItemMap.insert(std::make_pair(Begin, shared_ptr<Item>(SubConfig)));
				return SubConfig->SetItem(Rest, a_Item, CreateMode);
			}
			else
				return false;
		}
		else
		{
			if (Config *SubConfig = dynamic_cast<Config*>(it->second.get()))
				return SubConfig->SetItem(Rest, a_Item, CreateMode);
			else
				return false;
		}
	}
}

void Config::MustSetItem(const string &Path, Item *a_Item, uint4 CreateMode)
{
	if (!SetItem(Path, a_Item, CreateMode))
		throw Error("Nie mo¿na ustawiæ elementu konfiguracji pod œcie¿k¹: " + Path);
}

bool Config::SetData(const string &Path, const string &Data, uint4 CreateMode)
{
	size_t SlashPos = Path.find('/');
	// Nie ma ju¿ w œcie¿ce nastêpnego slasha - to coœ w tym configu
	if (SlashPos == string::npos)
	{
		ITEM_MAP::iterator it = pimpl->ItemMap.find(Path);
		// Element nie istnieje
		if (it == pimpl->ItemMap.end())
		{
			// Musi istnieæ - b³¹d
			if ((CreateMode & CM_MUST_EXIST))
				return false;
			// Nie musi - tworzymy
			Item *i = new Value(Data);
			pimpl->ItemMap.insert(std::make_pair(Path, shared_ptr<Item>(i)));
			return true;
		}
		// Element istnieje
		else
		{
			// Musi nie istnieæ - b³¹d
			if ((CreateMode & CM_MUST_NOT_EXIST))
				return false;
			// Musi byæ Value
			if (Value *di = dynamic_cast<Value*>(it->second.get()))
			{
				// Ustawiamy
				di->Data = Data;
				return true;
			}
			else
				return false;
		}
	}
	// Jest nastêpny slash - podaj dalej
	else
	{
		string Begin = Path.substr(0, SlashPos);
		string Rest = Path.substr(SlashPos+1);
		ITEM_MAP::iterator it = pimpl->ItemMap.find(Begin);
		if (it == pimpl->ItemMap.end())
		{
			if ((CreateMode & CM_CREATE_PATH))
			{
				Config *SubConfig = new Config();
				pimpl->ItemMap.insert(std::make_pair(Begin, shared_ptr<Item>(SubConfig)));
				return SubConfig->SetData(Rest, Data, CreateMode);
			}
			else
				return false;
		}
		else
		{
			if (Config *SubConfig = dynamic_cast<Config*>(it->second.get()))
				return SubConfig->SetData(Rest, Data, CreateMode);
			else
				return false;
		}
	}
}

void Config::MustSetData(const string &Path, const string &Data, uint4 CreateMode)
{
	if (!SetData(Path, Data, CreateMode))
		throw Error("Nie mo¿na wpisaæ danych do konfiguracji pod œcie¿kê: " + Path);
}

bool Config::DeleteItem(const string &Path)
{
	size_t SlashPos = Path.find('/');
	// Nie ma ju¿ w œcie¿ce nastêpnego slasha - to coœ w tym configu
	if (SlashPos == string::npos)
	{
		ITEM_MAP::iterator it = pimpl->ItemMap.find(Path);
		// Element nie istnieje
		if (it == pimpl->ItemMap.end())
			return false;
		// Element istnieje
		else
		{
			// Usuñ go
			pimpl->ItemMap.erase(it);
			return true;
		}
	}
	// Jest nastêpny slash - podaj dalej
	else
	{
		string Begin = Path.substr(0, SlashPos);
		string Rest = Path.substr(SlashPos+1);
		ITEM_MAP::iterator it = pimpl->ItemMap.find(Begin);
		// Nie ma pod-configa o nazwie Begin
		if (it == pimpl->ItemMap.end())
			return false;
		// Przeka¿ do pod-configa o nazwie Begin
		else
		{
			// To musi byæ podconfig
			if (Config *SubConfig = dynamic_cast<Config*>(it->second.get()))
				return SubConfig->DeleteItem(Rest);
			else
				return false;
		}
	}
}

void Config::MustDeleteItem(const string &Path)
{
	if (!DeleteItem(Path))
		throw Error("Nie mo¿na usun¹æ elementu konfiguracji: " + Path, __FILE__, __LINE__);
}

void Config::GetElementNames(STRING_VECTOR *OutNames)
{
	OutNames->clear();
	ITEM_MAP::iterator it = pimpl->ItemMap.begin();
	while (it != pimpl->ItemMap.end())
	{
		OutNames->push_back(it->first);
		++it;
	}
}

scoped_ptr<Config> g_Config;

} // namespace common
