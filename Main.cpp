# include "GameObjects.hpp"

// --- HUDの描画 ---
// チケット残量、各拠点の占領状況、およびゲームオーバー時の勝利画面
void draw_hud(const HashTable<Team, int32>& tickets, const Array<CapturePoint>& capture_points,
			  bool game_over, Team winner, const Font& fontHUD, const Font& fontCP, const Font& fontWin, const Font& fontSub) {
	// 上部バーと境界線
	Rect(0, 0, SCREEN_WIDTH, 55).draw(Color(20, 20, 20));
	Line(0, 55, SCREEN_WIDTH, 55).draw(2, Color(100, 100, 100));

	// チケット描画
	fontHUD(U"BLUE: {}"_fmt(tickets.at(Team::Blue))).draw(30, 18, Color(100, 100, 255));
	fontHUD(U"RED: {}"_fmt(tickets.at(Team::Red))).draw(SCREEN_WIDTH - 150, 18, Color(255, 100, 100));

	// 占領状況のインジケータ
	int32 start_x = SCREEN_WIDTH / 2 - ((int32)capture_points.size() * 50) / 2;
	for (size_t i = 0; i < capture_points.size(); ++i) {
		const auto& cp = capture_points[i];
		int32 x = start_x + (int32)i * 50 + 20;
		int32 y = 28;
		Color color = (cp.owner == Team::Blue) ? Color(50, 50, 255) : (cp.owner == Team::Red) ? Color(255, 50, 50) : Color(150, 150, 150);
		Circle(x, y, 16).draw(color).drawFrame(2, 0, Palette::White);
		fontCP(cp.name).draw(x - 6, y - 10, Palette::White);
	}

	// リザルト表示
	if (game_over) {
		Rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT).draw(ColorF(0, 0, 0, 0.78));
		fontWin((winner == Team::Blue) ? U"BLUE TEAM WINS!" : U"RED TEAM WINS!").drawAt(SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0 - 50, (winner == Team::Blue) ? Color(100, 100, 255) : Color(255, 100, 100));
		fontSub(U"Press SPACE to Restart").drawAt(SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0 + 30, Palette::White);
	}
}

// --- ミニマップの描画 ---
// 画面右上にマップ全体を縮小して表示
void draw_minimap(const Soldier& player, const Array<Soldier>& allies, const Array<Soldier>& enemies,
				  const Array<Obstacle>& obstacles, const Array<CapturePoint>& capture_points) {
	double m_size = 140.0;
	double mx = SCREEN_WIDTH - m_size - 20.0;
	double my = 75.0;
	// ミニパックの枠線と背景
	RectF(mx, my, m_size, m_size).draw(Color(15, 45, 15, 220)).drawFrame(2, 0, Color(200, 200, 200));
	double scale = m_size / MAP_WIDTH;

	// 各エンティティの描画
	for (const auto& wall : obstacles) RectF(mx + wall.rect.x * scale, my + wall.rect.y * scale, Max(1.0, wall.rect.w * scale), Max(1.0, wall.rect.h * scale)).draw(Color(90, 50, 20));
	for (const auto& cp : capture_points) Circle(mx + cp.pos.x * scale, my + cp.pos.y * scale, Max(2.0, cp.radius * scale)).drawFrame(1, 0, (cp.owner == Team::Blue) ? Color(50, 50, 255) : (cp.owner == Team::Red) ? Color(255, 50, 50) : Color(160, 160, 160));
	for (const auto& a : allies ) if (!a.dead && a.health > 0.0) Circle(mx + a.pos.x * scale, my + a.pos.y * scale, 2).draw(Color(100, 100, 255));
	for (const auto& e : enemies) if (!e.dead && e.health > 0.0) Circle(mx + e.pos.x * scale, my + e.pos.y * scale, 2).draw(Color(255, 50, 50));
	if (!player.dead && player.health > 0.0) Circle(mx + player.pos.x * scale, my + player.pos.y * scale, 3).draw(Palette::White);
}

// --- ゲーム状態のリセット ---
// マップ生成、オブジェクト配置、チケット等のデータ初期化
void reset_game(Array<Obstacle>& obstacles, Array<CapturePoint>& capture_points, Soldier& player,
				Array<Soldier>& allies, Array<Soldier>& enemies, Array<Bullet>& bullets, HashTable<Team, int32>& tickets) {
	obstacles = generate_random_obstacles(18);
	capture_points = { CapturePoint(400, 400, U"A"), CapturePoint(MAP_WIDTH / 2.0, MAP_HEIGHT / 2.0, U"B"), CapturePoint(1600, 1600, U"C") };
	player = Soldier(200, 200, Team::Blue, true);
	allies.clear();
	for (int i = 0; i < 9; ++i) {
		allies.emplace_back(Random(150, 350), Random(150, 350), Team::Blue);
	}
	enemies.clear();
	for (int i = 0; i < 12; ++i) {
		enemies.emplace_back(Random(1650, 1850), Random(1650, 1850), Team::Red);
	}
	bullets.clear();
	tickets = { {Team::Blue, 250}, {Team::Red, 250} };
}

// --- メイン関数 ---
void Main() {
	// ゲーム起動　ウィンドウ初期化、フォント読み込み、変数宣言
	Window::Resize(SCREEN_WIDTH, SCREEN_HEIGHT);
	Window::SetTitle(U"Battlefield: Conquest Mode");
	Scene::SetBackground(COLOR_BG);

	const Font fontCPText(40), fontState(20), fontHUD(28), fontCPHUD(22, Typeface::Bold), fontWin(74, Typeface::Bold), fontSub(36);
	Array<Obstacle> obstacles;
	Array<CapturePoint> capture_points;
	Soldier player(0, 0, Team::None);
	Array<Soldier> allies;
	Array<Soldier> enemies;
	Array<Bullet> bullets;
	Array<Grenade> grenades;
	HashTable<Team, int32> tickets;

	// マップ・障害物・拠点の配置、プレイヤー/AIの初期配置、初期チケットの設定
	reset_game(obstacles, capture_points, player, allies, enemies, bullets, tickets);
	double ticket_decay_timer = 0.0;
	bool game_over = false;
	Team winner = Team::None;
	double player_grenade_cd = 0.0;

	// 1フレームの始まり
	while (System::Update()) {
	// 1.入力の受付 & 特殊処理(Input & Global State)
		// カメラ位置
		double dt = Scene::DeltaTime();
		if (dt > 0.1) dt = 0.1;
		if (player_grenade_cd > 0.0) {
			player_grenade_cd -= dt;
		}
		Vec2 camera(Clamp<double>(player.pos.x - SCREEN_WIDTH / 2.0, 0.0, MAP_WIDTH - SCREEN_WIDTH), Clamp<double>(player.pos.y - SCREEN_HEIGHT / 2.0, 0.0, MAP_HEIGHT - SCREEN_HEIGHT));

		// プレイヤー射撃(元)
		//if (!player.dead && !game_over && MouseL.down()) {
		//	Vec2 world_mouse = Cursor::Pos() + camera;
		//	bullets.emplace_back(player.pos.x, player.pos.y, std::atan2(world_mouse.y - player.pos.y, world_mouse.x - player.pos.x), Team::Blue);
		//}
		// リスタート
		if (game_over && KeySpace.down()) {
			reset_game(obstacles, capture_points, player, allies, enemies, bullets, tickets);
			game_over = false; winner = Team::None; ticket_decay_timer = 0.0;
		}

	// 2. チケット・勝敗判定
		if (!game_over) {
			ticket_decay_timer += dt;
			if (ticket_decay_timer >= 1.0) {
				ticket_decay_timer = 0.0;
				int32 blue_cps = 0, red_cps = 0;
				for (const auto& cp : capture_points) {
					if (cp.owner == Team::Blue) {
						blue_cps++;
					}
					else if (cp.owner == Team::Red) {
						red_cps++;
					}
				}
				if (blue_cps > red_cps) {
					tickets[Team::Red] = Max(0, tickets[Team::Red] - (blue_cps - red_cps));
				}
				else if (red_cps > blue_cps) {
					tickets[Team::Blue] = Max(0, tickets[Team::Blue] - (red_cps - blue_cps));
				}
			}
			// どっちか0以下でゲームオーバー
			if (tickets[Team::Blue] <= 0) {
				game_over = true;
				winner = Team::Red;
			}
			else if (tickets[Team::Red] <= 0) {
				game_over = true;
				winner = Team::Blue;
			}
		}

	// 3. プレイヤーの移動
		if (!player.dead && player.health > 0.0 && !game_over ) {
			double dx = 0, dy = 0;
			if (KeyA.pressed()) dx -= 240.0 * dt;
			if (KeyD.pressed()) dx += 240.0 * dt;
			if (KeyW.pressed()) dy -= 240.0 * dt;
			if (KeyS.pressed()) dy += 240.0 * dt;
			if (KeyG.down() && player_grenade_cd <= 0.0 && !player.dead) {
				Vec2 target_pos = camera + Cursor::Pos();
				grenades.emplace_back(player.pos.x, player.pos.y, target_pos, Team::Blue);
				player_grenade_cd = 6.0; // プレイヤーのクールダウン6秒
			}
			// 障害物、マップ端の衝突判定
			if (dx != 0 || dy != 0) player.move_with_collision(dx, dy, obstacles);	 
		}

		// プレイヤーの射撃クールダウン更新
		if (player.shoot_cooldown > 0.0) {
			player.shoot_cooldown -= dt;
		}

		// セミ・フル切り替え
		bool wants_to_shoot = false;
		// フル
		if (player.weapon.type == WeaponType::AR || player.weapon.type == WeaponType::SMG) {
			wants_to_shoot = MouseL.pressed();
		}
		// セミ
		else {
			wants_to_shoot = MouseL.down();
		}

		// 射撃する
		//if (!player.dead && !game_over && wants_to_shoot && player.shoot_cooldown <= 0.0)
		if (!player.dead && player.health > 0.0 && !game_over && wants_to_shoot && player.shoot_cooldown <= 0.0) {
			Vec2 target_pos = camera + Cursor::Pos();
			Vec2 dir_vec = target_pos - player.pos;
			bullets.emplace_back(player.pos.x, player.pos.y, std::atan2(dir_vec.y, dir_vec.x), Team::Blue, player.weapon.damage);
			player.shoot_cooldown = player.weapon.fire_rate;
		}

	// 4. 拠点の更新
		// 全兵士を管理するポインタ配列作成
		Array<Soldier*> all_soldiers;
		all_soldiers.push_back(&player);


		for (auto& a : allies) {
			all_soldiers.push_back(&a);
		}
		for (auto& e : enemies) {
			all_soldiers.push_back(&e);
		}
		// 各拠点の人数判定、占領進捗更新
		if (!game_over) {
			for (auto& cp : capture_points) { 
				cp.update(all_soldiers, dt);
			}
		}

	// 5. AIの更新
		// 味方AI,敵AIそれぞれのポインタ配列作成
		Array<Soldier*> ptr_allies, ptr_enemies;
		for (auto& a : allies) {
			ptr_allies.push_back(&a);
		}
		for (auto& e : enemies) {
			ptr_enemies.push_back(&e);
		}

		// 味方AI更新
		for (auto& a : allies) {
			// 死亡してればリスポーンタイマーの進捗
			a.update_respawn(capture_points, dt);
			// 生存してれば思考と行動
			if (!a.dead && a.health > 0.0 && !game_over) {
				a.think_and_move(&player, ptr_enemies, Array<Soldier*>(), bullets, grenades, obstacles, capture_points, dt);
			}
			// 死亡時の処理
			//if (a.health <= 0 && !a.dead) {
			//	a.dead = true;
			//	a.respawn_timer = 3.0;
			//	tickets[Team::Blue] = Max(0, tickets[Team::Blue] - 1);
			//}
		}
		// 敵AI更新
		for (auto& e : enemies) {
			// 死亡してればリスポーンタイマーの進捗
			e.update_respawn(capture_points, dt);
			// 生存してれば思考と行動(敵は第3引数に味方ポインタのリスト渡す)
			if (!e.dead && e.health > 0.0 && !game_over) {
				e.think_and_move(&player, Array<Soldier*>(), ptr_allies, bullets, grenades, obstacles, capture_points, dt);
			}
			// 死亡時の処理
			//if (e.health <= 0 && !e.dead) {
			//	e.dead = true;
			//	e.respawn_timer = 3.0;
			//	tickets[Team::Red] = Max(0, tickets[Team::Red] - 1);
			//}
		}

		// プレイヤーのリスポーン処理
		//if (player.health <= 0 && !player.dead) {
		//	player.dead = true;
		//	player.respawn_timer = 3.0;
		//	tickets[Team::Blue] = Max(0, tickets[Team::Blue] - 1);
		//}
		player.update_respawn(capture_points, dt);

		// グレの更新と消去
		for (auto it = grenades.begin(); it != grenades.end();) {
			it->update(dt, all_soldiers, &player);
			if (it->timer >= (it->max_time + 0.25)) { // 爆発消去
				it = grenades.erase(it);
			}
			else {
				++it;
			}
		}

	// 6. 弾丸の更新&衝突判定
		if (!game_over) {
			for (auto it = bullets.begin(); it != bullets.end();) {
				it->update(dt);	// 速度に基づき移動

				// マップ外は削除
				if (it->pos.x < 0 || it->pos.x > MAP_WIDTH || it->pos.y < 0 || it->pos.y > MAP_HEIGHT) {
					it = bullets.erase(it);
					continue;
				}

				// 障害物との衝突判定
				bool hit_wall = false;
				for (const auto& wall : obstacles) {
					if (wall.rect.intersects(it->pos)) {
						hit_wall = true;
						break;
					}
				}
				if (hit_wall) {
					it = bullets.erase(it);
					continue;
				}

				// 兵士との衝突判定
				bool hit_soldier = false;
				for (auto target : all_soldiers) {
					if (!target->dead && target-> health > 0.0 && it->team != target->team && it->pos.distanceFrom(target->pos) < 20.0) {
						target->health -= it->damage;

						// ダメージを受けた時点でHP<=0なら即死
						if (target->health <= 0.0) {
							target->health = 0.0;
							target->dead = true;
							target->respawn_timer = 5.0;
							tickets[target->team] = Max(0, tickets[target->team] - 1);
						}

						hit_soldier = true;
						break;
					}
				}
				if (hit_soldier) {
					it = bullets.erase(it);
				}
				else {
					++it;
				}
			}
		}

	// 7. 画面描画
		// グリッド線(横)の描画：100px間隔
		for (int32 x = 0; x < MAP_WIDTH; x += 100) {
			double line_x = x - camera.x;
			if (0 <= line_x && line_x <= SCREEN_WIDTH) {
				Line(line_x, 0, line_x, SCREEN_HEIGHT).draw(COLOR_GRID);
			}
		}
		// グリッド線(縦)の描画：100px間隔
		for (int32 y = 0; y < MAP_HEIGHT; y += 100) {
			double line_y = y - camera.y;
			if (0 <= line_y && line_y <= SCREEN_HEIGHT) {
				Line(0, line_y, SCREEN_WIDTH, line_y).draw(COLOR_GRID);
			}
		}

		// 各種オブジェクトをカメラ位置からの相対座標で描画
		for (const auto& cp : capture_points) {
			cp.draw(camera, fontCPText);
		}
		for (const auto& wall : obstacles) {
			wall.draw(camera);
		}
		player.draw(camera, fontState);
		for (const auto& a : allies) {
			a.draw(camera, fontState);
		}
		for (const auto& e : enemies) {
			e.draw(camera, fontState);
		}
		for (const auto& b : bullets) {
			b.draw(camera);
		}
		for (const auto& g : grenades) {
			g.draw(camera);
		}

		// HUD, ミニマップ描画(カメラの影響受けない最前面固定レイヤー)
		draw_hud(tickets, capture_points, game_over, winner, fontHUD, fontCPHUD, fontWin, fontSub);
		draw_minimap(player, allies, enemies, obstacles, capture_points);
	}
}
