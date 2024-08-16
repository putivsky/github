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

#include "TerminalDialogWindowController.h"
#include "FrameViewController.h"

@interface TerminalDialogWindowController ()

@property (nonatomic, strong) IBOutlet NSTextField *resultField;
@property (nonatomic, strong) IBOutlet NSTextField *commandField;

@property (nonatomic, strong) IBOutlet NSButton *fixedFoont;
@property (nonatomic, strong) IBOutlet NSButton *sendButton;
@property (nonatomic, strong) IBOutlet NSButton *clearButton;
@property (nonatomic, strong) IBOutlet NSButton *closeButton;

@end

@implementation TerminalDialogWindowController

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}

- (IBAction)clickClose:(id)sender
{
    NSLog(@"clickClose");
    [[self window] close];
}

- (IBAction)clickClear:(id)sender
{
    NSLog(@"clickClear");
    [_resultField setStringValue:@""];
}

- (IBAction)clickSend:(id)sender
{
    NSLog(@"clickSend");
    std::string command = std::string([_commandField.stringValue UTF8String]);
    command += "\n";
    std::string response, error;
    
    [self.frameController executeCommand:command response:&response error:error];
    [_resultField setStringValue: [NSString stringWithFormat: @"%@%s%s", [_resultField stringValue], command.c_str(), response.c_str()]];
}

- (IBAction)clickFixedFont:(id)sender
{
    NSButton* checkBox = (NSButton*)sender;
    if (checkBox.state) {
        _resultField.font = [NSFont userFixedPitchFontOfSize:12];
    } else {
        _resultField.font = [NSFont systemFontOfSize:12];
    }
    
    NSLog(@"clickFixedFont");
}

- (void)runModal {
    [[NSApplication sharedApplication] runModalForWindow:self.window];
}

- (void)windowWillClose:(NSNotification *)notification {
    [[NSApplication sharedApplication] stopModal];
}

@end
