// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestAllControlWheelFront.h"
#include "UObject/ConstructorHelpers.h"

UTestAllControlWheelFront::UTestAllControlWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}