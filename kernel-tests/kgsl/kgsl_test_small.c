/******************************************************************************
-----------------------------------------------------------------------------
 Copyright (c) 2011-2012 Qualcomm Technologies, Inc.
 All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
******************************************************************************/
#include "kgsl_helper.h"


/*simple open/close of gpu fd*/
int kgsl_test_openclose()
{
	int rv = 0, fd = 0;
	TEST_STEP(0, rv, msm_kgsl_open(&fd, DEF_MODE), "---OPEN---\n");
	TEST_STEP(0, rv, msm_kgsl_close(fd), "---CLOSE---\n");
	return rv;
}

/*Does simple getprops operation.
 * Good for 'helloworld' of testing framework
 */
int kgsl_test_simple_getprops()
{
	int fd = 0, rv = 0;
	TEST_STEP(0, rv, msm_kgsl_open(&fd, DEF_MODE), "");
	TEST_STEP(0, rv, kgsl_test_getprops(fd), "---PROPS---\n");
	TEST_STEP(0, rv, msm_kgsl_close(fd), "");
	return rv;
}

/*Use to getprops of a certain fd*/
int kgsl_test_getprops(int fd)
{
	int rv = 0;
	rv |= msm_getprop_version_ioctl(fd);
	rv |= msm_getprop_shadow_ioctl(fd);
	rv |= msm_getprop_info_ioctl(fd);
	return rv;
}

/*Simple create & destroy of a ctxt*/
int kgsl_test_create_destroy()
{
	int rv = 0, fd = 0;
	unsigned int c_id = 0;
	TEST_STEP(0, rv, msm_kgsl_open(&fd, DEF_MODE), "---OPEN---\n");
	TEST_STEP(0, rv, msm_ctxtcreate_ioctl(fd, 0, &c_id),
		  "---CREATE CTXT---\n");
	TEST_STEP(0, rv, msm_ctxtdelete_ioctl(fd, c_id),
		  "---DESTROY CTXT---\n");
	TEST_STEP(0, rv, msm_kgsl_close(fd), "---CLOSE---\n");
	return 0;
}

/*checks the last issued ts with given ts*/
int kgsl_test_checkts(int fd, unsigned int ts)
{
	int rv = 0;
	unsigned int last_ts;

	/*wait for ts */
	rv |= msm_waitts_ioctl(fd, ts, KGSL_TEST_TIMEOUT);

	/*read last timestamp */
	kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE, "---READ REC TS---\n");
	TEST_STEP(0, rv,
		  msm_cmds_read_ts_ioctl(fd, KGSL_TIMESTAMP_CONSUMED, &last_ts),
		  "");

	/*check ts's */
	if (last_ts - ts > KGSL_TEST_EPSILON) {
		kgsl_test_log(KGSL_LOG_LEVEL_ERROR,
			      "***ERROR MISMATCH TS'S***\n");
		kgsl_test_logd(KGSL_LOG_LEVEL_ERROR, "---ISSUEIB TS---", ts);
		kgsl_test_logd(KGSL_LOG_LEVEL_ERROR, "---CMDREAD TS---",
			       last_ts);
		rv = KGSL_TEST_FAILURE;
	}

	return rv;
}

/*allocs gpu mem, maps into user, builds a simple cmd packet
 * and issues the cmd.
 * Returns gpuaddr, ts and userspace location where it was mapped
 */
int kgsl_test_simplecmd(int fd, size_t size, unsigned int flags,
			unsigned int *gpuaddr, unsigned int c_id,
			unsigned int *ts, unsigned int **cmds, int numcmds)
{
	int rv = 0;
	struct kgsl_ibdesc ib;
	memset(&ib, 0, sizeof(ib));

	/*alloc gpu mem */
	TEST_STEP(0, rv, msm_gpualloc_ioctl(fd, size, flags, gpuaddr),
		  "---GPU ALLOC---\n");
	/*map gpu memory into userspace */
	TEST_STEP(0, rv,
		  msm_mmap((size_t) size, fd, cmds, *gpuaddr, MAP_SHARED),
		  "---MAP GPU INTO USER---\n");
	if (*cmds == NULL)
		return KGSL_TEST_FAILURE;
	memset(*cmds, 0, size);
	/*link and build cmdbuf */
	TEST_STEP(0, rv, msm_create_simplebuf(*cmds, numcmds), "");
	/*issue cmd */
	ib.gpuaddr = *gpuaddr;
	ib.sizedwords = numcmds;
	ib.hostptr = *cmds;
	TEST_STEP(0, rv,
		  msm_rb_issueib_ioctl(fd, c_id, KGSL_CONTEXT_SUBMIT_IB_LIST,
				       &ib, 1, ts), "---ISSUE CMD---\n");
	TEST_STEP(0, rv, msm_waitts_ioctl(fd, *ts, KGSL_TEST_TIMEOUT), "");
	return rv;
}

/*Unmaps user_ptr from user and unmaps gpuaddr from gpu*/
int kgsl_test_unmapgpu(int fd, size_t size, unsigned int *user_ptr,
		       unsigned int gpuaddr)
{
	int rv = 0;
	/*unmap gpu */
	TEST_STEP(0, rv, msm_munmap(user_ptr, size), "---UNMAP MEM---\n");
	/*remove gmem1 from gpu */
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr),
		  "---FREE GPU MEM---\n");

	return rv;
}

/*Allocs from pmem into *dest in userspace
 * maps dest to gpuaddr in gpu using map_user_mem ioctl
 * writes to dest
 * frees gpuaddr, unmaps dest and frees pmem
 */
int kgsl_test_mapuser(int fd)
{
	int rv = 0, pmem_fd = 0;
	unsigned int gpuaddr = 0, len, numcmds, *dest = NULL;
	len = KGSL_PAGESIZE;
	numcmds = DEF_NUMCMDS;

	kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE, "---BEG MAP SHARE TEST---\n");

	/*open PMEM_DEV */
	kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE,"---OPEN PMEM---\n");
	rv =  msm_pmem_open(&pmem_fd, DEF_MODE);
	/*check if this device is PMEM enabled, if not PMEM enabled return with success*/
	if (rv == NON_PMEM_DEVICE)
	{
		printf("This is not a PMEM enabled device\n");
		return 0;
	}
	if (rv != 0) { 
		kgsl_test_exit(rv, 0, "---OPEN PMEM---\n");
	}

	/*alloc mem from pmem */
	TEST_STEP(0, rv, msm_kgslpmem_alloc(pmem_fd, len, &dest),
		  "---PMEM ALLOC---\n");
	/*map user's pmem into gpu */
	TEST_STEP(0, rv, msm_mapusermem_ioctl(fd, pmem_fd, len, 0, dest,
					      KGSL_USER_MEM_TYPE_PMEM,
					      &gpuaddr),
		  "---MAP USER INTO GPU---\n");
	/*write to the memory */
	TEST_STEP(0, rv, msm_create_simplebuf(dest, numcmds), "");
	/*free gpu block */
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr),
		  "---UNMAP USER MEM---\n");
	/*free pmem */
	TEST_STEP(0, rv, msm_kgslpmem_free(dest, len), "---FREE PMEM---\n");
	/*close pmem_fd */
	TEST_STEP(0, rv, msm_kgsl_close(pmem_fd), "---CLOSE PMEM---");
	kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE, "---END MAP SHARE---\n");

	return rv;
}

/*Tests all the ioctls except sharedmempmem & vmalloc
 * It has a lot of logging to make it easy to walk through
 */
int kgsl_test_simple(int count)
{
	int fd = 0, rv = 0, i = 0, numcmds;
	unsigned int ts, c_id1, gpuaddr, gsize, gflags = 0, *cmds = NULL;

	numcmds = DEF_NUMCMDS;
	gsize = KGSL_PAGESIZE;
	gflags |= KGSL_MEMFLAGS_GPUREADONLY;

	for (i = 0; i < count; i++) {
		kgsl_test_logd(KGSL_LOG_LEVEL_VERBOSE, "@@@ITERATION: ", i);
		TEST_STEP(0, rv, msm_kgsl_open(&fd, DEF_MODE),
			  "---OPEN KGSL---\n");
		TEST_STEP(0, rv, msm_ctxtcreate_ioctl(fd, 0, &c_id1),
			  "---CREATE CTXT---\n");

		TEST_STEP(0, rv,
			  kgsl_test_simplecmd(fd, gsize, gflags, &gpuaddr,
					      c_id1, &ts, &cmds, numcmds),
			  "---BEGIN SIMPLECMD TEST---\n");
		TEST_STEP(0, rv, kgsl_test_checkts(fd, ts), "---CHECK TS---\n");
		TEST_STEP(0, rv, msm_sharedmem_flush_cache_ioctl(fd, gpuaddr),
			  "---FLUSH CACHE---\n");
		TEST_STEP(0, rv, kgsl_test_unmapgpu(fd, gsize, cmds, gpuaddr),
			  "");
		kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE, "---END SIMPLECMD---\n");

		/*free mem test */
		TEST_STEP(0, rv,
			  kgsl_test_simplecmd(fd, gsize, gflags, &gpuaddr,
					      c_id1, &ts, &cmds, numcmds),
			  "---BEG TS FREEMEM TEST---\n");
		TEST_STEP(0, rv, msm_munmap(cmds, fd), "");
		TEST_STEP(0, rv, msm_cmds_free_on_ts_ioctl(fd, gpuaddr,
							   KGSL_TIMESTAMP_CONSUMED,
							   ts),
			  "---FREEMEMONTIMESTAMP---\n");

		kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE,
			      "---END TS FREEMEM TEST---\n");

		/*SHARED, PMEM and Map */
		TEST_STEP(0, rv, kgsl_test_mapuser(fd), "");

		/*BIN BASE OFFSET */
		TEST_STEP(0, rv, msm_drawctxt_bin_base_ioctl(fd, c_id1, 0),
			  "---BIN BASE---\n");
		/*delete ctxt */
		TEST_STEP(0, rv, msm_ctxtdelete_ioctl(fd, c_id1),
			  "---DELETE CTXT---\n");
		/*close fd */
		TEST_STEP(0, rv, msm_kgsl_close(fd), "---CLOSE KGSL---\n");
	}

	return rv;
}
