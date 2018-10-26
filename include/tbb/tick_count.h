/*
 * tick_count.h
 *
 *  Created on: Oct 26, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_TICK_COUNT_H_
#define INCLUDE_TBB_TICK_COUNT_H_

#include "tbb_stddef.h"

#if _WIN32||_WIN64
#include "machine/windows_api.h"
#elif __linux__
#include <ctime>
#else
#include <sys/time.h>
#endif

namespace tbb {

// Absolute timestamp
class tick_count {

private:
	long long my_count;

public:
	class interval_t {
		long long value;
		explicit interval_t(long long value_) : value (value_) {}
	public:
		interval_t() : value(0) {};
		explicit interval_t(double sec);
		double seconds() const;
		friend class tbb::tick_count;
		friend interval_t operator-(const tick_count& t1, const tick_count& t0);
		friend interval_t operator+(const interval_t& i, const interval_t& j) {
			return interval_t(i.value + j.value);
		}
		friend interval_t operator-(const interval_t& i, const interval_t& j) {
			return interval_t(i.value - j.value);
		}

		interval_t& operator+= (const interval_t& i) {
			value += i.value;
			return *this;
		}

		interval_t& operator-= (const interval_t& i) {
			value -= i.value;
			return this;
		}

	private:
		static long long ticks_per_second()
		{
#if _WIN32 || _WIN64
			LARGE_INTEGER qpfreq;
			int rval = QueryPerformanceFrequency(&qpfreq);
			__TBB_ASSERT_EX(rval, "QueryPerformanceFrequency returned zero");
			return static_cast<long long> (qpfreq.QuadPart);
#elif __linux_
			return static_cast<long long>(1E9);
#else
			return static_cast<long long>(1E6);
#endif
		}
	};

	tick_count() : my_count(0) {};
	static tick_count now();
	friend interval_t operator-(const tick_count& t1, const tick_count& t0);
	static double resolution()
	{
		return 1.0 / interval_t::ticks_per_second();
	}
};

inline tick_count tick_count::now() {
	tick_count result;
	#if _WIN32 || _WIN64
	LARGE_INTEGER qpcnt;
	int rval = QueryPerformanceCounter(&qpcnt);
	__TBB_ASSERT_EX(rval, "QueryPerformanceCounter failed");
	result.my_count = qpcnt.QuadPart;
	#elif __linux__
	struct timespec ts;
	int status = clock_gettime(CLOCK_REALTIME, &ts);
	__TBB_ASSERT_EX(status == 0, "CLOCK_REALTIME not supported");
	result.my_count = static_cast<long long>(1000000000UL)*static_cast<long long>(ts.tv_sec)
	                  + static_cast<long long>(ts.tv_nsec);
	#else
	struct timeval tv;
	int status = gettimeofday(&tv, NULL);
	__TBB_ASSERT_EX(status == 0, "gettimeofday failed");
	result.my_count = static_cast<long long>(1000000)*static_cast<long long>(ts.tv_sec)
	                  + static_cast<long long>(ts.tv_usec);
	#endif
	return result;
}

inline tick_count::interval_t::interval_t(double sec)
{
	value = static_cast<long long>(sec *interval_t::ticks_per_second());
}

inline tick_count::interval_t operator-(const tick_count& t1, const tick_count& t2)
{
	return tick_count::interval_t(t1.my_count - t2.my_count);
}

inline double tick_count::interval_t::seconds() const {
	return value * tick_count::resolution();
}

}

#endif /* INCLUDE_TBB_TICK_COUNT_H_ */
