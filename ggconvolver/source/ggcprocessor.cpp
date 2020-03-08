// Simple convolver VST3 plugin 
// Processor part

// Using the WDL convolver engine https://www.cockos.com/wdl/
// To build for 32 bit, update fft.h and set WDL_FFT_REALSIZE to 4
// To build for 64 bit, update fft.h and set WDL_FFT_REALSIZE to 8

#include <cassert>
#include <fstream> 

#include "../include/ggcprocessor.h"
#include "../include/plugids.h"
// #include "../include/WavUtils.h"  // only used if we read IR from file

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"


namespace Steinberg {
namespace Vst {
namespace GgConvolver {



const std::vector<float> GgcProcessor::m412_sm57_on_axis1_44100Hz_1ch =
{
	#include "../resource/412_sm57_on_axis1_44100Hz.data"
};

const std::vector<float> GgcProcessor::m412_sm57_on_axis2_44100Hz_1ch =
{
	#include "../resource/412_sm57_on_axis2_44100Hz.data"
};

const std::vector<float> GgcProcessor::m412_sm57_off_axis_44100Hz_1ch =
{
	#include "../resource/412_sm57_off_axis_44100Hz.data"
};

const std::vector<float> GgcProcessor::m210combo_center_U906_44100Hz_1ch =
{
	#include "../resource/210combo_center_U906_44100Hz.data"
};

// Update if IRs are added
const int32 GgcProcessor::mNbrOfImpulseResponses = 4;


ImpulseResponse::ImpulseResponse(std::string desc, SampleRate sampleRate, const std::vector<float> &irData)
{
	mDesc = desc;
	mSampleRate = sampleRate;
	mIrData = irData; 
}

std::string ImpulseResponse::getDesc() {
	return mDesc;
}

SampleRate ImpulseResponse::getSampleRate() {
	return mSampleRate;
}

float* ImpulseResponse::getImpulseResponse() {
	return mIrData.data();
}

int ImpulseResponse::getIrLength() {
	return mIrData.size();
}

GgcProcessor::GgcProcessor ()
{
	mImpulseResponses.push_back(ImpulseResponse("412_sm57_off_axis", 44100.f, m412_sm57_off_axis_44100Hz_1ch));
	mImpulseResponses.push_back(ImpulseResponse("412_sm57_on_axis_1", 44100.f, m412_sm57_on_axis1_44100Hz_1ch));
	mImpulseResponses.push_back(ImpulseResponse("412_sm57_on_axis_2", 44100.f, m412_sm57_on_axis2_44100Hz_1ch));
	mImpulseResponses.push_back(ImpulseResponse("210combo_on_axis_U906", 44100.f, m210combo_center_U906_44100Hz_1ch));

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

// Tell VST host that we handle 64 bit (otherwise, we dont't answer)

#if WDL_FFT_REALSIZE == 8
tresult PLUGIN_API GgcProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
	return kResultTrue;
}
#endif

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
	mIncomingAudioSampleRate = setup.sampleRate;

	return AudioEffect::setupProcessing (setup);
}

// Depends on the VST host, but Reaper does:
// Add plugin:  
//              setupProcessing()   (and some other init and setup before that)
//              setActive(1)        (1 = activate)
//              setActive(0)
//              setActive(1)
// After every time paused:
//              setActive(0)
//              setActive(1)
//              process() .... 
// Remove plugin:
//              setActive(0)     

tresult PLUGIN_API GgcProcessor::setActive (TBool state)
{

	if (state) // Activate
	{
		mCurrentImpulseResponse = 0;
		initiateConvolutionEngine(mImpulseResponses[0].getImpulseResponse(), mImpulseResponses[0].getSampleRate(), mImpulseResponses[0].getIrLength());
	}
	else // Deactivate
	{
		//
	}

	mVuLevelOld = 0.f;

	return AudioEffect::setActive (state);
}

tresult PLUGIN_API GgcProcessor::process(Vst::ProcessData& data)
{
	// For each processing block the host should provide information about its state, for example sample rate
	// ProcessContect data.processContext 

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
					case GgConvolverParams::kBypassId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) ==
							kResultTrue)
							mBypass = (value > 0.5f);
						break;

					case GgConvolverParams::kParamImpulseResponse:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) ==
							kResultTrue)
							mImpulseResponse = int32(value * ((ParamValue)mNbrOfImpulseResponses - 1) + 0.5);
						break;

					case GgConvolverParams::kParamLevelId:
						if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
							kResultTrue)
							mLevel = (float)value;  
						break;

					case GgConvolverParams::kParamPregainId:

						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) ==
							kResultTrue)
							mPregain = (float)value;
						break;	
				}
			}
		}
	}

	// Process Audio

	// Check if called without buffers (numInputs and numOutputs are zeroed), in order to flush parameters.
	// Parameters flush could happen only when the host needs to send parameter changes and no processing is called. 
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
	uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);

	// Audio buffer is Sample32[][] or Sample64[][] (float or double). An array of arrays of samples.
	// Typical buffer size 16 - 1024 samples (also called frames).

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
			float vuLevel = 0.f;
			float vuPregain = 0.f;

			if (mLevel < 0.0000001 || mPregain < 0.0000001)
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
				// If IR has changed, re-initiate convolution engine
				if (mCurrentImpulseResponse != mImpulseResponse) {
					initiateConvolutionEngine(mImpulseResponses[mImpulseResponse].getImpulseResponse(),
						mImpulseResponses[mImpulseResponse].getSampleRate(),
						mImpulseResponses[mImpulseResponse].getIrLength());
					mCurrentImpulseResponse = mImpulseResponse;
				}
				int32 samples = data.numSamples;
				// Pre-gain adjust. Using output buffer as temporary buffer
				for (int32 i = 0; i < numChannels; i++)
				{
					int32 sampleCounter = data.numSamples;
					WDL_FFT_REAL* ptrIn = (WDL_FFT_REAL*)inBuffer[i];
					WDL_FFT_REAL* ptrOut = (WDL_FFT_REAL*)outBuffer[i];
					WDL_FFT_REAL tmp;
					while (--sampleCounter >= 0)
					{
						// apply pre-gain
						tmp = (*ptrIn++) * mPregain;
						(*ptrOut++) = tmp;
					}
				}
				// outBuffer is buffer adjusted with pre-gain
				mEngine.Add((WDL_FFT_REAL**)outBuffer, samples, numChannels);

				// Note: Available samples from convolver may be less than input samples
				// according to convoengine.h. Not sure why or what the result will be.

				// By making type explicit <int> the preprocessor on stops matching to the windows 'min' macro.
				// Otherwise we get an error here on Windows. std::min necessary on unix
				int blocksInConvoBuffer = std::min<int>(mEngine.Avail(samples), samples);
				
				for (int32 i = 0; i < numChannels; i++)
				{
					int32 sampleCounter = data.numSamples;
 					WDL_FFT_REAL* ptrOut = (WDL_FFT_REAL*)outBuffer[i];
					WDL_FFT_REAL* ptrConvolved = (WDL_FFT_REAL*)mEngine.Get()[i];
					WDL_FFT_REAL tmp;
					while (--sampleCounter >= 0)
					{
						// VU meter after input+IR but before level
						tmp = *ptrConvolved;
						if (tmp > vuPregain)
						{
							vuPregain = tmp;
						}

						// apply out level

						tmp = (*ptrConvolved++) * mLevel;
						(*ptrOut++) = tmp;
						// VU meter after level
						if (tmp > vuLevel)
						{
							vuLevel = tmp;
						}
					}				
				}				
				mEngine.Advance(blocksInConvoBuffer);
			}

			// Write output parameter changes (VU)

			IParameterChanges* outParamChanges = data.outputParameterChanges;
			if (outParamChanges && mVuLevelOld != vuLevel)
			{
				int32 index = 0;
				IParamValueQueue* paramQueue = outParamChanges->addParameterData(kVuLevelId, index);
				if (paramQueue)
				{
					int32 index2 = 0;
					paramQueue->addPoint(0, vuLevel, index2);
				}
			}
			if (outParamChanges && mVuPregainOld != vuPregain)
			{
				int32 index = 0;
				IParamValueQueue* paramQueue = outParamChanges->addParameterData(kVuPregainId, index);
				if (paramQueue)
				{
					int32 index2 = 0;
					paramQueue->addPoint(0, vuPregain, index2);
				}
			}
			mVuLevelOld = vuLevel;
			mVuPregainOld = vuPregain;
		}
	}
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API GgcProcessor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	// Called when we load a preset or project, the model has to be reloaded
	// Has to be in the correct order! See implemantation of setComponentState() in controller part.

	IBStreamer streamer (state, kLittleEndian);
	
	int32 savedBypass = 0;
	if (streamer.readInt32(savedBypass) == false)
		return kResultFalse;

	int32 savedImpulseResponse = 0;
	if (streamer.readInt32(savedImpulseResponse) == false)
		return kResultFalse;

	float savedLevel = 0.f;
	if (streamer.readFloat (savedLevel) == false)
		return kResultFalse;

	float savedPregain = 0.f;
	if (streamer.readFloat(savedPregain) == false)
		return kResultFalse;

	mImpulseResponse = savedImpulseResponse;
	mLevel = savedLevel;
	mPregain = savedPregain;
	mBypass = savedBypass > 0;

	return kResultOk;
}

tresult PLUGIN_API GgcProcessor::getState (IBStream* state)
{
	// Save the model (preset or project)

	float toSaveLevel = mLevel;
	float toSavePregain = mPregain;
	int32 toSaveBypass = mBypass ? 1 : 0;
	int32 toSaveImpulseResponse = mImpulseResponse;

	IBStreamer streamer (state, kLittleEndian);

	// Has to be in the correct order! See implemantation of setComponentState() in controller part.
	streamer.writeInt32(toSaveBypass);
	streamer.writeInt32(toSaveImpulseResponse);
	streamer.writeFloat (toSaveLevel);
	streamer.writeFloat(toSavePregain);

	return kResultOk;
}

// Resamples (unless sample rates are equal) using linear interpolation
template <class I, class O>
void GgcProcessor::resample(const I* source, int sourceLength, double sourceSampleRate, O* target, double targetSampleRate)
{
	if (sourceSampleRate != targetSampleRate) {
		// Resample using linear interpolation.
		double pos = 0.;
		double delta = sourceSampleRate / targetSampleRate;
		int targetLength = targetSampleRate / sourceSampleRate * (double)sourceLength + 0.5;
		for (int i = 0; i < targetLength; ++i)
		{
			int idx = int(pos);
			if (idx < sourceLength)
			{
				double frac = pos - floor(pos);
				double interp = (1. - frac) * source[idx];
				if (++idx < sourceLength) {
					interp += frac * source[idx];
				}
				pos += delta;
				*target++ = (O)(delta * interp);
			}
			else
			{
				*target++ = 0;
			}
		}
	}
	else {
		// No need for resample
		for (int i = 0; i < sourceLength; ++i) {
			target[i] = (O)source[i];
		}
	}
}

// Initiate the convolution engine:
//   Create a WDL_ImpulseBuffer and load a (resampled if needed) impulse response
//   Create a WDL_ConvolutionEngine_Div

void GgcProcessor::initiateConvolutionEngine(float* impulseResponse, SampleRate irSampleRate, int irLength)
{
	// Initiate buffer where impulse response will be stored
	mImpulse.SetNumChannels(1);  // Not necessary, this is the default value
	// SetLength() initiates the buffer
	int destLength = mIncomingAudioSampleRate / irSampleRate * (double)irLength + 0.5;
	mImpulse.SetLength((int)destLength);

	// Get pointer to buffer where impulse response will be stored
	WDL_FFT_REAL* target = mImpulse.impulses[0].Get();

	// Write impulse response to buffer. If different sample rates, data will be resampled first
	resample(impulseResponse, irLength, irSampleRate, target, mIncomingAudioSampleRate);

	// Perhaps not necessary at this point. Clears out samples in convolution engine.
	mEngine.Reset();
	// Tie IR to convolution engine
	mEngine.SetImpulse(&mImpulse);
}

} 
} 
}
