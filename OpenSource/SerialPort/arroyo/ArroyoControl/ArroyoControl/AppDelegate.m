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

#import "AppDelegate.h"
#import "AboutDialog.h"
#import "AddPanelSelectorWindowController.h"
#import "MainWindowController.h"

@interface AppDelegate()

@property (strong) IBOutlet NSWindow *window;
@property (readonly, strong) NSPersistentContainer *persistentContainer;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    if (mainWindowController == nil) {
        mainWindowController = [[MainWindowController alloc] initWithWindowNibName:@"MainWindowController"];
    }
    
    [mainWindowController showWindow:self];
}


- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}


- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
    return YES;
}

- (IBAction)aboutAction:(id)sender {
    AboutDialog* aboutDialog = [[AboutDialog alloc] initWithWindowNibName:@"AboutDialog"];
    NSRect mainFrame = [[[NSApplication sharedApplication] windows] firstObject].frame;
    NSSize mainSize = mainFrame.size;
    NSSize dialogSize = [aboutDialog window].frame.size;
    NSPoint pos;
    pos.x = mainFrame.origin.x + mainSize.width / 2 - dialogSize.width / 2;
    pos.y = mainFrame.origin.y + mainSize.height / 2 - dialogSize.height / 2;

    [aboutDialog.window setFrame:CGRectMake(pos.x, pos.y,
                                            dialogSize.width ,
                                            dialogSize.height) display:NO];
    
    [aboutDialog runModal];
 }

- (IBAction)addPanelAction:(id)sender
{
    AddPanelSelectorWindowController* addPanelSelector = [[AddPanelSelectorWindowController alloc] initWithWindowNibName:@"AddPanelSelectorWindowController"];
    NSRect mainFrame = [[[NSApplication sharedApplication] windows] firstObject].frame;
    NSSize mainSize = mainFrame.size;
    NSSize dialogSize = [addPanelSelector window].frame.size;
    NSPoint pos;
    pos.x = mainFrame.origin.x + mainSize.width / 2 - dialogSize.width / 2;
    pos.y = mainFrame.origin.y + mainSize.height / 2 - dialogSize.height / 2;

    [addPanelSelector.window setFrame:CGRectMake(pos.x, pos.y,
                                             dialogSize.width ,
                                             dialogSize.height) display:NO];
            
    [addPanelSelector runModal];
    [mainWindowController changeViewController:addPanelSelector.responseCode];
}

#pragma mark - Core Data stack

@synthesize persistentContainer = _persistentContainer;

- (NSPersistentContainer *)persistentContainer {
    // The persistent container for the application. This implementation creates and returns a container, having loaded the store for the application to it.
    @synchronized (self) {
        if (_persistentContainer == nil) {
            _persistentContainer = [[NSPersistentContainer alloc] initWithName:@"About Arroyo Control"];
            [_persistentContainer loadPersistentStoresWithCompletionHandler:^(NSPersistentStoreDescription *storeDescription, NSError *error) {
                if (error != nil) {
                    // Replace this implementation with code to handle the error appropriately.
                    // abort() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development.
                    
                    /*
                     Typical reasons for an error here include:
                     * The parent directory does not exist, cannot be created, or disallows writing.
                     * The persistent store is not accessible, due to permissions or data protection when the device is locked.
                     * The device is out of space.
                     * The store could not be migrated to the current model version.
                     Check the error message to determine what the actual problem was.
                    */
                    NSLog(@"Unresolved error %@, %@", error, error.userInfo);
                    abort();
                }
            }];
        }
    }
    
    return _persistentContainer;
}

#pragma mark - Core Data Saving and Undo support

- (NSUndoManager *)windowWillReturnUndoManager:(NSWindow *)window {
    // Returns the NSUndoManager for the application. In this case, the manager returned is that of the managed object context for the application.
    return self.persistentContainer.viewContext.undoManager;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    NSAlert *alertBox = [[NSAlert alloc] init];
    [alertBox setAlertStyle:NSAlertStyleWarning];
    [alertBox addButtonWithTitle:@"Cancel"];
    [alertBox addButtonWithTitle:@"OK"];
    [alertBox setMessageText:@"Closing Arroy Control..."];
    [alertBox setInformativeText:@"Are you sure?"];
    [alertBox setIcon:[NSApp applicationIconImage]];

    NSModalResponse response = [ alertBox runModal];
    if (response == 1000) { // first button (Cancel) response code.
        return NSTerminateCancel;
    }

    return NSTerminateNow;
}

@end
