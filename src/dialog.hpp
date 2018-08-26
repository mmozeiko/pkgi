#pragma once

typedef struct pkgi_input pkgi_input;

void pkgi_dialog_init(void);

int pkgi_dialog_is_open(void);
int pkgi_dialog_is_cancelled(void);
void pkgi_dialog_allow_close(int allow);
void pkgi_dialog_message(const char* text);
void pkgi_dialog_error(const char* text);

void pkgi_dialog_close(void);

void pkgi_do_dialog(pkgi_input* input);
