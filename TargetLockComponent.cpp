// Fill out your copyright notice in the Description page of Project Settings.

// Fill out your copyright notice in the Description page of Project Settings.

#include "TargetLockComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

UTargetLockComponent::UTargetLockComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTargetLockComponent::BeginPlay()
{
    Super::BeginPlay();

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    UE_LOG(LogTemp, Warning,
        TEXT("[TargetLock-Debug] Component BeginPlay. Owner=%s, OwnerClass=%s, Controller=%s, Radius=%.0f"),
        *GetNameSafe(OwnerCharacter),
        *GetNameSafe(GetOwner() ? GetOwner()->GetClass() : nullptr),
        *GetNameSafe(OwnerCharacter ? OwnerCharacter->GetController() : nullptr),
        LockOnRadius);
}

void UTargetLockComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Unlock();
    Super::EndPlay(EndPlayReason);
}

void UTargetLockComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsLockedOn())
    {
        return;
    }

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    ACharacter* TargetCharacter = CurrentTarget.Get();

    if (!IsValid(OwnerCharacter) || !IsValid(TargetCharacter))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TargetLock] Target or owner became invalid. Unlocking."));
        Unlock();
        return;
    }

    if (bUnlockWhenTargetOutOfRange)
    {
        const float DistanceSquared = FVector::DistSquared(
            OwnerCharacter->GetActorLocation(), TargetCharacter->GetActorLocation());
        if (DistanceSquared > FMath::Square(LockOnRadius))
        {
            UE_LOG(LogTemp, Warning, TEXT("[TargetLock] Target moved out of range. Unlocking."));
            Unlock();
            return;
        }
    }

    UpdateLockedCamera(DeltaTime);
}

void UTargetLockComponent::ToggleLockOn()
{
    UE_LOG(LogTemp, Warning, TEXT("[TargetLock-Debug] ToggleLockOn called. CurrentTarget=%s"),
        *GetNameSafe(CurrentTarget.Get()));

    if (IsLockedOn())
    {
        Unlock();
        UE_LOG(LogTemp, Display, TEXT("[TargetLock] Unlocked."));
        return;
    }

    LockOn();
}

bool UTargetLockComponent::LockOn()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Error, TEXT("[TargetLock-Debug] LockOn failed: Owner is not a valid ACharacter. Owner=%s"),
            *GetNameSafe(GetOwner()));
        return false;
    }

    if (!IsValid(OwnerCharacter->GetController()))
    {
        UE_LOG(LogTemp, Error, TEXT("[TargetLock-Debug] LockOn failed: Owner has no Controller. Owner=%s"),
            *GetNameSafe(OwnerCharacter));
        return false;
    }

    if (IsLockedOn())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TargetLock-Debug] LockOn called while already locked."));
        return true;
    }

    ACharacter* NewTarget = FindNearestTarget();
    if (!IsValid(NewTarget))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TargetLock] No other Character found within %.0f units of %s."),
            LockOnRadius, *GetNameSafe(OwnerCharacter));
        return false;
    }

    CurrentTarget = NewTarget;
    ApplyRaisedCameraBoom();
    ApplyLockedCharacterSettings();

    UE_LOG(LogTemp, Display, TEXT("[TargetLock] Locked target: %s (distance %.0f units)."),
        *GetNameSafe(NewTarget),
        FVector::Distance(OwnerCharacter->GetActorLocation(), NewTarget->GetActorLocation()));
    return true;
}

void UTargetLockComponent::Unlock()
{
    CurrentTarget.Reset();
    RestoreCharacterSettings();
    RestoreCameraBoom();
}

bool UTargetLockComponent::IsLockedOn() const
{
    return CurrentTarget.IsValid();
}

AActor* UTargetLockComponent::GetCurrentTarget() const
{
    return CurrentTarget.Get();
}

ACharacter* UTargetLockComponent::FindNearestTarget() const
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    UWorld* World = GetWorld();
    if (!IsValid(OwnerCharacter) || !World)
    {
        UE_LOG(LogTemp, Error, TEXT("[TargetLock-Debug] FindNearestTarget failed: Owner or World is invalid."));
        return nullptr;
    }

    ACharacter* BestTarget = nullptr;
    float BestDistanceSquared = FMath::Square(LockOnRadius);
    int32 OtherCharacterCount = 0;

    for (TActorIterator<ACharacter> It(World); It; ++It)
    {
        ACharacter* Candidate = *It;
        if (!IsValid(Candidate) || Candidate == OwnerCharacter)
        {
            continue;
        }

        ++OtherCharacterCount;
        const float DistanceSquared = FVector::DistSquared(
            OwnerCharacter->GetActorLocation(), Candidate->GetActorLocation());
        const float Distance = FMath::Sqrt(DistanceSquared);

        UE_LOG(LogTemp, Warning,
            TEXT("[TargetLock-Debug] Candidate=%s, Class=%s, Distance=%.0f, InRange=%s"),
            *GetNameSafe(Candidate), *GetNameSafe(Candidate->GetClass()), Distance,
            DistanceSquared <= FMath::Square(LockOnRadius) ? TEXT("true") : TEXT("false"));

        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestTarget = Candidate;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[TargetLock-Debug] Other Character count=%d. Selected=%s"),
        OtherCharacterCount, *GetNameSafe(BestTarget));

    return BestTarget;
}

void UTargetLockComponent::UpdateLockedCamera(float DeltaTime)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    ACharacter* TargetCharacter = CurrentTarget.Get();
    if (!IsValid(OwnerCharacter) || !IsValid(TargetCharacter))
    {
        return;
    }

    AController* OwnerController = OwnerCharacter->GetController();
    if (!IsValid(OwnerController))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TargetLock-Debug] Camera update skipped: Controller invalid."));
        return;
    }

    FVector LookOrigin = OwnerCharacter->GetPawnViewLocation();
    if (USpringArmComponent* Boom = CameraBoom.Get())
    {
        LookOrigin = Boom->GetComponentLocation() + Boom->TargetOffset;
    }

    const FVector TargetAimLocation = TargetCharacter->GetActorLocation()
        + FVector(0.0f, 0.0f, TargetAimHeightOffset);

    FRotator DesiredRotation = (TargetAimLocation - LookOrigin).Rotation();
    DesiredRotation.Roll = 0.0f;
    DesiredRotation.Pitch = FMath::Clamp(DesiredRotation.Pitch, MinimumCameraPitch, MaximumCameraPitch);

    const FRotator SmoothedRotation = FMath::RInterpTo(
        OwnerController->GetControlRotation(), DesiredRotation, DeltaTime, CameraRotationSpeed);

    FRotator FinalRotation = SmoothedRotation;
    FinalRotation.Roll = 0.0f;
    FinalRotation.Pitch = FMath::Clamp(FinalRotation.Pitch, MinimumCameraPitch, MaximumCameraPitch);
    OwnerController->SetControlRotation(FinalRotation);
}

void UTargetLockComponent::ApplyRaisedCameraBoom()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!IsValid(OwnerCharacter))
    {
        return;
    }

    USpringArmComponent* Boom = OwnerCharacter->FindComponentByClass<USpringArmComponent>();
    if (!IsValid(Boom))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TargetLock-Debug] No SpringArmComponent found."));
        return;
    }

    CameraBoom = Boom;
    OriginalCameraBoomTargetOffset = Boom->TargetOffset;
    bHasSavedCameraBoomOffset = true;
    Boom->TargetOffset = OriginalCameraBoomTargetOffset + FVector(0.0f, 0.0f, CameraBoomHeightOffset);

    UE_LOG(LogTemp, Warning, TEXT("[TargetLock-Debug] Camera boom raised. Offset=%s"), *Boom->TargetOffset.ToString());
}

void UTargetLockComponent::ApplyLockedCharacterSettings()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!IsValid(OwnerCharacter) || !bFaceTargetWhileLocked)
    {
        return;
    }

    UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
    if (!IsValid(Movement))
    {
        return;
    }

    if (!bHasSavedCharacterRotationSettings)
    {
        bOriginalUseControllerRotationYaw = OwnerCharacter->bUseControllerRotationYaw;
        bOriginalOrientRotationToMovement = Movement->bOrientRotationToMovement;
        bHasSavedCharacterRotationSettings = true;
    }

    OwnerCharacter->bUseControllerRotationYaw = true;
    Movement->bOrientRotationToMovement = false;
}

void UTargetLockComponent::RestoreCharacterSettings()
{
    if (!bHasSavedCharacterRotationSettings)
    {
        return;
    }

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (IsValid(OwnerCharacter))
    {
        OwnerCharacter->bUseControllerRotationYaw = bOriginalUseControllerRotationYaw;
        if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
        {
            Movement->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
        }
    }

    bHasSavedCharacterRotationSettings = false;
}

void UTargetLockComponent::RestoreCameraBoom()
{
    if (!bHasSavedCameraBoomOffset)
    {
        CameraBoom.Reset();
        return;
    }

    if (USpringArmComponent* Boom = CameraBoom.Get())
    {
        Boom->TargetOffset = OriginalCameraBoomTargetOffset;
    }

    CameraBoom.Reset();
    bHasSavedCameraBoomOffset = false;
}



