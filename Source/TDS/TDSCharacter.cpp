// Copyright Epic Games, Inc. All Rights Reserved.

#include "TDSCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Chaos/ChaosGameplayEventDispatcher.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

ATDSCharacter::ATDSCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ATDSCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	MovementTick(DeltaSeconds);
}

void ATDSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ATDSCharacter::ImputAxisX);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ATDSCharacter::ImputAxisY);
	
}

void ATDSCharacter::BeginPlay()
{
	Super::BeginPlay();

	CharacterUpdate();
}

void ATDSCharacter::ImputAxisX(float Value)
{
	AxisX = Value;
}

void ATDSCharacter::ImputAxisY(float Value)
{
	AxisY = Value;
}

void ATDSCharacter::MovementTick(float DeltaTime)
{
	AddMovementInput(FVector(1.0f, 0.0f,0.0f), AxisX);
	AddMovementInput(FVector(0.0f, 1.0f,0.0f), AxisY);

	APlayerController* myController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if(myController)
	{
		FHitResult ResultHit;
		myController->GetHitResultUnderCursorByChannel(ETraceTypeQuery::TraceTypeQuery6, false, ResultHit);
		float targetRotationYaw = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ResultHit.Location).Yaw;
		SetActorRotation(FQuat(FRotator(0,targetRotationYaw,0)));
	}
}

void ATDSCharacter::CharacterUpdate()
{
	float ResSpeed = 600.0f;
	switch (MovementState)
	{
	case EMovementState::AimRun_State:
			ResSpeed = MovementSpeedInfo.AimRunState;
		break;
	case EMovementState::AimWalk_State:
			ResSpeed = MovementSpeedInfo.AimWalkSpeed;
		break;
	case EMovementState::Walk_State:
			ResSpeed = MovementSpeedInfo.WalkSpeed;
		break;
	case EMovementState::Run_State:
			ResSpeed = MovementSpeedInfo.RunSpeed;
		break;
	case EMovementState::SptintRun_State:
			ResSpeed = MovementSpeedInfo.SprintSpeed;
		break;
	case EMovementState::Crouch_State:
			ResSpeed = MovementSpeedInfo.CrouchSpeed;
		break;
	case EMovementState::AimCrouch_State:
			ResSpeed = MovementSpeedInfo.CrouchAimSpeed;
		break;
		default:
			break;
	}
	GetCharacterMovement()->MaxWalkSpeed = ResSpeed;
}

void ATDSCharacter::ChangeMovementState()
{
	if (!WalkEnabled && !SprintRunEnabled && !AimEnabled && !CroucnEnabled)
	{
		MovementState = EMovementState::Run_State;
	}
	else
	{
		if (SprintRunEnabled)
		{
			WalkEnabled = false;
			AimEnabled = false;
			CroucnEnabled = false;
			MovementState = EMovementState::SptintRun_State;
		}
		if (WalkEnabled && !SprintRunEnabled && AimEnabled && !CroucnEnabled)
		{
			MovementState = EMovementState::AimWalk_State;
		}
		else
		{
			if (WalkEnabled && !SprintRunEnabled && !AimEnabled && !CroucnEnabled)
			{
				MovementState = EMovementState::Walk_State;
			}
			if (!WalkEnabled && !SprintRunEnabled && AimEnabled && !CroucnEnabled)
			{
				MovementState = EMovementState::AimRun_State;
			}
			else
			{
				if (!WalkEnabled && !SprintRunEnabled && !AimEnabled && CroucnEnabled)
				{
					SprintRunEnabled = false;
					WalkEnabled = false;
					MovementState = EMovementState::Crouch_State;
				}
				if (!WalkEnabled && !SprintRunEnabled && AimEnabled && CroucnEnabled)
				{
					SprintRunEnabled = false;
					WalkEnabled = false;
					MovementState = EMovementState::AimCrouch_State;
				}
			}
		}
		
	}
	//MovementState = NewMovementState;
	CharacterUpdate();
}
