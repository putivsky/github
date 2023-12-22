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
    template < typename T, typename ...ARGS >
    static std::weak_ptr<T> create(ARGS&& ...args) noexcept(false);
    
    // If object has not been created yet or already been destroyed
    // method returns null weak_ptr,
    // otherwise returns weak_ptr to the singleton instance.
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
