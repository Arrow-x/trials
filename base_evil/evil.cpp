#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

using namespace std;

struct my_class_t {
  int age;
  string name;
};

struct another_t {
  float hight;
  int weight;
};

void draw(const another_t &x, ostream &out, size_t position) {
  out << to_string(position) << " " << "another_t \n";
}

void draw(const my_class_t &x, ostream &out, size_t position) {
  out << to_string(position) << " " << "my_class_t\n";
}

void draw(const int &x, ostream &out, size_t position) {
  out << to_string(position) << " " << "int \n";
}

void draw(const float &x, ostream &out, size_t position) {
  out << to_string(position) << " " << "float \n";
}

void draw(const string &x, ostream &out, size_t position) {
  out << to_string(position) << " " << "string\n";
}

// template <typename T> void draw(const T &x, ostream &out, size_t position) {
//   out << to_string(position) << " T " << x << endl;
// }

class object_t {
public:
  template <typename T>
  object_t(T x) : self_(make_shared<model<T>>(std::move(x))) {}

  friend void draw(const object_t &x, ostream &out, size_t position) {
    x.self_->draw_(out, position);
  }

private:
  struct concept_t {
    virtual ~concept_t() = default;
    virtual void draw_(ostream &, size_t) const = 0;
  };

  template <typename T> struct model : concept_t {
    model(T x) : data_(std::move(x)) {}
    void draw_(ostream &out, size_t position) const {
      draw(data_, out, position);
    }
    T data_;
  };

  shared_ptr<const concept_t> self_;
};

using document_t = vector<object_t>;

void draw(const document_t &x, ostream &out, size_t position) {
  out << "<document>" << endl;
  for (const auto &e : x)
    draw(e, out, position + 2);
  out << "</document>" << endl;
}

using history_t = vector<document_t>;
void commit(history_t &x) {
  assert(x.size());
  x.push_back(x.back());
}
void undo(history_t &x) {
  assert(x.size());
  x.pop_back();
}
document_t &current(history_t &x) {
  assert(x.size());
  return x.back();
}

int main() {
  document_t document;
  document.emplace_back(0);
  document.emplace_back(0.9f);
  document.emplace_back(string("hello"));
  document.emplace_back(document);
  document.emplace_back(my_class_t({.age = 13, .name = "James"}));
  document.emplace_back(another_t({.hight = 1.8f, .weight = 76}));
  draw(document, cout, 0);


  return 0;
}
