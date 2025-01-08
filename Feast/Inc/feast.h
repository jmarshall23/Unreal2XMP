#ifndef __FEAST_H__
#define __FEAST_H__
//****************************************************************************
//**
//**    FEAST.H
//**    Front-End / Abstract Syntax Tree library
//**	Version 1.03
//**
//**	Legal Disclaimer:
//**	Copyright (c) 2001 Chris Hargrove.  Provided for the exclusive
//**	use of Legend Entertainment; all other users must be authorized by
//**	written consent of the author.  Not intended for public distribution.
//**
//**	Revision History:
//**
//**	Version 1.03 - 7/2/01
//**		- Minor internal bugfix related to static initialization order.
//**		- No interface changes.
//**	Version 1.02 - 6/28/01
//**		- Removed LIB_Init/Shutdown in favor of new LIB_SetClient/GetClient
//**		  methods.
//**		- A default client implementation is now provided using C standard
//**		  library functions (see ILibClient below).
//**		- A change in client during the application will no longer cause
//**		  memory blocks allocated with the old client to crash when freed;
//**		  old blocks will be freed with the old client.
//**
//**	Version 1.01 - 6/18/01
//**		- Added IPrsAttr interface for clients in order to attach optional
//**		  semantic information to IPrsNode AST nodes.
//**
//**	Version 1.0 - 6/18/01
//**		- Initial version.
//**
//****************************************************************************

namespace FEAST {

//============================================================================
//    DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
// Stock regular expressions for convenience
enum ELexStockRegex
{
	LEXREGEX_Whitespace=1,	// "[ \\t\\n]*"
	LEXREGEX_EolComment,	// "//.*"
	LEXREGEX_BlockComment,	// "\\/\\*"
	LEXREGEX_Identifier,	// "[a-zA-Z_]([a-zA-Z0-9_])*"
	LEXREGEX_DecInteger,	// "[0-9]+"
	LEXREGEX_HexInteger,	// "0[xX][0-9a-fA-F]+"
	LEXREGEX_OctInteger,	// "0[qQ][0-7]+"
	LEXREGEX_BinInteger,	// "0[bB][0-1]+"
	LEXREGEX_Float,			// "([0-9]+[Ee][\\+\\-]?[0-9]+)|([0-9]*\\.[0-9]+([Ee][\\+\\-]?[0-9]+)?)|([0-9]+\\.[0-9]*([Ee][\\+\\-]?[0-9]+)?)"
	LEXREGEX_String,		// "\\\"(\\\\.|[^\\\\\"])*\\\""
	LEXREGEX_Character,		// "'(\\\\.|[^\\\\'])+'"
};

//============================================================================
//    CLASSES / STRUCTURES
//============================================================================
/*
	XLibException - Base exception class, can be thrown during
	callbacks like token intercept functions and propogated back to the
	client caller.  Exceptions not derived from this base may not
	be caught.
*/
class XLibException
{
public:
	virtual const char* LibExceptionGetText() { return(""); }
};

/*
	ILibClient - Interface passed to library initialization function,
	for memory allocation and error reporting.  Implemented by client
	application.
*/
class ILibClient
{
public:
	// Allocate a block of memory.
	// Default implementation uses malloc().
	virtual void* LibMalloc(unsigned long inSize)=0;

	// Reallocate a block of memory.
	// Default implementation uses realloc().
	virtual void* LibRealloc(void* inPtr, unsigned long inNewSize)=0;

	// Free a block of memory allocated with LibMalloc.
	// Default implementation uses free().
	virtual void LibFree(void* inPtr)=0;

	// Report a fatal error to the application.
	// Default implementation uses printf() and exit().
	virtual void LibError(const char* inErrorStr)=0;
};

/*
	SLexToken - Lexer token structure
*/
struct SLexToken
{
    // User tag value for token type, passed in at registration,
	// always non-zero for tokens returned by GetToken.
	unsigned long mTag;

    // Pointer to beginning of token lexeme within source buffer.  Note
	// that since this uses the original source buffer, the string is
	// *not* null-terminated.
	char* mLexeme;
    
	// Length of token lexeme string.
	unsigned long mLexemeLen;
    
	// Zero-based line and column where the token begins within the text.
	unsigned long mTextLine;
	unsigned long mTextColumn;
};

/*
	ILexLexer - Lexical analyzer (lexer) interface
*/
class ILexLexer
{
public:
	// Create a lexer interface.
	static ILexLexer* LexCreate();

	// Destroy and free the interface.
	virtual bool LexDestroy()=0;

	// Sets whether the token retrieval should be case sensitive or not.
	// The default is 1 (case sensitive).
	virtual bool LexCaseSensitivity(bool inIsCaseSensitive)=0;

	// Registers a token type for lexer recognition, associated with
	// a nonzero tag value which is returned by the GetToken function and
	// set inside the token structure.  A tag value of zero indicates
	// an "ignored" token, which is automatically skipped over by the
	// lexer (used for whitespace, skip-over characters, comments, etc).
	virtual bool LexRegisterToken(unsigned long inTag, const char* inRegex)=0;

	// Sets the token priority level for subsequent RegisterToken calls
	// (the priority is zero by default).  When more than one possible
	// tag match is available for the lexer to choose from, the one with
	// the highest priority is chosen.  Use this to distinguish constant
	// keywords from identifiers, for example.  Returns the previous
	// priority level.
	virtual unsigned char LexTokenPriority(unsigned char inPriority)=0;

	// Sets a function which, when the given tag is recognized by
	// GetToken, is called to give the user a chance to change the
	// token results before the function returns.  Useful for things
	// such as "simulated tags", where a tag can be returned that the
	// lexer would never recognize otherwise, for example distinguishing
	// type names from normal identifiers based on user symbol table data.
	// Other uses include explicit GetChar feeds for block comments and
	// the like.  Only affects tokens with this tag which are registered
	// BEFORE this function is called, not after.
	virtual bool LexTokenIntercept(unsigned long inTag, void (*inIntercept)(ILexLexer*, SLexToken*))=0;

	// Performs irreversable calculations and optimizations on the lexer
	// recognition data.  Only done once, optional, and very slow.
	// If this is used, no more calls to RegisterToken are allowed after,
	// but scanning speed will increase by several orders of magnitude.
	// Recommended for large scans or other cases where initialization
	// time is not important, but scanning speed is.  If Finalize is not
	// called, startup time may be significantly reduced but scanning
	// time may be unacceptably slow.
	virtual bool LexFinalize()=0;

	// Sets or gets the current input stream for the next
	// pending read.  SetText must be used at least once before
	// any characters or tokens are read, so the lexer can scan
	// from valid data.  The line and column counts are typically
	// initialized to zero when the text is first set.  The tab
	// spacing parameter sets how many columns should equate to one
	// tab character (\t) in the text.  A value of zero assumes the
	// default column count of 8.
	virtual bool LexSetText(const char* inText, unsigned long inLine=0, unsigned long inColumn=0, unsigned long inTabSpacing=8)=0;
	virtual char* LexGetText(unsigned long* outLine=0, unsigned long* outColumn=0)=0;

	// Reads a character from the input stream, either advancing the stream
	// by one character ("getchar"), or not ("peekchar").  The default is
	// to advance.
	virtual char LexGetChar(bool inAdvance=true)=0;

	// Reads a token from the input stream, either advancing the stream
	// by one token ("gettoken"), or not ("peektoken").  The default is
	// to advance.  Returns the tag value from the recognized token.
	// The outToken parameter is filled with the token data if non-null.
	// Skips over any tokens registered with a zero tag before returning.
	// Will return zero at the end of the input stream (i.e. a null
	// terminator) and/or another character not supported by any registered
	// token regex.
	virtual unsigned long LexGetToken(SLexToken* outToken, bool inAdvance=true)=0;

	// Returns a descriptive error string when lexer functions return failure.
	// Currently only used with regex errors reported by RegisterToken.
	virtual char* LexGetLastError()=0;

	// Return a stock regex expression for frequently used token expressions,
	// as a convenience function.  The returned strings are contained in internal
	// static buffers and should not be modified.
	virtual const char* LexGetStockRegex(ELexStockRegex inStockRegex)=0;
};

/*
	IPrsAttr - Abstract semantic attribute interface implementable
	by the client application.  Used to attach arbitrary additional
	client data to AST nodes while still allowing destruction when
	the AST nodes are destroyed (semantic information might include
	symbol table references, constant values of numerical terminals,
	and so forth).
*/
class IPrsAttr
{
public:
	// Destroy and free the attribute information
	virtual void AttrDestroy()=0;
};

/*
	IPrsNode - Abstract syntax tree node interface
*/
class IPrsNode
{
public:
	// Destroy and free the current node and all its children.
	// Also calls AttrDestroy on nodes' semantic attributes if non-null.
	virtual bool NodeDestroy()=0;
	
	// Get the node tag value of this node from the tags supplied
	// at registration time.
	virtual unsigned long NodeGetTag()=0;
	
	// Get the lexical token for this node if it's a terminal node.
	// For non-terminals, this will return null.
	virtual SLexToken* NodeGetToken()=0;

	// Get the parent node, or null if the root.
	virtual IPrsNode* NodeGetParent()=0;

	// Get the number of children beneath this node, may include null
	// children if marker values are skipped in non-terminal productions.
	virtual unsigned long NodeGetChildCount()=0;

	// Get the child node for a given index from zero through childcount-1.
	// May include null children if marker values are skipped in non-terminal
	// productions.
	virtual IPrsNode* NodeGetChild(unsigned long inIndex)=0;

	// Set/get semantic attribute information for the node, used by client
	// application.  All nodes initially have this set to null.
	virtual void NodeSetAttr(IPrsAttr* inAttr)=0;
	virtual IPrsAttr* NodeGetAttr()=0;
};

/*
	IPrsParser - Syntactical analyzer (parser) interface
*/
class IPrsParser
{
public:
	// Create a parser interface allowing the given number of lookahead tokens.
	// At this time, lookahead values greater than zero are supported, but may
	// return extremely confusing results if a syntax error occurs due to the
	// lack of backtracking markers.  For this reason, a zero lookahead is
	// recommended (most grammar productions can be written in a zero lookahead
	// form).
	static IPrsParser* PrsCreate(unsigned long inLookAhead=0);

	// Destroy and free the interface; will automatically destroy child lexer.
	virtual bool PrsDestroy()=0;

	// Returns an interface to the lexer used by the parser, for registering
	// terminal tokens matching RegisterTerm tags given to the parser.
	// Lexer commands must be issued before Build() is called in order for them
	// to take effect.  Note that Finalize() should not be called on the lexer,
	// as it will be altered during the Build process and will be finalized
	// automatically.
	virtual ILexLexer* PrsGetLexer()=0;

	// Registers a terminal rule using a given lexer tag and a given output
	// node tag for the AST nodes.  Terminal nodes are generated corresponding
	// to lexer tokens matching this tag value.
	virtual bool PrsRegisterT(const char* inRule, unsigned long inLexTag, unsigned long inNodeTag)=0;

	// Registers a non-terminal rule using a given production string from other
	// rules (with the production items containing child node markers) and an
	// output AST node tag.  Non-terminal nodes are generated bottom-up based
	// on the shift-reduce pattern matches of terminals.
	virtual bool PrsRegisterNT(const char* inRule, const char* inProduction, unsigned long inNodeTag)=0;

	// Build the parser based on the terminal and nonterminal rules registered
	// prior to this call.  This somewhat tedious function should only be called
	// once before the parser is first executed, and will fail if called multiple
	// times on the same parser.
	virtual bool PrsBuild()=0;

	// Sets an optional text alias to associate with a given rule terminal or
	// nonterminal when reporting errors.  The terminal/nonterminal must have
	// had at least one rule registered to it before calling this function.
	virtual bool PrsSetRuleAlias(const char* inRule, const char* inAlias)=0;

	// Executes the parser on a given buffer of text, returning the root of the
	// generated abstract syntax tree, or null if an error occured (likely a parse
	// syntax error).  The tab spacing parameter configures the lexer with the
	// corresponding tab spacing adjustment for line and column values.
	virtual IPrsNode* PrsExecute(const char* inText, unsigned long inTabSpacing=8)=0;

	// Returns a descriptive error string when parser functions return failure.
	// Currently only used with parse errors reported by Execute.
	virtual char* PrsGetLastError(unsigned long* outLine=0, unsigned long* outColumn=0)=0;
};

//============================================================================
//    GLOBAL FUNCTIONS
//============================================================================
// Set library to use the given client application interface (generally called
// before any interface creation functions).  This can be called as many times
// as desired, to change the client memory allocator at will.  Memory blocks
// allocated with previous clients will still be freed using those previous
// clients (applications should use static client implementation objects for
// this reason, not a client that may be dynamically destroyed).  Setting a
// null client will restore the default implementation.  Returns previous client.
ILibClient* LIB_SetClient(ILibClient* inClient);

// Get the current client interface being used, initially the default client.
ILibClient* LIB_GetClient();

// Return version number of the library
float LIB_GetVersion();

} // namespace FEAST

//****************************************************************************
//**
//**    END HEADER FEAST.H
//**
//****************************************************************************
#endif // __FEAST_H__
