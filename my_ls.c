#ifndef _NETBSD_SOURCE
#define _NETBSD_SOURCE 1
#endif

#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>

#ifdef __linux__
#include <sys/sysmacros.h>
#endif

#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#ifndef S_ISVTX
#define S_ISVTX 01000
#endif

#if defined(S_IFWHT) && !defined(S_ISWHT)
#define S_ISWHT(m) (((m) & S_IFMT) == S_IFWHT)
#elif !defined(S_ISWHT)
#define S_ISWHT(m) 0
#endif

typedef enum {
    TIME_MOD = 0, // mtime mặc định
    TIME_CHG,     // ctime (-c)
    TIME_ACC      // atime (-u)
} TimeType;

typedef struct {
    int A; // Bỏ qua . và ..
    int a; // Hiển thị tất cả file ẩn
    TimeType time_type; // mtime, ctime (-c), hay atime (-u)
    int d; // Thư mục xem như file thường
    int F; // Hậu tố ký tự file (*, /, @, =, |, %)
    int f; // Không sắp xếp
    int h; // Kích thước human-readable (ghi đè -k)
    int k; // Kích thước kilobytes (ghi đè -h)
    int i; // In số Inode
    int l; // Long format (ghi đè -n)
    int n; // Long format hiển thị UID/GID dạng số (ghi đè -l)
    int q; // In ký tự lạ thành '?' (ghi đè -w)
    int w; // In raw ký tự không in được (ghi đè -q)
    int R; // Đệ quy thư mục
    int r; // Đảo ngược sắp xếp
    int S; // Sắp xếp theo kích thước
    int s; // Hiển thị số block của từng file
    int t; // Sắp xếp theo thời gian
} Options;

typedef struct {
    char *name;
    char *path;
    struct stat st;
    int stat_ok;
} FileItem;

static Options opts = {0};
static int exit_code = 0;
static long blocksize = 512; 

void init_blocksize(void) {
    char *env_bs = getenv("BLOCKSIZE");
    if (env_bs && strlen(env_bs) > 0) {
        long bs = atol(env_bs);
        if (bs > 0) {
            blocksize = bs;
        }
    }
}

// Lấy mốc thời gian phù hợp theo cờ -c hoặc -u
time_t get_item_time(const struct stat *st) {
    if (opts.time_type == TIME_CHG) return st->st_ctime;
    if (opts.time_type == TIME_ACC) return st->st_atime;
    return st->st_mtime;
}

// Tính số lượng khối (block count) theo BLOCKSIZE, -k, hoặc 512 bytes
long long get_file_blocks(const struct stat *st) {
    long long bytes = (long long)st->st_blocks * 512LL;
    long bs = blocksize;
    if (opts.k) bs = 1024;
    return (bytes + bs - 1) / bs; // Làm tròn lên theo đặc tả
}

// In chuỗi định dạng human-readable (-h)
void format_human_size(off_t size, char *buf, size_t buf_len) {
    const char units[] = {'B', 'K', 'M', 'G', 'T'};
    double d_size = (double)size;
    int idx = 0;

    while (d_size >= 1024.0 && idx < 4) {
        d_size /= 1024.0;
        idx++;
    }

    if (idx == 0) {
        snprintf(buf, buf_len, "%4lldB", (long long)size);
    } else {
        snprintf(buf, buf_len, "%4.1f%c", d_size, units[idx]);
    }
}

// Định dạng quyền hạn 10 ký tự
void format_mode(mode_t mode, char *str) {
    if (S_ISREG(mode))       str[0] = '-';
    else if (S_ISDIR(mode))  str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else if (S_ISWHT(mode))  str[0] = 'w';
    else str[0] = '?';

    // Quyền User
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    if (mode & S_ISUID) {
        str[3] = (mode & S_IXUSR) ? 's' : 'S';
    } else {
        str[3] = (mode & S_IXUSR) ? 'x' : '-';
    }

    // Quyền Group
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    if (mode & S_ISGID) {
        str[6] = (mode & S_IXGRP) ? 's' : 'S';
    } else {
        str[6] = (mode & S_IXGRP) ? 'x' : '-';
    }

    // Quyền Others
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    if (mode & S_ISVTX) {
        str[9] = (mode & S_IXOTH) ? 't' : 'T';
    } else {
        str[9] = (mode & S_IXOTH) ? 'x' : '-';
    }

    str[10] = '\0';
}

// Ký tự phân loại file (-F)
char get_file_indicator(mode_t mode) {
    if (S_ISDIR(mode)) return '/';
    if (S_ISLNK(mode)) return '@';
    if (S_ISSOCK(mode)) return '=';
    if (S_ISFIFO(mode)) return '|';
    if (S_ISWHT(mode)) return '%';
    if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) return '*';
    return '\0';
}

// In tên file có xử lý ký tự không in được (-q / -w)
void print_sanitized_name(const char *name) {
    for (size_t i = 0; name[i] != '\0'; i++) {
        unsigned char c = (unsigned char)name[i];
        if (opts.q && !isprint(c)) {
            putchar('?');
        } else {
            putchar(c);
        }
    }
}

// Hàm so sánh cho qsort
int compare_items(const void *a, const void *b) {
    const FileItem *itemA = (const FileItem *)a;
    const FileItem *itemB = (const FileItem *)b;
    int res = 0;

    if (opts.t) {
        time_t tA = get_item_time(&itemA->st);
        time_t tB = get_item_time(&itemB->st);
        if (tA < tB) res = 1;
        else if (tA > tB) res = -1;
        else res = strcmp(itemA->name, itemB->name);
    } else if (opts.S) {
        if (itemA->st.st_size < itemB->st.st_size) res = 1;
        else if (itemA->st.st_size > itemB->st.st_size) res = -1;
        else res = strcmp(itemA->name, itemB->name);
    } else {
        res = strcmp(itemA->name, itemB->name);
    }

    return opts.r ? -res : res;
}

// In thông tin 1 file/thư mục
void print_item(const FileItem *item) {
    if (opts.i) {
        printf("%8lu ", (unsigned long)item->st.st_ino);
    }

    if (opts.s) {
        printf("%4lld ", get_file_blocks(&item->st));
    }

    if (!opts.l && !opts.n) {
        print_sanitized_name(item->name);
        if (opts.F) {
            char ind = get_file_indicator(item->st.st_mode);
            if (ind) putchar(ind);
        }
        putchar('\n');
        return;
    }

    // Long format (-l hoặc -n)
    char mode_str[12];
    format_mode(item->st.st_mode, mode_str);
    printf("%s %3lu ", mode_str, (unsigned long)item->st.st_nlink);

    // Xử lý hiển thị Owner / Group
    if (opts.n) {
        printf("%-8d %-8d ", item->st.st_uid, item->st.st_gid);
    } else {
        struct passwd *pw = getpwuid(item->st.st_uid);
        if (pw) printf("%-8s ", pw->pw_name);
        else printf("%-8d ", item->st.st_uid);

        struct group *gr = getgrgid(item->st.st_gid);
        if (gr) printf("%-8s ", gr->gr_name);
        else printf("%-8d ", item->st.st_gid);
    }

    // Kích thước hoặc số Major/Minor (nếu là thiết bị đặc biệt)
    if (S_ISCHR(item->st.st_mode) || S_ISBLK(item->st.st_mode)) {
        printf("%3u, %3u ", (unsigned int)major(item->st.st_rdev), (unsigned int)minor(item->st.st_rdev));
    } else if (opts.h) {
        char sz_buf[16];
        format_human_size(item->st.st_size, sz_buf, sizeof(sz_buf));
        printf("%6s ", sz_buf);
    } else if (opts.k) {
        printf("%8lld ", (long long)(item->st.st_size + 1023) / 1024);
    } else {
        printf("%8lld ", (long long)item->st.st_size);
    }

    // Mốc thời gian
    time_t ftime = get_item_time(&item->st);
    struct tm *lt = localtime(&ftime);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%b %e %H:%M", lt);
    printf("%s ", time_buf);

    // Tên file và cờ -F
    print_sanitized_name(item->name);
    if (opts.F) {
        char ind = get_file_indicator(item->st.st_mode);
        if (ind) putchar(ind);
    }

    // Nếu là Symbolic link -> in target
    if (S_ISLNK(item->st.st_mode)) {
        char target[1024];
        ssize_t len = readlink(item->path, target, sizeof(target) - 1);
        if (len != -1) {
            target[len] = '\0';
            printf(" -> ");
            print_sanitized_name(target);
        }
    }
    putchar('\n');
}

void process_dir(const char *dirpath, int print_dirname) {
    DIR *d = opendir(dirpath);
    if (!d) {
        fprintf(stderr, "ls: %s: %s\n", dirpath, strerror(errno));
        exit_code = 1;
        return;
    }

    if (print_dirname) {
        printf("%s:\n", dirpath);
    }

    struct dirent *entry;
    FileItem *items = NULL;
    size_t count = 0;
    long long total_blocks = 0;

    while ((entry = readdir(d)) != NULL) {
        if (!opts.a && !opts.A && entry->d_name[0] == '.') {
            continue;
        }
        if (opts.A && (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))) {
            continue;
        }

        items = realloc(items, sizeof(FileItem) * (count + 1));
        items[count].name = strdup(entry->d_name);

        char full_path[4096];
        if (strcmp(dirpath, "/") == 0) {
            snprintf(full_path, sizeof(full_path), "/%s", entry->d_name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", dirpath, entry->d_name);
        }
        items[count].path = strdup(full_path);

        if (lstat(full_path, &items[count].st) == 0) {
            items[count].stat_ok = 1;
            total_blocks += get_file_blocks(&items[count].st);
        } else {
            items[count].stat_ok = 0;
            memset(&items[count].st, 0, sizeof(struct stat));
        }
        count++;
    }
    closedir(d);

    // Nếu không có cờ -f thì sắp xếp
    if (!opts.f) {
        qsort(items, count, sizeof(FileItem), compare_items);
    }

    // Hiển thị tổng block nếu bật -l, -n hoặc -s khi xuất ra terminal
    if ((opts.l || opts.n || opts.s) && count > 0) {
        printf("total %lld\n", total_blocks);
    }

    for (size_t i = 0; i < count; i++) {
        print_item(&items[i]);
    }

    // Đệ quy -R
    if (opts.R) {
        for (size_t i = 0; i < count; i++) {
            if (S_ISDIR(items[i].st.st_mode)) {
                if (strcmp(items[i].name, ".") != 0 && strcmp(items[i].name, "..") != 0) {
                    putchar('\n');
                    process_dir(items[i].path, 1);
                }
            }
        }
    }

    for (size_t i = 0; i < count; i++) {
        free(items[i].name);
        free(items[i].path);
    }
    free(items);
}

int main(int argc, char *argv[]) {
    // Khởi tạo trạng thái mặc định cho hiển thị ký tự (mặc định ra terminal là -q)
    opts.q = isatty(STDOUT_FILENO) ? 1 : 0;
    opts.w = !opts.q;
    opts.time_type = TIME_MOD;

    init_blocksize();

    int opt;
    while ((opt = getopt(argc, argv, "AacdFfhiklnqRrSstuw")) != -1) {
        switch (opt) {
            case 'A': opts.A = 1; break;
            case 'a': opts.a = 1; break;
            case 'c': opts.time_type = TIME_CHG; break; // Ghi đè -u
            case 'u': opts.time_type = TIME_ACC; break; // Ghi đè -c
            case 'd': opts.d = 1; break;
            case 'F': opts.F = 1; break;
            case 'f': opts.f = 1; break;
            case 'h': opts.h = 1; opts.k = 0; break;    // Ghi đè -k
            case 'k': opts.k = 1; opts.h = 0; break;    // Ghi đè -h
            case 'i': opts.i = 1; break;
            case 'l': opts.l = 1; opts.n = 0; break;    // Ghi đè -n
            case 'n': opts.n = 1; opts.l = 0; break;    // Ghi đè -l
            case 'q': opts.q = 1; opts.w = 0; break;    // Ghi đè -w
            case 'w': opts.w = 1; opts.q = 0; break;    // Ghi đè -q
            case 'R': opts.R = 1; break;
            case 'r': opts.r = 1; break;
            case 'S': opts.S = 1; break;
            case 's': opts.s = 1; break;
            case 't': opts.t = 1; break;
            default:
                exit_code = 1;
                return exit_code;
        }
    }

    // Nếu có cờ -f thì tắt cả sắp xếp và bật -a theo đặc tả NetBSD
    if (opts.f) {
        opts.a = 1;
    }

    int remaining = argc - optind;

    if (remaining == 0) {
        if (opts.d) {
            FileItem item;
            item.name = ".";
            item.path = ".";
            lstat(".", &item.st);
            print_item(&item);
        } else {
            process_dir(".", 0);
        }
        return exit_code;
    }

    FileItem *files = NULL;
    size_t file_count = 0;
    char **dirs = NULL;
    size_t dir_count = 0;

    for (int i = optind; i < argc; i++) {
        struct stat st;
        if (lstat(argv[i], &st) != 0) {
            fprintf(stderr, "ls: %s: %s\n", argv[i], strerror(errno));
            exit_code = 1;
            continue;
        }

        if (opts.d || !S_ISDIR(st.st_mode)) {
            files = realloc(files, sizeof(FileItem) * (file_count + 1));
            files[file_count].name = argv[i];
            files[file_count].path = argv[i];
            files[file_count].st = st;
            files[file_count].stat_ok = 1;
            file_count++;
        } else {
            dirs = realloc(dirs, sizeof(char *) * (dir_count + 1));
            dirs[dir_count] = argv[i];
            dir_count++;
        }
    }

    // In non-directory operands trước
    if (file_count > 0) {
        if (!opts.f) {
            qsort(files, file_count, sizeof(FileItem), compare_items);
        }
        for (size_t i = 0; i < file_count; i++) {
            print_item(&files[i]);
        }
        free(files);
        if (dir_count > 0) putchar('\n');
    }

    // Sau đó duyệt và in các directory operands
    for (size_t i = 0; i < dir_count; i++) {
        int show_name = (remaining > 1 || opts.R);
        process_dir(dirs[i], show_name);
        if (i + 1 < dir_count) putchar('\n');
    }

    free(dirs);
    return exit_code;
}

