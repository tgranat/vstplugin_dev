// Simple convolver VST3 plugin 
// Processor part

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "../WDL/convoengine.h"
#include "../WDL/resample.h"

namespace Steinberg {
namespace Vst {
namespace GgConvolver {

// Processor part
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

#if WDL_FFT_REALSIZE == 8
	tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE;
#endif

protected:
	WDL_ImpulseBuffer mImpulse;
	WDL_ConvolutionEngine_Div mEngine;
	float mLevel = 0.5f;
	float mPregain = 0.5f;
	bool mBypass = false; 
	SampleRate mIncomingAudioSampleRate = 48000.f;
	float mVuLevelOld = 0.f;
	float mVuPregainOld = 0.f;


private:
	void initiateConvolutionEngine(const std::vector<float> impulseResponse, SampleRate irSampleRate);

	template <class I, class O>
		void resample(const I* source, int sourceLength, double sourceSampleRate, O* target, double targetSampleRate);

	static const std::vector<float> mCelestian_v30_48kHz_1ch_200ms;
	static const std::vector<float> m412_sm57_off_axis_44100Hz_1ch;

	static const int mResamplerBlockLength = 64;
};
}
}
} 

