#include "reducer_tree.h"

#include <map>

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

struct Empty {};
std::ostream& operator<<(std::ostream& os, Empty) {
  return os << "{}";
}

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
  bool all_ok = tree.ForAll([&](const T::key_type& key,
                                const T::value_type& value,
                                [[maybe_unused]] const T::reducer_type& reduced) {
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
  std::cout << __FILE__ << ":" << __LINE__ << " " << new_root << std::endl;
  assert(new_root.get() == ap && new_root->KeyForTest() == "a");
  assert(new_root->LeftForTest() == nullptr);
  assert(new_root->RightForTest() == bp);
}

static void Test1() {
  ReducerTree<size_t, std::string, StringToLengthReducer> tree;
  tree.Validate();
  assert(tree.Insert(3, "hello"));
  std::cout << tree << std::endl;
  tree.Validate();
  std::cout << "Validated" << std::endl;
  assert(tree.Insert(2, "a"));
    std::cout << "Inserted 2" << std::endl;
  std::cout << tree << std::endl;
  tree.Validate();
  std::map<size_t, std::string> expect({{2, "a"}, {3, "hello"}});
  CheckTreeContains(tree, expect);
  tree.Erase(3);
  expect.erase(3);
  tree.Validate();
  CheckTreeContains(tree, expect);
}

static void Test2() {
  ReducerTree<std::string, Empty, StringCatReducer> tree;
  tree.Validate();
  assert(tree.Insert("a", Empty()));

  assert(tree.Insert("b", Empty()));
  assert(tree.PrefixLt("a").value() == "");
  assert(tree.PrefixLt("b").value() == "a");
  assert(tree.PrefixLt("zzz").value() == "ab");

  std::cout << "AB:" << std::endl << tree << std::endl;
  tree.Validate();
  assert(tree.Insert("c", Empty()));
  std::cout << "ABC:" << std::endl << tree << std::endl;
  tree.Validate();
  assert(tree.PrefixLt("a").value() == "");
  assert(tree.PrefixLt("b").value() == "a");
  assert(tree.PrefixLt("c").value() == "ab");
  assert(tree.PrefixLt("zzz").value() == "abc");

  assert(tree.Insert("d", Empty()));
  assert(tree.PrefixLt("a").value() == "");
  assert(tree.PrefixLt("b").value() == "a");
  assert(tree.PrefixLt("c").value() == "ab");
  assert(tree.PrefixLt("d").value() == "abc");
  assert(tree.PrefixLt("zzz").value() == "abcd");

  assert(tree.Insert("e", Empty()));
  assert(tree.Insert("f", Empty()));
  assert(tree.PrefixLt("a").value() == "");
  std::cout << tree << std::endl;
  std::cout << tree.PrefixLt("b").value() << std::endl;
  assert(tree.PrefixLt("b").value() == "a");
  std::cout << tree.PrefixLt("c").value() << std::endl;
  assert(tree.PrefixLt("c").value() == "ab");
}

int main() {
  NodeTestSplitEmpty();
  NodeTestSplitOneLeft();
  NodeTestSplitOneRight();
  NodeTestInsert();
  Test1();
  Test2();
}
