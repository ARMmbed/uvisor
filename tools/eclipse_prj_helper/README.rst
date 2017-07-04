======================
Eclipse Project Helper
======================

This tool can be used for generating Eclipse + PyOCD Debug environment within one of uVisor examples

.. contents:: Table of Contents

Prerequisites
-------------
1. Eclipse
2. GNU ARM Eclipse plugins
  
  - Help -> Install New software -> Add ``http://gnuarmeclipse.sourceforge.net/updates``

3. PyOCD - ``sudo pip install -U pyocd``
4. MBED-Cli - ``sudo pip install -U mbed-cli mbed-ls``
5. GNU ARM Embedded toolchain

How To
------

1. Clone one of uVisor examples (``git@github.com:ARMmbed/mbed-os-example-uvisor.git``) to a ``<prj-root>``
#. Deploy MBED-OS::

    cd <prj-root>
    mbed deploy -vv

#. Instrument MBED-Os with uVisor sources::
    
    cd <prj-root>/mbed-os/features/FEATURE_UVISOR/importer
    make

#. Execute Eclipse project helper script::
    
    python <prj-root>/mbed-os/features/FEATURE_UVISOR/importer/TARGET_IGNORE/uvisor/tools/eclipse_prj_helper/generate_prj.py -w <prj-root>

#. Import Project to Eclipse: ``File -> Import -> General -> Existing Projects into Workspace``
#. Start Debugger: ``Run -> Debug Configurations -> GDB PyOCD Debugging -> PyOCD_K64F_<example-name>``

Troubleshooting
---------------
- Eclipse fails to start - make sure you have JRE/JDK installed - refer to Eclipse documentation
- Eclipse debug fails
 
  - ``pyocd-gdbserver --verson`` returns an error: 

    - Probably missing some x86 shared objects: ``sudo apt-get install libncurses5:i386``

  - Failure to connect to a device:
    
    - To verify execute following command while device is connected ``cd <example-root> && mbed detect``
    - Try previous step but with elevated permissions (sudo) - can be caused by USB mapping permissions issue
