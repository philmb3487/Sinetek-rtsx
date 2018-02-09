#pragma once

#include <stdint.h>

#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#include "sdmmcvar.h"

#define sdmmc_softc rtsx_softc

/* Number of registers to save for suspend/resume in terms of their ranges. */
#define RTSX_NREG ((0XFDAE - 0XFDA0) + (0xFD69 - 0xFD32) + (0xFE34 - 0xFE20))

class SDDisk;
struct rtsx_softc : public IOPCIDevice
{

	OSDeclareDefaultStructors(rtsx_softc);
    
public:
    
	virtual bool start(IOService * provider) override;
	virtual void stop(IOService * provider) override;
    
	void rtsx_pci_attach();
	void rtsx_pci_detach();
    /* syscl - Power Management Support */
    virtual IOReturn setPowerState(unsigned long powerStateOrdinal, IOService * policyMaker) override;
	
	void blk_attach();
	void blk_detach();
	
	// ** //
	IOPCIDevice *		provider_;
	IOWorkLoop *		workloop_;
	IOMemoryMap *		map_;
	IOMemoryDescriptor *	memory_descriptor_;
	IOInterruptEventSource *intr_source_;
	
	SDDisk *			sddisk_;
	struct sdmmc_task	read_task_;
	
	/*
	 * rtsx_softc variables.
	 */
	int		flags;
	uint32_t		intr_status;
	u_int8_t		regs[RTSX_NREG];/* host controller state */
	u_int32_t	regs4[6];	/* host controller state */
	IOBufferMemoryDescriptor * dmap_cmd, *dmap_data;
	
	
	/*
	 * Task thread.
	 */
	IOWorkLoop *		task_loop_;
	IOTimerEventSource *	task_execute_one_;
	void			task_add();
	void			prepare_task_loop();
	void			destroy_task_loop();
	static void		task_execute_one_impl_(OSObject *, IOTimerEventSource *);
	
	/*
	 * sdmmc_softc variables.
	 */
#define SDMMC_MAXNSEGS	((MAXPHYS / PAGE_SIZE) + 1)
	
	int sc_flags;
#define SMF_SD_MODE		0x0001	/* host in SD mode (MMC otherwise) */
#define SMF_IO_MODE		0x0002	/* host in I/O mode (SD mode only) */
#define SMF_MEM_MODE		0x0004	/* host in memory mode (SD or MMC) */
#define SMF_CARD_PRESENT	0x0010	/* card presence noticed */
#define SMF_CARD_ATTACHED	0x0020	/* card driver(s) attached */
#define	SMF_STOP_AFTER_MULTIPLE	0x0040	/* send a stop after a multiple cmd */
#define SMF_CONFIG_PENDING	0x0080	/* config_pending_incr() called */
	
	uint32_t sc_caps;		/* host capability */
#define SMC_CAPS_AUTO_STOP	0x0001	/* send CMD12 automagically by host */
#define SMC_CAPS_4BIT_MODE	0x0002	/* 4-bits data bus width */
#define SMC_CAPS_DMA		0x0004	/* DMA transfer */
#define SMC_CAPS_SPI_MODE	0x0008	/* SPI mode */
#define SMC_CAPS_POLL_CARD_DET	0x0010	/* Polling card detect */
#define SMC_CAPS_SINGLE_ONLY	0x0020	/* only single read/write */
#define SMC_CAPS_8BIT_MODE	0x0040	/* 8-bits data bus width */
#define SMC_CAPS_MULTI_SEG_DMA	0x0080	/* multiple segment DMA transfer */
#define SMC_CAPS_SD_HIGHSPEED	0x0100	/* SD high-speed timing */
#define SMC_CAPS_MMC_HIGHSPEED	0x0200	/* MMC high-speed timing */
#define SMC_CAPS_UHS_SDR50	0x0400	/* UHS SDR50 timing */
#define SMC_CAPS_UHS_SDR104	0x0800	/* UHS SDR104 timing */
#define SMC_CAPS_UHS_DDR50	0x1000	/* UHS DDR50 timing */
#define SMC_CAPS_UHS_MASK	0x1c00
#define SMC_CAPS_MMC_DDR52	0x2000  /* eMMC DDR52 timing */
#define SMC_CAPS_MMC_HS200	0x4000	/* eMMC HS200 timing */
#define SMC_CAPS_MMC_HS400	0x8000	/* eMMC HS400 timing */
	
	int sc_function_count;		/* number of I/O functions (SDIO) */
	struct sdmmc_function *sc_card;	/* selected card */
	struct sdmmc_function *sc_fn0;	/* function 0, the card itself */
	STAILQ_HEAD(, sdmmc_function) sf_head; /* list of card functions */
	int sc_dying;			/* bus driver is shutting down */
	struct proc *sc_task_thread;	/* asynchronous tasks */
	TAILQ_HEAD(, sdmmc_task) sc_tskq;   /* task thread work queue */
	struct sdmmc_task sc_discover_task; /* card attach/detach task */
	struct sdmmc_task sc_intr_task;	/* card interrupt task */
	//	struct rwlock sc_lock;		/* lock around host controller */
	void *sc_scsibus;		/* SCSI bus emulation softc */
	TAILQ_HEAD(, sdmmc_intr_handler) sc_intrq; /* interrupt handlers */
	long sc_max_xfer;		/* maximum transfer size */

};
