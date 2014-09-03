/** @author morenvino@xxxxxxxx.xxx */

//MV_TRACE_HD1(bs);
#define MV_IO_shelter
#define MV_IO_queue

// default: do nothing
#ifdef MV_IO_default 
#endif //MV_IO_default

// remove: drop random write
#ifdef MV_IO_remove 
if (MV_IS_HD1(bs)) { 
	unsigned int bytes = remaining_sectors << BDRV_SECTOR_BITS;
	if (bytes < RAND_WR_SIZE) { 
		return 0; //return bytes;
	}        
}
#endif // MV_IO_remove

// shelter: shelter random write to NFS
#ifdef  MV_IO_shelter 
if (MV_IS_HD1(bs)) {
	unsigned int bytes = remaining_sectors << BDRV_SECTOR_BITS;
	if (bytes < RAND_WR_SIZE) {
		int err = write(nfs_disk, nfs_fill, bytes); 
		if (err == -1) fprintf(stderr, "Error write to NFS: %m\n");

// queue: queue random write in memory
#ifdef MV_IO_queue 
		IoEvent *event = IoEvent_create(sector_num, remaining_sectors, bs, qiov, 0, 0, 0);
		pthread_mutex_lock(&ioQueueMutex); //qemu_co_mutex_lock(&ioQueueCoMutex);
		IoQueue_push(ioQueue, event);
		pthread_mutex_unlock(&ioQueueMutex); //qemu_co_mutex_unlock(&ioQueueCoMutex);
#endif // MV_IO_queue

		return 0;
	}
}
#endif // MV_IO_shelter
  