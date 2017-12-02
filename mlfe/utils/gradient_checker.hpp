#ifndef __GRADIENT_CHECKER_HPP__
#define __GRADIENT_CHECKER_HPP__
#include <memory>
#include <algorithm>
#include "../operators/operator.hpp"
#include "../device_context/cpu_context.hpp"
#include "../math/blas.hpp"

namespace mlfe{

template <class DataType, class DeviceContext>
class GradientChecker {
public:
    explicit GradientChecker(DataType _e){
        e = _e;
    }
    
    std::shared_ptr<TensorBlob<DeviceContext> > Run(
                                                    std::shared_ptr<Operator<DeviceContext> > op,
                                                    std::shared_ptr<TensorBlob<DeviceContext> > theta,
                                                    std::shared_ptr<TensorBlob<DeviceContext> > y,
                                                    std::shared_ptr<TensorBlob<DeviceContext> > analytical_gradient,
                                                    DataType scaler
                                                    ){
        std::shared_ptr<TensorBlob<DeviceContext> > numerical_gradient;
        std::shared_ptr<TensorBlob<DeviceContext> > gradient_checker;
        
        numerical_gradient = std::make_shared<TensorBlob<DeviceContext> >();
        gradient_checker = std::make_shared<TensorBlob<DeviceContext> >();
        numerical_gradient->template ReshapeLike<DataType>(y);
        gradient_checker->template ReshapeLike<DataType>(theta);
        
        for(int n = 0; n < theta->Size(); ++n){
            DataType val = theta->template GetPtrMutable<DataType>()[n];
            theta->template GetPtrMutable<DataType>()[n] = val + e;
            op->Compute();
            std::copy(y->template GetPtrConst<DataType>(),
                      y->template GetPtrConst<DataType>() + y->Size(),
                      numerical_gradient->template GetPtrMutable<DataType>());
            theta->template GetPtrMutable<DataType>()[n] = val - e;
            op->Compute();
            math::axpy<DataType, DeviceContext>(
                                                y->Size(),
                                                DataType(-1),
                                                y->template GetPtrConst<DataType>(),
                                                numerical_gradient->template GetPtrMutable<DataType>()
                                                );
            math::scal<DataType, DeviceContext>(
                                                numerical_gradient->Size(),
                                                DataType(1) / (DataType(2) * e),
                                                numerical_gradient->template GetPtrConst<DataType>(),
                                                numerical_gradient->template GetPtrMutable<DataType>()
                                                );
            
            DataType sum = std::accumulate(
                                           numerical_gradient->template GetPtrMutable<DataType>(),
                                           numerical_gradient->template GetPtrMutable<DataType>() + numerical_gradient->Size(),
                                           DataType(0)
                                           ) / scaler;
            
            gradient_checker->template GetPtrMutable<DataType>()[n] = analytical_gradient->template GetPtrConst<DataType>()[n] - sum;
            theta->template GetPtrMutable<DataType>()[n] = val;
        }
        return gradient_checker;
    }
    
private:
    DataType e;
};

} /* namespace mlfe */
#endif /* __GRADIENT_CHECKER_HPP__ */