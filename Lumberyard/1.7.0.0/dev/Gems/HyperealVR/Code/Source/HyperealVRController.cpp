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

	enum class HyperealInputSymbol
	{
		kButton,
		kIndexTrigger,
		kHandTrigger,
		kAxis
	};

#define BUTTON_DEFINES \
    X(BtnTouchPadRight,                        HY_BUTTON_TOUCHPAD_RIGHT,      HyperealInputSymbol::kButton,       AZ::VR::ControllerIndex::RightHand)  \
    X(BtnTouchPadLeft,                        HY_BUTTON_TOUCHPAD_LEFT,      HyperealInputSymbol::kButton,       AZ::VR::ControllerIndex::LeftHand)  \
    X(BtnMenuRight,                        HY_BUTTON_MENU,      HyperealInputSymbol::kButton,       AZ::VR::ControllerIndex::RightHand)   \
    X(BtnMenuLeft,                        HY_BUTTON_MENU,      HyperealInputSymbol::kButton,       AZ::VR::ControllerIndex::LeftHand)   \
	X(BtnHomeRight,                        HY_BUTTON_HOME,      HyperealInputSymbol::kButton,       AZ::VR::ControllerIndex::RightHand)   \
    X(BtnHomeLeft,                        HY_BUTTON_HOME,      HyperealInputSymbol::kButton,       AZ::VR::ControllerIndex::LeftHand)   \
    X(BtnTouchIndexTriggerLeft,              -1,               HyperealInputSymbol::kIndexTrigger, AZ::VR::ControllerIndex::LeftHand)   \
    X(BtnTouchIndexTriggerRight,             -1,               HyperealInputSymbol::kIndexTrigger, AZ::VR::ControllerIndex::RightHand)  \
/*    X(LeftHandTrigger,          -1,               HyperealInputSymbol::kHandTrigger,  AZ::VR::ControllerIndex::LeftHand)   \
    X(RightHandTrigger,         -1,               HyperealInputSymbol::kHandTrigger,  AZ::VR::ControllerIndex::RightHand) */ \
    X(LeftTouchPadX,          -1,               HyperealInputSymbol::kAxis,         AZ::VR::ControllerIndex::LeftHand)   \
    X(LeftTouchPadY,          -1,               HyperealInputSymbol::kAxis,         AZ::VR::ControllerIndex::LeftHand)   \
    X(RightTouchPadX,         -1,               HyperealInputSymbol::kAxis,         AZ::VR::ControllerIndex::RightHand)  \
    X(RightTouchPadY,         -1,               HyperealInputSymbol::kAxis,         AZ::VR::ControllerIndex::RightHand) 

#define X(buttonId, deviceButtonId, buttonType, hand) k##buttonId,
	enum ButtonIds
	{
		BUTTON_DEFINES
		BUTTON_COUNT
	};
#undef X
// 
// #define BUTTON_DEFINES \
//     X(BtnTouchPadRight,				HY_BUTTON_TOUCHPAD_RIGHT,			SInputSymbol::Button)  \
//     X(BtnTouchPadLeft,				HY_BUTTON_TOUCHPAD_LEFT,			SInputSymbol::Button)  \
//     X(BtnMenu,				HY_BUTTON_MENU,          SInputSymbol::Button)  \
//     X(BtnHome,				HY_BUTTON_HOME,			SInputSymbol::Button)  \
// 	X(TouchRight,				HY_TOUCH_TOUCHPAD_RIGHT,          SInputSymbol::Axis)  \
//     X(TouchLeft,				HY_TOUCH_TOUCHPAD_LEFT,			SInputSymbol::Axis)  \
//     X(TriggerLeft,      HY_TOUCH_INDEX_TRIGGER_RIGHT,   SInputSymbol::Trigger)  \
//     X(TriggerRight,           HY_TOUCH_INDEX_TRIGGER_LEFT,          SInputSymbol::Trigger)  
//  
// #define X(buttonId, deviceButtonId, buttonType) k##buttonId,
// 	enum ButtonIds
// 	{
// 		BUTTON_DEFINES
// 		BUTTON_COUNT
// 	};
// #undef X

	struct HyperealTouchButton
	{
		HyperealTouchButton(const char* buttonName, const uint32 buttonDeviceID, const HyperealInputSymbol buttonType, AZ::VR::ControllerIndex buttonHand) :
			name(buttonName),
			deviceID(buttonDeviceID),
			type(buttonType),
			hand(buttonHand)
		{
		}

		const char* name;
		const uint32 deviceID;
		const HyperealInputSymbol type;
		const AZ::VR::ControllerIndex hand;
	};

#define X(buttonId, deviceButtonId, buttonType, hand) HyperealTouchButton("OculusTouch_" #buttonId, deviceButtonId, buttonType, hand),
	const HyperealTouchButton g_deviceButtons[] =
	{
		BUTTON_DEFINES
		HyperealTouchButton("InvalidID", -1, HyperealInputSymbol::kButton, AZ::VR::ControllerIndex::RightHand)
	};
#undef X


	static const unsigned short kMaxPulseDurationMicroSec = 3999; // 5ms - minimum vibrate time. 3999 max value

	HyperealVRController::HyperealVRController(HyDevice *system) :
		m_system(system),
		m_deviceIndex(0),
		m_deviceID(0),
		m_enabled(false)
	{
		memset(&m_previousState, 0, sizeof(m_previousState));
		memset(&m_currentState, 0, sizeof(m_currentState));

// 		for (uint32 index = 0; index < static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers); ++index)
// 		{
// 			m_contollerMapping[index] = HY_SUBDEV_CONTROLLER_RESERVED;//max controller id
// 		}

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
		for (uint32 buttonIndex = 0; buttonIndex < BUTTON_COUNT; ++buttonIndex)
		{
			SInputEvent event;
			SInputSymbol* symbol = m_symbolMapping[buttonIndex];

			const HyperealTouchButton* button = &g_deviceButtons[buttonIndex];
			switch (button->type)
			{
			case HyperealInputSymbol::kButton:
			{
				bool isPressed = m_currentState[static_cast<uint32_t>(button->hand)].IsButtonPressed(button->deviceID);
				bool wasPressed = m_previousState[static_cast<uint32_t>(button->hand)].IsButtonPressed(button->deviceID);
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

			case HyperealInputSymbol::kIndexTrigger:
			{
				float currentValue = m_currentState[static_cast<uint32_t>(button->hand)].inputState.m_indexTrigger;
				float previousValue = m_previousState[static_cast<uint32_t>(button->hand)].inputState.m_indexTrigger;
				if (currentValue != previousValue)
				{
					symbol->ChangeEvent(currentValue);
					symbol->AssignTo(event);
					event.deviceIndex = m_deviceIndex;
					event.deviceType = eIDT_MotionController;
					gEnv->pInput->PostInputEvent(event);
				}
			}
			break;

// 			case HyperealInputSymbol::kHandTrigger:
// 			{
// 				float currentValue = m_currentState[static_cast<uint32_t>(button->hand)].inputState.HandTrigger;
// 				float previousValue = m_previousState[static_cast<uint32_t>(button->hand)].inputState.HandTrigger;
// 				if (currentValue != previousValue)
// 				{
// 					symbol->ChangeEvent(currentValue);
// 					symbol->AssignTo(event);
// 					event.deviceIndex = m_deviceIndex;
// 					event.deviceType = eIDT_MotionController;
// 					gEnv->pInput->PostInputEvent(event);
// 				}
// 			}
// 			break;

			case HyperealInputSymbol::kAxis:
			{
				HyVec2 currentState = m_currentState[static_cast<uint32_t>(button->hand)].inputState.m_touchpad;
				HyVec2 previousState = m_previousState[static_cast<uint32_t>(button->hand)].inputState.m_touchpad;

				// Process X and Y as separate events.
				{
					if (currentState.x != previousState.x)
					{
						symbol->ChangeEvent(currentState.x);
						symbol->AssignTo(event);
						event.deviceIndex = m_deviceIndex;
						event.deviceType = eIDT_MotionController;
						gEnv->pInput->PostInputEvent(event);
					}
				}

				{
					if (currentState.y != previousState.y)
					{
						symbol->ChangeEvent(currentState.y);
						symbol->AssignTo(event);
						event.deviceIndex = m_deviceIndex;
						event.deviceType = eIDT_MotionController;
						gEnv->pInput->PostInputEvent(event);
					}
				}
			}
			break;

			default:
				AZ_Assert(0, "Unknown OculusInputSymbol type");
				break;
			}

			// handle timed vibrate
			if (m_FFParams.timeInSeconds > 0.0f)
			{
				m_FFParams.timeInSeconds -= gEnv->pTimer->GetFrameTime();
				float strengthA = m_FFParams.strengthA;
				float strengthB = m_FFParams.strengthB;

				if (m_FFParams.timeInSeconds <= 0.0f)
				{
					strengthA = 0.0f;
					strengthB = 0.0f;
				}

				m_system->SetControllerVibration((HySubDevice)m_contollerMapping[0], static_cast<unsigned short>(strengthA*kMaxPulseDurationMicroSec), strengthA);
				m_system->SetControllerVibration((HySubDevice)m_contollerMapping[1], static_cast<unsigned short>(strengthB*kMaxPulseDurationMicroSec), strengthB);
			}
		}
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
		return g_deviceButtons[event.keyId].name;
	}

	const char* HyperealVRController::GetKeyName(const EKeyId keyId) const
	{
		return g_deviceButtons[keyId].name;
	}

	char HyperealVRController::GetInputCharAscii(const SInputEvent& event)
	{
		return 'e';
	}

	const char* HyperealVRController::GetOSKeyName(const SInputEvent& event)
	{
		return g_deviceButtons[event.keyId].name;
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
		return (m_contollerMapping[static_cast<uint32_t>(controllerIndex)] != HY_SUBDEV_Unknown);
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
	void HyperealVRController::SetCurrentState(const uint32_t deviceIndex, const AZ::VR::TrackingState& trackingState, const HyInputState& inputState)
	{
		m_previousState[deviceIndex] = m_currentState[deviceIndex];
		memcpy(&m_currentState[deviceIndex].inputState, &inputState, sizeof(inputState));
		
	}
	void MapSymbol(const uint32 buttonIndex, const uint32 deviceID, const TKeyName& name, const HyperealInputSymbol type, IInputDevice::TDevSpecIdToSymbolMap& mapping)
	{
		SInputSymbol::EType symbolType;
		switch (type)
		{
		case HyperealInputSymbol::kButton:
			symbolType = SInputSymbol::Button;
			break;

		case HyperealInputSymbol::kHandTrigger:
		case HyperealInputSymbol::kIndexTrigger:
			symbolType = SInputSymbol::Trigger;
			break;

		case HyperealInputSymbol::kAxis:
			symbolType = SInputSymbol::Axis;
			break;

		default:
			AZ_Assert(0, "Unknown OculusInputSymbol type");
			break;
		}

		SInputSymbol* symbol = new SInputSymbol(deviceID, static_cast<EKeyId>(KI_MOTION_BASE), name, symbolType);
		symbol->deviceType = eIDT_MotionController;
		symbol->state = eIS_Unknown;
		symbol->value = 0.0f;

		mapping[buttonIndex] = symbol;
	}

	void HyperealVRController::CreateInputMapping()
	{
		for (uint32 index = 0; index < BUTTON_COUNT; ++index)
		{
			const HyperealTouchButton* button = &g_deviceButtons[index];
			MapSymbol(index, button->deviceID, button->name, button->type, m_symbolMapping);
		}
	}


// 	void HyperealVRController::ConnectController(const uint32_t deviceIndex)
// 	{
// 		for (uint32 index = 0; index < static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers); ++index)
// 		{
// 			if (m_contollerMapping[index] == HY_SUBDEV_CONTROLLER_RESERVED/*vr::k_unTrackedDeviceIndexInvalid*/)
// 			{
// 				// This is an unused controller slot.
// 				m_contollerMapping[index] = deviceIndex;
// 				return;
// 			}
// 		}
// 
// 		AZ_Assert(0, "Attempting to map more than %d HyperealVR controllers", static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers));
// 	}
// 
// 	void HyperealVRController::DisconnectController(const uint32_t deviceIndex)
// 	{
// 		for (uint32 index = 0; index < static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers); ++index)
// 		{
// 			if (m_contollerMapping[index] == deviceIndex)
// 			{
// 				m_contollerMapping[index] = HY_SUBDEV_CONTROLLER_RESERVED/*vr::k_unTrackedDeviceIndexInvalid*/;
// 				return;
// 			}
// 		}
// 
// 		AZ_Assert(0, "Attempting to disconnect an HyperealVR controller that doesn't exist");
// 	}

} // namespace HyperealVR
