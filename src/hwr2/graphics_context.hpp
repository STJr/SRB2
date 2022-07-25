#ifndef __HWR2_GRAPHICS_CONTEXT_HPP__
#define __HWR2_GRAPHICS_CONTEXT_HPP__

#include <cstdint>

class IGraphicsContext {
protected:
	~IGraphicsContext() {}
public:
	virtual void Draw(int32_t vertices) = 0;
};

#endif // __HWR2_GRAPHICS_CONTEXT_HPP__
