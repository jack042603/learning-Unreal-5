// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

// 死亡事件
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnDeath,
    AActor*, DeadActor
);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOULLIKE2_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UHealthComponent();

protected:

    virtual void BeginPlay() override;

    // 最大 HP
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health")
    float MaxHealth = 100.0f;

    // 当前 HP
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
    float CurrentHealth = 100.0f;

    // 是否死亡
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
    bool bIsDead = false;

public:

    // 扣除 HP
    UFUNCTION(BlueprintCallable, Category="Health")
    void ApplyDamageAmount(float DamageAmount);

    // 获取当前 HP
    UFUNCTION(BlueprintPure, Category="Health")
    float GetCurrentHealth() const;

    // 获取最大 HP
    UFUNCTION(BlueprintPure, Category="Health")
    float GetMaxHealth() const;

    // 检查是否死亡
    UFUNCTION(BlueprintPure, Category="Health")
    bool IsDead() const;

    // 死亡事件（Blueprint 可以接收）
    UPROPERTY(BlueprintAssignable, Category="Health")
    FOnDeath OnDeath;
};

