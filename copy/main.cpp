import std;
import mymodule;

std::uint32_t global_id = 0;

struct OG_Class {
  std::uint32_t id;
  std::uint32_t gen;
  std::uint32_t age;
  std::string name;
  std::vector<std::string> friends;
  std::map<std::string, int> friend_to_id;
  std::function<void(double)> fixed_tick;
  std::function<void(double)> tick;
};

int main() {
  auto mason = new OG_Class{
      .id = global_id++,
      .gen = 1,
      .age = 36,
      .name = "Mason",
      .friends = {"John", "May"},
      .friend_to_id = {{"John", 1}, {"May", 5}},
      .fixed_tick =
          [](double delta) {
            std::println("hi from mason fixed_tick {}", delta);
          },
      .tick =
          [](double delta) { std::println("form mason's tick {}", delta); }};

  std::println("mason : {} {}, {}, {}, {}, {}", mason->name, mason->age,
               mason->friends[0], mason->friends[1],
               mason->friend_to_id.at(mason->friends[0]),
               mason->friend_to_id.at(mason->friends[1]));
  mason->fixed_tick(12);
  mason->tick(8);
  std::println("size of the mason struct is: {}", sizeof(*mason));

  std::println("{}", add_numbs(3, 5, 4));

  auto f2 = std::make_unique<OG_Class>(*mason);

  f2->age = 35;
  f2->name = "Chun";
  f2->friends = {"Ruy", "Ken"};
  f2->friend_to_id[f2->friends[0]] = 265;
  f2->friend_to_id[f2->friends[1]] = 128;
  std::println("f2: {}, {}, {}, {}, {}", f2->age, f2->friends[0],
               f2->friends[1], f2->friend_to_id.at(f2->friends[0]),
               f2->friend_to_id.at(f2->friends[1]));

  std::println("f1: {}, {}, {}, {}, {}", mason->age, mason->friends[0],
               mason->friends[1], mason->friend_to_id.at(mason->friends[0]),
               mason->friend_to_id.at(mason->friends[1]));
}
