/** @author morenvino@xxxxxxxx.xxx */

MV_TRACE();
if (MV_IS_HD1(bs)) {

#ifdef MV_IO_cleanup
	while (IoQueue_has_item(ioQueue)) {
		// pop queue				
		IoEvent *event = IoQueue_pop(ioQueue);
		IoEvent_destroy(event);
	}

	fprintf(log_file, "end\n"); fflush(log_file);
	fclose(log_file);
#endif
}
