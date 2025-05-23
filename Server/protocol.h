constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 100;

constexpr int MAX_USER = 10000;
constexpr int MAX_NPC = 200000;

constexpr int W_WIDTH = 2000;
constexpr int W_HEIGHT = 2000;

constexpr char MOVE_UP = 0;
constexpr char MOVE_DOWN = 1;	
constexpr char MOVE_LEFT = 2;
constexpr char MOVE_RIGHT = 3;


// Packet ID
constexpr char CS_LOGIN = 0;
constexpr char CS_MOVE = 1;
constexpr char CS_CHAT = 2;
constexpr char CS_ATTACK = 3;			// 4 방향 공격
constexpr char CS_TELEPORT = 4;			// RANDOM한 위치로 Teleport, Stress Test할 때 Hot Spot현상을 피하기 위해 구현	
constexpr char CS_LOGOUT = 5;			// 클라이언트에서 정상적으로 접속을 종료하는 패킷


constexpr char SC_LOGIN_INFO = 2;
constexpr char SC_ADD_OBJECT = 3;
constexpr char SC_REMOVE_OBJECT = 4;
constexpr char SC_MOVE_OBJECT = 5;
constexpr char SC_CHAT = 6;
constexpr char SC_LOGIN_OK = 7;
constexpr char SC_LOGIN_FAIL = 8;
constexpr char SC_STAT_CHANGE = 9;

constexpr char SC_OBJECT_STATE = 10;

constexpr int VIEW_RANGE = 5; // TEST

#pragma pack (push, 1)
struct CS_LOGIN_PACKET {
	unsigned char size;
	char	type;
	char	name[NAME_SIZE];
};


struct CS_MOVE_PACKET {
	unsigned char	size;
	char			type;
	char			direction;  // 0 : UP, 1 : DOWN, 2 : LEFT, 3 : RIGHT
	unsigned		move_time;
};


struct CS_ATTACK_PACKET {
	unsigned char size;
	char	type;
	int		id;
	long long attack_time;
};


struct SC_LOGIN_INFO_PACKET {
	unsigned char size;
	char	type;
	int		id;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
	short	x, y;
	char	dir;
};


struct SC_MOVE_OBJECT_PACKET {
	unsigned char size;
	char	type;
	int		id;
	short	x, y;
	unsigned int move_time;
	char		dir;
};


struct SC_ADD_OBJECT_PACKET {
	unsigned char	size;
	char			type;
	int				id;
	short			x, y;
	char			name[NAME_SIZE];
	unsigned char	objType;
	char			dir;
	// ITEM도 오브젝트 인데, 아래의 값이 필요할까? 그냥 쓸까? 
	// -> 그냥 쓰자 
	int				hp;
	int				maxHP;
	int				exp;
	int				level;
};


struct SC_REMOVE_OBJECT_PACKET {
	unsigned char	size;
	char			type;
	int				id;
	unsigned char	objType;
};


struct SC_OBJECT_STATE_PACKET {
	unsigned char	size;
	char			type;
	int				id;
	unsigned char	objType;
	int				hp;
	int				maxHP;
	int				exp;
	int				level;
};
#pragma pack (pop)

enum class TASK_TYPE {
	PLAYER_UPDATE,
	MONSTER_MOVE,

	PLAYER_HEAL,
	MONSTER_REVIVE,
};

enum class OBJECT_TYPE : unsigned char {
	PLAYER,
	MONSTER,
	ITEM,

	END
};

struct Stat {
	std::atomic_int		hp;
	int					maxHp;
	std::atomic_int		level;
	std::atomic_int		exp;
};

enum class MONSTER_TYPE : unsigned char {
	DEFAULT,

	END
};

enum class ITEM_TYPE : unsigned char {
	POTION,
	SWORD,
	PISTOL,

};

enum class DIRECTION_TYPE : char {
	UP,
	DOWN,
	LEFT,
	RIGHT,

	END
};

enum class MOVING_OBJECT_STATE : unsigned char {
	IDLE,
	MOVE,
	DEAD,

	END
};