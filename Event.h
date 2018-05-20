//
// Created by hyperandroid on 02/02/2016.
//

#ifndef HYPERCASINO_EVENT_H
#define HYPERCASINO_EVENT_H

#include <v8.h>
#include "Wrappable.h"

using namespace v8;


/**
 * A generic Event.
 * This will be the base class for all other existing events like TouchEvent, SurfaceEvent, etc.
 */
class Event : public Wrappable {

    DEFINE_WRAPPERTYPEINFO();

public:

    Event( const char* const );
    virtual ~Event();

    // this class disallows copy and assignment to prevent referencing twice the same v8 Persistent
    // ref.
    Event(const Event&) = delete;
    void operator=(const Event&) = delete;

    const char* Type() const;
    long TimeStamp() const;

    Wrappable* target;
    Wrappable* currentTarget;

    long timeStamp;
protected:

    char* type;
};

#endif //HYPERCASINO_EVENT_H
