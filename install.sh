echo -e "\n########################################################################";
echo -e "INSTALLING ARDUINO IDE"
echo "########################################################################";

# if .travis.yml does not set version
if [ -z $ARDUINO_IDE_VERSION ]; then
export ARDUINO_IDE_VERSION="1.8.8"
echo "NOTE: YOUR .TRAVIS.YML DOES NOT SPECIFY ARDUINO IDE VERSION, USING $ARDUINO_IDE_VERSION"
fi

# if newer version is requested
if [ ! -f $HOME/arduino_ide/$ARDUINO_IDE_VERSION ] && [ -f $HOME/arduino_ide/arduino ]; then
echo -n "DIFFERENT VERSION OF ARDUINO IDE REQUESTED: "
shopt -s extglob
cd $HOME/arduino_ide/
rm -rf *
cd $OLDPWD
fi

# if not already cached, download and install arduino IDE
echo -n "ARDUINO IDE STATUS: "
if [ ! -f $HOME/arduino_ide/arduino ]; then
echo -n "DOWNLOADING: "
wget --quiet https://downloads.arduino.cc/arduino-$ARDUINO_IDE_VERSION-linux64.tar.xz
if [ $? -ne 0 ]; then echo -e "\xe2\x9c\x96"; else echo -e "\xe2\x9c\x93"; fi
echo -n "UNPACKING ARDUINO IDE: "
[ ! -d $HOME/arduino_ide/ ] && mkdir $HOME/arduino_ide
tar xf arduino-$ARDUINO_IDE_VERSION-linux64.tar.xz -C $HOME/arduino_ide/ --strip-components=1
if [ $? -ne 0 ]; then echo -e "\xe2\x9c\x96"; else echo -e "\xe2\x9c\x93"; fi
touch $HOME/arduino_ide/$ARDUINO_IDE_VERSION
else
echo -n "CACHED: "
echo -e "\xe2\x9c\x93"
fi

# add the arduino CLI to our PATH
export PATH="$HOME/arduino_ide:$PATH"

echo -e "\n########################################################################";
echo -e "INSTALLING DEPENDENCIES"
echo "########################################################################";

arduino --pref "boardsmanager.additional.urls=http://digistump.com/package_digistump_index.json" --save-prefs 2>&1
arduino --install-boards digistump:avr 2>&1
arduino --pref "compiler.warning_level=all" --save-prefs 2>&1
arduino --board digistump:avr:digispark-tiny8 --save-prefs 2>&1
