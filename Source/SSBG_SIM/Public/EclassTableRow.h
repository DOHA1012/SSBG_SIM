#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EclassTableRow.generated.h"

USTRUCT(BlueprintType)
struct FEclassTableRow : public FTableRowBase
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AcademicCurrency = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ExtraCurrency = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 IdleCurrency = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Exp = 0;
};