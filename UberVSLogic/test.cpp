#include <chrono>
#include <iostream>
#include <random>
#include <vector>

// A massive struct holding all possible data
struct UberEntity {
  int type; // 0 = Ogre, 1 = Box, 2 = Door
  float posX, posY, posZ;
  float velocityX, velocityY, velocityZ;
  int health;
  bool isOpen;     // Relevant for doors
  bool isMovable;  // Relevant for boxes
  int attackPower; // Relevant for ogres
  int defense;
  float weight;
  int someOtherData[10]; // Filler to make it big
};

void updateUber(std::vector<UberEntity> &entities) {
  for (auto &e : entities) {
    switch (e.type) {
    case 0: // Ogre
      e.health -= 1;
      e.posX += e.velocityX;
      break;
    case 1: // Box
      if (e.isMovable)
        e.posX += e.velocityX;
      break;
    case 2: // Door
      if (e.isOpen)
        e.defense += 1;
      break;
    }
  }
}

struct Ogre {
  float posX, posY, posZ;
  float velocityX, velocityY, velocityZ;
  int health;
  int attackPower;
};

struct Box {
  float posX, posY, posZ;
  bool isMovable;
  float weight;
};

struct Door {
  bool isOpen;
  int defense;
};

void updateOgres(std::vector<Ogre> &ogres) {
  for (auto &o : ogres) {
    o.health -= 1;
    o.posX += o.velocityX;
  }
}

void updateBoxes(std::vector<Box> &boxes) {
  for (auto &b : boxes) {
    if (b.isMovable)
      b.posX += 0.5f; // Arbitrary movement
  }
}

void updateDoors(std::vector<Door> &doors) {
  for (auto &d : doors) {
    if (d.isOpen)
      d.defense += 1;
  }
}
using u32 = uint_least32_t;
using engine = std::mt19937;

int main() {
  const int entityCount = 1'000'000;
  std::vector<Ogre> ogres(entityCount / 3);
  std::vector<Box> boxes(entityCount / 3);
  std::vector<Door> doors(entityCount / 3);

  std::random_device os_seed;
  const u32 seed = os_seed();
  engine gen(seed);
  std::uniform_int_distribution<u32> dist(1, 365);

  // Initialize entities
  for (auto &o : ogres) {
    o.posX = dist(gen);
    o.posY = dist(gen);
    o.posZ = dist(gen);
    o.velocityX = dist(gen) * 1.9f;
    o.velocityY = dist(gen) * 1.9f;
    o.velocityZ = dist(gen) * 1.9f;
    o.health = dist(gen);
    o.attackPower = dist(gen);
  }
  for (auto &b : boxes) {
    b.posX = dist(gen);
    b.posY = dist(gen);
    b.posZ = dist(gen);
    b.isMovable = true;
  }
  for (auto &d : doors) {
    d.isOpen = false;
    d.defense = dist(gen);
  }

  auto start = std::chrono::high_resolution_clock::now();
  updateOgres(ogres);
  updateBoxes(boxes);
  updateDoors(doors);
  auto end = std::chrono::high_resolution_clock::now();

  std::cout << "Partitioned update took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                   .count()
            << " milliseconds.\n";

  const int entityCount2 = 1'000'000;
  std::vector<UberEntity> entities(entityCount);

  // Initialize entities randomly
  for (int i = 0; i < entityCount2; ++i) {
    entities[i].type = i % 3; // Cycle through types
    entities[i].posX = i * 0.1f;
    entities[i].velocityX = 0.5f;
    entities[i].health = 100;
    entities[i].isOpen = false;
    entities[i].isMovable = (i % 2 == 0);
  }

  auto start2 = std::chrono::high_resolution_clock::now();
  updateUber(entities);
  auto end2 = std::chrono::high_resolution_clock::now();

  std::cout << "Über-struct update took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end2 -
                                                                     start2)
                   .count()
            << " milliseconds.\n";

  return 0;
}
