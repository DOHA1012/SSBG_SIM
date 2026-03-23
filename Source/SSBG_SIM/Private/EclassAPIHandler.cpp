#include "EclassAPIHandler.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IHttpRequest.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

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
                    int32 Gold = JsonObject->GetIntegerField("gold");
                    int32 Exp = JsonObject->GetIntegerField("exp");

                    UE_LOG(LogTemp, Warning, TEXT("Gold: %d"), Gold);
                    UE_LOG(LogTemp, Warning, TEXT("Exp: %d"), Exp);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HTTP Request Failed"));
            }
        });

    Request->ProcessRequest();
}