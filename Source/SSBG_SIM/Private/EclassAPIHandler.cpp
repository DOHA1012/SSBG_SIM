#include "EclassAPIHandler.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "EclassSaveGame.h"

FEclassData UEclassAPIHandler::CachedData;

// 오라클 서버 주소
static const FString GAME_SERVER = TEXT("http://134.185.100.53:3000/api");

// 로컬 서버 주소
//static const FString GAME_SERVER = TEXT("http://127.0.0.1:3000/api");

// ================================================================
// 헬퍼
// ================================================================

static FEclassData ParseUserJson(TSharedPtr<FJsonObject> UserObj)
{
    FEclassData Result;
    if (!UserObj.IsValid()) return Result;

    int32 AC = 0, EC = 0, IC = 0, E = 0;
    UserObj->TryGetStringField(TEXT("studentId"), Result.StudentId);
    UserObj->TryGetNumberField(TEXT("academicCurrency"), AC);
    UserObj->TryGetNumberField(TEXT("extraCurrency"), EC);
    UserObj->TryGetNumberField(TEXT("idleCurrency"), IC);
    UserObj->TryGetNumberField(TEXT("exp"), E);

    Result.AcademicCurrency = AC;
    Result.ExtraCurrency = EC;
    Result.IdleCurrency = IC;
    Result.Exp = E;
    return Result;
}

static FEclassItemInfo ParseItemJson(const TSharedPtr<FJsonObject>& Obj)
{
    FEclassItemInfo Item;
    if (!Obj.IsValid()) return Item;

    int32 SlotIndex = 0;
    int32 InventoryId = 0;
    bool bEquipped = false;
    Obj->TryGetStringField(TEXT("itemCode"), Item.ItemCode);
    Obj->TryGetStringField(TEXT("name"), Item.Name);
    Obj->TryGetStringField(TEXT("description"), Item.Description);
    Obj->TryGetStringField(TEXT("itemType"), Item.ItemType);
    Obj->TryGetStringField(TEXT("cosmeticSlot"), Item.CosmeticSlot);
    Obj->TryGetStringField(TEXT("obtainedAt"), Item.ObtainedAt);
    Obj->TryGetNumberField(TEXT("slotIndex"), SlotIndex);
    Obj->TryGetNumberField(TEXT("inventoryId"), InventoryId);
    Obj->TryGetBoolField(TEXT("isEquipped"), bEquipped);

    Item.SlotIndex = SlotIndex;
    Item.InventoryId = InventoryId;
    Item.bIsEquipped = bEquipped;

    const TArray<TSharedPtr<FJsonValue>>* OptionsArr;
    if (Obj->TryGetArrayField(TEXT("options"), OptionsArr))
    {
        for (const TSharedPtr<FJsonValue>& Val : *OptionsArr)
        {
            const TSharedPtr<FJsonObject>& OptObj = Val->AsObject();
            if (!OptObj.IsValid()) continue;

            FEclassItemOptionInfo Opt;
            double Value = 1.0;
            OptObj->TryGetStringField(TEXT("optionCode"), Opt.OptionCode);
            OptObj->TryGetStringField(TEXT("name"), Opt.Name);
            OptObj->TryGetStringField(TEXT("description"), Opt.Description);
            OptObj->TryGetStringField(TEXT("valueType"), Opt.ValueType);
            OptObj->TryGetNumberField(TEXT("value"), Value);
            Opt.Value = (float)Value;

            Item.Options.Add(Opt);
        }
    }

    return Item;
}

// ================================================================
// 1. Login - 학번 검증만 (성공/실패)
// ================================================================

void UEclassAPIHandler::Login(FString UserId, FOnLoginComplete OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/login"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(
        FString::Printf(TEXT("{\"userId\":\"%s\"}"), *UserId)
    );

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            FLoginResult LoginResult;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[Login] Request Failed - Server Unreachable"));
                LoginResult.bLoginSuccess = false;
                LoginResult.LoginMessage = TEXT("서버 연결에 실패했습니다.");
                OnComplete.ExecuteIfBound(LoginResult);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
            {
                bool bServerSuccess = false;
                Root->TryGetBoolField(TEXT("success"), bServerSuccess);
                LoginResult.bLoginSuccess = bServerSuccess;
                Root->TryGetStringField(TEXT("message"), LoginResult.LoginMessage);

                UE_LOG(LogTemp, Log, TEXT("[Login] Result: %s | Message: %s"),
                    bServerSuccess ? TEXT("Success") : TEXT("Failed"),
                    *LoginResult.LoginMessage);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[Login] JSON Deserialization Failed"));
                LoginResult.bLoginSuccess = false;
                LoginResult.LoginMessage = TEXT("서버 응답 데이터 오류");
            }

            OnComplete.ExecuteIfBound(LoginResult);
        });

    Request->ProcessRequest();
}

// ================================================================
// 2. GetUserData - 로그인 성공 후 호출 (델타 + 리셋 정보)
// ================================================================

void UEclassAPIHandler::GetUserData(FString UserId, FOnUserSessionReceived OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/user/") + UserId);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, UserId](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            FUserSessionData SessionData;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GetUserData] Request Failed"));
                OnComplete.ExecuteIfBound(SessionData);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
            {
                // 재화 데이터 캐시 업데이트
                const TSharedPtr<FJsonObject>* UserObj;
                if (Root->TryGetObjectField(TEXT("user"), UserObj))
                {
                    UEclassAPIHandler::ApplyAndCache(ParseUserJson(*UserObj));
                }

                // 델타
                const TSharedPtr<FJsonObject>* DeltaObj;
                if (Root->TryGetObjectField(TEXT("delta"), DeltaObj))
                {
                    int32 AC = 0, EC = 0, IC = 0, E = 0;
                    (*DeltaObj)->TryGetNumberField(TEXT("academicCurrency"), AC);
                    (*DeltaObj)->TryGetNumberField(TEXT("extraCurrency"), EC);
                    (*DeltaObj)->TryGetNumberField(TEXT("idleCurrency"), IC);
                    (*DeltaObj)->TryGetNumberField(TEXT("exp"), E);
                    SessionData.Delta.AcademicCurrency = AC;
                    SessionData.Delta.ExtraCurrency = EC;
                    SessionData.Delta.IdleCurrency = IC;
                    SessionData.Delta.Exp = E;
                }

                bool bHasChange = false;
                Root->TryGetBoolField(TEXT("hasChange"), bHasChange);
                SessionData.Delta.bHasChange = bHasChange;

                // 리셋 정보
                bool bResetDone = false;
                int32 Seconds = 0;
                Root->TryGetBoolField(TEXT("resetDoneToday"), bResetDone);
                Root->TryGetNumberField(TEXT("secondsUntilReset"), Seconds);
                SessionData.bResetDoneToday = bResetDone;
                SessionData.SecondsUntilReset = Seconds;

                UE_LOG(LogTemp, Log, TEXT("[GetUserData] %s | ResetDone: %s | SecondsUntilReset: %d"),
                    *UserId,
                    bResetDone ? TEXT("true") : TEXT("false"),
                    Seconds);
            }

            OnComplete.ExecuteIfBound(SessionData);
        });

    Request->ProcessRequest();
}

// ================================================================
// 3. GetUserCurrency - 재화 전체 조회
// ================================================================

void UEclassAPIHandler::GetUserCurrency(FString UserId, FOnEclassDataReceived OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/user/") + UserId);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, UserId](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            FEclassData Data;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GetUserCurrency] Request Failed"));
                OnComplete.ExecuteIfBound(Data);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                const TSharedPtr<FJsonObject>* UserObj;
                if (Root->TryGetObjectField(TEXT("user"), UserObj))
                {
                    Data = ParseUserJson(*UserObj);
                    UEclassAPIHandler::ApplyAndCache(Data);
                }
            }

            OnComplete.ExecuteIfBound(Data);
        });

    Request->ProcessRequest();
}

// ================================================================
// 4. 재화별 개별 조회
// ================================================================

static void FetchSingleCurrency(FString UserId, FString FieldName, FOnCurrencyReceived OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://134.185.100.53:3000/api/user/") + UserId);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, FieldName, UserId](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            int32 Value = 0;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[FetchCurrency] Request Failed - %s"), *FieldName);
                OnComplete.ExecuteIfBound(Value);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                const TSharedPtr<FJsonObject>* UserObj;
                if (Root->TryGetObjectField(TEXT("user"), UserObj))
                {
                    (*UserObj)->TryGetNumberField(*FieldName, Value);
                }
            }

            UE_LOG(LogTemp, Log, TEXT("[GetCurrency] %s | %s: %d"), *UserId, *FieldName, Value);
            OnComplete.ExecuteIfBound(Value);
        });

    Request->ProcessRequest();
}

void UEclassAPIHandler::GetAcademicCurrency(FString UserId, FOnCurrencyReceived OnComplete)
{
    FetchSingleCurrency(UserId, TEXT("academicCurrency"), OnComplete);
}

void UEclassAPIHandler::GetExtraCurrency(FString UserId, FOnCurrencyReceived OnComplete)
{
    FetchSingleCurrency(UserId, TEXT("extraCurrency"), OnComplete);
}

void UEclassAPIHandler::GetIdleCurrency(FString UserId, FOnCurrencyReceived OnComplete)
{
    FetchSingleCurrency(UserId, TEXT("idleCurrency"), OnComplete);
}

void UEclassAPIHandler::GetExp(FString UserId, FOnCurrencyReceived OnComplete)
{
    FetchSingleCurrency(UserId, TEXT("exp"), OnComplete);
}

// ================================================================
// 5. SpendCurrency
// ================================================================

void UEclassAPIHandler::SpendCurrency(FString UserId, FString CurrencyType, int32 Amount, FOnSpendGoldResult OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/spend-currency"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(
        FString::Printf(TEXT("{\"userId\":\"%s\",\"currencyType\":\"%s\",\"amount\":%d}"),
            *UserId, *CurrencyType, Amount)
    );

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[SpendCurrency] Request Failed"));
                OnComplete.ExecuteIfBound(false);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                bool bSuccess2 = false;
                Root->TryGetBoolField(TEXT("success"), bSuccess2);

                if (bSuccess2)
                {
                    const TSharedPtr<FJsonObject>* UserObj;
                    if (Root->TryGetObjectField(TEXT("current"), UserObj))
                    {
                        UEclassAPIHandler::ApplyAndCache(ParseUserJson(*UserObj));
                    }
                }

                OnComplete.ExecuteIfBound(bSuccess2);
            }
        });

    Request->ProcessRequest();
}

// ================================================================
// 6. GainCurrency
// ================================================================

void UEclassAPIHandler::GainCurrency(FString UserId, int32 Amount, FString CurrencyType)
{
    TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
    JsonObj->SetStringField(TEXT("userId"), UserId);
    JsonObj->SetNumberField(TEXT("amount"), Amount);
    JsonObj->SetStringField(TEXT("currencyType"), CurrencyType);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/currency/gain"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonString);

    Request->OnProcessRequestComplete().BindLambda(
        [UserId](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (bSuccess && Res.IsValid())
            {
                TSharedPtr<FJsonObject> RootObj;
                TSharedRef<TJsonReader<>> Reader =
                    TJsonReaderFactory<>::Create(Res->GetContentAsString());
                if (FJsonSerializer::Deserialize(Reader, RootObj))
                {
                    FEclassData NewData = ParseUserJson(
                        RootObj->GetObjectField(TEXT("current")).ToSharedRef());
                    ApplyAndCache(NewData);
                    UE_LOG(LogTemp, Log, TEXT("[GainCurrency] %s - AC:%d"),
                        *UserId, NewData.AcademicCurrency);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[GainCurrency] Request Failed"));
            }
        });

    Request->ProcessRequest();
}

// ================================================================
// 7. RequestDailyReset
// ================================================================

void UEclassAPIHandler::RequestDailyReset(FString UserId, FOnDailyResetComplete OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/daily-reset"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(
        FString::Printf(TEXT("{\"userId\":\"%s\"}"), *UserId)
    );

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[DailyReset] Request Failed"));
                OnComplete.ExecuteIfBound(false);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                bool bReady = false;
                Root->TryGetBoolField(TEXT("readyForDreamShop"), bReady);

                const TSharedPtr<FJsonObject>* UserObj;
                if (Root->TryGetObjectField(TEXT("user"), UserObj))
                {
                    UEclassAPIHandler::ApplyAndCache(ParseUserJson(*UserObj));
                }

                OnComplete.ExecuteIfBound(bReady);
            }
        });

    Request->ProcessRequest();
}

// ================================================================
// 8. GetServerTime
// ================================================================

void UEclassAPIHandler::GetServerTime(FOnServerTimeReceived OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/server-time"));
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            FServerTime ServerTime;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GetServerTime] Request Failed"));
                OnComplete.ExecuteIfBound(ServerTime);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                int32 Hour = 0, Day = 0, Month = 0, Year = 0, Seconds = 0;
                Root->TryGetNumberField(TEXT("utcHour"), Hour);
                Root->TryGetNumberField(TEXT("utcDay"), Day);
                Root->TryGetNumberField(TEXT("utcMonth"), Month);
                Root->TryGetNumberField(TEXT("utcYear"), Year);
                Root->TryGetNumberField(TEXT("secondsUntilReset"), Seconds);

                ServerTime.UtcHour = Hour;
                ServerTime.UtcDay = Day;
                ServerTime.UtcMonth = Month;
                ServerTime.UtcYear = Year;
                ServerTime.SecondsUntilReset = Seconds;
            }

            OnComplete.ExecuteIfBound(ServerTime);
        });

    Request->ProcessRequest();
}

// ================================================================
// 9. GetAcademicLog
// ================================================================

void UEclassAPIHandler::GetAcademicLog(FString UserId, FOnAcademicLogReceived OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/academic-log/") + UserId);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            TArray<FAcademicLogEntry> Logs;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GetAcademicLog] Request Failed"));
                OnComplete.ExecuteIfBound(Logs);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                const TArray<TSharedPtr<FJsonValue>>* LogArray;
                if (Root->TryGetArrayField(TEXT("logs"), LogArray))
                {
                    for (const TSharedPtr<FJsonValue>& Val : *LogArray)
                    {
                        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
                        if (!Obj.IsValid()) continue;

                        FAcademicLogEntry Entry;
                        int32 Id = 0, DeltaExtra = 0, DeltaExp = 0;
                        Obj->TryGetNumberField(TEXT("id"), Id);
                        Obj->TryGetNumberField(TEXT("deltaExtra"), DeltaExtra);
                        Obj->TryGetNumberField(TEXT("deltaExp"), DeltaExp);
                        Obj->TryGetStringField(TEXT("changeType"), Entry.ChangeType);
                        Obj->TryGetStringField(TEXT("detail"), Entry.Detail);
                        Obj->TryGetStringField(TEXT("createdAt"), Entry.CreatedAt);

                        Entry.Id = Id;
                        Entry.DeltaExtra = DeltaExtra;
                        Entry.DeltaExp = DeltaExp;
                        Logs.Add(Entry);
                    }
                }
            }

            UE_LOG(LogTemp, Log, TEXT("[GetAcademicLog] %d logs received"), Logs.Num());
            OnComplete.ExecuteIfBound(Logs);
        });

    Request->ProcessRequest();
}

// ================================================================
// 10. GetInventory
// ================================================================

void UEclassAPIHandler::GetInventory(FString UserId, FString ItemType, FOnInventoryReceived OnComplete)
{
    FString URL = ItemType.IsEmpty()
        ? GAME_SERVER + TEXT("/inventory/") + UserId + TEXT("/tab")
        : GAME_SERVER + TEXT("/inventory/") + UserId + TEXT("/tab?type=") + ItemType;

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(URL);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, ItemType](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            TArray<FEclassItemInfo> Items;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GetInventory] Request Failed"));
                OnComplete.ExecuteIfBound(Items);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                const TArray<TSharedPtr<FJsonValue>>* ItemArray;
                if (Root->TryGetArrayField(TEXT("items"), ItemArray))
                {
                    for (const TSharedPtr<FJsonValue>& Val : *ItemArray)
                    {
                        Items.Add(ParseItemJson(Val->AsObject()));
                    }
                }
            }

            UE_LOG(LogTemp, Log, TEXT("[GetInventory] Type:%s | %d items"),
                ItemType.IsEmpty() ? TEXT("All") : *ItemType, Items.Num());
            OnComplete.ExecuteIfBound(Items);
        });

    Request->ProcessRequest();
}

// ================================================================
// 11. GetEquippedItems
// ================================================================

void UEclassAPIHandler::GetEquippedItems(FString UserId, FOnInventoryReceived OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/inventory/") + UserId + TEXT("/equipped"));
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            TArray<FEclassItemInfo> Items;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GetEquippedItems] Request Failed"));
                OnComplete.ExecuteIfBound(Items);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                const TArray<TSharedPtr<FJsonValue>>* ItemArray;
                if (Root->TryGetArrayField(TEXT("items"), ItemArray))
                {
                    for (const TSharedPtr<FJsonValue>& Val : *ItemArray)
                    {
                        Items.Add(ParseItemJson(Val->AsObject()));
                    }
                }
            }

            UE_LOG(LogTemp, Log, TEXT("[GetEquippedItems] %d items"), Items.Num());
            OnComplete.ExecuteIfBound(Items);
        });

    Request->ProcessRequest();
}

// ================================================================
// 12. EquipItem
// ================================================================

void UEclassAPIHandler::EquipItem(FString UserId, FString ItemCode, FOnEquipResult OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/inventory/equip"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(
        FString::Printf(TEXT("{\"userId\":\"%s\",\"itemCode\":\"%s\"}"), *UserId, *ItemCode)
    );

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, ItemCode](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[EquipItem] Request Failed"));
                OnComplete.ExecuteIfBound(false);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                bool bOk = false;
                Root->TryGetBoolField(TEXT("success"), bOk);
                UE_LOG(LogTemp, Log, TEXT("[EquipItem] %s - %s"),
                    *ItemCode, bOk ? TEXT("Success") : TEXT("Failed"));
                OnComplete.ExecuteIfBound(bOk);
            }
        });

    Request->ProcessRequest();
}

// ================================================================
// 13. UnequipItem
// ================================================================

void UEclassAPIHandler::UnequipItem(FString UserId, FString ItemCode, FOnEquipResult OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/inventory/unequip"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(
        FString::Printf(TEXT("{\"userId\":\"%s\",\"itemCode\":\"%s\"}"), *UserId, *ItemCode)
    );

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, ItemCode](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[UnequipItem] Request Failed"));
                OnComplete.ExecuteIfBound(false);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                bool bOk = false;
                Root->TryGetBoolField(TEXT("success"), bOk);
                UE_LOG(LogTemp, Log, TEXT("[UnequipItem] %s - %s"),
                    *ItemCode, bOk ? TEXT("Success") : TEXT("Failed"));
                OnComplete.ExecuteIfBound(bOk);
            }
        });

    Request->ProcessRequest();
}

// ================================================================
// 14. GetCollection
// ================================================================

void UEclassAPIHandler::GetCollection(FString UserId, FString CollectionType, FOnCollectionReceived OnComplete)
{
    FString URL = CollectionType.IsEmpty()
        ? GAME_SERVER + TEXT("/collection/") + UserId
        : GAME_SERVER + TEXT("/collection/") + UserId + TEXT("?type=") + CollectionType;

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(URL);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            TArray<FEclassCollectionEntry> Entries;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GetCollection] Request Failed"));
                OnComplete.ExecuteIfBound(Entries);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                const TArray<TSharedPtr<FJsonValue>>* EntryArray;
                if (Root->TryGetArrayField(TEXT("entries"), EntryArray))
                {
                    for (const TSharedPtr<FJsonValue>& Val : *EntryArray)
                    {
                        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
                        if (!Obj.IsValid()) continue;

                        FEclassCollectionEntry Entry;
                        bool bUnlocked = false;
                        Obj->TryGetStringField(TEXT("itemCode"), Entry.CollectionCode);
                        Obj->TryGetStringField(TEXT("name"), Entry.Name);
                        Obj->TryGetStringField(TEXT("description"), Entry.Description);
                        Obj->TryGetStringField(TEXT("collectionType"), Entry.CollectionType);
                        Obj->TryGetStringField(TEXT("unlockedAt"), Entry.UnlockedAt);
                        Obj->TryGetBoolField(TEXT("isUnlocked"), bUnlocked);
                        Entry.bIsUnlocked = bUnlocked;

                        Entries.Add(Entry);
                    }
                }
            }

            UE_LOG(LogTemp, Log, TEXT("[GetCollection] %d entries"), Entries.Num());
            OnComplete.ExecuteIfBound(Entries);
        });

    Request->ProcessRequest();
}

// ================================================================
// 15. GetUnlockedItemCodes
// ================================================================

void UEclassAPIHandler::GetUnlockedItemCodes(FString UserId, FString CollectionType, FOnUnlockedCodesReceived OnComplete)
{
    FString URL = CollectionType.IsEmpty()
        ? GAME_SERVER + TEXT("/collection/") + UserId + TEXT("/unlocked")
        : GAME_SERVER + TEXT("/collection/") + UserId + TEXT("/unlocked?type=") + CollectionType;

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(URL);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, CollectionType](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            TArray<FString> ItemCodes;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GetUnlockedItemCodes] Request Failed"));
                OnComplete.ExecuteIfBound(ItemCodes);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                const TArray<TSharedPtr<FJsonValue>>* CodesArray;
                if (Root->TryGetArrayField(TEXT("itemCodes"), CodesArray))
                {
                    for (const TSharedPtr<FJsonValue>& Val : *CodesArray)
                    {
                        ItemCodes.Add(Val->AsString());
                    }
                }
            }

            UE_LOG(LogTemp, Log, TEXT("[GetUnlockedItemCodes] Type:%s | %d codes"),
                CollectionType.IsEmpty() ? TEXT("All") : *CollectionType, ItemCodes.Num());
            OnComplete.ExecuteIfBound(ItemCodes);
        });

    Request->ProcessRequest();
}

// ================================================================
// 16. GetShop
// ItemType: "" = 전체 / "Hat" / "Bag" / ...
// ================================================================

void UEclassAPIHandler::GetShop(FString ItemType, FOnShopReceived OnComplete)
{
    FString URL = ItemType.IsEmpty()
        ? GAME_SERVER + TEXT("/shop")
        : GAME_SERVER + TEXT("/shop?type=") + ItemType;

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(URL);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, ItemType](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            TArray<FShopItem> Items;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GetShop] Request Failed"));
                OnComplete.ExecuteIfBound(Items);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                const TArray<TSharedPtr<FJsonValue>>* ItemArray;
                if (Root->TryGetArrayField(TEXT("items"), ItemArray))
                {
                    for (const TSharedPtr<FJsonValue>& Val : *ItemArray)
                    {
                        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
                        if (!Obj.IsValid()) continue;

                        FShopItem Item;
                        int32 Price = 0;
                        Obj->TryGetStringField(TEXT("shopId"), Item.ShopId);
                        Obj->TryGetStringField(TEXT("itemCode"), Item.ItemCode);
                        Obj->TryGetStringField(TEXT("name"), Item.Name);
                        Obj->TryGetStringField(TEXT("description"), Item.Description);
                        Obj->TryGetStringField(TEXT("itemType"), Item.ItemType);
                        Obj->TryGetStringField(TEXT("cosmeticSlot"), Item.CosmeticSlot);
                        Obj->TryGetStringField(TEXT("currencyType"), Item.CurrencyType);
                        Obj->TryGetNumberField(TEXT("price"), Price);
                        Item.Price = Price;

                        Items.Add(Item);
                    }
                }
            }

            UE_LOG(LogTemp, Log, TEXT("[GetShop] Type:%s | %d items"),
                ItemType.IsEmpty() ? TEXT("All") : *ItemType, Items.Num());
            OnComplete.ExecuteIfBound(Items);
        });

    Request->ProcessRequest();
}

// ================================================================
// 17. BuyItem
// ================================================================

void UEclassAPIHandler::BuyItem(FString UserId, FString ShopId, FOnBuyItemResult OnComplete)
{
    TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
    JsonObj->SetStringField(TEXT("userId"), UserId);
    JsonObj->SetStringField(TEXT("shopId"), ShopId);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/shop/buy"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(JsonString);

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, ShopId](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[BuyItem] Request Failed"));
                OnComplete.ExecuteIfBound(false);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                bool bOk = false;
                FString Message;
                Root->TryGetBoolField(TEXT("success"), bOk);
                Root->TryGetStringField(TEXT("message"), Message);

                // 구매 후 캐시 업데이트
                if (bOk)
                {
                    const TSharedPtr<FJsonObject>* UserObj;
                    if (Root->TryGetObjectField(TEXT("current"), UserObj))
                    {
                        UEclassAPIHandler::ApplyAndCache(ParseUserJson(*UserObj));
                    }
                }

                UE_LOG(LogTemp, Log, TEXT("[BuyItem] ShopId:%s | %s | %s"),
                    *ShopId,
                    bOk ? TEXT("Success") : TEXT("Failed"),
                    *Message);
                OnComplete.ExecuteIfBound(bOk);
            }
        });

    Request->ProcessRequest();
}

// ================================================================
// 18. CraftItem
// ================================================================

void UEclassAPIHandler::CraftItem(FString UserId, FString CraftId, FOnCraftItemResult OnComplete)
{
    TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
    JsonObj->SetStringField(TEXT("userId"), UserId);
    JsonObj->SetStringField(TEXT("craftId"), CraftId);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/craft"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(JsonString);

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, CraftId](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[CraftItem] Request Failed"));
                OnComplete.ExecuteIfBound(false);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                bool bOk = false;
                FString Message;
                Root->TryGetBoolField(TEXT("success"), bOk);
                Root->TryGetStringField(TEXT("message"), Message);

                // 제작 성공 시 재화 캐시 업데이트
                if (bOk)
                {
                    const TSharedPtr<FJsonObject>* UserObj;
                    if (Root->TryGetObjectField(TEXT("current"), UserObj))
                    {
                        UEclassAPIHandler::ApplyAndCache(ParseUserJson(*UserObj));
                    }
                }

                UE_LOG(LogTemp, Log, TEXT("[CraftItem] CraftId:%s | %s | %s"),
                    *CraftId,
                    bOk ? TEXT("Success") : TEXT("Failed"),
                    *Message);
                OnComplete.ExecuteIfBound(bOk);
            }
        });

    Request->ProcessRequest();
}

// ================================================================
// GetDreamShop
// ================================================================

void UEclassAPIHandler::GetDreamShop(FString UserId, FOnDreamShopReceived OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/dream-shop/") + UserId);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            FDreamShopResult Result;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[GetDreamShop] Request Failed"));
                OnComplete.ExecuteIfBound(Result);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
            {
                bool bOk = false;
                Root->TryGetBoolField(TEXT("success"), bOk);
                Root->TryGetStringField(TEXT("message"), Result.Message);
                Result.bSuccess = bOk;

                if (bOk)
                {
                    int32 MaxBuy = 1, UsedBuy = 0;
                    Root->TryGetNumberField(TEXT("maxBuyCount"), MaxBuy);
                    Root->TryGetNumberField(TEXT("usedBuyCount"), UsedBuy);
                    Result.MaxBuyCount = MaxBuy;
                    Result.UsedBuyCount = UsedBuy;

                    const TArray<TSharedPtr<FJsonValue>>* ItemArray;
                    if (Root->TryGetArrayField(TEXT("items"), ItemArray))
                    {
                        for (const TSharedPtr<FJsonValue>& Val : *ItemArray)
                        {
                            const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
                            if (!Obj.IsValid()) continue;

                            FDreamShopItem Item;
                            bool bBought = false;
                            double Multiplier = 1.0;
                            Obj->TryGetStringField(TEXT("itemCode"), Item.ItemCode);
                            Obj->TryGetStringField(TEXT("grade"), Item.Grade);
                            Obj->TryGetNumberField(TEXT("multiplier"), Multiplier);
                            Obj->TryGetBoolField(TEXT("bought"), bBought);
                            Item.Multiplier = (float)Multiplier;
                            Item.bBought = bBought;

                            // 옵션 파싱
                            const TArray<TSharedPtr<FJsonValue>>* OptArray;
                            if (Obj->TryGetArrayField(TEXT("options"), OptArray))
                            {
                                for (const TSharedPtr<FJsonValue>& OptVal : *OptArray)
                                {
                                    const TSharedPtr<FJsonObject>& OptObj = OptVal->AsObject();
                                    if (!OptObj.IsValid()) continue;

                                    FDreamShopOption Opt;
                                    double OptValue = 1.0;
                                    OptObj->TryGetStringField(TEXT("optionCode"), Opt.OptionCode);
                                    OptObj->TryGetStringField(TEXT("grade"), Opt.Grade);
                                    OptObj->TryGetNumberField(TEXT("value"), OptValue);
                                    Opt.Value = (float)OptValue;
                                    Item.Options.Add(Opt);
                                }
                            }

                            Result.Items.Add(Item);
                        }
                    }
                }

                UE_LOG(LogTemp, Log, TEXT("[GetDreamShop] %s | Items: %d | MaxBuy: %d"),
                    bOk ? TEXT("Success") : TEXT("Failed"),
                    Result.Items.Num(),
                    Result.MaxBuyCount);
            }

            OnComplete.ExecuteIfBound(Result);
        });

    Request->ProcessRequest();
}

// ================================================================
// BuyDreamShopItem
// ================================================================

void UEclassAPIHandler::BuyDreamShopItem(FString UserId, int32 ItemIndex, FOnDreamShopBuyResult OnComplete)
{
    TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
    JsonObj->SetStringField(TEXT("userId"), UserId);
    JsonObj->SetNumberField(TEXT("itemIndex"), ItemIndex);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/dream-shop/buy"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(JsonString);

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[BuyDreamShopItem] Request Failed"));
                OnComplete.ExecuteIfBound(false);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                bool bOk = false;
                FString Message;
                int32 ExpCost = 0;
                Root->TryGetBoolField(TEXT("success"), bOk);
                Root->TryGetStringField(TEXT("message"), Message);
                Root->TryGetNumberField(TEXT("expCost"), ExpCost);

                // 구매 성공 시 재화 캐시 업데이트
                if (bOk)
                {
                    const TSharedPtr<FJsonObject>* UserObj;
                    if (Root->TryGetObjectField(TEXT("current"), UserObj))
                    {
                        UEclassAPIHandler::ApplyAndCache(ParseUserJson(*UserObj));
                    }
                }

                UE_LOG(LogTemp, Log, TEXT("[BuyDreamShopItem] %s | %s | ExpCost: %d"),
                    bOk ? TEXT("Success") : TEXT("Failed"), *Message, ExpCost);

                OnComplete.ExecuteIfBound(bOk);
            }
        });

    Request->ProcessRequest();
}

// ================================================================
// EquipConsumable - Consumable 전용 장착
// ================================================================

void UEclassAPIHandler::EquipConsumable(FString UserId, FString ItemCode, FOnEquipResult OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/inventory/equip-consumable"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(
        FString::Printf(TEXT("{\"userId\":\"%s\",\"itemCode\":\"%s\"}"), *UserId, *ItemCode)
    );

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, ItemCode](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[EquipConsumable] Request Failed"));
                OnComplete.ExecuteIfBound(false);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                bool bOk = false;
                FString Unequipped;
                Root->TryGetBoolField(TEXT("success"), bOk);
                Root->TryGetStringField(TEXT("unequipped"), Unequipped);

                UE_LOG(LogTemp, Log, TEXT("[EquipConsumable] %s - %s | Unequipped: %s"),
                    *ItemCode,
                    bOk ? TEXT("Success") : TEXT("Failed"),
                    Unequipped.IsEmpty() ? TEXT("None") : *Unequipped);
                OnComplete.ExecuteIfBound(bOk);
            }
        });

    Request->ProcessRequest();
}

// ================================================================
// 19. FindRecipe - 재화 조합으로 레시피 찾기
// ================================================================

void UEclassAPIHandler::FindRecipe(int32 Academic, int32 Extra, int32 Idle, FOnFindRecipeResult OnComplete)
{
    TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
    JsonObj->SetNumberField(TEXT("academic"), Academic);
    JsonObj->SetNumberField(TEXT("extra"), Extra);
    JsonObj->SetNumberField(TEXT("idle"), Idle);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/craft/find"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(JsonString);

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[FindRecipe] Request Failed"));
                OnComplete.ExecuteIfBound(false, TEXT(""));
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                bool bOk = false;
                FString CraftId;
                FString Message;
                Root->TryGetBoolField(TEXT("success"), bOk);
                Root->TryGetStringField(TEXT("craftId"), CraftId);
                Root->TryGetStringField(TEXT("message"), Message);

                UE_LOG(LogTemp, Log, TEXT("[FindRecipe] %s | CraftId: %s"),
                    bOk ? TEXT("Found") : *Message, *CraftId);
                OnComplete.ExecuteIfBound(bOk, CraftId);
            }
        });

    Request->ProcessRequest();
}

// ================================================================
// 캐시
// ================================================================

void UEclassAPIHandler::ApplyAndCache(FEclassData NewData)
{
    CachedData = NewData;
    SaveEclassData();

    UE_LOG(LogTemp, Warning, TEXT("[Cache] StudentId: %s | Academic: %d, Extra: %d, Idle: %d, Exp: %d"),
        *CachedData.StudentId,
        CachedData.AcademicCurrency,
        CachedData.ExtraCurrency,
        CachedData.IdleCurrency,
        CachedData.Exp);
}

FEclassData UEclassAPIHandler::GetCachedData()
{
    return CachedData;
}

void UEclassAPIHandler::SaveEclassData()
{
    UEclassSaveGame* Save = Cast<UEclassSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UEclassSaveGame::StaticClass()));
    if (Save)
    {
        Save->StudentId = CachedData.StudentId;
        UGameplayStatics::SaveGameToSlot(Save, TEXT("EclassSlot"), 0);
    }
}

void UEclassAPIHandler::LoadEclassData()
{
    if (UGameplayStatics::DoesSaveGameExist(TEXT("EclassSlot"), 0))
    {
        UEclassSaveGame* Load = Cast<UEclassSaveGame>(
            UGameplayStatics::LoadGameFromSlot(TEXT("EclassSlot"), 0));
        if (Load)
        {
            CachedData.StudentId = Load->StudentId;
        }
    }
}