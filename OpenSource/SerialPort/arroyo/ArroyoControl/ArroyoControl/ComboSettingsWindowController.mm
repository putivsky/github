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

#include "ComboSettingsWindowController.h"
#include "FrameViewController.h"
#include "NSCustomPrecisionFormatter.h"
#include <string>
#include <map>

@interface ComboSettingsWindowController ()

@property (nonatomic, strong) IBOutlet NSTabView *tabView;
@property (nonatomic, strong) IBOutlet NSTabViewItem *tabLaserSettings;
@property (nonatomic, strong) IBOutlet NSTabViewItem *tabTECSettings;

@property (nonatomic, strong) IBOutlet NSButton *okButton;
@property (nonatomic, strong) IBOutlet NSButton *cancelButton;

@property (nonatomic, strong) IBOutlet NSButton *checkCurrentLimit;
@property (nonatomic, strong) IBOutlet NSButton *checkPDCurrentLimit;
@property (nonatomic, strong) IBOutlet NSButton *checkPDPowerLimit;
@property (nonatomic, strong) IBOutlet NSButton *checkOutOfTolerance;

@property (nonatomic, strong) IBOutlet NSTextField *cableRText;
@property (nonatomic, strong) IBOutlet NSTextField *pdResponseText;
@property (nonatomic, strong) IBOutlet NSTextField *pdBiasText;
@property (nonatomic, strong) IBOutlet NSTextField *tolTimeText;
@property (nonatomic, strong) IBOutlet NSTextField *tolIoText;
@property (nonatomic, strong) IBOutlet NSTextField *onDelayText;

@property (nonatomic, strong) IBOutlet NSTextField *ioLimitText;
@property (nonatomic, strong) IBOutlet NSTextField *vfLimitText;
@property (nonatomic, strong) IBOutlet NSTextField *imLimitText;
@property (nonatomic, strong) IBOutlet NSTextField *poLimitText;

@end

@implementation ComboSettingsWindowController

static const uint16_t kAlwaysEnabledBitsMask = 0x9192;// 1001 0001 1001 0010;
static const NSInteger kEncoding = NSUTF8StringEncoding;

-(void) setupFormatter:(NSTextField*)field digits:(NSInteger)digits {
    NSCustomPrecisionFormatter *formatter = [[NSCustomPrecisionFormatter alloc] init];
    [formatter setNumberStyle:NSNumberFormatterDecimalStyle];
    [formatter setMaximumFractionDigits:digits];
    [formatter setMinimumFractionDigits:0];
    [formatter setRoundingMode:NSNumberFormatterRoundDown];
    [formatter setUsesGroupingSeparator:FALSE];
    [formatter setMaximum:[[NSNumber alloc] initWithDouble:DBL_MAX]];
    [formatter setMinimum:[[NSNumber alloc] initWithDouble:0.0]];
    [field setFormatter:formatter];
}

- (void)windowDidLoad {
    [super windowDidLoad];
    
    switch (_activeTable) {
        case UNDEFINED:
            assert(false);
            break;
        case LASER:
            [_tabView removeTabViewItem:_tabTECSettings];
            break;
        case TEC:
             [_tabView removeTabViewItem:_tabLaserSettings];
            break;
        case COMBO:
            break;
    }

    if (_activeTable == LASER || _activeTable == COMBO) {
        [self setupFormatter:_cableRText digits:3];
        [self setupFormatter:_pdResponseText digits:3];
        [self setupFormatter:_pdBiasText digits:3];
        [self setupFormatter:_tolTimeText digits:0];
        [self setupFormatter:_tolIoText digits:0];
        [self setupFormatter:_onDelayText digits:0];
        
        [self setupFormatter:_ioLimitText digits:0];
        [self setupFormatter:_vfLimitText digits:3];
        [self setupFormatter:_imLimitText digits:0];
        [self setupFormatter:_poLimitText digits:0];
        
        std::string command = "LAS:ENAB:OUTOFF?;LAS:LIM:LDI?;LAS:LIM:MDI?;LAS:LIM:MDP?;LAS:LIM:LDV?;LAS:CABLER?;LAS:CALMD?;LAS:PDBIAS?;ONDELAY?;ERR?\r\n";
        
        std::string response, error;
        bool res = [self.frameController executeCommand:command response:&response error:error];
        NSLog(@"Executed command %s with error: %s", command.c_str(), error.c_str());
        if (!res) {
            [self.frameController writeLogEntry:error];
        } else {
            std::unordered_map<std::string, std::string> mapping = [self.frameController parseResponse:command response:response];
            if (mapping["ERR?\r\n"] == "0\r\n") {
                uint16_t bits = (uint16_t)std::stoul(mapping["LAS:ENAB:OUTOFF?"]);
                [_checkCurrentLimit setState:(bits & 0x1)];
                [_checkPDCurrentLimit setState:(bits & 0x4)];
                [_checkPDPowerLimit setState:(bits & 0x8)];
                [_checkOutOfTolerance setState:(bits & 0x200)];
                [_ioLimitText setStringValue:[NSString stringWithCString:mapping["LAS:LIM:LDI?"].data() encoding:kEncoding]];
                [_imLimitText setStringValue:[NSString stringWithCString:mapping["LAS:LIM:MDI?"].data() encoding:kEncoding]];
                [_poLimitText setStringValue:[NSString stringWithCString:mapping["LAS:LIM:MDP?"].data() encoding:kEncoding]];
                [_vfLimitText setStringValue:[NSString stringWithCString:mapping["LAS:LIM:LDV?"].data() encoding:kEncoding]];
                
                [_cableRText setStringValue:[NSString stringWithCString:mapping["LAS:CABLER?"].data() encoding:kEncoding]];
                [_pdBiasText setStringValue:[NSString stringWithCString:mapping["LAS:PDBIAS?"].data() encoding:kEncoding]];
                [_pdResponseText setStringValue:[NSString stringWithCString:mapping["LAS:CALMD?"].data() encoding:kEncoding]];
                [_onDelayText setStringValue:[NSString stringWithCString:mapping["ONDELAY?"].data() encoding:kEncoding]];
            }
        }
        
        command = "LAS:TOL?\n";
        res = [self.frameController executeCommand:command response:&response error:error];
        NSLog(@"Executed command %s with error: %s", command.c_str(), error.c_str());
        if (!res) {
            [self.frameController writeLogEntry:error];
        } else {
            std::size_t commaPos = response.find_first_of(",");
            std::size_t endOfLinePos;
            if (commaPos != std::string::npos
                && (endOfLinePos = response.find_first_of("\r\n")) != std::string::npos) {
                [_tolIoText setStringValue:[NSString stringWithCString:response.substr(0, commaPos).data() encoding:kEncoding]];
                [_tolTimeText setStringValue:[NSString stringWithCString:response.substr(commaPos + 1, endOfLinePos).data() encoding:kEncoding]];
            }
        }
    }
    
    if (_activeTable == TEC || _activeTable == COMBO) {
        // TODO
    }
 }

- (void)runModal {
    [[NSApplication sharedApplication] runModalForWindow:self.window];
}

- (void)windowWillClose:(NSNotification *)notification {
    [[NSApplication sharedApplication] stopModal];
}

- (IBAction)clickOkButton:(id)sender
{
    if (_activeTable == LASER || _activeTable == COMBO) {
        std::string command = "LAS:ENAB:OUTOFF ";
        
        uint16_t bits = kAlwaysEnabledBitsMask;
        if (_checkCurrentLimit.state) {
            bits |= 0x1;
        }
        if (_checkPDCurrentLimit.state) {
            bits |= 0x1;
        }
        if (_checkPDPowerLimit.state) {
            bits |= 0x8;
        }
        if (_checkOutOfTolerance.state) {
            bits |= 0x200;
        }
        
        command += std::to_string(bits);
        command += ";LAS:LIM:LDI ";
        command += std::string([_ioLimitText.stringValue UTF8String]);
        command += ";LAS:LIM:MDI ";
        command += std::string([_imLimitText.stringValue UTF8String]);
        command += ";LAS:LIM:MDP ";
        command += std::string([_poLimitText.stringValue UTF8String]);
        command += ";LAS:LIM:LDV ";
        command += std::string([_vfLimitText.stringValue UTF8String]);
        command += ";LAS:LIM:CABLER ";
        command += std::string([_cableRText.stringValue UTF8String]);
        command += ";LAS:LIM:PDBIAS ";
        command += std::string([_pdBiasText.stringValue UTF8String]);
        command += ";LAS:LIM:CALMD ";
        command += std::string([_pdResponseText.stringValue UTF8String]);
        command += ";LAS:LIM:ONDELAY ";
        command += std::string([_onDelayText.stringValue UTF8String]);
        command = ";LAS:TOL ";
        command += std::string([_tolIoText.stringValue UTF8String]);
        command += ",";
        command += std::string([_tolTimeText.stringValue UTF8String]);
        command = "\n";
        
        std::string error;
        bool res = [self.frameController executeCommand:command response:nil error:error];
        NSLog(@"Executed command %s with error: %s", command.c_str(), error.c_str());
        if (!res) {
            [self.frameController writeLogEntry:error];
        }
    }

    if (_activeTable == TEC || _activeTable == COMBO) {
        // TODO
    }
    
    [[self window] close];
}

- (IBAction)clickCancelButton:(id)sender
{
    [[self window] close];
}


@end
