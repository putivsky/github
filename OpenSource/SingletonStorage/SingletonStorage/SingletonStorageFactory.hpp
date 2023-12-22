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
