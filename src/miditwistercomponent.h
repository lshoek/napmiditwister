/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Nap includes
#include <midiinputcomponent.h>
#include <parameternumeric.h>
#include <nap/resourceptr.h>

namespace nap
{
	class MidiTwisterComponentInstance;

	// Midi Encoder Types
	enum class EMidiTwisterEncoderType
	{
		Absolute = 0,
		Relative = 1
	};

	// Midi Twister Channels
	enum class EMidiTwisterChannel
	{
		Encoder = 0,
		EncoderButton = 1,
		SideButton = 3
	};

	// Four banks of sixteen encoders each
	struct MidiTwisterEncoder
	{
		ResourcePtr<ParameterFloat> mParameter;
		EMidiTwisterEncoderType mEncoderType = EMidiTwisterEncoderType::Absolute;
		float mEncoderStepSize = 0.01f;
	};

	// Four banks of sixteen encoders each
	struct MidiTwisterBank
	{
		// A bank comprises sixteen encoders
		static constexpr size_t BANKSIZE = 16;
		std::array<MidiTwisterEncoder, BANKSIZE> mEncoders;
	};

	/**
	 * Component that maps Midi Fighter Twister signals to parameters
	 */
	class NAPAPI MidiTwisterComponent : public MidiInputComponent
	{
		RTTI_ENABLE(MidiInputComponent)
			DECLARE_COMPONENT(MidiTwisterComponent, MidiTwisterComponentInstance)

	public:
		MidiTwisterComponent() :
			MidiInputComponent() { }

		std::vector<MidiTwisterBank> mBanks;
		bool mEnable = true;
	};


	/**
	 * Instance of component that maps Midi Fighter Twister signals to parameters.
	 */
	class NAPAPI MidiTwisterComponentInstance : public MidiInputComponentInstance
	{
		RTTI_ENABLE(MidiInputComponentInstance)
			friend class MidiService;

	public:
		MidiTwisterComponentInstance(EntityInstance& entity, Component& resource) :
			MidiInputComponentInstance(entity, resource) { }

		// Initialize the component
		bool init(utility::ErrorState& errorState) override;

		// Destructor
		virtual ~MidiTwisterComponentInstance() { }

	private:
		MidiTwisterComponent* mResource = nullptr;

		/**
			*
			*/
		void onMessageReceived(const MidiEvent& event);
		Slot<const MidiEvent&> messageReceivedSlot = { this, &MidiTwisterComponentInstance::onMessageReceived };

		const uint mMaxBanks = 4;
	};
}
