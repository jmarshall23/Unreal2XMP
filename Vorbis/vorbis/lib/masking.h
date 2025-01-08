/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: masking curve data for psychoacoustics
 last mod: $Id: masking.h,v 1.1 2003/01/17 10:58:10 demiurge Exp $

 ********************************************************************/

#ifndef _V_MASKING_H_
#define _V_MASKING_H_

static float ATH_Bark_dB_lspconservative[]={  
   15,   15,   15,    15,    11,    10,     8,    7,    7,    7,
    6,    2,    0,     0,    -3,    -5,    -6,   -6, -4.5f, 2.5f,
   10,   15,   15,    15,    15,    15,   15};

/* The below masking curves are straight from the R. Ehmer
   (J. Acoustical Society of America) papers ca 1958-59.  I modified
   them slightly as Ehmer does not correct for the Absolute Threshold
   of Hearing, and the low dB portions of the curves are thus highly
   suspect. */

/* Let's all do the Trek thing and just call them 'Ehmer curves' ;-)
   Note these are octaves, not Bark scale. */

#define EHMER_OFFSET 16
#define EHMER_MAX 56

static float tone_125_40dB_SL[EHMER_MAX]={
  20,  20,  20,  20,  20,  20,  20,  20,   20, 20, 20, 20, 20, 20, 20, 20,  
  20,  18,  16,  14,  12,  11,   9,   7,    5,  3,  2,  0, -2, -4, -6, -8,
 -10, -12, -14, -16, -18, -20, -900, -900,  -900,-900,-900,-900,-900,-900,-900,-900,  
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_125_60dB_SL[EHMER_MAX]={
  40,  40,  40,  40,  40,  40,  40,  40,   40, 40, 40, 40, 40, 40, 40, 40,  
  40,  38,  36,  34,  32,  31,  29,  27,   25, 23, 22, 20, 18, 16, 14, 12,
  10,   8,   6,   4,   2,   0,  -3,  -5,   -8,-10,-14,-17,-20,-900,-900,-900,  
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_125_80dB_SL[EHMER_MAX]={
  60,  60,  60,  60,  60,  60,  60,  60,   60, 60, 60, 60, 60, 60, 60, 60,  
  60,  58,  56,  54,  52,  51,  49,  47,   45, 43, 42, 40, 38, 36, 34, 32,
  30,  28,  26,  24,  22,  20,  17,  15,   12, 10,  6,  3,  0, -4, -8,-12,  
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_125_100dB_SL[EHMER_MAX]={
  78,  78,  78,  78,  78,  78,  78,  78,   78, 78, 78, 78, 78, 78, 78, 78,  
  78,  77,  75,  74,  73,  72,  71,  70,   69, 68, 67, 65, 64, 63, 62, 60,
  58,  57,  55,  54,  52,  50,  48,  46,   44, 42, 40, 38, 36, 34, 31, 29,  
  27,  24,  22,  20,  18,  15,  13,  11};

static float tone_250_40dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,   -19, -13,  -7,  -1,   4,   9,  15,  20,
  22,  23,  22,  19,  18,  18,  16,  13,     9,   7,   3,   1,  -1,  -3,  -6,  -8,
 -10, -13, -16, -19,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_250_60dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900, -10,   -5,  1,  7, 13, 19, 25, 30, 33,  
  36,  39,  38,  37,  38,  39,  39,  40,   38, 36, 35, 34, 33, 31, 29, 28,
  28,  28,  25,  20,  14,  10,   5,   0,   -5,-10,-15,-20,-900,-900,-900,-900,  
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_250_80dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900, -10,   10, 17, 24, 30, 37, 41, 48, 49,  
  50,  53,  54,  53,  53,  54,  55,  57,   57, 57, 58, 59, 60, 58, 57, 58,
  59,  58,  57,  54,  52,  50,  49,  47,   46, 47, 46, 44, 43, 42, 41, 40,  
  38,  32,  27,  22,  17,  11,   6,   0};
/* no data, just interpolated from 80 */
static float tone_250_100dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900, -10,   15, 25, 34, 40, 50, 56, 60, 70,  
  70,  73,  74,  73,  73,  74,  75,  77,   77, 77, 78, 79, 80, 78, 77, 78,
  79,  78,  77,  74,  72,  70,  69,  67,   66, 67, 66, 64, 63, 62, 61, 60,  
  58,  52,  47,  42,  37,  31,  26,  20};

static float tone_500_40dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900, -20, -14,  -8,  -2,   4,  10,  17,
  23,  16,  12,   9,   6,   3,   0,  -3,    -7, -10, -13, -16, -20,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_500_60dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,   -18, -12,  -6,   0,   6,  13,  20,  30,
  39,  34,  31,  29,  29,  27,  24,  21,    18,  16,  13,   8,   6,   3,   1,  -1,
  -5,  -2,  -5,  -8, -12, -15, -18,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_500_80dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900, -900,-16,-10,  0, 10, 20, 32, 43,  
  53,  52,  52,  50,  49,  50,  52,  55,   55, 54, 51, 49, 46, 44, 44, 42,
  38,  34,  32,  29,  29,  28,  25,  23,   20, 16, 10,  7,  4,  2, -1, -4,  
  -7, -10, -15, -20,-900,-900,-900,-900};
static float tone_500_100dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900, -10,   -7,  2, 10, 19, 27, 35, 55, 56,  
  62,  61,  60,  58,  57,  57,  59,  63,   65, 66, 62, 60, 57, 57, 58, 58,
  57,  56,  56,  56,  57,  57,  56,  57,   57, 54, 47, 41, 37, 28, 21, 16,  
  10,   3,  -3,  -8, -13, -18,-900,-900};

static float tone_1000_40dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900, -20, -10,  0,   9,  20,
  27,  20,  13,  14,  13,   5,  -1,  -6,   -11, -20,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_1000_60dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-13,-3,  7,  17,  25,  37,
  42,  33,  25,  25,  23,  18,  13,   9,     4, -1, -7,-13,-18, -23, -28, -33,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_1000_80dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-14, -4,  6, 16, 27, 33, 50,  
  59,  57,  47,  41,  40,  43,  47,  48,    47, 42, 39, 37, 37, 36, 35, 32,
  30,  27,  21,  15,   5,  -2, -10, -18,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_1000_100dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-10,  0, 10, 23, 33, 45, 60,  
  70,  72,  55,  49,  43,  40,  44,  54,    59, 58, 49, 43, 52, 57, 57, 58,
  58,  54,  49,  47,  42,  39,  33,  28,    20, 15,  5,  0, -5,-15,-900,-900,  
-900,-900,-900,-900,-900,-900,-900,-900};

static float tone_2000_40dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,   5,  12,  20,
  24,  14,   8,   0,  -5,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_2000_60dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,  -2,   8,  19,  31,
  38,  30,  15,  15,  15,  13,   8,   5,    -2,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_2000_80dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,   2,  10,  22,  38,
  52,  51,  40,  30,  28,  30,  33,  37,    37,  35,  31,  26,  20,  15,  10,   5,
   0,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_2000_100dB_SL[EHMER_MAX]={  
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,   6,  20,  40,  55,
  66,  60,  53,  40,  35,  31,  34,  42,    58,  48,  43,  45,  52,  57,  50,  42,
  45,  45,  35,  25,  20,  11,   2,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};

static float tone_4000_40dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,   2,   4,  18,
  24,  10,   5,   0,  -5,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};

static float tone_4000_60dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,  -2,   2,   4,  20,
  38,  30,  22,  15,  10,   5,   0, -10,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};

static float tone_4000_80dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,   0,   4,  10,  35,
  51,  49,  35,  35,  36,  36,  36,  31,    25,  20,  10,   0,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};

static float tone_4000_100dB_SL[EHMER_MAX]={
-900,  -5,   1,   3,   5,   8,  10,  12,    14,  20,  26,  28,  32,  36,  41,  51,
  61,  59,  45,  45,  48,  48,  40,  25,    40,  30,  18,   5,  0, -900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};


/* here the data collected by ehmer is clearly suspect as I have
   several samples that provide counterexamples.  the data below is
   derated from Ehmers original numbers */

static float tone_8000_40dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900, -10,   3,  10,  18,
  26,  21,  14,   5,   0,  0,   0,    0,    20,  35,  55,  70,  70,  70,  70,  70,
  70,  70,  70,  70,  70, 70,  70,   70,  -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_8000_60dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900, -10,  5,   12,  30,
  43,  34,  22,  15,  10,   5,   0,   0,    20,  55,  70,  70,  70,  70,  70,  70,
  70,  70,  70,  70,  70,  70,  70,  70,  -900,-900,-900,-900,-900,-900,-900,-900,
  -900,-900,-900,-900,-900,-900,-900,-900};
static float tone_8000_80dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900, -10,    -1,   2,   6,  10,  13,  19,  25,  35,
  63,  50,  35,  35,  36,  36,  36,  31,    40,  55,  70,  70,  70,  70,  70,  70,
  70,  70,  70,  70,  70,  70,  70,  70,  -900,-900,-900,-900,-900,-900,-900,-900,
 -900,-900,-900,-900,-900,-900,-900,-900};
static float tone_8000_100dB_SL[EHMER_MAX]={
 -18, -12,  -7,  -3,   0,   2,   5,   8,    10,  14,  18,  20,  20,  22,  40,  40,
  70,  50,  30,  30,  30,  30,  30,  32,    43,  55,  70,  70,  70,  70,  70,  70,
  70,  70,  70,  70,  70,  70,  70,  70,  -900,-900,-900,-900,-900,-900,-900,-900,
  -900,-900,-900,-900,-900,-900,-900,-900};

/* a complete guess, but reasonably conservative and borne out by some
   empirical testing */

static float tone_16000_40dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900, -10,   3,  10,  18,
  26,  35,  55,  70,  70,  70,  70,  70,   70,  70,  70,  70,  70, 70,  70,  70, 
-900,-900,-900,-900,-900,-900,-900,-900, -900,-900,-900,-900,-900,-900,-900,-900,
-900,-900,-900,-900,-900,-900,-900,-900};
static float tone_16000_60dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900, -10,  5,   12,  30,
  43,  55,  70,  70,  70,  70,  70,  70,    70,  70,  70,  70,  70,  70,  70,  70,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
  -900,-900,-900,-900,-900,-900,-900,-900};
static float tone_16000_80dB_SL[EHMER_MAX]={
-900,-900,-900,-900,-900,-900,-900, -10,    -1,   2,   6,  10,  13,  19,  25,  35,
  63,  55,  70,  70,  70,  70,  70,  70,    70,  70,  70,  70,  70,  70,  70,  70,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
 -900,-900,-900,-900,-900,-900,-900,-900};
static float tone_16000_100dB_SL[EHMER_MAX]={
 -18, -12,  -7,  -3,   0,   2,   5,   8,    10,  14,  18,  20,  20,  22,  40,  40,
  70,  55,  70,  70,  70,  70,  70,  70,    70,  70,  70,  70,  70,  70,  70,  70,
-900,-900,-900,-900,-900,-900,-900,-900,  -900,-900,-900,-900,-900,-900,-900,-900,
  -900,-900,-900,-900,-900,-900,-900,-900};

#endif

