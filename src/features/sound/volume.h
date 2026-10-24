#pragma once
// Leave 2.7 dB of PCM headroom at full user volume; never apply positive gain.
inline unsigned xtouch_sound_peak(unsigned percent){return 24000u*(percent>100?100:percent)/100u;}

// Shared 5 ms linear envelope, independent of the board sample rate.
inline float xtouch_sound_envelope(unsigned position,unsigned total,unsigned rate=16000){
 if(position>=total)return 0.f;
 unsigned edge=position<total-position?position:total-position;
 const unsigned fade=rate/200;
 return edge>=fade?1.f:float(edge)/fade;
}
