## How to build the player software on Linux

### bladeRF

#### Build and install libbladeRF

https://github.com/Nuand/bladeRF/wiki/Getting-Started:-Linux

#### Build bladeplayer

```
$ make bladeplayer
```

### HackRF One

#### Build and install libhackrf

https://github.com/mossmann/hackrf/tree/master/host

#### Build hackplayer

```
$ make hackplayer
```

### LimeSDR

#### Build and install libLimeSuite

http://wiki.myriadrf.org/Lime_Suite

#### Build limeplayer

```
$ make limeplayer
```

### ADALM-Pluto

#### Build and install libiio

https://wiki.analog.com/resources/tools-software/linux-software/libiio

#### Build and insatall libad9361

```
$ git clone https://github.com/analogdevicesinc/libad9361-iio.git
$ cd libad9361-iio
$ cmake ./
$ make all
$ sudo make install
```

#### Build plutoplayer

```
$ make plutoplayer
```

### YunSDR

#### Build and insatall libyunsdr

```
$ git clone https://github.com/v3best/libyunsdr
$ cd libyunsdr
$ mkdir build
$ cd build
$ cmake ../
$ make && sudo make install && sudo ldconfig
```

#### Build yunsdrplayer

```
$ make yunsdrplayer
```