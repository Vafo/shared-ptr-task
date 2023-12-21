namespace memory {

template<
    typename T,
    typename Allocator = std::allocator<T>
>
class scoped_ptr {
public:
    scoped_ptr(T* in_ptr, const Allocator& in_alloc = Allocator())
        : m_ptr(in_ptr)
        , allocator(in_alloc)
    { }

    scoped_ptr(const scoped_ptr& other) = delete;

    scoped_ptr(scoped_ptr&& other)
    : m_ptr(other.m_ptr) {
        other.relax();
    }

    ~scoped_ptr() {
        using alloc_traits = std::allocator_traits< Allocator >; 

        checked_delete(m_ptr);
        if(m_ptr != nullptr)
            alloc_traits::deallocate(allocator, m_ptr, 1);
    }
    
    scoped_ptr& operator=(const scoped_ptr& other) = delete;

    void relax() {
        m_ptr = nullptr;
    }

private:
    T* m_ptr;
    Allocator allocator;
}; // class constructor


inline void scoped_relax() 
{ }

template<
    typename Arg1,
    typename ...Args
>
inline void scoped_relax(Arg1& arg1, Args&... args) {
    arg1.relax();
    scoped_relax(args...);
}


} // namespace memory