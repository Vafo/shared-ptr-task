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

    scoped_release(holder);
}

TEST_CASE("scoped_ptr: bad constructor", "[scoped_ptr]") {
    using allocator_t = std::allocator<bad_obj>;
    allocator_t bad_alloc;
    bad_obj* ptr = std::allocator_traits< allocator_t >::allocate(bad_alloc, 1);

    REQUIRE_THROWS(leak_safe_constructor<allocator_t>(ptr));
}

TEST_CASE("scoped_ptr: get & move", "[scoped_ptr]") {
    const int test_val = 123;
    scoped_ptr<int> int_ptr; // scoped ptr is just a unique_ptr
    REQUIRE(int_ptr.get() == nullptr);
    REQUIRE(int_ptr.release() == nullptr);

    int* int_raw_ptr = new int(test_val);
    scoped_ptr<int> ptr_holder(int_raw_ptr);
    REQUIRE(ptr_holder.get() == int_raw_ptr);
    
    {
        scoped_ptr<int> ptr_taker( std::move(ptr_holder) );
        REQUIRE(ptr_holder.get() == nullptr);
        REQUIRE(ptr_taker.get() == int_raw_ptr);
    } /*memory released*/

    REQUIRE(ptr_holder.get() == nullptr);
}

} // namespace memory