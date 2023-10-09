/* A reducer tree is a tree that also computes reductions over ranges, using an
 * associative operator.  Internal nodes maintain the reduced value of the range
 * below.  Implemented as a treap.
 *
 * We don't try to be compatible with the std::map API: In particular, we don't
 * have iterators.  So `Insert` returns a `bool` indicating whether the value
 * was inserted.  `Find` returns an `optional<tuple>` of values.  Instead of
 * iterate with have `ForAll` which takes a functor and applies it to all the
 * elements of the tree.  Since it's not quite the same as `std::map` we also
 * use camel case for the method names.
 *
 * We rely on the spaceship operator <=> working for keys.
 */

#ifndef _REDUCER_TREE_H
#define _REDUCER_TREE_H

#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <random>

template <class K, class V, class Reducer>
class ReducerNode;

// A Reducer Tree is like an (ordered) map, where we also have a reduction value
// for subtrees.
template <class K, class V, class Reducer>
class ReducerTree {
 private:
  using Node = ReducerNode<K, V, Reducer>;
  using Ptr = std::unique_ptr<Node>;
 public:
  using key_type = K;
  using value_type = V;
  using reducer_type = Reducer;

  // Inserts `{key, value}` into the tree, if it's not there.  If it is there,
  // then nothing is changed.
  //
  // Return true if the insertion happened, false if it was already there.
  bool Insert(key_type key, value_type value) {
    if (Find(key)) {
      return false;
    }
    Ptr node = std::make_unique<Node>(_uniform_distribution(_engine),
                                      std::move(key),
                                      std::move(value));
    _root = Node::Insert(std::move(_root), std::move(node));
    return true;
  }

  // If `key` is in the tree, then return a reference to the key, the associated
  // value, and the reduced value at that node.  Else return `std::nullopt`.
  std::optional<std::tuple<const key_type&,
                           const value_type&,
                           const reducer_type&>> Find(const key_type& key) const {
    return Node::Find(_root, key);
  }

  // Returns the reduction of all the keys that are `<` key.
  std::optional<const reducer_type&> Prefix(const key_type& key) const {
  }

  bool Erase(key_type key) {
    bool erased;
    _root = Node::Erase(std::move(_root), key, erased);
    return erased;
  }
  std::ostream& Print(std::ostream& os) const {
    os << "(";
    if (_root) _root->Print(os, 0);
    os << ")";
    return os;
  }
  void Validate() const {
    if (_root) {
      _root->Validate(nullptr, nullptr);
    }
  }

  bool ForAll(std::function<bool(const K& key, const V& value, const Reducer& reduced)> fun) const {
    if (!_root) {
      return true;
    }
    return _root->ForAll(fun);
  }

 private:
  friend std::ostream& operator<<(std::ostream& os, const ReducerTree& tree) {
    return tree.Print(os);
  }

  std::unique_ptr<Node> _root;
  std::random_device _device;
  std::default_random_engine _engine{_device()};
  std::uniform_int_distribution<size_t> _uniform_distribution;
};

template <class K, class V, class Reducer>
class ReducerNode {
 public:
  using key_type = K;
  using value_type = V;
  using reducer_type = Reducer;

  using Ptr = std::unique_ptr<ReducerNode>;
  // After construction, the _reduced value is in an undefined state.
  ReducerNode(size_t priority, key_type key, value_type value)
      :_priority(priority)
      ,_key(std::move(key))
      ,_value(std::move(value)) {}

  // Inserts `node` into the subtree rooted at `root`, returning the new root of
  // the subtree.  `root` can be null.  `node` must be a node with null
  // children.  Requires: `node->key` is not in the subtree rooted at `root`.
  static Ptr Insert(Ptr root, Ptr node) {
    if (!root) {
      assert(!node->_left);
      assert(!node->_right);
      node->RecomputeReduced();
      return node;
    }
    if (node->_priority < root->_priority) {
      // root remains root.
      std::strong_ordering cmp = node->_key <=> root->_key;
      if (std::is_lt(cmp)) {
        root->SetLeftAndUpdateReduced(Insert(std::move(root->_right), std::move(node)));
        return root;
      }
      if (std::is_gt(cmp)) {
        root->SetRightAndUpdateReduced(Insert(std::move(root->_left), std::move(node)));
        return root;
      }
      assert(false);
    }
    // node becomes root. Split root according to node's key.
    auto [new_left, new_right] = Split(std::move(root), node->_key);
    node->SetBothAndUpdateReduced(std::move(new_left), std::move(new_right));
    return node;
  }

  static std::optional<std::tuple<const key_type&,
                                  const value_type&,
                                  const reducer_type&>> Find(
                                      const Ptr& root, const key_type& key) {
    if (!root) {
      return std::nullopt;
    }
    auto cmp = key <=> root->_key;
    if (std::is_lt(cmp)) {
      return Find(root->_left, key);
    }
    if (std::is_gt(cmp)) {
      return Find(root->_right, key);
    }
    // Return `root` if key equals.
    return std::tuple<const key_type&, const value_type&, const reducer_type>(
        root->_key, root->_value, root->_reduced);
  }

  static Ptr Erase(Ptr node, const K& key, bool& erased) {
    if (!node) {
      erased = false;
      return node;
    }
    auto cmp = key <=> node->_key;
    if (std::is_lt(cmp)) {
      node->SetLeftAndUpdateReduced(
          Erase(std::move(node->_left), key, erased));
      return node;
    }
    if (std::is_gt(cmp)) {
      node->SetRightAndUpdateReduced(
          Erase(std::move(node->_right), key, erased));
      return node;
    }
    erased = true;
    return Merge(std::move(node->_left), std::move(node->_right));
  }

  // Returns the tree containing all the nodes of `a` and `b`.
  //
  // Requires: All keys in `a` are < all keys in `b`.
  //
  // Implementation hint: Note that `a` and `b` are both heap ordered by
  // priority.  So one of `a` and `b` will be the root.
  static Ptr Merge(Ptr a, Ptr b) {
    if (!a) {
      return b;
    }
    if (!b) {
      return a;
    }
    if (a->_priority > b->_priority) {
      // `a` is the new root.
      a->_right = Merge(std::move(a->_right), std::move(b));
      a->RecomputeReduced();
      return a;
    } else {
      // `b` is the new root
      b->_left = Merge(std::move(a), std::move(b->_left));
      b->RecomputeReduced();
      return b;
    }
  }

  static std::tuple<Ptr, Ptr> Split(Ptr node, const key_type &key) {
    if (!node) {
      return {nullptr, nullptr};
    }
    auto cmp = key <=> node->_key;
    if (std::is_lt(cmp)) {
      auto [left, right] = Split(std::move(node->_left), key);
      node->_left = std::move(right);
      return {std::move(left), std::move(node)};
    }
    if (std::is_gt(cmp)) {
      auto [left, right] = Split(std::move(node->_right), key);
      node->_right = std::move(left);
      return {std::move(node), std::move(right)};
    }
    assert(false);
  }

  // Applies `fun` to every node in the tree, (quitting early if `fun` ever
  // returns `false`).  Returns `true` if `fun` returned `true` every time it's
  // called.
  bool ForAll(std::function<bool(const K& key,
                                 const V& value,
                                 const Reducer& reducer)> fun) {
    if (_left && !_left->ForAll(fun)) {
      return false;
    }
    if (!fun(_key, _value, _reduced)) {
      return false;
    }
    if (_right && !_right->ForAll(fun)) {
      return false;
    }
    return true;
  }

  void Print(std::ostream& os, size_t depth) const {
    os << "(" << _key << " " << _value << " " << _priority << " " << _reduced.value();
    if (!_left && !_right) {
      // Don't use a newline when neither child exists.
      os << " _ _)";
      return;
    }
    os << std::endl << std::string(depth, ' ') << " ";
    if (_left) {
      _left->Print(os, depth + 1);
      os << std::endl << std::string(depth, ' ') << " ";
    } else {
      // For null left ptr we don't use a whole line.
      os << "_ ";
      depth += 2; // make things line up in this case.
    }
    if (_right) {
      _right->Print(os, depth + 1);
    } else {
      os << "_";
    }
    os << ")";
  }

  void Validate(const K* lower_bound, const K* upper_bound) const {
    if (lower_bound) {
      assert(*lower_bound < _key);
    }
    if (upper_bound) {
      assert(_key < *upper_bound);
    }
    if (_left) {
      assert(_priority > _left->_priority);
      _left->Validate(lower_bound, &_key);
    }
    if (_right) {
      assert(_priority > _right->_priority);
      _right->Validate(&_key, upper_bound);
    }
    Reducer rhere = Reducer(_key, _value);
    if (_left) {
      rhere = _left->_reduced + rhere;
    }
    if (_right) {
      rhere = rhere + _right->_reduced;
    }
    assert(rhere == _reduced);
  }

 private:

  void SetLeftAndUpdateReduced(Ptr new_left) {
    _left = std::move(new_left);
    RecomputeReduced();
  }

  void SetRightAndUpdateReduced(Ptr new_right) {
    _right = std::move(new_right);
    RecomputeReduced();
  }

  void SetBothAndUpdateReduced(Ptr new_left, Ptr new_right) {
    _left = std::move(new_left);
    _right = std::move(new_right);
    RecomputeReduced();
  }

  void RecomputeReduced() {
    _reduced = Reducer(_key, _value);
    if (_left) {
      _reduced = _left->_reduced + std::move(_reduced);
    }
    if (_right) {
      _reduced = std::move(_reduced) + _right->_reduced;
    }
  }
  size_t _priority;
  K _key;
  [[no_unique_address]] V _value;
  [[no_unique_address]] Reducer _reduced;
  Ptr _left;
  Ptr _right;
};

#endif  // _REDUCER_TREE_H
