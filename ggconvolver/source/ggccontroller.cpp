
#include "../include/ggccontroller.h"
#include "../include/plugids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include <pluginterfaces/base/ustring.h>

namespace Steinberg {
namespace Vst {
namespace GgConvolver {

	// Custom parameter class to display dB values for level instead of the range 0 to 1

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

			//------------------------------------------------------------------------
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

	//------------------------------------------------------------------------
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

			//-----------------------------------------------------------------------------
	tresult PLUGIN_API GgcController::initialize(FUnknown* context)
	{
		tresult result = EditController::initialize(context);
		if (result == kResultTrue)
		{
			//---Create Parameters------------
			parameters.addParameter(STR16("Bypass"), 0, 1, 0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
				GgConvolverParams::kBypassId);

			// To make the parameter display for example dB instead of 0..1
			// you need to implement an own parameter class that overrides to and fromString
			// parameters.addParameter can also take a Parameter*

			auto* levelParam = new LevelParameter(Vst::ParameterInfo::kCanAutomate, kParamLevelId, 2.0);
			parameters.addParameter(levelParam);

					// Stepcount: number of discrete steps (0: continuous, 1: toggle, discrete value otherwise 
					// (corresponding to max - min, for example: 127 for a min = 0 and a max = 127)
					//
					// Default normalized value: [0,1] (in case of discrete value: defaultNormalizedValue = defDiscreteValue / stepCount)
					//
					// 
					/*
					parameters.addParameter(STR16("Level"),    // Title
						STR16("dB"),                           // Unit
						0,                                     // Stepcount (0=continous)
						0.5,                                   // Default normalized value
						Vst::ParameterInfo::kCanAutomate,      // Flags
						TestPluginParams::kParamLevelId);      // Tag

						*/

						/*
						parameters.addParameter (STR16 ("Parameter 2"), STR16 ("On/Off"), 1, 1.,
												 Vst::ParameterInfo::kCanAutomate, TestPluginParams::kParamOnId, 0,
												 STR16 ("Param2"));
						*/
		}
		return kResultTrue;
	}

			//------------------------------------------------------------------------
			/*
			IPlugView* PLUGIN_API PlugController::createView(const char* name)
			{
				// someone wants my editor
				if (name && strcmp(name, "editor") == 0)
				{
					auto* view = new VSTGUI::VST3Editor(this, "view", "plug.uidesc");
					return view;
				}
				return nullptr;
			}
			*/

			//------------------------------------------------------------------------
	tresult PLUGIN_API GgcController::setComponentState(IBStream* state)
	{
		// we receive the current state of the component (processor part)
		// we read our parameters and bypass value...
		if (!state)
			return kResultFalse;

		IBStreamer streamer(state, kLittleEndian);


		float savedLevel = 0.f;
		if (streamer.readFloat(savedLevel) == false)
			return kResultFalse;
		setParamNormalized(GgConvolverParams::kParamLevelId, savedLevel);
				/*
				int8 savedParam2 = 0;
				if (streamer.readInt8 (savedParam2) == false)
					return kResultFalse;
				setParamNormalized (TestPluginParams::kParamOnId, savedParam2);
				*/
				// read the bypass
		int32 bypassState = 0;
		if (streamer.readInt32(bypassState) == false)
			return kResultFalse;
		setParamNormalized(kBypassId, bypassState ? 1 : 0);

		return kResultOk;
	}

			//------------------------------------------------------------------------
} 
}
}
