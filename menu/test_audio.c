#include <CoreAudio/CoreAudio.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    AudioObjectID aoid = kAudioObjectSystemObject;
    AudioObjectPropertyAddress propaddr;
    UInt32 sz = 0;
    AudioDeviceID *adid = NULL;
    int nd = 0;

    propaddr.mSelector = kAudioHardwarePropertyDevices;
    propaddr.mScope = kAudioObjectPropertyScopeGlobal;
    propaddr.mElement = 0;

    if (AudioObjectGetPropertyDataSize(aoid, &propaddr, 0, NULL, &sz) == noErr && sz > 0) {
        adid = (AudioDeviceID *) malloc(sz);
        if (AudioObjectGetPropertyData(aoid, &propaddr, 0, NULL, &sz, adid) == noErr) {
            nd = sz / sizeof(AudioDeviceID);
        }
    }
    
    printf("Number of devices: %d\n", nd);
    
    for (int i=0; i<nd; i++) {
        char name[256] = {0};
        UInt32 namesz = sizeof(name);
        propaddr.mSelector = kAudioDevicePropertyDeviceName;
        propaddr.mScope = kAudioObjectPropertyScopeGlobal;
        propaddr.mElement = 0;
        
        if (AudioObjectGetPropertyData(adid[i], &propaddr, 0, NULL, &namesz, name) == noErr) {
            printf("Device %d: %s\n", i, name);
        }
    }

    return;
}
