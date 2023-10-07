#include "reducer_tree.h"

#include <map>

class StringToLengthReducer {
 public:
  StringToLengthReducer([[maybe_unused]] size_t k,
                        const std::string& value) :_size(value.size()) {
  }
  StringToLengthReducer operator+(const StringToLengthReducer& other) const {
    return StringToLengthReducer(_size + other._size);
  }
  bool operator==(const StringToLengthReducer& other) const {
    return _size == other._size;
  }
  size_t value() const { return _size; }
 private:
  explicit StringToLengthReducer(size_t size) :_size(size) {}
  size_t _size;
};

template<class T, class C>
void CheckTreeContains(const T& tree, const C& ordered_container) {
  // Everything in the container is in the tree.
  for (const auto& [key, value] : ordered_container ) {
    auto result = tree.Lookup(key);
    assert(result);
    assert(result->key() == key);
    assert(result->value() == value);
  }
  // The set of values returned by the tree iterator exactly equals the set in
  // `ordered_container`.
  C found_values;
  tree.Iterate([&](const T::Node& node) {
    auto [it, inserted] = found_values.insert({node.key(), node.value()});
    assert(inserted);
    return true;
  });
  assert(found_values == ordered_container);
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
}

int main() {
  Test1();
}
