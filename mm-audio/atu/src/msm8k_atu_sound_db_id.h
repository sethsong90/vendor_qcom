/*
 *
 * Copyright (C) 2009 Qualcomm Technologies, Inc.
 *
 */

enum atu_sounds {		      /* Description of sound                 */
	ATU_MIN_COMPACT_ATU = 0,      /* Use for range check of first compact sound*/
	ATU_ALERT = 0,                /* 0 -  Ringing for incoming call            */
	ATU_WAKEUP,                   /* Wake-up/Power-up sound                    */
	ATU_DIAL_TONE,                /* Dial tone                                 */
	ATU_DTACO_ROAM_TONE,          /* DTACO roaming dial tone                   */
	ATU_RING_BACK,                /* Ring-back sound                           */
	ATU_INTERCEPT,                /* Send request intercepted locally          */
	ATU_REORDER,                  /* System busy                               */
	ATU_BUSY_ALERT,               /* Busy Signal                               */
	ATU_CONFIRMATION,             /* Confirmation Tone                         */
	ATU_CALL_WAITING,             /* Call Waiting                              */
	ATU_ANSWER,                   /* 10 - Answer Tone                          */
	ATU_OFF_HOOK,                 /* Off-Hook Warning                          */
	ATU_NORMAL_ALERT,             /* "Normal" Alerting                         */
	ATU_INTR_GROUP_ALERT,         /* Intergroup Alerting                       */
	ATU_SPCL_ALERT,               /* Special/Priority Alerting                 */
	ATU_PING_RING,                /* "Ping ring"                               */
	ATU_IS54B_LONG_H_ALERT,       /* IS-54B High Long                          */
	ATU_IS54B_SS_H_ALERT,         /* IS-54B High Short-short                   */
	ATU_IS54B_SSL_H_ALERT,        /* IS-54B High Short-short-long              */
	ATU_IS54B_SS2_H_ALERT,        /* IS-54B High Short-short-2                 */
	ATU_IS54B_SLS_H_ALERT,        /* 20 - IS-54B High Short-long-short         */
	ATU_IS54B_SSSS_H_ALERT,       /* IS-54B High Short-short-short-short       */
	ATU_IS54B_PBX_LONG_H_ALERT,   /* IS-54B High PBX Long                      */
	ATU_IS54B_PBX_SS_H_ALERT,     /* IS-54B High PBX Short-short               */
	ATU_IS54B_PBX_SSL_H_ALERT,    /* IS-54B High PBX Short-short-long          */
	ATU_IS54B_PBX_SLS_H_ALERT,    /* IS-54B High PBX Short-long-short          */
	ATU_IS54B_PBX_SSSS_H_ALERT,   /* IS-54B High PBX Short-short-short-short   */
	ATU_IS53A_PPPP_H_ALERT,       /* IS-53A High Pip-Pip-Pip-Pip Alert         */
	ATU_IS54B_LONG_M_ALERT,       /* IS-54B Medium Long                        */
	ATU_IS54B_SS_M_ALERT,         /* IS-54B Medium Short-short                 */
	ATU_IS54B_SSL_M_ALERT,        /* 30 - IS-54B Medium Short-short-long       */
	ATU_IS54B_SS2_M_ALERT,        /* IS-54B Medium Short-short-2               */
	ATU_IS54B_SLS_M_ALERT,        /* IS-54B Medium Short-long-short            */
	ATU_IS54B_SSSS_M_ALERT,       /* IS-54B Medium Short-short-short-short     */
	ATU_IS54B_PBX_LONG_M_ALERT,   /* IS-54B Medium PBX Long                    */
	ATU_IS54B_PBX_SS_M_ALERT,     /* IS-54B Medium PBX Short-short             */
	ATU_IS54B_PBX_SSL_M_ALERT,    /* IS-54B Medium PBX Short-short-long        */
	ATU_IS54B_PBX_SLS_M_ALERT,    /* IS-54B Medium PBX Short-long-short        */
	ATU_IS54B_PBX_SSSS_M_ALERT,   /* IS-54B Medium PBX Short-short-short-short */
	ATU_IS53A_PPPP_M_ALERT,       /* IS-53A Medium Pip-Pip-Pip-Pip Alert       */
	ATU_IS54B_LONG_L_ALERT,       /* 40 - IS-54B Low Long                      */
	ATU_IS54B_SS_L_ALERT,         /* IS-54B Low Short-short                    */
	ATU_IS54B_SSL_L_ALERT,        /* IS-54B Low Short-short-long               */
	ATU_IS54B_SS2_L_ALERT,        /* IS-54B Low Short-short-2                  */
	ATU_IS54B_SLS_L_ALERT,        /* IS-54B Low Short-long-short               */
	ATU_IS54B_SSSS_L_ALERT,       /* IS-54B Low Short-short-short-short        */
	ATU_IS54B_PBX_LONG_L_ALERT,   /* IS-54B Low PBX Long                       */
	ATU_IS54B_PBX_SS_L_ALERT,     /* IS-54B Low PBX Short-short                */
	ATU_IS54B_PBX_SSL_L_ALERT,    /* IS-54B Low PBX Short-short-long           */
	ATU_IS54B_PBX_SLS_L_ALERT,    /* IS-54B Low PBX Short-long-short           */
	ATU_IS54B_PBX_SSSS_L_ALERT,   /* 50 - IS-54B Low PBX Short-short-short-shrt*/
	ATU_IS53A_PPPP_L_ALERT,       /* IS-53A Low Pip-Pip-Pip-Pip Alert          */
	ATU_FADE_TONE,                /* Tone to inform user of a fade             */
	ATU_SVC_CHANGE,               /* Inform user of a service area change      */
	ATU_HORN_ALERT,               /* Horn alert                                */
	ATU_ABRV_REORDER,             /* Abbreviated System busy                   */
	ATU_ABRV_INTERCEPT,           /* Abbrev'd Send request intercepted locally */
	ATU_ALTERNATE_REORDER,        /* Alternate reorder                         */
	ATU_MESSAGE_ALERT,            /* Message Waiting Signal                    */
	ATU_ABRV_ALERT,               /* Abbreviated alert                         */
	ATU_PIP_TONE,                 /* 60 - Pip Tone (Voice Mail Waiting)        */
	ATU_ROAM_RING,                /* Ringing option while roaming              */
	ATU_SVC_ACQ,                  /* Service acquired sound                    */
	ATU_SVC_LOST,                 /* Service lost sound                        */
	ATU_SVC_CHNG_MORE_PREF,       /* Change to a more preferred service sound  */
	ATU_SVC_CHNG_LESS_PREF,       /* Change to a less preferred service sound  */
	ATU_EXT_PWR_ON,               /* External power on sound                   */
	ATU_EXT_PWR_OFF,              /* External power off sound                  */
	ATU_RING_1,                   /* User selectable ring 1                    */
	ATU_RING_2,                   /* User selectable ring 2                    */
	ATU_RING_3,                   /* User selectable ring 3                    */
	ATU_RING_4,                   /* User selectable ring 4                    */
	ATU_RING_5,                   /* User selectable ring 5                    */
	ATU_RING_6,                   /* User selectable ring 6                    */
	ATU_RING_7,                   /* User selectable ring 7                    */
	ATU_RING_8,                   /* User selectable ring 8                    */
	ATU_RING_9,                   /* User selectable ring 9                    */
	ATU_VR_HFK_CALL_RECEIVED,     /* Call answer sound when in HFK             */
	ATU_HFK_CALL_ORIG,            /* Call origination sound when in HFK        */
	ATU_SPECIAL_INFO,             /* Special info sound                        */
	/* GSM tones, defined in 3GPP 2.40          */ 
	ATU_SUBSCRIBER_BUSY,          /* 80 - Subscriber busy sound                */ 
	ATU_CONGESTION,               /* Congestion sound                          */ 
	ATU_ERROR_INFO,               /* Error information sound                   */ 
	ATU_NUMBER_UNOBTAINABLE,      /* Number unobtainable sound                 */ 
	ATU_AUTH_FAILURE,             /* Authentication failure sound              */ 
	ATU_RADIO_PATH_ACK,           /* Radio path acknowledgement sound          */ 
	ATU_RADIO_PATH_NOT_AVAIL,     /* Radio path not available sound            */ 
	ATU_CEPT_CALL_WAITING,        /* CEPT call waiting sound                   */ 
	ATU_CEPT_RING,                /* CEPT ringing sound                        */ 
	ATU_CEPT_DIAL_TONE,           /* CEPT dial tone                            */ 
	ATU_MAX_COMPACT_ATU,          /* Use for range check of last compact sound */
	ATU_LAST_ATU                  /* Use for range checking last sound         */
};
