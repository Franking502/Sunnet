#include "LuaAPI.h"
#include "stdint.h"
#include "Sunnet.h"
#include <unistd.h>
#include <string.h>

//注册lua函数
void LuaAPI::Register(lua_State *luaState)
{
    static luaL_Reg lualibs[] = 
    {
        {"NewService", NewService},
        //{"KillService", KillService},
        //{"Send", Send},
        //{"Listen", Listen},
        //{"CloseConn", CloseConn},
        //{"Write", Write},
        {NULL, NULL}
    };
    luaL_newlib(luaState, lualibs);
    lua_setglobal(luaState, "sunnet");
}

int LuaAPI::NewService(lua_State *luaState)
{
    //lua_gettop获取栈的大小. Lua调用C++时，被调用的方法会获得一个新栈，包含Lua传给C++的所有参数。故lua_gettop获取栈的大小即为参数个数
    int num = lua_gettop(luaState);
    //参数1：服务类型
    if(lua_isstring(luaState, 1) == 0)//1-是 0-否
    {
        lua_pushinteger(luaState, -1);
        return 1;
    }
    
    const char *type = lua_tostring(luaState, 1);
    char* newstr = new char[strlen(type) + 1];
    newstr[strlen(type)] = '\0';
    memcpy(newstr, type, strlen(type));
    auto t = make_shared<string>(newstr);

    uint32_t id = Sunnet::inst->NewService(t);

    //返回值
    lua_pushinteger(luaState, id);
    return 1;
}

int LuaAPI::KillService(lua_State *luaState)
{
    int num = lua_gettop(luaState);
    //参数1：服务id
    if(lua_isstring(luaState, 1) == 0)
    {
        return 0;
    }
    int id = lua_tointeger(luaState, 1);
    Sunnet::inst->KillService(id);
    return 0;
}