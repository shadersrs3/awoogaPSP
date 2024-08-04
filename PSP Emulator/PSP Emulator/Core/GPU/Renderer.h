#pragma once

#include <cstdint>

namespace Core::GPU {
struct GPUState;

enum RendererType : int {
    RENDERER_TYPE_NONE,
    RENDERER_TYPE_SW,
    RENDERER_TYPE_OPENGL,
    RENDERER_TYPE_VULKAN
};

struct RenderDevice {
public:
    virtual ~RenderDevice() {}
    virtual void displayListBegin() = 0;
    virtual void displayListEnd() = 0;
    virtual void prepareDraw(GPUState *state, int type, int count) = 0;
    virtual void drawPrimitive(GPUState *state, int type, int count) = 0;
    virtual void endDraw(GPUState *state) = 0;
    virtual const char *getDeviceName() { return "(null)"; }
    virtual RendererType getDeviceType() = 0;
};

RenderDevice *createRenderDevice(RendererType type);
void destroyRenderDevice(RenderDevice *device);

RenderDevice *getRenderDevice();
void setRenderDevice(RenderDevice *device);

void __openglUpdateFramebuffer(RenderDevice *device, int x, int y, int w, int h);
void __openglRenderScreen(RenderDevice *device);
void __openglSetStreamingTextureDevice(RenderDevice *device, bool state);
void __openglDebugTexture(const char *texcache);

void __RenderDeviceDisplayListBegin();
void __RenderDeviceDisplayListEnd();

void __DrawDebugPrimitive(GPUState *state, int type, int count);
void __PrepareDraw(GPUState *state, int type, int count);
void __DrawPrimitive(GPUState *state, int type, int count);
void __EndDraw(GPUState *state);

}