// code compatible with these codegen versions
#define __CUSTOM_COMPATLEVEL_110

#include <randNormal.hxx>

void MyBlip::move(double dt)
{
  dx += dueca::randNormal() * dt;
  dy += dueca::randNormal() * dt;
  x += dx * dt;
  y += dy * dt;
}
