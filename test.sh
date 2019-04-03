# copy source files to test location
cp src/config.ino test
cp src/d_controller.ino test
cp src/d_sensor.ino test
arduino --board arduino:avr:leonardo --verify test/test.ino
