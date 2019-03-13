// By Huang Yuanbing 2019
// bailuzhou@163.com
// https://github.com/ktprime/ktprime/blob/master/hash_table5.hpp

// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.


#pragma once

#include <cstdlib>
#include <iterator>
#include <utility>
#include <cstring>
#include <cassert>
#include <initializer_list>

#if TAF_V3
    #include "LogHelper.h"
#endif

#ifdef  GET_KEY
    #undef  GET_KEY
    #undef  GET_VAL
    #undef  BUCKET
    #undef  NEXT_BUCKET
    #undef  GET_PVAL
#endif

#if 1
    #define BUCKET(key)  int(_hasher(key) & _mask)
#else
    #define BUCKET(key)  hash_key(key)
#endif

#ifndef ORDER_INDEX
    #define ORDER_INDEX 2
#endif

#if ORDER_INDEX == 0
    #define GET_KEY(p,n)     p[n].second.first
    #define GET_VAL(p,n)     p[n].second.second
    #define NEXT_BUCKET(s,n) s[n].first
    #define GET_PVAL(s,n)    s[n].second
    #define NEW_KVALUE(key, value, bucket)  new(_pairs + bucket) PairT(bucket, std::pair<KeyT, ValueT>(key, value))
#elif ORDER_INDEX == 1
    #define GET_KEY(p,n)     p[n].first.first
    #define GET_VAL(p,n)     p[n].first.second
    #define NEXT_BUCKET(s,n) s[n].second
    #define GET_PVAL(s,n)    s[n].first
    #define NEW_KVALUE(key, value, bucket) new(_pairs + bucket) PairT(std::pair<KeyT, ValueT>(key, value), bucket);
#else
    #define GET_KEY(p,n)     p[n]._mypair.first
    #define GET_VAL(p,n)     p[n]._mypair.second
    #define NEXT_BUCKET(s,n) s[n]._mypair._ibucket
    #define GET_PVAL(s,n)    s[n]._mypair
    #define NEW_KVALUE(key, value, bucket) new(_pairs + bucket) PairT(key, value, bucket);
#endif

namespace emilib1 {
/// like std::equal_to but no need to #include <functional>
template<typename T>
struct HashMapEqualTo
{
    constexpr bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs == rhs;
    }
};

template <typename First, typename Second>
struct pair {
    typedef First  first_type;
    typedef Second second_type;

    // pair constructors are explicit so we don't accidentally call this ctor when we don't have to.
    pair(const First& firstArg, const Second& secondArg)
        : first{ firstArg }
        , second{ secondArg } { _ibucket = -1; }

    pair(First&& firstArg, Second&& secondArg)
        : first{ std::move(firstArg) }
        , second{ std::move(secondArg) } { _ibucket = -1; }

    void swap(pair<First, Second>& o) {
        std::swap(first, o.first);
        std::swap(second, o.second);
//      std::swap(_ibucket, o._ibucket);
    }

    First first; //long
    int    _ibucket;
    Second second;//int

};// __attribute__ ((packed));

/// A cache-friendly hash table with open addressing, linear probing and power-of-two capacity
template <typename KeyT, typename ValueT, typename HashT = std::hash<KeyT>, typename EqT = HashMapEqualTo<KeyT>>
class HashMap
{
    enum State
    {
        INACTIVE = -1, // Never been touched
    };

private:
    typedef  HashMap<KeyT, ValueT, HashT, EqT> MyType;

#if ORDER_INDEX == 0
    typedef std::pair<int, std::pair<KeyT, ValueT>> PairT;
#elif ORDER_INDEX == 1
    typedef std::pair<std::pair<KeyT, ValueT>, int> PairT;
#else

    struct PairT
    {
        explicit PairT(const KeyT& key, const ValueT& value, int bucket)
            :_mypair(key, value)
        {
            _mypair._ibucket = bucket;
        }

        explicit PairT(PairT& pairT)
            :_mypair(pairT._mypair.first, pairT._mypair.second)
        {
            _mypair._ibucket = pairT._mypair._ibucket;
        }

        explicit PairT(PairT&& pairT)
            :_mypair(pairT._mypair.first, pairT._mypair.second)
        {
            _mypair._ibucket = pairT._mypair._ibucket;
        }

        ~PairT()
        {
        }

        pair<KeyT, ValueT> _mypair;
    };
#endif

public:
    typedef  size_t       size_type;
    typedef  PairT        value_type;
    typedef  PairT&       reference;
    typedef  const PairT& const_reference;

    class iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef size_t                    difference_type;
        typedef size_t                    distance_type;

#if ORDER_INDEX > 1
        typedef pair<KeyT, ValueT>        value_type;
#else
        typedef std::pair<KeyT, ValueT>   value_type;
#endif

        typedef value_type*               pointer;
        typedef value_type&               reference;

        iterator() { }

        iterator(MyType* hash_map, unsigned int bucket) : _map(hash_map), _bucket(bucket)
        {
        }

        iterator& operator++()
        {
            this->goto_next_element();
            return *this;
        }

        iterator operator++(int)
        {
            auto old_index = _bucket;
            this->goto_next_element();
            return iterator(_map, old_index);
        }

        reference operator*() const
        {
            return _map->GET_PVAL(_pairs, _bucket);
        }

        pointer operator->() const
        {
            return &(_map->GET_PVAL(_pairs, _bucket));
        }

        bool operator==(const iterator& rhs) const
        {
            return this->_bucket == rhs._bucket;
        }

        bool operator!=(const iterator& rhs) const
        {
            return this->_bucket != rhs._bucket;
        }

    private:
        void goto_next_element()
        {
            do {
                _bucket++;
            } while (_bucket < _map->_num_buckets && _map->NEXT_BUCKET(_pairs, _bucket) == State::INACTIVE);
        }

        //private:
        //    friend class MyType;
    public:
        MyType* _map;
        unsigned int  _bucket;
    };

    class const_iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef size_t                    difference_type;
        typedef size_t                    distance_type;
#if ORDER_INDEX > 1
        typedef pair<KeyT, ValueT>        value_type;
#else
        typedef std::pair<KeyT, ValueT>   value_type;
#endif

        typedef value_type*               pointer;
        typedef value_type&               reference;

        const_iterator() { }

        const_iterator(iterator proto) : _map(proto._map), _bucket(proto._bucket)
        {
        }

        const_iterator(const MyType* hash_map, unsigned int bucket) : _map(hash_map), _bucket(bucket)
        {
        }

        const_iterator& operator++()
        {
            this->goto_next_element();
            return *this;
        }

        const_iterator operator++(int)
        {
            auto old_index = _bucket;
            this->goto_next_element();
            return const_iterator(_map, old_index);
        }

        reference operator*() const
        {
            return _map->GET_PVAL(_pairs, _bucket);
        }

        pointer operator->() const
        {
            return &(_map->GET_PVAL(_pairs, _bucket));
        }

        bool operator==(const const_iterator& rhs) const
        {
            return this->_bucket == rhs._bucket;
        }

        bool operator!=(const const_iterator& rhs) const
        {
            return this->_bucket != rhs._bucket;
        }

    private:
        void goto_next_element()
        {
            do {
                _bucket++;
            } while (_bucket < _map->_num_buckets && _map->NEXT_BUCKET(_pairs, _bucket) == State::INACTIVE);
        }

    public:
        const MyType* _map;
        unsigned int  _bucket;
    };

    // ------------------------------------------------------------------------

    void init()
    {
        _num_buckets = 0;
        _num_filled = 0;
        _mask = 0;  // _num_buckets minus one
        _shift = 64;
        _pairs = nullptr;
        _max_load_factor = 0.9;
        _load_buckets = 4 * _max_load_factor;
    }

    HashMap()
    {
        init();
        reserve(8);
    }

    HashMap(const HashMap& other)
    {
        init();
        reserve(other.size());
        insert(other.cbegin(), other.cend());
    }

    HashMap(HashMap&& other)
    {
        init();
        reserve(8);
        *this = std::move(other);
    }

    HashMap(std::initializer_list< std::pair<KeyT, ValueT>> il)
    {
        init();
        reserve(il.size());
        for (auto begin = il.begin(); begin != il.end(); ++begin)
            insert(*begin);
    }

    HashMap& operator=(const HashMap& other)
    {
        clear();
        reserve(other.size());
        insert(other.cbegin(), other.cend());
        return *this;
    }

    HashMap& operator=(HashMap&& other)
    {
        this->swap(other);
        return *this;
    }

    ~HashMap()
    {
        for (unsigned int bucket = 0; bucket < _num_buckets; ++bucket) {
            if (NEXT_BUCKET(_pairs, bucket) != State::INACTIVE) {
                _pairs[bucket].~PairT();
            }
        }
        if (_pairs)
            free(_pairs);
    }

    void swap(HashMap& other)
    {
        std::swap(_hasher, other._hasher);
        std::swap(_max_load_factor, other._max_load_factor);
        std::swap(_load_buckets, other._load_buckets);
        std::swap(_pairs, other._pairs);
        std::swap(_num_buckets, other._num_buckets);
        std::swap(_num_filled, other._num_filled);
        std::swap(_mask, other._mask);
        std::swap(_shift, other._shift);
    }

    // -------------------------------------------------------------

    iterator begin()
    {
        unsigned int bucket = 0;
        while (bucket < _num_buckets && NEXT_BUCKET(_pairs, bucket) == State::INACTIVE) {
            ++bucket;
        }
        return iterator(this, bucket);
    }

    const_iterator cbegin() const
    {
        unsigned int bucket = 0;
        while (bucket < _num_buckets && NEXT_BUCKET(_pairs, bucket) == State::INACTIVE) {
            ++bucket;
        }
        return const_iterator(this, bucket);
    }

    const_iterator begin() const
    {
        return cbegin();
    }

    iterator end()
    {
        return iterator(this, _num_buckets);
    }

    const_iterator cend() const
    {
        return const_iterator(this, _num_buckets);
    }

    const_iterator end() const
    {
        return cend();
    }

    size_type size() const
    {
        return _num_filled;
    }

    bool empty() const
    {
        return _num_filled == 0;
    }

    // Returns the number of buckets.
    size_type bucket_count() const
    {
        return _num_buckets;
    }

    /// Returns average number of elements per bucket.
    float load_factor() const
    {
        return static_cast<float>(_num_filled) / static_cast<float>(_num_buckets);
    }

    HashT hash_function() const
    {
        return _hasher;
    }

    constexpr float max_load_factor() const
    {
        return _max_load_factor;
    }

    void max_load_factor(float value)
    {
        if (value < 0.95 && value > 0.1)
           _max_load_factor = value;
    }

    constexpr size_type max_size() const
    {
        return (1 << 30) / sizeof(PairT);
    }

    constexpr size_type max_bucket_count() const
    {
        return (1 << 30) / sizeof(PairT);
    }

    //Returns the bucket number where the element with key k is located.
    size_type bucket(const KeyT& key) const
    {
        const auto ibucket = BUCKET(key);
        const auto next_bucket = NEXT_BUCKET(_pairs, ibucket);
        if (next_bucket == State::INACTIVE)
            return 0;
        if (ibucket == next_bucket)
            return ibucket + 1;

        const auto& bucket_key = GET_KEY(_pairs, ibucket);
        return BUCKET(bucket_key) + 1;
    }

    //Returns the number of elements in bucket n.
    size_type bucket_size(const size_type bucket) const
    {
        auto next_bucket = NEXT_BUCKET(_pairs, bucket);
        if (next_bucket == State::INACTIVE)
             return 0;

        const auto& bucket_key = GET_KEY(_pairs, bucket);
        next_bucket = BUCKET(bucket_key);
        int ibucket_size = 1;

        //find a new empty and linked it to tail
        while (true) {
            const auto nbucket = NEXT_BUCKET(_pairs, next_bucket);
            if (nbucket == next_bucket) {
                break;
            }
            ibucket_size ++;
            next_bucket = nbucket;
        }
        return ibucket_size;
    }

    size_type get_main_bucket(const unsigned int bucket) const
    {
        auto next_bucket = NEXT_BUCKET(_pairs, bucket);
        if (next_bucket == State::INACTIVE)
            return -1;

        const auto& bucket_key = GET_KEY(_pairs, bucket);
        const auto main_bucket = BUCKET(bucket_key);
        return main_bucket;
    }

    /****
    std::pair<iterator, iterator> equal_range(const keyT & key)
    {
        iterator found = find(key);
        if (found == end())
            return {found, found};
        else
            return {found, std::next(found)};
    }*/

    // ------------------------------------------------------------

    iterator find(const KeyT& key)
    {
        auto bucket = find_filled_bucket(key);
        if (bucket == State::INACTIVE) {
            return end();
        }
        return iterator(this, bucket);
    }

    const_iterator find(const KeyT& key) const
    {
        auto bucket = find_filled_bucket(key);
        if (bucket == State::INACTIVE) {
            return end();
        }
        return const_iterator(this, bucket);
    }

    bool contains(const KeyT& k) const
    {
        return find_filled_bucket(k) != State::INACTIVE;
    }

    size_t count(const KeyT& k) const
    {
        return find_filled_bucket(k) != State::INACTIVE ? 1 : 0;
    }

    /// Returns the matching ValueT or nullptr if k isn't found.
    bool try_get(const KeyT& k, ValueT& val)
    {
        auto bucket = find_filled_bucket(k);
        if (bucket != State::INACTIVE) {
            val = GET_VAL(_pairs, bucket);
            return true;
        }
        else {
            return false;
        }
    }

    /// Returns the matching ValueT or nullptr if k isn't found.
    ValueT* try_get(const KeyT& k)
    {
        auto bucket = find_filled_bucket(k);
        if (bucket != State::INACTIVE) {
            return &GET_VAL(_pairs, bucket);
        }
        else {
            return nullptr;
        }
    }

    /// Const version of the above
    const ValueT* try_get(const KeyT& k) const
    {
        auto bucket = find_filled_bucket(k);
        if (bucket != State::INACTIVE) {
            return &GET_VAL(_pairs, bucket);
        }
        else {
            return nullptr;
        }
    }

    /// Convenience function.
    const ValueT get_or_return_default(const KeyT& k) const
    {
        const ValueT* ret = try_get(k);
        if (ret) {
            return *ret;
        }
        else {
            return ValueT();
        }
    }

    // -----------------------------------------------------

    /// Returns a pair consisting of an iterator to the inserted element
    /// (or to the element that prevented the insertion)
    /// and a bool denoting whether the insertion took place.
    std::pair<iterator, bool> insert(const KeyT& key, const ValueT& value)
    {
        auto bucket = find_or_allocate(key);
        if (NEXT_BUCKET(_pairs, bucket) != State::INACTIVE) {
            return { iterator(this, bucket), false };
        }
        else {
            if (check_expand_need())
                bucket = find_main_bucket(key, true);

            NEW_KVALUE(key, value, bucket);
            _num_filled++;
            return { iterator(this, bucket), true };
        }
    }

    std::pair<iterator, bool> insert(KeyT&& key, ValueT&& value)
    {
        auto bucket = find_or_allocate(key);
        if (NEXT_BUCKET(_pairs, bucket) != State::INACTIVE) {
            return { iterator(this, bucket), false };
        }
        else {
            if (check_expand_need())
                bucket = find_main_bucket(key, true);

            NEW_KVALUE(std::move(key), std::move(value), bucket);
            _num_filled++;
            return { iterator(this, bucket), true };
        }
    }

    inline std::pair<iterator, bool> insert(const std::pair<KeyT, ValueT>& p)
    {
        return insert(p.first, p.second);
    }

    inline std::pair<iterator, bool> insert(std::pair<KeyT, ValueT>&& p)
    {
        return insert(std::move(p.first), std::move(p.second));
    }

    void insert(const_iterator begin, const_iterator end)
    {
        // TODO: reserve space exactly once.
        for (; begin != end; ++begin) {
            insert(begin->first, begin->second);
        }
    }

    /// Same as above, but contains(key) MUST be false
    unsigned int insert_unique(const KeyT& key, const ValueT& value)
    {
        check_expand_need();
        auto bucket = find_main_bucket(key, true);
        NEW_KVALUE(key, value, bucket);
        _num_filled++;
        return bucket;
    }

    unsigned int insert_unique(KeyT&& key, ValueT&& value)
    {
        check_expand_need();
        auto bucket = find_main_bucket(key, true);
        NEW_KVALUE(std::move(key), std::move(value), bucket);
        _num_filled++;
        return bucket;
    }

    inline unsigned int insert_unique(std::pair<KeyT, ValueT>&& p)
    {
        return insert_unique(std::move(p.first), std::move(p.second));
    }

    //not
    template <class... Args>
    inline std::pair<iterator, bool> emplace(Args&&... args)
    {
        return insert(std::forward<Args>(args)...);
    }

    void insert_or_assign(const KeyT& key, ValueT&& value)
    {
        check_expand_need();

        auto bucket = find_or_allocate(key);

        // Check if inserting a new value rather than overwriting an old entry
        if (NEXT_BUCKET(_pairs, bucket) != State::INACTIVE) {
            GET_VAL(_pairs, bucket) = value;
        }
        else {
            NEW_KVALUE(key, value, bucket);
            _num_filled++;
        }
    }

    /// Return the old value or ValueT() if it didn't exist.
    ValueT set_get(const KeyT& key, const ValueT& new_value)
    {
        check_expand_need();

        auto bucket = find_or_allocate(key);

        // Check if inserting a new value rather than overwriting an old entry
        if (NEXT_BUCKET(_pairs, bucket) != State::INACTIVE) {
            ValueT old_value = GET_VAL(_pairs, bucket);
            GET_VAL(_pairs, bucket) = new_value;
            return old_value;
        }
        else {
            NEW_KVALUE(key, new_value, bucket);
            _num_filled++;
            return ValueT();
        }
    }

    /// Like std::map<KeyT,ValueT>::operator[].
    ValueT& operator[](const KeyT& key)
    {
        auto bucket = find_or_allocate(key);
        /* Check if inserting a new value rather than overwriting an old entry */
        if (NEXT_BUCKET(_pairs, bucket) == State::INACTIVE) {
            if (check_expand_need())
                bucket = find_main_bucket(key, true);

            NEW_KVALUE(key, ValueT(), bucket);
            _num_filled++;
        }

        return GET_VAL(_pairs, bucket);
    }

    // -------------------------------------------------------

    /// Erase an element from the hash table.
    /// return false if element was not found
    bool erase(const KeyT& key)
    {
        auto bucket = erase_from_bucket(key);
        if (bucket == State::INACTIVE) {
            return false;
        }

        NEXT_BUCKET(_pairs, bucket) = State::INACTIVE;
        _pairs[bucket].~PairT();
        _num_filled -= 1;

#ifdef EMILIB_AUTO_SHRINK
        if (_num_buckets > 254 && _num_buckets > 4 * _num_filled)
            rehash(_num_filled / max_load_factor()  + 2);
#endif
        return true;
    }

    /// Erase an element typedef an iterator.
    /// Returns an iterator to the next element (or end()).
    iterator erase(iterator it)
    {
        auto bucket = it._bucket;
        bucket = erase_from_bucket(it->first);

        NEXT_BUCKET(_pairs, bucket) = State::INACTIVE;
        _pairs[bucket].~PairT();
        _num_filled -= 1;
        if (bucket == it._bucket)
            it++;

#ifdef EMILIB_AUTO_SHRINK
        if (_num_buckets > 254 && _num_buckets > 4 * _num_filled) {
            rehash(_num_filled * max_load_factor() + 2);
            it = begin();
        }
#endif
        return it;
    }

    /// Remove all elements, keeping full capacity.
    void clear()
    {
        for (unsigned int bucket = 0; bucket < _num_buckets; ++bucket) {
            if (NEXT_BUCKET(_pairs, bucket) != State::INACTIVE) {
                NEXT_BUCKET(_pairs, bucket) = State::INACTIVE;
                _pairs[bucket].~PairT();
                if ( _num_filled -- == 1)
                    break;
            }
        }
        _num_filled = 0;
    }

    /// Make room for this many elements
    bool reserve(unsigned int required_buckets)
    {
        if (required_buckets < _load_buckets)
            return false;
        else if (required_buckets < _num_filled)
            return false;

        rehash(required_buckets);
        return true;
    }

    /// Make room for this many elements
    void rehash(unsigned int required_buckets)
    {
        unsigned int num_buckets = 8;
        unsigned int shift = 3;
        while (num_buckets < required_buckets) { num_buckets *= 2;  shift ++;}
        if (num_buckets <= _num_buckets) {
            num_buckets = 2 * _num_buckets; shift ++ ;
        }

        assert(num_buckets * _max_load_factor + 2 >= _num_filled);
        auto new_pairs = (PairT*)malloc(num_buckets * sizeof(PairT));
        if (!new_pairs) {
            throw std::bad_alloc();
        }

        auto old_num_filled  = _num_filled;
        auto old_num_buckets = _num_buckets;
        auto old_pairs = _pairs;

        _num_filled  = 0;
        _num_buckets = num_buckets;
        _mask        = num_buckets - 1;
        _pairs       = new_pairs;
        _shift       = 64 - shift;

        for (unsigned int bucket = 0; bucket < num_buckets; bucket++)
            NEXT_BUCKET(_pairs, bucket) = State::INACTIVE;

        unsigned int collision = 0;
        //set all main bucket first
        for (unsigned int src_bucket = 0; src_bucket < old_num_buckets; src_bucket++) {
            if (NEXT_BUCKET(old_pairs, src_bucket) == State::INACTIVE) {
                continue;
            }

            const auto main_bucket = BUCKET(GET_KEY(old_pairs, src_bucket));
            auto& next_bucket = NEXT_BUCKET(_pairs, main_bucket);
            if (next_bucket == State::INACTIVE) {
                auto& src_pair = old_pairs[src_bucket];
                new(_pairs + main_bucket) PairT(std::move(src_pair)); src_pair.~PairT();
                next_bucket = main_bucket;
            }
            else {
                //move collision bucket to head
                NEXT_BUCKET(old_pairs, collision++) = (int)src_bucket;
            }
            _num_filled += 1;
            if (_num_filled >= old_num_filled)
                break ;
        }

        //reset all collisions bucket
        for (unsigned int src_bucket = 0; src_bucket < collision; src_bucket++) {
            const auto bucket = NEXT_BUCKET(old_pairs, src_bucket);
            auto new_bucket = find_main_bucket(GET_KEY(old_pairs, bucket), false);
            auto& src_pair = old_pairs[bucket];
            new(_pairs + new_bucket) PairT(std::move(src_pair)); src_pair.~PairT();
            //memcpy(&_pairs[new_bucket], &src_pair, sizeof(src_pair));
            NEXT_BUCKET(_pairs, new_bucket) = new_bucket;
        }

#ifdef LOG_REHASH
        if (_num_filled > 0) {
            static int ihashs = 0;
            char buff[255] = {0};
            sprintf(buff, "    _num_filled/packed/collision = %u/%zd/%.2lf%%", _num_filled, sizeof(PairT), (collision * 100.0 / _num_filled));
#if TAF_V3
            FDLOG() << "ORDER_INDEX = " << ORDER_INDEX << "|hash_nums = " << ihashs ++ << "|" <<__FUNCTION__ << "|" << buff << endl;
#else
            puts(buff);
#endif
        }
#endif

        _load_buckets = _num_buckets * max_load_factor();
        free(old_pairs);
        assert(old_num_filled == _num_filled);
    }

private:
    // Can we fit another element?
    inline bool check_expand_need()
    {
        return reserve(_num_filled);
    }

    int erase_from_bucket(const KeyT& key) const
    {
        const auto bucket = BUCKET(key);
        auto next_bucket = NEXT_BUCKET(_pairs, bucket);
        if (next_bucket == State::INACTIVE)
            return State::INACTIVE;
        else if (next_bucket == bucket) {
           if (GET_KEY(_pairs, bucket) == key)
               return bucket;
           return State::INACTIVE;
        }
        else if (GET_KEY(_pairs, bucket) == key) {
            const auto nbucket = NEXT_BUCKET(_pairs, next_bucket);
            GET_PVAL(_pairs, bucket).swap(GET_PVAL(_pairs, next_bucket));
            if (nbucket == next_bucket)
                NEXT_BUCKET(_pairs, bucket) = bucket;
            else
                NEXT_BUCKET(_pairs, bucket) = nbucket;
            return next_bucket;
        }

        auto prev_bucket = bucket;
        while (true) {
            const auto nbucket = NEXT_BUCKET(_pairs, next_bucket);
            if (GET_KEY(_pairs, next_bucket) == key) {
                if (nbucket == next_bucket)
                    NEXT_BUCKET(_pairs, prev_bucket) = prev_bucket;
                else
                    NEXT_BUCKET(_pairs, prev_bucket) = nbucket;
                return next_bucket;
            }

            if (nbucket == next_bucket)
                break;
            prev_bucket = next_bucket;
            next_bucket = nbucket;
        }

        return State::INACTIVE;
    }

    // Find the bucket with this key, or return State::INACTIVE
    int find_filled_bucket(const KeyT& key) const
    {
        const auto bucket = BUCKET(key);
        auto next_bucket = NEXT_BUCKET(_pairs, bucket);
        if (next_bucket == State::INACTIVE)
            return State::INACTIVE;
        else if (GET_KEY(_pairs, bucket) == key)
            return bucket;
        else if (next_bucket == bucket)
            return State::INACTIVE;

        //find next linked bucket
#if EMILIB_LRU_FIND
        auto prev_bucket = bucket;
#endif
        while (true) {
            if (GET_KEY(_pairs, next_bucket) == key) {
#if EMILIB_LRU_FIND
                  GET_PVAL(_pairs, next_bucket).swap(GET_PVAL(_pairs, prev_bucket));
                  return prev_bucket;
#else
                  return next_bucket;
#endif
            }
            const auto nbucket = NEXT_BUCKET(_pairs, next_bucket);
            if (nbucket == next_bucket)
                break;
#if EMILIB_LRU_FIND
            prev_bucket = next_bucket;
#endif
            next_bucket = nbucket;
        }

        return State::INACTIVE;
    }

    int reset_main_bucket(const int main_bucket, const int bucket)
    {
        const auto next_bucket = NEXT_BUCKET(_pairs, bucket);
        const auto new_bucket  = find_empty_bucket(bucket);
        const auto prev_bucket = find_prev_bucket(main_bucket, bucket);
        NEXT_BUCKET(_pairs, prev_bucket) = new_bucket;
        new(_pairs + new_bucket) PairT(std::move(_pairs[bucket])); _pairs[bucket].~PairT();
        if (next_bucket == bucket)
            NEXT_BUCKET(_pairs, new_bucket) = new_bucket;
        else
            NEXT_BUCKET(_pairs, new_bucket) = next_bucket;

         return new_bucket;
    }

    // Find the bucket with this key, or return a good empty bucket to place the key in.
    // In the latter case, the bucket is expected to be filled.
    int find_or_allocate(const KeyT& key)
    {
        const auto bucket = BUCKET(key);
        auto next_bucket = NEXT_BUCKET(_pairs, bucket);
        const auto& bucket_key = GET_KEY(_pairs, bucket);
        if (next_bucket == State::INACTIVE || bucket_key == key)
             return bucket;
        else if (next_bucket == bucket && bucket == BUCKET(bucket_key))
             return NEXT_BUCKET(_pairs, next_bucket) = find_empty_bucket(next_bucket);

        //find next linked bucket and check key
        while (true) {
            if (GET_KEY(_pairs, next_bucket) == key) {
#if EMILIB_LRU_SET
                GET_PVAL(_pairs, next_bucket).swap(GET_PVAL(_pairs, bucket));
                return bucket;
#else
                return next_bucket;
#endif
            }

            const auto nbucket = NEXT_BUCKET(_pairs, next_bucket);
            if (nbucket == next_bucket)
                break;
            next_bucket = nbucket;
        }

        //check current bucket_key is linked in main bucket
        const auto main_bucket = BUCKET(bucket_key);
        if (main_bucket == bucket) {
            //find a new empty and linked it to tail
            return NEXT_BUCKET(_pairs, next_bucket) = find_empty_bucket(next_bucket);
        }

        reset_main_bucket(main_bucket, bucket);
        NEXT_BUCKET(_pairs, bucket) = State::INACTIVE;
        return bucket;
    }

    // key is not in this map. Find a place to put it.
    int find_empty_bucket(int bucket_from)
    {
        constexpr int max_probe_length = (int)(128 / sizeof(PairT)) + 2;//cpu cache line 64 byte,2-3 cache line miss
        for (auto offset = 1; ; ++offset) {
            const auto bucket = (bucket_from + offset) & _mask;
            if (NEXT_BUCKET(_pairs, bucket) == State::INACTIVE)
                return bucket;
            else if (offset > max_probe_length) {
                const auto bucket1 = (bucket + offset * offset + 0) & _mask;
//                const auto bucket1 = (bucket + (offset + 1) * offset / 2) & _mask;
                if (NEXT_BUCKET(_pairs, bucket1) == State::INACTIVE)
                    return bucket1;

                const auto bucket2 = (bucket1 + 1) & _mask;
                if (NEXT_BUCKET(_pairs, bucket2) == State::INACTIVE)
                    return bucket2;
#if Q3S
                const auto bucket3 = (bucket1 - 1) & _mask;
                if (NEXT_BUCKET(_pairs, bucket3) == State::INACTIVE)
                    return bucket3;
#endif
            }
        }
    }

    int find_prev_bucket(int main_bucket, const int bucket)
    {
        while (true) {
            const auto next_bucket = NEXT_BUCKET(_pairs, main_bucket);
            if (next_bucket == bucket || next_bucket == main_bucket)
                return main_bucket;
            main_bucket = next_bucket;
        }
    }

    int find_main_bucket(const KeyT& key, bool check_main)
    {
        const auto bucket = BUCKET(key);
        auto next_bucket = NEXT_BUCKET(_pairs, bucket);
        const auto& bucket_key = GET_KEY(_pairs, bucket);
        if (next_bucket == State::INACTIVE)
            return bucket;
        else if (next_bucket == bucket && (BUCKET(bucket_key)) == bucket)
             return NEXT_BUCKET(_pairs, next_bucket) = find_empty_bucket(next_bucket);

        if (check_main) {
            const auto main_bucket = BUCKET(bucket_key);
            //check current bucket_key is linked in main bucket
            if (main_bucket != bucket) {
                reset_main_bucket(main_bucket, bucket);
                NEXT_BUCKET(_pairs, bucket) = State::INACTIVE;
                return bucket;
            }
        }

        //find a new empty and linked it to tail
        int last_bucket = next_bucket;
        while (true) {
            const auto nbucket = NEXT_BUCKET(_pairs, next_bucket);
            if (nbucket == next_bucket) {
                last_bucket = nbucket;
                break;
            }
            next_bucket = nbucket;
        }

        return NEXT_BUCKET(_pairs, last_bucket) = find_empty_bucket(last_bucket);
    }

    inline int hash_key(const KeyT& key) const
    {
        return (_hasher(key) * 11400714819323198485ull) >> _shift;
    }
private:
    HashT   _hasher;
    PairT*  _pairs;
//    EqT     _eq;

    unsigned int  _num_buckets;
    unsigned int  _num_filled;
    unsigned int  _mask;  // _num_buckets minus one
    unsigned char _shift;  // _num_buckets minus one

    float         _max_load_factor;
    unsigned int  _load_buckets;
};

} // namespace emilib

#if __cplusplus > 199711
template <class Key, class Val> using emihash = emilib1::HashMap<Key, Val, std::hash<Key>>;
#endif

