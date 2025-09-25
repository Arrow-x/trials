#include <iostream>
#include <memory>
#include <vector>

// Example of a type-erased interface (Concept and Model)
class DrawableConcept {
public:
  virtual ~DrawableConcept() = default;
  virtual void draw() const = 0;
  virtual std::unique_ptr<DrawableConcept> clone() const = 0;
};

template <typename T> class DrawableModel : public DrawableConcept {
public:
  DrawableModel(T val) : value_(std::move(val)) {}
  void draw() const override {
    value_.draw(); // Calls the draw method of the underlying type T
  }
  std::unique_ptr<DrawableConcept> clone() const override {
    return std::make_unique<DrawableModel<T>>(value_);
  }

private:
  T value_;
};

class Canvas {
public:
  template <typename T> void add(T obj) {
    drawables_.push_back(std::make_unique<DrawableModel<T>>(std::move(obj)));
  }

  void render() const {
    for (const auto &d : drawables_) {
      d->draw();
    }
  }

private:
  std::vector<std::unique_ptr<DrawableConcept>> drawables_;
};

struct Circle {
  void draw() const { std::cout << "Drawing a Circle." << std::endl; }
};

struct Square {
  void draw() const { std::cout << "Drawing a Square." << std::endl; }
};

int main() {
  Canvas canvas;
  canvas.add(Circle{});
  canvas.add(Square{});
  canvas.render();
  return 0;
}
