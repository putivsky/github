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

#import <Cocoa/Cocoa.h>
#include <string>
#include <map>

@class MainWindowController, PanelViewController;

NS_ASSUME_NONNULL_BEGIN

@interface FrameViewController : NSViewController

@property (nonatomic, strong) IBOutlet NSImageView *panelImage;
@property (nonatomic, strong) IBOutlet NSButton *toggleButton;

@property (nonatomic, strong) MainWindowController *mainWindowController;
@property (nonatomic) NSRect originalRect;
@property (nonatomic) bool hiddenState;
@property (nonatomic) bool isConnected;
@property (nonatomic) NSInteger panelType;

-(void) setConnected:(BOOL)state;
-(bool) executeCommand:(const std::string&) command response:(std::string* _Nullable)response error:(std::string&)error;
-(void) writeLogEntry:(const std::string&) errorMessage;
-(std::unordered_map<std::string, std::string>) parseResponse:(const std::string&)command response:(const std::string&)response;
-(void) notifyHideLogWindow;
-(void) updateLogWindow;

@end

NS_ASSUME_NONNULL_END
