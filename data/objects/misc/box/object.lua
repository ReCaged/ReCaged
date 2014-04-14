--Copyright (C) 2011 Mats Wahlberg
--
--Copying and distribution of this file, with or without modification,
--are permitted in any medium without royalty provided the copyright
--notice and this notice are preserved.

log.add(0, "hello, this is the Box object loading");

function spawn ()
	log.add(0, "hello, this is the Box object spawning");
end

return spawn;
