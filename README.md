mlvg
=========

mlvg is a cross-platform application and plugin framework.


to build:
---------

First, build and install madronalib as a static library using the instructions in the madronalib project.

Download the VST3 SDK from Steinberg.

You'll also need to download the CoreAudio SDK from Apple. It's easiest if you put the CoreAudio SDK in a directory next to the VST SDK---this way the VST SDK should find it automatically. The version of the CoreAudio SDK you want comes in a folder "CoreAudio" and has four subfolders: AudioCodecs, AudioFile, AudioUnits and PublicUtility. A current link: https://developer.apple.com/library/archive/samplecode/CoreAudioUtilityClasses/CoreAudioUtilityClasses.zip

To make the VST and AU plugins, first create an XCode project for MacOS using cmake:

- mkdir build
- cd build
- cmake -DVST3_SDK_ROOT=(your VST3 SDK location) -DCMAKE_BUILD_TYPE=Debug -GXcode ..



Cmake will create a project with obvious placeholders (llllCompanyllll, llllPluginNamellll) for the company and plugin names. 

Then, open the project and build all. Links to VST3 plugins will be made in ~/Library/Audio/Plug-Ins/VST3. The au component will be copied to ~/Library/Audio/Plug-Ins/Components.

Creating Windows projects should be very similar. You will need to install python and cmake. Instead of -GXcode for the generator argument you will specify something like -G "Visual Studio 14 2015 Win64".

note: currently using command line on Windows:
cmake -DVST3_SDK_ROOT="~/dev/vst3sdk" -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 14 2015 Win64" ..


note: currently using command line on Mac OS:
cmake -DVST3_SDK_ROOT="~/dev/vst3sdk" -GXcode ..


to clone:
---------

The python scripts here in examples/vst make it easy to create a new plugin project. To do this, run
`./clonePlugin.py destdir PlugName (company) (mfgr) (subtype) (url) (email)`. 
The vst directory will be copied to the specified destination, with a lot of searching and replacing done in various files to make the new plugin project consistent. 

example:
`./clonePlugin.py ~/dev NewHappyPlug 'Madrona Labs' MLbs hapy http://www.madronalabs.com mailto:support@madronalabs.com
`

Then you can move to the new directory, make a build directory and run cmake as before. 

TODO automatically remove the clonePlugin script and related files from the clone, where they are useless after the search / replace operations. For now do this manually.



building - MacOS
----------------

Building the (PlugName) target creates the .vst3 plugin in build/VST3/(Debug / Release) and copies it to /Library/Audio/Plug-Ins/VST3.

Building the (PlugName_au) target creates an intermediate .vst3 named (PluginName_vst_for_au), copies that into the (PluginName_au).component and copies that commponent to the /Library/Audio/Plug-Ins/Components directory. 

