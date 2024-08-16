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

#include "PanelViewController.h"
#include "FrameViewController.h"
#include <string>
#include <map>
#include <sstream>

@interface PanelViewController ()

@property (nonatomic, strong) IBOutlet NSBox *setPointLabel;
@property (nonatomic, strong) IBOutlet NSBox *outputLabel;
@property (nonatomic, strong) IBOutlet NSButton *comboSettings;
@property (nonatomic, strong) IBOutlet NSButton *systemSettings;

@property (nonatomic, strong) NSTimer *queryTimer;

@end

@implementation PanelViewController

static const struct ErrorMapping {
    int code;
    const char * const description;
    const char * const cause;
} sErrorMapping[] = {
    {3, "Factory EEPROM Error", "The factory section of the EEPROM, which contains the calibration data and system configuration systems, is corrupted. The unit must be returned to the factory for repair."},
    {4,  "User EEPROM Error", "The user settings section of the EEPROM, which contains user editable set points, modes, etc., is corrupted. To repair the corruption, the unit was reset to factory defaults."},
    {5, "User Reset EEPROM", "The user reset the unit to factory defaults. This is a notification message only."},
    {6, "User EEPROM Failed", "The user settings section of the EEPROM, which contains user editable set points, modes, etc., is corrupted. The corruption cannot be repaired automatically, please contact the factory for assistance."},
    {7, "Preset EEPROM Failed", "The preset settings section of the EEPROM, which contains user section defaults, is corrupted. The corruption cannot be repaired automatically, please contact the factory for assistance."},
    {8, "User EEPROM Failed", "The user settings section of the EEPROM, which contains user editable set points, modes, etc., is corrupted, and the preset section cannot restore it. The corruption cannot be repaired automatically, please contact the factory for assistance."},
    {9, "Output Disabled", "Added to every ERR? or ERRSTR? query when the EEPROM is corrupted. The output cannot be turned on until the corruption has been repaired."},
    {100, "General Error", "The error code is non-specific, and is generally used when no other error code is suitable."},
    {102, "Message too long", "The message is too long to process (USB/Serial only)."},
    {104, "Type not allowed", "The RADix type was invalid"},
    {123, "Path not found", "The message used an invalid path command (USB/Serial only)."},
    {124, "Data mismatch", "The message contained data that did not match the expected format (USB/Serial only)."},
    {126, "Too few or too many elements", "The command requires more or less than the number of parameters actually supplied."},
    {127, "Change not allowed", "An attempt was made to change a parameter that cannot be changed, or is currently read-only."},
    {128, "Script Terminated", "A executing script was terminated with a Ctrl-X."},
    {201, "Data out of range", "The message attempted to set a value that was outside the allowable range (USB/Serial only)."},
    {202, "Invalid data type", "When trying to parse the message, the data was in an invalid format (USB/Serial only)."},
    {204, "Suffix not valid", "An invalid number base suffix (radix) was encountered when parsing a number (USB/Serial only)."},
    {217, "Configuration Recall failed", "An attempt recall a configuration failed. This can be caused if no configuration exists in the selected slot, the slot number is out of range, or if the configuration is corrupted."},
    {218, "Configuration Save failed", "At attempt to save a configuration failed. This can be caused if the slot number is out of range, or the configuration memory is corrupted."},
    {220, "Script Save Failed", "At attempt to save a script failed. This can be caused if the script number is out of range, or the script memory is corrupted."},
    {221, "Cannot embed script", "A script was executed that contained a reference to another script."},
    {222, "Cannot execute script", "At attempt to execute a script failed. This can be caused if the script number is out of range, no script exists for the selected index, or the script memory is corrupted."},
    {303, "Input buffer overrun", "The command input buffer overran, which can be caused by excessively long command strings or improperly terminated commands."},
    {402, "Sensor open, output turned off", "A sensor open circuit was detected and the output was turned off."},
    {403, "Module open, output turned off", "A Peltier module open circuit was detected and the output was turned off. Computer Interfacing Manual · Page 99"},
    {404, "I limit, output turned off", "A current limit was detected and the output was turned off."},
    {405, "V limit, output turned off", "A voltage limit was detected and the output was turned off."},
    {406, "Thermistor resistance limit, output turned off", "The thermistor resistance limit (high or low) was exceeded and the output was turned off."},
    {407, "Temperature limit, output turned off", "The temperature limit (high or low) was exceeded and the output was turned off."},
    {409, "Sensor change, output off", "The sensor was changed while the output was on, and the output was turned off."},
    {410, "Temperature was out of tolerance, output turned off", "The temperature went out of tolerance and the output was turned off."},
    {415, "Sensor short, output turned off", "A sensor short circuit was detected and the output was turned off."},
    {416, "Calibration failure", "The calibration process failed due to improper setup, an interfering action (set point change, output on/off), or unexpected results."},
    {419, "TEC not stable The TEC is considered stable if the temperature has changed less than 0.02°C for more than 20 seconds."},
    {433, "Not a TEC", "The TEC:CHAN command attempted to select a non-TEC channel"},
    {434, "Ite limit exceeds cable rating", " The cable plugged into the unit cannot carry the current as limited by the Ite Limit setting. Lower the Ite limit to the cables capacity, or use a higher capacity cable."},
    {435, "Mode change", "A mode change occurred when the output was on, and the output was turned off."},
    {436, "AutoTune Failed", "The AutoTune process failed."},
    {437, "AutoTune Required T Mode", "The AutoTune process was cancelled because the instrument was not in T mode."},
    {438, "Thermal Trip", "The thermal limit of the heat sink was reached, output turned off."},
    {439, "Thermal Run-Away", "Thermal run-away, output off"},
    {450, "TECPak analog set disconnected", "An attempt to turn on the TEC output was prevented because the instrument was configured to use the analog set point input and no analog set point was detected."},
    {501, "Interlock shutdown output", "The interlock was open when the output was on (or was attempting to turn on)."},
    {504, "Laser current limit disabled output.", "The laser output was turned off because a current limit was detected and the corresponding bit in the OUTOFF register was set."},
    {505, "Laser voltage limit disabled output", "The laser voltage exceeded the voltage limit and the output was turned off."},
    {506, "Laser photodiode current limit disabled output", "The laser output was turned off because a photodiode current limit was detected and the corresponding bit in the OUTOFF register was set."},
    {507, "Laser photodiode power limit", "The laser output was turned off because a photodiode power limit was detected and the corresponding bit in the OUTOFF register was set. Page 100 · Computer Interfacing Manual disabled output"},
    {508, "TEC off disabled output", "The laser output was turned off because the TEC was off and the corresponding bit in the OUTOFF register was set."},
    {509, "Laser short circuit disabled output", "The laser output was turned off because a short condition was detected and the corresponding bit in the OUTOFF register was set."},
    {510, "Laser out of tolerance disabled output", "The laser output was turned off because an out-of-tolerance condition was detected and the corresponding bit in the OUTOFF register was set."},
    {511, "Laser control error disabled output", "A hardware control error was detected which forced a shutdown of the laser output."},
    {512, "Power failure", "A power failure was detected."},
    {514, "Laser mode change disabled output",
        "A change in the operating mode of the Laser driver while the output was on shutdown the output."},
    {516, "Incorrect configuration for calibration to start", "The Laser driver was not configured properly, including the mode and output on state, to be able to start the desired calibration process."},
    {517, "Calibration must have the output on to start", "The laser output must be on for the calibration process to start."},
    {521, "TEC temperature limit disabled output", "The laser output was turned off because the TEC temperature limit was exceeded and the corresponding bit in the OUTOFF register was set."},
    {534, "Po mode selected with PD Response set to zero", "Attempted to select Po mode and PD Response was zero, or Laser driver was in Po mode and PD Response was set to zero."},
    {535, "Calibration cancelled", "The active calibration process was cancelled."},
    {536, "Intermittent contact detected", "The instrument detected an intermittent contact and shut down the laser output. If this is triggering falsely (such as in a noisy environment), the intermittent contact detection can be disabled in the main menu."},
    {537, "Thermal Limit Exceeded", "The thermal load inside the instrument is too high, and the output was shutdown to protect the instrument."},
    {538, "Sensor Limits Exceeded", "A laser temperature sensor exceeded the resistance limits, laser output turned off."},
    {539, "Temperature Limits Exceeded", "A laser temperature sensor exceeded the temperature limits, laser output turned off."},
    {700, "Config saved Instrument configuration successfully stored."},
    {701, "Config loaded Instrument configuration successfully loaded."},
    {703, "Laser usercal reset", "The user-provided calibration for the laser measurements and set points reset by the user."},
    {704, "TEC usercal reset", "The user-provided calibration for the TEC measurements and set points reset by the user."},
    {800, "Remote Voltage Sense Low", "Notification message only: On instruments with remote voltage sense, the remote voltage measurement is much lower than the voltage at the output connector. This is only a warning, and does not indicate an actual problem. Computer Interfacing Manual · Page 101"},
    {801, "Burst Mode, Hold Output", "Notification message only: When in Io (Burst) mode, to turn the output on, the Output button must be held down for at least one second. If it is held down for less than one second, this warning message informs the user than the Output button press did not turn the output on."},
    {803, "User reset to factory defaults", "Notification message only: User pressed key sequence on start-up to reset unit to factory defaults."},
    {805, "User recall turned outputs off", "Notification message only: A user configuration was recalled from memory while the outputs were on, resulting in the outputs being turned off."},
    {806, "No function key assigned", "Notification message only: User attempted to execute a function key action that was not assigned."},
    {980, "Module Offline", "Communications was attempted to off-line module."},
    {981, "Slave module X communication failure", "A communication failure to the referenced salve module was detected. Slave module taken offline."},
    {982, "Slave module X communication failure", "A communication failure to the referenced salve module was detected. Slave module taken offline."},
    {983, "Slave module X communication failure", "A communication failure to the referenced salve module was detected. Slave module taken offline."},
    {984, "Slave module X communication failure", "A communication failure to the referenced salve module was detected. Slave module taken offline."},
    {988, "Power supply comm failure", "Failure to communicate with internal power supply. If problem persists, contact factory."},
    {989, "Network interface error", "Instrument could not communicate with the network module. Ethernet communications may be offline until next power cycle."},
    {990, "Hardware-related errors", "A hardware related error occurred. Attempt a power cycle to resolve. If error continues to occur, contact factory."},
    {991, "Hardware-related errors", "A hardware related error occurred. Attempt a power cycle to resolve. If error continues to occur, contact factory."},
    {992, "Hardware-related errors", "A hardware related error occurred. Attempt a power cycle to resolve. If error continues to occur, contact factory."},
    {993, "Hardware-related errors", "A hardware related error occurred. Attempt a power cycle to resolve. If error continues to occur, contact factory."},
    {994, "Hardware-related errors", "A hardware related error occurred. Attempt a power cycle to resolve. If error continues to occur, contact factory."},
    {995, "Hardware-related errors", "A hardware related error occurred. Attempt a power cycle to resolve. If error continues to occur, contact factory."},
    {996, "Hardware-related errors", "A hardware related error occurred. Attempt a power cycle to resolve. If error continues to occur, contact factory."},
    {997, "Hardware-related errors", "A hardware related error occurred. Attempt a power cycle to resolve. If error continues to occur, contact factory."},
    {998, "Command not supported", "A command was recognized but not supported by the Laser driver."},
    {999, "Non-specific error", "A non-specific error was encountered."},
};

-(void) showCommonSettings {
    // Derived classes must override this method
    assert(false);
}

- (IBAction)clickCommonSettings:(id)sender
{
    NSLog(@"clickCommonSettings");
    [self showCommonSettings];
}

-(void) showSystemSettings {
    // Derived classes must override this method
    assert(false);
}

- (IBAction)clickSystemSettings:(id)sender
{
    NSLog(@"clickSystemSettings");
    [self showSystemSettings];
}

- (IBAction)clickSet:(id)sender {
    [self onClickSetButton];
}

-(void) onClickSetButton {
    // Derived classes must override this method
    assert(false);
}

- (void)setConnected:(BOOL)state {
    _onOutputGroup.enabled = state;
    _offOutputGroup.enabled = state;
    _comboSettings.enabled = state;
    _systemSettings.enabled = state;
    _setButton.enabled = state;
    _setInput.enabled = state;
    _setUnits.hidden = !state;
    
    
    if (state) {
        _queryTimer = [NSTimer scheduledTimerWithTimeInterval:1 // 100ms
                                                       target:self
                                                       selector:@selector(onQueryTimer:)
                                                       userInfo:nil
                                                       repeats:YES];
    } else if (_queryTimer) {
        [_queryTimer invalidate];
        _queryTimer = nil;
        [_setButton setTag:0];
    }
}

-(void) onQueryTimer:(NSTimer *)timer {
    // Derived classes must override this method
    assert(false);
}

- (void)viewDidLoad {
    [super viewDidLoad];
    if (_comboMode) {
        [_outputLabel setTitle:@"Laser"];
        [_setPointLabel setTitle:@"Laser Setpoint"];
        [_comboSettings setTitle:@"Combo Settings..."];
    }

    NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
    [formatter setNumberStyle:NSNumberFormatterDecimalStyle];
    [formatter setMaximumFractionDigits:2];
    [formatter setRoundingMode:NSNumberFormatterRoundDown];
    [formatter setAlwaysShowsDecimalSeparator:FALSE];
    [formatter setUsesGroupingSeparator:FALSE];

    [_setInput setFormatter:formatter];
}

-(void)viewDidDisappear {
    if (_queryTimer) {
        [_queryTimer invalidate];
        _queryTimer = nil;
    }
}

-(std::string) errorDescriptionByCode:(const std::string&)code {
    int num = std::stoi(code);
    for (int idx = 0; idx < sizeof(sErrorMapping)/sizeof(*sErrorMapping); ++idx) {
        const auto& entry = sErrorMapping[idx];
        if (num == entry.code) {
            std::string result;
            result = std::to_string(entry.code);
            result += " ";
            result += entry.description;
            result += " [";
            result += entry.cause;
            result += "]";
            return result;
        }
    }
    
    std::string result = "Unkown error, code: " + code;
    return result;
}

@end

