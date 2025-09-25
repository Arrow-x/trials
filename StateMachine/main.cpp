#include "state.hpp"
#include <memory>

int main() {
  auto state = std::make_shared<TransitState>(4);
  state->set_up(4);
  state->pay_off();
  return 0;
}
