#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <atomic>
#include <cassert>

#include "shared_ptr_details.hpp"
#include "scoped_ptr.hpp"

namespace memory {

template<typename T>
class shared_ptr {
public:
    shared_ptr()
    : m_refcount()
    {}

    template<
        typename D,
        typename = std::enable_if< std::is_convertible<D*, T*>::value >::type
    >
    shared_ptr(const shared_ptr<D>& other)
    : m_refcount(other.m_refcount)
    , m_ptr(other.m_ptr)
    {}

    template<
        typename D,
        typename = std::enable_if< std::is_convertible<D*, T*>::value >::type
    >
    shared_ptr(D* ptr)
    : m_refcount(ptr)
    , m_ptr(ptr)
    {}

    shared_ptr&
    operator= (shared_ptr other) {
        swap(other);
        return *this;
    }

    T&
    operator*() {
        return *m_ptr;
    }

    const T&
    operator*() const {
        return *m_ptr;
    }

    T*
    operator->() {
        return m_ptr;
    }

    const T*
    operator->() const {
        return m_ptr;
    }

    // template<typename D>
    bool operator==(const shared_ptr& other) const {
        return m_refcount == other.m_refcount;
    }

    bool operator!=(const shared_ptr& other) const {
        return !(*this == other);
    }

    template<
        typename D,
        typename = std::enable_if< std::is_convertible<D*, T*>::value >::type
    >
    void swap(shared_ptr<D>& other) {
        using std::swap;

        swap(m_refcount, other.m_refcount);
        swap(m_ptr, other.m_ptr);
    }

    template<
        typename D,
        typename = std::enable_if< std::is_convertible<D*, T*>::value >::type
    >
    friend void swap(shared_ptr<T>& a, shared_ptr<D>& b) {
        a.swap(b);
    }

    template<typename D>
    friend class shared_ptr;

private:

    template<typename Allocator, typename ...Args>
    shared_ptr(detail::sp_cb_inplace_tag_t, const Allocator& obj_alloc, Args... args)
    : m_refcount(detail::sp_cb_inplace_tag, obj_alloc, m_ptr, args...)
    {}

    template<typename D, typename Allocator, typename ...Args>
    friend shared_ptr<D> allocate_shared(const Allocator& obj_alloc, Args... args);

    T* m_ptr;
    detail::sp_refcount m_refcount;
}; // class shared_ptr

template<typename T, typename Allocator, typename ...Args>
shared_ptr<T> allocate_shared(const Allocator& obj_alloc, Args... args) {
    return shared_ptr<T>(detail::sp_cb_inplace_tag, obj_alloc, args...);
}

template<typename T, typename ...Args>
shared_ptr<T> make_shared(Args... args) {
    return memory::allocate_shared<T>(
        std::allocator<T>{} /*used for template deduction*/,
        args...
    );
}


} // namespace memory

#endif