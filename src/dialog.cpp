#include "dialog.hpp"

extern "C"
{
#include "style.h"
#include "utils.h"
}
#include "pkgi.hpp"

#include <imgui.h>

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
int dialog_allow_close;
int dialog_cancelled;
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

    dialog_cancelled = 0;
    dialog_type = DialogMessage;

    pkgi_dialog_unlock();
}

void pkgi_dialog_error(const char* text)
{
    LOGF("Error dialog: {}", text);

    pkgi_dialog_lock();

    dialog_title = "ERROR";
    dialog_text = text;

    dialog_cancelled = 0;
    dialog_type = DialogError;

    pkgi_dialog_unlock();
}

void pkgi_dialog_close()
{
    pkgi_dialog_lock();
    dialog_type = DialogNone;
    pkgi_dialog_unlock();
}

void pkgi_do_dialog()
{
    pkgi_dialog_lock();

    DialogType local_type = dialog_type;
    std::string local_title;
    std::string local_text;
    int local_allow_close = dialog_allow_close;

    local_title = dialog_title;
    local_text = dialog_text;

    pkgi_dialog_unlock();

    ImGui::SetNextWindowPos(
            ImVec2{VITA_WIDTH / 2, VITA_HEIGHT / 2}, 0, ImVec2{.5f, .5f});
    ImGui::SetNextWindowSize(ImVec2{PKGI_DIALOG_WIDTH, -1});
    ImGui::Begin(
            local_title.c_str(),
            nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoInputs);
    ImGui::PushTextWrapPos(0.f);
    if (local_type == DialogMessage)
        ImGui::TextUnformatted(local_text.c_str());
    else
        ImGui::TextColored(
                ImVec4{1.f, .2f, .2f, 1.f}, "%s", local_text.c_str());
    ImGui::PopTextWrapPos();
    ImGui::Separator();
    if (local_allow_close)
    {
        if (ImGui::Button(
                    "OK", ImVec2{ImGui::GetWindowContentRegionWidth(), 20}))
        {
            pkgi_dialog_lock();
            dialog_type = DialogNone;
            pkgi_dialog_unlock();
        }
        ImGui::SetItemDefaultFocus();
    }
    ImGui::End();
}
