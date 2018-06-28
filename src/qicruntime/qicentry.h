#ifndef QICENTRY_H
#define QICENTRY_H

#ifdef _MSC_VER
#define QIC_DLL_EXPORT __declspec(dllexport)
#else
#define QIC_DLL_EXPORT
#endif

struct qicContext;

/**
    Entry point of the runtime-compiled library. The user has to generate the
    body of this function.
 */
extern "C" QIC_DLL_EXPORT void qic_entry(qicContext *ctx);

#endif // QICENTRY_H
