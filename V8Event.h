//
// Created by hyperandroid on 13/02/2016.
//

#ifndef HYPERCASINO_V8EVENT_H
#define HYPERCASINO_V8EVENT_H


#include <v8.h>
#include "Configuration.h"

using namespace v8;

/**
 * A generic Event.
 * This will be the base class for all other existing events like TouchEvent, SurfaceEvent, etc.
 */
class V8Event {
public:

    // This class must be static only
    V8Event() = delete;

    V8Event(const V8Event &) = delete;

    V8Event &operator=(const V8Event &) = delete;

    void *operator new(size_t) = delete;

    void *operator new(size_t, int, void *) = delete;

    void *operator new(size_t, void *) = delete;

    // V8 base.
    static Local<FunctionTemplate> InterfaceTemplate(Isolate *);

    static void
    InstallInterfaceTemplate(Isolate *isolate, Local<FunctionTemplate> interface_template);

    static void constructorCallback(const FunctionCallbackInfo<Value> &);

    static const Config::WrapperTypeInfo wrapperTypeInfo;
};

#endif //HYPERCASINO_V8EVENT_H
