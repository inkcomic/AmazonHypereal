
#pragma once

#include <AzCore/Component/Component.h>

#include <HyperealVR/HyperealVRBus.h>

namespace HyperealVR
{
    class HyperealVRSystemComponent
        : public AZ::Component
        , protected HyperealVRRequestBus::Handler
    {
    public:
        AZ_COMPONENT(HyperealVRSystemComponent, "{D610CD75-733D-4D09-8C21-FC9AB33B9554}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // HyperealVRRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
