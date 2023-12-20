#ifndef CHECKED_DELETE_H
#define CHECKED_DELETE_H

namespace memory {

template<typename T>
void checked_delete(T *ptr) {
    typedef char type_must_be_complete[ sizeof(T) ? 1 : -1 ];
    (void) sizeof(type_must_be_complete);
}

} // namespace memory

#endif