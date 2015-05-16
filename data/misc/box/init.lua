-- Copyright (C) 2015 Mats Wahlberg
--
-- Copying and distribution of this file, with or without modification,
-- are permitted in any medium without royalty provided the copyright
-- notice and this notice are preserved. This file is offered as-is,
-- without any warranty.
local log=require "log"
local object=require "object"
local geom=require "geom"

log.add(1, "loading box module")

local module={}

function module.create(parent, x, y, z, m)
	local x, y, z, m = x or 1, y or 1, z or 1 --, m or matrix.identity()

	log.add(1, "creating box at ("..x..","..y..","..z..")") --todo: remove this

	o=object.create(parent)
	local g=geom.box(o, 1, 1, 1)
	g:position(x, y, z)

	--todo: body

	return o
end

return module
