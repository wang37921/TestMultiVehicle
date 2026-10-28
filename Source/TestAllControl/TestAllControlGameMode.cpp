// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestAllControlGameMode.h"
#include "TestAllControlPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

ATestAllControlGameMode::ATestAllControlGameMode()
{
	PlayerControllerClass = ATestAllControlPlayerController::StaticClass();
}

APawn* ATestAllControlGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	if (!StartSpot)
	{
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	}

	FRotator StartRotation(ForceInit);
	StartRotation.Yaw = StartSpot->GetActorRotation().Yaw;

	FVector StartLocation = StartSpot->GetActorLocation();

	// 局域网联机多车起跑格（Grid Start）偏移计算：
	// 避免两辆开启刚体物理模拟的车辆在同一点重叠生成导致物理排斥弹飞
	if (SpawnedPlayerIndex > 0)
	{
		const FVector Forward = StartRotation.Vector();
		const FVector Right = FRotationMatrix(StartRotation).GetScaledAxis(EAxis::Y);

		// 排数（纵向往后退）与列数（左右错开）
		const int32 Row = (SpawnedPlayerIndex + 1) / 2;
		const float SideSign = (SpawnedPlayerIndex % 2 == 1) ? 1.0f : -1.0f;

		// 左右偏移 200cm（两车相距4米），每排向后退 600cm（6米）
		StartLocation += Right * (SideSign * 200.0f) - Forward * (Row * 600.0f);
	}

	SpawnedPlayerIndex++;

	const FTransform SpawnTransform(StartRotation, StartLocation);

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	if (PawnClass)
	{
		return GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo);
	}

	return nullptr;
}
