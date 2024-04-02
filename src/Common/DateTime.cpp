/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * DateTime - Data i czas
 * Dokumentacja: Patrz plik doc/DateTime.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include "DateTime.hpp"
#ifdef WIN32
	#include <windows.h>
#else
	#include <sys/time.h> // dla gettimeofday
#endif

namespace common
{

// this is the integral part of JDN of the midnight of Jan 1, 1970 (i.e. JDN(Jan 1, 1970) = 2440587.5)
const int EPOCH_JDN = 2440587;
const int MONTHS_IN_YEAR = 12;
const int SEC_PER_MIN = 60;
const int MIN_PER_HOUR = 60;
const int HOURS_PER_DAY = 24;
const int SECONDS_PER_DAY = 86400;
const int DAYS_PER_WEEK = 7;
const int MILLISECONDS_PER_DAY = 86400000;
const uint SECONDS_PER_HOUR = 3600;
// this constant is used to transform a time_t value to the internal
// representation, as time_t is in seconds and we use milliseconds it's
// fixed to 1000
const int TIME_T_FACTOR = 1000;

// the date of JDN -0.5 (as we don't work with fractional parts, this is the
// reference date for us) is Nov 24, 4714BC
const int JDN_0_YEAR = -4713;
const int JDN_0_MONTH = NOV;
const int JDN_0_DAY = 24;

// the constants used for JDN calculations
const int JDN_OFFSET         = 32046;
const int DAYS_PER_5_MONTHS  = 153;
const int DAYS_PER_4_YEARS   = 1461;
const int DAYS_PER_400_YEARS = 146097;

const char * const DEFAULT_FORMAT = "Y-N-D H:M:S";

const char * const WEEKDAY_NAMES_SHORT_LOWERCASE[] = {
	"ndz", "pn", "wt", "œr", "czw", "pt", "sob", "err"
};
const char * const WEEKDAY_NAMES_SHORT_FIRST_UPPERCASE[] = {
	"Ndz", "Pn", "Wt", "Œr", "Czw", "Pt", "Sob", "Err"
};
const char * const WEEKDAY_NAMES_SHORT_UPPERCASE[] = {
	"NDZ", "PN", "WT", "ŒR", "CZW", "PT", "SOB", "ERR"
};
const char * const WEEKDAY_NAMES_LONG_LOWERCASE[] = {
	"niedziela", "poniedzia³ek", "wtorek", "œroda", "czwartek", "pi¹tek", "sobota", "b³¹d"
};
const char * const WEEKDAY_NAMES_LONG_FIRST_UPPERCASE[] = {
	"Niedziela", "Poniedzia³ek", "Wtorek", "Œroda", "Czwartek", "Pi¹tek", "Sobota", "B³¹d"
};
const char * const WEEKDAY_NAMES_LONG_UPPERCASE[] = {
	"NIEDZIELA", "PONIEDZIA£EK", "WTOREK", "ŒRODA", "CZWARTEK", "PI¥TEK", "SOBOTA", "B£¥D"
};

const char * const MONTH_NAMES_SHORT_LOWERCASE[] = {
	"sty", "lut", "mar", "kwi", "maj", "cze", "lip", "sie", "wrz", "paz", "lis", "gru", "err"
};
const char * const MONTH_NAMES_SHORT_FIRST_UPPERCASE[] = {
	"Sty", "Lut", "Mar", "Kwi", "Maj", "Cze", "Lip", "Sie", "Wrz", "Paz", "Lis", "Gru", "err"
};
const char * const MONTH_NAMES_SHORT_UPPERCASE[] = {
	"STY", "LUT", "MAR", "KWI", "MAJ", "CZE", "LIP", "SIE", "WRZ", "PAZ", "LIS", "GRU", "ERR"
};
const char * const MONTH_NAMES_LONG_LOWERCASE[] = {
	"styczeñ", "luty", "marzec", "kwiecieñ", "maj", "czerwiec", "lipiec", "sierpieñ", "wrzesieñ", "paŸdziernik", "listopad", "grudzieñ", "b³¹d"
};
const char * const MONTH_NAMES_LONG_FIRST_UPPERCASE[] = {
	"Styczeñ", "Luty", "Marzec", "Kwiecieñ", "Maj", "Czerwiec", "Lipiec", "Sierpieñ", "Wrzesieñ", "PaŸdziernik", "Listopad", "Grudzieñ", "B³¹d"
};
const char * const MONTH_NAMES_LONG_UPPERCASE[] = {
	"STYCZEÑ", "LUTY", "MARZEC", "KWIECIEÑ", "MAJ", "CZERWIEC", "LIPIEC", "SIERPIEÑ", "WRZESIEÑ", "PADZIERNIK", "LISTOPAD", "GRUDZIEÑ", "B£¥D"
};

// Liczba dni w poszczególnych miesi¹cach lat 1. zwyk³ych 2. przestêpnych
const uint DAYS_IN_MONTH[2][MONTHS_IN_YEAR] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

const uint2 CUMULATIVE_DAYS[2][MONTHS_IN_YEAR] = {
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
};

// [wewnêtrzna]
// return the integral part of the JDN for the midnight of the given date (to
// get the real JDN you need to add 0.5, this is, in fact, JDN of the
// noon of the previous day)
int GetTruncatedJDN(uint2 day, MONTH mon, int year)
{
    // CREDIT: code below is by Scott E. Lee (but bugs are mine)

    // check the date validity
    assert(
      ((year > JDN_0_YEAR) ||
      ((year == JDN_0_YEAR) && (mon > JDN_0_MONTH)) ||
      ((year == JDN_0_YEAR) && (mon == JDN_0_MONTH) && (day >= JDN_0_DAY))) &&
      "date out of range - can't convert to JDN");

    // make the year positive to avoid problems with negative numbers division
    year += 4800;

    // months are counted from March here
    int month;
    if ( mon >= MAR )
    {
        month = mon - 2;
    }
    else
    {
        month = mon + 10;
        year--;
    }

    // now we can simply add all the contributions together
    return ((year / 100) * DAYS_PER_400_YEARS) / 4
            + ((year % 100) * DAYS_PER_4_YEARS) / 4
            + (month * DAYS_PER_5_MONTHS + 2) / 5
            + day
            - JDN_OFFSET;
}

// [wewnêtrzna]
// Napisana tak ¿eby by³a bezpieczna w¹tkowo, nie to co standardowa localtime.
void MyLocalTime(struct tm *OutTm, time_t Time)
{
#ifdef WIN32
	int R = localtime_s(OutTm, &Time);
	assert( R == 0 );

#else
	localtime_r(&Time, OutTm);

#endif
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// TIMESPAN

void TIMESPAN::Set(int Hours, int Minutes, int8 Seconds, int8 Milliseconds)
{
	// assign first to avoid precision loss
	m_Diff = Hours;
	m_Diff *= 60;
	m_Diff += Minutes;
	m_Diff *= 60;
	m_Diff += Seconds;
	m_Diff *= 1000;
	m_Diff += Milliseconds;
}

void TIMESPAN::ToString(string *Out, bool ShowMilliseconds) const
{
	Out->clear();

	if (ShowMilliseconds)
		Out->reserve(13);
	else
		Out->reserve(8);

	uint8 Milliseconds;

	if (IsNegative())
	{
		*Out += '-';
		Milliseconds = (uint8)(-GetMilliseconds());
	}
	else
		Milliseconds = (uint8)GetMilliseconds();

	uint8 Seconds = Milliseconds / 1000;
	Milliseconds -= Seconds * 1000;
	uint Minutes = (uint)(Seconds / 60);
	Seconds -= (uint8)Minutes * 60;
	uint Hours = Minutes / 60;
	Minutes -= Hours * 60;

	string s;
	UintToStr(&s, Hours);
	Out->append(s);
	*Out += ':';
	UintToStr2(&s, Minutes, 2);
	Out->append(s);
	*Out += ':';
	UintToStr2(&s, Seconds, 2);
	Out->append(s);
	if (ShowMilliseconds)
	{
		*Out += ':';
		UintToStr2(&s, Milliseconds, 3);
		Out->append(s);
	}
}

void TIMESPAN::ToString_Nice(string *Out) const
{
	Out->clear();

	uint8 Seconds;

	if (IsNegative())
	{
		*Out += '-';
		Seconds = (uint8)(-GetSeconds());
	}
	else
		Seconds = (uint8)GetSeconds();

	// "# s"
	if (Seconds < 60)
	{
		UintToStr(Out, Seconds);
		Out->append(" s");
	}
	// "#:## min"
	else if (Seconds < SECONDS_PER_HOUR)
	{
		uint Minutes = (uint)(Seconds / 60);
		Seconds -= (uint8)Minutes * 60;
		UintToStr(Out, Minutes);
		*Out += ':';
		string s;
		UintToStr2(&s, Seconds, 2);
		Out->append(s);
		Out->append(" min");
	}
	// "#:## h"
	else
	{
		uint Minutes = (uint)(Seconds / 60);
		uint Hours = Minutes / 60;
		Minutes -= Hours * 60;
		UintToStr(Out, Hours);
		*Out += ':';
		string s;
		UintToStr2(&s, Minutes, 2);
		Out->append(s);
		Out->append(" h");
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// TMSTRUCT

void TMSTRUCT::ComputeWeekDay() const
{
	// compute the week day from day/month/year: we use the dumbest algorithm
	// possible: just compute our JDN and then use the (simple to derive)
	// formula: weekday = (JDN + 1.5) % 7
	int d = (GetTruncatedJDN(mday, mon, year) + 2) % 7;
	wday = (WEEKDAY)d;
}

void TMSTRUCT::Set(uint2 Day, MONTH Month, int Year, uint2 Hour, uint2 Minute, uint2 Second, uint2 Millisec)
{
	assert(Month < INV_MONTH);
	assert(Day > 0 && Day <= GetNumOfDaysInMonth(Year, Month));
	assert(Hour < 64);
	assert(Minute < 60);
	assert(Second < 62);
	assert(Millisec < 1000);

    static const int YEAR_MIN_IN_RANGE = 1970;
    static const int YEAR_MAX_IN_RANGE = 2037;

	// Jeœli mieœci siê w granicach czasu Uniksa, u¿yj jego funkcji - szybsza
	if (Year >= YEAR_MIN_IN_RANGE && Year <= YEAR_MAX_IN_RANGE)
	{
		struct tm tm;
		tm.tm_year = Year - 1900;
		tm.tm_mon = Month;
		tm.tm_mday = Day;
		tm.tm_hour = Hour;
		tm.tm_min = Minute;
		tm.tm_sec = Second;
		tm.tm_isdst = -1; // na pewno? w wx by³o tutaj "mktime() will guess it"

		Set(tm);
		SetMillisecond(Millisec);
		wday = INV_WEEKDAY;
	}
	else
	{
		assert("Data poza zakresem Uniksa");

/*		// get the JDN for the midnight of this day
		m_Time = GetTruncatedJDN(Day, Month, Year);
		m_Time -= EPOCH_JDN;
		m_Time *= SECONDS_PER_DAY * TIME_T_FACTOR;

		// JDN corresponds to GMT, we take localtime
//        Add(wxTimeSpan(hour, minute, second + GetTimeZone(), millisec));
		Add(TIMESPAN(Hour, Minute, Second, Millisec));*/
	}
}

void TMSTRUCT::Set(const DATETIME &dt)
{
	time_t time = dt.GetTicks();
	assert(time != (time_t)-1);

	struct tm tm2;
	MyLocalTime(&tm2, time);

	sec = (uint2)tm2.tm_sec;
	min = (uint2)tm2.tm_min;
	hour = (uint2)tm2.tm_hour;
	mday = (uint2)tm2.tm_mday;
	mon = (MONTH)tm2.tm_mon;
	year = tm2.tm_year + 1900;
	wday = (WEEKDAY)tm2.tm_wday;

	long TimeOnly = (long)(dt.m_Time % MILLISECONDS_PER_DAY);
	msec = (uint2)(TimeOnly % 1000);
}

void TMSTRUCT::Set(const struct tm &tm)
{
	msec = 0;
	sec = (uint2)tm.tm_sec;
	min = (uint2)tm.tm_min;
	hour = (uint2)tm.tm_hour;
	mday = (uint2)tm.tm_mday;
	mon = (MONTH)tm.tm_mon;
	year = tm.tm_year + 1900;
	wday = (WEEKDAY)tm.tm_wday;
}

bool TMSTRUCT::IsValid() const
{
	// we allow for the leap seconds, although we don't use them (yet)
	return (mon < INV_MONTH) &&
		(mday <= GetNumOfDaysInMonth(year, mon)) &&
		(hour < 24) && (min < 60) && (sec < 62) && (msec < 1000);
}

bool TMSTRUCT::IsSameDate(const TMSTRUCT &tm) const
{
	return (year == tm.year && mon == tm.mon && mday == tm.mday);
}

bool TMSTRUCT::IsSameTime(const TMSTRUCT &tm) const
{
	return (hour == tm.hour && min == tm.min && sec == tm.sec && msec == tm.msec);
}

void TMSTRUCT::AddMonths(int monDiff)
{
	if (monDiff == 0) return;

	// normalize the months field
	while ( monDiff < -mon )
	{
		year--;
		monDiff += MONTHS_IN_YEAR;
	}

	while ( monDiff + mon >= MONTHS_IN_YEAR )
	{
		year++;
		monDiff -= MONTHS_IN_YEAR;
	}

	mon = (MONTH)(mon + monDiff);
	assert(mon >= 0 && mon < MONTHS_IN_YEAR);

	wday = INV_WEEKDAY;
}

void TMSTRUCT::AddDays(int dayDiff)
{
	if (dayDiff == 0) return;

	// normalize the days field
	while ( dayDiff + mday < 1 )
	{
		AddMonths(-1);
		dayDiff += GetNumOfDaysInMonth(year, mon);
	}

	mday = (uint2)( mday + dayDiff );
	while ( mday > GetNumOfDaysInMonth(year, mon) )
	{
		mday -= GetNumOfDaysInMonth(year, mon);
		AddMonths(1);
	}

	assert( mday > 0 && mday <= GetNumOfDaysInMonth(year, mon));

	wday = INV_WEEKDAY;
}

void TMSTRUCT::GetTm(struct tm *Out) const
{
	Out->tm_year = year - 1900;
	Out->tm_mon = mon;
	Out->tm_mday = mday;
	Out->tm_hour = hour;
	Out->tm_min = min;
	Out->tm_sec = sec;
	Out->tm_isdst = -1; // na pewno? w wx by³o tutaj "mktime() will guess it"
}

void TMSTRUCT::Add(const DATESPAN &d)
{
	year += d.GetYears();
	AddMonths(d.GetMonths());
	// Jeœli dzieñ poza zakresem, ogranicz do ostatniego dnia danego miesi¹ca
	if (mday > GetNumOfDaysInMonth(year, mon))
		mday = GetNumOfDaysInMonth(year, mon);
	AddDays(d.GetTotalDays());
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// DATETIME

bool DATETIME::IsInStdRange() const
{
	return m_Time >= 0l && (m_Time / TIME_T_FACTOR) < LONG_MAX;
}


void DATETIME::Set(time_t Time)
{
	//m_Time = Time - TIME_BASE_OFFSET;
	m_Time = Time; // Bo na naszych systemach jest liczone te¿ od 1970, czyli OFFSET=0
	m_Time *= TIME_T_FACTOR;
}

void DATETIME::Set(const struct tm &Time)
{
	struct tm tm2(Time);
	time_t timet = mktime(&tm2);
	if (timet == (time_t)-1)
		assert(0 && "mktime() failed.");
	Set(timet);
}

void DATETIME::Set(const TMSTRUCT &Time)
{
	struct tm tm;
	Time.GetTm(&tm);
	Set(tm);
}

void DATETIME::SetJDN(double JDN)
{
	// so that m_time will be 0 for the midnight of Jan 1, 1970 which is jdn EPOCH_JDN + 0.5
	JDN -= EPOCH_JDN + 0.5;
	m_Time = (int8)(JDN * MILLISECONDS_PER_DAY);
}

bool DATETIME::IsEqualUpTo(const DATETIME &dt, const TIMESPAN &ts) const
{
	DATETIME dt1 = dt - ts, dt2 = dt + ts;
	return IsBetween(dt1, dt2);
}

time_t DATETIME::GetTicks() const
{
	if (!IsInStdRange())
		return (time_t)-1;
//	return (time_t)(m_Time / TIME_T_FACTOR) + TIME_BASE_OFFSET;
	return (time_t)(m_Time / TIME_T_FACTOR);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje globalne

const char * GetWeekDayName(WEEKDAY WeekDay, NAME_FORM Form)
{
	if (Form & NAME_LONG)
	{
		if (Form & NAME_UPPERCASE)
		{
			if (WeekDay >= INV_WEEKDAY)
				return WEEKDAY_NAMES_LONG_UPPERCASE[INV_WEEKDAY];
			return WEEKDAY_NAMES_LONG_UPPERCASE[WeekDay];
		}
		else if (Form & NAME_FIRST_UPPERCASE)
		{
			if (WeekDay >= INV_WEEKDAY)
				return WEEKDAY_NAMES_LONG_FIRST_UPPERCASE[INV_WEEKDAY];
			return WEEKDAY_NAMES_LONG_FIRST_UPPERCASE[WeekDay];
		}
		else
		{
			if (WeekDay >= INV_WEEKDAY)
				return WEEKDAY_NAMES_LONG_LOWERCASE[INV_WEEKDAY];
			return WEEKDAY_NAMES_LONG_LOWERCASE[WeekDay];
		}
	}
	else
	{
		if (Form & NAME_UPPERCASE)
		{
			if (WeekDay >= INV_WEEKDAY)
				return WEEKDAY_NAMES_SHORT_UPPERCASE[INV_WEEKDAY];
			return WEEKDAY_NAMES_SHORT_UPPERCASE[WeekDay];
		}
		else if (Form & NAME_FIRST_UPPERCASE)
		{
			if (WeekDay >= INV_WEEKDAY)
				return WEEKDAY_NAMES_SHORT_FIRST_UPPERCASE[INV_WEEKDAY];
			return WEEKDAY_NAMES_SHORT_FIRST_UPPERCASE[WeekDay];
		}
		else
		{
			if (WeekDay >= INV_WEEKDAY)
				return WEEKDAY_NAMES_SHORT_LOWERCASE[INV_WEEKDAY];
			return WEEKDAY_NAMES_SHORT_LOWERCASE[WeekDay];
		}
	}
}

const char * GetMonthName(MONTH Month, NAME_FORM Form)
{
	if (Form & NAME_LONG)
	{
		if (Form & NAME_UPPERCASE)
		{
			if (Month >= INV_MONTH)
				return MONTH_NAMES_LONG_UPPERCASE[INV_MONTH];
			return MONTH_NAMES_LONG_UPPERCASE[Month];
		}
		else if (Form & NAME_FIRST_UPPERCASE)
		{
			if (Month >= INV_MONTH)
				return MONTH_NAMES_LONG_FIRST_UPPERCASE[INV_MONTH];
			return MONTH_NAMES_LONG_FIRST_UPPERCASE[Month];
		}
		else
		{
			if (Month >= INV_MONTH)
				return MONTH_NAMES_LONG_LOWERCASE[INV_MONTH];
			return MONTH_NAMES_LONG_LOWERCASE[Month];
		}
	}
	else
	{
		if (Form & NAME_UPPERCASE)
		{
			if (Month >= INV_MONTH)
				return MONTH_NAMES_SHORT_UPPERCASE[INV_MONTH];
			return MONTH_NAMES_SHORT_UPPERCASE[Month];
		}
		else if (Form & NAME_FIRST_UPPERCASE)
		{
			if (Month >= INV_MONTH)
				return MONTH_NAMES_SHORT_FIRST_UPPERCASE[INV_MONTH];
			return MONTH_NAMES_SHORT_FIRST_UPPERCASE[Month];
		}
		else
		{
			if (Month >= INV_MONTH)
				return MONTH_NAMES_SHORT_LOWERCASE[INV_MONTH];
			return MONTH_NAMES_SHORT_LOWERCASE[Month];
		}
	}
}

uint GetNumOfDaysInMonth(int Year, uint Month)
{
	return DAYS_IN_MONTH[IsLeapYear(Year)][Month];
}

DATETIME Now()
{
	return DATETIME(GetTimeNow());
}

DATETIME UNow()
{
	DATETIME R;

#ifdef WIN32
	SYSTEMTIME thenst = { 1970, 1, 4, 1, 0, 0, 0, 0 }; // 00:00:00 Jan 1st 1970
	FILETIME thenft;
	SystemTimeToFileTime(&thenst, &thenft);
	int8 then = (int8)thenft.dwHighDateTime * 0x100000000 + thenft.dwLowDateTime; // time in 100 nanoseconds

	SYSTEMTIME nowst;
	GetSystemTime(&nowst);
	FILETIME nowft;
	SystemTimeToFileTime(&nowst, &nowft);
	int8 now = (int8)nowft.dwHighDateTime * 0x100000000 + nowft.dwLowDateTime; // time in 100 nanoseconds

	R.m_Time = (now - then) / 10000;  // time from 00:00:00 Jan 1st 1970 to now in milliseconds

#else
	struct timeval tp;
	int gtod_r = gettimeofday(&tp, NULL);
	assert(gtod_r == 0 && "gettimeofday failed.");
	R.m_Time = (int8)tp.tv_sec * 1000;
	R.m_Time = (R.m_Time + (tp.tv_usec / 1000));

#endif

	return R;
}

time_t GetTimeNow()
{
	time_t R;
	time(&R);
	return R;
}

void GetTmNow(struct tm *Out)
{
	time_t Time = GetTimeNow();
	struct tm tmstruct;
	MyLocalTime(&tmstruct, Time);
	*Out = tmstruct;
}

void DateToStr(string *Out, const TMSTRUCT &tm, const string &Format)
{
	Out->clear();
	Out->reserve(Format.length()*2);
	size_t fi = 0;
	string s;
	while (fi < Format.length())
	{
		switch (Format[fi])
		{
		case 'h':
			UintToStr(&s, tm.GetHour());
			Out->append(s);
			fi++;
			break;
		case 'H':
			UintToStr2(&s, tm.GetHour(), 2);
			Out->append(s);
			fi++;
			break;
		case 'm':
			UintToStr(&s, tm.GetMinute());
			Out->append(s);
			fi++;
			break;
		case 'M':
			UintToStr2(&s, tm.GetMinute(), 2);
			Out->append(s);
			fi++;
			break;
		case 's':
			UintToStr(&s, tm.GetSecond());
			Out->append(s);
			fi++;
			break;
		case 'S':
			UintToStr2(&s, tm.GetSecond(), 2);
			Out->append(s);
			fi++;
			break;
		case 'd':
			UintToStr(&s, tm.GetDay());
			Out->append(s);
			fi++;
			break;
		case 'D':
			UintToStr2(&s, tm.GetDay(), 2);
			Out->append(s);
			fi++;
			break;
		case 'y':
			IntToStr2(&s, (tm.GetYear() > 99 ? tm.GetYear() % 100 : tm.GetYear()), 2);
			Out->append(s);
			fi++;
			break;
		case 'Y':
			IntToStr2(&s, tm.GetYear(), 4);
			Out->append(s);
			fi++;
			break;
		case 'i':
			IntToStr(&s, tm.GetMillisecond());
			Out->append(s);
			fi++;
			break;
		case 'I':
			IntToStr2(&s, tm.GetMillisecond(), 3);
			Out->append(s);
			fi++;
			break;

		case 'w':
		case 'W':
			{
				bool Textual = false;
				uint NameForm = (Format[fi] == 'W' ? NAME_LONG : NAME_SHORT);
				fi++;
				if (fi < Format.length())
				{
					if (Format[fi] == 'l')
					{
						NameForm |= NAME_LOWERCASE;
						Textual = true;
						fi++;
					}
					else if (Format[fi] == 'f')
					{
						NameForm |= NAME_FIRST_UPPERCASE;
						Textual = true;
						fi++;
					}
					else if (Format[fi] == 'u')
					{
						NameForm |= NAME_UPPERCASE;
						Textual = true;
						fi++;
					}
				}

				if (Textual)
					Out->append(GetWeekDayName(tm.GetWeekDay(), (NAME_FORM)NameForm));
				else
				{
					WEEKDAY WeekDay = tm.GetWeekDay();
					UintToStr(&s, (WeekDay == SUN ? 7 : (uint)WeekDay));
					Out->append(s);
				}
			}
			break;

		case 'n':
		case 'N':
			{
				bool Textual = false;
				uint NameForm = (Format[fi] == 'N' ? NAME_LONG : NAME_SHORT);
				fi++;
				if (fi < Format.length())
				{
					if (Format[fi] == 'l')
					{
						NameForm |= NAME_LOWERCASE;
						Textual = true;
						fi++;
					}
					else if (Format[fi] == 'f')
					{
						NameForm |= NAME_FIRST_UPPERCASE;
						Textual = true;
						fi++;
					}
					else if (Format[fi] == 'u')
					{
						NameForm |= NAME_UPPERCASE;
						Textual = true;
						fi++;
					}
				}

				if (Textual)
					Out->append(GetMonthName(tm.GetMonth(), (NAME_FORM)NameForm));
				else
				{
					MONTH Month = tm.GetMonth();
					if (NameForm == NAME_LONG)
						UintToStr2(&s, (uint)Month+1, 2);
					else
						UintToStr(&s, (uint)Month+1);
					Out->append(s);
				}
			}
			break;

		default:
			*Out += Format[fi];
			fi++;
		}
	}
}

WEEKDAY StrToWeekDay(const string &s)
{
	for (uint i = 0; i < 7; i++)
	{
		if (s == WEEKDAY_NAMES_SHORT_LOWERCASE[i] ||
			s == WEEKDAY_NAMES_SHORT_FIRST_UPPERCASE[i] ||
			s == WEEKDAY_NAMES_SHORT_UPPERCASE[i] ||
			s == WEEKDAY_NAMES_LONG_LOWERCASE[i] ||
			s == WEEKDAY_NAMES_LONG_FIRST_UPPERCASE[i] ||
			s == WEEKDAY_NAMES_LONG_UPPERCASE[i])
		{
			return (WEEKDAY)i;
		}
	}
	return INV_WEEKDAY;
}

MONTH StrToMonth(const string &s)
{
	for (uint i = 0; i < 12; i++)
	{
		if (s == MONTH_NAMES_SHORT_LOWERCASE[i] ||
			s == MONTH_NAMES_SHORT_FIRST_UPPERCASE[i] ||
			s == MONTH_NAMES_SHORT_UPPERCASE[i] ||
			s == MONTH_NAMES_LONG_LOWERCASE[i] ||
			s == MONTH_NAMES_LONG_FIRST_UPPERCASE[i] ||
			s == MONTH_NAMES_LONG_UPPERCASE[i])
		{
			return (MONTH)i;
		}
	}
	return INV_MONTH;
}

// [wewnêtrzna]
void ParseNumber(string *Out, const string &s, size_t *InOutPos)
{
	Out->clear();
	while (*InOutPos < s.length() && CharIsDigit(s[*InOutPos]))
	{
		*Out += s[*InOutPos];
		(*InOutPos)++;
	}
}

// [wewnêtrzna]
void ParseName(string *Out, const string &s, size_t *InOutPos)
{
	Out->clear();
	while (*InOutPos < s.length() && CharIsAlphaNumeric_f(s[*InOutPos]))
	{
		*Out += s[*InOutPos];
		(*InOutPos)++;
	}
}

bool StrToDate(TMSTRUCT *Out, const string &s, const string &Format)
{
	int Year = 0;
	MONTH Month = JAN;
	uint2 Day = 0;
	uint2 Hour = 0;
	uint2 Minute = 0;
	uint2 Second = 0;
	uint2 Millisecond = 0;

	size_t fi = 0, si = 0;
	string Tmp;
	while (fi < Format.length())
	{
		switch (Format[fi])
		{
		case 'h':
		case 'H':
			if (Hour != 0) return false;
			ParseNumber(&Tmp, s, &si);
			fi++;
			if (StrToUint(&Hour, Tmp) != 0) return false;
			break;
		case 'm':
		case 'M':
			if (Minute != 0) return false;
			ParseNumber(&Tmp, s, &si);
			fi++;
			if (StrToUint(&Minute, Tmp) != 0) return false;
			break;
		case 's':
		case 'S':
			if (Second != 0) return false;
			ParseNumber(&Tmp, s, &si);
			fi++;
			if (StrToUint(&Second, Tmp) != 0) return false;
			break;
		case 'd':
		case 'D':
			if (Day != 0) return false;
			ParseNumber(&Tmp, s, &si);
			fi++;
			if (StrToUint(&Day, Tmp) != 0) return false;
			break;
		case 'i':
		case 'I':
			if (Millisecond != 0) return false;
			ParseNumber(&Tmp, s, &si);
			fi++;
			if (StrToUint(&Millisecond, Tmp) != 0) return false;
			break;
		case 'y':
		case 'Y':
			{
				bool Short = (Format[fi] == 'y');
				if (Year != 0) return false;
				ParseNumber(&Tmp, s, &si);
				fi++;
				if (StrToInt(&Year, Tmp) != 0) return false;
				if (Short)
				{
					if (Year >= 70)
						Year += 1900;
					else
						Year += 2000;
				}
			}
			break;

		case 'w':
		case 'W':
			fi++;
			// Tekstowe
			if (fi < Format.length() && (Format[fi] == 'l' || Format[fi] == 'f' || Format[fi] == 'u'))
			{
				fi++;
				ParseName(&Tmp, s, &si);
			}
			// Liczbowe
			else
				ParseNumber(&Tmp, s, &si);
			break;

		case 'n':
		case 'N':
			fi++;
			// Tekstowe
			if (fi < Format.length() && (Format[fi] == 'l' || Format[fi] == 'f' || Format[fi] == 'u'))
			{
				fi++;
				ParseName(&Tmp, s, &si);
				Month = StrToMonth(Tmp);
				if (Month == INV_MONTH) return false;
			}
			// Liczbowe
			else
			{
				ParseNumber(&Tmp, s, &si);
				uint MonthNumber;
				if (StrToUint(&MonthNumber, Tmp) != 0) return false;
				Month = (MONTH)MonthNumber;
			}
			break;

		default:
			if (si >= s.length()) return false;
			if (s[si] != Format[fi]) return false;
			si++;
			fi++;
		}
	}

	*Out = TMSTRUCT(Day, Month, Year, Hour, Minute, Second, Millisecond);
	return true;
}

} // namespace common
