#pragma once

#include "log.hpp"

#include <string>

#include <stdarg.h>
#include <stdint.h>

// values compatible with psp2/ctrl.h header
#define PKGI_BUTTON_INTERCEPTED 0x00010000
#define PKGI_BUTTON_SELECT 0x00000001
#define PKGI_BUTTON_START 0x00000008

#define PKGI_BUTTON_UP 0x00000010
#define PKGI_BUTTON_RIGHT 0x00000020
#define PKGI_BUTTON_DOWN 0x00000040
#define PKGI_BUTTON_LEFT 0x00000080

#define PKGI_BUTTON_LT 0x00000100
#define PKGI_BUTTON_RT 0x00000200

#define PKGI_BUTTON_X 0x00004000 // cross
#define PKGI_BUTTON_O 0x00002000 // circle
#define PKGI_BUTTON_T 0x00001000 // triangle
#define PKGI_BUTTON_S 0x00008000 // square

#define PKGI_UNUSED(x) (void)(x)

typedef struct pkgi_input
{
    uint64_t delta; // microseconds from previous frame

    uint32_t pressed; // button pressed in last frame
    uint32_t down; // button is currently down
    uint32_t active; // button is pressed in last frame, or held down for a long
                     // time (10 frames)
} pkgi_input;

#define PKGI_COUNTOF(arr) (sizeof(arr) / sizeof(0 [arr]))

int pkgi_snprintf(char* buffer, uint32_t size, const char* msg, ...);
void pkgi_vsnprintf(char* buffer, uint32_t size, const char* msg, va_list args);
char* pkgi_strstr(const char* str, const char* sub);
int pkgi_stricontains(const char* str, const char* sub);
int pkgi_stricmp(const char* a, const char* b);
void pkgi_strncpy(char* dst, uint32_t size, const char* src);
char* pkgi_strrchr(const char* str, char ch);
void pkgi_memcpy(void* dst, const void* src, uint32_t size);
void pkgi_memmove(void* dst, const void* src, uint32_t size);
int pkgi_memequ(const void* a, const void* b, uint32_t size);

int pkgi_is_unsafe_mode(void);

int pkgi_ok_button(void);
int pkgi_cancel_button(void);

void pkgi_start(void);
int pkgi_update(pkgi_input* input);
void pkgi_swap(void);
void pkgi_end(void);

int pkgi_battery_present();
int pkgi_bettery_get_level();
int pkgi_battery_is_low();
int pkgi_battery_is_charging();

uint64_t pkgi_get_free_space(const char*);
const char* pkgi_get_config_folder(void);
int pkgi_is_incomplete(const char* partition, const char* titleid);

uint32_t pkgi_time_msec();

typedef void pkgi_thread_entry(void);
void pkgi_start_thread(const char* name, pkgi_thread_entry* start);
void pkgi_sleep(uint32_t msec);

void pkgi_lock_process(void);
void pkgi_unlock_process(void);

void pkgi_dialog_lock(void);
void pkgi_dialog_unlock(void);

void pkgi_dialog_input_text(const char* title, const char* text);
int pkgi_dialog_input_update(void);
void pkgi_dialog_input_get_text(char* text, uint32_t size);

int pkgi_check_free_space(uint64_t http_length);

std::string pkgi_get_system_version();

// UI stuff
typedef void* pkgi_texture;
#ifdef _MSC_VER
#define pkgi_load_png(name) pkgi_load_png_raw(#name##".png", 0)
#else
#define pkgi_load_png(name)                                   \
    ({                                                        \
        extern uint8_t _binary_assets_##name##_png_start;     \
        extern uint8_t _binary_assets_##name##_png_end;       \
        pkgi_load_png_raw(                                    \
                (void*)&_binary_assets_##name##_png_start,    \
                (uint32_t)(                                   \
                        &_binary_assets_##name##_png_end -    \
                        &_binary_assets_##name##_png_start)); \
    })
#endif

pkgi_texture pkgi_load_png_raw(const void* data, uint32_t size);
void pkgi_draw_texture(pkgi_texture texture, int x, int y);

void pkgi_clip_set(int x, int y, int w, int h);
void pkgi_clip_remove(void);
void pkgi_draw_rect(int x, int y, int w, int h, uint32_t color);
void pkgi_draw_text(int x, int y, uint32_t color, const char* text);
int pkgi_text_width(const char* text);
int pkgi_text_height(const char* text);

class Downloader;
struct DbItem;


void pkgi_create_psp_rif(std::string contentid, uint8_t* rif);

void pkgi_start_download(Downloader& downloader, const DbItem& item);

bool pkgi_is_module_present(const char* module_name);
