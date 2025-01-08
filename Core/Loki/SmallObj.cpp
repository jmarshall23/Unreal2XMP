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

#include "CorePrivate.h"

#include "SmallObj.h"
#include <limits.h>

#define SHOW_STATS 0

using namespace Loki;

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::Chunk::Init
// Initializes a chunk object
////////////////////////////////////////////////////////////////////////////////

void FixedAllocator::Chunk::Init(size_t blockSize, BYTE blocks)
{
	checkSlow(blockSize > 0);
	checkSlow(blocks > 0);
	// Overflow check
	checkSlow((blockSize * blocks) / blockSize == blocks);
	pData_ = new BYTE[blockSize * blocks];
	Reset(blockSize, blocks);
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::Chunk::Reset
// Clears an already allocated chunk
////////////////////////////////////////////////////////////////////////////////

void FixedAllocator::Chunk::Reset(size_t blockSize, BYTE blocks)
{
	checkSlow(blockSize > 0);
	checkSlow(blocks > 0);
	// Overflow check
	checkSlow((blockSize * blocks) / blockSize == blocks);
	firstAvailableBlock_ = 0;
	blocksAvailable_ = blocks;
	BYTE i = 0;
	BYTE* p = pData_;
	for (; i != blocks; p += blockSize)
	{
		*p = ++i;
	}
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::Chunk::Release
// Releases the data managed by a chunk
////////////////////////////////////////////////////////////////////////////////

void FixedAllocator::Chunk::Release()
{
	delete[] pData_;
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::Chunk::Allocate
// Allocates a block from a chunk
////////////////////////////////////////////////////////////////////////////////

void* FixedAllocator::Chunk::Allocate(size_t blockSize)
{
	if (!blocksAvailable_) return 0;
	checkSlow((firstAvailableBlock_ * blockSize) / blockSize == firstAvailableBlock_);
	BYTE* pResult = pData_ + (firstAvailableBlock_ * blockSize);
	firstAvailableBlock_ = *pResult;
	--blocksAvailable_;
	return pResult;
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::Chunk::Deallocate
// Dellocates a block from a chunk
////////////////////////////////////////////////////////////////////////////////

void FixedAllocator::Chunk::Deallocate(void* p, size_t blockSize)
{
	checkSlow(p >= pData_);
	BYTE* toRelease = static_cast<BYTE*>(p);
	// Alignment check
	checkSlow((toRelease - pData_) % blockSize == 0);
	*toRelease = firstAvailableBlock_;
	firstAvailableBlock_ = static_cast<BYTE>((toRelease - pData_) / blockSize);
	// Truncation check
	checkSlow(firstAvailableBlock_ == (toRelease - pData_) / blockSize);
	++blocksAvailable_;
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::FixedAllocator
// Creates a FixedAllocator object of a fixed block size
////////////////////////////////////////////////////////////////////////////////

FixedAllocator::FixedAllocator(size_t blockSize)
	: blockSize_(blockSize)
	, allocChunk_(0)
	, deallocChunk_(0)
{
	checkSlow(blockSize_ > 0);
	prev_ = next_ = this;
	size_t numBlocks = DEFAULT_CHUNK_SIZE / blockSize;
	if (numBlocks > UCHAR_MAX) numBlocks = UCHAR_MAX;
	else if (numBlocks == 0) numBlocks = 8 * blockSize;
	numBlocks_ = static_cast<BYTE>(numBlocks);
	checkSlow(numBlocks_ == numBlocks);
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::FixedAllocator(const FixedAllocator&)
// Creates a FixedAllocator object of a fixed block size
////////////////////////////////////////////////////////////////////////////////

FixedAllocator::FixedAllocator(const FixedAllocator& rhs)
	: blockSize_(rhs.blockSize_)
	, numBlocks_(rhs.numBlocks_)
	, chunks_(rhs.chunks_)
{
	prev_ = &rhs;
	next_ = rhs.next_;
	rhs.next_->prev_ = this;
	rhs.next_ = this;
	allocChunk_ = rhs.allocChunk_
		? &chunks_(0) + (rhs.allocChunk_ - &rhs.chunks_(0))
		: 0;
	deallocChunk_ = rhs.deallocChunk_
		? &chunks_(0) + (rhs.deallocChunk_ - &rhs.chunks_(0))
		: 0;
}

FixedAllocator& FixedAllocator::operator=(const FixedAllocator& rhs)
{
	FixedAllocator copy(rhs);
	copy.Swap(*this);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::~FixedAllocator
////////////////////////////////////////////////////////////////////////////////

FixedAllocator::~FixedAllocator()
{
	if (prev_ != this)
	{
		prev_->next_ = next_;
		next_->prev_ = prev_;
		return;
	}
	checkSlow(prev_ == next_);
	for (INT i=0; i<chunks_.Num(); i++)
	{
	   checkSlow(chunks_(i).blocksAvailable_ == numBlocks_);
	   chunks_(i).Release();
	}
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::Swap
////////////////////////////////////////////////////////////////////////////////

void FixedAllocator::Swap(FixedAllocator& rhs)
{
	Exchange(blockSize_, rhs.blockSize_);
	Exchange(numBlocks_, rhs.numBlocks_);
	ExchangeArray(chunks_, rhs.chunks_);
	Exchange(allocChunk_, rhs.allocChunk_);
	Exchange(deallocChunk_, rhs.deallocChunk_);
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::Allocate
// Allocates a block of fixed size
////////////////////////////////////////////////////////////////////////////////

void* FixedAllocator::Allocate()
{
	if (allocChunk_ == 0 || allocChunk_->blocksAvailable_ == 0)
	{
		for	(INT i=0;; i++)
		{
			if (i == chunks_.Num())
			{
				// Initialize
				chunks_.Add();
				chunks_.Last().Init(blockSize_, numBlocks_);
				allocChunk_ = &chunks_.Last();
				deallocChunk_ = &chunks_(0);
				break;
			}
			if (chunks_(i).blocksAvailable_ > 0)
			{
				allocChunk_ = &chunks_(i);
				break;
			}
		}
	}
	checkSlow(allocChunk_ != 0);
	checkSlow(allocChunk_->blocksAvailable_ > 0);
	return allocChunk_->Allocate(blockSize_);
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::Deallocate
// Deallocates a block previously allocated with Allocate
// (undefined behavior if called with the wrong pointer)
////////////////////////////////////////////////////////////////////////////////

void FixedAllocator::Deallocate(void* p)
{
	checkSlow(chunks_.Num()!=0);
	checkSlow(&chunks_(0) <= deallocChunk_);
	checkSlow(&chunks_.Last() >= deallocChunk_);
	deallocChunk_  = VicinityFind(p);
	checkSlow(deallocChunk_);
	DoDeallocate(p);
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::VicinityFind (internal)
// Finds the chunk corresponding to a pointer, using an efficient search
////////////////////////////////////////////////////////////////////////////////

FixedAllocator::Chunk* FixedAllocator::VicinityFind(void* p)
{
	checkSlow(chunks_.Num()!=0);
	checkSlow(deallocChunk_);
	
	const size_t chunkLength = numBlocks_ * blockSize_;

	Chunk* lo = deallocChunk_;
	Chunk* hi = (deallocChunk_!=&chunks_.Last()) ? deallocChunk_ + 1 : 0;
	Chunk* loBound = &chunks_(0);
	Chunk* hiBound = &chunks_.Last() + 1;

	for (;;)
	{
		if (lo)
		{
			if (p >= lo->pData_ && p < lo->pData_ + chunkLength)
			{
				return lo;
			}
			if (lo == loBound) lo = 0;
			else --lo;
		}
		
		if (hi)
		{
			if (p >= hi->pData_ && p < hi->pData_ + chunkLength)
			{
				return hi;
			}
			if (++hi == hiBound) hi = 0;
		}
	}
	checkSlow(false);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// FixedAllocator::DoDeallocate (internal)
// Performs deallocation. Assumes deallocChunk_ points to the correct chunk
////////////////////////////////////////////////////////////////////////////////

void FixedAllocator::DoDeallocate(void* p)
{
	checkSlow(deallocChunk_->pData_ <= p);
	checkSlow(deallocChunk_->pData_ + numBlocks_ * blockSize_ > p);

	// call into the chunk, will adjust the inner list but won't release memory
	deallocChunk_->Deallocate(p, blockSize_);

	if (deallocChunk_->blocksAvailable_ == numBlocks_)
	{
		// deallocChunk_ is completely free, should we release it? 
		Chunk& lastChunk = chunks_.Last();
		
		if (&lastChunk == deallocChunk_)
		{
			// check if we have two last chunks empty
			if (chunks_.Num() > 1 && deallocChunk_[-1].blocksAvailable_ == numBlocks_)
			{
				// Two free chunks, discard the last one
				lastChunk.Release();
				chunks_.Pop();
				allocChunk_ = deallocChunk_ = &chunks_(0);
			}
			return;
		}
		
		if (lastChunk.blocksAvailable_ == numBlocks_)
		{
			// Two free blocks, discard one
			lastChunk.Release();
			Chunk* FirstChunk = &chunks_(0);
			chunks_.Pop();
			// account for reallocation of chunks_
			if (FirstChunk != &chunks_(0))
			{
				deallocChunk_ = &chunks_(0) + (deallocChunk_ - FirstChunk);
			}
			allocChunk_ = deallocChunk_;
		}
		else
		{
			// move the empty chunk to the end
			Exchange(*deallocChunk_, lastChunk);
			allocChunk_ = &chunks_.Last();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// SmallObjAllocator::SmallObjAllocator
// Creates an allocator for small objects given chunk size and maximum 'small'
//	   object size
////////////////////////////////////////////////////////////////////////////////

SmallObjAllocator::SmallObjAllocator(
		size_t chunkSize, 
		size_t maxObjectSize)
	: pLastAlloc_(0), pLastDealloc_(0)
	, chunkSize_(chunkSize), maxObjectSize_(maxObjectSize) 
{}

////////////////////////////////////////////////////////////////////////////////
// SmallObjAllocator::Allocate
// Allocates 'numBytes' memory
// Uses an internal pool of FixedAllocator objects for small objects  
////////////////////////////////////////////////////////////////////////////////

#if SHOW_STATS
static size_t currentAllocated=0;
static size_t totalAllocated=0;
static size_t maxAllocated=0;
#endif

FString SmallObject::GetStats()
{
#if SHOW_STATS
	return FString::Printf(TEXT("Current: %d Max %d Total: %d"),currentAllocated,maxAllocated,totalAllocated);
#else
	return TEXT("Disabled");
#endif
}

void* SmallObjAllocator::Allocate(size_t numBytes)
{
	if (numBytes > maxObjectSize_) return operator new(numBytes);

#if SHOW_STATS
	totalAllocated += numBytes;
	currentAllocated += numBytes;
	maxAllocated = Max( maxAllocated, currentAllocated );
	//debugf(TEXT("current=%d max=%d total=%d"),currentAllocated,maxAllocated,totalAllocated);
#endif
	
	if (pLastAlloc_ && pLastAlloc_->BlockSize() == numBytes)
	{
		return pLastAlloc_->Allocate();
	}
	// find the first position numBytes can be added without disturbing the ordering of pool
	INT i=0; while (i < pool_.Num() && pool_(i) < numBytes) i++;
	if (i == pool_.Num() || pool_(i).BlockSize() != numBytes)
	{
		new(pool_,i)FixedAllocator(numBytes);
		pLastDealloc_ = &pool_(0);
	}
	pLastAlloc_ = &pool_(i);
	return pLastAlloc_->Allocate();
}

////////////////////////////////////////////////////////////////////////////////
// SmallObjAllocator::Deallocate
// Deallocates memory previously allocated with Allocate
// (undefined behavior if you pass any other pointer)
////////////////////////////////////////////////////////////////////////////////

void SmallObjAllocator::Deallocate(void* p, size_t numBytes)
{
	if (numBytes > maxObjectSize_) { operator delete(p); return; }

#if SHOW_STATS
	currentAllocated -= numBytes;
	//debugf(TEXT("remaining=%d"),currentAllocated);
#endif

	if (pLastDealloc_ && pLastDealloc_->BlockSize() == numBytes)
	{
		pLastDealloc_->Deallocate(p);
		return;
	}
	// find the first position numBytes can be added without disturbing the ordering of pool
	INT i=0; while (i < pool_.Num() && pool_(i) < numBytes) i++;
	checkSlow(i != pool_.Num());
	checkSlow(pool_(i).BlockSize() == numBytes);
	pLastDealloc_ = &pool_(i);
	pLastDealloc_->Deallocate(p);
}

////////////////////////////////////////////////////////////////////////////////
// SmallObject::MySmallObjAllocator::GetInstance
// Get singleton allocator object.
////////////////////////////////////////////////////////////////////////////////

SmallObject::MySmallObjAllocator* SmallObject::MySmallObjAllocator::GetInstance()
{
	static MySmallObjAllocator StaticSmallObject;
	return &StaticSmallObject;
}

