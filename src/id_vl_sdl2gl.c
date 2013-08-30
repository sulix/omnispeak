#include "id_vl.h"
#include <SDL.h>
#include "glad.h" 

static SDL_Window *vl_sdl2gl_window;
static SDL_GLContext *vl_sdl2gl_context;
static GLuint vl_sdl2gl_program;
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
			"uniform sampler2D palette;\n"\
			"\n"\
			"void main() {\n"\
			"\tgl_FragColor = texture2D(palette,vec2(texture2D(screenBuf, gl_TexCoord[0].xy).r,0.0));\n"\
			"}\n";


static void VL_SDL2GL_SetVideoMode(int w, int h)
{
	vl_sdl2gl_window = SDL_CreateWindow("Commander Keen",SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,w*4,h*4,SDL_WINDOW_OPENGL);
	vl_sdl2gl_context = SDL_GL_CreateContext(vl_sdl2gl_window);
	vl_sdl2gl_screenWidth = w;
	vl_sdl2gl_screenHeight = h;

	gladLoadGLLoader(&SDL_GL_GetProcAddress);

	// Compile the shader we use to emulate EGA palettes.
	GLuint ps = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(ps, 1, &pxprog, 0);
	glCompileShader(ps);
	vl_sdl2gl_program = glCreateProgram();
	glAttachShader(vl_sdl2gl_program, ps);
	glLinkProgram(vl_sdl2gl_program);

	GLuint palTex;
	glGenTextures(1, &palTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, palTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, VL_EGAPalette);
	glActiveTexture(GL_TEXTURE0);
}

static void VL_SDL2GL_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_SDL2GL_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	VL_SDL2GL_Surface *surf = malloc(sizeof(VL_SDL2GL_Surface));
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
	VL_SDL2GL_Surface *surf = (VL_SDL2GL_Surface *)surface;
	glBindTexture(GL_TEXTURE_2D, surf->textureHandle);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, GL_RED, GL_UNSIGNED_BYTE, surf->data);
	
	float scaleX = (float)vl_sdl2gl_screenWidth/((float)surf->w);
	float scaleY = (float)vl_sdl2gl_screenHeight/((float)surf->h);
	float offX = (float)(scrlX)/(float)(vl_sdl2gl_screenWidth);
	float offY = (float)(scrlY)/(float)(vl_sdl2gl_screenHeight);
	float endX = offX + scaleX;
	float endY = offY + scaleY;

	glUseProgram(vl_sdl2gl_program);
	glUniform1i(glGetUniformLocation(vl_sdl2gl_program, "screenBuf"),0);
	glUniform1i(glGetUniformLocation(vl_sdl2gl_program, "palette"),1);
	//TODO: All of the shader stuff.
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(offX,endY);
	glVertex2f(-1,-1);
	glTexCoord2f(endX,endY);
	glVertex2f(1,-1);
	glTexCoord2f(endX,offY);
	glVertex2f(1,1);
	glTexCoord2f(offX,offY);
	glVertex2f(-1,1);
	glEnd();
	SDL_GL_SwapWindow(vl_sdl2gl_window);
}

VL_Backend vl_sdl2gl_backend =
{
	.setVideoMode = &VL_SDL2GL_SetVideoMode,
	.createSurface = &VL_SDL2GL_CreateSurface,
	.destroySurface = &VL_SDL2GL_DestroySurface,
	.getSurfaceMemUse = &VL_SDL2GL_GetSurfaceMemUse,
	.surfaceRect = &VL_SDL2GL_SurfaceRect,
	.surfaceToSurface = &VL_SDL2GL_SurfaceToSurface,
	.surfaceToSelf = &VL_SDL2GL_SurfaceToSelf,
	.unmaskedToSurface = &VL_SDL2GL_UnmaskedToSurface,
	.maskedToSurface = &VL_SDL2GL_MaskedToSurface,
	.maskedBlitToSurface = &VL_SDL2GL_MaskedBlitToSurface,
	.bitToSurface = &VL_SDL2GL_BitToSurface,
	.bitBlitToSurface = &VL_SDL2GL_BitBlitToSurface,
	.present = &VL_SDL2GL_Present
};

VL_Backend *VL_SDL2GL_GetBackend()
{
	SDL_Init(SDL_INIT_VIDEO);
	return &vl_sdl2gl_backend;
}
