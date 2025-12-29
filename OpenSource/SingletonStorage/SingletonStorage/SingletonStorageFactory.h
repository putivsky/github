/*
 * The Software License
 * =================================================================================
 * Copyright (c) 2003-2024 The Terimber Corporation. All rights reserved.
 * =================================================================================
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * The end-user documentation included with the redistribution, if any,
 * must include the following acknowledgment:
 * "This product includes software developed by the Terimber Corporation."
 * =================================================================================
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE TERIMBER CORPORATION OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ================================================================================
*/

#pragma once

// Singleton objects stored at one place (Singleton Storage)
// that manages the creation and destruction of the singleton objects
// in the pre-defined order.
// A singleton instance goes through states:
// 1. Non-exists yet
// 2. Created
// 3. Destroyed
// The attempt to create a second instance
// of the already destroyed singleton type
// will throw the correspondent exception,
// re-creation of the singleton instances is not allowed.

// Also the recursive creation of singletons is not allowed, example:
// A::A() { SingletonStorageFactory::create<B>(); }
// B::B() { SingletonStorageFactory::create<A>(); }

// At the same time the chain creation is allowed, example:
// A::A() { SingletonStorageFactory<B>(); } 
// B::B() { SingletonStorageFactory<C>(); } 
// C::C() {}
// SingletonStorageFactory::create<A>(); -> creates objects in order C, B, A.

#include <memory>

namespace SM {

class SingletonStorageFactory {
  public:
    // Method returns the newly created object or returns the existed one.
    // If the instance of the specified type has been already destroyed
    // then method throws an exception.
    // ARGS must match the T(...) constructor parameters.
    // Constructor can throw an exception as well.
    // Caller must lock weak_ptr before accessing the singleton object,
    // it can be null because other thread has destoyed the object.
    template < typename T, typename ...ARGS >
    static std::weak_ptr<T> create(ARGS&& ...args) noexcept(false);
    
    // If object has not been created yet or already been destroyed
    // method returns null weak_ptr,
    // otherwise returns weak_ptr to the singleton instance.
    // Caller must lock weak_ptr before accessing the singleton object,
    // it can be null because other thread has destoyed the object.
    template< typename T >
    static std::weak_ptr<T> get() noexcept(true);
    
    // Destroys the list of instances in the specified order by their types.
    // Non-exited types or types of already destroyed instances are ignored.
    template< typename ...TS>
    static void destroy() noexcept(true);

    // Destroys all instances in the reverse order they have been created.
    // Storage will keep the track of the previously created objects.
    // Re-creation of destroyed objects is not allowed.
    static void clear() noexcept(true);

    // Destroys all instances in the reverse order they have been created.
    // Sets storage into the initial state, useful for unit tests.
    // Re-creation of destroyed objects is allowed.
    static void reset() noexcept(true);
};

}

#include "SingletonStorageFactory.hpp"
