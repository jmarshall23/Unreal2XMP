#ifndef _MDTPARTITION_H
#define _MDTPARTITION_H
/* -*- mode: C; -*- */

/*
   Copyright (c) 1997-2002 MathEngine PLC

   $Name:  $

   Date: $Date: 2003/01/17 09:55:31 $ - Revision: $Revision: 1.1 $

   This software and its accompanying manuals have been developed
   by MathEngine PLC ("MathEngine") and the copyright and all other
   intellectual property rights in them belong to MathEngine. All
   rights conferred by law (including rights under international
   copyright conventions) are reserved to MathEngine. This software
   may also incorporate information which is confidential to
   MathEngine.

   Save to the extent permitted by law, or as otherwise expressly
   permitted by MathEngine, this software and the manuals must not
   be copied (in whole or in part), re-arranged, altered or adapted
   in any way without the prior written consent of the Company. In
   addition, the information contained in the software may not be
   disseminated without the prior written consent of MathEngine.

 */
#include <MdtTypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

MEPUBLIC
void          MEAPI MdtPartInfoAddConstraint(MdtPartitionInfo* info, 
                        MdtBaseConstraint* c);

MEPUBLIC
void          MEAPI MdtPartInfoReset(MdtPartitionInfo* info);

/* MathEngine Dynamics Toolkit 'Main Loop' API */
MEPUBLIC
void          MEAPI MdtUpdatePartitions(MeDict *enabledBodyList,
                        MdtPartitionOutput* po, const MdtPartitionEndCB pcb,
                        void* pcbdata);

/* Util. */
MEPUBLIC
MdtPartitionOutput* MEAPI MdtPartOutCreateFromChunk(MeChunk* chunk,
                              int maxBodies, int maxConstraints);


#ifdef __cplusplus
}
#endif

#endif
