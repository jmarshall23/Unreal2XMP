
/*----------------------------------------------------------------------------
	TMatrix.
----------------------------------------------------------------------------*/

//
// Used for creating dynamic two dimensional arrays of items (objects, ints, etc).
//
// Data(y)(x): y = height index, x = width index.
//

// NOTE[aleiby]: This is a straightforward implementation with zero optimization.
// As such it is exceptionally slow.

template< class T > class TMatrix
{
public:
	TMatrix()
	:	Data()
	{}
	TMatrix( INT Height, INT Width )
	:	Data()
	{
		guard(TMatrix::TMatrix size);
		SetSize(Height,Width);
		unguard;
	}
	TMatrix( INT Height, INT Width, T(*Func)() )
	:	Data()
	{
		guard(TMatrix::TMatrix init);
		SetSize(Height,Width);
		InitAll(Func);
		unguard;
	}
	TMatrix( const TArray<T>& A )
	:	Data()
	{
		guard(TMatrix::TArray copy);
		new(Data)TArray<T>(A);
		unguard;
	}
	TMatrix( const TMatrix& Other )
	:	Data()
	{
		guard(TMatrix::TMatrix copy);
		SetSize(Other.Height(),Other.Width());
		for( INT y=0; y<Other.Data.Num(); y++ )
			for( INT x=0; x<Other.Data(y).Num(); x++ )
				Data(y)(x) = Other.Data(y)(x);
		unguard;
	}
	~TMatrix()
	{
		guard(TMatrix::~TMatrix);
		Empty();
		unguard;
	}
	// takes a pointer to a function that returns a value of type T to initilize our elements with
	// useful for setting all entries to a random number for example.
	TMatrix InitAll( T(*Func)() )
	{
		guard(InitAll);
		for( INT y=0; y<Data.Num(); y++ )
			for( INT x=0; x<Data(y).Num(); x++ )
				Data(y)(x) = (*Func)();
		return *this;
		unguard;
	}
	// takes a pointer to a function that takes a value of type T, and returns a value of type T.
	// useful for modifying all the elements in a matrix.
	TMatrix ApplyAll( T(*Func)(T) )
	{
		guard(InitAll);
		for( INT y=0; y<Data.Num(); y++ )
			for( INT x=0; x<Data(y).Num(); x++ )
				Data(y)(x) = (*Func)(Data(y)(x));
		return *this;
		unguard;
	}
	void Empty()
	{
		guard(TMatrix::Empty);
		for( INT y=0; y<Data.Num(); y++ )
			Data(y).Empty();
		Data.Empty();
		unguard;
	}
	void SetSize( INT Height, INT Width )
	{
		guard(TMatrix::SetSize);
		check(Height>=0);
		check(Width>=0);
		while (Height > Data.Num()) new(Data)TArray<T>(Width);
		if    (Height < Data.Num()) Data.Remove( Height, Data.Num() - Height );
		check(Data.Num()==Height);
		for( INT y=0; y<Data.Num(); y++ )
		{
			if      (Width > Data(y).Num()) Data(y).AddZeroed( Width - Data(y).Num() );
			else if (Width < Data(y).Num()) Data(y).Remove( Width, Data(y).Num() - Width );
			check(Data(y).Num()==Width);
		}
		unguard;
	}
	INT Height() const
	{
		guard(TMatrix::Height);
		return Data.Num();
		unguard;
	}
	INT Width( INT y=0 ) const
	{
		guard(TMatrix::Width);
		return y<Data.Num() ? Data(y).Num() : 0;
		unguard;
	}
	T& operator()( INT y, INT x )
	{
		guard(TMatrix::operator());
		return Data(y)(x);
		unguard;
	}
	const T& operator()( INT y, INT x ) const
	{
		guard(TMatrix::operator());
		return Data(y)(x);
		unguard;
	}
	TMatrix operator+( const TMatrix& M ) const
	{
		guard(TMatrix::operator+ addition);
		check(M.Height()==Height());
		check(M.Width()==Width());
		TMatrix R(Height(),Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(y)(x) = Data(y)(x) + M.Data(y)(x);
		return R;
		unguard;
	}
	TMatrix operator-( const TMatrix& M ) const
	{
		guard(TMatrix::operator- subtraction);
		check(M.Height()==Height());
		check(M.Width()==Width());
		TMatrix R(Height(),Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(y)(x) = Data(y)(x) - M.Data(y)(x);
		return R;
		unguard;
	}
	TMatrix operator*( const TMatrix& M ) const
	{
		guard(TMatrix::operator* simple multiply);
		check(M.Height()==Height());
		check(M.Width()==Width());
		TMatrix R(Height(),Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(y)(x) = Data(y)(x) * M.Data(y)(x);
		return R;
		unguard;
	}
	TMatrix operator/( const TMatrix& M ) const
	{
		guard(TMatrix::operator/ simple divide);
		check(M.Height()==Height());
		check(M.Width()==Width());
		TMatrix R(Height(),Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(y)(x) = Data(y)(x) / M.Data(y)(x);
		return R;
		unguard;
	}
	TMatrix operator^( const TMatrix& M ) const
	{
		guard(TMatrix::operator^ matrix multiply);
		check(Width()==M.Height());
		TMatrix R(Height(),M.Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<M.Width(); x++ )
			{
				R.Data(y)(x) = 0.f;
				for( INT i=0; i<Width(); i++ )
					R.Data(y)(x) += Data(y)(i) * M.Data(i)(x);
			}
		return R;
		unguard;
	}
	TMatrix operator+( FLOAT F ) const
	{
		guard(TMatrix::operator+ float);
		TMatrix R(Height(),Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(y)(x) = Data(y)(x) + F;
		return R;
		unguard;
	}
	TMatrix operator-( FLOAT F ) const
	{
		guard(TMatrix::operator- float);
		TMatrix R(Height(),Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(y)(x) = Data(y)(x) - F;
		return R;
		unguard;
	}
	TMatrix operator*( FLOAT F ) const
	{
		guard(TMatrix::operator* float);
		TMatrix R(Height(),Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(y)(x) = Data(y)(x) * F;
		return R;
		unguard;
	}
	TMatrix operator/( FLOAT F ) const
	{
		guard(TMatrix::operator/ float);
		TMatrix R(Height(),Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(y)(x) = Data(y)(x) / F;
		return R;
		unguard;
	}
	friend TMatrix operator+( FLOAT F, const TMatrix& M )
	{
		guard(TMatrix::operator+ float friend);
		TMatrix R(M.Height(),M.Width());
		for( INT y=0; y<M.Height(); y++ )
			for( INT x=0; x<M.Width(); x++ )
				R.Data(y)(x) = F + M.Data(y)(x);
		return R;
		unguard;
	}
	friend TMatrix operator-( FLOAT F, const TMatrix& M )
	{
		guard(TMatrix::operator- float friend);
		TMatrix R(M.Height(),M.Width());
		for( INT y=0; y<M.Height(); y++ )
			for( INT x=0; x<M.Width(); x++ )
				R.Data(y)(x) = F - M.Data(y)(x);
		return R;
		unguard;
	}
	friend TMatrix operator*( FLOAT F, const TMatrix& M )
	{
		guard(TMatrix::operator* float friend);
		TMatrix R(M.Height(),M.Width());
		for( INT y=0; y<M.Height(); y++ )
			for( INT x=0; x<M.Width(); x++ )
				R.Data(y)(x) = F * M.Data(y)(x);
		return R;
		unguard;
	}
	friend TMatrix operator/( FLOAT F, const TMatrix& M )
	{
		guard(TMatrix::operator/ float friend);
		TMatrix R(M.Height(),M.Width());
		for( INT y=0; y<M.Height(); y++ )
			for( INT x=0; x<M.Width(); x++ )
				R.Data(y)(x) = F / M.Data(y)(x);
		return R;
		unguard;
	}
	TMatrix operator-() const
	{
		guard(TMatrix::operator- unary);
		TMatrix R(Height(),Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(y)(x) = -Data(y)(x);
		return R;
		unguard;
	}
	TMatrix operator+=( const TMatrix& M )
	{
		guard(TMatrix::operator+=);
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				Data(y)(x) += M.Data(y)(x);
		return *this;
		unguard;
	}
	TMatrix operator-=( const TMatrix& M )
	{
		guard(TMatrix::operator-=);
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				Data(y)(x) -= M.Data(y)(x);
		return *this;
		unguard;
	}
	TMatrix operator*=( const TMatrix& M )
	{
		guard(TMatrix::operator*=);
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				Data(y)(x) *= M.Data(y)(x);
		return *this;
		unguard;
	}
	TMatrix operator/=( const TMatrix& M )
	{
		guard(TMatrix::operator/=);
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				Data(y)(x) /= M.Data(y)(x);
		return *this;
		unguard;
	}
	TMatrix Transpose() const
	{
		guard(TMatrix::Transpose);
		TMatrix R(Width(),Height());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(x)(y) = Data(y)(x);
		return R;
		unguard;
	}
	FLOAT Determinant() const
	{
		guard(TMatrix::Determinant);
		check(Width()==Height());
		if( Width()==0 ) return 0.f;
		if( Width()==1 ) return Data(0)(0);
		if( Width()==2 ) return Data(0)(0)*Data(0)(1) - Data(1)(0)*Data(1)(1);
		FLOAT D=0.f;
		for( INT x=0; x<Width(); x++ )
			D += Data(0)(x) * Submatrix(0,x).Determinant() * ((x%2) ? 1.f : -1.f);
		unguard;
	}
	TMatrix Inverse() const
	{
		guard(TMatrix::Inverse);
		FLOAT D=Determinant();
		TMatrix R(Height(),Width());
		for( INT y=0; y<Height(); y++ )
			for( INT x=0; x<Width(); x++ )
				R.Data(y)(x) = Submatrix(y,x).Determinant() / D * ((i+j)%2 ? -1.f : 1.f);
		return R.Transpose();
		unguard;
	}
	TMatrix Submatrix( INT Y, INT X ) const
	{
		guard(TMatrix::Submatrix);
		TMatrix R(Height()-1,Width()-1);
		for( INT y=0; y<Height(); y++ )
			if( y!=Y ) for( INT x=0; x<Width(); x++ )
				if( x!=X ) R.Data(y)(x) = Data(y)(x);
		unguard;
	}
	friend FArchive& operator<<( FArchive& Ar, TMatrix& M )
	{
		guard(TMatrix<<);
		return Ar << M.Data;
		unguard;
	}
	// takes a pointer to a function that converts our type T to FStrings.
	void Dump( FOutputDevice& Ar, FString(*StrFunc)(T) ) const
	{
		guard(TMatrix::Dump);
		FString S=TEXT("");
		for( INT y=0; y<Data.Num(); y++ )
		{
			for( INT x=0; x<Data(y).Num(); x++ )
			{
				S += (*StrFunc)( Data(y)(x) ) + TEXT(" ");
			}
			S += TEXT("\n");
		}
		Ar.Logf(TEXT("%s"), S );
		unguard;
	}
	static UBOOL Test( FOutputDevice* Ar=NULL )
	{
		guard(TMatrix::Test);

		if(!Ar) return false;	// currently not set up to support non-logging version.

		struct InitA { FLOAT Func(){ return 2.f; } };
		struct InitB { FLOAT Func(){ return 3.f; } };
		struct Print { FString Func( FLOAT f ){ return FString::Printf(TEXT("%f"),f); } }

		TMatrix<FLOAT> A=TMatrix<FLOAT>(4,4,&InitA::Func);
		TMatrix<FLOAT> B=TMatrix<FLOAT>(4,4,&InitB::Func);
		A.Dump( *Ar, &Print::Func );
		B.Dump( *Ar, &Print::Func );

		A.SetSize(6,6);
		Ar->Logf(TEXT("Height: %d Width: %d"),A.Height(),A.Width());
		A.Dump( *Ar, &Print::Func );

		A.SetSize(4,4);
		Ar->Logf(TEXT("Height: %d Width: %d"),A.Height(),A.Width());
		A.Dump( *Ar, &Print::Func );

		(A+B).Dump( *Ar, &Print::Func );
		(A-B).Dump( *Ar, &Print::Func );
		(A*B).Dump( *Ar, &Print::Func );
		(A/B).Dump( *Ar, &Print::Func );
		(A^B).Dump( *Ar, &Print::Func );

		(-A).Dump( *Ar, &Print::Func );

		A.Transpose().Dump( *Ar, &Print::Func );
		A.Inverse().Dump( *Ar, &Print::Func );
		
		Ar->Logf(TEXT("Determinant: %f"),A.Determinant());

		return false;	// currently not set up to support correct return value.
		unguard;
	}
protected:
	TArray< TArray< T > > Data;
};


