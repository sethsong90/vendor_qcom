/******************************************************************************
  -----------------------------------------------------------------------------
  Copyright (c) 2011-2012 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
 ******************************************************************************/
#ifndef __KGSL_HELPER_H__
#define __KGSL_HELPER_H__

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/msm_kgsl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
/*3D*/
#define KGSL_3D_DEV "/dev/kgsl-3d0"
#define CP_NOP 0x10
#define CP_WAIT_FOR_IDLE 0x26

#define build_type3_packet(opcode, cnt) ((((opcode) & 0xFF) << 8) | \
	(((cnt)-1) << 16) | ((unsigned int)0x3 << 30))

#define build_nop_packet(cnt) ((CP_NOP << 8) | (((cnt)-1) << 16) | \
	((unsigned int)0x3 << 30))
/*2D*/
#define KGSL_2D0_DEV "/dev/kgsl-2d0"
#define KGSL_2D1_DEV "/dev/kgsl_2d1"
#define ADDR_VGV3_LAST 0x007F
#define NOP_2D (ADDR_VGV3_LAST<<24)
/*log levels*/
#define  KGSL_LOG_LEVEL_ERROR 0x01
#define  KGSL_LOG_LEVEL_INFO  0x02
#define  KGSL_LOG_LEVEL_DISPLAY 0x03
#define  KGSL_LOG_LEVEL_VERBOSE 0x04
#define  KGSL_LOG_LEVEL_ALL 0x07
#define  KGSL_DEF_LOG_LEVEL KGSL_LOG_LEVEL_DISPLAY

#define  KGSL_PAGESIZE 0x1000
#define  KGSL_MAX_CMDS_PER_PAGE (KGSL_PAGESIZE/sizeof(unsigned int))
#define  KGSL_TEST_TIMEOUT 0xFFFFFFFF
#define  KGSL_TEST_EPSILON 0x0000000F
#define  KGSL_TEST_FAILURE -1
#define  KGSL_TESTMAXLOGSTRING 256

#define  DEF_DEVICE KGSL_3D_DEV
#define  DEF_MODE (mode_t) (O_RDWR)
#define  DEF_NUMCMDS 5
/*must be even*/
#define  DEF_ARRAY_SIZE 10
#define  PMEM_DEV "/dev/pmem_adsp"

/*Error Number returned when PMEM_DEV not found*/
#define NON_PMEM_DEVICE 2

#ifndef KGSL_CONTEXT_SUBMIT_IB_LIST
struct kgsl_ibdesc {
	unsigned int gpuaddr;
	void *hostptr;
	unsigned int sizedwords;
	unsigned int ctrl;
};
/*
 * define this flag to be 0 so we don't pass it to the kernel
 */
#define KGSL_CONTEXT_SUBMIT_IB_LIST 0
#endif

extern unsigned int kgsl_test_log_level;
extern char *dev_name;
/***LOG FCNS***/
void kgsl_test_exit(int result, int expected, const char *name);
char *kgsl_ioctl_to_name(int code);
void kgsl_test_log(unsigned int level, char *logstring);
void kgsl_test_logd(unsigned int level, char *logstring, int value);
void kgsl_test_logx(unsigned int level, char *logstring, int value);
void kgsl_test_log_error(char *funcstr, int errno);

/***STAT FCNS***/
void kgsl_test_stat_fd();
void kgsl_test_stat_refcount();
void kgsl_test_stat_pid();

/***OPEN/CLOSE***/
int msm_kgsl_open(int *fd, mode_t mode);
int msm_kgsl_close(int fd);
int msm_pmem_open(int *pmem_fd, mode_t mode);

/***IOCTL WRAPPERS***/
int msm_create_simplebuf(unsigned int *cmds, int cnt);
int msm_getprop_version_ioctl(int fd);
int msm_getprop_shadow_ioctl(int fd);
int msm_getprop_info_ioctl(int fd);
int msm_ctxtcreate_ioctl(int fd, int flags, unsigned int *c_id);
int msm_ctxtdelete_ioctl(int fd, unsigned int drawctxt_id);
int msm_drawctxt_bin_base_ioctl(int fd, unsigned int c_id, unsigned int off);
int msm_mapusermem_ioctl(int fd, int pfd, unsigned int len, unsigned int off,
			 unsigned int *hostptr, unsigned int memtype,
			 unsigned int *gpuaddr);
int msm_sharedpmem_ioctl(int fd, int pmem_fd, unsigned int len,
			 unsigned int off, unsigned int *gpuaddr);
int msm_sharedvmalloc_ioctl(int fd, unsigned int hostptr, unsigned int flags,
			    unsigned *gpuaddr);
int msm_sharedmem_flush_cache_ioctl(int fd, unsigned int gpuaddr);
int msm_sharedmemfree_ioctl(int fd, unsigned int gpuaddr);
int msm_gpualloc_ioctl(int fd, size_t size, unsigned int flags,
		       unsigned int *gpuaddr);
int msm_mmap(unsigned int size, int fd, unsigned int **dest, unsigned int addr,
	     int flags);
int msm_munmap(unsigned int *addr, unsigned int size);
int msm_kgslpmem_alloc(int pmem_fd, unsigned int size, unsigned int **dest);
int msm_kgslpmem_free(unsigned int *addr, unsigned int size);
int msm_cmds_free_on_ts_ioctl(int fd, unsigned int gpuaddr, unsigned int type,
			      unsigned int ts);
int msm_cmds_read_ts_ioctl(int fd, unsigned int type, unsigned int *ts);
int msm_waitts_ioctl(int fd, unsigned int ts, unsigned int timeout);
int msm_rb_issueib_ioctl(int fd, unsigned int ctxt_id, int flags,
			 struct kgsl_ibdesc *ibaddr, unsigned int numibs,
			 unsigned int *ts);
int msm_kgsl_ioctl(int fd, int ioctl_id, void *val, unsigned int size);

/***MULTI STEP OPERATIONS***/
int kgsl_test_openclose();
int kgsl_test_simple_getprops();
int kgsl_test_getprops(int fd);
int kgsl_test_create_destroy();
int kgsl_test_checkts(int fd, unsigned int ts);
int kgsl_test_simplecmd(int fd, size_t size, unsigned int flags,
			unsigned int *gpuaddr, unsigned int c_id,
			unsigned int *ts, unsigned int **cmds, int numcmds);
int kgsl_test_unmapgpu(int fd, size_t size, unsigned int *user_ptr,
		       unsigned int gpuaddr);
int kgsl_test_mapuser(int fd);
int kgsl_test_simple(int count);
int kgsl_test_badparams();
int kgsl_test_ctxts();

/***TOP LEVEL TESTS***/
int kgsl_nominal();
int kgsl_repeat(int cnt);
int kgsl_adv();
int kgsl_stress();

/*Used to run the tests,log and catch unexpected results
 * exits to kgsl_test_exit
 */
#define TEST_STEP(_expected, _result, _code, _name) \
	do { \
		kgsl_test_log(KGSL_LOG_LEVEL_VERBOSE, _name);\
		_result = _code; \
		if (_result != _expected) { \
			kgsl_test_exit(_result, _expected, _name); \
		} \
	} while (0)
#ifdef KGSL_VERSION_MAJOR
#define CHECK_VERSION(a_maj, a_min) (KGSL_VERSION_MAJOR > (a_maj) \
		|| (KGSL_VERSION_MAJOR == (a_maj) && \
		KGSL_VERSION_MINOR >= (a_min)))
#else
#define CHECK_VERSION(a_maj, a_min) 0
#endif
#endif /*__KGSL_HELPER_H__*/
