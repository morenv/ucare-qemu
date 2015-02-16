/** @author morenvino@xxxxxxxx.xxx */

#ifndef UCARE_SHELTER_CONFIG_H
#define UCARE_SHELTER_CONFIG_H

/* ===========================================================================
 * Build
 * ===========================================================================*/
// We have the following build modes:
// MV_IO_default : default QEMU build 
// MV_IO_remove  : remove/drop random write
// MV_IO_shelter : shelter random writes to NFS
// MV_IO_period  : shelter and queue to memory, cleanup by period
// MV_IO_activity: shelter and queue to memory, cleanup by disk activity 

// if cleanup policy is specified, enable sheltering and queuing too  
#if defined(MV_IO_period) || defined(MV_IO_activity)
#define MV_IO_shelter
#define MV_IO_queue
#define MV_IO_cleanup
#endif // MV_IO_period || MV_IO_activity 

/* ===========================================================================
 * Config
 * ===========================================================================*/

// Enable trace
#define MV_TRACE_ENABLE  1  

// Random write size
#define RAND_WR_SIZE     32768 // bytes

// Clean up timer
#ifdef MV_IO_period
#define CLEANUP_PERIOD   5000 // ms
#define LOG_PERIOD       5000
#else // MV_IO_activity
#define CLEANUP_PERIOD    200 // ms
#define LOG_PERIOD       1000
#endif // MV_IO_period

// Clean up threshold
#define CLEANUP_COUNT_THRESHOLD     20  // iop/cleanup_period -> 100 iop/s
#define CLEANUP_SIZE_THRESHOLD   20480  // blocks/cleanup_period -> 50 MB/s

// Number of io to submit during cleanup
//#define CLEANUP_BATCH_SIZE          32   // io/cleanup
#define CLEANUP_BATCH_SIZE       INT_MAX   // io/cleanup

// Which level to enable/disable sheltering logic ... 
//#define SHELTERING_ON_BLOCK 0
#define SHELTERING_ON_QCOW2 0
#define SHELTERING_ON_RAW   1


#endif // UCARE_SHELTER_CONFIG_H
