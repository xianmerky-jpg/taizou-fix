#include "taizou_core.h"
#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

#define LOG_TAG "TaizouCore"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace taizou {

static TaizouCore* g_instance = nullptr;

TaizouCore::TaizouCore() {
    g_instance = this;
}

TaizouCore::~TaizouCore() {
    shutdown();
    g_instance = nullptr;
}

bool TaizouCore::initialize(JNIEnv* env, jobject context) {
    if (initialized_) return true;

    env->GetJavaVM(&jvm_);
    global_context_ = env->NewGlobalRef(context);

    jclass context_class = env->GetObjectClass(context);
    jmethodID get_files_dir = env->GetMethodID(context_class, "getFilesDir", "()Ljava/io/File;");
    jobject files_dir = env->CallObjectMethod(context, get_files_dir);
    jclass file_class = env->GetObjectClass(files_dir);
    jmethodID get_absolute_path = env->GetMethodID(file_class, "getAbsolutePath", "()Ljava/lang/String;");
    jstring path_str = (jstring)env->CallObjectMethod(files_dir, get_absolute_path);
    const char* path_cstr = env->GetStringUTFChars(path_str, nullptr);
    files_dir_ = path_cstr;
    config_path_ = std::string(path_cstr) + "/config.json";
    env->ReleaseStringUTFChars(path_str, path_cstr);

    root_available_ = hasRootAccess();

    initializeDefaultConfigs();

    initialized_ = true;
    LOGI("TaizouCore initialized successfully");
    return true;
}

void TaizouCore::shutdown() {
    if (global_context_ && jvm_) {
        JNIEnv* env;
        if (jvm_->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            env->DeleteGlobalRef(global_context_);
            global_context_ = nullptr;
        }
    }
    initialized_ = false;
}

void TaizouCore::initializeDefaultConfigs() {
    checkboxes_ = {
        {"report", {"report", false,
            {{"libunity.so", 0x23A9F2, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xC9146E, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x39D57B, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xF26A31, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x54B2C4, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA9418D, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x19F6B0, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xD85C79, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x49E2AF, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x9C70D6, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x02AC58, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xB4F139, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x15D8E4, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xCE93A4, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x31C7FD, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}},
            {}}},
        {"tut", {"tut", false,
            {{"libunity.so", 0x6479CF0, hexStringToBytes("00 00 A0 E3 1E FF 2F E1")}},
            {}}},
        {"floatmenu4", {"floatmenu4", false,
            {{"libunity.so", 0xA0442D8, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA051778, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA057F8C, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA03F6BC, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA03E210, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}},
            {}}},
        {"memory", {"memory", false,
            {{"libunity.so", 0xA049A50, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA05032C, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA2DD980, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA044D3C, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA053638, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}},
            {}}},
        {"quality", {"quality", false,
            {{"libunity.so", 0xA0554BC, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA044F7C, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA055A74, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA045148, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA055FFC, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA0454C0, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA63A844, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA056E74, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA057F8C, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA05527C, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA04F47C, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xA04F7D4, hexStringToBytes("00 00 80 D2 C0 03 5F D6")}},
            {}}},
        {"wall", {"wall", false,
            {{"libunity.so", 0x548A67C, hexStringToBytes("1F 20 03 D5")}},
            {{"libunity.so", 0x548A67C, hexStringToBytes("80 00 00 36")}}}},
        {"redhack", {"redhack", false,
            {{"libunity.so", 0x9677554, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xAD1FCDC, hexStringToBytes("40 00 00 1C C0 03 5F D6")},
             {"libunity.so", 0xAD1FCDC + 4, hexStringToBytes("C0 03 5F D6 00 00 7A 44")},
             {"libunity.so", 0xAD1FCDC + 8, floatToHexLE(16.0f)},
             {"libunity.so", 0x967755C, hexStringToBytes("40 00 00 1C C0 03 5F D6")},
             {"libunity.so", 0x967755C + 4, hexStringToBytes("C0 03 5F D6 00 00 7A 44")},
             {"libunity.so", 0x967755C + 8, floatToHexLE(16.0f)}},
            {}}},
        {"hit", {"hit", false,
            {{"libunity.so", 0xC1514C0, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}},
            {}}},
        {"fscope", {"fscope", false,
            {{"libunity.so", 0x512B4FC, hexStringToBytes("00 2C 40 BC C0 03 5F D6")}},
            {{"libunity.so", 0x512B4FC, hexStringToBytes("E8 0F 1D FC F4 4F 01 A9")}}}},
        {"fastsw", {"fastsw", false,
            {{"libunity.so", 0x6AAADF8, hexStringToBytes("40 00 00 1C C0 03 5F D6")},
             {"libunity.so", 0x6AAAD8C, hexStringToBytes("40 00 00 1C C0 03 5F D6")}},
            {{"libunity.so", 0x6AAADF8, hexStringToBytes("E8 0F 1D FC")},
             {"libunity.so", 0x6AAAD8C, hexStringToBytes("E8 0F 1D FC")}}}},
        {"advance", {"advance", false,
            {{"libunity.so", 0xC1514C0, hexStringToBytes("20 00 80 52 C0 03 5F D6")}},
            {{"libunity.so", 0x5985F8C, hexStringToBytes("FF 03 02 D1 F8 5F 04 A9")}}}},
        {"spect", {"spect", false,
            {{"libunity.so", 0x904DD18, hexStringToBytes("00 10 20 1E C0 03 5F D6")},
             {"libunity.so", 0x4F7CF08, hexStringToBytes("00 10 20 1E C0 03 5F D6")},
             {"libunity.so", 0xC1EC20C, hexStringToBytes("00 10 20 1E C0 03 5F D6")}},
            {}}},
        {"wo", {"wo", false,
            {{"libunity.so", 0x9677554, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xAD1FCDC, hexStringToBytes("40 00 00 1C C0 03 5F D6")},
             {"libunity.so", 0xAD1FCDC + 4, hexStringToBytes("C0 03 5F D6 00 00 7A 44")},
             {"libunity.so", 0xAD1FCDC + 8, floatToHexLE(16.0f)},
             {"libunity.so", 0x967755C, hexStringToBytes("40 00 00 1C C0 03 5F D6")},
             {"libunity.so", 0x967755C + 4, hexStringToBytes("C0 03 5F D6 00 00 7A 44")},
             {"libunity.so", 0x967755C + 8, floatToHexLE(16.0f)}},
            {}}},
        {"nos", {"nos", false,
            {{"libunity.so", 0xC9B9618, hexStringToBytes("00 2C 40 BC C0 03 5F D6")}},
            {{"libunity.so", 0xC9B9618, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xC9B9618 + 4, hexStringToBytes("00 00 80 D2 C0 03 5F D6")}}}},
        {"noreload", {"noreload", false,
            {{"libunity.so", 0xC14943C, hexStringToBytes("00 2C 40 BC C0 03 5F D6")}},
            {{"libunity.so", 0xC14943C, hexStringToBytes("00 2C 40 BC C0 03 5F D6")}}}},
        {"norecoil", {"norecoil", false,
            {{"libunity.so", 0xC9BAFF8, hexStringToBytes("20 4C 40 BC C0 03 5F D6")},
             {"libunity.so", 0x664B8D0, hexStringToBytes("20 4C 40 BC C0 03 5F D6")}},
            {{"libunity.so", 0xC9BAFF8, hexStringToBytes("20 4C 40 BC C0 03 5F D6")},
             {"libunity.so", 0x664B8D0, hexStringToBytes("20 4C 40 BC C0 03 5F D6")}}}},
        {"speed", {"speed", false,
            {{"libunity.so", 0x51D2EB8, hexStringToBytes("00 10 20 1E C0 03 5F D6")}},
            {{"libunity.so", 0x892385C, hexStringToBytes("00 10 20 1E C0 03 5F D6")}}}},
        {"wo2", {"wo2", false,
            {{"libunity.so", 0x8D98CBC, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}},
            {{"libunity.so", 0x8D98CBC, hexStringToBytes("FE 0F 1E F8 F4 4F 01 A9")}}}},
        {"amo", {"amo", false,
            {{"libunity.so", 0x50EC794, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x50ECA80, hexStringToBytes("00 00 80 D2 C0 03 5F D6")}},
            {}}},
        {"fire", {"fire", false,
            {{"libunity.so", 0x50EBA5C, hexStringToBytes("00 10 20 1E C0 03 5F D6")}},
            {{"libunity.so", 0x50EBA5C, hexStringToBytes("00 10 20 1E C0 03 5F D6")}}}},
        {"paldo", {"paldo", false,
            {{"libunity.so", 0x5947C18, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0xC345020, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}},
            {}}},
        {"noshakegun", {"noshakegun", false,
            {{"libunity.so", 0x664B8D0, hexStringToBytes("00 00 80 D2 C0 03 5F D6")}},
            {}}},
        {"br", {"br", false,
            {{"libunity.so", 0xA7C93E4, hexStringToBytes("20 00 80 52 C0 03 5F D6")}},
            {}}},
        {"pump", {"pump", false,
            {{"libunity.so", 0x907D498, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}},
            {{"libunity.so", 0x907D498, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}}}},
        {"nop", {"nop", false,
            {{"libunity.so", 0x5DC662C, hexStringToBytes("00 10 20 1E C0 03 5F D6")}},
            {{"libunity.so", 0x5DC662C, hexStringToBytes("00 10 20 1E C0 03 5F D6")}}}},
        {"walk", {"walk", false,
            {{"libunity.so", 0x51D31BC, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x51F0810, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x54BC504, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}},
            {}}},
        {"crouch", {"crouch", false,
            {{"libunity.so", 0x5AC85B4, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x5A1EBE0, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x524D15C, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x5483A3C, hexStringToBytes("00 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x54B6820, hexStringToBytes("00 00 80 D2 C0 03 5F D6")}},
            {}}},
        {"un", {"un", false,
            {{"libunity.so", 0x901F988, hexStringToBytes("20 00 80 D2 C0 03 5F D6")},
             {"libunity.so", 0x9012214, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}},
            {}}},
    };

    seekbars_ = {
        {"aimbot_seekbar", {"aimbot_seekbar", 0,
            0x5161770, 0x5161774, 0x5161778, 0x666FB88, 0x666FB8C, 0x666FB90}},
        {"snowboard_seekbar", {"snowboard_seekbar", 0,
            0x522860C, 0x5228610, 0x5228614, 0x52286DC, 0x52286E0, 0x52286E4}},
        {"diveb_seekbar", {"diveb_seekbar", 0,
            0xC81FD4C, 0xC81FD50, 0xC81FD54, 0x5DE9880, 0x5DE9884, 0x5DE9888}},
        {"br_seekbar", {"br_seekbar", 0,
            0x6643848, 0x664384C, 0x6643850, 0, 0, 0}},
        {"mp_seekbar", {"mp_seekbar", 0,
            0x6D76CB4, 0x6D76CB8, 0x6D76CBC, 0x6AA24B4, 0x6AA24B8, 0x6AA24BC,
            0x599FB2C, 0x599FB2C + 4, 0x599FB2C + 8}},
    };

    radiobuttons_ = {
        {"character", {"shepherd", "character", false, "charss", "2020"}},
        {"character", {"sophia", "character", false, "charss", "1010"}},
        {"character", {"spectre", "character", false, "charss", "3030"}},
        {"character", {"templar", "character", false, "charss", "4040"}},
        {"character", {"siren", "character", false, "charss", "5050"}},
        {"character", {"ghost", "character", false, "charss", "6060"}},
        {"character", {"lazarus", "character", false, "charss", "8080"}},
        {"character", {"noir", "character", false, "charss", "90088"}},
        {"character", {"starlight", "character", false, "charss", "90087"}},
        {"character", {"homelander", "character", false, "charss", "90086"}},
        {"legend", {"chunli", "legend", false, "charss2", "191"}},
        {"legend", {"ryu", "legend", false, "charss2", "9090"}},
        {"legend", {"cammy", "legend", false, "charss2", "192"}},
        {"legend", {"akuma", "legend", false, "charss2", "190"}},
        {"epic", {"vivian", "epic", false, "haha", "02"}},
        {"epic", {"pader", "epic", false, "haha", "01"}},
        {"camo", {"offcamo", "camo", false, "fretzHAHA", "188"}},
        {"camo", {"diamond", "camo", false, "fretzHAHA", "1800"}},
        {"camo", {"redsprite", "camo", false, "fretzHAHA", "1801"}},
        {"camo", {"emerald", "camo", false, "fretzHAHA", "1815"}},
        {"camo", {"assault", "camo", false, "fretzHAHA", "1816"}},
        {"camo", {"scorch", "camo", false, "fretzHAHA", "1817"}},
        {"gun", {"ak117", "gun", false, "thumbnail", "078"}},
        {"gun", {"bp50", "gun", false, "thumbnail", "081"}},
        {"gun", {"ffar", "gun", false, "thumbnail", "079"}},
        {"gun", {"grau", "gun", false, "thumbnail", "080"}},
        {"gun", {"krig6", "gun", false, "thumbnail", "085"}},
        {"gun", {"type19", "gun", false, "thumbnail", "089"}},
        {"gun", {"dlq", "gun", false, "thumbnail", "082"}},
        {"gun", {"jak", "gun", false, "thumbnail", "094"}},
        {"gun", {"lucos", "gun", false, "thumbnail", "097"}},
        {"melee", {"tang", "melee", false, "fuckmellee", "1000"}},
        {"melee", {"longq", "melee", false, "fuckmellee", "999"}},
        {"melee", {"spear", "melee", false, "fuckmellee", "998"}},
        {"melee", {"scissors", "melee", false, "fuckmellee", "997"}},
        {"melee", {"tomahawk", "melee", false, "fuckmellee", "996"}},
        {"melee", {"saber", "melee", false, "fuckmellee", "995"}},
        {"melee", {"fiery", "melee", false, "fuckmellee", "994"}},
        {"melee", {"dark", "melee", false, "fuckmellee", "999"}},
        {"guns2", {"fennec", "guns2", false, "sken", "210"}},
        {"guns2", {"mg40", "guns2", false, "sken", "218"}},
        {"guns2", {"qq9", "guns2", false, "sken", "220"}},
        {"guns2", {"m13", "guns2", false, "sken", "222"}},
        {"guns2", {"x9", "guns2", false, "sken", "216"}},
        {"equip", {"sand", "equip", false, "gayontopp", "28193"}},
        {"equip", {"jetpack", "equip", false, "gayontopp", "18329"}},
        {"equip", {"farflight", "equip", false, "gayontopp", "28371"}},
        {"equip", {"mechair", "equip", false, "gayontopp", "19482"}},
        {"legendary_skin", {"warden", "legendary_skin", false, "warden", "30"}},
        {"epic_skin", {"yorsha", "epic_skin", false, "Cin", "2"}},
        {"epic_skin", {"Rambo", "epic_skin", false, "Rambo", "30"}},
        {"epic_skin", {"Ferg", "epic_skin", false, "ferg", "50"}},
        {"epic_skin", {"Roze", "epic_skin", false, "Roze", "40"}},
        {"epic_skin", {"Kestrel", "epic_skin", false, "Kestrelsnow", "60"}},
        {"mythic_skin", {"kuji", "mythic_skin", false, "CharssHAHA", "90089"}},
    };
}

bool TaizouCore::hasRootAccess() {
    FILE* fp = popen("su -c 'id'", "r");
    if (fp) {
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            result += buffer;
        }
        pclose(fp);
        return result.find("uid=0") != std::string::npos;
    }
    return false;
}

bool TaizouCore::checkOverlayPermission() {
    return true;
}

void TaizouCore::requestOverlayPermission(JNIEnv* env, jobject activity) {
    jclass settings_class = env->FindClass("android/provider/Settings");
    jmethodID can_draw_overlays = env->GetStaticMethodID(settings_class, "canDrawOverlays", "(Landroid/content/Context;)Z");
    jboolean can_draw = env->CallStaticBooleanMethod(settings_class, can_draw_overlays, activity);
    if (!can_draw) {
        jclass intent_class = env->FindClass("android/content/Intent");
        jmethodID intent_ctor = env->GetMethodID(intent_class, "<init>", "(Ljava/lang/String;)V");
        jstring action = env->NewStringUTF("android.settings.action.MANAGE_OVERLAY_PERMISSION");
        jobject intent = env->NewObject(intent_class, intent_ctor, action);
        jclass uri_class = env->FindClass("android/net/Uri");
        jmethodID parse_method = env->GetStaticMethodID(uri_class, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
        jstring package_str = env->NewStringUTF("package:com.taizou.paid");
        jobject uri = env->CallStaticObjectMethod(uri_class, parse_method, package_str);
        jmethodID set_data = env->GetMethodID(intent_class, "setData", "(Landroid/net/Uri;)Landroid/content/Intent;");
        env->CallObjectMethod(intent, set_data, uri);
        jclass activity_class = env->GetObjectClass(activity);
        jmethodID start_activity = env->GetMethodID(activity_class, "startActivity", "(Landroid/content/Intent;)V");
        env->CallVoidMethod(activity, start_activity, intent);
    }
}

int TaizouCore::findProcessId(const std::string& package_name) {
    std::string cmd = "pidof " + package_name;
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), fp)) {
            int pid = atoi(buffer);
            pclose(fp);
            return pid;
        }
        pclose(fp);
    }
    return -1;
}

uintptr_t TaizouCore::getLibraryBaseAddress(int pid, const std::string& lib_name, bool executableOnly) {
    if (pid <= 0) return 0;
    std::string maps_path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream maps(maps_path);
    std::string line;
    while (std::getline(maps, line)) {
        // Cheat patches use the FIRST mapping (library load base), exactly
        // like the original script. Only the bypass uses the executable
        // segment base (its offsets were calibrated to it).
        if (line.find(lib_name) == std::string::npos) continue;
        if (executableOnly && line.find("r-xp") == std::string::npos) continue;
        size_t dash_pos = line.find('-');
        if (dash_pos != std::string::npos) {
            std::string addr_str = line.substr(0, dash_pos);
            try {
                return std::stoull(addr_str, nullptr, 16);
            } catch (...) {
                // A malformed /proc maps line must not abort the injector
                // (uncaught C++ exceptions terminate the process).
                continue;
            }
        }
    }
    return 0;
}

std::vector<uint8_t> TaizouCore::hexStringToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    std::string clean_hex = hex;
    clean_hex.erase(std::remove(clean_hex.begin(), clean_hex.end(), ' '), clean_hex.end());
    clean_hex.erase(std::remove(clean_hex.begin(), clean_hex.end(), 'h'), clean_hex.end());

    for (size_t i = 0; i < clean_hex.length(); i += 2) {
        if (i + 1 < clean_hex.length()) {
            try {
                std::string byte_str = clean_hex.substr(i, 2);
                bytes.push_back(static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16)));
            } catch (...) {
                return {};
            }
        }
    }
    return bytes;
}

std::vector<uint8_t> TaizouCore::floatToHexLE(float value) {
    union {
        float f;
        uint32_t i;
    } converter;
    converter.f = value;
    uint32_t int_val = converter.i;
    return {
        static_cast<uint8_t>(int_val & 0xFF),
        static_cast<uint8_t>((int_val >> 8) & 0xFF),
        static_cast<uint8_t>((int_val >> 16) & 0xFF),
        static_cast<uint8_t>((int_val >> 24) & 0xFF)
    };
}

bool TaizouCore::applyMemoryPatch(int pid, const MemoryPatch& patch, bool executableOnly) {
    if (pid <= 0) return false;
    uintptr_t base = getLibraryBaseAddress(pid, patch.lib_name, executableOnly);
    if (base == 0) {
        LOGE("patch: library %s not found", patch.lib_name.c_str());
        return false;
    }

    uintptr_t target_addr = base + patch.offset;
    std::string mem_path = "/proc/" + std::to_string(pid) + "/mem";

    int fd = open(mem_path.c_str(), O_WRONLY);
    if (fd < 0) {
        LOGE("patch: cannot open mem (root required)");
        return false;
    }

    if (lseek(fd, target_addr, SEEK_SET) == -1) {
        LOGE("patch: seek failed");
        close(fd);
        return false;
    }

    ssize_t written = write(fd, patch.bytes.data(), patch.bytes.size());
    close(fd);
    if (written != static_cast<ssize_t>(patch.bytes.size())) {
        LOGE("patch: short write");
        return false;
    }
    return true;
}

bool TaizouCore::applyMemoryPatch(const std::string& lib_name, uintptr_t offset, const std::vector<uint8_t>& bytes) {
    int pid = findProcessId("com.garena.game.codm");
    if (pid <= 0) return false;

    MemoryPatch patch;
    patch.lib_name = lib_name;
    patch.offset = offset;
    patch.bytes = bytes;
    return applyMemoryPatch(pid, patch);
}

void TaizouCore::setCheckBoxState(const std::string& name, bool checked) {
    auto it = checkboxes_.find(name);
    if (it == checkboxes_.end()) return;

    it->second.checked = checked;
    const auto& patches = checked ? it->second.patches_on : it->second.patches_off;

    int pid = findProcessId("com.garena.game.codm");
    for (const auto& patch : patches) {
        applyMemoryPatch(pid, patch);
    }
}

void TaizouCore::setSeekBarProgress(const std::string& name, int progress) {
    auto it = seekbars_.find(name);
    if (it == seekbars_.end()) return;

    it->second.progress = progress;
    float value = progress * 1.0f;
    auto hex_bytes = floatToHexLE(value);

    int pid = findProcessId("com.garena.game.codm");
    if (pid <= 0) return;

    const auto& sb = it->second;
    if (sb.offset1) applyMemoryPatch("libunity.so", sb.offset1, hexStringToBytes("40 00 00 1C C0 03 5F D6"));
    if (sb.offset2) applyMemoryPatch("libunity.so", sb.offset2, hexStringToBytes("C0 03 5F D6 00 00 7A 44"));
    if (sb.offset3) applyMemoryPatch("libunity.so", sb.offset3, hex_bytes);
    if (sb.offset4) applyMemoryPatch("libunity.so", sb.offset4, hexStringToBytes("40 00 00 1C C0 03 5F D6"));
    if (sb.offset5) applyMemoryPatch("libunity.so", sb.offset5, hexStringToBytes("C0 03 5F D6 00 00 7A 44"));
    if (sb.offset6) applyMemoryPatch("libunity.so", sb.offset6, hex_bytes);
    if (sb.offset7) applyMemoryPatch("libunity.so", sb.offset7, hexStringToBytes("40 00 00 1C C0 03 5F D6"));
    if (sb.offset8) applyMemoryPatch("libunity.so", sb.offset8, hexStringToBytes("C0 03 5F D6 00 00 7A 44"));
    if (sb.offset9) applyMemoryPatch("libunity.so", sb.offset9, hex_bytes);
}

void TaizouCore::setRadioButtonState(const std::string& group, const std::string& name) {
    for (auto& [key, rb] : radiobuttons_) {
        if (rb.group == group) {
            rb.checked = (rb.name == name);
            if (rb.checked) {
                // Original cppPatch appends " 2 3 4" to the skin code.
                executeNativeBinaryRoot(rb.lib_name, rb.code + " 2 3 4");
            }
        }
    }
}

// Ports the original AndLua auto-bypass: 18 libanogs.so patches written once
// the game process and library are present. Same offsets/bytes, same order.
bool TaizouCore::applyAutoBypass() {
    static const uintptr_t kAnogsOffsets[] = {
        0x25164C, 0x204218, 0x24D534, 0x258DA0,
        0x261DC0, 0x26E6E8, 0x331ED8, 0x374374,
        0x419B6C, 0x41BA40, 0x42844C, 0x42DF74,
        0x444D68, 0x44A3F0, 0x44BC90, 0x494F48,
        0x497E64, 0x4A9944
    };
    static const char* kBypassBytes = "h00 00 80 D2 C0 03 5F D6";

    int pid = findProcessId("com.garena.game.codm");
    if (pid <= 0) return false;

    bool ok = true;
    for (uintptr_t offset : kAnogsOffsets) {
        MemoryPatch patch;
        patch.lib_name = "libanogs.so";
        patch.offset = offset;
        patch.bytes = hexStringToBytes(kBypassBytes);
        // Bypass offsets are calibrated to the executable segment base.
        if (!applyMemoryPatch(pid, patch, true)) ok = false;
    }
    return ok;
}

std::string TaizouCore::saveConfig() {
    std::ostringstream oss;
    oss << "{";
    bool first = true;

    for (const auto& [name, cb] : checkboxes_) {
        if (!first) oss << ",";
        oss << "\"" << name << "\":" << (cb.checked ? "true" : "false");
        first = false;
    }

    for (const auto& [name, sb] : seekbars_) {
        if (!first) oss << ",";
        oss << "\"" << name << "\":" << sb.progress;
        first = false;
    }

    oss << "}";
    std::string json = oss.str();

    std::ofstream file(config_path_);
    if (file.is_open()) {
        file << json;
        file.close();
    }
    return json;
}

bool TaizouCore::loadConfig(const std::string& json) {
    try {
        size_t pos = 0;
        while ((pos = json.find(':', pos)) != std::string::npos) {
            size_t name_start = json.rfind('"', pos - 1);
            size_t name_end = json.find('"', name_start + 1);
            if (name_start == std::string::npos || name_end == std::string::npos) break;
            std::string name = json.substr(name_start + 1, name_end - name_start - 1);

            size_t val_start = json.find_first_not_of(" \t", pos + 1);
            if (val_start == std::string::npos) break;

            if (json[val_start] == 't') {
                setCheckBoxState(name, true);
            } else if (json[val_start] == 'f') {
                setCheckBoxState(name, false);
            } else if (isdigit(json[val_start]) || json[val_start] == '-') {
                size_t val_end = json.find_first_of(",}", val_start);
                std::string val_str = json.substr(val_start, val_end - val_start);
                int progress = std::stoi(val_str);
                setSeekBarProgress(name, progress);
            }
            pos = val_start + 1;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// Ports the original clogs file-delete list (best-effort like the original
// pcall version: entries needing unavailable access simply fail).
void TaizouCore::clearLogs() {
    static const char* kPaths[] = {
        "/data/data/com.garena.game.codm/app_bugly",
        "/data/data/com.garena.game.codm/app_crashrecord",
        "/data/data/com.garena.game.codm/app_textures",
        "/data/data/com.garena.game.codm/app_webview",
        "/data/data/com.garena.game.codm/cache",
        "/data/data/com.garena.game.codm/code_cache",
        "/data/data/com.garena.game.codm/daabases",
        "/data/data/com.garena.game.codm/databases",
        "/data/data/com.garena.game.codm/files/AFRequestCache",
        "/data/data/com.garena.game.codm/files/com.gcloudsdk.gcloud.gvoice",
        "/data/data/com.garena.game.codm/files/facebook_ml",
        "/data/data/com.garena.game.codm/files/itop_login.txt",
        "/data/data/com.garena.game.codm/files/tpnlcache.data",
        "/data/data/com.garena.game.codm/no_backup",
        "/data/data/com.garena.game.codm/oat",
        "/data/data/com.garena.game.codm/files/tss_tmp",
        "/storage/emulated/0/Android/data/com.garena.game.codm/cache",
        "/storage/emulated/0/Android/data/com.garena.game.codm/files/ChatCache",
        "/storage/emulated/0/Android/data/com.garena.game.codm/files/TGPA",
        "/storage/emulated/0/Android/data/com.garena.game.codm/files/VoiceCache",
        "/storage/emulated/0/MidasOversea",
        "/storage/emulated/0/tencent",
    };
    for (const char* p : kPaths) {
        remove(p);
    }
}

// ================= External ESP (reads only, never writes) =================

namespace {
// Reference field offsets (validated per entity at runtime; garbage fails closed).
constexpr uintptr_t kEspPlayerInfo = 0x5C0;
constexpr uintptr_t kEspAlive = 0x548;
constexpr uintptr_t kEspBot = 0x5B9;
constexpr uintptr_t kEspHeadBone = 0x308;
constexpr uintptr_t kEspMesh = 0x628;
constexpr uintptr_t kEspAttackInfo = 0x78;
constexpr uintptr_t kEspCurHP = 0x34;
constexpr uintptr_t kEspMaxHP = 0x38;
constexpr uintptr_t kEspBoneMap = 0xA8;
constexpr uintptr_t kEspBoneTable = 0x10;
constexpr uintptr_t kEspBoneNode = 0x40;
// Index: 0 Head,1 Neck,2 Body,3 Hips,4 LUpperArm,5 LArm,6 LHand,7 RUpperArm,
// 8 RArm,9 RHand,10 LPaha,11 LBetis,12 LToe,13 RPaha,14 RBetis,15 RToe.
constexpr uintptr_t kEspBoneSlots[16] = {
    0x70, 0x90, 0x98, 0x50, 0x68, 0x60, 0x58, 0x88,
    0x80, 0x78, 0x30, 0x28, 0x20, 0x48, 0x40, 0x38
};
// Candidate localToWorld-matrix offsets inside a Unity Transform. Tried in
// order; a candidate only wins if it yields anatomically sane projections.
constexpr uintptr_t kEspMatrixCandidates[] = {
    0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x100, 0x110, 0x120
};
constexpr int kEspMaxEntities = 128;
constexpr uint64_t kEspScanCap = 512ull * 1024 * 1024;  // scan-loop byte budget
}

bool TaizouCore::espRead(int fd, uintptr_t addr, void* out, size_t len) const {
    if (fd < 0 || addr == 0 || out == nullptr || len == 0) return false;
    return pread(fd, out, len, (off_t)addr) == (ssize_t)len;
}

uint64_t TaizouCore::espU64(int fd, uintptr_t addr) const {
    uint64_t v = 0;
    espRead(fd, addr, &v, sizeof(v));
    return v;
}

int32_t TaizouCore::espI32(int fd, uintptr_t addr) const {
    int32_t v = 0;
    espRead(fd, addr, &v, sizeof(v));
    return v;
}

float TaizouCore::espF32(int fd, uintptr_t addr) const {
    float v = 0;
    espRead(fd, addr, &v, sizeof(v));
    return v;
}

int TaizouCore::espScorePawn(int fd, uint64_t pawn) const {
    if (pawn == 0 || (pawn & 0x7) != 0) return 0;
    int score = 0;
    uint64_t info = espU64(fd, pawn + kEspPlayerInfo);
    if (info != 0) score++;
    // HP lives in AttackableTargetInfo (pawn+0x78), NOT in PlayerInfo:
    // scoring PlayerInfo bytes as HP can never pass on real pawns.
    uint64_t atk = espU64(fd, pawn + kEspAttackInfo);
    if (atk != 0) {
        score++;
        float hp = 0, max = 0;
        if (espRead(fd, atk + kEspCurHP, &hp, 4) && espRead(fd, atk + kEspMaxHP, &max, 4) &&
            max > 0 && max <= 100000.0f && hp >= 0 && hp <= max) {
            score += 2;
        }
    }
    if (espU64(fd, pawn + kEspHeadBone) != 0) score++;
    if (espU64(fd, pawn + kEspMesh) != 0) score++;
    return score;  // max 6
}

bool TaizouCore::espReadName(int fd, uint64_t pawn, bool isBot, char out[48]) const {
    if (isBot) {
        strncpy(out, "BOT", 48);
        out[47] = '\0';
        return true;
    }
    uint64_t info = espU64(fd, pawn + kEspPlayerInfo);
    if (info == 0) return false;
    uint64_t str = espU64(fd, info + 0x158);
    if (str == 0) return false;
    int32_t len = 0;
    if (!espRead(fd, str + 0x10, &len, 4)) return false;
    if (len <= 0 || len > 40) {
        // Empty/unset name on a readable string: accept as unknown.
        strncpy(out, "?", 48);
        out[47] = '\0';
        return true;
    }
    uint16_t buf[40];
    if (!espRead(fd, str + 0x14, buf, (size_t)len * 2)) return false;
    int n = len < 47 ? len : 47;
    for (int i = 0; i < n; i++) {
        uint16_t c = buf[i];
        out[i] = (c >= 0x20 && c < 0x80) ? (char)c : '?';
    }
    out[n] = '\0';
    return true;
}

bool TaizouCore::espBoneWorld(int fd, uint64_t transformPtr, uintptr_t matrixOff, EspVec3& out) const {
    if (transformPtr == 0 || (transformPtr & 0x7) != 0) return false;
    float m[16];
    if (!espRead(fd, transformPtr + matrixOff, m, sizeof(m))) return false;
    for (int i = 0; i < 16; i++) {
        if (!(m[i] == m[i])) return false;
    }
    if (m[15] < 0.9f || m[15] > 1.1f) return false;  // affine localToWorld
    float tx = m[12], ty = m[13], tz = m[14];
    if (tx > 30000 || tx < -30000 || ty > 30000 || ty < -30000 || tz > 30000 || tz < -30000) return false;
    float r0 = m[0] * m[0] + m[1] * m[1] + m[2] * m[2];
    float r1 = m[4] * m[4] + m[5] * m[5] + m[6] * m[6];
    float r2 = m[8] * m[8] + m[9] * m[9] + m[10] * m[10];
    if (r0 < 0.05f || r0 > 20.0f || r1 < 0.05f || r1 > 20.0f || r2 < 0.05f || r2 > 20.0f) return false;
    out.x = tx;
    out.y = ty;
    out.z = tz;
    return true;
}

bool TaizouCore::espProject(const float m[16], const EspVec3& w, int viewW, int viewH, float& sx, float& sy) const {
    float cx = m[0] * w.x + m[4] * w.y + m[8] * w.z + m[12];
    float cy = m[1] * w.x + m[5] * w.y + m[9] * w.z + m[13];
    float cw = m[3] * w.x + m[7] * w.y + m[11] * w.z + m[15];
    if (!(cw > 0.01f)) return false;
    float nx = cx / cw, ny = cy / cw;
    if (nx < -1.5f || nx > 1.5f || ny < -1.5f || ny > 1.5f) return false;
    sx = (nx * 0.5f + 0.5f) * viewW;
    sy = (1.0f - (ny * 0.5f + 0.5f)) * viewH;  // fold Unity bottom-left flip here
    return true;
}

bool TaizouCore::espMapsRegions(int pid, uint64_t& rxStart, uint64_t& rxEnd,
                                std::vector<std::pair<uint64_t, uint64_t>>& rwRegions) const {
    rxStart = rxEnd = 0;
    rwRegions.clear();
    std::string path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream maps(path);
    if (!maps.is_open()) return false;
    std::string line;
    while (std::getline(maps, line)) {
        if (line.find("libunity.so") == std::string::npos) continue;
        size_t dash = line.find('-');
        size_t sp = line.find(' ', dash == std::string::npos ? 0 : dash);
        if (dash == std::string::npos || sp == std::string::npos) continue;
        uint64_t start = 0, end = 0;
        try {
            start = std::stoull(line.substr(0, dash), nullptr, 16);
            end = std::stoull(line.substr(dash + 1, sp - dash - 1), nullptr, 16);
        } catch (...) { continue; }
        if (end <= start) continue;
        if (line.find("r-xp") != std::string::npos) {
            if (rxStart == 0) { rxStart = start; rxEnd = end; }
            else if (end > rxEnd) rxEnd = end;
        } else if (line.find("rw-p") != std::string::npos) {
            if (end - start <= 256ull * 1024 * 1024) rwRegions.emplace_back(start, end);
        }
    }
    return rxStart != 0;
}

static bool espHeapRegions(int pid, std::vector<std::pair<uint64_t, uint64_t>>& out) {
    std::vector<std::pair<uint64_t, uint64_t>> hot, cold;
    std::string path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream maps(path);
    if (!maps.is_open()) return false;
    std::string line;
    while (std::getline(maps, line)) {
        if (line.find("rw-p") == std::string::npos && line.find("rw-s") == std::string::npos) continue;
        size_t dash = line.find('-');
        size_t sp = line.find(' ', dash == std::string::npos ? 0 : dash);
        if (dash == std::string::npos || sp == std::string::npos) continue;
        // Heap-capable mappings: pure anonymous, [heap], dalvik/art heaps
        // (including ashmem-backed), LinearAlloc, memfd. Never file-backed
        // libraries (which also contain '/' but no heap marker).
        size_t pathPos = line.find('/', sp);
        bool markedHeap = line.find("[heap]") != std::string::npos ||
                          line.find("[anon:") != std::string::npos ||
                          line.find("/dev/ashmem/") != std::string::npos ||
                          line.find("/memfd:") != std::string::npos;
        bool heapish = markedHeap || pathPos == std::string::npos;
        if (!heapish) continue;
        uint64_t start = 0, end = 0;
        try {
            start = std::stoull(line.substr(0, dash), nullptr, 16);
            end = std::stoull(line.substr(dash + 1, sp - dash - 1), nullptr, 16);
        } catch (...) { continue; }
        if (end <= start || end - start < 4096) continue;
        // Managed heaps before malloc/scudo/stack arenas.
        bool hotRegion = line.find("dalvik") != std::string::npos ||
                         line.find("[heap]") != std::string::npos ||
                         line.find("art") != std::string::npos ||
                         line.find("LinearAlloc") != std::string::npos;
        if (hotRegion) hot.emplace_back(start, end);
        else cold.emplace_back(start, end);
    }
    out.clear();
    out.insert(out.end(), hot.begin(), hot.end());
    out.insert(out.end(), cold.begin(), cold.end());
    return !out.empty();
}

static bool espMatrixSane(const float m[16]) {
    for (int i = 0; i < 16; i++) {
        if (!(m[i] == m[i]) || m[i] > 1e6f || m[i] < -1e6f) return false;
    }
    float row = m[0] * m[0] + m[1] * m[1] + m[2] * m[2] + m[4] * m[4] + m[5] * m[5] + m[6] * m[6];
    if (row < 0.01f || row > 400.0f) return false;
    // Perspective projection must have a w-term; m[15] intentionally NOT
    // filtered (convention-dependent: ~0 GL-style, ~1 with borrowed matrices).
    if (m[3] == 0 && m[7] == 0 && m[11] == 0) return false;
    return true;
}

bool TaizouCore::espFindList(int fd, uint64_t rxStart, uint64_t rxEnd, uint64_t& listAddr) const {
    // Cheap pre-filter in bulk chunks; full pawn validation only on hits.
    std::vector<std::pair<uint64_t, uint64_t>> regions;
    // Caller passes heap regions via reuse of rwRegions trick: scan heap here.
    (void)rxStart;
    (void)rxEnd;
    if (!espHeapRegions(espPid_, regions)) {
        LOGE("esp: no heap regions");
        return false;
    }
    const size_t kChunk = 1024 * 1024;
    std::vector<uint8_t> buf(kChunk + 64);
    int checked = 0;
    int maxPawn = 0;
    bool done = false;
    size_t scanned = 0;
    espDiagRegions_ = (int)regions.size();
    espDiagMB_ = 0;
    for (auto [rs, re] : regions) espDiagMB_ += (re - rs) / (1024 * 1024);
    for (auto [rs, re] : regions) {
        if (done || scanned >= kEspScanCap) break;
        for (uint64_t base = rs; base < re && !done; base += kChunk) {
            size_t len = (size_t)std::min<uint64_t>(kChunk + 64, re - base);
            if (len < 64 || !espRead(fd, base, buf.data(), len)) continue;
            scanned += len;
            size_t n = (len - 64) / 8;
            for (size_t i = 0; i < n; i++) {
                int32_t sz = 0;
                memcpy(&sz, buf.data() + i * 8 + 0x18, 4);
                if (sz < 1 || sz > 48) continue;
                uint64_t items = 0;
                memcpy(&items, buf.data() + i * 8 + 0x10, 8);
                if (items == 0 || (items & 7) != 0) continue;
                int32_t maxLen = espI32(fd, items + 0x18);
                if (maxLen < sz || maxLen > 1024) continue;
                int check = sz < 6 ? sz : 6, score = 0;
                for (int k = 0; k < check; k++) {
                    uint64_t pawn = espU64(fd, items + 0x20 + (uint64_t)k * 8);
                    int ps = espScorePawn(fd, pawn);
                    if (ps > maxPawn) maxPawn = ps;
                    if (ps >= 4) score++;
                }
                // Strict bar: garbage must never win. Single real enemies
                // still pass (one perfect pawn = 100%).
                bool accept = check > 0 && score * 100 / check >= 75 &&
                              (score >= 4 || (check == 1 && score == 1));
                if (accept) {
                    listAddr = base + i * 8;
                    espDiagChecked_ = checked;
                    espDiagBest_ = maxPawn;
                    LOGD("esp: enemy list @%llx score=%d/%d", (unsigned long long)listAddr, score, check);
                    return true;
                }
                if (++checked >= 4000) { done = true; break; }
            }
        }
    }
    espDiagChecked_ = checked;
    espDiagBest_ = maxPawn;
    LOGE("esp: no enemy list found");
    return false;
}

bool TaizouCore::espFindMatrix(int fd, const std::vector<EspEntity>& ents, int viewW, int viewH,
                               float outM[16], uintptr_t& outMatrixOff, uint64_t& outMatrixAddr) const {
    // Candidate VP matrices from libunity RW + heap (bulk scan, sanity filter).
    std::vector<std::pair<uint64_t, uint64_t>> regions;
    uint64_t rxS = 0, rxE = 0;
    if (!espMapsRegions(espPid_, rxS, rxE, regions)) return false;
    std::vector<std::pair<uint64_t, uint64_t>> heap;
    espHeapRegions(espPid_, heap);
    regions.insert(regions.end(), heap.begin(), heap.end());
    const size_t kChunk = 512 * 1024;
    std::vector<uint8_t> buf(kChunk + 64);
    struct Cand { float m[16]; uint64_t addr = 0; };
    std::vector<Cand> cands;
    size_t scanned = 0;
    for (auto [rs, re] : regions) {
        if (cands.size() >= 400 || scanned >= 192ull * 1024 * 1024) break;
        for (uint64_t base = rs; base < re && cands.size() < 400 && scanned < 192ull * 1024 * 1024; base += kChunk) {
            size_t len = (size_t)std::min<uint64_t>(kChunk + 64, re - base);
            if (len < 64 || !espRead(fd, base, buf.data(), len)) { scanned += len; continue; }
            for (size_t o = 0; o + 64 <= len && cands.size() < 400; o += 4) {
                float m[16];
                memcpy(m, buf.data() + o, 64);
                if (!espMatrixSane(m)) continue;
                Cand c;
                memcpy(c.m, m, 64);
                c.addr = base + o;
                cands.push_back(c);
            }
            scanned += len;
        }
    }
    LOGD("esp: %d matrix candidates", (int)cands.size());
    if (cands.empty() || ents.empty()) return false;
    // Joint (matrix, transform-offset) search scored by anatomical projection.
    int bestScore = 0;
    for (auto& c : cands) {
        for (uintptr_t toff : kEspMatrixCandidates) {
            int score = 0;
            for (auto& e : ents) {
                EspVec3 rw, hw;
                if (!espBoneWorld(fd, e.boneMesh, toff, rw)) continue;
                if (!espBoneWorld(fd, e.boneHead, toff, hw)) continue;
                float dx = hw.x - rw.x, dy = hw.y - rw.y, dz = hw.z - rw.z;
                float h = sqrtf(dx * dx + dy * dy + dz * dz);
                if (h < 0.3f || h > 3.0f) continue;
                float hx, hy, rx, ry;
                if (!espProject(c.m, hw, viewW, viewH, hx, hy)) continue;
                if (!espProject(c.m, rw, viewW, viewH, rx, ry)) continue;
                float pxH = fabsf(hy - ry);
                if (pxH < 10 || pxH > viewH * 1.5f || hy > ry) continue;
                score += 2;
            }
            if (score > bestScore) {
                bestScore = score;
                memcpy(outM, c.m, 64);
                outMatrixOff = toff;
                outMatrixAddr = c.addr;
            }
        }
    }
    LOGD("esp: matrix search best=%d over %d ents", bestScore, (int)ents.size());
    int n = (int)ents.size();
    int need = n >= 4 ? (n * 7 + 9) / 10 : n;  // >=70% (all of them when few)
    return bestScore >= need * 2;
}

std::string TaizouCore::resolveBinaryPath(const std::string& binary_name) {
    if (binary_name.find('/') != std::string::npos) return binary_name;
    if (!files_dir_.empty()) return files_dir_ + "/Res/" + binary_name;
    return binary_name;
}

void TaizouCore::executeNativeBinary(const std::string& binary_name, const std::string& args) {
    std::string path = resolveBinaryPath(binary_name);
    std::string cmd = "chmod 777 " + path + " && " + path + " " + args;
    system(cmd.c_str());
}

void TaizouCore::executeNativeBinaryRoot(const std::string& binary_name, const std::string& args) {
    std::string path = resolveBinaryPath(binary_name);
    std::string cmd = "su -c 'chmod 777 " + path + " && " + path + " " + args + "'";
    system(cmd.c_str());
}

void TaizouCore::speakText(const std::string& text) {
    // TTS handled in Kotlin layer
}

void TaizouCore::showToast(const std::string& message) {
    // Toast handled in Kotlin layer
}

} // namespace taizou

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_initialize(JNIEnv* env, jobject thiz, jobject context) {
    return taizou::g_instance ? taizou::g_instance->initialize(env, context) : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_shutdown(JNIEnv* env, jobject thiz) {
    if (taizou::g_instance) {
        taizou::g_instance->shutdown();
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_hasRootAccess(JNIEnv* env, jobject thiz) {
    return taizou::g_instance ? taizou::g_instance->hasRootAccess() : JNI_FALSE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_taizou_paid_TaizouNative_findProcessId(JNIEnv* env, jobject thiz, jstring packageName) {
    if (!taizou::g_instance) return -1;
    const char* pkg = env->GetStringUTFChars(packageName, nullptr);
    int result = taizou::g_instance->findProcessId(pkg);
    env->ReleaseStringUTFChars(packageName, pkg);
    return result;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_applyMemoryPatch(JNIEnv* env, jobject thiz, jstring libName, jlong offset, jbyteArray bytes) {
    if (!taizou::g_instance) return JNI_FALSE;
    const char* lib = env->GetStringUTFChars(libName, nullptr);
    jbyte* byte_array = env->GetByteArrayElements(bytes, nullptr);
    jsize len = env->GetArrayLength(bytes);
    std::vector<uint8_t> byte_vec(byte_array, byte_array + len);
    bool result = taizou::g_instance->applyMemoryPatch(lib, offset, byte_vec);
    env->ReleaseByteArrayElements(bytes, byte_array, JNI_ABORT);
    env->ReleaseStringUTFChars(libName, lib);
    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_applyMemoryPatchOffset(JNIEnv* env, jobject thiz, jstring libName, jlong offset, jstring hexBytes) {
    if (!taizou::g_instance) return JNI_FALSE;
    const char* lib = env->GetStringUTFChars(libName, nullptr);
    const char* hex = env->GetStringUTFChars(hexBytes, nullptr);
    auto bytes = taizou::g_instance->hexStringToBytes(hex);
    bool result = taizou::g_instance->applyMemoryPatch(lib, offset, bytes);
    env->ReleaseStringUTFChars(hexBytes, hex);
    env->ReleaseStringUTFChars(libName, lib);
    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_setCheckBoxState(JNIEnv* env, jobject thiz, jstring name, jboolean checked) {
    if (!taizou::g_instance) return;
    const char* n = env->GetStringUTFChars(name, nullptr);
    taizou::g_instance->setCheckBoxState(n, checked);
    env->ReleaseStringUTFChars(name, n);
}

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_setSeekBarProgress(JNIEnv* env, jobject thiz, jstring name, jint progress) {
    if (!taizou::g_instance) return;
    const char* n = env->GetStringUTFChars(name, nullptr);
    taizou::g_instance->setSeekBarProgress(n, progress);
    env->ReleaseStringUTFChars(name, n);
}

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_setRadioButtonState(JNIEnv* env, jobject thiz, jstring group, jstring name) {
    if (!taizou::g_instance) return;
    const char* g = env->GetStringUTFChars(group, nullptr);
    const char* n = env->GetStringUTFChars(name, nullptr);
    taizou::g_instance->setRadioButtonState(g, n);
    env->ReleaseStringUTFChars(name, n);
    env->ReleaseStringUTFChars(group, g);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_taizou_paid_TaizouNative_saveConfig(JNIEnv* env, jobject thiz) {
    if (!taizou::g_instance) return env->NewStringUTF("{}");
    std::string json = taizou::g_instance->saveConfig();
    return env->NewStringUTF(json.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_loadConfig(JNIEnv* env, jobject thiz, jstring json) {
    if (!taizou::g_instance) return JNI_FALSE;
    const char* j = env->GetStringUTFChars(json, nullptr);
    bool result = taizou::g_instance->loadConfig(j);
    env->ReleaseStringUTFChars(json, j);
    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_executeNativeBinary(JNIEnv* env, jobject thiz, jstring binaryName, jstring args) {
    if (!taizou::g_instance) return;
    const char* bin = env->GetStringUTFChars(binaryName, nullptr);
    const char* a = env->GetStringUTFChars(args, nullptr);
    taizou::g_instance->executeNativeBinary(bin, a);
    env->ReleaseStringUTFChars(args, a);
    env->ReleaseStringUTFChars(binaryName, bin);
}

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_executeNativeBinaryRoot(JNIEnv* env, jobject thiz, jstring binaryName, jstring args) {
    if (!taizou::g_instance) return;
    const char* bin = env->GetStringUTFChars(binaryName, nullptr);
    const char* a = env->GetStringUTFChars(args, nullptr);
    taizou::g_instance->executeNativeBinaryRoot(bin, a);
    env->ReleaseStringUTFChars(args, a);
    env->ReleaseStringUTFChars(binaryName, bin);
}

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_speakText(JNIEnv* env, jobject thiz, jstring text) {
    // Handled in Kotlin
}

// Creates the core singleton when the library is loaded
// (System.loadLibrary). Without this, g_instance stays null: every guarded
// native call silently no-ops and the unguarded ones would dereference null.
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    static taizou::TaizouCore core;
    (void)vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_applyAutoBypass(JNIEnv* env, jobject thiz) {
    if (!taizou::g_instance) return JNI_FALSE;
    return taizou::g_instance->applyAutoBypass() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_isLibraryLoaded(JNIEnv* env, jobject thiz, jint pid, jstring libName) {
    if (!taizou::g_instance) return JNI_FALSE;
    const char* lib = env->GetStringUTFChars(libName, nullptr);
    bool result = taizou::g_instance->getLibraryBaseAddress((int)pid, lib) != 0;
    env->ReleaseStringUTFChars(libName, lib);
    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_clearLogs(JNIEnv* env, jobject thiz) {
    if (taizou::g_instance) taizou::g_instance->clearLogs();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_taizou_paid_TaizouNative_pollEsp(JNIEnv* env, jobject thiz, jint viewW, jint viewH) {
    if (!taizou::g_instance) return 0;
    return taizou::g_instance->pollEsp((int)viewW, (int)viewH);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_getEspEntry(JNIEnv* env, jobject thiz, jint index, jfloatArray out) {
    if (!taizou::g_instance || out == nullptr) return JNI_FALSE;
    if (env->GetArrayLength(out) < 12) return JNI_FALSE;
    float buf[12];
    if (!taizou::g_instance->espEntry((int)index, buf)) return JNI_FALSE;
    env->SetFloatArrayRegion(out, 0, 12, buf);
    return JNI_TRUE;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_taizou_paid_TaizouNative_getEspName(JNIEnv* env, jobject thiz, jint index) {
    std::string s = taizou::g_instance ? taizou::g_instance->espName((int)index) : std::string();
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_getEspBones(JNIEnv* env, jobject thiz, jint index, jfloatArray out) {
    if (!taizou::g_instance || out == nullptr) return JNI_FALSE;
    if (env->GetArrayLength(out) < 48) return JNI_FALSE;
    float buf[48];
    if (!taizou::g_instance->espBones((int)index, buf)) return JNI_FALSE;
    env->SetFloatArrayRegion(out, 0, 48, buf);
    return JNI_TRUE;
}

extern "C" JNIEXPORT jintArray JNICALL
Java_com_taizou_paid_TaizouNative_getEspTotals(JNIEnv* env, jobject thiz) {
    jintArray arr = env->NewIntArray(3);
    if (arr == nullptr) return nullptr;
    jint vals[3] = {0, 0, 0};
    if (taizou::g_instance) {
        vals[0] = taizou::g_instance->espTotalEnemies();
        vals[1] = taizou::g_instance->espTotalBots();
        vals[2] = taizou::g_instance->espHasList() ? 1 : 0;
    }
    env->SetIntArrayRegion(arr, 0, 3, vals);
    return arr;
}

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_setEspMatchGame(JNIEnv* env, jobject thiz, jlong addr) {
    if (taizou::g_instance) taizou::g_instance->setEspMatchGame((uint64_t)addr);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_setEspMatrix(JNIEnv* env, jobject thiz, jfloatArray values) {
    if (!taizou::g_instance || values == nullptr) return JNI_FALSE;
    if (env->GetArrayLength(values) < 16) return JNI_FALSE;
    float m[16];
    env->GetFloatArrayRegion(values, 0, 16, m);
    return taizou::g_instance->setEspMatrix(m) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_taizou_paid_TaizouNative_getEspDiag(JNIEnv* env, jobject thiz) {
    std::string s = taizou::g_instance ? taizou::g_instance->espDiag() : std::string("native not ready");
    return env->NewStringUTF(s.c_str());
}

std::vector<float> TaizouCore::detectBoxes(const int32_t* pixels, int w, int h) {
    std::vector<float> out;
    out.push_back(0.0f);  // placeholder for n
    if (pixels == nullptr || w < 80 || h < 60 || w > 2000 || h > 2000) return out;
    int n = w * h;
    // Red mask: vivid reds only (wallhack shade), hue-agnostic integer test.
    std::vector<uint8_t> mask((size_t)n, 0);
    long redCount = 0;
    for (int i = 0; i < n; i++) {
        int32_t v = pixels[i];
        int r = v & 0xFF, g = (v >> 8) & 0xFF, b = (v >> 16) & 0xFF;
        int mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
        int mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
        if (mx < 115) continue;                    // V > ~0.45
        if (r <= g || r <= b) continue;            // red dominant (kills yellow/green/blue/white)
        int delta = mx - mn;
        int ab = g > b ? g - b : b - g;
        if (ab * 5 > delta) continue;              // hue within ~12deg of pure red
        if ((mx - mn) * 100 < mx * 65) continue;   // saturation > ~0.65
        mask[(size_t)i] = 1;
        redCount++;
    }
    // Fullscreen-red guard (damage vignette, red zone, menus): suppress all.
    if (redCount > (long)n / 12) return out;
    // Connected components (4-neighbourhood, union-find over mask pixels).
    std::vector<int> parent((size_t)n, -1);
    std::function<int(int)> find = [&](int x) -> int {
        int r = x;
        while (parent[(size_t)r] != r) r = parent[(size_t)r];
        while (parent[(size_t)x] != r) {
            int nxt = parent[(size_t)x];
            parent[(size_t)x] = r;
            x = nxt;
        }
        return r;
    };
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int i = y * w + x;
            if (!mask[(size_t)i]) continue;
            parent[(size_t)i] = i;
            if (x > 0 && mask[(size_t)(i - 1)]) {
                int a = find(i), b = find(i - 1);
                if (a != b) parent[(size_t)a] = b;
            }
            if (y > 0 && mask[(size_t)(i - w)]) {
                int a = find(i), b = find(i - w);
                if (a != b) parent[(size_t)a] = b;
            }
        }
    }
    struct Box { int x1, y1, x2, y2, area; };
    std::vector<Box> boxes;
    boxes.reserve(64);
    // Map root -> box index.
    std::vector<int> roots;
    roots.reserve(64);
    for (int i = 0; i < n; i++) {
        if (!mask[(size_t)i]) continue;
        int r = find(i);
        int x = i % w, y = i / w;
        size_t k = 0;
        for (; k < roots.size(); k++) {
            if (roots[k] == r) break;
        }
        if (k == roots.size()) {
            if (roots.size() >= 64) continue;
            roots.push_back(r);
            boxes.push_back({x, y, x, y, 0});
        }
        Box& b = boxes[k];
        if (x < b.x1) b.x1 = x;
        if (y < b.y1) b.y1 = y;
        if (x > b.x2) b.x2 = x;
        if (y > b.y2) b.y2 = y;
        b.area++;
    }
    // Merge near-touching boxes (occlusion splits), then filter.
    for (size_t i = 0; i < boxes.size(); i++) {
        for (size_t j = i + 1; j < boxes.size();) {
            Box& a = boxes[i];
            Box& b = boxes[j];
            bool nearX = a.x1 <= b.x2 + 6 && b.x1 <= a.x2 + 6;
            bool nearY = a.y1 <= b.y2 + 6 && b.y1 <= a.y2 + 6;
            if (nearX && nearY) {
                if (b.x1 < a.x1) a.x1 = b.x1;
                if (b.y1 < a.y1) a.y1 = b.y1;
                if (b.x2 > a.x2) a.x2 = b.x2;
                if (b.y2 > a.y2) a.y2 = b.y2;
                a.area += b.area;
                boxes.erase(boxes.begin() + (long)j);
            } else {
                j++;
            }
        }
    }
    int kept = 0;
    for (auto& b : boxes) {
        if (kept >= 64) break;
        int bw = b.x2 - b.x1 + 1, bh = b.y2 - b.y1 + 1;
        if (b.area < 40) continue;                       // noise/hitmarkers
        if (b.area > n / 4) continue;                    // fullscreen wash
        if (bh < 8) continue;
        out.push_back((float)b.x1);
        out.push_back((float)b.y1);
        out.push_back((float)b.x2);
        out.push_back((float)b.y2);
        kept++;
    }
    out[0] = (float)kept;
    return out;
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_taizou_paid_TaizouNative_detectBoxes(JNIEnv* env, jobject thiz, jintArray pixels, jint w, jint h) {
    jfloatArray empty = env->NewFloatArray(1);
    if (empty == nullptr) return nullptr;
    float zero = 0.0f;
    env->SetFloatArrayRegion(empty, 0, 1, &zero);
    if (pixels == nullptr || w <= 0 || h <= 0) return empty;
    jsize len = env->GetArrayLength(pixels);
    if (len < w * h || (long)w * h > 4 * 1024 * 1024) return empty;
    std::vector<int32_t> buf((size_t)w * (size_t)h);
    env->GetIntArrayRegion(pixels, 0, w * h, buf.data());
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return empty;
    }
    std::vector<float> boxes = TaizouCore::detectBoxes(buf.data(), (int)w, (int)h);
    jfloatArray out = env->NewFloatArray((jsize)boxes.size());
    if (out == nullptr) return empty;
    env->SetFloatArrayRegion(out, 0, (jsize)boxes.size(), boxes.data());
    return out;
}

// These ESP method definitions landed after the namespace close above;
// reopen it (legal C++) so the TaizouCore:: qualifiers resolve.
namespace taizou {

bool TaizouCore::espReadList(int fd, uint64_t listAddr, std::vector<uint64_t>& pawns) const {
    pawns.clear();
    if (listAddr == 0) return false;
    int32_t sz = espI32(fd, listAddr + 0x18);
    if (sz < 1 || sz > kEspMaxEntities) return false;
    uint64_t items = espU64(fd, listAddr + 0x10);
    if (items == 0) return false;
    for (int i = 0; i < sz; i++) {
        pawns.push_back(espU64(fd, items + 0x20 + (uint64_t)i * 8));
    }
    return true;
}

int TaizouCore::pollEsp(int viewW, int viewH) {
    espFrame_.clear();
    espTotalEnemies_ = 0;
    espTotalBots_ = 0;
    if (viewW <= 0 || viewH <= 0) return 0;
    int pid = 0;
    const char* pkgs[] = {
        "com.garena.game.codm",
        "com.activision.callofduty.shooter",
        "com.vng.codm"
    };
    for (const char* pkg : pkgs) {
        pid = findProcessId(pkg);
        if (pid > 0) break;
    }
    if (pid <= 0) {
        LOGE("esp: game not running");
        return 0;
    }
    if (pid != espPid_) {
        espPid_ = pid;
        espListAddr_ = 0;
        if (!espMatrixOverride_) espHasMatrix_ = false;
    }
    std::string memPath = "/proc/" + std::to_string(pid) + "/mem";
    int fd = open(memPath.c_str(), O_RDONLY);
    espDiagMem_ = fd >= 0;
    if (fd < 0) {
        LOGE("esp: cannot open mem");
        return 0;
    }

    espPollCount_++;
    uint64_t listAddr = 0;
    if (espMatchGameOverride_ != 0) {
        uint64_t cand = espU64(fd, espMatchGameOverride_ + 0x178);
        std::vector<uint64_t> tmp;
        // Validate the override like a scan hit (size + real pawns).
        if (espReadList(fd, cand, tmp) && !tmp.empty() &&
            espScorePawn(fd, tmp[0]) >= 4) {
            listAddr = cand;
        } else {
            LOGE("esp: matchGame override invalid");
        }
    }
    if (listAddr == 0 && espListAddr_ != 0) {
        std::vector<uint64_t> tmp;
        bool readable = espReadList(fd, espListAddr_, tmp) && !tmp.empty();
        // Periodic full re-validation: same-process match changes or a
        // latched garbage list would otherwise stick forever.
        if (readable && espPollCount_ % 60 == 0) {
            int check = 0, score = 0;
            for (uint64_t pawn : tmp) {
                if (check >= 6) break;
                check++;
                if (espScorePawn(fd, pawn) >= 4) score++;
            }
            if (check == 0 || score * 100 / check < 60) {
                LOGE("esp: cached list failed revalidation");
                readable = false;
            }
        }
        if (readable) listAddr = espListAddr_;
        else espListAddr_ = 0;
    }
    if (listAddr == 0) {
        // Throttle full heap scans: expensive, and hopeless more than ~10s apart.
        if (espNoListCooldown_ > 0) {
            espNoListCooldown_--;
            close(fd);
            return 0;
        }
        uint64_t rxS = 0, rxE = 0;
        std::vector<std::pair<uint64_t, uint64_t>> rw;
        if (espMapsRegions(pid, rxS, rxE, rw) && espFindList(fd, rxS, rxE, listAddr)) {
            espListAddr_ = listAddr;
            espNoListCooldown_ = 0;
        } else {
            espNoListCooldown_ = 40;  // ~2s at 50ms polls before rescanning
        }
    }
    if (listAddr == 0) {
        LOGE("esp: no entity list");
        close(fd);
        return 0;
    }

    std::vector<uint64_t> pawns;
    if (!espReadList(fd, listAddr, pawns)) {
        espListAddr_ = 0;
        close(fd);
        return 0;
    }

    // Snapshot pointer-level data (no projection needed for these).
    std::vector<EspEntity> ents;
    for (uint64_t pawn : pawns) {
        if (pawn == 0 || (int)ents.size() >= kEspMaxEntities) continue;
        if (espScorePawn(fd, pawn) < 4) continue;
        EspEntity e;
        e.pawn = pawn;
        e.valid = true;
        uint8_t alive = 0, bot = 0;
        espRead(fd, pawn + kEspAlive, &alive, 1);
        espRead(fd, pawn + kEspBot, &bot, 1);
        e.alive = alive != 0;
        e.isBot = bot != 0;
        if (!e.alive) continue;
        e.boneMesh = espU64(fd, pawn + kEspMesh);
        e.boneHead = espU64(fd, pawn + kEspHeadBone);
        uint64_t atk = espU64(fd, pawn + kEspAttackInfo);
        if (atk != 0) {
            e.curHP = (float)(int)espF32(fd, atk + kEspCurHP);
            e.maxHP = (float)(int)espF32(fd, atk + kEspMaxHP);
            if (!(e.maxHP > 0)) e.maxHP = 100.0f;
        }
        if (!espReadName(fd, pawn, e.isBot, e.name)) continue;  // unreadable identity
        ents.push_back(e);
    }

    // Geometry: re-read a cached matrix every frame (it changes as the
    // camera moves); otherwise run joint (matrix, transform) discovery.
    if (espHasMatrix_ && espMatrixAddr_ != 0 && !espMatrixOverride_) {
        float m[16];
        if (!espRead(fd, espMatrixAddr_, m, sizeof(m)) || !espMatrixSane(m)) {
            LOGE("esp: cached matrix went stale, rediscovering");
            espHasMatrix_ = false;
            espMatrixAddr_ = 0;
        } else {
            memcpy(espVP_, m, sizeof(espVP_));
        }
    }
    if (!espHasMatrix_ && !ents.empty()) {
        float m[16];
        uintptr_t toff = 0;
        uint64_t maddr = 0;
        if (espFindMatrix(fd, ents, viewW, viewH, m, toff, maddr)) {
            memcpy(espVP_, m, sizeof(espVP_));
            espMatrixOff_ = toff;
            espMatrixAddr_ = maddr;
            espHasMatrix_ = true;
            LOGD("esp: geometry resolved (matrixOff=0x%x @%llx)", (unsigned)toff, (unsigned long long)maddr);
        } else {
            LOGE("esp: geometry unresolved (no matrix/transform validated)");
        }
    }
    if (espHasMatrix_) {
        for (auto& e : ents) {
            if (!espBoneWorld(fd, e.boneMesh, espMatrixOff_, e.rootW)) continue;
            if (!espBoneWorld(fd, e.boneHead, espMatrixOff_, e.headW)) continue;
            float dx = e.headW.x - e.rootW.x, dy = e.headW.y - e.rootW.y, dz = e.headW.z - e.rootW.z;
            float h = sqrtf(dx * dx + dy * dy + dz * dz);
            if (h < 0.3f || h > 3.0f) continue;
            float hx, hy, rx, ry;
            if (!espProject(espVP_, e.headW, viewW, viewH, hx, hy)) continue;
            if (!espProject(espVP_, e.rootW, viewW, viewH, rx, ry)) continue;
            float pxH = fabsf(hy - ry);
            if (pxH < 10 || pxH > viewH * 1.5f || hy > ry) continue;
            e.headSX = hx; e.headSY = hy; e.rootSX = rx; e.rootSY = ry;
            e.boxH = pxH; e.boxW = pxH * 0.65f;
            // Camera-depth distance proxy (no local pawn externally).
            e.dist = espVP_[3] * e.rootW.x + espVP_[7] * e.rootW.y + espVP_[11] * e.rootW.z + espVP_[15];
            if (!(e.dist >= 0)) e.dist = 0;
            e.projected = true;
            // Skeleton bones.
            uint64_t boneMap = espU64(fd, e.pawn + kEspBoneMap);
            if (boneMap != 0) {
                uint64_t table = espU64(fd, boneMap + kEspBoneTable);
                if (table != 0) {
                    for (int b = 0; b < 16; b++) {
                        uint64_t node = espU64(fd, table + kEspBoneSlots[b]);
                        if (node == 0) continue;
                        uint64_t tr = espU64(fd, node + kEspBoneNode);
                        EspVec3 w;
                        if (!espBoneWorld(fd, tr, espMatrixOff_, w)) continue;
                        float sx, sy;
                        if (!espProject(espVP_, w, viewW, viewH, sx, sy)) continue;
                        e.bones[b][0] = sx;
                        e.bones[b][1] = sy;
                        e.bones[b][2] = 1.0f;
                    }
                }
            }
        }
    }

    espFrame_.clear();
    espTotalEnemies_ = 0;
    espTotalBots_ = 0;
    // Totals count every validated alive entity (like the reference HUD),
    // while the drawable frame holds projected ones only.
    for (auto& e : ents) {
        if (e.isBot) espTotalBots_++;
        else espTotalEnemies_++;
        if (!e.projected) continue;
        espFrame_.push_back(e);
    }
    LOGD("esp: pawns=%d valid=%d frame=%d bots=%d matrix=%s", (int)pawns.size(),
         (int)ents.size(), (int)espFrame_.size(),
         espTotalBots_, espHasMatrix_ ? "yes" : "no");
    espDiagEnts_ = (int)ents.size();
    espDiagProj_ = (int)espFrame_.size();
    close(fd);
    return (int)espFrame_.size();
}

bool TaizouCore::setEspMatrix(const float* m) {
    if (m == nullptr) return false;
    memcpy(espVP_, m, sizeof(espVP_));
    espHasMatrix_ = true;
    espMatrixOverride_ = true;
    return true;
}

void TaizouCore::setEspMatchGame(uint64_t addr) {
    espMatchGameOverride_ = addr;
    espListAddr_ = 0;  // re-validate through the override
}

int TaizouCore::espEntryCount() const {
    return (int)espFrame_.size();
}

bool TaizouCore::espEntry(int index, float* out12) const {
    if (index < 0 || index >= (int)espFrame_.size() || out12 == nullptr) return false;
    const EspEntity& e = espFrame_[(size_t)index];
    out12[0] = e.headSX; out12[1] = e.headSY;
    out12[2] = e.rootSX; out12[3] = e.rootSY;
    out12[4] = e.boxW; out12[5] = e.boxH;
    out12[6] = e.dist; out12[7] = e.curHP; out12[8] = e.maxHP;
    out12[9] = e.isBot ? 1.0f : 0.0f;
    out12[10] = e.projected ? 1.0f : 0.0f;
    out12[11] = e.alive ? 1.0f : 0.0f;
    return true;
}

std::string TaizouCore::espName(int index) const {
    if (index < 0 || index >= (int)espFrame_.size()) return {};
    return std::string(espFrame_[(size_t)index].name);
}

bool TaizouCore::espBones(int index, float* out48) const {
    if (index < 0 || index >= (int)espFrame_.size() || out48 == nullptr) return false;
    const EspEntity& e = espFrame_[(size_t)index];
    for (int i = 0; i < 16; i++) {
        out48[i * 3] = e.bones[i][0];
        out48[i * 3 + 1] = e.bones[i][1];
        out48[i * 3 + 2] = e.bones[i][2];
    }
    return true;
}

int TaizouCore::espTotalEnemies() const {
    return espTotalEnemies_;
}

int TaizouCore::espTotalBots() const {
    return espTotalBots_;
}

std::string TaizouCore::espDiag() const {
    char buf[512];
    snprintf(buf, sizeof(buf),
             "game pid: %d\nmem: %s\nheap: %d regions %llu MB\nlist: %s\n"
             "candidates: %d bestPawn: %d/6\nentities: %d projected: %d\nmatrix: %s\npolls: %d",
             espPid_, espDiagMem_ ? "open ok" : "OPEN FAIL",
             espDiagRegions_, (unsigned long long)espDiagMB_,
             espListAddr_ ? "FOUND" : "none",
             espDiagChecked_, espDiagBest_,
             espDiagEnts_, espDiagProj_,
             espHasMatrix_ ? "resolved" : "none",
             espPollCount_);
    return std::string(buf);
}

} // namespace taizou