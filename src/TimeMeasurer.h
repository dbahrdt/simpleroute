#ifndef SIMPLEROUTE_TIME_MEASURER_H
#define SIMPLEROUTE_TIME_MEASURER_H
#include <time.h>
#include <sys/time.h>
#include <cstdlib> 
#include <stdint.h>
#include <string.h>
#include <iostream>


namespace simpleroute {

class TimeMeasurer {
private:
	struct timeval m_begin, m_end;
public:
	TimeMeasurer() {
		::memset(&m_begin, 0, sizeof(struct timeval));
		::memset(&m_end, 0, sizeof(struct timeval));
	}
	
	~TimeMeasurer() {}
	
	inline void begin() {
		gettimeofday(&m_begin, NULL);
	}
	
	inline void end() {
		gettimeofday(&m_end, NULL);
	}
	
	inline long beginTime() const {
		return m_begin.tv_sec;
	}
	
	/** @return returns the elapsed time in useconds  */
	inline long elapsedTime() const {
		long mtime, seconds, useconds;
		seconds  = m_end.tv_sec  - m_begin.tv_sec;
		useconds = m_end.tv_usec - m_begin.tv_usec;
		mtime = ((seconds) * 1000*1000 + useconds) + 0.5;
		return mtime;
	}

	inline long elapsedMilliSeconds() const {
		return elapsedTime()/1000;
	}
};


}//end namespace

#endif