#include "NBodySimulationManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

constexpr float MAX_TICK_DURATION = 0.0167f;

ANBodySimulationManager::ANBodySimulationManager()
{
	InitializeActorTick();
	InitializeMeshComponent();
	Bounds.Set(SimulationParams.CameraOrthoWidth * 0.5f, SimulationParams.CameraOrthoHeight * 0.5f);
}

void ANBodySimulationManager::InitializeActorTick()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_DuringPhysics;
}

void ANBodySimulationManager::InitializeMeshComponent()
{
	InstancedMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedMesh"));
	SetRootComponent(InstancedMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Game/_Project/Assets/Circle'"));
	if (MeshAsset.Object)
	{
		InstancedMesh->SetStaticMesh(MeshAsset.Object);
	}
}

void ANBodySimulationManager::BeginPlay()
{
	Super::BeginPlay();

	if (GamificationParams.IsGamificationEnabled)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
		if (PlayerController)
		{
			PlayerController->bShowMouseCursor = true;
			PlayerController->bEnableClickEvents = true;
			PlayerController->bEnableMouseOverEvents = true;

			PlayerController->InputComponent->BindAction("LeftClick", IE_Pressed, this, &ANBodySimulationManager::OnLeftClick);
		}	
	}
	
	InitializeBodies();
}

void ANBodySimulationManager::InitializeBodies()
{
	AllocateBodies();
	PopulateBodies();
	
	InstancedMesh->AddInstances(Transforms, false);
}

void ANBodySimulationManager::AllocateBodies()
{
	Bodies.SetNumUninitialized(SimulationParams.NumBodies);
	Transforms.SetNumUninitialized(SimulationParams.NumBodies);
}

void ANBodySimulationManager::PopulateBodies()
{
	for (int32 Index = 0; Index < SimulationParams.NumBodies; ++Index)
	{
		auto [RandomPosition, RandomVelocity, Mass] = GenerateBodyParameters();
		FTransform MeshTransform(FRotator(), ConvertTo3D(RandomPosition), FVector(FMath::Sqrt(Mass) * SimulationParams.BodySize));

		Transforms[Index] = MeshTransform;
		Bodies[Index] = FBody{RandomPosition, RandomVelocity, Mass, Index};
	}
}

TTuple<FVector2D, FVector2D, float> ANBodySimulationManager::GenerateBodyParameters() const
{
	FVector2D RandomPosition = FMath::RandPointInCircle(SimulationParams.PlacementRadius);
	FVector2D RandomVelocity = GenerateRandomVelocity(RandomPosition);
	float Mass = FMath::FRandRange(SimulationParams.MinMass, SimulationParams.MaxMass);
	return {RandomPosition, RandomVelocity, Mass};
}

FVector2D ANBodySimulationManager::GenerateRandomVelocity(const FVector2D& Position) const
{
	float RadialSpeedFactor = SimulationParams.PlacementRadius / Position.Size();
	FVector2D Velocity(FMath::FRandRange(SimulationParams.MinVelocity, SimulationParams.MaxVelocity) / RadialSpeedFactor, 0.0f);
	return Velocity.GetRotated(90.0f + FMath::RadiansToDegrees(FMath::Atan2(Position.Y, Position.X)));
}

void ANBodySimulationManager::Tick(float DeltaTime)
{
	DeltaTime = FMath::Min(DeltaTime, MAX_TICK_DURATION);
	
	Super::Tick(DeltaTime);

	//Gamification
	if (GamificationParams.IsGamificationEnabled)
	{
		if (GamificationParams.IsRepelActive){ ApplyRepelForce(); }
	}
	
	if (SimulationParams.IsGravityEnabled){ ApplyGravity(DeltaTime); }
	UpdatePositions(DeltaTime);
}

void ANBodySimulationManager::ApplyGravity(float DeltaTime)
{
	ParallelFor(Bodies.Num(), [this, DeltaTime](int32 Index)
	{
		UpdateBodyVelocity(Index, DeltaTime);
	});
}

void ANBodySimulationManager::UpdateBodyVelocity(int32 Index, float DeltaTime)
{
	FVector2D Acceleration = CalculateAcceleration(Index);
	Bodies[Index].Velocity += Acceleration * DeltaTime;
}

FVector2D ANBodySimulationManager::CalculateAcceleration(int32 Index) const
{
	FVector2D AccumulatedAcceleration(0.0f, 0.0f);
	for (const FBody& OtherBody : Bodies)
	{
		if (OtherBody.Index == Index) continue;
		AccumulatedAcceleration += CalculateGravityEffect(Bodies[Index], OtherBody);
	}

	return AccumulatedAcceleration;
}

FVector2D ANBodySimulationManager::CalculateGravityEffect(const FBody& Body, const FBody& OtherBody) const
{
	float Distance = FVector2D::Distance(Body.Position, OtherBody.Position);
	Distance = FMath::Max(Distance, SimulationParams.MinimumGravityDistance);
	return OtherBody.Mass / (Distance * Distance) * SimulationParams.GravityForce * (OtherBody.Position - Body.Position).GetSafeNormal();
}

void ANBodySimulationManager::UpdatePositions(float DeltaTime)
{
	for (FBody& Body: Bodies)
	{
		UpdateBodyPosition(Body, DeltaTime);
		Transforms[Body.Index].SetTranslation(ConvertTo3D(Body.Position));
	}
	InstancedMesh->BatchUpdateInstancesTransforms(0, Transforms, false, true);
}

void ANBodySimulationManager::UpdateBodyPosition(FBody& Body, float DeltaTime)
{
	Body.Position += Body.Velocity * DeltaTime;
	WrapPosition(Body.Position);
}

void ANBodySimulationManager::WrapPosition(FVector2D& Position) const
{
	if (Position.X < -Bounds.X) Position.X += SimulationParams.CameraOrthoWidth;
    else if (Position.X > Bounds.X) Position.X -= SimulationParams.CameraOrthoWidth;

    if (Position.Y < -Bounds.Y) Position.Y += SimulationParams.CameraOrthoHeight;
    else if (Position.Y > Bounds.Y) Position.Y -= SimulationParams.CameraOrthoHeight;
}

FVector ANBodySimulationManager::ConvertTo3D(const FVector2D& XYCoordinates)
{
	return FVector(XYCoordinates.X, XYCoordinates.Y, 0.0f);
}

// Gamification
void ANBodySimulationManager::OnScreenClicked(FVector2D ScreenPosition)
{
	FVector WorldPosition = ConvertScreenToWorld(ScreenPosition);
	CreateRepelArea(WorldPosition);
	GetWorld()->GetTimerManager().SetTimer(GamificationParams.RepelTimerHandle, this, &ANBodySimulationManager::DisableRepelArea, GamificationParams.RepelDuration, false);
}

FVector ANBodySimulationManager::ConvertScreenToWorld(FVector2D ScreenPosition)
{
	FVector WorldPosition;
	FVector WorldDirection;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	if (PlayerController)
	{
		PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldPosition, WorldDirection);
		return FVector(WorldPosition.X, WorldPosition.Y, 0.0f);
	}

	return FVector();
}

void ANBodySimulationManager::CreateRepelArea(FVector WorldPosition)
{
	GamificationParams.RepelCenter = WorldPosition;
	GamificationParams.IsRepelActive = true;
}

void ANBodySimulationManager::DisableRepelArea()
{
	GamificationParams.IsRepelActive = false;
}

void ANBodySimulationManager::ApplyRepelForce()
{
	if (!GamificationParams.IsRepelActive){ return; }
	for (FBody& Body : Bodies)
	{
		FVector2D Direction = (Body.Position - FVector2D(GamificationParams.RepelCenter)).GetSafeNormal();
		float Distance = FVector2D::Distance(Body.Position, FVector2D(GamificationParams.RepelCenter));
		float ForceMagnitude = CalculateRepelForce(Distance);
		Body.Velocity += Direction * ForceMagnitude;
	}
}

float ANBodySimulationManager::CalculateRepelForce(float Distance)
{
	float BaseForce = 1000.0f / FMath::Max(Distance, 1.0f);
	return BaseForce * GamificationParams.RepelForceMultiplier;
}

void ANBodySimulationManager::OnLeftClick()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController)
	{
		float MouseX, MouseY;
		if (PlayerController->GetMousePosition(MouseX, MouseY))
		{
			FVector2D ScreenPosition(MouseX, MouseY);
			OnScreenClicked(ScreenPosition);
		}
	}
}