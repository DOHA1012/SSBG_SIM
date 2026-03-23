// EclassAPIHandler.cpp
#include "EclassAPIHandler.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "EclassSaveGame.h"
#include "Engine/DataTable.h"
#include "EclassTableRow.h"

FEclassData UEclassAPIHandler::CachedData;

void UEclassAPIHandler::SyncEclass(FString UserId)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL("http://127.0.0.1:3000/api/sync-eclass");
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");

    FString JsonBody = FString::Printf(TEXT("{\"userId\":\"%s\"}"), *UserId);
    Request->SetContentAsString(JsonBody);

    Request->OnProcessRequestComplete().BindLambda(
        [](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
        {
            if (bSuccess && Res.IsValid())
            {
                FString Response = Res->GetContentAsString();
                UE_LOG(LogTemp, Warning, TEXT("Response: %s"), *Response);

                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);

                if (FJsonSerializer::Deserialize(Reader, JsonObject))
                {
                    // 수정: TEXT() 매크로 추가
                    int32 GoldVal = 0, ExpVal = 0;
                    JsonObject->TryGetNumberField(TEXT("gold"), GoldVal);
                    JsonObject->TryGetNumberField(TEXT("exp"), ExpVal);
                    UEclassAPIHandler::CachedData.Gold = GoldVal;
                    UEclassAPIHandler::CachedData.Exp = ExpVal;

                    UE_LOG(LogTemp, Warning, TEXT("API Data - Gold: %d, Exp: %d"),
                        UEclassAPIHandler::CachedData.Gold,
                        UEclassAPIHandler::CachedData.Exp);

                    UEclassAPIHandler::SaveEclassData();
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HTTP Request Failed"));
            }
        });

    Request->ProcessRequest();
}

FEclassData UEclassAPIHandler::GetEclassData()
{
    return CachedData;
}

void UEclassAPIHandler::SaveEclassData()
{
    UEclassSaveGame* SaveGameInstance =
        Cast<UEclassSaveGame>(UGameplayStatics::CreateSaveGameObject(UEclassSaveGame::StaticClass()));

    if (SaveGameInstance)
    {
        SaveGameInstance->Gold = CachedData.Gold;
        SaveGameInstance->Exp = CachedData.Exp;

        bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("EclassSlot"), 0);
        UE_LOG(LogTemp, Warning, TEXT("Save %s"), bSaved ? TEXT("Success") : TEXT("Failed"));
    }
}

void UEclassAPIHandler::LoadEclassData()
{
    if (UGameplayStatics::DoesSaveGameExist(TEXT("EclassSlot"), 0))
    {
        UEclassSaveGame* LoadGame =
            Cast<UEclassSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("EclassSlot"), 0));

        if (LoadGame)
        {
            CachedData.Gold = LoadGame->Gold;
            CachedData.Exp = LoadGame->Exp;
            UE_LOG(LogTemp, Warning, TEXT("Load Complete - Gold: %d, Exp: %d"),
                CachedData.Gold,
                CachedData.Exp);
        }
    }
}

FEclassData UEclassAPIHandler::GetDataFromTable(UDataTable* Table)
{
    FEclassData Result; // 수정: 구조체 기본값(Gold=0, Exp=0)으로 초기화됨
    if (!Table) return Result;

    static const FString ContextString(TEXT("EclassData"));
    FEclassTableRow* Row = Table->FindRow<FEclassTableRow>(TEXT("Player"), ContextString);

    if (Row)
    {
        Result.Gold = Row->Gold;
        Result.Exp = Row->Exp;
        UE_LOG(LogTemp, Warning, TEXT("DataTable - Gold: %d, Exp: %d"),
            Result.Gold,
            Result.Exp);
    }

    return Result;
}