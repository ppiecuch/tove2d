-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local PointsWidget = {}
PointsWidget.__index = PointsWidget

local function lengthSqr(dx, dy)
	return dx * dx + dy * dy
end

local function isControlPointVisible(selected, traj, i, j, k)
	if selected ~= nil and selected.path == i and selected.traj == j then
		local s = selected.pt
		local distance = math.abs(s - k)
		distance = math.min(distance, traj.points.count - distance)

		return distance <= 2
	end
end

function PointsWidget:allpoints(f)
	local graphics = self.object.graphics

	for i = 1, graphics.paths.count do
		local path = graphics.paths[i]
		for j = 1, path.subpaths.count do
			local traj = path.subpaths[j]

			for k = 1, traj.points.count, 3 do
				local pts = traj.points
				local pt = pts[k]
				if  f(traj, pts, i, j, k, pt.x, pt.y) then
					return true
				end
			end
		end
	end

	return false
end

function PointsWidget:allcontrolpoints(f)
	local graphics = self.object.graphics
	local selected = self.selected
	local dragging = self.dragging

	for i = 1, graphics.paths.count do
		local path = graphics.paths[i]
		for j = 1, path.subpaths.count do
			local traj = path.subpaths[j]

			local pts = traj.points
			for k = 1, traj.points.count, 3 do
				if not traj:isEdgeAt(k) then
					local isActive = false
					if dragging ~= nil and dragging.path == i and
						dragging.traj == j and dragging.pt0 == k then
						isActive = true
					end

					if isActive or isControlPointVisible(
						selected, traj, i, j, k - 1) then
						local pt1 = pts[k - 1]
						if f(traj, pts, i, j, k, k - 1, pt1.x, pt1.y) then
							return true
						end
					end

					if isActive or isControlPointVisible(
						selected, traj, i, j, k + 1) then
						local pt2 = pts[k + 1]
						if f(traj, pts, i, j, k, k + 1, pt2.x, pt2.y) then
							return true
						end
					end
				end
			end
		end
	end

	return false
end

function PointsWidget:oncurve(ux, uy, gs)
	local scaledgraphics = self.object.scaledgraphics
	for i = 1, scaledgraphics.paths.count do
		local path = scaledgraphics.paths[i]
		for j = 1, path.subpaths.count do
			local traj = path.subpaths[i]
			local dmax = 2 * gs + path:getLineWidth() * 0.5
			local t, d = traj:nearest(ux, uy, dmax, gs)
			if t >= 0 then
				return i, j, t
			end
		end
	end
	return -1, -1, -1
end

local function polar(x, y, ox, oy)
	local dx = x - ox
	local dy = y - oy
	return math.atan2(dy, dx), math.sqrt(dx * dx + dy * dy)
end

local function mirrorControlPoint(qx, qy, px, py, p0x, p0y, rx, ry)
	local oldphi, oldmag = polar(qx, qy, p0x, p0y)
	local newphi, newmag = polar(px, py, p0x, p0y)

	local cp1phi, cp1mag = polar(rx, ry, p0x, p0y)

	cp1phi = cp1phi + (newphi - oldphi)

	return p0x + math.cos(cp1phi) * cp1mag, p0y + math.sin(cp1phi) * cp1mag
end

function PointsWidget:updateOverlayLine()
	local overlayLine = self.overlayLine
	local handleColor = self.handles.color
	overlayLine:set(self.object.scaledgraphics)
	for i = 1, overlayLine.paths.count do
		local p = overlayLine.paths[i]
		p:setFillColor()
		p:setLineColor(unpack(handleColor))
		p:setLineWidth(1)
	end
end

function PointsWidget:draw(gtransform)
	local transform = self.object.transform
	local handles = self.handles

	love.graphics.push("transform")
	love.graphics.applyTransform(gtransform)
	love.graphics.applyTransform(transform.drawtransform)
	self.overlayLine:draw()
	love.graphics.pop()

	local tfm = love.math.newTransform()
	tfm:apply(gtransform)
	tfm:apply(transform.mousetransform)

	self:allcontrolpoints(function(traj, pts, i, j, k0, k, lx, ly)
		local x, y = tfm:transformPoint(lx, ly)
		local kx, ky = tfm:transformPoint(pts[k0].x, pts[k0].y)
		love.graphics.line(kx, ky, x, y)
		handles.selected:draw(x, y, 0, 0.75, 0.75)
	end)

	local selected = self.selected

	self:allpoints(function(traj, pts, i, j, k, lx, ly)
		local type = traj:isEdgeAt(k) and "edge" or "smooth"
		local h
		if selected ~= nil and selected.path == i and selected.traj == j and selected.pt == k then
			h = handles.knots.selected
		else
			h = handles.knots.normal
		end
		local x, y = tfm:transformPoint(lx, ly)
		h[type]:draw(x, y)
	end)
end

function PointsWidget:createDragPointFunc()
	local transform = self.object.transform
	local graphics = self.object.graphics
	local dragging = self.dragging

	return function(gx, gy)
		self.object:changePoints(function()
			local mx, my = transform:inverseTransformPoint(gx, gy)
			local traj = graphics.paths[dragging.path].subpaths[dragging.traj]
			traj:move(dragging.pt, mx, my)
		end)

		self:updateOverlayLine()
	end
end

function PointsWidget:createMouldCurveFunc(i, j, t)
	local transform = self.object.transform
	local traj = self.object.graphics.paths[i].subpaths[j]

	return function(gx, gy)
		local mx, my = transform:inverseTransformPoint(gx, gy)

		local ci = 1 + math.floor(t)
		local c1 = traj.curves[ci]

		local cp1x, cp1y = c1.cp1x, c1.cp1y
		local cp2x, cp2y = c1.cp2x, c1.cp2y

		traj:mould(t, mx, my)

		local prev = traj.curves[ci - 1]
		prev.cp2x, prev.cp2y = mirrorControlPoint(
			cp1x, cp1y, c1.cp1x, c1.cp1y, c1.x0, c1.y0, prev.cp2x, prev.cp2y)

		local next = traj.curves[ci + 1]
		next.cp1x, next.cp1y = mirrorControlPoint(
			cp2x, cp2y, c1.cp2x, c1.cp2y, c1.x, c1.y, next.cp1x, next.cp1y)

		self.object:refresh()
		self:updateOverlayLine()
	end
end

function PointsWidget:mousedown(gx, gy, gs, button)
	local transform = self.object.transform
	local lx, ly = transform:inverseTransformPoint(gx, gy)
	local ux, uy = transform:inverseUnscaledTransformPoint(gx, gy)

	local clickRadiusSqr = self.handles.clickRadiusSqr * gs * gs

	self.dragging = nil

 	if self:allpoints(function(traj, pts, i, j, k, px, py)
		if lengthSqr(lx - px, ly - py) < clickRadiusSqr then
			self.selected = {path = i, traj = j, pt = k}
			self.dragging = self.selected
			return true
		end
	end) then
		return self:createDragPointFunc()
	end

	if self:allcontrolpoints(function(traj, pts, i, j, k0, k, px, py)
		if lengthSqr(lx - px, ly - py) < clickRadiusSqr then
			self.dragging = {path = i, traj = j, pt = k, pt0 = k0}
			return true
		end
	end) then
		return self:createDragPointFunc()
	end

	self.selected = nil

	-- click on curve?
	local i, j, t = self:oncurve(ux, uy, gs)
	if t >= 0 then
		return self:createMouldCurveFunc(i, j, t)
	end

	return nil
end

function PointsWidget:click(gx, gy, gs, button)
	local transform = self.object.transform
	local lx, ly = transform:inverseTransformPoint(gx, gy)
	local ux, uy = transform:inverseUnscaledTransformPoint(gx, gy)

	local clickRadiusSqr = self.handles.clickRadiusSqr * gs * gs
	if self:allpoints(function(traj, pts, i, j, k, px, py)
		if lengthSqr(lx - px, ly - py) < clickRadiusSqr then
			return true
		end
	end) then
		return
	end

	local i, j, t = self:oncurve(ux, uy, gs)
	if t >= 0 then
		local traj = self.object.graphics.paths[i].subpaths[j]
		local k = traj:insertCurveAt(t)
		self.selected = {path = i, traj = j, pt = k}
		self.object:refresh()
		self:updateOverlayLine()
	end
end

function PointsWidget:mousereleased()
	self.dragging = nil
end

function PointsWidget:keypressed(key)
	if key == "backspace" then
		local selected = self.selected
		if selected ~= nil then
			if (selected.pt - 1) % 3 == 0 then
				self.object:changePoints(function()
					local traj = self.object.graphics.paths[selected.path].subpaths[selected.traj]
					traj:removeCurve((selected.pt - 1) / 3)
				end)
				self:updateOverlayLine()
			end
		end
	end
end

return function(handles, object)
	local overlayLine = tove.newGraphics()
	overlayLine:setDisplay("mesh")
	overlayLine:setUsage("points", "dynamic")

	local pw = setmetatable({
		handles = handles,
		object = object,
		selected = nil,
		dragging = nil,
		overlayLine = overlayLine}, PointsWidget)

	pw:updateOverlayLine()

	return pw
end
