//
// Created by hyperandroid on 02/02/2016.
//

#include "Event.h"

Event::Event( const char* event ) : Wrappable(), target(nullptr), currentTarget(nullptr), timeStamp(0L) {

    // naive
    int len = strlen(event);
    type = new char[len+1];
    type[len]= 0;
    memcpy( type, event, len );
}

Event::~Event() {
    delete type;
}

const char* Event::Type() const {
    return type;
}

long Event::TimeStamp() const {
    return timeStamp;
}