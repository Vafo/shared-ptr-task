#include "catch2/catch_all.hpp"

#include "shared_ptr.hpp"

namespace memory {

struct bad_obj {
    bad_obj() {
        throw std::runtime_error("bad object");
    }

    int big_data;
};

template<typename Allocator>
void leak_safe_constructor(bad_obj* ptr) {
    using allocator_t = std::allocator<bad_obj>;
    Allocator bad_alloc;

    scoped_ptr holder(ptr);
    std::allocator_traits< allocator_t >::construct(bad_alloc, ptr); // throws

    scoped_relax(holder);
}

TEST_CASE("raii::ptr_holder: bad constructor", "[ptr_holder]") {
    using allocator_t = std::allocator<bad_obj>;
    allocator_t bad_alloc;
    bad_obj* ptr = std::allocator_traits< allocator_t >::allocate(bad_alloc, 1);

    REQUIRE_THROWS(leak_safe_constructor<allocator_t>(ptr));
}

} // namespace memory