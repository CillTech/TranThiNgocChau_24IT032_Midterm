# TranThiNgocChau_24IT032_Midterm

Dự án mô phỏng và tái hiện đầy đủ hành vi câu lệnh `ls` trên hệ điều hành NetBSD.

---

## 1. Giới thiệu tổng quan
Dự án được xây dựng bằng ngôn ngữ **C** nhằm mô phỏng lệnh `ls` của NetBSD. 

---

## 2. Tính năng hỗ trợ 

Chương trình triển khai các cờ theo đúng cú pháp:
`ls [-AacdFfhiklnqRrSstuw] [file ...]`

* **Lọc file:**
  * `-a`: Liệt kê tất cả các mục, bao gồm file ẩn bắt đầu bằng dấu chấm `.`.
  * `-A`: Liệt kê các file ẩn nhưng bỏ qua hai mục đặc biệt `.` và `..`.
  * `-d`: Hiển thị thư mục như file thông thường, không mở và duyệt nội dung bên trong.
* **Định dạng hiển thị:**
  * `-l`: Định dạng danh sách dài (Long format).
  * `-n`: Tương tự `-l` nhưng hiển thị Owner UID và Group GID dưới dạng số.
  * `-h`: Hiển thị dung lượng file theo định dạng dễ đọc cho người dùng (B, K, M, G, T) thay vì byte thuần.
  * `-k`: Hiển thị dung lượng theo đơn vị Kilobyte.
  * `-s`: Hiển thị số khối (file system blocks) được cấp phát cho mỗi file.
  * `-i`: Hiển thị số Serial Inode (`st_ino`).
  * `-F`: Đính kèm ký tự nhận dạng loại file ngay sau tên:
  * `/` (thư mục), `*` (file thực thi), `@` (symbolic link), `=` (socket), `|` (FIFO/pipe), `%` (whiteout).
  * `-q`: Thay thế các ký tự không in được bằng dấu `?` (mặc định bật khi xuất ra terminal).
  * `-w`: In thô (raw) các ký tự không in được (mặc định bật khi redirect ra file/pipe).
* **Tiêu chí sắp xếp:**
  * Sắp xếp từ điển (Lexicographical) mặc định.
  * `-t`: Sắp xếp theo mốc thời gian (mới nhất lên trước).
  * `-c`: Dùng thời gian thay đổi trạng thái file (`ctime`) cho việc sắp xếp `-t` hoặc in `-l`.
  * `-u`: Dùng thời gian truy cập gần nhất (`atime`) cho việc sắp xếp `-t` hoặc in `-l`.
  * `-S`: Sắp xếp theo kích thước file giảm dần.
  * `-f`: Tắt sắp xếp (xuất theo thứ tự đọc trong thư mục) và tự động kích hoạt cờ `-a`.
  * `-r`: Đảo ngược thứ tự sắp xếp.
* **Xử lý thư mục:**
  * `-R`: Duyệt đệ quy (recursive) toàn bộ cây thư mục con.
  * Phân tách toán tử: Nếu truyền nhiều tham số, in các file thường trước, sau đó mới duyệt các thư mục.
* **Môi trường & Hệ thống:**
  * Hỗ trợ biến môi trường `BLOCKSIZE` để điều chỉnh đơn vị tính toán số block.
  * Trả về Exit status: `0` nếu thành công, `>0` nếu phát sinh lỗi (permission denied, no such file, v.v.).

---

## 3. Cấu trúc thư mục dự án

```text
.
├── my_ls.c         # Mã nguồn C chính
├── Makefile        # Tập tin biên dịch tự động
├── test_diff.sh    # Script bash kiểm thử tự động với ls hệ thống
└── README.md       # Hướng dẫn sử dụng
