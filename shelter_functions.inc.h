/** @author morenvino@xxxxxxxx.xxx */

// Return a copy of the given QemuIoVector
static QEMUIOVector * qemu_iovec_create_copy(QEMUIOVector *qiov) 
{
	QEMUIOVector *copy = (QEMUIOVector *)malloc(sizeof(QEMUIOVector));
	if (copy) {
		int i;
		
		copy->size = qiov->size;
		copy->nalloc = qiov->nalloc;
		copy->niov = qiov->niov;
		copy->iov = (struct iovec *)malloc(sizeof(struct iovec) * copy->niov);
		
		for (i = 0; i < qiov->niov; ++i) {
			struct iovec *src = qiov->iov + i, *dst = copy->iov + i;
			dst->iov_len = src->iov_len;
			// disable copying real data for now
			//dst->iov_base = malloc(dst->iov_len);
			//memcpy(dst->iov_base, src->iov_base, dst->iov_len);
			dst->iov_base = nfs_fill;
		}
	}
	return copy;
}

// Destroy the copy of QemuIoVector
static void qemu_iovec_destroy_copy(QEMUIOVector *qiov) 
{
	int i;
	for (i = 0; i < qiov->niov; ++i) {
		struct iovec *iov = qiov->iov + i;;
		iov->iov_len = 0;
		//free(iov->iov_base); // disable copying real data for now
		iov->iov_base = NULL;
	}
	
	free(qiov->iov);
	qiov->iov = NULL;
	
	qiov->nalloc = 0;
	qiov->size = 0; 
	qiov->niov = 0;
	
	free(qiov);
}

// Print QemuIoVector
static void qemu_iovec_print(QEMUIOVector *qiov) 
{
		const int max_data = 5; // print the first max_data bytes only
		int i, j;
		
		printf("{count:%d, capacity:%d, size:%ld, [", 
			qiov->niov, qiov->nalloc, qiov->size);
		struct iovec *p = qiov->iov; 
		for (i = 0; i < qiov->niov; ++i) {
			printf("{size:%ld, data:", p->iov_len);
			for (j = 0; j < MIN(p->iov_len, max_data); ++j)
				printf("%02x", *((uint8_t *)p->iov_base + j));
			++p;
			if (max_data < p->iov_len) printf("..");
			printf("}, ");
		}
		printf("]}\n");
}

//#define DREF(P)  (P)? (*P) : -1
// Print AIOCB
/*
static void acb_print(BlockDriverAIOCBCoroutine *acb) {
		BlockDriverAIOCB *com = &acb->common;
		BlockRequest *req = &acb->req;
		QEMUIOVector *qiov = req->qiov;

		printf("%p -> {is_write:%d, done:%d, \n \
				com:{info:%p, bs:%p, cb:%p, op:%p}, \n \
				req:{sect:%ld, nbsect:%d, flag:%d, err:%d, cb:%p, op:%p}, \n \
				qiov:", 
				acb, acb->is_write, DREF(acb->done),
				com->aiocb_info, com->bs, com->cb, com->opaque,
				req->sector, req->nb_sectors, req->flags, req->error, req->cb, req->opaque    );
				qemu_iovec_print(qiov);
		printf("}\n");
}
*/

// Create Io Event
static IoEvent * IoEvent_create(size_t blkno, size_t blkcount, 
	BlockDriverState *bs, QEMUIOVector *qiov, BdrvRequestFlags flags,
	BlockDriverCompletionFunc *cb, void *opaque) 
{
	IoEvent *event = (IoEvent *)malloc(sizeof(IoEvent));
	if (event) {
		event->blkno = blkno;
		event->blkcount = blkcount;
		event->bs = bs;
		event->flags = flags;
		event->opaque = opaque;
		event->cb = cb;
		//event->qiov = qemu_iovec_create_copy(qiov);
		event->qiov = qiov;
		event->ret = NOT_DONE;
	}
	return event;
}

// Destroy Io Event
static void IoEvent_destroy(IoEvent *event) 
{
	event->blkno = 0;
	event->blkcount = 0;
	event->bs = NULL;
	if (event->qiov) qemu_iovec_destroy_copy(event->qiov);
	event->qiov = NULL;
	event->ret = 0;
	free(event);
}

// Create IO Queue
static IoQueue * IoQueue_create(void) 
{
	IoQueue *queue = (IoQueue *)malloc(sizeof(IoQueue));
	if (queue) {
		STAILQ_INIT(queue);
		ioQueueCount = 0;
	}
	return queue;
}

// Destroy IO Queue
static void IoQueue_destroy(IoQueue *queue) 
{
	free(queue);
}

// Return true if queue is empty
static inline int IoQueue_empty(IoQueue *queue) 
{
	return STAILQ_EMPTY(queue);
}

// Return true if queue is not empty
static inline int IoQueue_has_item(IoQueue *queue) 
{
	return !STAILQ_EMPTY(queue);
}

// Push item into queue
static inline void IoQueue_push(IoQueue *queue, IoEvent *item) 
{
	++ioQueueCount;
	ioQueueBlock += item->blkcount;
	STAILQ_INSERT_TAIL(queue, item, entries);
}

// Pop item from queue
static inline IoEvent * IoQueue_pop(IoQueue *queue) {
	IoEvent *item = STAILQ_FIRST(queue);
	STAILQ_REMOVE_HEAD(queue, entries);
	--ioQueueCount;
	ioQueueBlock -= item->blkcount;
	return item;
}

// Entry point to coroutine commit_writev()  
#if SHELTERING_ON_QCOW2
static void coroutine_fn qcow2_co_commit_entry(void *opaque)
{
	IoEvent *io = opaque;
	io->ret = qcow2_co_commit_writev(io->bs, io->blkno, io->blkcount, io->qiov);
}
#endif //SHELTERING_ON_QCOW2

// Run commit_writev() from non-coroutine
static int qcow2_commit_writev(IoEvent *io)
{
	// for qcow2
#if SHELTERING_ON_QCOW2
	Coroutine *co;
	
	if (qemu_in_coroutine()) {
		qcow2_co_commit_entry(io); // Fast-path if already in coroutine context 
	} else {
		co = qemu_coroutine_create(qcow2_co_commit_entry);
		qemu_coroutine_enter(co, io);
		while (io->ret == NOT_DONE) {
			qemu_aio_wait();
		}
	}
	return io->ret;
#endif //SHELTERING_ON_QCOW2
	
	// for raw-posix
	return 0;
}


// Clean up queue using thread
static void * cleanup_thread(void *arg)
{
#ifdef MV_IO_cleanup
	int tmp_disk = open("/data/ubuntu-kvm/temp_disk", O_WRONLY | O_DIRECT | O_SYNC);
	if (tmp_disk == -1) fprintf(stderr, "Error opening tmp disk: %m\n"); 

	while (1) {
		MV_DEBUG("queue:%d count:%d size:%ld\n", ioQueueCount, io_count, io_size);
		//cleanup_print();

		int commit_count = 0;
		int to_commit_count = ioQueueCount;
		while (to_commit_count != 0 && IoQueue_has_item(ioQueue) && commit_count < CLEANUP_BATCH_SIZE) {
			//printf(".");
			// pop queue				
			pthread_mutex_lock(&ioQueueMutex);
			IoEvent *event = IoQueue_pop(ioQueue);
			pthread_mutex_unlock(&ioQueueMutex);
			if (event == NULL) {
				MV_DEBUG("queue has item but fail to pop\n");
				continue;
			}
			
			//qemu_iovec_print(event->qiov);

			// commit io
			//qcow2_commit_writev(event);
			//MV_DEBUG("writing %ld %ld\n", event->blkcount, event->blkno);
			int len = pwrite(tmp_disk, tmp_fill, event->blkcount, event->blkno);
			if (len == -1) fprintf(stderr, "Error writing to disk: %m\n");

			++commit_count;
			--to_commit_count;

			//MV_DEBUG("before destroy\n");
			IoEvent_destroy(event);
			//MV_DEBUG("after destroy\n");
		}

		sleep(CLEANUP_PERIOD/1000);
	}
#endif
	return 0;
}

// Clean up queue using coroutine
static void coroutine_fn cleanup_coroutine(void *arg)
{
	while (1) {
		MV_DEBUG("count:%d\n", ioQueueCount);
		// pop queue
		while (!IoQueue_empty(ioQueue)) {
			//pthread_mutex_lock(&ioQueueMutex);
			IoEvent *event = IoQueue_pop(ioQueue);
			// pthread_mutex_unlock(&ioQueueMutex);
			
			// qcow2_co_commit_writev(event->bs, event->blkno, event->blkcount, event->qiov);
			
			IoEvent_destroy(event);
		}
		sleep(CLEANUP_PERIOD/1000);
	}
}

#ifdef MV_IO_cleanup

// log cleanup queue status
static inline void cleanup_print(void) {
	//if (ioQueueCount != 0) 
	fprintf(log_file, "%d,%ld,%d\n", ioQueueCount, ioQueueBlock, cleanup_count);
	fflush(log_file);
} 

static void log_timer(void *arg) {
	//cleanup_print();

	// repeat timer
	//timer_mod(logTimer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + LOG_PERIOD);
}

#endif //MV_IO_cleanup

// Clean up queue using timer
#if defined(MV_IO_period)
static void cleanup_timer(void *arg)
{
	MV_DEBUG("queue:%d count:%d size:%ld\n", ioQueueCount, io_count, io_size);
	cleanup_print();

	int commit_count = 0;
	//while (!IoQueue_empty(ioQueue)) {
	
	if (IoQueue_has_item(ioQueue))
		++cleanup_count;

	while (IoQueue_has_item(ioQueue) && commit_count < CLEANUP_BATCH_SIZE) {
			// pop queue				
		//pthread_mutex_lock(&ioQueueMutex);
		IoEvent *event = IoQueue_pop(ioQueue);
		//pthread_mutex_unlock(&ioQueueMutex);

		//qemu_iovec_print(event->qiov);

		// commit io
		qcow2_commit_writev(event);
		++commit_count;

		IoEvent_destroy(event);
	}
	
	// repeat timer
	timer_mod(ioTimer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + CLEANUP_PERIOD);
}

#elif defined(MV_IO_activity) 

static inline bool should_cleanup(void) 
{
	return io_count < CLEANUP_COUNT_THRESHOLD && io_size < CLEANUP_SIZE_THRESHOLD;
}

static void cleanup_timer(void *arg)
{	
	MV_DEBUG("queue:%d count:%d size:%ld\n", ioQueueCount, io_count, io_size);
	cleanup_print();

	if (should_cleanup()) { // disk is idle or queue is full ...
		int commit_count = 0;
		
		if (IoQueue_has_item(ioQueue))
			++cleanup_count;

		while (IoQueue_has_item(ioQueue) && commit_count < CLEANUP_BATCH_SIZE) {
			// pop queue				
			//pthread_mutex_lock(&ioQueueMutex);
			IoEvent *event = IoQueue_pop(ioQueue);
			//pthread_mutex_unlock(&ioQueueMutex);
			
			// commit io
			qcow2_commit_writev(event);
			++commit_count;

			IoEvent_destroy(event);
		}
	}

	io_count = io_size = 0; // reset io counter

	// repeat timer
	timer_mod(ioTimer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + CLEANUP_PERIOD);
}

#else // no cleanup

static void cleanup_timer(void *arg) {}
static void log_timer(void *arg) {}

#endif // MV_IO_period

// To avoid compiler warning: unused vars, functions, etc when debugging
static inline void avoid_compiler_warning(void) {

	nfs_disk = nfs_disk;
	nfs_fill = nfs_fill;
	ioQueue = ioQueue;
	ioQueueCount = ioQueueCount;
	ioQueueMutex = ioQueueMutex;
	ioTimer = ioTimer;
	logTimer = logTimer;
	log_file = log_file;
	cleanup_count = cleanup_count;
	tmp_fill = tmp_fill;

	io_size = io_size;
	io_count = io_count;

	qemu_iovec_create_copy(NULL);
	qemu_iovec_destroy_copy(NULL);
	qemu_iovec_print(NULL);

	//acb_print(NULL);

	IoEvent_create(0, 0, NULL, NULL, 0, NULL, NULL); 
	IoEvent_destroy(NULL);

	IoQueue_create();
	IoQueue_destroy(NULL);
	IoQueue_empty(NULL);
	IoQueue_push(NULL, NULL);
	IoQueue_pop(NULL);

	cleanup_thread(NULL);
	cleanup_coroutine(NULL);
	cleanup_timer(NULL);
	log_timer(NULL);

	qcow2_commit_writev(NULL);
} 
