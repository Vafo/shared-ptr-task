#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <atomic>
#include <cassert>

#include "checked_delete.hpp"
#include "raii.h"

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
private: /*explicit*/
    shared_ptr_impl(T *ptr): obj(ptr), ref_count(1) {}

    ~shared_ptr_impl() {
        using alloc_traits = std::allocator_traits< Allocator >; 
        // checked delete
        util::check_if_deletable(obj);        

        alloc_traits::destroy(allocator, obj);
        alloc_traits::deallocate(allocator, obj, 1);
    }

    void remove_owner() {
        int stored_val = ref_count.load();
        while(!ref_count.compare_exchange_weak(stored_val, stored_val - 1))
            ;

        if(stored_val - 1 == 0) {
            delete this; /*delete control block*/
        }
    }

    friend class shared_ptr<T, Allocator>; /*the only user*/

    T *obj;
    Allocator allocator;
    std::atomic<int> ref_count;
}; // class shared_ptr_impl

} // namespace detail


template<
    typename T,
    typename Allocator = std::allocator<T>
>
class shared_ptr {
public:
    shared_ptr(T *ptr = nullptr)
        : impl(nullptr)
    {
        if(ptr != nullptr)
            impl = new detail::shared_ptr_impl(ptr);
    }

    // Take ownership of obj
    // Argument is pointer to derived object
    template<typename D>
    shared_ptr(const D* obj)
        : impl(nullptr)
    {
        static_assert(std::is_base_of<T, D>::value);

        if(obj != nullptr)
            impl = new detail::shared_ptr_impl(obj);
    }

    // allocate copy of obj
    shared_ptr(const T& obj)
        : impl(nullptr)
    {
        using alloc_traits = std::allocator_traits< Allocator >; 
        // TODO: Reduce to 1 allocator call
        T *ptr = alloc_traits::allocate(allocator, 1);
        util::raii::ptr_holder obj_holder(ptr, allocator); /*if cb alloc fails*/

        impl = new detail::shared_ptr_impl(ptr); 
        util::raii::ptr_holder cb_holder(impl); /*if constructor fails*/

        alloc_traits::construct(allocator, ptr, obj);

        util::raii::relax(obj_holder, cb_holder);
    }

    // allocate copy of obj
    // Argument is derived object
    template<
        typename D,
        typename D_Allocator = std::allocator<D>
    >
    shared_ptr(
        const D& obj,
        D_Allocator d_allocator = D_Allocator()
    )
        : impl(nullptr)
    {
        static_assert(std::is_base_of<T, D>::value);
        using alloc_traits = std::allocator_traits< std::allocator<D> >; 

        // TODO: Exception safety anybody ?
        D *ptr = alloc_traits::allocate(d_allocator, 1);
        util::raii::ptr_holder obj_holder(ptr, d_allocator); /*if cb alloc fails*/
        
        impl = new detail::shared_ptr_impl(ptr);
        util::raii::ptr_holder cb_holder(impl); /*if constructor fails*/

        alloc_traits::construct(d_allocator, ptr);

        util::raii::relax(obj_holder, cb_holder);
    }
    
    shared_ptr(const shared_ptr& other)
        : impl(other.impl)
    {
        if(impl != nullptr) {
            ++impl->ref_count;
        }
    }

    shared_ptr&
    operator= (shared_ptr other) {
        swap(other); // copy and swap
        
        return *this;
    }

    ~shared_ptr() {
        release_ptr();
    }

    void release_ptr() {
        if(impl != nullptr) {
            impl->remove_owner();
            impl = nullptr;
        }
    }

    T&
    operator*() {
        assert(impl != nullptr);
        return *impl->obj;
    }

    const T&
    operator*() const {
        assert(impl != nullptr);
        return *impl->obj;
    }

    T*
    operator->() {
        assert(impl != nullptr);
        return impl->obj;
    }

    const T*
    operator->() const {
        assert(impl != nullptr);
        return impl->obj;
    }

    bool operator==(const shared_ptr &b) const {
        if(impl == nullptr || b.impl == nullptr) {
            if(impl == nullptr && b.impl == nullptr)
                return true;

            return false;
        }

        return impl->obj == b.impl->obj;
    }

    bool operator!=(const shared_ptr &b) const {
        return !(*this == b);
    }

    void swap(shared_ptr &other) {
        using std::swap;

        swap(impl, other.impl);
        swap(allocator, other.allocator);
    }

    friend void swap(shared_ptr &a, shared_ptr &b) {
        a.swap(b);
    }

private:

    detail::shared_ptr_impl<T, Allocator>* impl;
    Allocator allocator;
};

} // namespace memory

#endif