# Wrapping native v8 objects - Part I

While embedding v8, exposing native objects in Javascript is mostly unavoidable.
As easy as it might sound the process involves a very specific set of steps as well as
certain design decisions. From Javascript standpoint, there will no difference between
a pure Javascript object and a wrapped one.

For example, you must be able to extend a native object prototype with a plain Javascript object
or vice versa.

The link between Javascript and native world is bidirectional.
A native wrappable object, should refer to a single Javascript object
(not necessarily, but it will save a lot of headaches at a later stage with garbage collection).
This is done by keeping a reciprocal reference between a v8::Persisten<v8::Object> and a native object as we will see.

## Design decisions

Native wrappable objects will extends a base class object: Wrappable.
Its purpose is to keep track of the Javascript object wrapping a native object (hence its name)
and also does the heavy lifting to associate a native object with a Javascript object, like
invoking the constructor, setting Javascript object's native pointers, etc.

A Wrappable will rely on a WrapperTypeInfo to configure itself.
Each potentially wrappable exposed in Javascript will define one such struct.
For example, for an actual Event object I define:

```c++
const WrapperTypeInfo V8Event::wrapperTypeInfo = {
        V8Event::InterfaceTemplate,        // configuration function
        "Event",                           // js object context
        nullptr                            // inherit from
};
```

For code simplicity, I will define two cpp files for each object I'd like wrapped and exposed
in Javascript. For the Event object sample, a file Event.cpp will keep the delegated methods
invoked from javascript. Don't forgetEvent class extends Wrappable so inherits the possibility
to generate the Javascript wrapper on demand. The other file, V8Event.cpp contains all
javascript related stuff, like the constructor function, the list of accessors, functions, etc.

Another design decision I make is about object extension. Any Javascript object extending another
object, will reflect in its native wrappable extending the another wrappable.
Basically, makes sense an object extending another in Javascript, necessarily means c++ class
extending another one.

# Configuring wrapper objects

To expose an instance-able object in Javascript we need to create a constructorFunction.
Depending on whether we expect this to be part of the global context object,
we will create either a `v8::FunctionTemplate` or a regular `v8::Function object`.

In this article, I will be creating an Event object in Javascript, which is actually delegating
all its functionality to a Native c++ wrappable instance.

## FunctionTemplate structure

As we know, a javascript function, is a first class object.
It can be called, can be called as a constructor (sic. instanced), which will create a
prototype chain if needed, and the function object itself can hold functions and variables.
All this translates directly into native code, where a `v8::FunctionTemplate` object
(which we will name interface_template from now on), exposes two methods returning a
`Local<ObjectTemplate>`` :

```c++
// prototype function template
Local<ObjectTemplate> prototype_t= interface_t->PrototypeTemplate();
// instance function template
Local<ObjectTemplate> instance_t = interface_t->InstanceTemplate();
```

So, for each wrappable object, we will manage 3 different places to add native bindings code to:

+ Prototype template will be used, of course, to define accessors and function on the prototype.
+ Instance template will be used to add accessors or functions to an instance resulting from calling the constructor function.
+ Interface template will be used to add accessors or functions to the constructor function itself. These won't be accessible from any instance or object prototype though.

## Exposing a Constructor function in Javascript

The constructor function is the entry point to create new Javascript objects and bind them with
native wrappable objects. 

The first thing would be to make our instantiation function available in Javascript.
This is done by exposing our interface_template in the global context object (or any other object).
Something like:

```c++
global_template_object->Set(
        v8::String::NewFromUtf8(isolate_, "Event"),
        interface_template->GetFunction());
```

When the Event object is created in Javascript by calling `new Event()``, the wrapper will invoke
our supplied construction callback. This constructor callback is defined as:

```c++
// c++ function invoked when new Event() is called in js.
// see "Javascript-native object relationship"
interface_template->SetCallHandler(V8Event::constructorCallback);
// constructor length, e.g. number of parameters
interface_template->SetLength(1);
```

This `CallHandler` function, is an special native constructor delegate. It is responsible
for associating a Javascript with its native wrappable when invoked from javascript as `new Event()`,
but it also must handle the situation when an existing native object, just needs be wrapped and
be available in javascript. We'll see how to do this later in the article.

The CallHandler, has a signature of a regular native function callback:

```c++
void constructorCallback(const FunctionCallbackInfo<Value> &ci);
```

## Adding accessors

Accessors will act as properties' getter and setter function callbacks. Normally I will add
accessors in the prototype or instance templates. Or both. But they can be added as well to
the interface template. Once in Javascript, there will be no difference with a regular object
variable, except for the fact that behind the scenes, a native object is accessed, and a native
value is wrapped as a Javascript type.

For accessors in prototypes or instances, we need to add a regular `v8::FunctionTemplate`
representing the getter or setter to the corresponding v8::ObjectTemplate. 
Adding accessor to the interface template is a bit different, since its type is
`v8::FunctionTemplate`, and not `v8::ObjectTemplate`. Doing this is mostly trivial:

```c++
void native_getter(const FunctionCallbackInfo<Value> &info) {
    ...
}
void native_setter(const FunctionCallbackInfo<Value> &info) {
    ...
}
// getters don't need parameters, we pass 0.
Local<FunctionTemplate> getter = v8::FunctionTemplate::New(
        isolate, native_getter, Local<Value>(), Local<Value>(), 0 );
// setter need a parameter, so we pass 1.
Local<FunctionTemplate> setter = v8::FunctionTemplate::New(
        isolate, native_getter, Local<Value>(), Local<Value>(), 1);
// these getter/setter function callbacks don't need a prototype.
getter->RemovePrototype();
setter->RemovePrototype();
// create an accessor name
local<String> name = String::NewFromUtf8( isolate, "prop" );
// binding:
instance_or_template->SetAccessorProperty(
        name,
        getter, setter,
        attribute         // see v8::PropertyAttribute
);
```

Now, whenever we call from Javascript ev.prop, the native_getter function is invoked.
Our wrappable code starts to make sense now.

We must monotonically perform the same process for each accessor we'd like to have in our
Javascript wrapper objects. 

## Adding functions

Another functionality wrapper objects need is functions.
As in the case of accessors, Javascript function callbacks must be defined. Simply enough:

```c++
void callback(const FunctionCallbackInfo<Value> &info) {
    ...
}
v8::Local<v8::FunctionTemplate> function_template =
        v8::FunctionTemplate::New(
               isolate,
               callback,
               v8::Local<v8::Value>(),
               v8::Local<v8::Value>(),
               numberOfParameters );
// again, prototype not needed for this callback function
function_template->RemovePrototype();

instance_or_template->Set(
    method.MethodName(isolate),
    function_template,
    attribute           // see v8::PropertyAttribute
);
```

Again, repeat this process for each function we want to be available in our Javascript wrapper.

## Javascript-native object relationship - Javascript Instantiation

When code like this is executed in Javascript

```javascript
 new Event()
```

the registered constructor callback is invoked. It will also add all defined accessor and
function bindings, create the prototype chain, etc. As a native function callback, the signature is:

```c++
void constructorCallback(const FunctionCallbackInfo<Value> &ci)
```

It has three main responsibilities:

+ abort object creation if this is not a valid constructor. For example, TouchList is not instance-able by constructor, so it is safe to throw an exception here. A call of the form will do the trick.

```c++
isolate_->ThrowException( v8::Exception::Error( v8::String ...
// don't forget to return from constructorCallback after throwing...
return;
```

+ Generate native wrappable instance and associate it with the Javascript object:

```c++
// create a wrappable
Event* event = new Event();
// we associate a native Event object, with constructorCallback's
// holder. Holder() points to the object being constructed in
// javascript.
v8::Local<v8::Object> wrapper = ci.Holder();
// Event instance holds a v8::Persistent<v8::Object> reference
event->wrapper_.Reset(isolate, wrapper);
// Manage gc.
//
// e.g:
//    event->wrapper_.SetWrapperClassId( ...
//    event->wrapper_.SetWeak( ...
// or
//    event->wrapper_.ClearWeak();
```

See my other article to manage garbage collection of wrapped objects.

+ Associate the Javascript object with the native wrappable object

```c++
int indices[] = {0};
void* values[] = {event};

wrapper->SetAlignedPointerInInternalFields(1, indices, values);
```

After this code, a Javascript object has a pointer to the wrappable c++ object.
And the wrapple c++, has set a v8::Persistent  handle to the same Javascript object. 

## Accessor and Function callbacks

For the three of accessor getter, setter and function callbacks, we must specify a function of
type `v8::FunctionCallback`, that is

```c++
void fnName(const FunctionCallbackInfo<Value> &info);
```

How the Javascript object accesses its native wrappable object is as follows:

```c++
void fnName(const FunctionCallbackInfo<Value> &info) {
    Local<Object> holder = info.Holder();
    Event* event = reinterpret_cast<Event*>(
        object->GetAlignedPointerFromInternalField(0));
    info->GetReturnValue().Set( event->name );
}
```

The key here is where to obtain the native pointer to the wrappable object.
And that will always be the `Holder` object on every `FunctionCallback`.

## Javascript-native object relationship - Native Instantiation

Sometimes, we want to expose in Javascript a native wrappable object w/o it being created
from Javascript. For example, a pointer data generated in native needs to make its way through to Javascript.

```c++
Event* ev = new Event("load");
// set wrappable properties
ev->target = this;
ev->currentTarget = this;
// create a Javascript object to reference to this ev
// native instance.
```

For this to happen, we need to manually create a Javascript object of the needed type.
We have the interface template so seems like a trivial operation, just (deprecated: call
it as constructor) new instance the interface template function.

We would be mostly done, except for the fact that this code will invoke the `constructorCallback`
previously defined. Which will create a new Event instance. Our constructorCallback must
therefore be aware of the fact that an existing native object is being wrapped instead.
Other than that, the code will be the same for the constructorCallback.

```c++
// Signal constructorCallback to wrap an existing object,
// instead of creating a new one. RAII on isolate's private data.
ConstructorMode constructorMode(isolate);
v8::Local<v8::Object> wrapper = interface_template->
        GetFunction()->
        NewInstance(
           creation_context->CreationContext()
        ).ToLocalChecked();
// do wrapper association just as in the javascript instantiation
// example.
```

Needless to say , I also trivially check for an already existing Javascript wrapper for the native object.

## Prototype Inheritance

Inheriting a prototype for a wrappable object can only happen from a native prototype.
Don't worry though, you will be able to extend a wrapper object from Javascript.

The inheritance process boils down just to one native call, and must happen at constructor
function definition time. This code will extend the prototype chain of our wrapper object:

```c++
// function_template->Inherit( v8::Local<v8::FunctionTemplate> );
//
// this function template is the previously defined
// interface_template
//
interface_template->Inherit(parent_interface_template);
```

## Object naming

By default, wrappable objects will identify themselves in chrome devtools console as the i
nfamous: `{}`, or `[object Object]` that is, no name. To properly have our wrappables named
in dev-tools, we'd need to do a couple things:

+ set the constructor function class name.

```c++
interface_template->SetClassName( v8::Local<v8::String> );
```

This will properly name our wrapper exposed constructor function. Unfortunately, this won't be sufficient, since our wrapper's prototype object still won't show the expected naming. To fix this, a trickier approach must be taken:

+ name our prototype object:

```c++
prototype_template->Set(
        v8::Symbol::GetToStringTag(isolate),
        v8::Local<v8::String>,  // desired string representation
        v8::PropertyAttribute); // e.g.: v8::ReadOnly | v8::DontEnum
```

I bet you probably had never used a Symbol before uh ?

## Conclusion
There is still some other stuff that native wrappable objects can do. Indexed properties,
which would accept indexed access on them like an array or for the DOM TouchList object.
Or interceptors, Proxies, …

The bestiary of v8 is fairly interesting and each of them deserve one or more chapters to describe them.

As seen in the article, at first glance it is not easy to create a Javascript wrapper.
Too many steps, for something conceptually simple, specially if we compare it with defining
a plain Javascript object. That's Javascript's magic. It abstracts away all the dirty details
from the developer, like GC, instantiation, wiring, etc. This article might be also useful to
realise how complex a browser can just be. We just scratched the surface and shaw how to wire
the simplest native object possible.
At the same time, this article should give a grasp of how powerful V8 is. think of the hundreds
of really complex objects available in Javascript, like webaudio, video tags, css, ... mind blowing.

I'd like to make an explicit disclaimer here. All credit of this article must go to the V8 and
Chromium project developers. Being able to scavenge through the whole browser source code has
been an invaluable resource to figure out stuff, and to learn very solid foundations on v8
development. On my side I just made the effort of harvesting low hanging fruit.



# Executing the project

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