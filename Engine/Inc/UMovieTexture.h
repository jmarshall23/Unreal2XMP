/*=============================================================================
	UMovieTexture.h: In-Game Movie Utility
	Copyright 2002 Epic MegaGames, Inc. This software is a trade secret.

	Revision history:
		* Created by Chris Linder of Demiurge Studios.
=============================================================================*/

UMovieTexture();
virtual void Destroy();

virtual void InitMovie();

virtual void ConstantTimeTick();
virtual void PostEditChange();