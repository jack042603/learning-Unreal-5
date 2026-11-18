// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetLockComponent.generated.h"

class ACharacter;
class USpringArmComponent;

/**
 * Souls-like hard target lock for UE 5.8.
 * Q toggles the nearest other Character.
 * While locked, the camera continuously tracks the target and ignores mouse look input
 * (the Character's DoLook function must early-return while IsLockedOn() is true).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UTargetLockComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTargetLockComponent();

    /** Toggle target lock on/off. Bind this to Q. */
    UFUNCTION(BlueprintCallable, Category="Target Lock")
    void ToggleLockOn();

    /** Finds the nearest other Character and locks onto it. */
    UFUNCTION(BlueprintCallable, Category="Target Lock")
    bool LockOn();

    /** Unlocks and restores camera boom and character rotation settings. */
    UFUNCTION(BlueprintCallable, Category="Target Lock")
    void Unlock();

    UFUNCTION(BlueprintPure, Category="Target Lock")
    bool IsLockedOn() const;

    UFUNCTION(BlueprintPure, Category="Target Lock")
    AActor* GetCurrentTarget() const;

    /** Search radius in Unreal Units. 100 UU is approximately 1 metre. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Detection", meta=(ClampMin="100.0"))
    float LockOnRadius = 2000.0f;

    /** Height above target actor origin that the camera aims at. Higher values aim higher on the target. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Camera", meta=(ClampMin="0.0", ClampMax="250.0"))
    float TargetAimHeightOffset = 90.0f;

    /** Raises the SpringArm pivot while locked. This physically raises the camera viewpoint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Camera", meta=(ClampMin="0.0", ClampMax="150.0"))
    float CameraBoomHeightOffset = 120.0f;

    /** Higher values make the camera turn toward the target faster. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Camera", meta=(ClampMin="0.1", ClampMax="30.0"))
    float CameraRotationSpeed = 8.0f;

    /** Pitch limits in degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Camera", meta=(ClampMin="-89.0", ClampMax="0.0"))
    float MinimumCameraPitch = -65.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Camera", meta=(ClampMin="0.0", ClampMax="89.0"))
    float MaximumCameraPitch = 60.0f;

    /** Makes the player face the target and use controller-yaw strafing while locked. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Character")
    bool bFaceTargetWhileLocked = true;

    /** Unlock automatically if the target moves beyond LockOnRadius. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target Lock|Detection")
    bool bUnlockWhenTargetOutOfRange = true;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<ACharacter> CurrentTarget;
    TWeakObjectPtr<USpringArmComponent> CameraBoom;

    bool bHasSavedCharacterRotationSettings = false;
    bool bOriginalUseControllerRotationYaw = false;
    bool bOriginalOrientRotationToMovement = true;

    bool bHasSavedCameraBoomOffset = false;
    FVector OriginalCameraBoomTargetOffset = FVector::ZeroVector;

    ACharacter* FindNearestTarget() const;
    void UpdateLockedCamera(float DeltaTime);
    void ApplyLockedCharacterSettings();
    void ApplyRaisedCameraBoom();
    void RestoreCharacterSettings();
    void RestoreCameraBoom();
};



