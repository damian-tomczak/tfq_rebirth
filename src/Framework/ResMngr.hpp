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
	// Wywo³ywane przez klasê ResManager
	// W tej funkcji nie wolno tworzyæ ani niszczyæ zasobów.
	virtual void OnEvent(uint4 Type, void *Params) { }
	// Wywo³aæ jeœli zasób od³adowa³ sam siebie
	// Wolno to robiæ tylko jeœli jest w stanie ST_LOADED i wewn¹trz metody OnEvent.
	void Unloaded();

public:
	virtual ~IResource();

	// £aduje zasób, jeœli nie jest za³adowany
	// Po tym wywo³aniu, o ile nie polecia³ wyj¹tek z b³êdem, zasób bêdzie napewno dostêpny -
	// ale tylko do koñca klatki, w nastêpnej ju¿ móg³ zostaæ od³adowany.
	// Jeœli nie stosujemy blokowania, u¿ywaæ tej funkcji przed ka¿dym u¿yciem zasobu
	// lub przynajmniej przed pierwszym w ka¿dej klatce.
	// Wielokrotne wywo³ywanie nic nie robi. Jeœli jest zablokowany, te¿ nic nie robi,
	// bo jest na pewno za³adowany.
	void Load();
	// Od³adowuje zasób
	// To jest tylko sugestia - nie ma gwarancji, ¿e zasób zostanie natychmiast (ani w ogóle)
	// od³adowany - ale najprawdopodobniej tak.
	// Jeœli jest zablokowany, nic nie robi.
	void Unload();
	// Blokuje zasób, tak ¿e nie bêdzie móg³ zostaæ od³adowany a¿ do odblokowania.
	// Jeœli nie by³ za³adowany, ³aduje go natychmiast.
	// Jeœli by³ zablokowany, zwiêksza licznik zablokowañ.
	void Lock();
	// Odblokowuje zasób. Nie powoduje jego od³adowania - przynajmniej a¿ do nastêpnej klatki.
	// Jeœli zasób by³ zablokowany wiele razy, zmniejsza tylko licznik zablokowañ.
	// Jeœli nie by³ zablokowany, nic nie robi.
	void Unlock();

	STATE GetState() { return m_State; }
	bool IsLoaded() { return m_State != ST_UNLOADED; }
	const string & GetName() { return m_Name; }
	const string & GetGroup() { return m_Group; }

	// Ma za zadanie utworzyæ zasób o podanej nazwie i parametrach
	// Parametry maj¹ byæ parsowane a¿ do napotkania œrednika ';'.
	// Jeœli nie uda siê utworzyæ, ma rzuciæ wyj¹tek.
	// Jeœli b³¹d sk³adni XNL2, ma wywo³aæ x.CreateError.
	// static IResource * Create(const string &Name, xnl::Reader &x);
};

// Ma za zadanie sparsowaæ parametry zasobu razem z koñcz¹cym œrednikiem.
typedef IResource * (*RES_CREATE_FUNC)(const string &Name, const string &Group, Tokenizer &Params);

struct STATS
{
	// Obci¹¿enie pamiêci fizycznej RAM, w procentach (0..100)
	uint4 PhysicalMemoryLoad;
	// Wolna pamiêæ na tekstury, w MB
	// MAXUINT4 jeœli wartoœæ niedostêpna.
	uint4 TextureMemoryFree;
	// Liczba wszystkich zasobów w managerze
	uint4 ResourceCount;
	// Liczba zasobów w stanie LOADED (nie LOCKED i nie UNLOADED)
	uint4 LoadedCount;
	// Liczba zasobów w stanie LOCKED
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

	// Zwraca zasób o podanej nazwie lub 0 jeœli nie istnieje.
	IResource * GetResource(const string &Name);
	// Zwraca zasób o podanej nazwie lub rzuca wyj¹tek jeœli nie istnieje.
	IResource * MustGetResource(const string &Name);
	// Zwraca zasób o podanej nazwie podanego typu.
	// Jeœli nie istnieje lub nie jest tego typu, zwraca 0.
	template <typename T>
	T * GetResourceEx(const string &Name)
	{
		IResource *R = GetResource(Name);
		if (R == 0) return 0;
		return dynamic_cast<T*>(R);
	}
	// Zwraca zasób o podanej nazwie podanego typu.
	// Jeœli nie istnieje lub nie jest tego typu, rzuca wyj¹tek.
	template <typename T>
	T * MustGetResourceEx(const string &Name)
	{
		IResource *R = MustGetResource(Name);
		if (typeid(*R) != typeid(T)) throw Error(Format("res::ResManager::MustGetResourceEx: B³êdny typ zasobu. Nazwa: #, oczekiwany: #, aktualny: #") % Name % typeid(T).name() % typeid(*R).name(), __FILE__, __LINE__);
		return static_cast<T*>(R);
	}
	// Zwraca zasób o podanej nazwie podanego typu lub 0 jeœli nie istnieje.
	// Na docelowy tym rzutuje szybko ale i niebezpiecznie, bez sprawdzania.
	template <typename T>
	T * GetResourceExf(const string &Name)
	{
		IResource *R = GetResource(Name);
		if (R == 0) return 0;
		return static_cast<T*>(R);
	}
	// Zwraca zasób o podanej nazwie podanego typu lub rzuca wyj¹tek jeœli nie istnieje.
	// Na docelowy tym rzutuje szybko ale i niebezpiecznie, bez sprawdzania.
	template <typename T>
	T * MustGetResourceExf(const string &Name)
	{
		return static_cast<T*>(MustGetResource(Name));
	}

	// Zwraca true, jeœli istnieje zasób o podanej nazwie
	bool Exists(const string &Name)
	{
		return GetResource(Name) != 0;
	}
	// Zwrac true, jeœli istnieje zasób o podanej nazwie i jest podanego typu
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
