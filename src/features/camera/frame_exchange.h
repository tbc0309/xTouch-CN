#pragma once
#include <utility>

namespace CameraBuffer {
// Called with the exchange mutex held. The UI only exchanges ready/shown;
// the producer keeps exclusive ownership of decoded until this publication.
template <class Pixel> inline void publish(Pixel *&decoded, Pixel *&ready) {
    std::swap(decoded, ready);
}
} // namespace CameraBuffer
