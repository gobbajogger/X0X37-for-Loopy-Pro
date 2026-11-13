from pathlib import Path
import wave,hashlib,json,numpy as np,shutil
O=Path(__file__).resolve().parent;src=O.parents[1]/'assets/audio/approved-master.wav';assert hashlib.sha256(src.read_bytes()).hexdigest()=='19dbec641016005cde1e3b9fa4c89497e7eab8c252840d5791dbca3794febb14'
with wave.open(str(src)) as w:assert (w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getnframes())==(1,2,44100,120960);a=np.frombuffer(w.readframes(120960),dtype='<i2').copy()
shutil.copy2(src,O/'inputs/approved-master.wav')
frames=[]
for frame in range(1,193):
 # Native engine scope consumes every second output sample; native UI latches <=10Hz.
 t=(frame-1)/70;sample=min(len(a),int(t*10)//1*4410);sample=int(sample);window=np.zeros(256,dtype=np.int16);take=a[max(0,sample-256):sample];window[-len(take):]=take if len(take) else 0
 values=window[1::2];frames.append(values.tolist())
(O/'approved-scope-native128.json').write_text(json.dumps({'audio_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'frames':frames,'fps':70,'samples':120960,'native_scope_decimation':2,'latch_hz':10,'window_samples':256,'gain':1,'audio_unchanged':True,'anchors':{'first_snare_attack':15925,'first_snare_front':15923,'first_snare_peak':16226,'stretch_snare':76200,'grain_curve':83400,'last_kick':106080,'crush':113400}},separators=(',',':')))
