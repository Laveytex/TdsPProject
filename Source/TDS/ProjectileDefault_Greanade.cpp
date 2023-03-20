// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileDefault_Greanade.h"

#include "NiagaraFunctionLibrary.h"
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

	if (ProjectileSetting.ExploseFXNi)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),
			ProjectileSetting.ExploseFXNi, GetActorLocation(), FRotator::ZeroRotator);
	}
	if (ProjectileSetting.ExploseFXLeg)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ProjectileSetting.ExploseFXLeg,
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
		ProjectileSetting.ExploseOuterRadius, ProjectileSetting.ExploseDamageFalloff, NULL,
		IgnoredActor, this, nullptr);
	
	if (ProjectileSetting.ShowDebug)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), ProjectileSetting.ExploseInnerRadius, 8, FColor::Red, true, 0.1f, 0, 0);
		DrawDebugSphere(GetWorld(), GetActorLocation(), ((ProjectileSetting.ExploseInnerRadius+ProjectileSetting.ExploseOuterRadius)/2), 8, FColor::Yellow, true, 0.1f, 0, 0);
		DrawDebugSphere(GetWorld(), GetActorLocation(), ProjectileSetting.ExploseOuterRadius, 16, FColor::White, true, 0.1f, 0, 0);
	}
	
	this->Destroy();
}
