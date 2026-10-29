#pragma once
/* Pending jobs must run independently of transport exclusivity. On constrained
   boards release TLS first; PSRAM boards keep processing their MQTT session. */
template<class Pause,class Poll,class Start>
void xtouch_network_job_pump(bool exclusive,bool pending,Pause pause,Poll poll,Start start){
 if(exclusive)pause();else poll();
 if(pending)start();
}
