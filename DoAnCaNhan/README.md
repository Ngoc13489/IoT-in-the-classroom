# Mochi – Thú cưng ảo tương tác trên ESP32

**Mochi** là nguyên mẫu thú cưng ảo trên ESP32, được phát triển trong môn **Lập trình IoT** và chạy trong môi trường mô phỏng **Wokwi**. Thiết bị nhận tín hiệu môi trường và tương tác, tự xử lý hành vi, hiển thị trên màn hình **OLED SSD1306**, đồng thời gửi dữ liệu qua **MQTT** đến **Node-RED Dashboard 2.0** để theo dõi.

> **Phạm vi:** Dự án được thử nghiệm trong môi trường mô phỏng, chưa được xác nhận trên phần cứng thực. Dashboard phục vụ **giám sát một chiều**, không gửi lệnh điều khiển về ESP32.

## Xem nhanh

- **[Video trình diễn trên YouTube](https://www.youtube.com/watch?v=gol9ZqQjCO0)**
- **[Mô phỏng Mochi trên Wokwi](https://wokwi.com/projects/479910431597134849)**
- **[Thư mục mã nguồn ESP32](./Mochi_v1_6_MQTT_Dashboard_Stable/)**
- **[Tệp luồng Node-RED](./Mochi_NodeRED_Dashboard.json)**

## 1. Chức năng

| Nhóm | Mô tả |
| --- | --- |
| **PET** | Hiển thị đôi mắt RoboEyes; phản ứng với tương tác và điều kiện môi trường; có các hiệu ứng nóng, lạnh, ngáp và ngủ. |
| **MONITOR** | Hiển thị thông tin cảm biến trên hai trang OLED; nhấn nút tương tác để chuyển trang; cảnh báo nhiệt độ không tự chuyển trang. |
| **MQTT** | ESP32 xuất bản dữ liệu cảm biến và trạng thái dưới dạng JSON. |
| **Node-RED Dashboard** | Nhận dữ liệu MQTT, xử lý và trực quan hóa các giá trị/trạng thái của Mochi. |

### Quy tắc phản ứng chính

| Điều kiện | Phản ứng |
| --- | --- |
| Nhiệt độ **≥ 32 °C** | Bật hiệu ứng **HOT**; tắt khi nhiệt độ **< 31 °C**. |
| Nhiệt độ **≤ 18 °C** | Bật hiệu ứng **COLD**; tắt khi nhiệt độ **> 19 °C**. |
| Mức sáng quy ước **≤ 20%** | Chuyển sang quy trình ngáp rồi ngủ; thoát điều kiện tối khi mức sáng **> 25%**. |
| Có tương tác trong lúc ngủ | Mochi có thể phản ứng; nếu môi trường vẫn tối, quay lại quy trình ngủ theo logic chương trình. |
| Chế độ **MONITOR** | Nút tương tác chuyển giữa hai trang dữ liệu. |

Các giá trị trên là **ngưỡng phần mềm của nguyên mẫu**, không phải khuyến nghị về nhiệt độ hay ánh sáng thực tế. Phần trăm ánh sáng là giá trị quy đổi từ ADC, **không phải lux**.

## 2. Thành phần mô phỏng và kết nối

| Thành phần | ESP32 | Vai trò |
| --- | --- | --- |
| DHT22 | GPIO27 | Nhiệt độ và độ ẩm không khí |
| Quang trở, đầu ra analog | GPIO35 (ADC1) | Tín hiệu ánh sáng |
| Biến trở | GPIO34 (ADC1) | Giả lập mức tín hiệu âm thanh |
| Nút nhấn | GPIO25 | Giả lập thao tác chạm/tương tác |
| Công tắc trượt | GPIO32 | Chọn PET/MONITOR |
| OLED SSD1306 – SDA | GPIO21 | Dữ liệu I²C |
| OLED SSD1306 – SCL | GPIO22 | Xung nhịp I²C |

OLED có độ phân giải **128 × 64**, địa chỉ I²C **`0x3C`**. Biến trở **không phải micro**, nút nhấn **không phải cảm biến chạm thực**. Sơ đồ kết nối chi tiết được xác định bởi `diagram.json` của dự án.

## 3. Kiến trúc hệ thống

```text
DHT22 ───────────────┐
Quang trở ───────────┤
Biến trở (giả âm) ───┤
Nút nhấn ────────────┼──> ESP32 ──> OLED SSD1306 (PET / MONITOR)
Công tắc trượt ──────┘       │
                             └── Wi-Fi ── MQTT broker ── Node-RED ── Dashboard
```

**Phản ứng của Mochi được xử lý tại ESP32.** MQTT là kênh gửi dữ liệu giám sát; Dashboard không quyết định biểu cảm và không điều khiển thiết bị từ xa.

## 4. Cấu trúc dự án

Dự án nằm trong thư mục `DoAnCaNhan` của repository `IoT-in-the-classroom`:

```text
IoT-in-the-classroom/
└── DoAnCaNhan/
    ├── README.md
    ├── Mochi_v1_6_MQTT_Dashboard_Stable/   # Dự án ESP32/Wokwi
    └── Mochi_NodeRED_Dashboard.json        # Flow Node-RED
```

Thư mục mã nguồn chứa các tệp chương trình và cấu hình mô phỏng. Các tệp thường được tham chiếu trong báo cáo gồm `sketch.ino`, `diagram.json`, `MochiDisplay.cpp`, `MochiDisplay.h` và `MochiTypes.h`; vị trí cụ thể của chúng phụ thuộc vào cấu trúc được lưu trong thư mục dự án. Không cần di chuyển các tệp ra khỏi thư mục gốc của dự án.

## 5. Chạy mô phỏng Wokwi

### Cách nhanh nhất: mở dự án trực tuyến

1. Mở **[Mochi trên Wokwi](https://wokwi.com/projects/479910431597134849)**.
2. Nhấn **Start Simulation** và chờ ESP32 khởi động.
3. Quan sát OLED; thử thay đổi nhiệt độ DHT22, ánh sáng và giá trị biến trở trên sơ đồ.
4. Nhấn nút tương tác để quan sát phản ứng của Mochi.
5. Chuyển công tắc giữa **PET** và **MONITOR**; trong MONITOR, nhấn nút tương tác để chuyển trang.

Có thể quan sát **phản ứng cục bộ trên OLED** mà không cần mở Node-RED. Để xem Dashboard, thực hiện thêm phần 6.

### Sử dụng mã nguồn từ GitHub

1. Tại repository GitHub, chọn **Code → Download ZIP** (hoặc sử dụng Git để clone repository).
2. Giải nén và mở thư mục `DoAnCaNhan/Mochi_v1_6_MQTT_Dashboard_Stable/`.
3. Đọc các tệp cấu hình đi kèm, đặc biệt `diagram.json` và tệp khai báo thư viện (nếu có), để xác định cách chạy trong Wokwi/VS Code.
4. Giữ nguyên cấu trúc thư mục khi mở hoặc nhập dự án vào môi trường phát triển tương ứng.

**Lưu ý về môi trường:** Liên kết Wokwi trực tuyến là đường chạy nhanh đã được công bố. Cách chạy bản tải về trong VS Code/PlatformIO/Wokwi CLI phụ thuộc vào tệp cấu hình thực tế trong thư mục mã nguồn; không thể mặc định một lệnh build duy nhất cho mọi cấu trúc dự án.

## 6. Thiết lập Node-RED Dashboard

### 6.1. Chuẩn bị

Cần có **Node.js** (kèm npm), **Node-RED** và gói **FlowFuse Dashboard 2.0** (`@flowfuse/node-red-dashboard`).

Kiểm tra Node.js và npm trong Terminal/PowerShell:

```bash
node --version
npm --version
```

Nếu chưa có Node.js, cài từ [nodejs.org](https://nodejs.org/), sau đó mở lại Terminal. Cài Node-RED:

```bash
npm install -g node-red
```

Khởi động Node-RED:

```bash
node-red
```

Mở trình duyệt tại **http://localhost:1880**. Giữ Terminal chạy trong suốt quá trình sử dụng Node-RED.

### 6.2. Cài Dashboard 2.0

1. Trong Node-RED Editor, mở **Menu → Manage palette → Install**.
2. Tìm **`@flowfuse/node-red-dashboard`** và chọn **Install**.
3. Chờ cài đặt hoàn tất. Nếu Node-RED yêu cầu, khởi động lại dịch vụ.

### 6.3. Nhập flow Mochi

1. Tải tệp **[`Mochi_NodeRED_Dashboard.json`](./Mochi_NodeRED_Dashboard.json)** từ thư mục dự án.
2. Trong Node-RED Editor, chọn **Menu → Import**, chọn tệp JSON, rồi **Import**.
3. Kiểm tra các node MQTT, xử lý JSON và Dashboard; cài thêm node còn thiếu nếu được thông báo.
4. Kiểm tra thông số broker và topic ở các node MQTT, sau đó chọn **Deploy**.
5. Chạy mô phỏng Mochi trên Wokwi, chờ kết nối Wi-Fi/MQTT, rồi kiểm tra các node nhận dữ liệu.
6. Mở **http://localhost:1880/dashboard/mochi** để xem giao diện nếu flow giữ nguyên đường dẫn Dashboard này. Nếu không mở được, kiểm tra cấu hình đường dẫn trong Dashboard của Node-RED.

### 6.4. Cấu hình MQTT

```text
Broker: broker.hivemq.com
Port:   1883
Topic:  mochi/v1/test84721/telemetry
```

ESP32 đóng vai trò **publisher**, Node-RED là **subscriber**. Chu kỳ xuất bản được cấu hình danh định là **2.000 ms** khi điều kiện kết nối cho phép; đây **không phải độ trễ mạng được đo**.

Broker được sử dụng là dịch vụ thử nghiệm công khai, cổng 1883 **không mã hóa**. Không gửi mật khẩu, token hoặc dữ liệu cá nhân. Nếu thay đổi topic trong chương trình ESP32, phải thay topic tương ứng trong node MQTT của Node-RED. Trên broker công khai, các thiết bị sử dụng cùng topic có thể nhận dữ liệu của nhau; nên đổi topic riêng khi tái sử dụng dự án.

### 6.5. Dữ liệu gửi đi

Bản tin MQTT ở dạng JSON, gồm các trường:

| Trường | Ý nghĩa |
| --- | --- |
| `device_id` | Mã định danh thiết bị |
| `mode` | Chế độ PET hoặc MONITOR |
| `temperature_c` | Nhiệt độ (°C) |
| `humidity_pct` | Độ ẩm không khí (%) |
| `dht_ok` | Trạng thái đọc DHT |
| `light_pct` | Mức sáng quy đổi (%) |
| `light_adc` | Giá trị ADC ánh sáng |
| `sound_adc` | Giá trị ADC giả lập âm thanh |
| `dark` | Trạng thái điều kiện tối |
| `temperature_status` | Trạng thái nhiệt độ |
| `sleeping` | Trạng thái ngủ |

Dashboard thể hiện các chỉ số/trạng thái được cấu hình trong flow, không nhất thiết hiển thị toàn bộ trường JSON.

## 7. Thử nhanh các chức năng

Các bước dưới đây là **kịch bản thao tác kiểm tra**, không phải số liệu đo hiệu năng:

1. **PET:** Khởi động mô phỏng, quan sát biểu cảm mặc định; nhấn nút tương tác để xem phản ứng.
2. **HOT:** Chỉnh nhiệt độ DHT22 lên **34 °C**, quan sát hiệu ứng nóng; giảm xuống dưới **31 °C** để kiểm tra thoát HOT.
3. **COLD:** Chỉnh nhiệt độ xuống **16 °C**, quan sát hiệu ứng lạnh; tăng lên trên **19 °C** để kiểm tra thoát COLD.
4. **Ngủ:** Giảm mức sáng đến vùng tối, quan sát quá trình ngáp rồi ngủ; thử tương tác khi đang ngủ và tăng sáng để quan sát thay đổi.
5. **MONITOR:** Chuyển công tắc sang MONITOR, nhấn nút để đổi giữa hai trang OLED; kiểm tra cảnh báo nhiệt độ không tự chuyển trang.
6. **Dashboard:** Khi Node-RED đã kết nối, thay đổi nhiệt độ trong Wokwi và quan sát giá trị/trạng thái trên Dashboard.

Nếu một bước không khớp, cần đối chiếu với **phiên bản mã nguồn và sơ đồ đang chạy**; không xem các thao tác gợi ý này là bằng chứng rằng mọi chức năng đã được kiểm thử lại trên phiên bản mới.

## 8. Khắc phục sự cố

| Hiện tượng | Kiểm tra |
| --- | --- |
| Không chạy được Wokwi | Kiểm tra trạng thái mô phỏng, tệp sơ đồ và thông báo biên dịch. |
| OLED không hiển thị | Kiểm tra kết nối SDA GPIO21, SCL GPIO22, địa chỉ I²C `0x3C` và log chương trình. |
| Không thấy phản ứng HOT/COLD | Kiểm tra nhiệt độ DHT22, ngưỡng kích hoạt/thoát và trạng thái đọc cảm biến. |
| Node-RED không mở | Kiểm tra Node.js, lệnh `node-red`, Terminal đang chạy và cổng `1880`. |
| Import flow bị thiếu node | Cài `@flowfuse/node-red-dashboard` và các node khác được Node-RED chỉ rõ. |
| MQTT không kết nối | Kiểm tra kết nối Wi-Fi của Wokwi, địa chỉ broker, cổng 1883, tình trạng dịch vụ và cấu hình node MQTT. |
| Dashboard không có dữ liệu | Kiểm tra đã **Deploy**, trạng thái node MQTT, topic của publisher/subscriber và nội dung bản tin. |
| Đường dẫn Dashboard không mở | Kiểm tra route thực tế trong cấu hình Dashboard 2.0; đường dẫn có thể khác nếu flow được chỉnh sửa. |

## 9. Kết quả và giới hạn

Theo biên bản kiểm thử chức năng PET/MONITOR, nguyên mẫu có **22 tình huống**, trong đó **21 tình huống được ghi nhận đạt** và **1 tình huống chưa thực hiện** do giới hạn thao tác đồng thời trong môi trường Wokwi. Kết quả này **không phải** thống kê kiểm thử hiệu năng MQTT/Node-RED.

Dự án chưa cung cấp phép đo định lượng về độ trễ đầu cuối, tỷ lệ mất bản tin, độ chính xác cảm biến, điện năng tiêu thụ hoặc độ ổn định trên phần cứng thật. Giá trị âm thanh từ biến trở và phần trăm ánh sáng chỉ phục vụ mô phỏng logic.

## 10. Tài liệu và liên kết

- [Video demo Mochi trên YouTube](https://www.youtube.com/watch?v=gol9ZqQjCO0)
- [Dự án mô phỏng Wokwi](https://wokwi.com/projects/479910431597134849)
- [Mã nguồn ESP32/Wokwi](./Mochi_v1_6_MQTT_Dashboard_Stable/)
- [Flow Node-RED Dashboard](./Mochi_NodeRED_Dashboard.json)

## 11. Tác giả và phạm vi sử dụng

**Nguyễn Thị Hồng Ngọc** – Đồ án cá nhân môn **Lập trình IoT**, Khoa Công nghệ Thông tin, Trường Đại học An Giang (2026).

Dự án được công bố để giới thiệu và tham khảo cách xây dựng nguyên mẫu ESP32 tương tác, kết hợp giám sát MQTT và Node-RED. Repository chưa có tuyên bố giấy phép tái sử dụng riêng; việc mã nguồn được công khai không mặc nhiên cấp quyền sử dụng lại theo một giấy phép mã nguồn mở.
