/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * DateTime - Data i czas
 * Dokumentacja: Patrz plik doc/DateTime.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef COMMON_DATETIME_H_
#define COMMON_DATETIME_H_

#include <time.h>

namespace common
{

enum WEEKDAY
{
	SUN, MON, TUE, WED, THU, FRI, SAT, INV_WEEKDAY
};

enum MONTH
{
	JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC, INV_MONTH
};

// Flagi bitowe - nale¿y po³¹czyæ razem flagê SHORT/LONG z flag¹ LOWERCASE/FIRST_UPPERCASE/UPPERCASE
// lub u¿yæ jednej z ju¿ po³¹czonych.
enum NAME_FORM
{
	NAME_SHORT = 0x00,
	NAME_LONG  = 0x01,
	NAME_LOWERCASE       = 0x00,
	NAME_FIRST_UPPERCASE = 0x10,
	NAME_UPPERCASE       = 0x20,
	NAME_SHORT_LOWERCASE       = 0x00,
	NAME_SHORT_FIRST_UPPERCASE = 0x10,
	NAME_SHORT_UPPERCASE       = 0x20,
	NAME_LONG_LOWERCASE        = 0x01,
	NAME_LONG_FIRST_UPPERCASE  = 0x11,
	NAME_LONG_UPPERCASE        = 0x21,
};

// Domyœlny format dla StrToDate i DateToStr
extern const char * const DEFAULT_FORMAT;

struct DATESPAN;
struct TIMESPAN;
struct TMSTRUCT;
struct DATETIME;


struct DATESPAN
{
private:
	int m_Years;
	int m_Months;
	int m_Weeks;
	int m_Days;

public:
	// Zwraca DATESPAN reprezentuj¹cy n dni
	static DATESPAN Days(int n) { return DATESPAN(0, 0, 0, n); }
	// Zwraca DATESPAN reprezentuj¹cy 1 dzieñ
	static DATESPAN Day() { return Days(1); }
	// Zwraca DATESPAN reprezentuj¹cy n tygodni
	static DATESPAN Weeks(int n) { return DATESPAN(0, 0, n, 0); }
	// Zwraca DATESPAN reprezentuj¹cy 1 tydzieñ
	static DATESPAN Week() { return Weeks(1); }
	// Zwraca DATESPAN reprezentuj¹cy n miesiêcy
	static DATESPAN Months(int n) { return DATESPAN(0, n, 0, 0); }
	// Zwraca DATESPAN reprezentuj¹cy 1 miesi¹c
	static DATESPAN Month() { return Months(1); }
	// Zwraca DATESPAN reprezentuj¹cy n lat
	static DATESPAN Years(int n) { return DATESPAN(n, 0, 0, 0); }
	// Zwraca DATESPAN reprezentuj¹cy 1 rok
	static DATESPAN Year() { return Years(1); }

	DATESPAN(int Years, int Months, int Weeks, int Days) : m_Years(Years), m_Months(Months), m_Weeks(Weeks), m_Days(Days) { }

	DATESPAN & operator += (const DATESPAN &ds) { m_Years += ds.m_Years; m_Months += ds.m_Months; m_Weeks += ds.m_Weeks; m_Days += ds.m_Days; return *this; }
	DATESPAN & operator -= (const DATESPAN &ds) { m_Years -= ds.m_Years; m_Months -= ds.m_Months; m_Weeks -= ds.m_Weeks; m_Days -= ds.m_Days; return *this; }
	DATESPAN & operator *= (int n) { m_Years *= n; m_Months *= n; m_Weeks *= n; m_Days *= n; return *this; }
	DATESPAN operator - () const { return DATESPAN(-m_Years, -m_Months, -m_Weeks, -m_Days); }
	bool operator == (const DATESPAN &ds) const { return m_Years == ds.m_Years && m_Months == ds.m_Months && m_Weeks == ds.m_Weeks && m_Days == ds.m_Days; }
	bool operator != (const DATESPAN &ds) const { return m_Years != ds.m_Years || m_Months != ds.m_Months || m_Weeks != ds.m_Weeks || m_Days != ds.m_Days; }

	// Zwraca liczbê dni (bez tygodni)
	int GetDays() const { return m_Days; }
	// Zwraca liczbê tygodni
	int GetWeeks() const { return m_Weeks; }
	// Zwraca liczbê miesiêcy (bez lat)
	int GetMonths() const { return m_Months; }
	// Zwraca liczbê lat
	int GetYears() const { return m_Years; }
	// Zwraca liczbê dni wliczaj¹c tygodnie (ale nie miesi¹ce ani lata)
	int GetTotalDays() const { return m_Weeks * 7 + m_Days; }

	// Ustawia poszczególne pola
	void Set(int Years, int Months, int Weeks, int Days) { m_Years = Years; m_Months = Months; m_Weeks = Weeks; m_Days = Days; }
	void SetDays(int n) { m_Days = n; }
	void SetWeeks(int n) { m_Weeks = n; }
	void SetMonths(int n) { m_Months = n; }
	void SetYears(int n) { m_Years = n; }
};

inline DATESPAN operator + (const DATESPAN &ds1, const DATESPAN &ds2) { return DATESPAN(ds1.GetYears()+ds2.GetYears(), ds1.GetMonths()+ds2.GetMonths(), ds1.GetWeeks()+ds2.GetWeeks(), ds1.GetDays()+ds2.GetDays()); }
inline DATESPAN operator - (const DATESPAN &ds1, const DATESPAN &ds2) { return DATESPAN(ds1.GetYears()-ds2.GetYears(), ds1.GetMonths()-ds2.GetMonths(), ds1.GetWeeks()-ds2.GetWeeks(), ds1.GetDays()-ds2.GetDays()); }
inline DATESPAN operator * (const DATESPAN &ds, int n) { return DATESPAN(ds.GetYears()*n, ds.GetMonths()*n, ds.GetWeeks()*n, ds.GetDays()*n); }
inline DATESPAN operator * (int n, const DATESPAN &ds) { return DATESPAN(ds.GetYears()*n, ds.GetMonths()*n, ds.GetWeeks()*n, ds.GetDays()*n); }


// Reprezentuje okres czasu, czyli ró¿nicê miêdzy dwoma punktami w czasie
struct TIMESPAN
{
public:
	// Zwraca TIMESPAN dla podanego odcinka czasu
	static TIMESPAN Milliseconds(int8 Milliseconds) { return TIMESPAN(0, 0, 0, Milliseconds); }
	static TIMESPAN Millisecond() { return Milliseconds(1); }
	static TIMESPAN Seconds(int8 Seconds) { return TIMESPAN(0, 0, Seconds); }
	static TIMESPAN Second() { return Seconds(1); }
	static TIMESPAN Minutes(int Minutes) { return TIMESPAN(0, Minutes, 0); }
	static TIMESPAN Minutes() { return Minutes(1); }
	static TIMESPAN Hours(int Hours) { return TIMESPAN(Hours, 0, 0); }
	static TIMESPAN Hour() { return Hours(1); }
	static TIMESPAN Days(int Days) { return Hours(Days * 24); }
	static TIMESPAN Day() { return Days(1); }
	static TIMESPAN Weeks(int Weeks) { return Days(Weeks * 7); }
	static TIMESPAN Week() { return Weeks(1); }

	// W milisekundach
	int8 m_Diff;

	// Tworzy niezainicjalizowan¹
	TIMESPAN() { }
	// Tworzy i inicjalizuje liczb¹ milisekund
	explicit TIMESPAN(int8 Diff) { Set(Diff); }
	// Tworzy i inicjalizuje podanymi wartoœciami (zakres dowolny, niekoniecznie 0..59 ani nic takiego)
	TIMESPAN(int Hours, int Minutes, int8 Seconds, int8 Milliseconds = 0) { Set(Hours, Minutes, Seconds, Milliseconds); }

	void Set(int8 Diff) { m_Diff = Diff; }
	void Set(int Hours, int Minutes, int8 Seconds, int8 Milliseconds = 0);

	bool operator == (const TIMESPAN &t) const { return m_Diff == t.m_Diff; }
	bool operator != (const TIMESPAN &t) const { return m_Diff != t.m_Diff; }
	bool operator <= (const TIMESPAN &t) const { return m_Diff <= t.m_Diff; }
	bool operator >= (const TIMESPAN &t) const { return m_Diff >= t.m_Diff; }
	bool operator <  (const TIMESPAN &t) const { return m_Diff <  t.m_Diff; }
	bool operator >  (const TIMESPAN &t) const { return m_Diff >  t.m_Diff; }
	TIMESPAN operator - () const { return TIMESPAN(-m_Diff); }
	TIMESPAN & operator *= (int n) { m_Diff *= n; return *this; }
	TIMESPAN & operator += (const TIMESPAN &t) { m_Diff += t.m_Diff; return *this; }
	TIMESPAN & operator -= (const TIMESPAN &t) { m_Diff -= t.m_Diff; return *this; }

	// Porównywanie wartoœci bezwzglêdnej
	bool IsShorterThan(const TIMESPAN &t) const { return Abs() < t.Abs(); }
	bool IsLongerThan(const TIMESPAN &t) const { return Abs() > t.Abs(); }

	bool IsNull() const { return m_Diff == 0; }
	bool IsPositive() const { return m_Diff > 0; }
	bool IsNegative() const { return m_Diff < 0; }

	int GetWeeks() const { return GetDays() / 7; }
	int GetDays() const { return GetHours() / 24; }
	int GetHours() const { return GetMinutes() / 60; }
	int GetMinutes() const { return (int)(GetSeconds() / 60); }
	int8 GetSeconds() const { return m_Diff / 1000; }
	int8 GetMilliseconds() const { return m_Diff; }

	// Zwraca nowy TIMESPAN - wartoœæ bezwzglêdna
	TIMESPAN Abs() const { return TIMESPAN(m_Diff < 0 ? -m_Diff : m_Diff); }

	// Przetwarza ten odcinek czasu na ³añcuch
	// - W postaci formalnej: "0:00:00" (H:MM:SS) lub z milisekundami: "0:00:00:0000" (H:MM:SS:IIII)
	void ToString(string *Out, bool ShowMilliseconds = false) const;
	// - W postaci przyjaznej, np. "2:55 h", "3:10 min", "20 s"
	void ToString_Nice(string *Out) const;
};

inline TIMESPAN operator * (const TIMESPAN &t, int n) { return TIMESPAN(t.m_Diff * n); }
inline TIMESPAN operator * (int n, const TIMESPAN &t) { return TIMESPAN(t.m_Diff * n); }
inline TIMESPAN operator + (const TIMESPAN &t1, const TIMESPAN &t2) { return TIMESPAN(t1.m_Diff + t2.m_Diff); }
inline TIMESPAN operator - (const TIMESPAN &t1, const TIMESPAN &t2) { return TIMESPAN(t1.m_Diff - t2.m_Diff); }

// Data i czas jako poszczególne pola struktury, w czasie lokalnym
struct TMSTRUCT
{
private:
	uint2 msec, sec, min, hour, mday;
	MONTH mon;
	int year;

	mutable WEEKDAY wday;

	// Wylicza dzieñ tygodnia na podstawie pozosta³ych pól
	void ComputeWeekDay() const;

public:

	// Tworzy niezainicjalizowany
	TMSTRUCT() : wday(INV_WEEKDAY) { }
	// Tworzy na podstawie podanych danych
	explicit TMSTRUCT(const DATETIME &dt) { Set(dt); }
	explicit TMSTRUCT(const struct tm &tm) { Set(tm); }
	TMSTRUCT(uint2 Day, MONTH Month, int Year, uint2 Hour, uint2 Minute, uint2 Second, uint2 Millisec = 0) { Set(Day, Month, Year, Hour, Minute, Second, Millisec); }
	TMSTRUCT(uint2 Day, MONTH Month, int Year) { Set(Day, Month, Year, 0, 0, 0); }

	void Set(const DATETIME &dt);
	void Set(const struct tm &tm);

	int GetYear() const { return year; }
	MONTH GetMonth() const { return mon; }
	uint2 GetDay() const { return mday; }
	uint2 GetHour() const { return hour; }
	uint2 GetMinute() const { return min; }
	uint2 GetSecond() const { return sec; }
	uint2 GetMillisecond() const { return msec; }

	// W razie potrzeby wylicza i zwraca dzieñ tygodnia
	WEEKDAY GetWeekDay() const
	{
		if (wday == INV_WEEKDAY)
			ComputeWeekDay();
		return (WEEKDAY)wday;
	}

	TMSTRUCT & operator += (const DATESPAN &d) { Add(d);      return *this; }
	TMSTRUCT & operator -= (const DATESPAN &d) { Subtract(d); return *this; }

	void Add(const DATESPAN &d);
	void Subtract(const DATESPAN &d) { Add(-d); }

	// Sprawdza poprawnoœæ
	bool IsValid() const;
	bool IsSameDate(const TMSTRUCT &tm) const;
	bool IsSameTime(const TMSTRUCT &tm) const;

	// Dodaje miesi¹ce pozostawiaj¹c miesi¹c i rok znormalizowane
	// Mo¿e wyjœæ data poza zakresem np. 31 lutego.
	void AddMonths(int MonDiff);
	// Dodaje dni pozostawiaj¹c datê znormalizowan¹
	void AddDays(int DayDiff);

	// Ustawia poszczególne pola
	void Set(uint2 Day, MONTH Month, int Year, uint2 Hour, uint2 Minute, uint2 Second, uint2 Millisec = 0);
	void Set(uint2 Day, MONTH Month, int Year) { Set(Day, Month, Year, 0, 0, 0); }
	void SetYear(int Year) { year = Year; wday = INV_WEEKDAY; }
	void SetMonth(MONTH Month) { mon = Month; wday = INV_WEEKDAY; }
	void SetDay(uint2 Day) { mday = Day; wday = INV_WEEKDAY; }
	void SetHour(uint2 Hour) { hour = Hour; }
	void SetMinute(uint2 Minute) { min = Minute; }
	void SetSecond(uint2 Second) { sec = Second; }
	void SetMillisecond(uint2 Millisecond) { msec = Millisecond; }

	// Zeruje czas, zostawia tylko datê. Czas bêdzie wynosi³ 00:00:00:0
	void ResetTime() { hour = min = sec = msec = 0; }
	
	// Przetwarza na struct tm
	void GetTm(struct tm *Out) const;
};

inline TMSTRUCT operator + (const TMSTRUCT &tm, const DATESPAN &ds) { TMSTRUCT R = tm; R += ds; return R; }
inline TMSTRUCT operator - (const TMSTRUCT &tm, const DATESPAN &ds) { TMSTRUCT R = tm; R -= ds; return R; }

// Reprezentuje konkretny punkt w czasie, czyli datê i czas
// Liczba milisekund od 1 stycznia 1970, w UTC czy tam GMT, cokolwiek to znaczy ;)
struct DATETIME
{
private:
	bool IsInStdRange() const;

public:
	int8 m_Time;

	// Tworzy nowy, niezainicjalizowany
	DATETIME() { }
	// Tworzy nowy na podstawie podanych danych
	explicit DATETIME(time_t Time) { Set(Time); }
	explicit DATETIME(const struct tm &Time) { Set(Time); }
	explicit DATETIME(const TMSTRUCT &Time) { Set(Time); }

	bool operator == (const DATETIME &t) const { return m_Time == t.m_Time; }
	bool operator != (const DATETIME &t) const { return m_Time != t.m_Time; }
	bool operator <= (const DATETIME &t) const { return m_Time <= t.m_Time; }
	bool operator >= (const DATETIME &t) const { return m_Time >= t.m_Time; }
	bool operator <  (const DATETIME &t) const { return m_Time <  t.m_Time; }
	bool operator >  (const DATETIME &t) const { return m_Time >  t.m_Time; }

	DATETIME & operator += (const TIMESPAN &t) { Add(t);      return *this; }
	DATETIME & operator -= (const TIMESPAN &t) { Subtract(t); return *this; }

	// Ustawia ca³¹ wartoœæ na podan¹ datê i czas
	void SetValue(int8 Value) { m_Time = Value; }
	void Set(time_t Time);
	void Set(const struct tm &Time);
	void Set(const TMSTRUCT &Time);
	void SetJDN(double JDN);
	// Ustawia poszczególne pola
	void SetMillisecond(uint Millisecond) { m_Time -= m_Time % 1000; m_Time += Millisecond; }
	// Zwiêksza wartoœæ
	void Add(int8 Milliseconds) { m_Time += Milliseconds; }
	void Add(const TIMESPAN &t) { m_Time += t.m_Diff; }
	// Zmniejsza wartoœæ
	void Subtract(int8 Milliseconds) { m_Time -= Milliseconds; }
	void Subtract(const TIMESPAN &t) { m_Time -= t.m_Diff; }

	bool IsStrictlyBetween(const DATETIME &dt1, const DATETIME &dt2) const { return (m_Time > dt1.m_Time) && (m_Time < dt2.m_Time); }
	bool IsBetween(const DATETIME &dt1, const DATETIME &dt2) const { return (m_Time >= dt1.m_Time) && (m_Time <= dt2.m_Time); }
	bool IsEqualUpTo(const DATETIME &dt, const TIMESPAN &ts) const;

	// Przerabia na time_t
	// Zwraca (time_t)-1 jeœli poza zakresem
	time_t GetTicks() const;
};

inline TIMESPAN operator - (const DATETIME &dt1, const DATETIME &dt2) { return TIMESPAN(dt1.m_Time - dt2.m_Time); }
inline DATETIME operator + (const DATETIME &dt, const TIMESPAN &ts) { DATETIME R = dt; R += ts; return R; }
inline DATETIME operator - (const DATETIME &dt, const TIMESPAN &ts) { DATETIME R = dt; R -= ts; return R; }

// Zwraca true, jeœli podany rok jest przestêpny
inline bool IsLeapYear(int Year) { return (Year % 4 == 0) && ((Year % 100 != 0) || (Year % 400 == 0)); }
// Zwraca liczbê dni w podanym roku (365 lub 366)
inline uint GetNumOfDaysInYear(int Year) { return IsLeapYear(Year) ? 366 : 365; }
// Zwraca liczbê dni w podanym miesi¹cu podanego roku
uint GetNumOfDaysInMonth(int Year, uint Month);
// Zwraca wiek podanego roku
inline int GetCentury(int Year) { return (Year > 0) ? (Year / 100) : (Year / 100 - 1); }

// Zwraca bie¿¹c¹ datê i czas jako time_t
time_t GetTimeNow();
// Zwraca bie¿¹c¹ datê i czas jako struct tm
void GetTmNow(struct tm *Out);
// Zwraca bie¿¹c¹ datê i czas, dok³adnoœæ co do sekund
DATETIME Now();
// Zwraca bie¿¹c¹ datê i czas, dok³adnoœæ co do milisekund
DATETIME UNow();

// Zwraca nazwê dnia tygodnia w podanej formie
const char * GetWeekDayName(WEEKDAY WeekDay, NAME_FORM Form);
// Zwraca nazwê miesi¹ca w podanej formie
const char * GetMonthName(MONTH Month, NAME_FORM Form);
// Konwertuje datê na ³añcuch
void DateToStr(string *Out, const TMSTRUCT &tm, const string &Format);

// Konwertuje ³añcuch na dzieñ tygodnia. £añcuch mo¿e byæ w dowolnej formie.
// Jeœli siê nie uda, zwraca INV_WEEKDAY.
WEEKDAY StrToWeekDay(const string &s);
// Konwertuje ³añcuch na miesi¹c. £añcuch mo¿e byæ w dowolnej formie.
// Jeœli siê nie uda, zwraca INV_WEEKDAY.
MONTH StrToMonth(const string &s);
// Konwertuje ³añcuch na datê
// Jeœli siê nie uda, zwraca false.
// ¯eby to mia³o sens, musz¹ siê znaleŸæ tam co najmniej rok, miesi¹c i dzieñ.
bool StrToDate(TMSTRUCT *Out, const string &s, const string &Format);

} // namespace common

template <>
struct SthToStr_obj<common::TMSTRUCT>
{
	void operator () (string *Str, const common::TMSTRUCT &Sth)
	{
		common::DateToStr(Str, Sth, common::DEFAULT_FORMAT);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::TMSTRUCT>
{
	bool operator () (common::TMSTRUCT *Sth, const string &Str)
	{
		return common::StrToDate(Sth, Str, common::DEFAULT_FORMAT);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::DATETIME>
{
	void operator () (string *Str, const common::DATETIME &Sth)
	{
		SthToStr(Str, Sth.m_Time);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::TIMESPAN>
{
	void operator () (string *Str, const common::TIMESPAN &Sth)
	{
		SthToStr(Str, Sth.m_Diff);
	}
	static inline bool IsSupported() { return true; }
};

#endif
