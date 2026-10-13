# Midterm Project – Implement `ls(1)`

- **Sinh viên:** Lê Anh Trường
- **Mã sinh viên:** 23CE084
- **Tài khoản GitHub:** crysalix4
- **Tên repository:** `LeAnhTruong_23CE084_midterm`
- **Môi trường chạy chính:** FreeBSD

## 1. Mục tiêu và phạm vi

Chương trình `myls` được viết bằng C, thực hiện tập con của lệnh `ls(1)` theo **duy nhất manual được cung cấp trong đề**, lưu tại [docs/ls.pdf](docs/ls.pdf). Manual mang tiêu đề NetBSD 10.1 nhưng chương trình được chuẩn bị để biên dịch trên **FreeBSD**; các quy tắc đầu ra tuân theo bản PDF này.

Chương trình dùng các API hệ thống/thư viện C để đọc filesystem, không gọi lệnh `ls` bên ngoài, `system()` hoặc `popen()`. Mặc định in một mục mỗi dòng, kể cả khi stdout là terminal. Không triển khai những tùy chọn nằm ngoài đề như `--color`, `-H`, `-L`, `-P`, `-C` hoặc `-1`.

## 2. Cấu trúc dự án

```text
LeAnhTruong_23CE084_midterm/
├── Makefile
├── README.md
├── .gitignore
├── .gitattributes         # Giữ line ending LF khi checkout trên Windows
├── docs/
│   └── ls.pdf              # Manual gốc của đề
├── include/
│   ├── options.h
│   ├── entry.h
│   ├── listing.h
│   ├── format.h
│   └── util.h
└── src/
    ├── main.c             # Khởi tạo, gọi các module, trả exit status
    ├── options.c          # Phân tích tùy chọn và BLOCKSIZE
    ├── entry.c            # Danh sách động, thời gian và sắp xếp
    ├── listing.c          # Đọc operand/thư mục và duyệt đệ quy
    ├── format.c           # Định dạng tên, metadata và long listing
    └── util.c             # Bộ nhớ, nối đường dẫn, báo lỗi
```

File thực thi `myls` và các file `.o` được tạo khi build, đều nằm trong `.gitignore`.

## 3. Biên dịch và chạy trên FreeBSD

FreeBSD có sẵn Clang (`cc`) và BSD `make`. Chương trình chính không cần thư viện ngoài và không cần Python. Chạy bằng tài khoản sinh viên trong thư mục dự án:

```sh
cd ~/LeAnhTruong_23CE084_midterm
make
./myls
./myls -l
./myls -Alh /etc
./myls -iFs .
./myls -R src include
./myls -d /etc /usr
./myls -n /dev/null
```

Có thể gộp tùy chọn (`-Alh`) hoặc tách (`-A -l -h`). Đặt tùy chọn trước các đường dẫn. `--` kết thúc phần tùy chọn để xử lý tên bắt đầu bằng dấu `-`:

```sh
./myls -- -ten-file
./myls "thu muc co khoang trang"
```

Build với cảnh báo nghiêm ngặt, hoặc xóa các file đã biên dịch:

```sh
make clean
make CFLAGS='-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror'
make clean
```

**Chuyển từ Windows sang VM FreeBSD:** sao chép mã nguồn, sau đó chạy `make` bên trong FreeBSD. Không sử dụng file thực thi được build từ Windows/Ubuntu trên FreeBSD. Ví dụ PowerShell với cổng SSH NAT `2200` của VM hiện có:

```powershell
Set-Location D:\LapTrinhHeThong
tar -czf "$env:TEMP\LeAnhTruong_23CE084_midterm.tar.gz" --exclude=.git --exclude='*.o' --exclude=myls -C . LeAnhTruong_23CE084_midterm
scp -P 2200 "$env:TEMP\LeAnhTruong_23CE084_midterm.tar.gz" leanhtruong@127.0.0.1:~/
ssh -p 2200 leanhtruong@127.0.0.1
```

Sau khi đăng nhập FreeBSD:

```sh
tar -xzf ~/LeAnhTruong_23CE084_midterm.tar.gz -C ~
cd ~/LeAnhTruong_23CE084_midterm
make
./myls -l
```

Linux/WSL cũng được hỗ trợ để biên dịch bằng `make`. Các quyền file trên filesystem Windows gắn qua `/mnt/d` có thể khác filesystem FreeBSD; nên kiểm tra quyền file trực tiếp trên filesystem FreeBSD.

## 4. Chức năng đã triển khai

| Tùy chọn | Hành vi |
| --- | --- |
| `-A` | Hiện mục ẩn, trừ `.` và `..`; tự bật khi effective UID bằng 0. |
| `-a` | Hiện toàn bộ mục, gồm `.` và `..`. |
| `-c` | Chọn thời điểm thay đổi trạng thái (`ctime`) cho `-t` và `-l`. |
| `-d` | In chính directory operand như một file; không đọc nội dung hoặc theo symlink operand. |
| `-F` | Thêm `/` cho thư mục, `*` cho executable, `@` cho symlink, `%` cho whiteout, `=` cho socket, `\|` cho FIFO. |
| `-f` | Giữ thứ tự đầu vào/`readdir`, không sắp xếp. |
| `-h` | Hiển thị kích thước của `-s`, `-l` và tổng theo dạng dễ đọc. |
| `-i` | In inode number. |
| `-k` | Đơn vị block là 1024 byte; kích thước file trong `-l` vẫn là byte. |
| `-l` | Long listing với tên chủ sở hữu và nhóm. |
| `-n` | Long listing với UID/GID dạng số. |
| `-q` | Thay ký tự không in được bằng `?`; mặc định khi stdout là terminal. |
| `-R` | Liệt kê thư mục con theo chiều sâu. |
| `-r` | Đảo ngược toàn bộ thứ tự sắp xếp. |
| `-S` | Sắp xếp theo kích thước giảm dần, rồi theo tên. |
| `-s` | In dung lượng block thực cấp phát; có dòng `total` khi stdout là terminal. |
| `-t` | Sắp xếp theo thời gian mới nhất trước, rồi theo tên. |
| `-u` | Chọn thời điểm truy cập (`atime`) cho `-t` và `-l`. |
| `-w` | In nguyên tên; mặc định khi stdout được redirect/pipe. |

Manual có **19 chữ tùy chọn**: `A a c d F f h i k l n q R r S s t u w`. Mọi chữ trong synopsis đều được hỗ trợ.

Các cặp ghi đè theo thứ tự xuất hiện: `-h`/`-k`, `-l`/`-n`, `-c`/`-u`, `-q`/`-w`, `-R`/`-d`. Ví dụ `-Rd` chỉ in directory operand, còn `-dR` duyệt đệ quy. `-a` bao gồm nhiều mục hơn `-A` và không bị một `-A` phía sau vô hiệu hóa.

Không có operand thì dùng `.`. Với nhiều operand, các file được in trước các thư mục; hai nhóm được sắp xếp riêng. `-d` đưa toàn bộ operand vào cùng nhóm để in. `-f` chỉ tắt sắp xếp, không tự bật `-a` hoặc tắt long listing. `-c`/`-u` không tự bật sắp xếp theo thời gian nếu thiếu `-t`.

### Long listing và kiểu file

Thứ tự thông tin: file mode, số hard link, owner, group, kích thước byte (hoặc major/minor cho thiết bị), tháng viết tắt, ngày, giờ:phút, tên. Nếu lookup owner/group thất bại, in UID/GID số. Symlink có thêm ` -> target`.

Mode gồm ký tự loại file và 9 ký tự quyền; xử lý đủ `s`, `S`, `t`, `T` cho set-user-ID, set-group-ID và sticky bit. Các loại thông thường gồm regular file, directory, symlink, FIFO, socket, block device và character device. Whiteout và archive state `a`/`A` được nhận diện có điều kiện khi hệ điều hành cung cấp macro tương ứng; đây không phải kiểu file có thể tạo trên mọi filesystem.

`-l`/`-n` luôn in tổng block trước **nội dung mỗi thư mục**, kể cả khi redirect. Riêng `-s` in tổng trước mỗi nhóm listing khi stdout là terminal, kể cả nhóm file operand. Tổng chỉ tính các mục đang được hiển thị; cộng block thực trước, rồi đổi đơn vị và làm tròn lên một lần.

### Biến môi trường

`BLOCKSIZE` đặt đơn vị block khi không dùng `-h`/`-k`; mặc định là 512 byte. Ví dụ:

```sh
env BLOCKSIZE=1024 ./myls -s .
env BLOCKSIZE=4K ./myls -l .
env TZ=Asia/Ho_Chi_Minh ./myls -l .
env TZ=UTC ./myls -lt .
```

`st_blocks` được hiểu theo đơn vị 512 byte. Số block hiển thị là `ceil(st_blocks * 512 / block_size)`, không dùng độ dài logic `st_size` để thay thế. Vì vậy sparse file có thể có kích thước lớn nhưng dùng ít block. `TZ` được áp dụng qua `tzset()` và `localtime_r()`.

## 5. Thiết kế và xử lý trường hợp biên

1. `options_parse()` khởi tạo mặc định theo terminal/UID rồi đọc tùy chọn từ trái sang phải.
2. `list_paths()` dùng `lstat()` phân loại operand. File và directory operand được lưu trong hai danh sách động.
3. `read_directory()` dùng `open()`, `fdopendir()`, `readdir()` và khi cần metadata dùng `fstatat(..., AT_SYMLINK_NOFOLLOW)`. Listing chỉ cần tên/inode tận dụng `dirent`, nên vẫn hoạt động với thư mục có quyền đọc nhưng thiếu quyền tìm kiếm.
4. `entries_sort()` dùng `qsort()`; so sánh giá trị trực tiếp thay vì trừ size/time để tránh tràn số. So sánh thời gian đến nanosecond.
5. `format_entries()` in các trường và escape tên theo lựa chọn. `readlink()` dùng buffer tăng dần, không giả định giới hạn `PATH_MAX` hoặc độ dài link luôn bằng `st_size`.
6. Duyệt đệ quy bằng stack frame cấp phát trên heap. Mỗi directory stream được đóng trước khi đi xuống. Không đi vào `.`/`..` hoặc symlink trong nội dung thư mục; kiểm tra cặp device/inode của các ancestor để phát hiện chu trình.

Kiểm tra kết quả cấp phát, kiểm tra tràn khi tính kích thước buffer và block, giải phóng danh sách/đường dẫn sau khi dùng. Với operand không tồn tại, thiếu quyền, lỗi đọc directory/link hoặc lỗi ghi stdout, chương trình báo stderr và trả mã khác 0; tiếp tục xử lý các operand khác khi có thể. Mục không lấy được metadata sẽ được bỏ qua nếu chế độ hiện tại bắt buộc cần metadata. Tên quá dài được báo theo lỗi hệ thống, không ghi vượt buffer.

**Exit status:** `0` khi thành công, `1` khi có lỗi hoặc tùy chọn không hỗ trợ. Lỗi cấu hình `BLOCKSIZE` là cảnh báo và dùng lại 512 byte.

### Quy ước cho những điểm PDF không xác định chi tiết

- Thứ tự tên dùng `strcmp()` (theo byte), không giả định thứ tự chữ theo ngôn ngữ.
- `-S` và `-t`: chọn khóa xuất hiện sau cùng; `-f` tắt mọi sắp xếp, kể cả `-r`.
- `-h`: cơ số 1024, hậu tố `B/K/M/G/T/P/E/Z/Y`; một chữ số thập phân nếu trị số dưới 10 và đã đổi đơn vị, còn lại làm tròn số nguyên.
- `BLOCKSIZE`: số byte nguyên dương, tùy chọn hậu tố `K/M/G` không phân biệt hoa thường và chữ `B` sau hậu tố. Chấp nhận hậu tố đứng riêng, ví dụ `K` tương đương `1K` để phù hợp môi trường đăng nhập FreeBSD. Giá trị không hợp lệ phát cảnh báo rồi dùng 512. `-h`/`-k` bỏ qua biến này.
- Với listing thường, symlink operand trỏ đến directory được mở để liệt kê. `-d`, `-l`/`-n` hoặc `-F` giữ thông tin của link; link trong nội dung thư mục luôn được giữ nguyên. Link hỏng vẫn có thể được in bằng `lstat()`/`readlink()`.
- Ngày của cả file cũ và mới đều in `tháng ngày giờ:phút`, đúng các trường ghi trong PDF. Không thêm quy tắc đổi sang năm của những bản `ls` khác.
- Tên tháng/ký tự in được theo locale hiện tại. `-q` giữ nguyên ký tự đa byte hợp lệ, thay control character hoặc byte không hợp lệ bằng `?`.

## 6. Kiểm thử và đánh giá

Phạm vi gồm toàn bộ tùy chọn và thứ tự ghi đè; file ẩn; nhiều operand; inode/UID/GID/quyền; symlink hỏng và đích dài; FIFO/socket/device; size/time sort và tie; nanosecond; `TZ`; `BLOCKSIZE`; sparse file; terminal; tên đặc biệt; thiếu quyền; đệ quy; lỗi argument và lỗi ghi stdout. Kết quả được đối chiếu với metadata filesystem và quy tắc PDF, không lấy GNU `ls` làm đáp án.

| Môi trường | Kiểm tra | Kết quả |
| --- | --- | --- |
| FreeBSD 15.1-RELEASE amd64, Clang 19.1.7, BSD `make` | Biên dịch `-Wall -Wextra -Wpedantic -Werror` | PASS, không có cảnh báo |
| FreeBSD 15.1 | Bộ test shell bản thường | **72/72 PASS** |
| FreeBSD 15.1 | Bộ test shell với ASan/UBSan, không LeakSanitizer | **72/72 PASS** |
| Ubuntu/WSL, GCC 13.3 | Bộ test Python bản thường | **46/46 PASS** |
| Ubuntu/WSL, GCC 13.3 | Bộ test Python với ASan/UBSan và kiểm tra leak | **46/46 PASS** |

Đã chạy thêm bộ Python bằng tài khoản root trên WSL để kiểm tra tự bật `-A` và nhánh kiểm thử hạ quyền. Python chưa được cài trên VM FreeBSD, nên kết quả Python ở bảng trên thuộc Ubuntu; kết quả FreeBSD là bộ test shell chạy thực tế trong VM. Các kiểu whiteout/archive phụ thuộc filesystem và không có fixture thực tế trong các lần chạy này.

Có thể kiểm tra thủ công sau khi chạy `make`:

```sh
./myls -A .
./myls -lh src
./myls -R include src
./myls -d /etc /usr
./myls -n /dev/null
```
