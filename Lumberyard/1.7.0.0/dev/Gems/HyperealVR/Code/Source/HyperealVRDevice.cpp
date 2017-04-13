
#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MathUtils.h>
#include <MathConversion.h>
#include "HyperealVRDevice.h"
#include <d3d11.h>



#define HY_RELEASE(p) {if(p != nullptr) p->Release(); p = nullptr;}
#define DEFAULT_IPD 0.064f;

namespace HyperealVR
{
	// -------------------------------------------------------------------------

	inline AZ::Quaternion QuatToAZQuat(const Quat& quat)
	{
		return AZ::Quaternion(quat.v.x, quat.v.y, quat.v.z, quat.w);
	}
	inline  Quat& AZQuatToQuat(const AZ::Quaternion quat)
	{
		return Quat(quat.GetW(),quat.GetX(), quat.GetY(), quat.GetZ());
	}
	inline AZ::Vector3 Vec3ToAZVec3(const Vec3& vec)
	{
		return AZ::Vector3(vec.x,vec.y,vec.z);
	}
	inline Quat HmdQuatToWorldQuat(const Quat& quat)
	{
		Matrix33 m33(quat);
		Vec3 column1 = -quat.GetColumn2();
		m33.SetColumn2(m33.GetColumn1());
		m33.SetColumn1(column1);
		return Quat::CreateRotationX(gf_PI * 0.5f) * Quat(m33);
	}

	inline Quat HYQuatToQuat(const HyQuat& q) {
		return HmdQuatToWorldQuat(Quat(q.w, q.x, q.y, q.z));
	}

	inline Vec3 HYVec3ToVec3(const HyVec3& v) {
		return Vec3(v.x, -v.z, v.y);
	}
	inline HyQuat QuatToHYQuat(const Quat &q) {
		Quat invQuat = HmdQuatToWorldQuat(q).GetInverted();
		HyQuat hyQuat;
		hyQuat.w = invQuat.w;
		hyQuat.x = invQuat.v.x;
		hyQuat.y = invQuat.v.y;
		hyQuat.z = invQuat.v.z;
		return hyQuat;
	}
	inline HyVec3 Vec3ToHYVec3(const Vec3& v) {
		HyVec3 hyVec3;
		hyVec3.x = v.x;
		hyVec3.y = v.z;
		hyVec3.z = -v.y;
		return hyVec3;
	}
	// -------------------------------------------------------------------------
	void HyperealVRDevice::CopyPoseState(AZ::VR::PoseState& world, AZ::VR::PoseState& hmd, HyTrackingState& src)
	{
		AZ::Quaternion srcQuat = QuatToAZQuat(HYQuatToQuat(src.m_pose.m_rotation));
		AZ::Vector3 srcPos = Vec3ToAZVec3(HYVec3ToVec3(src.m_pose.m_position));
		AZ::Vector3 srcAngVel = Vec3ToAZVec3(HYVec3ToVec3(src.m_angularVelocity));
		AZ::Vector3 srcAngAcc = Vec3ToAZVec3(HYVec3ToVec3(src.m_angularAcceleration));
		AZ::Vector3 srcLinVel = Vec3ToAZVec3(HYVec3ToVec3(src.m_linearVelocity));
		AZ::Vector3 srcLinAcc = Vec3ToAZVec3(HYVec3ToVec3(src.m_linearAcceleration));

		hmd.orientation = srcQuat;
		hmd.position = srcPos;

		//hmd.linearVelocity = srcLinVel;
		//hmd.angularVelocity = srcAngVel;

		world.position = /*HmdVec3ToWorldVec3*/(hmd.position);
		world.orientation = /*HmdQuatToWorldQuat*/(hmd.orientation);
	//	world.linearVelocity = /*HmdVec3ToWorldVec3*/(hmd.linearVelocity);
	//	world.angularVelocity = /*HmdVec3ToWorldVec3*/(hmd.angularVelocity);
	}
	void HyperealVRDevice::CopyPose(const HyTrackingState& src, AZ::VR::PoseState& poseDest, AZ::VR::DynamicsState& dynamicDest)
	{
		AZ::Quaternion srcQuat = QuatToAZQuat(HYQuatToQuat(src.m_pose.m_rotation));
		AZ::Vector3 srcPos = Vec3ToAZVec3(HYVec3ToVec3(src.m_pose.m_position));
		AZ::Vector3 srcAngVel = Vec3ToAZVec3(HYVec3ToVec3(src.m_angularVelocity));
		AZ::Vector3 srcAngAcc = Vec3ToAZVec3(HYVec3ToVec3(src.m_angularAcceleration));
		AZ::Vector3 srcLinVel = Vec3ToAZVec3(HYVec3ToVec3(src.m_linearVelocity));
		AZ::Vector3 srcLinAcc = Vec3ToAZVec3(HYVec3ToVec3(src.m_linearAcceleration));

		poseDest.orientation = srcQuat;
		poseDest.position = srcPos;

		dynamicDest.linearVelocity = srcLinVel;
		dynamicDest.angularVelocity = srcAngVel;
		dynamicDest.angularAcceleration = AZ::Vector3(0.0f, 0.0f, 0.0f); // does not support accelerations.
		dynamicDest.linearAcceleration = AZ::Vector3(0.0f, 0.0f, 0.0f); // does not support accelerations.
	}

	HyFov ComputeSymmetricalFov(const HyFov& fovLeftEye, const HyFov& fovRightEye)
	{
		const float stereoDeviceAR = 1.7777f;

		HyFov fovMax;
		fovMax.m_upTan = max(fovLeftEye.m_upTan, fovRightEye.m_upTan);
		fovMax.m_downTan = max(fovLeftEye.m_downTan, fovRightEye.m_downTan);
		fovMax.m_leftTan = max(fovLeftEye.m_leftTan, fovRightEye.m_leftTan);
		fovMax.m_rightTan = max(fovLeftEye.m_rightTan, fovRightEye.m_rightTan);

		const float combinedTanHalfFovHorizontal = max(fovMax.m_leftTan, fovMax.m_rightTan);
		const float combinedTanHalfFovVertical = max(fovMax.m_upTan, fovMax.m_downTan);

		HyFov fovSym;
		fovSym.m_upTan = fovSym.m_downTan = combinedTanHalfFovVertical * 1.f;

		fovSym.m_leftTan = fovSym.m_rightTan = fovSym.m_upTan / (2.f / stereoDeviceAR);

		CryLog("[Hypereal] Fov: Up/Down tans [%f] Left/Right tans [%f]", fovSym.m_upTan, fovSym.m_leftTan);
		return fovSym;
	}


    void HyperealVRDevice::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<HyperealVRDevice, AZ::Component>()
                ->Version(0)
                ->SerializerForEmptyClass();

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<HyperealVRDevice>("HyperealVR", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                         ->Attribute(AZ::Edit::Attributes::Category, "VR")// Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void HyperealVRDevice::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
	//	provided.push_back(AZ_CRC("HMDDevice"));
        provided.push_back(AZ_CRC("HyperealVRService"));
    }

    void HyperealVRDevice::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("HyperealVRService"));
    }

    void HyperealVRDevice::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void HyperealVRDevice::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }
	// -------------------------------------------------------------------------
	bool HyperealVRDevice::IsConnected(uint32_t id) const
	{
		if (!m_pVrDevice)
			return false;
		bool bControllerConnected = false;
		if (HY_SUBDEV_CONTROLLER_LEFT == id)
			m_pVrDevice->GetBoolValue(HY_PROPERTY_DEVICE_CONNECTED_BOOL, bControllerConnected, HyGetSubDeviceType(HY_SUBDEV_CONTROLLER_LEFT));
		else if (HY_SUBDEV_CONTROLLER_RIGHT == id)
			m_pVrDevice->GetBoolValue(HY_PROPERTY_DEVICE_CONNECTED_BOOL, bControllerConnected, HyGetSubDeviceType(HY_SUBDEV_CONTROLLER_RIGHT));
		return bControllerConnected;
	}
    void HyperealVRDevice::Init()
    {
//		m_lastFrameID_UpdateTrackingState = -1;

// 		m_hasInputFocus = (false);
// 		m_hmdTrackingDisabled = (false);
// 		m_hmdQuadDistance = (CPlugin_Hypereal::s_hmd_quad_distance);
// 		m_hmdQuadWidth = (CPlugin_Hypereal::s_hmd_quad_width);
// 		m_hmdQuadAbsolute = (CPlugin_Hypereal::s_hmd_quad_absolute);
		m_pVrDevice = nullptr;
		m_pVrGraphicsCxt = nullptr;
		m_pPlayAreaVertices = nullptr;
		m_bPlayAreaValid = false;
		m_fPixelDensity = 1.0f;
		m_bVRInitialized = nullptr;
		m_bVRSystemValid = nullptr;
		m_bIsQuitting = false;
		m_fInterpupillaryDistance = -1.0f;
		m_qBaseOrientation = AZ::Quaternion(IDENTITY);;
		m_vBaseOffset = AZ::Vector3(IDENTITY);
		m_fMeterToWorldScale = 1.f;
		m_bPosTrackingEnable = true;
		m_bResetOrientationKeepPitchAndRoll = false;

// 		memset(m_trackedDevicePose, 0, sizeof(m_trackedDevicePose));
 		m_controller = new HyperealVRController(m_pVrDevice); // Note that this will be deleted by the input system and should not be deleted here.
    }

    void HyperealVRDevice::Activate()
    {
       // HyperealVRRequestBus::Handler::BusConnect();
	   // Connect to the EBus so that we can receive messages.
		AZ::VR::HMDInitRequestBus::Handler::BusConnect();
    }

    void HyperealVRDevice::Deactivate()
    {
        //HyperealVRRequestBus::Handler::BusDisconnect();
		AZ::VR::HMDInitRequestBus::Handler::BusDisconnect();
    }


	bool HyperealVRDevice::AttemptInit()
	{
		
 		//LogMessage("Attempting to initialize HyperealVR SDK");

		bool success = false;


		{
			for (int i = 0; i < 2; ++i)
			{
				for (int j =0;j<2;j++)
				{
					m_frameParams[i].m_RTDesc[j].m_uvSize = HyVec2{ 1.f, 1.f };
					m_frameParams[i].m_RTDesc[j].m_uvOffset = HyVec2{ 0.f, 0.f };
				}
			}

			HyResult startResult = HyStartup();
			m_bVRInitialized = hySucceeded(startResult);
			if (!m_bVRInitialized)
			{
				gEnv->pLog->Log("[HMD][Hypereal] HyperealVR Failed to Startup.");
				return false;
			}

			gEnv->pLog->Log("[HMD][Hypereal] HyperealVR Startup sucessfully.");

			m_bPlayAreaValid = false;
			m_nPlayAreaVertexCount = 0;


			HyResult hr = HyCreateInterface(sch_HyDevice_Version, 0, (void**)&m_pVrDevice);

			if (!hySucceeded(hr))
			{
				gEnv->pLog->Log("[HMD][Hypereal] HyCreateInterface failed.");
				return false;
			}



			memset(&m_VrDeviceInfo, 0, sizeof(DeviceInfo));

			m_pVrDevice->GetIntValue(HY_PROPERTY_HMD_RESOLUTION_X_INT, m_VrDeviceInfo.DeviceResolutionX);
			m_pVrDevice->GetIntValue(HY_PROPERTY_HMD_RESOLUTION_Y_INT, m_VrDeviceInfo.DeviceResolutionY);
			m_pVrDevice->GetFloatArray(HY_PROPERTY_HMD_LEFT_EYE_FOV_FLOAT4_ARRAY, m_VrDeviceInfo.Fov[HY_EYE_LEFT].val, 4);
			m_pVrDevice->GetFloatArray(HY_PROPERTY_HMD_RIGHT_EYE_FOV_FLOAT4_ARRAY, m_VrDeviceInfo.Fov[HY_EYE_RIGHT].val, 4);



			m_eyeFovSym = ComputeSymmetricalFov(m_VrDeviceInfo.Fov[HY_EYE_LEFT], m_VrDeviceInfo.Fov[HY_EYE_RIGHT]);
			//set for  generic HMD device
			m_deviceInfo.renderWidth = (uint)m_VrDeviceInfo.DeviceResolutionX;
			m_deviceInfo.renderHeight = (uint)m_VrDeviceInfo.DeviceResolutionY;

			m_deviceInfo.manufacturer = GetTrackedDeviceCharPointer(HY_PROPERTY_MANUFACTURER_STRING);
			m_deviceInfo.productName = GetTrackedDeviceCharPointer(HY_PROPERTY_PRODUCT_NAME_STRING);
			m_deviceInfo.fovH = 2.0f * atanf(m_eyeFovSym.m_leftTan);
			m_deviceInfo.fovV = 2.0f * atanf(m_eyeFovSym.m_upTan);


			bool isConnected = false;
			hr = m_pVrDevice->GetBoolValue(HY_PROPERTY_DEVICE_CONNECTED_BOOL, isConnected, HySubDevice::HY_SUBDEV_HMD);
			if (hySucceeded(hr) && isConnected)
			{
				//m_pVrDevice->ConfigureTrackingOrigin(m_pTrackingOriginCVar->GetIVal() == (int)EHmdTrackingOrigin::Floor == 1 ? HY_TRACKING_ORIGIN_FLOOR : HY_TRACKING_ORIGIN_EYE);

				RebuildPlayArea();
				gEnv->pLog->Log("[HMD][Hypereal] EnableStereo successfully.");


				// Check for any controllers that may be connected.
				{
				 	m_controller->Enable(true);
// 				 	for (int ii = 0; ii < 2; ++ii)
// 				 	{
// 						if (IsConnected(ii))
// 						{
// 							m_controller->ConnectController(ii);
// 						}
// 				 	}
				}

				SetTrackingLevel(AZ::VR::HMDTrackingLevel::kFloor); 
				// Default to a seated experience.
				 
				// Connect to the HMDDeviceBus in order to get HMD messages from the rest of the VR system.
				AZ::VR::HMDDeviceRequestBus::Handler::BusConnect();
				m_controller->ConnectToControllerBus();
				HyperealVRRequestBus::Handler::BusConnect();
				 
				success = true;
			}
			else
			{
				gEnv->pLog->Log("[HMD][Hypereal] HyperealVR HMD is Disconnected.");
			}

#if CRY_PLATFORM_WINDOWS
			// the following is (hopefully just) a (temporary) hack to shift focus back to the CryEngine window, after (potentially) spawning the SteamVR Compositor
			if (!gEnv->IsEditor())
			{
				LockSetForegroundWindow(LSFW_UNLOCK);
				SetForegroundWindow((HWND)gEnv->pSystem->GetHWND());
			}
#endif

		}
// 		vr::EVRInitError error = vr::EVRInitError::VRInitError_None;
// 		m_system = vr::VR_Init(&error, vr::EVRApplicationType::VRApplication_Scene);
// 
// 		if (error == vr::EVRInitError::VRInitError_None)
// 		{
// 			if (!vr::VR_IsHmdPresent())
// 			{
// 				LogMessage("No HMD connected");
// 			}
// 			else
// 			{
// 				m_compositor = static_cast<vr::IVRCompositor*>(vr::VR_GetGenericInterface(vr::IVRCompositor_Version, &error));
// 				if (error == vr::EVRInitError::VRInitError_None)
// 				{
// 					m_system->GetRecommendedRenderTargetSize(&m_deviceInfo.renderWidth, &m_deviceInfo.renderHeight);
// 
// 					m_deviceInfo.manufacturer = GetTrackedDeviceCharPointer(m_system, vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_ManufacturerName_String, nullptr);
// 					m_deviceInfo.productName = GetTrackedDeviceCharPointer(m_system, vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_ModelNumber_String, nullptr);
// 
// 					// Determine the device's symmetric field of view.
// 					{
// 						vr::HmdMatrix44_t projectionMatrix = m_system->GetProjectionMatrix(vr::EVREye::Eye_Left, 0.01, 1000.0f, vr::EGraphicsAPIConvention::API_DirectX);
// 
// 						float denom = 1.0f / projectionMatrix.m[1][1];
// 						float fovv = 2.0f * atanf(denom);
// 						float aspectRatio = projectionMatrix.m[1][1] / projectionMatrix.m[0][0];
// 						float fovh = 2.0f * atanf(denom * aspectRatio);
// 
// 						m_deviceInfo.fovH = fovh;
// 						m_deviceInfo.fovV = fovv;
// 					}
// 
// 					// Check for any controllers that may be connected.
// 					{
// 						m_controller->Enable(true);
// 						for (int ii = 0; ii < vr::k_unMaxTrackedDeviceCount; ++ii)
// 						{
// 							if (m_system->GetTrackedDeviceClass(ii) == vr::TrackedDeviceClass_Controller)
// 							{
// 								if (m_system->IsTrackedDeviceConnected(ii))
// 								{
// 									m_controller->ConnectController(ii);
// 								}
// 							}
// 						}
// 					}
// 
// 					SetTrackingLevel(AZ::VR::HMDTrackingLevel::kFloor); // Default to a seated experience.
// 
// 																		// Connect to the HMDDeviceBus in order to get HMD messages from the rest of the VR system.
// 					AZ::VR::HMDDeviceRequestBus::Handler::BusConnect();
// 					m_controller->ConnectToControllerBus();
// 					HyperealVRRequestBus::Handler::BusConnect();
// 
// 					success = true;
// 				}
// 			}
// 		}
 
 		if (!success)
 		{

 		//	LogMessage("Unable to initialize VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(error));

 			Shutdown();
 		}
 
 		return success;
	}

	AZ::VR::HMDInitBus::HMDInitPriority HyperealVRDevice::GetInitPriority() const
	{
		return HMDInitPriority::kMiddle;
	}

	void HyperealVRDevice::Shutdown()
	{
 		m_controller->DisconnectFromControllerBus();
// 
 		AZ::VR::HMDDeviceRequestBus::Handler::BusDisconnect();
		HyperealVRRequestBus::Handler::BusDisconnect();


		HY_RELEASE(m_pVrDevice);

		m_bVRSystemValid = false;

		if (m_bVRInitialized)
		{
			m_bVRInitialized = false;
			HyShutdown();

			gEnv->pSystem->Quit();
		}
	}

	// -------------------------------------------------------------------------
	float HyperealVRDevice::GetInterpupillaryDistance() const
	{
		if (m_fInterpupillaryDistance > 0.01f)
			return m_fInterpupillaryDistance;
		if (m_bVRSystemValid)
		{
			bool isConnected = false;
			HyResult hr = m_pVrDevice->GetBoolValue(HY_PROPERTY_DEVICE_CONNECTED_BOOL, isConnected, HY_SUBDEV_HMD);
			if (hySucceeded(hr) && isConnected)
			{
				float ipd = DEFAULT_IPD;
				hr = m_pVrDevice->GetFloatValue(HY_PROPERTY_HMD_IPD_FLOAT, ipd);
				if (hySucceeded(hr))
					return ipd;
			}
		}
		return DEFAULT_IPD;
	}

	void HyperealVRDevice::GetPerEyeCameraInfo(const EStereoEye eye, const float nearPlane, const float farPlane, AZ::VR::PerEyeCameraInfo& cameraInfo)
	{
		if (m_pVrGraphicsCxt == nullptr)
			return;

		float fNear = nearPlane;
		float fFar = farPlane;
		HyMat4 proj;
		m_pVrGraphicsCxt->GetProjectionMatrix(m_VrDeviceInfo.Fov[eye], fNear, fFar, true, proj);

		{
			cameraInfo.fov = (eye== EStereoEye::STEREO_EYE_LEFT)?(atanf(m_VrDeviceInfo.Fov[EStereoEye::STEREO_EYE_LEFT].m_downTan) + atanf(m_VrDeviceInfo.Fov[EStereoEye::STEREO_EYE_LEFT].m_upTan)):
				(atanf(m_VrDeviceInfo.Fov[EStereoEye::STEREO_EYE_LEFT].m_downTan) + atanf(m_VrDeviceInfo.Fov[EStereoEye::STEREO_EYE_LEFT].m_upTan));
		}

		cameraInfo.aspectRatio = proj.m[1][1] / proj.m[0][0];
		const float denom = 1.0f / proj.m[1][1];
		cameraInfo.frustumPlane.horizontalDistance = proj.m[0][2] * denom * cameraInfo.aspectRatio;
		cameraInfo.frustumPlane.verticalDistance = proj.m[1][2] * denom;

		// Read the inter-pupilar distance in order to determine the eye offset.
		{
			float eyeDistance = GetInterpupillaryDistance();
		 	float xPosition = 0.0f;
		 	if (eye == STEREO_EYE_LEFT)
		 	{
		 		xPosition = -eyeDistance * 0.5f;
		 	}
		 	else
		 	{
		 		xPosition = eyeDistance * 0.5f;
		 	}
		 
		 	cameraInfo.eyeOffset = AZ::Vector3(xPosition, 0.0f, 0.0f);
		}

// 		float left, right, top, bottom;
// 		m_system->GetProjectionRaw(MapHyperealVREyeToLY(eye), &left, &right, &top, &bottom);
// 
// 		cameraInfo.fov = 2.0f * atanf((bottom - top) * 0.5f);
// 		cameraInfo.aspectRatio = (right - left) / (bottom - top);
// 
// 		cameraInfo.frustumPlane.horizontalDistance = (right + left) * 0.5f;
// 		cameraInfo.frustumPlane.verticalDistance = (bottom + top) * 0.5f;
// 
// 		// Read the inter-pupilar distance in order to determine the eye offset.
// 		{
// 			float eyeDistance = m_system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_UserIpdMeters_Float, nullptr);
// 			float xPosition = 0.0f;
// 			if (eye == STEREO_EYE_LEFT)
// 			{
// 				xPosition = -eyeDistance * 0.5f;
// 			}
// 			else
// 			{
// 				xPosition = eyeDistance * 0.5f;
// 			}
// 
// 			cameraInfo.eyeOffset = AZ::Vector3(xPosition, 0.0f, 0.0f);
// 		}
	}
	void HyperealVRDevice::CreateGraphicsContext(void* graphicsDevice)
	{
		if (nullptr == m_pVrDevice)
			return;

		//graphic ctx should be ready
		HyGraphicsAPI graphicsAPI = HY_GRAPHICS_UNKNOWN;

		graphicsAPI = HY_GRAPHICS_D3D11;

		{
			m_VrGraphicsCxtDesc.m_mirrorWidth = gEnv->pRenderer->GetWidth();
			m_VrGraphicsCxtDesc.m_mirrorHeight = gEnv->pRenderer->GetHeight();
		}

		m_VrGraphicsCxtDesc.m_graphicsDevice = graphicsDevice;
		m_VrGraphicsCxtDesc.m_graphicsAPI = graphicsAPI;
		m_VrGraphicsCxtDesc.m_pixelFormat = HY_TEXTURE_R8G8B8A8_UNORM_SRGB;
		m_VrGraphicsCxtDesc.m_pixelDensity = m_fPixelDensity;
		m_VrGraphicsCxtDesc.m_flags = 0;

		HyResult hr = m_pVrDevice->CreateGraphicsContext(m_VrGraphicsCxtDesc, &m_pVrGraphicsCxt);
		if (!hySucceeded(hr))
		{
			gEnv->pLog->Log("[HMD][Hypereal] CreateGraphicsContext failed.");
			return;
		}
	}

	void HyperealVRDevice::ReleaseGraphicsContext()
	{
		HY_RELEASE(m_pVrGraphicsCxt);
	}

	bool HyperealVRDevice::CreateRenderTargets(void* renderDevice, const TextureDesc& desc, size_t eyeCount, AZ::VR::HMDRenderTarget* renderTargets[])
	{
		ID3D11Device* d3dDevice = static_cast<ID3D11Device*>(renderDevice);

		CreateGraphicsContext(renderDevice);
		
		for (int i = 0; i < 2; ++i)
		{
			for (int j = 0; j < 2; j++)
			{
				m_frameParams[i].m_RTDesc[j].m_uvSize = HyVec2{ 1.f, 1.f };
				m_frameParams[i].m_RTDesc[j].m_uvOffset = HyVec2{ 0.f, 0.f };
			}
		}

 		for (size_t i = 0; i < eyeCount; ++i)
 		{
 
 			D3D11_TEXTURE2D_DESC textureDesc;
 			textureDesc.Width = desc.width;
 			textureDesc.Height = desc.height;
 			textureDesc.MipLevels = 1;
 			textureDesc.ArraySize = 1;
 			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
 			textureDesc.SampleDesc.Count = 1;
 			textureDesc.SampleDesc.Quality = 0;
 			textureDesc.Usage = D3D11_USAGE_DEFAULT;
 			textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
 			textureDesc.CPUAccessFlags = 0;
 			textureDesc.MiscFlags = 0;
 
 			ID3D11Texture2D* texture;
 			d3dDevice->CreateTexture2D(&textureDesc, nullptr, &texture);
 
			
			for (int j = 0; j < 2; j++)
			{
				m_frameParams[j].m_RTDesc[i].m_texture = texture;
			}
		
//  			// Create a HyperealVR texture that is associated with this new D3D texture.
//  			vr::Texture_t* deviceTexture = new vr::Texture_t();
//  			deviceTexture->eColorSpace = vr::EColorSpace::ColorSpace_Auto;
//  			deviceTexture->eType = vr::EGraphicsAPIConvention::API_DirectX;
//  			deviceTexture->handle = texture;
 
 			// We only create one texture for HyperealVR (no swapchain).
 			renderTargets[i]->deviceSwapTextureSet = &m_frameParams[0].m_RTDesc[i];
 			renderTargets[i]->numTextures = 1;
 			renderTargets[i]->textures = new void*[1];
 			renderTargets[i]->textures[0] = texture;
 		}

		return true;
	}

	void HyperealVRDevice::DestroyRenderTarget(AZ::VR::HMDRenderTarget& renderTarget)
	{
		ReleaseGraphicsContext();
		renderTarget.deviceSwapTextureSet = nullptr;

 		//vr::TextureID_t* deviceTexture = static_cast<vr::TextureID_t*>(renderTarget.deviceSwapTextureSet);
 		//SAFE_DELETE(deviceTexture);
	}

	void HyperealVRDevice::Update()
	{
		UpdateInternal();
		const int frameId = gEnv->pRenderer->GetFrameID(false);

		{
			if (m_pVrDevice)
			{
				//m_pVrDevice->ConfigureTrackingOrigin(m_pTrackingOriginCVar->GetIVal() == (int)EHmdTrackingOrigin::Floor == 1 ? HY_TRACKING_ORIGIN_FLOOR : HY_TRACKING_ORIGIN_EYE);

				static const HySubDevice Devices[EDevice::Total_Count] =
				{ HY_SUBDEV_HMD, HY_SUBDEV_CONTROLLER_LEFT, HY_SUBDEV_CONTROLLER_RIGHT };

				HyTrackingState trackingState;
				for (uint32_t i = 0; i < EDevice::Total_Count; i++)
				{
					HyResult r = m_pVrDevice->GetTrackingState(Devices[i], frameId, trackingState);
					if (hySucceeded(r))
					{
						m_rTrackedDevicePose[i] = trackingState;
						m_IsDevicePositionTracked[i] = ((HY_TRACKING_POSITION_TRACKED & trackingState.m_flags) != 0);
						m_IsDeviceRotationTracked[i] = ((HY_TRACKING_ROTATION_TRACKED & trackingState.m_flags) != 0);


// 						m_localStates[i].statusFlags = m_nativeStates[i].statusFlags = ((m_IsDeviceRotationTracked[EDevice::Hmd]) ? eHmdStatus_OrientationTracked : 0) |
// 							((m_IsDevicePositionTracked[EDevice::Hmd]) ? eHmdStatus_PositionTracked : 0);


						if (m_IsDevicePositionTracked[i] || m_IsDeviceRotationTracked[i])
						{
							CopyPoseState(m_localStates[i].pose, m_nativeStates[i].pose, m_rTrackedDevicePose[i]);
							
							m_trackingState.statusFlags = (((HY_TRACKING_POSITION_TRACKED & trackingState.m_flags)) ? AZ::VR::HMDStatus_OrientationTracked | AZ::VR::HMDStatus_PositionConnected | AZ::VR::HMDStatus_CameraPoseTracked : 0) |
								(((HY_TRACKING_ROTATION_TRACKED & trackingState.m_flags)) ? AZ::VR::HMDStatus_OrientationTracked | AZ::VR::HMDStatus_HmdConnected : 0);

							CopyPose(trackingState, m_trackingState.pose, m_trackingState.dynamics);
						}

					}
					else
					{
						m_IsDevicePositionTracked[i] = false;
						m_IsDeviceRotationTracked[i] = false;
					}

					//recenter pose
					{
						float* ipdptr = (m_fInterpupillaryDistance > 0.01f ? &m_fInterpupillaryDistance : nullptr);
						HyPose hyEyeRenderPose[HY_EYE_MAX];
						m_pVrGraphicsCxt->GetEyePoses(m_rTrackedDevicePose[EDevice::Hmd].m_pose, ipdptr, hyEyeRenderPose);

						memcpy(&m_nativeEyePoseStates, &m_nativeStates[i/*EDevice::Hmd*/], sizeof(AZ::VR::TrackingState));
						memcpy(&m_localEyePoseStates, &m_localStates[i/*EDevice::Hmd*/], sizeof(AZ::VR::TrackingState));

						// compute centered transformation
						AZ::Quaternion eyeRotation = QuatToAZQuat(HYQuatToQuat(hyEyeRenderPose[HY_EYE_LEFT].m_rotation));
						AZ::Vector3 eyePosition = Vec3ToAZVec3(HYVec3ToVec3(hyEyeRenderPose[HY_EYE_LEFT].m_position));

						AZ::Quaternion qRecenterRotation = m_qBaseOrientation.GetInverseFull()*eyeRotation;
						qRecenterRotation.Normalize();
						AZ::Vector3 vRecenterPosition = (eyePosition - m_vBaseOffset) * m_fMeterToWorldScale;
						vRecenterPosition = m_qBaseOrientation.GetInverseFull()*vRecenterPosition;
						if (!m_bPosTrackingEnable)
						{
							vRecenterPosition.SetX(0.f);
							vRecenterPosition.SetY(0.f);
						}

						m_nativeStates[i].pose.orientation = m_nativeEyePoseStates.pose.orientation = m_localEyePoseStates.pose.orientation = qRecenterRotation;
						m_localStates[i].pose.position = m_nativeEyePoseStates.pose.position = m_localEyePoseStates.pose.position = vRecenterPosition;

					}

					if (i != Hmd)//controller
					{
						HyResult res;
						HyInputState controllerState;

						HySubDevice sid = static_cast<HySubDevice>(i);
						res = m_pVrDevice->GetControllerInputState(sid, controllerState);
						if (hySuccess == res)
						{
							/*m_controller.Update(sid, m_nativeStates[i], m_localStates[i], controllerState);*/
							m_controller->SetCurrentState(HyGetSubDeviceType(sid), m_localStates[i], controllerState);
						}
					}
					else//HMD
					{

						// Cache the current tracking state for later submission after this frame has finished rendering.
						{
							FrameParameters& frameParams = GetFrameParameters();
							frameParams.frameID = frameId;
						}
					}
				}
			}
		}

		// Process internal HyperealVR events.
// 		{
// 			vr::VREvent_t event;
// 			while (m_system->PollNextEvent(&event, sizeof(vr::VREvent_t)))
// 			{
// 				switch (event.eventType)
// 				{
// 				case vr::VREvent_TrackedDeviceActivated:
// 				{
// 					vr::ETrackedDeviceClass deviceClass = m_system->GetTrackedDeviceClass(event.trackedDeviceIndex);
// 					if (deviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller)
// 					{
// 						// A new controller has been connected to the system, process it.
// 						LogMessage("Controller connected:", event.trackedDeviceIndex);
// 						LogMessage("- Tracked Device Index: %i", event.trackedDeviceIndex);
// 						LogMessage("- Attached Device Id: %s", GetTrackedDeviceCharPointer(m_system, event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_AttachedDeviceId_String, nullptr));
// 						LogMessage("- Tracking System Name: %s", GetTrackedDeviceCharPointer(m_system, event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_TrackingSystemName_String, nullptr));
// 						LogMessage("- Model Number: %s", GetTrackedDeviceCharPointer(m_system, event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_ModelNumber_String, nullptr));
// 						LogMessage("- Serial Number: %s", GetTrackedDeviceCharPointer(m_system, event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_SerialNumber_String, nullptr));
// 						LogMessage("- Manufacturer: %s", GetTrackedDeviceCharPointer(m_system, event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_ManufacturerName_String, nullptr));
// 						LogMessage("- Tracking Firmware Version: %s", GetTrackedDeviceCharPointer(m_system, event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_TrackingFirmwareVersion_String, nullptr));
// 						LogMessage("- Hardware Revision: %s", GetTrackedDeviceCharPointer(m_system, event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_HardwareRevision_String, nullptr));
// 						LogMessage("- Is Wireless: %s", (m_system->GetBoolTrackedDeviceProperty(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_DeviceIsWireless_Bool) ? "True" : "False"));
// 						LogMessage("- Is Charging: %s", (m_system->GetBoolTrackedDeviceProperty(event.trackedDeviceIndex, vr::ETrackedDeviceProperty::Prop_DeviceIsCharging_Bool) ? "True" : "False"));
// 
// 						m_controller->ConnectController(event.trackedDeviceIndex);
// 					}
// 					break;
// 				}
// 
// 				case vr::VREvent_TrackedDeviceDeactivated:
// 				{
// 					vr::ETrackedDeviceClass deviceClass = m_system->GetTrackedDeviceClass(event.trackedDeviceIndex);
// 					if (deviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller)
// 					{
// 						// Controller has been disconnected.
// 						LogMessage("Controller disconnected: %i", event.trackedDeviceIndex);
// 
// 						m_controller->DisconnectController(event.trackedDeviceIndex);
// 					}
// 
// 					break;
// 				}
// 
// 				default:
// 					break;
// 				}
// 			}
// 		}
// 
// 		// Update tracking.
// 		{
// 			vr::Compositor_FrameTiming frameTiming;
// 			frameTiming.m_nSize = sizeof(vr::Compositor_FrameTiming);
// 			m_compositor->GetFrameTiming(&frameTiming, 0);
// 
// 			float secondsSinceLastVsync;
// 			m_system->GetTimeSinceLastVsync(&secondsSinceLastVsync, nullptr);
// 			float frequency = m_system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_DisplayFrequency_Float);
// 			float frameDuration = 1.0f / frequency;
// 			float secondsUntilPhotons = frameDuration - secondsSinceLastVsync + m_system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_SecondsFromVsyncToPhotons_Float);
// 			m_system->GetDeviceToAbsoluteTrackingPose(m_trackingOrigin, secondsUntilPhotons, m_trackedDevicePose, vr::k_unMaxTrackedDeviceCount);
// 
// 			for (int deviceIndex = 0; deviceIndex < vr::k_unMaxTrackedDeviceCount; ++deviceIndex)
// 			{
// 				AZ::VR::TrackingState::StatusFlags flags;
// 				flags = ((m_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) ? AZ::VR::HMDStatus_OrientationTracked | AZ::VR::HMDStatus_PositionConnected | AZ::VR::HMDStatus_CameraPoseTracked : 0) |
// 					((m_system->IsTrackedDeviceConnected(vr::k_unTrackedDeviceIndex_Hmd)) ? AZ::VR::HMDStatus_PositionConnected | AZ::VR::HMDStatus_HmdConnected : 0);
// 
// 				if (m_trackedDevicePose[deviceIndex].bPoseIsValid && m_trackedDevicePose[deviceIndex].bDeviceIsConnected)
// 				{
// 					vr::ETrackedDeviceClass deviceClass = m_system->GetTrackedDeviceClass(deviceIndex);
// 					switch (deviceClass)
// 					{
// 					case vr::ETrackedDeviceClass::TrackedDeviceClass_HMD:
// 					{
// 						m_trackingState.statusFlags = flags;
// 						CopyPose(m_trackedDevicePose[deviceIndex], m_trackingState.pose, m_trackingState.dynamics);
// 					}
// 					break;
// 
// 					case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller:
// 					{
// 						vr::VRControllerState_t buttonState;
// 						if (m_system->GetControllerState(deviceIndex, &buttonState))
// 						{
// 							AZ::VR::TrackingState controllerTrackingState;
// 							CopyPose(m_trackedDevicePose[deviceIndex], controllerTrackingState.pose, controllerTrackingState.dynamics);
// 							controllerTrackingState.statusFlags = flags;
// 
// 							m_controller->SetCurrentState(deviceIndex, controllerTrackingState, buttonState);
// 						}
// 					}
// 					break;
// 
// 					default:
// 						break;
// 					}
// 				}
// 			}
// 		}
	}

	AZ::VR::TrackingState* HyperealVRDevice::GetTrackingState()
	{
		return &m_trackingState;
	}

	void HyperealVRDevice::SubmitFrame(const EyeTarget& left, const EyeTarget& right)
	{
// 		if (m_compositor)
// 		{
// 			m_compositor->Submit(vr::Eye_Left, static_cast<vr::Texture_t*>(left.renderTarget));
// 			m_compositor->Submit(vr::Eye_Right, static_cast<vr::Texture_t*>(right.renderTarget));
// 
// 			m_compositor->WaitGetPoses(m_trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
// 		}
		const EyeTarget* eyes[] = { &left, &right };
		FrameParameters& frameParams = GetFrameParameters();

		for (int i=0;i<2;i++)
		{
			frameParams.m_RTDesc[i] = *static_cast<HyTextureDesc*> (eyes[i]->renderTarget);
		}

		if (m_pVrGraphicsCxt)
		{
			HyResult hr = hySuccess;

			hr = m_pVrGraphicsCxt->Submit(frameParams.frameID, frameParams.m_RTDesc, 2);
		}
	}

	void HyperealVRDevice::RecenterPose()
	{
		//if(m_lastFrameID_UpdateTrackingState>0)
			ResetOrientationAndPosition(0.0f);
	}

	void HyperealVRDevice::SetTrackingLevel(const AZ::VR::HMDTrackingLevel level)
	{
// 		switch (level)
// 		{
// 		case AZ::VR::HMDTrackingLevel::kHead:
// 			m_trackingOrigin = vr::ETrackingUniverseOrigin::TrackingUniverseSeated;
// 			break;
// 
// 		case AZ::VR::HMDTrackingLevel::kFloor:
// 			m_trackingOrigin = vr::ETrackingUniverseOrigin::TrackingUniverseStanding;
// 			break;
// 
// 		default:
// 			AZ_Assert(false, "Unknown tracking level %d requested for the HyperealVR", static_cast<int>(level));
// 			break;
// 		}
	}

	void HyperealVRDevice::OutputHMDInfo()
	{
// 		LogMessage("Device: %s", m_deviceInfo.productName);
// 		LogMessage("- Manufacturer: %s", m_deviceInfo.manufacturer);
// 		LogMessage("- SerialNumber: %s", GetTrackedDeviceString(m_system, vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_SerialNumber_String, nullptr).c_str());
// 		LogMessage("- Firmware: %s", GetTrackedDeviceString(m_system, vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_TrackingFirmwareVersion_String, nullptr).c_str());
// 		LogMessage("- Hardware Revision: %s", GetTrackedDeviceString(m_system, vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_HardwareRevision_String, nullptr).c_str());
// 		LogMessage("- Display Firmware: 0x%x", m_system->GetUint64TrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_HardwareRevision_Uint64));
// 		LogMessage("- Render Resolution: %dx%d", m_deviceInfo.renderWidth * 2, m_deviceInfo.renderHeight);
// 		LogMessage("- Refresh Rate: %.2f", m_system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::ETrackedDeviceProperty::Prop_DisplayFrequency_Float));
// 		LogMessage("- FOVH: %.2f", RAD2DEG(m_deviceInfo.fovH));
// 		LogMessage("- FOVV: %.2f", RAD2DEG(m_deviceInfo.fovV));
	}

	void HyperealVRDevice::EnableDebugging(bool enable)
	{
	}
	void HyperealVRDevice::DrawDebugInfo(const AZ::Transform& transform, IRenderAuxGeom* auxGeom)
	{
	}

	AZ::VR::HMDDeviceInfo* HyperealVRDevice::GetDeviceInfo()
	{
		return &m_deviceInfo;
	}

	bool HyperealVRDevice::IsInitialized()
	{
		return true;/* m_system != nullptr;*/
	}

	void HyperealVRDevice::RebuildPlayArea()
	{
		if (m_pPlayAreaVertices)
		{
			delete[] m_pPlayAreaVertices;
			m_pPlayAreaVertices = nullptr;
		}

		if (hySucceeded(m_pVrDevice->GetIntValue(HY_PROPERTY_CHAPERONE_VERTEX_COUNT_INT, m_nPlayAreaVertexCount)) && m_nPlayAreaVertexCount != 0)
		{
			m_pPlayAreaVertices = new HyVec2[m_nPlayAreaVertexCount];
			HyResult r = m_pVrDevice->GetFloatArray(HY_PROPERTY_CHAPERONE_VERTEX_VEC2_ARRAY, reinterpret_cast<float*>(m_pPlayAreaVertices), (uint)m_nPlayAreaVertexCount * 2);

			m_bPlayAreaValid = hySucceeded(r);

			if (!m_bPlayAreaValid)
			{
				delete[] m_pPlayAreaVertices;
				m_pPlayAreaVertices = nullptr;
			}
		}
	}

	// -------------------------------------------------------------------------
	const char* HyperealVRDevice::GetTrackedDeviceCharPointer(int nProperty)
	{
		int realStrLen = 512;
		//m_pVrDevice->GetStringValue(HY_PROPERTY_DEVICE_MANUFACTURER_STRING, 0, 0, &realStrLen);

		//if (realStrLen == 0)
		//	return nullptr;

		char* pBuffer = new char[realStrLen];
		m_pVrDevice->GetStringValue(HY_PROPERTY_MANUFACTURER_STRING, pBuffer, realStrLen, &realStrLen);
		return const_cast<char*>(pBuffer);
	}

	// -------------------------------------------------------------------------
	void HyperealVRDevice::UpdateInternal()
	{
		if (!m_pVrDevice || !m_pVrGraphicsCxt)
		{
			return;
		}
		const HyMsgHeader *msg;
		while (true)
		{
			m_pVrDevice->RetrieveMsg(&msg);
			if (msg->m_type == HY_MSG_NONE)
				break;
			switch (msg->m_type)
			{
			case HY_MSG_PENDING_QUIT:
				m_bIsQuitting = true;
				break;
			case HY_MSG_INPUT_FOCUS_CHANGED:
				break;
			case HY_MSG_VIEW_FOCUS_CHANGED:
				break;
			case HY_MSG_SUBDEVICE_STATUS_CHANGED:
			{
//  				HyMsgSubdeviceChange* pData = ((HyMsgSubdeviceChange*)msg);
//  				HySubDevice sid = static_cast<HySubDevice>(pData->m_subdevice);
//  				if (0 != pData->m_value)
//  					m_controller->ConnectController(sid);
//  				else
//  					m_controller->DisconnectController(sid);//?????bug? some times incorrect
			}
			break;
			default:
				m_pVrDevice->DefaultMsgFunction(msg);
				break;
			}
		}

		if (m_bIsQuitting)
		{
#if WITH_EDITOR
#endif	//WITH_EDITOR
			{
				gEnv->pSystem->Quit();
			}
			m_bIsQuitting = false;
		}

	}

	void HyperealVRDevice::GetPlayspace(Playspace& playspace) const
	{
// 		vr::HmdQuad_t openVRSpace;
// 		bool valid = vr::VRChaperone()->GetPlayAreaRect(&openVRSpace);
// 
// 		playspace.corners[0] = HYVec3ToVec3(openVRSpace.vCorners[0]);
// 		playspace.corners[1] = HYVec3ToVec3(openVRSpace.vCorners[1]);
// 		playspace.corners[2] = HYVec3ToVec3(openVRSpace.vCorners[2]);
// 		playspace.corners[3] = HYVec3ToVec3(openVRSpace.vCorners[3]);
// 		playspace.isValid = valid;
	}

	void HyperealVRDevice::ResetOrientationAndPosition(float Yaw)
	{
		ResetOrientation(Yaw);
		ResetPosition();
	}

	void HyperealVRDevice::ResetOrientation(float yaw)
	{
		const AZ::Quaternion& _qBaseOrientation = QuatToAZQuat(HYQuatToQuat(m_rTrackedDevicePose[EDevice::Hmd].m_pose.m_rotation));
		Quat qBaseOrientation(_qBaseOrientation.GetW(), _qBaseOrientation.GetX(), _qBaseOrientation.GetY(), _qBaseOrientation.GetZ());
		Ang3 currentAng = Ang3(qBaseOrientation);
		if (!m_bResetOrientationKeepPitchAndRoll)
		{
			currentAng.x = 0;//Pitch
			currentAng.y = 0;//Roll
		}

		if (fabs(yaw) > FLT_EPSILON)
		{
			currentAng.z -= yaw;
			//currentAng.Normalize();
		}
		const Quat& resQuat = Quat(currentAng);
		m_qBaseOrientation.SetW(resQuat.w);
		m_qBaseOrientation.SetX(resQuat.v.x);
		m_qBaseOrientation.SetY(resQuat.v.y);
		m_qBaseOrientation.SetZ(resQuat.v.z);
	}

	void HyperealVRDevice::ResetPosition()
	{
		m_vBaseOffset = Vec3ToAZVec3( HYVec3ToVec3(m_rTrackedDevicePose[EDevice::Hmd].m_pose.m_position));
	}

	HyperealVRDevice::FrameParameters& HyperealVRDevice::GetFrameParameters()
	{
		static IRenderer* renderer = gEnv->pRenderer;
		const int frameID = renderer->GetFrameID(false);

		return m_frameParams[frameID & 1]; 
	}
}
