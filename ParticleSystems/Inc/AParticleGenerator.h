
// Interfaces
virtual void AddForce( UForce* F );
virtual void RemoveForce( UForce* F );
virtual void RemoveForceType( FName ClassName, UBOOL bAndSubclasses=0 );
virtual void AddTemplate( UParticleTemplate* T );
virtual void RemoveTemplate( UParticleTemplate* T );
virtual void RemoveTemplateType( FName ClassName, UBOOL bAndSubclasses=0 );
virtual void ParticleTick( FLOAT DeltaSeconds, FVector& Location, FRotator& Rotation, FLOAT AdditionalStasisTime=0.f ){}
virtual void Conform( AParticleGenerator* Image, UBOOL bDeleteExisting=1 );
virtual void ExchangeTemplate( UParticleTemplate* Old, UParticleTemplate* New );
virtual AParticleGenerator* Duplicate( ULevel* Level );

virtual void LockParticles();
virtual void UnLockParticles();

void ManualTick( FLOAT DeltaSeconds );
virtual void ForceTick( FLOAT DeltaSeconds );

// AActor overrides.
UBOOL Tick( FLOAT DeltaSeconds, ELevelTick TickType );
void Render( FDynamicActor* Owner, FLevelSceneNode* SceneNode, TList<FDynamicLight*>* Lights, FRenderInterface* RI );
UPrimitive* GetPrimitive();
void InitExecution();
void Spawned();
void Destroy();
void PostEditChange();

// Helpers.
UParticleTemplate* GetNextTemplate();
bool ForceIsIgnored( UForce* F );
FLOAT GetMaxLifeSpan();
INT NumParticles();
void Clean();
void DestroyComponents();
void FixUp();
void ValidateComponents();
void RegisterExternallyAffectedTemplates();
void UnRegisterExternallyAffectedTemplates();

// Static data.
static TMap< UObject*, UObject* >* GParticleDuplicates;
static TMap< UParticle*, UParticle* >* GDuplicateParticles;
static TMap< UForceVars*, UForceVars* >* GDuplicateForceVars;

