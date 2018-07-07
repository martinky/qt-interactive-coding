# Qt Interactive Coding

Author: [Martin Kutny](https://kutny.net/) \
License: MIT

## Summary

Simple, cross-platform library to automate the process of compilation and
execution of C++ code from within a running application. Allows to use C++
as a scripting language. This library is implemented using Qt and therefore
is suitable for integrating with Qt projects.

Possible uses:

- interactive / creative / live coding
- debugging, state inspection
- application scripting

**Dependencies:** Qt 5 (libQtCore5, qmake), C++ compiler toolchain \
**Platforms:** Windows, Linux, possibly other platforms supported by Qt

## Demo

![demo gif](demo1.gif)

## Integration

The library consists of one C++ source file and a couple of headers. You can
either build it as a shared or static library using `qicruntime.pro`, or copy
the [code](src/qicruntime/) directly into your Qt project and include
`qicruntime.pri`.

## Usage

Basic example: Compiling and running C++ code at runtime.

``` c++
#include <qicruntime.h>

int main()
{
    qicRuntime rt;
    // configure your runtime build environment
    rt.setIncludePath({ "/path/to/src/qicruntime" });
    rt.setQmake("/path/to/qt/version/platform/bin/qmake");

    const char src[] = "#include <qicentry.h>\n"
                       "#include <stdio.h>\n"
                       "extern \"C\" QIC_ENTRY_EXPORT void qic_entry(qicContext *ctx) {\n"
                       "    printf(\"hello runtime!\");\n"
                       "}\n";

    rt.exec(src);

    return 0;
}
```

For more examples, see the code in the [examples](src/examples/) directory.

## Interop

To interact and exchange data with the runtime-compiled code, use *context
variables*. An entire, complex application model can be shared with the runtime
code this way.

``` c++
class AppModel {
    // This class represents the data and functionality of our application that
    // we want to be able to make 'scriptable'. Actually defined somewhere
    // in a common header or shared library, so it can be included and linked
    // by both the host program and the runtime-compiled code.
};

int main()
{
    AppModel model;

    qicRuntime rt;
    rt.ctx()->set(&model, "model");

    rt.exec(...);
}
```

In the runtime code, access the *context variable* via the `qicContext`.

``` c++
#include <AppModel.h> // possibly also link with AppModel.lib/.dll

extern "C" void qic_entry(qicContext *ctx)
{
    AppModel *const model = static_cast<AppModel*>(ctx->get("model"));
    model->... // access or modify data, call methods
}
```

## Design

The principle behind this library is very simple, no magic, no special tricks.
Conceptually, the process is similar to how plugin systems work and boils down
to 4 simple steps.

1. compile user code into a shared object (.dll)
2. dlopen()
3. entry_point = dlsym()
4. call entry_point(app_context)

The purpose of this library is to automate this process in a simple and
portable manner so that from the user's perspective this is a simple one-liner:

``` c++
//qicRuntime rt;
rt.exec(source_code);
```

To compile the runtime code, we make use of Qt's own build system `qmake` and
leverage its natural cross-platform capability.

There are no restrictions on what can or cannot go into the runtime-compiled
source code. The only requirement is that the code exports one C-style function
that serves as the main entry point:

``` c++
extern "C" void qic_entry(qicContext *ctx);
```

The `qicContext` is a simple interface for exchange of `void*` pointers to
arbitrary data between the host program and the runtime code. This is most
flexible, as the user is free to use any techique for data exchange and
interaction:

- pointers to POD structures,
- C function pointers, callbacks,
- virtual base interfaces,
- full C++ classes in common shared libraries,
- or whatever data persistence mechanism the user comes up with.

## Things to Keep in Mind

The mechanism of this library is similar to plugin systems and share the same
set of behaviors the user should be aware of:

1. We are loading and executing unsafe, untested, native code. There are
   a million ways how to shoot yourself in the foot with this. Let’s just
   accept that we can bring the host program down any time. This technique
   is intended for development only. It should not be used in production or
   situations where you can’t afford to lose data.
2. Make sure that both the host program and the script code are compiled in
   a binary compatible manner: using the same toolchain, same build options,
   and if they share any libraries, be sure that both link the same version
   of those libraries. Failing to do so is an invitation to undefined behavior
   and crashes.
3. You need to be aware of object lifetime and ownership when sharing data
   between the host program and a script. At some point, the library that
   contains script code will be unloaded – its code and data unmapped from
   the host process address space. If the host program accesses this data or
   code after it has been unloaded, it will result in a segfault. Typically,
   a strange crash just before the program exits is indicative of an object
   lifetime issue.

## Resources

Blog:

- [My Take On Run-time Compiled C++](https://blog.kutny.net/2018/07/02/my-take-on-run-time-compiled-c/)

Inspiration:

1. [Using runtime-compiled C++ code as a scripting language: under the hood](https://blog.molecular-matters.com/2014/05/10/using-runtime-compiled-c-code-as-a-scripting-language-under-the-hood/)
2. [Runtime Compiled C++](https://github.com/RuntimeCompiledCPlusPlus/RuntimeCompiledCPlusPlus)
3. [DLL Hot Reloading in Theory and Practice](http://ourmachinery.com/post/dll-hot-reloading-in-theory-and-practice/)
4. [Read-Compile-Run-Loop - a tiny REPL for C++](http://onqtam.com/programming/2018-02-12-read-compile-run-loop-a-tiny-repl-for-cpp/)
5. [A comprehensive list of projects and resources on runtime compiled C/C++](https://github.com/RuntimeCompiledCPlusPlus/RuntimeCompiledCPlusPlus/wiki/Alternatives) compiled by @dougbinks
