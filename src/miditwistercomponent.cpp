/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "miditwistercomponent.h"

// Nap includes
#include <entity.h>
#include <nap/core.h>

// Midi includes
#include "midiservice.h"

RTTI_BEGIN_ENUM(nap::EMidiTwisterEncoderType)
	RTTI_ENUM_VALUE(nap::EMidiTwisterEncoderType::Absolute, "Absolute"),
	RTTI_ENUM_VALUE(nap::EMidiTwisterEncoderType::Relative, "Relative")
RTTI_END_ENUM

RTTI_BEGIN_STRUCT(nap::MidiTwisterEncoder)
	RTTI_PROPERTY("Parameter", &nap::MidiTwisterEncoder::mParameter, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("EncoderType", &nap::MidiTwisterEncoder::mEncoderType, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("EncoderStepSize", &nap::MidiTwisterEncoder::mEncoderStepSize, nap::rtti::EPropertyMetaData::Default)
RTTI_END_STRUCT

RTTI_BEGIN_STRUCT(nap::MidiTwisterBank)
	RTTI_PROPERTY("Encoders", &nap::MidiTwisterBank::mEncoders, nap::rtti::EPropertyMetaData::Default)
RTTI_END_STRUCT

RTTI_BEGIN_CLASS(nap::MidiTwisterComponent)
	RTTI_PROPERTY("Banks", &nap::MidiTwisterComponent::mBanks, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::MidiTwisterComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{

	bool MidiTwisterComponentInstance::init(utility::ErrorState& errorState)
	{
		mResource = getComponent<MidiTwisterComponent>();
		if (!errorState.check(mResource->mBanks.size() <= mMaxBanks, "The MidiTwisterComponents supports up to %d banks", mMaxBanks))
			return false;

		if (!MidiInputComponentInstance::init(errorState))
			return false;

		messageReceived.connect(messageReceivedSlot);

		return true;
	}


	void MidiTwisterComponentInstance::onMessageReceived(const MidiEvent& event)
	{
		if (event.getNumber() >= mResource->mBanks.size() * MidiTwisterBank::BANKSIZE)
			return;

		const uint bank_index = event.getNumber() / MidiTwisterBank::BANKSIZE;
		if (bank_index >= mResource->mBanks.size())
			return;

		MidiTwisterBank& bank = mResource->mBanks[bank_index];
		const uint encoder_index = event.getNumber() % MidiTwisterBank::BANKSIZE;

		MidiTwisterEncoder& enc = bank.mEncoders[encoder_index];
		auto& param = enc.mParameter;

		if (param != nullptr)
		{
			EMidiTwisterChannel channel = static_cast<EMidiTwisterChannel>(event.getChannel());
			switch (channel)
			{
				case EMidiTwisterChannel::Encoder:
				{
					EMidiTwisterEncoderType enc_type = static_cast<EMidiTwisterEncoderType>(enc.mEncoderType);
					switch (enc_type)
					{
						case EMidiTwisterEncoderType::Absolute:
						{
							float normal = event.getValue() / 127.0f;
							float value = param->mMinimum + normal * (param->mMaximum - param->mMinimum);
							param->setValue(value);
							break;
						}
						case EMidiTwisterEncoderType::Relative:
						{
							// 63 = anticlockwise, 65 = clockwise
							bool clockwise = event.getValue() > 64.0f;
							param->setValue(param->mValue + (clockwise ? enc.mEncoderStepSize : -enc.mEncoderStepSize));
							break;
						}
					}
					break;
				}
				case EMidiTwisterChannel::EncoderButton:
				{
					param->setValue(param->mMinimum + 0.5f * (param->mMaximum - param->mMinimum));
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}
}
