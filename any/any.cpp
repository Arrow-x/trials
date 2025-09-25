#include <iostream>
#include <any>
#include <string>
#include <vector>
#include <array>

// Global new/delete overloads (just for demonstration)
void* operator new(std::size_t size) {
    std::cout << "GLOBAL new(" << size << ") called\n";
    return std::malloc(size);
}
void operator delete(void* ptr) noexcept {
    std::cout << "GLOBAL delete called\n";
    std::free(ptr);
}

// A custom class with its own potentially large data
class MyData {
public:
    std::string name; // This string might allocate on its own
    std::vector<int> numbers; // This vector will allocate on its own

    MyData(const std::string& n, int count) : name(n) {
        std::cout << "MyData Constructor: " << n << std::endl;
        numbers.resize(count); // This might cause std::vector to allocate
    }
    MyData(const MyData& other) : name(other.name), numbers(other.numbers) {
        std::cout << "MyData Copy Constructor: " << name << std::endl;
    }
    MyData(MyData&& other) noexcept : name(std::move(other.name)), numbers(std::move(other.numbers)) {
        std::cout << "MyData Move Constructor: " << name << std::endl;
    }
    ~MyData() {
        std::cout << "MyData Destructor: " << name << std::endl;
    }

    // Overload for this class itself (will NOT be called by std::any for its top-level storage)
    // static void* operator new(std::size_t size) {
    //     std::cout << "MyData::operator new(" << size << ") called\n";
    //     return ::operator new(size);
    // }
    // static void operator delete(void* ptr, std::size_t size) noexcept {
    //     std::cout << "MyData::operator delete(" << size << ") called\n";
    //     ::operator delete(ptr);
    // }
};

int main() {
    std::cout << "--- Storing a small integer ---\n";
    std::any a1 = 123; // Integer likely fits in SOO, no GLOBAL new from std::any
    std::cout << "Value in a1: " << std::any_cast<int>(a1) << std::endl;

    std::cout << "\n--- Storing a long string ---\n";
    // A long string will likely trigger std::any's heap allocation
    // AND the string's own internal heap allocation
    std::string long_str = "This is a very long string that will definitely go to the heap, bypassing SSO.";
    std::any a2 = long_str; // GLOBAL new (for std::any's storage) AND GLOBAL new (for string's internal buffer)
    std::cout << "Value in a2: " << std::any_cast<std::string>(a2).substr(0, 30) << "...\n";

    std::cout << "\n--- Storing MyData object ---\n";
    // MyData is likely too big for std::any's SOO, so std::any allocates for MyData.
    // MyData's internal string and vector will also allocate.
    std::any a3 = MyData{"TestObject", 100}; // GLOBAL new (for std::any's storage)
                                            // MyData's constructor runs
                                            // GLOBAL new (for MyData::name's internal string data)
                                            // GLOBAL new (for MyData::numbers' internal vector data)

    std::cout << "Accessing MyData: " << std::any_cast<MyData>(a3).name << std::endl;

    std::cout << "\n--- a3 going out of scope ---\n";
    // When a3 destructs, MyData's destructor will be called,
    // which in turn will call GLOBAL delete for its internal string and vector data.
    // Finally, GLOBAL delete will be called by std::any for the MyData object itself.
    return 0;
}
