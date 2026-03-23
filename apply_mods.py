#!/usr/bin/env python3
"""Apply ALL remaining modifications to Main.cpp in one pass."""

with open('/tmp/dump_main.cpp', 'r') as f:
    c = f.read()

# 1. Add ProcessAllEnemyModifications + ApplySpeedHack calls in Hooked_SendShootCommand
old = '''    ProcessAutomaticAdsWork();
    ProcessPendingKillAllWork();
    ProcessPendingEconomyWork();

    if (orig_SendShootCommand) {
        orig_SendShootCommand(instance);
    }'''
new = '''    ProcessAutomaticAdsWork();
    ProcessPendingKillAllWork();
    ProcessPendingEconomyWork();
    ProcessAllEnemyModifications();
    ApplySpeedHack();

    if (orig_SendShootCommand) {
        orig_SendShootCommand(instance);
    }'''
c = c.replace(old, new, 1)

# 2. Add same calls in Hooked_MoveCam
old = '''    ProcessAutomaticAdsWork();
    ProcessPendingKillAllWork();
    ProcessPendingEconomyWork();

    if (orig_MoveCam) {
        orig_MoveCam(instance);
    }'''
new = '''    ProcessAutomaticAdsWork();
    ProcessPendingKillAllWork();
    ProcessPendingEconomyWork();
    ProcessAllEnemyModifications();
    ApplySpeedHack();

    if (orig_MoveCam) {
        orig_MoveCam(instance);
    }'''
c = c.replace(old, new, 1)

# 3. Add same calls in Hooked_ActionShoot
old = '''    ProcessAutomaticAdsWork();
    ProcessPendingKillAllWork();
    ProcessPendingEconomyWork();

    AimCameraAtEnemy(instance);'''
# There are two places with this pattern (MoveCam already changed), find ActionShoot one
# Actually let's be more specific - find it in the ActionShoot context
old = '''    ProcessAutomaticAdsWork();
    ProcessPendingKillAllWork();
    ProcessPendingEconomyWork();

    AimCameraAtEnemy(instance);

    if (orig_ActionShoot) {'''
new = '''    ProcessAutomaticAdsWork();
    ProcessPendingKillAllWork();
    ProcessPendingEconomyWork();
    ProcessAllEnemyModifications();
    ApplySpeedHack();

    AimCameraAtEnemy(instance);

    if (orig_ActionShoot) {'''
c = c.replace(old, new, 1)

# 4. Add NeedReloading hook + resolve new APIs in InstallHooksThread
old = '''    gEnemyApplyDamage = reinterpret_cast<EnemyApplyDamage_t>(enemyApplyDamageTarget);
    gEnemyKillEnemy = reinterpret_cast<EnemyKill_t>(enemyKillTarget);'''
new = '''    gEnemyApplyDamage = reinterpret_cast<EnemyApplyDamage_t>(enemyApplyDamageTarget);
    gEnemyKillEnemy = reinterpret_cast<EnemyKill_t>(enemyKillTarget);
    gEnemySetCurrentHealth = reinterpret_cast<EnemySetFloat_t>(getAbsoluteAddress(kTargetLibName, kEnemyControllerSetCurrentHealthRva));
    gEnemySetDamage = reinterpret_cast<EnemySetFloat_t>(getAbsoluteAddress(kTargetLibName, kEnemyControllerSetDamageRva));
    gEnemySetDamageProbability = reinterpret_cast<EnemySetFloat_t>(getAbsoluteAddress(kTargetLibName, kEnemyControllerSetDamageProbabilityRva));
    const auto needReloadingTarget = reinterpret_cast<void*>(getAbsoluteAddress(kTargetLibName, kPlayerControlNeedReloadingRva));'''
c = c.replace(old, new, 1)

# 5. Add NeedReloading hook installation after enemyApplyDamage hook
old = '''    if (applovinHasVideoTarget) {'''
new = '''    if (needReloadingTarget) {
        MSHookFunction(
                needReloadingTarget,
                reinterpret_cast<void*>(Hooked_NeedReloading),
                reinterpret_cast<void**>(&orig_NeedReloading));
    }
    if (applovinHasVideoTarget) {'''
c = c.replace(old, new, 1)

# 6. Update GetFeatureList with new features
old = '''            OBFUSCATE("Category_Ads"),
            OBFUSCATE("21_Button_Desativar ads"),
            OBFUSCATE("22_Button_Desativar videos"),'''
new = '''            OBFUSCATE("Category_Arsenal"),
            OBFUSCATE("23_Toggle_Municao infinita"),
            OBFUSCATE("24_Toggle_Sem recuo (no recoil)"),
            OBFUSCATE("25_SeekBar_Speed hack_1_5"),
            OBFUSCATE("Category_Inimigos"),
            OBFUSCATE("26_Toggle_Congelar inimigos"),
            OBFUSCATE("27_Toggle_Inimigos nao atacam"),
            OBFUSCATE("28_Toggle_Remover escudo inimigos"),
            OBFUSCATE("29_Toggle_Desativar granadas inimigas"),
            OBFUSCATE("30_Toggle_One shot kill"),
            OBFUSCATE("31_Toggle_Mostrar info no ESP"),
            OBFUSCATE("Category_Ads"),
            OBFUSCATE("32_Button_Desativar ads"),
            OBFUSCATE("33_Button_Desativar videos"),'''
c = c.replace(old, new, 1)

# 7. Update switch cases - update old case numbers for ads
old = '''        case 21:
            gDisableAdsRequested = true;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Desativar ads enfileirado");
            break;
        case 22:
            gDisableVideosRequested = true;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Desativar videos enfileirado");
            break;'''
new = '''        case 23:
            gInfiniteAmmo = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Municao infinita %s", gInfiniteAmmo ? "ON" : "OFF");
            break;
        case 24:
            gNoRecoil = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "No recoil %s", gNoRecoil ? "ON" : "OFF");
            break;
        case 25:
            gSpeedHack = true;
            gSpeedMultiplier = static_cast<float>(value);
            if (gSpeedMultiplier < 1.0f) {
                gSpeedMultiplier = 1.0f;
                gSpeedHack = false;
            }
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Speed hack %.1fx", gSpeedMultiplier);
            break;
        case 26:
            gFreezeEnemies = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Freeze enemies %s", gFreezeEnemies ? "ON" : "OFF");
            break;
        case 27:
            gDisableEnemyDamage = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Disable enemy damage %s", gDisableEnemyDamage ? "ON" : "OFF");
            break;
        case 28:
            gRemoveEnemyShield = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Remove shield %s", gRemoveEnemyShield ? "ON" : "OFF");
            break;
        case 29:
            gDisableEnemyGrenades = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Disable grenades %s", gDisableEnemyGrenades ? "ON" : "OFF");
            break;
        case 30:
            gOneShotKill = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "One shot kill %s", gOneShotKill ? "ON" : "OFF");
            break;
        case 31:
            gEnemyInfoEsp = boolean;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Enemy info ESP %s", gEnemyInfoEsp ? "ON" : "OFF");
            break;
        case 32:
            gDisableAdsRequested = true;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Desativar ads enfileirado");
            break;
        case 33:
            gDisableVideosRequested = true;
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Desativar videos enfileirado");
            break;'''
c = c.replace(old, new, 1)

# 8. Add enemy info text in DrawOn (after the enemy line/skeleton drawing, before JNI exception check)
old = '''        if (ClearJniException(env, "desenhar inimigo")) {
            RegisterDrawFailure(env, "desenhar inimigo");
            return;
        }'''
new = '''        if (gEnemyInfoEsp) {
            const bool isBoss = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsBossOffset);
            const bool isSniper = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsSniperOffset);
            const bool isBazooka = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsBazookaOffset);
            const bool isZombie = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsZombieOffset);
            const bool isFlying = *reinterpret_cast<bool*>(enemyAddr + kEnemyControllerIsFlyingOffset);
            const char* tag = isBoss ? "BOSS" : isSniper ? "SNP" : isBazooka ? "RPG" : isZombie ? "ZMB" : isFlying ? "FLY" : nullptr;
            if (tag) {
                jstring infoText = env->NewStringUTF(tag);
                if (infoText) {
                    int tr = isBoss ? 255 : 200;
                    int tg = isBoss ? 60 : 200;
                    int tb = isBoss ? 60 : 200;
                    env->CallVoidMethod(
                            thiz,
                            gDraw.espDrawText,
                            canvas,
                            static_cast<jint>(255),
                            static_cast<jint>(tr),
                            static_cast<jint>(tg),
                            static_cast<jint>(tb),
                            infoText,
                            tx,
                            ty - 18.0f,
                            14.0f);
                    env->DeleteLocalRef(infoText);
                }
            }
        }

        if (ClearJniException(env, "desenhar inimigo")) {
            RegisterDrawFailure(env, "desenhar inimigo");
            return;
        }'''
c = c.replace(old, new, 1)

with open('/tmp/dump_main.cpp', 'w') as f:
    f.write(c)

print("OK - all modifications applied to /tmp/dump_main.cpp")
