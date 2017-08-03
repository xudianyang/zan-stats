#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "stats.h"

static int ROW, COL;
static int currrow = 1;

static void draw_progress_bar(uint x, uint y, uint width, uint total, uint v1, uint v2);

static void color_init()
{
    use_default_colors();
    init_pair(ZS_COLOR_WHITE, COLOR_WHITE, -1);
    init_pair(ZS_COLOR_RED, COLOR_RED, -1);
    init_pair(ZS_COLOR_GREEN, COLOR_GREEN, -1);
    init_pair(ZS_COLOR_CYAN, COLOR_CYAN, -1);
    init_pair(ZS_COLOR_BLACK_GREEN, 0, COLOR_CYAN);
}

static void draw_progress_bar(uint x, uint y, uint width, uint total, uint v1, uint v2)
{
    assert(v1 <= total);
    assert(v2 <= total);
    assert(v1 <= v2);

    float fv1 = v1, fv2 = v2;

    fv1 = ceil((fv1 / total) * width);
    fv2 = ceil((fv2 / total) * width);

    uint i;
    zs_select_color(ZS_COLOR_WHITE);
    zs_bold_on();
    mvaddch(y, x++, '[');
    zs_bold_off();
    for (i = 0; i < width; i++) {
        if (i <= fv1) {
            zs_select_color(ZS_COLOR_GREEN);
        } else {
            zs_select_color(ZS_COLOR_RED);
        }
        if (i < fv2) {
            mvaddch(y, x + i, '|');
        }
    }
    zs_select_color(ZS_COLOR_WHITE);
    zs_bold_on();
    mvaddch(y, x + i, ']');
    zs_bold_off();
    mvprintw(y, x + i + 2, "%d/%d/%d", v1, v2, total);
}

static void draw_worker_stats()
{
    zs_select_color(ZS_COLOR_WHITE);
    char worker_stats[] = "    Worker Stats: ";
    char tmp[32] = {0};
    mvaddnstr(currrow++, LEFT_ALIGN, worker_stats, sizeof(worker_stats));
    sprintf(tmp, "%d/%d/%d", 64, 30, 50);
    draw_progress_bar(sizeof(worker_stats) + 1, 3,
            COL - sizeof(worker_stats) - strlen(tmp) - 7, 64, 30, 50);
}

static void draw_task_worker_stats()
{
    zs_select_color(ZS_COLOR_WHITE);
    char tmp[32] = {0};
    char worker_stats[] = "TaskWorker Stats: ";
    sprintf(tmp, "%d/%d/%d", 64, 20, 40);
    mvaddnstr(currrow++, LEFT_ALIGN, worker_stats, sizeof(worker_stats));
    draw_progress_bar(sizeof(worker_stats) + 1, 4,
            COL - sizeof(worker_stats) - strlen(tmp) - 7, 64, 20, 40);
    currrow++;
}

static void draw_title()
{
    zs_bold_on();
    char *title = "Zan stats [zanphp.io]";
    mvprintw(currrow, (COL-strlen(title))/2, "%s", title);
    zs_bold_off();
    currrow += 2;
}

static void draw_text_with_width(int y, int x, char *label, int total_width, char *format, ...)
{
    static char buff[1024] = {0};
    int pad, len;
    va_list al;
    mvprintw(y, x, label);

    memset(buff, 0, 1024);
    va_start(al, format);
    len = vsprintf(buff, format, al);
    va_end(al);

    pad = total_width - strlen(label) - len;
    if (pad > 0) {
        memset(buff + len, ' ', pad);
    }

    zs_bold_on();
    len = printw(buff);
    zs_bold_off();
}

static void draw_base_info()
{
    int left_offset = 0;
#define line_step() do {\
    if (COL < (BASE_INFO_WIDTH * 2)) { \
        currrow++; \
        left_offset = 0; \
    } else {\
        left_offset = BASE_INFO_WIDTH; \
    } \
} while (0)
    draw_text_with_width(currrow, LEFT_ALIGN, "start time:     ", BASE_INFO_WIDTH, "2017-10-11 11:11:11");
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "last reload:  ", BASE_INFO_WIDTH, "2017-10-11 11:11:11");
    draw_text_with_width(currrow, LEFT_ALIGN, "connection num: ", BASE_INFO_WIDTH, "1111");
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "accept count: ", BASE_INFO_WIDTH, "123123");
    draw_text_with_width(currrow, LEFT_ALIGN, "close count:    ", BASE_INFO_WIDTH, "1111");
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "tasking num:  ", BASE_INFO_WIDTH, "100");

    draw_text_with_width(currrow, LEFT_ALIGN, "worker exit count(normal/abnomal): ", BASE_INFO_WIDTH, "%d/%d", 100, 200);
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "task worker exit count(normal/abnomal): ", BASE_INFO_WIDTH, "%d/%d", 50, 100);
    }

static zs_worker_detail *worker_detail_new(char *title, int width, int height, int x, int y, int total_worker)
{
    zs_worker_detail *detail = malloc(sizeof(zs_worker_detail));
    memset(detail, 0, sizeof(zs_worker_detail));

    detail->width = width;
    detail->height = height;
    detail->x = x;
    detail->y = y;

    detail->total_worker = total_worker;
    detail->item = calloc(sizeof(struct worker_detail_item), total_worker);

    detail->win = newwin(height, width, y, x);
    detail->panel = new_panel(detail->win);

    // show
    box(detail->win, 0, 0);
    mvwaddch(detail->win, 2, 0, ACS_LTEE);
    mvwhline(detail->win, 2, 1, ACS_HLINE, width - 2);
    mvwaddch(detail->win, 2, width - 1, ACS_RTEE);

    // print title
    int length, tx;
    length = strlen(title);
    tx = (int)(width - length)/ 2;
    wattron(detail->win, COLOR_PAIR(ZS_COLOR_GREEN));
    mvwprintw(detail->win, 1, tx, "%s", title);
    wattroff(detail->win, COLOR_PAIR(ZS_COLOR_GREEN));
    refresh();

    // draw th
    int i, th_width = width - 2, pad;
    int col_width = floor(th_width / WORKER_DETAIL_TH_NUM);
    char *th[] = WORKER_DETAIL_TH;
    for (i = 0; i < WORKER_DETAIL_TH_NUM; i++) {
        detail->th_width[i] = col_width;
    }
    if ((pad = (col_width * WORKER_DETAIL_TH_NUM)) < th_width) {
        detail->th_width[WORKER_DETAIL_TH_NUM - 1] += th_width - pad;;
    }

    // to draw
    char tmp[256] = {0};
    char fmt[16] = {0};
    wmove(detail->win, 3, 1);
    wattron(detail->win, COLOR_PAIR(ZS_COLOR_BLACK_GREEN));
    for (i = 0; i < WORKER_DETAIL_TH_NUM; i++) {
        sprintf(fmt, "%%-%ds", detail->th_width[0]);
        sprintf(tmp, fmt, th[i]);
        wprintw(detail->win, "%s", tmp);
    }
    wattroff(detail->win, COLOR_PAIR(ZS_COLOR_BLACK_GREEN));

    return detail;
}

static int worker_detail_update(zs_worker_detail *detail, int offset, struct worker_detail_item *item)
{
    if (offset > (detail->total_worker - 1)) {
        return -1;
    }

    struct worker_detail_item *old = &detail->item[offset];
    memcpy(old, item, sizeof(struct worker_detail_item));

    return 0;
}

static void worker_detail_refresh(zs_worker_detail *detail)
{
    int i = 0, pad;
    char tmp[256] = {0};
    char fmt[16] = {0};

    for (; i < detail->total_worker; i++) {
        wmove(detail->win, 4 + i, 1);
        // worker_id
        sprintf(fmt, "%%-%dd", detail->th_width[0]);
        sprintf(tmp, fmt, detail->item[i].worker_id);
        wprintw(detail->win, "%s", tmp);

        // start_time
        sprintf(fmt, "%%-%ds", detail->th_width[1]);
        sprintf(tmp, fmt, detail->item[i].start_time);
        wprintw(detail->win, "%s", tmp);

        // total request
        sprintf(fmt, "%%-%dd", detail->th_width[2]);
        sprintf(tmp, fmt, detail->item[i].total_request);
        wprintw(detail->win, "%s", tmp);

        // request
        sprintf(fmt, "%%-%dd", detail->th_width[3]);
        sprintf(tmp, fmt, detail->item[i].request);
        wprintw(detail->win, "%s", tmp);

        // status
        wprintw(detail->win, "%s", detail->item[i].status);

        if (i > (detail->height - 7)) {
            break;
        }
    }
}

static void worker_detail_free(zs_worker_detail* d)
{
    if (d->win) {
        delwin(d->win);
    }
    if (d->panel) {
        del_panel(d->panel);
    }
    if (d->item) {
        free(d->item);
    }
    free(d);
    d = NULL;
}

static void draw_worker_detail()
{
    currrow++;
    currrow++;

    int worker_num = 32;
    int width, height, i;
    width = (COL - (LEFT_ALIGN * 2)) / 2 - 5;
    height = ROW - currrow - 1;
    zs_worker_detail *worker_detail = worker_detail_new("Worker Detail",
            width, height, LEFT_ALIGN, currrow, 32);
    struct worker_detail_item item;
    for (i = 0; i < worker_num; i++) {
        item.worker_id = i + 1;
        strcpy(item.start_time, "11:11:11");
        item.total_request = i * 201 + 10;
        item.request = i * 101 + 4;
        if (i % 2 == 0) {
            strcpy(item.status, "BUSY");
        } else {
            strcpy(item.status, "IDLE");
        }
        worker_detail_update(worker_detail, i, &item);
    }
    refresh();
    update_panels();
    doupdate();
    worker_detail_refresh(worker_detail);
}

static void draw_task_worker_detail()
{
    int worker_num = 32;
    int width, height, i;
    width = (COL - (LEFT_ALIGN * 2)) / 2 - 5;
    height = ROW - currrow - 1;
    zs_worker_detail *worker_detail = worker_detail_new("Task Worker Detail",
            width, height, LEFT_ALIGN + width + 11, currrow, worker_num);
    struct worker_detail_item item;
    for (i = 0; i < worker_num; i++) {
        item.worker_id = i + 1;
        strcpy(item.start_time, "11:11:11");
        item.total_request = i * 201 + 10;
        item.request = i * 101 + 4;
        if (i % 2 == 0) {
            strcpy(item.status, "BUSY");
        } else {
            strcpy(item.status, "IDLE");
        }
        worker_detail_update(worker_detail, i, &item);
    }

    worker_detail_refresh(worker_detail);
    refresh();
    update_panels();
    doupdate();
}

int main() {
    initscr();    /* initializes curses */
    start_color();
    noecho();
    cbreak();
    color_init();
    getmaxyx(stdscr, ROW, COL);
#if 1
    draw_title();
    draw_worker_stats();
    draw_task_worker_stats();
    draw_base_info();
#endif
    draw_worker_detail();
    draw_task_worker_detail();
    refresh();
    getch();
    endwin();     /* cleanup curses */
    return 0;
}