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

log.add(1, "loading box module")

--table of methods to return (as this module/library)
local module={}

--only need to compute mass once, reused when creating
local m=mass.create()
m:boxtotal(400, 1, 1, 1) --400kg, 1x1x1 meter

--main feature of this module, the creator:
function module.create(parent, x, y, z, r)

	--create new object (belonging parent) to store all components
	o=object.create(parent)

	--create body with mass from above, and pos+rot as requested
	local b=body.create(o)
	b:mass(m)
	b:position(x, y, z)

	if r then
		b:rotation(r)
	end

	--create box geom and attach to body
	local g=geom.box(o, 1, 1, 1) --1x1x1m box
	g:body(b)
	g:mu(1) --friction 1N/N

	--provide access to this box object
	return o
end

--provide table as this module/library
return module

