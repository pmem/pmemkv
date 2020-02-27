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

	using value_type = std::pair<key_type, mapped_type>;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;

	using iterator = node_iterator<leaf_node_t, false>;
	using const_iterator = node_iterator<leaf_node_t, true>;

	leaf_node_t(uint64_t e);
	~leaf_node_t();

	template <typename K, typename M>
	iterator move(pool_base &pop, persistent_ptr<leaf_node_t> other, K &&key,
		      M &&obj);

	std::pair<iterator, bool> insert(pool_base &pop, const_reference entry);
	template <typename K, typename M>
	std::pair<iterator, bool> insert(pool_base &pop, K &&key, M &&obj);

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
	/**
	 * Array of indexes.
	 */
	struct leaf_entries_t {
		leaf_entries_t() : _size(0)
		{
			std::iota(std::begin(idxs), std::end(idxs), 0);
		}

		~leaf_entries_t()
		{
		}

		uint64_t idxs[capacity];
		size_t _size;
	};
	uint64_t epoch;
	uint32_t consistent_id;
	uint32_t p_consistent_id;
	leaf_entries_t v[2];
	union {
		value_type entries[capacity];
	};
	pmem::obj::persistent_ptr<leaf_node_t> prev;
	pmem::obj::persistent_ptr<leaf_node_t> next;
	key_compare comparator;

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

public:
	inner_node_t(size_type level, const_reference key, const node_pptr &first_child,
		     const node_pptr &second_child);
	inner_node_t(size_type level, const_iterator first, const_iterator last,
		     const inner_node_t *src);
	~inner_node_t();

	void update_splitted_child(pool_base &pop, const_reference key,
				   node_pptr &left_child, node_pptr &right_child);

	template <typename K>
	const node_pptr &get_child(K &&key) const;
	const node_pptr &get_child(const_reference key) const;
	const node_pptr &get_left_child(const_iterator it) const;
	const node_pptr &get_right_child(const_iterator it) const;

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

private:
	const static size_type child_capacity = capacity + 1;

	key_type entries[capacity];
	node_pptr children[child_capacity];
	size_type _size = 0;
	key_compare comparator;

	template <typename K>
	iterator lower_bound(K &&key);
	bool is_valid();
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
	using node_pptr = persistent_ptr<node_t>;
	using leaf_node_pptr = persistent_ptr<leaf_type>;
	using inner_node_pptr = persistent_ptr<inner_type>;
	using path_type = std::vector<inner_node_pptr>;

public:
	using value_type = typename leaf_type::value_type;
	using key_type = typename leaf_type::key_type;
	using mapped_type = typename leaf_type::mapped_type;
	using key_compare = Compare;

	using reference = typename leaf_type::reference;
	using const_reference = typename leaf_type::const_reference;
	using pointer = typename leaf_type::pointer;
	using const_pointer = typename leaf_type::const_pointer;

	using iterator = b_tree_iterator<leaf_type, false>;
	using const_iterator = b_tree_iterator<leaf_type, true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	b_tree_base(const key_compare &comp);
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

	void garbage_collection();

private:
	uint64_t epoch;
	persistent_ptr<node_t> root;
	persistent_ptr<node_t> split_node;
	persistent_ptr<node_t> left_child;
	persistent_ptr<node_t> right_child;
	key_compare comparator;

	static const key_type &get_last_key(const node_pptr &node);
	void create_new_root(pool_base &, const key_type &, node_pptr &, node_pptr &);
	std::pair<iterator, bool> insert_descend(pool_base &, const_reference);

	typename inner_type::const_iterator split_half(pool_base &pop,
						       persistent_ptr<inner_type> &node,
						       persistent_ptr<node_t> &left,
						       persistent_ptr<node_t> &right);
	void split_inner_node(pool_base &pop, inner_node_pptr &src_node,
			      inner_type *parent_node, node_pptr &left, node_pptr &right);
	iterator split_leaf_node(pool_base &, inner_type *, persistent_ptr<node_t> &,
				 const_reference, persistent_ptr<node_t> &,
				 persistent_ptr<node_t> &);
	template <typename K, typename M>
	iterator split_leaf_node(pool_base &, inner_type *, leaf_node_pptr &, K &&, M &&);

	static bool is_left_node(const leaf_type *src_node, const leaf_type *lnode);
	static bool is_right_node(const leaf_type *src_node, const leaf_type *rnode);

	void repair_leaf_split(pool_base &pop);
	void repair_inner_split(pool_base &pop);
	void correct_leaf_node_links(pool_base &, persistent_ptr<node_t> &,
				     persistent_ptr<node_t> &, persistent_ptr<node_t> &);

	void assignment(pool_base &pop, persistent_ptr<node_t> &lhs,
			const persistent_ptr<node_t> &rhs);

	leaf_type *find_leaf_node(const key_type &key) const;
	template <typename K>
	leaf_type *find_leaf_node(K &&key) const;
	// TODO: merge with previous method
	leaf_node_pptr find_leaf_to_insert(const key_type &key, path_type &path) const;
	template <typename K>
	leaf_node_pptr find_leaf_to_insert(K &&key, path_type &path) const;
	typename path_type::const_iterator find_full_node(const path_type &path);

	leaf_type *leftmost_leaf() const;
	leaf_type *rightmost_leaf() const;

	static inner_node_pptr &cast_inner(persistent_ptr<node_t> &node);
	static inner_type *cast_inner(node_t *node);
	static persistent_ptr<leaf_type> &cast_leaf(persistent_ptr<node_t> &node);
	static leaf_type *cast_leaf(node_t *node);

	template <typename... Args>
	inline inner_node_pptr allocate_inner(pool_base &pop, Args &&... args);
	template <typename... Args>
	inline persistent_ptr<leaf_type> allocate_leaf(pool_base &pop, Args &&... args);
	inline void deallocate(persistent_ptr<node_t> &node);
	inline void deallocate_inner(inner_node_pptr &node);
	inline void deallocate_leaf(leaf_node_pptr &node);

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
	assert(position >= off);
	return node_iterator(node, position - off);
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
leaf_node_t<Key, T, Compare, capacity>::leaf_node_t(uint64_t e)
    : node_t(), epoch(e), consistent_id(0), p_consistent_id(0)
{
	assert(is_sorted());
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
leaf_node_t<Key, T, Compare, capacity>::~leaf_node_t()
{
	for (iterator temp = begin(); temp != end(); ++temp) {
		(*temp).~value_type();
	}
	if (prev) {
		delete_persistent<leaf_node_t>(prev);
	}
	if (next) {
		delete_persistent<leaf_node_t>(next);
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
	auto middle = std::make_move_iterator(other->begin() + other->size() / 2);
	auto last = std::make_move_iterator(other->end());
	auto temp = middle;
	bool less = comparator(std::forward<K>(key), middle->first);
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
	if (hint != end() && hint->first == entry.first) {
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
	if (hint != end() && hint->first == key) {
		return std::pair<iterator, bool>(hint, false);
	}

	size_t insert_pos = get_insert_idx();
	assert(std::none_of(consistent()->idxs, consistent()->idxs + size(),
			    [insert_pos](uint64_t idx) { return insert_pos == idx; }));
	// insert an entry to the end
	pmem::obj::transaction::run(pop, [&] {
		emplace(insert_pos, std::forward<K>(key), std::forward<K>(obj));
	});
	// update tmp idxs
	size_t position = insert_idx(pop, insert_pos, hint);
	// update consistent
	switch_consistent(pop);

	assert(is_sorted());
	return std::pair<iterator, bool>(iterator(this, position), true);
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::find(K &&key)
{
	assert(is_sorted());

	iterator it = lower_bound(std::forward<K>(key));
	if (it == end() || it->first == key) {
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
	if (it == end() || it->first == key) {
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
	assert(is_sorted());
	iterator it, first = begin(), last = end();
	typename iterator::difference_type count, step;
	count = std::distance(first, last);

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if (comparator((*it).first, std::forward<K>(key))) {
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
	assert(is_sorted());
	return std::lower_bound(begin(), end(), std::forward<K>(key),
				[this](const_reference entry, const K &key) {
					return comparator(entry.first,
							  std::forward<K>(key));
				});
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::upper_bound(K &&key)
{
	assert(is_sorted());
	iterator it, first = begin(), last = end();
	typename iterator::difference_type count, step;
	count = std::distance(first, last);

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if (!comparator(key, (*it).first)) {
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
leaf_node_t<Key, T, Compare, capacity>::upper_bound(K &&key) const
{
	assert(is_sorted());
	return std::upper_bound(begin(), end(), std::forward<K>(key),
				[this](const K &key, const_reference entry) {
					return comparator(std::forward<K>(key),
							  entry.first);
				});
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
					 return comparator(a.first, b.first);
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
	// update tmp idxs
	remove_idx(pop, idx);
	pmem::obj::transaction::run(pop, [&] {
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
				      return comparator(a.first, b.first);
			      });
}

// -------------------------------------------------------------------------------------
// ------------------------------------- inner_node_t ----------------------------------
// -------------------------------------------------------------------------------------

template <typename Key, typename Compare, uint64_t capacity>
inner_node_t<Key, Compare, capacity>::inner_node_t(size_type level, const_reference key,
						   const node_pptr &first_child,
						   const node_pptr &second_child)
    : node_t(level)
{
	entries[0] = key;
	children[0] = first_child;
	children[1] = second_child;
	_size = 1;

	assert(is_valid());
	assert(is_sorted());
}

template <typename Key, typename Compare, uint64_t capacity>
inner_node_t<Key, Compare, capacity>::inner_node_t(size_type level, const_iterator first,
						   const_iterator last,
						   const inner_node_t *src)
    : node_t(level)
{
	size_type new_size = static_cast<size_type>(std::distance(first, last));
	assert(new_size > 0);
	std::copy(first, last, begin());
	_size = new_size;

	const node_pptr *first_child =
		std::next(src->children, std::distance(src->begin(), first));
	const node_pptr *last_child =
		std::next(first_child, static_cast<difference_type>(_size + 1));
	std::copy(first_child, last_child, this->children);

	assert(is_valid());
	assert(is_sorted());
}

template <typename Key, typename Compare, uint64_t capacity>
inner_node_t<Key, Compare, capacity>::~inner_node_t()
{
	for (size_t i = 0; i <= _size; ++i) {
		delete_persistent<node_t>(children[i]);
	}
	delete_persistent<node_t>(children[_size]);
}

/**
 * Updates inner node after splitting child.
 *
 *               [this]
 *               /
 *              /
 *   [left_child]       [new_child]
 *
 * @param[in] pop - persistent pool
 * @param[in] key - key of the last entry in left_child
 * @param[in] new_child - new child node that must be linked
 */
template <typename Key, typename Compare, uint64_t capacity>
void inner_node_t<Key, Compare, capacity>::update_splitted_child(pool_base &pop,
								 const_reference key,
								 node_pptr &left_child,
								 node_pptr &right_child)
{
	assert(!full());
	iterator insert_it = lower_bound(key);
	difference_type insert_idx = std::distance(begin(), insert_it);
	/* update entries inserting new key */
	iterator to_insert = std::copy_backward(insert_it, end(), end() + 1);
	assert(insert_idx < std::distance(begin(), to_insert));
	*(--to_insert) = key;
	_size = _size + 1;
	/* update children inserting new descendants */
	node_pptr *to_insert_child = std::copy_backward(
		children + insert_idx + 1, children + _size, children + _size + 1);
	*(--to_insert_child) = right_child;
	*(--to_insert_child) = left_child;

	assert(is_sorted());
	assert(is_valid());
}

template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
const typename inner_node_t<Key, Compare, capacity>::node_pptr &
inner_node_t<Key, Compare, capacity>::get_child(K &&key) const
{
	auto it = std::lower_bound(begin(), end(), std::forward<K>(key));
	return get_left_child(it);
}

template <typename Key, typename Compare, uint64_t capacity>
const typename inner_node_t<Key, Compare, capacity>::node_pptr &
inner_node_t<Key, Compare, capacity>::get_child(const_reference key) const
{
	auto it = std::lower_bound(begin(), end(), key);
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
	size_type child_pos = std::distance(begin(), it) + 1;
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
	return entries[this->size() - 1];
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::reference
inner_node_t<Key, Compare, capacity>::at(size_type pos)
{
	if (size() <= pos) {
		throw std::out_of_range("Accessing incorrect element in inner node");
	}
	return entries[pos];
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_reference
inner_node_t<Key, Compare, capacity>::at(size_type pos) const
{
	if (size() <= pos) {
		throw std::out_of_range("Accessing incorrect element inner node");
	}
	return entries[pos];
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::reference
	inner_node_t<Key, Compare, capacity>::operator[](size_type pos)
{
	return entries[pos];
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_reference
	inner_node_t<Key, Compare, capacity>::operator[](size_type pos) const
{
	return entries[pos];
}

template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
typename inner_node_t<Key, Compare, capacity>::iterator
inner_node_t<Key, Compare, capacity>::lower_bound(K &&key)
{
	assert(is_sorted());
	iterator it, first = begin(), last = end();
	typename iterator::difference_type count, step;
	count = std::distance(first, last);

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if (comparator(*it, std::forward<K>(key))) {
			first = ++it;
			count -= step + 1;
		} else
			count = step;
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
	return std::is_sorted(begin(), end(), comparator);
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
			leaf_it = current_node->last();
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
b_tree_base<Key, T, Compare, degree>::b_tree_base(const key_compare &comp)
    : epoch(0), comparator(comp)
{
	root = make_persistent<leaf_type>(epoch);
}

template <typename Key, typename T, typename Compare, size_t degree>
b_tree_base<Key, T, Compare, degree>::~b_tree_base()
{
	deallocate(root);
}

template <typename Key, typename T, typename Compare, size_t degree>
std::pair<typename b_tree_base<Key, T, Compare, degree>::iterator, bool>
b_tree_base<Key, T, Compare, degree>::insert(const_reference entry)
{
	auto pop = get_pool_base();

	if (root == nullptr) {
		pmem::obj::transaction::run(pop, [&] { allocate_leaf(pop, root); });
	}
	assert(root != nullptr);

	std::pair<iterator, bool> ret = insert_descend(pop, entry);

	return ret;
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K, typename M>
std::pair<typename b_tree_base<Key, T, Compare, degree>::iterator, bool>
b_tree_base<Key, T, Compare, degree>::try_emplace(K &&key, M &&obj)
{
	auto pop = get_pool_base();
	assert(root != nullptr);

	// ---------------- find leaf to insert -------------------
	path_type path;
	persistent_ptr<node_t> node = root;
	while (!node->leaf()) {
		path.push_back(cast_inner(node));
		node = cast_inner(node)->get_child(key);
	}

	leaf_type *leaf = cast_leaf(node).get();
	leaf->check_consistency(epoch);

	// ---------------- if leaf not full just insert ---------------
	if (!leaf->full()) {
		std::pair<typename leaf_type::iterator, bool> ret =
			leaf->insert(pop, std::forward<K>(key), std::forward<K>(obj));
		return std::pair<iterator, bool>(iterator(leaf, ret.first), ret.second);
	}

	// ------------ if entry with the same key found ------------
	typename leaf_type::iterator leaf_it = leaf->find(std::forward<K>(key));
	if (leaf_it != leaf->end()) {
		return std::pair<iterator, bool>(iterator(leaf, leaf_it), false);
	}

	// ------------------- if root is leaf ----------------------
	if (path.empty()) {
		iterator it = split_leaf_node(pop, nullptr, cast_leaf(node),
					      std::forward<K>(key), std::forward<K>(obj));
		return std::pair<iterator, bool>(it, true);
	}

	// --------------- find the first not full node ----------------
	auto i = path.end() - 1;
	for (; i > path.begin(); --i) {
		if (!(*i)->full())
			break;
	}

	inner_type *parent_node = nullptr;
	// -------------- if root is full split root ----------------
	if ((*i)->full()) {
		split_inner_node(pop, *i, parent_node, left_child, right_child);
		parent_node = cast_inner(
			cast_inner(root)->get_child(std::forward<K>(key)).get());
	} else {
		parent_node = (*i).get();
	}
	++i;

	for (; i != path.end(); ++i) {
		split_inner_node(pop, *i, parent_node, left_child, right_child);
		parent_node =
			cast_inner(parent_node->get_child(std::forward<K>(key)).get());
	}

	iterator it = split_leaf_node(pop, parent_node, cast_leaf(node),
				      std::forward<K>(key), std::forward<K>(obj));
	return std::pair<iterator, bool>(it, true);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::find(K &&key)
{
	leaf_type *leaf = find_leaf_node(std::forward<K>(key));
	if (leaf == nullptr)
		return end();

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
	if (leaf == nullptr)
		return end();

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
	if (leaf == nullptr)
		return end();

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
	if (leaf == nullptr)
		return end();

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
	if (leaf == nullptr)
		return end();

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
	if (leaf == nullptr)
		return end();

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
	if (leaf == nullptr)
		return end();

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
	if (leaf == nullptr)
		return end();

	typename leaf_type::const_iterator leaf_it = leaf->upper_bound(key);
	if (leaf->end() == leaf_it)
		return end();

	return const_iterator(leaf, leaf_it);
}

template <typename Key, typename T, typename Compare, size_t degree>
size_t b_tree_base<Key, T, Compare, degree>::erase(const key_type &key)
{
	leaf_type *leaf = find_leaf_node(key);
	if (leaf == nullptr)
		return size_t(0);
	auto pop = get_pool_base();
	return leaf->erase(pop, key);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K>
size_t b_tree_base<Key, T, Compare, degree>::erase(K &&key)
{
	leaf_type *leaf = find_leaf_node(std::forward<K>(key));
	if (leaf == nullptr)
		return size_t(0);
	auto pop = get_pool_base();
	return leaf->erase(pop, std::forward<K>(key));
}

template <typename Key, typename T, typename Compare, size_t degree>
void b_tree_base<Key, T, Compare, degree>::garbage_collection()
{
	pool_base pop = get_pool_base();
	++epoch;
	// pop.persist( &epoch, sizeof(epoch) );

	if (split_node != nullptr) {
		if (split_node->leaf()) {
			repair_leaf_split(pop);
		} else {
			repair_inner_split(pop);
		}
	}
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
	return iterator(leaf, leaf ? leaf->end() : typename leaf_type::iterator());
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
	return const_iterator(leaf,
			      leaf ? leaf->end() : typename leaf_type::const_iterator());
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

	pmem::obj::transaction::run(pop, [&] {
		cast_inner(root) =
			allocate_inner(pop, root->level() + 1, key, l_child, r_child);
	});
}

template <typename Key, typename T, typename Compare, size_t degree>
std::pair<typename b_tree_base<Key, T, Compare, degree>::iterator, bool>
b_tree_base<Key, T, Compare, degree>::insert_descend(pool_base &pop,
						     const_reference entry)
{
	path_type path;
	const key_type &key = entry.first;

	node_pptr node = find_leaf_to_insert(key, path);
	leaf_type *leaf = cast_leaf(node).get();
	inner_type *parent_node = nullptr;

	if (leaf->full()) {
		typename leaf_type::iterator leaf_it = leaf->find(key);
		if (leaf_it != leaf->end()) { // Entry with the same key found
			return std::pair<iterator, bool>(iterator(leaf, leaf_it), false);
		}

		/**
		 * If root is leaf.
		 */
		if (path.empty()) {
			iterator it = split_leaf_node(pop, nullptr, node, entry,
						      left_child, right_child);
			return std::pair<iterator, bool>(it, true);
		}

		// find the first full node
		auto i = find_full_node(path);

		/**
		 * If root is full. Split root
		 */
		if ((*i)->full()) {
			parent_node = nullptr;
			split_inner_node(pop, *i, parent_node, left_child, right_child);
			parent_node = cast_inner(cast_inner(root)->get_child(key).get());
		} else {
			parent_node = (*i).get();
		}
		++i;

		for (; i != path.end(); ++i) {
			split_inner_node(pop, *i, parent_node, left_child, right_child);

			parent_node = cast_inner(parent_node->get_child(key).get());
		}

		iterator it = split_leaf_node(pop, parent_node, node, entry, left_child,
					      right_child);
		return std::pair<iterator, bool>(it, true);
	}

	std::pair<typename leaf_type::iterator, bool> ret = leaf->insert(pop, entry);
	return std::pair<iterator, bool>(iterator(leaf, ret.first), ret.second);
	;
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::inner_type::const_iterator
b_tree_base<Key, T, Compare, degree>::split_half(pool_base &pop,
						 persistent_ptr<inner_type> &node,
						 persistent_ptr<node_t> &left,
						 persistent_ptr<node_t> &right)
{
	typename inner_type::const_iterator middle = node->begin() + node->size() / 2;
	typename inner_type::const_iterator l_begin = node->begin();
	typename inner_type::const_iterator l_end = middle;
	typename inner_type::const_iterator r_begin = middle + 1;
	typename inner_type::const_iterator r_end = node->end();
	pmem::obj::transaction::run(pop, [&] {
		cast_inner(left) =
			allocate_inner(pop, node->level(), l_begin, l_end, node.get());
		cast_inner(right) =
			allocate_inner(pop, node->level(), r_begin, r_end, node.get());
	});

	return middle;
}

template <typename Key, typename T, typename Compare, size_t degree>
void b_tree_base<Key, T, Compare, degree>::split_inner_node(pool_base &pop,
							    inner_node_pptr &src_node,
							    inner_type *parent_node,
							    node_pptr &left,
							    node_pptr &right)
{
	typename inner_type::const_iterator partition_point =
		split_half(pop, src_node, left, right);
	pmem::obj::transaction::run(pop, [&] {
		if (parent_node) {
			parent_node->update_splitted_child(pop, *partition_point, left,
							   right);
		} else {
			assert(root == src_node);
			create_new_root(pop, *partition_point, left, right);
		}
	});
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K, typename M>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::split_leaf_node(pool_base &pop,
						      inner_type *parent_node,
						      leaf_node_pptr &split_leaf, K &&key,
						      M &&obj)
{
	assert(split_leaf->full());

	leaf_node_pptr node;
	typename leaf_type::iterator entry_it;
	// move second half into node and insert new element where needed
	pmem::obj::transaction::run(pop, [&] {
		node = allocate_leaf(pop);
		entry_it = node->move(pop, split_leaf, std::forward<K>(key),
				      std::forward<M>(obj));
		// re-set node's pointers
		node->set_next(split_leaf->get_next());
		node->set_prev(split_leaf);
		if (split_leaf->get_next()) {
			split_leaf->get_next()->set_prev(node);
		}
		split_leaf->set_next(node);
		// take care of parent node
		if (parent_node) {
			parent_node->update_splitted_child(
				pop, split_leaf->back().first,
				reinterpret_cast<node_pptr &>(split_leaf),
				reinterpret_cast<node_pptr &>(node));
		} else {
			cast_inner(root) = allocate_inner(pop, root->level() + 1,
							  split_leaf->back().first,
							  split_leaf, node);
		}
	});

	assert(entry_it != node->end());
	assert(entry_it->first == key);
	assert(entry_it->second == obj);
	return iterator(node.get(), entry_it);
}

template <typename Key, typename T, typename Compare, size_t degree>
bool b_tree_base<Key, T, Compare, degree>::is_left_node(const leaf_type *src_node,
							const leaf_type *lnode)
{
	assert(src_node);
	assert(lnode);
	typename leaf_type::const_iterator middle =
		src_node->begin() + src_node->size() / 2;

	return std::includes(lnode->begin(), lnode->end(), src_node->begin(), middle);
}

template <typename Key, typename T, typename Compare, size_t degree>
bool b_tree_base<Key, T, Compare, degree>::is_right_node(const leaf_type *src_node,
							 const leaf_type *rnode)
{
	assert(src_node);
	assert(rnode);
	typename leaf_type::const_iterator middle =
		src_node->begin() + src_node->size() / 2;

	return std::includes(rnode->begin(), rnode->end(), middle, src_node->end());
}

template <typename Key, typename T, typename Compare, size_t degree>
void b_tree_base<Key, T, Compare, degree>::repair_leaf_split(pool_base &pop)
{
	assert(root != nullptr);
	assert(split_node != nullptr);
	assert(split_node->leaf());

	const key_type &key = get_last_key(split_node);
	path_type path;

	leaf_node_pptr found_node = find_leaf_to_insert(key, path);
	assert(path[0] == root);

	if (split_node == found_node) { // Split not completed
		const leaf_type *split_leaf = cast_leaf(split_node).get();
		leaf_type *lnode = cast_leaf(left_child).get();
		leaf_type *rnode = cast_leaf(right_child).get();

		if (left_child && is_left_node(split_leaf, lnode)) {
			if (right_child &&
			    is_right_node(split_leaf,
					  rnode)) { // Both children were allocated
						    // during split before crash
				inner_type *parent_node =
					path.empty() ? nullptr : path.back().get();

				lnode->set_next(cast_leaf(right_child));
				pop.persist(lnode->get_next());

				correct_leaf_node_links(pop, split_node, left_child,
							right_child);

				if (parent_node) {
					parent_node->update_splitted_child(
						pop, lnode->back().first, left_child,
						right_child);
				} else {
					create_new_root(pop, lnode->back().first,
							left_child, right_child);
				}
			} else { // Only left child was allocated during split
				 // before crash
				deallocate(left_child);
			}
		}
	} else { // split_node was replaced by two new (left and right) nodes.
		 // Need to deallocate split_node
		deallocate(split_node);
	}
	split_node = nullptr;
}

template <typename Key, typename T, typename Compare, size_t degree>
void b_tree_base<Key, T, Compare, degree>::repair_inner_split(pool_base &pop)
{
	assert(root != nullptr);
	assert(!root->leaf());
	assert(split_node != nullptr);
	assert(!split_node->leaf());

	const key_type &key = get_last_key(split_node);
	path_type path;

	find_leaf_to_insert(key, path);
	assert(path[0] == root);
	uint64_t root_level = path[0]->level();
	uint64_t split_level = split_node->level();
	assert(split_level <= root_level);

	if (split_node == path[root_level - split_level]) { // Split not completed
		// we could simply roll back
		const inner_type *inner = cast_inner(split_node).get();
		typename inner_type::const_iterator middle =
			inner->begin() + inner->size() / 2;
		if (left_child && !(left_child->leaf()) &&
		    std::equal(inner->begin(), middle, cast_inner(left_child)->begin())) {
			deallocate(left_child);
		}
		if (right_child && !(right_child->leaf()) &&
		    std::equal(middle + 1, inner->end(),
			       cast_inner(right_child)->begin())) {
			deallocate(right_child);
		}
	} else { // split_node was replaced by two new (left and right) nodes.
		 // Need to deallocate split_node
		deallocate(split_node);
	}
	split_node = nullptr;
}

template <typename Key, typename T, typename Compare, size_t degree>
void b_tree_base<Key, T, Compare, degree>::correct_leaf_node_links(
	pool_base &pop, persistent_ptr<node_t> &src_node, persistent_ptr<node_t> &left,
	persistent_ptr<node_t> &right)
{
	leaf_node_pptr lnode = cast_leaf(left);
	leaf_node_pptr rnode = cast_leaf(right);
	leaf_type *current_node = cast_leaf(src_node).get();

	if (current_node->get_prev() != nullptr) {
		current_node->get_prev()->set_next(lnode);
		pop.persist(current_node->get_prev()->get_next());
	}

	if (current_node->get_next() != nullptr) {
		current_node->get_next()->set_prev(rnode);
		pop.persist(current_node->get_next()->get_prev());
	}
}

template <typename Key, typename T, typename Compare, size_t degree>
void b_tree_base<Key, T, Compare, degree>::assignment(pool_base &pop,
						      persistent_ptr<node_t> &lhs,
						      const persistent_ptr<node_t> &rhs)
{
	// lhs.raw_ptr()->off = rhs.raw_ptr()->off;
	lhs = rhs;
	pop.persist(lhs);
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::find_leaf_node(const key_type &key) const
{
	if (root == nullptr)
		return nullptr;

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
	if (root == nullptr)
		return nullptr;

	node_pptr node = root;
	while (!node->leaf()) {
		node = cast_inner(node)->get_child(std::forward<K>(key));
	}
	leaf_type *leaf = cast_leaf(node).get();
	leaf->check_consistency(epoch);
	return leaf;
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_node_pptr
b_tree_base<Key, T, Compare, degree>::find_leaf_to_insert(const key_type &key,
							  path_type &path) const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		path.push_back(cast_inner(node));

		node = cast_inner(node)->get_child(key);
	}
	leaf_node_pptr leaf = cast_leaf(node);
	leaf->check_consistency(epoch);
	return leaf;
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::leaf_node_pptr
b_tree_base<Key, T, Compare, degree>::find_leaf_to_insert(K &&key, path_type &path) const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		path.push_back(cast_inner(node));
		node = cast_inner(node)->get_child(key);
	}
	leaf_node_pptr leaf = cast_leaf(node);
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
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::leftmost_leaf() const
{
	if (root == nullptr)
		return nullptr;

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
	if (root == nullptr)
		return nullptr;

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
typename b_tree_base<Key, T, Compare, degree>::inner_node_pptr &
b_tree_base<Key, T, Compare, degree>::cast_inner(persistent_ptr<node_t> &node)
{
	return reinterpret_cast<inner_node_pptr &>(node);
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::inner_type *
b_tree_base<Key, T, Compare, degree>::cast_inner(node_t *node)
{
	return static_cast<inner_type *>(node);
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_node_pptr &
b_tree_base<Key, T, Compare, degree>::cast_leaf(persistent_ptr<node_t> &node)
{
	return reinterpret_cast<leaf_node_pptr &>(node);
}

template <typename Key, typename T, typename Compare, size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::cast_leaf(node_t *node)
{
	return static_cast<leaf_type *>(node);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename... Args>
inline typename b_tree_base<Key, T, Compare, degree>::inner_node_pptr
b_tree_base<Key, T, Compare, degree>::allocate_inner(pool_base &pop, Args &&... args)
{
	return make_persistent<inner_type>(std::forward<Args>(args)...);
}

template <typename Key, typename T, typename Compare, size_t degree>
template <typename... Args>
inline typename b_tree_base<Key, T, Compare, degree>::leaf_node_pptr
b_tree_base<Key, T, Compare, degree>::allocate_leaf(pool_base &pop, Args &&... args)
{
	return make_persistent<leaf_type>(epoch, std::forward<Args>(args)...);
}

template <typename Key, typename T, typename Compare, size_t degree>
inline void b_tree_base<Key, T, Compare, degree>::deallocate(persistent_ptr<node_t> &node)
{
	if (node == nullptr)
		return;

	pool_base pop = get_pool_base();
	pmem::obj::transaction::run(pop, [&] {
		if (node->leaf()) {
			deallocate_leaf(cast_leaf(node));
		} else {
			deallocate_inner(cast_inner(node));
		}
		node = nullptr;
	});
}

template <typename Key, typename T, typename Compare, size_t degree>
inline void b_tree_base<Key, T, Compare, degree>::deallocate_inner(inner_node_pptr &node)
{
	delete_persistent<inner_type>(node);
}

template <typename Key, typename T, typename Compare, size_t degree>
inline void b_tree_base<Key, T, Compare, degree>::deallocate_leaf(leaf_node_pptr &node)
{
	delete_persistent<leaf_type>(node);
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

	explicit b_tree(const Compare &comp = Compare()) : base_type(comp)
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
