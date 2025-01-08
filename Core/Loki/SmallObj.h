////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design 
//	   Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any 
//	   purpose is hereby granted without fee, provided that the above copyright 
//	   notice appear in all copies and that both that copyright notice and this 
//	   permission notice appear in supporting documentation.
// The author or Addison-Welsey Longman make no representations about the 
//	   suitability of this software for any purpose. It is provided "as is" 
//	   without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

// Rewritten to play nicely with Unreal.  2002.02.07 (ARL)

#include "Core.h"

#ifndef SMALLOBJ_INC_
#define SMALLOBJ_INC_

#pragma warning (push)

#ifndef DEFAULT_CHUNK_SIZE
#define DEFAULT_CHUNK_SIZE 4096
#endif

#ifndef MAX_SMALL_OBJECT_SIZE
#define MAX_SMALL_OBJECT_SIZE 256
#endif

namespace Loki
{
////////////////////////////////////////////////////////////////////////////////
// class FixedAllocator
// Offers services for allocating fixed-sized objects
////////////////////////////////////////////////////////////////////////////////

	class CORE_API FixedAllocator
	{
	private:
		struct CORE_API Chunk
		{
			void Init(size_t blockSize, BYTE blocks);
			void* Allocate(size_t blockSize);
			void Deallocate(void* p, size_t blockSize);
			void Reset(size_t blockSize, BYTE blocks);
			void Release();
			BYTE	*pData_,
					firstAvailableBlock_,
					blocksAvailable_;
		};
		
		// Internal functions		 
		void DoDeallocate(void* p);
		Chunk* VicinityFind(void* p);
		
		// Data 
		size_t blockSize_;
		BYTE numBlocks_;
		typedef TArray<Chunk> Chunks;
		Chunks chunks_;
		Chunk* allocChunk_;
		Chunk* deallocChunk_;
		// For ensuring proper copy semantics
		mutable const FixedAllocator* prev_;
		mutable const FixedAllocator* next_;
		
	public:
		// Create a FixedAllocator able to manage blocks of 'blockSize' size
		explicit FixedAllocator(size_t blockSize = 0);
		FixedAllocator(const FixedAllocator&);
		FixedAllocator& operator=(const FixedAllocator&);
		~FixedAllocator();
		
		void Swap(FixedAllocator& rhs);
		
		// Allocate a memory block
		void* Allocate();
		// Deallocate a memory block previously allocated with Allocate()
		// (if that's not the case, the behavior is undefined)
		void Deallocate(void* p);
		// Returns the block size with which the FixedAllocator was initialized
		size_t BlockSize() const
		{ return blockSize_; }
		// Comparison operator for sorting 
		bool operator<(size_t rhs) const
		{ return BlockSize() < rhs; }
	};
	
////////////////////////////////////////////////////////////////////////////////
// class SmallObjAllocator
// Offers services for allocating small-sized objects
////////////////////////////////////////////////////////////////////////////////

	class CORE_API SmallObjAllocator
	{
	public:
		SmallObjAllocator(
			size_t chunkSize, 
			size_t maxObjectSize);
	
		void* Allocate(size_t numBytes);
		void Deallocate(void* p, size_t size);
	
	private:
		SmallObjAllocator(const SmallObjAllocator&);
		SmallObjAllocator& operator=(const SmallObjAllocator&);
		
		typedef TArray<FixedAllocator> Pool;
		Pool pool_;
		FixedAllocator* pLastAlloc_;
		FixedAllocator* pLastDealloc_;
		size_t chunkSize_;
		size_t maxObjectSize_;
	};

////////////////////////////////////////////////////////////////////////////////
// class SmallObject
// Base class for polymorphic small objects, offers fast
//	   allocations/deallocations
////////////////////////////////////////////////////////////////////////////////

	class CORE_API SmallObject
	{
		struct CORE_API MySmallObjAllocator : public SmallObjAllocator
		{
			MySmallObjAllocator() 
			: SmallObjAllocator(DEFAULT_CHUNK_SIZE, MAX_SMALL_OBJECT_SIZE)
			{}
			static MySmallObjAllocator* GetInstance();
		};
	public:
		static void* operator new(size_t size)
		{
			void* p;
#if (MAX_SMALL_OBJECT_SIZE != 0) && (DEFAULT_CHUNK_SIZE != 0)
			p=MySmallObjAllocator::GetInstance()->Allocate(size);
#else
			p=::operator new(size);
#endif
			appMemset(p,0,size);	// Fix ARL: We should implement a default object in uiClass so we don't need to initialize memory here, instead just blast in data from the default object.
			return p;
		}
		static void operator delete(void* p, size_t size)
		{
#if (MAX_SMALL_OBJECT_SIZE != 0) && (DEFAULT_CHUNK_SIZE != 0)
			MySmallObjAllocator::GetInstance()->Deallocate(p, size);
#else
			//::operator delete(p, size);
			::operator delete(p);
#endif
		}
		virtual ~SmallObject() {}
		static FString GetStats();
	};
} // namespace Loki

#pragma warning (pop)

#endif // SMALLOBJ_INC_
