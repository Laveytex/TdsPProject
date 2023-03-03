// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponDefault.h"


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

}

void AWeaponDefault::FireTick(float DeltaTime)
{
	if (WeaponFiring)
		if (FireTime < 0.f)
			Fire();
	else
		FireTime -=DeltaTime;

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
}

void AWeaponDefault::SetWeaponStateFire(bool bIsFire)
{
		if (CheckWeaponCanFire())
			WeaponFiring = bIsFire;
		else
			WeaponFiring = false;
}

bool AWeaponDefault::CheckWeaponCanFire()
{
	return true; 
}

void AWeaponDefault::Fire()
{
	FireTime = WeaponSetting. RateOfFire;

	if(ShootLocation)
	{
		FVector SpawnLocation = ShootLocation->GetComponentLocation();
		FRotator SpawnRotation = ShootLocation->GetComponentRotation();
		FProjectileInfo ProjectileInfo;
		ProjectileInfo = GetProjectile();

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
				myProjectile->InitialLifeSpan = 20.0f;
			}
		}

	}
}

FProjectileInfo AWeaponDefault::GetProjectile()
{
	return WeaponSetting.ProjectileSetting;
}

void AWeaponDefault::UpdateWeaponState(EMovementState NewMovementState)
{
	ChangeDespersion();
}

void AWeaponDefault::ChangeDespersion()
{
}

