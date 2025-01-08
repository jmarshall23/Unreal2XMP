class PARTICLESYSTEMS_API UGeneratorPrimitive : public UPrimitive
{
	DECLARE_CLASS(UGeneratorPrimitive,UPrimitive,0,Engine);

	// UPrimitive interface
	virtual UBOOL PointCheck
	(
		FCheckResult	&Result,
		AActor			*Owner,
		FVector			Location,
		FVector			Extent,
		DWORD           ExtraNodeFlags
	);
	virtual UBOOL LineCheck
	(
		FCheckResult	&Result,
		AActor			*Owner,
		FVector			End,
		FVector			Start,
		FVector			Extent,
		DWORD           ExtraNodeFlags
	);
	virtual FBox GetRenderBoundingBox( const AActor* Owner );
	virtual FSphere GetRenderBoundingSphere( const AActor* Owner );
	virtual FBox GetCollisionBoundingBox( const AActor* Owner ) const;
	virtual void Destroy();
};
