
class FU2SpriteParticleVertex
{
public:
	FVector	Position;
	FColor	Color;
	FLOAT	U,V;

	static INT GetComponents(FVertexComponent* OutComponents)
	{
		OutComponents[0].Type = CT_Float3;
		OutComponents[0].Function = FVF_Position;
		OutComponents[1].Type = CT_Color;
		OutComponents[1].Function = FVF_Diffuse;
		OutComponents[2].Type = CT_Float2;
		OutComponents[2].Function = FVF_TexCoord0;

		return 3;
	}
};

template<class TVertexClass,INT TSize>
class FU2ParticleVertexStream : public FVertexStream
{
public:
	TVertexClass*				VB;
	INT							BaseIndex;

	enum { SIZE=TSize };

	FU2ParticleVertexStream(FRenderInterface* RI)
	{
		BaseIndex = RI->LockDynamicStream(this,(BYTE**)&VB);
	}

	// FRenderResource interface (dynamic streams are never cached).
	QWORD GetCacheId()	{ return 0; }
	INT GetRevision()	{ return 0; }

	// FVertexStream interface.
	INT GetSize()		{ return TSize * sizeof(TVertexClass); }
	INT GetStride()		{ return sizeof(TVertexClass); }
	INT GetComponents(FVertexComponent* OutComponents)
						{ return TVertexClass::GetComponents(OutComponents); }
	void GetStreamData(void* Dest)
						{ appErrorf(TEXT("Bad Andrew!!  Don't call this function.  Copying copious amounts of data is slooow.")); }
	void GetRawStreamData(void** Dest,INT FirstVertex)
						{ appErrorf(TEXT("Bad Andrew!!  Don't call this function.  Copying copious amounts of data is slooow.")); }
	INT GetBaseIndex()	{ return BaseIndex; }
};

template<INT TSize>
class FU2RawIndexBuffer : public FIndexBuffer
{
public:
	_WORD*			IB;
	INT				BaseIndex;

	enum { SIZE=TSize };

	FU2RawIndexBuffer(FRenderInterface* RI)
	{
		BaseIndex = RI->LockDynamicIndexBuffer(this,(BYTE**)&IB);
	}

	// FRenderResource interface (dynamic streams are never cached).
	QWORD GetCacheId()	{ return 0; }
	INT GetRevision()	{ return 0; }

	// FIndexBuffer interface.
	INT GetSize()		{ return TSize * sizeof(_WORD); }
	INT GetIndexSize()	{ return sizeof(_WORD); }
	void GetContents(void* Data)
						{ appErrorf(TEXT("Bad Andrew!!  Don't call this function.  Copying copious amounts of data is slooow.")); }
	INT GetBaseIndex()	{ return BaseIndex; }
};

