#!/usr/bin/env python
# child python script

### Imports ###
import os
import sys
import argparse
import platform
import subprocess
import fileinput

# Check if SDK_SETUP_ENV is set or not for checking whether environment is set or not.
# Exit walkthrough if SDK_SETUP_ENV is not set.
if not os.getenv('SDK_SETUP_ENV'):
    sys.exit("\nSDK Environment not set up -> please run setup_sdk_env script from SDK's root directory.")

hexagon_sdk_root = os.getenv('HEXAGON_SDK_ROOT')
android_root = os.getenv('ANDROID_ROOT_DIR')
cmake_dir = os.getenv('CMAKE_ROOT_PATH')
cmake_exe = os.path.normpath(os.path.join(hexagon_sdk_root, "build", "cmake"))
script_dir = os.path.normpath(os.path.join(hexagon_sdk_root, "utils", "scripts"))
example_path = os.path.dirname(os.path.abspath(__file__))
example_dsp_path = os.path.normpath(os.path.join(example_path, "dsp"))
example_app_path = os.path.normpath(os.path.join(example_path, "app", "src", "main", "cpp"))

# Import common walkthrough.
sys.path.append(script_dir)
import common_walkthrough as CW

def add_args_to_parser(parser):
    parser.add_argument('-apk', '--apk', dest='apk', action='store_true', help='Prepare the project and build the APK')
    parser.add_argument('-p', '--sdk-path', dest='sdk', type=str, metavar='', help='Set the full path of the Android SDK used to build the APK')

def safe_escape(string):
    return string.replace("\\","\\\\").replace(":","\\:")

def build_and_install_apk(options, device):

    print("> Checking system requirements to build APK")
    if platform.system() != "Windows":
        sys.exit("ERROR: Creating AS project is supported only on Windows platform currently.")

    try:
        subprocess.call(["java", "-version"])
    except:
        sys.exit("ERROR: JAVA is not installed")

    try:
        subprocess.call(["javac", "-version"])
    except:
        sys.exit("ERROR: JDK is not installed")

    android_sdk_dir = ""
    user = os.getenv('USERNAME')
    android_sdk = os.path.normpath(os.path.join("C:\\Users", user, "AppData", "Local", "Android", "sdk"))
    if options.sdk:
        android_sdk_dir = os.path.normpath(options.sdk)
    elif os.path.isdir(android_sdk):
        android_sdk_dir = android_sdk
    else:
        sys.exit("ERROR: Android SDK is not installed in the location: {}".format(android_sdk))
    print("> Android SDK PATH: {}".format(android_sdk_dir))

    android_root_dir = ""
    if os.path.isdir(os.path.join(android_root, "sources", "third_party")):
        android_root_dir = android_root
    else:
        sys.exit("ERROR: Android NDK is not installed. Install FULL Android NDK to build APK.")
    print("> Android NDK PATH: {}".format(android_root_dir))

    if os.path.exists(os.path.join(cmake_dir, "bin", "ninja.exe")):
        print("> CMake Tools PATH: {}".format(cmake_dir))
    else:
        sys.exit("ERROR: Ninja application not found in CMake Tools Path.")

    local_properties = os.path.normpath(os.path.join(example_path, "local.properties"))
    if os.path.exists(local_properties):
        os.remove(local_properties)
    with open(local_properties, "a") as lp:
        lp.write("\nsdk.dir={}".format(safe_escape(android_sdk_dir)))
        lp.write("\ncmake.dir={}".format(safe_escape(cmake_dir)))

    gradle_properties = os.path.normpath(os.path.join(example_path, "gradle.properties"))
    if os.path.exists(gradle_properties):
        os.remove(gradle_properties)
    with open(gradle_properties, "a") as gp:
        gp.write("\nandroid.useAndroidX=true")
        gp.write("\nandroid.bundle.enableUncompressedNativeLibs=false")
        gp.write("\nHEXAGON_SDK_ROOT={}".format(safe_escape(hexagon_sdk_root)))
        gp.write("\nHEXAGON_CMAKE_ROOT={}".format(safe_escape(cmake_dir)))
        gp.write("\nANDROID_ROOT_DIR={}".format(safe_escape(android_root)))

    ndkPath = ""
    build_gradle = os.path.normpath(os.path.join(example_path, "app", "build.gradle"))
    bg = open(build_gradle, "r")
    for line in bg:
        if "ndkPath" in line:
            ndkPath = line
    bg.close()

    ndkPathUpdated = safe_escape(os.path.relpath(android_root_dir, example_path))
    ndkPathUpdated = "            ndkPath \"{}\"\n".format(ndkPathUpdated)
    for line in fileinput.input(build_gradle, inplace=1):
       line = line.replace(ndkPath, ndkPathUpdated)
       sys.stdout.write(line)

    print("\nApp is ready to be built!\n\n")

    config = "Debug"
    if "Release" in options.build_config:
        config = "Release"

    verbose = "--quiet"
    if options.verbose == True:
        verbose = "--stacktrace"

    clean_cmd = "gradlew clean {}".format(verbose)
    build_cmd = "gradlew assemble{} {}".format(config, verbose)
    built_apk_dir = os.path.normpath(os.path.join(example_path, "app", "build", "outputs", "apk"))

    print("> Building the project")
    print("[This may take a few minutes ...\nPlease wait ...]\n")
    print(clean_cmd)
    os.system(clean_cmd)
    print(build_cmd)
    os.system(build_cmd)

    built = False
    apk_dir = os.path.normpath(os.path.join(built_apk_dir, config))
    if os.path.isdir(apk_dir):
        print("\n> Build successful\n> APK is available here: {}".format(apk_dir))
        built = True

    if built == False:
        print("\n> Build failed")
        return

    install_cmd = "adb install {}\\app-debug.apk".format(apk_dir)
    print("You can install the APK using the command {}".format(install_cmd))
    return

def build_and_run_app(options, device):

    # building for HLOS
    hlos_dir = device.build_hlos(example_app_path)
    # building for dsp
    dsp_dir = device.build_hexagon(example_dsp_path)

    android_root_dir = android_root
    if sys.platform == "win32":
        libcpp_shared = '{}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/aarch64-linux-android/'.format(android_root_dir)
    else:
        libcpp_shared = '{}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/'.format(android_root_dir)

    app_name = 'android_app'
    app_test = 'android_app_test'

    android_app ='{}/{}/ship/android_app'.format(example_app_path,hlos_dir)
    android_app_test ='{}/{}/ship/android_app_test'.format(example_app_path,hlos_dir)

    libandroid_app ='{}/{}/ship/libandroid_app.so'.format(example_app_path,hlos_dir)
    libandroid_app_async ='{}/{}/ship/libandroid_app_async.so'.format(example_app_path,hlos_dir)
    libcpp = '{}/libc++_shared.so'.format(libcpp_shared)

    libandroid_app_skel_v68 ='{}/{}/ship/libandroid_app_skel_v68.so'.format(example_dsp_path,dsp_dir)
    libandroid_app_async_skel_v68 ='{}/{}/ship/libandroid_app_async_skel_v68.so'.format(example_dsp_path,dsp_dir)
    libandroid_app_skel_v69 ='{}/{}/ship/libandroid_app_skel_v69.so'.format(example_dsp_path,dsp_dir)
    libandroid_app_async_skel_v69 ='{}/{}/ship/libandroid_app_async_skel_v69.so'.format(example_dsp_path,dsp_dir)
    libandroid_app_skel_v73 ='{}/{}/ship/libandroid_app_skel_v73.so'.format(example_dsp_path,dsp_dir)
    libandroid_app_async_skel_v73 ='{}/{}/ship/libandroid_app_async_skel_v73.so'.format(example_dsp_path,dsp_dir)
    libandroid_app_skel_v75 ='{}/{}/ship/libandroid_app_skel_v75.so'.format(example_dsp_path,dsp_dir)
    libandroid_app_async_skel_v75 ='{}/{}/ship/libandroid_app_async_skel_v75.so'.format(example_dsp_path,dsp_dir)

    executables = [android_app, android_app_test]
    hlos_libs = [libandroid_app, libandroid_app_async, libcpp]
    all_dsp_libs = [libandroid_app_skel_v68, libandroid_app_async_skel_v68, libandroid_app_skel_v69, libandroid_app_async_skel_v69, libandroid_app_skel_v73, libandroid_app_async_skel_v73, libandroid_app_skel_v75, libandroid_app_async_skel_v75]
    dsp_libs = list()
    for lib in all_dsp_libs:
        if os.path.exists(lib):
            dsp_libs.append(lib)

    device.copy_binaries_to_target(executables,hlos_libs,dsp_libs)

    args = [" 10 4 2 1 1 100 1 0 "]
    device.run(app_name, args)
    device.run(app_test, "")
    return


def main():

    # Adding example arguments to common walkthrough parser
    add_args_to_parser(CW.parser)
    # Making object of class device
    device = CW.device()

    options = CW.get_options()
    android_app_supported_targets = ['v75', 'lanai', 'v73', 'lahaina', 'waipio', 'kailua', 'tofino', 'neo', 'netrani', 'camano', 'palawan', 'milos']
    if options.target not in android_app_supported_targets:
        print("\n {} target is not supported by android_app example\n".format(options.target))
        sys.exit()

    if options.HLOS != 'LA':
        sys.exit("android_app is supported only on Android devices")

    if options.apk:
        build_and_install_apk(options, device)
    else:
        build_and_run_app(options, device)
    return


if __name__ == '__main__':
    main()
    sys.stdout.flush()
