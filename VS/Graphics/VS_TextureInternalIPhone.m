/*
 *  VS_TextureInternalIPhone.mm
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/08/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

//#import "VS_TextureInternalIPhone.h"

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreGraphics/CGImage.h>
#import <Foundation/NSObject.h>
#import <UIKit/UIKit.h>
#import <UIKit/UIImage.h>

GLuint loadTexture( const char *name_in )
{
	GLuint location;
	
	glGenTextures(1, &location);
	
	NSString *name = [NSString stringWithUTF8String:name_in];  
	
	CGImageRef textureImage = [UIImage imageNamed:name].CGImage;
	if (textureImage == nil) {
        NSLog(@"Failed to load texture image");
		return 0;
    }
	
    NSInteger texWidth = CGImageGetWidth(textureImage);
    NSInteger texHeight = CGImageGetHeight(textureImage);
	
	GLubyte *textureData = (GLubyte *)malloc(texWidth * texHeight * 4);
	
    CGContextRef textureContext = CGBitmapContextCreate(textureData,
														texWidth, texHeight,
														8, texWidth * 4,
														CGImageGetColorSpace(textureImage),
														kCGImageAlphaPremultipliedLast);
	
	// Rotate the image
	//CGContextTranslateCTM(textureContext, 0, texHeight);
	//CGContextScaleCTM(textureContext, 1.0, -1.0);
	
	CGContextDrawImage(textureContext, CGRectMake(0.0, 0.0, (float)texWidth, (float)texHeight), textureImage);
	CGContextRelease(textureContext);
	
	glBindTexture(GL_TEXTURE_2D, location);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
	
	free(textureData);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	return location;
}

