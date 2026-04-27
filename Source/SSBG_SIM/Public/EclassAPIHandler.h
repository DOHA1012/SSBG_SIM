#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Interfaces/IHttpRequest.h"
#include "EclassAPIHandler.generated.h"

USTRUCT(BlueprintType)
struct FEclassData
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) int32 AcademicCurrency = 0;
    UPROPERTY(BlueprintReadWrite) int32 ExtraCurrency = 0;
    UPROPERTY(BlueprintReadWrite) int32 IdleCurrency = 0;
    UPROPERTY(BlueprintReadWrite) int32 Exp = 0;
};

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

USTRUCT(BlueprintType)
struct FLoginResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FEclassData Data;
    UPROPERTY(BlueprintReadWrite) FEclassDelta Delta;
    UPROPERTY(BlueprintReadWrite) bool  bResetDoneToday = false;
    UPROPERTY(BlueprintReadWrite) int32 SecondsUntilReset = 0;
};

USTRUCT(BlueprintType)
struct FServerTime
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) int32 UtcHour = 0;
    UPROPERTY(BlueprintReadWrite) int32 UtcDay = 0;
    UPROPERTY(BlueprintReadWrite) int32 UtcMonth = 0;
    UPROPERTY(BlueprintReadWrite) int32 UtcYear = 0;
    UPROPERTY(BlueprintReadWrite) int32 SecondsUntilReset = 0;
};

// 학사 변동 로그 1개
USTRUCT(BlueprintType)
struct FAcademicLogEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) int32   Id = 0;
    UPROPERTY(BlueprintReadWrite) FString ChangeType;   // "attendance" | "assignment"
    UPROPERTY(BlueprintReadWrite) FString Detail;       // "출석 1회 → Extra +100 / EXP +30 획득!"
    UPROPERTY(BlueprintReadWrite) int32   DeltaExtra = 0;
    UPROPERTY(BlueprintReadWrite) int32   DeltaExp = 0;
    UPROPERTY(BlueprintReadWrite) FString CreatedAt;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnLoginComplete, FLoginResult, Result);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnEclassDataReceived, FEclassData, Data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSpendGoldResult, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDailyResetComplete, bool, bReadyForDreamShop);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnServerTimeReceived, FServerTime, ServerTime);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGainCurrencyComplete, bool, bSuccess);
// 로그 조회 델리게이트
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAcademicLogReceived, const TArray<FAcademicLogEntry>&, Logs);

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

    UFUNCTION(BlueprintCallable, Category = "API")
    static void GainCurrency(FString UserId, int32 Amount, FString CurrencyType);

    UFUNCTION(BlueprintCallable)
    static void GetServerTime(FOnServerTimeReceived OnComplete);

    // 학사 변동 로그 조회
    UFUNCTION(BlueprintCallable)
    static void GetAcademicLog(FString UserId, FOnAcademicLogReceived OnComplete);

private:
    static FEclassData CachedData;
    static void ApplyAndCache(FEclassData NewData);
};