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

#include "NSCustomPrecisionFormatter.h"
#include <sstream>
#include <iomanip>

@implementation NSCustomPrecisionFormatter

- (NSString *)stringForObjectValue:(id)obj {
    if (![obj isKindOfClass:[NSNumber class]]) {
        NSLog(@"CustomFormatter:stringForObjectValue:isKindOfClass failed");
        return nil;
    }
    
    uint32_t maxDecimal = (uint32_t)self.maximumFractionDigits;

    // Print value to a string
    float value = [obj  floatValue];
    std::stringstream ss;
    ss << std::fixed << std::setprecision(maxDecimal) << value;
    std::string str = ss.str();
    if (str.find('.') != std::string::npos)
    {
        // Remove trailing zeroes
        str = str.substr(0, str.find_last_not_of('0') + 1);
        // If the decimal point is now the last character, remove that as well
        if (str.find('.') == str.size() - 1)
        {
            str = str.substr(0, str.size() - 1);
        }
    }
    
    return [NSString stringWithFormat:@"%s", str.c_str()];
}

- (BOOL)getObjectValue:(id *)obj forString:(NSString *)string errorDescription:(NSString  **)error {
    float floatResult;
    BOOL result = NO;
    
    NSScanner *scanner = [NSScanner scannerWithString: string];
    if ([scanner scanFloat:&floatResult] && ([scanner isAtEnd])) {

        if (floatResult < self.minimum.doubleValue) {
            floatResult = self.minimum.doubleValue;
        } else if (floatResult > self.maximum.doubleValue) {
            floatResult = self.maximum.doubleValue;
        }

        if (obj) {
            *obj = [NSNumber numberWithFloat:floatResult];
        }

        result = YES;
    } else if (error) {
        *error = NSLocalizedString(@"Couldn’t convert to float", @"Error converting");
        NSLog(@"CustomFormatter:scanner failed for %@, error: %@", string, *error);
    }

    return result;
}

@end
