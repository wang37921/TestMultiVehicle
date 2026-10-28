// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestAllControlWheelRear.h"
#include "UObject/ConstructorHelpers.h"

UTestAllControlWheelRear::UTestAllControlWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}