/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "StdAfx.h"
#include "HyperealVRController.h"
#include <sstream>

namespace HyperealVR
{

#define BUTTON_DEFINES \
    X(BtnA,				HY_BUTTON_A,			SInputSymbol::Button)  \
    X(BtnB,				HY_BUTTON_B,			SInputSymbol::Button)  \
    X(BtnX,				HY_BUTTON_X,          SInputSymbol::Button)  \
    X(BtnY,				HY_BUTTON_Y,			SInputSymbol::Button)  \
    X(TriggerBtnL,      HY_BUTTON_THUMB_LEFT,   SInputSymbol::Trigger)  \
    X(TriggerBtnR,           HY_BUTTON_THUMB_RIGHT,          SInputSymbol::Trigger)  \
    X(SideTriggerBtnL,         HY_BUTTON_SHOULDER_LEFT,        SInputSymbol::Trigger)  \
    X(SideTriggerBtnR,                HY_BUTTON_SHOULDER_RIGHT,                SInputSymbol::Trigger) 
 
#define X(buttonId, deviceButtonId, buttonType) k##buttonId,
	enum ButtonIds
	{
		BUTTON_DEFINES
		BUTTON_COUNT
	};
#undef X

	struct HyperealVRButton
	{
		HyperealVRButton(const char* buttonName, const HyButton& buttonDeviceID, const SInputSymbol::EType& buttonType) :
			name(buttonName),
			deviceID(buttonDeviceID),
			type(buttonType)
		{
		}

		const char* name;
		const HyButton deviceID;
		const SInputSymbol::EType type;
	};


	const HyperealVRButton g_deviceButtons[static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers)][BUTTON_COUNT] =
	{
#define X(buttonId, deviceButtonId, buttonType) HyperealVRButton("HyperealVR_" #buttonId "_0", deviceButtonId, buttonType),
		{
			BUTTON_DEFINES
		},
#undef X
#define X(buttonId, deviceButtonId, buttonType) HyperealVRButton("HyperealVR_" #buttonId "_1", deviceButtonId, buttonType),
		{
			BUTTON_DEFINES
		}
#undef X
	};

	static const unsigned short kMaxPulseDurationMicroSec = 3999; // 5ms - minimum vibrate time. 3999 max value

	HyperealVRController::HyperealVRController(HyDevice *system) :
		m_system(system),
		m_deviceIndex(0),
		m_deviceID(0),
		m_enabled(false)
	{
		memset(&m_previousState, 0, sizeof(m_previousState));
		memset(&m_currentState, 0, sizeof(m_currentState));

		for (uint32 index = 0; index < static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers); ++index)
		{
			m_contollerMapping[index] = HY_SUBDEV_CONTROLLER_RESERVED;//max controller id
		}

		CreateInputMapping();
		CrySystemEventBus::Handler::BusConnect();
	}
	HyperealVRController::~HyperealVRController()
	{
		CrySystemEventBus::Handler::BusDisconnect();
		TDevSpecIdToSymbolMap::iterator iter = m_symbolMapping.begin();
		for (; iter != m_symbolMapping.end(); ++iter)
		{
			delete iter->second;
		}
	}

	const char* HyperealVRController::GetDeviceName() const
	{
		return "HyperealVR Controller";
	}

	EInputDeviceType HyperealVRController::GetDeviceType() const
	{
		return eIDT_MotionController;
	}

	TInputDeviceId HyperealVRController::GetDeviceId() const
	{
		return m_deviceID;
	}

	int HyperealVRController::GetDeviceIndex() const
	{
		return m_deviceIndex;
	}

	bool HyperealVRController::Init()
	{
		return true;
	}

	void HyperealVRController::PostInit()
	{
	}

	void HyperealVRController::Update(bool focus)
	{
		// Note that the axis range from k_EButton_Axis0 to k_EButton_Axis4. Since these are don't matchup with the
		// rAxis array from HyperealVR we have to subtract any axis index by k_EButton_Axis0 in order to get the proper
		// rAxis array index.

		for (uint32 controllerIndex = 0; controllerIndex < static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers); ++controllerIndex)
		{
			// Make sure this controller is currently mapped (enabled).
			if (m_contollerMapping[controllerIndex] != HY_SUBDEV_CONTROLLER_RESERVED/*vr::k_unTrackedDeviceIndexInvalid*/)
			{
				ControllerState* currentState = &m_currentState[controllerIndex];
				ControllerState* previousState = &m_previousState[controllerIndex];

				for (uint32 buttonIndex = 0; buttonIndex < BUTTON_COUNT; ++buttonIndex)
				{
					SInputEvent event;
					const HyperealVRButton* button = &g_deviceButtons[controllerIndex][buttonIndex];
					SInputSymbol* symbol = m_symbolMapping[buttonIndex + (controllerIndex * BUTTON_COUNT)];

					switch (button->type)
					{
					case SInputSymbol::Button:
					{
						bool isPressed = currentState->IsButtonPressed(button->deviceID);
						bool wasPressed = previousState->IsButtonPressed(button->deviceID);
						if (isPressed != wasPressed)
						{
							symbol->PressEvent(isPressed);
							symbol->AssignTo(event);
							event.deviceIndex = m_deviceIndex;
							event.deviceType = eIDT_MotionController;
							gEnv->pInput->PostInputEvent(event);
						}
					}
					break;

					case SInputSymbol::Trigger:
					{
						float trigger = currentState->buttonState.m_trigger/*rAxis[button->deviceID - vr::k_EButton_Axis0].x*/;
						float previousTrigger = previousState->buttonState.m_trigger/*.rAxis[button->deviceID - vr::k_EButton_Axis0].x*/;

						if (trigger != previousTrigger)
						{
							symbol->ChangeEvent(trigger);
							symbol->AssignTo(event);
							event.deviceIndex = m_deviceIndex;
							event.deviceType = eIDT_MotionController;
							gEnv->pInput->PostInputEvent(event);
						}
					}
					break;

					case SInputSymbol::Axis:
					{
						// Process X and Y separately.
						{
							float x = currentState->buttonState.m_thumbstick.x/*rAxis[button->deviceID - vr::k_EButton_Axis0].x*/;
							float previousX = previousState->buttonState.m_thumbstick.x/*rAxis[button->deviceID - vr::k_EButton_Axis0].x*/;

							if (x != previousX)
							{
								symbol->ChangeEvent(x);
								symbol->AssignTo(event);
								event.deviceIndex = m_deviceIndex;
								event.deviceType = eIDT_MotionController;
								gEnv->pInput->PostInputEvent(event);
							}
						}

						{
							float y = currentState->buttonState.m_thumbstick.y/*rAxis[button->deviceID - vr::k_EButton_Axis0].y*/;
							float previousY = previousState->buttonState.m_thumbstick.y/*rAxis[button->deviceID - vr::k_EButton_Axis0].y*/;

							if (y != previousY)
							{
								symbol->ChangeEvent(y);
								symbol->AssignTo(event);
								event.deviceIndex = m_deviceIndex;
								event.deviceType = eIDT_MotionController;
								gEnv->pInput->PostInputEvent(event);
							}
						}
					}
					break;

					default:
						AZ_Assert(0, "Unknown button type");
						break;
					}
				}
			}
		}

		// handle timed vibrate
// 		if (m_FFParams.timeInSeconds > 0.0f)
// 		{
// 			m_FFParams.timeInSeconds -= gEnv->pTimer->GetFrameTime();
// 			float strengthA = m_FFParams.strengthA;
// 			float strengthB = m_FFParams.strengthB;
// 
// 			if (m_FFParams.timeInSeconds <= 0.0f)
// 			{
// 				strengthA = 0.0f;
// 				strengthB = 0.0f;
// 			}
// 
// 			if (m_system != nullptr)
// 			{
// 				m_system->TriggerHapticPulse(m_contollerMapping[0], 0, static_cast<unsigned short>(strengthA*kMaxPulseDurationMicroSec));
// 				m_system->TriggerHapticPulse(m_contollerMapping[1], 0, static_cast<unsigned short>(strengthB*kMaxPulseDurationMicroSec));
// 			}
// 		}
	}

	bool HyperealVRController::SetForceFeedback(IFFParams params)
	{
		// on basic rumble, set during update loop
		if (params.effectId == eFF_Rumble_Basic)
		{
			m_FFParams = params;
		}
		else
		{
			if (m_system != nullptr)
			{
				m_system->SetControllerVibration((HySubDevice)m_contollerMapping[0], static_cast<unsigned short>(params.strengthA*kMaxPulseDurationMicroSec), params.strengthA);
				m_system->SetControllerVibration((HySubDevice)m_contollerMapping[1], static_cast<unsigned short>(params.strengthB*kMaxPulseDurationMicroSec), params.strengthB);
			}
		}

		return true;
	}

	bool HyperealVRController::InputState(const TKeyName& key, EInputState state)
	{
		return true;
	}

	bool HyperealVRController::SetExclusiveMode(bool value)
	{
		return true;
	}

	void HyperealVRController::ClearKeyState()
	{
	}

	void HyperealVRController::ClearAnalogKeyState(TInputSymbols& clearedSymbols)
	{

	}

	void HyperealVRController::SetUniqueId(uint8 const uniqueId)
	{
		m_deviceID = uniqueId;
	}

	const char* HyperealVRController::GetKeyName(const SInputEvent& event) const
	{
		return g_deviceButtons[0][event.keyId].name;
	}

	const char* HyperealVRController::GetKeyName(const EKeyId keyId) const
	{
		return g_deviceButtons[0][keyId].name;
	}

	char HyperealVRController::GetInputCharAscii(const SInputEvent& event)
	{
		return 'e';
	}

	const char* HyperealVRController::GetOSKeyName(const SInputEvent& event)
	{
		return g_deviceButtons[0][event.keyId].name;
	}

	SInputSymbol* HyperealVRController::LookupSymbol(EKeyId id) const
	{
		TDevSpecIdToSymbolMap::const_iterator iter = m_symbolMapping.find(id);
		if (iter != m_symbolMapping.end())
		{
			return iter->second;
		}

		return nullptr;
	}

	const SInputSymbol* HyperealVRController::GetSymbolByName(const char* name) const
	{
		TDevSpecIdToSymbolMap::const_iterator iter = m_symbolMapping.begin();
		for (; iter != m_symbolMapping.end(); ++iter)
		{
			if (iter->second->name == TKeyName(name))
			{
				return iter->second;
			}
		}

		return nullptr;
	}

	bool HyperealVRController::IsOfDeviceType(EInputDeviceType type) const
	{
		return (type == eIDT_MotionController);
	}

	void HyperealVRController::Enable(bool enable)
	{
		m_enabled = enable;
	}

	bool HyperealVRController::IsEnabled() const
	{
		return m_enabled;
	}

	void HyperealVRController::OnLanguageChange()
	{

	}

	void HyperealVRController::SetDeadZone(float threshold)
	{

	}

	void HyperealVRController::RestoreDefaultDeadZone()
	{

	}

	const IInputDevice::TDevSpecIdToSymbolMap& HyperealVRController::GetInputToSymbolMappings() const
	{
		return m_symbolMapping;
	}

	AZ::VR::TrackingState* HyperealVRController::GetTrackingState(AZ::VR::ControllerIndex controllerIndex)
	{
		return &m_currentState[static_cast<uint32_t>(controllerIndex)].trackingState;
	}

	bool HyperealVRController::IsConnected(AZ::VR::ControllerIndex controllerIndex)
	{
		return (m_contollerMapping[static_cast<uint32_t>(controllerIndex)] != HY_SUBDEV_CONTROLLER_RESERVED);
	}

	void HyperealVRController::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
	{
		if (gEnv->pInput)
		{
			system.GetIInput()->AddInputDevice(this);
		}
	}

	void HyperealVRController::ConnectToControllerBus()
	{
		AZ::VR::ControllerRequestBus::Handler::BusConnect();
	}

	void HyperealVRController::DisconnectFromControllerBus()
	{
		AZ::VR::ControllerRequestBus::Handler::BusDisconnect();
	}

	void HyperealVRController::SetCurrentState(const uint32_t deviceIndex, const AZ::VR::TrackingState& trackingState, const HyInputState& buttonState)
	{
		uint32 index = 0;
		for (; index < static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers); ++index)
		{
			if (m_contollerMapping[index] == deviceIndex)
			{
				break;
			}
		}

		AZ_Assert(index != static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers), "Attempting to set state on a controller that has not been mapped");

		m_previousState[index] = m_currentState[index];
		m_currentState[index].Set(trackingState, buttonState);
	}

	void MapSymbol(uint32 buttonIndex, uint32 deviceID, const TKeyName& name, SInputSymbol::EType type, IInputDevice::TDevSpecIdToSymbolMap& mapping)
	{
		SInputSymbol* symbol = new SInputSymbol(deviceID, static_cast<EKeyId>(KI_MOTION_BASE), name, type);
		symbol->deviceType = eIDT_MotionController;
		symbol->state = eIS_Unknown;
		symbol->value = 0.0f;

		mapping[buttonIndex] = symbol;
	}

	void HyperealVRController::CreateInputMapping()
	{
		for (uint32 controllerIndex = 0; controllerIndex < static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers); ++controllerIndex)
		{
			for (uint32 index = 0; index < BUTTON_COUNT; ++index)
			{
				const HyperealVRButton* button = &g_deviceButtons[controllerIndex][index];
				uint32 buttonIndex = index + (controllerIndex * BUTTON_COUNT);
				MapSymbol(buttonIndex, button->deviceID, button->name, button->type, m_symbolMapping);
			}
		}
	}

	void HyperealVRController::ConnectController(const uint32_t deviceIndex)
	{
		for (uint32 index = 0; index < static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers); ++index)
		{
			if (m_contollerMapping[index] == HY_SUBDEV_CONTROLLER_RESERVED/*vr::k_unTrackedDeviceIndexInvalid*/)
			{
				// This is an unused controller slot.
				m_contollerMapping[index] = deviceIndex;
				return;
			}
		}

		AZ_Assert(0, "Attempting to map more than %d HyperealVR controllers", static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers));
	}

	void HyperealVRController::DisconnectController(const uint32_t deviceIndex)
	{
		for (uint32 index = 0; index < static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers); ++index)
		{
			if (m_contollerMapping[index] == deviceIndex)
			{
				m_contollerMapping[index] = HY_SUBDEV_CONTROLLER_RESERVED/*vr::k_unTrackedDeviceIndexInvalid*/;
				return;
			}
		}

		AZ_Assert(0, "Attempting to disconnect an HyperealVR controller that doesn't exist");
	}

} // namespace HyperealVR
