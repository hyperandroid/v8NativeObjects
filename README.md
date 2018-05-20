This project is the runnable part of this post:

This project is expected to run on Android, but all v8 related code is in
`initializeV8` and `RunV8Stuff`.

The goal is to showcase a barebones example of how to deal with wrappable native objects in Javascript.
How to define the constructor callback, set accessors and functions, how to properly name objects, etc.

When run, this project does the following steps:

+ initialize v8
+ create an isolate
+ define a wrappable object (Event)
+ generate a global object template with support for:
  + log: naive logging capabilities
  + nativeFactory: a native function callback info. Creates a native object and exposes it in javascript.
+ execute a few scripts relying in `log` function to show how the wrappable object works.

### Script results

#### context properties enumeration:

```javascript
log('context properties enumeration:');
for(var i in this) {
    log(i);
}
```

Output:
```
log
nativeFactory
Event
i
```

`log` and `nativeFactory` are native function callbacks.

`Event` is our wrappable object constructor function exposed in Javascript.

#### Event properties enumeration:

```javascript
this.ev = new Event('event_type');
log('event properties enumeration:');
for(var i in ev) {
    log(i)
};

log('typeof ev');
log(typeof ev);

log('object prototype toString');
log(Object.prototype.toString.call(ev));
```

This code creates a new wrapper object to a native Event.

```
// wrapper object attributes and functions.
type
timeStamp
target
currentTarget
preventDefault

// typeof ev
typeof ev
object

// Object toString representation:
object prototype toString
[object Event]
```

#### create from js

```javascript
log('create from js');
ev.timeStamp=43;
log(ev.timeStamp);
log(ev.type);
```

Output:
```
create from js
// timeStamp property is readonly. See V8Event.cpp
0
// 'event_type' is the name defined for this event in the previous script.
event_type
```

#### create from native

This example creates a native Event object, and exposes it in native. A different operation than
creating an Event instance from javascript.

```javascript
log('create from native');
var ev2 = nativeFactory();
log(ev2.timeStamp);
log(ev2.type);
```

Output:
```
create from native
// when created from native, we set a timeStamp. Still readonly though
92330
// native generated Event wrappers are of type `factory`.
factory
```

## Interoperability

This Event wrapper object, can be extended and used as any other regular Javascript object.

There's no operative difference with a pure Javascript object beyond the fact that its properties
values will interact with a c++ native object behind the scenes.