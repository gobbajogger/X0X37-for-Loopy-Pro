// SPDX-License-Identifier: GPL-3.0-only
// Original public-API AU host. Manifest-driven fresh instances; no editor/device/presets.
#import <Foundation/Foundation.h>
#include <AudioToolbox/AudioToolbox.h>
#include <vector>
#include <cmath>
#include <algorithm>
static void ck(OSStatus s,const char*w){if(s){fprintf(stderr,"STOP %s %d\n",w,(int)s);exit(10);}}
static OSStatus tempo(void*c,Float64*b,Float64*t){if(b)*b=*(double*)c/22050.;if(t)*t=120;return 0;}
int main(int argc,char**argv){@autoreleasepool{
 if(argc!=4)return 2;
 NSArray*trials=[NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:@(argv[1])] options:0 error:nil];if(!trials)return 3;
 AudioComponentDescription desc={kAudioUnitType_MusicDevice,'KG05','KORG',0,0};auto comp=AudioComponentFindNext(NULL,&desc);if(!comp)return 4;UInt32 ver=0;ck(AudioComponentGetVersion(comp,&ver),"version");if(ver!=196870)return 5;
 NSMutableArray*receipts=[NSMutableArray array];
 for(NSDictionary*trial in trials){AudioUnit au=nullptr;ck(AudioComponentInstanceNew(comp,&au),"new");
 AudioStreamBasicDescription fmt={};fmt.mSampleRate=44100;fmt.mFormatID=kAudioFormatLinearPCM;fmt.mFormatFlags=kAudioFormatFlagIsFloat|kAudioFormatFlagIsPacked|kAudioFormatFlagIsNonInterleaved;fmt.mBytesPerPacket=fmt.mBytesPerFrame=4;fmt.mFramesPerPacket=1;fmt.mChannelsPerFrame=2;fmt.mBitsPerChannel=32;
 ck(AudioUnitSetProperty(au,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&fmt,sizeof(fmt)),"format");AudioStreamBasicDescription got={};UInt32 size=sizeof(got);ck(AudioUnitGetProperty(au,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&got,&size),"readformat");if(got.mSampleRate!=44100||got.mChannelsPerFrame!=2)return 6;
 UInt32 block=256;ck(AudioUnitSetProperty(au,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&block,sizeof(block)),"block");double clock=0;HostCallbackInfo cb={};cb.hostUserData=&clock;cb.beatAndTempoProc=tempo;ck(AudioUnitSetProperty(au,kAudioUnitProperty_HostCallbacks,kAudioUnitScope_Global,0,&cb,sizeof(cb)),"callback");ck(AudioUnitInitialize(au),"init");
 NSMutableArray*params=[NSMutableArray array];NSArray*values=trial[@"chicago"];
 if(values.count!=22)return 7;
 for(UInt32 i=0;i<22;i++){float v=[values[i] floatValue],r;ck(AudioUnitSetParameter(au,i,0,0,v,0),"set");ck(AudioUnitGetParameter(au,i,0,0,&r),"read");if(fabs(v-r)>1e-6)return 8;AudioUnitParameterStringFromValue sv={i,&r,NULL};size=sizeof(sv);ck(AudioUnitGetProperty(au,kAudioUnitProperty_ParameterStringFromValue,0,0,&sv,&size),"display");[params addObject:@{@"id":@(i),@"requested":@(v),@"readback":@(r),@"display":(__bridge NSString*)sv.outString}];if(sv.outString)CFRelease(sv.outString);}
 ck(MusicDeviceMIDIEvent(au,0xb0,123,0,0),"alloff");int frames=[trial[@"frames"] intValue];NSArray*events=trial[@"events"];int ei=0;
 std::vector<float>l(block),r(block),data(frames*2);struct{UInt32 n;AudioBuffer b[2];}abl={2,{{1,block*4,l.data()},{1,block*4,r.data()}}};
 for(int pos=0;pos<frames;){while(ei<(int)events.count&&[events[ei][0] intValue]==pos){NSArray*e=events[ei++];ck(MusicDeviceMIDIEvent(au,[e[1] intValue],[e[2] intValue],[e[3] intValue],0),"midi");}int n=std::min(256,frames-pos);if(ei<(int)events.count)n=std::min(n,[events[ei][0] intValue]-pos);for(int edge:{11025,16538,22050,33075,44100,66150})if(edge>pos)n=std::min(n,edge-pos);if(n<=0)return 9;abl.b[0].mDataByteSize=abl.b[1].mDataByteSize=n*4;clock=pos;AudioTimeStamp ts={};ts.mSampleTime=pos;ts.mFlags=kAudioTimeStampSampleTimeValid;AudioUnitRenderActionFlags flags=0;ck(AudioUnitRender(au,&flags,&ts,0,n,(AudioBufferList*)&abl),"render");for(int j=0;j<n;j++){if(!std::isfinite(l[j])||!std::isfinite(r[j]))return 11;data[2*(pos+j)]=l[j];data[2*(pos+j)+1]=r[j];}pos+=n;}
 NSString*path=[@(argv[2]) stringByAppendingPathComponent:[trial[@"id"] stringByAppendingString:@".au.f32"]];NSData*bytes=[NSData dataWithBytes:data.data() length:data.size()*4];if(![bytes writeToFile:path atomically:YES])return 12;
 ck(AudioUnitUninitialize(au),"uninit");ck(AudioComponentInstanceDispose(au),"dispose");[receipts addObject:@{@"id":trial[@"id"],@"version":@(ver),@"rate":@(got.mSampleRate),@"frames":@(frames),@"parameters":params,@"events":events,@"freshInstance":@YES}];
 NSData*json=[NSJSONSerialization dataWithJSONObject:receipts options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:nil];if(![json writeToFile:@(argv[3]) atomically:YES])return 13;
 }return 0;
}}
