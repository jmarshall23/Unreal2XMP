//=============================================================================
// StandardFeastNodes.h
//=============================================================================

#include <math.h>

#define EPSILON 0.001f

/*----------------------------------------------------------------------------
	Tokens.
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Token: Dot (stays ID=1 because it was explicitly 1, not (T))
----------------------------------------------------------------------------*/
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(1);
	DEF_TYPE		(NT_Token);
	DEF_PRIORITY	(0);
	DEF_NAME		("Dot");
	DEF_REGEX		(".");
};

/*----------------------------------------------------------------------------
  Token: Whitespace (T -> 2)
----------------------------------------------------------------------------*/
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(2);
	DEF_TYPE		(NT_Token);
	DEF_PRIORITY	(1);
	DEF_NAME		("Whitespace");
	DEF_REGEX		("[ \\t\\n]*");
};

/*----------------------------------------------------------------------------
  Token: EolComment (T -> 3)
----------------------------------------------------------------------------*/
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(3);
	DEF_TYPE		(NT_Token);
	DEF_PRIORITY	(1);
	DEF_NAME		("EolComment");
	DEF_REGEX		("//.*");
};

/*----------------------------------------------------------------------------
  Token: BlockComment (originally template<DWORD T>, but we remove the template
  parameter and replace T -> 4)
----------------------------------------------------------------------------*/
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(4);
	DEF_TYPE		(NT_Token);
	DEF_PRIORITY	(1);
	DEF_NAME		("BlockComment");
	DEF_REGEX		("\\/\\*");
};

/*----------------------------------------------------------------------------
  Terminals (and more Tokens).
----------------------------------------------------------------------------*/

/* TermDecInteger (T -> 5) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(5);
	DEF_TYPE		(NT_Token | NT_Terminal);
	DEF_PRIORITY	(1);
	DEF_NAME		("TermDecInteger");
	DEF_RULE		("integer");
	DEF_PRODUCTION	("DECINTEGER");
	DEF_REGEX		("[0-9]+");

	EVALUATE_T_START
		Term = Term.Left( inNode->NodeGetToken()->mLexemeLen );
		Result = appAtoi( *Term );
		return true;
	EVALUATE_T_END
};

/* TermHexInteger (T -> 6) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(6);
	DEF_TYPE		(NT_Token | NT_Terminal);
	DEF_PRIORITY	(1);
	DEF_NAME		("TermHexInteger");
	DEF_RULE		("integer");
	DEF_PRODUCTION	("HEXINTEGER");
	DEF_REGEX		("0[xX][0-9a-fA-F]+");

	EVALUATE_T_START
		Term = Term.Left( inNode->NodeGetToken()->mLexemeLen );
		Result = appAtoi( *Term );
		return true;
	EVALUATE_T_END
};

/* TermBinInteger (T -> 7) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(7);
	DEF_TYPE		(NT_Token | NT_Terminal);
	DEF_PRIORITY	(1);
	DEF_NAME		("TermBinInteger");
	DEF_RULE		("integer");
	DEF_PRODUCTION	("BININTEGER");
	DEF_REGEX		("0[bB][0-1]+");

	EVALUATE_T_START
		Term = Term.Left( inNode->NodeGetToken()->mLexemeLen );
		Result = appAtoi( *Term );
		return true;
	EVALUATE_T_END
};

/* TermOctInteger (T -> 8) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(8);
	DEF_TYPE		(NT_Token | NT_Terminal);
	DEF_PRIORITY	(1);
	DEF_NAME		("TermOctInteger");
	DEF_RULE		("integer");
	DEF_PRODUCTION	("OCTINTEGER");
	DEF_REGEX		("0[qQ][0-7]+");

	EVALUATE_T_START
		Term = Term.Left( inNode->NodeGetToken()->mLexemeLen );
		Result = appAtoi( *Term );
		return true;
	EVALUATE_T_END
};

/* TermIdentifier (T -> 9) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(9);
	DEF_TYPE		(NT_Token | NT_Terminal);
	DEF_PRIORITY	(1);
	DEF_NAME		("TermIdentifier");
	DEF_RULE		("primary_expr");
	DEF_PRODUCTION	("IDENTIFIER");
	DEF_REGEX		("[a-zA-Z_]([a-zA-Z0-9_])*");

	EVALUATE_T_START
		Term = Term.Left( inNode->NodeGetToken()->mLexemeLen );
		Result = Term;
		return true;
	EVALUATE_T_END
};

/* TermCharacter (T -> 10) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(10);
	DEF_TYPE		(NT_Token | NT_Terminal);
	DEF_PRIORITY	(1);
	DEF_NAME		("TermCharacter");
	DEF_RULE		("primary_expr");
	DEF_PRODUCTION	("CHARACTER");
	DEF_REGEX		("'(\\\\.|[^\\\\'])+'");

	EVALUATE_T_START
		Term = Term.Mid( 1, inNode->NodeGetToken()->mLexemeLen - 2 );
		FName Name = FName( *Term, FNAME_Find );
		NOTE(debugf(TEXT("Searching for name: '%s'  %s"), *Term, (Name!=NAME_None) ? TEXT("Found") : TEXT("Not Found") ));
		Result = Name;
		return (Name != NAME_None) ? true : false;
	EVALUATE_T_END
};

/* TermString (T -> 11) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(11);
	DEF_TYPE		(NT_Token | NT_Terminal);
	DEF_PRIORITY	(1);
	DEF_NAME		("TermString");
	DEF_RULE		("primary_expr");
	DEF_PRODUCTION	("STRING");
	DEF_REGEX		("\\\"(\\\\.|[^\\\\\"])*\\\"");

	EVALUATE_T_START
		Term = Term.Mid( 1, inNode->NodeGetToken()->mLexemeLen - 2 );
		Result = Term;
		return true;
	EVALUATE_T_END
};

/* TermFloat (T -> 12) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(12);
	DEF_TYPE		(NT_Token | NT_Terminal);
	DEF_PRIORITY	(1);
	DEF_NAME		("TermFloat");
	DEF_RULE		("primary_expr");
	DEF_PRODUCTION	("FLOAT");
	DEF_REGEX		("([0-9]+[Ee][\\+\\-]?[0-9]+)|([0-9]*\\.[0-9]+([Ee][\\+\\-]?[0-9]+)?)|([0-9]+\\.[0-9]*([Ee][\\+\\-]?[0-9]+)?)");

	EVALUATE_T_START
		Term = Term.Left( inNode->NodeGetToken()->mLexemeLen );
		Result = appAtof( *Term );
		return true;
	EVALUATE_T_END
};

/*----------------------------------------------------------------------------
  Structural nodes (macros only, no (T) here).
----------------------------------------------------------------------------*/
STRUCTURE_NODE("start",         "expr"              )
STRUCTURE_NODE("primary_expr",  "integer"           )
STRUCTURE_NODE("primary_expr",  "'(' expr:1 ')'"    )
STRUCTURE_NODE("postfix_expr",  "primary_expr"      )
STRUCTURE_NODE("unary_expr",    "postfix_expr"      )
STRUCTURE_NODE("mul_expr",      "unary_expr"        )
STRUCTURE_NODE("add_expr",      "mul_expr"          )
STRUCTURE_NODE("rel_expr",      "add_expr"          )
STRUCTURE_NODE("equal_expr",    "rel_expr"          )
STRUCTURE_NODE("land_expr",     "equal_expr"        )
STRUCTURE_NODE("lor_expr",      "land_expr"         )
STRUCTURE_NODE("expr",          "lor_expr"          )


/*----------------------------------------------------------------------------
  Non-terminals (all had T in DEF_ID; we increment from 13 onward).
----------------------------------------------------------------------------*/

/* Neg (T -> 13) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(13);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("Neg");
	DEF_RULE		("unary_expr");
	DEF_PRODUCTION	("'-' mul_expr:1");

	EVALUATE_NT_START(1)
		bool b;		if( Operands[0].AsBool(b)  ){ Result = !b; return true; }
		INT i;		if( Operands[0].AsInt(i)   ){ Result = -i; return true; }
		FLOAT f;	if( Operands[0].AsFloat(f) ){ Result = -f; return true; }
	EVALUATE_NT_END
};

/* Add (T -> 14) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(14);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("Add");
	DEF_RULE		("add_expr");
	DEF_PRODUCTION	("add_expr:1 '+' mul_expr:2");

	EVALUATE_NT_START(2)
		FLOAT f0,f1; if( Operands[0].AsFloat(f0) && Operands[1].AsFloat(f1) ){ Result = (f0 + f1); return true; }
	EVALUATE_NT_END
};

/* Sub (T -> 15) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(15);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("Sub");
	DEF_RULE		("add_expr");
	DEF_PRODUCTION	("add_expr:1 '-' mul_expr:2");

	EVALUATE_NT_START(2)
		FLOAT f0,f1; if( Operands[0].AsFloat(f0) && Operands[1].AsFloat(f1) ){ Result = (f0 - f1); return true; }
	EVALUATE_NT_END
};

/* Mul (T -> 16) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(16);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("Mul");
	DEF_RULE		("mul_expr");
	DEF_PRODUCTION	("mul_expr:1 '*' unary_expr:2");

	EVALUATE_NT_START(2)
		FLOAT f0,f1; if( Operands[0].AsFloat(f0) && Operands[1].AsFloat(f1) ){ Result = (f0 * f1); return true; }
	EVALUATE_NT_END
};

/* Div (T -> 17) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(17);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("Div");
	DEF_RULE		("mul_expr");
	DEF_PRODUCTION	("mul_expr:1 '/' unary_expr:2");

	EVALUATE_NT_START(2)
		INT i0,i1;		if( Operands[0].AsInt(i0)   && Operands[1].AsInt(i1)   && i1 != 0             ){ Result = i0 / i1; return true; }
		FLOAT f0,f1;	if( Operands[0].AsFloat(f0) && Operands[1].AsFloat(f1) && fabsf(f1) > EPSILON ){ Result = f0 / f1; return true; }
	EVALUATE_NT_END
};

/* Mod (T -> 18) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(18);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("Mod");
	DEF_RULE		("mul_expr");
	DEF_PRODUCTION	("mul_expr:1 '%' unary_expr:2");

	EVALUATE_NT_START(2)
		INT i0,i1;		if( Operands[0].AsInt(i0)   && Operands[1].AsInt(i1)   && i1 != 0             ){ Result = (i0 % i1);		return true; }
		FLOAT f0,f1;	if( Operands[0].AsFloat(f0) && Operands[1].AsFloat(f1) && fabsf(f1) > EPSILON ){ Result = fmodf(f0, f1);	return true; }
	EVALUATE_NT_END
};

/* CmpLT (T -> 19) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(19);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("CmpLT");
	DEF_RULE		("rel_expr");
	DEF_PRODUCTION	("rel_expr:1 '<' add_expr:2");

	EVALUATE_NT_START(2)
		// INT's are promoted to floats by DataType::AsFloat
		FLOAT f0,f1;	if( Operands[0].AsFloat(f0)  && Operands[1].AsFloat(f1)  ){ Result = (f0 < f1);					return true; }
		FString s0,s1;	if( Operands[0].AsString(s0) && Operands[1].AsString(s1) ){ Result = appStrcmp( *s0, *s1 )<0;	return true; }
	EVALUATE_NT_END
};

/* CmpGT (T -> 20) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(20);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("CmpGT");
	DEF_RULE		("rel_expr");
	DEF_PRODUCTION	("rel_expr:1 '>' add_expr:2");

	EVALUATE_NT_START(2)
		FLOAT f0,f1;	if( Operands[0].AsFloat(f0)  && Operands[1].AsFloat(f1)  ){ Result = (f0 > f1);					return true; }
		FString s0,s1;	if( Operands[0].AsString(s0) && Operands[1].AsString(s1) ){ Result = appStrcmp( *s0, *s1 )>0;	return true; }
	EVALUATE_NT_END
};

/* CmpLE (T -> 21) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(21);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("CmpLE");
	DEF_RULE		("rel_expr");
	DEF_PRODUCTION	("rel_expr:1 \"<=\" add_expr:2");

	EVALUATE_NT_START(2)
		FLOAT f0,f1;	if( Operands[0].AsFloat(f0)  && Operands[1].AsFloat(f1)  ){ Result = (f0 <= f1);				return true; }
		FString s0,s1;	if( Operands[0].AsString(s0) && Operands[1].AsString(s1) ){ Result = appStrcmp( *s0, *s1 )<=0;	return true; }
	EVALUATE_NT_END
};

/* CmpGE (T -> 22) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(22);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("CmpGE");
	DEF_RULE		("rel_expr");
	DEF_PRODUCTION	("rel_expr:1 \">=\" add_expr:2");

	EVALUATE_NT_START(2)
		FLOAT f0,f1;	if( Operands[0].AsFloat(f0)  && Operands[1].AsFloat(f1)  ){ Result = (f0 >= f1);				return true; }
		FString s0,s1;	if( Operands[0].AsString(s0) && Operands[1].AsString(s1) ){ Result = appStrcmp( *s0, *s1 )>=0;	return true; }
	EVALUATE_NT_END
};

/* CmpEQ (T -> 23) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(23);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("CmpEQ");
	DEF_RULE		("equal_expr");
	DEF_PRODUCTION	("equal_expr:1 \"==\" rel_expr:2");

	EVALUATE_NT_START(2)
		bool b0,b1;		if( Operands[0].AsBool(b0)   && Operands[1].AsBool(b1)   ){ Result = (b0 == b1);				return true; }
		INT i0,i1;		if( Operands[0].AsInt(i0)	 && Operands[1].AsInt(i1)	 ){ Result = (i0 == i1);				return true; }
		FLOAT f0,f1;	if( Operands[0].AsFloat(f0)  && Operands[1].AsFloat(f1)  ){ Result = fabsf(f0-f1) < EPSILON;	return true; }
		FString s0,s1;	if( Operands[0].AsString(s0) && Operands[1].AsString(s1) ){ Result = appStrcmp( *s0, *s1 )==0;	return true; }
	EVALUATE_NT_END
};

/* CmpNE (T -> 24) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(24);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("CmpNE");
	DEF_RULE		("equal_expr");
	DEF_PRODUCTION	("equal_expr:1 \"!=\" rel_expr:2");

	EVALUATE_NT_START(2)
		bool b0,b1;		if( Operands[0].AsBool(b0)   && Operands[1].AsBool(b1)   ){ Result = (b0 != b1);				return true; }
		INT i0,i1;		if( Operands[0].AsInt(i0)	 && Operands[1].AsInt(i1)	 ){ Result = (i0 != i1);				return true; }
		FLOAT f0,f1;	if( Operands[0].AsFloat(f0)  && Operands[1].AsFloat(f1)  ){ Result = fabsf(f0-f1) >= EPSILON;	return true; }
		FString s0,s1;	if( Operands[0].AsString(s0) && Operands[1].AsString(s1) ){ Result = appStrcmp( *s0, *s1 )!=0;	return true; }
	EVALUATE_NT_END
};

/* LAnd (T -> 25) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(25);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("LAnd");
	DEF_RULE		("land_expr");
	DEF_PRODUCTION	("land_expr:1 \"&&\" equal_expr:2");

	EVALUATE_NT_START(2)
		bool b0,b1; if( Operands[0].AsBool(b0) && Operands[1].AsBool(b1) ){ Result = (b0 && b1); return true; }
	EVALUATE_NT_END
};

/* LOr (T -> 26) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(26);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("LOr");
	DEF_RULE		("lor_expr");
	DEF_PRODUCTION	("lor_expr:1 \"||\" land_expr:2");

	EVALUATE_NT_START(2)
		bool b0,b1; if( Operands[0].AsBool(b0) && Operands[1].AsBool(b1) ){ Result = (b0 || b1); return true; }
	EVALUATE_NT_END
};

/* LNot (T -> 27) */
template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(27);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("LNot");
	DEF_RULE		("unary_expr");
	DEF_PRODUCTION	("'!' mul_expr:1");

	EVALUATE_NT_START(1)
		bool b; if( Operands[0].AsBool(b) ){ Result = !b; return true; }
	EVALUATE_NT_END
};
