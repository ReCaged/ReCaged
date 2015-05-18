-- Copyright (C) 2015 Mats Wahlberg
--
-- Copying and distribution of this file, with or without modification,
-- are permitted in any medium without royalty provided the copyright
-- notice and this notice are preserved. This file is offered as-is,
-- without any warranty.

--see misc/box/init.lua for a detailed intro to scripting:
local object=require "object"
local body=require "body"
local mass=require "mass"
local geom=require "geom"
local model=require "model"

local Ballmass=mass.create()
Ballmass:spheretotal(20, 1)

local tmp=model.load("misc/beachball/sphere.obj")
local Balldraw=tmp:draw()
tmp:delete()

local module={}
function module.create(x, y, z, r)
	Obj=object.create()

	local Body=body.create(Obj)
	Body:model(Balldraw)
	Body:mass(Ballmass)
	Body:linear_drag(1) --low drag
	Body:angular_drag(1)
	Body:position(x, y, z)
	if r then
		Body:rotation(r)
	end

	local Geom=geom.sphere(Obj, 1)
	Geom:body(Body)
	Geom:mu(1)
	Geom:stiffness(5000) --springy/soft
	Geom:damping(1) --no damping (air drag is enough)

	Geom:damage(1000, 1500, function() --weak, but resistent
		Body:delete()
		Geom:delete()
	end)

	return Obj
end
return module
