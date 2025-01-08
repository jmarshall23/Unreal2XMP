/*=============================================================================
	UnMovie.cpp: In-Game Movie Implementations.
	Copyright 2002 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Chris Linder
=============================================================================*/

#include "EnginePrivate.h"
#include "UnMovie.h"

//=============================================================================
// UMovie 

IMPLEMENT_CLASS(UMovie);



UMovie::UMovie() :
	UObject(),
	fMovie(NULL)
{
}



void UMovie::execPlay( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execPlay);
	P_GET_STR(MovieFilename);
	P_GET_UBOOL(UseSound);
	P_GET_UBOOL(LoopMovie);
	P_GET_INT_OPTX( FrameRate, 30 );
	P_FINISH;

	if(fMovie)
	{
		delete fMovie;
		fMovie = NULL;
	}

	ECodecType CodecType;

	if(MovieFilename.Right(4).Caps() == FString(TEXT(".BIK")) )
	{
		CodecType = BINK_VIDEO;
	}
	else if(MovieFilename.Right(4).Caps() == FString(TEXT(".ROQ")) )
	{
		CodecType = ROQ_VIDEO;
	}
	else
	{
		CodecType = INVALID_VIDEO;
	}

	//TODO - this is a hack to get the the rendering device.  Why isn't GRenDev global, I don't know!
	TObjectIterator<UEngine> EngineIt;
	if((*EngineIt)->GRenDev)
	{
		fMovie = (*EngineIt)->GRenDev->GetNewMovie(CodecType, FString(TEXT("..\\Movies\\")) + MovieFilename, UseSound, FrameRate);
	}

	if(fMovie)
		fMovie->Play(LoopMovie);

	unguardexec;
}
IMPLEMENT_FUNCTION( UMovie, -1, execPlay );



void UMovie::execPause( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execPause);
	P_GET_UBOOL(p);
	P_FINISH;

	if(fMovie)
		fMovie->Pause(p);

	unguardexec;
}
IMPLEMENT_FUNCTION( UMovie, -1, execPause );



void UMovie::execIsPaused( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execIsPaused);
	P_FINISH;

	if(fMovie)
		*(UBOOL*)Result = fMovie->IsPaused();

	unguardexec;
}
IMPLEMENT_FUNCTION( UMovie, -1, execIsPaused );



void UMovie::execStopNow( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execStopNow);
	P_FINISH;

	if(fMovie)
		fMovie->StopNow();

	unguardexec;
}
IMPLEMENT_FUNCTION( UMovie, -1, execStopNow );



void UMovie::execStopAtEnd( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execStopAtEnd);
	P_FINISH;

	if(fMovie)
		fMovie->StopAtEnd();

	unguardexec;
}
IMPLEMENT_FUNCTION( UMovie, -1, execStopAtEnd );



void UMovie::execIsPlaying( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execIsPlaying);
	P_FINISH;

	if(fMovie)
		*(UBOOL*)Result =  fMovie->IsPlaying();

	unguardexec;
}
IMPLEMENT_FUNCTION( UMovie, -1, execIsPlaying );



void UMovie::execGetWidth( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execGetWidth);
	P_FINISH;

	if(fMovie)
		*(INT*)Result =  fMovie->GetWidth();

	unguardexec;
}
IMPLEMENT_FUNCTION( UMovie, -1, execGetWidth );



void UMovie::execGetHeight( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execGetHeight);
	P_FINISH;

	if(fMovie)
		*(INT*)Result =  fMovie->GetHeight();

	unguardexec;
}
IMPLEMENT_FUNCTION( UMovie, -1, execGetHeight );




//=============================================================================
// FMovie 

FMovie::FMovie( FString FileName, UBOOL UseSound ) :
	bUseSound(UseSound),
	bMoviePlaying(false)
{
}



FMovie::~FMovie()
{
}



UBOOL FMovie::IsPlaying()
{
	return bMoviePlaying;
}




//=============================================================================
// FRoQMovie 

FRoQMovie::FRoQMovie( FString FileName, UBOOL UseSound, INT InFrameRate  ) :
	FMovie(FileName, UseSound),
	FrameRate(InFrameRate),
	bStopAtEnd(false),
	bPauseMovie(false)
{
	if(UseSound)
	{
		//DO SOMETHING
	}

	ri = roq_open( const_cast<char*> (appToAnsi(*FileName)) );
}



FRoQMovie::~FRoQMovie()
{
	if (ri)
	{
		roq_close(ri);
		ri = NULL;
	}
}



UBOOL FRoQMovie::Play(UBOOL LoopMovie)
{
	if(ri)
	{
		//TODO handle sound

		//goto begining
		ri->frame_num = 0;
		ri->aud_pos = ri->vid_pos = ri->roq_start;
		roq_read_frame(ri);

		bMoviePlaying = true;
		bStopAtEnd = !LoopMovie;

		Pause(false);

		StartTime = appSeconds();

		return true;
	}
	return false;
}



void FRoQMovie::Pause(UBOOL Pause)
{
	//if we are not paused and we are getting paused record the pause time
	if(!bPauseMovie && Pause)
	{
		PauseStartTime = appSeconds();
	}

	//if we are paused and we are getting unpaused shift StartTime so the frame rate still works
	if(bPauseMovie && !Pause)
	{
		StartTime += appSeconds() - PauseStartTime;
	}

	bPauseMovie = Pause;
}



UBOOL FRoQMovie::IsPaused()
{
	return bPauseMovie;
}



void FRoQMovie::StopNow()
{
	bMoviePlaying = false;

	//TODO handle sound turn off on stop
}



void FRoQMovie::StopAtEnd()
{
	bStopAtEnd = true;
}



INT FRoQMovie::GetWidth()
{
	if(ri)
	{
		return ri->width;
	}
	return 0;
}


	
INT FRoQMovie::GetHeight()
{
	if(ri)
	{
		return ri->height;
	}
	return 0;
}


void FRoQMovie::PreRender(void* inRenderTarget, INT PosX, INT PosY)
{
	if(ri)
	{
		//deal with next frame time
		DWORD DesiredFrameNum = (DWORD) ((appSeconds() - StartTime) * FrameRate);
		if(!bPauseMovie)
		{
		if(DesiredFrameNum > (DWORD)ri->frame_num)
			//while(DesiredFrameNum > (DWORD)ri->frame_num)  //For some reason this crashes if the movie gets behind.
			{
				roq_read_frame(ri);
			}
		}

		//deal with looping
		if((DWORD)ri->frame_num >= ri->num_frames)
		{
			if(bStopAtEnd)
				StopNow();
			else
			{
				ri->frame_num = 0;
				ri->aud_pos = ri->vid_pos = ri->roq_start;
				StartTime = appSeconds();
			}
		}
	}
}


void FRoQMovie::RenderToRGBAArray(BYTE* RenderTarget)
{
	PreRender(RenderTarget, 0, 0);

	if(bMoviePlaying && RenderTarget)
	{		
		BYTE *pa = ri->y[0];
		BYTE *pb = ri->u[0];
		BYTE *pc = ri->v[0];

		BYTE *buf = (BYTE*) RenderTarget;

		INT x, y;
		INT rs = 2,
			gs = 1,
			bs = 0;
		
		#define limit(x) ((((x) > 0xffffff) ? 0xff0000 : (((x) <= 0xffff) ? 0 : (x) & 0xff0000)) >> 16)
		for(y = 0; y < ri->height; y++)
		{
			for(x = 0; x < ri->width/2; x++)
			{
				INT r, g, b, y1, y2, u, v;
				y1 = *(pa++); y2 = *(pa++);
				u = pb[x] - 128;
				v = pc[x] - 128;
				
				y1 *= 65536; y2 *= 65536;
				r = 91881 * v;
				g = -22554 * u + -46802 * v;
				b = 116130 * u;
				
				buf[(x*8)+rs] = limit(r + y1);
				buf[(x*8)+gs] = limit(g + y1);
				buf[(x*8)+bs] = limit(b + y1);
				buf[(x*8)+4+rs] = limit(r + y2);
				buf[(x*8)+4+gs] = limit(g + y2);
				buf[(x*8)+4+bs] = limit(b + y2);
			}
			if(y & 0x01)
			{
				pb += ri->width/2;
				pc += ri->width/2;
			}
			buf += ri->width*4;
		}
	}
}


//=============================================================================
// UMovieTexture

IMPLEMENT_CLASS(UMovieTexture);



UMovieTexture::UMovieTexture() :
	Movie(NULL)
{
	Format = TEXF_RGBA8;
}



void UMovieTexture::Destroy()
{
	if(Movie && Movie->fMovie)
	{
		delete Movie->fMovie;
		Movie->fMovie = NULL;
	}

	Super::Destroy();
}



void UMovieTexture::InitMovie()
{
	if(!Movie)
	{
		Movie = ConstructObject<UMovie>( UMovie::StaticClass() );
		Movie->fMovie = NULL;
	}

	if(Movie)
	{
		if(Movie->fMovie)
		{
			delete Movie->fMovie;
			Movie->fMovie = NULL;
		}


		ECodecType CodecType;

		if(MovieFilename.Right(4).Caps() == FString(TEXT(".BIK")) )
		{
			CodecType = BINK_VIDEO;
		}
		else if(MovieFilename.Right(4).Caps() == FString(TEXT(".ROQ")) )
		{
			CodecType = ROQ_VIDEO;
		}
		else
		{
			CodecType = INVALID_VIDEO;
		}

		TObjectIterator<UEngine> EngineIt;
		Movie->fMovie = (*EngineIt)->Client->Viewports(0)->RenDev->GetNewMovie(CodecType, FString(TEXT("..\\Movies\\")) + MovieFilename, false, FrameRate);
	}

	if(Movie && Movie->fMovie)
	{
		INT width = Movie->fMovie->GetWidth();
		INT height = Movie->fMovie->GetHeight();
		if( (width&(width-1)) || (height&(height-1)) )
		{
			GWarn->Logf( TEXT("Movie dimensions are not powers of two.") );
			if(GIsEditor)
			{
				appMsgf(0,TEXT("Movie dimensions are not powers of two.  Please Select Another Movie."));				
			}
			Init(0, 0);
		}
		else
		{
			Init(Movie->fMovie->GetWidth(), Movie->fMovie->GetWidth());
			Movie->fMovie->Play(true);
		}
	}

}



void UMovieTexture::ConstantTimeTick()
{
	// If everything is ready to render the movie
	if(Movie && Movie->fMovie && Mips.Num() > 0)
	{
		bRealtimeChanged=true;
		Movie->fMovie->RenderToRGBAArray( &Mips(0).DataArray(0) );
	}
	else
	{
		InitMovie();
	}
}



void UMovieTexture::PostEditChange()
{
	InitMovie();
}



void UMovieTexture::execInitializeMovie( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execInitializeMovie);
	P_FINISH;

	InitMovie();

	unguardexec;
}
//IMPLEMENT_FUNCTION( UMovieTexture, -1, execInitializeMovie );

