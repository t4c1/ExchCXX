#include "libxc_common.hpp"
#include <exchcxx/impl/builtin/util.hpp>
#include <exchcxx/util/unused.hpp>
#include <exchcxx/exceptions/exchcxx_exception.hpp>
//#include <functionals.cuh>



void recv_from_device( void* dest, const void* src, const size_t len, 
  cl::sycl::queue* queue ) {

  try {
    queue->memcpy( dest, src, len );
  } catch( cl::sycl::exception const &ex ) { 
    throw( std::runtime_error( "recv failed + " + std::string(ex.what()) ) ); 
  }

}


void send_to_device( void* dest, const void* src, const size_t len, 
  cl::sycl::queue* queue ) {

  try {
    queue->memcpy( dest, src, len );
  } catch( cl::sycl::exception const &ex ) { 
    throw( std::runtime_error( "send failed + " + std::string(ex.what()) ) ); 
  }

}

void queue_sync( cl::sycl::queue* queue ) {

  queue->wait_and_throw();

}


template <typename T>
using sycl_host_allocator = cl::sycl::usm_allocator<T, cl::sycl::usm::alloc::host>;

template <typename T>
using sycl_host_vector = std::vector<T, sycl_host_allocator<T>>;

namespace ExchCXX {

namespace detail {

// LDA interfaces
LDA_EXC_GENERATOR_DEVICE( LibxcKernelImpl::eval_exc_device_ ) const {

  throw_if_uninitialized();
  EXCHCXX_BOOL_CHECK("KERNEL IS NOT LDA",  is_lda() );

  size_t sz_rho = this->rho_buffer_len(N);
  size_t sz_exc = this->exc_buffer_len(N);

  size_t len_rho = sz_rho*sizeof(double);
  size_t len_eps = sz_exc*sizeof(double);

  std::vector<double> rho_host( sz_rho ), eps_host( sz_exc );

  recv_from_device( rho_host.data(), rho, len_rho, queue );

  queue_sync( queue );
  xc_lda_exc( &kernel_, N, rho_host.data(), eps_host.data() );

  send_to_device( eps, eps_host.data(), len_eps, queue );
  queue_sync( queue ); // Lifetime of host vectors

}


LDA_EXC_VXC_GENERATOR_DEVICE( LibxcKernelImpl::eval_exc_vxc_device_ ) const {

  throw_if_uninitialized();
  EXCHCXX_BOOL_CHECK("KERNEL IS NOT LDA",  is_lda() );

  size_t sz_rho = this->rho_buffer_len(N);
  size_t sz_exc = this->exc_buffer_len(N);
  size_t sz_vxc = this->vrho_buffer_len(N);

  size_t len_rho = sz_rho*sizeof(double);
  size_t len_eps = sz_exc*sizeof(double);
  size_t len_vxc = sz_vxc*sizeof(double);

  std::vector<double> rho_host( sz_rho ), eps_host( sz_exc ), vxc_host( sz_vxc );

  recv_from_device( rho_host.data(), rho, len_rho, queue );

  queue_sync( queue );
  xc_lda_exc_vxc( &kernel_, N, rho_host.data(), eps_host.data(), vxc_host.data() );

  send_to_device( eps, eps_host.data(), len_eps, queue );
  send_to_device( vxc, vxc_host.data(), len_vxc, queue );
  queue_sync( queue ); // Lifetime of host vectors

}

// TODO: LDA kxc interfaces

// GGA interface
GGA_EXC_GENERATOR_DEVICE( LibxcKernelImpl::eval_exc_device_ ) const {

  throw_if_uninitialized();
  EXCHCXX_BOOL_CHECK("KERNEL IS NOT GGA",  is_gga() );

  size_t sz_rho   = this->rho_buffer_len(N);
  size_t sz_sigma = this->sigma_buffer_len(N);
  size_t sz_eps   = this->exc_buffer_len(N);

  size_t len_rho   = sz_rho   *sizeof(double);
  size_t len_eps   = sz_eps   *sizeof(double);
  size_t len_sigma = sz_sigma *sizeof(double);

  std::vector<double> rho_host( sz_rho ), eps_host( sz_eps ), sigma_host( sz_sigma );

  recv_from_device( rho_host.data(),   rho,   len_rho  , queue );
  recv_from_device( sigma_host.data(), sigma, len_sigma, queue );

  queue_sync( queue );
  xc_gga_exc( &kernel_, N, rho_host.data(), sigma_host.data(), eps_host.data() );

  send_to_device( eps, eps_host.data(), len_eps, queue );
  queue_sync( queue ); // Lifetime of host vectors

}


GGA_EXC_VXC_GENERATOR_DEVICE( LibxcKernelImpl::eval_exc_vxc_device_ ) const {

  throw_if_uninitialized();
  EXCHCXX_BOOL_CHECK("KERNEL IS NOT GGA",  is_gga() );

  size_t sz_rho    = this->rho_buffer_len(N);
  size_t sz_sigma  = this->sigma_buffer_len(N);
  size_t sz_eps    = this->exc_buffer_len(N);
  size_t sz_vrho   = this->vrho_buffer_len(N);
  size_t sz_vsigma = this->vsigma_buffer_len(N);

  size_t len_rho    = sz_rho   *sizeof(double);
  size_t len_sigma  = sz_sigma *sizeof(double);
  size_t len_vrho   = sz_vrho  *sizeof(double);
  size_t len_vsigma = sz_vsigma*sizeof(double);
  size_t len_eps    = sz_eps   *sizeof(double);

  std::vector<double> rho_host( sz_rho ), eps_host( sz_eps ), 
    sigma_host( sz_sigma ), vrho_host( sz_vrho ), vsigma_host( sz_vsigma );

  recv_from_device( rho_host.data(),   rho,   len_rho  , queue );
  recv_from_device( sigma_host.data(), sigma, len_sigma, queue );
 
  queue_sync( queue );
  xc_gga_exc_vxc( &kernel_, N, rho_host.data(), sigma_host.data(), eps_host.data(), 
    vrho_host.data(), vsigma_host.data() );

  send_to_device( eps,    eps_host.data(),    len_eps   , queue);
  send_to_device( vrho,   vrho_host.data(),   len_vrho  , queue);
  send_to_device( vsigma, vsigma_host.data(), len_vsigma, queue);
  queue_sync( queue ); // Lifetime of host vectors

}

// TODO: GGA kxc interfaces  
  
  
// mGGA interface
MGGA_EXC_GENERATOR_DEVICE( LibxcKernelImpl::eval_exc_device_ ) const {

  throw_if_uninitialized();
  EXCHCXX_BOOL_CHECK("KERNEL IS NOT MGGA",  is_mgga() );

  size_t sz_rho   = this->rho_buffer_len(N)   ;
  size_t sz_sigma = this->sigma_buffer_len(N) ;
  size_t sz_lapl  = this->lapl_buffer_len(N)  ;
  size_t sz_tau   = this->tau_buffer_len(N)   ;
  size_t sz_eps   = this->exc_buffer_len(N)   ;

  size_t len_rho   = sz_rho  *sizeof(double);
  size_t len_sigma = sz_sigma*sizeof(double);
  size_t len_lapl  = sz_lapl *sizeof(double);
  size_t len_tau   = sz_tau  *sizeof(double);
  size_t len_eps   = sz_eps  *sizeof(double);

  std::vector<double> rho_host( sz_rho ), eps_host( sz_eps ), 
    sigma_host( sz_sigma ), lapl_host( sz_lapl ), 
    tau_host( sz_tau );

  recv_from_device( rho_host.data(),   rho,   len_rho  , queue );
  recv_from_device( sigma_host.data(), sigma, len_sigma, queue );
  recv_from_device( lapl_host.data(),  lapl,  len_lapl , queue );
  recv_from_device( tau_host.data(),   tau,   len_tau  , queue );

  queue_sync( queue );
  xc_mgga_exc( &kernel_, N, rho_host.data(), sigma_host.data(), lapl_host.data(), 
    tau_host.data(), eps_host.data() );

  send_to_device( eps, eps_host.data(), len_eps, queue );
  queue_sync( queue ); // Lifetime of host vectors

}


MGGA_EXC_VXC_GENERATOR_DEVICE( LibxcKernelImpl::eval_exc_vxc_device_ ) const {

  throw_if_uninitialized();
  EXCHCXX_BOOL_CHECK("KERNEL IS NOT MGGA",  is_mgga() );

  size_t sz_rho    = this->rho_buffer_len(N)   ;
  size_t sz_sigma  = this->sigma_buffer_len(N) ;
  size_t sz_lapl   = this->lapl_buffer_len(N)  ;
  size_t sz_tau    = this->tau_buffer_len(N)   ;
  size_t sz_eps    = this->exc_buffer_len(N)   ;
  size_t sz_vrho   = this->vrho_buffer_len(N)  ;
  size_t sz_vsigma = this->vsigma_buffer_len(N);
  size_t sz_vlapl  = this->vlapl_buffer_len(N) ;
  size_t sz_vtau   = this->vtau_buffer_len(N)  ;

  size_t len_rho    = sz_rho   *sizeof(double);
  size_t len_sigma  = sz_sigma *sizeof(double);
  size_t len_lapl   = sz_lapl  *sizeof(double);
  size_t len_tau    = sz_tau   *sizeof(double);
  size_t len_eps    = sz_eps   *sizeof(double);
  size_t len_vrho   = sz_vrho  *sizeof(double);
  size_t len_vsigma = sz_vsigma*sizeof(double);
  size_t len_vlapl  = sz_vlapl *sizeof(double);
  size_t len_vtau   = sz_vtau  *sizeof(double);

  std::vector<double> rho_host( sz_rho ), eps_host( sz_eps ), sigma_host( sz_sigma ), 
    lapl_host( sz_lapl ), tau_host( sz_tau );
  std::vector<double> vrho_host( sz_vrho ), vsigma_host( sz_vsigma ),  
    vlapl_host( sz_vlapl ), vtau_host( sz_vtau );

  recv_from_device( rho_host.data(),   rho,   len_rho  , queue );
  recv_from_device( sigma_host.data(), sigma, len_sigma, queue );
  recv_from_device( lapl_host.data(),  lapl,  len_lapl , queue );
  recv_from_device( tau_host.data(),   tau,   len_tau  , queue );

  queue_sync( queue );
  xc_mgga_exc_vxc( &kernel_, N, rho_host.data(), sigma_host.data(), 
    lapl_host.data(), tau_host.data(), eps_host.data(), vrho_host.data(), 
    vsigma_host.data(), vlapl_host.data(), vtau_host.data() );

  send_to_device( eps,    eps_host.data(), len_eps      , queue );
  send_to_device( vrho,   vrho_host.data(),   len_vrho  , queue );
  send_to_device( vsigma, vsigma_host.data(), len_vsigma, queue );
  send_to_device( vlapl,  vlapl_host.data(),  len_vlapl , queue );
  send_to_device( vtau,   vtau_host.data(),   len_vtau  , queue );
  queue_sync( queue ); // Lifetime of host vectors

}


UNUSED_DEVICE_INC_INTERFACE_GENERATOR( LDA, EXC, 
  LibxcKernelImpl::eval_exc_inc_device_, const )
UNUSED_DEVICE_INC_INTERFACE_GENERATOR( LDA, EXC_VXC, 
  LibxcKernelImpl::eval_exc_vxc_inc_device_, const )
UNUSED_DEVICE_INC_INTERFACE_GENERATOR( GGA, EXC, 
  LibxcKernelImpl::eval_exc_inc_device_, const )
UNUSED_DEVICE_INC_INTERFACE_GENERATOR( GGA, EXC_VXC, 
  LibxcKernelImpl::eval_exc_vxc_inc_device_, const )
UNUSED_DEVICE_INC_INTERFACE_GENERATOR( MGGA, EXC, 
  LibxcKernelImpl::eval_exc_inc_device_, const )
UNUSED_DEVICE_INC_INTERFACE_GENERATOR( MGGA, EXC_VXC, 
  LibxcKernelImpl::eval_exc_vxc_inc_device_, const )

}
}
