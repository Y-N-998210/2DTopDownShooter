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
// --- 武器 ---
enum class WeaponType { AR, SMG, SG, SR};

// 武器パラメータ
struct Weapon {
	WeaponType type;
	String name;
	double range;
	double fire_rate;
	double damage;

	// 弾丸とリロード設定
	int32 magazine_size;	// マガジン容量
	int32 ammo;				// 現在のマガジン残弾
	int32 reserve_ammo;		// 予備弾数
	double reload_time;		// リロードにかかる時間(s)

	static Weapon Create(WeaponType type) {
		switch (type) {
		case WeaponType::AR: return { type, U"AR", 400.0, 0.2, 12.0, 30, 30, 120, 2.0 };
		case WeaponType::SMG: return { type, U"SMG", 250.0, 0.1, 8.0, 25, 25, 150, 1.5 };
		case WeaponType::SG: return { type, U"SG", 150.0, 0.8, 35.0, 8, 8, 32, 2.5 };
		case WeaponType::SR: return { type, U"SR", 700.0, 1.5, 60.0, 5, 5, 20, 3.0 };
		default:			return { type, U"AR", 400.0, 0.2, 12.0, 30, 30, 120, 2.0 };
		}
	}

	// リロード処理メソッド
	void reload() {
		if (reserve_ammo <= 0 || ammo == magazine_size) return;	// 予備弾数0とフル装填のときはそのまま返す
		int32 needed = magazine_size - ammo;
		int32 amount = Min(needed, reserve_ammo);
		ammo += amount;
		reserve_ammo -= amount;
	}
};

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
