#pragma once

#include "pkgi_db.h"

typedef struct pkgi_input pkgi_input;

typedef enum {
    MenuResultSearch,
    MenuResultSearchClear,
    MenuResultAccept,
    MenuResultCancel,
    MenuResultRefreshGames,
    MenuResultRefreshUpdates,
    MenuResultRefreshDlcs,
    MenuResultRefreshPsxGames,
    MenuResultRefreshPspGames,
} MenuResult;

int pkgi_menu_is_open(void);
void pkgi_menu_get(Config* config);
MenuResult pkgi_menu_result(void);

void pkgi_menu_start(int search_clear, const Config* config, int allow_update);

int pkgi_do_menu(pkgi_input* input);
