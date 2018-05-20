//
// Created by hyperandroid on 13/02/2016.
//

#include "V8Event.h"
#include "Event.h"
#include "Configuration.h"



const int HC_GARBAGE_COLLECTED_CLASS_ID = 16;

namespace V8EventInternal {

    void preventDefault( const FunctionCallbackInfo<Value>& info ) {

    }

    void TypeGetter(const FunctionCallbackInfo<Value> &info) {
        Event* ev = Config::ToImpl<Event>(info.Holder());
        if ( ev!= nullptr ) {
            info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), ev->Type()));
        } else {
            info.GetReturnValue().Set(Null(info.GetIsolate()));
        }
    }

    void TimeStampGetter(const FunctionCallbackInfo<Value> &info) {
        Event* ev = Config::ToImpl<Event>(info.Holder());
        if ( ev!=nullptr ) {
            info.GetReturnValue().Set((double) ev->TimeStamp());
        } else {
            info.GetReturnValue().Set(Null(info.GetIsolate()));
        }
    }

    void TargetGetter(const FunctionCallbackInfo<Value> &info) {
        Event* ev = Config::ToImpl<Event>(info.Holder());
        if ( ev!=nullptr ) {
            if ( ev->target==nullptr ) {
                info.GetReturnValue().Set(Null(info.GetIsolate()));
            } else {
                info.GetReturnValue().Set(ev->target->GetWrapper(info.GetIsolate()));
            }
        } else {
            info.GetReturnValue().Set(Null(info.GetIsolate()));
        }
    }

    void CurrentTargetGetter(const FunctionCallbackInfo<Value> &info) {
        Event* ev = Config::ToImpl<Event>(info.Holder());
        if ( ev!=nullptr ) {
            if ( ev->currentTarget==nullptr ) {
                info.GetReturnValue().Set(Null(info.GetIsolate()));
            } else {
                info.GetReturnValue().Set(ev->currentTarget->GetWrapper(info.GetIsolate()));
            }
        } else {
            info.GetReturnValue().Set(Null(info.GetIsolate()));
        }
    }
}

const WrapperTypeInfo V8Event::wrapperTypeInfo = {
        V8Event::InterfaceTemplate,
        "Event",
        nullptr,
        2,
        HC_GARBAGE_COLLECTED_CLASS_ID
};

const WrapperTypeInfo& Event::wrapperTypeInfo_ = V8Event::wrapperTypeInfo;

static Config::AccessorConfiguration props[] = {
        {"type",            V8EventInternal::TypeGetter,            nullptr, v8::DontDelete, Config::kOnPrototype},
        {"timeStamp",       V8EventInternal::TimeStampGetter,       nullptr, v8::DontDelete, Config::kOnPrototype},
        {"target",          V8EventInternal::TargetGetter,          nullptr, v8::DontDelete, Config::kOnPrototype},
        {"currentTarget",   V8EventInternal::CurrentTargetGetter,   nullptr, v8::DontDelete, Config::kOnPrototype},
};

static Config::MethodConfiguration methods[] = {
        {"preventDefault", V8EventInternal::preventDefault, v8::DontDelete, Config::kOnPrototype, 0}
};

void V8Event::constructorCallback(const FunctionCallbackInfo<Value> &ci) {

    if ( !ci.IsConstructCall() ) {
        Config::Throw(ci.GetIsolate(), "Must be constructor");
        return;
    }

    // a better approach would be to have PerIsolate data, so that we can query for current
    // constructor mode.
    if ( Config::Status::CurrentConstructorMode == Config::ConstructorMode::kWrapExistingObject ) {
        ci.GetReturnValue().Set( ci.Holder() );
        return;
    }

    if ( ci.kArgsLength<1 || !ci[0]->IsString()) {
        Config::Throw(ci.GetIsolate(), "V8Event needs a type string argument.");
        return;
    }

    String::Utf8Value utf(ci[0]);

    Event* event = new Event(*utf);
    v8::Local<v8::Object> wrapper = ci.Holder();
    event->AssociateWithWrapper( ci.GetIsolate(), &V8Event::wrapperTypeInfo, wrapper);

    ci.GetReturnValue().Set(wrapper);
}

Local<FunctionTemplate> V8Event::InterfaceTemplate(Isolate *isolate) {
    return Config::InterfaceTemplate(isolate, wrapperTypeInfo, V8Event::InstallInterfaceTemplate);
}

void V8Event::InstallInterfaceTemplate( Isolate* isolate, Local<FunctionTemplate> interface_template ) {

    Config::InitializeInterfaceTemplate(isolate, interface_template, wrapperTypeInfo );

    // function template
    interface_template->SetCallHandler(V8Event::constructorCallback);
    interface_template->SetLength(1);

    v8::Local<v8::Signature> signature = v8::Signature::New(isolate, interface_template);

    Local<ObjectTemplate> prototype_t = interface_template->PrototypeTemplate();
    Local<ObjectTemplate> instance_t = interface_template->InstanceTemplate();

    Config::InstallAccessors(isolate, instance_t, prototype_t, interface_template, signature, props,
                          ARRAY_LENGTH(props));

    Config::InstallMethods(isolate, instance_t, prototype_t, interface_template, signature, methods,
                        ARRAY_LENGTH(methods));
}
