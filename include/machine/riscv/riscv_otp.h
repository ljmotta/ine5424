// EPOS RISC-V OTP Mediator Declarations

#ifndef __riscv_otp_h
#define __riscv_otp_h

#include <architecture/cpu.h>
#include <machine/uart.h>
#include <system/memory_map.h>
#include <utility/string.h>
#include <time.h>

#define EINVAL 22

__BEGIN_SYS

static inline void writel(unsigned int val, volatile unsigned int *addr) {
    ASM("fence " #ow "," #ow : : : "memory");
    (*(volatile unsigned int *)(addr) = (val));
}

static inline unsigned int readl(void *addr) {
    unsigned int val = (*(volatile unsigned int *)(addr));
    ASM("fence " #ir "," #ir : : : "memory");
    return val;
}

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

    // OTP registers offset from OTP_BASE
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

    enum {
        BYTES_PER_FUSE		    = 4, // 8 * 4
        TOTAL_FUSES             = 4096,
        PA_RESET_VAL		    = 0x00,
        PAS_RESET_VAL		    = 0x00,
        PAIO_RESET_VAL		    = 0x00,
        PDIN_RESET_VAL		    = 0x00,
        PTM_RESET_VAL		    = 0x00,
        PCLK_ENABLE_VAL			= 0x01,
        PCLK_DISABLE_VAL		= 0x00,
        PWE_WRITE_ENABLE		= 0x01,
        PWE_WRITE_DISABLE		= 0x00,
        PTM_FUSE_PROGRAM_VAL	= 0x10,
        PCE_ENABLE_INPUT		= 0x01,
        PCE_DISABLE_INPUT		= 0x00,
        PPROG_ENABLE_INPUT		= 0x01,
        PPROG_DISABLE_INPUT		= 0x00,
        PTRIM_ENABLE_INPUT		= 0x01,
        PTRIM_DISABLE_INPUT		= 0x00,
        PDSTB_DEEP_STANDBY_ENABLE	= 0x01,
        PDSTB_DEEP_STANDBY_DISABLE	= 0x00
    }

public:
    SiFive_OTP() {
    }

    int read(int offset, void *buf, int size) {
        /* Check if offset and size are multiple of BYTES_PER_FUSE */
        if ((size % BYTES_PER_FUSE) || (offset % BYTES_PER_FUSE)) {
            // printf("%s: size and offset must be multiple of 4.\n",
            //     __func__);
            return -EINVAL;
        }

        int fuseidx = offset / BYTES_PER_FUSE;
        int fusecount = size / BYTES_PER_FUSE;

        /* check bounds */
        if (offset < 0 || size < 0)
            return -EINVAL;
        if (fuseidx >= &SiFive_OTP::TOTAL_FUSES)
            return -EINVAL;
        if ((fuseidx + fusecount) > &SiFive_OTP::TOTAL_FUSES)
            return -EINVAL;

        unsigned int fusebuf[fusecount];

        /* init OTP */
        writel(PDSTB_DEEP_STANDBY_ENABLE, &SiFive_OTP::PDSTB);
        writel(PTRIM_ENABLE_INPUT, &SiFive_OTP::PTRIM);
        writel(PCE_ENABLE_INPUT, &SiFive_OTP::PCE);

        /* read all requested fuses */
        for (unsigned int i = 0; i < fusecount; i++, fuseidx++) {
            writel(fuseidx, &&SiFive_OTP::PA);

            /* cycle clock to read */
            writel(PCLK_ENABLE_VAL, &SiFive_OTP::PCLK);
            // EPOS
            Delay tcd(TCD_DELAY);
            writel(PCLK_DISABLE_VAL, &SiFive_OTP::PCLK);
            // EPOS
            Delay tkl(TKL_DELAY);

            /* read the value */
            fusebuf[i] = readl(&SiFive_OTP::PDOUT);
        }

        /* shut down */
        writel(PCE_DISABLE_INPUT, &SiFive_OTP::PCE);
        writel(PTRIM_DISABLE_INPUT, &SiFive_OTP::PTRIM);
        writel(PDSTB_DEEP_STANDBY_DISABLE, &SiFive_OTP::PDSTB);

        /* copy out */
        memcpy(buf, fusebuf, size);

        return size;
    }

    int write(int offset, const void *buf, int size) {
        /* Check if offset and size are multiple of BYTES_PER_FUSE */
        if ((size % BYTES_PER_FUSE) || (offset % BYTES_PER_FUSE)) {
            // printf("%s: size and offset must be multiple of 4.\n",
            //    __func__);
            return -EINVAL;
        }

        int fuseidx = offset / BYTES_PER_FUSE;
        int fusecount = size / BYTES_PER_FUSE;
        unsigned int *write_buf = (unsigned int *)buf;
        unsigned int write_data;
        int i, pas, bit;

        /* check bounds */
        if (offset < 0 || size < 0)
            return -EINVAL;
        if (fuseidx >= &SiFive_OTP::TOTAL_FUSES)
            return -EINVAL;
        if ((fuseidx + fusecount) > &SiFive_OTP::TOTAL_FUSES)
            return -EINVAL;

        /* init OTP */
        writel(PDSTB_DEEP_STANDBY_ENABLE, &SiFive_OTP::PDSTB);
        writel(PTRIM_ENABLE_INPUT, &SiFive_OTP::PTRIM);

        /* reset registers */
        writel(PCLK_DISABLE_VAL, &SiFive_OTP::PCLK);
        writel(PA_RESET_VAL, &SiFive_OTP::PA);
        writel(PAS_RESET_VAL, &SiFive_OTP::PAS);
        writel(PAIO_RESET_VAL, &SiFive_OTP::PAIO);
        writel(PDIN_RESET_VAL, &SiFive_OTP::PDIN);
        writel(PWE_WRITE_DISABLE, &SiFive_OTP::PWE);
        writel(PTM_FUSE_PROGRAM_VAL, &SiFive_OTP::PTM);
        // EPOS
        Delay tms(TMS_DELAY);

        writel(PCE_ENABLE_INPUT, &SiFive_OTP::PCE);
        writel(PPROG_ENABLE_INPUT, &SiFive_OTP::PPROG);

        /* write all requested fuses */
        for (i = 0; i < fusecount; i++, fuseidx++) {
            writel(fuseidx, &SiFive_OTP::PA);
            write_data = *(write_buf++);

            for (pas = 0; pas < 2; pas++) {
                writel(pas, &SiFive_OTP::PAS);

                for (bit = 0; bit < 32; bit++) {
                    writel(bit, &SiFive_OTP::PAIO);
                    writel(((write_data >> bit) & 1), &SiFive_OTP::PDIN);
                    // EPOS
                    Delay tasp(TASP_DELAY);

                    writel(PWE_WRITE_ENABLE, &SiFive_OTP::PWE);

                    // EPOS
                    Delay tpw(TPW_DELAY);
                    writel(PWE_WRITE_DISABLE, &SiFive_OTP::PWE);

                    // EPOS
                    Delay tpwi(TPWI_DELAY);
                }
            }

            writel(PAS_RESET_VAL, &SiFive_OTP::PAS);
        }

        /* shut down */
        writel(PWE_WRITE_DISABLE, &SiFive_OTP::PWE);
        writel(PPROG_DISABLE_INPUT, &SiFive_OTP::PPROG);
        writel(PCE_DISABLE_INPUT, &SiFive_OTP::PCE);
        writel(PTM_RESET_VAL, &SiFive_OTP::PTM);
        writel(PTRIM_DISABLE_INPUT, &SiFive_OTP::PTRIM);
        writel(PDSTB_DEEP_STANDBY_DISABLE, &SiFive_OTP::PDSTB);

        return size;
    }
};

class OTP: private SiFive_OTP {}

__END_SYS

#endif
