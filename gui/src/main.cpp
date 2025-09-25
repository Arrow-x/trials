#include <EASTL/allocator.h> // Good to include for allocator_traits, though not directly used below
#include <EASTL/string.h>
#include <iostream>

#define EASTL_INIT()                                                           \
  void *operator new[](size_t size, const char *pName, int flags,              \
                       unsigned debugFlags, const char *file, int line) {      \
    return new char[size];                                                     \
  }                                                                            \
  void *operator new[](size_t size, size_t alignment, size_t alignmentOffset,  \
                       const char *pName, int flags, unsigned debugFlags,      \
                       const char *file, int line) {                           \
    return new char[size];                                                     \
  }                                                                            \
  void operator delete[](void *p, const char *pName, int flags,                \
                         unsigned debugFlags, const char *file, int line) {    \
    delete[] (char *)p;                                                        \
  }                                                                            \
  void operator delete[](void *p, size_t alignment, size_t alignmentOffset,    \
                         const char *pName, int flags, unsigned debugFlags,    \
                         const char *file, int line) {                         \
    delete[] (char *)p;                                                        \
  }

// 1. Define your custom allocator
class MyArenaAllocator {
public:
  typedef char value_type;
  typedef char *pointer;
  typedef const char *const_pointer;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  char *m_buffer = nullptr;
  size_type m_capacity = 0;
  size_type m_current_offset = 0; // mutable for allocation tracking

  // Default constructor (required by EASTL even if you always pass args)
  MyArenaAllocator() : m_buffer(nullptr), m_capacity(0), m_current_offset(0) {}

  // Constructor to initialize with a buffer
  MyArenaAllocator(char *buffer, size_type capacity)
      : m_buffer(buffer), m_capacity(capacity), m_current_offset(0) {}

  // Copy constructor: The copy should manage the same arena resource
  MyArenaAllocator(const MyArenaAllocator &other)
      : m_buffer(other.m_buffer), m_capacity(other.m_capacity),
        m_current_offset(other.m_current_offset) {
    // Note: For an arena, copying 'm_current_offset' implies both allocators
    // track the same progress. If you want separate tracking, you'd rethink.
    // Usually, for an arena, all allocators associated with it just share the
    // same resource.
  }

  // Conversion constructor from other allocator types:
  // This is vital for EASTL containers. When a container needs to rebind its
  // allocator (e.g., eastl::vector<A, AllocA> needs to create an internal
  // AllocB for pairs), this constructor is called.
  template <typename OtherAllocator>
  MyArenaAllocator(const OtherAllocator &other) {
    // This is a common way to handle conversion for custom allocators.
    // It checks if the 'other' allocator is also a MyArenaAllocator.
    // If so, it copies its state. Otherwise, it might default construct
    // or throw an error if the types are truly incompatible.
    if constexpr (std::is_same_v<OtherAllocator, MyArenaAllocator>) {
      m_buffer = other.m_buffer;
      m_capacity = other.m_capacity;
      m_current_offset = other.m_current_offset;
    } else {
      // If you have a hierarchy of allocators or a system to get a base
      // resource, you'd put that logic here. For completely unrelated
      // allocators, this might mean defaulting to an invalid state or even
      // asserting/throwing. For a simple arena, it's often reasonable to
      // default construct if unknown.
      m_buffer = nullptr;
      m_capacity = 0;
      m_current_offset = 0;
      std::cerr << "Warning: MyArenaAllocator converting from unknown "
                   "allocator type. Defaulting to empty."
                << std::endl;
    }
  }

  // Allocation method
  void *allocate(size_type n, int flags = 0) {
    // Simple alignment. For robust allocators, use std::align or custom align
    // logic.
    size_type aligned_offset =
        (m_current_offset + (alignof(void *) - 1)) & ~(alignof(void *) - 1);

    if (m_buffer && (aligned_offset + n <= m_capacity)) {
      void *p = m_buffer + aligned_offset;
      m_current_offset = aligned_offset + n;
      std::cout << "MyArenaAllocator: Allocated " << n
                << " bytes (offset: " << aligned_offset << ")" << std::endl;
      return p;
    }
    std::cerr << "MyArenaAllocator: Allocation FAILED for " << n << " bytes!"
              << std::endl;
    return nullptr;
  }

  // Deallocation method (often a no-op for arenas)
  void deallocate(void *p, size_type n) {
    std::cout << "MyArenaAllocator: Deallocated " << n << " bytes at " << p
              << " (NO-OP for arena)" << std::endl;
  }

  // Comparison operators (crucial for stateful allocators)
  // Two arena allocators are equal if they manage the same buffer
  bool operator==(const MyArenaAllocator &other) const {
    return m_buffer == other.m_buffer;
  }

  bool operator!=(const MyArenaAllocator &other) const {
    return !(*this == other);
  }
};

int main() {
  // Ensure you have enough memory for the arena
  // Make it larger to handle reallocations, etc.
  static char my_buffer[2048]; // Using static to ensure it exists for main
  MyArenaAllocator arena_alloc(my_buffer, sizeof(my_buffer));

  // Define the string type with our custom allocator
  using ArenaString = eastl::basic_string<char, MyArenaAllocator>;

  // Create strings using the arena allocator
  ArenaString s1("Hello from Arena String!", arena_alloc);
  std::cout << "s1: " << s1.c_str()
            << " (Current arena offset: " << arena_alloc.m_current_offset << ")"
            << std::endl;

  ArenaString s2("Another shorter string", arena_alloc);
  std::cout << "s2: " << s2.c_str()
            << " (Current arena offset: " << arena_alloc.m_current_offset << ")"
            << std::endl;

  // Appending to s1 might trigger reallocation, using the same arena.
  // The previous memory used by s1 would become "fragmentation" in the arena,
  // only reclaimed when the arena is fully reset.
  s1 += " This is some additional data to append.";
  std::cout << "s1 after append: " << s1.c_str()
            << " (Current arena offset: " << arena_alloc.m_current_offset << ")"
            << std::endl;

  // An important note for arena allocators:
  // Memory is allocated linearly. Deallocations don't free up space in the
  // middle. To reuse the arena, you typically reset its offset. Make sure all
  // objects allocated from it are out of scope or explicitly destroyed before
  // resetting, or you'll have dangling pointers.
  std::cout << "\n--- Resetting Arena ---" << std::endl;
  arena_alloc.m_current_offset =
      0; // This effectively 'clears' the arena for new allocations

  ArenaString s3("New string after arena reset", arena_alloc);
  std::cout << "s3: " << s3.c_str()
            << " (Current arena offset: " << arena_alloc.m_current_offset << ")"
            << std::endl;

  // --- Using default eastl::string (uses global new/delete overloads) ---
  std::cout << "\n--- Default eastl::string ---" << std::endl;
  eastl::string default_s = "This uses the global new/delete.";
  std::cout << "Default string: " << default_s.c_str() << std::endl;

  return 0;
}