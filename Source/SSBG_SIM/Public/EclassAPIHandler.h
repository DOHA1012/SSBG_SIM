#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Interfaces/IHttpRequest.h"
#include "EclassAPIHandler.generated.h"

// ================================================================
// 재화 구조체
// ================================================================

USTRUCT(BlueprintType)
struct FEclassData
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString StudentId;
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

    UPROPERTY(BlueprintReadOnly, Category = "Login")
    bool bLoginSuccess = false;

    UPROPERTY(BlueprintReadOnly, Category = "Login")
    FString LoginMessage = "";
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

// ================================================================
// 학사 로그 구조체
// ================================================================

USTRUCT(BlueprintType)
struct FAcademicLogEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) int32   Id = 0;
    UPROPERTY(BlueprintReadWrite) FString ChangeType;
    UPROPERTY(BlueprintReadWrite) FString Detail;
    UPROPERTY(BlueprintReadWrite) int32   DeltaExtra = 0;
    UPROPERTY(BlueprintReadWrite) int32   DeltaExp = 0;
    UPROPERTY(BlueprintReadWrite) FString CreatedAt;
};

// ================================================================
// 아이템 구조체
// ItemType: "Hat"|"Bag"|"Clothes"|"Theme"|"Friend"|"Consumable"|"relic"
// ================================================================

USTRUCT(BlueprintType)
struct FEclassItemOptionInfo
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString OptionCode;
    UPROPERTY(BlueprintReadWrite) FString Name;
    UPROPERTY(BlueprintReadWrite) FString Description;
    UPROPERTY(BlueprintReadWrite) FString ValueType;
    UPROPERTY(BlueprintReadWrite) float   Value = 1.0f;
};

USTRUCT(BlueprintType)
struct FEclassItemInfo
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ItemCode;
    UPROPERTY(BlueprintReadWrite) FString Name;
    UPROPERTY(BlueprintReadWrite) FString Description;
    UPROPERTY(BlueprintReadWrite) FString ItemType;
    UPROPERTY(BlueprintReadWrite) FString CosmeticSlot;
    UPROPERTY(BlueprintReadWrite) int32   SlotIndex = 0;
    UPROPERTY(BlueprintReadWrite) bool    bIsEquipped = false;
    UPROPERTY(BlueprintReadWrite) FString ObtainedAt;
    UPROPERTY(BlueprintReadWrite) TArray<FEclassItemOptionInfo> Options;
};

USTRUCT(BlueprintType)
struct FEclassCollectionEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString CollectionCode;
    UPROPERTY(BlueprintReadWrite) FString Name;
    UPROPERTY(BlueprintReadWrite) FString Description;
    UPROPERTY(BlueprintReadWrite) FString CollectionType;
    UPROPERTY(BlueprintReadWrite) bool    bIsUnlocked = false;
    UPROPERTY(BlueprintReadWrite) FString UnlockedAt;
};

// ================================================================
// 델리게이트
// ================================================================

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnLoginComplete, FLoginResult, Result);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnEclassDataReceived, FEclassData, Data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSpendGoldResult, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDailyResetComplete, bool, bReadyForDreamShop);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnServerTimeReceived, FServerTime, ServerTime);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGainCurrencyComplete, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAcademicLogReceived, const TArray<FAcademicLogEntry>&, Logs);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnInventoryReceived, const TArray<FEclassItemInfo>&, Items);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnCollectionReceived, const TArray<FEclassCollectionEntry>&, Entries);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnEquipResult, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnUnlockedCodesReceived, const TArray<FString>&, ItemCodes);
// ✅ 재화 단일 값 반환용
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnCurrencyReceived, int32, Value);

// ================================================================
// 클래스
// ================================================================

UCLASS()
class SSBG_SIM_API UEclassAPIHandler : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static void Login(FString UserId, FOnLoginComplete OnComplete);

    // ✅ 재화 정보만 조회 (블루프린트 연결 단순화)
    UFUNCTION(BlueprintCallable, Category = "API")
    static void GetUserCurrency(FString UserId, FOnEclassDataReceived OnComplete);

    // ✅ 재화별 개별 조회
    UFUNCTION(BlueprintCallable, Category = "API|Currency")
    static void GetAcademicCurrency(FString UserId, FOnCurrencyReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Currency")
    static void GetExtraCurrency(FString UserId, FOnCurrencyReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Currency")
    static void GetIdleCurrency(FString UserId, FOnCurrencyReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Currency")
    static void GetExp(FString UserId, FOnCurrencyReceived OnComplete);

    UFUNCTION(BlueprintCallable)
    static void RequestDailyReset(FString UserId, FOnDailyResetComplete OnComplete);

    UFUNCTION(BlueprintCallable)
    static void SpendCurrency(FString UserId, FString CurrencyType, int32 Amount, FOnSpendGoldResult OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API")
    static void GainCurrency(FString UserId, int32 Amount, FString CurrencyType);

    UFUNCTION(BlueprintCallable)
    static void GetServerTime(FOnServerTimeReceived OnComplete);

    UFUNCTION(BlueprintCallable)
    static void GetAcademicLog(FString UserId, FOnAcademicLogReceived OnComplete);

    // 인벤토리
    // ItemType: "" = 전체 / "Hat" / "Bag" / "Clothes" / "Theme" / "Friend" / "Consumable" / "relic"
    UFUNCTION(BlueprintCallable, Category = "API|Inventory")
    static void GetInventory(FString UserId, FString ItemType, FOnInventoryReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Inventory")
    static void GetEquippedItems(FString UserId, FOnInventoryReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Inventory")
    static void EquipItem(FString UserId, FString ItemCode, FOnEquipResult OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Inventory")
    static void UnequipItem(FString UserId, FString ItemCode, FOnEquipResult OnComplete);

    // 도감 전체 조회
    // CollectionType: "" = 전체 / "Hat" / "Bag" / "Clothes" / "Theme" / "Friend" / "Consumable" / "relic"
    UFUNCTION(BlueprintCallable, Category = "API|Collection")
    static void GetCollection(FString UserId, FString CollectionType, FOnCollectionReceived OnComplete);

    // 해금된 itemCode 목록만 조회 (엔진 아이템 테이블 비교용)
    UFUNCTION(BlueprintCallable, Category = "API|Collection")
    static void GetUnlockedItemCodes(FString UserId, FString CollectionType, FOnUnlockedCodesReceived OnComplete);

    UFUNCTION(BlueprintCallable)
    static void SaveEclassData();

    UFUNCTION(BlueprintCallable)
    static void LoadEclassData();

private:
    static FEclassData CachedData;
    static void ApplyAndCache(FEclassData NewData);
};