/** @author morenvino@xxxxxxxx.xxx */

#ifndef UCARE_SHELTER_DEBUG_H
#define UCARE_SHELTER_DEBUG_H

/* ===========================================================================
 * Header
 * ===========================================================================*/

#include "shelter_config.h"

/* ===========================================================================
 * Macro
 * ===========================================================================*/

// Return true if block driver's device is ide0-hd'1'
// for now, just check 1 char instead of strcmp() to speed-up ...
#define MV_IS_HD1(bs)       (bs->device_name[7] == '1') 

// Return true if block driver's file is 's'ystem disk
#define MV_IS_SYS(bs)       (bs->filename[0] == 's')

// Trace ide0-hd1 or non system disk
#define MV_SHOULD_TRACE(bs) (MV_IS_HD1(bs) || !MV_IS_SYS(bs))

// Trace function
#if MV_TRACE_ENABLE

#define MV_TRACE()          printf("[%s]\n", __func__)
#define MV_TRACE_HD1(bs)    if (MV_SHOULD_TRACE(bs)) MV_TRACE()

#define MV_DEBUG(fmt, ...)         printf("[%s] " fmt, __func__, ##__VA_ARGS__) 
#define MV_DEBUG_HD1(bs, fmt, ...) if (MV_SHOULD_TRACE(bs)) MV_DEBUG(fmt, ##__VA_ARGS__)

#else // not MV_TRACE_ENABLE

#define MV_TRACE()             // disabled
#define MV_TRACE_HD1()         // disabled
#define MV_DEBUG(fmt, ...)     // disabled
#define MV_DEBUG_HD1(bs, fmt, ...) // disabled

#endif //MV_TRACE_ENABLE

#endif // UCARE_SHELTER_DEBUG_H
