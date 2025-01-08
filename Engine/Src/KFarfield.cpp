/*============================================================================
	Karma Farfield
    
    - Code for generating/handling pairs of McdModels as startand stop 
	  overlapping.
============================================================================*/

#include "EnginePrivate.h"

#ifdef WITH_KARMA
#include "McdBatch.h"

#define DISPLAY_HELLO	(0)
#define DISPLAY_GOODBYE	(0)

#define MODELGEOM_NOT_NULL(m) (McdGeometryGetTypeId(McdModelGetGeometry(m)) != kMcdGeometryTypeNull)

// This should give the same key regardless of model order.
QWORD KModelsToKey(McdModelID m1, McdModelID m2)
{
	guardSlow(KModelsToKey);

	check(sizeof(QWORD) == 2 * sizeof(McdModelID));

	// Make sure m1 is the 'lower' pointer.
	if(m2 > m1)
	{
		McdModelID tmp = m2;
		m2 = m1;
		m1 = tmp;
	}

	QWORD Key;
	McdModelID* models = (McdModelID*)&Key;
	models[0] = m1;
	models[1] = m2;

	return Key;

	unguardSlow;
}

//////////////////////////////////// KUPDATECONTACTS UTILS //////////////////////////////////////////////////////////

// Get any collision data out of this actor. That might be none though!
static void GetModels(AActor* actor, McdModelID &model, USkeletalMeshInstance* &inst)
{
	guard(GetModels);

	inst = NULL;
	model = NULL;

	model = actor->getKModel();
	if(model)
		return;

	if(actor->Mesh && actor->Mesh->IsA(USkeletalMesh::StaticClass()))
	{
		USkeletalMesh* skelMesh = Cast<USkeletalMesh>(actor->Mesh);
		inst = Cast<USkeletalMeshInstance>(skelMesh->MeshGetInstance(actor));
	}
	if(inst && !inst->KSkelIsInitialised)
		inst = NULL;

	unguard;
}

static UBOOL CheckModelOverlap(McdModelID m1, McdModelID m2)
{
	guard(CheckModelOverlap);

	MeVector3 m1Min, m1Max;
	McdModelGetAABB(m1, m1Min, m1Max);

	MeVector3 m2Min, m2Max;
	McdModelGetAABB(m2, m2Min, m2Max);

	if(	m2Min[0] > m1Max[0] || m1Min[0] > m2Max[0] ||
		m2Min[1] > m1Max[1] || m1Min[1] > m2Max[1] ||
		m2Min[2] > m1Max[2] || m1Min[2] > m2Max[2] )
		return 0;
	else
		return 1;

	unguard;
}

static UBOOL ActorsShouldCollide(AActor* a1, AActor* a2)
{
	guard(ActorsShouldCollide);

	// Currently, dont let ragdolls collide with each other or regular karma stuff.
	if((a1->Physics == PHYS_KarmaRagDoll || a2->Physics == PHYS_KarmaRagDoll))
	{
		// the other thing is a Karma object.
		if(a1->Physics == PHYS_Karma || a2->Physics == PHYS_Karma)
			return 0;

		// its ragdoll-ragdoll (a1 & a2 are not the same)
		check(a1 != a2);
		if(a1->Physics == PHYS_KarmaRagDoll && a2->Physics == PHYS_KarmaRagDoll)
			return 0;
	}

	// Dont allow things with  different bKDoubleTickRate to collide. 
	// JTODO: Can we fix this? Set bKDoubleTickRate on the fly?
	// NB. Should always have KParams at this point.
	UKarmaParams* kparams1 = Cast<UKarmaParams>(a1->KParams);
	UKarmaParams* kparams2 = Cast<UKarmaParams>(a2->KParams);
	if(kparams1 && kparams2 && kparams1->bKDoubleTickRate != kparams1->bKDoubleTickRate)
	{
		debugf(TEXT("ActorsShouldCollide: Allowed collion between actors with different bKDoubleTickRate"));
		return 0;
	}

	return 1;

	unguard;
}

// For generating model pairs betweeen different parts of the same skeletal asset.
// Will use the KSkelDisableTable to ignore certain pairs of models.
static void GenerateSelfOverlapPairs(USkeletalMeshInstance* inst, 
									 TArray<McdModelID>& PairM1, 
									 TArray<McdModelID>& PairM2 )
{
	guard(GenerateSelfOverlapPairs);

	for(INT i=inst->KPhysRootIndex; i <= inst->KPhysLastIndex-1; i++)
	{
		McdModelID model1 = inst->KSkelModels(i);
		if(model1 && MODELGEOM_NOT_NULL(model1))
		{
			for(INT j=i+1; j <= inst->KPhysLastIndex; j++)
			{
				McdModelID model2 = inst->KSkelModels(j);
				if(model2 && MODELGEOM_NOT_NULL(model2))
				{
					// If we dont already have this pair, and it isn't disabled, and the boxes overlap.
					// Note for intra-skeleton model pairs we use the skel instance KSkelDisableTable, 
					// not the level KDisableTable.
					QWORD Key = KModelsToKey(model1, model2);
					if( !inst->KSkelDisableTable.Find(Key) && CheckModelOverlap(model1, model2) )
					{
						PairM1.AddItem(model1);
						PairM2.AddItem(model2);
					}
				}
			}
		}
	}

	unguard;
}

// Given the two 'things' (either models of skeletal instances), generate all pairs of models
// whose bounding boxes currently overlap.
static void GenerateOverlapPairs(McdModelID model1, USkeletalMeshInstance* inst1, 
								 McdModelID model2, USkeletalMeshInstance* inst2,
								 TArray<McdModelID>& PairM1, TArray<McdModelID>& PairM2, 
								 UBOOL checkOverlap, TMap<QWORD, UBOOL>* DisableTable )
{
	guard(GenerateOverlapPairs);

	// If we have model-skel, make sure the model is the first one.
	if(inst1 && model2)
	{
		McdModelID tmpM = model1;
		USkeletalMeshInstance* tmpI = inst1;

		model1 = model2; inst1 = inst2;
		model2 = tmpM; inst2 = tmpI;
	}

	if(model1 && MODELGEOM_NOT_NULL(model1) && model2 && MODELGEOM_NOT_NULL(model2)) // MODEL-MODEL
	{
		// If we dont already have this pair, and this pair isn't disabled.
		// Dont do box check, because the ActorOverlapCheck has already done that.
		QWORD Key = KModelsToKey(model1, model2);
		if(!DisableTable->Find(Key))
		{
			PairM1.AddItem(model1);
			PairM2.AddItem(model2);
		}
	}
	else if(model1 && MODELGEOM_NOT_NULL(model1) && inst2) // MODEL-SKEL
	{
		for(INT j=inst2->KPhysRootIndex; j <= inst2->KPhysLastIndex; j++)
		{
			McdModelID skelModel2 = inst2->KSkelModels(j);
			if(skelModel2 && MODELGEOM_NOT_NULL(skelModel2))
			{
				// If we dont already have this pair, and this pair isn't disabled, and the boxes overlap (or we're not checking)
				QWORD Key = KModelsToKey(model1, skelModel2);
				if( !DisableTable->Find(Key) && (!checkOverlap || CheckModelOverlap(model1, skelModel2)) )
				{
					PairM1.AddItem(model1);
					PairM2.AddItem(skelModel2);
				}
			}
		}
	}
	else if(inst1 && inst2) // SKEL-SKEL
	{
		for(INT j=inst1->KPhysRootIndex; j <= inst1->KPhysLastIndex; j++)
		{
			McdModelID skelModel1 = inst1->KSkelModels(j);
			if(skelModel1 && MODELGEOM_NOT_NULL(skelModel1))
			{
				for(INT k=inst2->KPhysRootIndex; k <= inst2->KPhysLastIndex; k++)
				{
					McdModelID skelModel2 = inst2->KSkelModels(k);
					if(skelModel2 && MODELGEOM_NOT_NULL(skelModel2))
					{
						// If we dont already have this pair, and the boxes overlap (or we're not checking)
						QWORD Key = KModelsToKey(skelModel1, skelModel2);
						if( !DisableTable->Find(Key) && (!checkOverlap || CheckModelOverlap(skelModel1, skelModel2)) )
						{
							PairM1.AddItem(skelModel1);
							PairM2.AddItem(skelModel2);
						}
					}
				}
			}
		}
	}

	unguard;
}

static void UpdateModelPairs(TArray<McdModelID>& PairM1, TArray<McdModelID>& PairM2, 
							 TMap<QWORD, McdModelPairID>* OverlapPairs, TMap<QWORD, UBOOL>* ActivePairs,
							 McdModelPairContainer* pairContainer, ULevel* level)
{
	guard(UpdateModelPairs);

	for(INT i=0; i<PairM1.Num(); i++)
	{
		check(PairM1(i) && PairM2(i));

		// We dont want to remove (goodbye) this pair, so remove it from models goodbye-pending lists.
		// Note - one of these arrays will be empty! But its tricky to keep track of which one...
		KarmaModelUserData* d1 = (KarmaModelUserData*)McdModelGetUserData(PairM1(i));
		d1->GoodbyeModels.RemoveItem(PairM2(i));

		KarmaModelUserData* d2 = (KarmaModelUserData*)McdModelGetUserData(PairM2(i));
		d2->GoodbyeModels.RemoveItem(PairM1(i));


		// If this pair has already been found this frame, don't create it again.
		QWORD Key = KModelsToKey(PairM1(i), PairM2(i));
		if(ActivePairs->Find(Key))
			continue;

		// Look up this pair of models in the map of all model pairs (active or not)
		McdModelPairID pair = 0;
		McdModelPairID* pairPtr = OverlapPairs->Find(Key);

		// If there was no pair already, create a new model pair and hello it.
		if(!pairPtr)
		{
			pair = McdModelPairCreate(PairM1(i), PairM2(i));
			KHelloModelPair(pair, level);
		}
		else
		{
			pair = *pairPtr; // Otherwise, use existing pair.
		}

		// Want to generate new contacts for this pair.
		McdModelPairContainerInsertStayingPair(pairContainer, pair);

		// Make sure we dont generate this pair again!
		ActivePairs->Set(Key, 0);
	}

	unguard;
}

// Initially, all pairs of this model are candidates for goodbyeing.
// Every time we find a pair, we remove it from the pending-goodbye list.
void static SetupModelGoodbyeList(McdModelID model, USkeletalMeshInstance* inst)
{
	check(model || inst);
	check(!(model && inst));

	if(model)
	{
		KarmaModelUserData* d = (KarmaModelUserData*)McdModelGetUserData(model);
		d->GoodbyeModels.Empty();
		d->GoodbyeModels = d->OverlapModels;		
	}
	else
	{
		for(INT i=inst->KPhysRootIndex; i <= inst->KPhysLastIndex; i++)
		{
			McdModelID skelModel = inst->KSkelModels(i);
			if(skelModel)
				SetupModelGoodbyeList(skelModel, 0);
		}
	}
}

// This is called as the last thing for a contact generating model.
// It removes any pairs that are no longer pertinant.
void static ProcessModelGoodbyeList(McdModelID model, USkeletalMeshInstance* inst, ULevel* level)
{
	check(model || inst);
	check(!(model && inst));

	if(model)
	{
		KarmaModelUserData* d = (KarmaModelUserData*)McdModelGetUserData(model);
		for(INT i=0; i<d->GoodbyeModels.Num(); i++)
		{
			KGoodbyePair(model, d->GoodbyeModels(i), level);
		}
	}
	else
	{
		for(INT i=inst->KPhysRootIndex; i <= inst->KPhysLastIndex; i++)
		{
			McdModelID skelModel = inst->KSkelModels(i);
			if(skelModel)
				ProcessModelGoodbyeList(skelModel, 0, level);
		}
	}
}

///////////////////////////////////////// KUPDATECONTACTS //////////////////////////////////////////////////////////////////

// This is the main function which will update/add/remove contacts from the Karma simulation.
// It first figures out all hello(new), staying and goodbye (just finished) McdModel pairs.
// Then calls the nearfield contact-generation tests, and updates the MdtWorld accordingly.
// The bool indicates if we are generating contacts for fixed or variable rate actors.
void KUpdateContacts(TArray<AActor*> &actors, ULevel* level, UBOOL bDoubleRateActors)
{
	guard(KUpdateContacts);

	if(!level->Hash)
		return;

	FMemMark Mark(GMem);

	// Keep a lookup of pairs we already have, to prevent adding them twice.
	TMap<QWORD, UBOOL> ActivePairs;

	// Empty the model pair container
	McdModelPairContainerReset(KGData->filterPairs);

	// I am assuming that I dont need to update the models. That should always be done before things are upt back 
	// into the Hash (including for each part of a skeletal thing).

	// We iterate over the actors in the world. We will only query for an actor if it is physical,
	// and if it has bBlockKarma turned on. This will automatically ignore world-world pairs etc,
	// and karma things with no collision.
	for(INT i=0; i<actors.Num(); i++)
	{
		AActor* actor = actors(i);
		check(actor);
		check(actor->KParams);

		// Only update contacts for things we are stepping.
		UKarmaParams* kparams = Cast<UKarmaParams>(actor->KParams);
		check(kparams);
		if( (kparams->bKDoubleTickRate && !bDoubleRateActors) || (!kparams->bKDoubleTickRate && bDoubleRateActors) )
			continue;

		// Reset 'touching level' flag. Set again in KPerContactCB if we are.
		kparams->bContactingLevel = 0;

		// Do any other pre-contact gen stuff (reset flags set inside contact callback etc.)
		actor->preContactUpdate();

		//debugf( TEXT("ContactGen: %s"), actor->GetName() );

		check(!actor->bDeleteMe);
		check(actor->Physics == PHYS_Karma || actor->Physics == PHYS_KarmaRagDoll);
		check(actor->bBlockKarma);
		check(actor->bCollideActors);

		USkeletalMeshInstance* inst1 = NULL;
		McdModelID model1 = NULL;

		// Get the collision info for this actor (model or skel instance)
		GetModels(actor, model1, inst1);
		if(!model1 && !inst1)
			continue;
		check(!(model1 && inst1)); // Check we dont have a model AND a skel instance!


		// This is the model which has moved, so we might want to goodbye some pairs its involved with.
		// We assume we want to goodbye all the pairs. Then, for each pair we find we remove it from the
		// 'goodbye pending' list. Then we goodbye any remaining pairs before moving on to the next
		// moving model.
		SetupModelGoodbyeList(model1, inst1);

		// Find the actors (with bBlockKarma) which overlap this actors bounding box.
		FBox box = actor->GetPrimitive()->GetCollisionBoundingBox(actor);
		FCheckResult* first = level->Hash->ActorOverlapCheck(GMem, actor, &box, 1);

		FCheckResult* overlap;
		TArray<McdModelID> PairM1, PairM2;
		for( overlap = first; overlap!=NULL; overlap=overlap->GetNext() )
		{
			AActor* overActor = overlap->Actor;
			check(overActor);
			check(overActor->bBlockKarma);
			check(overActor->KParams);
			check(!overActor->bDeleteMe);

			//debugf(TEXT("Consider: %s - %s"), actor->GetName(), overActor->GetName() );

			// Check if this pair of actors should happen at all.
			if( actor == overActor || !ActorsShouldCollide(actor, overActor) )
				continue;

			USkeletalMeshInstance* inst2 = NULL;
			McdModelID model2 = NULL;

			GetModels(overActor, model2, inst2);
			if(!model2 && !inst2)
				continue;
			check(!(model2 && inst2));

			// Now generate the resulting set of overlapping models, using AABB test to reject further.
			//debugf(TEXT("GeneratePairs: %s - %s"), actor->GetName(), overActor->GetName() );
			GenerateOverlapPairs(model1, inst1, model2, inst2, PairM1, PairM2, 1, &level->KDisableTable);
			check(PairM1.Num() == PairM2.Num());
			
		} // FOR each overlapping actors

		// We also need to make sure there is a pair between this actor and the level model,
		// for BSP/Terrain collision via TriList. No need to bother with the overlap test.
		//debugf(TEXT("GeneratePairs (world): %s - WORLD"), actor->GetName() );
		GenerateOverlapPairs(model1, inst1, level->KLevelModel, 0, PairM1, PairM2, 0, &level->KDisableTable);

		// If this is a skeletal actor, we need to generate pairs of models between its parts.
		if(inst1)
			GenerateSelfOverlapPairs(inst1, PairM1, PairM2);

		check(PairM1.Num() == PairM2.Num());

		// Update model pair container from the set pairs we have found.
		UpdateModelPairs(PairM1, PairM2, &level->OverlapPairs, &ActivePairs, KGData->filterPairs, level);

		// Any pairs still left in the goodbye lists are dealt with now - they are no longer needed.
		ProcessModelGoodbyeList(model1, inst1, level);

	} // FOR each dynamics, bBlockKarma actor in world.

	Mark.Pop();


	clock(GStats.DWORDStats(GEngineStats.STATS_Karma_CollisionContactGen));
	// Only bother doing contact generation if we have some pairs!
	if(McdModelPairContainerGetHelloCount(KGData->filterPairs) + McdModelPairContainerGetStayingCount(KGData->filterPairs) > 0)
		KHandleCollisions(KGData->filterPairs, level);
	unclock(GStats.DWORDStats(GEngineStats.STATS_Karma_CollisionContactGen));
	
	unguard;
}


/////////////////////////////////////// HELLO/GOODBYE //////////////////////////////////////////////////////

void KHelloModelPair(McdModelPairID pair, ULevel* level)
{
	guard(KHelloModelPair);

#if DISPLAY_HELLO
	AActor* a1 = KModelGetActor(pair->model1);
	AActor* a2 = KModelGetActor(pair->model2);
	debugf(TEXT("HELLO: %s (%x) - %s (%x)"), a1?a1->GetName():TEXT("None"), pair->model1, a2?a2->GetName():TEXT("None"), pair->model2);
#endif

	MeI32 key1 = McdModelGetSortKey(pair->model1);
	MeI32 key2 = McdModelGetSortKey(pair->model2);

	McdGeometryType g1 = McdGeometryGetTypeId(McdModelGetGeometry(pair->model1));
	McdGeometryType g2 = McdGeometryGetTypeId(McdModelGetGeometry(pair->model2));

	/* if the geometries are the same, rearrange them before the hello
	so that the one with the lower sort key is first. */

	if(g1==g2 && key2<key1)
	{
		McdModelID tmp = pair->model1;
		pair->model1 = pair->model2;
		pair->model2 = tmp;
	}

	McdHello(pair);

	// Add each model to the others list of overlapping models.
	KarmaModelUserData* d1 = (KarmaModelUserData*)McdModelGetUserData(pair->model1);
	check(d1->OverlapModels.FindItemIndex(pair->model2) == INDEX_NONE); // Check model is not already in array
	d1->OverlapModels.AddItem(pair->model2);

	KarmaModelUserData* d2 = (KarmaModelUserData*)McdModelGetUserData(pair->model2);
	check(d2->OverlapModels.FindItemIndex(pair->model1) == INDEX_NONE);
	d2->OverlapModels.AddItem(pair->model1);

	// Add this pair to the store of model pairs.
	QWORD Key = KModelsToKey(pair->model1, pair->model2);
	level->OverlapPairs.Set(Key, pair);

	unguard;
}


void KGoodbyeModelPair(McdModelPairID pair, ULevel* level)
{
	guard(KGoodbyeModelPair);

#if DISPLAY_GOODBYE
	AActor* a1 = KModelGetActor(pair->model1);
	AActor* a2 = KModelGetActor(pair->model2);
	debugf(TEXT("GOODBYE: %s (%x) - %s (%x)"), a1?a1->GetName():TEXT("None"), pair->model1, a2?a2->GetName():TEXT("None"), pair->model2);
#endif

	// Make sure pair is removed from set of overlapping pairs.
	QWORD Key = KModelsToKey(pair->model1, pair->model2);
	level->OverlapPairs.Remove(Key);

	// Clear up any dyanmic contacts going on.
	MdtContactGroupID g = (MdtContactGroupID)pair->responseData;
	if(g)
	{
		MdtContactGroupReset(g);
		MdtContactGroupDestroy(g);
		pair->responseData = 0;
	}

	// Do any Karma cleanup on model pair.
	McdGoodbye(pair);

	// Remove reference to each model in the others overlapping model array.
	KarmaModelUserData* d1 = (KarmaModelUserData*)McdModelGetUserData(pair->model1);
	check(d1->OverlapModels.FindItemIndex(pair->model2) != INDEX_NONE);
	d1->OverlapModels.RemoveItem(pair->model2);

	KarmaModelUserData* d2 = (KarmaModelUserData*)McdModelGetUserData(pair->model2);
	check(d2->OverlapModels.FindItemIndex(pair->model1) != INDEX_NONE);
	d2->OverlapModels.RemoveItem(pair->model1);

	// Finally, free the pair itself.
	McdModelPairDestroy(pair);

	unguard;
}


// Remove any pair between these two models.
// Also removes any reference to each other in each models list of overlapping models.
void KGoodbyePair(McdModelID model1, McdModelID model2, ULevel* level)
{
	guard(KGoodbyePair);

	QWORD Key = KModelsToKey(model1, model2);
	McdModelPairID* pair = level->OverlapPairs.Find(Key);

	// There is no pair between these models, do nothing
	if(!pair)
	{
#if 1
		// Check neither model thinks its overlapping the other.
		KarmaModelUserData* d1 = (KarmaModelUserData*)McdModelGetUserData(model1);
		check(d1->OverlapModels.FindItemIndex(model2) == INDEX_NONE);

		KarmaModelUserData* d2 = (KarmaModelUserData*)McdModelGetUserData(model2);		
		check(d2->OverlapModels.FindItemIndex(model1) == INDEX_NONE);
#endif
		return;
	}

	KGoodbyeModelPair(*pair, level);

	unguard;
}

// Call this when you are turning off collision or destroying a model. 
// It will find, goodbye and destroy any related McdModelPairs.
void KGoodbyeAffectedPairs(McdModelID model, ULevel* level)
{
	guard(KGoodbyeAffectedPairs);

	KarmaModelUserData* data = (KarmaModelUserData*)McdModelGetUserData(model);

	while( data->OverlapModels.Num() > 0)
	{
		McdModelID model2 = data->OverlapModels(0);
		KGoodbyePair(model, model2, level);
	}

	unguard;
}

// Util for feeding above. Gets any models out of the supplied actor and goodbye any affected pairs.
void KGoodbyeActorAffectedPairs(AActor* actor)
{
	McdModelID model;
	USkeletalMeshInstance* inst;

	GetModels(actor, model, inst);
	check( !(model && inst) );

	if(inst)
	{
		if(!inst->KSkelIsInitialised)
			return;

		for(INT i=0; i<inst->KSkelModels.Num(); i++)
		{
			McdModelID model = inst->KSkelModels(i);
			if(!model)
				continue;

			KGoodbyeAffectedPairs(model, actor->GetLevel() );
		}
	}
	else if(model)
		KGoodbyeAffectedPairs(model, actor->GetLevel() );
}

/////////////////////////////////////// PAIRWISE ENABLE/DISABLE /////////////////////////////////////////////////////////////

// Re-enable collision between these two models.
// (Collision is on by default)
void KEnablePairCollision(McdModelID model1, McdModelID model2, ULevel* level)
{
	guard(KEnablePairCollision);

	// Make key, and remove from the table. Will do nothing if its not there (ie. collision is already enabled).
	QWORD Key = KModelsToKey(model1, model2);
	level->KDisableTable.Remove(Key);

	unguard;
}

// Disable collision between these models.
// This also cleans up and modelpair/contact group already between these models.
void KDisablePairCollision(McdModelID m1, McdModelID m2, ULevel* level)
{
	guard(KEnablePairCollision);

	QWORD Key = KModelsToKey(m1, m2);

	// First see if we are already in the table. If so, do nothing.
	UBOOL* b = level->KDisableTable.Find(Key);
	if(b)
		return;

	// Add key to table.
	level->KDisableTable.Set(Key, 0);

	// We also want to remove any model pair between these two models.
	KGoodbyePair(m1, m2, level);

	unguard;
}

/* ******************** CUSTOM BRIDGE HANDLERS ***************************** */

static void ConvertCollisionContact(MdtBclContactParams* params, McdContact* colC, MdtContactID dynC, int swap)
{
	guardSlow(ConvertCollisionContact);

    if(swap)
        MdtContactSetNormal(dynC, -colC->normal[0], -colC->normal[1], -colC->normal[2]);
    else
        MdtContactSetNormal(dynC, colC->normal[0], colC->normal[1], colC->normal[2]);

    MdtContactSetPosition(dynC, colC->position[0], colC->position[1], colC->position[2]);
    MdtContactSetPenetration(dynC,-(colC->separation));
    MdtContactSetParams(dynC, params);

	unguardSlow;
}

static MdtContactGroupID CreateContactGroup(MdtWorldID w, McdModelPairID pair)
{
	guard(CreateContactGroup);

    MeI32 key1 = McdModelGetSortKey(pair->model1);
    MeI32 key2 = McdModelGetSortKey(pair->model2);
    MdtBodyID body1 = McdModelGetBody(pair->model1);
    MdtBodyID body2 = McdModelGetBody(pair->model2);
    
    MdtContactGroupID g = 0;

    if(body1 || body2)
    {
        if(body1==body2)
        {
            MeFatalError(0, "Attempt to create contact group between two collision models with "
            "the same dynamics body. You should disable collisions between such models in the"
            "McdSpace.");
            return 0;
        }

        g = MdtContactGroupCreate(w);
        if (g == 0)
        {
            MeFatalError(0, "Constraint pool too small during Mcd->Mdt ContactGroup creation.");
            return 0;
        }
        
        /* If one of the contacts is with the world, make world the second body. */
        if(body1) 
        {
            MdtContactGroupSetBodies(g, body1, body2);
            MdtContactGroupSetSortKey(g, -((key1<<15)+key2));
            g->swapped = 0;
        }
        else
        {
            MdtContactGroupSetBodies(g, body2, body1);
            MdtContactGroupSetSortKey(g, -((key2<<15)+key1));
            g->swapped = 1;
        }
        MdtContactGroupSetGenerator(g,pair);
        pair->responseData = g;
    }
    return g;

	unguard;
}

void KHandleCollisions(McdModelPairContainer* pairs, ULevel* level)
{
	guard(KHandleCollisions);

    int i, j;

	MdtWorldID w = level->KWorld;
	MstBridgeID b = level->KBridge;

    McdIntersectResult *results = (McdIntersectResult*)MeMemoryALLOCA(pairs->size*sizeof(McdIntersectResult));
    McdContact *contacts = (McdContact*)MeMemoryALLOCA(1000*sizeof(McdContact)); 
    int contactCount, resultCount;
    MeBool arrayFinished;

    if(pairs->helloFirst==pairs->stayingEnd)
        return;

    McdBatchContextReset(b->context);
    arrayFinished = 0;
    while(!arrayFinished)
    {
        arrayFinished = KBatchIntersectEach(b->context, pairs, 
                                              results, &resultCount, pairs->size, 
                                              contacts, &contactCount, 1000);

        /* hello and staying - generate contacts and pass to dynamics */
        for( i = 0; i<resultCount ; i++ )
        {
            MdtContactID dynC;
            McdContact *colC;
            MdtContactGroupID group;
            McdIntersectResult *result = results+i;
            McdModelPairID pair = result->pair;
            
            MdtContactParamsID params;
            MstPerContactCBPtr contactCB;
            MstPerPairCBPtr pairCB;
            
            McdModelID m1 = pair->model1,
                       m2 = pair->model2;
            
            MstMaterialID material1 = McdModelGetMaterial(m1),
                          material2 = McdModelGetMaterial(m2);


            if(result->touch)
	        {
                MstIntersectCBPtr intersectCB = MstBridgeGetIntersectCB(b,
                    material1, material2);

                if(m1->mIntersectFn)
                    (*m1->mIntersectFn)(m1,result);

                if(m2->mIntersectFn)
                    (*m2->mIntersectFn)(m2,result);
                
                if(intersectCB)
                    (*intersectCB)(result);
            }
            
            /* For each contact in result: copy geo data into existing MdtContact.
            If new MdtContacts needed, get, point to params, then copy geo data. */
            colC = result->contacts;
            
            group = (MdtContactGroupID)pair->responseData;
            if(!group)
            {
                // no contact group, either because there was no space to create it
                // at some point, or because this is the first time these objects
                // have actually intersected.
                group = CreateContactGroup(w,pair);
            }

            if(group)
            {
				guard(CopyContacts);

                dynC = MdtContactGroupGetFirstContact(group);
                params = MstBridgeGetContactParams(b, material1, material2);
                contactCB = MstBridgeGetPerContactCB(b, material1, material2);
                
                /* First try to use contacts already in list */
                
                for( j = 0; j < result->contactCount; j++)
                {
                    if(dynC == MdtContactInvalidID)
                    {
                        dynC = MdtContactGroupCreateContact(group); /* auto-assigns bodies */
                    }
                    
                    if (!dynC || dynC == MdtContactInvalidID)
                    {
						debugf(TEXT("KHandleCollisions: Could not create contact"));
                    }
                    else 
                    {
                        ConvertCollisionContact(params, colC+j, dynC, MdtContactGroupIsSwapped(group));
                        
                        /* If there's no contact callback, or the contact callback says keep the contact,
                        advance to next contact. Otherwise we re-use it. */
                        
                        if(!contactCB || (*contactCB)(result, colC+j, dynC))
                            dynC = MdtContactGroupGetNextContact(group,dynC);
                    }
                }
                
                /* nuke all the remaining contacts in this contact group */
                
                while(dynC != MdtContactInvalidID)
                {
                    MdtContactID nextC = MdtContactGroupGetNextContact(group,dynC);
                    MdtContactGroupDestroyContact(group,dynC);
                    dynC = nextC;
                }
                
				guard(Callbacks);
                /* Call Per Pair Callback (if there is one) */
                pairCB = MstBridgeGetPerPairCB(b, material1, material2);
                
                if(pairCB && !((*pairCB)(result,group)))
                {
                    MdtContactID c = MdtContactGroupGetFirstContact(group);
                    while (c != MdtContactInvalidID)
                    {
                        MdtContactID nextC = MdtContactGroupGetNextContact(group,c);
                        MdtContactGroupDestroyContact(group,c);
                        c = nextC;
                    }
                }
                
                if(group->count > 0)
                { 
                    if(!MdtContactGroupIsEnabled(group))
                        MdtContactGroupEnable(group);
                }
                else
                {
                    if(MdtContactGroupIsEnabled(group))
                        MdtContactGroupDisable(group);
                }
				unguard; // CALLBACKS

				unguard; // COPY CONTACTS
            }                
        }
    }

	unguard;
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MCD BATCH /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

// This is all taken from McdBatch.cpp (inside Mcd/Frame). The only reason it is copied here
// is because it checks if the models are frozen, which requires them being in a Space. Hence 
// the #if 0 lines below.

// These are in McdFrame, but not in header! DAMMIT!

extern MeBool McdBatchUnflatten(McdBatchContext *context,
                              McdIntersectResult *resultArray, int * resultCount, int resultMaxCount,
                              McdContact *contactArray, int *contactCount, int contactMaxCount);

extern MeBool McdBatchFlattenAggregate(McdBatchContext *context,
                                     int flags,
                                     McdGeometryInstanceID ins1, McdGeometryInstanceID ins2, 
                                     MeReal eps1, MeReal eps2);

extern MeBool McdBatchIntersectBucket(McdBatchContext *context, int type1, int type2);


// // //

#define AGGREGATE_ARRAY_COUNT 16

typedef enum 
{
    kMcdBatchIsFlipped = 1,
    kMcdBatchIns1IsAggregated = 2,
    kMcdBatchIns2IsAggregated = 4
} McdBatchEntryAggregateFlags;


static inline void KBatchBuildGeometryData(McdBatchGeometryData *data,
                                      int type, McdGeometryInstance *ins, MeReal epsilon)
{
    data->type = type;
    data->eps = epsilon;
    data->min = ins->min;
    data->max = ins->max;
    data->geometry=ins->mGeometry;
    data->tm=ins->mTM;
}

static MeBool KBatchIntersectBucket(McdBatchContext *context, int type1, int type2)
{
	guard(KBatchIntersectBucket);

    McdBatchEntry *entry;
    McdModelPair p;
    McdIntersectResult r;
    McdModel m1,m2;
    int index = type1*context->typeCount+type2;
    int ct = 0;

    McdIntersectFn fn = McdFrameworkGetInteractions(context->frame, type1, type2)->intersectFn;

    check(context->table[index] != 0);

    if (fn == 0
        && !McdFrameworkGetInteractionsWarned(context->frame,type1,type2))
    {
        MeWarning(1,"No test registered between %s and %s",
            McdFrameworkGetTypeName(context->frame,type1),
            McdFrameworkGetTypeName(context->frame,type2)
        );

        McdFrameworkSetInteractionsWarned(context->frame,type1,type2,1);
    }

	entry = context->table[index];
    while (entry)
    {
        McdBatchContactPool *pool = context->pools + entry->pool;
        McdBatchPairData *pd = entry->pairData;

        /* we don't stall on aggregates, just on singletons */

        if (entry->pool == 0 && pool->contactMaxCount < pool->contactCount+401 )
        {
            //McdProfileEnd("McdBatchIntersectBucket");
            return 0;
        }

        /* make sure everything we'll need is 16-byte aligned */
        m1.mInstance.mTM = entry->geometryData1.tm;
        m1.mInstance.mGeometry = entry->geometryData1.geometry;
        m1.mPadding = entry->geometryData1.eps;
		m1.mData = pd->pair->model1->mData;

        m2.mInstance.mTM = entry->geometryData2.tm;
        m2.mInstance.mGeometry = entry->geometryData2.geometry;
        m2.mPadding = entry->geometryData2.eps;
		m2.mData = pd->pair->model2->mData;

        m1.frame = m2.frame = context->frame;

        MEASSERTALIGNED(m1.mInstance.mTM,MeALIGNTO);
        MEASSERTALIGNED(m2.mInstance.mTM,MeALIGNTO);
        MEASSERTALIGNED(m1.mInstance.mGeometry,MeALIGNTO);
        MEASSERTALIGNED(m2.mInstance.mGeometry,MeALIGNTO);

        p.model1 = &m1;
        p.model2 = &m2;
        p.request = pd->pair->request;
        p.userData = pd->pair->userData;
        p.m_cachedData = pd->status & kMcdBatchPairAggregate
            ? 0 : pd->pair->m_cachedData;

        r.contacts = pool->contacts + pool->contactCount;
        r.contactMaxCount = pool->contactMaxCount - pool->contactCount;
        r.pair = &p;

        MEASSERT(MeMatrix4IsTM(m1.mInstance.mTM,(MeReal)0.001));
        MEASSERT(MeMatrix4IsTM(m2.mInstance.mTM,(MeReal)0.001));

        MEASSERT(McdGeometryGetType(m1.mInstance.mGeometry)==type1);
        MEASSERT(McdGeometryGetType(m2.mInstance.mGeometry)==type2);

        /* If there is a function here (and at least 400 contacts left in pool), call it. */
        if (fn != 0 && (pool->contactMaxCount - pool->contactCount > 400) )
        {
            //McdProfileStart("McdBatchIntersectBucket-fn");
            (*fn)(&p, &r);
            //McdProfileEnd("McdBatchIntersectBucket-fn");
        }
        else
        {
            r.touch = 0;
            r.contactCount = 0;
        }

        pd->pair->m_cachedData = p.m_cachedData;
        entry->contacts = pool->contacts + pool->contactCount;
        entry->touch = r.touch;
        entry->contactCount = 0;
        
        if(++pd->done == pd->entries)
            pd->status = (pd->status & kMcdBatchPairAggregate)
                | kMcdBatchPairTested;

        if(entry->touch)
        {
            entry->contactCount = r.contactCount;
            pool->contactCount += r.contactCount;
            MeVector3Copy(entry->normal,r.normal);
        }

        context->table[index] = entry->next;
        ct++;

		entry = context->table[index];
    }

    return 1;

	unguardf(( TEXT("(Types %d %d)"), type1, type2 ));;
}

static MeBool KBatchTest(McdBatchContext *context)
{
	guard(KBatchTest);

    unsigned type1, type2;

    context->pools[0].contactCount = 0;

    for (type1 = 0; type1 < (unsigned)context->typeCount; type1++)
    {
        for (type2 = 0; type2 < (unsigned)context->typeCount; type2++)
        {
            const unsigned index = type1*context->typeCount+type2;

            if (context->table[index])
			{
				guard(McdBatchIntersectBucket);
                if (KBatchIntersectBucket(context, type1, type2) == 0)
                    return 0;
				unguard;
			}
        }
    }
    return 1;

	unguard;
}

static MeBool KBatchFlatten(McdBatchContext *context, McdModelPairContainer *pairs)
{
	guard(KBatchFlatten);

    int i;

    context->nextEntry = 0;
    context->nextTM = 0;
    context->nextPool = 1;
    context->nextPairData = 0;

    for(i=0;i<context->typeCount * context->typeCount;i++) 
        context->table[i]=0;
    
    for(i=0;i<=AGGREGATE_ARRAY_COUNT;i++)
        context->pools[i].contactCount = 0;                
                                
    for(context->nextFlattenPair;
        context->nextFlattenPair < pairs->stayingEnd;
        context->nextFlattenPair++,context->nextPairData++)
    {
        if(context->nextPairData>=context->pairDataMaxCount)
        {
            return 0;
        }

        McdBatchPairData *pd = context->pairData + context->nextPairData;
        pd->pair = pairs->array[context->nextFlattenPair];

        McdModelID model1 = pd->pair->model1;
        McdModelID model2 = pd->pair->model2;
        int type1, type2;
        McdGeometryInstanceID ins1,ins2;

#if 0
        /* Don't go on if both models are frozen. */
        if(McdSpaceModelIsFrozen(model1) && McdSpaceModelIsFrozen(model2))
        {
            pd->status = kMcdBatchPairFrozen;
            continue;
        }
#endif

        ins1 = McdModelGetGeometryInstance(model1);
        ins2 = McdModelGetGeometryInstance(model2);
        type1 = McdGeometryGetType(ins1->mGeometry);
        type2 = McdGeometryGetType(ins2->mGeometry);

        MEASSERT(MeMatrix4IsTM(ins1->mTM,(MeReal)0.001));
        MEASSERT(MeMatrix4IsTM(ins2->mTM,(MeReal)0.001));        

        if(type1!=kMcdGeometryTypeAggregate)
        {
            McdBatchEntry *entry;
            int index = type1 * context->typeCount + type2;

            MEASSERT(type2!=kMcdGeometryTypeAggregate);

            if(context->nextEntry >= context->maxEntryCount)
            {
                return 0;
            }

            entry = context->entryArray+context->nextEntry++;

            KBatchBuildGeometryData(&entry->geometryData1,type1,ins1,
                McdModelGetContactTolerance(model1));
            KBatchBuildGeometryData(&entry->geometryData2,type2,ins2,
                McdModelGetContactTolerance(model2));

            entry->ins1 = ins1;
            entry->ins2 = ins2;

            entry->pool = 0;
            entry->pairData = pd;
            entry->flags = 0;
            
            entry->next = context->table[index];
            context->table[index] = entry;

            pd->start = entry;
            pd->entries = 1;
            pd->done = 0;
            pd->status = kMcdBatchPairFlattened;
        }    
        else
        {
            int j, pool;
            MeReal eps1 = McdModelGetContactTolerance(model1);
            MeReal eps2 = McdModelGetContactTolerance(model2);
            int saveNextEntry = context->nextEntry;
            int saveNextFlattenPair = context->nextFlattenPair;
            int saveNextTM = context->nextTM;

            pd->start = context->entryArray + context->nextEntry;
 
            if(!McdBatchFlattenAggregate(context,0,ins1, ins2, eps1, eps2) ||
                context->nextEntry > saveNextEntry + 1 && 
                context->nextPool >= context->poolMaxCount)
            {
                /* unlink any aggregate entries from the batching table and rewind the state */
                for(j=context->nextEntry-1;j>=saveNextEntry;j--)
                {
                    McdBatchEntry *e = context->entryArray+j;
                    MEASSERT(context->table[e->geometryData1.type
                                 * context->typeCount + e->geometryData2.type]==e);
                    context->table[e->geometryData1.type
                        * context->typeCount + e->geometryData2.type] = e->next;
                }

                context->nextEntry = saveNextEntry;

                for(j=saveNextTM;j<context->nextTM;j++)
                    *context->tmTrack[j]=0;

                context->nextTM = saveNextTM;
                context->nextFlattenPair = saveNextFlattenPair;

                return 0;
            }
 
            pd->entries = context->nextEntry - saveNextEntry;
            pd->done = 0;
            pd->status = kMcdBatchPairAggregate | (pd->entries ? kMcdBatchPairFlattened
                                                : kMcdBatchPairTested);
            pool = pd->entries > 1 ? context->nextPool++ : 0;

            for(j=0;j<pd->entries;j++)
            {
                pd->start[j].pairData = pd;
                pd->start[j].pool = pool;
            }
        }    
    }

    return 1;
	unguard;
}

typedef enum
{
    kMcdBatchFlattenPending = 1,
    kMcdBatchTestsPending = 2,
    kMcdBatchUnflattenPending = 4,
} McdBatchStateFlags;

MeBool KBatchIntersectEach(McdBatchContext *context,
                      McdModelPairContainer *pairs,
                      McdIntersectResult* resultArray, 
                      int *resultCount,
                      int resultMaxCount,
                      McdContact* contactArray,
                      int *contactCount,
                      int contactMaxCount)
{
	guard(KBatchIntersectEach);

    int i;

    *resultCount = 0;
    *contactCount = 0;

    if(!(context->state & kMcdBatchUnflattenPending))
    {
        if(!(context->state & (kMcdBatchTestsPending|kMcdBatchUnflattenPending)))
        {
            if(!context->state)
                context->nextFlattenPair = pairs->helloFirst;
        
            if(KBatchFlatten(context,pairs))
                context->state &= ~kMcdBatchFlattenPending;
            else
                context->state |= kMcdBatchFlattenPending;
        }

        if(KBatchTest(context))
            context->state &= ~kMcdBatchTestsPending;
        else
            context->state |= kMcdBatchTestsPending;

        context->nextSingleUnflattenPair = 0;
        context->nextAggregateUnflattenPair = 0;
    }
    
    if(McdBatchUnflatten(context,resultArray,resultCount,resultMaxCount,
        contactArray,contactCount,contactMaxCount))
        context->state &= ~kMcdBatchUnflattenPending;
    else
        context->state |= kMcdBatchUnflattenPending;


    if(!(context->state & (kMcdBatchTestsPending|kMcdBatchUnflattenPending)))
    {
        for(i=0;i<context->nextTM;i++)
                *(context->tmTrack[i])=0;
        context->nextTM = 0;

        if(!context->state)
        {
            for(i=0;i<context->nextPairData;i++)
            {
                MEASSERT(context->pairData[i].status == kMcdBatchPairFrozen ||
                    context->pairData[i].status == kMcdBatchPairUnflattened ||
                    context->pairData[i].status == (kMcdBatchPairUnflattened|kMcdBatchPairAggregate)
                    );
            }
            
            return 1;
        }
    }

    return 0;

	unguard;
}


#endif

