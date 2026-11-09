# Mochi – Thú cưng ảo tương tác trên ESP32

Mochi là nguyên mẫu thú cưng ảo được xây dựng bằng **ESP32** và mô phỏng trên **Wokwi** trong khuôn khổ môn Lập trình IoT. Hệ thống nhận dữ liệu môi trường và thao tác tương tác, xử lý phản ứng cục bộ, hiển thị trên **OLED SSD1306** và gửi dữ liệu giám sát qua **MQTT** đến **Node-RED Dashboard 2.0**.

> **Phạm vi:** Đây là nguyên mẫu mô phỏng, chưa phải sản phẩm đã kiểm chứng trên phần cứng thật. Dashboard dùng để **giám sát một chiều**, không điều khiển ESP32 từ xa.

## Trải nghiệm dự án

- **Mô phỏng Wokwi:** [Mở dự án Mochi](https://wokwi.com/projects/479910431597134849) *(kiểm tra liên kết trỏ đúng phiên bản công khai trước khi phát hành repository)*.
- **Video demo YouTube:** Chưa công bố – sẽ cập nhật đường dẫn sau khi đăng video.
- **Báo cáo:** Xem thư mục `docs/` khi tài liệu được đưa lên repository.

## Chức năng chính

| Thành phần | Chức năng |
|---|---|
| **PET** | Hiển thị biểu cảm RoboEyes; phản ứng với tương tác, nhiệt độ và ánh sáng; hiệu ứng nóng/lạnh, ngáp và ngủ. |
| **MONITOR** | Hiển thị thông tin cảm biến trên hai trang OLED; thao tác tương tác để chuyển trang; cảnh báo nhiệt độ không tự đổi trang. |
| **MQTT** | ESP32 xuất bản bản tin JSON chứa dữ liệu cảm biến và trạng thái. |
| **Node-RED** | Nhận dữ liệu MQTT, xử lý các trường JSON và hiển thị Dashboard. |

Các ngưỡng và chu kỳ dưới đây là **cấu hình của nguyên mẫu**, không phải ngưỡng tiêu chuẩn cho môi trường thực tế: HOT bắt đầu từ 32 °C và thoát khi dưới 31 °C; COLD bắt đầu từ 18 °C trở xuống và thoát khi trên 19 °C; điều kiện tối khoảng 20%, thoát tối trên 25%. Các giá trị cần được đối chiếu với phiên bản mã nguồn được đăng trong repository.

## Cấu trúc repository

Cấu trúc dưới đây là **cách sắp xếp đề xuất** để người khác dễ tìm mã nguồn và tái lập dự án. Chỉ đưa lên những tệp thực sự có; không cần tạo ảnh hoặc tệp rỗng chỉ để đủ cấu trúc.

```text
mochi-esp32/
├── README.md
├── sketch.ino                 # Mã chương trình ESP32 cho Wokwi
├── diagram.json               # Sơ đồ linh kiện và kết nối Wokwi
├── libraries.txt              # Danh sách thư viện, nếu dự án sử dụng
├── node-red/
│   └── mochi-dashboard.json   # Flow Node-RED đã xuất, không chứa thông tin bí mật
├── docs/
│   ├── bao-cao.pdf            # Báo cáo đồ án, nếu công khai
│   └── kiem-thu.md            # Biên bản và phạm vi kiểm thử
└── media/
    ├── circuit.png            # Ảnh chụp thực tế từ Wokwi
    ├── pet.png
    ├── monitor.png
    └── dashboard.png
```

**Vì sao đặt `sketch.ino`, `diagram.json` ở thư mục gốc?** Đây là cách đơn giản để sao chép hoặc đối chiếu trực tiếp với một dự án Wokwi. Nếu mã nguồn hiện tại dùng PlatformIO (`src/main.cpp`, `platformio.ini`) hoặc nhiều tệp `.h/.cpp`, giữ nguyên cấu trúc đang biên dịch thành công và cập nhật cây thư mục này cho đúng thực tế.

## Hướng dẫn chạy mô phỏng

1. Mở [dự án Wokwi](https://wokwi.com/projects/479910431597134849). Nếu sử dụng mã từ GitHub, tạo hoặc sao chép một dự án Wokwi phù hợp rồi nạp `sketch.ino`, `diagram.json` và `libraries.txt` (nếu có).
2. Chọn **Start Simulation** và chờ ESP32 khởi động. Quan sát OLED ở chế độ **PET**.
3. Thay đổi giá trị cảm biến nhiệt độ/độ ẩm và ánh sáng để quan sát phản ứng. Thao tác với đầu vào tương tác để kiểm tra biểu cảm.
4. Chuyển công tắc **PET/MONITOR**; trong MONITOR, dùng đầu vào tương tác để đổi giữa hai trang dữ liệu.
5. Nếu không dùng MQTT hoặc Node-RED, các phản ứng cục bộ của PET/MONITOR vẫn có thể được kiểm tra trong Wokwi.

**Lưu ý:** Loại cảm biến và cách thao tác đầu vào phải theo đúng `diagram.json` của phiên bản công khai. Không thay DHT11/DHT22 hoặc nút nhấn/cảm ứng chỉ dựa vào ảnh minh họa cũ.

## Thiết lập MQTT và Node-RED

**Yêu cầu:** Node.js, Node-RED và các node **FlowFuse Dashboard / Dashboard 2.0** mà tệp flow thực tế sử dụng.

1. Cài đặt và khởi động Node-RED trên máy tính.
2. Trong Node-RED Editor, cài các node còn thiếu theo cảnh báo của flow; dùng **Menu → Import** để nhập `node-red/mochi-dashboard.json`.
3. Kiểm tra cấu hình MQTT broker, topic, tên các node và bấm **Deploy**.
4. Chạy mô phỏng Wokwi; kiểm tra node MQTT hiển thị trạng thái kết nối và Dashboard nhận bản tin.
5. Truy cập địa chỉ Dashboard được cấu hình trong flow. Trong bản triển khai được ghi nhận, địa chỉ cục bộ là `http://localhost:1880/dashboard/mochi` (địa chỉ này chỉ hoạt động trên máy đang chạy Node-RED).

Cấu hình được mô tả trong hồ sơ dự án:

```text
Broker: broker.hivemq.com
Port:   1883
Topic:  mochi/v1/test84721/telemetry
```

Đây là broker thử nghiệm công khai, sử dụng cổng MQTT không mã hóa. **Không gửi thông tin cá nhân, mật khẩu hoặc token** qua topic này. Nếu thay đổi topic trong mã nguồn, phải cập nhật tương ứng trong Node-RED.

Các trường dữ liệu được mô tả trong bản tin gồm `device_id`, `mode`, `temperature_c`, `humidity_pct`, `dht_ok`, `light_pct`, `light_adc`, `sound_adc`, `dark`, `temperature_status` và `sleeping`. Dashboard chỉ hiển thị một phần các trường theo cấu hình flow.

## Kiểm thử và giới hạn

Theo biên bản kiểm thử PET/MONITOR của dự án, có **22 tình huống**: **21 tình huống được ghi nhận đạt**, **1 tình huống chưa thực hiện** do hạn chế thao tác đồng thời trong môi trường mô phỏng. Kết quả này không đồng nghĩa với việc đã đo độ trễ mạng, độ chính xác cảm biến hay độ ổn định của thiết bị thật.

Các hình ảnh và video được đưa vào repository phải là ảnh chụp/ghi hình từ phiên chạy thực tế. Sơ đồ thiết kế không được dùng thay cho minh chứng thực nghiệm.

## Tài liệu và video

- **Báo cáo đồ án:** `docs/bao-cao.pdf` *(khi được công khai)*.
- **Biên bản kiểm thử:** `docs/kiem-thu.md` *(khi được công khai)*.
- **Video demo:** *Chưa có URL YouTube; bổ sung sau khi đăng tải*.
- **Wokwi:** https://wokwi.com/projects/479910431597134849

## Tác giả và mục đích sử dụng

Dự án được thực hiện phục vụ học tập môn **Lập trình IoT**. Mã nguồn, tài liệu và hình ảnh được chia sẻ để tham khảo cách xây dựng nguyên mẫu tương tác trên ESP32 và giám sát bằng MQTT–Node-RED.

**Giấy phép:** Chưa xác định. Chỉ bổ sung tệp `LICENSE` sau khi lựa chọn giấy phép phù hợp; việc repository công khai không tự động đồng nghĩa với cấp phép tái sử dụng.
