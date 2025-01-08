/*=============================================================================
	ALAudioSubsystem.cpp: Unreal OpenAL Audio interface object.
	Copyright 1999-2001 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Daniel Vogel.
	* Ported to Linux by Ryan C. Gordon.
=============================================================================*/

/*------------------------------------------------------------------------------------
	Audio includes.
------------------------------------------------------------------------------------*/

#include "ALAudioPrivate.h"
#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,8)
#endif
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif

// OpenAL functions.
#define DYNAMIC_BIND 1
#define AL_EXT(name) static UBOOL SUPPORTS##name;
#define AL_PROC(ext,ret,func,parms) static ret (CDECL *func)parms;
#include "ALFuncs.h"
#define INITGUID 1
#undef DEFINE_GUID
#define OPENAL

#if __WIN32__
#define __EAX__ 1
#endif

#if __EAX__
//!!WARNING: relies on modified eax.h
#include <eax.h>
EAXGet alEAXGet;
EAXSet alEAXSet;
#else
void *alEAXGet = NULL;
void *alEAXSet = NULL;
#endif

#undef AL_EXT
#undef AL_PROC

static UBOOL bMusicLocked=0;

/*------------------------------------------------------------------------------------
	UALAudioSubsystem.
------------------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UALAudioSubsystem);

//
// UALAudioSubsystem::UALAudioSubsystem
//
UALAudioSubsystem::UALAudioSubsystem()
{
	guard(UALAudioSubsystem::UALAudioSubsystem);
	LastTime	 = 0;
	LastHWUpdate = 0;
	LastPosition = FVector(0,0,0);
	PendingSong	 = TEXT("");
	unguard;
}

//
// UALAudioSubsystem::StaticConstructor
//
void UALAudioSubsystem::StaticConstructor()
{
	guard(UALAudioSubsystem::StaticConstructor);
	new(GetClass(),TEXT("Use3DSound"),		RF_Public)UBoolProperty  (CPP_PROPERTY(Use3DSound      ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("UseEAX"),				RF_Public)UBoolProperty  (CPP_PROPERTY(UseEAX          ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("CompatibilityMode"),	RF_Public)UBoolProperty  (CPP_PROPERTY(UseMMSYSTEM     ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("UsePrecache"),			RF_Public)UBoolProperty  (CPP_PROPERTY(UsePrecache     ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("ReverseStereo"),		RF_Public)UBoolProperty  (CPP_PROPERTY(ReverseStereo   ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("UseDefaultDriver"),RF_Public)UBoolProperty  (CPP_PROPERTY(UseDefaultDriver), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("Channels"),			RF_Public)UIntProperty   (CPP_PROPERTY(MaxChannels     ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("MusicVolume"),			RF_Public)UFloatProperty (CPP_PROPERTY(MusicVolume     ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("SoundVolume"),			RF_Public)UFloatProperty (CPP_PROPERTY(SoundVolume     ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("AmbientVolume"),   RF_Public)UFloatProperty (CPP_PROPERTY(AmbientVolume   ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("DopplerFactor"),		RF_Public)UFloatProperty (CPP_PROPERTY(DopplerFactor   ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("RollOff"),				RF_Public)UFloatProperty (CPP_PROPERTY(RollOff		   ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("TimeBetweenHWUpdates"),RF_Public)UFloatProperty (CPP_PROPERTY(TimeBetweenHWUpdates), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("DisablePitch"),    RF_Public)UBoolProperty  (CPP_PROPERTY(DisablePitch    ), TEXT("ALAudio"), CPF_Config );
	new(GetClass(),TEXT("LowQualitySound"), RF_Public)UBoolProperty  (CPP_PROPERTY(UseLowQualitySound ), TEXT("ALAudio"), CPF_Config );
	unguard;
}


/*------------------------------------------------------------------------------------
	UObject Interface.
------------------------------------------------------------------------------------*/

//
// UALAudioSubsystem::PostEditChange
//
void UALAudioSubsystem::PostEditChange()
{
	guard(UALAudioSubsystem::PostEditChange);

	// Validate configurable variables.
	SoundVolume = Clamp(SoundVolume,0.f,1.f);
	MusicVolume = Clamp(MusicVolume,0.f,1.f);

	SetVolumes();

	unguard;
}

//
// UALAudioSubsystem::Destroy
//
void UALAudioSubsystem::Destroy()
{
	guard(UALAudioSubsystem::Destroy);
	if( USound::Audio )
	{
		// Unhook.
		USound::Audio = NULL;

		for (INT i=0; i<Sources.Num(); i++)
		{
			StopSound(i);
			alDeleteSources( 1, & Sources(i).Source );
		}
		Sources.Empty();

		if (Viewport)
		{
			for(TObjectIterator<USound> SoundIt;SoundIt;++SoundIt)
				UnregisterSound(*SoundIt);
		}

		for (INT i=0; i<Buffers.Num(); i++ )
			alDeleteBuffers( 1, &Buffers(i).Buffer );
		Buffers.Empty();

		for (INT i=0; i<Streams.Num(); i++ )
			alDeleteBuffers( BUFFERS_PER_STREAM, &Streams(i).Buffer[0] );
		Streams.Empty();

		alcProcessContext( SoundContext );
		alcMakeContextCurrent( NULL );
		alcDestroyContext( SoundContext );
		alcCloseDevice( SoundDevice );

		SetViewport(NULL);
		
		appFreeDllHandle( DLLHandle );

		debugf(NAME_Exit,TEXT("OpenAL Audio subsystem shut down."));
	}
	Super::Destroy();
	unguard;
}

//
// UALAudioSubsystem::ShutdownAfterError
//
void UALAudioSubsystem::ShutdownAfterError()
{
	guard(UALAudioSubsystem::ShutdownAfterError);

	if( USound::Audio )
	{
		// Unhook.
		USound::Audio = NULL;
		
		alcProcessContext( SoundContext );
		alcMakeContextCurrent( NULL );
		alcDestroyContext( SoundContext );
		alcCloseDevice( SoundDevice );

		appFreeDllHandle( DLLHandle );

		debugf(NAME_Exit,TEXT("UALAudioSubsystem::ShutdownAfterError"));
	}

	Super::ShutdownAfterError();

	unguard;
}

void UALAudioSubsystem::Serialize( FArchive& Ar )
{
	guard(UALAudioSubsystem::Serialize);
	Super::Serialize(Ar);
	if( !Ar.IsLoading() && !Ar.IsSaving() )
	{
		for( INT i=0; i<Sources.Num(); i++ )
			Ar << Sources(i).Sound;
		for( INT i=0; i<StaticAmbientSounds.Num(); i++ )
			Ar << StaticAmbientSounds(i);
	}
	unguard;
}


/*------------------------------------------------------------------------------------
	UAudioSubsystem Interface.
------------------------------------------------------------------------------------*/

//
// UALAudioSubsystem::Init
//
UBOOL UALAudioSubsystem::Init()
{
	guard(UALAudioSubsystem::Init);

	if( USound::Audio )
		return 1;

#if DYNAMIC_BIND
	guard(DynamicBinding);
	// Find DLL's.
	DLLHandle = NULL;
	if( !UseDefaultDriver )
	{
		DLLHandle = appGetDllHandle( AL_DLL );
		if( !DLLHandle )
			debugf( NAME_Init, TEXT("Couldn't locate %s - trying default next."), AL_DLL );
	}
	if( !DLLHandle )
	{
		DLLHandle = appGetDllHandle( AL_DEFAULT_DLL );
		if( !DLLHandle )
		{
			debugf( NAME_Init, TEXT("Couldn't locate %s - giving up."), AL_DEFAULT_DLL );
			return 0;
		}
	}
	
	// Find functions.
	SUPPORTS_AL = 1;
	FindProcs( 0 );
	if( !SUPPORTS_AL )
		return 0;
	unguard;
#endif

	if( UseEAX )
		Use3DSound = true;

	char *Device = NULL;
	#if __WIN32__
		Device = UseMMSYSTEM ? "MMSYSTEM" : Use3DSound ? "DirectSound3D" : "DirectSound";
	#endif
	guard(alcOpenDevice);
	SoundDevice = alcOpenDevice( (unsigned char *)Device );
	unguard;
	if (!SoundDevice)
	{
		debugf(NAME_Init,TEXT("ALAudio: no OpenAL devices found."));
		return 0;
	}

	INT Caps[] = { ALC_FREQUENCY, 44100, 0 };
	guard(alcCreateContext);
	SoundContext = alcCreateContext( SoundDevice, Caps );
	unguard;
	if (!SoundContext)
	{
		debugf(NAME_Init, TEXT("ALAudio: context creation failed."));
		return 0;
	}

	guard(alcMakeContextCurrent);
	alcMakeContextCurrent( SoundContext );
	unguard;
	//alcProcessContext( SoundContext );
	
	if( alError(TEXT("Init")) )
	{
		debugf(NAME_Init,TEXT("ALAudio: makecurrent failed."));
		return 0;
	}

	if( !MaxChannels )
		return 0;

	// Initialize channels.
	guard(InitializingChannels);
	Sources.Empty();
	for (INT i=0; i<Min(MaxChannels, MAX_AUDIOCHANNELS); i++)
	{
		ALuint sid;
		alGenSources( 1, &sid );
		if ( !alError(TEXT("Init (creating sources)"),false) )
		{
			Sources.AddZeroed(1);
			Sources(i).Source = sid;
			// Initialize rolloff so DS3D wrapper can set global rolloff. (HACK)
			alSourcef( sid, AL_ROLLOFF_FACTOR, RollOff );
		}
		else
			break;
	}
	if( !Sources.Num() )
	{
		debugf(NAME_Error,TEXT("ALAudio: couldn't allocate sources"));
		return 0;
	}
	unguard;

	// Initialize streams.
	Streams.Empty();
	Streams.AddZeroed( MAX_AUDIOSTREAMS );

	// Adjust global rolloff factor.
	if( RollOff <= 0.0f )
		RollOff = 1.0f;
	
	// Use DS3D distance model.
	alDistanceModel( AL_INVERSE_DISTANCE_CLAMPED );
	alDopplerFactor( DopplerFactor );

	// Check for EAX support.
	guard(CheckingForEAX);
	OldListener = NULL;
	alEAXGet	= NULL;
	alEAXSet	= NULL;
	UBOOL SupportsEAX = alIsExtensionPresent((ALubyte*)"EAX3.0") == AL_TRUE; 
#if __EAX__
	if( UseEAX && SupportsEAX )
	{
		alEAXSet	= (EAXSet) alGetProcAddress((ALubyte*)"EAXSet");
		alEAXGet	= (EAXGet) alGetProcAddress((ALubyte*)"EAXGet");

		if( alEAXSet && alEAXGet )
		{
			// Set 'plain' EAX preset.
			ALuint uEnvironment = EAX_ENVIRONMENT_PLAIN;
			alEAXSet(&DSPROPSETID_EAX30_ListenerProperties,DSPROPERTY_EAXLISTENER_ENVIRONMENT,NULL, &uEnvironment, sizeof(ALuint));
			debugf(NAME_Init,TEXT("ALAudio: using EAX"));
		}
	}
#endif
	unguard;

	LastRealtime		= 0;	

	// Initialize stats.
	ALAudioStats.Init();

	// Initialized.
	USound::Audio = this;

	debugf(NAME_Init,TEXT("ALAudio: subsystem initialized."));
	return 1;

	unguard;
}

//
// UALAudioSubsystem::SetViewport
//
void UALAudioSubsystem::SetViewport(UViewport* InViewport)
{
	guard(UALAudioSubsystem::SetViewport);

	// Stop playing sounds.
	for(INT i=0; i<Sources.Num(); i++)
		if( Sources(i).Started )
//		if( Sources(i).Started && !(Sources(i).Flags & SF_Music) )
			StopSound(i);

	// Precache static ambient sounds.
	StaticAmbientSounds.Empty();
	if( !GIsEditor && InViewport && InViewport->Actor && InViewport->Actor->GetViewTarget() )
	{
		ULevel* Level = InViewport->Actor->GetViewTarget()->GetLevel();
		if( Level )
		{
			check(Level->iFirstDynamicActor<=Level->Actors.Num());
			for( INT i=0; i<Level->iFirstDynamicActor; i++ )
			{
				AActor* Actor = Level->Actors(i);
				
				if( !Actor || !Actor->AmbientSound )
				    continue;
				    
				StaticAmbientSounds.AddItem( Level->Actors(i) );
			}
		}
	}

	if( Viewport != InViewport )
	{
		// Switch viewports.
		Viewport = InViewport;

		// Set listener region to 0.
		appMemzero( &RegionListener, sizeof(RegionListener) );

		if( Viewport && UsePrecache )
		{
			// Register everything.
			for(TObjectIterator<USound> SoundIt;SoundIt;++SoundIt)
			{				
				// Don't precache temporary music objects.
				if( !(SoundIt->GetFlags() & SF_Music) )    
					RegisterSound(*SoundIt);
			}
		}
	}

	if( Viewport && Viewport->Actor && Viewport->Actor->Song!=TEXT("") )
		PlayMusic( Viewport->Actor->Song, 3.0f );

	unguard;
}

//
// UALAudioSubsystem::GetViewport
//
UViewport* UALAudioSubsystem::GetViewport()
{
	guard(UALAudioSubsystem::GetViewport);
	return Viewport;
	unguard;
}

//
// UALAudioSubsystem::RegisterSound
//
void UALAudioSubsystem::RegisterSound(USound* Sound)
{
	guard(UALAudioSubsystem::RegisterSound);

	checkSlow(Sound);

	if( !Sound->Handle )
	{
		// Avoid recursion as USound->Load calls RegisterSound.
		Sound->Handle = 0xDEADBEEF;

		if ( Sound->Flags & SF_Streaming )
		{
			BYTE* Data = new BYTE[STREAM_CHUNKSIZE*BUFFERS_PER_STREAM];
			OggVorbis_File* OggFile = new OggVorbis_File; //!! delete somewhere.
			
			EFileStreamType Type = (Sound->Flags & SF_Music) ? ST_OggLooping : ST_Ogg;
			INT Id = GFileStream->CreateStream( *Sound->Filename, STREAM_CHUNKSIZE, BUFFERS_PER_STREAM, Data, Type, OggFile );
			if ( Id < 0 )
			{
				debugf( NAME_Error, TEXT("Failed to register sound: %s"), *Sound->Filename );
				Sound->Handle = 0;
				return;
			}

			if ( ov_pcm_total(OggFile, -1) < STREAM_CHUNKSIZE * BUFFERS_PER_STREAM )
				debugf( NAME_Error, TEXT("Sound is too short for streaming: %s"), *Sound->Filename );
			
			vorbis_info *VorbisInfo = ov_info(OggFile, -1);

			Sound->Duration		= ov_time_total(OggFile, -1);
			Sound->Handle		= GetNewStream() + 1;
			ALStream& Stream	= Streams(Sound->Handle-1);
			Stream.Flags		= Sound->Flags;
			Stream.Counter		= 0;
			Stream.Id			= Id+1;
			Stream.Processed	= 0;
			Stream.Alive		= 1;
			Stream.Reset		= 0;
			Stream.Rate			= VorbisInfo->rate;
			Stream.Name			= Sound->GetPathName();
			Stream.Data			= new BYTE[STREAM_CHUNKSIZE];

			alGenBuffers( BUFFERS_PER_STREAM, &Stream.Buffer[0] );
			alError(TEXT("Creating streaming buffer."));

			switch ( VorbisInfo->channels )
			{
			case 1:
				Stream.Format = AL_FORMAT_MONO16;
				break;
			case 2:
				Stream.Format = AL_FORMAT_STEREO16;				
				break;
			default:
				Stream.Format = 0;
				appErrorf(TEXT("Invalid sound data"));
				break;
			}

			for (INT i=0; i<BUFFERS_PER_STREAM; i++)
				alBufferData( Stream.Buffer[i], Stream.Format, Data + i * STREAM_CHUNKSIZE, STREAM_CHUNKSIZE, Stream.Rate );

			// The initial request.
			GFileStream->RequestChunks( Stream.Id-1, 1, Stream.Data );

			// error checking!!
			delete Data;			
		}
		else
		{
			// debugf( NAME_DevSound, TEXT("Register sound: %s"), Sound->GetPathName() );

			// Load the data.
			Sound->Data.Load();
			check(Sound->Data.Num()>0);
	
			FWaveModInfo WaveInfo;
			if( !WaveInfo.ReadWaveInfo(Sound->Data) )
			{
				debugf( NAME_Warning, TEXT("%s is not a WAVE file?"), Sound->GetPathName() );
				Sound->Data.Unload();
				Sound->Handle = 0;
				return;
			}

			INT Flags = WaveInfo.SampleLoopsNum ? SF_Looping : 0;

			ALint Format;
			if ( *WaveInfo.pBitsPerSample == 8 )
			{
				//debugf( NAME_Warning, TEXT("WARNING: 8 bit sound detected [%s]"), Sound->GetPathName() );
				if (*WaveInfo.pChannels == 1)
					Format = AL_FORMAT_MONO8;
				else
					Format = AL_FORMAT_STEREO8;
			}
			else
			{
				if (*WaveInfo.pChannels == 1)
					Format = AL_FORMAT_MONO16;
				else
					Format = AL_FORMAT_STEREO16;				
			}

			ALuint bid;
			alGenBuffers( 1, &bid );
			alError(TEXT("RegisterSound (generating buffer)"));
		
			alBufferData( bid, Format, WaveInfo.SampleDataStart, WaveInfo.SampleDataSize, *WaveInfo.pSamplesPerSec );
	
			// Unload the data.
			Sound->Data.Unload();

			if (alError(TEXT("RegisterSound (creating buffer)")))
				Sound->Handle = 0;
			else
			{
				Sound->Handle = Buffers.AddZeroed( 1 ) + 1;
				ALBuffer& Buffer = Buffers(Sound->Handle-1);
				Buffer.Buffer = bid;
				Buffer.Flags  = Flags;
				Buffer.Name	= Sound->GetPathName();
			}
		}
	} 
	unguard;
}

//
// UALAudioSubsystem::UnregisterSound
//
void UALAudioSubsystem::UnregisterSound(USound* Sound)
{
	guard(UALAudioSubsystem::UnregisterSound);

	for( INT i=0; i<Sources.Num(); i++ )
		if( Sources(i).Sound == Sound )
			StopSound(i);

	alError(TEXT(""),false);
	if( Sound->Handle )
	{
		debugf( NAME_DevSound, TEXT("Unregister sound: %s"), Sound->GetPathName() );
		INT i = Sound->Handle - 1;
		if ( Sound->Flags & SF_Streaming )
		{
			alDeleteBuffers( BUFFERS_PER_STREAM, &Streams(i).Buffer[0] );
			appMemzero( Streams(i).Buffer, sizeof(Streams(i).Buffer) );
			delete [] Streams(i).Data;
			appMemzero( &Streams(i), sizeof(ALStream) );		
		}
		else
		{
			alDeleteBuffers( 1, &Buffers(i).Buffer );
			Buffers(i).Buffer = 0;
			Buffers(i).Flags  = 0;
		}
		Sound->Handle = 0;
	}

	unguard;
}

//
// UALAudioSubsystem::PlaySound
//
UBOOL UALAudioSubsystem::PlaySound( AActor* Actor, INT Id, USound* Sound, FVector Location, FLOAT Volume, FLOAT Radius, FLOAT Pitch, INT Flags, FLOAT FadeDuration, FLOAT InPriority )
{
	guard(UALAudioSubsystem::PlaySound);

	static INT FreeSlot;
	check(Radius);
    check( this );

	if( !Viewport || !Sound)
		return 0;
	if( !Sound->Handle )
		RegisterSound(Sound);
	if( !Sound->Handle )
		return 0;

	clock(GStats.DWORDStats(ALAudioStats.STATS_PlaySoundCycles));
	GStats.DWORDStats(ALAudioStats.STATS_PlaySoundCalls)++;

	// Global volume.
	if( !(Flags & SF_Music) )
		Volume *= SoundVolume;

	Pitch  = Clamp<FLOAT>(Pitch,0.5f,2.0f);
	Volume = Clamp<FLOAT>(Volume,0.0f,1.0f);

	if( Volume == 0.f )
		return 0;

	// Root motion.
#if MERGEHACK_ERIK
	if( (Flags & SF_RootMotion) && Actor )
		Location = Actor->GetRootLocation();
#endif

	// Compute priority.
	FLOAT Priority;
	
	if( InPriority && ((Id & 14) == SLOT_Ambient * 2) )
	{
		// Ambientsounds have their priority calculated in Update.
		Priority = InPriority;
	}
	else
	{
		// Calculate occlusion before priority.
		FLOAT OcclusionRadius = Radius;
		if( !(Flags & SF_No3D) && Actor && Actor->GetLevel() )
		{
			guard(SoundOcclusion);
			clock(GStats.DWORDStats(ALAudioStats.STATS_OcclusionCycles));
			if( !Actor->GetLevel()->IsAudibleAt( Location, LastPosition, Actor, (ESoundOcclusion) Actor->SoundOcclusion ) )
			{
				FPointRegion RegionSource	= Actor->GetLevel()->Model->PointRegion( Actor->Level, Location );
				OcclusionRadius = Radius * Actor->GetLevel()->CalculateRadiusMultiplier( RegionSource.ZoneNumber, RegionListener.ZoneNumber );
				OcclusionRadius *= OCCLUSION_FACTOR;
			}
			unclock(GStats.DWORDStats(ALAudioStats.STATS_OcclusionCycles));
			unguard;
		}
		Priority = SoundPriority( Viewport, Location, Volume, OcclusionRadius, Flags );
	}
	
	FLOAT BestPriority	= Priority;

	// Allocate a new slot if requested.
	if( (Id&14)==2*SLOT_None )
		Id = 16 * --FreeSlot;

	INT Index = -1;

	guard(PlaySound::FindingVoice);
	// Find a voice to play the sound in.
	for(INT i=0; i<Sources.Num(); i++ )
	{
		if( (Sources(i).Id&~1)==(Id&~1) )
		{
			// Skip if not interruptable.
			if( Id&1 )
				return 0;

			// Stop the sound.
			Index = i;
			break;
		}
		else if( (Sources(i).Priority * PLAYING_PRIORITY_MULTIPLIER ) < BestPriority )
		{
			Index = i;
			BestPriority = Sources(i).Priority;
		}
	}
    unguard;

	// If no sound, or its priority is overruled, don't play it.
	if( Index==-1 )
	{
		unclock(GStats.DWORDStats(ALAudioStats.STATS_PlaySoundCycles));
		return 0;
	}

	StopSound(Index);
	
	// Set default values.
    check( Index < Sources.Num() );
	Sources(Index).ZoneRadius	= Radius;
	Sources(Index).UsedRadius	= Max( Radius, 1.f );
	Sources(Index).WantedRadius = Radius;

	guard(PlaySound::CalcRadius);
	// Calculate initial radius.
	if( !(Flags & SF_No3D) && Actor && Actor->GetLevel() )
	{
		guard(SoundOcclusion);
		clock(GStats.DWORDStats(ALAudioStats.STATS_OcclusionCycles));
		if( !Actor->GetLevel()->IsAudibleAt( Location, LastPosition, Actor, (ESoundOcclusion) Actor->SoundOcclusion ) )
		{
			FPointRegion RegionSource	= Actor->GetLevel()->Model->PointRegion( Actor->Level, Location );
			FLOAT TempRadius = Radius * Actor->GetLevel()->CalculateRadiusMultiplier( RegionSource.ZoneNumber, RegionListener.ZoneNumber );
			Sources(Index).ZoneRadius	= TempRadius;
			TempRadius *= OCCLUSION_FACTOR;
			Sources(Index).UsedRadius	= TempRadius;
			Sources(Index).WantedRadius	= TempRadius;
		}
		unclock(GStats.DWORDStats(ALAudioStats.STATS_OcclusionCycles));
		unguard;
	}
    unguard;

	// Set up the voice.
	Sources(Index).Sound		= Sound;
	Sources(Index).Id			= Id;
	Sources(Index).Actor		= Actor;
	Sources(Index).Priority		= Priority;
	Sources(Index).Location		= (Flags & SF_No3D) ? FVector(0,0,0) : Location;
	Sources(Index).Radius		= Radius ? Radius : 10;
	Sources(Index).LastChange	= Viewport->CurrentTime;
	Sources(Index).FadeDuration = FadeDuration;
	Sources(Index).FadeTime		= 0.f;
	Sources(Index).FadeMode		= FadeDuration > 0.f ? FADE_In : FADE_None;
	Sources(Index).Volume		= Volume;
	Sources(Index).Started		= 1;
	Sources(Index).Paused		= 0;
	if (Flags & SF_Streaming)
		Sources(Index).Flags	= Flags | Streams( Sound->Handle-1 ).Flags;
	else
		Sources(Index).Flags	= Flags | Buffers( Sound->Handle-1 ).Flags;
	
	ALuint sid = Sources(Index).Source;

	if( alEAXSet == NULL )
	{
		alSourcef( sid, AL_REFERENCE_DISTANCE, Sources(Index).UsedRadius * DISTANCE_FACTOR );
	}
	else
	{
		#if __EAX__
		long Occlusion = Flags & SF_No3D ? 0 : (long) (868.589f * log( Max( Sources(Index).UsedRadius / Radius, 0.00001f) ) ) ;
			alEAXSet(&DSPROPSETID_EAX30_BufferProperties, DSPROPERTY_EAXBUFFER_OCCLUSION, sid, &Occlusion, sizeof(Occlusion));
			//alEAXSet(&DSPROPSETID_EAX30_BufferProperties, DSPROPERTY_EAXBUFFER_DIRECT, sid, &Occlusion, sizeof(Occlusion));
			alSourcef( sid, AL_REFERENCE_DISTANCE, Sources(Index).UsedRadius * DISTANCE_FACTOR );
		#endif
	}
	alSourcef( sid, AL_GAIN, Clamp( (Sources(Index).FadeMode == FADE_In ? 0.f : Sources(Index).Volume), 0.0f, 1.0f));
	if (!DisablePitch)
		alSourcef( sid, AL_PITCH, Pitch );
	alSourcei( sid, AL_LOOPING, (Sources(Index).Flags & SF_Looping) ? AL_TRUE : AL_FALSE );

	FVector Location =  Sources(Index).Location * DISTANCE_FACTOR;
	Location.Z *= -1;
	alSourcefv( sid, AL_POSITION, (ALfloat*) &Location ); 

	FVector Velocity = Actor ? Actor->Velocity : FVector(0,0,0);
	if( (Flags & SF_No3D) || (Velocity.Size() > MAX_SOUND_SPEED) )
		Velocity = FVector(0,0,0);
	else
		Velocity *= DISTANCE_FACTOR;
	alSourcefv( sid, AL_VELOCITY, (ALfloat *) &Velocity );

	//!!WARNING: DS3D doesn't support per source rolloff factor!
	alSourcef( sid, AL_ROLLOFF_FACTOR, RollOff );

	if( Flags & SF_No3D )
		alSourcei( sid, AL_SOURCE_RELATIVE, AL_TRUE );
	else
		alSourcei( sid, AL_SOURCE_RELATIVE, AL_FALSE );

	if( Flags & SF_Streaming )
		alSourceQueueBuffers( sid, BUFFERS_PER_STREAM, &Streams( Sound->Handle-1 ).Buffer[0] );
	else
		alSourcei( sid, AL_BUFFER, Buffers( Sound->Handle-1 ).Buffer );

	alSourcePlay( sid );

	//debugf(TEXT("playing sound in source %i with priority %f"), Index, Priority);

	unclock(GStats.DWORDStats(ALAudioStats.STATS_PlaySoundCycles));
	return 1;

	unguard;
}


UBOOL UALAudioSubsystem::StopSound( AActor* Actor, USound* Sound )
{
	UBOOL Stopped = false;

	// Stop all sounds.
	if ( !Actor && !Sound )
	{
		for (INT i=0; i<Sources.Num(); i++)	
		{
		    if( !(Sources(i).Flags & SF_Music) )
		        StopSound(i);
		}
		return true;
	}

	// Stop selected sound.
	for (INT i=0; i<Sources.Num(); i++)	
	{
		if ( Sound )
		{
			if ( Actor )
			{
				if ( (Actor == Sources(i).Actor) && (Sound == Sources(i).Sound) )
				{
					StopSound(i);
					Stopped = true;
				}
			}
			else
			{
				if ( Sound == Sources(i).Sound )
				{
					StopSound(i);
					Stopped = true;
				}
			}
		}
		else if ( Actor )
		{
			if ( Actor == Sources(i).Actor )
			{
				StopSound(i);
				Stopped = true;
			}
		}
	}
	return Stopped;
}
	

void UALAudioSubsystem::NoteDestroy(AActor* Actor)
{
	guard(UALAudioSubsystem::NoteDestroy);
	check(Actor);
	check(Actor->IsValid());

	// Stop the actor's sound, and dereference owned sounds.
	for(INT i=0; i<Sources.Num(); i++)
	{
		if(Sources(i).Actor == Actor)
		{
			if((Sources(i).Id & 14) == SLOT_Ambient * 2)
				StopSound(i);
			else 
				Sources(i).Actor = NULL; // Not interruptable sound.
		}
	}

	unguard;
}


INT UALAudioSubsystem::PlayMusic( FString Song, FLOAT FadeInTime )
{
	guard(UALAudioSubsystem::PlayMusic);

	if (bMusicLocked)
		return 0;
	
	// Start music.
	if ( Song != TEXT("") )
	{
		FLOAT Volume = Min(Viewport->Actor->Level->MusicVolumeOverride,1.f);
		if( Volume < 0 )
			Volume = MusicVolume;

		if( Volume == 0.f )
		{
			PendingSong = Song;
			return 0;
		}
		else
		{
			PendingSong = TEXT("");
			FString Filename = FString(TEXT("..\\Music\\")) + Song + FString(TEXT(".ogg"));
			USound* Music = new USound( *Filename, SF_Streaming | SF_Music );
			RegisterSound( Music );

			PlaySound( NULL, 2*SLOT_None | 1, Music, FVector(0,0,0), Clamp(Volume,0.f,1.f), 1000, 1.f, SF_Streaming | SF_No3D | SF_Music, FadeInTime );
	
			for( INT i=0; i<Sources.Num(); i++ )
				if( Sources(i).Sound == Music )
					return i+1;
			//!!TODO: warning message.
		}
	}
	else
		PendingSong = TEXT("");

	return 0;
	unguard;
}


UBOOL UALAudioSubsystem::StopMusic( INT SongHandle, FLOAT FadeOutTime )
{
	guard(UALAudioSubsystem::StopMusic);
	
	// Stop music.
	SongHandle--;
	if( (SongHandle < Sources.Num()) && (SongHandle >= 0) && Sources(SongHandle).Sound )
	{
		if( FadeOutTime > 0.f )
		{
			Sources(SongHandle).FadeTime		= 0.f;
			Sources(SongHandle).FadeDuration	= FadeOutTime;
			Sources(SongHandle).FadeMode		= FADE_Out;
		}
		else
			UnregisterSound( Sources(SongHandle).Sound );
		return 1;
	}
	else
		return 0;
	unguard;
}


INT UALAudioSubsystem::StopAllMusic( FLOAT FadeOutTime )
{
	guard(UALAudioSubsystem::StopAllMusic);
	
	// Stop all music.
	for (INT i=0; i<Sources.Num(); i++)
		if ( Sources(i).Flags & SF_Music )
				StopMusic(i+1, FadeOutTime);
	return 0;
	unguard;
}

void UALAudioSubsystem::UpdateAmbientSounds( TArray<ALAmbient>& AmbientSounds, AActor* Actor, ULevel* Level, FVector& ViewLocation )
{
	guard(UALAudioSubsystem::UpdateAmbientSounds);
	INT Id = Actor->GetIndex()*16 + SLOT_Ambient*2;
	INT j;
	for( j=0; j<Sources.Num(); j++ )
		if( Sources(j).Id==Id )
			break;
	if( j==Sources.Num() )
	{
		if( Actor->IsOwnedBy( Viewport->Actor->ViewTarget ) )
		{
			INT iAmbient = AmbientSounds.Add( 1 );
			AmbientSounds(iAmbient).Actor		= Actor;
			AmbientSounds(iAmbient).Priority	= SoundPriority( Viewport, Actor->Location, Actor->GetAmbientVolume(AmbientVolume), Actor->SoundRadius, SF_Looping | SF_No3D | SF_UpdatePitch );
			AmbientSounds(iAmbient).Flags		= SF_Looping | SF_No3D | SF_UpdatePitch;
			AmbientSounds(iAmbient).Id			= Id;
		}
		else
		{
			// Sound occclusion.
			clock(GStats.DWORDStats(ALAudioStats.STATS_OcclusionCycles));
			FLOAT RM = 1.f;
			if( !Level->IsAudibleAt( Actor->Location, ViewLocation, Actor, (ESoundOcclusion) Actor->SoundOcclusion ) )
			{
				FPointRegion RegionSource	= Level->Model->PointRegion( Actor->Level, Actor->Location );
				RM *= Level->CalculateRadiusMultiplier( RegionSource.ZoneNumber, RegionListener.ZoneNumber );
			}
			unclock(GStats.DWORDStats(ALAudioStats.STATS_OcclusionCycles));
		
			if (FDistSquared(ViewLocation,Actor->Location)<=Square(RM*Actor->SoundRadius*GAudioMaxRadiusMultiplier) )
			{
				INT iAmbient = AmbientSounds.Add( 1 );
				AmbientSounds(iAmbient).Actor		= Actor;
				AmbientSounds(iAmbient).Priority	= SoundPriority( Viewport, Actor->Location, Actor->GetAmbientVolume(AmbientVolume), RM*Actor->SoundRadius, SF_Looping | SF_UpdatePitch );
				AmbientSounds(iAmbient).Flags		= SF_Looping | SF_UpdatePitch;
				AmbientSounds(iAmbient).Id			= Id;
			}
		}
	}
	unguard;
}

static INT Compare(ALAmbient& A,ALAmbient& B)
{
	return (B.Priority - A.Priority >= 0) ? 1 : -1;
}
void UALAudioSubsystem::Update( FSceneNode* SceneNode )
{
	guard(UALAudioSubsystem::Update);

	if(!Viewport)
		return;

	clock(GStats.DWORDStats(ALAudioStats.STATS_UpdateCycles));

	// Projection/ Orientation.
	FVector ViewLocation = SceneNode->ViewOrigin;
	FVector ProjUp		 = SceneNode->CameraToWorld.TransformNormal(FVector(0,1000,0));
	//ProjUp.Z = Abs( ProjUp.Z ); // Don't allow flipping "up".
	FVector ProjRight	 = SceneNode->CameraToWorld.TransformNormal(FVector(ReverseStereo ? -1000 : 1000,0,0));
	FVector ProjFront	 = ProjRight ^ ProjUp;

	ProjUp.Normalize();
	ProjRight.Normalize();
	ProjFront.Normalize();

	// Find out which zone the listener is in.
	ULevel* Level = Viewport->Actor->GetViewTarget()->GetLevel();
	RegionListener = Level->Model->PointRegion( Viewport->Actor->GetViewTarget()->Level, ViewLocation );

	// Time passes...
	FLOAT DeltaTime	= Viewport->CurrentTime - LastTime;
	LastTime		= Viewport->CurrentTime;
	DeltaTime 		= Clamp(DeltaTime,0.0001f,1.0f);
	
	FVector ListenerVelocity = (ViewLocation - LastPosition) / DeltaTime;
	
	UBOOL Realtime = Viewport->IsRealtime() && !Level->IsPaused();

	// Stop all sounds if transitioning out of realtime.
	if( !Realtime && LastRealtime )
		StopSound( NULL, NULL );

	LastRealtime = Realtime;

	// Check for finished sounds
	for (INT i=0; i<Sources.Num(); i++)
	{
		if(	Sources(i).Id && Sources(i).Sound && !(Sources(i).Flags & SF_Looping) )
		{
			ALint State;
			alGetSourcei( Sources(i).Source, AL_SOURCE_STATE, &State );
			if ( State == AL_STOPPED )
			{
				if( (Sources(i).Flags & SF_Streaming) && Streams(Sources(i).Sound->Handle-1).Alive )
				{
					// Stream ran out of buffers!!!
					//debugf(TEXT("WARNING: audio buffer underrun"));
					Streams(Sources(i).Sound->Handle-1).Reset=1;
				}
				else
				{
					// Stream is finished.
					StopSound(i);
				}
			}
		}
	}

	// See if any new ambient sounds need to be started.
	guard(HandleAmbience);
	if( Realtime )
	{		
		TArray<ALAmbient> AmbientSounds;
		
		// Check dynamic actors.
		INT StartIndex = GIsEditor ? 0 : Level->iFirstDynamicActor;
		for( INT i=StartIndex; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor = Level->Actors(i);
			if ( Actor && Actor->AmbientSound )
				UpdateAmbientSounds( AmbientSounds, Actor, Level, ViewLocation );
		}
					
		// Check static actors.
		for( INT i=0; i<StaticAmbientSounds.Num(); i++ )
		{
			AActor* Actor = StaticAmbientSounds(i);
			if ( Actor && Actor->AmbientSound )
				UpdateAmbientSounds( AmbientSounds, Actor, Level, ViewLocation );			
		}
		// Sort ambient sounds by priority to avoid thrashing of channels.
		if( AmbientSounds.Num() )
		{
			Sort( &AmbientSounds(0), AmbientSounds.Num() );
		
			for( INT iAmbient = 0; iAmbient < AmbientSounds.Num(); iAmbient++ )
			{
				AActor* &Actor = AmbientSounds(iAmbient).Actor;
				FLOAT Pitch = Clamp<FLOAT>(Actor->SoundPitch/64.f,0.5f,2.0f);
				// Early out if sound couldn't be played because of priority.
				FLOAT Volume = Actor->GetAmbientVolume(AmbientVolume);
				//debugf(TEXT("Ambient sound %s volume %f was %d"),Actor->AmbientSound->GetName(),Volume,Actor->SoundVolume);
				if( !PlaySound( Actor, AmbientSounds(iAmbient).Id, Actor->AmbientSound, Actor->Location, Volume, Actor->SoundRadius ? Actor->SoundRadius : GAudioDefaultRadius, Pitch, AmbientSounds(iAmbient).Flags, 0.f ) )
					break;
			}
		}
	}
	unguard;

	// Update all playing ambient sounds.
	guard(UpdateAmbience);
	for(INT i=0; i<Sources.Num(); i++)
	{
		if((Sources(i).Id&14) == SLOT_Ambient*2)
		{
			if( Sources(i).Actor )
			{
				FLOAT NewDist = FDistSquared(ViewLocation,Sources(i).Actor->Location);				
				if(	(NewDist > Square(Sources(i).ZoneRadius*GAudioMaxRadiusMultiplier)) 
				|| 	(Sources(i).Actor->AmbientSound != Sources(i).Sound)
				||	(GIsEditor && Sources(i).Actor->SoundRadius != Sources(i).Radius)
				)
				{
					StopSound(i);
				}
			}
		}
	}
	unguard;

	// Update all active sounds.
	guard(UpdateSounds);
	for(INT i=0; i<Sources.Num(); i++)
	{
		if(Sources(i).Actor)
			check(Sources(i).Actor->IsValid());

		if(Sources(i).Id != 0 && !Sources(i).Paused)
		{
			// Manage streaming sounds.
			if (Sources(i).Flags & SF_Streaming)
			{
				guard(UpdateStreamingSounds);
				INT Processed		= 0; 
				INT ChunksWaiting	= 0;
				UBOOL Alive			= false;
				
				ALStream& Stream = Streams(Sources(i).Sound->Handle - 1);
				alGetSourcei( Sources(i).Source, AL_BUFFERS_PROCESSED, &Processed );
				
				if ( Stream.Alive )
					Alive = GFileStream->QueryStream( Stream.Id-1, ChunksWaiting );
	
				if (Stream.Looped != GFileStream->Streams[Stream.Id-1].Looped)
				{
#if MERGEHACK_JOE
					if (Viewport && Viewport->Console)
						Viewport->Console->eventNotifyMusicChange();
#endif
					Stream.Looped = 0;
					GFileStream->Streams[Stream.Id-1].Looped = 0;
				}

				alError(TEXT("Before queueing."),false);
				if( Processed )
				{
					if ( ChunksWaiting )
					{
					}
					else
					{
						if( Stream.Alive )
						{
							alSourceUnqueueBuffers( Sources(i).Source, 1, &Stream.Buffer[Stream.Counter] );
							alBufferData( Stream.Buffer[Stream.Counter], Stream.Format, Stream.Data, STREAM_CHUNKSIZE, Stream.Rate );
							alSourceQueueBuffers( Sources(i).Source, 1, &Stream.Buffer[Stream.Counter] );
				
							GFileStream->RequestChunks( Stream.Id-1, 1, Stream.Data );
							++Stream.Counter %= BUFFERS_PER_STREAM;
						}

						if( !Alive )
						{
							GFileStream->DestroyStream( Stream.Id-1, false );
							Stream.Alive = 0;
						}
					}
				}
				
				if ( Stream.Reset )
				{
					// Start playing again as source was stopped because it ran out of buffers.
					alSourcePlay( Sources(i).Source );
					Stream.Reset = 0;
				}

				GStats.DWORDStats(ALAudioStats.STATS_ActiveStreamingSounds)++;
				unguard;
			}
			else
				GStats.DWORDStats(ALAudioStats.STATS_ActiveRegularSounds)++;

			// Update position, velocity, pitch and volume from actor (if wanted)
			FVector Velocity	= FVector(0,0,0);
			FLOAT Volume		= Sources(i).Volume;
			if( Sources(i).Actor && !(Sources(i).Flags & SF_NoUpdates) )
			{
				// Set location.
#if MERGEHACK_ERIK
				Sources(i).Location = (Sources(i).Flags & SF_RootMotion) ? Sources(i).Actor->GetRootLocation() : Sources(i).Actor->Location;
#else
				Sources(i).Location = Sources(i).Actor->Location;
#endif
				if (!(Sources(i).Flags & SF_No3D))
					Velocity = (Sources(i).Actor->Velocity - ListenerVelocity) * DISTANCE_FACTOR;

				// Set pitch.
				if (( Sources(i).Flags & SF_UpdatePitch ) && (!DisablePitch))
					alSourcef( Sources(i).Source, AL_PITCH, Clamp<FLOAT>(Sources(i).Actor->SoundPitch/64.f,0.5f,2.0f));

				// Set Volume.
				if( (Sources(i).Id&14) == SLOT_Ambient*2 )
				{
					Volume = Sources(i).Actor->GetAmbientVolume(AmbientVolume) * SoundVolume;
					Sources(i).Volume = Volume;
					if( Sources(i).Actor->LightType!=LT_None )
						Volume *= Sources(i).Actor->LightBrightness / 255.f;
				}
			}
			
			if( Sources(i).FadeMode == FADE_In )
			{
				// Disregard initial loading time.
				if( DeltaTime < 1.f )
					Sources(i).FadeTime += DeltaTime;
				if( Sources(i).FadeTime >= Sources(i).FadeDuration )
					Sources(i).FadeMode = FADE_None;
				else
					Volume *= Sources(i).FadeTime / Sources(i).FadeDuration;
			}

			if( Sources(i).FadeMode == FADE_Out )
			{
				// Disregard initial loading time.
				if( DeltaTime < 1.f )
					Sources(i).FadeTime += DeltaTime;
				if( Sources(i).FadeTime >= Sources(i).FadeDuration )
				{
					// Stop sound.
					if( Sources(i).Flags & SF_Streaming )
						UnregisterSound(Sources(i).Sound);
					else
						StopSound(i);
					continue;
				}
				else
					Volume *= (1.f - Sources(i).FadeTime / Sources(i).FadeDuration);
			}

			alSourcef( Sources(i).Source, AL_GAIN, Clamp(Volume, 0.0f, 1.0f) );

			if( !(Sources(i).Flags & SF_No3D) )
			{
				// Sound occclusion.
				FLOAT Radius = Sources(i).Radius;
				clock(GStats.DWORDStats(ALAudioStats.STATS_OcclusionCycles));
				if( !Level->IsAudibleAt(	
						Sources(i).Location, 
						ViewLocation, 
						Sources(i).Actor, 
						Sources(i).Actor ? (ESoundOcclusion) Sources(i).Actor->SoundOcclusion : OCCLUSION_Default ) 
				)
				{					
					FPointRegion RegionSource = Level->Model->PointRegion( Viewport->Actor->Level, Sources(i).Location );
					Radius *= Level->CalculateRadiusMultiplier( RegionSource.ZoneNumber, RegionListener.ZoneNumber );
					Sources(i).ZoneRadius = Radius;
					Radius *= OCCLUSION_FACTOR;
					GStats.DWORDStats(ALAudioStats.STATS_OccludedSounds)++;
				}
				else
					Sources(i).ZoneRadius = Radius;
				unclock(GStats.DWORDStats(ALAudioStats.STATS_OcclusionCycles));
				
				// Smooth transition between radii.
				if( Sources(i).WantedRadius != Radius )
				{
					Sources(i).LastChange	= Viewport->CurrentTime;
					Sources(i).WantedRadius = Radius;
				}

				if( (Viewport->CurrentTime - Sources(i).LastChange) < 1.f )
					Radius = Lerp( Sources(i).UsedRadius, Radius, Viewport->CurrentTime - Sources(i).LastChange );
				else
					Sources(i).UsedRadius = Radius;
				
				Radius = Max( Radius, 1.f );

				// Set Radius.
				if( alEAXSet == NULL )
				{
					alSourcef( Sources(i).Source, AL_REFERENCE_DISTANCE, Radius * DISTANCE_FACTOR );
				}
				else
				{
#if __EAX__
					long Occlusion = (long) (868.589f * log( Max( Radius / Sources(i).Radius, 0.00001f) ) ) ;
					alEAXSet(&DSPROPSETID_EAX30_BufferProperties, DSPROPERTY_EAXBUFFER_OCCLUSION, Sources(i).Source, &Occlusion, sizeof(Occlusion));
					//alEAXSet(&DSPROPSETID_EAX30_BufferProperties, DSPROPERTY_EAXBUFFER_DIRECT, Sources(i).Source, &Occlusion, sizeof(Occlusion));
					alSourcef( Sources(i).Source, AL_REFERENCE_DISTANCE, Radius * DISTANCE_FACTOR );
#endif
				}
			}

			// Update the priority.
			Sources(i).Priority = SoundPriority(
				Viewport,
				Sources(i).Location,
				Sources(i).Volume,
				Sources(i).ZoneRadius,
				Sources(i).Flags
			);

			// Set position & velocity.
			if( !(Sources(i).Flags & SF_No3D) )
			{
				FVector Location = Sources(i).Location * DISTANCE_FACTOR;
                if( Velocity.Size() > MAX_SOUND_SPEED ) 
                    Velocity = FVector(0,0,0);
                else
					Velocity *= DISTANCE_FACTOR;

				Location.Z *= -1;
				alSourcefv( Sources(i).Source, AL_POSITION, (ALfloat *) &Location );
				alSourcefv( Sources(i).Source, AL_VELOCITY, (ALfloat *) &Velocity );
			}

			//sceSdRemote(1,rSdSetParam,CoreVoiceMask(i) | SD_VP_PITCH,(INT) ((((float) Voices[i].Sound->SoundRate) / 48000.0f) * Voices[i].Pitch * ViewActors[Voices[i].ViewportNum]->Level->TimeDilation * Doppler * (44100.0f / 48000.0f) * 4096.0f));
		}
	}

	// Set Player position and orientation.
	FLOAT Orientation[6] =
		{ +ProjFront.X
		, +ProjFront.Y
		, -ProjFront.Z	// OpenAL: positive Z goes out of the screen.
		, +ProjUp.X
		, +ProjUp.Y
		, -ProjUp.Z };	// OpenAL: positive Z goes out of the screen.

	// Make the listener still and the sounds move relatively -- this allows 
    // us to scale the doppler effect on a per-sound basis.
    ListenerVelocity = FVector(0,0,0);

	LastPosition		= ViewLocation;
	ViewLocation		*= DISTANCE_FACTOR;
	ViewLocation.Z		*= -1;

	alListenerfv( AL_POSITION, (ALfloat *) &ViewLocation );
	alListenerfv( AL_ORIENTATION, (ALfloat *) Orientation );
	alListenerfv( AL_VELOCITY, (ALfloat *) &ListenerVelocity );

	// Set I3DL2 listener zone effect.
	SetI3DL2Listener( RegionListener.Zone->ZoneEffect );

	// Deferred commit (enforce min time between updates).
	if( Viewport->CurrentTime < LastHWUpdate )
		LastHWUpdate = Viewport->CurrentTime;
	if( (Viewport->CurrentTime - LastHWUpdate) >= (TimeBetweenHWUpdates / 1000.f) )
	{
		LastHWUpdate = Viewport->CurrentTime;
		alcProcessContext( SoundContext );
#ifdef WIN32
		alcSuspendContext( SoundContext );
#endif
	}

	unclock(GStats.DWORDStats(ALAudioStats.STATS_UpdateCycles));

	unguard;
	unguard;
}


UBOOL UALAudioSubsystem::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UALAudioSubsystem::Exec);
	const TCHAR*	Str = Cmd;
	if(ParseCommand(&Str,TEXT("PAUSESOUNDS")))
	{
		for(INT i=0; i<Sources.Num(); i++)
		{
			if(Sources(i).Id)
			{
				Sources(i).Paused = 1;
				alSourcePause( Sources(i).Source );
			}
		}
		return 1;
	}
	else if(ParseCommand(&Str,TEXT("UNPAUSESOUNDS")))
	{
		for(INT i=0; i<Sources.Num(); i++)
		{
			if(Sources(i).Id)
			{
				Sources(i).Paused = 0;
				if (Sources(i).Started)
					alSourcePlay( Sources(i).Source );
			}
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("STOPSOUNDS")) )
	{
		// Stop all playing sounds except music.
		for( INT i=0; i<Sources.Num(); i++ )
			if( !(Sources(i).Flags & SF_Music) )
				StopSound(i);

		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("LOCKMUSIC")) )
	{
		bMusicLocked=1;
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("UNLOCKMUSIC")) )
	{
		bMusicLocked=0;
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("WEAPONRADIUS")) )
	{
		if (appStrcmp(Cmd,TEXT("")) != 0) 
			GAudioDefaultRadius=appAtof(Cmd);
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("ROLLOFF")) )
	{
		if (appStrcmp(Cmd,TEXT("")) != 0) 
		{		
			RollOff=appAtof(Cmd);
			StopSound( NULL, NULL );
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("AUDIOCHANNELSUSED")) )
	{
		INT NumChannels=0;
		for(INT i=0;i<Sources.Num();i++)
		{
			ALSource& S = Sources(i);
			if(S.Sound || S.Actor)
				NumChannels++;
		}
		Ar.Logf(TEXT("%d"),NumChannels);
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("AUDIOCHANNELSTOTAL")) )
	{
		Ar.Logf(TEXT("%d"),Sources.Num());
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("SOUNDVOLUME")) )
	{
		Ar.Logf(TEXT("%f"),SoundVolume);
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("MUSICVOLUME")) )
	{
		Ar.Logf(TEXT("%f"),MusicVolume);
		return 1;
	}
	return 0;	
	unguard;
}


/*------------------------------------------------------------------------------------
	Global Pause/Resume music functions, needed for sync with StaticLoadObject.
------------------------------------------------------------------------------------*/

void appPauseMusic()
{
	guard(UALAudioSubsystem::PauseMusic);
	unguard;
}

void appResumeMusic()
{
	guard(UALAudioSubsystem::ResumeMusic);
	unguard;
}

/*------------------------------------------------------------------------------------
	Internals.
------------------------------------------------------------------------------------*/


void UALAudioSubsystem::SetVolumes()
{
	guard(UALAudioSubsystem::SetVolumes);

	// Update the music volume.
	if( PendingSong != TEXT("") && MusicVolume > 0.f )
		PlayMusic( PendingSong, 0.f );

	for (INT i=0; i<Sources.Num(); i++)
	{
		if ( Sources(i).Flags & SF_Music )
        {
			ALfloat Clamped = Clamp( MusicVolume, 0.f, 1.f );
			Sources(i).Volume = Clamped;
			#if !__WIN32__  // !!! FIXME: Test this on win32!
				// Make the values between 0.0 and 1.0 somewhat linear.
				// Linux's OpenAL needs this, Windows doesn't. Not sure which
				// implementation is actually correct, though. --ryan.
				Clamped *= Clamped;
				Clamped *= Clamped;
			#endif
        	alSourcef( Sources(i).Source, AL_GAIN, Clamped );
        }
	}

	unguard;
}


void UALAudioSubsystem::StopSound( INT i )
{
	guard(UALAudioSubsystem::StopSound);

	if( Sources(i).Id != 0 )
	{
		if ( bMusicLocked && Sources(i).Flags & SF_Music )
			return;

		GStats.DWORDStats(ALAudioStats.STATS_StoppedSounds)++;

		if ( Sources(i).Flags & SF_Streaming )
			GFileStream->DestroyStream( Streams(Sources(i).Sound->Handle-1).Id-1, false );
		
			alSourceStop( Sources(i).Source );
			alSourcei( Sources(i).Source, AL_BUFFER, NULL );

		Sources(i).Sound	= NULL;
		Sources(i).Actor	= NULL;
		Sources(i).Flags	= 0;
		Sources(i).Priority	= 0;
		Sources(i).Id		= 0;
		Sources(i).Started	= 0;
		Sources(i).Paused	= 0;
	}
	unguard;
}


FLOAT UALAudioSubsystem::SoundPriority( UViewport* Viewport, FVector Location, FLOAT Volume, FLOAT Radius, INT Flags )
{
	guard(UALAudioSubsystem::SoundPriority);
	FLOAT RadiusFactor;
	if ( Radius )
		RadiusFactor = 1 - FDist(Location, Viewport->Actor->GetViewTarget()->Location) / (GAudioMaxRadiusMultiplier*Radius);
	else
		RadiusFactor = 1;
	RadiusFactor = Clamp(RadiusFactor, 0.01f, 1.f);
	Volume		 = Clamp(Volume, 0.f, 1.f );
	return (Volume ? (0.85f + 0.15f * Volume) : 0.f ) * RadiusFactor + ((Flags & SF_Streaming)? 1 : 0) + ((Flags & SF_Music)? 2 : 0) + ((Flags & SF_No3D)? 1 : 0);
	unguard;
}


INT UALAudioSubsystem::GetNewStream()
{
	guard(UALAudioSubsystem::GetNewStream);
	for( INT i=0; i< Streams.Num(); i++ )
	{
		if( Streams(i).Id == 0 )
		{
			appMemzero( &Streams(i), sizeof(ALStream) );
			return i;
		}
	}
	appErrorf(TEXT("More than %i streams in use"), MAX_AUDIOSTREAMS );
	return -1;
	unguard;
}


void UALAudioSubsystem::SetI3DL2Listener( UI3DL2Listener* Listener )
{
	guard(SetI3DL2Listener);

#if __EAX__

	// Do nothing if EAX isn't supported.
	if( !UseEAX || !alEAXSet || !alEAXGet )
		return;

	// Check whether update is necessary.
	if( (OldListener == Listener) && (!Listener || !Listener->Updated) )
		return;

	OldListener	= Listener;
	
	if( !Listener )
	{
		// Set 'plain' EAX preset.
		_EAXLISTENERPROPERTIES EAXProperties;
		appMemzero(&EAXProperties, sizeof(EAXProperties));

		EAXProperties.ulEnvironment				= EAX_ENVIRONMENT_GENERIC;
		EAXProperties.ulEnvironment				= EAX_ENVIRONMENT_GENERIC;
		EAXProperties.flEnvironmentSize			= 7.5f;
		EAXProperties.flEnvironmentDiffusion	= 1.0;
		EAXProperties.lRoom						= -10000;
		EAXProperties.lRoomHF					= 0;
		EAXProperties.lRoomLF					= 0;
		EAXProperties.flDecayTime				= 1.49f;
		EAXProperties.flDecayHFRatio			= 0.83f;
		EAXProperties.flDecayLFRatio			= 1.00f;
		EAXProperties.lReflections				= -2602;
		EAXProperties.flReflectionsDelay		= 0.007f;
		//EAXProperties.vReflectionsPan			= *((EAXVECTOR*)&Listener->ReflectionsPan);
		EAXProperties.lReverb					= 200;
		EAXProperties.flReverbDelay				= 0.011f;
		//EAXProperties.vReverbPan				= *((EAXVECTOR*)&Listener->ReverbPan);
		EAXProperties.flEchoTime				= 0.25f;
		EAXProperties.flEchoDepth				= 0.f;
		EAXProperties.flModulationTime			= 0.25f;
		EAXProperties.flModulationDepth			= 0.f;
		EAXProperties.flAirAbsorptionHF			= 0.f;
		EAXProperties.flHFReference				= 5000.f;
		EAXProperties.flLFReference				= 250.f;
		EAXProperties.flRoomRolloffFactor		= RollOff;
		EAXProperties.ulFlags					= EAXLISTENERFLAGS_DECAYTIMESCALE
												| EAXLISTENERFLAGS_REFLECTIONSSCALE
												| EAXLISTENERFLAGS_REFLECTIONSDELAYSCALE
												| EAXLISTENERFLAGS_REVERBSCALE
												| EAXLISTENERFLAGS_REVERBDELAYSCALE
												| EAXLISTENERFLAGS_REVERBDELAYSCALE
												| EAXLISTENERFLAGS_DECAYHFLIMIT;			
		
		alEAXSet(&DSPROPSETID_EAX30_ListenerProperties,DSPROPERTY_EAXLISTENER_ALLPARAMETERS, NULL, &EAXProperties, sizeof(EAXProperties));
	}
	else
	{
		DWORD Flags = 0;
		if( Listener->bDecayTimeScale )
			Flags |= EAXLISTENERFLAGS_DECAYTIMESCALE;
		if( Listener->bReflectionsScale )
			Flags |= EAXLISTENERFLAGS_REFLECTIONSSCALE;
		if( Listener->bReflectionsDelayScale )
			Flags |= EAXLISTENERFLAGS_REFLECTIONSDELAYSCALE;
		if( Listener->bReverbScale )
			Flags |= EAXLISTENERFLAGS_REVERBSCALE;
		if( Listener->bReverbDelayScale )
			Flags |= EAXLISTENERFLAGS_REVERBDELAYSCALE;
		if( Listener->bReverbDelayScale )
			Flags |= EAXLISTENERFLAGS_REVERBDELAYSCALE;
		if( Listener->bEchoTimeScale )
			Flags |= EAXLISTENERFLAGS_ECHOTIMESCALE;
		if( Listener->bModulationTimeScale )
			Flags |= EAXLISTENERFLAGS_MODULATIONTIMESCALE;
		if( Listener->bDecayHFLimit )
			Flags |= EAXLISTENERFLAGS_DECAYHFLIMIT;
		
		_EAXLISTENERPROPERTIES EAXProperties;

		EAXProperties.ulEnvironment				= EAX_ENVIRONMENT_GENERIC;
		EAXProperties.flEnvironmentSize			= Listener->EnvironmentSize;
		EAXProperties.flEnvironmentDiffusion	= Listener->EnvironmentDiffusion;
		EAXProperties.lRoom						= Listener->Room;
		EAXProperties.lRoomHF					= Listener->RoomHF;
		EAXProperties.lRoomLF					= Listener->RoomLF;
		EAXProperties.flDecayTime				= Listener->DecayTime;
		EAXProperties.flDecayHFRatio			= Listener->DecayHFRatio;
		EAXProperties.flDecayLFRatio			= Listener->DecayLFRatio;
		EAXProperties.lReflections				= Listener->Reflections;
		EAXProperties.flReflectionsDelay		= Listener->ReflectionsDelay;
		EAXProperties.vReflectionsPan			= *((EAXVECTOR*)&Listener->ReflectionsPan);
		EAXProperties.lReverb					= Listener->Reverb;
		EAXProperties.flReverbDelay				= Listener->ReverbDelay;
		EAXProperties.vReverbPan				= *((EAXVECTOR*)&Listener->ReverbPan);
		EAXProperties.flEchoTime				= Listener->EchoTime;
		EAXProperties.flEchoDepth				= Listener->EchoDepth;
		EAXProperties.flModulationTime			= Listener->ModulationTime;
		EAXProperties.flModulationDepth			= Listener->ModulationDepth;
		EAXProperties.flAirAbsorptionHF			= Listener->AirAbsorptionHF;
		EAXProperties.flHFReference				= Listener->HFReference;
		EAXProperties.flLFReference				= Listener->LFReference;
		EAXProperties.flRoomRolloffFactor		= Listener->RoomRolloffFactor ? Listener->RoomRolloffFactor : RollOff;
		EAXProperties.ulFlags					= Flags;
		
		alEAXSet(&DSPROPSETID_EAX30_ListenerProperties,DSPROPERTY_EAXLISTENER_ALLPARAMETERS, NULL, &EAXProperties, sizeof(EAXProperties));

		// Change has been commited.
		Listener->Updated = false;
	}

#endif  // __EAX__

	unguard;
}

void UALAudioSubsystem::ChangeVoiceChatter( DWORD IpAddr, DWORD ControllerPort, UBOOL Add )
{
}
void UALAudioSubsystem::EnterVoiceChat()
{
}
void UALAudioSubsystem::LeaveVoiceChat()
{
}

UBOOL UALAudioSubsystem::alError( TCHAR* Text, UBOOL Log )
{
	ALint Error = alGetError();
	if ( Error == AL_NO_ERROR )
		return false;
	else
	{
		if ( Log )
		{
			switch ( Error )
			{
			case AL_INVALID_NAME:
				debugf(TEXT("ALAudio: AL_INVALID_NAME in %s"), Text);
				break;
#if __WIN32__  // !!! FIXME: This isn't in the Linux reference implemention? --ryan.
			case AL_INVALID_ENUM:
				debugf(TEXT("ALAudio: AL_INVALID_ENUM in %s"), Text);
				break;
#endif
			case AL_INVALID_VALUE:
				debugf(TEXT("ALAudio: AL_INVALID_VALUE in %s"), Text);
				break;
#if __WIN32__  // !!! FIXME: This isn't in the Linux reference implemention? --ryan.
			case AL_INVALID_OPERATION:
				debugf(TEXT("ALAudio: AL_INVALID_OPERATION in %s"), Text);
				break;
#endif
			case AL_OUT_OF_MEMORY:
				debugf(TEXT("ALAudio: AL_OUT_OF_MEMORY in %s"), Text);
				break;
			default:
				debugf(TEXT("ALAudio: Unknown error in %s"), Text);
			}
		}
		return true;
	}
}

UALAudioSubsystem::FALAudioStats::FALAudioStats()
{
	guard(FALAudioStats::FALAudioStats)
	appMemset( &STATS_FirstEntry, 0xFF, (DWORD) &STATS_LastEntry - (DWORD) &STATS_FirstEntry );
	unguard;
}

void UALAudioSubsystem::FALAudioStats::Init()
{
	guard(FALAudioStats::Init);

	// If already initialized retrieve indices from GStats.
	if( GStats.Registered[STATSTYPE_Audio] )
	{
		INT* Dummy = &STATS_PlaySoundCycles;
		for( INT i=0; i<GStats.Stats[STATSTYPE_Audio].Num(); i++ )
			*(Dummy++) = GStats.Stats[STATSTYPE_Audio](i).Index;
		return;
	}

	// Register stats with GStat.
	STATS_PlaySoundCalls				= GStats.RegisterStats( STATSTYPE_Audio, STATSDATATYPE_DWORD, TEXT("PlaySound"		), TEXT("Audio"		), STATSUNIT_Combined_Default_MSec	);
	STATS_PlaySoundCycles				= GStats.RegisterStats( STATSTYPE_Audio, STATSDATATYPE_DWORD, TEXT("PlaySound"		), TEXT("Audio"		), STATSUNIT_MSec					);
	STATS_OccludedSounds				= GStats.RegisterStats( STATSTYPE_Audio, STATSDATATYPE_DWORD, TEXT("Occlusion"		), TEXT("Audio"		), STATSUNIT_Combined_Default_MSec	);
	STATS_OcclusionCycles				= GStats.RegisterStats( STATSTYPE_Audio, STATSDATATYPE_DWORD, TEXT("Occlusion"		), TEXT("Audio"		), STATSUNIT_MSec					);
	STATS_UpdateCycles					= GStats.RegisterStats( STATSTYPE_Audio, STATSDATATYPE_DWORD, TEXT("Update"			), TEXT("Audio"		), STATSUNIT_MSec					);
	STATS_ActiveStreamingSounds			= GStats.RegisterStats( STATSTYPE_Audio, STATSDATATYPE_DWORD, TEXT("Streaming"		), TEXT("Audio"		), STATSUNIT_Default				);
	STATS_ActiveRegularSounds			= GStats.RegisterStats( STATSTYPE_Audio, STATSDATATYPE_DWORD, TEXT("Regular"		), TEXT("Audio"		), STATSUNIT_Default				);
	STATS_StoppedSounds					= GStats.RegisterStats( STATSTYPE_Audio, STATSDATATYPE_DWORD, TEXT("StoppedSounds"	), TEXT("Audio"		), STATSUNIT_Default				);
	
	// Initialized.
	GStats.Registered[STATSTYPE_Audio] = 1;

	unguard;
}

UBOOL UALAudioSubsystem::FindExt( const TCHAR* Name )
{
	guard(UALAudioSubsystem::FindExt);
	UBOOL Result = appStrfind(appFromAnsi((char*)alGetString(AL_EXTENSIONS)),Name)!=NULL;
	if( Result )
		debugf( NAME_Init, TEXT("Device supports: %s"), Name );
	return Result;
	unguard;
}

void UALAudioSubsystem::FindProc( void*& ProcAddress, char* Name, char* SupportName, UBOOL& Supports, UBOOL AllowExt )
{
	guard(UALAudioSubsystem::FindProc);
	ProcAddress = NULL;
#if DYNAMIC_BIND
	if( !ProcAddress )
		ProcAddress = appGetDllExport( DLLHandle, appFromAnsi(Name) );
#endif
	if( !ProcAddress && Supports && AllowExt )
		ProcAddress = alGetProcAddress( (ALubyte*) Name );
	if( !ProcAddress )
	{
		if( Supports )
			debugf( TEXT("   Missing function '%s' for '%s' support"), appFromAnsi(Name), appFromAnsi(SupportName) );
		Supports = 0;
	}
	unguard;
}

void UALAudioSubsystem::FindProcs( UBOOL AllowExt )
{
	guard(UALAudioSubsystem::FindProcs);
	#define AL_EXT(name) if( AllowExt ) SUPPORTS##name = FindExt( TEXT(#name)+1 );
	#define AL_PROC(ext,ret,func,parms) FindProc( *(void**)&func, #func, #ext, SUPPORTS##ext, AllowExt );
	#include "ALFuncs.h"
	#undef AL_EXT
	#undef AL_PROC
	unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
