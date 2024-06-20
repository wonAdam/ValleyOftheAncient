// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "AddPatchTool.h"
#include "InstanceLevelCollisionBPLibrary.generated.h"


UENUM(BlueprintType)
enum class ECollisionMaxSlice : uint8
{
	MinXYBound,
	MaxXYBound,
	MinZ,
	WorldZ
};


UCLASS()
class UInstanceLevelCollisionBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

		UFUNCTION(BlueprintCallable, meta = (DisplayName = "GetCollisionSliceInfo", Keywords = "GetCollisionSliceInfo"), Category = "LevelInstanceCollision")
		static void GetCollisionSliceInfo(ALevelInstance* LevelInstanceBP, TArray<AStaticMeshActor*> MeshActor, float Offset, ECollisionMaxSlice CollisionType,int &SliceHeight);
	
		UFUNCTION(BlueprintCallable, meta = (DisplayName = "GenerateCollision", Keywords = "GenerateCollision"), Category = "LevelInstanceCollision")
		static void GenerateCollision(ALevelInstance* LevelInstanceBP, TArray<AStaticMeshActor*> MeshActor, float ZOffset, ECollisionMaxSlice CollisionType, bool bRemesh, int PreSimplificationPercentage, bool bSaveAsset, int VoxelDensity = 64, float TargetPercentage = 50.0, float Winding = 0.5);
};
