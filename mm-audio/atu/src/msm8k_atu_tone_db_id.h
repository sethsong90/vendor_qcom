/*
 *
 * Copyright (C) 2009 Qualcomm Technologies, Inc.
 *
 */

#define ATU_FIRST_TDB_TONE 1000

enum atu_tdb_tones {
	ATU_FIRST_TONE = ATU_FIRST_TDB_TONE,/* Use for range checking 1st tone  */
	ATU_0,              /* DTMF for 0 key                                   */
	ATU_1,              /* DTMF for 1 key                                   */
	ATU_2,              /* DTMF for 2 key                                   */
	ATU_3,              /* DTMF for 3 key                                   */
	ATU_4,              /* DTMF for 4 key                                   */
	ATU_5,              /* DTMF for 5 key                                   */
	ATU_6,              /* DTMF for 6 key                                   */
	ATU_7,              /* DTMF for 7 key                                   */
	ATU_8,              /* DTMF for 8 key                                   */
	ATU_9,              /* DTMF for 9 key                                   */
	ATU_A,              /* DTMF for A key                                   */
	ATU_B,              /* DTMF for B key                                   */
	ATU_C,              /* DTMF for C key                                   */
	ATU_D,              /* DTMF for D key                                   */
	ATU_POUND,          /* DTMF for # key                                   */
	ATU_STAR,           /* DTMF for * key                                   */
	ATU_CTRL,           /* Tone for a control key                           */
	ATU_2ND,            /* Tone for secondary function on a key             */
	ATU_WARN,           /* Warning tone (e.g. overwriting user phone# slot) */
	ATU_ERR,            /* Tone to indicate an error                        */
	ATU_TIME,           /* Time marker tone                                 */
	ATU_RING_A,         /* 1st Ringer tone                                  */
	ATU_RING_B,         /* 2nd Ringer tone                                  */
	ATU_RING_C,         /* 3rd Ringer tone                                  */
	ATU_RING_D,         /* 4th Ringer tone                                  */
	ATU_RING_A4,        /*  440.0 Hz  -Piano Notes-                         */
	ATU_RING_AS4,       /*  466.1 Hz                                        */
	ATU_RING_B4,        /*  493.8 Hz                                        */
	ATU_RING_C4,        /*  523.2 Hz                                        */
	ATU_RING_CS4,       /*  554.3 Hz                                        */
	ATU_RING_D4,        /*  587.3 Hz                                        */
	ATU_RING_DS4,       /*  622.2 Hz                                        */
	ATU_RING_E4,        /*  659.2 Hz                                        */
	ATU_RING_F4,        /*  698.5 Hz                                        */
	ATU_RING_FS4,       /*  739.9 Hz                                        */
	ATU_RING_G4,        /*  784.0 Hz                                        */
	ATU_RING_GS4,       /*  830.6 Hz                                        */
	ATU_RING_A5,        /*  880.0 Hz                                        */
	ATU_RING_AS5,       /*  932.2 Hz                                        */
	ATU_RING_B5,        /*  987.7 Hz                                        */
	ATU_RING_C5,        /* 1046.5 Hz                                        */
	ATU_RING_CS5,       /* 1108.7 Hz                                        */
	ATU_RING_D5,        /* 1174.6 Hz                                        */
	ATU_RING_DS5,       /* 1244.3 Hz                                        */
	ATU_RING_E5,        /* 1318.5 Hz                                        */
	ATU_RING_F5,        /* 1397.0 Hz                                        */
	ATU_RING_FS5,       /* 1479.9 Hz                                        */
	ATU_RING_G5,        /* 1568.0 Hz                                        */
	ATU_RING_GS5,       /* 1661.2 Hz                                        */
	ATU_RING_A6,        /* 1760.0 Hz                                        */
	ATU_RING_AS6,       /* 1864.7 Hz                                        */
	ATU_RING_B6,        /* 1975.5 Hz                                        */
	ATU_RING_C6,        /* 2093.1 Hz                                        */
	ATU_RING_CS6,       /* 2217.4 Hz                                        */
	ATU_RING_D6,        /* 2349.3 Hz                                        */
	ATU_RING_DS6,       /* 2489.1 Hz                                        */
	ATU_RING_E6,        /* 2637.0 Hz                                        */
	ATU_RING_F6,        /* 2793.7 Hz                                        */
	ATU_RING_FS6,       /* 2959.9 Hz                                        */
	ATU_RING_G6,        /* 3135.9 Hz                                        */
	ATU_RING_GS6,       /* 3322.4 Hz                                        */
	ATU_RING_A7,        /* 3520.0 Hz                                        */
	ATU_RBACK,          /* Ring back (audible ring)                         */
	ATU_BUSY,           /* Busy tone                                        */
	ATU_INTERCEPT_A,    /* First tone of an intercept                       */
	ATU_INTERCEPT_B,    /* Second tone of an intercept                      */
	ATU_REORDER_TONE,   /* Reorder                                          */
	ATU_PWRUP,          /* Power-up tone                                    */
	ATU_OFF_HOOK_TONE,  /* Off-hook tone, IS-95 (CAI 7.7.5.5)               */
	ATU_CALL_WT_TONE,   /* Call-waiting tone                                */
	ATU_DIAL_TONE_TONE, /* Dial tone                                        */
	ATU_ANSWER_TONE,    /* Answer tone                                      */
	ATU_HIGH_PITCH_A,   /* 1st High pitch for IS-54B alerting               */
	ATU_HIGH_PITCH_B,   /* 2nd High pitch for IS-54B alerting               */
	ATU_MED_PITCH_A,    /* 1st Medium pitch for IS-54B alerting             */
	ATU_MED_PITCH_B,    /* 2nd Medium pitch for IS-54B alerting             */
	ATU_LOW_PITCH_A,    /* 1st Low pitch for IS-54B alerting                */
	ATU_LOW_PITCH_B,    /* 2nd Low pitch for IS-54B alerting                */
	ATU_TEST_ON,        /* Test tone on                                     */
	ATU_MSG_WAITING,    /* Message Waiting Tone                             */
	ATU_PIP_TONE_TONE,  /* Used for Pip-Pip-Pip-Pip (Vocoder) Tone          */
	ATU_SPC_DT_INDIA,   /* Used for India's Special Dial Tone               */
	ATU_SIGNAL_INDIA,   /* Used in Various India Signalling Tones           */
	ATU_DT_TONE_INDIA,  /* Used for India's Normal Dial Tone (and others)   */
	ATU_DT_TONE_BRAZIL, /* Used for Brazil's Dial Tone                      */
	 ATU_DT_DTACO_TONE,  /* Used for DTACO's single tone (350Hz, 350Hz)      */
	ATU_HFK_TONE1,      /* These two tones used for Voice Activation and    */
	ATU_HFK_TONE2,      /* Incoming Call Answer in phone VR-HFK             */
	ATU_LAST_TONE       /* Use for range checking last tone                 */
};
