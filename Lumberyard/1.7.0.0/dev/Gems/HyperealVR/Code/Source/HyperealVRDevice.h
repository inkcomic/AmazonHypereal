
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Component.h>
#include "HMDBus.h"
#include <HyperealVR/HyperealVRBus.h>
#include <Hypereal_VR.h>

namespace HyperealVR
{
    class HyperealVRDevice
        : public AZ::Component
		, public AZ::VR::HMDDeviceRequestBus::Handler
		, public AZ::VR::HMDInitRequestBus::Handler
        , protected HyperealVRRequestBus::Handler
    {
    public:
        AZ_COMPONENT(HyperealVRDevice, "{D610CD75-733D-4D09-8C21-FC9AB33B9554}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // HyperealVRRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////
		HyperealVRDevice() = default;
		~HyperealVRDevice() override = default;
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

		/// HMDInitBus overrides /////////////////////////////////////////////////////
		bool AttemptInit() override;
		void Shutdown() override;
		AZ::VR::HMDInitBus::HMDInitPriority GetInitPriority() const override;
		//////////////////////////////////////////////////////////////////////////////


		/// IHMDDevice overrides ///////////////////////////////////////////////////
		void GetPerEyeCameraInfo(const EStereoEye eye, const float nearPlane, const float farPlane, AZ::VR::PerEyeCameraInfo& cameraInfo) override;
		bool CreateRenderTargets(void* renderDevice, const TextureDesc& desc, size_t eyeCount, AZ::VR::HMDRenderTarget* renderTargets[STEREO_EYE_COUNT]) override;
		void DestroyRenderTarget(AZ::VR::HMDRenderTarget& renderTarget) override;
		void Update() override;
		AZ::VR::TrackingState* GetTrackingState() override;
		void SubmitFrame(const EyeTarget& left, const EyeTarget& right) override;
		void RecenterPose() override;
		void SetTrackingLevel(const AZ::VR::HMDTrackingLevel level) override;
		void OutputHMDInfo() override;
		void EnableDebugging(bool enable) override;
		void DrawDebugInfo(const AZ::Transform& transform, IRenderAuxGeom* auxGeom) override;
		AZ::VR::HMDDeviceInfo* GetDeviceInfo() override;
		bool IsInitialized() override;
		////////////////////////////////////////////////////////////////////////////

	private:
		/// OpenVRRequests overrides ///////////////////////////////////////////////
// 		void GetPlayspace(Playspace& playspace) const override;
// 		////////////////////////////////////////////////////////////////////////////
// 
// 		vr::IVRSystem* m_system = nullptr;
// 		vr::IVRCompositor* m_compositor = nullptr;
// 		vr::TrackedDevicePose_t m_trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
// 		vr::ETrackingUniverseOrigin m_trackingOrigin;
// 
// 		OpenVRController* m_controller;
		AZ::VR::TrackingState m_trackingState; ///< Most recent tracking state of the HMD (not including any connected controllers).
		AZ::VR::HMDDeviceInfo m_deviceInfo;
    };
}
