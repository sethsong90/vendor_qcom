/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "pki.h"
#include <dlfcn.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>

#define LENGTH(x) (sizeof(x)/sizeof((x)[0]))

const char* Pki::userName;
const char* Pki::userPin;
char* Pki::salt;

bool Pki::isTimeBased = false;
bool Pki::pkcsModule  = false;
Pki* Pki::instance    = NULL_PTR;

CK_FUNCTION_LIST_PTR Pki::pFunctionList = NULL_PTR;
CK_SLOT_ID           Pki::currentSlotId;

CK_RV Pki::create_mutex(void **mutex)
{
  CK_RV ret = CKR_OK;

  if (!mutex) {
    ret = CKR_ARGUMENTS_BAD;
  } else {

    pthread_mutex_t *mut;
    mut = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (mut == NULL) {
      assert(mut);
      return CKR_HOST_MEMORY;
    }

    if (pthread_mutex_init(mut, NULL) != 0)
      assert(0);
    *mutex = mut;

  }

  return ret;
}

CK_RV Pki::destroy_mutex(void *mutex)
{
  CK_RV ret = CKR_OK;

  if (!mutex) {
    ret = CKR_MUTEX_BAD;
  } else {
    int res = pthread_mutex_destroy((pthread_mutex_t*) mutex);
    if (res != 0) {
      ret = CKR_GENERAL_ERROR;
    } else {
      free(mutex);
    }
  }
  return ret;
}

CK_RV Pki::lock_mutex(void *mutex)
{
  CK_RV ret = CKR_OK;

  if (!mutex) {
    ret = CKR_MUTEX_BAD;
  } else {
    int res = pthread_mutex_lock((pthread_mutex_t*) mutex);
    if (res != 0) {
      ret = CKR_GENERAL_ERROR;
    }
  }
  return ret;
}

CK_RV Pki::unlock_mutex(void *mutex)
{
  CK_RV ret = CKR_OK;

  if (!mutex) {
    ret = CKR_MUTEX_BAD;
  } else {
    int res = pthread_mutex_unlock((pthread_mutex_t*) mutex);
    if (res != 0) {
      ret = CKR_GENERAL_ERROR;
    }
  }
  return ret;
}

#define SALT "Salt"

Pki::Pki(const char* intStr){
  if(initModule(intStr)) {
    pkcsModule = true;
  }
  Pki::salt = (char*) malloc (strlen(SALT)+1);
  strlcpy(Pki::salt,SALT,(strlen(SALT)+1));
}

Pki::~Pki(){
  if (Pki::salt != NULL) {
    delete Pki::salt;
  }
}

bool
Pki::initModule(const char* intStr){
  CK_RV rv;
  CK_C_GetFunctionList pC_GetFunctionList = NULL_PTR;
  CK_C_Initialize      pC_Initialize      = NULL_PTR;

  LOGD("Pki::initModule+");

  void * module = dlopen("libSSEPKCS11.so", RTLD_NOW);
  if (module == NULL_PTR) {
    LOGE("Could not open library libSSEPKCS11.so : %s", dlerror());
    LOGD("Pki::initModule-");
    return false;
  }

  pC_GetFunctionList = (CK_C_GetFunctionList) dlsym(module, "C_GetFunctionList");
  if (pC_GetFunctionList == NULL_PTR) {
    LOGE("Could not C_GetFunctionList symbol in Lib");
    LOGD("Pki::initModule-");
    return false;
  }

  // initializing module functions
  // rv = C_GetFunctionList(&pFunctionList);
  rv = (pC_GetFunctionList)(&Pki::pFunctionList);
  if (CKR_OK != rv){
    LOGE("Error in C_GetFunctionList : 0x%x", rv);
    LOGD("Pki::initModule-");
    return false;
  }

  pC_Initialize = Pki::pFunctionList -> C_Initialize;
  // Call the C_Initialize function in the library
  CK_C_INITIALIZE_ARGS init_args;

  memset(&init_args, 0, sizeof(init_args));
  init_args.flags        = 0;
  init_args.CreateMutex  = Pki::create_mutex;
  init_args.DestroyMutex = Pki::destroy_mutex;
  init_args.LockMutex    = Pki::lock_mutex;
  init_args.UnlockMutex  = Pki::unlock_mutex;
  init_args.pReserved    = (CK_VOID_PTR)intStr;

  rv = (*pC_Initialize)(&init_args);
  if (CKR_OK != rv){
    LOGE("Error in C_Initialize : 0x%x", rv);
    LOGD("Pki::initModule-");
    return false;
  }
  LOGD("Pki::initModule-");
  return true;
}

Pki*
Pki::getPkiInstance(){
    return Pki::instance;
}

Pki*
Pki::initPkiInstance(const char* initLabel) {
  if ( NULL_PTR == Pki::instance ) {
    Pki::instance = new Pki(initLabel);
  }
  if ( NULL_PTR != Pki::instance ) {
    if ( false == Pki::instance->pkcsModule ) {
      delete Pki::instance;
      Pki::instance = NULL_PTR;
    }
  }
  return Pki::instance;
}

void
Pki::closePki() {
  LOGD("Pki::closePki()+");
  if ((NULL_PTR != Pki::instance)
    &&(Pki::instance->pkcsModule)) {

    CK_C_Finalize pC_Finalize = pFunctionList -> C_Finalize;
    if (NULL_PTR != pC_Finalize) {
      (*pC_Finalize)(NULL_PTR);
    }
    delete Pki::instance;
    Pki::instance = NULL_PTR;
  }
  LOGD("Pki::closePki()-");
}

const char*
Pki::getSalt(){
  LOGD("Pki::getSalt()+");
  LOGD("Pki::salt     - %s",    Pki::salt);
  LOGD("Pki::userPin  - %s",    Pki::userPin);
  LOGD("Pki::userName - %s",    Pki::userName);
  LOGD("Pki::getSalt()-");
  return Pki::salt;
}

void
Pki::setSalt(const char *aSalt){

  LOGD("Pki::setSalt()+");
  delete Pki::salt;
  Pki::salt = (char *)malloc(strlen(aSalt)+1);
  strlcpy(Pki::salt,aSalt, (strlen(SALT)+1));
  LOGD("Pki::salt     - %s",    Pki::salt);
  LOGD("Pki::userPin  - %s",    Pki::userPin);
  LOGD("Pki::userName - %s",    Pki::userName);
  LOGD("Pki::setSalt()-");
}
bool
Pki::setTimeBasedOTP(bool val){
   LOGD("Pki::setTimeBasedOTP()+ %d",val);
   if (val) {
     LOGD("Pki::setTimeBasedOTP()-TimeBased");
     Pki::isTimeBased = true;
   }else {
     LOGD("Pki::setTimeBasedOTP()-CounterBased");
     Pki::isTimeBased = false;
   }
   LOGD("Pki::setTimeBasedOTP()-");
   return true;
}

bool
Pki::obtainToken(const char* tokenLabel){
  CK_ULONG ulCount;
  CK_SLOT_ID_PTR pSlotList;
  CK_SLOT_INFO slotInfo;
  CK_TOKEN_INFO tokenInfo;
  CK_RV rv;

  LOGD("Pki::obtainToken+");
  rv = Pki::pFunctionList -> C_GetSlotList(CK_FALSE,
                                           NULL_PTR,
                                           &ulCount);
  if(rv != CKR_OK) {
    LOGE("C_GetSlotList error rv = 0x%x",rv);
    LOGD("Pki::obtainToken-");
    return false;
  }

  if (0 == ulCount) {
    LOGE("ulCount error : ulCount = %d",ulCount);
    LOGD("Pki::obtainToken-");
    return false;
  }
  else {
    LOGD("ulCount = %d",ulCount);
    pSlotList = (CK_SLOT_ID_PTR) malloc ( ulCount * sizeof(CK_SLOT_ID));
    if (NULL == pSlotList) {
      LOGE("(CK_SLOT_ID_PTR) malloc failed");
      LOGD("Pki::obtainToken-");
      return false;
    }
    rv = Pki::pFunctionList -> C_GetSlotList(CK_FALSE,
                                             pSlotList,
                                             &ulCount);
    if (rv != CKR_OK){
      LOGE("C_GetSlotList error rv = 0x%x",rv);
      LOGD("Pki::obtainToken-");
      return false;
    }

    for (unsigned int i = 0; i < ulCount ; i++) {
      /* Get slot information for the slot */
      rv = Pki::pFunctionList -> C_GetSlotInfo(pSlotList[i],
                                               &slotInfo);
      if (rv != CKR_OK) {
        LOGE("C_GetSlotInfo error rv = 0x%x",rv);
        continue;
      }
      else {
        if (slotInfo.flags & CKF_TOKEN_PRESENT) {
          /* Get token information for the slot */
          rv = Pki::pFunctionList -> C_GetTokenInfo(pSlotList[i],
                                                    &tokenInfo);
          if (rv == CKR_TOKEN_NOT_PRESENT) {
            LOGE("C_GetTokenInfo error rv after slotInfo.flags.CKF_TOKEN_PRESENT");
            /* look in the next slot */
            continue;
          }
          /* need token without Secure Keypad*/
          if( tokenInfo.flags & CKF_PROTECTED_AUTHENTICATION_PATH  ) {
            /* look in the next slot */
            continue;
          }

          /* ckeck if the token is already initialized */
          if( tokenInfo.flags & CKF_TOKEN_INITIALIZED  ) {
            LOGD("Token already initialized");
            LOGD("current slot == %d", pSlotList[i]);
            currentSlotId = pSlotList[i];
            break;
          }else {
            LOGD("Token not initialized");
            int labelLen  = strlen(tokenLabel);
            CK_UTF8CHAR label[32];
            memset(label, ' ', sizeof(label));
            memcpy(label, tokenLabel, labelLen );

            rv = Pki::pFunctionList -> C_InitToken(pSlotList[i],
                                                 (CK_UTF8CHAR_PTR)Pki::userPin,
                                                 strlen(Pki::userPin),
                                                 label);
            if (rv != CKR_OK) {
              LOGE("C_InitToken error rv = 0x%x", rv);
              continue;
            } else {
              LOGD("current slot == %d", pSlotList[i]);
              currentSlotId = pSlotList[i];
              break;
            }
          }
        } else {
           rv = CKR_OPERATION_NOT_INITIALIZED;
        }
      }
    }
    free(pSlotList);
  }
  if (rv != CKR_OK) {
    LOGD("Pki::obtainToken- error 0x%x",rv);
    return false;
  }
  LOGD("Pki::obtainToken-");
  return true;
}


bool
Pki::createOTPKey(const char* pPassword, const char* pKeyLabel){

  CK_SESSION_HANDLE hSession;
  CK_OBJECT_HANDLE  object;
  CK_RV rv;

  LOGD("Pki::createOTPKey+ %s , %s", pPassword, pKeyLabel );

  rv = Pki::pFunctionList -> C_OpenSession( currentSlotId,
                                            CKF_SERIAL_SESSION | CKF_RW_SESSION,
                                            (CK_VOID_PTR) NULL_PTR,
                                            NULL_PTR,
                                            &hSession);
  if (rv != CKR_OK) {
    LOGE("C_OpenSession failed with rv = 0x%x", rv);
    LOGD("Pki::createOTPKey-");
    return false;
  }

  LOGD("Pki::createOTPKey - pPki::salt    - %s",    Pki::salt);
  LOGD("Pki::createOTPKey - Pki::userPin  - %s",    Pki::userPin);
  LOGD("Pki::createOTPKey - Pki::userName - %s",    Pki::userName);

  rv = Pki::pFunctionList -> C_Login( hSession,
                                      CKU_USER,
                                      (CK_UTF8CHAR_PTR)Pki::userPin,
                                      strlen(Pki::userPin));
  if (rv != CKR_OK) {
    LOGE("C_Login failed with rv = 0x%x", rv);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::createOTPKey-");
    return false;
  }

  CK_ULONG passwordLength = strlen(pPassword);
  CK_PKCS5_PBKD2_PARAMS pkcs_pbkd2_param;

  pkcs_pbkd2_param.saltSource          = 1;
  pkcs_pbkd2_param.pSaltSourceData     = (CK_VOID_PTR)Pki::salt;
  pkcs_pbkd2_param.ulSaltSourceDataLen = strlen(Pki::salt);
  pkcs_pbkd2_param.iterations          = 100;
  pkcs_pbkd2_param.prf                 = CKP_PKCS5_PBKD2_HMAC_SHA256;
  pkcs_pbkd2_param.pPrfData            = NULL;
  pkcs_pbkd2_param.ulPrfDataLen        = 0;
  pkcs_pbkd2_param.pPassword           = (CK_UTF8CHAR_PTR)pPassword;
  pkcs_pbkd2_param.ulPasswordLen       = &passwordLength;


  LOGD("Pki::createOTPKey - pkcs_pbkd2_param.pSaltSourceData - %s",    pkcs_pbkd2_param.pSaltSourceData);
  LOGD("Pki::createOTPKey - pkcs_pbkd2_param.ulSaltSourceDataLen - %d",pkcs_pbkd2_param.ulSaltSourceDataLen);

  LOGD("Pki::createOTPKey - pkcs_pbkd2_param.pPassword  - %s",         pkcs_pbkd2_param.pPassword);
  LOGD("Pki::createOTPKey - pkcs_pbkd2_param.ulPasswordLen - %d",      (CK_ULONG)*pkcs_pbkd2_param.ulPasswordLen);

  CK_MECHANISM mech = { CKM_PKCS5_PBKD2,
                        &pkcs_pbkd2_param,
                        sizeof(CK_PKCS5_PBKD2_PARAMS)};

  CK_BBOOL _true           = CK_TRUE;
  CK_BBOOL _false          = CK_FALSE;
  CK_OBJECT_CLASS otpClass = CKO_OTP_KEY;
  CK_KEY_TYPE hotpKeyType  = CKK_HOTP;
  CK_ULONG otpDecimal      = CK_OTP_FORMAT_DECIMAL;

  CK_ULONG otpOutLen  = 6;
  CK_ULONG _length32  = 32;

  CK_ULONG otpReq = CK_OTP_PARAM_MANDATORY;
  CK_ULONG otpOpt = CK_OTP_PARAM_OPTIONAL;
  CK_ULONG otpIgn = CK_OTP_PARAM_IGNORED;

  CK_BYTE otpCounter[8] = {0};

  if (isTimeBased) {
    LOGD("Pki::createOTPKey-TimeBased");
    CK_ATTRIBUTE _hotpSecretKey32Time[] = {
         {CKA_CLASS,                                 &otpClass,      sizeof(otpClass)},
         {CKA_KEY_TYPE,                              &hotpKeyType,   sizeof(hotpKeyType)},
         {CKA_LABEL,                    (CK_VOID_PTR)pKeyLabel,      strlen(pKeyLabel)},
         {CKA_TOKEN,                                 &_true,         sizeof(_true)},
         {CKA_SENSITIVE,                             &_true,         sizeof(_true)},
         {CKA_SIGN,                                  &_true,         sizeof(_true)},
         {CKA_VERIFY,                                &_true,         sizeof(_true)},

         {CKA_OTP_LENGTH,                            &otpOutLen,     sizeof(otpOutLen)},
         {CKA_OTP_CHALLENGE_REQUIREMENT,             &otpIgn,        sizeof(otpIgn)},
         {CKA_OTP_PIN_REQUIREMENT,                   &otpIgn,        sizeof(otpIgn)},

         {CKA_OTP_TIME_REQUIREMENT,                  &otpReq,        sizeof(otpReq)},
         {CKA_OTP_COUNTER_REQUIREMENT,               &otpIgn,        sizeof(otpIgn)},

         {CKA_VALUE_LEN,                             &_length32,     sizeof(_length32)},
         {CKA_OTP_USER_IDENTIFIER,      (CK_VOID_PTR)Pki::userName,  strlen(Pki::userName)},

         {CKA_OTP_FORMAT,                            &otpDecimal,    sizeof(otpDecimal)},
         {CKA_OTP_COUNTER,                           otpCounter,     sizeof(otpCounter)},
    };
    rv = Pki::pFunctionList -> C_GenerateKey ( hSession,
                                               &mech,
                                               _hotpSecretKey32Time,
                                               16,
                                               &object);
  } else  {
    LOGD("Pki::createOTPKey-counterBased");
    CK_ATTRIBUTE _simpleHotp32Counter[] = {
         {CKA_CLASS,                                 &otpClass,      sizeof(otpClass)},
         {CKA_KEY_TYPE,                              &hotpKeyType,   sizeof(hotpKeyType)},
         {CKA_LABEL,                    (CK_VOID_PTR)pKeyLabel,      strlen(pKeyLabel)},
         {CKA_TOKEN,                                 &_true,         sizeof(_true)},
         {CKA_SENSITIVE,                             &_true,         sizeof(_true)},
         {CKA_SIGN,                                  &_true,         sizeof(_true)},
         {CKA_VERIFY,                                &_true,         sizeof(_true)},

         {CKA_OTP_LENGTH,                            &otpOutLen,     sizeof(otpOutLen)},
         {CKA_OTP_CHALLENGE_REQUIREMENT,             &otpIgn,        sizeof(otpIgn)},
         {CKA_OTP_PIN_REQUIREMENT,                   &otpIgn,        sizeof(otpIgn)},

         {CKA_OTP_TIME_REQUIREMENT,                  &otpIgn,        sizeof(otpIgn)},
         {CKA_OTP_COUNTER_REQUIREMENT,               &otpOpt,        sizeof(otpOpt)},

         {CKA_VALUE_LEN,                             &_length32,     sizeof(_length32)},
         {CKA_OTP_USER_IDENTIFIER,      (CK_VOID_PTR)Pki::userName,  strlen(Pki::userName)},

         {CKA_OTP_FORMAT,                            &otpDecimal,    sizeof(otpDecimal)},
         {CKA_OTP_COUNTER,                           otpCounter,     sizeof(otpCounter)},
    };
    rv = Pki::pFunctionList -> C_GenerateKey ( hSession,
                                               &mech,
                                               _simpleHotp32Counter,
                                               16,
                                               &object);
  }
  if (rv != CKR_OK) {
    LOGE("C_GenerateKey failed with rv = 0x%x", rv);
    Pki::pFunctionList -> C_Logout(hSession);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::createOTPKey-");
    return false;
  }

  Pki::pFunctionList -> C_Logout(hSession);
  Pki::pFunctionList -> C_CloseSession(hSession);
  LOGD("Pki::createOTPKey-");
  return true;
}

CK_OBJECT_HANDLE
Pki::retrieveKey(const char* keyLabel){

  CK_SESSION_HANDLE hSession;
  CK_RV rv;

  LOGD("Pki::retrieveKey+");

  rv = Pki::pFunctionList -> C_OpenSession( currentSlotId,
                                            CKF_SERIAL_SESSION | CKF_RW_SESSION,
                                            (CK_VOID_PTR) NULL_PTR,
                                            NULL_PTR,
                                            &hSession);
  if (rv != CKR_OK) {
    LOGE("C_OpenSession failed with rv = 0x%x", rv);
    LOGD("Pki::retrieveKey-");
    return NULL_PTR;
  }

  rv = Pki::pFunctionList -> C_Login( hSession,
                                      CKU_USER,
                                      (CK_UTF8CHAR_PTR)Pki::userPin,
                                      strlen(Pki::userPin));
  if (rv != CKR_OK) {
    LOGE("C_Login failed with rv = 0x%x", rv);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::retrieveKey-");
    return NULL_PTR;
  }

  CK_OBJECT_HANDLE hObject;
  CK_ULONG ulObjectCount;
  CK_ATTRIBUTE _Key[] = {
    {CKA_LABEL,        (CK_VOID_PTR)keyLabel,     strlen(keyLabel)},
  };

  rv = Pki::pFunctionList -> C_FindObjectsInit(hSession, _Key, 1);
  if (rv != CKR_OK) {
    LOGE("C_FindObjectsInit failed with rv = 0x%x", rv);
    Pki::pFunctionList -> C_Logout(hSession);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::retrieveKey-");
    return NULL_PTR;
  }

  while (1) {
    rv = Pki::pFunctionList -> C_FindObjects(hSession,
                                             &hObject,
                                             1,
                                             &ulObjectCount);
    if (rv != CKR_OK || ulObjectCount == 0){
      LOGE("C_FindObjects failed with rv = 0x%x", rv);
      Pki::pFunctionList -> C_Logout(hSession);
      Pki::pFunctionList -> C_CloseSession(hSession);
      LOGD("Pki::retrieveKey-");
      break;
    }
    else {

      Pki::pFunctionList -> C_FindObjectsFinal(hSession);
      Pki::pFunctionList -> C_Logout(hSession);
      Pki::pFunctionList -> C_CloseSession(hSession);
      LOGD("Found Key");
      LOGD("Pki::retrieveKey-");
      return hObject;
    }
  }
  rv = Pki::pFunctionList -> C_FindObjectsFinal(hSession);
  if (rv != CKR_OK) {
    LOGE("C_FindObjectsFinal failed with rv = 0x%x", rv);
    Pki::pFunctionList -> C_Logout(hSession);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::retrieveKey-");
    return NULL_PTR;
  }
  //not found
  Pki::pFunctionList -> C_Logout(hSession);
  Pki::pFunctionList -> C_CloseSession(hSession);
  LOGD("Pki::retrieveKey-");
  return NULL_PTR;
}

bool
Pki::deleteOTPKey(const char* keyLabel){

  CK_SESSION_HANDLE hSession;
  CK_RV rv;

  LOGD("Pki::deleteOTPKey+");

  CK_OBJECT_HANDLE hObject = retrieveKey(keyLabel);
  if (NULL_PTR == hObject) {
    LOGE("retrieveKey returned NULL ");
    LOGD("Pki::deleteOTPKey-");
    return false;
  }

  rv = Pki::pFunctionList -> C_OpenSession( currentSlotId,
                                            CKF_SERIAL_SESSION | CKF_RW_SESSION,
                                            (CK_VOID_PTR) NULL_PTR,
                                            NULL_PTR,
                                            &hSession);
  if (rv != CKR_OK) {
    LOGE("C_OpenSession failed with rv = 0x%x", rv);
    LOGD("Pki::deleteOTPKey-");
    return false;
  }

  rv = Pki::pFunctionList -> C_Login( hSession,
                                      CKU_USER,
                                      (CK_UTF8CHAR_PTR)Pki::userPin,
                                      strlen(Pki::userPin));
  if (rv != CKR_OK) {
    LOGE("C_Login failed with rv = 0x%x", rv);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::deleteOTPKey-");
    return false;
  }

  rv = Pki::pFunctionList -> C_DestroyObject(hSession,
                                             hObject);
  if (rv != CKR_OK) {
    LOGE("C_DestroyObject failed with rv = 0x%x", rv);
    Pki::pFunctionList -> C_Logout(hSession);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::deleteOTPKey-");
    return false;
  }

  Pki::pFunctionList -> C_Logout(hSession);
  Pki::pFunctionList -> C_CloseSession(hSession);
  LOGD("Pki::deleteOTPKey-");
  return true;
}

long
Pki::generateOTP(CK_OBJECT_HANDLE key_handle){

  CK_SESSION_HANDLE hSession;
  CK_RV rv;

  LOGD("Pki::generateOTP+");

  if (NULL_PTR == key_handle) {
    LOGE("Key is NULL");
    LOGD("Pki::generateOTP-");
    return 0;
  }

  rv = Pki::pFunctionList -> C_OpenSession( currentSlotId,
                                            CKF_SERIAL_SESSION | CKF_RW_SESSION,
                                            (CK_VOID_PTR) NULL_PTR,
                                            NULL_PTR,
                                            &hSession);
  if (rv != CKR_OK) {
    LOGE("C_OpenSession failed with rv = 0x%x", rv);
    LOGD("Pki::generateOTP-");
    return 0;
  }

  rv = Pki::pFunctionList -> C_Login( hSession,
                                      CKU_USER,
                                      (CK_UTF8CHAR_PTR)Pki::userPin,
                                      strlen(Pki::userPin));
  if (rv != CKR_OK) {
    LOGE("C_Login failed with rv = 0x%x", rv);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::generateOTP-");
    return 0;
  }

  CK_ULONG timeStatus;
  CK_ULONG countStatus;
  CK_ATTRIBUTE Attr[] = {{CKA_OTP_TIME_REQUIREMENT,    &timeStatus,  sizeof(timeStatus)},
                         {CKA_OTP_COUNTER_REQUIREMENT, &countStatus, sizeof(countStatus)},};
  rv = Pki::pFunctionList -> C_GetAttributeValue(hSession, key_handle, Attr, 2);
  if (rv == CKR_OK) {
    if (-1 != (CK_LONG)Attr[0].ulValueLen ) {
      if ( CK_OTP_PARAM_MANDATORY == timeStatus) {
        LOGD("CKA_OTP_TIME_REQUIREMENT field is CK_OTP_PARAM_MANDATORY");
        isTimeBased = true;
      }else if(CK_OTP_PARAM_IGNORED == timeStatus){
        LOGD("CKA_OTP_TIME_REQUIREMENT field is CK_OTP_PARAM_IGNORED");
        isTimeBased = false;
      }else {
        LOGE("CKA_OTP_TIME_REQUIREMENT field in the key is neither CK_OTP_PARAM_IGNORED nor CK_OTP_PARAM_MANDATORY");
        Pki::pFunctionList -> C_Logout(hSession);
        Pki::pFunctionList -> C_CloseSession(hSession);
        LOGD("Pki::generateOTP-");
      }
    }else{
      LOGE("C_GetAttributeValue returned  CKA_OTP_TIME_REQUIREMENT.ulValueLen as -1 ");
      Pki::pFunctionList -> C_Logout(hSession);
      Pki::pFunctionList -> C_CloseSession(hSession);
      LOGD("Pki::generateOTP-");
    }
  }else {
    LOGE("C_GetAttributeValue returned error rv = 0x%x", rv);
    Pki::pFunctionList -> C_Logout(hSession);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::generateOTP-");
  }

  struct timespec res;
  clock_gettime(CLOCK_REALTIME, &res);

  CK_BYTE *pOTP;
  CK_ULONG ulOTPLen;
  CK_ULONG time     = res.tv_sec;
  CK_ULONG i        = 0;
  CK_ULONG counter  = 0;

  LOGD("Pki::generateOTP-------time----------> %d", time);

  CK_FLAGS otpFlagsCounter = CKF_EXCLUDE_TIME | CKF_EXCLUDE_CHALLENGE | CKF_EXCLUDE_PIN;
  CK_FLAGS otpFlagsTimer   = CKF_EXCLUDE_COUNTER | CKF_EXCLUDE_CHALLENGE| CKF_EXCLUDE_PIN;

  CK_ULONG format    = CK_OTP_FORMAT_DECIMAL;
  CK_ULONG otpOutLen = 6;
  CK_BYTE_PTR aux;

  CK_OTP_PARAM paramCounter[] = {
      {CK_OTP_OUTPUT_FORMAT,  &format,              sizeof(CK_ULONG)},
      {CK_OTP_FLAGS,          &otpFlagsCounter,     sizeof(otpFlagsCounter)},
      {CK_OTP_OUTPUT_LENGTH,  &otpOutLen,           sizeof(otpOutLen)},
  };

  CK_OTP_PARAM paramTime[] = {
      {CK_OTP_OUTPUT_FORMAT,  &format,            sizeof(CK_ULONG)},
      {CK_OTP_FLAGS,          &otpFlagsTimer,     sizeof(otpFlagsTimer)},
      {CK_OTP_OUTPUT_LENGTH,  &otpOutLen,         sizeof(otpOutLen)},
      {CK_OTP_TIME,           &time,              sizeof(time)},
  };

  if (isTimeBased) {
    LOGD("Pki::generateOTP-TimeBased");
    CK_OTP_PARAMS params = {paramTime, LENGTH(paramTime)};
    CK_MECHANISM hotpMechanism = {CKM_HOTP,
                                  &params,
                                  sizeof(params)};

    rv = Pki::pFunctionList -> C_SignInit(hSession, &hotpMechanism, key_handle);
  } else {
    LOGD("Pki::generateOTP-counterBased");
    CK_OTP_PARAMS params = {paramCounter, LENGTH(paramCounter)};
    CK_MECHANISM hotpMechanism = {CKM_HOTP,
                                  &params,
                                  sizeof(params)};
    rv = Pki::pFunctionList -> C_SignInit(hSession, &hotpMechanism, key_handle);
  }

  if (rv != CKR_OK) {
    LOGE("C_SignInit failed with rv = 0x%x", rv);
    Pki::pFunctionList -> C_Logout(hSession);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::generateOTP-");
    return 0;
  }

  /* Get the buffer length needed for the OTP Value
     and any associated data. */
  rv = Pki::pFunctionList -> C_Sign(hSession, NULL_PTR, 0, NULL_PTR, &ulOTPLen);
  if (rv != CKR_OK) {
    LOGE("C_Sign failed while getting length with rv = 0x%x", rv);
    Pki::pFunctionList -> C_Logout(hSession);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::generateOTP-");
    return 0;
  };

  if ((pOTP = (CK_BYTE* )malloc(ulOTPLen)) == NULL_PTR) {
    LOGE("malloc failed for size %d bytes", ulOTPLen);
    Pki::pFunctionList -> C_Logout(hSession);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::generateOTP-");;
    return 0;
  };

  /* Get the actual OTP value and any
     associated data. */
  rv = Pki::pFunctionList -> C_Sign(hSession, NULL_PTR, 0, pOTP,&ulOTPLen);
  if (rv != CKR_OK) {
    LOGE("C_Sign failed with rv = 0x%x", rv);
    Pki::pFunctionList -> C_Logout(hSession);
    Pki::pFunctionList -> C_CloseSession(hSession);
    LOGD("Pki::generateOTP-");
    return 0;
  }

  CK_OTP_PARAMS_PTR outParamsPtr    = (CK_OTP_PARAMS_PTR)pOTP;
  CK_OTP_PARAM_PTR  outParamItemPtr = (CK_OTP_PARAM_PTR) outParamsPtr->pParams;
  CK_ULONG outParamItemCount        = outParamsPtr->ulCount;
  CK_ULONG result = 0;
  for (unsigned int i =0; i< outParamItemCount ; ++i) {
    LOGD("Out Param Type: %d  Length : %d",outParamItemPtr->type, outParamItemPtr->ulValueLen);
    if (CK_OTP_VALUE == outParamItemPtr->type  ) {
      result = *((CK_ULONG *)outParamItemPtr->pValue);
      LOGD("----- Found OTP value: %d",result);
    }else {
      outParamItemPtr++;
    }
  }

  Pki::pFunctionList -> C_Logout(hSession);
  Pki::pFunctionList -> C_CloseSession(hSession);
  LOGD("Pki::generateOTP-  %d ", result);
  return result;
}

