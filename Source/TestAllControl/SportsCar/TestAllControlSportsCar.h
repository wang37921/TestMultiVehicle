// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TestAllControlPawn.h"
#include "TestAllControlSportsCar.generated.h"

/**
 *  Sports car wheeled vehicle implementation
 */
UCLASS(abstract)
class ATestAllControlSportsCar : public ATestAllControlPawn
{
	GENERATED_BODY()
	
public:

	ATestAllControlSportsCar();
};
