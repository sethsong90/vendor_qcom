/******************************************************************************
 * @file  kgsl_test.c
 * @brief
 *
 * User-space unit test application for the kgsl.
 *
 * -----------------------------------------------------------------------------
 * Copyright (c) 2011-2012 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * -----------------------------------------------------------------------------
 ******************************************************************************/
#include "kgsl_helper.h"

#include <getopt.h>

#define NUM_CMDS 6

#define NOMINAL_T 0
#define ADV_T 1
#define REPEAT_T 2
#define STRESS_T 3
#define HELP 4

#define LOOP_CNT 2

static int cmd_switch[NUM_CMDS];
unsigned int kgsl_test_log_level = KGSL_DEF_LOG_LEVEL;
char *dev_name;

static void usage(void)
{
	printf
	    ("Usage: kgsl_test [-v] [-n] [-a] [-r] [-s] [-h] [--dev <dev_path>]\n");
	printf("-v Print debug messages. Error & Info by default\n");
	printf("-n Nominal Test\n");
	printf("-a Adversarial\n");
	printf("-r Repeatability Test\n");
	printf("-s Stress Test\n");
	printf("--dev path to dev (e.g. /dev/kgsl-3d0)\n");
	printf("-h Prints this help message and exits\n");
}
static struct option options[] = {
	{"verbose", 0, 0, 'v'},
	{"nominal", 0, 0, 'n'},
	{"adversarial", 0, 0, 'a'},
	{"repeatability", 0, 0, 'r'},
	{"stress", 0, 0, 's'},
	{"trinity", 0, 0, 't'},
	{"help", 0, 0, 'h'},
	{"dev", 1, 0, 'd'},
	{0, 0, 0, 0}
};

static void parse_options(int argc, char **argv)
{
	int opt = 0;
	int i;
	int got_testopt = 0;

	for (i = 0; i < NUM_CMDS; i++)
		cmd_switch[i] = 0;

	do {
		opt = getopt_long(argc, argv,"vnarsthd:", options, NULL);
		switch (opt) {
		case -1:
			break;
		case 'v':
			kgsl_test_log_level = KGSL_LOG_LEVEL_ALL;
			break;
		case 'n':
			cmd_switch[NOMINAL_T] = 1;
			got_testopt = 1;
			break;
		case 'a':
			cmd_switch[ADV_T] = 1;
			got_testopt = 1;
			break;
		case 'r':
			cmd_switch[REPEAT_T] = 1;
			got_testopt = 1;
			break;
		case 's':
			cmd_switch[STRESS_T] = 1;
			got_testopt = 1;
			break;
		case 'd':
			dev_name = strndup(optarg, 50);
			break;
		case 'h':
			usage();
			exit(0);
			break;
		default:
			printf("Unknown command argument\n");
			usage();
			exit(-1);
			break;
		}
	} while (opt >= 0);

	if (!got_testopt)
		cmd_switch[NOMINAL_T] = 1;

	if (dev_name == NULL)
		dev_name = strndup(DEF_DEVICE, 50);
}

/*Used to safely exit the test. Failures and end of test end here*/
void kgsl_test_exit(int result, int expected, const char *name)
{
	free(dev_name);
	if (result == expected) {
		printf("Test Passed\n");
		exit(0);
	} else {
		fprintf(stderr, "***ERROR %s <id%d> failed errno: %d %s", name,
			(int)pthread_self, result, strerror(result));
		fprintf(stderr, "\n\nTest Failed\n");
		exit(-1);
	}
}

int main(int argc, char **argv)
{

	int result = 0;
	parse_options(argc, argv);

	if (cmd_switch[NOMINAL_T]) {
		TEST_STEP(0, result, kgsl_nominal(), "\nNominal Test\n\n");
		kgsl_test_log(KGSL_LOG_LEVEL_INFO, "\nFinished Nominal\n\n");
	}
	if (cmd_switch[ADV_T]) {
		TEST_STEP(0, result, kgsl_adv(), "\nAdversarial Test\n\n");
		kgsl_test_log(KGSL_LOG_LEVEL_INFO,
			      "\nFinished Adversarial\n\n");
	}
	if (cmd_switch[STRESS_T]) {
		TEST_STEP(0, result, kgsl_stress(), "\nStress Test\n\n");
		kgsl_test_log(KGSL_LOG_LEVEL_INFO, "\nFinished Stress\n\n");
	}
	if (cmd_switch[REPEAT_T]) {
		TEST_STEP(0, result, kgsl_repeat(LOOP_CNT),
			  "\nRepeat Test\n\n");
		kgsl_test_log(KGSL_LOG_LEVEL_INFO, "\nFinished Repeat\n\n");
	}
	kgsl_test_exit(0, 0, "a");

	return 0;
}
