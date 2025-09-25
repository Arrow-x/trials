#include <cstddef>
#include <iostream>
#include <memory_resource>
#include <string>

struct CustomEntity {
	int update(int a, int b) { return a * b; };
	std::string name = "";
	int age = 0;
	float height = 0;
};

int main() {
	std::byte stackBuf[2048];
	std::pmr::monotonic_buffer_resource resource(stackBuf, sizeof stackBuf);
	std::pmr::polymorphic_allocator<CustomEntity> allocator(&resource);

	CustomEntity *c = allocator.allocate(1);

	allocator.construct(c, "Greate Struct", 15, 1.82);


	std::cout << c->name << std::endl;
    std::cout << c->update(40, 6) << std::endl;
}
