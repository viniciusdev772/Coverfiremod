#include <jni.h>

#include <android/log.h>
#include <cstdio>

#include "DialogOnLoad.h"
#include "Includes/obfuscate.h"

namespace {

constexpr const char* kLogTag = "MOD_NATIVE_MIN";
constexpr const char* kMenuTitle = "Cover Fire";
constexpr const char* kMenuSubtitle = "Login validado e menu pronto";
constexpr const char* kIconBase64 =
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+aF9sAAAAASUVORK5CYII=";
constexpr const char* kIconWebData =
        "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+aF9sAAAAASUVORK5CYII=";

void LogInfo(const char* message) {
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "%s", message ? message : "");
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

jboolean isGameLibLoaded(JNIEnv*, jobject) {
    return JNI_TRUE;
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
            OBFUSCATE("Category_Menu de Modificações"),
            OBFUSCATE("Collapse_Status"),
            OBFUSCATE("CollapseAdd_Category_Status"),
            OBFUSCATE("CollapseAdd_0_Button_Login validado no launcher"),
            OBFUSCATE("CollapseAdd_1_Button_Lib nativa mínima sem hooks"),
    };
    return NewStringArray(env, kFeatures, sizeof(kFeatures) / sizeof(kFeatures[0]));
}

void Changes(JNIEnv* env, jclass, jobject context, jint featNum, jstring, jint, jboolean, jstring) {
    RegisterDialogContext(env, context);
    if (!IsDialogLoginValidated()) {
        ShowQueuedLibLoadDialog(env, context);
        __android_log_print(ANDROID_LOG_WARN, kLogTag, "Mudança ignorada antes do login: %d", featNum);
        return;
    }

    __android_log_print(ANDROID_LOG_INFO, kLogTag, "Feature acionada sem hook ativo: %d", featNum);
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

void DrawOn(JNIEnv*, jobject, jobject) {
}

int RegisterMenu(JNIEnv* env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("Icon"), OBFUSCATE("()Ljava/lang/String;"), reinterpret_cast<void*>(Icon)},
            {OBFUSCATE("IconWebViewData"), OBFUSCATE("()Ljava/lang/String;"), reinterpret_cast<void*>(IconWebViewData)},
            {OBFUSCATE("IsGameLibLoaded"), OBFUSCATE("()Z"), reinterpret_cast<void*>(isGameLibLoaded)},
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

    LogInfo("Carregando JNI mínimo sem hooks");

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

    return JNI_VERSION_1_6;
}
