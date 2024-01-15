#ifndef SHARED_PTR_DETAILS_H
#define SHARED_PTR_DETAILS_H

#include <atomic>

#include "util/scoped_ptr.hpp"
#include "util/checked_delete.hpp"

namespace memory::detail {

class sp_cb_base {
private:
    std::atomic<int> ref_count;

private:
    sp_cb_base(const sp_cb_base& other) = delete;
    sp_cb_base& operator=(const sp_cb_base& other) = delete;

public:
    sp_cb_base()
        : ref_count(1)
    {}

    virtual ~sp_cb_base() {}

public:
    void* get_ptr()
    { return do_get_ptr(); }

    void incr_ref()
    { ++ref_count; }

    void decr_ref() {
        int stored_val = ref_count.load();
        while(!ref_count.compare_exchange_weak(stored_val, stored_val - 1))
            ;

        if(stored_val - 1 == 0) {
            destroy();
        }
    }

private:
    virtual void* do_get_ptr() = 0;
    virtual void destroy() = 0;

}; // class sp_cb_base

template<typename T, typename Allocator>
class sp_cb_separate: public sp_cb_base {
public:
    typedef 
        std::allocator_traits<Allocator>::template rebind_alloc<sp_cb_separate>
        allocator_type;

private:
    T* m_obj;
    const Allocator& m_alloc_type_ref;

public:
    sp_cb_separate(
        T* ptr,
        const Allocator& alloc_ref /*used for template deduction*/
    )
        : m_obj(ptr)
        , m_alloc_type_ref(alloc_ref)
    {}

    virtual
    ~sp_cb_separate() {
        using obj_alloc_traits = std::allocator_traits<Allocator>;
        Allocator obj_alloc;

        obj_alloc_traits::destroy(obj_alloc, m_obj);
        obj_alloc_traits::deallocate(obj_alloc, m_obj, 1);
    }

private:
    virtual void* do_get_ptr()
    { return reinterpret_cast<void*>(m_obj); };

    virtual void destroy() { 
        using cb_alloc_traits = std::allocator_traits<allocator_type>;
        allocator_type cb_alloc;

        cb_alloc_traits::destroy(cb_alloc, this);
        cb_alloc_traits::deallocate(cb_alloc, this, 1);
    };

}; // class sp_cb_separate

template<typename T, typename Allocator>
class sp_cb_inplace: public sp_cb_base {
public:
    typedef 
        std::allocator_traits<Allocator>::template rebind_alloc<sp_cb_inplace>
        allocator_type;

private:
    std::aligned_storage_t<sizeof(T), alignof(T)> m_obj_mem;
    const Allocator& m_alloc_type_ref;

public:
    template<typename ...Args>
    sp_cb_inplace(
        const Allocator& alloc_ref, /*used for template deduction*/
        Args... args
    )
        : m_alloc_type_ref(alloc_ref)
    { 
        using obj_alloc_traits = std::allocator_traits<Allocator>;
        Allocator obj_alloc;
        
        obj_alloc_traits::construct(obj_alloc, get_obj_ptr(), args...);
    }

    virtual
    ~sp_cb_inplace() {
        using obj_alloc_traits = std::allocator_traits<Allocator>;
        Allocator obj_alloc;

        obj_alloc_traits::destroy(obj_alloc, get_obj_ptr());
    }

public:
    virtual void* do_get_ptr()
    { return reinterpret_cast<void*>(get_obj_ptr()); };

    virtual void destroy() { 
        using cb_alloc_traits = std::allocator_traits<allocator_type>;
        allocator_type cb_alloc; 

        cb_alloc_traits::destroy(cb_alloc, this);
        cb_alloc_traits::deallocate(cb_alloc, this, 1);
    };

private:
    T* get_obj_ptr()
    { return reinterpret_cast<T*>(&m_obj_mem); }

}; // class sp_cb_inplace


// Helper Tag for inplace control block
struct sp_cb_inplace_tag_t {};
constexpr sp_cb_inplace_tag_t sp_cb_inplace_tag;

class sp_refcount {
private:
    sp_cb_base* m_cb_ptr;

public:
    sp_refcount()
        : m_cb_ptr(nullptr)
    {}

    sp_refcount(const sp_refcount& other)
        : m_cb_ptr(other.m_cb_ptr)
    {
        if(m_cb_ptr != nullptr)
            m_cb_ptr->incr_ref();
    }

    sp_refcount& operator=(sp_refcount other) { 
        swap(other);
        return *this;
    }

    ~sp_refcount() {
        if(m_cb_ptr != nullptr)
            m_cb_ptr->decr_ref();
    }

private:
    template<typename cb_T, typename ...Args>
    static auto S_init(Args... args) {
        using cb_alloc_t = typename cb_T::allocator_type;
        using alloc_traits = std::allocator_traits< cb_alloc_t >;
        cb_alloc_t cb_alloc;

        scoped_ptr<cb_T, cb_alloc_t> alloc_guard(
            alloc_traits::allocate(cb_alloc, 1)
        );

        alloc_traits::construct(cb_alloc, alloc_guard.get(), args...);

        return alloc_guard.release();
    }

public:
    // Control Block & Object are separate
    template<typename T>
    explicit
    sp_refcount(T* sep_ptr)
        : sp_refcount(
            std::allocator<T>(),/*used for template deduction*/
            sep_ptr)
    {}

    template<typename T, typename Allocator>
    sp_refcount(const Allocator& alloc_ref, T* sep_ptr)
        : m_cb_ptr( S_init<sp_cb_separate<T, Allocator>>(sep_ptr, alloc_ref) )
    {}

    // Control Block & Object are allocated together
    template<typename T, typename Allocator, typename ...Args>
    sp_refcount(
        sp_cb_inplace_tag_t, const Allocator& obj_alloc_ref,
        /*out*/T*& out_ptr, Args... args
    )
        : m_cb_ptr( S_init<sp_cb_inplace<T, Allocator>>(obj_alloc_ref, args...) )
    { out_ptr = reinterpret_cast<T*>(m_cb_ptr->get_ptr()); }

public:
    template<typename T>
    T* get() { 
        assert(m_cb_ptr != nullptr);
        return reinterpret_cast<T*>(m_cb_ptr->get_ptr());
    }

    bool operator==(const sp_refcount& other) const
    { return m_cb_ptr == other.m_cb_ptr; }

public:
    void swap(sp_refcount& other) {
        using std::swap;

        swap(m_cb_ptr, other.m_cb_ptr);
    }

    friend void swap(sp_refcount& a, sp_refcount& b)
    { a.swap(b); }

}; // class sp_refcount

} // namespace memory::detail

#endif
