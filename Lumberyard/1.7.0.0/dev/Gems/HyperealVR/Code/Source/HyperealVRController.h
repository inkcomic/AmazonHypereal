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

#pragma once

#include <IInput.h>
#include "HMDBus.h"
#include "VRControllerBus.h"
#include <CrySystemBus.h>

#include <Hypereal_VR.h>

namespace HyperealVR
{

	class HyperealVRController
		: public AZ::VR::ControllerRequestBus::Handler
		, public IInputDevice
		, protected CrySystemEventBus::Handler
	{
	public:

		HyperealVRController(HyDevice *system);
		~HyperealVRController();

		// IInputDevice overrides //////////////////////////////////////////////////
		const char* GetDeviceName() const override;
		EInputDeviceType GetDeviceType() const override;
		TInputDeviceId GetDeviceId() const override;
		int GetDeviceIndex() const override;
		bool Init() override;
		void PostInit() override;
		void Update(bool focus) override;
		bool SetForceFeedback(IFFParams params) override;
		bool InputState(const TKeyName& key, EInputState state) override;
		bool SetExclusiveMode(bool value) override;
		void ClearKeyState() override;
		void ClearAnalogKeyState(TInputSymbols& clearedSymbols) override;
		void SetUniqueId(uint8 const uniqueId) override;
		const char* GetKeyName(const SInputEvent& event) const override;
		const char* GetKeyName(const EKeyId keyId) const override;
		char GetInputCharAscii(const SInputEvent& event) override;
		const char* GetOSKeyName(const SInputEvent& event) override;
		SInputSymbol* LookupSymbol(EKeyId id) const override;
		const SInputSymbol* GetSymbolByName(const char* name) const override;
		bool IsOfDeviceType(EInputDeviceType type) const override;
		void Enable(bool enable) override;
		bool IsEnabled() const override;
		void OnLanguageChange() override;
		void SetDeadZone(float threshold) override;
		void RestoreDefaultDeadZone() override;
		const IInputDevice::TDevSpecIdToSymbolMap& GetInputToSymbolMappings() const override;
		////////////////////////////////////////////////////////////////////////////

		// IHMDController overrides ////////////////////////////////////////////////
		AZ::VR::TrackingState* GetTrackingState(AZ::VR::ControllerIndex controllerIndex) override;
		bool IsConnected(AZ::VR::ControllerIndex controllerIndex) override;
		////////////////////////////////////////////////////////////////////////////

		// CrySystemEventBus ///////////////////////////////////////////////////////
		void OnCrySystemInitialized(ISystem& system, const SSystemInitParams&) override;
		////////////////////////////////////////////////////////////////////////////

		void ConnectToControllerBus();
		void DisconnectFromControllerBus();

		void SetCurrentState(const uint32_t deviceIndex, const AZ::VR::TrackingState& trackingState, const HyInputState& buttonState);
		void ConnectController(const uint32_t deviceIndex);
		void DisconnectController(const uint32_t deviceIndex);

	private:

		HyperealVRController(const HyperealVRController& other) = delete;
		HyperealVRController& operator=(const HyperealVRController& other) = delete;

		void CreateInputMapping();
		void PostInputEvent(const uint32 buttonIndex);

		struct ControllerState
		{
			ControllerState()
			{
			}

			ControllerState& operator=(const ControllerState& otherState)
			{
				if (this != &otherState)
				{
					memcpy(&buttonState, &otherState.buttonState, sizeof(buttonState));
				}

				return *this;
			}

			void Set(const AZ::VR::TrackingState& newTrackingState, const HyInputState& newButtonState)
			{
				trackingState = newTrackingState;
				memcpy(&buttonState, &newButtonState, sizeof(newButtonState));
			}

			inline bool IsButtonPressed(const HyButton& buttonId) const
			{
				return (buttonState.m_buttons & buttonId) != 0;
			}

			inline bool IsButtonTouched(const  HyButton&  buttonId) const
			{
				return (buttonState.m_buttons & buttonId) != 0;
			}

			AZ::VR::TrackingState trackingState;
			HyInputState buttonState;
		};

		HyDevice*& m_system; // Note: the system can be created *after* the device is created
		IInputDevice::TDevSpecIdToSymbolMap m_symbolMapping;
		int m_deviceIndex;
		uint8 m_deviceID;
		bool m_enabled;
		ControllerState m_currentState[static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers)];
		ControllerState m_previousState[static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers)];
		/*vr::TrackedDeviceIndex_t*/uint32_t m_contollerMapping[static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers)];
		IFFParams m_FFParams;
	};

} // namespace HyperealVR

