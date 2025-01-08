//=============================================================================
// FeastExpressions.h: 
//=============================================================================

// Fix ARL: Who destroys the parser?
// Fix ARL: Make sure to delete the root when finished with it.

//!!	NONTERM("postfix_expr", "postfix_expr:1 '.' STRING:2", NODE_Field);	// Fix ARL: Not accounted for!!  Needed?

//NOTE[aleiby]: You'll need to add /Zm512 to your project settings for this to properly compile.

/*----------------------------------------------------------------------------
	Includes.
----------------------------------------------------------------------------*/

#include "..\..\feast\inc\DataType.h"
#include "..\..\feast\inc\feast.h"

using namespace FEAST;


/*----------------------------------------------------------------------------
	Public Interfaces.
----------------------------------------------------------------------------*/

// Convert the conditional string into a syntax tree
static IPrsNode* ParseExpression( char* inExpr );

// Evaluate the condition by recursively traversing the tree
static bool EvaluateExpression( IPrsNode* inNode, DataType& Result, void* UserData=NULL );


/*----------------------------------------------------------------------------
	Debug info.
----------------------------------------------------------------------------*/

#if 0 // set to 1 for runtime debug log info.
#ifdef NOTE
#undef NOTE
#endif
#define NOTE NOTE1
#endif


/*----------------------------------------------------------------------------
	Pragmas.
----------------------------------------------------------------------------*/

#if 0 // set to 1 to enable additional compiler settings.
#pragma warning( disable: 4786 ) // identifier was truncated to '255' characters in the debug information

#pragma inline_depth( 255 )
#pragma inline_recursion( on )
#pragma auto_inline( on )

#define inline __forceinline 
#endif


/*----------------------------------------------------------------------------
	UniqueID.
----------------------------------------------------------------------------*/

#define UNIQUE_ID __LINE__
#define MIN_UNIQUE_ID 0
#define MAX_UNIQUE_ID 512


/*----------------------------------------------------------------------------
	Macros.
----------------------------------------------------------------------------*/

#define DEF_ID(_id_) \
	enum { id=(_id_) };

#define DEF_TYPE(_type_) \
	enum { type=(_type_) };

#define DEF_PRIORITY(_priority_) \
	static inline DWORD		GetPriority()	{ return (_priority_);		}

#define DEF_NAME(_name_) \
	static inline char*		GetName()		{ return (_name_);			}

#define DEF_RULE(_rule_) \
	static inline char*		GetRule()		{ return (_rule_);			}

#define DEF_PRODUCTION(_production_) \
	static inline char*		GetProduction()	{ return (_production_);	}

#define DEF_REGEX(_regex_) \
	static inline char*		GetRegex()		{ return (_regex_);			}


#define EVALUATE_T_START \
	static inline bool EvaluateT( DataType& Result, IPrsNode* inNode, void* UserData=NULL ) \
	{ \
		guard(EvaluateT); \
		NOTE(debugf(TEXT("EvaluateT<%d>: %s [%d] '%0.*s'"), id, appFromAnsi(GetName()), inNode ? inNode->NodeGetTag() : -1, inNode->NodeGetToken()->mLexemeLen, appFromAnsi(inNode->NodeGetToken()->mLexeme))); \
		FString Term = appFromAnsi(inNode->NodeGetToken()->mLexeme);

#define EVALUATE_T_END \
		return false; \
		unguardf((TEXT("(%s)"),appFromAnsi(GetName()))); \
	}

#define EVALUATE_NT_START(num_parms) \
	static inline bool EvaluateNT( DataType& Result, DataType* Operands, INT nOps, void* UserData=NULL ) \
	{ \
		guard(EvaluateNT); \
		NOTE(debugf(TEXT("EvaluateNT<%d>: %s"), id, appFromAnsi(GetName()))); \
		check(Operands); \
		if(nOps<num_parms) \
		{ \
			debugf(NAME_Warning,TEXT("Not enough operands for operator '%s'! (Expected: %d Given: %d)"), appFromAnsi(GetName()), num_parms, nOps); \
			return false; \
		}

#define EVALUATE_NT_END \
		return false; \
		unguardf((TEXT("(%s)"),appFromAnsi(GetName()))); \
	}

#define STRUCTURE_NODE(R,P) \
	template<> struct FeastNode<UNIQUE_ID> \
	{ \
		DEF_ID			(0); \
		DEF_TYPE		(NT_NonTerminal); \
		DEF_NAME		("StructureNode") \
		DEF_RULE		(R); \
		DEF_PRODUCTION	(P); \
		EVALUATE_NT_START(0) \
			debugf(NAME_Warning, TEXT("Attempting to evaluate StructureNode: '%s'/'%s'"), GetRule(), GetProduction()); \
		EVALUATE_NT_END \
	};


/*----------------------------------------------------------------------------
	FeastNode template interface.
----------------------------------------------------------------------------*/

enum ENodeType
{
	NT_None			=0x0,	// Must define:
	NT_Token		=0x1,	// - priority, name, regex
	NT_Terminal		=0x2,	// - name, rule, production, EVALUATE_T
	NT_NonTerminal	=0x4,	// - name, rule, production, EVALUATE_NT
};

template<DWORD T> struct FeastNode
{
	DEF_ID			(T);		// required.
	DEF_TYPE		(NT_None);	// required.
	DEF_PRIORITY	(0);		// token priority.
	DEF_NAME		("");		// name of this terminal or non-terminal.
	DEF_RULE		("");		// group name for this terminal or non-terminal.
	DEF_PRODUCTION	("");		// for non-terminals only.
	DEF_REGEX		("");		// for terminals only.

	EVALUATE_T_START
		/* terminal implementation */
	EVALUATE_T_END

	EVALUATE_NT_START(0)
		/* non-terminal implementation */
	EVALUATE_NT_END
};


/*----------------------------------------------------------------------------
	Initialization.
----------------------------------------------------------------------------*/

static IPrsParser* parser = NULL;
static ILexLexer* lexer = NULL;

template< INT i > struct TokenIterator
{
	template< DWORD t > struct NodeType
	{
		static inline void Register()
		{
			NOTE(debugf(TEXT("TokenIterator<%d>::Register: (%d) '%s' '%s' [%d]"),i,FeastNode<i>::GetPriority(),appFromAnsi(FeastNode<i>::GetName()),appFromAnsi(FeastNode<i>::GetRegex()),FeastNode<i>::id));
			lexer->LexTokenPriority(FeastNode<i>::GetPriority());
			lexer->LexRegisterToken(FeastNode<i>::type == NT_Token ? 0 : FeastNode<i>::id,FeastNode<i>::GetRegex());		// register nodes which are *only* NT_Tokens as zero, so feast will ignore 'em .
		}
	};
	template<> struct NodeType<0>{ static inline void Register(){} };

	static inline void Iterate()
	{
		NodeType< FeastNode<i>::type & NT_Token >::Register();
		TokenIterator<i+1>::Iterate();
	}
};
template<> struct TokenIterator<MAX_UNIQUE_ID+1>{ static inline void Iterate(){} };

template< INT i > struct TerminalIterator
{
	template< DWORD t > struct NodeType
	{
		static inline void Register()
		{
			NOTE(debugf(TEXT("TerminalIterator<%d>::Register: '%s' '%s' [%d] [%d]"),i,appFromAnsi(FeastNode<i>::GetName()),appFromAnsi(FeastNode<i>::GetProduction()),FeastNode<i>::id,FeastNode<i>::id));
			parser->PrsRegisterT(FeastNode<i>::GetProduction(),FeastNode<i>::id,FeastNode<i>::id);
		}
	};
	template<> struct NodeType<0>{ static inline void Register(){} };

	static inline void Iterate()
	{
		NodeType< FeastNode<i>::type & NT_Terminal >::Register();
		TerminalIterator<i+1>::Iterate();
	}
};
template<> struct TerminalIterator<MAX_UNIQUE_ID+1>{ static inline void Iterate(){} };

template< INT i > struct NonTerminalIterator
{
	template< DWORD t > struct NodeType{ static inline void Register(){} };
	template<> struct NodeType< NT_Terminal >
	{
		static inline void Register()
		{
			NOTE(debugf(TEXT("NonTerminalIterator<%d>::Register<NT_Terminal>: '%s' '%s' '%s' [%d]"),i,appFromAnsi(FeastNode<i>::GetName()),appFromAnsi(FeastNode<i>::GetRule()),appFromAnsi(FeastNode<i>::GetProduction()),0));
			parser->PrsRegisterNT(FeastNode<i>::GetRule(),FeastNode<i>::GetProduction(),0);
		}
	};
	template<> struct NodeType< NT_NonTerminal >
	{
		static inline void Register()
		{
			NOTE(debugf(TEXT("NonTerminalIterator<%d>::Register<NT_NonTerminal>: '%s' '%s' '%s' [%d]"),i,appFromAnsi(FeastNode<i>::GetName()),appFromAnsi(FeastNode<i>::GetRule()),appFromAnsi(FeastNode<i>::GetProduction()),FeastNode<i>::id));
			parser->PrsRegisterNT(FeastNode<i>::GetRule(),FeastNode<i>::GetProduction(),FeastNode<i>::id);
		}
	};

	static inline void Iterate()
	{
		NodeType< FeastNode<i>::type & (NT_Terminal | NT_NonTerminal) >::Register();
		NonTerminalIterator<i+1>::Iterate();
	}
};
template<> struct NonTerminalIterator<MAX_UNIQUE_ID+1>{ static inline void Iterate(){} };

static inline void InitParser()
{
	if(parser) return;	// already initialized.

	// create parser
	parser = IPrsParser::PrsCreate();

	// get parser's lexer and set up terminal regexs
	lexer = parser->PrsGetLexer();

	// register all tokens [token]
	TokenIterator<MIN_UNIQUE_ID>::Iterate();

	// register terminal rules for lexer tags [terminal]
	TerminalIterator<MIN_UNIQUE_ID>::Iterate();
	
	// register non-terminal rules [terminal | non-terminal]
	NonTerminalIterator<MIN_UNIQUE_ID>::Iterate();

	// build the parser
	parser->PrsBuild();
}


/*----------------------------------------------------------------------------
	ParseExpression.
----------------------------------------------------------------------------*/

IPrsNode* ParseExpression( char* inExpr )
{
	guard(ParseExpression);
	NOTE(debugf(TEXT("ParseExpression: '%s'"),appFromAnsi(inExpr)));
	InitParser();

	// execute parser on test expression (with 4 space tabs)
	IPrsNode* root = parser->PrsExecute(inExpr,4);
	if( !root )
		debugf(NAME_Warning, TEXT("ParseExpression: Syntax error: '%s' Line: '%s'"), appFromAnsi(parser->PrsGetLastError()), appFromAnsi(inExpr) );

	// FIX tbr??
	NOTE0(DataType Result; EvaluateExpression(root,Result); FString s; Result.AsString(s); debugf(TEXT("ParseExpression: Result='%s'"),*s);)

	return root;
	unguard;
}


/*----------------------------------------------------------------------------
	EvaluateExpression.
----------------------------------------------------------------------------*/

static struct
{
	INT tag;
	IPrsNode* inNode;
	DataType* Result;
	DataType* Operands;
	void* UserData;

} NodeParms;

template< INT i > struct NodeIterator
{
	template< DWORD t > struct NodeType{ static inline bool Evaluate(){ return NodeIterator<i+1>::Iterate(); } };
	template<> struct NodeType< NT_Terminal >
	{
		static inline bool Evaluate()
		{
			if( FeastNode<i>::id == NodeParms.tag )
				return FeastNode<i>::EvaluateT( *NodeParms.Result, NodeParms.inNode, NodeParms.UserData );
			else
				return NodeIterator<i+1>::Iterate();
		}
	};
	template<> struct NodeType< NT_NonTerminal >
	{
		static inline bool Evaluate()
		{
			if( FeastNode<i>::id == NodeParms.tag )
				return FeastNode<i>::EvaluateNT( *NodeParms.Result, NodeParms.Operands, NodeParms.inNode->NodeGetChildCount(), NodeParms.UserData );
			else
				return NodeIterator<i+1>::Iterate();
		}
	};
	static inline bool Iterate()
	{
		return NodeType< FeastNode<i>::type & (NT_Terminal | NT_NonTerminal) >::Evaluate();
	}
};
template<> struct NodeIterator<MAX_UNIQUE_ID+1>
{
	static inline bool Iterate()
	{
		debugf(NAME_Warning, TEXT("Unknown node type: '%d'"), NodeParms.tag);
		return false;
	}
};

bool EvaluateExpression( IPrsNode* inNode, DataType& Result, void* UserData )
{
	guard(EvaluateExpression);
	if(!inNode){ debugf(NAME_Warning, TEXT("EvaluateExpression: NULL node")); return false; }

	DataType Operands[2];

	// Post-order depth-first traversal.
	for( DWORD i=0; i<inNode->NodeGetChildCount() && i < ARRAY_COUNT(Operands); i++ )
		if( !EvaluateExpression(inNode->NodeGetChild(i), Operands[i], UserData ) )
			{ debugf(NAME_Warning, TEXT("Bad node tag: '%d'"), inNode->NodeGetTag() ); return false; }

	// Dispatch to appropriate operator.
	NodeParms.tag		=inNode->NodeGetTag();
	NodeParms.inNode	=inNode;
	NodeParms.Result	=&Result;
	NodeParms.Operands	=Operands;
	NodeParms.UserData	=UserData;
	return NodeIterator<MIN_UNIQUE_ID>::Iterate();
	unguard;
}

