// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnderscoreSection.h"

bool FUnderscoreQuantizationRules::GetNextTriggerPoint(const FUnderscoreTransport& InTransport, FUnderscoreTransport& OutstartTime) const
{
	int32 SectionLengthInBeats = InTransport.ToBeats() + (InTransport.WrapLength + 1) * InTransport.TimeSignature.NumBeats;
	FUnderscoreTransport TransportIt = InTransport; 

	for (int32 BeatsChecked = 0; BeatsChecked < SectionLengthInBeats; ++BeatsChecked)
	{
		TransportIt.Increment();

		if (IsValidTriggerPoint(TransportIt))
		{
			OutstartTime = TransportIt;
			return true;
		}
	}

	return false;
}

bool FUnderscoreQuantizationRules::IsValidTriggerPoint(const FUnderscoreTransport& InTransport) const
{
	if (Quantization == EUnderscoreStingerQuantization::Beat)
	{
		if (bRestrictBeatSyncPoints && AllowedBeats.Contains(InTransport.Beat) == false)
		{
			return false;
		}
	}
	else if (InTransport.Beat != 1)
	{
		return false;
	}

	if (bRestrictBarSyncPoints && AllowedBars.Contains(InTransport.Bar) == false)
	{
		return false;
	}

	return true;
}

void FUnderscoreTransport::Wrap()
{
	if (WrapLength <= 0 || TimeSignature.NumBeats <= 0)
	{
		return;
	}

	while (Beat > TimeSignature.NumBeats)
	{
		Beat -= TimeSignature.NumBeats;
		Bar++;
	}

	while (Beat < 1)
	{
		Beat += TimeSignature.NumBeats;
		Bar--;
	}

	while (Bar > WrapLength)
	{
		Bar -= WrapLength;
	}

	while (Bar < 1)
	{
		Bar += WrapLength;
	}
}

void FUnderscoreTransport::Increment()
{
	++Beat;
	Wrap();
}

int32 FUnderscoreTransport::ToBeats() const
{
	return TimeSignature.NumBeats * (Bar) + Beat;
}

FUnderscoreTransport FUnderscoreTransport::FromBeats(const int32 InNumBeats, const int32 InWrapLength, const FQuartzTimeSignature& InTimeSignature)
{
	if (InTimeSignature.NumBeats == 0)
	{
		return FUnderscoreTransport();
	}

	FUnderscoreTransport OutTransport = {
		(InNumBeats / InTimeSignature.NumBeats),
		(InNumBeats % InTimeSignature.NumBeats),
		InWrapLength,
		InTimeSignature };

	OutTransport.Wrap();

	return OutTransport;
}
