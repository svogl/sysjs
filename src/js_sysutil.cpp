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

} SysUtilData;

static SysUtilData* SysUtilDataPtr;

#define fail_if_not(assert, ...) if (!(assert)) { \
		fprintf(stderr, "%s:%d :: ", __FUNCTION__, __LINE__ );\
		fprintf(stderr, __VA_ARGS__);\
		return JS_FALSE; \
	}

#define fail_if(assert, ...) fail_if_not (!(assert), __VA_ARGS__)

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


static JSBool SysUtil_s_system(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    fail_if_not((argc == 1), "pass the cmd you want to run!\n");
    fail_if_not(JSVAL_IS_STRING(argv[0]), "arg must be a string (url)!");
    JSString* cmdStr = JSVAL_TO_STRING(argv[0]);
    char* cmd = JS_GetStringBytes(cmdStr);

	int ret = system(cmd);

	*rval = INT_TO_JSVAL(ret);

    return JS_TRUE;
}

static JSFunctionSpec _SysUtilStaticFunctionSpec[] = {
    { "system", SysUtil_s_system, 0, 0, 0},
    { 0, 0, 0, 0, 0}
};


///// Actor Initialization Method

JS_BEGIN_EXTERN_C

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

    SysUtilDataPtr = new SysUtilData;
    SysUtilDataPtr->cx = cx;
    return o;
}

JS_END_EXTERN_C

/*************************************************************************************************/
