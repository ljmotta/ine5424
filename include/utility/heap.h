// EPOS Heap Utility Declarations

#ifndef __heap_h
#define __heap_h

#include <utility/debug.h>
#include <utility/list.h>
#include <utility/spin.h>

__BEGIN_UTIL

// Heap
class Heap: private Grouping_List<char, Traits<System>::HEAP_STRATEGY>
{
protected:
    static const bool typed = Traits<System>::multiheap;
    static const unsigned int heap_strategy = Traits<System>::HEAP_STRATEGY;

public:
    using Grouping_List_Base<char>::empty;
    using Grouping_List_Base<char>::size;
    using Grouping_List_Base<char>::grouped_size;

    Heap() {
        db<Init, Heaps>(TRC) << "Heap() => " << this << endl;
    }

    Heap(void * addr, unsigned long bytes) {
        db<Init, Heaps>(TRC) << "Heap(addr=" << addr << ",bytes=" << bytes << ") => " << this << endl;

        free(addr, bytes);
    }

    void * alloc(unsigned long bytes) {
        db<Heaps>(TRC) << "Heap::alloc(this=" << this << ",bytes=" << bytes;

        if(!bytes)
            return 0;

        if(!Traits<CPU>::unaligned_memory_access)
            while((bytes % sizeof(void *)))
                ++bytes;

        if(typed)
            bytes += sizeof(void *);  // add room for heap pointer
        bytes += sizeof(long);        // add room for size
        if(bytes < sizeof(Element))
            bytes = sizeof(Element);

        Element * e = search_decrementing(bytes);
        if(!e) {
            out_of_memory(bytes);
            return 0;
        }

        // e->object is the address of the element that was shrank
        // the new allocated bytes are in the begining of the element.
        // in the BOTTOM_UP config the address is (e->object() - bytes)
        long *addr = heap_strategy  == Traits_Tokens::TOP_DOWN ?
            reinterpret_cast<long *>(e->object() + e->size()) :
            reinterpret_cast<long *>(e->object() - bytes);
        if(typed)
            *addr++ = reinterpret_cast<long>(this);
        *addr++ = bytes;

        db<Heaps>(TRC) << ") => " << reinterpret_cast<void *>(addr) << endl;

        return addr;
    }

    void free(void * ptr, unsigned long bytes) {
        db<Heaps>(TRC) << "Heap::free(this=" << this << ",ptr=" << ptr << ",bytes=" << bytes << ")" << endl;

        if(ptr && (bytes >= sizeof(Element))) {
            // place the link data in the end of the element if it's BOTTOM_UP
            char * link_data_location = heap_strategy == Traits_Tokens::TOP_DOWN ?
                reinterpret_cast<char *>(ptr) :
                reinterpret_cast<char *>(ptr) + bytes - sizeof(Element);
            Element * e = new (link_data_location) Element(reinterpret_cast<char *>(ptr), bytes);
            Element * m1, * m2;
            insert_merging(e, &m1, &m2);
        }
    }

    static void typed_free(void * ptr) {
        long * addr = reinterpret_cast<long *>(ptr);
        unsigned long bytes = *--addr;
        Heap * heap = reinterpret_cast<Heap *>(*--addr);
        heap->free(addr, bytes);
    }

    static void untyped_free(Heap * heap, void * ptr) {
        long * addr = reinterpret_cast<long *>(ptr);
        unsigned long bytes = *--addr;
        heap->free(addr, bytes);
    }

private:
    void out_of_memory(unsigned long bytes);
};

__END_UTIL

#endif
