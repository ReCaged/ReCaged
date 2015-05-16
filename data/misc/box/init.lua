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

function module.create(parent, x, y, z, r)
	o=object.create(parent)
	local g=geom.box(o, 1, 1, 1)
	g:position(x, y, z)

	if r then
		g:rotation(r)
	end

	--todo: body

	return o
end

return module
