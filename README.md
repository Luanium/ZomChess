# 🧟 ZomChess Tactical Engine v2.1

Một trò chơi chiến thuật sinh tồn theo lượt (Turn-based Tactical Survival) độc đáo được phát triển bằng **C++**, kết hợp đồ họa **SFML** và giao diện quản lý **ImGui**. Trò chơi mô phỏng một bàn cờ sinh tử, nơi bạn phải vận dụng tư duy chiến thuật đỉnh cao để điều khiển nhân vật sống sót trước làn sóng Zombie khát máu.

---

## 🌌 Bối Cảnh Trận Đánh

Hậu tận thế. Bạn là đặc nhiệm sống sót duy nhất bị vây hãm trong một khu vực địa hình phức tạp đầy rẫy thực thể biến dị. Không có đường lui, không có tiếp viện. Thứ duy nhất ngăn cách bạn và cái chết là lượng đạn dược giới hạn, cái đầu lạnh và khả năng tính toán chuẩn xác từng bước đi trên chiến trường.

---

## 🎯 Mục Tiêu Tối Thượng

* **Sống sót hoặc Tiêu diệt:** Quét sạch toàn bộ Zombie trên bản đồ HOẶC sống sót trụ vững cho đến khi hết giới hạn lượt đi (`turn_limit`).
* **Điều kiện Thua cuộc:** Nhân vật cạn kiệt sinh lực (`HP <= 0`) hoặc không hoàn thành nhiệm vụ trong số lượt quy định.

---

## 🎮 Cơ Chế Gameplay Theo Lượt (Turn-Based)

Chiến trường hoạt động theo cơ chế hai pha luân phiên rõ ràng:

### 1. Pha của Con Người (Human Turn)
Mỗi lượt, bạn sẽ nhận được một lượng thể lực ngẫu nhiên (`Stamina`). Bạn có thể thực hiện các hành động sau miễn là còn đủ Stamina:
* **Di chuyển:** Đi tới 8 ô xung quanh. Đi qua nền Đất (`Dirt`) tốn 1 Stamina, lội qua Nước (`Water`) tốn 2 Stamina. Không thể đi vào Tường (`Wall`) hay Vật cản (`Obstacle`).
* **Tấn công & Sử dụng vũ khí:** Kích hoạt chế độ ngắm bắn và click chuột để chọn mục tiêu.

### 2. Pha của Zombie (Zombie Animating)
Sau khi bạn kết thúc lượt, hệ thống Radar Live Logs sẽ quét và kích hoạt AI của Zombie. Chúng tự động tìm đường, áp sát và tấn công bạn theo thuật toán khoảng cách.

---

## ⚔️ Kho Vũ Khí & Khả Năng Tác Chiến

Nhân vật được trang bị tận răng nhưng tài nguyên vô cùng hữu hạn:

* **🔪 Dao găm (Knife):** Vũ khí cận chiến tầm gần, không tốn đạn nhưng đòi hỏi áp sát nguy hiểm.
* **🔫 Súng lục (Pistol):** Vũ khí tầm xa tiêu chuẩn, độ chính xác ổn định.
* **💥 Súng săn (Shotgun):** Sức sát thương diện rộng hủy diệt ở cự ly gần.
* **💣 Lựu đạn (Grenade):** Ném vào một vị trí, kích nổ sau một khoảng thời gian (Grenade Timer) gây sát thương lan rộng phá hủy mục tiêu.
* **🛑 Mìn Claymore (Mines):** Đặt bẫy tại ô bất kỳ. Zombie dẫm phải sẽ kích hoạt nổ ngay lập tức.

---

## ☣️ Các Chủng Loại Zombie Biến Dị

Mỗi loại Zombie sở hữu chỉ số máu (`HP`) và cơ chế di chuyển khác nhau để khắc chế chiến thuật của bạn:

* **Normal Zombie:** Kẻ địch cơ bản, di chuyển 1 ô mỗi lượt.
* **Fast Sprinter:** Di chuyển siêu tốc 2 ô mỗi lượt, chuyên áp sát bất ngờ.
* **Volatile Exploder:** Zombie phát nổ. Khi bị tiêu diệt hoặc kích hoạt, chúng tự bạo gây sát thương lan ra xung quanh.
* **Vampiric Dracula:** Quái vật hút máu. Mỗi lần tấn công trúng con người, chúng sẽ được hồi phục sinh lực.

---

## 🌟 Tính Năng Đặc Sắc Thu Hút Người Chơi

* **⚡ Trận Đấu Nhanh (Quick Play):** Cung cấp 3 mức độ khó lập trình sẵn phù hợp với mọi trình độ: **Binh nhì (Easy)**, **Trung tá (Medium)**, và **Ác mộng (Hard)**.
* **🛠️ Trình Biên Tập Bản Đồ Trực Quan (Visual Map Editor):** Cho phép bạn tự tay vẽ địa hình (Đất, Nước, Tường, Vật cản) và đặt vị trí xuất phát cho nhân vật ngay trên giao diện đồ họa.
* **📥 Hệ Thống Chia Sẻ Thử Thách (.zom):** Tính năng Xuất/Nhập dữ liệu (`export_challenge_file` / `import_challenge_file`) giúp bạn dễ dàng lưu lại map tự chế hoặc tải bản đồ từ bạn bè để thử thách bản thân.
* **🛡️ Vùng Bảo Hiểm Thông Minh (Spawn Shield):** Tùy chọn kích hoạt vùng an toàn 7x7 quanh nhân vật khi bắt đầu game, ngăn chặn Zombie sinh ra quá gần gây bất công.
* **🖥️ Giao Diện Tác Chiến Cyberpunk:** Tông màu chủ đạo tối kết hợp bảng Live Radar Logs hiển thị thời gian thực mọi diễn biến trên chiến trường (sát thương, vụ nổ, zombie hồi máu...) tạo cảm giác nghẹt thở.

---

## 📑 Hướng Dẫn Tra Cứu Thông Tin Địa Hình

| Loại Địa Hình | Màu Sắc Hiển Thị | Đặc Tính Tác Chiến |
| :--- | :--- | :--- |
| **Dirt (Đất)** | Nâu Đất | Di chuyển bình thường. Tốn 1 Stamina. |
| **Water (Nước)** | Xanh Dương | Đầm lầy cản bước. Tốn tới 2 Stamina để vượt qua. |
| **Obstacle (Vật cản)** | Xám Đậm | Vật cản thấp, không thể đi vào. |
| **Wall (Tường)** | Xám Khói | Tường kiên cố biên giới hoặc kiến trúc nội thất, chặn hoàn toàn di chuyển. |

---

## 🛠️ Công Nghệ Phát Triển

* **Language:** C++17 / C++20.
* **Frameworks:** * **SFML (Simple and Fast Multimedia Library):** Quản lý cửa sổ, render đồ họa 2D, sprite và vòng lặp game.
    * **Dear ImGui + ImGui-SFML:** Tạo các slider cấu hình chỉ số nhân vật/zombie, bảng điều khiển và trình chỉnh sửa map editor mượt mà.
