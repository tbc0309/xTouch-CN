#pragma once
namespace BoardGeometry {
struct Point {int x,y;};
inline Point p4Native(int x,int y){return {479-y,x};}
inline bool p4Touch(int nx,int ny,Point &p){if(nx<0||nx>=480||ny<0||ny>=800)return false;p={ny,479-nx};return true;}
inline bool s3Touch(int nx,int ny,Point &p){if(nx<0||nx>=240||ny<0||ny>=320)return false;p={ny,239-nx};return true;}
}
