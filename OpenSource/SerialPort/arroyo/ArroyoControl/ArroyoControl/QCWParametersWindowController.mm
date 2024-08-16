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

#include "QCWParametersWindowController.h"
#include "FrameViewController.h"
#include "NSCustomPrecisionFormatter.h"
#include <string>

@interface QCWParametersWindowController ()

@property (nonatomic, strong) IBOutlet NSSlider *pwSlider;
@property (nonatomic, strong) IBOutlet NSSlider *frequencySlider;
@property (nonatomic, strong) IBOutlet NSSlider *dcSlider;
@property (nonatomic, strong) IBOutlet NSSlider *diSlider;
@property (nonatomic, strong) IBOutlet NSSlider *doSlider;
@property (nonatomic, strong) IBOutlet NSSlider *npSlider;

@property (nonatomic, strong) IBOutlet NSButton *okButton;
@property (nonatomic, strong) IBOutlet NSButton *cancelButton;
@property (nonatomic, strong) IBOutlet NSButton *applyButton;

@property (nonatomic, strong) IBOutlet NSButton *hfButton;
@property (nonatomic, strong) IBOutlet NSButton *hdcButton;

@property (nonatomic, strong) IBOutlet NSTextField *pwTextField;
@property (nonatomic, strong) IBOutlet NSTextField *frequencyTextField;
@property (nonatomic, strong) IBOutlet NSTextField *dcTextField;
@property (nonatomic, strong) IBOutlet NSTextField *diTextField;
@property (nonatomic, strong) IBOutlet NSTextField *doTextField;
@property (nonatomic, strong) IBOutlet NSTextField *npTextField;


@property (nonatomic, strong) IBOutlet NSTextField *frequencyUnits;
@property (nonatomic, strong) IBOutlet NSTextField *dcUnits;
@property (nonatomic, strong) IBOutlet NSTextField *diUnits;
@property (nonatomic, strong) IBOutlet NSTextField *doUnits;

@property (nonatomic, strong) IBOutlet NSTextField *frequencyLabel;
@property (nonatomic, strong) IBOutlet NSTextField *dcLabel;
@property (nonatomic, strong) IBOutlet NSTextField *diLabel;
@property (nonatomic, strong) IBOutlet NSTextField *doLabel;
@property (nonatomic, strong) IBOutlet NSTextField *npLabel;


@end

@implementation QCWParametersWindowController

static const NSInteger kEncoding = NSUTF8StringEncoding;

-(void) setupFormatter:(NSTextField*)field slider:(NSSlider*)slider digits:(NSInteger)digits {
    NSCustomPrecisionFormatter *formatter = [[NSCustomPrecisionFormatter alloc] init];
    [formatter setNumberStyle:NSNumberFormatterDecimalStyle];
    [formatter setMaximumFractionDigits:digits];
    [formatter setMinimumFractionDigits:0];
    [formatter setRoundingMode:NSNumberFormatterRoundDown];
    [formatter setUsesGroupingSeparator:FALSE];
    [formatter setMaximum:[[NSNumber alloc] initWithDouble:slider.maxValue]];
    [formatter setMinimum:[[NSNumber alloc] initWithDouble:slider.minValue]];
    [field setFormatter:formatter];
}

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Set formattors
    [self setupFormatter:_pwTextField slider:_pwSlider digits:0];
    [self setupFormatter:_frequencyTextField slider:_frequencySlider digits:0];
    [self setupFormatter:_dcTextField slider:_dcSlider digits:0];
    [self setupFormatter:_diTextField slider:_diSlider digits:6];
    [self setupFormatter:_doTextField slider:_doSlider digits:6];
    [self setupFormatter:_npTextField slider:_npSlider digits:0];
 
    std::string command = ":LAS:PW?;:LAS:F?;:LAS:DC?;:LAS:DELAYIN?;:LAS:DELAYOUT?;:LAS:QCWCOUNT?;:LAS:QCWHOLD?;ERR?\n";
    
    std::string response, error;
    bool res = [self.frameController executeCommand:command response:&response error:error];
    NSLog(@"Executed command %s with response: %s, error: %s", command.c_str(), response.c_str(), error.c_str());
    if (res) {
        std::unordered_map<std::string, std::string> mapping = [self.frameController parseResponse:command response:response];
        
        [_pwTextField setStringValue:[NSString stringWithCString:mapping[":LAS:PW?"].c_str() encoding:kEncoding]];
        [_frequencyTextField setStringValue:[NSString stringWithCString:mapping[":LAS:F?"].c_str() encoding:kEncoding]];
        [_dcTextField setStringValue:[NSString stringWithCString:mapping[":LAS:DC?"].c_str() encoding:kEncoding]];
        [_diTextField setStringValue:[NSString stringWithCString:mapping[":LAS:DELAYIN?"].c_str() encoding:kEncoding]];
        [_doTextField setStringValue:[NSString stringWithCString:mapping[":LAS:DELAYOUT?"].c_str() encoding:kEncoding]];
        [_npTextField setStringValue:[NSString stringWithCString:mapping[":LAS:QCWCOUNT?"].c_str() encoding:kEncoding]];

        [_pwSlider setDoubleValue:_pwTextField.doubleValue];
        [_frequencySlider setDoubleValue:_frequencyTextField.doubleValue];
        [_dcSlider setDoubleValue:_dcTextField.doubleValue];
        [_diSlider setDoubleValue:_diTextField.doubleValue];
        [_doSlider setDoubleValue:_doTextField.doubleValue];
        [_npSlider setDoubleValue:_npTextField.doubleValue];
        
        mapping[":LAS:QCWHOLD?"] == "0" ? [_hfButton setState:1] : [_hdcButton setState:1];

        _dcTextField.enabled = !_hfButton.state;
        _dcSlider.enabled = !_hfButton.state;
        _frequencyTextField.enabled = _hfButton.state;
        _frequencySlider.enabled = _hfButton.state;
        
        
        switch (_laserMode) {
            case 3: // Pulse - tag(3)
                [_diSlider setHidden:TRUE];
                [_diTextField setHidden:TRUE];
                [_diUnits setHidden:TRUE];
                [_diLabel setHidden:TRUE];

                [_npSlider setHidden:TRUE];
                [_npTextField setHidden:TRUE];
                [_npLabel setHidden:TRUE];
                break;
            case 4: // TRIG - tag(4)
                [_frequencySlider setHidden:TRUE];
                [_frequencyTextField setHidden:TRUE];
                [_frequencyUnits setHidden:TRUE];
                [_frequencyLabel setHidden:TRUE];

                [_dcSlider setHidden:TRUE];
                [_dcTextField setHidden:TRUE];
                [_dcUnits setHidden:TRUE];
                [_dcLabel setHidden:TRUE];

                [_npSlider setHidden:TRUE];
                [_npTextField setHidden:TRUE];
                [_npLabel setHidden:TRUE];

                [_hfButton setHidden:TRUE];
                [_hdcButton setHidden:TRUE];
                break;
            case 5: // BURST - tag(5)
                [_diSlider setHidden:TRUE];
                [_diTextField setHidden:TRUE];
                [_diUnits setHidden:TRUE];
                [_diLabel setHidden:TRUE];
                break;
            default:
                assert(false);
        }
    }
}

- (IBAction)clickOk:(id)sender
{
    NSLog(@"clickOk");
    [[self window] close];
}

- (IBAction)clickCancel:(id)sender
{
    NSLog(@"clickCancel");
    [[self window] close];
}

- (IBAction)clickApply:(id)sender
{
    NSLog(@"clickApply");
    std::string command = _hfButton.state ? ":LAS:PWF " : ":LAS:PWP ";
    command += std::string([_pwTextField.stringValue UTF8String]);
    command += ";:LAS:F ";
    command += std::string([_frequencyTextField.stringValue UTF8String]);
    command += ";:LAS:DC ";
    command += std::string([_dcTextField.stringValue UTF8String]);
    command += ";:LAS:DELAYIN ";
    command += std::string([_diTextField.stringValue UTF8String]);
    command += ";:LAS:DELAYOUT ";
    command += std::string([_doTextField.stringValue UTF8String]);
    command += ";:LAS:QCWCOUNT ";
    command += std::string([_npTextField.stringValue UTF8String]);
    command += ";:LAS:QCWHOLD ";
    command += _hfButton.state ? "0" : "1";
    command += "\n";
    std::string response, error;
    
    [self.frameController executeCommand:command response:&response error:error];
    NSLog(@"Executed command %s with response: %s, error: %s", command.c_str(), response.c_str(), error.c_str());
}

- (IBAction)clickGropHold:(id)sender
{
    _dcTextField.enabled = !_hfButton.state;
    _dcSlider.enabled = !_hfButton.state;
    _frequencyTextField.enabled = _hfButton.state;
    _frequencySlider.enabled = _hfButton.state;
    NSLog(@"clickGropHold");
}

-(void) adjustSettings {
    // ????
}

- (void)controlTextDidChange:(NSNotification *)notification {
    NSTextField *textField = [notification object];
    NSLog(@"controlTextDidChange: stringValue == %@", [textField stringValue]);
    if ([textField isEqualTo:_pwTextField]) {
        [_pwSlider setDoubleValue:textField.doubleValue];
    } else if ([textField isEqualTo:_frequencyTextField]) {
        [_frequencySlider setDoubleValue:textField.doubleValue];
    } else if ([textField isEqualTo:_dcTextField]) {
        [_dcSlider setDoubleValue:textField.doubleValue];
    } else if ([textField isEqualTo:_diTextField]) {
        [_diSlider setDoubleValue:textField.doubleValue];
    } else if ([textField isEqualTo:_doTextField]) {
        [_doSlider setDoubleValue:textField.doubleValue];
    } else if ([textField isEqualTo:_npTextField]) {
        [_npSlider setDoubleValue:textField.doubleValue];
    }
    
    [self adjustSettings];
}


- (IBAction)dragPWSlider:(id)sender
{
    NSLog(@"dragPWSlider");
    [_pwTextField setDoubleValue:_pwSlider.doubleValue];
    [self adjustSettings];
}

- (IBAction)dragFrequencySlider:(id)sender
{
    NSLog(@"dragFrequencySlider");
    [_frequencyTextField setDoubleValue:_frequencySlider.doubleValue];
    [self adjustSettings];
}

- (IBAction)dragDCSlider:(id)sender
{
    NSLog(@"dragDCSlider");
    [_dcTextField setDoubleValue:_dcSlider.doubleValue];
    [self adjustSettings];
}

- (IBAction)dragDISlider:(id)sender
{
    NSLog(@"dragDISlider");
    [_diTextField setDoubleValue:_diSlider.doubleValue];
    [self adjustSettings];
}

- (IBAction)dragDOSlider:(id)sender
{
    NSLog(@"dragDOSlider");
    [_doTextField setDoubleValue:_doSlider.doubleValue];
    [self adjustSettings];
}

- (IBAction)dragNPSlider:(id)sender
{
    NSLog(@"dragNPSlider");
    [_npTextField setDoubleValue:_npSlider.doubleValue];
    [self adjustSettings];
}

- (void)runModal {
    [[NSApplication sharedApplication] runModalForWindow:self.window];
}

- (void)windowWillClose:(NSNotification *)notification {
    [[NSApplication sharedApplication] stopModal];
}

@end
