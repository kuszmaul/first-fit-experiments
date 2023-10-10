#include "reducer_tree.h"

#include <map>
#include <random>

class StringToLengthReducer {
 public:
  StringToLengthReducer() = default;
  StringToLengthReducer(size_t,
                        const std::string& value) :_size(value.size()) {}
  StringToLengthReducer operator+(const StringToLengthReducer& other) const {
    return StringToLengthReducer(_size + other._size);
  }
  size_t value() const { return _size; }
  size_t value_view() const { return _size; }
 private:
  explicit StringToLengthReducer(size_t size) :_size(size) {}
  size_t _size;
};

struct Empty {
  friend std::ostream& operator<<(std::ostream& os, Empty) {
    return os << "{}";
  }
  friend bool operator==(Empty, Empty) { return true; }
};

class StringCatReducer {
 public:
  StringCatReducer() = default;
  explicit StringCatReducer(std::string k, Empty = Empty())
      :_string(std::move(k)) {}
  StringCatReducer operator+(const StringCatReducer& other) const {
    return StringCatReducer(_string + other._string);
  }
  std::string value() const { return _string; }
  const std::string& value_value() const { return _string; }
 private:
  std::string _string;
};

template<class T, class C>
void CheckTreeContains(const T& tree, const C& ordered_container) {
  tree.Validate();
  //assert(tree.Size() == ordered_container.size());
  // Everything in the container is in the tree.
  for (const auto& [key, value] : ordered_container ) {
    auto result = tree.Find(key);
    assert(result);
    const auto& [found_key, found_value, found_reducer] = *result;
    assert(found_key == key);
    assert(found_value == value);
  }
  // The set of values returned by the tree iterator exactly equals the set in
  // `ordered_container`.
  C found_values;
  bool all_ok = tree.ForAll([&](const typename T::key_type& key,
                                const typename T::value_type& value,
                                [[maybe_unused]] const typename T::reducer_type& reduced) {
    auto [it, inserted] = found_values.insert({key, value});
    assert(inserted);
    return true;
  });
  assert(all_ok);
  assert(found_values == ordered_container);
}

static void NodeTestSplitEmpty() {
  using Node = ReducerNode<std::string, Empty, StringCatReducer>;
  auto [l, r] = Node::Split(nullptr, "a");
  assert(!l && !r);
}

static void NodeTestSplitOneLeft() {
  using Node = ReducerNode<std::string, Empty, StringCatReducer>;
  auto [l, r] = Node::Split(Node::MakeNodeForTest(10, "b", Empty()), "a");
  assert(!l);
  assert(r);
  assert(r->KeyForTest() == "b");
}

static void NodeTestSplitOneRight() {
  using Node = ReducerNode<std::string, Empty, StringCatReducer>;
  auto [l, r] = Node::Split(Node::MakeNodeForTest(10, "b", Empty()), "c");
  assert(l);
  assert(l->KeyForTest() == "b");
  assert(!r);
}

static void NodeTestInsert() {
  using Node = ReducerNode<std::string, Empty, StringCatReducer>;
  Node::Ptr b = Node::MakeNodeForTest(2, "b", Empty());
  Node *bp = b.get();
  Node::Ptr a = Node::MakeNodeForTest(3, "a", Empty(), nullptr, std::move(b));
  Node *ap = a.get();
  Node::Ptr c = Node::MakeNodeForTest(1, "c", Empty());
  //Node *cp = b.get();
  Node::Ptr new_root = Node::Insert(std::move(a), std::move(c));
  assert(new_root.get() == ap && new_root->KeyForTest() == "a");
  assert(new_root->LeftForTest() == nullptr);
  assert(new_root->RightForTest() == bp);
}

static void Test1() {
  ReducerTree<size_t, std::string, StringToLengthReducer> tree;
  tree.Validate();
  assert(tree.Insert(3, "hello"));
  tree.Validate();
  assert(tree.Insert(2, "a"));
  tree.Validate();
  std::map<size_t, std::string> expect({{2, "a"}, {3, "hello"}});
  CheckTreeContains(tree, expect);
  tree.Erase(3);
  expect.erase(3);
  CheckTreeContains(tree, expect);
}

static void Test2() {
  ReducerTree<std::string, Empty, StringCatReducer> tree;
  std::map<std::string, Empty> expect;
  CheckTreeContains(tree, expect);

  assert(tree.Insert("a", Empty()));
  expect.insert({"a", Empty()});
  CheckTreeContains(tree, expect);

  assert(tree.Insert("b", Empty()));
  expect.insert({"b", Empty()});
  CheckTreeContains(tree, expect);

  assert(tree.PrefixLt("a").value() == "");
  assert(tree.PrefixLt("b").value() == "a");
  assert(tree.PrefixLt("zzz").value() == "ab");

  assert(tree.Insert("c", Empty()));
  expect.insert({"c", Empty()});
  CheckTreeContains(tree, expect);

  assert(tree.PrefixLt("a").value() == "");
  assert(tree.PrefixLt("b").value() == "a");
  assert(tree.PrefixLt("c").value() == "ab");
  assert(tree.PrefixLt("zzz").value() == "abc");

  assert(tree.Insert("d", Empty()));
  expect.insert({"d", Empty()});
  CheckTreeContains(tree, expect);

  assert(tree.PrefixLt("a").value() == "");
  assert(tree.PrefixLt("b").value() == "a");
  assert(tree.PrefixLt("c").value() == "ab");
  assert(tree.PrefixLt("d").value() == "abc");
  assert(tree.PrefixLt("zzz").value() == "abcd");

  assert(tree.Insert("e", Empty()));
  expect.insert({"e", Empty()});
  CheckTreeContains(tree, expect);

  assert(tree.Insert("f", Empty()));
  expect.insert({"f", Empty()});
  CheckTreeContains(tree, expect);

  assert(tree.PrefixLt("a").value() == "");
  assert(tree.PrefixLt("b").value() == "a");
  assert(tree.PrefixLt("c").value() == "ab");
  assert(tree.PrefixLt("d").value() == "abc");
  assert(tree.PrefixLt("e").value() == "abcd");
  assert(tree.PrefixLt("f").value() == "abcde");
  assert(tree.PrefixLt("zzz").value() == "abcdef");
}

class MaxReducer {
 public:
  MaxReducer() = default;
  MaxReducer(size_t, size_t v) :MaxReducer(v) {}
  MaxReducer operator+(const MaxReducer& other) const {
    return MaxReducer(std::max(_max, other._max));
  }
  size_t value() const { return _max; }
  size_t value_view() const { return _max; }
 private:
  explicit MaxReducer(size_t v) :_max(v) {}
  size_t _max;
};

static void RandomizedTest() {
  std::random_device device;
  std::default_random_engine engine{device()};
  auto GetRandomMod = [&] (size_t max) {
    std::uniform_int_distribution<size_t> distribution(0, max);
    return distribution(engine);
  };
  auto GetRandom = [&] () {
    std::uniform_int_distribution<size_t> distribution;
    return distribution(engine);
  };
  constexpr size_t num_ops = 1000;
  for (size_t trial = 0; trial < 10; ++trial) {
    ReducerTree<size_t, size_t, MaxReducer> tree;
    std::map<size_t, size_t> expect;
    auto Insert = [&] () {
      size_t v = GetRandomMod(num_ops);
      size_t r = GetRandom();
      //std::cout << " Insert " << v << " " << r << std::endl;
      tree.Insert(v, r);
      expect.insert({v, r});
      //tree.Print(std::cout);
      //std::cout << std::endl;
      CheckTreeContains(tree, expect);
    };
    auto Erase = [&] () {
      size_t which = GetRandomMod(tree.Size());
      for (auto it = expect.begin(); it != expect.end(); ++it, --which) {
        if (which == 0) {
          tree.Erase(it->first);
          expect.erase(it);
          break;
        }
      }
      CheckTreeContains(tree, expect);
    };
    for (size_t opnum = 0; opnum < num_ops; ++opnum) {
      if (opnum < num_ops || tree.Empty()) {
        Insert();
      } else {
        switch (GetRandomMod(2)) {
          case 0:
            Insert();
            break;
          case 1:
            Erase();
            break;
        }
      }
    }
  }
}

int main() {
  NodeTestSplitEmpty();
  NodeTestSplitOneLeft();
  NodeTestSplitOneRight();
  NodeTestInsert();
  Test1();
  Test2();
  RandomizedTest();
}
