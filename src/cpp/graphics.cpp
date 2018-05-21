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

#include "graphics.h"

void Graphics::_appendPath(const PathRef &path) {
	if (paths.empty()) {
		nsvg.shapes = &path->nsvg;
	} else {
		current()->setNext(path);
	}
	path->clearNext();
	path->claim(this);
	paths.push_back(path);
}

void Graphics::beginPath() {
	if (!newPath) {
		return;
	}

	PathRef path = std::make_shared<Path>(this);
	_appendPath(path);

	path->beginTrajectory();
	newPath = false;
	newTrajectory = false;
}

void Graphics::closePath(bool closeIndeed) {
	if (!paths.empty() && !newPath) {
		const PathRef &path = current();

		path->nsvg.strokeWidth = strokeWidth;
		path->nsvg.strokeDashOffset = strokeDashOffset;
		path->nsvg.strokeDashCount = std::min(
			size_t(nsvg::maxDashes()), strokeDashes.size());
		for (int i = 0; i < path->nsvg.strokeDashCount; i++) {
			path->nsvg.strokeDashArray[i] = strokeDashes[i];
		}
		path->nsvg.strokeLineJoin = strokeLineJoin;
		path->nsvg.strokeLineCap = strokeLineCap;
		path->nsvg.miterLimit = miterLimit;
		path->nsvg.fillRule = fillRule;
		path->nsvg.flags = NSVG_FLAGS_VISIBLE;

		path->closeTrajectory(closeIndeed);
		changed(CHANGED_GEOMETRY);

		newPath = true;
	} else if (closeIndeed) {
		current()->closeTrajectory(true);
	}
}

void Graphics::initialize(float width, float height) {
	memset(&nsvg, 0, sizeof(nsvg));
	nsvg.width = width;
	nsvg.height = height;

	strokeColor = std::make_shared<Color>(0.25, 0.25, 0.25);
	fillColor = std::make_shared<Color>(0.95, 0.95, 0.95);

	strokeWidth = 3.0;
	strokeDashOffset = 0.0;
	strokeLineJoin = NSVG_JOIN_MITER;
	strokeLineCap = NSVG_CAP_BUTT;
	miterLimit = 4;
	fillRule = NSVG_FILLRULE_NONZERO;

	newPath = true;
	newTrajectory = true;
	boundsDirty = true;

	for (int i = 0; i < 4; i++) {
		bounds[i] = 0.0;
	}
}

Graphics::Graphics() : changes(0) {
	initialize(1.0, 1.0);
}

Graphics::Graphics(const NSVGimage *image) : changes(0) {
	initialize(image->width, image->height);

	NSVGshape *shape = image->shapes;
	while (shape) {
		PathRef path = std::make_shared<Path>(this, shape);
		if (paths.empty()) {
			nsvg.shapes = &path->nsvg;
		} else {
			paths[paths.size() - 1]->setNext(path);
		}
		path->claim(this);
		paths.push_back(path);
		shape = shape->next;
	}
}

Graphics::Graphics(const GraphicsRef &graphics) {
	throw std::runtime_error("not yet implemented");
}

void Graphics::clear() {
	if (paths.size() > 0) {
		for (int i = 0; i < paths.size(); i++) {
			paths[i]->unclaim(this);
		}
		paths.clear();
		nsvg.shapes = nullptr;
		changed(CHANGED_GEOMETRY);
	}
}

PathRef Graphics::beginTrajectory() {
	beginPath();
	if (!newTrajectory) {
		return current();
	}
	closeTrajectory();
	PathRef p = current();
	p->beginTrajectory();
	newTrajectory = false;
	return p;
}

void Graphics::closeTrajectory() {
	if (!paths.empty() && !newTrajectory) {
		current()->closeTrajectory();
		newTrajectory = true;
	}
}

void Graphics::setLineDash(const float *dashes, int count) {
	float sum = 0;
	for (int i = 0; i < count; i++) {
		sum += dashes[i];
	}
	if (sum <= 1e-6f) {
		count = 0;
	}

	strokeDashes.resize(count);
	for (int i = 0; i < count; i++) {
		strokeDashes[i] = dashes[i];
	}
}

void Graphics::fill() {
	if (!paths.empty()) {
		current()->setFillColor(fillColor);
		closePath(true);
		changed(CHANGED_GEOMETRY);
	}
}

void Graphics::stroke() {
	if (!paths.empty()) {
		current()->setLineColor(strokeColor);
		current()->setLineWidth(strokeWidth);
		current()->setLineDash(&strokeDashes[0], strokeDashes.size());
		current()->setLineDashOffset(strokeDashOffset);
		closePath();
		changed(CHANGED_GEOMETRY);
	}
}

void Graphics::addPath(const PathRef &path) {
	if (path->isClaimed()) {
		addPath(path->clone());
	} else {
		closePath();

		path->closeTrajectory();
		_appendPath(path);

		newPath = true;
		changed(CHANGED_GEOMETRY);
	}
}

NSVGimage *Graphics::getImage() {
	closePath();

	for (int i = 0; i < paths.size(); i++) {
		paths[i]->updateNSVG();
	}

	return &nsvg;
}

const float *Graphics::getBounds() {
	closePath();

	if (boundsDirty) {
		for (int i = 0; i < paths.size(); i++) {
			const PathRef &path = paths[i];
			path->updateBounds();
			if (i == 0) {
				for (int j = 0; j < 4; j++) {
					bounds[j] = path->nsvg.bounds[j];
				}
			} else {
				bounds[0] = std::min(bounds[0], path->nsvg.bounds[0]);
				bounds[1] = std::min(bounds[1], path->nsvg.bounds[1]);
				bounds[2] = std::max(bounds[2], path->nsvg.bounds[2]);
				bounds[3] = std::max(bounds[3], path->nsvg.bounds[3]);
			}
		}
		boundsDirty = false;
	}

	return bounds;
}

void Graphics::clean(float eps) {
	for (int i = 0; i < paths.size(); i++) {
		paths[i]->clean(eps);
	}
}

void Graphics::setOrientation(ToveOrientation orientation) {
	for (int i = 0; i < paths.size(); i++) {
		paths[i]->setOrientation(orientation);
	}
}

void Graphics::transform(float sx, float sy, float tx, float ty) {
	closeTrajectory();
	closePath();

	for (int i = 0; i < paths.size(); i++) {
		paths[i]->transform(sx, sy, tx, ty);
	}

	changed(CHANGED_GEOMETRY);
}

ToveChangeFlags Graphics::fetchChanges(ToveChangeFlags flags, bool clearAll) {
	const ToveChangeFlags c = changes & flags;
	changes &= ~flags;
	if (clearAll) {
		for (int i = 0; i < paths.size(); i++) {
			paths[i]->clearChanges(flags);
		}
	}
	return c;
}

void Graphics::animate(const GraphicsRef &a, const GraphicsRef &b, float t) {
	const int n = a->paths.size();
	if (n != b->paths.size()) {
		return;
	}
	if (paths.size() != n) {
		clear();
		while (paths.size() < n) {
			addPath(std::make_shared<Path>());
		}
	}
	for (int i = 0; i < n; i++) {
		paths[i]->animate(a->paths[i], b->paths[i], t);
	}
}
