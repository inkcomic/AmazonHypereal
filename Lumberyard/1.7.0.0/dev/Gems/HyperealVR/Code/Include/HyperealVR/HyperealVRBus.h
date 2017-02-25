
#pragma once

#include <AzCore/EBus/EBus.h>

namespace HyperealVR
{
    class HyperealVRRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using HyperealVRRequestBus = AZ::EBus<HyperealVRRequests>;
} // namespace HyperealVR
