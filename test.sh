# copy source files to test location
cp src/controller.ino test
cp src/config.h test
cp src/sensor.ino test
arduino --board arduino:avr:leonardo --verify test/test.ino
