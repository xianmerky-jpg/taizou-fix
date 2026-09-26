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
    uintptr_t offset7 = 0;
    uintptr_t offset8 = 0;
    uintptr_t offset9 = 0;
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
    uintptr_t getLibraryBaseAddress(int pid, const std::string& lib_name, bool executableOnly = false);

    bool applyMemoryPatch(int pid, const MemoryPatch& patch, bool executableOnly = false);
    bool applyMemoryPatch(const std::string& lib_name, uintptr_t offset, const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> hexStringToBytes(const std::string& hex);

    void setCheckBoxState(const std::string& name, bool checked);
    void setSeekBarProgress(const std::string& name, int progress);
    void setRadioButtonState(const std::string& group, const std::string& name);

    std::string saveConfig();
    bool loadConfig(const std::string& json);

    void executeNativeBinary(const std::string& binary_name, const std::string& args);
    void executeNativeBinaryRoot(const std::string& binary_name, const std::string& args);

    // Original AndLua auto-bypass: 18 libanogs.so patches applied once the
    // game + library are detected (see applyAutoBypass in taizou_core.cpp).
    bool applyAutoBypass();

    // Original clogs cache-clean list (best-effort file deletes).
    void clearLogs();

    void speakText(const std::string& text);
    void showToast(const std::string& message);

    static std::vector<uint8_t> floatToHexLE(float value);

    // ---- External ESP (reads only, never writes) ----
    struct EspVec3 { float x = 0, y = 0, z = 0; };
    struct EspEntity {
        bool valid = false;
        bool isBot = false;
        bool alive = false;
        uint64_t pawn = 0;
        uint64_t boneMesh = 0;    // root Transform*
        uint64_t boneHead = 0;    // head Transform*
        EspVec3 rootW{0, 0, 0}, headW{0, 0, 0};   // world
        float headSX = 0, headSY = 0;             // screen
        float rootSX = 0, rootSY = 0;             // screen
        float boxW = 0, boxH = 0;
        float dist = -1, curHP = 0, maxHP = 0;
        char name[48] = {0};
        float bones[16][3] = {};                  // screen x, y, visible
        bool projected = false;
    };

    // View-projection matrix override (float[16]); empty = auto-discover.
    bool setEspMatrix(const float* m);
    // MatchGame/enemy-list anchor override (0 = auto-discover).
    void setEspMatchGame(uint64_t addr);
    // Refresh snapshot; returns entity count. viewW/H = overlay pixels.
    int pollEsp(int viewW, int viewH);
    int espEntryCount() const;
    bool espEntry(int index, float* out12) const;
    std::string espName(int index) const;
    bool espBones(int index, float* out48) const;
    int espTotalEnemies() const;
    int espTotalBots() const;
    bool espHasList() const { return espListAddr_ != 0; }
    std::string espDiag() const;

private:
    void initializeDefaultConfigs();
    // Bare binary names (e.g. "charss") live under files/Res/ after asset
    // extraction; absolute paths pass through untouched.
    std::string resolveBinaryPath(const std::string& binary_name);

    // ---- ESP internals ----
    bool espRead(int fd, uintptr_t addr, void* out, size_t len) const;
    uint64_t espU64(int fd, uintptr_t addr) const;
    int32_t espI32(int fd, uintptr_t addr) const;
    float espF32(int fd, uintptr_t addr) const;
    // Pawn pointer sanity (PlayerInfo/HP/bones present). No positions needed.
    int espScorePawn(int fd, uint64_t pawn) const;
    bool espReadName(int fd, uint64_t pawn, bool isBot, char out[48]) const;
    // Transform world position via localToWorld translation at matrixOff.
    bool espBoneWorld(int fd, uint64_t transformPtr, uintptr_t matrixOff, EspVec3& out) const;
    bool espProject(const float m[16], const EspVec3& w, int viewW, int viewH, float& sx, float& sy) const;
    // Discovery (bounded + logged). Returns false when nothing validated.
    bool espFindList(int fd, uint64_t rxStart, uint64_t rxEnd, uint64_t& listAddr) const;
    bool espReadList(int fd, uint64_t listAddr, std::vector<uint64_t>& pawns) const;
    bool espFindMatrix(int fd, const std::vector<EspEntity>& ents, int viewW, int viewH,
                       float outM[16], uintptr_t& outMatrixOff, uint64_t& outMatrixAddr) const;
    bool espMapsRegions(int pid, uint64_t& rxStart, uint64_t& rxEnd,
                        std::vector<std::pair<uint64_t, uint64_t>>& rwRegions) const;

    std::vector<EspEntity> espFrame_;
    int espTotalEnemies_ = 0;
    int espTotalBots_ = 0;
    float espVP_[16] = {0};
    bool espHasMatrix_ = false;
    uintptr_t espMatrixOff_ = 0;   // winning Transform matrix offset
    uint64_t espListAddr_ = 0;     // validated EnemyPawns list object
    uint64_t espMatchGameOverride_ = 0;
    bool espMatrixOverride_ = false;
    uint64_t espMatrixAddr_ = 0;   // where the winning VP matrix was read
    int espPid_ = 0;
    int espPollCount_ = 0;
    int espNoListCooldown_ = 0;
    // Last-run diagnostics surfaced to the UI.
    int espDiagChecked_ = 0;
    int espDiagBest_ = 0;
    int espDiagRegions_ = 0;
    uint64_t espDiagMB_ = 0;
    int espDiagEnts_ = 0;
    int espDiagProj_ = 0;
    int espDiagPolls_ = 0;
    bool espDiagMem_ = false;

    JavaVM* jvm_ = nullptr;
    jobject global_context_ = nullptr;
    std::map<std::string, CheckBoxConfig> checkboxes_;
    std::map<std::string, SeekBarConfig> seekbars_;
    // multimap: several entries share one group key (e.g. 10x "character").
    // A plain map would silently drop all but the first per group.
    std::multimap<std::string, RadioButtonConfig> radiobuttons_;
    std::string files_dir_;
    std::string config_path_;
    bool initialized_ = false;
    bool root_available_ = false;
};

// NOTE: namespace must be closed here. Leaving it open wraps every header
// included after this one (e.g. <fstream> in taizou_core.cpp) inside
// namespace taizou, which breaks libc++ lookup and fails the NDK build
// with errors like "unknown class name 'false_type'" in <system_error>.
} // namespace taizou

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

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_applyAutoBypass(JNIEnv* env, jobject thiz);

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_isLibraryLoaded(JNIEnv* env, jobject thiz, jint pid, jstring libName);

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_clearLogs(JNIEnv* env, jobject thiz);

extern "C" JNIEXPORT jint JNICALL
Java_com_taizou_paid_TaizouNative_pollEsp(JNIEnv* env, jobject thiz, jint viewW, jint viewH);

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_getEspEntry(JNIEnv* env, jobject thiz, jint index, jfloatArray out);

extern "C" JNIEXPORT jstring JNICALL
Java_com_taizou_paid_TaizouNative_getEspName(JNIEnv* env, jobject thiz, jint index);

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_getEspBones(JNIEnv* env, jobject thiz, jint index, jfloatArray out);

extern "C" JNIEXPORT jintArray JNICALL
Java_com_taizou_paid_TaizouNative_getEspTotals(JNIEnv* env, jobject thiz);

extern "C" JNIEXPORT void JNICALL
Java_com_taizou_paid_TaizouNative_setEspMatchGame(JNIEnv* env, jobject thiz, jlong addr);

extern "C" JNIEXPORT jboolean JNICALL
Java_com_taizou_paid_TaizouNative_setEspMatrix(JNIEnv* env, jobject thiz, jfloatArray values);

extern "C" JNIEXPORT jstring JNICALL
Java_com_taizou_paid_TaizouNative_getEspDiag(JNIEnv* env, jobject thiz);

#endif // TAIZOU_CORE_H