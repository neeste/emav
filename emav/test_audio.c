#include <CoreAudio/CoreAudio.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    AudioObjectID aoid = kAudioObjectSystemObject;
    AudioObjectPropertyAddress propaddr = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    UInt32 sz = 0;
    OSStatus err = AudioObjectGetPropertyDataSize(aoid, &propaddr, 0, NULL, &sz);
    if (err) return 1;

    int nd = sz / sizeof(AudioDeviceID);
    AudioDeviceID *adid = (AudioDeviceID *)malloc(sz);
    AudioObjectGetPropertyData(aoid, &propaddr, 0, NULL, &sz, adid);

    for (int i = 0; i < nd; i++) {
        CFStringRef name = NULL;
        UInt32 namesz = sizeof(name);
        AudioObjectPropertyAddress nameaddr = {
            kAudioObjectPropertyName,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        AudioObjectGetPropertyData(adid[i], &nameaddr, 0, NULL, &namesz, &name);

        char cname[256] = {0};
        if (name) {
            CFStringGetCString(name, cname, sizeof(cname), kCFStringEncodingUTF8);
            CFRelease(name);
        }

        UInt32 insz = 0, outsz = 0;
        AudioObjectPropertyAddress inaddr = {
            kAudioDevicePropertyStreamConfiguration,
            kAudioDevicePropertyScopeInput,
            kAudioObjectPropertyElementMain
        };
        AudioObjectGetPropertyDataSize(adid[i], &inaddr, 0, NULL, &insz);
        AudioBufferList *inlist = malloc(insz);
        AudioObjectGetPropertyData(adid[i], &inaddr, 0, NULL, &insz, inlist);
        int inchans = 0;
        for (int j=0; j<inlist->mNumberBuffers; j++) inchans += inlist->mBuffers[j].mNumberChannels;

        AudioObjectPropertyAddress outaddr = {
            kAudioDevicePropertyStreamConfiguration,
            kAudioDevicePropertyScopeOutput,
            kAudioObjectPropertyElementMain
        };
        AudioObjectGetPropertyDataSize(adid[i], &outaddr, 0, NULL, &outsz);
        AudioBufferList *outlist = malloc(outsz);
        AudioObjectGetPropertyData(adid[i], &outaddr, 0, NULL, &outsz, outlist);
        int outchans = 0;
        for (int j=0; j<outlist->mNumberBuffers; j++) outchans += outlist->mBuffers[j].mNumberChannels;

        printf("ID: %u, Name: '%s', InChannels: %d, OutChannels: %d\n", adid[i], cname, inchans, outchans);
    }
    return 0;
}
