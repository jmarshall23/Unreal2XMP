/*=============================================================================
	ALAudioSubsystem.h: Unreal OpenAL Audio interface object.
	Copyright 1999-2001 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Daniel Vogel.
	* Ported to Linux by Ryan C. Gordon.
=============================================================================*/

#ifndef _INC_ALAUDIOSUBSYSTEM
#define _INC_ALAUDIOSUBSYSTEM

/*------------------------------------------------------------------------------------
	Dependencies.
------------------------------------------------------------------------------------*/

#include "altypes.h"
#include "alctypes.h"
typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;
#if __WIN32__
typedef ALenum (*EAXSet)(const GUID*, ALuint, ALuint, ALvoid*, ALuint);
typedef ALenum (*EAXGet)(const GUID*, ALuint, ALuint, ALvoid*, ALuint);
#else
//#include <AL/altypes.h>
//#include <AL/alctypes.h>
#endif

/*------------------------------------------------------------------------------------
	Helpers
------------------------------------------------------------------------------------*/

// Constants.

// Maximum number of audio channels.
#define MAX_AUDIOCHANNELS 32

// Maximum number of audio streams.
#define MAX_AUDIOSTREAMS 8

// Maximum sound velocity in meters/ seconds.
#define MAX_SOUND_SPEED 300.f

// Amount of OpenAL buffers per stream.
#define BUFFERS_PER_STREAM 4

// Size in bytes per buffer in stream.
#define STREAM_CHUNKSIZE 65536

// Occlusion factor.
#define OCCLUSION_FACTOR 0.35f

// Priority modifier for playing sounds.
#define PLAYING_PRIORITY_MULTIPLIER 1.0f

// Conversion factor for Unreal units -> meter
#define DISTANCE_FACTOR ( 0.0254f / 2.f )


#if __WIN32__
#define AL_DLL TEXT("OpenAL32.dll")
#define AL_DEFAULT_DLL TEXT("DefOpenAL32.dll")
#else
#define AL_DLL TEXT("libopenal.so")
#define AL_DEFAULT_DLL TEXT("openal.so")
#endif
/*------------------------------------------------------------------------------------
	UGenericAudioSubsystem.
------------------------------------------------------------------------------------*/

struct ALSource
{
	USound*		Sound;
	ALuint		Source;
	AActor*		Actor;
	FVector		Location;
	FLOAT		Priority;
	FLOAT		Radius;
	FLOAT		ZoneRadius;
	FLOAT		UsedRadius;
	FLOAT		WantedRadius;
	DOUBLE		LastChange;
	FLOAT		Volume;
	FLOAT		FadeDuration;
	FLOAT		FadeTime;
	EFadeMode	FadeMode;
	INT			Flags;
	INT			Id;
	UBOOL		Started;
	UBOOL		Paused;
};

struct ALBuffer
{
	ALuint		Buffer;
	INT			Flags;
	FString		Name;
};

struct ALStream
{
	ALuint		Buffer[BUFFERS_PER_STREAM];
	INT			Id;
	INT			Flags;
	INT			Counter;
	INT			Processed;
	INT			Rate;
	INT			Looped;
	UBOOL		Alive;
	UBOOL		Reset;
	ALuint		Format;
	void*		Data;
	FString		Name;
};

struct ALAmbient
{
	AActor*		Actor;
	FLOAT		Priority;
	DWORD		Flags;
	INT			Id;
};

//
// The Generic implementation of UAudioSubsystem.
//
class ALAUDIO_API UALAudioSubsystem : public UAudioSubsystem
{
	DECLARE_CLASS(UALAudioSubsystem,UAudioSubsystem,CLASS_Config,ALAudio)

	// Variables.
	UViewport*			Viewport;
	UViewport*			DummyViewport;
	DOUBLE				LastTime;
	DOUBLE		LastHWUpdate;
	FVector				LastPosition;
	UBOOL				LastRealtime;
	FString		PendingSong;
	UI3DL2Listener*		OldListener;
	FPointRegion		RegionListener;
	TArray<AActor*>		StaticAmbientSounds;

	// Channels.
	TArray<ALSource>	Sources;
	TArray<ALBuffer>	Buffers;
	TArray<ALStream>	Streams;

	// AL specific
	ALCdevice*			SoundDevice;
	ALCcontext*			SoundContext;
	void*		DLLHandle;

	// Configuration.
	FLOAT				DopplerFactor,
						MusicVolume,
						AmbientVolume,
						SoundVolume,
						TimeBetweenHWUpdates,
						RollOff;
	INT					MaxChannels;
	UBOOL				ReverseStereo,
						UsePrecache,
						UseEAX,
						Use3DSound,
						UseMMSYSTEM,
						UseDefaultDriver,
						DisablePitch,
						UseLowQualitySound;
    

	// Stats.
	class FALAudioStats
	{
	public:

		INT		STATS_FirstEntry,
				STATS_PlaySoundCycles,
				STATS_UpdateCycles,
				STATS_OcclusionCycles,
				STATS_PlaySoundCalls,
				STATS_OccludedSounds,
				STATS_ActiveStreamingSounds,
				STATS_ActiveRegularSounds,
				STATS_StoppedSounds,
				STATS_LastEntry;
		FALAudioStats();
		void Init();
	} ALAudioStats;

	// Constructor.
	UALAudioSubsystem();
	void StaticConstructor();

	// UObject interface.
	void Destroy();
	void PostEditChange();
	void ShutdownAfterError();
	void Serialize(FArchive& Ar);
	
	// UAudioSubsystem interface.
	UBOOL Init();
	void SetViewport( UViewport* Viewport );
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	void Update( FSceneNode* SceneNode );
	void RegisterSound( USound* Sound );
	void UnregisterSound( USound* Sound );
	UBOOL PlaySound( AActor* Actor, INT Id, USound* Sound, FVector Location, FLOAT Volume, FLOAT Radius, FLOAT Pitch, INT Flags, FLOAT FadeDuration, FLOAT Priority = 0.f );
	UBOOL StopSound( AActor* Actor, USound* Sound );

	INT PlayMusic( FString Song, FLOAT FadeInTime );
	UBOOL StopMusic( INT SongHandle, FLOAT FadeOutTime );
	INT StopAllMusic( FLOAT FadeOutTime );

	UBOOL LowQualitySound() { return UseLowQualitySound; }

	void ChangeVoiceChatter( DWORD IpAddr, DWORD ControllerPort, UBOOL Add );
	void EnterVoiceChat();
	void LeaveVoiceChat();

	void NoteDestroy( AActor* Actor );
	UViewport* GetViewport();

	// Internal functions.
	void UpdateAmbientSounds( TArray<ALAmbient>& AmbientSounds, AActor* Actor, ULevel* Level, FVector& ViewLocation );
	void SetVolumes();
	void StopSound( INT Index );
	FLOAT SoundPriority( UViewport* Viewport, FVector Location, FLOAT Volume, FLOAT Radius, INT Flags );
	INT GetNewStream();
	void SetI3DL2Listener( UI3DL2Listener* Listener );
	UBOOL alError( TCHAR* Text, UBOOL Log = true );

	// Dynamic binding.
	void FindProc( void*& ProcAddress, char* Name, char* SupportName, UBOOL& Supports, UBOOL AllowExt );
	void FindProcs( UBOOL AllowExt );
	UBOOL FindExt( const TCHAR* Name );
};

#define AUTO_INITIALIZE_REGISTRANTS_ALAUDIO	\
	UALAudioSubsystem::StaticClass();

#endif