#include "multirom_ui.h"
#include "multirom_ui_themes.h"
#include "multirom.h"
#include "framebuffer.h"
#include "util.h"
#include "button.h"
#include "version.h"

#define HEADER_WIDTH 300
#define TAB_BTN_HEIGHT 75

#define ROMS_FOOTER_H 90
#define ROMS_HEADER_H 60

#define BOOTBTN_W 300
#define BOOTBTN_H 80

#define REFRESHBTN_W 400
#define REFRESHBTN_H 60

#define MISCBTN_W 530
#define MISCBTN_H 100

#define CLRBTN_W 50
#define CLRBTN_B 10
#define CLRBTN_TOTAL (CLRBTN_W+CLRBTN_B)
#define CLRBTN_Y 700

static void destroy(multirom_theme *t)
{

}

static void init_header(multirom_theme *t)
{
    button **tab_btns = t->data->tab_btns;
    fb_text **tab_texts = t->data->tab_texts;

    int i, text_x, text_y;
    int y = TAB_BTN_HEIGHT;

    static const char *str[] = { "Internal", "USB", "Misc", "MultiROM" };

    text_x = center_x(0, HEADER_WIDTH, SIZE_EXTRA, str[3]);
    fb_add_text(text_x, 5, WHITE, SIZE_EXTRA, str[3]);

    for(i = 0; i < TAB_COUNT; ++i)
    {
        text_x = center_x(0, HEADER_WIDTH, SIZE_NORMAL, str[i]);
        text_y = center_y(y, TAB_BTN_HEIGHT, SIZE_NORMAL);
        tab_texts[i] = fb_add_text(text_x, text_y, WHITE, SIZE_NORMAL, str[i]);

        fb_add_rect(0, y, HEADER_WIDTH, 2, WHITE);

        tab_btns[i] = malloc(sizeof(button));
        memset(tab_btns[i], 0, sizeof(button));
        tab_btns[i]->y = y;
        tab_btns[i]->w = HEADER_WIDTH;
        tab_btns[i]->h = TAB_BTN_HEIGHT;
        tab_btns[i]->action = i;
        tab_btns[i]->clicked = &multirom_ui_switch;
        button_init_ui(tab_btns[i], NULL, 0);

        y += TAB_BTN_HEIGHT;
    }
    fb_add_rect(0, y, HEADER_WIDTH, 2, WHITE);

    fb_add_rect(HEADER_WIDTH-2, 0, 2, fb_height, WHITE);
}

static void header_select(multirom_theme *t, int tab)
{
    int i;
    for(i = 0; i < TAB_COUNT; ++i)
        t->data->tab_texts[i]->color = (i == tab) ? BLACK : WHITE;

    if(!t->data->selected_tab_rect)
        t->data->selected_tab_rect = fb_add_rect(0, 0, HEADER_WIDTH, TAB_BTN_HEIGHT, WHITE);

    t->data->selected_tab_rect->head.y = TAB_BTN_HEIGHT + (TAB_BTN_HEIGHT * tab);
}

static void tab_rom_init(multirom_theme *t, tab_data_roms *d, int tab_type)
{
    int base_y = fb_height-ROMS_FOOTER_H;

    d->rom_name = fb_add_text(HEADER_WIDTH, center_y(base_y, ROMS_FOOTER_H, SIZE_NORMAL),
                              WHITE, SIZE_NORMAL, "");

    d->list->x = HEADER_WIDTH;
    d->list->y = ROMS_HEADER_H;
    d->list->w = fb_width-HEADER_WIDTH;
    d->list->h = fb_height - d->list->y - ROMS_FOOTER_H-20;

    // header
    int y = center_y(0, ROMS_HEADER_H, SIZE_BIG);
    d->title_text = fb_add_text(HEADER_WIDTH, y, CLR_PRIMARY, SIZE_BIG, "");
    list_add(d->title_text, &d->ui_elements);

    // footer
    fb_rect *sep = fb_add_rect(HEADER_WIDTH+2, fb_height-ROMS_FOOTER_H, fb_width-HEADER_WIDTH-2, 2, CLR_PRIMARY);
    list_add(sep, &d->ui_elements);

    // boot btn
    d->boot_btn->x = fb_width - BOOTBTN_W - 5;
    d->boot_btn->y = base_y + (ROMS_FOOTER_H-BOOTBTN_H)/2;
    d->boot_btn->w = BOOTBTN_W;
    d->boot_btn->h = BOOTBTN_H;
}

static void tab_misc_init(multirom_theme *t, tab_data_misc *d, int color_scheme)
{
    int x = HEADER_WIDTH + ((fb_width - HEADER_WIDTH)/2 - MISCBTN_W/2);
    int y = 10;

    button *b = mzalloc(sizeof(button));
    b->x = x;
    b->y = y;
    b->w = MISCBTN_W;
    b->h = MISCBTN_H;
    b->clicked = &multirom_ui_tab_misc_copy_log;
    button_init_ui(b, "Copy log to /sdcard", SIZE_BIG);
    list_add(b, &d->buttons);

    y += MISCBTN_H+50;

    static const char *texts[] = 
    {
        "Reboot",               // 0
        "Reboot to recovery",   // 1
        "Reboot to bootloader", // 2
        "Shutdown",             // 3
        NULL
    };

    static const int exit_codes[] = {
        UI_EXIT_REBOOT, UI_EXIT_REBOOT_RECOVERY,
        UI_EXIT_REBOOT_BOOTLOADER, UI_EXIT_SHUTDOWN
    };

    int i;
    for(i = 0; texts[i]; ++i)
    {
        b = mzalloc(sizeof(button));
        b->x = x;
        b->y = y;
        b->w = MISCBTN_W;
        b->h = MISCBTN_H;
        b->action = exit_codes[i];
        b->clicked = &multirom_ui_reboot_btn;
        button_init_ui(b, texts[i], SIZE_BIG);
        list_add(b, &d->buttons);

        y += MISCBTN_H+20;
        if(i == 2)
            y += 30;
    }

    fb_text *text = fb_add_text(HEADER_WIDTH+5, fb_height-16, WHITE, SIZE_SMALL, "MultiROM v%d with trampoline v%d.",
                               VERSION_MULTIROM, multirom_get_trampoline_ver());
    list_add(text, &d->ui_elements);

    char bat_text[16];
    sprintf(bat_text, "Battery: %d%%", multirom_get_battery());
    text = fb_add_text_long(fb_width-strlen(bat_text)*8, fb_height-16, WHITE, SIZE_SMALL, bat_text);
    list_add(text, &d->ui_elements);

    x =  HEADER_WIDTH + ((fb_width - HEADER_WIDTH)/2 - (CLRS_MAX*CLRBTN_TOTAL)/2);
    uint32_t p, s;
    fb_rect *r;
    for(i = 0; i < CLRS_MAX; ++i)
    {
        multirom_ui_setup_colors(i, &p, &s);

        if(i == color_scheme)
        {
            r = fb_add_rect(x, CLRBTN_Y, CLRBTN_TOTAL, CLRBTN_TOTAL, WHITE);
            list_add(r, &d->ui_elements);
        }

        r = fb_add_rect(x+CLRBTN_B/2, CLRBTN_Y+CLRBTN_B/2, CLRBTN_W, CLRBTN_W, p);
        list_add(r, &d->ui_elements);

        b = mzalloc(sizeof(button));
        b->x = x;
        b->y = CLRBTN_Y;
        b->w = CLRBTN_TOTAL;
        b->h = CLRBTN_TOTAL;
        b->action = i;
        b->clicked = &multirom_ui_tab_misc_change_clr;
        button_init_ui(b, NULL, 0);
        list_add(b, &d->buttons);

        x += CLRBTN_TOTAL;
    }
}

static int get_tab_width(multirom_theme *t)
{
    return fb_width - HEADER_WIDTH;
}

static int get_tab_height(multirom_theme *t)
{
    return fb_height;
}

static void center_rom_name(tab_data_roms *d, const char *name)
{
    d->rom_name->head.x = center_x(HEADER_WIDTH, fb_width-BOOTBTN_W-HEADER_WIDTH-20, SIZE_NORMAL, name);
}

multirom_theme *init_theme_info_1280x800(void)
{
    multirom_theme *t = mzalloc(sizeof(multirom_theme));
    t->width = 1280;
    t->height = 800;

    t->destroy = &destroy;
    t->init_header = &init_header;
    t->header_select = &header_select;
    t->tab_rom_init = &tab_rom_init;
    t->tab_misc_init = &tab_misc_init;
    t->get_tab_width = &get_tab_width;
    t->get_tab_height = &get_tab_height;
    t->center_rom_name = &center_rom_name;

    return t;
}
