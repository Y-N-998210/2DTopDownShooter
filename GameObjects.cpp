# include "GameObjects.hpp"

// --- Obstacle ---
Obstacle::Obstacle(double x, double y, double width, double height) : rect(x, y, width, height) {}
void Obstacle::draw(const Vec2& camera) const {
	// -cameraによりスクリーン座標に変換
	RectF draw_rect = rect.movedBy(-camera);
	draw_rect.draw(COLOR_WALL);
	draw_rect.drawFrame(3, 0, Color(100, 50, 10));
}

// --- CapturePoint ---
CapturePoint::CapturePoint(double x, double y, const String& n) : pos(x, y), name(n) {}
void CapturePoint::update(const Array<Soldier*>& all_soldiers, double dt) {
	int32 blue_cnt = 0, red_cnt = 0;
	// 範囲内兵士カウント
	for (auto s : all_soldiers) {
		if (!s->dead && s->pos.distanceFrom(pos) < radius) {
			if (s->team == Team::Blue) blue_cnt++;
			else if (s->team == Team::Red) red_cnt++;
		}
	}
	// 人数差による占領進捗増減
	// 青が多いと100、赤だと-100に近づく
	if (blue_cnt > red_cnt) capture_progress += 30.0 * (blue_cnt - red_cnt) * dt;
	else if (red_cnt > blue_cnt) capture_progress -= 30.0 * (red_cnt - blue_cnt) * dt;

	// 進捗状況を-100～100に制限
	capture_progress = Clamp(capture_progress, -100.0, 100.0);
	// 進捗状況による占領
	if (capture_progress == 100.0) owner = Team::Blue;
	else if (capture_progress == -100.0) owner = Team::Red;
	else if (capture_progress == 0.0) owner = Team::None;
}

void CapturePoint::draw(const Vec2& camera, const Font& font) const {
	Vec2 draw_pos = pos - camera;
	Color base_color(150, 150, 150);	// デフォルト(中立)はグレー
	if (owner == Team::Blue) base_color = Color(100, 100, 255);
	else if (owner == Team::Red) base_color = Color(255, 100, 100);

	// 拠点の範囲描画
	Circle(draw_pos, radius).drawFrame(4, 0, base_color);

	// 占領の進捗率に応じた円の変化
	if (capture_progress != 0.0) {
		double prog_radius = radius * (Math::Abs(capture_progress) / 100.0);
		Color fill_color = (capture_progress > 0) ? Color(50, 50, 255) : Color(255, 50, 50);
		Circle(draw_pos, prog_radius).draw(fill_color);
	}

	// 拠点名表示
	font(name).draw(40, draw_pos.x - 10, draw_pos.y - 20, Palette::White);
}

// --- Bullet ---
Bullet::Bullet(double x, double y, double angle, Team t) : pos(x, y), team(t) {
	double speed = 720.0;

	// 角度(ラジアン)から方向ベクトルを作成し、速度を決定
	velocity = Vec2(std::cos(angle), std::sin(angle)) * speed;
}
void Bullet::update(double dt) {
	pos += velocity * dt;
}
void Bullet::draw(const Vec2& camera) const {
	Circle(pos - camera, 3).draw(Palette::Yellow);
}

// --- 視界判定 ---
// AABB(軸平行境界ボックス)と線分の交差判定(スラブ側判定測の一種)を利用し、2転換に壁が割り込んでいるかを計算
bool has_line_of_sight(const Vec2& pos1, const Vec2& pos2, const Array<Obstacle>& obstacles) {
	for (const auto& wall : obstacles) {
		Vec2 line_dir = pos2 - pos1;
		double t_min = 0.0, t_max = 1.0;	// 線分上の比率(0.0=始点, 1.0=終点)

		// X軸方向の衝突判定
		if (Math::Abs(line_dir.x) > 1e-6) {
			double t1 = (wall.rect.x - pos1.x) / line_dir.x;
			double t2 = (wall.rect.x + wall.rect.w - pos1.x) / line_dir.x;
			if (t1 > t2) std::swap(t1, t2);
			t_min = Max(t_min, t1); t_max = Min(t_max, t2);
			if (t_min > t_max) continue;	// この壁とは交差しない
		}
		// 垂線かつ壁のX範囲外なら交差しない
		else if (pos1.x < wall.rect.x || pos1.x > wall.rect.x + wall.rect.w) continue;

		// Y軸方向の衝突判定
		if (Math::Abs(line_dir.y) > 1e-6) {
			double t1 = (wall.rect.y - pos1.y) / line_dir.y;
			double t2 = (wall.rect.y + wall.rect.h - pos1.y) / line_dir.y;
			if (t1 > t2) std::swap(t1, t2);
			t_min = Max(t_min, t1); t_max = Min(t_max, t2);
			if (t_min > t_max) continue;	// この壁とは交差しない
		}
		// 水平線で、壁のY範囲外なら交差しない
		else if (pos1.y < wall.rect.y || pos1.y > wall.rect.y + wall.rect.h) continue;

		// 判定をすり抜けなかった=線分が壁の矩形と交差している⇒視線がさえぎられている
		return false;
	}
	return true;	// どの壁にも遮られなかった
}

// --- Soldier ---
// コンストラクタ
Soldier::Soldier(double x, double y, Team t, bool player) : pos(x, y), team(t), is_player(player), weapon(Weapon::Create(WeaponType::AR)) {
	if (is_player) {
		color = COLOR_PLAYER;
		role = Role::Player;
		weapon = Weapon::Create(WeaponType::SMG);
	}
	else {
		color = (team == Team::Blue) ? COLOR_ALLY : COLOR_ENEMY;

		// 役割抽選
		int32 r = Random(0, 3);
		if (r == 0) role = Role::Assault;
		else if (r == 1) role = Role::Defender;
		else if (r == 2) role = Role::Support;
		else role = Role::Flanker;

		// 役割ごとの武器
		if (role == Role::Assault) {
			weapon = Weapon::Create(RandomBool() ? WeaponType::AR : WeaponType::SMG);
		} else if (role == Role::Flanker) {
			weapon = Weapon::Create(RandomBool() ? WeaponType::SMG : WeaponType::SG); 
		} else if (role == Role::Support) {
			weapon = Weapon::Create(WeaponType::SR); 
		} else {
			weapon = Weapon::Create(WeaponType::AR);
		}

		// Flankerの周り方向決定
		flank_sign = RandomBool() ? 1.0 : -1.0;
	}
}

// 衝突判定つき移動関数(X移動、Y移動を別にして壁に沿えるようにした)
void Soldier::move_with_collision(double dx, double dy, const Array<Obstacle>& obstacles) {
	// X軸方向の移動と衝突判定
	pos.x += dx;
	RectF my_rect(pos.x - 20, pos.y - 20, 40, 40);	// 自身のヒットボックス(40*40)
	for (const auto& wall : obstacles) {
		if (my_rect.intersects(wall.rect)) {
			// 右移動中にぶつかったら壁の左端へ、左移動中なら壁の右端へ座標を押し戻す
			if (dx > 0) pos.x = wall.rect.x - 20;
			if (dx < 0) pos.x = wall.rect.x + wall.rect.w + 20;
		}
	}
	// Y軸方向の移動と衝突判定
	pos.y += dy;
	my_rect = RectF(pos.x - 20, pos.y - 20, 40, 40);
	for (const auto& wall : obstacles) {
		if (my_rect.intersects(wall.rect)) {
			// 下移動中にぶつかったら壁の上端へ、上移動中なら壁の下端へ座標を押し戻す
			if (dy > 0) pos.y = wall.rect.y - 20;
			if (dy < 0) pos.y = wall.rect.y + wall.rect.h + 20;
		}
	}
	// マップの端（境界線）からはみ出さないようにクランプ
	pos.x = Clamp<double>(pos.x, 20, MAP_WIDTH - 20);
	pos.y = Clamp<double>(pos.y, 20, MAP_HEIGHT - 20);
}


// AIの思考と行動ロジック
void Soldier::think_and_move(Soldier* player, const Array<Soldier*>& enemies, const Array<Soldier*>& allies,
							 Array<Bullet>& bullets, const Array<Obstacle>& obstacles, const Array<CapturePoint>& capture_points, double dt) {
	if (is_player || dead) return;

	// 敵の索敵
	// 自身のチームに応じ、ターゲットをセット ex)自分が青なら敵は赤
	Array<Soldier*> targets = (team == Team::Blue) ? enemies : allies;
	if (team == Team::Red) targets.push_back(player);

	// 視線(Line of Sight)が通り、かつ最も近い敵を探す
	Soldier* nearest_target = nullptr;
	double min_dist = 9999.0;
	for (auto t : targets) {
		if (t->dead) continue;
		double d = pos.distanceFrom(t->pos);
		if (d < min_dist && has_line_of_sight(pos, t->pos, obstacles)) { min_dist = d; nearest_target = t; }
	}

	// 拠点の探索
	// 【目標拠点の設定】自チームが未占領の拠点の中で、最も近い場所を探す
	const CapturePoint* nearest_cp = nullptr;
	double min_cp_dist = 9999.0;
	for (const auto& cp : capture_points) {
		if (cp.owner != team) {
			double d = pos.distanceFrom(cp.pos);
			if (d < min_cp_dist) { min_cp_dist = d; nearest_cp = &cp; }
		}
	}

	// 弾の危機察知
	// 【危険察知】自分に向かって飛んできている近くの敵の弾丸がないかチェック
	bool incoming_bullet = false;
	Vec2 evade_vec(0, 0);
	for (const auto& b : bullets) {
		if (b.team != team && pos.distanceFrom(b.pos) < 150.0) {
			if (b.velocity.normalized().dot((pos - b.pos).normalized()) > 0.8) {
				incoming_bullet = true;
				evade_vec += Vec2(-b.velocity.y, b.velocity.x).normalized();
			}
		}
	}

	// --- 状態遷移 ---
	if (incoming_bullet) state = State::Evade;
	else if (health < 30.0 && nearest_target && min_dist < 300.0) state = State::Retreat;
	else if (nearest_target && min_dist < weapon.range) state = State::Attack;
	else if (nearest_cp) state = State::Capture;
	else state = State::Idle;

	// 各状態のロジック
	// EVADE（弾丸回避）
	if (state == State::Evade) {
		move_with_collision(evade_vec.normalize().x * 180.0 * dt, evade_vec.normalize().y * 180.0 * dt, obstacles);
	}

	// RETREAT（撤退・逃走）
	else if (state == State::Retreat) {
		move_with_collision((pos - nearest_target->pos).normalize().x * 150.0 * dt, (pos - nearest_target->pos).normalize().y * 150.0 * dt, obstacles);
	}

	// --- ATTACK（攻撃・交戦） ---
	else if (state == State::Attack) {
		Vec2 dir_vec = (nearest_target->pos - pos);
		// 射撃
		shoot_cooldown -= dt;
		if (shoot_cooldown <= 0.0) {
			bullets.emplace_back(pos.x, pos.y, std::atan2(dir_vec.y, dir_vec.x), team);
			shoot_cooldown = weapon.fire_rate;
		}

		// 移動方法
		if (role == Role::Flanker) {
			// 敵の真横に
			Vec2 side_vec(-dir_vec.y, dir_vec.x);
			if (side_vec.isZero()) side_vec = Vec2(1, 0);
			Vec2 move_dir = (side_vec.normalize() * flank_sign * 0.7) + (dir_vec.normalize() * 0.3);
			move_with_collision(move_dir.normalize().x * 140.0 * dt, move_dir.normalize().y * 140.0 * dt, obstacles);
		}
		else {
			double ideal_dist = weapon.range * 0.7;
			if (min_dist > ideal_dist + 40.0) {
				move_with_collision(dir_vec.normalize().x * 120.0 * dt, dir_vec.normalize().y * 120.0 * dt, obstacles);
			}
			else if (min_dist < ideal_dist - 40.0) {
				move_with_collision(-dir_vec.normalize().x * 120.0 * dt, -dir_vec.normalize().y * 120.0 * dt, obstacles);
			}
		}
	}

	// --- CAPTURE（拠点占領へ移動） ---
	else if (state == State::Capture) {
		Vec2 cp_dir = (nearest_cp->pos - pos);
		double dist_to_cp = cp_dir.length();

		// 拠点に向かう時のアプローチ（兵科ごとの固有ルーチン）
		if (role == Role::Assault) {
			// ASSAULT: 中心へ突撃
			if (dist_to_cp > 20.0) {
				move_with_collision(cp_dir.normalize().x * 120.0 * dt, cp_dir.normalize().y * 120.0 * dt, obstacles);
			}
		}
		else if (role == Role::Defender) {
			// DEFENDER: 拠点に入り次第防衛、入るまでは移動
			if (dist_to_cp > nearest_cp->radius * 0.8) {
				move_with_collision(cp_dir.normalize().x * 90.0 * dt, cp_dir.normalize().y * 90.0 * dt, obstacles);
			}
			// エリア内でうろうろ刺せるか、停止（現在は停止)
		}
		else if (role == Role::Support) {
			// SUPPORT: 遅く追従
			if (dist_to_cp > 40.0) {
				move_with_collision(cp_dir.normalize().x * 70.0 * dt, cp_dir.normalize().y * 70.0 * dt, obstacles);
			}
		}
	}
}

// リスポーン処理
void Soldier::update_respawn(const Array<CapturePoint>& capture_points, double dt) {
	if (!dead) return;
	respawn_timer -= dt;

	if (respawn_timer <= 0.0) {
		dead = false;
		health = 100.0;
		state = State::Idle;

		// 自チームが支配している（所有している）拠点をリストアップ
		Array<const CapturePoint*> owned_cps;
		for (const auto& cp : capture_points) if (cp.owner == team) owned_cps.push_back(&cp);

		if (!owned_cps.isEmpty()) {
			// 所有拠点がある場合：その中からランダムに1つ選び、周囲に少し散らして（±60px）復活
			pos = Sample(owned_cps)->pos + Vec2(Random(-60, 60), Random(-60, 60));
		}
		else {
			// 所有拠点がない場合：チームごとの本拠地座標（初期位置）で復活
			pos = (team == Team::Blue) ? Vec2(200, 200) : Vec2(1800, 1800);
		}
	}
}
void Soldier::draw(const Vec2& camera, const Font& font) const {
	if (dead) return;	// 死亡中は描画しない
	Vec2 draw_pos = pos - camera;
	// 兵士
	Circle(draw_pos, 20).draw(color);
	// ステート：兵科
	font(U"{}:{} [{}]"_fmt(StateToString(state), RoleToString(role), weapon.name)).draw(11, draw_pos.x - 45, draw_pos.y + 25, Palette::White);
	// HPバー
	RectF(draw_pos.x - 20, draw_pos.y - 30, 40, 5).draw(Palette::Red);
	RectF(draw_pos.x - 20, draw_pos.y - 30, 40 * (Max(0.0, health) / 100.0), 5).draw(Palette::Green);
}

// --- Helpers ---
Array<Obstacle> generate_random_obstacles(int32 count) {
	Array<Obstacle> obstacles;
	for (int32 i = 0; i < count; ++i) {
		double w = Random(50, 300), h = Random(50, 300);
		// 50%の確率で「縦に細長い壁」か「横に平べったい壁」のどちらかに極端に変形させる
		if (RandomBool()) w = 50; else h = 50;
		// マップの端（スポーン地付近）を避けた中央寄りのエリアにランダム配置
		obstacles.emplace_back(Random(200.0, MAP_WIDTH - 200.0 - w), Random(200.0, MAP_HEIGHT - 200.0 - h), w, h);
	}
	return obstacles;
}
