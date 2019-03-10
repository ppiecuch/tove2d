/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#ifndef __TOVE_COMMON
#define __TOVE_COMMON 1

#define BEGIN_TOVE_NAMESPACE namespace tove {
#define END_TOVE_NAMESPACE }

#define TOVE_TARGET_LOVE2D 1
#define TOVE_TARGET_GODOT 2

#ifdef TOVE_GODOT
#define TOVE_TARGET TOVE_TARGET_GODOT
#else
#define TOVE_TARGET TOVE_TARGET_LOVE2D
#endif

#define TOVE_GPUX_MESH_BAND 1
#define TOVE_DEBUG 0

#include "interface.h"
#include "warn.h"

#include <memory>
#include <limits>
#include <assert.h>

BEGIN_TOVE_NAMESPACE
#define NANOSVG_CPLUSPLUS 1
#include "../thirdparty/nanosvg.h"
#include "../thirdparty/nanosvgrast.h"
END_TOVE_NAMESPACE

#include "../thirdparty/clipper.hpp"

#if _MSC_VER
#define M_PI 3.1415926535897932384626433832795
#endif

#include "shared.h"
#include "tovedebug.h"

BEGIN_TOVE_NAMESPACE

typedef tove_gpu_float_t gpu_float_t;

inline void store_gpu_float(float &p, float x) {
	p = x;
}

#if __clang__
inline void store_gpu_float(uint16_t &p, float x) {
	*reinterpret_cast<__fp16*>(&p) = x;
}
#elif __GNUC__
#include <x86intrin.h>
inline void store_gpu_float(uint16_t &p, float x) {
	p = _cvtss_sh(x, 0);
}
#else
#include "../thirdparty/half/include/half.hpp"
inline void store_gpu_float(uint16_t &p, float x) {
	half_float::half y(x);
	p = *(uint16_t*)&y;
}
#endif

typedef ClipperLib::Path ClipperPath;
typedef ClipperLib::Paths ClipperPaths;
typedef ClipperLib::IntPoint ClipperPoint;

class AbstractPaint;
typedef SharedPtr<AbstractPaint> PaintRef;

class Subpath;
typedef SharedPtr<Subpath> SubpathRef;

class Graphics;
typedef SharedPtr<Graphics> GraphicsRef;

class Path;
typedef SharedPtr<Path> PathRef;

class AbstractTesselator;
typedef SharedPtr<AbstractTesselator> TesselatorRef;

class AbstractMesh;
typedef SharedPtr<AbstractMesh> MeshRef;

class AbstractFeed;
typedef SharedPtr<AbstractFeed> FeedRef;

inline int nextpow2(uint32_t v) {
	// see https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	if (v <= 8) {
		return 8;
	}
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

typedef float coeff;

END_TOVE_NAMESPACE

#endif // TOVE_COMMON
