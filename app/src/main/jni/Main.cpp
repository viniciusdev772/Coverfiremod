#include <jni.h>

#include <android/log.h>
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
constexpr uintptr_t kPlayerControlSceneControllerOffset = 0x24;
constexpr uintptr_t kPlayerControlInitialLifeOffset = 0x6C;
constexpr uintptr_t kPlayerControlActualLifeOffset = 0x70;
constexpr uintptr_t kPlayerControlIsDeadOffset = 0x524;
constexpr uintptr_t kPlayerControlCameraOffset = 0x1E4;
constexpr uintptr_t kPlayerControlMoveCamRva = 0xBE15C0;
constexpr uintptr_t kSceneControllerMapEnemiesOffset = 0x58;
constexpr uintptr_t kEnemyControllerIsActiveOffset = 0x104;
constexpr uintptr_t kEnemyControllerIsDeadOffset = 0x106;
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
constexpr size_t kManagedPointerSize = sizeof(void*);
constexpr int kMaxEnemyLinesToDraw = 24;
constexpr int kCameraMonoEye = 2;

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
ComponentGetTransform_t gComponentGetTransform = nullptr;
TransformLookAt_t gTransformLookAt = nullptr;
PhysicsRaycast_t gPhysicsRaycast = nullptr;
MoveCam_t orig_MoveCam = nullptr;
ActionShoot_t orig_ActionShoot = nullptr;
bool gGodMode = false;
bool gInfiniteLife999 = false;
bool gHookInstalled = false;
bool gEnemyLines = false;
bool gTriggerBot = false;
bool gAutoKillAll = false;
bool gKillAllNowRequested = false;
bool gAimBot = false;
int gLineOriginMode = 2;
void* gLocalPlayerControl = nullptr;
void* gLastTriggerTarget = nullptr;
float gTriggerRadiusPixels = 90.0f;
float gTriggerDamage = 200.0f;
constexpr useconds_t kTriggerLoopDelayUs = 30000;
constexpr long long kTriggerWindowMs = 260;
constexpr long long kTriggerRefireCooldownMs = 150;
constexpr float kTriggerLockBias = 1.35f;
constexpr int kTriggerBurstShots = 1;
long long gLastShootCommandTimeMs = 0;
long long gLastTriggerFireTimeMs = 0;

struct EnemyListSnapshot {
    int total = 0;
    int active = 0;
    void* enemies[kMaxEnemyLinesToDraw] = {};
};

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

bool IsTriggerBotEnabled() {
    return gHookInstalled && gTriggerBot && gEnemyApplyDamage;
}

bool IsAutoKillAllEnabled() {
    return gHookInstalled && gAutoKillAll && gEnemyApplyDamage;
}

bool IsAimBotEnabled() {
    return gAimBot && gLocalPlayerControl && gComponentGetTransform && gTransformLookAt;
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

    auto* player = reinterpret_cast<uintptr_t*>(gLocalPlayerControl);
    if (!player) {
        return snapshot;
    }

    auto* sceneController = *reinterpret_cast<uintptr_t**>(
            reinterpret_cast<uintptr_t>(player) + kPlayerControlSceneControllerOffset);
    if (!sceneController) {
        return snapshot;
    }

    auto* enemyList = *reinterpret_cast<uintptr_t**>(
            reinterpret_cast<uintptr_t>(sceneController) + kSceneControllerMapEnemiesOffset);
    if (!enemyList) {
        return snapshot;
    }

    auto* items = *reinterpret_cast<uintptr_t**>(reinterpret_cast<uintptr_t>(enemyList) + kManagedListItemsOffset);
    const int size = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(enemyList) + kManagedListSizeOffset);
    if (!items || size <= 0) {
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

    if (!gTransformGetPosition_Injected || !gLocalPlayerControl) {
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

    gLocalPlayerControl = playerInstance;
    const auto playerAddr = reinterpret_cast<uintptr_t>(playerInstance);

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
    if (!enemy || !gEnemyApplyDamage) {
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
    gEnemyApplyDamage(enemy, gTriggerDamage, 10.0f, force, nullptr, hitPoint, 0, true);
    return true;
}

int FireKillAllBurst() {
    if (!gEnemyApplyDamage) {
        return 0;
    }

    const EnemyListSnapshot snapshot = ReadLocalEnemyList();
    int fired = 0;
    for (int i = 0; i < snapshot.active; ++i) {
        if (FireTriggerShot(snapshot.enemies[i])) {
            fired++;
            usleep(2500);
        }
    }

    return fired;
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
        gLocalPlayerControl = instance;
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
        gLocalPlayerControl = instance;
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
        gLocalPlayerControl = instance;
    }

    gLastShootCommandTimeMs = GetMonotonicTimeMs();

    if (orig_SendShootCommand) {
        orig_SendShootCommand(instance);
    }
}

void Hooked_MoveCam(void* instance) {
    if (instance) {
        gLocalPlayerControl = instance;
    }

    if (orig_MoveCam) {
        orig_MoveCam(instance);
    }

    AimCameraAtEnemy(instance);
}

void* Hooked_ActionShoot(void* instance) {
    if (instance) {
        gLocalPlayerControl = instance;
    }

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
    gEnemyApplyDamage = reinterpret_cast<EnemyApplyDamage_t>(getAbsoluteAddress(kTargetLibName, kEnemyControllerApplyDamageRva));

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

    gHookInstalled = (orig_ApplyDamage != nullptr || orig_ApplyDirectDamage != nullptr);

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
        if (gKillAllNowRequested) {
            gKillAllNowRequested = false;
            const int fired = FireKillAllBurst();
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Kill-all burst executado: %d alvos", fired);
        }

        if (IsAutoKillAllEnabled()) {
            FireKillAllBurst();
            usleep(120000);
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

    jobjectArray result = env->NewObjectArray(static_cast<jsize>(count), stringClass, env->NewStringUTF(""));
    if (!result) {
        return nullptr;
    }

    for (size_t i = 0; i < count; ++i) {
        env->SetObjectArrayElement(result, static_cast<jsize>(i), env->NewStringUTF(items[i] ? items[i] : ""));
    }
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
        return nullptr;
    }

    jclass launcherClass = env->FindClass(OBFUSCATE("vdev/com/android/support/Launcher"));
    if (!launcherClass) {
        return nullptr;
    }

    return env->NewObject(intentClass, ctor, context, launcherClass);
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
        return false;
    }

    return env->CallStaticBooleanMethod(settingsClass, canDrawOverlays, context);
}

void StartLauncherService(JNIEnv* env, jobject context) {
    jobject intent = NewLauncherIntent(env, context);
    if (!intent) {
        LogInfo("Falha ao criar intent do Launcher");
        return;
    }

    jclass contextClass = env->GetObjectClass(context);
    if (!contextClass) {
        return;
    }

    jmethodID startService = env->GetMethodID(
            contextClass,
            OBFUSCATE("startService"),
            OBFUSCATE("(Landroid/content/Intent;)Landroid/content/ComponentName;"));
    if (!startService) {
        return;
    }

    env->CallObjectMethod(context, startService, intent);
}

void OpenOverlayPermissionScreen(JNIEnv* env, jobject context) {
    jclass settingsClass = env->FindClass(OBFUSCATE("android/provider/Settings"));
    jclass uriClass = env->FindClass(OBFUSCATE("android/net/Uri"));
    jclass intentClass = env->FindClass(OBFUSCATE("android/content/Intent"));
    jclass contextClass = env->GetObjectClass(context);
    if (!settingsClass || !uriClass || !intentClass || !contextClass) {
        return;
    }

    jfieldID actionField = env->GetStaticFieldID(
            settingsClass,
            OBFUSCATE("ACTION_MANAGE_OVERLAY_PERMISSION"),
            OBFUSCATE("Ljava/lang/String;"));
    if (!actionField) {
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
        return;
    }

    if (titleView) {
        env->CallVoidMethod(titleView, setText, env->NewStringUTF(kMenuTitle));
    }
    if (subTitleView) {
        env->CallVoidMethod(subTitleView, setText, env->NewStringUTF(kMenuSubtitle));
    }
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
            OBFUSCATE("1_Button_Status do hook ARMv7"),
            OBFUSCATE("2_Toggle_God mode"),
            OBFUSCATE("3_Toggle_Draw line inimigos locais"),
            OBFUSCATE("4_Spinner_Origem da linha_Topo,Centro,Base,Topo esquerda,Topo direita,Base esquerda,Base direita"),
            OBFUSCATE("5_Toggle_Vida infinita 999"),
            OBFUSCATE("6_Toggle_Trigger bot (1 tiro por disparo)"),
            OBFUSCATE("7_Button_Matar todos agora"),
            OBFUSCATE("8_Toggle_Auto kill continuo"),
            OBFUSCATE("9_SeekBar_Raio do trigger_30_400"),
            OBFUSCATE("10_SeekBar_Dano por tiro_50_5000"),
            OBFUSCATE("11_Toggle_Aimbot (mira na cabeca)"),
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
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Status hook=%s | player=%p",
                    gHookInstalled ? "instalado" : "pendente",
                    gLocalPlayerControl);
            break;
        case 2:
            gGodMode = boolean;
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Super dano %s | hook=%s",
                    gGodMode ? "ON" : "OFF",
                    gHookInstalled ? "instalado" : "pendente");
            break;
        case 3:
            gEnemyLines = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Draw line inimigos %s", gEnemyLines ? "ON" : "OFF");
            break;
        case 4:
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
        case 5:
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
        case 6:
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
        case 7: {
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
        case 8:
            gAutoKillAll = boolean;
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Auto kill continuo %s",
                    gAutoKillAll ? "ON" : "OFF");
            break;
        case 9:
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
        case 10:
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
        case 11:
            gAimBot = boolean;
            __android_log_print(
                    ANDROID_LOG_INFO,
                    kLogTag,
                    "Aimbot %s | lookAt=%s | moveCam=%s",
                    gAimBot ? "ON" : "OFF",
                    gTransformLookAt ? "ok" : "null",
                    orig_MoveCam ? "hookeado" : "null");
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
        return;
    }

    jstring message = env->NewStringUTF(text ? text : "");
    jobject toast = env->CallStaticObjectMethod(toastClass, makeText, context, message, static_cast<jint>(length));
    if (toast) {
        env->CallVoidMethod(toast, show);
    }
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
    jclass paintClass = nullptr;
    jmethodID paintCtor = nullptr;
    jmethodID setColor = nullptr;
    jmethodID setStrokeWidth = nullptr;
    jmethodID setTextSize = nullptr;
    jmethodID setAntiAlias = nullptr;
    jmethodID drawText = nullptr;
    jmethodID drawLine = nullptr;
    jmethodID drawCircle = nullptr;
    jmethodID getWidth = nullptr;
    jmethodID getHeight = nullptr;
    jint red = 0;
    jint cyan = 0;
    jint white = 0;
};

static DrawCache gDraw;

bool ResolveDrawCache(JNIEnv* env) {
    if (gDraw.resolved) {
        return true;
    }

    jclass cc = env->FindClass(OBFUSCATE("android/graphics/Canvas"));
    jclass pc = env->FindClass(OBFUSCATE("android/graphics/Paint"));
    jclass clr = env->FindClass(OBFUSCATE("android/graphics/Color"));
    if (!cc || !pc || !clr) {
        return false;
    }

    gDraw.canvasClass = reinterpret_cast<jclass>(env->NewGlobalRef(cc));
    gDraw.paintClass = reinterpret_cast<jclass>(env->NewGlobalRef(pc));

    gDraw.paintCtor = env->GetMethodID(pc, OBFUSCATE("<init>"), OBFUSCATE("()V"));
    gDraw.setColor = env->GetMethodID(pc, OBFUSCATE("setColor"), OBFUSCATE("(I)V"));
    gDraw.setStrokeWidth = env->GetMethodID(pc, OBFUSCATE("setStrokeWidth"), OBFUSCATE("(F)V"));
    gDraw.setTextSize = env->GetMethodID(pc, OBFUSCATE("setTextSize"), OBFUSCATE("(F)V"));
    gDraw.setAntiAlias = env->GetMethodID(pc, OBFUSCATE("setAntiAlias"), OBFUSCATE("(Z)V"));
    gDraw.drawText = env->GetMethodID(cc, OBFUSCATE("drawText"),
            OBFUSCATE("(Ljava/lang/String;FFLandroid/graphics/Paint;)V"));
    gDraw.drawLine = env->GetMethodID(cc, OBFUSCATE("drawLine"),
            OBFUSCATE("(FFFFLandroid/graphics/Paint;)V"));
    gDraw.drawCircle = env->GetMethodID(cc, OBFUSCATE("drawCircle"),
            OBFUSCATE("(FFFLandroid/graphics/Paint;)V"));
    gDraw.getWidth = env->GetMethodID(cc, OBFUSCATE("getWidth"), OBFUSCATE("()I"));
    gDraw.getHeight = env->GetMethodID(cc, OBFUSCATE("getHeight"), OBFUSCATE("()I"));

    if (!gDraw.paintCtor || !gDraw.setColor || !gDraw.setStrokeWidth || !gDraw.drawLine ||
        !gDraw.drawCircle || !gDraw.getWidth || !gDraw.getHeight || !gDraw.drawText) {
        return false;
    }

    jmethodID rgbMethod = env->GetStaticMethodID(clr, OBFUSCATE("rgb"), OBFUSCATE("(III)I"));
    if (rgbMethod) {
        gDraw.red = env->CallStaticIntMethod(clr, rgbMethod, 255, 96, 96);
        gDraw.cyan = env->CallStaticIntMethod(clr, rgbMethod, 72, 200, 255);
        gDraw.white = env->CallStaticIntMethod(clr, rgbMethod, 245, 245, 245);
    }

    gDraw.resolved = true;
    return true;
}

void DrawOn(JNIEnv* env, jobject, jobject canvas) {
    if (!env || !canvas || !IsEnemyLineEspEnabled()) {
        return;
    }

    if (!ResolveDrawCache(env)) {
        return;
    }

    const auto canvasWidth = static_cast<float>(env->CallIntMethod(canvas, gDraw.getWidth));
    const auto canvasHeight = static_cast<float>(env->CallIntMethod(canvas, gDraw.getHeight));
    if (canvasWidth <= 0.0f || canvasHeight <= 0.0f) {
        return;
    }

    const EnemyListSnapshot snapshot = ReadLocalEnemyList();

    jobject paint = env->NewObject(gDraw.paintClass, gDraw.paintCtor);
    if (!paint) {
        return;
    }

    env->CallVoidMethod(paint, gDraw.setAntiAlias, JNI_TRUE);

    char statusBuf[120];
    std::snprintf(statusBuf, sizeof(statusBuf), "enemies: %d/%d | %s",
            snapshot.active, snapshot.total, GetLineOriginModeName());
    jstring statusText = env->NewStringUTF(statusBuf);
    env->CallVoidMethod(paint, gDraw.setColor, gDraw.white);
    env->CallVoidMethod(paint, gDraw.setTextSize, 26.0f);
    env->CallVoidMethod(canvas, gDraw.drawText, statusText, 60.0f, 340.0f, paint);
    env->DeleteLocalRef(statusText);

    if (snapshot.active > 0) {
        float startX = 0.0f, startY = 0.0f;
        GetLineOriginPoint(canvasWidth, canvasHeight, &startX, &startY);

        env->CallVoidMethod(paint, gDraw.setStrokeWidth, 2.0f);

        float positions[kMaxEnemyLinesToDraw * 2];
        int visibleCount = 0;

        for (int i = 0; i < snapshot.active; ++i) {
            float tx = 0.0f, ty = 0.0f;
            if (!TryGetEnemyScreenPosition(snapshot.enemies[i], canvasHeight, &tx, &ty)) {
                continue;
            }
            if (tx < 0.0f || tx > canvasWidth || ty < 0.0f || ty > canvasHeight) {
                continue;
            }
            positions[visibleCount * 2] = tx;
            positions[visibleCount * 2 + 1] = ty;
            visibleCount++;
        }

        for (int i = 0; i < visibleCount; ++i) {
            const float tx = positions[i * 2];
            const float ty = positions[i * 2 + 1];
            env->CallVoidMethod(paint, gDraw.setColor, gDraw.red);
            env->CallVoidMethod(canvas, gDraw.drawLine, startX, startY, tx, ty, paint);
            env->CallVoidMethod(paint, gDraw.setColor, gDraw.cyan);
            env->CallVoidMethod(canvas, gDraw.drawCircle, tx, ty, 6.0f, paint);
        }
    }

    env->DeleteLocalRef(paint);
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
    return env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) == 0 ? JNI_OK : JNI_ERR;
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
    return env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) == 0 ? JNI_OK : JNI_ERR;
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
    return env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) == 0 ? JNI_OK : JNI_ERR;
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
    return env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) == 0 ? JNI_OK : JNI_ERR;
}

int RegisterESPView(JNIEnv* env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("DrawOn"), OBFUSCATE("(Landroid/graphics/Canvas;)V"), reinterpret_cast<void*>(DrawOn)},
    };

    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/ESPView"));
    if (!clazz) {
        return JNI_ERR;
    }
    return env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) == 0 ? JNI_OK : JNI_ERR;
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
