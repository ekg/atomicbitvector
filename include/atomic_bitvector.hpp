/*
 * Copyright 2019 Erik Garrison
 * Copyright 2019 Facebook, Inc. and its affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

//#include <array>
#include <vector>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include "iterator_tpl.h"

namespace atomicbitvector {

/*
 * A wrapper that allows us to construct a vector of atomic elements
 * https://stackoverflow.com/questions/13193484/how-to-declare-a-vector-of-atomic-in-c
 */

template <typename T>
struct atomwrapper {
    std::atomic<T> _a;
    atomwrapper() : _a() { }
    atomwrapper(const std::atomic<T> &a) : _a(a.load()) { }
    atomwrapper(const atomwrapper &other) : _a(other._a.load()) { }
    atomwrapper &operator=(const atomwrapper &other) {
        _a.store(other._a.load());
    }
    std::atomic<T>& ref(void) { return _a; }
    const std::atomic<T>& const_ref(void) const { return _a; }
};

/**
 * An atomic bitvector with size fixed at construction time
 */
class atomic_bv_t {
public:
    /**
     * Construct a atomic_bv_t; all bits are initially false.
     */
    atomic_bv_t(size_t N) : _size(N),
                            kNumBlocks((N + kBitsPerBlock - 1) / kBitsPerBlock) {
        data_.resize(kNumBlocks);
    }

private:
	template<class CharT>
	void _copy_from_ptr(const CharT* str, typename std::basic_string<CharT>::size_type n, CharT zero, CharT one)
	{
		if (n != std::basic_string<CharT>::npos && str != nullptr)
		{
			_size = n;
			kNumBlocks = (n + kBitsPerBlock - 1) / kBitsPerBlock;
			data_.resize(kNumBlocks);
			for (auto ii{typename std::basic_string<CharT>::size_type(0)}; ii < n; ++ii)
			{
				if (*(str + ii) == one)
					set(ii);
				else if (*(str + ii) == zero)
					reset(ii);
				else
					throw std::invalid_argument(str);
			}
		}
		else
			throw std::out_of_range("invalid bitstring");
	}

public:
    /**
     * Construct a atomic_bv_t from char *
     */
	template<class CharT>
	explicit atomic_bv_t(const CharT* str, typename std::basic_string<CharT>::size_type n = std::basic_string<CharT>::npos,
	  CharT zero = CharT('0'), CharT one = CharT('1'))
	{
		_copy_from_ptr(str, n, zero, one);
	}

    /**
     * Construct a atomic_bv_t from std::string
     */
	template<class CharT = char, class Traits = std::char_traits<CharT>, class Allocator = std::allocator<CharT>>
	explicit atomic_bv_t(const std::basic_string<CharT,Traits,Allocator>& str, CharT zero = CharT('0'), CharT one = CharT('1'))
	{
		_copy_from_ptr(str.c_str(), str.size(), zero, one);
	}

    /**
     * Construct std::string from bitset
     */
	template<class CharT = char, class Traits = std::char_traits<CharT>, class Allocator = std::allocator<CharT>>
	std::basic_string<CharT,Traits,Allocator> to_string(CharT zero = CharT('0'), CharT one = CharT('1')) const
	{
		std::basic_string<CharT,Traits,Allocator> str(_size, zero);
		for (size_t ii{}; ii < _size; ++ii)
			if (test(ii))
				str[ii] = one;
		return str;
	}

    /**
     * sendto friend
     */
	template <class CharT, class Traits>
	friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const atomic_bv_t& what)
	{
		return os << what.to_string();
	}

    /**
     * Set bit idx to true, using the given memory order. Returns the
     * previous value of the bit.
     *
     * Note that the operation is a read-modify-write operation due to the use
     * of fetch_or.
     */
    bool set(size_t idx, std::memory_order order = std::memory_order_seq_cst);

    /**
     * Set bit idx to false, using the given memory order. Returns the
     * previous value of the bit.
     *
     * Note that the operation is a read-modify-write operation due to the use
     * of fetch_and.
     */
    bool reset(size_t idx, std::memory_order order = std::memory_order_seq_cst);

    /**
     * Set bit idx to the given value, using the given memory order. Returns
     * the previous value of the bit.
     *
     * Note that the operation is a read-modify-write operation due to the use
     * of fetch_and or fetch_or.
     *
     * Yes, this is an overload of set(), to keep as close to std::bitset's
     * interface as possible.
     */
    bool set(
        size_t idx,
        bool value,
        std::memory_order order = std::memory_order_seq_cst);

    /**
     * Read bit idx.
     */
    bool test(size_t idx, std::memory_order order = std::memory_order_seq_cst)
        const;

    /**
     * Same as test() with the default memory order.
     */
    bool operator[](size_t idx) const;

    /**
     * Return the size of the bitset.
     */
    constexpr size_t size() const {
        return _size;
    }

    /**
     * Iterators to run through the set values
     */
    struct it_state {
        size_t pos;
        inline void next(const atomic_bv_t* ref) { ++pos; while (pos < ref->size() && !ref->test(pos)) { ++pos; } }
        inline void prev(const atomic_bv_t* ref) { --pos; while (pos > 0 && !ref->test(pos)) { --pos; } }
        inline void begin(const atomic_bv_t* ref) { pos = 0; while (pos < ref->size() && !ref->test(pos)) { ++pos; } }
        inline void end(const atomic_bv_t* ref) { pos = ref->size(); }
        inline size_t get(atomic_bv_t* ref) { return pos; }
        inline const size_t get(const atomic_bv_t* ref) const { return ref->test(pos); }
        inline bool cmp(const it_state& s) const { return pos != s.pos; }
    };
    SETUP_ITERATORS(atomic_bv_t, size_t, it_state);
    SETUP_REVERSE_ITERATORS(atomic_bv_t, size_t, it_state);

private:
    // Pick the largest lock-free type available
#if (ATOMIC_LLONG_LOCK_FREE == 2)
    typedef unsigned long long BlockType;
#elif (ATOMIC_LONG_LOCK_FREE == 2)
    typedef unsigned long BlockType;
#else
    // Even if not lock free, what can we do?
    typedef unsigned int BlockType;
#endif
    typedef atomwrapper<BlockType> AtomicBlockType;

    static constexpr size_t kBitsPerBlock =
        std::numeric_limits<BlockType>::digits;

    static constexpr size_t blockIndex(size_t bit) {
        return bit / kBitsPerBlock;
    }

    static constexpr size_t bitOffset(size_t bit) {
        return bit % kBitsPerBlock;
    }

    // avoid casts
    static constexpr BlockType kOne = 1;
    size_t _size; // dynamically set
    size_t kNumBlocks; // dynamically set
    std::vector<AtomicBlockType> data_; // filled at instantiation time
};

inline bool atomic_bv_t::set(size_t idx, std::memory_order order) {
    assert(idx < _size);
    BlockType mask = kOne << bitOffset(idx);
    return data_[blockIndex(idx)].ref().fetch_or(mask, order) & mask;
}

inline bool atomic_bv_t::reset(size_t idx, std::memory_order order) {
    assert(idx < _size);
    BlockType mask = kOne << bitOffset(idx);
    return data_[blockIndex(idx)].ref().fetch_and(~mask, order) & mask;
}

inline bool atomic_bv_t::set(size_t idx, bool value, std::memory_order order) {
    return value ? set(idx, order) : reset(idx, order);
}

inline bool atomic_bv_t::test(size_t idx, std::memory_order order) const {
    assert(idx < _size);
    BlockType mask = kOne << bitOffset(idx);
    return data_[blockIndex(idx)].const_ref().load(order) & mask;
}

inline bool atomic_bv_t::operator[](size_t idx) const {
    return test(idx);
}

}
