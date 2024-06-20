// Copyright Epic Games, Inc. All Rights Reserved.

#include "InstanceLevelCollisionBPLibrary.h"
#include "InstanceLevelCollision.h"
#include "ComponentSourceInterfaces.h" 
#include "Components/InstancedStaticMeshComponent.h"

//Mesh Creation
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMeshToMeshDescription.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "CompGeom/ConvexHull2.h"
#include "Generators/SweepGenerator.h"
#include "Engine/StaticMesh.h"
#include "ToolSceneQueriesUtil.h"
#include "CompGeom/PolygonTriangulation.h"
#include "Generators/RectangleMeshGenerator.h"
#include "DynamicMesh/MeshTransforms.h"
#include "TransformTypes.h"

//AssetCreation
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include "AssetToolsModule.h"
#include "FileHelpers.h"
#include "Materials/Material.h"
#include "PhysicsEngine/BodySetup.h"


//LevelInstance
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "LevelInstance/LevelInstanceActor.h"

//Mesh Simplification
#include "DynamicMesh/Operations/MergeCoincidentMeshEdges.h"
#include "OverlappingCorners.h"
#include "StaticMeshOperations.h"
#include "IMeshReductionManagerModule.h"
#include "IMeshReductionInterfaces.h"

#include "CleaningOps/SimplifyMeshOp.h"
#include "CleaningOps/RemeshMeshOp.h"
#include "CleaningOps/RemoveOccludedTrianglesOp.h"
#include "CompositionOps/VoxelMorphologyMeshesOp.h"
#include "CompositionOps/VoxelSolidifyMeshesOp.h"

#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#if WITH_EDITOR
#include "Misc/ScopedSlowTask.h"
#include "Editor.h"
#endif

using namespace UE::Geometry;

UInstanceLevelCollisionBPLibrary::UInstanceLevelCollisionBPLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool CapBottom(FDynamicMesh3* Mesh, FDynamicMesh3& Projected, float& ZValue, float Offset, FTransform Actortransform, ECollisionMaxSlice CollisionType = ECollisionMaxSlice::MinZ, bool bFlatBase = true, bool bMakeBasin = false)
{
	// compute the 2D convex hull
	FConvexHull2d HullCompute;
	TArray<FVector2d> ProjectedVertices;
	ProjectedVertices.SetNum(Mesh->MaxVertexID());
	for (int VID : Mesh->VertexIndicesItr())
	{
		const FVector3d& V = Mesh->GetVertexRef(VID);
		ProjectedVertices[VID] = FVector2d(V.X, V.Y);
	}
	bool bOK = HullCompute.Solve(Mesh->MaxVertexID(),
		[&ProjectedVertices](int VID) { return ProjectedVertices[VID]; },
		[&Mesh](int VID) { return Mesh->IsVertex(VID); }
	);
	if (!bOK)
	{
		return false;
	}
	// extract polygon
	const TArray<int32>& PolygonIndices = HullCompute.GetPolygonIndices();
	TArray<FVector2d> PolygonVertices;  PolygonVertices.SetNum(PolygonIndices.Num());
	// the min and max Z positions along the outer boundary; could be good reference points for placing the bottom cap
	double MinZ = FMathd::MaxReal, MaxZ = -FMathd::MaxReal;

	switch (CollisionType)
	{
	case ECollisionMaxSlice::MinXYBound:
	{
		for (int32 Idx = 0; Idx < PolygonVertices.Num(); Idx++)
		{
			PolygonVertices[Idx] = ProjectedVertices[PolygonIndices[Idx]];
			if (bFlatBase)
			{
				double Z = Mesh->GetVertex(PolygonIndices[Idx]).Z;
				MinZ = FMathd::Min(MinZ, Z);
				MaxZ = FMathd::Max(MaxZ, Z);
			}
		}
		ZValue = MinZ;
		ZValue += Offset;
		break;
	}
	case ECollisionMaxSlice::MaxXYBound:
	{
		for (int32 Idx = 0; Idx < PolygonVertices.Num(); Idx++)
		{
			PolygonVertices[Idx] = ProjectedVertices[PolygonIndices[Idx]];
			if (bFlatBase)
			{
				double Z = Mesh->GetVertex(PolygonIndices[Idx]).Z;
				MinZ = FMathd::Min(MinZ, Z);
				MaxZ = FMathd::Max(MaxZ, Z);
			}
		}
		ZValue = MaxZ;
		ZValue += Offset;
		break;
	}
	case ECollisionMaxSlice::MinZ:
	{
		FAxisAlignedBox3d MeshBound = Mesh->GetBounds();
		MinZ = MeshBound.Min.Z;
		ZValue = MinZ + Offset;
		break;
	}

	case ECollisionMaxSlice::WorldZ:
		ZValue = Actortransform.InverseTransformPosition(FVector::ZeroVector).Z + Offset;
		break;
	}

	// triangulate polygon
	TArray<FIndex3i> Triangles;
	PolygonTriangulation::TriangulateSimplePolygon(PolygonVertices, Triangles);
	// fill mesh with result
	// optionally make an open base enclosing the bottom region by sweeping the convex hull
	if (bFlatBase && bMakeBasin)
	{
		// add sides
		FGeneralizedCylinderGenerator MeshGen;
		MeshGen.CrossSection = FPolygon2d(PolygonVertices);
		MeshGen.Path.Add(FVector3d(0, 0, MinZ));
		MeshGen.Path.Add(FVector3d(0, 0, MaxZ));
		MeshGen.bCapped = false;
		MeshGen.Generate();
		Projected.Copy(&MeshGen);
	}
	else
	{
		Projected.Clear();
	}
	int StartVID = Projected.MaxVertexID();
	for (int32 Idx : PolygonIndices)
	{
		// either follow the shape of the boundary (with gaps) ...
		FVector3d Vertex = Mesh->GetVertex(Idx);
		// ... or make a flat base
		if (bFlatBase)
		{
			Vertex.Z = ZValue;
		}
		Projected.AppendVertex(Vertex);
	}
	for (const FIndex3i& Tri : Triangles)
	{
		Projected.AppendTriangle(FIndex3i(Tri.A + StartVID, Tri.B + StartVID, Tri.C + StartVID));
	}


	return true;
}

double CalculateTargetEdgeLength(int TargetTriCount, TSharedPtr<FDynamicMesh3, ESPMode::ThreadSafe> OriginalMesh)
{
	double InitialMeshArea = 0;
	for (int tid : OriginalMesh->TriangleIndicesItr())
	{
		InitialMeshArea += OriginalMesh->GetTriArea(tid);
	}

	double TargetTriArea = InitialMeshArea / (double)TargetTriCount;
	double EdgeLen = TriangleUtil::EquilateralEdgeLengthForArea(TargetTriArea);
	return (double)FMath::RoundToInt(EdgeLen * 100.0) / 100.0;
}

void GetMergeMesh(ALevelInstance* LevelInstance, FDynamicMesh3& MergedMesh, TArray<AStaticMeshActor*>& MeshActor)
{
	if (LevelInstance)
	{
		FTransform ActorTransform = LevelInstance->GetActorTransform();
		TArray<AActor*> BreakActors;

		TArray<UStaticMeshComponent*> StaticMeshComponents;
		LevelInstance->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
		// Create progress indicator dialog
		FText TaskLength = FText::FromString("Reading Staticmesh : 0 / " + FString::FromInt(StaticMeshComponents.Num()));
		FScopedSlowTask SlowTask(StaticMeshComponents.Num(), TaskLength);
		SlowTask.MakeDialog();

		//Merge 
		FDynamicMeshEditor MergeEditor(&MergedMesh);
		FMeshIndexMappings Mappings;
		int index = 0;

		for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
		{
			if (UInstancedStaticMeshComponent* ISMComponent = Cast<UInstancedStaticMeshComponent>(StaticMeshComponent))
			{
				SlowTask.EnterProgressFrame(1.0, FText::FromString("Reading Staticmesh : " + FString::FromInt(index) + " / " + FString::FromInt(StaticMeshComponents.Num())));
				index++;

				FDynamicMesh3 Mesh;
				bool bMeshIsRealBad = false;
				FTriMeshCollisionData CollisionData;
				if (ISMComponent->GetStaticMesh())
				{
					ISMComponent->GetStaticMesh()->GetPhysicsTriMeshData(&CollisionData, true);
					for (const FVector3f& V : CollisionData.Vertices)
					{
						Mesh.AppendVertex((FVector3d)V);
					}
					for (FTriIndices T : CollisionData.Indices)
					{
						if (Mesh.FindTriangle(T.v0, T.v1, T.v2) != FDynamicMesh3::InvalidID)
						{
							bMeshIsRealBad = true;
							continue; // skip duplicate triangles in mesh
						}
						if (FDynamicMesh3::NonManifoldID == Mesh.AppendTriangle(T.v0, T.v1, T.v2))
						{
							int New0 = Mesh.AppendVertex(Mesh, T.v0);
							int New1 = Mesh.AppendVertex(Mesh, T.v1);
							int New2 = Mesh.AppendVertex(Mesh, T.v2);
							Mesh.AppendTriangle(New0, New1, New2);
							bMeshIsRealBad = true;
						}
					}
					//MeshActor.Add(BreakMesh);
				}

				for (int32 InstanceIndex = 0; InstanceIndex < ISMComponent->GetInstanceCount(); ++InstanceIndex)
				{
					FTransform InstanceTransform;
					if (ensure(ISMComponent->GetInstanceTransform(InstanceIndex, InstanceTransform, /*bWorldSpace=*/ true)))
					{
						FTransform LocalTransform = InstanceTransform.GetRelativeTransform(ActorTransform);
						FDynamicMesh3 SubMesh = Mesh;
						FTransformSRT3d Transform = FTransformSRT3d(LocalTransform);
						if (Transform.GetDeterminant() < 0)
						{
							SubMesh.ReverseOrientation(false);
						}
						MergeEditor.AppendMesh(&SubMesh, Mappings, [&Transform, &ActorTransform](int, const FVector3d& P) {return Transform.TransformPosition(P) /*- ActorTransform.GetTranslation()*/; }, [&Transform](int, const FVector3d& N) {return Transform.TransformVector(N); });
					}
				}
			}
		}
	}
}


void UInstanceLevelCollisionBPLibrary::GetCollisionSliceInfo(ALevelInstance* LevelInstance, TArray<AStaticMeshActor*> MeshActor, float Offset, ECollisionMaxSlice CollisionType, int& SliceHeight)
{

	FString LevelName = LevelInstance->GetActorLabel();
	FString SavePath = FPaths::GetPath(LevelInstance->GetWorldAssetPackage());
	TArray<TSharedPtr<FDynamicMesh3>> MeshList;
	FTransform Actortransform = LevelInstance->GetTransform();

	if (LevelInstance)
	{
		FTransform SavedTransform = LevelInstance->GetTransform();
		if (CollisionType != ECollisionMaxSlice::WorldZ)
		{
			FDynamicMesh3 MeshMerge;
			UWorld* World = LevelInstance->GetWorld();
			GetMergeMesh(LevelInstance, MeshMerge, MeshActor);
			FDynamicMesh3 MeshLoop;
			bool bMeshIsRealBad = false;
			FTriMeshCollisionData CollisionData;
			FProgressCancel Progress;

			//Merge 
			
			FDynamicMeshEditor MergeEditor(&MeshMerge);
			FMeshIndexMappings Mappings;


			// Cap the bottom of the mesh
			FDynamicMesh3 Projected;


			FDynamicMesh3* Mesh = &MeshMerge;
			FConvexHull2d HullCompute;
			TArray<FVector2d> ProjectedVertices;
			bool bFlatBase = true;
			bool bMakeBasin = false;
			ProjectedVertices.SetNum(Mesh->MaxVertexID());
			for (int VID : Mesh->VertexIndicesItr())
			{
				const FVector3d& V = Mesh->GetVertexRef(VID);
				ProjectedVertices[VID] = FVector2d(V.X, V.Y);
			}
			bool bOK = HullCompute.Solve(Mesh->MaxVertexID(),
				[&ProjectedVertices](int VID) { return ProjectedVertices[VID]; },
				[&Mesh](int VID) { return Mesh->IsVertex(VID); }
			);
			if (!bOK)
			{
				return;
			}
			// extract polygon
			const TArray<int32>& PolygonIndices = HullCompute.GetPolygonIndices();
			TArray<FVector2d> PolygonVertices;  PolygonVertices.SetNum(PolygonIndices.Num());
			// the min and max Z positions along the outer boundary; could be good reference points for placing the bottom cap
			double MinZ = FMathd::MaxReal, MaxZ = -FMathd::MaxReal;

			switch (CollisionType)
			{
			case ECollisionMaxSlice::MinXYBound:
			{
				for (int32 Idx = 0; Idx < PolygonVertices.Num(); Idx++)
				{
					PolygonVertices[Idx] = ProjectedVertices[PolygonIndices[Idx]];
					if (bFlatBase)
					{
						double Z = Mesh->GetVertex(PolygonIndices[Idx]).Z;
						MinZ = FMathd::Min(MinZ, Z);
						MaxZ = FMathd::Max(MaxZ, Z);
					}
				}
				SliceHeight = MinZ;
				SliceHeight += Offset;
				break;
			}
			case ECollisionMaxSlice::MaxXYBound:
			{
				for (int32 Idx = 0; Idx < PolygonVertices.Num(); Idx++)
				{
					PolygonVertices[Idx] = ProjectedVertices[PolygonIndices[Idx]];
					if (bFlatBase)
					{
						double Z = Mesh->GetVertex(PolygonIndices[Idx]).Z;
						MinZ = FMathd::Min(MinZ, Z);
						MaxZ = FMathd::Max(MaxZ, Z);
					}
				}
				SliceHeight = MaxZ;
				SliceHeight += Offset;
				break;
			}
			case ECollisionMaxSlice::MinZ:
			{
				FAxisAlignedBox3d MeshBound = Mesh->GetBounds();
				MinZ = MeshBound.Min.Z;
				SliceHeight = MinZ + Offset;
				break;
			}

			case ECollisionMaxSlice::WorldZ:
				SliceHeight = Actortransform.InverseTransformPosition(FVector::ZeroVector).Z + Offset;
				break;
			}

			for (int i = 0; i < MeshActor.Num(); i++)
			{
				MeshActor[i]->Destroy();
			}
		}
		else
		{
			SliceHeight = Actortransform.InverseTransformPosition(FVector::ZeroVector).Z + Offset;
		}
		//LevelInstance->Discard();
		UE_LOG(LogTemp, Warning, TEXT("%d"), SliceHeight);
	}
}

void MergeInstancesMeshes(ALevelInstance* LevelInstance, FDynamicMesh3 &MergedMesh, TMap<UStaticMesh*, TArray<FTransform>>&InstancesInfo, int PreSimplificationPercentage)
{
	if (LevelInstance)
	{
		FTransform ActorTransform = LevelInstance->GetActorTransform();
		TArray<AActor*> BreakActors;
		//LevelInstance->GetLevelInstanceSubsystem()->BreakLevelInstance(LevelInstance, 1U, &BreakActors);
		TArray<UStaticMeshComponent*> StaticMeshComponents;
		LevelInstance->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
		// Create progress indicator dialog
		FText TaskLength = FText::FromString("Reading Staticmesh : 0 / " + FString::FromInt(StaticMeshComponents.Num()));
		FScopedSlowTask SlowTask(StaticMeshComponents.Num(), TaskLength);
		SlowTask.MakeDialog();

		//Merge 
		FDynamicMeshEditor MergeEditor(&MergedMesh);
		FMeshIndexMappings Mappings;

		for (int j = 0; j < StaticMeshComponents.Num(); j++)// UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
		{
			if (UInstancedStaticMeshComponent* ISMComponent = Cast<UInstancedStaticMeshComponent>(StaticMeshComponents[j]))
			{
				SlowTask.EnterProgressFrame(1.0, FText::FromString("Reading Staticmesh : " + FString::FromInt(j) + " / " + FString::FromInt(StaticMeshComponents.Num())));

				FDynamicMesh3 Mesh;
				bool bMeshIsRealBad = false;
				FTriMeshCollisionData CollisionData;
				if (ISMComponent->GetStaticMesh())
				{
					ISMComponent->GetStaticMesh()->GetPhysicsTriMeshData(&CollisionData, true);
					for (const FVector3f& V : CollisionData.Vertices)
					{
						Mesh.AppendVertex((FVector3d)V);
					}
					for (FTriIndices T : CollisionData.Indices)
					{
						if (Mesh.FindTriangle(T.v0, T.v1, T.v2) != FDynamicMesh3::InvalidID)
						{
							bMeshIsRealBad = true;
							continue; // skip duplicate triangles in mesh
						}
						if (FDynamicMesh3::NonManifoldID == Mesh.AppendTriangle(T.v0, T.v1, T.v2))
						{
							int New0 = Mesh.AppendVertex(Mesh, T.v0);
							int New1 = Mesh.AppendVertex(Mesh, T.v1);
							int New2 = Mesh.AppendVertex(Mesh, T.v2);
							Mesh.AppendTriangle(New0, New1, New2);
							bMeshIsRealBad = true;
						}
					}
					FMergeCoincidentMeshEdges Merger(&Mesh);
					Merger.Apply();
					//MeshActor.Add(BreakMesh);
				}

				FProgressCancel Progress;
				//Init Simply Mesh tool

				TUniquePtr<FSimplifyMeshOp> SimplifyOp = MakeUnique<FSimplifyMeshOp>();
				SimplifyOp->bDiscardAttributes = false;
				SimplifyOp->bPreventNormalFlips = true;
				SimplifyOp->bPreserveSharpEdges = true;
				SimplifyOp->bAllowSeamCollapse = false;
				SimplifyOp->bReproject = false;
				SimplifyOp->SimplifierType = ESimplifyType::QEM;
				SimplifyOp->TargetEdgeLength = 5.0;
				SimplifyOp->TargetMode = ESimplifyTargetType::Percentage;
				SimplifyOp->TargetPercentage = PreSimplificationPercentage;
				SimplifyOp->MeshBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
				SimplifyOp->GroupBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
				SimplifyOp->MaterialBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
				SimplifyOp->OriginalMesh = MakeShared<FDynamicMesh3, ESPMode::ThreadSafe>(Mesh);
				SimplifyOp->OriginalMeshSpatial = MakeShared<FDynamicMeshAABBTree3, ESPMode::ThreadSafe>(SimplifyOp->OriginalMesh.Get());
				SimplifyOp->CalculateResult(&Progress);
				TUniquePtr<FDynamicMesh3> SimplifyNewMesh = SimplifyOp->ExtractResult();

				TArray<FTransform> TransformList;
				for (int32 InstanceIndex = 0; InstanceIndex < ISMComponent->GetInstanceCount(); ++InstanceIndex)
				{
					FTransform InstanceTransform;
					if(ISMComponent->IsValidInstance(InstanceIndex))
					{
						if (ensure(ISMComponent->GetInstanceTransform(InstanceIndex, InstanceTransform, true)))
						{
							FTransform LocalTransform = InstanceTransform.GetRelativeTransform(ActorTransform);
							FDynamicMesh3 SubMesh = *SimplifyNewMesh.Get();
							FTransformSRT3d Transform = FTransformSRT3d(LocalTransform);
							if (Transform.GetDeterminant() < 0)
							{
								SubMesh.ReverseOrientation(false);
							}
							MergeEditor.AppendMesh(&SubMesh, Mappings, [&Transform, &ActorTransform](int, const FVector3d& P) {return Transform.TransformPosition(P); }, [&Transform](int, const FVector3d& N) {return Transform.TransformVector(N); });


							//MeshTransforms::ApplyTransform(SubMesh, FTransformSRT3d(LocalTransform));
							TransformList.Add(InstanceTransform);
						}
					}
				}
				InstancesInfo.Add(ISMComponent->GetStaticMesh(), TransformList);
				SimplifyNewMesh->Clear();
			}
		}
	}
}


void MergeActorMeshes(TArray<AStaticMeshActor*> MeshActor, FDynamicMesh3& MergedMesh, TMap<UStaticMesh*, TArray<FTransform>>& InstancesInfo, int PreSimplificationPercentage)
{
	{
		if (MeshActor.Num() > 0)
		{
			FTransform ActorTransform = MeshActor[0]->GetActorTransform();
			TArray<AStaticMeshActor*> BreakActors = MeshActor;
			// Create progress indicator dialog


			TMap < UStaticMesh*, TArray<FTransform >> ActorMap;

			for (int i = 0; i < MeshActor.Num(); i++)
			{
				TArray<FTransform> transform;
				if (ActorMap.Contains(MeshActor[i]->GetStaticMeshComponent()->GetStaticMesh()))
				{
					transform = *ActorMap.Find(MeshActor[i]->GetStaticMeshComponent()->GetStaticMesh());
					transform.Add(MeshActor[i]->GetActorTransform());
					ActorMap.Add(MeshActor[i]->GetStaticMeshComponent()->GetStaticMesh(), transform);
				}
				else
				{
					transform.Add(MeshActor[i]->GetActorTransform());
					ActorMap.Add(MeshActor[i]->GetStaticMeshComponent()->GetStaticMesh(), transform);
				}
			}

			FText TaskLength = FText::FromString("Reading Staticmesh : 0 / " + FString::FromInt(ActorMap.Num()));
			FScopedSlowTask SlowTask(ActorMap.Num(), TaskLength);
			SlowTask.MakeDialog();
			//Merge 
			FDynamicMeshEditor MergeEditor(&MergedMesh);
			FMeshIndexMappings Mappings;
			int j = 0;
			for (auto& Elem : ActorMap)
			{
				SlowTask.EnterProgressFrame(1.0, FText::FromString("Reading Staticmesh : " + FString::FromInt(j) + " / " + FString::FromInt(ActorMap.Num())));

				FDynamicMesh3 Mesh;
				bool bMeshIsRealBad = false;
				FTriMeshCollisionData CollisionData;
				if (Elem.Key)
				{
					Elem.Key->GetPhysicsTriMeshData(&CollisionData, true);
					for (const FVector3f& V : CollisionData.Vertices)
					{
						Mesh.AppendVertex((FVector3d)V);
					}
					for (FTriIndices T : CollisionData.Indices)
					{
						if (Mesh.FindTriangle(T.v0, T.v1, T.v2) != FDynamicMesh3::InvalidID)
						{
							bMeshIsRealBad = true;
							continue; // skip duplicate triangles in mesh
						}
						if (FDynamicMesh3::NonManifoldID == Mesh.AppendTriangle(T.v0, T.v1, T.v2))
						{
							int New0 = Mesh.AppendVertex(Mesh, T.v0);
							int New1 = Mesh.AppendVertex(Mesh, T.v1);
							int New2 = Mesh.AppendVertex(Mesh, T.v2);
							Mesh.AppendTriangle(New0, New1, New2);
							bMeshIsRealBad = true;
						}
					}
					FMergeCoincidentMeshEdges Merger(&Mesh);
					Merger.Apply();
					//MeshActor.Add(BreakMesh);
				}

				FProgressCancel Progress;
				//Init Simply Mesh tool

				TUniquePtr<FSimplifyMeshOp> SimplifyOp = MakeUnique<FSimplifyMeshOp>();
				SimplifyOp->bDiscardAttributes = false;
				SimplifyOp->bPreventNormalFlips = true;
				SimplifyOp->bPreserveSharpEdges = true;
				SimplifyOp->bAllowSeamCollapse = false;
				SimplifyOp->bReproject = false;
				SimplifyOp->SimplifierType = ESimplifyType::QEM;
				SimplifyOp->TargetEdgeLength = 5.0;
				SimplifyOp->TargetMode = ESimplifyTargetType::Percentage;
				SimplifyOp->TargetPercentage = PreSimplificationPercentage;
				SimplifyOp->MeshBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
				SimplifyOp->GroupBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
				SimplifyOp->MaterialBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
				SimplifyOp->OriginalMesh = MakeShared<FDynamicMesh3, ESPMode::ThreadSafe>(Mesh);
				SimplifyOp->OriginalMeshSpatial = MakeShared<FDynamicMeshAABBTree3, ESPMode::ThreadSafe>(SimplifyOp->OriginalMesh.Get());
				SimplifyOp->CalculateResult(&Progress);
				TUniquePtr<FDynamicMesh3> SimplifyNewMesh = SimplifyOp->ExtractResult();

				TArray<FTransform> TransformList;
				for (int32 InstanceIndex = 0; InstanceIndex < Elem.Value.Num(); ++InstanceIndex)
				{
					FTransform InstanceTransform;
					if (Elem.Value.IsValidIndex(InstanceIndex))
					{

						FTransform LocalTransform = Elem.Value[InstanceIndex].GetRelativeTransform(FTransform(FRotator(0, 0, 0),ActorTransform.GetLocation(), FVector(1, 1, 1)));
						FDynamicMesh3 SubMesh = *SimplifyNewMesh.Get();
						FTransformSRT3d Transform = FTransformSRT3d(LocalTransform);
						if (Transform.GetDeterminant() < 0)
						{
							SubMesh.ReverseOrientation(false);
						}
						MergeEditor.AppendMesh(&SubMesh, Mappings, [&Transform, &ActorTransform](int, const FVector3d& P) {return Transform.TransformPosition(P); }, [&Transform](int, const FVector3d& N) {return Transform.TransformVector(N); });


						//MeshTransforms::ApplyTransform(SubMesh, FTransformSRT3d(LocalTransform));
						TransformList.Add(InstanceTransform);
					}
				}
				InstancesInfo.Add(Elem.Key, TransformList);
				SimplifyNewMesh->Clear();

				j++;
			}
		}
	}
}


void CreateCollisionMesh(ALevelInstance* LevelInstance,  TSharedPtr<FDynamicMesh3, ESPMode::ThreadSafe>TargetMesh, FString LevelName, FString SavePath, FTransform OriginalTransform, int TargetPercentage, bool bSaveAsset)
{

	//Init Asset Name and Mesh
	FString Name = LevelName;
	const FString DefaultSuffix = TEXT("_Collider");
	FString PackageName = SavePath + "/Collider/" + Name + DefaultSuffix;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);
	UPackage* Package = CreatePackage(*PackageName);

	//Create StaticMesh Collision
	UStaticMesh* myStaticMesh = NewObject<UStaticMesh>(Package, *Name, RF_Public | RF_Standalone);
	myStaticMesh->InitResources();
	myStaticMesh->SetNumSourceModels(0);
	FStaticMeshSourceModel& SrcModel = myStaticMesh->AddSourceModel();
	FMeshBuildSettings buildsetting;
	FMeshDescription* MeshDescription = myStaticMesh->CreateMeshDescription(0);
	FDynamicMeshToMeshDescription Converters;
	Converters.Convert(TargetMesh.Get(), *MeshDescription);

	//Simplify Final Mesh
	TUniquePtr<FSimplifyMeshOp> FinalOp = MakeUnique<FSimplifyMeshOp>();
	FinalOp->bDiscardAttributes = false;
	FinalOp->bPreventNormalFlips = true;
	FinalOp->bPreserveSharpEdges = true;
	FinalOp->bAllowSeamCollapse = false;
	FinalOp->bReproject = false;
	FinalOp->TargetEdgeLength = 5.0;
	FinalOp->SimplifierType = ESimplifyType::UEStandard;
	FinalOp->TargetMode = ESimplifyTargetType::TriangleCount;
	FinalOp->TargetCount = TargetPercentage;
	FinalOp->MeshBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
	FinalOp->GroupBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
	FinalOp->MaterialBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
	FinalOp->OriginalMesh = TargetMesh;
	FinalOp->OriginalMeshSpatial = MakeShared<FDynamicMeshAABBTree3, ESPMode::ThreadSafe>(FinalOp->OriginalMesh.Get());
	FinalOp->OriginalMeshDescription = MakeShared<FMeshDescription, ESPMode::ThreadSafe>(*MeshDescription);
	IMeshReductionManagerModule& MeshReductionModule = FModuleManager::Get().LoadModuleChecked<IMeshReductionManagerModule>("MeshReductionInterface");
	FinalOp->MeshReduction = MeshReductionModule.GetStaticMeshReductionInterface();
	FProgressCancel Progress;
	FinalOp->CalculateResult(&Progress);
	TUniquePtr<FDynamicMesh3> FinalMesh = FinalOp->ExtractResult();

	Converters.Convert(FinalMesh.Get(), *MeshDescription);

	TArray<const FMeshDescription*> MeshDescriptionPointers;
	MeshDescriptionPointers.Add(MeshDescription);
	UStaticMesh::FBuildMeshDescriptionsParams paramsColl;
	paramsColl.bBuildSimpleCollision = true;
	myStaticMesh->BuildFromMeshDescriptions(MeshDescriptionPointers, paramsColl);
	myStaticMesh->GetBodySetup()->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;
	TArray<UPackage*> SavePackage;
	SavePackage.Add(myStaticMesh->GetOutermost());
	if(bSaveAsset)
	FEditorFileUtils::PromptForCheckoutAndSave(SavePackage, true, true);

	//Add infos to the Spawned LevelInstance
	FActorSpawnParameters param;
	if (LevelInstance)
	{
		LevelInstance->EnterEdit();
		LevelInstance->Modify();

		LevelInstance->GetLevelInstanceSubsystem()->ForEachActorInLevelInstance(LevelInstance, [](AActor* Actor) {
			if (Actor->GetActorLabel().Contains("Collider"))
				Actor->Destroy();
			return true;
			});

		ULevelInstanceSubsystem* subLevel = LevelInstance->GetLevelInstanceSubsystem();
		ULevel* level = subLevel->GetLevelInstanceLevel(LevelInstance);
		UWorld* LIworld = level->GetWorld();
		param.OverrideLevel = level;
		AStaticMeshActor* Collider = LIworld->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), OriginalTransform.GetTranslation(), FRotator(0, 0, 0), param);
		Collider->GetStaticMeshComponent()->SetVisibility(false);
		Collider->SetActorLabel(Name);
		Collider->GetStaticMeshComponent()->SetStaticMesh(myStaticMesh);
		Collider->MarkComponentsRenderStateDirty();
		LevelInstance->ExitEdit();
	}
	else
	{
		UWorld* CollisionWorld = GEditor->GetEditorWorldContext().World();
		AStaticMeshActor* Collider = CollisionWorld->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), OriginalTransform.GetTranslation(), FRotator(0, 0, 0), param);
		Collider->SetActorHiddenInGame(true);
		Collider->SetActorLabel(Name);
		Collider->GetStaticMeshComponent()->SetStaticMesh(myStaticMesh);
		Collider->MarkComponentsRenderStateDirty();
	}
}

void UInstanceLevelCollisionBPLibrary::GenerateCollision(ALevelInstance* LevelInstance, TArray<AStaticMeshActor*> SelectedMeshActor, float ZOffset, ECollisionMaxSlice CollisionType, bool bRemesh, int PreSimplificationPercentage, bool bSaveAsset, int VoxelDensity, float TargetPercentage, float Winding)
{
	float NumTask = 5.0;
	FText SlowTaskText = NSLOCTEXT("ReadAllMeshes", "ReadAllMeshes", "Reading all meshes ...");
	const int32 UnitProgressOutOfLoop = 1;  // One progress frame is equivalent to a minimum of 2% of the loop progress

	FScopedSlowTask SlowTask(NumTask, SlowTaskText);
	SlowTask.MakeDialog();

	// Declare progress shortcut lambdas
	auto EnterProgressFrame = [&SlowTask](float Progress, const FText& Text = FText())
	{
		SlowTask.EnterProgressFrame(Progress, Text);
	};

	UWorld* World = GEditor->GetEditorWorldContext().World();
	FString LevelName;
	FString SavePath;
	TArray<TSharedPtr<FDynamicMesh3>> MeshList;
	TArray<UStaticMesh*> StaticMeshList;
	FTransform OriginalTransform;
	TMap<UStaticMesh*, TArray<FTransform>>InstancesInfo;
	TArray<FTransform> ListTransforms;
	TArray<AStaticMeshActor*> InstanceMeshes;
	TArray<AStaticMeshActor*> MeshActor;
	FDynamicMesh3 MergedMesh;

	//If Collision is generated for a Level Instance
	if (LevelInstance)
	{
		LevelName = LevelInstance->GetActorLabel();
		OriginalTransform = LevelInstance->GetTransform();
		SavePath = FPaths::GetPath(LevelInstance->GetWorldAssetPackage());
		FTransform SavedTransform = LevelInstance->GetTransform();
		World = LevelInstance->GetWorld();

		MergeInstancesMeshes(LevelInstance, MergedMesh, InstancesInfo, PreSimplificationPercentage);
		InstanceMeshes = MeshActor;
	}
	//If Collision is generated for a StaticMesh
	if (SelectedMeshActor.Num() > 0)
	{
		World = SelectedMeshActor[0]->GetWorld();
		OriginalTransform = SelectedMeshActor[0]->GetTransform();
		LevelName = SelectedMeshActor[0]->GetActorLabel();
		SavePath = FPaths::GetPath(SelectedMeshActor[0]->GetStaticMeshComponent()->GetStaticMesh()->GetOutermost()->GetPathName());
		MergeActorMeshes(SelectedMeshActor, MergedMesh, InstancesInfo, PreSimplificationPercentage);
		MeshActor.Append(SelectedMeshActor);
	}

	// Cap the bottom of the mesh
	FDynamicMesh3 Projected;
	float ZValue = 0;
	CapBottom(&MergedMesh, Projected, ZValue, ZOffset, OriginalTransform, CollisionType);
	TArray<int> RemoveTris;
	for (int tid : MergedMesh.TriangleIndicesItr())
	{
		FIndex3i Tri = MergedMesh.GetTriangle(tid);
		if (MergedMesh.GetVertex(Tri.A).Z < ZValue)
		{
			RemoveTris.Add(tid);
		}
	}
	FDynamicMeshEditor Editor(&MergedMesh);
	Editor.RemoveTriangles(RemoveTris, true);

	//Add the Cap to the merge mesh - Useful to preview the Cap mesh
	/*FMeshIndexMappings IndexMaps;
	Editor.AppendMesh(&Projected, IndexMaps);*/

	SlowTaskText = NSLOCTEXT("Remeshing", "Remeshing", "Remeshing ...");
	EnterProgressFrame(1.0, SlowTaskText);

	FProgressCancel Progress;
	TUniquePtr<FDynamicMesh3> Remesh;


	//Remesh
	if (bRemesh)
	{
		TUniquePtr<FRemeshMeshOp> RemeshOp = MakeUnique<FRemeshMeshOp>();
		RemeshOp->RemeshType = ERemeshType::Standard;
		RemeshOp->bCollapses = true;
		RemeshOp->bDiscardAttributes = false;
		RemeshOp->bFlips = true;
		RemeshOp->bPreserveSharpEdges = true;
		RemeshOp->SmoothingType = ERemeshSmoothingType::MeanValue;
		RemeshOp->MaxRemeshIterations = 20;
		RemeshOp->RemeshIterations = 20;
		RemeshOp->bReproject = true;
		RemeshOp->ProjectionTarget = &MergedMesh;
		RemeshOp->MeshBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
		RemeshOp->GroupBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
		RemeshOp->MaterialBoundaryConstraint = EEdgeRefineFlags::NoConstraint;
		TUniquePtr<FDynamicMesh3> ProjectionTarget = MakeUnique<FDynamicMesh3>(MergedMesh);
		TUniquePtr<FDynamicMeshAABBTree3> ProjectionTargetSpatial = MakeUnique<FDynamicMeshAABBTree3>(ProjectionTarget.Get(), true);
		RemeshOp->ProjectionTargetSpatial = ProjectionTargetSpatial.Get();
		RemeshOp->OriginalMesh = MakeShared<FDynamicMesh3, ESPMode::ThreadSafe>(MergedMesh);
		RemeshOp->OriginalMeshSpatial = MakeShared<FDynamicMeshAABBTree3, ESPMode::ThreadSafe>(RemeshOp->OriginalMesh.Get(), true);
		RemeshOp->TargetEdgeLength = CalculateTargetEdgeLength(MergedMesh.TriangleCount(), RemeshOp->OriginalMesh);
		RemeshOp->CalculateResult(&Progress);
		Remesh = RemeshOp->ExtractResult();
		RemeshOp = nullptr;
		MergedMesh.Clear();
	}
	else
	{
		Remesh = MakeUnique<FDynamicMesh3>(MergedMesh);
		MergedMesh.Clear();
	}


	//Jacketing Mesh 
	SlowTaskText = NSLOCTEXT("Jacketing", "Jacketing", "Jacketing ...");
	EnterProgressFrame(1.0, SlowTaskText);

	TUniquePtr<FRemoveOccludedTrianglesOp> JacketingOp = MakeUnique<FRemoveOccludedTrianglesOp>();
	JacketingOp->InsideMode = EOcclusionCalculationMode::SimpleOcclusionTest;
	JacketingOp->AddTriangleSamples = 4;
	JacketingOp->AddRandomRays = 4;
	JacketingOp->MeshTransforms.SetNum(1);
	JacketingOp->OriginalMesh = MakeShareable<FDynamicMesh3>(Remesh.Release());
	JacketingOp->OccluderTrees.Add(MakeShared<FDynamicMeshAABBTree3, ESPMode::ThreadSafe>(JacketingOp->OriginalMesh.Get()));
	JacketingOp->OccluderWindings.Emplace(); // empty winding tree, because simple occlusion test doesn't need it
	JacketingOp->OccluderTransforms.Emplace(); // default constructor is identity
	JacketingOp->OccluderTrees.Emplace(MakeShared<FDynamicMeshAABBTree3, ESPMode::ThreadSafe>(&Projected));
	JacketingOp->OccluderWindings.Emplace(); // empty winding tree, because simple occlusion test doesn't need it
	JacketingOp->OccluderTransforms.Emplace(); // default constructor is identity
	JacketingOp->CalculateResult(&Progress);
	TUniquePtr<FDynamicMesh3> jacketMesh = JacketingOp->ExtractResult();
	JacketingOp = nullptr;


	// Init Vox Wrap tool
	SlowTaskText = NSLOCTEXT("VoxWrap", "VoxWrap", "Vox Wrap ...");
	EnterProgressFrame(1.0, SlowTaskText);

	TUniquePtr<FVoxelSolidifyMeshesOp> Op = MakeUnique<FVoxelSolidifyMeshesOp>();
	Op->Transforms.SetNum(1);
	Op->Meshes.SetNum(1);
	TArray<TSharedPtr<const FDynamicMesh3, ESPMode::ThreadSafe>> OriginalDynamicMeshes;
	OriginalDynamicMeshes.Add(MakeShareable<FDynamicMesh3>(jacketMesh.Release()));
	Op->Meshes = OriginalDynamicMeshes;
	Op->Transforms[0] = FTransform::Identity;
	Op->OutputVoxelCount = VoxelDensity;
	Op->InputVoxelCount = VoxelDensity;
	Op->bAutoSimplify = false;
	Op->WindingThreshold = Winding;
	Op->CalculateResult(&Progress);
	TUniquePtr<FDynamicMesh3> Newmesh = Op->ExtractResult();
	Op = nullptr;

	// Init Vox Morph tool
	SlowTaskText = NSLOCTEXT("VoxMorph", "VoxMorph", "Vox Morph ...");
	EnterProgressFrame(1.0, SlowTaskText);

	TUniquePtr<FVoxelMorphologyMeshesOp> MorphOp = MakeUnique<FVoxelMorphologyMeshesOp>();
	MorphOp->Transforms.SetNum(1);
	MorphOp->Meshes.SetNum(1);
	TArray<TSharedPtr<const FDynamicMesh3, ESPMode::ThreadSafe>> OriginalDynamicMeshesMorph;
	OriginalDynamicMeshesMorph.Add(MakeShareable<FDynamicMesh3>(Newmesh.Release()));
	MorphOp->Meshes = OriginalDynamicMeshesMorph;
	MorphOp->Transforms[0] = FTransform::Identity;
	MorphOp->OutputVoxelCount = VoxelDensity;
	MorphOp->InputVoxelCount = VoxelDensity;
	MorphOp->bAutoSimplify = false;
	MorphOp->Operation = EMorphologyOperation::Close;
	MorphOp->CalculateResult(&Progress);
	TUniquePtr<FDynamicMesh3> Morphmesh = MorphOp->ExtractResult();
	MorphOp = nullptr;

	// Simplification and Save
	SlowTaskText = NSLOCTEXT("Simplifying", "Simplifying", "Simplifying ...");
	EnterProgressFrame(1.0, SlowTaskText);
	CreateCollisionMesh(LevelInstance, MakeShared<FDynamicMesh3, ESPMode::ThreadSafe>(*Morphmesh), LevelName, SavePath, OriginalTransform, TargetPercentage, bSaveAsset);


}