//
//  SprattIPhoneAppDelegate.h
//  SprattIPhone
//
//  Created by Trevor Powell on 21/08/10.
//  Copyright Trevor Powell 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

@class EAGLView;

@interface SprattIPhoneAppDelegate : NSObject <UIApplicationDelegate, UIAccelerometerDelegate> {
    UIWindow *window;
    EAGLView *glView;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet EAGLView *glView;

@end

