//
// Created by hyperandroid on 12/02/2016.
//

#ifndef HYPERCASINO_WRAPPABLE_H
#define HYPERCASINO_WRAPPABLE_H

#include <v8.h>
#include "Configuration.h"

using namespace Config;

class Wrappable {

public:

    Wrappable() {};

    virtual ~Wrappable();

    virtual const WrapperTypeInfo *GetWrapperTypeInfo() const = 0;

    virtual v8::Local<v8::Object> Wrap(v8::Isolate *,
                                       v8::Local<v8::Context> creation_context);

    virtual v8::Local<v8::Object> AssociateWithWrapper(
            v8::Isolate *,
            const WrapperTypeInfo *,
            v8::Local<v8::Object> wrapper);

    bool SetWrapper(v8::Isolate *isolate,
                    v8::Local<v8::Object>& wrapper,
                    const WrapperTypeInfo* wrapper_type_info);

    void SetNativeInfo(const WrapperTypeInfo *wrapper_type_info,
                       v8::Local<v8::Object> wrapper );

    bool IsEqualTo(v8::Isolate *isolate, const v8::Local<v8::Object> &other) const {
        return wrapper_.Get(isolate) == other;
    }

    bool ContainsWrapper() const { return !wrapper_.IsEmpty(); }

    v8::Local<v8::Object> GetWrapper(v8::Isolate *isolate) const {
        return wrapper_.Get(isolate);
    }

    void SetWrapperClassId( uint16_t type ) {
        wrapper_.SetWrapperClassId(type);
    }

protected:

private:

    v8::Persistent<v8::Object> wrapper_;
};

#define DEFINE_WRAPPERTYPEINFO()                                        \
        const WrapperTypeInfo* GetWrapperTypeInfo() const override {    \
            return &wrapperTypeInfo_;                                   \
        }                                                               \
    private:                                                            \
        static const WrapperTypeInfo& wrapperTypeInfo_

#endif //HYPERCASINO_WRAPPABLE_H
