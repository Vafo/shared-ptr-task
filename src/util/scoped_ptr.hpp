#ifndef SCOPED_PTR_H
#define SCOPED_PTR_H

#include <memory> // std::allocator

#include "checked_delete.hpp"

namespace memory {

template<
    typename T,
    typename Allocator = std::allocator<T>
>
class scoped_ptr {
private:
    T* m_ptr;

private:
    scoped_ptr(const scoped_ptr& other) = delete;
    scoped_ptr& operator=(const scoped_ptr& other) = delete;

public:
    scoped_ptr(T* in_ptr = nullptr)
        : m_ptr(in_ptr)
    {}

    scoped_ptr(scoped_ptr&& other)
        : m_ptr(other.release())
    {}

    ~scoped_ptr() {
        using alloc_traits = std::allocator_traits< Allocator >; 

        checked_delete(m_ptr);
       
        if(m_ptr != nullptr) {
            Allocator alloc;
            alloc_traits::deallocate(alloc, m_ptr, 1);
        }
    }

public:
    T* get() {
        return m_ptr;
    }

    T* release() {
        T* tmp = m_ptr;
        m_ptr = nullptr;
        return tmp;
    }

}; // class scoped_ptr


inline void scoped_release() 
{ }

template<
    typename Arg1,
    typename ...Args
>
inline void scoped_release(Arg1& arg1, Args&... args) {
    arg1.release();
    scoped_release(args...);
}

} // namespace memory

#endif