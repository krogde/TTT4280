# add this to run commands from the shell
import subprocess
# to delete file
import os
# import PiCamera from the picamera library
from picamera import PiCamera
from time import sleep

# Create an instance of PiCamera called camera
# This is an object which has several attributes we can change as you
# will see below.
camera = PiCamera()
# The resolution is one of the things we can change. Try some different
# ones if you want to, but make sure to choose a frame rate which is
# supported by the resolution you choose
camera.resolution = (1640, 922)
camera.framerate = 40
# Set a low ISO. This should not be changed. 0 means auto so avoid that too
camera.iso = 10
# Add a bit of dealy here so that the camera has time to adjust its settings.
# Skipping this will cause effects that may be unwanted.
sleep(2)
# switch these two off so that we can manually control the awb_gains
camera.exposure_mode = 'off'
camera.awb_mode = 'off'
camera.awb_gains = 1

# change this to the path where you want the .h264 file to be saved
savePath = "/home/pi/Documents/github/TTT4280/"
# change this to what you want to name the file
fileName = "vid.h264"
# how long we want to record
recordTime = 30

# If we were not running the Pi headless (without a monitor) startring the
# preview would show us what the camera was capturing.
# Now, since we run it headless, it allows the rest of the settings to be
# set.
print "Starting preview"
camera.start_preview()
#print "Sleep to set WB"
sleep(5)
print "Start recording"
# Concatenate savePath and fileName strings and save file to that location,
# in my case here: /mnt/shared_pi_folder/optics_lab/videos/test.h264
camera.start_recording(savePath+fileName)
# Record for the amount of time we want
camera.wait_recording(recordTime)
# When finished, stop recording and stop preview.
camera.stop_recording()
print "Recording finished"
camera.stop_preview()

###########################################################################
## The following section is where we wrap the .h264 into a mp4 container ##
###########################################################################
# If we were to skip this first try statement, that deletes the mp4 file if
# it already exist we would get some weird results. The command that takes
# care of the wrapping MP4Box -add <fielname>. The -add can be used several
# times, i.e, to wrap both a video and an audio file. However, if we do not
# delete an already existing file we would add another video file to the
# container, and this is not what we want.
# Thus, we first try to remove the file. This will be successfully done
# if it exist. If not it will jump to the except. Here we will get an error
# message telling us the file does not exist, but as this is what we want
# we do not have to do anything about that.
try:
    f = savePath+"mp4/"+fileName[:-5]+".mp4"
    os.remove(f)
    print "removed " + savePath+"mp4/"+fileName[:-5]+".mp4"
except Exception as e:
	pass
    # Entering here means file did not exist, so we can proceed
    # without doing anything
# This command enters the shell and runs a command as we would there
# here that command would be:
# MP4Box -fps 40 -add /mnt/shared_pi_folder/optics_lab/videos/test.h264 /mnt/shared_pi_folder/optics_lab/videos/mp4/test.mp4
# The -fps option sets the fps information that will be contained in the mp4 file
# that can be read by a media player to play it at the correct frame rate.
# The next is the -add command which chooses which file to add to the output.
# Here we add the .h264 file to the output mp4 file
subprocess.check_output(["MP4Box", "-fps", "40", "-add", savePath+fileName,savePath+"mp4/"+fileName[:-5]+".mp4"])
print "Program finished successfully"
