# TaizouInjector - Android Port

Modern Android port of the original AndLua (ALP) CODM injector project.

## Features

- **CODM Memory Hacking**: Wallhack, aimbot, no recoil, no spread, rapid fire, etc.
- **Skin Unlocker**: Mythic, Epic, Legendary, Melee, Vehicle skins
- **Config Management**: Save/load configurations (JSON + SharedPreferences)
- **Floating Overlay**: Draggable SYSTEM_ALERT_WINDOW with 6-tab cheat menu
- **Custom Animations**: Analog clock, ECG wave, RGB-cycling toasts
- **TTS Announcements**: Voice feedback for feature activation
- **Anti-Debug**: Detects HTTP Canary, SSTool, Luadec, DroidC
- **Root Execution**: Native binary execution via su for skin patches

## Requirements

- Android 5.0+ (API 21)
- ARM64 device (arm64-v8a)
- **Root access** (Magisk/KernelSU) for memory patching
- Overlay permission (SYSTEM_ALERT_WINDOW)
- Storage permissions for config/assets

## Architecture

```
Kotlin (UI/Android Framework)
    ↓ JNI
C++17 (Core: Memory Patching, Process Utils, Config)
    ↓ Native
ARM64 ELF Binaries (50+ skin/character patches)
```

## Project Structure

```
TaizouInjector/
├── .github/workflows/build.yml      # GitHub Actions CI/CD
├── app/
│   ├── src/main/
│   │   ├── java/com/taizou/paid/    # Kotlin source
│   │   ├── cpp/                     # C++17 native core
│   │   ├── res/                     # XML layouts, drawables, values
│   │   ├── assets/
│   │   │   ├── fonts/               # Custom fonts
│   │   │   ├── video/bg.mp4         # Background video
│   │   │   └── native/              # Extracted Res/* binaries
│   │   └── jniLibs/arm64-v8a/       # Native ELF libraries
│   └── build.gradle.kts
├── gradle.properties                 # Version/config
└── settings.gradle.kts
```

## Building Locally

```bash
# Prerequisites
# - Android SDK (API 34)
# - NDK r26c+
# - JDK 17
# - Gradle 8.5+

# Clone and build
git clone <repo-url>
cd TaizouInjector
./gradlew assembleDebug
# APK at: app/build/outputs/apk/debug/app-debug.apk

# Release build (requires signing config in local.properties)
./gradlew assembleRelease
```

## GitHub Actions CI/CD Setup

### 1. Create Private Repository

```bash
# Using GitHub CLI
gh repo create TaizouInjector --private --source=. --push

# Or manually at github.com/new
# - Repository name: TaizouInjector
# - Private: ✓
# - Initialize with: No (we have files)
```

### 2. Configure Repository Secrets

Go to **Settings → Secrets and variables → Actions → New repository secret**:

| Secret Name | Value | Required |
|-------------|-------|----------|
| `KEYSTORE_BASE64` | Base64-encoded keystore.jks | For release signing |
| `KEYSTORE_PASSWORD` | Keystore password | For release signing |
| `KEY_ALIAS` | Key alias (e.g., `taizou`) | For release signing |
| `KEY_PASSWORD` | Key password | For release signing |
| `GITHUB_TOKEN` | Auto-provided (`${{ secrets.GITHUB_TOKEN }}`) | For releases/packages |

Generate keystore:
```bash
keytool -genkeypair -v -keystore keystore.jks -keyalg RSA -keysize 2048 -validity 10000 -alias taizou
base64 -w0 keystore.jks  # Copy output to KEYSTORE_BASE64 secret
```

### 3. Enable GitHub Actions

1. Push to main branch
2. Actions tab → "I understand my workflows, go ahead and enable them"
3. Workflow runs on push/PR/tag

### 4. Create Release

```bash
# Tag a version
git tag v1.6.57
git push origin v1.6.57
```

This triggers:
- Debug + Release builds
- APK artifacts uploaded (30-day retention)
- GitHub Release created with signed APK
- (Optional) Maven package published to GitHub Packages

### 5. Workflow Triggers

| Event | Action |
|-------|--------|
| Push to main/master | Build debug + release |
| Pull Request | Build debug + release |
| Tag `v*` | Build + Create Release + Publish |
| Manual (`workflow_dispatch`) | Choose debug/release |

## Signing Configuration

Create `local.properties` (not committed):

```properties
storeFile=keystore.jks
storePassword=your_keystore_password
keyAlias=taizou
keyPassword=your_key_password
```

In CI, these come from repository secrets (decoded in workflow).

## Native Libraries

The 50+ ARM64 ELF binaries are bundled in:
- `app/src/main/jniLibs/arm64-v8a/` - For APK packaging
- `app/src/main/assets/native/` - Extracted at runtime to `files/Res/`

Libraries include: `charss`, `Cin`, `Rambo`, `Roze`, `TuwadNaForXiel...`, `xielskins1`, etc.

## Memory Patching

Targets `libunity.so` and `libanogs.so` in `com.garena.game.codm` process:
- Reads `/proc/pid/maps` for base address
- Writes to `/proc/pid/mem` at calculated offsets
- Offsets defined in `taizou_core.cpp` (30+ checkboxes, 5 seekbars)

## License

Private/Proprietary - Do not distribute without permission.

## Disclaimer

This tool modifies game memory. Use at your own risk. May violate game ToS. For educational purposes only.
## Build target
This project is configured for ARM64 (`arm64-v8a`) only. The GitHub Actions workflow verifies that the generated APK contains `lib/arm64-v8a/libtaizou_core.so` and does not contain other ABI directories.
