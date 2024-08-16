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
#include <stdio.h>
#include <map>
#include <string>
#include <termios.h>

namespace Arroyo {

class SerialPort {
public:
    static std::map<uint8_t, std::string> enumerateSerialPorts();
    
    SerialPort();
    ~SerialPort();
    
    bool Open(const std::string& portName, std::string& error, int speed = 9600);
    bool Write(const std::string& commnd, std::string& error);
    bool Read(std::string* response, std::string& error);
    void Close();
    
    bool isOpen() const { return m_DeviceHandle != -1; }
private:
    int m_DeviceHandle = -1;
    struct termios m_originOptions;
};

}
