#include <cassert>

#include "../include/ggcprocessor.h"
#include "../include/plugids.h"
#include "../include/WavUtils.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"



namespace Steinberg {
namespace Vst {
namespace GgConvolver {

GgcProcessor::GgcProcessor ()
{
	// register its editor class
	setControllerClass (GgConvolverControllerUID);
}

tresult PLUGIN_API GgcProcessor::initialize (FUnknown* context)
{
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	if (result != kResultTrue)
	{
		return kResultFalse;
	}

	// Create Audio In/Out buses
	// Having stereo Input and a Stereo Output
	addAudioInput (STR16 ("AudioInput"), SpeakerArr::kStereo);
	addAudioOutput (STR16 ("AudioOutput"), SpeakerArr::kStereo);

	return kResultTrue;
}

tresult PLUGIN_API GgcProcessor::setBusArrangements (SpeakerArrangement* inputs,
                                                            int32 numIns,
                                                            SpeakerArrangement* outputs,
                                                            int32 numOuts)
{
	// we only support one in and output bus and these buses must have the same number of channels
	if (numIns == 1 && numOuts == 1 && inputs[0] == outputs[0])
	{
		return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
	}
	return kResultFalse;
}

// Called in disable state before setProcessing is called and processing will begin.
// In setup you get for example information about sampleRate, processMode, maximum number of samples per audio block
tresult PLUGIN_API GgcProcessor::setupProcessing (ProcessSetup& setup)
{
	// TODO:
	// 1)
	// - Should this be in setActive() instead? 
	// - Memory handling: can we free irBuffer memory?
	// - Test with other sample rate
	// - Implement re-sample of the IR file

	// 2)
	// - IR file name configurable

	mSampleRate = setup.sampleRate;

	const char* irFileName = "C:/Users/tobbe/source/my_vstplugins/ggconvolver/resource/IR_test_Celestion.wav";
	// audioRead reads into a float (32 bit)
	// We are using WDL_FFT_REAL to decide if we are built as a 32 or 64 bit plugin
	std::vector<float> irBuffer;
	int sampleRate;
	int numChannels;
	audioRead(irFileName, irBuffer, sampleRate, numChannels);
	size_t irFrames = irBuffer.size();
	// 
	// SetLength creates IR buffer 
	mImpulse.SetLength((int)irFrames); 
	// Load IR
	WDL_FFT_REAL* dest = mImpulse.impulses[0].Get();
	for (int i = 0; i < irFrames; ++i) {
		dest[i] = (WDL_FFT_REAL)irBuffer[i];
	}
	mImpulse.SetNumChannels(1);  // This is the default value

	// Perhaps not necessary. Clears out samples in convolution engine.
	mEngine.Reset();
	// Tie IR to convolution engine
	// SetImpulse(WDL_ImpulseBuffer *impulse, int fft_size=-1, int impulse_sample_offset=0, int max_imp_size=0, bool forceBrute=false);
	mEngine.SetImpulse(&mImpulse);

	return AudioEffect::setupProcessing (setup);
}

tresult PLUGIN_API GgcProcessor::setActive (TBool state)
{
	if (state) // Activate
	{
		//
	}
	else // Deactivate
	{
		// 
	}

	//  reset VU here?
	return AudioEffect::setActive (state);
}

tresult PLUGIN_API GgcProcessor::process(Vst::ProcessData& data)
{
	// Read input parameter changes

	if (data.inputParameterChanges)
	{
		int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
		for (int32 index = 0; index < numParamsChanged; index++)
		{
			Vst::IParamValueQueue* paramQueue =
				data.inputParameterChanges->getParameterData(index);
			if (paramQueue)
			{
				Vst::ParamValue value;
				int32 sampleOffset;
				int32 numPoints = paramQueue->getPointCount();
				switch (paramQueue->getParameterId())
				{	
					
					case GgConvolverParams::kParamLevelId:
						if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
							kResultTrue)
							mLevel = (float)value * 1.0f;  
						break;
					/*
					case TestPluginParams::kParamOnId:
						if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
							kResultTrue)
							mParam2 = value > 0 ? 1 : 0;
						break;
					*/
					case GgConvolverParams::kBypassId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) ==
							kResultTrue)
							mBypass = (value > 0.5f);
						break;
				}
			}
		}
	}

	// Process Audio

	if (data.numInputs == 0 || data.numOutputs == 0)
	{
		// nothing to do
		return kResultOk;
	}

	// - Assuming one input and one output audio bus
	// - Assuming that we have the same number of input and output channels

	int32 numChannels = data.inputs[0].numChannels;

	// Fetch audio buffers
	// processSetup set in AudioEffect::setupProcess(). This class is derived from AudioEffect

	// Get size of buffer in bytes
	uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples); // eg 128

	// Audio buffer is Sample32[][] or Sample64[][]  (float or double). An array of arrays of samples.
	// Typical buffer size 32 - 1024 samples
	// A sample is one float or double for each channel (array of Sample32 or Sample64)
	// Sample can also be called a frame

	void** inBuffer = getChannelBuffersPointer(processSetup, data.inputs[0]);
	void** outBuffer = getChannelBuffersPointer(processSetup, data.outputs[0]);

	// Check if input is marked as silent.

	if (data.inputs[0].silenceFlags != 0)
	{
		// mark output silence too
		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

		// If silent: clear the output buffer

		for (int32 i = 0; i < numChannels; i++)
		{
			// do not need to be cleared if the buffers are the same (in this case input buffer are
			// already cleared by the host)
			if (inBuffer[i] != outBuffer[i])
			{
				memset(outBuffer[i], 0, sampleFramesSize);
			}
		}

		return kResultOk;
	}

	// Mark outputs not silent
	data.outputs[0].silenceFlags = 0;

	if (data.numSamples > 0)
	{
		// If bypass, copy input buffer to output buffer
		if (mBypass)
		{
			for (int32 i = 0; i < numChannels; i++)
			{
				// do not need to be copied if the buffers are the same
				if (inBuffer[i] != outBuffer[i])
				{
					memcpy(outBuffer[i], inBuffer[i], sampleFramesSize);
				}
			}
		}
		else {
			float vuPPM = 0.f;

			if (mLevel < 0.0000001)
			{
				// If very low input, clear output buffer and mark output as silent
				for (int32 i = 0; i < numChannels; i++)
				{
					memset(outBuffer[i], 0, sampleFramesSize);
				}
				// Set 1 to all channels. FIXME: the Arithmetic overflow  warning
				data.outputs[0].silenceFlags = (1 << numChannels) - 1;
			}
			else {
				// Process the data
				int32 samples = data.numSamples;
				mEngine.Add((WDL_FFT_REAL**)inBuffer, samples, numChannels);

				// Note: Available samples from convolver may be less than input samples
				// according to convoengine.h. Not sure why or what the result will be.

				int blocksInConvoBuffer = min(mEngine.Avail(samples), samples);
				
				for (int32 i = 0; i < numChannels; i++)
				{
					int32 sampleCounter = data.numSamples;
 					WDL_FFT_REAL* ptrOut = (WDL_FFT_REAL*)outBuffer[i];
					WDL_FFT_REAL* ptrConvolved = (WDL_FFT_REAL*)mEngine.Get()[i];
					WDL_FFT_REAL tmp;
					while (--sampleCounter >= 0)
					{
						// apply gain
						tmp = (*ptrConvolved++) * mLevel;
						(*ptrOut++) = tmp;
						if (tmp > vuPPM)
						{
							vuPPM = tmp;
						}
					}				
				}				
				mEngine.Advance(blocksInConvoBuffer);

			}

			// Write output parameter changes (VU)

			IParameterChanges* outParamChanges = data.outputParameterChanges;
			if (outParamChanges && mVuPPMOld != vuPPM)
			{
				int32 index = 0;
				IParamValueQueue* paramQueue = outParamChanges->addParameterData(kVuPPMId, index);
				if (paramQueue)
				{
					int32 index2 = 0;
					paramQueue->addPoint(0, vuPPM, index2);
				}
			}
			mVuPPMOld = vuPPM;
		}
	}
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API GgcProcessor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	// called when we load a preset or project, the model has to be reloaded

	IBStreamer streamer (state, kLittleEndian);
	
	float savedLevel = 0.f;
	if (streamer.readFloat (savedLevel) == false)
		return kResultFalse;
	/*
	int32 savedParam2 = 0;
	if (streamer.readInt32 (savedParam2) == false)
		return kResultFalse;
	*/
	int32 savedBypass = 0;
	if (streamer.readInt32 (savedBypass) == false)
		return kResultFalse;

	mLevel = savedLevel;
	//mParam2 = savedParam2 > 0 ? 1 : 0;
	mBypass = savedBypass > 0;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API GgcProcessor::getState (IBStream* state)
{
	// here we need to save the model (preset or project)

	float toSaveLevel = mLevel;
	//int32 toSaveParam2 = mParam2;
	int32 toSaveBypass = mBypass ? 1 : 0;

	IBStreamer streamer (state, kLittleEndian);
	streamer.writeFloat (toSaveLevel);
	//streamer.writeInt32 (toSaveParam2);
	streamer.writeInt32 (toSaveBypass);

	return kResultOk;
}

//------------------------------------------------------------------------
} 
} 
}
