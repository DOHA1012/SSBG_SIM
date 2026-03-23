#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EclassSaveGame.generated.h"

UCLASS()
class SSBG_SIM_API UEclassSaveGame : public USaveGame
{
    GENERATED_BODY()

public:

    UPROPERTY(VisibleAnywhere)
    int32 Gold;

    UPROPERTY(VisibleAnywhere)
    int32 Exp;
};