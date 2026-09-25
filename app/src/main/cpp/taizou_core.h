#ifndef TAIZOU_CORE_H
#define TAIZOU_CORE_H

#include <jni.h>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>

namespace taizou {

struct MemoryPatch {
    std::string lib_name;
    uintptr_t offset;
    std::vector<uint8_t> bytes;
    bool enabled = false;
};

struct SeekBarConfig {
    std::string name;
    int progress = 0;
    uintptr_t offset1 = 0;
    uintptr_t offset2 = 0;
    uintptr_t offset3 = 0;
    uintptr_t offset4 = 0;
    uintptr_t offset5 = 0;
    uintptr_t offset6 = 0;
};

struct CheckBoxConfig {
    std::string name;
    bool checked = false;
    std::vector<MemoryPatch> patches_on;
    std::vector<MemoryPatch> patches_off;
};

struct RadioButtonConfig {
    std::string name;
    std::string group;
    bool checked = false;
    std::string lib_name;
    std::string code;
};

class TaizouCore {
public:
    TaizouCore();
    ~TaizouCore();

    bool initialize(JNIEnv* env, jobject context);
    void shutdown();

    bool hasRootAccess();
    bool checkOverlayPermission();
    void requestOverlayPermission(JNIEnv* env, jobject activity);

    int findProcessId(const std::string& package_name);
    uintptr_t getLibraryBaseAddress(int pid, const std::string& lib_name);

    bool applyMemoryPatch(int pid, const MemoryPatch& patch);
    bool applyMemoryPatch(const std::string& lib_name, uintptr_t offset, const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> hexStringToBytes(const std::string& hex);

    void setCheckBoxState(const std::string& name, bool checked);
    void setSeekBarProgress(const std::string& name, int progress);
    void setRadioButtonState(const std::string& group, const std::string& name);

    std::string saveConfig();
    bool loadConfig(const std::string& json);

    void executeNativeBinary(const std::string& binary_name, const std::string& args);
    void executeNativeBinaryRoot(const std::string& binary_name, const std::string& args);

    void speakText(const std::string& text);
    void showToast(const std::string& message);

    static std::vector<uint8_t> floatToHexLE(float value);

private:
    JavaVM* jvm_ = nullptr;
    jobject global_context_ = nullptr;
    std::map<std::string, CheckBoxConfig> checkboxes_;
    std::map<std::string, SeekBarConfig> seekbars_;
    std::map<std::string, RadioButtonConfig> radiobuttons_;
    std::string config_path_;
    bool initialized_ = false;
    bool root_available_ = false;
};

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_initialize(JNIEnv* env, jobject thiz, jobject context);

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_shutdown(JNIEnv* env, jobject thiz);

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_hasRootAccess(JNIEnv* env, jobject thiz);

extern "C" JNIEXPORT jint JNICALL
Java_com_taizou_paid_TaizouNative_findProcessId(JNIEnv* env, jobject thiz, jstring packageName);

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_applyMemoryPatch(JNIEnv* env, jobject thiz, jstring libName, jlong offset, jbyteArray bytes);

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_applyMemoryPatchOffset(JNIEnv* env, jobject thiz, jstring libName, jlong offset, jstring hexBytes);

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_setCheckBoxState(JNIEnv* env, jobject thiz, jstring name, jboolean checked);

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_setSeekBarProgress(JNIEnv* env, jobject thiz, jstring name, jint progress);

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_setRadioButtonState(JNIEnv* env, jobject thiz, jstring group, jstring name);

extern "C" JNIEXPORT jstring JNICALL
Java_com_taizou_paid_TaizouNative_saveConfig(JNIEnv* env, jobject thiz);

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_loadConfig(JNIEnv* env, jobject thiz, jstring json);

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_executeNativeBinary(JNIEnv* env, jobject thiz, jstring binaryName, jstring args);

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_executeNativeBinaryRoot(JNIEnv* env, jobject thiz, jstring binaryName, jstring args);

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_speakText(JNIEnv* env, jobject thiz, jstring text);

#endif // TAIZOU_CORE_H