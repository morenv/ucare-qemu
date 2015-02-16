/** @author morenvino@xxxxxxxx.xxx */

MV_DEBUG("dev:%s file:%s, format:%s\n\n", bs->device_name, bs->filename, bs->drv->format_name);

//if (MV_IS_HD1(bs)) {
if (MV_SHOULD_TRACE(bs)) {
	
#if defined(MV_IO_shelter) || defined(MV_IO_shelterall)
	// create nfs disk
	//MV_DEBUG("opening NFS disk\n");
	nfs_disk = open("/mnt/nfsdisk", O_WRONLY | O_DIRECT | O_SYNC);  
	if (nfs_disk == -1) fprintf(stderr, "Error opening nfs disk: %m\n"); 
		
	nfs_fill = malloc(RAND_WR_SIZE);
	if (nfs_fill == NULL) fprintf(stderr, "Error allocating nfs fill\n"); 

#endif //MV_IO_shelter

#ifdef MV_IO_queue

	// create io queue
	ioQueue = IoQueue_create();
	if (ioQueue == NULL) fprintf(stderr, "Error creating io queue\n"); 

#endif //MV_IO_queue

#ifdef MV_IO_cleanup

	// create cleanup thread
	//MV_DEBUG("creating cleanup thread\n");
	if (posix_memalign(&tmp_fill, 512, RAND_WR_SIZE)) 
		fprintf(stderr, "Error allocating tmp fill\n"); 

	pthread_t t; pthread_create(&t, NULL, cleanup_thread, NULL);
	
	// create cleanup coroutine
	//qemu_co_mutex_init(&ioQueueCoMutex);
	//Coroutine *co = qemu_coroutine_create(cleanup_coroutine);
	//qemu_coroutine_enter(co, NULL);

	// create log file
	//log_file = fopen("/mnt-tmp/qemu.log", "a");
	//fprintf(log_file, "begin\n");
	//fflush(log_file);

	// create cleanup timer
	//ioTimer = timer_new_ms(QEMU_CLOCK_VIRTUAL, cleanup_timer, NULL);
	//timer_mod(ioTimer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + CLEANUP_PERIOD);

	//logTimer = timer_new_ms(QEMU_CLOCK_VIRTUAL, log_timer, NULL);
	//timer_mod(logTimer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + LOG_PERIOD);

#endif // MV_IO_cleanup

}    
