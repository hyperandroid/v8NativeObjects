#include <main.h>
#include <v8-version-string.h>
#include <libplatform/libplatform.h>
#include "V8Event.h"
#include "Event.h"

using namespace v8;

/**
 * Must stub in case external snapshot files are used.
 */
namespace v8 {
    namespace internal {
        void ReadNatives() {}

        void DisposeNatives() {}

        void SetNativesFromFile(v8::StartupData *s) {}

        void SetSnapshotFromFile(v8::StartupData *s) {}
    }
}

static Isolate* isolate_;
static Persistent<Context> context_;

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOGV("JNI OnLoad called");

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    vm->AttachCurrentThread(&env, nullptr);

    return JNI_VERSION_1_6;
}


void log( const v8::FunctionCallbackInfo<Value>& info ) {

    // naive implementation
    v8::String::Utf8Value utf(info[0].As<v8::String>());
    LOGV("%s", *utf );
}

void nativeFactory( const v8::FunctionCallbackInfo<Value>& info ) {

    HandleScope hs( info.GetIsolate() );

    Event *ev = new Event("factory");

    struct timespec __now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &__now);
    ev->timeStamp = __now.tv_sec;

    Local<Object> new_js_event = ev->Wrap( isolate_, context_.Get(isolate_) );
    info.GetReturnValue().Set( new_js_event );
}

void initializeV8() {
    // 666: leaking platform.
    Platform *platform = v8::platform::CreateDefaultPlatform();
    V8::InitializePlatform(platform);
    V8::Initialize();
}


void runScript(const char* cscript ) {

    v8::HandleScope scope(isolate_);
    v8::Local<v8::Context> context = context_.Get( isolate_ );
    v8::Context::Scope context_scope(context);

    //
    v8::Local<v8::String> source;
    source = v8::String::NewFromUtf8(isolate_, cscript);
    v8::ScriptOrigin origin(v8::String::NewFromUtf8(isolate_, "test"));
    v8::MaybeLocal<v8::Script> maybescript = v8::Script::Compile(context, source, &origin);
    if (maybescript.IsEmpty()) {
        return;
    }

    v8::Local<v8::Script> script;

    if (maybescript.ToLocal(&script)) {
        v8::Local<v8::Value> result = script->Run();
        if (result.IsEmpty()) {
            // an error ocurred.
            // catched by isolate handlers.
        }
    }
}

void RunV8Stuff() {
    v8::Isolate::CreateParams params;
    params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate_ = v8::Isolate::New(params);
    isolate_->Enter();

    v8::Isolate::Scope isolatescope(isolate_);
    v8::HandleScope scope(isolate_);

    v8::Eternal<v8::FunctionTemplate> v8EventConstructor(isolate_,
                                                         V8Event::InterfaceTemplate(isolate_));

    // Create a gloal object template
    auto global_template = v8::ObjectTemplate::New(isolate_);

    global_template->Set(
            v8::String::NewFromUtf8(isolate_, "log"),
            v8::FunctionTemplate::New(isolate_, log)
    );

    global_template->Set(
            v8::String::NewFromUtf8(isolate_, "nativeFactory"),
            v8::FunctionTemplate::New(isolate_, nativeFactory)
    );

    global_template->Set(
            v8::String::NewFromUtf8(isolate_, "Event"),
            v8EventConstructor.Get(isolate_));

    /**
     * create a context with the global context template. Our Event object is there as a
     * constructor function.
     */
    v8::Local<v8::Context> context = v8::Context::New(isolate_, nullptr, global_template);
    context_.Reset(isolate_, context);

    /**
     * Enumerate our global object.
     */
    runScript( "log('context properties enumeration:'); for(var i in this) {log(i);}; ");

    /**
     * Enumerate properties in fresh Event object created from javascript.
     */
    runScript( "this.ev = new Event('event_type'); log('event properties enumeration:'); for(var i in ev) { log(i) }; log('typeof ev'); log(typeof ev); log('object prototype toString'); log(Object.prototype.toString.call(ev));");

    /**
     * ev.timeStamp does not change, it is readonly.
     */
    runScript( "log('create from js'); ev.timeStamp=43; log(ev.timeStamp); log(ev.type);");

    /**
     * calling nativeFactory, creates a native Event wrappable, and exposes it in javascript as
     * `nativeFunction` return value.
     * These native wrappables have timesatmp set to something !=0, and their type as `factory`.
     */
    runScript( "log('create from native'); var ev2 = nativeFactory(); log(ev2.timeStamp); log(ev2.type);");

}

extern "C" {
JNIEXPORT void JNICALL
Java_com_socialgames_v8tutorial_SocialGames_InitializeV8(JNIEnv *env, jobject obj) {

    initializeV8();


    // call com.socialgames.v8tutorial.SocialGames.setVersion( v8_version )
    // find java method com.socialgames.v8tutorial.SocialGames.setVersion
    jclass clazz = env->GetObjectClass(obj);
    jmethodID method = env->GetMethodID(clazz, "setVersion", "(Ljava/lang/String;)V");
    if (env->ExceptionCheck() == JNI_TRUE) {
        LOGV("JNI Exception detected");
        env->ExceptionDescribe();
        env->ExceptionClear();
    } else {
        // if found, call it.
        jstring jVersionStr = env->NewStringUTF(V8_VERSION_STRING);
        env->CallVoidMethod(obj, method, jVersionStr);
        env->DeleteLocalRef(jVersionStr);
    }

    RunV8Stuff();
}

}

