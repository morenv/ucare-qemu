/** @author morenvino@xxxxxxxx.xxx */

/* ===========================================================================
 * Headers
 * ===========================================================================*/

//#include <execinfo.h> 
#include <sys/queue.h>
#include "shelter_config.h"
#include "shelter_debug.h"

/* ===========================================================================
 * Const
 * ===========================================================================*/

// Qemu Block.c aio status
#define NOT_DONE 0x7fffffff  // used while emulated sync operation in progress

/* ===========================================================================
 * Types
 * ===========================================================================*/

// IO Event
typedef struct IoEvent {
	size_t  blkno;
	size_t  blkcount;
	BlockDriverState *bs;
	QEMUIOVector *qiov;
	void *opaque;
	BlockDriverCompletionFunc *cb;
	BdrvRequestFlags flags;
	int ret;
	STAILQ_ENTRY(IoEvent) entries; // entries to next IoEvent
} IoEvent;

// IoQueue is queue of IO Events
typedef STAILQ_HEAD(IoQueue, IoEvent) IoQueue;

/* ===========================================================================
 * Global vars
 * ===========================================================================*/

// Global NFS disk 
static int nfs_disk = -1;
static void *nfs_fill = NULL; 

// A global queue of IO Events
static IoQueue *ioQueue = NULL;
static int ioQueueCount = 0;

// Mutex for IoQueue
static pthread_mutex_t ioQueueMutex = PTHREAD_MUTEX_INITIALIZER;
//static CoMutex ioQueueCoMutex;

// Cleanup Timer
static QEMUTimer *ioTimer;

/* ===========================================================================
 * Function
 * ===========================================================================*/

// Return a copy of the given QemuIoVector
static QEMUIOVector * qemu_iovec_create_copy(QEMUIOVector *qiov); 

// Destroy the copy of QemuIoVector
static void qemu_iovec_destroy_copy(QEMUIOVector *qiov); 

// Print QemuIoVector
static void qemu_iovec_print(QEMUIOVector *qiov); 

// Print AIOCB
// static void acb_print(BlockDriverAIOCBCoroutine *acb); 

// Create Io Event
static IoEvent * IoEvent_create(size_t blkno, size_t blkcount, 
		BlockDriverState *bs, QEMUIOVector *qiov, BdrvRequestFlags flags,
		BlockDriverCompletionFunc *cb, void *opaque);

// Destroy Io Event
static void IoEvent_destroy(IoEvent *event);

// Create IO Queue
static IoQueue * IoQueue_create(void);

// Destroy IO Queue
static void IoQueue_destroy(IoQueue *queue); 

// Return true if queue is empty
static inline int IoQueue_empty(IoQueue *queue); 

// Push item into queue
static inline void IoQueue_push(IoQueue *queue, IoEvent *item); 

// Pop item from queue
static inline IoEvent * IoQueue_pop(IoQueue *queue);

// Entry point to coroutine commit_writev()  
static void coroutine_fn qcow2_co_commit_entry(void *opaque);

// Run commit_writev() from non-coroutine
static int qcow2_commit_writev(IoEvent *io);

// Clean up queue using thread
static void * cleanup_thread(void *arg);

// Clean up queue using coroutine
static void coroutine_fn cleanup_coroutine(void *arg);

// Clean up queue using timer
static void cleanup_timer(void *arg);
