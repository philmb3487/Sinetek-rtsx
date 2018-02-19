#include "Sinetek_rtsx.hpp"

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOTimerEventSource.h>

#undef super
#define super IOService
OSDefineMetaClassAndStructors(rtsx_softc, super);

#include "rtsxreg.h"
#include "rtsxvar.h"
#include "SDDisk.hpp"

bool rtsx_softc::start(IOService *provider)
{
	if (!super::start(provider))
		return false;

	assert(OSDynamicCast(IOPCIDevice, provider));
	provider_ = OSDynamicCast(IOPCIDevice, provider);
	if (!provider_)
	{
		printf("IOPCIDevice cannot be cast.\n");
		return false;
	}
	
	workloop_ = getWorkLoop();
	/*
	 * Enable memory response from the card.
	 */
	provider_->setMemoryEnable(true);
	
	prepare_task_loop();
	rtsx_pci_attach();
	
	return true;
}

void rtsx_softc::stop(IOService *provider)
{
	rtsx_pci_detach();
	destroy_task_loop();
	
	super::stop(provider);
}

static void trampoline_intr(OSObject *ih, IOInterruptEventSource *, int count)
{
	/* go to isr handler */
	rtsx_softc * that = OSDynamicCast(rtsx_softc, ih);
	rtsx_intr(that);
}

void rtsx_softc::rtsx_pci_attach()
{
	uint device_id;
	uint32_t flags;
    int bar = RTSX_PCI_BAR;
	
	if ((provider_->extendedConfigRead16(RTSX_CFG_PCI) & RTSX_CFG_ASIC) != 0)
	{
		printf("no asic\n");
		return;
	}
	
	/* Enable the device */
	provider_->setBusMasterEnable(true);
	
	/* Power up the device */
	if (this->requestPowerDomainState(kIOPMPowerOn,
					  (IOPowerConnection *) this->getParentEntry(gIOPowerPlane),
					  IOPMLowestState) != IOPMNoErr)
	{
		printf("pci_set_powerstate_D0: domain D0 not received.\n");
		return;
	}
    
    /* Map device memory with register. */
    map_ = provider_->mapDeviceMemoryWithRegister(bar);
    if (!map_) return;
    memory_descriptor_ = map_->getMemoryDescriptor();
    
    /* Map device interrupt. */
    intr_source_ = IOInterruptEventSource::interruptEventSource(this, trampoline_intr, provider_);
    if (!intr_source_)
    {
        printf("can't map interrupt source\n");
        return;
    }
    workloop_->addEventSource(intr_source_);
    intr_source_->enable();
    
    /* Get the vendor and try to match on it. */
    device_id = provider_->extendedConfigRead16(kIOPCIConfigDeviceID);
    switch (device_id) {
        case PCI_PRODUCT_REALTEK_RTS5209:
            flags = RTSX_F_5209;
            break;
        case PCI_PRODUCT_REALTEK_RTS5229:
        case PCI_PRODUCT_REALTEK_RTS5249:
            flags = RTSX_F_5229;
            break;
        case PCI_PRODUCT_REALTEK_RTS525A:
            /* syscl - RTS525A */
            flags = RTSX_F_525A;
            bar = RTSX_PCI_BAR_525A;
            break;
        default:
            flags = 0;
            break;
    }
	
	int error = rtsx_attach(this);
	if (!error)
	{
//		pci_present_and_attached_ = true;
	}
}

void rtsx_softc::rtsx_pci_detach()
{
//	rtsx_detach();
	
	workloop_->removeEventSource(intr_source_);
	intr_source_->release();
}


IOReturn rtsx_softc::setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker)
{
    IOReturn ret = IOPMAckImplied;
    
    IOLog("%s::setPowerState() ===>\n", __func__);
    
    if (powerStateOrdinal)
    {
        IOLog("%s::setPowerState: Wake from sleep\n", __func__);
        rtsx_activate(this, 1);
        goto done;
    }
    else
    {
        IOLog("%s::setPowerState: Sleep the card\n", __func__);
        rtsx_activate(this, 0);
        goto done;
    }

done:
    IOLog("%s::setPowerState() <===\n", __func__);
    
    return ret;
}

void rtsx_softc::prepare_task_loop()
{
	task_loop_ = IOWorkLoop::workLoop();
	task_execute_one_ = IOTimerEventSource::timerEventSource(this, task_execute_one_impl_);
	task_loop_->addEventSource(task_execute_one_);
}

void rtsx_softc::destroy_task_loop()
{
	task_execute_one_->cancelTimeout();
	task_loop_->removeEventSource(task_execute_one_);
	task_loop_->release();
}

/*
 * Takes a task off the list, executes it, and then
 * deletes that same task.
 */
extern auto sdmmc_del_task(struct sdmmc_task *task)		-> void;

void rtsx_softc::task_execute_one_impl_(OSObject *target, IOTimerEventSource *sender)
{
	rtsx_softc *sc = (rtsx_softc *)target;
	struct sdmmc_task *task;
	
	for (task = TAILQ_FIRST(&sc->sc_tskq); task != NULL;
	     task = TAILQ_FIRST(&sc->sc_tskq)) {
		sdmmc_del_task(task);
		task->func(task->arg);
	}
}

/**
 *  Attach the macOS portion of the driver: block storage.
 *
 *  That block device will hand us back calls such as read/write blk.
 */
void rtsx_softc::blk_attach()
{
	printf("rtsx: blk_attack()\n");
	
	sddisk_ = new SDDisk();
	sddisk_->init(this);
	sddisk_->attach(this);
	sddisk_->release();
	sddisk_->registerService();
}

void rtsx_softc::blk_detach()
{
	printf("rtsx: blk_detach()\n");

	sddisk_->terminate();
}
