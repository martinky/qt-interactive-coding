/* Copyright (c) 2018 Martin Kutny

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
SOFTWARE. */

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QElapsedTimer>
#include <QLibrary>
#include <QProcess>
#include "qicruntime.h"
#include "qiccontext.h"


struct qicVar
{
    void *ptr = nullptr;
    char *name = nullptr;
    void (*deleter)(void *) = nullptr;
};

struct qicFrame
{
    QLibrary *lib = nullptr;
    std::vector<qicVar> vars;
};


struct qicContextImpl : public qicContext
{
    // Stack of context frames. A frame holds the library that contains the
    // runtime-compiled code and any variables this code may have registered.
    std::vector<qicFrame> frames;

    qicContextImpl()
    {
        // push one empty frame to hold user defined global variables
        frames.push_back(qicFrame());
    }

    ~qicContextImpl()
    {
        // unload libs in reverse order
        for (auto fit = frames.rbegin(); fit != frames.rend(); ++fit) {
            // destroy lib vars in reverse order before unload
            for (auto vit = fit->vars.rbegin(); vit != fit->vars.rend(); ++vit) {
                ::free(vit->name);
                if (vit->deleter) {
                    vit->deleter(vit->ptr);
                }
            }
            if (fit->lib) {
                fit->lib->unload();
                delete fit->lib;
            }
        }
    }

    void *get(const char *name) override
    {
        // search context frames and their variables in reverse order - most
        // recently set variables override previously set variables
        for (auto fit = frames.rbegin(); fit != frames.rend(); ++fit) {
            for (auto vit = fit->vars.rbegin(); vit != fit->vars.rend(); ++vit) {
                if (0 == ::strcmp(name, vit->name)) {
                    return vit->ptr;
                }
            }
        }

        return nullptr;
    }

    void *set(void *ptr, const char *name, void(*deleter)(void*)) override
    {
        Q_ASSERT(frames.empty() == false);
        frames.back().vars.push_back({ ptr, ::_strdup(name), deleter });
        return ptr;
    }

    void debug(const char *fmt, ...) override
    {
        char buff[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buff, 1024, fmt, args);
        va_end(args);
        qDebug("%s", buff);
    }
};


class qicRuntimePrivate
{
public:
    QTemporaryDir dir;
    QProcessEnvironment env;
    QString qmake, make;
    // qmake project variables
    QStringList defines;        // DEFINES
    QStringList include_path;   // INCLUDEPATH
    QStringList qtlibs;         // QT
    QStringList qtconf;         // CONFIG
    QStringList libs;           // LIBS

    qicContextImpl ctx;

    QIODevice *output_sink = nullptr;
    QFile fstdout;

    qicRuntimePrivate()
    {
        env = QProcessEnvironment::systemEnvironment();

        qmake = "qmake"; // assume to be on PATH
#ifdef Q_OS_WIN
        make = "nmake";
#else
        make = "make";
#endif

        fstdout.open(stdout, QIODevice::WriteOnly|QIODevice::Unbuffered);
        output_sink = &fstdout;
    }

    void output(const char *fmt, ...)
    {
        char buff[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buff, 1024, fmt, args);
        va_end(args);
        if (output_sink) {
            using Qt::endl;
            QTextStream t(output_sink);
            t << buff << endl;
        }
    }

    bool loadEnv(QString path)
    {
        //env.clear();

        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            return false;
        }

        while (!f.atEnd()) {
            QString line = f.readLine().trimmed();
            int eq = line.indexOf(QChar('='));
            if (eq < 0) continue;
            QString name = line.mid(0, eq);
            QString value = line.mid(eq+1);
            env.insert(name, value);
        }

        return true;
    }

    void sinkProcessOutput(QByteArray name, QProcess *proc)
    {
        while (proc->bytesAvailable() > 0) {
            const QByteArray line = proc->readLine();
            if (output_sink) {
                output_sink->write(name + line);
            } // otherwise just discard process output
        }
    }

    bool runProcess(QByteArray name, QString program, QStringList arguments = QStringList())
    {
        QProcess proc;
        proc.setWorkingDirectory(dir.path());
        proc.setProcessEnvironment(env);
        proc.setProcessChannelMode(QProcess::MergedChannels);
        QObject::connect(&proc, &QProcess::readyRead, [&]{
            sinkProcessOutput(name, &proc);
        });
        proc.start(program, arguments);
        proc.waitForFinished();
        return proc.exitStatus() == QProcess::NormalExit &&
               proc.state()      == QProcess::NotRunning &&
               proc.exitCode()   == 0;
    }

    QString getLibPath() const
    {
#ifdef Q_OS_WIN
        QString libn = qtconf.contains("debug") ? "debug/a%1.dll" : "release/a%1.dll";
#else
        QString libn = "a%1";
#endif
        return dir.filePath(libn.arg(seq()));
    }

    int seq() const
    {
        return (int)ctx.frames.size();
    }
};



qicRuntime::qicRuntime() :
    p(new qicRuntimePrivate)
{
}

qicRuntime::~qicRuntime()
{
    delete p;
}

bool qicRuntime::exec(QString source)
{
    // compile

    if (!compile(source)) {
        return false;
    }

    // load library

    QString lib_path = p->getLibPath();
    QLibrary *lib = new QLibrary(lib_path);
    if (!lib->load()) {
        p->output("qicRuntime: Failed to load library %s: %s", qPrintable(lib_path), qPrintable(lib->errorString()));
        delete lib;
        return false;
    }

    // resolve entry point

    typedef void (*qic_entry_f)(qicContext *);
    qic_entry_f qic_entry = (qic_entry_f) lib->resolve("qic_entry");
    if (!qic_entry) {
        p->output("qicRuntime: Failed to resolve qic_entry: %s", qPrintable(lib->errorString()));
        lib->unload();
        delete lib;
        return false;
    }

    // add frame record

    qicFrame frame;
    frame.lib = lib;
    p->ctx.frames.push_back(frame);

    // execute

    qic_entry(&p->ctx);

    return true;
}

bool qicRuntime::execFile(QString filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        p->output("qicRuntime: Failed to open source file: %s", qPrintable(filename));
        return false;
    }
    QTextStream t(&f);
    return exec(t.readAll());
}

void qicRuntime::setEnv(QString name, QString value)
{
    p->env.insert(name, value);
}

void qicRuntime::addEnv(QString name, QString value)
{
    QString old = p->env.value(name);
    if (!old.isEmpty() && !value.isEmpty()) {
        value.append(QDir::listSeparator());
    }
    value.append(old);
    p->env.insert(name, value);
}

bool qicRuntime::loadEnv(QString path)
{
    return p->loadEnv(path);
}

void qicRuntime::setQmake(QString path)
{
    p->qmake = path;
}

void qicRuntime::setMake(QString path)
{
    p->make = path;
}

void qicRuntime::setDefines(QStringList defines)
{
    p->defines = defines;
}

void qicRuntime::setIncludePath(QStringList dirs)
{
    p->include_path = dirs;
}

void qicRuntime::setLibs(QStringList libs)
{
    p->libs = libs;
}

void qicRuntime::setQtLibs(QStringList qtlibs)
{
    p->qtlibs = qtlibs;
}

void qicRuntime::setQtConfig(QStringList qtconf)
{
    p->qtconf = qtconf;
}

void qicRuntime::setOutputTo(QIODevice *device)
{
    p->output_sink = device;
}

void qicRuntime::setOutputToStdOut()
{
    p->output_sink = &p->fstdout;
}

qicContext *qicRuntime::ctx()
{
    return &p->ctx;
}

bool qicRuntime::compile(QString src)
{
    QElapsedTimer timer;
    timer.start();

    if (!p->dir.isValid()) {
        p->output("qicRuntime: Failed to create temp directory.");
        return false;
    }

    const int seq = p->seq();

    QString fncpp = QString("a%1.cpp").arg(seq);
    QFile fcpp(p->dir.filePath(fncpp));
    if (!fcpp.open(QIODevice::WriteOnly)) {
        p->output("qicRuntime: Failed to create temp source file.");
        return false;
    }
    {
        QTextStream tcpp(&fcpp);
        tcpp << src;
    }
    fcpp.close();

    QString fnpro = QString("a%1.pro").arg(seq);
    QFile fpro(p->dir.filePath(fnpro));
    if (!fpro.open(QIODevice::WriteOnly)) {
        p->output("qicRuntime: Failed to create temp project file.");
        return false;
    }
    {
        using Qt::endl;
        QTextStream tpro(&fpro);
        tpro << "TEMPLATE = lib" << endl;
        tpro << "QT = " << p->qtlibs.join(QChar(' ')) << endl;
        tpro << "CONFIG += " << p->qtconf.join(QChar(' ')) << endl;
        tpro << "SOURCES = " << fncpp << endl;
        for (QString def: p->defines) {
            tpro << "DEFINES += " << def << endl;
        }
        for (QString inc : p->include_path) {
            tpro << "INCLUDEPATH += " << inc << endl;
        }
        for (QString lib : p->libs) {
            tpro << "LIBS += " << lib << endl;
        }
    }
    fpro.close();

//    for (QString k : p->env.keys()) {
//        QString v = p->env.value(k);
//        p->output("[env]   %s=%s", qPrintable(k), qPrintable(v));
//    }

    if (!p->runProcess("[qmake] ", p->qmake, { fnpro })) {
        p->output("qicRuntime: Failed to generate Makefile.");
        return false;
    }

    if (!p->runProcess("[make]  ", p->make)) {
        p->output("qicRuntime: Build failed.");
        return false;
    }

    p->output("qicRuntime: Build finished in %g seconds.", (timer.elapsed() / 1000.0));
    return true;
}
