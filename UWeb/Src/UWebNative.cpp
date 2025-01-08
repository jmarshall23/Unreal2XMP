/*=============================================================================
	UWebNative.cpp: Native function lookup table for static libraries.
	Copyright 2000 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Brandon Reinhart
=============================================================================*/

#include "UWeb.h"
#include "UWebNative.h"

#if __STATIC_LINK
UWebResponseNativeInfo GUWebUWebResponseNatives[] =
{
	MAP_NATIVE(UWebResponse, execIncludeBinaryFile)
	MAP_NATIVE(UWebResponse, execIncludeUHTM)
	MAP_NATIVE(UWebResponse, execClearSubst)
	MAP_NATIVE(UWebResponse, execSubst)
	{NULL, NULL}
};
IMPLEMENT_NATIVE_HANDLER(UWeb,UWebResponse);

UWebRequestNativeInfo GUWebUWebRequestNatives[] =
{
	MAP_NATIVE(UWebRequest, execGetVariableNumber)
	MAP_NATIVE(UWebRequest, execGetVariableCount)
	MAP_NATIVE(UWebRequest, execGetVariable)
	MAP_NATIVE(UWebRequest, execAddVariable)
	MAP_NATIVE(UWebRequest, execDecodeBase64)
	{NULL, NULL}
};
IMPLEMENT_NATIVE_HANDLER(UWeb,UWebRequest);
#endif