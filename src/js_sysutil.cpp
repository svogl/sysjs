#include <math.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
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

#define fail_if_not(cx, assert, ...) if (!(assert)) { \
		fprintf(stderr, "%s:%d :: ", __FUNCTION__, __LINE__ );\
		fprintf(stderr, __VA_ARGS__);\
		JS_ReportError(cx, __VA_ARGS__); \
		return JS_FALSE; \
	}

#define fail_if(assert, ...) fail_if_not (!(assert), __VA_ARGS__)

#define ADD_INT_CONST(cx,obj,prop, val) do { \
		jsval v = INT_TO_JSVAL( val); \
		JSBool ret = JS_SetProperty(cx, obj, prop, &v ); \
		if (!ret) printf("failed for %s\n", #prop ); \
	} while (0)

#define INT_PROP(p, v) \
		ADD_INT_CONST(cx,obj, p, v)

/*************************************************************************************************/
JSBool _SysUtilDefineProps(JSContext *cx, JSObject *obj);

/** SysUtil Constructor */
static JSBool SysUtilConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc == 0) {
        SysUtilData* dta = NULL;

        dta = new SysUtilData;

        if (!JS_SetPrivate(cx, obj, dta))
            return JS_FALSE;
        return JS_TRUE;
    }

    return JS_TRUE;
}

static void SysUtilDestructor(JSContext *cx, JSObject *obj) {
    //fprintf(stderr, "Destroying SysUtil object\n");
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

/** this function maps the getenv()-call. if an environment-variable is found, a 
 * string is returned, null otherwise 
 */
static JSBool SysUtil_s_getenv(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    fail_if_not(cx, (argc == 1), "which env do you wnt to read?\n");
    fail_if_not(cx, JSVAL_IS_STRING(argv[0]), "arg must be a string (env.var)!");
    JSString* varStr = JSVAL_TO_STRING(argv[0]);
    char* var = JS_GetStringBytes(varStr);

    char *val = getenv(var);

    if (val!=NULL) {
	    JSString* valStr=NULL;
	    valStr = JS_NewStringCopyZ(cx, val);
	    *rval = STRING_TO_JSVAL(valStr);
    } else {
	    // no match
	    *rval = JSVAL_NULL;
    }
    return JS_TRUE;
}

static JSBool SysUtil_s_getpid(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    fail_if_not(cx, (argc == 0), "no arguments, please.\n");
	pid_t p = getpid();

    *rval = INT_TO_JSVAL(p);

    return JS_TRUE;
}

static JSBool SysUtil_s_system(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    fail_if_not(cx, (argc == 1), "pass the cmd you want to run!\n");
    fail_if_not(cx, JSVAL_IS_STRING(argv[0]), "arg must be a string (url)!");
    JSString* cmdStr = JSVAL_TO_STRING(argv[0]);
    char* cmd = JS_GetStringBytes(cmdStr);

	int ret = system(cmd);

	*rval = INT_TO_JSVAL(ret);

    return JS_TRUE;
}

static JSBool SysUtil_s_open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    fail_if_not(cx, (argc == 2), "pass the (file,flags)");
    fail_if_not(cx, JSVAL_IS_STRING(argv[0]), "arg0 must be a string (path)!");
    fail_if_not(cx, JSVAL_IS_INT(argv[1]), "arg1 must be a int (flags)!");

    JSString* cmdStr = JSVAL_TO_STRING(argv[0]);
    char* file = JS_GetStringBytes(cmdStr);
	jsint flags = JSVAL_TO_INT(argv[1]);

	int ret = open(file, flags, 0644);
	
	if (ret == -1) {
		JS_ReportError(cx, "open failed: %s", strerror(errno));
		return JS_FALSE;
	}

	*rval = INT_TO_JSVAL(ret);
    return JS_TRUE;
}

static JSBool SysUtil_s_close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    fail_if_not(cx, (argc == 1), "pass the fd you want to close!\n");
    fail_if_not(cx, JSVAL_IS_STRING(argv[0]), "arg must be a int!");

	jsint fd = JSVAL_TO_INT(argv[0]);

	int ret = close(fd);

	*rval = INT_TO_JSVAL(ret);

    return JS_TRUE;
}


static JSBool SysUtil_s_unlink(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    fail_if_not(cx, (argc == 1), "pass the file to delete!\n");
    fail_if_not(cx, JSVAL_IS_STRING(argv[0]), "arg must be a string!");
    JSString* cmdStr = JSVAL_TO_STRING(argv[0]);
    char* file = JS_GetStringBytes(cmdStr);

	int ret = unlink(file);

	if (ret == -1) {
		JS_ReportError(cx, "unlink failed: %s", strerror(errno));
		return JS_FALSE;
	}

	*rval = INT_TO_JSVAL(ret);
    return JS_TRUE;
}

static JSBool SysUtil_s_rename(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    fail_if_not(cx, (argc == 2), "pass the cmd you want to run!\n");
    fail_if_not(cx, JSVAL_IS_STRING(argv[0]) && JSVAL_IS_STRING(argv[1]), "args must be string!");
    JSString* fromStr = JSVAL_TO_STRING(argv[0]);
    JSString* toStr = JSVAL_TO_STRING(argv[1]);
    char* from = JS_GetStringBytes(fromStr);
    char* to = JS_GetStringBytes(toStr);

	int ret = rename(from, to);

	if (ret == -1) {
		JS_ReportError(cx, "rename failed: %s", strerror(errno));
		return JS_FALSE;
	}

	*rval = INT_TO_JSVAL(ret);
    return JS_TRUE;
}


static JSBool SysUtil_s_sleep(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    fail_if_not(cx, (argc == 1), "pass the milliseconds you want to sleep!\n");
    fail_if_not(cx, JSVAL_IS_INT(argv[0]), "arg must be a int!");

	jsint ms = JSVAL_TO_INT(argv[0]);

	int ret = usleep(ms*1000);

	*rval = INT_TO_JSVAL(ret);

    return JS_TRUE;
}

static JSBool SysUtil_s_a2f(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    fail_if_not(cx, (argc == 4), "pass the milliseconds you want to sleep!\n");
    fail_if_not(cx, JSVAL_IS_INT(argv[0]), "arg must be a int!");
    fail_if_not(cx, JSVAL_IS_INT(argv[1]), "arg must be a int!");
    fail_if_not(cx, JSVAL_IS_INT(argv[2]), "arg must be a int!");
    fail_if_not(cx, JSVAL_IS_INT(argv[3]), "arg must be a int!");

	jsint a0 = JSVAL_TO_INT(argv[0]);
	jsint a1 = JSVAL_TO_INT(argv[1]);
	jsint a2 = JSVAL_TO_INT(argv[2]);
	jsint a3 = JSVAL_TO_INT(argv[3]);

	float f;
	unsigned char*c = (unsigned char*)&f;
	c[3] = (unsigned char)(a0);
	c[2] = (unsigned char)(a1);
	c[1] = (unsigned char)(a2);
	c[0] = (unsigned char)(a3);

//printf("A2F: COULD THIS BE %g %g %g?\n",f, (double)f, (jsdouble)f);
	jsdouble* d = JS_NewDouble(cx, f);
printf("A2F: COULD THIS BE %g?\n",*d);
	*rval = DOUBLE_TO_JSVAL(d);

    return JS_TRUE;
}


JSBool _SysUtilDefineProps(JSContext *cx, JSObject *obj)
{
//	JsClass class* = JS_GetClass(cx, obj);
	INT_PROP("E2BIG", E2BIG);
	INT_PROP("EACCES", EACCES);
	INT_PROP("EADDRINUSE", EADDRINUSE);
	INT_PROP("EADDRNOTAVAIL", EADDRNOTAVAIL);
	INT_PROP("EAFNOSUPPORT", EAFNOSUPPORT);
	INT_PROP("EAGAIN", EAGAIN);
	INT_PROP("EALREADY", EALREADY);
	INT_PROP("EBADF", EBADF);
	INT_PROP("EBADMSG", EBADMSG);
	INT_PROP("EBUSY", EBUSY);
	INT_PROP("ECANCELED", ECANCELED);
	INT_PROP("ECHILD", ECHILD);
	INT_PROP("ECONNABORTED", ECONNABORTED);
	INT_PROP("ECONNREFUSED", ECONNREFUSED);
	INT_PROP("ECONNRESET", ECONNRESET);
	INT_PROP("EDEADLK", EDEADLK);
	INT_PROP("EDESTADDRREQ", EDESTADDRREQ);
	INT_PROP("EDOM", EDOM);
	INT_PROP("EDQUOT", EDQUOT);
	INT_PROP("EEXIST", EEXIST);
	INT_PROP("EFAULT", EFAULT);
	INT_PROP("EFBIG", EFBIG);
	INT_PROP("EHOSTUNREACH", EHOSTUNREACH);
	INT_PROP("EIDRM", EIDRM);
	INT_PROP("EILSEQ", EILSEQ);
	INT_PROP("EINPROGRESS", EINPROGRESS);
	INT_PROP("EINTR", EINTR);
	INT_PROP("EINVAL", EINVAL);
	INT_PROP("EIO", EIO);
	INT_PROP("EISCONN", EISCONN);
	INT_PROP("EISDIR", EISDIR);
	INT_PROP("ELOOP", ELOOP);
	INT_PROP("EMFILE", EMFILE);
	INT_PROP("EMLINK", EMLINK);
	INT_PROP("EMSGSIZE", EMSGSIZE);
	INT_PROP("EMULTIHOP", EMULTIHOP);
	INT_PROP("ENAMETOOLONG", ENAMETOOLONG);
	INT_PROP("ENETDOWN", ENETDOWN);
	INT_PROP("ENETRESET", ENETRESET);
	INT_PROP("ENETUNREACH", ENETUNREACH);
	INT_PROP("ENFILE", ENFILE);
	INT_PROP("ENOBUFS", ENOBUFS);
	INT_PROP("ENODATA", ENODATA);
	INT_PROP("ENODEV", ENODEV);
	INT_PROP("ENOENT", ENOENT);
	INT_PROP("ENOEXEC", ENOEXEC);
	INT_PROP("ENOLCK", ENOLCK);
	INT_PROP("ENOLINK", ENOLINK);
	INT_PROP("ENOMEM", ENOMEM);
	INT_PROP("ENOMSG", ENOMSG);
	INT_PROP("ENOPROTOOPT", ENOPROTOOPT);
	INT_PROP("ENOSPC", ENOSPC);
	INT_PROP("ENOSR", ENOSR);
	INT_PROP("ENOSTR", ENOSTR);
	INT_PROP("ENOSYS", ENOSYS);
	INT_PROP("ENOTCONN", ENOTCONN);
	INT_PROP("ENOTDIR", ENOTDIR);
	INT_PROP("ENOTEMPTY", ENOTEMPTY);
	INT_PROP("ENOTSOCK", ENOTSOCK);
	INT_PROP("ENOTSUP", ENOTSUP);
	INT_PROP("ENOTTY", ENOTTY);
	INT_PROP("ENXIO", ENXIO);
	INT_PROP("EOPNOTSUPP", EOPNOTSUPP);
	INT_PROP("EOVERFLOW", EOVERFLOW);
	INT_PROP("EPERM", EPERM);
	INT_PROP("EPIPE", EPIPE);
	INT_PROP("EPROTO", EPROTO);
	INT_PROP("EPROTONOSUPPORT", EPROTONOSUPPORT);
	INT_PROP("EPROTOTYPE", EPROTOTYPE);
	INT_PROP("ERANGE", ERANGE);
	INT_PROP("EROFS", EROFS);
	INT_PROP("ESPIPE", ESPIPE);
	INT_PROP("ESRCH", ESRCH);
	INT_PROP("ESTALE", ESTALE);
	INT_PROP("ETIMEDOUT", ETIMEDOUT);
	INT_PROP("ETIME", ETIME);
	INT_PROP("ETXTBSY", ETXTBSY);
	INT_PROP("EWOULDBLOCK", EWOULDBLOCK);
	INT_PROP("EXDEV", EXDEV);
	INT_PROP("O_APPEND", O_APPEND);
	INT_PROP("O_CREAT", O_CREAT);
	INT_PROP("O_DIRECTORY", O_DIRECTORY);
	INT_PROP("O_EXCL", O_EXCL);
	INT_PROP("O_EXCL", O_EXCL);
	INT_PROP("O_NOCTTY", O_NOCTTY);
	INT_PROP("O_NOFOLLOW", O_NOFOLLOW);
	INT_PROP("O_RDONLY", O_RDONLY);
	INT_PROP("O_RDWR", O_RDWR);
	INT_PROP("O_SYNC", O_SYNC);
	INT_PROP("O_TRUNC", O_TRUNC);
	INT_PROP("O_WRONLY", O_WRONLY);
	INT_PROP("S_IFBLK", S_IFBLK);
	INT_PROP("S_IFCHR", S_IFCHR);
	INT_PROP("S_IFDIR", S_IFDIR);
	INT_PROP("S_IFIFO", S_IFIFO);
	INT_PROP("S_IFLNK", S_IFLNK);
	INT_PROP("S_IFMT", S_IFMT);
	INT_PROP("S_IFREG", S_IFREG);
	INT_PROP("S_IFSOCK", S_IFSOCK);
	INT_PROP("SIGABRT", SIGABRT);
	INT_PROP("SIGALRM", SIGALRM);
	INT_PROP("SIGBUS", SIGBUS);
	INT_PROP("SIGCHLD", SIGCHLD);
	INT_PROP("SIGCONT", SIGCONT);
	INT_PROP("SIGFPE", SIGFPE);
	INT_PROP("SIGHUP", SIGHUP);
	INT_PROP("SIGILL", SIGILL);
	INT_PROP("SIGINT", SIGINT);
	INT_PROP("SIGIO", SIGIO);
	INT_PROP("SIGIOT", SIGIOT);
	INT_PROP("SIGKILL", SIGKILL);
	INT_PROP("SIGPIPE", SIGPIPE);
	INT_PROP("SIGPOLL", SIGPOLL);
	INT_PROP("SIGPROF", SIGPROF);
	INT_PROP("SIGPWR", SIGPWR);
	INT_PROP("SIGQUIT", SIGQUIT);
	INT_PROP("SIGSEGV", SIGSEGV);
	INT_PROP("SIGSTKFLT", SIGSTKFLT);
	INT_PROP("SIGSTOP", SIGSTOP);
	INT_PROP("SIGSYS", SIGSYS);
	INT_PROP("SIGTERM", SIGTERM);
	INT_PROP("SIGTRAP", SIGTRAP);
	INT_PROP("SIGTSTP", SIGTSTP);
	INT_PROP("SIGTTIN", SIGTTIN);
	INT_PROP("SIGTTOU", SIGTTOU);
	INT_PROP("SIGUNUSED", SIGUNUSED);
	INT_PROP("SIGURG", SIGURG);
	INT_PROP("SIGUSR1", SIGUSR1);
	INT_PROP("SIGUSR2", SIGUSR2);
	INT_PROP("SIGVTALRM", SIGVTALRM);
	INT_PROP("SIGWINCH", SIGWINCH);
	INT_PROP("SIGXCPU", SIGXCPU);
	INT_PROP("SIGXFSZ", SIGXFSZ);
	INT_PROP("S_IRGRP", S_IRGRP);
	INT_PROP("S_IROTH", S_IROTH);
	INT_PROP("S_IRUSR", S_IRUSR);
	INT_PROP("S_IRWXG", S_IRWXG);
	INT_PROP("S_IRWXO", S_IRWXO);
	INT_PROP("S_IRWXU", S_IRWXU);
	INT_PROP("S_IWGRP", S_IWGRP);
	INT_PROP("S_IWOTH", S_IWOTH);
	INT_PROP("S_IWUSR", S_IWUSR);
	INT_PROP("S_IXGRP", S_IXGRP);
	INT_PROP("S_IXOTH", S_IXOTH);
	INT_PROP("S_IXUSR", S_IXUSR);
}


static JSFunctionSpec _SysUtilStaticFunctionSpec[] = {
    { "system", SysUtil_s_system, 0, 0, 0},
    { "getenv", SysUtil_s_getenv, 0, 0, 0},
    { "getpid", SysUtil_s_getpid, 0, 0, 0},
    { "open", SysUtil_s_open, 0, 0, 0},
    { "close", SysUtil_s_close, 0, 0, 0},
    { "unlink", SysUtil_s_unlink, 0, 0, 0},
    { "rename", SysUtil_s_rename, 0, 0, 0},
    { "sleep", SysUtil_s_sleep, 0, 0, 0},
    { "array2float", SysUtil_s_a2f, 0, 0, 0},
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
	if (o) {
	    SysUtilDataPtr = new SysUtilData;
	    SysUtilDataPtr->cx = cx;

		_SysUtilDefineProps(cx, o);
	}

    return o;
}

JS_END_EXTERN_C

/*************************************************************************************************/
