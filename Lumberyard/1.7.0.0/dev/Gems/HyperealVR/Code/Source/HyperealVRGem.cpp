
#include "StdAfx.h"
#include <platform_impl.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "HyperealVRDevice.h"

#include <IGem.h>

namespace HyperealVR
{
    class HyperealVRModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(HyperealVRModule, "{622A7E1C-7289-49A2-9F7D-31AFFE83074C}", CryHooksModule);

        HyperealVRModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                HyperealVRDevice::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<HyperealVRDevice>(),
            };
        }

		void OnSystemEvent(ESystemEvent event, UINT_PTR, UINT_PTR) override
		{
			switch (event)
			{
			case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
				RegisterExternalFlowNodes();
				break;
			}
		}
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(HyperealVR_378c5c15ab614569b6adf976c41f36e5, HyperealVR::HyperealVRModule)
