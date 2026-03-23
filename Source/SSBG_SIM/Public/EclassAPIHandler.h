// EclassAPIHandler.h
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EclassAPIHandler.generated.h"

USTRUCT(BlueprintType)
struct FEclassData
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite)
    int32 Gold = 0;
    UPROPERTY(BlueprintReadWrite)
    int32 Exp = 0;
};

UCLASS()
class SSBG_SIM_API UEclassAPIHandler : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static void SyncEclass(FString UserId);

    UFUNCTION(BlueprintCallable)
    static FEclassData GetEclassData();

    UFUNCTION(BlueprintCallable)
    static void SaveEclassData();

    UFUNCTION(BlueprintCallable)
    static void LoadEclassData();

    UFUNCTION(BlueprintCallable)
    static FEclassData GetDataFromTable(UDataTable* Table);

private:
    static FEclassData CachedData;
};