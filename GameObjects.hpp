#pragma once
# include "Common.hpp"

// 前方宣言
struct Soldier;

// --- 障害物構造体 ---
struct Obstacle {
	RectF rect;
	Obstacle(double x, double y, double width, double height);
	void draw(const Vec2& camera) const;
};

// --- 拠点(CP)構造体　---
struct CapturePoint {
	Vec2 pos;
	double radius = 120.0;
	String name;
	Team owner = Team::None;
	double capture_progress = 0.0;

	CapturePoint(double x, double y, const String& n);
	void update(const Array<Soldier*>& all_soldiers, double dt);	// 占領ゲージの増減処理
	void draw(const Vec2& camera, const Font& font) const;
};

// --- 弾丸構造体 ---
struct Bullet {
	Vec2 pos;
	Vec2 velocity;
	Team team;
	double damage

	Bullet(double x, double y, double angle, Team t, double dmg);
	void update(double dt);
	void draw(const Vec2& camera) const;
};

// --- 視線判定関数 ---
// pos1-pos2上で障害物チェック(Trueで視界通る)
bool has_line_of_sight(const Vec2& pos1, const Vec2& pos2, const Array<Obstacle>& obstacles);

// --- 兵士(Player・AI)構造体 ---
struct Soldier {
	Vec2 pos;
	Team team;
	Role role;
	Weapon weapon;
	double flank_sign = 1.0;	// 左:-1, 右:1
	double health = 100.0;
	double shoot_cooldown = 0.0;
	bool is_player = false;
	State state = State::Idle;
	bool dead = false;
	double respawn_timer = 0.0;
	Color color;

	Soldier(double x, double y, Team t, bool player = false);

	// 障害物、マップは使途の当たり判定
	void move_with_collision(double dx, double dy, const Array<Obstacle>& obstacles);
	// 思考と行動
	void think_and_move(Soldier* player, const Array<Soldier*>& enemies, const Array<Soldier*>& allies,
						Array<Bullet>& bullets, const Array<Obstacle>& obstacles, const Array<CapturePoint>& capture_points, double dt);
	// 復活までのカウントダウンと、復活時の位置決定処理
	void update_respawn(const Array<CapturePoint>& capture_points, double dt);
	// 兵士の円、兵科、HPバー描画
	void draw(const Vec2& camera, const Font& font) const;
};

// --- マップ生成ヘルパー関数 ---
// 指定個数のランダムなサイズ・位置の壁生成
Array<Obstacle> generate_random_obstacles(int32 count);
