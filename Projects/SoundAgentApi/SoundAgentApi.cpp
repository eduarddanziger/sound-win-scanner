#include "stdafx.h"

#include "SoundAgentApi.h"

#include "public/SoundAgentInterface.h"
#include "OsInfo.h"
#include "ApiClient/common/ClassDefHelper.h"

#include "VersionInformation.h"

#include <algorithm>
#include <crtdbg.h>
#include <intsafe.h>

#include "ApiClient/common/SpdLogger/Logger.h"

namespace {
    struct HandleContext {
        std::unique_ptr<SoundDeviceCollectionInterface> DeviceCollection;
        std::unique_ptr<SoundDeviceObserverInterface> DeviceCollectionObserver;
    };

    HandleContext* GetHandleContextOrNull(const SaaHandle handle)
    {
        return reinterpret_cast<HandleContext*>(handle);
    }
}

class DllObserver final : public SoundDeviceObserverInterface {
public:
    explicit DllObserver(SoundDeviceCollectionInterface* deviceCollection
        , TSaaDefaultChangedCallback defaultRenderChangedCallback
        , TSaaDefaultChangedCallback defaultCaptureChangedCallback)
        : deviceCollection_(deviceCollection)
        , defaultRenderChangedCallback_(defaultRenderChangedCallback)
        , defaultCaptureChangedCallback_(defaultCaptureChangedCallback)
    {
    }
    DISALLOW_COPY_MOVE(DllObserver);
    ~DllObserver() override;

    void OnCollectionChanged(SoundDeviceEventType event, const std::string& devicePnpId) override;

private:
    SoundDeviceCollectionInterface* deviceCollection_;
    TSaaDefaultChangedCallback defaultRenderChangedCallback_;
    TSaaDefaultChangedCallback defaultCaptureChangedCallback_;
};

DllObserver::~DllObserver() = default;


void DllObserver::OnCollectionChanged(SoundDeviceEventType event, const std::string& devicePnpId)
{
    if (defaultRenderChangedCallback_ != nullptr)
    {
        if (event == SoundDeviceEventType::DefaultRenderChanged)
        {
            defaultRenderChangedCallback_(devicePnpId.empty() ? SaaDefaultRenderDetached : SaaDefaultRenderAttached);
        }
        if (
            (event == SoundDeviceEventType::VolumeRenderChanged || event == SoundDeviceEventType::VolumeCaptureChanged)
            && deviceCollection_ != nullptr && deviceCollection_->GetDefaultRenderDevicePnpId() == devicePnpId
        )
        {
            defaultRenderChangedCallback_(event == SoundDeviceEventType::VolumeRenderChanged
                ? SaaVolumeRenderChanged
                : SaaVolumeCaptureChanged);
        }
    }
    if (defaultCaptureChangedCallback_ != nullptr)
    {
        if (event == SoundDeviceEventType::DefaultCaptureChanged)
        {
            defaultCaptureChangedCallback_(devicePnpId.empty() ? SaaDefaultCaptureDetached : SaaDefaultCaptureAttached);
        }
        if (
            (event == SoundDeviceEventType::VolumeCaptureChanged || event == SoundDeviceEventType::VolumeRenderChanged)
            && deviceCollection_ != nullptr && deviceCollection_->GetDefaultCaptureDevicePnpId() == devicePnpId
        )
        {
            defaultCaptureChangedCallback_(event == SoundDeviceEventType::VolumeCaptureChanged
                ? SaaVolumeCaptureChanged
                : SaaVolumeRenderChanged);
        }
    }
}


namespace  {
    TSaaGotLogMessageCallback got_log_message_callback = nullptr;

    void LoggerMessageBridge(const std::string& timestamp, const std::string& level, const std::string& message)
    {
        if (got_log_message_callback == nullptr) return;
        SaaLogMessage out{}; // zero-init
        strncpy_s(out.Timestamp, _countof(out.Timestamp), timestamp.c_str(), _TRUNCATE);
        strncpy_s(out.Level, _countof(out.Level), level.c_str(), _TRUNCATE);
        strncpy_s(out.Content, _countof(out.Content), message.c_str(), _TRUNCATE);
        got_log_message_callback(out);
    }

    void SetUpLog
        (
        TSaaGotLogMessageCallback gotLogMessageCallback,
        const CHAR* appName,
        const CHAR* appVersion
        )
    {
        const auto appNameString = appName != nullptr ? std::string(appName) : std::string(RESOURCE_FILENAME_ATTRIBUTE);
        const auto appVersionString = appVersion != nullptr ? std::string(appVersion) : std::string(PRODUCT_VERSION_ATTRIBUTE);

        got_log_message_callback = gotLogMessageCallback;

        ed::model::Logger::Inst()
            .ConfigureAppNameAndVersion(appNameString, appVersionString)
            .SetOutputToConsole(false);
        if (gotLogMessageCallback != nullptr)
        {
            ed::model::Logger::Inst()
                .SetMessageCallback(
                    LoggerMessageBridge
                );
        }

    }

}

SaaResult SaaInitialize(SaaHandle* handle,
    TSaaGotLogMessageCallback gotLogMessageCallback,
    const CHAR* appName,
    const CHAR* appVersion
)
{
    SetUpLog(gotLogMessageCallback, appName, appVersion);

    auto context = std::make_unique<HandleContext>();
    context->DeviceCollection = SoundAgent::CreateDeviceCollection();
    *handle = reinterpret_cast<SaaHandle>(context.release());

    return 0;
}

SaaResult SaaRegisterCallbacks(SaaHandle handle
    , TSaaDefaultChangedCallback defaultRenderChangedCallback
    , TSaaDefaultChangedCallback defaultCaptureChangedCallback
)
{
    const auto context = GetHandleContextOrNull(handle);
    if (context == nullptr || context->DeviceCollection == nullptr)
    {
        return 0;
    }

    if (context->DeviceCollectionObserver != nullptr)
    {
        context->DeviceCollection->Unsubscribe(*context->DeviceCollectionObserver);
    }

    context->DeviceCollectionObserver = std::make_unique<DllObserver>(context->DeviceCollection.get(),
        defaultRenderChangedCallback,
        defaultCaptureChangedCallback);
    context->DeviceCollection->Subscribe(*context->DeviceCollectionObserver);
    context->DeviceCollection->ResetContent();

    return 0;
}

namespace
{
    SaaResult GetDeviceOnPnpId(const SoundDeviceCollectionInterface* deviceCollection,
        SaaDescription* description,
        const std::optional<std::string>& pnpId);
}


SaaResult SaaGetDefaultRender(SaaHandle handle, SaaDescription* description)
{
    if (description == nullptr)
    {
        return 0;
    }
    const auto context = GetHandleContextOrNull(handle);
    if (context == nullptr || context->DeviceCollection == nullptr)
    {
        return 0;
    }
    const auto pnpId = context->DeviceCollection->GetDefaultRenderDevicePnpId();

    return GetDeviceOnPnpId(context->DeviceCollection.get(), description, pnpId);
}

SaaResult SaaGetDefaultCapture(SaaHandle handle, SaaDescription* description)
{
    if (description == nullptr)
    {
        return 0;
    }
    const auto context = GetHandleContextOrNull(handle);
    if (context == nullptr || context->DeviceCollection == nullptr)
    {
        return 0;
    }
    const auto pnpId = context->DeviceCollection->GetDefaultCaptureDevicePnpId();

    return GetDeviceOnPnpId(context->DeviceCollection.get(), description, pnpId);
}

SaaResult SaaGetOperationSystemName(SaaHandle handle, SaaOsInfo* osInfo)
{
    if (osInfo == nullptr)
    {
        return 0;
    }

    if (GetHandleContextOrNull(handle) == nullptr)
    {
        return 0;
    }

    std::ranges::fill(osInfo->Name, '\0');

    const auto operationSystemName = ed::audio::GetOperationSystemName();
    strncpy_s(osInfo->Name, _countof(osInfo->Name), operationSystemName.c_str(), _TRUNCATE);

    return 0;
}

namespace
{
    SaaResult GetDeviceOnPnpId(const SoundDeviceCollectionInterface* deviceCollection,
        SaaDescription* description,
        const std::optional<std::string>& pnpId)
    {
        std::ranges::fill(description->PnpId, '\0');
        std::ranges::fill(description->Name, '\0');
        description->IsRender = FALSE;
        description->IsCapture = FALSE;
        description->RenderVolume = 0;
        description->CaptureVolume = 0;
        if (deviceCollection != nullptr && pnpId.has_value())
        {
            if (const auto device = deviceCollection->CreateItem(*pnpId)
                ; device != nullptr)
            {
                {

                    const auto devicePnpId = device->GetPnpId();
                    const auto deviceName = device->GetName();
                    strncpy_s(description->PnpId, _countof(description->PnpId), devicePnpId.c_str(),_TRUNCATE);
                    strncpy_s(description->Name, _countof(description->Name), deviceName.c_str(), _TRUNCATE);
                }
                description->IsRender = device->GetFlow() == SoundDeviceFlowType::Render || device->GetFlow() ==
                                        SoundDeviceFlowType::RenderAndCapture
                                            ? TRUE
                                            : FALSE;
                description->IsCapture = device->GetFlow() == SoundDeviceFlowType::Capture || device->GetFlow() ==
                                         SoundDeviceFlowType::RenderAndCapture
                                             ? TRUE
                                             : FALSE;
                description->RenderVolume = device->GetCurrentRenderVolume();
                description->CaptureVolume = device->GetCurrentCaptureVolume();
            }
            return 0;
        }
        return 0;
    }
}


SaaResult SaaUnInitialize(SaaHandle handle)
{
    if (const auto context = GetHandleContextOrNull(handle); context != nullptr)
    {
        if (context->DeviceCollection != nullptr && context->DeviceCollectionObserver != nullptr)
        {
            context->DeviceCollection->Unsubscribe(*context->DeviceCollectionObserver);
            context->DeviceCollectionObserver.reset();
        }
        context->DeviceCollection.reset();
        delete context;
    }
    return 0;
}
