
#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "HyperealVRDevice.h"

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
// 		LogMessage("Attempting to initialize OpenVR SDK");
// 
// 		bool success = false;
// 
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
// 
// 		if (!success)
// 		{
// 			// Something went wrong during initialization.
// 			if (error != vr::EVRInitError::VRInitError_None)
// 			{
// 				LogMessage("Unable to initialize VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(error));
// 			}
// 
// 			Shutdown();
// 		}
// 
// 		return success;
		return true;
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

}
