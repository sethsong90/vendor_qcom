/* Copyright (c) 2010 by Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef ACDBPARAMS_H
#define ACDBPARAMS_H


/* ABID_RX_DBM_OFFSET 0x0001005D */
/* IID_GAIN 0x0001004A */
uint16_t gain;

/* ABID_CODEC_RX_GAIN 0x0001005C */
/* IID_GAIN 0x0001004A */
uint16_t gain;

/* ABID_RX_AGC2 0x000100B4 */
/* IID_AGC2 0x000100B0 */
struct acdb_agc_block {
        uint16_t     enable_status;
        uint16_t     comp_rlink_static_gain;
        uint16_t     comp_rlink_aig_flag;
        uint16_t     exp_rlink_threshold;
        uint16_t     exp_rlink_slope;
        uint16_t     comp_rlink_threshold;
        uint16_t     comp_rlink_slope;
        uint16_t     comp_rlink_aig_attack_k;
        uint16_t     comp_rlink_aig_leak_down;
        uint16_t     comp_rlink_aig_leak_up;
        uint16_t     comp_rlink_aig_max;
        uint16_t     comp_rlink_aig_min;
        uint16_t     comp_rlink_aig_release_k;
        uint16_t     comp_rlink_aig_sm_leak_rate_fast;
        uint16_t     comp_rlink_aig_sm_leak_rate_slow;
        uint16_t     comp_rlink_attack_k_msw;
        uint16_t     comp_rlink_attack_k_lsw;
        uint16_t     comp_rlink_delay;
        uint16_t     comp_rlink_release_k_msw;
        uint16_t     comp_rlink_release_k_lsw;
        uint16_t     comp_rlink_rms_trav;
};

/* ABID_RX_VOICE_FIR 0x00010060 */
/* IID_VOICE_FIR_FILTER 0x0001004B */
struct acdb_voice_rx_fir {
	uint16_t 	tap0;
	uint16_t 	tap1;
	uint16_t 	tap2;
	uint16_t 	tap3;
	uint16_t 	tap4;
	uint16_t 	tap5;
	uint16_t 	tap6;
};

struct iir_coeff_type {
        uint16_t     b0_lo;
        uint16_t     b0_hi;
        uint16_t     b1_lo;
        uint16_t     b1_hi;
        uint16_t     b2_lo;
        uint16_t     b2_hi;
};

struct iir_coeff_stage_a {
        uint16_t     a1_lo;
        uint16_t     a1_hi;
        uint16_t     a2_lo;
        uint16_t     a2_hi;
};

/* ABID_RX_VOICE_IIR 0x00010061 */
/* IID_VOICE_IIR_FILTER 0x0001004C */
struct acdb_voice_iir_block {
        uint16_t 			enable_flag;
        uint16_t 			stage_count;
        struct iir_coeff_type 		stages_b[5];
        struct iir_coeff_stage_a 	stages_a[5];
        uint16_t 			shift_factor[5];
};

/* ABID_AUDIO_IIR_RX 0x0001006C */
/* IID_AUDIO_IIR_COEFF 0x00010073 */
struct acdb_iir_block {
        uint16_t                     	enable_flag;
        uint16_t                     	stage_count;
        struct iir_coeff_type		stages[4];
        struct iir_coeff_stage_a 	stages_a[4];
        uint16_t                     	shift_factor[4];
        uint16_t                     	pan[4];
};

/* ABID_AUDIO_MBADRC_RX 0x0001006E */
/* IID_MBADRC_BAND_CONFIG 0x00010076 */
struct mbadrc_band_config_type {
        uint16_t     mbadrc_sub_band_enable;
        uint16_t     mbadrc_sub_mute;
        uint16_t     mbadrc_comp_rms_tav;
        uint16_t     mbadrc_comp_threshold;
        uint16_t     mbadrc_comp_slop;
        uint16_t     mbadrc_comp_attack_msw;
        uint16_t     mbadrc_comp_attack_lsw;
        uint16_t     mbadrc_comp_release_msw;
        uint16_t     mbadrc_comp_release_lsw;
        uint16_t     mbadrc_make_up_gain;
};

/* ABID_AUDIO_MBADRC_RX 0x0001006E */
/* IID_MBADRC_PARAMETERS 0x00010077 */
struct mbadrc_parameter {
        uint16_t	mbadrc_enable;
        uint16_t        mbadrc_num_bands;
        uint16_t        mbadrc_down_sample_level;
        uint16_t        mbadrc_delay;
};


/* ABID_AUDIO_MBADRC_RX 0x0001006E */
/* IID_MBADRC_EXT_BUFF 0x00010075 */
int16_t ext_buff[196];


/* ABID_AUDIO_QAFX_RX 0x0001006F */
/* IID_QAFX_PARAMETERS 0x00010079 */
uint16_t qafx_param;

/* ABID_AUDIO_QCONCERT_PLUS_RX 0x0001009B */
/* IID_QCONCERT_PLUS_PARAMETERS 0x0001009C */
struct acdb_qconcert_plus_rx {
	uint16_t outputmode;
	uint16_t delay;
};

/* ABID_AUDIO_STF_RX 0x00010071 */
/* IID_AUDIO_IIR_COEFF 0x00010073 */
struct acdb_audio_stf_rx {
	uint16_t			enable_flag;
	uint16_t 			stage_count;
	struct iir_coeff_type   	stages[4];
        struct iir_coeff_stage_a 	stages_a[4];
	uint16_t 			shift_factor[4];
	uint16_t 			pan[4];
};

/* ABID_RVE_PARAM_RX 0x00010087 */
/* IID_RVE_PARAM 0x00010086 */
struct rve_param_rx {
	int16_t  enableflag;
	uint16_t alpha_noise;
	uint16_t alpha_ref;
	uint16_t alpha_rec;
	uint16_t alpha_gain_increase;
	uint16_t alpha_amp;
	uint16_t alpha_pl_attack;
	uint16_t alpha_pl_decay;
	uint16_t lowgain_limit;
	uint16_t headroom;
	uint16_t dshift;
	uint16_t hf_dshift;
	uint16_t scale_fact1;
	uint16_t scale_fact2;
	uint16_t peak_lim;
	uint16_t hardpeak_lim;
	uint16_t noisesens_thresh[4];
	uint16_t upperout_lim;
	uint16_t opmode;
	uint16_t aeqvad_threshold;
	uint16_t refvadhangover_max;
	uint16_t alpha_gain_decrease;
	uint16_t gain_multvector[4];
	uint16_t reserved[10];
};

/* ABID_SIDETONE_GAIN 0x00010095 */
/* IID_ST_GAIN_PARAMS 0x00010091 */
struct acdb_side_tone_gain {
	uint16_t enable;
	uint16_t gain;
};

/* ABID_TX_VOICE_GAIN 0x00010051 */
/* IID_GAIN 0x0001004A */
uint16_t gain;

/* ABID_TX_DTMF_GAIN 0x00010052 */
/* IID_GAIN 0x0001004A */
uint16_t gain;

/* ABID_CODEC_TX_GAIN 0x00010053 */
/* IID_GAIN 0x0001004A */
uint16_t  gain;

/* ABID_HSSD 0x00010054 */
/* IID_HSSD 0x0108B6B9 */
uint8_t enable;

/* ABID_TX_AGC2 0x000100B6 */
/* IID_AGC2 0x000100B0 */
struct acdb_agc_block_tx {
	uint16_t enable_flag;
	uint16_t agc_static_gain;
	uint16_t agc_aig;
	uint16_t agc_exp_thres;
	uint16_t agc_exp_slope;
	uint16_t agc_compr_thres;
	uint16_t agc_compr_slope;
	uint16_t agc_comp_aig_attackK;
	uint16_t agc_comp_aig_leakdown;
	uint16_t agc_comp_aig_leakup;
	uint16_t agc_comp_aig_max;
	uint16_t agc_comp_aig_min;
	uint16_t agc_comp_aig_releaseK;
	uint16_t agc_comp_aig_smleakratefast;
	uint16_t agc_comp_aig_smleakrateslow;
	uint16_t agc_comp_attackK;
	uint16_t agc_comp_delay;
	uint16_t agc_comp_releaseK;
	uint16_t agc_comp_rmstav;
};

/* ABID_TX_VOICE_FIR 0x00010056 */
/* IID_VOICE_FIR_FILTER 0x0001004B */
struct  acdb_voice_tx_fir {
	uint16_t	tap0;
	uint16_t 	tap1;
	uint16_t 	tap2;
	uint16_t 	tap3;
	uint16_t 	tap4;
	uint16_t 	tap5;
};

/* ABID_TX_VOICE_IIR 0x00010057 */
/* IID_VOICE_IIR_FILTER 0x0001004C */
struct acdb_voice_tx_iir {
	uint16_t 			enable_flag;
	uint16_t 			stage_count;
	struct iir_coeff_type   	stages[5];
	struct iir_coeff_stage_a 	stages_a[5];
	uint16_t 			shift_factor[5];
};

/* ABID_ECHO_CANCELLER 0x00010058 */
/* IID_ECHO_CANCELLER_VERSION 0x00010042 */
uint16_t ec_version;

/* ABID_ECHO_CANCELLER 0x00010058 */
/* IID_ECHO_CANCELLER_MODE 0x00010043 */
uint16_t ec_mode;

/* ABID_ECHO_CANCELLER 0x00010058 */
/* IID_ECHO_CANCELLER_NOISE_SUPPRESSOR_ENABLE 0x00010044 */
uint8_t enable;

/* ABID_ECHO_CANCELLER 0x00010058 */
/* IID_ECHO_CANCELLER_PARAMETERS 0x00010045 */
struct acdb_echo_canceller_params {
	uint16_t ecmode;
	uint16_t ec_switch;
	uint16_t reset_flag;
	uint16_t ec_startup_mute_hangover_thres;
	uint16_t ec_farend_hangover_thres;
	uint16_t esec_doubletalk_hangover_thres;
	uint16_t hsec_doubletalk_hangover_thres;
	uint16_t asec_doubletalk_hangover_thres;
	uint16_t ec_startup_mute_mode;
	uint16_t ec_mute_override;
	uint16_t ec_startup_erle_thres;
	uint16_t ec_force_half_duplex;
	uint16_t esec_reset_thres;
	uint16_t hec_reset_thres;
	uint16_t aec_reset_thres;
	uint16_t ec_window_shift;
};

/* ABID_ECHO_CANCELLER_NB_LVHF 0x00010059 */
/* IID_ECHO_CANCELLER_NEXTGEN_NB_PARAMETERS 0x00010046 */
struct acdb_echo_canceller_nb_lvhf {
	uint16_t nlpp_limit;
	uint16_t nlpp_gain;
	uint16_t af_limit;
	uint16_t mode;
	uint16_t tuning_mode;
	uint16_t echo_path_delay;
	uint16_t output_gain;
	uint16_t input_gain;
	uint16_t af_twoalpha;
	uint16_t af_erl;
	uint16_t af_taps;
	uint16_t af_preset_coefs;
	uint16_t af_offset;
	uint16_t af_erl_bg;
	uint16_t af_taps_bg;
	uint16_t pcd_threshold;
	uint16_t minimum_erl;
	uint16_t erl_step;
	uint16_t max_noise_floor;
	uint16_t det_threshold;
	uint16_t spdet_far;
	uint16_t spdet_mic;
	uint16_t spdet_xclip;
	uint16_t dens_tail_alpha;
	uint16_t dens_tail_portion;
	uint16_t dens_gamma_e_alpha;
	uint16_t dens_gamma_e_high;
	uint16_t dens_gamma_e_dt;
	uint16_t dens_gamma_e_low;
	uint16_t dens_gamma_e_rescue;
	uint16_t dens_spdet_near;
	uint16_t dens_spdet_act;
	uint16_t dens_gamma_n;
	uint16_t dens_NFE_blocksize;
	uint16_t dens_limit_ns;
	uint16_t dens_NL_atten;
	uint16_t dens_CNI_level;
	uint16_t wb_echo_ratio;
};

/* ABID_ECHO_CANCELLER_WB_LVHF 0x0001005A */
/* IID_ECHO_CANCELLER_NEXTGEN_WB_PARAMETERS 0x00010047 */
struct acdb_echo_canceller_wb_lvhf {
	uint16_t nlpp_limit;
	uint16_t nlpp_gain;
	uint16_t af_limit;
	uint16_t mode;
	uint16_t tuning_mode;
	uint16_t echo_path_delay;
	uint16_t output_gain;
	uint16_t input_gain;
	uint16_t af_twoalpha;
	uint16_t af_erl;
	uint16_t af_taps;
	uint16_t af_preset_coefs;
	uint16_t af_offset;
	uint16_t af_erl_bg;
	uint16_t af_taps_bg;
	uint16_t pcd_threshold;
	uint16_t minimum_erl;
	uint16_t erl_step;
	uint16_t max_noise_floor;
	uint16_t det_threshold;
	uint16_t spdet_far;
	uint16_t spdet_mic;
	uint16_t spdet_xclip;
	uint16_t dens_tail_alpha;
	uint16_t dens_tail_portion;
	uint16_t dens_gamma_e_alpha;
	uint16_t dens_gamma_e_high;
	uint16_t dens_gamma_e_dt;
	uint16_t dens_gamma_e_low;
	uint16_t dens_gamma_e_rescue;
	uint16_t dens_spdet_near;
	uint16_t dens_spdet_act;
	uint16_t dens_gamma_n;
	uint16_t dens_NFE_blocksize;
	uint16_t dens_limit_ns;
	uint16_t dens_NL_atten;
	uint16_t dens_CNI_level;
	uint16_t wb_echo_ratio;
};

/* ABID_FLUENCE2 0x00010099 */
/* IID_FLUENCE2_PARAMETERS 0x0001009A */
struct acdb_fluence {
	uint16_t mode;
	uint16_t tuning_mode_word;
	uint16_t echo_path_delay;
	uint16_t af1_twoalpha;
	uint16_t af1_erl;
	uint16_t af1_taps;
	uint16_t af1_preset_coefs;
	uint16_t af1_offset;
	uint16_t af2_twoalpha;
	uint16_t af2_erl;
	uint16_t af2_taps;
	uint16_t af2_preset_coefs;
	uint16_t af2_offset;
	uint16_t pcd_twoalpha;
	uint16_t pcd_offset;
	uint16_t pcd_Thres;
	uint16_t wgthreshold;
	uint16_t mpthreshold;
	uint16_t sf_init_table0[8];
	uint16_t sf_init_table1[8];
	uint16_t sf_taps;
	uint16_t sf_twoalpha;
	uint16_t dnns_echoalpharev;
	uint16_t dnns_Echoycomp;
	uint16_t dnns_WbThreshold;
	uint16_t dnns_EchoGammaHi;
	uint16_t dnns_EchoGammaLo;
	uint16_t dnns_NoiseGammaS;
	uint16_t dnns_NoiseGammaN;
	uint16_t dnns_NoiseGainMinS;
	uint16_t dnns_NoiseGainMinN;
	uint16_t dnns_NoiseBiasComp;
	uint16_t dnns_AcThreshold;
	uint16_t wb_echo_ratio;
	uint16_t wb_Gamma_E;
	uint16_t wb_Gamma_NN;
	uint16_t wb_Gamma_SN;
	uint16_t vcodec_Delay_1;
	uint16_t vcodec_Delay_2;
	uint16_t vcodec_Flen_1;
	uint16_t vcodec_Flen_2;
	uint16_t vcodec_Vad_Th;
	uint16_t vcodec_Pow_Th;
	uint16_t fixcalfactormic1;
	uint16_t fixcalfactormic2;
	uint16_t cs_outputgain;
	uint16_t vcodec_Meu_1;
	uint16_t vcodec_Meu_2;
	uint16_t fixed_over_est;
	uint16_t rx_nlpp_limit;
	uint16_t rx_nlpp_gain;
	uint16_t wnd_threshold;
	uint16_t wnd_ns_hover;
	uint16_t wnd_pwr_smalpha;
	uint16_t wnd_det_ESmAlpha;
	uint16_t wnd_ns_rOffset;
	uint16_t wnd_sm_ratio;
	uint16_t wnd_det_Coefs[5];
	uint16_t wnd_th1;
	uint16_t wnd_th2;
	uint16_t wnd_fq;
	uint16_t wnd_dfc;
	uint16_t wnd_sm_alphainc;
	uint16_t wnd_sm_alphadec;
	uint16_t crystalspeechcoeffspeech[30];
	uint16_t crystalspeechcoeffnoise[30];
	uint16_t cs_speaker[7];
	uint16_t ns_fac;
	uint16_t ns_blockSize;
	uint16_t is_bias;
	uint16_t is_bias_Inp;
	uint16_t sc_initb;
	uint16_t sc_resetb;
	uint16_t sc_avar;
	uint16_t is_hover[5];
	uint16_t is_cf_Level;
	uint16_t is_cf_InA;
	uint16_t is_cf_InB;
	uint16_t is_cf_a;
	uint16_t is_cf_b;
	uint16_t sc_th;
	uint16_t sc_pscale;
	uint16_t sc_nc;
	uint16_t sc_hover;
	uint16_t sc_alphaS;
	uint16_t sc_cfac;
	uint16_t sc_sdmax;
	uint16_t sc_sdmin;
	uint16_t sc_initl;
	uint16_t sc_maxval;
	uint16_t sc_spmin;
	uint16_t is_ec_th;
	uint16_t is_fx_dL;
	uint16_t coeffs_iva_filt_1[32];
	uint16_t coeffs_iva_filt_2[32];
};

/* ABID_AFE_VOL_CTRL 0x00010067 */
/* IID_AFE_VOLUME_CONTROL 0x00010049 */
struct acdb_afe_vol_ctrl {
	uint16_t tx_vol;
	uint16_t rx_vol;
	uint16_t tx_buf_offset;
};

/* ABID_AUDIO_IIR_TX 0x0001006B */
/* IID_AUDIO_IIR_COEFF 0x00010073 */
struct acdb_audio_tx_iir {
	uint16_t 			enable_flag;
	uint16_t 			stage_count;
        struct iir_coeff_type 		stages_b[4];
        struct iir_coeff_stage_a 	stages_a[4];
	uint16_t 			shift_factor[4];
	uint16_t 			pan[4];
};

/* ABID_AUDIO_AGC_TX 0x00010068 */
/* IID_AUDIO_AGC_PARAMETERS 0x0001007E */
struct acdb_audio_agc_tx {
	uint16_t enable_status;
	uint16_t comp_rlink_static_gain;
	uint16_t comp_rlink_aig_flag;
	uint16_t exp_rlink_threshold;
	uint16_t exp_rlink_slope;
	uint16_t comp_rlink_threshold;
	uint16_t comp_rlink_slope;
	uint16_t comp_rlinkaigattackK;
	uint16_t comp_rlinkaigleakdown;
	uint16_t comp_rlinkaigleakup;
	uint16_t comp_rlinkaigmax;
	uint16_t comp_rlinkaigmin;
	uint16_t comp_rlinkaigreleasek;
	uint16_t comp_rlinkaigsmleakratefast;
	uint16_t comp_rlinkaigsmleakrateslow;
	uint16_t comp_rlinkattackkmsw;
	uint16_t comp_rlinkattack_klsw;
	uint16_t comp_rlinkdelay;
	uint16_t comp_rlinkRelease_kmsw;
	uint16_t comp_rlinkrelease_klsw;
	uint16_t comp_rlinkrmstav;
};

/* ABID_AUDIO_NS_TX 0x00010069 */
/* IID_ns_PARAMETERS 0x00010072 */
struct acdb_audio_ns_tx {
	uint16_t ebable_status;
	uint16_t dens_gamma_n;
	uint16_t dens_NFE_blocksize;
	uint16_t dens_limit_ns;
	uint16_t dens_limit_ns_d;
	uint16_t wb_gamma_e;
	uint16_t wb_gamma_n;
};

/* ABID_WIDE_VOICE 0x00010090 */
/* IID_WIDE_VOICE_PARAM 0x0001008F */
struct acdb_wide_voice {
	int16_t rxWveEnableflag;
	int16_t reserved[29];
};

/* ABID_SLOPE_IIR 0x00010092 */
/* IID_SLOPE_IIR_CODEC_FORMAT_REG 0x00010093 */
uint16_t slope_iir_codec_format_reg;

/* ABID_AVC2 0x000100B5 */
/* IID_AVC2 0x000100B1 */
struct acdb_avc2 {
	uint16_t avc_enable_flag;
	uint16_t rlink_bne_ramp;
	uint16_t rlink_sensitivity_offset;
	uint16_t flink_headroom;
	uint16_t flink_smoothk;
	uint16_t flink_threshold_list[24];
	uint16_t flink_targetgain_list[12];
};

/* ABID_FENS_RX 0x00010096 */
/* IID_FEns_PARAM 0x00010097 */
struct acdb_fens_rx {
	uint16_t fns_enableflag;
	uint16_t fns_param_mode;
	uint16_t fns_param_inputgain;
	uint16_t fns_param_outputgain;
	uint16_t fns_param_target_ns;
	uint16_t fns_params_alpha;
	uint16_t fns_paramn_alpha;
	uint16_t fns_paramn_alphamax;
	uint16_t fns_parame_alpha;
	uint16_t fns_paramn_snrmax;
	uint16_t fns_param_sn_block;
	uint16_t fns_param_ni;
	uint16_t fns_param_np_scale;
	uint16_t fns_paramn_lamba;
	uint16_t fns_paramn_lambdaf;
	uint16_t fns_param_gsbias;
	uint16_t fns_param_gsmax;
	uint16_t fns_params_alpha_hb;
	uint16_t fns_paramn_alpha_maxhb;
	uint16_t fns_parame_alphahb;
	uint16_t fns_paramn_lambda0;
	uint16_t fns_param_gsfast;
	uint16_t fns_param_gsmed;
	uint16_t fns_param_gsslow;
	uint16_t fns_param_avadthresh;
	uint16_t fns_param_avadpscale;
	uint16_t fns_param_avadhangover;
	uint16_t fns_param_avadalphas;
	uint16_t fns_param_avadsdmax;
	uint16_t fns_param_avadsdmin;
	uint16_t fns_param_avalinitlength;
	uint16_t fns_param_avadmaxval;
	uint16_t fns_paramavadinitbound;
	uint16_t fns_param_avadresetbound;
	uint16_t fns_param_avadavar;
	uint16_t fnsparam_avadnc;
	uint16_t fns_param_avadsmin;
	uint16_t fns_reserved[10];
};

/* ABID_SLOW_TALK 0x000100AD */
/* IID_SLOW_TALK_PARAM 0x000100AE */
struct acdb_slow_talk {
	uint16_t ez_hear_EnableFlag;
	uint16_t ez_hear_TargetExpRatioIF;
	uint16_t ez_hear_VocoderNBorWBIF;
	uint16_t ez_hear_MaxLocalExpCompDiffIF;
	uint16_t ez_hear_minSilenceDurationIF;
	uint16_t ez_hear_processdelayIF;
	uint16_t ez_hear_avadthreshIF;
	uint16_t ez_hear_avadthresh2IF;
	uint16_t ez_hear_avadpwrScaleIF;
	uint16_t ez_hear_avadhangoverMaxIF;
	uint16_t ez_hear_avadalphaSNRIF;
	uint16_t ez_hear_avadsnrDiffMaxIF;
	uint16_t ez_hear_avadsnrDiffMinIF;
	uint16_t ez_hear_avadinitLengthIF;
	uint16_t ez_hear_avadavarScaleIF;
	uint16_t ez_hear_avadmaxValIF;
	uint16_t ez_hear_avadinitBoundIF;
	uint16_t ez_hear_avadresetBoundIF;
	uint16_t ez_hear_avadsubNcIF;
	uint16_t ez_hear_avadcfacIF;
	uint16_t ez_hear_avadspowMinIF;
	uint16_t ez_hear_cirbufsizeIF_lw;
	uint16_t ez_hear_cirbufsizeIF_hw;
	uint16_t ez_hear_minimusimilarityIF_lw;
	uint16_t ez_hear_minimusimilarityIF_hw;
	uint16_t ez_hear_min_pastsimilarityIF_lw;
	uint16_t ez_hear_min_pastsimilarityIF_hw;
	uint16_t ez_heari_min_force_similarityIF_lw;
	uint16_t ez_hear_min_forcesimilarityIF_hw;
	uint16_t ez_hear_reserved0;
	uint16_t ez_hear_reserved1;
	uint16_t ez_hear_reserved2;
	uint16_t ez_hear_reserved3;
	uint16_t ez_hear_reserved4;
	uint16_t ez_hear_reserved5;
	uint16_t ez_hear_reserved6;
	uint16_t ez_hear_reserved7;
	uint16_t ez_hear_reserved8;
	uint16_t ez_hear_reserved9;
	uint16_t ez_hear_reserved;
};

/* ABID_AUDIO_CALIBRATION_GAIN_RX  0x00011162 */
/* IID_AUDIO_CALIBRATION_GAIN_RX   0x00011163 */
struct  acdb_calib_gain_rx {
	uint16_t  audppcalgain;
	uint16_t  reserved;
};

/* ABID_VOICE_GAIN_RX 0x00011172 */
/* IID_VOICE_GAIN_RX 0x00011173 */
uint16_t voice_rxgain;

/* ABID_VOICE_GAIN_TX 0x00011175 */
/* IID_VOICE_GAIN_TX 0x00011176 */
uint16_t voice_txgain;


#endif /* ACDBPARAMS_H */
