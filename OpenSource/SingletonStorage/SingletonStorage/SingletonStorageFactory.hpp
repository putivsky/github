#include "SingletonStorageImpl.h"

namespace SM {

// Creates a static instance of the storage.
SingletonStorageImpl& getInstance() {
    static SingletonStorageImpl instance;
    return instance;
}

// Set of functions that delegate calls to the static instance of the storage.
template < typename T, typename ...ARGS >
std::weak_ptr<T> SingletonStorageFactory::create(ARGS&& ...args)
    noexcept( false ) {
    return getInstance().create<T>(std::forward<ARGS>(args)...);
}
    
template< typename T >
std::weak_ptr<T> SingletonStorageFactory::get() noexcept(true) {
    return getInstance().get<T>();
}
    
template< typename ...TS>
void SingletonStorageFactory::destroy() noexcept(true) {
    getInstance().destroy<TS...>();
}

void SingletonStorageFactory::clear() noexcept(true) {
    getInstance().clear();
}

void SingletonStorageFactory::reset() noexcept(true) {
    getInstance().reset();
}

}
