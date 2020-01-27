#pragma once

namespace Steinberg {
namespace Vst {
namespace GgConvolver {

	// Parameter ids which are exported to the host
	enum GgConvolverParams : ParamID
	{
		kBypassId = 100,
		kParamImpulseResponse,
		kParamLevelId,
		kParamPregainId,
		kVuLevelId,
		kVuPregainId
	};

	// Unique class ids for processor and for controller
	static const FUID GgConvolverProcessorUID(0x9DB9A9A6, 0x0A0243A2, 0xB4B4B445, 0x307AAF79);
	static const FUID GgConvolverControllerUID(0x545C1D90, 0x5F684382, 0xBD50868F, 0x50281DD8);
}
}
}
