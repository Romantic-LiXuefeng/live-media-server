#ifndef DGLOBAL_HPP
#define DGLOBAL_HPP

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DFree(ptr) \
    do { if (ptr) { delete ptr; ptr = NULL;} } while (0)

#define DFreeArray(ptr) \
    do { if (ptr) { delete [] ptr; ptr = NULL;} } while (0)

#define DAssert(x) do { assert(x); } while (0)

#define DAutoFree(className, instance) \
impl__DAutoFree<className> _auto_free_##instance(&instance, false)

#define DAutoFreeArray(className, instance) \
impl__DAutoFree<className> _auto_free_array_##instance(&instance, true)

template<class T>
class impl__DAutoFree
{
private:
    T** ptr;
    bool is_array;
public:
    /**
     * auto delete the ptr.
     */
    impl__DAutoFree(T** p, bool array) {
        ptr = p;
        is_array = array;
    }

    virtual ~impl__DAutoFree() {
        if (ptr == NULL || *ptr == NULL) {
            return;
        }

        if (is_array) {
            delete[] *ptr;
        } else {
            delete *ptr;
        }

        *ptr = NULL;
    }
};

#define DMin(a, b) (((a) < (b))? (a) : (b))
#define DMax(a, b) (((a) < (b))? (b) : (a))

#define D_UNUSED(name) (void)name;

typedef uint8_t duint8;
typedef int8_t dint8;
typedef uint16_t duint16;
typedef int16_t dint16;
typedef uint32_t duint32;
typedef int32_t dint32;
typedef int64_t dint64;
typedef uint64_t duint64;

#endif // DGLOBAL_HPP
