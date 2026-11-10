# Mochi – Thú cưng ảo tương tác trên ESP32

Mochi là nguyên mẫu thú cưng ảo được xây dựng bằng **ESP32** và mô phỏng trên **Wokwi** trong khuôn khổ môn **Lập trình IoT**. Hệ thống nhận dữ liệu môi trường và thao tác tương tác, xử lý phản ứng cục bộ, hiển thị trên **OLED SSD1306** và gửi dữ liệu giám sát qua **MQTT** đến **Node-RED Dashboard 2.0**.

> **Phạm vi:** Đây là nguyên mẫu mô phỏng, chưa phải sản phẩm đã kiểm chứng trên phần cứng thật. Dashboard dùng để **giám sát một chiều**, không điều khiển ESP32 từ xa.

## Trải nghiệm dự án

- **Mô phỏng Wokwi:** [Mở dự án Mochi](https://wokwi.com/projects/479910431597134849).
- **Video demo YouTube:** [Mở video Mochi](https://www.youtube.com/watch?v=gol9ZqQjCO0).
- **Mã nguồn:** Thư mục [`Mochi_v1_6_MQTT_Dashboard_Stable/`](./Mochi_v1_6_MQTT_Dashboard_Stable/) *(đường dẫn hoạt động sau khi tải thư mục lên GitHub)*.
- **Flow Node-RED:** [`Mochi_NodeRED_Dashboard.json`](./Mochi_NodeRED_Dashboard.json) *(đường dẫn hoạt động sau khi tải tệp lên GitHub)*.
- **Báo cáo:** Sẽ bổ sung vào thư mục `docs/` dưới dạng PDF.

## Chức năng chính

| Thành phần | Chức năng |
|---|---|
| **PET** | Hiển thị biểu cảm RoboEyes; phản ứng với tương tác, nhiệt độ và ánh sáng; hiệu ứng nóng/lạnh, ngáp và ngủ. |
| **MONITOR** | Hiển thị thông tin cảm biến trên hai trang OLED; thao tác tương tác để chuyển trang; cảnh báo nhiệt độ không tự đổi trang. |
| **MQTT** | ESP32 xuất bản bản tin JSON chứa dữ liệu cảm biến và trạng thái. |
| **Node-RED** | Nhận dữ liệu MQTT, xử lý các trường JSON và hiển thị Dashboard. |

Các ngưỡng và chu kỳ dưới đây là **cấu hình của nguyên mẫu**, không phải ngưỡng tiêu chuẩn cho môi trường thực tế: HOT bắt đầu từ **32 °C** và thoát khi dưới **31 °C**; COLD bắt đầu từ **18 °C** trở xuống và thoát khi trên **19 °C**; điều kiện tối ở mức **20%** trở xuống, thoát tối khi trên **25%**. Đối chiếu các giá trị với phiên bản mã nguồn được công bố nếu có thay đổi.

## Cấu trúc repository

Dự án Mochi là **một thư mục con** trong repository `IoT-in-the-classroom`, không phải repository độc lập. Hiện tại trên GitHub, bạn mới đưa lên `DoAnCaNhan/README.md`. Các tệp còn lại trong cây dưới đây là **kế hoạch tải lên**, không phải danh sách tệp đã có sẵn trên GitHub.

```text
IoT-in-the-classroom/
└── DoAnCaNhan/
    ├── README.md
    ├── Mochi_v1_6_MQTT_Dashboard_Stable/
    │   └── ... giữ nguyên các tệp mã nguồn và cấu hình Wokwi hiện có
    ├── Mochi_NodeRED_Dashboard.json
    ├── docs/                         
    │   └── BC_CaNhanIoT_NTHN.docx
    └── images/                       
        └── ... ảnh mạch, OLED và Dashboard
```

**Nguyên tắc tổ chức:** Giữ nguyên thư mục `Mochi_v1_6_MQTT_Dashboard_Stable/` và toàn bộ cấu trúc bên trong như phiên bản đang chạy ổn định; **không tự ý chuyển** `sketch.ino`, `diagram.json`, các tệp `.h/.cpp` hoặc cấu hình PlatformIO sang thư mục khác. Tên và danh sách tệp con sẽ được cập nhật sau khi kiểm tra nội dung thư mục thực tế.

## Hướng dẫn chạy mô phỏng

### Cách 1: Mở trực tiếp trên Wokwi

1. Mở [dự án Mochi trên Wokwi](https://wokwi.com/projects/479910431597134849).
2. Chọn **Start Simulation**, chờ ESP32 khởi động và quan sát OLED ở chế độ **PET**.
3. Thay đổi giá trị cảm biến nhiệt độ/độ ẩm và ánh sáng để quan sát phản ứng; thao tác với đầu vào tương tác để kiểm tra biểu cảm.
4. Chuyển công tắc **PET/MONITOR**. Trong MONITOR, dùng đầu vào tương tác để chuyển giữa hai trang dữ liệu.
5. Các phản ứng cục bộ PET/MONITOR có thể được quan sát mà không cần mở Node-RED Dashboard.

### Cách 2: Sử dụng mã nguồn trong GitHub

1. Tải repository bằng **Code → Download ZIP** hoặc clone repository:

   ```bash
   git clone https://github.com/<ten-tai-khoan>/IoT-in-the-classroom.git
   ```

   Thay `<ten-tai-khoan>` bằng tên tài khoản GitHub thực tế.
2. Mở `IoT-in-the-classroom/DoAnCaNhan/Mochi_v1_6_MQTT_Dashboard_Stable/`.
3. Giữ nguyên các tệp của dự án. Cách mở/chạy phụ thuộc vào cấu trúc thực tế: dự án Wokwi trực tiếp hoặc Wokwi tích hợp VS Code/PlatformIO. **Không giả định** luôn có `sketch.ino` ở thư mục gốc.
4. Đối chiếu cấu hình linh kiện và thư viện với phiên bản Wokwi công khai trước khi chạy.

**Lưu ý:** Loại cảm biến và cách thao tác đầu vào phải theo đúng sơ đồ/cấu hình của phiên bản công khai. Không tự thay DHT11/DHT22 hoặc nút nhấn/cảm ứng chỉ dựa vào ảnh minh họa cũ.

## Thiết lập MQTT và Node-RED

**Yêu cầu:** Node.js, Node-RED và các node **FlowFuse Dashboard / Dashboard 2.0** mà flow thực tế sử dụng.

1. Cài đặt và khởi động Node-RED trên máy tính.
2. Trong Node-RED Editor, chọn **Menu → Import**, nhập tệp [`Mochi_NodeRED_Dashboard.json`](./Mochi_NodeRED_Dashboard.json) nằm cùng cấp với README. Cài các node còn thiếu nếu Node-RED thông báo.
3. Kiểm tra cấu hình MQTT broker, topic và các node; sau đó chọn **Deploy**.
4. Chạy mô phỏng Wokwi; kiểm tra node MQTT đã kết nối và Dashboard có nhận bản tin.
5. Mở địa chỉ Dashboard được cấu hình trong flow. Địa chỉ được ghi nhận trong dự án là `http://localhost:1880/dashboard/mochi` — địa chỉ này chỉ hoạt động trên máy đang chạy Node-RED và khi route của Dashboard không bị thay đổi.

Cấu hình được mô tả trong hồ sơ dự án:

```text
Broker: broker.hivemq.com
Port:   1883
Topic:  mochi/v1/test84721/telemetry
```

Đây là broker thử nghiệm công khai, sử dụng cổng MQTT không mã hóa. **Không gửi thông tin cá nhân, mật khẩu hoặc token** qua topic này. Nếu thay đổi topic trong mã nguồn, phải cập nhật tương ứng trong Node-RED.

Các trường bản tin được mô tả gồm `device_id`, `mode`, `temperature_c`, `humidity_pct`, `dht_ok`, `light_pct`, `light_adc`, `sound_adc`, `dark`, `temperature_status` và `sleeping`. Dashboard hiển thị các trường theo cấu hình flow; không nhất thiết hiển thị toàn bộ JSON.

## Kiểm thử và giới hạn

Theo biên bản kiểm thử PET/MONITOR của dự án, có **22 tình huống**: **21 tình huống được ghi nhận đạt**, **1 tình huống chưa thực hiện** do hạn chế thao tác đồng thời trong môi trường mô phỏng. Thống kê này **không bao gồm kiểm thử hiệu năng MQTT/Node-RED** và không đồng nghĩa với việc đã đo độ trễ mạng, độ chính xác cảm biến hay độ ổn định trên thiết bị thật.

Ảnh và video minh chứng phải lấy từ phiên chạy thực tế. Sơ đồ thiết kế không thay thế được bằng chứng thực thi.

## Tài liệu và video

- **Báo cáo đồ án:** `docs/BC_CaNhanIoT_NTHN.docx`.
- **Ảnh minh họa:** `images/` .
- **Video demo YouTube:** (https://www.youtube.com/watch?v=gol9ZqQjCO0).
- **Mô phỏng Wokwi:** https://wokwi.com/projects/479910431597134849

## Tác giả và mục đích sử dụng

Dự án được thực hiện phục vụ học tập môn **Lập trình IoT**. Mã nguồn, tài liệu và hình ảnh được công bố nhằm giới thiệu cách xây dựng nguyên mẫu tương tác trên ESP32 và giám sát bằng MQTT–Node-RED.

**Giấy phép:** Chưa xác định. Chỉ bổ sung tệp `LICENSE` sau khi lựa chọn giấy phép phù hợp; việc repository công khai không tự động đồng nghĩa với cấp phép tái sử dụng.
