#include "jit.common.h"
#include "kinect_wrapper.h"

CustomFrameListener::CustomFrameListener(unsigned int frame_types)
    : libfreenect2::SyncMultiFrameListener(frame_types), callback_function(nullptr), callback_user_data(nullptr) {
}

CustomFrameListener::~CustomFrameListener() {
}

void CustomFrameListener::setCallback(void (*callback)(void *), void *user_data) {
    callback_function = callback;
    callback_user_data = user_data;
}

bool CustomFrameListener::onNewFrame(libfreenect2::Frame::Type type, libfreenect2::Frame *frame) {
    bool result = libfreenect2::SyncMultiFrameListener::onNewFrame(type, frame);

    // Send as soon as depth frame is available
    if (callback_function && callback_user_data && type == libfreenect2::Frame::Depth) {
        callback_function(callback_user_data);
    }

    return result;
}

kinect_wrapper::kinect_wrapper() : listener(libfreenect2::Frame::Color | libfreenect2::Frame::Depth) {
    device = nullptr;
    pipeline = nullptr;
    config.MaxDepth = 4.5f;
    registration = nullptr;
    isOpen = false;
}

kinect_wrapper::~kinect_wrapper() {
    close();
}

void kinect_wrapper::setFrameCallback(void (*callback)(void *), void *user_data) {
    listener.setCallback(callback, user_data);
}

bool kinect_wrapper::open(long depth_pipeline = 0) {
    if (freenect2.enumerateDevices() == 0) {
        post("Could not find device");
        isOpen = false;
        return isOpen;
    }

    switch (depth_pipeline) {
        case 1:
            pipeline = new libfreenect2::OpenGLPacketPipeline();
            post("Using OpenGL packet pipeline");
            break;

        case 2:
            pipeline = new libfreenect2::OpenCLPacketPipeline();
            post("Using OpenCL packet pipeline");
            break;

        default:
            pipeline = new libfreenect2::CpuPacketPipeline();
            post("Using CPU packet pipeline");
            break;
    }

    libfreenect2::DepthPacketProcessor *depthProcessor = pipeline->getDepthPacketProcessor();
    depthProcessor->setConfiguration(config);

    device = freenect2.openDefaultDevice(pipeline);

    if (device == 0) {
        post("Could not open device");
        isOpen = false;
        return isOpen;
    }

    device->setColorFrameListener(&listener);
    device->setIrAndDepthFrameListener(&listener);

    if (!device->start()) {
        post("Could not start device");
        isOpen = false;
        return isOpen;
    }

    registration = new libfreenect2::Registration(device->getIrCameraParams(), device->getColorCameraParams());
    isOpen = true;
    post("Device is ready");
    return isOpen;
}

void kinect_wrapper::setMaxDepth(float m) {
    config.MaxDepth = m;

    if (!pipeline) {
        return;
    }

    libfreenect2::DepthPacketProcessor *depthProcessor = pipeline->getDepthPacketProcessor();
    depthProcessor->setConfiguration(config);
}

bool kinect_wrapper::hasNewFrames() {
    return listener.hasNewFrame();
}

libfreenect2::FrameMap kinect_wrapper::getframes() {
    listener.waitForNewFrame(frames);
    return frames;
}

libfreenect2::Frame * kinect_wrapper::frame(FRAMETYPE type) {
    switch (type) {
        case Color:
            return frames[libfreenect2::Frame::Color];

        case Depth:
            return frames[libfreenect2::Frame::Depth];
    }
}

void kinect_wrapper::registerFrames() {
    registration->apply(frames[libfreenect2::Frame::Color], frames[libfreenect2::Frame::Depth], &undistorted, &registered);
}

void kinect_wrapper::getPoint3D(int r, int c, float &x, float &y, float &z) {
    registration->getPointXYZ(&undistorted, r, c, x, y, z);
}

void kinect_wrapper::release() {
    listener.release(frames);
}

void kinect_wrapper::close() {
    setFrameCallback(nullptr, nullptr);

    if (isOpen) {
        device->stop();
        device->close();
        isOpen = false;
    }

    if (registration) {
        delete registration;
        registration = nullptr;
    }
}
