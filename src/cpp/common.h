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

#include "interface.h"
#include <memory>

#include "../thirdparty/nanosvg.h"
#include "../thirdparty/nanosvgrast.h"

#include "../thirdparty/clipper.hpp"
typedef ClipperLib::Path ClipperPath;
typedef ClipperLib::Paths ClipperPaths;
typedef ClipperLib::IntPoint ClipperPoint;

#define MAX_FLATTEN_RECURSIONS 10

class AbstractPaint;
typedef std::shared_ptr<AbstractPaint> PaintRef;

class Trajectory;
typedef std::shared_ptr<Trajectory> TrajectoryRef;

class Graphics;
typedef std::shared_ptr<Graphics> GraphicsRef;

class Path;
typedef std::shared_ptr<Path> PathRef;

class AbstractMesh;
typedef std::shared_ptr<AbstractMesh> MeshRef;

class AbstractShaderLink;
typedef std::shared_ptr<AbstractShaderLink> ShaderLinkRef;

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

class triangulation_failed {
};

#endif
