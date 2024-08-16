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

#include "TECPanelViewController.h"
#include "ComboSettingsWindowController.h"
#include "FrameViewController.h"

@interface TECPanelViewController ()

@property (nonatomic, strong) IBOutlet NSButton *tModeGroup;
@property (nonatomic, strong) IBOutlet NSButton *rModeGroup;
@property (nonatomic, strong) IBOutlet NSButton *iteModeGroup;

@end

@implementation TECPanelViewController

- (void)setConnected:(BOOL)state {
    [super setConnected:state];
    _tModeGroup.enabled = state;
    _rModeGroup.enabled = state;
    _iteModeGroup.enabled = state;
}

- (IBAction)clickOutputGroup:(id)sender
{
    NSLog(@"clickOutputGroup tag %ld", ((NSButton*)sender).tag);
    // LAS:OUT 0
    // LAS:OUT 1
}

- (IBAction)clickModeGroup:(id)sender
{
    NSInteger tag = ((NSButton*)sender).tag;
    NSLog(@"clickModeGroup tag %ld", tag);
    std::string command;
    switch (tag) {
        case 1: // T
            command = ":TEC:MODE:T;ERR?\n";
            break;
        case 2: // R
            command = ":TEC:MODE:R;ERR?\n";
            break;
        case 3: // ITE
            command = ":TEC:MODE:ITE;ERR?\n";
            break;
        default:
            NSLog(@"Uknown mode group change: %@, tag: %ld", self.className, tag);
            return;
    }
        
    NSLog(@"Got mode group change: %@, tag: %ld", self.className, tag);

    std::string error;
    bool res = [self.frameController executeCommand:command response:nil error:error];
    NSLog(@"Executed command %s with error: %s", command.c_str(), error.c_str());
    if (!res) {
        [self.frameController writeLogEntry:error];
    } else {
        command = "ERRSTR?\r\n";
        std::string response;
        res = [self.frameController executeCommand:command response:&response error:error];
        if (!res) {
            [self.frameController writeLogEntry:error];
        } else if (response != "0\r\n") {
            [self.frameController writeLogEntry:response];
        }
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self setConnected:FALSE];
}

-(void) onQueryTimer:(NSTimer *)timer {
    std::string command = ":TEC:R?;T?;ITE?;V?;OUT?;MODE?;:TEC:SET:R?;T?;ITE?;ERR?\r\n";
    std::string error;
    std::string response;
    [self.frameController executeCommand:command response:&response error:error];

    // Parse response
    std::unordered_map<std::string, std::string> mapping = [self.frameController parseResponse:command response:response];
    std::string mode = mapping["MODE?"];
    
    NSLog(@"Executed command %s with response code: %s", command.c_str(), response.c_str());
    std::string outValue = mapping["OUT?"];
    if (outValue == "0") {
        [super.offOutputGroup setState:1];
    } else if (outValue == "1") {
        [super.onOutputGroup setState:1];
    }

    NSLog(@"Executed command %s with response code: %s", command.c_str(), response.c_str());
}

-(void) showCommonSettings {
    ComboSettingsWindowController* comboSettings = [[ComboSettingsWindowController alloc] initWithWindowNibName:@"ComboSettingsWindowController"];
    
    [comboSettings setFrameController:self.frameController];
    [comboSettings setActiveTable:TEC];
    
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
