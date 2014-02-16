#include "id_vl.h"
#include "id_us.h"
#include <stdlib.h>
#include <SDL.h>
#include <SDL_opengl.h>


// OpenGL 1.3 Function Pointers:
typedef void (APIENTRYP PFN_ID_GLACTIVETEXTUREPROC) (GLenum texture);
PFN_ID_GLACTIVETEXTUREPROC id_glActiveTexture = 0;
// OpenGL 2.0 Function Pointers:
typedef void (APIENTRYP PFN_ID_GLATTACHSHADERPROC) (GLuint program, GLuint shader);
PFN_ID_GLATTACHSHADERPROC id_glAttachShader = 0;
typedef void (APIENTRYP PFN_ID_GLCOMPILESHADERPROC) (GLuint shader);
PFN_ID_GLCOMPILESHADERPROC id_glCompileShader = 0;
typedef GLuint (APIENTRYP PFN_ID_GLCREATEPROGRAMPROC) (void);
PFN_ID_GLCREATEPROGRAMPROC id_glCreateProgram = 0;
typedef GLuint (APIENTRYP PFN_ID_GLCREATESHADERPROC) (GLenum type);
PFN_ID_GLCREATESHADERPROC id_glCreateShader = 0;
typedef void (APIENTRYP PFN_ID_GLDELETEPROGRAMPROC) (GLuint program);
PFN_ID_GLDELETEPROGRAMPROC id_glDeleteProgram = 0;
typedef void (APIENTRYP PFN_ID_GLDELETESHADERPROC) (GLuint shader);
PFN_ID_GLDELETESHADERPROC id_glDeleteShader = 0;
typedef void (APIENTRYP PFN_ID_GLDETACHSHADERPROC) (GLuint program, GLuint shader);
PFN_ID_GLDETACHSHADERPROC id_glDetachShader = 0;
typedef void (APIENTRYP PFN_ID_GLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
PFN_ID_GLGETPROGRAMIVPROC id_glGetProgramiv = 0;
typedef void (APIENTRYP PFN_ID_GLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
PFN_ID_GLGETPROGRAMINFOLOGPROC id_glGetProgramInfoLog = 0;
typedef void (APIENTRYP PFN_ID_GLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
PFN_ID_GLGETSHADERIVPROC id_glGetShaderiv = 0;
typedef void (APIENTRYP PFN_ID_GLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
PFN_ID_GLGETSHADERINFOLOGPROC id_glGetShaderInfoLog = 0;
typedef GLint (APIENTRYP PFN_ID_GLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
PFN_ID_GLGETUNIFORMLOCATIONPROC id_glGetUniformLocation = 0;
typedef void (APIENTRYP PFN_ID_GLLINKPROGRAMPROC) (GLuint program);
PFN_ID_GLLINKPROGRAMPROC id_glLinkProgram = 0;
typedef void (APIENTRYP PFN_ID_GLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length);
PFN_ID_GLSHADERSOURCEPROC id_glShaderSource = 0;
typedef void (APIENTRYP PFN_ID_GLUSEPROGRAMPROC) (GLuint program);
PFN_ID_GLUSEPROGRAMPROC id_glUseProgram = 0;
typedef void (APIENTRYP PFN_ID_GLUNIFORM1IPROC) (GLint location, GLint v0);
PFN_ID_GLUNIFORM1IPROC id_glUniform1i = 0;
// EXT_framebuffer_object
typedef GLboolean (APIENTRYP PFN_ID_GLISFRAMEBUFFEREXTPROC) (GLuint framebuffer);
PFN_ID_GLISFRAMEBUFFEREXTPROC id_glIsFramebufferEXT = 0;
typedef void (APIENTRYP PFN_ID_GLBINDFRAMEBUFFEREXTPROC) (GLenum target, GLuint framebuffer);
PFN_ID_GLBINDFRAMEBUFFEREXTPROC id_glBindFramebufferEXT = 0;
typedef void (APIENTRYP PFN_ID_GLDELETEFRAMEBUFFERSEXTPROC) (GLsizei n, const GLuint *framebuffers);
PFN_ID_GLDELETEFRAMEBUFFERSEXTPROC id_glDeleteFramebuffersEXT = 0;
typedef void (APIENTRYP PFN_ID_GLGENFRAMEBUFFERSEXTPROC) (GLsizei n, GLuint *framebuffers);
PFN_ID_GLGENFRAMEBUFFERSEXTPROC id_glGenFramebuffersEXT = 0;
typedef GLenum (APIENTRYP PFN_ID_GLCHECKFRAMEBUFFERSTATUSEXTPROC) (GLenum target);
PFN_ID_GLCHECKFRAMEBUFFERSTATUSEXTPROC id_glCheckFramebufferStatusEXT = 0;
typedef void (APIENTRYP PFN_ID_GLFRAMEBUFFERTEXTURE2DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
PFN_ID_GLFRAMEBUFFERTEXTURE2DEXTPROC id_glFramebufferTexture2DEXT = 0;
// EXT_framebuffer_blit
typedef void (APIENTRYP PFN_ID_GLBLITFRAMEBUFFEREXTPROC) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
PFN_ID_GLBLITFRAMEBUFFEREXTPROC id_glBlitFramebufferEXT = 0;


static bool VL_SDL2GL_LoadGLProcs()
{
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
	if (!SDL_GL_ExtensionSupported("GL_EXT_framebuffer_object")) return false;
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
static int vl_sdl2gl_framebufferWidth, vl_sdl2gl_framebufferHeight;
static int vl_sdl2gl_screenWidth;
static int vl_sdl2gl_screenHeight;

typedef struct VL_SDL2GL_Surface
{
	VL_SurfaceUsage use;
	GLuint textureHandle;
	int w, h;
	void *data;
} VL_SDL2GL_Surface;	

const char *pxprog = 	"#version 110\n"\
			"\n"\
			"uniform sampler2D screenBuf;\n"\
			"uniform sampler1D palette;\n"\
			"\n"\
			"void main() {\n"\
			"\tgl_FragColor = texture1D(palette,texture2D(screenBuf, gl_TexCoord[0].xy).r);\n"\
			"}\n";


static void VL_SDL2GL_SetVideoMode(int w, int h)
{
	vl_sdl2gl_window = SDL_CreateWindow("Commander Keen",SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,w*4,h*4,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	vl_sdl2gl_context = SDL_GL_CreateContext(vl_sdl2gl_window);
	vl_sdl2gl_screenWidth = w;
	vl_sdl2gl_screenHeight = h;

	if (!VL_SDL2GL_LoadGLProcs())
	{
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

	// Load the palette into a texture.
	// TODO: Redo the palette API so we don't need to hack this and read invalid memory. :/
	GLuint palTex;
	glGenTextures(1, &palTex);
	id_glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, palTex);
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 16, GL_RGB, GL_UNSIGNED_BYTE, VL_EGAPalette);
	id_glActiveTexture(GL_TEXTURE0);

	// Setup framebuffer stuff, just in case.
	vl_sdl2gl_framebufferWidth = w;
	vl_sdl2gl_framebufferHeight = h;
	vl_sdl2gl_framebufferTexture = 0;
	vl_sdl2gl_framebufferObject = 0;
}

static void VL_SDL2GL_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_SDL2GL_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface*)malloc(sizeof(VL_SDL2GL_Surface));
	surf->w = w;
	surf->h = h;
	surf->textureHandle = 0;
	if (usage == VL_SurfaceUsage_FrontBuffer)
	{
		glGenTextures(1, &(surf->textureHandle));
		glBindTexture(GL_TEXTURE_2D, surf->textureHandle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, surf->w, surf->h, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
	}
	surf->data = malloc(w * h); // 8-bit pal for now
	return surf;
}

static void VL_SDL2GL_DestroySurface(void *surface)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface*)surface;
	if (surf->textureHandle)
		glDeleteTextures(1, &(surf->textureHandle));
	if (surf->data)
		free(surf->data);
	free(surf);
}

static long VL_SDL2GL_GetSurfaceMemUse(void *surface)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)surface;
	return surf->w*surf->h;
}

static void VL_SDL2GL_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface*) dst_surface;
	for (int _y = y; _y < y+h; ++_y)
	{
		memset(((uint8_t*)surf->data)+_y*surf->w+x, colour, w);
	}
}

static void VL_SDL2GL_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)src_surface;
	VL_SDL2GL_Surface *dest = (VL_SDL2GL_Surface *)dst_surface;
	for (int _y = sy; _y < sy+sh; ++_y)
	{
		memcpy(((uint8_t*)dest->data)+(_y-sy+y)*dest->w+x,((uint8_t*)surf->data)+_y*surf->w+sx, sw);
	}
}

static void VL_SDL2GL_SurfaceToSelf(void *surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_SDL2GL_Surface *srf = (VL_SDL2GL_Surface *)surface;
	bool directionX = sx > x;
	bool directionY = sy > y;

	if (directionY)
	{
		for (int yi = 0; yi < sh; ++yi)
		{
			memmove(((uint8_t*)srf->data)+((yi+y)*srf->w+x),((uint8_t*)srf->data)+((sy+yi)*srf->w+sx),sw);
		}
	}
	else	
	{
		for (int yi = sh-1; yi >= 0; --yi)
		{
			memmove(((uint8_t*)srf->data)+((yi+y)*srf->w+x),((uint8_t*)srf->data)+((sy+yi)*srf->w+sx),sw);
		}
	}

}

static void VL_SDL2GL_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h) {
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_UnmaskedToPAL8(src, surf->data, x, y, surf->w, w, h);
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

static void VL_SDL2GL_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)dst_surface;
	VL_1bppBlitToPAL8(src, surf->data, x, y, surf->w, w,h, colour);
}

static void VL_SDL2GL_Present(void *surface, int scrlX, int scrlY)
{
	// Get the real window size
	int realWinX, realWinY;
	SDL_GetWindowSize(vl_sdl2gl_window, &realWinX, &realWinY);

	int integerScaleX = (realWinX/vl_sdl2gl_screenWidth)*vl_sdl2gl_screenWidth;
	int integerScaleY = (realWinY/vl_sdl2gl_screenHeight)*vl_sdl2gl_screenHeight;

	// If our gfx hardware supports it, render into an offscreen framebuffer for the final linear phase of scaling.
	if (id_glBlitFramebufferEXT)
	{
		// Reset the framebuffer object if we can do a better integer scale.
		if (vl_sdl2gl_framebufferWidth != integerScaleX || vl_sdl2gl_framebufferHeight != integerScaleY)
		{
			if (vl_sdl2gl_framebufferTexture)
				glDeleteTextures(1, &vl_sdl2gl_framebufferTexture);
			if (vl_sdl2gl_framebufferObject)
				id_glDeleteFramebuffersEXT(1, &vl_sdl2gl_framebufferObject);
			vl_sdl2gl_framebufferWidth = integerScaleX;
			vl_sdl2gl_framebufferHeight = integerScaleY;
			vl_sdl2gl_framebufferTexture = 0;
			vl_sdl2gl_framebufferObject = 0;
		}

		if (!vl_sdl2gl_framebufferTexture)
		{
			glGenTextures(1, &vl_sdl2gl_framebufferTexture);
			glBindTexture(GL_TEXTURE_2D, vl_sdl2gl_framebufferTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// Use GL_RGBA as GL_RGB causes problems on Mesa/Intel.
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, integerScaleX, integerScaleY, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
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
		
		glViewport(0, 0, integerScaleX, integerScaleY);
		
	}
	else
	{
		// Otherwise do nearest-neighbour scaling the whole time.
		glViewport(0, 0, realWinX, realWinY);
	}

	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)surface;
	glBindTexture(GL_TEXTURE_2D, surf->textureHandle);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, GL_RED, GL_UNSIGNED_BYTE, surf->data);
	
	float scaleX = (float)vl_sdl2gl_screenWidth/((float)surf->w);
	float scaleY = (float)vl_sdl2gl_screenHeight/((float)surf->h);
	float offX = (float)(scrlX)/(float)(surf->w);
	float offY = (float)(scrlY)/(float)(surf->h);
	float endX = offX + scaleX;
	float endY = offY + scaleY;

	id_glUseProgram(vl_sdl2gl_program);
	id_glUniform1i(id_glGetUniformLocation(vl_sdl2gl_program, "screenBuf"),0);
	id_glUniform1i(id_glGetUniformLocation(vl_sdl2gl_program, "palette"),1);

	float vtxCoords[] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};
	float texCoords[] = {offX, endY, endX, endY, endX, offY, offX, offY};

	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vtxCoords);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

	glDrawArrays(GL_QUADS, 0, 4);

	glViewport(0, 0, realWinX, realWinY);
	// If we're using framebuffers, linearly scale it to the screen.
	if (true)
	{
		// Use EXT_framebuffer_blit if available, otherwise draw a quad.
		if (id_glBlitFramebufferEXT)
		{
			id_glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
			id_glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, vl_sdl2gl_framebufferObject);
			
			id_glBlitFramebufferEXT(0, 0, integerScaleX, integerScaleY,
						0, 0, realWinX, realWinY,
						GL_COLOR_BUFFER_BIT,
						GL_LINEAR);
		}
		else
		{
			id_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			float fboCoords[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
			glVertexPointer(2, GL_FLOAT, 0, vtxCoords);
			glTexCoordPointer(2, GL_FLOAT, 0, fboCoords);
			id_glUseProgram(0);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, vl_sdl2gl_framebufferTexture);
			glDrawArrays(GL_QUADS, 0, 4);
		}
	}

	SDL_GL_SwapWindow(vl_sdl2gl_window);
}

// Unfortunately, we can't take advantage of designated initializers in C++.
VL_Backend vl_sdl2gl_backend =
{
	/*.setVideoMode =*/ &VL_SDL2GL_SetVideoMode,
	/*.createSurface =*/ &VL_SDL2GL_CreateSurface,
	/*.destroySurface =*/ &VL_SDL2GL_DestroySurface,
	/*.getSurfaceMemUse =*/ &VL_SDL2GL_GetSurfaceMemUse,
	/*.surfaceRect =*/ &VL_SDL2GL_SurfaceRect,
	/*.surfaceToSurface =*/ &VL_SDL2GL_SurfaceToSurface,
	/*.surfaceToSelf =*/ &VL_SDL2GL_SurfaceToSelf,
	/*.unmaskedToSurface =*/ &VL_SDL2GL_UnmaskedToSurface,
	/*.maskedToSurface =*/ &VL_SDL2GL_MaskedToSurface,
	/*.maskedBlitToSurface =*/ &VL_SDL2GL_MaskedBlitToSurface,
	/*.bitToSurface =*/ &VL_SDL2GL_BitToSurface,
	/*.bitBlitToSurface =*/ &VL_SDL2GL_BitBlitToSurface,
	/*.present =*/ &VL_SDL2GL_Present
};

VL_Backend *VL_Impl_GetBackend()
{
	SDL_Init(SDL_INIT_VIDEO);
	return &vl_sdl2gl_backend;
}
