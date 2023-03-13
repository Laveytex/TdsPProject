// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileDefault_Greanade.h"

#include "Kismet/GameplayStatics.h"

void AProjectileDefault_Greanade::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectileDefault_Greanade::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TimerExplose(DeltaSeconds);
}

void AProjectileDefault_Greanade::TimerExplose(float DeltaSeconds)
{
		if (TimerEnabled)
		{
			if (TimerToExplose > ProjectileSetting.TimeToExlose)
			{
				Explose();
			}
			else
			{
				TimerToExplose += DeltaSeconds;
			}
		}
}

void AProjectileDefault_Greanade::BulletCollisionSphereHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::BulletCollisionSphereHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileDefault_Greanade::ImpactProjectile()
{
		TimerEnabled = true;
}

void AProjectileDefault_Greanade::Explose()
{
	TimerEnabled = false;
	if (ProjectileSetting.ExploseFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ProjectileSetting.ExploseFX,
			GetActorLocation(), GetActorRotation(), FVector(1.0f));
	}
	if (ProjectileSetting.ExploseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ProjectileSetting.ExploseSound, GetActorLocation());
	}

	TArray<AActor*> IgnoredActor;

	UGameplayStatics::ApplyRadialDamageWithFalloff(GetWorld(), ProjectileSetting.ExploseDamage,
		ProjectileSetting.ExploseDamage*ProjectileSetting.ExploseReductionFactorToMinDamage,
		GetActorLocation(), ProjectileSetting.ExploseInnerRadius,
		ProjectileSetting.ExploseOuterRadius, ProjectileSetting.ExploseDamageFalloff,
		NULL, IgnoredActor, nullptr, nullptr);

	if (ProjectileSetting.ShowDebug)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), ProjectileSetting.ExploseInnerRadius, 8, FColor::Red, true, -1, 0, 2);
		DrawDebugSphere(GetWorld(), GetActorLocation(), ProjectileSetting.ExploseOuterRadius, 16, FColor::Yellow, true, -1, 0, 2);
	}
	
	this->Destroy();
}
