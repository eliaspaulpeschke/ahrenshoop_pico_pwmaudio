if  make -j4; then
    openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg -c "program audiopwm.elf verify reset exit"
    minicom -b 115200 -o -D /dev/ttyACM0 
fi
