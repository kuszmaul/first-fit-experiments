#include "reducer_tree.h"

#include <map>

class StringToLengthReducer {
 public:
  StringToLengthReducer() = default;
  StringToLengthReducer([[maybe_unused]] size_t k,
                        const std::string& value) :_size(value.size()) {}
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

int main() {
  Test1();
}
