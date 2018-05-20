//
// Created by hyperandroid on 12/02/2016.
//

#include "Wrappable.h"
#include "Configuration.h"

Wrappable::~Wrappable() {
    if (ContainsWrapper()) {
        wrapper_.Reset();
    }
}

v8::Local<v8::Object> Wrappable::Wrap(
        v8::Isolate *isolate,
        v8::Local<v8::Context> creation_context) {

    auto othis = GetWrapper(isolate);
    if (ContainsWrapper() && IsEqualTo(isolate, othis)) {
        return othis;
    }

    const WrapperTypeInfo *wrapper_type_info = GetWrapperTypeInfo();

    // wrapping means we are wrapping an existing object.
    // bugbug: use RAII. this just for demo purposes.
    Config::ConstructorMode prevConstructorMode = Config::Status::CurrentConstructorMode;
    Config::Status::CurrentConstructorMode = Config::kWrapExistingObject;

    // create a cache of boilerplate objects and Clone ??
    v8::Local<v8::Object> wrapper = wrapper_type_info->
            template_function(isolate)->
            GetFunction()->
            NewInstance(creation_context).ToLocalChecked();

    auto ret = AssociateWithWrapper(isolate, wrapper_type_info, wrapper);

    Config::Status::CurrentConstructorMode = prevConstructorMode;

    return ret;
}

v8::Local<v8::Object> Wrappable::AssociateWithWrapper(
        v8::Isolate *isolate,
        const WrapperTypeInfo *wrapper_type_info,
        v8::Local<v8::Object> wrapper) {

    if (SetWrapper(isolate, wrapper, wrapper_type_info)) {
        SetNativeInfo(wrapper_type_info, wrapper);
    }

    return wrapper;
}

void Wrappable::SetNativeInfo(const WrapperTypeInfo *wrapper_type_info,
                            v8::Local<v8::Object> wrapper) {

    // set native info
    int indices[] = {0, 1};
    void *values[] = {this,
                      const_cast<WrapperTypeInfo *>(wrapper_type_info)};

    wrapper->SetAlignedPointerInInternalFields(
            2, indices, values);

}

static void weakCallbackForDOMObjectHolder(const v8::WeakCallbackInfo<Wrappable> &data) {
    delete data.GetParameter();
}

bool Wrappable::SetWrapper(v8::Isolate *isolate,
                         v8::Local<v8::Object> &wrapper,
                         const WrapperTypeInfo *wrapper_type_info) {

    if (ContainsWrapper()) {
        wrapper = v8::Local<v8::Object>::New(isolate, wrapper_);
        return false;
    }

    wrapper_.Reset(isolate, wrapper);
    wrapper_.SetWrapperClassId(wrapper_type_info->gc_class_id);

    // basic GC management.
    wrapper_.SetWeak(
            this,
            weakCallbackForDOMObjectHolder,
            v8::WeakCallbackType::kParameter);

    return true;
}