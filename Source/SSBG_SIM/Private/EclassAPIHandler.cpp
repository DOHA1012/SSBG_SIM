#include "EclassAPIHandler.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "EclassSaveGame.h"

FEclassData UEclassAPIHandler::CachedData;

static const FString GAME_SERVER = TEXT("http://127.0.0.1:3000/api");

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