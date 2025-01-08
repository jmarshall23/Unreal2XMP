/*=============================================================================
	UnRenderResource.cpp: Render resource implementation.
	Copyright 1997-2000 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Andrew Scheidecker
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(URenderResource);
IMPLEMENT_CLASS(UVertexStreamBase);
IMPLEMENT_CLASS(UVertexStreamVECTOR);
IMPLEMENT_CLASS(UVertexStreamCOLOR);
IMPLEMENT_CLASS(UVertexStreamUV);
IMPLEMENT_CLASS(UVertexStreamPosNormTex);
IMPLEMENT_CLASS(UVertexBuffer);
IMPLEMENT_CLASS(UIndexBuffer);
IMPLEMENT_CLASS(USkinVertexBuffer);

//	URenderResource
void URenderResource::Serialize(FArchive& Ar)
{
	guard(URenderResource::Serialize);

	Super::Serialize(Ar);

	Ar << Revision;

	unguard;
}


// UIndexBuffer
void UIndexBuffer::Serialize(FArchive& Ar)
{
	guard(UIndexBuffer::Serialize);

	Super::Serialize(Ar);

	Ar << Indices;

	unguard;
}


// USkinVertexBuffer
void USkinVertexBuffer::Serialize(FArchive& Ar)
{
	guard(UIndexBuffer::Serialize);

	Super::Serialize(Ar);

	Ar << Vertices;

	unguard;
}


FShader::FShader(UHardwareShader* InShader, INT InPass) :
	Shader(InShader),
	Pass(InPass),
	Revision(1)
{
	CacheId = MakeCacheID(CID_RenderShader);
}


FShader::~FShader()
{

}


QWORD FShader::GetCacheId()
{
	return CacheId;
}


INT FShader::GetRevision()
{
	return Revision;
}

void FShader::IncRevision()
{
	Revision++;
}


FPixelShader::FPixelShader(UHardwareShader* InShader, INT InPass) :
	FShader(InShader, InPass)
{

}


FPixelShader::~FPixelShader()
{

}


FVertexShader::FVertexShader(UHardwareShader* InShader, INT InPass) :
	FShader(InShader, InPass)
{

}


FVertexShader::~FVertexShader()
{

}

