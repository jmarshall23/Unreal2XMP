/*=============================================================================
	UnMovie.h: In-Game Movie Utility
	Copyright 2002 Epic MegaGames, Inc. This software is a trade secret.

	Revision history:
		* Created by Chris Linder of Demiurge Studios.
=============================================================================*/

#ifndef _UN_MOVIE_H_
#define _UN_MOVIE_H_

extern "C"
{
#include "roq.h"		// RoQ
}
//
//	FMovie
//
class ENGINE_API FMovie
{

public:
		
	// Consturcts an FMovie from the given FileName.  If UseSound is set, the movie
	// will play the sound in the movie if there is any.
	FMovie(FString FileName, UBOOL UseSound);

	// Destructor called when object is deleted.	
	virtual ~FMovie();

	// Plays this movie based on the settings in the constructor.
	// Will loop the movie if LoopMovie is true.
	// Returns false if fails to start playing
	virtual UBOOL Play(UBOOL LoopMovie) = 0;

	// Pauses or unpaused the movie and continues to display the movie.
	virtual void Pause(UBOOL Pause) = 0;

	// Returns true if the movie is paused
	virtual UBOOL IsPaused() = 0;

	//Stops the movie playback right now.
	virtual void StopNow() = 0;

	// Stops the movie playback when the movie is over.
	// Returns false if fails.
	virtual void StopAtEnd() = 0;

	// Returns true if the movie is playing.  If this is false, the movie will not de displayed.
	virtual UBOOL IsPlaying();

	// Returns the width of the FMovie in pixels
	virtual INT GetWidth() = 0;
	
	// Returns the height of the FMovie in pixels
	virtual INT GetHeight() = 0;

	// Does calculations before the render like if the movie is at the end should it loop
	virtual void PreRender(void* RenderTarget, INT PosX, INT PosY) = 0;

	// Renders the movie to and goes to the next frame if appropriate.
	// This function non optimized for any rendering device and just copies
	// the movie into the Unreal RGBA data array representation.
	virtual void RenderToRGBAArray(BYTE* inRenderTarget) = 0;

	// Renders the movie and goes to the next frame if appropriate.
	// This function is optimized for particular rendering devices and should
	// work closely with the rending device.  The RenderTarget should be an
	// apporpriate hardware accelerated surface.
	//
	// This function has blank body because it is not needed, it just is a good idea.
	// Some movie type or rendering device may not be able to implement a hardware accelerated solution 
	virtual void RenderToNative(void* RenderTarget, INT PosX, INT PosY) { return; };



protected:

	//if this true the movie is being played.  
    UBOOL bMoviePlaying;

	//if the movie should use sound - set by constuctor
	const UBOOL bUseSound;

};



//
//	FRoQMovie
//
class ENGINE_API FRoQMovie : public FMovie
{

public:
				
	// Consturcts an FMovie from the given FileName.  If UseSound is set, the movie
	// will play the sound in the movie if there is any.
	// FrameRate is the frame rate for the movie.  RoQ movies do not store frame rate in the
	// file so this parameter is nessisary.
	FRoQMovie(FString FileName, UBOOL UseSound, INT FrameRate = 30);

	// Destructor called when object is deleted.	
	virtual ~FRoQMovie();

	// Plays this movie based on the settings in the constructor.
	// Will loop the movie if LoopMovie is true.
	// Returns false if fails to start playing
	virtual UBOOL Play(UBOOL LoopMovie);

	// Pauses or unpaused the movie and continues to display the movie.
	virtual void Pause(UBOOL Pause);

	// Returns true if the movie is paused
	virtual UBOOL IsPaused();

	// Stops the movie playback right now.
	virtual void StopNow();

	// Stops the movie playback when the movie is over.
	// Returns false if fails.
	virtual void StopAtEnd();

	// Returns the width of the FMovie in pixels
	virtual INT GetWidth();
	
	// Returns the height of the FMovie in pixels
	virtual INT GetHeight();

	// Does calculations before the render like if the movie is at the end should it loop
	virtual void PreRender(void* RenderTarget, INT PosX, INT PosY);

	// Renders the movie to and goes to the next frame if appropriate.
	// This function non optimized for any rendering device and just copies
	// the movie into the Unreal RGBA data array representation.
	virtual void RenderToRGBAArray(BYTE* inRenderTarget);


protected:

	// The RoQ movie
	roq_info *ri;

	//The framerate of this RoQ movie
	INT FrameRate;

	//if the movie stop at the end
	UBOOL bStopAtEnd;

	//true if the movie is paused
	UBOOL bPauseMovie;

	//the last time play was called.  Used to match frame rate.
	DOUBLE StartTime;

	//the time the movie was paused.  Used to match frame rate.
	DOUBLE PauseStartTime;
};


#endif
