// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#ifndef PERSISTENT_B_TREE
#define PERSISTENT_B_TREE

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <numeric>
#include <type_traits>
#include <vector>

#include <cassert>

namespace pmem
{
namespace kv
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
	using size_type = typename leaf_type::size_type;
	using difference_type = typename leaf_type::difference_type;
	using reference = typename std::conditional<is_const, const value_type &,
						    value_type &>::type;
	using pointer = typename std::conditional<is_const, const value_type *,
						  value_type *>::type;

	node_iterator();

	node_iterator(leaf_node_ptr node_ptr, size_type p);

	node_iterator(const node_iterator &other);

	template <typename T = void,
		  typename = typename std::enable_if<is_const, T>::type>
	node_iterator(const node_iterator<leaf_type, false> &other);

	node_iterator &operator=(const node_iterator &other);

	node_iterator &operator++();
	node_iterator operator++(int);
	node_iterator &operator--();
	node_iterator operator--(int);

	node_iterator operator+(size_type off) const;
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
	size_type position;
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

	leaf_node_t();
	~leaf_node_t();

	void move(pool_base &pop, persistent_ptr<leaf_node_t> other, const key_compare &);
	template <typename K, typename M>
	iterator insert(iterator idxs_pos, K &&key, M &&obj);

	template <typename K>
	iterator find(const K &key, const key_compare &);
	template <typename K>
	const_iterator find(const K &key, const key_compare &) const;
	template <typename K>
	iterator lower_bound(const K &key, const key_compare &comp);

	template <typename K>
	size_type erase(pool_base &pop, const K &key, const key_compare &);

	iterator begin();
	const_iterator begin() const;
	const_iterator cbegin() const;
	iterator end();
	const_iterator end() const;
	const_iterator cend() const;

	size_type size() const;
	bool full() const;
	const_reference front() const;
	const_reference back() const;
	reference operator[](size_type pos);
	const_reference operator[](size_type pos) const;

	const persistent_ptr<leaf_node_t> &get_next() const;
	void set_next(const persistent_ptr<leaf_node_t> &n);
	const persistent_ptr<leaf_node_t> &get_prev() const;
	void set_prev(const persistent_ptr<leaf_node_t> &p);

private:
	/* uninitialized static array of value_type is used to avoid entries
	 * default initialization and to avoid additional allocations */
	union {
		value_type entries[capacity];
	};
	/* array of indexes to support ordering */
	pmem::obj::array<difference_type, capacity> idxs;
	pmem::obj::p<size_type> _size;
	/* persistent pointers to the neighboring leafs */
	pmem::obj::persistent_ptr<leaf_node_t> prev;
	pmem::obj::persistent_ptr<leaf_node_t> next;

	/* private helper methods */
	template <typename... Args>
	pointer emplace(difference_type pos, Args &&... args);
	size_type insert_idx(const_iterator pos);
	void remove_idx(size_type idx);
	void internal_erase(pool_base &pop, iterator it);
	bool is_sorted(const key_compare &);
	void add_to_tx(size_type begin, size_type end);
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

	inner_node_t(size_type level);
	inner_node_t(size_type level, const_reference key, const node_pptr &first_child,
		     const node_pptr &second_child);
	~inner_node_t();

	iterator move(pool_base &pop, inner_node_t &other, key_pptr &partition_key);
	template <typename K>
	void replace(iterator it, const K &key);
	void delete_with_child(iterator it, bool left);
	void inherit_child(iterator it, node_pptr &child, bool left);
	void update_splitted_child(pool_base &pop, const_reference key,
				   node_pptr &left_child, node_pptr &right_child,
				   const key_compare &);

	template <typename K>
	std::tuple<node_t *, node_t *, node_t *, iterator>
	get_child_and_siblings(const K &key, const key_compare &);
	template <typename K>
	const node_pptr &get_child(const K &key, const key_compare &) const;
	const node_pptr &get_child(const_reference key, const key_compare &) const;
	const node_pptr &get_left_child(const_iterator it) const;
	const node_pptr &get_right_child(const_iterator it) const;

	bool full() const;

	iterator begin();
	const_iterator begin() const;
	const_iterator cbegin() const;
	iterator end();
	const_iterator end() const;
	const_iterator cend() const;

	size_type size() const;
	const_reference back() const;
	reference operator[](size_type pos);
	const_reference operator[](size_type pos) const;

private:
	key_pptr entries[capacity];
	node_pptr children[capacity + 1];
	pmem::obj::p<size_type> _size = 0;

	pool_base get_pool() const noexcept;
	bool is_sorted(const key_compare &);
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
	bool operator==(const b_tree_iterator &other) const;
	bool operator!=(const b_tree_iterator &other) const;
	reference operator*() const;
	pointer operator->() const;

private:
	leaf_node_ptr current_node;
	leaf_iterator leaf_it;
}; /* class b_tree_iterator */

template <typename Key, typename T, typename Compare, std::size_t degree>
class b_tree_base {
private:
	const static std::size_t node_capacity = degree - 1;

	using self_type = b_tree_base<Key, T, Compare, degree>;
	using leaf_type = leaf_node_t<Key, T, Compare, node_capacity>;
	using inner_type = inner_node_t<Key, Compare, node_capacity>;
	using key_pptr = persistent_ptr<Key>;
	using node_pptr = persistent_ptr<node_t>;
	using leaf_pptr = persistent_ptr<leaf_type>;
	using inner_pptr = persistent_ptr<inner_type>;
	using path_type = std::vector<inner_pptr>;

	using inner_pair = std::pair<inner_pptr, typename inner_type::iterator>;

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

	template <typename K, typename M>
	std::pair<iterator, bool> try_emplace(K &&key, M &&obj);

	template <typename K>
	iterator find(const K &key);
	template <typename K>
	const_iterator find(const K &key) const;
	template <typename K>
	iterator lower_bound(const K &key);
	template <typename K>
	const_iterator lower_bound(const K &key) const;
	template <typename K>
	iterator upper_bound(const K &key);
	template <typename K>
	const_iterator upper_bound(const K &key) const;

	template <typename K>
	size_type erase(const K &key);

	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator cbegin() const;
	const_iterator cend() const;
	reverse_iterator rbegin();
	reverse_iterator rend();

	size_type size() const noexcept;

	reference operator[](size_type pos);
	const_reference operator[](size_type pos) const;

	key_compare &key_comp();
	const key_compare &key_comp() const;

private:
	node_pptr root;
	node_pptr split_node;
	node_pptr left_child;
	node_pptr right_child;
	key_compare compare;
	pmem::obj::p<size_type> _size;

	const key_type &get_last_key(const node_pptr &node);
	leaf_type *leftmost_leaf() const;
	leaf_type *rightmost_leaf() const;

	void create_new_root(const key_type &, node_pptr &, node_pptr &);
	typename inner_type::const_iterator split_half(pool_base &pop, inner_pptr &node,
						       inner_pptr &other,
						       key_pptr &partition_key);
	void split_inner_node(pool_base &pop, inner_pptr &src_node);
	void split_inner_node(pool_base &pop, inner_pptr &src_node,
			      inner_type *parent_node);
	template <typename K, typename M>
	std::pair<iterator, bool> split_leaf_node(pool_base &pop, leaf_pptr &split_leaf,
						  K &&key, M &&obj);
	template <typename K, typename M>
	std::pair<iterator, bool> split_leaf_node(pool_base &pop, inner_type *parent_node,
						  leaf_pptr &split_leaf, K &&key,
						  M &&obj);

	leaf_type *find_leaf_node(const key_type &key) const;
	template <typename K>
	leaf_type *find_leaf_node(const K &key) const;
	template <typename K>
	leaf_pptr find_leaf_to_insert(const K &key, path_type &path) const;
	typename path_type::const_iterator find_full_node(const path_type &path);
	template <typename K, typename M>
	std::pair<iterator, bool> internal_insert(leaf_pptr leaf, K &&key, M &&obj);
	template <typename K>
	leaf_pptr get_path_ext(const K &key, std::vector<inner_pair> &path,
			       std::vector<std::pair<node_pptr, node_pptr>> &neighbors,
			       inner_pair &inner_ptr);
	const_reference get_suitable_entry(inner_pair &node);
	void delete_leaf_ext(leaf_pptr &leaf, inner_pair &parent, bool has_left_sibling);
	void delete_inner_ext(inner_pptr &node, inner_pair &parent,
			      std::pair<node_pptr, node_pptr> &neighbors,
			      bool has_left_sibling);

	static inner_pptr &cast_inner(node_pptr &node);
	static inner_type *cast_inner(node_t *node);
	static leaf_pptr &cast_leaf(node_pptr &node);
	static leaf_type *cast_leaf(node_t *node);
	static node_pptr &cast_node(leaf_pptr &node);
	static node_pptr &cast_node(inner_pptr &node);

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
node_iterator<LeafType, is_const>::node_iterator(leaf_node_ptr node_ptr, size_type p)
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
node_iterator<LeafType, is_const> &
node_iterator<LeafType, is_const>::operator=(const node_iterator &other)
{
	if (this == &other)
		return *this;

	node = other.node;
	position = other.position;
	return *this;
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
node_iterator<LeafType, is_const>::operator+(size_type off) const
{
	return node_iterator(node, position + off);
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const>
node_iterator<LeafType, is_const>::operator+=(difference_type off)
{
	position += static_cast<size_type>(off);
	return *this;
}

template <typename LeafType, bool is_const>
node_iterator<LeafType, is_const>
node_iterator<LeafType, is_const>::operator-(difference_type off) const
{
	assert(node != nullptr);
	assert(position >= static_cast<size_type>(off));
	return node_iterator(node, position - static_cast<size_type>(off));
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
	return node->operator[](position);
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
leaf_node_t<Key, T, Compare, capacity>::leaf_node_t() : node_t()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	std::iota(idxs.begin(), idxs.end(), 0);
	_size = 0;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
leaf_node_t<Key, T, Compare, capacity>::~leaf_node_t()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	try {
		for (value_type &e : *this) {
			e.first.~key_type();
			e.second.~mapped_type();
		}
	} catch (transaction_error &e) {
		std::terminate();
	}
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
void leaf_node_t<Key, T, Compare, capacity>::move(pool_base &pop,
						  persistent_ptr<leaf_node_t> other,
						  const key_compare &comp)
{
	assert(other->full());
	assert(this->size() == 0);
	size_type middle_idx = other->size() / 2;
	auto middle = std::make_move_iterator(other->begin() + middle_idx);
	auto last = std::make_move_iterator(other->end());
	auto temp = middle;
	/* move second half from 'other' to 'this' */
	pmem::obj::transaction::run(pop, [&] {
		/* add range to tx before moving to avoid sequential snapshotting */
		other->add_to_tx(middle_idx, other->size());
		difference_type count = 0;
		while (temp != last) {
			emplace(count++, *temp++);
		}
		_size = static_cast<size_type>(count);
		other->_size -= static_cast<size_type>(count);
	});
	assert(std::distance(begin(), end()) > 0);
	assert(is_sorted(comp));
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::lower_bound(const K &key, const key_compare &comp)
{
	return std::lower_bound(
		begin(), end(), key,
		[&comp](const_reference e, const K &key) { return comp(e.first, key); });
}

/**
 * Inserts element into the leaf in a sorted way specified by idxs_pos.
 *
 * @pre key must not already exist in the leaf.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K, typename M>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::insert(iterator idxs_pos, K &&key, M &&obj)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(!full());
	difference_type insert_pos = idxs[size()];
	assert(std::none_of(
		idxs.cdata(), idxs.cdata() + size(),
		[&insert_pos](difference_type idx) { return insert_pos == idx; }));
	// insert an entry to the end
	emplace(insert_pos, std::forward<K>(key), std::forward<M>(obj));
	// update idxs & return iterator
	return iterator(this, insert_idx(idxs_pos));
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::iterator
leaf_node_t<Key, T, Compare, capacity>::find(const K &key, const key_compare &comp)
{
	iterator it = lower_bound(key, comp);
	if (it != end() && (!comp(it->first, key) && !comp(key, it->first))) {
		return it;
	} else {
		return end();
	}
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::const_iterator
leaf_node_t<Key, T, Compare, capacity>::find(const K &key, const key_compare &comp) const
{
	const_iterator it = lower_bound(key, comp);
	if (it != cend() && (!comp(it->first, key) && !comp(key, it->first))) {
		return it;
	} else {
		return cend();
	}
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename K>
typename leaf_node_t<Key, T, Compare, capacity>::size_type
leaf_node_t<Key, T, Compare, capacity>::erase(pool_base &pop, const K &key,
					      const key_compare &comp)
{
	iterator it = find(key, comp);
	if (it == end())
		return size_type(0);
	internal_erase(pop, it);
	assert(is_sorted(comp));
	return size_type(1);
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
 * Return const_iterator to the beginning.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_iterator
leaf_node_t<Key, T, Compare, capacity>::cbegin() const
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
	return iterator(this, size());
}

/**
 * Return const_iterator to the end.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_iterator
leaf_node_t<Key, T, Compare, capacity>::end() const
{
	return const_iterator(this, size());
}

/**
 * Return const_iterator to the end.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_iterator
leaf_node_t<Key, T, Compare, capacity>::cend() const
{
	return const_iterator(this, size());
}

/**
 * Return the size of the array of entries (key/value).
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::size_type
leaf_node_t<Key, T, Compare, capacity>::size() const
{
	return _size;
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
	return entries[idxs[0]];
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_reference
leaf_node_t<Key, T, Compare, capacity>::back() const
{
	return entries[idxs[size() - 1]];
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::reference
	leaf_node_t<Key, T, Compare, capacity>::operator[](size_type pos)
{
	assert(pos <= size());
	return entries[idxs[pos]];
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::const_reference
	leaf_node_t<Key, T, Compare, capacity>::operator[](size_type pos) const
{
	assert(pos <= size());
	return entries[idxs[pos]];
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

/**
 * Constructs value_type in position 'pos' of entries with arguments 'args'.
 *
 * @pre must be called in a transaction scope.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
template <typename... Args>
typename leaf_node_t<Key, T, Compare, capacity>::pointer
leaf_node_t<Key, T, Compare, capacity>::emplace(difference_type pos, Args &&... args)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	/* to avoid snapshotting of an uninitialized memory */
	pmemobj_tx_xadd_range_direct(entries + pos, sizeof(value_type),
				     POBJ_XADD_NO_SNAPSHOT);
	return new (entries + pos) value_type(std::forward<Args>(args)...);
}

/**
 * Replaces index of newly allocated on position idxs[size()] element in sorted order.
 *
 * @param pos - position in sorted idxs array where entry must reside.
 *
 * @pre must be used right after addition of a new entry
 * @pre new entry must be added into idxs[size()] position
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
typename leaf_node_t<Key, T, Compare, capacity>::size_type
leaf_node_t<Key, T, Compare, capacity>::insert_idx(const_iterator pos)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	difference_type new_entry_idx = idxs[size()];

	auto idx_pos = static_cast<size_type>(std::distance(cbegin(), pos));
	auto slice = idxs.range(idx_pos, size() - idx_pos);
	auto to_insert = std::copy_backward(slice.begin(), slice.end(), slice.end() + 1);
	*(--to_insert) = new_entry_idx;
	++_size;

	size_type result = static_cast<size_type>(to_insert - idxs.cbegin());
	assert(result >= 0);
	return result;
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::remove_idx(size_type idx)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size() > 0);

	difference_type to_replace = idxs[idx];
	auto slice = idxs.range(idx, size() - idx);
	auto replace_pos = std::copy(slice.begin() + 1, slice.end(), slice.begin());
	*replace_pos = to_replace;
	--_size;
}

/**
 * Remove element pointed by iterator.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::internal_erase(pool_base &pop, iterator it)
{
	size_type idx = static_cast<size_type>(std::distance(begin(), it));
	pmem::obj::transaction::run(pop, [&] {
		/* destruct key-value pair */
		(*it).first.~key_type();
		(*it).second.~mapped_type();
		/* update idxs */
		remove_idx(idx);
	});
}

template <typename Key, typename T, typename Compare, uint64_t capacity>
bool leaf_node_t<Key, T, Compare, capacity>::is_sorted(const key_compare &comp)
{
	return std::is_sorted(begin(), end(),
			      [&comp](const_reference a, const_reference b) {
				      return comp(a.first, b.first);
			      });
}

/**
 * Adds the range [begin, end) of underlying array to transaction.
 */
template <typename Key, typename T, typename Compare, uint64_t capacity>
void leaf_node_t<Key, T, Compare, capacity>::add_to_tx(size_type begin, size_type end)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	pmemobj_tx_xadd_range_direct(entries + begin, sizeof(value_type) * (end - begin),
				     POBJ_XADD_ASSUME_INITIALIZED);
}

// -------------------------------------------------------------------------------------
// ------------------------------------- inner_node_t ----------------------------------
// -------------------------------------------------------------------------------------

template <typename Key, typename Compare, uint64_t capacity>
inner_node_t<Key, Compare, capacity>::inner_node_t(size_type level)
    : node_t(level), _size(0)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
}

template <typename Key, typename Compare, uint64_t capacity>
inner_node_t<Key, Compare, capacity>::inner_node_t(size_type level, const_reference key,
						   const node_pptr &first_child,
						   const node_pptr &second_child)
    : node_t(level)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	entries[0] = pmem::obj::persistent_ptr<key_type>(&key);
	children[0] = first_child;
	children[1] = second_child;
	_size = 1;
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
	return begin();
}

/**
 * Changes entry specified by the iterator with key pointer
 */
template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
void inner_node_t<Key, Compare, capacity>::replace(iterator it, const K &key)
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
								 node_pptr &right_child,
								 const key_compare &comp)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(!full());
	const_iterator insert_it = std::lower_bound(
		cbegin(), cend(), key, [&comp](const_reference lhs, const_reference rhs) {
			return comp(lhs, rhs);
		});
	difference_type insert_idx = std::distance(cbegin(), insert_it);
	/* update entries inserting new key */
	key_pptr *to_insert = std::copy_backward(entries + insert_idx, entries + size(),
						 entries + size() + 1);
	assert(insert_idx < std::distance(entries, to_insert));
	*(--to_insert) = pmem::obj::persistent_ptr<key_type>(&key);
	++_size;
	/* update children inserting new descendants */
	node_pptr *to_insert_child = std::copy_backward(
		children + insert_idx + 1, children + size(), children + size() + 1);
	*(--to_insert_child) = right_child;
	*(--to_insert_child) = left_child;

	assert(is_sorted(comp));
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
	std::move(entries + pos + 1, entries + size(), entries + pos);
	if (left) {
		std::move(children + pos + 1, children + size() + 1, children + pos);
	} else {
		std::move(children + pos + 2, children + size() + 1, children + pos + 1);
	}
	--_size;
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
inner_node_t<Key, Compare, capacity>::get_child(const K &key,
						const key_compare &comp) const
{
	const_iterator it = std::upper_bound(
		cbegin(), cend(), key,
		[&comp](const K &lhs, const_reference rhs) { return comp(lhs, rhs); });
	return get_left_child(it);
}

template <typename Key, typename Compare, uint64_t capacity>
template <typename K>
std::tuple<node_t *, node_t *, node_t *,
	   typename inner_node_t<Key, Compare, capacity>::iterator>
inner_node_t<Key, Compare, capacity>::get_child_and_siblings(const K &key,
							     const key_compare &comp)
{
	assert(size() > 0);
	iterator it = std::upper_bound(
		begin(), end(), key,
		[&comp](const K &lhs, const_reference rhs) { return comp(lhs, rhs); });
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
inner_node_t<Key, Compare, capacity>::get_child(const_reference key,
						const key_compare &comp) const
{
	const_iterator it = std::upper_bound(
		cbegin(), cend(), key, [&comp](const_reference lhs, const_reference rhs) {
			return comp(lhs, rhs);
		});
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

/**
 * Return begin const_iterator on an array of keys.
 */
template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_iterator
inner_node_t<Key, Compare, capacity>::begin() const
{
	return const_iterator(this, 0);
}

/**
 * Return begin const_iterator on an array of keys.
 */
template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_iterator
inner_node_t<Key, Compare, capacity>::cbegin() const
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

/**
 * Return end const_iterator on an array of keys.
 */
template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_iterator
inner_node_t<Key, Compare, capacity>::end() const
{
	return begin() + this->size();
}

/**
 * Return end const_iterator on an array of keys.
 */
template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_iterator
inner_node_t<Key, Compare, capacity>::cend() const
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
	inner_node_t<Key, Compare, capacity>::operator[](size_type pos)
{
	assert(pos <= size());
	return *entries[pos];
}

template <typename Key, typename Compare, uint64_t capacity>
typename inner_node_t<Key, Compare, capacity>::const_reference
	inner_node_t<Key, Compare, capacity>::operator[](size_type pos) const
{
	assert(pos <= size());
	return *entries[pos];
}

template <typename Key, typename Compare, uint64_t capacity>
bool inner_node_t<Key, Compare, capacity>::is_sorted(const key_compare &comp)
{
	return std::is_sorted(begin(), end(),
			      [&comp](const key_type &lhs, const key_type &rhs) {
				      return comp(lhs, rhs);
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
bool b_tree_iterator<LeafType, is_const>::operator==(const b_tree_iterator &other) const
{
	return current_node == other.current_node && leaf_it == other.leaf_it;
}

template <typename LeafType, bool is_const>
bool b_tree_iterator<LeafType, is_const>::operator!=(const b_tree_iterator &other) const
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

template <typename Key, typename T, typename Compare, std::size_t degree>
b_tree_base<Key, T, Compare, degree>::b_tree_base()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	cast_leaf(root) = allocate_leaf();
	_size = 0;
}

template <typename Key, typename T, typename Compare, std::size_t degree>
b_tree_base<Key, T, Compare, degree>::~b_tree_base()
{
	try {
		deallocate(root);
	} catch (transaction_error &e) {
		std::terminate();
	}
}

template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K, typename M>
std::pair<typename b_tree_base<Key, T, Compare, degree>::iterator, bool>
b_tree_base<Key, T, Compare, degree>::try_emplace(K &&key, M &&obj)
{
	auto pop = get_pool_base();

	path_type path;
	leaf_pptr leaf = find_leaf_to_insert(std::forward<K>(key), path);

	// --------------- entry with the same key found ---------------
	typename leaf_type::iterator leaf_it = leaf->find(std::forward<K>(key), compare);
	if (leaf_it != leaf->end()) {
		return std::pair<iterator, bool>(iterator(leaf.get(), leaf_it), false);
	}

	// ------------------ leaf not full -> insert ------------------
	if (!leaf->full()) {
		return internal_insert(leaf, std::forward<K>(key), std::forward<M>(obj));
	}

	// -------------------- if root is leaf ------------------------
	if (path.empty()) {
		return split_leaf_node(pop, leaf, std::forward<K>(key),
				       std::forward<M>(obj));
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
			cast_inner(root)->get_child(std::forward<K>(key), compare).get());
	} else {
		parent_node = (*i).get();
	}
	++i;

	for (; i != path.end(); ++i) {
		split_inner_node(pop, *i, parent_node);
		parent_node = cast_inner(
			parent_node->get_child(std::forward<K>(key), compare).get());
	}

	return split_leaf_node(pop, parent_node, leaf, std::forward<K>(key),
			       std::forward<M>(obj));
}

template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::find(const K &key)
{
	leaf_type *leaf = find_leaf_node(key);
	typename leaf_type::iterator leaf_it = leaf->find(key, compare);
	if (leaf->end() == leaf_it)
		return end();

	return iterator(leaf, leaf_it);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::find(const K &key) const
{
	leaf_type *leaf = find_leaf_node(key);
	typename leaf_type::const_iterator leaf_it = leaf->find(key);
	if (leaf->cend() == leaf_it)
		return cend();

	return const_iterator(leaf, leaf_it);
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
template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::lower_bound(const K &key)
{
	leaf_type *leaf = find_leaf_node(key);
	typename leaf_type::iterator leaf_it = std::lower_bound(
		leaf->begin(), leaf->end(), key, [this](const_reference e, const K &key) {
			return compare(e.first, key);
		});
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
template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::lower_bound(const K &key) const
{
	leaf_type *leaf = find_leaf_node(key);
	typename leaf_type::const_iterator leaf_it =
		std::lower_bound(leaf->cbegin(), leaf->cend(), key,
				 [this](const_reference e, const K &key) {
					 return compare(e.first, key);
				 });
	if (leaf->cend() == leaf_it)
		return cend();

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
template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::upper_bound(const K &key)
{
	leaf_type *leaf = find_leaf_node(key);
	typename leaf_type::iterator leaf_it = std::upper_bound(
		leaf->begin(), leaf->end(), key, [this](const K &key, const_reference e) {
			return compare(key, e.first);
		});
	if (leaf->end() == leaf_it) {
		if (leaf->get_next())
			return iterator(leaf->get_next().get(),
					leaf->get_next()->begin());
		return end();
	}

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
template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::upper_bound(const K &key) const
{
	leaf_type *leaf = find_leaf_node(key);
	typename leaf_type::const_iterator leaf_it =
		std::upper_bound(leaf->cbegin(), leaf->cend(), key,
				 [this](const K &key, const_reference e) {
					 return compare(key, e.first);
				 });
	if (leaf->cend() == leaf_it) {
		if (leaf->get_next())
			return iterator(leaf->get_next().get(),
					leaf->get_next()->cbegin());
		return cend();
	}

	return const_iterator(leaf, leaf_it);
}

/**
 * Searches leaf with given key saving extended path including neighbors and inner node
 * with pointer to leaf entry (inner_ptr).
 */
template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::leaf_pptr
b_tree_base<Key, T, Compare, degree>::get_path_ext(
	const K &key, std::vector<inner_pair> &path,
	std::vector<std::pair<node_pptr, node_pptr>> &neighbors, inner_pair &inner_ptr)
{
	node_pptr temp = root;
	while (!temp->leaf()) {
		auto set = cast_inner(temp)->get_child_and_siblings(key, compare);
		path.push_back(std::make_pair(cast_inner(temp).get(), std::get<3>(set)));
		neighbors.push_back(std::make_pair(std::get<1>(set), std::get<2>(set)));
		if (!compare(*std::get<3>(set), key) &&
		    !compare(key, *std::get<3>(set))) {
			assert(inner_ptr.first == nullptr); // it should not duplicate
			inner_ptr = std::make_pair(cast_inner(temp), std::get<3>(set));
		}
		temp = std::get<0>(set);
	}
	return cast_leaf(temp);
}

/**
 * Searches leaf in the right subtree of inner node (node.first), first element of which
 * must reside at given iterator (node.second).
 */
template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_reference
b_tree_base<Key, T, Compare, degree>::get_suitable_entry(inner_pair &node)
{
	node_pptr temp = node.first->get_right_child(node.second);
	while (!temp->leaf())
		temp = cast_inner(temp)->get_left_child(cast_inner(temp)->begin());
	return cast_leaf(temp)->front();
}

/**
 * Deletes leaf from the tree leaving parent_node and neighbors in consistent state.
 */
template <typename Key, typename T, typename Compare, std::size_t degree>
void b_tree_base<Key, T, Compare, degree>::delete_leaf_ext(leaf_pptr &leaf,
							   inner_pair &parent,
							   bool no_left_sibling)
{
	/* if left sibling exists then leaf is right child */
	parent.first->delete_with_child(parent.second, no_left_sibling);
	/* correct leaf siblings pointers before deleting it */
	if (leaf->get_prev()) {
		leaf->get_prev()->set_next(leaf->get_next());
	}
	if (leaf->get_next()) {
		leaf->get_next()->set_prev(leaf->get_prev());
	}
	deallocate(leaf);
}

/**
 * Deletes inner node from the tree leaving parent_node and neighbors in consistent state.
 * Also inherites remaining child.
 */
template <typename Key, typename T, typename Compare, std::size_t degree>
void b_tree_base<Key, T, Compare, degree>::delete_inner_ext(
	inner_pptr &node, inner_pair &parent, std::pair<node_pptr, node_pptr> &neighbors,
	bool has_left_sibling)
{
	if (neighbors.first) {
		parent.first->inherit_child(parent.second, neighbors.first,
					    !has_left_sibling);
	} else if (neighbors.second) {
		parent.first->inherit_child(parent.second, neighbors.second,
					    !has_left_sibling);
	}
	deallocate(node);
}

/**
 * Erases entry specified by key from the tree.
 */
template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::size_type
b_tree_base<Key, T, Compare, degree>::erase(const K &key)
{
	using const_key = const key_type &;
	/* search leaf saving path, neighbors */
	std::vector<inner_pair> path;				// [root, leaf)
	std::vector<std::pair<node_pptr, node_pptr>> neighbors; // (root, leaf]
	inner_pair to_replace; // inner node with key reference
	leaf_pptr leaf = get_path_ext(key, path, neighbors, to_replace);

	auto pop = get_pool_base();
	size_type result(1);
	pmem::obj::transaction::run(pop, [&] {
		/* remove entry */
		size_type deleted = leaf->erase(pop, key, compare);
		if (!deleted) {
			result = size_type(0);
			return;
		}
		/* still left elements in leaf -> replace pointer in inner node */
		if (leaf->size() > 0) {
			if (to_replace.first != nullptr) {
				const_key new_key = get_suitable_entry(to_replace).first;
				to_replace.first->replace(to_replace.second, new_key);
			}
			--_size;
			return;
		}
		/* leaf is empty and it is root */
		if (path.empty()) {
			--_size;
			return;
		}
		/* handle leaf node */
		auto nbors = neighbors.back();
		auto parent = path.back();
		/* if left sibling exists then leaf is right child */
		delete_leaf_ext(leaf, parent, nbors.first == nullptr);
		/* handle inner nodes */
		auto node = path.back().first;
		if (path.size() > 1 && node->size() == 0) {
			path.pop_back();
			neighbors.pop_back();

			delete_inner_ext(node, path.back(), nbors,
					 neighbors.back().first != nullptr);
		}
		/* replace pointer in inner node */
		if (to_replace.first) {
			const_key new_key = get_suitable_entry(to_replace).first;
			to_replace.first->replace(to_replace.second, new_key);
		}
		/* one of the main subtrees deleted, other will become root */
		if (path.back().first->size() == 0) {
			if (nbors.first) {
				cast_inner(root) = nbors.first;
			} else if (nbors.second) {
				cast_inner(root) = nbors.second;
			}
		}
		--_size;
	});
	/* all done, return */
	return result;
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::begin()
{
	return iterator(leftmost_leaf());
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::iterator
b_tree_base<Key, T, Compare, degree>::end()
{
	leaf_type *leaf = rightmost_leaf();
	return iterator(leaf, leaf->end());
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::begin() const
{
	return const_iterator(leftmost_leaf());
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::end() const
{
	const leaf_type *leaf = rightmost_leaf();
	return const_iterator(leaf, leaf->end());
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::cbegin() const
{
	return begin();
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_iterator
b_tree_base<Key, T, Compare, degree>::cend() const
{
	return end();
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::reverse_iterator
b_tree_base<Key, T, Compare, degree>::rbegin()
{
	return reverse_iterator(end());
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::reverse_iterator
b_tree_base<Key, T, Compare, degree>::rend()
{
	return reverse_iterator(begin());
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::size_type
b_tree_base<Key, T, Compare, degree>::size() const noexcept
{
	return _size;
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::reference
	b_tree_base<Key, T, Compare, degree>::operator[](size_type pos)
{
	leaf_type *temp = leftmost_leaf();
	while (temp != nullptr && pos >= temp->size()) {
		pos -= temp->size();
		temp = temp->get_next().get();
	}
	return temp->operator[](pos);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::const_reference
	b_tree_base<Key, T, Compare, degree>::operator[](size_type pos) const
{
	leaf_type *temp = leftmost_leaf();
	while (temp != nullptr && pos >= temp->size()) {
		pos -= temp->size();
		temp = temp->get_next().get();
	}
	return temp->operator[](pos);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
const typename b_tree_base<Key, T, Compare, degree>::key_type &
b_tree_base<Key, T, Compare, degree>::get_last_key(const node_pptr &node)
{
	if (node->leaf()) {
		return cast_leaf(node.get())->back().first;
	} else {
		return cast_inner(node.get())->back();
	}
}

template <typename Key, typename T, typename Compare, std::size_t degree>
void b_tree_base<Key, T, Compare, degree>::create_new_root(const key_type &key,
							   node_pptr &l_child,
							   node_pptr &r_child)
{
	assert(l_child != nullptr);
	assert(r_child != nullptr);
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	cast_inner(root) = allocate_inner(root->level() + 1, key, l_child, r_child);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
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
template <typename Key, typename T, typename Compare, std::size_t degree>
void b_tree_base<Key, T, Compare, degree>::split_inner_node(pool_base &pop,
							    inner_pptr &src_node)
{
	assert(root == src_node);
	pmem::obj::transaction::run(pop, [&] {
		node_pptr other(nullptr);
		key_pptr partition_key(nullptr);
		split_half(pop, src_node, cast_inner(other), partition_key);
		assert(partition_key != nullptr);
		create_new_root(*partition_key, cast_node(src_node), other);
	});
}

/* when root is not the only inner node (2 or more inner node layers) */
template <typename Key, typename T, typename Compare, std::size_t degree>
void b_tree_base<Key, T, Compare, degree>::split_inner_node(pool_base &pop,
							    inner_pptr &src_node,
							    inner_type *parent_node)
{
	pmem::obj::transaction::run(pop, [&] {
		node_pptr other(nullptr);
		key_pptr partition_key(nullptr);
		split_half(pop, src_node, cast_inner(other), partition_key);
		assert(partition_key != nullptr);
		parent_node->update_splitted_child(pop, *partition_key,
						   cast_node(src_node), other, compare);
	});
}

/* split leaf in case when root is leaf */
template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K, typename M>
std::pair<typename b_tree_base<Key, T, Compare, degree>::iterator, bool>
b_tree_base<Key, T, Compare, degree>::split_leaf_node(pool_base &pop,
						      leaf_pptr &split_leaf, K &&key,
						      M &&obj)
{
	assert(split_leaf->full());

	leaf_pptr node;
	std::pair<iterator, bool> result(nullptr, false);
	auto middle = split_leaf->begin() + split_leaf->size() / 2;
	bool less = compare(std::forward<K>(key), middle->first);
	// move second half into node and insert new element where needed
	pmem::obj::transaction::run(pop, [&] {
		node = allocate_leaf();
		node->move(pop, split_leaf, compare);
		/* insert entry(key, obj) into needed half */
		if (less) {
			result = internal_insert(split_leaf, std::forward<K>(key),
						 std::forward<M>(obj));
		} else {
			result = internal_insert(node, std::forward<K>(key),
						 std::forward<M>(obj));
		}
		create_new_root(node->front().first, cast_node(split_leaf),
				cast_node(node));
		// re-set node's pointers
		node->set_next(split_leaf->get_next());
		node->set_prev(split_leaf);
		if (split_leaf->get_next()) {
			split_leaf->get_next()->set_prev(node);
		}
		split_leaf->set_next(node);
	});

	assert(!compare(result.first->first, key) && !compare(key, result.first->first));
	return result;
}

/* split leaf in case when root is not leaf */
template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K, typename M>
std::pair<typename b_tree_base<Key, T, Compare, degree>::iterator, bool>
b_tree_base<Key, T, Compare, degree>::split_leaf_node(pool_base &pop,
						      inner_type *parent_node,
						      leaf_pptr &split_leaf, K &&key,
						      M &&obj)
{
	assert(split_leaf->full());

	leaf_pptr node;
	std::pair<iterator, bool> result(nullptr, false);
	auto middle = split_leaf->begin() + split_leaf->size() / 2;
	bool less = compare(std::forward<K>(key), middle->first);
	// move second half into node and insert new element where needed
	pmem::obj::transaction::run(pop, [&] {
		node = allocate_leaf();
		node->move(pop, split_leaf, compare);
		/* insert entry(key, obj) into needed half */
		if (less) {
			result = internal_insert(split_leaf, std::forward<K>(key),
						 std::forward<M>(obj));
		} else {
			result = internal_insert(node, std::forward<K>(key),
						 std::forward<M>(obj));
		}
		// take care of parent node
		parent_node->update_splitted_child(pop, node->front().first,
						   cast_node(split_leaf), cast_node(node),
						   compare);
		// re-set node's pointers
		node->set_next(split_leaf->get_next());
		node->set_prev(split_leaf);
		if (split_leaf->get_next()) {
			split_leaf->get_next()->set_prev(node);
		}
		split_leaf->set_next(node);
	});

	assert(!compare(result.first->first, key) && !compare(key, result.first->first));
	return result;
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::find_leaf_node(const key_type &key) const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		node = cast_inner(node)->get_child(key);
	}
	return cast_leaf(node).get();
}

template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::find_leaf_node(const K &key) const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		node = cast_inner(node)->get_child(key, compare);
	}
	return cast_leaf(node).get();
}

template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K>
typename b_tree_base<Key, T, Compare, degree>::leaf_pptr
b_tree_base<Key, T, Compare, degree>::find_leaf_to_insert(const K &key,
							  path_type &path) const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		path.push_back(cast_inner(node));
		node = cast_inner(node)->get_child(key, compare);
	}
	return cast_leaf(node);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
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

template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename K, typename M>
std::pair<typename b_tree_base<Key, T, Compare, degree>::iterator, bool>
b_tree_base<Key, T, Compare, degree>::internal_insert(leaf_pptr leaf, K &&key, M &&obj)
{
	auto idxs_pos = leaf->lower_bound(std::forward<K>(key), compare);
	if (idxs_pos != leaf->end() && !compare(idxs_pos->first, std::forward<K>(key)) &&
	    !compare(std::forward<K>(key), idxs_pos->first)) {
		return std::pair<iterator, bool>(iterator(leaf.get(), idxs_pos), false);
	}
	auto pop = get_pool_base();
	typename leaf_type::iterator res;
	pmem::obj::transaction::run(pop, [&] {
		res = leaf->insert(idxs_pos, std::forward<K>(key), std::forward<M>(obj));
		++_size;
	});
	return std::pair<iterator, bool>(iterator(leaf.get(), res), true);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::key_compare &
b_tree_base<Key, T, Compare, degree>::key_comp()
{
	return compare;
}

template <typename Key, typename T, typename Compare, std::size_t degree>
const typename b_tree_base<Key, T, Compare, degree>::key_compare &
b_tree_base<Key, T, Compare, degree>::key_comp() const
{
	return compare;
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::leftmost_leaf() const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		inner_type *inner_node = cast_inner(node).get();
		node = inner_node->get_left_child(inner_node->begin());
	}
	return cast_leaf(node).get();
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::rightmost_leaf() const
{
	assert(root != nullptr);
	node_pptr node = root;
	while (!node->leaf()) {
		inner_type *inner_node = cast_inner(node).get();
		node = inner_node->get_left_child(inner_node->end());
	}
	return cast_leaf(node).get();
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::inner_pptr &
b_tree_base<Key, T, Compare, degree>::cast_inner(node_pptr &node)
{
	return reinterpret_cast<inner_pptr &>(node);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::inner_type *
b_tree_base<Key, T, Compare, degree>::cast_inner(node_t *node)
{
	return static_cast<inner_type *>(node);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_pptr &
b_tree_base<Key, T, Compare, degree>::cast_leaf(node_pptr &node)
{
	return reinterpret_cast<leaf_pptr &>(node);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::leaf_type *
b_tree_base<Key, T, Compare, degree>::cast_leaf(node_t *node)
{
	return static_cast<leaf_type *>(node);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::node_pptr &
b_tree_base<Key, T, Compare, degree>::cast_node(leaf_pptr &node)
{
	return reinterpret_cast<node_pptr &>(node);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
typename b_tree_base<Key, T, Compare, degree>::node_pptr &
b_tree_base<Key, T, Compare, degree>::cast_node(inner_pptr &node)
{
	return reinterpret_cast<node_pptr &>(node);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename... Args>
inline typename b_tree_base<Key, T, Compare, degree>::inner_pptr
b_tree_base<Key, T, Compare, degree>::allocate_inner(Args &&... args)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	return make_persistent<inner_type>(std::forward<Args>(args)...);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
template <typename... Args>
inline typename b_tree_base<Key, T, Compare, degree>::leaf_pptr
b_tree_base<Key, T, Compare, degree>::allocate_leaf(Args &&... args)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	return make_persistent<leaf_type>(std::forward<Args>(args)...);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
inline void b_tree_base<Key, T, Compare, degree>::deallocate(node_pptr &node)
{
	assert(node != nullptr);
	if (node->leaf()) {
		deallocate(cast_leaf(node));
	} else {
		deallocate(cast_inner(node));
	}
}

template <typename Key, typename T, typename Compare, std::size_t degree>
inline void b_tree_base<Key, T, Compare, degree>::deallocate(leaf_pptr &node)
{
	assert(node != nullptr);
	pool_base pop = get_pool_base();
	pmem::obj::transaction::run(pop, [&] {
		delete_persistent<leaf_type>(node);
		node = nullptr;
	});
}

template <typename Key, typename T, typename Compare, std::size_t degree>
inline void b_tree_base<Key, T, Compare, degree>::deallocate(inner_pptr &node)
{
	assert(node != nullptr);
	pool_base pop = get_pool_base();
	pmem::obj::transaction::run(pop, [&] {
		delete_persistent<inner_type>(node);
		node = nullptr;
	});
}

template <typename Key, typename T, typename Compare, std::size_t degree>
PMEMobjpool *b_tree_base<Key, T, Compare, degree>::get_objpool()
{
	PMEMoid oid = pmemobj_oid(this);
	return pmemobj_pool_by_oid(oid);
}

template <typename Key, typename T, typename Compare, std::size_t degree>
pool_base b_tree_base<Key, T, Compare, degree>::get_pool_base()
{
	return pool_base(get_objpool());
}

} /* namespace internal */

template <typename Key, typename Value, typename Compare = std::less<Key>,
	  std::size_t degree = 64>
class b_tree : public internal::b_tree_base<Key, Value, Compare, degree> {
private:
	using base_type = internal::b_tree_base<Key, Value, Compare, degree>;

public:
	using base_type::begin;
	using base_type::end;
	using base_type::erase;
	using base_type::find;
	using base_type::try_emplace;

	/* type definitions */
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
	}

	b_tree(const b_tree &) = delete;
	b_tree &operator=(const b_tree &) = delete;
};

} // namespace kv
} // namespace persistent
#endif // PERSISTENT_B_TREE
