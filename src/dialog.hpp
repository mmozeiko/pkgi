#pragma once

#include <functional>
#include <string>

typedef struct pkgi_input pkgi_input;

struct Response
{
    std::string text;
    std::function<void()> callback;
};

void pkgi_dialog_init();

int pkgi_dialog_is_open();
int pkgi_dialog_is_cancelled();
void pkgi_dialog_allow_close(int allow);
void pkgi_dialog_message(const char* text, int allow_close = 1);
void pkgi_dialog_error(const char* text);
void pkgi_dialog_question(
        const std::string& text, const std::vector<Response>& responses);

void pkgi_dialog_close();

void pkgi_do_dialog();
