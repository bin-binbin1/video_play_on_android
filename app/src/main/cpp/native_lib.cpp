#include <jni.h>
#include "log.h"

#include <string>
#include <iostream>

#include"SingletonPlayer.h"
#include"ANWRender.h"
//#include "test.h"
#include<unistd.h>
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_androidplayer_Player_nativePlay(JNIEnv *env, jobject thiz, jstring file,
                                                 jobject surface) {
    // TODO: implement nativePlay()

    const char* path=env->GetStringUTFChars(file,NULL);
    SingletonPlayer* p=SingletonPlayer::getInstance();
    p->play(path+5,1024,436, ANativeWindow_fromSurface(env,surface));
    return 0;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_androidplayer_Player_nativePause(JNIEnv *env, jobject thiz, jboolean p) {
    // TODO: implement nativePause()
    SingletonPlayer::getInstance()->stop(p);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_androidplayer_Player_nativeSeek(JNIEnv *env, jobject thiz, jdouble position) {
    // TODO: implement nativeSeek()
    SingletonPlayer::getInstance()->seekTo(position);
    return 1;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_androidplayer_Player_nativeStop(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeStop()
    SingletonPlayer::getInstance()->stop(true);
    return 1;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_androidplayer_Player_nativeSetSpeed(JNIEnv *env, jobject thiz, jfloat speed) {
    // TODO: implement nativeSetSpeed()
    SingletonPlayer::getInstance()->changeSpeed(speed);
    return 1;
}
extern "C"
JNIEXPORT jdouble JNICALL
Java_com_example_androidplayer_Player_nativeGetPosition(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeGetPosition()
    LOGD("position","%lf, total=%lf",static_cast<jdouble>( SingletonPlayer::getInstance()->getPosition()),
         static_cast<jdouble>( SingletonPlayer::getInstance()->getDuration()));
    return  static_cast<jdouble>( SingletonPlayer::getInstance()->getPosition());

}
extern "C"
JNIEXPORT jdouble JNICALL
Java_com_example_androidplayer_Player_nativeGetDuration(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeGetDuration()

    return static_cast<jdouble>( SingletonPlayer::getInstance()->getDuration());

}