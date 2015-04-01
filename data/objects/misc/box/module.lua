-- Copyright (C) 2015 Mats Wahlberg
--
-- Copying and distribution of this file, with or without modification,
-- are permitted in any medium without royalty provided the copyright
-- notice and this notice are preserved. This file is offered as-is,
-- without any warranty.

print "loading box module"

local module={}

function module.spawn(x, y, z)
	print("spawning at ("..x..","..y..","..z..")")
end

return module
