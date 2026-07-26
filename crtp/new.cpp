#include <concepts>
#include <iostream>
#include <string>

// 1. Define a clean rule: Any class using this must provide a 'log_name()'
// function
template <typename T>
concept HasLogName = requires(const T &obj) {
  { obj.log_name() } -> std::convertible_to<std::string>;
};

// 2. The Mixin: Enforces the rule automatically
template <typename Derived> struct Loggable {
  void log() const {
    // Enforce the concept rule at the exact moment log() is called
    static_assert(HasLogName<Derived>,
                  "Your class must implement string log_name()!");

    const auto &derived = static_cast<const Derived &>(*this);
    std::cout << "[LOG]: " << derived.log_name() << "\n";
  }
};

// 3. Clean CRTP again! No extra helper structs or forward declarations.
struct User : public Loggable<User> {
  std::string name;
  User(std::string n) : name(std::move(n)) {}
  std::string log_name() const { return name; }
};

struct Book : public Loggable<Book> {
  std::string title;
  Book(std::string t) : title(std::move(t)) {}
  std::string log_name() const { return title; }
};

int main() {
  User u{"Alice"};
  Book b{"C++ Templates"};

  u.log(); // Outputs: [LOG]: Alice
  b.log(); // Outputs: [LOG]: C++ Templates
}
