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
#include <thread>
#include <chrono>

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
            {{"libunity.so", 0xC1514C0, hexStringToBytes("20 00 80 D2 C0 03 5F D6")}},
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
            0x6D76CB4, 0x6D76CB8, 0x6D76CBC, 0x6AA24B4, 0x6AA24B8, 0x6AA24BC}},
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

uintptr_t TaizouCore::getLibraryBaseAddress(int pid, const std::string& lib_name) {
    if (pid <= 0) return 0;
    std::string maps_path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream maps(maps_path);
    std::string line;
    while (std::getline(maps, line)) {
        if (line.find(lib_name) != std::string::npos && line.find("r-xp") != std::string::npos) {
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
            std::string byte_str = clean_hex.substr(i, 2);
            bytes.push_back(static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16)));
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

bool TaizouCore::applyMemoryPatch(int pid, const MemoryPatch& patch) {
    if (pid <= 0) return false;
    uintptr_t base = getLibraryBaseAddress(pid, patch.lib_name);
    if (base == 0) return false;

    uintptr_t target_addr = base + patch.offset;
    std::string mem_path = "/proc/" + std::to_string(pid) + "/mem";

    int fd = open(mem_path.c_str(), O_WRONLY);
    if (fd < 0) return false;

    if (lseek(fd, target_addr, SEEK_SET) == -1) {
        close(fd);
        return false;
    }

    ssize_t written = write(fd, patch.bytes.data(), patch.bytes.size());
    close(fd);
    return written == static_cast<ssize_t>(patch.bytes.size());
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
}

void TaizouCore::setRadioButtonState(const std::string& group, const std::string& name) {
    for (auto& [key, rb] : radiobuttons_) {
        if (rb.group == group) {
            rb.checked = (rb.name == name);
            if (rb.checked) {
                executeNativeBinaryRoot(rb.lib_name, rb.code);
            }
        }
    }
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

void TaizouCore::executeNativeBinary(const std::string& binary_name, const std::string& args) {
    std::string cmd = "chmod 777 " + binary_name + " && " + binary_name + " " + args;
    system(cmd.c_str());
}

void TaizouCore::executeNativeBinaryRoot(const std::string& binary_name, const std::string& args) {
    std::string cmd = "su -c 'chmod 777 " + binary_name + " && " + binary_name + " " + args + "'";
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