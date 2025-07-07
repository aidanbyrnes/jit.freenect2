/**
 @file
 ta.jit.kinect2 - reads rgb and depth matrix from Kinect v2
 using libfreenect2 (https://github.com/OpenKinect/libfreenect2)
 
 @ingroup	examples
	@see		ta.jit.kinect2
 
	Copyright 2015 - Tiago Ângelo aka p1nh0 (p1nh0.c0d1ng@gmail.com) — Digitópia/Casa da Música
 */

#include "jit.common.h"
#include "max.jit.mop.h"

// matrix dimensions
//#define RGB_WIDTH 1920 //AB: remove RGB dim for now. Using registered RGB frames which use depth dim
//#define RGB_HEIGHT 1080
#define DEPTH_WIDTH 512
#define DEPTH_HEIGHT 424



// Max object instance data
// Note: most instance data is in the Jitter object which we will wrap
typedef struct _max_jit_freenect2 {
    t_object	ob;
    void		*obex;
} t_max_jit_freenect2;


// prototypes
BEGIN_USING_C_LINKAGE
t_jit_err	jit_freenect2_init(void);
void		*max_jit_freenect2_new(t_symbol *s, long argc, t_atom *argv);
void		max_jit_freenect2_free(t_max_jit_freenect2 *x);
//TA: own methods
void        max_jit_freenect2_outputmatrix(t_max_jit_freenect2 *x);
void        max_jit_freenect2_bang(t_max_jit_freenect2 *x);
END_USING_C_LINKAGE

// globals
static void	*max_jit_freenect2_class = NULL;


/************************************************************************************/

void ext_main(void *r)
{
    t_class *max_class, *jit_class;
    
    jit_freenect2_init();
    
    max_class = class_new("jit.freenect2", (method)max_jit_freenect2_new, (method)max_jit_freenect2_free, sizeof(t_max_jit_freenect2), NULL, A_GIMME, 0);
    max_jit_class_obex_setup(max_class, calcoffset(t_max_jit_freenect2, obex));
    
    jit_class = jit_class_findbyname(gensym("jit_freenect2"));
    
    max_jit_class_mop_wrap(max_class, jit_class, MAX_JIT_MOP_FLAGS_OWN_OUTPUTMATRIX|MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX|MAX_JIT_MOP_FLAGS_OWN_BANG|MAX_JIT_MOP_FLAGS_OWN_TYPE|MAX_JIT_MOP_FLAGS_OWN_PLANECOUNT|MAX_JIT_MOP_FLAGS_OWN_DIM|MAX_JIT_MOP_FLAGS_OWN_ADAPT|MAX_JIT_MOP_FLAGS_OWN_OUTPUTMODE);			// attrs & methods for name, type, dim, planecount, bang, outputmatrix, etc // TA: overriding methods and attributes
    
    max_jit_class_wrap_standard(max_class, jit_class, 0);		// attrs & methods for getattributes, dumpout, maxjitclassaddmethods, etc
    
    class_addmethod(max_class, (method)max_jit_freenect2_outputmatrix, "outputmatrix", A_USURP_LOW, 0); // TA: override outputmatrix method
    class_addmethod(max_class, (method)max_jit_freenect2_bang, "bang"); // TA: override bang method (it shouldn't be necessary but I'm not getting it to work be only overriding the outputmatrix method)
    class_addmethod(max_class, (method)max_jit_mop_assist, "assist", A_CANT, 0);	// standard matrix-operator (mop) assist fn
    
    class_register(CLASS_BOX, max_class);
    max_jit_freenect2_class = max_class;
}


/************************************************************************************/
// Object Life Cycle

void *max_jit_freenect2_new(t_symbol *s, long argc, t_atom *argv)
{
    t_max_jit_freenect2	*x;
    void			*o;
    
    x = (t_max_jit_freenect2 *)max_jit_object_alloc(max_jit_freenect2_class, gensym("jit_freenect2"));
    if (x) {
        o = jit_object_new(gensym("jit_freenect2"));
        if (o) {
            max_jit_mop_setup_simple(x, o, argc, argv);
            max_jit_attr_args(x, argc, argv);
            t_atom_long depthdim[2] = {DEPTH_WIDTH, DEPTH_HEIGHT};
            t_atom_long rgbdim[2] = {DEPTH_WIDTH, DEPTH_HEIGHT}; //AB: set rgb dim to depth dim since rgb will be registered to depth
            
            //TA: set depth matrix initial attributes
            void *output = max_jit_mop_getoutput(x, 1);
            jit_attr_setsym(output, _jit_sym_type, _jit_sym_float32);
            jit_attr_setlong_array(output, _jit_sym_dim, 2, depthdim);
            jit_attr_setlong(output, _jit_sym_planecount, 3); //AB: matrix should now be 3 planes (x, y, & z)
            
            //TA: set rgb matrix initial attributes
            output = max_jit_mop_getoutput(x, 2);
            jit_attr_setsym(output, _jit_sym_type, _jit_sym_char);
            jit_attr_setlong_array(output, _jit_sym_dim, 2, rgbdim);
            jit_attr_setlong(output, _jit_sym_planecount, 4);

        }
        else {
            jit_object_error((t_object *)x, "jit.freenect2: could not allocate object");
            object_free((t_object *)x);
            x = NULL;
        }
    }
    return (x);
}


void max_jit_freenect2_free(t_max_jit_freenect2 *x)
{
    max_jit_mop_free(x);
    jit_object_free(max_jit_obex_jitob_get(x));
    max_jit_object_free(x);
}
/************************************************************************************/
// TA: Obejct own methods

void max_jit_freenect2_outputmatrix(t_max_jit_freenect2 *x)
{
    void *mop=max_jit_obex_adornment_get(x,_jit_sym_jit_mop);
    t_jit_err err;
    
    if (mop) { //always output
        if ((err=(t_jit_err)jit_object_method(max_jit_obex_jitob_get(x),
                                              _jit_sym_matrix_calc,
                                              jit_object_method(mop,_jit_sym_getinputlist),
                                              jit_object_method(mop,_jit_sym_getoutputlist))))
        {
            jit_error_code(x,err);
        }
        else {
            max_jit_mop_outputmatrix(x);
        }
    }
}

void max_jit_freenect2_bang(t_max_jit_freenect2 *x){
    max_jit_freenect2_outputmatrix(x);
}

//TA: assist method
void max_jit_freenect2_assist(t_max_jit_freenect2 *x, void *b, long msg, long arg, char *s){
    if (msg == ASSIST_INLET) { // inlet assist
        // add inlet assist here
    }
    else{ // outlet assist
        switch (arg) {
            case 0:
                sprintf(s, "(matrix) pointcloud");
                break;
            case 1:
                sprintf(s, "(matrix) rgb");
                break;
            case 2:
                sprintf(s, "dumpout");
                break;
        }
    }
}


