#ifndef __HWR2_RENDERER_H__
#define __HWR2_RENDERER_H__

#ifdef __cplusplus

#include <cstdint>

#include "graphics_context.hpp"
#include "transfer_context.hpp"

extern "C" {
#endif

/// A C-side handle to a HWR2 rendere instance.
typedef void* hwr2renderer_h;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

struct GraphicsPipeline {
	uint32_t foo;
};

struct GraphicsPipelineInfo {
	uint32_t foo;
};

/// A class which implements functionality for rendering.
class IRenderer {
protected:
	~IRenderer() {}
public:
	virtual IGraphicsContext* BeginGraphics() = 0;
	virtual void EndGraphics(IGraphicsContext* context) = 0;
	virtual ITransferContext* BeginTransfer() = 0;
	virtual void EndTransfer(ITransferContext* context) = 0;

	virtual GraphicsPipeline* CreateGraphicsPipeline(const GraphicsPipelineInfo& info) = 0;
	virtual void DestroyGraphicsPipeline(GraphicsPipeline* pipeline) = 0;

	virtual void Present() = 0;
};

#else

#endif // __cplusplus

#endif // __HWR2_RENDERER_H__
