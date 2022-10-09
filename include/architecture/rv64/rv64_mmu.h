// EPOS RISC-V 64 MMU Mediator Declarations

#ifndef __rv64_mmu_h
#define __rv64_mmu_h

// #define __mmu_common_only__
// #include <architecture/mmu.h>
// #undef __mmu_common_only__
// #include <system/memory_map.h>
#include <system/memory_map.h>
#include <utility/string.h>
#include <utility/list.h>
#include <utility/debug.h>
#include <architecture/cpu.h>
#include <architecture/mmu.h>

__BEGIN_SYS

class Sv39_MMU: public MMU_Common<9, 9, 12>
{
    friend class CPU;

private:
    // groupping list of frames
    typedef Grouping_List<Frame> List;

    static const unsigned long RAM_BASE = Memory_Map::RAM_BASE;
    static const unsigned long APP_LOW = Memory_Map::APP_LOW;
    static const unsigned long PHY_MEM = Memory_Map::PHY_MEM;
    static const unsigned long LEVELS = 3;

public:
    // Page Flags
    class Page_Flags
    {
    public:
        // RISC-V flags
        enum {
            V    = 1 << 0, // Valid (0 = not, 1 = valid)
            R    = 1 << 1, // Readable (0 = not, 1= yes)
            W    = 1 << 2, // Writable (0 = not, 1= yes)
            X    = 1 << 3, // Executable (0 = not, 1= yes)
            U    = 1 << 4, // User accessible (0 = not, 1= yes)
            G    = 1 << 5, // Global (mapped in multiple PTs)
            A    = 1 << 6, // Accessed (A == R?)
            D    = 1 << 7, // Dirty (D == W)
            APP  = (V | R | W | X),
            SYS  = (V | R | W | X),
            MASK = (1 << 8) - 1
        };

        // SATP
        enum {
            SV39    = 1UL << 63 // Sv39 MODE
        };

    public:
        Page_Flags() {}
        Page_Flags(unsigned int f) : _flags(f) {}
        Page_Flags(Flags f) : _flags(V | R | X |
                                     ((f & Flags::RW)  ? W  : 0) | // use W flag
                                     ((f & Flags::USR) ? U  : 0) | // use U flag (0 = supervisor, 1 = user)
                                     ((f & Flags::CWT) ? 0  : 0) | // cache mode
                                     ((f & Flags::CD)  ? 0  : 0)) {} // no cache

        operator unsigned int() const { return _flags; }
        friend Debug & operator<<(Debug & db, const Page_Flags & f) { db << hex << f._flags; return db; }

    private:
        unsigned int _flags;
    };

    // Page_Table
    class Page_Table
    {
    public:
        Page_Table() {}

        PT_Entry & operator[](unsigned int i) { return _entry[i]; }

        // change map to multi level
        void map(int from, int to, Page_Flags flags) {
            Phy_Addr * addr = alloc(to - from);
            if(addr)
                remap(addr, from, to, flags);
            else
                for( ; from < to; from++) {
                    // ptes[from] = ((alloc(1) >> 12) << 10) | flags ;
                    Log_Addr * pte = phy2log(&_entry[from]);
                    *pte = phy2pte(alloc(1), flags);
                }
        }

        void remap(Phy_Addr addr, int from, int to, Page_Flags flags) {
            addr = align_page(addr);
            for( ; from < to; from++) {
                Log_Addr * pte = phy2log(&_entry[from]);
                *pte = phy2pte(addr, flags);
                addr += sizeof(Page);
            }
        }   

        // system free
        void unmap(int from, int to) {
            for( ; from < to; from++) {
                free(_entry[from]);
                Log_Addr * tmp = phy2log(&_entry[from]);
                *tmp = 0;
            }
        }

    private:
        PT_Entry _entry[PT_ENTRIES];
    };

    // Chunk (for Segment)
    class Chunk
    {
    public:
        Chunk() {}

        Chunk(unsigned int bytes, Flags flags)
        : _from(0), _to(pages(bytes)), _pts(page_tables(_to - _from)), _flags(Page_Flags(flags)), _pt(calloc(_pts)) {
            _pt->map(_from, _to, _flags);
        }

        Chunk(Phy_Addr phy_addr, unsigned int bytes, Flags flags)
        : _from(0), _to(pages(bytes)), _pts(page_tables(_to - _from)), _flags(Page_Flags(flags)), _pt(calloc(_pts)) {
            _pt->remap(phy_addr, _from, _to, flags);
        }

        ~Chunk() {
            for( ; _from < _to; _from++)
                free((*static_cast<Page_Table *>(phy2log(_pt)))[_from]);
            free(_pt, _pts);
        }

        unsigned int pts() const { return _pts; }
        Page_Flags flags() const { return _flags; }
        Page_Table * pt() const { return _pt; }
        unsigned int size() const { return (_to - _from) * sizeof(Page); }
        Phy_Addr phy_address() const { return Phy_Addr(indexes((*_pt)[_from])); }
        int resize(unsigned int amount) { return 0; }

    private:
        unsigned int _from;
        unsigned int _to;
        unsigned int _pts;
        Page_Flags _flags;
        Page_Table * _pt;
    };

    // Page Directory
    typedef Page_Table Page_Directory;

    // Directory (for Address_Space)
    class Directory
    {
    public:
        Directory() : _pd(phy2log(calloc(1))), _free(true) {
            for(unsigned int i = directory(PHY_MEM); i < PD_ENTRIES; i++)
                (*_pd)[i] = (*_master)[i];
        }

        Directory(Page_Directory * pd) : _pd(pd), _free(false) {}

        ~Directory() { if(_free) free(_pd); }

        Phy_Addr pd() const { return _pd; }

        // MODE = 1000
        void activate() const { CPU::satp((1UL << 63) | reinterpret_cast<CPU::Reg64>(_pd) >> PAGE_SHIFT); }

        Log_Addr attach(const Chunk & chunk, unsigned int from = directory(APP_LOW)) {
            for(unsigned int i = from; i < PD_ENTRIES; i++)
                if(attach(i, chunk.pt(), chunk.pts(), chunk.flags()))
                    return i << DIRECTORY_SHIFT;
            return Log_Addr(false);
        }

        Log_Addr attach(const Chunk & chunk, Log_Addr addr) {
            unsigned int from = directory(addr);
            if(attach(from, chunk.pt(), chunk.pts(), chunk.flags()))
                return from << DIRECTORY_SHIFT;
            return Log_Addr(false);
        }

        void detach(const Chunk & chunk) {
            for(unsigned int i = 0; i < PD_ENTRIES; i++) {
                if(indexes((*_pd)[i]) == indexes(phy2pde(chunk.pt()))) {
                    detach(i, chunk.pt(), chunk.pts());
                    return;
                }
            }
            db<MMU>(WRN) << "MMU::Directory::detach(pt=" << chunk.pt() << ") failed!" << endl;
        }

        void detach(const Chunk & chunk, Log_Addr addr) {
            unsigned int from = directory(addr);
            if(indexes((*_pd)[from]) != indexes(chunk.pt())) {
                db<MMU>(WRN) << "MMU::Directory::detach(pt=" << chunk.pt() << ",addr=" << addr << ") failed!" << endl;
                return;
            }
            detach(from, chunk.pt(), chunk.pts());
        }

        Phy_Addr physical(Log_Addr addr) {
            Page_Table * pt = reinterpret_cast<Page_Table *>((void *)(*_pd)[directory(addr)]);
            return (*pt)[page(addr)] | offset(addr);
        }

    private:
        bool attach(unsigned int from, const Page_Table * pt, unsigned int n, Page_Flags flags) {
            for(unsigned int i = from; i < from + n; i++)
                if((*_pd)[i])
                    return false;
            for(unsigned int i = from; i < from + n; i++, pt++)
                (*_pd)[i] = phy2pde(Phy_Addr(pt));
            return true;
        }

        void detach(unsigned int from, const Page_Table * pt, unsigned int n) {
            for(unsigned int i = from; i < from + n; i++)
                (*_pd)[i] = 0;
        }

    private:
        Page_Directory * _pd;
        bool _free;
    };

public:
    Sv39_MMU() {}

    // Frame 4kB
    static Phy_Addr alloc(unsigned int frames = 1) {
        Phy_Addr phy(false);

        if(frames) {
            // Encontra primeiro frame que cabe
            List::Element * e = _free.search_decrementing(frames);
            if(e) {
                // Para onde o frame aponta + tamanho do frame? = 1?
                phy = e->object() + e->size();
                db<MMU>(TRC) << "MMU::alloc(frames=" << frames << ") => " << phy << endl;
            }
        }
        return phy;
    }

    static Phy_Addr calloc(unsigned int frames = 1) {
        Phy_Addr phy = alloc(frames);
        memset(phy2log(phy), 0, sizeof(Frame) * frames);
        return phy;
    }

    // n = ??
    static void free(Phy_Addr frame, unsigned int n = 1) {
        frame = indexes(frame); // clean frame

        db<MMU>(TRC) << "MMU::free(frame=" << frame << ",n=" << n << ")" << endl;

        if(frame && n) {
            List::Element * e = new (phy2log(frame)) List::Element(frame, n);
            List::Element * m1, * m2;
            _free.insert_merging(e, &m1, &m2);
        }
    }

    static unsigned int allocable() { return _free.head() ? _free.head()->size() : 0; }

    static Page_Directory * volatile current() { return static_cast<Page_Directory * volatile>(phy2log(CPU::satp() << 12)); }

    static Phy_Addr physical(Log_Addr addr) {
        Page_Directory * pd = current();
        Page_Table * pt = (*pd)[directory(addr)];
        return (*pt)[page(addr)] | offset(addr);
    }

    // PNN -> PTE
    static PT_Entry phy2pte(Phy_Addr frame, Page_Flags flags) { return (frame >> 2) | flags; }

    // PTE -> PNN
    static Phy_Addr pte2phy(PT_Entry entry) { return (entry & ~Page_Flags::MASK) << 2; }

    // PNN -> PDE (??? Page Directory Entry?)
    static PD_Entry phy2pde(Phy_Addr frame) { return (frame >> 2) | Page_Flags::V; }

    // PDE -> PNN
    static Phy_Addr pde2phy(PD_Entry entry) { return (entry & ~Page_Flags::MASK) << 2; }

    static void flush_tlb() {
        // sfence.vma??
    }
    static void flush_tlb(Log_Addr addr) {
        // sfence.vma??
    }

private:
    static void init();

    // PNN -> VPN
    static Log_Addr phy2log(Phy_Addr phy) { return Log_Addr((RAM_BASE == PHY_MEM) ? phy : (RAM_BASE > PHY_MEM) ? phy - (RAM_BASE - PHY_MEM) : phy + (PHY_MEM - RAM_BASE)); }

private:
    static List _free;
    static Page_Directory * _master;
};


class MMU: public Sv39_MMU {};

__END_SYS

#endif
