#include <iostream>
#include <string>

// CRTP Mixin 1: Adds logging capability
template <typename Derived> struct Loggable {
  void log() const {
    // Cast 'this' to access fields of the Derived class
    const auto &derived = static_cast<const Derived &>(*this);
    std::cout << "[LOG]: " << derived.name << std::endl;
  }
};

// CRTP Mixin 2: Adds XML serialization capability
template <typename Derived> struct XmlSerializable {
  void to_xml() const {
    const auto &derived = static_cast<const Derived &>(*this);
    std::cout << "<user><name>" << derived.name << "</name></user>\n";
  }
};

// Core class composing both capabilities via multiple CRTP inheritance
struct User : public Loggable<User>, public XmlSerializable<User> {
  std::string name;
};

int main() {
  User user;
  user.name = "Alice";

  user.log();    // Provided by Loggable CRTP mixin
  user.to_xml(); // Provided by XmlSerializable CRTP mixin

  return 0;
}
