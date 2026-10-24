#pragma once
struct TouchWakeGate {
 bool swallow=false,held=false,began=false;
 void suppressUntilRelease(){swallow=true;held=true;began=false;}
 bool accept(bool pressed,bool asleep){began=pressed&&!held;held=pressed;if(!pressed){swallow=false;return false;}if(asleep)swallow=true;return !swallow;}
};
