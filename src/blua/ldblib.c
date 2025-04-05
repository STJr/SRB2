/*
** $Id: ldblib.c,v 1.104.1.4 2009/08/04 18:50:18 roberto Exp $
** Interface from Lua to its debug API
** See Copyright Notice in lua.h
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ldblib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"



static void settabss (lua_State *L, const char *i, const char *v) {
  lua_pushstring(L, v);
  lua_setfield(L, -2, i);
}


static void settabsi (lua_State *L, const char *i, int v) {
  lua_pushinteger(L, v);
  lua_setfield(L, -2, i);
}


static lua_State *getthread (lua_State *L, int *arg) {
  if (lua_isthread(L, 1)) {
    *arg = 1;
    return lua_tothread(L, 1);
  }
  else {
    *arg = 0;
    return L;
  }
}


static void treatstackoption (lua_State *L, lua_State *L1, const char *fname) {
  if (L == L1) {
    lua_pushvalue(L, -2);
    lua_remove(L, -3);
  }
  else
    lua_xmove(L1, L, 1);
  lua_setfield(L, -2, fname);
}


static int db_getinfo (lua_State *L) {
  lua_Debug ar;
  int arg;
  lua_State *L1 = getthread(L, &arg);
  const char *options = luaL_optstring(L, arg+2, "flnSu");
  if (lua_isnumber(L, arg+1)) {
    if (!lua_getstack(L1, (int)lua_tointeger(L, arg+1), &ar)) {
      lua_pushnil(L);  /* level out of range */
      return 1;
    }
  }
  else if (lua_isfunction(L, arg+1)) {
    lua_pushfstring(L, ">%s", options);
    options = lua_tostring(L, -1);
    lua_pushvalue(L, arg+1);
    lua_xmove(L, L1, 1);
  }
  else
    return luaL_argerror(L, arg+1, "function or level expected");
  if (!lua_getinfo(L1, options, &ar))
    return luaL_argerror(L, arg+2, "invalid option");
  lua_createtable(L, 0, 2);
  if (strchr(options, 'S')) {
    settabss(L, "source", ar.source);
    settabss(L, "short_src", ar.short_src);
    settabsi(L, "linedefined", ar.linedefined);
    settabsi(L, "lastlinedefined", ar.lastlinedefined);
    settabss(L, "what", ar.what);
  }
  if (strchr(options, 'l'))
    settabsi(L, "currentline", ar.currentline);
  if (strchr(options, 'u'))
    settabsi(L, "nups", ar.nups);
  if (strchr(options, 'n')) {
    settabss(L, "name", ar.name);
    settabss(L, "namewhat", ar.namewhat);
  }
  if (strchr(options, 'L'))
    treatstackoption(L, L1, "activelines");
  if (strchr(options, 'f'))
    treatstackoption(L, L1, "func");
  return 1;  /* return table */
}


static int db_getlocal (lua_State *L) {
  int arg;
  lua_State *L1 = getthread(L, &arg);
  lua_Debug ar;
  const char *name;
  if (!lua_getstack(L1, luaL_checkint(L, arg+1), &ar))  /* out of range? */
    return luaL_argerror(L, arg+1, "level out of range");
  name = lua_getlocal(L1, &ar, luaL_checkint(L, arg+2));
  if (name) {
    lua_xmove(L1, L, 1);
    lua_pushstring(L, name);
    lua_pushvalue(L, -2);
    return 2;
  }
  else {
    lua_pushnil(L);
    return 1;
  }
}


static int auxupvalue (lua_State *L, int get) {
  const char *name;
  int n = luaL_checkint(L, 2);
  luaL_checktype(L, 1, LUA_TFUNCTION);
  if (lua_iscfunction(L, 1)) return 0;  /* cannot touch C upvalues from Lua */
  name = get ? lua_getupvalue(L, 1, n) : lua_setupvalue(L, 1, n);
  if (name == NULL) return 0;
  lua_pushstring(L, name);
  lua_insert(L, -(get+1));
  return get + 1;
}


static int db_getupvalue (lua_State *L) {
  return auxupvalue(L, 1);
}



static char KEY_HOOK = 'h';


static void hookf (lua_State *L, lua_Debug *ar) {
  static const char *const hooknames[] =
    {"call", "return", "line", "count", "tail return"};
  lua_pushlightuserdata(L, (void *)&KEY_HOOK);
  lua_rawget(L, LUA_REGISTRYINDEX);
  lua_pushlightuserdata(L, L);
  lua_rawget(L, -2);
  if (lua_isfunction(L, -1)) {
    lua_pushstring(L, hooknames[(int)ar->event]);
    if (ar->currentline >= 0)
      lua_pushinteger(L, ar->currentline);
    else lua_pushnil(L);
    lua_assert(lua_getinfo(L, "lS", ar));
    lua_call(L, 2, 0);
  }
}


static int makemask (const char *smask, int count) {
  int mask = 0;
  if (strchr(smask, 'c')) mask |= LUA_MASKCALL;
  if (strchr(smask, 'r')) mask |= LUA_MASKRET;
  if (strchr(smask, 'l')) mask |= LUA_MASKLINE;
  if (count > 0) mask |= LUA_MASKCOUNT;
  return mask;
}


static char *unmakemask (int mask, char *smask) {
  int i = 0;
  if (mask & LUA_MASKCALL) smask[i++] = 'c';
  if (mask & LUA_MASKRET) smask[i++] = 'r';
  if (mask & LUA_MASKLINE) smask[i++] = 'l';
  smask[i] = '\0';
  return smask;
}


static void gethooktable (lua_State *L) {
  lua_pushlightuserdata(L, (void *)&KEY_HOOK);
  lua_rawget(L, LUA_REGISTRYINDEX);
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    lua_createtable(L, 0, 1);
    lua_pushlightuserdata(L, (void *)&KEY_HOOK);
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_REGISTRYINDEX);
  }
}


static int db_sethook (lua_State *L) {
  int arg, mask, count;
  lua_Hook func;
  lua_State *L1 = getthread(L, &arg);
  if (lua_isnoneornil(L, arg+1)) {
    lua_settop(L, arg+1);
    func = NULL; mask = 0; count = 0;  /* turn off hooks */
  }
  else {
    const char *smask = luaL_checkstring(L, arg+2);
    luaL_checktype(L, arg+1, LUA_TFUNCTION);
    count = luaL_optint(L, arg+3, 0);
    func = hookf; mask = makemask(smask, count);
  }
  gethooktable(L);
  lua_pushlightuserdata(L, L1);
  lua_pushvalue(L, arg+1);
  lua_rawset(L, -3);  /* set new hook */
  lua_pop(L, 1);  /* remove hook table */
  lua_sethook(L1, func, mask, count);  /* set hooks */
  return 0;
}


static int db_gethook (lua_State *L) {
  int arg;
  lua_State *L1 = getthread(L, &arg);
  char buff[5];
  int mask = lua_gethookmask(L1);
  lua_Hook hook = lua_gethook(L1);
  if (hook != NULL && hook != hookf)  /* external hook? */
    lua_pushliteral(L, "external hook");
  else {
    gethooktable(L);
    lua_pushlightuserdata(L, L1);
    lua_rawget(L, -2);   /* get hook */
    lua_remove(L, -2);  /* remove hook table */
  }
  lua_pushstring(L, unmakemask(mask, buff));
  lua_pushinteger(L, lua_gethookcount(L1));
  return 3;
}


#define LEVELS1	12	/* size of the first part of the stack */
#define LEVELS2	10	/* size of the second part of the stack */

static int db_errorfb (lua_State *L) {
  int level;
  int firstpart = 1;  /* still before eventual `...' */
  int arg;
  lua_State *L1 = getthread(L, &arg);
  lua_Debug ar;
  if (lua_isnumber(L, arg+2)) {
    level = (int)lua_tointeger(L, arg+2);
    lua_pop(L, 1);
  }
  else
    level = (L == L1) ? 1 : 0;  /* level 0 may be this own function */
  if (lua_gettop(L) == arg)
    lua_pushliteral(L, "");
  else if (!lua_isstring(L, arg+1)) return 1;  /* message is not a string */
  else lua_pushliteral(L, "\n");
  lua_pushliteral(L, "stack traceback:");
  while (lua_getstack(L1, level++, &ar)) {
    if (level > LEVELS1 && firstpart) {
      /* no more than `LEVELS2' more levels? */
      if (!lua_getstack(L1, level+LEVELS2, &ar))
        level--;  /* keep going */
      else {
        lua_pushliteral(L, "\n\t...");  /* too many levels */
        while (lua_getstack(L1, level+LEVELS2, &ar))  /* find last levels */
          level++;
      }
      firstpart = 0;
      continue;
    }
    lua_pushliteral(L, "\n\t");
    lua_getinfo(L1, "Snl", &ar);
    lua_pushfstring(L, "%s:", ar.short_src);
    if (ar.currentline > 0)
      lua_pushfstring(L, "%d:", ar.currentline);
    if (*ar.namewhat != '\0')  /* is there a name? */
        lua_pushfstring(L, " in function " LUA_QS, ar.name);
    else {
      if (*ar.what == 'm')  /* main? */
        lua_pushfstring(L, " in main chunk");
      else if (*ar.what == 'C' || *ar.what == 't')
        lua_pushliteral(L, " ?");  /* C function or tail call */
      else
        lua_pushfstring(L, " in function <%s:%d>",
                           ar.short_src, ar.linedefined);
    }
    lua_concat(L, lua_gettop(L) - arg);
  }
  lua_concat(L, lua_gettop(L) - arg);
  return 1;
}


static const luaL_Reg dblib[] = {
  {"gethook", db_gethook},
  {"getinfo", db_getinfo},
  {"getlocal", db_getlocal},
  {"getupvalue", db_getupvalue},
  {"sethook", db_sethook},
  {"traceback", db_errorfb},
  {NULL, NULL}
};


LUALIB_API int luaopen_debug (lua_State *L) {
  luaL_register(L, LUA_DBLIBNAME, dblib);
  return 1;
}
