/*
 * Copyright (c) 2012, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * Developed by QRD Engineering team.
 */
#define DEBUG_ON 0

#ifndef _ROOTAGENT_UTILS
#define _ROOTAGENT_UTILS
#include "stdio.h"
#include <stdlib.h>
#include <string>
using namespace std;

#if !DEBUG_ON
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>
#else
#include "iostream"
#endif

void logd(int d);
void logd(char* s);
void logd(char c);
void logd(string s);
int mkDirs(const char *path);
void writeFile(const char* path, const char* content );
#endif
