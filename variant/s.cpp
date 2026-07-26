#include <variant>
#include <vector>
namespace game {

enum Type { ENEMY, PLAYER };

struct Enemy {};
struct Player {};

struct TurnCard {
  Type type;
  void *pointer_to_data;
};

void deal_with(Enemy *) {};
void deal_with(Player *) {};
void dispatch(std::vector<TurnCard> &s) {
  for (size_t i = 0; i < s.size(); i++) {
    switch (s[i].type) {
    case Type::ENEMY:
      deal_with((Enemy *)(s[i].pointer_to_data));
      break;
    case Type::PLAYER:
      deal_with((Enemy *)s[i].pointer_to_data);
      break;
    }
  }
}

} // namespace game
struct Visitor {
  void operator()(game::Enemy &) {}
  void operator()(game::Player &) {}
};

typedef std::variant<game::Enemy, game::Player> Unit;
int main() {
  std::vector<game::TurnCard> s;
  dispatch(s);

  Visitor visitor;
  Unit unit;
  std::visit(visitor, unit);

  return 0;
}
