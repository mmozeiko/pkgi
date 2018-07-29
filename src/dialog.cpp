extern "C" {
#include "dialog.h"

#include "style.h"
#include "utils.h"
}
#include "pkgi.hpp"

typedef enum {
    DialogNone,
    DialogMessage,
    DialogError,
} DialogType;

static DialogType dialog_type;
static char dialog_title[256];
static char dialog_text[256];
static char dialog_extra[256];
static int dialog_allow_close;
static int dialog_cancelled;

static int32_t dialog_width;
static int32_t dialog_height;
static int32_t dialog_delta;

void pkgi_dialog_init(void)
{
    dialog_type = DialogNone;
    dialog_allow_close = 1;
}

int pkgi_dialog_is_open(void)
{
    return dialog_type != DialogNone;
}

int pkgi_dialog_is_cancelled(void)
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

    pkgi_strncpy(dialog_text, sizeof(dialog_text), text);
    dialog_title[0] = 0;
    dialog_extra[0] = 0;

    dialog_cancelled = 0;
    dialog_type = DialogMessage;
    dialog_delta = 1;

    pkgi_dialog_unlock();
}

void pkgi_dialog_error(const char* text)
{
    LOGF("Error dialog: {}", text);

    pkgi_dialog_lock();

    pkgi_strncpy(dialog_title, sizeof(dialog_title), "ERROR");
    pkgi_strncpy(dialog_text, sizeof(dialog_text), text);
    dialog_extra[0] = 0;

    dialog_cancelled = 0;
    dialog_type = DialogError;
    dialog_delta = 1;

    pkgi_dialog_unlock();
}

void pkgi_dialog_close(void)
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
            dialog_text[0] = 0;
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
    char local_title[256];
    char local_text[256];
    char local_extra[256];
    int local_allow_close = dialog_allow_close;
    int32_t local_width = dialog_width;
    int32_t local_height = dialog_height;

    pkgi_strncpy(local_title, sizeof(local_title), dialog_title);
    pkgi_strncpy(local_text, sizeof(local_text), dialog_text);
    pkgi_strncpy(local_extra, sizeof(local_extra), dialog_extra);

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

    if (local_title[0])
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

        int width = pkgi_text_width(local_title);
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
                    local_title);
            pkgi_clip_remove();
        }
        else
        {
            pkgi_draw_text(
                    (VITA_WIDTH - width) / 2,
                    PKGI_DIALOG_VMARGIN + font_height,
                    color,
                    local_title);
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

    int textw = pkgi_text_width(local_text);
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
                local_text);
        pkgi_clip_remove();
    }
    else
    {
        pkgi_draw_text(
                (VITA_WIDTH - textw) / 2,
                VITA_HEIGHT / 2 - font_height / 2,
                color,
                local_text);
    }

    if (local_allow_close)
    {
        char text[256];
        pkgi_snprintf(
                text,
                sizeof(text),
                "press %s to close",
                pkgi_ok_button() == PKGI_BUTTON_X ? PKGI_UTF8_X : PKGI_UTF8_O);
        pkgi_draw_text(
                (VITA_WIDTH - pkgi_text_width(text)) / 2,
                PKGI_DIALOG_VMARGIN + h - 2 * font_height,
                PKGI_COLOR_TEXT_DIALOG,
                text);
    }
}
