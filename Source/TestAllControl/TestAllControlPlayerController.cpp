// Copyright Epic Games, Inc. All Rights Reserved.


#include "TestAllControlPlayerController.h"
#include "TestAllControlPawn.h"
#include "TestAllControlUI.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "TestAllControl.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ATestAllControlPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// ensure we're attached to the vehicle pawn so that World Partition streaming works correctly
	bAttachToPawn = true;
}

void ATestAllControlPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}

			if (bUseSteeringWheelControls)
			{
				Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 0);
			}
		}
	}

	// only spawn UI on local player controllers
	if (IsLocalPlayerController())
	{
		if (ShouldUseTouchControls())
		{
			// spawn the mobile controls widget
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

			if (MobileControlsWidget)
			{
				// add the controls to the player screen
				MobileControlsWidget->AddToPlayerScreen(0);

			} else {

				UE_LOG(LogTestAllControl, Error, TEXT("Could not spawn mobile controls widget."));

			}
		}
		

		// spawn the UI widget and add it to the viewport
		VehicleUI = CreateWidget<UTestAllControlUI>(this, VehicleUIClass);

		if (VehicleUI)
		{
			VehicleUI->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogTestAllControl, Error, TEXT("Could not spawn vehicle UI widget."));

		}
	}
}

void ATestAllControlPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	// 若 VehiclePawn 为空，尝试动态从当前拥有的 Pawn 恢复（保障客户端连接初期即时就绪）
	if (!IsValid(VehiclePawn))
	{
		VehiclePawn = Cast<ATestAllControlPawn>(GetPawn());
	}

	if (IsValid(VehiclePawn) && IsValid(VehicleUI) && VehiclePawn->GetChaosVehicleMovement())
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
}

void ATestAllControlPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 权威端缓存受控车辆指针
	VehiclePawn = CastChecked<ATestAllControlPawn>(InPawn);

	// 监听车辆销毁代理
	VehiclePawn->OnDestroyed.AddUniqueDynamic(this, &ATestAllControlPlayerController::OnPawnDestroyed);
}

void ATestAllControlPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);

	// 客户端确认拥有 Pawn：缓存 VehiclePawn 指针以驱动客户端仪表盘 UI
	VehiclePawn = Cast<ATestAllControlPawn>(InPawn);
	if (VehiclePawn)
	{
		VehiclePawn->OnDestroyed.AddUniqueDynamic(this, &ATestAllControlPlayerController::OnPawnDestroyed);
	}
}

void ATestAllControlPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	// 客户端同步到新 Pawn 时更新引用
	VehiclePawn = Cast<ATestAllControlPawn>(GetPawn());
}

void ATestAllControlPlayerController::OnPawnDestroyed(AActor* DestroyedPawn)
{
	// 仅在权威端（服务器）执行重生逻辑，客户端无权生成 Actor
	if (!HasAuthority())
	{
		return;
	}

	// 查找起跑点并重新生成车辆
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{
		const FTransform SpawnTransform = ActorList[0]->GetActorTransform();

		if (ATestAllControlPawn* RespawnedVehicle = GetWorld()->SpawnActor<ATestAllControlPawn>(VehiclePawnClass, SpawnTransform))
		{
			Possess(RespawnedVehicle);
		}
	}
}

bool ATestAllControlPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
