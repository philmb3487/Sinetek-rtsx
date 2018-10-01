#include <IOKit/IOLib.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <IOKit/IOMemoryDescriptor.h>

#include "SDDisk.hpp"
#include "Sinetek_rtsx.hpp"
#include "sdmmcvar.h"
#include "device.h"

// Define the superclass
#define super IOBlockStorageDevice
OSDefineMetaClassAndStructors(SDDisk, IOBlockStorageDevice)

bool SDDisk::init(struct sdmmc_softc *sc_sdmmc, OSDictionary* properties)
{
	if (super::init(properties) == false)
		return false;
	
	util_lock_ = IOLockAlloc();
	
	return true;
}

bool SDDisk::attach(IOService* provider)
{
	if (super::attach(provider) == false)
		return false;
	
	provider_ = OSDynamicCast(rtsx_softc, provider);
	if (provider_ == NULL)
		return false;
	
	provider_->retain();
	
	num_blocks_ = provider_->sc_fn0->csd.capacity;
	blk_size_   = provider_->sc_fn0->csd.sector_size;
	
	printf("rtsx: attaching SDDisk, num_blocks:%d  blk_size:%d\n",
	       num_blocks_, blk_size_);
	
	return true;
}

void SDDisk::detach(IOService* provider)
{
	provider_->release();
	provider_ = NULL;
	
	super::detach(provider);
}

IOReturn SDDisk::doEjectMedia(void)
{
    IOLog("%s: RAMDISK: doEjectMedia.", __func__);
	
	// XXX signal intent further down the stack?
    // syscl - implement eject routine here
    rtsx_card_eject(provider_);
	return kIOReturnSuccess;
}

IOReturn SDDisk::doFormatMedia(UInt64 byteCapacity)
{
	return kIOReturnSuccess;
}

UInt32 SDDisk::GetBlockCount() const
{
    return num_blocks_;
}

UInt32 SDDisk::doGetFormatCapacities(UInt64* capacities, UInt32 capacitiesMaxCount) const
{
	// Ensure that the array is sufficient to hold all our formats (we require 1 element).
	if ((capacities != NULL) && (capacitiesMaxCount < 1))
		return 0;               // Error, return an array size of 0.
	
	/*
	 * We need to run circles around the const-ness of this function.
	 */
//	auto blockCount = const_cast<SDDisk *>(this)->getBlockCount();
    auto blockCount = GetBlockCount();
	
	// The caller may provide a NULL array if it wishes to query the number of formats that we support.
	if (capacities != NULL)
		capacities[0] = blockCount * blk_size_;
	return 1;
}

IOReturn SDDisk::doLockUnlockMedia(bool doLock)
{
	return kIOReturnUnsupported;
}

IOReturn SDDisk::doSynchronizeCache(void)
{
	return kIOReturnSuccess;
}

char* SDDisk::getVendorString(void)
{
    // syscl - safely converted to char * use const_static due
    // to ISO C++11 does not allow conversion from string literal to 'char *'
    return const_cast<char *>("Realtek");
}

char* SDDisk::getProductString(void)
{
    // syscl - safely converted to char * use const_static due
    // to ISO C++11 does not allow conversion from string literal to 'char *'
    return const_cast<char *>("SD Card Reader");
}

char* SDDisk::getRevisionString(void)
{
    // syscl - safely converted to char * use const_static due
    // to ISO C++11 does not allow conversion from string literal to 'char *'
    return const_cast<char *>("1.0");
}

char* SDDisk::getAdditionalDeviceInfoString(void)
{
    // syscl - safely converted to char * use const_static due
    // to ISO C++11 does not allow conversion from string literal to 'char *''
    return const_cast<char *>("1.0");
}

IOReturn SDDisk::reportBlockSize(UInt64 *blockSize)
{
	*blockSize = blk_size_;
	return kIOReturnSuccess;
}

IOReturn SDDisk::reportEjectability(bool *isEjectable)
{
    *isEjectable = true; // syscl - should be true
    return kIOReturnSuccess;
}

/* syscl - deprecated
IOReturn SDDisk::reportLockability(bool *isLockable)
{
	*isLockable = false;
	return kIOReturnSuccess;
}*/

IOReturn SDDisk::reportMaxValidBlock(UInt64 *maxBlock)
{
//	*maxBlock = num_blocks_ - 1;
	*maxBlock = num_blocks_ - 1000;
	return kIOReturnSuccess;
}

IOReturn SDDisk::reportMediaState(bool *mediaPresent, bool *changedState)
{
	*mediaPresent = true;
	*changedState = false;
	
	return kIOReturnSuccess;
}

IOReturn SDDisk::reportPollRequirements(bool *pollRequired, bool *pollIsExpensive)
{
	*pollRequired = false;
	*pollIsExpensive = false;
	return kIOReturnSuccess;
}

IOReturn SDDisk::reportRemovability(bool *isRemovable)
{
	*isRemovable = true;
	return kIOReturnSuccess;
}

IOReturn SDDisk::reportWriteProtection(bool *isWriteProtected)
{
	*isWriteProtected = true; // XXX
	return kIOReturnSuccess;
}

IOReturn SDDisk::getWriteCacheState(bool *enabled)
{
	return kIOReturnUnsupported;
}

IOReturn SDDisk::setWriteCacheState(bool enabled)
{
	return kIOReturnUnsupported;
}


struct BioArgs
{
	IOMemoryDescriptor *buffer;
	IODirection direction;
	UInt64 block;
	UInt64 nblks;
	IOStorageAttributes attributes;
	IOStorageCompletion completion;
	SDDisk *that;
};

// move me
int
sdmmc_mem_read_block(struct sdmmc_function *sf, int blkno, u_char *data,
		     size_t datalen);
int
sdmmc_mem_single_read_block(struct sdmmc_function *sf, int blkno, u_char *data,
			    size_t datalen);
void
sdmmc_add_task(struct sdmmc_softc *sc, struct sdmmc_task *task);
void
sdmmc_go_idle_state(struct sdmmc_softc *sc);
int
sdmmc_mem_read_block_subr(struct sdmmc_function *sf,
			  int blkno, u_char *data, size_t datalen);



void read_task_impl_(void *_args)
{
	BioArgs *args = (BioArgs *) _args;
	IOByteCount actualByteCount;
	int error = 0;
	
	if (!args) return;
	
	printf("read_task_impl_  sz %llu\n", args->nblks * args->that->blk_size_);
	printf("sf->csd.sector_size %d\n", args->that->provider_->sc_fn0->csd.sector_size);
	
	
	actualByteCount = args->nblks * args->that->blk_size_;
	auto map = args->buffer->map();
	u_char * buf = (u_char *) map->getVirtualAddress();
	
	for (UInt64 b = 0; b < args->nblks; ++b)
	{
		sdmmc_mem_single_read_block(args->that->provider_->sc_fn0,
						    0, buf + b * 512, 512);
		sdmmc_mem_read_block_subr(args->that->provider_->sc_fn0,
					  0, buf, 512);
		sdmmc_go_idle_state(args->that->provider_);
	}
	
	for (UInt64 b = 0; b < args->nblks; ++b)
	{
		printf("would: %lld  last block %d\n", args->block + b, args->that->num_blocks_ - 1);
		//unsigned int would = args->block + b;
        auto would = args->block + b;
		//if ( would > 60751872 ) would = 60751871;
		error = sdmmc_mem_read_block_subr(args->that->provider_->sc_fn0,
				static_cast<int>(would), buf + b * 512, 512);
		if (error) {
			(args->completion.action)(args->completion.target, args->completion.parameter, kIOReturnIOError, 0);
			goto out;
		}
	}
	
	(args->completion.action)(args->completion.target, args->completion.parameter, kIOReturnSuccess, actualByteCount);
	
out:
	delete args;
}

/**
 * Start an async read or write operation.
 * @param buffer
 * An IOMemoryDescriptor describing the data-transfer buffer. The data direction
 * is contained in the IOMemoryDescriptor. Responsibility for releasing the descriptor
 * rests with the caller.
 * @param block
 * The starting block number of the data transfer.
 * @param nblks
 * The integral number of blocks to be transferred.
 * @param attributes
 * Attributes of the data transfer. See IOStorageAttributes.
 * @param completion
 * The completion routine to call once the data transfer is complete.
 */
IOReturn SDDisk::doAsyncReadWrite(IOMemoryDescriptor *buffer,
				  UInt64 block,
				  UInt64 nblks,
				  IOStorageAttributes *attributes,
				  IOStorageCompletion *completion)
{
	IODirection		direction;
	
	// Return errors for incoming I/O if we have been terminated
	if (isInactive() != false)
		return kIOReturnNotAttached;
	
	direction = buffer->getDirection();
	if ((direction != kIODirectionIn) && (direction != kIODirectionOut))
		return kIOReturnBadArgument;
	
	if ((block + nblks) > num_blocks_)
		return kIOReturnBadArgument;
	
	if ((direction != kIODirectionIn) && (direction != kIODirectionOut))
		return kIOReturnBadArgument;
	
	// printf("=====================================================\n");
	IOLockLock(util_lock_);

	/*
	 * Copy things over as we're going to lose the parameters once this
	 * method returns. (async call)
	 */
	BioArgs *bioargs = new BioArgs;
	bioargs->buffer = buffer;
	bioargs->direction = direction;
	bioargs->block = block;
	bioargs->nblks = nblks;
	if (attributes != nullptr)
		bioargs->attributes = *attributes;
	if (completion != nullptr)
		bioargs->completion = *completion;
	bioargs->that = this;
	
	sdmmc_init_task(&provider_->read_task_, read_task_impl_, bioargs);
	sdmmc_add_task(provider_, &provider_->read_task_);
	
	IOLockUnlock(util_lock_);
	// printf("=====================================================\n");
	
	return kIOReturnSuccess;
}
