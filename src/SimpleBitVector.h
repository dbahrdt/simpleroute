#ifndef SSERIALIZE_SIMPLE_BIT_VECTOR_H
#define SSERIALIZE_SIMPLE_BIT_VECTOR_H
#include <vector>
#include <limits>
#include <stdint.h>

namespace sserialize {

class SimpleBitVector {
public:
	typedef uint64_t BaseStorageType;
private:
	static constexpr int digits = std::numeric_limits<BaseStorageType>::digits;
public:
	SimpleBitVector();
	SimpleBitVector(std::size_t size);
	~SimpleBitVector();
	std::size_t storageSizeInBytes() const;
	std::size_t capacity() const;
	void resize(std::size_t count);
	void set(std::size_t pos);
	bool isSet(std::size_t pos);
	void reset();
	template<typename TInputIterator>
	void set(TInputIterator begin, const TInputIterator & end);
	///get all set positions in ascending order 
	template<typename TOutputIterator>
	void getSet(TOutputIterator out) const;
private:
	std::vector<BaseStorageType> m_d;
};

template<typename TInputIterator>
void SimpleBitVector::set(TInputIterator begin, const TInputIterator & end) {
	for(; begin != end; ++begin) {
		set(*begin);
	}
}

template<typename TOutputIterator>
void SimpleBitVector::getSet(TOutputIterator out) const {
	std::size_t v = 0;
	for(BaseStorageType x : m_d) {
		for(std::size_t myV(v); x; ++myV, x >>= 1) {
			if (x & 0x1) {
				*out = myV;
				++out;
			}
		}
		v += digits;
	}
}

}//end namespace sserialize


#endif