// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SBCharacterAnimInstance.generated.h"

class ACharacter;

/**
 * 
 */
UCLASS()
class SPACEBLASTER_API USBCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Character")
	ACharacter* Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Character")
	float MovementSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Character")
	float Direction;

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
