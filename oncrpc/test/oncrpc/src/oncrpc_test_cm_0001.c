/******************************************************************************
  @file  oncrpc_test_cm_0001
  @brief Linux user-space cm test to verify utility functions

  DESCRIPTION
  Oncrpc test program for Linux user-space .

  -----------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when         who      what, where, why
--------     ---      -------------------------------------------------------
04/15/2009   rr       Initial version, based on oncrpc_test


======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "oncrpc.h"

#include "comdef.h"
#include "cm.h"
#include "cm_rpc.h"


#ifdef FEATURE_EXPORT_CM
   #include "cm.h"
   #include "cm_rpc.h"
#endif



/*=====================================================================
      Constants and Macros
======================================================================*/

#define CHAR_INPUT_MAX_LEN (100)
#define RESULT_ARRAY_SIZE  (5)
/*---------------------------------------------------------------------
  Type Definitions
---------------------------------------------------------------------*/
typedef struct
{
   uint32 toolvers;
   uint32 features;
   uint32 proghash;
   uint32 cbproghash;
}api_data_type;

typedef struct
{
   const char *name;
   const char *location;
   uint32   prog;
   uint32   vers;
   api_data_type  local_api_info;
   api_data_type  remote_api_info;
   api_data_type  match_api_info;
   int   verified;
   int   found;
   int   valid;
   int   used;
}oncrpc_lib_verify_status_type;



/*=====================================================================
     Forward Declaration
======================================================================*/
static oncrpc_lib_verify_status_type oncrpc_lib_verify_status[RESULT_ARRAY_SIZE];
static uint32 verify_status_index = 0;

static void check_api_status(uint32 prog,
        uint32 vers,
        api_data_type *remote_api_info,
        api_data_type *local_api_info);



/*===========================================================================
    FUNCTION  update_status_lookup
    ===========================================================================*/
/*!
@brief
Update and save the status of this API in local database

@return
None

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
void update_status_lookup(uint32 prog, uint32 vers, const char *name, const char *location,int found)
{
   int i;
   for(i=0;i<verify_status_index;i++)
   {
      if((prog == oncrpc_lib_verify_status[i].prog) && (vers == oncrpc_lib_verify_status[i].vers))
      {
         /* Exists in result db */
         oncrpc_lib_verify_status[i].found = found;
         oncrpc_lib_verify_status[i].name = name;
         return;
      }
   }
   if(verify_status_index >= RESULT_ARRAY_SIZE)
   {
      printf("Cannot add entry, database full \n");
      return;
   }
   /* Add a new entry*/
   oncrpc_lib_verify_status[verify_status_index].prog = prog;
   oncrpc_lib_verify_status[verify_status_index].vers = vers;
   oncrpc_lib_verify_status[verify_status_index].name = name;
   oncrpc_lib_verify_status[verify_status_index].location = location;
   oncrpc_lib_verify_status[verify_status_index].found = found;
   verify_status_index++;
}

static int mismatch_errors_found = 0;
static int api_not_found_error = 0;
/*===========================================================================
    FUNCTION  check_api_status
    ===========================================================================*/
/*!
@brief
Save the status of this API in local database

@return
None

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
void check_api_status(uint32 prog,
        uint32 vers,
        api_data_type *remote_api_info,
        api_data_type *local_api_info)
{
   uint32 i;
   int found_index=-1;
   for(i=0;i<verify_status_index;i++)
   {
      if((prog == oncrpc_lib_verify_status[i].prog) && (vers == oncrpc_lib_verify_status[i].vers))
      {
         /* API Exists in database, just update the data */
         oncrpc_lib_verify_status[i].local_api_info = *local_api_info;
         oncrpc_lib_verify_status[i].remote_api_info = *remote_api_info;
         found_index = i;
         break;
      }
   }

   if(found_index == -1)
   {
      if(verify_status_index >= RESULT_ARRAY_SIZE)
      {
         printf("ERROR Cannot add entry, database full \n");
         return;
      }

      /* Add a new entry*/
      oncrpc_lib_verify_status[verify_status_index].prog = prog;
      oncrpc_lib_verify_status[verify_status_index].vers = vers;
      oncrpc_lib_verify_status[verify_status_index].name = "unknown";
      oncrpc_lib_verify_status[verify_status_index].found = -1;
      oncrpc_lib_verify_status[verify_status_index].local_api_info = *local_api_info;
      oncrpc_lib_verify_status[verify_status_index].remote_api_info = *remote_api_info;
      found_index = verify_status_index;
      verify_status_index ++;
   }

   if(local_api_info->toolvers ==  remote_api_info->toolvers)
   {
      oncrpc_lib_verify_status[found_index].match_api_info.toolvers = 1;
   } else
   {
      printf(" Api 0x%08x Tool Version Differs Local:0x%08x, Remote:0x%08x \n",((int)prog),((int)local_api_info->toolvers),((int)remote_api_info->toolvers));
   }

   if(local_api_info->features ==  remote_api_info->features)
   {
      oncrpc_lib_verify_status[found_index].match_api_info.features = 1;
   } else
   {
      printf(" Api 0x%08x Features Flags Differs Local:0x%08x, Remote:0x%08x \n",
             ((int)prog),((int)local_api_info->features),((int)remote_api_info->features));
   }

   if((local_api_info->proghash ==  remote_api_info->proghash) && ( (local_api_info->proghash & 0x80000000) || (remote_api_info->proghash & 0x80000000) ))
   {
      oncrpc_lib_verify_status[found_index].match_api_info.proghash = 1;
      printf("Api Version is Hashkey and matches \n");
   } else
   {
      uint32 local_ver_lo   = local_api_info->proghash & 0x0000ffff;
      uint32 remote_ver_lo  = remote_api_info->proghash & 0x0000ffff;
      uint32 local_ver_hi   = local_api_info->proghash & 0x0fff0000;
      uint32 remote_ver_hi  = local_api_info->proghash & 0x0fff0000;
      uint32 local_ver_bwc  = ( local_api_info->proghash & 0x80000000) == 0;
      uint32 remote_ver_bwc = ( local_api_info->proghash & 0x80000000) == 0;
      if((local_ver_bwc && remote_ver_bwc ) &&
              (local_ver_hi == remote_ver_hi ) &&
              (local_ver_lo <= remote_ver_lo )
              )
      {
         printf(" Api Version is Hi/Lo Backwards compatible, compatibility verified \n");
         oncrpc_lib_verify_status[found_index].match_api_info.proghash = 1;
      } else
      {
         printf(" Api Version is Hi/Lo BWC, compatibility check FAILED \n");
         printf(" Local vers 0x%08x Remove vers 0x%08x \n",(unsigned int)local_api_info->proghash, (unsigned int)remote_api_info->proghash);
         oncrpc_lib_verify_status[found_index].match_api_info.proghash = 0;
      }
   }

   if((local_api_info->cbproghash ==  remote_api_info->cbproghash) && ( (local_api_info->cbproghash & 0x80000000) || (remote_api_info->cbproghash & 0x80000000) ))
   {
      oncrpc_lib_verify_status[found_index].match_api_info.cbproghash = 1;
      //printf("Api Callback Version is Hashkey and matches \n");
   } else
   {
      uint32 local_ver_lo   = local_api_info->cbproghash & 0x0000ffff;
      uint32 remote_ver_lo  = remote_api_info->cbproghash & 0x0000ffff;
      uint32 local_ver_hi   = local_api_info->cbproghash & 0x0fff0000;
      uint32 remote_ver_hi  = local_api_info->cbproghash & 0x0fff0000;
      uint32 local_ver_bwc  = ( local_api_info->cbproghash & 0x80000000) == 0;
      uint32 remote_ver_bwc = ( local_api_info->cbproghash & 0x80000000) == 0;
      if((local_ver_bwc && remote_ver_bwc ) &&
              (local_ver_hi == remote_ver_hi ) &&
              (local_ver_lo <= remote_ver_lo )
              )
      {
         //printf(" Api Callback Version is Hi/Lo Backwards compatible, compatibility verified \n");
         oncrpc_lib_verify_status[found_index].match_api_info.cbproghash = 1;
      } else
      {
         printf(" Api Callback Version is Hi/Lo BWC, compatibility check FAILED \n");
         printf(" Local vers 0x%08x Remove vers 0x%08x \n",(unsigned int)local_api_info->cbproghash, (unsigned int)remote_api_info->cbproghash);
         oncrpc_lib_verify_status[found_index].match_api_info.cbproghash = 0;
      }
   }

   /* Do not check for tool and features */
   if((oncrpc_lib_verify_status[found_index].match_api_info.proghash  == 1) &&
           (oncrpc_lib_verify_status[found_index].match_api_info.cbproghash == 1))
   {
      oncrpc_lib_verify_status[found_index].verified = 1;
      printf("API matches with remote server Api:0x%08x\n",((int)prog));
   } else
   {
      if(oncrpc_lib_verify_status[found_index].found == 1)
      {
         printf("\n***ERROR*** API DOES NOT MATCH with remote server Api:0x%08x\n",((int)prog));
         mismatch_errors_found++;
      }
   }
   printf("--------------------------------------------------------------------\n");
}

/*===========================================================================
  FUNCTION  print_status_db
===========================================================================*/
/*!
@brief
  Print the status of an API based on current database entries.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void print_status_db(void)
{
   uint32 i;
   int ver_hi;
   int ver_lo;
   for(i=0;i<verify_status_index;i++)
   {
      printf("%16s, Prog:0x%08x, V:0x%08x",
              oncrpc_lib_verify_status[i].name,
              (unsigned int)oncrpc_lib_verify_status[i].prog,
              (unsigned int)oncrpc_lib_verify_status[i].vers);
      if(oncrpc_lib_verify_status[i].found)
      {
         printf(" Found    ");
         ver_lo = (int)oncrpc_lib_verify_status[i].remote_api_info.toolvers & 0xffff;
         ver_hi = ((int)oncrpc_lib_verify_status[i].remote_api_info.toolvers & 0xffff0000)>>16;

         printf("V:%d.%d Feat:0x%02x ",((int)ver_hi),((int)ver_lo),((int)oncrpc_lib_verify_status[i].remote_api_info.features));
         if(oncrpc_lib_verify_status[i].verified)
         {
            printf(" OK ");
         } else
         {
            printf(" ERR ");
         }
      } else
      {
         printf(" Not Found");
         printf("V:X Feat:----  N/A");
      }

      printf("\n");
   }
   printf("--------------------------------------------------------------------\n");
}

void print_hex02(const byte *data,uint32 len)
{
   uint32 i;
   for(i=0;i<len;i++)
   {
      printf("0x%02x ",data[i]);
   }
   printf("\n");
}


int api_verify(void)
{
   uint32 prog;
   uint32 vers;
   uint32 found;
   api_data_type  remote_api_info;
   api_data_type  local_api_info;
   uint32 i;

   memset(oncrpc_lib_verify_status,0,sizeof(oncrpc_lib_verify_status_type)*RESULT_ARRAY_SIZE);
   verify_status_index = 0;

   for(i=0;i<RESULT_ARRAY_SIZE;i++)
   {
      oncrpc_lib_verify_status[i].remote_api_info.toolvers = -1;
      oncrpc_lib_verify_status[i].remote_api_info.features = 0;
      oncrpc_lib_verify_status[i].remote_api_info.proghash = 0xffffffff;
      oncrpc_lib_verify_status[i].remote_api_info.cbproghash = 0xffffffff;
      oncrpc_lib_verify_status[i].local_api_info.toolvers = -2;
      oncrpc_lib_verify_status[i].local_api_info.features = 0;
      oncrpc_lib_verify_status[i].local_api_info.proghash = 0xfffffffe;
      oncrpc_lib_verify_status[i].local_api_info.cbproghash = 0xfffffffe;
   }

#ifdef FEATURE_EXPORT_CM
   printf("\nVerifying API: CM \n");
   prog = CMPROG;
   vers = CMVERS;
   found = cm_null();
   if(found)
   {
      update_status_lookup(prog,vers,"CM","libs\\remote_apis\\cm",found);
      cm_rpc_glue_code_info_remote(&remote_api_info.toolvers,&remote_api_info.features,&remote_api_info.proghash,&remote_api_info.cbproghash);
      cm_rpc_glue_code_info_local(&local_api_info.toolvers,&local_api_info.features,&local_api_info.proghash,&local_api_info.cbproghash);
      check_api_status(prog,vers, &remote_api_info,&local_api_info);
   } else
   {
      api_not_found_error ++;
   }
#endif

   print_status_db();
   return api_not_found_error;
}

int cm_util_ascii_to_gsm_alphabet_test(void)
{
   int errcnt=0;

#ifdef ONCRPC_CM_UTIL_ASCII_TO_GSM_ALPHABET_PROC
#define CM_UTIL_ASCII_TO_GSM_ALPHABET_OUT_SIZE 256
   printf("\nCM_UTIL ASCII_TO_GSM_ALPHABET TEST \n");

   byte gsm_alphabet[CM_UTIL_ASCII_TO_GSM_ALPHABET_OUT_SIZE];
   const byte ascii_in[]="abcdefghijklmnopqrstuvwxyz";

   const byte expected_gsm_alphabet[]={0x61,0xf1,0x98,0x5c,0x36,0x9f,0xd1,0x69,0xf5,0x9a,0xdd,0x76,0xbf,0xe1,0x71,
      0xf9,0x9c,0x5e,0xb7,0xdf,0xf1,0x79,0x3d};


   memset(gsm_alphabet,0,CM_UTIL_ASCII_TO_GSM_ALPHABET_OUT_SIZE);
   byte num_output_bytes;

   printf("INPUT>>");
   printf("Ascii :%s",ascii_in);
   printf("\n");
   num_output_bytes = cm_util_ascii_to_gsm_alphabet(gsm_alphabet,ascii_in,strlen((char*)ascii_in));
   printf("Number of output bytes:%d \n",(unsigned int)num_output_bytes);
   printf("OUTPUT>>");
   print_hex02(gsm_alphabet,num_output_bytes);
   if(memcmp(expected_gsm_alphabet,gsm_alphabet,num_output_bytes)!=0)
   {
      printf("FAIL \n");
      printf("Expected:");
      print_hex02(expected_gsm_alphabet,sizeof(expected_gsm_alphabet));
      printf("Got:");
      print_hex02(gsm_alphabet,num_output_bytes);
      errcnt++;
   } else
   {
      printf("PASS\n");
   }
#else
    printf("API for %s is not present\n",__FUNCTION__);
    printf("PASS\n");
#endif

   return errcnt;
}


int cm_util_gsm_alphabet_to_ascii_test(void)
{
   int errcnt=0;
#ifdef ONCRPC_CM_UTIL_GSM_ALPHABET_TO_ASCII_PROC
#define CM_UTIL_GSM_ALPHABET_TO_ASCII_OUT_SIZE 256

   printf("\nCM_UTIL GSM_ALPHABET_TO_ASCII TEST \n");

   const byte expected_ascii[]="abcdefghijklmnopqrstuvwxyz";
   const byte gsm_alphabet_in[]={0x61,0xf1,0x98,0x5c,0x36,0x9f,0xd1,0x69,0xf5,0x9a,0xdd,0x76,0xbf,0xe1,0x71,
      0xf9,0x9c,0x5e,0xb7,0xdf,0xf1,0x79,0x3d};
   byte ascii_out[CM_UTIL_GSM_ALPHABET_TO_ASCII_OUT_SIZE];
   memset(ascii_out,0,CM_UTIL_GSM_ALPHABET_TO_ASCII_OUT_SIZE);
   byte num_output_bytes;

   printf("INPUT>>");
   print_hex02(gsm_alphabet_in,sizeof(gsm_alphabet_in));
   num_output_bytes = cm_util_gsm_alphabet_to_ascii(ascii_out,gsm_alphabet_in,sizeof(gsm_alphabet_in));
   printf("OUTPUT>>");
   printf("Ascii :%s",ascii_out);
   printf("\n");

   printf("Size %d \n",strlen((char*)expected_ascii));
   if(strncmp((char*)expected_ascii,(char*)ascii_out,strlen((char*)expected_ascii))!=0)
   {
      printf("FAIL \n");
      printf("Expected:%s  Got:%s \n",(char*)expected_ascii,(char*)ascii_out);
      errcnt++;
   } else
   {
      printf("PASS\n");
   }
   printf("\n");

#else
    printf("API for %s is not present\n",__FUNCTION__);
    printf("PASS\n");
#endif
   return errcnt;
}


int cm_util_pack_unpack_test(void)
{
   int errcnt=0;
#if (defined(ONCRPC_CM_UTIL_PACK_PROC) && defined (ONCRPC_CM_UTIL_UNPACK_PROC))

   byte num_chars;
#define CM_UTIL_PACK_TEST_SIZE 256

   const byte string_to_convert[]="abcdefghijklmnopqrstuvwxyz";
   byte pack_out[CM_UTIL_PACK_TEST_SIZE];
   byte unpack_out_string[CM_UTIL_PACK_TEST_SIZE];
   memset(pack_out,0,CM_UTIL_PACK_TEST_SIZE);
   num_chars = strlen((char*)string_to_convert);

   cm_util_pack(pack_out,num_chars,string_to_convert);
   printf("INPUT>>");
   printf("%s \n",string_to_convert);
   printf("\n");
   printf("OUTPUT>>");
   print_hex02(pack_out,32);
   cm_util_unpack(pack_out,num_chars,unpack_out_string);
   printf("UNPACK OUTPUT>>");
   printf("%s \n",unpack_out_string);
   printf("\n");

   if(strncmp((char*)unpack_out_string,(char*)string_to_convert,strlen((char*)string_to_convert))!=0)
   {
      printf("FAIL \n");
      printf("Expected:%s  Got:%s \n",(char*)string_to_convert,(char*)unpack_out_string);
      errcnt++;
   } else
   {
      printf("PASS\n");
   }
   printf("\n");
#else
    printf("API for %s is not present\n",__FUNCTION__);
    printf("PASS\n");
#endif
   return errcnt;
}

int cm_util_bcd_to_ascii_test(void)
{
   int errcnt=0;
#ifdef ONCRPC_CM_UTIL_BCD_TO_ASCII_PROC

#define CM_UTIL_BCD_TO_ASCII_BCD_SIZE 11
#define CM_UTIL_BCD_TO_ASCII_ASCII_SIZE 256
   printf("\nCM_UTIL BCD_TO_ASCII TEST \n");

   printf("Testing basic operation \n");
   byte bcd[CM_UTIL_BCD_TO_ASCII_BCD_SIZE]={0x0a,0x00,0x00,0x9a,0x78,0x56,0x34,0x12,0x10,0x032,0x054};
   byte ascii[CM_UTIL_BCD_TO_ASCII_ASCII_SIZE];
   const byte expected_ascii[]="*987654321012345";
   memset(ascii,0,CM_UTIL_BCD_TO_ASCII_ASCII_SIZE);

   printf("INPUT>>");
   printf("Bcd :");
   print_hex02(bcd,CM_UTIL_BCD_TO_ASCII_BCD_SIZE);
   cm_util_bcd_to_ascii(bcd,ascii);
   printf("OUTPUT>>");
   printf("Ascii %s \n",ascii);
   print_hex02(ascii,strlen((char*)ascii));

   if(strncmp((char*)expected_ascii,(char*)ascii,strlen((char*)expected_ascii))!=0)
   {
      printf("FAIL \n");
      printf("Expected:%s  Got:%s \n",(char*)expected_ascii,(char*)ascii);
      errcnt++;
   } else
   {
      printf("PASS\n");
   }

   printf("\n");
#else
    printf("API for %s is not present\n",__FUNCTION__);
    printf("PASS\n");
#endif
   return errcnt;
}

int cm_util_ascii_to_def_alphabet_test(void)
{
   int errcnt=0;
#if (defined(ONCRPC_CM_UTIL_DEF_ALPHABET_TO_ASCII_PROC) && defined(ONCRPC_CM_UTIL_ASCII_TO_DEF_ALPHABET_PROC))
   char in_ascii_string[1024] = "abcdefghijklmnopqrstuvwxyz";
   char out_alphabet_string[1024];
   char out_ascii_string[1024];
   printf("\nCM_UTIL ASCII_TO_DEF_ALPHABET <> TEST\n");
   printf("INPUT>> %s\n", in_ascii_string);
   cm_util_ascii_to_def_alphabet((byte*)out_alphabet_string,
           strlen(in_ascii_string),
           (byte*)in_ascii_string);//input
   printf("OUTPUT>> ");
   print_hex02((byte*)out_alphabet_string,strlen(in_ascii_string));

   printf("\nCM_UTIL DEF_ALPHABET_TOASCII <> TEST\n");
   printf("INPUT>> %s ",out_alphabet_string);
   cm_util_def_alphabet_to_ascii((byte*)out_alphabet_string, //input
           strlen((char*)out_alphabet_string)*8/7,
           (byte*)out_ascii_string);
   printf("OUTPUT>> %s\n", out_ascii_string);
   if(strncmp((char*)out_ascii_string,(char*)in_ascii_string,strlen((char*)in_ascii_string))!=0)
   {
      printf("FAIL \n");
      printf("Expected:%s  Got:%s \n",(char*)in_ascii_string,(char*)out_ascii_string);
      errcnt++;
   } else
   {
      printf("PASS\n");
   }
#else
    printf("API for %s is not present\n",__FUNCTION__);
    printf("PASS\n");
#endif
   return errcnt;
}



/*===========================================================================
  FUNCTION  main
===========================================================================*/
/*!
@brief
  entry to test

@return
  0

@note
  - Dependencies
    - ONCRPC, CM
  - Side Effects
*/
/*=========================================================================*/
int main(int argc, char *argv[])
{
   int api_not_found_error = 0;
   int api_errors = 0;
   oncrpc_init();
   oncrpc_task_start();
   cmcb_app_init();

   printf("ONCRPC_TEST_CM_0001 STARTED \n");
   api_not_found_error = api_verify();
   api_errors += cm_util_bcd_to_ascii_test();
   api_errors += cm_util_ascii_to_gsm_alphabet_test();
   api_errors += cm_util_gsm_alphabet_to_ascii_test();
   api_errors += cm_util_pack_unpack_test();
   api_errors += cm_util_ascii_to_def_alphabet_test();

   printf("ONCRPC_TEST_CM_0001 COMPLETE...\n");
   if((api_errors == 0) && (api_not_found_error == 0))
   {
      printf("PASS\n");
   } else
   {
      printf("FAIL\n");
   }
   oncrpc_deinit();
   return(-api_errors-api_not_found_error);
}


