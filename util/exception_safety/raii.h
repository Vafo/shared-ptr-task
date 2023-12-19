namespace memory::util {

namespace raii {

template<
    typename T,
    typename Allocator = std::allocator<T>
>
class ptr_holder {
public:
    ptr_holder(T* in_ptr, const Allocator& in_alloc = Allocator())
        : ptr(in_ptr)
        , allocator(in_alloc)
    { }

    void relax() {
        ptr = nullptr;
    }

    ~ptr_holder() {
        using alloc_traits = std::allocator_traits< Allocator >; 

        check_if_deletable(ptr);
        if(ptr != nullptr)
            alloc_traits::deallocate(allocator, ptr, 1);
    }

private:
    T* ptr;
    Allocator allocator;
}; // class constructor


inline void relax() 
{ }

template<
    typename Arg1,
    typename ...Args
>
inline void relax(Arg1& arg1, Args&... args) {
    arg1.relax();
    relax(args...);
}



} // namespace raii

} // namespace memory::util