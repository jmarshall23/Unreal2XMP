
/*----------------------------------------------------------------------------
	TDoubleLinkList.
----------------------------------------------------------------------------*/

//
// Simple double-linked list template.
//
template< class T > class TDoubleLinkList : public T
{
public:
	TDoubleLinkList* Next;
	TDoubleLinkList* Prev;	// Fix ARL: Storing a pointer to the previous (TDoubleLinkList**) would allow us to update the cursor pointer as well when removing nodes.
	TDoubleLinkList( const T& Other )
	: T(Other), Next(NULL), Prev(NULL)
	{}
	// Pass in a pointer to the node you wish this node to replace.
	// The node we are replacing will end up after us in the list.
	// The given pointer will end up pointing to this node when we're done.
	// Given List=NodeA and NodeB->Link(List) we'll end up with:
	// List=>NodeB<->NodeA
	void Link( TDoubleLinkList*& Node )
	{
		if( Node )
		{
			Prev = Node->Prev;
			Node->Prev = this;
		}
		if( Prev )
		{
			Prev->Next = this;
		}

		Next = Node;
		Node = this;
	}
	void Unlink()
	{
		if(Next) Next->Prev=Prev;
		if(Prev) Prev->Next=Next;
	}
};


