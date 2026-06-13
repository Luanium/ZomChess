1. **Bước 1 (Web Dev):** Dịch mã nguồn C++ sang WebAssembly (Emscripten). Thử nghiệm kéo-thả bản build lên Vercel/Netlify.
2. **Bước 2 (Multi-Platform CI/CD):** Thiết lập GitHub Actions tự động build ra 3 file chạy (Linux, Windows, macOS) và tự động cập nhật bản Web khi push code.
3. **Bước 3 (Game Backend):** Tích hợp Đăng nhập Guest/User và Bảng xếp hạng trực tuyến qua LootLocker (Miễn phí).
4. **Bước 4 (Franchise & Monetize):** Tạo một trang chủ làm Hub chứa chuỗi game ZomChess. Đăng ký phát hành lên Poki/CrazyGames để lấy traffic và dòng tiền quảng cáo nuôi Web riêng.

---

## II. Bộ Prompt Siêu Tối Ưu Cho AI (Hiệu suất cao, Tiết kiệm Token)

*Mẹo tiết kiệm token:* Hãy gửi kèm danh sách file trong repo của bạn (ví dụ: `main.cpp`, `CMakeLists.txt`) để AI viết đúng ngay từ lần đầu, tránh giải thích lòng vòng.

### 1. Prompt Giai đoạn 1: Biên dịch sang WebAssembly
* **Tác dụng:** Chuyển đổi vòng lặp game (`while(true)`) và cấu hình build C++ sang môi trường trình duyệt.

> 🇻🇳 **Tiếng Việt:**
> "Tôi có game C++ (dùng C++17, SFML/ImGui). Hãy chuyển Main Loop sang `emscripten_set_main_loop` để tương thích WebAssembly. Chỉ xuất file mã nguồn đã sửa và file CMakeLists.txt/Makefile để build bằng `emcc`. Không giải thích dài dòng."

> 🇬🇧 **English:**
> "I have a C++ game (C++17, SFML/ImGui). Adapt the Main Loop using `emscripten_set_main_loop` for WebAssembly. Provide only the modified source code and the updated CMakeLists.txt/Makefile for `emcc` compilation. No lengthy explanations."

### 2. Prompt Giai đoạn 2: Tự động hóa Multi-Platform (GitHub Actions)
* **Tác dụng:** Tạo file cấu hình để GitHub tự động xuất file chạy `.exe` (Windows), `.app/zip` (macOS), và binary (Linux) mỗi khi bạn cập nhật code.

> 🇻🇳 **Tiếng Việt:**
> "Viết file cấu hình GitHub Actions `.github/workflows/multi-platform.yml`. Khi push lên `main` hoặc tạo `release`, tự động khởi tạo môi trường để build game C++ (SFML/ImGui) thành 3 bản: Linux (Ubuntu), Windows (MinGW/MSVC), và macOS (Clang). Tự đính kèm các file chạy thành phẩm vào GitHub Releases. Viết code ngắn gọn, không giải thích."

> 🇬🇧 **English:**
> "Write a GitHub Actions workflow `.github/workflows/multi-platform.yml`. On push to `main` or `release`, automatically build the C++ game (SFML/ImGui) for 3 platforms: Linux (Ubuntu), Windows (MinGW/MSVC), and macOS (Clang). Upload artifacts to GitHub Releases. Code only, no explanations."

### 3. Prompt Giai đoạn 3: Đọc/Ghi file đa nền tảng
* **Tác dụng:** Giúp AI xử lý tính năng Lưu game (Savegame) độc lập giữa PC (ghi vào ổ cứng) và Web (ghi vào bộ nhớ trình duyệt).

> 🇻🇳 **Tiếng Việt:**
> "Viết hàm lưu/tải game bằng C++ sử dụng tiền xử lý `#ifdef __EMSCRIPTEN__`. Nếu là Web, dùng `EM_ASM` để lưu vào LocalStorage/IndexedDB. Nếu là PC (Windows/Linux/macOS), ghi file thông thường. Xuất code trực tiếp."

> 🇬🇧 **English:**
> "Write C++ save/load functions using `#ifdef __EMSCRIPTEN__`. For Web, use `EM_ASM` to save to LocalStorage/IndexedDB. For PC (Windows/Linux/macOS), use standard file I/O. Output code directly."

### 4. Prompt Giai đoạn 4: Tích hợp Bảng xếp hạng LootLocker
* **Tác dụng:** Gọi API của LootLocker để làm tính năng tài khoản trực tuyến mà không cần tự viết server.

> 🇻🇳 **Tiếng Việt:**
> "Hướng dẫn tích hợp LootLocker REST API vào game C++ để: Đăng nhập Guest, Đăng nhập Account, và Gửi điểm lên Leaderboard. Chỉ cung cấp đoạn code xử lý HTTP Request/JSON ngắn gọn nhất, không giải thích lý thuyết."

> 🇬🇧 **English:**
> "Show how to integrate LootLocker REST API in C++ for: Guest login, Account login, and Submitting scores to a Leaderboard. Provide only the minimalist HTTP/JSON request code. No theoretical explanations."

---

## III. Các lưu ý cốt lõi khi Vận hành Multi-Platform

1.  **Hạn chế thư viện độc quyền:** Tránh dùng các hàm chỉ chạy trên Windows (như `<windows.h>`) hoặc chỉ chạy trên Linux. Hãy để AI ưu tiên dùng thư viện chuẩn C++ (`std::`).
2.  **Quản lý tài nguyên (Assets):** Hình ảnh, âm thanh trong code phải dùng đường dẫn tương đối (ví dụ: `assets/image.png`), không dùng đường dẫn tuyệt đối (như `C:/Users/...`) vì game sẽ bị lỗi khi chạy trên Linux/macOS/Web.
3.  **Chi phí chạy GitHub Actions:** Gói miễn phí của GitHub cho bạn 2000 phút chạy Actions mỗi tháng. Bản build Windows và macOS sẽ tốn thời gian chạy (và nhân hệ số token/phút) nhiều hơn Linux. Vì vậy, **chỉ nên kích hoạt tự động build khi bạn tạo một "Release" mới (phiên bản chính thức)**, tránh kích hoạt mỗi lần sửa một dòng code nhỏ (Push thông thường).
4.  **Ký số trên macOS (Code Signing):** Bản build macOS do GitHub Actions tạo ra khi tải về máy Mac của người chơi sẽ bị cảnh báo "Unknown Developer" (Nhà phát triển không xác định). Bạn cần hướng dẫn người chơi nhấn chuột phải chọn `Open` để bỏ qua cảnh báo này (vì chứng chỉ chính thức của Apple tốn $99/năm).
"""

# Path to save the markdown file
file_path = "cheatsheet_zomchess_multiplatform.md"

# Write the content to the file
with open(file_path, "w", encoding="utf-8") as file:
    file.write(cheatsheet_content)

print(f"File created successfully at: {file_path}")
