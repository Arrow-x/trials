#include <chrono>
#include <iostream>
#include <vector>

#define TIME_IT(code)                                                          \
  {                                                                            \
    auto start = std::chrono::high_resolution_clock::now();                    \
    code;                                                                      \
    auto end = std::chrono::high_resolution_clock::now();                      \
    std::cout                                                                  \
        << "Time: "                                                            \
        << std::chrono::duration<double, std::milli>(end - start).count()      \
        << "ms\n";                                                             \
  }

constexpr size_t NUM_ENTITIES = 1000000; // 1 million entities
// -------------------- Expanded Fat Struct (AoS) --------------------
struct FatStruct {
  float x, y, z;                            // 12 bytes
  float velocityX, velocityY, velocityZ;    // 12 bytes
  int health, armor, damage;                // 12 bytes
  float state;                              // 4 bytes
  float extraData1, extraData2, extraData3; // 12 bytes
  // Additional fields to make the struct > 128 bytes
  float extraData4, extraData5, extraData6; // 12 bytes
  int extraInt1, extraInt2;                 // 8 bytes
  float extraFloat1, extraFloat2;           // 8 bytes
  int padding[5];                           // 20 bytes for alignment
  // Total = 172 bytes (well over 128 bytes)
};

std::vector<FatStruct> fatStructEntities(NUM_ENTITIES);

void fat_struct_update() {
  for (auto &entity : fatStructEntities) {
    // Use all fields in a computation
    entity.x += entity.velocityX;
    entity.y += entity.velocityY;
    entity.z += entity.velocityZ;
    entity.armor += entity.extraInt1;
    entity.damage += 10 - entity.armor;
    entity.health -= entity.damage;
    entity.state *= entity.extraData2;
    entity.extraData1 = entity.extraData1 * 0.99f + entity.extraData2;
    entity.extraData2 = entity.extraData2 * 0.98f + entity.extraData3;
    entity.extraData3 = entity.extraData3 * 0.97f + entity.extraData4;
    entity.extraData4 = entity.extraData4 * 0.96f + entity.extraData5;
    entity.extraData5 = entity.extraData5 * 0.95f + entity.extraData6;
    entity.extraData6 = entity.extraData6 * 0.94f + entity.extraData1;
    entity.extraInt1 += 10;
    entity.extraInt2 += 20;
    entity.extraFloat1 *= 1.2f;
    entity.extraFloat2 *= 1.3f;
  }
}

// -------------------- Structure of Arrays (SoA) --------------------
struct Position {
  float x, y, z;
};

struct Velocity {
  float velocityX, velocityY, velocityZ;
};

struct Combat {
  int health, armor, damage;
};

struct State {
  float state;
};

struct ExtraData {
  float extraData1, extraData2, extraData3, extraData4, extraData5, extraData6;
};

struct ExtraInts {
  int extraInt1, extraInt2;
};

struct ExtraFloats {
  float extraFloat1, extraFloat2;
};

std::vector<Position> positions(NUM_ENTITIES);
std::vector<Velocity> velocities(NUM_ENTITIES);
std::vector<Combat> combats(NUM_ENTITIES);
std::vector<State> states(NUM_ENTITIES);
std::vector<ExtraData> extraData(NUM_ENTITIES);
std::vector<ExtraInts> extraInts(NUM_ENTITIES);
std::vector<ExtraFloats> extraFloats(NUM_ENTITIES);

// // Position & Velocity Combined
// struct PositionVelocity {
//   float x, y, z;
//   float velocityX, velocityY, velocityZ;
// };
//
// // Combat Data Combined
// struct CombatData {
//   int health, armor, damage;
// };
//
// // Extra Data in a single struct
// struct ExtraData {
//   float extraData1, extraData2, extraData3;
//   float extraData4, extraData5, extraData6;
//   int extraInt1, extraInt2;
//   float extraFloat1, extraFloat2;
// };
//
// // Use SoA with these mini-structs instead
// std::vector<PositionVelocity> posVel(NUM_ENTITIES);
// std::vector<CombatData> combat(NUM_ENTITIES);
// std::vector<ExtraData> extra(NUM_ENTITIES);
// std::vector<float> states(NUM_ENTITIES); // Keep single floats separate

void soa_update() {
  for (size_t i = 0; i < NUM_ENTITIES; ++i) {
    // Use all fields from the separate arrays
    positions[i].x += velocities[i].velocityX;
    positions[i].y += velocities[i].velocityY;
    positions[i].z += velocities[i].velocityZ;
    combats[i].health += extraData[i].extraData2;
    combats[i].armor += extraData[i].extraData4;
    combats[i].damage += 2;
    states[i].state *= extraFloats[i].extraFloat1;
    extraData[i].extraData1 =
        extraData[i].extraData1 * 0.99f + extraData[i].extraData2;
    extraData[i].extraData2 =
        extraData[i].extraData2 * 0.98f + extraData[i].extraData3;
    extraData[i].extraData3 =
        extraData[i].extraData3 * 0.97f + extraData[i].extraData4;
    extraData[i].extraData4 =
        extraData[i].extraData4 * 0.96f + extraData[i].extraData5;
    extraData[i].extraData5 =
        extraData[i].extraData5 * 0.95f + extraData[i].extraData6;
    extraInts[i].extraInt1 += 10;
    extraInts[i].extraInt2 += 20;
    extraFloats[i].extraFloat1 *= 1.2f;
    extraFloats[i].extraFloat2 *= 1.3f;
  }
}

// void soa_update() {
//   for (size_t i = 0; i < NUM_ENTITIES; ++i) {
//     posVel[i].x += posVel[i].velocityX;
//     posVel[i].y += posVel[i].velocityY;
//     posVel[i].z += posVel[i].velocityZ;
//
//     combat[i].health += 5;
//     combat[i].armor += 3;
//     combat[i].damage += 2;
//
//     states[i] *= 1.1f;
//
//     extra[i].extraData1 = extra[i].extraData1 * 0.99f + extra[i].extraData2;
//     extra[i].extraData2 = extra[i].extraData2 * 0.98f + extra[i].extraData3;
//     extra[i].extraData3 = extra[i].extraData3 * 0.97f + extra[i].extraData4;
//     extra[i].extraData4 = extra[i].extraData4 * 0.96f + extra[i].extraData5;
//     extra[i].extraData5 = extra[i].extraData5 * 0.95f + extra[i].extraData6;
//
//     extra[i].extraInt1 += 10;
//     extra[i].extraInt2 += 20;
//
//     extra[i].extraFloat1 *= 1.2f;
//     extra[i].extraFloat2 *= 1.3f;
//   }
// }
// ---------------- Benchmark -----------------
int main() {
  // Initialize Fat Struct
  for (auto &entity : fatStructEntities) {
    entity = {3.3f, 2.9f, 2.2f, 1.7f, 3.0f, 9.4f, 138, 93,   20,   1.0f, 4.8f,
              0.1f, 0.2f, 0.9f, 4.0f, 2.0f, 30,   20,  1.0f, 1.0f, {5}};
  }

  // Initialize Structure of Arrays
  for (size_t i = 0; i < NUM_ENTITIES; ++i) {
    // posVel[i].x = 0.4f;
    // posVel[i].y = 0.4f;
    // posVel[i].z = 0.4f;
    //
    // combat[i].health = 5;
    // combat[i].armor = 3;
    // combat[i].damage = 2;
    //
    // states[i] = 1.1f;
    //
    // extra[i].extraData1 = 0.99f;
    // extra[i].extraData2 = 0.98f;
    // extra[i].extraData3 = 0.97f;
    // extra[i].extraData4 = 0.96f;
    // extra[i].extraData5 = 0.95f;
    //
    // extra[i].extraInt1 = 10;
    // extra[i].extraInt2 = 20;
    //
    // extra[i].extraFloat1 = 1.2f;
    // extra[i].extraFloat2 = 1.3f;
    positions[i] = {1.4f, 8.3f, 9.4f};
    velocities[i] = {1.4f, 6.9f, 3.5f};
    combats[i] = {100, 50, 20};
    states[i] = {1.0f};
    extraData[i] = {0.3f, 1.4f, 6.2f, 4.4f, 4.1f, 1.4f};
    extraInts[i] = {10, 20};
    extraFloats[i] = {3.6f, 1.3f};
  }

  std::cout << "Fat Struct (AoS) Update: ";
  TIME_IT(fat_struct_update());

  std::cout << "Structure of Arrays (SoA) Update: ";
  TIME_IT(soa_update());

  return 0;
}
