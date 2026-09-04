LOCAL_PATH := $(call my-dir)
TPT_ROOT := $(LOCAL_PATH)/..

include $(CLEAR_VARS)
LOCAL_MODULE := sdl-1.2
LOCAL_SRC_FILES := prebuilt/$(TARGET_ARCH_ABI)/libsdl-1.2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := luajit
LOCAL_SRC_FILES := prebuilt/$(TARGET_ARCH_ABI)/libluajit.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := bzip2
LOCAL_SRC_FILES := prebuilt/$(TARGET_ARCH_ABI)/libbzip2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := nghttp2
LOCAL_SRC_FILES := prebuilt/$(TARGET_ARCH_ABI)/libnghttp2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_SRC_FILES := prebuilt/$(TARGET_ARCH_ABI)/libcurl.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := fftw3f
LOCAL_SRC_FILES := prebuilt/$(TARGET_ARCH_ABI)/libfftw3f.so
include $(PREBUILT_SHARED_LIBRARY)

rwildcard = $(foreach entry,$(wildcard $1*),$(call rwildcard,$(entry)/,$2) $(filter $(subst *,%,$2),$(entry)))
TPT_SOURCES := $(call rwildcard,$(TPT_ROOT)/src/,*.cpp) $(call rwildcard,$(TPT_ROOT)/src/,*.c)

include $(CLEAR_VARS)
LOCAL_MODULE := application
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(TPT_SOURCES))
LOCAL_C_INCLUDES := \
	$(TPT_ROOT)/includes \
	$(sort $(dir $(TPT_SOURCES))) \
	$(LOCAL_PATH)/third_party/include \
	$(LOCAL_PATH)/third_party/include/SDL
LOCAL_CFLAGS := -w -Wall -Werror \
	-DLIN -DTOUCHUI -DLUACONSOLE -DGRAVFFT -DNOMOD -DLUAJIT -DSDL1_2
LOCAL_CPPFLAGS := -std=c++17 -frtti -fexceptions
LOCAL_CPP_FEATURES := exceptions rtti
LOCAL_SHARED_LIBRARIES := sdl-1.2 luajit bzip2 nghttp2 curl fftw3f
LOCAL_LDLIBS := -lGLESv1_CM -ldl -llog -lz
LOCAL_LDFLAGS := -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384
LOCAL_ARM_MODE := arm
include $(BUILD_SHARED_LIBRARY)
