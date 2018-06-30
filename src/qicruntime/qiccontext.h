#ifndef QICCONTEXT_H
#define QICCONTEXT_H

/**
    This pure virtual class serves as the context for communication between
    the host runtime - qicRuntime - and the runtime-compiled code. A pointer to
    qicContext is passed by the runtime to qic_entry(), the main function of
    the runtime-compiled code.

        extern "C" void qic_entry(qicContext *ctx);

    get()
        Retrieves a variable previously stored by set().

    set()
        Registers a variable with the context. This variable will be persisted
        and accessible to subsequent runtime-compiled code as well as to the
        host of qicRuntime. If a deleter function is provided, it will be used
        to dispose of the variable when the library that holds the code is
        unloaded. Do not set() pointers to local variables defined in the scope
        of qic_entry(). Globals and variables allocated on the heap (malloc,
        new) are OK.

    debug()
        Prints a debug message.
 */
struct qicContext
{
    virtual void *get(const char *name) = 0;
    virtual void *set(void *ptr, const char *name, void(*deleter)(void*)) = 0;

    virtual void debug(const char *fmt, ...) = 0;
};

#endif // QICCONTEXT_H
