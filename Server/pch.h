	#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#define SINGLETON(name)	\
private:						\
name()noexcept { }	\
~name()noexcept  { } 	\
friend class Singleton; 

#define MANAGER(name) (name::GetInstance())

#define NOMINMAX
#define ENABLE_VIEW_PROCESSING
#define ENABLE_SPACE_DEVISION
#define _CRT_SECURE_NO_WARNINGS

// #define MOVE_INTERVAL_1S
// #define ATTACK_INTERVAL_1S

// #define PLAYER_POS_FIX
#define MAX_MONSTER
// #define AI_LUA

#include <iostream>
// STL
#include <memory>
using std::unique_ptr;
using std::make_unique;

using std::shared_ptr;
using std::make_shared;

using std::weak_ptr;
using std::enable_shared_from_this;

#include <vector>
using std::vector;

#include <list>
using std::list;

#include <map>
using std::map;
using std::pair;

#include <unordered_map>
using std::unordered_map;
using std::make_pair;

#include <unordered_set>
using std::unordered_set;

#include <array>
using std::array;

#include <stack>
using std::stack;

#include <queue>
using std::queue;

#include <deque>
using std::deque;

#include <set>
using std::set;

#include <unordered_set>
using std::unordered_set;

// CPP
#include <filesystem>
namespace fs = std::filesystem;

#include <chrono>
namespace chrono = std::chrono;

#include <format>
using std::format;

#include <functional>
using std::function;

#include <algorithm>

#include <type_traits>

#include <cmath>

#include <optional>

#include <tuple>
using std::tuple;

#include <variant>
using std::variant;

#include <random>

#include <string>
using std::string;
using std::string_view;
using std::wstring;
using std::wstring_view;
#include <cassert>

#include <fstream>
using std::ifstream;
using std::ofstream;

#include <bitset>
using std::bitset;

#include <thread>
using std::thread;
using std::jthread;

#include <mutex>
using std::mutex;
using std::lock_guard;
using std::unique_lock;

#include <condition_variable>
using std::condition_variable;
using std::condition_variable_any;

#include <future>
using std::future;
using std::promise;
using std::packaged_task;

#include <latch>
#include <barrier>
#include <atomic>
#include <shared_mutex>
using std::array;


#include <print>
#include <format>	

#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>
#include <concurrent_unordered_set.h>
#include <concurrent_priority_queue.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

struct PacketHeader {
	unsigned char size;
	char type;
};

#include "protocol.h"

#include <sqlext.h>  
#include <locale.h>

// constexpr int PORT_NUM = 4000;

using namespace  std;

extern inline constinit thread_local unsigned int TLS_ThreadID = 0;

enum SERVER_STATE { ST_FREE, ST_ALLOC, ST_INGAME };

enum class KEY_INPUT : unsigned char {
	UP,
	DOWN,
	LEFT,
	RIGHT,
};

struct Vec2Int {
	int x;
	int y;

public:
	explicit Vec2Int()
		:x{ 0 }, y{ 0 }
	{

	}

	explicit Vec2Int(int ix, int iy)
		:x{ ix }, y{ iy }
	{

	}

	explicit Vec2Int(float fx, float fy)
		:x{ static_cast<int>(fx) }, y{ static_cast<int>(fy) }
	{

	}

	Vec2Int(const Vec2Int& other)
		:x{ other.x }, y{ other.y }
	{

	}

public:
	bool IsZero()
	{
		if(x = 0 && y == 0)
			return true;
		return false;
	}

	Vec2Int operator + (int i)
	{
		return Vec2Int{ x + i, y + i };
	}


	Vec2Int operator+(const Vec2Int& other)
	{
		Vec2Int ret;
		ret.x = x + other.x;
		ret.y = y + other.y;
		return ret;
	}

	Vec2Int operator-(const Vec2Int& other)
	{
		Vec2Int ret;
		ret.x = x - other.x;
		ret.y = y - other.y;
		return ret;
	}

	Vec2Int operator*(int value)
	{
		Vec2Int ret;
		ret.x = x * value;
		ret.y = y * value;
		return ret;
	}

	bool operator<(const Vec2Int& other) const
	{
		if(x != other.x)
			return x < other.x;

		return y < other.y;
	}

	bool operator>(const Vec2Int& other) const
	{
		if(x != other.x)
			return x > other.x;

		return y > other.y;
	}

	bool operator==(const Vec2Int& other) const
	{
		return x == other.x && y == other.y;
	}

	void operator+=(const Vec2Int& other)
	{
		x += other.x;
		y += other.y;
	}

	void operator-=(const Vec2Int& other)
	{
		x -= other.x;
		y -= other.y;
	}

	int LengthSquared()
	{
		return x * x + y * y;
	}

	float Length()
	{
		return (float)::sqrt(LengthSquared());
	}

	void Normalize()
	{
		float length = Length();
		if(length < 0.00000000001f)
			return;

		x /= static_cast<int>(length);
		y /= static_cast<int>(length);
	}

	int Dot(Vec2Int other)
	{
		return x * other.x + y * other.y;
	}

	int Cross(Vec2Int other)
	{
		return x * other.y - y * other.x;
	}
};

void print_error_message(int s_err);

extern std::default_random_engine dre;
extern std::uniform_int_distribution<short> randomPos;
extern std::uniform_int_distribution<short> playerSpawnPos;
#include "PacketFunc.h"
#include "SendBuffer.h"
#include "..\Include\lua.hpp"
#include "func.h"