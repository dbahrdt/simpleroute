#ifndef SIMPLE_ROUTE_MULTI_READER_SINGLE_WRITER_LOCK_H
#define SIMPLE_ROUTE_MULTI_READER_SINGLE_WRITER_LOCK_H
#include <QSemaphore>

namespace simpleroute {

struct MultiReaderSingleWriterLockBase {
	typedef enum { READ_LOCK=0x1, WRITE_LOCK=0x7FFFFFFF} LockType;
};

class MultiReaderSingleWriterLock: public MultiReaderSingleWriterLockBase {
public:
	MultiReaderSingleWriterLock() : m_s(WRITE_LOCK) {}
	~MultiReaderSingleWriterLock() {
		m_s.acquire(WRITE_LOCK);
		m_s.release(WRITE_LOCK);
	}
	inline void lock(LockType lt) {
		m_s.acquire(lt);
	}
	inline void unlock(LockType lt) {
		m_s.release(lt);
	}
private:
	QSemaphore m_s;
};

class MultiReaderSingleWriterLocker: public MultiReaderSingleWriterLockBase {
	MultiReaderSingleWriterLock & m_s;
	const LockType m_lt;
public:
	MultiReaderSingleWriterLocker(MultiReaderSingleWriterLock & s, LockType lt) : m_s(s), m_lt(lt) {
		m_s.lock(m_lt);
	}
	~MultiReaderSingleWriterLocker() {
		m_s.unlock(m_lt);
	}
};

}

#endif