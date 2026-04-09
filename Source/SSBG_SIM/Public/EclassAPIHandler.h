#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EclassAPIHandler.generated.h"

// 1. FEclassData 먼저
USTRUCT(BlueprintType)
struct FEclassData
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) int32 AcademicCurrency = 0;
    UPROPERTY(BlueprintReadWrite) int32 ExtraCurrency = 0;
    UPROPERTY(BlueprintReadWrite) int32 IdleCurrency = 0;
    UPROPERTY(BlueprintReadWrite) int32 Exp = 0;
};

// 2. FEclassDelta 다음
USTRUCT(BlueprintType)
struct FEclassDelta
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) int32 AcademicCurrency = 0;
    UPROPERTY(BlueprintReadWrite) int32 ExtraCurrency = 0;
    UPROPERTY(BlueprintReadWrite) int32 IdleCurrency = 0;
    UPROPERTY(BlueprintReadWrite) int32 Exp = 0;
    UPROPERTY(BlueprintReadWrite) bool  bHasChange = false;
};

// 3. 델리게이트
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnLoginComplete, FEclassData, Data, FEclassDelta, Delta);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnEclassDataReceived, FEclassData, Data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSpendGoldResult, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDailyResetComplete, bool, bReadyForDreamShop);

// 4. 클래스
UCLASS()
class SSBG_SIM_API UEclassAPIHandler : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static void Login(FString UserId, FOnLoginComplete OnComplete);

    UFUNCTION(BlueprintCallable)
    static void RequestDailyReset(FString UserId, FOnDailyResetComplete OnComplete);

    UFUNCTION(BlueprintCallable)
    static void SpendCurrency(FString UserId, FString CurrencyType, int32 Amount, FOnSpendGoldResult OnComplete);

    UFUNCTION(BlueprintCallable)
    static FEclassData GetEclassData();

    UFUNCTION(BlueprintCallable)
    static void SaveEclassData();

    UFUNCTION(BlueprintCallable)
    static void LoadEclassData();

private:
    static FEclassData CachedData;
    static void ApplyAndCache(FEclassData NewData);
};