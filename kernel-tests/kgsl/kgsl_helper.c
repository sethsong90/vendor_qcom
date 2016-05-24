/******************************************************************************
-----------------------------------------------------------------------------
 Copyright (c) 2011-2012 Qualcomm Technologies, Inc.
 All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
******************************************************************************/
#include "kgsl_helper.h"
/*Opens a gpu fd specified by dev_name*/
int msm_kgsl_open(int *fd, mode_t mode)
{
	*fd = open(dev_name, mode);
	if (*fd == -1) {
		printf("error opening with dev_name %s and mode %d\n", dev_name,
		       mode);
		return errno;
	}
	return 0;
}

/*Closes a fd*/
int msm_kgsl_close(int fd)
{
	int rv = 0;
	rv = close(fd);
	if (rv)
		return errno;
	return rv;
}

/*Opens a pmem_fd specificed by PMEM_DEV*/
int msm_pmem_open(int *pmem_fd, mode_t mode)
{
	*pmem_fd = open(PMEM_DEV, mode);
	if (*pmem_fd == -1) {
		printf("error opening pmem %s with mode %d\n", PMEM_DEV, mode);
		return errno;
	}
	return 0;
}

char *kgsl_ioctl_to_name(int code)
{
	switch (code) {
	case IOCTL_KGSL_DEVICE_GETPROPERTY:
		return "IOCTL_KGSL_DEVICE_GETPROPERTY";
	case IOCTL_KGSL_DEVICE_WAITTIMESTAMP:
		return "IOCTL_KGSL_DEVICE_WAITTIMESTAMP";
	case IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS:
		return "IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS";
	case IOCTL_KGSL_CMDSTREAM_READTIMESTAMP:
		return "IOCTL_KGSL_CMDSTREAM_READTIMESTAMP";
	case IOCTL_KGSL_CMDSTREAM_FREEMEMONTIMESTAMP:
		return "IOCTL_KGSL_CMDSTREAM_FREEMEMONTIMESTAMP";
	case IOCTL_KGSL_DRAWCTXT_CREATE:
		return "IOCTL_KGSL_DRAWCTXT_CREATE";
	case IOCTL_KGSL_DRAWCTXT_DESTROY:
		return "IOCTL_KGSL_DRAWCTXT_DESTROY";
	case IOCTL_KGSL_SHAREDMEM_FROM_PMEM:
		return "IOCTL_KGSL_SHAREDMEM_FROM_PMEM";
	case IOCTL_KGSL_MAP_USER_MEM:
		return "IOCTL_KGSL_MAP_USER_MEM";
	case IOCTL_KGSL_SHAREDMEM_FROM_VMALLOC:
		return "IOCTL_KGSL_SHAREDMEM_FROM_VMALLOC";
	case IOCTL_KGSL_SHAREDMEM_FREE:
		return "IOCTL_KGSL_SHAREDMEM_FREE";
	case IOCTL_KGSL_DRAWCTXT_SET_BIN_BASE_OFFSET:
		return "IOCTL_KGSL_DRAWCTXT_SET_BIN_BASE_OFFSET";
#if CHECK_VERSION(3, 3)
	case IOCTL_KGSL_GPUMEM_ALLOC:
		return "IOCTL_KGSL_GPUMEM_ALLOC";
#endif
	default:
		return "unknown";
	}
}
/*Log fcn used to control amount of logging. Controlled by
 * kgsl_test_log_level which can be set in kgsl_test.c or kgsl_helper.h
 * at run or compile time
 */
void kgsl_test_log(unsigned int level, char *logstring)
{
	if (!logstring || !logstring[0])
		return;
	/*If a message starts with | then it's the beginning of a test*/
	if (level & kgsl_test_log_level || logstring[0] == '|')
		fprintf(stderr, "<id:%d> %s", (int)pthread_self(), logstring);
}

/*Log an integer*/
void kgsl_test_logd(unsigned int level, char *logstring, int value)
{
	static char tempstring[KGSL_TESTMAXLOGSTRING];
	snprintf(tempstring, KGSL_TESTMAXLOGSTRING, "%s %d\n", logstring,
		 value);
	kgsl_test_log(level, tempstring);
}

/*Log a hex val*/
void kgsl_test_logx(unsigned int level, char *logstring, int value)
{
	static char tempstring[KGSL_TESTMAXLOGSTRING];
	snprintf(tempstring, KGSL_TESTMAXLOGSTRING, "%s 0x%X\n", logstring,
		 value);
	kgsl_test_log(level, tempstring);
}

/*Use to track #fd's. This is just for debugging the test since
 * I made my own changes to the kernel to add this stat
 */
void kgsl_test_stat_fd()
{
	FILE *fd_open;
	char fd_count[15];
	char *helper;
	bzero(fd_count, 15);
	fd_open = fopen("/sys/class/kgsl/kgsl/fd_open", "r");
	helper = fgets(fd_count, 15, fd_open);
	if (helper)
		printf("---number of fd's open is %s---\n", fd_count);
	fclose(fd_open);

	fd_open = fopen("/sys/class/kgsl/kgsl/fd_open_max", "r");
	bzero(fd_count, 15);
	helper = fgets(fd_count, 15, fd_open);
	if (helper)
		printf("---max number open is %s---\n", fd_count);
	fclose(fd_open);
}

/*Counts the number of mem_entries active. Also just for debugging
 * the test.
 */
void kgsl_test_stat_refcount()
{
	FILE *rfile;
	FILE *rmax;
	char refcount[15];
	char *helper;

	bzero(refcount, 15);
	rfile = fopen("/sys/class/kgsl/kgsl/refcount", "r");
	helper = fgets(refcount, 15, rfile);
	if (helper)
		printf("---refcount is %s---\n", refcount);
	fclose(rfile);
	rmax = fopen("/sys/class/kgsl/kgsl/refcount", "r");
	helper = fgets(refcount, 15, rmax);
	if (helper)
		printf("---refcount max is %s---\n", refcount);
	fclose(rmax);
}

/*Used to check if calling process has an open gpu fd.*/
void kgsl_test_stat_pid()
{
	FILE *pid_file;
	char filepath[100];
	pid_t test_pid;
	bzero(filepath, 100);

	test_pid = getpid();
	snprintf(filepath, 100, "/sys/class/kgsl/kgsl/proc/%d", (int)test_pid);
	pid_file = fopen(filepath, "r");
	if (pid_file == NULL) {
		printf("---No fd open---\n");
	} else {
		printf("---Proc has fd---\n");
		fclose(pid_file);
	}
}

/*Builds a cmdpacket. Assumes that cmds points to wordsize*cnt mem
 * Working to configure for 2d devices as well as 3d
 */
int msm_create_simplebuf(unsigned int *cmds, int cnt)
{
	if (cmds == NULL)
		kgsl_test_exit(0, -1, "---NULL CMDBUF---\n");
	if (cnt == 0)
		return 0;
	if ((strncmp(dev_name, "/dev/kgsl-2d0", 13) == 0) ||
	    (strncmp(dev_name, "/dev/kgsl-2d1", 13) == 0)) {
		while (cnt > 0) {
			*cmds++ = NOP_2D;
			cnt--;
		}
		return 0;
	}
	if (cnt == 1) {
		*cmds = build_nop_packet(1);
		return 0;
	}
	*cmds = build_nop_packet(cnt - 1);
	cmds += cnt - 1;
	*cmds++ = build_type3_packet(CP_WAIT_FOR_IDLE, 1);
	return 0;
}

/*Performs a get_prop ioctl with the version option and prints vals*/
#if CHECK_VERSION(3, 1)
int msm_getprop_version_ioctl(int fd)
{
	int rv = 0;
	struct kgsl_device_getproperty param;
	struct kgsl_version info;
	static char temp[KGSL_TESTMAXLOGSTRING];

	memset(&temp, 0, KGSL_TESTMAXLOGSTRING);
	memset(&info, 0, sizeof(info));
	memset(&param, 0, sizeof(param));
	param.type = KGSL_PROP_VERSION;
	param.value = (void *)&info;
	param.sizebytes = sizeof(info);

	TEST_STEP(0, rv,
		  msm_kgsl_ioctl(fd, IOCTL_KGSL_DEVICE_GETPROPERTY,
				 (void *)&param, sizeof(param)), "");

	snprintf(temp, KGSL_TESTMAXLOGSTRING,
		 "drv_major %u  drv_minor %u dev_major %u dev_minor %u\n",
		 info.drv_major, info.drv_minor, info.dev_major,
		 info.dev_minor);
	kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE, temp);
	return rv;
}
/*Performs a get_prop ioctl with shadow option and prints vals*/
int msm_getprop_shadow_ioctl(int fd)
{
	int rv = 0;
	struct kgsl_device_getproperty param;
	struct kgsl_shadowprop info;
	static char temp[KGSL_TESTMAXLOGSTRING];

	memset(&temp, 0, KGSL_TESTMAXLOGSTRING);
	memset(&info, 0, sizeof(info));
	memset(&param, 0, sizeof(param));
	param.type = KGSL_PROP_DEVICE_SHADOW;
	param.value = (void *)&info;
	param.sizebytes = sizeof(info);

	TEST_STEP(0, rv,
		  msm_kgsl_ioctl(fd, IOCTL_KGSL_DEVICE_GETPROPERTY,
				 (void *)&param, sizeof(param)), "");

	snprintf(temp, KGSL_TESTMAXLOGSTRING, "gpuaddr 0x%x size %u flags %u\n",
		 info.gpuaddr, info.size, info.flags);
	kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE, temp);
	return rv;
}
#else
int msm_getprop_version_ioctl(int fd)
{
	kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE,
			"version prop not supported\n");
	kgsl_test_exit(-1, 0, "getprop version wrong\n");
	return -1;
}
int msm_getprop_shadow_ioctl(int fd)
{
	kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE,
			"shadow prop not supported\n");
	kgsl_test_exit(-1, 0, "getprop shadow wrong\n");
	return -1;
}
#endif
/*Performs a get_prop ioctl with info option and prints vals*/
int msm_getprop_info_ioctl(int fd)
{
	int rv = 0;
	struct kgsl_device_getproperty param;
	struct kgsl_devinfo info;
	static char temp[KGSL_TESTMAXLOGSTRING];

	memset(&temp, 0, KGSL_TESTMAXLOGSTRING);
	memset(&info, 0, sizeof(info));
	memset(&param, 0, sizeof(param));
	param.type = KGSL_PROP_DEVICE_INFO;
	param.value = (void *)&info;
	param.sizebytes = sizeof(info);

	TEST_STEP(0, rv,
		  msm_kgsl_ioctl(fd, IOCTL_KGSL_DEVICE_GETPROPERTY,
				 (void *)&param, sizeof(param)), "");

	snprintf(temp, KGSL_TESTMAXLOGSTRING,
		 "dev id %u chip_id 0x%08x mmu %u gmemaddr 0x%x size %u\n",
		 info.device_id, info.chip_id, info.mmu_enabled,
		 info.gmem_gpubaseaddr, info.gmem_sizebytes);
	kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE, temp);
	return rv;
}

/*Creates a gpu ctxt c_id with specified flags and fd
 * outputs the c_id created
 */
int msm_ctxtcreate_ioctl(int fd, int flags, unsigned int *c_id)
{
	int rv = 0;
	struct kgsl_drawctxt_create ctxt;
	memset(&ctxt, 0, sizeof(ctxt));
	ctxt.flags = flags;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_DRAWCTXT_CREATE, (void *)&ctxt,
			    sizeof(ctxt));
	*c_id = ctxt.drawctxt_id;
	return rv;
}

/*Deletes ctxt drawctxt_id in specified fd*/
int msm_ctxtdelete_ioctl(int fd, unsigned int drawctxt_id)
{
	int rv = 0;
	struct kgsl_drawctxt_destroy victim;
	memset(&victim, 0, sizeof(victim));
	victim.drawctxt_id = drawctxt_id;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_DRAWCTXT_DESTROY, (void *)&victim,
			    sizeof(victim));

	return rv;
}

/*Performs drawctxt_bin_base ioctl with specified c_id & off*/
int msm_drawctxt_bin_base_ioctl(int fd, unsigned int c_id, unsigned off)
{
	int rv = 0;
	struct kgsl_drawctxt_set_bin_base_offset base;
	memset(&base, 0, sizeof(base));
	base.drawctxt_id = c_id;
	base.offset = off;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_DRAWCTXT_SET_BIN_BASE_OFFSET,
			    (void *)&base, sizeof(base));
	return rv;
}

/*Maps userptr hostptr (input) into gpuaddr (output) with the map_user_mem
 * ioctl. The parameters of the map_user_mem struct are set to arguments
 * passed in
 */
int msm_mapusermem_ioctl(int fd, int pfd, unsigned int len, unsigned int off,
			 unsigned int *hostptr, unsigned int memtype,
			 unsigned int *gpuaddr)
{
	int rv = 0;
	struct kgsl_map_user_mem usermem;
	memset(&usermem, 0, sizeof(usermem));
	usermem.fd = pfd;
	usermem.len = len;
	usermem.offset = off;
	usermem.memtype = (enum kgsl_user_mem_type)memtype;
	usermem.hostptr = (unsigned int)hostptr;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_MAP_USER_MEM, (void *)&usermem,
			    sizeof(usermem));
	*gpuaddr = usermem.gpuaddr;
	return rv;
}

/*Shares vm pointed to by hostptr(input) to gpuaddr (output) with flags
 * given by using the sharedvmalloc ioctl
 */
int msm_sharedvmalloc_ioctl(int fd, unsigned int hostptr, unsigned int flags,
			    unsigned int *gpuaddr)
{
	int rv = 0;
	struct kgsl_sharedmem_from_vmalloc vm;
	memset(&vm, 0, sizeof(vm));
	vm.hostptr = hostptr;
	vm.flags = flags;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_SHAREDMEM_FROM_VMALLOC, (void *)&vm,
			    sizeof(vm));
	*gpuaddr = vm.gpuaddr;
	return rv;
}

/*Shares pmem described by pmem_fd with gpu at gpuaddr(output)
 * by using the sharedpmem ioctl
 */
int msm_sharedpmem_ioctl(int fd, int pmem_fd, unsigned int len,
			 unsigned int off, unsigned int *gpuaddr)
{
	int rv = 0;
	struct kgsl_sharedmem_from_pmem pmem1;
	memset(&pmem1, 0, sizeof(pmem1));
	pmem1.pmem_fd = pmem_fd;
	pmem1.len = len;
	pmem1.offset = off;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_SHAREDMEM_FROM_PMEM, (void *)&pmem1,
			    sizeof(pmem1));
	*gpuaddr = pmem1.gpuaddr;
	return rv;
}

/*Flushes the cache at gpuaddr usig the flush cache ioctl*/
int msm_sharedmem_flush_cache_ioctl(int fd, unsigned int gpuaddr)
{
	int rv = 0;
	struct kgsl_sharedmem_free fmem;
	memset(&fmem, 0, sizeof(fmem));
	fmem.gpuaddr = gpuaddr;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_SHAREDMEM_FLUSH_CACHE, (void *)&fmem,
			    sizeof(fmem));
	return rv;
}

/*Frees the mem at gpuaddr (input) using the shared_mem_free ioctl*/
int msm_sharedmemfree_ioctl(int fd, unsigned int gpuaddr)
{
	int rv = 0;
	struct kgsl_sharedmem_free fmem;
	memset(&fmem, 0, sizeof(fmem));
	fmem.gpuaddr = gpuaddr;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_SHAREDMEM_FREE, (void *)&fmem,
			    sizeof(fmem));
	return rv;
}

/*Allocs size (input) bytes with flags (input) in the gpu at
 * gpuaddr (output) using the gpumem_alloc ioctl
 */
#if CHECK_VERSION(3, 3)
int msm_gpualloc_ioctl(int fd, size_t size, unsigned int flags,
		       unsigned int *gpuaddr)
{
	struct kgsl_gpumem_alloc gmem;
	int rv = 0;
	memset(&gmem, 0, sizeof(gmem));
	gmem.size = size;
	gmem.flags = flags;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_GPUMEM_ALLOC, (void *)&gmem,
			    sizeof(gmem));
	*gpuaddr = (unsigned int)gmem.gpuaddr;
	return rv;
}
#else
int msm_gpualloc_ioctl(int fd, size_t size, unsigned int flags,
		unsigned int *gpuaddr)
{
	kgsl_test_log(KGSL_LOG_LEVEL_ERROR, "GPUALLOC not supported\n");
	kgsl_test_exit(-1, 0, "GPUALLOC NOT SUPPORTED\n");
	return -1;
}
#endif
/*Maps mem from fd (input) at addr (input) to dest (input) with specified
 * flags using mmap
 */
int msm_mmap(unsigned int size, int fd, unsigned int **dest, unsigned int addr,
	     int flags)
{
	int rv = 0;
	if (fd == 0)
		return 0;
	*dest = mmap(NULL, size, PROT_READ | PROT_WRITE, flags, fd, addr);
	if (*dest == MAP_FAILED) {
		printf("mmap failed %d %s size: %d addr %x\n", errno,
		       strerror(errno), size, addr);
		rv = errno;
	}
	return rv;
}

/*Unmaps size(input) mem from addr(input)*/
int msm_munmap(unsigned int *addr, unsigned int size)
{
	int rv = 0;
	if (munmap(addr, size)) {
		printf("munmap failed %d %s size: %d addr: %x\n", errno,
		       strerror(errno), size, *addr);
		rv = errno;
	}
	return rv;
}

/*Allocs mem from already open pmem_fd(input) of size_bytes size(input)
 * to dest(input)
 */
int msm_kgslpmem_alloc(int pmem_fd, unsigned int size, unsigned int **dest)
{
	int rv = 0;
	/*mmap is alloc for pmem */
	rv = msm_mmap(size, (unsigned int)pmem_fd, dest, 0, MAP_SHARED);
	return rv;
}

/*Free's pmem by unmapping*/
int msm_kgslpmem_free(unsigned int *addr, unsigned int size)
{
	int rv = 0;
	rv = msm_munmap(addr, size);
	return rv;
}

/*Frees gpuaddr(input) when the rb ts pointer of specified type is passed
 * timestamp ts. Also removes gpuaddr from mem_entry list into mem_to_be_freed
 */
int msm_cmds_free_on_ts_ioctl(int fd, unsigned int gpuaddr, unsigned int type,
			      unsigned int ts)
{
	int rv = 0;
	struct kgsl_cmdstream_freememontimestamp cmdsfree;
	memset(&cmdsfree, 0, sizeof(cmdsfree));
	cmdsfree.gpuaddr = gpuaddr;
	cmdsfree.type = type;
	cmdsfree.timestamp = ts;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_CMDSTREAM_FREEMEMONTIMESTAMP,
			    (void *)&cmdsfree, sizeof(cmdsfree));
	return rv;
}

/*Reads the rb's ts of specified type into ts(output) using the
 * read_timestamp ioctl
 */
int msm_cmds_read_ts_ioctl(int fd, unsigned int type, unsigned int *ts)
{
	int rv = 0;
	struct kgsl_cmdstream_readtimestamp cmdsread;
	memset(&cmdsread, 0, sizeof(cmdsread));
	cmdsread.type = type;
	*ts = 0;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_CMDSTREAM_READTIMESTAMP,
			    (void *)&cmdsread, sizeof(cmdsread));
	*ts = cmdsread.timestamp;
	return rv;
}

/*Waits for time timeout for timestamp ts to be executed
 * using the wait_timestamp ioctl
 */
int msm_waitts_ioctl(int fd, unsigned ts, unsigned int timeout)
{
	int rv = 0;
	struct kgsl_device_waittimestamp waitts;
	memset(&waitts, 0, sizeof(waitts));
	waitts.timestamp = ts;
	waitts.timeout = timeout;
	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_DEVICE_WAITTIMESTAMP,
			    (void *)&waitts, sizeof(waitts));
	return rv;
}

#if KGSL_CONTEXT_SUBMIT_IB_LIST
/*Issues cmds with the issueibcmds ioctl and sets all the params
 * as passed in. ts (output)*/
int msm_rb_issueib_ioctl(int fd, unsigned int ctxt_id, int flags,
			 struct kgsl_ibdesc *ibaddr, unsigned int numibs,
			 unsigned int *ts)
{
	int rv = 0;
	struct kgsl_ringbuffer_issueibcmds issueib;
	memset(&issueib, 0, sizeof(issueib));

	issueib.drawctxt_id = ctxt_id;
	issueib.ibdesc_addr = (unsigned int)ibaddr;
	issueib.numibs = numibs;
	issueib.flags = (unsigned int)flags;

	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS,
			    (void *)&issueib, sizeof(issueib));
	*ts = issueib.timestamp;
	return rv;
}
#else
int msm_rb_issueib_ioctl(int fd, unsigned int ctxt_id, int flags,
			 struct kgsl_ibdesc *ibaddr, unsigned int numibs,
			 unsigned int *ts)
{
	int rv = 0;
	struct kgsl_ringbuffer_issueibcmds issueib;
	memset(&issueib, 0, sizeof(issueib));

	if (numibs > 1) {
		kgsl_test_exit(-1, 0, "KGSL_CONTEXT_SUBMIT_IB_LIST not supported, so only 1 ib allowed\n");
		return -1;
	}

	issueib.drawctxt_id = ctxt_id;
	issueib.ibaddr = ibaddr[0].gpuaddr;
	issueib.sizedwords = ibaddr[0].sizedwords;
	issueib.flags = (unsigned int)flags;

	rv = msm_kgsl_ioctl(fd, IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS,
			    (void *)&issueib, sizeof(issueib));
	*ts = issueib.timestamp;
	return rv;
}
#endif

/*Calls the ioctl specificed by ioctl_id with specified val and size*/
int msm_kgsl_ioctl(int fd, int ioctl_id, void *val, unsigned int size)
{
	int rv = 0;
	rv = ioctl(fd, ioctl_id, val, size);
	if (rv != 0)
		rv = errno;
	return rv;
}
