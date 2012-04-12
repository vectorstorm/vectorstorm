//
//  VectorStormIPhoneAppDelegate.m
//  VectorStormIPhone
//
//  Created by Trevor Powell on 21/08/10.
//  Copyright Trevor Powell 2010. All rights reserved.
//

#import "VectorStormIPhoneAppDelegate.h"
#import "EAGLView.h"

#import "Wedge.h"

@implementation VectorStormIPhoneAppDelegate

@synthesize window;
@synthesize glView;


#define MAXPATHLEN (256)
- (void) setupWorkingDirectory
{	
	char dir[MAXPATHLEN];
	
	NSString* readPath = [[NSBundle mainBundle] resourcePath];
	[readPath getCString:dir maxLength:MAXPATHLEN encoding:NSASCIIStringEncoding];
	
	NSLog(@"%@",readPath);
	//NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationDirectory, NSUserDomainMask, YES); //creates paths so that you can pull the app's path from it
	//NSString *documentsDirectory = [paths objectAtIndex:0]; //gets the applications directory on the users iPhone
	//CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	//CFURLGetFileSystemRepresentation(url, true, (UInt8 *)dir, MAXPATHLEN);
	
	//	const char *scoreFile = [[documentsDirectory stringByAppendingString:appBundleName] UTF8String]; //appends scoreFileName to the end of documentsDirectory & converts it to a const char so that fopen can deal with it.
	//	const char *scoreFile = [documentsDirectory UTF8String]; //converts it to a const char so that fopen can deal with it.
	//	assert( chdir( scoreFile ) == 0 );
	
//	int result = chdir( dir );
//	assert( result == 0 );

	NSFileManager *filemgr;
	
	filemgr = [NSFileManager defaultManager];
	if ([filemgr changeCurrentDirectoryPath: readPath] == NO)
	{
        // Directory does not exist – take appropriate action
		assert(0);
	}	
	[filemgr release];
}	

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	[self setupWorkingDirectory];
	
	// Configure and start the accelerometer
	[[UIAccelerometer sharedAccelerometer] setUpdateInterval:(1.0 / 30.f)];
	[[UIAccelerometer sharedAccelerometer] setDelegate:self];
	
	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didRotate:) name:@"UIDeviceOrientationDidChangeNotification" object:nil];	
	[self performSelector:@selector(getOriented) withObject:nil afterDelay:0.0f];
	
    [glView startAnimation];
	return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    [glView stopAnimation];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    [glView startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    [glView stopAnimation];
}


- (void)dealloc
{
    [window release];
    [glView release];

    [super dealloc];
}

- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration {
    // Update the accelerometer graph view
    
	SetAcceleration(acceleration.x, acceleration.y);
}

- (void) didRotate:(NSNotification *)notification
{	
	UIApplication *theApp = [UIApplication sharedApplication];
	UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
	// save orientation somewhere
	
	switch ( orientation )
	{
		case UIDeviceOrientationUnknown:
			//SetDeviceOrientation( Orientation_Unknown );
			//NSLog(@"Unknown");
			break;
		case UIDeviceOrientationPortrait:
			SetDeviceOrientation( Orientation_Portrait );
			theApp.statusBarOrientation = orientation;
			NSLog(@"Portrait");
			break;
		case UIDeviceOrientationPortraitUpsideDown:
			SetDeviceOrientation( Orientation_PortraitUpsideDown );
			theApp.statusBarOrientation = orientation;
			NSLog(@"PortraitUpsideDown");
			break;
		case UIDeviceOrientationLandscapeLeft:
			SetDeviceOrientation( Orientation_LandscapeLeft );
			theApp.statusBarOrientation = orientation;
			NSLog(@"LandscapeLeft");
			break;
		case UIDeviceOrientationLandscapeRight:
			SetDeviceOrientation( Orientation_LandscapeRight );
			theApp.statusBarOrientation = orientation;
			NSLog(@"LandscapeRight");
			break;
		case UIDeviceOrientationFaceUp:
			SetDeviceOrientation( Orientation_FaceUp );
			NSLog(@"FaceUp");
			break;
		case UIDeviceOrientationFaceDown:
			SetDeviceOrientation( Orientation_FaceDown );
			NSLog(@"FaceDown");
			break;
	}
}

- (void)getOriented 
{ 
	UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
	// save orientation somewhere

	switch ( orientation )
	{
		case UIDeviceOrientationUnknown:
			//SetDeviceOrientation( Orientation_Unknown );
			break;
		case UIDeviceOrientationPortrait:
			SetDeviceOrientation( Orientation_Portrait );
			break;
		case UIDeviceOrientationPortraitUpsideDown:
			SetDeviceOrientation( Orientation_PortraitUpsideDown );
			break;
		case UIDeviceOrientationLandscapeLeft:
			SetDeviceOrientation( Orientation_LandscapeLeft );
			break;
		case UIDeviceOrientationLandscapeRight:
			SetDeviceOrientation( Orientation_LandscapeRight );
			break;
		case UIDeviceOrientationFaceUp:
			SetDeviceOrientation( Orientation_FaceUp );
			break;
		case UIDeviceOrientationFaceDown:
			SetDeviceOrientation( Orientation_FaceDown );
			break;
	}
}

@end
