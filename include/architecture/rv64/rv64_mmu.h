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

// Levels with same size;
template<unsigned int LEVELS, unsigned int PAGE_BITS, unsigned int OFFSET_BITS>
class RV64_MMU_Common {
protected:
    RV64_MMU_Common() {}

protected:
    // CPU imports
    typedef CPU::Log_Addr Log_Addr;
    typedef CPU::Phy_Addr Phy_Addr;

    // Page constants
    static const unsigned long PAGE_SHIFT = OFFSET_BITS;
    static const unsigned long PAGE_SIZE = 1 << PAGE_SHIFT;
    // directory shift is where the directory starts; = bit 21; for sv39
    static const unsigned long DIRECTORY_SHIFT = OFFSET_BITS + PAGE_BITS; // 21
    static const unsigned long PAGE_TABLE_MASK = (1UL << (DIRECTORY_SHIFT) - 1); // 1..20
    static const unsigned long LOGICAL_ADDRESS_SIZE = ((PAGE_BITS * LEVELS) + OFFSET_BITS); // 39 for Sv39
    static const unsigned long LOGICAL_ADDRESS_MASK = (1UL << LOGICAL_ADDRESS_SIZE) - 1; // 0 to 38 for Sv39

public:
    // Memory page
    typedef unsigned char Page[PAGE_SIZE];
    typedef Page Frame;

    // Page_Table and Page_Directory entries
    typedef Phy_Addr PT_Entry;
    typedef Phy_Addr PD_Entry;

    // Page Flags
    class Flags
    {
    public:
        enum {
            VALID       = 1 << 0, // Valid
            READ        = 1 << 1, // Readable
            WRITE       = 1 << 2, // Writable
            EXECUTE     = 1 << 3, // Executable
            USER        = 1 << 4, // Access Control (0=supervisor, 1=user)
            GLOBAL      = 1 << 5, // All address spaces
            ACCESSED    = 1 << 6, // Accessed, set to 1
            DIRTY       = 1 << 7, // Dirty, set to 1
            CT          = 1 << 8, // Contiguous (reserved for use by supervisor RSW)
            MIO         = 1 << 9, // I/O (reserved for use by supervisor RSW)

            SYS  = (VALID | READ | WRITE | EXECUTE | ACCESSED | DIRTY),
            APP  = (VALID | READ | WRITE | EXECUTE | USER),
            APPC = (VALID | READ | EXECUTE | USER),
            APPD = (VALID | READ | WRITE | USER),
            IO   = (SYS | MIO),

            // requires ACCESSED and DIRTY for non leafs?
            PD  = (VALID | ACCESSED | DIRTY),

            MASK = ((0x3ffUL << 54) | (0xfffUL))
        };

    public:
        Flags() {}
        Flags(const Flags & f) : _flags(f._flags) {}
        Flags(unsigned long f) : _flags(f) {}

        operator unsigned long() const { return _flags; }

        friend Debug & operator<<(Debug & db, Flags f) { db << hex << f._flags << dec; return db; }

    private:
        unsigned long _flags;
    };

    // Number of entries in Page_Table and Page_Directory 9 = 512
    static const unsigned  PT_ENTRIES = 1UL << PAGE_BITS; // 512
    static const unsigned  PT_MASK = PT_ENTRIES - 1; // 0..511
    static const unsigned  DIRECTORY_LV1_SHIFT = DIRECTORY_SHIFT; // 21
    static const unsigned  DIRECTORY_LV2_SHIFT = DIRECTORY_SHIFT + PAGE_BITS; // 30

public:
    // qtt of pages in n bytes
    constexpr static unsigned long pages(unsigned long bytes) { return (bytes + sizeof(Page) - 1) / sizeof(Page); }
    // qtt of tables for the qtt of pages lv1!
    constexpr static unsigned long page_tables(unsigned long pages) { return sizeof(Page) > sizeof(long) ? (pages + PT_ENTRIES - 1) / PT_ENTRIES : 0; }

    constexpr static unsigned long offset(const Log_Addr & addr) { return addr & (sizeof(Page) - 1); }
    // should signal extend last bit
    constexpr static unsigned long indexes(const Log_Addr & addr) {
        unsigned long addr_msb = addr >> (LOGICAL_ADDRESS_SIZE - 1); // 38
        if (addr_msb) {
            return (addr | ~LOGICAL_ADDRESS_MASK) & ~(sizeof(Page) - 1);
        }
        return (addr & LOGICAL_ADDRESS_MASK) & ~(sizeof(Page) - 1);
    }

    constexpr static unsigned long page(const Log_Addr & addr) { return (addr >> (PAGE_SHIFT)) & PT_MASK; }
    // returns the entire directory, all levels;
    constexpr static unsigned long directory(const Log_Addr & addr) { return (addr >> DIRECTORY_SHIFT) & ((1UL << (PAGE_BITS * LEVELS - 1)) - 1); }
    constexpr static unsigned long directory_lv1(const unsigned long directory) { return directory & PT_MASK; }
    constexpr static unsigned long directory_lv2(const unsigned long directory) { return (directory >> PAGE_BITS) & PT_MASK; }
    constexpr static unsigned long index(const Log_Addr & base, const Log_Addr & addr) { return (addr - base) >> PAGE_SHIFT; }

    constexpr static Log_Addr align_page(const Log_Addr & addr) { return (addr + sizeof(Page) - 1) & ~(sizeof(Page) - 1); }
    constexpr static Log_Addr align_directory(const Log_Addr & addr) { return (addr + PT_ENTRIES * sizeof(Page) - 1) &  ~(PT_ENTRIES * sizeof(Page) - 1); }
    constexpr static Log_Addr directory_bits(const Log_Addr & addr) { return (addr & ~((1 << PAGE_BITS) - 1)); }
};


// use 3 levels of 2^9 entries with 2^12 page size;
class MMU: public RV64_MMU_Common<3, 9, 12>
{
    friend class CPU;

private:
    // A list of frames = pages (4kb)
    typedef Grouping_List<Frame> List;

    static const unsigned long long RAM_BASE = Memory_Map::RAM_BASE;
    static const unsigned long long PHY_MEM = Memory_Map::PHY_MEM;
    static const unsigned long long APP_LOW = Memory_Map::APP_LOW;

public:
    // Page_Table
    class Page_Table
    {
    public:
        Page_Table() {}

        PT_Entry & operator[](unsigned long i) { return _pte[i]; }

        // já foi feito a alocaçao com calloc, porque alloc de novo?
        void map(unsigned long from, unsigned long to, Flags flags) {
            Phy_Addr * addr = alloc(to - from);
            if(addr) {
                remap(addr, from, to, flags);
            }
            else {
                for( ; from < to; from++) {
                    _pte[from] = phy2pte(alloc(1), flags);
                }
            }
        }

        void remap(Phy_Addr addr, unsigned long from, unsigned long to, Flags flags) {
            addr = align_page(addr);
            for(unsigned int i = from; i < to; i++) {
                _pte[i] = phy2pte(addr, flags);
                addr += sizeof(Page);
            }
        }

        void remap_d(Phy_Addr addr, unsigned long from, unsigned long to, Flags flags) {
            addr = align_page(addr);
            for(unsigned int i = from; i < to; i++) {
                _pte[i] = phy2pte(addr, flags);
                addr += 0x201000UL;
            }
        }

    private:
        PT_Entry _pte[PT_ENTRIES];
    };

    // Chunk (for Segment) -> é mapeado um pedaço físico em lógico. 
    class Chunk
    {
    public:
        Chunk() {}

        // cria um chunk com quantidade de bytes e flags.
        // _to = quantidade de páginas
        // _pts = quantidade total de páginas de 4kb.
        // _pt = endereço físico da alocação, um array de page tables; é alocado a quantidade de páginas necessária
        // 
        Chunk(unsigned long bytes, Flags flags)
        : _from(0), _to(pages(bytes)), _pts(page_tables(_to - _from)), _flags(Flags(flags)), _pt(calloc(_pts)) {
            _pt->map(_from, _to, _flags);
        }

        Chunk(Phy_Addr phy_addr, unsigned long bytes, Flags flags)
        : _from(0), _to(pages(bytes)), _pts(page_tables(_to - _from)), _flags(Flags(flags)), _pt(calloc(_pts)) {
            _pt->remap(phy_addr, _from, _to, flags);
        }

        ~Chunk() {
            for( ; _from < _to; _from++) {
                free((*static_cast<Page_Table *>(phy2log(_pt)))[_from]);
            }
            free(_pt, _pts);
        }

        // 1 table lv0 = 512 * 4kb = 2mb,
        // 2 tables lv0 = 4mb..
        // 512 tables lv0 = 1gb = 1 table lv 1
        // quantas tabelas de paginas foram necessárias para alocar o chunk
        unsigned long pts() const { return _pts; }
        Flags flags() const { return _flags; }
        // tabela que o chunk ta mapeado
        Page_Table * pt() const { return _pt; }
        unsigned long size() const { return (_to - _from) * sizeof(Page); }
        Phy_Addr phy_address() const { return Phy_Addr(indexes((*_pt)[_from])); }
        long resize(unsigned long amount) { return 0; }

    private:
        unsigned long _from;
        unsigned long _to;
        unsigned long _pts;
        Flags _flags;
        Page_Table * _pt;
    };

    // Page Directory
    typedef Page_Table Page_Directory;

    // Directory (for Address_Space) -> adiciona um chunck ao address space!
    class Directory
    {
    public:
        Directory() : _pd(calloc(1)), _free(true) {
            for(unsigned long i = 0; i < PT_ENTRIES; i++)
                (*_pd)[i] = (*_master)[i];
        }

        Directory(Page_Directory * pd) : _pd(pd), _free(false) {}

        ~Directory() { if(_free) free(_pd); }

        Phy_Addr pd() const { return _pd; }

        // MODE = 1000 = Sv39
        void activate() const { CPU::satp((1UL << 63) | reinterpret_cast<CPU::Reg>(_pd) >> PAGE_SHIFT); }

        // attach a chunk to page directory, by default starts at app_low
        // directory returns the entire directory, for sv39 returns lv2 and lv1
        Log_Addr attach(const Chunk & chunk, unsigned long dir = directory(APP_LOW)) {
            for(unsigned long i = dir; i < PT_ENTRIES; i++) { // 0..511
                // attach(entire directory, chunk page table, qtt of page tables, flags)
                if(attach(i, chunk.pt(), chunk.pts(), chunk.flags())) {
                    return i << DIRECTORY_SHIFT;
                }
            }

            return Log_Addr(false);
        }

        Log_Addr attach(const Chunk & chunk, Log_Addr addr) {
            unsigned long from = directory(addr);
            if(attach(from, chunk.pt(), chunk.pts(), chunk.flags())) {
                return from << DIRECTORY_SHIFT;
            }
            return Log_Addr(false);
        }

        void detach(const Chunk & chunk) {
            for(unsigned long i = 0; i < PT_ENTRIES; i++) {
                if(indexes((*_pd)[i]) == indexes(phy2pde(chunk.pt()))) {
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
        // attach a page table to pd... n = qtt of pages necessary
        // need to recalc n. if n > PAGE_BITS * PAGE_BITS, need to check lv2, alloc bigger than 1gb;
        bool attach(unsigned long directory, const Page_Table * pt, unsigned long n, Flags flags) {
            unsigned long lv1 = directory_lv1(directory);
            unsigned long lv2 = directory_lv2(directory);
            // need to check in the next lv2 directory
            if (lv1 + n >= PT_ENTRIES) {
                // lv2 directory is the last one?
                if (lv2 + 1 >= PT_ENTRIES) { // >= 512
                    return false;
                }
                // check the next lv2, if it's taken can't attach.
                if((*_pd)[lv2 + 1]) {
                    // for n = 3; pt_entries = 512; lv1 = 510
                    // i < 2
                    for(unsigned long i = 0; i < n - ((PT_ENTRIES - 1) - lv1); i++) {
                        if((*_pd)[lv2 + 1][i]) {
                            return false;
                        }
                    }
                    return false;
                }
            }
            // lv2 exists, check if lv1 is taken.
            if((*_pd)[lv2]) {
                // lv1 = 510, n = 3; should iterate until 511;
                unsigned int to = lv1 + n;
                if (to >= PT_ENTRIES) {
                    to = PT_ENTRIES - 1;
                }
                for(unsigned long i = lv1; i < to; i++) {
                    if((*_pd)[lv2][i]) {
                        return false;
                    }
                }
                return false;
            }

            // start allocating from lv1;
            unsigned int to = lv1 + n;
            if (to >= PT_ENTRIES) {
                to = PT_ENTRIES - 1;
            }
            // pd lv1 doens't exist?
            if (!((*_pd)[lv2])) {
                // create pd lv1.. correct?
                (*_pd)[lv2] = phy2pde(Phy_Addr(directory << DIRECTORY_SHIFT));
            }
            for(unsigned long i = lv1; i < to; i++, pt++) {
                (*_pd)[lv2][i] = phy2pde(Phy_Addr(pt));
            }
            // if requires next lv2 to attach...
            unsigned int remaining = n - ((PT_ENTRIES - 1) - lv1);
            if (remaining > 0) {
                for(unsigned long i = lv1; i < remaining; i++, pt++) {
                    (*_pd)[lv2 + 1][i] = phy2pde(Phy_Addr(pt));
                }
            }            
            return true;
        }

        // never puts 0 in lv2;
        void detach(unsigned long directory, const Page_Table * pt, unsigned long n) {
            unsigned long lv1 = directory_lv1(directory);
            unsigned long lv2 = directory_lv2(directory);
            unsigned int to = lv1 + n;
            if (to >= PT_ENTRIES) {
                to = PT_ENTRIES - 1;
            }
            for(unsigned long i = lv1; i < to + n; i++) {
                (*_pd)[lv2][i] = 0;
            }
            unsigned int remaining = n - ((PT_ENTRIES - 1) - lv1);
            if (remaining > 0) {
                for(unsigned long i = lv1; i < remaining; i++) {
                    (*_pd)[lv2 + 1][i] = 0;
                }
            } 
        }

    private:
        Page_Directory * _pd;
        bool _free;
    };

public:
    MMU() {}

    // _free is a list of frames.
    static Phy_Addr alloc(unsigned long frames = 1) {
        Phy_Addr phy(false);

        if(frames) {
            List::Element * e = _free.search_decrementing(frames);
            if(e) {
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
    // starting the system, we free the entire ram;
    // the pagging system is only in virtual memory...
    static void free(Phy_Addr frame, unsigned long n = 1) {
        frame = indexes(frame); // frame without offset

        db<MMU>(TRC) << "MMU::free(frame=" << frame << ",n=" << n << ")" << endl;

        if(frame && n) {
            Log_Addr place = phy2log(frame);
            db<MMU>(TRC) << "MMU::free(place=" << place << ")" << endl;
            List::Element * e = new (place) List::Element(frame, n);
            db<MMU>(TRC) << "MMU::free(e=" << e << ")" << endl;
            List::Element * m1, * m2;
            _free.insert_merging(e, &m1, &m2);
        }
    }

    static unsigned long allocable() { return _free.head() ? _free.head()->size() : 0; }

    // returns current PNN on SATP
    // static Page_Directory * volatile current() { return static_cast<Page_Directory * volatile>(phy2log(CPU::satp() << 12)); }
    static Page_Directory * volatile current() { return _master; }

    static Phy_Addr physical(Log_Addr addr) {
        Page_Directory * pd = current();
        Page_Table * pt = (*pd)[directory(addr)];
        return (*pt)[page(addr)] | offset(addr);
    }

    // PNN -> PTE
    static PT_Entry phy2pte(Phy_Addr frame, Flags flags) {
        // if (flags == Flags::SYS) {
        //     db<MMU>(TRC) << "MMU::phy2pte(frame" << frame << endl;
        // }
        return ((frame & ~Flags::MASK) >> 2) | flags;
    }
    // PNN -> PDE (X | R | W = 0)
    static PD_Entry phy2pde(Phy_Addr frame) {
        return ((frame & ~Flags::MASK) >> 2) | Flags::VALID;
    }

    // only necessary for multihart
    static void flush_tlb() { ASM("sfence.vma"); }
    static void flush_tlb(Log_Addr addr) {}

private:
    static void init();
    static Log_Addr phy2log(Phy_Addr phy) {
        db<MMU>(TRC) << "MMU::phy2log(phy=" << phy << ", RAM_BASE=" << RAM_BASE << ", PHY_MEM=" << PHY_MEM <<  ")" << endl;
        if (RAM_BASE < PHY_MEM) {
            return Log_Addr(phy + (PHY_MEM - RAM_BASE));
        }
        if (RAM_BASE > PHY_MEM) {
            return Log_Addr(phy - (RAM_BASE - PHY_MEM));
        }
        // RAM_BASE == PHY_MEM
        return Log_Addr(phy);
    }

private:
    static List _free;
    static Page_Directory *_master;
};

// class MMU: public Sv39_MMU {};

__END_SYS

#endif
