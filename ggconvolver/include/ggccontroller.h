// Simple convolver VST3 plugin 
// Controller part

#pragma once

#include "vstgui/plugin-bindings/vst3editor.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace Steinberg {
namespace Vst {
namespace GgConvolver {

class GgcController : public EditController, public VSTGUI::VST3EditorDelegate
{
public:
	// Create new instances of this controller. Required by plugin factory

	static FUnknown* createInstance(void*)
	{
		return (IEditController*)new GgcController();
	}

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;
	IPlugView* PLUGIN_API createView(const char* name) SMTG_OVERRIDE;
private:
	int32 mNumberOfIRs;
};
}
}
}

