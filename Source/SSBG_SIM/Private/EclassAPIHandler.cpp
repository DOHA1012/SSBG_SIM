#include "EclassAPIHandler.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "EclassSaveGame.h"

FEclassData UEclassAPIHandler::CachedData;

static const FString GAME_SERVER = TEXT("http://134.185.100.53:3000/api");

// 공통: JSON 파싱 헬퍼
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

// 1. Login - 접속 시 서버 DB에서 데이터 수신
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
            FEclassData Data;
            FEclassDelta Delta;

            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[Login] Request Failed - Using Local Data"));
                UEclassAPIHandler::LoadEclassData();
                OnComplete.ExecuteIfBound(UEclassAPIHandler::CachedData, Delta);
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
                    Data = ParseUserJson(*UserObj);
                    UEclassAPIHandler::ApplyAndCache(Data);
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
                    Delta.AcademicCurrency = AC;
                    Delta.ExtraCurrency = EC;
                    Delta.IdleCurrency = IC;
                    Delta.Exp = E;
                }

                bool bHasChange = false;
                Root->TryGetBoolField(TEXT("hasChange"), bHasChange);
                Delta.bHasChange = bHasChange;
            }

            OnComplete.ExecuteIfBound(Data, Delta);
        });

    Request->ProcessRequest();
}

// 2. SpendCurrency - 재화 사용 요청
void UEclassAPIHandler::SpendCurrency(FString UserId, FString CurrencyType, int32 Amount, FOnSpendGoldResult OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GAME_SERVER + TEXT("/spend-currency"));  // 수정
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(
        FString::Printf(TEXT("{\"userId\":\"%s\",\"currencyType\":\"%s\",\"amount\":%d}"),
            *UserId, *CurrencyType, Amount)  // currencyType 추가
    );

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[SpendCurrency] Request Failed"));  // 한글 제거
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
    SaveEclassData(); // 서버 응답 받을 때마다 로컬에도 저장

    // 로그 출력 부분도 3종 재화에 맞게 수정합니다.
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

                // 재화 갱신
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

void UEclassAPIHandler::SendCurrencyUpdate(int32 Amount, FString ChangeType)
{
    // JSON 데이터 바구니 생성
    TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
    JsonObj->SetStringField(TEXT("user_id"), TEXT("MyUser_01"));
    JsonObj->SetNumberField(TEXT("amount"), Amount);
    JsonObj->SetStringField(TEXT("type"), ChangeType);

    // 바구니 내용을 텍스트로 변환 (직렬화)
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

    // HTTP 요청 만들기 및 설정
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://134.185.100.53:3000/api/currency/update")); // Node.js 서버 주소
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonString);

    // 답장 오면 실행할 함수 연결: 람다 함수 활용
    Request->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
        {
            if (bSuccess && Res.IsValid())
            {
                UE_LOG(LogTemp, Log, TEXT("서버 응답 성공: %s"), *Res->GetContentAsString());
                // 주의: 라이브러리는 화면이 없어서 여기서 바로 UI를 갱신하기는 어렵습니다.
                // 보통은 로그를 찍거나 전역 매니저에 데이터를 넘깁니다.
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("서버 통신 실패"));
            }
        });

    // 발송
    Request->ProcessRequest();
}

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