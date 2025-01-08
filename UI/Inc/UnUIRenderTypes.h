

/*-----------------------------------------------------------------------------
	FUIVertex.
-----------------------------------------------------------------------------*/

class FUIVertex
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


/*-----------------------------------------------------------------------------
	FUIVertexStream.
-----------------------------------------------------------------------------*/

template<class TVertexClass,INT TSize>
class FUIVertexStream : public FVertexStream
{
public:
	TVertexClass*				VB;
	INT							BaseIndex;

	enum { SIZE=TSize };

	FUIVertexStream(FRenderInterface* RI)
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


/*-----------------------------------------------------------------------------
	FUIIndexBuffer.
-----------------------------------------------------------------------------*/

template<INT TSize>
class FUIIndexBuffer : public FIndexBuffer
{
public:
	_WORD*	IB;
	INT		BaseIndex;

	enum { SIZE=TSize };

	FUIIndexBuffer(FRenderInterface* RI)
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


/*-----------------------------------------------------------------------------
	FVertexStreamChunk.
-----------------------------------------------------------------------------*/

template<INT TNumVertices,INT TNumIndices>
struct FVertexStreamChunk
{
	enum { VERTICES=TNumVertices, INDICES=TNumIndices };

	FVertexStreamChunk(UMaterial* InMaterial,DWORD InPolyFlags,FUIVertex* VBStart,_WORD* IBStart,INT BaseIndex)
	:	Material(InMaterial), PolyFlags(InPolyFlags)
	,	VBIndex(VBStart), IBIndex(IBStart)
	,	VertexIndex(BaseIndex)
	,	VertexCount(0), IndexCount(0)
	{}

	FVertexStreamChunk()
	{}

	~FVertexStreamChunk()
	{}

	INT AddVertex(FUIVertex* &Vertex)
	{
		Vertex = VBIndex++;
		VertexCount++;
		NOTE(debugf(TEXT("FVertexStreamChunk<%d,%d>::AddVertex: %d %d %d"),VERTICES,INDICES,(DWORD)Vertex,VertexCount,VertexIndex));
		return VertexIndex++;
	}
	void AddIndex(INT Index)
	{
		*(IBIndex++) = Index;
		IndexCount++;
		NOTE(debugf(TEXT("FVertexStreamChunk<%d,%d>::AddIndex: %d %d"),VERTICES,INDICES,IndexCount,Index));
	}

	INT GetVertexCount()	{ return VertexCount;			}
	INT GetIndexCount()		{ return IndexCount;			}
	INT GetTriangleCount()	{ return IndexCount/3;			}

	UBOOL HasRoom(INT RequiredVertices,INT RequiredIndices)
	{
		return	GetVertexCount()+RequiredVertices <= VERTICES
			&&	GetIndexCount()+RequiredIndices <= INDICES;
	}

	UMaterial* Material;
	DWORD PolyFlags;

	FUIVertex*	VBIndex;
	_WORD*		IBIndex;

	INT VertexIndex;
	INT VertexCount,IndexCount;
};


/*-----------------------------------------------------------------------------
	ChunkHash.
-----------------------------------------------------------------------------*/

class ChunkHash
{
public:
	DWORD Hash;
	ChunkHash(UMaterial* Material,DWORD PolyFlags)
	:	Hash((DWORD)Material^PolyFlags)
	{}
	operator DWORD() const { return Hash; }
};
inline DWORD GetTypeHash( const ChunkHash A )
{
	return A;
}


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

