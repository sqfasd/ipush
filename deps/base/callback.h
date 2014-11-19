// classes and functions to replace pointer-to-functions

// Abstract interface for a callback.  When calling an RPC, you must provide
// a Closure to call when the procedure completes.  See the Service interface
// in service.h.
//
// To automatically construct a Closure which calls a particular function or
// method with a particular set of parameters, use the NewOneTimeCallback()
// function.
// Example:
//   void FooDone(const FooResponse* response) {
//     ...
//   }
//
//   void CallFoo() {
//     ...
//     // When done, call FooDone() and pass it a pointer to the response.
//     Closure* callback = NewOneTimeCallback(&FooDone, response);
//     // Make the call.
//     service->Foo(controller, request, response, callback);
//   }
//
// Example that calls a method:
//   class Handler {
//    public:
//     ...
//
//     void FooDone(const FooResponse* response) {
//       ...
//     }
//
//     void CallFoo() {
//       ...
//       // When done, call FooDone() and pass it a pointer to the response.
//       Closure* cb = NewOneTimeCallback(this, &Handler::FooDone, response);
//       // Make the call.
//       service->Foo(controller, request, response, cb);
//     }
//   };
//
// Currently NewOneTimeCallback() supports binding up to three arguments.
//
// Callbacks created with NewOneTimeCallback() automatically delete themselves
// when executed. They should be used when a callback is to be called exactly
// once (usually the case with RPC callbacks).  If a callback may be called
// a different number of times (including zero), create it with
// NewPermanentCallback() instead. You are then responsible for deleting the
// callback (using the "delete" keyword as normal).
//
// Note that NewOneTimeCallback() is a bit touchy regarding argument types.
// Generally, the values you provide for the parameter bindings must exactly
// match the types accepted by the callback function.  For example:
//   void Foo(string s);
//   NewOneTimeCallback(&Foo, "foo");  // WON'T WORK: const char* != string
//   NewOneTimeCallback(&Foo, string("foo"));  // WORKS
// Also note that the arguments cannot be references:
//   void Foo(const string& s);
//   string my_str;
//   NewOneTimeCallback(&Foo, my_str);  // WON'T WORK:  Can't use referecnes.
// However, correctly-typed pointers will work just fine.

#ifndef BASE_CALLBACK_H_
#define BASE_CALLBACK_H_

#include "base/callback_spec.h"

#endif  // BASE_CALLBACK_H_
