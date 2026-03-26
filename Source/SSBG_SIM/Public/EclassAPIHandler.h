#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EclassAPIHandler.generated.h"

USTRUCT(BlueprintType)
struct FEclassData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) int32 Gold = 0;
    UPROPERTY(BlueprintReadWrite) int32 Exp = 0;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnEclassDataReceived, FEclassData, Data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSpendGoldResult, bool, bSuccess);

UCLASS()
class SSBG_SIM_API UEclassAPIHandler : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // 접속 시 호출 - 게임서버 DB에서 유저 데이터 로드
    UFUNCTION(BlueprintCallable)
    static void Login(FString UserId, FOnEclassDataReceived OnComplete);

    // 재화 사용 요청
    UFUNCTION(BlueprintCallable)
    static void SpendGold(FString UserId, int32 Amount, FOnSpendGoldResult OnComplete);

    // 캐시에서 즉시 반환 (UI 갱신용)
    UFUNCTION(BlueprintCallable)
    static FEclassData GetEclassData();

    // 로컬 저장/불러오기 (오프라인 대비)
    UFUNCTION(BlueprintCallable)
    static void SaveEclassData();

    UFUNCTION(BlueprintCallable)
    static void LoadEclassData();

private:
    static FEclassData CachedData;
    static void ApplyAndCache(FEclassData NewData);
};