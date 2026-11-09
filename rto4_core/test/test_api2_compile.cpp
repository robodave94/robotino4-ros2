#include <gtest/gtest.h>

#include "rec/robotino/api2/Com.h"
#include "rec/robotino/api2/OmniDrive.h"
#include "rec/robotino/api2/Odometry.h"
#include "rec/robotino/api2/MotorArray.h"
#include "rec/robotino/api2/c/Com.h"

TEST(Api2Compile, CppSurfaceConstructs)
{
  rec::robotino::api2::Com com;
  rec::robotino::api2::OmniDrive drive;
  rec::robotino::api2::Odometry odom;
  rec::robotino::api2::MotorArray motors;
  drive.setComId(com.id());
  odom.setComId(com.id());
  motors.setComId(com.id());
  SUCCEED();
}

TEST(Api2Compile, CSurfaceConstructsAndDestroysCom)
{
  const ComId id = Com_construct();
  ASSERT_GE(id, 0);
  EXPECT_TRUE(Com_destroy(id));
}
