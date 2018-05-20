//
// Created by hyperandroid on 20/11/2015.
//

#include <map>
#include <string>
#include "Configuration.h"

using namespace v8;

Config::ConstructorMode Config::Status::CurrentConstructorMode = Config::ConstructorMode::kCreateNewObject;

void Config::InstallAccessors(
        Isolate *isolate,
        v8::Local<ObjectTemplate> instance_or_template,
        v8::Local<ObjectTemplate> prototype_or_template,
        v8::Local<FunctionTemplate> interface_or_template,
        v8::Local<v8::Signature> signature,
        const AccessorConfiguration *props,
        size_t length) {

    for (int i = 0; i < length; i++) {
        InstallAccessor(isolate, instance_or_template, prototype_or_template, interface_or_template, signature, props[i]);
    }
}

template <class ObjectOrTemplate, class FunctionOrTemplate>
void Config::InstallAccessor(
        Isolate *isolate,
        v8::Local<ObjectOrTemplate> instance_or_template,
        v8::Local<ObjectOrTemplate> prototype_or_template,
        v8::Local<FunctionOrTemplate> interface_or_template,
        v8::Local<v8::Signature> signature,
        const AccessorConfiguration &prop) {

    Local<FunctionTemplate> getter = v8::FunctionTemplate::New(
            isolate, prop.getter, Local<Value>(), signature, 0 );

    if (!getter.IsEmpty()) {
        getter->RemovePrototype();
        getter->SetAcceptAnyReceiver(true);
    }

    Local<FunctionTemplate> setter = v8::FunctionTemplate::New(
            isolate, prop.setter, Local<Value>(), signature, 1 );
    if (!setter.IsEmpty()) {
        setter->RemovePrototype();
        setter->SetAcceptAnyReceiver(true);
    }

    unsigned location = prop.property_location_configuration;
    Local<String> name = String::NewFromUtf8( isolate, prop.name, v8::NewStringType::kInternalized, strlen(prop.name) ).ToLocalChecked();

    if ( location & (kOnInstance | kOnPrototype) ) {

        if (location & kOnInstance && !instance_or_template.IsEmpty()) {
            instance_or_template->SetAccessorProperty(
                    name, getter, setter,
                    static_cast<v8::PropertyAttribute>(prop.attribute));
        }

        if (location & kOnPrototype && !instance_or_template.IsEmpty()) {
            prototype_or_template->SetAccessorProperty(
                    name, getter, setter,
                    static_cast<v8::PropertyAttribute>(prop.attribute));
        }
    }

    if (location & kOnInterface) {
        interface_or_template->SetAccessorProperty(
                name, getter, setter,
                static_cast<v8::PropertyAttribute>(prop.attribute));
    }
}

void Config::InstallMethods(
        Isolate *isolate,
        v8::Local<v8::ObjectTemplate> instance_or_template,
        v8::Local<v8::ObjectTemplate> prototype_or_template,
        v8::Local<v8::FunctionTemplate> interface_or_template,
        v8::Local<v8::Signature> signature,
        const MethodConfiguration *methods,
        size_t length) {

    for (int i = 0; i < length; i++) {
        InstallMethod(isolate, instance_or_template, prototype_or_template, interface_or_template, signature, methods[i]);
    }
}

void Config::InstallMethod(
        Isolate* isolate,
        v8::Local<v8::ObjectTemplate> instance_or_template,
        v8::Local<v8::ObjectTemplate> prototype_or_template,
        v8::Local<v8::FunctionTemplate> interface_or_template,
        v8::Local<v8::Signature> signature,
        const MethodConfiguration& method ) {

    v8::FunctionCallback callback = method.callback;

    if (method.property_location_configuration & (kOnInstance | kOnPrototype)) {
        v8::Local<v8::FunctionTemplate> function_template =
                v8::FunctionTemplate::New(isolate, callback, v8::Local<v8::Value>(),
                                          signature, method.length);

        function_template->RemovePrototype();

        if (method.property_location_configuration & kOnInstance) {
            instance_or_template->Set(method.MethodName(isolate), function_template,
                                      static_cast<v8::PropertyAttribute>(method.attribute));
        }
        if (method.property_location_configuration & kOnPrototype) {
            prototype_or_template->Set(method.MethodName(isolate), function_template,
                                       static_cast<v8::PropertyAttribute>(method.attribute));
        }
    }

    if (method.property_location_configuration & kOnInterface) {

        v8::Local<v8::FunctionTemplate> function_template =
                v8::FunctionTemplate::New(isolate, callback, v8::Local<v8::Value>(),
                                          v8::Local<v8::Signature>(), method.length);
        function_template->RemovePrototype();

        interface_or_template->Set(method.MethodName(isolate), function_template,
                                   static_cast<v8::PropertyAttribute>(method.attribute));
    }
}

static std::map<std::string, Eternal<FunctionTemplate>> iterfaceTemplates_;

Local<FunctionTemplate> Config::InterfaceTemplate( v8::Isolate* isolate,
                                           const WrapperTypeInfo& typeInfo,
                                           Config::InstallTemplateFunction itf) {

    std::string name = typeInfo.interface_name;
    auto iter= iterfaceTemplates_.find(name);
    if ( iter != iterfaceTemplates_.end() ) {
        return (*iter).second.Get(isolate);
    }

    auto ft = v8::FunctionTemplate::New( isolate );
    itf(isolate, ft );
    iterfaceTemplates_.insert(
            std::pair<std::string, v8::Eternal<v8::FunctionTemplate>>(
                    name,
                    v8::Eternal<v8::FunctionTemplate>(isolate, ft)));

    return ft;
}


void Config::SetClassString(
        v8::Isolate* isolate,
        v8::Local<v8::ObjectTemplate> object_template,
        const char* class_string) {

    object_template->Set(
            v8::Symbol::GetToStringTag(isolate),
            v8::String::NewFromOneByte(isolate, (const uint8_t *)class_string),
            static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontEnum));
}


void Config::InitializeInterfaceTemplate(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> interface_template,
                                 const WrapperTypeInfo& typeInfo) {


    interface_template->SetClassName(v8::String::NewFromOneByte(isolate, (const uint8_t *)typeInfo.interface_name));
    interface_template->ReadOnlyPrototype();

    v8::Local<v8::ObjectTemplate> instance_template = interface_template->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> prototype_template = interface_template->PrototypeTemplate();
    instance_template->SetInternalFieldCount(typeInfo.internal_field_count);

    SetClassString(isolate, prototype_template, typeInfo.interface_name);

    if ( typeInfo.parent_class!=nullptr ) {
        Local<FunctionTemplate> parent_interface_template = typeInfo.parent_class(isolate);
        if (!parent_interface_template.IsEmpty()) {
            interface_template->Inherit(parent_interface_template);
            prototype_template->SetInternalFieldCount(1);
        }
    }
}

void Config::Throw(v8::Isolate* isolate_, const char* message) {
    if (!isolate_->IsExecutionTerminating()) {
        isolate_->ThrowException(
                v8::Exception::Error(
                        v8::String::NewFromUtf8(
                                isolate_,
                                message)));
    }
}