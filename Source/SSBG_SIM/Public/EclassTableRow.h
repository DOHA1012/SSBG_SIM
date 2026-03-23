#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EclassTableRow.generated.h"

USTRUCT(BlueprintType)
struct FEclassTableRow : public FTableRowBase
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Gold;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Exp;
};