// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponDefault.h"
#include "DrawDebugHelpers.h"
#include "MainTypes.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

class AStaticMeshActor;
// Sets default values
AWeaponDefault::AWeaponDefault()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RootComponent = SceneComponent;

	SkeletalMeshWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skelital mesh"));
	SkeletalMeshWeapon->SetGenerateOverlapEvents(false);
	SkeletalMeshWeapon->SetCollisionProfileName(TEXT("NoCollision"));
	SkeletalMeshWeapon->SetupAttachment(RootComponent);

	StaticMeshWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	StaticMeshWeapon->SetGenerateOverlapEvents(false);
	StaticMeshWeapon->SetCollisionProfileName(TEXT("NoCollision"));
	StaticMeshWeapon->SetupAttachment(RootComponent);

	ShootLocation = CreateDefaultSubobject<UArrowComponent>(TEXT("ShootLocation"));
	ShootLocation->SetupAttachment(RootComponent);

	ClipSpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("Magazin point"));
	ClipSpawnPoint->SetupAttachment(RootComponent);
	ClipSpawnPoint->SetArrowColor(FColor::Yellow);
	
	ShellSpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SleevBullet point"));
	ShellSpawnPoint->SetupAttachment(RootComponent);
	ShellSpawnPoint->SetArrowColor(FColor::Green);
}

// Called when the game starts or when spawned
void AWeaponDefault::BeginPlay()
{
	Super::BeginPlay();

	WeaponInit();
}

// Called every frame
void AWeaponDefault::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FireTick(DeltaTime);
	ReloadTick(DeltaTime);
	DispersionTick(DeltaTime);
	ClipDropTick(DeltaTime);
	ShellDropTick(DeltaTime);
}

void AWeaponDefault::FireTick(float DeltaTime)
{
	if (GetWeaponRound() > 0)
	{
		if (WeaponFiring)
			if (FireTimer < 0.f)
			{
				if(!WeaponReloading)
				{
					Fire();
				}
			}
			else
				FireTimer -=DeltaTime;
	}
	else
	{
		if (!WeaponReloading)
				InitReload();
	}
}

void AWeaponDefault::ReloadTick(float DeltaTime)
{
	if(WeaponReloading)
	{
		if(ReloadTimer < 0.0f)
		{			
			FinishReload();
		}
		else
		{
			ReloadTimer -= DeltaTime;
		}
		
	}
}

void AWeaponDefault::DispersionTick(float DeltaTime)
{
	if(!WeaponReloading)
	{
		if(!WeaponFiring)
		{
			if (ShouldReduceDispersion)
				CurrentDispersion = CurrentDispersion - CurrentDispersionReduction;
			else
				CurrentDispersion = CurrentDispersion + CurrentDispersionReduction;
		}
		if(CurrentDispersion < CurrentDispersionMin)
			CurrentDispersion = CurrentDispersionMin;
		else
		{
			if (CurrentDispersion > CurrentDispersionMax)
			{
				CurrentDispersion = CurrentDispersionMax;
			}
		}
	}
	if (ShowDebug)
		UE_LOG(LogTemp, Warning, TEXT("Dispersion: MAX = %f, MIN = %f, Current = %f"),
			CurrentDispersionMax, CurrentDispersionMin, CurrentDispersion);
}

void AWeaponDefault::ClipDropTick(float DeltaTime)
{
	if (DropClipFlag)
	{
		if (DropClipTimer < 0.0f)
		{
			DropClipFlag = false;
			ClipDrop();
		}
		else
			DropClipTimer -= DeltaTime;
	}
}

void AWeaponDefault::ShellDropTick(float DeltaTime)
{
	if (DropSheelFlag)
	{
		if (DropSheelTimer < 0.0f)
		{
			DropSheelFlag = false;
			ShellDrop();
		}
		else
			DropSheelTimer -= DeltaTime;
	}
}

void AWeaponDefault::WeaponInit()
{
	if (SkeletalMeshWeapon && !SkeletalMeshWeapon->GetSkinnedAsset())
	{
		SkeletalMeshWeapon->DestroyComponent(true);
	}

	if (StaticMeshWeapon && !StaticMeshWeapon->GetStaticMesh())
	{
		StaticMeshWeapon->DestroyComponent(true);
	}
	
	UpdateWeaponState(EMovementState::Walk_State);
}

void AWeaponDefault::SetWeaponStateFire(bool bIsFire)
{
		if (CheckWeaponCanFire())
			WeaponFiring = bIsFire;
		else
		{
			WeaponFiring = false;
			FireTimer = 0.01f;
		}
}

bool AWeaponDefault::CheckWeaponCanFire()
{
	return !BlockFire; 
}

void AWeaponDefault::Fire()
{
	UAnimMontage* AnimToPlay = nullptr;
	if (WeaponAiming)
		AnimToPlay = WeaponSetting.WeaponAnimation.AnimCharFireAim;
	else
		AnimToPlay = WeaponSetting.WeaponAnimation.AnimCharFire;

	if (WeaponSetting.WeaponAnimation.AnimWeaponFire && SkeletalMeshWeapon
		&& SkeletalMeshWeapon->GetAnimInstance())
	{
		SkeletalMeshWeapon->GetAnimInstance()->Montage_Play(WeaponSetting.WeaponAnimation.AnimWeaponFire);
	}
	
	FireTimer = WeaponSetting.RateOfFire;
	
	ChangeDespersionByShot();

	OnWeaponFireStart.Broadcast(AnimToPlay);
	
	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), WeaponSetting.SoundFireWeapon, ShootLocation->GetComponentLocation());

	if (WeaponSetting.EffectFireWeaponNi)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WeaponSetting.EffectFireWeaponNi,
			ShootLocation->GetComponentLocation(), FRotator::ZeroRotator);
	else if (WeaponSetting.EffectFireWeaponLeg)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), WeaponSetting.EffectFireWeaponLeg, ShootLocation->GetComponentLocation(),
    		FRotator(ShootLocation->GetComponentRotation()));
	
	int8 NumberProjectile = GetNumberProjectileByShoot();
	
	if(ShootLocation)
	{
		FVector SpawnLocation = ShootLocation->GetComponentLocation();
		FRotator SpawnRotation = ShootLocation->GetComponentRotation();
		FProjectileInfo ProjectileInfo;
		ProjectileInfo = GetProjectile();

		FVector EndLocation;
		
		for (int8 i=0;i<NumberProjectile; i++)
		{
			WeaponInfo.Round--;
			
			EndLocation = GetFireEndLocation();
			
			FVector Dir = GetFireEndLocation() - SpawnLocation;

			Dir.Normalize();
			FMatrix myMatrix(Dir, FVector(0,1,0),FVector(0,0,1), FVector::ZeroVector);
			SpawnRotation = myMatrix.Rotator();

			if (ProjectileInfo.Projectile)
			{
				FActorSpawnParameters SpawnParametrs;
				SpawnParametrs.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParametrs.Owner = GetOwner();
				SpawnParametrs.Instigator = GetInstigator();
				
				AProjectileDefault* myProjectile = Cast<AProjectileDefault>
				(GetWorld()->SpawnActor(ProjectileInfo.Projectile, &SpawnLocation, &SpawnRotation, SpawnParametrs));
				if(myProjectile)
				{
					myProjectile->InitProjectile(WeaponSetting.ProjectileSetting);
				}
			}
			else
			{
				TArray<AActor*> IgnoredActor;
				FHitResult HitResoult;
				
				FCollisionQueryParams CollisionParams;
				CollisionParams.AddIgnoredActor(this);
				
				EDrawDebugTrace::Type DebugTrace;
				DebugTrace = EDrawDebugTrace::None;
				
				UKismetSystemLibrary::LineTraceSingle(GetWorld(), SpawnLocation, EndLocation,
					ETraceTypeQuery::TraceTypeQuery_MAX, false, IgnoredActor, DebugTrace, HitResoult, true, FLinearColor::Red,FLinearColor::Green, 5.0f);
				
				/*
				bool Trace = GetWorld()->LineTraceSingleByChannel(HitResoult, SpawnLocation, EndLocation,
					ECollisionChannel::ECC_Visibility, CollisionParams, FCollisionResponseParams());
				*/
				
				if (ProjectileInfo.ScanShootFX && HitResoult.IsValidBlockingHit())
				{
					UNiagaraComponent* ShootFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),
						ProjectileInfo.ScanShootFX, HitResoult.TraceStart, ShootLocation->GetComponentRotation());
					
					if (WeaponSetting.ProjectileSetting.isTraceEndLocation)
					{
						ShootFX->SetVectorParameter(FName (TEXT("EndPointLocation")), HitResoult.ImpactPoint);
					}
				
					
				}
				
				if (ProjectileInfo.HitSound && HitResoult.IsValidBlockingHit())
				{
					UGameplayStatics::SpawnSoundAtLocation(GetWorld(), ProjectileInfo.HitSound, HitResoult.ImpactPoint);
				}
				
				if(ShowDebug)
				{
					FHitResult HitDebug;
					UKismetSystemLibrary::LineTraceSingle(GetWorld(), SpawnLocation, EndLocation,
					ETraceTypeQuery::TraceTypeQuery1, false, IgnoredActor,
					EDrawDebugTrace::ForDuration,HitDebug, true,
					FLinearColor::Green,FLinearColor::Red, 0.5f);
				}
				
				if (HitResoult.GetActor() && HitResoult.PhysMaterial.IsValid())
				{					 
					EPhysicalSurface mySurfacetype = UGameplayStatics::GetSurfaceType(HitResoult);
					
					if (WeaponSetting.ProjectileSetting.HitDecals.Contains(mySurfacetype))
					{
						UMaterialInterface* myMaterial = WeaponSetting.ProjectileSetting.HitDecals[mySurfacetype];

						if (myMaterial && HitResoult.GetComponent())
						{
							UGameplayStatics::SpawnDecalAttached(myMaterial, FVector(20.0f),
								HitResoult.GetComponent(), NAME_None, HitResoult.ImpactPoint, 
								HitResoult.Normal.Rotation(),EAttachLocation::KeepWorldPosition, 10.0f);
						}
					}
					if (WeaponSetting.ProjectileSetting.HitFXLeg.Contains(mySurfacetype) ||
						WeaponSetting.ProjectileSetting.HitFXNi.Contains(mySurfacetype))
					{
						if (!WeaponSetting.ProjectileSetting.HitFXNi.IsEmpty())
						{
							UNiagaraSystem* myParticle = WeaponSetting.ProjectileSetting.HitFXNi[mySurfacetype];
							if(myParticle)
								UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),
									myParticle, HitResoult.ImpactPoint, FRotator::ZeroRotator);
						}
						else 
						{
							UParticleSystem* myParticle = WeaponSetting.ProjectileSetting.HitFXLeg[mySurfacetype];
							if(myParticle)
								UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),myParticle, FTransform
									(HitResoult.ImpactNormal.Rotation(), HitResoult.ImpactPoint, FVector(1.0f)));
						}
					}
				}
				UGameplayStatics::ApplyDamage(HitResoult.GetActor(), ProjectileInfo.ProjectileDamage,
					GetInstigatorController(),this, NULL);
			}
		}
	}
	if(WeaponSetting.ShellDrop.DropMesh)
	{
		if (WeaponSetting.ShellDrop.DropMeshTime < 0.0f)
		{
			ShellDrop();
		}
		else
		{
			DropSheelFlag = true;
			DropSheelTimer = WeaponSetting.ShellDrop.DropMeshTime;
		}
	}
}

FProjectileInfo AWeaponDefault::GetProjectile()
{
	return WeaponSetting.ProjectileSetting;
}

void AWeaponDefault::UpdateWeaponState(EMovementState NewMovementState)
{
	BlockFire = false;
	
	switch (NewMovementState)
	{
	case EMovementState::AimRun_State:
		WeaponAiming = true;
		CurrentDispersionMax = WeaponSetting.DispersionWeapon.AimRun_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.AimRun_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.AimRun_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.AimRun_StateDispersionAimReduction;
		break;
	case EMovementState::AimWalk_State:
		WeaponAiming = true;
		CurrentDispersionMax = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimReduction;
	break;
	case EMovementState::Walk_State:
		WeaponAiming = false;
		CurrentDispersionMax = WeaponSetting.DispersionWeapon.Walk_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.Walk_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.Walk_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.Walk_StateDispersionAimReduction;
	break;
	case EMovementState::Run_State:
		WeaponAiming = false;
		CurrentDispersionMax = WeaponSetting.DispersionWeapon.Run_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.Run_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.Run_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.Run_StateDispersionAimReduction;
	break;
	case EMovementState::SptintRun_State:
		WeaponAiming = false;
		BlockFire = true;
		SetWeaponStateFire(false);
		break;
		default:
			break;
	}
}

void AWeaponDefault::ChangeDespersionByShot()
{
	CurrentDispersion = CurrentDispersion + CurrentDispersionRecoil;
}

float AWeaponDefault::GetCurrentDispersion() const
{
	float Result = CurrentDispersion;
	return Result;
}

FVector AWeaponDefault::ApplyDispersionToShoot(FVector DirectionShoot) const
{
	return FMath::VRandCone(DirectionShoot, GetCurrentDispersion() * PI / 180.f);
}

FVector AWeaponDefault::GetFireEndLocation() const
{
	bool bShootDirection = false;
	FVector EndLocation = FVector(0.f);
	FProjectileInfo ProjectileInfo;

	
FVector tmpV = (ShootLocation->GetComponentLocation() - ShootEndLocation);
	//UE_LOG(LogTemp, Warning, TEXT("Vector: X = %f. Y = %f. Size = %f"), tmpV.X, tmpV.Y, tmpV.Size());
	
//**Change fire target if cursor close the char

	if(tmpV.Size() > SizeVectorToChangeShootDirection)
	{
		EndLocation = ShootLocation->GetComponentLocation() + ApplyDispersionToShoot
		((ShootLocation->GetComponentLocation() - ShootEndLocation).GetSafeNormal()) * -(WeaponSetting.DistantTrace);
		
		if(ShowDebug)
		{
			DrawDebugCone(GetWorld(), ShootLocation->GetComponentLocation(), -(ShootLocation->GetComponentLocation()
			- ShootEndLocation), WeaponSetting.DistantTrace, GetCurrentDispersion()* PI / 180.f,
			GetCurrentDispersion()* PI / 180.f, 32,
			FColor::Emerald, false, .1f, (uint8)'\000', 1.0f);
		}
	}	
	else
	{
		EndLocation = ShootLocation->GetComponentLocation() + ApplyDispersionToShoot(ShootLocation->GetForwardVector()) * (WeaponSetting.DistantTrace);
		
		if (ShowDebug)
		{
			DrawDebugCone(GetWorld(), ShootLocation->GetComponentLocation(), ShootLocation->GetForwardVector(),
				WeaponSetting.DistantTrace, GetCurrentDispersion()* PI / 180.f,
				GetCurrentDispersion()* PI / 180.f, 32,
				FColor::Emerald, false, .1f, (uint8)'\000', 1.0f);
		}
	}
		
	if (ShowDebug)
	{
		//direction weapon look
		DrawDebugLine(GetWorld(), ShootLocation->GetComponentLocation(),
			ShootLocation->GetComponentLocation() + ShootLocation->GetForwardVector() * 500.0f,
			FColor::Cyan, false, 5.f, (uint8)'\000', 0.5f);

		//direction projectile must fly
		DrawDebugLine(GetWorld(), ShootLocation->GetComponentLocation(), ShootEndLocation,
			FColor::Red, false, 5.f, (uint8)'\000', 0.5f);
		
		//Direction Projectile Current fly
		DrawDebugLine(GetWorld(), ShootLocation->GetComponentLocation(),	EndLocation,
			FColor::Black, false, 5.f, (uint8)'\000', 0.5f);
	}

	return EndLocation;
}

int8 AWeaponDefault::GetNumberProjectileByShoot() const
{
	return WeaponSetting.NumberProjectileByShot;
}

int32 AWeaponDefault::GetWeaponRound()
{
	return WeaponInfo.Round;
}

void AWeaponDefault::ClipDrop()
{
	InitDropMesh(WeaponSetting.ClipDrop.DropMesh, ClipSpawnPoint, WeaponSetting.ClipDrop.DropMeshOffset,
		WeaponSetting.ClipDrop.DropMeshImpulseDirection, WeaponSetting.ClipDrop.DropMeshLifeTime,
		WeaponSetting.ClipDrop.ImpulseRandomDispersion, WeaponSetting.ClipDrop.PowerImpulse,
		WeaponSetting.ClipDrop.CustomMass);
}

void AWeaponDefault::InitDropMesh(UStaticMesh* DropMesh, UArrowComponent* ArrowComponent, FTransform Offset,
	FVector DropImpulsDirection, float LifeTimeMesh, float ImpulsRandomDispersion, float PowerImpuls, float CustormMass)
{
	FTransform Transform;

	FVector locDirection = ArrowComponent->GetComponentLocation();
	Transform.SetLocation(locDirection);
	Transform.SetRotation(ArrowComponent->GetComponentRotation().Quaternion() + Offset.Rotator().Quaternion());


	AStaticMeshActor* NewActor = nullptr;
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.Instigator = GetInstigator();
	
	NewActor = (GetWorld()->SpawnActor<AStaticMeshActor>
		(AStaticMeshActor::StaticClass(), Transform.GetLocation(),
			Transform.GetRotation().Rotator(), SpawnParameters));

	if(NewActor && NewActor->GetStaticMeshComponent())
	{
		NewActor->InitialLifeSpan = LifeTimeMesh;
		
		NewActor->GetStaticMeshComponent()->SetRelativeScale3D(Offset.GetScale3D());
		NewActor->GetStaticMeshComponent()->SetCollisionResponseToChannel
			(ECollisionChannel::ECC_Pawn, ECR_Ignore);
		NewActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		NewActor->GetStaticMeshComponent()->SetStaticMesh(DropMesh);
		NewActor->GetStaticMeshComponent()->SetSimulatePhysics(true);
		
		if (CustormMass > 0.0f)
		{
			NewActor->GetStaticMeshComponent()->SetMassOverrideInKg(NAME_None, CustormMass, true);
		}

		//**Repair impuls
		//***Add Direction
		FVector finDirection;
		finDirection = ArrowComponent->GetForwardVector();
		NewActor->GetStaticMeshComponent()->AddImpulse(finDirection * PowerImpuls);
		
		if (!FMath::IsNearlyZero(ImpulsRandomDispersion))
			{
				finDirection += UKismetMathLibrary::RandomUnitVectorInConeInDegrees
				(locDirection, ImpulsRandomDispersion);
				//finDirection.GetSafeNormal(0.0001f);

				NewActor->GetStaticMeshComponent()->AddImpulse(finDirection * PowerImpuls);
			}
		
	}
}

void AWeaponDefault::ShellDrop()
{
	InitDropMesh(WeaponSetting.ShellDrop.DropMesh, ShellSpawnPoint, WeaponSetting.ShellDrop.DropMeshOffset,
	WeaponSetting.ShellDrop.DropMeshImpulseDirection, WeaponSetting.ShellDrop.DropMeshLifeTime,
	WeaponSetting.ShellDrop.ImpulseRandomDispersion, WeaponSetting.ShellDrop.PowerImpulse,
	WeaponSetting.ShellDrop.CustomMass);
}

void AWeaponDefault::InitReload()
{
	UAnimMontage* AnimCharToPlay = nullptr;
	UAnimMontage* AnimWeaponToPlay = nullptr;
	
	if (WeaponReloading == false)
		{
			if (WeaponAiming)
				AnimCharToPlay = WeaponSetting.WeaponAnimation.AnimCharReloadAim;
			else
				AnimCharToPlay = WeaponSetting.WeaponAnimation.AnimCharReload;
		
				OnWeaponReloadStart.Broadcast(AnimCharToPlay);
		
			if (WeaponAiming)
				AnimWeaponToPlay = WeaponSetting.WeaponAnimation.AnimWeaponReloadAim;
			else
				AnimWeaponToPlay = WeaponSetting.WeaponAnimation.AnimWeaponReload;

			if (WeaponSetting.WeaponAnimation.AnimCharReload && SkeletalMeshWeapon && SkeletalMeshWeapon->GetAnimInstance())
			{
				SkeletalMeshWeapon->GetAnimInstance()->Montage_Play(AnimWeaponToPlay);
			}
		
					if (WeaponSetting.SoundReloadWeapon)
						UGameplayStatics::SpawnSoundAtLocation(GetWorld(), WeaponSetting.SoundReloadWeapon,
							ShootLocation->GetComponentLocation());
					
					ReloadTimer = WeaponSetting.ReloadTime;
					
					if(WeaponSetting.ClipDrop.DropMesh)
					{
						DropClipFlag = true;
						DropClipTimer = WeaponSetting.ClipDrop.DropMeshTime;
					}
		}
	WeaponReloading = true;
}

void AWeaponDefault::FinishReload()
{
	WeaponReloading = false;
	WeaponInfo.Round = WeaponSetting.MaxRound;

	OnWeaponReloadEnd.Broadcast();
}



