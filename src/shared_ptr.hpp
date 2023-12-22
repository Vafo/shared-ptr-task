#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <atomic>
#include <cassert>

#include "shared_ptr_details.hpp"
#include "scoped_ptr.hpp"

namespace memory {

template<
    typename T,
    typename Allocator = std::allocator<T>
>
class shared_ptr {
public:

    shared_ptr()
    : impl(nullptr)
    { }

    // Take ownership of obj
    // Argument is pointer to derived object
    shared_ptr(T* obj)
    : impl(nullptr) {
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

    shared_ptr(const shared_ptr& other)
    : impl(other.impl) {
        if(impl != nullptr) {
            ++impl->ref_count;
        }
    }

    ~shared_ptr() {
        release_ptr();    
    }

    shared_ptr&
    operator= (shared_ptr other) {
        swap(other); // copy and swap
        
        return *this;
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

    bool operator==(const shared_ptr& b) const {
        if(impl == nullptr || b.impl == nullptr)
            return impl == nullptr && b.impl == nullptr;

        return impl->obj == b.impl->obj;
    }

    bool operator!=(const shared_ptr& b) const {
        return !(*this == b);
    }

    void swap(shared_ptr& other) {
        using std::swap;

        swap(impl, other.impl);
    }

    friend void swap(shared_ptr& a, shared_ptr& b) {
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


namespace new_impl {

class sp_cb_base {
public:

    sp_cb_base()
    : ref_count(1)
    {}

    virtual ~sp_cb_base() {}

    void incr_ref()
    { ++ref_count; }

    void decr_ref() {
            int stored_val = ref_count.load();
            while(!ref_count.compare_exchange_weak(stored_val, stored_val - 1))
                ;

            // TODO: CHANGE TO SUPPORT ALLOCATOR
            if(stored_val - 1 == 0) {
                destroy();
            }
    }

    virtual void* get_ptr() = 0;

protected:
    virtual void destroy() = 0;

    sp_cb_base(const sp_cb_base& other) = delete;
    sp_cb_base& operator=(const sp_cb_base& other) = delete;

private:
    std::atomic<int> ref_count;
}; // class sp_cb_base

template<typename T, typename Allocator>
class sp_cb_separate: public sp_cb_base {
public:

    typedef 
        std::allocator_traits<Allocator>::template rebind_alloc<sp_cb_separate>
        allocator_type;

    sp_cb_separate(T* ptr, const Allocator& alloc_ref)
    : sp_cb_base()
    , m_obj(ptr)
    , m_alloc_ref(alloc_ref)
    { }

    virtual ~sp_cb_separate() {
        using obj_alloc_traits = std::allocator_traits<Allocator>;
        Allocator obj_alloc(m_alloc_ref);

        obj_alloc_traits::destroy(obj_alloc, m_obj);
        obj_alloc_traits::deallocate(obj_alloc, m_obj, 1);
    }

    virtual void* get_ptr()
    { return reinterpret_cast<void*>(m_obj); };

    virtual void destroy() { 
        using cb_alloc_traits = std::allocator_traits<allocator_type>;
        allocator_type cb_alloc;

        cb_alloc_traits::destroy(cb_alloc, this);
        cb_alloc_traits::deallocate(cb_alloc, this, 1);
    };


    sp_cb_separate(const sp_cb_separate& other) = delete;
    sp_cb_separate& operator=(const sp_cb_separate& other) = delete;

private:
    T* m_obj;
    const Allocator& m_alloc_ref;
}; // class sp_cb_separate

template<typename T>
class sp_cb_inplace {


    virtual ~sp_cb_inplace() {}
};

struct sp_cb_inplace_tag_t {};
constexpr sp_cb_inplace_tag_t sp_cb_inplace_tag;

class sp_refcount {
public:
    sp_refcount()
    : m_cb_ptr(nullptr)
    {}

    sp_refcount(const sp_refcount& other)
    : m_cb_ptr(other.m_cb_ptr) {
        if(m_cb_ptr != nullptr)
            m_cb_ptr->incr_ref();
    }

    template<typename T>
    explicit
    sp_refcount(T* sep_ptr)
    : sp_refcount(sep_ptr, std::allocator<T>())
    {}

    template<typename T, typename Allocator>
    explicit
    sp_refcount(T* sep_ptr, Allocator alloc)
    : m_cb_ptr(nullptr) {
        using cb_t = sp_cb_separate<T, Allocator>;
        using cb_alloc_t = typename cb_t::allocator_type;
        using alloc_traits = std::allocator_traits< cb_alloc_t >;
        cb_alloc_t cb_alloc;

        cb_t* cb_ptr = alloc_traits::allocate(cb_alloc, 1);
        // TODO: exception safety
        alloc_traits::construct(cb_alloc, cb_ptr, /*args*/ sep_ptr, alloc);

        m_cb_ptr = cb_ptr;
    }

    ~sp_refcount() {
        if(m_cb_ptr != nullptr)
            m_cb_ptr->decr_ref();
    }

    sp_refcount& operator=(sp_refcount other) { 
        swap(other);
        return *this;
    }

    template<typename T>
    T* get()
    { 
        assert(m_cb_ptr != nullptr);
        return reinterpret_cast<T*>(m_cb_ptr->get_ptr());
    }

    bool operator==(const sp_refcount& other) const
    { return m_cb_ptr == other.m_cb_ptr; }

    void swap(sp_refcount& other) {
        using std::swap;

        swap(m_cb_ptr, other.m_cb_ptr);
    }

    friend void swap(sp_refcount& a, sp_refcount& b)
    { a.swap(b); }

    sp_cb_base* m_cb_ptr;

}; // class sp_refcount

template<typename T>
class shared_ptr {
public:
    shared_ptr()
    : m_refcount()
    {}

    // TODO: Is assignable?
    shared_ptr(const shared_ptr& other)
    : m_refcount(other.m_refcount)
    {}

    template<
        typename D,
        typename = std::enable_if< std::is_convertible<D*, T*>::value >::type
    >
    shared_ptr(const shared_ptr<D>& other)
    : m_refcount(other.m_refcount)
    {}

    template<
        typename D,
        typename = std::enable_if< std::is_convertible<D*, T*>::value >::type
    >
    shared_ptr(D* ptr)
    : m_refcount(ptr)
    {}

    // TODO: Is assignable?
    template<
        typename D,
        typename = std::enable_if< std::is_convertible<D*, T*>::value >::type
    >
    shared_ptr&
    operator= (shared_ptr<D>& other) {
        shared_ptr(other).swap(*this);
        
        return *this;
    }

    T&
    operator*() {
        return *m_refcount.get<T>();
    }

    const T&
    operator*() const {
        return *m_refcount.get<T>();
    }

    T*
    operator->() {
        return m_refcount.get<T>();
    }

    const T*
    operator->() const {
        return m_refcount.get<T>();
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
    sp_refcount m_refcount;

}; // class shared_ptr

// template<typename T, typename ...Args>
// shared_ptr<T> make_shared(Args... args) {

// }


} // namespace new_impl

} // namespace memory

#endif