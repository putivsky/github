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

#include "FrameViewController.h"
#include "PanelViewController.h"
#include "MainWindowController.h"
#include "LogWindowController.h"
#include "AppDelegate.h"
#include "SerialPortCommunication.h"
#include "DeviceCommunication.h"

#import <map>
#import <sstream>

@interface FrameViewController ()

@property (nonatomic, strong) IBOutlet NSButton *connectButton;
@property (nonatomic, strong) IBOutlet NSButton *closeButton;
@property (nonatomic, strong) IBOutlet NSComboBox *portComboBox;
@property (nonatomic, strong) IBOutlet NSTextField *deviceInfo;
@property (nonatomic, strong) IBOutlet NSTextField *logWindow;
@property (nonatomic, strong) IBOutlet NSButton *logButton;
@property (nonatomic, strong) IBOutlet NSBox *logBox;

@property (nonatomic) NSMutableArray* logEntries;
@property (nonatomic) std::map<uint8_t, std::string> serialDevices;
@property (nonatomic) NSInteger serialDeviceSelected;
@property (nonatomic) Arroyo::DeviceCommunication *deviceComm;

@property (nonatomic) LogWindowController *logController;

@property (readonly, strong) NSPersistentContainer *persistentContainer;

@end

@implementation FrameViewController

- (IBAction)clickToggle:(id)sender {
    [_mainWindowController togglePanel: self button:_toggleButton];
}

- (IBAction)clickClose:(id)sender {
    NSAlert *alertBox = [[NSAlert alloc] init];
    [alertBox setAlertStyle:NSAlertStyleWarning];
    [alertBox addButtonWithTitle:@"Cancel"];
    [alertBox addButtonWithTitle:@"OK"];
    [alertBox setMessageText:@"Do you want to remove this panel?"];
    [alertBox setInformativeText:@"Are you sure?"];

    [alertBox setIcon:[NSApp applicationIconImage]];

    NSModalResponse response = [alertBox runModal];
    if (response == 1000) { // first button (Cancel) response code.
        return;
    }

    // remove itself from splitview
    [_mainWindowController removePanel: self];
}

- (IBAction)clickConnect:(id)sender {
    if (_isConnected) {
        if (_isConnected) {
            // Switch to local control
            std::string request("LOCAL\n"), response, error;
            if (!_deviceComm->ExecuteCommand(request, &response, error)) {
                NSLog(@"Can't execute command: %s, error: %s", request.c_str(), error.c_str());
            } else {
                NSLog(@"Execute command: %s, response: %s", request.c_str(), response.c_str());
            }
        }
        _deviceComm->Disconnect();
        _isConnected = false;
        [_deviceInfo setStringValue:@""];
    } else {
        int idx = 1; // zero index for no port selection
        for (const auto& item : _serialDevices) {
            if (idx++ == _serialDeviceSelected) {
                std::string request, response, error;
                _isConnected = _deviceComm->Connect(item.second, error);
                if (_isConnected) {
                    request = "TERMINAL OFF;REMERR 0;RADIX DEC;VER?\n";
                    if (!_deviceComm->ExecuteCommand(request, &response, error) || !error.empty() || response.empty()) {
                        NSLog(@"Can't execute command: %s, error: %s", request.c_str(), error.c_str());
                    } else {
                        NSLog(@"Execute command: %s, response: %s", request.c_str(), response.c_str());
                    }
                    
                    if (!response.starts_with("v1.")) {
                        request = "HEXFLOAT 0;ERR?\n";
                        if (!_deviceComm->ExecuteCommand(request, &response, error)|| !error.empty() || response.empty()) {
                            NSLog(@"Can't execute command: %s, error: %s", request.c_str(), error.c_str());
                        } else {
                            NSLog(@"Execute command: %s, response: %s", request.c_str(), response.c_str());
                        }
                    }

                    request = "*IDN?\n";
                    std::string error;
                    if (!_deviceComm->ExecuteCommand(request, &response, error) || !error.empty() || response.empty()) {
                        NSAlert *alertBox = [[NSAlert alloc] init];
                        [alertBox setAlertStyle:NSAlertStyleWarning];
                        [alertBox addButtonWithTitle:@"OK"];
                        [alertBox setMessageText:@"Arroy Control is not found"];
                        [alertBox setIcon:[NSApp applicationIconImage]];
                        _deviceComm->Disconnect();
                        _isConnected = false;
                        [ alertBox runModal];
                    } else {
                        NSString *infoMessage = [NSString stringWithFormat:@"Connected to %s", response.data()];
                        [_deviceInfo setStringValue:infoMessage];
                    }
                }
                break;
            }
        }
    }
    
    [self setConnected:_isConnected];
}

- (IBAction)clickPortComboBox:(id)sender {
    // on success - enable sub panel
    NSLog(@"clickPortComboBox index %ld", ((NSComboBox*)sender).indexOfSelectedItem);
    _serialDeviceSelected = _portComboBox.indexOfSelectedItem;
    _connectButton.enabled = _serialDeviceSelected != 0;
}

- (IBAction)setConnected:(BOOL)state {
    [_connectButton setTitle:(state ? @"Disconnect" : @"Connect")];
    _portComboBox.enabled = !state;
    _logButton.enabled = state;

    NSArray<NSViewController*>* controllers = [self childViewControllers];
    for (int i = 0; i < controllers.count; ++i) {
        PanelViewController* controller = (PanelViewController*)controllers[i];
        [controller setConnected:state];
    }
}

- (IBAction)clickLogButton:(id)sender {
    // on success - enable sub panel
    NSInteger tag = ((NSButton*)sender).tag;
    NSLog(@"clickLogButton tag %ld", tag);
    // Toggle button
    switch (tag) {
        case 0:
            [_logButton setTag:1];
            [_logButton setTitle:@"Hide\r\nLog"];
            [_logController showWindow:_logController.window];
            break;
        case 1:
            [_logButton setTag:0];
            [_logButton setTitle:@"Show\r\nLog"];
            [_logController close];
            break;
    }
}

- (void) viewDidAppear {
    [super viewDidAppear];
    _deviceComm = new Arroyo::DeviceCommunication;
    _isConnected = false;
    // Populate ComboBox
    _serialDeviceSelected = 0;
    _persistentContainer = [[NSPersistentContainer alloc] initWithName:@"Model"];
    [_persistentContainer loadPersistentStoresWithCompletionHandler:^(NSPersistentStoreDescription *storeDescription, NSError *error) {
        if (error != nil) {
            NSLog(@"Unresolved error %@, %@", error, error.userInfo);
        } else {
            NSLog(@"Storage loaded, description %@", storeDescription);
            NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] initWithEntityName:@"PortIndex"];
            NSPredicate *predicate = [NSPredicate predicateWithFormat:@"name == %id", self.panelType];
            [fetchRequest setPredicate:predicate];
            @try {
                NSArray *ports = [self.persistentContainer.viewContext executeFetchRequest:fetchRequest error:&error];
                if (ports.count != 0) {
                    NSManagedObject *port = ports.firstObject;
                    self.serialDeviceSelected = [[port valueForKey:@"value"] integerValue];
                } else {
                    NSManagedObject *newPort = [NSEntityDescription insertNewObjectForEntityForName:@"PortIndex" inManagedObjectContext:self.persistentContainer.viewContext];
                    [newPort setValue:@(self.panelType) forKey:@"name"];
                    [newPort setValue:@(self.serialDeviceSelected) forKey:@"value"];
                }
            } @catch (NSException *exception) {
                NSLog(@"Exception!!! - %@", exception.reason);
            }
        }
    }];
    
    _serialDevices = Arroyo::SerialPort::enumerateSerialPorts();
    int idx = 0;
    NSString* entry = [[NSString alloc] initWithFormat:@"<Select Port>"];
    [_portComboBox insertItemWithObjectValue:entry atIndex:idx++];
    for (auto& item : _serialDevices) {
        // Construct like "COM{N} - Communication Port
        entry = [[NSString alloc] initWithFormat:@"COM%d - Communication Port", item.first];
        [_portComboBox insertItemWithObjectValue:entry atIndex:idx++];
    }
    
    if (_serialDeviceSelected < idx) {
        [_portComboBox selectItemAtIndex:_serialDeviceSelected];
    }
    
    _connectButton.enabled = _serialDeviceSelected != 0;
    _logEntries = [[NSMutableArray alloc] init];
    _logController = [[LogWindowController alloc] initWithWindowNibName:@"LogWindowController"];
    [_logController setLogEntries:_logEntries];
    [_logController setFrameController:self];
    [_logController setPanelType:_panelType];
    
    NSRect mainFrame = [[[NSApplication sharedApplication] windows] firstObject].frame;
    NSSize mainSize = mainFrame.size;
    NSSize windowSize = [_logController window].frame.size;
    NSPoint pos;
    pos.x = mainFrame.origin.x + mainSize.width / 2 - windowSize.width / 2;
    pos.y = mainFrame.origin.y + mainSize.height / 2 - windowSize.height / 2;

    [_logController.window setFrame:CGRectMake(pos.x, pos.y,
                                               windowSize.width ,
                                               windowSize.height) display:NO];
}

- (void) viewDidDisappear {
    [super viewDidDisappear];
    // clean-up
    [_logController close];
    _logController = nil;
    _logEntries = nil;
    delete _deviceComm;
    
    // save persistant settings
    if (_persistentContainer != nil) {
        NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] initWithEntityName:@"PortIndex"];
        NSPredicate *predicate = [NSPredicate predicateWithFormat:@"name == %id", self.panelType];
        [fetchRequest setPredicate:predicate];
        @try {
            NSError *error = nil;
            NSArray *ports = [self.persistentContainer.viewContext executeFetchRequest:fetchRequest error:&error];
            if (ports.count != 0) {
                NSManagedObject *port = ports.firstObject;
                [port setValue:@(_serialDeviceSelected) forKey:@"value"];
            } else {
                NSManagedObject *newPort = [NSEntityDescription insertNewObjectForEntityForName:@"PortIndex" inManagedObjectContext:self.persistentContainer.viewContext];
                [newPort setValue:@(self.panelType) forKey:@"name"];
                [newPort setValue:@(_serialDeviceSelected) forKey:@"value"];
            }
        } @catch (NSException *exception) {
            NSLog(@"Exception!!! - %@", exception.reason);
        }
                
        NSManagedObjectContext *context = _persistentContainer.viewContext;
        //[_persistentContainer.managedObjectModel.entities.in
        NSError *error = nil;
        if ([context hasChanges]) {
            if (![context save:&error]) {
                NSLog(@"Saving persistent container failed, %@", error);
            }
        }
    }
}

-(bool) executeCommand:(const std::string&)command response:(std::string*)response error:(std::string&)error {
    if (!_deviceComm->ExecuteCommand(command, response, error)) {
        NSLog(@"ExecuteCommand failed, command: %s, error: %sd", command.c_str(), error.c_str());
        return false;
    }
    return true;
}

-(void) writeLogEntry:(const std::string&) errorMessage {
    NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"dd-MM-yyyy HH:mm:ss"];
    NSDate* currentDate = [NSDate date];
    NSString* fullMsg = [NSString stringWithFormat:@"%@ - %s",
                          [formatter stringFromDate:currentDate],
                          errorMessage.c_str()];
    [_logEntries addObject:fullMsg];
    [self updateLogWindow];
    [_logController refreshLogEntries];
}

-(void) updateLogWindow {
    NSInteger count = _logEntries.count;
    if (count > 0) {
        [_logWindow setStringValue:_logEntries[count - 1]];
        NSString* title = [NSString stringWithFormat:@"Error log - %ld error(s) (showing last error)", count];
        [_logBox setTitle:title];
    } else {
        [_logBox setTitle:@"Error Log - No Errors"];
        [_logWindow setStringValue:@""];
    }
}

-(std::unordered_map<std::string, std::string>) parseResponse:(const std::string&)command response:(const std::string&)response {
    std::unordered_map<std::string, std::string> result;
    
    std::string token;

    std::istringstream commandStream(command);
    std::vector<std::string> commandTokens;
    while (std::getline(commandStream, token, ';')) {
        commandTokens.push_back(token);
    }

    std::istringstream responseStream(response);
    std::vector<std::string> responseTokens;
    while (std::getline(responseStream, token, ',')) {
        responseTokens.push_back(token);
    }
    
    int idx = 0;
    for (idx = 0; idx < std::min(commandTokens.size(), responseTokens.size()); ++idx) {
        result.emplace(commandTokens[idx], responseTokens[idx]);
    }
    
    return result;
}

-(void) notifyHideLogWindow {
    [_logButton setTag:0];
    [_logButton setTitle:@"Show\r\nLog"];
}

@end
