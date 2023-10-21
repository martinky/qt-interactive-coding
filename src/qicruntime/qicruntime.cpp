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
#include <QThread>
#include <QFileSystemWatcher>
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

    // Unload libs in destructor.
    bool unloadLibs = true;

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
                if (unloadLibs) {
                    fit->lib->unload();
                }
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
        frames.back().vars.push_back({ ptr, ::qstrdup(name), deleter });
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
    // additional flags
    bool autodebug = true;      // add "debug" to CONFIG automatically

    QFileSystemWatcher *watcher = nullptr;

    qicContextImpl ctx;

    qicRuntimePrivate()
    {
        env = QProcessEnvironment::systemEnvironment();

        qmake = "qmake"; // assume to be on PATH
#ifdef Q_OS_WIN
        make = "nmake";
#else
        make = "make";
#endif
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

    bool runProcess(QString fnlog, QString program, QStringList arguments = QStringList())
    {
        QString fplog = dir.filePath(fnlog);
        QProcess proc;
        proc.setWorkingDirectory(dir.path());
        proc.setProcessEnvironment(env);
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.setStandardOutputFile(fplog, QProcess::Append);
        proc.start(program, arguments);
        proc.waitForFinished();
        return proc.exitStatus() == QProcess::NormalExit &&
               proc.state()      == QProcess::NotRunning &&
               proc.exitCode()   == 0;
    }

    QString getLibPath() const
    {
#ifdef Q_OS_WIN
        QString libn = "bin/a%1.dll";
#else
        QString libn = "bin/a%1";
#endif
        return dir.filePath(libn.arg(seq()));
    }

    int seq() const
    {
        return (int)ctx.frames.size();
    }
};



qicRuntime::qicRuntime(QObject *parent) : QObject(parent),
    p(new qicRuntimePrivate)
{
}

qicRuntime::~qicRuntime()
{
    delete p;
}

void qicRuntime::setTempDir(QString path)
{
    p->dir = QTemporaryDir(path);
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
        qWarning("qicRuntime: Failed to load library %s: %s", qPrintable(lib_path), qPrintable(lib->errorString()));
        delete lib;
        return false;
    }

    // resolve entry point

    typedef void (*qic_entry_f)(qicContext *);
    qic_entry_f qic_entry = (qic_entry_f) lib->resolve("qic_entry");
    if (!qic_entry) {
        qWarning("qicRuntime: Failed to resolve qic_entry: %s", qPrintable(lib->errorString()));
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
        qWarning("qicRuntime: Failed to open source file: %s", qPrintable(filename));
        return false;
    }
    QTextStream t(&f);
    return exec(t.readAll());
}

bool qicRuntime::watchExecFile(QString filename, bool execNow)
{
    QFileInfo file(filename);
    if (!file.exists()) return false;

    QString absfn = file.canonicalFilePath();
    if (absfn.isEmpty()) return false;

    if (p->watcher == nullptr) {
        p->watcher = new QFileSystemWatcher(this);
        QObject::connect(p->watcher, &QFileSystemWatcher::fileChanged, this, [=](const QString &path){
            // Small workaround, because some editors save files in a "weird" way
            // that actually deletes and replaces the file and this confuses the
            // QFileSystemWatcher, which stops watching for subsequent changes.
            QThread::msleep(250);
            p->watcher->addPath(path);

            execFile(path);
        });
    }

    if (!p->watcher->addPath(absfn)) return false;

    if (execNow) {
        return execFile(absfn);
    }

    return true;
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

void qicRuntime::setIncludeDirs(QList<QDir> dirs)
{
    QStringList paths;
    for (const QDir &d : dirs) {
        paths += d.canonicalPath();
    }
    setIncludePath(paths);
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

void qicRuntime::setAutoDebug(bool enable)
{
    p->autodebug = enable;
}

void qicRuntime::setUnloadLibs(bool unload)
{
    p->ctx.unloadLibs = unload;
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
        qWarning("qicRuntime: Failed to create temp directory.");
        return false;
    }

    const int seq = p->seq();

    QString fncpp = QString("a%1.cpp").arg(seq);
    QFile fcpp(p->dir.filePath(fncpp));
    if (!fcpp.open(QIODevice::WriteOnly)) {
        qWarning("qicRuntime: Failed to create temp source file.");
        return false;
    }
    {
        QTextStream tcpp(&fcpp);
        tcpp << src;
    }
    fcpp.close();

    QString fnlog = QString("a%1.log").arg(seq);
    QString fnpro = QString("a%1.pro").arg(seq);
    QFile fpro(p->dir.filePath(fnpro));
    if (!fpro.open(QIODevice::WriteOnly)) {
        qWarning("qicRuntime: Failed to create temp project file.");
        return false;
    }
    {
        using Qt::endl;
        QTextStream tpro(&fpro);
        tpro << "TEMPLATE = lib" << endl;
        tpro << "QT = " << p->qtlibs.join(QChar(' ')) << endl;
        tpro << "CONFIG += " << p->qtconf.join(QChar(' ')) << endl;
#ifdef QT_DEBUG
        if (p->autodebug) {
            tpro << "CONFIG += debug" << endl;
        }
#endif
        tpro << "DESTDIR = bin" << endl;
        tpro << "SOURCES = " << fncpp << endl;
        for (const QString &def: p->defines) {
            tpro << "DEFINES += " << def << endl;
        }
        for (const QString &inc : p->include_path) {
            tpro << "INCLUDEPATH += " << inc << endl;
        }
        for (const QString &lib : p->libs) {
            tpro << "LIBS += " << lib << endl;
        }
    }
    fpro.close();

//    for (QString k : p->env.keys()) {
//        QString v = p->env.value(k);
//        qDebug("[env]   %s=%s", qPrintable(k), qPrintable(v));
//    }

    if (!p->runProcess(fnlog, p->qmake, { fnpro })) {
        qWarning("qicRuntime: Failed to generate Makefile. See log: %s", qPrintable(fnlog));
        return false;
    }

    if (!p->runProcess(fnlog, p->make)) {
        qWarning("qicRuntime: Build failed. See log: %s", qPrintable(fnlog));
        return false;
    }

    qDebug("qicRuntime: Build finished in %g seconds.", (timer.elapsed() / 1000.0));
    return true;
}
