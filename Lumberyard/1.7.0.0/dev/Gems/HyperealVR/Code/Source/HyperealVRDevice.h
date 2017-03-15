
#pragma once

#include <AzCore/Component/Component.h>
#include <HyperealVR/HyperealVRBus.h>
#include "HMDBus.h"
#include "HyperealVRController.h"
#include <HMDBus.h>
#include <VRControllerBus.h>
#include <CrySystemBus.h>

#include <Hypereal_VR.h>

namespace HyperealVR
{
	class HyperealVRController;

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
		/// HyperealVRRequests overrides ///////////////////////////////////////////////
 		void GetPlayspace(Playspace& playspace) const override;
		bool IsConnected(uint32_t id) const;
// 		////////////////////////////////////////////////////////////////////////////
// 
// 		vr::IVRSystem* m_system = nullptr;
// 		vr::IVRCompositor* m_compositor = nullptr;
// 		vr::TrackedDevicePose_t m_trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
// 		vr::ETrackingUniverseOrigin m_trackingOrigin;
// 
 		HyperealVRController* m_controller;
		AZ::VR::TrackingState m_trackingState; ///< Most recent tracking state of the HMD (not including any connected controllers).
		AZ::VR::HMDDeviceInfo m_deviceInfo;

		// Tracking related:
		enum EDevice
		{
			Hmd,
			Left_Controller,
			Right_Controller,
			Total_Count
		};
		HyTrackingState			m_rTrackedDevicePose[EDevice::Total_Count];
		AZ::VR::TrackingState        m_nativeStates[EDevice::Total_Count];
		AZ::VR::TrackingState        m_localStates[EDevice::Total_Count];
		AZ::VR::TrackingState        m_nativeEyePoseStates;
		AZ::VR::TrackingState        m_localEyePoseStates;
		
		HyPose	m_CurDevicePose[EDevice::Total_Count];
		bool	m_IsDevicePositionTracked[EDevice::Total_Count];
		bool	m_IsDeviceRotationTracked[EDevice::Total_Count];

// 		AZ::VR::TrackingState        m_disabledTrackingState;

		//////////////////////////////////////////////////////////////////////////
		//HVR device member
		struct DeviceInfo
		{
			int64 DeviceResolutionX;
			int64 DeviceResolutionY;
			HyFov Fov[HY_EYE_MAX];
		};
		HyDevice *m_pVrDevice;
		DeviceInfo m_VrDeviceInfo;
		HyGraphicsContext *m_pVrGraphicsCxt;
		HyGraphicsContextDesc m_VrGraphicsCxtDesc;

		int                     m_lastFrameID_UpdateTrackingState;

		HyFov m_eyeFovSym;
		float m_fPixelDensity;
		bool m_bVRInitialized;
		bool m_bVRSystemValid;
		bool m_bIsQuitting;
		HyTextureDesc m_RTDesc[2];
		float m_fInterpupillaryDistance;

		HyVec2 *m_pPlayAreaVertices;
		int64 m_nPlayAreaVertexCount;
		bool m_bPlayAreaValid;

		AZ::Quaternion					m_qBaseOrientation;
		AZ::Vector3						m_vBaseOffset;
		float					m_fMeterToWorldScale;
		bool					m_bPosTrackingEnable;
		bool					m_bResetOrientationKeepPitchAndRoll;

		// Controller related:
//		OpenVRController              m_controller;
// 		bool                    m_hasInputFocus;
// 		bool                    m_hmdTrackingDisabled;
// 		float                   m_hmdQuadDistance;
// 		float                   m_hmdQuadWidth;
// 		int                     m_hmdQuadAbsolute;

		void RebuildPlayArea();
		const char* GetTrackedDeviceCharPointer(int nProperty);
		inline void CopyPoseState(AZ::VR::PoseState& world, AZ::VR::PoseState& hmd, HyTrackingState& source);
		inline void CopyPose(const HyTrackingState& src, AZ::VR::PoseState& poseDest, AZ::VR::DynamicsState& dynamicDest);

		float GetInterpupillaryDistance() const;

		void CreateGraphicsContext(void* graphicsDevice);
		void ReleaseGraphicsContext();
		void UpdateInternal();

		void ResetOrientationAndPosition(float Yaw);
		void ResetOrientation(float yaw);
		void ResetPosition();
    };
}
