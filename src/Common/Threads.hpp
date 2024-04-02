/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Threads - Biblioteka do wielow�tkowo�ci i synchronizacji
 * Dokumentacja: Patrz plik doc/Threads.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_THREADS_H_
#define COMMON_THREADS_H_

namespace common
{

// Deklaracje zapowiadaj�ce
class Thread_pimpl;
class Mutex_pimpl;
class Semaphore_pimpl;
class Cond_pimpl;
class Barrier_pimpl;
class Event_pimpl;

/*
Klasa bazowa w�tku.
- Odziedzicz po tej klasie, �eby zdefiniowa� w�asny typ w�tku. Nadpisa� metod� Run.
- Obiekt tej klasy reprezentuje pojedynczy w�tek.
- W�tek mo�na odpali� jeden raz - Start.
- Przed zniszczeniem obiektu na odpalony w�tek trzeba poczeka� - Join.
- Obiekt mo�na tworzy� na stosie lub na stercie - oboj�tne.
- Obiekt, je�li alokowany new, trzeba zniszczy� - sam si� nie zniszczy.
*/
class Thread
{
	DECLARE_NO_COPY_CLASS(Thread)
	friend class Thread_pimpl;

private:
	scoped_ptr<Thread_pimpl> pimpl;
	
protected:
	// Napisz swoj� wersj� z kodem, kt�ry ma si� wykona� w osobnym w�tku.
	virtual void Run() = 0;

	// Ko�czy dzia�anie w�tku
	// - Wywo�ywa� z funkcji Run.
	// - Lepiej jednak normalnie zako�czy� funkcj� Run.
	//   Stosowanie Exit nie ma sensu i jest niebezpieczne dla zasob�w.
	void Exit();
	// Oddaje sterowanie innemu w�tkowi
	// - Wywo�ywa� z funkcji Run.
	// - Nazwa taka dziwna, bo jaki� cham zdefiniowa� makro o nazwie Yield!
	void Yield_();

public:
	Thread();
	virtual ~Thread();

	// Uruchamiamia w�tek.
	void Start();
	// Czeka na zako�czenie w�tku.
	// - Je�li w�tek ju� si� zako�czy�, wychodzi od razu.
	// - Je�li w�tek jeszcze nie by� odpalony albo ju� raz by�o testowane Join, nic si� nie dzieje.
	void Join();

	// Brutalnie zabija w�tek - nie u�ywa�!
	void Kill();

// Rzeczy kt�re nie wiem jak zrealizowa� w pthreads - dzia�aj� tylko w Windows
#ifdef WIN32
	// Zwraca true, je�li w�tek jest w tej chwili uruchomiony i naprawd� jeszcze dzia�a
	bool IsRunning();
	// Zatrzymuje w�tek, wznawia w�tek (posiada wewn�trzny licznik zatrzyma�)
	// - Dzia�a tylko mi�dzy Start a Join.
	// - Nie ma sensu tego u�ywa�.
	void Pause();
	void Resume();

	// Prioritytet w�tku jest liczb� ca�kowit�. Te sta�e to niekt�re warto�ci. Mog� by� te� inne.
	static const int PRIORITY_IDLE;
	static const int PRIORITY_VERY_LOW;
	static const int PRIORITY_LOW;
	static const int PRIORITY_DEFAULT;
	static const int PRIORITY_HIGH;
	static const int PRIORITY_VERY_HIGH;
	static const int PRIORITY_REALTIME;
	// Ustawia lub zwraca priorytet w�tku
	// - Dzia�aj� tylko mi�dzy Start a Join.
	void SetPriority(int Priority);
	int GetPriority();
#endif
};

/*
Muteks
Obiekt do wyznaczania sekcji krytycznej na wzajemnie wykluczaj�c� si� wy��czno��
jednego w�tku.
*/
class Mutex
{
	DECLARE_NO_COPY_CLASS(Mutex)
	friend class Mutex_pimpl;
	friend class Cond;

private:
	scoped_ptr<Mutex_pimpl> pimpl;

public:
	// Flagi bitowe do konstruktora. ��czy� operatorem |
	// - Podaj, je�li b�dziesz robi� w ramach jednego w�tku zagnie�d�one blokowania.
	static const uint FLAG_RECURSIVE    = 0x01;
	// - Podaj, je�li b�dziesz u�ywa� metody TimeoutLock.
	static const uint FLAG_WAIT_TIMEOUT = 0x02;

	Mutex(uint Flag);
	~Mutex();

	// Blokuje muteks.
	void Lock();
	// Odblokowuje muteks.
	void Unlock();
	// Pr�buje wej�� do sekcji krytycznej.
	// - Je�li si� uda, wchodzi blokuj�c muteks i zwraca true.
	// - Je�li si� nie uda bo muteks jest zablokowany przez inny w�tek, nie blokuje sterowania tylko zawraca false.
	bool TryLock();
	// Pr�buje wej�� do sekcji krytycznej czekaj�c co najwy�ej podany czas.
	// - Je�li si� uda, wchodzi blokuj�c muteks i zwraca true.
	// - Je�li si� nie uda przez podany czas, bo muteks jest zablokowany przez inny w�tek, zwraca false.
	bool TimeoutLock(uint Milliseconds);
};

/*
Klasa pomagaj�ca blokowa� muteks
- W konstruktorze blokuje, w destruktorze odblokowuje.
- U�ywaj jej zamiast metod Mutex::Lock i Mutex::Unlock kiedy tylko mo�na.
*/
class MutexLock
{
private:
	Mutex *m_Mutex;
	DECLARE_NO_COPY_CLASS(MutexLock)

public:
	MutexLock(Mutex *m) : m_Mutex(m) { m->Lock(); }
	~MutexLock() { m_Mutex->Unlock(); }
};

/*
Dla jeszcze wi�szej wygody, zamiast tworzy� obiekt klasy MutexLock wystarczy na
pocz�tku funkcji czy dowolnego bloku { } postawi� to makro (M ma by� typu
Mutex*).
*/
#define MUTEX_LOCK(M) MutexLock __mutex_lock_obj(M);

/*
Semafor zliczaj�cy
- Posiada wewn�trznie nieujemn� warto�� ca�kowit�, kt�r� mo�na sobie wyobrazi�
  jako liczb� dost�pnych zasob�w. Nie ma do niej dost�pu bezpo�rednio.
- Warto�� maksymalna tej liczby nie jest okre�lona, jest jaka� du�a.
*/
class Semaphore
{
	DECLARE_NO_COPY_CLASS(Semaphore)

private:
	scoped_ptr<Semaphore_pimpl> pimpl;

public:
	Semaphore(uint InitialValue);
	~Semaphore();

	// Opuszcza semafor - zmniejsza jego warto�� o jeden.
	// - Je�li semafor jest zerowy, czeka a� kto� go podniesie zanim go opu�ci i zwr�ci sterowanie.
	void P();
	// Pr�buje opu�ci� semafor - zmniejszy� jego warto�� o jeden.
	// - Je�li si� uda, zmniejsza i zwraca true.
	// - Je�li si� nie uda bo warto�� wynosi 0, nie blokuje sterowania tylko zawraca false.
	bool TryP();
	// Pr�buje opu�ci� semafor czekaj�c co najwy�ej podany czas.
	// - Je�li si� uda, zmniejsza i zwraca true.
	// - Je�li si� nie uda przez podany czas, bo warto�� wynosi 0, zwraca false.
	bool TimeoutP(uint Milliseconds);

	// Podnosi semafor zwi�kszaj�c go o 1. Nigdy nie blokuje.
	void V();
	// Podnosi semafor zwi�kszaj�c go o ReleaseCount. Nigdy nie blokuje.
	void V(uint ReleaseCount);
};

/*
Zmienna warunkowa
Nie pami�ta �adnego swojego stanu. Pozwala za to zawiesi� w�tek �eby na niej
poczeka� (Wait) oraz wznowi� niezdefiniowany jeden (Signal) lub wszystkie
(Broadcast) czekaj�ce na niej w�tki. Sensu nabiera dopiero kiedy zrozumie si�
spos�b jej u�ycia w kontek�cie wzorca monitora do sprawdzania warunku.
*/
class Cond
{
	DECLARE_NO_COPY_CLASS(Cond)

private:
	scoped_ptr<Cond_pimpl> pimpl;

public:
	Cond();
	~Cond();

	// Odblokowuje muteks, zawiesza w�tek czekaj�c na sygna�, po doczekaniu si� blokuje muteks z powrotem.
	// - Muteks m musi by� zablokowany w chwili tego wywo�ania.
	void Wait(Mutex *m);
	// Odblokowuje muteks, zawiesza w�tek czekaj�c na sygna� przez co najwy�ej podany czas,
	// po doczekaniu si� blokuje muteks z powrotem.
	// - Je�li w tym czasie nast�pi�o przerwanie czekania przez sygna�, zwraca true.
	// - Muteks m musi by� zablokowany w chwili tego wywo�ania.
	bool TimeoutWait(Mutex *m, uint Milliseconds);

	// Odwiesza przypadkowy jeden czekaj�cy w�tek.
	// - Je�li �aden w�tek nie czeka, nie robi nic.
	// - Nie jest wymagane �eby jaki� muteks musia� by� zablokowany w chwili tego wywo�ania.
	void Signal();
	// Odwiesza wszystkie czekaj�ce w�tki.
	// - Je�li �aden w�tek nie czeka, nie robi nic.
	// - Nie jest wymagane �eby jaki� muteks musia� by� zablokowany w chwili tego wywo�ania.
	void Broadcast();
};

/*
Bariera
W�tki kt�re wywo�uj� Wait blokuj� si� czekaj�c, a� si� ich uzbiera tyle
czekaj�cych, ile wynosi przekazany do konstruktora NumThreads. Kiedy ostatni z
nich wywo�a Wait, wszystkie ruszaj� dalej.
*/
class Barrier
{
	DECLARE_NO_COPY_CLASS(Barrier)

private:
	scoped_ptr<Barrier_pimpl> pimpl;

public:
	Barrier(uint NumThreads);
	~Barrier();

	void Wait();
};

/*
Zdarzenie
Ten obiekt, wzorowany na WinAPI. Jest troch� podobny do zmiennej warunkowej,
ale ma sw�j stan - true lub false.
*/
class Event
{
	DECLARE_NO_COPY_CLASS(Event);

private:
	scoped_ptr<Event_pimpl> pimpl;

public:
	enum TYPE
	{
		// Zdarzenie samo przywr�ci si� do false, kiedy jaki� jeden w�tek doczeka si� true w wywo�aniu Wait.
		// SetEvent wznawia jeden w�tek czekajacy na Wait.
		TYPE_AUTO_RESET,
		// Zdarzenie samo nie przywr�ci si� do false.
		// SetEvent wznawia wszystkie w�tki czekaj�ce na Wait.
		TYPE_MANUAL_RESET,
	};

	Event(bool InitialState, TYPE Type);
	~Event();

	// Zmienia stan na true.
	// - TYPE_AUTO_RESET: Je�li jakie� w�tki czeka�y, wznawia jeden i przestawia z powrotem na false.
	// - TYPE_MANUAL_RESET: Je�li jakie� w�tki czeka�y, wznawia je wszystkie.
	void Set();
	// Zmienia stan na false.
	void Reset();

	// Czeka, a� stan b�dzie true.
	// - Je�li ju� jest, zwraca sterowanie od razu.
	// - TYPE_AUTO_RESET: Kiedy si� doczeka, przestawia go z powrotem na false.
	void Wait();
	// Zwraca stan
	// - TYPE_AUTO_RESET: Je�li by� true, przestawia go z powrotem na false.
	bool Test();
	// Czeka, a� stan b�dzie true przez co najwy�ej podany czas.
	// - Je�li jest albo si� doczeka a� b�dzie, zwraca true.
	// - Je�li si� nie doczeka przez podany czas (stan jest ca�y czas false), zwraca false.
	// - TYPE_AUTO_RESET: Kiedy si� doczeka na stan true, przestawia go z powrotem na false.
	bool TimeoutWait(uint Milliseconds);
};

} // namespace common

#endif
