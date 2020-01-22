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

const std::vector<float> GgcProcessor::mCelestian_v30_48kHz_1ch_200ms =
{
	#include "../resource/celestion_v30_48kHz_200ms.data"
};

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

tresult PLUGIN_API GgcProcessor::setActive (TBool state)
{
	// TODO:

// - Implement re-sample of the IR file.
// - IR file name configurable. Or something
	// How can IR files be loaded dynamically?

	if (state) // Activate
	{
		initiateConvolutionEngine();


		/*
		//const char* irFileName = "C:/Users/tobbe/source/my_vstplugins/ggconvolver/resource/Studio_Nord_Plate_3sec.wav";
		const char* irFileName = "C:/Users/tobbe/source/my_vstplugins/ggconvolver/resource/IR_test_Celestion_48kHz_200ms.wav";
		//	const char* irFileName = "C:/Users/tobbe/source/my_vstplugins/ggconvolver/resource/IR_test_Celestion_96kHz_500ms.wav";
		// audioRead reads into a float (32 bit)
		// We are using WDL_FFT_REAL to decide if we are built as a 32 or 64 bit plugin since we have dependencies to WDL convolver
		std::vector<float> irBuffer;
		int sampleRateIRFile;
		int numChannels;

		audioRead(irFileName, irBuffer, sampleRateIRFile, numChannels);
		size_t irFrames = irBuffer.size();

		if (mIncomingAudioSampleRate != sampleRateIRFile) {
			// Resample IR
		}
		// 
		// SetLength creates IR buffer 
		mImpulse.SetLength((int)irFrames);
		// Load IR
		WDL_FFT_REAL* dest = mImpulse.impulses[0].Get();
		for (int i = 0; i < irFrames; ++i) {
			dest[i] = (WDL_FFT_REAL)irBuffer[i];
		}
		mImpulse.SetNumChannels(1);  // Not necessary, this is the default value

		// Perhaps not necessary. Clears out samples in convolution engine.
		mEngine.Reset();
		// Tie IR to convolution engine
		// SetImpulse(WDL_ImpulseBuffer *impulse, int fft_size=-1, int impulse_sample_offset=0, int max_imp_size=0, bool forceBrute=false);
		mEngine.SetImpulse(&mImpulse);
		*/
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
					case GgConvolverParams::kParamLevelId:
						if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
							kResultTrue)
							mLevel = (float)value * 2.0f;  
						break;

					case GgConvolverParams::kParamPregainId:
						// initiateConvolutionEngine(); // TEST init IR in process()
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) ==
							kResultTrue)
							mPregain = (float)value * 2.0f;
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

				int blocksInConvoBuffer = min(mEngine.Avail(samples), samples);
				
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

						// apply gain

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

	// called when we load a preset or project, the model has to be reloaded

	IBStreamer streamer (state, kLittleEndian);
	
	float savedLevel = 0.f;
	if (streamer.readFloat (savedLevel) == false)
		return kResultFalse;

	float savedPregain = 0.f;
	if (streamer.readFloat(savedPregain) == false)
		return kResultFalse;

	int32 savedBypass = 0;
	if (streamer.readInt32 (savedBypass) == false)
		return kResultFalse;

	mLevel = savedLevel;
	mPregain = savedPregain;
	mBypass = savedBypass > 0;

	return kResultOk;
}

tresult PLUGIN_API GgcProcessor::getState (IBStream* state)
{
	// here we need to save the model (preset or project)

	float toSaveLevel = mLevel;
	float toSavePregain = mPregain;
	int32 toSaveBypass = mBypass ? 1 : 0;

	IBStreamer streamer (state, kLittleEndian);

	streamer.writeFloat (toSaveLevel);
	streamer.writeFloat(toSavePregain);
	streamer.writeInt32 (toSaveBypass);

	return kResultOk;
}

void GgcProcessor::initiateConvolutionEngine()
{
	// if IR is reloaded, is it enough to just update the buffer in mImpulse? It should be

	// This part is if you read from file

	// const char* irFileName = "C:/Users/tobbe/source/my_vstplugins/ggconvolver/resource/IR_test_Celestion_48kHz_200ms.wav";
	// audioRead reads into a float (32 bit)
	// We are using WDL_FFT_REAL to decide if we are built as a 32 or 64 bit plugin since we have dependencies to WDL convolver
	
	// Assuming only one channel in IR file
	
	//std::vector<float> irBuffer;
	//int sampleRateIRFile;
	//int numChannels;

	//audioRead(irFileName, irBuffer, sampleRateIRFile, numChannels);
	//size_t irFrames = irBuffer.size();

	//if (mIncomingAudioSampleRate != sampleRateIRFile) {
		// Resample IR
	//}

	// End of out-commented read file part
	
	// SetLength creates IR buffer 
	int irFrames = mCelestian_v30_48kHz_1ch_200ms.size();
	mImpulse.SetLength((int)irFrames);
	// Load IR
	WDL_FFT_REAL* dest = mImpulse.impulses[0].Get();
	for (int i = 0; i < irFrames; ++i) {
		dest[i] = (WDL_FFT_REAL)mCelestian_v30_48kHz_1ch_200ms[i];
	}
	mImpulse.SetNumChannels(1);  // Not necessary, this is the default value

	// Perhaps not necessary. Clears out samples in convolution engine.
	mEngine.Reset();
	// Tie IR to convolution engine
	// SetImpulse(WDL_ImpulseBuffer *impulse, int fft_size=-1, int impulse_sample_offset=0, int max_imp_size=0, bool forceBrute=false);
	mEngine.SetImpulse(&mImpulse);
}


} 
} 
}
