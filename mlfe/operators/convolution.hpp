#ifndef __CONVOLUTION_BASE_OP_HPP__
#define __CONVOLUTION_BASE_OP_HPP__

#include "operator.hpp"
#include "../core/tensor_blob.hpp"
#include "../core/param_def.hpp"

namespace mlfe{

template <class DeviceContext>
class ConvolutionBaseOp : public Operator<DeviceContext>{
public:
    void Compute() override = 0;
    
protected:
    explicit ConvolutionBaseOp(
                               std::vector<std::shared_ptr<TensorBlob<DeviceContext>>> inputs,
                               std::vector<std::shared_ptr<TensorBlob<DeviceContext>>> outputs,
                               ParamDef param
                               ) : Operator<DeviceContext>(inputs, outputs, param){}
    
    int OutHeightSize(){
        int height = this->Input(0)->Dim(2);
        return (height + 2 * padding - kernel_size[0]) / stride[0] + 1;
    }
    
    int OutWidthSize(){
        int width = this->Input(0)->Dim(3);
        return (width + 2 * padding - kernel_size[1]) / stride[1] + 1;
    }
    
    std::vector<int> kernel_size;
    std::vector<int> stride;
    int filters;
    int padding;
};

} /* namespace mlfe */
#endif /* __CONVOLUTION_BASE_OP_HPP__ */