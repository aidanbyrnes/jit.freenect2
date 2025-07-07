#include "jit.common.h"
#include "kinect_wrapper.h"
#include <iostream>

// matrix dimensions
#define DEPTH_WIDTH 512
#define DEPTH_HEIGHT 424

// Our Jitter object instance data
typedef struct _jit_freenect2 {
    t_object ob;
    long depth_processor;
    float max_depth;
    kinect_wrapper *kinect;
} t_jit_freenect2;


// prototypes
BEGIN_USING_C_LINKAGE
t_jit_err        jit_freenect2_init                (void);
t_jit_freenect2    *jit_freenect2_new                (void);
void            jit_freenect2_free                (t_jit_freenect2 *x);
t_jit_err        jit_freenect2_matrix_calc        (t_jit_freenect2 *x, void *inputs, void *outputs);
void            jit_freenect2_open(t_jit_freenect2 *x);
void            jit_freenect2_close(t_jit_freenect2 *x);
kinect_wrapper* jit_freenect2_get_kinect_wrapper(t_jit_freenect2 *x);
void            jit_freenect2_copy_depthdata(t_jit_freenect2 *x, long dimcount, t_jit_matrix_info *out_minfo, char *bop);
void            jit_freenect2_copy_rgbdata(t_jit_freenect2 *x, long dimcount, t_jit_matrix_info *out_minfo, char *bop);
t_jit_err jit_freenect2_max_depth_set(t_jit_freenect2* x, void* attr, long ac, t_atom* av);
END_USING_C_LINKAGE


// globals
static void *s_jit_freenect2_class = NULL;

/************************************************************************************/

t_jit_err jit_freenect2_init(void)
{
    long attrflags = JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW;
    t_jit_object *attr;
    t_jit_object *mop;

    s_jit_freenect2_class = jit_class_new("jit_freenect2", (method)jit_freenect2_new, (method)jit_freenect2_free, sizeof(t_jit_freenect2), 0);
    
    mop = (t_jit_object *)jit_object_new(_jit_sym_jit_mop, 0, 2);
    jit_class_addadornment(s_jit_freenect2_class, mop);
    
    jit_class_addmethod(s_jit_freenect2_class, (method)jit_freenect2_matrix_calc, "matrix_calc", A_CANT, 0);
    jit_class_addmethod(s_jit_freenect2_class, (method)jit_freenect2_open, "open", 0);
    jit_class_addmethod(s_jit_freenect2_class, (method)jit_freenect2_close, "close", 0);
    jit_class_addmethod(s_jit_freenect2_class, (method)jit_freenect2_get_kinect_wrapper, "get_kinect_wrapper", A_CANT, 0);

    attr = (t_jit_object *)jit_object_new(_jit_sym_jit_attr_offset,
                                          "depth_processor", _jit_sym_long, attrflags,
                                          (method)NULL, (method)NULL, calcoffset(t_jit_freenect2, depth_processor));
    object_addattr_parse(attr, "label", _jit_sym_symbol, 0, "\"Depth processor\"");
    object_addattr_parse(attr, "style", _jit_sym_symbol, 0, "enumindex");
    object_addattr_parse(attr, "enumvals", _jit_sym_symbol, 0, "CPU OpenGL OpenCL");
    jit_class_addattr(s_jit_freenect2_class, attr);
    
    attr = (t_jit_object *)jit_object_new(_jit_sym_jit_attr_offset,
                                          "max_depth", _jit_sym_float32, attrflags,
                                          (method)NULL, (method)jit_freenect2_max_depth_set, calcoffset(t_jit_freenect2, max_depth));
    object_addattr_parse(attr, "label", _jit_sym_symbol, 0, "\"Maximum depth\"");
    jit_class_addattr(s_jit_freenect2_class, attr);
    
    jit_class_register(s_jit_freenect2_class);
    return JIT_ERR_NONE;
}

/************************************************************************************/

t_jit_freenect2 *jit_freenect2_new(void)
{
    t_jit_freenect2 *x = NULL;
    x = (t_jit_freenect2 *)jit_object_alloc(s_jit_freenect2_class);
    if (x) {
        x->depth_processor = 1;
        x->max_depth = 4.5f;
        x->kinect = new kinect_wrapper();
    }
    return x;
}

void jit_freenect2_free(t_jit_freenect2 *x)
{
    if(x->kinect != NULL){
        delete x->kinect;
    }
}

/************************************************************************************/

void jit_freenect2_open(t_jit_freenect2 *x){
    if(x->kinect->isOpen){
        post("Device already open");
        return;
    }
    x->kinect->open(x->depth_processor);
}

t_jit_err jit_freenect2_max_depth_set(t_jit_freenect2* x, void* attr, long ac, t_atom* av){
    x->max_depth = atom_getfloat(av);
    x->kinect->setMaxDepth(x->max_depth);
}

kinect_wrapper* jit_freenect2_get_kinect_wrapper(t_jit_freenect2 *x)
{
    return x->kinect;
}

void jit_freenect2_close(t_jit_freenect2 *x){
    x->kinect->close();
    post("device closed");
}

/************************************************************************************/

t_jit_err jit_freenect2_matrix_calc(t_jit_freenect2 *x, void *inputs, void *outputs)
{
    t_jit_err err = JIT_ERR_NONE;
    long rgb_savelock, depth_savelock;
    t_jit_matrix_info rgb_minfo, depth_minfo;
    char *rgb_bp, *depth_bp;
    void *rgb_matrix, *depth_matrix;
    
    if (!x->kinect->isOpen || !x->kinect->hasNewFrames()) {
        return JIT_ERR_NONE;
    }
    
    rgb_matrix = jit_object_method(outputs,_jit_sym_getindex,1);
    depth_matrix = jit_object_method(outputs,_jit_sym_getindex,0);
    
    if (x && depth_matrix && rgb_matrix) {
        rgb_savelock = (long) jit_object_method(rgb_matrix, _jit_sym_lock, 1);
        depth_savelock = (long) jit_object_method(depth_matrix, _jit_sym_lock, 1);
        
        jit_object_method(rgb_matrix, _jit_sym_getinfo, &rgb_minfo);
        jit_object_method(depth_matrix, _jit_sym_getinfo, &depth_minfo);
        
        jit_object_method(rgb_matrix, _jit_sym_getdata, &rgb_bp);
        jit_object_method(depth_matrix, _jit_sym_getdata, &depth_bp);
        
        if (!rgb_bp) { err=JIT_ERR_INVALID_INPUT; goto out; }
        if (!depth_bp) { err=JIT_ERR_INVALID_OUTPUT; goto out; }
        
        x->kinect->getframes();
        x->kinect->registerFrames();
        
        jit_freenect2_copy_rgbdata(x, rgb_minfo.dimcount, &rgb_minfo, rgb_bp);
        jit_freenect2_copy_depthdata(x, depth_minfo.dimcount, &depth_minfo, depth_bp);

        x->kinect->release();
    }
    else {
        return JIT_ERR_INVALID_PTR;
    }
    
out:
    jit_object_method(depth_matrix, _jit_sym_lock, depth_savelock);
    jit_object_method(rgb_matrix, _jit_sym_lock, rgb_savelock);
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

