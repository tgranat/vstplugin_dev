// Custom parameter class to display dB values for level instead of the range 0 to 1
// Implements parameter class that overrides toString() and fromString().

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

namespace Steinberg {
namespace Vst {
namespace GgConvolver {

class DbParameter : public Steinberg::Vst::Parameter
{
public:
	DbParameter(Steinberg::int32 flags, Steinberg::int32 id, float levelAdjust);
	void toString(Steinberg::Vst::ParamValue normValue, Steinberg::Vst::String128 string) const SMTG_OVERRIDE;
	bool fromString(const Steinberg::Vst::TChar* string, Steinberg::Vst::ParamValue& normValue) const SMTG_OVERRIDE;
private:
	float adjust;
};
}
}
}