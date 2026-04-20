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

//  로그인 응답 전체를 담는 구조체
USTRUCT(BlueprintType)
struct FLoginResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FEclassData Data;
    UPROPERTY(BlueprintReadWrite) FEclassDelta Delta;
    UPROPERTY(BlueprintReadWrite) bool  bResetDoneToday = false;      // 오늘 정산 완료 여부
    UPROPERTY(BlueprintReadWrite) int32 SecondsUntilReset = 0;        // 다음 리셋까지 남은 초
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

// 델리게이트: FLoginResult 하나로 통합
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnLoginComplete, FLoginResult, Result);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnEclassDataReceived, FEclassData, Data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSpendGoldResult, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDailyResetComplete, bool, bReadyForDreamShop);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnServerTimeReceived, FServerTime, ServerTime);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGainCurrencyComplete, bool, bSuccess);

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

private:
    static FEclassData CachedData;
    static void ApplyAndCache(FEclassData NewData);
    // 서버가 정보 받으면 자동으로 호출되는 콜백함수(현 코드에서 필요없어 주석처리함)
//void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
