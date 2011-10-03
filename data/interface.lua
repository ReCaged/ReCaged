--Copyright (C) 2011 Mats Wahlberg
--
--Copying and distribution of this file, with or without modification,
--are permitted in any medium without royalty provided the copyright
--notice and this notice are preserved.

--[[this lua script is the "main loop" for the interface thread. it also takes
care of start/stop the simulation thread and instructs it what it should
load. will eventually handle menu]]

--TODO: arguments to recaged should be passed to this script...
log.print(0, "hello, this is rc.lua providing the user interface");

--[[the following will be used to mimic menu selections

hello, welcome to rc! select a profile (or create a new):

profile default

#main menu... player selects "race"... and free-roam mode
#select world:
world Sandbox
#select track:
track Box

#waiting for players...
#one player
#please select team:
team Nemesis
#what car?
car Venom
#ok, tyre and rim
diameter 2 #rim diameter
tyre Slick #by world or track)
rim Split #by team or car)
#todo: automatic spawning position instead of
#manual x and y coords relative to track start
position -8 0

#another player joins the cage
team Nemesis
car Venom
diameter 2
tyre Slick
rim Split
position 8 0
]]

--simulation.start();

while simulation.tmp_runlevel() do
	interface.tmp_frame();
end

--simulation.stop();

