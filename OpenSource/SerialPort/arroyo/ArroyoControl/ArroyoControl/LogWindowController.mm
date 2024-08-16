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

#include "LogWindowController.h"
#include "FrameViewController.h"
#include "MainWindowController.h"

@interface LogWindowController ()
@property (nonatomic, strong) IBOutlet NSButton *clearButton;
@property (nonatomic, strong) IBOutlet NSButton *hideButton;

@property (nonatomic, strong) IBOutlet NSTextField *outputText;

@end

@implementation LogWindowController

- (void)windowDidLoad {
    [super windowDidLoad];
    switch (_panelType) {
        case kLaserView:
            self.window.title = @"Error log for LaserSource";
            break;
        case kTECView:
            self.window.title = @"Error log for TECSource";
            break;
        case kComboView:
            self.window.title = @"Error log for ComboSource";
            break;
    }
}

- (BOOL)windowShouldClose:(id)sender {
    [NSApp hide:nil];
    return NO;
}

- (void) refreshLogEntries {
    NSInteger count = _logEntries.count;
    // show all entries
    NSMutableString* result = [NSMutableString stringWithCapacity:count];
    for (int i = 0; i < count; ++i) {
        [result appendFormat:@"%@\n", _logEntries[i]];
    }
    
    [_outputText setStringValue:result];
}

- (IBAction)showWindow:(nullable id)sender {
    [self refreshLogEntries];
    [super showWindow:sender];
}

- (IBAction)clickClear:(id)sender {
    [_logEntries removeAllObjects];
    [_outputText setStringValue:@""];
    [_frameController updateLogWindow];
}

- (IBAction)clickHide:(id)sender {
    [self close];
}

-(void) close {
    [_frameController notifyHideLogWindow];
    [super close];
}

@end
