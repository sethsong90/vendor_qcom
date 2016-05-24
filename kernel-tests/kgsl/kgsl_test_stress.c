/******************************************************************************
-----------------------------------------------------------------------------
 Copyright (c) 2011-2012 Qualcomm Technologies, Inc.
 All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
******************************************************************************/
#include "kgsl_helper.h"

pthread_mutex_t cmdsLock;
pthread_cond_t cmdsSignal;
char cmdsBool;

struct kgsl_test_ctxt {
	int fd;
	size_t size;
	unsigned int c_id;
	unsigned int gpuaddr;
	unsigned int *u_ptr;
	unsigned int ts;
	int numcmds;
};

void *kgsl_test_stress_issue_cmds(void *arg);
void *kgsl_test_stress_gpufree(void *arg);
void *kgsl_test_stress_issue_free(void *arg);
void *kgsl_test_stress_issue_wait(void *arg);
#define STRESS_ARRAY 180 /*divisible by 4 */
#define KGSL_STRESS_NUM_PAGES 4
#define KGSL_STRESS_SIZE (KGSL_STRESS_NUM_PAGES*KGSL_PAGESIZE)
#define KGSL_STRESS_NUMCMDS (KGSL_STRESS_NUM_PAGES*KGSL_MAX_CMDS_PER_PAGE/2)
/***************FCNS****************/
/*Takes a kgsl_test_ctxt and handles the operations to issue cmds*/
void *kgsl_test_stress_issue_cmds(void *arg)
{
	int rv = 0;
	unsigned int flags = KGSL_MEMFLAGS_GPUREADONLY;
	struct kgsl_test_ctxt *cmd = arg;
	TEST_STEP(0, rv,
		  kgsl_test_simplecmd(cmd->fd, cmd->size, flags, &cmd->gpuaddr,
				      cmd->c_id, &cmd->ts, &cmd->u_ptr,
				      cmd->numcmds), "[--SIMPLE THR--]\n");
	return NULL;
}

/*Unmaps from user and frees from gpu*/
void *kgsl_test_stress_gpufree(void *arg)
{
	int rv = 0;
	struct kgsl_test_ctxt *cmds = arg;
	TEST_STEP(0, rv, msm_munmap(cmds->u_ptr, cmds->size), "");
	TEST_STEP(0, rv, msm_sharedmemfree_ioctl(cmds->fd, cmds->gpuaddr), "");
	return NULL;
}

/*Unmaps from user and uses free_on_ts to free from gpu*/
void *kgsl_test_stress_issue_free(void *arg)
{
	int rv = 0;
	struct kgsl_test_ctxt *cmds = arg;
	TEST_STEP(0, rv, msm_munmap(cmds->u_ptr, cmds->size), "");
	TEST_STEP(0, rv,
		  msm_cmds_free_on_ts_ioctl(cmds->fd, cmds->gpuaddr,
					    KGSL_TIMESTAMP_CONSUMED, cmds->ts),
		  "");
	return NULL;
}

/*Waits on ts from a kgsl_test_ctxt*/
void *kgsl_test_stress_issue_wait(void *arg)
{
	int rv = 0;
	struct kgsl_test_ctxt *cmds = arg;
	TEST_STEP(0, rv,
		  msm_waitts_ioctl(cmds->fd, cmds->ts, KGSL_TEST_TIMEOUT), "");
	return NULL;
}

/*Does different combinations of fds & ctxts pseudo concurrently*/
static int kgsl_test_stress_issue()
{
	int rv = 0, i = 0;
	struct kgsl_test_ctxt cmds[STRESS_ARRAY];
	memset(cmds, 0, sizeof(struct kgsl_test_ctxt) * STRESS_ARRAY);
	pthread_t issue_one_fd[STRESS_ARRAY];
	pthread_attr_t one_fd_attr[STRESS_ARRAY];

	/*make many ctxts & each one does an issueib with 1 fd */
	msm_kgsl_open(&cmds[0].fd, DEF_MODE);
	for (i = 0; i < STRESS_ARRAY; i++) {
		cmds[i].fd = cmds[0].fd;
		cmds[i].size = KGSL_STRESS_SIZE;
		cmds[i].numcmds = KGSL_STRESS_NUMCMDS;
		TEST_STEP(0, rv,
			  msm_ctxtcreate_ioctl(cmds[i].fd, 0, &cmds[i].c_id),
			  "");
		pthread_attr_init(&one_fd_attr[i]);
	}
	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv,
			  pthread_create(&issue_one_fd[i], &one_fd_attr[i],
					 &kgsl_test_stress_issue_cmds,
					 &cmds[i]), "[--1 FD--]\n");
	}
	for (i = 0; i < STRESS_ARRAY; i++)
		TEST_STEP(0, rv, pthread_join(issue_one_fd[i], NULL), "");

	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv, (int)kgsl_test_stress_gpufree(&cmds[i]), "");
		TEST_STEP(0, rv, msm_ctxtdelete_ioctl(cmds[i].fd, cmds[i].c_id),
			  "");
	}
	TEST_STEP(0, rv, msm_kgsl_close(cmds[0].fd), "");
	/*many fds 1 ctxt each */
	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv, msm_kgsl_open(&cmds[i].fd, DEF_MODE), "");
		TEST_STEP(0, rv,
			  msm_ctxtcreate_ioctl(cmds[i].fd, 0, &cmds[i].c_id),
			  "");
	}

	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv,
			  pthread_create(&issue_one_fd[i], &one_fd_attr[i],
					 &kgsl_test_stress_issue_cmds,
					 &cmds[i]), "[--MULTI FD--]\n");
	}

	for (i = 0; i < STRESS_ARRAY; i++)
		pthread_join(issue_one_fd[i], NULL);

	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv, (int)kgsl_test_stress_gpufree(&cmds[i]), "");
		TEST_STEP(0, rv, msm_ctxtdelete_ioctl(cmds[i].fd, cmds[i].c_id),
			  "");
		TEST_STEP(0, rv, msm_kgsl_close(cmds[i].fd), "");
	}
	/*STRESS_ARRAY/2 fds STRESS_ARRAY/4 ctxt each */
	for (i = 0; i < STRESS_ARRAY; i++) {
		if (i % 2 == 0)
			TEST_STEP(0, rv, msm_kgsl_open(&cmds[i].fd, DEF_MODE),
				  "");
		else
			cmds[i].fd = cmds[i - 1].fd;
		TEST_STEP(0, rv,
			  msm_ctxtcreate_ioctl(cmds[i].fd, 0, &cmds[i].c_id),
			  "");
	}

	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv,
			  pthread_create(&issue_one_fd[i], &one_fd_attr[i],
					 &kgsl_test_stress_issue_cmds,
					 &cmds[i]),
			  "[--MULTI CTXT PER FD--]\n");
	}

	for (i = 0; i < STRESS_ARRAY; i++)
		pthread_join(issue_one_fd[i], NULL);

	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv, (int)kgsl_test_stress_gpufree(&cmds[i]), "");
		TEST_STEP(0, rv, msm_ctxtdelete_ioctl(cmds[i].fd, cmds[i].c_id),
			  "");
		if (i % 2)
			TEST_STEP(0, rv, msm_kgsl_close(cmds[i].fd), "");
		pthread_attr_destroy(&one_fd_attr[i]);
	}
	return rv;
}

/*issues cmd with cmds[i] and tells wait_fcn to go*/
static void *issue_fcn(void *arg)
{
	int rv = 0, i = 0;
	struct kgsl_test_ctxt *cmds = arg;

	pthread_mutex_lock(&cmdsLock);
	cmdsBool = 1;
	while (cmdsBool == 1) {
		pthread_cond_broadcast(&cmdsSignal);
		pthread_cond_wait(&cmdsSignal, &cmdsLock);
	}
	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv, (int)kgsl_test_stress_issue_cmds(&cmds[i]),
			  "[--ISSUE--]\n");
		cmdsBool = 1;
		while (cmdsBool == 1) {
			TEST_STEP(0, rv, pthread_cond_broadcast(&cmdsSignal),
				  "");
			TEST_STEP(0, rv,
				  pthread_cond_wait(&cmdsSignal, &cmdsLock),
				  "");
		}
	}
	cmdsBool = 1;
	pthread_cond_broadcast(&cmdsSignal);
	pthread_mutex_unlock(&cmdsLock);
	return NULL;
}

/*waits for issue fcn to issue with cmds[i] and use that ts to wait and free*/
static void *wait_fcn(void *arg)
{
	int rv = 0, i = 0;
	struct kgsl_test_ctxt *cmds = arg;

	TEST_STEP(0, rv, pthread_mutex_lock(&cmdsLock), "");
	cmdsBool = 0;
	while (cmdsBool == 0) {
		pthread_cond_broadcast(&cmdsSignal);
		pthread_cond_wait(&cmdsSignal, &cmdsLock);
	}
	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv, (int)kgsl_test_stress_issue_wait(&cmds[i]),
			  "[--WAIT--]\n");
		TEST_STEP(0, rv, (int)kgsl_test_stress_issue_free(&cmds[i]),
			  "[--FREE--]\n");
		cmdsBool = 0;
		while (cmdsBool == 0) {
			TEST_STEP(0, rv, pthread_cond_broadcast(&cmdsSignal),
				  "");
			TEST_STEP(0, rv,
				  pthread_cond_wait(&cmdsSignal, &cmdsLock),
				  "");
		}
	}

	cmdsBool = 0;
	TEST_STEP(0, rv, pthread_cond_broadcast(&cmdsSignal), "");
	TEST_STEP(0, rv, pthread_mutex_unlock(&cmdsLock), "");
	return NULL;
}

/*Issues cmds in issue thread and then waits and frees on ts in wait*/
static int kgsl_test_stress_wait()
{
	int rv = 0, i = 0;
	struct kgsl_test_ctxt cmds[STRESS_ARRAY];
	memset(cmds, 0, sizeof(struct kgsl_test_ctxt) * STRESS_ARRAY);
	pthread_t wait, issue;
	pthread_attr_t wait_attr, issue_attr;
	pthread_attr_init(&wait_attr);
	pthread_attr_init(&issue_attr);
	/*init cmds */
	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv, msm_kgsl_open(&cmds[i].fd, DEF_MODE), "");
		TEST_STEP(0, rv,
			  msm_ctxtcreate_ioctl(cmds[i].fd, 0, &cmds[i].c_id),
			  "");
		cmds[i].size = KGSL_STRESS_SIZE;
		cmds[i].numcmds = KGSL_STRESS_NUMCMDS;
	}

	/*issue thread & wait thread */
	pthread_mutex_lock(&cmdsLock);
	TEST_STEP(0, rv, pthread_create(&issue, &issue_attr, &issue_fcn, cmds),
		  "[--ISSUE THREAD--]\n");
	/*Wait for Issue_fcn to get lock & then wait*/
	while (cmdsBool == 0) {
		pthread_cond_broadcast(&cmdsSignal);
		pthread_cond_wait(&cmdsSignal, &cmdsLock);
	}
	TEST_STEP(0, rv, pthread_create(&wait, &wait_attr, &wait_fcn, cmds),
		  "[--WAIT THREAD--]\n");
	pthread_cond_broadcast(&cmdsSignal);
	cmdsBool = 1;
	pthread_mutex_unlock(&cmdsLock);

	/*signal and wait after each issueib and waitts */
	pthread_join(issue, NULL);
	pthread_join(wait, NULL);
	for (i = 0; i < STRESS_ARRAY; i++) {
		TEST_STEP(0, rv, msm_ctxtdelete_ioctl(cmds[i].fd, cmds[i].c_id),
			  "");
		TEST_STEP(0, rv, msm_kgsl_close(cmds[i].fd), "");
	}
	pthread_attr_destroy(&wait_attr);
	pthread_attr_destroy(&issue_attr);

	return rv;
}

/*Runs stress tests for rb ioctls*/
static int kgsl_test_stress_cmds()
{
	int rv = 0;
	pthread_mutex_init(&cmdsLock, NULL);
	pthread_cond_init(&cmdsSignal, NULL);
	cmdsBool = 0;
	TEST_STEP(0, rv, kgsl_test_stress_issue(), "|---ISSUE---|\n");
	TEST_STEP(0, rv, kgsl_test_stress_wait(),
		  "|---ISSUE, WAIT, FREE---|\n");
	pthread_cond_destroy(&cmdsSignal);
	pthread_mutex_destroy(&cmdsLock);
	return rv;
}

/*Allocs pages in jumps of powers of 2*/
static int kgsl_test_stress_gpularge()
{
	int rv = 0, fd = 0, cnt = 1;
	unsigned int gflags = 0, gpuaddr = 0;
	size_t gsize = 0;
	gflags = KGSL_MEMFLAGS_GPUREADONLY;
	gsize = KGSL_PAGESIZE;
	msm_kgsl_open(&fd, DEF_MODE);

	while (rv == 0) {
		rv = msm_gpualloc_ioctl(fd, gsize, gflags, &gpuaddr);
		if (rv == 0) {
			cnt++;
			gsize += gsize;
			TEST_STEP(0, rv, msm_sharedmemfree_ioctl(fd, gpuaddr),
				  "");
		} else
			break;
	}
	kgsl_test_logx(KGSL_LOG_LEVEL_INFO, "---LARGEST NUM PAGES---:",
		       gsize / (KGSL_PAGESIZE));

	TEST_STEP(0, rv, msm_kgsl_close(fd), "");

	return rv;
}

/*Runs mem stress tests*/
static int kgsl_test_stress_mem()
{
	int rv = 0;
	/*alloc and free lots of mem */
	TEST_STEP(0, rv, kgsl_test_stress_gpularge(),
		  "|---LARGE GPU CALLS---|\n");
	return rv;
}

int kgsl_stress()
{
	int rv = 0;

	kgsl_test_log(KGSL_LOG_LEVEL_INFO, "\nStarting Stress & Stability\n\n");
	TEST_STEP(0, rv, kgsl_test_stress_cmds(), "|---THREAD CMDS---|\n");
	TEST_STEP(0, rv, kgsl_test_stress_mem(), "|---THREAD MEM---|\n");
	return rv;
}
