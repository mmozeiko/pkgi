#pragma once

typedef struct pkgi_input pkgi_input;

void pkgi_dialog_init();

int pkgi_dialog_is_open();
int pkgi_dialog_is_cancelled();
void pkgi_dialog_allow_close(int allow);
void pkgi_dialog_message(const char* text, int allow_close = 1);
void pkgi_dialog_error(const char* text);

void pkgi_dialog_close();

void pkgi_do_dialog();
