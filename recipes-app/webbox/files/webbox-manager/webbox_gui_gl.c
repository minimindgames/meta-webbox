//https://gist.github.com/Miouyouyou/89e9fe56a2c59bce7d4a18a858f389ef

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

    struct my_display {
        struct gbm_device *gbm;
        EGLDisplay egl;
    };

static const struct drm *drm;
/*
static struct {
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
	GLuint program;
	GLint modelviewmatrix, modelviewprojectionmatrix, normalmatrix;
	GLuint vbo;
	GLuint positionsoffset, colorsoffset, normalsoffset;
} gl;
*/


    static void
    check_extensions(void)
    {
//    #ifdef EGL_MESA_platform_gbm
        const char *client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

	printf("EGL Version \"%s\"\n", eglQueryString(EGL_NO_DISPLAY, EGL_VERSION));
	printf("EGL Vendor \"%s\"\n", eglQueryString(EGL_NO_DISPLAY, EGL_VENDOR));
	printf("EGL Extensions \"%s\"\n", eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));

        if (!client_extensions) {
            // EGL_EXT_client_extensions is unsupported.
            abort();
        }
printf("EGL: %s\n", client_extensions);
        if (!strstr(client_extensions, "EGL_MESA_platform_gbm")) {
            abort();
        }
//    #endif
    }

#define EGL_PLATFORM_WAYLAND 0x31D8
#define EGL_PLATFORM_GBM_MESA 0x31D7

void init_gbm_egl_gles(void)
{
//init_drm();
init_gbm();
return;

printf("init_gbm_egl_gles\n");
   check_extensions();

   struct my_display dpy;
   int32_t fd = open ("/dev/dri/renderD128", O_RDWR | FD_CLOEXEC);
   assert (fd > 0);

printf("card0 OK\n");

   struct gbm_device *gbm = gbm_create_device (fd);
//	gbm.dev = gbm_create_device(drm.fd);

/*
struct gbm_surface *gbm_surface;
	gbm_surface = gbm_surface_create(gbm,
			drm.mode->hdisplay, drm.mode->vdisplay,
			GBM_FORMAT_XRGB8888,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!gbm_surface) {
		printf("failed to create gbm surface\n");
		return -1;
	}
*/

//   struct gbm_device *gbm = gbm_create_device (fd);
printf("GBM \n");
   assert (gbm);
printf("GBM OK\n");
 
//Display* x_display = XOpenDisplay(NULL);
//   dpy.egl = eglGetDisplay(gbm);
typedef int32_t EGLint;
	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	static const EGLint config_attribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
/*	static const EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
*/

	PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;
	get_platform_display =
		(void *) eglGetProcAddress("eglGetPlatformDisplayEXT");
printf("eglGetPlatformDisplayEXT %p\n", get_platform_display);
	assert(get_platform_display != NULL);
//dpy.egl = get_platform_display(EGL_PLATFORM_GBM_MESA, gbm, NULL);
dpy.egl = get_platform_display(EGL_PLATFORM_GBM_MESA, gbm, NULL);

// EGL_MESA_platform_gbm ||EGL_KHR_platform_gbm
//dpy.egl = eglGetPlatformDisplay(EGL_PLATFORM_GBM_MESA, dpy.gbm, NULL);
//dpy.egl = get_platform_display(EGL_PLATFORM_GBM_MESA, gbm, NULL);
   assert(dpy.egl != EGL_NO_DISPLAY);
printf("EGL created\n");



EGLint major, minor;
   int res = eglInitialize (dpy.egl, &major, &minor);
   assert (res);

printf("EGL init %d, %d\n", major, minor);

	printf("EGL Version \"%s\"\n", eglQueryString(dpy.egl, EGL_VERSION));
	printf("EGL Vendor \"%s\"\n", eglQueryString(dpy.egl, EGL_VENDOR));
	printf("EGL Extensions \"%s\"\n", eglQueryString(dpy.egl, EGL_EXTENSIONS));


	EGLConfig config;
EGLint n;

	if (!eglChooseConfig(dpy.egl, config_attribs, &config, 1, &n) || n != 1) {
		printf("failed to choose config: %d\n", n);
	}
printf("Config ok\n");



	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		printf("failed to bind api EGL_OPENGL_ES_API\n");
	}
printf("Bind ok\n");


//	EGLSurface surface;
	EGLContext context = eglCreateContext(dpy.egl, config,
			EGL_NO_CONTEXT, context_attribs);

	if (context == NULL) {
		printf("failed to create context\n");
	}
printf("Context ok\n");
/*
        struct gbm_surface *gbm_surface = gbm_surface_create(gbm,
                                        256, 256,
                                        GBM_FORMAT_XRGB8888,
                                        GBM_BO_USE_RENDERING);

	if (gbm_surface == NULL) {
		printf("failed to create surface\n");
	}
printf("GBM surface ok\n");
*/
/*
PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC get_platform_surface = NULL;
	get_platform_surface =
		(void *) eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");
printf("eglCreatePlatformWindowSurfaceEXT %p\n", get_platform_surface);
	assert(get_platform_surface != NULL);
EGLSurface egl_surface = 
	//eglCreatePlatformWindowSurfaceEXT(
	get_platform_surface(
							dpy.egl,
                                                       config,
                                                       gbm_surface,
                                                       NULL);
printf("EGL surface ok\n");
*/

/*
	EGLSurface egl_surface = eglCreateWindowSurface(dpy.egl, config, gbm_surface, NULL);
	if (egl_surface == EGL_NO_SURFACE) {
		printf("failed to create egl surface\n");
	}

*/
//	eglMakeCurrent(dpy.egl, egl_surface, egl_surface, context);
	eglMakeCurrent(dpy.egl, EGL_NO_SURFACE, EGL_NO_SURFACE, context);

	printf("GL_VENDOR: \"%s\"\n", glGetString(GL_VENDOR));
	printf("GL_RENDERER: \"%s\"\n", glGetString(GL_RENDERER));
	printf("GL_VERSION: \"%s\"\n", glGetString(GL_VERSION));
	printf("GL Extensions: \"%s\"\n", glGetString(GL_EXTENSIONS));


printf("INIT DONE\n");

float b = 0;
for (;;) {
    glClearColor(0.5f, 0.5f, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();

b += 0.1;
if (b >= 1) b = 0;
sleep(1);
//    eglSwapBuffers(dpy.egl, egl_surface);
printf("b %f\n", b);
}

printf("ALL DONE\n");
}

#if 0
// gcc -o drmgl Linux_DRM_OpenGLES.c `pkg-config --cflags --libs libdrm` -lgbm -lEGL -lGLESv2

/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 * Copyright (c) 2012 Rob Clark <rob@ti.com>
 * Copyright (c) 2017 Miouyouyou <Myy> <myy@miouyouyou.fr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Based on a egl cube test app originally written by Arvin Schnell */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <assert.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


static struct {
	struct gbm_device *dev;
	struct gbm_surface *surface;
} gbm;

static struct {
	int fd;
	drmModeModeInfo *mode;
	uint32_t crtc_id;
	uint32_t connector_id;
} drm;

struct drm_fb {
	struct gbm_bo *bo;
	uint32_t fb_id;
};

static uint32_t find_crtc_for_encoder(const drmModeRes *resources,
				      const drmModeEncoder *encoder) {
	int i;

	for (i = 0; i < resources->count_crtcs; i++) {
		/* possible_crtcs is a bitmask as described here:
		 * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
		 */
		const uint32_t crtc_mask = 1 << i;
		const uint32_t crtc_id = resources->crtcs[i];
		if (encoder->possible_crtcs & crtc_mask) {
			return crtc_id;
		}
	}

	/* no match found */
	return -1;
}

static uint32_t find_crtc_for_connector(const drmModeRes *resources,
					const drmModeConnector *connector) {
	int i;

	for (i = 0; i < connector->count_encoders; i++) {
		const uint32_t encoder_id = connector->encoders[i];
		drmModeEncoder *encoder = drmModeGetEncoder(drm.fd, encoder_id);

		if (encoder) {
			const uint32_t crtc_id = find_crtc_for_encoder(resources, encoder);

			drmModeFreeEncoder(encoder);
			if (crtc_id != 0) {
				return crtc_id;
			}
		}
	}

	/* no match found */
	return -1;
}

static int init_drm(void)
{
	drmModeRes *resources;
	drmModeConnector *connector = NULL;
	drmModeEncoder *encoder = NULL;
	int i, area;

	drm.fd = open("/dev/dri/card0", O_RDWR);
	
	if (drm.fd < 0) {
		printf("could not open drm device\n");
		return -1;
	}

	resources = drmModeGetResources(drm.fd);
	if (!resources) {
		printf("drmModeGetResources failed: %s\n", strerror(errno));
		return -1;
	}

	/* find a connected connector: */
	for (i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
		if (connector->connection == DRM_MODE_CONNECTED) {
			/* it's connected, let's use this! */
			break;
		}
		drmModeFreeConnector(connector);
		connector = NULL;
	}

	if (!connector) {
		/* we could be fancy and listen for hotplug events and wait for
		 * a connector..
		 */
		printf("no connected connector!\n");
		return -1;
	}

	/* find prefered mode or the highest resolution mode: */
	for (i = 0, area = 0; i < connector->count_modes; i++) {
		drmModeModeInfo *current_mode = &connector->modes[i];

		if (current_mode->type & DRM_MODE_TYPE_PREFERRED) {
			drm.mode = current_mode;
		}

		int current_area = current_mode->hdisplay * current_mode->vdisplay;
		if (current_area > area) {
			drm.mode = current_mode;
			area = current_area;
		}
	}

	if (!drm.mode) {
		printf("could not find mode!\n");
		return -1;
	}

	/* find encoder: */
	for (i = 0; i < resources->count_encoders; i++) {
		encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
		if (encoder->encoder_id == connector->encoder_id)
			break;
		drmModeFreeEncoder(encoder);
		encoder = NULL;
	}

	if (encoder) {
		drm.crtc_id = encoder->crtc_id;
	} else {
		uint32_t crtc_id = find_crtc_for_connector(resources, connector);
		if (crtc_id == 0) {
			printf("no crtc found!\n");
			return -1;
		}

		drm.crtc_id = crtc_id;
	}

	drm.connector_id = connector->connector_id;

	return 0;
}

static int init_gbm(void)
{
	return 0;
}

static int init_gl(void)
{
	EGLint major, minor, n;
	GLuint vertex_shader, fragment_shader;
	GLint ret;

	gl.display = get_platform_display(EGL_PLATFORM_GBM_KHR, gbm.dev, NULL);

	if (!eglInitialize(gl.display, &major, &minor)) {
		printf("failed to initialize\n");
		return -1;
	}

	printf("Using display %p with EGL version %d.%d\n",
			gl.display, major, minor);












	return 0;
}

/* Draw code here */
static void draw(uint32_t i)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.2f, 0.3f, 0.5f, 1.0f);
}

static void
drm_fb_destroy_callback(struct gbm_bo *bo, void *data)
{
	struct drm_fb *fb = data;
	struct gbm_device *gbm = gbm_bo_get_device(bo);

	if (fb->fb_id)
		drmModeRmFB(drm.fd, fb->fb_id);

	free(fb);
}

static struct drm_fb * drm_fb_get_from_bo(struct gbm_bo *bo)
{
	struct drm_fb *fb = gbm_bo_get_user_data(bo);
	uint32_t width, height, stride, handle;
	int ret;

	if (fb)
		return fb;

	fb = calloc(1, sizeof *fb);
	fb->bo = bo;

	width = gbm_bo_get_width(bo);
	height = gbm_bo_get_height(bo);
	stride = gbm_bo_get_stride(bo);
	handle = gbm_bo_get_handle(bo).u32;

	ret = drmModeAddFB(drm.fd, width, height, 24, 32, stride, handle, &fb->fb_id);
	if (ret) {
		printf("failed to create fb: %s\n", strerror(errno));
		free(fb);
		return NULL;
	}

	gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

	return fb;
}

static void page_flip_handler(int fd, unsigned int frame,
		  unsigned int sec, unsigned int usec, void *data)
{
	int *waiting_for_flip = data;
	*waiting_for_flip = 0;
}

int main2(int argc, char *argv[])
{
	fd_set fds;
	drmEventContext evctx = {
			.version = DRM_EVENT_CONTEXT_VERSION,
			.page_flip_handler = page_flip_handler,
	};
	struct gbm_bo *bo;
	struct drm_fb *fb;
	uint32_t i = 0;
	int ret;

	ret = init_drm();
	if (ret) {
		printf("failed to initialize DRM\n");
		return ret;
	}

	FD_ZERO(&fds);
	FD_SET(0, &fds);
	FD_SET(drm.fd, &fds);

	ret = init_gbm();
	if (ret) {
		printf("failed to initialize GBM\n");
		return ret;
	}

	ret = init_gl();
	if (ret) {
		printf("failed to initialize EGL\n");
		return ret;
	}

	/* clear the color buffer */
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(gl.display, gl.surface);
	bo = gbm_surface_lock_front_buffer(gbm.surface);
	fb = drm_fb_get_from_bo(bo);

	/* set mode: */
	ret = drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0,
			&drm.connector_id, 1, drm.mode);
	if (ret) {
		printf("failed to set mode: %s\n", strerror(errno));
		return ret;
	}

	while (1) {
		struct gbm_bo *next_bo;
		int waiting_for_flip = 1;

		draw(i++);

		eglSwapBuffers(gl.display, gl.surface);
		next_bo = gbm_surface_lock_front_buffer(gbm.surface);
		fb = drm_fb_get_from_bo(next_bo);

		/*
		 * Here you could also update drm plane layers if you want
		 * hw composition
		 */

		ret = drmModePageFlip(drm.fd, drm.crtc_id, fb->fb_id,
				DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
		if (ret) {
			printf("failed to queue page flip: %s\n", strerror(errno));
			return -1;
		}

		while (waiting_for_flip) {
			ret = select(drm.fd + 1, &fds, NULL, NULL, NULL);
			if (ret < 0) {
				printf("select err: %s\n", strerror(errno));
				return ret;
			} else if (ret == 0) {
				printf("select timeout!\n");
				return -1;
			} else if (FD_ISSET(0, &fds)) {
				printf("user interrupted!\n");
				break;
			}
			drmHandleEvent(drm.fd, &evctx);
		}

		/* release last buffer to render on again: */
		gbm_surface_release_buffer(gbm.surface, bo);
		bo = next_bo;
	}

	return ret;
}
#endif

