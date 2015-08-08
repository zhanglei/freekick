#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <fk_item.h>
#include <fk_lua.h>
#include <freekick.h>

static int fk_lua_pcall(lua_State *L);

lua_State *gL = NULL;

static const struct luaL_Reg fklib[] = {
	{"pcall", fk_lua_pcall},
	{NULL, NULL}
};

void fk_lua_init()
{
	gL = luaL_newstate();

	luaL_openlibs(gL);

	luaL_register(gL, "freekick", fklib);
}

int fk_lua_pcall(lua_State *L)
{
	size_t len;
	int argc, i;
	const char *arg;
	fk_str *cmd;
	fk_item *itm;
	fk_proto *pto;
	fk_conn *lua_conn;

	lua_conn = fk_conn_create(0);

	argc = lua_gettop(L);

	arg = luaL_checklstring(L, 1, &len);
	cmd = fk_str_create((char *)arg, len);
	fk_str_2upper(cmd);	

	for (i = 0; i < argc; i++) {
		arg = luaL_checklstring(L, 1, &len);
		itm = fk_item_create(FK_ITEM_STR, fk_str_create((char *)arg, len));
		fk_conn_arg_set(lua_conn, lua_conn->arg_idx, itm);
		fk_item_ref_inc(itm);
	}

	pto = fk_proto_search(cmd);
	pto->handler(lua_conn);

	return 1;
}

int fk_lua_keys_push(fk_conn *conn, int keyc)
{
	int i;
	fk_item *key;

	lua_newtable(gL);

	for (i = 0; i < keyc; i++) {
		key = fk_conn_arg_get(conn, 3 + i);	
		printf("key %d: %s\n", i, fk_str_raw((fk_str *)fk_item_raw(key)));
		lua_pushstring(gL, fk_str_raw((fk_str *)fk_item_raw(key)));
		lua_rawseti(gL, -2, i + 1);
	}

	lua_setglobal(gL, "KEYS");

	return 0;
}

int fk_lua_argv_push(fk_conn *conn, int argc, int keyc)
{
	int i;
	fk_item *arg;

	lua_newtable(gL);

	for (i = 0; i < argc; i++) {
		arg = fk_conn_arg_get(conn, 3 + keyc + i);	
		printf("arg %d: %s\n", i, fk_str_raw((fk_str *)fk_item_raw(arg)));
		lua_pushstring(gL, fk_str_raw((fk_str *)fk_item_raw(arg)));
		lua_rawseti(gL, -2, i + 1);
	}

	lua_setglobal(gL, "ARGV");

	return 0;
}

int fk_lua_script_run(char *code)
{
	int rt;

	printf("code: %s\n", code);

	rt = luaL_loadstring(gL, code) || lua_pcall(gL, 0, LUA_MULTRET, 0);

	return 0;
}
