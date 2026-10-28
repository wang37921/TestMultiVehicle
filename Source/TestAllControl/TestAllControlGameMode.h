// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TestAllControlGameMode.generated.h"

UCLASS(abstract)
class ATestAllControlGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATestAllControlGameMode();

	/** 重写玩家车辆生成逻辑，实现赛车起跑格（Grid Start）排布，防止多车联机出生重叠发生物理碰撞弹飞 */
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

protected:
	/** 已生成的玩家计数（用于分配起跑格位） */
	UPROPERTY(Transient)
	int32 SpawnedPlayerIndex = 0;
};



