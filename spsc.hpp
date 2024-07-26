#pragma once

// c
#include <stdint.h>

// c++
#include <algorithm>
#include <atomic>
#include <memory>
#include <vector>



inline uint32_t roundup_pow_of_two(uint32_t n)
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;

	return n;
}


inline uint32_t rounddown_pow_of_two(uint32_t n)
{
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;

	return (n + 1) >> 1;
}


// lock free, yet thread-safe single-producer single-consumer buffer
template<typename T>
class lock_free_spsc
{
public:
	lock_free_spsc()
		: m_input_offset(0)
		, m_output_offset(0)
	{
	}

	lock_free_spsc(uint32_t size)
		: m_input_offset(0)
		, m_output_offset(0)
	{
		reset(size);
	}

	~lock_free_spsc()
	{
		reset(0);
	}

	void reset(uint32_t size)
	{
		if (0 == size) {
			m_ring_buffer.clear();
			m_input_offset = 0;
			m_output_offset = 0;
		}
		else {
			if (size & (size - 1)) {
				size = roundup_pow_of_two(size);
			}

			if (size > 0) {
				m_ring_buffer.resize(size, 0);
			}
		}
	}

	bool is_null()
	{
		return m_ring_buffer.empty();
	}

	uint32_t put(const std::vector<T> &input_buffer)
	{
		return put(input_buffer.data(), (uint32_t)input_buffer.size());
	}

	uint32_t put(const T *input_buffer, uint32_t length)
	{
		uint32_t size = (uint32_t)m_ring_buffer.size();

		length = std::min(length, size - load_relaxed(m_input_offset) + load_relaxed(m_output_offset));
		if (length <= 0) {
			return 0;
		}

		// ensure that we sample the input offset before we start putting bytes into the buffer
		std::atomic_thread_fence(std::memory_order_acquire);

		uint32_t write_offset = load_relaxed(m_input_offset) & (size - 1);

		// first put the data starting from in to buffer end
		uint32_t first_part = std::min(length, size - write_offset);
		std::memcpy(m_ring_buffer.data() + write_offset, input_buffer, sizeof(T) * first_part);

		// then put the rest (if any) at the beginning of the buffer
		std::memcpy(m_ring_buffer.data(), input_buffer + first_part, sizeof(T) * (length - first_part));

		// ensure that we add the bytes to the buffer before we update the input offset
		std::atomic_thread_fence(std::memory_order_release);

		store_relaxed(m_input_offset, load_relaxed(m_input_offset) + length);

		return length;
	}

	uint32_t get(std::vector<T> &output_buffer)
	{
		return get(output_buffer.data(), (uint32_t)output_buffer.size());
	}

	uint32_t get(T *output_buffer, uint32_t length)
	{
		auto buff_len = length;

		length = std::min(length, load_relaxed(m_input_offset) - load_relaxed(m_output_offset));
		if (length <= 0) {
			return 0;
		}

		// ensure that we sample the output offset before we start removing bytes from the buffer
		std::atomic_thread_fence(std::memory_order_acquire);

		uint32_t size = (uint32_t)m_ring_buffer.size();

		uint32_t read_offset = load_relaxed(m_output_offset) & (size - 1);

		// first get the data from out until the end of the buffer
		uint32_t first_part = std::min(length, size - read_offset);
		std::memcpy(output_buffer, m_ring_buffer.data() + read_offset, sizeof(T) * first_part);

		// then get the rest (if any) from the beginning of the buffer
		std::memcpy(output_buffer + first_part, m_ring_buffer.data(), sizeof(T) * (length - first_part));

		// ensure that we remove the bytes from the buffer before we update the output offset
		std::atomic_thread_fence(std::memory_order_release);

		store_relaxed(m_output_offset, load_relaxed(m_output_offset) + length);

		return length;
	}


protected:
	inline uint32_t load_relaxed(const std::atomic<uint32_t> &offset) const
	{
		return offset.load(std::memory_order_relaxed);
	}

	inline void store_relaxed(std::atomic<uint32_t> &offset, uint32_t value)
	{
		offset.store(value, std::memory_order_relaxed);
	}


private:
	std::vector<T> m_ring_buffer;  // the buffer holding the data
	std::atomic<uint32_t> m_input_offset;  // data is added at offset: m_input_offset % (size - 1)
	std::atomic<uint32_t> m_output_offset;  // data is extracted from offset: m_output_offset % (size - 1)
};

