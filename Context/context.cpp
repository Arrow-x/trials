#include <iostream>
#include <memory_resource>
#include <stack>
#include <string>

// === Memory Context ===
struct MemoryContext {
    std::pmr::memory_resource* gmem;  // Garbage memory (clears each frame)
    std::pmr::memory_resource* smem;  // Scoped memory (persists until user clears)
    std::stack<std::pmr::memory_resource*> smemStack;  // Stack for nested scopes
};

// === Scoped Memory Override (Clears Temporary Allocators) ===
struct ScopedMemory {
    MemoryContext& ctx;
    std::pmr::monotonic_buffer_resource* tempResource;  // Track temp allocators

    explicit ScopedMemory(MemoryContext& ctx, std::pmr::memory_resource* new_smem)
        : ctx(ctx), tempResource(nullptr) {
        ctx.smemStack.push(ctx.smem);  // Push current smem onto the stack
        ctx.smem = new_smem;           // Switch to new scoped allocator

        if (auto* monotonic = dynamic_cast<std::pmr::monotonic_buffer_resource*>(new_smem)) {
            tempResource = monotonic;  // Track it for cleanup
        }
    }

    ~ScopedMemory() {
        ctx.smem = ctx.smemStack.top();  // Restore previous smem
        ctx.smemStack.pop();  // Pop from the stack
    }
};

// === Garbage Allocation ===
template <typename T, typename... Args>
T* galloc(MemoryContext& ctx, Args&&... args) {
    return new (ctx.gmem->allocate(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
}

// === Scoped Allocation ===
template <typename T, typename... Args>
T* salloc(MemoryContext& ctx, Args&&... args) {
    return new (ctx.smem->allocate(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
}

// === String Helpers ===
std::pmr::string gstring(MemoryContext& ctx, const char* str) {
    return std::pmr::string(str, ctx.gmem);
}

std::pmr::string sstring(MemoryContext& ctx, const char* str) {
    return std::pmr::string(str, ctx.smem);
}

// === Example Usage ===
int main() {
    std::byte buffer1[1024 * 1024];  // 1MB Garbage Memory
    std::byte buffer2[1024 * 1024];  // 1MB Scoped Memory

    std::pmr::monotonic_buffer_resource garbageMem(buffer1, sizeof(buffer1));
    std::pmr::monotonic_buffer_resource scopedMem(buffer2, sizeof(buffer2));

    MemoryContext ctx{&garbageMem, &scopedMem};  // Default allocators

    // Garbage allocations
    auto str1 = gstring(ctx, "Garbage String");
    auto obj1 = galloc<int>(ctx, 123);
    std::cout << "gmem: " << str1 << ", " << *obj1 << "\n";

    // Scoped allocations
    auto str2 = sstring(ctx, "Scoped String");
    auto obj2 = salloc<int>(ctx, 456);
    std::cout << "smem: " << str2 << ", " << *obj2 << "\n";

    std::pmr::monotonic_buffer_resource subScopedMem(1024 * 1024, ctx.smem);
    std::pmr::monotonic_buffer_resource deeperScopedMem(512 * 1024, ctx.smem);
	std::pmr::string deep;


    // Scoped override (temporary allocator)
    {
        ScopedMemory scope(ctx, &subScopedMem);

        auto str3 = sstring(ctx, "Nested Scoped String");
        auto obj3 = salloc<int>(ctx, 789);
        std::cout << "Sub-scope smem: " << str3 << ", " << *obj3 << "\n";

        // Deeply nested scope
        {
            ScopedMemory deeperScope(ctx, &deeperScopedMem);

            deep = sstring(ctx, "Deepest Scoped String");
            std::cout << "Deep scope smem: " << deep<< "\n";
        } // Deeper scope cleared, back to subScopedMem

    } // Sub-scope cleared, back to original scopedMem

    // This should still use the original `scopedMem`, not any sub-scopes
    auto str5 = sstring(ctx, "Persistent Scoped String");
    std::cout << "smem after sub-scope: " << str5 << "\n";
    std::cout << "smem after deep sub-scope: " << deep<< "\n";

    return 0;
}
