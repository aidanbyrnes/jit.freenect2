#include "jit.common.h"
#include "kinect_wrapper.h"
#include "max.jit.mop.h"

#define DEPTH_WIDTH  512
#define DEPTH_HEIGHT 424

// Max object instance data
typedef struct _max_jit_freenect2 {
    t_object ob;
    void *obex;
    void *qelem; // For thread-safe callback
} t_max_jit_freenect2;


// prototypes
BEGIN_USING_C_LINKAGE
t_jit_err jit_freenect2_init(void);
void * max_jit_freenect2_new(t_symbol *s, long argc, t_atom *argv);
void max_jit_freenect2_free(t_max_jit_freenect2 *x);
void max_jit_freenect2_outputmatrix(t_max_jit_freenect2 *x);
void max_jit_freenect2_bang(t_max_jit_freenect2 *x);
void max_jit_freenect2_qfn(t_max_jit_freenect2 *x);
void max_jit_freenect2_setup_callback(t_max_jit_freenect2 *x);
void max_jit_freenect2_stop(t_max_jit_freenect2 *x);
void max_jit_freenect2_trigger_output(void *x); // Static callback trigger
void max_jit_freenect2_assist(t_max_jit_freenect2 *x, void *b, long msg, long arg, char *s);
END_USING_C_LINKAGE

// globals
static void *max_jit_freenect2_class = NULL;


/************************************************************************************/

void ext_main(void *r) {
    t_class *max_class, *jit_class;

    jit_freenect2_init();

    max_class = class_new("jit.freenect2", (method)max_jit_freenect2_new, (method)max_jit_freenect2_free, sizeof(t_max_jit_freenect2), NULL, A_GIMME, 0);
    max_jit_class_obex_setup(max_class, calcoffset(t_max_jit_freenect2, obex));

    jit_class = (t_class *)jit_class_findbyname(gensym("jit_freenect2"));
    max_jit_class_mop_wrap(max_class, jit_class,  MAX_JIT_MOP_FLAGS_OWN_ADAPT);
    max_jit_class_wrap_standard(max_class, jit_class, 0);

    class_addmethod(max_class, (method)max_jit_freenect2_outputmatrix, "outputmatrix", A_USURP_LOW, 0);
    class_addmethod(max_class, (method)max_jit_freenect2_bang, "bang", 0);
    class_addmethod(max_class, (method)max_jit_freenect2_setup_callback, "start", 0);
    class_addmethod(max_class, (method)max_jit_freenect2_stop, "stop", 0);
    class_addmethod(max_class, (method)max_jit_freenect2_assist, "assist", A_CANT, 0);

    class_register(CLASS_BOX, max_class);
    max_jit_freenect2_class = max_class;
}

/************************************************************************************/

void * max_jit_freenect2_new(t_symbol *s, long argc, t_atom *argv) {
    t_max_jit_freenect2 *x;
    void *o;

    x = (t_max_jit_freenect2 *)max_jit_object_alloc((t_class *)max_jit_freenect2_class, gensym("jit_freenect2"));

    if (x) {
        o = jit_object_new(gensym("jit_freenect2"));

        if (o) {
            max_jit_mop_setup_simple(x, o, argc, argv);
            max_jit_attr_args(x, argc, argv);
            t_atom_long depthdim[2] = { DEPTH_WIDTH, DEPTH_HEIGHT };
            t_atom_long rgbdim[2] = { DEPTH_WIDTH, DEPTH_HEIGHT }; //AB: set rgb dim to depth dim since rgb will be registered to depth

            //TA: set depth matrix initial attributes
            void *output = max_jit_mop_getoutput(x, 1);
            jit_attr_setsym(output, _jit_sym_type, _jit_sym_float32);
            jit_attr_setlong_array(output, _jit_sym_dim, 2, depthdim);
            jit_attr_setlong(output, _jit_sym_planecount, 3);

            //TA: set rgb matrix initial attributes
            output = max_jit_mop_getoutput(x, 2);
            jit_attr_setsym(output, _jit_sym_type, _jit_sym_char);
            jit_attr_setlong_array(output, _jit_sym_dim, 2, rgbdim);
            jit_attr_setlong(output, _jit_sym_planecount, 4);

            // Create queue element for thread-safe output
            x->qelem = qelem_new(x, (method)max_jit_freenect2_qfn);
        } else {
            jit_object_error((t_object *)x, "jit.freenect2: could not allocate object");
            object_free((t_object *)x);
            x = NULL;
        }
    }

    return (x);
}

void max_jit_freenect2_free(t_max_jit_freenect2 *x) {
    if (x->qelem) {
        qelem_free(x->qelem);           // Free the qelem
    }

    max_jit_mop_free(x);
    jit_object_free(max_jit_obex_jitob_get(x));
    max_jit_object_free(x);
}

/************************************************************************************/

void max_jit_freenect2_outputmatrix(t_max_jit_freenect2 *x) {
    void *mop = max_jit_obex_adornment_get(x, _jit_sym_jit_mop);
    t_jit_err err;

    if (mop) {
        if ((err = (t_jit_err)jit_object_method(max_jit_obex_jitob_get(x),
                                                _jit_sym_matrix_calc,
                                                jit_object_method(mop, _jit_sym_getinputlist),
                                                jit_object_method(mop, _jit_sym_getoutputlist)))) {
            jit_error_code(x, err);
        } else {
            max_jit_mop_outputmatrix(x);
        }
    }
}

void max_jit_freenect2_bang(t_max_jit_freenect2 *x) {
    max_jit_freenect2_outputmatrix(x);
}

void max_jit_freenect2_qfn(t_max_jit_freenect2 *x) {
    max_jit_freenect2_outputmatrix(x);
}

void max_jit_freenect2_trigger_output(void *x) {
    t_max_jit_freenect2 *max_obj = (t_max_jit_freenect2 *)x;

    if (max_obj && max_obj->qelem) {
        qelem_set(max_obj->qelem); // Schedule output
    }
}

// Set up frame callback
void max_jit_freenect2_setup_callback(t_max_jit_freenect2 *x) {
    void *jit_ob = max_jit_obex_jitob_get(x);

    if (jit_ob) {
        // Get the kinect wrapper from the jitter object
        kinect_wrapper *kinect = (kinect_wrapper *)jit_object_method(jit_ob, gensym("get_kinect_wrapper"));

        if (kinect) {
            kinect->setFrameCallback(max_jit_freenect2_trigger_output, x);
        }
    }
}

// Stop frame callback
void max_jit_freenect2_stop(t_max_jit_freenect2 *x) {
    void *jit_ob = max_jit_obex_jitob_get(x);

    if (jit_ob) {
        // Get the kinect wrapper from the jitter object
        kinect_wrapper *kinect = (kinect_wrapper *)jit_object_method(jit_ob, gensym("get_kinect_wrapper"));

        if (kinect) {
            // Clear the callback by passing nullptr
            kinect->setFrameCallback(nullptr, nullptr);
        }
    }
}

void max_jit_freenect2_assist(t_max_jit_freenect2 *x, void *b, long msg, long arg, char *s) {
    if (msg == ASSIST_OUTLET) {
        switch (arg) {
            case 0:
                sprintf(s, "(matrix) pointcloud");
                break;

            case 1:
                sprintf(s, "(matrix) color");
                break;

            case 2:
                sprintf(s, "dumpout");
                break;
        }
    }
}
