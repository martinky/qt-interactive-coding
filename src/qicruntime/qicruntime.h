#ifndef QICRUNTIME_H
#define QICRUNTIME_H

#include <QString>

#include "qiccontext.h"

#if defined QIC_DLL
#define QIC_EXPORT Q_DECL_EXPORT
#else
#define QIC_EXPORT Q_DECL_IMPORT
#endif

class qicRuntimePrivate;

/**
    The qicRuntime class provides the runtime build and execution environment.

    The exec() method takes a piece of C++ source code, wraps it in a shared
    library `qmake` project and builds using the installed C++ toolchain and Qt
    SDK. The code passed to this function must define and export the qic_entry()
    function. Upon successful compilation of this code into a shared library,
    this library is loaded, and the qic_entry() function is called.

    Use the addVar() method to pass data to the runtime-compiled code and
    getVar() to retrieve data created by the runtime code.

    Use the various setters to control the build environment. You can override
    environment variables, path to the `qmake` and `make` programs, add defines,
    include paths and linked libraries. By default, the compiled library links
    the QtCore library. You can override this or link with other Qt libraries
    using setQtLibs(). Use setDebug(true) to produce a debug build.

    The loadEnv() method comes in handy, if you need to replicate a build
    environment that is not your default command line environment. It loads a
    set of environment variables from a file. You can easily take a snapshot of
    your build environment using the `env` command on Linux and `set` command
    on Windows and then replicate this build environment in qicRuntime. To do
    this, open a terminal, configure your build environment, then grab the
    output of `env` or `set` into a file and load this file using loadEnv().
 */
class QIC_EXPORT qicRuntime : public qicContext
{
public:
    qicRuntime();
    ~qicRuntime();

    //TODO: make exec() async, return a quasi future object qicExecutable

    bool exec(QString source);
    bool execFile(QString filename);

    // build env

    void setEnv(QString name, QString value);
    void addEnv(QString name, QString value);
    void loadEnv(QString path);
    void setQmake(QString path);
    void setMake(QString path);
    void setDebug(bool debug);
    void setDefines(QStringList defines);
    void setIncludePath(QStringList dirs);
    void setLibs(QStringList libs);
    void setQtLibs(QStringList qtlibs);

    // ctx

    void *getVar(const char *name) override;
    void *addVar(void *ptr, const char *name, void(*deleter)(void*)) override;

    void debug(const char *str) override;

private:
    bool compile(QString src);

private:
    qicRuntimePrivate *p;
};

#endif // QICRUNTIME_H
