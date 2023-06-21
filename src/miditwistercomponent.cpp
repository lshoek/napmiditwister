/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "miditwistercomponent.h"

// Nap includes
#include <entity.h>
#include <nap/core.h>

// Midi includes
#include "midiservice.h"

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
		if (!errorState.check(mResource->mBanks.size() <= mMaxBanks, "The MidiTwisterComponents supprots up to %d banks", mMaxBanks))
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

		auto& bank = mResource->mBanks[bank_index];
		const uint encoder_index = event.getNumber() % MidiTwisterBank::BANKSIZE;
		auto& param = bank.mEncoders[encoder_index];

		if (param != nullptr)
		{
			EMidiTwisterChannel channel = static_cast<EMidiTwisterChannel>(event.getChannel());
			switch (channel)
			{
				case EMidiTwisterChannel::Encoder:
				{
					const float normal = event.getValue() / 127.0f;
					param->setValue(param->mMinimum + normal * (param->mMaximum - param->mMinimum));
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
