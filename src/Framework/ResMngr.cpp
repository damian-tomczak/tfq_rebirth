/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "..\Framework\pch.hpp"
#include "Framework.hpp"
#include "ResMngr.hpp"

namespace res
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// STA�E

// == POLITYKA WYMIANY ==

// Co ile sekund sprawdza� czy nie warto zrobi� wymiany
// Tak�e co ile sekund robi� wymian� piln� je�li jest ma�o pami�ci
const float GC_CHECK_TIME = 3.0f;

// Granica trybu pilnego jako zaj�to�� pami�ci fizycznej w %
const uint4 FREE_ALERT_SYSMEM_LOAD = 90;
// Granica trybu pilnego jako ilo�� wolnej pami�ci tekstur, w MB
const uint4 FREE_ALERT_TEXMEM_FREE = 2;

// Co ile sekund robi� wymian� zwyk�� je�li nie jest ma�o pami�ci
const float GC_NORMAL_TIME = 1.0f * 60.0f;
// Ile sekund od odstatniego u�ycia musi min�� w ramach zwyk�ej wymiany �eby
// zas�b zosta� uznany za stary i zosta� zwolniony
const float GC_NORMAL_OLD_RES_TIME = 2.0f * 60.0f;

// Ile sekund musi by� conajmniej zas�b nieu�ywany, �eby m�g� zosta� usuni�ty
// podczas wymiany pilnej
const float GC_ALERT_OLD_RES_TIME = 3.0f;
// Ile zasob�w na raz co najwy�ej usuwa� w ramach wymiany pilnej
const uint4 GC_ALERT_RES_COUNT = 10;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ResManager_pimpl

class ResManager_pimpl
{
private:
	void GarbageCollect2(bool Alert);
	static bool ResourceOlderCompare(IResource *r1, IResource *r2);

public:
	typedef std::map<string, RES_CREATE_FUNC> RESOURCE_TYPE_MAP;
	typedef std::vector<IResource*> RESOURCE_VECTOR;
	typedef std::set<IResource*> RESOURCE_SET;
	typedef std::map<string, IResource*> RESOURCE_MAP;

	// Zarejestrowane typy zasob�w
	RESOURCE_TYPE_MAP m_ResourceTypes;
	// Zbi�r absolutnie wszystkich zasob�w
	RESOURCE_SET m_AllResources;
	// Mapa wszystkich zasob�w kt�re maj� nazwy: Nazwa => Zas�b
	RESOURCE_MAP m_NamedResources;
	// Zbiory zasob�w
	RESOURCE_SET m_LoadedResources; // Tylko te w stanie ST_LOADED
	RESOURCE_SET m_LockedResources; // Tylko te w stanie ST_LOCKED
	// True je�li jeste�my podczas zwalniania wszystkich zasob�w
	bool m_Deleting;

	float m_GC_LastCheckTime;
	float m_GC_LastCollectTime;

	ResManager_pimpl();
	~ResManager_pimpl();

	// Dodaje zas�b do kolekcji
	void Create(IResource *NewRes, const string &Group);
	// Tworzy pojedynczy zas�b wczytuj�c jego parametry z dokumentu, dodaje go do listy i zwraca
	IResource * Create(const string &Type, const string &Name, STATE State, Tokenizer &Params, const string &Group);
	// Tworzy pojedynczy zas�b parsuj�c jego dane z dokumentu, dodaje go do listy i zwraca
	IResource * CreateFromDocument(Tokenizer &tokenizer, const string &Group);
	void Event(uint4 Type, void *Params);
	// Zwraca w wektorze zasoby nale��ce do podanej grupy (przegl�daj�c wszystkie)
	void GetResourcesFromGroup(RESOURCE_VECTOR *V, const string &Group);
	// Wymiana
	void GarbageCollect();

	// Wywo�uje IResource po zmianie stanu
	void OnResourceStateChange(IResource *Res);

	// ======== Dla IResource ========
	void AddResource(IResource *Res);
	void RemoveResource(IResource *Res);
};


bool ResManager_pimpl::ResourceOlderCompare(IResource *r1, IResource *r2)
{
	return (r1->GetLastUseTime() < r2->GetLastUseTime());
}

ResManager_pimpl::ResManager_pimpl() :
	m_Deleting(false),
	m_GC_LastCheckTime(frame::Timer1.GetTime()),
	m_GC_LastCollectTime(frame::Timer1.GetTime())
{
}

ResManager_pimpl::~ResManager_pimpl()
{
	m_Deleting = true;
	// Zwolnij zasoby
	for (RESOURCE_SET::reverse_iterator it = m_AllResources.rbegin(); it != m_AllResources.rend(); ++it)
		delete *it;
	m_Deleting = false;
}

IResource * ResManager_pimpl::Create(const string &Type, const string &Name, STATE State, Tokenizer &Params, const string &Group)
{
	// Typ
	ResManager_pimpl::RESOURCE_TYPE_MAP::iterator type_it = m_ResourceTypes.find(Type);
	if (type_it == m_ResourceTypes.end())
		throw Error(Format("Nieznany typ zasobu \"#\"") % Type, __FILE__, __LINE__);

	// Utw�rz parsuj�c parametry
	IResource *Res = (*type_it->second)(Name, Group, Params);

	// Opcjonalnie wczytaj
	if (State == ST_LOADED)
		Res->Load();
	else if (State == ST_LOCKED)
		Res->Lock();

	// Zwr��
	return Res;
}

IResource * ResManager_pimpl::CreateFromDocument(Tokenizer &tokenizer, const string &Group)
{
	// Typ
	tokenizer.AssertToken(Tokenizer::TOKEN_IDENTIFIER);
	string Type = tokenizer.GetString();
	tokenizer.Next();

	// Nazwa
	tokenizer.AssertToken(Tokenizer::TOKEN_STRING);
	string Name = tokenizer.GetString();
	tokenizer.Next();

	// Stan
	STATE State = ST_UNLOADED;
	if (tokenizer.GetToken() == Tokenizer::TOKEN_IDENTIFIER)
	{
		if (tokenizer.GetString() == "LOAD")
		{
			State = ST_LOADED;
			tokenizer.Next();
		}
		else if (tokenizer.GetString() == "LOCK")
		{
			State = ST_LOCKED;
			tokenizer.Next();
		}
	}

	return Create(Type, Name, State, tokenizer, Group);
}

void ResManager_pimpl::GetResourcesFromGroup(RESOURCE_VECTOR *V, const string &Group)
{
	for (RESOURCE_SET::iterator it = m_AllResources.begin(); it != m_AllResources.end(); ++it)
	{
		if ((*it)->GetGroup() == Group)
			V->push_back(*it);
	}
}

void ResManager_pimpl::GarbageCollect2(bool Alert)
{
	RESOURCE_VECTOR NormalOld;
	RESOURCE_VECTOR AlertOld;
	RESOURCE_VECTOR::iterator vit;

	float NormalOldTime = frame::Timer1.GetTime() - GC_NORMAL_OLD_RES_TIME;
	float AlertOldTime = 0.0f;
	if (Alert)
		AlertOldTime = frame::Timer1.GetTime() - GC_ALERT_OLD_RES_TIME;

	// Przejd� przez wszystkie zasoby w stanie LOADED, znajd� te do od�adowania
	for (RESOURCE_SET::iterator it = m_LoadedResources.begin(); it != m_LoadedResources.end(); ++it)
	{
		if ((*it)->GetLastUseTime() < NormalOldTime)
			NormalOld.push_back(*it);
		else if (Alert)
		{
			if ((*it)->GetLastUseTime() < AlertOldTime)
				AlertOld.push_back(*it);
		}
	}

	// Usu� te w trybie normalnym
	for (vit = NormalOld.begin(); vit != NormalOld.end(); ++vit)
		(*vit)->Unload();

	// Wybierz i usu� najstarsze z tych w trybie pilnym
	if (Alert)
	{
		std::partial_sort(
			AlertOld.begin(),
			AlertOld.begin() + std::min(AlertOld.size(), GC_ALERT_RES_COUNT),
			AlertOld.end(),
			ResourceOlderCompare);

		uint4 Deleted = 0;
		for (
			vit = AlertOld.begin();
			vit != AlertOld.end() && Deleted < GC_ALERT_RES_COUNT;
			vit++, Deleted++)
		{
			(*vit)->Unload();
		}
	}
}

void ResManager_pimpl::GarbageCollect()
{
	if (m_GC_LastCheckTime + GC_CHECK_TIME < frame::Timer1.GetTime())
	{
		STATS Stats;
		g_Manager->GetStats(&Stats);

		// Tryb pilny
		if (Stats.PhysicalMemoryLoad > FREE_ALERT_SYSMEM_LOAD ||
			Stats.TextureMemoryFree < FREE_ALERT_TEXMEM_FREE)
		{
			GarbageCollect2(true);
			m_GC_LastCollectTime = frame::Timer1.GetTime();
		}
		// Tryb zwyk�y
		else
		{
			if (m_GC_LastCollectTime + GC_NORMAL_TIME < frame::Timer1.GetTime())
			{
				GarbageCollect2(false);
				m_GC_LastCollectTime = frame::Timer1.GetTime();
			}
		}

		m_GC_LastCheckTime = frame::Timer1.GetTime();
	}
}


void ResManager_pimpl::Event(uint4 Type, void *Params)
{
	// Powiadom wszystkie zasoby
	for (RESOURCE_SET::iterator it = m_AllResources.begin(); it != m_AllResources.end(); ++it)
		(*it)->OnEvent(Type, Params);
}

void ResManager_pimpl::OnResourceStateChange(IResource *Res)
{
	// Lista LOADED
	RESOURCE_SET::iterator it = m_LoadedResources.find(Res);
	// - Dodaj
	if (Res->GetState() == ST_LOADED)
	{
		if (it == m_LoadedResources.end())
			m_LoadedResources.insert(Res);
	}
	// - Usu�
	else
	{
		if (it != m_LoadedResources.end())
			m_LoadedResources.erase(it);
	}

	// Lista LOCKED
	it = m_LockedResources.find(Res);
	// - Dodaj
	if (Res->GetState() == ST_LOCKED)
	{
		if (it == m_LockedResources.end())
			m_LockedResources.insert(Res);
	}
	// - Usu�
	else
	{
		if (it != m_LockedResources.end())
			m_LockedResources.erase(it);
	}
}

void ResManager_pimpl::AddResource(IResource *Res)
{
	if (!Res->GetName().empty())
	{
		if (m_NamedResources.insert(RESOURCE_MAP::value_type(Res->GetName(), Res)).second == false)
			throw Error("Nie mo�na doda� zasobu \"" + Res->GetName() + "\" - najprawdopodobniej zas�b o tej nazwie ju� istnieje.", __FILE__, __LINE__);
	}

	if (m_AllResources.insert(Res).second == false)
		throw Error("Nie mo�na doda� zasobu \"" + Res->GetName() + "\".");

	assert(Res->GetState() == ST_UNLOADED);
}

void ResManager_pimpl::RemoveResource(IResource *Res)
{
	if (Res == NULL) return;

	ERR_TRY;

	if (Res->GetState() == ST_LOCKED)
		m_LockedResources.erase(Res);
	else if (Res->GetState() == ST_LOADED)
		m_LoadedResources.erase(Res);

	if (!Res->GetName().empty())
	{
		RESOURCE_MAP::iterator it = m_NamedResources.find(Res->GetName());
		assert(it != m_NamedResources.end());
		m_NamedResources.erase(it);
	}

	if (!m_Deleting)
	{
		RESOURCE_SET::iterator it = m_AllResources.find(Res);
		assert(it != m_AllResources.end());
		m_AllResources.erase(it);
	}

	ERR_CATCH_FUNC;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa IResource

IResource::IResource(const string &Name, const string &Group) :
	m_State(ST_UNLOADED),
	m_Name(Name),
	m_Group(Group),
	m_LockCount(0),
	m_LastUseTime(frame::Timer1.GetTime())
{
	assert(g_Manager != NULL);
	g_Manager->pimpl->AddResource(this);
}

IResource::~IResource()
{
	assert(g_Manager != NULL);
	g_Manager->pimpl->RemoveResource(this);
}

void IResource::Unloaded()
{
	m_State = ST_UNLOADED;
	g_Manager->pimpl->OnResourceStateChange(this);
}

void IResource::Load()
{
	ERR_TRY;

	if (GetState() == ST_UNLOADED)
	{
		OnLoad();
		m_State = ST_LOADED;
		g_Manager->pimpl->OnResourceStateChange(this);
	}
	m_LastUseTime = frame::Timer1.GetTime();

	ERR_CATCH_FUNC;
}

void IResource::Unload()
{
	if (GetState() == ST_LOADED)
	{
		try
		{
			OnUnload();
		}
		catch (...) { }
		m_State = ST_UNLOADED;
		g_Manager->pimpl->OnResourceStateChange(this);
	}
}

void IResource::Lock()
{
	ERR_TRY;

	if (GetState() == ST_UNLOADED)
	{
		OnLoad();
		m_State = ST_LOCKED;
		g_Manager->pimpl->OnResourceStateChange(this);
	}
	else if (GetState() == ST_LOADED)
	{
		m_State = ST_LOCKED;
		g_Manager->pimpl->OnResourceStateChange(this);
	}
	m_LockCount++;
	m_LastUseTime = frame::Timer1.GetTime();

	ERR_CATCH_FUNC;
}

void IResource::Unlock()
{
	if (GetState() == ST_LOCKED)
	{
		m_LockCount--;
		if (m_LockCount == 0)
		{
			m_State = ST_LOADED;
			g_Manager->pimpl->OnResourceStateChange(this);
		}
	}
	// Tu te� uaktualniam po to, �eby taki odblokowany zas�b nie od�adowa� si� natychmiast tylko dlatego,
	// �e dawno nikt nie robi� mu Lock ani Load.
	m_LastUseTime = frame::Timer1.GetTime();
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa ResManager

ResManager::ResManager() :
	pimpl(new ResManager_pimpl())
{
}

ResManager::~ResManager()
{
}

void ResManager::RegisterResourceType(const string &TypeName, RES_CREATE_FUNC CreateFunc)
{
	if (pimpl->m_ResourceTypes.insert(std::make_pair(TypeName, CreateFunc)).second == false)
		throw Error("Nie mo�na zarejestrowa� typu zasob�w: "+TypeName, __FILE__, __LINE__);
}

void ResManager::OnFrame()
{
	ERR_TRY;

	// Wymiana
	pimpl->GarbageCollect();

	ERR_CATCH_FUNC;
}

void ResManager::GetStats(STATS *Stats)
{
	// Pami�� systemowa
	MEMORYSTATUS MemoryStatus;
	MemoryStatus.dwLength = sizeof(MemoryStatus);
	GlobalMemoryStatus(&MemoryStatus);
	Stats->PhysicalMemoryLoad = MemoryStatus.dwMemoryLoad;

	// Pami�� tekstur
	if (frame::Dev)
		Stats->TextureMemoryFree = frame::Dev->GetAvailableTextureMem() / (1024*1024);
	else
		Stats->TextureMemoryFree = MAXUINT4;

	Stats->ResourceCount = pimpl->m_AllResources.size();
	Stats->LoadedCount = pimpl->m_LoadedResources.size();
	Stats->LockedCount = pimpl->m_LockedResources.size();
}

void ResManager::Event(uint4 Type, void *Params)
{
	ERR_TRY;

	pimpl->Event(Type, Params);

	ERR_CATCH(Format("res::ResManager::Event: Type = #") % Type);
}

void ResManager::CreateFromFile(const string &FileName, const string &Group)
{
	ERR_TRY;

	FileStream file(FileName, FM_READ);
	Tokenizer tokenizer(&file, 0);
	tokenizer.Next();

	while (tokenizer.GetToken() != Tokenizer::TOKEN_EOF)
		pimpl->CreateFromDocument(tokenizer, Group);

	ERR_CATCH("Nie mo�na wczyta� zasob�w z pliku: " + FileName);
}

IResource * ResManager::CreateFromString(const string &Data, const string &Group)
{
	Tokenizer tokenizer(&Data, 0);
	return pimpl->CreateFromDocument(tokenizer, Group);
}

IResource * ResManager::CreateFromString(const string &Type, const string &Name, STATE State, const string &Params, const string &Group)
{
	Tokenizer tokenizer(&Params, 0);
	return pimpl->Create(Type, Name, State, tokenizer, Group);
}

bool ResManager::Destroy(const string &Name)
{
	IResource * Res = GetResource(Name);
	if (Res == NULL)
		return false;
	delete Res;
	return true;
}

uint ResManager::DestroyGroup(const string &Group)
{
	ERR_TRY;

	ResManager_pimpl::RESOURCE_VECTOR V;
	pimpl->GetResourcesFromGroup(&V, Group);

	for (ResManager_pimpl::RESOURCE_VECTOR::iterator vit = V.begin(); vit != V.end(); ++vit)
		delete *vit;

	return V.size();

	ERR_CATCH_FUNC;
}

IResource * ResManager::GetResource(const string &Name)
{
	ResManager_pimpl::RESOURCE_MAP::iterator it = pimpl->m_NamedResources.find(Name);
	if (it == pimpl->m_NamedResources.end())
		return 0;
	else
		return it->second;
}

IResource * ResManager::MustGetResource(const string &Name)
{
	ResManager_pimpl::RESOURCE_MAP::iterator it = pimpl->m_NamedResources.find(Name);
	if (it == pimpl->m_NamedResources.end())
		throw Error("res::ResManager::MustGetResource: Zas�b o nazwie \"" + Name + "\" nie istnieje.", __FILE__, __LINE__);
	return it->second;
}

int ResManager::LoadGroup(const string &Group)
{
	ResManager_pimpl::RESOURCE_VECTOR V;
	pimpl->GetResourcesFromGroup(&V, Group);

	for (ResManager_pimpl::RESOURCE_VECTOR::iterator vit = V.begin(); vit != V.end(); ++vit)
		(*vit)->Load();

	return V.size();
}

int ResManager::UnloadGroup(const string &Group)
{
	ResManager_pimpl::RESOURCE_VECTOR V;
	pimpl->GetResourcesFromGroup(&V, Group);

	for (ResManager_pimpl::RESOURCE_VECTOR::iterator vit = V.begin(); vit != V.end(); ++vit)
		(*vit)->Unload();

	return V.size();
}

int ResManager::LockGroup(const string &Group)
{
	ResManager_pimpl::RESOURCE_VECTOR V;
	pimpl->GetResourcesFromGroup(&V, Group);

	for (ResManager_pimpl::RESOURCE_VECTOR::iterator vit = V.begin(); vit != V.end(); ++vit)
		(*vit)->Lock();

	return V.size();
}

int ResManager::UnlockGroup(const string Group)
{
	ResManager_pimpl::RESOURCE_VECTOR V;
	pimpl->GetResourcesFromGroup(&V, Group);

	for (ResManager_pimpl::RESOURCE_VECTOR::iterator vit = V.begin(); vit != V.end(); ++vit)
		(*vit)->Unlock();

	return V.size();
}

ResManager *g_Manager = NULL;


} // namespace res
