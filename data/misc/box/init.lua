-- Copyright (C) 2015 Mats Wahlberg
--
-- Copying and distribution of this file, with or without modification,
-- are permitted in any medium without royalty provided the copyright
-- notice and this notice are preserved. This file is offered as-is,
-- without any warranty.

--libraries:
local log=require "log"
local object=require "object"
local geom=require "body"
local geom=require "mass"
local geom=require "geom"
local model=require "model"

log.add(1, "loading box module")

--only need to compute mass once, reused when creating
local Boxmass=mass.create()
Boxmass:boxtotal(400, 1, 1, 1) --400kg, 1x1x1 meter

--load 3D model and create rendering data
--NOTE: this is a quick hack, syntax might change in future!
local tmp=model.load("misc/box/box.obj")
local Boxmodel=tmp:draw()
tmp:delete()

--table of methods to return (as this module/library)
local module={}

--main feature of this module, the creator:
function module.create(parent, x, y, z, r)

	--create new object (belonging parent) to store all components
	Obj=object.create(parent)

	--create body with mass from above, and pos+rot as requested
	local Body=body.create(Obj)
	Body:mass(Boxmass)
	Body:position(x, y, z)

	if r then
		Body:rotation(r)
	end

	--set 3D model to render
	Body:model(Boxmodel)

	--create box geom and attach to body
	local Geom=geom.box(Obj, 1, 1, 1) --1x1x1m box
	Geom:body(Body)
	Geom:mu(1) --friction 1N/N

	--configure callback for damage
	--threshold N, damage buffer Ns, function when buffer reach 0
	Geom:damage(100000, 10000, function()
		log.add(1, "box destroyed!")
		Body:delete()
		Geom:delete()
	end)

	--provide access to this box object
	return Obj
end

--provide table as this module/library
return module

