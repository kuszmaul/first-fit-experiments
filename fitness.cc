#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

class FirstFit;

class Address {
 private:
  friend FirstFit;
  Address(size_t address) :_address(address) {}
  size_t _address;
};

class FirstFit {
 private:
  struct Block {
    size_t start;
    size_t len;
  };
  std::vector<Block> busy_blocks;
 public:
  Address Alloc(size_t size);
  void Free([[maybe_unused]]  Address address) {
  }
  size_t get_high_water() const {
    return _high_water;
  }
 private:
  size_t _high_water = 0;
};

Address FirstFit::Alloc(size_t size) {
  size_t here = _high_water;
  _high_water += size;
  return Address(here);
}

int main() {
  FirstFit ff;
  Address a = ff.Alloc(10);
  ff.Free(a);
  a = ff.Alloc(10);
  std::cout << "High-water = " << ff.get_high_water() << std::endl;
  assert(ff.get_high_water() < 20);
}
