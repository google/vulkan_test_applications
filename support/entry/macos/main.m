/* Copyright 2019 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

NSThread* evtThread;
void* CreateMacOSWindow(uint32_t width, uint32_t height) {
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  id applicationName = [[NSProcessInfo processInfo] processName];
  NSUInteger windowStyle = NSTitledWindowMask  | NSClosableWindowMask | NSResizableWindowMask | NSMiniaturizableWindowMask;

  id window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, width, height)
                                          styleMask:windowStyle backing:NSBackingStoreBuffered defer:NO];
  [window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
  [window setTitle: applicationName];
  [window makeKeyAndOrderFront:nil];
  [NSApp activateIgnoringOtherApps:YES];
  NSView* view = [window contentView];
  view.wantsLayer = YES;
  view.layer = [CAMetalLayer layer];
  return view;
}
bool running = false;
void RunMacOS() {
  running = true;

  [NSApp finishLaunching];

  while (running) {
    @autoreleasepool {
      NSEvent* ev;
      do {
        ev = [NSApp nextEventMatchingMask: NSAnyEventMask
                                untilDate: nil
                                   inMode: NSDefaultRunLoopMode
                                  dequeue: YES];
        if (ev) {
          // handle events here
          [NSApp sendEvent: ev];
        }
      } while (ev);
    }
  }

}

void StopMacOS() {
  running = false;
}
