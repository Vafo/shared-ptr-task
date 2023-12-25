#include "catch2/catch_all.hpp"

#include <string>

#include "shared_ptr.hpp"
#include <vector>

namespace memory {

TEST_CASE("shared_ptr: construct shared_ptr", "[shared_ptr][normal]") {
    shared_ptr<int> iptr = make_shared<int>(5);

    REQUIRE(*iptr == 5);

    *iptr = 123;
    REQUIRE(*iptr == 123);
    // Creates new shared_ptr (BAD PRACTICE, delegate conversion to shared_ptr to make_shared )
    iptr = make_shared<int>(123);
    REQUIRE(*iptr == 123);
}

TEST_CASE("shared_ptr: copy ptr", "[shared_ptr][normal]") {
    // Conversion is not supported :-(
    shared_ptr<std::string> sptr = make_shared<std::string>("Hello World!");
    shared_ptr<std::string> sptr_copy = sptr;
    // They point to same object
    REQUIRE( sptr_copy == sptr );
    REQUIRE( *sptr_copy == *sptr );

    shared_ptr<std::string> sptr_other = make_shared<std::string>("Hello World!");

    REQUIRE( sptr_copy != sptr_other );
    REQUIRE( *sptr_copy == *sptr_other );
}

TEST_CASE("shared_ptr: vector of shared_ptr", "[shared_ptr][normal]") {
    const int iterations = 10;
    std::vector<shared_ptr<int>> ptrvec;
    for(int i = 0; i < iterations; ++i)
        // Conversion constructor is called
        ptrvec.push_back( make_shared<int>(i) );

    for(int i = 0; i < iterations; ++i)
        REQUIRE(*ptrvec[i] == i);
}

struct counter_t {
    static int counter;
    counter_t() {
        ++counter;
    }

    counter_t(const counter_t &other) {
        ++counter;
    }

    ~counter_t() {
        --counter;
    }
};

int counter_t::counter = 0;

TEST_CASE("shared_ptr: count const and dest of class", "[shared_ptr][normal]") {
    const int iterations = 10;

    SECTION("construct individual ptrs") {
        // check if precondition is valid (independent from code under test)
        assert(counter_t::counter == 0);
        for(int i = 0; i < iterations; ++i)
            shared_ptr<counter_t> count_ptr = make_shared<counter_t>();
        REQUIRE(counter_t::counter == 0);
    }

    SECTION("constuct ptrs in vector") {
        assert(counter_t::counter == 0);
        {
            std::vector<shared_ptr<counter_t>> count_vec;
            for(int i = 0; i < iterations; ++i)
                count_vec.push_back( make_shared<counter_t>() );
            REQUIRE(counter_t::counter == iterations);
        }
        REQUIRE(counter_t::counter == 0);
    }

}

TEST_CASE("shared_ptr: copy-and-swap test", "[shared_ptr][normal][assignment]") {
    const int iterations = 10;
    shared_ptr<std::string> ptr = make_shared<std::string>("Heya!");

    shared_ptr<std::string> ptr_copy;
    for(int i = 0; i < iterations; ++i) {
        ptr_copy = ptr;
        REQUIRE( ((ptr_copy == ptr) && (*ptr == *ptr_copy)) );
    }

}

struct empty_t;

TEST_CASE("shared_ptr: checked delete", "[shared_ptr][normal]") {
    // Uncomment to check (is there other way around to check it automatically?)
    // shared_ptr<empty_t> ptr;
}

TEST_CASE("shared_ptr: ptr reusage", "[shared_ptr]") {
    const int test_val = 10;
    shared_ptr<int> keka = make_shared<int>(test_val);

    {
        shared_ptr<int> other_ptr = keka;

        REQUIRE(*other_ptr == test_val);
    }

    REQUIRE(*keka == test_val);
    
    REQUIRE(keka != shared_ptr<int>());

    keka = shared_ptr<int>();
    REQUIRE(keka == shared_ptr<int>());
}


class base_t {
public:
    static const int val = 0;

    virtual int get_val()
    { return val; }
    
    virtual ~base_t()
    {}
};

class derived_t: public base_t {
public:
    static const int val = 1;

    virtual int get_val()
    { return val; }

    virtual ~derived_t()
    {}
};

class unrelated_t {
    int empty;
};

TEST_CASE("shared_ptr: ptr to derived object ", "[shared_ptr]") {
    shared_ptr<base_t> based_ptr = make_shared<base_t>();
    shared_ptr<derived_t> derived_ptr = make_shared<derived_t>();

    REQUIRE(based_ptr->get_val() == base_t::val);
    based_ptr = derived_ptr;
    REQUIRE(based_ptr->get_val() == derived_t::val);

    SECTION("unrelated object ptr copy & assign") {
        shared_ptr<unrelated_t> unrel_ptr = make_shared<unrelated_t>();

        // unrel_ptr = based_ptr;
        // shared_ptr<unrelated_t> unrel_tmp(based_ptr);
        // shared_ptr<base_t> based_tmp(unrel_ptr);
    }

}


} // namespace postfix::util