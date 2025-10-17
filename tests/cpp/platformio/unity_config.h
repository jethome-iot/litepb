#pragma once

#if defined(ESP_PLATFORM)

#define UNITY_INCLUDE_DOUBLE
#define UNITY_DOUBLE_TYPE double
#define UNITY_DOUBLE_PRECISION 1e-9

//#define UNITY_SUPPORT_64 // <-- NOT works!!!

#elif defined(__unix__) || defined(__APPLE__)

// Enable support for double-precision assertions in Unity
#define UNITY_INCLUDE_DOUBLE
#define UNITY_DOUBLE_TYPE double
#define UNITY_DOUBLE_PRECISION 1e-9

// Enable 64-bit integer assertions (int64_t / uint64_t)
#define UNITY_SUPPORT_64

#else
#error "Unsupported platform"
#endif
