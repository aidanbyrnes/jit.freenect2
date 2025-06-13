/**
 @file
 ta.jit.kinect2 - reads rgb and depth matrix from Kinect v2
 using libfreenect2 (https://github.com/OpenKinect/libfreenect2)
 
 @ingroup	examples
	@see		ta.jit.kinect2
 
	Copyright 2015 - Tiago Ângelo aka p1nh0 (p1nh0.c0d1ng@gmail.com) — Digitópia/Casa da Música
 */

#include "jit.common.h"
#include "kinect_wrapper.h"
#include <iostream>

// matrix dimensions
//#define RGB_WIDTH 1920 //AB: remove RGB dim for now. Using registered RGB frames which use depth dim
//#define RGB_HEIGHT 1080
#define DEPTH_WIDTH 512
#define DEPTH_HEIGHT 424


// Our Jitter object instance data
typedef struct _jit_freenect2 {
    t_object ob;
    long depth_processor;

    kinect_wrapper *kinect;

    /*
     
     AB: all of this now exists in the kinect_wrapper class
     
    libfreenect2::Freenect2 freenect2;
    libfreenect2::Registration *registration; // AB: declare registration
    libfreenect2::Freenect2Device *device; // TA: declare freenect2 device
    libfreenect2::PacketPipeline *pipeline; // TA: declare packet pipeline
    libfreenect2::SyncMultiFrameListener *listener; //TA: depth frame listener
    libfreenect2::FrameMap *frame_map; // TA: frame map (contains all frames: depth, rgb, etc...)
    t_bool isOpen;
    */
} t_jit_freenect2;


// prototypes
BEGIN_USING_C_LINKAGE
t_jit_err		jit_freenect2_init				(void);
t_jit_freenect2	*jit_freenect2_new				(void);
void			jit_freenect2_free				(t_jit_freenect2 *x);
t_jit_err       jit_freenect2_has_new_frames    (t_jit_freenect2 *x);
t_jit_err		jit_freenect2_matrix_calc		(t_jit_freenect2 *x, void *inputs, void *outputs);

void jit_freenect2_copy_depthdata(t_jit_freenect2 *x, long dimcount, t_jit_matrix_info *out_minfo, char *bop);
void jit_freenect2_copy_rgbdata(t_jit_freenect2 *x, long dimcount, t_jit_matrix_info *out_minfo, char *bop);
void            jit_freenect2_open(t_jit_freenect2 *x);
void            jit_freenect2_close(t_jit_freenect2 *x);
END_USING_C_LINKAGE


// globals
static void *s_jit_freenect2_class = NULL;

/************************************************************************************/

t_jit_err jit_freenect2_init(void)
{
    long			attrflags = JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW;
    t_jit_object	*attr;
    t_jit_object	*mop;

    
    s_jit_freenect2_class = jit_class_new("jit_freenect2", (method)jit_freenect2_new, (method)jit_freenect2_free, sizeof(t_jit_freenect2), 0);
    
    // add matrix operator (mop)
    mop = (t_jit_object *)jit_object_new(_jit_sym_jit_mop, 0, 2); // args are  num inputs and num outputs
    jit_class_addadornment(s_jit_freenect2_class, mop);
    
    // add method(s)
    jit_class_addmethod(s_jit_freenect2_class, (method)jit_freenect2_matrix_calc, "matrix_calc", A_CANT, 0);
    jit_class_addmethod(s_jit_freenect2_class, (method)jit_freenect2_open, "open", 0);
    jit_class_addmethod(s_jit_freenect2_class, (method)jit_freenect2_close, "close", 0);
    jit_class_addmethod(s_jit_freenect2_class, (method)jit_freenect2_has_new_frames, "has_new_frames", A_CANT, 0);
    
    // add attribute(s)
    attr = (t_jit_object *)jit_object_new(_jit_sym_jit_attr_offset,
                                          "depth_processor",
                                          _jit_sym_long,
                                          attrflags,
                                          (method)NULL, (method)NULL,
                                          calcoffset(t_jit_freenect2, depth_processor));
    
    jit_class_addattr(s_jit_freenect2_class, attr);
    
    // finalize class
    jit_class_register(s_jit_freenect2_class);
    return JIT_ERR_NONE;
}


/************************************************************************************/
// Object Life Cycle

t_jit_freenect2 *jit_freenect2_new(void)
{
    t_jit_freenect2	*x = NULL;
    
    x = (t_jit_freenect2 *)jit_object_alloc(s_jit_freenect2_class);
    // TA: initialize other data or structs
    if (x) {
        x->depth_processor = 1; //AB: change default pipeline to OpenGL
        //x->freenect2 = *new libfreenect2::Freenect2();
        /*x->device = 0; //TA: init device
        x->pipeline = 0; //TA: init pipeline
        x->isOpen = false;*/
        x->kinect = new kinect_wrapper();
    }
    
    return x;
}


void jit_freenect2_free(t_jit_freenect2 *x)
{
    post("closing device...");
    if (x->kinect->isOpen == false) {
        return; // quit close method if no device is open
    }
    
    x->kinect->close();
    x->kinect->release();
    
    delete x->kinect;
    /*x->device->stop();
    x->device->close();
    
    x->listener->release(*x->frame_map);
    x->listener = NULL;
    x->frame_map = NULL;
    x->device = NULL;
    x->pipeline = NULL;*/
}

/************************************************************************************/
// TA: METHODS BOUND TO KINECT

//TA: open kinect device
void jit_freenect2_open(t_jit_freenect2 *x){
    if(x->kinect->isOpen){
        post("Device already open");
        return;
    }

    x->kinect->open(x->depth_processor);
}

//TA: close kinect device
void jit_freenect2_close(t_jit_freenect2 *x){
    post("closing device...");
    if (x->kinect->isOpen == false) {
        return; // quit close method if no device is open
    }
    
    x->kinect->close();
    //x->kinect->release();
    
    post("device closed");
}

t_jit_err jit_freenect2_has_new_frames(t_jit_freenect2 *x)
{
    if (!x || !x->kinect || !x->kinect->isOpen) {
        return JIT_ERR_GENERIC; // No device or not open
    }
    
    if (x->kinect->hasNewFrames()) {
        return JIT_ERR_NONE; // Has new frames
    }
    
    return JIT_ERR_GENERIC; // No new frames
}

/************************************************************************************/
// Methods bound to input/inlets

t_jit_err jit_freenect2_matrix_calc(t_jit_freenect2 *x, void *inputs, void *outputs)
{
    t_jit_err			err = JIT_ERR_NONE;
    long				rgb_savelock;
    long				depth_savelock;
    t_jit_matrix_info	rgb_minfo;
    t_jit_matrix_info	depth_minfo;
    char				*rgb_bp;
    char				*depth_bp;
    void				*rgb_matrix;
    void				*depth_matrix;
    
    // Early exit if device is not open or no new frames available
    if (!x->kinect->isOpen || !x->kinect->hasNewFrames()) {
        return JIT_ERR_NONE; // Return success but don't process matrices
    }
    
    rgb_matrix 	= jit_object_method(outputs,_jit_sym_getindex,1);
    depth_matrix = jit_object_method(outputs,_jit_sym_getindex,0);
    
    if (x && depth_matrix && rgb_matrix) {
        rgb_savelock = (long) jit_object_method(rgb_matrix, _jit_sym_lock, 1);
        depth_savelock = (long) jit_object_method(depth_matrix, _jit_sym_lock, 1);
        
        jit_object_method(rgb_matrix, _jit_sym_getinfo, &rgb_minfo);
        jit_object_method(depth_matrix, _jit_sym_getinfo, &depth_minfo);
        
        jit_object_method(rgb_matrix, _jit_sym_getdata, &rgb_bp);
        jit_object_method(depth_matrix, _jit_sym_getdata, &depth_bp);
        
        if (!rgb_bp) {
            err=JIT_ERR_INVALID_INPUT;
            goto out;
        }
        if (!depth_bp) {
            err=JIT_ERR_INVALID_OUTPUT;
            goto out;
        }
        
        /************************************************************************************/
        x->kinect->getframes();
        //AB: register here to get both the RGB and depth frames
        x->kinect->registerFrames();
        
        jit_freenect2_copy_rgbdata(x, rgb_minfo.dimcount, &rgb_minfo, rgb_bp);
        jit_freenect2_copy_depthdata(x, depth_minfo.dimcount, &depth_minfo, depth_bp);

        x->kinect->release();
        //x->listener->release(*x->frame_map);
        /************************************************************************************/
        
    }
    else
        return JIT_ERR_INVALID_PTR;
    
out:
    jit_object_method(depth_matrix,_jit_sym_lock,depth_savelock);
    jit_object_method(rgb_matrix,_jit_sym_lock,rgb_savelock);
    return err;
}


/*********************************RGB************************************************/
void jit_freenect2_looprgb(t_jit_freenect2 *x, t_jit_op_info *out_opinfo, t_jit_matrix_info *out_minfo, char *bop)
{
    long xPos, yPos;
        
    // Correctly access the frame data as unsigned char pointer
    unsigned char *frame_data = (unsigned char *)x->kinect->registered.data;
    out_opinfo->p = bop;
    unsigned char *op = (unsigned char *)out_opinfo->p;
    unsigned char *aPos;
    
    for(yPos = 0; yPos < DEPTH_HEIGHT; yPos++){
        for(xPos = 0; xPos < DEPTH_WIDTH; xPos++){
            
            // Flip horizontally
            long flipped_x = DEPTH_WIDTH - 1 - xPos;
            unsigned char *source_pixel = frame_data + (yPos * DEPTH_WIDTH + flipped_x) * 4;
            aPos = source_pixel + 3;
                        
            //TA: alpha
            *op = *aPos;
            op++;
            aPos--;
            //TA: red
            *op = *aPos;
            op++;
            aPos--;
            //TA: green
            *op = *aPos;
            op++;
            aPos--;
            //TA: blue
            *op = *aPos;
            op++;
            aPos--;
        }
    }
}

void jit_freenect2_copy_rgbdata(t_jit_freenect2 *x, long dimcount, t_jit_matrix_info *out_minfo, char *bop)
{
    t_jit_op_info	out_opinfo;
    
    if (dimcount < 1)
        return; // safety
    //else:
    jit_freenect2_looprgb(x, &out_opinfo, out_minfo, bop);
}

/********************************DEPTH***********************************************/
void jit_freenect2_loopdepth(t_jit_freenect2 *x, t_jit_op_info *out_opinfo, t_jit_matrix_info *out_minfo, char *bop)
{
    int xPos, yPos;
    
    out_opinfo->p = bop;
    float *op = (float *)out_opinfo->p;
    
    float x_coord, y_coord, z_coord;
    
    for(yPos = 0; yPos < DEPTH_HEIGHT; yPos++){
        for(xPos = DEPTH_WIDTH - 1; xPos >= 0; xPos--){
            
            //AB: get 3D point from depth
            x->kinect->getPoint3D(yPos, xPos, x_coord, y_coord, z_coord);
            
            *op = -x_coord;
            op++;
            
            *op = -y_coord;
            op++;
            
            *op = -z_coord;
            op++;
        }
    }
}


void jit_freenect2_copy_depthdata(t_jit_freenect2 *x, long dimcount, t_jit_matrix_info *out_minfo, char *bop)
{
    t_jit_op_info	out_opinfo;
    
    if (dimcount < 1)
        return; // safety
    // else:
    jit_freenect2_loopdepth(x, &out_opinfo, out_minfo, bop);
}

