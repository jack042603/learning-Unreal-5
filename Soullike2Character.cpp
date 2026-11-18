// Copyright Epic Games, Inc. All Rights Reserved.

#include "Soullike2Character.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Soullike2.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "HealthComponent.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

ASoullike2Character::ASoullike2Character()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void ASoullike2Character::BeginPlay()
{
    Super::BeginPlay();

    UHealthComponent* HealthComponent = FindComponentByClass<UHealthComponent>();

    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &ASoullike2Character::HandleDeath);
        UE_LOG(LogTemp, Display, TEXT("[Death System] Health Component connected successfully."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Death System] Health Component not found! Add it to BP_ThirdPersonCharacter."));
    }
}

void ASoullike2Character::HandleDeath(AActor* DeadActor)
{
    (void)DeadActor;

    if (bIsDead)
    {
        return;
    }

    bIsDead = true;
    UE_LOG(LogTemp, Warning, TEXT("[Death System] Character has died!"));

    if (bIsTargetLocked)
    {
        UnlockTarget();
    }

    bIsAttacking = false;
    CurrentComboIndex = 0;
    CurrentComboSection = NAME_None;
    bNextComboSectionQueued = false;

    // Stop any currently playing montage (including the attack combo).
    // Then play the Death Montage if one has been assigned in the Blueprint.
    if (GetMesh())
    {
        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            AnimInstance->Montage_Stop(0.15f);
        }

        if (DeathMontage)
        {
            const float DeathMontageLength = PlayAnimMontage(DeathMontage, 1.0f);
            if (DeathMontageLength > 0.0f)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Death System] Death Montage started."));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[Death System] Failed to play Death Montage. Check its Slot and Skeleton."));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[Death System] No Death Montage assigned. Character will remain in place."));
        }
    }

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
        Movement->DisableMovement();
    }

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        // Disable gameplay input. UI input will still be handled by the PlayerController.
        DisableInput(PlayerController);

        if (GameOverWidgetClass)
        {
            GameOverWidgetInstance = CreateWidget<UUserWidget>(
                PlayerController,
                GameOverWidgetClass
            );

            if (GameOverWidgetInstance)
            {
                GameOverWidgetInstance->AddToViewport(100);

                // Show the mouse and allow the player to click the Retry button.
                PlayerController->bShowMouseCursor = true;
                PlayerController->SetInputMode(FInputModeUIOnly());

                UE_LOG(LogTemp, Warning, TEXT("[Game Over] UI displayed successfully."));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[Game Over] Failed to create Widget."));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[Game Over] GameOverWidgetClass is not assigned."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Game Over] PlayerController is invalid."));
    }

    UE_LOG(LogTemp, Warning, TEXT("[Death System] Movement, attack and input disabled."));
}

void ASoullike2Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }

        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASoullike2Character::Move);
        }

        if (MouseLookAction)
        {
            EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ASoullike2Character::Look);
        }
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASoullike2Character::Look);
        }

        if (TargetLockAction)
        {
            EnhancedInputComponent->BindAction(TargetLockAction, ETriggerEvent::Started, this, &ASoullike2Character::OnTargetLockPressed);
            UE_LOG(LogTemp, Display, TEXT("[SimpleTargetLock] IA_TargetLock bound successfully."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[SimpleTargetLock] TargetLockAction is NULL. Assign IA_TargetLock in BP_ThirdPersonCharacter."));
        }

        if (AttackAction)
        {
            EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ASoullike2Character::Attack);
            UE_LOG(LogTemp, Display, TEXT("[Combat] Attack Input bound successfully."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[Combat] AttackAction is NULL. Assign IA_Attack in Character Blueprint."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SimpleTargetLock] EnhancedInputComponent not found."));
    }
}

void ASoullike2Character::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdateComboSection();

    if (!bIsTargetLocked)
    {
        return;
    }

    ACharacter* Target = CurrentTarget.Get();
    if (!IsValid(Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("[SimpleTargetLock] Target became invalid. Unlocking."));
        UnlockTarget();
        return;
    }

    if (bUnlockWhenTargetOutOfRange)
    {
        const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
        if (DistanceSquared > FMath::Square(TargetLockRadius))
        {
            UE_LOG(LogTemp, Display, TEXT("[SimpleTargetLock] Target left the search radius. Unlocking."));
            UnlockTarget();
            return;
        }
    }

    UpdateTargetLockCamera(DeltaTime);
}

void ASoullike2Character::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    DoMove(MovementVector.X, MovementVector.Y);
}

void ASoullike2Character::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ASoullike2Character::DoMove(float Right, float Forward)
{
    if (bIsDead)
    {
        return;
    }

    // Ignore player movement input during the attack Montage. Root Motion still works.
    if (bIsAttacking)
    {
        return;
    }

    if (GetController() != nullptr)
    {
        const FRotator Rotation = GetController()->GetControlRotation();
        const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(ForwardDirection, Forward);
        AddMovementInput(RightDirection, Right);
    }
}

void ASoullike2Character::DoLook(float Yaw, float Pitch)
{
    if (bIsDead || GetController() == nullptr || bIsTargetLocked)
    {
        return;
    }

    AddControllerYawInput(Yaw);
    AddControllerPitchInput(Pitch);
}

void ASoullike2Character::DoJumpStart()
{
    if (bIsDead)
    {
        return;
    }

    Jump();
}

void ASoullike2Character::DoJumpEnd()
{
    if (bIsDead)
    {
        return;
    }

    StopJumping();
}

void ASoullike2Character::OnTargetLockPressed()
{
    if (bIsDead)
    {
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[SimpleTargetLock] Q input received."));

    if (bIsTargetLocked)
    {
        UnlockTarget();
        UE_LOG(LogTemp, Display, TEXT("[SimpleTargetLock] Unlocked."));
        return;
    }

    ACharacter* NewTarget = FindNearestTarget();
    if (!IsValid(NewTarget))
    {
        UE_LOG(LogTemp, Warning, TEXT("[SimpleTargetLock] No other Character found within %.0f units."), TargetLockRadius);
        return;
    }

    CurrentTarget = NewTarget;
    bIsTargetLocked = true;

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        bOriginalUseControllerRotationYaw = bUseControllerRotationYaw;
        bOriginalOrientRotationToMovement = Movement->bOrientRotationToMovement;
        bHasSavedRotationSettings = true;
        bUseControllerRotationYaw = true;
        Movement->bOrientRotationToMovement = false;
    }

    if (IsValid(CameraBoom))
    {
        OriginalCameraBoomTargetOffset = CameraBoom->TargetOffset;
        bHasSavedCameraBoomOffset = true;
        CameraBoom->TargetOffset = OriginalCameraBoomTargetOffset + FVector(0.0f, 0.0f, CameraBoomHeightOffset);
    }

    UE_LOG(LogTemp, Display, TEXT("[SimpleTargetLock] Locked target: %s"), *GetNameSafe(NewTarget));
}

ACharacter* ASoullike2Character::FindNearestTarget() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[SimpleTargetLock] World is invalid."));
        return nullptr;
    }

    ACharacter* BestTarget = nullptr;
    float BestDistanceSquared = FMath::Square(TargetLockRadius);
    const FVector MyLocation = GetActorLocation();

    for (TActorIterator<ACharacter> It(World); It; ++It)
    {
        ACharacter* Candidate = *It;
        if (!IsValid(Candidate) || Candidate == this)
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared(MyLocation, Candidate->GetActorLocation());
        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

void ASoullike2Character::UpdateTargetLockCamera(float DeltaTime)
{
    ACharacter* Target = CurrentTarget.Get();
    AController* OwnerController = GetController();
    if (!IsValid(Target) || !IsValid(OwnerController))
    {
        return;
    }

    FVector LookOrigin = GetPawnViewLocation();
    if (IsValid(CameraBoom))
    {
        LookOrigin = CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset;
    }

    const FVector AimLocation = Target->GetActorLocation() + FVector(0.0f, 0.0f, TargetAimHeightOffset);
    FRotator DesiredRotation = (AimLocation - LookOrigin).Rotation();
    DesiredRotation.Roll = 0.0f;

    const FRotator CurrentRotation = OwnerController->GetControlRotation();
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, CameraRotationSpeed);
    NewRotation.Roll = 0.0f;
    OwnerController->SetControlRotation(NewRotation);
}

void ASoullike2Character::UnlockTarget()
{
    bIsTargetLocked = false;
    CurrentTarget.Reset();

    if (bHasSavedRotationSettings)
    {
        bUseControllerRotationYaw = bOriginalUseControllerRotationYaw;
        if (UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
            Movement->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
        }
        bHasSavedRotationSettings = false;
    }

    if (bHasSavedCameraBoomOffset && IsValid(CameraBoom))
    {
        CameraBoom->TargetOffset = OriginalCameraBoomTargetOffset;
    }
    bHasSavedCameraBoomOffset = false;

    UE_LOG(LogTemp, Display, TEXT("[SimpleTargetLock] Lock released; normal camera restored."));
}

// ========================================
// COMBAT - FOUR HIT CLICK-TO-CONTINUE COMBO
// One Montage, four sections: Default -> Attack1 -> Attack2 -> Attack3.
// Click 1 starts Default. While each section is playing, one further click
// queues the next section. If no click is received, the Montage ends at the
// end of the current section. No Combo Notify State is required.
// ========================================

void ASoullike2Character::Attack()
{
    if (bIsDead)
    {
        return;
    }

    UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("[Combo] Animation Instance is NULL."));
        return;
    }

    // First click starts the combo at its first section.
    if (!bIsAttacking)
    {
        if (!AttackMontage)
        {
            UE_LOG(LogTemp, Error, TEXT("[Combo] AttackMontage is NULL."));
            return;
        }

        const bool bSectionsExist =
            AttackMontage->GetSectionIndex(ComboFirstSectionName) != INDEX_NONE &&
            AttackMontage->GetSectionIndex(ComboSecondSectionName) != INDEX_NONE &&
            AttackMontage->GetSectionIndex(ComboThirdSectionName) != INDEX_NONE &&
            AttackMontage->GetSectionIndex(ComboFourthSectionName) != INDEX_NONE;

        if (!bSectionsExist)
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("[Combo] Missing montage section. Required order: %s -> %s -> %s -> %s."),
                *ComboFirstSectionName.ToString(),
                *ComboSecondSectionName.ToString(),
                *ComboThirdSectionName.ToString(),
                *ComboFourthSectionName.ToString()
            );
            return;
        }

        if (UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
            // Remove leftover player input velocity; montage Root Motion remains enabled.
            Movement->StopMovementImmediately();
        }

        const float MontageLength = AnimInstance->Montage_Play(AttackMontage, 1.0f);
        if (MontageLength <= 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Combo] Failed to play Combo Montage."));
            return;
        }

        bIsAttacking = true;
        CurrentComboIndex = 1;
        CurrentComboSection = ComboFirstSectionName;
        bNextComboSectionQueued = false;

        // Important: do not auto-chain. Each next section is linked only
        // when the player clicks during the currently playing section.
        AnimInstance->Montage_SetNextSection(ComboFirstSectionName, NAME_None, AttackMontage);
        AnimInstance->Montage_SetNextSection(ComboSecondSectionName, NAME_None, AttackMontage);
        AnimInstance->Montage_SetNextSection(ComboThirdSectionName, NAME_None, AttackMontage);
        AnimInstance->Montage_SetNextSection(ComboFourthSectionName, NAME_None, AttackMontage);
        AnimInstance->Montage_JumpToSection(ComboFirstSectionName, AttackMontage);

        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &ASoullike2Character::OnAttackMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate, AttackMontage);

        UE_LOG(
            LogTemp,
            Display,
            TEXT("[Combo] Hit 1 started (Section: %s). Waiting for another click to continue."),
            *ComboFirstSectionName.ToString()
        );
        return;
    }

    // A click during the current hit queues exactly one following section.
    if (!AttackMontage || !AnimInstance->Montage_IsPlaying(AttackMontage))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Combo] Montage is no longer playing; ignoring combo click."));
        return;
    }

    UpdateComboSection();

    if (bNextComboSectionQueued)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Combo] Next hit is already queued for this section."));
        return;
    }

    FName ActiveSection = AnimInstance->Montage_GetCurrentSection(AttackMontage);
    if (ActiveSection.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Combo] Could not determine the active Montage section."));
        return;
    }

    FName NextSection = NAME_None;
    int32 NextHitIndex = 0;

    if (ActiveSection == ComboFirstSectionName)
    {
        NextSection = ComboSecondSectionName;
        NextHitIndex = 2;
    }
    else if (ActiveSection == ComboSecondSectionName)
    {
        NextSection = ComboThirdSectionName;
        NextHitIndex = 3;
    }
    else if (ActiveSection == ComboThirdSectionName)
    {
        NextSection = ComboFourthSectionName;
        NextHitIndex = 4;
    }
    else if (ActiveSection == ComboFourthSectionName)
    {
        UE_LOG(LogTemp, Display, TEXT("[Combo] Final hit is playing; no further section."));
        return;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Combo] Unknown active section: %s."), *ActiveSection.ToString());
        return;
    }

    if (AttackMontage->GetSectionIndex(NextSection) == INDEX_NONE)
    {
        UE_LOG(LogTemp, Error, TEXT("[Combo] Next section %s does not exist."), *NextSection.ToString());
        return;
    }

    // The current section will continue normally. At its end, Montage follows
    // this link if and only if the player clicked while this section was active.
    AnimInstance->Montage_SetNextSection(ActiveSection, NextSection, AttackMontage);
    bNextComboSectionQueued = true;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[Combo] Click accepted during hit %d. At section end, continue to hit %d (%s -> %s)."),
        CurrentComboIndex,
        NextHitIndex,
        *ActiveSection.ToString(),
        *NextSection.ToString()
    );
}

void ASoullike2Character::SetComboInputWindow(bool bOpen)
{
    // Kept only so an old USoullikeComboWindowNotifyState class still compiles.
    // This combo implementation does not use Notify Windows. Remove old Combo
    // Notify State bars from AM_Attack; clicks are accepted during each section.
    (void)bOpen;
}

void ASoullike2Character::UpdateComboSection()
{
    if (!bIsAttacking || !AttackMontage)
    {
        return;
    }

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance || !AnimInstance->Montage_IsActive(AttackMontage))
    {
        return;
    }

    const FName ActiveSection = AnimInstance->Montage_GetCurrentSection(AttackMontage);
    if (ActiveSection.IsNone() || ActiveSection == CurrentComboSection)
    {
        return;
    }

    CurrentComboSection = ActiveSection;

    if (ActiveSection == ComboFirstSectionName)
    {
        CurrentComboIndex = 1;
    }
    else if (ActiveSection == ComboSecondSectionName)
    {
        CurrentComboIndex = 2;
    }
    else if (ActiveSection == ComboThirdSectionName)
    {
        CurrentComboIndex = 3;
    }
    else if (ActiveSection == ComboFourthSectionName)
    {
        CurrentComboIndex = 4;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Combo] Entered unexpected montage section: %s."), *ActiveSection.ToString());
        return;
    }

    // A new section can accept one new click.
    bNextComboSectionQueued = false;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[Combo] Entered hit %d (Section: %s). Waiting for a click to continue."),
        CurrentComboIndex,
        *CurrentComboSection.ToString()
    );
}

void ASoullike2Character::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage != AttackMontage)
    {
        return;
    }

    bIsAttacking = false;
    CurrentComboIndex = 0;
    CurrentComboSection = NAME_None;
    bNextComboSectionQueued = false;

    if (bInterrupted)
    {
        UE_LOG(LogTemp, Display, TEXT("[Combo] Montage interrupted; combo reset."));
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("[Combo] No further click was queued; combo finished. Next click starts hit 1."));
    }
}
