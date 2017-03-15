
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


		///
		/// Rectangle storing the playspace defined by the user when
		/// setting up HyperealVR.
		///
		struct Playspace
		{
			bool isValid = false; ///< The playspace data is valid (calibrated).
			AZ::Vector3 corners[4];      ///< Playspace corners defined in device-local space. The center of the playspace is 0.
		};

		virtual void GetPlayspace(Playspace& playspace) const = 0;
    };
    using HyperealVRRequestBus = AZ::EBus<HyperealVRRequests>;
} // namespace HyperealVR
