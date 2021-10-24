#include "assert.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdlib.h>
#include <string.h>
#include "id_us.h"
#include "id_vl.h"
#include "id_vl_private.h"
#include "ck_cross.h"

#define VL_SDL2GL_NUM_BUFFERS 1

// OpenGL 1.0 and 1.1 Function Pointers:
typedef const GLubyte *(APIENTRYP PFN_ID_GLGETSTRING)(GLenum name);
PFN_ID_GLGETSTRING id_glGetString = 0;
typedef void(APIENTRYP PFN_ID_GLGENTEXTURES)(GLsizei n, GLuint *textures);
PFN_ID_GLGENTEXTURES id_glGenTextures = 0;
typedef void(APIENTRYP PFN_ID_GLBINDTEXTURE)(GLenum target, GLuint texture);
PFN_ID_GLBINDTEXTURE id_glBindTexture = 0;
typedef void(APIENTRYP PFN_ID_GLTEXPARAMETERF)(GLenum target, GLenum pname, GLfloat param);
PFN_ID_GLTEXPARAMETERF id_glTexParameterf = 0;
typedef void(APIENTRYP PFN_ID_GLTEXPARAMETERI)(GLenum target, GLenum pname, GLint param);
PFN_ID_GLTEXPARAMETERI id_glTexParameteri = 0;
typedef void(APIENTRYP PFN_ID_GLTEXIMAGE1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
PFN_ID_GLTEXIMAGE1D id_glTexImage1D = 0;
typedef void(APIENTRYP PFN_ID_GLTEXIMAGE2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
PFN_ID_GLTEXIMAGE2D id_glTexImage2D = 0;
typedef void(APIENTRYP PFN_ID_GLTEXSUBIMAGE1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
PFN_ID_GLTEXSUBIMAGE1D id_glTexSubImage1D = 0;
typedef void(APIENTRYP PFN_ID_GLTEXSUBIMAGE2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
PFN_ID_GLTEXSUBIMAGE2D id_glTexSubImage2D = 0;
typedef void(APIENTRYP PFN_ID_GLDELETETEXTURES)(GLsizei n, const GLuint *textures);
PFN_ID_GLDELETETEXTURES id_glDeleteTextures = 0;
typedef void(APIENTRYP PFN_ID_GLENABLE)(GLenum cap);
PFN_ID_GLENABLE id_glEnable = 0;
typedef void(APIENTRYP PFN_ID_GLDISABLE)(GLenum cap);
PFN_ID_GLDISABLE id_glDisable = 0;
typedef void(APIENTRYP PFN_ID_GLCLEAR)(GLbitfield mask);
PFN_ID_GLCLEAR id_glClear = 0;
typedef void(APIENTRYP PFN_ID_GLCLEARCOLOR)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
PFN_ID_GLCLEARCOLOR id_glClearColor = 0;
typedef void(APIENTRYP PFN_ID_GLVIEWPORT)(GLint x, GLint y, GLsizei width, GLsizei height);
PFN_ID_GLVIEWPORT id_glViewport = 0;
typedef void(APIENTRYP PFN_ID_GLPIXELSTOREI)(GLenum pname, GLint param);
PFN_ID_GLPIXELSTOREI id_glPixelStorei = 0;
typedef void(APIENTRYP PFN_ID_GLENABLECLIENTSTATE)(GLenum array);
PFN_ID_GLENABLECLIENTSTATE id_glEnableClientState = 0;
typedef void(APIENTRYP PFN_ID_GLDISABLECLIENTSTATE)(GLenum array);
PFN_ID_GLDISABLECLIENTSTATE id_glDisableClientState = 0;
typedef void(APIENTRYP PFN_ID_GLDRAWARRAYS)(GLenum mode, GLint first, GLsizei count);
PFN_ID_GLDRAWARRAYS id_glDrawArrays = 0;
typedef void(APIENTRYP PFN_ID_GLVERTEXPOINTER)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
PFN_ID_GLVERTEXPOINTER id_glVertexPointer = 0;
typedef void(APIENTRYP PFN_ID_GLTEXCOORDPOINTER)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
PFN_ID_GLTEXCOORDPOINTER id_glTexCoordPointer = 0;
// OpenGL 1.3 Function Pointers:
typedef void(APIENTRYP PFN_ID_GLACTIVETEXTUREPROC)(GLenum texture);
PFN_ID_GLACTIVETEXTUREPROC id_glActiveTexture = 0;
// OpenGL 2.0 Function Pointers:
typedef void(APIENTRYP PFN_ID_GLATTACHSHADERPROC)(GLuint program, GLuint shader);
PFN_ID_GLATTACHSHADERPROC id_glAttachShader = 0;
typedef void(APIENTRYP PFN_ID_GLCOMPILESHADERPROC)(GLuint shader);
PFN_ID_GLCOMPILESHADERPROC id_glCompileShader = 0;
typedef GLuint(APIENTRYP PFN_ID_GLCREATEPROGRAMPROC)(void);
PFN_ID_GLCREATEPROGRAMPROC id_glCreateProgram = 0;
typedef GLuint(APIENTRYP PFN_ID_GLCREATESHADERPROC)(GLenum type);
PFN_ID_GLCREATESHADERPROC id_glCreateShader = 0;
typedef void(APIENTRYP PFN_ID_GLDELETEPROGRAMPROC)(GLuint program);
PFN_ID_GLDELETEPROGRAMPROC id_glDeleteProgram = 0;
typedef void(APIENTRYP PFN_ID_GLDELETESHADERPROC)(GLuint shader);
PFN_ID_GLDELETESHADERPROC id_glDeleteShader = 0;
typedef void(APIENTRYP PFN_ID_GLDETACHSHADERPROC)(GLuint program, GLuint shader);
PFN_ID_GLDETACHSHADERPROC id_glDetachShader = 0;
typedef void(APIENTRYP PFN_ID_GLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
PFN_ID_GLGETPROGRAMIVPROC id_glGetProgramiv = 0;
typedef void(APIENTRYP PFN_ID_GLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
PFN_ID_GLGETPROGRAMINFOLOGPROC id_glGetProgramInfoLog = 0;
typedef void(APIENTRYP PFN_ID_GLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
PFN_ID_GLGETSHADERIVPROC id_glGetShaderiv = 0;
typedef void(APIENTRYP PFN_ID_GLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
PFN_ID_GLGETSHADERINFOLOGPROC id_glGetShaderInfoLog = 0;
typedef GLint(APIENTRYP PFN_ID_GLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
PFN_ID_GLGETUNIFORMLOCATIONPROC id_glGetUniformLocation = 0;
typedef void(APIENTRYP PFN_ID_GLLINKPROGRAMPROC)(GLuint program);
PFN_ID_GLLINKPROGRAMPROC id_glLinkProgram = 0;
typedef void(APIENTRYP PFN_ID_GLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length);
PFN_ID_GLSHADERSOURCEPROC id_glShaderSource = 0;
typedef void(APIENTRYP PFN_ID_GLUSEPROGRAMPROC)(GLuint program);
PFN_ID_GLUSEPROGRAMPROC id_glUseProgram = 0;
typedef void(APIENTRYP PFN_ID_GLUNIFORM1IPROC)(GLint location, GLint v0);
PFN_ID_GLUNIFORM1IPROC id_glUniform1i = 0;
// EXT_framebuffer_object
typedef GLboolean(APIENTRYP PFN_ID_GLISFRAMEBUFFEREXTPROC)(GLuint framebuffer);
PFN_ID_GLISFRAMEBUFFEREXTPROC id_glIsFramebufferEXT = 0;
typedef void(APIENTRYP PFN_ID_GLBINDFRAMEBUFFEREXTPROC)(GLenum target, GLuint framebuffer);
PFN_ID_GLBINDFRAMEBUFFEREXTPROC id_glBindFramebufferEXT = 0;
typedef void(APIENTRYP PFN_ID_GLDELETEFRAMEBUFFERSEXTPROC)(GLsizei n, const GLuint *framebuffers);
PFN_ID_GLDELETEFRAMEBUFFERSEXTPROC id_glDeleteFramebuffersEXT = 0;
typedef void(APIENTRYP PFN_ID_GLGENFRAMEBUFFERSEXTPROC)(GLsizei n, GLuint *framebuffers);
PFN_ID_GLGENFRAMEBUFFERSEXTPROC id_glGenFramebuffersEXT = 0;
typedef GLenum(APIENTRYP PFN_ID_GLCHECKFRAMEBUFFERSTATUSEXTPROC)(GLenum target);
PFN_ID_GLCHECKFRAMEBUFFERSTATUSEXTPROC id_glCheckFramebufferStatusEXT = 0;
typedef void(APIENTRYP PFN_ID_GLFRAMEBUFFERTEXTURE2DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
PFN_ID_GLFRAMEBUFFERTEXTURE2DEXTPROC id_glFramebufferTexture2DEXT = 0;
// EXT_framebuffer_blit
typedef void(APIENTRYP PFN_ID_GLBLITFRAMEBUFFEREXTPROC)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
PFN_ID_GLBLITFRAMEBUFFEREXTPROC id_glBlitFramebufferEXT = 0;

static bool VL_SDL2GL_LoadGLProcs()
{
	int majorGLVersion = 0;
	id_glGetString = (PFN_ID_GLGETSTRING)SDL_GL_GetProcAddress("glGetString");
	const char *GLVersion = (const char *)id_glGetString(GL_VERSION);
	sscanf(GLVersion, "%d", &majorGLVersion);
	if (majorGLVersion < 2)
		return false;
	id_glGenTextures = (PFN_ID_GLGENTEXTURES)SDL_GL_GetProcAddress("glGenTextures");
	id_glBindTexture = (PFN_ID_GLBINDTEXTURE)SDL_GL_GetProcAddress("glBindTexture");
	id_glTexParameterf = (PFN_ID_GLTEXPARAMETERF)SDL_GL_GetProcAddress("glTexParameterf");
	id_glTexParameteri = (PFN_ID_GLTEXPARAMETERI)SDL_GL_GetProcAddress("glTexParameteri");
	id_glTexImage1D = (PFN_ID_GLTEXIMAGE1D)SDL_GL_GetProcAddress("glTexImage1D");
	id_glTexImage2D = (PFN_ID_GLTEXIMAGE2D)SDL_GL_GetProcAddress("glTexImage2D");
	id_glTexSubImage1D = (PFN_ID_GLTEXSUBIMAGE1D)SDL_GL_GetProcAddress("glTexSubImage1D");
	id_glTexSubImage2D = (PFN_ID_GLTEXSUBIMAGE2D)SDL_GL_GetProcAddress("glTexSubImage2D");
	id_glDeleteTextures = (PFN_ID_GLDELETETEXTURES)SDL_GL_GetProcAddress("glDeleteTextures");
	id_glEnable = (PFN_ID_GLENABLE)SDL_GL_GetProcAddress("glEnable");
	id_glDisable = (PFN_ID_GLDISABLE)SDL_GL_GetProcAddress("glDisable");
	id_glClear = (PFN_ID_GLCLEAR)SDL_GL_GetProcAddress("glClear");
	id_glClearColor = (PFN_ID_GLCLEARCOLOR)SDL_GL_GetProcAddress("glClearColor");
	id_glViewport = (PFN_ID_GLVIEWPORT)SDL_GL_GetProcAddress("glViewport");
	id_glPixelStorei = (PFN_ID_GLPIXELSTOREI)SDL_GL_GetProcAddress("glPixelStorei");
	id_glEnableClientState = (PFN_ID_GLENABLECLIENTSTATE)SDL_GL_GetProcAddress("glEnableClientState");
	id_glDisableClientState = (PFN_ID_GLDISABLECLIENTSTATE)SDL_GL_GetProcAddress("glDisableClientState");
	id_glDrawArrays = (PFN_ID_GLDRAWARRAYS)SDL_GL_GetProcAddress("glDrawArrays");
	id_glVertexPointer = (PFN_ID_GLVERTEXPOINTER)SDL_GL_GetProcAddress("glVertexPointer");
	id_glTexCoordPointer = (PFN_ID_GLTEXCOORDPOINTER)SDL_GL_GetProcAddress("glTexCoordPointer");
	// OpenGL 1.3
	id_glActiveTexture = (PFN_ID_GLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");
	// OpenGL 2.0
	id_glAttachShader = (PFN_ID_GLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
	id_glCompileShader = (PFN_ID_GLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
	id_glCreateProgram = (PFN_ID_GLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
	id_glCreateShader = (PFN_ID_GLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
	id_glDeleteProgram = (PFN_ID_GLDELETEPROGRAMPROC)SDL_GL_GetProcAddress("glDeleteProgram");
	id_glDeleteShader = (PFN_ID_GLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
	id_glGetProgramiv = (PFN_ID_GLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
	id_glGetProgramInfoLog = (PFN_ID_GLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
	id_glGetShaderiv = (PFN_ID_GLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
	id_glGetShaderInfoLog = (PFN_ID_GLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
	id_glGetUniformLocation = (PFN_ID_GLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
	id_glLinkProgram = (PFN_ID_GLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
	id_glShaderSource = (PFN_ID_GLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
	id_glUseProgram = (PFN_ID_GLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
	id_glUniform1i = (PFN_ID_GLUNIFORM1IPROC)SDL_GL_GetProcAddress("glUniform1i");
	// EXT_framebuffer_object
	if (!SDL_GL_ExtensionSupported("GL_EXT_framebuffer_object"))
		return false;
	id_glIsFramebufferEXT = (PFN_ID_GLISFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glIsFramebufferEXT");
	id_glBindFramebufferEXT = (PFN_ID_GLBINDFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindFramebufferEXT");
	id_glDeleteFramebuffersEXT = (PFN_ID_GLDELETEFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
	id_glGenFramebuffersEXT = (PFN_ID_GLGENFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenFramebuffersEXT");
	id_glCheckFramebufferStatusEXT = (PFN_ID_GLCHECKFRAMEBUFFERSTATUSEXTPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");
	id_glFramebufferTexture2DEXT = (PFN_ID_GLFRAMEBUFFERTEXTURE2DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
	// EXT_framebuffer_blit)
	if (SDL_GL_ExtensionSupported("GL_EXT_framebuffer_blit"))
	{
		id_glBlitFramebufferEXT = (PFN_ID_GLBLITFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glBlitFramebufferEXT");
	}
	return true;
}

static SDL_Window *vl_sdl2gl_window;
static SDL_GLContext vl_sdl2gl_context;
static GLuint vl_sdl2gl_program;
static GLuint vl_sdl2gl_framebufferTexture;
static GLuint vl_sdl2gl_framebufferObject;
static GLuint vl_sdl2gl_palTextureHandle;
static int vl_sdl2gl_framebufferWidth, vl_sdl2gl_framebufferHeight;
static int vl_sdl2gl_screenWidth;
static int vl_sdl2gl_screenHeight;

/* TODO (Overscan border):
 * - If a texture is used for offscreen rendering with scaling applied later,
 * it's better to have the borders within the texture itself.
 */

typedef struct VL_SDL2GL_Surface
{
	VL_SurfaceUsage use;
	GLuint textureHandle;
	GLuint textureHandles[VL_SDL2GL_NUM_BUFFERS];
	int w, h;
	int activePage;
	void *data;
	void *dataPages[VL_SDL2GL_NUM_BUFFERS];
} VL_SDL2GL_Surface;

const char *pxprog = "#version 110\n"
		     "\n"
		     "uniform sampler2D screenBuf;\n"
		     "uniform sampler1D palette;\n"
		     "\n"
		     "void main() {\n"
		     "\tgl_FragColor = texture1D(palette,texture2D(screenBuf, gl_TexCoord[0].xy).r);\n"
		     "}\n";

void VL_SDL2GL_SetIcon(SDL_Window *wnd);

static void VL_SDL2GL_SetVideoMode(int mode)
{
	if (mode == 0xD)
	{
		// Here is how the dimensions of the window are currently picked:
		// 1. The emulated 320x200 sub-window is first zoomed
		// by a factor of 3 (for each dimension) to 960x600.
		// 2. The height is then multiplied by 1.2, so the internal contents
		// (without the borders) have the aspect ratio of 4:3.
		//
		// There are a few more tricks in use to handle the overscan border
		// and VGA line doubling.
		vl_sdl2gl_window = SDL_CreateWindow(VL_WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			VL_DEFAULT_WINDOW_WIDTH, VL_DEFAULT_WINDOW_HEIGHT,
			SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | (vl_isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
		vl_sdl2gl_context = SDL_GL_CreateContext(vl_sdl2gl_window);
		vl_sdl2gl_screenWidth = VL_EGAVGA_GFX_WIDTH;
		vl_sdl2gl_screenHeight = VL_EGAVGA_GFX_HEIGHT;

		SDL_SetWindowMinimumSize(vl_sdl2gl_window, VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER / VL_VGA_GFX_WIDTH_SCALEFACTOR, VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER / VL_VGA_GFX_HEIGHT_SCALEFACTOR);

		VL_SDL2GL_SetIcon(vl_sdl2gl_window);

		if (!VL_SDL2GL_LoadGLProcs())
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
				"Omnispeak", "Omnispeak requires OpenGL 2.0 or higher (with EXT_framebuffer_object) to run. "
					     "Check that your drivers are installed correctly.",
				NULL);
			Quit("Your system does not have one or more required OpenGL extensions.");
		}

		SDL_GL_SetSwapInterval(1);

		// Compile the shader we use to emulate EGA palettes.
		int compileStatus = 0;
		GLuint ps = id_glCreateShader(GL_FRAGMENT_SHADER);
		id_glShaderSource(ps, 1, &pxprog, 0);
		id_glCompileShader(ps);
		id_glGetShaderiv(ps, GL_COMPILE_STATUS, &compileStatus);
		if (!compileStatus)
		{
			id_glDeleteShader(ps);
			Quit("Could not compile palette conversion fragment shader!");
		}

		vl_sdl2gl_program = id_glCreateProgram();
		id_glAttachShader(vl_sdl2gl_program, ps);
		id_glLinkProgram(vl_sdl2gl_program);
		id_glDeleteShader(ps);
		compileStatus = 0;
		id_glGetProgramiv(vl_sdl2gl_program, GL_LINK_STATUS, &compileStatus);
		if (!compileStatus)
		{
			id_glDeleteProgram(vl_sdl2gl_program);
			Quit("Could not link palette conversion program!");
		}

		// Generate palette texture
		id_glGenTextures(1, &vl_sdl2gl_palTextureHandle);
		id_glActiveTexture(GL_TEXTURE1);
		id_glBindTexture(GL_TEXTURE_1D, vl_sdl2gl_palTextureHandle);
		id_glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		id_glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		id_glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		id_glActiveTexture(GL_TEXTURE0);

		// Setup framebuffer stuff, just in case.
		vl_sdl2gl_framebufferWidth = vl_sdl2gl_screenWidth;
		vl_sdl2gl_framebufferHeight = vl_sdl2gl_screenHeight;
		vl_sdl2gl_framebufferTexture = 0;
		vl_sdl2gl_framebufferObject = 0;

		// Hide mouse cursor
		SDL_ShowCursor(0);
	}
	else
	{
		if (vl_sdl2gl_framebufferTexture)
			id_glDeleteTextures(1, &vl_sdl2gl_framebufferTexture);
		if (vl_sdl2gl_framebufferObject)
			id_glDeleteFramebuffersEXT(1, &vl_sdl2gl_framebufferObject);
		id_glDeleteProgram(vl_sdl2gl_program);
		id_glDeleteTextures(1, &vl_sdl2gl_palTextureHandle);
		SDL_ShowCursor(1);
		SDL_GL_DeleteContext(vl_sdl2gl_context);
		SDL_DestroyWindow(vl_sdl2gl_window);
	}
}

static void VL_SDL2GL_SetSurfacePage(VL_SDL2GL_Surface *surf, int page)
{
	if (surf->use != VL_SurfaceUsage_FrontBuffer)
		Quit("Tried to set page for a single buffered surface!");
	
	surf->activePage = page;
	surf->data = surf->dataPages[page];
	surf->textureHandle = surf->textureHandles[page];
}

static void VL_SDL2GL_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_SDL2GL_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)malloc(sizeof(VL_SDL2GL_Surface));
	surf->w = w;
	surf->h = h;
	surf->textureHandle = 0;
	surf->use = usage;
	
	if (usage == VL_SurfaceUsage_FrontBuffer)
	{
		surf->data = malloc(w * h * VL_SDL2GL_NUM_BUFFERS); // 8-bit pal for now
		id_glGenTextures(VL_SDL2GL_NUM_BUFFERS, surf->textureHandles);
		for (int i = 0; i < VL_SDL2GL_NUM_BUFFERS; ++i) {
			id_glBindTexture(GL_TEXTURE_2D, surf->textureHandles[i]);
			id_glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, surf->w, surf->h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
			surf->dataPages[i] = (void*)((uintptr_t)surf->data + i * w * h);
		}
		
		VL_SDL2GL_SetSurfacePage(surf, 0);
	}
	else
	{
		surf->data = malloc(w * h); // 8-bit pal for now
	}
	return surf;
}

static void VL_SDL2GL_DestroySurface(void *surface)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)surface;
	if (surf->use == VL_SurfaceUsage_FrontBuffer)
	{
		id_glDeleteTextures(VL_SDL2GL_NUM_BUFFERS, surf->textureHandles);
		free(surf->dataPages[0]);
	}
	else
	{
		if (surf->data)
			free(surf->data);
	}
	free(surf);
}

static long VL_SDL2GL_GetSurfaceMemUse(void *surface)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)surface;
	if (surf->use == VL_SurfaceUsage_FrontBuffer)
		return surf->w * surf->h * VL_SDL2GL_NUM_BUFFERS;
	else
		return surf->w * surf->h;
}

static void VL_SDL2GL_GetSurfaceDimensions(void *surface, int *w, int *h)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)surface;
	if (w)
		*w = surf->w;
	if (h)
		*h = surf->h;
}

static void VL_SDL2GL_SetGLClearColorFromBorder(void)
{
	id_glClearColor((GLclampf)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][0] / 255,
		(GLclampf)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][1] / 255,
		(GLclampf)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][2] / 255,
		1.0f);
}

static void VL_SDL2GL_RefreshPaletteAndBorderColor(void *screen)
{
	static uint8_t sdl2gl_palette[16][3];

	for (int i = 0; i < 16; i++)
	{
		memcpy(sdl2gl_palette[i], VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]], 3);
	}
	// Load the palette into a texture.
	id_glActiveTexture(GL_TEXTURE1);
	id_glBindTexture(GL_TEXTURE_1D, vl_sdl2gl_palTextureHandle);
	id_glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 16, GL_RGB, GL_UNSIGNED_BYTE, sdl2gl_palette);
	id_glActiveTexture(GL_TEXTURE0);

	VL_SDL2GL_SetGLClearColorFromBorder();
}

static int VL_SDL2GL_SurfacePGet(void *surface, int x, int y)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)surface;
	return ((uint8_t *)surf->data)[y * surf->w + x];
}

static void VL_SDL2GL_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	for (int _y = y; _y < y + h; ++_y)
	{
		memset(((uint8_t *)surf->data) + _y * surf->w + x, colour, w);
	}
}

static void VL_SDL2GL_SurfaceRect_PM(void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	mapmask &= 0xF;
	colour &= mapmask;

	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	for (int _y = y; _y < y + h; ++_y)
	{
		for (int _x = x; _x < x + w; ++_x)
		{
			uint8_t *p = ((uint8_t *)surf->data) + _y * surf->w + _x;
			*p &= ~mapmask;
			*p |= colour;
		}
	}
}

static void VL_SDL2GL_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)src_surface;
	VL_SDL2GL_Surface *dest = (VL_SDL2GL_Surface *)dst_surface;
	for (int _y = sy; _y < sy + sh; ++_y)
	{
		memcpy(((uint8_t *)dest->data) + (_y - sy + y) * dest->w + x, ((uint8_t *)surf->data) + _y * surf->w + sx, sw);
	}
}

static void VL_SDL2GL_SurfaceToSelf(void *surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_SDL2GL_Surface *srf = (VL_SDL2GL_Surface *)surface;
	bool directionY = sy > y;

	if (directionY)
	{
		for (int yi = 0; yi < sh; ++yi)
		{
			memmove(((uint8_t *)srf->data) + ((yi + y) * srf->w + x), ((uint8_t *)srf->data) + ((sy + yi) * srf->w + sx), sw);
		}
	}
	else
	{
		for (int yi = sh - 1; yi >= 0; --yi)
		{
			memmove(((uint8_t *)srf->data) + ((yi + y) * srf->w + x), ((uint8_t *)srf->data) + ((sy + yi) * srf->w + sx), sw);
		}
	}
}

static void VL_SDL2GL_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_UnmaskedToPAL8(src, surf->data, x, y, surf->w, w, h);
}

static void VL_SDL2GL_UnmaskedToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int mapmask)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_UnmaskedToPAL8_PM(src, surf->data, x, y, surf->w, w, h, mapmask);
}

static void VL_SDL2GL_MaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_MaskedToPAL8(src, surf->data, x, y, surf->w, w, h);
}

static void VL_SDL2GL_MaskedBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_MaskedBlitClipToPAL8(src, surf->data, x, y, surf->w, w, h, surf->w, surf->h);
}

static void VL_SDL2GL_BitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_1bppToPAL8(src, surf->data, x, y, surf->w, w, h, colour);
}

static void VL_SDL2GL_BitToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_1bppToPAL8_PM(src, surf->data, x, y, surf->w, w, h, colour, mapmask);
}

static void VL_SDL2GL_BitXorWithSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_1bppXorWithPAL8(src, surf->data, x, y, surf->w, w, h, colour);
}

static void VL_SDL2GL_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_1bppBlitToPAL8(src, surf->data, x, y, surf->w, w, h, colour);
}

static void VL_SDL2GL_BitInvBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_1bppInvBlitClipToPAL8(src, surf->data, x, y, surf->w, w, h, surf->w, surf->h, colour);
}

static void VL_SDL2GL_ScrollSurface(void *surface, int x, int y)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)surface;
	int dx = 0, dy = 0, sx = 0, sy = 0;
	int w = surf->w - CK_Cross_max(x, -x), h = surf->h - CK_Cross_max(y, -y);
	if (x > 0)
	{
		dx = 0;
		sx = x;
	}
	else
	{
		dx = -x;
		sx = 0;
	}
	if (y > 0)
	{
		dy = 0;
		sy = y;
	}
	else
	{
		dy = -y;
		sy = 0;
	}
	
	int oldPage = surf->activePage;
	if (surf->use == VL_SurfaceUsage_FrontBuffer)
	{
		for (int i = 0; i < VL_SDL2GL_NUM_BUFFERS; ++i)
		{
			VL_SDL2GL_SetSurfacePage(surf, i);
			VL_SDL2GL_SurfaceToSelf(surface, dx, dy, sx, sy, w, h);
		}
		VL_SDL2GL_SetSurfacePage(surf, oldPage);
	}
	else
		VL_SDL2GL_SurfaceToSelf(surface, dx, dy, sx, sy, w, h);
}

static void VL_SDL2GL_Present(void *surface, int scrlX, int scrlY, bool singleBuffered)
{
	int realWinW, realWinH;
	// Get the real window size
	SDL_GL_GetDrawableSize(vl_sdl2gl_window, &realWinW, &realWinH);

	VL_CalculateRenderRegions(realWinW, realWinH);

	if (vl_isAspectCorrected)
	{
		id_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		id_glClear(GL_COLOR_BUFFER_BIT);
		VL_SDL2GL_SetGLClearColorFromBorder();
	}

	// Reset the framebuffer object if we can do a better integer scale.
	if (vl_sdl2gl_framebufferWidth != vl_integerWidth || vl_sdl2gl_framebufferHeight != vl_integerHeight)
	{
		if (vl_sdl2gl_framebufferTexture)
			id_glDeleteTextures(1, &vl_sdl2gl_framebufferTexture);
		if (vl_sdl2gl_framebufferObject)
			id_glDeleteFramebuffersEXT(1, &vl_sdl2gl_framebufferObject);
		vl_sdl2gl_framebufferWidth = vl_integerWidth;
		vl_sdl2gl_framebufferHeight = vl_integerHeight;
		vl_sdl2gl_framebufferTexture = 0;
		vl_sdl2gl_framebufferObject = 0;
	}

	if (!vl_sdl2gl_framebufferTexture)
	{
		id_glGenTextures(1, &vl_sdl2gl_framebufferTexture);
		id_glBindTexture(GL_TEXTURE_2D, vl_sdl2gl_framebufferTexture);
		id_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		id_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Use GL_RGBA as GL_RGB causes problems on Mesa/Intel.
		id_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vl_integerWidth, vl_integerHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}

	if (!vl_sdl2gl_framebufferObject)
	{
		id_glGenFramebuffersEXT(1, &vl_sdl2gl_framebufferObject);
		id_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vl_sdl2gl_framebufferObject);
		id_glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, vl_sdl2gl_framebufferTexture, 0);

		GLenum framebufferStatus = id_glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE_EXT)
		{
			Quit("FBO was not complete!");
		}
	}
	else
	{
		id_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vl_sdl2gl_framebufferObject);
	}

	id_glClear(GL_COLOR_BUFFER_BIT);
	id_glViewport(vl_renderRgn_x, vl_renderRgn_y, vl_renderRgn_w, vl_renderRgn_h);

	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)surface;
	id_glBindTexture(GL_TEXTURE_2D, surf->textureHandle);
	id_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	id_glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	id_glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	id_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, GL_RED, GL_UNSIGNED_BYTE, surf->data);

	float scaleX = (float)vl_sdl2gl_screenWidth / ((float)surf->w);
	float scaleY = (float)vl_sdl2gl_screenHeight / ((float)surf->h);
	float offX = (float)(scrlX) / (float)(surf->w);
	float offY = (float)(scrlY) / (float)(surf->h);
	float endX = offX + scaleX;
	float endY = offY + scaleY;

	id_glUseProgram(vl_sdl2gl_program);
	id_glUniform1i(id_glGetUniformLocation(vl_sdl2gl_program, "screenBuf"), 0);
	id_glUniform1i(id_glGetUniformLocation(vl_sdl2gl_program, "palette"), 1);

	float vtxCoords[] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};
	float texCoords[] = {offX, endY, endX, endY, endX, offY, offX, offY};

	id_glEnable(GL_TEXTURE_2D);
	id_glEnableClientState(GL_VERTEX_ARRAY);
	id_glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	id_glVertexPointer(2, GL_FLOAT, 0, vtxCoords);
	id_glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

	id_glDrawArrays(GL_QUADS, 0, 4);

	id_glViewport(vl_fullRgn_x, realWinH - vl_fullRgn_y - vl_fullRgn_h, vl_fullRgn_w, vl_fullRgn_h);
	// Use EXT_framebuffer_blit if available, otherwise draw a quad.
	if (SDL_GL_ExtensionSupported("GL_EXT_framebuffer_blit"))
	{
		id_glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
		id_glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, vl_sdl2gl_framebufferObject);

		id_glBlitFramebufferEXT(0, 0, vl_integerWidth, vl_integerHeight,
			vl_fullRgn_x, realWinH - vl_fullRgn_y - vl_fullRgn_h, vl_fullRgn_x + vl_fullRgn_w, realWinH - vl_fullRgn_y,
			GL_COLOR_BUFFER_BIT,
			GL_LINEAR);
	}
	else
	{
		id_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		float fboCoords[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
		id_glVertexPointer(2, GL_FLOAT, 0, vtxCoords);
		id_glTexCoordPointer(2, GL_FLOAT, 0, fboCoords);
		id_glUseProgram(0);
		id_glEnable(GL_TEXTURE_2D);
		id_glBindTexture(GL_TEXTURE_2D, vl_sdl2gl_framebufferTexture);
		id_glDrawArrays(GL_QUADS, 0, 4);
	}

	if (!singleBuffered && surf->use == VL_SurfaceUsage_FrontBuffer)
		VL_SDL2GL_SetSurfacePage(surf, (surf->activePage + 1) % VL_SDL2GL_NUM_BUFFERS);
	SDL_GL_SwapWindow(vl_sdl2gl_window);
}

static int VL_SDL2GL_GetActiveBufferId(void *surface)
{
	VL_SDL2GL_Surface *srf = (VL_SDL2GL_Surface*)surface;
	return srf->activePage;
}

static int VL_SDL2GL_GetNumBuffers(void *surface)
{
	(void)surface;
	return VL_SDL2GL_NUM_BUFFERS;
}

void VL_SDL2GL_FlushParams()
{
	SDL_SetWindowFullscreen(vl_sdl2gl_window, vl_isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

	SDL_SetWindowMinimumSize(vl_sdl2gl_window, VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER / VL_VGA_GFX_WIDTH_SCALEFACTOR, VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER / VL_VGA_GFX_HEIGHT_SCALEFACTOR);

	if (SDL_GL_SetSwapInterval(vl_swapInterval) < 0)
		vl_swapInterval = SDL_GL_GetSwapInterval();
}

static void VL_SDL2GL_WaitVBLs(int vbls)
{
	SDL_Delay(vbls * 1000 / 70);
}

// Unfortunately, we can't take advantage of designated initializers in C++.
VL_Backend vl_sdl2gl_backend =
	{
		/*.setVideoMode =*/&VL_SDL2GL_SetVideoMode,
		/*.createSurface =*/&VL_SDL2GL_CreateSurface,
		/*.destroySurface =*/&VL_SDL2GL_DestroySurface,
		/*.getSurfaceMemUse =*/&VL_SDL2GL_GetSurfaceMemUse,
		/*.getSurfaceDimensions =*/&VL_SDL2GL_GetSurfaceDimensions,
		/*.refreshPaletteAndBorderColor =*/&VL_SDL2GL_RefreshPaletteAndBorderColor,
		/*.surfacePGet =*/&VL_SDL2GL_SurfacePGet,
		/*.surfaceRect =*/&VL_SDL2GL_SurfaceRect,
		/*.surfaceRect_PM =*/&VL_SDL2GL_SurfaceRect_PM,
		/*.surfaceToSurface =*/&VL_SDL2GL_SurfaceToSurface,
		/*.surfaceToSelf =*/&VL_SDL2GL_SurfaceToSelf,
		/*.unmaskedToSurface =*/&VL_SDL2GL_UnmaskedToSurface,
		/*.unmaskedToSurface_PM =*/&VL_SDL2GL_UnmaskedToSurface_PM,
		/*.maskedToSurface =*/&VL_SDL2GL_MaskedToSurface,
		/*.maskedBlitToSurface =*/&VL_SDL2GL_MaskedBlitToSurface,
		/*.bitToSurface =*/&VL_SDL2GL_BitToSurface,
		/*.bitToSurface_PM =*/&VL_SDL2GL_BitToSurface_PM,
		/*.bitXorWithSurface =*/&VL_SDL2GL_BitXorWithSurface,
		/*.bitBlitToSurface =*/&VL_SDL2GL_BitBlitToSurface,
		/*.bitInvBlitToSurface =*/&VL_SDL2GL_BitInvBlitToSurface,
		/*.scrollSurface =*/&VL_SDL2GL_ScrollSurface,
		/*.present =*/&VL_SDL2GL_Present,
		/*.getActiveBufferId =*/&VL_SDL2GL_GetActiveBufferId,
		/*.getNumBuffers =*/&VL_SDL2GL_GetNumBuffers,
		/*.flushParams =*/&VL_SDL2GL_FlushParams,
		/*.waitVBLs =*/&VL_SDL2GL_WaitVBLs};

VL_Backend *VL_Impl_GetBackend()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_LoadLibrary(0);
	return &vl_sdl2gl_backend;
}
