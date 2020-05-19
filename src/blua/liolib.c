/*
** $Id: liolib.c,v 2.73.1.3 2008/01/18 17:47:43 roberto Exp $
** Standard I/O (and system) library
** See Copyright Notice in lua.h
*/


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define liolib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "../i_system.h"
#include "../g_game.h"
#include "../d_netfil.h"
#include "../lua_libs.h"
#include "../byteptr.h"
#include "../lua_script.h"
#include "../m_misc.h"


#define IO_INPUT	1
#define IO_OUTPUT	2

#define FILELIMIT (1024 * 1024) // Size limit for reading/writing files

#define FMT_FILECALLBACKID "file_callback_%d"


// Allow scripters to write files of these types to SRB2's folder
static const char *whitelist[] = {
	".bmp",
	".cfg",
	".csv",
	".dat",
	".png",
	".sav2",
	".txt",
};


static int pushresult (lua_State *L, int i, const char *filename) {
  int en = errno;  /* calls to Lua API may change this value */
  if (i) {
    lua_pushboolean(L, 1);
    return 1;
  }
  else {
    lua_pushnil(L);
    if (filename)
      lua_pushfstring(L, "%s: %s", filename, strerror(en));
    else
      lua_pushfstring(L, "%s", strerror(en));
    lua_pushinteger(L, en);
    return 3;
  }
}


#define tofilep(L)	((FILE **)luaL_checkudata(L, 1, LUA_FILEHANDLE))


static int io_type (lua_State *L) {
  void *ud;
  luaL_checkany(L, 1);
  ud = lua_touserdata(L, 1);
  lua_getfield(L, LUA_REGISTRYINDEX, LUA_FILEHANDLE);
  if (ud == NULL || !lua_getmetatable(L, 1) || !lua_rawequal(L, -2, -1))
    lua_pushnil(L);  /* not a file */
  else if (*((FILE **)ud) == NULL)
    lua_pushliteral(L, "closed file");
  else
    lua_pushliteral(L, "file");
  return 1;
}


static FILE *tofile (lua_State *L) {
  FILE **f = tofilep(L);
  if (*f == NULL)
    luaL_error(L, "attempt to use a closed file");
  return *f;
}



/*
** When creating file handles, always creates a `closed' file handle
** before opening the actual file; so, if there is a memory error, the
** file is not left opened.
*/
static FILE **newfile (lua_State *L) {
  FILE **pf = (FILE **)lua_newuserdata(L, sizeof(FILE *));
  *pf = NULL;  /* file handle is currently `closed' */
  luaL_getmetatable(L, LUA_FILEHANDLE);
  lua_setmetatable(L, -2);
  return pf;
}


/*
** function to (not) close the standard files stdin, stdout, and stderr
*/
static int io_noclose (lua_State *L) {
  lua_pushnil(L);
  lua_pushliteral(L, "cannot close standard file");
  return 2;
}


/*
** function to close regular files
*/
static int io_fclose (lua_State *L) {
  FILE **p = tofilep(L);
  int ok = (fclose(*p) == 0);
  *p = NULL;
  return pushresult(L, ok, NULL);
}


static int aux_close (lua_State *L) {
  lua_getfenv(L, 1);
  lua_getfield(L, -1, "__close");
  return (lua_tocfunction(L, -1))(L);
}


static int io_close (lua_State *L) {
  if (lua_isnone(L, 1))
    lua_rawgeti(L, LUA_ENVIRONINDEX, IO_OUTPUT);
  tofile(L);  /* make sure argument is a file */
  return aux_close(L);
}


static int io_gc (lua_State *L) {
  FILE *f = *tofilep(L);
  /* ignore closed files */
  if (f != NULL)
    aux_close(L);
  return 0;
}


static int io_tostring (lua_State *L) {
  FILE *f = *tofilep(L);
  if (f == NULL)
    lua_pushliteral(L, "file (closed)");
  else
    lua_pushfstring(L, "file (%p)", f);
  return 1;
}


// Create directories in the path
void MakePathDirs(char *path)
{
	char *c;

	for (c = path; *c; c++)
		if (*c == '/' || *c == '\\')
		{
			char sep = *c;
			*c = '\0';
			I_mkdir(path, 0755);
			*c = sep;
		}
}


static int CheckFileName(lua_State *L, const char *filename)
{
	int length = strlen(filename);
	boolean pass = false;
	size_t i;

	if (strchr(filename, '\\'))
	{
		luaL_error(L, "access denied to %s: \\ is not allowed, use / instead", filename);
		return pushresult(L,0,filename);
	}

	for (i = 0; i < (sizeof (whitelist) / sizeof(const char *)); i++)
		if (!stricmp(&filename[length - strlen(whitelist[i])], whitelist[i]))
		{
			pass = true;
			break;
		}
	if (strstr(filename, "./")
		|| strstr(filename, "..") || strchr(filename, ':')
		|| filename[0] == '/'
		|| !pass)
	{
		luaL_error(L, "access denied to %s", filename);
		return pushresult(L,0,filename);
	}

	return 0;
}

static int io_open (lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);
	const char *mode = luaL_optstring(L, 2, "r");
	int checkresult;

	checkresult = CheckFileName(L, filename);
	if (checkresult)
		return checkresult;

	luaL_checktype(L, 3, LUA_TFUNCTION);

	if (!(strchr(mode, 'r') || strchr(mode, '+')))
		luaL_error(L, "open() is only for reading, use openlocal() for writing");

	AddLuaFileTransfer(filename, mode);

	return 0;
}


static int io_openlocal (lua_State *L) {
	FILE **pf;
	const char *filename = luaL_checkstring(L, 1);
	const char *mode = luaL_optstring(L, 2, "r");
	char *realfilename;
	luafiletransfer_t *filetransfer;
	int checkresult;

	checkresult = CheckFileName(L, filename);
	if (checkresult)
		return checkresult;

	realfilename = va("%s" PATHSEP "%s", luafiledir, filename);

	if (client && strnicmp(filename, "client/", strlen("client/")))
		I_Error("Access denied to %s\n"
		        "Clients can only access files stored in luafiles/client/\n",
		        filename);

	// Prevent access if the file is being downloaded
	for (filetransfer = luafiletransfers; filetransfer; filetransfer = filetransfer->next)
		if (!stricmp(filetransfer->filename, filename))
			I_Error("Access denied to %s\n"
			        "Files can't be opened while being downloaded\n",
			        filename);

	MakePathDirs(realfilename);

	// Open and return the file
	pf = newfile(L);
	*pf = fopen(realfilename, mode);
	return (*pf == NULL) ? pushresult(L, 0, filename) : 1;
}


void Got_LuaFile(UINT8 **cp, INT32 playernum)
{
	FILE **pf = NULL;
	UINT8 success = READUINT8(*cp); // The first (and only) byte indicates whether the file could be opened

	if (playernum != serverplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal luafile command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (!luafiletransfers)
		I_Error("No Lua file transfer\n");

	// Retrieve the callback and push it on the stack
	lua_pushfstring(gL, FMT_FILECALLBACKID, luafiletransfers->id);
	lua_gettable(gL, LUA_REGISTRYINDEX);

	// Push the first argument (file handle or nil) on the stack
	if (success)
	{
		char mode[4];

		// Ensure we are opening in binary mode
		// (if it's a text file, newlines have been converted already)
		strcpy(mode, luafiletransfers->mode);
		if (!strchr(mode, 'b'))
			strcat(mode, "b");

		pf = newfile(gL); // Create and push the file handle
		*pf = fopen(luafiletransfers->realfilename, mode); // Open the file
		if (!*pf)
			I_Error("Can't open file \"%s\"\n", luafiletransfers->realfilename); // The file SHOULD exist
	}
	else
		lua_pushnil(gL);

	// Push the second argument (file name) on the stack
	lua_pushstring(gL, luafiletransfers->filename);

	// Call the callback
	LUA_Call(gL, 2);

	if (success)
	{
		// Close the file
		if (*pf)
		{
			fclose(*pf);
			*pf = NULL;
		}

		if (client)
			remove(luafiletransfers->realfilename);
	}

	RemoveLuaFileTransfer();

	if (waitingforluafilecommand)
	{
		waitingforluafilecommand = false;
		CL_PrepareDownloadLuaFile();
	}

	if (server && luafiletransfers)
		SV_PrepareSendLuaFile();
}


void StoreLuaFileCallback(INT32 id)
{
	lua_pushfstring(gL, FMT_FILECALLBACKID, id);
	lua_pushvalue(gL, 3); // Parameter 3 is the callback
	lua_settable(gL, LUA_REGISTRYINDEX); // registry[callbackid] = callback
}


void RemoveLuaFileCallback(INT32 id)
{
	lua_pushfstring(gL, FMT_FILECALLBACKID, id);
	lua_pushnil(gL);
	lua_settable(gL, LUA_REGISTRYINDEX); // registry[callbackid] = nil
}


static int io_tmpfile (lua_State *L) {
  FILE **pf = newfile(L);
  *pf = tmpfile();
  return (*pf == NULL) ? pushresult(L, 0, NULL) : 1;
}


static int io_readline (lua_State *L);


static void aux_lines (lua_State *L, int idx, int toclose) {
  lua_pushvalue(L, idx);
  lua_pushboolean(L, toclose);  /* close/not close file when finished */
  lua_pushcclosure(L, io_readline, 2);
}


static int f_lines (lua_State *L) {
  tofile(L);  /* check that it's a valid file handle */
  aux_lines(L, 1, 0);
  return 1;
}


/*
** {======================================================
** READ
** =======================================================
*/


static int read_number (lua_State *L, FILE *f) {
  lua_Number d;
  if (fscanf(f, LUA_NUMBER_SCAN, &d) == 1) {
    lua_pushnumber(L, d);
    return 1;
  }
  else return 0;  /* read fails */
}


static int test_eof (lua_State *L, FILE *f) {
  int c = getc(f);
  ungetc(c, f);
  lua_pushlstring(L, NULL, 0);
  return (c != EOF);
}


static int read_line (lua_State *L, FILE *f) {
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  for (;;) {
    size_t l;
    char *p = luaL_prepbuffer(&b);
    if (fgets(p, LUAL_BUFFERSIZE, f) == NULL) {  /* eof? */
      luaL_pushresult(&b);  /* close buffer */
      return (lua_objlen(L, -1) > 0);  /* check whether read something */
    }
    l = strlen(p);
    if (l == 0 || p[l-1] != '\n')
      luaL_addsize(&b, l);
    else {
      luaL_addsize(&b, l - 1);  /* do not include `eol' */
      luaL_pushresult(&b);  /* close buffer */
      return 1;  /* read at least an `eol' */
    }
  }
}


static int read_chars (lua_State *L, FILE *f, size_t n) {
  size_t rlen;  /* how much to read */
  size_t nr;  /* number of chars actually read */
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  rlen = LUAL_BUFFERSIZE;  /* try to read that much each time */
  do {
    char *p = luaL_prepbuffer(&b);
    if (rlen > n) rlen = n;  /* cannot read more than asked */
    nr = fread(p, sizeof(char), rlen, f);
    luaL_addsize(&b, nr);
    n -= nr;  /* still have to read `n' chars */
  } while (n > 0 && nr == rlen);  /* until end of count or eof */
  luaL_pushresult(&b);  /* close buffer */
  return (n == 0 || lua_objlen(L, -1) > 0);
}


static int g_read (lua_State *L, FILE *f, int first) {
  int nargs = lua_gettop(L) - 1;
  int success;
  int n;
  clearerr(f);
  if (nargs == 0) {  /* no arguments? */
    success = read_line(L, f);
    n = first+1;  /* to return 1 result */
  }
  else {  /* ensure stack space for all results and for auxlib's buffer */
    luaL_checkstack(L, nargs+LUA_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (lua_type(L, n) == LUA_TNUMBER) {
        size_t l = (size_t)lua_tointeger(L, n);
        success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
      }
      else {
        const char *p = lua_tostring(L, n);
        luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
        switch (p[1]) {
          case 'n':  /* number */
            success = read_number(L, f);
            break;
          case 'l':  /* line */
            success = read_line(L, f);
            break;
          case 'a':  /* file */
            read_chars(L, f, ~((size_t)0));  /* read MAX_SIZE_T chars */
            success = 1; /* always success */
            break;
          default:
            return luaL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (ferror(f))
    return pushresult(L, 0, NULL);
  if (!success) {
    lua_pop(L, 1);  /* remove last result */
    lua_pushnil(L);  /* push nil instead */
  }
  return n - first;
}


static int f_read (lua_State *L) {
  return g_read(L, tofile(L), 2);
}


static int io_readline (lua_State *L) {
  FILE *f = *(FILE **)lua_touserdata(L, lua_upvalueindex(1));
  int sucess;
  if (f == NULL)  /* file is already closed? */
    luaL_error(L, "file is already closed");
  sucess = read_line(L, f);
  if (ferror(f))
    return luaL_error(L, "%s", strerror(errno));
  if (sucess) return 1;
  else {  /* EOF */
    if (lua_toboolean(L, lua_upvalueindex(2))) {  /* generator created file? */
      lua_settop(L, 0);
      lua_pushvalue(L, lua_upvalueindex(1));
      aux_close(L);  /* close it */
    }
    return 0;
  }
}

/* }====================================================== */


static int g_write (lua_State *L, FILE *f, int arg) {
  int nargs = lua_gettop(L) - 1;
  int status = 1;
  for (; nargs--; arg++) {
    if (lua_type(L, arg) == LUA_TNUMBER) {
      /* optimization: could be done exactly as for strings */
      status = status &&
          fprintf(f, LUA_NUMBER_FMT, lua_tonumber(L, arg)) > 0;
    }
    else {
      size_t l;
      const char *s = luaL_checklstring(L, arg, &l);
      if (ftell(f) + l > FILELIMIT) {
          luaL_error(L,"write limit bypassed in file. Changes have been discarded.");
          break;
      }
      status = status && (fwrite(s, sizeof(char), l, f) == l);
    }
  }
  return pushresult(L, status, NULL);
}


static int f_write (lua_State *L) {
  return g_write(L, tofile(L), 2);
}


static int f_seek (lua_State *L) {
  static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
  static const char *const modenames[] = {"set", "cur", "end", NULL};
  FILE *f = tofile(L);
  int op = luaL_checkoption(L, 2, "cur", modenames);
  long offset = luaL_optlong(L, 3, 0);
  op = fseek(f, offset, mode[op]);
  if (op)
    return pushresult(L, 0, NULL);  /* error */
  else {
    lua_pushinteger(L, ftell(f));
    return 1;
  }
}


static int f_setvbuf (lua_State *L) {
  static const int mode[] = {_IONBF, _IOFBF, _IOLBF};
  static const char *const modenames[] = {"no", "full", "line", NULL};
  FILE *f = tofile(L);
  int op = luaL_checkoption(L, 2, NULL, modenames);
  lua_Integer sz = luaL_optinteger(L, 3, LUAL_BUFFERSIZE);
  int res = setvbuf(f, NULL, mode[op], sz);
  return pushresult(L, res == 0, NULL);
}


static int f_flush (lua_State *L) {
  return pushresult(L, fflush(tofile(L)) == 0, NULL);
}


static const luaL_Reg iolib[] = {
  {"close", io_close},
  {"open", io_open},
  {"openlocal", io_openlocal},
  {"tmpfile", io_tmpfile},
  {"type", io_type},
  {NULL, NULL}
};


static const luaL_Reg flib[] = {
  {"close", io_close},
  {"flush", f_flush},
  {"lines", f_lines},
  {"read", f_read},
  {"seek", f_seek},
  {"setvbuf", f_setvbuf},
  {"write", f_write},
  {"__gc", io_gc},
  {"__tostring", io_tostring},
  {NULL, NULL}
};


static void createmeta (lua_State *L) {
  luaL_newmetatable(L, LUA_FILEHANDLE);  /* create metatable for file handles */
  lua_pushvalue(L, -1);  /* push metatable */
  lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  luaL_register(L, NULL, flib);  /* file methods */
}


static void createstdfile (lua_State *L, FILE *f, int k, const char *fname) {
  *newfile(L) = f;
  if (k > 0) {
    lua_pushvalue(L, -1);
    lua_rawseti(L, LUA_ENVIRONINDEX, k);
  }
  lua_pushvalue(L, -2);  /* copy environment */
  lua_setfenv(L, -2);  /* set it */
  lua_setfield(L, -3, fname);
}


static void newfenv (lua_State *L, lua_CFunction cls) {
  lua_createtable(L, 0, 1);
  lua_pushcfunction(L, cls);
  lua_setfield(L, -2, "__close");
}


LUALIB_API int luaopen_io (lua_State *L) {
  createmeta(L);
  /* create (private) environment (with fields IO_INPUT, IO_OUTPUT, __close) */
  newfenv(L, io_fclose);
  lua_replace(L, LUA_ENVIRONINDEX);
  /* open library */
  luaL_register(L, LUA_IOLIBNAME, iolib);
  /* create (and set) default files */
  newfenv(L, io_noclose);  /* close function for default files */
  createstdfile(L, stdin, IO_INPUT, "stdin");
  createstdfile(L, stdout, IO_OUTPUT, "stdout");
  createstdfile(L, stderr, 0, "stderr");
  lua_pop(L, 1);  /* pop environment for default files */
  return 1;
}

