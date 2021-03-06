#include "one_hot.hpp"
#include "../device_context/cpu_context.hpp"
#include "../math/blas.hpp"
#include "../utils/assert.hpp"

namespace mlfe{

template <class DT, class DC>
OneHotOp<DT, DC>::OneHotOp(
                                      OperatorIO &opio,
                                      ItemHolder *ih
                                      ) : Operator<DC>(opio, ih) {
    runtime_assert(this->inputs.size() == 1,
                   "[OneHot Op] inputs.size() == 1.");
    runtime_assert(this->outputs.size() == 1,
                   "[OneHot Op] outputs.size() == 1.");
    const auto x = this->inputs[InputSchema::x];
    auto y = this->outputs[OutputSchema::y];
    
    if(opio.param.HasParam("Dim") &&
       y->IsEmpty() &&
       !x->IsEmpty()
       ){
        dim = opio.param.GetParam<int>("Dim");
        y->template Resize<DT>({x->Dim(0), dim});
    }
    else{
        runtime_assert(x->Dims() == y->Dims(),
                       "[OneHot Op] x->Dims() == y->Dims().");
    }
}

template <class DT, class DC>
void OneHotOp<DT, DC>::Compute(){
    const auto x = this->inputs[InputSchema::x];
    auto y = this->outputs[OutputSchema::y];
    math::scal<DT, DC>(
                                  y->Size(),
                                  static_cast<DT>(0),
                                  y->template GetPtrConst<DT>(),
                                  y->template GetPtrMutable<DT>()
                                  );
    for(int b = 0; b < x->Dim(0); ++b){
        int val = x->template GetPtrConst<DT>()[b];
        y->template GetPtrMutable<DT>()[b * dim + val] = static_cast<DT>(1);
    }
}

REGIST_OPERATOR_CPU(OneHot_float, OneHotOp<float, CPUContext>)
REGIST_OPERATOR_CPU(OneHot_double, OneHotOp<double, CPUContext>)

} /* namespace mlfe */
