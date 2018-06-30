#ifndef QICENTRY_H
#define QICENTRY_H

#ifdef _MSC_VER
#define QIC_DLL_EXPORT __declspec(dllexport)
#else
#define QIC_DLL_EXPORT
#endif

struct qicContext;

/**
    \file qicentry.h

    \fn void qic_entry(qicContext *ctx)
    Entry point exported by the runtime-compiled library. The user code must
    define and export this function.

        extern "C" void qic_entry(qicContext *ctx);

    Once the user code is successfully compiled, the qicRuntime loads the
    library, resolves and then calls this function.
 */
extern "C" QIC_DLL_EXPORT void qic_entry(qicContext *ctx);

#endif // QICENTRY_H
