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

 function: build a VQ codebook decision tree 
 last mod: $Id: vqsplit.h,v 1.1 2003/01/17 11:22:37 demiurge Exp $

 ********************************************************************/

#ifndef _VQSPL_H_
#define _VQSPL_H_

#include "codebook.h"

extern void vqsp_book(vqgen *v,codebook *b,long *quantlist);
extern int vqenc_entry(codebook *b,float *val);
extern int lp_split(float *pointlist,long totalpoints,
		    codebook *b,
		    long *entryindex,long entries, 
		    long *pointindex,long points,
		    long *membership,long *reventry,
		    long depth, long *pointsofar);

#endif





