#include <cassert>
#include <cstddef>
#include <iostream>
#include <set>

class FirstFit;

class Block {
 public:
  Block(size_t start, size_t size) :_start(start), _size(size) {}
  size_t start() const { return _start; }
  size_t size() const { return _size; }

 private:
  size_t _start;
  size_t _size;
};

bool operator<(Block a, Block b) {
  if (a.start() == b.start()) assert(a.size() == b.size());
  return a.start() < b.start();
}

bool operator==(Block a, Block b) {
  if (a.start() == b.start()) assert(a.size() == b.size());
  return a.start() == b.start();
}

std::ostream& operator<<(std::ostream& os, Block b) {
  return os << "{" << b.start() << ", " << b.size() << "}";
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
    assert(block.start() >= prev_end);
    if (block.start() - prev_end >= size) {
      Block block{prev_end, size};
      _allocated_blocks.insert(block);
      return block;
    }
    prev_end = block.start() + block.size();
  }
  size_t end = 0;
  if (!_allocated_blocks.empty()) {
    auto it = _allocated_blocks.end();
    --it;
    end = it->start() + it->size();
  }
  _high_water = std::max(_high_water, end + size);
  Block block{end, size};
  _allocated_blocks.insert(block);
  return block;
}

void FirstFit::Free(Block block) {
  auto it = _allocated_blocks.find(block);
  assert(it != _allocated_blocks.end());
  assert(*it == block);
  _allocated_blocks.erase(it);
}

// A simple test.  Do we reuse allocations?
void Test1() {
  FirstFit ff;
  Block a = ff.Alloc(10);
  std::cout << "Allocated " << a << std::endl;
  ff.Free(a);
  Block b = ff.Alloc(10);
  std::cout << "Allocated " << a << std::endl;
  std::cout << "High-water = " << ff.get_high_water() << std::endl;
  assert(ff.get_high_water() < 20);
  assert(a == b);
}

void Test2() {
  FirstFit ff;
  /*Block a = */ff.Alloc(10);
  Block b = ff.Alloc(15);
  /*Block c = */ff.Alloc(20);
  Block d = ff.Alloc(25);
  /*Block e = */ff.Alloc(30);
  ff.Free(b);
  ff.Free(d);
  Block f = ff.Alloc(21);
  assert(f.start() == 10 + 15 + 20);
  Block g = ff.Alloc(14);
  assert(g.start() == 10);
  Block h = ff.Alloc(2);
  assert(h.start() == 10 + 15 + 20 + 21);
  assert(ff.get_high_water() == 10 + 15 + 20 + 25 + 30);
}

int main() {
  Test1();
  Test2();
}
