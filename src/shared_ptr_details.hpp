#ifndef SHARED_PTR_DETAILS_H
#define SHARED_PTR_DETAILS_H

#include "checked_delete.hpp"

namespace memory {

// Forward Declaration
template<
    typename T,
    typename Allocator
>
class shared_ptr;

namespace detail {

template<
    typename T,
    typename Allocator = std::allocator<T>
>
class shared_ptr_impl {
public: /*public for allocator*/

    typedef 
        std::allocator_traits<Allocator>::template rebind_alloc<shared_ptr_impl>
        allocator_type;

    shared_ptr_impl(T *ptr)
    : obj(ptr)
    , ref_count(1)
    {}

    ~shared_ptr_impl() {
        using alloc_traits = std::allocator_traits< Allocator >; 
        // checked delete (https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Checked_delete)
        checked_delete(obj);

        Allocator allocator;
        alloc_traits::destroy(allocator, obj);
        alloc_traits::deallocate(allocator, obj, 1);
    }

    friend class shared_ptr<T, Allocator>; /*the only user*/

private:
    T *obj;
    std::atomic<int> ref_count;
}; // class shared_ptr_impl

} // namespace detail

} // namespace memory

#endif