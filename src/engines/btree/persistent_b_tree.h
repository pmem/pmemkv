/*
 * Copyright 2017-2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PERSISTENT_B_TREE
#define PERSISTENT_B_TREE

#include <vector>
#include <algorithm>
#include <numeric>
#include <memory>
#include <utility>
#include <functional>

#include <cassert>

#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/pool.hpp>


namespace persistent {

namespace internal {
	using namespace pmem::obj;

    class node_t {
        uint64_t _level;
    public:
        node_t( uint64_t level = 0 ) : _level( level ) { }

        bool leaf() const {
            return  _level == 0ull;
        }

        uint64_t level() const {
            return _level;
        }
    };

    template <typename TLeafNode, bool is_const>
    class node_iterator {
        typedef TLeafNode leaf_node_type;
        typedef typename std::conditional<is_const, const leaf_node_type*, leaf_node_type*>::type leaf_node_ptr;
        friend class node_iterator<leaf_node_type, true>;

    public:
        typedef typename leaf_node_type::value_type value_type;
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = ptrdiff_t;
        typedef typename std::conditional<is_const, const value_type&, value_type&>::type reference;
        typedef typename std::conditional<is_const, const value_type*, value_type*>::type pointer;

        node_iterator() : node( nullptr ), position( 0 ) {
        }

        node_iterator(leaf_node_ptr node_ptr, size_t p) : node(node_ptr), position(p) {
        }

        node_iterator(const node_iterator& other) : node(other.node), position(other.position) {
        }

        template <typename T = void, typename = typename std::enable_if<is_const, T>::type>
        node_iterator(const node_iterator<leaf_node_type, false>& other) : node(other.node), position(other.position) {
        }

        node_iterator& operator++() {
            ++position;
            return *this;
        }

        node_iterator operator++( int ) {
            node_iterator tmp = *this;
            ++*this;
            return tmp;
        }

        node_iterator& operator--() {
            assert( position > 0 );
            --position;
            return *this;
        }

        node_iterator operator--( int ) {
            node_iterator tmp = *this;
            --*this;
            return tmp;
        }

        node_iterator operator+( size_t off ) const {
            return node_iterator( node, position + off );
        }

        node_iterator operator+=( difference_type off ) {
            position += off;
            return *this;
        }

        node_iterator operator-( difference_type off ) const {
            assert( node != nullptr );
            assert( position >= off );
            return node_iterator( node, position - off );
        }

        difference_type operator-( const node_iterator& other ) const {
            assert( node != nullptr );
            assert( other.node != nullptr );
            assert( node == other.node );
            return position - other.position;
        }

        bool operator==( const node_iterator& other ) const {
            return node == other.node && position == other.position;
        }

        bool operator!=( const node_iterator& other ) const {
            return !(*this == other);
        }

        bool operator<( const node_iterator& other ) const {
            assert( node != nullptr );
            assert( other.node != nullptr );
            assert( node == other.node );
            return position < other.position;
        }

        bool operator>( const node_iterator& other ) const {
            assert( node != nullptr );
            assert( other.node != nullptr );
            assert( node == other.node );
            return position > other.position;
        }

        reference operator*() const {
            assert( node != nullptr );
            return (*node)[position];
        }

        pointer operator->() const {
            return &**this;
        }

    private:
        leaf_node_ptr node;
        size_t position;
    };

    template <typename TKey, typename TValue, uint64_t number_entrys_slots>
    class leaf_node_t : public node_t {
        /**
        * Array of indexes.
        */
        struct leaf_entries_t {
            leaf_entries_t() : _size(0) {
                for (uint64_t i = 0; i < number_entrys_slots; ++i) {
                    idxs[i] = i;
                }
            }

            uint64_t idxs[number_entrys_slots];
            size_t _size;
        };
    public:
        typedef TKey                key_type;
        typedef TValue              mapped_type;
        typedef std::pair<key_type, mapped_type>  value_type;
        typedef value_type&         reference;
        typedef const value_type&   const_reference;
        typedef value_type*         pointer;
        typedef const value_type*   const_pointer;

        typedef node_iterator<leaf_node_t, false> iterator;
        typedef node_iterator<leaf_node_t, true> const_iterator;

        leaf_node_t( uint64_t e ) : node_t(), epoch( e ), consistent_id( 0 ), p_consistent_id( 0 ) {
			assert(std::is_sorted(begin(), end(), [](const_reference a, const_reference b) { return a.first < b.first; }));
		}

        leaf_node_t( uint64_t e, const_reference entry ) : node_t(), epoch( e ), consistent_id( 0 ), p_consistent_id( 0 ) {
            entries[0] = entry;
            consistent()->idxs[0] = 0;
            consistent()->_size = 1;
            assert( std::is_sorted( begin(), end(), []( const_reference a, const_reference b ) { return a.first < b.first; } ) );
        }

        leaf_node_t( uint64_t e, const_iterator first, const_iterator last, const persistent_ptr<leaf_node_t>& _prev, const persistent_ptr<leaf_node_t>& _next ) : node_t(), epoch( e ), consistent_id( 0 ), prev(_prev), next(_next), p_consistent_id( 0 ) {
            copy(first, last);
            assert( size() == std::distance(first, last ) );
			assert(std::is_sorted(begin(), end(), [](const_reference a, const_reference b) { return a.first < b.first; }));
        }

        leaf_node_t( uint64_t e, const_reference entry, const_iterator first, const_iterator last, const persistent_ptr<leaf_node_t>& _prev, const persistent_ptr<leaf_node_t>& _next ) : node_t(), epoch( e ), consistent_id( 0 ), prev( _prev ), next( _next ), p_consistent_id( 0 ) {
            copy_insert( entry, first, last );
            assert( size() == std::distance( first, last ) + 1 );
            assert( std::binary_search( begin(), end(), entry, []( const_reference a, const_reference b ) { return a.first < b.first; }) );
			assert(std::is_sorted(begin(), end(), [](const_reference a, const_reference b) { return a.first < b.first; }));
        }

        std::pair<iterator, bool> insert( pool_base& pop, const_reference entry ) {
            return insert( pop, entry, this->begin(), this->end() );
        }

        iterator find( const key_type& key ) {
			assert(std::is_sorted(begin(), end(), [](const_reference a, const_reference b) { return a.first < b.first; }));
            iterator it = std::lower_bound( begin(), end(), key, [] ( const_reference entry, const TKey& key ) {
                return entry.first < key;
            } );
            if ( it == end() || it->first == key )
                return it;
            else
                return end();
        }

        const_iterator find( const key_type& key ) const {
			assert(std::is_sorted(begin(), end(), [](const_reference a, const_reference b) { return a.first < b.first; }));
            const_iterator it = std::lower_bound( begin(), end(), key, [] ( const_reference entry, const TKey& key ) {
                return entry.first < key;
            } );
            if ( it == end() || it->first == key )
                return it;
            else
                return end();
        }

        size_t erase(pool_base& pop, const key_type& key) {
            assert(std::is_sorted(begin(), end(), [](const_reference a, const_reference b) { return a.first < b.first; }));
            iterator it = find(key);
            if (it == end())
                return size_t(0);
            internal_erase(pop, it);
            return size_t(1);
        }

        /**
        * Return begin iterator on an array of correct indexs.
        */
        iterator begin() {
            return iterator( this, 0);
        }

        /**
        * Return const_iterator to the beginning.
        */
        const_iterator begin() const {
            return const_iterator( this, 0 );
        }

        /**
        * Return end iterator on an array of indexs.
        */
        iterator end() {
            return iterator( this, consistent()->_size );
        }

        /**
        * Return const_iterator to the end.
        */
        const_iterator end() const {
            return const_iterator( this, consistent()->_size );
        }

        /**
        * Return the size of the array of entries (key/value).
        */
        size_t size() const {
            return consistent()->_size;
        }

        bool full() const {
            return size() == number_entrys_slots;
        }

        const_reference back() const {
            return entries[consistent()->idxs[consistent()->_size - 1]];
        }

        reference at( size_t pos ) {
            if ( size() <= pos ) {
				throw std::out_of_range("Accessing incorrect element in leaf node");
            }
            return entries[consistent()->idxs[pos]];
        }

        const_reference at( size_t pos ) const {
            if ( size() <= pos ) {
				throw std::out_of_range("Accessing incorrect element in leaf node");
            }
            return entries[consistent()->idxs[pos]];
        }

        reference operator[]( size_t pos ) {
            return entries[consistent()->idxs[pos]];
        }

        const_reference operator[]( size_t pos ) const {
            return entries[consistent()->idxs[pos]];
        }

		const persistent_ptr<leaf_node_t>& get_next() const {
			return this->next;
		}

        void set_next( const persistent_ptr<leaf_node_t>& n ) {
            this->next = n;
        }

		const persistent_ptr<leaf_node_t>& get_prev() const {
			return this->prev;
		}

        void set_prev( const persistent_ptr<leaf_node_t>& p ) {
            this->prev = p;
        }

        void check_consistency( uint64_t global_epoch ) {
            if ( global_epoch != epoch ) {
                consistent_id = p_consistent_id;
                epoch = global_epoch;
            }
        }

    private:
        uint64_t epoch;
        uint32_t consistent_id;
        persistent_ptr<leaf_node_t> prev;
        persistent_ptr<leaf_node_t> next;
        char padding[64];
        value_type entries[number_entrys_slots];
        leaf_entries_t v[2];
        char padding1[64];
        uint32_t p_consistent_id;

        leaf_entries_t* consistent() {
            assert( consistent_id < 2 );
            return v + consistent_id;
        }

        const leaf_entries_t* consistent() const {
            assert( consistent_id < 2 );
            return v + consistent_id;
        }

        leaf_entries_t* working_copy() {
            assert( consistent_id < 2 );
            uint32_t working_id = 1 - consistent_id;
            return v + working_id;
        }

        void switch_consistent( pool_base &pop ) {
            p_consistent_id = consistent_id = 1 - consistent_id;
            // TODO: need to check if it make sense to use non-temporal store
            pop.persist( &p_consistent_id, sizeof( p_consistent_id ) );
        }

        /**
        * Insert new 'entry' in array of entries, update idxs.
        */
        std::pair<iterator, bool> insert( pool_base& pop, const_reference entry, iterator begin, iterator end ) {
            assert( !full() );

            iterator hint = std::lower_bound( begin, end, entry.first, [&] ( const_reference entry, const key_type& key ) {
                return entry.first < key;
            } );

            if (hint != end && hint->first == entry.first) {
                return std::pair<iterator, bool>( hint, false );
            }
            
            size_t insert_pos = get_insert_idx();
            assert(std::none_of(consistent()->idxs, consistent()->idxs + size(), [insert_pos](uint64_t idx) { return insert_pos == idx; }));
            // insert an entry to the end
            entries[insert_pos] = entry;
            pop.flush( &(entries[insert_pos]), sizeof( entries[insert_pos] ) );
            // update tmp idxs
            size_t position = insert_idx( pop, insert_pos, hint );
            // update consistent
            switch_consistent( pop );

            assert(std::is_sorted(this->begin(), this->end(), [](const_reference a, const_reference b) { return a.first < b.first; }));

            return std::pair<iterator, bool>( iterator( this, position ), true );
        }

        size_t get_insert_idx() const {
            const leaf_entries_t* c = consistent();
            return c->idxs[c->_size];
        }

        size_t insert_idx( pool_base& pop, uint64_t new_entry_idx, iterator hint ) {
            size_t size = this->size();
            leaf_entries_t* tmp = working_copy();
            auto in_begin = consistent()->idxs;
            auto in_end = in_begin + size;
            auto partition_point = in_begin + std::distance( this->begin(), hint );
            auto out_begin = tmp->idxs;
            auto insert_pos = std::copy( in_begin, partition_point, out_begin );
            *insert_pos = new_entry_idx;
            std::copy( partition_point, in_end, insert_pos + 1 );
            tmp->_size = size + 1;
#if 0
            pop.flush( tmp->idxs, sizeof(tmp->idxs[0])*tmp->_size );
            pop.persist( &(tmp->_size), sizeof(tmp->_size) );
#else
            pop.persist( tmp, sizeof(leaf_entries_t) );
#endif

            return std::distance( out_begin, insert_pos );
        }

        void remove_idx(pool_base& pop, iterator it) {
            size_t size = this->size();
            leaf_entries_t* tmp = working_copy();
            auto in_begin = consistent()->idxs;
            auto in_end = in_begin + size;
            auto partition_point = in_begin + std::distance(this->begin(), it);
            auto out = tmp->idxs;
            out = std::copy(in_begin, partition_point, out);
            out = std::copy(partition_point + 1, in_end, out);
            *out = *partition_point;
            tmp->_size = size - 1;

            pop.persist(tmp, sizeof(leaf_entries_t));
        }

        /**
        * Copy entries from another node in the range of [first, last) and insert new entry.
        */
        void copy_insert( const_reference entry, const_iterator first, const_iterator last ) {
            assert( std::distance( first, last ) < number_entrys_slots );

            auto d_last = std::merge( first, last, &entry, &entry + 1, entries, []( const_reference a, const_reference b ) { return a.first < b.first; } );
            consistent()->_size = std::distance( entries, d_last );
            std::iota( consistent()->idxs, consistent()->idxs + consistent()->_size, 0 );
        }

        /**
        * Copy entries from another node in the range of [first, last).
        */
        void copy( const_iterator first, const_iterator last ) {
            assert( std::distance( first, last ) <= number_entrys_slots );

            auto d_last = std::copy(first, last, entries);
            consistent()->_size = std::distance( entries, d_last );
            std::iota( consistent()->idxs, consistent()->idxs + consistent()->_size, 0 );
        }

        /**
         * Remove element pointed by iterator.
         */
        void internal_erase(pool_base& pop, iterator it) {
            // update tmp idxs
            remove_idx(pop, it);
            // update consistent
            switch_consistent(pop);

            assert(std::is_sorted(this->begin(), this->end(), [](const_reference a, const_reference b) { return a.first < b.first; }));
        }
    }; // class leaf_node_t

    template <typename TKey, uint64_t number_entrys_slots>
    class inner_node_t : public node_t {
        typedef inner_node_t<TKey, number_entrys_slots> self_type;
    public:
        typedef TKey key_type;
        typedef key_type value_type; // Inner node stores only keys
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef node_iterator<self_type, false> iterator;
        typedef node_iterator<self_type, true> const_iterator;

    private:
        const static size_t number_children_slots = number_entrys_slots + 1;

        struct inner_entries_t {
            value_type entries[number_entrys_slots];
            persistent_ptr<node_t> children[number_children_slots];
            size_t _size = 0;
        };

        inner_entries_t v[2];
        uint32_t consistent_id;

        inner_entries_t* consistent() {
            assert( consistent_id < 2 );
            return v + consistent_id;
        }

        inner_entries_t* working_copy() {
            assert( consistent_id < 2 );
            uint32_t working_id = 1 - consistent_id;
            return v + working_id;
        }

        const inner_entries_t* consistent() const {
            assert( consistent_id < 2 );
            return v + consistent_id;
        }

        void switch_consistent( pool_base &pop ) {
            consistent_id = 1 - consistent_id;
            pop.persist( &consistent_id, sizeof( consistent_id ) );
        }

    public:
        inner_node_t( size_t level, const value_type& key, const persistent_ptr<node_t>& child_0, const persistent_ptr<node_t>& child_1 ) : node_t( level ), consistent_id( 0 ) {
            inner_entries_t* consist = consistent();
            consist->entries[0] = key;
            consist->_size++;
            consist->children[0] = child_0;
            consist->children[1] = child_1;
            assert(std::is_sorted(begin(), end()));
        }

        inner_node_t( size_t level, const_iterator first, const_iterator last, const inner_node_t* src ) : node_t( level ), consistent_id( 0 ) {
            auto o_last = std::copy( first, last, consistent()->entries );
            consistent()->_size = std::distance( consistent()->entries, o_last );
            auto in_cbegin = std::next(src->consistent()->children, std::distance( src->begin(), first ));
            auto in_cend = std::next(in_cbegin, consistent()->_size + 1);
            auto o_clast = std::copy( in_cbegin, in_cend, consistent()->children );
            assert(std::is_sorted(begin(), end()));
        }

        /**
         * Update splitted node with pair of new nodes
         */
        void update_splitted_child( pool_base& pop, const_reference entry, persistent_ptr<node_t>& lnode, persistent_ptr<node_t>& rnode, const persistent_ptr<node_t>& splitted_node ) {
            assert( !full() );
            iterator partition_point = std::lower_bound( this->begin(), this->end(), entry );

            // Insert new key
            auto in_entries_begin = consistent()->entries;
            auto in_entries_end = std::next(in_entries_begin, consistent()->_size);
            auto in_entries_splitted = std::next( in_entries_begin, std::distance( this->begin(), partition_point ) );

            auto out_entries_begin = working_copy()->entries;

            auto insert_pos = std::copy( in_entries_begin, in_entries_splitted, out_entries_begin );
            *insert_pos = entry;
            auto out_entries_end = std::copy( in_entries_splitted, in_entries_end, ++insert_pos );
            working_copy()->_size = std::distance( out_entries_begin, out_entries_end );
            pop.flush( working_copy()->entries, sizeof( working_copy()->entries[0] )*working_copy()->_size );
            pop.flush( &(working_copy()->_size), sizeof( working_copy()->_size ) );
            
            // Update children
            auto in_children_begin = consistent()->children;
            auto in_children_splitted = std::next( in_children_begin, std::distance( this->begin(), partition_point ) );
            auto in_children_end = std::next( in_children_begin, consistent()->_size + 1 );
            auto out_children_begin = working_copy()->children;
            assert( *in_children_splitted == splitted_node );
            auto out_insert_pos = std::copy( in_children_begin, in_children_splitted, out_children_begin );
            *out_insert_pos++ = lnode;
            *out_insert_pos++ = rnode;
            auto out_children_end = std::copy( ++in_children_splitted, in_children_end, out_insert_pos );
            pop.flush( working_copy()->children, sizeof( working_copy()->children[0] )*std::distance(out_children_begin, out_children_end));

            switch_consistent( pop );
            assert( std::is_sorted( this->begin(), this->end() ) );
        }

        const persistent_ptr<node_t>& get_child( const_reference key ) const {
            auto it = std::lower_bound( this->begin(), this->end(), key );
            return get_left_child(it);
        }

        const  persistent_ptr<node_t>& get_left_child(const_iterator it) const {
            size_t child_pos = std::distance(this->begin(), it);
            return this->consistent()->children[child_pos];
        }

        const persistent_ptr<node_t>& get_right_child(const_iterator it) const {
            size_t child_pos = std::distance(this->begin(), it) + 1;
            return this->consistent()->children[child_pos];
        }

        bool full() const {
            return this->size() == number_entrys_slots;
        }

        /**
        * Return begin iterator on an array of keys.
        */
        iterator begin() {
            return iterator(this, 0);
        }

        const_iterator begin() const {
            return const_iterator(this, 0);
        }

        /**
        * Return end iterator on an array of keys.
        */
        iterator end() {
            return begin() + this->size();
        }

        const_iterator end() const {
            return begin() + this->size();
        }

        /**
        * Return the size of the array of keys.
        */
        size_t size() const {
            return consistent()->_size;
        }

        const_reference back() const {
            return consistent()->entries[this->size() - 1];
        }

        reference at(size_t pos) {
            if (size() <= pos) {
                throw std::out_of_range("Accessing incorrect element in inner node");
            }
            return consistent()->entries[pos];
        }

        const_reference at(size_t pos) const {
            if (size() <= pos) {
                throw std::out_of_range("Accessing incorrect element inner node");
            }
            return consistent()->entries[pos];
        }

        reference operator[](size_t pos) {
            return consistent()->entries[pos];
        }

        const_reference operator[](size_t pos) const {
            return consistent()->entries[pos];
        }
    }; // class inner_node_t

    template<typename LeafNode, bool is_const>
    class b_tree_iterator {
    private:
        typedef LeafNode leaf_node_type;
        typedef typename std::conditional<is_const, const leaf_node_type*, leaf_node_type*>::type leaf_node_ptr;
        typedef typename std::conditional<is_const, typename leaf_node_type::const_iterator, typename leaf_node_type::iterator>::type leaf_iterator;
        friend class b_tree_iterator<leaf_node_type, true>;
	public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        typedef typename leaf_iterator::value_type value_type;
        typedef typename leaf_iterator::reference reference;
        typedef typename leaf_iterator::pointer pointer;

        b_tree_iterator( std::nullptr_t ) : current_node(nullptr), leaf_it() {}

        b_tree_iterator( leaf_node_ptr node ) : current_node( node ), leaf_it( node->begin() ) {}
	
        b_tree_iterator( leaf_node_ptr node, leaf_iterator _leaf_it ) : current_node( node ), leaf_it( _leaf_it ) {}

        b_tree_iterator( const b_tree_iterator& other ) : current_node( other.current_node ), leaf_it( other.leaf_it ) {}

        template <typename T = void, typename = typename std::enable_if<is_const, T>::type>
        b_tree_iterator(const b_tree_iterator<leaf_node_type, false>& other) : current_node(other.current_node), leaf_it(other.leaf_it) {}

		b_tree_iterator& operator=(const b_tree_iterator& other) {
			current_node = other.current_node;
			leaf_it = other.leaf_it;
			return *this;
		}

        b_tree_iterator& operator++() {
            ++leaf_it;
            if ( leaf_it == current_node->end() ) {
                leaf_node_ptr tmp = current_node->get_next().get();
                if ( tmp ) {
                    current_node = tmp;
                    leaf_it = current_node->begin();
                }
            }
            return *this;
        }

		b_tree_iterator operator++(int) {
			b_tree_iterator tmp = *this;
			++*this;
			return tmp;
		}

        b_tree_iterator& operator--() {
            if ( leaf_it == current_node->begin() ) {
                leaf_node_ptr tmp = current_node->get_prev().get();
                if ( tmp ) {
                    current_node = tmp;
                    leaf_it = current_node->last();
                }
            }
            else {
                --leaf_it;
            }
            return *this;
        }

		b_tree_iterator operator--(int) {
			b_tree_iterator tmp = *this;
			--*this;
			return tmp;
		}

        bool operator==( const b_tree_iterator &other ) { return current_node == other.current_node && leaf_it == other.leaf_it; }

        bool operator!=( const b_tree_iterator &other ) { return !(*this == other); }

        reference operator*() const {
            return *(leaf_it);
        }

        pointer operator->() const {
            return &**this;
        }

	private:
		leaf_node_ptr current_node;
		leaf_iterator leaf_it;
    }; // class b_tree_iterator

    template<typename TKey, typename TValue, size_t degree>
    class b_tree_base {
        const static size_t number_entrys_slots = degree - 1;
        const static size_t number_children_slots = degree;
        typedef leaf_node_t<TKey, TValue, number_entrys_slots> leaf_node_type;
        typedef inner_node_t<TKey, number_entrys_slots> inner_node_type;
        typedef persistent_ptr<node_t> node_persistent_ptr;
        typedef persistent_ptr<leaf_node_type> leaf_node_persistent_ptr;
        typedef persistent_ptr<inner_node_type> inner_node_persistent_ptr;

    public:
        typedef b_tree_base<TKey, TValue, degree> self_type;
        typedef typename leaf_node_type::value_type value_type;
		typedef typename leaf_node_type::key_type key_type;
		typedef typename leaf_node_type::mapped_type mapped_type;
        typedef typename leaf_node_type::reference reference;
        typedef typename leaf_node_type::const_reference const_reference;
        typedef typename leaf_node_type::pointer pointer;
        typedef typename leaf_node_type::const_pointer const_pointer;
        
        typedef b_tree_iterator<leaf_node_type, false> iterator;
        typedef b_tree_iterator<leaf_node_type, true> const_iterator;

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    private:

        static const key_type& get_last_key( const node_persistent_ptr& node ) {
            if (node->leaf()) {
                return cast_leaf( node.get() )->back().first;
            }
            else {
                return cast_inner( node.get() )->back();
            }
        }

        uint64_t epoch;

        persistent_ptr<node_t> root;

        /**
         * Splitting node.
         */
        persistent_ptr<node_t> split_node;

        /**
         * Left and right node during split.
         */
        persistent_ptr<node_t> left_child;

        persistent_ptr<node_t> right_child;

        void create_new_root(pool_base&, const key_type&, node_persistent_ptr&, node_persistent_ptr& );

        std::pair<iterator, bool> insert_descend( pool_base&, const_reference );

        typename inner_node_type::const_iterator split_half( pool_base& pop, persistent_ptr<node_t>& node, persistent_ptr<node_t>& left, persistent_ptr<node_t>& right ) {
            assert( split_node == node );
            inner_node_type* inner = cast_inner( node ).get();

            typename inner_node_type::const_iterator middle = inner->begin() + inner->size() / 2;
            typename inner_node_type::const_iterator l_begin = inner->begin();
            typename inner_node_type::const_iterator l_end = middle;
            typename inner_node_type::const_iterator r_begin = middle + 1;
            typename inner_node_type::const_iterator r_end = inner->end();
            allocate_inner( pop, left, inner->level(), l_begin, l_end, inner );
            allocate_inner( pop, right, inner->level(), r_begin, r_end, inner );

            return middle;
        }

        void split_inner_node( pool_base &pop, const node_persistent_ptr &src_node, inner_node_type* parent_node, node_persistent_ptr &left, node_persistent_ptr &right ) {
            assert( split_node == nullptr );
            assignment( pop, split_node, src_node );
            typename inner_node_type::const_iterator partition_point = split_half( pop, split_node, left, right );
            assert( partition_point != cast_inner( split_node )->end() );
            if (parent_node) {
                parent_node->update_splitted_child( pop, *partition_point, left, right, split_node );
            }
            else { // Root node is split
                assert( root == split_node );
                create_new_root( pop, *partition_point, left, right );
            }
            deallocate( split_node );
        }

        iterator split_leaf_node(pool_base&, inner_node_type*, persistent_ptr<node_t>&, const_reference, persistent_ptr<node_t>&, persistent_ptr<node_t>&);

        static bool is_left_node( const leaf_node_type* src_node, const leaf_node_type* lnode ) {
            assert( src_node );
            assert( lnode );
            typename leaf_node_type::const_iterator middle = src_node->begin() + src_node->size() / 2;

            return std::includes( lnode->begin(), lnode->end(), src_node->begin(), middle );
        }

        static bool is_right_node( const leaf_node_type* src_node, const leaf_node_type* rnode ) {
            assert( src_node );
            assert( rnode );
            typename leaf_node_type::const_iterator middle = src_node->begin() + src_node->size() / 2;
            
            return std::includes( rnode->begin(), rnode->end(), middle, src_node->end() );
        }

        void repair_leaf_split( pool_base& pop ) {
            assert( root != nullptr );
            assert( split_node != nullptr );
            assert( split_node->leaf() );

            const key_type &key = get_last_key( split_node );
            path_type path;

            leaf_node_persistent_ptr found_node = find_leaf_to_insert( key, path );
            assert( path[0] == root );

            if (split_node == found_node) { // Split not completted
                const leaf_node_type* split_leaf = cast_leaf( split_node ).get();
                leaf_node_type* lnode = cast_leaf( left_child ).get();
                leaf_node_type* rnode = cast_leaf( right_child ).get();
                
                typename leaf_node_type::const_iterator middle = split_leaf->begin() + split_leaf->size() / 2;
                if (left_child && is_left_node( split_leaf, lnode )) {
                    if (right_child && is_right_node( split_leaf, rnode )) { // Both children were allcoated during split before crash
                        inner_node_type* parent_node = path.empty() ? nullptr : path.back().get();

                        lnode->set_next( cast_leaf( right_child ) );
                        pop.persist( lnode->get_next() );

                        correct_leaf_node_links( pop, split_node, left_child, right_child );

                        if (parent_node) {
                            parent_node->update_splitted_child( pop, lnode->back().first, left_child, right_child, split_node );
                        }
                        else {
                            create_new_root( pop, lnode->back().first, left_child, right_child );
                        }
                    }
                    else { // Only left child was allocated during split before crash
                        deallocate( left_child );
                    }
                }
            }
            else { // split_node was replaced by two new (left and right) nodes. Need to deallocate split_node
                deallocate( split_node );
            }
            split_node = nullptr;
        }

        void repair_inner_split(pool_base& pop) {
            assert( root != nullptr );
            assert( !root->leaf() );
            assert( split_node != nullptr );
            assert( !split_node->leaf() );

            const key_type &key = get_last_key( split_node );
            path_type path;

            find_leaf_to_insert( key, path );
            assert( path[0] == root );
            uint64_t root_level = path[0]->level();
            uint64_t split_level = split_node->level();
            assert( split_level <= root_level );
            
            if (split_node == path[root_level - split_level]) { // Split not completted
                // we could simply roll back
                const inner_node_type* inner = cast_inner( split_node ).get();
                typename inner_node_type::const_iterator middle = inner->begin() + inner->size() / 2;
                if (left_child && !(left_child->leaf()) && std::equal( inner->begin(), middle, cast_inner( left_child )->begin() )) {
                    deallocate( left_child );
                }
                if (right_child && !(right_child->leaf()) && std::equal( middle + 1, inner->end(), cast_inner( right_child )->begin() )) {
                    deallocate( right_child );
                }
            }
            else { // split_node was replaced by two new (left and right) nodes. Need to deallocate split_node
                deallocate( split_node );
            }
            split_node = nullptr;
        }

        void correct_leaf_node_links(pool_base&, persistent_ptr<node_t>&, persistent_ptr<node_t>&, persistent_ptr<node_t>&);

        void assignment( pool_base& pop, persistent_ptr<node_t>& lhs, const persistent_ptr<node_t>& rhs ) {
            //lhs.raw_ptr()->off = rhs.raw_ptr()->off;
            lhs = rhs;
            pop.persist( lhs );
        }

        leaf_node_type* find_leaf_node( const key_type& key ) const {
            if (root == nullptr)
                return nullptr;

            node_persistent_ptr node = root;
            while (!node->leaf()) {
                node = cast_inner( node )->get_child( key );
            }
            leaf_node_type* leaf = cast_leaf( node ).get();
            leaf->check_consistency( epoch );
            return leaf;
        }

        // TODO: merge with previous method
        typedef std::vector<inner_node_persistent_ptr> path_type;
        leaf_node_persistent_ptr find_leaf_to_insert( const key_type& key, path_type& path ) const {
            assert( root != nullptr );
            node_persistent_ptr node = root;
            while (!node->leaf()) {
                path.push_back( cast_inner(node) );

                node = cast_inner( node )->get_child( key );
            }
            leaf_node_persistent_ptr leaf = cast_leaf( node );
            leaf->check_consistency( epoch );
            return leaf;
        }

        typename path_type::const_iterator find_full_node( const path_type& path ) {
            auto i = path.end() - 1;
            for (; i > path.begin(); --i) {
                if (!(*i)->full())
                    return i;
            }
            return i;
        }

        leaf_node_type* leftmost_leaf() const {
            if (root == nullptr)
                return nullptr;

            node_persistent_ptr node = root;
            while (!node->leaf()) {
                inner_node_type* inner_node = cast_inner(node).get();
                node = inner_node->get_left_child(inner_node->begin());
            }
            leaf_node_type* leaf = cast_leaf(node).get();
            leaf->check_consistency(epoch);
            return leaf;
        }

        leaf_node_type* rightmost_leaf() const {
            if (root == nullptr)
                return nullptr;

            node_persistent_ptr node = root;
            while (!node->leaf()) {
                inner_node_type* inner_node = cast_inner(node).get();
                node = inner_node->get_left_child(inner_node->end());
            }
            leaf_node_type* leaf = cast_leaf(node).get();
            leaf->check_consistency(epoch);
            return leaf;
        }

        static persistent_ptr<inner_node_type>& cast_inner(persistent_ptr<node_t>& node) {
            return reinterpret_cast<persistent_ptr<inner_node_type>&>(node);
        }

        static inner_node_type* cast_inner( node_t* node ) {
            return static_cast<inner_node_type*>(node);
        }

        static persistent_ptr<leaf_node_type>& cast_leaf(persistent_ptr<node_t>& node) {
            return reinterpret_cast<persistent_ptr<leaf_node_type>&>(node);
        }

        static leaf_node_type* cast_leaf( node_t* node ) {
            return static_cast<leaf_node_type*>(node);
        }

        template<typename... Args>
        inline persistent_ptr<inner_node_type> allocate_inner(pool_base& pop, persistent_ptr<node_t>& node, Args&& ...args) {
            make_persistent_atomic<inner_node_type>(pop, cast_inner(node), args...);
            return cast_inner(node);
        }

        template<typename... Args>
        inline persistent_ptr<leaf_node_type> allocate_leaf(pool_base& pop, persistent_ptr<node_t>& node, Args&& ...args) {
            make_persistent_atomic<leaf_node_type>(pop, cast_leaf(node), epoch, args...);
            return cast_leaf(node);
        }

        inline void deallocate(persistent_ptr<node_t>& node) {
            if (node == nullptr)
                return;

            pool_base pop = get_pool_base();
            transaction::manual tx( pop );
            if (node->leaf()) {
                deallocate_leaf( cast_leaf( node ) );
            } else {
                deallocate_inner( cast_inner( node ) );
            }
            node = nullptr;
            transaction::commit();
        }

        inline void deallocate_inner( inner_node_persistent_ptr& node) {
            delete_persistent<inner_node_type>( node );
        }

        inline void deallocate_leaf(leaf_node_persistent_ptr& node) {
            delete_persistent<leaf_node_type>( node );
        }
        
        PMEMobjpool *get_objpool() {
            PMEMoid oid = pmemobj_oid( this );
            return pmemobj_pool_by_oid( oid );
        }

        pool_base get_pool_base() {
            return pool_base( get_objpool() );
        }

    public:
        b_tree_base() : epoch( 0 ) {
        }

        std::pair<iterator, bool> insert( const_reference entry) {
            auto pop = get_pool_base();

            if ( root == nullptr ) {
                allocate_leaf( pop, root );
            }
            assert( root != nullptr );

            std::pair<iterator, bool> ret = insert_descend( pop, entry );

            return ret;
        }

        iterator find( const key_type& key ) {
            leaf_node_type* leaf = find_leaf_node( key );
            if (leaf == nullptr) return end();

            typename leaf_node_type::iterator leaf_it = leaf->find( key );
            if (leaf->end() == leaf_it) return end();

            return iterator( leaf, leaf_it );
        }

        const_iterator find( const key_type& key ) const {
            const leaf_node_type* leaf = find_leaf_node( key );
            if (leaf == nullptr) return end();

            typename leaf_node_type::const_iterator leaf_it = leaf->find( key );
            if (leaf->end() == leaf_it) return end();

            return const_iterator( leaf, leaf_it );
        }

        size_t erase(const key_type& key) {
            leaf_node_type* leaf = find_leaf_node(key);
            if (leaf == nullptr) return size_t(0);
            auto pop = get_pool_base();
            return leaf->erase(pop, key);
        }
        
        void garbage_collection();
        
        iterator begin() {
            return iterator(leftmost_leaf());
        }

        iterator end() {
            leaf_node_type* leaf = rightmost_leaf();
            return iterator( leaf, leaf ? leaf->end() : typename leaf_node_type::iterator() );
        }

        const_iterator begin() const {
            return const_iterator(leftmost_leaf());
        }

        const_iterator end() const {
            const leaf_node_type* leaf = rightmost_leaf();
            return const_iterator( leaf, leaf ? leaf->end() : typename leaf_node_type::const_iterator() );
        }

        const_iterator cbegin() const {
            return begin();
        }

        const_iterator cend() const {
            return end();
        }

        reverse_iterator rbegin() {
            return reverse_iterator( end() );
        }

        reverse_iterator rend() {
            return reverse_iterator( begin() );
        }
    }; // class b_tree_base
    
    template<typename TKey, typename TValue, size_t degree>
    void b_tree_base<TKey, TValue, degree>::garbage_collection() {
        pool_base pop = get_pool_base();
        ++epoch;
        //pop.persist( &epoch, sizeof(epoch) );

        if (split_node != nullptr) {
            if ( split_node->leaf() ) {
                repair_leaf_split( pop );
            }
            else {
                repair_inner_split( pop );
            }
        }
    }

    template<typename TKey, typename TValue, size_t degree>
    typename b_tree_base<TKey, TValue, degree>::iterator b_tree_base<TKey, TValue, degree>::split_leaf_node(pool_base& pop, inner_node_type* parent_node, persistent_ptr<node_t>& src_node, const_reference entry, persistent_ptr<node_t>& left, persistent_ptr<node_t>& right) {
        const leaf_node_type* split_leaf = cast_leaf(src_node).get();
        assert( split_leaf->full() );
        assignment( pop, split_node, src_node );

        typename leaf_node_type::const_iterator middle = split_leaf->begin() + split_leaf->size() / 2;
        
        leaf_node_type* insert_node = nullptr;
        leaf_node_type* lnode = nullptr;
        if ( entry.first < middle->first ) {
            lnode = insert_node = allocate_leaf( pop, left, entry, split_leaf->begin(), middle, split_leaf->get_prev(), nullptr ).get();
            allocate_leaf( pop, right, middle, split_leaf->end(), cast_leaf(left), split_leaf->get_next() ).get();
        }
        else {
            lnode = allocate_leaf( pop, left, split_leaf->begin(), middle, split_leaf->get_prev(), nullptr ).get();
            insert_node = allocate_leaf( pop, right, entry, middle, split_leaf->end(), cast_leaf( left ), split_leaf->get_next() ).get();
        }

        lnode->set_next( cast_leaf( right ) );
        pop.persist( lnode->get_next() );
        
        correct_leaf_node_links(pop, src_node, left, right);

        if (parent_node) {
            parent_node->update_splitted_child( pop, lnode->back().first, left, right, split_node );
        }
        else {
            create_new_root( pop, lnode->back().first, left, right );
        }

        deallocate( split_node );

        typename leaf_node_type::iterator leaf_it = insert_node->find( entry.first );
        assert( leaf_it != insert_node->end() );
        assert( leaf_it->first == entry.first );
        assert( leaf_it->second == entry.second );
        return iterator(insert_node, leaf_it);
    }
    
    template<typename TKey, typename TValue, size_t degree>
    void b_tree_base<TKey, TValue, degree>::correct_leaf_node_links(pool_base& pop, persistent_ptr<node_t>& src_node, persistent_ptr<node_t>& left, persistent_ptr<node_t>& right) {
        persistent_ptr<leaf_node_type> lnode = cast_leaf(left);
        persistent_ptr<leaf_node_type> rnode = cast_leaf(right);
        leaf_node_type* current_node = cast_leaf(src_node).get();

        if (current_node->get_prev() != nullptr) {
            current_node->get_prev()->set_next( lnode );
            pop.persist( current_node->get_prev()->get_next() );
        }

        if (current_node->get_next() != nullptr) {
            current_node->get_next()->set_prev( rnode );
            pop.persist( current_node->get_next()->get_prev() );
        }
    }

    template<typename TKey, typename TValue, size_t degree>
    void b_tree_base<TKey, TValue, degree>::create_new_root(pool_base& pop, const key_type& key, node_persistent_ptr& l_child, node_persistent_ptr& r_child ) {
        assert( l_child != nullptr );
        assert( r_child != nullptr );
        assert( split_node == root );

        persistent_ptr<inner_node_type> inner_root = allocate_inner( pop, root, root->level() + 1, key, l_child, r_child );
    }
    
    template<typename TKey, typename TValue, size_t degree>
    std::pair<typename b_tree_base<TKey, TValue, degree>::iterator, bool> b_tree_base<TKey, TValue, degree>::insert_descend( pool_base& pop, const_reference entry ) {
        path_type path;
        const key_type& key = entry.first;

        node_persistent_ptr node = find_leaf_to_insert( key, path );
        leaf_node_type* leaf = cast_leaf( node ).get();
        inner_node_type* parent_node = nullptr;

        if (leaf->full()) {
            typename leaf_node_type::iterator leaf_it = leaf->find( key );
            if (leaf_it != leaf->end()) { // Entry with the same key found
                return std::pair<iterator, bool>( iterator( leaf, leaf_it ), false );
            }

           /**
             * If root is leaf.
             */
            if (path.empty()) {
                iterator it = split_leaf_node( pop, nullptr, node, entry, left_child, right_child );
                return std::pair<iterator, bool>( it, true );
            }

            // find the first full node
            auto i = find_full_node( path );

            /**
             * If root is full. Split root
             */
            if (( *i )->full()) {
                parent_node = nullptr;
                split_inner_node( pop, *i, parent_node, left_child, right_child );
                parent_node = cast_inner( cast_inner( root )->get_child( key ).get() );
            }
            else {
                parent_node = (*i).get();
            }
            ++i;

            for (; i != path.end(); ++i) {
                split_inner_node( pop, *i, parent_node, left_child, right_child );

                parent_node = cast_inner( parent_node->get_child( key ).get() );
            }

            iterator it = split_leaf_node( pop, parent_node, node, entry, left_child, right_child );
            return std::pair<iterator, bool>( it, true );
        }

        std::pair<typename leaf_node_type::iterator, bool> ret = leaf->insert( pop, entry );
        return std::pair<iterator, bool>( iterator( leaf, ret.first ), ret.second );;
    }

} // namespace internal

// TODO: add key comparator as a template argument; std::less should be default
template<typename Key, typename Value, size_t degree>
class b_tree : public internal::b_tree_base<Key, Value, degree> {
    // Base type definitions
    typedef b_tree<Key, Value, degree> self_type;
    typedef internal::b_tree_base<Key, Value, degree> base_type;
public:
    using base_type::begin;
    using base_type::end;
    using base_type::find;
    using base_type::insert;
    using base_type::erase;

    // Type definitions
    typedef typename base_type::key_type key_type;
    typedef typename base_type::mapped_type mapped_type;
    typedef typename base_type::value_type value_type;
    typedef typename base_type::iterator iterator;
    typedef typename base_type::const_iterator const_iterator;
    typedef typename base_type::reverse_iterator reverse_iterator;

    explicit b_tree() : base_type() {}
    ~b_tree() {}

    b_tree( const b_tree& ) = delete;
    b_tree& operator=( const b_tree& ) = delete;
};

} // namespace persistent
#endif // PERSISTENT_B_TREE
