/*============================================================================
	Karma Integration Support
    
    - PerContact/PerPair Callbacks
    - Game/Level/Actor/Constraint Init/Term
============================================================================*/

#include "EnginePrivate.h"

#ifdef WITH_KARMA

/* Global data */
ENGINE_API KarmaGlobals* KGData = 0;

const int K_collisionGeometryTypesMaxCount = 3;
const int K_collisionGroupsMaxCount = 1;
const int K_materialsMaxCount = 2;
const int K_bridgeModelPairBufferSize = 1024;

#define ME_UNIT_LENGTH	((MeReal)0.95) // 0.95
#define ME_UNIT_MASS	((MeReal)0.95) // 0.95
#define ME_GAMMA_PERSEC ((MeReal)6) // 6
#define ME_EPSILON		((MeReal)0.001) // 0.001
#define ME_PENOFFSET	((MeReal)0.015) // 0.015
#define ME_PENSCALE		((MeReal)1.0) // 1
#define ME_CONTACTSOFT	((MeReal)0.01) // 0.005

// Try and make the effect of both slip and box friction the same.
#define ME_FRICTIONSCALE ((MeReal)0.5)
#define ME_MAX_TIMESTEP  ((MeReal)0.04)

// If the timestep goes above this, start double-ticking bDoubleTickRate actors
#define ME_DO_DOUBLETICK_TIMESTEP	((MeReal)0.025)
//#define ME_DO_DOUBLETICK_TIMESTEP	((MeReal)0.0)

#define ME_MAX_PENETRATION ((MeReal)0.1)


/********************* BODY ENABLE/DISABLE CALLBACKS **********************/

// Currently these only deal with single-model karma actors (ie not ragdolls).


void MEAPI RemoveActiveActorFromBody(const MdtBodyID b)
{
	guard(RemoveActiveActorFromBody);

	AActor* actor = KBodyGetActor(b);

	if(!actor)
		return;

	if(actor->getKModel() && !actor->bDeleteMe)
		KActorContactGen(actor, 0);

	unguard;
}

void MEAPI AddActiveActorFromBody(const MdtBodyID b)
{
	guard(AddActiveActorFromBody);

	AActor* actor = KBodyGetActor(b);

	if(!actor)
		return;

	if(actor->getKModel() && !actor->bDeleteMe && actor->bBlockKarma && actor->bCollideActors)
		KActorContactGen(actor, 1);

	unguard;
}

void MEAPI DestroyContactGroupReferences(const MdtContactGroupID c)
{
	guard(DestroyContactGroupReferences);

    McdModelPairID m = (McdModelPairID)c->generator;
    if(m)
        m->responseData = 0;

	unguard;
}

// Straight add or remove from the 
void KActorContactGen(AActor* actor, UBOOL gen)
{
	guard(KActorContactGen);

	ULevel* level = actor->GetLevel();

	if(gen)
	{
		// Should set an actor to contact gen unless this is true!
		check( !actor->bDeleteMe );
		check( actor->Physics == PHYS_Karma || actor->Physics == PHYS_KarmaRagDoll );
		check( actor->bBlockKarma );
		check( actor->bCollideActors); 

		// Add if not already there.
		if(level->KContactGenActors.FindItemIndex(actor) == INDEX_NONE)
			level->KContactGenActors.AddItem(actor);
	}
	else
	{
		level->KContactGenActors.RemoveItem(actor);
	}

	unguard;
}

/********************* COPIED FROM TOOLKIT ***********************/

#define TM_STACK_SIZE 1000
#define MAX_ENTRY_COUNT 1000
#define CONTACT_BUFFER_SIZE 1000
#define MAX_PAIR_COUNT 1000
#define AGGREGATE_ARRAY_COUNT 16
#define AGGREGATE_ARRAY_SIZE 800 // 128
#define ATOMIC_ARRAY_SIZE 1024

static McdBatchContext* KBatchContextCreate(McdFramework *frame)
{
	guard(KBatchContextCreate);

    int i;

    McdBatchContext *context;
    
    context = (McdBatchContext *)MeMemoryAPI.create(sizeof(McdBatchContext));
    context->frame = frame;
    context->typeCount = context->frame->geometryRegisteredCountMax;

    context->tmArray = (MeMatrix4 *)
        MeMemoryAPI.createAligned(sizeof(MeMatrix4)*TM_STACK_SIZE,MeALIGNTO);
    context->tmTrack = (MeMatrix4Ptr **)
        MeMemoryAPI.create(sizeof(MeMatrix4Ptr *)*TM_STACK_SIZE);
    context->tmMaxCount = TM_STACK_SIZE;

    context->table = (McdBatchEntry **)MeMemoryAPI.create(sizeof(McdBatchEntry *) * 
        context->typeCount * context->typeCount); 

    context->entryArray = (McdBatchEntry *)MeMemoryAPI.create(sizeof(McdBatchEntry) * MAX_ENTRY_COUNT);
    context->maxEntryCount = MAX_ENTRY_COUNT;

    context->pairData = (McdBatchPairData *)MeMemoryAPI.create(sizeof(McdBatchPairData) * MAX_PAIR_COUNT);
    context->pairDataMaxCount = MAX_PAIR_COUNT;

    context->pools = (McdBatchContactPool *)MeMemoryAPI.create((AGGREGATE_ARRAY_COUNT+1)*sizeof(McdBatchContactPool));
    context->poolMaxCount = AGGREGATE_ARRAY_COUNT+1;
    context->pools[0].contacts = (McdContact *)MeMemoryAPI.create(ATOMIC_ARRAY_SIZE * sizeof(McdContact));
    context->pools[0].contactMaxCount = ATOMIC_ARRAY_SIZE;

    for(i=1;i<=AGGREGATE_ARRAY_COUNT;i++)
    {
        context->pools[i].contacts = (McdContact *)MeMemoryAPI.create(AGGREGATE_ARRAY_SIZE * sizeof(McdContact));
        context->pools[i].contactMaxCount = AGGREGATE_ARRAY_SIZE;
    }
    return context;

	unguard;
}

static MstBridgeID KBridgeCreate(McdFrameworkID frame, const unsigned int maxMaterials)
{
	guard(KBridgeCreate);

    unsigned int nElements, i;
    MstMaterialPair* p;
    MstBridgeID b = (MstBridgeID)MeMemoryAPI.create(sizeof(MstBridge));

    if(!b)
    {
        MeWarning(0, "MstBridgeCreate: Could not allocate memory for bridge.");
        return b;
    }

    /* Initialise material pool */
    b->maxMaterials = maxMaterials;
    nElements = MeUpperDiagonalSize(b->maxMaterials);
    b->materialPairArray = (MstMaterialPair *)
        MeMemoryAPI.create(nElements * sizeof (MstMaterialPair));

    /* Set all material pairs to the default contact params. */
    p = b->materialPairArray;
    for(i=0; i<nElements; i++)
    {
        MdtContactParamsReset(&(p->cp));

        /* Set contact callback to null. */
        p->contactCB = NULL;
        p->intersectCB = NULL;
        p->pairCB = NULL;
        p++;
    }

    /* Material '0' is used as default */
    b->usedMaterials = 1;

    /* Allocate ModelPair buffer. */
    b->pairs = McdModelPairContainerCreate(50);
    b->context = KBatchContextCreate(frame);

    /* Allocate McdContact buffer. */
    b->contactsMaxCount = 50;
    b->contacts = (McdContact*)
        MeMemoryAPI.create(b->contactsMaxCount * sizeof(McdContact));

    /* Set callbacks for body enable/disable. */
    return b;

	unguard;
}

/********************* CONTACTS ***********************/
// Do we always assume world geometry (blocking volumes etc.) have friction of 1.
#define WORLD_FRICTION_ONE (1)

/* Called for all contacts. */
static MeBool MEAPI KPerContactCB(McdIntersectResult* result, McdContact* colC, MdtContactID dynC)
{
    guard(KPerContactCB);
    
    McdModelID m1, m2;
    AActor *a1, *a2;
    McdModelPairGetModels(result->pair, &m1, &m2);
	a1 = KModelGetActor(m1);
	a2 = KModelGetActor(m2);

    /* Reject rogue contacts. */
    MeReal d2, penetration = MdtContactGetPenetration(dynC);
    
	/* Subtract an amount from penetration, so we get some resting penetration. */
	penetration -= ME_PENOFFSET;
	if(penetration < 0)
        penetration = 0;

	MdtContactSetPenetration(dynC, penetration * ME_PENSCALE);

    MeVector3 normal, position;
    MdtContactGetPosition(dynC, position);
	MdtContactGetNormal(dynC, normal);
	d2 = MeVector3Magnitude(normal);
	if (d2 < 0.98f || d2 > 1.02f)
	{
		debugf(TEXT("(Karma:) Bad Normal Length: %f"), d2);
		return 0;
	}

    MeI16 g1type = McdGeometryGetTypeId(McdModelGetGeometry(m1));
    MeI16 g2type = McdGeometryGetTypeId(McdModelGetGeometry(m2));

	// NB: The ONLY model without an Actor must be the 'world', which must be the tri-list.
	if((a1 && !a1->KParams) || (a2 && !a2->KParams))
	{
		debugf(TEXT("(Karma:) Contact with Actor with no KParams."));
		return 1;
	}

    MdtBodyID b1 = MdtContactGetBody(dynC, 0);
    MdtBodyID b2 = MdtContactGetBody(dynC, 1);

    //  Calculate friction/restitution from actors/physics zone. 
	//	For the world (terrain/bsp) - we use per triangle data.
    MeReal totalFriction;
	MeReal totalRes;
	MeVector3 worldVel = {0, 0, 0};
	KarmaTriUserData* triData = 0;
	ULevel* level = NULL;
	short triFlags = 0;

	if(!a1)
	{
		if(!a2)
		{
			debugf(TEXT("(Karma): Contact with 2 Triangle Lists!"));
			return 1;
		}
		else
		{
			level = a2->GetLevel();

			triData = (KarmaTriUserData*)(colC->element1.ptr);
			triFlags = colC->dims & 0x00FF;
			check(triData);

			totalFriction = a2->KParams->KFriction * triData->localFriction;
			totalRes = a2->KParams->KRestitution * triData->localRestitution;

			a2->KParams->bContactingLevel = 1;
		}
	}
	else // Non-trilist
	{
		level = a1->GetLevel();

		if(!a2)
		{
			triData = (KarmaTriUserData*)(colC->element2.ptr);
			triFlags = colC->dims>>8;
			check(triData);

			totalFriction = a1->KParams->KFriction * triData->localFriction;
			totalRes = a1->KParams->KRestitution * triData->localRestitution;

			a1->KParams->bContactingLevel = 1;
		}
		else // Non-trilist
		{
			// If its a contact with itself (eg. ragdoll) then use zero friction.
			if(a1 == a2 || (a1->ConglomerateID && a2->ConglomerateID))
			{
				totalFriction = 0;
				if (a1->ConglomerateID && a2->ConglomerateID)
					totalRes = 5;
				else
					totalRes = 0;
			}
			else
			{
				// HACK! If this is a contact with a world-geometry object,
				// we default the friction to 1.0
				if(!McdModelGetBody(m1))
				{
#if WORLD_FRICTION_ONE
					totalFriction = a2->KParams->KFriction;
					totalRes = a2->KParams->KRestitution;
#else
					totalFriction = a1->KParams->KFriction * a2->KParams->KFriction;
					totalRes = a1->KParams->KRestitution * a2->KParams->KRestitution;
#endif
					// Check for moving world geometry (ie. bStatic==False & no Body)
					// We calculate the world velocity, and enable body.
					if(a1 && !a1->bStatic)
					{
						KU2MEPosition(worldVel, a1->Velocity);
						MdtBodyEnable(McdModelGetBody(m2));
					}

					a2->KParams->bContactingLevel = 1;
				}
				else if(!McdModelGetBody(m2))
				{
#if WORLD_FRICTION_ONE
					totalFriction = a1->KParams->KFriction;
					totalRes = a1->KParams->KRestitution;
#else
					totalFriction = a1->KParams->KFriction * a2->KParams->KFriction;
					totalRes = a1->KParams->KRestitution * a2->KParams->KRestitution;
#endif
					if(a2 && !a2->bStatic)
					{
						KU2MEPosition(worldVel, a2->Velocity);
						MdtBodyEnable(McdModelGetBody(m1));
					}

					a1->KParams->bContactingLevel = 1;
				}
				else // Karma-Karma collision.
				{
					totalFriction = a1->KParams->KFriction * a2->KParams->KFriction;
					totalRes = a1->KParams->KRestitution * a2->KParams->KRestitution;
				}
			}
		}
	}

	check(level != NULL);

    MdtContactParamsID params = MdtContactGetParams(dynC);

	if(totalFriction < 0.01f)
	{
		MdtContactParamsSetType(params, MdtContactTypeFrictionZero);
		totalFriction = 0;
	}
	else
	{
		if(level->GetLevelInfo()->bKStaticFriction) // Box Friction
		{
			MdtContactParamsSetType(params, MdtContactTypeFriction2D);
			MdtContactParamsSetFrictionModel(params, MdtFrictionModelNormalForce);
			MdtContactParamsSetFrictionCoeffecient(params, ME_FRICTIONSCALE * totalFriction);
			MdtContactParamsSetSlip( params, 0 );
		}
		else // Slip Friction
		{
			MdtContactParamsSetType(params, MdtContactTypeFriction2D);
			MdtContactParamsSetFrictionModel(params, MdtFrictionModelBox);
			MdtContactParamsSetFrictionCoeffecient(params, MEINFINITY);
			MdtContactParamsSetFriction(params, MEINFINITY);
			MdtContactParamsSetSlip( params, MeMAX(ME_EPSILON, (1/totalFriction)-1) );
		}
	}
	MdtContactParamsSetRestitution(params, totalRes);
	MdtContactParamsSetSoftness(params, ME_CONTACTSOFT);

	MdtContactSetWorldVelocity(dynC, worldVel[0], worldVel[1], worldVel[2]);

#if 0
    // If its tri-list contact, do some extra filtering.
    if(g1type == kMcdGeometryTypeTriangleList || g2type == kMcdGeometryTypeTriangleList)
    {
#else
	// If dynamics-world contact, do extra filtering
	if( (b1 && !b2) || (!b1 && b2) )
	{
#endif
        // Don't do hack to sphere or sphyl contacts
        if(g1type != kMcdGeometryTypeSphere && g2type != kMcdGeometryTypeSphere //)
			&& g1type != kMcdGeometryTypeSphyl && g2type != kMcdGeometryTypeSphyl)
        {
            MdtBodyID body = (b1)?b1:b2;
            if(!body)
                appErrorf(TEXT("(Karma): Contact with no bodies!"));
            
            /* HACK! Cap contact penetration to the distance travelled last timestep. */
            MeVector3 bvel;
            MdtBodyGetLinearVelocity(body, bvel);
            MeVector3Scale(bvel, KGData->TimeStep);
            MeReal bvelMag = MeVector3Magnitude(bvel);
            
            if(penetration > bvelMag + ME_MAX_PENETRATION)
            {
				//debugf(TEXT("Shrink Pen F: %f T: %f"), penetration, bvelMag + (MeReal)0.1);
                MdtContactSetPenetration(dynC, bvelMag + ME_MAX_PENETRATION);
            }
        }
    }
	// If physics-physics - still cap maximum penetration
	else if(b1 && b2)
	{
		if(penetration > ME_MAX_PENETRATION)
		{
			//debugf(TEXT("Shrink Pen F: %f T: %f"), penetration, bvelMag + (MeReal)0.1);
			MdtContactSetPenetration(dynC, ME_MAX_PENETRATION);
		}
	}

    /* Find mag of relative normal and tangential velocity at contact. */
    MeVector3 relVel, bVel1, bVel2 = {0, 0, 0};
    MdtBodyGetVelocityAtPoint(b1, position, bVel1);
    if(b2)
        MdtBodyGetVelocityAtPoint(b2, position, bVel2);
    MeVector3Subtract(relVel, bVel2, bVel1);

    /*  If either of these actors are listening for the 'impact' event, 
        check rel vel magnitude and call event if threshold exceeded. */
	if ( (a1 && a1->IsProbing(ENGINE_KImpact)) || (a2 && a2->IsProbing(ENGINE_KImpact)) )
    {
        MeReal relVelMag = MeVector3Magnitude(relVel);
        FVector upos, uvel, unorm = FVector(normal[0], normal[1], normal[2]);

        KME2UPosition(&upos, position);
        KME2UPosition(&uvel, relVel);

		//If either actor is a vehicle, only send the event if the impact is head-on (ignore undercarriage scaping world, etc)
		if ((a1 && a1->IsA(AKVehicle::StaticClass())) || (a2 && a2->IsA(AKVehicle::StaticClass())))
		{
			if (a1 && a2 && a1->ConglomerateID && a2->ConglomerateID && a1->ConglomerateID != a2->ConglomerateID)
			{
					a1->ConglomerateID->eventKImpact(a2->ConglomerateID, upos, uvel, unorm);
					a2->ConglomerateID->eventKImpact(a1->ConglomerateID, upos, uvel, unorm);
			} /*else if (a1 && a1->IsA(AKVehicle::StaticClass()) && !Cast<AKVehicle>(a1)->frontLeft->bTireOnGround && !Cast<AKVehicle>(a1)->frontRight->bTireOnGround && !Cast<AKVehicle>(a1)->rearLeft->bTireOnGround && !Cast<AKVehicle>(a1)->rearRight->bTireOnGround && (relVelMag * K_ME2UScale > a1->KParams->KImpactThreshold) && a1->IsProbing(ENGINE_KImpact))
			{
				a1->ConglomerateID->eventKImpact(a2->ConglomerateID, upos, uvel, unorm);
			} else if (a2 && a2->IsA(AKVehicle::StaticClass()) && !Cast<AKVehicle>(a2)->frontLeft->bTireOnGround && !Cast<AKVehicle>(a2)->frontRight->bTireOnGround && !Cast<AKVehicle>(a2)->rearLeft->bTireOnGround && !Cast<AKVehicle>(a2)->rearRight->bTireOnGround && (relVelMag * K_ME2UScale > a2->KParams->KImpactThreshold) && a2->IsProbing(ENGINE_KImpact))
			{
				a2->ConglomerateID->eventKImpact(a2->ConglomerateID, upos, uvel, unorm);
			}*/ else
			{
				if(a1 && a1->ConglomerateID)
				{
					FVector normalizeuvel = a1->Rotation.Vector();//uvel; 
					normalizeuvel.Normalize();
					float DirectionalImpact = normalizeuvel|unorm;

					if (DirectionalImpact > 0.75 && (relVelMag * K_ME2UScale > a1->KParams->KImpactThreshold) && a1->IsProbing(ENGINE_KImpact))
						a1->eventKImpact(a2, upos, uvel, unorm);
				}
				if(a2 && a2->ConglomerateID)
				{
					FVector normalizeuvel = a1->Rotation.Vector();//uvel; 
					normalizeuvel.Normalize();
					float DirectionalImpact = normalizeuvel|unorm;

					if (DirectionalImpact > 0.75 && (relVelMag * K_ME2UScale > a2->KParams->KImpactThreshold) && a2->IsProbing(ENGINE_KImpact))
						a2->eventKImpact(a1, upos, uvel, unorm);
				}
			}
		}else
		{
			if(a1 && (relVelMag * K_ME2UScale > a1->KParams->KImpactThreshold) && a1->IsProbing(ENGINE_KImpact))
				a1->eventKImpact(a2, upos, uvel, unorm);

			if(a2 && (relVelMag * K_ME2UScale > a2->KParams->KImpactThreshold) && a2->IsProbing(ENGINE_KImpact))
				a2->eventKImpact(a1, upos, uvel, unorm);
		}
    }

    /*** TYRE MODEL ***/
    guard(TireModel);

	// Find which actor is the tyre, and which is the ground its on.
    AKTire* tire1 = Cast<AKTire>(a1);
    AKTire* tire2 = Cast<AKTire>(a2);

    AKTire* t;
	AActor* ground;
	if(tire1)
	{
		t = tire1;
		ground = a2;
	}
	else
	{
		t = tire2;
		ground = a1;
	}

    /* if this is a tyre/non-tyre collision, we do the tyre model stuff */
    if(t && !(tire1 && tire2))
    {
        AKCarWheelJoint* cw = t->WheelJoint;
        if(!cw)
           goto endContact;

		MdtConstraintID constraint = cw->getKConstraint();
		if(!constraint)
			goto endContact;

        MdtCarWheelID mdtCW = MdtConstraintDCastCarWheel(constraint);
        if(!mdtCW)
            goto endContact;

        MdtBodyID tBody = McdModelGetBody(t->getKModel());
        if(!tBody)
            goto endContact;

		// Do contact filtering for tri-list (ie when there is no ground actor).
		McdContact *cp1 = &(result->contacts[0]);
		if(!ground && colC != cp1) 
		{
			check(triData);

			// If its an edge or vertex contact, and its angle is too similar to our first face contact,
			// throw it away.
			if((triFlags != 2) && MeVector3Dot(normal, cp1->normal) > 0.5f) 
			{
				// Straightened normal is too similar to cp1's.  We can kill it.
				return 0;
			}
		}

#if 0
		// Set all normal direction to the same as the first face contact.
		if (colC == cp1 || ((colC->dims>>8) == 2)) 
		{
			colC->normal[0] = normal[0];
			colC->normal[1] = normal[1];
			colC->normal[2] = normal[2];

			MdtContactSetNormal(dynC, normal[0], normal[1], normal[2]);
			MdtContactSetPosition(dynC, position[0], position[1], position[2]);
			MdtContactSetPenetration(dynC, penetration);
		}
#endif

        /* calculate rolling direction of tyre at contact by cross product of wheel axis with contact normal */
        MeVector3 haxis, dir;
        MdtCarWheelGetHingeAxis(mdtCW, haxis);

        MeVector3Cross(dir, haxis, normal);
        if(MeVector3MagnitudeSqr(dir) < 0.01 * 0.01) /* If this is bad (ie. contact with side of wheel!) */
        {
            MeVector3 a, b;
            MeVector3PlaneSpace(normal, a, b); /* Pick any direction orth. to normal */
            MeVector3Copy(dir, a);
        }
        else
            MeVector3Normalize(dir);

        MdtContactSetDirection(dynC, dir[0], dir[1], dir[2]);

		// Calculate how far up the sphere the contact is.
		// Use frictionless contacts (and do no more tyre model stuff) if its less than 45 degrees (ie sides of wheel).
		MeReal contactAngle = MeVector3Dot(haxis, normal);
		if(fabs(contactAngle) > 0.7071f)
		{
			MdtContactParamsSetType(params, MdtContactTypeFrictionZero);
		}
		else // TYRE MODEL HERE!
		{
			MdtContactParamsSetType(params, MdtContactTypeFriction2D);
			MdtContactParamsSetFrictionModel(params, MdtFrictionModelNormalForce);

			if(triData)
			{
				MdtContactParamsSetPrimaryFrictionCoeffecient(params, t->RollFriction * triData->localFriction);
				MdtContactParamsSetSecondaryFrictionCoeffecient(params, t->LateralFriction * triData->localFriction);
			}
			else
			{
				MdtContactParamsSetPrimaryFrictionCoeffecient(params, t->RollFriction);
				MdtContactParamsSetSecondaryFrictionCoeffecient(params, t->LateralFriction);
			}

			// calculate slip (force proportional to velocity) terms
			MeVector3 angVel;
			MdtBodyGetAngularVelocity(tBody, angVel);
			MeReal meSpinSpeed = MeFabs(MeVector3Dot(haxis, angVel)); // rad/sec
			t->SpinSpeed = K_Rad2U * meSpinSpeed; // 65535 = 1 rev/sec

			MeReal priSlip = MeMIN(t->RollSlip, t->MinSlip + (meSpinSpeed * t->SlipRate));
			MeReal secSlip = MeMIN(t->LateralSlip, t->MinSlip + (meSpinSpeed * t->SlipRate));

			//debugf(TEXT("Slip: %f, %f"), priSlip, secSlip);

			// Fabs just here to be sure :)
			MdtContactParamsSetPrimarySlip(params, MeFabs(priSlip));
			MdtContactParamsSetSecondarySlip(params, MeFabs(secSlip));

			// normal direction
			MdtContactParamsSetRestitution(params, t->Restitution);
			MdtContactParamsSetMaxAdhesiveForce(params, t->Adhesion);
			MdtContactParamsSetSoftness(params, t->Softness);

			// This flag is reset at the start of KUpdateContacts.
			// Note - this flag not set for wheel-side contacts.
			t->bTireOnGround = 1;

			// Note with this stuff, if there are multiple contacts, it will just be the last contact,
			// but I can't think of a better way to choose. Normally there will be only 1.

			// Subtract component of velocity in normal direction and cal magnitude
			MeReal normVelMag = MeVector3Dot(relVel, normal);
			MeVector3MultiplyAdd(relVel, -normVelMag, normal);
			t->GroundSlipVel = MeVector3Magnitude(relVel); // used for squeels etc.
			KME2UPosition(&(t->GroundSlipVec), relVel);

			// Set the material that the tyre is on.
			if(triData)
			{
				check(!ground);
				t->GroundMaterial = triData->localMaterial;
				//t->GroundSurfaceType = t->GroundMaterial->SurfaceType;
			}
			else
			{
				check(ground);
				t->GroundMaterial = NULL;
				//t->GroundSurfaceType = ground->SurfaceType;
			}

#if 0
			if(!MdtCarWheelIsSteeringLocked(mdtCW))
			{
				TCHAR propName[512];
				appSprintf(propName, TEXT("%s-PriSlip"), t->GetName());
				//GStatGraph->AddDataPoint(propName, priSlip, 1);

				appSprintf(propName, TEXT("%s-SecSlip"), t->GetName());
				//GStatGraph->AddDataPoint(propName, secSlip, 1);

				appSprintf(propName, TEXT("%s-NormForce"), t->GetName());
				MeVector3 normForce;
				MdtContactGetForce(dynC, 0, normForce);
				GStatGraph->AddDataPoint(propName, MeSqrt(normForce[2] * normForce[2]), 1);

				appSprintf(propName, TEXT("%s-TanForce"), t->GetName());
				GStatGraph->AddDataPoint(propName, MeSqrt(normForce[0] * normForce[0] + normForce[1] * normForce[1]), 1);

				appSprintf(propName, TEXT("%s-SlipVel"), t->GetName());
				GStatGraph->AddDataPoint(propName, t->GroundSlipVel, 1);
				//debugf(TEXT("SS:%s-%f"), propName, t->GroundSlipVel);
			}
#endif
		}
    }
    
    unguard; // TireModel

    

endContact:
	// Done inside KLevelDebugDrawConstraints now
    //KConstraintDraw(MdtContactQuaConstraint(dynC), KGData->DebugDrawOpt, KLineDraw);

    return 1;

    unguard;
}

/* Called for pairs of contacts. */
static MeBool MEAPI KPerPairCB(McdIntersectResult* result, MdtContactGroupID c)
{
    guard(KPerPairCB);
    return 1;
    unguard;
}

/********************** INIT/TERM *************************/


/*	This destroys the model, and the geometry if nothing else needs it. 
    Will also try to shutdown if bKGShutdownPending and no models/geometry left. */
void MEAPI KModelDestroy(McdModelID model)
{
    guard(KModelDestroy);

	MeMatrix4Ptr modelTM = McdModelGetTransformPtr(model);
    McdGeometryID geom = McdModelGetGeometry(model);

	KarmaModelUserData* data = (KarmaModelUserData*)McdModelGetUserData(model);
	check(data->OverlapModels.Num() == 0); // Check we have been properly removed from any pairs.
	delete data;

    McdModelDestroy(model);
    (KGData->ModelCount)--;
	
	MeMemoryAPI.destroyAligned(modelTM);
	
	/*  We dont want to destroy the Null geometry in the Geometry Manager. */
    if(McdGeometryGetTypeId(geom) != kMcdGeometryTypeNull)
    {
        KGMDestroyGeometry(KGData->GeomMan, geom);
    }

    KTermGameKarma(); // 'Try' and shut down.
    unguard;
}

/*  Add dynamics to an Actor.
    Should call KInitActorCollision first. */
void MEAPI KInitActorDynamics(AActor* actor)
{
    guard(KInitActorDynamics);

    if(!KGData || actor->bDeleteMe) 
        return;

    ULevel* level = actor->GetLevel();
    if(GIsEditor || !KGData->Framework || !level || actor->bDeleteMe)
        return;

	if(actor->bStatic)
		debugf(TEXT("(Karma): KInitActorDynamics: bStatic is true."));

	RTN_WITH_ERR_IF(!actor->KParams, "(Karma): KInitActorDynamics: No KParams.");

    // Check we have collision set up.
    McdModelID model = actor->getKModel();
	RTN_WITH_ERR_IF(!model, "(Karma): KInitActorDynamics: No Model.");

    MdtBodyID body = McdModelGetBody(model);
    if(body)
        return; /* We already have dynamics. */

    RTN_WITH_ERR_IF(actor->Physics != PHYS_Karma, "(Karma): KInitActorDynamics: Not in a Karma PHYS mode.");

	// We need to have a KarmaParams, a KarmaParamsCollision isn't enough.
	UKarmaParams* kparams = Cast<UKarmaParams>(actor->KParams);
    RTN_WITH_ERR_IF(!kparams, "(Karma): KInitActorDynamics: No KarmaParams.");

    MdtWorldID world = actor->GetLevel()->KWorld;
    RTN_WITH_ERR_IF(!world, "(Karma): KInitActorDynamics: No Karma World");

	// // GET SOME PHYSICS PROPERTIES // //
	UKMeshProps* mp = 0;

	// Try the static mesh
	if( actor->StaticMesh )
		mp = actor->StaticMesh->KPhysicsProps;

	// Now try a skeletal mesh
	if( !mp )
	{
		USkeletalMesh* smesh = 0;
		if(actor->Mesh)
			smesh = Cast<USkeletalMesh>(actor->Mesh);
		if(smesh)
			mp = smesh->KPhysicsProps;
	}

	// Last check is a KarmaParamsRBFull
	// We have to get inertia tensor/com-offset from _somewhere_!
    if( !mp && !kparams->IsA(UKarmaParamsRBFull::StaticClass()) )
	{
		debugf(TEXT("(Karma): KInitActorDynamics: (%s) No StaticMesh or KarmaParamsRBFull"), actor->GetName());
		return;
	}

    /* Get 'static' model transformation, to free later */
    MeMatrix4Ptr oldTM = McdModelGetTransformPtr(model);

    /* Create a new body and set all its properties from the asset. */
    body = MdtBodyCreate(world);

    MeMatrix4 mMatrix;
    KU2METransform(mMatrix, actor->Location, actor->Rotation);
    MdtBodySetTransform(body, mMatrix);
    McdModelSetBody(model, body);

	// Use DrawScale as physics scaling
	kparams->KScale = actor->DrawScale;
	kparams->KScale3D = actor->DrawScale3D;

	// First set mass properties from data stored with StaticMesh.
	if(mp)
	{
		MeMatrix3 I;
		FVector totalScale = kparams->KScale3D * kparams->KScale;
		I[0][0] =			mp->InertiaTensor[0] * kparams->KMass * totalScale.Y * totalScale.Z;
		I[0][1] = I[1][0] = mp->InertiaTensor[1] * kparams->KMass * totalScale.X * totalScale.Y;
		I[0][2] = I[2][0] = mp->InertiaTensor[2] * kparams->KMass * totalScale.Z * totalScale.X;
		I[1][1] =			mp->InertiaTensor[3] * kparams->KMass * totalScale.X * totalScale.Z;
		I[1][2] = I[2][1] = mp->InertiaTensor[4] * kparams->KMass * totalScale.Y * totalScale.Z;
		I[2][2] =			mp->InertiaTensor[5] * kparams->KMass * totalScale.X * totalScale.Y;

		KBodySetInertiaTensor(body, I);
		KBodySetMass(body, kparams->KMass);
		
		MeVector3 o;
		KU2MEVecCopy(o, mp->COMOffset * totalScale);
		MeDictNode *node = MeDictFirst(&body->constraintDict);
		if(!node)
			MdtBodySetCenterOfMassRelativePosition(body, o);
	}

    // Set body properties from KParams parameters.
	// If KParams is a KarmaParamsRBFull it will use inertia-tensor/com-offset from there.
    actor->KParams->PostEditChange();

    //MeMemoryAPI.destroy(oldTM);
	MeMemoryAPI.destroyAligned(oldTM);


	if(kparams->KStartEnabled)
	{
		MdtBodyEnable(body); // This should call AddActiveActorFromBody
	}
	else
	{
		MdtBodyDisable(body); // This should call RemoveActiveActorFromBody
	}

	// Finally, set startup linear ang angular velocity 
	MeVector3 meLinVel, meAngVel;
	KU2MEPosition(meLinVel, kparams->KStartLinVel);
	KU2MEPosition(meAngVel, kparams->KStartAngVel);
	MdtBodySetLinearVelocity(body, meLinVel[0], meLinVel[1], meLinVel[2]);
	MdtBodySetAngularVelocity(body, meAngVel[0], meAngVel[1], meAngVel[2]);

    unguard;
}

/*  Remove dynamics from an Actor (leaving just collision). */
void MEAPI KTermActorDynamics(AActor* actor)
{
    guard(KTermActorDynamics);

    if(!KGData) 
        return;

    ULevel* level = actor->GetLevel();
    McdModelID model = actor->getKModel();
	if(!model || !level)
		return;
	
    MdtBodyID body = McdModelGetBody(model);
	if(!body)
		return;
    
	UKarmaParams* kparams = Cast<UKarmaParams>(actor->KParams);

	// Remove any Angular3's on this Actor.
	if(kparams && kparams->KAng3)
	{
		MdtAngular3ID ang3 = (MdtAngular3ID)kparams->KAng3;
		MdtAngular3Disable(ang3);
		MdtAngular3Destroy(ang3);
		kparams->KAng3 = NULL;
		kparams->bKStayUpright = 0;
	}

	// Remove any KRepulsor contacts on this actor.
	if(kparams && kparams->Repulsors.Num() > 0)
	{
		for(INT i=0; i<kparams->Repulsors.Num(); i++)
		{
			AKRepulsor* rep = kparams->Repulsors(i);
			if(rep->KContact)
			{
				MdtContactGroupID cg = (MdtContactGroupID)rep->KContact;
				MdtContactGroupDisable(cg);
				MdtContactGroupDestroy(cg);
				rep->KContact = NULL;
			}
		}
	}

	// Do any vehicle clean-up (removing contacts)
	ASVehicle* v = Cast<ASVehicle>(actor);
	if(v)
		KTermSVehicleDynamics(v);

	// Term and KConstraints to this body.
	KBodyTermKConstraints(body);
        
    // Allocate matrix, copy MdtBody transform into it and destroy MdtBody.
	KSetSecName(TEXT("KARMA: MODEL TM"));
    MeMatrix4Ptr modelTM = (MeMatrix4Ptr)MeMemoryAPI.createAligned(sizeof(MeMatrix4), 16);
	KSetSecName(TEXT("KARMA: POST MODEL TM"));

    MdtBodyGetTransform(body, modelTM);

    MdtBodyDisable(body); // _should_ freeze model, if in space
    
    McdModelSetBody(model, 0);
    McdModelSetTransformPtr(model, modelTM);

    MdtBodyDestroy(body);

	// We do a final update on its bounding box to make sure.
    McdGeometryID geom = McdModelGetGeometry(model);
    if(McdGeometryGetTypeId(geom) != kMcdGeometryTypeNull)
    {
        McdModelUpdate(model);
    }

	// Make sure actor is not in KContactGenActors array.
	KActorContactGen(actor, 0);

    unguard;
}

// Cleanup all McdGeometry currently in this StaticMesh. Must be called before the StaticMesh is destroyed.
void MEAPI KTermStaticMeshCollision(UStaticMesh* smesh)
{
	// Iterate over all McdGeometrys, decrementing reference count.
	int i;
	for(i=0; i<smesh->KCollisionGeom.Num(); i++)
	{
		McdGeometryID geom = smesh->KCollisionGeom(i);
		if(geom)
		{
			McdGeometryDecrementReferenceCount(geom);
			KGMDestroyGeometry(KGData->GeomMan, geom); // this will try to delete geometry, if nothing else needs it.
			smesh->KCollisionGeom(i) = 0;
		}
	}
	
	KTermGameKarma(); // In case we need to shut down.
}



static McdGeometryID KCreateActorGeometry(AActor* actor)
{
	guard(KCreateActorGeometry);

	McdGeometryID geom = 0;
	
	USkeletalMesh* smesh = 0;
	if(actor->Mesh)
		smesh = Cast<USkeletalMesh>(actor->Mesh);

	FVector scale3D = actor->DrawScale * actor->DrawScale3D;

	// For actors with a static mesh which has Karma physics props - use that.
	if(actor->StaticMesh && actor->StaticMesh->KPhysicsProps && actor->StaticMesh->UseSimpleKarmaCollision)
	{
		KSetSecName(TEXT("KARMA: INIT COLLISION: GEOM: STATICMESH"));

		if(actor->StaticMesh->KPhysicsProps->AggGeom.GetElementCount() == 0)
		{
			debugf(TEXT("(Karma): StaticMesh (%s) with empty Karma KAggregateGeometry."), actor->StaticMesh->GetName() );
			return NULL;
		}

		// First - see if this geometry has already been created at (about) the correct scale.
		for(INT i=0; i<actor->StaticMesh->KCollisionGeomScale3D.Num(); i++)
		{
			if((actor->StaticMesh->KCollisionGeomScale3D(i) - scale3D).IsNearlyZero())
				geom = actor->StaticMesh->KCollisionGeom(i); // yes it has!
		}

		if(!geom)
		{
			// If it hasn't, then create it here, and add to static meshes list of geoms.
			geom = KAggregateGeomInstance(&actor->StaticMesh->KPhysicsProps->AggGeom, 
				actor->DrawScale * actor->DrawScale3D,
				KGData->GeomMan, actor->GetName());

			if(geom)
			{
				actor->StaticMesh->KCollisionGeomScale3D.AddItem(scale3D);
				actor->StaticMesh->KCollisionGeom.AddItem(geom);
				McdGeometryIncrementReferenceCount(geom); // Count this StaticMeshes reference to this geometry.
			}
		}
	}
	// Skeletal meshes with karma collision - for use with vehicles
	else if(smesh && smesh->KPhysicsProps)
	{
		KSetSecName(TEXT("KARMA: INIT COLLISION: GEOM: SKELETALMESH"));

		if(!scale3D.IsUniform())
		{
			debugf(TEXT("(Karma): Actor (%s) with non uniform scale factor cannot use skeletal mesh karma collision."), actor->GetName() );
			return NULL;
		}

		if(smesh->KPhysicsProps->AggGeom.GetElementCount() == 0)
		{
			debugf(TEXT("(Karma): SkeletalMesh (%s) with empty Karma KAggregateGeometry."), smesh->GetName() );
			return NULL;
		}

		geom = KAggregateGeomInstance(&smesh->KPhysicsProps->AggGeom, 
				actor->DrawScale * actor->DrawScale3D,
				KGData->GeomMan, actor->GetName());
	}
	// Pawns without static meshes dont create collision.
	else if(actor->IsA(APawn::StaticClass()))
	{
		return NULL;
	}
	// For blocking volumes, decompose into convex polytopes and make aggregate on load.
	else if(actor->IsA(ABlockingVolume::StaticClass()) && actor->Brush)
	{
		KSetSecName(TEXT("KARMA: INIT COLLISION: GEOM: BLOCKING VOLUME"));

		FKAggregateGeom aggGeom;

		// First, convert model into several convex hulls.
		KModelToHulls(&aggGeom, actor->Brush, actor->PrePivot);

		if(aggGeom.ConvexElems.Num() == 0)
		{
			debugf( TEXT("(Karma): Could not create Karma collision from blocking volume: %s"), actor->GetName() );
			return NULL;
		}

		// Then instance that geometry using Karma.
		// Global count is incremented inside this function.
		geom = KAggregateGeomInstance(&aggGeom, FVector(1,1,1), KGData->GeomMan, actor->GetName());
	}

	return geom;

	unguard;
}

/*  Create the McdModel for an actor.
	If makeNull == true, create a 'null' geometry that wont collide with anything.
	For things with StaticMeshes with Karma collision props, use that.
	For Pawns without static meshes, use a cylinder.
	For anything else - just its bounding box. 

	This function assumes that the Actor has a KarmaParamsCollision (or child) */
void MEAPI KInitActorCollision(AActor* actor, UBOOL makeNull)
{
    guard(KInitActorCollision);

    if(!KGData || actor->bDeleteMe) 
        return;

    ULevel* level = actor->GetLevel();
    if(GIsEditor || !KGData->Framework || !level)
        return;

    McdModelID model = actor->getKModel();
	if(model)
        return; // already done!

	KSetSecName(TEXT("KARMA: INIT COLLISION: GEOM"));


	// Create required geometry.
	McdGeometryID geom;

	guard(CreateGeometry);
	check(KGData->GeomMan);

	if(makeNull)
		geom = KGData->GeomMan->nullGeom;
	else
		geom = KCreateActorGeometry(actor);

	if(!geom)
		return; // Couldn't create/find geometry for whatever reason

	unguard;

	guard(CreateModel);

	// Create actual model
	KSetSecName(TEXT("KARMA: INIT COLLISION: MODEL"));


	model = McdModelCreate(geom);
	check(model);
	(KGData->ModelCount)++;

    /* Convert actors position/orientation into tm matrix. */
	//MeMatrix4Ptr modelTM = (MeMatrix4Ptr)appMalloc(sizeof(MeMatrix4), TEXT("KARMA: MODEL TM"));
	KSetSecName(TEXT("KARMA: MODEL TM"));
	MeMatrix4Ptr modelTM = (MeMatrix4Ptr)MeMemoryAPI.createAligned(sizeof(MeMatrix4), 16);
	KSetSecName(TEXT("KARMA: POST MODEL TM"));

    KU2METransform(modelTM, actor->Location, actor->Rotation);
	McdModelSetTransformPtr(model, modelTM);

	if(McdGeometryGetTypeId(geom) != kMcdGeometryTypeNull)
		McdModelUpdate(model);
	
	unguard;

	guard(SetModelPointers);

	/* Model <-> Actor pointers. */
	KarmaModelUserData* data = new(KarmaModelUserData);
	check(data);
	data->actor = actor;
	McdModelSetUserData(model, (void*)data);

	check(actor->KParams);
    actor->KParams->KarmaData = (INT)model;

	unguard;

    KSetSecName(TEXT("KARMA: POST INIT COLLISION"));

    unguard;
}

/* Remove _all_ existing dynamics & collision from this Actor. */
void MEAPI KTermActorCollision(AActor* actor)
{
    guard(KTermActorCollision);

    if(!KGData) 
        return;

    RTN_WITH_ERR_IF(!actor->getKModel(), "(Karma): KTermActorCollision: Actor has no collision.");
    
    McdModelID model = actor->getKModel();
    if(McdModelGetBody(model))
    {
        debugf(TEXT("(Karma): KTermActorCollision: Actor still has dynamics. Automatic KTermActorDynamics."));
        KTermActorDynamics(actor);
    }
    
    // Destroy any pairs that this actor's model was involved with.
    KGoodbyeAffectedPairs(model, actor->GetLevel());

    // This will destroy the geometry if its not needed by anything else any more.
    KModelDestroy(model);
	
    actor->KParams->KarmaData = NULL;

    unguard;
}

/*  Initialise collision/constraint/skeleton for an Actor. 
    Dynamics is done inside AActor::physKarma */
void MEAPI KInitActorKarma(AActor* actor)
{
    guard(KInitActorKarma);

    if(!KGData || actor->bDeleteMe) 
        return;

    ULevel* level = actor->GetLevel();

    if(!level || GIsEditor || !KGData->Framework || actor->bDeleteMe)
        return;

	/* *** SKELETAL *** */
	// Try and initialise rag-doll physics (must have a SkeletalMeshInstance)
	// Doesn't actually matter if we fail here - thi is tried again at the start of physKarmaRagDoll.
    if(actor->Physics == PHYS_KarmaRagDoll)
	{
		if( actor->Mesh == NULL || !actor->Mesh->IsA(USkeletalMesh::StaticClass()) )
			return;

		USkeletalMesh* skelMesh = Cast<USkeletalMesh>(actor->Mesh);
		USkeletalMeshInstance* inst = Cast<USkeletalMeshInstance>(skelMesh->MeshGetInstance(actor));

		KInitSkeletonKarma(inst);

		return;
	}
	
	/* *** CONSTRAINT *** */
	// See if its a Constraint - and init.
	AKConstraint* conActor = Cast<AKConstraint>(actor);
	if(conActor)
	{
		// If neither constrained actor has been set, do nothing (do not change physics mode)
		// This is useful when spawning a constraint in game, as you can set it all up and _then_ call SetPhysics(PHYS_Karma)
		if(!conActor->KConstraintActor1 && !conActor->KConstraintActor2)
		{
			conActor->Physics = PHYS_None;
			return;
		}
		else
			KInitConstraintKarma(conActor);

		return;
	}

	/* *** OTHER ACTOR *** */
	// If this actor is supposed to block Karma stuff - give it some kind of Karma collision geometry.
	if(actor->bBlockKarma)
	{
		// If this needs collision, but doesn't have a KarmaParamsCollision, create one here.
		// This will only allow you to turn Karma collision on (not dynamics),
		// but you would need to have given it a KarmaParams already anyway to do that.
		if(!actor->KParams)
		{
			actor->KParams = ConstructObject<UKarmaParamsCollision>( 
				UKarmaParamsCollision::StaticClass(), actor->GetOuter() );
		}
		
		KInitActorCollision(actor, 0);
	}

	if(actor->Physics == PHYS_Karma)
	{
		// If this is physics, but its not supposed to collide, we still need a model,
		// so create a 'null' one now.
		if(!actor->bBlockKarma)
		{
			KInitActorCollision(actor, 1);
		}

		// Then initialise Karma dynamics.
		KInitActorDynamics(actor);
	}

    unguard;
}
    

void MEAPI KCheckActor(AActor* actor)
{
	guard(KCheckActor);

	// Check constraint
	AKConstraint* con = Cast<AKConstraint>(actor);
	if(con)
	{
		if(con->KConstraintActor2 && !con->KConstraintActor1)
			GWarn->MapCheck_Add( MCTYPE_ERROR, actor, 
				TEXT("Constraints to World must have second ConstraintActor as NULL."));

		// JTODO: Do more checking (ie. things connected have KarmaParams)
	}

	// no non-constraint actor should have Physics set to PHYS_Karma unless it has a KParams editinline struct
	if(actor->Physics == PHYS_Karma && !actor->KParams && !actor->IsA(AKConstraint::StaticClass()) ) 
	{
		GWarn->MapCheck_Add( MCTYPE_ERROR, actor, 
			TEXT("Actor using PHYS_Karma has no KParams."));
	}

	// if its set to KarmaRagDoll physics, but without KParams of type KarmaParamsSkel
	if(actor->Physics == PHYS_KarmaRagDoll && (!actor->KParams || !actor->KParams->IsA(UKarmaParamsSkel::StaticClass())))
	{
		GWarn->MapCheck_Add( MCTYPE_ERROR, actor, 
			TEXT("Actor using PHYS_KarmaRagDoll has no KParams of type KarmaParamsSkel."));
	}

	unguard;
}

/* Terminate all dynamics and collision for an Actor. */
void MEAPI KTermActorKarma(AActor* actor)
{
    guard(KTermActorKarma);

    if(!KGData) 
        return;

	/* *** OTHER ACTOR *** */
    if(actor->getKModel())
    {
        /* If there are dynamics on this actor - terminate them. */
        if(McdModelGetBody(actor->getKModel()))
            KTermActorDynamics(actor);

        KTermActorCollision(actor);
		return;
    }

	/* *** SKELETAL *** */
    /* Terminate Skeleton dynamics/collision if present. */
    if(actor->MeshInstance &&
        actor->MeshInstance->IsA(USkeletalMeshInstance::StaticClass()))
    {
        USkeletalMeshInstance* inst = Cast<USkeletalMeshInstance>(actor->MeshInstance);
        KTermSkeletonKarma(inst);
		return;
    }

    /* *** CONSTRAINT *** */
    AKConstraint* conActor = Cast<AKConstraint>(actor);
    if(conActor && conActor->getKConstraint())
    {
        KTermConstraintKarma(conActor);
		return;
    }

    unguard;
}



/*  Initialise Karma for a level.
    theFramework needs to be valid for this to work. */
void MEAPI KInitLevelKarma(ULevel* level)
{
    guard(KInitLevelKarma);

    if(!KGData) 
        return;

    if(GIsEditor || level->KWorld || !KGData->Framework)
        return;
    
    debugf(TEXT("(Karma): Initialising Karma for Level."));
    
    /* ** COLLISION ** */
    guard(Collision);
    KSetSecName(TEXT("KARMA: MCDSPACE"));

    unguard;


    /* ** DYNAMICS ** */
    guard(Dynamics);
    KSetSecName(TEXT("KARMA: MDTWORLD"));

	// Using malloc pools, so max bodies/constraints unnecessary
    level->KWorld = MdtWorldCreate(1, 1, ME_UNIT_LENGTH, ME_UNIT_MASS);
    
	MdtWorldSetCheckSim(level->KWorld, 1);
    MdtWorldSetAutoDisable(level->KWorld, 1);
    MdtWorldSetEpsilon(level->KWorld, ME_EPSILON);
    
    /* Tweak auto-disable thresholds */
 /*   MdtWorldSetAutoDisableVelocityThreshold(level->KWorld, (MeReal)0.02);
    MdtWorldSetAutoDisableAccelerationThreshold(level->KWorld, (MeReal)0.05);
    MdtWorldSetAutoDisableAngularVelocityThreshold(level->KWorld, (MeReal)0.001);
    MdtWorldSetAutoDisableAngularAccelerationThreshold(level->KWorld, (MeReal)0.05);*/
    MdtWorldSetAutoDisableVelocityThreshold(level->KWorld, (MeReal)0.1);
    MdtWorldSetAutoDisableAccelerationThreshold(level->KWorld, (MeReal)0.25);
    MdtWorldSetAutoDisableAngularVelocityThreshold(level->KWorld, (MeReal)0.005);
    MdtWorldSetAutoDisableAngularAccelerationThreshold(level->KWorld, (MeReal)0.25);

	// Set callback for when  
    level->KWorld->bodyDisableCallback = RemoveActiveActorFromBody;
    level->KWorld->bodyEnableCallback = AddActiveActorFromBody;
    level->KWorld->contactGroupDestroyCallback = DestroyContactGroupReferences;

    unguard;

    /* ** BRIDGE ** */
    
    guard(Bridge);
    KSetSecName(TEXT("KARMA: MSTBRIDGE"));

    level->KBridge = KBridgeCreate(KGData->Framework, K_materialsMaxCount);
    MstBridgeSetModelPairBufferSize(level->KBridge, K_bridgeModelPairBufferSize);

    unguard;

	/* ** ASSET FACTORY ** */
	guard(AssetFactory);
    KSetSecName(TEXT("KARMA: ASSETFACTORY"));

	// We dont use the normal MeAssetFactoryCreate function, because we want just one geometry manager
	// for the whole game, but we need a new asset factory for each level.
	//level->KAssetFactory = MeAssetFactoryCreate(level->KWorld, level->KSpace, KGData->Framework);
    level->KAssetFactory = (MeAssetFactory*)appMalloc(sizeof(MeAssetFactory), TEXT("KARMA: ASSET FACTORY"));
    level->KAssetFactory->gm = KGData->GeomMan;
    level->KAssetFactory->geometryPostCreateCB = 0;
    level->KAssetFactory->geometryPostCreateCBUserdata = 0;
    level->KAssetFactory->modelCreateFunc = KModelCreateFromMeFAssetPart;
    level->KAssetFactory->modelPostCreateCB = 0;
    level->KAssetFactory->modelPostCreateCBUserdata = 0;
    level->KAssetFactory->jointCreateFunc = MdtConstraintCreateFromMeFJoint;
    level->KAssetFactory->jointPostCreateCB = 0;
    level->KAssetFactory->jointPostCreateCBUserdata = 0;

	unguard;

    /* ** LEVEL ** */
    
    guard(LevelCollision);
	/*  'Level' triangle-list collision primitive.
        Terrain/StaticMesh/BSP triangles all go in here. 
        We can't use the normal AssetFactory for this tri-list geometry.
     */
    KSetSecName(TEXT("KARMA: TRILIST"));

    /* 'Level' collision model */
    //MeMatrix4Ptr identTM = (MeMatrix4Ptr)appMalloc(sizeof(MeMatrix4), TEXT("KARMA: TRILIST TM"));
    KSetSecName(TEXT("KARMA: MODEL TM"));
    MeMatrix4Ptr identTM = (MeMatrix4Ptr)MeMemoryAPI.createAligned(sizeof(MeMatrix4), 16);
    KSetSecName(TEXT("KARMA: POST MODEL TM"));
    MeMatrix4TMMakeIdentity(identTM);

#if 0
	McdPlaneID triList = McdPlaneCreate(KGData->Framework);
	identTM[3][2] = -7.68f;
#else
	MeVector3 worldMax = {WORLD_MAX,WORLD_MAX,WORLD_MAX};
	MeVector3 worldMin = {-WORLD_MAX,-WORLD_MAX,-WORLD_MAX};
	
	McdTriangleListID triList = McdTriangleListCreate(
        KGData->Framework, worldMin, worldMax, KTRILIST_SIZE, KTriListGenerator);
    (KGData->GeometryCount)++;

	// Need this to look up triangles in callback.
	McdTriangleListSetUserData(triList, (void*)level);
#endif
    
    level->KLevelModel = McdModelCreate(triList);
    (KGData->ModelCount)++;

	McdModelSetTransformPtr(level->KLevelModel, identTM);
	McdModelUpdate(level->KLevelModel);

	KarmaModelUserData* data = new(KarmaModelUserData);
	data->actor = 0;
	McdModelSetUserData(level->KLevelModel, (void*)data);

    unguard;

    McdRequest* defReq = McdFrameworkGetDefaultRequestPtr(KGData->Framework);
    defReq->faceNormalsFirst = 1;
    defReq->contactMaxCount = 10;
    
    /* Set some callbacks to set materials etc. */
    MstBridgeSetPerContactCB(level->KBridge, 
        MstBridgeGetDefaultMaterial(), MstBridgeGetDefaultMaterial(),
        KPerContactCB);
    
    MstBridgeSetPerPairCB(level->KBridge,  
        MstBridgeGetDefaultMaterial(), MstBridgeGetDefaultMaterial(),
        KPerPairCB);
    
    MdtContactParamsID params = MstBridgeGetContactParams(level->KBridge,
        MstBridgeGetDefaultMaterial(), MstBridgeGetDefaultMaterial());

    MdtContactParamsSetType(params, MdtContactTypeFriction2D);
    MdtContactParamsSetFrictionModel(params, MdtFrictionModelNormalForce);
    MdtContactParamsSetFrictionCoeffecient(params, (MeReal)0.5);
    MdtContactParamsSetSoftness(params, ME_CONTACTSOFT);
    MdtContactParamsSetFriction(params, 10);



    KSetSecName(TEXT("KARMA: POST INIT LEVEL"));

    unguard;
}

static void MEAPI OutputType(MdtConstraintID con, void* userData)
{
	if(MdtConstraintDCastHinge(con))
		debugf(TEXT("(Karma:) Hinge"));
	else if(MdtConstraintDCastSkeletal(con))
		debugf(TEXT("(Karma:) Skeletal"));
	else if(MdtConstraintDCastContactGroup(con))
		debugf(TEXT("(Karma:) Contact Group"));
	else if(MdtConstraintDCastBSJoint(con))
		debugf(TEXT("(Karma:) Ball and Socket"));
	else
		debugf(TEXT("(Karma:) Other Constraint"));
}

/* Destroy all dynamics/collision from this level (including from actors). */
void MEAPI KTermLevelKarma(ULevel* level)
{
    guard(KTermLevelKarma);
    
    if(!KGData) 
        return;

    if(!(level->KWorld) || !(level->KBridge))
        return;

    debugf(TEXT("(Karma): Terminating Karma for Level."));

    guard(DePhysAllActors);
    /*  Cleanup Karma (if necessary) 
        First remove Karma stuff from all Actors. */
    for( INT iActor=0; iActor<level->Actors.Num(); iActor++ )
    {
        AActor* actor = level->Actors(iActor);
        if(actor)
        {
			KTermActorKarma(actor);
        }
    }
    unguard;

	check(level->KContactGenActors.Num() == 0);
	check(level->Ragdolls.Num() == 0);

    guard(RemoveLevelColl);
    /* Remove level model */
    if(level->KLevelModel)
    {
		// Make sure no pairs/contacts still reference this model.
		KGoodbyeAffectedPairs(level->KLevelModel, level);

        /* Do this manually - cant use Geometry Manager for tri-list. */
        McdGeometryID lgeom = McdModelGetGeometry(level->KLevelModel);
        MeMatrix4Ptr ltm = McdModelGetTransformPtr(level->KLevelModel);

		KarmaModelUserData* data = (KarmaModelUserData*)McdModelGetUserData(level->KLevelModel);
		check(data->OverlapModels.Num() == 0); // Check we have been properly removed from any pairs.
		check(data->actor == 0);
		delete data;

        McdModelDestroy(level->KLevelModel);
        (KGData->ModelCount)--;
        level->KLevelModel = 0;

        McdGeometryDestroy(lgeom);
        (KGData->GeometryCount)--;

		MeMemoryAPI.destroyAligned(ltm);
        //appFree(ltm);
    }
    unguard;
    
    guard(Bridge);
    /* Then remove bridge/space/world. */
    if(level->KBridge)   
    {
        MstBridgeDestroy(level->KBridge);
        level->KBridge = NULL;
    }
    unguard;

    guard(Collision);

	// Check there are no models still overlapping!
	check(level->OverlapPairs.Num() == 0);

	unguard;

    guard(Dynamics);
    if(level->KWorld)
    {
		INT bodyCount = MdtWorldGetTotalBodies(level->KWorld);
		if(bodyCount != 0)
		{
			debugf(TEXT("(Karma:) KTermLevelKarma: %d Bodies left in MdtWorld."), bodyCount);
		}
		check(bodyCount == 0);

		INT conCount = MdtWorldGetTotalConstraints(level->KWorld);
		if(conCount != 0)
		{
			debugf(TEXT("(Karma:) KTermLevelKarma: %d Constraints left in MdtWorld."), conCount);

			MdtWorldForAllConstraints(level->KWorld, OutputType, 0);
		}
		check(conCount == 0);

        MdtWorldDestroy(level->KWorld);
        level->KWorld = NULL;
    }
    unguard;
    
	guard(AssetFactory);
	if(level->KAssetFactory)
	{
		//MeAssetFactoryDestroy(level->KAssetFactory);
		appFree(level->KAssetFactory);
		level->KAssetFactory = NULL;
	}
	unguard;

    KTermGameKarma();

	// Free any tri-lists allocated for this level.
	while(level->TriListPool.Num() > 0)
	{
		KarmaTriListData* list = level->TriListPool(0);
		level->TriListPool.Remove(0);
		appFree(list);
	}

    debugf(TEXT("(Karma): Level Karma Terminated."));

    unguard;
}



void MEAPI KTickLevelKarma(ULevel* level, FLOAT DeltaSeconds)
{
    guard(KTickLevelKarma);
    
    if(!KGData || !level->KWorld)
        return;
    
	//debugf( TEXT("KTICK: %f"), DeltaSeconds );

	// If we are in playersonly mode - set Karma evolve to false.
	if(level->GetLevelInfo())
		KGData->bAutoEvolve = !level->GetLevelInfo()->bPlayersOnly;

    if(KGData->bAutoEvolve)
        KGData->bDoTick = 1;

	// Work out timesteps to use.
    MeReal meTimeStep, halfMeTimeStep;
	MeReal timeScale = level->GetLevelInfo()->KarmaTimeScale; // Overall Karma timestep scaling.

    if(KGData->bAutoEvolve)
        meTimeStep = timeScale * DeltaSeconds;
    else /* If we are single-stepping use a fixed timestep. */
        meTimeStep = (MeReal)0.03;


	halfMeTimeStep = (MeReal)0.5 * meTimeStep;

	// Ensure no timestep is bigger than allowed max.
	meTimeStep = Min(meTimeStep, ME_MAX_TIMESTEP);
	halfMeTimeStep = Min(halfMeTimeStep, ME_MAX_TIMESTEP);

	///////////// NORMAL RATE /////////////////
    MeReal meGamma = MeCLAMP(ME_GAMMA_PERSEC*meTimeStep, 0, (MeReal)0.5);
    MdtWorldSetGamma(level->KWorld, meGamma);
    KGData->TimeStep = meTimeStep;
    
	// Update triangle-list for each rag-doll.
	clock(GStats.DWORDStats(GEngineStats.STATS_Karma_RagdollTrilist));
	for(INT i=0; i<level->Ragdolls.Num(); i++)
		KUpdateRagdollTrilist(level->Ragdolls(i), 0);
	unclock(GStats.DWORDStats(GEngineStats.STATS_Karma_RagdollTrilist));

	KSetSecName(TEXT("KARMA: UPDATE CONTACTS"));
	clock(GStats.DWORDStats(GEngineStats.STATS_Karma_Collision));
    KUpdateContacts(level->KContactGenActors, level, 0); 
	unclock(GStats.DWORDStats(GEngineStats.STATS_Karma_Collision));
    KSetSecName(TEXT("KARMA: POST UPDATE CONTACTS"));
    
	if(KGData->bDoTick)
	{
		KSetSecName(TEXT("KARMA: WORLDSTEP"));
		clock(GStats.DWORDStats(GEngineStats.STATS_Karma_Dynamics));

		KWorldStepSafeTime(level->KWorld, meTimeStep, level, 0);

		unclock(GStats.DWORDStats(GEngineStats.STATS_Karma_Dynamics));
		KSetSecName(TEXT("KARMA: POST WORLDSTEP"));
	}

	///////////// DOUBLE RATE /////////////////

	FLOAT DTTimeStep;
	INT DTIterations;

	// If the regular timestep is small enough, dont bother double ticking (ie. 1 iteration at full timestep).
	if(meTimeStep < ME_DO_DOUBLETICK_TIMESTEP)
	{
		DTTimeStep = meTimeStep;
		DTIterations = 1;
	}
	else
	{
		DTTimeStep = halfMeTimeStep;
		DTIterations = 2;
	}

	meGamma = MeCLAMP(ME_GAMMA_PERSEC*DTTimeStep, 0, (MeReal)0.5);
	MdtWorldSetGamma(level->KWorld, meGamma);
	KGData->TimeStep = DTTimeStep;

	for(INT t=0; t<DTIterations; t++)
	{
		// Update triangle-list for each rag-doll.
		clock(GStats.DWORDStats(GEngineStats.STATS_Karma_RagdollTrilist));
		for(INT i=0; i<level->Ragdolls.Num(); i++)
			KUpdateRagdollTrilist(level->Ragdolls(i), 1);
		unclock(GStats.DWORDStats(GEngineStats.STATS_Karma_RagdollTrilist));

		KSetSecName(TEXT("KARMA: UPDATE CONTACTS"));
		clock(GStats.DWORDStats(GEngineStats.STATS_Karma_Collision));
		KUpdateContacts(level->KContactGenActors, level, 1); 
		unclock(GStats.DWORDStats(GEngineStats.STATS_Karma_Collision));
		KSetSecName(TEXT("KARMA: POST UPDATE CONTACTS"));

		if(KGData->bDoTick)
		{
			KSetSecName(TEXT("KARMA: WORLDSTEP"));
			clock(GStats.DWORDStats(GEngineStats.STATS_Karma_Dynamics));

			KWorldStepSafeTime(level->KWorld, DTTimeStep, level, 1);

			unclock(GStats.DWORDStats(GEngineStats.STATS_Karma_Dynamics));
			KSetSecName(TEXT("KARMA: POST WORLDSTEP"));
		}
	}

    unguard;
}

/* Called once at startup - stuff that persists during entire game. */
void MEAPI KInitGameKarma()
{
    guard(KInitGameKarma);

    if(GIsEditor)
    {
        debugf(TEXT("(Karma): Not Initialising Karma In Editor."));
        return;
    }

    if(KGData == 0)
    {
        debugf(TEXT("(Karma): Beginning Karma for game."));

        KGData = (KarmaGlobals*)appMalloc(sizeof(KarmaGlobals), TEXT("KarmaGlobals"));
        appMemset(KGData, 0, sizeof(KarmaGlobals));
        KGData->bAutoEvolve = 1;
        KGData->bDoTick = 1;
		KGData->bUseSafeTime = 1;
		//KGData->bUseSafeTime = 0;

        guard(SetHandlers);
        KSetSecName(TEXT("KARMA: SET HANDLERS"));

        /* Route memory allocation through Unreal memory allocator */
        MeMemoryAPI.create =		&KMemCreate;
        MeMemoryAPI.destroy =		&KMemDestroy;
        MeMemoryAPI.resize =		&KMemResize;
        MeMemoryAPI.createZeroed =	&KMemCreateZeroed;

        /* Don't use fixed size pools - use malloc instead. */
		MePoolAPI.init =			&KPoolMallocInit;
		MePoolAPI.destroy =			&KPoolMallocDestroy;
		MePoolAPI.deset =			&KPoolMallocReset;
		MePoolAPI.getStruct =		&KPoolMallocGetStruct;
		MePoolAPI.putStruct =		&KPoolMallocPutStruct;
		MePoolAPI.getUsed =			&KPoolMallocGetUsed;
		MePoolAPI.getUnused =		&KPoolMallocGetUnused;

        /* Route messages through Unreal logging */
        MeSetInfoShow(KMessageShow);
        MeSetWarningShow(KMessageShow);
        MeSetFatalErrorShow(KMessageShow);
        MeSetDebugShow(KMessageShow);

        MeSetInfoHandler(KDebugHandler);
        MeSetWarningHandler(KDebugHandler);
        MeSetFatalErrorHandler(KErrorHandler);
        MeSetDebugHandler(KDebugHandler);
        
        unguard;

        guard(Collision);
        KSetSecName(TEXT("KARMA: MCDINIT"));

        // Most stuff lives with a ULevel - but we need this to keep McdGeometry across levels.
		// Using malloc pools, so geom instance max count unnecessary
        KGData->Framework = McdInit(
            K_collisionGeometryTypesMaxCount, 
            K_collisionModelsMaxCount,
            1, ME_UNIT_LENGTH);

		KSetSecName(TEXT("KARMA: MCDINIT: REGISTER"));

        McdPrimitivesRegisterTypes(KGData->Framework);
        McdConvexMeshRegisterType(KGData->Framework);
        McdAggregateRegisterType(KGData->Framework);
        McdNullRegisterType(KGData->Framework);

        McdPrimitivesRegisterInteractions(KGData->Framework);
        McdConvexMeshPrimitivesRegisterInteractions(KGData->Framework);
        McdAggregateRegisterInteractions(KGData->Framework);

        //McdConvexMeshConvexMeshRegisterInteraction(KGData->Framework);
        //McdTriangleListConvexMeshRegisterInteractions(KGData->Framework);

		KSetSecName(TEXT("KARMA: MCDINIT: MODELPAIRCONTAINER"));

		KGData->filterPairs = McdModelPairContainerCreate(K_bridgeModelPairBufferSize);

		KGData->ModelCount = 0;
		KGData->GeometryCount = 0;
        unguard;

        guard(AssetDB);
        KSetSecName(TEXT("KARMA: MEFILE"));

        /* Load all geometry into KGeometryMeFile */
        KCreateAssetDB(&KGData->AssetDB, &KGData->GeomMan);
        unguard;
    }
    else
        debugf(TEXT("(Karma): KInitGameKarma: Already Initialised!"));

    KSetSecName(TEXT("KARMA: POST INIT GAME"));

    unguard;
}

/* 
    Cleanup on exit. 
    This function is 'safe' because it will do nothing if the game is not shutting down,
    or there are still any geometries/models around.
*/
void ENGINE_API KTermGameKarma()
{
    guard(KTermGameKarma);

    if(!KGData) 
        return;

    if(!KGData->bShutdownPending || !KGData->GeomMan || 
        McdGMGetGeomCount(KGData->GeomMan) > 0 ||
		KGData->GeometryCount > 0 ||
		KGData->ModelCount > 0 )
        return;

	if(KGData->bShutdownPending && McdGMGetGeomCount(KGData->GeomMan) < 0)
		debugf(TEXT("(Karma): Negative GeometryManager Geometry Count on Shutdown."));
	
	if(KGData->bShutdownPending && KGData->GeometryCount < 0)
		debugf(TEXT("(Karma): Negative Geometry Count on Shutdown."));
		
	if(KGData->bShutdownPending && KGData->ModelCount < 0)
		debugf(TEXT("(Karma): Negative Model Count on Shutdown."));	

    debugf(TEXT("(Karma): Ending Karma for game."));

    guard(ModelPairContainer);
	if(KGData->filterPairs)
	{
		McdModelPairContainerDestroy(KGData->filterPairs);
		KGData->filterPairs = NULL;
	}
	unguard;

    guard(GeomMan);
    if(KGData->GeomMan)
    {
        debugf(TEXT("(Karma): Destroying Geometry Manager."));
        McdGMDestroy(KGData->GeomMan);
        KGData->GeomMan = NULL;
    }
    unguard;

    guard(AssetDB);
    if( KGData->AssetDB )
    {
        debugf(TEXT("(Karma): Destroying Asset Database."));
        MeAssetDBDestroy(KGData->AssetDB);
        KGData->AssetDB = NULL;
    }
    unguard;

    guard(Collision);
    if(KGData->Framework)
    {
        debugf(TEXT("(Karma): Destroying Framework."));
        McdTerm(KGData->Framework);
        KGData->Framework = NULL;
    }
    unguard;

	KGData->StaticMeshTris.Empty();

    appFree(KGData);
    KGData=NULL;

    unguard;
}

/* ********************* */

// 'newBlock' indicates if this Actor should have Karma collision now.
// Doesn't look at bBlockKarma.
void MEAPI KSetActorCollision(AActor* actor, UBOOL newBlock)
{
	guard(KSetActorCollision);

	// Do nothing for constraints!
	if(actor->IsA(AKConstraint::StaticClass()) || actor->bDeleteMe)
		return;

	ULevel* level = actor->GetLevel();

	/////////// UPDATE KCONTACTGENACTORS ////////////

	// Turned off bBlockKarma - make sure actor is not in KContactGenActors.
	if(!newBlock)
		KActorContactGen(actor, 0);
	// Turning on bBlockKarma - add to KContactGenActors if in a Karma physics mode.
	else if( actor->bCollideActors && (actor->Physics == PHYS_KarmaRagDoll || actor->Physics == PHYS_Karma) )
		KActorContactGen(actor, 1);

	//////// SKELETAL CASE /////////
	// For ragdoll karma, we dont destroy the geometry, we just clean up any existing pairs.
	// (And that only needs doing if we are turning off collision)
	// If we are turning collision on, we will just start generating pairs!
	if(actor->Physics == PHYS_KarmaRagDoll)
	{
		if(!newBlock)
			KGoodbyeActorAffectedPairs(actor);

		return;
	}

	//////// KARMA ACTOR CASE /////////
	McdModelID model = actor->getKModel();

	if(!model)
	{
		// If there is no model, and our new state is to have no Karma collision, we're done.
		if(!newBlock)
			return;
		else
		{
			// Create a model and geometry.
			KInitActorCollision(actor, 0);
			return;
		}
	}

	// We already have a model

	// Just in case its a model with no body, and we are turning off collision,
	// we might as well just remove it. A model with null geometry and no body
	// is just silly.
	if(!McdModelGetBody(model) && !newBlock)
	{
		KTermActorCollision(actor);
		return;
	}


	// Otherwise, we need to change its current geometry.
	McdGeometryID geom = McdModelGetGeometry(model);
	if(McdGeometryGetTypeId(geom) == kMcdGeometryTypeNull)
	{
		// If geometry is type 'null', and our new state is to have no Karma collision, we're done.
		if(!newBlock)
			return;
		else
		{
			// Otherwise, we need to create the right geometry here, and change the McdModel to it.
			McdGeometryID newGeom = KCreateActorGeometry(actor);
			McdModelSetGeometry(model, newGeom);
			// Don't destroy old geometry because it was the 'null' one (part of McdGeomMan).
		}
	}
	else
	{
		// If geometry is non-null, and our new state is having Karma collision, we're done.
		if(newBlock)
			return;
		else
		{
			// Otherwise, assign this model the 'null' geometry here.
			
			// Clean up any pairs with other models
			KGoodbyeAffectedPairs(model, level);

			McdModelSetGeometry(model, KGData->GeomMan->nullGeom);

			// TEMPORARY BUG WORKAROUND (FIXED IN KARMA 1.2)
			model->mInstance.child = 0;

			// Destroy old geometry, if no longer being used.
			KGMDestroyGeometry(KGData->GeomMan, geom);
		}
	}

	unguard;
}

#endif // WITH_KARMA
