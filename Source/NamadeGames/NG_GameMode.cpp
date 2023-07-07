// Copyright Epic Games, Inc. All Rights Reserved.

#include "NG_GameMode.h"
#include "NG_ThirdPersonCharacter.h"
#include "UObject/ConstructorHelpers.h"

ANG_GameMode::ANG_GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
