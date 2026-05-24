with open('src/main.cpp', 'r') as f:
    lines = f.readlines()

start_idx = 1690
end_idx = 1732

new_block = r"""            if (show_guide_popup) ImGui::OpenPopup(tr("Game Guide", "Cam Nang Tro Choi"));
            if (ImGui::BeginPopupModal(tr("Game Guide", "Cam Nang Tro Choi"), &show_guide_popup, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::SetNextWindowSizeConstraints(ImVec2(700, 0), ImVec2(700, 580));
                ImGui::BeginChild("##guide_scroll", ImVec2(680, 520), false, ImGuiWindowFlags_HorizontalScrollbar);

                // ── STORY ──────────────────────────────────────────────────────────────
                ImGui::TextColored(ImVec4(0.95f, 0.9f, 0.35f, 1.0f), "%s", tr("ZOMCHESS — GAME WIKI", "ZOMCHESS — CAM NANG TRO CHOI"));
                ImGui::Spacing();
                if (ImGui::CollapsingHeader(tr("Story", "Boi Canh"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::TextWrapped("%s", tr(
                        "Year 2047. A rogue bioweapon experiment in the Arctic Circle shattered containment. The undead plague spread faster than any evacuation could handle. You are the last field operative of ECHO Squad — cut off, alone, and surrounded. Your only objective: eliminate every hostile before the extraction window closes. The battlefield is alive — fire spreads, storms roll in, and the dead keep coming.",
                        "Nam 2047. Mot thi nghiem vu khi sinh hoc bi mat o Bac Cuc mat kiem soat. Dich benh zombie lan rong truoc khi bat ky cuoc di tan nao co the thuc hien. Ban la chien binh cuoi cung cua Doi ECHO — bi cat dut lien lac, don doc, va bi bao vay. Muc tieu duy nhat: tieu diet moi ke thu truoc khi cua so thoat hiem dong lai. Chien truong song dong — lua lan, bao to ap den, va nhung ke chet van tiep tuc tien."
                    ));
                }
                ImGui::Spacing();

                // ── BASIC GAMEPLAY ─────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Basic Gameplay & Controls", "Huong Dan Co Ban & Dieu Khien"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Turn Structure", "Cau Truc Luot"));
                    ImGui::BulletText("%s", tr("Each round: Human acts first, then all Zombies act, then the Environment phase.", "Moi vong: Human hanh dong truoc, sau do tat ca Zombie hanh dong, cuoi cung la pha Moi Truong."));
                    ImGui::BulletText("%s", tr("Win: eliminate all zombies. Lose: HP reaches 0 or turn limit exceeded.", "Thang: tieu diet het zombie. Thua: HP ve 0 hoac het gioi han luot."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Stamina System", "He Thong The Luc"));
                    ImGui::BulletText("%s", tr("At the start of each Human turn, stamina is randomly rolled 1-6 (uniform).", "Dau moi luot Human, the luc duoc tung ngau nhien 1-6 (dong deu)."));
                    ImGui::BulletText("%s", tr("Moving to an adjacent tile costs 1 stamina (2 if the tile is Water).", "Di chuyen sang o ke ben ton 1 the luc (2 neu o do la Nuoc)."));
                    ImGui::BulletText("%s", tr("Every weapon action costs 1 stamina. Running out ends your turn automatically.", "Moi hanh dong vu khi ton 1 the luc. Het the luc tu dong ket thuc luot."));
                    ImGui::BulletText("%s", tr("If Paralyzed, stamina is set to 0 for that turn.", "Neu bi Te Liet, the luc ve 0 trong luot do."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Controls", "Dieu Khien"));
                    ImGui::BulletText("%s", tr("Left-click an adjacent tile (8-directional) to move.", "Click trai vao o ke ben (8 huong) de di chuyen."));
                    ImGui::BulletText("%s", tr("Select a weapon button, then left-click a target tile or direction.", "Chon nut vu khi, sau do click trai vao o muc tieu hoac huong ban."));
                    ImGui::BulletText("%s", tr("END TURN button: manually end your turn and pass to zombies.", "Nut KET THUC LUOT: ket thuc luot thu cong va chuyen sang zombie."));
                    ImGui::BulletText("%s", tr("RETURN HUB: return to main menu (confirmation required).", "RETURN HUB: quay ve menu chinh (can xac nhan)."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("In-Game Panel", "Bang Thong Tin In-Game"));
                    ImGui::BulletText("%s", tr("Top bar: current turn / turn limit, HP bar, stamina bar.", "Thanh tren: luot hien tai / gioi han luot, thanh HP, thanh the luc."));
                    ImGui::BulletText("%s", tr("Weapon buttons show remaining ammo/count. Greyed out = unavailable.", "Cac nut vu khi hien so dan/so luong con lai. Mo nhat = khong the dung."));
                    ImGui::BulletText("%s", tr("Status icons (B/P/F/S) appear next to HP when active.", "Bieu tuong trang thai (B/P/F/S) hien ben canh HP khi kich hoat."));
                    ImGui::BulletText("%s", tr("Combat log on the right shows all events in real time.", "Nhat ky chien dau ben phai hien tat ca su kien theo thoi gian thuc."));
                    ImGui::BulletText("%s", tr("Music and SFX toggles are in the top-right of the panel.", "Nut bat/tat nhac va am thanh o goc tren phai cua bang."));
                }
                ImGui::Spacing();

                // ── ZOMBIE TYPES ───────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Zombie Types", "Cac Loai Zombie"))) {
                    ImGui::TextColored(ImVec4(0.45f, 0.9f, 0.45f, 1.0f), "%s", tr("Normal Zombie  [HP: 2]", "Zombie Thuong  [HP: 2]"));
                    ImGui::BulletText("%s", tr("Moves 1 tile per turn (8-directional, weighted toward Human).", "Di chuyen 1 o moi luot (8 huong, uu tien huong Human)."));
                    ImGui::BulletText("%s", tr("Orthogonal attack: Bite -2 HP. Diagonal attack: Scratch -1 HP.", "Tan cong ngang/doc: Can -2 HP. Tan cong cheo: Cao -1 HP."));
                    ImGui::BulletText("%s", tr("In Water: Bite deals only -1 HP; Scratch deals 0 HP.", "Trong Nuoc: Can chi gay -1 HP; Cao gay 0 HP."));
                    ImGui::BulletText("%s", tr("AI uses exponential weighting (lambda=1.2) — mostly advances, rarely retreats.", "AI dung trong so mu (lambda=1.2) — chu yeu tien, hiem khi lui."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "%s", tr("Fast Sprinter  [HP: 2]", "Zombie Nhanh  [HP: 2]"));
                    ImGui::BulletText("%s", tr("Takes 2 move+attack actions per turn instead of 1.", "Co 2 hanh dong di chuyen+tan cong moi luot thay vi 1."));
                    ImGui::BulletText("%s", tr("In Water: capped to 1 action per turn (same as Normal).", "Trong Nuoc: bi gioi han con 1 hanh dong moi luot (giong Thuong)."));
                    ImGui::BulletText("%s", tr("When Frozen: thaws after only 1 turn (others need 2 turns).", "Khi bi Dong Bang: tu giai sau 1 luot (loai khac can 2 luot)."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.1f, 1.0f), "%s", tr("Exploding Zombie  [HP: 3]", "Zombie No  [HP: 3]"));
                    ImGui::BulletText("%s", tr("On death (any cause): triggers explosion radius 1 — center 3 dmg, ring 2 dmg.", "Khi chet (bat ky nguyen nhan): kich no ban kinh 1 — tam 3 sat thuong, vong ngoai 2 sat thuong."));
                    ImGui::BulletText("%s", tr("Chain reaction: if another Exploder is in blast range, it also detonates.", "Phan ung day chuyen: neu Zombie No khac trong vung no, no cung phat no."));
                    ImGui::BulletText("%s", tr("Explosion radius reduced by 1 if standing on Water or Ice.", "Ban kinh no giam 1 neu dang dung tren Nuoc hoac Bang."));
                    ImGui::BulletText("%s", tr("Blast destroys Walls, melts Ice into Water, and destroys loot drops.", "Vu no pha Tuong, tan Bang thanh Nuoc, va pha huy loot drop."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.75f, 0.2f, 0.75f, 1.0f), "%s", tr("Vampire Zombie  [HP: 4]", "Zombie Hut Mau  [HP: 4]"));
                    ImGui::BulletText("%s", tr("After each successful attack, heals +1 HP (no cap shown, but max_hp is 4).", "Sau moi don tan cong thanh cong, hoi phuc +1 HP."));
                    ImGui::BulletText("%s", tr("Same attack pattern as Normal (Bite/Scratch). Highest base HP of all zombies.", "Cung kieu tan cong nhu Thuong (Can/Cao). HP co ban cao nhat."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.85f, 0.9f, 0.2f, 1.0f), "%s", tr("Sick Zombie  [HP: 2]", "Zombie Benh  [HP: 2]"));
                    ImGui::BulletText("%s", tr("On orthogonal Bite: halves remaining turns AND inflicts -1 stamina next turn.", "Khi Can ngang/doc: giam nua so luot con lai VA tru -1 stamina luot sau."));
                    ImGui::BulletText("%s", tr("Diagonal Scratch does NOT trigger infection — only direct Bite does.", "Cao cheo KHONG gay nhiem — chi Can truc tiep moi gay nhiem."));
                    ImGui::BulletText("%s", tr("Multiple bites stack: each halves the remaining turns again.", "Nhieu lan can chong chong: moi lan lai giam nua so luot con lai."));
                }
                ImGui::Spacing();

                // ── WEAPONS ───────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Weapons", "Vu Khi"))) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.7f, 1.0f), "%s", tr("Knife  [Cost: 1 stamina, unlimited]", "Dao  [Chi phi: 1 the luc, khong gioi han]"));
                    ImGui::BulletText("%s", tr("Melee: click an adjacent tile (range 1, 8-directional). Deals -1 HP.", "Cong cu: click o ke ben (tam 1, 8 huong). Gay -1 HP."));
                    ImGui::BulletText("%s", tr("No ammo limit. Reliable for finishing low-HP zombies.", "Khong gioi han dan. Dang tin cay de ket lieu zombie it HP."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.35f, 1.0f), "%s", tr("Pistol  [Cost: 1 stamina + 1 ammo]", "Sung Luc  [Chi phi: 1 the luc + 1 dan]"));
                    ImGui::BulletText("%s", tr("Fires in 8 directions, range up to 20 tiles. Deals -1 HP on hit.", "Ban theo 8 huong, tam toi da 20 o. Gay -1 HP khi trung."));
                    ImGui::BulletText("%s", tr("Accuracy: exponential decay with distance (lambda=4.32). Point-blank ~100%%, long range ~10%%.", "Do chinh xac: giam mu theo khoang cach (lambda=4.32). Gan ~100%%, xa ~10%%."));
                    ImGui::BulletText("%s", tr("Bullet stops at first zombie hit or Wall. Passes through empty tiles.", "Dan dung lai o zombie dau tien trung hoac Tuong. Xuyen qua o trong."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "%s", tr("Shotgun  [Cost: 1 stamina + 1 shell]", "Sung Hoa Mai  [Chi phi: 1 the luc + 1 vien]"));
                    ImGui::BulletText("%s", tr("Straight shot: expanding cone 3 steps deep (up to 9 cells). Diagonal: triangular area.", "Ban thang: hinh non mo rong 3 buoc (toi da 9 o). Cheo: vung tam giac."));
                    ImGui::BulletText("%s", tr("Each zombie in cone takes -1 HP, then pushed 1 tile in fire direction.", "Moi zombie trong vung chiu -1 HP, sau do bi day 1 o theo huong ban."));
                    ImGui::BulletText("%s", tr("If pushed into Wall or another zombie: +1 collision damage to both.", "Neu bi day vao Tuong hoac zombie khac: +1 sat thuong va cham cho ca hai."));
                    ImGui::BulletText("%s", tr("Recoil: Human pushed 1 tile backward. If blocked by Wall: Wall destroyed, Human -1 HP.", "Giat lui: Human bi day 1 o ra sau. Neu bi chan boi Tuong: Tuong bi pha, Human -1 HP."));
                    ImGui::BulletText("%s", tr("Shotgun blast destroys loot drops in its cone area.", "Dan shotgun pha huy loot drop trong vung ban."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", tr("Grenade  [Cost: 1 stamina + 1 grenade]", "Luu Dan  [Chi phi: 1 the luc + 1 qua]"));
                    ImGui::BulletText("%s", tr("Thrown in 8 directions. Travels 1-6 tiles randomly (stops before Walls).", "Nem theo 8 huong. Bay 1-6 o ngau nhien (dung truoc Tuong)."));
                    ImGui::BulletText("%s", tr("Detonates after 1 zombie phase. Blast radius 2: center -3 HP, ring -2 HP, outer -1 HP.", "No sau 1 pha zombie. Ban kinh 2: tam -3 HP, vong giua -2 HP, ngoai -1 HP."));
                    ImGui::BulletText("%s", tr("Entities at ring/outer are pushed outward; if blocked, +1 impact damage.", "Thuc the o vong giua/ngoai bi day ra; neu bi chan, +1 sat thuong va cham."));
                    ImGui::BulletText("%s", tr("Wind can blow a live grenade 1 tile in wind direction before it detonates.", "Gio co the thoi luu dan dang bay 1 o theo huong gio truoc khi no."));
                    ImGui::BulletText("%s", tr("Thrown into own feet (all walls ahead): explodes immediately under Human.", "Nem vao chan minh (tat ca phia truoc la Tuong): no ngay duoi chan Human."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.1f, 1.0f), "%s", tr("Molotov  [Cost: 1 stamina + 1 molotov]", "Bom Xang  [Chi phi: 1 the luc + 1 chai]"));
                    ImGui::BulletText("%s", tr("Thrown in 8 directions. Travels 1-6 tiles randomly (stops before Walls).", "Nem theo 8 huong. Bay 1-6 o ngau nhien (dung truoc Tuong)."));
                    ImGui::BulletText("%s", tr("Lands on Water: fizzles out — no fire, no heat transfer.", "Roi xuong Nuoc: tat ngay — khong co lua, khong truyen nhiet."));
                    ImGui::BulletText("%s", tr("Lands on Dirt/Forest: creates Fire cell (lasts 2 turns). Melts adjacent Ice (8-dir).", "Roi xuong Dat/Rung: tao o Lua (ton 2 luot). Tan Bang ke canh (8 huong)."));
                    ImGui::BulletText("%s", tr("Lands on Frozen entity: unfreezes it, no fire spread.", "Roi trung thuc the Dong Bang: giai bang, khong lan lua."));
                    ImGui::BulletText("%s", tr("Lands on Ice: melts that cell + all adjacent Ice (8-dir) into Water.", "Roi xuong Bang: tan o do + tat ca Bang ke canh (8 huong) thanh Nuoc."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "%s", tr("Mine  [Cost: 0 stamina, placed on current tile]", "Min  [Chi phi: 0 the luc, dat tai o hien tai]"));
                    ImGui::BulletText("%s", tr("Placed invisibly on Human's current tile. Triggers when any entity steps on it.", "Dat an tren o Human dang dung. Kich hoat khi bat ky thuc the buoc len."));
                    ImGui::BulletText("%s", tr("Explosion radius 1: same damage as grenade (center 3, ring 2, outer 1).", "Ban kinh no 1: sat thuong giong luu dan (tam 3, vong 2, ngoai 1)."));
                }
                ImGui::Spacing();

                // ── TERRAIN ───────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Terrain Types", "Cac Loai Dia Hinh"))) {
                    ImGui::TextColored(ImVec4(0.75f, 0.6f, 0.4f, 1.0f), "%s", tr("Dirt", "Dat"));
                    ImGui::BulletText("%s", tr("Default terrain. No movement penalty. Molotov/lightning can ignite it.", "Dia hinh mac dinh. Khong phat sinh them chi phi. Molotov/set co the dot chay."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.25f, 0.55f, 1.0f, 1.0f), "%s", tr("Water", "Nuoc"));
                    ImGui::BulletText("%s", tr("Human movement costs 2 stamina. Zombies (except Sprinter) unaffected.", "Di chuyen cua Human ton 2 the luc. Zombie (tru Sprinter) khong bi anh huong."));
                    ImGui::BulletText("%s", tr("Sprinter capped to 1 action/turn in Water.", "Sprinter bi gioi han con 1 hanh dong/luot trong Nuoc."));
                    ImGui::BulletText("%s", tr("Extinguishes Burning status on any entity that enters.", "Dap tat trang thai Chay cua bat ky thuc the buoc vao."));
                    ImGui::BulletText("%s", tr("Zombie attacks in Water: Bite -1 HP (not 2), Scratch 0 HP.", "Zombie tan cong trong Nuoc: Can -1 HP (khong phai 2), Cao 0 HP."));
                    ImGui::BulletText("%s", tr("Molotov landing here fizzles. Explosion radius reduced by 1 here.", "Molotov roi vao day se tat. Ban kinh no giam 1 tai day."));
                    ImGui::BulletText("%s", tr("Loot drops on Water cannot be picked up until the tile changes.", "Loot drop tren Nuoc khong the nhat cho den khi o thay doi."));
                    ImGui::BulletText("%s", tr("Heatwave: isolated water cells may evaporate to Dirt.", "Nang Nong: o nuoc co lap co the boc hoi thanh Dat."));
                    ImGui::BulletText("%s", tr("Blizzard: water clusters may freeze into Ice.", "Bao Tuyet: cum nuoc co the dong bang thanh Bang."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "%s", tr("Wall", "Tuong"));
                    ImGui::BulletText("%s", tr("Impassable for all entities. Blocks projectiles (pistol, shotgun).", "Khong the di qua voi moi thuc the. Chan dan (sung luc, hoa mai)."));
                    ImGui::BulletText("%s", tr("Shotgun recoil into Wall: Wall destroyed, Human -1 HP.", "Giat lui shotgun vao Tuong: Tuong bi pha, Human -1 HP."));
                    ImGui::BulletText("%s", tr("Explosion destroys adjacent Walls.", "Vu no pha Tuong ke canh."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.25f, 0.75f, 0.25f, 1.0f), "%s", tr("Forest", "Rung"));
                    ImGui::BulletText("%s", tr("Normal movement. Flammable: adjacent Fire spreads here each fire-spread event.", "Di chuyen binh thuong. De chay: Lua ke canh lan vao day moi su kien lan lua."));
                    ImGui::BulletText("%s", tr("Lightning on Forest with no entity: ignites to Fire. With entity: suppressed.", "Set danh Rung khong co thuc the: bat lua. Co thuc the: bi uc che."));
                    ImGui::BulletText("%s", tr("Heatwave: 15%% chance each Forest cell dies (becomes Dirt).", "Nang Nong: 15%% kha nang moi o Rung chet (thanh Dat)."));
                    ImGui::BulletText("%s", tr("Rain: 12%% chance Forest spreads to adjacent Dirt.", "Mua: 12%% kha nang Rung lan sang Dat ke canh."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.1f, 1.0f), "%s", tr("Fire", "Lua"));
                    ImGui::BulletText("%s", tr("Lasts 2 turns, then reverts to Dirt.", "Ton tai 2 luot, sau do chuyen lai thanh Dat."));
                    ImGui::BulletText("%s", tr("Entity entering Fire: gains Burning status.", "Thuc the buoc vao Lua: nhan trang thai Chay."));
                    ImGui::BulletText("%s", tr("Entity already Burning entering Fire: -1 HP immediately.", "Thuc the dang Chay buoc vao Lua: -1 HP ngay lap tuc."));
                    ImGui::BulletText("%s", tr("Spreads to adjacent Forest (4-dir) at: end of Human turn, end of all Zombie turns, end of Environment phase.", "Lan sang Rung ke canh (4 huong) vao: cuoi luot Human, cuoi tat ca luot Zombie, cuoi pha Moi Truong."));
                    ImGui::BulletText("%s", tr("Melts adjacent Ice (4-dir) into Water each spread event.", "Tan Bang ke canh (4 huong) thanh Nuoc moi su kien lan."));
                    ImGui::BulletText("%s", tr("Loot drops on Fire are destroyed immediately.", "Loot drop tren Lua bi pha huy ngay lap tuc."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", tr("Ice", "Bang"));
                    ImGui::BulletText("%s", tr("Normal movement cost. Entity standing on Water when it freezes gets Frozen status.", "Chi phi di chuyen binh thuong. Thuc the dang dung tren Nuoc khi dong bang nhan trang thai Dong Bang."));
                    ImGui::BulletText("%s", tr("Ice slide: if >= 4 consecutive Ice tiles ahead in move direction, 50%% chance to slide.", "Truot bang: neu >= 4 o Bang lien tiep phia truoc theo huong di, 50%% kha nang truot."));
                    ImGui::BulletText("%s", tr("Slide ends at first non-Ice tile or map edge. Collision with Wall/entity: Stunned.", "Truot dung o o khong phai Bang dau tien hoac bien ban do. Va cham voi Tuong/thuc the: bi Choang."));
                    ImGui::BulletText("%s", tr("Adjacent to Fire (4-dir): melts to Water each fire-spread event.", "Ke canh Lua (4 huong): tan thanh Nuoc moi su kien lan lua."));
                    ImGui::BulletText("%s", tr("Loot drops on Ice cannot be picked up until the tile changes.", "Loot drop tren Bang khong the nhat cho den khi o thay doi."));
                    ImGui::BulletText("%s", tr("Explosion, lightning, or molotov heat melts Ice to Water and unfreezes entities.", "Vu no, set, hoac nhiet molotov tan Bang thanh Nuoc va giai bang thuc the."));
                }
                ImGui::Spacing();

                // ── WEATHER ───────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Weather Events", "Cac Su Kien Thoi Tiet"))) {
                    ImGui::TextWrapped("%s", tr("One weather event occurs each turn (after all zombies act). Probabilities are configurable in Hub.", "Mot su kien thoi tiet xay ra moi luot (sau khi tat ca zombie hanh dong). Xac suat co the chinh trong Hub."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.9f, 0.95f, 1.0f, 1.0f), "%s", tr("Clear Skies", "Troi Quang"));
                    ImGui::BulletText("%s", tr("Nothing happens. Default ~50%% probability.", "Khong co gi xay ra. Xac suat mac dinh ~50%%."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.65f, 0.85f, 1.0f, 1.0f), "%s", tr("Windstorm", "Bao Gio"));
                    ImGui::BulletText("%s", tr("Blows in a random cardinal/diagonal direction. Pushes all non-frozen entities 1 tile.", "Thoi theo huong ngau nhien. Day tat ca thuc the khong bi Dong Bang 1 o."));
                    ImGui::BulletText("%s", tr("Entity must have open space both behind and ahead to be pushed (no pinning).", "Thuc the phai co khoang trong ca phia sau lan phia truoc moi bi day (khong bi kem)."));
                    ImGui::BulletText("%s", tr("Also blows live grenades and loot drops 1 tile in wind direction.", "Cung thoi luu dan dang bay va loot drop 1 o theo huong gio."));
                    ImGui::BulletText("%s", tr("Frozen entities are immune to wind push.", "Thuc the Dong Bang mien nhiem voi gio."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "%s", tr("Heavy Rain", "Mua To"));
                    ImGui::BulletText("%s", tr("Extinguishes all Burning entities (Human and Zombies).", "Dap tat trang thai Chay cua tat ca thuc the (Human va Zombie)."));
                    ImGui::BulletText("%s", tr("30%% chance each Dirt adjacent to Water floods to Water.", "30%% kha nang moi o Dat ke canh Nuoc bi ngap thanh Nuoc."));
                    ImGui::BulletText("%s", tr("10%% chance isolated Dirt cells flood to Water.", "10%% kha nang o Dat co lap bi ngap thanh Nuoc."));
                    ImGui::BulletText("%s", tr("12%% chance each Forest spreads to adjacent Dirt.", "12%% kha nang moi o Rung lan sang Dat ke canh."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.7f, 1.0f), "%s", tr("Dark Clouds", "May Den"));
                    ImGui::BulletText("%s", tr("Activates Dark Cloud overlay. Reduces pistol effective range (visual effect).", "Kich hoat lop phu May Den. Giam tam nhin hieu qua cua sung luc (hieu ung hinh anh)."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.35f, 1.0f), "%s", tr("Lightning Strike", "Set Danh"));
                    ImGui::BulletText("%s", tr("Picks a random cell weighted by: Water x4, entity-occupied x2, default x1.", "Chon o ngau nhien theo trong so: Nuoc x4, co thuc the x2, mac dinh x1."));
                    ImGui::BulletText("%s", tr("Direct strike: -1 HP to entity on that cell. Unfreezes Frozen entity on direct hit.", "Danh truc tiep: -1 HP cho thuc the tren o do. Giai bang thuc the Dong Bang bi danh truc tiep."));
                    ImGui::BulletText("%s", tr("Electricity spreads through connected Water/Ice cluster: all occupants Paralyzed.", "Dien lan qua cum Nuoc/Bang lien ket: tat ca thuc the trong cum bi Te Liet."));
                    ImGui::BulletText("%s", tr("Forest struck with no entity: ignites to Fire. With entity: fire suppressed.", "Rung bi set khong co thuc the: bat lua. Co thuc the: lua bi uc che."));
                    ImGui::BulletText("%s", tr("Loot drop on the struck cell is destroyed.", "Loot drop tren o bi set bi pha huy."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "%s", tr("Heatwave", "Nang Nong Gay Gat"));
                    ImGui::BulletText("%s", tr("All Ice melts to Water. Entities on melted Ice are unfrozen.", "Tat ca Bang tan thanh Nuoc. Thuc the tren Bang tan duoc giai bang."));
                    ImGui::BulletText("%s", tr("Isolated Water cells evaporate to Dirt (probability decreases with water neighbors).", "O Nuoc co lap boc hoi thanh Dat (xac suat giam khi co nhieu Nuoc ke canh)."));
                    ImGui::BulletText("%s", tr("15%% chance each Forest cell dies (becomes Dirt) from drought.", "15%% kha nang moi o Rung chet (thanh Dat) vi han han."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", tr("Blizzard", "Bao Tuyet"));
                    ImGui::BulletText("%s", tr("25%% chance each Water cell seeds a freeze. Entire connected Water cluster freezes.", "25%% kha nang moi o Nuoc khoi dong dong bang. Toan bo cum Nuoc lien ket dong bang."));
                    ImGui::BulletText("%s", tr("Entities standing on newly frozen cells gain Frozen status.", "Thuc the dang dung tren o vua dong bang nhan trang thai Dong Bang."));
                }
                ImGui::Spacing();

                // ── STATUS EFFECTS ────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Status Effects", "Trang Thai Hieu Ung"))) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.1f, 1.0f), "%s", tr("[B] Burning", "[B] Chay"));
                    ImGui::BulletText("%s", tr("Gained by: entering Fire tile, proximity to fire source, or molotov hit.", "Nhan duoc khi: buoc vao o Lua, gan nguon lua, hoac bi molotov danh trung."));
                    ImGui::BulletText("%s", tr("Effect: at start of next zombie phase, entity takes -1 HP and Burning is cleared.", "Hieu ung: dau pha zombie tiep theo, thuc the chiu -1 HP va Chay bi xoa."));
                    ImGui::BulletText("%s", tr("Entering Fire while already Burning: -1 HP immediately.", "Buoc vao Lua khi dang Chay: -1 HP ngay lap tuc."));
                    ImGui::BulletText("%s", tr("Cured by: stepping into Water, or Heavy Rain event.", "Chua bang: buoc vao Nuoc, hoac su kien Mua To."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.1f, 1.0f), "%s", tr("[P] Paralyzed", "[P] Te Liet"));
                    ImGui::BulletText("%s", tr("Human: stamina set to 0 for that turn (cannot act).", "Human: the luc ve 0 trong luot do (khong the hanh dong)."));
                    ImGui::BulletText("%s", tr("Zombie: loses its entire action for that turn.", "Zombie: mat toan bo hanh dong trong luot do."));
                    ImGui::BulletText("%s", tr("Caused by: electricity spreading through conductive Water/Ice cluster.", "Nguyen nhan: dien lan qua cum Nuoc/Bang dan dien."));
                    ImGui::BulletText("%s", tr("Only the direct lightning strike cell unfreezes Frozen entities — conductive spread does NOT.", "Chi o bi set danh truc tiep moi giai bang — lan dien KHONG giai bang."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "%s", tr("[F] Frozen", "[F] Dong Bang"));
                    ImGui::BulletText("%s", tr("Gained by: standing on Water when Blizzard freezes it.", "Nhan duoc khi: dang dung tren Nuoc khi Bao Tuyet dong bang no."));
                    ImGui::BulletText("%s", tr("Human: costs 2 stamina to break free (click any tile). If < 2 stamina, turn is skipped.", "Human: ton 2 the luc de tu giai (click bat ky o). Neu < 2 the luc, bo qua luot."));
                    ImGui::BulletText("%s", tr("Zombie: loses action each turn. Normal/Vampire/Sick/Exploding thaw after 2 turns. Sprinter after 1 turn.", "Zombie: mat hanh dong moi luot. Thuong/Hut Mau/Benh/No giai sau 2 luot. Sprinter sau 1 luot."));
                    ImGui::BulletText("%s", tr("Frozen entities cannot be pushed by Wind.", "Thuc the Dong Bang mien nhiem voi Gio."));
                    ImGui::BulletText("%s", tr("Cleared by: ice melting (any cause), explosion blast, lightning direct hit, molotov heat.", "Giai bang: bang tan (bat ky nguyen nhan), vu no, set danh truc tiep, nhiet molotov."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "%s", tr("[S] Stunned", "[S] Choang"));
                    ImGui::BulletText("%s", tr("Caused by: ice slide collision with Wall or entity at end of slide.", "Nguyen nhan: truot bang va cham voi Tuong hoac thuc the cuoi duong truot."));
                    ImGui::BulletText("%s", tr("Effect: entity loses all remaining actions for that turn.", "Hieu ung: thuc the mat tat ca hanh dong con lai trong luot do."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.82f, 0.93f, 0.24f, 1.0f), "%s", tr("[I] Infected (Sick Zombie bite)", "[I] Nhiem Doc (Zombie Benh can)"));
                    ImGui::BulletText("%s", tr("Halves remaining turns immediately (e.g. 10 left -> 5 left).", "Giam nua so luot con lai ngay lap tuc (vd: con 10 -> con 5)."));
                    ImGui::BulletText("%s", tr("Also inflicts -1 stamina penalty at the start of the next Human turn.", "Cung gay phat -1 the luc dau luot Human tiep theo."));
                    ImGui::BulletText("%s", tr("Multiple bites stack: each halves the remaining turns again.", "Nhieu lan can chong chong: moi lan lai giam nua so luot con lai."));
                }
                ImGui::Spacing();

                // ── LOOT DROPS ────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Loot Drops", "Loot Drop"))) {
                    ImGui::TextWrapped("%s", tr("Every zombie leaves a '?' loot drop on death. Walk onto it to collect.", "Moi zombie de lai loot drop '?' khi chet. Di chuyen vao o do de nhat."));
                    ImGui::BulletText("%s", tr("55%% Junk (useless). 45%% useful items split by rarity.", "55%% Rac (vo dung). 45%% do huu ich phan chia theo do hiem."));
                    ImGui::BulletText("%s", tr("Items: Health Potion +2 HP, Stamina Potion (restore to 6), Pistol Ammo +3, Shotgun Shell +1, Grenade +1, Molotov +1, Mine +1.", "Do: Binh Mau +2 HP, Binh The Luc (phuc hoi ve 6), Dan Luc +3, Dan Hoa Mai +1, Luu Dan +1, Bom Xang +1, Min +1."));
                    ImGui::BulletText("%s", tr("Cannot pick up on Water or Ice tiles. Pickup resumes when tile changes to Dirt/Forest.", "Khong the nhat tren o Nuoc hoac Bang. Co the nhat khi o chuyen thanh Dat/Rung."));
                    ImGui::BulletText("%s", tr("Destroyed by: explosion blast, shotgun cone, lightning direct hit, tile becoming Fire, or Wind blowing it into Fire.", "Bi pha huy boi: vu no, vung shotgun, set danh truc tiep, o chuyen thanh Lua, hoac Gio thoi vao Lua."));
                    ImGui::BulletText("%s", tr("Wind blows loot drops 1 tile (same as grenades). Frozen loot is NOT immune to wind.", "Gio thoi loot drop 1 o (giong luu dan). Loot KHONG mien nhiem voi gio."));
                }
                ImGui::Spacing();

                // ── SETUP & SHARING ───────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Game Setup & .zom Files", "Thiet Lap & File .zom"))) {
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Quick Play", "Choi Nhanh"));
                    ImGui::BulletText("%s", tr("4 presets: Easy / Medium / Hard / Unfair. Each sets HP, stamina, ammo, zombie counts, terrain ratios, and weather probabilities.", "4 che do: De / Trung Binh / Kho / Bat Cong. Moi che do thiet lap HP, the luc, dan, so luong zombie, ti le dia hinh va xac suat thoi tiet."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Custom Setup", "Thiet Lap Tuy Chinh"));
                    ImGui::BulletText("%s", tr("Adjust map size (up to 25x25), all terrain ratios, weather probabilities, zombie counts, and Human stats.", "Chinh kich thuoc ban do (toi da 25x25), ti le dia hinh, xac suat thoi tiet, so luong zombie va chi so Human."));
                    ImGui::BulletText("%s", tr("Map Editor: paint terrain tile-by-tile and place Human spawn manually.", "Trinh chinh sua ban do: to mau dia hinh tung o va dat vi tri xuat hien Human thu cong."));
                    ImGui::BulletText("%s", tr("Spawn Shield: protects a 5x5 area around Human spawn from zombie spawns.", "Khien Xuat Hien: bao ve vung 5x5 quanh vi tri xuat hien Human khoi zombie."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr(".zom Challenge Files", "File Thach Thuc .zom"));
                    ImGui::BulletText("%s", tr("Export: saves all current settings (map, stats, weather, terrain) to a .zom text file.", "Xuat: luu tat ca cai dat hien tai (ban do, chi so, thoi tiet, dia hinh) vao file .zom."));
                    ImGui::BulletText("%s", tr("Import: loads a .zom file to recreate the exact same challenge.", "Nhap: tai file .zom de tai tao chinh xac thach thuc do."));
                    ImGui::BulletText("%s", tr("Share .zom files with friends to compete on identical maps.", "Chia se file .zom voi ban be de thi dau tren ban do giong het nhau."));
                }
                ImGui::Spacing();

                // ── CREDITS ───────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Credits", "Tin Chi"))) {
                    ImGui::TextColored(ImVec4(0.95f, 0.9f, 0.35f, 1.0f), "ZomChess v1.0");
                    ImGui::BulletText("%s", tr("Design & Programming: Luan Nguyen", "Thiet ke & Lap trinh: Nguyen Luan"));
                    ImGui::BulletText("%s", tr("Music: 'Ancient Rite', 'Discovery Hit', 'Impending Boom', 'The Ice Giants' — licensed for use.", "Nhac nen: 'Ancient Rite', 'Discovery Hit', 'Impending Boom', 'The Ice Giants' — duoc cap phep su dung."));
                    ImGui::BulletText("%s", tr("Built with: C++, SFML, Dear ImGui, ImGui-SFML.", "Xay dung bang: C++, SFML, Dear ImGui, ImGui-SFML."));
                    ImGui::BulletText("%s", tr("Sound effects synthesized procedurally in-engine.", "Hieu ung am thanh duoc tong hop thu tuc trong engine."));
                }

                ImGui::EndChild();
                ImGui::Separator();
                if (ImGui::Button(tr("Close", "Dong"), ImVec2(160, 34))) {
                    show_guide_popup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
"""

new_lines = new_block.splitlines(keepends=True)
lines[start_idx:end_idx] = new_lines

with open('src/main.cpp', 'w') as f:
    f.writelines(lines)

print("Done. Replaced lines", start_idx, "to", end_idx)
