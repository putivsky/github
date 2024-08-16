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

#include "MainWindowController.h"
#include "SerialPortCommunication.h"
#include "NSCustomPrecisionFormatter.h"

#include <map>

@interface MainWindowController ()

@property (nonatomic, strong) IBOutlet NSComboBox *horizontalPortComboBox;
@property (nonatomic, strong) IBOutlet NSComboBox *verticalPortComboBox;
@property (nonatomic, strong) IBOutlet NSButton *connectButton;

@property (nonatomic, strong) IBOutlet NSButton *beginPositionButton;
@property (nonatomic, strong) IBOutlet NSButton *centerPositionButton;
@property (nonatomic, strong) IBOutlet NSButton *endPositionButton;

@property (nonatomic, strong) IBOutlet NSButton *runButton;
@property (nonatomic, strong) IBOutlet NSButton *stopButton;
@property (nonatomic, strong) IBOutlet NSButton *closeButton;

@property (nonatomic, strong) IBOutlet NSTextField *horizontalScanText;
@property (nonatomic, strong) IBOutlet NSTextField *verticalScanText;
@property (nonatomic, strong) IBOutlet NSTextField *logText;

@property (nonatomic) std::map<uint8_t, std::string> serialDevices;
@property (nonatomic) NSInteger horizontalDeviceSelected;
@property (nonatomic) NSInteger verticalDeviceSelected;
@property (nonatomic) Arroyo::SerialPort* horizontalComm;
@property (nonatomic) Arroyo::SerialPort* verticalComm;
@property (nonatomic) BOOL isConnected;
@property (nonatomic) BOOL isRunning;
@property (nonatomic) BOOL interruptFlag;
//@property (nonatomic, strong) NSTimer *queryTimer;
@property (nonatomic, strong) NSThread *workingThread;
@property (nonatomic) uint32_t scanHorizontal;
@property (nonatomic) uint32_t scanVertical;

@property (readonly, strong) NSPersistentContainer *persistentContainer;

@end

@implementation MainWindowController

static const NSInteger kEncoding = NSUTF8StringEncoding;

constexpr size_t kZaberCommandLength = 6;
constexpr uint8_t kZaberDeviceNumber = 0;
constexpr uint8_t kZaberMoveAbsCommand = 20;
constexpr uint8_t kZaberMoveRelativeCommand = 21;
constexpr uint8_t kZaberSetTargetSpeed = 42;
constexpr uint8_t kGetStatus = 54;

constexpr uint32_t kSpeedDataHorizontal = 5600;
constexpr uint32_t kSpeedDataVertical = 131000;
constexpr uint32_t kMM1Move = 5249;
constexpr uint32_t kHalfMove = 12.75 * kMM1Move; // 66924.75

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

-(void) setConnected:(BOOL)state {
    _isConnected = state;
    _connectButton.enabled = _horizontalDeviceSelected != 0 && _verticalDeviceSelected != 0;
    _beginPositionButton.enabled = state;
    _centerPositionButton.enabled = state;
    _endPositionButton.enabled = state;
    _horizontalScanText.enabled = state;
    _verticalScanText.enabled = state;
    _horizontalPortComboBox.enabled = !state;
    _verticalPortComboBox.enabled = !state;

    [_connectButton setTitle: (state ? @"Disconnect" : @"Connect")];
    
    _runButton.enabled = state;
    _stopButton.enabled = FALSE;
    if (_workingThread) {
        _interruptFlag = TRUE;
        [_workingThread cancel];
        _workingThread = nil;
    }
}

-(void) setRunning:(BOOL)state {
    _interruptFlag = !state;
    _isRunning = state;
    _runButton.enabled = _isConnected && !state;
    _stopButton.enabled = _isConnected && state;
    _beginPositionButton.enabled = _isConnected && !state;
    _centerPositionButton.enabled = _isConnected && !state;
    _endPositionButton.enabled = _isConnected && !state;
    _horizontalScanText.enabled = !state;
    _verticalScanText.enabled = !state;
    if (_workingThread && !state) {
        _interruptFlag = TRUE;
        [_workingThread cancel];
        _workingThread = nil;
    }
}

-(std::string) ZaberEncode:(uint8_t)device command:(uint8_t)command data:(uint32_t)data {
    std::string destination;
    destination.resize(kZaberCommandLength);

    destination[0] = device;
    destination[1] = command;
    for (size_t i = 2; i < 6; ++i)
    {
        destination[i] = (uint8_t)data;
        data >>= 8;
    }
    return destination;
}

-(uint32_t) ZaberDecode:(const std::string&)data {
    uint32_t destination = 0;
    for (size_t i = kZaberCommandLength - 1; i < data.size() && i > 1; --i)
    {
        destination <<= 8;
        destination |= data[i];
    }
    
    return destination;
}

-(bool) ExecuteCommand:(Arroyo::SerialPort*)port code:(uint8_t)code data:(uint32_t)data reply:(uint32_t*)reply {
    std::string encoded = [self ZaberEncode:kZaberDeviceNumber command:code data:data];
    std::string error;
    if (!port->Write(encoded, error)) {
        [_logText setStringValue:[NSString stringWithCString:error.c_str() encoding:kEncoding]];
        return false;
    }
    
    std::string response, output;
    // Read byte by byte
    for (size_t i = 0; i < kZaberCommandLength; ++i) {
        if (!port->Read(&output, error)) {
            [_logText setStringValue:[NSString stringWithCString:error.c_str() encoding:kEncoding]];
            return false;
        }

        if (output.empty()) { /* timed out */
            break;
        }

        response.push_back(output[0]);
    }
 
    *reply = [self ZaberDecode:response];
    return true;
}

-(void) ZaberWaitForIdle:(Arroyo::SerialPort*)port {
  uint32_t result;
  while (true) {

      if (![self ExecuteCommand:port code:kGetStatus data:0 reply:&result]) {
          return;
      }

      if (result == 0) {
          break;
      }

      usleep(1000);
  }
}

-(void) cleanUp {
    
    if (_horizontalComm) {
        if (_isConnected) {
            _horizontalComm->Close();
        }
        delete _horizontalComm;
        _horizontalComm = nil;
    }

    if (_verticalComm) {
        if (_isConnected) {
            _verticalComm->Close();
        }
        delete _verticalComm;
        _verticalComm = nil;
    }

    if (_workingThread) {
        _interruptFlag = TRUE;
        [_workingThread cancel];
        _workingThread = nil;
    }
    
    // save persistant settings
    if (_persistentContainer != nil) {
        NSFetchRequest *hFetchRequest = [[NSFetchRequest alloc] initWithEntityName:@"HPort"];
        NSPredicate *hPredicate = [NSPredicate predicateWithFormat:@"name == %id", 1];
        [hFetchRequest setPredicate:hPredicate];
        @try {
            NSError *error = nil;
            NSArray *ports = [self.persistentContainer.viewContext executeFetchRequest:hFetchRequest error:&error];
            if (ports.count != 0) {
                NSManagedObject *port = ports.firstObject;
                [port setValue:@(self.horizontalDeviceSelected) forKey:@"value"];
            } else {
                NSManagedObject *newPort = [NSEntityDescription insertNewObjectForEntityForName:@"HPort" inManagedObjectContext:self.persistentContainer.viewContext];
                [newPort setValue:@(1) forKey:@"name"];
                [newPort setValue:@(self.horizontalDeviceSelected) forKey:@"value"];
            }
        } @catch (NSException *exception) {
            NSLog(@"Exception!!! - %@", exception.reason);
        }
                
        NSFetchRequest *vFetchRequest = [[NSFetchRequest alloc] initWithEntityName:@"VPort"];
        NSPredicate *vPredicate = [NSPredicate predicateWithFormat:@"name == %id", 2];
        [vFetchRequest setPredicate:vPredicate];
        @try {
            NSError *error = nil;
            NSArray *ports = [self.persistentContainer.viewContext executeFetchRequest:vFetchRequest error:&error];
            if (ports.count != 0) {
                NSManagedObject *port = ports.firstObject;
                [port setValue:@(self.verticalDeviceSelected) forKey:@"value"];
            } else {
                NSManagedObject *newPort = [NSEntityDescription insertNewObjectForEntityForName:@"HPort" inManagedObjectContext:self.persistentContainer.viewContext];
                [newPort setValue:@(2) forKey:@"name"];
                [newPort setValue:@(self.verticalDeviceSelected) forKey:@"value"];
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

- (void)windowDidLoad {
    [super windowDidLoad];
    
    _interruptFlag = TRUE;

    [_horizontalScanText setIntValue:8];
    [_verticalScanText setIntValue:8];
    [self setupFormatter:_horizontalScanText digits:0];
    [self setupFormatter:_verticalScanText digits:0];
    
    _horizontalComm = new Arroyo::SerialPort;
    _verticalComm = new Arroyo::SerialPort;
    // Populate ComboBox
    _horizontalDeviceSelected = 0;
    _verticalDeviceSelected = 0;
    
    _persistentContainer = [[NSPersistentContainer alloc] initWithName:@"Model"];
    [_persistentContainer loadPersistentStoresWithCompletionHandler:^(NSPersistentStoreDescription *storeDescription, NSError *error) {
        if (error != nil) {
            NSLog(@"Unresolved error %@, %@", error, error.userInfo);
        } else {
            NSLog(@"Storage loaded, description %@", storeDescription);
            NSFetchRequest *hFetchRequest = [[NSFetchRequest alloc] initWithEntityName:@"HPort"];
            NSPredicate *hPredicate = [NSPredicate predicateWithFormat:@"name == %id", 1];
            [hFetchRequest setPredicate:hPredicate];
            @try {
                NSArray *ports = [self.persistentContainer.viewContext executeFetchRequest:hFetchRequest error:&error];
                if (ports.count != 0) {
                    NSManagedObject *port = ports.firstObject;
                    self.horizontalDeviceSelected = [[port valueForKey:@"value"] integerValue];
                } else {
                    NSManagedObject *newPort = [NSEntityDescription insertNewObjectForEntityForName:@"HPort" inManagedObjectContext:self.persistentContainer.viewContext];
                    [newPort setValue:@(1) forKey:@"name"];
                    [newPort setValue:@(self.horizontalDeviceSelected) forKey:@"value"];
                }
            } @catch (NSException *exception) {
                NSLog(@"Exception!!! - %@", exception.reason);
            }
            
            NSFetchRequest *vFetchRequest = [[NSFetchRequest alloc] initWithEntityName:@"VPort"];
            NSPredicate *vPredicate = [NSPredicate predicateWithFormat:@"name == %id", 2];
            [vFetchRequest setPredicate:vPredicate];
            @try {
                NSArray *ports = [self.persistentContainer.viewContext executeFetchRequest:vFetchRequest error:&error];
                if (ports.count != 0) {
                    NSManagedObject *port = ports.firstObject;
                    self.verticalDeviceSelected = [[port valueForKey:@"value"] integerValue];
                } else {
                    NSManagedObject *newPort = [NSEntityDescription insertNewObjectForEntityForName:@"VPort" inManagedObjectContext:self.persistentContainer.viewContext];
                    [newPort setValue:@(2) forKey:@"name"];
                    [newPort setValue:@(self.verticalDeviceSelected) forKey:@"value"];
                }
            } @catch (NSException *exception) {
                NSLog(@"Exception!!! - %@", exception.reason);
            }
        }
    }];
    
    
    _serialDevices = Arroyo::SerialPort::enumerateSerialPorts();
    
    int idx = 0;
    NSString* entry = [[NSString alloc] initWithFormat:@"<Select Port>"];
    [_horizontalPortComboBox insertItemWithObjectValue:entry atIndex:idx];
    [_verticalPortComboBox insertItemWithObjectValue:entry atIndex:idx];
    ++idx;
    for (auto& item : _serialDevices) {
        // Construct like "COM{N}
        entry = [[NSString alloc] initWithFormat:@"COM%d", item.first];
        [_horizontalPortComboBox insertItemWithObjectValue:entry atIndex:idx];
        [_verticalPortComboBox insertItemWithObjectValue:entry atIndex:idx];
        ++idx;
    }
    
    if (_horizontalDeviceSelected < idx) {
        [_horizontalPortComboBox selectItemAtIndex:_horizontalDeviceSelected];
    }
    if (_verticalDeviceSelected < idx) {
        [_verticalPortComboBox selectItemAtIndex:_verticalDeviceSelected];
    }
    
    [self setConnected:FALSE];
    [self setRunning:FALSE];
}

- (IBAction)clickHorizontalPortComboBox:(id)sender {
    // on success - enable sub panel
    NSLog(@"clickPortComboBox index %ld", ((NSComboBox*)sender).indexOfSelectedItem);
    _horizontalDeviceSelected = _horizontalPortComboBox.indexOfSelectedItem;
    _connectButton.enabled = _horizontalDeviceSelected != 0 && _verticalDeviceSelected != 0;
}

- (IBAction)clickVeticalPortComboBox:(id)sender {
    // on success - enable sub panel
    NSLog(@"clickPortComboBox index %ld", ((NSComboBox*)sender).indexOfSelectedItem);
    _verticalDeviceSelected = _verticalPortComboBox.indexOfSelectedItem;
    _connectButton.enabled = _horizontalDeviceSelected != 0 && _verticalDeviceSelected != 0;
}

- (IBAction)clickConnectButton:(id)sender
{
    bool horizontalConnected = false, verticalConnected = false;
    NSLog(@"clickBeginButton");
    if (_isConnected) {
        [self setConnected:FALSE];
    } else {
        int idx = 1; // zero index for no port selection
        std::string error;
        for (const auto& item : _serialDevices) {
            if (idx++ == _horizontalDeviceSelected) {
                std::string error;
                horizontalConnected = _horizontalComm->Open(item.second, error);
                if (!horizontalConnected) {
                    [_logText setStringValue:[NSString stringWithCString:error.c_str() encoding:kEncoding]];
                }
                break;
            }
        }
        
        if (horizontalConnected) {
            idx = 1; // zero index for no port selection
            for (const auto& item : _serialDevices) {
                if (idx++ == _verticalDeviceSelected) {
                    std::string error;
                    verticalConnected = _verticalComm->Open(item.second, error);
                    if (!verticalConnected) {
                        [_logText setStringValue:[NSString stringWithCString:error.c_str() encoding:kEncoding]];
                        _horizontalComm->Close();
                    }
                    break;
                }
            }
        }
    }
    
    [self setConnected:(horizontalConnected && verticalConnected)];
}

- (IBAction)clickBeginButton:(id)sender
{
    NSLog(@"clickBeginButton");
    uint32_t result;
    uint32_t data = kHalfMove - (uint32_t)_horizontalScanText.integerValue * kMM1Move / 2;
    if (![self ExecuteCommand:_horizontalComm code:kZaberMoveAbsCommand data:data reply:&result]) {
        return;
    } else {
        printf("Current horizontal position is %d\n", result);
    }

    data = kHalfMove - (uint32_t)_verticalScanText.integerValue * kMM1Move / 2;
    if (![self ExecuteCommand:_verticalComm code:kZaberMoveAbsCommand data:data reply:&result]) {
        return;
    } else {
        printf("Current vertical position is %d\n", result);
    }
    
    [self ZaberWaitForIdle:_horizontalComm];
    [self ZaberWaitForIdle:_verticalComm];
}

- (IBAction)clickCenterButton:(id)sender
{
    NSLog(@"clickCenterButton");
    uint32_t result;
    if (![self ExecuteCommand:_horizontalComm code:kZaberMoveAbsCommand data:kHalfMove reply:&result]) {
        return;
    } else {
        printf("Current horizontal position is %d\n", result);
    }

    if (![self ExecuteCommand:_verticalComm code:kZaberMoveAbsCommand data:kHalfMove reply:&result]) {
        return;
    } else {
        printf("Current vertical position is %d\n", result);
    }
    
    [self ZaberWaitForIdle:_horizontalComm];
    [self ZaberWaitForIdle:_verticalComm];
}

- (IBAction)clickEndButton:(id)sender
{
    NSLog(@"clickEndButton");
    
    uint32_t result;
    uint32_t data = kHalfMove + (uint32_t)_horizontalScanText.integerValue * kMM1Move / 2;
    if (![self ExecuteCommand:_horizontalComm code:kZaberMoveAbsCommand data:data reply:&result]) {
        return;
    } else {
        printf("Current horizontal position is %d\n", result);
    }

    data = kHalfMove + (uint32_t)_verticalScanText.integerValue * kMM1Move / 2;
    if (![self ExecuteCommand:_verticalComm code:kZaberMoveAbsCommand data:data reply:&result]) {
        return;
    } else {
        printf("Current vertical position is %d\n", result);
    }
    
    [self ZaberWaitForIdle:_horizontalComm];
    [self ZaberWaitForIdle:_verticalComm];
}

-(void) threadFunction:(void*)param {
    BOOL flipSign = FALSE;
    uint32_t result;
    while (!_interruptFlag) {
        for (int j = 0; !_interruptFlag && j < _scanVertical * 10; ++j) {
            flipSign = ~flipSign;
            uint32_t data = kHalfMove + (flipSign ? (_scanHorizontal * kMM1Move / 2) : -(_scanHorizontal * kMM1Move / 2));
            if (![self ExecuteCommand:_verticalComm code:kZaberMoveAbsCommand data:data reply:&result]) {
                return;
            } else {
                printf("Current horizontal position is %d\n", result);
            }
            
            [self ZaberWaitForIdle:_verticalComm];
            
            data = kMM1Move / 10;
            if (![self ExecuteCommand:_horizontalComm code:kZaberMoveRelativeCommand data:data reply:&result]) {
                return;
            } else {
                printf("Current vertical position is %d\n", result);
            }
            
            [self ZaberWaitForIdle:_horizontalComm];
            
            printf("................................:%d\n", _scanVertical * 10 - j);
        }
    }
}

- (IBAction)clickRunButton:(id)sender
{
    NSLog(@"clickRunButton");

    uint32_t result;
    uint32_t data = kHalfMove - (uint32_t)_horizontalScanText.integerValue * kMM1Move / 2;
    if (![self ExecuteCommand:_horizontalComm code:kZaberMoveAbsCommand data:data reply:&result]) {
        return;
    } else {
        printf("Current horizontal position is %d\n", result);
    }

    data = kHalfMove - (uint32_t)_verticalScanText.integerValue * kMM1Move / 2;
    if (![self ExecuteCommand:_verticalComm code:kZaberMoveAbsCommand data:data reply:&result]) {
        return;
    } else {
        printf("Current vertical position is %d\n", result);
    }

    // set horizontal speed
    if (![self ExecuteCommand:_horizontalComm code:kZaberSetTargetSpeed data:kSpeedDataHorizontal reply: &result]) {
        return;
    } else {
        printf("Current horizontal target speed is %d\n", result);
    }

    if (![self ExecuteCommand:_verticalComm code:kZaberSetTargetSpeed data:kSpeedDataVertical reply: &result]) {
        return;
    } else {
        printf("Current vertical target speed is %d\n", result);
    }

    _scanHorizontal = (uint32_t)_horizontalScanText.integerValue;
    _scanVertical = (uint32_t)_verticalScanText.integerValue;
    _interruptFlag = FALSE;
    _workingThread = [[NSThread alloc] initWithTarget:self selector:@selector(threadFunction:) object:nil];
    [_workingThread start];
    
    [self setRunning:TRUE];
}

- (IBAction)clickStopButton:(id)sender
{
    NSLog(@"clickStopButton");
    [ self setRunning:FALSE];
}

- (void)controlTextDidChange:(NSNotification *)notification {
    NSTextField *textField = [notification object];
    NSLog(@"controlTextDidChange: stringValue == %@", [textField stringValue]);
    if ([textField isEqualTo:_horizontalScanText]) {
        [_horizontalScanText setIntegerValue:textField.integerValue];
    } else if ([textField isEqualTo:_verticalScanText]) {
        [_verticalScanText setIntegerValue:textField.integerValue];
    }
}

@end
