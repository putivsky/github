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

#include "SystemSettingsWindowController.h"
#include "FrameViewController.h"
#include "TerminalDialogWindowController.h"

@interface SystemSettingsWindowController ()

@property (nonatomic, strong) IBOutlet NSTextField *modelField;
@property (nonatomic, strong) IBOutlet NSTextField *snField;
@property (nonatomic, strong) IBOutlet NSTextField *calDateField;
@property (nonatomic, strong) IBOutlet NSTextField *firmwareField;
@property (nonatomic, strong) IBOutlet NSTextField *bnField;

@property (nonatomic, strong) IBOutlet NSTextField *messageStore;
@property (nonatomic, strong) IBOutlet NSButton *checkAllow;
@property (nonatomic, strong) IBOutlet NSButton *terminalButton;
@property (nonatomic, strong) IBOutlet NSButton *okButton;
@property (nonatomic, strong) IBOutlet NSButton *cancelButton;

@end

@implementation SystemSettingsWindowController

static const NSInteger kEncoding = NSUTF8StringEncoding;

- (void)windowDidLoad {
    [super windowDidLoad];

    // request system info
    std::string command = "MODEL?;SN?;CALDATE?;VER?;BUILD?;REMSET?;ERR?\n";
    std::string error, response;
    bool res = [_frameController executeCommand:command response:&response error:error];

    NSLog(@"Executed command %s with response: %s, error: %s", command.c_str(), response.c_str(), error.c_str());
    if (res) {
        std::unordered_map<std::string, std::string> mapping = [_frameController parseResponse:command response:response];
        
        if (mapping["ERR?\n"] == "0\r\n") {
            // request message separately
            command = "MES?\n";
            res = [_frameController executeCommand:command response:&response error:error];
            NSLog(@"Executed command %s with response: %s, error: %s", command.c_str(), response.c_str(), error.c_str());
            std::string message;
            size_t pos = response.find_last_of("\r\n");
            if (pos != std::string::npos) {
                response.resize(pos);
            }
            message = response;
            
            int check = std::stoi(mapping["REMSET?"]);
            [_modelField setStringValue:[NSString stringWithCString:mapping["MODEL?"].data() encoding:kEncoding]];
            [_snField setStringValue:[NSString stringWithCString:mapping["SN?"].data() encoding:kEncoding]];
            [_calDateField setStringValue:[NSString stringWithCString:mapping["CALDATE?"].data() encoding:kEncoding]];
            [_firmwareField setStringValue:[NSString stringWithCString:mapping["VER?"].data() encoding:kEncoding]];
            [_bnField setStringValue:[NSString stringWithCString:mapping["BUILD?"].data() encoding:kEncoding]];
            [_messageStore setStringValue:[NSString stringWithCString:message.data() encoding:kEncoding]];
            [_checkAllow setState:(check ? 1 : 0)];
        }
    }
}

- (IBAction)clickOk:(id)sender
{
    NSLog(@"clickOk");
    
    std::string command = "MES ";
    command += std::string([[_messageStore stringValue] UTF8String]);
    command += ";ERR?\n";
    std::string response, error;
    bool res = [_frameController executeCommand:command response:&response error:error];
    NSLog(@"Executed command %s with response: %s, error: %s", command.c_str(), response.c_str(), error.c_str());

    if (res) {
        std::unordered_map<std::string, std::string> mapping = [_frameController parseResponse:command response:response];
        if (mapping["ERR?\n"] == "0\r\n") {
            command += "REMSET ";
            command += (_checkAllow.state ? "1" : "0");
            command += ";ERR?\n";
            res = [_frameController executeCommand:command response:&response error:error];
            NSLog(@"Executed command %s with response: %s, error: %s", command.c_str(), response.c_str(), error.c_str());
        }
    }

    [[self window] close];
}

- (IBAction)clickCancel:(id)sender
{
    NSLog(@"clickCancel");
    [[self window] close];
}

- (IBAction)clickTerminal:(id)sender
{
    NSLog(@"clickTerminal");
    // TODO - show modal Terminal Dialog
    NSAlert *alertBox = [[NSAlert alloc] init];
    [alertBox setAlertStyle:NSAlertStyleWarning];
    [alertBox addButtonWithTitle:@"Cancel"];
    [alertBox addButtonWithTitle:@"OK"];
    [alertBox setMessageText:@"WARNING!\r\nChanges to settings are not tracked by ArroyoControl.\r\n If operational parameters are changed while in terminal mode\r\n a disconnect/connect in ArroyoControl is recommended."];
    [alertBox setIcon:[NSApp applicationIconImage]];
    NSModalResponse response = [alertBox runModal];
    if (response == 1000) { // first button (Cancel) response code.
        return;
    }
    
    TerminalDialogWindowController* terminalDiaolg = [[TerminalDialogWindowController alloc] initWithWindowNibName:@"TerminalDialogWindowController"];
    
    [terminalDiaolg setFrameController:_frameController];
    
    NSRect mainFrame = [[[NSApplication sharedApplication] windows] firstObject].frame;
    NSSize mainSize = mainFrame.size;
    NSSize windowSize = [terminalDiaolg window].frame.size;
    NSPoint pos;
    pos.x = mainFrame.origin.x + mainSize.width / 2 - windowSize.width / 2;
    pos.y = mainFrame.origin.y + mainSize.height / 2 - windowSize.height / 2;

    [terminalDiaolg.window setFrame:CGRectMake(pos.x, pos.y,
                                               windowSize.width ,
                                               windowSize.height) display:NO];
    [terminalDiaolg runModal];
}

- (void)runModal {
    [[NSApplication sharedApplication] runModalForWindow:self.window];
}

- (void)windowWillClose:(NSNotification *)notification {
    [[NSApplication sharedApplication] stopModal];
}


@end
