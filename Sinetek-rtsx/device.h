//
//  device.h
//  Sinetek-rtsx
//
//  Created by syscl on 2/9/18.
//  Copyright © 2018 sinetek. All rights reserved.
//

#ifndef device_h
#define device_h

/*
 * Actions for ca_activate.
 */
#define    DVACT_DEACTIVATE    1    /* deactivate the device */
#define    DVACT_QUIESCE        2    /* warn the device about suspend */
#define    DVACT_SUSPEND        3    /* suspend the device */
#define    DVACT_RESUME        4    /* resume the device */
#define    DVACT_WAKEUP        5    /* tell device to recover after resume */
#define    DVACT_POWERDOWN        6    /* power device down */

#endif /* device_h */
