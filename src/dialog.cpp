#include "dialog.hpp"

extern "C"
{
#include "style.h"
#include "utils.h"
}
#include "pkgi.hpp"

namespace
{
typedef enum
{
    DialogNone,
    DialogMessage,
    DialogError,
} DialogType;

DialogType dialog_type;
std::string dialog_title;
std::string dialog_text;
std::string dialog_extra;
int dialog_allow_close;
int dialog_cancelled;

int32_t dialog_width;
int32_t dialog_height;
int32_t dialog_delta;
}

void pkgi_dialog_init()
{
    dialog_type = DialogNone;
    dialog_allow_close = 1;
}

int pkgi_dialog_is_open()
{
    return dialog_type != DialogNone;
}

int pkgi_dialog_is_cancelled()
{
    return dialog_cancelled;
}

void pkgi_dialog_allow_close(int allow)
{
    pkgi_dialog_lock();
    dialog_allow_close = allow;
    pkgi_dialog_unlock();
}

void pkgi_dialog_message(const char* text)
{
    pkgi_dialog_lock();

    dialog_text = text;
    dialog_title = "";
    dialog_extra = "";

    dialog_cancelled = 0;
    dialog_type = DialogMessage;
    dialog_delta = 1;

    pkgi_dialog_unlock();
}

void pkgi_dialog_error(const char* text)
{
    LOGF("Error dialog: {}", text);

    pkgi_dialog_lock();

    dialog_title = "ERROR";
    dialog_text = text;
    dialog_extra = "";

    dialog_cancelled = 0;
    dialog_type = DialogError;
    dialog_delta = 1;

    pkgi_dialog_unlock();
}

void pkgi_dialog_close()
{
    dialog_delta = -1;
}

void pkgi_do_dialog(pkgi_input* input)
{
    pkgi_dialog_lock();

    if (dialog_allow_close)
    {
        if ((dialog_type == DialogMessage || dialog_type == DialogError) &&
            (input->pressed & pkgi_ok_button()))
        {
            dialog_delta = -1;
        }
    }

    if (dialog_delta != 0)
    {
        dialog_width +=
                dialog_delta *
                (int32_t)(input->delta * PKGI_ANIMATION_SPEED / 1000000);
        dialog_height +=
                dialog_delta *
                (int32_t)(input->delta * PKGI_ANIMATION_SPEED / 1000000);

        if (dialog_delta < 0 && (dialog_width <= 0 || dialog_height <= 0))
        {
            dialog_type = DialogNone;
            dialog_text = "";
            dialog_extra[0] = 0;

            dialog_width = 0;
            dialog_height = 0;
            dialog_delta = 0;
            pkgi_dialog_unlock();
            return;
        }
        else if (dialog_delta > 0)
        {
            if (dialog_width >= PKGI_DIALOG_WIDTH &&
                dialog_height >= PKGI_DIALOG_HEIGHT)
            {
                dialog_delta = 0;
            }
            dialog_width = min32(dialog_width, PKGI_DIALOG_WIDTH);
            dialog_height = min32(dialog_height, PKGI_DIALOG_HEIGHT);
        }
    }

    DialogType local_type = dialog_type;
    std::string local_title;
    std::string local_text;
    std::string local_extra;
    int local_allow_close = dialog_allow_close;
    int32_t local_width = dialog_width;
    int32_t local_height = dialog_height;

    local_title = dialog_title;
    local_text = dialog_text;
    local_extra = dialog_extra;

    pkgi_dialog_unlock();

    if (local_width != 0 && local_height != 0)
    {
        pkgi_draw_rect(
                (VITA_WIDTH - local_width) / 2,
                (VITA_HEIGHT - local_height) / 2,
                local_width,
                local_height,
                PKGI_COLOR_MENU_BACKGROUND);
    }

    if (local_width != PKGI_DIALOG_WIDTH || local_height != PKGI_DIALOG_HEIGHT)
    {
        return;
    }

    int font_height = pkgi_text_height("M");

    int w = VITA_WIDTH - 2 * PKGI_DIALOG_HMARGIN;
    int h = VITA_HEIGHT - 2 * PKGI_DIALOG_VMARGIN;

    if (!local_title.empty())
    {
        uint32_t color;
        if (local_type == DialogError)
        {
            color = PKGI_COLOR_TEXT_ERROR;
        }
        else
        {
            color = PKGI_COLOR_TEXT_DIALOG;
        }

        int width = pkgi_text_width(local_title.c_str());
        if (width > w + 2 * PKGI_DIALOG_PADDING)
        {
            pkgi_clip_set(
                    PKGI_DIALOG_HMARGIN + PKGI_DIALOG_PADDING,
                    PKGI_DIALOG_VMARGIN + PKGI_DIALOG_PADDING,
                    w - 2 * PKGI_DIALOG_PADDING,
                    h - 2 * PKGI_DIALOG_PADDING);
            pkgi_draw_text(
                    (VITA_WIDTH - width) / 2,
                    PKGI_DIALOG_VMARGIN + font_height,
                    color,
                    local_title.c_str());
            pkgi_clip_remove();
        }
        else
        {
            pkgi_draw_text(
                    (VITA_WIDTH - width) / 2,
                    PKGI_DIALOG_VMARGIN + font_height,
                    color,
                    local_title.c_str());
        }
    }

    uint32_t color;
    if (local_type == DialogMessage)
    {
        color = PKGI_COLOR_TEXT_DIALOG;
    }
    else // local_type == DialogError
    {
        color = PKGI_COLOR_TEXT_ERROR;
    }

    int textw = pkgi_text_width(local_text.c_str());
    if (textw > w + 2 * PKGI_DIALOG_PADDING)
    {
        pkgi_clip_set(
                PKGI_DIALOG_HMARGIN + PKGI_DIALOG_PADDING,
                PKGI_DIALOG_VMARGIN + PKGI_DIALOG_PADDING,
                w - 2 * PKGI_DIALOG_PADDING,
                h - 2 * PKGI_DIALOG_PADDING);
        pkgi_draw_text(
                PKGI_DIALOG_HMARGIN + PKGI_DIALOG_PADDING,
                VITA_HEIGHT / 2 - font_height / 2,
                color,
                local_text.c_str());
        pkgi_clip_remove();
    }
    else
    {
        pkgi_draw_text(
                (VITA_WIDTH - textw) / 2,
                VITA_HEIGHT / 2 - font_height / 2,
                color,
                local_text.c_str());
    }

    if (local_allow_close)
    {
        std::string text = fmt::format(
                "press {} to close",
                pkgi_ok_button() == PKGI_BUTTON_X ? PKGI_UTF8_X : PKGI_UTF8_O);
        pkgi_draw_text(
                (VITA_WIDTH - pkgi_text_width(text.c_str())) / 2,
                PKGI_DIALOG_VMARGIN + h - 2 * font_height,
                PKGI_COLOR_TEXT_DIALOG,
                text.c_str());
    }
}
