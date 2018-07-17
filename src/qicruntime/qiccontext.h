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
