#ifndef ANDROID_SRS_PROCESSING_API
#define ANDROID_SRS_PROCESSING_API

namespace android {

class SRS_Processing {
public:
	static const int AUTO;										// A special-case handle is _always_ available SRS_Processing::AUTO

	// Setup/Shutdown
	static int CreateHandle();									// Create a unique handle to an instance of SRS_Processing
	static void DestroyHandle(int handle);						// Destroy a handle
	
	// Audio to Speaker/Output
	static void ProcessOutNotify(int handle, void* pSource, bool init);		// Buffers from pSource will be processed - (or closed if init=false)
	static void ProcessOutRoute(int handle, void* pSource, int device);		// Called on any Routing parameter changes - device is from AudioSystem::DEVICE_OUT_XXX
	static void ProcessOut(int handle, void* pSource, void* pSamples, int sampleBytes, int sampleRate, int countChans);		// Process the buffer specified
	
	// Audio from Mic/Input
	static void ProcessInNotify(int handle, void* pSource, bool init);		// Buffers from pSource will be processed - (or closed if init=false)
	static void ProcessInRoute(int handle, void* pSource, int device);		// Called on any Routing parameter changes - device is from AudioSystem::DEVICE_IN_XXX
	static void ProcessIn(int handle, void* pSource, void* pSamples, int sampleBytes, int sampleRate, int countChans);		// Process the buffer specified
	
	// Parameters via String
	static void ParamsSet(int handle, const String8& keyValues);
	static String8 ParamsGet(int handle, const String8& keys);
	
	static bool ParamsSet_Notify(int handle, const String8& keyValues);
	
	// Typeless Data...
	static void RawDataSet(int* pHandle, char* pKey, void* pData, int dataLen);		// Used for side-band configuration.  NULL handle means 'global' or singleton.
	
	// Parameters via Enum	- param defined in seperate params enum header
	//ENUMS SAVED FOR LATER
	//static void EnumSet(int handle, int param, float value);
	//static float EnumGet(int handle, int param);
};

};	// namespace android

#endif // ANDROID_SRS_PROCESSING_API
