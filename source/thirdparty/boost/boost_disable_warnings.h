// Boost is a very warning noisy library, so we allow some warnings to be disabled
// by including this header before including any boost headers. The companion to
// this is boost_enable_warnings.h which should be included as soon as possible
// after the affected code; especially if we're including from inside another header.

#include "thirdparty/gccdiagaware.h"

#ifdef GCC_DIAGNOSTIC_AWARE
// GCC/clang warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wparentheses"

#ifndef __clang__
// GCC specific warnings
#pragma GCC diagnostic ignored "-Wlogical-op"
#endif

#endif

#ifdef _MSC_VER
// MSVC warnings
#pragma warning( push )
#pragma warning( disable : 4100 ) // Unreferenced formal parameter
#pragma warning( disable : 4503 ) // Decorated name length exceeded, name was truncated
#pragma warning( disable : 6011 ) // Dereferencing NULL pointer
#endif
