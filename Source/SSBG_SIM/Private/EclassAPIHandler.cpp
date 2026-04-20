#include "EclassAPIHandler.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "EclassSaveGame.h"

FEclassData UEclassAPIHandler::CachedData;

<<<<<<< HEAD
static const FString GAME_SERVER = TEXT("http://localhost:3000/api");
=======
// 오라클 서버 주소
static const FString GAME_SERVER = TEXT("http://134.185.100.53:3000/api");
// 로컬 서버 주소
//static const FString GAME_SERVER = TEXT("http://127.0.0.1:3000/api");
>>>>>>> 5f3b77f16368d6f5cf6fa920c803e2457353785c

static FEclassData ParseUserJson(TSharedPtr<FJsonObject> UserObj)
{
    FEclassData Result;
    if (!UserObj.IsValid()) return Result;

    int32 AC = 0, EC = 0, IC = 0, E = 0;
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

// 1. Login
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
        [OnComplete, UserId](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            FLoginResult LoginResult;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[Login] Request Failed - Using Local Data"));
                UEclassAPIHandler::LoadEclassData();
                LoginResult.Data = UEclassAPIHandler::CachedData;
                OnComplete.ExecuteIfBound(LoginResult);
                return;
            }

            TSharedPtr<FJsonObject> Root;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (FJsonSerializer::Deserialize(Reader, Root))
            {
                // user 파싱
                const TSharedPtr<FJsonObject>* UserObj;
                if (Root->TryGetObjectField(TEXT("user"), UserObj))
                {
                    LoginResult.Data = ParseUserJson(*UserObj);
                    UEclassAPIHandler::ApplyAndCache(LoginResult.Data);

                    UE_LOG(LogTemp, Log, TEXT("[Login] Synced. User: %s | AC: %d, EC: %d, IC: %d, EXP: %d"),
                        *UserId,
                        LoginResult.Data.AcademicCurrency,
                        LoginResult.Data.ExtraCurrency,
                        LoginResult.Data.IdleCurrency,
                        LoginResult.Data.Exp);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[Login] 'user' field missing!"));
                }

                // delta 파싱
                const TSharedPtr<FJsonObject>* DeltaObj;
                if (Root->TryGetObjectField(TEXT("delta"), DeltaObj))
                {
                    int32 AC = 0, EC = 0, IC = 0, E = 0;
                    (*DeltaObj)->TryGetNumberField(TEXT("academicCurrency"), AC);
                    (*DeltaObj)->TryGetNumberField(TEXT("extraCurrency"), EC);
                    (*DeltaObj)->TryGetNumberField(TEXT("idleCurrency"), IC);
                    (*DeltaObj)->TryGetNumberField(TEXT("exp"), E);
                    LoginResult.Delta.AcademicCurrency = AC;
                    LoginResult.Delta.ExtraCurrency = EC;
                    LoginResult.Delta.IdleCurrency = IC;
                    LoginResult.Delta.Exp = E;
                }

                bool bHasChange = false;
                Root->TryGetBoolField(TEXT("hasChange"), bHasChange);
                LoginResult.Delta.bHasChange = bHasChange;

                // ✅ 시간 정보 파싱 (표시 전용)
                bool bResetDone = false;
                int32 Seconds = 0;
                Root->TryGetBoolField(TEXT("resetDoneToday"), bResetDone);
                Root->TryGetNumberField(TEXT("secondsUntilReset"), Seconds);
                LoginResult.bResetDoneToday = bResetDone;
                LoginResult.SecondsUntilReset = Seconds;

                UE_LOG(LogTemp, Log, TEXT("[Login] ResetDone: %s | SecondsUntilReset: %d"),
                    bResetDone ? TEXT("true") : TEXT("false"), Seconds);
            }

            OnComplete.ExecuteIfBound(LoginResult);
        });

    Request->ProcessRequest();
}

// 2. SpendCurrency
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
                        FEclassData Data = ParseUserJson(*UserObj);
                        UEclassAPIHandler::ApplyAndCache(Data);
                    }
                }

                OnComplete.ExecuteIfBound(bSuccess2);
            }
        });

    Request->ProcessRequest();
}

// 3. 캐시 반환 / 저장 / 불러오기
FEclassData UEclassAPIHandler::GetEclassData()
{
    return CachedData;
}

void UEclassAPIHandler::ApplyAndCache(FEclassData NewData)
{
    CachedData = NewData;
    SaveEclassData();

    UE_LOG(LogTemp, Warning, TEXT("[Cache] Academic: %d, Extra: %d, Idle: %d, Exp: %d"),
        CachedData.AcademicCurrency,
        CachedData.ExtraCurrency,
        CachedData.IdleCurrency,
        CachedData.Exp);
}

void UEclassAPIHandler::SaveEclassData()
{
    UEclassSaveGame* Save = Cast<UEclassSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UEclassSaveGame::StaticClass()));
    if (Save)
    {
        Save->AcademicCurrency = CachedData.AcademicCurrency;
        Save->ExtraCurrency = CachedData.ExtraCurrency;
        Save->IdleCurrency = CachedData.IdleCurrency;
        Save->Exp = CachedData.Exp;
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
            CachedData.AcademicCurrency = Load->AcademicCurrency;
            CachedData.ExtraCurrency = Load->ExtraCurrency;
            CachedData.IdleCurrency = Load->IdleCurrency;
            CachedData.Exp = Load->Exp;
        }
    }
}

// 4. RequestDailyReset (수동 호출용 - 보통은 cron이 처리)
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

// 5. GainCurrency
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

    Request->OnProcessRequestComplete().BindLambda([UserId](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (bSuccess && Res.IsValid())
            {
                TSharedPtr<FJsonObject> RootObj;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Res->GetContentAsString());

                if (FJsonSerializer::Deserialize(Reader, RootObj))
                {
                    FEclassData NewSyncedData = ParseUserJson(RootObj->GetObjectField(TEXT("current")).ToSharedRef());
                    ApplyAndCache(NewSyncedData);
                    UE_LOG(LogTemp, Log, TEXT("[GainCurrency] %s - AC:%d"), *UserId, NewSyncedData.AcademicCurrency);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[GainCurrency] Request Failed"));
            }
        });

    Request->ProcessRequest();
}

// 6. GetServerTime (표시 전용)
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