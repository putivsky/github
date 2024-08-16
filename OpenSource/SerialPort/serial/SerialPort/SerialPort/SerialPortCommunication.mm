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

#include "SerialPortCommunication.h"
#include <sstream>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFString.h>
#include <Foundation/NSDictionary.h>
#include <Foundation/NSValue.h>
#include <Foundation/NSData.h>
#include <Foundation/NSArray.h>
#include <Foundation/NSString.h>

#import <IOKit/IOKitLib.h>
#import <IOKit/serial/IOSerialKeys.h>

namespace Arroyo {

uint8_t propertyToUInt8(id value) {
    if (value == nil)
        return 0;
    
    if ([value isKindOfClass:[NSNumber class]])
        return [value unsignedCharValue];
    else if ([value isKindOfClass:[NSData class]])
    {
        NSData *data = (NSData *)value;
        uint8_t retVal = 0;
        
        memcpy(&retVal, data.bytes, MIN(data.length, 1));
        
        return retVal;
    }
    
    return 0;
}

bool getIORegParent(io_service_t device, NSString *name, io_service_t *foundDevice, bool recursive)
{
    io_iterator_t parentIterator;
    kern_return_t kr = IORegistryEntryCreateIterator(device, kIOServicePlane, (recursive ? kIORegistryIterateRecursively : 0) | kIORegistryIterateParents, &parentIterator);
    
    if (kr != KERN_SUCCESS)
        return false;
    
    for (io_service_t parentDevice; IOIteratorIsValid(parentIterator) && (parentDevice = IOIteratorNext(parentIterator)); IOObjectRelease(parentDevice))
    {
        if (IOObjectConformsTo(parentDevice, [name UTF8String]))
        {
            *foundDevice = parentDevice;
            
            IOObjectRelease(parentIterator);
            
            return true;
        }
    }
    
    return false;
}

SerialPort::SerialPort() {
}

SerialPort::~SerialPort() {
    Close();
}

std::map<uint8_t, std::string> SerialPort::enumerateSerialPorts() {
    std::map<uint8_t, std::string> result;
        
    // Serial devices are instances of class IOSerialBSDClient.
    // Create a matching dictionary to find those instances.
    CFMutableDictionaryRef classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch == NULL) {
        NSLog(@"IOServiceMatching returned a NULL dictionary.");
        return result;
    }

    // Look for devices that claim to be modems.
    CFDictionarySetValue(classesToMatch,
                         CFSTR(kIOSerialBSDTypeKey),
                         CFSTR(kIOSerialBSDRS232Type));

    io_iterator_t iteratorService;
    kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, classesToMatch, &iteratorService);
    if (KERN_SUCCESS != kr) {
        NSLog(@"IOServiceGetMatchingServices returned %d", kr);
        return result;
    }
    
    uint8_t portCounter = 0;
    
    for (io_service_t usbService;
         IOIteratorIsValid(iteratorService) && (usbService = IOIteratorNext(iteratorService));
         IOObjectRelease(usbService)) {
        // Get the callout device's path (/dev/cu.xxxxx). The callout device should almost always be
        // used: the dialin device (/dev/tty.xxxxx) would be used when monitoring a serial port for
        // incoming calls, e.g. a fax listener.
        CFTypeRef pathAsCFType = IORegistryEntryCreateCFProperty(usbService,
                                                            CFSTR(kIOCalloutDeviceKey),
                                                            kCFAllocatorDefault,
                                                            0);
        if (pathAsCFType) {
            // Convert the path from a CFString to a C (NUL-terminated) string for use
            // with the POSIX open() call.
            char bsdPath[MAXPATHLEN];
            CFStringRef pathAsCFString = (CFStringRef)pathAsCFType;
            
            Boolean resConvert = CFStringGetCString(pathAsCFString,
                                        bsdPath,
                                        sizeof(bsdPath),
                                        kCFStringEncodingUTF8);

            CFRelease(pathAsCFType);
                        
            if (resConvert) {
                // name should be like /dev/cu.usbserial-*
                constexpr const char* kSerialPrefix = "/dev/cu.usbserial-";
                std::string fullPath(bsdPath);
                if (fullPath.starts_with(kSerialPrefix)) {
                    uint8_t portNum = 0;
                    // Find and assign some numeric port as a key
                    io_service_t portService;
                    if (getIORegParent(usbService, @"AppleUSBHostPort", &portService, true)) {
                        if (kr == KERN_SUCCESS)
                        {
                            CFMutableDictionaryRef usbPortPropertyDictionaryRef = 0;
                            kr = IORegistryEntryCreateCFProperties(portService, &usbPortPropertyDictionaryRef, kCFAllocatorDefault, kNilOptions);
                            
                            if (kr == KERN_SUCCESS) {
                                NSMutableDictionary *usbPortPropertyDictionary = (__bridge NSMutableDictionary *)usbPortPropertyDictionaryRef;
                                portNum = propertyToUInt8([usbPortPropertyDictionary objectForKey:@"port"]);
                            }
                            
                            CFRelease(usbPortPropertyDictionaryRef);
                        }
                    }
                    
                    if (portNum) {
                        result.emplace(portNum, std::move(fullPath));
                    } else {
                        result.emplace(++portCounter, std::move(fullPath));
                    }
                }
            }
        }
    }
    
    return result;
}

bool SerialPort::Open(const std::string& portName, std::string& error, int speed) {
    Close();
    
    m_DeviceHandle = ::open(portName.c_str(), O_RDWR | O_NOCTTY);
    if (m_DeviceHandle == -1) {
        NSLog(@"Error open for %s -  %s (%d)\n", portName.c_str(), strerror(errno), errno);
        return false;
    }
    
    // Note that open() follows POSIX semantics: multiple open() calls to the same file will succeed
    // unless the TIOCEXCL ioctl is issued. This will prevent additional opens except by root-owned
    // processes.
    // See tty(4) <x-man-page//4/tty> and ioctl(2) <x-man-page//2/ioctl> for details.
    if (::ioctl(m_DeviceHandle, TIOCEXCL) == -1) {
        NSLog(@"Error setting TIOCEXCL %s -  %s (%d)\n", portName.c_str(), strerror(errno), errno);
        Close();
        return false;
    }
    
    // Now that the device is open, clear the O_NONBLOCK flag so subsequent I/O will block.
    // See fcntl(2) <x-man-page//2/fcntl> for details.
    
    if (::fcntl(m_DeviceHandle, F_SETFL, 0) == -1) {
        NSLog(@"Error clearing O_NONBLOCK %s -  %s (%d)\n", portName.c_str(), strerror(errno), errno);
        Close();
        return false;
    }
    
    // Get the current options and save them so we can restore the default settings later.
    if (::tcgetattr(m_DeviceHandle, &m_originOptions) == -1) {
        NSLog(@"Error getting tty attributes %s -  %s (%d)\n", portName.c_str(), strerror(errno), errno);
        Close();
        return false;
    }
    
    // The serial port attributes such as timeouts and baud rate are set by modifying the termios
    // structure and then calling tcsetattr() to cause the changes to take effect. Note that the
    // changes will not become effective without the tcsetattr() call.
    // See tcsetattr(4) <x-man-page://4/tcsetattr> for details.
    
    struct termios currentOptions;
    currentOptions = m_originOptions;
    ::cfmakeraw(&currentOptions);
    
    // Arroy manual
    /*
     Baud Rate 38400
     Parity None
     Data Bits 8
     Stop Bits 1
     Flow Control None
     */
    ::cfsetispeed(&currentOptions, speed);//38400); //9600
    ::cfsetospeed(&currentOptions, speed);
    currentOptions.c_cflag &= ~CSIZE;
    currentOptions.c_cflag |= CS8; // 8 bits
    currentOptions.c_cflag &= ~CSTOPB; // 1 stop bit
    currentOptions.c_cflag &= ~PARENB; // disable parity
    currentOptions.c_iflag &= ~INPCK; // no parity check
    currentOptions.c_iflag &= ~(CCTS_OFLOW | CRTS_IFLOW | CDTR_IFLOW | CDSR_OFLOW | CCAR_OFLOW); // disable flow control
    
    currentOptions.c_cc[VTIME] = 50; // 5 seconds timeout
    currentOptions.c_cc[VMIN] = 0; // zero bytes for read is acceptable
    ::tcsetattr(m_DeviceHandle, TCSANOW, &currentOptions);
    
    if (::tcflush(m_DeviceHandle, TCIOFLUSH) != 0) {
        NSLog(@"Error %s (%d) for function tcflush\n", strerror(errno), errno);
        Close();
        return false;
    }
    
    return true;
}
 
bool SerialPort::Write(const std::string& commnd, std::string& error) {
    if (m_DeviceHandle == -1) {
        NSLog(@"Device is not initialized\n");
        return false;
    }
    
    if (commnd.size() != ::write(m_DeviceHandle, commnd.data(), commnd.size())) {
        NSLog(@"Error %s (%d) for function write\n", strerror(errno), errno);
        return false;
    }
    
    return true;
}

bool SerialPort::Read(std::string* response, std::string& error) {
    error.clear();
    
    if (m_DeviceHandle == -1) {
        NSLog(@"Device is not initialized\n");
        return false;
    }
    
    int length;
    
    if (0 == ::ioctl(m_DeviceHandle, FIONREAD, &length) && 0 == length) {
        ::usleep(response ? 200000 : 50000);
    }
    
    if (-1 == ::ioctl(m_DeviceHandle, FIONREAD, &length)) {
        NSLog(@"Error %s (%d) for function ioctl\n", strerror(errno), errno);
        return false;
    }
    
    if (length > 0) {
        std::string tempBuffer;
        std::string* buffer = response != nullptr ? response : &tempBuffer;
        buffer->resize(length);
        if (length == ::read(m_DeviceHandle, buffer->data(), length)) {
            return true;
        }
    }
    
    if (response) {
        response->clear();
    }
    return true;
}

void SerialPort::Close() {
    if (m_DeviceHandle != -1) {
        std::string request = "LOCAL\n";
        std::string error;
        Write(request, error);
        std::string response;
        Read(nullptr, error);

        // Block until all written output has been sent from the device.
        // Note that this call is simply passed on to the serial device driver.
        // See tcsendbreak(3) <x-man-page://3/tcsendbreak> for details.
        if (::tcdrain(m_DeviceHandle) == -1) {
            NSLog(@"Error waiting for drain - %s(%d).\n", strerror(errno), errno);
        }

        // Traditionally it is good practice to reset a serial port back to
        // the state in which you found it. This is why the original termios struct
        // was saved.
        if (::tcsetattr(m_DeviceHandle, TCSANOW, &m_originOptions) == -1) {
            NSLog(@"Error resetting tty attributes - %s(%d).\n", strerror(errno), errno);
        }
        

        ::close(m_DeviceHandle);
        m_DeviceHandle = -1;
    }
}

}
