// Simple convolver VST3 plugin 
// Processor part

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

namespace Steinberg {
namespace Vst {
namespace GgConvolver {

class GgcProcessor : public Vst::AudioEffect
{
public:
	GgcProcessor();

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs,
		int32 numIns,
		SpeakerArrangement* outputs,
		int32 numOuts) SMTG_OVERRIDE;
	tresult PLUGIN_API setupProcessing(ProcessSetup& setup) SMTG_OVERRIDE;
	tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;
	tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE;
	tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;
	// Create new instances of this processor. Required by plugin factory
	static FUnknown* createInstance(void*)
	{
		return (IAudioProcessor*)new GgcProcessor();
	}

protected:
	float mLevel = 1.f;
	bool mBypass = false; 
	SampleRate mSampleRate = 44100.f;
	float mVuPPMOld = 0.f;
};
}
}
} 

