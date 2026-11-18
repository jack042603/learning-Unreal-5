// Fill out your copyright notice in the Description page of Project Settings.

#include "SoullikeComboWindowNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Soullike2Character.h"

void USoullikeComboWindowNotifyState::NotifyBegin(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    float TotalDuration,
    const FAnimNotifyEventReference& EventReference
)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!IsValid(MeshComp))
    {
        return;
    }

    if (ASoullike2Character* Character = Cast<ASoullike2Character>(MeshComp->GetOwner()))
    {
        UE_LOG(LogTemp, Display, TEXT("[Combo Notify] Begin: %s, Duration: %.3f."), *GetNameSafe(Animation), TotalDuration);
        Character->SetComboInputWindow(true);
    }
}

void USoullikeComboWindowNotifyState::NotifyEnd(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference
)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!IsValid(MeshComp))
    {
        return;
    }

    if (ASoullike2Character* Character = Cast<ASoullike2Character>(MeshComp->GetOwner()))
    {
        UE_LOG(LogTemp, Display, TEXT("[Combo Notify] End: %s."), *GetNameSafe(Animation));
        Character->SetComboInputWindow(false);
    }
}

