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

#include "LaserPanelViewController.h"
#include "FrameViewController.h"
#include "SystemSettingsWindowController.h"
#include "ComboSettingsWindowController.h"
#include "QCWParametersWindowController.h"

@interface LaserPanelViewController ()

@property (nonatomic, strong) IBOutlet NSButton *ioModeGroup;
@property (nonatomic, strong) IBOutlet NSButton *ioHBWModeGroup;
@property (nonatomic, strong) IBOutlet NSButton *pulseModeGroup;
@property (nonatomic, strong) IBOutlet NSButton *extTrigModeGroup;
@property (nonatomic, strong) IBOutlet NSButton *burstModeGroup;
@property (nonatomic, strong) IBOutlet NSButton *imModeGroup;
@property (nonatomic, strong) IBOutlet NSButton *poModeGroup;
@property (nonatomic, strong) IBOutlet NSButton *vfModeGroup;
@property (nonatomic, strong) IBOutlet NSButton *qcwSettings;

@property (nonatomic, strong) IBOutlet NSTextField *ioLabel;
@property (nonatomic, strong) IBOutlet NSTextField *imLabel;
@property (nonatomic, strong) IBOutlet NSTextField *poLabel;
@property (nonatomic, strong) IBOutlet NSTextField *vfLabel;

@property (nonatomic, strong) IBOutlet NSTextField *ioTextField;
@property (nonatomic, strong) IBOutlet NSTextField *imTextField;
@property (nonatomic, strong) IBOutlet NSTextField *poTextField;
@property (nonatomic, strong) IBOutlet NSTextField *vfTextField;

@property (nonatomic, strong) IBOutlet NSTextField *ioUnits;
@property (nonatomic, strong) IBOutlet NSTextField *imUnits;
@property (nonatomic, strong) IBOutlet NSTextField *poUnits;
@property (nonatomic, strong) IBOutlet NSTextField *vfUnits;

@end

@implementation LaserPanelViewController

static const NSInteger kEncoding = NSUTF8StringEncoding;

static const struct LaserMode {
    const char* const name;
    const char* const value;
    const char* const units;
} sLaserModes[] =
{
    {"MODE", "MODE", nullptr}, // tag(0)
    {"LDI", "ILBW", "mA"}, // Io - tag(1)
    {"LDI", "IHBW", "mA"}, // Io HBW - tag(2)
    {"LDI", "PULSE", "mA"}, // Pulse - tag(3)
    {"LDI", "TRIG", "mA"}, // TRIG - tag(4)
    {"LDI", "BURST", "mA"}, // BURST - tag(5)
    {"MDI", "MDI", "μA"}, // Im - tag(6)
    {"MDP", "MDP", "mV"}, // Po - tag(7)
    {"LDV", "LDV", "WW"}, // Vf - tag(8)
};

static const int sLaserModeSize = sizeof(sLaserModes)/sizeof(*sLaserModes);

static const struct LaserOutput {
    const char* const value;
} sLaserOutputs[] =
{
    {"OUT"},
    {"OUT 1"},
    {"OUT 0"},
};

static const int sLaserOutputSize = sizeof(sLaserOutputs)/sizeof(*sLaserOutputs);

struct LaserSettings {
    float ldi;
    float mdi;
    float mdp;
    float ldv;
};

LaserSettings laserLimits;
LaserSettings laserSets;
LaserSettings laserCurrents;

- (void)setConnected:(BOOL)state {
    [super setConnected:state];
    _ioModeGroup.enabled = state;
    _ioHBWModeGroup.enabled = state;
    _imModeGroup.enabled = state;
    
    _poLabel.hidden = !state;
    _poTextField.hidden = !state;
    _poUnits.hidden = !state;

    _pulseModeGroup.hidden = !state;
    _extTrigModeGroup.hidden = !state;
    _burstModeGroup.hidden = !state;
    
    _poModeGroup.enabled = state;
    _vfModeGroup.enabled = state;
    _qcwSettings.enabled = state;
    
    if (state) {
        [self fillOutSets:FALSE];
    }
}

- (IBAction)clickOutputGroup:(id)sender
{
    NSInteger tag = ((NSButton*)sender).tag;
    NSLog(@"clickOutputGroup tag %ld", tag);
    if (tag >= sLaserOutputSize) {
        NSLog(@"Uknown mode group change: %@, tag: %ld", self.className, tag);
        return;
    }
    
    std::string command = ":LAS:";
    command += sLaserOutputs[tag].value;
    command += "\n"; // add teriminator
    
    std::string error;
    bool res = [self.frameController executeCommand:command response:nil error:error];
    NSLog(@"Executed command %s with error: %s", command.c_str(), error.c_str());
    if (!res) {
        [self.frameController writeLogEntry:error];
    } else {
        command = "ERR?\r\n";
        std::string response;
        res = [self.frameController executeCommand:command response:&response error:error];
        NSLog(@"Executed command %s with response %s, error: %s", command.c_str(), response.c_str(), error.c_str());
        if (!res) {
            [self.frameController writeLogEntry:error];
        } else if (response != "0\r\n") {
            [self.frameController writeLogEntry:[super errorDescriptionByCode:response]];
        }
    }
}

- (IBAction)clickModeGroup:(id)sender
{
    NSInteger tag = ((NSButton*)sender).tag;
    NSLog(@"clickModeGroup tag %ld", tag);
    if (tag >= sLaserModeSize) {
        NSLog(@"Uknown mode group change: %@, tag: %ld", self.className, tag);
        return;
    }
    
    std::string command = ":LAS:MODE:";
    command += sLaserModes[tag].value;
    command += "\n"; // add teriminator
    NSLog(@"Got mode group change: %@, tag: %ld", self.className, tag);

    std::string error;
    bool res = [self.frameController executeCommand:command response:nil error:error];
    NSLog(@"Executed command %s with error: %s", command.c_str(), error.c_str());
    if (!res) {
        [self.frameController writeLogEntry:error];
    } else {
        [self fillOutSets:FALSE];
    }
}

- (void) fillOutSets:(BOOL)onTimer {
    std::string command;
    for (int idx = 5; idx < sLaserModeSize; ++idx) {
        //  current value
        command += ":LAS:";
        command += sLaserModes[idx].name;
        command += "?;";
        if (!onTimer) {
            // set value
            command += ":LAS:SET:";
            command += sLaserModes[idx].name;
            command += "?;";
            // limit value
            command += ":LAS:LIM:";
            command += sLaserModes[idx].name;
            command += "?;";
        }
    }

    // add mode
    command += ":LAS:";
    command += sLaserModes[0].name;
    command += "?;";

    // add output
    command += ":LAS:";
    command += sLaserOutputs[0].value;
    command += "?;";

    // add error query
    command += "ERR?\n";
    
    std::string response, error;
    bool res = [self.frameController executeCommand:command response:&response error:error];
    NSLog(@"Executed command %s with response %s, error: %s", command.c_str(), response.c_str(), error.c_str());
    if (!res) {
        [self.frameController writeLogEntry:error];
    } else  {
        std::unordered_map<std::string, std::string> mapping = [self.frameController parseResponse:command response:response];
        std::string modeValue = mapping[":LAS:MODE?"];
        int index = 0;
        for (; index < sLaserModeSize; ++index) {
            if (modeValue == sLaserModes[index].value) {
                break;
            }
        }
        
        switch (index) {
            case 0:
                assert(false);
            case 1:
                [_ioModeGroup setState:1];
                break;
            case 2:
                [_ioHBWModeGroup setState:1];
                break;
            case 3:
                [_pulseModeGroup setState:1];
                break;
            case 4:
                [_extTrigModeGroup setState:1];
                break;
            case 5:
                [_burstModeGroup setState:1];
                break;
            case 6:
                [_imModeGroup setState:1];
                break;
            case 7:
                [_poModeGroup setState:1];
                break;
            case 8:
                [_vfModeGroup setState:1];
                break;
            default:
                NSLog(@"Executed command %s with response %s, error: %s", command.c_str(), response.c_str(), error.c_str());
                return;
                //assert(false);
        }
        
        _qcwSettings.enabled = (index == 3 || index == 4 || index == 5);

        std::string inputKey = ":LAS:SET:";
        inputKey += sLaserModes[index].name;
        inputKey += "?";
        
        std::string inputUnits = sLaserModes[index].units;
        if (mapping["ERR?\n"] != "0\r\n") {
            [self.frameController writeLogEntry:[super errorDescriptionByCode:mapping["ERR?\n"]]];
        }
        
        if (!onTimer) {
            [super.setUnits setStringValue:[NSString stringWithCString:inputUnits.c_str() encoding:kEncoding]];
            [super.setInput setStringValue:[NSString stringWithCString:mapping[inputKey].c_str() encoding:kEncoding]];

            laserLimits.ldi = std::stof(mapping[":LAS:LIM:LDI?"]);
            laserLimits.mdi = std::stof(mapping[":LAS:LIM:MDI?"]);
            laserLimits.mdp = std::stof(mapping[":LAS:LIM:MDP?"]);
            laserLimits.ldv = std::stof(mapping[":LAS:LIM:LDV?"]);

            laserSets.ldi = std::stof(mapping[":LAS:SET:LDI?"]);
            laserSets.mdi = std::stof(mapping[":LAS:SET:MDI?"]);
            laserSets.mdp = std::stof(mapping[":LAS:SET:MDP?"]);
            laserSets.ldv = std::stof(mapping[":LAS:SET:LDV?"]);
        }
        
        // current values
        [_ioTextField setStringValue:[NSString stringWithCString:mapping[":LAS:LDI?"].c_str() encoding:kEncoding]];
        [_imTextField setStringValue:[NSString stringWithCString:mapping[":LAS:MDI?"].c_str() encoding:kEncoding]];
        [_poTextField setStringValue:[NSString stringWithCString:mapping[":LAS:MDP?"].c_str() encoding:kEncoding]];
        [_vfTextField setStringValue:[NSString stringWithCString:mapping[":LAS:LDV?"].c_str() encoding:kEncoding]];

        laserCurrents.ldi = std::stof(mapping[":LAS:LDI?"]);
        laserCurrents.mdi = std::stof(mapping[":LAS:MDI?"]);
        laserCurrents.mdp = std::stof(mapping[":LAS:MDP?"]);
        laserCurrents.ldv = std::stof(mapping[":LAS:LDV?"]);

        std::string outValue = mapping[":LAS:OUT?"];
        if (outValue == "0") {
            [super.offOutputGroup setState:1];
        } else if (outValue == "1") {
            [super.onOutputGroup setState:1];
        }

        [super.setButton setTag:index];
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    if (super.comboMode) {
        [_qcwSettings setHidden:TRUE];
    }
    [self setConnected:FALSE];
}

-(void) onQueryTimer:(NSTimer *)timer {
    [ self fillOutSets:TRUE];

}

- (IBAction)clickSet:(id)sender {
    float value = super.setInput.floatValue;
    NSLog(@"clickSet, value: %f", value);
    NSInteger tag = super.setButton.tag;
    if (tag >= sLaserModeSize) {
        NSLog(@"Unknown mode group change: %@, tag: %ld", self.className, tag);
        return;
    }
    
    std::string command = ":LAS:";
    command += sLaserModes[tag].name;
    command += " ";
    command += std::to_string(value);
    command += "\n"; // add teriminator

    std::string error;
    if (![self.frameController executeCommand:command response:nil error:error]) {
        NSLog(@"Executed command %s with error code: %s", command.c_str(), error.c_str());
        return;
    }
    
    NSLog(@"Executed command %s", command.c_str());
    [self fillOutSets:FALSE];
}

- (IBAction)clickQCWSettings:(id)sender
{
    NSLog(@"clickQCWSettings");
    QCWParametersWindowController* qcwParametersSettings = [[QCWParametersWindowController alloc] initWithWindowNibName:@"QCWParametersWindowController"];
    
    [qcwParametersSettings setFrameController:self.frameController];
    [qcwParametersSettings setLaserMode:super.setButton.tag];
    
    NSRect mainFrame = [[[NSApplication sharedApplication] windows] firstObject].frame;
    NSSize mainSize = mainFrame.size;
    NSSize windowSize = [qcwParametersSettings window].frame.size;
    NSPoint pos;
    pos.x = mainFrame.origin.x + mainSize.width / 2 - windowSize.width / 2;
    pos.y = mainFrame.origin.y + mainSize.height / 2 - windowSize.height / 2;

    [qcwParametersSettings.window setFrame:CGRectMake(pos.x, pos.y,
                                               windowSize.width ,
                                               windowSize.height) display:NO];

    [qcwParametersSettings runModal];
}

-(void) showSystemSettings {
    SystemSettingsWindowController* systemSettings = [[SystemSettingsWindowController alloc] initWithWindowNibName:@"SystemSettingsWindowController"];
    
    [systemSettings setFrameController:self.frameController];
    
    NSRect mainFrame = [[[NSApplication sharedApplication] windows] firstObject].frame;
    NSSize mainSize = mainFrame.size;
    NSSize windowSize = [systemSettings window].frame.size;
    NSPoint pos;
    pos.x = mainFrame.origin.x + mainSize.width / 2 - windowSize.width / 2;
    pos.y = mainFrame.origin.y + mainSize.height / 2 - windowSize.height / 2;

    [systemSettings.window setFrame:CGRectMake(pos.x, pos.y,
                                               windowSize.width ,
                                               windowSize.height) display:NO];

    [systemSettings runModal];
}

-(void) showCommonSettings {
    ComboSettingsWindowController* comboSettings = [[ComboSettingsWindowController alloc] initWithWindowNibName:@"ComboSettingsWindowController"];
    
    [comboSettings setFrameController:self.frameController];
    [comboSettings setActiveTable:LASER];
    
    NSRect mainFrame = [[[NSApplication sharedApplication] windows] firstObject].frame;
    NSSize mainSize = mainFrame.size;
    NSSize windowSize = [comboSettings window].frame.size;
    NSPoint pos;
    pos.x = mainFrame.origin.x + mainSize.width / 2 - windowSize.width / 2;
    pos.y = mainFrame.origin.y + mainSize.height / 2 - windowSize.height / 2;

    [comboSettings.window setFrame:CGRectMake(pos.x, pos.y,
                                               windowSize.width ,
                                               windowSize.height) display:NO];

    [comboSettings runModal];
}

@end
