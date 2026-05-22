#pragma once
#include "pch.h"
#include "SharedPointer.h"

struct FCurlHttpResponse;
struct FCurlHttpRequest;

typedef TSharedPtr<FCurlHttpRequest, ESPMode::ThreadSafe> FHttpRequestPtr;
typedef TSharedPtr<FCurlHttpResponse, ESPMode::ThreadSafe> FHttpResponsePtr;

struct FCurlHttpRequest
{
public:
    void** VTable;
    /*FCurlHttpRequest()
    {
        static auto Constructor = (void (*)(FCurlHttpRequest*))(ImageBase + 0x2072D54);

        Constructor(this);
    }*/

    void SetURL(FString&& InURL)
    {
        static auto InternalSetURL = (void (*)(FCurlHttpRequest*, FString&))(ImageBase + 0x2768148);

        return InternalSetURL(this, InURL);
    }

    void SetVerb(FString&& InVerb)
    {
        static auto InternalSetVerb = (void (*)(FCurlHttpRequest*, FString&))(ImageBase + 0x2768090);

        return InternalSetVerb(this, InVerb);
    }

    void SetHeader(FString&& HeaderName, FString&& HeaderValue)
    {
        static auto InternalSetHeader = (void (*)(FCurlHttpRequest*, FString&, FString&))(ImageBase + 0x2769FA4);

        return InternalSetHeader(this, HeaderName, HeaderValue);
    }

    void SetContent(TArray<unsigned char>&& ContentPayload)
    {
        static auto InternalSetContent = (void (*)(FCurlHttpRequest*, TArray<unsigned char>&))(ImageBase + 0x3C4332C);

        return InternalSetContent(this, ContentPayload);
    }

    void SetContent(const TArray<unsigned char>&& ContentPayload)
    {
        static auto InternalSetContent = (void (*)(FCurlHttpRequest*, const TArray<unsigned char>&))(ImageBase + 0x5F906B8);

        return InternalSetContent(this, ContentPayload);
    }

    void SetContentAsString(FString&& ContentString)
    {
        static auto InternalSetContentAsString = (void (*)(FCurlHttpRequest*, FString&))(ImageBase + 0x3C431D8);

        return InternalSetContentAsString(this, ContentString);
    }

    void ProcessRequest()
    {
        static auto InternalProcessRequest = (void (*)(FCurlHttpRequest*))(ImageBase + 0x2C0DC40);

        return InternalProcessRequest(this);
    }

    void OnRequestComplete(void* ThisPointer, void (*CallbackFunction)(void*, FHttpRequestPtr, FHttpResponsePtr, bool))
    {
        static auto BindRaw = (void (*)(void*, void*, void*))(ImageBase + 0x6C4CEC0);

        BindRaw(LPVOID(__int64(this) + 0x18), ThisPointer, CallbackFunction);
    }
};

struct FCurlHttpResponse
{
    void** VTable;

    FString GetHeader(FString&& HeaderName)
    {
        static auto InternalGetHeader = (void (*)(FCurlHttpResponse*, FString*, FString&))(ImageBase + 0x3227FA8);

        FString result;

        InternalGetHeader(this, &result, HeaderName);

        return result;
    }

    const TArray<unsigned char>& GetContent()
    {
        static auto InternalGetContent = (const TArray<unsigned char>& (*)(FCurlHttpResponse*))(ImageBase + 0x206D064);

        return InternalGetContent(this);
    }

    int GetResponseCode()
    {
        static auto InternalGetResponseCode = (int (*)(FCurlHttpResponse*))(ImageBase + 0x4B3D250);

        return InternalGetResponseCode(this);
    }

    FString GetContentAsString()
    {
        static auto InternalGetContentAsString = (void (*)(FCurlHttpResponse*, FString*))(ImageBase + 0x35E5EA4);

        FString result;

        InternalGetContentAsString(this, &result);

        return result;
    }
};

inline TSharedRef<FCurlHttpRequest, ESPMode::ThreadSafe> CreateRequest()
{
    static auto InternalCreateRequest = (void (*)(void*, TSharedRef<FCurlHttpRequest, ESPMode::ThreadSafe>*))(ImageBase + 0x20737F4);
    static auto GetHttpModule = (void* (*)())(ImageBase + 0x1E5F354);

    TSharedRef<FCurlHttpRequest, ESPMode::ThreadSafe> OutRequest;

    InternalCreateRequest(GetHttpModule(), &OutRequest);

    return OutRequest;
}