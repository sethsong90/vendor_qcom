/* Copyright (c) 2009-2010 by Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

			A U D I O   E Q U A L I Z E R

GENERAL DESCRIPTION
  These file contains calculation for audio equalizer

EXTERNALIZED FUNCTIONS
  audioeq_calccoefs

INITIALIZATION AND SEQUENCING REQUIREMENTS

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/* <EJECT> */
/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to this file.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/multimedia/audio/pp/audioeq/main/latest/src/audioeq.c#1 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/15/10    jw     Update to AUDIO_SYSTEM_POSTPROC_CPP_ECI_EQ_DESIGN_UNIT_TEST
			_RELEASE_1.0.1_10.01.10
04/02/06    aw     Fixed compiling warnings.
12/13/04    aw     Fixed compiling warnings.
06/12/04    aw     Initial version.
===========================================================================*/

/*===========================================================================

                       INCLUDE FILES FOR MODULE


===========================================================================*/
#include <stdio.h>
#include <stdint.h>

/* <EJECT> */
/*===========================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

===========================================================================*/
#define ONEQ28			(1<<28)		/* The value of 1 in Q28   */
#define ONEQ24			(1<<24)		/* The value of 1 in Q24   */
#define SQRT2Q30		(0x5A827998)	/* Square root of 2 in Q30 */
#define SQRT2Q28		(379625062)	/* Square root of 2 in Q28 */
#define PIQ29			(0x3243F6A8*2)	/* The value of PI in Q29  */
#define GAIN_INDEX_OFFSET	(0)		/* The Gain table offset   */
#define K_MAX			388276097
#define ANGLES_LENGTH		20
#define UNIT_DEFINED_QFACTOR	1073741824

/* Linear gains. Q28. To convert logarithmic gains to linear gains */
int V0LinearTable[]={
	268435456, /* 0 */
	301189535,
	337940217,
	379175160, /* 3 */
	425441527,
	477353244,
	535599149, /* 6 */
	600952130,
	674279380,
	756553907, /* 9 */
	848867446,
	952444939,
	1068660799, /* 12 */
	1199057137,
	1345364236,
	1509523501 /* 15 */
};

/*===========================================================================

FUNCTION voceq_calccoefs

DESCRIPTION
  This function calculate the coefs and the shift compensation for the 
  numerator coefs
  type: 1 -> bass
        2 -> treble
        3 -> band

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void audioeq_calccoefs(
	int32_t  V0,
	int32_t  fc,
	int32_t  fs,
	uint16_t type,
	int32_t  Q,
	int32_t  *bassNum,
	int32_t  *bassDen,
	uint16_t *shiftQ)
{
	int shift;
	short i;
	int Den[3];
	int *Num = (int *)bassNum;

	*shiftQ=2;	// default shift compensation = 2
	/* select the correct equalizer required*/

	if (V0 < 0) {
		type += 3;
		V0 = -V0;
	}
	switch(type)
	{
	case 1: /* bassboost*/
		*shiftQ=(short)calculateBassBoost(V0, fc, fs, Num, Den);
		break;
	case 2:  /* trebleboost*/
		calculateTrebleboost(V0, fc, fs, Num, Den,&shift);
		*shiftQ = (short)(32- shift); //Q factor compensation shift
		break;
	case 3:	 /* bandboost*/
		*shiftQ=(short)calculateBandBoost(V0, Q, fc, fs, Num, Den);
		break;
	case 4:	 /* bass cut*/
		calculateBassCut(V0, fc, fs, Num, Den);
		break;
	case 5:
		calculateTrebleCut(V0, fc, fs, Num, Den);
		break;
	case 6:
		calculatebandCut(V0, fc, fs, Q, Num, Den);
		break;
	default:
		return;
	} /* end of switch*/

	for(i=0;i<2;i++)
	{
		bassDen[i]   = Den[i+1];
	}
} // end of wrapper function to calculate coefs.


/*===========================================================================

FUNCTION calculateBassBoost

DESCRIPTION
  This function is used to calculate the coefs for bassboost case.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateBassBoost(int V0, int fc, int fs, int *bassNum, int *bassDen)
{
	int mySR=1, DEN, K, index;

	/* calculate V0, V0 in Q28*/
	index = (int)V0+GAIN_INDEX_OFFSET;
	if(index > 15)
		index = 15;

	V0=V0LinearTable[index];
	/* return DEN = Q24, K = Q28 */
	DEN=calculateBoostDEN(fc, fs, &K);
	/* calculate the numerator coeffs, mySR is the numerators Qfactor*/
	mySR = calculateNumeratorBassBoost(V0, K, bassNum, DEN);
	/* calculate the denominator coeffs, Qfactor is fixed to 30*/
	calculateDenominatorBassBoost(K, bassDen, DEN);
	mySR = 32-mySR;	 // Q factors for the numerator.
	return mySR;
}

long mul32x32Shift2(int x, int y, unsigned short shift);
/*===========================================================================

FUNCTION calculateBoostDEN

DESCRIPTION
  This function calculates K and DEN for given filter uses newton's devide 
  method.
  Wc    = 2*pi*(0.1/32); % cut off freq =  2*pi*fc/fs
  K     = Wc/2
  V0    = 10^(6/20);
  DEN   = (1+(2^0.5)*K+K^2);
  B1(1) = (1+(2*V0)^0.5*K+V0*(K^2))/DEN;
  B1(2) = (2*(V0*(K^2)-1))/DEN;
  B1(3) = (1-((2*V0)^0.5)*K+V0*(K^2))/DEN;
  A1(1) = 1;
  A1(2) = (2*(K^2-1))/DEN;
  A1(3) = ( 1- (2^0.5)*K + (K^2) )/DEN;

DEPENDENCIES
  None

RETURN VALUE
  DEN = (1+(2^0.5)*K+K^2); q24

SIDE EFFECTS
  None

===========================================================================*/
int calculateBoostDEN(
	int cutoffQ0,                   /* cutoff or center freq.                */
	int SamplingFreqQ0,             /* sampling freq.                        */
	int* KQ28                       /* pi*(fc/fs), q28                       */
)
{
	int SR, K, x, y;
	int64_t temp64;

	/***** Calculate K = tan(fc/Fs*pi) *****/
	// Compute fc/Fs with qfactor = SR
	dsplib_approx_divide(cutoffQ0, SamplingFreqQ0, &K, &SR);
	// fc/Fs < 1 always, hence represent it in Q31
	if (SR > 31) {
		K = K >> (SR-31);
	} else {
		K = K << (31-SR);
	}

	// K = Q31 + Q29 - Q32 + Q0 = Q28
	K = mul32x32Shift2(K,PIQ29,0);

	// if using tan(20/44.1*pi), max K = 6.8 (in Q28)
	if(K >= K_MAX)
		K = K_MAX;
	// Compute K = tan(K) = tan(fc/Fs*pi), output is still Q28
	K = tangent(K, 20);

	/**** Calculate DEN = (1+(2^0.5)*K+K^2); ****/
	// sqrt2Q30 = 2^0.5 in Q30
	// Compute x = (2^0.5)*K in Q26
	// Q28 + Q30 - Q32 + Q0 = Q26
	x = mul32x32Shift2(K,SQRT2Q30,0);

	// Compute y = K*K in Q24
	// Q28 + Q28 - Q32 + Q0 = Q24
	y = mul32x32Shift2(K,K,0);

	// (2^0.5)*K is in Q26, needs 2-bit right shift before summation
	temp64 = (int64_t)ONEQ24 + ((int64_t)x>>2) + (int64_t)y;
	// DEN = (1+(2^0.5)*K+K^2) in Q24
	x = saturate_s32(temp64);
	*KQ28 = K;
	return x;
}

/*===========================================================================

FUNCTION calculateNumeratorBassBoost

DESCRIPTION
  This function calculates Numerator coeff. for BassBoost

DEPENDENCIES
  None

RETURN VALUE
  Smax -- Qfactor of the numerators Smax -- Qfactor of the numerators

SIDE EFFECTS
  None

===========================================================================*/
int calculateNumeratorBassBoost(
  int V0,                                /* Gain value                     */
  int K,                                 /* Shift value                    */
  int *numerator,                        /* output: numerator[], q30       */
  int DEN                                /* Den value                      */
)
{
	int V0double, V0sqrt, V02sqrtQ28, KSqrQ24, temp1, temp2, normb;
	int B1Q24, B2Q24, B3Q24, SR;
	int B1Q30, B2Q30, B3Q30;
	int SR1,SR2,SR3,Smax;
	int64_t temp64;

	/* calculate KsqrQ24 = Q24  */
	KSqrQ24 = mul32x32Shift2(K, K, 0);

	/**** Calculate sqrt(V0*2) in Q28 ****/
	// v0DOUBLE is in Q28/
	V0double = V0;
	// Compute V0sqrt = Sqrt(V0) in Q14 */
	V0sqrt=sqrtfp(V0double);
	// Shift V0sqrt to be Q28
	V0sqrt = V0sqrt << 14;

	// Compute sqrt(2*V0) = sqrt(2)*sqrt(v0) in Q28
	// sqrt(V0): Q28 + Q30 - Q32 + Q2 = Q28
	V02sqrtQ28 = mul32x32Shift2(V0sqrt,SQRT2Q30,2);

	/* Calculate B(0) = 1+(2*V0)^0.5*K+V0*(K^2); */
	// temp1 = K*sqrt(2*V0): Q28 + Q28 - Q32 + Q0 = Q24
	temp1 = mul32x32Shift2(K,V02sqrtQ28,0);

	// For large V0 and K, V0*K^2 might need up to 9 integer bits
	// temp2 = V0*(K^2): Q28 + Q24 - Q32 + Q2 = Q22
	temp2 = mul32x32Shift2(V0,KSqrQ24,2);

	// Compute 64-bit sum: 1+(2*V0)^0.5*K+V0*(K^2);  in Q24
	temp64 = (int64_t)ONEQ24+ (int64_t)temp1 + ((int64_t)temp2<<2);
	// extra function now.
	qdsp_norm64(temp64,&normb,&SR1);

	// B1Q24's Qfactor is 24+SR1
	B1Q24 = normb; //Q fctr = sr1+24;

	/* Calculate B1 = 2*(V0*K^2 - 1); now */
	// Still in Q24
	temp64 = (((int64_t)temp2<<2) - (int64_t)ONEQ24)<<1;
	qdsp_norm64(temp64,&normb,&SR2);
	B2Q24 = normb;

	/* Calculate B2 = 1-sqrt(2*V0)*K +V0*K^2; */
	temp64 = (int64_t)ONEQ24 - (int64_t)temp1 + ((int64_t)temp2<<2);
	qdsp_norm64(temp64,&normb,&SR3);
	B3Q24 = (int)normb;		// no overflows.

	// now divide by DEN
	dsplib_approx_divide(B1Q24, DEN ,&B1Q30, &SR);
	SR1 = SR+SR1;
	dsplib_approx_divide(B2Q24, DEN ,&B2Q30, &SR);
	SR2 = SR+SR2;
	dsplib_approx_divide(B3Q24, DEN ,&B3Q30, &SR);
	SR3 = SR+SR3;

	// find min of the SRs, the Q-factor for numerator fixed point representation
	Smax = (SR1>SR2)?SR2:SR1;
	Smax= (Smax>SR3)?SR3:Smax;

	numerator[0] = B1Q30>>(SR1-Smax);
	numerator[1] = B2Q30>>(SR2-Smax);
	numerator[2] = B3Q30>>(SR3-Smax);
	return Smax;
}

/*===========================================================================

FUNCTION calculateDenominatorBassBoost

DESCRIPTION
  This function calculates denominator coeff. for BassBoost

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateDenominatorBassBoost(
  int K,                          /* inputs:  K q28,                       */
  int *denomenator,               /* Output: denominator[], q30            */
  int DEN                         /* inputs:  DEN q28                      */
)
{
	int KSqrQ24, temp1;
	int A1Q24, A2Q24, SR;
	int A0Q30, A1Q30, A2Q30;
	int64_t temp64;

	/* Calculate A(1)=(2*(K^2-1));  in Q24 */
	// K is in Q28
	// KSqrQ24=K*K: Q28 + Q28 - Q32 + Q0 = Q24
	KSqrQ24 = mul32x32Shift2(K, K, 0);
	temp64 = 2*((int64_t)KSqrQ24 - (int64_t)ONEQ24);
	A1Q24 = saturate_s32(temp64);

	/* Calculate A(2)=( 1- (2^0.5)*K + (K^2)); in Q24 */
	// temp1 = (2^0.5)*K: Q28 + Q28 -Q32 + Q0 = Q24
	temp1 = mul32x32Shift2(K, SQRT2Q28, 0);
	temp64 = (int64_t)ONEQ24 - (int64_t)temp1 + (int64_t)KSqrQ24;
	A2Q24 = saturate_s32(temp64);

	/* divide by DEN */
	// Assume all denominators are < 2, fixed Qfactor to 30
	A0Q30 = 1 << 30;

	// Compute A1Q30
	dsplib_approx_divide(A1Q24, DEN ,&A1Q30, &SR);
	if (SR>30)
		A1Q30 = A1Q30 >> (SR-30);
	else
		A1Q30 = A1Q30 << (30-SR);

	// Compute A2Q30
	dsplib_approx_divide(A2Q24, DEN ,&A2Q30, &SR);
	if (SR>30)
		A2Q30 = A2Q30 >> (SR-30);
	else
		A2Q30 = A2Q30 << (30-SR);

	denomenator[0]=A0Q30;
	denomenator[1]=A1Q30;
	denomenator[2]=A2Q30;
	return 1;
}

/*===========================================================================

FUNCTION calculateBandBoost

DESCRIPTION
  This function calculate the coefficients of Bandboost
  K    = Wc/2. Wc=28pi*fc/fs
  DEN  = 1+k/Q+k^2
  V0   = 10^(gain/20)
  B(0) = (1+V0*K/Q+K^2)/DEN
  B(1) = 2*(K^2-1)/DEN
  B(2) = (1-V0K/Q +K^2)/DEN
  A(1) = 2*(K^2-1)/DEN
  A(2) = (1-K/Q+k^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateBandBoost(int V0, int que, int fc, int fs, int *bassNum, int *bassDen)
{
	int mySR, DEN, K,KbyQ, index;
	/* Calcualte K*/
	/* Themax value of K is pi/2 = 1.5708*/
	/* hence the Q factor can be Q2.30 (unsigned)*/

	/* calculate V0, V0 in Q28*/
	index = (int)V0+GAIN_INDEX_OFFSET;
	if(index > 15)
		index = 15;

	V0=V0LinearTable[index];

	/* calculate DEN, returns K,KbyQ & DEN in Q24.*/
	DEN=calculateBandBoostDEN(fc, fs, que,&K, &KbyQ);

	/* calculate the numerator coeffs, mySR is the numerators Qfactor*/
	mySR = calculateNumBandBoost(V0, K, bassNum, DEN,KbyQ);
	/* calculate the denominator coeffs, Qfactor fixed to 30 */
	calculateDenBandBoost(K, bassDen, DEN,KbyQ);
	return (32-mySR);
}

/*===========================================================================

FUNCTION calculateBandBoostDEN

DESCRIPTION
  This function calculates K and DEN for given filter uses dsp_approx_divide

DEPENDENCIES
  None

RETURN VALUE
  DEN = 1+k/Q+k^2; q28

SIDE EFFECTS
  None

===========================================================================*/
int calculateBandBoostDEN
(
  int cutoffQ0,                    /* Input: fc: cutoff or center freq.    */
  int SamplingFreqQ0,              /* Input: fs: sampling freq.            */
  int queQ0,
  int *KQ28,                       /* Output: Kq28=pi*(fc/fs), q28.        */
  int *KbyQ27                      /* Output: DEN = 1+k/Q+k^2; q27         */
)
{
	int SR, K, x, y;
	int invque,queSR;
	int64_t temp64;

	/*calculate 1/Q. output: invque with qfactor=SR*/
	if (queQ0 < 256)
		queQ0 = 256;
	dsplib_approx_divide(1, queQ0, &invque, &queSR);
	//queQ0 = Q*2^8: Q is in Q8 when passed in
	queSR = queSR -8;
	// Assume Q > 1, hence convert Q to Q31
	if (queSR > 31) {
		invque = invque >> (queSR-31);
	} else {
		invque = invque << (31-queSR);
	}

	/***** Calculate K = tan(fc/Fs*pi) *****/

	/* Compute fc/Fs with qfactor = SR */
	dsplib_approx_divide(cutoffQ0, SamplingFreqQ0, &K, &SR);
	// fc/Fs < 1 always, hence represent it in Q31
	if (SR > 31) {
		K = K >> (SR-31);
	} else {
		K = K << (31-SR);
	}

	/* Compute K = K*pi = fc/Fs*pi */
	// K = Q31 + Q29 - Q32 + Q0 = Q28
	K = mul32x32Shift2(K,PIQ29,0);

	// if using tan(20/44.1*pi), max K = 6.8 (in Q28)
	if(K >= K_MAX)
		K = K_MAX;
	// Compute K = tan(K) = tan(fc/Fs*pi), output is still Q28
	K = tangent(K, 20);
	*KQ28 = K;

	/**** Calculate DEN = DEN = 1+k/Q+k^2; *****/

	/* x= K *invQ, */
	// Q28 + Q31 - Q32 + Q0 = Q27
	x = mul32x32Shift2(K,invque,0);
	*KbyQ27 = x;
	//Convert K/Q to Q24
	x = x>>3;

	/* y = K*K, q24 */
	// Q28 + Q28 - Q32 + Q0 = Q24
	y = mul32x32Shift2(K,K,0);

	/* x = 1 + K/Q + K^2; */
	// output is in Q24
	temp64 = ONEQ24+x+y;
	x = saturate_s32(temp64);

	return x;
} /* End of function to calculate DEN for bandboost*/


/*===========================================================================

FUNCTION calculateNumBandBoost

DESCRIPTION
  This function calculates Numerator coeff. for BandBoost
  B(0) = (1+V0*K/Q+K^2)/DEN
  B(1) = 2*(K^2-1)/DEN
  B(2) = (1-V0K/Q +K^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateNumBandBoost(
  int V0,                  /* input: V0 q28,                               */
  int K,                   /* input: K q28,                                */
  int *numerator,          /* output: numerator[], q30                     */
  int DEN,                 /* input: DEN, Q24                              */
  int KbyQ                 /* input: KbyQ, q27                             */
)
{
	int V0KbyQ,  KSqrQ24;
	int normb;
	int B0Q24,B1Q24, B2Q24, SR,SR0,SR1,SR2,Smax;
	int B0Q30, B1Q30, B2Q30;
	int64_t b0,b1,b2;

	/**** calculate V0*K/Q ****/

	/* Calculate V0KbyQ = V0*K/Q, q26 */
	// V0 = 10^(G/20) in Q28 format, using table look-up to find its value
	// KbyQ = K/Q in Q27 format, got from DEN computation
	// V0KbyQ = K/Q: Q28 + Q27 - Q32 + Q1 = Q24
	V0KbyQ = mul32x32Shift2(V0,KbyQ,1);

	/* calculate KsqrQ24 = K*K in q24  */
	// We need Q25 at least if max(K) = tan(20/44.1*pi)
	// KSqrQ24 = K*K: Q28 + Q28 - Q32 + Q0 = Q24
	KSqrQ24 = mul32x32Shift2(K, K, 0);

 	/* Calculate b(0) = (1+V0*K/Q+(K^2)) */
	// b0 in Q24
	b0 = (int64_t)ONEQ24 + (int64_t)V0KbyQ + (int64_t)KSqrQ24;
	qdsp_norm64(b0,&normb,&SR0);
	// although it's called B0Q24, it is not in Q24 now, but Q(24+SR1)
	B0Q24 = normb;

	/* Calculate b(1) = 2*(K^2-1) */
	// b1 in Q24, for if Kmax = tan(20/44.1*pi), (K^2-1)*2 needs Q24
	b1 = (KSqrQ24-ONEQ24)<<1;
	qdsp_norm64(b1,&normb,&SR1);
	// although it's called B1Q24, it is not in Q24 now, but Q(24+SR2)
	B1Q24 = normb;

	/* Calculate b(2)= (1-V0K/Q +K^2)*/
	// b2 in Q24, 64-bit,
	b2 = (int64_t)ONEQ24 - (int64_t)V0KbyQ + (int64_t)KSqrQ24;
	qdsp_norm64(b2,&normb,&SR2);
	// although it's called B2Q24, it is not in Q28 now, but Q(24+SR3)
	B2Q24 = normb;

	/* b0,b1,b2 divided by DEN */
	//B0Q30 is in Q(SR1+SR) format at first, then converted to Q30
	dsplib_approx_divide(B0Q24, DEN ,&B0Q30, &SR);
	SR0=SR+SR0;

	//B1Q30 is in Q(SR2+SR) format at first, then converted to Q30
	dsplib_approx_divide(B1Q24, DEN ,&B1Q30, &SR);
	SR1=SR+SR1;

	//B2Q30 is in Q(SR3+SR) format at first, then converted to Q30
	dsplib_approx_divide(B2Q24, DEN ,&B2Q30, &SR);
	SR2=SR2+SR;

	Smax = (SR0>SR1)?SR1:SR0;
	Smax= (Smax>SR2)?SR2:Smax;

	numerator[0] = B0Q30>>(SR0-Smax);
	numerator[1] = B1Q30>>(SR1-Smax);
	numerator[2] = B2Q30>>(SR2-Smax);

	return Smax;
} /* end of band boost num calculations.*/

/*===========================================================================

FUNCTION calculateDenBandBoost

DESCRIPTION
  This function calculates denominator coeff. for BassBoost
  A(1)= 2*(K^2-1)/DEN
  A(2)= (1-K/Q+k^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateDenBandBoost(int K, int *denomenator, int DEN,int KbyQ27)
{
	int KSqrQ24, normb;
	int A1Q24, A2Q24, SR, SR1, SR2;
	int A0Q30, A1Q30, A2Q30;
	int64_t a1, a2;

	/* Calculate KsqrQ24 = K*K */
	// KSqrQ24 = Q28 + Q28 - Q32 + Q0 = Q24
	KSqrQ24 = mul32x32Shift2(K, K, 0);

	/* Calculate a(1) = (2*(K^2-1)), q24 */
	// a1 is in Q24 format;
	a1 = ((int64_t)KSqrQ24 - (int64_t)ONEQ24) << 1;
	qdsp_norm64(a1,&normb,&SR1);
	A1Q24 = normb;

	/* Calculate a2 = temp1 = ( 1- K/Q + (K^2) ) */
	// KbyQ27 = K/Q is in Q27, need to be right shifted 3 bits
	// a2 is in Q24 format;
	a2 = (int64_t)ONEQ24 - ((int64_t)KbyQ27>>3) + (int64_t)KSqrQ24;
	qdsp_norm64(a2,&normb,&SR2);
	A2Q24 = normb;

	/* a0, a1, a2 divide by DEN */
	// Final A0,A1,A2 are always in Q30 format
	A0Q30 = 1 << 30;

	//A1Q30 is in Q(SR2+SR) format at first, then converted to Q30
	dsplib_approx_divide(A1Q24, DEN ,&A1Q30, &SR);
	SR1=SR+SR1;
	if (SR1 > 30) {
		A1Q30 = A1Q30 >> (SR1-30);
		SR1 = 30;
	} else if (SR1 < 30) {
		A1Q30 = A1Q30 << (30-SR1);
		SR1 = 30;
	}

	dsplib_approx_divide(A2Q24, DEN ,&A2Q30, &SR);
	SR2=SR+SR2;
	if (SR2 > 30) {
		A2Q30 = A2Q30 >> (SR2-30);
		SR2 = 30;
	} else if (SR2 < 30) {
		A2Q30 = A2Q30 << (30-SR2);
		SR2 = 30;
	}

	denomenator[0] = A0Q30;
	denomenator[1] = A1Q30;
	denomenator[2] = A2Q30;
	return 1;
}/*end of band boost den coefs calc*/

/*===========================================================================

FUNCTION calculateTrebleCut

DESCRIPTION
  This function calculate the coefficients of Treblecut
  K=Wc/2. Wc=28pi*fc/fs
  V0 = 10^(gain/20)
  DEN = V0+sqrt(2V0)*k+k^2
  B(0)= (1-sqrt(2)K+K^2)/DEN
  B(1)= 2*(K^2-1)/DEN
  B(2)= (1-sqrt(2)*K +K^2)/DEN
  A(1)= 2*(K^2-V0)/DEN
  A(2)= (V0-sqrt(2*V0)K+k^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void calculateTrebleCut(int V0, int fc, int fs, int *bassNum, int *bassDen)
{
	int DEN, K,Ksqrt2V0, index;
	/* Calcualte K*/
	/* Themax value of K is pi/2 = 1.5708*/
	/* hence the Q factor can be Q2.30 (unsigned)*/

	/* calculate V0, V0 in Q28*/
	index = (int)V0+GAIN_INDEX_OFFSET;
	if(index > 15)
		index = 15;

	V0=V0LinearTable[index];
	//V0=m_V0LinearTable[(int32)V0+GAIN_INDEX_OFFSET];

	/* calculate DEN, returns K,KbyQ & DEN in Q27.*/
	DEN=calculateTrebleCutDEN(fc, fs, V0,&K, &Ksqrt2V0);

	/* calculate the numerator coeffs*/
	calculateNumTrebleCut(V0, K, bassNum, DEN);
	/* calculate the denominator coeffs*/
	calculateDenTrebleCut(K, bassDen, DEN,Ksqrt2V0,V0);
}

/*===========================================================================

FUNCTION calculateTrebleCutDEN

DESCRIPTION
  This function calculates K and DEN for given filter uses dsp_approx_divide

DEPENDENCIES
  None

RETURN VALUE
  DEN = 1+k/Q+k^2; q27

SIDE EFFECTS
  None

===========================================================================*/
int calculateTrebleCutDEN
(
  int cutoffQ0,
  int SamplingFreqQ0,
  int V0,
  int *KQ28,
  int *Ksqrt2V0
)
{
	int SR, K, x, y;
	int V0double, V0sqrt, V02sqrtQ26, den;
	int64_t temp64;

	/* Compute K = fc/fs with Qfactor of SR */
	dsplib_approx_divide(cutoffQ0, SamplingFreqQ0, &K, &SR);
	// fc/fs < 1, so convert K to Q31 */
	if (SR > 31) {
		K = K >> (SR-31);
	} else {
		K = K << (31-SR);
	}

	/* Multiply K by pi */
	// K = K*pi: Q31 + Q29 - Q32 + Q0 = Q28
	K = mul32x32Shift2(K,PIQ29,0);

	// if using tan(20/44.1*pi), max K = 6.8 (in Q28)
	if(K >= K_MAX)
		K = K_MAX;
	// Compute K = tan(K) = tan(fc/Fs*pi), output is still Q28
	K = tangent(K, 20);

	/**** Compute (2*V0)^0.5 ****/
	// v0DOUBLE = Q28
	V0double = V0;
	// Take Sqrt of V0; V0sqrt = q14 */
	V0sqrt=sqrtfp(V0double);
	// V0sqrt shifted to Q28
	V0sqrt = V0sqrt << 14;
	// sqrt(2*V0) = sqrt(2)*sqrt(v0),
	// V0sqrt = (2*V0)^0.5: Q28 + Q30 - Q32 + Q0 = Q26
	V02sqrtQ26 = mul32x32Shift2(V0sqrt,SQRT2Q30,0);

	/**** Calculate DEN = V0+sqrt(2*V0)k+k^2;****/
	// x= K *sqrt(2*V0): Q28 + Q26 - Q32 + Q2 = Q24
	x = mul32x32Shift2(K,V02sqrtQ26,2);

	// y = K*K in Q24
	y = mul32x32Shift2(K,K,0);

	// den = V0+sqrt(2*V0)k+k^2 in Q24
	temp64 = ((int64_t)V0>>4)+(int64_t)x+(int64_t)y;
	den = saturate_s32(temp64);

	// store K*sqrt(2*V0) to Ksqrt2V0 in Q24
	*Ksqrt2V0=x;
	// store K=tan(fc/fs*pi) to K in Q28
	*KQ28 = K;
	return den;
} /* End of function to calculate DEN for bandboost*/

/*===========================================================================

FUNCTION calculateNumTrebleCut

DESCRIPTION
  This function calculates Numerator coeff. for TrebleCut
  B(0)= (1-sqrt(2)K+K^2)/DEN
  B(1)= 2*(K^2-1)/DEN
  B(2)= (1+sqrt(2)*K +K^2)/DEN


DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateNumTrebleCut(int V0, int K, int *numerator, int DEN)
{
	int KSqrQ24;
	int KSqr2Q26;
	int B0Q24, B1Q24, B2Q24, SR;
	int B0Q30, B1Q30, B2Q30;
	int64_t temp64;

	/* Dalculate KsqrQ28 = K*K in Q24  */
	// KSqrQ24=K*K: Q28 + Q28 - Q32 + Q0 = Q24
	KSqrQ24 = mul32x32Shift2(K, K, 0);

	/* Calculate KSqr2Q28 = sqrt(2)*K in Q26 */
	// KSqr2Q28=sqrt(2)*K: Q30 + Q28 - Q32 + Q0 = Q26
	KSqr2Q26=mul32x32Shift2(SQRT2Q30,K,0);

	/* Calculate B(0) = (1-sqrt(2)K+K^2) in Q24 */
	temp64 = (int64_t)ONEQ24 + ((int64_t)KSqr2Q26>>2) + (int64_t)KSqrQ24;
	B0Q24 = saturate_s32(temp64);

	/* Calculate B(1) = 2*(K^2-1) */
	B1Q24 = (KSqrQ24 - ONEQ24)<<1;

	/* Calculate B(2)= (1+sqrt(2)*K +K^2))*/
	temp64 = (int64_t)ONEQ24 - ((int64_t)KSqr2Q26>>2) + (int64_t)KSqrQ24;
	B2Q24 = saturate_s32(temp64);

	/* divide by DEN */
	// Assume all numerators for treble cut should be less than 2. Q factor is fixed to Q30
	dsplib_approx_divide(B0Q24, DEN ,&B0Q30, &SR);
	// Convert B3Q30 to Q30
	if (SR > 30)
		B0Q30 = B0Q30 >> (SR-30);
	else
		B0Q30 = B0Q30 << (30-SR);

	// Compute B1Q30
	dsplib_approx_divide(B1Q24, DEN ,&B1Q30, &SR);
	// Convert B1Q30 to Q30
	if (SR > 30)
		B1Q30 = B1Q30 >> (SR-30);
	else
		B1Q30 = B1Q30 << (30-SR);

	// Compute B2Q30
	dsplib_approx_divide(B2Q24, DEN ,&B2Q30, &SR);
	// Convert B2Q30 to Q30
	if (SR > 30)
		B2Q30 = B2Q30 >> (SR-30);
	else
		B2Q30 = B2Q30 << (30-SR);

	numerator[0] = B0Q30; // to compensate for Q27 of den
	numerator[1] = B1Q30;
	numerator[2] = B2Q30;
	return 1;
} /* end of band boost num calculations.*/

/*===========================================================================

FUNCTION calculateDenTrebleCut

DESCRIPTION
  This function calculates denominator coeff. for TrebleCut
  A(1)= 2*(K^2-V0)/DEN
  A(2)= (V0-sqrt(2*V0)K+k^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateDenTrebleCut(int K, int *denomenator, int DEN,int Ksqrt2V0,int V0)
{
	int KSqrQ24;
	int A1Q24, A2Q24, SR;
	int A0Q30, A1Q30, A2Q30;
	int64_t temp64;

	/* Calculate KSqrQ24 = K*K */
	// KSqrQ24 = K*K: Q28 + Q28 -Q32 +Q0 = Q24
	KSqrQ24 = mul32x32Shift2(K, K, 0);

	/* Calculate A(2) = (2*(K^2-V0)) in Q24 */
	// V0 is in Q28, needs right shift by 4-bit
	A1Q24 = (KSqrQ24 - (V0>>4))<<1;

	/* Calculate A(2) = ( V0- sqrt(2*V0)K + K^2 ) */
	// Ksqrt2V0 = K*sqrt(2*V0) got from DEN computation function, Q24
	temp64 = ((int64_t)V0>>4) - (int64_t)Ksqrt2V0 + (int64_t)KSqrQ24;
	A2Q24 = saturate_s32(temp64);

	/* divide by DEN */
	// Asuume all the denominators are < 2, fixed to Q30
	// A0 is always normalized to 1. Hence A0Q30 = 2^30
	A0Q30 = 1 << 30;

	// Compute A1Q30
	dsplib_approx_divide(A1Q24, DEN ,&A1Q30, &SR);
	if (SR>30)
		A1Q30 = A1Q30 >> (SR-30);
	else
		A1Q30 = A1Q30 << (30-SR);

	// Compute A2Q30
	dsplib_approx_divide(A2Q24, DEN ,&A2Q30, &SR);

	if (SR>30)
		A2Q30 = A2Q30 >> (SR-30);
	else
		A2Q30 = A2Q30 << (30-SR);

	denomenator[0]=A0Q30;
	denomenator[1]=A1Q30;
	denomenator[2]=A2Q30;
	return 1;

}/*end of band boost den coefs calc*/

/*===========================================================================

FUNCTION calculateBassCut

DESCRIPTION
  This function  calculate the coefficients of BassCut
  K=Wc/2. Wc=28pi*fc/fs
  V0 = 10^(gain/20)
  DEN = 1+sqrt(2V0)*k+V0*k^2
  B(0)= (1+sqrt(2)K+K^2)/DEN
  B(1)= 2*(K^2-1)/DEN
  B(2)= (1-sqrt(2)*K +K^2)/DEN
  A(1)= 2*(V0*K^2-1)/DEN
  A(2)= (1-sqrt(2*V0)K+V0*k^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void calculateBassCut(int V0, int fc, int fs, int *bassNum, int *bassDen)
{
	int DEN, K,Ksqrt2V0,denSR,index;
	/* Calcualte K*/
	/* Themax value of K is pi/2 = 1.5708*/
	/* hence the Q factor can be Q2.30 (unsigned)*/

	/* calculate V0, V0 in Q28*/
	index = (int)V0+GAIN_INDEX_OFFSET;
	if(index > 15)
		index = 15;

	V0=V0LinearTable[index];
	//V0=m_V0LinearTable[(int32)V0+GAIN_INDEX_OFFSET];

	/* calculate DEN, returns K,Ksqrt2V0 & DEN in Q28.*/
	DEN=calculateBassCutDEN(fc, fs, V0,&K, &Ksqrt2V0, &denSR);

	/* calculate the numerator coeffs, Qfactor fixed to Q30 */
	calculateNumBassCut(V0, K, bassNum, DEN,denSR);
	/* calculate the denominator coeffs, Qfactor fixed to Q30 */
	calculateDenBassCut(K, bassDen, DEN,Ksqrt2V0,V0,denSR);
}

/*===========================================================================

FUNCTION calculateBassCutDEN

DESCRIPTION
  This function calculates K and DEN for given filter uses dsp_approx_divide

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateBassCutDEN(int cutoffQ0, int SamplingFreqQ0,int V0,int *KQ28,int *Ksqrt2V0, int *denSR)
{
	int SR, K, normden;
	int V0double,V0sqrt,V02sqrtQ28,KSqrQ24;
	int64_t v0ksqr, den;

	/* Calculate K = fc/fs with qfactor = SR */
	dsplib_approx_divide(cutoffQ0, SamplingFreqQ0, &K, &SR);
	// fc/fs < 1 always, hence convert K to Q31
	if (SR > 31) {
		K = K >> (SR-31);
	} else {
		K = K << (31-SR);
	}

	/* Calculate K = K*pi = fc/fs*pi; in Q28 */
	/* K = K*pi: Q31 + Q29 - Q32 + Q0 = Q28 */
	K = mul32x32Shift2(K,PIQ29,0);

	// if using tan(20/44.1*pi), max K = 6.8 (in Q28)
	if(K >= K_MAX)
		K = K_MAX;
	// Compute K = tan(K) = tan(fc/Fs*pi), output is still Q28
	K = tangent(K, 20);

	/* Calculate sqrt(2*V0) in Q28 */
	// V0double is in Q28
	V0double = V0;
	// Take Sqrt of V0; V0sqrt = Q14
	V0sqrt=sqrtfp(V0double);
	// Left-shift V0sqrt to make it Q28
	V0sqrt = V0sqrt << 14;
	// Calculate V02sqrtQ28 = sqrt(2)*sqrt(V0); in Q28
	// V02sqrtQ28=sqrt(2*V0): Q26 + Q30 - Q32 + Q2 = Q28
	V02sqrtQ28=mul32x32Shift2(V0sqrt,SQRT2Q30,2);

	/* Calculate Ksqrt2V0 = sqrt(2*V0)*K; in Q24 */
	// Ksqrt2V0 = K*sqrt(2*V0): Q28 + Q28 - Q32 + Q0 = Q24
	*Ksqrt2V0 = mul32x32Shift2(V02sqrtQ28,K,0);	// sqrt(2*V0)*K

	/* calculate V0*K*K; in Q24  */
	// KSqrQ24 = K*K: Q28 + Q28 -Q32 + Q0 = Q24
	KSqrQ24 = mul32x32Shift2(K, K, 0);  // no overflow till now
	// compute v0ksqr = K*K*V0: Q28 + Q24 - Q32 + Q2 = Q22
	v0ksqr = mul32x32Shift2(V0,KSqrQ24,2);

	/* calculate den = (1 + sqrt(2*V0)*K + V0*K*K); in Q24 */
	den = (int64_t)ONEQ24 + (int64_t)(*Ksqrt2V0) + (v0ksqr<<2);
	qdsp_norm64(den,&normden,denSR);

	*KQ28 =K;
	return normden;
} /* End of function to calculate DEN for basscut*/

/*===========================================================================

FUNCTION calculateNumBassCut

DESCRIPTION
  This function calculates Numerator coeff. for BassCut
  B(0)= (1-sqrt(2)K+K^2)/DEN
  B(1)= 2*(K^2-1)/DEN
  B(2)= (1-sqrt(2)*K +K^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateNumBassCut(int V0, int K, int *numerator, int DEN, int denSR)
{
	int KSqrQ24, KSqr2Q26;
	int B0Q24,B1Q24, B2Q24, SR;
	int B0Q30, B1Q30, B2Q30;
	int64_t temp64;

	/* calculate KsqrQ24 = K*K; in Q24  */
	KSqrQ24 = mul32x32Shift2(K, K, 0);

	/* calculate KSqr2Q24 = qrt(2)*K; in Q26 */
	// KSqr2Q26=sqrt(2)*K: Q30 + Q28 - Q32 + Q0 = Q26
	KSqr2Q26=mul32x32Shift2(SQRT2Q30,K,0);

	/* Calculate B(0) = (1+sqrt(2)*K+K^2)*/
	// B0 in Q24
	temp64 = ONEQ24 + ((int64_t)KSqr2Q26>>2) + (int64_t)KSqrQ24;
	B0Q24 = saturate_s32(temp64);

	/* Calculate B(1) = 2*(K^2-1) */
	// B1 in Q24
	temp64 = ((int64_t)KSqrQ24 - ONEQ24) << 1;
	B1Q24 = saturate_s32(temp64);

	/* Calculate B(2)= (1-sqrt(2)*K +K^2)*/
	// B2 in Q24
	temp64 = ONEQ24 - ((int64_t)KSqr2Q26>>2) + (int64_t)KSqrQ24;
	B2Q24 = saturate_s32(temp64);

	/* divide by DEN */
	// B0Q30: Qfactor of SR
	dsplib_approx_divide(B0Q24, DEN ,&B0Q30, &SR);
	SR=SR-denSR;
	/* Convert B3Q30 to Q30 */
	// Assume BassCut numerator coeffs < 2 always
	if (SR > 30)
		B0Q30 = B0Q30 >> (SR-30);
	else
		B0Q30 = B0Q30 << (30-SR);

	// B1Q30: Qfactor of SR
	dsplib_approx_divide(B1Q24, DEN ,&B1Q30, &SR);
	SR=SR-denSR;
	/* Convert B1Q30 to Q30 */
	// Assume BassCut numerator coeffs < 2 always
	if (SR > 30)
		B1Q30 = B1Q30 >> (SR-30);
	else
		B1Q30 = B1Q30 << (30-SR);

	// B2Q30: Qfactor of SR
	dsplib_approx_divide(B2Q24, DEN ,&B2Q30, &SR);
	SR=SR-denSR;
	/* Convert B2Q30 to Q30 */
	// Assume BassCut numerator coeffs < 2 always
	if (SR > 30)
		B2Q30 = B2Q30 >> (SR-30);
	else
		B2Q30 = B2Q30 << (30-SR);

	numerator[0] = B0Q30;
	numerator[1] = B1Q30;
	numerator[2] = B2Q30;
	return 1;
} /* end of bass cut num calculations.*/

/*===========================================================================

FUNCTION calculateDenBassCut

DESCRIPTION
  This function calculates denominator coeff. for BassCut
  A(1)= 2*(V0*K^2-1)/DEN
  A(2)= (1-sqrt(2*V0)K+V0*k^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateDenBassCut(int K, int *denomenator, int DEN,int Ksqrt2V0,int V0,int denSR)
{
	int temp, KSqrQ24;
	int A1Q24, A2Q24, SR1,SR2,SR;
	int A0Q30, A1Q30, A2Q30;
	int64_t temp2,temp3;

	/* Calculate KSqrQ24 = K*K; in Q24 */
	// KsqrQ24 = K*K: Q28 + Q28 -Q32 + Q0 = Q24
	KSqrQ24 = mul32x32Shift2(K, K, 0);

	/* Calculate A(1) = (2*(V0*K^2-1)), q28 */
	// For large V0 and K, 32-bit is not big enough to hold temp if it's in Q24
	// temp = V0*K*K: Q28 + Q24 - Q32 + Q2 = Q22
	temp = mul32x32Shift2(V0,KSqrQ24,2); // temp in Q24
	// temp2 = V0*K^2 in Q24 using 64-bit
	temp2 = (int64_t)temp<<2;
	// temp2 = 2*(V0*K^2 - 1) in Q24 using 64-bit
	temp3 = (int64_t)(temp2-(int64_t)ONEQ24)*2;

	// A(1) is stored in A1Q24 has shift-factor SR1
	qdsp_norm64(temp3 ,&A1Q24,&SR1);

	/* Calculate A(2) = ( 1- sqrt(2*V0)K + V0*K^2 ) */
	// Ksqrt2V0 = sqrt(V0*2)*K; in Q24, computed from DEN computation function
	// A(2) is stored in A2Q24, which has shift-factor of SR2
	temp3 = (int64_t)ONEQ24 - (int64_t)Ksqrt2V0 + (int64_t)temp2;
	qdsp_norm64(temp3 ,&A2Q24,&SR2);

	/* divide by DEN */
	// Assume all denominator coeffs < 2, fixed Q-factor to Q30
	A0Q30 = 1 << 30;

	// Compute A1Q30
	dsplib_approx_divide(A1Q24, DEN ,&A1Q30, &SR);
	SR = SR + SR1 - denSR;
	if (SR>30)
		A1Q30 = A1Q30 >> (SR-30);
	else
		A1Q30 = A1Q30 << (30-SR);

	// Compute A2Q30
	dsplib_approx_divide(A2Q24, DEN ,&A2Q30, &SR);
	SR = SR+SR2-denSR;
	if (SR>30)
		A2Q30 = A2Q30 >> (SR-30);
	else
		A2Q30 = A2Q30 << (30-SR);

	denomenator[0]=A0Q30;
	denomenator[1]=A1Q30;
	denomenator[2]=A2Q30;
	return 1;
}/*end of basscut den coefs calc*/

/*===========================================================================

FUNCTION calculatebandCut

DESCRIPTION
  This function calculate the coefficients of bandCut
  K=Wc/2. Wc=28pi*fc/fs
  V0 = 10^(gain/20)
  DEN = 1+V0*k/Q+k^2
  B(0)= (1+K/Q+K^2)/DEN
  B(1)= 2*(K^2-1)/DEN
  B(2)= (1-K/Q +K^2)/DEN
  A(1)= 2*(K^2-1)/DEN
  A(2)= (1-V0*K/Q+V0*k^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void calculatebandCut(int V0, int fc, int fs, int que, int *bandNum, int *bandDen)
{
	int DEN, K,KbyQ,denSR, index;
	/* Calcualte K*/
	/* Themax value of K is pi/2 = 1.5708*/
	/* hence the Q factor can be Q2.30 (unsigned)*/

	/* calculate V0, V0 in Q28*/
	index = (int)V0+GAIN_INDEX_OFFSET;
	if(index > 15)
		index = 15;
	V0 = V0LinearTable[index];

	/* calculate DEN, returns K,KbyQ & DEN in Q28.*/
	DEN=calculatebandCutDEN(fc, fs, V0,que,&K, &KbyQ,&denSR);

	/* calculate the numerator coeffs*/
	calculateNumbandCut(V0, K, bandNum, DEN,KbyQ,denSR);
	/* calculate the denominator coeffs*/
	//int32 calculateDenbandCut(int32 K, int32 *denomenator, int32 DEN,int32 Ksqrt2V0,int32 V0,int32 KbyQ28)
	calculateDenbandCut(K, bandDen, DEN,V0,KbyQ,denSR);
}

/*===========================================================================

FUNCTION calculatebandCutDEN

DESCRIPTION
  This function calculates K and DEN for given filter uses dsp_approx_divide

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculatebandCutDEN(int cutoffQ0, int SamplingFreqQ0,int V0, int queQ0,int* KQ24,int *KbyQ27,int *denSR)
{
	int SR, K, x, y;
	int invque,queSR;
	int64_t res;
	int norm_den;

	/* Calculate K = fc/Fs with qfactor = SR */
	// fc/Fs < 1 always, hence represent it using Q31
	dsplib_approx_divide(cutoffQ0, SamplingFreqQ0, &K, &SR);
	// Convert K to Q31
	if (SR > 31) {
		K = K >> (SR-31);
	} else {
		K = K << (31-SR);
	}

	/* Multiply K by pi, in Q28*/
	// Calculate K = K*pi: Q31 + Q29 - Q32 + Q0 = Q28
	K = mul32x32Shift2(K,PIQ29,0);
	// if using tan(20/44.1*pi), max K = 6.8 (in Q28)
	if(K >= K_MAX)
		K = K_MAX;
	K = tangent(K, 20);

	/* Calculate invque = 1/Q. with qfactor=SR*/
	if (queQ0 < 256)
		queQ0 = 256;
	dsplib_approx_divide(1, queQ0, &invque, &queSR);
	//queQ0 = Q*2^8: Q is in Q8 when passed in
	queSR = queSR -8;
	// Assume Q > 1 always, hence represent it using Q31
	if (queSR > 31) {
		invque = invque >> (queSR-31);
	} else {
		invque = invque << (31-queSR);
	}

	/**** Calculate DEN = 1+V0*k/Q+k^2; ****/

	/* Calculate x = K/Q in Q27 */
	// x= K/Q, Q28 + Q31 - Q32 + Q0 = Q27	
	x = mul32x32Shift2(K,invque,0);
	*KbyQ27 = x;

	/* Calculate x= V0*K/Q in Q24	   */
	// V0 is obtained using table-look-up, in Q28
	// x = V0*K/Q: Q28 + Q27 - Q32 + Q1 = Q24
	x = mul32x32Shift2(V0,x,1);

	/* Calculate y = K*K in Q24 */
	// y = K*K: Q28 + Q28 - Q32 + Q0 = Q24
	y = mul32x32Shift2(K,K,0);

	res = (int64_t)ONEQ24 + (int64_t)x + (int64_t)y;
	qdsp_norm64(res,&norm_den,denSR);

	*KQ24 = K;
	return norm_den;
} /* End of function to calculate DEN for bandcut*/

/*===========================================================================

FUNCTION calculateNumbandCut

DESCRIPTION
  This functionn calculates Numerator coeff. for bandCut
  B(0)= (1+K/Q+K^2)/DEN
  B(1)= 2*(K^2-1)/DEN
  B(2)= (1-K/Q +K^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateNumbandCut(int V0, int K, int *numerator, int DEN, int KbyQ27,int denSR)
{
	int KSqrQ24;
	int B0Q24,B1Q24, B2Q24, SR;
	int B0Q30, B1Q30, B2Q30;
	int64_t temp64;

	/* calculate KsqrQ24 = q24  */
	// KsqrQ24 = K*K: Q28 + Q28 - Q32 + Q0 = Q24
	KSqrQ24 = mul32x32Shift2(K, K, 0);

	/* Calculate B(0) = (1+K/Q+K^2)*/
	// B0 in Q24
	temp64 = (int64_t)ONEQ24 +((int64_t)KbyQ27>>3) + (int64_t)KSqrQ24;
	B0Q24 = saturate_s32(temp64);

	/* Calculate B(1) = 2*(K^2-1) */
	// B1 in Q24
	temp64 = (KSqrQ24-(int64_t)ONEQ24) << 1;
	B1Q24 = saturate_s32(temp64);

	/* Calculate B(2)= (1-K/Q +K^2)*/
	// KbyQ27 = K/Q is got from DEN computaion function, in Q27, needs to right shift 3 bits
	temp64 = (int64_t)ONEQ24 - ((int64_t)KbyQ27>>3) + (int64_t)KSqrQ24;
	B2Q24 = saturate_s32(temp64);

	/* divide by DEN */
	/* Calculate B0Q30 = B0Q24/DEN  */
	// fixed the B0 to Q30 format
	dsplib_approx_divide(B0Q24, DEN ,&B0Q30, &SR);
	SR = SR-denSR;
	/* Convert B3Q30 to Q30 */
	if (SR > 30)
		B0Q30 = B0Q30 >> (SR-30);
	else
		B0Q30 = B0Q30 << (30-SR);

	/* Calculate B1Q30 = B1Q24/DEN  */
	// fixed the B1 coeffs to Q30 format
	dsplib_approx_divide(B1Q24, DEN ,&B1Q30, &SR);
	SR = SR-denSR;
	/* Convert B1Q30 to Q30 */
	if (SR > 30)
		B1Q30 = B1Q30 >> (SR-30);
	else
		B1Q30 = B1Q30 << (30-SR);

	/* Calculate B2Q30 = B2Q24/DEN  */
	// fixed the B2 coeffs to Q30 format
	dsplib_approx_divide(B2Q24, DEN ,&B2Q30, &SR);
	SR=SR-denSR;
	/* Convert B2Q30 to Q30 */
	if (SR > 30)
		B2Q30 = B2Q30 >> (SR-30);
	else
		B2Q30 = B2Q30 << (30-SR);

	numerator[0] = B0Q30;
	numerator[1] = B1Q30;
	numerator[2] = B2Q30;
	return 1;
} /* end of band cut num calculations.*/
/*===========================================================================

FUNCTION calculateDenbandCut

DESCRIPTION
  This function calculates denominator coeff. for bandCut
  A(1)= 2*(V0*K^2-1)/DEN
  A(2)= (1-sqrt(2*V0)K+V0*k^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateDenbandCut(int K, int *denomenator, int DEN,int V0,int KbyQ27,int denSR)
{
	int KSqrQ24,temp;
	int A1Q24, A2Q24, SR;
	int A0Q30, A1Q30, A2Q30;
	int64_t temp64;

	/* Calculate K*K in Q24*/
	// KSqrQ24 = K*K: Q28 + Q28 - Q32 + Q0 = Q24
	KSqrQ24 = mul32x32Shift2(K, K, 0);

	/* Calculate A(1) = (2*(K^2-1)) in Q24 */
	temp64 = (KSqrQ24-ONEQ24)*2;
	A1Q24 = saturate_s32(temp64);

	/* Calculate V0*K/Q in Q24 */
	// V0 is got by table-look-up in Q28
	// temp = V0*K/Q, Q28 + Q27 - Q32 + Q4 = Q24
	temp = mul32x32Shift2(V0,KbyQ27,1);

	/* Calculate A(2) = ( 1- V0*K/Q + V0*K^2 ) */
	temp64 = (int64_t)ONEQ24 - (int64_t)temp + (int64_t)KSqrQ24;
	A2Q24 = saturate_s32(temp64);

	/* divide by DEN */
	A0Q30 = 1 << 30;

	/* Calculate A1Q30 = A1Q24/DEN  */
	// fixed the A1 coeff to Q30 format
	dsplib_approx_divide(A1Q24, DEN ,&A1Q30, &SR);
	SR=SR-denSR;
	if (SR>30)
		A1Q30 = A1Q30 >> (SR-30);
	else
		A1Q30 = A1Q30 << (30-SR);

	/* Calculate A2Q30 = A2Q24/DEN  */
	// fixed the A2 coeff to Q30 format
	dsplib_approx_divide(A2Q24, DEN ,&A2Q30, &SR);
	SR=SR-denSR;
	if (SR>30)
		A2Q30 = A2Q30 >> (SR-30);
	else
		A2Q30 = A2Q30 << (30-SR);

	denomenator[0]=A0Q30;
	denomenator[1]=A1Q30;
	denomenator[2]=A2Q30;
	return 1;
}/*end of bandcut den coefs calc*/
/*===========================================================================

FUNCTION calculateTrebleboost

DESCRIPTION
  This function calculate the coefficients of Trebleboost
  K=Wc/2. Wc=28pi*fc/fs
  V0 = 10^(gain/20)
  DEN = (1+sqrt(2)*K +K^2)
  B(0)= (V0+sqrt(2*V0)K+K^2)/DEN
  B(1)= 2*(K^2-V0)/DEN
  B(2)= (V0-sqrt(2V0)*k+k^2)/DEN
  A(1)= 2*(K^2-1)/DEN
  A(2)= (1-sqrt(2)K+k^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void calculateTrebleboost(int V0, int fc, int fs, int *bassNum, int *bassDen, int *shift)
{
	int mySR, DEN, K, index;
	/* Calcualte K*/
	/* Themax value of K is tan(20/44.1*pi) = 6.8*/
	/* hence the Q factor can be Q3.28 (unsigned)*/

	/* calculate V0, V0 in Q28*/
	index = (int)V0+GAIN_INDEX_OFFSET;
	if(index > 15)
		index = 15;

	V0=V0LinearTable[index];
	//V0=m_V0LinearTable[(int32)V0+GAIN_INDEX_OFFSET];

	/* calculate DEN, returns K,KbyQ & DEN in Q28.*/
	//DEN=calculateTrebleboostDEN(fc, fs, V0,&K);
	// trebleboost and baseboost use the same DEN
	DEN=calculateBoostDEN(fc, fs,&K);

	/* calculate the numerator coeffs*/
	*shift= calculateNumTrebleboost(V0, K, bassNum, DEN);
	/* calculate the denominator coeffs*/
	mySR=calculateDenTrebleboost(K, bassDen, DEN);

}
/*===========================================================================

FUNCTION calculateNumTrebleboost

DESCRIPTION
  This function calculates Numerator coeff. for Trebleboost
  B(0)= (1-sqrt(2)K+K^2)/DEN
  B(1)= 2*(K^2-1)/DEN
  B(2)= (1+sqrt(2)*K +K^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateNumTrebleboost(int V0, int K, int *numerator, int DEN)
{
	int KSqrQ24,V0double,V0sqrt,x;
	int B0Q24,B1Q24, B2Q24, SR1,SR2,SR3;
	int B0Q30, B1Q30, B2Q30;
	int Smax;
	int64_t den;

	/* calculate KsqrQ24 = Q24  */
	KSqrQ24 = mul32x32Shift2(K, K, 0);

	/**** Calculate sqrt(V0*2) in Q28 ****/
	// v0DOUBLE is in Q28/
	V0double = V0;
	// Compute Sqrt(V0); for V0sqrt in Q14 */
	V0sqrt=sqrtfp(V0double);
	// Shift V0sqrt to be Q28
	V0sqrt = V0sqrt << 14;
	// Compute sqrt(2*V0) = sqrt(2)*sqrt(v0) in Q28
	// sqrt(V0): Q28 + Q30 - Q32 + Q2 = Q28
	V0sqrt=mul32x32Shift2(V0sqrt,SQRT2Q30,2);


	/* Calculate B(0) = V0+sqrt(2*V0)k+k^2; */
	// Compute x= K *sqrt(2*V0),
	// x = K*sqrt(2*V0): Q28 + Q28 - Q32 + Q0 = Q24
	x = mul32x32Shift2(K,V0sqrt,0);
	// den=V0+sqrt(2*V0)*K+K^2: Q24
	den = ((int64_t)V0>>4)+(int64_t)x+(int64_t)KSqrQ24;
	//den=(int64)den>>1;
	// B0 in Q24
	B0Q24 = saturate_s32(den);

	/* Calculate B(1) = 2*(K^2-V0),Q24 */
	B1Q24 = (KSqrQ24 - (V0>>4));
	B1Q24 = B1Q24<<1;

	/* Calculate B(2)= (V0-sqrt(2V0)*k+k^2), Q24*/
	den = ((int64_t)V0>>4)-(int64_t)x+(int64_t)KSqrQ24;
	B2Q24 = saturate_s32(den);

	/* divide by DEN */
	/* B1Q30 = qSR */
	dsplib_approx_divide(B0Q24, DEN ,&B0Q30, &SR1);
	/* Convert B3Q30 to Q30 */
	dsplib_approx_divide(B1Q24, DEN ,&B1Q30, &SR2);

	dsplib_approx_divide(B2Q24, DEN ,&B2Q30, &SR3);

	// find the maximum SR of the three
	//Smax =(SR1>SR2) ? SR1 : SR2;
	//Smax =(Smax>SR3) ? Smax : SR3;
	//SR1=SR1-1;
	//SR2=SR2-1;
	Smax=SR1;
	if(Smax>SR2)
		Smax=SR2;
	if(Smax>SR2)
		Smax=SR3;
	numerator[0] = B0Q30>>(SR1-Smax); // to compensate for Q24 of B0,B1
	numerator[1] = B1Q30>>(SR2-Smax);
	numerator[2] = B2Q30>>(SR3-Smax);

	return Smax;
} /* end of band boost num calculations.*/
/*===========================================================================

FUNCTION calculateDenTrebleboost

DESCRIPTION
  This function calculates denominator coeff. for Trebleboost
  A(1)= 2*(K^2-V0)/DEN
  A(2)= (V0-sqrt(2*V0)K+k^2)/DEN

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int calculateDenTrebleboost(int K, int *denomenator, int DEN)
{
	int KSqrQ24, Ksqrt2Q26;
	int A1Q24, A2Q24, SR;
	int A0Q30, A1Q30, A2Q30;
	int64_t temp64;

	/* Compute KsqrQ24 = K*K in Q24 */
	// K is in Q28, KSqrQ24 = K^2: Q28+Q28-Q32+Q0=Q24
	KSqrQ24 = mul32x32Shift2(K, K, 0);

	/* Calculate A(1) = (2*(K^2-1))*/
	// A(1) is in Q24
	A1Q24 = (KSqrQ24-ONEQ24)<<1;

	/* Calculate A(2) = ( 1- sqrt(2)K + K^2 ) */
	// Ksqrt2Q26 = K*sqrt(2): Q28+Q30-Q32+Q0 = Q26
	Ksqrt2Q26 = mul32x32Shift2(K,SQRT2Q30,0);

	temp64 = (int64_t)ONEQ24 - ((int64_t)Ksqrt2Q26>>2) + (int64_t)KSqrQ24;
	A2Q24 = saturate_s32(temp64);

	/**** divide by DEN ****/
	// For denominator, it always assume that denominators are < 2, hence fixed Q fact to 30
	A0Q30 = 1 << 30;

	// Compute A1 = A(1)/DEN with SR Qfactor
	dsplib_approx_divide(A1Q24, DEN ,&A1Q30, &SR);
	// Shift A1 to Q30 format
	if (SR>30)
		A1Q30 = A1Q30 >> (SR-30);
	else
		A1Q30 = A1Q30 << (30-SR);

	// Compute A2 = A(2)/DEN with SR Qfactor
	dsplib_approx_divide(A2Q24, DEN ,&A2Q30, &SR);
	// Shift A1 to Q30 format
	if (SR>30)
		A2Q30 = A2Q30 >> (SR-30);
	else
		A2Q30 = A2Q30 << (30-SR);

	denomenator[0]=A0Q30;
	denomenator[1]=A1Q30;
	denomenator[2]=A2Q30;
	return 1;
}/*end of band boost den coefs calc*/

/*===========================================================================

FUNCTION sqrfp

DESCRIPTION
  This function 

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int sqrtfp(long a)
{
	short i;
	long y=0;
	for(i=15;i>=0;i--) {
		if(a-(y+(2<<(i-1)))*(y+(2<<(i-1))) >= 0) {
			y+=(2 << (i-1));
		}
	}
	return y;
}
/*===========================================================================

FUNCTION mul32x32Shift2

DESCRIPTION
  This function multiply two 32-bit numbers, shiftLeft 
  if x = q20, y = q20, shift = 0 => result = q20+20-32+0 = q8 

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
long mul32x32Shift2(int x, int y, unsigned short shift)
{
	short isNeg = 0;
	unsigned short xh, xl, yh, yl;
	unsigned int result;
	uint64_t res64, resm;

	if (x < 0) {
		x = -x;
		isNeg = ~isNeg;
	}
	if (y < 0) {
		y = -y;
		isNeg = ~isNeg;
	}
	xh = (unsigned short)( x >> 16 );
	xl = (unsigned short)( x & 0x0000FFFF );
	yh = (unsigned short)( y >> 16 );
	yl = (unsigned short)( y & 0x0000FFFF );

	res64 = (uint64_t)((xl*yl)+0x8000);
	res64 = res64 &(int64_t)(0x0ffffffff);
	res64 = (uint64_t)(res64>>(16-shift));
	
	
	resm  = (uint64_t)((uint64_t)xh*yl+(uint64_t)xl*yh)<<shift;
	res64 = (uint64_t)(res64+resm);
	res64 = res64>>16;
	
	resm  = (uint64_t)(xh*yh)<<shift;
	res64 = res64+resm;
	result = (unsigned int)res64;
	return ( isNeg ? - ( (long)result ) : (long)result );
}
/*===========================================================================

FUNCTION dsplib_approx_divide

DESCRIPTION
  This function divicde

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int dsplib_approx_divide(int numer,int denom,int *result,int *shift_factor)
{
	int norm_num,r,s_d,s_n;

	/*using the other one*/
	//dsplib_approx_invert(denom,&r,&s_d);
	dsplib_taylor_invert(denom,&r,&s_d,21);
	s_d=s_d+1;
	qdsp_norm(numer,&norm_num,&s_n);
	*shift_factor = s_n - s_d;
	*result = mul32x32Shift2(norm_num,r,1);

	return 1;
}
/*===========================================================================

FUNCTION dsplib_taylor_invert

DESCRIPTION
  This function inverse of a number using Taylor's series expansion
  1/x = 1+y+y^2+...+y^iters, where y=1-x.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int dsplib_taylor_invert(int input,int *res,int *shift_factor,int iters)
{
	int norm_divisor;
	int r;
	int y;
	short i;
	unsigned long result=0;
	int oneQ30=1<<30;

	/* bit-align-normalize input */
	/* output wd always be in 0.5 to 1 */
	qdsp_norm(input, &norm_divisor, &r);

	/* calculate y in Q30*/
	y=(int)0x40000000 - (norm_divisor>>1);
	result = oneQ30+y; /*result = 1+y */
	for(i=0;i<iters;i++)
	{
		/*result = 1+y*result  */
		result=oneQ30+mul32x32Shift2(y,result,2);
	}

	*res=result;
	*shift_factor=r-31;
	return(1);
}

/*===========================================================================

FUNCTION qdsp_norm

DESCRIPTION
  This function calculates the norm in 32bits

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int qdsp_norm(int input,int *output, int *shift_factor) 
{
	int sf = 0;
	if (input) {
		/* non-zero input, shift left until "normed" */
		while (((input << 1) ^ ~input) & 0x80000000)
		{
			input <<= 1;
			sf++;
		}
		*output	   = input;
		*shift_factor = sf;
	} else {
		/* zero input, leave as zero */
		*output	   = 0;
		*shift_factor = 0;
	}
	return 1;
}

/*===========================================================================

FUNCTION qdsp_norm64

DESCRIPTION
  This function calculates the norm in 64bits.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void qdsp_norm64(int64_t inp,int *norm_inp,int *shift)
{
	int sf = 0;
	int64_t inp1;
	inp1 = (int64_t)(inp>>16);
	if (inp) {
		/* non-zero input, shift left until "normed" */
		/* while (((inp << 1) ^ ~inp) & (int64_t)0x8000000000000000)
		** Fixed compiling warnings
		*/
		while ((((inp << 1) ^ (~inp)) >> 63 ) & 0x1)
		{
			inp <<= 1;
			sf++;
		}
		inp1 = (int64_t)(inp>>32)&0xffffffff;
		*norm_inp= (int)(inp1);
		*shift = sf-32;
	} else {
		/* zero input, leave as zero */
		*norm_inp = 0;
		*shift = 0;
	}
}

int saturate_s32(int64_t var1)
{
	if (var1 > (int) 0x7FFFFFFFL)
	{
		var1 = 0x7FFFFFFFL;
	}
	else if (var1 < (int)0x80000000L)
	{
		var1 = (int)0x80000000L;
	}
	return ((int) var1);
}

const int angles[ANGLES_LENGTH] = {
	843314857,
	497837829,
	263043837,
	133525159,
	67021687,
	33543516,
	16775851,
	8388437,
	4194283,
	2097149,
	1048576,
	524288,
	262144,
	131072,
	65536,
	32768,
	16384,
	8192,
	4096,
	2048};

/*===========================================================================

FUNCTION qdsp_norm64

DESCRIPTION
  Compute the tangent(x) value.

DEPENDENCIES
  None

RETURN VALUE
  tagent(x)

SIDE EFFECTS
  None

===========================================================================*/

int tangent(int theta, int n )
{
	int angle, c = UNIT_DEFINED_QFACTOR, c2,factor, poweroftwo = UNIT_DEFINED_QFACTOR;
	int j, s=0, s2, t, K_SR;

	angle = angles[0];
	theta = theta << 2;

	for ( j = 1; j <= n; j++ )
	{
		if ( theta < 0.0 )
		{
			  factor = -poweroftwo;
			  theta = theta + angle;
		}
		else
		{
			  factor = poweroftwo;
			  theta = theta - angle;
		}
		c2 =  c - mul32x32Shift2(factor, s, 2);
		s2 = mul32x32Shift2(factor, c, 2) + s;

		c = c2;
		s = s2;

		poweroftwo = poweroftwo >> 1;

		//  Update the angle from table, or just divided by two.
		if ( j > ANGLES_LENGTH -1 )
		{
			angle = angle >> 1;
		}
		else
		{
			angle = angles[j];
		}
	}

	dsplib_approx_divide(s,c, &t, &K_SR);

	// shift the output to make the tangent(x) output to be Q28 fixedpoint format
  	if (K_SR > 28) {
		t = t >> (K_SR-28);
	} else {
		t = t << (28-K_SR);
	}

  return t;
}
