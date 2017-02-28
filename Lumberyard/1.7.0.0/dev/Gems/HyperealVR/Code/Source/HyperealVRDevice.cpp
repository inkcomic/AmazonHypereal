
#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "HyperealVRDevice.h"

#define HY_RELEASE(p) {if(p != nullptr) p->Release(); p = nullptr;}
#define DEFAULT_IPD 0.064f;


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


namespace HyperealVR
{
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
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void HyperealVRDevice::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
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

    void HyperealVRDevice::Init()
    {
// 		memset(m_trackedDevicePose, 0, sizeof(m_trackedDevicePose));
// 		m_controller = new OpenVRController(m_system); // Note that this will be deleted by the input system and should not be deleted here.
    }

    void HyperealVRDevice::Activate()
    {
        HyperealVRRequestBus::Handler::BusConnect();
    }

    void HyperealVRDevice::Deactivate()
    {
        HyperealVRRequestBus::Handler::BusDisconnect();
    }


	bool HyperealVRDevice::AttemptInit()
	{
 		//LogMessage("Attempting to initialize OpenVR SDK");

		bool success = false;


		{
			for (int i = 0; i < 2; ++i)
			{
				m_RTDesc[i].m_uvSize = HyVec2{ 1.f, 1.f };
				m_RTDesc[i].m_uvOffset = HyVec2{ 0.f, 0.f };
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

			m_pVrDevice->GetIntValue(HY_PROPERTY_DEVICE_RESOLUTION_X_INT, m_VrDeviceInfo.DeviceResolutionX);
			m_pVrDevice->GetIntValue(HY_PROPERTY_DEVICE_RESOLUTION_Y_INT, m_VrDeviceInfo.DeviceResolutionY);
			m_pVrDevice->GetFloatArray(HY_PROPERTY_DEVICE_LEFT_EYE_FOV_FLOAT4_ARRAY, m_VrDeviceInfo.Fov[HY_EYE_LEFT].val, 4);
			m_pVrDevice->GetFloatArray(HY_PROPERTY_DEVICE_RIGHT_EYE_FOV_FLOAT4_ARRAY, m_VrDeviceInfo.Fov[HY_EYE_RIGHT].val, 4);



			m_eyeFovSym = ComputeSymmetricalFov(m_VrDeviceInfo.Fov[HY_EYE_LEFT], m_VrDeviceInfo.Fov[HY_EYE_RIGHT]);
			//set for  generic HMD device
			m_deviceInfo.renderWidth = (uint)m_VrDeviceInfo.DeviceResolutionX;
			m_deviceInfo.renderHeight = (uint)m_VrDeviceInfo.DeviceResolutionY;

			m_deviceInfo.manufacturer = GetTrackedDeviceCharPointer(HY_PROPERTY_DEVICE_MANUFACTURER_STRING);
			m_deviceInfo.productName = GetTrackedDeviceCharPointer(HY_PROPERTY_DEVICE_PRODUCT_NAME_STRING);
			m_deviceInfo.fovH = 2.0f * atanf(m_eyeFovSym.m_leftTan);
			m_deviceInfo.fovV = 2.0f * atanf(m_eyeFovSym.m_upTan);


			bool isConnected = false;
			hr = m_pVrDevice->GetBoolValue(HY_PROPERTY_HMD_CONNECTED_BOOL, isConnected);
			if (hySucceeded(hr) && isConnected)
			{
				//m_pVrDevice->ConfigureTrackingOrigin(m_pTrackingOriginCVar->GetIVal() == (int)EHmdTrackingOrigin::Floor == 1 ? HY_TRACKING_ORIGIN_FLOOR : HY_TRACKING_ORIGIN_EYE);

				RebuildPlayArea();
				gEnv->pLog->Log("[HMD][Hypereal] EnableStereo successfully.");
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
// 					OpenVRRequestBus::Handler::BusConnect();
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
// 		m_controller->DisconnectFromControllerBus();
// 
// 		AZ::VR::HMDDeviceRequestBus::Handler::BusDisconnect();
// 		OpenVRRequestBus::Handler::BusDisconnect();
// 		vr::VR_Shutdown();

		HY_RELEASE(m_pVrDevice);

		m_bVRSystemValid = false;

		if (m_bVRInitialized)
		{
			m_bVRInitialized = false;
			HyShutdown();
		}
	}



	void HyperealVRDevice::GetPerEyeCameraInfo(const EStereoEye eye, const float nearPlane, const float farPlane, AZ::VR::PerEyeCameraInfo& cameraInfo)
	{
// 		float left, right, top, bottom;
// 		m_system->GetProjectionRaw(MapOpenVREyeToLY(eye), &left, &right, &top, &bottom);
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

	bool HyperealVRDevice::CreateRenderTargets(void* renderDevice, const TextureDesc& desc, size_t eyeCount, AZ::VR::HMDRenderTarget* renderTargets[])
	{
// 		for (size_t i = 0; i < eyeCount; ++i)
// 		{
// 			ID3D11Device* d3dDevice = static_cast<ID3D11Device*>(renderDevice);
// 
// 			D3D11_TEXTURE2D_DESC textureDesc;
// 			textureDesc.Width = desc.width;
// 			textureDesc.Height = desc.height;
// 			textureDesc.MipLevels = 1;
// 			textureDesc.ArraySize = 1;
// 			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
// 			textureDesc.SampleDesc.Count = 1;
// 			textureDesc.SampleDesc.Quality = 0;
// 			textureDesc.Usage = D3D11_USAGE_DEFAULT;
// 			textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
// 			textureDesc.CPUAccessFlags = 0;
// 			textureDesc.MiscFlags = 0;
// 
// 			ID3D11Texture2D* texture;
// 			d3dDevice->CreateTexture2D(&textureDesc, nullptr, &texture);
// 
// 			// Create a OpenVR texture that is associated with this new D3D texture.
// 			vr::Texture_t* deviceTexture = new vr::Texture_t();
// 			deviceTexture->eColorSpace = vr::EColorSpace::ColorSpace_Auto;
// 			deviceTexture->eType = vr::EGraphicsAPIConvention::API_DirectX;
// 			deviceTexture->handle = texture;
// 
// 			// We only create one texture for OpenVR (no swapchain).
// 			renderTargets[i]->deviceSwapTextureSet = deviceTexture;
// 			renderTargets[i]->numTextures = 1;
// 			renderTargets[i]->textures = new void*[1];
// 			renderTargets[i]->textures[0] = texture;
// 		}

		return true;
	}

	void HyperealVRDevice::DestroyRenderTarget(AZ::VR::HMDRenderTarget& renderTarget)
	{
// 		vr::TextureID_t* deviceTexture = static_cast<vr::TextureID_t*>(renderTarget.deviceSwapTextureSet);
// 		SAFE_DELETE(deviceTexture);
	}

	void HyperealVRDevice::Update()
	{
		// Process internal OpenVR events.
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
	}

	void HyperealVRDevice::RecenterPose()
	{
// 		if (m_system)
// 		{
// 			m_system->ResetSeatedZeroPose();
// 		}
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
// 			AZ_Assert(false, "Unknown tracking level %d requested for the Oculus", static_cast<int>(level));
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
		m_pVrDevice->GetStringValue(HY_PROPERTY_DEVICE_MANUFACTURER_STRING, pBuffer, realStrLen, &realStrLen);
		return const_cast<char*>(pBuffer);
	}
}
