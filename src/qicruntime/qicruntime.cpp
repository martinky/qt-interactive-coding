#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QElapsedTimer>
#include <QLibrary>
#include <QProcess>
#include "qicruntime.h"


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

    // As libs are compiled and loaded, they are appended to this list. The
    // last one is the current/last executed lib. Each lib can add variables
    // that will be persisted and available to subsequently compiled libs.
    // A lib registers a deleter with every variable. Variables defined by a
    // lib are destroyed before the lib is unloaded by calling the registered
    // deleter. Libs and variables are unloaded and destroyed in reverse order.
    std::vector<qicFrame> frames;
    // User-registered vars with the runtime. These are not deleted by the RT.
    std::vector<qicVar> globals;

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

    ~qicRuntimePrivate()
    {
        for (auto fit = frames.rbegin(); fit != frames.rend(); ++fit) {
            // unload libs in reverse order
            for (auto vit = fit->vars.rbegin(); vit != fit->vars.rend(); ++vit) {
                // destroy vars in reverse order
                ::free(vit->name);
                if (vit->deleter) {
                    vit->deleter(vit->ptr);
                }
            }
            fit->lib->unload();
            delete fit->lib;
        }
    }

    void *getVar(const char *name) const
    {
        // search lib frames and their variables in reverse order
        for (auto fit = frames.rbegin(); fit != frames.rend(); ++fit) {
            for (auto vit = fit->vars.rbegin(); vit != fit->vars.rend(); ++vit) {
                if (0 == ::strcmp(name, vit->name)) {
                    return vit->ptr;
                }
            }
        }

        // if not found, search globals in reverse order
        for (auto vit = globals.rbegin(); vit != globals.rend(); ++vit) {
            if (0 == ::strcmp(name, vit->name)) {
                return vit->ptr;
            }
        }

        return nullptr;
    }

    void *addVar(void *ptr, const char *name, void (*deleter)(void *))
    {
        if (frames.empty()) {
            globals.push_back({ ptr, ::strdup(name), deleter });
        } else {
            frames.back().vars.push_back({ ptr, ::strdup(name), deleter });
        }
        return ptr;
    }

    void loadEnv(QString path)
    {
        //env.clear();

        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning("qicRuntime: Failed to read env file %s.", qPrintable(path));
            return;
        }

        while (!f.atEnd()) {
            QString line = f.readLine().trimmed();
            int eq = line.indexOf(QChar('='));
            if (eq < 0) continue;
            QString name = line.mid(0, eq);
            QString value = line.mid(eq+1);
            env.insert(name, value);
        }
    }

    QString getLib() const
    {
#ifdef Q_OS_WIN
        QString libn = qtconf.contains("debug") ? "debug/a%1.dll" : "release/a%1.dll";
#else
        QString libn = "liba%1.so";
#endif
        return dir.filePath(libn.arg(seq()));
    }

    int seq() const
    {
        return (int)frames.size();
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

    QString lib_path = p->getLib();
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
    p->frames.push_back(frame);

    // execute

    qic_entry(this);

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

void qicRuntime::loadEnv(QString path)
{
    p->loadEnv(path);
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

void *qicRuntime::getVar(const char *name)
{
    return p->getVar(name);
}

void *qicRuntime::addVar(void *ptr, const char *name, void (*deleter)(void *))
{
    return p->addVar(ptr, name, deleter);
}

void qicRuntime::debug(const char *fmt, ...)
{
    char buff[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff, 1024, fmt, args);
    va_end(args);
    qDebug(buff);
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

    QString fnpro = QString("a%1.pro").arg(seq);
    QFile fpro(p->dir.filePath(fnpro));
    if (!fpro.open(QIODevice::WriteOnly)) {
        qWarning("qicRuntime: Failed to create temp project file.");
        return false;
    }
    {
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
//        qDebug("[env]   %s=%s", qPrintable(k), qPrintable(v));
//    }

    QProcess pqmake;
    pqmake.setWorkingDirectory(p->dir.path());
    pqmake.setProcessEnvironment(p->env);
    pqmake.start(p->qmake, { fnpro });
    pqmake.waitForFinished();
    while (!pqmake.atEnd()) {
        QByteArray s = pqmake.readLine().trimmed();
        qDebug("[qmake] %s", qPrintable(s));
    }
    if (    !pqmake.exitStatus() == QProcess::NormalExit ||
            !pqmake.state() == QProcess::NotRunning ||
            !pqmake.exitCode() == 0) {
        qWarning("qicRuntime: Failed to generate Makefile.");
        return false;
    }

    QProcess pmake;
    pmake.setWorkingDirectory(p->dir.path());
    pmake.setProcessEnvironment(p->env);
    pmake.start(p->make);
    pmake.waitForFinished();
    while (!pmake.atEnd()) {
        QByteArray s = pmake.readLine().trimmed();
        qDebug("[make]  %s", qPrintable(s));
    }
    if (    !pmake.exitStatus() == QProcess::NormalExit ||
            !pmake.state() == QProcess::NotRunning ||
            !pmake.exitCode() == 0) {
        qWarning("qicRuntime: Build failed.");
        return false;
    }

    qDebug("qicRuntime: Build finished in %g seconds.", (timer.elapsed() / 1000.0));
    return true;
}
