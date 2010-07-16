#include <math.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <map>

#include <assert.h>

#include <jsapi.h>
#include <jsfun.h>

using namespace std;

////////////////////////////////////////
/////   SysUtil class
////////////////////////////////////////

typedef struct _SysUtil {
    // my data;
    JSContext *cx;
    JSObject* SysUtil;

    gpointer SysUtilPtr;
} SysUtilData;

static SysUtilData* SysUtilData;


/*************************************************************************************************/

/** SysUtil Constructor */
static JSBool SysUtilConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc == 0) {
        SysUtilData* dta = NULL;

        dta = new SysUtilData;

        if (!JS_SetPrivate(cx, obj, dta))
            return JS_FALSE;
        return JS_TRUE;
    }
    return JS_FALSE;
}

static void SysUtilDestructor(JSContext *cx, JSObject *obj) {
    printf("Destroying SysUtil object\n");
    SysUtilData* dta = (SysUtilData *) JS_GetPrivate(cx, obj);
    if (dta) {
        delete dta;
    }
}

/*************************************************************************************************/

///// SysUtil Function Table
static JSFunctionSpec _SysUtilFunctionSpec[] = {
    { 0, 0, 0, 0, 0}
};

/*************************************************************************************************/
///// SysUtil Class Definition

static JSClass SysUtil_jsClass = {
    "SysUtil", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, // add/del prop
    JS_PropertyStub, JS_PropertyStub, // get/set prop
    JS_EnumerateStub, JS_ResolveStub, // enum / resolve
    JS_ConvertStub, SysUtilDestructor, // convert / finalize
    0, 0, 0, 0,
    0, 0, 0, 0
};
/*************************************************************************************************/

/* static funcs: */
static JSBool SysUtil_s_mainloop(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    check_args((argc == 0), "must not pass an argument!\n");
    /*
    JSObject *bus = JS_ConstructObject(cx, &SysUtil_jsClass, NULL, NULL);
    SysUtilData* dta = (SysUtilData *) JS_GetPrivate(cx, bus);
     */
    g_main_run(mainloop);

    return JS_TRUE;
}

static JSBool SysUtil_s_system(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    check_args((argc == 0), "must not pass an argument!\n");

    g_main_loop_quit(mainloop);

    return JS_TRUE;
}

static JSFunctionSpec _SysUtilStaticFunctionSpec[] = {
    { "system", SysUtil_s_system, 0, 0, 0},
    { 0, 0, 0, 0, 0}
};


///// Actor Initialization Method

JSObject* SysUtilInit(JSContext *cx, JSObject *obj) {
    if (obj == NULL)
        obj = JS_GetGlobalObject(cx);

    JSObject* o = JS_InitClass(cx, obj, NULL,
            &SysUtil_jsClass,
            SysUtilConstructor, 0,
            NULL, // properties
            _SysUtilFunctionSpec, // functions
            NULL, // static properties
            _SysUtilStaticFunctionSpec // static functions
            );

    SysUtilData = new SysUtilData;
    SysUtilData->cx = cx;
    return o;
}

/*************************************************************************************************/
