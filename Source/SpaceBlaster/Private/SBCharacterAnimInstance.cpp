// Fill out your copyright notice in the Description page of Project Settings.


#include "SBCharacterAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "GameFramework/Character.h"


void USBCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	Character = Cast<ACharacter>(GetOwningActor());
}

void USBCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (Character)
	{
		MovementSpeed = Character->GetVelocity().Length();
		Direction = UKismetAnimationLibrary::CalculateDirection(Character->GetVelocity(),
		                                                        Character->GetActorRotation());
	}
}
