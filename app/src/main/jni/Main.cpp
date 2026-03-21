#include <jni.h>

#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "DialogOnLoad.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "Substrate/SubstrateHook.h"

namespace {

constexpr const char* kLogTag = "teste";
const char* kTargetLibName = OBFUSCATE("libil2cpp.so");
constexpr const char* kMenuTitle = "teste";
constexpr const char* kMenuSubtitle = "teste";
constexpr const char* kIconBase64 =
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+aF9sAAAAASUVORK5CYII=";
constexpr const char* kIconWebData =
        "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+aF9sAAAAASUVORK5CYII=";
constexpr uintptr_t kPlayerControlApplyDamageRva = 0xBEEE58;
constexpr uintptr_t kPlayerControlApplyDirectDamageRva = 0xBF2B6C;
constexpr uintptr_t kPlayerControlSendShootCommandRva = 0xBD2B20;
constexpr uintptr_t kPlayerControlActionShootRva = 0xBD0ECC;
constexpr uintptr_t kEnemyControllerApplyDamageRva = 0xB115A8;
constexpr uintptr_t kEnemyControllerKillEnemyRva = 0xB14590;
constexpr uintptr_t kPlayerControlSceneControllerOffset = 0x24;
constexpr uintptr_t kPlayerControlInitialLifeOffset = 0x6C;
constexpr uintptr_t kPlayerControlActualLifeOffset = 0x70;
constexpr uintptr_t kPlayerControlIsDeadOffset = 0x524;
constexpr uintptr_t kPlayerControlCameraOffset = 0x1E4;
constexpr uintptr_t kPlayerControlMoveCamRva = 0xBE15C0;
constexpr uintptr_t kSceneControllerMapEnemiesOffset = 0x58;
constexpr uintptr_t kSceneControllerWaveEnemiesOffset = 0x5C;
constexpr uintptr_t kSceneControllerStateOffset = 0x30;
constexpr uintptr_t kSceneControllerMyPlayerControlOffset = 0x38;
constexpr uintptr_t kSceneControllerIsPlayingMissionOffset = 0x131;
constexpr uintptr_t kEnemyControllerIsActiveOffset = 0x104;
constexpr uintptr_t kEnemyControllerIsDeadOffset = 0x106;
constexpr uintptr_t kEnemyControllerIsAwareOffset = 0x108;
constexpr uintptr_t kEnemyControllerIsShootingOffset = 0x114;
constexpr uintptr_t kEnemyControllerPlayerControlOffset = 0x1C;
constexpr uintptr_t kEnemyControllerTransformOffset = 0x12C;
constexpr uintptr_t kManagedListItemsOffset = 0x8;
constexpr uintptr_t kManagedListSizeOffset = 0xC;
constexpr uintptr_t kManagedArrayFirstItemOffset = 0x10;
constexpr uintptr_t kCameraGetMainRva = 0x210E7F8;
constexpr uintptr_t kCameraGetCurrentRva = 0x210E838;
constexpr uintptr_t kCameraWorldToScreenPointInjectedRva = 0x210E210;
constexpr uintptr_t kScreenGetWidthRva = 0x2118538;
constexpr uintptr_t kScreenGetHeightRva = 0x2118578;
constexpr uintptr_t kTransformGetPositionInjectedRva = 0x2148AA4;
constexpr uintptr_t kComponentGetTransformRva = 0x2139D90;
constexpr uintptr_t kTransformLookAtRva = 0x214A5AC;
constexpr uintptr_t kPhysicsRaycastRva = 0x2196C2C;
constexpr uintptr_t kEnemyHeadTransformOffset = 0x238;
constexpr uintptr_t kEnemySpine2TransformOffset = 0x234;
constexpr uintptr_t kEnemyHipsTransformOffset = 0x230;
constexpr uintptr_t kEnemyLeftArmTransformOffset = 0x23C;
constexpr uintptr_t kEnemyRightArmTransformOffset = 0x240;
constexpr uintptr_t kEnemyRightForeArmTransformOffset = 0x244;
constexpr uintptr_t kEnemyLeftForeArmTransformOffset = 0x248;
constexpr uintptr_t kEnemyLeftUpLegTransformOffset = 0x24C;
constexpr uintptr_t kEnemyRightUpLegTransformOffset = 0x250;
constexpr uintptr_t kEnemyLeftLegTransformOffset = 0x254;
constexpr uintptr_t kEnemyRightLegTransformOffset = 0x258;
constexpr size_t kManagedPointerSize = sizeof(void*);
constexpr int kMaxEnemyLinesToDraw = 24;
constexpr int kCameraMonoEye = 2;
constexpr int kSceneStatePrefinal = 4;
constexpr int kSceneStateFinalMission = 5;
constexpr int kEnemyBodyPartChest = 2;
constexpr long long kDrawMinFrameIntervalMs = 20;
constexpr long long kDrawFailureBackoffMs = 1500;

struct Vec3 {
    float x;
    float y;
    float z;
};

using ApplyDamage_t = void (*)(void*,
                               float,
                               Vec3,
                               Vec3,
                               Vec3,
                               void*,
                               int,
                               void*);
using ApplyDirectDamage_t = void (*)(void*, float, Vec3, Vec3, int);
using SendShootCommand_t = void (*)(void*);
using CameraGetMain_t = void* (*)();
using CameraGetCurrent_t = void* (*)();
using CameraWorldToScreenPoint_Injected_t = void (*)(void*, Vec3*, int, Vec3*);
using ScreenGetWidth_t = int (*)();
using ScreenGetHeight_t = int (*)();
using TransformGetPosition_Injected_t = void (*)(void*, Vec3*);
using EnemyApplyDamage_t = void (*)(void*, float, float, Vec3, void*, Vec3, int, bool);
using EnemyKill_t = void (*)(void*);
using ComponentGetTransform_t = void* (*)(void*);
using TransformLookAt_t = void (*)(void*, Vec3);
using PhysicsRaycast_t = bool (*)(Vec3, Vec3, float, int);
using MoveCam_t = void (*)(void*);
using ActionShoot_t = void* (*)(void*);

ApplyDamage_t orig_ApplyDamage = nullptr;
ApplyDirectDamage_t orig_ApplyDirectDamage = nullptr;
SendShootCommand_t orig_SendShootCommand = nullptr;
CameraGetMain_t gCameraGetMain = nullptr;
CameraGetCurrent_t gCameraGetCurrent = nullptr;
CameraWorldToScreenPoint_Injected_t gCameraWorldToScreenPoint_Injected = nullptr;
ScreenGetWidth_t gScreenGetWidth = nullptr;
ScreenGetHeight_t gScreenGetHeight = nullptr;
TransformGetPosition_Injected_t gTransformGetPosition_Injected = nullptr;
EnemyApplyDamage_t gEnemyApplyDamage = nullptr;
EnemyApplyDamage_t orig_EnemyApplyDamage = nullptr;
EnemyKill_t gEnemyKillEnemy = nullptr;
ComponentGetTransform_t gComponentGetTransform = nullptr;
TransformLookAt_t gTransformLookAt = nullptr;
PhysicsRaycast_t gPhysicsRaycast = nullptr;
MoveCam_t orig_MoveCam = nullptr;
ActionShoot_t orig_ActionShoot = nullptr;
bool gGodMode = false;
bool gInfiniteLife999 = false;
bool gHookInstalled = false;
bool gEnemyLines = false;
bool gEnemySkeleton = false;
bool gEspOnlyAware = false;
bool gEspOnlyShooting = false;
bool gTriggerBot = false;
bool gAutoKillAll = false;
bool gKillAllNowRequested = false;
bool gAimBot = false;
int gLineOriginMode = 2;
int gEspColorPreset = 3;
void* gLocalPlayerControl = nullptr;
void* gLastTriggerTarget = nullptr;
float gTriggerRadiusPixels = 90.0f;
float gTriggerDamage = 200.0f;
float gDamageMultiplier = 1.0f;
constexpr useconds_t kTriggerLoopDelayUs = 30000;
constexpr long long kTriggerWindowMs = 260;
constexpr long long kTriggerRefireCooldownMs = 150;
constexpr float kTriggerLockBias = 1.35f;
constexpr int kTriggerBurstShots = 1;
constexpr long long kLocalPlayerStaleTimeoutMs = 3000;
constexpr long long kAutoKillTickCooldownMs = 180;
long long gLastShootCommandTimeMs = 0;
long long gLastTriggerFireTimeMs = 0;
long long gLastLocalPlayerSeenMs = 0;
long long gLastAutoKillTickMs = 0;

struct EnemyListSnapshot {
    int total = 0;
    int active = 0;
    void* enemies[kMaxEnemyLinesToDraw] = {};
};

long long GetMonotonicTimeMs();
bool HasFreshLocalPlayerControl();
EnemyListSnapshot ReadEnemyListFromSceneOffset(uintptr_t listOffset);
bool TryGetScreenSize(int* outWidth, int* outHeight);
bool TryGetEnemyScreenPosition(void* enemy, float canvasHeight, float* outX, float* outY);

Vec3 SubVec3(const Vec3& a, const Vec3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 MulVec3(const Vec3& v, float scalar) {
    return {v.x * scalar, v.y * scalar, v.z * scalar};
}

float DotVec3(const Vec3& a, const Vec3& b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

float LengthSqVec3(const Vec3& v) {
    return DotVec3(v, v);
}

bool NormalizeVec3(Vec3* v) {
    if (!v) {
        return false;
    }

    const float lenSq = LengthSqVec3(*v);
    if (lenSq <= 0.000001f) {
        return false;
    }

    const float invLen = 1.0f / std::sqrt(lenSq);
    *v = MulVec3(*v, invLen);
    return true;
}

void SetLocalPlayerLife999(void* player) {
    if (!player) {
        return;
    }

    *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + kPlayerControlInitialLifeOffset) = 999.0f;
    *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + kPlayerControlActualLifeOffset) = 999.0f;
    *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(player) + kPlayerControlIsDeadOffset) = false;
}

bool IsInfiniteLifeEnabled() {
    return gHookInstalled && gInfiniteLife999;
}

bool IsGodModeEnabled() {
    return gHookInstalled && gGodMode;
}

bool IsEnemyLineEspEnabled() {
    return gHookInstalled && gEnemyLines;
}

bool IsEnemySkeletonEspEnabled() {
    return gHookInstalled && gEnemySkeleton;
}

bool IsAnyVisualEspEnabled() {
    return IsEnemyLineEspEnabled() || IsEnemySkeletonEspEnabled();
}

const char* GetEspColorPresetName() {
    switch (gEspColorPreset) {
        case 0: return "Verde";
        case 1: return "Vermelho";
        case 2: return "Azul";
        case 3: return "Ciano";
        case 4: return "Amarelo";
        case 5: return "Branco";
        case 6: return "Rosa";
        case 7: return "Laranja";
        default: return "Ciano";
    }
}

void GetEspPresetColor(int* outR, int* outG, int* outB) {
    if (!outR || !outG || !outB) {
        return;
    }

    switch (gEspColorPreset) {
        case 0: *outR = 90;  *outG = 255; *outB = 120; break;
        case 1: *outR = 255; *outG = 96;  *outB = 96;  break;
        case 2: *outR = 90;  *outG = 150; *outB = 255; break;
        case 3: *outR = 72;  *outG = 200; *outB = 255; break;
        case 4: *outR = 255; *outG = 220; *outB = 80;  break;
        case 5: *outR = 245; *outG = 245; *outB = 245; break;
        case 6: *outR = 255; *outG = 120; *outB = 210; break;
        case 7: *outR = 255; *outG = 155; *outB = 72;  break;
        default:*outR = 72;  *outG = 200; *outB = 255; break;
    }
}

bool IsTriggerBotEnabled() {
    return gHookInstalled && gTriggerBot && gEnemyApplyDamage;
}

bool IsAutoKillAllEnabled() {
    return gHookInstalled && gAutoKillAll && gEnemyApplyDamage;
}

bool IsAimBotEnabled() {
    return gAimBot && HasFreshLocalPlayerControl() && gComponentGetTransform && gTransformLookAt;
}

bool IsDamageMultiplierEnabled() {
    return gHookInstalled && gDamageMultiplier > 1.0f;
}

float ApplyDamageMultiplierIfNeeded(float baseDamage) {
    if (!IsDamageMultiplierEnabled() || baseDamage <= 0.0f) {
        return baseDamage;
    }

    return baseDamage * gDamageMultiplier;
}

void ClearLocalPlayerControl() {
    gLocalPlayerControl = nullptr;
    gLastTriggerTarget = nullptr;
    gLastLocalPlayerSeenMs = 0;
}

void UpdateLocalPlayerControl(void* player) {
    if (!player) {
        return;
    }

    gLocalPlayerControl = player;
    gLastLocalPlayerSeenMs = GetMonotonicTimeMs();
}

bool HasFreshLocalPlayerControl() {
    if (!gLocalPlayerControl) {
        return false;
    }

    const long long lastSeenMs = gLastLocalPlayerSeenMs;
    if (lastSeenMs <= 0) {
        return false;
    }

    if ((GetMonotonicTimeMs() - lastSeenMs) > kLocalPlayerStaleTimeoutMs) {
        ClearLocalPlayerControl();
        return false;
    }

    return true;
}

bool IsSceneControllerGameplayActive(void* sceneController, void* player) {
    if (!sceneController || !player) {
        return false;
    }

    const auto sceneAddr = reinterpret_cast<uintptr_t>(sceneController);
    const int sceneState = *reinterpret_cast<int*>(sceneAddr + kSceneControllerStateOffset);
    const bool isPlayingMission = *reinterpret_cast<bool*>(sceneAddr + kSceneControllerIsPlayingMissionOffset);
    auto* currentPlayer = *reinterpret_cast<void**>(sceneAddr + kSceneControllerMyPlayerControlOffset);

    if (!isPlayingMission) {
        return false;
    }

    if (sceneState >= kSceneStatePrefinal || sceneState == kSceneStateFinalMission) {
        return false;
    }

    return currentPlayer == player;
}

bool IsEnemyOwnedByLocalPlayer(void* enemy) {
    if (!enemy || !HasFreshLocalPlayerControl()) {
        return false;
    }

    auto* enemyPlayer = *reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(enemy) + kEnemyControllerPlayerControlOffset);
    return enemyPlayer == gLocalPlayerControl;
}

long long GetMonotonicTimeMs() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (static_cast<long long>(ts.tv_sec) * 1000LL) + (static_cast<long long>(ts.tv_nsec) / 1000000LL);
}

bool IsTriggerWindowOpen() {
    const long long lastShootTime = gLastShootCommandTimeMs;
    if (lastShootTime <= 0) {
        return false;
    }

    return (GetMonotonicTimeMs() - lastShootTime) <= kTriggerWindowMs;
}

EnemyListSnapshot ReadLocalEnemyList() {
    EnemyListSnapshot snapshot;
    return ReadEnemyListFromSceneOffset(kSceneControllerMapEnemiesOffset);
}

EnemyListSnapshot ReadEnemyListFromSceneOffset(uintptr_t listOffset) {
    EnemyListSnapshot snapshot;

    if (!HasFreshLocalPlayerControl()) {
        return snapshot;
    }

    auto* player = reinterpret_cast<uintptr_t*>(gLocalPlayerControl);
    if (!player) {
        return snapshot;
    }

    auto* sceneController = *reinterpret_cast<uintptr_t**>(
            reinterpret_cast<uintptr_t>(player) + kPlayerControlSceneControllerOffset);
    if (!sceneController) {
        ClearLocalPlayerControl();
        return snapshot;
    }

    if (!IsSceneControllerGameplayActive(sceneController, player)) {
        ClearLocalPlayerControl();
        return snapshot;
    }

    auto* enemyList = *reinterpret_cast<uintptr_t**>(
            reinterpret_cast<uintptr_t>(sceneController) + listOffset);
    if (!enemyList) {
        return snapshot;
    }

    auto* items = *reinterpret_cast<uintptr_t**>(reinterpret_cast<uintptr_t>(enemyList) + kManagedListItemsOffset);
    const int size = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(enemyList) + kManagedListSizeOffset);
    if (!items || size <= 0) {
        gLastTriggerTarget = nullptr;
        return snapshot;
    }

    snapshot.total = size;

    const int limit = size < kMaxEnemyLinesToDraw ? size : kMaxEnemyLinesToDraw;
    for (int i = 0; i < limit; ++i) {
        auto enemy = *reinterpret_cast<void**>(
                reinterpret_cast<uintptr_t>(items) + kManagedArrayFirstItemOffset + (i * kManagedPointerSize));
        if (!enemy) {
            continue;
        }

        const auto isActive = *reinterpret_cast<bool*>(
                reinterpret_cast<uintptr_t>(enemy) + kEnemyControllerIsActiveOffset);
        const auto isDead = *reinterpret_cast<bool*>(
                reinterpret_cast<uintptr_t>(enemy) + kEnemyControllerIsDeadOffset);
        if (!isActive || isDead) {
            continue;
        }

        snapshot.enemies[snapshot.active++] = enemy;
    }

    return snapshot;
}

EnemyListSnapshot ReadCurrentWaveEnemyList() {
    return ReadEnemyListFromSceneOffset(kSceneControllerWaveEnemiesOffset);
}

bool FindBestTargetInSnapshot(const EnemyListSnapshot& snapshot, void** outEnemy) {
    if (!outEnemy) {
        return false;
    }

    *outEnemy = nullptr;

    if (!HasFreshLocalPlayerControl()) {
        return false;
    }

    int screenWidth = 0;
    int screenHeight = 0;
    if (!TryGetScreenSize(&screenWidth, &screenHeight) || screenWidth <= 0 || screenHeight <= 0) {
        return false;
    }

    if (snapshot.active <= 0) {
        return false;
    }

    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float centerY = static_cast<float>(screenHeight) * 0.5f;
    const float maxDistanceSq = gTriggerRadiusPixels * gTriggerRadiusPixels;
    float bestDistanceSq = maxDistanceSq;
    void* bestEnemy = nullptr;

    for (int i = 0; i < snapshot.active; ++i) {
        float targetX = 0.0f;
        float targetY = 0.0f;
        if (!TryGetEnemyScreenPosition(snapshot.enemies[i], static_cast<float>(screenHeight), &targetX, &targetY)) {
            continue;
        }

        const float dx = targetX - centerX;
        const float dy = targetY - centerY;
        const float distanceSq = (dx * dx) + (dy * dy);
        if (distanceSq > bestDistanceSq) {
            continue;
        }

        bestDistanceSq = distanceSq;
        bestEnemy = snapshot.enemies[i];
    }

    *outEnemy = bestEnemy;
    return bestEnemy != nullptr;
}

void ResolveUnityDrawApis() {
    if (gCameraGetMain && gCameraWorldToScreenPoint_Injected && gTransformGetPosition_Injected && gScreenGetWidth &&
        gScreenGetHeight) {
        return;
    }

    gCameraGetMain = reinterpret_cast<CameraGetMain_t>(getAbsoluteAddress(kTargetLibName, kCameraGetMainRva));
    gCameraGetCurrent = reinterpret_cast<CameraGetCurrent_t>(getAbsoluteAddress(kTargetLibName, kCameraGetCurrentRva));
    gCameraWorldToScreenPoint_Injected = reinterpret_cast<CameraWorldToScreenPoint_Injected_t>(
            getAbsoluteAddress(kTargetLibName, kCameraWorldToScreenPointInjectedRva));
    gScreenGetWidth = reinterpret_cast<ScreenGetWidth_t>(getAbsoluteAddress(kTargetLibName, kScreenGetWidthRva));
    gScreenGetHeight = reinterpret_cast<ScreenGetHeight_t>(getAbsoluteAddress(kTargetLibName, kScreenGetHeightRva));
    gTransformGetPosition_Injected = reinterpret_cast<TransformGetPosition_Injected_t>(
            getAbsoluteAddress(kTargetLibName, kTransformGetPositionInjectedRva));
}

void ResolveAimApis() {
    if (gComponentGetTransform && gTransformLookAt && gPhysicsRaycast) {
        return;
    }

    gComponentGetTransform = reinterpret_cast<ComponentGetTransform_t>(
            getAbsoluteAddress(kTargetLibName, kComponentGetTransformRva));
    gTransformLookAt = reinterpret_cast<TransformLookAt_t>(
            getAbsoluteAddress(kTargetLibName, kTransformLookAtRva));
    gPhysicsRaycast = reinterpret_cast<PhysicsRaycast_t>(
            getAbsoluteAddress(kTargetLibName, kPhysicsRaycastRva));
}

void* GetActiveCamera() {
    ResolveUnityDrawApis();

    if (gCameraGetMain) {
        if (void* camera = gCameraGetMain()) {
            return camera;
        }
    }

    if (gCameraGetCurrent) {
        return gCameraGetCurrent();
    }

    return nullptr;
}

bool TryGetEnemyScreenPosition(void* enemy, float canvasHeight, float* outX, float* outY) {
    if (!enemy || !outX || !outY || !gTransformGetPosition_Injected || !gCameraWorldToScreenPoint_Injected) {
        return false;
    }

    void* camera = GetActiveCamera();
    if (!camera) {
        return false;
    }

    auto* transform = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(enemy) + kEnemyControllerTransformOffset);
    if (!transform) {
        return false;
    }

    Vec3 worldPos{};
    gTransformGetPosition_Injected(transform, &worldPos);
    worldPos.y += 1.4f;

    Vec3 screenPos{};
    gCameraWorldToScreenPoint_Injected(camera, &worldPos, kCameraMonoEye, &screenPos);
    if (screenPos.z <= 0.01f) {
        return false;
    }

    *outX = screenPos.x;
    *outY = canvasHeight - screenPos.y;
    return true;
}

bool TryGetEnemyWorldPosition(void* enemy, Vec3* outPosition) {
    if (!enemy || !outPosition || !gTransformGetPosition_Injected) {
        return false;
    }

    auto* transform = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(enemy) + kEnemyControllerTransformOffset);
    if (!transform) {
        return false;
    }

    gTransformGetPosition_Injected(transform, outPosition);
    return true;
}

bool TryGetEnemyBonePosition(void* enemy, uintptr_t boneOffset, Vec3* outPosition) {
    if (!enemy || !outPosition || !gTransformGetPosition_Injected) {
        return false;
    }

    auto* bone = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(enemy) + boneOffset);
    if (!bone) {
        return false;
    }

    gTransformGetPosition_Injected(bone, outPosition);
    return true;
}

bool TryGetScreenSize(int* outWidth, int* outHeight);

bool TryGetEnemyHeadPosition(void* enemy, Vec3* outPosition) {
    if (!enemy || !outPosition || !gTransformGetPosition_Injected) {
        return false;
    }

    const auto addr = reinterpret_cast<uintptr_t>(enemy);

    auto* head = *reinterpret_cast<void**>(addr + kEnemyHeadTransformOffset);
    if (head) {
        gTransformGetPosition_Injected(head, outPosition);
        return true;
    }

    auto* spine = *reinterpret_cast<void**>(addr + kEnemySpine2TransformOffset);
    if (spine) {
        gTransformGetPosition_Injected(spine, outPosition);
        outPosition->y += 0.25f;
        return true;
    }

    auto* hips = *reinterpret_cast<void**>(addr + kEnemyHipsTransformOffset);
    if (hips) {
        gTransformGetPosition_Injected(hips, outPosition);
        outPosition->y += 0.5f;
        return true;
    }

    if (!TryGetEnemyWorldPosition(enemy, outPosition)) {
        return false;
    }
    outPosition->y += 1.6f;
    return true;
}

struct EnemyScreenBones {
    bool head = false;
    bool spine = false;
    bool hips = false;
    bool leftArm = false;
    bool rightArm = false;
    bool leftForeArm = false;
    bool rightForeArm = false;
    bool leftUpLeg = false;
    bool rightUpLeg = false;
    bool leftLeg = false;
    bool rightLeg = false;
    float headX = 0.0f; float headY = 0.0f;
    float spineX = 0.0f; float spineY = 0.0f;
    float hipsX = 0.0f; float hipsY = 0.0f;
    float leftArmX = 0.0f; float leftArmY = 0.0f;
    float rightArmX = 0.0f; float rightArmY = 0.0f;
    float leftForeArmX = 0.0f; float leftForeArmY = 0.0f;
    float rightForeArmX = 0.0f; float rightForeArmY = 0.0f;
    float leftUpLegX = 0.0f; float leftUpLegY = 0.0f;
    float rightUpLegX = 0.0f; float rightUpLegY = 0.0f;
    float leftLegX = 0.0f; float leftLegY = 0.0f;
    float rightLegX = 0.0f; float rightLegY = 0.0f;
};

bool IsEnemyVisibleFromCamera(const Vec3& camPos, void* enemy) {
    if (!gPhysicsRaycast) {
        return true;
    }

    Vec3 headPos{};
    if (!TryGetEnemyHeadPosition(enemy, &headPos)) {
        return false;
    }

    Vec3 dir = SubVec3(headPos, camPos);
    const float dist = std::sqrt(LengthSqVec3(dir));
    if (dist < 0.5f) {
        return true;
    }

    const float invDist = 1.0f / dist;
    dir.x *= invDist;
    dir.y *= invDist;
    dir.z *= invDist;

    constexpr int kDefaultLayer = ~(1 << 2);
    return !gPhysicsRaycast(camPos, dir, dist - 0.5f, kDefaultLayer);
}

void* FindClosestActiveEnemy() {
    const EnemyListSnapshot snapshot = ReadLocalEnemyList();
    if (snapshot.active <= 0) {
        return nullptr;
    }

    if (!gTransformGetPosition_Injected || !HasFreshLocalPlayerControl()) {
        return snapshot.enemies[0];
    }

    const auto playerAddr = reinterpret_cast<uintptr_t>(gLocalPlayerControl);
    Vec3 camPos{};
    bool hasCamPos = false;

    auto* cam = *reinterpret_cast<void**>(playerAddr + kPlayerControlCameraOffset);
    if (cam && gComponentGetTransform) {
        void* camTr = gComponentGetTransform(cam);
        if (camTr) {
            gTransformGetPosition_Injected(camTr, &camPos);
            hasCamPos = true;
        }
    }
    if (!hasCamPos) {
        return snapshot.enemies[0];
    }

    void* bestVisible = nullptr;
    float bestVisibleDist = 999999.0f;
    void* bestAny = nullptr;
    float bestAnyDist = 999999.0f;

    for (int i = 0; i < snapshot.active; ++i) {
        Vec3 pos{};
        if (!TryGetEnemyWorldPosition(snapshot.enemies[i], &pos)) {
            continue;
        }
        const Vec3 diff = SubVec3(pos, camPos);
        const float dist = LengthSqVec3(diff);

        if (dist < bestAnyDist) {
            bestAnyDist = dist;
            bestAny = snapshot.enemies[i];
        }

        if (IsEnemyVisibleFromCamera(camPos, snapshot.enemies[i])) {
            if (dist < bestVisibleDist) {
                bestVisibleDist = dist;
                bestVisible = snapshot.enemies[i];
            }
        }
    }

    return bestVisible ? bestVisible : bestAny;
}

void AimCameraAtEnemy(void* playerInstance) {
    if (!IsAimBotEnabled() || !playerInstance) {
        return;
    }

    UpdateLocalPlayerControl(playerInstance);
    if (!HasFreshLocalPlayerControl()) {
        return;
    }

    const auto playerAddr = reinterpret_cast<uintptr_t>(playerInstance);
    auto* sceneController = *reinterpret_cast<void**>(playerAddr + kPlayerControlSceneControllerOffset);
    if (!IsSceneControllerGameplayActive(sceneController, playerInstance)) {
        ClearLocalPlayerControl();
        return;
    }

    auto* cam = *reinterpret_cast<void**>(playerAddr + kPlayerControlCameraOffset);
    if (!cam) {
        return;
    }

    void* camTransform = gComponentGetTransform(cam);
    if (!camTransform) {
        return;
    }

    void* enemy = FindClosestActiveEnemy();
    if (!enemy) {
        return;
    }

    Vec3 headPos{};
    if (!TryGetEnemyHeadPosition(enemy, &headPos)) {
        return;
    }

    gTransformLookAt(camTransform, headPos);
}

bool TryGetScreenSize(int* outWidth, int* outHeight) {
    ResolveUnityDrawApis();
    if (!outWidth || !outHeight || !gScreenGetWidth || !gScreenGetHeight) {
        return false;
    }

    const int width = gScreenGetWidth();
    const int height = gScreenGetHeight();
    if (width <= 0 || height <= 0) {
        return false;
    }

    *outWidth = width;
    *outHeight = height;
    return true;
}

bool FindBestTriggerTarget(void** outEnemy) {
    if (!outEnemy) {
        return false;
    }

    *outEnemy = nullptr;

    if (!IsTriggerBotEnabled()) {
        return false;
    }

    if (!HasFreshLocalPlayerControl()) {
        return false;
    }

    int screenWidth = 0;
    int screenHeight = 0;
    if (!TryGetScreenSize(&screenWidth, &screenHeight) || screenWidth <= 0 || screenHeight <= 0) {
        return false;
    }

    const EnemyListSnapshot snapshot = ReadLocalEnemyList();
    if (snapshot.active <= 0) {
        return false;
    }

    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float centerY = static_cast<float>(screenHeight) * 0.5f;
    const float maxDistanceSq = gTriggerRadiusPixels * gTriggerRadiusPixels;
    float bestDistanceSq = maxDistanceSq;
    void* bestEnemy = nullptr;

    if (gLastTriggerTarget) {
        float lockX = 0.0f;
        float lockY = 0.0f;
        if (TryGetEnemyScreenPosition(gLastTriggerTarget, static_cast<float>(screenHeight), &lockX, &lockY)) {
            const float lockDx = lockX - centerX;
            const float lockDy = lockY - centerY;
            const float lockDistanceSq = (lockDx * lockDx) + (lockDy * lockDy);
            if (lockDistanceSq <= (maxDistanceSq * kTriggerLockBias)) {
                bestDistanceSq = lockDistanceSq;
                bestEnemy = gLastTriggerTarget;
            }
        } else {
            gLastTriggerTarget = nullptr;
        }
    }

    for (int i = 0; i < snapshot.active; ++i) {
        float targetX = 0.0f;
        float targetY = 0.0f;
        if (!TryGetEnemyScreenPosition(snapshot.enemies[i], static_cast<float>(screenHeight), &targetX, &targetY)) {
            continue;
        }

        const float dx = targetX - centerX;
        const float dy = targetY - centerY;
        const float distanceSq = (dx * dx) + (dy * dy);
        if (distanceSq > bestDistanceSq) {
            continue;
        }

        bestDistanceSq = distanceSq;
        bestEnemy = snapshot.enemies[i];
    }

    *outEnemy = bestEnemy;
    return bestEnemy != nullptr;
}

bool FireTriggerShot(void* enemy) {
    if (!enemy || !gEnemyApplyDamage || !HasFreshLocalPlayerControl()) {
        return false;
    }

    const auto playerAddr = reinterpret_cast<uintptr_t>(gLocalPlayerControl);
    auto* sceneController = *reinterpret_cast<void**>(playerAddr + kPlayerControlSceneControllerOffset);
    if (!IsSceneControllerGameplayActive(sceneController, gLocalPlayerControl)) {
        ClearLocalPlayerControl();
        return false;
    }

    const auto isActive = *reinterpret_cast<bool*>(
            reinterpret_cast<uintptr_t>(enemy) + kEnemyControllerIsActiveOffset);
    const auto isDead = *reinterpret_cast<bool*>(
            reinterpret_cast<uintptr_t>(enemy) + kEnemyControllerIsDeadOffset);
    if (!isActive || isDead) {
        return false;
    }

    if (orig_SendShootCommand && gLocalPlayerControl) {
        orig_SendShootCommand(gLocalPlayerControl);
        gLastShootCommandTimeMs = GetMonotonicTimeMs();
    }

    Vec3 force{};
    Vec3 hitPoint{};
    if (!TryGetEnemyWorldPosition(enemy, &hitPoint)) {
        return false;
    }

    hitPoint.y += 1.4f;
    force.y = 0.35f;
    force.z = 2.25f;
    gEnemyApplyDamage(enemy, ApplyDamageMultiplierIfNeeded(gTriggerDamage), 10.0f, force, nullptr, hitPoint, 0, true);
    return true;
}

bool FireDirectAutoKillDamage(void* enemy) {
    if (!enemy || !gEnemyApplyDamage || !HasFreshLocalPlayerControl()) {
        return false;
    }

    const auto playerAddr = reinterpret_cast<uintptr_t>(gLocalPlayerControl);
    auto* sceneController = *reinterpret_cast<void**>(playerAddr + kPlayerControlSceneControllerOffset);
    if (!IsSceneControllerGameplayActive(sceneController, gLocalPlayerControl)) {
        ClearLocalPlayerControl();
        return false;
    }

    const auto enemyAddr = reinterpret_cast<uintptr_t>(enemy);
    const bool isActive = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsActiveOffset);
    const bool isDead = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsDeadOffset);
    if (!isActive || isDead) {
        return false;
    }

    Vec3 force{};
    Vec3 hitPoint{};
    if (!TryGetEnemyHeadPosition(enemy, &hitPoint) && !TryGetEnemyWorldPosition(enemy, &hitPoint)) {
        return false;
    }

    force.y = 0.35f;
    force.z = 2.25f;
    gEnemyApplyDamage(
            enemy,
            ApplyDamageMultiplierIfNeeded(gTriggerDamage),
            10.0f,
            force,
            nullptr,
            hitPoint,
            kEnemyBodyPartChest,
            true);
    return true;
}

bool ForceKillEnemyNow(void* enemy) {
    if (!enemy || !HasFreshLocalPlayerControl()) {
        return false;
    }

    const auto playerAddr = reinterpret_cast<uintptr_t>(gLocalPlayerControl);
    auto* sceneController = *reinterpret_cast<void**>(playerAddr + kPlayerControlSceneControllerOffset);
    if (!IsSceneControllerGameplayActive(sceneController, gLocalPlayerControl)) {
        ClearLocalPlayerControl();
        return false;
    }

    const auto enemyAddr = reinterpret_cast<uintptr_t>(enemy);
    const bool isActive = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsActiveOffset);
    const bool isDead = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsDeadOffset);
    if (!isActive || isDead) {
        return false;
    }

    if (gEnemyKillEnemy) {
        gEnemyKillEnemy(enemy);
        return true;
    }

    return FireTriggerShot(enemy);
}

int FireKillAllBurst() {
    if (!gEnemyKillEnemy && !gEnemyApplyDamage) {
        return 0;
    }

    const EnemyListSnapshot snapshot = ReadLocalEnemyList();
    int fired = 0;
    for (int i = 0; i < snapshot.active; ++i) {
        if (ForceKillEnemyNow(snapshot.enemies[i])) {
            fired++;
            usleep(4000);
        }
    }

    return fired;
}

void ProcessPendingKillAllWork() {
    if (!HasFreshLocalPlayerControl()) {
        return;
    }

    const long long nowMs = GetMonotonicTimeMs();
    if ((nowMs - gLastAutoKillTickMs) < kAutoKillTickCooldownMs) {
        return;
    }

    if (gKillAllNowRequested) {
        gKillAllNowRequested = false;
        const int fired = FireKillAllBurst();
        gLastAutoKillTickMs = nowMs;
        __android_log_print(ANDROID_LOG_INFO, kLogTag, "Kill-all burst executado: %d alvos", fired);
        return;
    }

    if (IsAutoKillAllEnabled()) {
        void* target = nullptr;
        const EnemyListSnapshot waveSnapshot = ReadCurrentWaveEnemyList();
        if (FindBestTargetInSnapshot(waveSnapshot, &target) && FireDirectAutoKillDamage(target)) {
            gLastAutoKillTickMs = nowMs;
            gLastTriggerTarget = target;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Auto kill dano aplicado em %p", target);
        }
    }
}

const char* GetLineOriginModeName() {
    switch (gLineOriginMode) {
        case 0:
            return "Topo";
        case 1:
            return "Centro";
        case 2:
            return "Base";
        case 3:
            return "Topo esquerda";
        case 4:
            return "Topo direita";
        case 5:
            return "Base esquerda";
        case 6:
            return "Base direita";
        default:
            return "Base";
    }
}

void GetLineOriginPoint(float canvasWidth, float canvasHeight, float* outX, float* outY) {
    if (!outX || !outY) {
        return;
    }

    switch (gLineOriginMode) {
        case 0:
            *outX = canvasWidth * 0.5f;
            *outY = 40.0f;
            break;
        case 1:
            *outX = canvasWidth * 0.5f;
            *outY = canvasHeight * 0.5f;
            break;
        case 2:
            *outX = canvasWidth * 0.5f;
            *outY = canvasHeight - 80.0f;
            break;
        case 3:
            *outX = 40.0f;
            *outY = 40.0f;
            break;
        case 4:
            *outX = canvasWidth - 40.0f;
            *outY = 40.0f;
            break;
        case 5:
            *outX = 40.0f;
            *outY = canvasHeight - 80.0f;
            break;
        case 6:
            *outX = canvasWidth - 40.0f;
            *outY = canvasHeight - 80.0f;
            break;
        default:
            *outX = canvasWidth * 0.5f;
            *outY = canvasHeight - 80.0f;
            break;
    }
}

void LogInfo(const char* message) {
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "%s", message ? message : "");
}

void Hooked_ApplyDamage(void* instance,
                        float damage,
                        Vec3 directionDamage,
                        Vec3 impactPoint,
                        Vec3 hitNormal,
                        void* damageCreator,
                        int originDamage,
                        void* originObject) {
    if (instance) {
        UpdateLocalPlayerControl(instance);
    }

    if (IsInfiniteLifeEnabled()) {
        SetLocalPlayerLife999(instance);
        if (orig_ApplyDamage) {
            orig_ApplyDamage(
                    instance,
                    0.0f,
                    directionDamage,
                    impactPoint,
                    hitNormal,
                    damageCreator,
                    originDamage,
                    originObject);
        }
        SetLocalPlayerLife999(instance);
        return;
    }

    if (IsGodModeEnabled()) {
        if (orig_ApplyDamage) {
            orig_ApplyDamage(
                    instance,
                    0.0f,
                    directionDamage,
                    impactPoint,
                    hitNormal,
                    damageCreator,
                    originDamage,
                    originObject);
        }
        return;
    }

    if (orig_ApplyDamage) {
        orig_ApplyDamage(
                instance,
                damage,
                directionDamage,
                impactPoint,
                hitNormal,
                damageCreator,
                originDamage,
                originObject);
    }
}

void Hooked_ApplyDirectDamage(void* instance,
                              float damage,
                              Vec3 directionDamage,
                              Vec3 impactPoint,
                              int originDamage) {
    if (instance) {
        UpdateLocalPlayerControl(instance);
    }

    if (IsInfiniteLifeEnabled()) {
        SetLocalPlayerLife999(instance);
        if (orig_ApplyDirectDamage) {
            orig_ApplyDirectDamage(instance, 0.0f, directionDamage, impactPoint, originDamage);
        }
        SetLocalPlayerLife999(instance);
        return;
    }

    if (IsGodModeEnabled()) {
        if (orig_ApplyDirectDamage) {
            orig_ApplyDirectDamage(instance, 0.0f, directionDamage, impactPoint, originDamage);
        }
        return;
    }

    if (orig_ApplyDirectDamage) {
        orig_ApplyDirectDamage(instance, damage, directionDamage, impactPoint, originDamage);
    }
}

void Hooked_SendShootCommand(void* instance) {
    if (instance) {
        UpdateLocalPlayerControl(instance);
    }

    gLastShootCommandTimeMs = GetMonotonicTimeMs();
    ProcessPendingKillAllWork();

    if (orig_SendShootCommand) {
        orig_SendShootCommand(instance);
    }
}

void Hooked_MoveCam(void* instance) {
    if (instance) {
        UpdateLocalPlayerControl(instance);
    }

    ProcessPendingKillAllWork();

    if (orig_MoveCam) {
        orig_MoveCam(instance);
    }

    AimCameraAtEnemy(instance);
}

void Hooked_EnemyApplyDamage(void* instance,
                             float damageShoot,
                             float multiplier,
                             Vec3 force,
                             void* objetoToApplyForce,
                             Vec3 point,
                             int zonebody,
                             bool applyRotationForces) {
    if (!orig_EnemyApplyDamage) {
        return;
    }

    float finalDamage = damageShoot;
    if (IsDamageMultiplierEnabled() && IsEnemyOwnedByLocalPlayer(instance)) {
        finalDamage = ApplyDamageMultiplierIfNeeded(damageShoot);
    }

    orig_EnemyApplyDamage(
            instance,
            finalDamage,
            multiplier,
            force,
            objetoToApplyForce,
            point,
            zonebody,
            applyRotationForces);
}

void* Hooked_ActionShoot(void* instance) {
    if (instance) {
        UpdateLocalPlayerControl(instance);
    }

    ProcessPendingKillAllWork();

    AimCameraAtEnemy(instance);

    if (orig_ActionShoot) {
        return orig_ActionShoot(instance);
    }
    return nullptr;
}

void* InstallHooksThread(void*) {
    while (!isLibraryLoaded(kTargetLibName)) {
        sleep(1);
    }

    ResolveUnityDrawApis();
    ResolveAimApis();

    const auto target = reinterpret_cast<void*>(getAbsoluteAddress(kTargetLibName, kPlayerControlApplyDamageRva));
    const auto directDamageTarget = reinterpret_cast<void*>(getAbsoluteAddress(kTargetLibName, kPlayerControlApplyDirectDamageRva));
    const auto shootCommandTarget = reinterpret_cast<void*>(getAbsoluteAddress(kTargetLibName, kPlayerControlSendShootCommandRva));
    const auto actionShootTarget = reinterpret_cast<void*>(getAbsoluteAddress(kTargetLibName, kPlayerControlActionShootRva));
    const auto moveCamTarget = reinterpret_cast<void*>(getAbsoluteAddress(kTargetLibName, kPlayerControlMoveCamRva));
    const auto enemyApplyDamageTarget = reinterpret_cast<void*>(getAbsoluteAddress(kTargetLibName, kEnemyControllerApplyDamageRva));
    const auto enemyKillTarget = reinterpret_cast<void*>(getAbsoluteAddress(kTargetLibName, kEnemyControllerKillEnemyRva));
    gEnemyApplyDamage = reinterpret_cast<EnemyApplyDamage_t>(enemyApplyDamageTarget);
    gEnemyKillEnemy = reinterpret_cast<EnemyKill_t>(enemyKillTarget);

#if defined(__aarch64__)

#else
    if (target) {
        MSHookFunction(target, reinterpret_cast<void*>(Hooked_ApplyDamage), reinterpret_cast<void**>(&orig_ApplyDamage));
    }
    if (directDamageTarget) {
        MSHookFunction(
                directDamageTarget,
                reinterpret_cast<void*>(Hooked_ApplyDirectDamage),
                reinterpret_cast<void**>(&orig_ApplyDirectDamage));
    }
    if (shootCommandTarget) {
        MSHookFunction(
                shootCommandTarget,
                reinterpret_cast<void*>(Hooked_SendShootCommand),
                reinterpret_cast<void**>(&orig_SendShootCommand));
    }
    if (actionShootTarget) {
        MSHookFunction(
                actionShootTarget,
                reinterpret_cast<void*>(Hooked_ActionShoot),
                reinterpret_cast<void**>(&orig_ActionShoot));
    }
    if (moveCamTarget) {
        MSHookFunction(
                moveCamTarget,
                reinterpret_cast<void*>(Hooked_MoveCam),
                reinterpret_cast<void**>(&orig_MoveCam));
    }
    if (enemyApplyDamageTarget) {
        MSHookFunction(
                enemyApplyDamageTarget,
                reinterpret_cast<void*>(Hooked_EnemyApplyDamage),
                reinterpret_cast<void**>(&orig_EnemyApplyDamage));
    }

    if (orig_EnemyApplyDamage) {
        gEnemyApplyDamage = orig_EnemyApplyDamage;
    }

    gHookInstalled = (orig_ApplyDamage != nullptr || orig_ApplyDirectDamage != nullptr || orig_EnemyApplyDamage != nullptr);

    if (gHookInstalled) {
        LogInfo("sucess");
    } else {
        LogInfo("failed");
    }
#endif

    return nullptr;
}

void* TriggerBotThread(void*) {
    while (true) {
        if (!HasFreshLocalPlayerControl()) {
            gLastTriggerTarget = nullptr;
            usleep(kTriggerLoopDelayUs);
            continue;
        }

        if (!IsTriggerBotEnabled() || !IsTriggerWindowOpen()) {
            gLastTriggerTarget = nullptr;
            usleep(kTriggerLoopDelayUs);
            continue;
        }

        void* target = nullptr;
        if (FindBestTriggerTarget(&target)) {
            const long long nowMs = GetMonotonicTimeMs();
            const bool targetChanged = target != gLastTriggerTarget;
            const bool cooldownReady = (nowMs - gLastTriggerFireTimeMs) >= kTriggerRefireCooldownMs;
            if (targetChanged || cooldownReady) {
                int burstFired = 0;
                for (int shot = 0; shot < kTriggerBurstShots; ++shot) {
                    if (FireTriggerShot(target)) {
                        burstFired++;
                    }
                    usleep(3500);
                }
                if (burstFired > 0) {
                    gLastTriggerFireTimeMs = nowMs;
                    gLastTriggerTarget = target;
                }
            }
        } else {
            gLastTriggerTarget = nullptr;
        }

        usleep(kTriggerLoopDelayUs);
    }
}

jobjectArray NewStringArray(JNIEnv* env, const char* const* items, size_t count) {
    jclass stringClass = env->FindClass(OBFUSCATE("java/lang/String"));
    if (!stringClass) {
        return nullptr;
    }

    jstring empty = env->NewStringUTF("");
    jobjectArray result = env->NewObjectArray(static_cast<jsize>(count), stringClass, empty);
    if (empty) {
        env->DeleteLocalRef(empty);
    }
    if (!result) {
        env->DeleteLocalRef(stringClass);
        return nullptr;
    }

    for (size_t i = 0; i < count; ++i) {
        jstring item = env->NewStringUTF(items[i] ? items[i] : "");
        if (!item) {
            continue;
        }
        env->SetObjectArrayElement(result, static_cast<jsize>(i), item);
        env->DeleteLocalRef(item);
    }
    env->DeleteLocalRef(stringClass);
    return result;
}

jobject NewLauncherIntent(JNIEnv* env, jobject context) {
    jclass intentClass = env->FindClass(OBFUSCATE("android/content/Intent"));
    if (!intentClass) {
        return nullptr;
    }

    jmethodID ctor = env->GetMethodID(
            intentClass,
            OBFUSCATE("<init>"),
            OBFUSCATE("(Landroid/content/Context;Ljava/lang/Class;)V"));
    if (!ctor) {
        env->DeleteLocalRef(intentClass);
        return nullptr;
    }

    jclass launcherClass = env->FindClass(OBFUSCATE("vdev/com/android/support/Launcher"));
    if (!launcherClass) {
        env->DeleteLocalRef(intentClass);
        return nullptr;
    }

    jobject intent = env->NewObject(intentClass, ctor, context, launcherClass);
    env->DeleteLocalRef(launcherClass);
    env->DeleteLocalRef(intentClass);
    return intent;
}

bool CanDrawOverlays(JNIEnv* env, jobject context) {
    jclass settingsClass = env->FindClass(OBFUSCATE("android/provider/Settings"));
    if (!settingsClass) {
        return false;
    }

    jmethodID canDrawOverlays = env->GetStaticMethodID(
            settingsClass,
            OBFUSCATE("canDrawOverlays"),
            OBFUSCATE("(Landroid/content/Context;)Z"));
    if (!canDrawOverlays) {
        env->DeleteLocalRef(settingsClass);
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(settingsClass, canDrawOverlays, context);
    env->DeleteLocalRef(settingsClass);
    return result;
}

void StartLauncherService(JNIEnv* env, jobject context) {
    jobject intent = NewLauncherIntent(env, context);
    if (!intent) {
        LogInfo("Falha ao criar intent do Launcher");
        return;
    }

    jclass contextClass = env->GetObjectClass(context);
    if (!contextClass) {
        env->DeleteLocalRef(intent);
        return;
    }

    jmethodID startService = env->GetMethodID(
            contextClass,
            OBFUSCATE("startService"),
            OBFUSCATE("(Landroid/content/Intent;)Landroid/content/ComponentName;"));
    if (!startService) {
        env->DeleteLocalRef(contextClass);
        env->DeleteLocalRef(intent);
        return;
    }

    env->CallObjectMethod(context, startService, intent);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(intent);
}

void OpenOverlayPermissionScreen(JNIEnv* env, jobject context) {
    jclass settingsClass = env->FindClass(OBFUSCATE("android/provider/Settings"));
    jclass uriClass = env->FindClass(OBFUSCATE("android/net/Uri"));
    jclass intentClass = env->FindClass(OBFUSCATE("android/content/Intent"));
    jclass contextClass = env->GetObjectClass(context);
    if (!settingsClass || !uriClass || !intentClass || !contextClass) {
        if (settingsClass) env->DeleteLocalRef(settingsClass);
        if (uriClass) env->DeleteLocalRef(uriClass);
        if (intentClass) env->DeleteLocalRef(intentClass);
        if (contextClass) env->DeleteLocalRef(contextClass);
        return;
    }

    jfieldID actionField = env->GetStaticFieldID(
            settingsClass,
            OBFUSCATE("ACTION_MANAGE_OVERLAY_PERMISSION"),
            OBFUSCATE("Ljava/lang/String;"));
    if (!actionField) {
        env->DeleteLocalRef(settingsClass);
        env->DeleteLocalRef(uriClass);
        env->DeleteLocalRef(intentClass);
        env->DeleteLocalRef(contextClass);
        return;
    }

    jobject action = env->GetStaticObjectField(settingsClass, actionField);
    jmethodID getPackageName = env->GetMethodID(
            contextClass,
            OBFUSCATE("getPackageName"),
            OBFUSCATE("()Ljava/lang/String;"));
    jmethodID parseMethod = env->GetStaticMethodID(
            uriClass,
            OBFUSCATE("parse"),
            OBFUSCATE("(Ljava/lang/String;)Landroid/net/Uri;"));
    jmethodID intentCtor = env->GetMethodID(
            intentClass,
            OBFUSCATE("<init>"),
            OBFUSCATE("(Ljava/lang/String;Landroid/net/Uri;)V"));
    jmethodID startActivity = env->GetMethodID(
            contextClass,
            OBFUSCATE("startActivity"),
            OBFUSCATE("(Landroid/content/Intent;)V"));
    if (!action || !getPackageName || !parseMethod || !intentCtor || !startActivity) {
        if (action) {
            env->DeleteLocalRef(action);
        }
        env->DeleteLocalRef(settingsClass);
        env->DeleteLocalRef(uriClass);
        env->DeleteLocalRef(intentClass);
        env->DeleteLocalRef(contextClass);
        return;
    }

    jstring packageName = static_cast<jstring>(env->CallObjectMethod(context, getPackageName));
    const char* packageChars = packageName ? env->GetStringUTFChars(packageName, nullptr) : nullptr;

    char buffer[256] = {0};
    if (packageChars) {
        std::snprintf(buffer, sizeof(buffer), "package:%s", packageChars);
        env->ReleaseStringUTFChars(packageName, packageChars);
    } else {
        std::snprintf(buffer, sizeof(buffer), "package:");
    }

    jstring packageUriText = env->NewStringUTF(buffer);
    jobject packageUri = env->CallStaticObjectMethod(uriClass, parseMethod, packageUriText);
    jobject intent = env->NewObject(intentClass, intentCtor, action, packageUri);
    env->CallVoidMethod(context, startActivity, intent);

    if (packageName) {
        env->DeleteLocalRef(packageName);
    }
    if (packageUriText) {
        env->DeleteLocalRef(packageUriText);
    }
    if (packageUri) {
        env->DeleteLocalRef(packageUri);
    }
    if (intent) {
        env->DeleteLocalRef(intent);
    }
    env->DeleteLocalRef(action);
    env->DeleteLocalRef(settingsClass);
    env->DeleteLocalRef(uriClass);
    env->DeleteLocalRef(intentClass);
    env->DeleteLocalRef(contextClass);
}

}  // namespace

void Init(JNIEnv* env, jobject, jobject context, jobject titleView, jobject subTitleView) {
    if (!env || !context) {
        return;
    }

    RegisterDialogContext(env, context);

    jclass textViewClass = env->FindClass(OBFUSCATE("android/widget/TextView"));
    if (!textViewClass) {
        return;
    }

    jmethodID setText = env->GetMethodID(textViewClass, OBFUSCATE("setText"), OBFUSCATE("(Ljava/lang/CharSequence;)V"));
    if (!setText) {
        env->DeleteLocalRef(textViewClass);
        return;
    }

    if (titleView) {
        jstring titleText = env->NewStringUTF(kMenuTitle);
        if (titleText) {
            env->CallVoidMethod(titleView, setText, titleText);
            env->DeleteLocalRef(titleText);
        }
    }
    if (subTitleView) {
        jstring subtitleText = env->NewStringUTF(kMenuSubtitle);
        if (subtitleText) {
            env->CallVoidMethod(subTitleView, setText, subtitleText);
            env->DeleteLocalRef(subtitleText);
        }
    }
    env->DeleteLocalRef(textViewClass);
}

jstring Icon(JNIEnv* env, jobject) {
    return env->NewStringUTF(kIconBase64);
}

jstring IconWebViewData(JNIEnv* env, jobject) {
    return env->NewStringUTF(kIconWebData);
}

jboolean MenuIsGameLibLoaded(JNIEnv*, jobject) {
    return libLoaded ? JNI_TRUE : JNI_FALSE;
}

jobjectArray SettingsList(JNIEnv* env, jobject) {
    static const char* const kSettings[] = {
            OBFUSCATE("Category_Configurações"),
            OBFUSCATE("-1_Toggle_Salvar preferências"),
            OBFUSCATE("-3_Toggle_Menu expandido"),
    };
    return NewStringArray(env, kSettings, sizeof(kSettings) / sizeof(kSettings[0]));
}

jobjectArray GetFeatureList(JNIEnv* env, jobject) {
    static const char* const kFeatures[] = {
            OBFUSCATE("Category_Combate"),
            OBFUSCATE("1_Toggle_Aimbot (mira na cabeca)"),
            OBFUSCATE("2_Toggle_Trigger bot (1 tiro por disparo)"),
            OBFUSCATE("3_SeekBar_Multiplicador de dano_1_20"),
            OBFUSCATE("4_Button_Matar todos agora"),
            OBFUSCATE("5_Toggle_Auto kill continuo"),
            OBFUSCATE("6_SeekBar_Raio do trigger_30_400"),
            OBFUSCATE("7_SeekBar_Dano por tiro_50_5000"),
            OBFUSCATE("Category_Visual"),
            OBFUSCATE("8_Toggle_Draw line inimigos locais"),
            OBFUSCATE("9_Toggle_Draw esqueleto inimigos"),
            OBFUSCATE("10_Spinner_Cor do ESP_Verde,Vermelho,Azul,Ciano,Amarelo,Branco,Rosa,Laranja"),
            OBFUSCATE("11_Toggle_ESP apenas alertas"),
            OBFUSCATE("12_Toggle_ESP apenas inimigos atirando"),
            OBFUSCATE("13_Spinner_Origem da linha_Topo,Centro,Base,Topo esquerda,Topo direita,Base esquerda,Base direita"),
            OBFUSCATE("Category_Defesa"),
            OBFUSCATE("14_Toggle_God mode"),
            OBFUSCATE("15_Toggle_Vida infinita 999"),
            OBFUSCATE("Category_Util"),
            OBFUSCATE("16_Button_Status do hook ARMv7"),
    };
    return NewStringArray(env, kFeatures, sizeof(kFeatures) / sizeof(kFeatures[0]));
}

void Changes(JNIEnv* env, jclass, jobject context, jint featNum, jstring, jint value, jboolean boolean, jstring) {
    RegisterDialogContext(env, context);
    if (!IsDialogLoginValidated()) {
        ShowQueuedLibLoadDialog(env, context);
        __android_log_print(ANDROID_LOG_WARN, kLogTag, "Mudança ignorada antes do login: %d", featNum);
        return;
    }

    switch (featNum) {
        case 1:
            gAimBot = boolean;
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Aimbot %s | lookAt=%s | moveCam=%s",
                    gAimBot ? "ON" : "OFF",
                    gTransformLookAt ? "ok" : "null",
                    orig_MoveCam ? "hookeado" : "null");
            break;
        case 2:
            gTriggerBot = boolean;
            if (!gTriggerBot) {
                gLastTriggerTarget = nullptr;
            }
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Trigger bot %s | enemyApply=%p",
                    gTriggerBot ? "ON" : "OFF",
                    reinterpret_cast<void*>(gEnemyApplyDamage));
            break;
        case 3:
            gDamageMultiplier = static_cast<float>(value);
            if (gDamageMultiplier < 1.0f) {
                gDamageMultiplier = 1.0f;
            }
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Multiplicador de dano ajustado para %.1fx",
                    gDamageMultiplier);
            break;
        case 4: {
            gKillAllNowRequested = true;
            const EnemyListSnapshot snapshot = ReadLocalEnemyList();
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Kill-all solicitado | inimigos ativos=%d total=%d",
                    snapshot.active,
                    snapshot.total);
            break;
        }
        case 5:
            gAutoKillAll = boolean;
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Auto kill continuo %s",
                    gAutoKillAll ? "ON" : "OFF");
            break;
        case 6:
            gTriggerRadiusPixels = static_cast<float>(value);
            if (gTriggerRadiusPixels < 30.0f) {
                gTriggerRadiusPixels = 30.0f;
            }
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Raio trigger ajustado para %.1f px",
                    gTriggerRadiusPixels);
            break;
        case 7:
            gTriggerDamage = static_cast<float>(value);
            if (gTriggerDamage < 50.0f) {
                gTriggerDamage = 50.0f;
            }
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Dano trigger/kill ajustado para %.1f",
                    gTriggerDamage);
            break;
        case 8:
            gEnemyLines = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Draw line inimigos %s", gEnemyLines ? "ON" : "OFF");
            break;
        case 9:
            gEnemySkeleton = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Draw esqueleto inimigos %s", gEnemySkeleton ? "ON" : "OFF");
            break;
        case 10:
            gEspColorPreset = value;
            if (gEspColorPreset < 0 || gEspColorPreset > 7) {
                gEspColorPreset = 3;
            }
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Cor do ESP=%d (%s)",
                    gEspColorPreset,
                    GetEspColorPresetName());
            break;
        case 11:
            gEspOnlyAware = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "ESP apenas alertas %s", gEspOnlyAware ? "ON" : "OFF");
            break;
        case 12:
            gEspOnlyShooting = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "ESP apenas atirando %s", gEspOnlyShooting ? "ON" : "OFF");
            break;
        case 13:
            gLineOriginMode = value;
            if (gLineOriginMode < 0 || gLineOriginMode > 6) {
                gLineOriginMode = 2;
            }
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Origem da linha=%d (%s)",
                    gLineOriginMode,
                    GetLineOriginModeName());
            break;
        case 14:
            gGodMode = boolean;
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Super dano %s | hook=%s",
                    gGodMode ? "ON" : "OFF",
                    gHookInstalled ? "instalado" : "pendente");
            break;
        case 15:
            gInfiniteLife999 = boolean;
            if (IsInfiniteLifeEnabled()) {
                SetLocalPlayerLife999(gLocalPlayerControl);
            }
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Vida infinita 999 %s",
                    gInfiniteLife999 ? "ON" : "OFF");
            break;
        case 16:
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Status hook=%s | player=%p",
                    gHookInstalled ? "instalado" : "pendente",
                    gLocalPlayerControl);
            break;
        default:
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Feature sem ação dedicada: %d", featNum);
            break;
    }
}

void CheckOverlayPermission(JNIEnv* env, jclass, jobject context) {
    if (!env || !context) {
        return;
    }

    RegisterDialogContext(env, context);

    if (CanDrawOverlays(env, context)) {
        StartLauncherService(env, context);
        return;
    }

    OpenOverlayPermissionScreen(env, context);
}

void Toast(JNIEnv* env, jobject context, const char* text, int length) {
    if (!env || !context) {
        return;
    }

    jclass toastClass = env->FindClass(OBFUSCATE("android/widget/Toast"));
    if (!toastClass) {
        return;
    }

    jmethodID makeText = env->GetStaticMethodID(
            toastClass,
            OBFUSCATE("makeText"),
            OBFUSCATE("(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;"));
    jmethodID show = env->GetMethodID(toastClass, OBFUSCATE("show"), OBFUSCATE("()V"));
    if (!makeText || !show) {
        env->DeleteLocalRef(toastClass);
        return;
    }

    jstring message = env->NewStringUTF(text ? text : "");
    jobject toast = env->CallStaticObjectMethod(toastClass, makeText, context, message, static_cast<jint>(length));
    if (toast) {
        env->CallVoidMethod(toast, show);
        env->DeleteLocalRef(toast);
    }
    if (message) {
        env->DeleteLocalRef(message);
    }
    env->DeleteLocalRef(toastClass);
}

void QueueLoginSuccessHints(const char*, int) {
}

extern "C" jstring SubmitNativeLogin(JNIEnv* env, jclass, jobject context, jstring email, jstring password) {
    if (!env || !context) {
        return env ? env->NewStringUTF("Contexto invalido para login.") : nullptr;
    }

    const char* emailChars = email ? env->GetStringUTFChars(email, nullptr) : nullptr;
    const char* passwordChars = password ? env->GetStringUTFChars(password, nullptr) : nullptr;

    const char* result = SubmitJavaLogin(
            env,
            context,
            emailChars ? emailChars : "",
            passwordChars ? passwordChars : "");

    if (emailChars) {
        env->ReleaseStringUTFChars(email, emailChars);
    }
    if (passwordChars) {
        env->ReleaseStringUTFChars(password, passwordChars);
    }

    return env->NewStringUTF((result && result[0]) ? result : "");
}

extern "C" jstring GetNativeLoginSummary(JNIEnv* env, jclass) {
    if (!env) {
        return nullptr;
    }

    const char* summary = GetJavaLoginSuccessSummary();
    return env->NewStringUTF((summary && summary[0]) ? summary : "{}");
}

struct DrawCache {
    bool resolved = false;
    jclass canvasClass = nullptr;
    jclass espViewClass = nullptr;
    jmethodID espDrawText = nullptr;
    jmethodID espDrawLine = nullptr;
    jmethodID espDrawCircle = nullptr;
    jmethodID espDrawFilledCircle = nullptr;
    jmethodID getWidth = nullptr;
    jmethodID getHeight = nullptr;
    long long lastFrameMs = 0;
    long long disabledUntilMs = 0;
    int consecutiveFailures = 0;
};

static DrawCache gDraw;

bool ResolveDrawCache(JNIEnv* env) {
    if (gDraw.resolved) {
        return true;
    }

    jclass cc = env->FindClass(OBFUSCATE("android/graphics/Canvas"));
    jclass esp = env->FindClass(OBFUSCATE("vdev/com/android/support/ESPView"));
    if (!cc || !esp) {
        if (cc) {
            env->DeleteLocalRef(cc);
        }
        if (esp) {
            env->DeleteLocalRef(esp);
        }
        return false;
    }

    gDraw.canvasClass = reinterpret_cast<jclass>(env->NewGlobalRef(cc));
    gDraw.espViewClass = reinterpret_cast<jclass>(env->NewGlobalRef(esp));
    gDraw.espDrawText = env->GetMethodID(
            esp,
            OBFUSCATE("drawText"),
            OBFUSCATE("(Landroid/graphics/Canvas;IIIILjava/lang/String;FFF)V"));
    gDraw.espDrawLine = env->GetMethodID(
            esp,
            OBFUSCATE("drawLine"),
            OBFUSCATE("(Landroid/graphics/Canvas;IIIIFFFFF)V"));
    gDraw.espDrawCircle = env->GetMethodID(
            esp,
            OBFUSCATE("drawCircle"),
            OBFUSCATE("(Landroid/graphics/Canvas;IIIIFFFF)V"));
    gDraw.espDrawFilledCircle = env->GetMethodID(
            esp,
            OBFUSCATE("drawFilledCircle"),
            OBFUSCATE("(Landroid/graphics/Canvas;IIIIFFF)V"));
    gDraw.getWidth = env->GetMethodID(cc, OBFUSCATE("getWidth"), OBFUSCATE("()I"));
    gDraw.getHeight = env->GetMethodID(cc, OBFUSCATE("getHeight"), OBFUSCATE("()I"));

    if (!gDraw.espDrawText || !gDraw.espDrawLine || !gDraw.espDrawCircle || !gDraw.espDrawFilledCircle ||
        !gDraw.getWidth || !gDraw.getHeight) {
        env->DeleteLocalRef(cc);
        env->DeleteLocalRef(esp);
        return false;
    }

    env->DeleteLocalRef(cc);
    env->DeleteLocalRef(esp);

    gDraw.resolved = true;
    return true;
}

bool ClearJniException(JNIEnv* env, const char* stage) {
    if (!env || !env->ExceptionCheck()) {
        return false;
    }

    env->ExceptionClear();
    __android_log_print(
            ANDROID_LOG_WARN,
            kLogTag,
            "JNI exception durante %s; overlay pausado",
            stage ? stage : "draw");
    return true;
}

void RegisterDrawFailure(JNIEnv* env, const char* stage) {
    ClearJniException(env, stage);
    gDraw.consecutiveFailures = std::min(gDraw.consecutiveFailures + 1, 8);
    gDraw.disabledUntilMs = GetMonotonicTimeMs() + kDrawFailureBackoffMs;
}

void RegisterDrawSuccess() {
    gDraw.consecutiveFailures = 0;
}

bool ShouldSkipDrawFrame() {
    const long long nowMs = GetMonotonicTimeMs();
    if (nowMs < gDraw.disabledUntilMs) {
        return true;
    }

    if ((nowMs - gDraw.lastFrameMs) < kDrawMinFrameIntervalMs) {
        return true;
    }

    gDraw.lastFrameMs = nowMs;
    return false;
}

bool TryProjectWorldToCanvas(void* camera, const Vec3& worldPos, float canvasHeight, float* outX, float* outY) {
    if (!camera || !outX || !outY || !gCameraWorldToScreenPoint_Injected) {
        return false;
    }

    Vec3 worldCopy = worldPos;
    Vec3 screenPos{};
    gCameraWorldToScreenPoint_Injected(camera, &worldCopy, kCameraMonoEye, &screenPos);
    if (screenPos.z <= 0.01f) {
        return false;
    }

    *outX = screenPos.x;
    *outY = canvasHeight - screenPos.y;
    return true;
}

bool TryGetEnemyScreenPositionWithCamera(void* camera, void* enemy, float canvasHeight, float* outX, float* outY) {
    if (!enemy || !outX || !outY || !gTransformGetPosition_Injected) {
        return false;
    }

    auto* transform = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(enemy) + kEnemyControllerTransformOffset);
    if (!transform) {
        return false;
    }

    Vec3 worldPos{};
    gTransformGetPosition_Injected(transform, &worldPos);
    worldPos.y += 1.4f;
    return TryProjectWorldToCanvas(camera, worldPos, canvasHeight, outX, outY);
}

bool TryProjectEnemyBone(void* camera,
                         void* enemy,
                         uintptr_t boneOffset,
                         float canvasHeight,
                         float* outX,
                         float* outY) {
    Vec3 bonePos{};
    if (!TryGetEnemyBonePosition(enemy, boneOffset, &bonePos)) {
        return false;
    }

    return TryProjectWorldToCanvas(camera, bonePos, canvasHeight, outX, outY);
}

EnemyScreenBones ReadEnemyScreenBones(void* camera, void* enemy, float canvasHeight) {
    EnemyScreenBones bones;
    bones.head = TryProjectEnemyBone(camera, enemy, kEnemyHeadTransformOffset, canvasHeight, &bones.headX, &bones.headY);
    bones.spine = TryProjectEnemyBone(camera, enemy, kEnemySpine2TransformOffset, canvasHeight, &bones.spineX, &bones.spineY);
    bones.hips = TryProjectEnemyBone(camera, enemy, kEnemyHipsTransformOffset, canvasHeight, &bones.hipsX, &bones.hipsY);
    bones.leftArm = TryProjectEnemyBone(camera, enemy, kEnemyLeftArmTransformOffset, canvasHeight, &bones.leftArmX, &bones.leftArmY);
    bones.rightArm = TryProjectEnemyBone(camera, enemy, kEnemyRightArmTransformOffset, canvasHeight, &bones.rightArmX, &bones.rightArmY);
    bones.leftForeArm = TryProjectEnemyBone(camera, enemy, kEnemyLeftForeArmTransformOffset, canvasHeight, &bones.leftForeArmX, &bones.leftForeArmY);
    bones.rightForeArm = TryProjectEnemyBone(camera, enemy, kEnemyRightForeArmTransformOffset, canvasHeight, &bones.rightForeArmX, &bones.rightForeArmY);
    bones.leftUpLeg = TryProjectEnemyBone(camera, enemy, kEnemyLeftUpLegTransformOffset, canvasHeight, &bones.leftUpLegX, &bones.leftUpLegY);
    bones.rightUpLeg = TryProjectEnemyBone(camera, enemy, kEnemyRightUpLegTransformOffset, canvasHeight, &bones.rightUpLegX, &bones.rightUpLegY);
    bones.leftLeg = TryProjectEnemyBone(camera, enemy, kEnemyLeftLegTransformOffset, canvasHeight, &bones.leftLegX, &bones.leftLegY);
    bones.rightLeg = TryProjectEnemyBone(camera, enemy, kEnemyRightLegTransformOffset, canvasHeight, &bones.rightLegX, &bones.rightLegY);
    return bones;
}

void DrawEspSegment(JNIEnv* env,
                    jobject thiz,
                    jobject canvas,
                    int a,
                    int r,
                    int g,
                    int b,
                    float stroke,
                    bool fromOk,
                    float fromX,
                    float fromY,
                    bool toOk,
                    float toX,
                    float toY) {
    if (!fromOk || !toOk) {
        return;
    }

    env->CallVoidMethod(
            thiz,
            gDraw.espDrawLine,
            canvas,
            static_cast<jint>(a),
            static_cast<jint>(r),
            static_cast<jint>(g),
            static_cast<jint>(b),
            stroke,
            fromX,
            fromY,
            toX,
            toY);
}

void DrawOn(JNIEnv* env, jobject thiz, jobject canvas) {
    if (!env || !thiz || !canvas || !IsAnyVisualEspEnabled()) {
        return;
    }

    if (ShouldSkipDrawFrame()) {
        return;
    }

    if (!ResolveDrawCache(env)) {
        return;
    }

    const auto canvasWidth = static_cast<float>(env->CallIntMethod(canvas, gDraw.getWidth));
    const auto canvasHeight = static_cast<float>(env->CallIntMethod(canvas, gDraw.getHeight));
    if (ClearJniException(env, "obter tamanho do canvas")) {
        RegisterDrawFailure(env, "obter tamanho do canvas");
        return;
    }
    if (canvasWidth <= 0.0f || canvasHeight <= 0.0f) {
        return;
    }

    const EnemyListSnapshot snapshot = ReadLocalEnemyList();
    if (snapshot.active <= 0) {
        RegisterDrawSuccess();
        return;
    }

    void* camera = GetActiveCamera();
    if (!camera) {
        RegisterDrawSuccess();
        return;
    }

    char statusBuf[120];
    std::snprintf(statusBuf, sizeof(statusBuf), "enemies: %d/%d | %s",
            snapshot.active, snapshot.total, GetLineOriginModeName());
    jstring statusText = env->NewStringUTF(statusBuf);
    if (!statusText) {
        RegisterDrawFailure(env, "criar texto de status");
        return;
    }
    int baseR = 72;
    int baseG = 200;
    int baseB = 255;
    GetEspPresetColor(&baseR, &baseG, &baseB);
    env->CallVoidMethod(
            thiz,
            gDraw.espDrawText,
            canvas,
            static_cast<jint>(235),
            static_cast<jint>(baseR),
            static_cast<jint>(baseG),
            static_cast<jint>(baseB),
            statusText,
            120.0f,
            120.0f,
            20.0f);
    env->DeleteLocalRef(statusText);
    if (ClearJniException(env, "desenhar status")) {
        RegisterDrawFailure(env, "desenhar status");
        return;
    }

    float startX = 0.0f;
    float startY = 0.0f;
    GetLineOriginPoint(canvasWidth, canvasHeight, &startX, &startY);

    for (int i = 0; i < snapshot.active; ++i) {
        float tx = 0.0f;
        float ty = 0.0f;
        if (!TryGetEnemyScreenPositionWithCamera(camera, snapshot.enemies[i], canvasHeight, &tx, &ty)) {
            continue;
        }
        if (tx < 0.0f || tx > canvasWidth || ty < 0.0f || ty > canvasHeight) {
            continue;
        }
        const auto enemyAddr = reinterpret_cast<uintptr_t>(snapshot.enemies[i]);
        const bool isAware = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsAwareOffset);
        const bool isShooting = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsShootingOffset);
        if (gEspOnlyAware && !isAware) {
            continue;
        }
        if (gEspOnlyShooting && !isShooting) {
            continue;
        }

        int lineR = baseR;
        int lineG = baseG;
        int lineB = baseB;
        if (isShooting) {
            lineR = 255;
            lineG = 96;
            lineB = 96;
        } else if (isAware) {
            lineR = std::min(255, baseR + 35);
            lineG = std::min(255, baseG + 15);
            lineB = std::max(0, baseB - 25);
        }

        if (IsEnemyLineEspEnabled()) {
            env->CallVoidMethod(
                    thiz,
                    gDraw.espDrawLine,
                    canvas,
                    static_cast<jint>(210),
                    static_cast<jint>(lineR),
                    static_cast<jint>(lineG),
                    static_cast<jint>(lineB),
                    2.2f,
                    startX,
                    startY,
                    tx,
                    ty);
            env->CallVoidMethod(
                    thiz,
                    gDraw.espDrawFilledCircle,
                    canvas,
                    static_cast<jint>(170),
                    static_cast<jint>(72),
                    static_cast<jint>(200),
                    static_cast<jint>(255),
                    tx,
                    ty,
                    4.5f);
            env->CallVoidMethod(
                    thiz,
                    gDraw.espDrawCircle,
                    canvas,
                    static_cast<jint>(235),
                    static_cast<jint>(255),
                    static_cast<jint>(255),
                    static_cast<jint>(255),
                    1.5f,
                    tx,
                    ty,
                    7.0f);
        }

        if (IsEnemySkeletonEspEnabled()) {
            const EnemyScreenBones bones = ReadEnemyScreenBones(camera, snapshot.enemies[i], canvasHeight);
            DrawEspSegment(env, thiz, canvas, 220, lineR, lineG, lineB, 1.6f,
                           bones.head, bones.headX, bones.headY,
                           bones.spine, bones.spineX, bones.spineY);
            DrawEspSegment(env, thiz, canvas, 220, lineR, lineG, lineB, 1.6f,
                           bones.spine, bones.spineX, bones.spineY,
                           bones.hips, bones.hipsX, bones.hipsY);
            DrawEspSegment(env, thiz, canvas, 210, lineR, lineG, lineB, 1.4f,
                           bones.spine, bones.spineX, bones.spineY,
                           bones.leftArm, bones.leftArmX, bones.leftArmY);
            DrawEspSegment(env, thiz, canvas, 210, lineR, lineG, lineB, 1.4f,
                           bones.leftArm, bones.leftArmX, bones.leftArmY,
                           bones.leftForeArm, bones.leftForeArmX, bones.leftForeArmY);
            DrawEspSegment(env, thiz, canvas, 210, lineR, lineG, lineB, 1.4f,
                           bones.spine, bones.spineX, bones.spineY,
                           bones.rightArm, bones.rightArmX, bones.rightArmY);
            DrawEspSegment(env, thiz, canvas, 210, lineR, lineG, lineB, 1.4f,
                           bones.rightArm, bones.rightArmX, bones.rightArmY,
                           bones.rightForeArm, bones.rightForeArmX, bones.rightForeArmY);
            DrawEspSegment(env, thiz, canvas, 200, lineR, lineG, lineB, 1.4f,
                           bones.hips, bones.hipsX, bones.hipsY,
                           bones.leftUpLeg, bones.leftUpLegX, bones.leftUpLegY);
            DrawEspSegment(env, thiz, canvas, 200, lineR, lineG, lineB, 1.4f,
                           bones.leftUpLeg, bones.leftUpLegX, bones.leftUpLegY,
                           bones.leftLeg, bones.leftLegX, bones.leftLegY);
            DrawEspSegment(env, thiz, canvas, 200, lineR, lineG, lineB, 1.4f,
                           bones.hips, bones.hipsX, bones.hipsY,
                           bones.rightUpLeg, bones.rightUpLegX, bones.rightUpLegY);
            DrawEspSegment(env, thiz, canvas, 200, lineR, lineG, lineB, 1.4f,
                           bones.rightUpLeg, bones.rightUpLegX, bones.rightUpLegY,
                           bones.rightLeg, bones.rightLegX, bones.rightLegY);
        }

        if (ClearJniException(env, "desenhar inimigo")) {
            RegisterDrawFailure(env, "desenhar inimigo");
            return;
        }
    }

    RegisterDrawSuccess();
}

int RegisterMenu(JNIEnv* env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("Icon"), OBFUSCATE("()Ljava/lang/String;"), reinterpret_cast<void*>(Icon)},
            {OBFUSCATE("IconWebViewData"), OBFUSCATE("()Ljava/lang/String;"), reinterpret_cast<void*>(IconWebViewData)},
            {OBFUSCATE("IsGameLibLoaded"), OBFUSCATE("()Z"), reinterpret_cast<void*>(MenuIsGameLibLoaded)},
            {OBFUSCATE("Init"), OBFUSCATE("(Landroid/content/Context;Landroid/widget/TextView;Landroid/widget/TextView;)V"), reinterpret_cast<void*>(Init)},
            {OBFUSCATE("SettingsList"), OBFUSCATE("()[Ljava/lang/String;"), reinterpret_cast<void*>(SettingsList)},
            {OBFUSCATE("GetFeatureList"), OBFUSCATE("()[Ljava/lang/String;"), reinterpret_cast<void*>(GetFeatureList)},
    };

    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/Menu"));
    if (!clazz) {
        return JNI_ERR;
    }
    const int result = env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) == 0 ? JNI_OK : JNI_ERR;
    env->DeleteLocalRef(clazz);
    return result;
}

int RegisterPreferences(JNIEnv* env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("Changes"),
             OBFUSCATE("(Landroid/content/Context;ILjava/lang/String;IZLjava/lang/String;)V"),
             reinterpret_cast<void*>(Changes)},
    };

    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/Preferences"));
    if (!clazz) {
        return JNI_ERR;
    }
    const int result = env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) == 0 ? JNI_OK : JNI_ERR;
    env->DeleteLocalRef(clazz);
    return result;
}

int RegisterMain(JNIEnv* env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("CheckOverlayPermission"),
             OBFUSCATE("(Landroid/content/Context;)V"),
             reinterpret_cast<void*>(CheckOverlayPermission)},
    };

    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/Main"));
    if (!clazz) {
        return JNI_ERR;
    }
    const int result = env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) == 0 ? JNI_OK : JNI_ERR;
    env->DeleteLocalRef(clazz);
    return result;
}

int RegisterMainActivity(JNIEnv* env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("SubmitNativeLogin"),
             OBFUSCATE("(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"),
             reinterpret_cast<void*>(SubmitNativeLogin)},
            {OBFUSCATE("GetNativeLoginSummary"),
             OBFUSCATE("()Ljava/lang/String;"),
             reinterpret_cast<void*>(GetNativeLoginSummary)},
    };

    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/MainActivity"));
    if (!clazz) {
        return JNI_ERR;
    }
    const int result = env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) == 0 ? JNI_OK : JNI_ERR;
    env->DeleteLocalRef(clazz);
    return result;
}

int RegisterESPView(JNIEnv* env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("DrawOn"), OBFUSCATE("(Landroid/graphics/Canvas;)V"), reinterpret_cast<void*>(DrawOn)},
    };

    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/ESPView"));
    if (!clazz) {
        return JNI_ERR;
    }
    const int result = env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) == 0 ? JNI_OK : JNI_ERR;
    env->DeleteLocalRef(clazz);
    return result;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv* env = nullptr;
    if (!vm || vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env) {
        return JNI_ERR;
    }

    LogInfo("Carregando JNI com hook ARMv7 de ApplyDamage");

    if (RegisterMenu(env) != JNI_OK) {
        return JNI_ERR;
    }
    if (RegisterPreferences(env) != JNI_OK) {
        return JNI_ERR;
    }
    if (RegisterMain(env) != JNI_OK) {
        return JNI_ERR;
    }
    if (RegisterMainActivity(env) != JNI_OK) {
        return JNI_ERR;
    }
    if (RegisterESPView(env) != JNI_OK) {
        return JNI_ERR;
    }

    pthread_t hookThread{};
    if (pthread_create(&hookThread, nullptr, InstallHooksThread, nullptr) == 0) {
        pthread_detach(hookThread);
    } else {
        LogInfo("Falha ao criar thread de hooks");
    }

    pthread_t triggerThread{};
    if (pthread_create(&triggerThread, nullptr, TriggerBotThread, nullptr) == 0) {
        pthread_detach(triggerThread);
    } else {
        LogInfo("Falha ao criar thread do trigger bot");
    }

    return JNI_VERSION_1_6;
}
