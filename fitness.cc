#include <cassert>
#include <cstddef>
#include <iostream>
#include <set>

class FirstFit;

class Block {
 private:
  friend FirstFit;
  Block(size_t start, size_t length) :_start(start), _length(length) {}
 public:
  friend bool operator<(Block a, Block b);
  friend std::ostream& operator<<(std::ostream& os, Block b);

 private:
  size_t _start;
  // We remember the length associated with an address.
  size_t _length;
};

bool operator<(Block a, Block b) {
  if (a._start == b._start) assert(a._length == b._length);
  return a._start < b._start;
}

std::ostream& operator<<(std::ostream& os, Block b) {
  return os << "{" << b._start << ", " << b._length << "}";
}

class FirstFit {
 private:
 public:
  Block Alloc(size_t size);
  void Free(Block address);
  size_t get_high_water() const {
    return _high_water;
  }
 private:
  std::set<Block> _allocated_blocks;
  size_t _high_water = 0;
};

Block FirstFit::Alloc(size_t size) {
  size_t prev_end = 0;
  for (const Block& block : _allocated_blocks) {
    assert(block._start >= prev_end);
    if (block._start - prev_end >= size) {
      Block block{prev_end, size};
      _allocated_blocks.insert(block);
      return block;
    }
    prev_end = block._start + block._length;
  }
  size_t end = 0;
  if (!_allocated_blocks.empty()) {
    auto it = _allocated_blocks.end();
    --it;
    end = it->_start + it->_length;
  }
  _high_water = std::max(_high_water, end + size);
  Block block{end, size};
  _allocated_blocks.insert(block);
  return block;
}

void FirstFit::Free(Block block) {
  auto it = _allocated_blocks.find(block);
  assert(it != _allocated_blocks.end());
  assert(it->_start == block._start);
  assert(it->_length == block._length);
  _allocated_blocks.erase(it);
}

int main() {
  FirstFit ff;
  Block a = ff.Alloc(10);
  std::cout << "Allocated " << a << std::endl;
  ff.Free(a);
  a = ff.Alloc(10);
  std::cout << "Allocated " << a << std::endl;
  std::cout << "High-water = " << ff.get_high_water() << std::endl;
  assert(ff.get_high_water() < 20);
}
