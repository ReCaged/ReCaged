-- Copyright (C) 2015 Mats Wahlberg
--
-- Copying and distribution of this file, with or without modification,
-- are permitted in any medium without royalty provided the copyright
-- notice and this notice are preserved. This file is offered as-is,
-- without any warranty.
local math=require "math"
local object=require "object"
local rotation=require "rotation"

local track=object.create()

local box=require "misc/box"

--2d pyramid, yes can automate with loops, but this is demo:
box.create(track, 10, 20, 0.5)
box.create(track, 11, 20, 0.5)
box.create(track, 12, 20, 0.5)
box.create(track, 13, 20, 0.5)
box.create(track, 14, 20, 0.5)

box.create(track, 10.5, 20, 1.5)
box.create(track, 11.5, 20, 1.5)
box.create(track, 12.5, 20, 1.5)
box.create(track, 13.5, 20, 1.5)

box.create(track, 11, 20, 2.5)
box.create(track, 12, 20, 2.5)
box.create(track, 13, 20, 2.5)

box.create(track, 11.5, 20, 3.5)
box.create(track, 12.5, 20, 3.5)

box.create(track, 12, 20, 4.5)

local rot=rotation.matrix()
local turn=math.pi/(2*7)
for i=0,7 do
	rot:fromaxisandangle(0, 0, 1, turn*i)
	box.create(track, -15, 20, 0.5+i, rot)
end


--[[> misc/flipper
12 0 0.25
-12 0 0.25

> misc/beachball
2 0 20

> misc/funbox
-8 -5 9

> misc/NH4
-2 16 1.2

> misc/building
20 60 0

> misc/pillar
30 30 0
-30 30 0
30 -30 0
-30 -30 0

> misc/tetrahedron
5 15 1
]]

