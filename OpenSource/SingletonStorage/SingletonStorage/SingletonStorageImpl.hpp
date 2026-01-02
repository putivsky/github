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
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#else
#include <cxxabi.h>
#endif
#include <stdexcept>


namespace SM {

thread_local std::unordered_set<std::type_index> lRecursiveFlags;

template < typename T, typename ...ARGS >
std::weak_ptr<T> SingletonStorageImpl::create(ARGS&&... args)
    noexcept( false ) {
    // LOCK READ (shared) mutex first,
    // if object exists return it.
    std::shared_lock<std::shared_mutex> readLock(mLock);
    auto wptr = find<T>(true /*disallow deleted*/);
    if (0 != wptr.use_count()) {
        return wptr;
    } 
    readLock.unlock();
            
    // Protect from the recursive calls for the same type from the same thread.
    if (!lRecursiveFlags.insert(makeKey<T>()).second) {
        // The same thread tries to create the same type of singleton.
        std::string error = "Object [";
        error += demnagleType<T>();
        error += "] creation is in the recursion.";

        throw std::logic_error(error);
    }
        
    // Reset flag even in case T(...) may throw an exception.
    scope_exit guard([&]() {
        lRecursiveFlags.erase(makeKey<T>());
    });
            
    // Create a new std::shared_ptr<T> instance.
    // At this point T(...) constructor can call this method again recursively,
    // for the different singleton types it's allowed,
    // for the same singleton type exception will be thrown.
    auto newObject = std::make_shared<T>(std::forward<ARGS>(args)...);

    // LOCK WRITE (shared) mutex.
    std::unique_lock<std::shared_mutex> writeLock(mLock);
    // Try to insert,
    // if failed it means another thread could already create
    // the correspondent instance and even destroy it.
    auto itAccess = mFastAccess.insert({makeKey<T>(), IteratorOptional()});
    auto& refOptional = itAccess.first->second;
    if (itAccess.second) {
       // Insert into the beginning,
       // making sure the destruction will be performed in a reverse order.
       mInstances.push_front(std::make_shared<StorageObject<T>>(newObject));
       // update mFastAccess
        refOptional = IteratorOptional(mInstances.begin());
    } else if (!itAccess.first->second) {
       // Object has been destroyed already.
       std::string error = "Object [";
       error += demnagleType<T>();
       error += "] has been deleted";
       throw std::logic_error(std::move(error));
    }
        
    return std::weak_ptr<T>((*refOptional.value())->template cast<T>());
}
    
template< typename T >
std::weak_ptr<T> SingletonStorageImpl::get() noexcept(true) {
    std::shared_lock readlock(mLock);
    return find<T>(false /*ignore deleted*/);
}

template< typename T >
void SingletonStorageImpl::deleteType(StorageType& objectsToBeDeleted) {
    auto itAccess = mFastAccess.find(makeKey<T>());
    if (itAccess == mFastAccess.end() || !itAccess->second) {
        // The singleton of the specified type
        // doesn't exist or has been deleted.
        return;
    }
    // Copies iterator
    auto itInstances = itAccess->second.value();
    // Saves object to the container.
    objectsToBeDeleted.push_back(*itInstances);
    // Removes instance of object.
    // But keeps the mFastAccess key
    // to track deleted objects.
    mInstances.erase(itInstances);
    // Resets an optional value to empty and mark the deleted instance.
    itAccess->second = IteratorOptional();
}

template< typename ...TS >
void SingletonStorageImpl::destroy() noexcept(true) {
    // Container for objects deletion outside lock.
    std::list<std::shared_ptr<VTObject>> objectsToBeDeleted;
    // Lock the access
    std::unique_lock<std::shared_mutex> writeLock(mLock);
    (deleteType<TS>(objectsToBeDeleted), ...);
    // Releases the lock before destroying singleton objects.
    writeLock.unlock();
    // Because singleton objects are wrapped by the shared_ptr
    // actual deletion will be done when the last weak_ptr
    // got released by the clients, obtained through
    // create or get calls.
    objectsToBeDeleted.clear();
}

void SingletonStorageImpl::clearUp(bool removeKeys) noexcept(true)
{
    std::list<std::shared_ptr<VTObject>> objectsToBeDeleted;
    std::unique_lock<std::shared_mutex> writeLock(mLock);
    
    while (!mInstances.empty()) {
        // Because the created objects were inserted into mInstances list
        // through push_front, i.e. list looks like C->B->A.
        // Deletion in a reverse order also from front to back.
        auto itInstances = mInstances.begin();
        // Object must be valid at this point
        assert(*itInstances);
        // Save the object for the deletion
        objectsToBeDeleted.push_back(*itInstances);
         // Pass reference to the object itself.
        auto itAccess = mFastAccess.find(makeKeyObject(**itInstances));
        assert(itAccess != mFastAccess.end());
        assert(itAccess->second);
        itAccess->second = IteratorOptional();
        
       // Remove the current instance from the container.
        mInstances.pop_front();
    }
    
    if (removeKeys) {
        mFastAccess.clear();
    }
        
    // Releases the lock before destroying singleton objects.
    writeLock.unlock();
    // Because singleton objects are wrapped by the shared_ptr
    // actual deletion will be done when the last weak_ptr
    // got released by the clients, obtained through
    // create or get calls.
    objectsToBeDeleted.clear();
}

void SingletonStorageImpl::clear() noexcept(true) {
    clearUp(false);
    assert(lRecursiveFlags.empty());
}

void SingletonStorageImpl::reset() noexcept(true) {
    clearUp(true);
    assert(lRecursiveFlags.empty());
}

template< typename T >
std::string SingletonStorageImpl::demnagleType() noexcept(true) {
    auto& info = typeid(T);
    int status = -1;
    const char* decoratedName = info.name();

#ifdef _WIN32
    char* readableName = (char*)malloc(1024 * sizeof(char));
    DWORD processed = ::UnDecorateSymbolName(decoratedName, readableName, 1024, 0);
    readableName[processed] = 0;
    status = processed > 0 ? 0 : -1;
#else
    char *readableName = abi::__cxa_demangle(decoratedName, NULL, NULL, &status);
#endif
    std::string name(status == 0 ? readableName : decoratedName);
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
            return std::weak_ptr<T>(object->template cast<T>());
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
