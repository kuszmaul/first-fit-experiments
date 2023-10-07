#include "reducer_tree.h"

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

int main() {
  ReducerTree<size_t, std::string, StringToLengthReducer> tree;
  tree.Validate();
  assert(tree.insert(3, "hello"));
  std::cout << tree << std::endl;
  tree.Validate();
  std::cout << "Validated" << std::endl;
  assert(tree.insert(2, "a"));
    std::cout << "Inserted 2" << std::endl;
  std::cout << tree << std::endl;
  tree.Validate();

}
