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

#include <utility>
#include <format>
#include <cxxabi.h>

namespace SM {

template < typename T, typename ...ARGS >
std::weak_ptr<T> SingletonStorageImpl::create(ARGS&&... args)
    noexcept( false ) {
    // read lock first
    // if object if not available
    // => write lock
    //  if object if not available again => create it,
    //  otherwise return existed object.    
    std::shared_lock<std::shared_mutex> rlock(mLock);
    auto wptr = find<T>(true /*disallow deleted*/);
    if (0 != wptr.use_count()) {
        return wptr;
    } 
    // Release the read lock.
    rlock.unlock();
    
    // Protect from the recursive calls for the same type from the same thread.
    // Enter the write (recursive) lock.
    // At this point only call from the same thread can pass through.
    std::lock_guard<std::recursive_mutex> reLock(mRecursiveLock);
    
    // Lock read mutex again.
    rlock.lock();
    // Create a new instance, but first check again.
    auto it = mFastAccess.find(makeKey<T>());
    if (it != mFastAccess.end()) {
        // Another thread between rlock.unlock() and reLock.lock() calls
        // can delete singleton instance of the same type.
        return find<T>(true /*disallow deleted*/);
    }
    // Release read lock again.
    rlock.unlock();
    
    auto itInsert = mRecursiveFlags.insert({makeKey<T>(), false});
    if (itInsert.first->second) {
        // The same thread tries to create the same type of singleton.
        std::string error = "Object [";
        error += demnagleType<T>();
        error += "] creation is in the recursion.";

        throw std::logic_error(error);
    }
    
    // Set flag.
    itInsert.first->second = true;
    
    // Reset flag even T(...) may throw an exception.
    ScopeGuard guard([&]() {     
        // Reset flag.
        mRecursiveFlags[makeKey<T>()] = false;
    });
    
    // At this point T(...) constructor can call this method again recursively.
    auto newObject = std::make_shared<T>(std::forward<ARGS>(args)...);

    // Create a new StorageObject<T> instance.
    std::unique_lock<std::shared_mutex> wlock(mLock);
    // Insert into the beginning,
    // making sure the destruction will be performed in a reverse order.
    mInstances.push_front(std::make_shared<StorageObject<T>>(newObject));
    // Add the new key for the singlton instance.
    mFastAccess.insert({makeKey<T>(), IteratorOptional(mInstances.begin())});
    
    return std::weak_ptr<T>(newObject);
}
    
template< typename T >
std::weak_ptr<T> SingletonStorageImpl::get() noexcept(true) {
    std::shared_lock rlock(mLock);
    return find<T>(false /*ignore deleted*/);
}

template< typename ...TS >
void SingletonStorageImpl::destroy() noexcept(true) {
    // Container for objects deletion outside lock.
    std::list<std::shared_ptr<VTObject>> objectsToBeDeleted;
    std::unique_lock<std::shared_mutex> wlock(mLock);
    auto func = [&](std::type_index idx) {
        auto itAccess = mFastAccess.find(idx);
        if (itAccess == mFastAccess.end() || !itAccess->second) {
            // The singleton of the specified type
            // doesn't exist or has been deleted.
            return;
        }
        // Copies iterator
        auto itInstances = itAccess->second.value();
        // Resets an optional value to empty and mark the deleted instance.
        itAccess->second = IteratorOptional();
        // Saves object to the container.
        objectsToBeDeleted.push_back(*itInstances);
        // Removes instance of singleton.
        mInstances.erase(itInstances);
        // But keeps the mFastAccess key
        // to track deleted objects.
    };
    
    // Creates the tuple of std::type_identity objects,
    // Then loops through such tuple object and invokes func
    // for each key made from the type.
    std::apply(
        [&](auto... ts) {
            (func(makeKey< typename decltype(ts)::type >()), ...);},
        std::make_tuple(std::type_identity<TS>()...)
     );
 
    // Releases the lock before destroying singleton objects.
    wlock.unlock();
    // Because singleton objects are wrapped by the shared_ptr
    // actual deletion will be done when the last weak_ptr
    // got released by the clients, obtained through
    // create or get calls.
    objectsToBeDeleted.clear();
}

void SingletonStorageImpl::clearUp(bool removeKeys) noexcept(true)
{
    std::list<std::shared_ptr<VTObject>> objectsToBeDeleted;
    std::unique_lock<std::shared_mutex> wlock(mLock);
    
    while (!mInstances.empty()) {
        // Because created objects were inserted into mInstances list
        // through push_front, i.e. C->B->A.
        // Deletion in a reverse order also from front to back.
        auto itInstances = mInstances.begin();
        // Pass reference to the object itself.
        auto itAccess = mFastAccess.find(makeKeyObject(**itInstances));
        assert(itAccess != mFastAccess.end());
        assert(itAccess->second);
        itAccess->second = IteratorOptional();
        
        objectsToBeDeleted.push_back(*itInstances);
        // Remove the current instance from the container.
        mInstances.pop_front();
    }
    
    if (removeKeys) {
        mFastAccess.clear();
    }
        
    // Releases the lock before destroying singleton objects.
    wlock.unlock();
    // Because singleton objects are wrapped by the shared_ptr
    // actual deletion will be done when the last weak_ptr
    // got released by the clients, obtained through
    // create or get calls.
    objectsToBeDeleted.clear();
}

void SingletonStorageImpl::clear() noexcept(true) {
    clearUp(false);
}

void SingletonStorageImpl::reset() noexcept(true) {
    clearUp(true);
}

template < typename T >
std::type_index SingletonStorageImpl::makeKey() noexcept(true) {
    return std::type_index(typeid(StorageObject<T>));
}

std::type_index SingletonStorageImpl::makeKeyObject(const VTObject& obj)
    noexcept(true) {
    return std::type_index(typeid(obj));
}

template< typename T >
std::string SingletonStorageImpl::demnagleType()
noexcept(true) {
    auto& info = typeid(T);
    int status = -1;
    char *readableName = abi::__cxa_demangle(info.name(), NULL, NULL, &status);
    std::string name(status == 0 ? readableName : info.name());
    ::free(readableName);
    return name;
}


template< typename T >
std::weak_ptr<T> SingletonStorageImpl::find(bool disallowDeleted)
    const noexcept(false) {
    auto itAccess = mFastAccess.find(makeKey<T>());
    if (itAccess != mFastAccess.end()) {
        // check iterator
        if (itAccess->second) {
            auto object = *itAccess->second.value();
            // check object
            assert(object != nullptr);
            return std::weak_ptr<T>(VTObject::cast<T>(object));
         } else if (disallowDeleted) {
            // Object has been destroyed already.
            std::string error = "Object [";
            error += demnagleType<T>();
            error += "] has been deleted";
            throw std::logic_error(std::move(error));
        }
    }
    return std::weak_ptr<T>();
}

}
