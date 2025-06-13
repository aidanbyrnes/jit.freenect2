//
//  kinect_wrapper.cpp
//  jit.freenect2
//
//  Created by Aidan Byrnes on 6/12/25.
//

#include <iostream>
#include "jit.common.h"

#include "kinect_wrapper.h"

bool kinect_wrapper::open(long depth_pipeline=0)
{
    if(freenect2.enumerateDevices() == 0)
    {
        post("Could not find device");
        isOpen = false;
        return isOpen;
    }
    
    switch (depth_pipeline) {
        case 1:
            pipeline = new libfreenect2::OpenGLPacketPipeline();
            post("using OpenGL packet pipeline");
            break;
        case 2:
            pipeline = new libfreenect2::OpenCLPacketPipeline();
            post("using OpenCL packet pipeline");
            break;
        default:
            pipeline = new libfreenect2::CpuPacketPipeline();
            post("using CPU packet pipeline");
            break;
    }
    
    device = freenect2.openDefaultDevice(pipeline);
    
    if(device == 0)
    {
        post("Could not open device");
        isOpen = false;
        return isOpen;
    }
    
    device->setColorFrameListener(&listener);
    device->setIrAndDepthFrameListener(&listener);
    
    if (!device->start())
    {
        post("Could not start device");
        isOpen = false;
        return isOpen;
    }
    
    registration = new libfreenect2::Registration(device->getIrCameraParams(), device->getColorCameraParams());
    
    isOpen = true;
    
    post("Device is ready");
    
    return isOpen;
}

bool kinect_wrapper::hasNewFrames(){
    return listener.hasNewFrame();
}

libfreenect2::FrameMap kinect_wrapper::getframes() {
    listener.waitForNewFrame(frames);
    return frames;
}

libfreenect2::Frame* kinect_wrapper::frame(FRAMETYPE type) {
    switch (type)
    {
        case Color:
            return frames[libfreenect2::Frame::Color];
            break;
        case Depth:
            return frames[libfreenect2::Frame::Depth];
            break;
    }
}

void kinect_wrapper::registerFrames(){
    registration->apply(frames[libfreenect2::Frame::Color], frames[libfreenect2::Frame::Depth], &undistorted, &registered);
}

void kinect_wrapper::getPoint3D(int r, int c, float &x, float &y, float &z){
    registration->getPointXYZ(&undistorted, r, c, x, y, z);
}

void kinect_wrapper::release() {
    listener.release(frames);
}

void kinect_wrapper::close() {
    if (isOpen)
    {
        device->stop();
        device->close();
    }
    
    delete registration;
}
