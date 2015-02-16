/** @author morenvino@xxxxxxxx.xxx */

//MV_TRACE_HD1(bs);
//MV_DEBUG("%s\n", bs->device_name);
BlockDriverState *bs = aiocb->bs;

// default: do nothing
#ifdef MV_IO_default 
#endif //MV_IO_default

// keep track of write_count
if (MV_SHOULD_TRACE(bs)) {
	++io_count;
	io_size += aiocb->aio_nbytes;

	//MV_DEBUG("%s\n", bs->device_name);
}

// remove: drop random write
#ifdef MV_IO_remove 
if (MV_SHOULD_TRACE(bs)) { 
	unsigned int bytes = aiocb->aio_nbytes;
	if (bytes <= RAND_WR_SIZE) { 
		return bytes; //return bytes;
	}
}
#endif // MV_IO_remove

// shelter: shelter random write to NFS
#ifdef  MV_IO_shelter 
if (MV_SHOULD_TRACE(bs)) {
	unsigned int bytes = aiocb->aio_nbytes;
	if (bytes <= RAND_WR_SIZE) {
		int err = write(nfs_disk, nfs_fill, bytes); 
		if (err == -1) fprintf(stderr, "Error write to NFS: %m\n");

// queue: queue random write in memory
#ifdef MV_IO_queue 
		//IoEvent *event = IoEvent_create(sector_num, remaining_sectors, bs, qiov, 0, 0, 0);
		//IoEvent *event = IoEvent_create(aiocb->aio_offset, aiocb->aio_nbytes, bs, qiov, 0, 0, 0);
		IoEvent *event = IoEvent_create(aiocb->aio_offset, aiocb->aio_nbytes, bs, 0, 0, 0, 0);
		pthread_mutex_lock(&ioQueueMutex); //qemu_co_mutex_lock(&ioQueueCoMutex);
		IoQueue_push(ioQueue, event);
		pthread_mutex_unlock(&ioQueueMutex); //qemu_co_mutex_unlock(&ioQueueCoMutex);
		//printf("+\n");
#endif // MV_IO_queue

		return bytes;
	}
}

#endif // MV_IO_shelter

// shelterall: shelter all write to NFS
#ifdef  MV_IO_shelterall 
if (MV_SHOULD_TRACE(bs)) {
	unsigned int bytes = aiocb->aio_nbytes;
	int err = write(nfs_disk, nfs_fill, bytes); 
	if (err == -1) fprintf(stderr, "Error write to NFS: %m\n");
	return bytes;
}

#endif // MV_IO_shelterall
  