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

	MagazineSpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("Magazin point"));
	MagazineSpawnPoint->SetupAttachment(RootComponent);
	MagazineSpawnPoint->SetArrowColor(FColor::Yellow);
	
	SleevBulletSpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SleevBullet point"));
	SleevBulletSpawnPoint->SetupAttachment(RootComponent);
	SleevBulletSpawnPoint->SetArrowColor(FColor::Green);
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
	
	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), WeaponSetting.SoundFireWeapon, ShootLocation->GetComponentLocation());
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), WeaponSetting.EffectFireWeapon, ShootLocation->GetComponentLocation(),
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
						ProjectileInfo.ScanShootFX, HitResoult.TraceStart, FRotator::ZeroRotator);
					ShootFX->SetVectorParameter(FName (TEXT("EndPointLocation")), HitResoult.ImpactPoint);
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
				
				
				//***************
				/*UPhysicalMaterial* Pmat = HitResoult.PhysMaterial.Get();
				AActor* Actor = HitResoult.GetActor();
				bool Pmatd = HitResoult.PhysMaterial.IsValid();
				UE_LOG(LogTemp, Warning, TEXT("AWeaponDefault::Fire  -Actor name: %s, Surface type: %i"), *Actor->GetName(), Pmatd);
				//****************/
				
				
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
								HitResoult.ImpactPoint.Rotation(),EAttachLocation::KeepWorldPosition, 10.0f);
						}
					}
					if (WeaponSetting.ProjectileSetting.HitFXLeg.Contains(mySurfacetype) || WeaponSetting.ProjectileSetting.HitFXNi.Contains(mySurfacetype))
					{
						if (!WeaponSetting.ProjectileSetting.HitFXLeg.IsEmpty())
						{
							UParticleSystem* myParticle = WeaponSetting.ProjectileSetting.HitFXLeg[mySurfacetype];
							if(myParticle)
								UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),myParticle, FTransform
									(HitResoult.ImpactNormal.Rotation(), HitResoult.ImpactPoint, FVector(1.0f)));
						}
						if (!WeaponSetting.ProjectileSetting.HitFXNi.IsEmpty())
						{
							UNiagaraSystem* myParticle = WeaponSetting.ProjectileSetting.HitFXNi[mySurfacetype];
							if(myParticle)
								UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),
									myParticle, HitResoult.ImpactPoint, FRotator::ZeroRotator);
						}
					}
				}
				UGameplayStatics::ApplyDamage(HitResoult.GetActor(), ProjectileInfo.ProjectileDamage,
					GetInstigatorController(),this, NULL);
			}
		}
	}
	if(WeaponSetting.WeaponDrop.SheelMesh)
	{
		if (WeaponSetting.WeaponDrop.SheelDropTime < 0.0f)
		{
			ShellDrop();
		}
		else
		{
			DropSheelFlag = true;
			DropSheelTimer = WeaponSetting.WeaponDrop.SheelDropTime;
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
		FActorSpawnParameters SpawnParametrs;
    	SpawnParametrs.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    	SpawnParametrs.Owner = GetOwner();
    	SpawnParametrs.Instigator = GetInstigator();
    	
    	FVector SpawnLocation = MagazineSpawnPoint->GetComponentLocation();
    	FRotator SpawnRotation = MagazineSpawnPoint->GetComponentRotation();
    	
    	AStaticMeshActor* SpawnedMagazine = (GetWorld()->SpawnActor<AStaticMeshActor>
    		(AStaticMeshActor::StaticClass(), SpawnLocation, SpawnRotation, SpawnParametrs));
    	if (SpawnedMagazine)
    	{
    		SpawnedMagazine->SetMobility(EComponentMobility::Movable);
    		UStaticMeshComponent* comp = SpawnedMagazine->GetStaticMeshComponent();
    		comp->SetStaticMesh(WeaponSetting.WeaponDrop.ClipMesh);
    		comp->SetSimulatePhysics(true);
    		comp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Ignore);
    	}
}

void AWeaponDefault::ShellDrop()
{
			FActorSpawnParameters SpawnParametrs;
			SpawnParametrs.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParametrs.Owner = GetOwner();
			SpawnParametrs.Instigator = GetInstigator();

			FVector SpawnLocation = SleevBulletSpawnPoint->GetComponentLocation();
			FRotator SpawnRotation = SleevBulletSpawnPoint->GetComponentRotation() + FRotator(0,0,-90);

			float ImpulstMultiply;
			ImpulstMultiply = FMath::FRandRange(2.0f,4.0f);
		
			FVector ForwardVector = SleevBulletSpawnPoint->GetForwardVector();

			AStaticMeshActor* SleevBullet = (GetWorld()->SpawnActor<AStaticMeshActor>
				(AStaticMeshActor::StaticClass(), SpawnLocation, SpawnRotation, SpawnParametrs));
			if (SleevBullet)
			{
				SleevBullet->SetMobility(EComponentMobility::Movable);
				SleevBullet->InitialLifeSpan = 1.0f;
				SleevBullet->SetActorEnableCollision(true);
			
				UStaticMeshComponent* comp = SleevBullet->GetStaticMeshComponent();
				if (comp)
				{
					comp->SetStaticMesh(WeaponSetting.WeaponDrop.SheelMesh);
					comp->SetSimulatePhysics(true);
					comp->AddImpulse(ForwardVector * ImpulstMultiply);
					comp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Ignore);
				}
			}
}

void AWeaponDefault::InitReload()
{
	
	
	if (WeaponReloading == false)
		{
			if (WeaponSetting.WeaponAnimation.AnimCharReload)
				OnWeaponReloadStart.Broadcast(WeaponSetting.WeaponAnimation.AnimCharReload);
	
					if (WeaponSetting.SoundReloadWeapon)
						UGameplayStatics::SpawnSoundAtLocation(GetWorld(), WeaponSetting.SoundReloadWeapon,
							ShootLocation->GetComponentLocation());
					
					ReloadTimer = WeaponSetting.ReloadTime;
					
					if(WeaponSetting.WeaponDrop.ClipMesh)
					{
						DropClipFlag = true;
						DropClipTimer = WeaponSetting.WeaponDrop.ClipDropTime;
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

