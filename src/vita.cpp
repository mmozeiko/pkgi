#include "pkgi.hpp"

extern "C"
{
#include "style.h"
}
#include "config.hpp"
#include "db.hpp"
#include "file.hpp"
#include "http.hpp"
#include "log.hpp"

#include <fmt/format.h>

#include <boost/scope_exit.hpp>

#include <string>

#include <vita2d.h>

#include <psp2/appmgr.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/ime_dialog.h>
#include <psp2/io/devctl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/libssl.h>
#include <psp2/net/http.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/power.h>
#include <psp2/promoterutil.h>
#include <psp2/shellutil.h>
#include <psp2/sqlite.h>
#include <psp2/sysmodule.h>
#include <psp2/vshbridge.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static vita2d_pgf* g_font;

static SceKernelLwMutexWork g_dialog_lock;
static volatile int g_power_lock;

static int g_ok_button;
static int g_cancel_button;
static uint32_t g_button_frame_count;

static SceUInt64 g_time;

#ifdef PKGI_ENABLE_LOGGING
static int g_log_socket;
#endif

#define ANALOG_CENTER 128
#define ANALOG_THRESHOLD 64
#define ANALOG_SENSITIVITY 16

#define VITA_COLOR(c) RGBA8((c)&0xff, (c >> 8) & 0xff, (c >> 16) & 0xff, 255)

#define PKGI_ERRNO_ENOENT (int)(0x80010000 + SCE_NET_ENOENT)

#ifdef PKGI_ENABLE_LOGGING
void pkgi_log(const char* msg, ...)
{
    char buffer[512];

    va_list args;
    va_start(args, msg);
    // TODO: why sceClibVsnprintf doesn't work here?
    int len = vsnprintf(buffer, sizeof(buffer) - 1, msg, args);
    va_end(args);
    buffer[len] = '\n';

    sceNetSend(g_log_socket, buffer, len + 1, 0);
    // sceKernelDelayThread(10);
}
#endif

int pkgi_snprintf(char* buffer, uint32_t size, const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    // TODO: why sceClibVsnprintf doesn't work here?
    int len = vsnprintf(buffer, size - 1, msg, args);
    va_end(args);
    buffer[len] = 0;
    return len;
}

void pkgi_vsnprintf(char* buffer, uint32_t size, const char* msg, va_list args)
{
    // TODO: why sceClibVsnprintf doesn't work here?
    int len = vsnprintf(buffer, size - 1, msg, args);
    buffer[len] = 0;
}

char* pkgi_strstr(const char* str, const char* sub)
{
    return strstr(str, sub);
}

int pkgi_stricontains(const char* str, const char* sub)
{
    return strcasestr(str, sub) != NULL;
}

int pkgi_stricmp(const char* a, const char* b)
{
    return strcasecmp(a, b);
}

void pkgi_strncpy(char* dst, uint32_t size, const char* src)
{
    sceClibStrncpy(dst, src, size);
}

char* pkgi_strrchr(const char* str, char ch)
{
    return strrchr(str, ch);
}

void pkgi_memcpy(void* dst, const void* src, uint32_t size)
{
    sceClibMemcpy(dst, src, size);
}

void pkgi_memmove(void* dst, const void* src, uint32_t size)
{
    sceClibMemmove(dst, src, size);
}

int pkgi_memequ(const void* a, const void* b, uint32_t size)
{
    return memcmp(a, b, size) == 0;
}

static void pkgi_start_debug_log(void)
{
#ifdef PKGI_ENABLE_LOGGING
    g_log_socket = sceNetSocket(
            "log_socket",
            SCE_NET_AF_INET,
            SCE_NET_SOCK_DGRAM,
            SCE_NET_IPPROTO_UDP);

    SceNetSockaddrIn addr{};
    addr.sin_family = SCE_NET_AF_INET;
    addr.sin_port = sceNetHtons(30000);
    sceNetInetPton(SCE_NET_AF_INET, "239.255.0.100", &addr.sin_addr);

    sceNetConnect(g_log_socket, (SceNetSockaddr*)&addr, sizeof(addr));
    LOG("debug logging socket initialized");
#endif
}

static void pkgi_stop_debug_log(void)
{
#ifdef PKGI_ENABLE_LOGGING
    sceNetSocketClose(g_log_socket);
#endif
}

// TODO: this is from VitaShell
// no idea why, but this seems to be required for promoter utility functions to
// work
static void pkgi_load_sce_paf()
{
    uint32_t args[] = {
            0x00400000,
            0x0000ea60,
            0x00040000,
            0x00000000,
            0x00000001,
            0x00000000,
    };

    uint32_t result = 0xDEADBEEF;

    uint32_t buffer[4] = {};
    buffer[1] = (uint32_t)&result;

    sceSysmoduleLoadModuleInternalWithArg(
            SCE_SYSMODULE_INTERNAL_PAF, sizeof(args), args, buffer);
}

int pkgi_is_unsafe_mode(void)
{
    return sceIoDevctl("ux0:", 0x3001, NULL, 0, NULL, 0) != (int)0x80010030;
}

int pkgi_ok_button(void)
{
    return g_ok_button;
}

int pkgi_cancel_button(void)
{
    return g_cancel_button;
}

static int pkgi_power_thread(SceSize args, void* argp)
{
    PKGI_UNUSED(args);
    PKGI_UNUSED(argp);
    for (;;)
    {
        int lock;
        __atomic_load(&g_power_lock, &lock, __ATOMIC_SEQ_CST);
        if (lock > 0)
        {
            sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
        }

        sceKernelDelayThread(10 * 1000 * 1000);
    }
    return 0;
}

void pkgi_dialog_lock(void)
{
    int res = sceKernelLockLwMutex(&g_dialog_lock, 1, NULL);
    if (res < 0)
    {
        LOG("dialog unlock failed error=0x%08x", res);
    }
}

void pkgi_dialog_unlock(void)
{
    int res = sceKernelUnlockLwMutex(&g_dialog_lock, 1);
    if (res < 0)
    {
        LOG("dialog lock failed error=0x%08x", res);
    }
}

static int g_ime_active;

static uint16_t g_ime_title[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
static uint16_t g_ime_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
static uint16_t g_ime_input[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];

static int convert_to_utf16(
        const char* utf8, uint16_t* utf16, uint32_t available)
{
    int count = 0;
    while (*utf8)
    {
        uint8_t ch = (uint8_t)*utf8++;
        uint32_t code;
        uint32_t extra;

        if (ch < 0x80)
        {
            code = ch;
            extra = 0;
        }
        else if ((ch & 0xe0) == 0xc0)
        {
            code = ch & 31;
            extra = 1;
        }
        else if ((ch & 0xf0) == 0xe0)
        {
            code = ch & 15;
            extra = 2;
        }
        else
        {
            // TODO: this assumes there won't be invalid utf8 codepoints
            code = ch & 7;
            extra = 3;
        }

        for (uint32_t i = 0; i < extra; i++)
        {
            uint8_t next = (uint8_t)*utf8++;
            if (next == 0 || (next & 0xc0) != 0x80)
            {
                return count;
            }
            code = (code << 6) | (next & 0x3f);
        }

        if (code < 0xd800 || code >= 0xe000)
        {
            if (available < 1)
                return count;
            utf16[count++] = (uint16_t)code;
            available--;
        }
        else // surrogate pair
        {
            if (available < 2)
                return count;
            code -= 0x10000;
            utf16[count++] = 0xd800 | (code >> 10);
            utf16[count++] = 0xdc00 | (code & 0x3ff);
            available -= 2;
        }
    }
    return count;
}

static int convert_from_utf16(const uint16_t* utf16, char* utf8, uint32_t size)
{
    int count = 0;
    while (*utf16)
    {
        uint32_t code;
        uint16_t ch = *utf16++;
        if (ch < 0xd800 || ch >= 0xe000)
        {
            code = ch;
        }
        else // surrogate pair
        {
            uint16_t ch2 = *utf16++;
            if (ch < 0xdc00 || ch > 0xe000 || ch2 < 0xd800 || ch2 > 0xdc00)
            {
                return count;
            }
            code = 0x10000 + ((ch & 0x03FF) << 10) + (ch2 & 0x03FF);
        }

        if (code < 0x80)
        {
            if (size < 1)
                return count;
            utf8[count++] = (char)code;
            size--;
        }
        else if (code < 0x800)
        {
            if (size < 2)
                return count;
            utf8[count++] = (char)(0xc0 | (code >> 6));
            utf8[count++] = (char)(0x80 | (code & 0x3f));
            size -= 2;
        }
        else if (code < 0x10000)
        {
            if (size < 3)
                return count;
            utf8[count++] = (char)(0xe0 | (code >> 12));
            utf8[count++] = (char)(0x80 | ((code >> 6) & 0x3f));
            utf8[count++] = (char)(0x80 | (code & 0x3f));
            size -= 3;
        }
        else
        {
            if (size < 4)
                return count;
            utf8[count++] = (char)(0xf0 | (code >> 18));
            utf8[count++] = (char)(0x80 | ((code >> 12) & 0x3f));
            utf8[count++] = (char)(0x80 | ((code >> 6) & 0x3f));
            utf8[count++] = (char)(0x80 | (code & 0x3f));
            size -= 4;
        }
    }
    return count;
}

void pkgi_dialog_input_text(const char* title, const char* text)
{
    SceImeDialogParam param;
    sceImeDialogParamInit(&param);

    int title_len =
            convert_to_utf16(title, g_ime_title, PKGI_COUNTOF(g_ime_title) - 1);
    int text_len =
            convert_to_utf16(text, g_ime_text, PKGI_COUNTOF(g_ime_text) - 1);
    g_ime_title[title_len] = 0;
    g_ime_text[text_len] = 0;

    param.supportedLanguages = 0x0001FFFF;
    param.languagesForced = SCE_TRUE;
    param.type = SCE_IME_TYPE_DEFAULT;
    param.option = 0;
    param.title = g_ime_title;
    param.maxTextLength = 128;
    param.initialText = g_ime_text;
    param.inputTextBuffer = g_ime_input;

    int res = sceImeDialogInit(&param);
    if (res < 0)
    {
        LOG("sceImeDialogInit failed, error 0x%08x", res);
    }
    else
    {
        g_ime_active = 1;
    }
}

int pkgi_dialog_input_update(void)
{
    if (!g_ime_active)
    {
        return 0;
    }

    SceCommonDialogStatus status = sceImeDialogGetStatus();
    if (status == SCE_COMMON_DIALOG_STATUS_FINISHED)
    {
        SceImeDialogResult result{};
        sceImeDialogGetResult(&result);

        g_ime_active = 0;
        sceImeDialogTerm();

        if (result.button == SCE_IME_DIALOG_BUTTON_ENTER)
        {
            return 1;
        }
    }

    return 0;
}

void pkgi_dialog_input_get_text(char* text, uint32_t size)
{
    int count = convert_from_utf16(g_ime_input, text, size - 1);
    text[count] = 0;
}

void pkgi_start(void)
{
    pkgi_load_sce_paf();
    sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    sceSysmoduleLoadModule(SCE_SYSMODULE_HTTP);
    sceSysmoduleLoadModule(SCE_SYSMODULE_SSL);
    sceSysmoduleLoadModule(SCE_SYSMODULE_SQLITE);

    static uint8_t netmem[1024 * 1024];
    SceNetInitParam net = {
            .memory = netmem,
            .size = sizeof(netmem),
            .flags = 0,
    };

    sceNetInit(&net);
    sceNetCtlInit();

    pkgi_start_debug_log();

    LOG("initializing SSL");
    sceSslInit(1024 * 1024);
    LOG("initializing HTTP");
    sceHttpInit(1024 * 1024);
    LOG("network initialized");

    sceHttpsDisableOption(SCE_HTTPS_FLAG_SERVER_VERIFY);

    sceKernelCreateLwMutex(&g_dialog_lock, "dialog_lock", 2, 0, NULL);

    scePowerSetArmClockFrequency(444);

    sceShellUtilInitEvents(0);
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_USB_CONNECTION);

    SceAppUtilInitParam init{};
    SceAppUtilBootParam boot{};
    sceAppUtilInit(&init, &boot);

    SceCommonDialogConfigParam config;
    sceCommonDialogConfigParamInit(&config);
    sceAppUtilSystemParamGetInt(
            SCE_SYSTEM_PARAM_ID_LANG, (int*)&config.language);
    sceAppUtilSystemParamGetInt(
            SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int*)&config.enterButtonAssign);
    sceCommonDialogSetConfigParam(&config);

    if (config.enterButtonAssign == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE)
    {
        g_ok_button = PKGI_BUTTON_O;
        g_cancel_button = PKGI_BUTTON_X;
    }
    else
    {
        g_ok_button = PKGI_BUTTON_X;
        g_cancel_button = PKGI_BUTTON_O;
    }

    if (scePromoterUtilityInit() < 0)
    {
        LOG("cannot initialize promoter utility");
    }

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    g_power_lock = 0;
    SceUID power_thread = sceKernelCreateThread(
            "power_thread",
            &pkgi_power_thread,
            0x10000100,
            0x40000,
            0,
            0,
            NULL);
    if (power_thread >= 0)
    {
        sceKernelStartThread(power_thread, 0, NULL);
    }

    vita2d_init_advanced(4 * 1024 * 1024);
    g_font = vita2d_load_default_pgf();

    g_time = sceKernelGetProcessTimeWide();

    sqlite3_rw_init();
    LOG("start done");
}

int pkgi_update(pkgi_input* input)
{
    SceCtrlData pad{};
    sceCtrlPeekBufferPositive(0, &pad, 1);

    uint32_t previous = input->down;

    input->down = pad.buttons;
    if (pad.lx < ANALOG_CENTER - ANALOG_THRESHOLD)
        input->down |= PKGI_BUTTON_LEFT;
    if (pad.lx > ANALOG_CENTER + ANALOG_THRESHOLD)
        input->down |= PKGI_BUTTON_RIGHT;
    if (pad.ly < ANALOG_CENTER - ANALOG_THRESHOLD)
        input->down |= PKGI_BUTTON_UP;
    if (pad.ly > ANALOG_CENTER + ANALOG_THRESHOLD)
        input->down |= PKGI_BUTTON_DOWN;

    input->pressed = input->down & ~previous;
    input->active = input->pressed;

    if (input->down == previous)
    {
        if (g_button_frame_count >= 10)
        {
            input->active = input->down;
        }
        g_button_frame_count++;
    }
    else
    {
        g_button_frame_count = 0;
    }

    vita2d_start_drawing();

    uint64_t time = sceKernelGetProcessTimeWide();
    input->delta = time - g_time;
    g_time = time;

    return 1;
}

void pkgi_swap(void)
{
    // LOG("vita2d pool free space = %u KB", vita2d_pool_free_space() / 1024);

    vita2d_end_drawing();
    vita2d_common_dialog_update();
    vita2d_swap_buffers();
    sceDisplayWaitVblankStart();
}

void pkgi_end(void)
{
    pkgi_stop_debug_log();

    vita2d_fini();
    vita2d_free_pgf(g_font);

    scePromoterUtilityExit();

    sceAppUtilShutdown();

    sceKernelDeleteLwMutex(&g_dialog_lock);

    sceHttpTerm();
    // sceSslTerm();
    sceNetCtlTerm();
    sceNetTerm();

    sceSysmoduleUnloadModule(SCE_SYSMODULE_SSL);
    sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTP);
    sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
    sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);

    sceKernelExitProcess(0);
}

int pkgi_battery_present()
{
    return sceKernelGetModel() == SCE_KERNEL_MODEL_VITA;
}

int pkgi_bettery_get_level()
{
    return scePowerGetBatteryLifePercent();
}

int pkgi_battery_is_low()
{
    return scePowerIsLowBattery() && !scePowerIsBatteryCharging();
}

int pkgi_battery_is_charging()
{
    return scePowerIsBatteryCharging();
}

uint64_t pkgi_get_free_space(const char* requested_partition)
{
    SceIoDevInfo info{};
    sceIoDevctl(requested_partition, 0x3001, NULL, 0, &info, sizeof(info));
    return info.free_size;
}

const char* pkgi_get_config_folder()
{
    if (0)
    {
    }
#define CHECK_FOLDER(f) else if (pkgi_file_exists(f "/config.txt")) return f
    CHECK_FOLDER("ur0:pkgj");
    CHECK_FOLDER("ux0:pkgj");
    CHECK_FOLDER("ur0:pkgi");
    CHECK_FOLDER("ux0:pkgi");
#undef CHECK_FOLDER
    else throw std::runtime_error("no config.txt found");
}

int pkgi_is_incomplete(const char* partition, const char* contentid)
{
    return pkgi_file_exists(
            fmt::format("{}pkgj/{}.resume", partition, contentid).c_str());
}

void pkgi_delete_dir(const std::string& path)
{
    SceUID dfd = sceIoDopen(path.c_str());
    if (dfd == PKGI_ERRNO_ENOENT)
        return;

    if (dfd < 0)
        throw formatEx<std::runtime_error>(
                "failed sceIoDopen({}):\n{:#08x}",
                path,
                static_cast<uint32_t>(dfd));

    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (dfd >= 0)
            sceIoClose(dfd);
    };

    int res = 0;
    SceIoDirent dir;
    memset(&dir, 0, sizeof(SceIoDirent));
    while ((res = sceIoDread(dfd, &dir)) > 0)
    {
        std::string d_name = dir.d_name;

        if (d_name == "." || d_name == "..")
            continue;

        std::string new_path =
                path + (path[path.size() - 1] == '/' ? "" : "/") + d_name;

        if (SCE_S_ISDIR(dir.d_stat.st_mode))
            pkgi_delete_dir(new_path);
        else
        {
            const auto ret = sceIoRemove(new_path.c_str());
            if (ret < 0)
                throw formatEx<std::runtime_error>(
                        "failed sceIoRemove({}):\n{:#08x}",
                        new_path,
                        static_cast<uint32_t>(ret));
        }
    }

    sceIoDclose(dfd);
    dfd = -1;

    res = sceIoRmdir(path.c_str());
    if (res < 0)
        throw formatEx<std::runtime_error>(
                "failed sceIoRmdir({}):\n{:#08x}",
                path,
                static_cast<uint32_t>(res));
}

uint32_t pkgi_time_msec()
{
    return sceKernelGetProcessTimeLow() / 1000;
}

static int pkgi_vita_thread(SceSize args, void* argp)
{
    PKGI_UNUSED(args);
    pkgi_thread_entry* start = *((pkgi_thread_entry**)argp);
    start();
    return sceKernelExitDeleteThread(0);
}

void pkgi_start_thread(const char* name, pkgi_thread_entry* start)
{
    SceUID id = sceKernelCreateThread(
            name, &pkgi_vita_thread, 0x40, 1024 * 1024, 0, 0, NULL);
    if (id < 0)
    {
        LOG("failed to start %s thread", name);
    }
    else
    {
        sceKernelStartThread(id, sizeof(start), &start);
    }
}

void pkgi_sleep(uint32_t msec)
{
    sceKernelDelayThread(msec * 1000);
}

void pkgi_lock_process(void)
{
    if (__atomic_fetch_add(&g_power_lock, 1, __ATOMIC_SEQ_CST) == 0)
    {
        LOG("locking shell functionality");
        if (sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN) < 0)
        {
            LOG("sceShellUtilLock failed");
        }
    }
}

void pkgi_unlock_process(void)
{
    if (__atomic_sub_fetch(&g_power_lock, 1, __ATOMIC_SEQ_CST) == 0)
    {
        LOG("unlocking shell functionality");
        if (sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN) < 0)
        {
            LOG("sceShellUtilUnlock failed");
        }
    }
}

pkgi_texture pkgi_load_png_raw(const void* data, uint32_t size)
{
    (void)size;
    vita2d_texture* tex = vita2d_load_PNG_buffer((const char*)data);
    if (!tex)
    {
        LOG("failed to load texture");
    }
    return tex;
}

void pkgi_draw_texture(pkgi_texture texture, int x, int y)
{
    vita2d_texture* tex = static_cast<vita2d_texture*>(texture);
    vita2d_draw_texture(tex, (float)x, (float)y);
}

void pkgi_clip_set(int x, int y, int w, int h)
{
    vita2d_enable_clipping();
    vita2d_set_clip_rectangle(x, y, x + w - 1, y + h - 1);
}

void pkgi_clip_remove(void)
{
    vita2d_disable_clipping();
}

void pkgi_draw_rect(int x, int y, int w, int h, uint32_t color)
{
    vita2d_draw_rectangle(
            (float)x, (float)y, (float)w, (float)h, VITA_COLOR(color));
}

void pkgi_draw_text(int x, int y, uint32_t color, const char* text)
{
    vita2d_pgf_draw_text(g_font, x, y + 20, VITA_COLOR(color), 1.f, text);
}

int pkgi_text_width(const char* text)
{
    return vita2d_pgf_text_width(g_font, 1.f, text);
}

int pkgi_text_height(const char* text)
{
    PKGI_UNUSED(text);
    // return vita2d_pgf_text_height(g_font, 1.f, text);
    return 23;
}

std::string pkgi_get_system_version()
{
    SceKernelFwInfo info{};
    info.size = sizeof(info);
    const auto res = _vshSblGetSystemSwVersion(&info);
    if (res < 0)
        throw std::runtime_error(fmt::format(
                "sceKernelGetSystemSwVersion failed: {:#08x}",
                static_cast<uint32_t>(res)));
    return info.versionString;
}
