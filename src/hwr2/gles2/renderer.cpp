#include "renderer.hpp"

Gles2Renderer::~Gles2Renderer()
{
}

IGraphicsContext* Gles2Renderer::BeginGraphics()
{
	return nullptr;
}

void Gles2Renderer::EndGraphics(IGraphicsContext* context)
{
}

ITransferContext* Gles2Renderer::BeginTransfer()
{
	return nullptr;
}

void Gles2Renderer::EndTransfer(ITransferContext* context)
{
}

GraphicsPipeline* Gles2Renderer::CreateGraphicsPipeline(const GraphicsPipelineInfo& info)
{
	return nullptr;
}

void Gles2Renderer::DestroyGraphicsPipeline(GraphicsPipeline* pipeline)
{
}
