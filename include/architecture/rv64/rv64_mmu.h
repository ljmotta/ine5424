// EPOS RISC-V 64 MMU Mediator Declarations

#ifndef __rv64_mmu_h
#define __rv64_mmu_h

#include <system/memory_map.h>
#include <utility/string.h>
#include <utility/list.h>
#include <utility/debug.h>
#include <architecture/cpu.h>
#include <architecture/mmu.h>

__BEGIN_SYS


// use 2 Mb pages;
class Sv39_MMU: public MMU_Common<9, 9, 21>
{
    friend class CPU;

private:
    // groupping list of frames
    typedef Grouping_List<Frame> List;

    static const unsigned long RAM_BASE = Memory_Map::RAM_BASE;
    static const unsigned long APP_LOW = Memory_Map::APP_LOW;

public:
    // Page Flags
    class RV64_Flags
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
            A    = 1 << 6, // Accessed (A == R)
            D    = 1 << 7, // Dirty (D == W)
            APP  = (V | R | W | X | U),
            SYS  = (V | R | W | X),
            LEAF = (R | W | X),
            MASK = (1 << 8) - 1,
        };

    public:
        RV64_Flags() {}
        RV64_Flags(unsigned long f) : _flags(f) {}
        RV64_Flags(Flags f) : _flags(V | R | X |
                                     ((f & Flags::RW)  ? W  : 0) | // use W flag
                                     ((f & Flags::USR) ? U  : 0) | // use U flag (0 = supervisor, 1 = user)
                                     ((f & Flags::CWT) ? 0  : 0) | // cache mode
                                     ((f & Flags::CD)  ? 0  : 0)) {} // no cache

        operator unsigned long() const { return _flags; }
        friend Debug & operator<<(Debug & db, const RV64_Flags & f) { db << hex << f._flags; return db; }

    private:
        unsigned long _flags;
    };

    // Page_Table
    class Page_Table
    {
    public:
        Page_Table() {}

        PT_Entry & operator[](unsigned long i) { return _pte[i]; }

        void map(long from, long to, RV64_Flags flags) {
            Phy_Addr * addr = alloc(to - from);
            if(addr)
                remap(addr, from, to, flags);
            else
                for( ; from < to; from++) {
                    _pte[from] = pnn2pte(alloc(1), flags);
                }
        }

        void remap(Phy_Addr addr, long from, long to, RV64_Flags flags) {
            addr = align_page(addr);
            for( ; from < to; from++) {
                _pte[from] = pnn2pte(addr, flags);
                addr += sizeof(Page);
            }
        }

    private:
        PT_Entry _pte[PT_ENTRIES];
    };

    // Chunk (for Segment)
    class Chunk
    {
    public:
        Chunk() {}

        Chunk(unsigned long bytes, Flags flags)
        : _from(0), _to(pages(bytes)), _pts(page_tables(_to - _from)), _flags(RV64_Flags(flags)), _pt(calloc(_pts)) {
            _pt->map(_from, _to, _flags);
        }

        Chunk(Phy_Addr phy_addr, unsigned long bytes, Flags flags)
        : _from(0), _to(pages(bytes)), _pts(page_tables(_to - _from)), _flags(RV64_Flags(flags)), _pt(calloc(_pts)) {
            _pt->remap(phy_addr, _from, _to, flags);
        }

        ~Chunk() {
            for( ; _from < _to; _from++)
                free((*static_cast<Page_Table *>(phy2log(_pt)))[_from]);
            free(_pt, _pts);
        }

        // quantas tabelas de paginas foram necessárias para alocar o chunk
        unsigned long pts() const { return _pts; }
        RV64_Flags flags() const { return _flags; }
        // tabela que o chunk ta mapeado
        Page_Table * pt() const { return _pt; }
        unsigned long size() const { return (_to - _from) * sizeof(Page); }
        Phy_Addr phy_address() const { return Phy_Addr(indexes((*_pt)[_from])); }
        long resize(unsigned long amount) { return 0; }

    private:
        unsigned long _from;
        unsigned long _to;
        unsigned long _pts;
        RV64_Flags _flags;
        Page_Table * _pt;
    };

    // Page Directory
    typedef Page_Table Page_Directory;

    // Directory (for Address_Space)
    class Directory
    {
    public:
        Directory() : _pd(calloc(1)), _free(true) {
            for(unsigned long i = 0; i < PD_ENTRIES; i++)
                (*_pd)[i] = (*_master)[i];
        }

        Directory(Page_Directory * pd) : _pd(pd), _free(false) {}

        ~Directory() { if(_free) free(_pd); }

        Phy_Addr pd() const { return _pd; }

        // MODE = 1000 = Sv39
        void activate() const { CPU::satp((1UL << 63) | reinterpret_cast<CPU::Reg64>(_pd) >> PAGE_SHIFT); }

        // attach chunk
        Log_Addr attach(const Chunk & chunk, unsigned long from = directory(APP_LOW)) {
            for(unsigned long i = from; i < PD_ENTRIES; i++)
                if(attach(i, chunk.pt(), chunk.pts(), chunk.flags()))
                    return i << DIRECTORY_SHIFT;
            return Log_Addr(false);
        }

        Log_Addr attach(const Chunk & chunk, Log_Addr addr) {
            unsigned long from = directory(addr);
            if(attach(from, chunk.pt(), chunk.pts(), chunk.flags()))
                return from << DIRECTORY_SHIFT;
            return Log_Addr(false);
        }

        void detach(const Chunk & chunk) {
            for(unsigned long i = 0; i < PD_ENTRIES; i++) {
                if(indexes((*_pd)[i]) == indexes(pnn2pde(chunk.pt()))) {
                    detach(i, chunk.pt(), chunk.pts());
                    return;
                }
            }
            db<MMU>(WRN) << "MMU::Directory::detach(pt=" << chunk.pt() << ") failed!" << endl;
        }

        void detach(const Chunk & chunk, Log_Addr addr) {
            unsigned long from = directory(addr);
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
        bool attach(unsigned long from, const Page_Table * pt, unsigned long n, RV64_Flags flags) {
            for(unsigned long i = from; i < from + n; i++)
                if((*_pd)[i])
                    return false;
            for(unsigned long i = from; i < from + n; i++, pt++)
                (*_pd)[i] = pnn2pde(Phy_Addr(pt));
            return true;
        }

        void detach(unsigned long from, const Page_Table * pt, unsigned long n) {
            for(unsigned long i = from; i < from + n; i++)
                (*_pd)[i] = 0;
        }

    private:
        Page_Directory * _pd;
        bool _free;
    };

public:
    Sv39_MMU() {}

    static Phy_Addr alloc(unsigned long frames = 1) {
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

    static Phy_Addr calloc(unsigned long frames = 1) {
        Phy_Addr phy = alloc(frames);
        memset(phy2log(phy), 0, sizeof(Frame) * frames);
        return phy;
    }

    // n = qtt of frames
    static void free(Phy_Addr frame, unsigned long n = 1) {
        frame = indexes(frame); // clean frame

        db<MMU>(TRC) << "MMU::free(frame=" << frame << ",n=" << n << ")" << endl;

        if(frame && n) {
            List::Element * e = new (phy2log(frame)) List::Element(frame, n);
            List::Element * m1, * m2;
            _free.insert_merging(e, &m1, &m2);
        }
    }

    static unsigned long allocable() { return _free.head() ? _free.head()->size() : 0; }

    // returns current PNN on SATP
    static Page_Directory * volatile current() { return static_cast<Page_Directory * volatile>(phy2log(CPU::satp() << 12)); }

    static Phy_Addr physical(Log_Addr addr) {
        Page_Directory * pd = current();
        Page_Table * pt = (*pd)[directory(addr)];
        return (*pt)[page(addr)] | offset(addr);
    }


    // PNN -> PTE
    static PT_Entry pnn2pte(Phy_Addr frame, RV64_Flags flags) { return (frame >> 2) | flags; }
    // PNN -> PDE (Page Directory Entry = pte, but with X | R | W = 0)
    static PD_Entry pnn2pde(Phy_Addr frame) { return (frame >> 2) | RV64_Flags::V; }

    // only necessary for multihart
    static void flush_tlb() {}
    static void flush_tlb(Log_Addr addr) {}

private:
    static void init();
    // PHY está no mesmo endereço que a RAM
    static Log_Addr phy2log(const Phy_Addr & phy) { return phy; }

private:
    static List _free;
    static Page_Directory *_master;
};

class MMU: public Sv39_MMU {};

__END_SYS

#endif
