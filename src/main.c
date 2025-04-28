#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * load_processes:
 *   - filename    : đường dẫn tới file input
 *   - out_pcbs    : &pcb_t*        (mảng pcb_t)
 *   - out_arrival : &int*          (mảng arrival time)
 *   - out_burst   : &int*          (mảng burst time)
 *   - out_remain  : &int*          (mảng remaining time)
 *   - out_n       : &int           (số lượng process)
 *
 * File input định dạng:
 *   n
 *   pid arrival_time burst_time
 *   pid arrival_time burst_time
 *   ...
 */
void load_processes(const char *filename,
                    pcb_t   **out_pcbs,
                    int     **out_arrival,
                    int     **out_burst,
                    int     **out_remain,
                    int     *out_n)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    int n;
    if (fscanf(fp, "%d", &n) != 1 || n <= 0) {
        fprintf(stderr, "Error: invalid process count in '%s'\n", filename);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    *out_n = n;

    // Cấp phát mảng
    *out_pcbs    = malloc(sizeof(pcb_t) * n);
    *out_arrival = malloc(sizeof(int)   * n);
    *out_burst   = malloc(sizeof(int)   * n);
    *out_remain  = malloc(sizeof(int)   * n);
    if (!*out_pcbs || !*out_arrival || !*out_burst || !*out_remain) {
        perror("malloc");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        int pid, at, bt;
        if (fscanf(fp, "%d %d %d", &pid, &at, &bt) != 3) {
            fprintf(stderr,
                    "Error: bad format at line %d in '%s'\n",
                    i + 2, filename);
            fclose(fp);
            exit(EXIT_FAILURE);
        }

        // Khởi tạo pcb_t chỉ chứa CFS fields
        (*out_pcbs)[i].pid       = (uint32_t)pid;
        (*out_pcbs)[i].path[0]   = '\0';          // nếu không dùng path
        (*out_pcbs)[i].vruntime  = 0;
        (*out_pcbs)[i].weight    = 1024;          // default weight

        // Điền mảng dữ liệu riêng
        (*out_arrival)[i] = at;
        (*out_burst)[i]   = bt;
        (*out_remain)[i]  = bt;                   // remaining = burst ban đầu
    }

    fclose(fp);
}
