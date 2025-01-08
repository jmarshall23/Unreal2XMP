#ifndef _MCDBOX_H
#define _MCDBOX_H
/* -*- mode: C; -*- */

/*
   Copyright (c) 1997-2002 MathEngine PLC

   $Name:  $

   Date: $Date: 2003/01/17 09:52:58 $ - Revision: $Revision: 1.1 $

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

/**
  @file
  The box primitive geometry type
*/

#include <McdCTypes.h>
#include <McdGeometryTypes.h>
#include <McdGeometry.h>

#ifdef __cplusplus
extern "C" {
#endif


MCD_DECLARE_GEOMETRY_TYPE(McdBox);

MEPUBLIC
McdBoxID          MEAPI McdBoxCreate(McdFramework *frame, MeReal dx, MeReal dy, MeReal dz );
MEPUBLIC
void              MEAPI McdBoxSetDimensions( McdGeometryID g,
                            MeReal dx, MeReal dy, MeReal dz );
MEPUBLIC
void              MEAPI McdBoxGetDimensions( McdGeometryID g,
                            MeReal *dx, MeReal *dy, MeReal *dz );

MEPUBLIC
MeReal*           MEAPI McdBoxGetRadii( McdBoxID );
MEPUBLIC
void              MEAPI McdBoxGetXYAABB( McdBoxID, MeMatrix4 tm,
                            MeReal bounds[4]);
MEPUBLIC
void              MEAPI McdBoxMaximumPointLocal( McdBoxID, MeReal* inDir,
                            MeReal* outPoint);


#ifdef __cplusplus
}
#endif

#endif /* _MCDBOX_H */