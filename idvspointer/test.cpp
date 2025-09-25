#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>

struct Entity {
    uint32_t id;
    float value;

    void update() {
        value += 1.0f;  // Simple operation to simulate work
    }
};

constexpr size_t NUM_ENTITIES = 1000000;

int main() {
    // Create entities
    std::vector<Entity> entities;
    entities.reserve(NUM_ENTITIES);

    // Vector of pointers for direct access
    std::vector<Entity*> entity_ptrs;
    entity_ptrs.reserve(NUM_ENTITIES);

    // Hash map for ID-based access
    std::unordered_map<uint32_t, size_t> entity_map;

    // Fill data
    for (size_t i = 0; i < NUM_ENTITIES; i++) {
        entities.push_back({static_cast<uint32_t>(i), 0.0f});
        entity_ptrs.push_back(&entities.back());
        entity_map[i] = i;
    }

    // Shuffle entities (to avoid cache benefits of sequential access)
    // std::random_device rd;
    // std::mt19937 g(rd());
    // std::shuffle(entity_ptrs.begin(), entity_ptrs.end(), g);

    // Pointer-based iteration
    auto start1 = std::chrono::high_resolution_clock::now();
    for (const auto& e : entity_ptrs) {
        e->update();
    }
    auto end1 = std::chrono::high_resolution_clock::now();

    // Hash map lookup iteration
    auto start2 = std::chrono::high_resolution_clock::now();
    for (const auto& [id, index] : entity_map) {
        entities[index].update();
    }
    auto end2 = std::chrono::high_resolution_clock::now();

    // Results
    std::cout << "Pointer-based iteration: "
              << std::chrono::duration<double, std::milli>(end1 - start1).count() << " ms\n";

    std::cout << "Hash map lookup iteration: "
              << std::chrono::duration<double, std::milli>(end2 - start2).count() << " ms\n";

    return 0;
}
