//
// Created by hyperandroid on 01/02/2016.
//

#ifndef HYPERCASINO_CONFIGURATION_H
#define HYPERCASINO_CONFIGURATION_H

#include <v8.h>

namespace Config {

    typedef v8::Local<v8::FunctionTemplate> (*CreateTemplateFunction)(v8::Isolate*);

    class WrapperTypeInfo {
    public:

        CreateTemplateFunction template_function;
        const char* const interface_name;
        CreateTemplateFunction parent_class;
        int internal_field_count;
        uint16_t gc_class_id;
    };

    enum ConstructorMode : unsigned { kWrapExistingObject, kCreateNewObject };

    enum PropertyLocationConfiguration : unsigned {
        kOnInstance = 1 << 0,
        kOnPrototype = 1 << 1,
        kOnInterface = 1 << 2,
    };

    struct Status {
        static ConstructorMode CurrentConstructorMode;
    };

    template<typename T, size_t Size>
    char (&ArrayLengthHelperFunction(T (&)[Size]))[Size];

    #define ARRAY_LENGTH(array) sizeof(ArrayLengthHelperFunction(array))

    struct AccessorConfiguration {
        AccessorConfiguration &operator=(const AccessorConfiguration &) = delete;

        const char *const name;
        v8::FunctionCallback getter;
        v8::FunctionCallback setter;
        unsigned attribute : 8;                         // v8::PropertyAttribute
        unsigned property_location_configuration : 3;   // PropertyLocationConfiguration
    };

    struct MethodConfiguration {
        MethodConfiguration &operator=(const MethodConfiguration &) = delete;

        v8::Local<v8::Name> MethodName(v8::Isolate *isolate) const {

            return v8::String::NewFromOneByte(
                    isolate, reinterpret_cast<const uint8_t *>(name),
                    v8::NewStringType::kInternalized, strlen(name))
                    .ToLocalChecked();
        }

        const char *const name;
        v8::FunctionCallback callback;
        unsigned attribute : 8;                         // v8::PropertyAttribute
        unsigned property_location_configuration : 3;   // PropertyLocationConfiguration
        int length;
    };

    template<class T>
    T *ToImpl(v8::Local<v8::Object> object) {
        return reinterpret_cast<T *>(object->GetAlignedPointerFromInternalField(0));
    }

    void InstallAccessors(
            v8::Isolate *isolate,
            v8::Local<v8::ObjectTemplate> instance_or_template,
            v8::Local<v8::ObjectTemplate> prototype_or_template,
            v8::Local<v8::FunctionTemplate> interface_or_template,
            v8::Local<v8::Signature> signature,
            const AccessorConfiguration *props,
            size_t length);

    template<class ObjectOrTemplate, class FunctionOrTemplate>
    void InstallAccessor(
            v8::Isolate *isolate,
            v8::Local<ObjectOrTemplate> instance_or_template,
            v8::Local<ObjectOrTemplate> prototype_or_template,
            v8::Local<FunctionOrTemplate> interface_or_template,
            v8::Local<v8::Signature> signature,
            const AccessorConfiguration &prop);

    void InstallMethods(
            v8::Isolate *isolate,
            v8::Local<v8::ObjectTemplate> instance_or_template,
            v8::Local<v8::ObjectTemplate> prototype_or_template,
            v8::Local<v8::FunctionTemplate> interface_or_template,
            v8::Local<v8::Signature> signature,
            const MethodConfiguration *methods,
            size_t length);

    void InstallMethod(
            v8::Isolate *isolate,
            v8::Local<v8::ObjectTemplate> instance_or_template,
            v8::Local<v8::ObjectTemplate> prototype_or_template,
            v8::Local<v8::FunctionTemplate> interface_or_template,
            v8::Local<v8::Signature> signature,
            const MethodConfiguration &method);

    typedef void (*InstallTemplateFunction)(v8::Isolate *,
                                            v8::Local<v8::FunctionTemplate>);

    void SetClassString(
            v8::Isolate *isolate,
            v8::Local<v8::ObjectTemplate> object_template,
            const char *class_string);

    v8::Local<v8::FunctionTemplate> InterfaceTemplate(v8::Isolate *,
                                                      const WrapperTypeInfo &,
                                                      InstallTemplateFunction);

    typedef void (*InstallTemplateFunction)(v8::Isolate*,
                                            v8::Local<v8::FunctionTemplate>);

    void InitializeInterfaceTemplate(v8::Isolate *,
                                     v8::Local<v8::FunctionTemplate>,
                                     const WrapperTypeInfo &);

    void Throw(v8::Isolate* isolate_, const char* message);
}

#endif //HYPERCASINO_CONFIGURATION_H
