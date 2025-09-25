#include <iostream>
#include <memory_resource>
#include <new>
#include <ostream>
#include <vector>

int main() {
  std::byte b[1028];
  std::pmr::monotonic_buffer_resource m(b, sizeof(b),
                                        std::pmr::null_memory_resource());
  std::pmr::vector<int> v(&m);

  for (int i = 0; i < 100099; i++) {
    try {
      v.push_back(i);
    } catch (const std::bad_alloc &e) {
      std::cerr << "bad alloc is thrown at iteration number : " << i
                << std::endl;
      std::cerr << "Capacity at failure: " << v.capacity() << std::endl;
      std::cerr << "Size at failure: " << v.size() << std::endl;
      break;
    }
  }
  std::cerr << "I got out?\n" << v.capacity();

  return 0;
}
