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
#include "LaserPanelViewController.h"
#include "TECPanelViewController.h"
#include "FrameViewController.h"

@interface MainWindowController ()

@property (nonatomic, strong) NSSplitViewController *mainSplitViewController;
@property (nonatomic, strong) IBOutlet NSImageView *logoImage;
@property (nonatomic, strong) IBOutlet NSButton *addButton;
@property (nonatomic, strong) IBOutlet NSButton *exitButton;

@end

@implementation MainWindowController

@synthesize mainSplitViewController;

static NSString* kHidePanel = @"Hide\nPanel";
static NSString* kShowPanel = @"Show\nPanel";

- (void)windowDidLoad {
    [super windowDidLoad];
    
    if (self.mainSplitViewController == nil)
    {
        self.mainSplitViewController = [[NSSplitViewController alloc] init];
        [hostView addSubview:[self.mainSplitViewController view]];
        // make sure we automatically resize the controller's view to the current window size
        [[self.mainSplitViewController view] setFrame:[hostView bounds]];
    }
}

- (void)changeViewController:(NSInteger)whichViewTag
{
    LaserPanelViewController *laserController;
    TECPanelViewController* tecController;
    NSString *imageName;
    
    // Frame controller new instance
    FrameViewController* frameController = [[FrameViewController alloc] initWithNibName:@"FrameViewController" bundle:nil];
    [frameController setMainWindowController:self];
    [frameController setPanelType:whichViewTag];
        
    switch (whichViewTag)
    {
        case kLaserView:
            laserController = [[LaserPanelViewController alloc] initWithNibName:@"LaserPanelViewController" bundle:nil];
            [laserController setFrameController:frameController];
            imageName = @"arroyo_bar_rotate";
            laserController.comboMode = FALSE;
            break;
        case kTECView:
            // Create new instance of TecPanelViewController
            tecController = [[TECPanelViewController alloc] initWithNibName:@"TECPanelViewController" bundle:nil];
            [tecController setFrameController:frameController];
            imageName = @"arroyo_bar_rotate";
            tecController.comboMode = FALSE;
            break;
        case kComboView:
        {
            laserController = [[LaserPanelViewController alloc] initWithNibName:@"LaserPanelViewController" bundle:nil];
            [laserController setFrameController:frameController];
            tecController = [[TECPanelViewController alloc] initWithNibName:@"TECPanelViewController" bundle:nil];
            [tecController setFrameController:frameController];
            imageName = @"arroyo_bar_rotate";
            laserController.comboMode = TRUE;
            tecController.comboMode = TRUE;
            break;
        }
        case kCancelCode:
        {
            return;
        }
            break;
        case kUndefined:
        default:
        {
            NSLog(@"Unrecognized tag is %d", (int)whichViewTag);
            return;
        }
    }
    
    // Force to load view from XIB file
    [frameController loadView];
    // Set custom image for Laser View
    NSImage* image = [NSImage imageNamed:imageName];
    [frameController.panelImage setImage:image];
    
    [self constructNewFramePanel:frameController laser:laserController tec:tecController];
    [self addNewFramePanel:frameController];
}

- (void)constructNewFramePanel:(FrameViewController*)frame laser:(LaserPanelViewController*)laser tec:(TECPanelViewController*)tec {
    // Adjust position & bounds
    NSViewController* panel = (laser ? laser : tec);
    NSViewController* extra = (laser ? tec : NULL);
    
    NSView *panelView = panel.view;
    // Frame bounds
    NSPoint frameOrigin = frame.panelImage.frame.origin;
    NSRect imageRect = frame.panelImage.bounds;
    // Panel bounds
    NSRect panelRect = panelView.bounds;
    // Set origin as frame origin + frame width
    panelRect.origin.x += 2 * frameOrigin.x + imageRect.size.width;
    panelRect.origin.y = 80; // TODO - offset from bottom.
    [panelView setFrame:panelRect];

    // Insert container into responders chain
    [frame addChildViewController:panel];
    // Add child view
    [frame.view addSubview:panelView];
    
    if (extra) {
        NSView *extraView = extra.view;
        // Panel bounds
        NSRect extraRect = extraView.bounds;
        // Set origin as frame origin + frame width
        extraRect.origin.x += 3 * frameOrigin.x + imageRect.size.width + panelRect.size.width;
        extraRect.origin.y = 70; // TODO - offset from bottom.
        [extraView setFrame:extraRect];

        // Insert container into responders chain
        [frame addChildViewController:extra];
        // Add child view
        [frame.view addSubview:extraView];

        NSRect firstPanel = frame.view.frame;
        firstPanel.size.width += frameOrigin.x + extraView.bounds.size.width;
        [frame.view setFrame:firstPanel];
    }
    
    [frame setOriginalRect:frame.view.frame];
    [frame setHiddenState:false];
}

- (void)addNewFramePanel:(FrameViewController*)frame {
    NSSplitViewItem* frameItem = [NSSplitViewItem splitViewItemWithViewController:frame];
    
    // adjust windows size
    [frameItem setMinimumThickness:frame.view.bounds.size.width];
    
    // add new split item
    [self.mainSplitViewController addSplitViewItem:frameItem];
    [self adjustSizes];
}

- (void)removePanel:(FrameViewController*)frame {
    NSUInteger count = self.mainSplitViewController.splitViewItems.count;
    for (int i = 0; i < count; ++i) {
        NSSplitViewItem *item = self.mainSplitViewController.splitViewItems[i];
        if (item.viewController == frame) {
            [self.mainSplitViewController removeSplitViewItem:item];
            break;
        }
    }
    [self adjustSizes];
}

- (void)togglePanel:(FrameViewController*)frame button:(NSButton*)button {
    NSRect rc;
    NSString* title = NULL;
    NSSplitViewItem *viewItem = NULL;
    FrameViewController* item = NULL;
    bool hiddenState = false;
    NSUInteger count = self.mainSplitViewController.splitViewItems.count;
    for (int i = 0; i < count; ++i) {
        viewItem = self.mainSplitViewController.splitViewItems[i];
        if (viewItem.viewController == frame) {
            item = (FrameViewController*)viewItem.viewController;
            if ([button.title isEqual:kShowPanel]) {
                title = kHidePanel;
                rc = item.originalRect;
                hiddenState = false;
            } else {
                title = kShowPanel;
                rc = item.view.frame;
                NSRect imageFrame = item.panelImage.frame;
                rc.size.width = 2 * (imageFrame.origin.x - rc.origin.x) + imageFrame.size.width;
                hiddenState = true;
            }
            break;
        }
    }
    
    if ([title isEqual:kShowPanel]) {
        bool foundFullPanel = false;
        for (int i = 0; i < count && !foundFullPanel; ++i) {
            FrameViewController* it = (FrameViewController*)self.mainSplitViewController.splitViewItems[i].viewController;
            foundFullPanel = (it != frame && !it.hiddenState);
        }
        
        if (!foundFullPanel) {
            NSBeep();
            return;
        }
    }

    [button setTitle:title];
    [item setHiddenState:hiddenState];
    // adjust windows size
    [viewItem setMinimumThickness:rc.size.width];
    [viewItem.viewController.view setFrame:rc];

    [self adjustSizes];
}

- (void)adjustSizes {
    NSUInteger count = self.mainSplitViewController.splitViewItems.count;
    if (count == 0) {
        return;
    }
    
    NSSplitViewItem* viewItem = self.mainSplitViewController.splitViewItems[0];
    FrameViewController* item = (FrameViewController*)viewItem.viewController;
    
    if (count == 1 && item.hiddenState) {
        // Last item can't be in a hidden state
        NSRect rc = item.originalRect;
        [item setHiddenState:FALSE];
        [item.toggleButton setTitle:kHidePanel];
        [viewItem setMinimumThickness:rc.size.width];
        [viewItem.viewController.view setFrame:rc];
    }

    item.toggleButton.enabled = (count == 1 ? FALSE : TRUE);

    int width = 0;
    for (int i = 0; i < count; ++i) {
        NSSplitViewItem *item = self.mainSplitViewController.splitViewItems[i];
        width += item.viewController.view.frame.size.width;
    }
    
    // main app window resizing
    NSWindow* mainWindow = [[[NSApplication sharedApplication] windows] firstObject];
    NSRect mainFrame = mainWindow.frame;
    mainFrame.size.width = width;
    [mainWindow setFrame: mainFrame display: NO animate: NO];
    
    // logo image resizing
    NSRect logoImageFrame = self.logoImage.frame;
    logoImageFrame.size.width = width;
    [self.logoImage setFrame:logoImageFrame];
    
    // host view resizing
    NSRect hostFrame = hostView.frame;
    hostFrame.size.width = width;
    [hostView setFrame:hostFrame];
    
    // adjust split view window
    [self.mainSplitViewController.view setFrame:hostView.frame];
    
    // add/exit buttons set postiion
    NSRect exitButtonFrame = self.exitButton.frame;
    NSRect addButtonFrame = self.addButton.frame;
    // move buttons into new positions
    exitButtonFrame.origin.x = width - exitButtonFrame.size.width - 10; // TODO
    addButtonFrame.origin.x = exitButtonFrame.origin.x - addButtonFrame.size.width - 10;
    [self.exitButton setFrame:exitButtonFrame];
    [self.addButton setFrame:addButtonFrame];
}

- (void)dealloc
{
    mainSplitViewController = nil;
}

@end
