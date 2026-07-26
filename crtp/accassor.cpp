#include <iostream>
#include <string>

// The Mixin: Expects a Getter policy class
template <typename Derived, typename Getter> struct Loggable {
  void log() const {
    const auto &derived = static_cast<const Derived &>(*this);
    // Delay lookup: Extract the value using the Getter at call-time
    std::cout << "[LOG]: " << Getter::get(derived) << std::endl;
  }
};

// --- Class 1: User ---
struct User; // Forward declaration

// Policy on how to get data from User
struct UserGetter {
  static std::string get(const User &u);
};

struct User : public Loggable<User, UserGetter> {
  std::string name;
};

// Inline definition after User is fully defined
inline std::string UserGetter::get(const User &u) { return u.name; }

// --- Class 2: Book ---
struct Book; // Forward declaration

struct BookGetter {
  static std::string get(const Book &b);
};

struct Book : public Loggable<Book, BookGetter> {
  std::string title;
};

inline std::string BookGetter::get(const Book &b) { return b.title; }

// --- Main ---
int main() {
  User u;
  u.name = "Alice";

  Book b;
  b.title = "C++ Templates";

  u.log(); // Outputs: [LOG]: Alice
  b.log(); // Outputs: [LOG]: C++ Templates

  return 0;
}
