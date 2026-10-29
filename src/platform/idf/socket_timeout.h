#pragma once
#include <stdint.h>
namespace xtouch_idf {
constexpr unsigned socket_timeout_ms(unsigned seconds){
 return seconds>UINT32_MAX/1000u?UINT32_MAX:seconds*1000u;
}
}
