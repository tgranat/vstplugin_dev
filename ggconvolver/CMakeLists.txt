
set(plug_sources
    include/dbparameter.h
    include/ggccontroller.h
    include/plugids.h
    include/ggcprocessor.h
    include/version.h
	include/WavUtils.h
    source/dbparameter.cpp
    source/ggcfactory.cpp
    source/ggccontroller.cpp
    source/ggcprocessor.cpp
	source/WavUtils.cpp
    resource/celestion_v30_48kHz_200ms.data
    WDL/convoengine.h
    WDL/denormal.h
    WDL/fastqueue.h
    WDL/fft.h
    WDL/heapbuf.h
    WDL/ptrlist.h
    WDL/queue.h
    WDL/resample.h
    WDL/timing.h
    WDL/wdltypes.h
    WDL/convoengine.cpp
    WDL/fft.c
    WDL/resample.cpp
    WDL/timing.c
)

#--- HERE change the target Name for your plug-in (for ex. set(target myDelay))-------
set(target ggconvolver)

smtg_add_vst3plugin(${target} ${plug_sources})
set_target_properties(${target} PROPERTIES ${SDK_IDE_MYPLUGINS_FOLDER})
target_include_directories(${target} PUBLIC ${VSTGUI_ROOT}/vstgui4)
target_link_libraries(${target} PRIVATE base sdk vstgui_support)
#target_link_libraries(${target} PRIVATE base sdk)

smtg_add_vst3_resource(${target} "resource/plug.uidesc")
smtg_add_vst3_resource(${target} "resource/background.png")
smtg_add_vst3_resource(${target} "resource/slider_background.png")
smtg_add_vst3_resource(${target} "resource/slider_handle.png")
smtg_add_vst3_resource(${target} "resource/slider_handle_2.0x.png")
smtg_add_vst3_resource(${target} "resource/vu_on.png")
smtg_add_vst3_resource(${target} "resource/vu_off.png")

if(SMTG_MAC)
    smtg_set_bundle(${target} INFOPLIST "${CMAKE_CURRENT_LIST_DIR}/resource/Info.plist" PREPROCESS)
elseif(SMTG_WIN)
    target_sources(${target} PRIVATE resource/plug.rc)
endif()
