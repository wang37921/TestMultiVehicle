// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestAllControlPawn.h"
#include "TestAllControlWheelFront.h"
#include "TestAllControlWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "TestAllControl.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

ATestAllControlPawn::ATestAllControlPawn()
{
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

	// 开启 Actor 网络复制与物理移动复制
	bReplicates = true;
	AActor::SetReplicateMovement(true);
	SetNetUpdateFrequency(66.0f);
	SetMinNetUpdateFrequency(33.0f);
}

void ATestAllControlPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ATestAllControlPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ATestAllControlPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ATestAllControlPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ATestAllControlPawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ATestAllControlPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ATestAllControlPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ATestAllControlPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ATestAllControlPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ATestAllControlPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ATestAllControlPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ATestAllControlPawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ATestAllControlPawn::ResetVehicle);
	}
	else
	{
		UE_LOG(LogTestAllControl, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATestAllControlPawn::BeginPlay()
{
	Super::BeginPlay();

	// set up the flipped check timer
	GetWorld()->GetTimerManager().SetTimer(FlipCheckTimer, this, &ATestAllControlPawn::FlippedCheck, FlipCheckTime, true);
}

void ATestAllControlPawn::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	// clear the flipped check timer
	GetWorld()->GetTimerManager().ClearTimer(FlipCheckTimer);

	Super::EndPlay(EndPlayReason);
}

void ATestAllControlPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// 仅对本地控制的车辆平滑对齐后置摄像机 Yaw
	if (IsLocallyControlled() && BackSpringArm)
	{
		float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
		CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);
		BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
	}
}

void ATestAllControlPawn::Steering(const FInputActionValue& Value)
{
	// route the input
	DoSteering(Value.Get<float>());
}

void ATestAllControlPawn::Throttle(const FInputActionValue& Value)
{
	// route the input
	DoThrottle(Value.Get<float>());
}

void ATestAllControlPawn::Brake(const FInputActionValue& Value)
{
	// route the input
	DoBrake(Value.Get<float>());
}

void ATestAllControlPawn::StartBrake(const FInputActionValue& Value)
{
	// route the input
	DoBrakeStart();
}

void ATestAllControlPawn::StopBrake(const FInputActionValue& Value)
{
	// route the input
	DoBrakeStop();
}

void ATestAllControlPawn::StartHandbrake(const FInputActionValue& Value)
{
	// route the input
	DoHandbrakeStart();
}

void ATestAllControlPawn::StopHandbrake(const FInputActionValue& Value)
{
	// route the input
	DoHandbrakeStop();
}

void ATestAllControlPawn::LookAround(const FInputActionValue& Value)
{
	// route the input
	DoLookAround(Value.Get<float>());
}

void ATestAllControlPawn::ToggleCamera(const FInputActionValue& Value)
{
	// route the input
	DoToggleCamera();
}

void ATestAllControlPawn::ResetVehicle(const FInputActionValue& Value)
{
	// route the input
	DoResetVehicle();
}

void ATestAllControlPawn::DoSteering(float SteeringValue)
{
	// add the input
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ATestAllControlPawn::DoThrottle(float ThrottleValue)
{
	// add the input
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);

	// reset the brake input
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ATestAllControlPawn::DoBrake(float BrakeValue)
{
	// add the input
	ChaosVehicleMovement->SetBrakeInput(BrakeValue);

	// reset the throttle input
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void ATestAllControlPawn::DoBrakeStart()
{
	// 记录刹车状态并触发刹车灯逻辑
	bReplicatedBraking = true;
	BrakeLights(true);

	// 若在客户端，通知服务端同步该状态
	if (!HasAuthority())
	{
		ServerSetBrakeState(true);
	}
}

void ATestAllControlPawn::DoBrakeStop()
{
	bReplicatedBraking = false;
	BrakeLights(false);

	if (!HasAuthority())
	{
		ServerSetBrakeState(false);
	}

	// 重置刹车输入
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ATestAllControlPawn::DoHandbrakeStart()
{
	// 启用手刹输入并亮起刹车灯
	ChaosVehicleMovement->SetHandbrakeInput(true);
	bReplicatedBraking = true;
	BrakeLights(true);

	if (!HasAuthority())
	{
		ServerSetBrakeState(true);
	}
}

void ATestAllControlPawn::DoHandbrakeStop()
{
	// 停用手刹输入
	ChaosVehicleMovement->SetHandbrakeInput(false);
	bReplicatedBraking = false;
	BrakeLights(false);

	if (!HasAuthority())
	{
		ServerSetBrakeState(false);
	}
}

void ATestAllControlPawn::DoLookAround(float YawDelta)
{
	// rotate the spring arm
	BackSpringArm->AddLocalRotation(FRotator(0.0f, YawDelta, 0.0f));
}

void ATestAllControlPawn::DoToggleCamera()
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ATestAllControlPawn::DoResetVehicle()
{
	// 客户端请求服务端执行权威重置，避免被服务器物理状态瞬时拉回
	if (!HasAuthority())
	{
		ServerResetVehicle();
		return;
	}

	// 服务端权威重置：重置到当前位置稍高处并消除倾角与物理线/角速度
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
}

bool ATestAllControlPawn::ServerResetVehicle_Validate()
{
	return true;
}

void ATestAllControlPawn::ServerResetVehicle_Implementation()
{
	DoResetVehicle();
}

bool ATestAllControlPawn::ServerSetBrakeState_Validate(bool bBraking)
{
	return true;
}

void ATestAllControlPawn::ServerSetBrakeState_Implementation(bool bBraking)
{
	bReplicatedBraking = bBraking;
	BrakeLights(bBraking);
}

void ATestAllControlPawn::OnRep_Braking()
{
	// 客户端接收服务端同步的刹车状态，驱动尾灯
	BrakeLights(bReplicatedBraking);
}

void ATestAllControlPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATestAllControlPawn, bReplicatedBraking);
}

void ATestAllControlPawn::FlippedCheck()
{
	// check the difference in angle between the mesh's up vector and world up
	const float UpDot = FVector::DotProduct(FVector::UpVector, GetMesh()->GetUpVector());

	if (UpDot < FlipCheckMinDot)
	{
		// is this the second time we've checked that the vehicle is still flipped?
		if (bPreviousFlipCheck)
		{
			// reset the vehicle to upright
			DoResetVehicle();
		}
		
		// set the flipped check flag so the next check resets the car
		bPreviousFlipCheck = true;

	} else {

		// we're upright. reset the flipped check flag
		bPreviousFlipCheck = false;
	}
}

#undef LOCTEXT_NAMESPACE