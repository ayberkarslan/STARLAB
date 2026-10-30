#pragma once
#if defined(_WIN32)
#define STARLAB_EXPORT extern "C" __declspec(dllexport)
#else
#define STARLAB_EXPORT extern "C"
#endif
