/*=============================================================================
	UnVoiceChat.h: Unreal Voice Chat support header.
	Copyright 2002 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Daniel Vogel
=============================================================================*/

struct FVoiceChatterInfo
{
	class AController*	Controller;
	DWORD				IpAddr;
	INT					Handle;
};

