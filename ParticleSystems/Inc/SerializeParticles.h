/*=============================================================================
	SerializeParticles.h: Serialization code for particles.
	Copyright 2002 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Aaron Leiby
=============================================================================*/


/*----------------------------------------------------------------------------
	Macros.
----------------------------------------------------------------------------*/

#define SERIALIZE_BOOL(b) \
	{ BYTE Value=b; Ar.SerializeBits(&Value,1); b=Value; } \


/*----------------------------------------------------------------------------
	FArchiveParticles.
----------------------------------------------------------------------------*/

class FArchiveParticles : public FArchive
{
public:
	FArchiveParticles( FArchive& _Ar ): Ar(_Ar)
	{
		ArVer			= Ar.Ver();
		ArNetVer		= Ar.NetVer();
		ArLicenseeVer	= Ar.LicenseeVer();
//!!MERGE		ArLicenseeFlags	= Ar.LicenseeFlags();
		ArIsLoading		= Ar.IsLoading();
		ArIsSaving		= Ar.IsSaving();
		ArIsTrans		= Ar.IsTrans();
		ArIsPersistent	= Ar.IsPersistent();
		ArForEdit		= Ar.ForEdit();
		ArForClient		= Ar.ForClient();
		ArForServer		= Ar.ForServer();
		ArIsError		= Ar.IsError();
	}
	virtual ~FArchiveParticles(){}

	virtual FArchiveParticles& operator<<( class UParticle* &A ){ return *this; }
	virtual FArchiveParticles& operator<<( class UForceVars* &A ){ return *this; }

protected:
	FArchive& Ar;
	TMap< DWORD, UParticle* > ParticleMap;
	TMap< DWORD, UForceVars* > ForceVarsMap;

public:
	// FArchive interface.
	virtual void Serialize( void* V, INT Length )				{ Ar.Serialize( V, Length ); }
	virtual void SerializeBits( void* V, INT LengthBits )		{ Ar.SerializeBits( V, LengthBits ); }
	virtual void SerializeInt( DWORD& Value, DWORD Max )		{ Ar.SerializeInt( Value, Max ); }
	virtual void Preload( UObject* Object )						{ Ar.Preload( Object ); }
	virtual void CountBytes( SIZE_T InNum, SIZE_T InMax )		{ Ar.CountBytes( InNum, InMax ); }
	virtual FArchive& operator<<( class FName& N )				{ Ar << N; return *this; }
	virtual FArchive& operator<<( class UObject*& Res )			{ Ar << Res; return *this; }
	virtual INT MapName( FName* Name )							{ return Ar.MapName( Name ); }
	virtual INT MapObject( UObject* Object )					{ return Ar.MapObject( Object ); }
	virtual INT Tell()											{ return Ar.Tell(); }
	virtual INT TotalSize()										{ return Ar.TotalSize(); }
	virtual UBOOL AtEnd()										{ return Ar.AtEnd(); }
	virtual UBOOL AtStopper()									{ return Ar.AtStopper(); }
	virtual void SetStopper( INT InStopper=INDEX_NONE )			{ Ar.SetStopper( InStopper ); }
	virtual void Seek( INT InPos )								{ Ar.Seek( InPos ); }
	virtual void AttachLazyLoader( FLazyLoader* LazyLoader )	{ Ar.AttachLazyLoader( LazyLoader ); }
	virtual void DetachLazyLoader( FLazyLoader* LazyLoader )	{ Ar.DetachLazyLoader( LazyLoader ); }
	virtual void Precache( INT HintCount )						{ Ar.Precache( HintCount ); }
	virtual void Flush()										{ Ar.Flush(); }
	virtual UBOOL Close()										{ return Ar.Close(); }
	virtual UBOOL GetError()									{ return Ar.GetError(); }

	// Friend archivers.
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, ANSICHAR& A )
		{ _Ar.Ar << A; return _Ar; }
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, BYTE& A )
		{ _Ar.Ar << A; return _Ar; }
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, SBYTE& A )
		{ _Ar.Ar << A; return _Ar; }
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, _WORD& A )
		{ _Ar.Ar << A; return _Ar; }
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, SWORD& A )
		{ _Ar.Ar << A; return _Ar; }
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, DWORD& A )
		{ _Ar.Ar << A; return _Ar; }
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, INT& A )
		{ _Ar.Ar << A; return _Ar; }
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, FLOAT& A )
		{ _Ar.Ar << A; return _Ar; }
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, DOUBLE& A )
		{ _Ar.Ar << A; return _Ar; }
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, QWORD& A )
		{ _Ar.Ar << A; return _Ar; }
	friend FArchiveParticles& operator<<( FArchiveParticles& _Ar, SQWORD& A )
		{ _Ar.Ar << A; return _Ar; }
};


/*----------------------------------------------------------------------------
	LoadParticles.
----------------------------------------------------------------------------*/

class LoadParticles : public FArchiveParticles
{
public:
	LoadParticles( FArchive& _Ar ): FArchiveParticles(_Ar){}
	virtual ~LoadParticles(){}

	FArchiveParticles& operator<<( UParticle* &A );
	FArchiveParticles& operator<<( UForceVars* &A );
};


/*----------------------------------------------------------------------------
	SaveParticles.
----------------------------------------------------------------------------*/

class SaveParticles : public FArchiveParticles
{
public:
	SaveParticles( FArchive& _Ar ): FArchiveParticles(_Ar){}
	virtual ~SaveParticles(){}

	FArchiveParticles& operator<<( UParticle* &A );
	FArchiveParticles& operator<<( UForceVars* &A );
};


