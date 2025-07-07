#include <iostream>

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <internal/libfreenect2/depth_packet_processor.h>
//#include "libfreenect2/depth_packet_processor.h"

#ifndef __KINECT_WRAPPER_H__
#define __KINECT_WRAPPER_H__

enum FRAMETYPE {
    Color = libfreenect2::Frame::Color,
    Depth = libfreenect2::Frame::Depth
};

class CustomFrameListener : public libfreenect2::SyncMultiFrameListener
{
public:
    CustomFrameListener(unsigned int frame_types);
    virtual ~CustomFrameListener();
    
    // Set a callback function to be called on new frames
    void setCallback(void (*callback)(void*), void* user_data);
    
    // Override onNewFrame to trigger callback
    virtual bool onNewFrame(libfreenect2::Frame::Type type, libfreenect2::Frame* frame) override;
    
private:
    void (*callback_function)(void*);
    void* callback_user_data;
};

class kinect_wrapper {
public:
    kinect_wrapper();
    ~kinect_wrapper();
    
    bool isOpen = false;
    
    libfreenect2::Frame undistorted = libfreenect2::Frame(512, 424, 4);
    libfreenect2::Frame registered = libfreenect2::Frame(512, 424, 4);
    
    bool open(long depth_pipeline /* 0-CPU, 1-OpenGL, 2-OpenCL */);
    void setMaxDepth(float m);
    bool hasNewFrames();
    libfreenect2::FrameMap getframes();
    libfreenect2::Frame* frame(FRAMETYPE type);
    void registerFrames();
    void getPoint3D(int r, int c, float & x, float & y, float & z);
    void release();
    void close();
    
    void setFrameCallback(void (*callback)(void*), void* user_data);
    
private:
    libfreenect2::Freenect2 freenect2;
    libfreenect2::Freenect2Device *device;
    libfreenect2::PacketPipeline *pipeline;
    libfreenect2::DepthPacketProcessor::Config config;
    libfreenect2::Registration *registration;
    CustomFrameListener listener;
    libfreenect2::FrameMap frames;
};

#endif /*__KINECT_WRAPPER_H__*/
