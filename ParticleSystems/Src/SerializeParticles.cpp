/*=============================================================================
	SerializeParticles.cpp: Serialization code for particles.
	Copyright 2002 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Aaron Leiby
=============================================================================*/

#include "ParticleSystems.h"


/*----------------------------------------------------------------------------
	LoadParticles.
----------------------------------------------------------------------------*/

FArchiveParticles& LoadParticles::operator<<( UParticle* &P )
{
	guard(LoadParticles<<);
	*this << *(DWORD*)&P;
	if( P )	// if the pointer is NULL then we're done.
	{
		DWORD Key = (DWORD)P;	// use old memory address as our key.
		UParticle* Ref = ParticleMap.FindRef(Key);
		if( Ref )
		{
			P = Ref;
		}
		else	// if this particle hasn't been previously loaded...
		{
			FString ClassName;
			*this << ClassName;
			particleClass* C = StringFindParticleClass(*ClassName);
			if( C )
			{
				P = C->CreateInstance();
				NOTE(debugf(TEXT("Created new particle %s (%d)"),P->GetClassName(),P));
			}
			else
			{
				P = NULL;
				appErrorf(TEXT("Error loading particle!  Class not found: '%s'"),*ClassName);
			}
			P->Serialize(*this);
			ParticleMap.Set(Key,P);
		}
	}
	return *this;
	unguard;
}

FArchiveParticles& LoadParticles::operator<<( UForceVars* &V )
{
	guard(LoadForceVars<<);
	*this << *(DWORD*)&V;
	if( V )	// if the pointer is NULL then we're done.
	{
		DWORD Key = (DWORD)V;	// use old memory address as our key.
		UForceVars* Ref = ForceVarsMap.FindRef(Key);
		if( Ref )
		{
			V = Ref;
		}
		else	// if this particle hasn't been previously loaded...
		{
			FString ClassName;
			*this << ClassName;
			forcevarsClass* C = StringFindForceVarsClass(*ClassName);
			if( C )
			{
				V = C->CreateInstance();
				NOTE(debugf(TEXT("Created new particle %s (%d)"),V->GetClassName(),V));
			}
			else
			{
				V = NULL;
				appErrorf(TEXT("Error loading particle!  Class not found: '%s'"),*ClassName);
			}
			V->Serialize(*this);
			ForceVarsMap.Set(Key,V);
		}
	}
	return *this;
	unguard;
}


/*----------------------------------------------------------------------------
	SaveParticles.
----------------------------------------------------------------------------*/

FArchiveParticles& SaveParticles::operator<<( UParticle* &P )
{
	guard(SaveParticles<<);
	*this << *(DWORD*)&P;
	if( P )	// if the pointer is NULL then we're done.
	{
		DWORD Key = (DWORD)P;	// use old memory address as our key.
		UParticle* Ref = ParticleMap.FindRef(Key);
		if( Ref )
		{
			P = Ref;
		}
		else	// if this particle hasn't been previously saved...
		{
			FString ClassName = P->GetClassName();
			*this << ClassName;
			P->Serialize(*this);
			ParticleMap.Set(Key,P);
		}
	}
	return *this;
	unguard;
}

FArchiveParticles& SaveParticles::operator<<( UForceVars* &V )
{
	guard(SaveForceVars<<);
	*this << *(DWORD*)&V;
	if( V )	// if the pointer is NULL then we're done.
	{
		DWORD Key = (DWORD)V;	// use old memory address as our key.
		UForceVars* Ref = ForceVarsMap.FindRef(Key);
		if( Ref )
		{
			V = Ref;
		}
		else	// if this particle hasn't been previously saved...
		{
			FString ClassName = V->GetClassName();
			*this << ClassName;
			V->Serialize(*this);
			ForceVarsMap.Set(Key,V);
		}
	}
	return *this;
	unguard;
}

