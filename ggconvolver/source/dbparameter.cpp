#include "../include/dbparameter.h"
#include <pluginterfaces/base/ustring.h>

namespace Steinberg {
namespace Vst {
namespace GgConvolver {

DbParameter::DbParameter(Steinberg::int32 flags, Steinberg::int32 id, float levelAdjust = 1.0f)
{
	Steinberg::UString(info.title, USTRINGSIZE(info.title)).assign(USTRING("Level"));
	Steinberg::UString(info.units, USTRINGSIZE(info.units)).assign(USTRING("dB"));

	adjust = levelAdjust;
	info.flags = flags;
	info.id = id;
	info.stepCount = 0;
	info.defaultNormalizedValue = 0.5f;
	info.unitId = Steinberg::Vst::kRootUnitId;

	setNormalized(1.f);
}


void DbParameter::toString(Steinberg::Vst::ParamValue normValue, Steinberg::Vst::String128 string) const
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


bool DbParameter::fromString(const Steinberg::Vst::TChar* string, Steinberg::Vst::ParamValue& normValue) const
{
	Steinberg::String wrapper((Steinberg::Vst::TChar*)string); // don't know buffer size here!
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

}
}
}