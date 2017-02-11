#pragma once

#include <IOKit/storage/IOBlockStorageDevice.h>
#include "Sinetek_rtsx.hpp"

class SDDisk : public IOBlockStorageDevice
{
	OSDeclareDefaultStructors(SDDisk)
	
	friend void read_task_impl_(void *arg);
	
private:
	rtsx_softc *		provider_;
	IOLock *			util_lock_;
	uint32_t			num_blocks_;
	uint32_t			blk_size_;
	
public:
	virtual bool		init(struct sdmmc_softc *sc_sdmmc,
				     OSDictionary* properties = 0);
	
	virtual bool		attach(IOService* provider) override;
	virtual void		detach(IOService* provider) override;
	
	/**
	 * Subclassing requirements.
	 */
	
	virtual IOReturn	doEjectMedia(void) override;
	virtual IOReturn	doFormatMedia(UInt64 byteCapacity) override;
	virtual UInt32		doGetFormatCapacities(UInt64 * capacities, UInt32 capacitiesMaxCount) const override;
	virtual IOReturn	doLockUnlockMedia(bool doLock) override;
	virtual IOReturn	doSynchronizeCache(void) override;
	virtual char*		getVendorString(void) override;
	virtual char*		getProductString(void) override;
	virtual char*		getRevisionString(void) override;
	virtual char*		getAdditionalDeviceInfoString(void) override;
	virtual IOReturn	reportBlockSize(UInt64 *blockSize) override;
	virtual IOReturn	reportEjectability(bool *isEjectable) override;
	virtual IOReturn	reportLockability(bool *isLockable) override;
	virtual IOReturn	reportMaxValidBlock(UInt64 *maxBlock) override;
	virtual IOReturn	reportMediaState(bool *mediaPresent,bool *changedState) override;
	virtual IOReturn	reportPollRequirements(bool *pollRequired, bool *pollIsExpensive) override;
	virtual IOReturn	reportRemovability(bool *isRemovable) override;
	virtual IOReturn	reportWriteProtection(bool *isWriteProtected) override;
	virtual IOReturn	getWriteCacheState(bool *enabled) override;
	virtual IOReturn	setWriteCacheState(bool enabled) override;
	virtual IOReturn	doAsyncReadWrite(IOMemoryDescriptor *buffer, UInt64 block, UInt64 nblks, IOStorageAttributes *attributes, IOStorageCompletion *completion) override;
	
};
