#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <atomic>
#include <cassert>

#include "checked_delete.hpp"
#include "scoped_ptr.hpp"

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


template<
    typename T,
    typename Allocator = std::allocator<T>
>
class shared_ptr {
public:
    shared_ptr(T *ptr = nullptr)
    : impl(nullptr) {
        if(ptr != nullptr) {
            using cb_type = detail::shared_ptr_impl<T, Allocator>;
            using cb_alloc_traits = std::allocator_traits< typename cb_type::allocator_type >;

            typename cb_type::allocator_type cb_alloc;

            impl = cb_alloc_traits::allocate(cb_alloc, 1);
            scoped_ptr cb_guard(impl); /*if constructor fails*/

            cb_alloc_traits::construct(cb_alloc, impl, ptr);

            scoped_relax(cb_guard);
        }
            
    }

    // Take ownership of obj
    // Argument is pointer to derived object
    template<typename D>
    shared_ptr(const D* obj)
    : impl(nullptr) {
        static_assert(std::is_base_of<T, D>::value);

        if(obj != nullptr) {
            using cb_type = detail::shared_ptr_impl<T, Allocator>;
            using cb_alloc_traits = std::allocator_traits< typename cb_type::allocator_type >;

            typename cb_type::allocator_type cb_alloc;

            impl = cb_alloc_traits::allocate(cb_alloc, 1);
            scoped_ptr cb_guard(impl); /*if constructor fails*/

            cb_alloc_traits::construct(cb_alloc, impl, obj);

            scoped_relax(cb_guard);
        }
    }

    shared_ptr(const shared_ptr<T>& other)
    : impl(other.impl) {
        if(impl != nullptr) {
            ++impl->ref_count;
        }
    }

    template<typename D>
    shared_ptr(const shared_ptr<D>& other)
    : impl(other.impl) {
        static_assert(std::is_base_of<T, D>::value);
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
        if(impl == nullptr || b.impl == nullptr)
            return impl == nullptr && b.impl == nullptr;

        return impl->obj == b.impl->obj;
    }

    bool operator!=(const shared_ptr &b) const {
        return !(*this == b);
    }

    void swap(shared_ptr &other) {
        using std::swap;

        swap(impl, other.impl);
    }

    friend void swap(shared_ptr &a, shared_ptr &b) {
        a.swap(b);
    }

private:

    void release_ptr() {
        if(impl != nullptr) {
            int stored_val = impl->ref_count.load();
            while(!impl->ref_count.compare_exchange_weak(stored_val, stored_val - 1))
                ;

            // TODO: CHANGE TO SUPPORT ALLOCATOR
            if(stored_val - 1 == 0) {
                using cb_type = detail::shared_ptr_impl<T, Allocator>;
                using cb_alloc_traits = std::allocator_traits< typename cb_type::allocator_type >;

                typename cb_type::allocator_type cb_alloc;

                cb_alloc_traits::destroy(cb_alloc, impl);
                cb_alloc_traits::deallocate(cb_alloc, impl, 1);
            }

            impl = nullptr;
        }
    }

    detail::shared_ptr_impl<T, Allocator>* impl;
};



template<typename T, typename Allocator = std::allocator<T>, typename ...Args>
shared_ptr<T, Allocator> make_shared(Args... args) {
    using obj_alloc_traits = std::allocator_traits< Allocator >;
    
    // TODO: Reduce to inplace cb & obj allocator call
    Allocator obj_alloc;
    T *ptr = obj_alloc_traits::allocate(obj_alloc, 1);
    scoped_ptr obj_guard(ptr, obj_alloc); /*if constructor fails*/

    obj_alloc_traits::construct(obj_alloc, ptr, args...);
    
    scoped_relax(obj_guard);
    return shared_ptr<T, Allocator>(ptr);
}

} // namespace memory

#endif