/* A reducer tree is a tree that also computes reductions over ranges, using an
 * associative operator.  Internal nodes maintain the reduced value of the range
 * below.  Implemented as a treap. */

#ifndef _REDUCER_TREE_H
#define _REDUCER_TREE_H

#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <random>

template <class K, class V, class Reducer>
class ReducerTree {
 private:
  class Node;
  using NodePtr = std::unique_ptr<Node>;
  class Node {
   public:
    Node(size_t priority, K key, V value)
        :_priority(priority)
        ,_key(std::move(key))
        ,_value(std::move(value))
        ,_reduced(key, value) {}
    size_t priority() const { return _priority; }
    const K& key() const { return _key; }
    const V& value() const { return _value; }
    const NodePtr& left() const { return _left; }
    const NodePtr& right() const { return _right; }
    const Reducer& reduced() const { return _reduced; }
    // root can be nullptr.  node must be a node with no children.
    static NodePtr Insert(NodePtr root, NodePtr node, bool& inserted) {
      if (!root) {
        inserted = true;
        root->_reduced = Reducer(node->key(), node->value());
        return node;
      }
      if (node->key() < root->key()) {
        NodePtr new_left = Insert(std::move(root->_left), std::move(node), inserted);
        return SetLeft(std::move(root), std::move(new_left));
      } else if (root->key() < node->key()) {
        NodePtr new_right = Insert(std::move(root->_right), std::move(node), inserted);
        return SetRight(std::move(root), std::move(new_right));
      } else {
        inserted = false;
        return root;
      }
    }

   private:
    static NodePtr SetLeft(NodePtr old_root, NodePtr new_left) {
      old_root->_left = std::move(new_left);
      if (!old_root->left() || old_root->priority() >=old_root->left()->priority()) {
        // If new left is null or the priorites are OK, just recompute the
        // reduction and return.
        old_root->RecomputeReduced();
        return old_root;
      } else {
        return RotateRight(std::move(old_root));
      }
    }
    static NodePtr SetRight(NodePtr old_root, NodePtr new_right) {
      old_root->_right = std::move(new_right);
      if (!old_root->right() || old_root->priority() >=old_root->right()->priority()) {
        // If new_left is null or the priorites are OK, just recompute the
        // reduction and return.
        old_root->RecomputeReduced();
        return old_root;
      } else {
        return RotateLeft(std::move(old_root));
      }
    }
    // If the left child has higher priority than root, then rotates that child
    // to be the root.  Requires that that child is present.  Returns the new
    // root.
    static NodePtr RotateRight(NodePtr old_root) {
      assert(old_root->left());
      // We shouldn't be rotating unless the priorities are wrong.
      assert(old_root->priority() < old_root->left()->priority());
      // We have
      //               old_root
      //      old_L            old_R
      //  old_LL  old_LR
      //
      // which becomes
      //
      //               old_L
      //      old_LL           old_root
      //                    old_LR     old_R
      NodePtr old_L = std::move(old_root->_left);
      NodePtr old_LR = std::move(old_L->_right);

      old_root->SetLeftAndUpdateReduced(std::move(old_LR));
      old_L->SetRightAndUpdateReduced(std::move(old_root));

      return old_L;
    }
    // If the right child has higher priority than root, then rotates that child
    // to be the root.  Requires that that child is present.  Returns the new
    // root.
    static NodePtr RotateLeft(NodePtr old_root) {
      assert(old_root->right());
      // We shouldn't be rotating unless the priorities are wrong.
      assert(old_root->priority() < old_root->right()->priority());
      // We have
      //               old_root
      //      old_L            old_R
      //                    old_RL  old_RR
      //
      // which becomes
      //
      //               old_R
      //      old_root          old_RR
      //    old_L old_RL
      NodePtr old_R = std::move(old_root->_right);
      NodePtr old_RL = std::move(old_R->_left);

      old_root->SetRightAndUpdateReduced(std::move(old_RL));
      old_R->SetLeftAndUpdateReduced(std::move(old_root));
      return old_R;
    }

    void SetLeftAndUpdateReduced(NodePtr new_left) {
      _left = std::move(new_left);
      RecomputeReduced();
    }

    void SetRightAndUpdateReduced(NodePtr new_right) {
      _right = std::move(new_right);
      RecomputeReduced();
    }

    void RecomputeReduced() {
      _reduced = Reducer(key(), value());
      if (left()) {
        _reduced = left()->reduced() + std::move(_reduced);
      }
      if (right()) {
        _reduced = std::move(_reduced) + right()->reduced();
      }
    }
    size_t _priority;
    K _key;
    V _value;
    Reducer _reduced;
    NodePtr _left;
    NodePtr _right;
  };
 public:
  // Return true if the insertion happened, false if it was already there.
  bool insert(K k, V v);
  std::ostream& Print(std::ostream& os) const;
  void Validate() const;

 private:
  static void PrintNode(std::ostream& os, const NodePtr& node, size_t depth);
  static void Validate(const NodePtr& root,
                       const K* lower_bound,
                       const K* upper_bound);

  std::unique_ptr<Node> _root;
  std::random_device _device;
  std::default_random_engine _engine{_device()};
  std::uniform_int_distribution<size_t> _uniform_distribution;
};

template <class K, class V, class Reducer>
std::ostream& operator<<(std::ostream& os, const ReducerTree<K, V, Reducer>& tree) {
  return tree.Print(os);
}

template <class K, class V, class Reducer>
std::ostream& ReducerTree<K, V, Reducer>::Print(std::ostream& os) const {
  os << "(";
  PrintNode(os, _root, 0);
  os << ")";
  return os;
}

template <class K, class V, class Reducer>
/*static*/ void ReducerTree<K, V, Reducer>::PrintNode(std::ostream& os,
                                                      const NodePtr& node,
                                                      size_t depth) {
  if (!node) {
    os << "_";
    return;
  }
  os << "(" << node->key() << " " << node->value() << " " << node->priority() << " " << node->reduced().value();
  os << std::endl << std::string(depth, ' ') << " ";
  PrintNode(os, node->left(), depth + 1);
  os << std::endl << std::string(depth, ' ') << " ";
  PrintNode(os, node->right(), depth + 1);
  os << ")";
  return;
}

template <class K, class V, class Reducer>
void ReducerTree<K, V, Reducer>::Validate() const {
  Validate(_root, nullptr, nullptr);
}

template <class K, class V, class Reducer>
/*static*/ void ReducerTree<K, V, Reducer>::Validate(const NodePtr& root,
                                                     const K* lower_bound,
                                                     const K* upper_bound) {
  if (root == nullptr) {
    return;
  }
  if (lower_bound) {
    assert(*lower_bound < root->key());
  }
  if (upper_bound) {
    assert(root->key() < *upper_bound);
  }
  Validate(root->left(), lower_bound, &root->key());
  Validate(root->right(), &root->key(), upper_bound);
  if (root->left()) {
    assert(root->priority() > root->left()->priority());
  }
  if (root->right()) {
    assert(root->priority() > root->right()->priority());
  }
  Reducer rhere = Reducer(root->key(), root->value());
  if (root->left()) {
    rhere = root->left()->reduced() + rhere;
  }
  if (root->right()) {
    rhere = rhere + root->right()->reduced();
  }
  assert(rhere == root->reduced());
}



template <class K, class V, class Reducer>
bool ReducerTree<K, V, Reducer>::insert(K k, V v) {
  Reducer reduced{k, v};
  NodePtr node = std::make_unique<Node>(_uniform_distribution(_engine),
                                        std::move(k),
                                        std::move(v));
  bool result;
  _root = Node::Insert(std::move(_root), std::move(node), result);
  return result;
}

#endif  // _REDUCER_TREE_H
