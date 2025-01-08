/*=============================================================================
	UnTemplate.h: Unreal templates.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Type information.
-----------------------------------------------------------------------------*/

#include <string.h>

//
// Type information for initialization.
//
template <class T> struct TTypeInfoBase
{
public:
	typedef const T& ConstInitType;
	static UBOOL NeedsDestructor() {return 1;}
	static UBOOL DefinitelyNeedsDestructor() {return 0;}
	static const T& ToInit( const T& In ) {return In;}
};
template <class T> struct TTypeInfo : public TTypeInfoBase<T>
{
};

template <> struct TTypeInfo<BYTE> : public TTypeInfoBase<BYTE>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<SBYTE> : public TTypeInfoBase<SBYTE>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<ANSICHAR> : public TTypeInfoBase<ANSICHAR>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<INT> : public TTypeInfoBase<INT>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<DWORD> : public TTypeInfoBase<DWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<_WORD> : public TTypeInfoBase<_WORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<SWORD> : public TTypeInfoBase<SWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<QWORD> : public TTypeInfoBase<QWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<SQWORD> : public TTypeInfoBase<SQWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<FName> : public TTypeInfoBase<FName>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<UObject*> : public TTypeInfoBase<UObject*>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};

/*-----------------------------------------------------------------------------
	Standard templates.
-----------------------------------------------------------------------------*/

template< class T > inline T Abs( const T A )
{
	return (A>=(T)0) ? A : -A;
}
template< class T > inline T Sgn( const T A )
{
	return (A>0) ? 1 : ((A<0) ? -1 : 0);
}
template< class T > inline T Max( const T A, const T B )
{
	return (A>=B) ? A : B;
}
template< class T > inline T Min( const T A, const T B )
{
	return (A<=B) ? A : B;
}
template< class T > inline T Max3( const T A, const T B, const T C )
{
	return Max ( Max( A, B ), C );
}
template< class T > inline T Min3( const T A, const T B, const T C )
{
	return Min ( Min( A, B ), C );
}
template< class T > inline T Square( const T A )
{
	return A*A;
}
template< class T > inline T Clamp( const T X, const T Min, const T Max )
{
	return X<Min ? Min : X<Max ? X : Max;
}
template< class T > inline T Align( const T Ptr, INT Alignment )
{
	return (T)(((DWORD)Ptr + Alignment - 1) & ~(Alignment-1));
}
template< class T > inline void Exchange( T& A, T& B )
{
	const T Temp = A;
	A = B;
	B = Temp;
}
template< class T > T Lerp( T& A, T& B, FLOAT Alpha )
{
	return (T)(A + Alpha * (B-A));
}
inline DWORD GetTypeHash( const BYTE A )
{
	return A;
}
inline DWORD GetTypeHash( const SBYTE A )
{
	return A;
}
inline DWORD GetTypeHash( const _WORD A )
{
	return A;
}
inline DWORD GetTypeHash( const SWORD A )
{
	return A;
}
inline DWORD GetTypeHash( const INT A )
{
	return A;
}
inline DWORD GetTypeHash( const DWORD A )
{
	return A;
}
inline DWORD GetTypeHash( const QWORD A )
{
	return (DWORD)A+((DWORD)(A>>32) * 23);
}
inline DWORD GetTypeHash( const SQWORD A )
{
	return (DWORD)A+((DWORD)(A>>32) * 23);
}
inline DWORD GetTypeHash( const TCHAR* S )
{
	return appStrihash(S);
}
#define ExchangeB(A,B) {UBOOL T=A; A=B; B=T;}

/*----------------------------------------------------------------------------
	Standard macros.
----------------------------------------------------------------------------*/

// Number of elements in an array.
#define ARRAY_COUNT( array ) \
	( sizeof(array) / sizeof((array)[0]) )

// Offset of a struct member.
#define STRUCT_OFFSET( struc, member ) \
	( (INT)&((struc*)NULL)->member )

/*-----------------------------------------------------------------------------
	Allocators.
-----------------------------------------------------------------------------*/

template <class T> class TAllocator
{};

/*-----------------------------------------------------------------------------
	Dynamic array template.
-----------------------------------------------------------------------------*/

//
// Base dynamic array.
//
class CORE_API FArray
{
public:
	void* GetData()
	{
		return Data;
	}
	const void* GetData() const
	{
		return Data;
	}
	UBOOL IsValidIndex( INT i ) const
	{
		return i>=0 && i<ArrayNum;
	}
	FORCEINLINE INT Num() const
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		return ArrayNum;
	}
	void InsertZeroed( INT Index, INT Count, INT ElementSize )
	{
		guardSlow(FArray::InsertZeroed);
		Insert( Index, Count, ElementSize );
		appMemzero( (BYTE*)Data+Index*ElementSize, Count*ElementSize );
		unguardSlow;
	}
	void Insert( INT Index, INT Count, INT ElementSize )
	{
		guardSlow(FArray::Insert);
		checkSlow(Count>=0);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		checkSlow(Index>=0);
		checkSlow(Index<=ArrayNum);

		INT OldNum = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = ArrayNum + 3*ArrayNum/8 + 32;
			Realloc( ElementSize );
		}
		appMemmove
		(
			(BYTE*)Data + (Index+Count )*ElementSize,
			(BYTE*)Data + (Index       )*ElementSize,
			              (OldNum-Index)*ElementSize
		);

		unguardSlow;
	}
	INT Add( INT Count, INT ElementSize )
	{
		guardSlow(FArray::Add);
		checkSlow(Count>=0);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);

		INT Index = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = ArrayNum + 3*ArrayNum/8 + 32;
			Realloc( ElementSize );
		}

		return Index;
		unguardSlow;
	}
	INT AddZeroed( INT ElementSize, INT n=1 )
	{
		guardSlow(FArray::AddZeroed);
		INT Index = Add( n, ElementSize );
		appMemzero( (BYTE*)Data+Index*ElementSize, n*ElementSize );
		return Index;
		unguardSlow;
	}
	void Shrink( INT ElementSize )
	{
		guardSlow(FArray::Shrink);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		if( ArrayMax != ArrayNum )
		{
			ArrayMax = ArrayNum;
			Realloc( ElementSize );
		}
		unguardSlow;
	}
	void Empty( INT ElementSize, INT Slack=0 )
	{
		guardSlow(FArray::Empty);
		ArrayNum = 0;
		ArrayMax = Slack;
		Realloc( ElementSize );
		unguardSlow;
	}
	FArray()
	:	ArrayNum( 0 )
	,	ArrayMax( 0 )
	,	Data	( NULL )
	{}
	FArray( ENoInit )
	{}
	~FArray()
	{
		guardSlow(FArray::~FArray);
		if( Data )
			appFree( Data );
		Data = NULL;
		ArrayNum = ArrayMax = 0;
		unguardSlow;
	}
	void CountBytes( FArchive& Ar, INT ElementSize )
	{
		guardSlow(FArray::CountBytes);
		Ar.CountBytes( ArrayNum*ElementSize, ArrayMax*ElementSize );
		unguardSlow;
	}
	void Remove( INT Index, INT Count, INT ElementSize );
protected:
	void Realloc( INT ElementSize );
	FArray( INT InNum, INT ElementSize )
	:	ArrayNum( InNum )
	,	ArrayMax( InNum )
	,	Data    ( NULL  )
	{
		Realloc( ElementSize );
	}
	void* Data;
	INT	  ArrayNum;
	INT	  ArrayMax;
};

//
// Templated dynamic array.
//
template< class T > class TArray : public FArray
{
public:
	typedef T ElementType;
	TArray()
	:	FArray()
	{}
	TArray( INT InNum )
	:	FArray( InNum, sizeof(T) )
	{}
	TArray( const TArray& Other )
	:	FArray( Other.ArrayNum, sizeof(T) )
	{
		guardSlow(TArray::copyctor);
		if( TTypeInfo<T>::NeedsDestructor() )
		{
			ArrayNum=0;
			for( INT i=0; i<Other.ArrayNum; i++ )
				new(*this)T(Other(i));
		}
		else if( sizeof(T)!=1 )
		{
			for( INT i=0; i<ArrayNum; i++ )
				(*this)(i) = Other(i);
		}
		else
		{
			appMemcpy( &(*this)(0), &Other(0), ArrayNum * sizeof(T) );
		}
		unguardSlow;
	}
	TArray( ENoInit )
	: FArray( E_NoInit )
	{}
	~TArray()
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		Remove( 0, ArrayNum );
	}
    T& operator()( INT i )
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[i];
		unguardSlow;
	}
	const T& operator()( INT i ) const
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[i];
		unguardSlow;
	}
	T Pop()
	{
		guardSlow(TArray::Pop);
		check(ArrayNum>0);
		checkSlow(ArrayMax>=ArrayNum);
		T Result = ((T*)Data)[ArrayNum-1];
		Remove( ArrayNum-1 );
		return Result;
		unguardSlow;
	}
	T& Last( INT c=0 )
	{
		guardSlow(TArray::Last);
		check(c<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[ArrayNum-c-1];
		unguardSlow;
	}
	const T& Last( INT c=0 ) const
	{
		guardSlow(TArray::Last);
		checkSlow(c<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[ArrayNum-c-1];
		unguardSlow;
	}
	void Shrink()
	{
		guardSlow(TArray::Shrink);
		FArray::Shrink( sizeof(T) );
		unguardSlow;
	}
	UBOOL FindItem( const T& Item, INT& Index ) const
	{
		guardSlow(TArray::FindItem);
		for( Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return 1;
		return 0;
		unguardSlow;
	}
	INT FindItemIndex( const T& Item ) const
	{
		guardSlow(TArray::FindItemIndex);
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return INDEX_NONE;
		unguardSlow;
	}
#if __MWERKS__
	friend FArchive& operator<<( FArchive& Ar, TArray& A );
#else
	friend FArchive& operator<<( FArchive& Ar, TArray& A )
	{
		guard(TArray<<);
		A.CountBytes( Ar );
		if( sizeof(T)==1 )
		{
			// Serialize simple bytes which require no construction or destruction.
			Ar << AR_INDEX(A.ArrayNum);
			if( Ar.IsLoading() )
			{
				A.ArrayMax = A.ArrayNum;
				A.Realloc( sizeof(T) );
			}
			Ar.Serialize( &A(0), A.Num() );
		}
		else if( Ar.IsLoading() )
		{
			// Load array.
			INT NewNum;
			Ar << AR_INDEX(NewNum);
			if( NewNum<0 )
			{
				debugf(NAME_Warning,TEXT("Error loading TArray!!  Memory corruption...  Serialization aborted."));
				return Ar;
			}
			A.Empty( NewNum );
			for( INT i=0; i<NewNum; i++ )
				Ar << *new(A)T;
		}
		else
		{
			// Save array.
			Ar << AR_INDEX(A.ArrayNum);
			for( INT i=0; i<A.ArrayNum; i++ )
				Ar << A( i );
		}
		return Ar;
		unguard;
	}
#endif
	void CountBytes( FArchive& Ar )
	{
		guardSlow(TArray::CountBytes);
		FArray::CountBytes( Ar, sizeof(T) );
		unguardSlow;
	}

	// Add, Insert, Remove, Empty interface.
	INT Add( INT n=1 )
	{
		guardSlow(TArray::Add);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		return FArray::Add( n, sizeof(T) );
		unguardSlow;
	}
	void Reserve( INT Count )
	{
		guardSlow(TArray::Reserve);
		if( Count > ArrayNum )
			FArray::Add( (Count - ArrayNum), sizeof(T) );
		unguardSlow;
	}
	void Insert( INT Index, INT Count=1 )
	{
		guardSlow(TArray::Insert);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		FArray::Insert( Index, Count, sizeof(T) );
		unguardSlow;
	}
	void InsertZeroed( INT Index, INT Count=1 )
	{
		guardSlow(TArray::InsertZeroed);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		FArray::InsertZeroed( Index, Count, sizeof(T) );
		unguardSlow;
	}
	void Remove( INT Index, INT Count=1 )
	{
		guardSlow(TArray::Remove);
		check(Index>=0);
		check(Index<=ArrayNum);
		check(Index+Count<=ArrayNum);
		if( TTypeInfo<T>::NeedsDestructor() )
			for( INT i=Index; i<Index+Count; i++ )
				(&(*this)(i))->~T();
		FArray::Remove( Index, Count, sizeof(T) );
		unguardSlow;
	}
	void Empty( INT Slack=0 )
	{
		guardSlow(TArray::Empty);
		if( TTypeInfo<T>::NeedsDestructor() )
			for( INT i=0; i<ArrayNum; i++ )
				(&(*this)(i))->~T();
		FArray::Empty( sizeof(T), Slack );
		unguardSlow;
	}

	// Functions dependent on Add, Remove.
	TArray& operator+( const TArray& Other )
	{
		guardSlow(TArray::operator=);
		if( this != &Other )
		{
			for( INT i=0; i<Other.ArrayNum; i++ )
				new( *this )T( Other(i) );
		}
		return *this;
		unguardSlow;
	}
	TArray& operator+=( const TArray& Other )
	{
		guardSlow(TArray::operator=);
		if( this != &Other )
		{
			*this = *this + Other;
		}
		return *this;
		unguardSlow;
	}
	TArray& operator=( const TArray& Other )
	{
		guardSlow(TArray::operator=);
		if( this != &Other )
		{
			Empty( Other.ArrayNum );
			for( INT i=0; i<Other.ArrayNum; i++ )
				new( *this )T( Other(i) );
		}
		return *this;
		unguardSlow;
	}
	INT AddItem( const T& Item )
	{
		guardSlow(TArray::AddItem);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		INT Index=Add();
		(*this)(Index)=Item;
		return Index;
		unguardSlow;
	}
	INT AddZeroed( INT n=1 )
	{
		guardSlow(TArray::AddZeroed);
		return FArray::AddZeroed( sizeof(T), n );
		unguardSlow;
	}
	INT AddUniqueItem( const T& Item )
	{
		guardSlow(TArray::AddUniqueItem);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return AddItem( Item );
		unguardSlow;
	}
	INT RemoveItem( const T& Item )
	{
		guardSlow(TArray::RemoveItem);
		INT OriginalNum=ArrayNum;
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				Remove( Index-- );
		return OriginalNum - ArrayNum;
		unguardSlow;
	}
	UBOOL operator==( const TArray& Other ) const
	{
		guardSlow(TArray::operator==);
		if( ArrayNum!=Other.Num() )
			return false;
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)!=Other(Index) )
				return false;
		return true;
		unguardSlow;
	}
	UBOOL operator!=( const TArray& Other ) const
	{
		guardSlow(TArray::operator!=);
		return !((*this)==Other);
		unguardSlow;
	}

	// Iterator.
	class TIterator
	{
	public:
		TIterator( TArray<T>& InArray ) : Array(InArray), Index(-1) { ++*this;      }
		void operator++()      { ++Index;                                           }
		void RemoveCurrent()   { Array.Remove(Index--); }
		INT GetIndex()   const { return Index;                                      }
		operator UBOOL() const { return Index < Array.Num();                        }
		T& operator*()   const { return Array(Index);                               }
		T* operator->()  const { return &Array(Index);                              }
		T& GetCurrent()  const { return Array( Index );                             }
		T& GetPrev()     const { return Array( Index ? Index-1 : Array.Num()-1 );   }
		T& GetNext()     const { return Array( Index<Array.Num()-1 ? Index+1 : 0 ); }
	private:
		TArray<T>& Array;
		INT Index;
	};
};

#if __MWERKS__
template< class T > inline FArchive& operator<<( FArchive& Ar, TArray<T>& A )
{
	guard(TArray<<);
	A.CountBytes( Ar );
	if( sizeof(T)==1 )
	{
		// Serialize simple bytes which require no construction or destruction.
		Ar << AR_INDEX(A.ArrayNum);
		if( Ar.IsLoading() )
		{
			A.ArrayMax = A.ArrayNum;
			A.Realloc( sizeof(T) );
		}
		Ar.Serialize( &A(0), A.Num() );
	}
	else if( Ar.IsLoading() )
	{
		// Load array.
		INT NewNum;
		Ar << AR_INDEX(NewNum);
		A.Empty( NewNum );
		for( INT i=0; i<NewNum; i++ )
			Ar << *new(A)T;
	}
	else
	{
		// Save array.
		Ar << AR_INDEX(A.ArrayNum);
		for( INT i=0; i<A.ArrayNum; i++ )
			Ar << A( i );
	}
	return Ar;
	unguard;
}

// Since the above doesn't seem to actually do anything, I'm also including this
// handy macro to make manually instantiating these things easier.

#define INSTANTIATE_TEMPLATE(TypeName) \
	FArchive& operator<<( FArchive& Ar, TArray<TypeName>& A ) \
	{ \
		A.CountBytes( Ar ); \
		if( sizeof(TypeName)==1 ) \
		{ \
			Ar << AR_INDEX(A.ArrayNum); \
			if( Ar.IsLoading() ) \
			{ \
				A.ArrayMax = A.ArrayNum; \
				A.Realloc( sizeof(TypeName) ); \
			} \
			Ar.Serialize( &A(0), A.Num() ); \
		} \
		else if( Ar.IsLoading() ) \
		{ \
			INT NewNum; \
			Ar << AR_INDEX(NewNum); \
			A.Empty( NewNum ); \
			for( INT i=0; i<NewNum; i++ ) \
				Ar << *new(A)TypeName; \
		} \
		else \
		{ \
			Ar << AR_INDEX(A.ArrayNum); \
			for( INT i=0; i<A.ArrayNum; i++ ) \
				Ar << A(i); \
		} \
		return Ar; \
	}

#endif
template<class T> class TArrayNoInit : public TArray<T>
{
public:
	TArrayNoInit()
	: TArray<T>(E_NoInit)
	{}
	TArrayNoInit& operator=( const TArrayNoInit& Other )
	{
		TArray<T>::operator=(Other);
		return *this;
	}
};

//
// Templated dynamic array (ordered based on T::operator<).
// [From highest at Index=0 to lowest at Index=ArrayNum-1]
//
// Note: Every time you add a new item, any stored indices will be invalidated (since
// all items will be resorted accordingly).  As such, it's not wise to store indices 
// returned by AddItem, etc.
//
template< class T > class TOrderedArray : public TArray<T>
{
public:
	TOrderedArray()
	:	TArray<T>()
	{}
	TOrderedArray( ENoInit )
	: TArray<T>( E_NoInit )
	{}
	TOrderedArray& operator=( const TOrderedArray& Other )
	{
		TArray<T>::operator=(Other);
		return *this;
	}
#if 0
    T& operator()( INT i )
	{
		guard(TOrderedArray::operator());
		#error "Use const operator instead.  Items should not be modified externally without using AddItem or RemoveItem."
		return TArray<T>::(i);
		unguard;
	}
#endif
	INT AddItem( const T& Item )
	{
		guardSlow(TOrderedArray::AddItem);
		INT Index=TArray<T>::AddItem(Item);
		return Percolate(Index);
		unguardSlow;
	}
	void Sort()
	{
		guardSlow(TOrderedArray::Sort);
		// Not exactly a perc-sort, but close enough.
		for( INT i=0; i<ArrayNum; i++ )
			Percolate( i );
		unguardSlow;
	}
private:
	// Does a single pass of a perc-sort starting at index i.
	// Assumes: All Elements from 0 to i-1 are already in order.
	// Requires: T::operator< has been defined.
	INT Percolate( INT i )
	{
		guardSlow(TOrderedArray::Percolate);
		for( ; i>0; i-- )
		{
			if( ((T*)Data)[i-1]->IsLessThan( ((T*)Data)[i] ) )
				ExchangeItem( i-1, i );
			else
				return i;
		}
		return 0;
		unguardSlow;
	}
	void ExchangeItem( INT A, INT B )
	{
		guardSlow(TOrderedArray::ExchangeItem);
		checkSlow(A>=0);
		checkSlow(A<=ArrayNum);
		checkSlow(B>=0);
		checkSlow(B<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		if( A!=B )
		{
			T Temp=((T*)Data)[A];
			((T*)Data)[A]=((T*)Data)[B];
			((T*)Data)[B]=Temp;
		}
		unguardSlow;
	}
};

template< class T > class TOrderedArrayNoInit : public TOrderedArray<T>
{
public:
	TOrderedArrayNoInit()
	: TOrderedArray<T>(E_NoInit)
	{}
	TOrderedArrayNoInit& operator=( const TOrderedArrayNoInit& Other )
	{
		TOrderedArray<T>::operator=(Other);
		return *this;
	}
};

//
// Array operator news.
//
template <class T> void* operator new( size_t Size, TArray<T>& Array )
{
	guardSlow(TArray::operator new);
	INT Index = Array.FArray::Add(1,sizeof(T));
	return &Array(Index);
	unguardSlow;
}
template <class T> void* operator new( size_t Size, TArray<T>& Array, INT Index )
{
	guardSlow(TArray::operator new);
	Array.FArray::Insert(Index,1,sizeof(T));
	return &Array(Index);
	unguardSlow;
}

#if __GNUG__
//!!vogel: gcc needs a little help (explicit instantiation) in this case.
template void* operator new( size_t Size, TArray<FString>& Array );
#endif

//
// Array exchanger.
//
template <class T> inline void ExchangeArray( TArray<T>& A, TArray<T>& B )
{
	guardSlow(ExchangeTArray);
	appMemswap( &A, &B, sizeof(FArray) );
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Transactional array.
-----------------------------------------------------------------------------*/

template< class T > class TTransArray : public TArray<T>
{
public:
	// Constructors.
	TTransArray( UObject* InOwner, INT InNum=0 )
	:	TArray<T>( InNum )
	,	Owner( InOwner )
	{
		checkSlow(Owner);
	}
	TTransArray( UObject* InOwner, const TArray<T>& Other )
	:	TArray<T>( Other )
	,	Owner( InOwner )
	{
		checkSlow(Owner);
	}
	TTransArray& operator=( const TTransArray& Other )
	{
		operator=( (const TArray<T>&)Other );
		return *this;
	}

	// Add, Insert, Remove, Empty interface.
	INT Add( INT Count=1 )
	{
		guardSlow(TTransArray::Add);
		INT Index = TArray<T>::Add( Count );
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		return Index;
		unguardSlow;
	}
	void Insert( INT Index, INT Count=1 )
	{
		guardSlow(TTransArray::InsertZeroed);
		FArray::Insert( Index, Count, sizeof(T) );
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		unguardSlow;
	}
	void Remove( INT Index, INT Count=1 )
	{
		guardSlow(TTransArray::Remove);
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, -1, sizeof(T), SerializeItem, DestructItem );
		TArray<T>::Remove( Index, Count );
		unguardSlow;
	}
	void Empty( INT Slack=0 )
	{
		guardSlow(TTransArray::Empty);
		if( GUndo )
			GUndo->SaveArray( Owner, this, 0, ArrayNum, -1, sizeof(T), SerializeItem, DestructItem );
		TArray<T>::Empty( Slack );
		unguardSlow;
	}

	// Functions dependent on Add, Remove.
	TTransArray& operator=( const TArray<T>& Other )
	{
		guardSlow(TTransArray::operator=);
		if( this != &Other )
		{
			Empty( Other.Num() );
			for( INT i=0; i<Other.Num(); i++ )
				new( *this )T( Other(i) );
		}
		return *this;
		unguardSlow;
	}
	INT AddItem( const T& Item )
	{
		guardSlow(TTransArray::AddItem);
		INT Index=Add();
		(*this)(Index)=Item;
		return Index;
		unguardSlow;
	}
	INT AddZeroed( INT n=1 )
	{
		guardSlow(TTransArray::AddZeroed);
		INT Index = Add(n);
		appMemzero( &(*this)(Index), n*sizeof(T) );
		return Index;
		unguardSlow;
	}
	INT AddUniqueItem( const T& Item )
	{
		guardSlow(TTransArray::AddUniqueItem);
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return AddItem( Item );
		unguardSlow;
	}
	INT RemoveItem( const T& Item )
	{
		guardSlow(TTransArray::RemoveItem);
		INT OriginalNum=ArrayNum;
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				Remove( Index-- );
		return OriginalNum - ArrayNum;
		unguardSlow;
	}

	// TTransArray interface.
	UObject* GetOwner()
	{
		return Owner;
	}
	void ModifyItem( INT Index )
	{
		guardSlow(TTransArray::ModifyItem);
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, 1, 0, sizeof(T), SerializeItem, DestructItem );
		unguardSlow;
	}
	void ModifyAllItems()
	{
		guardSlow(TTransArray::ModifyAllItems);
		if( GUndo )
			GUndo->SaveArray( Owner, this, 0, Num(), 0, sizeof(T), SerializeItem, DestructItem );
		unguardSlow;
	}
	friend FArchive& operator<<( FArchive& Ar, TTransArray& A )
	{
		guard(TTransArray<<);
		if( !Ar.IsTrans() )
			Ar << (TArray<T>&)A;
		return Ar;
		unguard;
	}
protected:
	static void SerializeItem( FArchive& Ar, void* TPtr )
	{
		guardSlow(TArray::SerializeItem);
		Ar << *(T*)TPtr;
		unguardSlow;
	}
	static void DestructItem( void* TPtr )
	{
		guardSlow(TArray::SerializeItem);
		((T*)TPtr)->~T();
		unguardSlow;
	}
	UObject* Owner;
private:

	// Disallow the copy constructor.
	TTransArray( const TArray<T>& Other )
	{}
};

//
// Transactional array operator news.
//
template <class T> void* operator new( size_t Size, TTransArray<T>& Array )
{
	guardSlow(TArray::operator new);
	INT Index = Array.Add();
	return &Array(Index);
	unguardSlow;
}
template <class T> void* operator new( size_t Size, TTransArray<T>& Array, INT Index )
{
	guardSlow(TArray::operator new);
	Array.Insert(Index);
	return &Array(Index);
	unguardSlow;
}

/*----------------------------------------------------------------------------
	THeap.	 -- Paul DuBois, InfiniteMachine
----------------------------------------------------------------------------*/

//
// A heap, useful for implementing a priority queue.
// Requires a function matching bool operator<(const T&, const T&);
//
// Implemented with an FArray with a dummy sentinel element 0 to make
// the tree-numbering work out cleanly.  It's nicer if the root of the
// tree is at element 1 instead of element 0.
//
template< class T > class THeap : private FArray
{
public:
	typedef T ElementType;
	THeap()
	:	FArray()
	{}
	THeap( ENoInit )
	:	FArray( E_NoInit )
	{}
    T& operator()( INT i )
	{
		guardSlow(THeap::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[i];
		unguardSlow;
	}
	const T& operator()( INT i ) const
	{
		guardSlow(THeap::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[i];
		unguardSlow;
	}
	T& Top()
	{
		guard(THeap::Top);
		check(FArray::Num()>1);
		return ((T*)Data)[1];
		unguard;
	}
	bool IsEmpty() const
	{
		guard(THeap::IsEmpty);
		return FArray::Num()<=1;
		unguard;
	}
	void RemoveTop()
	{
		guard(THeap::RemoveTop);
		if( !IsEmpty() )
			HeapRemove(1);
		unguard;
	}
	void ReplaceTop( T& Item )
	{
		guard(THeap::ReplaceTop);
		check(FArray::Num()>1);
		Elt(1) = Item;
		fixDown(1);
		unguard;
	}
	void Add( const T& Item )
	{
		guard(THeap::Add);
		if( FArray::Num()==0 )
			FArray::Add(1,sizeof(T)); // sentinel
		int idx = FArray::Add(1,sizeof(T));
		Elt(idx) = Item;
		fixUp(idx);
		unguard;
	}
	INT BaseAdd( INT Count, INT ElementSize )	// used to get around compiler error C2248.
	{
		guardSlow(THeap::BaseAdd);
		return FArray::Add(Count,ElementSize);
		unguardSlow;
	}
	INT Num() // FArray members are private within THeap
	{
		return FArray::Num();
	}
	friend FArchive& operator<<( FArchive& Ar, THeap& A )
	{
		guard(THeap<<);
		A.CountBytes( Ar );
		if( sizeof(T)==1 )
		{
			// Serialize simple bytes which require no construction or destruction.
			Ar << AR_INDEX(A.ArrayNum);
			if( Ar.IsLoading() )
			{
				A.ArrayMax = A.ArrayNum;
				A.Realloc( sizeof(T) );
			}
			Ar.Serialize( &A(0), A.FArray::Num() );
		}
		else if( Ar.IsLoading() )
		{
			// Load array.
			INT NewNum;
			Ar << AR_INDEX(NewNum);
			A.Empty( NewNum );
			A.AddZeroed( sizeof(T), 1 ); //NEW (mdf) account for sentinel entry (or we'll be off by one)
			for( INT i=1; i<NewNum; i++ )		// don't read sentinel
				Ar << *new(A)T;
		}
		else
		{
			// Save array.
			Ar << AR_INDEX(A.ArrayNum);
			for( INT i=1; i<A.ArrayNum; i++ )	// don't save sentinel
				Ar << A( i );
		}
		return Ar;
		unguard;
	}
	void CountBytes( FArchive& Ar )
	{
		guardSlow(THeap::CountBytes);
		FArray::CountBytes( Ar, sizeof(T) );
		unguardSlow;
	}
	class TUnsortedIterator
	{
	public:
		TUnsortedIterator( THeap<T>& InHeap ) : Heap(InHeap), Index(0) { ++*this;				}
		void operator++()	   { ++Index;														}
#if 1 //NEW (mdf) tbd: slightly expensive fix for iterator problem (see FTimerManager.cpp)
		void RemoveCurrent()   { Heap.HeapRemove(Index); Index=0;								}
#else		
		void RemoveCurrent()   { Heap.HeapRemove(Index--);										}
#endif
		operator UBOOL() const { return Index < Heap.FArray::Num(); 							}
		T& operator*()	 const { return Heap.Elt(Index);										}
		T* operator->()  const { return &Heap.Elt(Index);										}
		T& GetCurrent()  const { return Heap.Elt( Index );										}
		T& GetPrev()	 const { return Heap.Elt( Index>1 ? Index-1 : Heap.FArray::Num()-1);	}
		T& GetNext()	 const { return Heap.Elt( Index<Heap.FArray::Num()-1 ? Index+1 : 1 );	}
	private:
		THeap<T>& Heap;
		INT Index;
	};
	friend class TUnsortedIterator;

protected:
	T& Elt(INT i)
	{
		guard(THeap::Elt);
		check/*Slow*/(i>0); // not >= 0 because why would we look at the sentinel?
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[i];
		unguard;
	}
	void fixUp( INT k ) 	// Fix heap upwards from element k
	{
		guard(THeap::fixUp);
		for( ; k>1 && Elt(k) < Elt(k/2); k/=2 )
			appMemswap(&Elt(k), &Elt(k/2), sizeof(T));
		unguard;
	}
	void fixDown( INT k )	// Fix heap downwards from element k
	{
		guard(THeap::fixDown);
		while( 2*k < FArray::Num() )
		{
			int child = 2*k;
			if( child+1 < FArray::Num() && Elt(child+1) < Elt(child) )
				child++;
			if( !(Elt(child) < Elt(k)) )
				break;
			appMemswap(&Elt(child), &Elt(k), sizeof(T));
			k = child;
		}
		unguard;
	}
	// Remove by filling in hole with last element
	void HeapRemove(INT Index)
	{
		guard(THeap::HeapRemove);
		check(Index<FArray::Num());
		T& Last = Elt(FArray::Num()-1);
		ArrayNum--;
		if( Index<FArray::Num() )
		{
			Elt(Index) = Last;
			fixUp(Index);
			fixDown(Index);
		}
		unguard;
	}
};

//
// Heap operator news.
//
template <class T> void* operator new( size_t Size, THeap<T>& Heap )
{
	guardSlow(THeap::operator new);
	INT Index = Heap.BaseAdd(1,sizeof(T));
	return &Heap(Index);
	unguardSlow;
}
template <class T> void* operator new( size_t Size, THeap<T>& Heap, INT Index )
{
	guardSlow(THeap::operator new);
	Heap.FArray::Insert(Index,1,sizeof(T));
	return &Heap(Index);
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Lazy loading.
-----------------------------------------------------------------------------*/

//
// Lazy loader base class.
//
class FLazyLoader
{
	friend class ULinkerLoad;
protected:
	FArchive*	 SavedAr;
	INT          SavedPos;
public:
	FLazyLoader()
	: SavedAr( NULL )
	, SavedPos( 0 )
	{}
	virtual void Load()=0;
	virtual void Unload()=0;
};

//
// Lazy-loadable dynamic array.
//
template <class T> class TLazyArray : public TArray<T>, public FLazyLoader
{
public:
	TLazyArray( INT InNum=0 )
	: TArray<T>( InNum )
	, FLazyLoader()
	{}
	~TLazyArray()
	{
		guard(TLazyArray::~TLazyArray);
		if( SavedAr )
			SavedAr->DetachLazyLoader( this );
		unguard;
	}
#if LOAD_ON_DEMAND /* Breaks because of untimely accesses of operator() !! */
    T& operator()( INT i )
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		if( SavedPos>0 )
			Load();
		return ((T*)Data)[i];
		unguardSlow;
	}
	const T& operator()( INT i ) const
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		if( SavedPos>0 )
			Load();
		return ((T*)Data)[i];
		unguardSlow;
	}
#endif
	void Load()
	{
		// Make sure this array is loaded.
		guard(TLazyArray::Load);
		if( SavedPos>0 )
		{
			// Lazy load it now.
			INT PushedPos = SavedAr->Tell();
			SavedAr->Seek( SavedPos );
			*SavedAr << (TArray<T>&)*this;
			SavedPos *= -1;
			SavedAr->Seek( PushedPos );
		}
		unguard;
	}
	void Unload()
	{
		// Make sure this array is unloaded.
		guard(TLazyArray::Unload);
		if( SavedPos<0 )
		{
			// Unload it now.
			Empty();
			SavedPos *= -1;
		}
		unguard;
	}
	 
	friend FArchive& operator<<( FArchive& Ar, TLazyArray& This )
	{
		guard(TLazyArray<<);
		if( Ar.IsLoading() )
		{
			INT SeekPos=0;
			if( Ar.Ver() <= 61 )
			{
				//oldver: Handles dynamic arrays of fixed-length serialized items only.
				Ar.AttachLazyLoader( &This );
				INT SkipCount;
				Ar << AR_INDEX(SkipCount);
				SeekPos = Ar.Tell() + SkipCount*sizeof(T);
					
			}
			else
			{
				Ar << SeekPos;
				if( GUglyHackFlags & 8 )
					Ar << (TArray<T>&)This;		
				else
					Ar.AttachLazyLoader( &This );
			}
			if( !GLazyLoad )
				This.Load();
			Ar.Seek( SeekPos );
		}
		else if( Ar.IsSaving() && Ar.Ver()>61 )
		{
			// Save out count for skipping over this data.
			INT CountPos = Ar.Tell();
			Ar << CountPos << (TArray<T>&)This;
			INT EndPos = Ar.Tell();
			Ar.Seek( CountPos );
			Ar << EndPos;
			Ar.Seek( EndPos );
		}
		else Ar << (TArray<T>&)This;
		return Ar;
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	Dynamic strings.
-----------------------------------------------------------------------------*/

//
// A dynamically sizeable string.
//
class CORE_API FString : protected TArray<TCHAR>
{
public:
	FString()
	: TArray<TCHAR>()
	{}
	FString( const FString& Other )
	: TArray<TCHAR>( Other.ArrayNum )
	{
		if( ArrayNum )
			appMemcpy( &(*this)(0), &Other(0), ArrayNum*sizeof(TCHAR) );
	}
	FString( const TCHAR* In )
	: TArray<TCHAR>( *In ? (appStrlen(In)+1) : 0 )
	{
		if( ArrayNum )
			appMemcpy( &(*this)(0), In, ArrayNum*sizeof(TCHAR) );
	}
#ifdef UNICODE // separate this out if ANSICHAR != UNICHAR
	FString( const ANSICHAR* In )
	: TArray<TCHAR>( *In ? (strlen(In)+1) : 0 )
    {
		if( ArrayNum )
			appMemcpy(&(*this)(0), ANSI_TO_TCHAR(In), ArrayNum*sizeof(TCHAR));
    }
#endif
	FString( ENoInit )
	: TArray<TCHAR>( E_NoInit )
	{}
	explicit FString( BYTE   Arg, INT Digits=1 );
	explicit FString( SBYTE  Arg, INT Digits=1 );
	explicit FString( _WORD  Arg, INT Digits=1 );
	explicit FString( SWORD  Arg, INT Digits=1 );
	explicit FString( INT    Arg, INT Digits=1 );
	explicit FString( DWORD  Arg, INT Digits=1 );
	explicit FString( FLOAT  Arg, INT Digits=1, INT RightDigits=0, UBOOL LeadZero=1 );
	explicit FString( DOUBLE Arg, INT Digits=1, INT RightDigits=0, UBOOL LeadZero=1 );
	FString& operator=( const TCHAR* Other )
	{
		if( &(*this)(0)!=Other )
		{
			ArrayNum = ArrayMax = *Other ? appStrlen(Other)+1 : 0;
			Realloc( sizeof(TCHAR) );
			if( ArrayNum )
				appMemcpy( &(*this)(0), Other, ArrayNum*sizeof(TCHAR) );
		}
		return *this;
	}
	FString& operator=( const FString& Other )
	{
		if( this != &Other )
		{
			ArrayNum = ArrayMax = Other.Num();
			Realloc( sizeof(TCHAR) );
			if( ArrayNum )
				appMemcpy( &(*this)(0), *Other, ArrayNum*sizeof(TCHAR) );
		}
		return *this;
	}
    TCHAR& operator[]( INT i )
	{
		guardSlow(FString::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((TCHAR*)Data)[i];
		unguardSlow;
	}
	const TCHAR& operator[]( INT i ) const
	{
		guardSlow(FString::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((TCHAR*)Data)[i];
		unguardSlow;
	}

	~FString()
	{
		TArray<TCHAR>::Empty();		
	}
	void Empty()
	{
		TArray<TCHAR>::Empty();
	}
	void Shrink()
	{
		TArray<TCHAR>::Shrink();
	}
	const TCHAR* operator*() const
	{
		return Num() ? &(*this)(0) : TEXT("");
	}
	operator UBOOL() const
	{
		return Num()!=0;
	}
	TArray<TCHAR>& GetCharArray()
	{
		//warning: Operations on the TArray<CHAR> can be unsafe, such as adding
		// non-terminating 0's or removing the terminating zero.
		return (TArray<TCHAR>&)*this;
	}
	FString& operator+=( const TCHAR* Str )
	{
		if( *Str != '\0' )
		{
		    if( ArrayNum )
		    {
			    INT Index = ArrayNum-1;
			    Add( appStrlen(Str) );
			    appStrcpy( &(*this)(Index), Str );
		    }
		    else if( *Str )
		    {
			    Add( appStrlen(Str)+1 );
			    appStrcpy( &(*this)(0), Str );
		    }
		}
		return *this;
	}
	FString& operator+=( const FString& Str )
	{
		return operator+=( *Str );
	}
	FString operator+( const TCHAR* Str )
	{
		return FString( *this ) += Str;
	}
	FString operator+( const FString& Str )
	{
		return operator+( *Str );
	}
	FString& operator*=( const TCHAR* Str )
	{
		if( ArrayNum>1 && (*this)(ArrayNum-2)!=PATH_SEPARATOR[0] )
			*this += PATH_SEPARATOR;
		return *this += Str;
	}
	FString& operator*=( const FString& Str )
	{
		return operator*=( *Str );
	}
	FString operator*( const TCHAR* Str ) const
	{
		return FString( *this ) *= Str;
	}
	FString operator*( const FString& Str ) const
	{
		return operator*( *Str );
	}
	UBOOL operator<=( const TCHAR* Other ) const
	{
		return !(appStricmp( **this, Other ) > 0);
	}
	UBOOL operator<=( const FString& Other ) const
	{
		return !(appStricmp( **this, *Other ) > 0);
	}
	UBOOL operator<( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other ) < 0;
	}
	UBOOL operator<( const FString& Other ) const
	{
		return appStricmp( **this, *Other ) < 0;
	}
	UBOOL operator>=( const TCHAR* Other ) const
	{
		return !(appStricmp( **this, Other ) < 0);
	}
	UBOOL operator>=( const FString& Other ) const
	{
		return !(appStricmp( **this, *Other ) < 0);
	}
	UBOOL operator>( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other ) > 0;
	}
	UBOOL operator>( const FString& Other ) const
	{
		return appStricmp( **this, *Other ) > 0;
	}
	UBOOL operator==( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other )==0;
	}
	UBOOL operator==( const FString& Other ) const
	{
		return appStricmp( **this, *Other )==0;
	}
	UBOOL operator!=( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other )!=0;
	}
	UBOOL operator!=( const FString& Other ) const
	{
		return appStricmp( **this, *Other )!=0;
	}
	INT Len() const
	{
		return Num() ? Num()-1 : 0;
	}
	FString Left( INT Count ) const
	{
		return FString( Clamp(Count,0,Len()), **this );
	}
	FString LeftChop( INT Count ) const
	{
		return FString( Clamp(Len()-Count,0,Len()), **this );
	}
	FString Right( INT Count ) const
	{
		return FString( **this + Len()-Clamp(Count,0,Len()) );
	}
	FString Mid( INT Start, INT Count=MAXINT ) const
	{
		DWORD End = Start+Count;
		Start    = Clamp( (DWORD)Start, (DWORD)0,     (DWORD)Len() );
		End      = Clamp( (DWORD)End,   (DWORD)Start, (DWORD)Len() );
		return FString( End-Start, **this + Start );
	}
	INT InStr( const TCHAR* SubStr, UBOOL Right=0 ) const
	{
		if( !Right )
		{
			TCHAR* Tmp = appStrstr(**this,SubStr);
			return Tmp ? (Tmp-**this) : -1;
		}
		else
		{
			for( INT i=Len()-1; i>=0; i-- )
			{
				INT j;
				for( j=0; SubStr[j]; j++ )
					if( (*this)(i+j)!=SubStr[j] )
						break;
				if( !SubStr[j] )
					return i;
			}
			return -1;
		}
	}
	INT InStr( const FString& SubStr, UBOOL Right=0 ) const
	{
		return InStr( *SubStr, Right );
	}
	UBOOL Split( const FString& InS, FString* LeftS, FString* RightS, UBOOL Right=0 ) const
	{
		INT InPos = InStr(InS,Right);
		if( InPos<0 )
			return 0;
		if( LeftS )
			*LeftS = Left(InPos);
		if( RightS )
			*RightS = Mid(InPos+InS.Len());
		return 1;
	}
#if 1 //NEW: be *very* careful with these.
	//If they're used on FName strings, they cause GC errors due to the string changing,
	//but not updating it's position in the hashtable
	void Insert( const FString& InS, INT InPos )
	{
		InPos = Clamp( InPos, 0, Len() );
		FString L = Left(InPos);
		FString R = Mid(InPos);
		*this = L + InS + R;
	}
	void Delete( INT InPos, INT Num=1 )
	{
		InPos = Clamp( InPos, 0, Len()-1 );
		*this = Left(InPos) + Mid(InPos+Num);
	}
#endif
	FString Caps() const
	{
		FString New( **this );
		for( INT i=0; i<ArrayNum; i++ )
			New(i) = appToUpper(New(i));
		return New;
	}
	FString Locs() const
	{
		FString New( **this );
		for( INT i=0; i<ArrayNum; i++ )
			New(i) = appToLower(New(i));
		return New;
	}
	FString LeftPad( INT ChCount );
	FString RightPad( INT ChCount );
	static FString Printf( const TCHAR* Fmt, ... );
	static FString Chr( TCHAR Ch );
	CORE_API friend FArchive& operator<<( FArchive& Ar, FString& S );
	friend struct FStringNoInit;
	// Breaks up this delimited string into elements of a string array.
	INT ParseIntoArray( const TCHAR* pchDelim, TArray<FString>* InArray, UBOOL bUnique=0 )
	{
		guard(FString::ParseIntoArray);
		check(InArray);

		FString S = *this;

		if( bUnique )
		{
			INT Index;
			for( INT i = S.InStr( pchDelim ) ; i > 0 ; )
			{
				if( !InArray->FindItem( S.Left(i), Index ) )
					new(*InArray)FString( S.Left(i) );
				S = S.Mid( i + 1, S.Len() );
				i = S.InStr( pchDelim );
			}
			if( !InArray->FindItem( S, Index ) )
				new(*InArray)FString( S );
		}
		else
		{
			for( INT i = S.InStr( pchDelim ) ; i > 0 ; )
			{
				new(*InArray)FString( S.Left(i) );
				S = S.Mid( i + 1, S.Len() );
				i = S.InStr( pchDelim );
			}
			new(*InArray)FString( S );
		}
		return InArray->Num();
		unguard;
	}
	// Reverses the string
	FString Reverse()
	{
		guard(FString::Reverse);
		FString New;
		for( int x = Len()-1 ; x > -1 ; x-- )
			New += Mid(x,1);
		*this = New;
		return New;
		unguard;
	}
	// Takes the number passed in and formats the string in comma format ( 12345 becomes "12,345")
	static FString FormatAsNumber( INT InNumber )
	{
		FString Number = appItoa( InNumber ), Result;

		int dec = 0;
		for( int x = Number.Len()-1 ; x > -1 ; --x )
		{
			Result += Number.Mid(x,1);

			dec++;
			if( dec == 3 && x > 0 )
			{
				Result += TEXT(",");
				dec = 0;
			}
		}

		return Result.Reverse();
	}
private:
	FString( INT InCount, const TCHAR* InSrc )
	:	TArray<TCHAR>( InCount ? InCount+1 : 0 )
	{
		if( ArrayNum )
			appStrncpy( &(*this)(0), InSrc, InCount+1 );
	}
};
struct CORE_API FStringNoInit : public FString
{
	FStringNoInit()
	: FString( E_NoInit )
	{}
	FStringNoInit& operator=( const TCHAR* Other )
	{
		if( &(*this)(0)!=Other )
		{
			ArrayNum = ArrayMax = *Other ? appStrlen(Other)+1 : 0;
			Realloc( sizeof(TCHAR) );
			if( ArrayNum )
				appMemcpy( &(*this)(0), Other, ArrayNum*sizeof(TCHAR) );
		}
		return *this;
	}
	FStringNoInit& operator=( const FString& Other )
	{
		if( this != &Other )
		{
			ArrayNum = ArrayMax = Other.Num();
			Realloc( sizeof(TCHAR) );
			if( ArrayNum )
				appMemcpy( &(*this)(0), *Other, ArrayNum*sizeof(TCHAR) );
		}
		return *this;
	}
	void ClearData()
	{
		ArrayNum = ArrayMax = 0;
		Data = NULL;
	}
};
inline DWORD GetTypeHash( const FString& S )
{
	return appStrihash(*S);
}
template <> struct TTypeInfo<FString> : public TTypeInfoBase<FString>
{
	typedef const TCHAR* ConstInitType;
	static const TCHAR* ToInit( const FString& In ) {return *In;}
	static UBOOL DefinitelyNeedsDestructor() {return 0;}
};

//
// String exchanger.
//
inline void ExchangeString( FString& A, FString& B )
{
	guardSlow(ExchangeTArray);
	appMemswap( &A, &B, sizeof(FString) );
	unguardSlow;
}

/*----------------------------------------------------------------------------
	Special archivers.
----------------------------------------------------------------------------*/

//
// String output device.
//
class FStringOutputDevice : public FString, public FOutputDevice
{
public:
	FStringOutputDevice( const TCHAR* InStr=TEXT("") )
	: FString( InStr )
	{}
	void Serialize( const TCHAR* Data, EName Event )
	{
		*this += (TCHAR*)Data;
	}
};

//
// Buffer writer.
//
class FBufferWriter : public FArchive
{
public:
	FBufferWriter( TArray<BYTE>& InBytes )
	: Bytes( InBytes )
	, Pos( 0 )
	{
		ArIsSaving = 1;
	}
	void Serialize( void* InData, INT Length )
	{
		if( Pos+Length>Bytes.Num() )
			Bytes.Add( Pos+Length-Bytes.Num() );
		appMemcpy( &Bytes(Pos), InData, Length );
		Pos += Length;
	}
	INT Tell()
	{
		return Pos;
	}
	void Seek( INT InPos )
	{
		Pos = InPos;
	}
	INT TotalSize()
	{
		return Bytes.Num();
	}
private:
	TArray<BYTE>& Bytes;
	INT Pos;
};

//
// Buffer archiver.
//
class FBufferArchive : public FBufferWriter, public TArray<BYTE>
{
public:
	FBufferArchive()
	: FBufferWriter( (TArray<BYTE>&)*this )
	{}
};

//
// Buffer reader.
//
class CORE_API FBufferReader : public FArchive
{
public:
	FBufferReader( const TArray<BYTE>& InBytes )
	:	Bytes	( InBytes )
	,	Pos 	( 0 )
	{
		ArIsLoading = ArIsTrans = 1;
	}
	void Serialize( void* Data, INT Num )
	{
		check(Pos>=0);
		check(Pos+Num<=Bytes.Num());
		appMemcpy( Data, &Bytes(Pos), Num );
		Pos += Num;
	}
	INT Tell()
	{
		return Pos;
	}
	INT TotalSize()
	{
		return Bytes.Num();
	}
	void Seek( INT InPos )
	{
		check(InPos>=0);
		check(InPos<=Bytes.Num());
		Pos = InPos;
	}
	UBOOL AtEnd()
	{
		return Pos>=Bytes.Num();
	}
private:
	const TArray<BYTE>& Bytes;
	INT Pos;
};

/*----------------------------------------------------------------------------
	TMap.
----------------------------------------------------------------------------*/

//
// Maps unique keys to values.
//
template< class TK, class TI > class TMapBase
{
protected:
	class TPair
	{
	public:
		INT HashNext;
		TK Key;
		TI Value;
#if __MWERKS__
		TPair( struct TTypeInfo<TK>::ConstInitType InKey, struct TTypeInfo<TI>::ConstInitType InValue )
#else
		TPair( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
#endif
		: Key( InKey ), Value( InValue )
		{}
		TPair()
		{}
#if __MWERKS__
		friend FArchive& operator<<( FArchive& Ar, TPair& F );
#else
		friend FArchive& operator<<( FArchive& Ar, TPair& F )
		{
			guardSlow(TMapBase::TPair<<);
			return Ar << F.Key << F.Value;
			unguardSlow;
		}
#endif
	};
	void Rehash()
	{
		guardSlow(TMapBase::Rehash);
		checkSlow(!(HashCount&(HashCount-1)));
		checkSlow(HashCount>=8);
		INT* NewHash = new(TEXT("HashMapHash"))INT[HashCount];
		{for( INT i=0; i<HashCount; i++ )
		{
			NewHash[i] = INDEX_NONE;
		}}
		{for( INT i=0; i<Pairs.Num(); i++ )
		{
			TPair& Pair    = Pairs(i);
			INT    iHash   = (GetTypeHash(Pair.Key) & (HashCount-1));
			Pair.HashNext  = NewHash[iHash];
			NewHash[iHash] = i;
		}}
		if( Hash )
#if __MWERKS__
			delete[] Hash;
#else
			delete Hash;
#endif
		Hash = NewHash;
		unguardSlow;
	}
	void Relax()
	{
		guardSlow(TMapBase::Relax);
		while( HashCount>Pairs.Num()*2+8 )
			HashCount /= 2;
		Rehash();
		unguardSlow;
	}
#if __MWERKS__
	TI& Add( struct TTypeInfo<TK>::ConstInitType InKey, struct TTypeInfo<TI>::ConstInitType InValue )
#else
	TI& Add( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
#endif
	{
		guardSlow(TMapBase::Add);
		TPair& Pair   = *new(Pairs)TPair( InKey, InValue );
		INT    iHash  = (GetTypeHash(Pair.Key) & (HashCount-1));
		Pair.HashNext = Hash[iHash];
		Hash[iHash]   = Pairs.Num()-1;
		if( HashCount*2+8 < Pairs.Num() )
		{
			HashCount *= 2;
			Rehash();
		}
		return Pair.Value;
		unguardSlow;
	}
	TArray<TPair> Pairs;
	INT* Hash;
	INT HashCount;
public:
	TMapBase()
	:	Hash( NULL )
	,	HashCount( 8 )
	{
		guardSlow(TMapBase::TMapBase);
		Rehash();
		unguardSlow;
	}
	TMapBase( const TMapBase& Other )
	:	Pairs( Other.Pairs )
	,	HashCount( Other.HashCount )
	,	Hash( NULL )
	{
		guardSlow(TMapBase::TMapBase copy);
		Rehash();
		unguardSlow;
	}
	~TMapBase()
	{
		guardSlow(TMapBase::~TMapBase);
		if( Hash )
			delete Hash;
		Hash = NULL;
		HashCount = 0;
		unguardSlow;
	}
	TMapBase& operator=( const TMapBase& Other )
	{
		guardSlow(TMapBase::operator=);
		Pairs     = Other.Pairs;
		HashCount = Other.HashCount;
		Rehash();
		return *this;
		unguardSlow;
	}
	void Empty()
	{
		guardSlow(TMapBase::Empty);
		checkSlow(!(HashCount&(HashCount-1)));
		Pairs.Empty();
		HashCount = 8;
		Rehash();
		unguardSlow;
	}

#if __MWERKS__
	TI& Set( struct TTypeInfo<TK>::ConstInitType InKey, struct TTypeInfo<TI>::ConstInitType InValue )
#else
	TI& Set( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
#endif
	{
		guardSlow(TMap::Set);
		for( INT i=Hash[(GetTypeHash(InKey) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==InKey )
				{Pairs(i).Value=InValue; return Pairs(i).Value;}
		return Add( InKey, InValue );
		unguardSlow;
	}
#if __MWERKS__
	INT Remove( struct TTypeInfo<TK>::ConstInitType InKey )
#else
	INT Remove( typename TTypeInfo<TK>::ConstInitType InKey )
#endif
	{
		guardSlow(TMapBase::Remove);
		INT Count=0;
		for( INT i=Pairs.Num()-1; i>=0; i-- )
			if( Pairs(i).Key==InKey )
				{Pairs.Remove(i); Count++;}
		if( Count )
			Relax();
		return Count;
		unguardSlow;
	}
	TI* Find( const TK& Key )
	{
		guardSlow(TMapBase::Find);
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key )
				return &Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
	TI FindRef( const TK& Key )
	{
		guardSlow(TMapBase::Find);
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key )
				return Pairs(i).Value;
		return (TI) NULL;
		unguardSlow;
	}
	const TI* Find( const TK& Key ) const
	{
		guardSlow(TMapBase::Find);
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key )
				return &Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
	friend FArchive& operator<<( FArchive& Ar, TMapBase& M )
	{
		guardSlow(TMapBase<<);
		Ar << M.Pairs;
		if( Ar.IsLoading() )
			M.Rehash();
		return Ar;
		unguardSlow;
	}
	void Dump( FOutputDevice& Ar )
	{
		guard(TMapBase::Dump);
		Ar.Logf( TEXT("TMapBase: %i items, %i hash slots"), Pairs.Num(), HashCount );
		for( INT i=0; i<HashCount; i++ )
		{
			INT c=0;
			for( INT j=Hash[i]; j!=INDEX_NONE; j=Pairs(j).HashNext )
				c++;
			Ar.Logf( TEXT("   Hash[%i] = %i"), i, c );
		}
		unguard;
	}
	class TIterator
	{
	public:
		TIterator( TMapBase& InMap ) : Map( InMap ), Pairs( InMap.Pairs ), Index( 0 ), Removed(0) {}
		~TIterator()               { if( Removed ) Map.Relax(); }
		void operator++()          { ++Index; }
		void RemoveCurrent()       { Pairs.Remove(Index--); Removed++; }
		operator UBOOL() const     { return Index<Pairs.Num(); }
		TK& Key() const            { return Pairs(Index).Key; }
		TI& Value() const          { return Pairs(Index).Value; }
	private:
		TMapBase& Map;
		TArray<TPair>& Pairs;
		INT Index;
		INT Removed;
	};
	friend class TIterator;
};

#if __MWERKS__
template< class TK, class TI > inline FArchive& operator<<( FArchive& Ar, TMapBase<TK,TI>::TPair& F )
{
	guardSlow(TMapBase::TPair<<);
	return Ar << F.Key << F.Value;
	unguardSlow;
}
#endif

template< class TK, class TI > class TMap : public TMapBase<TK,TI>
{
public:
	TMap& operator=( const TMap& Other )
	{
		TMapBase<TK,TI>::operator=( Other );
		return *this;
	}

	int Num()
	{
		guardSlow(TMap::Num);
		return Pairs.Num();
		unguardSlow;
	}
};
template< class TK, class TI > class TMultiMap : public TMapBase<TK,TI>
{
public:
	TMultiMap& operator=( const TMultiMap& Other )
	{
		TMapBase<TK,TI>::operator=( Other );
		return *this;
	}
	void MultiFind( const TK& Key, TArray<TI>& Values ) 
	{
		guardSlow(TMap::MultiFind);
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key )
				new(Values)TI(Pairs(i).Value);
		unguardSlow;
	}
#if __MWERKS__
	TI& Add( struct TTypeInfo<TK>::ConstInitType InKey, TTypeInfo<TI>::ConstInitType InValue )
#else
	TI& Add( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
#endif
	{
		return TMapBase<TK,TI>::Add( InKey, InValue );
	}
#if __MWERKS__
	TI& AddUnique( struct TTypeInfo<TK>::ConstInitType InKey, struct TTypeInfo<TI>::ConstInitType InValue )
#else
	TI& AddUnique( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
#endif
	{
		for( INT i=Hash[(GetTypeHash(InKey) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==InKey && Pairs(i).Value==InValue )
				return Pairs(i).Value;
		return Add( InKey, InValue );
	}
#if __MWERKS__
	INT RemovePair( struct TTypeInfo<TK>::ConstInitType InKey, struct TTypeInfo<TI>::ConstInitType InValue )
#else
	INT RemovePair( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
#endif
	{
		guardSlow(TMap::Remove);
		INT Count=0;
		for( INT i=Pairs.Num()-1; i>=0; i-- )
			if( Pairs(i).Key==InKey && Pairs(i).Value==InValue )
				{Pairs.Remove(i); Count++;}
		if( Count )
			Relax();
		return Count;
		unguardSlow;
	}
	TI* FindPair( const TK& Key, const TK& Value )
	{
		guardSlow(TMap::Find);
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key && Pairs(i).Value==Value )
				return &Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
};

/*----------------------------------------------------------------------------
	Sorting template.
----------------------------------------------------------------------------*/

//
// Sort elements. The sort is unstable, meaning that the ordering of equal 
// items is not necessarily preserved.
//
template<class T> struct TStack
{
	T* Min;
	T* Max;
};
template<class T> void Sort( T* First, INT Num )
{
	guard(Sort);
	if( Num<2 )
		return;
	TStack<T> RecursionStack[32]={{First,First+Num-1}}, Current, Inner;
	for( TStack<T>* StackTop=RecursionStack; StackTop>=RecursionStack; --StackTop )
	{
		Current = *StackTop;
	Loop:
		INT Count = Current.Max - Current.Min + 1;
		if( Count <= 8 )
		{
			// Use simple bubble-sort.
			while( Current.Max > Current.Min )
			{
				T *Max, *Item;
				for( Max=Current.Min, Item=Current.Min+1; Item<=Current.Max; Item++ )
					if( Compare(*Item, *Max) > 0 )
						Max = Item;
				Exchange( *Max, *Current.Max-- );
			}
		}
		else
		{
			// Grab middle element so sort doesn't exhibit worst-cast behaviour with presorted lists.
			Exchange( Current.Min[Count/2], Current.Min[0] );

			// Divide list into two halves, one with items <=Current.Min, the other with items >Current.Max.
			Inner.Min = Current.Min;
			Inner.Max = Current.Max+1;
			for( ; ; )
			{
				while( ++Inner.Min<=Current.Max && Compare(*Inner.Min, *Current.Min) <= 0 );
				while( --Inner.Max> Current.Min && Compare(*Inner.Max, *Current.Min) >= 0 );
				if( Inner.Min>Inner.Max )
					break;
				Exchange( *Inner.Min, *Inner.Max );
			}
			Exchange( *Current.Min, *Inner.Max );

			// Save big half and recurse with small half.
			if( Inner.Max-1-Current.Min >= Current.Max-Inner.Min )
			{
				if( Current.Min+1 < Inner.Max )
				{
					StackTop->Min = Current.Min;
					StackTop->Max = Inner.Max - 1;
					StackTop++;
				}
				if( Current.Max>Inner.Min )
				{
					Current.Min = Inner.Min;
					goto Loop;
				}
			}
			else
			{
				if( Current.Max>Inner.Min )
				{
					StackTop->Min = Inner  .Min;
					StackTop->Max = Current.Max;
					StackTop++;
				}
				if( Current.Min+1<Inner.Max )
				{
					Current.Max = Inner.Max - 1;
					goto Loop;
				}
			}
		}
	}
	unguard;
}

/*----------------------------------------------------------------------------
	TDoubleLinkedList.
----------------------------------------------------------------------------*/

//
// Simple double-linked list template.
//
template< class T > class TDoubleLinkedList : public T
{
public:
	TDoubleLinkedList* Next;
	TDoubleLinkedList** PrevLink;
	void Unlink()
	{
		if( Next )
			Next->PrevLink = PrevLink;
		*PrevLink = Next;
	}
	void Link( TDoubleLinkedList*& Before )
	{
		if( Before )
			Before->PrevLink = &Next;
		Next     = Before;
		PrevLink = &Before;
		Before   = this;
	}
};

/*----------------------------------------------------------------------------
	TList.
----------------------------------------------------------------------------*/

//
// Simple single-linked list template.
//
template <class ElementType> class TList
{
public:

	ElementType			Element;
	TList<ElementType>*	Next;

	// Constructor.

	TList(ElementType InElement,TList<ElementType>* InNext = NULL)
	{
		Element = InElement;
		Next = InNext;
	}
};

/*----------------------------------------------------------------------------
	FRainbowPtr.
----------------------------------------------------------------------------*/

//
// A union of pointers of all base types.
//
union CORE_API FRainbowPtr
{
	// All pointers.
	void*  PtrVOID;
	BYTE*  PtrBYTE;
	_WORD* PtrWORD;
	DWORD* PtrDWORD;
	QWORD* PtrQWORD;
	FLOAT* PtrFLOAT;

	// Conversion constructors.
	FRainbowPtr() {}
	FRainbowPtr( void* Ptr ) : PtrVOID(Ptr) {};
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
