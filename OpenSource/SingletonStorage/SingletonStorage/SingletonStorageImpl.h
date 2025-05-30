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

#include <memory>
#include <shared_mutex>
#include <mutex>
#include <list>
#include <map>
#include <unordered_map>
#include <typeindex>
#include <optional>
#include <type_traits>
#include <assert.h>

namespace SM {

class SingletonStorageImpl
{
    // Base class for casting to the derived type.
    struct VTObject {
        virtual ~VTObject() = default;
        template< typename T >
        static std::shared_ptr<T> cast(std::shared_ptr<VTObject>& obj) {
            // Convert to StorageObject<T> first
            auto stObject = dynamic_cast<StorageObject<T>*>(obj.get());
            assert(stObject != nullptr);
            return stObject->mObject;
        }
    };

    // Base class for casting to the derived type.
    template< typename T >
    struct StorageObject : VTObject {
        StorageObject(std::shared_ptr<T> ptr) : mObject(ptr) {}
        std::shared_ptr<T> mObject;
    };
    
    // Simple scope guard
    template < typename T >
    struct ScopeGuard {
        ScopeGuard(T&& f) : mF(f) {}
        ~ScopeGuard() { mF(); }
    
    private:
       T mF;
    };
    
public:
    // See methods description at SingletonStorageFactory.hpp
    template < typename T, typename ...ARGS >
    std::weak_ptr<T> create(ARGS&& ...args) noexcept( false );
    template< typename T >
    std::weak_ptr<T> get() noexcept(true);
    template< typename ...TS>
    void destroy() noexcept(true);
    void clear() noexcept(true);
    void reset() noexcept(true);
    
private:
    // Helper methods.
    // Finds the singleton instance by the specified type.
    // Thows an exception if object has been deleted already.
    template< typename T >
    std::weak_ptr<T> find(bool disallowDeleted) const noexcept(false);
    
    // key => std::type_index(typeid(StorageObject<T>))
    // Construct the unique key by the specified type.
    template < typename T >
    static std::type_index makeKey() noexcept(true);
    // Construct the unique key for the specified object,
    // even object got cast the base class (RTTI).
    static std::type_index makeKeyObject(const VTObject& obj) noexcept(true);
    
    // Outputs the readable name for the provided type.
    template< typename T >
    static std::string demnagleType() noexcept(true);
    
    // Clear the storage.
    // @removeKeys indicates if the keys of deleted types
    // will be removed as well.
    void clearUp(bool removeKeys) noexcept(true);

private:
    // Protects access to the singleton instances,
    // multiple readers, single writer access.
    std::shared_mutex mLock;
    
    // Keeps the list of std::shared_ptr<StorageObject<T>> objects
    // for different types, casted to the base class VTObject.
    using StorageType = std::list<std::shared_ptr<VTObject>>;
    StorageType mInstances;
    
    // Maps unique keys of different types to the iterators
    // pointing to the corespndent singleton instances.
    // Once the specific insatance got deleted, valid iterator
    // got replaced by empty std::optional,
    // and signals the deletion of the specified singleton instance.
    using IteratorOptional = std::optional<StorageType::iterator>;
    std::unordered_map<std::type_index, IteratorOptional> mFastAccess;
    
    // States:
// 1. Non-exist yet -> there is no a correspondent entry,
//  i.e. mFastAccess.count(typeid(T)) == 0
// 2. Created -> there is the correspondent entries
//  in mFastAccess and mInstances,
//  i.e. auto it = mFastAccess.find(makeKey<T>());
//      it != mFastAccess.end() &&
//      it->second == true &&
//      it->second.value() != nullptr
// 3. Destroyed -> there is the correspondent entry only in mFastAccess,
//  i.e. auto it = mFastAccess.find(makeKey<T>());
//      it != mFastAccess.end() &&
//      it->second == false

    // Protection for the recursive calls for the same type.
    std::recursive_mutex mRecursiveLock;
    std::unordered_map<std::type_index, bool> mRecursiveFlags;
};

}

#include "SingletonStorageImpl.hpp"
