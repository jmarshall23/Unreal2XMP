
void ParticleTick( FLOAT DeltaSeconds, FVector& Location, FRotator& Rotation, FLOAT AdditionalStasisTime=0.f );

FLOAT GetVolume()
{
    UClient* C=GetLevel()->Engine->Client;
    return C ? appBlend( MinVolume, Volume, (FLOAT)(C->ParticleDensity) / 255.f ) * VolumeScale : Volume;
}

