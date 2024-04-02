/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef RES_MNGR_H_
#define RES_MNGR_H_

namespace res
{

enum STATE
{
	ST_UNLOADED = 0,
	ST_LOADED = 1,
	ST_LOCKED = 2
};

class IResource
{
	friend class ResManager;
	friend class ResManager_pimpl;

private:
	STATE m_State;
	string m_Name;
	string m_Group;
	uint4 m_LockCount;
	float m_LastUseTime;

	// Dla Managera do realizowania wymiany
	float GetLastUseTime() { return m_LastUseTime; }

protected:
	IResource(const string &Name, const string &Group = "");

	virtual void OnLoad() = 0;
	virtual void OnUnload() = 0;
	// Wywo�ywane przez klas� ResManager
	// W tej funkcji nie wolno tworzy� ani niszczy� zasob�w.
	virtual void OnEvent(uint4 Type, void *Params) { }
	// Wywo�a� je�li zas�b od�adowa� sam siebie
	// Wolno to robi� tylko je�li jest w stanie ST_LOADED i wewn�trz metody OnEvent.
	void Unloaded();

public:
	virtual ~IResource();

	// �aduje zas�b, je�li nie jest za�adowany
	// Po tym wywo�aniu, o ile nie polecia� wyj�tek z b��dem, zas�b b�dzie napewno dost�pny -
	// ale tylko do ko�ca klatki, w nast�pnej ju� m�g� zosta� od�adowany.
	// Je�li nie stosujemy blokowania, u�ywa� tej funkcji przed ka�dym u�yciem zasobu
	// lub przynajmniej przed pierwszym w ka�dej klatce.
	// Wielokrotne wywo�ywanie nic nie robi. Je�li jest zablokowany, te� nic nie robi,
	// bo jest na pewno za�adowany.
	void Load();
	// Od�adowuje zas�b
	// To jest tylko sugestia - nie ma gwarancji, �e zas�b zostanie natychmiast (ani w og�le)
	// od�adowany - ale najprawdopodobniej tak.
	// Je�li jest zablokowany, nic nie robi.
	void Unload();
	// Blokuje zas�b, tak �e nie b�dzie m�g� zosta� od�adowany a� do odblokowania.
	// Je�li nie by� za�adowany, �aduje go natychmiast.
	// Je�li by� zablokowany, zwi�ksza licznik zablokowa�.
	void Lock();
	// Odblokowuje zas�b. Nie powoduje jego od�adowania - przynajmniej a� do nast�pnej klatki.
	// Je�li zas�b by� zablokowany wiele razy, zmniejsza tylko licznik zablokowa�.
	// Je�li nie by� zablokowany, nic nie robi.
	void Unlock();

	STATE GetState() { return m_State; }
	bool IsLoaded() { return m_State != ST_UNLOADED; }
	const string & GetName() { return m_Name; }
	const string & GetGroup() { return m_Group; }

	// Ma za zadanie utworzy� zas�b o podanej nazwie i parametrach
	// Parametry maj� by� parsowane a� do napotkania �rednika ';'.
	// Je�li nie uda si� utworzy�, ma rzuci� wyj�tek.
	// Je�li b��d sk�adni XNL2, ma wywo�a� x.CreateError.
	// static IResource * Create(const string &Name, xnl::Reader &x);
};

// Ma za zadanie sparsowa� parametry zasobu razem z ko�cz�cym �rednikiem.
typedef IResource * (*RES_CREATE_FUNC)(const string &Name, const string &Group, Tokenizer &Params);

struct STATS
{
	// Obci��enie pami�ci fizycznej RAM, w procentach (0..100)
	uint4 PhysicalMemoryLoad;
	// Wolna pami�� na tekstury, w MB
	// MAXUINT4 je�li warto�� niedost�pna.
	uint4 TextureMemoryFree;
	// Liczba wszystkich zasob�w w managerze
	uint4 ResourceCount;
	// Liczba zasob�w w stanie LOADED (nie LOCKED i nie UNLOADED)
	uint4 LoadedCount;
	// Liczba zasob�w w stanie LOCKED
	uint4 LockedCount;
};

class ResManager_pimpl;

class ResManager
{
	friend class IResource;

private:
	scoped_ptr<ResManager_pimpl> pimpl;

public:
	ResManager();
	~ResManager();

	void RegisterResourceType(const string &TypeName, RES_CREATE_FUNC CreateFunc);
	void OnFrame();
	void GetStats(STATS *Stats);
	void Event(uint4 Type, void *Params);

	void CreateFromFile(const string &FileName, const string &Group = "");
	IResource * CreateFromString(const string &Data, const string &Group = "");
	IResource * CreateFromString(const string &Type, const string &Name, STATE State, const string &Params, const string &Group = "");

	bool Destroy(const string &Name);
	uint DestroyGroup(const string &Group);

	// Zwraca zas�b o podanej nazwie lub 0 je�li nie istnieje.
	IResource * GetResource(const string &Name);
	// Zwraca zas�b o podanej nazwie lub rzuca wyj�tek je�li nie istnieje.
	IResource * MustGetResource(const string &Name);
	// Zwraca zas�b o podanej nazwie podanego typu.
	// Je�li nie istnieje lub nie jest tego typu, zwraca 0.
	template <typename T>
	T * GetResourceEx(const string &Name)
	{
		IResource *R = GetResource(Name);
		if (R == 0) return 0;
		return dynamic_cast<T*>(R);
	}
	// Zwraca zas�b o podanej nazwie podanego typu.
	// Je�li nie istnieje lub nie jest tego typu, rzuca wyj�tek.
	template <typename T>
	T * MustGetResourceEx(const string &Name)
	{
		IResource *R = MustGetResource(Name);
		if (typeid(*R) != typeid(T)) throw Error(Format("res::ResManager::MustGetResourceEx: B��dny typ zasobu. Nazwa: #, oczekiwany: #, aktualny: #") % Name % typeid(T).name() % typeid(*R).name(), __FILE__, __LINE__);
		return static_cast<T*>(R);
	}
	// Zwraca zas�b o podanej nazwie podanego typu lub 0 je�li nie istnieje.
	// Na docelowy tym rzutuje szybko ale i niebezpiecznie, bez sprawdzania.
	template <typename T>
	T * GetResourceExf(const string &Name)
	{
		IResource *R = GetResource(Name);
		if (R == 0) return 0;
		return static_cast<T*>(R);
	}
	// Zwraca zas�b o podanej nazwie podanego typu lub rzuca wyj�tek je�li nie istnieje.
	// Na docelowy tym rzutuje szybko ale i niebezpiecznie, bez sprawdzania.
	template <typename T>
	T * MustGetResourceExf(const string &Name)
	{
		return static_cast<T*>(MustGetResource(Name));
	}

	// Zwraca true, je�li istnieje zas�b o podanej nazwie
	bool Exists(const string &Name)
	{
		return GetResource(Name) != 0;
	}
	// Zwrac true, je�li istnieje zas�b o podanej nazwie i jest podanego typu
	template <typename T>
	bool IsOfType(const string &Name)
	{
		return GetResourceEx(Name) != 0;
	}

	bool Load(const string &Name)
	{
		IResource *r = GetResource(Name); if (r) { r->Load(); return true; } else return false;
	}
	bool Unload(const string &Name)
	{
		IResource *r = GetResource(Name); if (r) { r->Unload(); return true; } else return false;
	}
	bool Lock(const string &Name)
	{
		IResource *r = GetResource(Name); if (r) { r->Lock(); return true; } else return false;
	}
	bool Unlock(const string &Name)
	{
		IResource *r = GetResource(Name); if (r) { r->Unlock(); return true; } else return false;
	}
	int LoadGroup(const string &Group);
	int UnloadGroup(const string &Group);
	int LockGroup(const string &Group);
	int UnlockGroup(const string Group);
};

extern ResManager *g_Manager;

} // namespace res

#endif
