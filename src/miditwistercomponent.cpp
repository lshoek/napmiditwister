/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "miditwistercomponent.h"

// Nap includes
#include <entity.h>
#include <nap/core.h>
#include <nap/logger.h>

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
	/**
	 * Supported parameter types
	 */
	static std::vector<rtti::TypeInfo> sSupportedParameterTypes
	{
		RTTI_OF(ParameterFloat),
		RTTI_OF(ParameterInt),
		RTTI_OF(ParameterBool)
	};


	static bool isSupported(rtti::TypeInfo inType)
	{
		for (auto t : sSupportedParameterTypes)
			if (inType.is_derived_from(t))
				return true;
		return false;
	}


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
		if (enc.mParameter != nullptr)
		{
			if (!isSupported(enc.mParameter.get()->get_type()))
			{
				nap::Logger::warn("%s: Unsupported parameter type", mResource->mID.c_str());
				return;
			}

			EMidiTwisterChannel channel = static_cast<EMidiTwisterChannel>(event.getChannel());
			switch (channel)
			{
				case EMidiTwisterChannel::Twist:
				{
					EMidiTwisterEncoderType enc_type = static_cast<EMidiTwisterEncoderType>(enc.mEncoderType);
					if (enc.mParameter.get()->get_type().is_derived_from(RTTI_OF(ParameterFloat)))
					{
						auto* param = static_cast<ParameterFloat*>(enc.mParameter.get());
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
					else if (enc.mParameter.get()->get_type().is_derived_from(RTTI_OF(ParameterInt)))
					{
						auto* param = static_cast<ParameterInt*>(enc.mParameter.get());
						bool clockwise = event.getValue() > 64.0f;
						int value = std::clamp(param->mValue + (clockwise ? 1 : -1), param->mMinimum, param->mMaximum);
						param->setValue(value);
						break;
					}
				}
				case EMidiTwisterChannel::Push:
				{
					if (enc.mParameter.get()->get_type().is_derived_from(RTTI_OF(ParameterFloat)))
					{
						// No action
					}
					else if (enc.mParameter.get()->get_type().is_derived_from(RTTI_OF(ParameterInt)))
					{
						// No action
					}
					else if (enc.mParameter.get()->get_type().is_derived_from(RTTI_OF(ParameterBool)))
					{
						if (event.getValue() >= 127.0f)
						{
							auto* param = static_cast<ParameterBool*>(enc.mParameter.get());
							param->setValue(!param->getValue());
						}
					}
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
