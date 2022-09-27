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

class OTP
{
private:
    typedef CPU::Reg32 Reg32;

    volatile unsigned int * _otp_base = (volatile unsigned int *)Memory_Map::OTP_BASE;
    static const int BYTES_PER_FUSE = Traits<OTP>::BYTES_PER_FUSE;
    static const int TOTAL_FUSES = Traits<OTP>::TOTAL_FUSES;

public:
    // RISC-V OTP registers offset from OTP_Base
    enum RegOTP {
	    PA      = 0x00, /* Address input */
	    PAIO    = 0x04, /* Program address input */
	    PAS     = 0x08, /* Program redundancy cell selection input */
	    PCE     = 0x0C, /* OTP Macro enable input */
	    PCLK    = 0x10, /* Clock input */
	    PDIN    = 0x14, /* Write data input */
	    PDOUT   = 0x18, /* Read data output */
	    PDSTB   = 0x1C, /* Deep standby mode enable input (active low) */
	    PPROG   = 0x20, /* Program mode enable input */
	    PTC     = 0x24, /* Test column enable input */
	    PTM     = 0x28, /* Test mode enable input */
	    PTM_REP = 0x2C, /* Repair function test mode enable input */
	    PTR     = 0x30, /* Test row enable input */
	    PTRIM   = 0x34, /* Repair function enable input */
	    PWE     = 0x38  /* Write enable input (defines program cycle) */
    };

    enum : unsigned int {
        TPW_DELAY  = 20, // program pulse width delay
        TPWI_DELAY = 5,  // program pulse interval delay
        TASP_DELAY = 1,  // program address setup delay
        TCD_DELAY  = 40, // read data access delay
        TKL_DELAY  = 10, // clock pulse low delay
        TMS_DELAY  = 1   // ptm mode setup delay
    };
    
    // helper values
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
    OTP() {}

    /// @brief read the otp data and copy to buf
    /// @param offset in bits
    /// @param buf pointer to a buffer
    /// @param size in bits
    /// @return if it succeds returns the size, if it fails an error code is returned
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
        writel(RegOTP::PDSTB, PDSTB_DEEP_STANDBY_ENABLE);
        writel(RegOTP::PTRIM, PTRIM_ENABLE_INPUT);
        writel(RegOTP::PCE, PCE_ENABLE_INPUT);

        /* read all requested fuses */
        for (unsigned int i = 0; i < fusecount; i++, fuseidx++) {
            writel(RegOTP::PA, fuseidx);

            /* cycle clock to read */
            writel(RegOTP::PCLK, PCLK_ENABLE_VAL);

            Delay tcd(TCD_DELAY); // 40us

            writel(RegOTP::PCLK, PCLK_DISABLE_VAL);

            Delay tkl(TKL_DELAY); // 10us

            /* read the value */
            fusebuf[i] = readl(RegOTP::PDOUT);
        }

        /* shut down */
        writel(RegOTP::PCE, PCE_DISABLE_INPUT);
        writel(RegOTP::PTRIM, PTRIM_DISABLE_INPUT);
        writel(RegOTP::PDSTB, PDSTB_DEEP_STANDBY_DISABLE);

        /* copy out */
        memcpy(buf, fusebuf, size);

        return size;
    }

    /// @brief writes to the otp memory
    /// @param offset in bits
    /// @param buf pointer to a buffer
    /// @param size in bits
    /// @return if it succeds returns the size, if it fails an error code is returned
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
        writel(RegOTP::PDSTB, PDSTB_DEEP_STANDBY_ENABLE);
        writel(RegOTP::PTRIM, PTRIM_ENABLE_INPUT);

        /* reset registers */
        writel(RegOTP::PCLK, PCLK_DISABLE_VAL);
        writel(RegOTP::PA, PA_RESET_VAL);
        writel(RegOTP::PAS, PAS_RESET_VAL);
        writel(RegOTP::PAIO, PAIO_RESET_VAL);
        writel(RegOTP::PDIN, PDIN_RESET_VAL);
        writel(RegOTP::PWE, PWE_WRITE_DISABLE);
        writel(RegOTP::PTM, PTM_FUSE_PROGRAM_VAL);

        // EPOS Timer
        Delay tms(TMS_DELAY); // 1 us

        writel(RegOTP::PCE, PCE_ENABLE_INPUT);
        writel(RegOTP::PPROG, PPROG_ENABLE_INPUT);

        /* write all requested fuses */
        for (i = 0; i < fusecount; i++, fuseidx++) {
            writel(RegOTP::PA, fuseidx);
            write_data = *(write_buf++);

            for (pas = 0; pas < 2; pas++) {
                writel(RegOTP::PAS, pas);

                for (bit = 0; bit < 32; bit++) {
                    writel(RegOTP::PAIO, bit);
                    unsigned int value = (write_data >> bit) & 1;
                    writel(RegOTP::PDIN, value);

                    Delay tasp(TASP_DELAY); // 1u

                    writel(RegOTP::PWE, PWE_WRITE_ENABLE);

                    Delay tpw(TPW_DELAY); // 20us

                    writel(RegOTP::PWE, PWE_WRITE_DISABLE);

                    Delay tpwi(TPWI_DELAY); // 5u
                }
            }

            writel(RegOTP::PAS, PAS_RESET_VAL);
        }

        /* shut down */
        writel(RegOTP::PWE, PWE_WRITE_DISABLE);
        writel(RegOTP::PPROG, PPROG_DISABLE_INPUT);
        writel(RegOTP::PCE, PCE_DISABLE_INPUT);
        writel(RegOTP::PTM, PTM_RESET_VAL);
        writel(RegOTP::PTRIM, PTRIM_DISABLE_INPUT);
        writel(RegOTP::PDSTB, PDSTB_DEEP_STANDBY_DISABLE);

        // memset2(&_otp_base, 0x1, sizeof(int) * TOTAL_FUSES);
        return size;
    }

private:
    static volatile CPU::Reg32 & reg(unsigned int o) { return reinterpret_cast<volatile CPU::Reg32 *>(Memory_Map::OTP_BASE)[o / sizeof(CPU::Reg32)]; }

    // write val into address
    static void writel(unsigned int addr, int val) {
        ASM("fence ow, ow" : : : "memory"); // wmb
        reg(addr) = val;
    }

    static volatile CPU::Reg32 readl(unsigned int addr) {
        ASM("fence ir, ir" : : : "memory"); // rmb
        return reg(addr);
    }


//     static inline void *memset2(void *s, int c, size_t n)
// {
//     size_t i;
//     unsigned char *p = reinterpret_cast<unsigned char *>(s);

//     for (i = 0; i < n; i++) {
//         p[i] = c;
//     }

//     return s;
// }
};

__END_SYS

#endif
