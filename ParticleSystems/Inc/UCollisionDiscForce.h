
void Apply( AParticleGenerator* System, FLOAT DeltaTime );

// =Input=
// Origin: Center of the disc in three-space.
// Normal: A vector perpendicular to the collision surface.
// Radius: Radius of disc. (if zero, we assume infinite extents)
// P1, P2: Endpoints of ray.
// =Output=
// P: Point of intersection (if any).
// Returns: True(1) if ray intersects with the disc.
UBOOL DiscRayTrace( FVector Origin, FVector Normal, FLOAT Radius, FVector P1, FVector P2, FVector& P );