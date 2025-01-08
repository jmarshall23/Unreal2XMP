
static TArray<UDestroyOnDamage*> Instances;

UDestroyOnDamage();
void Destroy();
//void Notify( FVector Origin, FLOAT DamageRadius, UClass* DamageType );
void Notify( AActor* Other, FBox& BBox );

//static void StaticNotify( ULevel* Level, FVector Origin, FLOAT DamageRadius, UClass* DamageType );
static void StaticNotify( AActor* Other );

