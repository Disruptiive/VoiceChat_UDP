#include <stdexcept>
#include <algorithm>
#include <array>

template <typename T, int N, int sz>
class RingBuffer {
public:
	RingBuffer() {
		std::generate(element_array.begin(), element_array.end(), [] {return new T[sz]; });
	}
	T* getChunk() {
		if (m_sz < sz) {
			++m_sz;
			auto* chunk = element_array[ptr];
			ptr = increment(ptr);
			return chunk;
		}
		else {
			throw std::runtime_error("Ring Buffer Full");
		}
	}

	void returnChunks(int k = 1) {
		k = std::min(k, m_sz);
		m_sz -= k;
	}

	~RingBuffer() {
		for (int i = 0; i < N; ++i) {
			delete[] element_array[i];
		}
	}
private:
	int increment(int i, int k = 1) {
		return (i + k) % N;
	}
	std::array<T*, N> element_array;
	int m_sz{ 0 };
	int ptr{ 0 };
};