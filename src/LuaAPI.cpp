#include "LuaAPI.h"
#include "stdint.h"
#include "Sunnet.h"
#include <unistd.h>
#include <string.h>
#include <iostream>

//注册lua函数
void LuaAPI::Register(lua_State *luaState)
{
    static luaL_Reg lualibs[] = 
    {
        {"NewService", NewService},
        {"KillService", KillService},
        {"Send", Send},
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

int LuaAPI::Send(lua_State *luaState)
{
    int num = lua_gettop(luaState);
    if(num != 3)
    {
        cout << "Send fail, need 3 args, passed " << num << endl;
        return 0;
    }

    //参数1：确认发送方，即自身
    if(lua_isinteger(luaState, 1) == 0)
    {
        cout << "Send fail, sender error" << endl;
        return 0;
    }
    int source = lua_tointeger(luaState, 1);

    //参数2：确认接收方
    if(lua_isinteger(luaState, 2) == 0)
    {
        cout << "Send fail, receiver error" << endl;
        return 0;
    }
    int toId = lua_tointeger(luaState, 2);

    //参数3：发送内容
    if(lua_isstring(luaState, 3) == 0)
    {
        cout << "Send fail, send content error" << endl;
        return 0;
    }

    size_t len = 0;
    const char* buff = lua_tolstring(luaState, 3, &len);
    char* newstr = new char[len];
    memcpy(newstr, buff, len);

    //消息发送
    auto msg = make_shared<ServiceMsg>();
    msg->type = BaseMsg::TYPE::SERVICE;
    msg->source = source;
    msg->buff = shared_ptr<char>(newstr);
    msg->size = len;
    Sunnet::inst->Send(toId, msg);

    return 0;
}