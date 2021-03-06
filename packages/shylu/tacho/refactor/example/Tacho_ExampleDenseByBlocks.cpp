#include "Teuchos_CommandLineProcessor.hpp"

#include "ShyLUTacho_config.h"

#include <Kokkos_Core.hpp>
#include <impl/Kokkos_Timer.hpp>

#include "TachoExp_Util.hpp"
#include "TachoExp_DenseMatrixView.hpp"
#include "TachoExp_DenseFlopCount.hpp"

#include "TachoExp_Chol_ByBlocks.hpp"
#include "TachoExp_Gemm_ByBlocks.hpp"
#include "TachoExp_Herk_ByBlocks.hpp"
#include "TachoExp_Trsm_ByBlocks.hpp"

#ifdef HAVE_SHYLUTACHO_MKL
#include "mkl_service.h"
#endif

using namespace Tacho;
using namespace Tacho::Experimental;

int main (int argc, char *argv[]) {
  Teuchos::CommandLineProcessor clp;
  clp.setDocString("This example program measure the performance of dense-by-blocks algorithms on Kokkos::OpenMP execution space.\n");

  bool serial = false;
  clp.setOption("enable-serial", "disable-verbose", &serial, "Flag for invoking serial algorithm");

  int nthreads = 1;
  clp.setOption("kokkos-threads", &nthreads, "Number of threads");

  bool verbose = true;
  clp.setOption("enable-verbose", "disable-verbose", &verbose, "Flag for verbose printing");

  int mbeg = 1000;
  clp.setOption("begin", &mbeg, "Test problem begin size");

  int mend = 6000;
  clp.setOption("end", &mend, "Test problem end size");

  int step = 1000;
  clp.setOption("step", &step, "Test problem step size");

  int mb = 128;
  clp.setOption("mb", &mb, "Blocksize");

  clp.recogniseAllOptions(true);
  clp.throwExceptions(false);

  Teuchos::CommandLineProcessor::EParseCommandLineReturn r_parse= clp.parse( argc, argv );

  if (r_parse == Teuchos::CommandLineProcessor::PARSE_HELP_PRINTED) return 0;
  if (r_parse != Teuchos::CommandLineProcessor::PARSE_SUCCESSFUL  ) return -1;

  Kokkos::initialize(argc, argv);
  if (std::is_same<Kokkos::DefaultHostExecutionSpace,Kokkos::Serial>::value) 
    std::cout << "Kokkos::Serial\n";
  else
    Kokkos::DefaultHostExecutionSpace::print_configuration(std::cout, false);
  
  int r_val = 0;
  
  {
    typedef double value_type;
    typedef Kokkos::DefaultHostExecutionSpace exec_space;
    typedef DenseMatrixView<value_type,exec_space>          DenseMatrixViewType;
    typedef DenseMatrixView<DenseMatrixViewType,exec_space> DenseMatrixOfBlocksType;
    typedef Kokkos::pair<ordinal_type,ordinal_type> range_type;

    Kokkos::Impl::Timer timer;

    typedef Kokkos::TaskScheduler<exec_space> sched_type;
    sched_type sched;

    typedef TaskFunctor_Chol<sched_type,DenseMatrixOfBlocksType,
      Uplo::Upper,Algo::ByBlocks> task_functor_chol;
    typedef TaskFunctor_Trsm<sched_type,double,DenseMatrixOfBlocksType,
      Side::Left,Uplo::Upper,Trans::ConjTranspose,Diag::NonUnit,Algo::ByBlocks> task_functor_trsm;
    typedef TaskFunctor_Gemm<sched_type,double,DenseMatrixOfBlocksType,
      Trans::NoTranspose,Trans::NoTranspose,Algo::ByBlocks> task_functor_gemm;

    const ordinal_type max_functor_size = 4*sizeof(task_functor_gemm);
    
    Kokkos::View<value_type*,exec_space> 
      a("a", mend*mend), a1("a1", mend*mend), a2("a2", mend*mend), 
      b("b", mend*mend);
    
    const ordinal_type bmend = (mend/mb) + 1;
    Kokkos::View<DenseMatrixViewType*,exec_space> 
      ha("ha", bmend*bmend), hb("hb", bmend*bmend), hc("hc", bmend*bmend);

    {
      const ordinal_type
        task_queue_capacity = bmend*bmend*bmend*max_functor_size,
        min_block_size  = 16,
        max_block_size  = 4*max_functor_size,
        num_superblock  = 4,
        superblock_size = task_queue_capacity/num_superblock;
      
      sched = sched_type(typename exec_space::memory_space(),
                         task_queue_capacity,
                         min_block_size,
                         max_block_size,
                         superblock_size);
    }

    const ordinal_type dry = -2, niter = 3;

    double t_reference = 0, t_byblocks = 0;

    Random<value_type> random;
    auto randomize = [&](const DenseMatrixViewType &mat) {
      const ordinal_type m = mat.dimension_0(), n = mat.dimension_1();
      for (ordinal_type j=0;j<n;++j)
        for (ordinal_type i=0;i<m;++i)
          mat(i,j) = random.value();
    };

    ///
    /// Chol
    ///
    for (ordinal_type m=mbeg;m<=mend;m+=step) {
      t_reference = 0; t_byblocks = 0;
      auto sub_a  = Kokkos::subview(a,  range_type(0,m*m));
      auto sub_a1 = Kokkos::subview(a1, range_type(0,m*m));
      auto sub_a2 = Kokkos::subview(a2, range_type(0,m*m));
      
      {
        Kokkos::deep_copy(sub_a, 0);

        DenseMatrixViewType A;
        A.set_view(m, m);
        A.attach_buffer(1, m, a.data());
        for (ordinal_type i=0;i<m;++i) {
          A(i,i) = 4;
          const ordinal_type ip = i+1;
          if (ip < m) {
            A(ip,i ) = 1;
            A(i ,ip) = 1;
          }
        }
      }

      // reference 
      {
        int dummy = 0;
        
        DenseMatrixViewType A;
        A.set_view(m, m);
        A.attach_buffer(1, m, a1.data());

        for (ordinal_type iter=dry;iter<niter;++iter) {
          Kokkos::deep_copy(sub_a1, sub_a);
          timer.reset();
          Chol<Uplo::Upper,Algo::External>::invoke(dummy, dummy, A);
          t_reference += (iter >= 0)*timer.seconds();
        }
        t_reference /= niter;
      }
      
      // dense by blocks
      {
        DenseMatrixViewType A;
        A.set_view(m, m);
        A.attach_buffer(1, m, a2.data());

        DenseMatrixOfBlocksType HA;
        const ordinal_type bm = (m/mb) + (m%mb>0);
        HA.set_view(bm, bm);
        HA.attach_buffer(1, bm, ha.data());
        {          
          for (ordinal_type iter=dry;iter<niter;++iter) {
            Kokkos::deep_copy(sub_a2, sub_a);
            
            timer.reset();
            setMatrixOfBlocks(HA, m, m, mb);
            attachBaseBuffer(HA, A.data(), A.stride_0(), A.stride_1());

            Kokkos::host_spawn(Kokkos::TaskTeam(sched, Kokkos::TaskPriority::High),
                               task_functor_chol(sched, HA));
            Kokkos::wait(sched);
            t_byblocks += (iter >=0)*timer.seconds();

            clearFutureOfBlocks(HA);
          }
          t_byblocks /= niter;
        }
      }

      {
        double diff = 0.0, norm = 0.0;
        for (ordinal_type p=0;p<(m*m);++p) {
          norm += a1(p)*a1(p);
          diff += (a1(p) - a2(p))*(a1(p) - a2(p));
        }
        const double relerr = sqrt(diff/norm), eps = std::numeric_limits<double>::epsilon()*1000;

        if (sqrt(diff/norm) > eps) {
          printf("******* chol problem %d fails, reltaive error against reference is %10.4f\n", 
                 m, relerr);
          r_val = -1;
          break;
        }
      }
      
      {
        const double kilo = 1024, gflop = DenseFlopCount<value_type>::Chol(m)/kilo/kilo/kilo;
        printf("chol problem %10d, gflop %10.2f, gflop/s :: reference %10.2f, byblocks %10.2f\n", 
               m, gflop, gflop/t_reference, gflop/t_byblocks);
      }
    }
    printf("\n\n");

    ///
    /// Trsm
    ///
    
    for (ordinal_type m=mbeg;m<=mend;m+=step) {
      t_reference = 0; t_byblocks = 0;
      auto sub_a  = Kokkos::subview(a,  range_type(0,m*m));
      auto sub_a1 = Kokkos::subview(a1, range_type(0,m*m));
      auto sub_a2 = Kokkos::subview(a2, range_type(0,m*m));
      
      {
        DenseMatrixViewType A, B;
        A.set_view(m, m);
        A.attach_buffer(1, m, a.data());

        B.set_view(m, m);
        B.attach_buffer(1, m, a1.data());

        randomize(A);
        randomize(B);

        Kokkos::deep_copy(a2, a1);
      }

      // reference 
      {
        int dummy = 0;
        
        DenseMatrixViewType A, B;
        A.set_view(m, m);
        A.attach_buffer(1, m, a.data());

        B.set_view(m, m);
        B.attach_buffer(1, m, a1.data());
        
        const double alpha = -1.0;

        for (ordinal_type iter=dry;iter<niter;++iter) {
          timer.reset();
          Trsm<Side::Left,Uplo::Upper,Trans::ConjTranspose,Algo::External>
            ::invoke(dummy, dummy, Diag::NonUnit(), alpha, A, B);
          t_reference += (iter >= 0)*timer.seconds();
        }
        t_reference /= niter;
      }
      
      // dense by blocks
      {
        DenseMatrixViewType A, B;
        A.set_view(m, m);
        A.attach_buffer(1, m, a.data());

        B.set_view(m, m);
        B.attach_buffer(1, m, a2.data());

        DenseMatrixOfBlocksType HA, HB;
        const ordinal_type bm = (m/mb) + (m%mb>0);

        HA.set_view(bm, bm);
        HA.attach_buffer(1, bm, ha.data());

        HB.set_view(bm, bm);
        HB.attach_buffer(1, bm, hb.data());
        {
          const double alpha = -1.0;

          for (ordinal_type iter=dry;iter<niter;++iter) {
            timer.reset();
            setMatrixOfBlocks(HA, m, m, mb);
            attachBaseBuffer(HA, A.data(), A.stride_0(), A.stride_1());
            
            setMatrixOfBlocks(HB, m, m, mb);
            attachBaseBuffer(HB, B.data(), B.stride_0(), B.stride_1());

            Kokkos::host_spawn(Kokkos::TaskTeam(sched, Kokkos::TaskPriority::High),
                               task_functor_trsm(sched, alpha, HA, HB));
            Kokkos::wait(sched);
            t_byblocks += (iter >=0)*timer.seconds();

            clearFutureOfBlocks(HB);
          }
          t_byblocks /= niter;
        }
      }
      
      {
        double diff = 0.0, norm = 0.0;
        for (ordinal_type p=0;p<(m*m);++p) {
          norm += a1(p)*a1(p);
          diff += (a1(p) - a2(p))*(a1(p) - a2(p));
        }
        const double relerr = sqrt(diff/norm), eps = std::numeric_limits<double>::epsilon()*1000;

        if (sqrt(diff/norm) > eps) 
          printf("******* trsm problem %d fails, reltaive error against reference is %10.4f\n", 
                 m, relerr);
      }
      
      {
        const double kilo = 1024, gflop = DenseFlopCount<value_type>::Trsm(true, m, m)/kilo/kilo/kilo;
        printf("trsm problem %10d, gflop %10.2f, gflop/s :: reference %10.2f, byblocks %10.2f\n", 
               m, gflop, gflop/t_reference, gflop/t_byblocks);
      }
    }
    printf("\n\n");

    ///
    /// Gemm
    ///
    
    for (ordinal_type m=mbeg;m<=mend;m+=step) {
      t_reference = 0; t_byblocks = 0;
      auto sub_a  = Kokkos::subview(a,  range_type(0,m*m));
      auto sub_b  = Kokkos::subview(b,  range_type(0,m*m));
      auto sub_a1 = Kokkos::subview(a1, range_type(0,m*m));
      auto sub_a2 = Kokkos::subview(a2, range_type(0,m*m));

      {
        DenseMatrixViewType A, B, C;
        A.set_view(m, m);
        A.attach_buffer(1, m, a.data());

        B.set_view(m, m);
        B.attach_buffer(1, m, b.data());

        C.set_view(m, m);
        C.attach_buffer(1, m, a1.data());

        randomize(A);
        randomize(B);
        randomize(C);

        Kokkos::deep_copy(a2, a1);
      }

      // reference 
      {
        int dummy = 0;
        
        DenseMatrixViewType A, B, C;
        A.set_view(m, m);
        A.attach_buffer(1, m, a.data());

        B.set_view(m, m);
        B.attach_buffer(1, m, b.data());

        C.set_view(m, m);
        C.attach_buffer(1, m, a1.data());
        
        const double alpha = -1.0, beta = 1.0;

        for (ordinal_type iter=dry;iter<niter;++iter) {
          timer.reset();
          Gemm<Trans::NoTranspose,Trans::NoTranspose,Algo::External>
            ::invoke(dummy, dummy, alpha, A, B, beta, C);
          t_reference += (iter >= 0)*timer.seconds();
        }
        t_reference /= niter;
      }
      
      // dense by blocks
      {
        DenseMatrixViewType A, B, C;
        A.set_view(m, m);
        A.attach_buffer(1, m, a.data());

        B.set_view(m, m);
        B.attach_buffer(1, m, b.data());

        C.set_view(m, m);
        C.attach_buffer(1, m, a2.data());

        DenseMatrixOfBlocksType HA, HB, HC;
        const ordinal_type bm = (m/mb) + (m%mb>0);

        HA.set_view(bm, bm);
        HA.attach_buffer(1, bm, ha.data());

        HB.set_view(bm, bm);
        HB.attach_buffer(1, bm, hb.data());

        HC.set_view(bm, bm);
        HC.attach_buffer(1, bm, hc.data());
        {
          const double alpha = -1.0, beta = 1.0;

          for (ordinal_type iter=dry;iter<niter;++iter) {
            timer.reset();

            setMatrixOfBlocks(HA, m, m, mb);
            attachBaseBuffer(HA, A.data(), A.stride_0(), A.stride_1());
            
            setMatrixOfBlocks(HB, m, m, mb);
            attachBaseBuffer(HB, B.data(), B.stride_0(), B.stride_1());
            
            setMatrixOfBlocks(HC, m, m, mb);
            attachBaseBuffer(HC, C.data(), C.stride_0(), C.stride_1());

            Kokkos::host_spawn(Kokkos::TaskTeam(sched, Kokkos::TaskPriority::High),
                               task_functor_gemm(sched, alpha, HA, HB, beta, HC));
            Kokkos::wait(sched);
            t_byblocks += (iter >=0)*timer.seconds();

            clearFutureOfBlocks(HC);
          }
          t_byblocks /= niter;
        }
      }
      
      {
        double diff = 0.0, norm = 0.0;
        for (ordinal_type p=0;p<(m*m);++p) {
          norm += a1(p)*a1(p);
          diff += (a1(p) - a2(p))*(a1(p) - a2(p));
        }
        const double relerr = sqrt(diff/norm), eps = std::numeric_limits<double>::epsilon()*1000;

        if (sqrt(diff/norm) > eps) 
          printf("******* gemm problem %d fails, reltaive error against reference is %10.4f\n", 
                 m, relerr);
      }
      
      {
        const double kilo = 1024, gflop = DenseFlopCount<value_type>::Gemm(m, m, m)/kilo/kilo/kilo;
        printf("gemm problem %10d, gflop %10.2f, gflop/s :: reference %10.2f, byblocks %10.2f\n", 
               m, gflop, gflop/t_reference, gflop/t_byblocks);
      }
    }
    printf("\n\n");
  }
  Kokkos::finalize();

  return r_val;
}
