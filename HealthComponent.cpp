// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    // 初始化 HP
    CurrentHealth = MaxHealth;

    // 重置死亡状态
    bIsDead = false;
}

void UHealthComponent::ApplyDamageAmount(float DamageAmount)
{
    // 如果已经死亡，不再接受伤害
    if (bIsDead)
    {
        return;
    }

    // 忽略无效伤害
    if (DamageAmount <= 0.0f)
    {
        return;
    }

    // 扣除 HP
    CurrentHealth = FMath::Clamp(
        CurrentHealth - DamageAmount,
        0.0f,
        MaxHealth
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Current HP: %f / %f"),
        CurrentHealth,
        MaxHealth
    );

    // 检查是否死亡
    if (CurrentHealth <= 0.0f)
    {
        bIsDead = true;

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("CHARACTER DIED!")
        );

        // 发送死亡事件
        OnDeath.Broadcast(GetOwner());
    }
}

float UHealthComponent::GetCurrentHealth() const
{
    return CurrentHealth;
}

float UHealthComponent::GetMaxHealth() const
{
    return MaxHealth;
}

bool UHealthComponent::IsDead() const
{
    return bIsDead;
}

