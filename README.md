# jit.freenect2
<img align="right" width="12.5%" src="https://github.com/aidanbyrnes/jit.freenect2/blob/master/icon.png?raw=true"/>

MaxMSP external to provide multi-platform support for the Kinect v2.

Based on [ta.jit.kinect2](https://github.com/Digitopia/ta.jit.kinect2) by Tiago Ângelo and uses [libfreenect2](https://github.com/OpenKinect/libfreenect2).

### macOS / Xcode

* Make sure the following are available: wget, git, cmake, pkg-config
* Download source
    ```
    git clone --recursive https://github.com/aidanbyrnes/jit.freenect2.git
    cd jit.freenect2
    ```
* Install dependencies for libfreenect2: libusb, GLFW
    ```
    brew update
    brew install libusb
    brew install glfw3
    ```
* Create Xcode project
    ```
    mkdir build && cd build
    cmake -G Xcode ..
    ```
