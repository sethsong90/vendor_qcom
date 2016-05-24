/*
 * Copyright (c) 2010-2012 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 */

/*
 * Run this in a loop, For example:
 * while test; do true; done
 * For more coverage, run this loop in two separate ADB sessions, concurrently.
 * The test should stop if an error is seen.
 */

/* Maximum instruction buffer size  */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>

#define N_INST 100
#define MSIZE  4096

/* Disable this if testing on ARMv6 */
#define HAVE_THUMB


#ifdef HAVE_THUMB
#define F1_RESULT 47
#define F2_RESULT -94
#else
#define F1_RESULT 18
#define F2_RESULT -36
#endif


/* Our 'function buffer' */
unsigned int *fun;

unsigned int function_1[N_INST] = {
	/* --- ARM CODE --- */
	0xE3A03000, 	/* mov r3, #0 		*/
	0xEA000008, 	/* b pc + #8 */
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/

#ifdef HAVE_THUMB
	0xE28F2001,	/* add r2, pc, #1	*/
	0xE12FFF12,	/* bx r2		*/

	/* --- THUMB CODE --- */
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/

	0x6802BF00,	/* nop			*/
			/* ldr r2, [r0]		*/

	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/
	0x6802189B,	/* adds r3, r3, r2	*/
			/* ldr r2, [r0]		*/

	0xBF00189B,	/* adds r3, r3, r2	*/
			/* nop			*/
	0x189B6802,	/* ldr r2, [r0]		*/
			/* adds r3, r3, r2	*/
	0x4710467A,	/* mov r2, pc		*/
			/* bx r2		*/
#endif
	/* --- ARM CODE --- */
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/
	0xE5902000, 	/* ldr r2, [r0]		*/
	0xE0833002, 	/* add r3, r3, r2 	*/

	0xE1A00003,	/* mov r0, r3 		*/
	0xE1A0F00E	/* mov pc, lr   	*/
};

unsigned int function_2[N_INST] = {
	/* --- ARM CODE --- */
	0xE3A03000, 	/* mov r3, #0 		*/
	0xEA000010, 	/* b pc + #10 		*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/

#ifdef HAVE_THUMB
	0xE28F2001,	/* add r2, pc, #1	*/
	0xE12FFF12,	/* bx r2		*/

	/* --- THUMB CODE --- */
	0x1A9B680A,	/* ldr r2, [r1]		*/
			/* subs r3, r3, r2	*/
	0x1A9B680A,	/* ldr r2, [r1]		*/
			/* subs r3, r3, r2	*/
	0x1A9B680A,	/* ldr r2, [r1]		*/
			/* subs r3, r3, r2	*/
	0x1A9B680A,	/* ldr r2, [r1]		*/
			/* subs r3, r3, r2	*/
	0x1A9B680A,	/* ldr r2, [r1]		*/
			/* subs r3, r3, r2	*/
	0x1A9B680A,	/* ldr r2, [r1]		*/
			/* subs r3, r3, r2	*/
	0x1A9B680A,	/* ldr r2, [r1]		*/
			/* subs r3, r3, r2	*/
	0x1A9B680A,	/* ldr r2, [r1]		*/
			/* subs r3, r3, r2	*/
	0x1A9B680A,	/* ldr r2, [r1]		*/
			/* subs r3, r3, r2	*/

	0x680ABF00,	/* nop			*/
			/* ldr r2, [r1]		*/

	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/
	0x680A1A9B,	/* subs r3, r3, r2	*/
			/* ldr r2, [r1]		*/

	0xBF001A9B,	/* subs r3, r3, r2	*/
			/* nop			*/
	0x1A9B680A,	/* ldr r2, [r1]		*/
			/* subs r3, r3, r2	*/
	0x4710467A,	/* mov r2, pc		*/
			/* bx r2		*/
#endif
	/* --- ARM CODE --- */
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/
	0xE5912000, 	/* ldr r2, [r1] 	*/
	0xE0433002, 	/* sub r3, r3, r2	*/

	0xE1A00003,	/* mov r0, r3 		*/
	0xE1A0F00E	/* mov pc, lr   	*/
};

#ifdef KDEV
void cacheflush(long b, long e, int f)
{
	__clear_cache((void*) b, (void *) e);
}
#endif

void flush(void)
{
	cacheflush((long)fun, (long)(fun+N_INST), 0);
}

int segv_handler(void)
{
	printf("**************************************************\n");
	printf("SIGSEGV caught\n");
	printf("Self-modifying code test FAILED!\n");
	printf("**************************************************\n");
	exit(-1);
	return 0;
}

int main(int argc, char** argv)
{
	int (*do_stuff)(unsigned int,unsigned int);;

	int ret = 0, i = 0, n = 0;
	volatile int addval;
	unsigned int ptr = &addval;

	fun = mmap(NULL, MSIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
		   MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	do_stuff = fun;
	if (fun == MAP_FAILED) {
		printf("MMAP FAILED\n");
		goto fail1;
	}

	printf("Self-modifying code test starting.\n");

	signal(SIGSEGV, segv_handler);

	/* Some iteration counter stuff... */
	for (n = 0; n < 1000; n++) {
		for (i = 0; i < N_INST; i++)	/* Copy first function into */
			fun[i] = function_1[i];	/* execution buffer */

		addval = 1;
		flush();			/* Flush caches */
		ret = do_stuff(ptr, 0);		/* Call execution buffer */

		if (ret != F1_RESULT) {		/* Compare result value */
			printf("Crash! Test 1, ret = %d\n", ret);
			goto fail;
		}

		for (i = 0; i < N_INST; i++)	/* Copy secnod function into */
			fun[i] = function_2[i];	/* execution buffer */

		addval = 2;
		flush();			/* Flush caches */

		ret = do_stuff(0, ptr);		/* Call execution buffer */

		if (ret != F2_RESULT) {		/* Compare result value */
			printf("Crash! Test 2, ret = %d\n", ret);
			goto fail;
		}
	}
	munmap(fun, MSIZE);
	printf("Self-modifying code test finished OK!\n");
	return 0;
fail:
	munmap(fun, MSIZE);
	printf("Self-modifying code test FAILED!\n");
fail1:
	return -1;
}
