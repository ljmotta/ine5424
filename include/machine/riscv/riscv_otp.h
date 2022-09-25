// EPOS Timer RISC-V OTP Mediator Declarations

#ifndef __riscv_otp_h
#define __riscv_otp_h

#include <architecture/cpu.h>
#include <machine/otp.h>
#include <system/memory_map.h>
#include <utility/string.h>
#include <time.h>

#define EINVAL 22

__BEGIN_SYS
    // write val into address
    // static void writel(unsigned int val, volatile unsigned int *addr) {
    //     ASM("fence w, w" : : : "memory"); // wmb
    // }

    // static Reg readl(void *addr) {
    //     ASM("fence r, r" : : : "memory"); // rmb
    // }

class SiFive_OTP
{
private:
    typedef CPU::Reg32 Reg32;

    static const unsigned int TPW_DELAY = Traits<OTP>::TPW_DELAY;
    static const unsigned int TPWI_DELAY = Traits<OTP>::TPWI_DELAY;
    static const unsigned int TASP_DELAY = Traits<OTP>::TASP_DELAY;
    static const unsigned int TCD_DELAY = Traits<OTP>::TCD_DELAY;
    static const unsigned int TKL_DELAY = Traits<OTP>::TKL_DELAY;
    static const unsigned int TMS_DELAY = Traits<OTP>:: TMS_DELAY;

public:
    // OTP registers offset from OTP_Base
    enum {
	    PA      = 0x00,     /* Address input */
	    PAIO    = 0x04,   /* Program address input */
	    PAS     = 0x08,    /* Program redundancy cell selection input */
	    PCE     = 0x0C,    /* OTP Macro enable input */
	    PCLK    = 0x10,   /* Clock input */
	    PDIN    = 0x14,   /* Write data input */
	    PDOUT   = 0x18,  /* Read data output */
	    PDSTB   = 0x1C,  /* Deep standby mode enable input (active low) */
	    PPROG   = 0x20,  /* Program mode enable input */
	    PTC     = 0x24,    /* Test column enable input */
	    PTM     = 0x28,    /* Test mode enable input */
	    PTM_REP = 0x2C,/* Repair function test mode enable input */
	    PTR     = 0x30,    /* Test row enable input */
	    PTRIM   = 0x34,  /* Repair function enable input */
	    PWE     = 0x38    /* Write enable input (defines program cycle) */
    };

    enum : int {
        BYTES_PER_FUSE		        = 4, // 8 * 4
        TOTAL_FUSES                 = 3840 // = 16kB -1kb / 4
    };
    
    enum {
        PA_RESET_VAL		        = 0x00,
        PAS_RESET_VAL		        = 0x00,
        PAIO_RESET_VAL		        = 0x00,
        PDIN_RESET_VAL		        = 0x00,
        PTM_RESET_VAL		        = 0x00,
        PCLK_ENABLE_VAL			    = 0x01,
        PCLK_DISABLE_VAL		    = 0x00,
        PWE_WRITE_ENABLE		    = 0x01,
        PWE_WRITE_DISABLE		    = 0x00,
        PTM_FUSE_PROGRAM_VAL	    = 0x10,
        PCE_ENABLE_INPUT		    = 0x01,
        PCE_DISABLE_INPUT		    = 0x00,
        PPROG_ENABLE_INPUT		    = 0x01,
        PPROG_DISABLE_INPUT		    = 0x00,
        PTRIM_ENABLE_INPUT		    = 0x01,
        PTRIM_DISABLE_INPUT		    = 0x00,
        PDSTB_DEEP_STANDBY_ENABLE	= 0x01,
        PDSTB_DEEP_STANDBY_DISABLE	= 0x00
    };

public:
    SiFive_OTP() {}

    int read(int offset, void *buf, int size) {
        /* Check if offset and size are multiple of BYTES_PER_FUSE */
        if ((size % BYTES_PER_FUSE) || (offset % BYTES_PER_FUSE)) {
            return -EINVAL + 1;
        }

        unsigned int fuseidx = offset / BYTES_PER_FUSE;
        unsigned int fusecount = size / BYTES_PER_FUSE;

        /* check bounds */
        if (offset < 0 || size < 0)
            return -EINVAL + 2;
        if (fuseidx >= TOTAL_FUSES)
            return -EINVAL + 3;
        if ((fuseidx + fusecount) > TOTAL_FUSES)
            return -EINVAL + 4;

        unsigned int fusebuf[fusecount];

        /* init OTP */
        reg(SiFive_OTP::PDSTB) = PDSTB_DEEP_STANDBY_ENABLE;
        reg(SiFive_OTP::PTRIM) = PTRIM_ENABLE_INPUT;
        reg(SiFive_OTP::PCE) = PCE_ENABLE_INPUT;

        /* read all requested fuses */
        for (unsigned int i = 0; i < fusecount; i++, fuseidx++) {
            reg(SiFive_OTP::PA) = fuseidx;

            /* cycle clock to read */
            reg(SiFive_OTP::PCLK) = PCLK_ENABLE_VAL;
            // EPOS Timer
            Delay tcd(TCD_DELAY);
            reg(SiFive_OTP::PCLK) = PCLK_DISABLE_VAL;
            // EPOS Timer
            Delay tkl(TKL_DELAY);

            /* read the value */
            fusebuf[i] = reg(SiFive_OTP::PDOUT);
        }

        /* shut down */
        reg(SiFive_OTP::PCE) = PCE_DISABLE_INPUT;
        reg(SiFive_OTP::PTRIM) = PTRIM_DISABLE_INPUT;
        reg(SiFive_OTP::PDSTB) = PDSTB_DEEP_STANDBY_DISABLE;

        /* copy out */
        memcpy(buf, fusebuf, size);

        return size;
    }

    int write(int offset, const void *buf, int size) {
        /* Check if offset and size are multiple of BYTES_PER_FUSE */
        if ((size % BYTES_PER_FUSE) || (offset % BYTES_PER_FUSE)) {
            return -EINVAL + 1;
        }

        int fuseidx = offset / BYTES_PER_FUSE;
        int fusecount = size / BYTES_PER_FUSE;
        unsigned int *write_buf = (unsigned int *)buf;
        unsigned int write_data;
        int i, pas, bit;

        /* check bounds */
        if (offset < 0 || size < 0)
            return -EINVAL + 2;
        if (fuseidx >= TOTAL_FUSES)
            return -EINVAL + 3;
        if ((fuseidx + fusecount) > TOTAL_FUSES)
            return -EINVAL + 4;

        /* init OTP */
        reg(SiFive_OTP::PDSTB) = PDSTB_DEEP_STANDBY_ENABLE;
        reg(SiFive_OTP::PTRIM) = PTRIM_ENABLE_INPUT;

        /* reset registers */
        reg(SiFive_OTP::PCLK) = PCLK_DISABLE_VAL;
        reg(SiFive_OTP::PA) = PA_RESET_VAL;
        reg(SiFive_OTP::PAS) = PAS_RESET_VAL;
        reg(SiFive_OTP::PAIO) = PAIO_RESET_VAL;
        reg(SiFive_OTP::PDIN) = PDIN_RESET_VAL;
        reg(SiFive_OTP::PWE) = PWE_WRITE_DISABLE;
        reg(SiFive_OTP::PTM) = PTM_FUSE_PROGRAM_VAL;

        // EPOS Timer
        Delay tms(TMS_DELAY);

        reg(SiFive_OTP::PCE) = PCE_ENABLE_INPUT;
        reg(SiFive_OTP::PPROG) = PPROG_ENABLE_INPUT;

        /* write all requested fuses */
        for (i = 0; i < fusecount; i++, fuseidx++) {
            reg(SiFive_OTP::PA) = fuseidx;
            write_data = *(write_buf++);

            for (pas = 0; pas < 2; pas++) {
                reg(SiFive_OTP::PAS) = pas;

                for (bit = 0; bit < 32; bit++) {
                    reg(SiFive_OTP::PAIO) = bit;
                    reg(SiFive_OTP::PDIN) = (write_data >> bit) & 1;
                    // EPOS Timer
                    Delay tasp(TASP_DELAY);

                    reg(SiFive_OTP::PWE) = PWE_WRITE_ENABLE;

                    // EPOS Timer
                    Delay tpw(TPW_DELAY);
                    reg(SiFive_OTP::PWE) = PWE_WRITE_DISABLE;

                    // EPOS Timer
                    Delay tpwi(TPWI_DELAY);
                }
            }

            reg(SiFive_OTP::PAS) = PAS_RESET_VAL;
        }

        /* shut down */
        reg(SiFive_OTP::PWE) = PWE_WRITE_DISABLE;
        reg(SiFive_OTP::PPROG) = PPROG_DISABLE_INPUT;
        reg(SiFive_OTP::PCE) = PCE_DISABLE_INPUT;
        reg(SiFive_OTP::PTM) = PTM_RESET_VAL;
        reg(SiFive_OTP::PTRIM) = PTRIM_DISABLE_INPUT;
        reg(SiFive_OTP::PDSTB) = PDSTB_DEEP_STANDBY_DISABLE;

        return size;
    }

private:
    static volatile CPU::Reg32 & reg(unsigned int o) { return reinterpret_cast<volatile CPU::Reg32 *>(Memory_Map::OTP_BASE)[o / sizeof(CPU::Reg32)]; }
};

class OTP: private OTP_Common, private SiFive_OTP {};

__END_SYS

#endif
