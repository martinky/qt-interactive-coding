/*
Copyright (c) 2018 Martin Kutny

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef QICRUNTIME_H
#define QICRUNTIME_H

#include <QString>

#if defined QIC_DLL
#define QIC_EXPORT Q_DECL_EXPORT
#else
#define QIC_EXPORT Q_DECL_IMPORT
#endif

class qicRuntimePrivate;

struct qicContext;

class QIODevice;

/**
    \class qicRuntime
    The qicRuntime class provides the runtime build and execution environment.

    The exec() method takes a piece of self-contained C++ source code, wraps it
    in a shared library `qmake` project and builds it using the installed C++
    toolchain and Qt SDK. The source code must define and export the
    qic_entry() function. Upon successful compilation of this code into a
    shared library, this library is loaded, and the qic_entry() function is
    resolved and called.

    The ctx() method returns the qicContext object for exchange of data with
    the runtime-compiled code.

    Use the various setters to control the build environment. You can override
    environment variables, path to the `qmake` and `make` programs, add defines,
    include paths and linked libraries. By default, the compiled library does
    not link with Qt. You can override and link with Qt libraries using
    setQtLibs(). Use setQtConfig() to configure additional build options.

    \fn qicRuntime::qicRuntime()
    Constructs a default build and runtime environment. Environment variables
    are inherited from the parent process. The `qmake` and `make` (or `nmake`
    on Windows) utilities are expected to be on PATH.

    \fn qicRuntime::~qicRuntime()
    Destroys the runtime environment. Destroys all objects registered by the
    runtime-compiled code via qicContext::set() and unloads all libraries in
    reverse order.

    \fn qicRuntime::exec()
    Compiles and executes the provided C++ code. This method is blocking and
    returns only after the build process completes and the qic_entry() function
    returns.

    \fn qicRuntime::execFile()
    Same as exec() except the source code is read from the \a filename;

    \fn qicRuntime::setEnv()
    Sets an environment variable for the build process.

    \fn qicRuntime::addEnv()
    Appends an environment variable, using the system's native path delimiter.
    Useful for appending the PATH env variable.

    \fn qicRuntime::loadEnv()
    Sets one or more environment variables loaded from a file. Useful for
    configuring a complete build environment.

    \fn qicRuntime::setQmake()
    Sets the path to the `qmake` utility. Useful when multiple installations
    of the Qt SDK are present.

    \fn qicRuntime::setMake()
    Sets the path to the `make` utility, or `nmake` on Windows.

    \fn qicRuntime::setDefines()
    Sets the content of the **DEFINES** `qmake` variable.

    \fn qicRuntime::setIncludePath()
    Sets the content of the **INCLUDEPATH** `qmake` variable.

    \fn qicRuntime::setLibs()
    Sets the content of the **LIBS** `qmake` variable.

    \fn qicRuntime::setQtLibs()
    Sets the content of the **QT** `qmake` variable. This controls which Qt
    libraries will be linked with the binary. By default Qt is not linked.

    \fn qicRuntime::setQtConfig()
    Sets the content of the **CONFIG** `qmake` variable. This is used to
    control build options such as debug/release, rtti, exceptions, etc. If the
    host application is compiled using `CONFIG=debug`, make sure, the runtime
    code is also compiled with the same option. Otherwise the host application
    and the runtime code will be linked with different Qt libraries and
    different versions of the CRT runtime which will cause unpredictable fatal
    errors.

    \fn qicRuntime::setOutputSink()
    Directs the output of the build process to the given \a device. This can be
    used to record the output and display it through a GUI, for example. To
    completely discard the output, set this to `nullptr`.

    \fn qicRuntime::setOutputSinkToStdOut()
    Directs the output of the build process to the standard output. This is the
    default behavior.

    \fn qicRuntime::ctx()
    Returns pointer to qicContext that can be used to share data with the
    runtime code.
 */
class QIC_EXPORT qicRuntime
{
public:
    qicRuntime();
    ~qicRuntime();

    bool exec(QString source);
    bool execFile(QString filename);

    // build env

    void setEnv(QString name, QString value);
    void addEnv(QString name, QString value);
    bool loadEnv(QString path);
    void setQmake(QString path);
    void setMake(QString path);
    void setDefines(QStringList defines);
    void setIncludePath(QStringList dirs);
    void setLibs(QStringList libs);
    void setQtLibs(QStringList qtlibs);
    void setQtConfig(QStringList qtconf);

    void setOutputSink(QIODevice *device);
    void setOutputSinkToStdOut();

    qicContext *ctx();

private:
    bool compile(QString src);

private:
    qicRuntimePrivate *p;
};

#endif // QICRUNTIME_H
