//
//  kinect_wrapper.h
//  jit.freenect2
//
//  Created by Aidan Byrnes on 6/12/25.
//

#include <iostream>

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>

enum FRAMETYPE {
    Color = libfreenect2::Frame::Color,
    Depth = libfreenect2::Frame::Depth
};

class kinect_wrapper {
public:
    
    bool isOpen = false;
    
    libfreenect2::Freenect2 freenect2;
    libfreenect2::Freenect2Device *device;
    libfreenect2::PacketPipeline *pipeline;
    libfreenect2::Registration *registration;
    
    libfreenect2::SyncMultiFrameListener listener = libfreenect2::SyncMultiFrameListener(libfreenect2::Frame::Color | libfreenect2::Frame::Depth);
    libfreenect2::FrameMap frames;
    libfreenect2::Frame undistorted = libfreenect2::Frame(512, 424, 4);
    libfreenect2::Frame registered = libfreenect2::Frame(512, 424, 4);
    
    bool open(long depth_pipeline /* 0-CPU, 1-OpenGL, 2-OpenCL */);
    bool hasNewFrames();
    libfreenect2::FrameMap getframes();
    libfreenect2::Frame* frame(FRAMETYPE type);
    void registerFrames();
    void getPoint3D(int r, int c, float & x, float & y, float & z);
    void release();
    void close();
    
};
