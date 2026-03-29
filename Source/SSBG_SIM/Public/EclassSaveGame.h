#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EclassSaveGame.generated.h"

UCLASS()
class SSBG_SIM_API UEclassSaveGame : public USaveGame
{
    GENERATED_BODY()

public:

    UPROPERTY(VisibleAnywhere) int32 AcademicCurrency = 0;
    UPROPERTY(VisibleAnywhere) int32 ExtraCurrency = 0;
    UPROPERTY(VisibleAnywhere) int32 IdleCurrency = 0;
    UPROPERTY(VisibleAnywhere) int32 Exp = 0;
};