#include "fully_connected.hpp"
#include "../device_context/cpu_context.hpp"

namespace mlfe{

template <class DT, class DC>
FullyConnectedOp<DT, DC>::FullyConnectedOp(
                                                      OperatorIO &opio,
                                                      ItemHolder *ih
                                                      ) : Operator<DC>(opio, ih) {
    runtime_assert(this->inputs.size() == 3,
                   "[Fully Connected Op] inputs.size() == 3.");
    runtime_assert(this->outputs.size() == 1,
                   "[Fully Connected Op] outputs.size() == 1.");
    
    const auto x = this->inputs[InputSchema::x];
    const auto w = this->inputs[InputSchema::w];
    const auto b = this->inputs[InputSchema::b];
    auto y = this->outputs[OutputSchema::y];
    int units;
    
    if(opio.param.HasParam("Units") &&
       w->IsEmpty() &&
       b->IsEmpty() &&
       y->IsEmpty() &&
       !x->IsEmpty() &&
       x->Dims() == 2){
        units = opio.param.GetParam<int>("Units");
        w->template Resize<DT>({units, x->Dim(1)});
        b->template Resize<DT>({units});
        y->template Resize<DT>({x->Dim(0), units});
    }
    else{
        runtime_assert(x->Dims() == 2,
                       "[Fully Connected Op] x->Dims() == 2.");
        runtime_assert(x->Dim(0) == y->Dim(0),
                       "[Fully Connected Op] x->Dim(0) == y->Dim(0).");
        runtime_assert(x->Dim(1) == w->Dim(1),
                       "[Fully Connected Op] x->Dim(1) == w->Dim(1).");
        runtime_assert(y->Dim(1) == w->Dim(0),
                       "[Fully Connected Op] y->Dim(1) == w->Dim(0).");
    }
    
    bias_multiplier.template Resize<DT, DC>({x->Dim(0)});
    bias_multiplier.template SetByConst<DT>(DT(1));
    
    /*
     * batch size.
     */
    m = x->Dim(0);
    /*
     * output size.
     */
    n = w->Dim(0);
    /*
     * total input's element size.
     */
    k = w->Dim(1);
}

template <class DT, class DC>
void FullyConnectedOp<DT, DC>::Compute(){
    const auto x = this->inputs[InputSchema::x];
    const auto w = this->inputs[InputSchema::w];
    const auto b = this->inputs[InputSchema::b];
    auto y = this->outputs[OutputSchema::y];
    /*
     * Forward computation.
     * x(batch_size x input_size) * w(output_size x input_size)^T
     *  = y(batch_size x output_size)
     */
    math::gemm<DT, DC>(
                                  false, true,
                                  m, n, k,
                                  DT(1), x->template GetPtrConst<DT>(), k,
                                  w->template GetPtrConst<DT>(), k,
                                  DT(0), y->template GetPtrMutable<DT>(), n, nullptr
                                  );
    
    /*
     * Add the bias term.
     * y = y + b;
     */
    
    math::gemm<DT, DC>(
                                  false, false,
                                  m, n, 1,
                                  DT(1), bias_multiplier.template GetPtrConst<DT>(), 1
                                  , b->template GetPtrConst<DT>(), n,
                                  DT(1), y->template GetPtrMutable<DT>(), n, nullptr
                                  );
}

REGIST_OPERATOR_CPU(FC_float, FullyConnectedOp<float, CPUContext>)
REGIST_OPERATOR_CPU(FC_double, FullyConnectedOp<double, CPUContext>)

template <class DT, class DC>
FullyConnectedGradientOp<DT, DC>::FullyConnectedGradientOp(
                                                                      OperatorIO &opio,
                                                                      ItemHolder *ih
                                                                      ) : Operator<DC>(opio, ih){
    runtime_assert(this->inputs.size() == 3,
                   "[Fully Connected Gradient Op] inputs.size() == 3.");
    runtime_assert(this->outputs.size() == 3,
                   "[Fully Connected Gradient Op] outputs.size() == 3.");
    
    const auto x = this->inputs[InputSchema::x];
    const auto w = this->inputs[InputSchema::w];
    const auto dy = this->inputs[InputSchema::dy];
    auto dw = this->outputs[OutputSchema::dw];
    auto db = this->outputs[OutputSchema::db];
    auto dx = this->outputs[OutputSchema::dx];
    int units;
    if(opio.param.HasParam("Units") &&
       dw->IsEmpty() &&
       db->IsEmpty() &&
       dx->IsEmpty() &&
       !dy->IsEmpty() &&
       !x->IsEmpty() &&
       x->Dims() == 2
       ){
        units = opio.param.GetParam<int>("Units");
        dw->template Resize<DT>(*w);
        db->template Resize<DT>({units});
        dx->template Resize<DT>(*x);
    }
    else{
        runtime_assert(x->Dims() == 2,
                       "[Fully Connected Gradient Op]x->Dims() == 2.");
        runtime_assert(x->Dim(1) == w->Dim(1),
                       "[Fully Connected Gradient Op] x->Dim(1) == w->Dim(1).");
        runtime_assert(dw->CompareSizeWith(*w),
                       "[Fully Connected Gradient Op] dw->CompareSizeWith(w).");
        runtime_assert(dx->CompareSizeWith(*x),
                       "[Fully Connected Gradient Op] dx->CompareSizeWith(x).");
    }
    
    bias_multiplier.template Resize<DT, DC>({x->Dim(0)});
    bias_multiplier.template SetByConst<DT>(DT(1));
    
    /*
     * batch size.
     */
    m = x->Dim(0);
    /*
     * output size.
     */
    n = w->Dim(0);
    /*
     * total input's element size.
     */
    k = w->Dim(1);
}

template <class DT, class DC>
void FullyConnectedGradientOp<DT, DC>::Compute(){
    const auto x = this->inputs[InputSchema::x];
    const auto w = this->inputs[InputSchema::w];
    const auto dy = this->inputs[InputSchema::dy];
    auto dw = this->outputs[OutputSchema::dw];
    auto db = this->outputs[OutputSchema::db];
    auto dx = this->outputs[OutputSchema::dx];
    /*
     * db = dy.
     */
    math::gemv<DT, DC>(true, m, n, DT(1),
                                  dy->template GetPtrConst<DT>(), n,
                                  bias_multiplier.template GetPtrConst<DT>(), DT(0),
                                  db->template GetPtrMutable<DT>(), n, nullptr);
    
    /*
     * Calculate gradients of weights.
     * dy(batch_size x output_size)^T * x(batch_size x input_size)
     *  = dw(output_size x input_size)
     */
    math::gemm<DT, DC>(true, false,
                                  n, k, m,
                                  DT(1), dy->template GetPtrConst<DT>(), n,
                                  x->template GetPtrConst<DT>(), k,
                                  DT(0), dw->template GetPtrMutable<DT>(), k, nullptr);
    
    /*
     * Calculate loss to propagate through bottom.
     * dy(batch_size x output_size) * w(output_size x input_size)
     *  = dx(batch_size x input_size)
     */
    math::gemm<DT, DC>(
                                  false, false,
                                  m, k, n,
                                  DT(1), dy->template GetPtrConst<DT>(), n,
                                  w->template GetPtrConst<DT>(), k,
                                  DT(0), dx->template GetPtrMutable<DT>(), k, nullptr);
    
    math::scal<DT, DC>(
                                  db->Size(),
                                  DT(1) / static_cast<DT>(x->Dim(0)),
                                  db->template GetPtrConst<DT>(),
                                  db->template GetPtrMutable<DT>()
                                  );
    
    math::scal<DT, DC>(
                                  dw->Size(),
                                  DT(1) / static_cast<DT>(x->Dim(0)),
                                  dw->template GetPtrConst<DT>(),
                                  dw->template GetPtrMutable<DT>()
                                  );
}

REGIST_OPERATOR_CPU(FC_float_Gradient, FullyConnectedGradientOp<float, CPUContext>)
REGIST_OPERATOR_CPU(FC_double_Gradient, FullyConnectedGradientOp<double, CPUContext>)

struct FCGradientIO : public GradientIO{
    OperatorIO GetGradientIO(OperatorIO opio) override{
        OperatorIO opio_grad;
        opio_grad.type = opio.type + "_" + opio.data_type + "_Gradient";
        opio_grad.data_type = opio.data_type;
        opio_grad.inputs.push_back(opio.inputs[0]);
        opio_grad.inputs.push_back(opio.inputs[1]);
        opio_grad.inputs.push_back(opio.outputs[0] + "_grad");
        opio_grad.outputs.push_back(opio.inputs[1] + "_grad");
        opio_grad.outputs.push_back(opio.inputs[2] + "_grad");
        opio_grad.outputs.push_back(opio.inputs[0] + "_grad");
        opio_grad.param = opio.param;
        
        return opio_grad;
    }
};

REGIST_OPERATOR_GRADIENT_IO(FC, FCGradientIO);

} /* namespace mlfe */
