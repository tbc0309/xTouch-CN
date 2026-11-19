#pragma once
namespace BoardGeometry {
struct Point {
    int x, y;
};
inline Point portraitNative(int nativeWidth, int x, int y) {
    return {nativeWidth - 1 - y, x};
}
inline bool portraitTouch(int nativeWidth, int nativeHeight, int nx, int ny, Point &p) {
    if (nx < 0 || nx >= nativeWidth || ny < 0 || ny >= nativeHeight)
        return false;
    p = {ny, nativeWidth - 1 - nx};
    return true;
}
inline Point p4Native(int x, int y) {
    return portraitNative(480, x, y);
}
inline bool p4Touch(int nx, int ny, Point &p) {
    return portraitTouch(480, 800, nx, ny, p);
}
inline bool s3Touch(int nx, int ny, Point &p) {
    return portraitTouch(240, 320, nx, ny, p);
}
} // namespace BoardGeometry
