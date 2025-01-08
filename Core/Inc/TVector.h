
/*=============================================================================
	TVector.h: Generic vector support using templates.
	Copyright 2001 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Expanded and rewritten by Aaron Leiby using source from Tomas Arce
		  http://www.flipcode.com/tutorials/tut_fastmath.shtml.
=============================================================================*/

/*----------------------------------------------------------------------------
	Pragmas.
----------------------------------------------------------------------------*/

#pragma warning( disable: 4786 ) // identifier was truncated to '255' characters in the debug information

#pragma inline_depth( 255 )
#pragma inline_recursion( on )
#pragma auto_inline( on )

#define inline __forceinline 

/*----------------------------------------------------------------------------
	Includes.
----------------------------------------------------------------------------*/

#include "Tensors.h"

/*----------------------------------------------------------------------------
	TVector.
----------------------------------------------------------------------------*/

namespace TVector
{
	/*----------------------------------------------------------------------------
		Arguments.
	----------------------------------------------------------------------------*/

	template< class ta >
	class vecarg
	{
		const ta& A;
	public:
		enum { DIM=ta::DIM };
		inline vecarg( const ta& a ) : A(a) {}
		inline const FLOAT Evaluate( const INT i ) const 
		{ return A.Evaluate( i ); }
	};

	template<> 
	class vecarg< const FLOAT >
	{
		const ta& A;
	public:
		enum { DIM=1 };
		inline vecarg( const ta& a ) : A(a) {}
		inline const FLOAT Evaluate( const INT i ) const 
		{ return A; }
	};

	template<> 
	class vecarg< const INT >
	{
		const ta& A;
	public:
		enum { DIM=1 };
		inline vecarg( const ta& a ) : A(a) {}
		inline const FLOAT Evaluate( const INT i ) const 
		{ return (FLOAT)A; }
	};

	/*----------------------------------------------------------------------------
		Expressions.
	----------------------------------------------------------------------------*/

	template< class ta, class tb, class teval >
	class vecexp_2
	{
		const vecarg<ta> A;
		const vecarg<tb> B;
	public:
		enum { DIM=vecarg<ta>::DIM };
		inline vecexp_2( const ta& a, const tb& b ) : A(a), B(b) {}
		inline const FLOAT Evaluate( const INT i ) const
		{ return teval::Evaluate( i, A, B ); }
	};

	template< class ta, class teval >
	class vecexp_1
	{
		const vecarg<ta> A;
	public:
		enum { DIM=vecarg<ta>::DIM };
		inline vecexp_1( const ta& a ) : A(a) {}
		inline const FLOAT Evaluate( const INT i ) const
		{ return teval::Evaluate( i, A ); }
	};

	/*----------------------------------------------------------------------------
		Base class.
	----------------------------------------------------------------------------*/

	template< class T >
	struct base : public T
	{
		enum { DIM=T::DIM };

		/*----------------------------------------------------------------------------
			Structors.
		----------------------------------------------------------------------------*/

		inline base(): T(){}
		template< class ta >
		inline base( const ta& A )
		{ *this = A; }

		/*----------------------------------------------------------------------------
			Accessors.
		----------------------------------------------------------------------------*/

		inline		 FLOAT&  operator[]( const INT i )		 { check(i<DIM); return ((FLOAT*)this)[i]; } 
		inline const FLOAT	 Evaluate  ( const INT i ) const { check(i<DIM); return ((FLOAT*)this)[i]; }

		/*----------------------------------------------------------------------------
			Assignement operator macro definition.
		----------------------------------------------------------------------------*/

		#define ASSIGN_OP_START(name,op) \
		template< class ta, class tb > struct name##_scope \
		{ \
			template< INT i > struct recurse \
			{ \
				static inline void name( ta& A, const tb& B ) \
				{
		#define ASSIGN_OP_END(name,op) \
					recurse<i+1>::name( A, B ); \
				} \
			}; \
			template<> struct recurse<DIM> \
			{ \
				static inline void name( ta& A, const tb& B ){} \
			}; \
			static inline void name( ta& A, const tb& B ) \
			{ recurse<0>::name( A, B ); } \
		}; \
		template< class ta > inline const base<T>& operator op ( const ta& A ) \
		{ \
			name##_scope< base<T>, vecarg< const ta > >::name( *this, vecarg< const ta >( A ) ); \
			return *this; \
		} \

		#define ASSIGN_OP(name,op) \
			ASSIGN_OP_START(name,op) \
				A[i] op B.Evaluate(i); \
			ASSIGN_OP_END(name,op)

		/*----------------------------------------------------------------------------
			Assignement operators.
		----------------------------------------------------------------------------*/

		ASSIGN_OP(_assign,=)
		ASSIGN_OP(_addeql,+=)
		ASSIGN_OP(_subeql,-=)
		ASSIGN_OP(_muleql,*=)
		ASSIGN_OP(_diveql,/=)

		/*----------------------------------------------------------------------------
			Cross product:
				A x B = Eijk Aj Bk

			This is really just the outer product of two bivectors, but it has the
			nice property that it generalizes to the cross product in 3-space.
			(Also known as the Wedge Product.)

			for( INT i=0; i<DIM; i++ )
				for( INT j=0; j<DIM; j++ )
					for( INT k=0; k<DIM; k++ )
						C[i] += LeviCivita::E(i,j,k) * A.Evaluate(j) * B.Evaluate(k);
		----------------------------------------------------------------------------*/

		struct cross_scope
		{
			// Fix ARL: It would be nice to capture the unrolling properties of these
			// templates in an easy-to-use / easy-to-read macro.  Unfortunately, we 
			// need to specify the inner function prototype multiple times within the 
			// defintion, and since it has commas in it, there's no way to pass it in 
			// as a single parameter to the macro.
			template< INT i > struct recurse_i
			{
				template< INT j > struct recurse_j
				{
					template< INT k > struct recurse_k
					{
						static inline const FLOAT Cross( const base<T>& A, const base<T>& B )
						{
							if( LeviCivita::E(i,j,k) > 0 ) return + A.Evaluate(j) * B.Evaluate(k) + recurse_k<k+1>::Cross( A, B );
							if( LeviCivita::E(i,j,k) < 0 ) return - A.Evaluate(j) * B.Evaluate(k) + recurse_k<k+1>::Cross( A, B );
							return recurse_k<k+1>::Cross( A, B );
						}
					};
					template<> struct recurse_k<DIM>
					{
						static inline const FLOAT Cross( const base<T>& A, const base<T>& B )
						{ return 0.f; }
					};
					static inline const FLOAT Cross( const base<T>& A, const base<T>& B )
					{
						return recurse_k<0>::Cross( A, B ) + recurse_j<j+1>::Cross( A, B );
					}
				};
				template<> struct recurse_j<DIM>
				{
					static inline const FLOAT Cross( const base<T>& A, const base<T>& B )
					{ return 0.f; }
				};
				static inline void Cross( const base<T>& A, const base<T>& B, base<T>& C )
				{
					C[i] = recurse_j<0>::Cross( A, B );
					recurse_i<i+1>::Cross( A, B, C );
				}
			};
			template<> struct recurse_i<DIM>
			{
				static inline void Cross( const base<T>& A, const base<T>& B, base<T>& C ){}
			};
			static inline void Cross( const base<T>& A, const base<T>& B, base<T>& C )
			{ recurse_i<0>::Cross( A, B, C ); }
		};
		template< class ta > inline
		const base<T> operator ^ ( const ta& A ) const
		{
			base<T> B = base<T>();
			cross_scope::Cross( *this, A, B );
			return B;
		}
		// NOTE[aleiby]: This *should* account for (A+B) cross C, and (A+B) cross (C+D), etc.
		// (basically any situation where you wind up with an expression on the left), but
		// unfortunately the compiler refuses to expand this template function (probably 
		// something to do with templated friend functions within templated classes - I couldn't 
		// find anything about it on the net).  The error you'll get will be something like this: 
		// "error C2679: binary '^' : no operator defined which takes a right-hand operand of 
		// type 'struct Vector3' (or there is no acceptable conversion)"
		template< class ta > inline friend
		const base<T> operator ^ ( const ta& A, const base<T>& B )
		{
			base<T> C = base<T>();
			cross_scope::Cross( A, B, C );
			return C;
		}
	};

	/*----------------------------------------------------------------------------
		Simple operator macro definition.
	----------------------------------------------------------------------------*/

	#define SIMPLE_OP_START(name,op) \
	struct name \
	{ \
		template< class ta, class tb > inline static \
		const FLOAT Evaluate( const INT i, const ta& A, const tb& B ) \
		{
	#define SIMPLE_OP_END(name,op) \
		} \
	}; \
	template< class ta, class tb > inline \
	const vecexp_2< const ta, const tb, name > \
	operator op ( const ta& A, const tb& B ) \
	{ \
		return vecexp_2< const ta, const tb, name >( A, B ); \
	} \

	#define SIMPLE_OP(name,op) \
		SIMPLE_OP_START(name,op) \
			return A.Evaluate(i) op B.Evaluate(i); \
		SIMPLE_OP_END(name,op)

	/*----------------------------------------------------------------------------
		Simple operators.
	----------------------------------------------------------------------------*/

	SIMPLE_OP(_add,+)
	SIMPLE_OP(_sub,-)
	SIMPLE_OP(_mul,*)
	SIMPLE_OP(_div,/)

	/*----------------------------------------------------------------------------
		Unary operator macro definition.
	----------------------------------------------------------------------------*/

	#define UNARY_OP_START(name,op) \
	struct name \
	{ \
		template< class ta > inline static \
		const FLOAT Evaluate( const INT i, const ta& A ) \
		{
	#define UNARY_OP_END(name,op) \
		} \
	}; \
	template< class ta > inline \
	const vecexp_1< const ta, name > \
	operator op ( const ta& A ) \
	{ \
		return vecexp_1< const ta, name >( A ); \
	} \

	#define UNARY_OP(name,op) \
		UNARY_OP_START(name,op) \
			return op A.Evaluate(i); \
		UNARY_OP_END(name,op)

	/*----------------------------------------------------------------------------
		Unary operators.
	----------------------------------------------------------------------------*/

	UNARY_OP(_neg,-)

	/*----------------------------------------------------------------------------
		Complex operator macro definition.
	----------------------------------------------------------------------------*/

	#define COMPLEX_OP_START(name,op,type,def) \
	template< class ta, class tb > struct name##_scope \
	{ \
		template< INT i > struct recurse \
		{ \
			static inline const type name( const ta& A, const tb& B ) \
			{ \
				return 
	#define COMPLEX_OP_END(name,op,type,def) \
				recurse<i+1>::name( A, B ); \
			} \
		}; \
		template<> struct recurse<vecarg<ta>::DIM> \
		{ \
			static inline const type name( const ta& A, const tb& B ) \
			{ return def; } \
		}; \
		static inline const type name( const ta& A, const tb& B ) \
		{ return recurse<0>::name( A, B ); } \
	}; \
	template< class ta, class tb > inline \
	const type operator op ( const ta& A, const tb& B ) \
	{ \
		return name##_scope< vecarg< const ta >, vecarg< const tb > > \
			::name( vecarg< const ta >( A ), vecarg< const tb >( B ) ); \
	} \

	#define COMPLEX_OP(name,op,type,func,def) \
		COMPLEX_OP_START(name,op,type,def) \
			func \
		COMPLEX_OP_END(name,op,type,def)

	/*----------------------------------------------------------------------------
		Complex operators.
	----------------------------------------------------------------------------*/

	COMPLEX_OP(_equals,==,UBOOL,A.Evaluate(i)==B.Evaluate(i)&&, true)
	COMPLEX_OP(_nequal,!=,UBOOL,A.Evaluate(i)!=B.Evaluate(i)||,false)
	COMPLEX_OP(_dotprd, |,FLOAT,A.Evaluate(i) *B.Evaluate(i) +,  0.f)

	/*----------------------------------------------------------------------------
		Data.
	----------------------------------------------------------------------------*/

	struct desc_xyz
	{
		enum { DIM=3 };
		FLOAT X,Y,Z;
	};

	struct desc_xy
	{
		enum { DIM=2 };
		FLOAT X,Y;
	};

	struct desc_uv
	{
		enum { DIM=2 };
		FLOAT U,V;
	};

	struct desc_8d
	{
		enum { DIM=8 };
		FLOAT D[DIM];
	};
};

/*----------------------------------------------------------------------------
	Vector definition macros.
----------------------------------------------------------------------------*/

#define VEC_DEF_START(name,desc) \
	struct name : public TVector::base< TVector::desc > \
	{ \
		typedef TVector::base< TVector::desc > base; \
		inline name(){} \
		template< class ta > inline name( const ta& A ) \
		{ base::operator = ( A ); } \
		template< class ta > inline \
		const name& operator = ( const ta& A ) \
		{ base::operator = ( A ); return *this; } \
		/* additional structors go here */
#define VEC_DEF_END \
	};

/*----------------------------------------------------------------------------
	Vector3.
----------------------------------------------------------------------------*/

VEC_DEF_START(Vector3,desc_xyz)
	inline Vector3( const FLOAT x, const FLOAT y, const FLOAT z )
	{ X=x; Y=y; Z=z; }
	inline const Vector3 operator ^ ( const Vector3& A ) const
	{
		return Vector3
		(
			Y * A.Z - Z * A.Y,
			Z * A.X - X * A.Z,
			X * A.Y - Y * A.X
		);
	}
VEC_DEF_END

/*----------------------------------------------------------------------------
	Vector2.
----------------------------------------------------------------------------*/

VEC_DEF_START(Vector2,desc_xy)
	inline Vector2( const FLOAT x, const FLOAT y )
	{ X=x; Y=y; }
VEC_DEF_END

/*----------------------------------------------------------------------------
	TexCoords.
----------------------------------------------------------------------------*/

VEC_DEF_START(TexCoords,desc_uv)
	inline TexCoords( const FLOAT u, const FLOAT v )
	{ U=u; V=v; }
VEC_DEF_END

/*----------------------------------------------------------------------------
	Eight dimensional vector (octonion).
----------------------------------------------------------------------------*/

VEC_DEF_START(Vector8,desc_8d)
VEC_DEF_END

