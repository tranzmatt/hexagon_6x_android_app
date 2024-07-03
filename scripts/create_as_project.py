# ==============================================================================
#  Copyright (c) 2021 Qualcomm Technologies, Inc.
#  All rights reserved. Qualcomm Proprietary and Confidential.
# ==============================================================================

# This script assists with the process of creating an Android Studio project from the source
# provided in this example folder
#
# Steps to perform prior to running the current script
#
# * Open Android Studio -- (Tested with version 4.0)
# * Create a new Android ***Native C++*** project
# * Name the project `android_app`
# * Select Java as the language
# * Use default toolchain
# * Close Android Studio
# * Run this script
# * Reopen Android Studio and confirm you can build the project

import os
import sys
import shutil
import argparse

def safe_escape(string):
    return string.replace("\\","\\\\").replace(":","\\:")

if not os.getenv('SDK_SETUP_ENV'):
    sys.exit("SDK Environment not set up. Please run setup_sdk_env script from the SDK root directory.")
hexagon_sdk_location=os.getenv('HEXAGON_SDK_ROOT')
hexagon_tools_location=os.getenv('DEFAULT_HEXAGON_TOOLS_ROOT')

android_ndk_location=hexagon_sdk_location+"\\tools\\android-ndk-r25c"
if os.getenv('ANDROID_ROOT_DIR'):
    android_ndk_location=os.getenv('ANDROID_ROOT_DIR')

cmake_exe_location=hexagon_sdk_location+"\\tools\\utils\\cmake-3.17.0-win64-x64"
if os.getenv('CMAKE_ROOT_PATH'):
    cmake_exe_location=os.getenv('CMAKE_ROOT_PATH')

user = os.getenv('USERNAME')
android_sdk_location="C:\\Users\\" + user + "\\AppData\\Local\\Android\\sdk"

parser = argparse.ArgumentParser(description='Turn an empty native AS project into the android_app AS example or prepare properties file for existing android_app example.')
parser.add_argument('output', action="store", help='Location of the native AS project.')
args = parser.parse_args()

source = os.path.normpath(os.path.join(os.getcwd(),"..\\"))
destination = os.path.abspath(args.output)

print("Source:                " + source)
print("Destination:           " + destination)
print("Android SDK location:  " + android_sdk_location)
print("Android NDK location:  " + android_ndk_location)
print("CMAKE location:        " + cmake_exe_location)

self = False
if os.path.samefile(destination, source):
    self = True

if os.path.exists(destination + "\\local.properties"):
    os.remove(destination + "\\local.properties")
with open(destination + "\local.properties", "a") as local_properties:
    local_properties.write("\nsdk.dir="+safe_escape(android_sdk_location))
    local_properties.write("\ncmake.dir="+safe_escape(cmake_exe_location))

if os.path.exists(destination + "\\gradle.properties"):
    os.remove(destination + "\\gradle.properties")
with open(destination + "\\gradle.properties", "a") as gradle_properties:
    gradle_properties.write("\nandroid.useAndroidX=true")
    gradle_properties.write("\nandroid.bundle.enableUncompressedNativeLibs=false")
    gradle_properties.write("\nHEXAGON_SDK_ROOT="+safe_escape(hexagon_sdk_location))
    gradle_properties.write("\nHEXAGON_TOOLS_ROOT="+safe_escape(hexagon_tools_location))
    gradle_properties.write("\nANDROID_ROOT_DIR="+safe_escape(android_ndk_location))

search_text = str("${HEXAGON_SDK_ROOT}/tools/android-ndk-r25c")
replace_text = os.path.relpath(android_ndk_location, destination)
replace_text = replace_text.replace("\\","/")
data = ""
with open(destination + "\\app\\build.gradle", "r") as build_gradle:
    data = build_gradle.read()
    data = data.replace(search_text, replace_text)

with open(destination + "\\app\\build.gradle", "w") as build_gradle:
    build_gradle.write(data)

if self == True:
    sys.exit("\napp is ready to be built!")

if os.path.exists(destination+"\\app\\src"):
    shutil.rmtree(destination+"\\app\\src")

print("Copying " + source + "\\app\\src" + " to " + destination + "\\app\\src")
shutil.copytree(source+"\\app\\src",destination+"\\app\\src")
shutil.copy(source + "\\app\\build.gradle", destination + "\\app\\build.gradle")

if not os.path.exists(destination + "\\dsp"):
    print("Copying " + source + "\\dsp" + " to " + destination + "\\dsp")
    shutil.copytree(source + "\\dsp", destination + "\\dsp")

if not os.path.exists(destination + "\\scripts"):
    print("Copying " + source + "\\scripts" + " to " + destination + "\\scripts")
    shutil.copytree(source + "\\scripts", destination + "\\scripts")

if not os.path.exists(destination + "\\gradle"):
    print("Copying " + source + "\\gradle" + " to " + destination + "\\gradle")
    shutil.copytree(source + "\\gradle", destination + "\\gradle")
    shutil.copy(source + "\\gradlew", destination + "\\gradlew")
    shutil.copy(source + "\\gradlew.bat", destination + "\\gradlew.bat")
    shutil.copy(source + "\\settings.gradle", destination + "\\settings.gradle")
    shutil.copy(source + "\\build.gradle", destination + "\\build.gradle")

print("\app is ready to be built!")
