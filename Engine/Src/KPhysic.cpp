/*============================================================================
	Karma Integration
    
    - PHYS_Karma & PHYS_KarmaRagDoll funcs
	- Other C++ Karma-related member functions.
============================================================================*/
#include "EnginePrivate.h"

#ifdef WITH_KARMA

void AActor::KFreezeRagdoll()
{
	guard(AActor::KFreezeRagdoll);

	if( !MeshInstance || !MeshInstance->IsA(USkeletalMeshInstance::StaticClass()) )
	{
		debugf(TEXT("(Karma:) KFreezeRagdoll: No skeletal mesh."));
		return;
	}

	USkeletalMeshInstance* inst = Cast<USkeletalMeshInstance>(this->MeshInstance);
	if(!inst->KSkelIsInitialised)
	{
		debugf(TEXT("(Karma:) KFreezeRagdoll: Ragdoll not initialised."));
		return;
	}

	// Turn off/destroy Karma for rag-doll
	KTermSkeletonKarma(inst);

	// We have to be careful here - because setting the 'frozen' flag changes
	// which bounding-box the rag-doll uses.


#if 1
	// When we freeze a ragdoll, we drop it out of the world. Debateable.
    if( Physics == PHYS_KarmaRagDoll )
    {
        setPhysics(PHYS_Falling);
		Velocity = FVector(0, 0, 0);
    }
#endif

	ULevel* level = GetLevel();
	if(bCollideActors && level && level->Hash)
		level->Hash->RemoveActor(this);

	inst->KFrozen = 1;

	if(bCollideActors && level && level->Hash)
		level->Hash->AddActor(this);

	unguard;
}
#endif



/*****************************************************/

// Create a default KarmaParams for a newly created AKActor (as long as its not a constraint)
void AKActor::Spawned()
{
	guard(AKActor::Spawned);
    if( KParams == NULL && !this->IsA(AKConstraint::StaticClass()))
		KParams = ConstructObject<UKarmaParams>( UKarmaParams::StaticClass(), this->GetOuter() );
	unguard;
}

/*****************************************************/


#ifdef WITH_KARMA

// Returns actors model (or NULL if it has none, or is a constraint.
McdModelID AActor::getKModel() const
{
    if(!this->KParams)
        return 0;

    return ((McdModelID)this->KParams->KarmaData);
}

// Ensure this actor is awake (ie being simulated).
void AActor::KWake()
{
	guard(AActor::KWake);

	// If this is an actor with Karma dynamics, make sure they are enabled.
    McdModelID m = this->getKModel();
    if(m && McdModelGetBody(m))
	{
		if(!MdtBodyIsEnabled(McdModelGetBody(m)))
			MdtBodyEnable(McdModelGetBody(m));
		return;
	}

	// If this is a set-up rag-doll, enable the first model.
	if(this->Physics == PHYS_KarmaRagDoll && this->MeshInstance)
	{
		USkeletalMeshInstance *skelInst = Cast<USkeletalMeshInstance>(this->MeshInstance);
		if(skelInst && skelInst->KSkelIsInitialised)
		{
			m = skelInst->KSkelModels(0);
			if(m && McdModelGetBody(m))
			{
				MdtBodyEnable(McdModelGetBody(m));
				return;
			}
		}
	}

	unguard;
}

void AActor::KGetRigidBodyState(FKRigidBodyState* RBstate)
{
	guard(AActor::KGetRigidBodyState);

	// If no model or body, just return zero.
	McdModelID model = this->getKModel();
	if(!model)
	{
		appMemset( RBstate, 0, sizeof(FKRigidBodyState) );
		RBstate->Quaternion.W = 1.0f;
		return;
	}

	MdtBodyID body = McdModelGetBody(model);
	if(!body)
	{
		appMemset( RBstate, 0, sizeof(FKRigidBodyState) );
		RBstate->Quaternion.W = 1.0f;
		return;
	}

	MeVector3 tmp;
	MdtBodyGetPosition(body, tmp);
	RBstate->Position.X = tmp[0] * K_ME2UScale;
	RBstate->Position.Y = tmp[1] * K_ME2UScale;
	RBstate->Position.Z = tmp[2] * K_ME2UScale;

	MeVector4 q;
	MdtBodyGetQuaternion(body, q);
	RBstate->Quaternion.W = q[0];
	RBstate->Quaternion.X = q[1];
	RBstate->Quaternion.Y = q[2];
	RBstate->Quaternion.Z = q[3];

	MdtBodyGetLinearVelocity(body, tmp);
	RBstate->LinVel.X = tmp[0] * K_ME2UScale;
	RBstate->LinVel.Y = tmp[1] * K_ME2UScale;
	RBstate->LinVel.Z = tmp[2] * K_ME2UScale;

	MdtBodyGetAngularVelocity(body, tmp);
	RBstate->AngVel.X = tmp[0];
	RBstate->AngVel.Y = tmp[1];
	RBstate->AngVel.Z = tmp[2];

	unguard;
}

UBOOL AActor::KIsAwake()
{
	guard(AActor::KIsAwake);

    McdModelID model = this->getKModel();
	if(!model)
		return false;

	MdtBodyID body = McdModelGetBody(model);
	if(!body)
		return false;

	if(MdtBodyIsEnabled(body))
		return true;
	else
		return false;

	unguard;
}

void AActor::KDrawRigidBodyState(FKRigidBodyState* RBstate, UBOOL AltColour)
{
	guard(AActor::KDrawRigidBodyState);

	FVector pos(RBstate->Position.X, RBstate->Position.Y, RBstate->Position.Z);

	MeVector4 quat = {RBstate->Quaternion.W, RBstate->Quaternion.X, RBstate->Quaternion.Y, RBstate->Quaternion.Z};
	MeMatrix4 tm;
	MeQuaternionToTM(tm, quat);

	FVector x, y, z;
	KME2UPosition(&x, tm[0]);
	KME2UPosition(&y, tm[1]);
	KME2UPosition(&z, tm[2]);

	const float AxisLength = 1.8f;

	if(!AltColour)
	{
		GTempLineBatcher->AddLine(pos, pos + AxisLength * x, FColor(255, 0, 0));
		GTempLineBatcher->AddLine(pos, pos + AxisLength * y, FColor(0, 255, 0));
		GTempLineBatcher->AddLine(pos, pos + AxisLength * z, FColor(0, 0, 255));
	}
	else
	{
		GTempLineBatcher->AddLine(pos, pos + 0.8f * AxisLength * x, FColor(255, 128, 128));
		GTempLineBatcher->AddLine(pos, pos + 0.8f * AxisLength * y, FColor(128, 255, 128));
		GTempLineBatcher->AddLine(pos, pos + 0.8f * AxisLength * z, FColor(128, 128, 255));
	}

	unguard;
}


void AActor::KAddForces(FVector force, FVector torque)
{
	guard(AActor::KAddForces);

	McdModelID model = getKModel();
	if(!model)
		return;

	MdtBodyID body = McdModelGetBody(model);
	if(!body)
		return;

	MdtBodyAddForce(body, force.X, force.Y, force.Z);
	MdtBodyAddTorque(body, torque.X, torque.Y, torque.Z);

	unguard;
}
	

// Update any MdtBody's mass properties using the UnrealScript version.
// Need to call this after changing mass, damping, inertia tensor, com-position, KScale(3D), stay-upright etc.
void UKarmaParams::PostEditChange()
{
	guard(UKarmaParams::PostEditChange);

	Super::PostEditChange();

	McdModelID model = (McdModelID)this->KarmaData;
	if(!model)
		return;

	MdtBodyID body = McdModelGetBody(model);
	if(!body)
		return;

	//AActor* actor = KBodyGetActor(body);


	// If we have a 'full' Karma Params including inertia tensor/com-offset, use that.
	UKarmaParamsRBFull* fullParams = Cast<UKarmaParamsRBFull>(this);
	if(fullParams)
	{
		// Mass props
		MeMatrix3 I;
		FVector totalScale = KScale3D * KScale;
		I[0][0] =			fullParams->KInertiaTensor[0] * KMass * totalScale.Y * totalScale.Z;
		I[0][1] = I[1][0] = fullParams->KInertiaTensor[1] * KMass * totalScale.X * totalScale.Y;
		I[0][2] = I[2][0] = fullParams->KInertiaTensor[2] * KMass * totalScale.Z * totalScale.X;
		I[1][1] =			fullParams->KInertiaTensor[3] * KMass * totalScale.X * totalScale.Z;
		I[1][2] = I[2][1] = fullParams->KInertiaTensor[4] * KMass * totalScale.Y * totalScale.Z;
		I[2][2] =			fullParams->KInertiaTensor[5] * KMass * totalScale.X * totalScale.Y;

		// Centre-of-mass position.
		// Can only do this if there are no constraints.
		MeVector3 o;
		FVector newCOM = fullParams->KCOMOffset * totalScale;
		KU2MEVecCopy(o, newCOM);
		MeDictNode *node = MeDictFirst(&body->constraintDict);
		if(!node)
			MdtBodySetCenterOfMassRelativePosition(body, o);

		// Inertia tensor
		KBodySetInertiaTensor(body, I);
	}
	else
	{
		//	JTODO: Mass properties came from static mesh, so we need to get hold of them again 
		//	to rescale them in case scale or mass has changed. But how!?
	}
	
	KBodySetMass(body, this->KMass);
	
	// Spherical?
	if(this->bKNonSphericalInertia)
		MdtBodyEnableNonSphericalInertia(body);
	else
		MdtBodyDisableNonSphericalInertia(body);

    // Damping
    MdtBodySetAngularVelocityDamping(body, this->KAngularDamping);
    MdtBodySetLinearVelocityDamping(body, this->KLinearDamping);

	// Stay upright stuff

	// We use an Angular3 constraint to keep this thing upright.
	if(this->bKStayUpright)
	{
		MdtAngular3ID ang3 = (MdtAngular3ID)this->KAng3;

		if(!ang3) // need to create it
		{
			ang3 = MdtAngular3Create(MdtBodyGetWorld(body));
			MdtAngular3SetBodies(ang3, body, 0);
			
			MdtConstraintID mdtCon = MdtAngular3QuaConstraint(ang3);

			MdtConstraintBodySetAxesRel(mdtCon, 0, 
				0, 0, 1, 
				1, 0, 0);
			
			MdtConstraintBodySetAxesRel(mdtCon, 1, 
				0, 0, 1, 
				1, 0, 0);

			MdtAngular3Enable(ang3);

			this->KAng3 = (INT)ang3;
		}

		if(MdtAngular3RotationIsEnabled(ang3) && !this->bKAllowRotate)
			MdtAngular3EnableRotation(ang3, 0);
		else if(!MdtAngular3RotationIsEnabled(ang3) && this->bKAllowRotate)
			MdtAngular3EnableRotation(ang3, 1);

		MdtAngular3SetStiffness(ang3, this->StayUprightStiffness);
		MdtAngular3SetDamping(ang3, this->StayUprightDamping);
	}
	else
	{
		// need to destroy it
		if(this->KAng3)
		{		
			MdtAngular3ID ang3 = (MdtAngular3ID)this->KAng3;

			MdtAngular3Disable(ang3);
			MdtAngular3Destroy(ang3);
			this->KAng3 = 0;
		}
	}

    unguard;
}

/////////////////////////// PRE/POST STEP ///////////////////////////////////

// This is called just before this actor is simulated. 
// You CAN change damping and add forces.
// You CANNOT create or destroy anything.
void AActor::preKarmaStep(FLOAT DeltaTime)
{
	guard(AActor::preKarmaStep);
	check(Physics == PHYS_Karma || Physics == PHYS_KarmaRagDoll);

	if(Physics == PHYS_KarmaRagDoll)
	{
		this->preKarmaStep_skeletal(DeltaTime);
		return;
	}

    McdModelID model = this->getKModel();
    if(!model)
        return;
    
    MdtBodyID body = McdModelGetBody(model);
    if(!body)
        return;

	MeReal meMass = MdtBodyGetMass(body);
	FVector calcForce(0, 0, 0);

    MeVector3 gforce = {0, 0, 0};
	if(PhysicsVolume)
		KU2MEPosition(gforce, PhysicsVolume->Gravity); // scales gravity (m/s^2) for ME sizes

	// Get the parameters for this actors simulation.
	UKarmaParams* kparams = 0;
	if(this->KParams)
		kparams = Cast<UKarmaParams>(this->KParams);

	if(!kparams)
	{
        debugf(TEXT("(Karma:) preKarmaStep: Actor has no KParams."));
        return;
	}

	MeReal gravScale = ME_GRAVSCALE * kparams->KActorGravScale;
	if(Level)
		gravScale *= Level->KarmaGravScale;

	// Buoyancy calculations - basically reduces affect of gravity and increases damping.
	MeReal buoyEffect = 0.f;
	MeReal linDamp = kparams->KLinearDamping;
	MeReal angDamp = kparams->KAngularDamping;

	if(PhysicsVolume->bWaterVolume)
	{
		buoyEffect = PhysicsVolume->KBuoyancy * kparams->KBuoyancy;
		linDamp += PhysicsVolume->KExtraLinearDamping;
		angDamp += PhysicsVolume->KExtraAngularDamping;
	}

	calcForce.X = gravScale * meMass * gforce[0];
	calcForce.Y = gravScale * meMass * gforce[1];
	calcForce.Z = gravScale * meMass * (gforce[2] * (1-buoyEffect));

	// Set damping
	MdtBodySetLinearVelocityDamping(body, linDamp);
	MdtBodySetAngularVelocityDamping(body, angDamp);

	// Any user forces
	FVector userForce(0, 0, 0), userTorque(0, 0, 0);
	eventKApplyForce(userForce, userTorque);

	// Apply force and torque
    MdtBodyAddForce(body, 
		calcForce.X + userForce.X, 
		calcForce.Y + userForce.Y, 
		calcForce.Z + userForce.Z);

	MdtBodyAddTorque(body,
		userTorque.X,
		userTorque.Y,
		userTorque.Z);

	unguard;
}

// Called just after the actor has finished being simulated.
void AActor::postKarmaStep()
{
	guard(AActor::postKarmaStep);

	if(bDeleteMe)
		return;

	if(Physics != PHYS_Karma && Physics != PHYS_KarmaRagDoll)
	{
		debugf(TEXT("(Karma:) postKarmaStep: Actors with non-Karma physics."));
		return;
	}

	if(Physics == PHYS_KarmaRagDoll)
	{
		this->postKarmaStep_skeletal();
		return;
	}

    McdModelID model = this->getKModel();
    if(!model)
        return;
    
    MdtBodyID body = McdModelGetBody(model);
    if(!body)
        return;

	// Update Unreal position/rotation from Karma body.
	MeMatrix4 tm;        
	MdtBodyGetTransform(body, tm);

	FRotator rot;
	FVector newPos, moveBy;
	FCheckResult Hit(1.0f);
	KME2UTransform(&newPos, &rot, tm);
	moveBy = newPos - this->Location; 

	// Just to be sure..
	this->bCollideWorld = 0;

	// Keep bounding box up to date!
	if(McdGeometryGetTypeId(McdModelGetGeometry(model)) != kMcdGeometryTypeNull)
		McdModelUpdate(model);

	// Actually move the actor. 
	// This could actually destroy the actor due to Touch etc. - so we check afterwards.
	GetLevel()->MoveActor(this, moveBy, rot, Hit);
	if(this->bDeleteMe || this->getKModel() != model || McdModelGetBody(model) != body)
		return;

	// Velocity capping. KTODO: How should this be exposed? Move AirSpeed into Actor?
	MeReal meMaxSpeed = K_U2MEScale * ME_MAX_KARMA_SPEED;
	MeVector3 v;
	MdtBodyGetLinearVelocity(body, v);
	if(MeVector3MagnitudeSqr(v) > meMaxSpeed * meMaxSpeed)
	{
	    MeVector3Normalize(v);
	    MeVector3Scale(v, meMaxSpeed);
	    MdtBodySetLinearVelocity(body, v[0], v[1], v[2]);
	}

	// Set the Velocity field of the Actor, in case its used by anything.
	KME2UPosition(&this->Velocity, v);

	unguard;
}


void AActor::physKarma_internal(FLOAT deltaTime)
{
	check(Physics == PHYS_Karma);

    FRotator rot;
    FVector newPos, moveBy;
    FCheckResult Hit(1.0f);

    MdtWorldID world = this->GetLevel()->KWorld;
    if(!world)
    {
        debugf(TEXT("(Karma:) AActor::physKarma: No Karma MdtWorld found."));
        return;
    }

    McdModelID model = this->getKModel();
    if(!model)
        return;
    
    MdtBodyID body = McdModelGetBody(model);
    if(!body)
        return;

	// Handle any updates to the rigid body state from script.
	// Note: Because actors are always ticked before constraints, we can be sure the constraint will
	// get the most up-to-date state.
	FKRigidBodyState newState;
	UBOOL doUpdate = eventKUpdateState(newState);
	if(doUpdate)
	{
#if 1
		// Check the quaternion...
		MeVector4 quat = {newState.Quaternion.W, newState.Quaternion.X, newState.Quaternion.Y, newState.Quaternion.Z};
		MeReal k = MeVector4MagnitudeSqr(quat);
		if(ME_IS_ZERO_TOL(k, ME_MEDIUM_EPSILON))
		{
			debugf( TEXT("Invalid zero quaternion set for body. (%s)"), this->GetName() );
			return;
		}
		/* Else if quaternion is not unit length. */
		else if(!ME_IS_ZERO_TOL((k - 1.f), ME_MEDIUM_EPSILON))
		{
			debugf( TEXT("Quaternion (%f %f %f %f) with non-unit magnitude detected. (%s)"), quat[0], quat[1], quat[2], quat[3], this->GetName() );
			return;
		}
#endif

		if(!MdtBodyIsEnabled(body))
			MdtBodyEnable(body); // If getting new state from the server - make sure we are awake...

		MeVector3 oldPos;
		MdtBodyGetPosition(body, oldPos);

		MeVector3 newPos;
		newPos[0] = K_U2MEScale * newState.Position.X;
		newPos[1] = K_U2MEScale * newState.Position.Y;
		newPos[2] = K_U2MEScale * newState.Position.Z;

		// Find out how much of a correction we are making
		MeVector3 delta;
		MeVector3Subtract(delta, newPos, oldPos);
		MeReal deltaMagSqr = MeVector3MagnitudeSqr(delta);
		//debugf(TEXT("DMS: %f"), deltaMagSqr );

		// If its a small correction, only make a partial correction, and
		// calculate a velocity that would fix it over 'fixTime'.
		MeVector3 setPos, fixVel;
		if(deltaMagSqr < 0.35f)
		//if(deltaMagSqr < 1000000.0f)
		{
			MeReal adjustFactor = 0.2f;
			setPos[0] = adjustFactor * newPos[0] + (1.0f-adjustFactor) * oldPos[0];
			setPos[1] = adjustFactor * newPos[1] + (1.0f-adjustFactor) * oldPos[1];
			setPos[2] = adjustFactor * newPos[2] + (1.0f-adjustFactor) * oldPos[2];

			MeReal fixTime = 1.0f;
			MeVector3Subtract(fixVel, newPos, setPos);
			MeVector3Scale(fixVel, 1.0f/fixTime);
		}
		else
		{
			MeVector3Copy(setPos, newPos);
			MeVectorSetZero(fixVel, 3);
		}

		MdtBodySetPosition( body, setPos[0], setPos[1], setPos[2] );

		MdtBodySetQuaternion(body, 
			newState.Quaternion.W, 
			newState.Quaternion.X, 
			newState.Quaternion.Y, 
			newState.Quaternion.Z);

		MdtBodySetLinearVelocity(body, 
			K_U2MEScale * newState.LinVel.X + fixVel[0], 
			K_U2MEScale * newState.LinVel.Y + fixVel[1], 
			K_U2MEScale * newState.LinVel.Z + fixVel[2]);

		MdtBodySetAngularVelocity(body, 
			newState.AngVel.X, 
			newState.AngVel.Y, 
			newState.AngVel.Z);
	}

	// If this body is not enabled - we dont need to do anything more.
	// That includes updating its graphics position.
	if(!MdtBodyIsEnabled(body) || !KGData->bDoTick)
	{
		this->Velocity = FVector(0, 0, 0);
		return;
	}
}

// THe Karma collision detection/contact generation/simulation for this frame should have been called already by 
// this point. This function moves the graphics to where the physics are, and applys user forces etc.
// It also updates the position of constraints, and updates any controlling code.
void AActor::physKarma(FLOAT deltaTime)
{
    guard(AActor::physKarma);
	clock(GStats.DWORDStats(GEngineStats.STATS_Karma_physKarma));
    physKarma_internal(deltaTime);
	unclock(GStats.DWORDStats(GEngineStats.STATS_Karma_physKarma));
    unguard;
}


//////////// AKVEHICLE C++ ////////////////

void AKVehicle::PostNetReceive()
{
	guard(AKVehicle::PostNetReceive);

	Super::PostNetReceive();
	eventVehicleStateReceived();

	unguard;
}

void AKVehicle::PostEditChange()
{
	guard(AKVehicle::PostEditChange);
	Super::PostEditChange();

	// Tell script that a parameters has changed, in case it needs to KUpdateConstraintParams on any constraints.
	this->eventKVehicleUpdateParams();
    unguard;
}

void AKVehicle::setPhysics(BYTE NewPhysics, AActor *NewFloor, FVector NewFloorV)
{
	guard(AKVehicle::setPhysics);

	check(Physics == PHYS_Karma);

	if(NewPhysics != PHYS_Karma)
	{
		debugf(TEXT("%s->setPhysics(%d). KVehicle's can only have Physics == PHYS_Karma."), this->GetName(), NewPhysics);
		return;
	}

	unguard;
}

void AKVehicle::TickAuthoritative( FLOAT DeltaSeconds )
{
	guard(AKVehicle::TickAuthoritative);

	check(Physics == PHYS_Karma); // karma vehicles should always be in PHYS_Karma

	eventTick(DeltaSeconds);
	ProcessState( DeltaSeconds );
	UpdateTimers(DeltaSeconds );

	// Update LifeSpan.
	if( LifeSpan!=0.f )
	{
		LifeSpan -= DeltaSeconds;
		if( LifeSpan <= 0.0001f )
		{
			GetLevel()->DestroyActor( this );
			return;
		}
	}

	// Perform physics.
	if ( !bDeleteMe )
		performPhysics( DeltaSeconds );

	unguard;
}

void AKVehicle::TickSimulated( FLOAT DeltaSeconds )
{
	guard(AKVehicle::TickSimulated);
	TickAuthoritative(DeltaSeconds);
	unguard;
}

#endif // WITH_KARMA

// Handy function for plotting script variables onto screen graph.
void AKVehicle::execGraphData( FFrame& Stack, RESULT_DECL )
{
	guard(AKVehicle::execGraphData);

	P_GET_STR(DataName);
	P_GET_FLOAT(DataValue);
    P_FINISH;

	// Make graph line name by concatenating vehicle name with data name.
	FString lineName = FString::Printf(TEXT("%s_%s"), 
		this->GetName(), (TCHAR*)DataName.GetCharArray().GetData());

	GStatGraph->AddDataPoint(lineName, DataValue, 1);

	unguard;
}
