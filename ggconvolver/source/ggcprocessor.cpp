#include "../include/ggcprocessor.h"
#include "../include/plugids.h"

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
	// TODO: use samplerate if we need to re-sample the IR file
	mSampleRate = setup.sampleRate;
	return AudioEffect::setupProcessing (setup);
}

tresult PLUGIN_API GgcProcessor::setActive (TBool state)
{
	if (state) // Activate
	{
		// Allocate Memory Here
		// Ex: algo.create ();
	}
	else // Deactivate
	{
		// Free Memory if still allocated
		// Ex: if(algo.isCreated ()) { algo.destroy (); }
	}

		return AudioEffect::setActive (state);
}

tresult PLUGIN_API GgcProcessor::process(Vst::ProcessData& data)
{
	//--- Read inputs parameter changes-----------
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
						if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
							mLevel = (float)value * 2.0;  
						break;
					/*
					case TestPluginParams::kParamOnId:
						if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
							kResultTrue)
							mParam2 = value > 0 ? 1 : 0;
						break;
					*/
					case GgConvolverParams::kBypassId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
							mBypass = (value > 0.5f);
						break;
				}
			}
		}
	}

	//--- Process Audio---------------------
	//--- ----------------------------------
	if (data.numInputs == 0 || data.numOutputs == 0)
	{
		// nothing to do
		return kResultOk;
	}

	// (simplification) we suppose in this example that we have the same input channel count than
	// the output
	int32 numChannels = data.inputs[0].numChannels;

	//---get audio buffers----------------

	// processSetup set in AudioEffect::setupProcess(). This class is derived from AudioEffect

	// Get size of buffer in bytes
	uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);

	// Audio buffer is Sample32[][] or Sample64[][]  (float or double). An array of arrays of samples.
	// Typical buffer size 32 - 1024 samples
	// A sample is one float or double for each channel (array of Sample32 or Sample64)
	// Sample sometimes called a frame
	// 

	void** inBuffer = getChannelBuffersPointer(processSetup, data.inputs[0]);
	void** outBuffer = getChannelBuffersPointer(processSetup, data.outputs[0]);

	//---check if silence---------------
// normally we have to check each channel (simplification)
	if (data.inputs[0].silenceFlags != 0)
	{
		// mark output silence too
		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

		// the Plug-in has to be sure that if it sets the flags silence that the output buffer are
		// clear
		for (int32 i = 0; i < numChannels; i++)
		{
			// do not need to be cleared if the buffers are the same (in this case input buffer are
			// already cleared by the host)
			if (inBuffer[i] != outBuffer[i])
			{
				memset(outBuffer[i], 0, sampleFramesSize);
			}
		}

		// nothing to do at this point
		return kResultOk;
	}

	// mark our outputs has not silent
	data.outputs[0].silenceFlags = 0;

	if (data.numSamples > 0)
	{

		//---in bypass mode outputs should be like inputs-----
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
				for (int32 i = 0; i < numChannels; i++)
				{
					memset(outBuffer[i], 0, sampleFramesSize);
				}
				data.outputs[0].silenceFlags =
					(1 << numChannels) - 1; // this will set to 1 all channels
			}
			else {
				// Here we do stuff.
				// Move this to a processAudio() function or something


				for (int32 i = 0; i < numChannels; i++)
				{
					if (data.symbolicSampleSize == Vst::SymbolicSampleSizes::kSample32) {
						int32 samples = data.numSamples;
						Vst::Sample32* ptrIn = (Vst::Sample32*)inBuffer[i];
						Vst::Sample32* ptrOut = (Vst::Sample32*)outBuffer[i];
						Vst::Sample32 tmp;
						
						while (--samples >= 0)
						{
							// apply gain
							tmp = (*ptrIn++) * mLevel;
							(*ptrOut++) = tmp;

							if (tmp > vuPPM)
							{
								vuPPM = tmp;
							}
						}
					}
					else {
						int32 samples = data.numSamples;
						Vst::Sample64* ptrIn = (Vst::Sample64*)inBuffer[i];
						Vst::Sample64* ptrOut = (Vst::Sample64*)outBuffer[i];
						Vst::Sample64 tmp;
						while (--samples >= 0)
						{
							// apply gain
							tmp = (*ptrIn++) * mLevel;
							(*ptrOut++) = tmp;

							if (tmp > vuPPM)
							{
								vuPPM = tmp;
							}
						}
					}
				}
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
