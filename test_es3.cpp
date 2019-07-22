#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>


#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

int width = 320;
int height = 240;

// X11 related local variables
static Display *x_display = NULL;
static Atom s_wmDeleteMessage;


///
// GetContextRenderableType()
//
//    Check whether EGL_KHR_create_context extension is supported.  If so,
//    return EGL_OPENGL_ES3_BIT_KHR instead of EGL_OPENGL_ES2_BIT
//
EGLint GetContextRenderableType ( EGLDisplay eglDisplay )
{
#ifdef EGL_KHR_create_context
   const char *extensions = eglQueryString ( eglDisplay, EGL_EXTENSIONS );

   // check whether EGL_KHR_create_context is in the extension string
   if ( extensions != NULL && strstr( extensions, "EGL_KHR_create_context" ) )
   {
      // extension is supported
      return EGL_OPENGL_ES3_BIT_KHR;
   }
#endif
   // extension is not supported
   return EGL_OPENGL_ES2_BIT;
}

///
//  WinCreate()
//
//      This function initialized the native X11 display and window for EGL
//
EGLBoolean WinCreate(EGLNativeWindowType& nativeWindow)
{
    Window root;
    XSetWindowAttributes swa;
    XSetWindowAttributes  xattr;
    Atom wm_state;
    XWMHints hints;
    XEvent xev;
    EGLConfig ecfg;
    EGLint num_config;
    Window win;

    /*
     * X11 native display initialization
     */

    x_display = XOpenDisplay(NULL);
    if ( x_display == NULL )
    {
        return EGL_FALSE;
    }

    root = DefaultRootWindow(x_display);

    swa.event_mask  =  ExposureMask | PointerMotionMask | KeyPressMask;
    win = XCreateWindow(
               x_display, root,
               0, 0, width, height, 0,
               CopyFromParent, InputOutput,
               CopyFromParent, CWEventMask,
               &swa );
    s_wmDeleteMessage = XInternAtom(x_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(x_display, win, &s_wmDeleteMessage, 1);

    xattr.override_redirect = FALSE;
    XChangeWindowAttributes ( x_display, win, CWOverrideRedirect, &xattr );

    hints.input = TRUE;
    hints.flags = InputHint;
    XSetWMHints(x_display, win, &hints);

    // make the window visible on the screen
    XMapWindow (x_display, win);
    XStoreName (x_display, win, "title");

    // get identifiers for the provided atom name strings
    wm_state = XInternAtom (x_display, "_NET_WM_STATE", FALSE);

    memset ( &xev, 0, sizeof(xev) );
    xev.type                 = ClientMessage;
    xev.xclient.window       = win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format       = 32;
    xev.xclient.data.l[0]    = 1;
    xev.xclient.data.l[1]    = FALSE;
    XSendEvent (
       x_display,
       DefaultRootWindow ( x_display ),
       FALSE,
       SubstructureNotifyMask,
       &xev );

    nativeWindow = (EGLNativeWindowType) win;
    //eglNativeDisplay = (EGLNativeDisplayType) x_display;
    return EGL_TRUE;
}


EGLBoolean initializeWindow(EGLNativeWindowType nativeWindow,
                            EGLDisplay& display,
                            EGLSurface& eglSurface)
{
    const EGLint contextAttribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    display = eglGetDisplay ( EGL_DEFAULT_DISPLAY );
    if (display == EGL_NO_DISPLAY)
    {
        return EGL_FALSE;
    }
    printf("eglGetDisplay succ\n");
    const EGLint configAttribs[] =
      {
         EGL_RED_SIZE,       5,
         EGL_GREEN_SIZE,     6,
         EGL_BLUE_SIZE,      5,
         EGL_ALPHA_SIZE,     EGL_DONT_CARE,
         EGL_DEPTH_SIZE,     EGL_DONT_CARE,
         EGL_STENCIL_SIZE,   EGL_DONT_CARE,
         EGL_SAMPLE_BUFFERS, 0,
         // if EGL_KHR_create_context extension is supported, then we will use
         // EGL_OPENGL_ES3_BIT_KHR instead of EGL_OPENGL_ES2_BIT in the attribute list
         EGL_RENDERABLE_TYPE, GetContextRenderableType ( display ),
         EGL_NONE
      };


    EGLint major, minor;
    if (!eglInitialize ( display, &major, &minor ))
    {
        return EGL_FALSE;
    }
    printf("eglInitialize succ\n");
    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig ( display, configAttribs, &config, 1,
        &numConfigs))
    {
        return EGL_FALSE;
    }
    printf("eglChooseConfig succ\n");
    eglSurface = eglCreateWindowSurface(display, config,
                            nativeWindow, NULL);
    if (eglSurface == EGL_NO_SURFACE)
    {
        return EGL_FALSE;
    }
    printf("eglCreateWindowSurface succ\n");
    EGLContext context = eglCreateContext(display, config,
        EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT)
    {
        return EGL_FALSE;
    }
    printf("eglCreateContext succ\n");
    if (!eglMakeCurrent(display, eglSurface, eglSurface, context))
    {
        return EGL_FALSE;
    }
    printf("eglMakeCurrent succ\n");
    return EGL_TRUE;
}

//
// Create a shader object, load the shader source, and
// compile the shader.
//
GLuint LoadShader ( GLenum type, const char *shaderSrc )
{
   GLuint shader;
   GLint compiled;

   // Create the shader object
   shader = glCreateShader ( type );

   if ( shader == 0 )
   {
      return 0;
   }

   // Load the shader source
   glShaderSource ( shader, 1, &shaderSrc, NULL );

   // Compile the shader
   glCompileShader ( shader );

   // Check the compile status
   glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

   if ( !compiled )
   {
      GLint infoLen = 0;

      glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char *infoLog = (char*) (malloc ( sizeof ( char ) * infoLen ));

         glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
            printf ("Error compiling shader:\n%s\n", infoLog );

         free ( infoLog );
      }

      glDeleteShader ( shader );
      return 0;
   }

   return shader;

}


int Init (GLuint& programObject)
{
   char vShaderStr[] =
      "#version 300 es                          \n"
      "layout(location = 0) in vec4 vPosition;  \n"
      "void main()                              \n"
      "{                                        \n"
      "   gl_Position = vPosition;              \n"
      "}                                        \n";

   char fShaderStr[] =
      "#version 300 es                              \n"
      "precision mediump float;                     \n"
      "out vec4 fragColor;                          \n"
      "void main()                                  \n"
      "{                                            \n"
      "   fragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );  \n"
      "}                                            \n";

   GLuint vertexShader;
   GLuint fragmentShader;
   GLint linked;

   // Load the vertex/fragment shaders
   vertexShader = LoadShader ( GL_VERTEX_SHADER, vShaderStr );
   fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fShaderStr );

   // Create the program object
   programObject = glCreateProgram ( );

   if ( programObject == 0 )
   {
      return 0;
   }

   glAttachShader ( programObject, vertexShader );
   glAttachShader ( programObject, fragmentShader );

   // Link the program
   glLinkProgram ( programObject );

   // Check the link status
   glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );

   if ( !linked )
   {
      GLint infoLen = 0;

      glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char *infoLog = (char*)(malloc ( sizeof ( char ) * infoLen ));

         glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
         printf ( "Error linking program:\n%s\n", infoLog );

         free ( infoLog );
      }

      glDeleteProgram ( programObject );
      return FALSE;
   }

   glClearColor ( 1.0f, 1.0f, 1.0f, 0.0f );
   return TRUE;
}

void Draw (GLuint programObject)
{
   GLfloat vVertices[] = {  0.0f,  0.5f, 0.0f,
                            -0.5f, -0.5f, 0.0f,
                            0.5f, -0.5f, 0.0f
                         };

   // Set the viewport
   glViewport ( 0, 0, width, height );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   // Use the program object
   glUseProgram ( programObject );

   // Load the vertex data
   glVertexAttribPointer ( 0, 3, GL_FLOAT, GL_FALSE, 0, vVertices );
   glEnableVertexAttribArray ( 0 );

   glDrawArrays ( GL_TRIANGLES, 0, 3 );
}

void Shutdown(GLuint programObject)
{
   glDeleteProgram(programObject);
}

int main() {
    EGLNativeWindowType nativeWindow;
    bool rtn = WinCreate(nativeWindow);
    printf("WinCreate rtn %d\n", rtn);
    EGLDisplay eglDisplay;
    EGLSurface eglSurface;
    rtn = initializeWindow(nativeWindow, eglDisplay, eglSurface);
    printf("initializeWindow rtn %d\n", rtn);
    GLuint programObject;
    rtn = Init(programObject);
    printf("Init rtn %d\n", rtn);
    //Draw(programObject);
    //printf("Draw rtn %d\n", rtn);

    struct timeval t1, t2;
    struct timezone tz;
    float deltatime;

    gettimeofday ( &t1 , &tz );

    while(true)
    {
        gettimeofday(&t2, &tz);
        deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
        t1 = t2;

        Draw(programObject);
        eglSwapBuffers(eglDisplay, eglSurface);        
    }

    Shutdown(programObject);

    return 0;
}