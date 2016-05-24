/*
 *
 * Copyright (C) 2009 Qualcomm Technologies, Inc.
 *
 */

#include "msm8k_atui.h"
#include "msm8k_atu_tone_db_id.h"

#define DEF_DTMF(i,h,l)   (h), (l)

const struct atu_tdb_dtmf_type atu_tdb_dtmf[] = {
	{DEF_DTMF((unsigned short)ATU_0             , 1336  , 941  )}, 
	{DEF_DTMF((unsigned short)ATU_1             , 1209  , 697  )}, 
	{DEF_DTMF((unsigned short)ATU_2             , 1336  , 697  )}, 
	{DEF_DTMF((unsigned short)ATU_3             , 1477  , 697  )}, 
	{DEF_DTMF((unsigned short)ATU_4             , 1209  , 770  )}, 
	{DEF_DTMF((unsigned short)ATU_5             , 1336  , 770  )}, 
	{DEF_DTMF((unsigned short)ATU_6             , 1477  , 770  )}, 
	{DEF_DTMF((unsigned short)ATU_7             , 1209  , 852  )}, 
	{DEF_DTMF((unsigned short)ATU_8             , 1336  , 852  )}, 
	{DEF_DTMF((unsigned short)ATU_9             , 1477  , 852  )}, 
	{DEF_DTMF((unsigned short)ATU_A             , 1633  , 697  )}, 
	{DEF_DTMF((unsigned short)ATU_B             , 1633  , 770  )}, 
	{DEF_DTMF((unsigned short)ATU_C             , 1633  , 852  )}, 
	{DEF_DTMF((unsigned short)ATU_D             , 1633  , 941  )}, 
	{DEF_DTMF((unsigned short)ATU_POUND         , 1477  , 941  )}, 
	{DEF_DTMF((unsigned short)ATU_STAR          , 1209  , 941  )}, 
	{DEF_DTMF((unsigned short)ATU_CTRL          , 852   , 852  )}, 
	{DEF_DTMF((unsigned short)ATU_2ND           , 770   , 770  )},
	{DEF_DTMF((unsigned short)ATU_WARN          , 2000  , 2000 )}, 
	{DEF_DTMF((unsigned short)ATU_ERR           , 2000  , 2000 )}, 
	{DEF_DTMF((unsigned short)ATU_TIME          , 2000  , 2000 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_A        , 2600  , 2600 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_B        , 2900  , 2900 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_C        , 2750  , 2750 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_D        , 2750  , 2750 )},
	{DEF_DTMF((unsigned short)ATU_RING_A4       , 440   , 440  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_AS4      , 466   , 466  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_B4       , 494   , 494  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_C4       , 523   , 523  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_CS4      , 554   , 554  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_D4       , 587   , 587  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_DS4      , 622   , 622  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_E4       , 659   , 659  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_F4       , 698   , 698  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_FS4      , 740   , 740  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_G4       , 784   , 784  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_GS4      , 831   , 831  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_A5       , 880   , 880  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_AS5      , 932   , 932  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_B5       , 988   , 988  )}, 
	{DEF_DTMF((unsigned short)ATU_RING_C5       , 1047  , 1047 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_CS5      , 1109  , 1109 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_D5       , 1175  , 1175 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_DS5      , 1244  , 1244 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_E5       , 1318  , 1318 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_F5       , 1397  , 1397 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_FS5      , 1480  , 1480 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_G5       , 1568  , 1568 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_GS5      , 1661  , 1661 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_A6       , 1760  , 1760 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_AS6      , 1865  , 1865 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_B6       , 1975  , 1975 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_C6       , 2093  , 2093 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_CS6      , 2217  , 2217 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_D6       , 2349  , 2349 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_DS6      , 2489  , 2489 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_E6       , 2637  , 2637 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_F6       , 2794  , 2794 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_FS6      , 2960  , 2960 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_G6       , 3136  , 3136 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_GS6      , 3322  , 3322 )}, 
	{DEF_DTMF((unsigned short)ATU_RING_A7       , 3520  , 3520 )}, 
	{DEF_DTMF((unsigned short)ATU_RBACK         , 480   , 440  )}, 
	{DEF_DTMF((unsigned short)ATU_BUSY          , 620   , 480  )}, 
	{DEF_DTMF((unsigned short)ATU_INTERCEPT_A   , 440   , 440  )}, 
	{DEF_DTMF((unsigned short)ATU_INTERCEPT_B   , 620   , 620  )}, 
	{DEF_DTMF((unsigned short)ATU_REORDER_TONE  , 620   , 480  )}, 
	{DEF_DTMF((unsigned short)ATU_PWRUP         , 620   , 620  )}, 
	{DEF_DTMF((unsigned short)ATU_OFF_HOOK_TONE , 2600  , 1400 )}, 
	{DEF_DTMF((unsigned short)ATU_CALL_WT_TONE  , 440   , 440  )}, 
	{DEF_DTMF((unsigned short)ATU_DIAL_TONE_TONE, 440   , 350  )}, 
	{DEF_DTMF((unsigned short)ATU_ANSWER_TONE   , 1000  , 660  )}, 
	{DEF_DTMF((unsigned short)ATU_HIGH_PITCH_A  , 3700  , 3700 )}, 
	{DEF_DTMF((unsigned short)ATU_HIGH_PITCH_B  , 4000  , 4000 )}, 
	{DEF_DTMF((unsigned short)ATU_MED_PITCH_A   , 2600  , 2600 )}, 
	{DEF_DTMF((unsigned short)ATU_MED_PITCH_B   , 2900  , 2900 )}, 
	{DEF_DTMF((unsigned short)ATU_LOW_PITCH_A   , 1300  , 1300 )}, 
	{DEF_DTMF((unsigned short)ATU_LOW_PITCH_B   , 1450  , 1450 )}, 
	{DEF_DTMF((unsigned short)ATU_TEST_ON       , 1000  , 1000 )}, 
	{DEF_DTMF((unsigned short)ATU_MSG_WAITING   , 2000  , 2000 )},
	{DEF_DTMF((unsigned short)ATU_PIP_TONE_TONE , 480   , 480  )}, 
	{DEF_DTMF((unsigned short)ATU_SPC_DT_INDIA  , 400   , 300  )}, 
	{DEF_DTMF((unsigned short)ATU_SIGNAL_INDIA  , 400   , 400  )}, 
	{DEF_DTMF((unsigned short)ATU_DT_TONE_INDIA , 400   , 250  )},
	{DEF_DTMF((unsigned short)ATU_DT_TONE_BRAZIL, 425   , 425  )},
	{DEF_DTMF((unsigned short)ATU_DT_DTACO_TONE , 350   , 350  )},
	{DEF_DTMF((unsigned short)ATU_HFK_TONE1     , 1350  , 675  )},
	{DEF_DTMF((unsigned short)ATU_HFK_TONE2     , 1800  , 900  )}
};

unsigned short atu_tdb_get_tone_freq(unsigned short tone,
		struct atu_tdb_dtmf_type *dtmf)
{
	if((tone > ATU_FIRST_TONE) && (tone < ATU_LAST_TONE)) {
		*dtmf = atu_tdb_dtmf[tone - ATU_FIRST_TONE - 1];
		return (ATUI_SUCCESS);
	} else {
		return (ATUI_FAILURE);
	}
}
