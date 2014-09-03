/** @author morenvino@xxxxxxxx.xxx */

MV_DEBUG("drv:%s, file:%s\n", bs->device_name, bs->filename);

if (MV_IS_HD1(bs)) {
	
//#ifdef MV_IO_shelter

	// create nfs disk
	nfs_disk = open("/mnt/nfsdisk", O_WRONLY | O_DIRECT | O_SYNC);  
	if (nfs_disk == -1) fprintf(stderr, "Error opening nfs disk: %m\n"); 
	    
	nfs_fill = malloc(RAND_WR_SIZE);
	if (nfs_fill == NULL) fprintf(stderr, "Error allocating nfs fill\n"); 

//#endif //MV_IO_shelter

	// create io queue
	ioQueue = IoQueue_create();
	if (ioQueue == NULL) fprintf(stderr, "Error creating io queue\n"); 

	// create cleanup thread
	//pthread_t t; pthread_create(&t, NULL, cleanup_thread, NULL);
	
	// create cleanup coroutine
	//qemu_co_mutex_init(&ioQueueCoMutex);
	//Coroutine *co = qemu_coroutine_create(cleanup_coroutine);
  //qemu_coroutine_enter(co, NULL);

	// create cleanup timer
  ioTimer = timer_new_ms(QEMU_CLOCK_VIRTUAL, cleanup_timer, NULL);
  timer_mod(ioTimer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + CLEANUP_PERIOD);
}    
