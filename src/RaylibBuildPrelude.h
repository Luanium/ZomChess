#ifndef RAYLIB_BUILD_PRELUDE_H
#define RAYLIB_BUILD_PRELUDE_H
// Combined prelude header for the Raylib build.
// CMake's target_compile_options() collapses repeated `-include` flags when
// called multiple times, so both forced-includes are merged into this single
// file and injected with one -include flag instead.
#include "RaylibCompatTypes.h"
#include "RaylibAudioManagerReal.h"
#endif // RAYLIB_BUILD_PRELUDE_H
