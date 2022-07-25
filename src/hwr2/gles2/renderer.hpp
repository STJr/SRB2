#ifndef __HWR2_GLES2_RENDERER_HPP__
#define __HWR2_GLES2_RENDERER_HPP__

#include "../renderer.h"

class Gles2Renderer : public IRenderer {
public:
	~Gles2Renderer();

	virtual IGraphicsContext* BeginGraphics();
	virtual void EndGraphics(IGraphicsContext* context);
	virtual ITransferContext* BeginTransfer();
	virtual void EndTransfer(ITransferContext* context);

	virtual GraphicsPipeline* CreateGraphicsPipeline(const GraphicsPipelineInfo& info);
	virtual void DestroyGraphicsPipeline(GraphicsPipeline* pipeline);

	virtual void Present() = 0;
};

#endif
