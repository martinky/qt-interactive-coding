#ifndef QICCONTEXT_H
#define QICCONTEXT_H

/**
    \class qicContext
    The qicContext pure virtual class serves as the interface for communication
    between the host program and the runtime-compiled code. A pointer to
    qicContext is passed by the runtime to qic_entry(), the main function of
    the runtime-compiled code.

    \fn qicContext::get()
    Retrieves an object previously stored by set().

    \fn qicContext::set()
    Registers an object with the context. This object will be accessible to
    subsequent runtime-compiled code as well as to the user of qicRuntime. If
    \a deleter function is provided, it will be used to dispose of the object
    when the library that holds the code is unloaded. Never pass pointers to
    local variables to set().

    \fn qicContext::debug()
    Prints a debug message.
 */
struct qicContext
{
    virtual void *get(const char *name) = 0;
    virtual void *set(void *ptr, const char *name, void(*deleter)(void*) = nullptr) = 0;

    virtual void debug(const char *fmt, ...) = 0;
};

#endif // QICCONTEXT_H
