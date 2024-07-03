# `android_app` example


The `android_app` example is a simplified end-to-end APK-based application that runs concurrently two sessions that each offload work onto the DSP to perform tasks independently.

The `android_app` example demonstrates how to use a number of high-level features and how some of these may impact applications running concurrently. Refer to the [Feature Matrix](../../reference/feature_matrix.md) for example support and to know the DSP architecture on the target.

You can run walkthrough script with dry run (-DR) option to get the list of build, push and run commands for any specific target. Review the generic [setup](../README.md#setup) and [walkthrough_scripts](../README.md#walkthrough-scripts) instructions to learn more about setting up your device and using walkthrough scripts.

## Overview

The `android_app` APK packages all the required libraries needed to run on a number of Qualcomm devices.  The UI allows the user to run up to two sessions in parallel. These sessions emulate two applications that run independently.  Both of them are configurable and offload tasks onto the DSP.

This example also includes a standalone main and test Android executables that interface the same DSP shared libraries and access them from the command line instead of using the APK.

Use this project to learn more about the following:

* How to build an APK with CMake
* How to include selectively Release or Debug binary versions whether the Android build variant is Release or Debug
* How to create an APK that packages Hexagon shared objects that support different DSP versions
* How to allocate VTCM using the computer resource manager
* How to serialize resource requests and observe their impact on overall performance
* How to make FastRPC synchronous or asynchronous calls and observe their impact on overall performance
* How to unit test shared objects used in an APK
* How to share data structures across UI views, Java, and C code on the CPU and DSP
* How to create worker tasks on the CPU and DSP and manage their priorities
* How to load shared objects dynamically on the CPU and DSP
* How to determine the DSP version and check on the availability of specific symbols on target
* How to parallelize DSP and CPU tasks
* How to use callbacks on the DSP and CPU
* How to generate and display message logs from Java, native C/C++ running on the application processor and C/C++ running on the DSP

***Note: *** The android_app executable and APK work properly on targets with v68, v69, and v73 CDSPs.  However, the android_app unit tests currently assume a number of HVX units fixed to 4 per DSP, which causes some tests to fail on devices that do not meet this condition.


## Requirements

* Hexagon SDK 5.0.0.0 or higher with full NDK installed
* Android Studio
  * This example has been tested with Android Studio version 4.0 but is expected to work on other versions as well

### UI

#### Homescreen

![jpg](../../images/android_app_homescreen.jpg)

The main screen of the APK example allows the user to perform the following actions:

* Configure either session by pressing on the corresponding `SETTINGS` button
* Activate or deactivate either session by toggling the corresponding `ON/OFF` button
* Launch the sessions
* Terminate the sessions (if they have not terminated on their own already)
* Display statistics from the last run

#### Application settings

![jpg](../../images/android_app_settings.PNG)

The configuration screen is identical for both applications. The user can access it to specify the following options of the corresponding session:

* Whether DSP tasks should start periodically or as soon as the previous task has completed.
    * When tasks start periodically, they are required to complete within the period specified by the user.  When they don't, these tasks are aborted.
* How many DSP tasks should run before the application completes
* How many DSP worker tasks should be created in parallel to execute a DSP task
* How many cycles each DSP task should take
* How much VTCM memory each task should consume
* Whether VTCM requests should be serialized or not
* The priority of the threads running on the DSP
* Whether the FastRPC calls should be synchronous or asynchronous

#### Application statistics

![jpg](../../images/android_app_stats.jpg)

The statistics collected at the end of a run include the following:

* Average task per second: Number of tasks processed successfully per second
* Percentage DSP compute time: DSP processing time* over the entire test duration. The test duration includes the sleep time between invocations of periodical tasks.
* Percentage DSP acquire time: DSP time spent waiting on compute resource over DSP processing time*
* Number of tasks that:
    * Completed successfully
    * Were interrupted (when they were interrupted by a higher priority task requesting the compute resource)
    * Were aborted (when, for example, they did not complete before the next task had to be launched)
    * Failed (when, for example, they did not acquire the compute resource within the allocated time or some other resource failed)
* Task time CPU: Total time spent on the CPU thread launching the DSP tasks and waiting for their completions. This excludes the sleep time between successive invocations of periodical tasks.
* Total time DSP: DSP processing time
* Average FastRPC overhead per call

`*` DSP processing time is defined as the time spent on the DSP thread servicing a FastRPC call.

***Note: *** For a definition of all common metrics and terms relevant to this application, refer to the [definitions](#definitions) section.

### CPU workflow

The application processor runs up to two sessions in parallel. Each application runs on a separate software thread, each making independently FastRPC calls to invoke the DSP synchronously or asynchronously.  The application processor and DSP accumulate statistics such as processing time and number of tasks completed to make a [report](#application-statistics) available at the end of a test case.

The application processor uses a circular buffer for accumulating the input and output arrays, thus allowing an arbitrary number of tasks to be run.

The application processor also checks the correctness of all output buffers using a `verify` function.  This `verify` function follows the FastRPC call to the `run` method in the case of the synchronous implementation, and executes as part of the asynchronous function callback in the case of the asynchronous implementation.  In a larger application, this `verify` function could be instead a function that launches a postprocessing task. Instead of simply verifying the correctness of the output buffer, the application processor could save that buffer to memory and launch a post-processing task in parallel thus allowing the DSP to proceed immediately with running a new task while the previous output buffers are being post-processed.

### DSP workflow

Each FastRPC call results in executing one DSP task. The application processor executes the number of DSP tasks specified by the user in the [UI](#ui) of the application configuration.

The DSP worker pool library is used to create the number of worker tasks specified by the user in the same UI.

A pool of worker tasks is created for all workers to execute whenever they have completed their ongoing task and are free to process a new one.

All worker tasks are identical, designed to take a fixed amount of time--`WORKER_TASK_DURATION_US`--to complete assuming they are not interrupted.

The number of cycles each worker task takes is determined as follows:

* The DSP runs the following loop in assembly:

        .startLoop:
            { V0.b = VADD(V3.b,V3.b)}
            { V1.b = VADD(V4.b,V4.b)}:endloop0

* This loop is made of two packets containing one HVX instruction each. These instructions do not have interdependencies and do not stall.  Since each instruction takes two processor cycles (pcycles) to complete, each loop iteration is expected to consume ideally 4 pcycles on most targets.

* Up to four hardware threads can execute this code in parallel at the same throughput.

* The assembly `wait_pcycles` function consumes the input register R0 and divides it by four to determine the number of loop iterations.

* As a result, `wait_pcycles` waits for the number of processor cycles specified as its only input argument.

The number of worker tasks to execute is determined so that the DSP executes the number of cycles specified by the user in the UI.  Lahaina, for example, has four HVX units, which means up to four hardware DSP threads can execute the same tasks fully independently in parallel.  This means that for a fixed number of cycles to be executed in a DSP task, if only one application is running, using four software threads will result in completing a task four times faster than using one.

### Definitions

The following terms are used when referring to the `android_app` example.

|Name|Definition|
|--|--|
|session|CPU software thread that offloads one or more task to the DSP.|
|DSP task|Processing task executed by the DSP during each FastRPC call.  Executes one or more worker task.|
|Worker task|Task executed by a DSP worker thread.|
|Task period|Period at which a new DSP task is launched when `TaskTrigger.TIMED` is selected.  When DSP tasks are periodical, DSP tasks taking longer than the specified period are aborted and a new task is  launched.|
|Task trigger|Control for the way a session launches its DSP tasks. `TaskTrigger.TIMED` results in launching DSP tasks at fixed intervals while `TaskTrigger.QUEUED` results in launching a new DSP task as soon as the previous one has completed.|
|DSP processing time| Time spent on the DSP thread servicing a FastRPC call. This includes the time spent processing data as well as the time spent acquiring the processing resources.|
|Task interrution|Occurs when a task is interrupted by a higher priority task.|
|Task abortion|Occurs when a periodical task did not complete on time or was manually interrupted by the user.|
|Task failure|Occurs when a resource failure occurs, including a timeout on resource acquisition.|

## Setup

The `android_app` project example contains all the source code and build framework needed to create the DSP shared objects, test executables and APK.

The instructions below along with the `create_as_project.py` script allow to create an `android_app` Android Studio project, which can be used to rebuild the APK that is prepackaged as a binary:

* Use this [link](https://developer.android.com/studio/) to download and install the Android SDK.
* If you are new to Android Studio, familiarize yourself with existing [online tutorials](https://developer.android.com/training/basics/firstapp/creating-project) to learn how to create, build, and run projects
* Create a new Android ***Native C++*** project.
    Native development enables the Java application to interface C methods, which can then make FastRPC calls to offload workload on the Hexagon DSP.
* Select the following options
    * Name your project `android_app`
    * Select Java as the language
    * Use the default toolchain (on the next configuration page)
* Go to the scripts folder and run the [create_as_project.py](#create_as_projectpy) python script:

        cd scripts
        ./create_as_project.py

You should then be able to build the same APK that is provided as binary in this example.

## Building the code

The project relies on `CMake` to build the code. Two approaches are available to building the code.

### Building from Android Studio

The `android_app` APK is built from Android Studio by selecting the `Make Project` command from the `Build` tab.

The user can select the build variant of their choice: `Debug`, selected by default, or `Release`. Selecting`Release` results in using the ReleaseG target when building the Android and Hexagon binaries.

***Note: *** If the APK is built using the Release flavor, it will need to be [signed](https://developer.android.com/studio/publish/app-signing) before it can be installed on your device.

The build rules are defined under the app `build.gradle` file to trigger the following main actions:

* Build the code using `CMake` and the `CMakeLists.txt` file under `src/main/cpp`

        externalNativeBuild {
            cmake {
                path file('src/main/cpp/CMakeLists.txt')
                buildStagingDirectory "src/main/cpp/android_ReleaseG_aarch64"
                version "3.17.0"
            }
        }

    Variables that need to be passed onto the CMake command are set using the `arguments` property of the `cmake` configuration:

        arguments "-DANDROID_STL=c++_shared",
        "-DSYSTEM_TYPE=AS",
        "-DOS_TYPE=AS_HLOS",
        "-DHEXAGON_CMAKE_ROOT=${HEXAGON_SDK_ROOT}/build/cmake",
        "-DPREBUILT_LIB_DIR=android_aarch64",
        "-DHEXAGON_SDK_ROOT=${HEXAGON_SDK_ROOT}",
        "-DDSP_TYPE=3"

    The `SYSTEM_TYPE` variable allows CMake to determine when a build is triggered from Android Studio.  When this is the case, both the CPU and DSP binaries are being generated, unlike what happens when building Hexagon or CPU binaries from the command line.

* Copy the DSP shared objects and binary stubs to the `jniLibs` folder. For example:

        task copyVMKDOCS_DEF_DSP_ARCH_VERSIONFileToJniLibsRelease(type: Copy) {
            from file("../dsp/hexagon_ReleaseG_MKDOCS_HEX_TOOLS_VARIANT_vMKDOCS_DEF_DSP_ARCH_VERSION/libandroid_app_skel_vMKDOCS_DEF_DSP_ARCH_VERSION.so")
            into file("src/main/jniLibs/arm64-v8a")
        }

    This task copies the Release version of a vMKDOCS_DEF_DSP_ARCH_VERSION shared object into the `jniLibs/arm64-v8a` folder.  All binaries placed in this folder will be packaged into the APK.

* Build the APK, which will be placed under `app/build/outputs/apk`

The user then simply needs to install the resulting APK on a Qualcomm device to access the `android_app` application from the device. For example:

    adb install app/build/outputs/apk/debug/app-debug.apk

***Notes:*** Building the APK from Android Studio leads to building all the binaries that can also be generated separately from the [command line](#building-from-the-command-line).  Android binaries are present under `app/src/main/cpp/android_<build_flavor>_aarch64` while Hexagon binaries are present under `dsp/hexagon_<build_flavor>_<tool_version>_<dsp_arch>`.

### Building from the command line

The `android_app` APK can be built from the command line with the help of the `gradlew` daemon. This approach allows to independently build the DSP and CPU binaries outside of the Android Studio.

#### Build the APK

To build the APK, execute the following command from the `android_app` folder:

    gradlew build

Additionally, `gradlew` provides an extensive list of `tasks`, including `clean` to clean the `android_app` project from the command line.

    gradlew clean

#### Build the CPU and DSP binaries

The CPU and DSP binaries can be used to run and test the application outside of the APK using [standalone executables](#standalone-execution).

These binaries are built using the executable `build_cmake` provided in the SDK to build source code with `CMake`.

To build the application stubs and executable, execute the following command from the `app\src\main\cpp` folder:

        build_cmake android

To build the DSP shared objects, execute the following command from the `dsp` folder:

        build_cmake hexagon DSP_ARCH=vMKDOCS_DEF_DSP_ARCH_VERSION

Use `v69` instead of `vMKDOCS_DEF_DSP_ARCH_VERSION` to build the v69 flavor of the code if you want to test your code on a v69 target, and use the `BUILD` property to control whether to build a `Release`, `ReleaseG` or `Debug` flavor.

## Using the walkthrough script

The walkthrough script `android_app_walkthrough.py` automates the steps of signing the device, building, pushing and running the `android_app` example. You can run the walkthrough script with the dry-run (-DR) option to display all the commands that the script would execute without actually running them.

Review the generic [setup](../README.md#setup) and [walkthrough_scripts](../README.md#walkthrough-scripts) instructions to learn more about setting up your device and using the walkthrough script.

## Multithreading support

### CPU multithreading

In order to simulate a scenario in which independant applications co-exist and both request DSP services, the CPU launches two threads `app_0_thread` and `app_1_thread` using the `pthread` library.  The main thread waits for the completion of both threads by using `pthread_join`.

Both CPU threads execute the same function `launch_application` with different data. This function, in turn, makes FastRPC calls to the DSP.

### DSP multithreading

Multithreading on the DSP is achieved by using the QuRT APIs directly or by relying on the DSP worker pool library instead.  The `android_app` example illustrates how to use the DSP worker pool to launch a number of independent workers to execute worker tasks.

## Message logging

The `android_app` application illustrates how to generate and capture messages from different contexts:

* Java targeting the application processor
* Native C/C++ running on the application processor
* C/C++ running on the DSP

*** Note: *** It is also possible to generate messages from the DSP using the `qprintf` library as illustrated in the [qprintf example](../qprintf/README.md).

### Generating messages

#### Java

With Java, messages can be generated using the `android.util.Log` library.  A tag associated to each log message helps identify where these messages come from and filter them in or out as needed.  A common practice consists of defining a `TAG` variable as `private static final String` and setting it to either the class simple name with the `getSimpleName()` class method or `getName()`, which prepends the simple class name with its package name.  For this project, we adopt the convention of using the full name to easily identify all Java messages related to this example by greping for `com.example.android_app`.

#### C/C++ for the application processor

C describing code running on the cpu will be compiled with the NDK tools, which support both the `printf` and `__android_log_print` functions.  The Android log library is useful to send messages to logcat when the APK is running, while the printf library can simply be used to send messages to the command window directly when running a test executable.  Since both of these approaches are useful for source used to build either the APK or a cpu executable, we define a `LOG` macro in `logs.h` that calls both libraries when creating a message:

    #define LOG(...) {__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__);printf(__VA_ARGS__);}

We set `TAG` to the package name `com.example.android_app`, thus allowing logcat to capture messages written both in Java and native C by simply searching for the package name.  Another approach may be to set the `TAG` to `ANDROID_APP` or any other short string easy to search for when using [logcat](../../tools/messaging.md#logcat) or [mini-dm](../../tools/messaging.md#mini-dm).

***Notes: ***

* A file that is solely used to build a command line executable may simply use `printf` to send messages out.

* Starting with Android API 30, `__android_log_set_minimum_priority` allows to control the level of logging to be displayed.  To remain compatible with older versions, this function is commented out in the code.  Android log messages are simply enabled or disabled manually by commenting them in or out.

#### C/C++ for the DSP

To send messages from C code running on the DSP, using [FARF APIs](../../tools/messaging.md#generating-messages) is the preferred approach.  These messages may be captured both with `mini-dm` and `logcat`.

### Capturing messages

The base SDK [explains](../../tools/messaging.md) how to use [logcat](https://developer.android.com/studio/command-line/logcat) to capture messages.  Since logcat captures all device messages, it is generally necessary to filter them out, either by using the logcat filtering options (`-s`, `-e`, `-m`, etc.) or piping its output to grep for further filtering as follows:

    adb logcat | grep -iE "`android_app`|adsprpc"

***Note:*** In the `android_app` project, FARF messages are enabled conditionally from the DSP at runtime using the `HAP_setFARFRuntimeLoggingParams` API and based on the value of the `verbose_mask` configuration variable.  For these messages to be directed in logcat, a `com.example.android_app.farf` also needs to be created on the DSP library path.  This is accomplished by writing this file to the path provided by `getExternalCacheDir` to which the APK has access, and by adding this path to the `DSP_LIBRARY_PATH` environment variable.

## APK library packaging

The APK includes all the binaries required to run the application on any target.
This allows the application to choose at run-time which library flavor to load depending on both the CDSP version supported by the target and whether the target supports asynchronous FastRPC.

The `build.gradle` file configures the project to run `CMake`, which builds the application-side executable and its libraries.  Once the build is complete, `build.gradle` pushes to the `src/main/jniLibs` folder the DSP binaries in order for them to be packaged as part of the APK binaries.

At runtime, the `MainActivity` class loads the library path where all APK binaries are present using `getApplicationInfo().nativeLibraryDir`. The class then passes the String to the native code using  JNI in order to set the `DSP_LIBRARY_PATH` environment variable.  This allows the DSP to find the location of the shared object upon loading the DSP libraries.

## Multi-target support

### Skel selection

Prior to opening the DSP shared object, we query the DSP version using the `remote_handle_control` API. We then set the skel URI according to both the DSP version and whether the application needs to run synchronously or asynchronously.

### Stub selection

In order to support multiple targets most efficiently, the stub libraries are also loaded dynamically.  (Most Hexagon SDK examples link the stubs dynamically.)

When the user has requested to run the application asynchronously, the application attempts to load the asynchronous library stub.  If this fails, the application concludes that the target does not have asynchronous support and the application settings are changed to run the application synchronously.

The synchronous library stub is also loaded dynamically.  A failure is then considered a fatal error.

## Asynchronous support

The `android_app` example can launch the DSP tasks synchronously or asynchronously.

From the DSP side, the asynchronous skel implementation is a simple wrapper around the corresponding synchronous functions.

From the CPU side, the asynchronous support comes with the following additions to the synchronous code base:

* An `async_callback_fn` callback function invoked whenever the DSP completes an asynchronous call
* An `async_call_context_t` handle allowing the callback function to access the context of the asynchronous call.
    The context includes variables such as the output array produced by the DSP and its attributes. It also includes a `num_failed_tests` allowing to track how many buffers were processed correctly.
* Management of a `fastrpc_async_descriptor` asynchronous descriptor that allows to specify how the framework should notify the CPU when an asynchronous task has completed and define the context aforementioned.
* A `async_semaphore` semaphore ensuring no more than `MAX_PENDING_ASYNC_TASKS` asynchronous tasks are pending at one time. This allows to constrain the memory space required to buffer up requests to be sent to the DSP.
* `async_num_tasks_completed` and `async_num_tasks_submitted` variables to track the number of asynchronous tasks completed by and submitted to the DSP.
* An `async_completion_condition` condition variable and its associated `async_completion_mutex` mutex for updating safely the number of tasks completed and submitted.  After all tasks have been submitted, the CPU halts its execution by waiting on the completion condition.  From the callback invoked when an asynchronous task has been completed, the CPU locks the mutex, updates the number of tasks completed, and, if that number matches the number of tasks submitted, sends a completion signal before unlocking the mutex. This signal will only have an effect if the main CPU thread was already waiting on the completion condition.

***Note: *** Most calls in the asynchronous library are still synchronous.  In a typical application only a small number of FastRPC calls benefit from being asynchronous.  This example illustrates how to make selected calls asynchronous while leaving others as regular synchronous FastRPC calls to minimize code changes. This example by default runs on synchronous FastRPC.

## Compute resource management

The `android_app` example illustrates some of the features of the compute resource manager: in order to launch the DSP workers, the DSP is expected to reserve a chunk of VTCM memory.  The UI allows the user to specify how much VTCM memory each application needs to request, and whether the request is serialized or not.  The resource acquisition time-out is configurable in the `resource_acquire_timeout_us` field of the `android_app_configs` structure but not from the UI.

## Standalone execution

In addition to the APK, the `android_app` example comes with two Android executables allowing the user to run the example outside its GUI.

### Test executable

The test executable implemented in `android_app_test.c` runs a number of unit tests that exercise specific application features.  A couple of macros automate some recurring tests:

* `CHECK_VALUE` is used to compare an actual value against a reference.  Errors result in error messages and incrementing an error count.
* `CHECK_STATS` is used to compare the value of an `android_app_stats` field against a reference.  Errors result in the same behavior as the `CHECK_VALUE` macro.

The test executable takes one optional argument allowing to run only one of the unit tests.

### Main executable

The main executable implemented in `android_app_main.c` is a simple executable that exposes the main application configurations thus allowing the user to run simple usage scenarios directly outside the GUI.

## Data structures

It is common for APIs to use data structure to capture complex types.  The `android_app` example illustrates how to share application configurations and statistics across all major code components:

* The user interface
* The Java code
* The application native code
* The DSP code

This data sharing is accomplished as follows:

* The DSP and application processor share the same data structures, `android_app_stats` and `android_app_configs`, which are declared in the IDL file `android_app_defs.idl`

    * The `android_app_configs` structure is set by the application processor and marked as read-only when passed to the DSP
    * The `android_app_stats` structure is updated by the DSP after each run
    * When all tasks have been executed, the application processor queries the DSP to retrieve its final statistics and then updates them based on data collected on the application side

* The C and Java code base running on the application processor communicate using JNI

    The `jni.cpp` source implements a `populate_stats_java` and `populate_configs_c` functions to respectively

    * Populate the fields of the native jobject  representing the Java class `ApplicationStats` from the C structure `android_app_stats`
    * Populate the fields of the C structure android_app_configs from the native jobject representing the Java class `ApplicationConfigs`

    ***Note:*** To keep the GUI simpler, some application configuration fields are not exposed through the GUI and simply set to some default values in the `populate_configs_c` method instead.

* The UI exchanges data structures across views using the Android `Bundle` class

    The `ApplicationConfigs` and `ApplicationStats` classes encapsulate the logic for transferring data between the bundle and the class

    * A constructor that takes a Bundle as input to create a class and populate its fields from the bundle elements
    * A `getBundle()` method that returns a Bundle with elements populated from the class fields available

    All string keys of the bundles are also maintained in the Java `ApplicationConfigs` and `ApplicationStats` classes

## Helper scripts

The `scripts` folder includes a script intended to automate some common tasks:

### create_as_project.py

This script automates the following steps, which you may perform manually instead:

* Update `local.properties` to

    * Set `sdk.dir` to default Android SDK install location.  For example, `C:\Users\%USERNAME%\AppData\Local\Android\sdk`.
    * Set `ndk.dir` to a valid full NDK install.  For example, `%HEXAGON_SDK_ROOT%\tools\android-ndk-r25c`.
    * Set `cmake.dir` to the Hexagon SDK `cmake` directory: `%HEXAGON_SDK_ROOT%\tools\utils\cmake-3.22.2-win64-x64`

* Update `gradle.properties` to set `android.bundle.enableUncompressedNativeLibs` to `false` and ensure the DSP shared objects included in the SDK are not compressed.

* Update the contents of the `app` folder with the code provided in the project example.

* Create a `dsp` folder with the contents of the DSP code provided in the project example.

* Update `build.gradle` to

    * Use `externalNativeBuild` to use `cmake` as the toolchain and configure it to use the proper CMakeLists.txt file and CMake arguments
    * Pass the proper arguments and target name to cmake
    * Use `packagingOptions` to exclude `libcdsprpc.so` from the APK library folder to ensure the APK relies on the library present on device
    * Copy to the JniLibs folder the DSP shared objects and stubs to be included in the APK
    * Create different packaging rules whether the build configuration is `Debug` or `Release`

    For more details on any of these steps, please refer to the provided `build.gradle` file.

* Update `gradle` and related scripts in the new project.

You can obtain the script usage description with the `-h` option:

    usage: create_as_project.py [-h] output

    Turn an empty native AS project into the android_app AS example or prepare
    properties file for existing android_app example.

    positional arguments:
      output      Location of the native AS project.

    optional arguments:
      -h, --help  show this help message and exit

***Note: *** If you want to turn the existing `android_app` folder from the Hexagon SDK into an Android Studio project ready to build (as opposed to creating the project in a separate folder), simply pass the example path itself to the script: `python create_as_project.py ..`

## Troubleshooting

* `Unsupported ABI for NDK r19+: ARMEABI`

    If upon attempting to build the AS project you get `Unsupported ABI for NDK r19+: ARMEABI`, it probably means that while you are pointing to the Android NDK installed with the Hexagon SDK, you didn't install the full NDK as part of the Hexagon SDK.

* `CMake 3.14.3 or higher is required`

    If you follow the [setup instructions](#setup) properly, the `cmake.dir` variable in the file `local.properties` should point to the version of CMake included in the Hexagon SDK. This version is higher than the minimum required version in the `CMakeLists.txt` file and should therefore not lead to this error.  If your `local.properties` file is properly setup and you still get the error above, you might still be using the CMake executable included in Android Studio by default, which has a version lower than the minimum required in the `CMakeLists.txt` files.  This may happen when using Build --> Rebuild Project or Build --> Clean Project instead of using Build --> Make Project.

* `Execution failed for task ':app:compileDebugJavaWithJavac'

    If this error is seen while building the APK from the command line manually or by using `android_app_walkthrough.py -a`, then it means a valid `JDK` is either not installed or is not accessible to the `gradlew` daemon. To fix this issue, make sure `JDK` is installed and is added to the system path.
