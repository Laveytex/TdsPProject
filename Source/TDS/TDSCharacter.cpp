// Copyright Epic Games, Inc. All Rights Reserved.

#include "TDSCharacter.h"

#include "TDSGameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Chaos/ChaosGameplayEventDispatcher.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "WeaponDefault.h"
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

	//Add cursor
	if(CurrentCursor)
	{
		APlayerController* myPC = Cast<APlayerController>(GetController());
		if(myPC)
		{
			FHitResult TraceHitResult;
			myPC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();

			CurrentCursor->SetWorldLocation(TraceHitResult.Location);
			CurrentCursor->SetWorldRotation(CursorR);
		}
	}

	MovementTick(DeltaSeconds);
}

void ATDSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ATDSCharacter::InputAxisX);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ATDSCharacter::InputAxisY);

	PlayerInputComponent->BindAction(TEXT("FireEvent"),
		EInputEvent::IE_Pressed, this, &ATDSCharacter::InputAttackPressed);
	PlayerInputComponent->BindAction(TEXT("FireEvent"),
		EInputEvent::IE_Released, this, &ATDSCharacter::InputAttackReleased);
	PlayerInputComponent->BindAction(TEXT("ReloadEvent"),
		EInputEvent::IE_Pressed, this, &ATDSCharacter::TryReloadWeapon);
	
}

void ATDSCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitWeapon(InitWeaponName);

	if(CursorMaterial)
	{
		CurrentCursor = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CursorMaterial, CursorSize, FVector(0));
	}

	CharacterUpdate();
}

void ATDSCharacter::InputAxisX(float Value)
{
	AxisX = Value;
}

void ATDSCharacter::InputAxisY(float Value)
{
	AxisY = Value;
}

void ATDSCharacter::InputAttackPressed()
{
	AttackCharEvent(true);
}

void ATDSCharacter::InputAttackReleased()
{
	AttackCharEvent(false);
}

void ATDSCharacter::AttackCharEvent(bool bIsFiring)
{
	AWeaponDefault* myWeapon = nullptr;
	myWeapon = GetCurrentWeapon();
	if(myWeapon)
	{
		myWeapon->SetWeaponStateFire(bIsFiring);
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("TDSCharacter::AttackCharEvent - CurrWeapon - NULL"));
	
}

void ATDSCharacter::MovementTick(float DeltaTime)
{
	AddMovementInput(FVector(1.0f, 0.0f,0.0f), AxisX);
	AddMovementInput(FVector(0.0f, 1.0f,0.0f), AxisY);

	APlayerController* myController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if(myController)
	{
		FHitResult ResultHit;
		//myController->GetHitResultUnderCursorByChannel(ETraceTypeQuery::TraceTypeQuery6, false, ResultHit);
		myController->GetHitResultUnderCursor(ECC_GameTraceChannel1, true, ResultHit);
		
		float targetRotationYaw = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ResultHit.Location).Yaw;
		SetActorRotation(FQuat(FRotator(0.0f,targetRotationYaw,0.0f)));

		if(CurrentWeapon)
		{
			FVector Displacment = FVector(0);
			switch (MovementState)
			{
			case EMovementState::AimWalk_State:
				Displacment = FVector(0.0f,0.0f,160.0f);
				break;
			case EMovementState::AimRun_State:
				Displacment = FVector(0.0f,0.0f,160.0f);
				break;
			case EMovementState::Walk_State:
				Displacment = FVector(0.0f,0.0f,120.0f);
				break;
			case EMovementState::Run_State:
				Displacment = FVector(0.0f,0.0f,120.0f);
				break;
			case EMovementState::SptintRun_State:
				break;
			default:
				break;
			}
			CurrentWeapon->ShootEndLocation = ResultHit.Location + Displacment;
		}
	}
	if (CurrentWeapon)
		if (FMath::IsNearlyZero(GetVelocity().Size(), 0.5f))
			CurrentWeapon->ShouldReduceDispersion = true;
		else
			CurrentWeapon->ShouldReduceDispersion = false;
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
	CharacterUpdate();

	//Weapon state update
	AWeaponDefault* myWeapon = GetCurrentWeapon();
	if (myWeapon)
	{
		myWeapon->UpdateWeaponState(MovementState);
	}
}

AWeaponDefault* ATDSCharacter::GetCurrentWeapon()
{
	return CurrentWeapon;
}

void ATDSCharacter::InitWeapon(FName IDWeaponName)
{
	UTDSGameInstance* myGI = Cast<UTDSGameInstance>(GetGameInstance());
	FWeaponInfo myWeaponInfo;
	if (myGI)
	{
		if(myGI->GetWeaponInfoByName(IDWeaponName, myWeaponInfo))
		{
			if(myWeaponInfo.WeaponClass)
			{
				FVector SpawnLocation = FVector(0);
				FRotator SpawnRotation = FRotator(0);

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParams.Owner = GetOwner();
				SpawnParams.Instigator = GetInstigator();

				AWeaponDefault* myWeapon = Cast<AWeaponDefault>
				(GetWorld()->SpawnActor(myWeaponInfo.WeaponClass, &SpawnLocation, &SpawnRotation, SpawnParams));
				if(myWeapon)
				{
					FAttachmentTransformRules Rule(EAttachmentRule::SnapToTarget, false);
					myWeapon->AttachToComponent(GetMesh(), Rule, FName("WeaponSocketRightHand"));
					CurrentWeapon = myWeapon;

					myWeapon->WeaponSetting = myWeaponInfo;
					myWeapon->WeaponInfo.Round = myWeaponInfo.MaxRound;
					//Debug
					myWeapon->ReloadTime = myWeaponInfo.ReloadTime;
					myWeapon->UpdateWeaponState(MovementState);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("TDSCharacter::InitWeapon - Weapon not found in table"));
			}
		}
	}
	
}

void ATDSCharacter::TryReloadWeapon()
{
	if (CurrentWeapon)
	{
		if (CurrentWeapon->GetWeaponRound() <= CurrentWeapon->WeaponSetting.MaxRound)
		{
			CurrentWeapon->InitReload();
		}
	}
	
}

UDecalComponent* ATDSCharacter::GetCursorToWorld()
{
	return CurrentCursor;
}
