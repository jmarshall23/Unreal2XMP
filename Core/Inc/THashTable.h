
#ifndef _INC_THASHTABLE_
#define _INC_THASHTABLE_

/*----------------------------------------------------------------------------
	THashTable.
----------------------------------------------------------------------------*/

//
// Used for mapping Keys to Values using a fixed size table of pointers to
// pairs storing collisions in a linked list format.
//
// Example usage: THashTable< FString, UObject*, 8 > would be used to map
// FStrings to pointers to UObjects in a 2^8 or 256 element table.
//
// Use Set(Key,Value) to map pairs.
// Use Find(Key) or FindRef(Key) to retrieve values.
// Use Remove(Key) to remove entries.
//
template< class TK, class TI, INT TN=10 > class THashTable
{
public:
	enum { SIZE=(1<<TN) };
protected:
	class THashPair
	{
	public:
		TK Key;
		TI Value;
		THashPair* Next;
		THashPair( TK InKey, TI InValue )
		: Key( InKey ), Value( InValue ), Next( NULL )
		{}
		THashPair()
		{}
		~THashPair()
		{
			guard(THashPair::~THashPair);
			if( Next )
				{ delete Next; Next=NULL; }
			unguard;
		}
		friend FArchive& operator<<( FArchive& Ar, THashPair& P )
		{
			guard(THashTable::THashPair<<);
			return Ar << P.Key << P.Value << P.Next;
			unguard;
		}
		friend FArchive& operator<<( FArchive& Ar, THashPair* &P )
		{
			guard(THashTable::THashPair*<<);
			Ar << *(DWORD*)&P;
			if( P )
			{
				if( Ar.IsLoading() )
					P = new THashPair();
				Ar << *P;
			}
			return Ar;
			unguard;
		}
		THashPair( const THashPair& Other )
		: Key( Other.Key ), Value( Other.Value )
		{
			guard(THashPair::THashPair copy);
			if( Other.Next )
				Next = new THashPair(Other.Next);
			else
				Next = NULL;
			unguard;
		}
		THashPair( THashPair* Other )
		: Key( Other->Key ), Value( Other->Value )
		{
			guard(THashPair::THashPair copy);
			if( Other->Next )
				Next = new THashPair(Other->Next);
			else
				Next = NULL;
			unguard;
		}
	};
public:	// for TCollisionIterator's sake (couldn't get 'friend' to work)
	THashPair* Pairs[SIZE];
public:
	THashTable()
	{
		guard(THashTable::THashTable);
		appMemzero(&Pairs,sizeof(Pairs));
		unguard;
	}
	THashTable( const THashTable& Other )
	{
		guard(THashTable::THashTable copy);
		appMemzero(&Pairs,sizeof(Pairs));
		for( int i=0; i<SIZE; i++ )
			if( Other.Pairs[i] )
				Pairs[i] = new THashPair( Other.Pairs[i] );
		unguard;
	}
	THashTable( THashTable* Other )
	{
		guard(THashTable::THashTable copy);
		appMemzero(&Pairs,sizeof(Pairs));
		if( Other )
			for( int i=0; i<SIZE; i++ )
				if( Other->Pairs[i] )
					Pairs[i] = new THashPair( Other->Pairs[i] );
		unguard;
	}
	~THashTable()
	{
		guard(THashTable::~THashTable);
		Empty();
		unguard;
	}
	THashTable& operator=( const THashTable& Other )
	{
		guard(TMapBase::operator=);
		appErrorf(TEXT("THashTable::operator= Not Implemented"));
		return *this;
		unguard;
	}
	void Empty()
	{
		guard(THashTable::Empty);
		for( INT i=0; i<SIZE; i++ )
			if( Pairs[i] )
				{ delete Pairs[i]; Pairs[i]=NULL; }
		unguard;
	}
	void Set( TK InKey, TI InValue )
	{
		guard(THashTable::Set);
		INT iHash = GetTypeHash(InKey) & (SIZE-1);
		if( !Pairs[iHash] )
			Pairs[iHash] = new THashPair(InKey,InValue);
		else if( Pairs[iHash]->Key==InKey )
			{ Pairs[iHash]->Value = InValue; return; }
		else
		{
			THashPair* P;
			for( P=Pairs[iHash]; P->Next; P=P->Next )
				if( P->Next->Key==InKey )
					{ P->Next->Value = InValue; return; }
			P->Next = new THashPair(InKey,InValue);
		}
		unguard;
	}
	void Remove( TK InKey )
	{
		// Assumes only one occurrence in the hash.
		guard(THashTable::Remove);
		INT iHash = GetTypeHash(InKey) & (SIZE-1);
		if( Pairs[iHash] )
		{
			if( Pairs[iHash]->Key==InKey )
			{
				THashPair* DeleteMe = Pairs[iHash];
				Pairs[iHash] = Pairs[iHash]->Next;
				DeleteMe->Next = NULL;
				delete DeleteMe;
				return;
			}
			// Pairs[iHash] is non-NULL due to above check "if( Pairs[iHash] )".
			// Pairs[iHash]->Key!=InKey or it would already have been removed.
			THashPair* Prev=Pairs[iHash];
			THashPair* Next=Prev->Next;
			while( Next )
			{
				if( Next->Key==InKey )
				{
					Prev->Next = Next->Next;
					Next->Next = NULL;
					delete Next;
					return;
				}
				Prev = Next;
				Next = Prev->Next;
			}
		}
		unguard;
	}
	TI* Find( const TK& Key )
	{
		guard(THashTable::Find);
		INT iHash = GetTypeHash(Key) & (SIZE-1);
		for( THashPair* P=Pairs[iHash]; P; P=P->Next )
			if( P->Key==Key )
				return &P->Value;
		return NULL;
		unguard;
	}
	TI FindRef( const TK& Key )
	{
		guard(THashTable::FindRef);
		INT iHash = GetTypeHash(Key) & (SIZE-1);
		for( THashPair* P=Pairs[iHash]; P; P=P->Next )
			if( P->Key==Key )
				return P->Value;
		return NULL;
		unguard;
	}
	const TI* Find( const TK& Key ) const
	{
		guard(THashTable::Find const);
		INT iHash = GetTypeHash(Key) & (SIZE-1);
		for( THashPair* P=Pairs[iHash]; P; P=P->Next )
			if( P->Key==Key )
				return &P->Value;
		return NULL;
		unguard;
	}
	TK* FindKey( const TI& Value )	// inherently slow.
	{
		guard(THashTable::FindKey);
		for( INT i=0; i<SIZE; i++ )
			for( THashPair* P=Pairs[i]; P; P=P->Next )
				if( P->Value==Value )
					return &P->Key;
		return NULL;
		unguard;
	}
	TK FindKeyRef( const TI& Value )	// inherently slow.
	{
		guard(THashTable::FindKeyRef);
		for( INT i=0; i<SIZE; i++ )
			for( THashPair* P=Pairs[i]; P; P=P->Next )
				if( P->Value==Value )
					return P->Key;
		return NULL;
		unguard;
	}
	const TK* FindKey( const TI& Value ) const	// inherently slow.
	{
		guard(THashTable::FindKey const);
		for( INT i=0; i<SIZE; i++ )
			for( THashPair* P=Pairs[i]; P; P=P->Next )
				if( P->Value==Value )
					return &P->Key;
		return NULL;
		unguard;
	}
	friend FArchive& operator<<( FArchive& Ar, THashTable& H )
	{
		guard(THashTable<<);
		for( INT i=0; i<THashTable::SIZE; i++ )
			Ar << H.Pairs[i];
		return Ar;
		unguard;
	}
	void Dump( FOutputDevice& Ar )
	{
		guard(THashTable::Dump);
		Ar.Logf(TEXT("HashTable contents:"));
		INT TotalUsedSlots=0;
		INT TotalCollisions=0;
		for( INT i=0; i<SIZE; i++ )
			if( Pairs[i] )
			{
				TotalUsedSlots++;
				INT Count=0;
				for( THashPair* P=Pairs[i]->Next; P; P=P->Next )
					Count++;
				Ar.Logf(TEXT("    Slot %d has %d collisions."), i, Count );
				TotalCollisions += Count;
			}
		Ar.Logf(TEXT("--Stats------------------------"));
		Ar.Logf(TEXT("              size: %d"), SIZE );
		Ar.Logf(TEXT("  total used slots: %d"), TotalUsedSlots );
		Ar.Logf(TEXT("  total collisions: %d"), TotalCollisions );
		Ar.Logf(TEXT("             usage: %f"), TotalUsedSlots/((FLOAT)SIZE+0.000001f) );
		unguard;
	}

	// Iterators.
	class TCollisionIterator
	{
	public:
		TCollisionIterator( THashTable<TK,TI,TN>& Hash, TK Key )
		:	P(Hash.Pairs[GetTypeHash(Key) & (THashTable<TK,TI,TN>::SIZE-1)])
								{ ++*this;			}
		void operator++()		{ P=P->Next;		}
		operator UBOOL() const	{ return P!=NULL;	}
		TI& operator*()  const	{ return P->Value;	}
		TI* operator->() const	{ return &P->Value;	}
	private:
		THashPair* P;
	};

	class TIterator
	{
	public:
		TIterator( THashTable& _HashTable ) : HashTable(_HashTable), Current(NULL), Index(0) { ++*this; }
		void operator++()          { if(Current) Current=Current->Next; if(!Current) while(!Current && Index<SIZE) Current=HashTable.Pairs[Index++]; }
		operator UBOOL() const     { return Current!=NULL; }
		TK& Key() const            { return Current->Key; }
		TI& Value() const          { return Current->Value; }
	private:
		THashTable& HashTable;
		THashPair *Current;
		INT Index;
	};

//	friend class TCollisionIterator;
//	friend class TIterator;
};

#endif	//_INC_THASHTABLE_
