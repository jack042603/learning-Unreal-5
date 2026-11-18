// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Soullike2Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimMontage;
class UUserWidget;

struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 * Third-person character with a built-in Souls-like hard target lock and
 * a one-Montage, four-hit click-to-continue combo.
 * Combo sections: Default (hit 1) -> Attack1 (hit 2) -> Attack2 (hit 3) -> Attack3 (hit 4).
 */
UCLASS(abstract)
class ASoullike2Character : public ACharacter
{
    GENERATED_BODY()

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    UCameraComponent* FollowCamera;

protected:
    // ========================================
    // INPUT ACTIONS
    // ========================================

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MouseLookAction;

    /** Q: Toggle target lock. Assign IA_TargetLock in the Character Blueprint. */
    UPROPERTY(EditAnywhere, Category="Input|Target Lock")
    UInputAction* TargetLockAction;

    /** Attack input action (for example, Left Mouse Button). */
    UPROPERTY(EditAnywhere, Category="Input|Combat")
    UInputAction* AttackAction = nullptr;

    // ========================================
    // TARGET LOCK SETTINGS
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Detection", meta=(ClampMin="100.0"))
    float TargetLockRadius = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Camera", meta=(ClampMin="0.0", ClampMax="250.0"))
    float TargetAimHeightOffset = 110.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Camera", meta=(ClampMin="0.0", ClampMax="150.0"))
    float CameraBoomHeightOffset = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Camera", meta=(ClampMin="0.1", ClampMax="30.0"))
    float CameraRotationSpeed = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Detection")
    bool bUnlockWhenTargetOutOfRange = true;

    // ========================================
    // COMBAT SETTINGS - ONE MONTAGE / FOUR HITS
    // ========================================

    /** One montage containing Default, Attack1, Attack2 and Attack3. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Combo")
    UAnimMontage* AttackMontage = nullptr;

    /** Hit 1 section. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Combo")
    FName ComboFirstSectionName = FName(TEXT("Default"));

    /** Hit 2 section. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Combo")
    FName ComboSecondSectionName = FName(TEXT("Attack1"));

    /** Hit 3 section (final hit). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Combo")
    FName ComboThirdSectionName = FName(TEXT("Attack2"));

    /** Hit 4 section (final hit). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Combo")
    FName ComboFourthSectionName = FName(TEXT("Attack3"));

    // ========================================
    // DEATH SETTINGS
    // ========================================

    /** Optional death animation. Assign a compatible Montage in BP_ThirdPersonCharacter. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Death")
    UAnimMontage* DeathMontage = nullptr;

    /** Game Over widget assigned in BP_ThirdPersonCharacter. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Game Over")
    TSubclassOf<UUserWidget> GameOverWidgetClass;

    /** Starts hit 1, or queues the next section when clicked during the current hit. */
    void Attack();

    /** Called when the combo montage ends or is interrupted. */
    void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    /** Updates CurrentComboIndex when the Montage enters another section. */
    void UpdateComboSection();

public:
    ASoullike2Character();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** Called once when the HealthComponent broadcasts OnDeath. */
    UFUNCTION()
    void HandleDeath(AActor* DeadActor);
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void OnTargetLockPressed();
    ACharacter* FindNearestTarget() const;
    void UpdateTargetLockCamera(float DeltaTime);
    void UnlockTarget();

public:
    /**
     * Legacy compatibility for the old Notify State class.
     * The click-to-continue combo no longer uses Notify Windows.
     */
    void SetComboInputWindow(bool bOpen);

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoMove(float Right, float Forward);

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoLook(float Yaw, float Pitch);

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpStart();

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpEnd();

    FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    FORCEINLINE bool IsTargetLocked() const { return bIsTargetLocked; }
    FORCEINLINE ACharacter* GetLockedTarget() const { return CurrentTarget.Get(); }

private:
    // ========================================
    // DEATH STATE
    // ========================================

    bool bIsDead = false;

    /** Runtime instance of the Game Over widget. */
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> GameOverWidgetInstance = nullptr;

    // ========================================
    // TARGET LOCK VARIABLES
    // ========================================

    TWeakObjectPtr<ACharacter> CurrentTarget;
    bool bIsTargetLocked = false;
    bool bOriginalUseControllerRotationYaw = false;
    bool bOriginalOrientRotationToMovement = true;
    bool bHasSavedRotationSettings = false;
    FVector OriginalCameraBoomTargetOffset = FVector::ZeroVector;
    bool bHasSavedCameraBoomOffset = false;

    // ========================================
    // COMBAT VARIABLES - FOUR HITS
    // ========================================

    /** True while the single four-section Montage is playing. */
    bool bIsAttacking = false;

    /** Current hit index: 1, 2, 3 or 4. */
    int32 CurrentComboIndex = 0;

    /** Name of the section currently playing. */
    FName CurrentComboSection = NAME_None;

    /** True after a click has queued the next section for the current hit. */
    bool bNextComboSectionQueued = false;
};
