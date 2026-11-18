// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Soullike2GameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ASoullike2GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	ASoullike2GameMode();
};



