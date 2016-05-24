/*============================================================================
  @file voem_if_test.c
  This module contains the implementation of voem native C layer

  Copyright (c) 2010-2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/
#include "comdef.h"

#include <utils/Log.h>
#include "common_log.h"
#include "voem_if.h"
#include "voem_if_rpc.h"

#define LOG_TAG "voem_if"
struct voem_if_config {
        int    wve_enable;
        int    agc_enable;
        int    avc_enable;
        int    rve_enable;
        int    ec_enable;
        int    pcmfilter_enable;
	int    wnr_enable;
	int    fens_enable;
	int    st_enable;
	int    drx_enable;
	int    dtx_enable;
	int    pbe_enable;
};
static struct voem_if_config voemConfig;

void oncrpc_start(void)
{
	memset(&voemConfig, 0, sizeof(struct voem_if_config));
	/* init oncrpc and start oncrpc task */
	oncrpc_init();
	oncrpc_task_start();
	voem_ifcb_app_init();
}

int command( int feature, int enable)
{
		switch(feature) {
			case 1:

				if (enable)
					voemConfig.wve_enable = VOEM_ENABLE;
				else
					voemConfig.wve_enable = VOEM_DISABLE;
				/* enable/disable WVE */
				LOGV(" configure WVE, value=%d sent to modem\n",voemConfig.wve_enable);
				voem_pp_control(VOEM_PPBLOCK_WVE, voemConfig.wve_enable);
				break;
			case 2:

				if (enable)
					voemConfig.rve_enable = VOEM_ENABLE;
				else
                                       voemConfig.rve_enable = VOEM_DISABLE;
                                /* enable/disable RVE */
				LOGV(" configure RVE, value=%d sent to modem\n",voemConfig.rve_enable);
                                voem_pp_control(VOEM_PPBLOCK_RVE, voemConfig.rve_enable);
				break;
			case 3:

				if (enable)
					voemConfig.agc_enable = VOEM_ENABLE;
				else
                                        voemConfig.agc_enable = VOEM_DISABLE;
                                /* enable/disable AGC */
                                LOGV(" configure AGC, value=%d sent to modem\n",voemConfig.agc_enable);
                                voem_pp_control(VOEM_PPBLOCK_AGC, voemConfig.agc_enable);
				break;
			case 4:

				if (enable)
					voemConfig.avc_enable = VOEM_ENABLE;
				else
                                        voemConfig.avc_enable = VOEM_DISABLE;
                                /* enable/disable AVC */
                                LOGV(" configure AVC, value=%d sent to modem\n",voemConfig.avc_enable);
                                voem_pp_control(VOEM_PPBLOCK_AVC, voemConfig.avc_enable);
				break;
			case 5:

				if (enable)
					voemConfig.ec_enable = VOEM_ENABLE;
				else
                                        voemConfig.ec_enable = VOEM_DISABLE;
                                /* enable/disable EC */
				LOGV(" configure EC, value=%d sent to modem\n",voemConfig.ec_enable);
                                voem_pp_control(VOEM_PPBLOCK_EC, voemConfig.ec_enable);
				break;
			case 6:
				if (enable)
					voemConfig.pcmfilter_enable = VOEM_ENABLE;
				else
                                        voemConfig.pcmfilter_enable = VOEM_DISABLE;
                                /* enable/disable PCMFILTER */
			      	LOGV(" configure PCMFILTER, value=%d sent to modem\n",voemConfig.pcmfilter_enable);
                                voem_pp_control(VOEM_PPBLOCK_PCMFILTER, voemConfig.pcmfilter_enable);
				break;
			case 7:

                                if (enable)
                                        voemConfig.wnr_enable = VOEM_ENABLE;
                                else
                                        voemConfig.wnr_enable = VOEM_DISABLE;
                                /* enable/disable WNR */
				LOGV(" configure WNR, value=%d sent to modem\n",voemConfig.wnr_enable);
                                voem_pp_control(VOEM_PPBLOCK_WNR, voemConfig.wnr_enable);
                                break;
	                case 8:

                                if (enable)
                                        voemConfig.fens_enable = VOEM_ENABLE;
                                else
                                        voemConfig.fens_enable = VOEM_DISABLE;
                                /* enable/disable FENS */
				LOGV(" configure FENS, value=%d sent to modem\n",voemConfig.fens_enable);
                                voem_pp_control(VOEM_PPBLOCK_FENS, voemConfig.fens_enable);
                                break;
  		        case 9:
				#ifdef AUDIO_7x27
					return 0;
				#endif

                                if (enable)
                                        voemConfig.st_enable = VOEM_ENABLE;
                                else
                                        voemConfig.st_enable = VOEM_DISABLE;
                                /* enable/disable FENS */
                                LOGV(" configure ST, value=%d sent to modem\n",voemConfig.st_enable);
                                voem_pp_control(VOEM_PPBLOCK_SLOWTALK, voemConfig.st_enable);
                                break;
		        case 10:

                                if (enable)
                                        voemConfig.drx_enable = VOEM_ENABLE;
                                else
                                        voemConfig.drx_enable = VOEM_DISABLE;
                                /* enable/disable FENS */
                                LOGV(" configure DRX, value=%d sent to modem\n",voemConfig.drx_enable);
                                voem_pp_control(VOEM_PPBLOCK_DTMF_DETECT_RX, voemConfig.drx_enable);
                                break;
		        case 11:

                                if (enable)
                                        voemConfig.dtx_enable = VOEM_ENABLE;
                                else
                                        voemConfig.dtx_enable = VOEM_DISABLE;
                                /* enable/disable FENS */
                                LOGV(" configure DTX, value=%d sent to modem\n",voemConfig.dtx_enable);
                                voem_pp_control(VOEM_PPBLOCK_DTMF_DETECT_TX, voemConfig.dtx_enable);
                                break;
		        case 12:
				#ifdef AUDIO_7x27
					return 0;
				#endif
                                if (enable)
                                        voemConfig.pbe_enable = VOEM_ENABLE;
                                else
                                        voemConfig.pbe_enable = VOEM_DISABLE;
                                /* enable/disable FENS */
                                LOGV(" configure PBE, value=%d sent to modem\n",voemConfig.pbe_enable);
                                voem_pp_control(VOEM_PPBLOCK_PBE, voemConfig.pbe_enable);
                                break;

		        default:
                                return 0;

                }//switch
return 1;
}

 void oncrpc_stop(void)
{
	oncrpc_task_stop();

}

