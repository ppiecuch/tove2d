-- TÖVE Demo: Flipbooks and animations.
-- (C) 2018 Bernhard Liebl, MIT license.

require "lib/tove"
require "assets/tovedemo"

local svg1 = love.filesystem.read("gradient-circle_00001.svg")
local svg2 = love.filesystem.read("gradient-circle_00002.svg")

local tween = tove.newTween(svg1):to(svg2, 1)

local animations = {}

table.insert(animations, tove.newFlipbook(8, tween, "bitmap"))
table.insert(animations, tove.newAnimation(tween, "mesh", tove.fixed(2)))
table.insert(animations, tove.newAnimation(tween, "curves"))

local flow = tovedemo.newCoverFlow(0.5)
flow:add("flipbook", animations[1])
flow:add("mesh", animations[2])
flow:add("curves", animations[3])

function love.draw()
	tovedemo.draw("Blob.")

	local speed = 1
	local t = math.abs(math.sin(love.timer.getTime() * speed))

	for i, animation in ipairs(animations) do
		animation.t = t
	end

	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end
