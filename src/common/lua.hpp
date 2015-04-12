/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2015 Mats Wahlberg
 *
 * This file is part of RCX.
 *
 * RCX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RCX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RCX.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#ifndef _RCX_LUA_H
#define _RCX_LUA_H

extern "C" {
#if OVERRIDE_LUA_HEADERS
#include OVERRIDE_LUA_H
#include OVERRIDE_LUALIB_H
#include OVERRIDE_LAUXLIB_H
#else
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#endif
}

#endif
