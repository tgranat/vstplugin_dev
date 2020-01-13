
#include "../include/ggccontroller.h"
#include "../include/plugids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include <pluginterfaces/base/ustring.h>

namespace Steinberg {
namespace Vst {
namespace GgConvolver {

// Custom parameter class to display dB values for level instead of the range 0 to 1
// To make the parameter display for example dB instead of 0..1
// you need to implement your own parameter class that overrides toString() and fromString().

class LevelParameter : public Vst::Parameter
{
public:
	LevelParameter(int32 flags, int32 id, float levelAdjust);
	void toString(Vst::ParamValue normValue, Vst::String128 string) const SMTG_OVERRIDE;
	bool fromString(const Vst::TChar* string, Vst::ParamValue& normValue) const SMTG_OVERRIDE;
private:
	float adjust;
};

LevelParameter::LevelParameter(int32 flags, int32 id, float levelAdjust = 1)
{
	UString(info.title, USTRINGSIZE(info.title)).assign(USTRING("Level"));
	UString(info.units, USTRINGSIZE(info.units)).assign(USTRING("dB"));

	adjust = levelAdjust;
	info.flags = flags;
	info.id = id;
	info.stepCount = 0;
	info.defaultNormalizedValue = 0.5f;
	info.unitId = Vst::kRootUnitId;

	setNormalized(1.f);
}

	
void LevelParameter::toString(Vst::ParamValue normValue, Vst::String128 string) const
{
	char text[32];
	if (normValue > 0.0001)
	{
		sprintf(text, "%.2f", 20 * log10f((float)normValue * adjust));
	}
	else
	{
		strcpy(text, "-oo");
	}

	Steinberg::UString(string, 128).fromAscii(text);
}


bool LevelParameter::fromString(const Vst::TChar* string, Vst::ParamValue& normValue) const
{
	String wrapper((Vst::TChar*)string); // don't know buffer size here!
	double tmp = 0.0;
	if (wrapper.scanFloat(tmp))
	{
		// allow only values between -oo and 0dB
		if (tmp > 0.0)
		{
			tmp = -tmp;
		}

		normValue = expf(logf(10.f) * (float)tmp / 20.f);
		return true;
	}
	return false;
}

// Initialize controller:
//   - Create parameters

tresult PLUGIN_API GgcController::initialize(FUnknown* context)
{
	tresult result = EditController::initialize(context);
	if (result == kResultTrue)
	{
		// Bypass parameter
		parameters.addParameter(STR16("Bypass"), nullptr, 1, 0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
			GgConvolverParams::kBypassId);

		// Level parameter
		// Use own Parameter class that display dB values for level instead of the range 0 to 1
		// parameters.addParameter can also take a Parameter*

		auto* levelParam = new LevelParameter(Vst::ParameterInfo::kCanAutomate, kParamLevelId, 2.0);
		parameters.addParameter(levelParam);

		//---VuMeter parameter---
		int32 stepCount = 0;
		ParamValue defaultVal = 0;
		int32 flags = ParameterInfo::kIsReadOnly;
		int32 tag = kVuPPMId;
		parameters.addParameter(STR16("VuPPM"), nullptr, stepCount, defaultVal, flags, tag);

	}
	return kResultTrue;
}

tresult PLUGIN_API GgcController::setComponentState(IBStream* state)
{
	// we receive the current state of the component (processor part)
	// we read our parameters and bypass value...
	if (!state) {
		return kResultFalse;
	}

	IBStreamer streamer(state, kLittleEndian);

	float savedLevel = 0.f;
	if (streamer.readFloat(savedLevel) == false) {
		return kResultFalse;
	}
	setParamNormalized(GgConvolverParams::kParamLevelId, savedLevel);

	// read the bypass
	int32 bypassState = 0;
	if (streamer.readInt32(bypassState) == false) {
		return kResultFalse;
	}
	setParamNormalized(kBypassId, bypassState ? 1 : 0);

	return kResultOk;
}

} // namespaces
}
}
