#pragma once
# include <Siv3D.hpp>

// --- 画面・マップ・システム設定 ---
constexpr int32 SCREEN_WIDTH = 1000;
constexpr int32 SCREEN_HEIGHT = 700;
//constexpr int32 SCREEN_WIDTH = 1920;
//constexpr int32 SCREEN_HEIGHT = 1080;
constexpr int32 MAP_WIDTH = 2000;	// (カメラが動く範囲)
constexpr int32 MAP_HEIGHT = 2000;	// (カメラが動く範囲)

const Color COLOR_BG(34, 139, 34);
const Color COLOR_GRID(50, 160, 50);
const Color COLOR_PLAYER(50, 50, 255);
const Color COLOR_ALLY(100, 100, 255);
const Color COLOR_ENEMY(255, 50, 50);
const Color COLOR_WALL(139, 69, 19);

// --- チーム定義 ---
enum class Team { None, Blue, Red };
// --- AIのstate ---
enum class State { Idle, Evade, Retreat, Attack, Capture };
// --- AIのrole ---
enum class Role { Player, Assault, Defender, Support, Flanker};

// --- ヘルパー関数: Stateの文字列変換 ---
// デバッグ描画で状態名を表示するために使用
inline String StateToString(State state) {
	switch (state) {
	case State::Idle: return U"IDLE";
	case State::Evade: return U"EVADE";
	case State::Retreat: return U"RETREAT";
	case State::Attack: return U"ATTACK";
	case State::Capture: return U"CAPTURE";
	default: return U"";
	}
}

// --- ヘルパー関数: Roleの文字列変換 ---
// デバッグ描画で兵科名を表示するために使用
inline String RoleToString(Role role) {
	switch (role)
	{
	case Role::Player: return U"PLAYER";
	case Role::Assault: return U"ASSAULT";
	case Role::Defender: return U"DEFENDER";
	case Role::Support: return U"SUPPORT";
	case Role::Flanker: return U"FLANKER";
	default: return U"";
	}
}
