// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MainTypes.h"
#include "GameFramework/Character.h"
#include "TDSCharacter.generated.h"

UCLASS(Blueprintable)
class ATDSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATDSCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	EMovementState MovementState = EMovementState::Run_State;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FCharacterSpeed MovementSpeedInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool SprinRunEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool WalkEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool AimEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool CroucnEnabled = false;
	
	UFUNCTION()
	void ImputAxisX(float Value);
	UFUNCTION()
	void ImputAxisY(float Value);

	float AxisX = 0.0f;
	float AxisY = 0.0f;

	// Tick Func
	UFUNCTION()
	void MovementTick(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void CharacterUpdate();
	UFUNCTION(BlueprintCallable)
	void ChangeMovementState();
};

