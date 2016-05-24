/**
 * Copyright (c) 2012 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Confidential and Proprietary.
 */

#pragma once

#ifdef _WIN32
#include "windows.h"
#undef uuid_t


/* Specifies that the function is a DLL entry point. */
#define IMPORT_SPEC __declspec(dllimport)

/* Define CRYPTOKI_EXPORTS during the build of cryptoki libraries. Do
 * not define it in applications.
 */
#ifdef LIB_EXPORTS
/* Specified that the function is an exported DLL entry point. */
#define EXPORT_SPEC __declspec(dllexport)
#else
#define EXPORT_SPEC IMPORT_SPEC
#endif

/* Ensures the calling convention for Win32 builds */
#define CALL_SPEC __cdecl

#else

#define IMPORT_SPEC
#define EXPORT_SPEC
#define CALL_SPEC

#endif


