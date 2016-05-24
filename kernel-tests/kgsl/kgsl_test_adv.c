/******************************************************************************
-----------------------------------------------------------------------------
 Copyright (c) 2011-2012 Qualcomm Technologies, Inc.
 All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
******************************************************************************/
#include "kgsl_helper.h"

/*Tests GETPROPERTY IOCTL by
 * sending bad sizes
 * wrong types
 * invalid fds
 */
#if CHECK_VERSION(3, 1)
static int kgsl_test_adv_getprop()
{
	int i = 0, rv = 0, fd = 0;
	struct kgsl_device_getproperty prop;

	memset(&prop, 0, sizeof(prop));
	msm_kgsl_open(&fd, DEF_MODE);

	TEST_STEP(EFAULT, rv,
		  msm_kgsl_ioctl(fd, IOCTL_KGSL_DEVICE_GETPROPERTY, NULL, 0),
		  "[--NULL PARAM--]\n");
	TEST_STEP(EINVAL, rv,
		  msm_kgsl_ioctl(fd, IOCTL_KGSL_DEVICE_GETPROPERTY,
				 (void *)&prop, sizeof(prop)),
		  "[--BLANK PARAM--]\n");
	prop.type = KGSL_PROP_VERSION;

	for (i = 0; i < 3; i++) {
		prop.sizebytes = 0;
		TEST_STEP(EINVAL, rv,
			  msm_kgsl_ioctl(fd, IOCTL_KGSL_DEVICE_GETPROPERTY,
					 (void *)&prop, sizeof(prop)),
			  "[--BLANK SIZE--]\n");
		prop.sizebytes = sizeof(int);
		TEST_STEP(EINVAL, rv,
			  msm_kgsl_ioctl(fd, IOCTL_KGSL_DEVICE_GETPROPERTY,
					 (void *)&prop, sizeof(prop)),
			  "[--WRONG SIZE--]\n");
		if (!i)
			prop.sizebytes = sizeof(struct kgsl_version);
		else if (i == 1)
			prop.sizebytes = sizeof(struct kgsl_shadowprop);
		else
			prop.sizebytes = sizeof(struct kgsl_devinfo);
		TEST_STEP(EFAULT, rv,
			  msm_kgsl_ioctl(fd, IOCTL_KGSL_DEVICE_GETPROPERTY,
					 (void *)&prop, sizeof(prop)),
			  "[--INVALID PTR--]\n");
		if (i == 1)
			prop.type = KGSL_PROP_DEVICE_INFO;
		else
			prop.type = KGSL_PROP_DEVICE_SHADOW;
	}
	prop.type = 0xA;
	TEST_STEP(EINVAL, rv,
		  msm_kgsl_ioctl(fd, IOCTL_KGSL_DEVICE_GETPROPERTY,
				 (void *)&prop, sizeof(prop)),
		  "[--INVALID TYPE--]\n");
	TEST_STEP(0, rv, msm_kgsl_close(fd), "");

	return rv;
}
#else
static int kgsl_test_adv_getprop()
{
	kgsl_test_log(KGSL_LOG_LEVEL_ERROR, "kgsl_test_adv_getprop skipped\n");
	return 0;
}
#endif

/*Tests ctxt create & delete ioctl by
 * mixing ctxts and fds
 * double delete ctxts
 */
static int kgsl_test_adv_ctxt_simple()
{
	int rv, cflags = 0, i;
	int fd[DEF_ARRAY_SIZE];
	int *fd_ptr;
	unsigned int c_id[DEF_ARRAY_SIZE];
	unsigned int *c_ptr;
	fd_ptr = fd;
	c_ptr = c_id;
	memset(fd, 0, DEF_ARRAY_SIZE * sizeof(int));
	memset(c_id, 0, DEF_ARRAY_SIZE * sizeof(unsigned int));
	/*try ctxts without any fd's */
	for (i = 0; i < DEF_ARRAY_SIZE; i++) {
		TEST_STEP(EINVAL, rv,
			  msm_ctxtcreate_ioctl(*fd_ptr++, cflags, c_ptr++),
			  "[--NO FD CTXT--]\n");
	}
	fd_ptr = fd;
	c_ptr = c_id;
	/*open fd's */
	for (i = 0; i < DEF_ARRAY_SIZE; i++)
		msm_kgsl_open(fd_ptr++, DEF_MODE);
	fd_ptr = fd;
	/*create ctxts */
	for (i = 0; i < DEF_ARRAY_SIZE; i++) {
		TEST_STEP(0, rv,
			  msm_ctxtcreate_ioctl(*fd_ptr++, cflags, c_ptr++),
			  "[--PROPER CTXTS--]\n");
	}
	fd_ptr = fd;
	c_ptr = &c_id[DEF_ARRAY_SIZE - 1];
	for (i = 0; i < DEF_ARRAY_SIZE; i++) {
		TEST_STEP(EINVAL, rv, msm_ctxtdelete_ioctl(*fd_ptr++, *c_ptr--),
			  "[--BAD DELETE--]\n");
	}
	fd_ptr = fd;
	c_ptr = c_id;

	for (i = 0; i < DEF_ARRAY_SIZE; i++) {
		TEST_STEP(0, rv, msm_ctxtdelete_ioctl(*fd_ptr++, *c_ptr++),
			  "[--GOOD DELETE--]\n");
	}
	fd_ptr = fd;
	c_ptr = c_id;
	for (i = 0; i < DEF_ARRAY_SIZE; i++) {
		TEST_STEP(EINVAL, rv, msm_ctxtdelete_ioctl(*fd_ptr++, *c_ptr++),
			  "[--DOUBLE DELETE--]\n");
	}

	fd_ptr = fd;
	for (i = 0; i < DEF_ARRAY_SIZE; i++) {
		TEST_STEP(0, rv, msm_kgsl_close(*fd_ptr++),
			  "[--GOOD CLOSE--]\n");
	}

	return rv;
}

/*Tests bin base on
 * no fd, no ctxt, deleted ctxt
 */
static int kgsl_test_adv_binbase()
{
	int rv = 0, fd = 0;
	unsigned int c_id = 0;

	TEST_STEP(EINVAL, rv, msm_drawctxt_bin_base_ioctl(fd, c_id, 0),
		  "[--NO FD--]\n");
	TEST_STEP(0, rv, msm_kgsl_open(&fd, DEF_MODE), "");
	TEST_STEP(EINVAL, rv, msm_drawctxt_bin_base_ioctl(fd, c_id, 0),
		  "[--NO CID--]\n");
	TEST_STEP(0, rv, msm_ctxtcreate_ioctl(fd, 0, &c_id), "");
	TEST_STEP(0, rv, msm_ctxtdelete_ioctl(fd, c_id), "");
	TEST_STEP(EINVAL, rv, msm_drawctxt_bin_base_ioctl(fd, c_id, 0),
		  "[--NO CID--]\n");
	TEST_STEP(0, rv, msm_kgsl_close(fd), "");

	return rv;
}

/*Tests flushs cache ioctl on
 * invalid gpu, free'd gpu
 */
static int kgsl_test_adv_flush_cache()
{
	int rv = 0, fd = 0, gsize = 0;
	unsigned int gflags = 0, gpuaddr = 0;
	gsize = KGSL_PAGESIZE;
	gflags = KGSL_MEMFLAGS_GPUREADONLY;
	msm_kgsl_open(&fd, DEF_MODE);
	TEST_STEP(EINVAL, rv, msm_sharedmem_flush_cache_ioctl(fd, gpuaddr),
		  "[--INVALID GPUADDR--]\n");
	gpuaddr = 0x30000000;
	TEST_STEP(EINVAL, rv, msm_sharedmem_flush_cache_ioctl(fd, gpuaddr),
		  "[--INVALID GPUADDR--]\n");
	TEST_STEP(0, rv, msm_gpualloc_ioctl(fd, gsize, gflags, &gpuaddr), "");
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr), "");
	TEST_STEP(EINVAL, rv, msm_sharedmem_flush_cache_ioctl(fd, gpuaddr),
		  "[--FLUSH AFTER FREE--]\n");
	TEST_STEP(0, rv, msm_kgsl_close(fd), "");
	return rv;
}

/*Tests sharedmemfree on
 * uninit gpu addr, already free'd gpuaddr
 */
static int kgsl_test_adv_sharedmem_free()
{
	int rv = 0, fd = 0, gsize = 0;
	unsigned int gflags = 0, gpuaddr = 0;
	gsize = KGSL_PAGESIZE;
	gflags = KGSL_MEMFLAGS_GPUREADONLY;
	msm_kgsl_open(&fd, DEF_MODE);
	TEST_STEP(EINVAL, rv, msm_sharedmemfree_ioctl(fd, gpuaddr),
		  "[--INVALID MEM--]\n");
	gpuaddr = 0x30000000;
	TEST_STEP(EINVAL, rv, msm_sharedmemfree_ioctl(fd, gpuaddr),
		  "[--INVALID MEM--]\n");
	TEST_STEP(0, rv, msm_gpualloc_ioctl(fd, gsize, gflags, &gpuaddr), "");
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr),
		  "[--NORMAL FREE--]\n");
	TEST_STEP(EINVAL, rv, msm_sharedmemfree_ioctl(fd, gpuaddr),
		  "[--DOUBLE FREE--]\n");
	TEST_STEP(0, rv, msm_kgsl_close(fd), "");
	return rv;
}

/*Tests freememonts ioctl by
 *using invalid gpuaddr and ts already passed
 *issuing simplecmds and trying to freemem on already free'd gpu addr
 */
#define FMTS_SIZE 3
static int kgsl_test_adv_freememonts()
{
	int i = 0, rv = 0, fd = 0, gsize = 0, type = 0;
	unsigned int gpuaddr[FMTS_SIZE], c_id[FMTS_SIZE], ts[FMTS_SIZE], gflags,
	    *user_ptr[FMTS_SIZE];
	memset(gpuaddr, 0, sizeof(unsigned int) * FMTS_SIZE);
	memset(c_id, 0, sizeof(unsigned int) * FMTS_SIZE);
	memset(ts, 0, sizeof(unsigned int) * FMTS_SIZE);
	memset(user_ptr, 0, sizeof(unsigned int) * FMTS_SIZE);
	gflags = 0;
	gflags = KGSL_MEMFLAGS_GPUREADONLY;
	gsize = KGSL_PAGESIZE;
	type = KGSL_TIMESTAMP_RETIRED;

	msm_kgsl_open(&fd, DEF_MODE);
	for (i = 0; i < FMTS_SIZE; i++)
		msm_ctxtcreate_ioctl(fd, 0, &c_id[i]);
	TEST_STEP(EINVAL, rv,
		  msm_cmds_free_on_ts_ioctl(fd, gpuaddr[0], type, 0),
		  "[--INVALID ADDR & TS--]\n");
	TEST_STEP(0, rv, msm_gpualloc_ioctl(fd, gsize, gflags, &gpuaddr[0]),
		  "");
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr[0]),
		  "[--NORMAL FREE--]\n");
	TEST_STEP(EINVAL, rv,
		  msm_cmds_free_on_ts_ioctl(fd, gpuaddr[0], type, 0),
		  "[--MEM IN A DIFF LIST--]\n");
	TEST_STEP(0, rv,
		  kgsl_test_simplecmd(fd, gsize, gflags, &gpuaddr[1], c_id[0],
				      &ts[0], &user_ptr[1], 2),
		  "[--NORMAL SUBMITIB--]\n");
	TEST_STEP(0, rv, kgsl_test_unmapgpu(fd, gsize, user_ptr[1], gpuaddr[1]),
		  "[--NORMAL FREE--]\n");
	TEST_STEP(EINVAL, rv,
		  msm_cmds_free_on_ts_ioctl(fd, gpuaddr[1], type, ts[0]),
		  "[--DOUBLE FREE--]\n");

	TEST_STEP(0, rv, kgsl_test_simplecmd(fd, gsize, gflags, &gpuaddr[2],
					     c_id[1], &ts[1], &user_ptr[2], 2),
		  "[--NORMAL SUBMITIB--]\n");
	TEST_STEP(0, rv, msm_munmap(user_ptr[2], gsize),
		  "[--NORMAL UNMAP--]\n");
	TEST_STEP(0, rv, msm_cmds_free_on_ts_ioctl(fd, gpuaddr[2], type, ts[1]),
		  "[--NORMAL TS FREE--]\n");
	TEST_STEP(EINVAL, rv,
		  msm_cmds_free_on_ts_ioctl(fd, gpuaddr[2], type, ts[1]),
		  "[--DOUBLE TS FREE-]\n");

	for (i = 0; i < FMTS_SIZE; i++)
		msm_ctxtdelete_ioctl(fd, c_id[i]);
	TEST_STEP(0, rv, msm_kgsl_close(fd), "");
	return rv;
}

/*Tests issueib ioctl by
 * using invalid ctxt,fd ibdesc, and gpuaddr params
 * Does not test hanging and recovering
 */
static int kgsl_test_adv_issueib()
{
	int rv = 0, numcmds = 0, fd = 0;
	unsigned int gpuaddr1 = 0, gsize, gflags =
	    0;
	unsigned int *user_ptr1 = NULL;
	unsigned int c_id1 = 0, c_id2 = 0, c_id3 = 0;
	unsigned int ts1 = 0, ts2 = 0;
	struct kgsl_ibdesc ib;
	memset(&ib, 0, sizeof(ib));
	gsize = KGSL_PAGESIZE;
	gflags |= KGSL_MEMFLAGS_GPUREADONLY;

	msm_kgsl_open(&fd, DEF_MODE);
	TEST_STEP(EINVAL, rv, msm_rb_issueib_ioctl(fd, c_id1,
						   KGSL_CONTEXT_SUBMIT_IB_LIST,
						   &ib, 1, &ts1),
		  "[--BLANK CTXT--]\n");
	msm_ctxtcreate_ioctl(fd, 0, &c_id1);
	TEST_STEP(EINVAL, rv, msm_rb_issueib_ioctl(fd, c_id1,
						   KGSL_CONTEXT_SUBMIT_IB_LIST,
						   &ib, 0, &ts1),
		  "[--0 IB--]\n");
	TEST_STEP(EINVAL, rv,
		  msm_rb_issueib_ioctl(fd, c_id1, KGSL_CONTEXT_SUBMIT_IB_LIST,
				       &ib, 1, &ts1), "[--NO MEM--]\n");
	TEST_STEP(EINVAL, rv, msm_rb_issueib_ioctl(fd, c_id1, 0, &ib, 0, &ts1),
		  "[--NO MEM SINGLE IB--]\n");
	TEST_STEP(EFAULT, rv,
		  msm_rb_issueib_ioctl(fd, c_id1, KGSL_CONTEXT_SUBMIT_IB_LIST,
				       NULL, 1, &ts1), "[--NO IBDESCR--]\n");

	msm_ctxtcreate_ioctl(fd, 0, &c_id2);
	msm_ctxtcreate_ioctl(fd, 0, &c_id3);

	numcmds = 2;

	TEST_STEP(0, rv, kgsl_test_simplecmd(fd, gsize, gflags, &gpuaddr1,
					     c_id3, &ts2, &user_ptr1, numcmds),
		  "[--NORMAL IB--]\n");

	TEST_STEP(0, rv, kgsl_test_unmapgpu(fd, gsize, user_ptr1, gpuaddr1),
		  "[--FREE MEM3--]\n");
	ib.gpuaddr = gpuaddr1;
	ib.sizedwords = 2;
	TEST_STEP(EINVAL, rv, msm_rb_issueib_ioctl(fd, c_id3,
						   KGSL_CONTEXT_SUBMIT_IB_LIST,
						   &ib, 1, &ts2),
		  "[--FREED MEM IB--]\n");

	msm_ctxtdelete_ioctl(fd, c_id1);
	msm_ctxtdelete_ioctl(fd, c_id2);
	msm_ctxtdelete_ioctl(fd, c_id3);
	TEST_STEP(EINVAL, rv, msm_rb_issueib_ioctl(fd, c_id3,
						   KGSL_CONTEXT_SUBMIT_IB_LIST,
						   &ib, 1, &ts2),
		  "[--DELETED CTXT--]\n");
	TEST_STEP(0, rv, msm_kgsl_close(fd), "");
	return rv;
}

static int kgsl_test_adv_badparams()
{
	int rv = 0;

	/*props */
	TEST_STEP(0, rv, kgsl_test_adv_getprop(), "|---INVALID PROPS---|\n");
	/*ctxts */
	TEST_STEP(0, rv, kgsl_test_adv_ctxt_simple(),
		  "|---INVALID CTXTS---|\n");
	/*bin base */
	TEST_STEP(0, rv, kgsl_test_adv_binbase(), "|---INVALID BIN BASE---|\n");

	/*flush cache */
	TEST_STEP(0, rv, kgsl_test_adv_flush_cache(),
		  "|---INVALID FLUSH---|\n");
	/*sharedmemfree */
	TEST_STEP(0, rv, kgsl_test_adv_sharedmem_free(),
		  "|---INVALID FREE---|\n");
	/*free on ts */
	TEST_STEP(0, rv, kgsl_test_adv_freememonts(),
		  "|---INVALID FREE ON TS---|\n");
	/*issueib */
	TEST_STEP(0, rv, kgsl_test_adv_issueib(), "|---INVALID ISSUEIB---|\n");
	return rv;
}

/*Tests sharedpmem ioctl*/
static int kgsl_test_adv_sharepmem(int fd)
{
	int rv = 0, pmem_fd = 0;
	unsigned int gpuaddr0 = 0, gpuaddr1 = 0, *dest = NULL, len;
	len = KGSL_PAGESIZE;

	kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE, "---OPEN PMEM---\n");

	rv = msm_pmem_open(&pmem_fd, DEF_MODE);
	if (rv == NON_PMEM_DEVICE)
	{
		printf("This is not a PMEM enabled device\n");
		return 0;
	}
	if (rv != 0) { 
		kgsl_test_exit(rv, 0,  "---OPEN PMEM---\n"); \
	}

	TEST_STEP(0, rv, msm_kgslpmem_alloc(pmem_fd, len, &dest),
		  "---PMEM ALLOC---\n");
	TEST_STEP(0, rv,
		  msm_sharedpmem_ioctl(fd, pmem_fd, len, 0, &gpuaddr0),
		  "---MAP PMEM INTO GPU---\n");
	TEST_STEP(0, rv,
		  msm_sharedpmem_ioctl(fd, pmem_fd, len, 0, &gpuaddr1),
		  "---MAP PMEM INTO GPU---\n");
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr0),
		  "---FREE---\n");
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr1),
		  "---FREE---\n");
	TEST_STEP(0, rv, msm_kgslpmem_free(dest, len), "---UNMAP---\n");
	TEST_STEP(0, rv, msm_kgsl_close(pmem_fd), "---CLOSE PMEM---\n");

	return rv;
}

/*Tests mapuser ioctl pmem type only
 * tests invalid offset, size, and dest parameters
 * double frees and maps
 * need example from hp for addr memtype
 */
#define BMEM_SIZE 10
static int kgsl_test_adv_mapuser(int fd)
{
	int rv = 0, pfd = 0;
	unsigned int gpuaddr[BMEM_SIZE], c_id[BMEM_SIZE], gsize,
	    memtype, off;
	unsigned int *dest = NULL;
	memset(gpuaddr, 0, sizeof(unsigned int) * BMEM_SIZE);
	memset(c_id, 0, sizeof(unsigned int) * BMEM_SIZE);
	gsize = KGSL_PAGESIZE * 2;
	memtype = KGSL_USER_MEM_TYPE_MAX;
	off = 0;
	TEST_STEP(EINVAL, rv,
		  msm_mapusermem_ioctl(fd, pfd, gsize, off, dest, memtype,
				       &gpuaddr[0]),
		  "[--INVALID MEM TYPE--]\n");
	memtype = KGSL_USER_MEM_TYPE_PMEM;
	TEST_STEP(EINVAL, rv,
		  msm_mapusermem_ioctl(fd, pfd, gsize, off, dest, memtype,
				       &gpuaddr[0]), "[--NO PMEM FD--]\n");
	
	/*Check if this is a PMEM enabled device, if not return test with success*/
	if(msm_pmem_open(&pfd, DEF_MODE)==NON_PMEM_DEVICE)
	{
		printf("This is not a PMEM enabled device\n");
		return 0;
	}

	TEST_STEP(1, rv,
		  msm_mapusermem_ioctl(fd, pfd, gsize, off, dest, memtype,
				       &gpuaddr[0]), "[--NO PMEM MEM--]\n");
	TEST_STEP(0, rv, msm_kgslpmem_alloc(pfd, gsize, &dest), "");

	TEST_STEP(EINVAL, rv,
		  msm_mapusermem_ioctl(fd, pfd, 0, off, dest, memtype,
				       &gpuaddr[0]), "[--FULL MEM--]\n");

	TEST_STEP(EINVAL, rv,
		  msm_mapusermem_ioctl(fd, pfd, gsize, 1, dest, memtype,
				       &gpuaddr[0]), "[--1 PAST ALL MEM--]\n");

	off = 0xFFFFFFFF;
	TEST_STEP(EINVAL, rv,
		  msm_mapusermem_ioctl(fd, pfd, gsize, off, dest, memtype,
				       &gpuaddr[0]), "[--WRAP AROUND--]\n");
	TEST_STEP(EINVAL, rv,
		  msm_mapusermem_ioctl(fd, pfd, 2 * gsize, off, dest, memtype,
				       &gpuaddr[0]), "[--WRAP AROUND 2--]\n");
	off = 0xFFFF;
	TEST_STEP(EINVAL, rv,
		  msm_mapusermem_ioctl(fd, pfd, 2 * gsize, off, dest, memtype,
				       &gpuaddr[0]), "[--WRAP AROUND 3--]\n");

	off = 0;
	TEST_STEP(0, rv,
		  msm_mapusermem_ioctl(fd, pfd, gsize, 0, dest, memtype,
				       &gpuaddr[0]), "[--GOOD MAP--]\n");
	TEST_STEP(0, rv,
		  msm_mapusermem_ioctl(fd, pfd, gsize, 0, dest, memtype,
				       &gpuaddr[1]), "[--DOUBLE MAP--]\n");
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr[0]), "");
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr[1]), "");

	TEST_STEP(0, rv, msm_kgslpmem_free(dest, gsize), "");
	TEST_STEP(0, rv,
		  msm_mapusermem_ioctl(fd, pfd, gsize, 0, dest, memtype,
				       &gpuaddr[0]), "[--RE MAP--]\n");

	TEST_STEP(0, rv, msm_kgsl_close(pfd), "");

	if(msm_pmem_open(&pfd, DEF_MODE)==NON_PMEM_DEVICE)
	{
		printf("This is not a PMEM enabled device\n");
		return 0;
	}
	TEST_STEP(0, rv, msm_kgslpmem_alloc(pfd, gsize, &dest), "");
	TEST_STEP(0, rv, msm_kgsl_close(pfd), "");

	TEST_STEP(1, rv,
		  msm_mapusermem_ioctl(fd, pfd, gsize, 0, dest, memtype,
				       &gpuaddr[0]),
		  "[--MAP ON CLOSED PMEMFD--]\n");
	TEST_STEP(0, rv, msm_kgslpmem_free(dest, gsize), "");
	return rv;
}
/*Tests sharedvmalloc
 * with no mem
 * double map
 */
static int kgsl_test_adv_vmalloc(int fd)
{
	int rv = 0;
	unsigned int gpuaddr = 0, gsize = KGSL_PAGESIZE, flags;
	unsigned int *hostptr = NULL;
	flags = KGSL_MEMFLAGS_GPUREADONLY;
	TEST_STEP(EINVAL, rv,
		  msm_sharedvmalloc_ioctl(fd, (unsigned int)hostptr, flags,
					  &gpuaddr), "---INVAL MEM---\n");
	TEST_STEP(0, rv,
		  msm_mmap(gsize, -1, &hostptr, 0, MAP_SHARED | MAP_ANON),
		  "---MAP USER---\n");
	TEST_STEP(0, rv,
		  msm_sharedvmalloc_ioctl(fd, (unsigned int)hostptr, flags,
					  &gpuaddr), "---MAP USER MEM---\n");
	TEST_STEP(0, rv, msm_munmap(hostptr, gsize), "---UN MAP---\n");
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr), "---FREE---\n");
	return rv;
}

static int kgsl_test_adv_badmem()
{
	int fd = 0, rv = 0;
	msm_kgsl_open(&fd, DEF_MODE);
	/*map user */
	TEST_STEP(0, rv, kgsl_test_adv_mapuser(fd), "|---INV MAP USER---|\n");
	/*shared pmem */
	TEST_STEP(0, rv, kgsl_test_adv_sharepmem(fd), "|---SHARED PMEM---|\n");
	/*vmalloc */
	TEST_STEP(0, rv, kgsl_test_adv_vmalloc(fd), "|---VMALLOC---|\n");
	msm_kgsl_close(fd);
	return rv;
}

int kgsl_adv()
{
	int rv = 0;
	/*
	 * ctxts with wrong fd
	 * non init contexts
	 * non init fd, various structs
	 * empty cmdbufs
	 * bad gpu address's and lengths
	 * hostptr's need to be mean
	 * use each of the kgsl funcs on freed mem
	 * double free
	 * delete & reuse contexts
	 * bad params for each ioctl amd mmap
	 */
	kgsl_test_log(KGSL_LOG_LEVEL_INFO, "\nStarting Adversarial\n\n");
	/*bad params */
	TEST_STEP(0, rv, kgsl_test_adv_badparams(), "|---INVALID PARAMS---|\n");
	/*Memory tests */
	TEST_STEP(0, rv, kgsl_test_adv_badmem(), "|---BAD MEM---|\n");
	return rv;
}
