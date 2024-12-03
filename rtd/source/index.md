# User Guide for MISO

This guide explains in a step-by-step approach the recommended way to install the MISO software library.

## Windows Subsystem for Linux (WSL)

A Linux OS must be used to access the provided tools. If you are already running on a Linux OS (Ubuntu, Fedora, etc.), skip this step, and continue with installing OpenMPI and System Packages. If you are running on a Windows OS, one available option to access this content is by using a Linux simulator, for example, WSL. Note that the code blocks in this guide assume that Ubuntu is being used, so certain steps may be different on other platforms.

To install WSL, enter the terminal (on Windows, you can use "windows_key + r" and type in "cmd" to the prompt to open the terminal) and enter the following line of code:

```
wsl --install
```

Accept all permissions on the popups and allow WSL to be installed. Once finished, you will be asked to enter a username and password. This will be your Linux administrator account, and the password will be needed to be used for all "sudo" commands to install required system packages.

Start up a new Ubuntu (Linux) terminal by selecting the dropdown arrow and choosing "Ubuntu". The default directory is "/home/[username]". From this point, use the command:

```
mkdir motor
```

to create a folder for all downloads. Below is a list of basic Linux commands to navigate between folders:

```
explorer.exe .   # open file explorer
ls               # display folders and files within current directory
cd [folder]      # enter a folder within the current directory
cd ..            # exit current directory to its container folder
cd               # navigate to /home/[username]
cd /             # navigate to /
```

## OpenMPI and System Packages

OpenMPI is a software package that is an implementation of the Message Passing Interface (MPI) standard that is required for a parallel build of MFEM. Alternatives exist, such as MPICH, but OpenMPI is the recommended MPI implementation. MPI is required for MISO.

In addition to MISO, it is highly recommended to install gcc, which is a collection of compilers, and cmake, which is a build system generator. Alternatives exist for both of these installations, but this guide will use these choices. git and build-essential are mandatory packages, and while build-essential may be installed by default, you should confirm its presence before proceeding.

To download these packages, you must be on an administrator account. Use the following lines of code in the terminal to download the packages:

```
sudo apt install gcc
sudo apt install cmake
sudo apt install git
sudo apt install build-essential
sudo apt install openmpi-bin openmpi-doc libopenmpi-dev
```

For OpenMPI (or any MPI package) to be used, it must be able to be found on the PC's path. To confirm that OpenMPI has been successfully installed and can be used, enter the terminal and use the following lines of code:

```
which mpicc
which mpicxx
```

After each command, a directory should be listed that holds these particular executables. If these files can not be found, then they need to be added to your path. Enter the file explorer and search for "mpicc". The file, along with mpicxx and others, should be in a "bin" directory". Add this directory as a Path variable by using the following code:

```
export PATH=$PATH:[path_to_mpi_container_directory]
```

This command MUST be repeated whenever a new command prompt is opened, as an export statement is not permanent. Every export command throughout this process must be repeated when that particular installation needs to be used.

## MFEM Prerequisites

Two dependencies are required in order to build the MFEM library, used for finite element discretization: HYPRE and METIS.

### HYPRE

To download the HYPRE package, follow the below steps:
1. Go to [HYPRE GitHub](https://github.com/hypre-space/hypre)
2. Download version 2.28.0 (this is NOT the current version; go to the "Releases" section to see past versions)
3. Unzip the folder and extract it to [path_to_motor_folder]/Dependencies
      Note that the default browser download is to Windows. Drag and drop the extracted files in file explorer to copy them from Windows over to Linux
4. Additional information can be found in the README.md and INSTALL.md files for configuration and installation

Enter the terminal and navigate to the motor folder. Use the following lines of code to configure and install HYPRE.

```
cd Dependencies/hypre-2.28.0/src./
./configure
    # If this comes back as an error, use the following line of code then repeat ./configure
    chmod +x configure
cd cmbuild
cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DHYPRE_INSTALL_PREFIX="[path_to_motor_folder]/Installations/hypre" -DHYPRE_ENABLE_SHARED=ON
make install
cd ../../../..
export PATH=$PATH:[path_to_motor_folder]/Installations/hypre
```

Note that EVERY "export" statement must be repeated upon each PC reboot, if MISO needs to be used.

### METIS

The official METIS website can be unreliable, so its recommended to use the GitHub page linked from MFEM that has some relevant versions for using with MFEM.

To download the METIS package, follow the below steps:
1. Go to [Metis-MFEM GitHub](https://github.com/mfem/tpls)
2. Download the default package
3. Unzip the folder and enter it
4. Unzip the metis-5.1.0 folder and extract it to [path_to_motor_folder]/Dependencies

Enter the terminal and navigate to the motor folder. Use the following lines of code to configure and install METIS.

```
cd Dependencies/metis-5.1.0
make config shared=1 cc=gcc prefix=[path_to_motor_folder]/Installations/metis
make -j 4
make install
cd ../..
export PATH=$PATH:[path_to_motor_folder]/Installations/metis
```

## PUMI (additional mesh tool)

PUMI is a library that is optional for standalone MFEM, but is required for certain MISO tests. This project uses the CORE package, which is a modification of PUMI and serves the same purpose. EngineeringSketchPad (ESP) is a dependency for CORE, and OpenCASCADE is in turn a requirement for ESP.

### OpenCASCADE

[Specific folder, check for alternatives]
[Figure out how to attach file to RTD]
[Alternative: possibility of downloading directly from site?]

### ESP

Enter the terminal and navigate to the motor folder. Use the following lines of code to download, configure, and install ESP.

```
cd Dependencies
git clone https://github.com/tuckerbabcock/EngSketchPad.git
cd EngSketchPad
git checkout 8a5ca2bd6fbed747d371eb926e8a24d0cfcda087
cd config
./makeEnv /[path_to_motor_folder]/Installations/OpenCASCADE
cd ..
source ESPenv.sh
cd src
make
cd ../../..
```

In addition to every "export" statement, the following command must be repeated upon each PC reboot, if MISO needs to be used:

```
source [path_to_motor_folder]/Dependencies/EngSketchPad/ESPenv.sh
```

### CORE (PUMI)

A configuration file is highly recommended to be used with cmake to more specifically configure the build of CORE. This file can be made using the "cat" command in the terminal, or made using a text editor. The "cat" command can be used in the following way:

```
cat >> config_core.sh
# Write line-by-line code for the file
# Use CTRL-Z to stop command
```

The below file name and code block are recommended for this configuration file.

#### config_core.sh

```
cmake .. \
  -DCMAKE_C_COMPILER="MPICC" \
  -DCMAKE_CXX_COMPILER="MPICXX" \
  -DCMAKE_INSTALL_PREFIX="/[path_to_motor_folder]/Installations/core" \
  -DBUILD_SHARED_LIBS=ON \
  -DSCOREC_CXX_OPTIMIZE=ON \
  -DSCOREC_CXX_SYMBOLS=ON \
  -DSCOREC_CXX_WARNINGS=OFF \
  -DSCOREC_EXTRA_CXX_FLAGS="-Wextra -Wall" \
  -DENABLE_ZOLTAN=OFF \
  -DENABLE_EGADS=ON \
  -DPUMI_USE_EGADSLITE=OFF \
  -DIS_TESTING=OFF
```

#### Terminal Code

```
cd Dependencies
git clone https://github.com/tuckerbabcock/core.git
cd core
git checkout egads-dev
mkdir build
cd build
# make config_core.sh (see above) or move file into build folder
source config_core.sh
make -j 4
make install
cd ../../..
export PATH=$PATH:/[path_to_motor_folder]/Installations/core
```

## Building MFEM

Once HYPRE, METIS, and CORE have been successfully installed, MFEM can now be built. For more information on the package, see the [MFEM Website](https://mfem.org/building/). You can also go to their [GitHub](https://github.com/mfem/mfem) and inspect the README.md and INSTALL.md files.

A configuration file is highly recommended to be used with cmake to more specifically configure the build of MFEM.

#### config_mfem.sh

```
cmake .. \
  -DCMAKE_CXX_COMPILER="MPICXX" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX="/[path_to_motor_folder]/Installations/mfem" \
  -DBUILD_SHARED_LIBS=ON \
  -DMFEM_DEBUG=ON \
  -DMFEM_USE_MPI=ON \
  -DMFEM_USE_METIS=ON \
  -DMFEM_USE_METIS_5=ON \
  -DMFEM_USE_PUMI=ON \
  -DPUMI_DIR="/[path_to_motor_folder]/Installations/core" \
  -DMFEM_ENABLE_EXAMPLES=OFF \
  -DMFEM_ENABLE_MINIAPPS=OFF
```

#### Terminal Code

```
cd Dependencies
git clone https://github.com/mfem/mfem.git
cd mfem
git checkout odl
mkdir build
cd build
# make config_mfem.sh (see above) or move file into build folder
source config_mfem.sh
make -j 4
make install
make examples -j 4
make test
cd ../../..
export PATH=$PATH:/[path_to_motor_folder]/Installations/mfem
```

It is not required to run the "make examples" and "make test" commands. If you decide to run the tests, a number of them should fail, as they are only built when the "MFEM_ENABLE_MINIAPPS" option is enabled. These modules are not used by MISO and lengthen the installation process, so it is not recommended to build them.

## MISO

MISO is the main library used to make finite element simulations, and is based on MFEM. One additional dependency, ADEPT, used for automatic differentiation, is required before the build process can begin.

### ADEPT

To download the ADEPT package, follow the below steps:
1. Go to [ADEPT Website](https://www.met.reading.ac.uk/clouds/adept/download.html)
2. Download version 2.1 (this is NOT the most recent version; version 2.1.1 may be compatible but has not been tested)
3. Unzip the folder and extract it to [path_to_motor_folder]/Dependencies

Enter the terminal and navigate to the motor folder. Use the following lines of code to configure and install ADEPT.

```
cd Dependencies/adept
./configure --prefix="/[path_to_motor_folder]/Installations/adept
make
make install
cd ../..
export PATH=$PATH:/[path_to_motor_folder]/Installations/adept
```

### Building MISO

Once MFEM and ADEPT have been successfully installed, MISO can now be built. For more information on the package, go to the [GitHub](https://github.com/OptimalDesignLab/MISO) and inspect the README.md file.

A configuration file is highly recommended to be used with cmake to more specifically configure the build of MISO.

#### config_miso.sh

```
cmake .. \
  -DCMAKE_C_COMPILER="MPICC" \
  -DCMAKE_CXX_COMPILER="MPICXX" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMFEM_DIR="/[path_to_motor_folder]/Dependencies/mfem" \
  -DAdept_ROOT="/[path_to_motor_folder]/Installations/adept" \
  -DPUMI_DIR="/[path_to_motor_folder]/Installations/core" \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

#### Terminal Code

```
git clone https://github.com/OptimalDesignLab/MISO.git
cd MISO
git checkout dev
mkdir build
cd build
# make config_miso.sh (see above) or move file into build folder
source config_miso.sh
make -j 4
make install
make tests
cd ../..
```

The "make tests" command is expected to take a long time to complete. If any of the 57 tests results in a failure, it is most likely a problem with a dependency. It is also possible that you are running an outdated version of MISO, which may be rectified by navigating to the MISO folder and running the "git submodule update" command

At this point, MISO has been fully installed and may be used for projects.
