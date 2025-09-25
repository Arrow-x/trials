#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

struct Human {
  enum Sex { MALE, FEMALE };
  enum Districts { DOWNTOWN, PARK, UNIVERSITY };
  enum Tags { POLICE, CRIMINAL };
  char name[16];
  int32_t height;
  int32_t age;
  Sex sex;
  Districts district;
  Tags tag;
  size_t index_to_specialazation;
};

struct Police_Officer {
  int rank;
};

struct Drug_Dealer {
  int wanted_level;
};

int main() {
  std::vector<Police_Officer> popos{{.rank = 4}, {.rank = 3}};
  std::vector<Drug_Dealer> drugees{{.wanted_level = 2}, {.wanted_level = 4}};

  std::vector<Human> npcs{{.name = "john",
                           .height = 182,
                           .age = 43,
                           .sex = Human::Sex::MALE,
                           .district = Human::Districts::DOWNTOWN,
                           .tag = Human::Tags::POLICE,
                           .index_to_specialazation = 1},

                          {.name = "creag",
                           .height = 180,
                           .age = 22,
                           .sex = Human::Sex::MALE,
                           .district = Human::Districts::PARK,
                           .tag = Human::Tags::CRIMINAL,
                           .index_to_specialazation = 1}};

  for (auto &h : npcs) {
    switch (h.tag) {
    case Human::Tags::CRIMINAL:
      std::cout << "A crinal at Wanted level: "
                << drugees[h.index_to_specialazation].wanted_level << std::endl;
      break;
    case Human::Tags::POLICE:
      std::cout << "A cop at Rank: " << popos[h.index_to_specialazation].rank
                << std::endl;
      break;
    }
  }
}
