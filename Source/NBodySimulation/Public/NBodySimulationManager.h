#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NBodySimulationManager.generated.h"

USTRUCT()
struct FBody
{
	GENERATED_BODY()

	UPROPERTY()
	FVector2D Position;

	UPROPERTY()
	FVector2D Velocity;

	UPROPERTY()
	float Mass = 1.0f;

	UPROPERTY()
	int32 Index;
};

USTRUCT()
struct FNBodySimulationParams
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "NBody Simulation Parameters", meta = (ToolTip = "Enable gravity between bodies."))
	bool IsGravityEnabled = true;

	UPROPERTY(EditAnywhere, Category = "NBody Simulation Parameters", meta = (ToolTip = "Number of bodies to simulate."))
	int NumBodies = 1000;

	UPROPERTY(EditAnywhere, Category = "NBody Simulation Parameters", meta = (ToolTip = "Placement radius of bodies from the center of the scene."))
	float PlacementRadius = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "NBody Simulation Parameters", meta = (ToolTip = "Minimum velocity a newly spawned body has."))
	float MinVelocity = 400.0f;

	UPROPERTY(EditAnywhere, Category = "NBody Simulation Parameters", meta = (ToolTip = "Maximum velocity a newly spawned body has."))
	float MaxVelocity = 600.0f;

	UPROPERTY(EditAnywhere, Category = "NBody Simulation Parameters", meta = (ToolTip = "Size of the bodies."))
	float BodySize = 0.02f;

	UPROPERTY(EditAnywhere, Category = "NBody Simulation Parameters", meta = (ToolTip = "Gravity force applied to bodies."))
	float GravityForce = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "NBody Simulation Parameters", meta = (ToolTip = "Minimum mass a newly spawned body has."))
	float MinMass = 20.0f;

	UPROPERTY(EditAnywhere,	Category = "NBody Simulation Parameters", meta = (ToolTip = "Maximum mass a newly spawned body has."))
	float MaxMass = 100.0f;

	UPROPERTY(EditAnywhere, Category = "NBody Simulation Parameters", meta = (ToolTip = "Minimum distance between bodies for gravity to be applied."))
	float MinimumGravityDistance = 100.0f;
	
	float CameraOrthoWidth = 8000.0f;
	float CameraOrthoHeight = CameraOrthoWidth / 1.777778;
};

USTRUCT()
struct FNBodySimulationGamificationParams
{
	GENERATED_BODY()

	FVector RepelCenter;
	
	UPROPERTY(EditAnywhere, Category="NBody Simulation", meta = (ToolTip = "Enable gamification features."))
	bool IsGamificationEnabled = true;
	
	UPROPERTY(EditAnywhere, Category="NBody Simulation", meta = (ToolTip = "Indicates if the repel effect is currently active."))
	bool IsRepelActive = false;
	
	UPROPERTY(EditAnywhere, Category="NBody Simulation", meta = (ToolTip = "Affects the force of the repel effect."))
	float RepelForceMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category="NBody Simulation", meta = (ToolTip = "Duration of the repel effect."))
	float RepelDuration = 5.0f;

	FTimerHandle RepelTimerHandle;
};

UCLASS()
class NBODYSIMULATION_API ANBodySimulationManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ANBodySimulationManager();

private:
	UPROPERTY(VisibleAnywhere, Category="Mesh", meta=(AllowPrivateAccess="true"))
	UInstancedStaticMeshComponent* InstancedMesh;
	
	UPROPERTY(EditAnywhere, Category="NBody Simulation", meta = (ToolTip = "Parameters of the simulation that can be modified."))
	FNBodySimulationParams SimulationParams;

	UPROPERTY(EditAnywhere, Category="NBody Simulation", meta = (ToolTip = "Parameters of the gamification that can be modified."))
	FNBodySimulationGamificationParams GamificationParams;
	
	UPROPERTY()
	TArray<FBody> Bodies;

	UPROPERTY()
	TArray<FTransform> Transforms;

	UPROPERTY()
	FVector2D Bounds;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	void InitializeActorTick();
	void InitializeMeshComponent();
	void InitializeBodies();
	void AllocateBodies();
	void PopulateBodies();
	TTuple<FVector2D, FVector2D, float> GenerateBodyParameters() const;
	FVector2D GenerateRandomVelocity(const FVector2D& Position) const;
	void ApplyGravity(float DeltaTime);
	void UpdateBodyVelocity(int32 Index, float DeltaTime);
	FVector2D CalculateAcceleration(int32 Index) const;
	FVector2D CalculateGravityEffect(const FBody& Body, const FBody& OtherBody) const;
	void UpdatePositions(float DeltaTime);
	void UpdateBodyPosition(FBody& Body, float DeltaTime);
	void WrapPosition(FVector2D& Position) const;

	static FVector ConvertTo3D(const FVector2D& XYCoordinates);

	// Gamification
public:
	UFUNCTION(BlueprintCallable, Category="NBody Simulation Gamification")
	void OnScreenClicked(FVector2D ScreenPosition);

private:
	void OnLeftClick();
	FVector ConvertScreenToWorld(FVector2D ScreenPosition);
	void CreateRepelArea(FVector WorldPosition);
	void ApplyRepelForce();
	float CalculateRepelForce(float Distance);
	void DisableRepelArea();
};
