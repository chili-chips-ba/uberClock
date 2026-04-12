VProc and mem_model Co-simulation Components
============================================

This page summarizes ``4.miniac/4.sim/models/cosim/README.md``.

Overview
--------

The VProc virtual processor and mem_model sparse memory model form the core of
the co-simulation environment for the Wireguard FPGA top-level test bench. The
``soc_cpu.VProc`` instance runs user code against the simulated design, while
the memory model provides shared sparse memory that can be accessed from both
software and HDL.

VProc
-----

VProc is a co-simulation component that allows a natively compiled user program
to run against an HDL processor component. It exposes C and C++ APIs for bus
transactions and for advancing simulation time.

The documented integration uses a local ``soc_if`` interface rather than a
standard interconnect such as AXI or Avalon. Multiple VProc instances can be
instantiated, each with its own node number.

The original README also references several architecture diagrams and API
illustrations:

- `VProc overview diagram <https://github.com/user-attachments/assets/b272d61c-fa97-4ba2-a609-307ed4d0840c>`_
- `VProc API illustration <https://github.com/user-attachments/assets/f8dd88fe-ed63-4a6c-a0eb-55a13d0f845d>`_
- `Simple VProc program illustration <https://github.com/user-attachments/assets/a5b03827-e882-42d5-8765-c9cc3128b31a>`_

mem_model
---------

The test bench also uses the
`mem_model <https://github.com/wyvernSemi/mem_model>`_ co-simulation component.
It provides a sparse C memory model together with HDL access components and a
software API.

The API exposes read and write functions over shared address spaces and allows
both HDL logic and multiple VProc instances to interact with the same memory.

The original README references additional diagrams:

- `mem_model API illustration <https://github.com/user-attachments/assets/06278e89-e718-4396-9a05-d8c9cbb51efa>`_
- `mem_model HDL integration diagram <https://github.com/user-attachments/assets/e991b49b-8b50-4e8f-bf7d-caa96992a680>`_

Integration Notes
-----------------

The directory contains HDL wrapper modules such as ``f_VProc.sv`` and
``mem_model.sv`` plus the supporting Verilog headers and prebuilt host
libraries. For native simulation code:

- Include ``VProcClass.h`` for the C++ VProc API or ``VUser.h`` for the C API.
- Include ``mem.h`` for the memory model API.
- When compiling as C++, wrap the C headers in ``extern "C"``.

Example:

.. code-block:: c++

   extern "C" {
   #include "VUser.h"
   #include "mem.h"
   }

More Information
----------------

- `VProc manual <https://github.com/wyvernSemi/vproc/blob/master/doc/VProc.pdf>`_
- `mem_model manual <https://github.com/wyvernSemi/mem_model/blob/main/doc/mem_model_manual.pdf>`_
