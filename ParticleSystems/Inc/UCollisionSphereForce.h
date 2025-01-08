
void Apply( AParticleGenerator* System, FLOAT DeltaTime );

// =Input=
// Origin: Location of sphere in three-space.
// Radius: Radius of sphere.
// P1, P2: Endpoints of ray.
// =Output=
// P: Point of intersection (if any).
// Returns: True(1) if ray intersects with sphere.
UBOOL SphereRayTrace( FVector Origin, FLOAT Radius, FVector P1, FVector P2, FVector& P );