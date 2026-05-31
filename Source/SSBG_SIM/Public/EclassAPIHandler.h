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

// ================================================================
// 로그인 결과 (성공 여부만)
// ================================================================

USTRUCT(BlueprintType)
struct FLoginResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "Login") bool    bLoginSuccess = false;
    UPROPERTY(BlueprintReadOnly, Category = "Login") FString LoginMessage;
};

// ================================================================
// 유저 세션 데이터 (로그인 후 호출)
// 재화 변동량 + 리셋 정보
// ================================================================

USTRUCT(BlueprintType)
struct FUserSessionData
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FEclassDelta Delta;
    UPROPERTY(BlueprintReadWrite) bool         bResetDoneToday = false;
    UPROPERTY(BlueprintReadWrite) int32        SecondsUntilReset = 0;
};

// ================================================================
// 서버 시간
// ================================================================

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
    UPROPERTY(BlueprintReadWrite) int32   InventoryId = 0;
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
// 꿈상점 구조체
// ================================================================

USTRUCT(BlueprintType)
struct FDreamShopOption
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString OptionCode;
    UPROPERTY(BlueprintReadWrite) FString Grade;
    UPROPERTY(BlueprintReadWrite) float   Value = 1.0f;
};

USTRUCT(BlueprintType)
struct FDreamShopItem
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ItemCode;
    UPROPERTY(BlueprintReadWrite) FString Grade;        // basic/low/mid/high/top
    UPROPERTY(BlueprintReadWrite) float   Multiplier = 1.0f;
    UPROPERTY(BlueprintReadWrite) TArray<FDreamShopOption> Options;
    UPROPERTY(BlueprintReadWrite) bool    bBought = false;
};

USTRUCT(BlueprintType)
struct FDreamShopResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) bool                    bSuccess = false;
    UPROPERTY(BlueprintReadWrite) FString                 Message;
    UPROPERTY(BlueprintReadWrite) TArray<FDreamShopItem>  Items;
    UPROPERTY(BlueprintReadWrite) int32                   MaxBuyCount = 1;
    UPROPERTY(BlueprintReadWrite) int32                   UsedBuyCount = 0;
};

USTRUCT(BlueprintType)
struct FShopItem
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ShopId;
    UPROPERTY(BlueprintReadWrite) FString ItemCode;
    UPROPERTY(BlueprintReadWrite) FString Name;
    UPROPERTY(BlueprintReadWrite) FString Description;
    UPROPERTY(BlueprintReadWrite) FString ItemType;
    UPROPERTY(BlueprintReadWrite) FString CosmeticSlot;
    UPROPERTY(BlueprintReadWrite) FString CurrencyType;  // "academicCurrency"|"extraCurrency"|"idleCurrency"
    UPROPERTY(BlueprintReadWrite) int32   Price = 0;
};

// ================================================================
// 델리게이트
// ================================================================

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnLoginComplete, FLoginResult, Result);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnUserSessionReceived, FUserSessionData, SessionData);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnEclassDataReceived, FEclassData, Data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnCurrencyReceived, int32, Value);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSpendGoldResult, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDailyResetComplete, bool, bReadyForDreamShop);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnServerTimeReceived, FServerTime, ServerTime);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAcademicLogReceived, const TArray<FAcademicLogEntry>&, Logs);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnInventoryReceived, const TArray<FEclassItemInfo>&, Items);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnCollectionReceived, const TArray<FEclassCollectionEntry>&, Entries);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnEquipResult, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnUnlockedCodesReceived, const TArray<FString>&, ItemCodes);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnShopReceived, const TArray<FShopItem>&, Items);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnBuyItemResult, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnCraftItemResult, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnFindRecipeResult, bool, bSuccess, FString, CraftId);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDreamShopReceived, FDreamShopResult, Result);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDreamShopBuyResult, bool, bSuccess);

// ================================================================
// 클래스
// ================================================================

UCLASS()
class SSBG_SIM_API UEclassAPIHandler : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // 로그인 - 학번 검증만 (성공/실패)
    UFUNCTION(BlueprintCallable, Category = "API")
    static void Login(FString UserId, FOnLoginComplete OnComplete);

    // 로그인 성공 후 호출 - 델타 + 리셋 정보
    UFUNCTION(BlueprintCallable, Category = "API")
    static void GetUserData(FString UserId, FOnUserSessionReceived OnComplete);

    // 재화 전체 조회
    UFUNCTION(BlueprintCallable, Category = "API|Currency")
    static void GetUserCurrency(FString UserId, FOnEclassDataReceived OnComplete);

    // 재화별 개별 조회
    UFUNCTION(BlueprintCallable, Category = "API|Currency")
    static void GetAcademicCurrency(FString UserId, FOnCurrencyReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Currency")
    static void GetExtraCurrency(FString UserId, FOnCurrencyReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Currency")
    static void GetIdleCurrency(FString UserId, FOnCurrencyReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Currency")
    static void GetExp(FString UserId, FOnCurrencyReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API")
    static void RequestDailyReset(FString UserId, FOnDailyResetComplete OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API")
    static void SpendCurrency(FString UserId, FString CurrencyType, int32 Amount, FOnSpendGoldResult OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API")
    static void GainCurrency(FString UserId, int32 Amount, FString CurrencyType);

    UFUNCTION(BlueprintCallable, Category = "API")
    static void GetServerTime(FOnServerTimeReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API")
    static void GetAcademicLog(FString UserId, FOnAcademicLogReceived OnComplete);

    // 인벤토리
    UFUNCTION(BlueprintCallable, Category = "API|Inventory")
    static void GetInventory(FString UserId, FString ItemType, FOnInventoryReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Inventory")
    static void GetEquippedItems(FString UserId, FOnInventoryReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Inventory")
    static void EquipItem(FString UserId, FString ItemCode, FOnEquipResult OnComplete);

    // Consumable 전용 장착 (최대 3개, 초과 시 가장 왼쪽 해제)
    UFUNCTION(BlueprintCallable, Category = "API|Inventory")
    static void EquipConsumable(FString UserId, FString ItemCode, FOnEquipResult OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Inventory")
    static void UnequipItem(FString UserId, FString ItemCode, FOnEquipResult OnComplete);

    // 도감
    UFUNCTION(BlueprintCallable, Category = "API|Collection")
    static void GetCollection(FString UserId, FString CollectionType, FOnCollectionReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Collection")
    static void GetUnlockedItemCodes(FString UserId, FString CollectionType, FOnUnlockedCodesReceived OnComplete);

    // 상점
    // ItemType: "" = 전체 / "Hat" / "Bag" / "Clothes" / "Theme" / "Friend" / "Consumable"
    UFUNCTION(BlueprintCallable, Category = "API|Shop")
    static void GetShop(FString ItemType, FOnShopReceived OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API|Shop")
    static void BuyItem(FString UserId, FString ShopId, FOnBuyItemResult OnComplete);

    // 꿈상점
    UFUNCTION(BlueprintCallable, Category = "API|DreamShop")
    static void GetDreamShop(FString UserId, FOnDreamShopReceived OnComplete);

    // itemIndex: 0부터 시작
    UFUNCTION(BlueprintCallable, Category = "API|DreamShop")
    static void BuyDreamShopItem(FString UserId, int32 ItemIndex, FOnDreamShopBuyResult OnComplete);

    // 제작
    UFUNCTION(BlueprintCallable, Category = "API|Craft")
    static void CraftItem(FString UserId, FString CraftId, FOnCraftItemResult OnComplete);

    // 재화 조합으로 레시피 찾기 (사용 안 하는 재화는 0으로)
    UFUNCTION(BlueprintCallable, Category = "API|Craft")
    static void FindRecipe(int32 Academic, int32 Extra, int32 Idle, FOnFindRecipeResult OnComplete);

    UFUNCTION(BlueprintCallable, Category = "API")
    static void SaveEclassData();

    UFUNCTION(BlueprintCallable, Category = "API")
    static void LoadEclassData();

    // 캐시된 유저 데이터 조회 (자동로그인용)
    UFUNCTION(BlueprintCallable, Category = "API")
    static FEclassData GetCachedData();

private:
    static FEclassData CachedData;
    static void ApplyAndCache(FEclassData NewData);
};