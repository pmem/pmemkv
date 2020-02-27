// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#ifndef PERSISTENT_B_TREE
#define PERSISTENT_B_TREE

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <numeric>
#include <vector>

#include <cassert>

namespace persistent
{

namespace internal
{

using namespace pmem::obj;

/**
 * Base node type for inner and leaf node types
 */
class node_t {
public:
	node_t(uint64_t level = 0) : _level(level)
	{
	}

	bool leaf() const
	{
		return _level == 0ull;
	}

	uint64_t level() const
	{
		return _level;
	}

private:
	uint64_t _level;
}; /* class node_t */

/**
 * Implements iteration over a single tree node
 */
template <typename LeafType, bool is_const>
class node_iterator {
private:
	using leaf_type = LeafType;
	using leaf_node_ptr =
		typename std::conditional<is_const, const leaf_type *, leaf_type *>::type;
	friend class node_iterator<leaf_type, true>;

public:
	using value_type = typename leaf_type::value_type;
	using iterator_category = std::random_access_iterator_tag;
	using difference_type = ptrdiff_t;
	using reference = typename std::conditional<is_const, const value_type &,
						    value_type &>::type;
	using pointer = typename std::conditional<is_const, const value_type *,
						  value_type *>::type;

	node_iterator();

	node_iterator(leaf_node_ptr node_ptr, size_t p);

	node_iterator(const node_iterator &other);

	template <typename T = void,
		  typename = typename std::enable_if<is_const, T>::type>
	node_iterator(const node_iterator<leaf_type, false> &other);

	node_iterator &operator++();
	node_iterator operator++(int);
	node_iterator &operator--();
	node_iterator operator--(int);

	node_iterator operator+(size_t off) const;
	node_iterator operator+=(difference_type off);
	node_iterator operator-(difference_type off) const;

	difference_type operator-(const node_iterator &other) const;

	bool operator==(const node_iterator &other) const;
	bool operator!=(const node_iterator &other) const;
	bool operator<(const node_iterator &other) const;
	bool operator>(const node_iterator &other) const;

	reference operator*() const;
	pointer operator->() const;

private:
	leaf_node_ptr node;
	size_t position;
}; /* class node_iterator */

template <typename Key, typename T, typename Compare, uint64_t capacity>
class leaf_node_t : public node_t {
public:
	using key_type = Key;
	using mapped_type = T;
	using key_compare = Compare;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using value_type = std::pair<key_type, mapped_type>;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;

	using iterator = node_iterator<leaf_node_t, false>;
	using const_iterator = node_iterator<leaf_node_t, true>;

	leaf_node_t(const key_compare &comp, uint64_t e);
	~leaf_node_t();

	template <typename K, typename M>
	iterator move(pool_base &pop, persistent_ptr<leaf_node_t> other, K &&key,
		      M &&obj);

	std::pair<iterator, bool> insert(pool_base &pop, const_reference entry);
	template <typename K, typename M>
	std::pair<iterator, bool> insert(pool_base &pop, K &&key, M &&obj);
	void move_first(pool_base &pop, persistent_ptr<leaf_node_t> other);
	void move_last(pool_base &pop, persistent_ptr<leaf_node_t> other);

	template <typename K>
	iterator find(K &&key);
	template <typename K>
	const_iterator find(K &&key) const;
	template <typename K>
	iterator lower_bound(K &&key);
	template <typename K>
	const_iterator lower_bound(K &&key) const;
	template <typename K>
	iterator upper_bound(K &&key);
	template <typename K>
	const_iterator upper_bound(K &&key) const;

	size_t erase(pool_base &pop, const key_type &key);
	template <typename K>
	size_t erase(pool_base &pop, K &&key);

	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;

	size_t size() const;
	bool full() const;
	const_reference front() const;
	const_reference back() const;
	reference at(size_t pos);
	const_reference at(size_t pos) const;
	reference operator[](size_t pos);
	const_reference operator[](size_t pos) const;

	const persistent_ptr<leaf_node_t> &get_next() const;
	void set_next(const persistent_ptr<leaf_node_t> &n);
	const persistent_ptr<leaf_node_t> &get_prev() const;
	void set_prev(const persistent_ptr<leaf_node_t> &p);

	void check_consistency(uint64_t global_epoch);

private:
	/* uninitialized static array of value_type */
	union {
		value_type entries[capacity];
	};
	/* array of indexes to support ordering */
	struct leaf_entries_t {
		leaf_entries_t() : _size(0)
		{
			std::iota(std::begin(idxs), std::end(idxs), 0);
		}

		~leaf_entries_t()
		{
		}

		uint64_t idxs[capacity];
		size_type _size;
	};
	leaf_entries_t v[2];
	/* varibales for managing indexes and consistency */
	uint64_t epoch;
	uint32_t consistent_id;
	uint32_t p_consistent_id;
	/* persistent pointers to the neighboring leafs */
	pmem::obj::persistent_ptr<leaf_node_t> prev;
	pmem::obj::persistent_ptr<leaf_node_t> next;
	const key_compare *comparator;

	/* private helper methods */
	leaf_entries_t *consistent();
	const leaf_entries_t *consistent() const;
	leaf_entries_t *working_copy();
	void switch_consistent(pool_base &pop);
	template <typename... Args>
	pointer emplace(size_t pos, Args &&... args);
	size_t get_insert_idx() const;
	size_t insert_idx(pool_base &pop, uint64_t new_entry_idx, iterator hint);
	void remove_idx(pool_base &pop, ptrdiff_t idx);
	void copy_insert(const_reference entry, const_iterator first,
			 const_iterator last);
	void copy(const_iterator first, const_iterator last);
	void internal_erase(pool_base &pop, iterator it);
	bool is_sorted();
}; /* class leaf_node_t */

template <typename Key, typename Compare, uint64_t capacity>
class inner_node_t : public node_t {
private:
	using self_type = inner_node_t<Key, Compare, capacity>;
	using node_pptr = pmem::obj::persistent_ptr<node_t>;
	using key_pptr = pmem::obj::persistent_ptr<Key>;

public:
	using key_type = Key;
	using value_type = key_type;
	using key_compare = Compare;

	using reference = key_type &;
	using const_reference = const key_type &;
	using pointer = key_type *;
	using const_pointer = const key_type *;

	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using iterator = node_iterator<self_type, false>;
	using const_iterator = node_iterator<self_type, true>;

	inner_node_t(const key_compare &comp, size_type level);
	inner_node_t(const key_compare &comp, size_type level, const_reference key,
		     const node_pptr &first_child, const node_pptr &second_child);
	~inner_node_t();

	iterator move(pool_base &pop, inner_node_t &other, key_pptr &partition_key);
	template <typename K>
	void replace(iterator it, K &&key);
	void move_first(persistent_ptr<inner_node_t> other);
	void move_last(persistent_ptr<inner_node_t> other);
	void delete_with_child(iterator it, bool left);
	void inherit_child(iterator it, node_pptr &child, bool left);
	void update_splitted_child(pool_base &pop, const_reference key,
				   node_pptr &left_child, node_pptr &right_child);

	template <typename K>
	std::tuple<node_t *, node_t *, node_t *, iterator>
	get_child_and_siblings(K &&key);
	template <typename K>
	const node_pptr &get_child(K &&key) const;
	const node_pptr &get_child(const_reference key) const;
	const node_pptr &get_left_child(const_iterator it) const;
	const node_pptr &get_right_child(const_iterator it) const;
	template <typename K>
	iterator lower_bound(K &&key);
	template <typename K>
	const_iterator lower_bound(K &&key) const;
	template <typename K>
	iterator upper_bound(K &&key);
	template <typename K>
	const_iterator upper_bound(K &&key) const;

	bool full() const;

	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;

	size_type size() const;
	const_reference back() const;
	reference at(size_type pos);
	const_reference at(size_type pos) const;
	reference operator[](size_type pos);
	const_reference operator[](size_type pos) const;

	bool is_valid();

private:
	const static size_type child_capacity = capacity + 1;

	key_pptr entries[capacity];
	node_pptr children[child_capacity];
	size_type _size = 0;
	const key_compare *comparator;

	pool_base get_pool() const noexcept;
	bool is_sorted();
}; /* class inner_node_t */

template <typename LeafType, bool is_const>
class b_tree_iterator {
private:
	using leaf_type = LeafType;
	using leaf_node_ptr =
		typename std::conditional<is_const, const leaf_type *, leaf_type *>::type;
	using leaf_iterator =
		typename std::conditional<is_const, typename leaf_type::const_iterator,
					  typename leaf_type::iterator>::type;
	friend class b_tree_iterator<leaf_type, true>;

public:
	using iterator_category = std::bidirectional_iterator_tag;
	using difference_type = ptrdiff_t;
	using value_type = typename leaf_iterator::value_type;
	using reference = typename leaf_iterator::reference;
	using pointer = typename leaf_iterator::pointer;

	b_tree_iterator(std::nullptr_t);
	b_tree_iterator(leaf_node_ptr node);
	b_tree_iterator(leaf_node_ptr node, leaf_iterator _leaf_it);
	b_tree_iterator(const b_tree_iterator &other);
	template <typename T = void,
		  typename = typename std::enable_if<is_const, T>::type>
	b_tree_iterator(const b_tree_iterator<leaf_type, false> &other);

	b_tree_iterator &operator=(const b_tree_iterator &other);
	b_tree_iterator &operator++();
	b_tree_iterator operator++(int);
	b_tree_iterator &operator--();
	b_tree_iterator operator--(int);
	bool operator==(const b_tree_iterator &other);
	bool operator!=(const b_tree_iterator &other);
	reference operator*() const;
	pointer operator->() const;

private:
	leaf_node_ptr current_node;
	leaf_iterator leaf_it;
}; /* class b_tree_iterator */

template <typename Key, typename T, typename Compare, size_t degree>
class b_tree_base {
private:
	const static size_t node_capacity = degree - 1;

	using self_type = b_tree_base<Key, T, Compare, degree>;
	using leaf_type = leaf_node_t<Key, T, Compare, node_capacity>;
	using inner_type = inner_node_t<Key, Compare, node_capacity>;
	using key_pptr = persistent_ptr<Key>;
	using node_pptr = persistent_ptr<node_t>;
	using leaf_pptr = persistent_ptr<leaf_type>;
	using inner_pptr = persistent_ptr<inner_type>;
	using path_type = std::vector<inner_pptr>;

public:
	using value_type = typename leaf_type::value_type;
	using key_type = typename leaf_type::key_type;
	using mapped_type = typename leaf_type::mapped_type;
	using key_compare = Compare;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using reference = typename leaf_type::reference;
	using const_reference = typename leaf_type::const_reference;
	using pointer = typename leaf_type::pointer;
	using const_pointer = typename leaf_type::const_pointer;

	using iterator = b_tree_iterator<leaf_type, false>;
	using const_iterator = b_tree_iterator<leaf_type, true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	b_tree_base();
	~b_tree_base();

	std::pair<iterator, bool> insert(const_reference entry);
	template <typename K, typename M>
	std::pair<iterator, bool> try_emplace(K &&key, M &&obj);

	template <typename K>
	iterator find(K &&key);
	template <typename K>
	const_iterator find(K &&key) const;
	iterator lower_bound(const key_type &key);
	template <typename K>
	iterator lower_bound(K &&key);
	const_iterator lower_bound(const key_type &key) const;
	iterator upper_bound(const key_type &key);
	template <typename K>
	iterator upper_bound(K &&key);
	const_iterator upper_bound(const key_type &key) const;

	size_t erase(const key_type &key);
	template <typename K>
	size_t erase(K &&key);

	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator cbegin() const;
	const_iterator cend() const;
	reverse_iterator rbegin();
	reverse_iterator rend();

	key_compare &key_comp();
	const key_compare &key_comp() const;

private:
	uint64_t epoch;
	node_pptr root;
	node_pptr split_node;
	node_pptr left_child;
	node_pptr right_child;
	key_compare comparator;

	const key_type &get_last_key(const node_pptr &node);
	leaf_type *leftmost_leaf() const;
	leaf_type *rightmost_leaf() const;

	void create_new_root(pool_base &, const key_type &, node_pptr &, node_pptr &);
	typename inner_type::const_iterator split_half(pool_base &pop, inner_pptr &node,
						       inner_pptr &other,
						       key_pptr &partition_key);
	void split_inner_node(pool_base &pop, inner_pptr &src_node);
	void split_inner_node(pool_base &pop, inner_pptr &src_node,
			      inner_type *parent_node);
	template <typename K, typename M>
	iterator split_leaf_node(pool_base &pop, leaf_pptr &split_leaf, K &&key, M &&obj);
	template <typename K, typename M>
	iterator split_leaf_node(pool_base &pop, inner_type *parent_node,
				 leaf_pptr &split_leaf, K &&key, M &&obj);

	leaf_type *find_leaf_node(const key_type &key) const;
	template <typename K>
	leaf_type *find_leaf_node(K &&key) const;
	template <typename K>
	leaf_pptr find_leaf_to_insert(K &&key, path_type &path) const;
	typename path_type::const_iterator find_full_node(const path_type &path);

	static inner_pptr &cast_inner(node_pptr &node);
	static inner_type *cast_inner(node_t *node);
	static leaf_pptr &cast_leaf(node_pptr &node);
	static leaf_type *cast_leaf(node_t *node);

	template <typename... Args>
	inline inner_pptr allocate_inner(Args &&... args);
	template <typename... Args>
	inline leaf_pptr allocate_leaf(Args &&... args);
	inline void deallocate(node_pptr &node);
	inline void deallocate(leaf_pptr &node);
	inline void deallocate(inner_pptr &node);

	PMEMobjpool *get_objpool();
	pool_base get_pool_base();
}; /* class b_tree_base */

// -------------------------------------------------------------------------------------
// ------------------------------------- node_iterator ---------------------------------
// -------------------------------------------------------------------------------------

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const>::node_iterator() : node(nullptr), position(0)
{
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const>::node_iterator(leaf_node_ptr node_ptr, size_t p)
    : node(node_ptr), position(p)
{
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const>::node_iterator(const node_iterator &other)
    : node(other.node), position(other.position)
{
}

template <typename LeafType, bool is_const>
template <typename T, typename>
node_iterator<LeafType, is_const>::node_iterator(
	const node_iterator<leaf_type, false> &other)
    : node(other.node), position(other.position)
{
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const> &node_iterator<LeafType, is_const>::operator++()
{
	++position;
	return *this;
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const> node_iterator<LeafType, is_const>::operator++(int)
{
	node_iterator tmp = *this;
	++*this;
	return tmp;
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const> &node_iterator<LeafType, is_const>::operator--()
{
	assert(position > 0);
	--position;
	return *this;
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const> node_iterator<LeafType, is_const>::operator--(int)
{
	node_iterator tmp = *this;
	--*this;
	return tmp;
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const>
node_iterator<LeafType, is_const>::operator+(size_t off) const
{
	return node_iterator(node, position + off);
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const>
node_iterator<LeafType, is_const>::operator+=(difference_type off)
{
	position += static_cast<std::size_t>(off);
	return *this;
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const>
node_iterator<LeafType, is_const>::operator-(difference_type off) const
{
	assert(node != nullptr);
	assert(position >= static_cast<size_t>(off));
	return node_iterator(node, position - static_cast<size_t>(off));
}

template <typename LeafType, bool is_const>
typename node_iterator<LeafType, is_const>::difference_type
node_iterator<LeafType, is_const>::operator-(const node_iterator &other) const
{
	assert(node != nullptr);
	assert(other.node != nullptr);
	assert(node == other.node);
	return static_cast<difference_type>(position - other.position);
}

template <typename LeafType, bool is_const>
bool node_iterator<LeafType, is_const>::operator==(const node_iterator &other) const
{
	return node == other.node && position == other.position;
}

template <typename LeafType, bool is_const>
bool node_iterator<LeafType, is_const>::operator!=(const node_iterator &other) const
{
	return !(*this == other);
}

template <typename LeafType, bool is_const>
bool node_iterator<LeafType, is_const>::operator<(const node_iterator &other) const
{
	assert(node != nullptr);
	assert(other.node != nullptr);
	assert(node == other.node);
	return position < other.position;
}

template <typename LeafType, bool is_const>
bool node_iterator<LeafType, is_const>::operator>(const node_iterator &other) const
{
	assert(node != nullptr);
	assert(other.node != nullptr);
	assert(node == other.node);
	return position > other.position;
}

template <typename LeafType, bool is_const>
typename node_iterator<LeafType, is_const>::reference
	node_iterator<LeafType, is_const>::operator*() const
{
	assert(node != nullptr);
	return (*node)[position];
}

template <typename LeafType, bool is_const>
typename node_iterator<LeafType, is_const>::pointer
	node_iterator<LeafType, is_const>::operator->() const
{
	return &**this;
}

// -------------------------------------------------------------------------------------
// ------------------------------------- leaf_node_t -----------------------------------
// -------------------------------------------------------------------------------------

template <typename Key, typename T, typename Compare, uint64_t capacity>
leaf_node_t<Key, T, Compare, capacity>::leaf_node_t(const key_compare &comp, uint64_t e)
    : node_t(), epoch(e), consistent_id(0), p_consistent_id(0)
{
	comparator = &comp;
	assert(is_sorted());
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
leaf_node_t<Key, T, Compare, capacity>::~leaf_node_t()
{
	for (iterator temp = begin(); temp != end(); ++temp) {
		(*temp).~value_type();
	}
	if (prev) {
		prev.~persistent_ptr<leaf_node_t>();
	}
	if (next) {
		next.~persistent_ptr<leaf_node_t>();
	}
	v[0].~leaf_entries_t();
	v[1].~leaf_entries_t();
}

/**
 * Moves second half of the 'other' to 'this' in sorted order.
 * Inserts given key-object as a new entry to an appropriate node.
 *
 * @pre std:distance(middle, end) > 0
 *
 * @post this = other[middle, end)
 * @post other = other[0, middle)
 *
 * @return iterator on newly inserted entry
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K, typename M>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::move(pool_base &pop,
					     persistent_ptr<leaf_node_t> other, K &&key,
					     M &&obj)
{
	assert(other->full());
	assert(this->size() == 0);
	auto middle = std::make_move_iterator(other->begin() + other->size() / 2);
	auto last = std::make_move_iterator(other->end());
	auto temp = middle;
	bool less = (*comparator)(std::forward<K>(key), middle->first);
	std::pair<iterator, bool> result;
	// move second half from 'other' to 'this'
	pmem::obj::transaction::run(pop, [&] {
		size_t count = 0;
		while (temp != last) {
			emplace(count++, *temp++);
		}
		consistent()->_size = count;
		other->consistent()->_size -= count;
		// need to leave other's idxs in consistent state
		leaf_entries_t *other_c = other->consistent();
		leaf_entries_t *other_w = other->working_copy();
		std::copy(other_c->idxs, other_c->idxs + capacity, other_w->idxs);
		// insert entriy(key, obj) into needed half
		if (less) {
			result = other->insert(pop, std::forward<K>(key),
					       std::forward<M>(obj));
		} else {
			result = this->insert(pop, std::forward<K>(key),
					      std::forward<M>(obj));
		}
	});
	assert(std::distance(begin(), end()) > 0);
	assert(is_sorted());
	return result.first;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
std::pair<typename leaf_node_t<Key, T, Compare, capacity>::iterator, bool>
leaf_node_t<Key, T, Compare, capacity>::insert(pool_base &pop, const_reference entry)
{
	assert(!full());

	iterator hint = lower_bound(entry.first);
	if (hint != end() && !(*comparator)(hint->first, entry.first) &&
	    !(*comparator)(entry.first, hint->first)) {
		return std::pair<iterator, bool>(hint, false);
	}

	size_t insert_pos = get_insert_idx();
	assert(std::none_of(consistent()->idxs, consistent()->idxs + size(),
			    [insert_pos](uint64_t idx) { return insert_pos == idx; }));
	// insert an entry to the end
	pmem::obj::transaction::run(pop, [&] { entries[insert_pos] = entry; });
	pop.flush(&(entries[insert_pos]), sizeof(entries[insert_pos]));
	// update tmp idxs
	size_t position = insert_idx(pop, insert_pos, hint);
	// update consistent
	switch_consistent(pop);

	assert(is_sorted());

	return std::pair<iterator, bool>(iterator(this, position), true);
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K, typename M>
std::pair<typename leaf_node_t<Key, T, Compare, capacity>::iterator, bool>
leaf_node_t<Key, T, Compare, capacity>::insert(pool_base &pop, K &&key, M &&obj)
{
	assert(!full());

	iterator hint = lower_bound(std::forward<K>(key));
	if (hint != end() && !(*comparator)(hint->first, std::forward<K>(key)) &&
	    !(*comparator)(std::forward<K>(key), hint->first)) {
		return std::pair<iterator, bool>(hint, false);
	}

	size_t insert_pos = get_insert_idx();
	assert(std::none_of(consistent()->idxs, consistent()->idxs + size(),
			    [insert_pos](uint64_t idx) { return insert_pos == idx; }));
	// insert an entry to the end
	pmem::obj::transaction::run(pop, [&] {
		emplace(insert_pos, std::forward<K>(key), std::forward<M>(obj));
	});
	// update tmp idxs
	size_t position = insert_idx(pop, insert_pos, hint);
	// update consistent
	switch_consistent(pop);

	assert(is_sorted());
	return std::pair<iterator, bool>(iterator(this, position), true);
}

/**
 * Moves first element from 'other' to 'this'
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::move_first(pool_base &pop,
							persistent_ptr<leaf_node_t> other)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size() == 0);
	assert(other->size() > 1);

	emplace(0, std::move(other->front()));
	consistent()->_size++;
	other->consistent()->_size--;
	other->remove_idx(pop, 0);
	other->switch_consistent(pop);
}

/**
 * Moves last element from 'other' to 'this'
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::move_last(pool_base &pop,
						       persistent_ptr<leaf_node_t> other)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size() == 0);
	assert(other->size() > 1);

	emplace(0, std::move(other->back()));
	consistent()->_size++;
	other->consistent()->_size--;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::find(K &&key)
{
	assert(is_sorted());

	iterator it = lower_bound(std::forward<K>(key));
	if (it == end() ||
	    (!(*comparator)(it->first, std::forward<K>(key)) &&
	     !(*comparator)(std::forward<K>(key), it->first))) {
		return it;
	} else {
		return end();
	}
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::const_iterator
leaf_node_t<Key, T, Compare, capacity>::find(K &&key) const
{
	assert(is_sorted());

	iterator it = lower_bound(std::forward<K>(key));
	if (it == end() ||
	    (!(*comparator)(it->first, std::forward<K>(key)) &&
	     !(*comparator)(std::forward<K>(key), it->first))) {
		return it;
	} else {
		return end();
	}
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::lower_bound(K &&key)
{
	iterator it, first = begin(), last = end();
	typename iterator::difference_type count, step;
	count = std::distance(first, last);

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if ((*comparator)((*it).first, std::forward<K>(key))) {
			first = ++it;
			count -= step + 1;
		} else
			count = step;
	}
	return first;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::const_iterator
leaf_node_t<Key, T, Compare, capacity>::lower_bound(K &&key) const
{
	const_iterator it, first = begin(), last = end();
	typename const_iterator::difference_type count, step;
	count = std::distance(first, last);

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if ((*comparator)((*it).first, std::forward<K>(key))) {
			first = ++it;
			count -= step + 1;
		} else {
			count = step;
		}
	}
	return first;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::upper_bound(K &&key)
{
	iterator it, first = begin();
	typename iterator::difference_type count, step;
	count = std::distance(first, end());

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if (!(*comparator)(std::forward<K>(key), (*it).first)) {
			first = ++it;
			count -= step + 1;
		} else {
			count = step;
		}
	}
	return first;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::const_iterator
leaf_node_t<Key, T, Compare, capacity>::upper_bound(K &&key) const
{
	const_iterator it, first = begin();
	typename const_iterator::difference_type count, step;
	count = std::distance(first, end());

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if (!(*comparator)(std::forward<K>(key), (*it).first)) {
			first = ++it;
			count -= step + 1;
		} else {
			count = step;
		}
	}
	return first;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
size_t leaf_node_t<Key, T, Compare, capacity>::erase(pool_base &pop, const key_type &key)
{
	assert(is_sorted());
	iterator it = find(key);
	if (it == end())
		return size_t(0);
	internal_erase(pop, it);
	return size_t(1);
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
size_t leaf_node_t<Key, T, Compare, capacity>::erase(pool_base &pop, K &&key)
{
	assert(is_sorted());
	iterator it = find(std::forward<K>(key));
	if (it == end())
		return size_t(0);
	internal_erase(pop, it);
	return size_t(1);
}

/**
 * Return begin iterator on an array of correct indices.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::begin()
{
	return iterator(this, 0);
}

/**
 * Return const_iterator to the beginning.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_iterator
leaf_node_t<Key, T, Compare, capacity>::begin() const
{
	return const_iterator(this, 0);
}

/**
 * Return end iterator on an array of indices.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::end()
{
	return iterator(this, consistent()->_size);
}

/**
 * Return const_iterator to the end.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_iterator
leaf_node_t<Key, T, Compare, capacity>::end() const
{
	return const_iterator(this, consistent()->_size);
}

/**
 * Return the size of the array of entries (key/value).
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
size_t leaf_node_t<Key, T, Compare, capacity>::size() const
{
	return consistent()->_size;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
bool leaf_node_t<Key, T, Compare, capacity>::full() const
{
	return size() == capacity;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_reference
leaf_node_t<Key, T, Compare, capacity>::front() const
{
	return entries[consistent()->idxs[0]];
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_reference
leaf_node_t<Key, T, Compare, capacity>::back() const
{
	return entries[consistent()->idxs[consistent()->_size - 1]];
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::reference
leaf_node_t<Key, T, Compare, capacity>::at(size_t pos)
{
	if (size() <= pos) {
		throw std::out_of_range("Accessing incorrect element in leaf node");
	}
	return entries[consistent()->idxs[pos]];
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_reference
leaf_node_t<Key, T, Compare, capacity>::at(size_t pos) const
{
	if (size() <= pos) {
		throw std::out_of_range("Accessing incorrect element in leaf node");
	}
	return entries[consistent()->idxs[pos]];
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::reference
	leaf_node_t<Key, T, Compare, capacity>::operator[](size_t pos)
{
	return entries[consistent()->idxs[pos]];
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_reference
	leaf_node_t<Key, T, Compare, capacity>::operator[](size_t pos) const
{
	return entries[consistent()->idxs[pos]];
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
const persistent_ptr<leaf_node_t<Key, T, Compare, capacity>> &
leaf_node_t<Key, T, Compare, capacity>::get_next() const
{
	return this->next;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::set_next(
	const persistent_ptr<leaf_node_t> &n)
{
	this->next = n;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
const persistent_ptr<leaf_node_t<Key, T, Compare, capacity>> &
leaf_node_t<Key, T, Compare, capacity>::get_prev() const
{
	return this->prev;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::set_prev(
	const persistent_ptr<leaf_node_t> &p)
{
	this->prev = p;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::check_consistency(uint64_t global_epoch)
{
	if (global_epoch != epoch) {
		consistent_id = p_consistent_id;
		epoch = global_epoch;
	}
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::leaf_entries_t *
leaf_node_t<Key, T, Compare, capacity>::consistent()
{
	assert(consistent_id < 2);
	return v + consistent_id;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
const typename leaf_node_t<Key, T, Compare, capacity>::leaf_entries_t *
leaf_node_t<Key, T, Compare, capacity>::consistent() const
{
	assert(consistent_id < 2);
	return v + consistent_id;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::leaf_entries_t *
leaf_node_t<Key, T, Compare, capacity>::working_copy()
{
	assert(consistent_id < 2);
	uint32_t working_id = 1 - consistent_id;
	return v + working_id;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::switch_consistent(pool_base &pop)
{
	p_consistent_id = consistent_id = 1 - consistent_id;
	// TODO: need to check if it make sense to use non-temporal store
	pop.persist(&p_consistent_id, sizeof(p_consistent_id));
}

/**
 * Constructs value_type in position 'pos' of entries with arguments 'args'.
 *
 * @pre must be called in a transaction scope.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename... Args>
typename leaf_node_t<Key, T, Compare, capacity>::pointer
leaf_node_t<Key, T, Compare, capacity>::emplace(size_t pos, Args &&... args)
{
	pmem::detail::conditional_add_to_tx(entries + pos, 1, POBJ_XADD_NO_SNAPSHOT);
	return new (entries + pos) value_type(std::forward<Args>(args)...);
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
size_t leaf_node_t<Key, T, Compare, capacity>::get_insert_idx() const
{
	const leaf_entries_t *c = consistent();
	return c->idxs[c->_size];
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
size_t leaf_node_t<Key, T, Compare, capacity>::insert_idx(pool_base &pop,
							  uint64_t new_entry_idx,
							  iterator hint)
{
	size_t size = this->size();
	leaf_entries_t *tmp = working_copy();
	auto in_begin = consistent()->idxs;
	auto in_end = in_begin + size;
	auto partition_point = in_begin + std::distance(begin(), hint);
	auto out_begin = tmp->idxs;
	auto insert_pos = std::copy(in_begin, partition_point, out_begin);
	*insert_pos = new_entry_idx;
	std::copy(partition_point, in_end, insert_pos + 1);
	tmp->_size = size + 1;

	pop.persist(tmp, sizeof(leaf_entries_t));

	auto result = std::distance(out_begin, insert_pos);
	assert(result >= 0);

	return static_cast<std::size_t>(result);
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::remove_idx(pool_base &pop, ptrdiff_t idx)
{
	size_t size = this->size();
	leaf_entries_t *tmp = working_copy();
	auto in_begin = consistent()->idxs;
	auto in_end = in_begin + size;
	auto partition_point = in_begin + idx;
	auto out = tmp->idxs;
	out = std::copy(in_begin, partition_point, out);
	out = std::copy(partition_point + 1, in_end, out);
	*out = *partition_point;
	tmp->_size = size - 1;

	pop.persist(tmp, sizeof(leaf_entries_t));
}

/**
 * Copy entries from another node in the range of [first, last) and insert new
 * entry.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::copy_insert(const_reference entry,
							 const_iterator first,
							 const_iterator last)
{
	assert(std::distance(first, last) >= 0);
	assert(static_cast<std::size_t>(std::distance(first, last)) < capacity);

	auto d_last = std::merge(first, last, &entry, &entry + 1, entries,
				 [this](const_reference a, const_reference b) {
					 return (*comparator)(a.first, b.first);
				 });

	assert(std::distance(entries, d_last) >= 0);
	consistent()->_size = static_cast<std::size_t>(std::distance(entries, d_last));

	std::iota(consistent()->idxs, consistent()->idxs + consistent()->_size, 0);
}

/**
 * Copy entries from another node in the range of [first, last).
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::copy(const_iterator first,
						  const_iterator last)
{
	assert(std::distance(first, last) >= 0);
	assert(static_cast<std::size_t>(std::distance(first, last)) < capacity);

	auto d_last = std::copy(first, last, entries);

	assert(std::distance(entries, d_last) >= 0);
	consistent()->_size = static_cast<std::size_t>(std::distance(entries, d_last));

	std::iota(consistent()->idxs, consistent()->idxs + consistent()->_size, 0);
}

/**
 * Remove element pointed by iterator.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::internal_erase(pool_base &pop, iterator it)
{
	ptrdiff_t idx = std::distance(begin(), it);
	pmem::obj::transaction::run(pop, [&] {
		// update tmp idxs
		remove_idx(pop, idx);
		// erase
		(*it).first.~key_type();
		(*it).second.~mapped_type();
	});
	// update consistent
	switch_consistent(pop);

	assert(is_sorted());
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
bool leaf_node_t<Key, T, Compare, capacity>::is_sorted()
{
	return std::is_sorted(begin(), end(),
			      [this](const_reference a, const_reference b) {
				      return (*comparator)(a.first, b.first);
			      });
}

// -------------------------------------------------------------------------------------
// ------------------------------------- inner_node_t ----------------------------------
// -------------------------------------------------------------------------------------

template <typename Key, typename Compare, uint64_t capacity>
inner_node_t<Key, Compare, capacity>::inner_node_t(const key_compare &comp,
						   size_type level)
    : node_t(level), _size(0)
{
	comparator = &comp;
}

template <typename Key, typename Compare, uint64_t capacity>
inner_node_t<Key, Compare, capacity>::inner_node_t(const key_compare &comp,
						   size_type level, const_reference key,
						   const node_pptr &first_child,
						   const node_pptr &second_child)
    : node_t(level)
{
	entries[0] = pmem::obj::persistent_ptr<key_type>(&key);
	children[0] = first_child;
	children[1] = second_child;
	_size = 1;

	comparator = &comp;

	assert(is_valid());
	assert(is_sorted());
}

template <typename Key, typename Compare, uint64_t capacity>
inner_node_t<Key, Compare, capacity>::~inner_node_t()
{
}

/**
 * Moves second half from 'other' to 'this'.
 * Returns iterator to first from 'this'.
 */
template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::iterator
inner_node_t<Key, Compare, capacity>::move(pool_base &pop, inner_node_t &other,
					   key_pptr &partition_key)
{
	assert(size() == 0);
	assert(other.size() > size_type(1));
	key_pptr *middle = other.entries + other.size() / 2;
	key_pptr *last = other.entries + other.size();
	size_type new_size = static_cast<size_type>(std::distance(middle + 1, last));
	node_pptr *middle_child = other.children + (other.size() / 2) + 1;
	node_pptr *last_child = other.children + other.size() + 1;
	/* move second half from 'other' to 'this' */
	pmem::obj::transaction::run(pop, [&] {
		/* save partition key */
		partition_key = *middle;
		std::move(middle + 1, last, entries);
		std::move(middle_child, last_child, children);
		_size = new_size;
		other._size -= (new_size + 1);
	});
	assert(std::distance(begin(), end()) > 0);
	assert(is_valid());
	assert(other.is_valid());
	assert(is_sorted());
	assert(other.is_sorted());
	return begin();
}

/**
 * Changes entry specified by the iterator with key pointer
 */
template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
void inner_node_t<Key, Compare, capacity>::replace(iterator it, K &&key)
{
	size_type pos = static_cast<size_type>(std::distance(begin(), it));
	entries[pos] = pmem::obj::persistent_ptr<key_type>(&key);
}

/**
 * Updates inner node after splitting child.
 *
 * @param[in] pop - persistent pool
 * @param[in] key - key of the first entry in right_child
 * @param[in] left_child - new child node that must be linked
 * @param[in] right_child - new child node that must be linked
 */
template <typename Key, typename Compare, uint64_t capacity>
void inner_node_t<Key, Compare, capacity>::update_splitted_child(pool_base &pop,
								 const_reference key,
								 node_pptr &left_child,
								 node_pptr &right_child)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(!full());
	iterator insert_it = lower_bound(key);
	difference_type insert_idx = std::distance(begin(), insert_it);
	/* update entries inserting new key */
	key_pptr *to_insert = std::copy_backward(entries + insert_idx, entries + _size,
						 entries + _size + 1);
	assert(insert_idx < std::distance(entries, to_insert));
	*(--to_insert) = pmem::obj::persistent_ptr<key_type>(&key);
	_size = _size + 1;
	/* update children inserting new descendants */
	node_pptr *to_insert_child = std::copy_backward(
		children + insert_idx + 1, children + _size, children + _size + 1);
	*(--to_insert_child) = right_child;
	*(--to_insert_child) = left_child;

	assert(is_sorted());
	assert(is_valid());
}

/**
 * Moves first element with first child from 'other' to 'this'
 */
template <typename Key, typename Compare, uint64_t capacity>
void inner_node_t<Key, Compare, capacity>::move_first(persistent_ptr<inner_node_t> other)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size() == 0);
	assert(other->size() > 1);

	entries[0] = std::move(other->entries[0]);
	children[0] = std::move(other->children[0]);
	std::move(other->entries + 1, other->entries + other->_size, other->entries);
	std::move(other->children + 1, other->children + other->_size + 1,
		  other->children);

	_size++;
	other->_size--;
}

/**
 * Moves last element with last child from 'other' to 'this'
 */
template <typename Key, typename Compare, uint64_t capacity>
void inner_node_t<Key, Compare, capacity>::move_last(persistent_ptr<inner_node_t> other)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size() == 0);
	assert(other->size() > 1);

	entries[0] = std::move(other->entries[other->_size - 1]);
	children[0] = std::move(other->children[other->_size]);

	_size++;
	other->_size--;
}

/**
 * Deletes key specified by iterator.
 * Must be followed by node balancing.
 */
template <typename Key, typename Compare, uint64_t capacity>
void inner_node_t<Key, Compare, capacity>::delete_with_child(iterator it, bool left)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size() > 0);

	size_type pos = static_cast<size_type>(std::distance(begin(), it));
	std::move(entries + pos + 1, entries + _size, entries + pos);
	if (left) {
		std::move(children + pos + 1, children + _size + 1, children + pos);
	} else {
		std::move(children + pos + 2, children + _size + 1, children + pos + 1);
	}
	_size--;
}

/**
 * Inherits child specified by iterator and 'left' bool.
 * Key is updated by smallest in right subtree.
 * Assuming that previous child is no longer used and must be deleted.
 *
 * @pre child.size() == 0
 */
template <typename Key, typename Compare, uint64_t capacity>
void inner_node_t<Key, Compare, capacity>::inherit_child(iterator it, node_pptr &child,
							 bool left)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size() > 0);

	size_type pos = static_cast<size_type>(std::distance(begin(), it));
	if (left) {
		children[pos] = child;
	} else {
		children[pos + 1] = child;
	}
}

template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
const typename inner_node_t<Key, Compare, capacity>::node_pptr &
inner_node_t<Key, Compare, capacity>::get_child(K &&key) const
{
	auto it = upper_bound(std::forward<K>(key));
	return get_left_child(it);
}

template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
std::tuple<node_t *, node_t *, node_t *,
	   typename inner_node_t<Key, Compare, capacity>::iterator>
inner_node_t<Key, Compare, capacity>::get_child_and_siblings(K &&key)
{
	assert(size() > 0);
	iterator it = upper_bound(std::forward<K>(key));
	if (it == begin()) {
		return std::make_tuple(get_left_child(it).get(), nullptr,
				       get_right_child(it).get(), it);
	} else if (it == end()) {
		return std::make_tuple(get_left_child(it).get(),
				       get_left_child(it - 1).get(), nullptr, it - 1);
	} else {
		return std::make_tuple(get_left_child(it).get(),
				       get_left_child(it - 1).get(),
				       get_right_child(it).get(), it - 1);
	}
}

template <typename Key, typename Compare, uint64_t capacity>
const typename inner_node_t<Key, Compare, capacity>::node_pptr &
inner_node_t<Key, Compare, capacity>::get_child(const_reference key) const
{
	auto it = upper_bound(key);
	return get_left_child(it);
}

template <typename Key, typename Compare, uint64_t capacity>
const typename inner_node_t<Key, Compare, capacity>::node_pptr &
inner_node_t<Key, Compare, capacity>::get_left_child(const_iterator it) const
{
	auto result = std::distance(begin(), it);
	assert(result >= 0);

	size_type child_pos = static_cast<size_type>(result);
	return children[child_pos];
}

template <typename Key, typename Compare, uint64_t capacity>
const typename inner_node_t<Key, Compare, capacity>::node_pptr &
inner_node_t<Key, Compare, capacity>::get_right_child(const_iterator it) const
{
	auto result = std::distance(begin(), it);
	assert(result >= 0);

	size_type child_pos = static_cast<size_type>(result + 1);
	return children[child_pos];
}

template <typename Key, typename Compare, uint64_t capacity>
bool inner_node_t<Key, Compare, capacity>::full() const
{
	return this->size() == capacity;
}

/**
 * Return begin iterator on an array of keys.
 */
template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::iterator
inner_node_t<Key, Compare, capacity>::begin()
{
	return iterator(this, 0);
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_iterator
inner_node_t<Key, Compare, capacity>::begin() const
{
	return const_iterator(this, 0);
}

/**
 * Return end iterator on an array of keys.
 */
template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::iterator
inner_node_t<Key, Compare, capacity>::end()
{
	return begin() + this->size();
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_iterator
inner_node_t<Key, Compare, capacity>::end() const
{
	return begin() + this->size();
}

/**
 * Return the size of the array of keys.
 */
template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::size_type
inner_node_t<Key, Compare, capacity>::size() const
{
	return _size;
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_reference
inner_node_t<Key, Compare, capacity>::back() const
{
	return *entries[this->size() - 1];
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::reference
inner_node_t<Key, Compare, capacity>::at(size_type pos)
{
	if (size() <= pos) {
		throw std::out_of_range("Accessing incorrect element in inner node");
	}
	return *entries[pos];
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_reference
inner_node_t<Key, Compare, capacity>::at(size_type pos) const
{
	if (size() <= pos) {
		throw std::out_of_range("Accessing incorrect element inner node");
	}
	return *entries[pos];
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::reference
	inner_node_t<Key, Compare, capacity>::operator[](size_type pos)
{
	return *entries[pos];
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_reference
	inner_node_t<Key, Compare, capacity>::operator[](size_type pos) const
{
	return *entries[pos];
}

template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
typename inner_node_t<Key, Compare, capacity>::iterator
inner_node_t<Key, Compare, capacity>::lower_bound(K &&key)
{
	iterator it, first = begin(), last = end();
	typename iterator::difference_type count, step;
	count = std::distance(first, last);

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if ((*comparator)(*it, std::forward<K>(key))) {
			first = ++it;
			count -= step + 1;
		} else
			count = step;
	}
	return first;
}

template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
typename inner_node_t<Key, Compare, capacity>::const_iterator
inner_node_t<Key, Compare, capacity>::lower_bound(K &&key) const
{
	const_iterator it, first = begin(), last = end();
	typename const_iterator::difference_type count, step;
	count = std::distance(first, last);

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if ((*comparator)(*it, std::forward<K>(key))) {
			first = ++it;
			count -= step + 1;
		} else
			count = step;
	}
	return first;
}

template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
typename inner_node_t<Key, Compare, capacity>::iterator
inner_node_t<Key, Compare, capacity>::upper_bound(K &&key)
{
	iterator it, first = begin();
	typename iterator::difference_type count, step;
	count = std::distance(first, end());

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if (!(*comparator)(std::forward<K>(key), *it)) {
			first = ++it;
			count -= step + 1;
		} else {
			count = step;
		}
	}
	return first;
}

template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
typename inner_node_t<Key, Compare, capacity>::const_iterator
inner_node_t<Key, Compare, capacity>::upper_bound(K &&key) const
{
	const_iterator it, first = begin();
	typename const_iterator::difference_type count, step;
	count = std::distance(first, end());

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if (!(*comparator)(std::forward<K>(key), *it)) {
			first = ++it;
			count -= step + 1;
		} else {
			count = step;
		}
	}
	return first;
}

template <typename Key, typename Compare, uint64_t capacity>
bool inner_node_t<Key, Compare, capacity>::is_valid()
{
	return std::all_of(begin(), end(), [this](key_type &e) { return e != ""; });
}

template <typename Key, typename Compare, uint64_t capacity>
bool inner_node_t<Key, Compare, capacity>::is_sorted()
{
	return std::is_sorted(begin(), end(),
			      [this](const key_type &lhs, const key_type &rhs) {
				      return (*comparator)(lhs, rhs);
			      });
}

template <typename Key, typename Compare, uint64_t capacity>
pool_base inner_node_t<Key, Compare, capacity>::get_pool() const noexcept
{
	auto pop = pmemobj_pool_by_ptr(this);
	assert(pop != nullptr);
	return pool_base(pop);
}

// -------------------------------------------------------------------------------------
// ----------------------------------- b_tree_iterator ---------------------------------
// -------------------------------------------------------------------------------------

template <typename LeafType, bool is_const>
b_tree_iterator<LeafType, is_const>::b_tree_iterator(std::nullptr_t)
    : current_node(nullptr), leaf_it()
{
}

template <typename LeafType, bool is_const>
b_tree_iterator<LeafType, is_const>::b_tree_iterator(leaf_node_ptr node)
    : current_node(node), leaf_it(node->begin())
{
}

template <typename LeafType, bool is_const>
b_tree_iterator<LeafType, is_const>::b_tree_iterator(leaf_node_ptr node,
						     leaf_iterator _leaf_it)
    : current_node(node), leaf_it(_leaf_it)
{
}

template <typename LeafType, bool is_const>
b_tree_iterator<LeafType, is_const>::b_tree_iterator(const b_tree_iterator &other)
    : current_node(other.current_node), leaf_it(other.leaf_it)
{
}

template <typename LeafType, bool is_const>
template <typename T, typename>
b_tree_iterator<LeafType, is_const>::b_tree_iterator(
	const b_tree_iterator<leaf_type, false> &other)
    : current_node(other.current_node), leaf_it(other.leaf_it)
{
}

template <typename LeafType, bool is_const>
b_tree_iterator<LeafType, is_const> &
b_tree_iterator<LeafType, is_const>::operator=(const b_tree_iterator &other)
{
	current_node = other.current_node;
	leaf_it = other.leaf_it;
	return *this;
}

template <typename LeafType, bool is_const>
b_tree_iterator<LeafType, is_const> &b_tree_iterator<LeafType, is_const>::operator++()
{
	++leaf_it;
	if (leaf_it == current_node->end()) {
		leaf_node_ptr tmp = current_node->get_next().get();
		if (tmp) {
			current_node = tmp;
			leaf_it = current_node->begin();
		}
	}
	return *this;
}

template <typename LeafType, bool is_const>
b_tree_iterator<LeafType, is_const> b_tree_iterator<LeafType, is_const>::operator++(int)
{
	b_tree_iterator tmp = *this;
	++*this;
	return tmp;
}

template <typename LeafType, bool is_const>
b_tree_iterator<LeafType, is_const> &b_tree_iterator<LeafType, is_const>::operator--()
{
	if (leaf_it == current_node->begin()) {
		leaf_node_ptr tmp = current_node->get_prev().get();
		if (tmp) {
			current_node = tmp;
			leaf_it = current_node->end();
		}
	} else {
		--leaf_it;
	}
	return *this;
}

template <typename LeafType, bool is_const>
b_tree_iterator<LeafType, is_const> b_tree_iterator<LeafType, is_const>::operator--(int)
{
	b_tree_iterator tmp = *this;
	--*this;
	return tmp;
}

template <typename LeafType, bool is_const>
bool b_tree_iterator<LeafType, is_const>::operator==(const b_tree_iterator &other)
{
	return current_node == other.current_node && leaf_it == other.leaf_it;
}

template <typename LeafType, bool is_const>
bool b_tree_iterator<LeafType, is_const>::operator!=(const b_tree_iterator &other)
{
	return !(*this == other);
}

template <typename LeafType, bool is_const>
typename b_tree_iterator<LeafType, is_const>::reference
	b_tree_iterator<LeafType, is_const>::operator*() const
{
	return *(leaf_it);
}

template <typename LeafType, bool is_const>
typename b_tree_iterator<LeafType, is_const>::pointer
	b_tree_iterator<LeafType, is_const>::operator->() const
{
	return &**this;
}

// -------------------------------------------------------------------------------------
// ------------------------------------- b_tree_base -----------------------------------
// -------------------------------------------------------------------------------------

template <typename Key, typename T, typename Compare, size_t degree>
b_tree_base<Key, T, Compare, degree>::b_tree_base() : epoch(0)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	cast_leaf(root) = allocate_leaf();
}

template <typename Key, typename T, typename Compare, size_t degree>
b_tree_base<Key, T, Compare, degree>::~b_tree_base()
{
	deallocate(root);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K, typename M>
std::pair<typename b_tree_base<Key, T, Compare, degree>::iterator, bool>
b_tree_base<Key, T, Compare, degree>::try_emplace(K &&key, M &&obj)
{
	auto pop = get_pool_base();

	path_type path;
	leaf_pptr leaf = find_leaf_to_insert(std::forward<K>(key), path);

	// --------------- entry with the same key found ---------------
	typename leaf_type::iterator leaf_it = leaf->find(std::forward<K>(key));
	if (leaf_it != leaf->end()) {
		return std::pair<iterator, bool>(iterator(leaf.get(), leaf_it), false);
	}

	// ------------------ leaf not full -> insert ------------------
	if (!leaf->full()) {
		std::pair<typename leaf_type::iterator, bool> ret =
			leaf->insert(pop, std::forward<K>(key), std::forward<M>(obj));
		return std::pair<iterator, bool>(iterator(leaf.get(), ret.first),
						 ret.second);
	}

	// -------------------- if root is leaf ------------------------
	if (path.empty()) {
		iterator it = split_leaf_node(pop, leaf, std::forward<K>(key),
					      std::forward<M>(obj));
		return std::pair<iterator, bool>(it, true);
	}

	// ---------- find the first not full node from leaf -----------
	auto i = path.end() - 1;
	for (; i > path.begin(); --i) {
		if (!(*i)->full()) {
			break;
		}
	}

	// -------------- if root is full split root -------------------
	inner_type *parent_node = nullptr;
	if ((*i)->full()) {
		split_inner_node(pop, *i);
		parent_node = cast_inner(
			cast_inner(root)->get_child(std::forward<K>(key)).get());
	} else {
		parent_node = (*i).get();
	}
	++i;

	for (; i != path.end(); ++i) {
		split_inner_node(pop, *i, parent_node);
		parent_node =
			cast_inner(parent_node->get_child(std::forward<K>(key)).get());
	}

	iterator it = split_leaf_node(pop, parent_node, leaf, std::forward<K>(key),
				      std::forward<M>(obj));
	return std::pair<iterator, bool>(it, true);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::find(K &&key)
{
	leaf_type *leaf = find_leaf_node(std::forward<K>(key));
	typename leaf_type::iterator leaf_it = leaf->find(std::forward<K>(key));
	if (leaf->end() == leaf_it)
		return end();

	return iterator(leaf, leaf_it);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::find(K &&key) const
{
	leaf_type *leaf = find_leaf_node(std::forward<K>(key));
	typename leaf_type::iterator leaf_it = leaf->find(std::forward<K>(key));
	if (leaf->end() == leaf_it)
		return end();

	return iterator(leaf, leaf_it);
}

/**
 * Returns an iterator pointing to the least element which is larger than or equal
 * to the given key. Keys are sorted in binary order (see
 * std::string::compare).
 *
 * @param[in] key sets the lower bound (inclusive)
 *
 * @return iterator
 */
template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::lower_bound(const key_type &key)
{
	leaf_type *leaf = find_leaf_node(key);
	typename leaf_type::iterator leaf_it = leaf->lower_bound(key);
	if (leaf->end() == leaf_it)
		return end();

	return iterator(leaf, leaf_it);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::lower_bound(K &&key)
{
	leaf_type *leaf = find_leaf_node(std::forward<K>(key));
	typename leaf_type::iterator leaf_it = leaf->lower_bound(std::forward<K>(key));
	if (leaf->end() == leaf_it)
		return end();

	return iterator(leaf, leaf_it);
}

/**
 * Returns a const iterator pointing to the least element which is larger than or
 * equal to the given key. Keys are sorted in binary order (see
 * std::string::compare).
 *
 * @param[in] key sets the lower bound (inclusive)
 *
 * @return const_iterator
 */
template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::lower_bound(const key_type &key) const
{
	const leaf_type *leaf = find_leaf_node(key);
	typename leaf_type::const_iterator leaf_it = leaf->lower_bound(key);
	if (leaf->end() == leaf_it)
		return end();

	return const_iterator(leaf, leaf_it);
}

/**
 * Returns an iterator pointing to the least element which is larger than the
 * given key. Keys are sorted in binary order (see
 * std::string::compare).
 *
 * @param[in] key sets the lower bound (exclusive)
 *
 * @return iterator
 */
template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::upper_bound(const key_type &key)
{
	leaf_type *leaf = find_leaf_node(key);
	typename leaf_type::iterator leaf_it = leaf->upper_bound(key);
	if (leaf->end() == leaf_it)
		return end();

	return iterator(leaf, leaf_it);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::upper_bound(K &&key)
{
	leaf_type *leaf = find_leaf_node(std::forward<K>(key));
	typename leaf_type::iterator leaf_it = leaf->upper_bound(std::forward<K>(key));
	if (leaf->end() == leaf_it)
		return end();

	return iterator(leaf, leaf_it);
}

/**
 * Returns a const iterator pointing to the least element which is larger than the
 * given key. Keys are sorted in binary order (see
 * std::string::compare).
 *
 * @param[in] key sets the lower bound (exclusive)
 *
 * @return const_iterator
 */
template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::upper_bound(const key_type &key) const
{
	const leaf_type *leaf = find_leaf_node(key);
	typename leaf_type::const_iterator leaf_it = leaf->upper_bound(key);
	if (leaf->end() == leaf_it)
		return end();

	return const_iterator(leaf, leaf_it);
}

template <typename Key, typename T, typename Compare, size_t degree>
size_t b_tree_base<Key, T, Compare, degree>::erase(const key_type &key)
{
	leaf_type *leaf = find_leaf_node(key);
	auto pop = get_pool_base();
	return leaf->erase(pop, key);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K>
size_t b_tree_base<Key, T, Compare, degree>::erase(K &&key)
{
	assert(root != nullptr);

	using const_key = const key_type &;
	using inner_iter = typename inner_type::iterator;
	using inner_pair = std::pair<inner_pptr, inner_iter>;
	using neighbors_t = std::pair<node_pptr, node_pptr>;
	/* search leaf saving path, neighbors, inner node with key */
	std::vector<inner_pair> path;	    // [root, leaf)
	std::vector<neighbors_t> neighbors; // (root, leaf]
	inner_pair to_replace;
	node_pptr temp = root;
	while (!temp->leaf()) {
		auto set = cast_inner(temp)->get_child_and_siblings(std::forward<K>(key));
		path.push_back(std::make_pair(cast_inner(temp).get(), std::get<3>(set)));
		neighbors.push_back(std::make_pair(std::get<1>(set), std::get<2>(set)));
		if (!comparator(*std::get<3>(set), std::forward<K>(key)) &&
		    !comparator(std::forward<K>(key), *std::get<3>(set))) {
			assert(to_replace.first == nullptr);
			to_replace = std::make_pair(cast_inner(temp), std::get<3>(set));
		}
		temp = std::get<0>(set);
	}
	leaf_pptr leaf = cast_leaf(temp);
	leaf->check_consistency(epoch);
	/* inner_node key reference deleted at this point */
	/* functor to find leaf with suitable key to replace */
	auto get_suitable_leaf = [](inner_pair &pair) -> leaf_pptr {
		node_pptr temp = pair.first->get_right_child(pair.second);
		while (!temp->leaf())
			temp = cast_inner(temp)->get_left_child(
				cast_inner(temp)->begin());
		return cast_leaf(temp);
	};
	auto pop = get_pool_base();
	size_type result(1);
	pmem::obj::transaction::run(pop, [&] {
		/* remove entry */
		size_type deleted = leaf->erase(pop, std::forward<K>(key));
		if (!deleted) {
			result = size_type(0);
			return;
		}
		/* still left elements in leaf */
		if (leaf->size() > 0) {
			/* replace key with smallest in right subtree */
			if (to_replace.first != nullptr) {
				const_key new_key =
					get_suitable_leaf(to_replace)->front().first;
				to_replace.first->replace(to_replace.second, new_key);
			}
			return;
		}
		/* leaf is empty and it is root */
		if (path.empty()) {
			return;
		}
		/* handle leaf node */
		auto sibs = neighbors.back();
		auto parent = path.back();
		/* if left sibling exists then leaf is right child */
		if (sibs.first == nullptr) {
			parent.first->delete_with_child(parent.second, true);
		} else {
			parent.first->delete_with_child(parent.second, false);
		}
		deallocate(leaf);
		/* handle inner nodes */
		auto node = parent.first;
		while (path.size() > 1 && node->size() == 0) {
			path.pop_back();
			auto parent = path.back();
			sibs = neighbors.back();
			neighbors.pop_back();
			auto nbors = neighbors.back();

			if (sibs.first) {
				parent.first->inherit_child(parent.second, sibs.first,
							    nbors.first == nullptr);
			} else if (sibs.second) {
				parent.first->inherit_child(parent.second, sibs.second,
							    nbors.first == nullptr);
			}
			deallocate(node);

			node = parent.first;
		}
		if (to_replace.first) {
			const_key new_key = get_suitable_leaf(to_replace)->front().first;
			to_replace.first->replace(to_replace.second, new_key);
		}
		/* one of the main subtrees deleted, other will become root */
		if (path.back().first->size() == 0) {
			if (sibs.first) {
				cast_inner(root) = sibs.first;
			} else if (sibs.second) {
				cast_inner(root) = sibs.second;
			}
		}
	});
	/* all done, return */
	return result;
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::begin()
{
	return iterator(leftmost_leaf());
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::end()
{
	leaf_type *leaf = rightmost_leaf();
	return iterator(leaf, leaf->end());
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::begin() const
{
	return const_iterator(leftmost_leaf());
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::end() const
{
	const leaf_type *leaf = rightmost_leaf();
	return const_iterator(leaf, leaf->end());
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::cbegin() const
{
	return begin();
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::cend() const
{
	return end();
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::reverse_iterator
b_tree_base<Key, T, Compare, degree>::rbegin()
{
	return reverse_iterator(end());
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::reverse_iterator
b_tree_base<Key, T, Compare, degree>::rend()
{
	return reverse_iterator(begin());
}

template <typename Key, typename T, typename Compare, size_t degree>
const typename b_tree_base<Key, T, Compare, degree>::key_type &
b_tree_base<Key, T, Compare, degree>::get_last_key(const node_pptr &node)
{
	if (node->leaf()) {
		return cast_leaf(node.get())->back().first;
	} else {
		return cast_inner(node.get())->back();
	}
}

template <typename Key, typename T, typename Compare, size_t degree>
void b_tree_base<Key, T, Compare, degree>::create_new_root(pool_base &pop,
							   const key_type &key,
							   node_pptr &l_child,
							   node_pptr &r_child)
{
	assert(l_child != nullptr);
	assert(r_child != nullptr);
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	cast_inner(root) = allocate_inner(root->level() + 1, key, l_child, r_child);
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::inner_type::const_iterator
b_tree_base<Key, T, Compare, degree>::split_half(pool_base &pop, inner_pptr &node,
						 inner_pptr &other,
						 key_pptr &partition_key)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(other == nullptr);
	other = allocate_inner(node->level());
	return other->move(pop, *node, partition_key);
}

/* when root is the only inner node */
template <typename Key, typename T, typename Compare, size_t degree>
void b_tree_base<Key, T, Compare, degree>::split_inner_node(pool_base &pop,
							    inner_pptr &src_node)
{
	assert(root == src_node);
	pmem::obj::transaction::run(pop, [&] {
		node_pptr other(nullptr);
		key_pptr partition_key(nullptr);
		split_half(pop, src_node, cast_inner(other), partition_key);
		assert(partition_key != nullptr);
		create_new_root(pop, *partition_key,
				reinterpret_cast<node_pptr &>(src_node), other);
	});
}

/* when root is not the only inner node (2 or more inner node layers) */
template <typename Key, typename T, typename Compare, size_t degree>
void b_tree_base<Key, T, Compare, degree>::split_inner_node(pool_base &pop,
							    inner_pptr &src_node,
							    inner_type *parent_node)
{
	pmem::obj::transaction::run(pop, [&] {
		node_pptr other(nullptr);
		key_pptr partition_key(nullptr);
		split_half(pop, src_node, cast_inner(other), partition_key);
		assert(partition_key != nullptr);
		parent_node->update_splitted_child(
			pop, *partition_key, reinterpret_cast<node_pptr &>(src_node),
			other);
	});
}

/* split leaf in case when root is leaf */
template <typename Key, typename T, typename Compare, size_t degree>
template <typename K, typename M>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::split_leaf_node(pool_base &pop,
						      leaf_pptr &split_leaf, K &&key,
						      M &&obj)
{
	assert(split_leaf->full());

	leaf_pptr node;
	typename leaf_type::iterator entry_it;
	// move second half into node and insert new element where needed
	pmem::obj::transaction::run(pop, [&] {
		node = allocate_leaf();
		entry_it = node->move(pop, split_leaf, std::forward<K>(key),
				      std::forward<M>(obj));
		cast_inner(root) = allocate_inner(root->level() + 1, node->front().first,
						  split_leaf, node);
		// re-set node's pointers
		node->set_next(split_leaf->get_next());
		node->set_prev(split_leaf);
		if (split_leaf->get_next()) {
			split_leaf->get_next()->set_prev(node);
		}
		split_leaf->set_next(node);
	});

	assert(entry_it != node->end());
	assert(!comparator(entry_it->first, key) && !comparator(key, entry_it->first));
	return iterator(node.get(), entry_it);
}

/* split leaf in case when root is not leaf */
template <typename Key, typename T, typename Compare, size_t degree>
template <typename K, typename M>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::split_leaf_node(pool_base &pop,
						      inner_type *parent_node,
						      leaf_pptr &split_leaf, K &&key,
						      M &&obj)
{
	assert(split_leaf->full());

	leaf_pptr node;
	typename leaf_type::iterator entry_it;
	// move second half into node and insert new element where needed
	pmem::obj::transaction::run(pop, [&] {
		node = allocate_leaf();
		entry_it = node->move(pop, split_leaf, std::forward<K>(key),
				      std::forward<M>(obj));
		// take care of parent node
		parent_node->update_splitted_child(
			pop, node->front().first,
			reinterpret_cast<node_pptr &>(split_leaf),
			reinterpret_cast<node_pptr &>(node));
		// re-set node's pointers
		node->set_next(split_leaf->get_next());
		node->set_prev(split_leaf);
		if (split_leaf->get_next()) {
			split_leaf->get_next()->set_prev(node);
		}
		split_leaf->set_next(node);
	});

	assert(entry_it != node->end());
	assert(!comparator(entry_it->first, key) && !comparator(key, entry_it->first));
	return iterator(node.get(), entry_it);
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::find_leaf_node(const key_type &key) const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		node = cast_inner(node)->get_child(key);
	}
	leaf_type *leaf = cast_leaf(node).get();
	leaf->check_consistency(epoch);
	return leaf;
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::find_leaf_node(K &&key) const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		node = cast_inner(node)->get_child(std::forward<K>(key));
	}
	leaf_type *leaf = cast_leaf(node).get();
	leaf->check_consistency(epoch);
	return leaf;
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::leaf_pptr
b_tree_base<Key, T, Compare, degree>::find_leaf_to_insert(K &&key, path_type &path) const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		path.push_back(cast_inner(node));
		node = cast_inner(node)->get_child(std::forward<K>(key));
	}
	leaf_pptr leaf = cast_leaf(node);
	leaf->check_consistency(epoch);
	return leaf;
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::path_type::const_iterator
b_tree_base<Key, T, Compare, degree>::find_full_node(const path_type &path)
{
	auto i = path.end() - 1;
	for (; i > path.begin(); --i) {
		if (!(*i)->full())
			return i;
	}
	return i;
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::key_compare &
b_tree_base<Key, T, Compare, degree>::key_comp()
{
	return comparator;
}

template <typename Key, typename T, typename Compare, size_t degree>
const typename b_tree_base<Key, T, Compare, degree>::key_compare &
b_tree_base<Key, T, Compare, degree>::key_comp() const
{
	return comparator;
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::leftmost_leaf() const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		inner_type *inner_node = cast_inner(node).get();
		node = inner_node->get_left_child(inner_node->begin());
	}
	leaf_type *leaf = cast_leaf(node).get();
	leaf->check_consistency(epoch);
	return leaf;
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::rightmost_leaf() const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		inner_type *inner_node = cast_inner(node).get();
		node = inner_node->get_left_child(inner_node->end());
	}
	leaf_type *leaf = cast_leaf(node).get();
	leaf->check_consistency(epoch);
	return leaf;
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::inner_pptr &
b_tree_base<Key, T, Compare, degree>::cast_inner(node_pptr &node)
{
	return reinterpret_cast<inner_pptr &>(node);
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::inner_type *
b_tree_base<Key, T, Compare, degree>::cast_inner(node_t *node)
{
	return static_cast<inner_type *>(node);
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_pptr &
b_tree_base<Key, T, Compare, degree>::cast_leaf(node_pptr &node)
{
	return reinterpret_cast<leaf_pptr &>(node);
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::cast_leaf(node_t *node)
{
	return static_cast<leaf_type *>(node);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename... Args>
inline typename b_tree_base<Key, T, Compare, degree>::inner_pptr
b_tree_base<Key, T, Compare, degree>::allocate_inner(Args &&... args)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	return make_persistent<inner_type>(comparator, std::forward<Args>(args)...);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename... Args>
inline typename b_tree_base<Key, T, Compare, degree>::leaf_pptr
b_tree_base<Key, T, Compare, degree>::allocate_leaf(Args &&... args)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	return make_persistent<leaf_type>(comparator, epoch, std::forward<Args>(args)...);
}

template <typename Key, typename T, typename Compare, size_t degree>
inline void b_tree_base<Key, T, Compare, degree>::deallocate(node_pptr &node)
{
	assert(node != nullptr);
	if (node->leaf()) {
		deallocate(cast_leaf(node));
	} else {
		deallocate(cast_inner(node));
	}
}

template <typename Key, typename T, typename Compare, size_t degree>
inline void b_tree_base<Key, T, Compare, degree>::deallocate(leaf_pptr &node)
{
	assert(node != nullptr);
	pool_base pop = get_pool_base();
	pmem::obj::transaction::run(pop, [&] {
		delete_persistent<leaf_type>(node);
		node = nullptr;
	});
}

template <typename Key, typename T, typename Compare, size_t degree>
inline void b_tree_base<Key, T, Compare, degree>::deallocate(inner_pptr &node)
{
	assert(node != nullptr);
	pool_base pop = get_pool_base();
	pmem::obj::transaction::run(pop, [&] {
		delete_persistent<inner_type>(node);
		node = nullptr;
	});
}

template <typename Key, typename T, typename Compare, size_t degree>
PMEMobjpool *b_tree_base<Key, T, Compare, degree>::get_objpool()
{
	PMEMoid oid = pmemobj_oid(this);
	return pmemobj_pool_by_oid(oid);
}

template <typename Key, typename T, typename Compare, size_t degree>
pool_base b_tree_base<Key, T, Compare, degree>::get_pool_base()
{
	return pool_base(get_objpool());
}

} /* namespace internal */

template <typename Key, typename Value, typename Compare = std::less<Key>,
	  size_t degree = 64>
class b_tree : public internal::b_tree_base<Key, Value, Compare, degree> {
private:
	using base_type = internal::b_tree_base<Key, Value, Compare, degree>;

public:
	using base_type::begin;
	using base_type::end;
	using base_type::erase;
	using base_type::find;
	using base_type::insert;

	// Type definitions
	using key_type = typename base_type::key_type;
	using mapped_type = typename base_type::mapped_type;
	using value_type = typename base_type::value_type;
	using iterator = typename base_type::iterator;
	using const_iterator = typename base_type::const_iterator;
	using reverse_iterator = typename base_type::reverse_iterator;

	explicit b_tree() : base_type()
	{
	}

	~b_tree()
	{
		~base_type();
	}

	b_tree(const b_tree &) = delete;
	b_tree &operator=(const b_tree &) = delete;
};

} // namespace persistent
#endif // PERSISTENT_B_TREE
