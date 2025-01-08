/*=============================================================================
	UMovie.h: In-Game Movie Utility
	Copyright 2002 Epic MegaGames, Inc. This software is a trade secret.

	Revision history:
		* Created by Chris Linder of Demiurge Studios.
=============================================================================*/

#ifndef _U_MOVIE_H_
#define _U_MOVIE_H_

//
//	UMovie
//
class ENGINE_API UMovie : public UObject
{
	DECLARE_CLASS(UMovie,UObject,0,Engine);
//	NO_DEFAULT_CONSTRUCTOR(UMovie)

public:

	UMovie();	

	class FMovie *fMovie;	

    DECLARE_FUNCTION(execGetHeight);
    DECLARE_FUNCTION(execGetWidth);
    DECLARE_FUNCTION(execIsPlaying);
    DECLARE_FUNCTION(execStopAtEnd);
    DECLARE_FUNCTION(execStopNow);
    DECLARE_FUNCTION(execIsPaused);
    DECLARE_FUNCTION(execPause);
    DECLARE_FUNCTION(execPlay);
};


#endif




