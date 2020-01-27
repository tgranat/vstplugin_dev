
#include "../include/ggccontroller.h"
#include "../include/plugids.h"
#include "../include/dbparameter.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include <pluginterfaces/base/ustring.h>

namespace Steinberg {
namespace Vst {
namespace GgConvolver {

tresult PLUGIN_API GgcController::initialize(FUnknown* context)
{
	// Using EditController. 
	// Use EditControllerx1 to be able to create units (components) 
	tresult result = EditController::initialize(context);
	if (result == kResultTrue)
	{
		// Bypass parameter
		parameters.addParameter(STR16("Bypass"), nullptr, 1, 0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
			GgConvolverParams::kBypassId);

		// Impulse response parameter
		auto* irList = new StringListParameter(STR16("ImpulseResponse"), kParamImpulseResponse);

		irList->appendString(STR16("412_sm57_off_axis"));
		irList->appendString(STR16("412_sm57_on_axis_1"));
		irList->appendString(STR16("412_sm57_on_axis_2"));
		parameters.addParameter(irList);
		
		//irList->setNormalized(kIsYetPrefetchable / (kNumPrefetchableSupport - 1));

		// Level parameter
		// Use own DbParameter class that display dB values for level instead of the range 0 to 1
		// parameters.addParameter also takes a Parameter*

		auto* levelParam = new DbParameter(Vst::ParameterInfo::kCanAutomate, kParamLevelId, 2.0);
		parameters.addParameter(levelParam);

		// Pregain parameter
		auto* pregainParam = new DbParameter(Vst::ParameterInfo::kCanAutomate, kParamPregainId, 2.0);
		parameters.addParameter(pregainParam);

		// VuMeter parameters
		int32 stepCount = 0;
		ParamValue defaultVal = 0;
		int32 flags = ParameterInfo::kIsReadOnly;
		int32 tag = kVuPregainId;
		parameters.addParameter(STR16("VuPregain"), nullptr, stepCount, defaultVal, flags, tag);

		parameters.addParameter(STR16("VuLevel"), nullptr, 0, 0, ParameterInfo::kIsReadOnly, kVuLevelId);

	}
	return kResultTrue;
}

tresult PLUGIN_API GgcController::setComponentState(IBStream* state)
{
	// Receive the current state of the component (processor part)
	if (!state) {
		return kResultFalse;
	}

	IBStreamer streamer(state, kLittleEndian);

	// Read parameter values.
	// Has to be in the correct order! See implementation of setState() and getState() in processor part.
	int32 bypassState = 0;
	if (streamer.readInt32(bypassState) == false) {
		return kResultFalse;
	}
	setParamNormalized(kBypassId, bypassState ? 1 : 0);

	float savedImpulseResponse = 0;
	if (streamer.readFloat(savedImpulseResponse) == false) {
		return kResultFalse;
	}
	setParamNormalized(kParamImpulseResponse, savedImpulseResponse);

	float savedLevel = 0.f;
	if (streamer.readFloat(savedLevel) == false) {
		return kResultFalse;
	}
	setParamNormalized(kParamLevelId, savedLevel);

	float savedPregain = 0.f;
	if (streamer.readFloat(savedPregain) == false) {
		return kResultFalse;
	}
	setParamNormalized(kParamPregainId, savedPregain);

	return kResultOk;
}

IPlugView* PLUGIN_API GgcController::createView(const char* name)
{
	// someone wants my editor
	if (name && strcmp(name, "editor") == 0)
	{
		auto* view = new VSTGUI::VST3Editor(this, "view", "plug.uidesc");
		return view;
	}
	return nullptr;
}


} // namespaces
}
}
